**Course:** Microprocessor Systems (ENCE-4731 - 20919) 
**Instructor:** Dr. Alexz Farrall  
**Lab Number & Title:**  Lab 1 - Flashing LED
**Student Name:** Farid Ibadov  
**Student ID:** 17954  
**Program / Section:** BSCE26  
**Date:** March, 2026

---
## 1. Objective

The main goal of this lab was to blink onboard LED for 1 Hz using 2 types of Arduino programming:
1. High-level Arduino Programming in C++
2. Regiter-level AVR programming

After that both methods should be compared in regard to memory allocation and execution speed. 

## 2. Code Overview

### 2.1 digitalWrite() - 10ms delay

```cpp
const int LED_PIN = 13;

void setup() {
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}
```

First we initialize the onboard LED which is directly connected to the digital pin 13. It is a good practice to set pins that are not going to be used later in the code as `const` - accidentally changing the pin value in the code.

In `setup()` using `pinMode` function we set the pin 13 as the output since we want to blink the LED, which will require current.

In the main loop which will execute continuously as far as power is supplied to the board, using `digitalWrite()` function, we set pin `HIGH`, meaning the board will supply current to that pin and LED will turn on.

Using `delay(500)` we keep pin HIGH for 500 milliseconds.

After 500 milliseconds pass, using `digitalWrite()` once again, we set the pin to the `LOW` mode, aborting the current flow to that pin. This will be followed with another 500ms-delay, after which the loop will move to the next iteration and the whole process will repeat.

Having LED ON for 0.5 seconds and OFF for 0.5 seconds, the whole loop will take 1 seconds to execute, meaning the frequency at which loop updates is
$$f = \frac{1}{1s} = 1\,Hz$$
meeting the lap requirement.

### 2.2 Register-Level - 10ms delay

```cpp
void setup() {
  DDRB |= (1 << DDB5);
}

void loop() {
  PORTB |= (1 << PORTB5);
  delay(500);
  PORTB &= ~(1 << PORTB5);
  delay(500);
}
```

Register-level programming on Arduino is performed using AVR instructions. Some of those used in this code are:
- `|=` - bitwise OR operation used to set the specified pin as HIGH.
- `&= ~` - bitwise NOT + AND - to set the specific bit as LOW.

Since Arduino uses ATmega328P microcontroller, to access specified pins to manipulate with them,  you must access the corresponding AVR registers and modify the required bits that control each pin’s direction or output state.

Since we want to access digital pin 13, we first look at the pinout diagram of the ATmega328P
![image](images/Pasted%20image%2020260204115127.png)

here we see that `PB5` pin on microcontroller corresponds to the digital pin 13. In `PB5`, letter P stands for the port, B indicates the port - PORTB, 5 - number of pin on that port.

ATmega328P has 3 ports: 
- PORTD -> D0-7
- PORTB -> D8-13
- PORTC -> A0-5

Each port is a register inside the microcontroller, containing 8 bits. The number of that bit corresponds to particular pin. For **PORTB**:
![image](images/Pasted%20image%2020260320141727.png)

This indicates, that if we want to access digital pin 13 and make it either high or low, we should work with bit 5 of PORTB register - **PORTB5**
- HIGH -> 1
- LOW -> 0

But before that the the data direction of that pin also should be specified and it is done using **DDRB** register:
![image](images/Pasted%20image%2020260320142030.png)

where:
- as an Input -> DDB5 = 0.
- as an Output -> DDB5 = 1.

This is better observed on example:
Suppose, **DDRB** was `0100 1101` before our intervention
**`DDRB |= (1 << DDB5)`**
1. left shift by 5: `0010 0000`
2. OR: `0100 1101` | `0010 0000` = `0110 1101`
we:
	- set bit 5 as high -> **output**
	- did not change other bits.

Suppose, **DDRB** was `1101 0001` before our intervention
**`PORTB |= (1 << PORTB5)`**
1. left shift by 5: `0010 0000`
2. OR: `1101 0001` | `0010 0000` = `1111 0001`
we:
	- set bit 5 as **HIGH** 
	- did not change other bits.

**`PORTB &= ~(1 << PORTB5)`**
from previous operation PORTB = `1111 0001`
 1. left shift by 5: `0010 0000`
 2. invert: ~`0010 0000`=`1101 1111`
 3. AND: `1111 0001` & `1101 1111` = `1101 0001`
we:
	- set bit 5 as **low** 
	- did not change other bits.

## 3. High-level vs. Register-level

`digitalWrite()` method uses Arduino framework which acts as a library with predefined function. Essentially, it is doing the same thing as register-level, but at a higher level of abstraction, hiding direct register manipulation and making the code easier to read and write.

Register-lever, manipulates register directly, significantly increasing the speed of execution and locating less space in memory. 

So, from practical perspective, register-level is much better, for convenience - digitalWrite() is more preferable.

### 3.1 Timing

To observe the actual speed of the program execution, best practice in this case is to remove delays completely. By doing so, we can observe how much time it takes for a program to perform each instruction. 

For instance, for **digitalWrite()**, if we measure the amount of time the signal stays high using oscilloscope, it will be around **3.2 $\micro s$**
![image](images/Pasted%20image%2020260320144307.png) 

For register-level, **133** $ns$ which is only **0.133** $\micro s$ - 24 times faster.

![image](images/Pasted%20image%2020260320144428.png)

This huge difference is due to the fact that `digitalWrite()` spends too much time for **extra function-call** and **pin-mapping overhead**, while register-level executes directly

### 3.2 Memory Occupation

Memory Occupation:
- **digitalWrite()** with delay: **924 bytes**
![image](images/Pasted%20image%2020260320145256.png)
- **digitalWrite()** no delay: **734 bytes**
![image](images/Pasted%20image%2020260320145339.png)
- **resiter-level** with delay: **640 bytes**
![image](images/Pasted%20image%2020260320145418.png)
- **resiter-level** no delay: **450 bytes**
![image](images/Pasted%20image%2020260320145454.png)

This shows that register control is not only faster, but also more-efficient in program memory.

### 4. Conclusion

Overall, both methods are doing the same thing, the difference only - how they do it. Register control method is more efficient, digitalWrite() is more intuitional and convenient for regular programmer. 
