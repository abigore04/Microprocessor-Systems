#include <Arduino.h>
#include <avr/io.h>
#include <stdint.h>

const uint8_t LED_PIN = 13;
const uint8_t BTN_PIN = 8;

volatile uint8_t actionState = 0;
volatile uint8_t actionSelector = 0;

enum Mode : uint8_t {
  MODE_A = 0,
  MODE_B = 1,
  MODE_C = 2
};

extern "C" uint8_t dispatchActionAsm(uint8_t idx);

__asm__(
R"ASM(
.global dispatchActionAsm
dispatchActionAsm:
    ldi r30, lo8(pm(ActionTable))
    ldi r31, hi8(pm(ActionTable))
    add r30, r24
    adc r31, __zero_reg__
    ijmp

ActionTable:
    rjmp Action0
    rjmp Action1
    rjmp Action2
    rjmp Action3

Action0:
    ldi r24, 0
    ret

Action1:
    ldi r24, 1
    ret

Action2:
    ldi r24, 2
    ret

Action3:
    ldi r24, 0
    ret
)ASM");

void setLed(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

void bootWait_RJMP() {
  asm volatile(
    "wait_btn:\n\t"
    "sbic %[pinreg], %[bit]\n\t"
    "rjmp wait_btn\n\t"
    :
    : [pinreg] "I" (_SFR_IO_ADDR(PINB)),
      [bit]    "I" (PB0)
  );
}

void startDetectedFlash() {
  setLed(true);
  delay(200);
  setLed(false);
}

void waitReleaseAfterBoot() {
  while (digitalRead(BTN_PIN) == LOW) {
    delay(5);
  }
  delay(25);
}

Mode chooseModeIn2Seconds() {
  uint8_t pressCount = 0;

  bool lastReading = HIGH;
  bool stableState = HIGH;
  unsigned long lastChange = 0;

  unsigned long t0 = millis();

  while (millis() - t0 < 2000) {
    bool reading = digitalRead(BTN_PIN);

    if (reading != lastReading) {
      lastReading = reading;
      lastChange = millis();
    }

    if (millis() - lastChange > 25) {
      if (reading != stableState) {
        stableState = reading;

        if (stableState == LOW) {
          pressCount++;
        }
      }
    }

    delay(2);
  }

  if (pressCount <= 1) return MODE_A;
  if (pressCount == 2) return MODE_B;
  return MODE_C;
}

void pollActionButton() {
  static bool lastReading = HIGH;
  static bool stableState = HIGH;
  static unsigned long lastChange = 0;

  bool reading = digitalRead(BTN_PIN);

  if (reading != lastReading) {
    lastReading = reading;
    lastChange = millis();
  }

  if (millis() - lastChange > 25) {
    if (reading != stableState) {
      stableState = reading;

      if (stableState == LOW) {
        actionSelector++;
        if (actionSelector > 3) actionSelector = 1;

        actionState = dispatchActionAsm(actionSelector);
      }
    }
  }
}

void runSegment(bool logicalOn, unsigned long durationMs) {
  unsigned long start = millis();

  while (millis() - start < durationMs) {
    pollActionButton();

    bool out = logicalOn;

    if (actionState == 1) {
      out = !out;
    } else if (actionState == 2) {
      if (logicalOn) {
        unsigned long phase = (millis() - start) % 120;
        out = (phase < 18);
      } else {
        out = false;
      }
    }

    setLed(out);
    delay(2);
  }
}

void slowBlinkPattern() {
  runSegment(true, 400);
  runSegment(false, 400);
}

void doubleBlinkPattern() {
  runSegment(true, 120);
  runSegment(false, 120);
  runSegment(true, 120);
  runSegment(false, 500);
}

void fastStrobePattern() {
  runSegment(true, 60);
  runSegment(false, 60);
}

void runSelectedMode(Mode mode) {
  actionState = 0;
  actionSelector = 0;

  if (mode == MODE_A) {
    asm goto("jmp %l0" : : : : modeA_entry);
  } else if (mode == MODE_B) {
    asm goto("jmp %l0" : : : : modeB_entry);
  } else {
    asm goto("jmp %l0" : : : : modeC_entry);
  }

modeA_entry:
  while (true) {
    slowBlinkPattern();
  }

modeB_entry:
  while (true) {
    doubleBlinkPattern();
  }

modeC_entry:
  while (true) {
    fastStrobePattern();
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  setLed(false);

  bootWait_RJMP();

  delay(20);
  waitReleaseAfterBoot();

  startDetectedFlash();

  Mode selectedMode = chooseModeIn2Seconds();

  runSelectedMode(selectedMode);
}

void loop() {
}
