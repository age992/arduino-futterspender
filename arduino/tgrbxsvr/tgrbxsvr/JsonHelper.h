#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "Models.h"

String serializeStatus(MachineStatus data);
void setJsonStatus(MachineStatus* data, ArduinoJson::JsonObject statusObject);

String serializeScaleData(ScaleData data);
void setJsonScaleHistory(ScaleData* data, ArduinoJson::JsonObject dataObject);

Notification deserializeNotification(String data);
String serializeNotification(Notification data);
String serializeUserSettings(const UserSettings* settings);
UserSettings* deserializeUserSettings(const char* data); 

String serializeSchedule(Schedule data);
String serializeSchedules(const std::vector<Schedule>& data);
void setJsonSchedule(ArduinoJson::JsonObject scheduleObject, Schedule* schedule);
Schedule* deserializeSchedule(char* data);

String serializeDaytimes(const std::vector<long>& data);
std::vector<long> deserializeDaytimes(String data);

#endif