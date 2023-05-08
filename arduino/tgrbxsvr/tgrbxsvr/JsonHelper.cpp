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