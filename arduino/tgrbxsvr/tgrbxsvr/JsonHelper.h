#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <Arduino.h>
#include <vector>
#include "Models.h"

String serializeStatus(MachineStatus data);
String serializeScaleData(ScaleData data);

String serializeNotification(Notification data);
String serializeDaytimes(std::vector<long> data);

#endif