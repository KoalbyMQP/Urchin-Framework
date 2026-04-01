//
// Created by gabri on 3/24/2025.
//

#ifndef LIMITCHECKS_H
#define LIMITCHECKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Global/Bridge.h"

    typedef struct {
        unsigned int RLower;
        unsigned int RUpper;

        double ALower;
        double AUpper;
    }AngleSet;


    int BridgeBounds(BridgeMotor motor, int angle);
    double RoundPress(double value, int tenth);
    int AngleToPoint(AngleSet motor, double angle);
    double PointToAngle(AngleSet motor, int point);



#ifdef __cplusplus
}
#endif


#endif //LIMITCHECKS_H
