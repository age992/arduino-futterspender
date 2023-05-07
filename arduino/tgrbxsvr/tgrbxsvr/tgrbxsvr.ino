#include "freertos/FreeRTOS.h"
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include "DataAccess.h"
#include "MachineController.h"
#include "NetworkController.h"
#include "Models.h"

const double WEIGHT_D_THRESHOLD = 1;  //in gramm/second
const double SAFETY_WEIGHT_GAP = 1; //in gramm, start closing a bit earlier to include container closing time
const double CONTAINER_EMPTY_THRESHOLD = 1;
const double NO_CONTAINER_THRESHOLD = -10; //negative, since empty container = tar weight
const double PLATE_EMPTY_THRESHOLD = 1;

const double LOOP_FREQ_NORMAL = 0.5;
const double LOOP_FREQ_FAST = 3;
const double COOLDOWN_TIME = 5;  //seconds

double CURRENT_LOOP_FREQ = LOOP_FREQ_NORMAL;
long noStatusChangeTimestamp = 0;

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
std::vector<ScaleData> containerScaleHistoryBuffer;
std::vector<ScaleData> plateScaleHistoryBuffer;

void setup() {
  Serial.begin(115200);

  dataAccess.init();
  config = dataAccess.getConfig();
  systemSettings = dataAccess.getSystemSettings();
  userSettings = dataAccess.getUserSettings();

  networkController.initNetworkConnection(*config);
  networkController.initNTP();
  currentTimestamp = networkController.getCurrentTime();
  today = getDay(currentTimestamp);

  selectedSchedule = dataAccess.getSelectedSchedule();
  //const long actualLastFedTimestamp = dataAccess.getLastFedTimestampBefore(currentTimestamp);
  lastFedTimestamp = currentTimestamp;
  //log missed feeds betwen actualLastFedTimestamp and lastFedTimestamp
  numTimesFedToday = dataAccess.getNumFedFromTo(today, today + 1);

  machineController.initControls();
  machineController.calibrateContainerScale(systemSettings->ContainerScale, systemSettings->ContainerOffset);
  machineController.calibratePlateScale(systemSettings->PlateScale, systemSettings->PlateOffset);
  networkController.initWebserver();

  previousTimestamp = currentTimestamp;
  MachineStatus status;
  status.ContainerLoad = machineController.getContainerLoad();
  status.PlateLoad = machineController.getPlateLoad();
  status.Open = false;
  status.MotorOperation = true;
  status.SDCardConnection = true;
  status.WiFiConnection = true;
  *previousStatus = status;
}

void loop() {
  currentTimestamp = networkController.getCurrentTime();
  *currentStatus = getUpdatedStatus();

  if (feedPending()) {
    if(currentStatus->ContainerLoad > CONTAINER_EMPTY_THRESHOLD){
      if(!currentStatus->Open){
        //start feeding
        currentFeedTargetWeight = min(userSettings->PlateFilling, currentStatus->PlateLoad + currentStatus->ContainerLoad) - SAFETY_WEIGHT_GAP;
        openContainer();
      }else if(currentStatus->PlateLoad >= currentFeedTargetWeight || currentStatus->ContainerLoad == 0){
        //finished feeding
        lastFedTimestamp = currentTimestamp;
        currentFeedTargetWeight = 0;
        numTimesFedToday++;
        if(selectedSchedule->Mode == MaxTimes && numTimesFedToday == selectedSchedule->MaxTimes){
          //set wait timestamp tomorrow
        }
        closeContainer();
        //log history: eventtype = feed, currentTimestamp
      }else if(!currentStatus->MotorOperation){
        //abort feeding
        lastFedTimestamp = currentTimestamp;
        currentFeedTargetWeight = 0;
        closeContainer();
        //log history: eventtype = missedFeed, currentTimestamp
      }
    }
  }

  if(userSettings->Notifications.ContainerEmpty.Active
  && previousStatus->ContainerLoad > CONTAINER_EMPTY_THRESHOLD
  && currentStatus->ContainerLoad <= CONTAINER_EMPTY_THRESHOLD 
  && currentStatus->ContainerLoad > NO_CONTAINER_THRESHOLD){
    //Task -> chech if condition still true later, if yes send notification
  }

  if(dayChanged()){
    numTimesFedToday = 0;
    if(userSettings->Notifications.DidNotEatInADay.Active){
      //send notification
    }
  }

  const bool significantChange = weightDifferenceSignificant();

  if (significantChange || currentStatus->Open) {
    CURRENT_LOOP_FREQ = LOOP_FREQ_FAST;
    noStatusChangeTimestamp = 0;
    if(significantChange){

    }
  } else if (noStatusChangeTimestamp == 0) {
    noStatusChangeTimestamp = currentTimestamp;
  } else if (currentTimestamp - noStatusChangeTimestamp > COOLDOWN_TIME) {
    CURRENT_LOOP_FREQ = LOOP_FREQ_NORMAL;
  }
  
  networkController.broadcast("status data...");
  networkController.broadcast("history data...");

  previousTimestamp = currentTimestamp;
  previousStatus = currentStatus;
  delay(1 / CURRENT_LOOP_FREQ);
}

void openContainer(){
  machineController.openContainer();
  currentStatus->Open = true;
  //check task
}

void closeContainer(){
  machineController.closeContainer();
  currentStatus->Open = false;
  //check task
}

MachineStatus getUpdatedStatus() {
  MachineStatus status = *previousStatus;
  status.ContainerLoad = machineController.getContainerLoad();
  status.PlateLoad = machineController.getPlateLoad();
  return status;
}

bool weightDifferenceSignificant() {
  const double time_D = currentTimestamp - previousTimestamp;
  const double container_D = (currentStatus->ContainerLoad - previousStatus->ContainerLoad) / time_D;
  const double plate_D = (currentStatus->PlateLoad - previousStatus->PlateLoad) / time_D;
  return abs(container_D) > WEIGHT_D_THRESHOLD || abs(plate_D) > WEIGHT_D_THRESHOLD;
}

bool feedPending() {
  if (!selectedSchedule->Active) {
    return false;
  }

  bool pending = false;

  if (selectedSchedule->Mode == FixedDaytime) {
    std::vector<long> dayTimesSmallerNow;
    std::copy_if(selectedSchedule->Daytimes.begin(), selectedSchedule->Daytimes.end(), std::back_inserter(dayTimesSmallerNow), [](long n) {
      return n <= currentTimestamp;
    });
    auto max_it = std::max_element(dayTimesSmallerNow.begin(), dayTimesSmallerNow.end());
    pending = max_it == dayTimesSmallerNow.end() ? false : *max_it > lastFedTimestamp;
  } else if (selectedSchedule->Mode == MaxTimes) {
    pending = currentStatus->PlateLoad <= PLATE_EMPTY_THRESHOLD && numTimesFedToday < selectedSchedule->MaxTimes;
  }

  return pending;
}

bool dayChanged(){
  return getDay(currentTimestamp) > getDay(previousTimestamp);
}

long getDay(long timestamp){
  return timestamp / 86400L;
}
