#include "ButtonMachine.h"

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <URLEncoder.h>
#include <WiFiNINA.h>

#include "Carrier.h"
#include "Utils.h"

#ifndef DEBUG_BUTTON
#define DEBUG_BUTTON false
#endif

ButtonMachine::ButtonMachine(touchButtons aButton)
    : StateMachine("BUTTON_" + String(aButton), stateStrings, DEBUG_BUTTON),
      button(aButton) {}

bool ButtonMachine::traceState(int state, String type) {
  return (state != Tap && state != Hold && state != TapHold && state != DoubleTap) || StateMachine::traceState(state, type);
}

void ButtonMachine::reset() {
  prevDown = 0;
  prevUp = 0;
  prevTouch = 0;
  StateMachine::reset();
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

void ButtonMachine::poll(int buttonState, unsigned long sinceChange) {
  const bool touch = this->getTouch();

  const unsigned long lastDown = NOW - prevDown;
  const unsigned long lastUp = NOW - prevUp;

  // The buttons are capacitive and even at a non-sensitive config level
  // there is noise when a finger hovers close to the touchpad. This results
  // in state quickly osscilating between up/down. I tested by quickly
  // tapping and could never get it under 30ms, so anything under 30ms is
  // deemed noise which means we just reset the button state to up.
  const bool discard = touch ? lastUp < TAP : lastDown < TAP;
  const int state = discard ? Discard : buttonState;

  switch (state) {
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
    DEBUG_MACHINE("TOUCH_DOWN", state, "");
  } else if (!touch && prevTouch) {
    prevTouch = 0;
    prevUp = NOW;
    DEBUG_MACHINE("TOUCH_UP", state, "");
  }
}
