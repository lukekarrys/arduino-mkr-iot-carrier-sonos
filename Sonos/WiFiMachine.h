#ifndef WIFI_MACHINE_H
#define WIFI_MACHINE_H

#include <Arduino.h>
#include <WiFiNINA.h>

#include "Enum.h"
#include "StateMachine.h"

class WiFiMachine : public StateMachine {
 public:
  WiFiMachine(const char *ssid, const char *password);

  ENUM_STATES(
      Error,
      Connect,
      Check,
      Backoff,
      Connected, );

  void reset();
  void ready();
  bool isConnected();

  int backoffCount = 0;
  String errorReason;

 protected:
  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  const char *ssid;
  const char *password;
};

#endif
