#include "WiFiMachine.h"

#include <Arduino.h>
#include <WiFiNINA.h>

#include "StateMachine.h"
#include "Utils.h"

#ifndef DEBUG_WIFI
#define DEBUG_WIFI false
#endif

WiFiMachine::WiFiMachine(const char *aSsid, const char *aPassword)
    : StateMachine("WIFI", stateStrings, DEBUG_WIFI),
      ssid(aSsid),
      password(aPassword) {
}

void WiFiMachine::reset() {
  StateMachine::reset();
  WiFi.setTimeout(0);
  WiFi.end();
  backoffCount = 0;
  errorReason = "";
}

void WiFiMachine::ready() {
  this->setState(Connect);
}

bool WiFiMachine::isConnected() {
  return this->getState() == Connected;
}

void WiFiMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Connect:
      // Deal with unrecoverable error states that require physical connections
      // or flashing firmware
      if (
          WiFi.status() == WL_NO_MODULE ||
          WiFi.status() == WL_NO_SHIELD ||
          WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION) {
        this->setState(Error);
      } else {
        WiFi.begin(ssid, password);
        this->setState(Check);
      }
      break;

    case Error:
      if (exitState == Backoff) {
        errorReason = "Backoff Exceeded";
      } else if (WiFi.status() == WL_NO_MODULE) {
        errorReason = "WL_NO_MODULE";
      } else if (WiFi.status() == WL_NO_SHIELD) {
        errorReason = "WL_NO_SHIELD";
      } else if (WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION) {
        errorReason = "Upgrade WiFi Firmware to " + String(WIFI_FIRMWARE_LATEST_VERSION);
      } else {
        errorReason = "Unknown WiFi Error";
      }
      backoffCount = 0;
      WiFi.end();
      break;

    case Check:
      if (WiFi.status() == WL_CONNECTED) {
        this->setState(Connected);
      } else {
        this->setState(Backoff);
      }
      break;

    case Backoff:
      backoffCount += 1;
      break;

    case Connected:
      backoffCount = 0;
      break;
  }
}

void WiFiMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case Backoff:
      if (backoffCount >= 10) {
        this->setState(Error);
      } else if (sinceChange > BACKOFF(backoffCount)) {
        this->setState(Check);
      }
      break;

    case Connected:
      if (sinceChange > WIFI_CHECK) {
        if (WiFi.status() != WL_CONNECTED) {
          this->setState(Check);
        } else {
          this->resetSinceChange();
        }
      }
      break;
  }
}
