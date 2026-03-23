**Course:** Microprocessor Systems (ENCE-4731 - 20919) 
**Instructor:** Dr. Alexz Farrall  
**Lab Number & Title:**  Lab 3 - EEPROM
**Student Name:** Farid Ibadov  
**Student ID:** 17954  
**Program / Section:** BSCE26  
**Date:** March, 2026

---

## 1. Objective

The objective of this lab was to implement a counter that increments a single register by 1 every 1 second, using AVR language. After that, the number of CPU cycles and instructions that take place within this window had to be identified. Implementing a serial command allowing to store or reset the current counter value in the EEPROM, aster unplugging Arduino, and powering it up again, the counter was expected to continue from the last stored value, rather than starting all over again, unless reset button is pressed. Any anomalies or limitations were expected to be explained and the address location of the register used for increment is expected to be located.

## 2. Code Overview

```cpp
volatile uint8_t counter = 0;
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(9600);

  while (EECR & 0x02);
  EEAR = 0;
  EECR |= 0x01;
  uint8_t valueRead = EEDR;
  
  if (valueRead == 255) {
    counter = 0;
  } else {
    counter = valueRead;
  }

  Serial.print("Started. Counter value: ");
  Serial.println(counter);
  Serial.println("Send 'S' to Save to EEPROM, 'R' to Reset.");
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    asm volatile (
      "lds r24, counter \n\t"
      "inc r24 \n\t"
      "sts counter, r24 \n\t"
    );

    Serial.print("Counter: ");
    Serial.println(counter);
  }

  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'S' || cmd == 's') {
      while (EECR & 0x02);
      EEAR = 0;
      EEDR = counter;
      EECR |= 0x04;
      EECR |= 0x02;
      Serial.println(">>> Counter saved to EEPROM.");
    } 
    else if (cmd == 'R' || cmd == 'r') {
      counter = 0;
      while (EECR & 0x02);
      EEAR = 0;
      EEDR = counter;
      EECR |= 0x04;
      EECR |= 0x02;
      Serial.println(">>> Counter and EEPROM reset to 0.");
    }
  }
}
```

Global variable `volatile uint8_t counter` is used to hold the current value which is incrementing. `volatile` is used to avoid the compiler optimizing or cashing the variable, so it stays "untouched". By doing so, it says in the memory where the code expects it to be. It is also used in `asm volatile` ensuring assembly instructions are executed just as they are written.

At startup, the code reads the EEPROM address 0. 

> EEPROM itself is a 1kB non-volatile memory that can preserve the data written in it even after unplugging the power, working on the "latch" mechanism. Address zero is used in this code, since we store only one value in EEPROM, and 0 is the first available one, like a "thumb rule".  

```cpp
  while (EECR & 0x02);
  EEAR = 0;
  EECR |= 0x01;
  uint8_t valueRead = EEDR;
```

to read the value value of the EEPROM, we should make sure that EEPROM is not busy using `while (EECR & 0x02);`, meaning it is not performing write operation. If we access the EEPROM while it is in a busy state, the operation we are trying to perform may fail or result in incorrect data.
![image](images/Pasted%20image%2020260320154021.png)

**EECR**'s bit 1 (**EEPE**) is responsible for that:
![image](images/Pasted%20image%2020260320153326.png)
![image](images/Pasted%20image%2020260320153723.png)

After **EEPE** becomes zero, we can read the EEPROM value at 0 address, accessing it with `EEAR = 0;`
![image](images/Pasted%20image%2020260320153845.png)
Read by setting the 0 bit of **EECR**
![image](images/Pasted%20image%2020260320154006.png)
then store the value to `valueRead` using **EEDR**
![image](images/Pasted%20image%2020260320154208.png)

```cpp
  if (valueRead == 255) {
    counter = 0;
  } else {
    counter = valueRead;
  }
```

255 is the maximum value that can be stored in 8-bit register, so when the counter hits this value, it will be reset back to 0, again incrementing up to 255. 

![image](images/Pasted%20image%2020260320162218.png)

```cpp
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;

    asm volatile (
      "lds r24, counter \n\t"
      "inc r24 \n\t"
      "sts counter, r24 \n\t"
    );

    Serial.print("Counter: ");
    Serial.println(counter);
  }
```

If 1 second (1000milliseconds) passes, program loads value from SRAM inro the register `r24`, increments it, and writes back to SRAM.

```cpp
  if (Serial.available() > 0) {
    char cmd = Serial.read();

    if (cmd == 'S' || cmd == 's') {
      // WRITE TO EEPROM (Address 0)
      while (EECR & 0x02);     // Wait if EEPROM is busy, EEPE (EEPROM Program Enable)
      EEAR = 0;                // Set address to 0
      EEDR = counter;          // Set data to write
      EECR |= 0x04;            // Setting EEMPE (Master Write Enable), 
                               // stays active for 4 clk cycles.
      EECR |= 0x02;            // Setting EEPE (Program Enable) to start writing
      Serial.println(">>> Counter saved to EEPROM.");
    } 
    else if (cmd == 'R' || cmd == 'r') {
      counter = 0;
      // RESET EEPROM (Address 0) TO 0
      while (EECR & 0x02);     // Wait if EEPROM is busy
      EEAR = 0;                // Set address to 0
      EEDR = counter;          // Set data to write (0)
      EECR |= 0x04;            // Master write enable
      EECR |= 0x02;            // Start write
      Serial.println(">>> Counter and EEPROM reset to 0.");
    }
  }
```

Two commands can be passed through serial interface and therefore **write** in EEPROM:
- S - to **save** the current value to EEMPOM (address 0). 
```cpp
      while (EECR & 0x02);     // Wait if EEPROM if busy
      EEAR = 0;                // Set address to 0
      EEDR = counter;          // Set data to write
      EECR |= 0x04;            // Setting EEMPE
      EECR |= 0x02;            // Setting EEPE to start writing
```
- R - to **reset** the counter and store 0 to EEPROM
```cpp
      counter = 0;             // setting couter to 0
      while (EECR & 0x02);
      EEAR = 0;
      EEDR = counter;
      EECR |= 0x04;
      EECR |= 0x02;
```

As mentioned before in the ATmega datasheet snippets, to write in EEPROM using **EEPE**, first, **EEMPE** bit must be set to 1.
![image](images/Pasted%20image%2020260320155429.png)

This acts as a double security precaution, avoiding risk of accidental write.

## 3. Timing Analysis of AVR increment

Increment operation is implemented in inline AVR assembly using `lds`, `inc`, and `sts` instruction. Since the register operations are faster than SRAM access:
- `lds` takes 2 cycles.
- `inc` takes 1 cycle.
- `sts` takes 2 cycles.
with total
$$N_{cycles} = 2+1+2 = 5$$
at CPU frequency of 16 MHz:
$$T_{clk} = \frac{1}{16,000,000}=62.5 ns$$
so, by that formula
$$t=N_{cycles} \, \cdot \,T_{clk}$$
the execution time for the block used in increment is
$$t = 5 \, \cdot \, 32.5ns = 312.5ns$$
So the inline AVR assembly block, responsible for incrementing the counter, takes **3 instructions** and **5 CPU cycles** to execute. This takes around **312.5ns** of time

## 4. Register Used and Address Location

```cpp
    asm volatile (
      "lds r24, counter \n\t" // SRAM -> r24
      "inc r24 \n\t"          // "r24++"
      "sts counter, r24 \n\t" // r24 -> SRAM
    );
```

For increment register `r24` was used. Address of that register can be found by referring to the datasheet
![image](images/Pasted%20image%2020260320162010.png)
since `R17` is at 0x11, by simply counting, `R24` is at **0x18**.

## 5. Anomalies and Limitations

Possible anomaly:
- small timing drift may occur because the increment is exact in AVR assembly, but the 1-second update is scheduled by `millis()`, which depends on the Arduino timer and clock accuracy rather than a dedicated hardware timer routine.

Limitation in 255 being maximum value that can be stored is explained in the code overview section.
