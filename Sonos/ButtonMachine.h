#ifndef BUTTON_MACHINE_H
#define BUTTON_MACHINE_H

#include "Carrier.h"
#include "Debug.h"
#include "Enum.h"
#include "StateMachine.h"

class ButtonMachine : public StateMachine {
 public:
  ButtonMachine(touchButtons button);

  ENUM_STATES(
      Init,
      Up,
      Down,
      Tapped,
      DoubleTapped,
      Discard,
      PostHold,
      Tap,
      Hold,
      TapHold,
      DoubleTap, );

  void reset();
  void ready();

 protected:
#ifdef DEBUG_OR_TRACE
  bool traceState(int state) override;
#endif

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
