**Course:** Microprocessor Systems (ENCE-4731 - 20919) 
**Instructor:** Dr. Alexz Farrall  
**Lab Number & Title:**  Lab 4 - Timers
**Student Name:** Farid Ibadov  
**Student ID:** 17954  
**Program / Section:** BSCE26  
**Date:** March, 2026

---

## 1. Objective

In this lab, an interrupt-driven system was build, where pressing a button connected to D2 turns an output pin ON for exactly 10 milliseconds and the turns it OFF automatically. Timer1 was configured to generate an interrupt every 1 millisecond, and this interrupt is used to control a 10-ms countdown, meaning interrupt triggers 10 times during this.

## 2. Code Overview

```cpp
volatile uint8_t countdown = 0; 

void setup() {
  DDRB |= (1 << DDB5); 
  PORTB &= ~(1 << PORTB5); 
  DDRD &= ~(1 << DDD2); 
  PORTD |= (1 << PORTD2); 
  EICRA = (EICRA & 0xFC) | 0x02; 
  EIMSK |= (1 << INT0); 
  TCCR1A = 0; 
  TCCR1B = 0; 
  TCNT1  = 0;
  OCR1A = 249; 
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11) | (1 << CS10); 
  TIMSK1 |= (1 << OCIE1A); 
  sei(); 
}

void loop() {
}

ISR(INT0_vect) {
  TCNT1=0;
  PORTB |= (1 << PORTB5); 
  countdown = 10; 
}

ISR(TIMER1_COMPA_vect) {
  if (countdown > 0) {
    countdown--;
    if (countdown == 0) {
      PORTB &= ~(1 << PORTB5); 
    }
  }
}
```

`volatile uint8_t countdown = 0;` - initialize counter

- `DDRB |= (1 << DDB5); ` - set the digital pin 13 as an output.

- `PORTB &= ~(1 << PORTB5);` - set the digital pin 13 LOW.

- `DDRD &= ~(1 << DDD2); ` - set digital pin 2 as an input.

- `PORTD |= (1 << PORTD2);` - set digital pin2 in internal pull-up mode (for button). On PD2, **INT0** is located, used later for interrupt.
![image](images/Pasted%20image%2020260320165731.png)

- `EICRA = (EICRA & 0xFC) | 0x02;` 
![image](images/Pasted%20image%2020260320165300.png)
0xFC = 0000 0011, meaning set **ISC01** to 1 and **ISC00** to 0, so the **falling edge** of **INT0** will generate an interrupt.
![image](images/Pasted%20image%2020260320165622.png)

- `EIMSK |= (1 << INT0);` - When the **INT0** bit is set (one) and the I-bit in the status register (SREG) is set (one), the external pin interrupt is enabled.
![image](images/Pasted%20image%2020260320165850.png)

- `TCCR1A = 0;` & `TCCR1B = 0;` - clearing the control registers to start fresh. 

- `TCNT1  = 0;` - resetting Timer1's timer

- `OCR1A = 249;` - configuring output compare register to 249, meaning, when the timer will make 250 ticks, interrupt will happen.

- `TCCR1B |= (1 << WGM12);` - configuring CTC waveform (Clear Timer on Compare Match mode). (Mode 4).
![image](images/Pasted%20image%2020260320170307.png)
![image](images/Pasted%20image%2020260320170325.png)

- `TCCR1B |= (1 << CS11) | (1 << CS10);` - configuring prescaler to 64.
![image](images/Pasted%20image%2020260320170520.png)

- `TIMSK1 |= (1 << OCIE1A);`  - when this **OCIE1A** is written to one, and the **I-flag** in the status register is set (interrupts globally enabled), the Timer/Counter1 output compare A match interrupt is enabled.
![image](images/Pasted%20image%2020260320170549.png)

- `sei();` - enable global interrupt. 

When D2 detect a falling edge, the following ISR is triggered:
```cpp
ISR(INT0_vect) {
  TCNT1=0;
  PORTB |= (1 << PORTB5); 
  countdown = 10; 
}
```

it resets the timer, sets the the digital pin HIGH (lights the LED), assigns 10 to the counter.

If timer is not reset, the signal will not show exact 10ms-interval, since it may capture the moment when CTC have not hit TOP value yet. 

This ISR is triggered by Timer1 every 1ms:
```cpp
ISR(TIMER1_COMPA_vect) {
  if (countdown > 0) {
    countdown--;
    if (countdown == 0) {
      PORTB &= ~(1 << PORTB5); 
    }
  }
}
```
so it counts down 10 times (10 milliseconds, since ISR is triggered by each millisecond passed), then turns on LED by pulling pin 13 low.

![image](images/Pasted%20image%2020260320171804.png)

since signal is high for only 10 milliseconds, it is hard to catch it on oscilloscope by pressing stop button. Instead, **Normal** mode should be selected and most importantly trigger level should be set to the lower voltage than the signal produces, to catch it.

## 3. Timer Calculation

It is crucial to set correct prescaler, so that **OCR1A** fits in it, as well as correct OCR1A value, so that interrupt happens exactly after 1 millisecond.
$$f_{timer}=\frac{16\,MHz}{prescaler}=\frac{16,000,000}{64}=250,000\,Hz$$
$$T_{tick} = \frac{1}{250,000}=0.000004=4 \micro s$$
so to get 1 ms, the timer must count
$$\frac{1ms}{4\micro s} = \frac{1000 \micro s}{4\micro s} = 250 \; ticks$$
Since CTC mode counts from 0
$$OCR1A = 250-1=249$$

## 4. Possible limitations

![image](images/Pasted%20image%2020260320172917.png)
![image](images/Pasted%20image%2020260320172950.png)
There is a possibility that if `INTF0` was already set before enabling `INT0`, the interrupt service routine could execute immediately. This may happen due to unexpected noise or other factors, resulting in a flipped bit. This behavior was not observed in our test, likely because no pending interrupt flag existed at startup.
