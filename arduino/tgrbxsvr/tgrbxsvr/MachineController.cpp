#include "MachineController.h"
#include <Servo.h>
#include <HX711.h>

const int servoPin = 14;
const int SERVO_CLOSE_ANGLE = 0;
const int SERVO_OPEN_ANGLE = 90;
Servo servo;

const int LOADCELL_TIMES = 2;

//Container
const int LOADCELL_A_SCK_PIN = 19;
const int LOADCELL_A_DOUT_PIN = 21;
HX711 scale_A;

//Plate
const int LOADCELL_B_SCK_PIN = 5;
const int LOADCELL_B_DOUT_PIN = 18;
HX711 scale_B;

double getScaleValue(HX711 scale) {
  long val = scale.get_units(LOADCELL_TIMES) * 10;
  double scaleValue = (double)val / 10;
  return scaleValue;
}

bool MachineController::initControls() {
  Serial.println("Setting up scale A...");
  scale_A.begin(LOADCELL_A_DOUT_PIN, LOADCELL_A_SCK_PIN);

  while (!scale_A.is_ready()) {
    Serial.println(".");
    delay(2000);
  }

  Serial.println("Scale A ready.");

  Serial.println("Setting up scale B...");
  scale_B.begin(LOADCELL_B_DOUT_PIN, LOADCELL_B_SCK_PIN);

  while (!scale_B.is_ready()) {
    Serial.println(".");
    delay(2000);
  }

  Serial.println("Scale B ready.");

  Serial.println("Start servo");
  servo.attach(servoPin);
  servo.write(systemSettings->ContainerAngleClose);

  return true;
}

void MachineController::setContainerScaleCalibration(double scaleFactor, int offset) {
  scale_A.set_scale(scaleFactor);
  scale_A.set_offset(offset);
};

void MachineController::setPlateScaleCalibration(double scaleFactor, int offset) {
  scale_B.set_scale(scaleFactor);
  scale_B.set_offset(offset);
};

bool MachineController::calibrateContainerScale(double targetWeight) {
  scale_A.calibrate_scale(targetWeight, 5);
  systemSettings->ContainerScale = scale_A.get_scale();
  return dataAccess.updateSystemSettings(systemSettings);
};

bool MachineController::calibratePlateScale(double targetWeight) {
  scale_B.calibrate_scale(targetWeight, 5);
  systemSettings->PlateScale = scale_B.get_scale();
  return dataAccess.updateSystemSettings(systemSettings);
};

bool MachineController::tareContainerScale() {
  scale_A.tare(LOADCELL_TIMES);
  systemSettings->ContainerOffset = scale_A.get_offset();
  return dataAccess.updateSystemSettings(systemSettings);
};

bool MachineController::tarePlateScale() {
  scale_B.tare(LOADCELL_TIMES);
  systemSettings->PlateOffset = scale_B.get_offset();
  return dataAccess.updateSystemSettings(systemSettings);
};

bool MachineController::tarePlateScaleWithPlate() {
  const double currentWeight = getScaleValue(scale_B);
  userSettings->PlateTAR = currentWeight;
  return dataAccess.updateUserSettings(userSettings);
};

double MachineController::getContainerLoad() {
  return getScaleValue(scale_A);
};

double MachineController::getPlateLoad() {
  return getScaleValue(scale_B) - userSettings->PlateTAR;
};

void MachineController::openContainer() {
  servo.write(systemSettings->ContainerAngleClose + systemSettings->ContainerAngleOpen);
};

void MachineController::closeContainer() {
  servo.write(systemSettings->ContainerAngleClose);
};