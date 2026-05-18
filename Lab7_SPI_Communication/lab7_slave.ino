#include <avr/io.h>
#include <stdint.h>

// ----------------------------------------------------------
// SPI Slave initialization
// ----------------------------------------------------------
void SPI_SlaveInit(void)
{
  DDRB |= (1 << DDB4);  //MISO

  DDRB &= ~(1 << DDB2); //SS
  DDRB &= ~(1 << DDB3); //MOSI
  DDRB &= ~(1 << DDB5); //SCK

  /*
    Enable SPI only.
    MSTR = 0 by default, so this Arduino becomes Slave.
  */
  SPCR = (1 << SPE);

  // dummy value
  SPDR = 0x00;
}

// ----------------------------------------------------------
// Receive one byte through SPI
// ----------------------------------------------------------
uint8_t SPI_SlaveReceive(void)
{
  /*
    Wait until one full byte is received.
    SPIF becomes 1 after 8 clock pulses from Master.
  */
  while (!(SPSR & (1 << SPIF)))
  {
    // wait
  }

  /*
    Reading SPDR gives the received byte.
  */
  uint8_t received = SPDR;

  /*
    Reload dummy value for next full-duplex transfer.
  */
  SPDR = 0x00;

  return received;
}

// ----------------------------------------------------------
// Arduino setup
// ----------------------------------------------------------
void setup()
{
  Serial.begin(9600);
  SPI_SlaveInit();

  Serial.println("SPI Slave started");
  Serial.println("Waiting for data...");
}

// ----------------------------------------------------------
// Arduino loop
// ----------------------------------------------------------
void loop()
{
  uint8_t receivedValue = SPI_SlaveReceive();

  Serial.print("Slave received DEC: ");
  Serial.print(receivedValue);

  Serial.print(" | HEX: 0x");
  if (receivedValue < 16)
  {
    Serial.print("0");
  }
  Serial.print(receivedValue, HEX);

  Serial.print(" | BIN: ");
  Serial.println(receivedValue, BIN);
}