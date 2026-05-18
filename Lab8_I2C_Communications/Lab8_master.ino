/*
  Board A - I2C Master

  Function:
  - Reads Button A on D2.
  - Sends Button A state to Board B.
  - Board B uses this state to control LED B.
  - Requests Button B state from Board B.
  - Controls local LED A on D8 based on Button B state.

  Hardware:
  - Button A: D2 to GND, using internal pull-up.
  - LED A: D8 -> resistor -> LED -> GND.
  - I2C:
      A4 = SDA
      A5 = SCL
      GND must be connected to Board B GND.

  Speed control:
  - I2C clock speed is changed using ATmega328P TWI registers:
      TWBR = TWI Bit Rate Register
      TWSR TWPS1:0 = prescaler bits
*/


#include <Wire.h>
#include <avr/io.h>

#define SLAVE_ADDRESS 0x12 // 0001 0010
#define CMD_SET_LED   0xA1  // command to set LED state // 1010 0001

#define BUTTON_A_PIN  2
#define LED_A_PIN     8

// -------------------- I2C SPEED SETTINGS --------------------
// Use this first for stable testing: 100 kHz.
// For 16 MHz Arduino UNO:
// f_SCL = f_CPU / (16 + 2 * TWBR * prescaler)
// f_SCL = 16 MHz / (16 + 2 * 72 * 1) = 100 kHz
void setI2CSpeed_100kHz()
{
  // Enable TWI peripheral clock. Power Reduction Register is 
  // used to save energy. When PRTWI is 1 - TWI is off, 0 - on (page 38)
  PRR &= ~(1 << PRTWI);

  // Set TWI prescaler to 1: TWPS1 = 0, TWPS0 = 0. (page 200)
  // set to 1 to make formula simpler
  TWSR &= ~((1 << TWPS0) | (1 << TWPS1));

  // Set bit rate register for approximately 100 kHz.
  // TWBR's used to set the SCL clock line speed 
  TWBR = 72;

  // Configure SDA/SCL pins as inputs and enable pull-ups.
  // A4 = PC4 = SDA, A5 = PC5 = SCL.
  DDRC &= ~((1 << DDC4) | (1 << DDC5));
  PORTC |= (1 << PORTC4) | (1 << PORTC5);

  // Enable TWI hardware.
  TWCR |= (1 << TWEN);
}

// Use this later for the second oscilloscope test: 400 kHz.
// f_SCL = 16 MHz / (16 + 2 * 12 * 1) = 400 kHz
void setI2CSpeed_400kHz()
{
  PRR &= ~(1 << PRTWI);

  TWSR &= ~((1 << TWPS0) | (1 << TWPS1));

  TWBR = 12;

  DDRC &= ~((1 << DDC4) | (1 << DDC5));
  PORTC |= (1 << PORTC4) | (1 << PORTC5);

  TWCR |= (1 << TWEN);
}

uint8_t readButtonA()
{
  // With INPUT_PULLUP:
  // HIGH = not pressed
  // LOW  = pressed
  if (digitalRead(BUTTON_A_PIN) == LOW) {
    return 1;
  } else {
    return 0;
  }
}

void sendLedCommandToSlave(uint8_t ledState)
{
  /*
    Master sends two bytes to Slave:
      byte 1 = command 0xA1
      byte 2 = LED state: 0 or 1
  */

  // START → address → data → STOP

  Wire.beginTransmission(SLAVE_ADDRESS); // preparing trasmission
  Wire.write(CMD_SET_LED);               // adding first byte
  Wire.write(ledState);                  // adding second byte
  // so, 0xA1 and 0x01
  uint8_t result = Wire.endTransmission();

  // if result = 0, then success
  Serial.print("Sent Button A state = ");
  Serial.print(ledState);
  Serial.print(" | I2C result = ");
  Serial.println(result);

  /*
    result = 0 means success.
    If result is not 0, check wiring, slave address, GND, SDA/SCL.
  */
}

uint8_t requestButtonBFromSlave()
{
  /*
    Master requests one byte from Slave.
    Slave responds:
      0 = Button B not pressed
      1 = Button B pressed
  */

  uint8_t receivedState = 0;

  Wire.requestFrom(SLAVE_ADDRESS, 1); // "Slave 0x12, send me 1 byte."

  if (Wire.available()) {
    receivedState = Wire.read();
  }

  return receivedState;
}

void setup()
{
  Serial.begin(115200);

  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(LED_A_PIN, OUTPUT);

  digitalWrite(LED_A_PIN, LOW);

  // Start I2C as Master.
  Wire.begin(); // since no address, means begin as a master

  // Start with 100 kHz. After it works, change to setI2CSpeed_400kHz().
  setI2CSpeed_100kHz();
  //setI2CSpeed_400kHz();

  Serial.println("Board A / Master started.");
}

void loop()
{
  static uint8_t lastButtonAState = 0;
  static uint32_t lastPollTime = 0;

  uint8_t buttonAState = readButtonA();

  // Send only when Button A changes.
  if (buttonAState != lastButtonAState) {
    lastButtonAState = buttonAState;
    sendLedCommandToSlave(buttonAState);
    delay(30); // simple debounce delay
  }

  // Ask Button B state every 50 ms.
  if (millis() - lastPollTime >= 50) {
    lastPollTime = millis();

    uint8_t buttonBState = requestButtonBFromSlave();

    if (buttonBState == 1) {
      digitalWrite(LED_A_PIN, HIGH);
    } else {
      digitalWrite(LED_A_PIN, LOW);
    }
  }
}