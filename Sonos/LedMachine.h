#ifndef LED_MACHINE_H
#define LED_MACHINE_H

#include <Arduino.h>

#include "StateMachine.h"

class LedMachine : public StateMachine {
 public:
  LedMachine();
  LedMachine(int aButton);

  enum States {
    Off,
    On,
    BlinkOn,
    BlinkOff,
  };
  String stateStrings[4] = {
      "Off",
      "On",
      "BlinkOn",
      "BlinkOff",
  };
  static States states;

  void reset();
  void ready();

  void on(uint32_t aColor);
  void on(uint32_t aColor, unsigned long aDuration);

  void off();
  void blink(uint32_t aColor, unsigned long aDuration);

 protected:
  bool traceState(int state, String type) override;

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