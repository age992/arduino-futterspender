#ifndef DATAACESS_H
#define DATAACESS_H

#include "Models.h"
#include <vector>


extern void printRam();

class DataAccess {

public:
  bool init();
  Config *getConfig();

  //--- DB: System ---

  SystemSettings *getSystemSettings();

  //--- DB: History ---

  int getNumFedFromTo(long from, long to);
  long getLastFedTimestampBefore(long before);
  bool logEventHistory(Event event);
  bool logScaleHistory(std::vector<ScaleData>& scaleData);
  bool logScheduleHistory(Schedule *schedule);

  //--- DB: User ---

  UserSettings *getUserSettings();
  bool updateUserSettings(UserSettings *userSettings);

  Schedule *getSelectedSchedule();
  bool setSelectSchedule(int scheduleID, bool selected);
  bool setActiveSchedule(int scheduleID, bool active);
  void getAllSchedules(std::vector<Schedule>& schedules);
  Schedule* getScheduleByID(int id);
  int insertSchedule(Schedule* schedule);
  bool updateSchedule(Schedule* schedule);
  bool deleteSchedule(int scheduleID);
  
};

#endif