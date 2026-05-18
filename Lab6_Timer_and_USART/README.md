# Lab 6 - Timer and USART

**Course**: Microprocessor Systems (ENCE-4731 - 20919)  
**Instructor**: Dr. Alexz Farrall  
**Lab Number & Title**: Lab 6 - Timer and USART  
**Student Name**: Farid Ibadov  
**Student ID**: 17954  
**Program / Section**: BSCE26  
**Date**: May, 2026  

---

## 1. Objective

The objective of this lab was to drive a single-digit 7-segment display using register-level AVR programming and parallel Input/Output communication. Display had to count from zero to nine with 500-millisecond interval between counts and then circle back to the zero.

To generate required 500-millisecond interval the Timer1 was used, in CTC mode. Regular push button which was connected to `PD2/INT0` was used to pause and resume the counter, since this pin is suitable for the external interrupts. When user presses the button, timer stops by clearing Timer1 clock-select bits. When user presses it once again, Timer1 resumes from exactly the same `TCNT1` value, so that the counter doesn't start from zero.

USART was also configured for debug output, where the code is able to print the TCNT1 value when timer is stopped and resumed.

---
## 2. Code Overview

Program is written with no Arduino's `setup()` and `loop()` function. Instead, it uses normal C `main()` function and directly configures ATmega328P registers, as we used to in previous labs.

The main idea is to configure:
1. the 7-segment display pins as outputs,
2. USART0 for debug messages,
3. Timer0 to generate 1 ms ticks for button debounce,
4. INT0 on `PD2` for pause/resume button,
5. Timer1 in CTC mode for 0.5-second display update.

```c
int main(void)
{
    display_init();
    USART0_init();
    timer0_init();
    int0_init();
    timer1_init();

    sei();

    while (1) {
        ...
    }
}
```

Important not to forget about `sei()` to enable global interrupts, since the most of the work is interrupt driven.

The 7-segment display is controlled using a lookup table, where each hex value corresponds to the binary value, bits of which represent the pin to control segments of the display and produced desired digit:

```c
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
```

Each bit represents one segment:

| bit | segment |
| --- | ------- |
| 0   | a       |
| 1   | b       |
| 2   | c       |
| 3   | d       |
| 4   | e       |
| 5   | f       |
| 6   | g       |

![image](images/Pasted%20image%2020260518090408.png)

`PB0-PB4` are used for segments `A-E`, while `PC0` and `PC1` are used for segments `F` and `G`.

```c
DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB4);
DDRC |= (1 << DDC0) | (1 << DDC1);
```

Since the display used in this lab has the configuration of common-cathode (5161as), writing logic 1 turns the corresponding segment ON, therefore, pins are set as outputs.

The function `display_digit()` writes the required pattern to `PORTB` and `PORTC`.

```c
PORTB = (PORTB & ~0x1F) | (pattern & 0x1F);
```

`0x1F = 0001 1111`, so only the lower five bits of `PORTB` are changed (`PB0-PB4`). Other bits of `PORTB` are preserved.

Timer1 used for the main 500-millisecond counting interval:

```c
TCCR1A = 0x00;
TCCR1B = 0x00;
TCNT1  = 0x0000;

OCR1A = 31249;

TCCR1B |= (1 << WGM12);
TIMSK1 |= (1 << OCIE1A);
TCCR1B |= (1 << CS12);
```

`WGM12 = 1` configures Timer1 in CTC mode.
	![image](images/Pasted%20image%2020260518091230.png)

`OCR1A = 31249` is the TOP value. 

$$
f_{\text{timer}} = \frac{f_{\text{CPU}}}{\text{prescaler}}
$$

$$
f_{\text{timer}} = \frac{16{,}000{,}000}{256} = 62{,}500 \text{ Hz}
$$

$$
T_{\text{tick}} = \frac{1}{62{,}500} = 16 \mu s
$$

$$
N_{\text{ticks}} = \frac{0.5s}{16\mu s} = 31{,}250
$$

$$
OCR1A = N_{\text{ticks}} - 1
$$

$$
OCR1A = 31{,}250 - 1 = 31{,}249
$$

`OCIE1A = 1` enables the Timer1 Compare Match A interrupt. 
	![image](images/Pasted%20image%2020260518091657.png)

`CS12 = 1` selects prescaler 256.
	![image](images/Pasted%20image%2020260518091442.png)

When Timer1 reaches `OCR1A`, this interrupt is triggered:

```c
ISR(TIMER1_COMPA_vect)
{
    current_digit++;
    if (current_digit > 9) {
        current_digit = 0;
    }
    display_digit(current_digit);
}
```

So every 0.5 seconds, the digit increases by one and after `9`, it resets back to `0`.

Timer0 is used only for debounce. It generates an interrupt every 1 ms and increments `ms_ticks`.

```c
OCR0A = 249;
TCCR0A |= (1 << WGM01);
TIMSK0 |= (1 << OCIE0A);
TCCR0B |= (1 << CS01) | (1 << CS00);
```

With 16 MHz clock and prescaler 64, `OCR0A = 249` gives 1 ms interrupt period.


$$
f_{\text{timer0}} = \frac{16{,}000{,}000}{64} = 250{,}000 \text{ Hz}
$$

$$
T_{\text{tick}} = \frac{1}{250{,}000} = 4 \mu s
$$

$$
N_{\text{ticks}} = \frac{1000\mu s}{4\mu s} = 250
$$

$$
OCR0A = 250 - 1 = 249
$$

The push button is connected to `PD2/INT0`. Internal pull-up is enabled:

```c
DDRD  &= ~(1 << DDD2);
PORTD |=  (1 << PORTD2);
```

The interrupt is configured on falling edge, as if it were configured on rising edge the interrupt would trigger only when button is released, since button uses internal pull-up:

```c
EICRA &= ~(1 << ISC00);
EICRA |=  (1 << ISC01);
EIFR  |=  (1 << INTF0);
EIMSK |=  (1 << INT0);
```

When the button is pressed, `PD2` goes from HIGH to LOW and triggers `INT0`.

Inside the external interrupt, variable `paused` is toggled:

```c
paused ^= 1;
```

If the system becomes paused and Timer1 is stopped by clearing CS (clock select) bits:

```c
TCCR1B &= ~((1 << CS12) | (1 << CS11) | (1 << CS10));
```

![image](images/Pasted%20image%2020260518092515.png)

This stops the Timer1 clock but does not clear `TCNT1` at all and timer value is preserved.

If system resumes, the same prescaler is enabled once again:

```c
TCCR1B &= ~((1 << CS11) | (1 << CS10));
TCCR1B |=  (1 << CS12);
```

So Timer1 continues from the same `TCNT1` value.

USART0 is configured just for debugging:

```c
UBRR0H = 0;
UBRR0L = 103;

UCSR0B = (1 << TXEN0);
UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
```

`UBRR0L = 103` sets 9600 baud for 16 MHz clock.

$$
UBRR = \frac{f_{\text{CPU}}}{16 \cdot \text{BAUD}} - 1
$$

$$
UBRR = \frac{16{,}000{,}000}{16 \cdot 9600} - 1
$$

$$
UBRR \approx 103
$$

Since `103` fits in the lower 8 bits:

$$
UBRR0H = 0
$$

$$
UBRR0L = 103
$$

![image](images/Pasted%20image%2020260422095309.png)

`TXEN0` enables transmission
	![image](images/Pasted%20image%2020260422093949.png)

`UCSZ01:0 = 11` selects 8-bit data format. As parity and extra stop bits aren't enabled, this is regular `8N1` UART format.
	![image](images/Pasted%20image%2020260422094700.png)

Debug output is not printed directly inside the interrupt - the ISR just saves the event and `TCNT1` value:

```c
debug_tcnt1 = timer1_read_atomic();
debug_event = 1;
```

then main loop prints it later:

```c
if (event_local == 1) {
    USART0_sendLine("Stopped TCNT1 = ", tcnt_local);
} else if (event_local == 2) {
    USART0_sendLine("Resume from TCNT1 = ", tcnt_local);
}
```

This is much better since UART transmission is slow and shouldn't be done right inside an ISR, as it can slow it down.

---

## 3. Program Flow

Program slow is simple: after initialization, Timer1 deals with the count interval, Timer0 just produces 1ms time base for button debounce, INT0 is responsible for pause/resume.

![image](images/mermaid-diagram%201.png)


---

## 4. Results and Evidence


![image](images/Pasted%20image%2020260518085144.png)

| Component / Signal | ATmega328P Pin | Arduino Pin | Direction | Purpose                                |
| ------------------ | -------------- | ----------- | --------- | -------------------------------------- |
| Segment A          | `PB0`          | `D8`        | Out       | Controls segment `a`                   |
| Segment B          | `PB1`          | `D9`        | Out       | `b`                                    |
| Segment C          | `PB2`          | `D10`       | Out       | `c`                                    |
| Segment D          | `PB3`          | `D11`       | Out       | `d`                                    |
| Segment E          | `PB4`          | `D12`       | Out       | `e`                                    |
| Segment F          | `PC0`          | `A0`        | Out       | `f`                                    |
| Segment G          | `PC1`          | `A1`        | Out       | `g`                                    |
| Push button        | `PD2 / INT0`   | `D2`        | In        | External interrupt for pause/resume    |
| USART TX           | `PD1 / TXD`    | `D1 / TX`   | Out       | Sends debug messages to Serial Monitor |
| Common cathode pin | `GND`          | `GND`       | -         | Common ground of 7-segment display     |

Serial output, showing the how `TCNT1` preserved its value:

![image](images/Pasted%20image%2020260518095105.png)

## 5. Conclusion

In this lab, Timer1 was successfully configured in CTC mode to update 5161as 7-segment display every 0.5 seconds. Display incremented from `0` to `9` using a lookup table and parallel Input-Output pins. The pause - resume function was handled through `INT0`, where Timer1 was stopped and resumed without clearing `TCNT1` value.

## 6. References

[1] A. Farrall, *Lecture 6: Communication Buses (UART)*, Microprocessor Systems, ADA University, 2026.
[2] Microchip Technology Inc., *AVR Instruction Set Manual*, DS40002198A, 2020.
[3] Atmel Corporation, *ATmega328P: 8-bit AVR Microcontroller with 32K Bytes In-System Programmable Flash Datasheet*, 7810D-AVR-01/15, 2015.

