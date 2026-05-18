# Lab 7 - SPI Communication

**Course**: Microprocessor Systems (ENCE-4731 - 20919)  
**Instructor**: Dr. Alexz Farrall  
**Lab Number & Title**: Lab 7 - SPI Communication  
**Student Name**: Farid Ibadov  
**Student ID**: 17954  
**Program / Section**: BSCE26  
**Date**: May, 2026  

---

## 1. Objective

The main goal of this lab was to implement the register-level SPI communication between two Arduino boards, while one was configured as the SPI Master, and another as SPI Slave.

The Master had to send three decimal values in sequential order, which circled back forever:

$$
85 \rightarrow 170 \rightarrow 255
$$

Values are sent once every second through SPI communication bus. Slave receives each byte from Master then prints received value to serial monitor in decimal, hexadecimal, and binary formats.

SPI communication was configured directly using AVR registers such as `SPCR`, `SPSR`, `SPDR`, `DDRB`, and `PORTB`. The main SPI lines used were `MOSI`, `MISO`, `SCK`, and `SS`.

---

## 2. Code Overview

This lab uses two separate Arduino programs. One program is uploaded to the Master Arduino, and the other one - to Slave Arduino. Arduino `Serial` and `delay()` are still used for printing/timing, but SPI itself is configured directly through AVR registers.

Two Arduinos are connected directly as follows:

| Master | Slave |
| ------ | ----- |
| SS     | SS    |
| MOSI   | MOSI  |
| MISO   | MISO  |
| SCK    | SCK   |


### 2.1 Master Code

Master stores defined values in an array:

```c
uint8_t values[] = {85, 170, 255};
```

These values are 8-bit values and fit directly inside one SPI transfer. In binary and hexadecimal format:

| Decimal | Binary     | Hex    |
| ------- | ---------- | ------ |
| `85`    | `01010101` | `0x55` |
| `170`   | `10101010` | `0xAA` |
| `255`   | `11111111` | `0xFF` |
The Master SPI pins are configured through `DDRB`:

```c
DDRB |= (1 << DDB2);  // ss as output
DDRB |= (1 << DDB3);   // mosi
DDRB |= (1 << DDB5);  // sck
DDRB &= ~(1 << DDB4); // miso as input
```

| SPI Signal | ATmega328P Pin | Arduino Pin | Master Direction |
| ---------- | -------------- | ----------- | ---------------- |
| `SS`       | `PB2`          | `D10`       | Output           |
| `MOSI`     | `PB3`          | `D11`       | Output           |
| `MISO`     | `PB4`          | `D12`       | Input            |
| `SCK`      | `PB5`          | `D13`       | Output           |

`MISO` is input because used to receive data from Slave side. `SS`, `MOSI`, and `SCK` are outputs - Master controls slave selection, sends data, and generates the clock. 

![image](images/Pasted%20image%2020260518114358.png)

This keeps `SS` HIGH when the Slave is not selected:

```c
PORTB |= (1 << PB2);
``` 

During transmission, `SS` is pulled LOW:

```c
PORTB &= ~(1 << PB2);
```

After transfer is complete - returned HIGH again:

```c
PORTB |= (1 << PB2);
```

![image](images/Pasted%20image%2020260518115251.png)

SPI itself is configured using `SPCR`:

```c
SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
```

![image](images/Pasted%20image%2020260518115320.png)

Here:

- `SPE = 1` enables SPI
- `MSTR = 1` configures as Master
- `SPR0 = 1` selects SPI clock divider (with `SPR1 = 0` -> /16)
- `CPOL = 0` and `CPHA = 0` are left at zero, so SPI Mode 0 "default"
- `DORD = 0` is also left at zero, so data is sent MSB first.

![image](images/Pasted%20image%2020260518115517.png)

The SPI clock frequency is:

$$
f_{SCK} = \frac{f_{CPU}}{16}
$$

$$
f_{SCK} = \frac{16\,000\,000}{16} = 1\,000\,000 \text{ Hz} = 1 \text{ MHz}
$$

To send one byte, the code writes the data to `SPDR`:

```c
SPDR = data;
```

![image](images/Pasted%20image%2020260518115543.png)

Writing to `SPDR` starts SPI transfer and hardware shifts 8 bits out on `MOSI`, while the clock pulses are generated on `SCK`.

```c
while (!(SPSR & (1 << SPIF)))
{
    // wait
}
```

![image](images/Pasted%20image%2020260518115625.png)

The code waits until `SPIF` becomes `1` - this flag means that the SPI transfer is complete.

### 2.2 Slave Code

On the Slave side only `MISO` configured as an output:

```c
DDRB |= (1 << DDB4);
```

`SS`, `MOSI`, and `SCK` are configured as inputs:

```c
DDRB &= ~(1 << DDB2); // ss
DDRB &= ~(1 << DDB3);  // mosi
DDRB &= ~(1 << DDB5); // sck
```

Slave doesn't generate a clock and does not decide the start of the communication - it obeys the Master, waiting until Master pulls slave select LOW and initiates clock pulses on `SCK`

Slave SPI is enabled using:

```c
SPCR = (1 << SPE);
```

Only `SPE` is set and `MSTR` remains `0`.

To receive a byte, Slave waits for the transfer complete flag:

```c
while (!(SPSR & (1 << SPIF)))
{
    // wait
}
```

When `SPIF` becomes `1`, this indicates that 8 bits were received. The received byte is then read from `SPDR`:

```c
uint8_t received = SPDR;
```

After that, the Slave reloads `SPDR` with dummy value:

```c
SPDR = 0x00;
```

Since SPI is a full-duplex communication protocol, the Slave data register still shifts data back through `MISO`. So dummy value is assigned, which will be ignored by Master.

Finally, the Slave prints received value in 3 formats:

```c
Serial.print("Slave received DEC: ");
Serial.print(receivedValue);

Serial.print(" | HEX: 0x");
Serial.print(receivedValue, HEX);

Serial.print(" | BIN: ");
Serial.println(receivedValue, BIN);
```

So if the Master sends `85`, the Slave should display it as:

```text
Slave received DEC: 85 | HEX: 0x55 | BIN: 1010101
```

The same should happen for `170` and `255`, once every second.

## 3. Results and Evidence

When both Arduinos were connected and flashed, the serial monitor provided the following continuous output stream.

![image](images/Pasted%20image%2020260518121444.png)

By changing all the values to `170` in code:

```
uint8_t values[] = {170, 170, 170};
```

and by connecting probes of the oscilloscope to the wires coming out of `SCK` and `MOSI` pins

![image](images/Pasted%20image%2020260518113030.png)

it became easier to capture the actual timing diagram of `SCK` and `MOSI`, and trace the actual binary value of 170, that was being transferred (can be done with any value):

![image](images/Pasted%20image%2020260518122232.png)

## 4. References

[1] A. Farrall, *Lecture 7: Serial Peripheral Interface (SPI)*, Microprocessor Systems, ADA University, 2026.
[2] Atmel Corporation, *ATmega328P: 8-bit AVR Microcontroller with 32K Bytes In-System Programmable Flash Datasheet*, 7810D-AVR-01/15, 2015.
[3] Microchip Technology Inc., *AVR Instruction Set Manual*, DS40002198A, 2020.
