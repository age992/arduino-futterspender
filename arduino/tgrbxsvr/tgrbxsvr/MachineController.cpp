#include "MachineController.h"
#include <Servo.h>
#include <HX711.h>

const int servoPin = 32;
const int SERVO_CLOSE_ANGLE = 0;
const int SERVO_OPEN_ANGLE = 90;
Servo servo;

const int LOADCELL_TIMES = 1;

//Container
const int LOADCELL_A_SCK_PIN = 27;
const int LOADCELL_A_DOUT_PIN = 26;
HX711 scale_A;

//Plate
const int LOADCELL_B_SCK_PIN = 25;
const int LOADCELL_B_DOUT_PIN = 33;
HX711 scale_B;

double getScaleValue(HX711 scale) {
  long val = scale.get_units(LOADCELL_TIMES) * 10;
  //Serial.print("raw val ");
  //Serial.println(val);
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
  servo.write(SERVO_CLOSE_ANGLE);

  return true;
}

void MachineController::calibrateContainerScale(double scaleFactor, int offset){
  scale_A.set_scale(scaleFactor);
  scale_A.set_offset(offset);
};

void MachineController::calibratePlateScale(double scaleFactor, int offset){
  scale_B.set_scale(scaleFactor);
  scale_B.set_offset(offset);
};

double MachineController::getContainerLoad(){
  return getScaleValue(scale_A);
};

double MachineController::getPlateLoad(){
  return getScaleValue(scale_B);
};

void MachineController::openContainer(){
  servo.write(SERVO_OPEN_ANGLE);
};

void MachineController::closeContainer(){
  servo.write(SERVO_CLOSE_ANGLE);
};