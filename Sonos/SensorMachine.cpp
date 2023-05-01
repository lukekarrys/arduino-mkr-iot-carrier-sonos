
#include "SensorMachine.h"

#include <Arduino.h>
#include <BQ24195.h>

#include "Carrier.h"
#include "Consts.h"
#include "StateMachine.h"

#ifndef DEBUG_SENSOR
#define DEBUG_SENSOR false
#endif

SensorMachine::SensorMachine()
    : StateMachine("SENSOR", stateStrings, DEBUG_SENSOR) {
}

void SensorMachine::reset() {
  analogReference(AR_DEFAULT);
  analogReadResolution(12);

  PMIC.begin();
  PMIC.enableBoostMode();

  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);
  PMIC.setChargeVoltage(batteryFullVoltage);
}

void SensorMachine::setBattery() {
  float rawADC = analogRead(ADC_BATTERY);
  float voltADC = rawADC * (batteryEmptyVoltage / 4095.0);
  float voltBat = voltADC * (maxSourceVoltage / batteryEmptyVoltage);
  int batteryPct = (voltBat - batteryEmptyVoltage) * (100) / (batteryFullVoltage - batteryEmptyVoltage);
  battery = batteryPct < 0 ? -1 : batteryPct;
}

void SensorMachine::setTemperature() {
  float celsius = (carrier.Env.readTemperature() * (9 / 5)) + 32;
  temperature = (int)celsius;
}

void SensorMachine::setHumidity() {
  humidity = (int)carrier.Env.readHumidity();
}

void SensorMachine::ready() {
  this->setBattery();
  this->setTemperature();
  this->setHumidity();
  this->setState(On);
}

void SensorMachine::enter(int state, int exitState, unsigned long sinceChange) {
  switch (state) {
    case Update:
      this->ready();
      break;
  }
}

void SensorMachine::poll(int state, unsigned long sinceChange) {
  switch (state) {
    case On:
      if (sinceChange > SENSOR_CHECK) {
        this->setState(Update);
      }
      break;
  }
}
