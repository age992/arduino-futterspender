#ifndef DATAACESS_H
#define DATAACESS_H

#include "Models.h"
#include <vector>

class DataAccess {

public:
  bool init();
  Config *getConfig();

  //--- DB: System ---

  SystemSettings *getSystemSettings();

  //--- DB: History ---

  int getNumFedFromTo(long from, long to);
  long getLastFedTimestampBefore(long before);

  //--- DB: User ---

  UserSettings *getUserSettings();
  bool updateUserSettings(UserSettings userSettings);

  Schedule *getSelectedSchedule();
  bool setSelectSchedule(int scheduleID, bool selected);
  bool setActiveSchedule(int scheduleID, bool active);
  int insertSchedule(Schedule schedule);
  bool updateSchedule(Schedule schedule);
  bool deleteSchedule(Schedule schedule);
  
};

#endif