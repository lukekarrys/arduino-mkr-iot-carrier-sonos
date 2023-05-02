
#include "AppMachine.h"

#include "ButtonMachine.h"
#include "Carrier.h"
#include "Consts.h"
#include "Debug.h"
#include "DisplayMachine.h"
#include "LedMachine.h"
#include "PlayerMachine.h"
#include "SensorMachine.h"
#include "StateMachine.h"
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
  sensorMachine.ready();
  buttonMachine.ready();
  ledsMachine.ready();
  displayMachine.ready();
  this->drawSensors();
  this->setState(WiFi);
}

void AppMachine::drawSensors() {
  displayMachine.setBattery(sensorMachine.battery);
  displayMachine.setHumidity(sensorMachine.humidity);
  displayMachine.setTemperature(sensorMachine.temperature);
}

bool AppMachine::isConnected() {
  const int connectedState = this->getState();
  return connectedState != WiFi && connectedState != Error && connectedState != Sleep;
}

void AppMachine::reset() {
  StateMachine::reset();
  sensorMachine.reset();
  wifiMachine.reset();
  ledsMachine.reset();
  buttonMachine.reset();
  displayMachine.reset();
  playerMachine.reset();
}

void AppMachine::run() {
  StateMachine::run();
  sensorMachine.run();
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
      sensorMachine.ready();
      ledsMachine.blink(LedBlue, FAST_BLINK);
      playerMachine.reset();
      displayMachine.wifi("WiFi:", "Connecting");
      break;

    case Player:
      sensorMachine.reset();
      playerMachine.ready();
      break;

    case Sleep:
      sensorMachine.ready();
      wifiMachine.reset();
      ledsMachine.reset();
      playerMachine.reset();
      displayMachine.sleep("Sleep", "Sleeping. Tap and hold button 0 to restart.");
      break;

    case Error:
      sensorMachine.ready();
      wifiMachine.reset();
      ledsMachine.on(LedRed);
      playerMachine.reset();
      displayMachine.error("Error", errorReason + ". Tap and hold button 0 to restart.");
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

  if (wifiMachine.getState() == WiFiMachine::Error) {
    errorReason = wifiMachine.errorReason;
    this->setState(Error);
    return;
  }

  if (playerMachine.getState() == PlayerMachine::Error) {
    errorReason = playerMachine.errorReason;
    this->setState(Error);
    return;
  }

  if (sensorMachine.getState() == SensorMachine::Update) {
    this->drawSensors();
  }

  switch (state) {
    case WiFi:
      if (wifiMachine.getState() == WiFiMachine::Backoff) {
        displayMachine.setWifi("Connecting" + dots(wifiMachine.backoffCount));
      }
      break;

    case Player:
      if (sinceChange > MINUTE) {
        this->checkSleep();
      }
      break;
  }
}

void AppMachine::checkSleep() {
  const unsigned long sleep = playerMachine.sleepSinceChange();
  if (sleep > SLEEP) {
    this->setState(Sleep);
  } else {
    DEBUG_MACHINE("SLEEP_CHECK", String((SLEEP - sleep) / MINUTE) + "m");
    this->resetSinceChange();
  }
}
