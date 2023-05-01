
#include "DisplayMachine.h"

#include "Carrier.h"
#include "StateMachine.h"

#ifndef DEBUG_DISPLAY
#define DEBUG_DISPLAY false
#endif

#define TOP_Y 42
#define TEXT_3_STEP 24
#define TEXT_3_CHAR 9
#define TEXT_2_STEP 16
#define MARGIN 5

#define SCREEN_WIDTH 240
#define CENTER 120

#define MESSAGE 50
#define CONTROLS 215
#define PLAY_STATE 110

DisplayMachine::DisplayMachine()
    : StateMachine("DISPLAY", stateStrings, DEBUG_DISPLAY) {}

void DisplayMachine::exit(int state, int enterState) {
  switch (state) {
    case WiFi:
      this->unsetValue(wifiTitle);
      this->unsetValue(wifiMessage);
      break;

    case Sonos:
      this->unsetValue(sonosTitle);
      this->unsetValue(sonosMessage);
      break;

    case Error:
      this->unsetValue(errorTitle);
      this->unsetValue(errorMessage);
      break;

    case Sleep:
      this->unsetValue(sleepTitle);
      this->unsetValue(sleepMessage);
      break;

    case Player:
      this->unsetValue(title);
      this->unsetValue(artist);
      this->unsetValue(album);
      this->unsetValue(repeat);
      this->unsetValue(shuffle);
      this->unsetValue(volume);
      this->unsetValue(playButton);
      this->unsetValue(action);
      this->unsetValue(actionColor);
      break;
  }

  this->unsetValue(battery);
  this->unsetValue(humidity);
  this->unsetValue(temperature);
}

void DisplayMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case WiFi:
      this->resetScreen(ST77XX_BLACK, ST77XX_WHITE, true);
      break;

    case Sonos:
      this->resetScreen(ST77XX_BLACK, ST77XX_WHITE, true);
      break;

    case Error:
      this->resetScreen(ST77XX_RED, ST77XX_WHITE, true);
      break;

    case Sleep:
      this->resetScreen(ST77XX_BLACK, ST77XX_WHITE, true);
      break;

    case Player:
      this->resetScreen(ST77XX_BLACK, ST77XX_WHITE, false);
      this->drawPlayerControls();
      this->setValue(actionColor, ST77XX_WHITE);

      carrier.display.setTextSize(3);
      carrier.display.setCursor(MARGIN, TOP_Y + (TEXT_3_STEP * 3) + TEXT_2_STEP);

      carrier.display.setTextSize(2);
      carrier.display.print("Repeat:");
      carrier.display.setCursor(MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
      carrier.display.print("Shuffle:");
      carrier.display.setCursor(MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
      carrier.display.print("Volume:");
      carrier.display.setCursor(MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
      break;
  }

  this->setStale();
}

void DisplayMachine::poll(int state, unsigned long sinceChange) {
  if (stale) {
    this->redraw();
  }
}

void DisplayMachine::setStale() {
  stale = true;
}

void DisplayMachine::setValue(String strs[2], String str) {
  strs[0] = str;
  if (strs[0] != strs[1]) {
    this->setStale();
  }
}

void DisplayMachine::setValue(uint16_t colors[2], uint16_t color) {
  colors[0] = color;
  if (colors[0] == colors[1]) {
    this->setStale();
  }
}

void DisplayMachine::unsetValue(String strs[2]) {
  strs[1] = "";
}

void DisplayMachine::unsetValue(uint16_t colors[2]) {
  colors[1] = 0;
}

void DisplayMachine::setBattery(int v) {
  this->setValue(battery, v < 0 ? "" : "Bat:" + String(v) + "%");
}

void DisplayMachine::setHumidity(int v) {
  this->setValue(humidity, "Hum:" + String(v) + "%");
}

void DisplayMachine::setTemperature(int v) {
  this->setValue(temperature, "Temp:" + String(v) + "F");
}

void DisplayMachine::wifi(String t, String m) {
  this->setWifi(t, m);
  this->setState(WiFi);
}
void DisplayMachine::setWifi(String m) {
  this->setValue(wifiMessage, m);
}
void DisplayMachine::setWifi(String t, String m) {
  this->setValue(wifiTitle, t);
  this->setValue(wifiMessage, m);
}

void DisplayMachine::sonos(String t, String m) {
  this->setSonos(t, m);
  this->setState(Sonos);
}
void DisplayMachine::setSonos(String m) {
  this->setValue(sonosMessage, m);
}
void DisplayMachine::setSonos(String t, String m) {
  this->setValue(sonosTitle, t);
  this->setValue(sonosMessage, m);
}

void DisplayMachine::error(String t, String m) {
  this->setError(t, m);
  this->setState(Error);
}
void DisplayMachine::setError(String m) {
  this->setValue(errorMessage, m);
}
void DisplayMachine::setError(String t, String m) {
  this->setValue(errorTitle, t);
  this->setValue(errorMessage, m);
}

void DisplayMachine::sleep(String t, String m) {
  this->setSleep(t, m);
  this->setState(Sleep);
}
void DisplayMachine::setSleep(String m) {
  this->setValue(sleepMessage, m);
}
void DisplayMachine::setSleep(String t, String m) {
  this->setValue(sleepTitle, t);
  this->setValue(sleepMessage, m);
}

void DisplayMachine::player() {
  this->setState(Player);
}
void DisplayMachine::setPlayer(String _title, String _artist, String _album, int _volume, bool _mute, String _repeat, bool _shuffle, bool _playing) {
  this->setValue(title, _title);
  this->setValue(artist, _artist);
  this->setValue(album, _album);
  this->setValue(repeat, _repeat);
  this->setValue(shuffle, _shuffle ? "On" : "Off");
  this->setValue(volume, _mute ? "Muted" : String(_volume));
  this->setValue(playButton, _playing ? "||" : ">");
}
void DisplayMachine::setPlayerAction(String a) {
  this->setPlayerAction(a, ST77XX_WHITE);
}
void DisplayMachine::setPlayerAction(String a, uint16_t c) {
  this->setValue(action, a);
  this->setValue(actionColor, c);
}
void DisplayMachine::drawPlayerControls() {
  this->drawPlayerControls(foregroundColor);
}
void DisplayMachine::drawPlayerControls(uint16_t c) {
  carrier.display.setTextColor(c);
  carrier.display.setTextSize(3);
  carrier.display.setCursor(MARGIN, MARGIN);
  carrier.display.print("-");
  carrier.display.setCursor(CONTROLS, MARGIN);
  carrier.display.print("+");
  carrier.display.setCursor(MARGIN, CONTROLS);
  carrier.display.print("<");
  carrier.display.setCursor(CONTROLS, CONTROLS);
  carrier.display.print(">");
  this->setStale();
  carrier.display.setTextColor(foregroundColor);
}

void DisplayMachine::resetScreen(uint16_t bg, uint16_t fg, bool textWrap) {
  carrier.display.fillScreen(bg);
  carrier.display.setTextColor(fg);
  carrier.display.setTextWrap(textWrap);
  backgroundColor = bg;
  foregroundColor = fg;
}

void DisplayMachine::drawBasicScreen(String title[2], String message[2], bool drawSensors) {
  carrier.display.setTextSize(3);
  this->drawString(title, MARGIN, MARGIN);
  carrier.display.setTextSize(2);
  this->drawString(message, MARGIN, MESSAGE);
  if (drawSensors) {
    this->drawString(battery, MARGIN, CONTROLS + MARGIN);
    this->drawString(humidity, MARGIN, carrier.display.getCursorY() - TEXT_3_STEP);
    this->drawString(temperature, MARGIN, carrier.display.getCursorY() - TEXT_3_STEP);
  }
  carrier.display.setCursor(MARGIN, MESSAGE + TEXT_3_STEP);
}

void DisplayMachine::drawString(String strs[2], int x, int y) {
  this->drawString(strs, x, y, foregroundColor);
}

void DisplayMachine::drawString(String strs[2], int x, int y, uint16_t fgColor) {
  carrier.display.setCursor(x, y);
  if (strs[0] != strs[1]) {
    carrier.display.setTextColor(backgroundColor);
    carrier.display.print(strs[1]);
    carrier.display.setCursor(x, y);
    carrier.display.setTextColor(fgColor);
    carrier.display.print(strs[0]);
    strs[1] = strs[0];
  }
}

void DisplayMachine::drawPositionString(String strs[2], int x, int y, int x2, int y2) {
  this->drawPositionString(strs, x, y, x2, y2, foregroundColor, false);
}

void DisplayMachine::drawPositionString(String strs[2], int x, int y, int x2, int y2, uint16_t fgColor, bool force) {
  if (strs[0] != strs[1] || force) {
    carrier.display.setCursor(x2, y2);
    carrier.display.setTextColor(backgroundColor);
    carrier.display.print(strs[1]);
    carrier.display.setCursor(x, y);
    carrier.display.setTextColor(fgColor);
    carrier.display.print(strs[0]);
    strs[1] = strs[0];
  }
}

void DisplayMachine::printLine(String str) {
  carrier.display.print(str);
  carrier.display.setCursor(MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
}

void DisplayMachine::redraw() {
  int16_t x1, y1;
  uint16_t w, h, playAction1X, playAction2X;

  DEBUG_MACHINE("REDRAW", this->getState(), "");

  stale = false;

  switch (this->getState()) {
    case WiFi:
      this->drawBasicScreen(wifiTitle, wifiMessage, true);
      break;

    case Sonos:
      this->drawBasicScreen(sonosTitle, sonosMessage, false);
      break;

    case Error:
      this->drawBasicScreen(errorTitle, errorMessage, true);
      break;

    case Sleep:
      this->drawBasicScreen(sleepTitle, sleepMessage, true);
      break;

    case Player:
      carrier.display.setTextSize(3);
      this->drawPositionString(playButton,
                               playButton[0].length() == 1 ? (CENTER - TEXT_3_CHAR) : (CENTER - TEXT_3_CHAR * 2), MARGIN,
                               playButton[1].length() == 1 ? (CENTER - TEXT_3_CHAR) : (CENTER - TEXT_3_CHAR * 2), MARGIN);

      carrier.display.getTextBounds(action[0], 0, 0, &x1, &y1, &w, &h);
      playAction1X = (SCREEN_WIDTH - w) / 2;
      carrier.display.getTextBounds(action[1], 0, 0, &x1, &y1, &w, &h);
      playAction2X = (SCREEN_WIDTH - w) / 2;

      this->drawPositionString(action, playAction1X, CONTROLS, playAction2X, CONTROLS, actionColor[0], actionColor[0] != actionColor[1]);
      this->drawString(title, MARGIN, TOP_Y);
      this->drawString(artist, MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
      this->drawString(album, MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);

      carrier.display.setTextSize(2);
      this->drawString(repeat, PLAY_STATE, carrier.display.getCursorY() + TEXT_3_STEP + TEXT_2_STEP);
      this->drawString(shuffle, PLAY_STATE, carrier.display.getCursorY() + TEXT_3_STEP);
      this->drawString(volume, PLAY_STATE, carrier.display.getCursorY() + TEXT_3_STEP);
      break;
  }
}
