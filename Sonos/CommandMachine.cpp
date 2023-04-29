#include "CommandMachine.h"

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <URLEncoder.h>
#include <WiFiNINA.h>

#include "Utils.h"

#ifndef DEBUG_COMMAND
#define DEBUG_COMMAND false
#endif

CommandMachine::CommandMachine(const char *aServer, uint16_t aPort)
    : StateMachine("COMMAND", stateStrings, DEBUG_COMMAND),
      client(HttpClient(wifi, aServer, aPort)) {
}

void CommandMachine::reset() {
  room = "";
  commandPath = "";
  client.stop();
  StateMachine::reset();
}

void CommandMachine::ready(String newRoom) {
  room = newRoom;
  this->setState(Ready);
}

void CommandMachine::sendRequest(String path) {
  commandPath = path;
  this->setState(Connect);
}

String CommandMachine::getUrl() {
  const String url = "/" + URLEncoderClass::encode(room) + "/" + commandPath;
  DEBUG_MACHINE("SEND", url);
  return url;
}

void CommandMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Ready:
      client.stop();
      break;

    case Success:
      client.stop();
      this->setState(Ready);
      break;

    case Error:
      client.stop();
      this->setState(Ready);
      break;

    case Connect:
      client.stop();
      client.get(this->getUrl());
      break;
  }
}

void CommandMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case Connect:
      if (sinceChange > TIMEOUT) {
        this->setState(Error);
      } else if (client.available()) {
        int statusCode = client.responseStatusCode();
        this->setState(statusCode == SUCCESS ? Success : Error);
      }
      break;
  }
}
