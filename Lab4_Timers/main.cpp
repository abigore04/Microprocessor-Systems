volatile uint8_t countdown = 0; 

void setup() {
  DDRB |= (1 << DDB5); 
  PORTB &= ~(1 << PORTB5); 
  DDRD &= ~(1 << DDD2); 
  PORTD |= (1 << PORTD2); 
  EICRA = (EICRA & 0xFC) | 0x02; 
  EIMSK |= (1 << INT0); 
  TCCR1A = 0; 
  TCCR1B = 0; 
  TCNT1  = 0;
  OCR1A = 249; 
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS11) | (1 << CS10); 
  TIMSK1 |= (1 << OCIE1A); 
  sei(); 
}

void loop() {
}

ISR(INT0_vect) {
  TCNT1=0;
  PORTB |= (1 << PORTB5); 
  countdown = 10; 
}

ISR(TIMER1_COMPA_vect) {
  if (countdown > 0) {
    countdown--;
    if (countdown == 0) {
      PORTB &= ~(1 << PORTB5); 
    }
  }
}
