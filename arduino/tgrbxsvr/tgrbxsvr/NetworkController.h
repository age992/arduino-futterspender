#ifndef NETWORKCONTROLLER_H
#define NETWORKCONTROLLER_H

#include "Models.h"

class NetworkController {
public:
  bool initNetworkConnection(Config config);

  bool initNTP();
  long getCurrentTime();

  bool initWebserver();
};

#endif