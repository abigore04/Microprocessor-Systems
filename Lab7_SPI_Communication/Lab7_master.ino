#include <avr/io.h>
#include <stdint.h>

// Sequence required by the lab task
// 85  = 01010101 = 0x55
// 170 = 10101010 = 0xAA
// 255 = 11111111 = 0xFF

uint8_t values[] = {85, 170, 255};
uint8_t indexValue = 0; // when 0 -> 85, 1 -> 170

// ----------------------------------------------------------
// SPI Master initialization
// ----------------------------------------------------------
void SPI_MasterInit(void)
{
  DDRB |= (1 << DDB2);  //SS as output
  DDRB |= (1 << DDB3);  //MOSI
  DDRB |= (1 << DDB5);  //SCK

  DDRB &= ~(1 << DDB4); //MISO - input

  // keeping ss high
  PORTB |= (1 << PB2);

  /*
    SPCR = SPI Control Register

    SPE  = 1 -> enable SPI
    MSTR = 1 -> configure as Master
    SPR0 = 1 -> SCK = F_CPU / 16

    CPOL = 0 and CPHA = 0 by default, so this is SPI Mode 0.
    DORD = 0 by default, so data is sent MSB first.
  */
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0);

  /*
    SPSR = SPI Status Register

    SPI2X = 0 -> no double speed
    With 16 MHz Arduino:
    SCK = 16 MHz / 16 = 1 MHz
  */
  SPSR &= ~(1 << SPI2X);
}

// ----------------------------------------------------------
// Send one byte through SPI
// ----------------------------------------------------------
void SPI_MasterTransmit(uint8_t data)
{
  // Slave select
  PORTB &= ~(1 << PB2);

  /*
    Writing to SPDR starts SPI transmission.
    Hardware shifts out 8 bits on MOSI.
  */
  SPDR = data;

  /*
    Wait until SPIF becomes 1.
    SPIF = SPI Interrupt Flag / transfer complete flag.
  */
  while (!(SPSR & (1 << SPIF)))
  {
    // wait
  }

  // Read SPDR to complete/clear transfer state dummy address from slave
  uint8_t dummy = SPDR;
  (void)dummy;

  // return ss high
  PORTB |= (1 << PB2);
}

// ----------------------------------------------------------
// Arduino setup
// ----------------------------------------------------------
void setup()
{
  Serial.begin(9600);
  SPI_MasterInit();

  Serial.println("SPI Master started");
}

// ----------------------------------------------------------
// Arduino loop
// ----------------------------------------------------------
void loop()
{
  uint8_t dataToSend = values[indexValue];

  SPI_MasterTransmit(dataToSend);

  Serial.print("Master sent: ");
  Serial.println(dataToSend);

  indexValue++;

  if (indexValue >= 3)
  {
    indexValue = 0;
  }

  delay(1000);
}