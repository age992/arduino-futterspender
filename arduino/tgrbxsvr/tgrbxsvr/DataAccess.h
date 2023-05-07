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
  Schedule *getSelectedSchedule();
  
};

#endif