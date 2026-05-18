# Lab 8 - I2C Communication 

**Course**: Microprocessor Systems (ENCE-4731 - 20919)  
**Instructor**: Dr. Alexz Farrall  
**Lab Number & Title**: Lab 8 - I2C Communication  
**Student Name**: Farid Ibadov  
**Student ID**: 17954  
**Program / Section**: BSCE26  
**Date**: May, 2026  

---

## 1. Objective

The main goal of this lab was to implement I2C communication between two Arduino Uno boards, one of which was configured as the I2C Master, and another as I2C Slave.

Each board had one corresponding push button and one LED. When Button A on the Master board is pressed, Master sends command to Slave using Slave address, and the Slave turns ON its LED. And vice versa, Master constantly requests the Slave's button state from the Slave and whenever Button B on Slave board is pressed and turns ON Master's LED.

I2C communication was implemented using standard ATmega328P TWI (Two-Wire Interface) pins:

| I2C Signal | ATmega328P Pin | Arduino Pin |
| --- | --- | --- |
| `SDA` | `PC4` | `A4` |
| `SCL` | `PC5` | `A5` |
| `GND` | `GND` | `GND` |

The clock speed was adjusted using TWI registers `TWBR` and `TWSR` and two speeds were tested: `100 kHz` and `400 kHz`. Oscilloscope captures were used to observe the SDA and SCL behavior through waveforms on the I2C bus under different button states.

---

## 2. Code Overview

One program is uploaded to the Master Arduino, while the other to the Slave Arduino. Codes use the Arduino `Wire` library for I2C handling, but the TWI clock speed is still configured manually through dedicated AVR registers such as `PRR`, `TWSR`, `TWBR`, `DDRC`, `PORTC`, and `TWCR`.

| Component / Signal | Master Arduino Pin | Slave Arduino Pin | ATmega328P Pin | Direction / Role                       | Purpose                                                 |
| ------------------ | ------------------ | ----------------- | -------------- | -------------------------------------- | ------------------------------------------------------- |
| **`SDA`**          | `A4`               | `A4`              | `PC4 / SDA`    | Bidirectional                          | I2C data line                                           |
| **`SCL`**          | `A5`               | `A5`              | `PC5 / SCL`    | Master output clock, Slave input clock | I2C clock line                                          |
| **`GND`**          | `GND`              | `GND`             | `GND`          | Common reference                       | Shared ground between boards                            |
| **Button A**       | `D2`               | -                 | `PD2`          | Master input                           | Master-side button, controls Slave LED                  |
| **LED A**          | `D8`               | -                 | `PB0`          | Master output                          | Turns ON when Slave button state is received as pressed |
| **Button B**       | -                  | `D2`              | `PD2`          | Slave input                            | Slave-side button, state sent back to Master            |
| **LED B**          | -                  | `D8`              | `PB0`          | Slave output                           | Turns ON when Master sends Button A pressed state       |

### 2.1 Master Code

Master board reads Button A, sends this state to Slave, then requests Button B state from the Slave, and controls LED A according to the received state.

```cpp
#define SLAVE_ADDRESS 0x12
#define CMD_SET_LED   0xA1

#define BUTTON_A_PIN  2
#define LED_A_PIN     8
```

`SLAVE_ADDRESS = 0x12` is the address of Slave board set manually. `CMD_SET_LED = 0xA1` is the command byte used by Master to tell Slave that the next byte is the LED state.

Master initializes I2C by:
```cpp
Wire.begin();
setI2CSpeed_100kHz();
// setI2CSpeed_400kHz();
```

`Wire.begin()` starts the board as Master, since no address is passed in parenthesis. After that, the function `setI2CSpeed_100kHz()` or `setI2CSpeed_400kHz()` changes the TWI clock register values. One of functions is commented to play around with the speed. 

For 100 kHz:
```cpp
void setI2CSpeed_100kHz()
{
  PRR &= ~(1 << PRTWI);

  TWSR &= ~((1 << TWPS0) | (1 << TWPS1));

  TWBR = 72;

  DDRC &= ~((1 << DDC4) | (1 << DDC5));
  PORTC |= (1 << PORTC4) | (1 << PORTC5);

  TWCR |= (1 << TWEN);
}
```

`PRR &= ~(1 << PRTWI)` is used to enable the TWI peripheral. If `PRTWI` is `1`, the TWI module is powered down, therefore it must be cleared, if we are willing to use I2C (TWI).
![image](images/Pasted%20image%2020260518135034.png)

`TWSR` bits `TWPS1:0` are cleared, therefore the prescaler is set to `1`.
![image](images/Pasted%20image%2020260518135131.png)
![image](images/Pasted%20image%2020260518135145.png)


`TWBR = 72` sets the I2C bit rate close to 100 kHz. This register represents just a regular 8-bit storage, with no specific bits. Its value is parsed into the formula used for calculation of TWI clock frequency 
![image](images/Pasted%20image%2020260518135218.png)

The TWI clock formula is:

$$
f_{SCL} = \frac{f_{CPU}}{16 + 2 \cdot TWBR \cdot \text{Prescaler}}
$$

For 100 kHz:

$$
f_{SCL} = \frac{16{,}000{,}000}{16 + 2 \cdot 72 \cdot 1}
$$

$$
f_{SCL} = \frac{16{,}000{,}000}{160} = 100{,}000 \text{ Hz}
$$

$$
f_{SCL} = 100 \text{ kHz}
$$

For 400 kHz:

```cpp
void setI2CSpeed_400kHz()
{
  PRR &= ~(1 << PRTWI);

  TWSR &= ~((1 << TWPS0) | (1 << TWPS1));

  TWBR = 12;

  DDRC &= ~((1 << DDC4) | (1 << DDC5));
  PORTC |= (1 << PORTC4) | (1 << PORTC5);

  TWCR |= (1 << TWEN);
}
```

For 400 kHz:

$$
f_{SCL} = \frac{16{,}000{,}000}{16 + 2 \cdot 12 \cdot 1}
$$

$$
f_{SCL} = \frac{16{,}000{,}000}{40} = 400{,}000 \text{ Hz}
$$

$$
f_{SCL} = 400 \text{ kHz}
$$

The SDA and SCL pins are configured as inputs with pull-ups:

```cpp
DDRC &= ~((1 << DDC4) | (1 << DDC5));
PORTC |= (1 << PORTC4) | (1 << PORTC5);
```

This is the right choice for I2C communication since SDA and SCL function like open-drain/open-collector lines. Device pull this line LOW actively and HIGH logic is only achieved using pull-up resistors, when the line is released by devices.

Then, TWI is enabled using `TWCR |= (1 << TWEN);`
![image](images/Pasted%20image%2020260518140207.png)
![image](images/Pasted%20image%2020260518140216.png)

Master reads its own button using:

```cpp
uint8_t readButtonA()
{
  if (digitalRead(BUTTON_A_PIN) == LOW) {
    return 1;
  } else {
    return 0;
  }
}
```

Since `INPUT_PULLUP` is used, logic is inversed:

- `HIGH` means not pressed
- `LOW` means pressed

Function returns `1` when button is pressed.

To send Button A state to Slave, Master sends two bytes in order:

```cpp
void sendLedCommandToSlave(uint8_t ledState)
{
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(CMD_SET_LED);
  Wire.write(ledState);
  uint8_t result = Wire.endTransmission();

  Serial.print("Sent Button A state = ");
  Serial.print(ledState);
  Serial.print(" | I2C result = ");
  Serial.println(result);
}
```

Those are the commands for:

| Byte | Meaning |
| --- | --- |
| `0xA1` | Command: set LED |
| `0x00` or `0x01` | LED state: OFF or ON |

So if Button A is pressed, Master sends:

```text
0xA1 0x01 // set LED, ON
```

If Button A is released, the Master sends:

```text
0xA1 0x00 // set LED, OFF
```

Master requests Button B state using:

```cpp
uint8_t requestButtonBFromSlave()
{
  uint8_t receivedState = 0;

  Wire.requestFrom(SLAVE_ADDRESS, 1);

  if (Wire.available()) {
    receivedState = Wire.read();
  }

  return receivedState;
}
```

And Slave returns one byte:

| Received Byte | Meaning              |
| ------------- | -------------------- |
| `0`           | Button B not pressed |
| `1`           | Button B pressed     |

Master then controls its LED A according to this received byte signal:

```cpp
if (buttonBState == 1) {
  digitalWrite(LED_A_PIN, HIGH);
} else {
  digitalWrite(LED_A_PIN, LOW);
}
```


### 2.2 Slave Code

Slave board receives LED commands from Master, then sends its own button state when Master requests for it.


```cpp
#define SLAVE_ADDRESS 0x12
#define CMD_SET_LED   0xA1

#define BUTTON_B_PIN  2
#define LED_B_PIN     8
```

Slave starts I2C with address `0x12`, TWI understand this is the Slave, since the address is indicated in parenthesis:

```cpp
Wire.begin(SLAVE_ADDRESS);
```

Callback functions then are registered:

```cpp
Wire.onReceive(receiveEvent);
Wire.onRequest(requestEvent);
```

`receiveEvent()` runs automatically when the Master writes data to the Slave:

```cpp
void receiveEvent(int byteCount)
{
  if (byteCount < 2) {
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  uint8_t command = Wire.read();
  uint8_t value   = Wire.read();

  while (Wire.available()) {
    Wire.read();
  }

  if (command == CMD_SET_LED) {
    if (value == 1) {
      digitalWrite(LED_B_PIN, HIGH);
    } else {
      digitalWrite(LED_B_PIN, LOW);
    }
  }
}
```

Slave expects minimum two bytes: first byte - command, the second byte - LED state. If the command is `0xA1`, the Slave changes its LED B according to received value.

`requestEvent()` runs when the Master requests one byte from the Slave:

```cpp
void requestEvent()
{
  uint8_t buttonState;

  if (digitalRead(BUTTON_B_PIN) == LOW) {
    buttonState = 1;
  } else {
    buttonState = 0;
  }

  Wire.write(buttonState);
}
```

This sends Button B state back to the Master.

Note that the Slave's `loop()` is empty:

```cpp
void loop()
{
}
```

This is done so, since I2C on Slave side is handled entirely through `receiveEvent()` and `requestEvent()` callbacks. Slave can respond only when addressed, otherwise it just waits.

---

## 3. System Flow Diagram

![image](images/mermaid-diagram%202.png)

---

## 4. Message Protocol Design

There are 2 logical directions in this lab - each can be conceived better by looking at the system flow diagram above.

First, Master sends Button A state to Slave. This controls LED B.

```text
Master Button A → I2C write → Slave LED B
```

Second, Master requests Button B state from Slave. This controls LED A.

```text
Slave Button B → I2C read response → Master LED A
```

From the first glance it may appear that both boards have the right to control each other; however, it is not what is happening actually - Slave can never start the communication by itself. Therefore, Button B state is sent only when Master requests for it.

Write message format is:

```text
START → Slave Address + Write → 0xA1 → LED State → STOP
```

where:

| Field | Value |
| --- | --- |
| Slave address | `0x12` |
| Command byte | `0xA1` |
| LED state | `0x00` or `0x01` |

Read message format is:

```text
START → Slave Address + Read → Button State Byte → STOP
```

where:

| Returned Byte | Meaning |
| --- | --- |
| `0x00` | Button B not pressed |
| `0x01` | Button B pressed |

---

## 5. Pull-Up Resistors on I2C Lines

![image](images/Pasted%20image%2020260518150051.png)

A pair of external 4.7 $k\Omega$ pull-up resistors were connected to both of the I2C lines of the circuitry. This was done so since, as discussed earlier, I2C does not drive the lines HIGH actively but uses open-drain/open-collector type of communication. But why Arduino's internal pull-up resistors were not used for that purpose? 

The internal pull-up resistor on the Arduino Uno (ATmega328P microcontroller) ranges from 20 to 50 $k\Omega s$, with a typical nominal value of around 30 $k\Omega s$. Since resistance value is much bigger, those resistors are weak and may work slower, especially when multiple devices communicate on a single I2C bus. Though for 2 devices, as in this lab, the change may not seem very significant, the fact that performance may slightly degrade still exists. 

External 4.7 $k\Omega$ pull-up resistors result in a stronger and more predictable pull-up, so the line returns in a HIGH state more faster and oscilloscope waveform becomes much cleaner. The weaker the resistors (greater the resistance), the lower the signal transition frequency becomes (signal changes its state slower, appearing flatter on an oscilloscope), which can lead to a situation where the signal will not keep up with the clock and will not have time to change its logic, resulting in data corruption. 

![image](images/Pasted%20image%2020260518150512.png)

The rising edge of the I2C signal depends on the RC time constant:

$$
\tau = R \cdot C
$$

where:

- $\tau$ - rise-time behavior of the line,
- $R$ - pull-up resistance,
- $C$ - total bus capacitance from wires, breadboard, board pins, and oscilloscope probes.

However, reducing resistance is not always a good practice, since, though line can rise faster, more current start to flow when device pulls the line LOW, which can result in increased power consumption and may even overload the system, damaging the components.

Therefore, 4.7 $k\Omega$ resistors serve as a good compromise between the performance and the robustness of the system.

---

## 6. Results and Evidence

![image](images/Pasted%20image%2020260518150844.png)

Even when no button was pressed and both LEDs stayed off, oscilloscope still showed I2C activity. This was because the Master, every 50ms, was requesting Button B state from the slave. 

![image](images/Pasted%20image%2020260518151940.png)

![image](images/Pasted%20image%2020260518154016.png)

After Slave sends `0x00`, since no button is pressed, the following `NACK` is generated by Master. `NACK` tells Slave that Master doesn't want more data, after which the Master releases the bus initiating a STOP condition.

When Button A on Master board was pressed and kept pressed, LED B on the Slave board turned and stayed ON. Master sends LED command only when Button A state changes. Since no change of state occurred, the LED kept emitting light. This is why oscilloscope does not show continuous “button pressed" level.

![image](images/Pasted%20image%2020260518154419.png)

When Slave's Button B was pressed, Master's LED A turned ON. However, this doesn't mean that Slave has initiated the communication - Master has been periodically requestion one byte from Slave during which Slave returned `1` value.

![image](images/Pasted%20image%2020260518154713.png)

When both buttons were pressed, both LEDs stayed ON. However, this does not mean that both boards transmitted at exactly the same time - I2C communication works on an Half-Duplex principle and is controlled by the Master. Transactions happen one after another on the same SDA/SCL bus. Both LEDs can remain ON at the same time since each board keeps its last output state after corresponding I2C transaction.

![image](images/Pasted%20image%2020260518154944.png)


Regarding the I2C speed, the same I2C transaction was also tested after switching Master code from `setI2CSpeed_100kHz()` to `setI2CSpeed_400kHz()`. At higher speed SCL pulses became more dense while SDA rising edges still depended on pull-up resistors. This allows us to formulate a rule according to which the I2C protocol data transfer rate and the resistor nominal parameters must be coordinated slowly to avoid data corruption. It can also be said that no communication failures were observed during testing of the final assembly and that resistors with a nominal resistance of 4.7$k\Omega$ were ideal for both 100 $kHz$ and 400 $kHz$ speeds. However, the picture may change if additional devices are added to the circuit or the connection between devices is extended.

---

## 7. References

[1] A. Farrall, *Lecture 8: Inter-integrated Circuit (I2C) Protocol*, Microprocessor Systems, ADA University, 2026.
[2] Atmel Corporation, *ATmega328P: 8-bit AVR Microcontroller with 32K Bytes In-System Programmable Flash Datasheet*, 7810D-AVR-01/15, 2015.
[3] Microchip Technology Inc., *AVR Instruction Set Manual*, DS40002198A, 2020.
[4] Arduino Forum, “Use of the internal pull-up resistor,” *Arduino Forum*, Jul. 26, 2012. [Online]. Available: https://forum.arduino.cc/t/use-of-the-internal-pull-up-resistor/113541. [Accessed: May 18, 2026].
