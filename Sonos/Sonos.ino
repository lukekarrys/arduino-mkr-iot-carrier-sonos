#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <URLEncoder.h>
#include "Secrets.h"
#include "SonosServer.h"
#include "Debug.h"

#define HOLD 1000
#define TAP 40
#define DOUBLE_TAP 100
#define MARGIN 5
#define TOP_Y 42
#define TEXT_3_STEP 24
#define TEXT_2_STEP 16
#define SLEEP 3600000 // 1 hour
#define COMMAND_TIMEOUT 1000
#define SUCCESS_TONE 523
#define ERROR_TONE 262
#define INFO_TONE 349
#define NOW millis()

MKRIoTCarrier carrier;

String sonosRoom = SONOS_ROOM;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, SONOS_SERVER, SONOS_PORT);
WiFiClient commandWifi;
HttpClient commandClient = HttpClient(commandWifi, SONOS_SERVER, SONOS_PORT);

const uint32_t red = carrier.leds.Color(255, 0, 0);
const uint32_t blue = carrier.leds.Color(0, 0, 255);
const uint32_t green = carrier.leds.Color(0, 255, 0);
const uint32_t off = carrier.leds.Color(0, 0, 0);

enum class WifiState { Init, Error, Connect, Status, Connected, Idle, Sleep };
WifiState wifiState = WifiState::Init;
WifiState wifiPrevState = WifiState::Sleep;
unsigned long wifiStateChange = 0;
int wifiBackoffCount = 0;

enum class ButtonsState { Locked, Unlocked };
ButtonsState buttonsState = ButtonsState::Unlocked;
ButtonsState buttonsPrevState = ButtonsState::Locked;
unsigned long buttonsStateChange = 0;

enum class ButtonState { Up, Down, Tap, Hold, TapHold, DoubleTap, Discard };
ButtonState buttonState [5] = { ButtonState::Up, ButtonState::Up, ButtonState::Up, ButtonState::Up, ButtonState::Up };
unsigned long buttonPrevDownUp [5][2] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };
char buttonPrevTouch [5] = { 0, 0, 0, 0, 0 };

enum class LedState { Off, On, BlinkOn, BlinkOff };
LedState ledState = LedState::Off;
LedState ledPrevState = LedState::On;
unsigned long ledStateChange = 0;
uint32_t ledColor = blue;
int ledButton [2] = { 0, 5 };
unsigned long ledDuration = 0;

enum class BuzzerState { Off, On };
BuzzerState buzzerState = BuzzerState::Off;
BuzzerState buzzerPrevState = BuzzerState::On;
unsigned long buzzerStateChange = 0;
unsigned long buzzerTone = SUCCESS_TONE;
unsigned long buzzerDuration = 0;

enum class ServerState { Idle, Connect, Connecting, Connected, Error };
ServerState serverState = ServerState::Idle;
ServerState serverPrevState = ServerState::Connect;
unsigned long serverStateChange = 0;
int serverBackoffCount = 0;
String serverResponse = "";

enum class CommandState { Idle, Ready, Connect, Connecting, Success, Error };
CommandState commandState = CommandState::Idle;
CommandState commandPrevState = CommandState::Ready;
unsigned long commandStateChange = 0;
String commandAction = "";
int commandButton = -1;

enum class DisplayState { Init, Error, Player, Sleep };
DisplayState displayState = DisplayState::Init;
DisplayState displayPrevState = DisplayState::Sleep;
unsigned long displayDataChange = 0;
unsigned long displayStateChange = 0;
uint16_t displayBg;
uint16_t displayFg;
String displayTitle [2] = { "", "" };
String displayMessage [2]  = { "", "" };
String displayBattery [2]  = { "", "" };
String volume [2]  = { "", "" };
String artist [2]  = { "", "" };
String title [2]  = { "", "" };
String album [2]  = { "", "" };
String repeat [2]  = { "", "" };
String shuffle [2] = { "", "" };
String sound [2] = { "", "" };
String playButton [2] = { "", "" };
String playerAction [2] = { "", "" };
bool mute [2]  = { false, false };
bool playing [2]  = { false, false };

void resetDisplay(uint16_t bg, uint16_t fg, bool textWrap) {
  carrier.display.fillScreen(bg);
  carrier.display.setTextColor(fg);
  carrier.display.setTextWrap(textWrap);
  displayBg = bg;
  displayFg = fg;
}

void setCursorY(int num) {
  carrier.display.setCursor(carrier.display.getCursorX(), num);
}

void addCursorY(int num) {
  setCursorY(carrier.display.getCursorY() + num);
}

void setCursorX(int num) {
  carrier.display.setCursor(num, carrier.display.getCursorY());
}

void addCursorX(int num) {
  setCursorX(carrier.display.getCursorX() + num);
}

void overwriteString(String strs [2], int x, int y, uint16_t fgColor = displayFg, bool force = false) {
  carrier.display.setCursor(x, y);
  if (strs[0] != strs[1] || force) {
    carrier.display.setTextColor(displayBg);
    carrier.display.print(strs[1]);
    carrier.display.setCursor(x, y);
    carrier.display.setTextColor(fgColor);
    carrier.display.print(strs[0]);
    strs[1] = strs[0];
  }
}

void overwritePositionedString(String strs [2], int x, int y, int x2, int y2, uint16_t fgColor = displayFg, bool force = false) {
  if (strs[0] != strs[1] || force) {
    carrier.display.setCursor(x2, y2);
    carrier.display.setTextColor(displayBg);
    carrier.display.print(strs[1]);
    carrier.display.setCursor(x, y);
    carrier.display.setTextColor(fgColor);
    carrier.display.print(strs[0]);
    strs[1] = strs[0];
  }
}

void displayStale() {
  displayDataChange = NOW;
}

void setTitle(String str) {
  TRACE("Set title: ");
  TRACELN(str);
  displayTitle[0] = str;
  displayStale();
}

void setMessage(String str) {
  TRACE("Set message: ");
  TRACELN(str);
  displayMessage[0] = str;
  displayStale();
}

String getBattery() {
  const int battery = readBattery();
  return(String(battery) + "%");
}

void setBattery() {
  const String battery = getBattery();
  TRACE("Set battery: ");
  TRACELN(battery);
  displayBattery[0] = "Battery: " + battery;
  displayStale();
}

void setPlayerState(String rawState) {
  TRACE("Set player state: ");
  TRACELN(rawState);

  if (!rawState.length()) {
    volume[0] = "";
    volume[1] = "";
    artist[0] = "";
    artist[1] = "";
    title[0] = "";
    title[1] = "";
    album[0] = "";
    album[1] = "";
    repeat[0] = "";
    repeat[1] = "";
    shuffle[0] = "";
    shuffle[1] = "";
    mute[0] = false;
    mute[1] = false;
    sound[0] = "";
    sound[1] = "";
    playing[0] = false;
    playing[1] = false;
    playButton[0] = "";
    playButton[1] = "";
    if (displayState == DisplayState::Player) {
      displayStale();
    }
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, rawState);

  if (error) {
    PRINT("json error: ");
    PRINTLN(error.f_str());
    return;
  }

  bool didUpdate = false;
  const String type = doc["type"];

  if (type == "transport-state") {
    volume[0] = doc["data"]["state"]["volume"].as<String>();
    mute[0] = doc["data"]["state"]["mute"];
    artist[0] = doc["data"]["state"]["currentTrack"]["artist"].as<String>();
    title[0] = doc["data"]["state"]["currentTrack"]["title"].as<String>();
    album[0] = doc["data"]["state"]["currentTrack"]["album"].as<String>();
    repeat[0] = doc["data"]["state"]["playMode"]["repeat"].as<String>();
    shuffle[0] = doc["data"]["state"]["playMode"]["shuffle"] ? "on" : "off";
    playing[0] = doc["data"]["state"]["playbackState"].as<String>() == "PLAYING";
    didUpdate = true;
  } else if (type == "volume-change") {
    volume[0] = doc["data"]["newVolume"].as<String>();
    didUpdate = true;
  } else if (type == "mute-change") {
    mute[0] = doc["data"]["newMute"];
    didUpdate = true;
  }

  if (didUpdate) {
    if (displayState == DisplayState::Player) {
      displayStale();
    }
    sound[0] = mute[0] ? "muted" : volume[0];
    playButton[0] = playing[0] ? "||" : ">";
    PRINT("type:");
    PRINT(doc["type"].as<String>());
    PRINT(" playing:");
    PRINT(playing[0]);
    PRINT(" volume:");
    PRINT(volume[0]);
    PRINT(" mute:");
    PRINT(mute[0]);
    PRINT(" sound:");
    PRINT(sound[0]);
    PRINT(" artist:");
    PRINT(artist[0]);
    PRINT(" title:");
    PRINT(title[0]);
    PRINT(" album:");
    PRINT(album[0]);
    PRINT(" repeat:");
    PRINT(repeat[0]);
    PRINT(" shuffle:");
    PRINTLN(shuffle[0]);
  } else {
    PRINTLN("No player update");
  }
}

void setup() {
  // Serial.begin has to be called for the display to work (not sure why?) but
  // we cant wait for it since the code will run without serial connection when
  // not plugged in. So 2000 is a decent enough time to wait for a connectioin
  // if it exists and not lose anything.
  Serial.begin(9600);
  delay(2000);

  carrier.withCase();
  carrier.Buttons.updateConfig(200);
  carrier.leds.setBrightness(1);

  // Set timeout to 0 so begin returns right away and the loop deals with
  // polling for connection status
  WiFi.setTimeout(0);

  if (!carrier.begin()) {
    Serial.print("Error: no carrier");
    while (true);
  }

  setupBattery();
  setBattery();
}

void loop() {
  PERF_LOOP();

  PERF_MACHINE_START();
  buttonsMachine();
  PERF_MACHINE_END("BUTTONS", 16);

  PERF_MACHINE_START();
  displayMachine();
  PERF_MACHINE_END("DISPLAY", 16);

  PERF_MACHINE_START();
  ledMachine();
  PERF_MACHINE_END("LED", 16);
  
  PERF_MACHINE_START();
  buzzerMachine();
  PERF_MACHINE_END("BUZZER", 16);
  
  PERF_MACHINE_START();
  wifiMachine();
  PERF_MACHINE_END("WIFI", 16);

  PERF_MACHINE_START();
  serverMachine();
  PERF_MACHINE_END("SERVER", 16);
  
  PERF_MACHINE_START();
  commandMachine();
  PERF_MACHINE_END("COMMAND", 16);
}

// ==============================
//
// WiFi State Machine
//
// ==============================
void wifiMachine() {
  const WifiState enterState = wifiState;
  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (wifiPrevState != wifiState) {
    TRACE_STATE_CHANGE("WIFI", wifiPrevState, wifiState, wifiStateChange);
    switch (wifiState) {
    case WifiState::Init:
      PRINTLN("Init wifi machine");
      setTitle("Starting");
      setMessage("");
      buttonsState = ButtonsState::Unlocked;
      // Deal with unrecoverable error states that require physical connections
      // or flashing firmware
      if (
        WiFi.status() == WL_NO_MODULE ||
        WiFi.status() == WL_NO_SHIELD ||
        WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION
      ) {
        wifiState = WifiState::Error;
      } else {
        wifiState = WifiState::Connect;
      }
      break;

    case WifiState::Error:
      PRINTLN("WiFi Error:");
      carrier.Buzzer.beep(ERROR_TONE, 20);
      displayState = DisplayState::Error;
      if (WiFi.status() == WL_NO_MODULE) {
        setTitle("WL_NO_MODULE");
      } else if (WiFi.status() == WL_NO_SHIELD) {
        setTitle("WL_NO_SHIELD");
      } else if (WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION) {
        setTitle("Upgrade WiFi Firmware to " + String(WIFI_FIRMWARE_LATEST_VERSION));
      } else {
        setTitle("Unknown WiFi Error");
      }
      setMessage("Fix the error and reboot");
      break;

    case WifiState::Connect:
      PRINTLN("Begin wifi connection");
      carrier.Buzzer.beep(INFO_TONE, 20);
      setTitle("WiFi:");
      setMessage("Connecting");
      PERF_COMMAND_START();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      PERF_COMMAND_END("WiFi.begin");
      wifiState = WifiState::Status;
      ledState = LedState::BlinkOn;
      break;

    case WifiState::Status:
      if (WiFi.status() == WL_CONNECTED) {
        wifiState = WifiState::Connected;
        if (wifiPrevState != WifiState::Connected) {
          serverState = ServerState::Connect;
          ledState = LedState::Off;
          wifiBackoffCount = 0;
          setMessage("Connected!");
          PRINT("WiFi connected: ");
          PRINT(WiFi.SSID());
          PRINT(" Signal: ");
          PRINTLN(WiFi.RSSI());
        }
      } else {
        wifiState = WifiState::Idle;
      }
      if (wifiPrevState == WifiState::Connected) {
        TRACELN("Checking wifi connection");
      } else {
        PRINTLN("Checking wifi connection");
      }
      break;

    case WifiState::Idle:
      serverState = ServerState::Idle;
      wifiBackoffCount += 1;
      setMessage("Checking in " + String(pow(2, wifiBackoffCount)) + "s");
      PRINT("WiFi backoff count: ");
      PRINTLN(wifiBackoffCount);
      break;

    case WifiState::Sleep:
      PRINTLN("Sleeping WiFi");
      carrier.Buzzer.beep(INFO_TONE, 20);
      serverState = ServerState::Idle;
      displayState = DisplayState::Sleep;
      WiFi.end();
      setBattery();
      if (serverState == ServerState::Error) {
        setTitle("Sonos:");
        setMessage("Could not to server, hold any button to try again");
      } else if (wifiPrevState == WifiState::Connected) {
        setTitle("Sleeping:");
        setMessage("No changes for an hour, hold any button to wake");
      } else {
        setTitle("WiFi:");
        setMessage("Could not connect, hold any button to try again");
      }
      break;
    }

    wifiStateChange = NOW;
  }

  // ==============================
  //
  // POLL STATES
  //
  // ==============================
  const unsigned long sinceChange = NOW - wifiStateChange;
  switch (wifiState) {
  case WifiState::Idle:
    if (wifiBackoffCount >= 10) {
      wifiState = WifiState::Sleep;
    } else if (sinceChange > (1000 * pow(2, wifiBackoffCount))) {
      wifiState = WifiState::Status;
    }
    break;

  case WifiState::Connected:
    if (NOW - serverStateChange > SLEEP && NOW - commandStateChange > SLEEP) {
      wifiState = WifiState::Sleep;
    } else if (sinceChange > 5000) {
      wifiState = WifiState::Status;
    }
    break;

  case WifiState::Sleep:
    if (sinceChange > 60000) {
      setBattery();
      wifiStateChange = NOW;
    }
    break;
  }

  wifiPrevState = enterState;
}

// ==============================
//
// LED Blink Machine
// Blinks all LEDs blue/off at intervals
//
// ==============================
void ledMachine() {
  const LedState enterState = ledState;
  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (ledPrevState != ledState) {
    TRACE_STATE_CHANGE("LED", ledPrevState, ledState, ledStateChange);
    switch (ledState) {    
    case LedState::On:
    case LedState::BlinkOn:
      carrier.leds.fill(ledColor, ledButton[0], ledButton[1]);
      carrier.leds.show();
      break;

    case LedState::Off:
    case LedState::BlinkOff:
      carrier.leds.fill(off, ledButton[0], ledButton[1]);
      carrier.leds.show();
      break;
    }

    ledStateChange = NOW;
  }

  // ==============================
  //
  // POLL STATES
  //
  // ==============================
  const unsigned long sinceChange = NOW - ledStateChange;
  switch (ledState) {
  case LedState::On:
    if (ledDuration && sinceChange > ledDuration) {
      ledState = LedState::Off;
    }
    break;

  case LedState::BlinkOn:
    if (sinceChange >= 200) {
      ledState = LedState::BlinkOff;
    }
    break;

  case LedState::BlinkOff:
    if (sinceChange >= 200) {
      ledState = LedState::BlinkOn;
    }
    break;
  }

  ledPrevState = enterState;
}

// ==============================
//
// Buzzer Machine
//
// ==============================
void buzzerMachine() {
  const BuzzerState enterState = buzzerState;
  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (buzzerPrevState != buzzerState) {
    TRACE_STATE_CHANGE("BUZZER", buzzerPrevState, buzzerState, buzzerStateChange);
    switch (buzzerState) {    
    case BuzzerState::On:
      carrier.Buzzer.sound(buzzerTone);
      break;

    case BuzzerState::Off:
      carrier.Buzzer.noSound();
      break;
    }

    buzzerStateChange = NOW;
  }

  const unsigned long sinceChange = NOW - buzzerStateChange;
  switch (buzzerState) {    
  case BuzzerState::On:
    if (buzzerDuration && sinceChange > buzzerDuration) {
      buzzerState = BuzzerState::Off;
    }
    break;
  }

  buzzerPrevState = enterState;
}

// ==============================
//
// Server Machine
// Used to read and maintain connection
// to the server SSE stream
//
// ==============================
void serverMachine() {
  const ServerState enterState = serverState;
  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (serverPrevState != serverState) {
    TRACE_STATE_CHANGE("SERVER", serverPrevState, serverState, serverStateChange);
    switch (serverState) {    
    case ServerState::Connect:
      client.stop();
      displayState = DisplayState::Init;
      setTitle("Sonos:");
      setMessage("Connecting to " + sonosRoom);
      PERF_COMMAND_START();
      client.get("/" + URLEncoderClass::encode(sonosRoom) + "/events");
      PERF_COMMAND_END("client.get(/events)");
      serverState = ServerState::Connecting;
      PRINTLN("GET /events");
      break;

    case ServerState::Error:
      serverBackoffCount += 1;
      setMessage("Retrying in " + String(pow(2, serverBackoffCount)) + "s");
      client.stop();
      break;

    case ServerState::Idle:
      commandState = CommandState::Idle;
      client.stop();
      break;

    case ServerState::Connected:
      setMessage("Connected");
      serverBackoffCount = 0;
      displayState = DisplayState::Player;
      commandState = CommandState::Ready;
      client.skipResponseHeaders();
      break;
    }

    serverStateChange = NOW;
  }

  // ==============================
  //
  // POLL STATES
  //
  // ==============================
  const unsigned long sinceChange = NOW - serverStateChange;
  switch (serverState) {  
  case ServerState::Connecting:
    if (client.available()) {
      PERF_COMMAND_START();
      int statusCode = client.responseStatusCode();
      PERF_COMMAND_END("client.get(/events).responseStatusCode");
      if (statusCode != 200) {
        serverState = ServerState::Error;
      } else {
        serverState = ServerState::Connected;
      }
    }
    break;

  case ServerState::Error:
    if (serverBackoffCount >= 10) {
      wifiState = WifiState::Sleep;
    } else if (sinceChange > (1000 * pow(2, serverBackoffCount))) {
      serverState = ServerState::Connect;
    }
    break;

  case ServerState::Connected:
    if (!client.connected()) {
      serverState = ServerState::Connect;
    } else if (client.available()) {
      const char c = client.read();
      if (c == '\n') {
        serverResponse.remove(0, 6);
        if (serverResponse.length()) {
          setPlayerState(serverResponse);
        }
        serverResponse = "";
      } else {
        serverResponse += c;
      }
    }
    break;
  }

  serverPrevState = enterState;
}

// ==============================
//
// Command Machine
//
// ==============================
void commandMachine() {
  const CommandState enterState = commandState;
  // ==============================
  //
  // EXIT STATES
  //
  // ==============================
  if (commandPrevState != commandState) {
    TRACE_STATE_CHANGE("COMMAND EXIT", commandPrevState, commandState, commandStateChange);
    switch (commandPrevState) {        
    case CommandState::Success:
    case CommandState::Error:
      buzzerState = BuzzerState::Off;
      commandAction = "";
      playerAction[0] = getBattery();
      commandButton = -1;
      displayStale();
      commandClient.stop();
      break;

    case CommandState::Connecting:
      commandClient.stop();
      break;
    }
  }

  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (commandPrevState != commandState) {
    TRACE_STATE_CHANGE("COMMAND ENTER", commandPrevState, commandState, commandStateChange);
    switch (commandState) {
    case CommandState::Ready:
    case CommandState::Idle:
      buzzerState = BuzzerState::Off;
      commandAction = "";
      playerAction[0] = getBattery();
      commandButton = -1;
      displayStale();
      commandClient.stop();
      break;
  
    case CommandState::Success:
      buzzerState = BuzzerState::On;
      buzzerTone = SUCCESS_TONE;
      buzzerDuration = 40;
      displayStale();
      break;

    case CommandState::Error:
      buzzerState = BuzzerState::On;
      buzzerTone = ERROR_TONE;
      buzzerDuration = 40;
      displayStale();
      break;

    case CommandState::Connect:
      PERF_COMMAND_START();
      commandClient.get("/" + URLEncoderClass::encode(sonosRoom) + "/" + commandAction);
      PERF_COMMAND_END("commandClient.get(/action)");
      commandState = CommandState::Connecting;
      buzzerState = BuzzerState::On;
      buzzerTone = INFO_TONE;
      buzzerDuration = 0;
      // blue led while the request is running
      ledState = LedState::On;
      ledButton[0] = commandButton;
      ledButton[1] = 1;
      ledColor = blue;
      displayStale();
      PRINT("GET ");
      PRINTLN(commandAction);
      break;
    }

    commandStateChange = NOW;
  }

  // ==============================
  //
  // POLL STATES
  //
  // ==============================
  const unsigned long sinceChange = NOW - commandStateChange;
  switch (commandState) {
    case CommandState::Connecting:
      if (commandClient.available()) {
        int statusCode = commandClient.responseStatusCode();
        PRINT("Code: ");
        PRINTLN(statusCode);
        commandState = statusCode == 200 ? CommandState::Success : CommandState::Error;
        ledState = LedState::Off;
        buzzerState = BuzzerState::Off;
      } else if (sinceChange > COMMAND_TIMEOUT) {
        commandState = CommandState::Error;
        buzzerState = BuzzerState::Off;
      }
    break;

  case CommandState::Success:
  case CommandState::Error:
    if (sinceChange > 500) {
      commandState = CommandState::Ready;
    }
    break;
  }

  commandPrevState = enterState;
}

void sendCommand(int button, String action, String msg) {
  if (commandPrevState != CommandState::Connecting && commandPrevState != CommandState::Idle) {
    // No concurrent requests or if the server is not connected
    commandState = CommandState::Connect;
    commandAction = action;
    commandButton = button;
    playerAction[0] = msg;
  }
}

// ==============================
//
// Display machine
//
// ==============================
void displayMachine() {
  const DisplayState enterState = displayState;
  const bool displayHasNewState = displayPrevState != displayState;
  const bool displayHasNewData = displayDataChange > 0;

    // ==============================
  //
  // EXIT STATES
  //
  // ==============================
  if (displayHasNewState) {
    switch (displayPrevState) {
    case DisplayState::Sleep:
    case DisplayState::Init:
    case DisplayState::Error:
      // setTitle("");
      // setMessage("");
      break;

    case DisplayState::Player:
      setPlayerState("");
      break;
    }
  }
  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (displayHasNewState) {
    TRACE_STATE_CHANGE("DISPLAY", displayPrevState, displayState, displayStateChange);
    switch (displayState) {  
    case DisplayState::Sleep:
    case DisplayState::Init:
      resetDisplay(ST77XX_BLACK, ST77XX_WHITE, true);
      break;

    case DisplayState::Error:
      resetDisplay(ST77XX_RED, ST77XX_BLACK, true);
      break;

    case DisplayState::Player:
      resetDisplay(ST77XX_BLACK, ST77XX_WHITE, false);
      carrier.display.setTextSize(3);
      carrier.display.setCursor(MARGIN, MARGIN);
      carrier.display.print("-");
      carrier.display.setCursor(215, MARGIN);
      carrier.display.print("+");
      carrier.display.setCursor(MARGIN, 215);
      carrier.display.print("<");
      carrier.display.setCursor(215, 215);
      carrier.display.print(">");

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
  }

  if (displayHasNewState || displayHasNewData) {
    const unsigned long sinceRedraw = NOW - displayStateChange;

    TRACE("Redraw display: ");
    TRACE(sinceRedraw);
    TRACE(" - ");
    TRACE(displayHasNewState ? "state" : "data");
    TRACELN("");

    int16_t  x1, y1;
    uint16_t w, h, playAction1X, playAction2X;

    switch (displayState) {
    case DisplayState::Player:
      carrier.display.setTextSize(3);
      overwritePositionedString(playButton, playButton[0].length() == 1 ? 111 : 102, MARGIN, playButton[1].length() == 1 ? 111 : 102, MARGIN);

      carrier.display.getTextBounds(playerAction[0], 0, 0, &x1, &y1, &w, &h);
      playAction1X = (240 - w) / 2;
      carrier.display.getTextBounds(playerAction[1], 0, 0, &x1, &y1, &w, &h);
      playAction2X = (240 - w) / 2;

      overwritePositionedString(playerAction, playAction1X, 215, playAction2X, 215,
        commandState == CommandState::Success
        ? ST77XX_GREEN
        : commandState == CommandState::Error || buttonsState == ButtonsState::Locked
        ? ST77XX_RED
        : ST77XX_WHITE,
      true);

      overwriteString(title, MARGIN, TOP_Y);
      overwriteString(artist, MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
      overwriteString(album, MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);

      carrier.display.setTextSize(2);
      overwriteString(repeat, 110, carrier.display.getCursorY() + TEXT_3_STEP + TEXT_2_STEP);
      overwriteString(shuffle, 110, carrier.display.getCursorY() + TEXT_3_STEP);
      overwriteString(sound, 110, carrier.display.getCursorY() + TEXT_3_STEP);      
      break;

    default:
      carrier.display.setTextSize(3);
      overwriteString(displayTitle, MARGIN, MARGIN);
      carrier.display.setTextSize(2);
      overwriteString(displayMessage, MARGIN, 50);
      overwriteString(displayBattery, MARGIN, 220);
      break;
    }

    displayStateChange = NOW;
  }

  displayDataChange = 0;
  displayPrevState = enterState;
}

void buttonsMachine() {
  const ButtonsState enterState = buttonsState;

  carrier.Buttons.update();

  if (wifiState == WifiState::Sleep) {
    if (buttonMachine(TOUCH4) == ButtonState::TapHold) {
      wifiState = WifiState::Init;
    }
    return;
  }

  // ==============================
  //
  // ENTER STATES
  //
  // ==============================
  if (buttonsPrevState != buttonsState) {
    TRACE_STATE_CHANGE("BUTTONS", buttonsPrevState, buttonsState, buttonsStateChange);
    switch (buttonsState) {    
    case ButtonsState::Locked:
      playerAction[0] = "Locked";
      buzzerState = BuzzerState::On;
      buzzerTone = INFO_TONE;
      buzzerDuration = 40;
      displayStale();
      break;

    case ButtonsState::Unlocked:
      playerAction[0] = "";
      buzzerState = BuzzerState::On;
      buzzerTone = INFO_TONE;
      buzzerDuration = 40;
      displayStale();
      break;
    }

    buttonsStateChange = NOW;
  }

    // ==============================
  //
  // POLL STATES
  //
  // ==============================
  const unsigned long sinceChange = NOW - buttonsStateChange;
  switch (buttonsState) {
  case ButtonsState::Locked:
    if (buttonMachine(TOUCH4) == ButtonState::TapHold) {
      buttonsState = ButtonsState::Unlocked;
    }
    break;

  case ButtonsState::Unlocked:
    switch (buttonMachine(TOUCH0)) {
    case ButtonState::Tap:
      sendCommand(TOUCH0, "previous", "Prev");
      break;
    case ButtonState::DoubleTap:
      sendCommand(TOUCH0, "trackseek/1", "Start");
      break;
    case ButtonState::TapHold:
      sonosRoom = sonosRoom == SONOS_ROOM ? "Kitchen" : SONOS_ROOM;
      serverState = ServerState::Connect;
      break;
    }
    
    switch (buttonMachine(TOUCH1)) {
    case ButtonState::Tap:
      sendCommand(TOUCH1, "volume/-1", "Vol -1");
      break;
    case ButtonState::DoubleTap:
      sendCommand(TOUCH1, "volume/-5", "Vol -5");
      break;
    case ButtonState::Hold:
      sendCommand(TOUCH1, "togglemute", mute[0] ? "Unmute" : "Mute");
      break;
    }

    switch (buttonMachine(TOUCH2)) {
    case ButtonState::Tap:
      sendCommand(TOUCH2, "playpause", playing[0] ? "Pause" : "Play");
      break;
    case ButtonState::DoubleTap:
      sendCommand(TOUCH2, "shuffle/toggle", shuffle[0] == "on" ? "Shuf Off" : "Shuf On");
      break;
    case ButtonState::Hold:
      sendCommand(TOUCH2, "repeat/toggle", "Rep " + String(repeat[0] == "all" ? "One" : repeat[0] == "one" ? "None" : "All"));
      break;
    }

    switch (buttonMachine(TOUCH3)) {
    case ButtonState::Tap:
      sendCommand(TOUCH3, "volume/+1", "Vol +1");
      break;
    case ButtonState::DoubleTap:
      sendCommand(TOUCH3, "volume/+5", "Vol +5");
      break;
    case ButtonState::Hold:
      sendCommand(TOUCH3, "togglemute", mute[0] ? "Unmute" : "Mute");
      break;
    }

    switch (buttonMachine(TOUCH4)) {
    case ButtonState::Tap:
      sendCommand(TOUCH4, "next", "Next");
      break;
    case ButtonState::Hold:
      wifiState = WifiState::Sleep;
      break;
    case ButtonState::TapHold:
      buttonsState = ButtonsState::Locked;
      break;
    }
    break;
  }


  buttonsPrevState = enterState;
}

ButtonState buttonMachine(touchButtons btn) {
  const unsigned long lastDown = NOW - buttonPrevDownUp[btn][0];
  const unsigned long lastUp = NOW - buttonPrevDownUp[btn][1];
  const bool touch = carrier.Buttons.getTouch(btn);

  // The buttons are capacitive and even at a non-sensitive config level
  // there is noise when a finger hovers close to the touchpad. This results
  // in state quickly osscilating between up/down. I tested by quickly
  // tapping and could never get it under 30ms, so anything under 30ms is
  // deemed noise which means we just reset the button state to up.
  const bool discard = touch ? lastUp < TAP : lastDown < TAP;

  const ButtonState state = discard ? ButtonState::Discard : buttonState[btn];
  ButtonState action = ButtonState::Up;

  switch (state) {
  case ButtonState::Up:
    if (touch) {
      buttonState[btn] = ButtonState::Down;
    }
    break;

  case ButtonState::Down:
    if (touch) {
      if (lastDown > HOLD) {
        buttonState[btn] = ButtonState::Hold;
        action = ButtonState::Hold;
      }
    } else {
      buttonState[btn] = ButtonState::Tap;
    }
    break;

  case ButtonState::Hold:
    if (!touch) {
      buttonState[btn] = ButtonState::Up;
    }
    break;

  case ButtonState::Tap:
    if (!touch) {
      if (lastUp > DOUBLE_TAP) {
        buttonState[btn] = ButtonState::Up;
        action = ButtonState::Tap;
      }
    } else {
      buttonState[btn] = ButtonState::DoubleTap;
    }
    break;

  case ButtonState::DoubleTap:
    if (touch) {
      if (lastDown > HOLD) {
        buttonState[btn] = ButtonState::Hold;
        action = ButtonState::TapHold;
      }
    } else {
      if (lastUp > DOUBLE_TAP) {
        buttonState[btn] = ButtonState::Up;
        action = ButtonState::DoubleTap;
      }
    }
    break;
  }

  const unsigned long lastTouch = buttonPrevTouch[btn];
  if (touch && !lastTouch) {
    buttonPrevTouch[btn] = 1;
    buttonPrevDownUp[btn][0] = NOW;
    TRACE(btn);
    TRACE(" DOWN - LASTDOWN:");
    TRACE(lastDown);
    TRACE("ms LASTUP:");
    TRACE(lastUp);
    TRACELN("ms");
  } else if (!touch && lastTouch) {
    buttonPrevTouch[btn] = 0;
    buttonPrevDownUp[btn][1] = NOW;
    TRACE(btn);
    TRACE(" UP - LASTDOWN:");
    TRACE(lastDown);
    TRACE("ms LASTUP:");
    TRACE(lastUp);
    TRACELN("ms");
  }

  return(action);
}
