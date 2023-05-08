#ifndef MODELS_H
#define MODELS_H

#include <Arduino.h>
#include <vector>

struct MachineStatus {
  double ContainerLoad;
  double PlateLoad;
  bool Open;
  bool SDCardConnection;
  bool MotorOperation;
  bool WiFiConnection;
};

struct MotorCheckParams {
  double ContainerLoad;
  bool Open;
};

enum SignificantWeightChange {
  None,
  OnlyContainer,
  OnlyPlate,
  Both
};

struct Config {
  char ssid[64];
  char password[64];
  char localDomainName[64];
};

//--- DB: System ---

struct SystemSettings {
  int CalibrationWeight;
  double ContainerScale;
  int ContainerOffset;
  double PlateScale;
  int PlateOffset;
};

//--- DB: History ---

enum EventType { Feed,
                 MissedFeed,
                 ContainerEmpty,
                 MotorFaliure,
                 WiFiConnectionLost,
                 WiFiConnectionReturned,
                 SDConnectionLost,
                 SDConnectionReturned
};

struct Event {
  int ID;
  long CreatedOn;
  EventType Type;
  String message;
};

struct ScaleData {
  int ID;
  long CreatedOn;
  int ScaleID;  //0 = scale_A, 1 = scale_B
  double Value;
};

//--- DB: User ---

enum ScheduleMode {
  FixedDaytime,
  MaxTimes
};

struct Schedule {  //also for history
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

struct Notification {
  bool Active;
  bool Email;
  bool Phone;
};

struct NotificationList {
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

#endif