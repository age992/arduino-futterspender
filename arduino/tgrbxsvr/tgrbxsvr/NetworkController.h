#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include "Models.h"
#include "DataAccess.h"
#include "MachineController.h"

extern DataAccess dataAccess;
extern MachineController machineController;
extern Schedule* selectedSchedule;
extern UserSettings* userSettings;

extern void setSchedule(Schedule* newSchedule);
extern long getDay(long timestamp);

class NetworkController {
public:
  bool initNetworkConnection(Config* config);

  bool initNTP();
  long getCurrentDaytime();
  int getToday();

  bool initWebserver();
  bool hasWebClients();
  void broadcast(const char* serializedMessage);
};

#endif