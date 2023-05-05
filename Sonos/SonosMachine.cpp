#include "SonosMachine.h"

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <URLEncoder.h>
#include <WiFiNINA.h>

#include "Consts.h"

#ifndef DEBUG_SONOS
#define DEBUG_SONOS false
#endif

#define DISPLAY_DEBOUNCE 100

SonosMachine::SonosMachine(const char *aServer, uint16_t aPort)
    : StateMachine("SONOS", stateStrings, DEBUG_SONOS),
      client(HttpClient(wifi, aServer, aPort)) {
}

SonosMachine::SonosMachine(const char *aServer, uint16_t aPort, String aInitialRoom)
    : StateMachine("SONOS", stateStrings, DEBUG_SONOS),
      client(HttpClient(wifi, aServer, aPort)),
      initialRoom(aInitialRoom) {
}

void SonosMachine::reset() {
  StateMachine::reset();

  backoffCount = 0;
  errorReason = "";
  response = "";

  this->setSonos("");

  roomIndex = -1;
  roomSize = 0;
  hasRooms = false;
  for (int i = 0; i < MAX_ROOMS; i++) {
    rooms[i] = "";
  }

  this->resetClient();
}

void SonosMachine::resetClient() {
  client.stop();
}

bool SonosMachine::isConnected() {
  const int state = this->getState();
  return state == Connected || state == Update || state == PendingUpdate;
}

void SonosMachine::ready() {
  if (hasRooms) {
    this->setState(Connect);
  } else {
    this->setState(Zones);
  }
}

void SonosMachine::backoff(String reason) {
  DEBUG_MACHINE("BACKOFF", reason);
  errorReason = reason;
  this->setState(Backoff);
}

void SonosMachine::nextRoom() {
  roomIndex++;
  if (roomIndex >= roomSize) {
    roomIndex = 0;
  }
}

void SonosMachine::prevRoom() {
  roomIndex--;
  if (roomIndex < 0) {
    roomIndex = roomSize - 1;
  }
}

String SonosMachine::getRoom() {
  if (roomIndex == -1) {
    return "";
  }
  return rooms[roomIndex];
}

void SonosMachine::get(String url) {
  unsigned long getPerf = NOW;
  client.get(url);
  DEBUG_MACHINE(url, String(NOW - getPerf) + "ms");
}

void SonosMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Error:
      backoffCount = 0;
      this->resetClient();
      break;

    case Backoff:
      backoffCount += 1;
      this->resetClient();
      break;

    case Zones:
      this->resetClient();
      this->get("/zones/names");
      break;

    case Connect:
      this->resetClient();
      this->get("/" + URLEncoderClass::encode(this->getRoom()) + "/events");
      break;

    case Connected:
      backoffCount = 0;
      client.skipResponseHeaders();
      break;

    case Update:
      this->setState(Connected);
      break;
  }
}

void SonosMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case Zones:
      if (sinceChange > TIMEOUT) {
        this->backoff("Timeout connecting to /zones");
      } else if (client.connected() && client.available()) {
        if (client.endOfHeadersReached()) {
          this->setRooms(client.responseBody());
        } else {
          int statusCode = client.responseStatusCode();
          DEBUG_MACHINE("/zones", String(statusCode));
          if (statusCode == SUCCESS) {
            client.skipResponseHeaders();
          } else {
            this->backoff("Status code " + String(statusCode));
          }
        }
      }
      break;

    case Connect:
      if (sinceChange > TIMEOUT) {
        this->backoff("Timeout connecting to /events");
      } else if (client.connected() && client.available()) {
        int statusCode = client.responseStatusCode();
        if (statusCode == SUCCESS) {
          this->setState(Connected);
        } else {
          this->backoff("Status code " + statusCode);
        }
      }
      break;

    case Backoff:
      if (backoffCount >= 4) {
        this->setState(Error);
      } else if (sinceChange > BACKOFF(backoffCount)) {
        this->ready();
      }
      break;

    case Connected:
      if (this->readEvent()) {
        this->setState(PendingUpdate);
      }
      break;

    case PendingUpdate:
      if (this->readEvent()) {
        this->resetSinceChange();
      } else if (sinceChange > DISPLAY_DEBOUNCE) {
        this->setState(Update);
      }
      break;
  }
}

bool SonosMachine::readEvent() {
  bool didUpdate = false;
  if (!client.connected()) {
    this->ready();
  } else if (client.available()) {
    const char c = client.read();
    if (c == '\n') {
      response.remove(0, 6);
      if (response.length()) {
        didUpdate = this->setSonos(response);
      }
      response = "";
    } else {
      response += c;
    }
  }
  return didUpdate;
}

void SonosMachine::setRooms(String rawResponse) {
  if (!rawResponse.length()) {
    this->backoff("No rooms found");
    return;
  }

  TRACE_MACHINE("SET_ROOMS", rawResponse);

  DynamicJsonDocument doc(768);
  DeserializationError error = deserializeJson(doc, rawResponse);

  if (error) {
    this->backoff("JSON error: " + String(error.c_str()));
    return;
  }

  JsonArray responseRooms = doc.as<JsonArray>();

  int currentIndex = 0;
  for (JsonObject responseRoom : responseRooms) {
    String roomName = responseRoom["roomName"].as<String>();
    rooms[currentIndex] = roomName;
    if (initialRoom == roomName) {
      roomIndex = currentIndex;
    }
    currentIndex++;
    if (currentIndex >= 10) {
      break;
    }
  }

  roomSize = responseRooms.size();
  hasRooms = true;
  if (roomIndex == -1) {
    roomIndex = 0;
  }

  DEBUG_MACHINE("CURRENT_ROOM", this->getRoom());

  this->ready();
}

bool SonosMachine::setSonos(String rawResponse) {
  TRACE_MACHINE("SET_SONOS", rawResponse);

  if (!rawResponse.length()) {
    volume = 0;
    mute = false;
    artist = "";
    title = "";
    album = "";
    repeat = "";
    shuffle = false;
    playing = false;
    return (true);
  }

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, rawResponse);

  if (error) {
    DEBUG_MACHINE("SET_SONOS_ERROR", error.f_str());
    return (false);
  }

  bool didUpdate = false;
  const String type = doc["type"];

  if (type == "transport-state") {
    volume = doc["data"]["state"]["volume"];
    mute = doc["data"]["state"]["mute"];
    artist = doc["data"]["state"]["currentTrack"]["artist"].as<String>();
    title = doc["data"]["state"]["currentTrack"]["title"].as<String>();
    album = doc["data"]["state"]["currentTrack"]["album"].as<String>();
    repeat = doc["data"]["state"]["playMode"]["repeat"].as<String>();
    shuffle = doc["data"]["state"]["playMode"]["shuffle"];
    playing = doc["data"]["state"]["playbackState"].as<String>() == "PLAYING";
    didUpdate = true;
  } else if (type == "volume-change") {
    volume = doc["data"]["newVolume"];
    didUpdate = true;
  } else if (type == "mute-change") {
    mute = doc["data"]["newMute"];
    didUpdate = true;
  }

  DEBUG_MACHINE("SET_SONOS", type + (didUpdate ? " (UPDATE)" : " (NO_UPDATE)"));

  return (didUpdate);
}
