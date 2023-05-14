#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <freertos/projdefs.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <ArduinoJson.h>
#include "JsonHelper.h"
#include "DataAccess.h"
#include "MachineController.h"
#include "NetworkController.h"
#include "Models.h"

const double WEIGHT_D_THRESHOLD = 1;  //in gramm/second
const double SAFETY_WEIGHT_GAP = 1;   //in gramm, start closing a bit earlier to include container closing time
const double CONTAINER_EMPTY_THRESHOLD = 1;
const double NO_CONTAINER_THRESHOLD = -10;  //negative, since empty container = tar weight
const double PLATE_EMPTY_THRESHOLD = 1;

const double LOOP_FREQ_NORMAL = 0.5;
const double LOOP_FREQ_FAST = 3;
const double COOLDOWN_TIME = 5;  //seconds

double CURRENT_LOOP_FREQ = LOOP_FREQ_NORMAL;
long noStatusChangeTimestamp = 0;

Schedule* selectedSchedule = nullptr;
SystemSettings* systemSettings = nullptr;
UserSettings* userSettings = nullptr;
Config* config = nullptr;

MachineStatus* previousStatus = nullptr;
MachineStatus* currentStatus = nullptr;

DataAccess dataAccess;
MachineController machineController;
NetworkController networkController;

long previousTimestamp = 0; //daytime in ms
long currentTimestamp = 0; //daytime in ms

long lastFedTimestamp = 0; //unix in s
int numTimesFedToday = 0;

double currentFeedTargetWeight = 0;

const int MAX_HISTORY_BUFFER = 100;
std::vector<ScaleData> containerScaleHistoryBuffer;
std::vector<ScaleData> plateScaleHistoryBuffer;

Schedule* historySchedule = nullptr;
std::vector<Event> eventHistoryBuffer;

const int MOTOR_CHECK_WAIT = 2;  //seconds
bool motorCheckRunning = false;
TaskHandle_t motorOperationCheckHandle;
MotorCheckParams motorCheckParams;

void motorOperationCheckTask(void* pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(MOTOR_CHECK_WAIT * 1000));

  const double container_D = currentStatus->ContainerLoad - motorCheckParams.ContainerLoad;

  if (motorCheckParams.Open) {
    if (container_D / (double)MOTOR_CHECK_WAIT > -1 * WEIGHT_D_THRESHOLD) {
      currentStatus->MotorOperation = false;
    } else {
      currentStatus->MotorOperation = true;
    }
  } else {
    if (container_D <= -1 * WEIGHT_D_THRESHOLD) {
      currentStatus->MotorOperation = false;
    } else {
      currentStatus->MotorOperation = true;
    }
  }

  vTaskDelete(NULL);
}

void setSchedule(Schedule* newSchedule) {
  delete selectedSchedule;
  selectedSchedule = newSchedule;
  historySchedule = new Schedule(*selectedSchedule);
  historySchedule->CreatedOn = getUnixTimestamp(currentTimestamp);
  dataAccess.logScheduleHistory(historySchedule);
}

void setup() {
  Serial.begin(115200);

  dataAccess.init();
  config = dataAccess.getConfig();
  Serial.println("Loaded Config");
  systemSettings = dataAccess.getSystemSettings();
  Serial.println("Loaded SystemSettings");
  userSettings = dataAccess.getUserSettings();
  Serial.println("Loaded UserSettings");

  networkController.initNetworkConnection(config);
  networkController.initNTP();
  currentTimestamp = networkController.getCurrentDaytime();
  //Serial.print("CurrentTimestamp: ");
  //Serial.println(std::to_string(currentTimestamp).c_str());
  //Serial.print("Today: ");
  //Serial.println(std::to_string(networkController.getToday()).c_str());

  selectedSchedule = dataAccess.getSelectedSchedule();
  Serial.print("Selected Schedule: ");
  if (selectedSchedule != nullptr) {
    Serial.println(selectedSchedule->ID);
    Serial.println(selectedSchedule->Name);
  } else {
    Serial.println(" nullptr");
  }
  //const long actualLastFedTimestamp = dataAccess.getLastFedTimestampBefore(currentTimestamp);
  lastFedTimestamp = getUnixTimestamp(currentTimestamp);
  //log missed feeds betwen actualLastFedTimestamp and lastFedTimestamp
  numTimesFedToday = dataAccess.getNumFedFromTo(networkController.getToday(), networkController.getToday() + 1);

  machineController.initControls();
  machineController.calibrateContainerScale(systemSettings->ContainerScale, systemSettings->ContainerOffset);
  machineController.calibratePlateScale(systemSettings->PlateScale, systemSettings->PlateOffset);
  networkController.initWebserver();

  previousTimestamp = currentTimestamp;
  MachineStatus* status = new MachineStatus();
  status->ContainerLoad = machineController.getContainerLoad();
  status->PlateLoad = machineController.getPlateLoad();
  status->Open = false;
  status->MotorOperation = true;
  status->SDCardConnection = true;
  status->WiFiConnection = true;
  status->AutomaticFeeding = false;
  status->ManualFeeding = false;
  previousStatus = status;
}

void printRam() {
  size_t totalHeap = ESP.getHeapSize();

  // Get the free heap size
  size_t freeHeap = ESP.getFreeHeap();

  // Calculate the used heap size
  size_t usedHeap = totalHeap - freeHeap;

  // Print RAM usage information
  Serial.print("Total Heap: ");
  Serial.print(totalHeap);
  Serial.print(" bytes\tFree Heap: ");
  Serial.print(freeHeap);
  Serial.print(" bytes\tUsed Heap: ");
  Serial.print(usedHeap);
  Serial.println(" bytes");
}

void loop() {
  //printRam();
  //Serial.print("loop ");
  //Serial.println(millis());
  currentTimestamp = networkController.getCurrentDaytime();
  currentStatus = getUpdatedStatus();

  if (feedPending()) {
    if (currentStatus->ContainerLoad > CONTAINER_EMPTY_THRESHOLD) {
      if(!selectedSchedule->Active || currentStatus->ManualFeeding){
        //skip feed
        lastFedTimestamp = getUnixTimestamp(currentTimestamp);
        Event skippedFeed;
        skippedFeed.CreatedOn = lastFedTimestamp;
        skippedFeed.Type = SkippedFeed;
        dataAccess.logEventHistory(skippedFeed);
        eventHistoryBuffer.push_back(skippedFeed);
        //log
      }else if (!currentStatus->Open) {
        //start feeding
        Serial.println("Start feeding!");
        currentFeedTargetWeight = min(userSettings->PlateFilling, currentStatus->PlateLoad + currentStatus->ContainerLoad) - SAFETY_WEIGHT_GAP;
        openContainer();
        currentStatus->AutomaticFeeding = true;
      } else if (currentStatus->PlateLoad >= currentFeedTargetWeight || currentStatus->ContainerLoad <= CONTAINER_EMPTY_THRESHOLD) {
        //finished feeding
        Serial.println("Finished feeding");
        lastFedTimestamp = getUnixTimestamp(currentTimestamp);
        currentFeedTargetWeight = 0;
        numTimesFedToday++;
        if (selectedSchedule != nullptr && selectedSchedule->Mode == MaxTimes && numTimesFedToday == selectedSchedule->MaxTimes) {
          //set wait timestamp tomorrow
        }
        closeContainer();
        currentStatus->AutomaticFeeding = false;
        Event feed;
        feed.CreatedOn = lastFedTimestamp;
        feed.Type = Feed;
        dataAccess.logEventHistory(feed);
        eventHistoryBuffer.push_back(feed);
      } else if (!currentStatus->MotorOperation) {
        //abort feeding
        Serial.println("Abort feeding because Motor fail");
        lastFedTimestamp = getUnixTimestamp(currentTimestamp);
        currentFeedTargetWeight = 0;
        Serial.print("Current Feed Targetweight: ");
        Serial.println(currentFeedTargetWeight);
        closeContainer();
        currentStatus->AutomaticFeeding = false;
        Event motorFailure;
        motorFailure.CreatedOn = lastFedTimestamp;
        motorFailure.Type = MotorFaliure;
        Event feedMissed;
        feedMissed.CreatedOn = lastFedTimestamp;
        feedMissed.Type = MissedFeed;
        dataAccess.logEventHistory(motorFailure);
        dataAccess.logEventHistory(feedMissed);
        eventHistoryBuffer.push_back(motorFailure);
        eventHistoryBuffer.push_back(feedMissed);
      }
    }
  }
  /*
  Serial.print("prevTime: ");
  Serial.println(previousTimestamp);
  Serial.print("curTime: ");
  Serial.println(currentTimestamp);
  */
  const SignificantWeightChange significantChange = weightDifferenceSignificant();
  updateLoopFrequency(significantChange);
  handleCurrentData(significantChange);
  handleNotifications();
  previousTimestamp = currentTimestamp;
  delete previousStatus;
  previousStatus = currentStatus;
  double delayVal = (1 / CURRENT_LOOP_FREQ) * 1000;
  delay(delayVal);
}

ScaleData createScaleDataHistory(Scale scaleID, double value) {
  ScaleData data;
  data.CreatedOn = getUnixTimestamp(currentTimestamp);
  data.ScaleID = scaleID;
  data.Value = value;
  return data;
}

void openContainer() {
  Serial.println("Open Container!");
  machineController.openContainer();
  currentStatus->Open = true;

  if (motorOperationCheckHandle != nullptr && eTaskGetState(motorOperationCheckHandle) != eDeleted) {
    vTaskDelete(motorOperationCheckHandle);
  }

  motorCheckParams.ContainerLoad = currentStatus->ContainerLoad;
  motorCheckParams.Open = true;
  xTaskCreatePinnedToCore(motorOperationCheckTask, "MotorOpenCheck", 4069, NULL, 1, &motorOperationCheckHandle, 1);
}

void closeContainer() {
  Serial.println("Close Container!");
  machineController.closeContainer();
  currentStatus->Open = false;

  if (motorOperationCheckHandle != nullptr && eTaskGetState(motorOperationCheckHandle) != eDeleted) {
    vTaskDelete(motorOperationCheckHandle);
  }

  motorCheckParams.ContainerLoad = currentStatus->ContainerLoad;
  motorCheckParams.Open = false;
  xTaskCreatePinnedToCore(motorOperationCheckTask, "MotorCloseCheck", 4069, NULL, 1, &motorOperationCheckHandle, 1);
}

MachineStatus* getUpdatedStatus() {
  MachineStatus* status = new MachineStatus();
  *status = *previousStatus;
  status->ContainerLoad = machineController.getContainerLoad();
  status->PlateLoad = machineController.getPlateLoad();
  return status;
}

SignificantWeightChange weightDifferenceSignificant() {
  const double time_D = (double)(currentTimestamp - previousTimestamp) / 1000;
  const double container_D = (currentStatus->ContainerLoad - previousStatus->ContainerLoad) / time_D;
  const double plate_D = (currentStatus->PlateLoad - previousStatus->PlateLoad) / time_D;
  SignificantWeightChange significantChange = None;

  if (abs(container_D) > WEIGHT_D_THRESHOLD && abs(plate_D) > WEIGHT_D_THRESHOLD) {
    significantChange = Both;
  } else if (abs(container_D) > WEIGHT_D_THRESHOLD) {
    significantChange = OnlyContainer;
  } else if (abs(plate_D) > WEIGHT_D_THRESHOLD) {
    significantChange = OnlyPlate;
  }

  return significantChange;
}

void updateLoopFrequency(SignificantWeightChange significantChange) {
  if (significantChange != None || currentStatus->Open) {
    CURRENT_LOOP_FREQ = LOOP_FREQ_FAST;
    noStatusChangeTimestamp = 0;
    if (significantChange != None) {
    }
  } else if (noStatusChangeTimestamp == 0) {
    noStatusChangeTimestamp = currentTimestamp;
  } else if (currentTimestamp - noStatusChangeTimestamp > COOLDOWN_TIME * 1000) {
    CURRENT_LOOP_FREQ = LOOP_FREQ_NORMAL;
  }
}

void handleCurrentData(SignificantWeightChange significantChange) {
  const bool clientsAvailable = networkController.hasWebClients();

  ArduinoJson::DynamicJsonDocument doc(1024);
  ArduinoJson::JsonObject status = doc.createNestedObject("status");
  setJsonStatus(currentStatus, status);
  ArduinoJson::JsonObject history = doc.createNestedObject("history");
  ArduinoJson::JsonArray schedules = history.createNestedArray("schedules");
  ArduinoJson::JsonArray events = history.createNestedArray("events");
  ArduinoJson::JsonArray scaleData = history.createNestedArray("scaleData");

  if (historySchedule != nullptr) {
    ArduinoJson::JsonObject dataObject = schedules.createNestedObject();
    setJsonSchedule(dataObject, historySchedule);
    delete historySchedule;
    historySchedule = nullptr;
  }
  
  if (!eventHistoryBuffer.empty()) {
    for (Event eventData : eventHistoryBuffer) {
      ArduinoJson::JsonObject dataObject = events.createNestedObject();
      dataObject["CreatedOn"] = eventData.CreatedOn;
      dataObject["Type"] = eventData.Type;
    }
    eventHistoryBuffer.clear();
  }

  switch (significantChange) {
    case OnlyContainer:
      {
        ScaleData data = createScaleDataHistory(Container, currentStatus->ContainerLoad);
        containerScaleHistoryBuffer.push_back(data);
        ArduinoJson::JsonObject dataObject = scaleData.createNestedObject();
        setJsonScaleHistory(&data, dataObject);
        break;
      }
    case OnlyPlate:
      {
        /*ScaleData data = createScaleDataHistory(Plate, currentStatus->PlateLoad);
        plateScaleHistoryBuffer.push_back(data);
        ArduinoJson::JsonObject dataObject = scaleData.createNestedObject();
        setJsonScaleHistory(&data, dataObject);*/
        break;
      }
    case Both:
      {
        ScaleData data_A = createScaleDataHistory(Container, currentStatus->ContainerLoad);
        containerScaleHistoryBuffer.push_back(data_A);
        ArduinoJson::JsonObject dataObject_A = scaleData.createNestedObject();
        setJsonScaleHistory(&data_A, dataObject_A);
        ScaleData data_B = createScaleDataHistory(Plate, currentStatus->PlateLoad);
        plateScaleHistoryBuffer.push_back(data_B);
        ArduinoJson::JsonObject dataObject_B = scaleData.createNestedObject();
        setJsonScaleHistory(&data_B, dataObject_B);
        break;
      }
  }

  if (clientsAvailable) {
    String serialized;
    serializeJson(doc, serialized);
    networkController.broadcast(serialized.c_str());
    //Serial.print("Status doc overflowed: ");
    //Serial.println(doc.overflowed());
  }
  doc.clear();

  //save and clear history buffers if possible/necessary
  if (CURRENT_LOOP_FREQ == LOOP_FREQ_NORMAL) {
    if (!containerScaleHistoryBuffer.empty()) {
      //log
      Serial.print("Empty + log Container Scale buffer");
      dataAccess.logScaleHistory(containerScaleHistoryBuffer);
      containerScaleHistoryBuffer.clear();
    }
    if (!plateScaleHistoryBuffer.empty()) {
      //log
      Serial.print("Empty + log Plate Scale buffer");
      dataAccess.logScaleHistory(plateScaleHistoryBuffer);
      plateScaleHistoryBuffer.clear();
    }
  } else {
    if (containerScaleHistoryBuffer.size() >= MAX_HISTORY_BUFFER) {
      //log
      Serial.print("Empty + log Container Scale buffer");
      dataAccess.logScaleHistory(containerScaleHistoryBuffer);
      containerScaleHistoryBuffer.clear();
    }
    if (plateScaleHistoryBuffer.size() >= MAX_HISTORY_BUFFER) {
      //log
      Serial.print("Empty + log Plate Scale buffer");
      dataAccess.logScaleHistory(plateScaleHistoryBuffer);
      plateScaleHistoryBuffer.clear();
    }
  }
}

void handleNotifications() {
  if (userSettings->Notifications.ContainerEmpty.Active
      && previousStatus->ContainerLoad > CONTAINER_EMPTY_THRESHOLD
      && currentStatus->ContainerLoad <= CONTAINER_EMPTY_THRESHOLD
      && currentStatus->ContainerLoad > NO_CONTAINER_THRESHOLD) {
    //Task -> check if condition still true later, if yes send notification
  }

  if (dayChanged()) {
    numTimesFedToday = 0;
    if (userSettings->Notifications.DidNotEatInADay.Active) {
      //send notification
    }
  }
}

bool feedPending() {
  if (selectedSchedule == nullptr) {
    return false;
  }

  bool pending = false;

  if (selectedSchedule->Mode == FixedDaytime) {
    long latestFeedtimeSmallerNow = -1;
    //order daytimes descending
    std::sort(selectedSchedule->Daytimes.begin(), selectedSchedule->Daytimes.end(), [](int a, int b) {
      return a > b;
    });
    for (long datetime : selectedSchedule->Daytimes) {
      if (datetime <= currentTimestamp) {
        latestFeedtimeSmallerNow = datetime;
        break;
      }
    }
    /*Serial.print("CurrentTimestamp: ");
    Serial.println(currentTimestamp);
    Serial.print("LatestFeedtimeSmallerNow: ");
    Serial.println(latestFeedtimeSmallerNow);
    Serial.print("LastFed: ");
    Serial.println(lastFedTimestamp);*/
    pending = latestFeedtimeSmallerNow != -1 && getUnixTimestamp(latestFeedtimeSmallerNow) > lastFedTimestamp;
  } else if (selectedSchedule->Mode == MaxTimes) {
    pending = currentStatus->PlateLoad <= PLATE_EMPTY_THRESHOLD && numTimesFedToday < selectedSchedule->MaxTimes;
  }

  return pending;
}

bool dayChanged() {
  //TODO
  return getDay(currentTimestamp / 1000) > getDay(previousTimestamp / 1000);
}

long getDay(long timestamp) {
  return timestamp / 86400L;
}

long getUnixTimestamp(long daytimeMS){
  return networkController.getToday() + daytimeMS / 1000;
}

