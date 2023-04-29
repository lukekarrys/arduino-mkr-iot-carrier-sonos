
#include "AppMachine.h"

#include "ButtonMachine.h"
#include "Carrier.h"
#include "DisplayMachine.h"
#include "LedMachine.h"
#include "PlayerMachine.h"
#include "StateMachine.h"
#include "Utils.h"
#include "WiFiMachine.h"

#ifndef DEBUG_APP
#define DEBUG_APP false
#endif

String dots(int count) {
  String times = "";
  for (int i = 0; i < count; i++) {
    times += ".";
  }
  return times;
}

AppMachine::AppMachine(const char *ssid, const char *password, const char *server, uint16_t port)
    : StateMachine("APP", stateStrings, DEBUG_APP),
      wifiMachine(ssid, password),
      buttonMachine(TOUCH0),
      playerMachine(&displayMachine, &ledsMachine, server, port) {
}

AppMachine::AppMachine(const char *ssid, const char *password, const char *server, uint16_t port, String initialRoom)
    : StateMachine("APP", stateStrings, DEBUG_APP),
      wifiMachine(ssid, password),
      buttonMachine(TOUCH0),
      playerMachine(&displayMachine, &ledsMachine, server, port, initialRoom) {
}

void AppMachine::ready() {
  buttonMachine.ready();
  ledsMachine.ready();
  displayMachine.ready();
  this->setState(WiFi);
}

bool AppMachine::isConnected() {
  const int state = this->getState();
  return state != WiFi && state != Error && state != Sleep;
}

void AppMachine::reset() {
  StateMachine::reset();
  wifiMachine.reset();
  ledsMachine.reset();
  buttonMachine.reset();
  displayMachine.reset();
  playerMachine.reset();
}

int AppMachine::run() {
  StateMachine::run();
  wifiMachine.run();
  ledsMachine.run();
  buttonMachine.run();
  displayMachine.run();
  playerMachine.run();
}

void AppMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case WiFi:
      wifiMachine.ready();
      ledsMachine.blink(LedBlue, FAST_BLINK);
      playerMachine.reset();
      displayMachine.wifi("WiFi:", "Connecting");
      break;

    case Player:
      playerMachine.ready();
      break;

    case Sleep:
      wifiMachine.reset();
      ledsMachine.reset();
      playerMachine.reset();
      displayMachine.sleep("Sleep", "Sleeping");
      break;

    case Error:
      wifiMachine.reset();
      ledsMachine.on(LedRed);
      playerMachine.reset();
      displayMachine.error("Error", "Bad things");
      break;
  }
}

void AppMachine::poll(int state, unsigned long sinceChange) {
  if (buttonMachine.getState() == ButtonMachine::TapHold) {
    this->setState(state == Sleep || state == Error ? WiFi : Sleep);
    return;
  }

  if (this->isConnected() != wifiMachine.isConnected()) {
    this->setState(this->isConnected() ? WiFi : Player);
    return;
  }

  if (wifiMachine.getState() == WiFiMachine::Error || playerMachine.getState() == PlayerMachine::Error) {
    this->setState(Error);
    return;
  }

  switch (state) {
    case WiFi:
      if (wifiMachine.getState() == WiFiMachine::Backoff) {
        displayMachine.setWifi("Connecting" + dots(wifiMachine.backoffCount));
      }
      break;

    case Player:
      if (sinceChange > MINUTE) {
        if (playerMachine.isSleepy()) {
          this->setState(Sleep);
        } else {
          this->resetSinceChange();
        }
      }
      break;
  }
}
