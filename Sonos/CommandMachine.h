#ifndef COMMAND_MACHINE_H
#define COMMAND_MACHINE_H

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>

#include "StateMachine.h"

class CommandMachine : public StateMachine {
 public:
  CommandMachine(const char *aServer, uint16_t aPort);

  enum States {
    Ready,
    Connect,
    Success,
    Error,
  };
  String stateStrings[5] = {
      "Ready",
      "Connect",
      "Success",
      "Error",
  };
  static States states;

  void reset();

  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

  void ready(String newRoom);
  void sendRequest(String path);

 private:
  WiFiClient wifi;
  HttpClient client;

  String room;
  String commandPath;
  String getUrl();
};

#endif