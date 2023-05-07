#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <vector>

struct Config {
  char ssid[64];
  char password[64];
  char localDomainName[64];
};

struct SystemSettings{
  int CalibrationWeight;
  double ContainerScale;
  int ContainerOffset;
  double PlateScale;
  int PlateOffset;
};

struct Notification{
  bool Active;
  bool Email;
  bool Phone;
};

struct NotificationList{
  Notification ContainerEmpty;
  Notification DidNotEatInADay;
};

struct UserSettings {
  String PetName;
  double PlateTAR;
  double PlateFilling;
  NotificationList Notifications;
  String Email;
  String Phone;
  int Language;
  int Theme;
};

struct MachineStatus{
  double ContainerLoad;
  double PlateLoad;
  bool SDCardConnection;
  bool MotorOperation;
  bool WiFiConnection;
};

struct Schedule{
  int ID;
  long CreatedOn;
  String Name;
  int Mode;
  bool Selected;
  bool Active;
  std::vector<long> Daytimes;
  int MaxTimes;
  long MaxTimesStartTime;
  bool OnlyWhenEmpty;
};

#endif