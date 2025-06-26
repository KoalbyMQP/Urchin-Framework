
/*

 Hekulex.h - Library for Dongbu Herkulex DRS-0101/DRS-0201 
 Copyright (c) 2012 - http://robottini.altervista.org
 Created by Alessandro on 09/12/2012.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,  
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 Herkulex.h.ignore
    PLEASE START READING: Herkulex Servo Manual (http://www.hovis.co.kr/guide/herkulexeng.pdf)
 *****************************************************************************
 
 IMPORTANT:

  The library works on Arduino UNO/2009 - Arduino Mega.
  Please with Arduino UNO/2009 works with SoftwareSerial library modified with baud rate 57.600.
  Use this begin type:
		begin(57600, int rx, int tx);
 
  For Arduino Mega, please use baud rate 115.200

 *****************************************************************************
 Contact: alegiaco@gmail.com
 Web:     http://robottini.altervista.org
 Autor:   Alessandro Giacomel
 Updater: Gabriel Weaver 6/23/2025
 *****************************************************************************  
*/

#ifdef __cplusplus

#ifndef Herkulex_h
#define Herkulex_h

//
// #if defined(ARDUINO) && ARDUINO >= 100  // Arduino IDE Version
// #include "Arduino.h"
// #else
// #include "WProgram.h"
// #endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "DataTypes.h"



#define DATA_SIZE	 30		// buffer for input data
#define DATA_MOVE  	 50		// max 10 servos <---- change this for more servos!
#define TIME_OUT     5   	//timeout serial communication

// SERVO HERKULEX COMMAND - See Manual p40
#define HEEPWRITE    0x01 	//Rom write
#define HEEPREAD     0x02 	//Rom read
#define HRAMWRITE	 0x03 	//Ram write
#define HRAMREAD	 0x04 	//Ram read
#define HINDEJOG		 0x05 	//Write n servo with different timing
#define HSIMULJOG		 0x06 	//Write n servo with same time
#define HSTAT	 	 0x07 	//Read error
#define HROLLBACK	 0x08 	//Back to factory value
#define HREBOOT	 	 0x09 	//Reboot

// HERKULEX LED - See Manual p29
// static int LED_GREEN =	 0x01;
// static int LED_BLUE  =   0x02;
// static int LED_CYAN  =   0x03;
// static int LED_RED   = 	 0x04;
// static int LED_GREEN2= 	 0x05;
// static int LED_PINK  =   0x06;
// static int LED_WHITE =   0x07;




enum RAMObject {
  Voltage,
  Temperature,
  ControlMode,
  Tick,
  CalibrationPosition,
  AbsolutePosition,
  DifferentialPosition,
  PWM,
  Absolute2ndPosition,
  AbsoluteGoalPosition,
  DesiredTrajectoryPosition,
  DesiredVelocity,
  RAMObjectCount,
  StatusAll
};

typedef struct  {
  uint8_t address;
  uint8_t length;
  //const char* comment;
}RAMInfo;

enum LedColor {
  LED_GREEN_HRAMWRITE  = 0x01,
  LED_BLUE_HRAMWRITE   = 0x02,
  LED_CYAN_HRAMWRITE   = 0x03,
  LED_RED_HRAMWRITE    = 0x04,
  LED_GREEN2_HRAMWRITE = 0x05,
  LED_PINK_HRAMWRITE   = 0x06,
  LED_WHITE_HRAMWRITE  = 0x07
};

enum JogLedColor {
  LED_GREEN = 0x02,
  LED_BLUE = 0x03,
  LED_RED = 0x04,
};

enum HerkulexModel {
  MODEL_0101,
  MODEL_0201,
  MODEL_0601,
  MODEL_0602
};

struct StatusData {
  byte StatusError;
  byte StatusDetail;
};

// HERKULEX STATUS ERROR - See Manual p39
static byte H_STATUS_OK					= 0x00;
static byte H_ERROR_INPUT_VOLTAGE 		= 0x01;
static byte H_ERROR_POS_LIMIT			= 0x02;
static byte H_ERROR_TEMPERATURE_LIMIT	= 0x04;
static byte H_ERROR_INVALID_PKT			= 0x08;
static byte H_ERROR_OVERLOAD			= 0x10;
static byte H_ERROR_DRIVER_FAULT  		= 0x20;
static byte H_ERROR_EEPREG_DISTORT		= 0x40;

static const RAMInfo ramInfoTable[] = {
  {0x36, 1}, //voltage currently received
  {0x37, 1}, //Internal temperature in Celcius
  {0x38, 1}, //Current Control Method, 0: Position Control, 1: Velocity Turn Control
  {0x39, 1}, //Actual Servo Tick time
  {0x3A, 2}, //Calibrated Position
  {0x3C, 2}, //Absolute Raw Position Data
  {0x3E, 2}, //Exponent Showing Speed
  {0x40, 2}, //PWM
  {0x42, 2}, //Potentiometer Absolute Position Raw Data
  {0x44, 2}, //Absolute goal Position Raw Data
  {0x46, 2}, //Current goal position based on Speed Profile
  {0x48, 2} //Raw data of desired speed
};

// HERKULEX Broadcast Servo ID
static byte BROADCAST_ID = 0xFE;

class HerkulexClass {
public:
  void  begin(uart_port_t uart_num,long baud, int rx, int tx);
  void  beginSerial1(long baud);
  void  beginSerial2(long baud);
  void  beginSerial3(long baud);
  void  end();
  
  void  initialize();
  StatusData  stat(int servoID);
  void  ACK(int valueACK);
  byte  model();
  void  set_ID(int ID_Old, int ID_New);
  void  clearError(int servoID);
  
  void  torqueON(int servoID);
  void  torqueOFF(int servoID);
  
  void  moveAll(int servoID, int Goal, JogLedColor valueLed, HerkulexModel model);
  void  moveSpeedAll(int servoID, int Goal, JogLedColor valueLed, HerkulexModel model);
  void  moveAllAngle(int servoID, float angle, JogLedColor valueLed, HerkulexModel model);
  void  actionAll(int pTime, bool showStatus);

  void  sendRamWrite(byte* data, int lenghtString);

  void  sendEepWriteRegistry(int servoID, int address, byte* values, int length);
  uint16_t  eepRead(int servoID, int address, int length);
  void  sendSJog(int servoID, int mode, int pTime, JogLedColor valueLed, HerkulexModel model, bool Mode, int LSB, int MSB);
  void  sendIJog();
  void  moveSpeedOne(int servoID, int Goal, int totalTime, JogLedColor valueLed, HerkulexModel model);
  void  moveSpeedOneTracking(int servoID, int Goal, int pTime, JogLedColor valueLed, HerkulexModel model, bool showStatus);
  void  moveOne(int servoID, int Goal, int pTime, JogLedColor valueLed, HerkulexModel model);
  void  moveOneAngle(int servoID, float angle, int pTime, JogLedColor valueLed, HerkulexModel model, bool showStatus);
  // void  moveOneAngleTracking(int servoID, float angle, int pTime, JogLedColor valueLed, HerkulexModel model);

  int   getPosition(int servoID);
  float getAngle(int servoID, HerkulexModel model);
  int   getSpeed(int servoID);
		
  void  reboot(int servoID);
  void  setLed(int servoID, LedColor valueLed);
  void  test_stop(void);
  void  motor_stop(int servoID);
 
  void  writeRegistryRAM(int servoID, int address, int writeByte);
  void  writeRegistryEEP(int servoID, int address, int writeByte);

  void sendData(byte* buffer, int lenght);
  int  readData(int size);
  int  checksum2(int XOR);

  uint16_t RAMRead(uint8_t servoID, RAMObject obj);
  uint16_t RAMReadSerial(uint8_t servoID, RAMObject obj);

  
  int pSize;
  int pID;
  int cmd;
  int lenghtString;
  int ck1;
  int ck2;
  byte dataEx[DATA_MOVE+8];
  byte data[DATA_SIZE]; 
  byte moveData[DATA_MOVE];

// private area  
private:
  
  
  void addData(int GoalLSB, int GoalMSB, int set, int servoID);
  int  checksum1(byte* data, int lenghtString);
  
  void clearBuffer();
  void printHexByte(byte x);

  uart_port_t port;
  
  
  int conta;
  
  int XOR;
  int playTime;
 
};

extern HerkulexClass Herkulex;

#endif // __cplusplus

#endif     //Herkulex_h
