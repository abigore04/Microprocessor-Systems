# Microprocessor Systems

This repository contains my lab work for the **Microprocessor Systems** course, focused on low-level programming with the **ATmega328P / Arduino Uno** platform.

The main goal of this repository is to document how AVR-based systems work beyond only high-level Arduino functions. Across the labs, I worked with **register-level programming**, CPU registers, SREG flags, EEPROM, timers, interrupts, SPI, I2C/TWI, USART, and assembly-level control flow.

---

## Overview

The course started from simple LED blinking and then gradually moved closer to how the ATmega328P actually works internally. Instead of only using Arduino functions like `digitalWrite()` or `delay()`, many labs required direct control of AVR registers such as `DDRB`, `PORTB`, `TCCR1B`, `SPCR`, `TWBR`, `EECR`, and `SREG`.

The repository includes work with:

- high-level Arduino programming
- AVR register-level programming
- CPU registers and inline AVR assembly
- SREG flag analysis
- EEPROM read/write operations
- Timer1 CTC mode
- external interrupts with `INT0`
- USART debug output
- SPI Master/Slave communication
- I2C/TWI Master/Slave communication
- pull-up resistor behavior on I2C lines
- oscilloscope-based timing and signal verification
- jump instructions: `RJMP`, `JMP`, and `IJMP`

---

## Hardware / Tools Used

### Main Hardware

- **Arduino Uno**
- **ATmega328P**
- two Arduino Uno boards for SPI and I2C communication labs
- single-digit 7-segment display
- push buttons
- LEDs and resistors
- breadboard and jumper wires
- external 4.7 kΩ pull-up resistors for I2C
- oscilloscope / measurement tools

### Software / Tools

- **Arduino IDE**
- **Serial Monitor**
- **C / C++**
- **inline AVR assembly**
- ATmega328P datasheet
- AVR instruction set manual

---

## Labs Included

### Lab 1 - Flashing LED

This lab implemented 1 Hz blinking on the onboard Arduino LED using two different methods:

1. high-level Arduino code with `digitalWrite()`
2. register-level AVR code with `DDRB` and `PORTB`

**Main ideas:**

- using Arduino pin 13 / `PB5`
- configuring pin direction with `DDRB`
- setting and clearing output state with `PORTB`
- comparing `digitalWrite()` with direct register access
- measuring execution speed with oscilloscope
- comparing memory usage of both approaches

The important result was that both methods produced the same visible LED behavior, but register-level control was much faster and used less memory.

---

### Lab 2 - Registers

This lab created a serial-based subtraction task using CPU registers and inline AVR assembly.

**Main ideas:**

- reading two 8-bit values from Serial Monitor
- limiting input values to `0..255`
- loading values into AVR registers `r16`, `r17`, and `r18`
- performing subtraction using the `sub` instruction
- reading the `SREG` status register right after subtraction
- analyzing the **Zero (Z)** and **Carry (C)** flags
- understanding unsigned underflow in AVR subtraction

This lab was important because it showed that the Carry flag behaves differently in subtraction than many beginners expect. In unsigned subtraction, `C = 1` means a borrow occurred, meaning `A < B`.

---

### Lab 3 - EEPROM

This lab implemented a counter that increments once per second and can be saved into EEPROM.

**Main ideas:**

- using `volatile uint8_t counter`
- incrementing the counter with inline AVR assembly
- reading from EEPROM address `0`
- writing the counter value to EEPROM
- resetting both the counter and EEPROM through Serial commands
- using EEPROM control registers such as `EECR`, `EEAR`, and `EEDR`
- checking `EEPE` before EEPROM access
- using `EEMPE` before `EEPE` for safe EEPROM writing
- analyzing instruction count and CPU cycles for `lds`, `inc`, and `sts`

The counter could continue from the stored EEPROM value after power loss. This showed the difference between normal SRAM variables and non-volatile EEPROM storage.

---

### Lab 4 - Timers

This lab built an interrupt-driven output pulse system. Pressing a button connected to `D2 / INT0` turns the output ON for exactly 10 ms, and Timer1 turns it OFF automatically.

**Main ideas:**

- using `INT0` external interrupt on `PD2`
- using internal pull-up for the button
- configuring `EICRA` for falling-edge interrupt
- enabling external interrupt with `EIMSK`
- configuring Timer1 in **CTC mode**
- setting `OCR1A = 249` for 1 ms interrupt period
- using prescaler 64
- using a countdown variable to generate a 10 ms pulse
- verifying timing with oscilloscope

This lab showed why timer-based timing is better than manually waiting inside the button interrupt.

---

### Lab 5 - Jumps

This lab created a one-button LED firmware demonstrating three AVR jump styles:

- `RJMP`
- `JMP`
- `IJMP`

**Main ideas:**

- using `RJMP` for a local polling loop at boot
- using button press count to choose a mode
- using `JMP` to enter one of three mode labels
- using `IJMP` and the Z register pair `r31:r30` for jump-table dispatch
- cycling actions during a running mode
- combining Arduino runtime with AVR-style control flow

The final firmware worked like a small one-button LED gadget. First it waited for a button press, then counted presses during a 2-second window, then jumped into one of three blinking modes.

---

### Lab 6 - Timer and 7-Segment Display

This lab used register-level AVR programming to drive a single-digit 7-segment display and count from `0` to `9`.

**Main ideas:**

- driving a common-cathode 7-segment display
- using lookup table values for digit patterns
- controlling segments through `PORTB` and `PORTC`
- using Timer1 in CTC mode for a 500 ms display update interval
- calculating `OCR1A = 31249`
- using Timer0 as a 1 ms time base for debounce
- using `INT0` on `PD2` for pause/resume
- stopping Timer1 by clearing clock-select bits
- preserving `TCNT1` when paused
- using USART0 debug output to print stopped/resumed timer values

The main point was that the timer could be paused without clearing `TCNT1`, so after resuming the count continued from the same timer value instead of restarting.

---

### Lab 7 - SPI Communication

This lab implemented SPI communication between two Arduino Uno boards, where one board acted as SPI Master and the other as SPI Slave.

**Main ideas:**

- configuring SPI using AVR registers
- using `SPCR`, `SPSR`, `SPDR`, `DDRB`, and `PORTB`
- setting Master pins: `SS`, `MOSI`, and `SCK` as outputs
- setting `MISO` as input on Master
- enabling SPI with `SPE`
- enabling Master mode with `MSTR`
- selecting SPI clock divider using `SPR0`
- waiting for transfer completion using `SPIF`
- sending values `85`, `170`, and `255`
- observing `SCK` and `MOSI` on oscilloscope

The oscilloscope capture was used to confirm that the actual transmitted bit pattern matched the expected value, especially for `170` / `0xAA` / `10101010`.

---

### Lab 8 - I2C Communication

This lab implemented I2C communication between two Arduino Uno boards, one configured as Master and the other as Slave.

**Main ideas:**

- using Arduino `Wire` library for I2C communication
- manually configuring TWI speed with AVR registers
- using `TWBR`, `TWSR`, `TWCR`, `PRR`, `DDRC`, and `PORTC`
- using standard ATmega328P TWI pins:
  - `SDA = PC4 / A4`
  - `SCL = PC5 / A5`
- testing both 100 kHz and 400 kHz I2C speeds
- sending a command byte `0xA1` and LED state byte
- requesting one byte back from the Slave
- using external 4.7 kΩ pull-up resistors
- observing SDA/SCL waveforms on oscilloscope
- explaining ACK/NACK, START, STOP, and half-duplex bus behavior

This lab showed that even when no button was pressed, there was still I2C activity because the Master periodically requested the Slave button state. It also showed why stronger external pull-ups make the rising edges cleaner than weak internal pull-ups.

---

## Repository Structure

```text
Microprocessor-Systems/
├── Lab1_Flashing_LED/
├── Lab2_Registers/
├── Lab3_EEPROM/
├── Lab4_Timers/
├── Lab5_Jumps/
├── Lab6_Timer_and_USART/
├── Lab7_SPI_Communication/
└── Lab8_I2C_Communications/
```

---

## What This Repository Shows

This repository reflects my practical work in low-level microprocessor programming, especially:

- moving from high-level Arduino functions to direct register control
- understanding how AVR ports and bits map to Arduino pins
- using CPU registers directly through inline assembly
- reading and interpreting status flags
- working with non-volatile EEPROM memory
- configuring timers and interrupts manually
- using serial communication for debug and verification
- implementing SPI and I2C communication between microcontrollers
- validating digital signals with oscilloscope traces
- connecting datasheet register descriptions with real code behavior

---

## Notes

Some labs use Arduino functions where they are not the focus of the task, for example `Serial.print()` or `delay()`. However, the main logic in the register-level labs is implemented through direct ATmega328P register manipulation. The repository is mainly intended to show the practical transition from Arduino-style programming to lower-level AVR control.
