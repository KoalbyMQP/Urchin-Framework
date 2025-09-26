/**
 * @file Herkulex.h
 * @brief Header file for Herkulex motor drivers
 * @author Alessandro Giacomel
 * @author Gabriel Weaver
 * @date 09-07-2025
 * @version 1.0
 *
 * @defgroup Herkulex HerkuleX-Drivers
 */

/*
 Hekulex.h - Library for Dongbu Herkulex DRS-0101/DRS-0201 
 Copyright (c) 2012 - http://robottini.altervista.org
 Refactored by Gabriel Weaver 6/23/2025
 Original Created by Alessandro on 09/12/2012.
 
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
 
 Herkulex.h
    PLEASE START READING: Herkulex Servo Manual (http://www.hovis.co.kr/guide/herkulexeng.pdf)
 *****************************************************************************
 
 IMPORTANT:
  This library only works for ESP-IDF
 *****************************************************************************

 Refactored for ESP-IDF: Gabriel Weaver

 Original Autor:   Alessandro Giacomel
 Contact: alegiaco@gmail.com
 Web:     http://robottini.altervista.org

 *****************************************************************************  
*/

#ifdef __cplusplus

#ifndef Herkulex_h
#define Herkulex_h


#include "freertos/FreeRTOS.h"
#include "driver/uart.h"
#include "DataTypes.h"


/**
 * @def buffer for input data
 */
#define DATA_SIZE	 30


/**
 * @def max 10 servos <---- change this for more servos!
 */
#define DATA_MOVE  	 50




// SERVO HERKULEX COMMAND - See Manual p40
/**
 * @def ROM write
 */
#define HEEPWRITE    0x01


/**
 * @def ROM read
 */
#define HEEPREAD     0x02


/**
 * @def RAM write
 */
#define HRAMWRITE	 0x03


/**
 * @def RAM read
 */
#define HRAMREAD	 0x04


/**
 * @def Write n servo with different timing
 */
#define HINDEJOG		 0x05


/**
 * @def Write n servo with same time
 */
#define HSIMULJOG		 0x06


/**
 * @def Read error
 */
#define HSTAT	 	 0x07


/**
 * @def Back to factory value
 */
#define HROLLBACK	 0x08


/**
 * @def Reboot
 */
#define HREBOOT	 	 0x09

/**
 * @enum RAMObject
 * @brief Brief description of the enum
 *
 */




/**
 * @enum RAMObject
 * @brief Brief description of the enum
 */
enum RAMObject {
  Voltage, /**< Description of VALUE_ONE */
  Temperature, /**< Description of VALUE_ONE */
  ControlMode, /**< Description of VALUE_ONE */
  Tick, /**< Description of VALUE_ONE */
  CalibrationPosition, /**< Description of VALUE_ONE */
  AbsolutePosition, /**< Description of VALUE_ONE */
  DifferentialPosition, /**< Description of VALUE_ONE */
  PWM, /**< Description of VALUE_ONE */
  Absolute2ndPosition, /**< Description of VALUE_ONE */
  AbsoluteGoalPosition, /**< Description of VALUE_ONE */
  DesiredTrajectoryPosition, /**< Description of VALUE_ONE */
  DesiredVelocity, /**< Description of VALUE_ONE */
  RAMObjectCount, /**< Description of VALUE_ONE */
  StatusAll /**< Description of VALUE_ONE */
};


/**
 * @struct RAMInfo
 * @brief FILL THIS IN
 */
typedef struct  {
  uint8_t address; /**< FILL THIS IN */
  uint8_t length; /**< FILL THIS IN */
  const char* comment; /**< FILL THIS IN */
}RAMInfo;

/**
 * @enum LedColor
 */
enum LedColor {
  LED_GREEN_HRAMWRITE  = 0x01, /**< Green */
  LED_BLUE_HRAMWRITE   = 0x02, /**< Blue */
  LED_CYAN_HRAMWRITE   = 0x03, /**< Cyan */
  LED_RED_HRAMWRITE    = 0x04, /**< Red */
  LED_GREEN2_HRAMWRITE = 0x05, /**< Soft Green */
  LED_PINK_HRAMWRITE   = 0x06, /**< Pink */
  LED_WHITE_HRAMWRITE  = 0x07 /**< White */
};

/**
 * @enum JogLedColor
 */
enum JogLedColor {
  LED_GREEN = 0x02,
  LED_BLUE = 0x03,
  LED_RED = 0x04
};

/**
 * @enum HerkulexModel
 */
enum HerkulexModel {
  MODEL_0101,
  MODEL_0201,
  MODEL_0601,
  MODEL_0602
};

enum MoveMode {
    POSITION = 0x00,
    WHEEL = 0x04,
    STOP = 0x02
};

/**
 *
 */
struct StatusData {
  byte StatusError;
  byte StatusDetail;
};





/**
 *
 * @param uart_num The uart port number
 * @param band Date speed rate, see manual
 * @param BUF_SIZE max size of packets, see manual
 * @param tx_pin ESP32 data transfer pin
 * @param rx_pin ESP32 data receive pin
 */
void init_uart(uart_port_t uart_num, int band ,unsigned int BUF_SIZE ,int tx_pin, int rx_pin);

/**
 *
 * @param ms
 */
void delay(unsigned long ms);


/**
 *
 *
 * @return
 */
unsigned long millis();


/**
 *
 * @param a
 * @param b
 * @return
 */
int min(int a, int b);


/**
 *
 */
class HerkulexClass {
public:


  /**
   *
   * @param uart_num
   * @param baud
   * @param rx
   * @param tx
   */
  void  begin(uart_port_t uart_num,long baud, int rx, int tx);


  /**
   *
   */
  void  end();


  /**
   *
   */
  void  initialize();


  /**
   *
   * @param servoID
   * @return
   */
  StatusData  stat(int servoID);


  /**
   *
   * @param valueACK
   */
  void  ACK(int valueACK);


  /**
   *
   * @return
   */
  byte  model();


  /**
   * Changes the ID number of a motor
   * @param ID_Old The old number
   * @param ID_New The new number
   */
  void  set_ID(int ID_Old, int ID_New);


  /**
   *
   * @param servoID
   */
  void  clearError(int servoID);


  /**
   *
   * @param servoID
   */
  void  torqueON(int servoID);


  /**
   *
   * @param servoID
   */
  void  torqueOFF(int servoID);


  /**
   *
   * @param servoID
   * @param Goal
   * @param valueLed
   * @param model
   */
  void  moveAll(int servoID, int Goal, JogLedColor valueLed, HerkulexModel model);


  /**
   *
   * @param servoID
   * @param Goal
   * @param valueLed
   * @param model
   */
  void  moveSpeedAll(int servoID, int Goal, JogLedColor valueLed, HerkulexModel model);


  /**
   *
   * @param servoID
   * @param angle
   * @param valueLed
   * @param model
   */
  void  moveAllAngle(int servoID, float angle, JogLedColor valueLed, HerkulexModel model);


  /**
   *
   * @param pTime
   * @param showStatus
   */
  void  actionAll(int pTime, bool showStatus);


  /**
   *
   * @param data
   * @param lenghtString
   */
  void  sendRamWrite(byte* data, int lenghtString);


  /**
   *
   * @param servoID
   * @param address
   * @param values
   * @param length
   */
  void  sendEepWriteRegistry(int servoID, int address, byte* values, int length);


  /**
   *
   * @param servoID
   * @param address
   * @param length
   * @return
   */
  uint16_t  eepRead(int servoID, int address, int length);


  /**
   *
   * @param servoID
   * @param mode
   * @param pTime
   * @param valueLed
   * @param model
   * @param Mode
   * @param LSB
   * @param MSB
   */
  void  sendSJog(int servoID, int mode, int pTime, JogLedColor valueLed, HerkulexModel model, bool Mode, int LSB, int MSB);


  /**
   *
   * @param servoID
   * @param Goal
   * @param totalTime
   * @param valueLed
   * @param model
   */
  void  moveSpeedOne(int servoID, int Goal, int totalTime, JogLedColor valueLed, HerkulexModel model);


  /**
   *
   * @param servoID
   * @param Goal
   * @param pTime
   * @param valueLed
   * @param model
   * @param showStatus
   */
  void  moveSpeedOneTracking(int servoID, int Goal, int pTime, JogLedColor valueLed, HerkulexModel model, bool showStatus);


  /**
   *
   * @param servoID
   * @param Goal
   * @param pTime
   * @param valueLed
   * @param model
   */
  void  moveOne(int servoID, int Goal, int pTime, JogLedColor valueLed, HerkulexModel model);


  /**
   *
   * @param servoID
   * @param angle
   * @param pTime
   * @param valueLed
   * @param model
   * @param showStatus
   */
  void  moveOneAngle(int servoID, float angle, int pTime, JogLedColor valueLed, HerkulexModel model, bool showStatus);


  /**
   *
   * @param servoID
   * @return
   */
  int getPosition(int servoID);


  /**
   *
   * @param servoID
   * @param model
   * @return
   */
  float getAngle(int servoID, HerkulexModel model);


  /**
   *
   * @param servoID
   * @return
   */
  int   getSpeed(int servoID);


  /**
   *
   * @param servoID
   */
  void  reboot(int servoID);


  /**
   *
   * @param servoID
   * @param valueLed
   */
  void  setLed(int servoID, LedColor valueLed);


  /**
   *
   */
  void  test_stop(void);


  /**
   *
   * @param servoID
   */
  void  motor_stop(int servoID);


  /**
   *
   * @param buffer
   * @param lenght
   */
  void sendData(byte* buffer, int length);


  /**
   *
   * @param size
   * @return
   */
  int readData(int size);


  /**
   *
   * @param servoID
   * @param obj
   * @return
   */
  uint16_t RAMRead(uint8_t servoID, RAMObject obj);


  /**
   *
   * @param servoID
   * @param obj
   * @return
   */
  uint16_t RAMReadSerial(uint8_t servoID, RAMObject obj);

  /**
   *
   * @param indirect
   */
  void SetIndirect(bool indirect);



private:

    int pSize; /**< The total size of the packet */
    int pID; /**< The Motor ID */
    int cmd; /**< The type of motor command  */
    int lenghtString; /**< Size of data section of packet */
    int ck1; /**< Check sum 1 */
    int ck2; /**< Check sum 2 */
    byte dataEx[DATA_MOVE+8]; /**< Full packet (ready to send) */
    byte data[DATA_SIZE];  /**< Parameters only (working buffer)  */
    byte moveData[DATA_MOVE]; /**< Helper buffer specifically for motion commands.  */

    byte BusPacket[233];
    int BusPacketLength;
    bool Indirect;


  /**
   *
   * @param GoalLSB
   * @param GoalMSB
   * @param set
   * @param servoID
   */
  void addData(int GoalLSB, int GoalMSB, int set, int servoID);


  /**
   *
   * @param data
   * @param lenghtString
   * @return
   */
  byte  checksum1(byte* data, int lenghtString);


  /**
   *
   * @param data
   * @param lenghtString
   * @return
   */
  byte  checksum2(byte* data, int lenghtString);


  /**
   *
   */
  void clearBuffer();


  /**
   *
   * @param x
   */
  void printHexByte(byte x);



  uart_port_t port; /**< The UART port for motor movement */
  int conta; /**< Packet buffer index (used when filling dataEx[] with bytes) */
  int playTime; /**< Motion duration in units of 11.2 ms (0–255) */
 
};

extern HerkulexClass Herkulex;

#endif // __cplusplus

#endif //Herkulex_h
