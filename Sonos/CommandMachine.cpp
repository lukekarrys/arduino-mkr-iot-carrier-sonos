#include "CommandMachine.h"

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <URLEncoder.h>
#include <WiFiNINA.h>

#include "Consts.h"

#ifndef DEBUG_COMMAND
#define DEBUG_COMMAND false
#endif

CommandMachine::CommandMachine(const char *aServer, uint16_t aPort)
    : StateMachine("COMMAND", stateStrings, DEBUG_COMMAND),
      client(HttpClient(wifi, aServer, aPort)) {
}

void CommandMachine::reset() {
  StateMachine::reset();
  room = "";
  commandPath = "";
  this->resetClient();
}

void CommandMachine::resetClient() {
  client.stop();
}

void CommandMachine::ready() {
  this->setState(Ready);
}

void CommandMachine::ready(String newRoom) {
  room = newRoom;
  this->setState(Ready);
}

void CommandMachine::sendRequest(String path) {
  commandPath = path;
  this->setState(Connect);
}

void CommandMachine::get() {
  String url = "/" + URLEncoderClass::encode(room) + "/" + commandPath;
  unsigned long getPerf = NOW;
  client.get(url);
  DEBUG_MACHINE(url, String(NOW - getPerf) + "ms");
}

void CommandMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Ready:
      this->resetClient();
      break;

    case Success:
      this->resetClient();
      this->setState(Ready);
      break;

    case Error:
      this->resetClient();
      this->setState(Ready);
      break;

    case Connect:
      this->resetClient();
      this->get();
      break;

    case Locked:
      this->resetClient();
      break;
  }

  lockCheck = NOW;
}

void CommandMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case Connect:
      if (sinceChange > TIMEOUT) {
        DEBUG_MACHINE("TIMEOUT");
        this->setState(Error);
      } else if (client.available()) {
        int statusCode = client.responseStatusCode();
        DEBUG_MACHINE("STATUS", String(statusCode));
        this->setState(statusCode == SUCCESS ? Success : Error);
      }
      break;

    case Ready:
#ifdef AUTO_LOCK
      if (sinceChange > LOCK) {
        this->setState(Locked);
      } else if ((NOW - lockCheck) > MINUTE) {
        DEBUG_MACHINE("LOCK_CHECK", String((LOCK - sinceChange) / MINUTE) + "m");
        lockCheck = NOW;
      }
#endif
      break;
  }
}
