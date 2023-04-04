#include <SPI.h>
#include <WiFiNINA.h>
#include <Arduino_MKRIoTCarrier.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include "Secrets.h"
#include "Sonos.h"

MKRIoTCarrier carrier;

const unsigned long WIFI_POLL = 5000L;
const int HOLD = 600;
const int TAP = 30;
const int DOUBLE_TAP = 100;

bool wifiBlink = true;
bool wifiInitialConnect = true; 
int wifiStatus = WL_IDLE_STATUS;
unsigned long lastCheckWifiStatus = 0;

String room = SONOS_ROOM;
char serverAddress[] = SONOS_SERVER;
int port = SONOS_PORT;
WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

unsigned long redrawDisplay = 1;
int volume = 0;
bool mute = false;
String artist = "";
String title = "";
String album = "";
String repeat = "none";
bool shuffle = false;
bool playing = false;

uint32_t red = carrier.leds.Color(255, 0, 0);
uint32_t blue = carrier.leds.Color(0, 0, 255);
uint32_t green = carrier.leds.Color(0, 255, 0);
uint32_t off = carrier.leds.Color(0, 0, 0);

enum class ButtonAction { None, Tap, Hold, TapHold, DoubleTap };
enum class ButtonState { Up, Down, Tap, Hold, DoubleTap, Discard };
ButtonState buttonState [5] = {
  ButtonState::Up,
  ButtonState::Up,
  ButtonState::Up,
  ButtonState::Up,
  ButtonState::Up
};

unsigned long buttonPhysical [5][3] = {
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0},
  {0, 0, 0}
};

void setup() {
  Serial.begin(9600);
  delay(2000);

  carrier.withCase();
  carrier.Buttons.updateConfig(200);
  carrier.leds.setBrightness(1);
  if (!carrier.begin()) {
    Serial.print("Error: no carrier");
    while (true);
  }
  
  // Indicate setup has begun
  carrier.display.fillScreen(ST77XX_WHITE);

  // Deal with unrecoverable error states that require pluggin stuff in
  // or flashing firmware
  if (WiFi.status() == WL_NO_MODULE) {
    displayError("Error: WL_NO_MODULE");
    while (true);
  }

  if (WiFi.status() == WL_NO_SHIELD) {
    displayError("Error: WL_NO_SHIELD");
    while (true);
  }

  if (WiFi.firmwareVersion() < WIFI_FIRMWARE_LATEST_VERSION) {
    displayError("Error: upgrade WiFi firmware");
    while (true);
  }

  // Start wifi connection process. Set timeout to 0 so begin returns
  // right away and the loop deals with polling for connection status
  WiFi.setTimeout(0);
  wifiStatus = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  #ifdef DEBUG
    Serial.print("Staring wifi connection: ");
    Serial.println(wifiStatus);
  #endif
}

void loop() {
  if (pollWiFi()) {
    return;
  }

  if (checkWiFi()) {
    return;
  }

  if (connectEvents()) {
    return;
  }

  displayControls();
  readEvents();

  carrier.Buttons.update();

  ButtonAction button0 = trackButton(TOUCH0);
  if (button0 != ButtonAction::None) {
    const ButtonAction action = button0;
    const int button = TOUCH0;
    switch (action) {
    case ButtonAction::Tap:
      sendCommand(button, "previous", "Prev");
      break;
    case ButtonAction::DoubleTap:
      sendCommand(button, "trackseek/1", "Start");
      break;
    case ButtonAction::TapHold:
      // sendCommand(button, "trackseek/1", "Start");
      break;
    }
  }

  ButtonAction button1 = trackButton(TOUCH1);
  if (button1 != ButtonAction::None) {
    const ButtonAction action = button1;
    const int button = TOUCH1;
    switch (action) {
    case ButtonAction::Tap:
      sendCommand(button, "volume/-1", "Vol -1");
      break;
    case ButtonAction::DoubleTap:
      sendCommand(button, "volume/-5", "Vol -5");
      break;
    case ButtonAction::Hold:
      sendCommand(button, "mute", "Mute");
      break;
    }
  }

  ButtonAction button2 = trackButton(TOUCH2);
  if (button2 != ButtonAction::None) {
    const ButtonAction action = button2;
    const int button = TOUCH2;
    switch (action) {
    case ButtonAction::Tap:
      sendCommand(button, "playpause", "Toggle");
      break;
    case ButtonAction::DoubleTap:
      sendCommand(button, "shuffle/toggle", "Shuffle");
      break;
    case ButtonAction::Hold:
      sendCommand(button, "repeat/toggle", "Repeat");
      break;
    }
  }

  ButtonAction button3 = trackButton(TOUCH3);
  if (button3 != ButtonAction::None) {
    const ButtonAction action = button3;
    const int button = TOUCH3;
    switch (action) {
    case ButtonAction::Tap:
      sendCommand(button, "volume/+1", "Vol +1");
      break;
    case ButtonAction::DoubleTap:
      sendCommand(button, "volume/+5", "Vol +5");
      break;
    case ButtonAction::Hold:
      sendCommand(button, "unmute", "Unmute");
      break;
    }
  }

  ButtonAction button4 = trackButton(TOUCH4);
  if (button4 != ButtonAction::None) {
    const ButtonAction action = button4;
    const int button = TOUCH4;
    switch (action) {
    case ButtonAction::Tap:
      sendCommand(button, "next", "Next");
      break;
    case ButtonAction::TapHold:
      sendCommand(button, "next", "Next");
      break;
    }
  }
}

void displayError(String error) {
  #ifdef DEBUG
    Serial.println(error);
  #endif
  
  carrier.display.setTextWrap(true);
  carrier.display.fillScreen(ST77XX_RED);
  carrier.display.setTextColor(ST77XX_WHITE);
  carrier.display.setCursor(20, 20);
  carrier.display.setTextSize(2);
  carrier.display.print(error);

  carrier.leds.fill(red, 0, 5);
  carrier.leds.show();
}

bool pollWiFi() {
  if (wifiStatus != WL_CONNECTED) {
    #ifdef DEBUG
      Serial.print("Waiting for wifi connection: ");
      Serial.println(wifiStatus);
    #endif

    carrier.leds.fill(wifiBlink ? blue : off, 0, 5);
    carrier.leds.show();

    delay(1000);
    
    wifiBlink = !wifiBlink;
    wifiStatus = WiFi.status();
    
    return true;
  }

  if (wifiInitialConnect) {
    #ifdef DEBUG
      Serial.print("WiFi connected: ");
      Serial.print(WiFi.SSID());
      Serial.print(" - Signal: ");
      Serial.println(WiFi.RSSI());
    #endif

    wifiInitialConnect = false;
    lastCheckWifiStatus = millis();
    
    carrier.leds.clear();
    carrier.leds.show();
  }

  return false;
}

bool checkWiFi() {
  if ((millis() - lastCheckWifiStatus) > WIFI_POLL) {
    int wifiCurrentStatus = WiFi.status();
    lastCheckWifiStatus = millis();
    
    if (wifiCurrentStatus != WL_CONNECTED) {
      wifiStatus = wifiCurrentStatus;
      wifiInitialConnect = true;
      wifiBlink = true;

      #ifdef DEBUG
        Serial.print("WiFi connection lost: ");
        Serial.println(wifiCurrentStatus);
      #endif

      return true;
    }
  }

  return false;
}

bool connectEvents() {
  if (!client.connected()) {
    client.get("/" + room + "/events");
    int statusCode = client.responseStatusCode();

    if (statusCode != 200) {
      #ifdef DEBUG
        Serial.print("Could not connect to /events: ");
        Serial.println(statusCode);
      #endif
      displayError("Error: could not connect to server");
      delay(5000);
      return true;
    }

    #ifdef DEBUG
      Serial.println("Connected to /events: ");
    #endif

    client.skipResponseHeaders();

    // TODO: get initial list of zones so a command can
    // go to the next zone
  }

  return false;
}

void readEvents() {
  String response = "";
  while (client.available()) {
    const char c = client.read();
    if (c == '\n') {
      response.remove(0, 6);
      if (response.length()) {
        setPlayerState(response);
      }
      response = "";
    } else {
      response += c;
    }
  }
}

void setPlayerState(String rawState) {
  #ifdef DEBUG
    Serial.println("raw state");
    Serial.println(rawState);
  #endif

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, rawState);
        
  if (error) {
    #ifdef DEBUG
      Serial.print("json error: ");
      Serial.println(error.f_str());
      Serial.println("------------------");
    #endif
  } else {
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
      redrawDisplay = millis();
    } else if (type == "volume-change") {
      volume = doc["data"]["newVolume"];
      redrawDisplay = millis();
    } else if (type == "mute-change") {
      mute = doc["data"]["newMute"];
      redrawDisplay = millis();
    }
    #ifdef DEBUG
      Serial.print("type:");
      Serial.print(doc["type"].as<String>());
      Serial.print(" playing:");
      Serial.print(playing);
      Serial.print(" volume:");
      Serial.print(volume);
      Serial.print(" mute:");
      Serial.print(mute);
      Serial.print(" artist:");
      Serial.print(artist);
      Serial.print(" title:");
      Serial.print(title);
      Serial.print(" album:");
      Serial.print(album);
      Serial.print(" repeat:");
      Serial.print(repeat);
      Serial.print(" shuffle:");
      Serial.println(shuffle);
      Serial.println("------------------");
    #endif
  }
}

void displayControls() {
  if (redrawDisplay && (millis() - redrawDisplay) > 100) {
    redrawDisplay = 0;

    carrier.leds.clear();
    carrier.leds.show();
    
    carrier.display.fillScreen(ST77XX_BLACK);
    carrier.display.setTextColor(ST77XX_WHITE);
    carrier.display.setTextSize(3);
    carrier.display.setTextWrap(false);

    carrier.display.setCursor(90, 5);
    carrier.display.print(playing ? "||" : "|>");

    carrier.display.setCursor(5, 5);
    carrier.display.print("-");

    carrier.display.setCursor(215, 5);
    carrier.display.print("+");

    carrier.display.setCursor(5, 215);
    carrier.display.print("<");

    carrier.display.setCursor(215, 215);
    carrier.display.print(">");

    carrier.display.setTextSize(3);

    carrier.display.setCursor(5, 38);
    carrier.display.print(title);

    carrier.display.setCursor(5, 68);
    carrier.display.print(artist);

    carrier.display.setCursor(5, 98);
    carrier.display.print(album);

    carrier.display.setTextSize(2);

    carrier.display.setCursor(5, 128);
    carrier.display.print("Repeat: " + repeat);

    carrier.display.setCursor(5, 148);
    carrier.display.print("Shuffle: " + String(shuffle ? "on" : "off"));

    carrier.display.setCursor(5, 168);
    carrier.display.print("Volume: " + String(volume));
    
    carrier.display.setCursor(5, 188);
    carrier.display.print("Mute: " + String(mute ? "on" : "off"));
  }
}

void sendCommand(int button, String action, String msg) {
  #ifdef DEBUG
    Serial.print("Button: ");
    Serial.print(button);
    Serial.print(" - ");
    Serial.println(action);
  #endif

  carrier.Buzzer.beep(800, 20);
  
  carrier.display.setTextSize(5);
  carrier.display.setTextWrap(true);
  carrier.display.setCursor(5, 5);
  carrier.display.fillScreen(ST77XX_BLACK);
  carrier.display.print(msg);

  carrier.leds.fill(blue, button, 1);
  carrier.leds.show();

  client.stop();
  client.get("/" + room + "/" + action);
  int statusCode = client.responseStatusCode();
  client.stop();

  #ifdef DEBUG
    Serial.print("Status code: ");
    Serial.println(statusCode);
  #endif

  if (statusCode != 200) {
    carrier.Buzzer.beep(200, 40);

    carrier.display.setCursor(5, 80);
    carrier.display.setTextColor(ST77XX_RED);
    carrier.display.print("Error: /" + action);
  }

  carrier.leds.clear();
  carrier.leds.show();
}

// TODO: move button stuff to a class in a different file
ButtonAction trackButton(touchButtons btn) {
  const unsigned long now = millis();
  const unsigned long lastDown = now - buttonPhysical[btn][1];
  const unsigned long lastUp = now - buttonPhysical[btn][2];
  
  const bool touch = carrier.Buttons.getTouch(btn);

  // The buttons are capacitive and even at a non-sensitive config level
  // there is noise when a finger hovers close to the touchpad. This results
  // in state quickly osscilating between up/down. I tested by quickly
  // tapping and could never get it under 30ms, so anything under 30ms is
  // deemed noise which means we just reset the button state to up.
  const bool discard = touch ? lastUp < TAP : lastDown < TAP;

  const ButtonState state = discard ? ButtonState::Discard : buttonState[btn];
  ButtonAction action = ButtonAction::None;

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
        action = ButtonAction::Hold;
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
        action = ButtonAction::Tap;
      }
    } else {
      buttonState[btn] = ButtonState::DoubleTap;
    }
    break;

  case ButtonState::DoubleTap:
    if (touch) {
      if (lastDown > HOLD) {
        buttonState[btn] = ButtonState::Hold;
        action = ButtonAction::TapHold;
      }
    } else {
      if (lastUp > DOUBLE_TAP) {
        buttonState[btn] = ButtonState::Up;
        action = ButtonAction::DoubleTap;
      }
    }
    break;

  }

  const unsigned long lastTouch = buttonPhysical[btn][0];
  if (touch && !lastTouch) {
    buttonPhysical[btn][0] = 1;
    buttonPhysical[btn][1] = now;
  } else if (!touch && lastTouch) {
    buttonPhysical[btn][0] = 0;
    buttonPhysical[btn][2] = now;
  }

  return(action);
}
