#include "JsonHelper.h"

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

void setJsonStatus(MachineStatus* data, ArduinoJson::JsonObject statusObject) {
  statusObject["ContainerLoad"] = data->ContainerLoad;
  statusObject["PlateLoad"] = data->PlateLoad;
  statusObject["Open"] = data->Open;
  statusObject["MotorOperation"] = data->MotorOperation;
  statusObject["SDCardConnection"] = data->SDCardConnection;
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

void setJsonScaleHistory(ScaleData* data, ArduinoJson::JsonObject dataObject) {
  dataObject["ID"] = data->ScaleID;
  dataObject["CreatedOn"] = data->CreatedOn;
  dataObject["ScaleID"] = data->ScaleID;
  dataObject["Value"] = data->Value;
}

String serializeNotification(Notification data) {
  String serialized;
  ArduinoJson::StaticJsonDocument<512> doc;
  doc["Active"] = data.Active;
  doc["Email"] = data.Email;
  doc["Phone"] = data.Phone;
  serializeJson(doc, serialized);
  return serialized;
};

void setJsonDaytimes(ArduinoJson::JsonArray& jsonArray, const std::vector<long>& data) { 
  for (long datetime : data) {
    jsonArray.add(datetime);
  }
}

String serializeDaytimes(const std::vector<long>& data) {
  Serial.println("Serialize daytimes");
  String serialized;
  ArduinoJson::StaticJsonDocument<1024> doc;
  ArduinoJson::JsonArray jsonArray = doc.to<JsonArray>();
  setJsonDaytimes(jsonArray, data);
  serializeJson(jsonArray, serialized);
  Serial.println(serialized);
  return serialized;
};

String serializeSchedules(const std::vector<Schedule>& data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(4096);
  ArduinoJson::JsonArray list = doc.createNestedArray("schedules");

  for (size_t i = 0; i < data.size(); i++) {
    Schedule schedule = data.at(i);
    ArduinoJson::JsonObject scheduleObject = list.createNestedObject();
    scheduleObject["ID"] = schedule.ID;
    scheduleObject["CreatedOn"] = schedule.CreatedOn;
    scheduleObject["Name"] = schedule.Name;
    scheduleObject["Mode"] = schedule.Mode;
    scheduleObject["Selected"] = schedule.Selected;
    scheduleObject["Active"] = schedule.Active;
    ArduinoJson::JsonArray daytimesArray = scheduleObject.createNestedArray("Daytimes");
    setJsonDaytimes(daytimesArray, schedule.Daytimes);
    scheduleObject["MaxTimes"] = schedule.MaxTimes;
    scheduleObject["MaxTimesStartTime"] = schedule.MaxTimesStartTime;
    scheduleObject["OnlyWhenEmpty"] = schedule.OnlyWhenEmpty;
  }

  serializeJson(doc, serialized);
  return serialized;
};

Schedule* deserializeSchedule(char* data) {
  ArduinoJson::DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.println(F("Failed to deserialize schedule"));
    return nullptr;
  };

  Schedule* schedule = new Schedule();
  schedule->ID = doc["ID"].as<int>();
  schedule->CreatedOn = doc["CreatedOn"].as<long>();
  schedule->Name = doc["Name"].as<String>();
  schedule->Mode = doc["Mode"].as<int>();
  schedule->Selected = doc["Selected"].as<bool>();
  schedule->Active = doc["Active"].as<bool>();
  schedule->Daytimes = deserializeDaytimes(doc["Daytimes"].as<String>());
  schedule->MaxTimes = doc["MaxTimes"].as<int>();
  schedule->MaxTimesStartTime = doc["MaxTimesStartTime"].as<long>();
  schedule->OnlyWhenEmpty = doc["OnlyWhenEmpty"].as<bool>();
  return schedule;
};

std::vector<long> deserializeDaytimes(String data) {
  std::vector<long> daytimes;
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, data);

  if (error) {
    Serial.println(F("Failed to deserialize daytimes for schedule"));
    Serial.println(error.c_str());
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