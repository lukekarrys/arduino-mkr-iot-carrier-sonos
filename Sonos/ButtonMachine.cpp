#include "ButtonMachine.h"

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <URLEncoder.h>
#include <WiFiNINA.h>

#include "Carrier.h"
#include "Consts.h"
#include "Debug.h"

#ifndef DEBUG_BUTTON
#define DEBUG_BUTTON false
#endif

#define HOLD SECOND
#define TAP 40
#define DOUBLE_TAP 100

ButtonMachine::ButtonMachine(touchButtons aButton)
    : StateMachine("BUTTON_" + String(aButton), stateStrings, DEBUG_BUTTON),
      button(aButton) {}

#ifdef DEBUG_OR_TRACE
bool ButtonMachine::traceState(int state) {
  return state != Tap && state != Hold && state != TapHold && state != DoubleTap;
}
#endif

void ButtonMachine::reset() {
  StateMachine::reset();
  prevDown = 0;
  prevUp = 0;
  prevTouch = 0;
}

void ButtonMachine::ready() {
  this->setState(Init);
}

bool ButtonMachine::getTouch() {
  carrier.Buttons.update();
  return carrier.Buttons.getTouch(button);
}

void ButtonMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Init:
      this->setState(this->getTouch() ? Down : Up);
      break;

    case Tap:
    case DoubleTap:
      this->setState(Up);
      break;

    case Hold:
    case TapHold:
      this->setState(PostHold);
      break;
  }
}

void ButtonMachine::poll(int state, unsigned long sinceChange) {
  const bool touch = this->getTouch();

  const unsigned long lastDown = NOW - prevDown;
  const unsigned long lastUp = NOW - prevUp;

  // The buttons are capacitive and even at a non-sensitive config level
  // there is noise when a finger hovers close to the touchpad. This results
  // in state quickly osscilating between up/down. I tested by quickly
  // tapping and could never get it under 30ms, so anything under 30ms is
  // deemed noise which means we just reset the button state to up.
  const bool discard = touch ? lastUp < TAP : lastDown < TAP;
  const int switchState = discard ? Discard : state;

  switch (switchState) {
    case Up:
      if (touch) {
        this->setState(Down);
      }
      break;

    case Down:
      if (touch) {
        if (lastDown > HOLD) {
          this->setState(Hold);
        }
      } else {
        this->setState(Tapped);
      }
      break;

    case PostHold:
      if (!touch) {
        this->setState(Up);
      }
      break;

    case Tapped:
      if (!touch) {
        if (lastUp > DOUBLE_TAP) {
          this->setState(Tap);
        }
      } else {
        this->setState(DoubleTapped);
      }
      break;

    case DoubleTapped:
      if (touch) {
        if (lastDown > HOLD) {
          this->setState(TapHold);
        }
      } else {
        if (lastUp > DOUBLE_TAP) {
          this->setState(DoubleTap);
        }
      }
      break;
  }

  if (touch && !prevTouch) {
    prevTouch = 1;
    prevDown = NOW;
    TRACE_MACHINE("TOUCH_DOWN", switchState, "");
  } else if (!touch && prevTouch) {
    prevTouch = 0;
    prevUp = NOW;
    TRACE_MACHINE("TOUCH_UP", switchState, "");
  }
}
