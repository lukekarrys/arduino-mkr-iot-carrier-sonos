#ifndef PLAYER_MACHINE_H
#define PLAYER_MACHINE_H

#include "ButtonMachine.h"
#include "CommandMachine.h"
#include "DisplayMachine.h"
#include "LedMachine.h"
#include "SonosMachine.h"
#include "StateMachine.h"

class PlayerMachine : public StateMachine {
 public:
  PlayerMachine(DisplayMachine *displayMachine, LedMachine *ledsMachine, const char *server, uint16_t port);
  PlayerMachine(DisplayMachine *displayMachine, LedMachine *ledsMachine, const char *server, uint16_t port, String initialRoom);

  enum States {
    Error,
    Connect,
    Connected,
    Ready,
    Action,
    Result,
    Locked,
  };
  String stateStrings[7] = {
      "Error",
      "Connect",
      "Connected",
      "Ready",
      "Action",
      "Result",
      "Locked",
  };
  static States states;

  int run();
  void reset();
  void ready();
  bool isConnected();
  bool isSleepy();

  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;
  void exit(int exitState, int enterState) override;

 private:
  int actionButton = -1;
  String actionMessage = "";
  char actionSuccess = 0;
  void resetAction();

  String errorReason = "";

  SonosMachine sonosMachine;

  CommandMachine commandMachine;
  void handleCommand();

  LedMachine *ledsMachine;
  LedMachine led0Machine;
  LedMachine led1Machine;
  LedMachine led2Machine;
  LedMachine led3Machine;
  LedMachine led4Machine;
  LedMachine *ledMachines[5];

  void resetLeds();
  void readyLeds();

  ButtonMachine button0Machine;
  ButtonMachine button1Machine;
  ButtonMachine button2Machine;
  ButtonMachine button3Machine;
  ButtonMachine button4Machine;
  void resetButtons();
  void readyButtons();
  void handleButtons();
  bool button0(int action);
  bool button1(int action);
  bool button2(int action);
  bool button3(int action);
  bool button4(int action);

  DisplayMachine *displayMachine;
};

#endif
