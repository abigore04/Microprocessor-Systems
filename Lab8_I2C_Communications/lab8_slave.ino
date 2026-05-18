/*
  Board B - I2C Slave

  Function:
  - Receives LED command from Board A / Master.
  - Controls its local LED on D8.
  - Sends Button B state back when Master requests it.

  Hardware:
  - Button B: D2 to GND, using internal pull-up.
  - LED B: D8 -> resistor -> LED -> GND.
  - I2C:
      A4 = SDA
      A5 = SCL
      GND must be connected to Board A GND.
*/

#include <Wire.h>
#include <avr/io.h>

#define SLAVE_ADDRESS 0x12
#define CMD_SET_LED   0xA1

#define BUTTON_B_PIN  2
#define LED_B_PIN     8

void receiveEvent(int byteCount)
{
  /*
    This function is called automatically when Master writes data to this slave.
    not used delay() or Serial.print() here because this runs inside I2C interrupt context.
  */

  if (byteCount < 2) {
    // If the message is incomplete, clear received bytes and ignore it.
    while (Wire.available()) {
      Wire.read();
    }
    return;
  }

  uint8_t command = Wire.read();
  uint8_t value   = Wire.read();

  // Clear any extra bytes if they exist.
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

void requestEvent()
{
  /*
    This function is called when Master requests data from this slave.
    Slave sends one byte:
      0 = Button B not pressed
      1 = Button B pressed
  */

  uint8_t buttonState;

  // Because INPUT_PULLUP is used:
  // HIGH = not pressed, LOW = pressed.
  if (digitalRead(BUTTON_B_PIN) == LOW) {
    buttonState = 1;
  } else {
    buttonState = 0;
  }

  Wire.write(buttonState);
}

void setup()
{
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);
  pinMode(LED_B_PIN, OUTPUT);

  digitalWrite(LED_B_PIN, LOW);

  // Start I2C in slave mode with address 0x12.
  Wire.begin(SLAVE_ADDRESS);

  // Register callback functions.
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
}

void loop()
{
  // Nothing is needed here.
  // Slave communication is handled by receiveEvent() and requestEvent().
}