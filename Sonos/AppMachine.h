#ifndef APP_MACHINE_H
#define APP_MACHINE_H

#include "ButtonMachine.h"
#include "DisplayMachine.h"
#include "LedMachine.h"
#include "PlayerMachine.h"
#include "StateMachine.h"
#include "WiFiMachine.h"

class AppMachine : public StateMachine {
 public:
  AppMachine(const char *ssid, const char *password, const char *server, uint16_t port);
  AppMachine(const char *ssid, const char *password, const char *server, uint16_t port, String initialRoom);

  enum States {
    WiFi,
    Player,
    Sleep,
    Error,
  };
  String stateStrings[5] = {
      "WiFi",
      "Player",
      "Sleep",
      "Error",
  };
  static States states;

  bool isConnected();
  void reset();
  void ready();
  int run();

  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  LedMachine ledsMachine;
  WiFiMachine wifiMachine;
  ButtonMachine buttonMachine;
  PlayerMachine playerMachine;
  DisplayMachine displayMachine;

  String errorReason = "";
};

#endif
