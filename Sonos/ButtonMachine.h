#ifndef BUTTON_MACHINE_H
#define BUTTON_MACHINE_H

#include "Carrier.h"
#include "StateMachine.h"

class ButtonMachine : public StateMachine {
 public:
  ButtonMachine(touchButtons button);

  enum States {
    Init,
    Up,
    Down,
    Tapped,
    DoubleTapped,
    Discard,
    PostHold,
    // action states
    Tap,
    Hold,
    TapHold,
    DoubleTap,
  };
  String stateStrings[11] = {
      "Init",
      "Up",
      "Down",
      "Tapped",
      "DoubleTapped",
      "Discard",
      "PostHold",
      // action states
      "Tap",
      "Hold",
      "TapHold",
      "DoubleTap",
  };
  static States states;

  void reset();
  void ready();

 protected:
  bool traceState(int state) override;

  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  bool getTouch();

  const touchButtons button;

  unsigned long prevDown = 0;
  unsigned long prevUp = 0;
  char prevTouch = 0;
};

#endif
