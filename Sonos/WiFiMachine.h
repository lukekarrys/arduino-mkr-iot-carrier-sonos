#ifndef WIFI_MACHINE_H
#define WIFI_MACHINE_H

#include <Arduino.h>
#include <WiFiNINA.h>

#include "StateMachine.h"

class WiFiMachine : public StateMachine {
 public:
  WiFiMachine(const char *ssid, const char *password);

  enum States {
    Error,
    Connect,
    Check,
    Backoff,
    Connected,
  };
  String stateStrings[5] = {
      "Error",
      "Connect",
      "Check",
      "Backoff",
      "Connected",
  };
  static States states;

  void reset();
  void ready();
  bool isConnected();

  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

  int backoffCount = 0;
  String errorReason;

 private:
  const char *ssid;
  const char *password;
};

#endif
