#include <Arduino.h>

#include "AppMachine.h"
#include "Carrier.h"
#include "Secrets.h"
#include "SonosConfig.h"

AppMachine appMachine = AppMachine(WIFI_SSID, WIFI_PASSWORD, SONOS_SERVER, SONOS_PORT, SONOS_ROOM);

void setup() {
  Serial.begin(9600);
  delay(2000);

  carrier.withCase();
  carrier.Buttons.updateConfig(200);
  carrier.leds.setBrightness(1);

  if (!carrier.begin()) {
    Serial.print("Error: no carrier");
    while (true)
      ;
  }

  appMachine.reset();
  appMachine.ready();
}

void loop() {
  appMachine.run();
}
