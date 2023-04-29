
#include "LedMachine.h"

#include "Carrier.h"
#include "StateMachine.h"
#include "Utils.h"

#ifndef DEBUG_LED
#define DEBUG_LED false
#endif

LedMachine::LedMachine()
    : StateMachine("LED_ALL", stateStrings, DEBUG_LED),
      button(0),
      count(5) {}

LedMachine::LedMachine(int aButton)
    : StateMachine("LED_" + String(aButton), stateStrings, DEBUG_LED),
      button(aButton),
      count(1) {}

bool LedMachine::traceState(int state, String type) {
  return (state == BlinkOn || state == BlinkOff) || StateMachine::traceState(state, type);
}

void LedMachine::reset() {
  color = NULL;
  duration = 0;
  blinkDuration = 0;
  carrier.leds.fill(LedOff, button, count);
  carrier.leds.show();
  StateMachine::reset();
}

void LedMachine::ready() {
  this->setState(Off);
}

void LedMachine::on(uint32_t aColor) {
  color = aColor;
  this->setState(On);
}

void LedMachine::on(uint32_t aColor, unsigned long aDuration) {
  color = aColor;
  duration = aDuration;
  this->setState(On);
}

void LedMachine::blink(uint32_t aColor, unsigned long aDuration) {
  color = aColor;
  blinkDuration = aDuration;
  this->setState(BlinkOn);
}

void LedMachine::off() {
  this->setState(Off);
}

void LedMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case On:
    case BlinkOn:
      carrier.leds.fill(color, button, count);
      carrier.leds.show();
      break;

    case BlinkOff:
    case Off:
      carrier.leds.fill(LedOff, button, count);
      carrier.leds.show();
      break;
  }
}

void LedMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case On:
      if (duration > 0 && sinceChange >= duration) {
        this->setState(Off);
      }
      break;

    case BlinkOn:
      if (blinkDuration > 0 && sinceChange >= blinkDuration) {
        this->setState(BlinkOff);
      }
      break;

    case States::BlinkOff:
      if (blinkDuration > 0 && sinceChange >= blinkDuration) {
        this->setState(BlinkOn);
      }
      break;
  }
}
