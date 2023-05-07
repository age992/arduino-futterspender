#include "DataAccess.h"
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <sqlite3.h>

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

  Config configObject;
  File file = SD.open(configPath);
  StaticJsonDocument<512> doc;

  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.println(F("Failed to deserialize config file"));
    return nullptr;
  };

  strlcpy(configObject.ssid, doc["ssid"], sizeof(configObject.ssid));
  strlcpy(configObject.password, doc["password"], sizeof(configObject.password));
  strlcpy(configObject.localDomainName, doc["localDomainName"], sizeof(configObject.localDomainName));
  file.close();

  return &configObject;
}

//--- DB: System ---

SystemSettings *DataAccess::getSystemSettings() {
  SystemSettings settings;

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbSystem, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings.CalibrationWeight = sqlite3_column_int(stmt, 0);
    settings.ContainerScale = sqlite3_column_double(stmt, 1);
    settings.ContainerOffset = sqlite3_column_int(stmt, 2);
    settings.PlateScale = sqlite3_column_double(stmt, 3);
    settings.PlateOffset = sqlite3_column_int(stmt, 4);
  }

  sqlite3_finalize(stmt);
  return &settings;
}

//--- DB: History ---

int DataAccess::getNumFedFromTo(long from, long to) {
  return 1;
}

long DataAccess::getLastFedTimestampBefore(long before) {
  return 1;
}

//--- DB: User ---

UserSettings *DataAccess::getUserSettings() {
  UserSettings settings;

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings.PetName = sqlite3_column_string(stmt, 0);
    settings.PlateTAR = sqlite3_column_double(stmt, 1);
    settings.PlateFilling = sqlite3_column_double(stmt, 2);

    String notiListString = sqlite3_column_string(stmt, 3);  //deserialize json
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, notiListString.c_str());

    if (error) {
      Serial.println(F("Failed to deserialize daytimes for active schedule"));
      return nullptr;
    };

    NotificationList notificationList;
    notificationList.ContainerEmpty.Active = doc["ContainerEmpty"]["Active"].as<bool>();
    notificationList.ContainerEmpty.Email = doc["ContainerEmpty"]["Email"].as<bool>();
    notificationList.ContainerEmpty.Phone = doc["ContainerEmpty"]["Phone"].as<bool>();
    notificationList.DidNotEatInADay.Active = doc["DidNotEatInADay"]["Active"].as<bool>();
    notificationList.DidNotEatInADay.Email = doc["DidNotEatInADay"]["Email"].as<bool>();
    notificationList.DidNotEatInADay.Phone = doc["DidNotEatInADay"]["Phone"].as<bool>();

    settings.Notifications = notificationList;
    settings.Email = sqlite3_column_string(stmt, 4);
    settings.Phone = sqlite3_column_string(stmt, 5);
    settings.Language = sqlite3_column_int(stmt, 6);
    settings.Theme = sqlite3_column_int(stmt, 7);
  }

  sqlite3_finalize(stmt);
  return &settings;
}

Schedule *DataAccess::getSelectedSchedule() {
  Schedule schedule;

  char *sql = "SELECT * FROM Schedules LIMIT 1 WHERE Selected=1";
  sqlite3_stmt *stmt;

  rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedule.ID = sqlite3_column_int(stmt, 0);
    schedule.CreatedOn = sqlite3_column_int64(stmt, 1);
    schedule.Name = sqlite3_column_string(stmt, 2);
    schedule.Mode = sqlite3_column_int(stmt, 3);
    schedule.Selected = sqlite3_column_int(stmt, 4) == 1;
    schedule.Active = sqlite3_column_int(stmt, 5) == 1;

    String daytimesString = sqlite3_column_string(stmt, 6);
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, daytimesString.c_str());

    if (error) {
      Serial.println(F("Failed to deserialize daytimes for active schedule"));
      return nullptr;
    };

    JsonArray jsonArray = doc.as<JsonArray>();
    std::vector<long> daytimes;

    for (JsonVariant v : jsonArray) {
      daytimes.push_back(v.as<long>());
    }

    schedule.Daytimes = daytimes;
    schedule.MaxTimes = sqlite3_column_int(stmt, 7);
    schedule.MaxTimesStartTime = sqlite3_column_int64(stmt, 8);
    schedule.OnlyWhenEmpty = sqlite3_column_int(stmt, 9) == 1;
  }

  sqlite3_finalize(stmt);
  return &schedule;
}