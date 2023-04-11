#include <BQ24195.h>

float rawADC;
float voltADC;
float voltBat;
int R1 =  330000;
int R2 = 1000000;
int max_Source_voltage;
float batteryFullVoltage = 4.2;
float batteryEmptyVoltage = 3.3;
float batteryCapacity = 1.800;

void setupBattery() {
  analogReference(AR_DEFAULT); 
  analogReadResolution(12);

  PMIC.begin();
  PMIC.enableBoostMode();

  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);
  PMIC.setChargeVoltage(batteryFullVoltage);

  max_Source_voltage = (3.3 * (R1 + R2))/R2;
}

int readBattery() {
  rawADC = analogRead(ADC_BATTERY);
  voltADC = rawADC * (3.3/4095.0);
  voltBat = voltADC * (max_Source_voltage/3.3);
  int new_batt = (voltBat - batteryEmptyVoltage) * (100) / (batteryFullVoltage - batteryEmptyVoltage);
  return(new_batt);
}
