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

String serializeNotification(Notification data);
String serializeSchedule(Schedule data);
String serializeDaytimes(const std::vector<long>& data);
String serializeSchedules(const std::vector<Schedule>& data);

Schedule* deserializeSchedule(char* data);
std::vector<long> deserializeDaytimes(String data);

Notification deserializeNotification(String data);

#endif