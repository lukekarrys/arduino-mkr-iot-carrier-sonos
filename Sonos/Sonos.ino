#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "Secrets.h"
#include "SonosServer.h"

#define HOLD 600
#define TAP 30
#define DOUBLE_TAP 100
#define MARGIN 5
#define TOP_Y 42
#define TEXT_3_STEP 24
#define TEXT_2_STEP 16
#define NOW millis()


#ifdef DEBUG
  #define PRINT(...) Serial.print(__VA_ARGS__)
  #define PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define PRINT(...)
  #define PRINTLN(...)
#endif

#ifdef TRACE
  #define TRACE(...) Serial.print(__VA_ARGS__)
  #define TRACELN(...) Serial.println(__VA_ARGS__)
  #define DELAY(...) delay(__VA_ARGS__)
  #define TRACE_STATE_CHANGE(name, prev, current, change) {\
    Serial.print(name);\
    Serial.print(" STATE CHANGE: ");\
    Serial.print(static_cast<int>(prev));\
    Serial.print(" -> ");\
    Serial.print(static_cast<int>(current));\
    Serial.print(" - ");\
    Serial.print(NOW - change);\
    Serial.println("ms");\
  }
#else
  #define TRACE(...)
  #define TRACELN(...)
  #define DELAY(...)
  #define TRACE_STATE_CHANGE(...)
#endif

#ifdef PERF
  unsigned long PERF_LAST_LOOP = 0;
  unsigned long PERF_LAST_COMMAND = 0;
  unsigned long PERF_LAST_MACHINE = 0;
  unsigned long PERF_LOOP_COUNT = 0;
  #define PERF_LOOP() {\
    if (PERF_LAST_LOOP > 0) {\
      const unsigned long latest = NOW - PERF_LAST_LOOP;\
      if (latest > 16) {\
        Serial.print("LOOP PERF: ");\
        Serial.print(latest);\
        Serial.print("ms - COUNT: ");\
        Serial.println(PERF_LOOP_COUNT);\
        PERF_LOOP_COUNT = 0;\
      }\
    }\
    PERF_LOOP_COUNT++;\
    PERF_LAST_LOOP = NOW;\
  }
  #define PERF_COMMAND_START() {\
    PERF_LAST_COMMAND = NOW;\
  }
  #define PERF_COMMAND_END(msg) {\
    const unsigned long latest = NOW - PERF_LAST_COMMAND;\
    Serial.print("COMMAND PERF ");\
    Serial.print(msg);\
    Serial.print(": ");\
    Serial.println(latest);\
    PERF_LAST_COMMAND = 0;\
  }
  #define PERF_MACHINE_START() {\
    PERF_LAST_MACHINE = NOW;\
  }
  #define PERF_MACHINE_END(msg, slow) {\
    const unsigned long latest = NOW - PERF_LAST_MACHINE;\
    if (latest >= slow) {\
      Serial.print("MACHINE PERF ");\
      Serial.print(msg);\
      Serial.print(": ");\
      Serial.println(latest);\
    }\
    PERF_LAST_MACHINE = 0;\
  }
#else
  #define PERF_LOOP()
  #define PERF_COMMAND_START()
  #define PERF_COMMAND_END(...)
  #define PERF_MACHINE_START(...)
  #define PERF_MACHINE_END(...)
#endif

MKRIoTCarrier carrier;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, SONOS_SERVER, SONOS_PORT);
WiFiClient commandWifi;
HttpClient commandClient = HttpClient(commandWifi, SONOS_SERVER, SONOS_PORT);

const uint32_t red = carrier.leds.Color(255, 0, 0);
const uint32_t blue = carrier.leds.Color(0, 0, 255);
const uint32_t green = carrier.leds.Color(0, 255, 0);
const uint32_t off = carrier.leds.Color(0, 0, 0);

enum class ButtonState { Up, Down, Tap, Hold, TapHold, DoubleTap, Discard };
ButtonState buttonState [5] = { ButtonState::Up, ButtonState::Up, ButtonState::Up, ButtonState::Up, ButtonState::Up };
unsigned long buttonPhysical [5][2] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };
char buttonStateChange [5][2] = { {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0} };

enum class WifiState { Init, Error, Connect, Status, Connected, Idle, Sleep };
WifiState wifiState = WifiState::Init;
WifiState wifiPrevState = WifiState::Sleep;
unsigned long wifiStateChange = 0;
int wifiBackoffCount = 0;

enum class LedState { Off, On, BlinkOn, BlinkOff };
LedState ledState = LedState::Off;
LedState ledPrevState = LedState::On;
unsigned long ledStateChange = 0;
uint32_t ledColor = blue;
int ledButton [2] = { 0, 5 };

enum class BuzzerState { Off, On };
BuzzerState buzzerState = BuzzerState::Off;
BuzzerState buzzerPrevState = BuzzerState::On;
unsigned long buzzerStateChange = 0;
unsigned long buzzerTone = 800;

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

void setDisplayColors(uint16_t bg, uint16_t fg) {
  carrier.display.fillScreen(bg);
  carrier.display.setTextColor(fg);
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

void displayStale() {
  displayDataChange = NOW;
}

void setTitle(String str) {
  displayTitle[0] = str;
  displayStale();
}

void setMessage(String str) {
  displayMessage[0] = str;
  displayStale();
}

void setPlayerState(String rawState) {
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
      setTitle("Starting...");
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
      PRINTLN("Init wifi machine");
      break;

    case WifiState::Error:
      carrier.Buzzer.beep(400, 20);
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
      PRINTLN("WiFi error");
      break;

    case WifiState::Connect:
      carrier.Buzzer.beep(800, 20);
      setTitle("Connecting...");
      PERF_COMMAND_START();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      PERF_COMMAND_END("WiFi.begin");
      wifiState = WifiState::Status;
      ledState = LedState::BlinkOn;
      PRINTLN("Begin wifi connection");
      break;

    case WifiState::Status:
      if (WiFi.status() == WL_CONNECTED) {
        wifiState = WifiState::Connected;
        if (wifiPrevState != WifiState::Connected) {
          serverState = ServerState::Connect;
          ledState = LedState::Off;
          wifiBackoffCount = 0;
          setTitle("Connected");
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
      PRINT("WiFi backoff count: ");
      PRINTLN(wifiBackoffCount);
      break;

    case WifiState::Sleep:
      serverState = ServerState::Idle;
      displayState = DisplayState::Sleep;
      WiFi.end();
      if (wifiPrevState == WifiState::Connected) {
        setTitle("Sleeping");
        setMessage("Hold any button to wake");
      } else {
        setTitle("No WiFi Found");
        setMessage("Hold any button to try again");
      }
      PRINTLN("Sleeping WiFi");
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
    if (serverStateChange > 3600000 && commandStateChange > 3600000) {
      wifiState = WifiState::Sleep;
    } else if (sinceChange > 5000) {
      wifiState = WifiState::Status;
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
      displayState = DisplayState::Init;
      setTitle("Sonos...");
      PERF_COMMAND_START();
      client.get("/" + String(SONOS_ROOM) + "/events");
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
      setTitle("Could not connect to server");
      setMessage("Reboot to try again");
      serverState = ServerState::Idle;
      displayState = DisplayState::Error;
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
      ledState = LedState::Off;
      buzzerState = BuzzerState::Off;
      commandAction = "";
      playerAction[0] = "";
      commandButton = -1;
      displayStale();
      break;

    case CommandState::Connecting:
      PERF_COMMAND_START();
      commandClient.stop();
      PERF_COMMAND_END("commandClient.stop()");
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
    case CommandState::Success:
      buzzerState = BuzzerState::On;
      buzzerTone = 1200;
      ledState = LedState::On;
      ledColor = green;
      displayStale();
      break;

    case CommandState::Error:
      buzzerState = BuzzerState::On;
      buzzerTone = 400;
      ledState = LedState::On;
      ledColor = red;
      displayStale();
      break;

    case CommandState::Connect:
      if (commandPrevState == CommandState::Connecting) {
        commandState = CommandState::Connecting;
      } else {
        PERF_COMMAND_START();
        commandClient.get("/" + String(SONOS_ROOM) + "/" + commandAction);
        PERF_COMMAND_END("commandClient.get(/action)");
        commandState = CommandState::Connecting;
        buzzerState = BuzzerState::On;
        buzzerTone = 800;
        ledState = LedState::BlinkOn;
        ledButton[0] = commandButton;
        ledButton[1] = 1;
        ledColor = blue;
        displayStale();
        PRINT("GET ");
        PRINTLN(commandAction);
      }
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
      if (sinceChange > 20) {
        buzzerState = BuzzerState::Off;
      }
      if (commandClient.available()) {
        int statusCode = commandClient.responseStatusCode();
        PRINT("Code: ");
        PRINTLN(statusCode);
        commandState = statusCode == 200 ? CommandState::Success : CommandState::Error;
        buzzerState = BuzzerState::Off;
      } else if (sinceChange > 500) {
        buzzerState = BuzzerState::Off;
        commandState = CommandState::Error;
      }
    break;

  case CommandState::Success:
  case CommandState::Error:
    if (sinceChange > 20) {
      buzzerState = BuzzerState::Off;
    }
    if (sinceChange > 500) {
      commandState = CommandState::Ready;
    }
    break;
  }

  commandPrevState = enterState;
}

void sendCommand(int button, String action, String msg) {
  commandState = CommandState::Connect;
  commandAction = action;
  commandButton = button;
  playerAction[0] = msg;
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
  // ENTER STATES
  //
  // ==============================
  if (displayHasNewState) {
    TRACE_STATE_CHANGE("DISPLAY", displayPrevState, displayState, displayStateChange);
    switch (displayState) {  
    case DisplayState::Sleep:
    case DisplayState::Init:
      setDisplayColors(ST77XX_BLACK, ST77XX_WHITE);
      carrier.display.setTextWrap(true);
      break;

    case DisplayState::Error:
      setDisplayColors(ST77XX_RED, ST77XX_BLACK);
      carrier.display.setTextWrap(true);
      break;

    case DisplayState::Player:
      setDisplayColors(ST77XX_BLACK, ST77XX_WHITE);
      carrier.display.setTextWrap(false);
      carrier.display.setTextSize(3);
      carrier.display.setCursor(MARGIN, MARGIN);
      carrier.display.print("-");
      carrier.display.setCursor(215, MARGIN);
      carrier.display.print("+");
      carrier.display.setCursor(MARGIN, 215);
      carrier.display.print("<");
      carrier.display.setCursor(215, 215);
      carrier.display.print(">");

      carrier.display.setTextSize(2);
      carrier.display.setCursor(MARGIN, TOP_Y + (TEXT_3_STEP * 3) + TEXT_2_STEP);

      carrier.display.print("Repeat: ");
      carrier.display.setCursor(MARGIN, carrier.display.getCursorY() + TEXT_2_STEP);
      carrier.display.print("Shuffle: ");
      carrier.display.setCursor(MARGIN, carrier.display.getCursorY() + TEXT_2_STEP);
      carrier.display.print("Volume: ");
      break;
    }
  }

  if (displayHasNewState || displayHasNewData) {
    const unsigned long sinceRedraw = NOW - displayStateChange;

    PRINT("Redraw display: ");
    PRINT(sinceRedraw);
    PRINT(" - ");
    PRINT(displayHasNewState ? "state" : "data");
    PRINT(" - command state: ");
    PRINT(static_cast<int>(commandState));
    PRINTLN("");

    switch (displayState) {
    case DisplayState::Player:
      carrier.display.setTextSize(3);
      overwriteString(playButton, 110, MARGIN);
      overwriteString(playerAction, MARGIN + TEXT_3_STEP * 2, 215,
        commandState == CommandState::Success ? ST77XX_GREEN : commandState == CommandState::Error ? ST77XX_RED : ST77XX_WHITE, true);

      overwriteString(title, MARGIN, TOP_Y);
      overwriteString(artist, MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);
      overwriteString(album, MARGIN, carrier.display.getCursorY() + TEXT_3_STEP);

      carrier.display.setTextSize(2);
      overwriteString(repeat, 120, carrier.display.getCursorY() + TEXT_3_STEP + TEXT_2_STEP);
      overwriteString(shuffle, 120, carrier.display.getCursorY() + TEXT_2_STEP);
      overwriteString(sound, 120, carrier.display.getCursorY() + TEXT_2_STEP);      
      break;

    default:
      carrier.display.setTextSize(3);
      overwriteString(displayTitle, MARGIN, MARGIN);
      carrier.display.setTextSize(2);
      overwriteString(displayMessage, MARGIN, 50);
      break;
    }

    displayStateChange = NOW;
  }

  displayDataChange = 0;
  displayPrevState = enterState;
}

void buttonsMachine() {
  carrier.Buttons.update();
  if (wifiState == WifiState::Sleep) {
    if (
      buttonMachine(TOUCH0) == ButtonState::Hold ||
      buttonMachine(TOUCH1) == ButtonState::Hold ||
      buttonMachine(TOUCH2) == ButtonState::Hold ||
      buttonMachine(TOUCH3) == ButtonState::Hold ||
      buttonMachine(TOUCH4) == ButtonState::Hold
    ) {
      wifiState = WifiState::Init;
    }
  } else {
    switch (buttonMachine(TOUCH0)) {
    case ButtonState::Tap:
      sendCommand(TOUCH0, "previous", "Prev");
      break;
    case ButtonState::DoubleTap:
      sendCommand(TOUCH0, "trackseek/1", "Start");
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
      sendCommand(TOUCH1, "mute/toggle", mute[0] ? "Unmute" : "Mute");
      break;
    }

    switch (buttonMachine(TOUCH2)) {
    case ButtonState::Tap:
      sendCommand(TOUCH2, "playpause", playing[0] ? "Pause" : "Play");
      break;
    case ButtonState::DoubleTap:
      sendCommand(TOUCH2, "shuffle/toggle", shuffle ? "Shuf Off" : "Shuf On");
      break;
    case ButtonState::Hold:
      sendCommand(TOUCH2, "repeat/toggle", "Repeat");
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
      sendCommand(TOUCH3, "mute/toggle", mute[0] ? "Unmute" : "Mute");
      break;
    }

    switch (buttonMachine(TOUCH4)) {
    case ButtonState::Tap:
      sendCommand(TOUCH4, "next", "Next");
      break;
    case ButtonState::Hold:
      wifiState = WifiState::Sleep;
      break;
    }
  }
}

ButtonState buttonMachine(touchButtons btn) {
  const unsigned long lastDown = NOW - buttonPhysical[btn][0];
  const unsigned long lastUp = NOW - buttonPhysical[btn][1];

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

  const unsigned long lastTouch = buttonStateChange[btn][0];
  if (touch && !lastTouch) {
    buttonPhysical[btn][0] = 1;
    buttonPhysical[btn][1] = NOW;
  } else if (!touch && lastTouch) {
    buttonPhysical[btn][0] = 0;
    buttonPhysical[btn][2] = NOW;
  }

  return(action);
}
