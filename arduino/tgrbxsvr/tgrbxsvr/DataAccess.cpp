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