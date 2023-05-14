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
  statusObject["AutomaticFeeding"] = data->AutomaticFeeding;
  statusObject["ManualFeeding"] = data->ManualFeeding;
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

void setJsonDaytimes(ArduinoJson::JsonArray& jsonArray, const std::vector<long>& data) {
  for (long datetime : data) {
    jsonArray.add(datetime);
  }
}

String serializeDaytimes(const std::vector<long>& data) {
  String serialized;
  ArduinoJson::StaticJsonDocument<1024> doc;
  ArduinoJson::JsonArray jsonArray = doc.to<JsonArray>();
  setJsonDaytimes(jsonArray, data);
  serializeJson(jsonArray, serialized);
  return serialized;
};

String serializeSchedules(const std::vector<Schedule>& data) {
  String serialized;
  ArduinoJson::DynamicJsonDocument doc(4096);
  ArduinoJson::JsonArray list = doc.createNestedArray("schedules");

  for (size_t i = 0; i < data.size(); i++) {
    Schedule schedule = data.at(i);
    ArduinoJson::JsonObject scheduleObject = list.createNestedObject();
    setJsonSchedule(scheduleObject, &schedule);
  }

  serializeJson(doc, serialized);
  return serialized;
};

void setJsonSchedule(ArduinoJson::JsonObject scheduleObject, Schedule* schedule) {
  scheduleObject["ID"] = schedule->ID;
  scheduleObject["CreatedOn"] = schedule->CreatedOn;
  scheduleObject["Name"] = schedule->Name;
  scheduleObject["Mode"] = schedule->Mode;
  scheduleObject["Selected"] = schedule->Selected;
  scheduleObject["Active"] = schedule->Active;
  ArduinoJson::JsonArray daytimesArray = scheduleObject.createNestedArray("Daytimes");
  setJsonDaytimes(daytimesArray, schedule->Daytimes);
  scheduleObject["MaxTimes"] = schedule->MaxTimes;
  scheduleObject["MaxTimesStartTime"] = schedule->MaxTimesStartTime;
  scheduleObject["OnlyWhenEmpty"] = schedule->OnlyWhenEmpty;
}

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
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, data);
  Notification notification;

  if (error) {
    Serial.println(F("Failed to deserialize Notification"));
    return notification;
  };

  notification.Active = doc["Active"].as<bool>();
  notification.Email = doc["Email"].as<bool>();
  notification.Phone = doc["Phone"].as<bool>();
  Serial.print("Noti Active: ");
  Serial.println(notification.Active);
  return notification;
}

String serializeUserSettings(const UserSettings* settings) {
  // Create a DynamicJsonDocument
  DynamicJsonDocument jsonDoc(1024); // Adjust the size as per your requirements

  // Create JsonObject for UserSettings
  JsonObject userSettingsObj = jsonDoc.to<JsonObject>();

  // Set values in the UserSettings object
  userSettingsObj["PetName"] = settings->PetName;
  userSettingsObj["PlateTAR"] = settings->PlateTAR;
  userSettingsObj["PlateFilling"] = settings->PlateFilling;

  // Create JsonObject for Notifications
  JsonObject notificationsObj = userSettingsObj.createNestedObject("Notifications");
  notificationsObj["ContainerEmpty"]["Active"] = settings->Notifications.ContainerEmpty.Active;
  notificationsObj["ContainerEmpty"]["Email"] = settings->Notifications.ContainerEmpty.Email;
  notificationsObj["ContainerEmpty"]["Phone"] = settings->Notifications.ContainerEmpty.Phone;
  notificationsObj["DidNotEatInADay"]["Active"] = settings->Notifications.DidNotEatInADay.Active;
  notificationsObj["DidNotEatInADay"]["Email"] = settings->Notifications.DidNotEatInADay.Email;
  notificationsObj["DidNotEatInADay"]["Phone"] = settings->Notifications.DidNotEatInADay.Phone;

  userSettingsObj["Email"] = settings->Email;
  userSettingsObj["Phone"] = settings->Phone;
  userSettingsObj["Language"] = settings->Language;
  userSettingsObj["Theme"] = settings->Theme;

  // Serialize the UserSettings object to a JSON string
  String jsonString;
  serializeJson(userSettingsObj, jsonString);

  return jsonString;
}

UserSettings* deserializeUserSettings(const char* data) {
  // Create a DynamicJsonDocument
  DynamicJsonDocument jsonDoc(1024); // Adjust the size as per your requirements

  // Deserialize the JSON string
  DeserializationError error = deserializeJson(jsonDoc, data);

  // Check for any deserialization errors
  if (error) {
    Serial.print("Deserialization error: ");
    Serial.println(error.c_str());
    return nullptr; // Return nullptr if there's an error
  }

  // Create a UserSettings object dynamically
  UserSettings* settings = new UserSettings();

  // Get the JsonObject from the deserialized JSON
  JsonObject userSettingsObj = jsonDoc.as<JsonObject>();

  // Extract values from the JsonObject and assign them to the UserSettings object
  settings->PetName = userSettingsObj["PetName"].as<String>();
  settings->PlateTAR = userSettingsObj["PlateTAR"].as<double>();
  settings->PlateFilling = userSettingsObj["PlateFilling"].as<double>();

  JsonObject notificationsObj = userSettingsObj["Notifications"].as<JsonObject>();
  settings->Notifications.ContainerEmpty.Active = notificationsObj["ContainerEmpty"]["Active"].as<bool>();
  settings->Notifications.ContainerEmpty.Email = notificationsObj["ContainerEmpty"]["Email"].as<bool>();
  settings->Notifications.ContainerEmpty.Phone = notificationsObj["ContainerEmpty"]["Phone"].as<bool>();
  settings->Notifications.DidNotEatInADay.Active = notificationsObj["DidNotEatInADay"]["Active"].as<bool>();
  settings->Notifications.DidNotEatInADay.Email = notificationsObj["DidNotEatInADay"]["Email"].as<bool>();
  settings->Notifications.DidNotEatInADay.Phone = notificationsObj["DidNotEatInADay"]["Phone"].as<bool>();

  settings->Email = userSettingsObj["Email"].as<String>();
  settings->Phone = userSettingsObj["Phone"].as<String>();
  settings->Language = userSettingsObj["Language"].as<int>();
  settings->Theme = userSettingsObj["Theme"].as<int>();

  return settings;
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