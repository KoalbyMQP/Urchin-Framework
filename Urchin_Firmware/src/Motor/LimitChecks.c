//
// Created by gabri on 3/24/2025.
//

#include "LimitChecks.h"
#include <math.h>





int BridgeBounds(BridgeMotor motor, int angle) {
    // Check bounds
    if (angle < motor.BoundsMin || angle > motor.BoundsMax) {
        return -1;
    }
    return 0;
}

double RoundPress(double value, int tenth) {
    double jump = (double)tenth*10.0f;
    return round(value*jump)/jump;
}


int AngleToPoint(AngleSet motor, double angle) {

    // Check Input
    if (angle < motor.ALower) {
        return -1;
    }

    if ( angle > motor.AUpper) {
        return -1;
    }


    double normalizedAngle = (angle-motor.ALower)/(motor.AUpper-motor.ALower);
    // Clamp
    if (normalizedAngle < 0.0) normalizedAngle = 0.0;
    if (normalizedAngle > 1.0) normalizedAngle = 1.0;

    double ABS = (double)(motor.RLower + motor.RUpper);
    int value = (int)((normalizedAngle*ABS)+0.5f);
    return value;
}



double PointToAngle(AngleSet motor, int point) {
    if (point < 0 || point > 2047) {
        return NAN;
    }
    double ABS = (double)(motor.RLower + motor.RUpper);
    double normalizedAngle = point / ABS;
    // Clamp
    if (normalizedAngle < 0.0) normalizedAngle = 0.0;
    if (normalizedAngle > 1.0) normalizedAngle = 1.0;


    double angle = motor.ALower + normalizedAngle * (motor.AUpper - motor.ALower);

    return RoundPress(angle,1);

}