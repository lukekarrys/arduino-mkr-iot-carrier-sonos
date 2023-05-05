#ifndef DISPLAY_MACHINE_H
#define DISPLAY_MACHINE_H

#include <Arduino.h>

#include "Carrier.h"
#include "Enum.h"
#include "StateMachine.h"

class DisplayMachine : public StateMachine {
 public:
  DisplayMachine();

  ENUM_STATES(
      WiFi,
      Sonos,
      Error,
      Player,
      Sleep, );

  void setStale();
  void printLine(String str);

  void setBattery(int v);
  void setTemperature(int v);
  void setHumidity(int v);

  void wifi(String title, String message);
  void setWifi(String message);
  void setWifi(String title, String message);

  void sonos(String title, String message);
  void setSonos(String message);
  void setSonos(String title, String message);

  void error(String title, String message);
  void setError(String message);
  void setError(String title, String message);

  void sleep(String title, String message);
  void setSleep(String message);
  void setSleep(String title, String message);

  void player();
  void setPlayer(String title, String artist, String album, int volume, bool mute, String repeat, bool shuffle, bool playing);
  void setPlayerAction(String action);
  void setPlayerAction(String action, uint16_t color);
  void drawPlayerControls();
  void drawPlayerControls(uint16_t color);
  void drawPlayPauseControl();

 protected:
  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;
  void exit(int exitState, int enterState) override;

 private:
  uint16_t backgroundColor;
  uint16_t foregroundColor;
  bool textWrap;

  bool stale = false;

  void drawBasicScreen(String title[2], String message[2], bool drawBattery);
  void resetScreen(uint16_t bg, uint16_t fg, bool wrap);
  void redraw();

  void drawString(String strs[2], int x, int y);
  void drawString(String strs[2], int x, int y, uint16_t fgColor);
  void drawPositionString(String strs[2], int x, int y, int x2, int y2);
  void drawPositionString(String strs[2], int x, int y, int x2, int y2, uint16_t colors[2]);

  void setValue(String strs[2], String str);
  void setValue(uint16_t colors[2], uint16_t color);
  void unsetValue(String strs[2]);
  void unsetValue(uint16_t colors[2]);

  String battery[2] = {"", ""};
  String temperature[2] = {"", ""};
  String humidity[2] = {"", ""};

  String wifiTitle[2] = {"", ""};
  String wifiMessage[2] = {"", ""};

  String sonosTitle[2] = {"", ""};
  String sonosMessage[2] = {"", ""};

  String errorTitle[2] = {"", ""};
  String errorMessage[2] = {"", ""};

  String sleepTitle[2] = {"", ""};
  String sleepMessage[2] = {"", ""};

  String title[2] = {"", ""};
  String artist[2] = {"", ""};
  String album[2] = {"", ""};
  String repeat[2] = {"", ""};
  String shuffle[2] = {"", ""};
  String volume[2] = {"", ""};
  String playButton[2] = {"", ""};
  uint16_t playButtonColor[2] = {
      0,
      0,
  };

  String action[2] = {"", ""};
  uint16_t actionColor[2] = {
      0,
      0,
  };
};

#endif