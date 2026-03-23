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
