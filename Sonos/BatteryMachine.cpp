
#include "BatteryMachine.h"

#include <BQ24195.h>

#include "StateMachine.h"
#include "Utils.h"

#ifndef DEBUG_BATTERY
#define DEBUG_BATTERY false
#endif

BatteryMachine::BatteryMachine()
    : StateMachine("BATTERY", stateStrings, DEBUG_BATTERY) {
}

void BatteryMachine::reset() {
  analogReference(AR_DEFAULT);
  analogReadResolution(12);

  PMIC.begin();
  PMIC.enableBoostMode();

  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);
  PMIC.setChargeVoltage(batteryFullVoltage);
}

void BatteryMachine::setBatteryLevel() {
  float rawADC = analogRead(ADC_BATTERY);
  float voltADC = rawADC * (batteryEmptyVoltage / 4095.0);
  float voltBat = voltADC * (maxSourceVoltage / batteryEmptyVoltage);
  batteryLevel = (voltBat - batteryEmptyVoltage) * (100) / (batteryFullVoltage - batteryEmptyVoltage);
  this->setState(batteryLevel < 0 ? Off : On);
}

void BatteryMachine::ready() {
  this->setBatteryLevel();
}

void BatteryMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Update:
      this->setBatteryLevel();
      break;
  }
}

void BatteryMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case On:
      if (sinceChange > BATTERY_CHECK) {
        this->setState(Update);
      }
      break;
  }
}
