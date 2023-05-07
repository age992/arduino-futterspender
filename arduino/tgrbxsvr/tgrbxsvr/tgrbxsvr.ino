#include "freertos/FreeRTOS.h"
#include <stdlib.h>
#include <vector>
#include "DataAccess.cpp"
#include "Models.cpp"

DataAccess dataAccess;

SystemSettings* systemSettings = nullptr;
Config* config = nullptr;
UserSettings* userSettings = nullptr;

Schedule* selectedSchedule = nullptr;

MachineStatus* previousStatus = nullptr;
MachineStatus* currentStatus = nullptr;

long previousTimestamp = 0;
long currentTimestamp = 0;

long lastFedTimestamp = 0;
int numTimesFedToday = 0;

double currentFeedTargetWeight = 0;

const int MAX_HISTORY_BUFFER = 100;
std::vector<long> containerScaleHistoryBuffer;
std::vector<long> plateScaleHistoryBuffer;

void setup() {
  Serial.begin(115200);

  dataAccess.init();
  
  config = dataAccess.getConfig();
  systemSettings = dataAccess.getSystemSettings();
  userSettings = dataAccess.getUserSettings();

  selectedSchedule = dataAccess.getSelectedSchedule();

  //lastFedTimestamp = dataAccess.getLastFedTimestampBefore(now);
  //numTimesFedToday = dataAccess.getNumFedFromTo(yesterday, tomorrow);
}

void loop() {
}
