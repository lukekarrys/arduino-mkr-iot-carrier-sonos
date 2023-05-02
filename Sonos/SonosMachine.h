#ifndef SONOS_MACHINE_H
#define SONOS_MACHINE_H

#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>

#include "Enum.h"
#include "StateMachine.h"

class SonosMachine : public StateMachine {
 public:
  SonosMachine(const char *server, uint16_t port);
  SonosMachine(const char *server, uint16_t port, String initialRoom);

  ENUM_STATES(
      Error,
      Backoff,
      Zones,
      Connect,
      Connected,
      PendingUpdate,
      Update, );

  void reset();
  void ready();

  bool isConnected();
  void sendRequest(String path);

  int roomIndex = -1;
  int roomSize = 0;
  void nextRoom();
  void prevRoom();
  String getRoom();
  String rooms[7] = {};

  int volume;
  bool mute;
  String artist;
  String title;
  String album;
  String repeat;
  bool shuffle;
  bool playing;

  String errorReason;

 protected:
  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  WiFiClient wifi;
  HttpClient client;
  void get(String url);
  void resetClient();

  int backoffCount = 0;
  void backoff(String reason);

  String response;

  bool readEvent();
  bool setSonos(String response);
  void setRooms(String response);

  String initialRoom;
  bool hasRooms = false;
};

#endif
