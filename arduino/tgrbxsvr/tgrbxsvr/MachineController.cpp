#include "MachineController.h"
#include <Servo.h>
#include <HX711.h>

const int servoPin = 32;
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

bool MachineController::initControls(){
  Serial.println("Setting up scale A...");
  scale_A.begin(LOADCELL_A_DOUT_PIN, LOADCELL_A_SCK_PIN);

  while(!scale_A.is_ready()){
    Serial.println(".");
    delay(2000);
  }

  Serial.println("Scale A ready.");

  Serial.println("Setting up scale B...");
  scale_B.begin(LOADCELL_B_DOUT_PIN, LOADCELL_B_SCK_PIN);

  while(!scale_B.is_ready()){
    Serial.println(".");
    delay(2000);
  }

  Serial.println("Scale B ready.");

  Serial.println("Start servo");
  servo.attach(servoPin);

  return true;
}

double getScaleValue(HX711 scale){
  long val = scale.get_units(LOADCELL_TIMES) / 100;
  double scaleValue =  (double) val / 10;
  Serial.println(scaleValue);
  return scaleValue;
}