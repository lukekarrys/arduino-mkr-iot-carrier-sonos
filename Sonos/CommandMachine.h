#ifndef COMMAND_MACHINE_H
#define COMMAND_MACHINE_H

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>

#include "Enum.h"
#include "StateMachine.h"

class CommandMachine : public StateMachine {
 public:
  CommandMachine(const char *aServer, uint16_t aPort);

  ENUM_STATES(
      Ready,
      Connect,
      Success,
      Error, );

  void reset();
  void ready();
  void ready(String newRoom);
  void sendRequest(String path);

 protected:
  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  WiFiClient wifi;
  HttpClient client;

  String room;
  String commandPath;
  void get();
  void resetClient();
};

#endif
