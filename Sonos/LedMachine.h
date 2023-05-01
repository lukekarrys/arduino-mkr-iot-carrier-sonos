#ifndef LED_MACHINE_H
#define LED_MACHINE_H

#include <Arduino.h>

#include "Debug.h"
#include "Enum.h"
#include "StateMachine.h"

class LedMachine : public StateMachine {
 public:
  LedMachine();
  LedMachine(int aButton);

  ENUM_STATES(
      Off,
      On,
      BlinkOn,
      BlinkOff, );

  void reset();
  void ready();

  void on(uint32_t aColor);
  void on(uint32_t aColor, unsigned long aDuration);

  void off();
  void blink(uint32_t aColor, unsigned long aDuration);

 protected:
#ifdef DEBUG_OR_TRACE
  bool traceState(int state) override;
#endif

  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  const int button;
  const int count;

  uint32_t color;
  unsigned long duration = 0;
  unsigned long blinkDuration = 0;
};

#endif