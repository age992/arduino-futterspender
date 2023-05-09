/* === Start of DataAccess.cpp === */
#include "DataAccess.h"
#include "JsonHelper.h"
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>

bool initialized = false;
int rc;

const char *configPath = "/config.json";
const String dbPrefix = "/sd";
const String dbRootPath = "/data";

sqlite3 *dbSystem;
sqlite3 *dbHistory;
sqlite3 *dbUser;

int openDb(const char *filename, sqlite3 **db) {
  rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

const char *sqlite3_column_string(sqlite3_stmt *stmt, int index) {
  return reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
}

bool DataAccess::init() {
  if (initialized) {
    return true;
  }

  while (!SD.begin()) {
    Serial.println("Trying to initialize SD Card...");
    delay(3000);
  }

  Serial.println("SD card initialized.");
  Serial.println("Init DB...");

  sqlite3_initialize();

  String systemDbPath = dbPrefix + dbRootPath + "/System.db";
  if (openDb(systemDbPath.c_str(), &dbSystem)) {
    return false;
  }

  String historyDbPath = dbPrefix + dbRootPath + "/History.db";
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    return false;
  }

  String userDbPath = dbPrefix + dbRootPath + "/User.db";
  if (openDb(userDbPath.c_str(), &dbUser)) {
    return false;
  }

  initialized = true;
  return true;
}

Config *DataAccess::getConfig() {
  if (!SD.exists(configPath)) {
    Serial.println(F("Config file not found"));
    return nullptr;
  }

  Config* configObject = new Config();
  File file = SD.open(configPath);
  StaticJsonDocument<512> doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize config file"));
    return nullptr;
  };

  strlcpy(configObject->ssid, doc["ssid"], sizeof(configObject->ssid));
  strlcpy(configObject->password, doc["password"], sizeof(configObject->password));
  strlcpy(configObject->localDomainName, doc["localDomainName"], sizeof(configObject->localDomainName));
  file.close();
  
  return configObject;
}

//--- DB: System ---

SystemSettings *DataAccess::getSystemSettings() {
  SystemSettings* settings = new SystemSettings();

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbSystem, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings->CalibrationWeight = sqlite3_column_int(stmt, 0);
    settings->ContainerScale = sqlite3_column_double(stmt, 1);
    settings->ContainerOffset = sqlite3_column_int(stmt, 2);
    settings->PlateScale = sqlite3_column_double(stmt, 3);
    settings->PlateOffset = sqlite3_column_int(stmt, 4);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbSystem));
  }

  sqlite3_finalize(stmt);
  return settings;
}

//--- DB: History ---

int DataAccess::getNumFedFromTo(long from, long to) {
  int count = 0;

  char *sql = "SELECT COUNT(*) FROM Event WHERE Type = ? AND CreatedOn >= ? AND CreatedOn < ?";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, Feed);
  rc = sqlite3_bind_int64(stmt, 2, from);
  rc = sqlite3_bind_int64(stmt, 3, to);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  sqlite3_finalize(stmt);
  return count;
}

long DataAccess::getLastFedTimestampBefore(long before) {
  long lastFedTimestamp;

  char *sql = "SELECT CreatedOn FROM Event LIMIT 1 WHERE Type = ? AND CreatedOn <= ?";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, Feed);
  rc = sqlite3_bind_int64(stmt, 2, before);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    String lastFedTimestampString = sqlite3_column_string(stmt, 0);
    lastFedTimestamp = std::stol(lastFedTimestampString.c_str());
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  sqlite3_finalize(stmt);
  return lastFedTimestamp;
}

//--- DB: User ---

UserSettings *DataAccess::getUserSettings() {
  UserSettings* settings = new UserSettings();

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings->PetName = sqlite3_column_string(stmt, 0);
    settings->PlateTAR = sqlite3_column_double(stmt, 1);
    settings->PlateFilling = sqlite3_column_double(stmt, 2);

    String notiListString = sqlite3_column_string(stmt, 3);  //deserialize json
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, notiListString.c_str());

    if (error) {
      Serial.println(F("Failed to deserialize Notifications for UserSettings"));
      return nullptr;
    };

    NotificationList* notificationList = new NotificationList();
    notificationList->ContainerEmpty.Active = doc["ContainerEmpty"]["Active"].as<bool>();
    notificationList->ContainerEmpty.Email = doc["ContainerEmpty"]["Email"].as<bool>();
    notificationList->ContainerEmpty.Phone = doc["ContainerEmpty"]["Phone"].as<bool>();
    notificationList->DidNotEatInADay.Active = doc["DidNotEatInADay"]["Active"].as<bool>();
    notificationList->DidNotEatInADay.Email = doc["DidNotEatInADay"]["Email"].as<bool>();
    notificationList->DidNotEatInADay.Phone = doc["DidNotEatInADay"]["Phone"].as<bool>();

    settings->Notifications = *notificationList;
    settings->Email = sqlite3_column_string(stmt, 4);
    settings->Phone = sqlite3_column_string(stmt, 5);
    settings->Language = sqlite3_column_int(stmt, 6);
    settings->Theme = sqlite3_column_int(stmt, 7);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return settings;
}

bool DataAccess::updateUserSettings(UserSettings userSettings) {
  return false;
}

Schedule *readScheduleRow(sqlite3_stmt* stmt) {
  Schedule schedule;
  schedule.ID = sqlite3_column_int(stmt, 0);
  schedule.CreatedOn = sqlite3_column_int64(stmt, 1);
  schedule.Name = sqlite3_column_string(stmt, 2);
  schedule.Mode = sqlite3_column_int(stmt, 3);
  schedule.Selected = sqlite3_column_int(stmt, 4) == 1;
  schedule.Active = sqlite3_column_int(stmt, 5) == 1;
  String daytimesString = sqlite3_column_string(stmt, 6);
  schedule.Daytimes = deserializeDaytimes(daytimesString);
  schedule.MaxTimes = sqlite3_column_int(stmt, 7);
  schedule.MaxTimesStartTime = sqlite3_column_int64(stmt, 8);
  schedule.OnlyWhenEmpty = sqlite3_column_int(stmt, 9) == 1;
  return &schedule;
}

Schedule *DataAccess::getSelectedSchedule() {
  Schedule *schedulePointer = nullptr;

  char *sql = "SELECT * FROM Schedules LIMIT 1 WHERE Selected=1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedulePointer = readScheduleRow(stmt);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return schedulePointer;
}

bool DataAccess::setSelectSchedule(int scheduleID, bool selected) {
  sqlite3_stmt *stmt;
  const char *sql = "UPDATE Schedules "
                    "SET Selected = ? "
                    "WHERE ID = ?;";

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  rc = sqlite3_bind_int(stmt, 1, selected ? 1 : 0);
  rc = sqlite3_bind_int(stmt, 2, scheduleID);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  return rc == SQLITE_OK;
}

bool DataAccess::setActiveSchedule(int scheduleID, bool active) {
  sqlite3_stmt *stmt;
  const char *sql = "UPDATE Schedules "
                    "SET Selected = 1, "
                    "Active = ? "
                    "WHERE ID = ?;";

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  rc = sqlite3_bind_int(stmt, 1, active ? 1 : 0);
  rc = sqlite3_bind_int(stmt, 2, scheduleID);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  return rc == SQLITE_OK;
}

std::vector<Schedule> DataAccess::getAllSchedules() {
  std::vector<Schedule> schedules;

  char *sql = "SELECT * FROM Schedules;";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    Schedule* schedulePointer = readScheduleRow(stmt);
    if(schedulePointer != nullptr){
      schedules.push_back(*schedulePointer);
    }
  }
  
  if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return schedules;
}

Schedule *DataAccess::getScheduleByID(int id) {
  Schedule *schedulePointer = nullptr;

  char *sql = "SELECT * FROM Schedules LIMIT 1 WHERE ID = ?;";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, id);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedulePointer = readScheduleRow(stmt);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return schedulePointer;
}

int DataAccess::insertSchedule(Schedule schedule) {
  return 0;
}

bool DataAccess::updateSchedule(Schedule schedule) {
  return false;
}

bool DataAccess::deleteSchedule(int scheduleID) {
  sqlite3_stmt *stmt;
  const char *sql = "DELETE FROM Schedules WHERE ID = ?;";

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, scheduleID);
  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  return rc == SQLITE_OK;
}
/* === End of DataAccess.cpp === */

/* === Start of DataAccess.h === */
#ifndef DATAACESS_H
#define DATAACESS_H

#include "Models.h"
#include <vector>

class DataAccess {

public:
  bool init();
  Config *getConfig();

  //--- DB: System ---

  SystemSettings *getSystemSettings();

  //--- DB: History ---

  int getNumFedFromTo(long from, long to);
  long getLastFedTimestampBefore(long before);

  //--- DB: User ---

  UserSettings *getUserSettings();
  bool updateUserSettings(UserSettings userSettings);

  Schedule *getSelectedSchedule();
  bool setSelectSchedule(int scheduleID, bool selected);
  bool setActiveSchedule(int scheduleID, bool active);
  std::vector<Schedule> getAllSchedules();
  Schedule* getScheduleByID(int id);
  int insertSchedule(Schedule schedule);
  bool updateSchedule(Schedule schedule);
  bool deleteSchedule(int scheduleID);
  
};

#endif
/* === End of DataAccess.h === */

/* === Start of JsonHelper.cpp === */
#include "JsonHelper.h"
#include <ArduinoJson.h>

String serializeStatus(MachineStatus data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);
  doc["ContainerLoad"] = data.ContainerLoad;
  doc["PlateLoad"] = data.PlateLoad;
  doc["Open"] = data.Open;
  doc["MotorOperation"] = data.MotorOperation;
  doc["SDCardConnection"] = data.SDCardConnection;
  serializeJson(doc, serialized);
  return serialized;
}

String serializeScaleData(ScaleData data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);
  doc["ID"] = data.ScaleID;
  doc["CreatedOn"] = data.CreatedOn;
  doc["ScaleID"] = data.ScaleID;
  doc["Value"] = data.Value;
  serializeJson(doc, serialized);
  return serialized;
}

String serializeNotification(Notification data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);
  doc["Active"] = data.Active;
  doc["Email"] = data.Email;
  doc["Phone"] = data.Phone;
  serializeJson(doc, serialized);
  return serialized;
};

String serializeDaytimes(std::vector<long> data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);

  for (long datetime : data) {
    doc.add(datetime);
  }

  serializeJson(doc, serialized);
  return serialized;
};

Schedule deserializeSchedule(String data){
  
};

std::vector<long> deserializeDaytimes(String data) {
  std::vector<long> daytimes;
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data.c_str());

  if (error) {
    Serial.println(F("Failed to deserialize daytimes for schedule"));
    return daytimes;
  };

  JsonArray jsonArray = doc.as<JsonArray>();

  for (JsonVariant v : jsonArray) {
    daytimes.push_back(v.as<long>());
  }

  return daytimes;
}

Notification deserializeNotification(String data) {
}
/* === End of JsonHelper.cpp === */

/* === Start of JsonHelper.h === */
#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <Arduino.h>
#include <vector>
#include "Models.h"

String serializeStatus(MachineStatus data);
String serializeScaleData(ScaleData data);

String serializeNotification(Notification data);
String serializeSchedule(Schedule data);
String serializeDaytimes(std::vector<long> data);

Schedule deserializeSchedule(String data);
std::vector<long> deserializeDaytimes(String data);

Notification deserializeNotification(String data);

#endif
/* === End of JsonHelper.h === */

/* === Start of MachineController.cpp === */
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
  long val = scale.get_units(LOADCELL_TIMES) / 100;
  double scaleValue = (double)val / 10;
  Serial.println(scaleValue);
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
/* === End of MachineController.cpp === */

/* === Start of MachineController.h === */
#ifndef MACHINECONTROLLER_H
#define MACHINECONTROLLER_H

#include "Models.h"
#include "DataAccess.h"

extern DataAccess dataAccess;

class MachineController {
  public:
    bool initControls();
    void calibrateContainerScale(double scaleFactor, int offset );
    void calibratePlateScale(double scaleFactor, int offset );
    double getContainerLoad();
    double getPlateLoad();
    void openContainer();
    void closeContainer();
}; 

#endif
/* === End of MachineController.h === */

/* === Start of Models.cpp === */
#include "Models.h"
/* === End of Models.cpp === */

/* === Start of Models.h === */
#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <vector>

struct MachineStatus {
  double ContainerLoad;
  double PlateLoad;
  bool Open;
  bool SDCardConnection;
  bool MotorOperation;
  bool WiFiConnection;
};

struct MotorCheckParams {
  double ContainerLoad;
  bool Open;
};

enum SignificantWeightChange {
  None,
  OnlyContainer,
  OnlyPlate,
  Both
};

struct Config {
  char ssid[64];
  char password[64];
  char localDomainName[64];
};

//--- DB: System ---

struct SystemSettings {
  int CalibrationWeight;
  double ContainerScale;
  int ContainerOffset;
  double PlateScale;
  int PlateOffset;
};

//--- DB: History ---

enum EventType { Feed,
                 MissedFeed,
                 ContainerEmpty,
                 MotorFaliure,
                 WiFiConnectionLost,
                 WiFiConnectionReturned,
                 SDConnectionLost,
                 SDConnectionReturned
};

struct Event {
  int ID;
  long CreatedOn;
  EventType Type;
  String message;
};

struct ScaleData {
  int ID;
  long CreatedOn;
  int ScaleID;  //0 = scale_A, 1 = scale_B
  double Value;
};

//--- DB: User ---

enum ScheduleMode {
  FixedDaytime,
  MaxTimes
};

struct Schedule {  //also for history
  int ID;
  long CreatedOn;
  String Name;
  int Mode;
  bool Selected;
  bool Active;
  std::vector<long> Daytimes;
  int MaxTimes;
  long MaxTimesStartTime;
  bool OnlyWhenEmpty;
};

struct Notification {
  bool Active;
  bool Email;
  bool Phone;
};

struct NotificationList {
  Notification ContainerEmpty;
  Notification DidNotEatInADay;
};

struct UserSettings {
  String PetName;
  double PlateTAR;
  double PlateFilling;
  NotificationList Notifications;
  String Email;
  String Phone;
  int Language;
  int Theme;
};

#endif
/* === End of Models.h === */

/* === Start of NetworkController.cpp === */
#include <vector>
#include "NetworkController.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <freertos/projdefs.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <SD.h>

const String frontendRootPath = "/frontend/";

WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int CLEAN_CLIENTS_INTERVAL = 10;  //seconds
TaskHandle_t clearClientsTaskHandle;

void clearClientsTask(void *pvParameters) {
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(CLEAN_CLIENTS_INTERVAL * 1000));
    ws.cleanupClients();
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("Websocket client connection received");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
  }
}

//void switchSelectedSchedule(Schedule* newSchedule){}

//-- Rest API handlers --//

void handleApiScheduleActivate(AsyncWebServerRequest *request) {
  Serial.println("Handle activate schedule request");

  if (!request->hasParam("id") || !request->hasParam("active")) {
    request->send(400);
  }

  AsyncWebParameter *pId = request->getParam("id");
  AsyncWebParameter *pActive = request->getParam("active");

  try {
    const int id = std::stoi(pId->value().c_str());
    const bool active = pActive->value().equals("true");

    dataAccess.setActiveSchedule(id, true);

    if (selectedSchedule != nullptr) {
      if (selectedSchedule->ID != id) {
        dataAccess.setSelectSchedule(selectedSchedule->ID, false);
        selectedSchedule = dataAccess.getSelectedSchedule();
      } else {
        selectedSchedule->Active = active;
      }
    } else {
      selectedSchedule = dataAccess.getSelectedSchedule();
    }
  } catch (...) {
    request->send(400);
  }
}

void handleApiScheduleGet(AsyncWebServerRequest *request) {
  if (request->hasParam("id")) {
    try {
      AsyncWebParameter *pId = request->getParam("id");
      const int id = std::stoi(pId->value().c_str());
      Schedule *schedule = dataAccess.getScheduleByID(id);

      if (schedule == nullptr) {
        //return empty response
      } else {
        //return serialized response
      }
    } catch (...) {
      request->send(400);
    }
  } else {
    std::vector<Schedule> schedules = dataAccess.getAllSchedules();
    //return serialized response
  }
}

void handleApiScheduleDelete(AsyncWebServerRequest *request) {
  if (!request->hasParam("id")) {
    request->send(400);
  }
  try {
    AsyncWebParameter *pId = request->getParam("id");
    const int id = std::stoi(pId->value().c_str());
    if (dataAccess.deleteSchedule(id)) {
      if (selectedSchedule != nullptr && selectedSchedule->ID == id) {
        selectedSchedule = nullptr;
      }
      request->send(200);
    } else {
      request->send(404);
    }
  } catch (...) {
    request->send(400);
  }
}

void handleApiSchedule(AsyncWebServerRequest *request, uint8_t *data) {
  WebRequestMethodComposite method = request->method();
  
  switch (method) {
    case HTTP_POST:
      {
        Serial.print("POST ");
        break;
      }
    case HTTP_PUT:
      {
        Serial.print("PUT ");
        break;
      }
  }
  Serial.println("Schedule data: ");
  Serial.println(reinterpret_cast<char*>(data));
  request->send(400);
}

void handleApiSettings(AsyncWebServerRequest *request) {
  WebRequestMethodComposite method = request->method();
  request->send(200);
}

void handleApiContainer(AsyncWebServerRequest *request) {
  if (!request->hasParam("open")) {
    request->send(400);
    return;
  }

  AsyncWebParameter *p = request->getParam("open");
  bool open = p->value().equals("true");

  if (open) {
    //servo.write(90);
  } else {
    //servo.write(0);
  }

  request->send(200);
}

//---//

bool NetworkController::initNetworkConnection(Config* config) {
  Serial.print("Connecting to WiFi...");
  Serial.print(config->ssid);
  Serial.print(" ");
  Serial.println(config->password);
  WiFi.begin(config->ssid, config->password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Start MDNS...");
  const char *domainName = config->localDomainName != nullptr ? config->localDomainName : "futterspender";
  if (MDNS.begin(domainName)) {
    Serial.println("MDNS started!");
  } else {
    Serial.println("Couldn't start MDNS");
  }
  return true;
}

bool NetworkController::initNTP() {
  ntpClient.begin();
  return true;
}

long NetworkController::getCurrentTime() {
  ntpClient.update();
  return ntpClient.getEpochTime();
}

bool NetworkController::initWebserver() {
  server.on("/api/container", [](AsyncWebServerRequest *request) {
    handleApiContainer(request);
  });
  server.on("/api/settings", [](AsyncWebServerRequest *request) {
    handleApiSettings(request);
  });
  server.on("/api/schedule/activate", [](AsyncWebServerRequest *request) {
    handleApiScheduleActivate(request);
  });
  server.on("/api/schedule", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleApiScheduleGet(request);
  });
  server.on("/api/schedule", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    handleApiScheduleDelete(request);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/api/schedule/activate") {
      handleApiSchedule(request, data);
    }
  });

  server.on("/api", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text", "Sorry, this page does not exist!");
    }
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  /*
  AsyncCallbackJsonWebHandler *scheduleJsonHandler = new AsyncCallbackJsonWebHandler("/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &jsonObj = json.as<JsonObject>();
    Serial.println("Received Schedule: ");
    jsonObj.prettyPrintTo(Serial);
    // ...
  });
  server.addHandler(scheduleJsonHandler);*/

  server.begin();
  Serial.println("Web Server started");
  xTaskCreate(clearClientsTask, "ClearClientsTask", 4096, NULL, 1, &clearClientsTaskHandle);
  return true;
}

bool NetworkController::hasWebClients() {
  return ws.count() > 0;
}

void NetworkController::broadcast(const char *serializedMessage) {
  ws.textAll(serializedMessage);
}
/* === End of NetworkController.cpp === */

/* === Start of NetworkController.h === */
#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include "Models.h"
#include "DataAccess.h"

extern DataAccess dataAccess;
extern Schedule* selectedSchedule;
extern UserSettings* userSettings;

class NetworkController {
public:
  bool initNetworkConnection(Config* config);

  bool initNTP();
  long getCurrentTime();

  bool initWebserver();
  bool hasWebClients();
  void broadcast(const char* serializedMessage);
};

#endif
/* === End of NetworkController.h === */

/* === Start of tgrbxsvr.ino === */
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

DataAccess dataAccess;
MachineController machineController;
NetworkController networkController;

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

const int MOTOR_CHECK_WAIT = 2; //seconds
TaskHandle_t motorOperationCheckHandle;
MotorCheckParams motorCheckParams;

void motorOperationCheckTask(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(MOTOR_CHECK_WAIT * 1000));

  const double container_D = currentStatus->ContainerLoad - motorCheckParams.ContainerLoad;

  if(motorCheckParams.Open){
    if(container_D / (double) MOTOR_CHECK_WAIT > -1 * WEIGHT_D_THRESHOLD){
      currentStatus->MotorOperation = false;
    }else{
      currentStatus->MotorOperation = true;
    }
  }else{
    if(container_D <= -1 * WEIGHT_D_THRESHOLD){
      currentStatus->MotorOperation = false;
    }else{
      currentStatus->MotorOperation = true;
    }
  }
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
    if (currentStatus->ContainerLoad > CONTAINER_EMPTY_THRESHOLD) {
      if (!currentStatus->Open) {
        //start feeding
        currentFeedTargetWeight = min(userSettings->PlateFilling, currentStatus->PlateLoad + currentStatus->ContainerLoad) - SAFETY_WEIGHT_GAP;
        openContainer();
      } else if (currentStatus->PlateLoad >= currentFeedTargetWeight || currentStatus->ContainerLoad <= CONTAINER_EMPTY_THRESHOLD) {
        //finished feeding
        lastFedTimestamp = currentTimestamp;
        currentFeedTargetWeight = 0;
        numTimesFedToday++;
        if (selectedSchedule != nullptr && selectedSchedule->Mode == MaxTimes && numTimesFedToday == selectedSchedule->MaxTimes) {
          //set wait timestamp tomorrow
        }
        closeContainer();
        //log history: eventtype = feed, currentTimestamp
      } else if (!currentStatus->MotorOperation) {
        //abort feeding
        lastFedTimestamp = currentTimestamp;
        currentFeedTargetWeight = 0;
        closeContainer();
        //log history: eventtype = missedFeed, currentTimestamp
      }
    }
  }

  const SignificantWeightChange significantChange = weightDifferenceSignificant();
  updateLoopFrequency(significantChange);
  handleCurrentData(significantChange);
  handleNotifications();

  previousTimestamp = currentTimestamp;
  previousStatus = currentStatus;
  delay(1 / CURRENT_LOOP_FREQ);
}

ScaleData createScaleDataHistory(int scaleID, double value) {
  ScaleData data;
  data.CreatedOn = currentTimestamp;
  data.ScaleID = scaleID;
  data.Value = value;
  return data;
}

void openContainer() {
  machineController.openContainer();
  currentStatus->Open = true;

  if(eTaskGetState(motorOperationCheckHandle) != eDeleted){
    vTaskDelete(motorOperationCheckHandle);
  }
  motorCheckParams.ContainerLoad = currentStatus->ContainerLoad;
  motorCheckParams.Open = true;
  xTaskCreate(motorOperationCheckTask, "MotorOperation Open Check", 4096, NULL, 1, &motorOperationCheckHandle);
}

void closeContainer() {
  machineController.closeContainer();
  currentStatus->Open = false;

  if(eTaskGetState(motorOperationCheckHandle) != eDeleted){
    vTaskDelete(motorOperationCheckHandle);
  }
  motorCheckParams.ContainerLoad = currentStatus->ContainerLoad;
  motorCheckParams.Open = false;
  xTaskCreate(motorOperationCheckTask, "MotorOperation Open Check", 4096, NULL, 1, &motorOperationCheckHandle);
}

MachineStatus getUpdatedStatus() {
  MachineStatus status = *previousStatus;
  status.ContainerLoad = machineController.getContainerLoad();
  status.PlateLoad = machineController.getPlateLoad();
  return status;
}

SignificantWeightChange weightDifferenceSignificant() {
  const double time_D = currentTimestamp - previousTimestamp;
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
  } else if (currentTimestamp - noStatusChangeTimestamp > COOLDOWN_TIME) {
    CURRENT_LOOP_FREQ = LOOP_FREQ_NORMAL;
  }
}

void handleCurrentData(SignificantWeightChange significantChange) {
  const bool clientsAvailable = networkController.hasWebClients();

  ArduinoJson::DynamicJsonDocument doc(1024);
  doc["status"] = serializeStatus(*currentStatus);
  ArduinoJson::JsonObject history = doc.createNestedObject("history");
  ArduinoJson::JsonArray schedules = history.createNestedArray("schedules");
  ArduinoJson::JsonArray events = history.createNestedArray("events");
  ArduinoJson::JsonArray scaleData = history.createNestedArray("scaleData");

  switch (significantChange) {
    case OnlyContainer:
      {
        ScaleData data = createScaleDataHistory(0, currentStatus->ContainerLoad);
        containerScaleHistoryBuffer.push_back(data);
        String serializedData = serializeScaleData(data);
        scaleData.add(serializedData);
        break;
      }
    case OnlyPlate:
      {
        ScaleData data = createScaleDataHistory(1, currentStatus->PlateLoad);
        plateScaleHistoryBuffer.push_back(data);
        String serializedData = serializeScaleData(data);
        scaleData.add(serializedData);
        break;
      }
    case Both:
      {
        ScaleData data_A = createScaleDataHistory(0, currentStatus->ContainerLoad);
        containerScaleHistoryBuffer.push_back(data_A);
        String serializedData_A = serializeScaleData(data_A);
        scaleData.add(serializedData_A);
        ScaleData data_B = createScaleDataHistory(1, currentStatus->PlateLoad);
        plateScaleHistoryBuffer.push_back(data_B);
        String serializedData_B = serializeScaleData(data_B);
        scaleData.add(serializedData_B);
        break;
      }
  }

  if (clientsAvailable) {
    String serialized;
    serializeJson(doc, serialized);
    networkController.broadcast(serialized.c_str());
  }

  //save and clear history buffers if possible/necessary
  if (CURRENT_LOOP_FREQ == LOOP_FREQ_NORMAL) {
    if (!containerScaleHistoryBuffer.empty()) {
    }
    if (!plateScaleHistoryBuffer.empty()) {
    }
  } else {
    if (containerScaleHistoryBuffer.size() >= MAX_HISTORY_BUFFER) {
    }
    if (plateScaleHistoryBuffer.size() >= MAX_HISTORY_BUFFER) {
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
  if (selectedSchedule == nullptr || !selectedSchedule->Active) {
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

bool dayChanged() {
  return getDay(currentTimestamp) > getDay(previousTimestamp);
}

long getDay(long timestamp) {
  return timestamp / 86400L;
}
/* === End of tgrbxsvr.ino === */

/* === Start of output.cpp === */
/* === Start of DataAccess.cpp === */
#include "DataAccess.h"
#include "JsonHelper.h"
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>

bool initialized = false;
int rc;

const char *configPath = "/config.json";
const String dbPrefix = "/sd";
const String dbRootPath = "/data";

sqlite3 *dbSystem;
sqlite3 *dbHistory;
sqlite3 *dbUser;

int openDb(const char *filename, sqlite3 **db) {
  rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
  }
  return rc;
}

const char *sqlite3_column_string(sqlite3_stmt *stmt, int index) {
  return reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
}

bool DataAccess::init() {
  if (initialized) {
    return true;
  }

  while (!SD.begin()) {
    Serial.println("Trying to initialize SD Card...");
    delay(3000);
  }

  Serial.println("SD card initialized.");
  Serial.println("Init DB...");

  sqlite3_initialize();

  String systemDbPath = dbPrefix + dbRootPath + "/System.db";
  if (openDb(systemDbPath.c_str(), &dbSystem)) {
    return false;
  }

  String historyDbPath = dbPrefix + dbRootPath + "/History.db";
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    return false;
  }

  String userDbPath = dbPrefix + dbRootPath + "/User.db";
  if (openDb(userDbPath.c_str(), &dbUser)) {
    return false;
  }

  initialized = true;
  return true;
}

Config *DataAccess::getConfig() {
  if (!SD.exists(configPath)) {
    Serial.println(F("Config file not found"));
    return nullptr;
  }

  Config* configObject = new Config();
  File file = SD.open(configPath);
  StaticJsonDocument<512> doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize config file"));
    return nullptr;
  };

  strlcpy(configObject->ssid, doc["ssid"], sizeof(configObject->ssid));
  strlcpy(configObject->password, doc["password"], sizeof(configObject->password));
  strlcpy(configObject->localDomainName, doc["localDomainName"], sizeof(configObject->localDomainName));
  file.close();
  
  return configObject;
}

//--- DB: System ---

SystemSettings *DataAccess::getSystemSettings() {
  SystemSettings* settings = new SystemSettings();

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbSystem, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings->CalibrationWeight = sqlite3_column_int(stmt, 0);
    settings->ContainerScale = sqlite3_column_double(stmt, 1);
    settings->ContainerOffset = sqlite3_column_int(stmt, 2);
    settings->PlateScale = sqlite3_column_double(stmt, 3);
    settings->PlateOffset = sqlite3_column_int(stmt, 4);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbSystem));
  }

  sqlite3_finalize(stmt);
  return settings;
}

//--- DB: History ---

int DataAccess::getNumFedFromTo(long from, long to) {
  int count = 0;

  char *sql = "SELECT COUNT(*) FROM Event WHERE Type = ? AND CreatedOn >= ? AND CreatedOn < ?";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, Feed);
  rc = sqlite3_bind_int64(stmt, 2, from);
  rc = sqlite3_bind_int64(stmt, 3, to);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  sqlite3_finalize(stmt);
  return count;
}

long DataAccess::getLastFedTimestampBefore(long before) {
  long lastFedTimestamp;

  char *sql = "SELECT CreatedOn FROM Event LIMIT 1 WHERE Type = ? AND CreatedOn <= ?";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, Feed);
  rc = sqlite3_bind_int64(stmt, 2, before);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    String lastFedTimestampString = sqlite3_column_string(stmt, 0);
    lastFedTimestamp = std::stol(lastFedTimestampString.c_str());
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  sqlite3_finalize(stmt);
  return lastFedTimestamp;
}

//--- DB: User ---

UserSettings *DataAccess::getUserSettings() {
  UserSettings* settings = new UserSettings();

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings->PetName = sqlite3_column_string(stmt, 0);
    settings->PlateTAR = sqlite3_column_double(stmt, 1);
    settings->PlateFilling = sqlite3_column_double(stmt, 2);

    String notiListString = sqlite3_column_string(stmt, 3);  //deserialize json
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, notiListString.c_str());

    if (error) {
      Serial.println(F("Failed to deserialize Notifications for UserSettings"));
      return nullptr;
    };

    NotificationList* notificationList = new NotificationList();
    notificationList->ContainerEmpty.Active = doc["ContainerEmpty"]["Active"].as<bool>();
    notificationList->ContainerEmpty.Email = doc["ContainerEmpty"]["Email"].as<bool>();
    notificationList->ContainerEmpty.Phone = doc["ContainerEmpty"]["Phone"].as<bool>();
    notificationList->DidNotEatInADay.Active = doc["DidNotEatInADay"]["Active"].as<bool>();
    notificationList->DidNotEatInADay.Email = doc["DidNotEatInADay"]["Email"].as<bool>();
    notificationList->DidNotEatInADay.Phone = doc["DidNotEatInADay"]["Phone"].as<bool>();

    settings->Notifications = *notificationList;
    settings->Email = sqlite3_column_string(stmt, 4);
    settings->Phone = sqlite3_column_string(stmt, 5);
    settings->Language = sqlite3_column_int(stmt, 6);
    settings->Theme = sqlite3_column_int(stmt, 7);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return settings;
}

bool DataAccess::updateUserSettings(UserSettings userSettings) {
  return false;
}

Schedule *readScheduleRow(sqlite3_stmt* stmt) {
  Schedule schedule;
  schedule.ID = sqlite3_column_int(stmt, 0);
  schedule.CreatedOn = sqlite3_column_int64(stmt, 1);
  schedule.Name = sqlite3_column_string(stmt, 2);
  schedule.Mode = sqlite3_column_int(stmt, 3);
  schedule.Selected = sqlite3_column_int(stmt, 4) == 1;
  schedule.Active = sqlite3_column_int(stmt, 5) == 1;
  String daytimesString = sqlite3_column_string(stmt, 6);
  schedule.Daytimes = deserializeDaytimes(daytimesString);
  schedule.MaxTimes = sqlite3_column_int(stmt, 7);
  schedule.MaxTimesStartTime = sqlite3_column_int64(stmt, 8);
  schedule.OnlyWhenEmpty = sqlite3_column_int(stmt, 9) == 1;
  return &schedule;
}

Schedule *DataAccess::getSelectedSchedule() {
  Schedule *schedulePointer = nullptr;

  char *sql = "SELECT * FROM Schedules LIMIT 1 WHERE Selected=1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedulePointer = readScheduleRow(stmt);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return schedulePointer;
}

bool DataAccess::setSelectSchedule(int scheduleID, bool selected) {
  sqlite3_stmt *stmt;
  const char *sql = "UPDATE Schedules "
                    "SET Selected = ? "
                    "WHERE ID = ?;";

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  rc = sqlite3_bind_int(stmt, 1, selected ? 1 : 0);
  rc = sqlite3_bind_int(stmt, 2, scheduleID);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  return rc == SQLITE_OK;
}

bool DataAccess::setActiveSchedule(int scheduleID, bool active) {
  sqlite3_stmt *stmt;
  const char *sql = "UPDATE Schedules "
                    "SET Selected = 1, "
                    "Active = ? "
                    "WHERE ID = ?;";

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  rc = sqlite3_bind_int(stmt, 1, active ? 1 : 0);
  rc = sqlite3_bind_int(stmt, 2, scheduleID);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  return rc == SQLITE_OK;
}

std::vector<Schedule> DataAccess::getAllSchedules() {
  std::vector<Schedule> schedules;

  char *sql = "SELECT * FROM Schedules;";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    Schedule* schedulePointer = readScheduleRow(stmt);
    if(schedulePointer != nullptr){
      schedules.push_back(*schedulePointer);
    }
  }
  
  if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return schedules;
}

Schedule *DataAccess::getScheduleByID(int id) {
  Schedule *schedulePointer = nullptr;

  char *sql = "SELECT * FROM Schedules LIMIT 1 WHERE ID = ?;";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, id);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedulePointer = readScheduleRow(stmt);
  } else if (rc == SQLITE_ERROR) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  return schedulePointer;
}

int DataAccess::insertSchedule(Schedule schedule) {
  return 0;
}

bool DataAccess::updateSchedule(Schedule schedule) {
  return false;
}

bool DataAccess::deleteSchedule(int scheduleID) {
  sqlite3_stmt *stmt;
  const char *sql = "DELETE FROM Schedules WHERE ID = ?;";

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, scheduleID);
  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  return rc == SQLITE_OK;
}
/* === End of DataAccess.cpp === */

/* === Start of DataAccess.h === */
#ifndef DATAACESS_H
#define DATAACESS_H

#include "Models.h"
#include <vector>

class DataAccess {

public:
  bool init();
  Config *getConfig();

  //--- DB: System ---

  SystemSettings *getSystemSettings();

  //--- DB: History ---

  int getNumFedFromTo(long from, long to);
  long getLastFedTimestampBefore(long before);

  //--- DB: User ---

  UserSettings *getUserSettings();
  bool updateUserSettings(UserSettings userSettings);

  Schedule *getSelectedSchedule();
  bool setSelectSchedule(int scheduleID, bool selected);
  bool setActiveSchedule(int scheduleID, bool active);
  std::vector<Schedule> getAllSchedules();
  Schedule* getScheduleByID(int id);
  int insertSchedule(Schedule schedule);
  bool updateSchedule(Schedule schedule);
  bool deleteSchedule(int scheduleID);
  
};

#endif
/* === End of DataAccess.h === */

/* === Start of JsonHelper.cpp === */
#include "JsonHelper.h"
#include <ArduinoJson.h>

String serializeStatus(MachineStatus data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);
  doc["ContainerLoad"] = data.ContainerLoad;
  doc["PlateLoad"] = data.PlateLoad;
  doc["Open"] = data.Open;
  doc["MotorOperation"] = data.MotorOperation;
  doc["SDCardConnection"] = data.SDCardConnection;
  serializeJson(doc, serialized);
  return serialized;
}

String serializeScaleData(ScaleData data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);
  doc["ID"] = data.ScaleID;
  doc["CreatedOn"] = data.CreatedOn;
  doc["ScaleID"] = data.ScaleID;
  doc["Value"] = data.Value;
  serializeJson(doc, serialized);
  return serialized;
}

String serializeNotification(Notification data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);
  doc["Active"] = data.Active;
  doc["Email"] = data.Email;
  doc["Phone"] = data.Phone;
  serializeJson(doc, serialized);
  return serialized;
};

String serializeDaytimes(std::vector<long> data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(512);

  for (long datetime : data) {
    doc.add(datetime);
  }

  serializeJson(doc, serialized);
  return serialized;
};

Schedule deserializeSchedule(String data){
  
};

std::vector<long> deserializeDaytimes(String data) {
  std::vector<long> daytimes;
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data.c_str());

  if (error) {
    Serial.println(F("Failed to deserialize daytimes for schedule"));
    return daytimes;
  };

  JsonArray jsonArray = doc.as<JsonArray>();

  for (JsonVariant v : jsonArray) {
    daytimes.push_back(v.as<long>());
  }

  return daytimes;
}

Notification deserializeNotification(String data) {
}
/* === End of JsonHelper.cpp === */

/* === Start of JsonHelper.h === */
#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <Arduino.h>
#include <vector>
#include "Models.h"

String serializeStatus(MachineStatus data);
String serializeScaleData(ScaleData data);

String serializeNotification(Notification data);
String serializeSchedule(Schedule data);
String serializeDaytimes(std::vector<long> data);

Schedule deserializeSchedule(String data);
std::vector<long> deserializeDaytimes(String data);

Notification deserializeNotification(String data);

#endif
/* === End of JsonHelper.h === */

/* === Start of MachineController.cpp === */
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
  long val = scale.get_units(LOADCELL_TIMES) / 100;
  double scaleValue = (double)val / 10;
  Serial.println(scaleValue);
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
/* === End of MachineController.cpp === */

/* === Start of MachineController.h === */
#ifndef MACHINECONTROLLER_H
#define MACHINECONTROLLER_H

#include "Models.h"
#include "DataAccess.h"

extern DataAccess dataAccess;

class MachineController {
  public:
    bool initControls();
    void calibrateContainerScale(double scaleFactor, int offset );
    void calibratePlateScale(double scaleFactor, int offset );
    double getContainerLoad();
    double getPlateLoad();
    void openContainer();
    void closeContainer();
}; 

#endif
/* === End of MachineController.h === */

/* === Start of Models.cpp === */
#include "Models.h"
/* === End of Models.cpp === */

/* === Start of Models.h === */
#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <vector>

struct MachineStatus {
  double ContainerLoad;
  double PlateLoad;
  bool Open;
  bool SDCardConnection;
  bool MotorOperation;
  bool WiFiConnection;
};

struct MotorCheckParams {
  double ContainerLoad;
  bool Open;
};

enum SignificantWeightChange {
  None,
  OnlyContainer,
  OnlyPlate,
  Both
};

struct Config {
  char ssid[64];
  char password[64];
  char localDomainName[64];
};

//--- DB: System ---

struct SystemSettings {
  int CalibrationWeight;
  double ContainerScale;
  int ContainerOffset;
  double PlateScale;
  int PlateOffset;
};

//--- DB: History ---

enum EventType { Feed,
                 MissedFeed,
                 ContainerEmpty,
                 MotorFaliure,
                 WiFiConnectionLost,
                 WiFiConnectionReturned,
                 SDConnectionLost,
                 SDConnectionReturned
};

struct Event {
  int ID;
  long CreatedOn;
  EventType Type;
  String message;
};

struct ScaleData {
  int ID;
  long CreatedOn;
  int ScaleID;  //0 = scale_A, 1 = scale_B
  double Value;
};

//--- DB: User ---

enum ScheduleMode {
  FixedDaytime,
  MaxTimes
};

struct Schedule {  //also for history
  int ID;
  long CreatedOn;
  String Name;
  int Mode;
  bool Selected;
  bool Active;
  std::vector<long> Daytimes;
  int MaxTimes;
  long MaxTimesStartTime;
  bool OnlyWhenEmpty;
};

struct Notification {
  bool Active;
  bool Email;
  bool Phone;
};

struct NotificationList {
  Notification ContainerEmpty;
  Notification DidNotEatInADay;
};

struct UserSettings {
  String PetName;
  double PlateTAR;
  double PlateFilling;
  NotificationList Notifications;
  String Email;
  String Phone;
  int Language;
  int Theme;
};

#endif
/* === End of Models.h === */

/* === Start of NetworkController.cpp === */
#include <vector>
#include "NetworkController.h"
#include "freertos/FreeRTOS.h"
#include <freertos/task.h>
#include <freertos/projdefs.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <AsyncTCP.h>
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <SD.h>

const String frontendRootPath = "/frontend/";

WiFiUDP ntpUdp;
NTPClient ntpClient(ntpUdp);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const int CLEAN_CLIENTS_INTERVAL = 10;  //seconds
TaskHandle_t clearClientsTaskHandle;

void clearClientsTask(void *pvParameters) {
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(CLEAN_CLIENTS_INTERVAL * 1000));
    ws.cleanupClients();
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("Websocket client connection received");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("Client disconnected");
  }
}

//void switchSelectedSchedule(Schedule* newSchedule){}

//-- Rest API handlers --//

void handleApiScheduleActivate(AsyncWebServerRequest *request) {
  Serial.println("Handle activate schedule request");

  if (!request->hasParam("id") || !request->hasParam("active")) {
    request->send(400);
  }

  AsyncWebParameter *pId = request->getParam("id");
  AsyncWebParameter *pActive = request->getParam("active");

  try {
    const int id = std::stoi(pId->value().c_str());
    const bool active = pActive->value().equals("true");

    dataAccess.setActiveSchedule(id, true);

    if (selectedSchedule != nullptr) {
      if (selectedSchedule->ID != id) {
        dataAccess.setSelectSchedule(selectedSchedule->ID, false);
        selectedSchedule = dataAccess.getSelectedSchedule();
      } else {
        selectedSchedule->Active = active;
      }
    } else {
      selectedSchedule = dataAccess.getSelectedSchedule();
    }
  } catch (...) {
    request->send(400);
  }
}

void handleApiScheduleGet(AsyncWebServerRequest *request) {
  if (request->hasParam("id")) {
    try {
      AsyncWebParameter *pId = request->getParam("id");
      const int id = std::stoi(pId->value().c_str());
      Schedule *schedule = dataAccess.getScheduleByID(id);

      if (schedule == nullptr) {
        //return empty response
      } else {
        //return serialized response
      }
    } catch (...) {
      request->send(400);
    }
  } else {
    std::vector<Schedule> schedules = dataAccess.getAllSchedules();
    //return serialized response
  }
}

void handleApiScheduleDelete(AsyncWebServerRequest *request) {
  if (!request->hasParam("id")) {
    request->send(400);
  }
  try {
    AsyncWebParameter *pId = request->getParam("id");
    const int id = std::stoi(pId->value().c_str());
    if (dataAccess.deleteSchedule(id)) {
      if (selectedSchedule != nullptr && selectedSchedule->ID == id) {
        selectedSchedule = nullptr;
      }
      request->send(200);
    } else {
      request->send(404);
    }
  } catch (...) {
    request->send(400);
  }
}

void handleApiSchedule(AsyncWebServerRequest *request, uint8_t *data) {
  WebRequestMethodComposite method = request->method();
  
  switch (method) {
    case HTTP_POST:
      {
        Serial.print("POST ");
        break;
      }
    case HTTP_PUT:
      {
        Serial.print("PUT ");
        break;
      }
  }
  Serial.println("Schedule data: ");
  Serial.println(reinterpret_cast<char*>(data));
  request->send(400);
}

void handleApiSettings(AsyncWebServerRequest *request) {
  WebRequestMethodComposite method = request->method();
  request->send(200);
}

void handleApiContainer(AsyncWebServerRequest *request) {
  if (!request->hasParam("open")) {
    request->send(400);
    return;
  }

  AsyncWebParameter *p = request->getParam("open");
  bool open = p->value().equals("true");

  if (open) {
    //servo.write(90);
  } else {
    //servo.write(0);
  }

  request->send(200);
}

//---//

bool NetworkController::initNetworkConnection(Config* config) {
  Serial.print("Connecting to WiFi...");
  Serial.print(config->ssid);
  Serial.print(" ");
  Serial.println(config->password);
  WiFi.begin(config->ssid, config->password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  Serial.println("Connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Start MDNS...");
  const char *domainName = config->localDomainName != nullptr ? config->localDomainName : "futterspender";
  if (MDNS.begin(domainName)) {
    Serial.println("MDNS started!");
  } else {
    Serial.println("Couldn't start MDNS");
  }
  return true;
}

bool NetworkController::initNTP() {
  ntpClient.begin();
  return true;
}

long NetworkController::getCurrentTime() {
  ntpClient.update();
  return ntpClient.getEpochTime();
}

bool NetworkController::initWebserver() {
  server.on("/api/container", [](AsyncWebServerRequest *request) {
    handleApiContainer(request);
  });
  server.on("/api/settings", [](AsyncWebServerRequest *request) {
    handleApiSettings(request);
  });
  server.on("/api/schedule/activate", [](AsyncWebServerRequest *request) {
    handleApiScheduleActivate(request);
  });
  server.on("/api/schedule", HTTP_GET, [](AsyncWebServerRequest *request) {
    handleApiScheduleGet(request);
  });
  server.on("/api/schedule", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    handleApiScheduleDelete(request);
  });
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    if (request->url() == "/api/schedule/activate") {
      handleApiSchedule(request, data);
    }
  });

  server.on("/api", [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "Api base route");
  });

  server.serveStatic("/", SD, frontendRootPath.c_str()).setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404, "text", "Sorry, this page does not exist!");
    }
  });

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  /*
  AsyncCallbackJsonWebHandler *scheduleJsonHandler = new AsyncCallbackJsonWebHandler("/api/schedule", HTTP_POST, [](AsyncWebServerRequest *request, JsonVariant &json) {
    JsonObject &jsonObj = json.as<JsonObject>();
    Serial.println("Received Schedule: ");
    jsonObj.prettyPrintTo(Serial);
    // ...
  });
  server.addHandler(scheduleJsonHandler);*/

  server.begin();
  Serial.println("Web Server started");
  xTaskCreate(clearClientsTask, "ClearClientsTask", 4096, NULL, 1, &clearClientsTaskHandle);
  return true;
}

bool NetworkController::hasWebClients() {
  return ws.count() > 0;
}

void NetworkController::broadcast(const char *serializedMessage) {
  ws.textAll(serializedMessage);
}
/* === End of NetworkController.cpp === */

/* === Start of NetworkController.h === */
#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include "Models.h"
#include "DataAccess.h"

extern DataAccess dataAccess;
extern Schedule* selectedSchedule;
extern UserSettings* userSettings;

class NetworkController {
public:
  bool initNetworkConnection(Config* config);

  bool initNTP();
  long getCurrentTime();

  bool initWebserver();
  bool hasWebClients();
  void broadcast(const char* serializedMessage);
};

#endif
/* === End of NetworkController.h === */

/* === Start of tgrbxsvr.ino === */
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

DataAccess dataAccess;
MachineController machineController;
NetworkController networkController;

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

const int MOTOR_CHECK_WAIT = 2; //seconds
TaskHandle_t motorOperationCheckHandle;
MotorCheckParams motorCheckParams;

void motorOperationCheckTask(void *pvParameters) {
  vTaskDelay(pdMS_TO_TICKS(MOTOR_CHECK_WAIT * 1000));

  const double container_D = currentStatus->ContainerLoad - motorCheckParams.ContainerLoad;

  if(motorCheckParams.Open){
    if(container_D / (double) MOTOR_CHECK_WAIT > -1 * WEIGHT_D_THRESHOLD){
      currentStatus->MotorOperation = false;
    }else{
      currentStatus->MotorOperation = true;
    }
  }else{
    if(container_D <= -1 * WEIGHT_D_THRESHOLD){
      currentStatus->MotorOperation = false;
    }else{
      currentStatus->MotorOperation = true;
    }
  }
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
    if (currentStatus->ContainerLoad > CONTAINER_EMPTY_THRESHOLD) {
      if (!currentStatus->Open) {
        //start feeding
        currentFeedTargetWeight = min(userSettings->PlateFilling, currentStatus->PlateLoad + currentStatus->ContainerLoad) - SAFETY_WEIGHT_GAP;
        openContainer();
      } else if (currentStatus->PlateLoad >= currentFeedTargetWeight || currentStatus->ContainerLoad <= CONTAINER_EMPTY_THRESHOLD) {
        //finished feeding
        lastFedTimestamp = currentTimestamp;
        currentFeedTargetWeight = 0;
        numTimesFedToday++;
        if (selectedSchedule != nullptr && selectedSchedule->Mode == MaxTimes && numTimesFedToday == selectedSchedule->MaxTimes) {
          //set wait timestamp tomorrow
        }
        closeContainer();
        //log history: eventtype = feed, currentTimestamp
      } else if (!currentStatus->MotorOperation) {
        //abort feeding
        lastFedTimestamp = currentTimestamp;
        currentFeedTargetWeight = 0;
        closeContainer();
        //log history: eventtype = missedFeed, currentTimestamp
      }
    }
  }

  const SignificantWeightChange significantChange = weightDifferenceSignificant();
  updateLoopFrequency(significantChange);
  handleCurrentData(significantChange);
  handleNotifications();

  previousTimestamp = currentTimestamp;
  previousStatus = currentStatus;
  delay(1 / CURRENT_LOOP_FREQ);
}

ScaleData createScaleDataHistory(int scaleID, double value) {
  ScaleData data;
  data.CreatedOn = currentTimestamp;
  data.ScaleID = scaleID;
  data.Value = value;
  return data;
}

void openContainer() {
  machineController.openContainer();
  currentStatus->Open = true;

  if(eTaskGetState(motorOperationCheckHandle) != eDeleted){
    vTaskDelete(motorOperationCheckHandle);
  }
  motorCheckParams.ContainerLoad = currentStatus->ContainerLoad;
  motorCheckParams.Open = true;
  xTaskCreate(motorOperationCheckTask, "MotorOperation Open Check", 4096, NULL, 1, &motorOperationCheckHandle);
}

void closeContainer() {
  machineController.closeContainer();
  currentStatus->Open = false;

  if(eTaskGetState(motorOperationCheckHandle) != eDeleted){
    vTaskDelete(motorOperationCheckHandle);
  }
  motorCheckParams.ContainerLoad = currentStatus->ContainerLoad;
  motorCheckParams.Open = false;
  xTaskCreate(motorOperationCheckTask, "MotorOperation Open Check", 4096, NULL, 1, &motorOperationCheckHandle);
}

MachineStatus getUpdatedStatus() {
  MachineStatus status = *previousStatus;
  status.ContainerLoad = machineController.getContainerLoad();
  status.PlateLoad = machineController.getPlateLoad();
  return status;
}

SignificantWeightChange weightDifferenceSignificant() {
  const double time_D = currentTimestamp - previousTimestamp;
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
  } else if (currentTimestamp - noStatusChangeTimestamp > COOLDOWN_TIME) {
    CURRENT_LOOP_FREQ = LOOP_FREQ_NORMAL;
  }
}

void handleCurrentData(SignificantWeightChange significantChange) {
  const bool clientsAvailable = networkController.hasWebClients();

  ArduinoJson::DynamicJsonDocument doc(1024);
  doc["status"] = serializeStatus(*currentStatus);
  ArduinoJson::JsonObject history = doc.createNestedObject("history");
  ArduinoJson::JsonArray schedules = history.createNestedArray("schedules");
  ArduinoJson::JsonArray events = history.createNestedArray("events");
  ArduinoJson::JsonArray scaleData = history.createNestedArray("scaleData");

  switch (significantChange) {
    case OnlyContainer:
      {
        ScaleData data = createScaleDataHistory(0, currentStatus->ContainerLoad);
        containerScaleHistoryBuffer.push_back(data);
        String serializedData = serializeScaleData(data);
        scaleData.add(serializedData);
        break;
      }
    case OnlyPlate:
      {
        ScaleData data = createScaleDataHistory(1, currentStatus->PlateLoad);
        plateScaleHistoryBuffer.push_back(data);
        String serializedData = serializeScaleData(data);
        scaleData.add(serializedData);
        break;
      }
    case Both:
      {
        ScaleData data_A = createScaleDataHistory(0, currentStatus->ContainerLoad);
        containerScaleHistoryBuffer.push_back(data_A);
        String serializedData_A = serializeScaleData(data_A);
        scaleData.add(serializedData_A);
        ScaleData data_B = createScaleDataHistory(1, currentStatus->PlateLoad);
        plateScaleHistoryBuffer.push_back(data_B);
        String serializedData_B = serializeScaleData(data_B);
        scaleData.add(serializedData_B);
        break;
      }
  }

  if (clientsAvailable) {
    String serialized;
    serializeJson(doc, serialized);
    networkController.broadcast(serialized.c_str());
  }

  //save and clear history buffers if possible/necessary
  if (CURRENT_LOOP_FREQ == LOOP_FREQ_NORMAL) {
    if (!containerScaleHistoryBuffer.empty()) {
    }
    if (!plateScaleHistoryBuffer.empty()) {
    }
  } else {
    if (containerScaleHistoryBuffer.size() >= MAX_HISTORY_BUFFER) {
    }
    if (plateScaleHistoryBuffer.size() >= MAX_HISTORY_BUFFER) {
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
  if (selectedSchedule == nullptr || !selectedSchedule->Active) {
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

bool dayChanged() {
  return getDay(currentTimestamp) > getDay(previousTimestamp);
}

long getDay(long timestamp) {
  return timestamp / 86400L;
}
/* === End of tgrbxsvr.ino === */

/* === End of output.cpp === */

