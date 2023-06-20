#ifndef MACHINECONTROLLER_H
#define MACHINECONTROLLER_H

#include "Models.h"
#include "DataAccess.h"

extern DataAccess dataAccess;

extern SystemSettings* systemSettings;
extern UserSettings* userSettings;

class MachineController {
  public:
    bool initControls();
    void setContainerScaleCalibration(double scaleFactor, int offset );
    void setPlateScaleCalibration(double scaleFactor, int offset );
    bool calibrateContainerScale(double targetWeight);
    bool calibratePlateScale(double targetWeight);
    bool tareContainerScale();
    bool tarePlateScale();
    bool tarePlateScaleWithPlate();
    double getContainerLoad();
    double getPlateLoad();
    void openContainer();
    void closeContainer();
}; 

#endif