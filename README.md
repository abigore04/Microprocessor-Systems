# Microprocessor Systems

This repository contains my lab work for the **Microprocessor Systems** course, focused on low-level programming with the **ATmega328P / Arduino Uno** platform.

The main goal of this repo is to document my progress in understanding how AVR-based systems work beyond high-level Arduino functions — including **register-level programming**, **status flags**, **EEPROM**, **timers**, **interrupts**, and **assembly-level control flow**.

## Course Focus

This repository explores:

- Arduino programming in C/C++
- AVR register-level programming
- CPU registers and SREG flags
- EEPROM read/write operations
- Timer configuration and interrupt-driven design
- AVR jump instructions: `RJMP`, `JMP`, `IJMP`
- Practical timing verification with an oscilloscope

## Labs Included

### Lab 1 — Flashing LED
Implemented 1 Hz blinking on the onboard LED in two ways:
- high-level Arduino code using `digitalWrite()`
- register-level AVR programming using `DDRB` and `PORTB`

The two methods were then compared in terms of:
- execution speed
- memory usage
- abstraction vs efficiency tradeoff

### Lab 2 — Registers
Built a serial-based subtraction task using **CPU registers only**.

Main ideas:
- loading operands into AVR registers
- subtracting using inline AVR assembly
- reading and interpreting **SREG**
- analyzing **Zero (Z)** and **Carry (C)** flags
- understanding unsigned underflow behavior in AVR subtraction

### Lab 3 — EEPROM
Implemented a counter that:
- increments once per second
- can be saved to EEPROM
- can be restored after power loss
- can be reset through serial commands

Main ideas:
- EEPROM control registers
- persistent storage
- instruction/cycle counting
- timing analysis of AVR increment operations

### Lab 4 — Timers
Designed an **interrupt-driven** system where a button press triggers a 10 ms output pulse.

Main ideas:
- external interrupts with `INT0`
- Timer1 in **CTC mode**
- 1 ms periodic interrupt generation
- countdown-based pulse timing
- timing verification with oscilloscope

### Lab 5 — Jumps
Created a one-button LED firmware demonstrating three AVR jump styles:

- `RJMP` for a local polling loop
- `JMP` for mode switching
- `IJMP` for runtime action dispatch using a jump table

Main ideas:
- assembly-supported control flow
- mode selection by button press count
- action dispatch through computed jumps
- combining Arduino runtime with AVR-style logic

## Hardware / Platform

- **Board:** Arduino Uno
- **Microcontroller:** ATmega328P
- **Language:** C/C++ with inline AVR assembly
- **Tools:** Arduino IDE, Serial Monitor, Oscilloscope
