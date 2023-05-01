#ifndef PLAYER_MACHINE_H
#define PLAYER_MACHINE_H

#include "ButtonMachine.h"
#include "CommandMachine.h"
#include "DisplayMachine.h"
#include "Enum.h"
#include "LedMachine.h"
#include "SonosMachine.h"
#include "StateMachine.h"

class PlayerMachine : public StateMachine {
 public:
  PlayerMachine(DisplayMachine *displayMachine, LedMachine *ledsMachine, const char *server, uint16_t port);
  PlayerMachine(DisplayMachine *displayMachine, LedMachine *ledsMachine, const char *server, uint16_t port, String initialRoom);

  ENUM_STATES(
      Error,
      Connect,
      Connected,
      Ready,
      Action,
      Result,
      Locked, );

  void run();
  void reset();
  void ready();
  bool isConnected();
  bool isSleepy();

  String errorReason = "";

 protected:
  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;
  void exit(int exitState, int enterState) override;

 private:
  int actionButton = -1;
  String actionMessage = "";
  bool actionError = false;
  void resetAction();

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
