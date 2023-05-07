#ifndef MACHINECONTROLLER_H
#define MACHINECONTROLLER_H

#include "Models.h"

class MachineController {
  public:
    bool initControls();
    void calibrateContainerScale(double scaleFactor, int offset );
    void calibratePlateScale(double scaleFactor, int offset );
    double getContainerLoad();
    double getPlateLoad();
    void openContainer();
    void closeContainer();
}; 

#endif