**Course:** Microprocessor Systems (ENCE-4731 - 20919) 
**Instructor:** Dr. Alexz Farrall  
**Lab Number & Title:**  Lab 5 - Jumps
**Student Name:** Farid Ibadov  
**Student ID:** 17954  
**Program / Section:** BSCE26  
**Date:** March, 2026

---

## Lab Requirements

**Hardware:** Arduino Uno (ATmega328P) | LED: Built-in LED on D13 (PB5) | Button: D8 (PB0) to GND (use internal pull-up)

**Objective:** Build one firmware that behaves like a simple “one-button LED gadget” and demonstrates three different jump styles:

- RJMP: tight local polling loop at boot
- JMP: hard jump into one of three modes (Mode A/B/C)
- IJMP: runtime dispatch to one of several “actions” inside the selected mode

**Final behaviour:** On power-up, the device waits for the first button press, then confirms with a short LED flash. During the next 2 seconds, the number of presses selects a mode:

- 1 press = Mode A (slow blink)
- 2 presses = Mode B (double blink)
- 3 presses = Mode C (fast strobe)


While running in a mode, each short press cycles an “action” (0..3). The action is executed using a jump table and IJMP.

**Phase requirements:**

- **Phase 1 (RJMP):** Implement the boot wait loop in assembly using a label, a bit-test (SBIC or SBIS), and `rjmp` back to the label while the button is not pressed.
- **Phase 2 (JMP):** After counting presses, enter the chosen mode by using an explicit `jmp modeX_entry`. The mode entry functions run forever and do not return.
- **Phase 3 (IJMP):** Inside the mode loop, implement action dispatch by loading a handler address into Z (r31:r30) and executing ijmp. Each handler must jump back to the mode loop (since IJMP does not push a return address).

## 2. Code Overview

```cpp
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
```

The program configures LED as output button as input-pullup, after that calls three phases, each in order. `setup()` and `loop()` still run inside Arduino runtime, even when AVR-style code used in them.

### 2.1 First Phase - RJMP

**RJMP** (Relative Jump) - jumps to a nearby label by offset (k) from current instruction position: PC <- PC + k + 1. Is best for short and local loops, takes less cycles that **JMP**

`bootWait_RJMP()` being assembly code loop, checks `PINB` bit 0 with `SBIC` and if button not pressed, executes `rjmp wait_btn` to jump back to the same label - looping in same place.

`sbic` skips the next instruction if bit in input/output register is zero. Button is active low, so as long as it is idle, the PB0 stays high and the `rjmp` throws us back to the same label.

So, after the first press, the LED turns on for 200ms as a start confirmation.

### 2.2 Second Phase - JMP

After boot, program waits 2 seconds and counts the number of presses. 25ms debounce value is used as always for buttons.

Selected mode entered using `runSelectedMode()`. It uses `asm goto("jmp %10")` to jump to one of 3 labels: 
- `modeA_entry` for slow blink
- `modeB_entry` for double blink
- `modeC_entry` for fast strobe blinking

**JMP** jumps to the absolute address k (PC <- k ) and is used for far jumps. It loads the PC with an absolute program-memory address and. Compared with RJMP, it is larger and slower.

So, if zero or 1 press occurs, program stays in mode A, if 2 presses - mode B, 3 presses - mode C.

### 3.3 Third Phase - IJMP

While mode running, `pollActionButton()` watches button presses - each press increments `actionSelector` (1, 2, 3, 1, 2, 3, ...) and this value is passed into `dispatchAction()` 

- `r24` holds requested action idx
- `r30:31` is a Z register pair and is loaded with the base addr of ActionTable
- index is added to Z
- IJMP jumps to the selected table entry

Since IJMP takes as destination the value from Z reg pair, it is used for computed jumps and jump tables:

1. `Action0` -> normal output
2. `Action1` -> inverted output
3. `Action2` -> short pulses
4. `Action3` -> back to normal

Each handler loads the result into `r24` and finishes with `ret`. This works sinc `dispatchActionAsm()` was called from C first, and the normal func call already pushed a return address onto the stack.

![image](images/Pasted%20image%2020260320231229.png)
![image](images/Pasted%20image%2020260320231248.png)
![image](images/Pasted%20image%2020260320231343.png)
