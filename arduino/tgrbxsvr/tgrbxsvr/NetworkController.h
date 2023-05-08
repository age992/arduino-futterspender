#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include "Models.h"
#include "DataAccess.h"

class NetworkController {

private:
  DataAccess& dataAccess;
public:
  NetworkController(DataAccess& _dataAccess);
  bool initNetworkConnection(Config config);

  bool initNTP();
  long getCurrentTime();

  bool initWebserver();
  bool hasWebClients();
  void broadcast(const char* serializedMessage);
};

#endif