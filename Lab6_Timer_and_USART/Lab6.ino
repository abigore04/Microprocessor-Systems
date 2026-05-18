#define F_CPU 16000000UL

#include <avr/io.h>         // registers' names
#include <avr/interrupt.h>  // ISR, sei, cli
#include <stdint.h>         // uint8_t, uint32_t
#include <stdlib.h>         // utoa

// -------------------- Global state --------------------
volatile uint8_t current_digit = 0;
volatile uint8_t paused = 0;        // 0 when counts, 1 when paused

// For debounce
volatile uint32_t ms_ticks = 0;         // after 1ms -> 1, 10ms ->10 ...
volatile uint32_t last_button_time = 0;

#define DEBOUNCE_MS 50 // 50ms debounce

// Debug print request from ISR -> main
volatile uint8_t debug_event = 0;   // 0 = none, 1 = stopped, 2 = resume
volatile uint16_t debug_tcnt1 = 0;

// -------------------- 7-segment lookup table --------------------
// bit0=a, bit1=b, bit2=c, bit3=d, bit4=e, bit5=f, bit6=g
static const uint8_t segLUT[10] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};

// -------------------- UART debug --------------------
static void USART0_init(void)
{
    UBRR0H = 0;
    UBRR0L = 103; // 9600 baud at 16 MHz

    UCSR0B = (1 << TXEN0);                    // TX only
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  // 8N1
}

static void USART0_sendChar(char c)
{
    while (!(UCSR0A & (1 << UDRE0)));
    UDR0 = c;
}

static void USART0_sendString(const char *s)
{
    while (*s) {
        USART0_sendChar(*s++);
    }
}

static void USART0_sendUint16(uint16_t value)
{
    char buf[6]; // max 65535 + '\0'
    utoa(value, buf, 10);
    USART0_sendString(buf);
}

static void USART0_sendLine(const char *label, uint16_t value)
{
    USART0_sendString(label);
    USART0_sendUint16(value);
    USART0_sendString("\r\n");
}

// atomic read of 16-bit TCNT1
static uint16_t timer1_read_atomic(void)
{
    uint8_t sreg = SREG;
    cli();
    uint16_t value = TCNT1;
    SREG = sreg;
    return value;
}

// -------------------- Display functions --------------------
static void display_digit(uint8_t digit)
{
    uint8_t pattern = segLUT[digit];

    // PORTB:
    // PB0 -> A
    // PB1 -> B
    // PB2 -> C
    // PB3 -> D
    // PB4 -> E

    // ~0x1F = 1110 0000 - to zero lsb, keep msb (since do not use them)
    // OR it with pattern to get the segments lit
    PORTB = (PORTB & ~0x1F) | (pattern & 0x1F);

    // PORTC:
    // PC0 -> F
    // PC1 -> G

    // 1 << 5 is bit for f
    if (pattern & (1 << 5)) PORTC |=  (1 << PC0);
    else                    PORTC &= ~(1 << PC0);

    if (pattern & (1 << 6)) PORTC |=  (1 << PC1);
    else                    PORTC &= ~(1 << PC1);
}

static void display_init(void)
{
    // setting as output
    DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB4);
    DDRC |= (1 << DDC0) | (1 << DDC1);

    // make all segments 0. If common anode -> make one
    PORTB &= ~0x1F;
    PORTC &= ~((1 << PC0) | (1 << PC1));

    display_digit(0); // first 0 appears
}

// -------------------- Timer1 init --------------------
// CTC mode, OCR1A top, interrupt every 0.5 s
static void timer1_init(void)
{
    // resetting registers
    TCCR1A = 0x00;
    TCCR1B = 0x00;
    TCNT1  = 0x0000;

    OCR1A = 31249;

    TCCR1B |= (1 << WGM12);     // CTC mode
    TIMSK1 |= (1 << OCIE1A);    // Enable Timer1 Compare A interrupt

    // prescaler 256
    TCCR1B |= (1 << CS12);
}

// -------------------- Timer0 init for debounce --------------------
// 1 ms interrupt:
// 16 MHz / 64 = 250 kHz
// 250 counts = 1 ms
// OCR0A = 249 because counting starts from 0
static void timer0_init(void)
{
    TCCR0A = 0x00;
    TCCR0B = 0x00;
    TCNT0  = 0x00;

    OCR0A = 249;

    TCCR0A |= (1 << WGM01);                 // CTC mode
    TIMSK0 |= (1 << OCIE0A);                // Enable Timer0 Compare A interrupt
    TCCR0B |= (1 << CS01) | (1 << CS00);   // Prescaler 64
}

// -------------------- INT0 button init --------------------
// Button on PD2/INT0 to GND, internal pull-up enabled
static void int0_init(void)
{
    DDRD  &= ~(1 << DDD2);      // PD2 (int0) input
    PORTD |=  (1 << PORTD2);    // internal pull-up

    // Falling edge on INT0
    EICRA &= ~(1 << ISC00);
    EICRA |=  (1 << ISC01);

    EIFR  |=  (1 << INTF0);     // clear any pending INT0 flag
    EIMSK |=  (1 << INT0);      // enable INT0
}

// -------------------- ISRs --------------------
// triggers each 0.5s
ISR(TIMER1_COMPA_vect)
{
    current_digit++;
    if (current_digit > 9) {
        current_digit = 0;
    }
    display_digit(current_digit);
}

// triggers each 1ms
ISR(TIMER0_COMPA_vect)
{
    ms_ticks++;
}

// ext interrupt
ISR(INT0_vect)
{
    uint32_t now = ms_ticks;

    // Ignore bounces within debounce window
    if ((now - last_button_time) < DEBOUNCE_MS) {
        return;
    }

    last_button_time = now;
    paused ^= 1;  // toggling process

    if (paused) {
        // Stop Timer1 clock only, keep TCNT1 value
        TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
        debug_tcnt1 = timer1_read_atomic();
        debug_event = 1; // stopped
    } else {
        // Read value from which timer will continue
        debug_tcnt1 = timer1_read_atomic();
        debug_event = 2; // resuming

        // Resume Timer1 with same prescaler 256
        TCCR1B &= ~((1 << CS11) | (1 << CS10));
        TCCR1B |=  (1 << CS12);
    }
}

// -------------------- Main --------------------
int main(void)
{
    display_init();
    USART0_init();  // UART debug
    timer0_init();  // debounce time base
    int0_init();
    timer1_init();

    sei();

    while (1) {
        uint8_t event_local = 0;
        uint16_t tcnt_local = 0;

        // copy shared debug data atomically
        uint8_t sreg = SREG;
        cli();
        if (debug_event != 0) {
            event_local = debug_event;
            tcnt_local = debug_tcnt1;
            debug_event = 0;
        }
        SREG = sreg;

        if (event_local == 1) {
            USART0_sendLine("Stopped TCNT1 = ", tcnt_local);
        } else if (event_local == 2) {
            USART0_sendLine("Resume from TCNT1 = ", tcnt_local);
        }
    }
}