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