#include "DataAccess.h"
#include "JsonHelper.h"
#include <FS.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>

bool initialized = false;

const int SD_CS_PIN = 32;
const int SD_CLK_PIN = 33;
const int SD_MISO_PIN = 26;
const int SD_MOSI_PIN = 25;

SPIClass SPI_custom(HSPI);

const char *configPath = "/config.json";
const String dbPrefix = "/sd";
const String dbRootPath = "/data";

String systemDbPath = dbPrefix + dbRootPath + "/System.db";
String historyDbPath = dbPrefix + dbRootPath + "/History.db";
String userDbPath = dbPrefix + dbRootPath + "/User.db";

sqlite3 *dbSystem;
sqlite3 *dbHistory;
sqlite3 *dbUser;

int openDb(const char *filename, sqlite3 **db) {
  int rc = sqlite3_open(filename, db);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(*db));
    return rc;
  } else {
    //Serial.printf("Opened database successfully\n");
  }
  return rc;
}

const char *sqlite3_column_string(sqlite3_stmt *stmt, int index) {
  return reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
}

int insertScheduleToDB(Schedule *schedule, sqlite3 *db) {
  const char *sql = "INSERT INTO Schedules (CreatedOn, Name, Mode, Selected, Active, Daytimes, MaxTimes, MaxTimesStartTime, OnlyWhenEmpty) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

  sqlite3_bind_int64(stmt, 1, schedule->CreatedOn);
  sqlite3_bind_text(stmt, 2, schedule->Name.c_str(), strlen(schedule->Name.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, schedule->Mode);
  sqlite3_bind_int(stmt, 4, schedule->Active ? 1 : 0);
  sqlite3_bind_int(stmt, 5, schedule->Selected ? 1 : 0);
  String dayTimesString = serializeDaytimes(schedule->Daytimes);
  sqlite3_bind_text(stmt, 6, dayTimesString.c_str(), strlen(dayTimesString.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 7, schedule->MaxTimes);
  sqlite3_bind_int64(stmt, 8, schedule->MaxTimesStartTime);
  sqlite3_bind_int(stmt, 9, schedule->OnlyWhenEmpty ? 1 : 0);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(db));
  }

  rc = sqlite3_finalize(stmt);
  return sqlite3_last_insert_rowid(db);
}

bool DataAccess::init() {
  if (initialized) {
    return true;
  }

  SPI_custom.begin(SD_CLK_PIN, SD_MISO_PIN,SD_MOSI_PIN, SD_CS_PIN);

  pinMode(SD_CS_PIN, OUTPUT);

  while (!SD.begin(SD_CS_PIN, SPI_custom)) {
    Serial.println("Trying to initialize SD Card...");
    delay(3000);
  }

  Serial.println("SD card initialized.");
  Serial.println("Init DB...");

  sqlite3_initialize();

  initialized = true;
  return true;
}

Config *DataAccess::getConfig() {
  if (!SD.exists(configPath)) {
    Serial.println(F("Config file not found"));
    return nullptr;
  }

  Config *configObject = new Config();
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
  if (openDb(systemDbPath.c_str(), &dbSystem)) {
    Serial.println("Couldnt open dbSystem!");
    return nullptr;
  }

  SystemSettings *settings = new SystemSettings();

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbSystem, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings->CalibrationWeight = sqlite3_column_int(stmt, 0);
    settings->ContainerScale = sqlite3_column_double(stmt, 1);
    settings->ContainerOffset = sqlite3_column_int(stmt, 2);
    settings->PlateScale = sqlite3_column_double(stmt, 3);
    settings->PlateOffset = sqlite3_column_int(stmt, 4);
    settings->ContainerAngleClose = sqlite3_column_int(stmt, 5);
    settings->ContainerAngleOpen = sqlite3_column_int(stmt, 6);
  } else {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbSystem));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbSystem);
  return settings;
}

bool DataAccess::updateSystemSettings(SystemSettings* settings){
  if (openDb(systemDbPath.c_str(), &dbSystem)) {
    Serial.println("Couldnt open dbSystem!");
    return false;
  }
  
  char *sql = "UPDATE Settings "
              "SET CalibrationWeight = ?,"
              "ContainerScale = ?,"
              "ContainerOffset = ?,"
              "PlateScale = ?,"
              "PlateOffset = ?,"
              "ContainerAngleClose = ?,"
              "ContainerAngleOpen = ? ";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbSystem, sql, -1, &stmt, NULL);

  sqlite3_bind_int(stmt, 1, settings->CalibrationWeight);
  sqlite3_bind_double(stmt, 2, settings->ContainerScale);
  sqlite3_bind_int(stmt, 3, settings->ContainerOffset);
  sqlite3_bind_double(stmt, 4, settings->PlateScale);
  sqlite3_bind_int(stmt, 5, settings->PlateOffset);
   sqlite3_bind_double(stmt, 6, settings->ContainerAngleClose);
    sqlite3_bind_double(stmt, 7, settings->ContainerAngleOpen);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbSystem));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbSystem);
  return rc == SQLITE_OK;
};

//--- DB: History ---

int DataAccess::getNumFedFromTo(long from, long to) {
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    Serial.println("Couldnt open dbHistory!");
    return 0;
  }
  int count = 0;

  char *sql = "SELECT COUNT(*) FROM Events WHERE Type = ? AND CreatedOn >= ? AND CreatedOn < ?";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, Feed);
  rc = sqlite3_bind_int64(stmt, 2, from);
  rc = sqlite3_bind_int64(stmt, 3, to);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    count = sqlite3_column_int(stmt, 0);
  } else {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbHistory);
  return count;
}

long DataAccess::getLastFedTimestampBefore(long before) {
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    Serial.println("Couldnt open dbHistory!");
    return 0;
  }

  long lastFedTimestamp = 0;

  char *sql = "SELECT CreatedOn FROM Events WHERE Type = ? AND CreatedOn <= ? LIMIT 1";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, Feed);
  rc = sqlite3_bind_int64(stmt, 2, before);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    String lastFedTimestampString = sqlite3_column_string(stmt, 0);
    lastFedTimestamp = std::stol(lastFedTimestampString.c_str());
  } else {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbHistory);
  return lastFedTimestamp;
}

bool DataAccess::logEventHistory(Event event) {
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    Serial.println("Couldnt open dbHistory!");
    return 0;
  }

  const char *sql = "INSERT INTO Events (CreatedOn, Type) VALUES (?, ?)";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);

  sqlite3_bind_int64(stmt, 1, event.CreatedOn);
  sqlite3_bind_int(stmt, 2, event.Type);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbHistory);
  return rc == SQLITE_DONE;
};

bool DataAccess::logScaleHistory(std::vector<ScaleData> &scaleData) {
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    Serial.println("Couldnt open dbHistory!");
    return 0;
  }

  int rc = SQLITE_OK;
  sqlite3_stmt *stmt;
  const char *sql = "INSERT INTO ScaleData (CreatedOn, ScaleID, Value) VALUES (?, ?, ?)";
  rc = sqlite3_prepare_v2(dbHistory, sql, -1, &stmt, NULL);

  for (ScaleData data : scaleData) {
    sqlite3_bind_int64(stmt, 1, data.CreatedOn);
    sqlite3_bind_int(stmt, 2, data.ScaleID);
    sqlite3_bind_double(stmt, 3, data.Value);
    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
      Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbHistory));
    }
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbHistory);
  return rc == SQLITE_DONE;
};

bool DataAccess::logScheduleHistory(Schedule *schedule) {
  if (openDb(historyDbPath.c_str(), &dbHistory)) {
    Serial.println("Couldnt open dbHistory!");
    return 0;
  }
  int newID = insertScheduleToDB(schedule, dbHistory);
  sqlite3_close(dbHistory);
  return newID;
};

//--- DB: User ---

UserSettings *DataAccess::getUserSettings() {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return nullptr;
  }

  UserSettings *settings = new UserSettings();

  char *sql = "SELECT * FROM Settings LIMIT 1";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    settings->PetName = sqlite3_column_string(stmt, 0);
    settings->PlateTAR = sqlite3_column_double(stmt, 1);
    settings->PlateFilling = sqlite3_column_double(stmt, 2);
    String notiContainerEmptyString = sqlite3_column_string(stmt, 3);
    Notification notiContainerEmpty = deserializeNotification(notiContainerEmptyString);
    settings->Notifications.ContainerEmpty = notiContainerEmpty;
    String notiDidNotEatString = sqlite3_column_string(stmt, 4);
    Notification notiDidNotEat = deserializeNotification(notiDidNotEatString);
    settings->Notifications.DidNotEatInADay = notiDidNotEat;
    Serial.print("Noti eat Active: ");
    Serial.println(settings->Notifications.DidNotEatInADay.Active);
    settings->Email = sqlite3_column_string(stmt, 5);
    settings->Phone = sqlite3_column_string(stmt, 6);
    settings->Language = sqlite3_column_int(stmt, 7);
    settings->Theme = sqlite3_column_int(stmt, 8);
  } else {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return settings;
}

bool DataAccess::updateUserSettings(UserSettings *userSettings) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return false;
  }

  char *sql = "UPDATE Settings "
              "SET PetName = ?,"
              "PlateTAR = ?,"
              "PlateFilling = ?,"
              "NotificationContainerEmpty = ?,"
              "NotificationDidNotEatInADay = ?,"
              "Email = ?,"
              "Phone = ?,"
              "Language = ?,"
              "Theme = ? ";

  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  sqlite3_bind_text(stmt, 1, userSettings->PetName.c_str(), strlen(userSettings->PetName.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 2, userSettings->PlateTAR);
  sqlite3_bind_double(stmt, 3, userSettings->PlateFilling);
  String serializedNotiContEmpty = serializeNotification(userSettings->Notifications.ContainerEmpty);
  sqlite3_bind_text(stmt, 4, serializedNotiContEmpty.c_str(), strlen(serializedNotiContEmpty.c_str()), SQLITE_TRANSIENT);
  String serializedNotiDidNotEat = serializeNotification(userSettings->Notifications.DidNotEatInADay);
  sqlite3_bind_text(stmt, 5, serializedNotiDidNotEat.c_str(), strlen(serializedNotiDidNotEat.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 6, userSettings->Email.c_str(), strlen(userSettings->Email.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 7, userSettings->Phone.c_str(), strlen(userSettings->Phone.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 8, userSettings->Language);
  sqlite3_bind_int(stmt, 9, userSettings->Theme);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return rc == SQLITE_OK;
}

Schedule *readScheduleRow(sqlite3_stmt *stmt) {
  Schedule *schedule = new Schedule();
  schedule->ID = sqlite3_column_int(stmt, 0);
  schedule->CreatedOn = sqlite3_column_int64(stmt, 1);
  schedule->Name = sqlite3_column_string(stmt, 2);
  schedule->Mode = sqlite3_column_int(stmt, 3);
  schedule->Selected = sqlite3_column_int(stmt, 4) == 1;
  schedule->Active = sqlite3_column_int(stmt, 5) == 1;
  String daytimesString = sqlite3_column_string(stmt, 6);
  schedule->Daytimes = deserializeDaytimes(daytimesString);
  schedule->MaxTimes = sqlite3_column_int(stmt, 7);
  schedule->MaxTimesStartTime = sqlite3_column_int64(stmt, 8);
  schedule->OnlyWhenEmpty = sqlite3_column_int(stmt, 9) == 1;
  return schedule;
}

Schedule *DataAccess::getSelectedSchedule() {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return nullptr;
  }

  Schedule *schedulePointer = nullptr;

  char *sql = "SELECT * FROM Schedules WHERE Selected=1 LIMIT 1";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedulePointer = readScheduleRow(stmt);
  } else {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return schedulePointer;
}

bool DataAccess::setSelectSchedule(int scheduleID, bool selected) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return false;
  }

  sqlite3_stmt *stmt;
  const char *sql = "UPDATE Schedules "
                    "SET Selected = ? "
                    "WHERE ID = ?";

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  rc = sqlite3_bind_int(stmt, 1, selected ? 1 : 0);
  rc = sqlite3_bind_int(stmt, 2, scheduleID);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return rc == SQLITE_OK;
}

bool DataAccess::setActiveSchedule(int scheduleID, bool active) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return false;
  }

  sqlite3_stmt *stmt;
  const char *sql = "UPDATE Schedules "
                    "SET Active = ? "
                    "WHERE ID = ?";

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  rc = sqlite3_bind_int(stmt, 1, active ? 1 : 0);
  rc = sqlite3_bind_int(stmt, 2, scheduleID);

  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return rc == SQLITE_OK;
}

void DataAccess::getAllSchedules(std::vector<Schedule> &schedules) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
  }

  char *sql = "SELECT * FROM Schedules";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  do {
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
      Schedule *schedulePointer = readScheduleRow(stmt);
      if (schedulePointer != nullptr) {
        schedules.push_back(*schedulePointer);
      }
    }
  } while (rc == SQLITE_ROW);

  if (!(rc == SQLITE_OK || rc == SQLITE_DONE)) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
}

Schedule *DataAccess::getScheduleByID(int id) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return nullptr;
  }

  Schedule *schedulePointer = nullptr;

  char *sql = "SELECT * FROM Schedules WHERE ID = ? LIMIT 1";
  sqlite3_stmt *stmt;

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, id);
  rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    schedulePointer = readScheduleRow(stmt);
  } else {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return schedulePointer;
}

int DataAccess::insertSchedule(Schedule *schedule) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return 0;
  }
  int newID = insertScheduleToDB(schedule, dbUser);
  sqlite3_close(dbUser);
  return newID;
}

bool DataAccess::updateSchedule(Schedule *schedule) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return false;
  }

  const char *sql = "UPDATE Schedules "
                    "SET CreatedOn=?, Name=?, Mode=?, Selected=?, Active=?, Daytimes=?, MaxTimes=?, MaxTimesStartTime=?, OnlyWhenEmpty=? "
                    "WHERE ID=?";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);

  sqlite3_bind_int64(stmt, 1, schedule->CreatedOn);
  sqlite3_bind_text(stmt, 2, schedule->Name.c_str(), strlen(schedule->Name.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 3, schedule->Mode);
  sqlite3_bind_int(stmt, 4, schedule->Active ? 1 : 0);
  sqlite3_bind_int(stmt, 5, schedule->Selected ? 1 : 0);
  String dayTimesString = serializeDaytimes(schedule->Daytimes);
  sqlite3_bind_text(stmt, 6, dayTimesString.c_str(), strlen(dayTimesString.c_str()), SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 7, schedule->MaxTimes);
  sqlite3_bind_int64(stmt, 8, schedule->MaxTimesStartTime);
  sqlite3_bind_int(stmt, 9, schedule->OnlyWhenEmpty ? 1 : 0);
  sqlite3_bind_int(stmt, 10, schedule->ID);

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return rc == SQLITE_OK;
}

bool DataAccess::deleteSchedule(int scheduleID) {
  if (openDb(userDbPath.c_str(), &dbUser)) {
    Serial.println("Couldnt open dbUser!");
    return false;
  }

  sqlite3_stmt *stmt;
  const char *sql = "DELETE FROM Schedules WHERE ID = ?";

  int rc = sqlite3_prepare_v2(dbUser, sql, -1, &stmt, NULL);
  rc = sqlite3_bind_int(stmt, 1, scheduleID);
  rc = sqlite3_step(stmt);

  if (rc != SQLITE_DONE) {
    Serial.printf("ERROR executing stmt: %s\n", sqlite3_errmsg(dbUser));
  }

  rc = sqlite3_finalize(stmt);
  sqlite3_close(dbUser);
  return rc == SQLITE_OK;
}