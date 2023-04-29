#include "Carrier.h"

#include <Arduino.h>
#include <Arduino_MKRIoTCarrier.h>

MKRIoTCarrier carrier;

uint32_t LedOff = carrier.leds.Color(0, 0, 0);
uint32_t LedBlue = carrier.leds.Color(0, 0, 255);
uint32_t LedYellow = carrier.leds.Color(255, 255, 0);
uint32_t LedRed = carrier.leds.Color(255, 0, 0);
