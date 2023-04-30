#ifndef SENSOR_MACHINE_H
#define SENSOR_MACHINE_H

#include <Arduino.h>

#include "StateMachine.h"

#define R1_VALUE 330000
#define R2_VALUE 1000000
#define BATTERY_EMPTY 3.3
#define BATTERY_FULL 4.2

#define BATTERY_CAPACITY 1.800

class SensorMachine : public StateMachine {
 public:
  SensorMachine();

  enum States {
    Off,
    On,
    Update,
  };
  String stateStrings[3] = {
      "Off",
      "On",
      "Update",
  };

  static States states;

  void reset();
  void ready();

  int battery;
  int temperature;
  int humidity;

 protected:
  void poll(int state, unsigned long since) override;
  void enter(int enterState, int exitState, unsigned long since) override;

 private:
  void setBattery();
  void setTemperature();
  void setHumidity();

  const int maxSourceVoltage = (BATTERY_EMPTY * (R1_VALUE + R2_VALUE)) / R2_VALUE;
  const int R1 = R1_VALUE;
  const int R2 = R2_VALUE;
  const float batteryFullVoltage = BATTERY_FULL;
  const float batteryEmptyVoltage = BATTERY_EMPTY;
  const float batteryCapacity = BATTERY_CAPACITY;
};

#endif