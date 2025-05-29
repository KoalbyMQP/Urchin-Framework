//
// Created by gabri on 5/23/2025.
//

#ifndef DEVICE_H
#define DEVICE_H

/**
 *
 */
class Device{
    virtual int (char buffer)=0;
};


/**
 *
 */
class Sensor: public Device{


};



/**
 *
 */
class Motor: public Device{


};

/**
 *
 */
class Dynamixel: public Motor{



};


/**
 *
 */
class Herkulex: public Motor{


};



#endif //DEVICE_H
