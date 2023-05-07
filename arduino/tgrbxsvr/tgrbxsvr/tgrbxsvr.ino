#include "freertos/FreeRTOS.h"
#include <stdlib.h>
#include <vector>
#include "DataAccess.h"
#include "MachineController.h"
#include "NetworkController.h"
#include "Models.h"

DataAccess dataAccess;
MachineController machineController;
NetworkController networkController;

SystemSettings* systemSettings = nullptr;
Config* config = nullptr;
UserSettings* userSettings = nullptr;

Schedule* selectedSchedule = nullptr;

MachineStatus* previousStatus = nullptr;
MachineStatus* currentStatus = nullptr;

long today = 0;
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

  networkController.initNetworkConnection(*config);
  networkController.initNTP();
  currentTimestamp = networkController.getCurrentTime();
  today = currentTimestamp  / 86400L;

  selectedSchedule = dataAccess.getSelectedSchedule();
  const long actualLastFedTimestamp = dataAccess.getLastFedTimestampBefore(currentTimestamp);
  lastFedTimestamp = currentTimestamp;
  //log missed feeds betwen actualLastFedTimestamp and lastFedTimestamp
  numTimesFedToday = dataAccess.getNumFedFromTo(today, today + 1);

  machineController.initControls();

}

void loop() {
}
