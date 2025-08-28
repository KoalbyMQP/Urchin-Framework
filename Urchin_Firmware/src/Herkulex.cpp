/*
 src/coms.c
 Hekulex.cpp - Library for Dongbu Herkulex DRS-0101/DRS-0201 
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
 *****************************************************************************  
*/
#include "../src/Herkulex.h"

#include "../src/coms.h"
#include "freertos/FreeRTOS.h"
#include "../src/MSGQueue.h"
#include <string>
#include <cmath>
//#include "SoftwareSerial.h"


// Macro for the Serial port selection
#define HSerial1     1 		// Write in Serial 1 port Arduino Mega - Pin 19(rx) - 18 (tx) 
#define HSerial2     2   	// Write in Serial 2 port Arduino Mega - Pin 17(rx) - 16 (tx) 
#define HSerial3     3   	// Write in Serial 3 port Arduino Mega - Pin 15(rx) - 14 (tx)
#define Serial      4   	// Write in SoftSerial Arduino with 328p or Mega
 
//extern SoftwareSerial SwSerial(0, 1);


void delay(unsigned long ms) {
	vTaskDelay(ms / portTICK_PERIOD_MS);
}

unsigned long millis() {
	return xTaskGetTickCount() * portTICK_PERIOD_MS;
}


// void init_uart(uart_port_t uart_num, unsigned long band ,unsigned int BUF_SIZE ,int tx_pin, int rx_pin) {
void init_uart(uart_port_t uart_num, int band ,unsigned int BUF_SIZE ,int tx_pin, int rx_pin) {
	uart_config_t uart_config = {
		.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		.rx_flow_ctrl_thresh = 122,  // Common safe value
		.source_clk = UART_SCLK_APB  // Ensure flags are set
		};

	// Configure UART parameters
	ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

	// Set UART pins
	ESP_ERROR_CHECK(uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

	// Install UART driver
	ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
}



int min(int a, int b) {
	return (a < b) ? a : b;
}

// Herkulex begin with Arduino Uno
void HerkulexClass::begin(uart_port_t uart_num,long baud, int rx, int tx){

	init_uart(uart_num,baud,233,tx,rx);

	port = uart_num;
}


//Types of Commands Sent to Herkulex

void HerkulexClass::sendRamWrite(byte* data, int lenghtString){
	ck1=checksum1(data,lenghtString);	//6. Checksum1
	ck2=checksum2(ck1);					//7. Checksum2

	dataEx[0] = 0xFF;			// Packet Header
	dataEx[1] = 0xFF;			// Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = pID;			// Servo ID
	dataEx[4] = cmd;			// Command Ram Write
	dataEx[5] = ck1;			// Checksum 1
	dataEx[6] = ck2;			// Checksum 2
	for (int i = 0; i < lenghtString; i++) {
		dataEx[7 + i] = data[i];
	}

 	sendData(dataEx, pSize);
}
uint16_t HerkulexClass::eepRead(int servoID, int address, int length) {
 	pSize = 0x09;         // Initial packet size
    pID = servoID;
    cmd = HEEPREAD;

    data[0] = address;    // Start address
    data[1] = length;     // Number of bytes to read

    lenghtString = 2;
    ck1 = checksum1(data, lenghtString);
    ck2 = checksum2(ck1);

    dataEx[0] = 0xFF;
    dataEx[1] = 0xFF;
    dataEx[2] = pSize;
    dataEx[3] = pID;
    dataEx[4] = cmd;
    dataEx[5] = ck1;
    dataEx[6] = ck2;
    dataEx[7] = data[0];
    dataEx[8] = data[1];

    sendData(dataEx, pSize);
    delay(1);

    int totalToRead = 9 + length;  // Response length = 9 + data bytes
    readData(totalToRead);

    // Check basic header
    if (dataEx[0] != 0xFF || dataEx[1] != 0xFF || dataEx[4] != 0x4A) return -2;

    // Extract and return data one byte at a time (as int packed result)
    int result = 0;
    for (int i = 0; i < length; i++) {
        result |= (dataEx[9 + i] << (8 * i));  // Little endian pack
    }

    return result;  // Return packed integer result
}
void HerkulexClass::sendEepWriteRegistry(int servoID, int address, byte* values, int length) {
	pSize = 0x07 + 2 + length;  // Header (7) + Address/Length (2) + Data
	pID   = servoID;
	cmd   = HEEPWRITE;

	data[0] = address;
	data[1] = length;

	for (int i = 0; i < length; i++) {
		data[2 + i] = values[i];
	}

	lenghtString = 2 + length;
	ck1 = checksum1(data, lenghtString);
	ck2 = checksum2(ck1);

	dataEx[0] = 0xFF;
	dataEx[1] = 0xFF;
	dataEx[2] = pSize;
	dataEx[3] = pID;
	dataEx[4] = cmd;
	dataEx[5] = ck1;
	dataEx[6] = ck2;

	for (int i = 0; i < lenghtString; i++) {
		dataEx[7 + i] = data[i];
	}

	sendData(dataEx, pSize);
}

void HerkulexClass::sendSJog(int servoID, int Target, int pTime, JogLedColor valueLed, HerkulexModel model, bool Mode, int LSB, int MSB){
	int SetValue;
	if(Mode == true){
		Mode = 1;
		SetValue = (1<<Mode) | (1<<valueLed);
	} 
	else{
		SetValue = (1<<valueLed);
	}
	// int SetValue = (1<<Mode) | (1<<valueLed); //shifts the value by the mode and the LED. A multi-command
	playTime=int((float)pTime/11.2);	
  
	pSize = 0x0C; //SJog Command is 12 Bits long, varies with other functions
	cmd   = HSIMULJOG; //Movement functions typically use SJog for simultanenous movement or IJog for indiviual motor movement.        		
  
	data[0]=LSB; 
	data[1]=MSB; 
	data[2]=SetValue;                         
	data[3]=servoID;                    		
  
	pID=servoID^playTime; //Necessary for code to function
  
	lenghtString=4;             				// lenghtData
  
	ck1=checksum1(data,lenghtString);			//6. Checksum1
	ck2=checksum2(ck1);						//7. Checksum2
  
	pID=servoID; 
  
	dataEx[0] = 0xFF;				// CONST Packet Header
	dataEx[1] = 0xFF;				// CONST Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = servoID;
	dataEx[4] = cmd;
	dataEx[5] = ck1;
	dataEx[6] = ck2;
	dataEx[7] = playTime; 	

	for (int i = 0; i < lenghtString; i++) {
		dataEx[8 + i] = data[i];
	}
	sendData(dataEx, pSize);
}

// void sendIJog();


// initialize servos
void HerkulexClass::initialize()
{
        conta=0;
		lenghtString=0;
        delay(100);       
        clearError(BROADCAST_ID);	// clear error for all servos
        delay(10);
        ACK(1);						// set ACK
        delay(10);
        torqueON(BROADCAST_ID);		// torqueON for all servos
        delay(10);
		
}

// stat
StatusData HerkulexClass::stat(int servoID)
{
	// int TotalBits = 8; //Total Bits sent in the status byte
	const char* StatusErrorMessages[] = {
		"Exceed Allowed Input Voltage Limit", //Exceeding the Voltage Limit Input of the Motors, 7.4V for 0101/0201, 12V for the 0401/0601
		"Exceed Allowed Position", //Exceeded allowed Position, This only counts for non full-range motors. Change Limits in Herkulex Manager
		"Exceed Allowed Temperature", //Exceeded Temperature Limit motor interally 
		"Invalid Packet: Multiple Possible Errors, See Status Detail", //Invalid Packet Sent, Refer to Status2 for detailed Explaination of Error
		"Overload Error Detected", //Overload Detected
		"Reserved Bit", //Reserved Bit
		"EEP REG Distorted", //EEP Register distorted
		"Reserved Bit", //Reserved Bit 
	};
	const char* StatusDetailMessage[] = {
		"Moving Flag Active", //Moving Flag, the Motor is moving. Read Only
		"InPosition Flag Active", //The motor is within the position limits. Read Only
		"Checksum Error Occured", //Checksum did not equal up. 
		"Unknown Command Sent", //The motor did not understand the command sent. Was not in the approved list of commands
		"Exceeded Register Range", //Too much information was sent and exceeded the register range
		"Garbage Detected", //Garbage Data Detected.
		"Torque On", //Torque on Flag activated. Read only
		"Reserved Bit", //Reserved Bit
	};

	{
	pSize    = 0x07;			//3.Packet size
	pID      = servoID;			//4.Servo ID - 0XFE=All servos
	cmd      = HSTAT;			//5.CMD
	
	ck1=(pSize^pID^cmd)&0xFE;
        ck2=(~(pSize^pID^cmd))&0xFE ; 
  
	dataEx[0] = 0xFF;			// Packet Header
	dataEx[1] = 0xFF;			// Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = pID;			// Servo ID
	dataEx[4] = cmd;			// Command Ram Write
	dataEx[5] = ck1;			// Checksum 1
	dataEx[6] = ck2;			// Checksum 2
	     
	sendData(dataEx, pSize);
	delay(2);
	readData(9); 				// read 9 bytes from serial

	
	pSize = dataEx[2];           // 3.Packet size 7-58
	pID   = dataEx[3];           // 4. Servo ID
	cmd   = dataEx[4];           // 5. CMD
	data[0]=dataEx[7];
    data[1]=dataEx[8];
    lenghtString=2;

	
    ck1 = (dataEx[2]^dataEx[3]^dataEx[4]^dataEx[7]^dataEx[8]) & 0xFE;
	ck2=checksum2(ck1);			
	
	if (ck1 != dataEx[5]) return {static_cast<byte>(-1), 0};
    if (ck2 != dataEx[6]) return {static_cast<byte>(-2), 0};
	
	byte status1 = dataEx[7];
	byte status2 = dataEx[8];
	std::string StatusMessage = "Status Errors: ";
	std::string StatusDetail = "Status Details: ";

    	if (status1 == 0) {
			StatusMessage += "No errors detected.";//Initial Code, No errors Detected
		} else {
			bool first = true;
			for (int i = 0; i < 8; i++) { //Loop through all bits to detect possible Error codes
				if (status1 & (1 << i)) {
					if (!first) StatusMessage += ", ";
					StatusMessage += StatusErrorMessages[i]; //Append any detected error code onto StatusMessage
					first = false;
				}
			}
		}
		if (status2 == 0) {
			StatusDetail += "No details available.";//No Details Available is an Error, There should always be details even without errors
		} else {
			bool first = true;
			for (int i = 0; i < 8; i++) {
				if (status2 & (1 << i)) {
					if (!first) StatusDetail += ", ";
					StatusDetail += StatusDetailMessage[i];//Append any details detected onto StatusDetail
					first = false;
				}
			}
		}
		PrintfToPI(DebugQueue,(StatusMessage+"\n").c_str());
		PrintfToPI(DebugQueue,(StatusDetail+"\n").c_str());
		return {status1, status2};
	}
}

// torque on - 
void HerkulexClass::torqueON(int servoID)
{
	pSize = 0x0A;               // 3.Packet size 7-58
	pID   = servoID;            // 4. Servo ID
	cmd   = HRAMWRITE;          // 5. CMD
	data[0]=0x34;               // 8. Address
	data[1]=0x01;               // 9. Lenght
	data[2]=0x60;               // 10. 0x60=Torque ON
	lenghtString=3;             // lenghtData

	sendRamWrite(data, lenghtString);
}

// torque off - the torque is FREE, not Break
void HerkulexClass::torqueOFF(int servoID)
{
	pSize = 0x0A;               // 3.Packet size 7-58
	pID   = servoID;            // 4. Servo ID
	cmd   = HRAMWRITE;          // 5. CMD
	data[0]=0x34;               // 8. Address
	data[1]=0x01;               // 9. Lenght
	data[2]=0x00;               // 10. 0x00=Torque Free
	lenghtString=3;             // lenghtData

	sendRamWrite(data, lenghtString);
}

// ACK  - 0=No Replay, 1=Only reply to READ CMD, 2=Always reply
void HerkulexClass::ACK(int valueACK)
{
	pSize = 0x0A;               // 3.Packet size 7-58
	pID   = 0xFE;	            // 4. Servo ID
	cmd   = HRAMWRITE;          // 5. CMD
	data[0]=0x34;               // 8. Address
	data[1]=0x01;               // 9. Lenght
	data[2]=valueACK;           // 10.Value. 0=No Replay, 1=Only reply to READ CMD, 2=Always reply
	lenghtString=3;             // lenghtData
  	
	sendRamWrite(data, lenghtString);
}

// model - 1=0101 - 2=0201
byte HerkulexClass::model()
{
	pSize = 0x09;               // 3.Packet size 7-58
	pID   = 0xFE;	            // 4. Servo ID
	cmd   = HEEPREAD;           // 5. CMD
	data[0]=0x00;               // 8. Address
	data[1]=0x01;               // 9. Lenght
	lenghtString=2;             // lenghtData
  	
	ck1=checksum1(data,lenghtString);	//6. Checksum1
	ck2=checksum2(ck1);					//7. Checksum2

	dataEx[0] = 0xFF;			// Packet Header
	dataEx[1] = 0xFF;			// Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = pID;			// Servo ID
	dataEx[4] = cmd;			// Command Ram Write
	dataEx[5] = ck1;			// Checksum 1
	dataEx[6] = ck2;			// Checksum 2
	dataEx[7] = data[0]; 		// Address
	dataEx[8] = data[1]; 		// Length

    sendData(dataEx, pSize);

	delay(1);
	readData(9);
	
	pSize = dataEx[2];           // 3.Packet size 7-58
	pID   = dataEx[3];           // 4. Servo ID
	cmd   = dataEx[4];           // 5. CMD
	data[0]=dataEx[7];           // 8. 1st byte
	lenghtString=1;              // lenghtData
  	
	ck1=checksum1(data,lenghtString);	//6. Checksum1
	ck2=checksum2(ck1);					//7. Checksum2

	if (ck1 != dataEx[5]) return -1; //checksum verify
	if (ck2 != dataEx[6]) return -2;
		
	return dataEx[7];			// return status

}

// setID - Need to restart the servo
void HerkulexClass::set_ID(int ID_Old, int ID_New)
{
	pSize = 0x0A;               // 3.Packet size 7-58
	pID   = ID_Old;		        // 4. Servo ID OLD - original servo ID
	cmd   = HEEPWRITE;          // 5. CMD
	data[0]=0x06;               // 8. Address
	data[1]=0x01;               // 9. Lenght
	data[2]=ID_New;             // 10. ServoID NEW
	lenghtString=3;             // lenghtData
  	
	ck1=checksum1(data,lenghtString);	//6. Checksum1
	ck2=checksum2(ck1);					//7. Checksum2

	dataEx[0] = 0xFF;			// Packet Header
	dataEx[1] = 0xFF;			// Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = pID;			// Servo ID
	dataEx[4] = cmd;			// Command Ram Write
	dataEx[5] = ck1;			// Checksum 1
	dataEx[6] = ck2;			// Checksum 2
	dataEx[7] = data[0]; 		// Address 52
	dataEx[8] = data[1]; 		// Length
	dataEx[9] = data[2]; 		// Value

	sendData(dataEx, pSize);

}

// clearError
void HerkulexClass::clearError(int servoID)
{
	pSize = 0x0B;               // 3.Packet size 7-58
	pID   = servoID;     		// 4. Servo ID - 253=all servos
	cmd   = HRAMWRITE;          // 5. CMD
	data[0]=0x30;               // 8. Address
	data[1]=0x02;               // 9. Lenght
	data[2]=0x00;               // 10. Write error=0
	data[3]=0x00;               // 10. Write detail error=0
	
	lenghtString=4;             // lenghtData

	sendRamWrite(data, lenghtString);
}

/*
int HerkulexClass::getPosition(int servoID, bool is0601) {
	int Position  = 0;

    pSize = 0x09;               // 3.Packet size 7-58
	pID   = servoID;     	    // 4. Servo ID - 253=all servos
	cmd   = HRAMREAD;           // 5. CMD
	data[0]=0x3A;               // 8. Address
	data[1]=0x02;               // 9. Lenght
	
	lenghtString=2;             // lenghtData
  	
	ck1=checksum1(data,lenghtString);	//6. Checksum1
	ck2=checksum2(ck1);					//7. Checksum2

	dataEx[0] = 0xFF;			// Packet Header
	dataEx[1] = 0xFF;			// Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = pID;			// Servo ID
	dataEx[4] = cmd;			// Command Ram Write
	dataEx[5] = ck1;			// Checksum 1
	dataEx[6] = ck2;			// Checksum 2
	dataEx[7] = data[0];      	// Address  
	dataEx[8] = data[1]; 		// Length
	
	sendData(dataEx, pSize);

    delay(1);
	readData(13);

        	
	pSize = dataEx[2];           // 3.Packet size 7-58
	pID   = dataEx[3];           // 4. Servo ID
	cmd   = dataEx[4];           // 5. CMD
	data[0]=dataEx[7];
    data[1]=dataEx[8];
    data[2]=dataEx[9];
    data[3]=dataEx[10];
    data[4]=dataEx[11];
    data[5]=dataEx[12];
    lenghtString=6;

    ck1=checksum1(data,lenghtString);	//6. Checksum1
	ck2=checksum2(ck1);					//7. Checksum2

    if (ck1 != dataEx[5]) return -1;
	if (ck2 != dataEx[6]) return -1;

    int maxMSB = is0601 ? 0x07 : 0x03;

	Position = ((dataEx[10]&maxMSB)<<8) | dataEx[9];
        return Position;
	
}
*/

// float HerkulexClass::getAngle(int servoID, bool is0601) {
// 	int pos = (int)getPosition(servoID, is0601);
//     int conversionFactor = is0601 ? 2 : 1;
// 	return (pos/conversionFactor - 512) * 0.325;
// }

// reboot single servo - pay attention 253 - all servos doesn't work!
void HerkulexClass::reboot(int servoID) {
        
    pSize = 0x07;               // 3.Packet size 7-58
	pID   = servoID;     	    // 4. Servo ID - 253=all servos
	cmd   = HREBOOT;            // 5. CMD
    ck1=(pSize^pID^cmd)&0xFE;
    ck2=(~(pSize^pID^cmd))&0xFE ; ;	

	dataEx[0] = 0xFF;			// Packet Header
	dataEx[1] = 0xFF;			// Packet Header	
	dataEx[2] = pSize;	 		// Packet Size
	dataEx[3] = pID;			// Servo ID
	dataEx[4] = cmd;			// Command Ram Write
	dataEx[5] = ck1;			// Checksum 1
	dataEx[6] = ck2;			// Checksum 2
	
	sendData(dataEx, pSize);

}

// LED  - see table of colors 
void HerkulexClass::setLed(int servoID, LedColor valueLed)
{
	pSize   = 0x0A;               // 3.Packet size 7-58
	pID     = servoID;            // 4. Servo ID
	cmd     = HRAMWRITE;          // 5. CMD
	data[0] = 0x35;               // 8. Address 53
    data[1] = 0x01;               // 9. Lenght
	data[2] = valueLed;           // 10.LedValue
	lenghtString=3;               // lenghtData

	sendRamWrite(data, lenghtString);
}
/*
// get the speed for one servo - values betweeb -1023 <--> 1023
int HerkulexClass::getSpeed(int servoID) {
  int speedy  = 0;

  pSize = 0x09;               // 3.Packet size 7-58
  pID   = servoID;     	   	  // 4. Servo ID 
  cmd   = HRAMREAD;           // 5. CMD
  data[0]=0x40;               // 8. Address
  data[1]=0x02;               // 9. Lenght

  lenghtString=2;             // lenghtData

  ck1=checksum1(data,lenghtString);		//6. Checksum1
  ck2=checksum2(ck1);					//7. Checksum2

  dataEx[0] = 0xFF;			// Packet Header
  dataEx[1] = 0xFF;			// Packet Header	
  dataEx[2] = pSize;		// Packet Size
  dataEx[3] = pID;			// Servo ID
  dataEx[4] = cmd;			// Command Ram Write
  dataEx[5] = ck1;			// Checksum 1
  dataEx[6] = ck2;			// Checksum 2
  dataEx[7] = data[0]; 	    // Address  
  dataEx[8] = data[1]; 		// Length

  sendData(dataEx, pSize);

  delay(1);
  readData(13);


  pSize = dataEx[2];           // 3.Packet size 7-58
  pID   = dataEx[3];           // 4. Servo ID
  cmd   = dataEx[4];           // 5. CMD
  data[0]=dataEx[7];
  data[1]=dataEx[8];
  data[2]=dataEx[9];
  data[3]=dataEx[10];
  data[4]=dataEx[11];
  data[5]=dataEx[12];
  lenghtString=6;

  ck1=checksum1(data,lenghtString);	//6. Checksum1
  ck2=checksum2(ck1);				//7. Checksum2

  if (ck1 != dataEx[5]) return -1;
  if (ck2 != dataEx[6]) return -1;

  speedy = ((dataEx[10]&0xFF)<<8) | dataEx[9];
  return speedy;

}
  */

int HerkulexClass::getSpeed(int servoID){
	return Herkulex.RAMRead(servoID, DifferentialPosition);
}
int HerkulexClass::getPosition(int servoID) {
    return RAMRead(servoID, CalibrationPosition);
}

float HerkulexClass::getAngle(int servoID, HerkulexModel model) {
    int pos = (int)getPosition(servoID);

    int center = 0;
    float degreesPerUnit = 0.0;

    switch (model) {
		case MODEL_0101:
        case MODEL_0201:
            center = 512;
            degreesPerUnit = 0.325;
            break;
        case MODEL_0601:
            center = 1024;
            degreesPerUnit = 0.161;
            break;
        case MODEL_0602:
            center = 16384;
            degreesPerUnit = 0.011;
            break;
    }
    return (pos - center) * degreesPerUnit;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Test stop function starts+++++++++++++++++++++++++++++++++++++++++++++++++
void HerkulexClass::test_stop(void){

    dataEx[0] = 0xFF;				// Packet Header
    dataEx[1] = 0xFF;				// Packet Header	
    dataEx[2] = 0x0C;	 		// Packet Size
    dataEx[3] = 0x0B;				// Servo ID
    dataEx[4] = 0x06;				// Command Ram Write
    dataEx[5] = 0x6C;				// Checksum 1
    dataEx[6] = 0x92;				// Checksum 2
    dataEx[7] = 0x64;  		// Execution time	
    dataEx[8] = 0x00;
    dataEx[9] = 0x00;
    dataEx[10] = 0x02;
    dataEx[11] = 0x0B;
  
    sendData(dataEx, 0x0C);

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Test stop function ends+++++++++++++++++++++++++++++++++++++++++++++++++

void HerkulexClass::motor_stop(int servoID){

	pSize = 0x0C;               // 3.Packet size 7-58
	pID   = servoID;     	   	  // 4. Servo ID 
	cmd   = HSIMULJOG;           // 5. CMD
	data[0]=0x00;               // 8. Address
	data[1]=0x00;               // 9. Lenght

	lenghtString=2;             // lenghtData

	ck1=checksum1(data,lenghtString);		//6. Checksum1
	PrintfToPI(DebugQueue,"%d",ck1);
	ck2=checksum2(ck1);					//7. Checksum2
	PrintfToPI(DebugQueue,"%d",ck2);

    dataEx[0] = 0xFF;				// Packet Header
    dataEx[1] = 0xFF;				// Packet Header	
    dataEx[2] = pSize;	 		// Packet Size
    dataEx[3] = pID;				// Servo ID
    dataEx[4] = cmd;				// Command Ram Write
    dataEx[5] = 0x08;				// Checksum 1
    dataEx[6] = 0xF6;				// Checksum 2
    dataEx[7] = data[0];  		// Execution time	
    dataEx[8] = data[1];
    dataEx[9] = 0x00;
    dataEx[10] = 0x02;
    dataEx[11] = pID;

	// Serial.println("=== motor_stop() DEBUG ===");
    // Serial.print("pSize: "); Serial.println(pSize);
    // Serial.print("pID: "); Serial.println(pID);
    // Serial.print("cmd: "); Serial.println(cmd, HEX);
    // Serial.print("data[0] (Address): "); Serial.println(data[0], HEX);
    // Serial.print("data[1] (Length): "); Serial.println(data[1], HEX);
    // Serial.print("ck1: "); Serial.println(ck1, HEX);
    // Serial.print("ck2: "); Serial.println(ck2, HEX);

    // Serial.println("dataEx:");
    // for (int i = 0; i < 12; i++) {
    //     Serial.print("dataEx["); Serial.print(i); Serial.print("]: ");
    //     Serial.println(dataEx[i], HEX);
    // }

    // Serial.println("==========================");
  
    sendData(dataEx, 0x0C);

}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Test RamRead Function Starts +++++++++++++++++++++++++++++++++++++++++++

uint16_t HerkulexClass::RAMReadSerial(uint8_t servoID, RAMObject obj) {
    static const RAMInfo ramInfoTable[RAMObjectCount] = {
        {0x36, 1, "voltage currently received"},
        {0x37, 1, "Internal temperature in Celcius"},
        {0x38, 1, "Current Control Method, 0: Position Control, 1: Velocity Turn Control"},
        {0x39, 1, "Actual Servo Tick time"},
        {0x3A, 2, "Calibrated Position"},
        {0x3C, 2, "Absolute Raw Position Data"},
        {0x3E, 2, "Exponent Showing Speed"},
        {0x40, 2, "Size of the Energy Output"},
        {0x42, 2, "Potentiometer Absolute Position Raw Data"},
        {0x44, 2, "Absolute goal Position Raw Data"},
        {0x46, 2, "Current goal position based on Speed Profile"},
        {0x48, 2, "Raw data of desired speed"}
    };
	// static const RAMInfo ramInfoTable[] = {
	// 	{0x36, 1}, //voltage currently received
	// 	{0x37, 1}, //Internal temperature in Celcius
	// 	{0x38, 1}, //Current Control Method, 0: Position Control, 1: Velocity Turn Control
	// 	{0x39, 1}, //Actual Servo Tick time
	// 	{0x3A, 2}, //Calibrated Position
	// 	{0x3C, 2}, //Absolute Raw Position Data
	// 	{0x3E, 2}, //Exponent Showing Speed
	// 	{0x40, 2}, //PWM
	// 	{0x42, 2}, //Potentiometer Absolute Position Raw Data
	// 	{0x44, 2}, //Absolute goal Position Raw Data
	// 	{0x46, 2}, //Current goal position based on Speed Profile
	// 	{0x48, 2} //Raw data of desired speed
	// };

	if (obj == StatusAll) {
        int count = 0;  // Counter to track the number of items printed per line
        const char* labels[RAMObjectCount] = {
            "Volt: ", "Temp: ", "Mode: ", "Tic: ", "CalPos: ", "RawPos: ",
            "ExpSpd: ", "PWM: ", "PotPos: ", "GoalPos: ", "DesireTraj: ", "DesireSpd: "
        };

        // Loop through all the RAM objects and print their values
        for (int i = 0; i < RAMObjectCount; ++i) {
            // const RAMInfo& info = ramInfoTable[i];
            uint16_t result = RAMRead(servoID, static_cast<RAMObject>(i));  // Call RAMRead for each object

            // Print status abbreviation and value
            if (count == 0) {
                PrintfToPI(DebugQueue,"[ ");  // Start of the line
            }

            // Print the label for the current status
             PrintfToPI(DebugQueue,labels[i]);

            // If it's the voltage status, print it as a float
            if (i == Voltage) {
                 PrintfToPI(DebugQueue,"%f",result /10.0f);
            } else {
                 PrintfToPI(DebugQueue,"%d",result);
            }

            count++;

            // After printing 6 items, end the line and start a new one
            if (count == 6) {
                 PrintfToPI(DebugQueue," ]");
                count = 0;  // Reset the counter for the next line
            } else {
                PrintfToPI(DebugQueue," | ");  // Separator between statuses
            }
        }

        // If there are any remaining items not printed on the last line, print the closing bracket
        if (count > 0) {
            PrintfToPI(DebugQueue," ]");
        }
    } else {
        // Otherwise, print just the requested status
        if (obj < 0 || obj >= RAMObjectCount) {
             PrintfToPI(DebugQueue,"Invalid RAM object selected.");
            return 0;
        }

    const RAMInfo& info = ramInfoTable[obj];

    if ((info.address + info.length) > 0x7F) {
         PrintfToPI(DebugQueue,"RAMRead: Exceeds register range. Aborting.");
        return 0;
    }

		pSize = 0x09;
		pID = servoID;
		cmd = HRAMREAD;

		data[0] = info.address;
		data[1] = info.length;
		lenghtString = 2;

		ck1 = checksum1(data, lenghtString);
		ck2 = checksum2(ck1);

		dataEx[0] = 0xFF;
		dataEx[1] = 0xFF;
		dataEx[2] = pSize;
		dataEx[3] = pID;
		dataEx[4] = cmd;
		dataEx[5] = ck1;
		dataEx[6] = ck2;
		dataEx[7] = data[0];
		dataEx[8] = data[1];

		sendData(dataEx, pSize);
		delay(1); // Small delay to allow response

		int expectedSize = 9 + info.length;
		readData(expectedSize);

		uint16_t result = 0;
		if (info.length == 1) {
			result = dataEx[9];
		} else if (info.length == 2) {
			result = ((uint16_t)dataEx[10] << 8) | dataEx[9];
		}

		 PrintfToPI(DebugQueue,info.comment);
		 PrintfToPI(DebugQueue,": ");
		if (obj == Voltage) {
			 PrintfToPI(DebugQueue,"%f",result /10.0f);  // Display as float, e.g., 12.3
		} else {
			 PrintfToPI(DebugQueue,"%d",result);
		}	

		return result;
	}

	return 0;
}
uint16_t HerkulexClass::RAMRead(uint8_t servoID, RAMObject obj) {

    if (obj < 0 || obj >= (sizeof(ramInfoTable) / sizeof(RAMInfo))) {
        return 0;
    }

    const RAMInfo& info = ramInfoTable[obj];

    if ((info.address + info.length) > 0x7F) {
        return 0;
    }

    pSize = 0x09;
    pID = servoID;
    cmd = HRAMREAD;

    data[0] = info.address;
    data[1] = info.length;
    lenghtString = 2;

    ck1 = checksum1(data, lenghtString);
    ck2 = checksum2(ck1);

    dataEx[0] = 0xFF;
    dataEx[1] = 0xFF;
    dataEx[2] = pSize;
    dataEx[3] = pID;
    dataEx[4] = cmd;
    dataEx[5] = ck1;
    dataEx[6] = ck2;
    dataEx[7] = data[0];
    dataEx[8] = data[1];

    sendData(dataEx, pSize);
    delay(1);

    int expectedSize = 9 + info.length;
    readData(expectedSize);

    uint16_t result = 0;
    if (info.length == 1) {
        result = dataEx[9];
    } else if (info.length == 2) {
        result = ((uint16_t)dataEx[10] << 8) | dataEx[9];
    }

    return result;
}

// uint16_t HerkulexClass::RAMRead(uint8_t servoID, uint8_t startAddress, uint8_t length) {
//     pSize = 0x09;
//     pID = servoID;
//     cmd = HRAMREAD;

// 	if ((startAddress + length) > 0x7F) {
// 		Serial.println("RAMRead: Exceeds register range. Aborting.");
// 		return 0;
// 	}

//     data[0] = startAddress;
//     data[1] = length;
//     lenghtString = 2;

//     ck1 = checksum1(data, lenghtString);
//     ck2 = checksum2(ck1);

//     dataEx[0] = 0xFF;
//     dataEx[1] = 0xFF;
//     dataEx[2] = pSize;
//     dataEx[3] = pID;
//     dataEx[4] = cmd;
//     dataEx[5] = ck1;
//     dataEx[6] = ck2;
//     dataEx[7] = data[0];
//     dataEx[8] = data[1];

//     sendData(dataEx, pSize);
//     delay(1); // Small delay to allow response

//     int expectedSize = 9 + length;  // 9-byte header + data length
//     readData(expectedSize);         // fills dataEx[]

//     // Now extract and return the value from dataEx[9] and dataEx[10]
//     if (length == 1) {
//         return dataEx[9];
//     } else if (length == 2) {
//         return ((uint16_t)dataEx[10] << 8) | dataEx[9];
//     } else {
//         // Return 0 for now if unsupported size
//         return 0;
//     }
// }
// bool HerkulexClass::RAMRead(uint8_t servoID, uint8_t startAddress, uint8_t length) {
//     pSize = 0x09;               // Total packet size
//     pID   = servoID;
//     cmd   = HRAMREAD;
//     data[0] = startAddress;
//     data[1] = length;
//     lenghtString = 2;

//     ck1 = checksum1(data, lenghtString);
//     ck2 = checksum2(ck1);

//     dataEx[0] = 0xFF;
//     dataEx[1] = 0xFF;
//     dataEx[2] = pSize;
//     dataEx[3] = pID;
//     dataEx[4] = cmd;
//     dataEx[5] = ck1;
//     dataEx[6] = ck2;
//     dataEx[7] = data[0];
//     dataEx[8] = data[1];

//     sendData(dataEx, pSize);

//     delay(1);
//     int expectedSize = 9 + length;  // standard read response is 9 + data
//     readData(expectedSize);

//     // Optional: verify checksum here if needed
//     return true;  // You could also add error codes
// }

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ Test RamRead Function Ends +++++++++++++++++++++++++++++++++++++++++++++

void HerkulexClass::moveSpeedOneTracking(int servoID, int targSpeed, int totalTimeMs, JogLedColor valueLed, HerkulexModel model, bool showStatus) {
    if (targSpeed > 1023 || targSpeed < -1023) {
        PrintfToPI(DebugQueue,"Invalid speed input.");
        return;
    }

    // Set speed goal with sign bit if needed
    int GoalSpeedSign = abs(targSpeed);
    if (targSpeed < 0) {
        GoalSpeedSign |= 0x4000;  // set sign bit
    }

    int LSB = GoalSpeedSign & 0x00FF;
    int MSB = (GoalSpeedSign >> 8) & 0x00FF;
    bool Mode = true;  // speed mode

    // Start the speed command
    Herkulex.sendSJog(servoID, targSpeed, 0, valueLed, model, Mode, LSB, MSB);

    // Record start tick
    uint8_t startTick = Herkulex.RAMRead(servoID, Tick);
    uint8_t currentTick = startTick;
    int elapsedTicks = 0;

    const int tickIntervalMs = 11;  // Typical tick duration per datasheet
    int targetTicks = totalTimeMs / tickIntervalMs;

    while (elapsedTicks < targetTicks) {
        delay(tickIntervalMs);

        uint8_t newTick = Herkulex.RAMRead(servoID, Tick);
        int tickDiff = (newTick - currentTick + 256) % 256;  // handle overflow
        elapsedTicks += tickDiff;
        currentTick = newTick;

        if (showStatus) {
            Herkulex.RAMReadSerial(servoID, StatusAll);  // Print abbreviated statuses
        }
    }

    // Stop motor by setting speed to 0
    Herkulex.motor_stop(servoID);
     PrintfToPI(DebugQueue,"Speed movement finished.");
}

// move one servo with continous rotation
void HerkulexClass::moveSpeedOne(int servoID, int targSpeed, int totalTime, JogLedColor valueLed, HerkulexModel model)
{
	// if (targSpeed > 1023 || targSpeed < -1023) return;              // speed (goal) non correct
	// if ((pTime <0) || (pTime > 2856)) return;
  
	int GoalSpeedSign;
	if (targSpeed < 0) {
	  GoalSpeedSign = (-1)* targSpeed ;
	  GoalSpeedSign |= 0x4000;  //bit number 14 
	} 
	else {
	  GoalSpeedSign = targSpeed;
	}
	int LSB=GoalSpeedSign & 0X00FF; 		       // MSB speedGoal 
	int MSB=(GoalSpeedSign & 0xFF00) >> 8;      // LSB speedGoal 

	const bool Mode = true;
    const int maxTick = 254;
    int timeRemaining = totalTime;
	timeRemaining = min(int((float)totalTime /11.2), 255);

    // Read the starting tick count from motor RAM (Tick address is usually 52)
    // uint8_t startTick = RAMRead(servoID, Tick);	
    // unsigned long startMillis = millis(); // for backup timing

    while (timeRemaining > 0) {
        int thisTick = min(timeRemaining, maxTick);  // Limit each jog to maxTick
        sendSJog(servoID, targSpeed, thisTick, valueLed, model, Mode, LSB, MSB);

        // Wait for 'thisTick' duration using RAM Tick
        uint8_t tickStart = RAMRead(servoID, Tick);
        while (true) {
            uint8_t currentTick = RAMRead(servoID, Tick);
            int tickDelta = (int)currentTick - (int)tickStart;

            if (tickDelta < 0) tickDelta += 256;  // Handle wraparound
            if (tickDelta >= thisTick) break;

            delay(5); // small delay to reduce read spam
        }

        timeRemaining -= thisTick;
		 PrintfToPI(DebugQueue,"%d",timeRemaining);
    }

    // Total time reached; stop motor
    motor_stop(servoID);
	int rawPos = RAMRead(servoID, AbsolutePosition);
	float currentAngle = 0.0;
	float degreesPerUnit = 1.0;
	float center = 0.0;

	switch (model) {
		case MODEL_0601:
			center = 1024;
			degreesPerUnit = 0.161;
			break;
		case MODEL_0602:
			center = 16384;
			degreesPerUnit = 0.011;
			break;
		case MODEL_0101:
		case MODEL_0201:
			center = 512;
			degreesPerUnit = 0.325;
			break;
	}

	currentAngle = (rawPos - center) * degreesPerUnit;

	// Find nearest multiple of 5
	float snappedAngle = round(currentAngle / 5.0) * 5.0;

	// Move back to the snapped angle
	moveOneAngle(servoID, snappedAngle, 2000,  valueLed, model, false);
}
	// int SetValue = (1<<1) | (1<<valueLed);// | valueLed; //iGreen*4+iBlue*8+iRed*16;		//assign led value 
  
	// playTime=int((float)pTime/11.2);				// 8. Execution time
  
	// pSize = 0x0C;              					// 3.Packet size 7-58
	// cmd   = HSIMULJOG;      					        // 5. CMD
  
	// data[0]=speedGoalLSB;            			    // 8. speedLSB
	// data[1]=speedGoalMSB;              			// 9. speedMSB
	// data[2]=SetValue;                          	// 10. Mode=0;
	// data[3]=servoID;                    			// 11. ServoID
  
	// pID=servoID^playTime;
  
	// lenghtString=4;             					// lenghtData
  
	// ck1=checksum1(data,lenghtString);				//6. Checksum1
	// ck2=checksum2(ck1);							//7. Checksum2

	// Serial.println("Check 1 and 2 ");
	// Serial.println(ck1);
	// Serial.println(ck2);
  
	// pID=servoID;
  
	// dataEx[0] = 0xFF;				// Packet Header
	// dataEx[1] = 0xFF;				// Packet Header	
	// dataEx[2] = pSize;	 		// Packet Size
	// dataEx[3] = pID;				// Servo ID
	// dataEx[4] = cmd;				// Command Ram Write
	// dataEx[5] = ck1;				// Checksum 1
	// dataEx[6] = ck2;				// Checksum 2
	// dataEx[7] = playTime;  		// Execution time	
	// dataEx[8] = data[0];
	// dataEx[9] = data[1];
	// dataEx[10] = data[2];
	// dataEx[11] = data[3];
	
	// sendData(dataEx, pSize);

	// Serial.print("DataPacket playTime: ");
	// Serial.println(dataEx[7], BIN);
	// Serial.print("Actual Calc Playtime: ");
	// Serial.println(playTime);
	// Serial.print("PID: ");
	// Serial.println(pID);

	// unsigned long startTime = millis();
	
	// while ((millis() - startTime) < pTime) {
	// 	delay(1);  // Small delay to prevent CPU overload
	// }
	// test_stop();

	// // Stop the motor by turning torque off
	// Herkulex.torqueOFF(servoID);
	// Herkulex.torqueON(servoID);


// move one servo at target position 0 - 1024
void HerkulexClass::moveOne(int servoID, int targPosition, int pTime, JogLedColor valueLed, HerkulexModel model)
{
  // command-specific checks
  int posLimit = 0;

  switch (model) {
	case MODEL_0601:
		posLimit = 2047;
		break;
	case MODEL_0602:
		posLimit = 32767;
		break;
	case MODEL_0101:
	case MODEL_0201:
		posLimit = 1023;
		break;
}
  if (targPosition > posLimit || targPosition < 0) {
	 PrintfToPI(DebugQueue,"HerkulexLib: moveOne: Error targPosition out of range");
    return;              // speed (goal) non correct
  }
  if ((pTime < 0) || (pTime > 2856)) {
	 PrintfToPI(DebugQueue,"HerkulexLib: moveOne: Error playTime out of range");
	return;
  }
  int LSB=targPosition & 0X00FF;								// MSB Pos
  int MSB=(targPosition & 0XFF00) >> 8;						// LSB Pos

  bool Mode = false;

//   int SetValue = (1<<valueLed);	//assign led value 

  playTime=int((float)pTime/11.2);			// 8. Execution time

  sendSJog(servoID, targPosition, pTime, valueLed, model, Mode, LSB, MSB);
}
//   pSize = 0x0C;          			    	// 3.Packet size 7-58
// //   playTime = 0x5A;
//   cmd   = HSIMULJOG;              				// 5. CMD

//   data[0]=LSB;               			// 8. speedLSB
//   data[1]=MSB;               			// 9. speedMSB
//   data[2]=SetValue;                         // 10. Mode=0;
//   data[3]=servoID;                    		// 11. ServoID

//   pID=servoID^playTime;

//   lenghtString=4;             				// lenghtData

//   ck1=checksum1(data,lenghtString);			//6. Checksum1
//   ck2=checksum2(ck1);						//7. Checksum2

//   pID=servoID;

//   dataEx[0] = 0xFF;				// Packet Header
//   dataEx[1] = 0xFF;				// Packet Header	
//   dataEx[2] = pSize;	 		// Packet Size
//   dataEx[3] = pID;				// Servo ID
//   dataEx[4] = cmd;				// Command Ram Write
//   dataEx[5] = ck1;				// Checksum 1
//   dataEx[6] = ck2;				// Checksum 2
//   dataEx[7] = playTime;  		// Execution time	
//   dataEx[8] = data[0];
//   dataEx[9] = data[1];
//   dataEx[10] = data[2];
//   dataEx[11] = data[3];

//   sendData(dataEx, pSize);



// move one servo to an angle between -160 and 160

void HerkulexClass::moveOneAngle(int servoID, float angle, int pTime, JogLedColor valueLed, HerkulexModel model, bool showStatus) {
    int center = 0;
    float degreesPerUnit = 0.0;
    int posLimit = 0;

    switch (model) {
        case MODEL_0601:
            center = 1024;
            degreesPerUnit = 0.161;
            posLimit = 2047;
            break;
        case MODEL_0602:
            center = 16384;
            degreesPerUnit = 0.011;
            posLimit = 32767;
            break;
        case MODEL_0101:
        case MODEL_0201:
            center = 512;
            degreesPerUnit = 0.325;
            posLimit = 1023;
            break;
    }

    int position = (int)((angle / degreesPerUnit) + center);

    if (position > posLimit || position < 0) {
         PrintfToPI(DebugQueue,"HerkulexLib: moveOneAngle: Error position out of range for model ");
        PrintfToPI(DebugQueue,"%d",model);
        return;
    }

    // Send move command
    moveOne(servoID, position, pTime, valueLed, model);

    // Track the position in real-time while the motor moves
    unsigned long lastUpdate = 0;
    const unsigned long updateInterval = 20;  // 100ms update interval
    uint16_t currentPos = 0;
    bool reachedTarget = false;

    // Track live position until the target is reached
    while (!reachedTarget) {
        // Only update every 'updateInterval' ms
        if (millis() - lastUpdate >= updateInterval) {
            lastUpdate = millis();

			if(showStatus){
				RAMReadSerial(servoID, StatusAll); //optional to print out the statuses of the motor being called 
			}
            // Read the current position
            currentPos = RAMRead(servoID, AbsolutePosition);

            // Check if we've reached the target position
            if (abs(currentPos - position) < 10) {  // Allow a small margin for error
                 PrintfToPI(DebugQueue,"Target Position Reached!");
                reachedTarget = true;  // Exit the loop once the target is reached
            }
        }
        // Allow other tasks to run
        delay(10);  // Small delay to yield control to other tasks (optional)
    }
}

// move all servo at the same time to a position: servo list building
void HerkulexClass::moveAll(int servoID, int Goal, JogLedColor valueLed, HerkulexModel model)
{

	// int center = 0;
    // float degreesPerUnit = 0.0;
    int posLimit = 0;

    switch (model) {
        case MODEL_0601:
            // center = 1024;
            // degreesPerUnit = 0.161;
            posLimit = 2047;
            break;
        case MODEL_0602:
            // center = 16384;
            // degreesPerUnit = 0.011;
            posLimit = 32767;
            break;
        case MODEL_0101:
        case MODEL_0201:
            // center = 512;
            // degreesPerUnit = 0.325;
            posLimit = 1023;
            break;
    }
	if (Goal > posLimit || Goal < 0) {
		PrintfToPI(DebugQueue,"HerkulexLib: moveOne: Error targPosition out of range");
		return;              // speed (goal) non correct
	  }

	  int SetValue = (1<<valueLed);
	  int posLSB=Goal & 0X00FF;					// MSB Pos
	  int posMSB=(Goal & 0XFF00) >> 8;			// LSB Pos

	  addData(posLSB, posMSB, SetValue, servoID);	//add servo data to list, pos mode
}

// move all servo at the same time to a position: servo list building
void HerkulexClass::moveAllAngle(int servoID, float angle, JogLedColor valueLed, HerkulexModel model)
{
	int center = 0;
    float degreesPerUnit = 0.0;
    int posLimit = 0;

    switch (model) {
        case MODEL_0601:
            center = 1024;
            degreesPerUnit = 0.161;
            posLimit = 2047;
            break;
        case MODEL_0602:
            center = 16384;
            degreesPerUnit = 0.011;
            posLimit = 32767;
            break;
        case MODEL_0101:
        case MODEL_0201:
            center = 512;
            degreesPerUnit = 0.325;
            posLimit = 1023;
            break;
    }

    int position = (int)((angle / degreesPerUnit) + center);
	// Serial.println(position);

    if (position > posLimit || position < 0) {
        PrintfToPI(DebugQueue,"HerkulexLib: moveOneAngle: Error position out of range for model: ");
        // Serial.println(model);
        return;
    }
	moveAll(servoID, position, valueLed, model);
}



// move all servo at the same time with different speeds: servo list building
void HerkulexClass::moveSpeedAll(int servoID, int Goal, JogLedColor valueLed, HerkulexModel model)
{
	  if (Goal > 1023 || Goal < -1023)
		return;								 //-1023 <--> 1023 range
	  int Mode = 1; //Speed Mode is always 1 bit shifted extra in set value, position mode shift by 0
	  int SetValue = (1<<Mode) | (1<<valueLed);
	//   int iMode=1;                  		// mode=continous rotation
	//   int iStop=0;                  		// Stop=0

	  // Speed definition
	  int GoalSpeedSign;
	  if (Goal < 0) {
		GoalSpeedSign = (-1)* Goal ;
		GoalSpeedSign |= 0x4000;  //bit n�14 
	  } 
	  else {
		GoalSpeedSign = Goal;
	  }

	  int speedGoalLSB=GoalSpeedSign & 0X00FF; 	      		 // MSB speedGoal 
	  int speedGoalMSB=(GoalSpeedSign & 0xFF00) >> 8;        // LSB speedGoal 

	  addData(speedGoalLSB, speedGoalMSB, SetValue, servoID);		//add servo data to list, speed mode
}

// move all servo with the same execution time

// void HerkulexClass::actionAll(int pTime, bool showStatus)
// {
//     if ((pTime < 0) || (pTime > 2856)) return;

//     int playTime = int((float)pTime / 11.2);  // Execution time
//     int blockSize = 4;  // Each move is 4 bytes: [LSB, MSB, setValue, ID]

//     for (int i = 0; i < conta; i += blockSize) {
//         // Prepare data for just one servo
//         byte tempMove[4];
//         for (int j = 0; j < blockSize; ++j) {
//             tempMove[j] = moveData[i + j];
//         }

//         // Recalculate packet
//         int tempCont = 4; // Only one servo
//         int pSize = 0x08 + tempCont;
//         byte ck1 = checksum1(tempMove, tempCont);
//         byte ck2 = checksum2(ck1);

//         // Packet construction
//         dataEx[0] = 0xFF;
//         dataEx[1] = 0xFF;
//         dataEx[2] = pSize;
//         dataEx[3] = pID;
//         dataEx[4] = HSIMULJOG;
//         dataEx[5] = ck1;
//         dataEx[6] = ck2;
//         dataEx[7] = playTime;

//         for (int j = 0; j < tempCont; ++j) {
//             dataEx[8 + j] = tempMove[j];
//         }

//         // Debug: Show what we're sending
//         Serial.print("Sending to Servo ID ");
//         Serial.println(tempMove[3]);
//         sendData(dataEx, pSize);

//         // === WAIT UNTIL REACHED BEFORE NEXT COMMAND === //
//         const unsigned long updateInterval = 20;
//         unsigned long lastUpdate = millis();
//         bool reached = false;

//         while (!reached) {
//             if (millis() - lastUpdate >= updateInterval) {
//                 lastUpdate = millis();

//                 int goalLSB = tempMove[0];
//                 int goalMSB = tempMove[1];
//                 int setValue = tempMove[2];
//                 int servoID  = tempMove[3];

//                 if ((setValue & 0x01) == 0) {  // Check position mode
//                     int target = (goalMSB << 8) | goalLSB;
//                     uint16_t current = RAMRead(servoID, AbsolutePosition);

//                     // Serial.print("Servo ");
//                     // Serial.print(servoID);
//                     // Serial.print(" current: ");
//                     // Serial.println(current);

//                     if (abs(current - target) < 10) {
//                         reached = true;
//                         Serial.print("✅ Servo ");
//                         Serial.print(servoID);
//                         Serial.println(" reached its target.");
//                     }

//                     if (showStatus) {
//                         RAMReadSerial(servoID, StatusAll);
//                     }
//                 }
//             }

//             delay(5);  // Yield a bit
//         }
//     }

//     conta = 0; // Reset movement buffer
// }
void HerkulexClass::actionAll(int pTime, bool showStatus)
{
	if ((pTime <0) || (pTime > 2856)) return;

    pSize = 0x08 + conta;     	    // 3.Packet size 7-58
	cmd   = HSIMULJOG;		 			// 5. CMD SJOG Write n servo with same execution time
	playTime=int((float)pTime/11.2);// 8. Execution time
 
    pID=0xFE^playTime;
    ck1=checksum1(moveData,conta);	//6. Checksum1
	ck2=checksum2(ck1);				//7. Checksum2

    pID=0xFE;
	dataEx[0] = 0xFF;				// Packet Header
	dataEx[1] = 0xFF;				// Packet Header	
	dataEx[2] = pSize;	 			// Packet Size
	dataEx[3] = pID;				// Servo ID
	dataEx[4] = cmd;				// Command Ram Write
	dataEx[5] = ck1;				// Checksum 1
	dataEx[6] = ck2;				// Checksum 2
	dataEx[7] = playTime;			// Execution time	
	
	// Serial.println("Sending motor movement data:");
    // for (int i = 0; i < conta; i++) {
    //     Serial.print("Data[");
    //     Serial.print(i);
    //     Serial.print("]: ");
    //     Serial.println(dataEx[i + 8]);
    // }

	for (int i=0; i < conta; i++)
		dataEx[i+8]=moveData[i];	// Variable servo data	
	sendData(dataEx, pSize);
	

	const unsigned long updateInterval = 20;
    unsigned long lastUpdate = millis();
    bool allReached = false;

    while (!allReached) {
        if (millis() - lastUpdate >= updateInterval) {
            lastUpdate = millis();
            allReached = true;

            for (int i = 0; i < conta; i += 4) {
                int goalLSB = moveData[i];
                int goalMSB = moveData[i + 1];
                // int setValue = moveData[i + 2];
                int servoID = moveData[i + 3];

                // Only check for position mode (bit 0 of SetValue = 0)
                    int target = (goalMSB << 8) | goalLSB;
                    uint16_t current = RAMRead(servoID, AbsolutePosition);
					// Serial.println(current);

                    if (abs(current - target) <= 10) {
                        allReached = true;
                    }

                    if (showStatus) {
                        RAMReadSerial(servoID, StatusAll);
                    }
                }
            }

            // if (allReached) {
            //     // Serial.println("✅ All servos reached target positions.");
            // }
        }

        delay(5); // Give some breathing room

    conta = 0;  // Reset queued movement buffer
}

// void HerkulexClass::moveOneAngle(int servoID, float angle, int pTime, JogLedColor valueLed, HerkulexModel model) {
//     // int conversionFactor = is0601 ? 2 : 1; // 0601 ranges from 0-2047, 2x that of 0201/0101
// 	// int position = (int)(conversionFactor * (angle/0.325 + 512));
	
// 	// moveOne(servoID, position, pTime, valueLed, is0601);
// 	int center = 0;
//     float degreesPerUnit = 0.0;
// 	int posLimit = 0;

//     switch (model) {
//         case MODEL_0601:
//             center = 1024;
//             degreesPerUnit = 0.161;
// 			posLimit = 2047;
//             break;
//         case MODEL_0602:
//             center = 16384;
//             degreesPerUnit = 0.011;
// 			posLimit = 32767;
//             break;
//         case MODEL_0101:
//         case MODEL_0201:
//             center = 512;
//             degreesPerUnit = 0.325;
// 			posLimit = 1023; 
//             break;
//     }

//     int position = (int)((angle / degreesPerUnit) + center);

// 	if (position > posLimit || position < 0) {
//         Serial.print("HerkulexLib: moveOneAngle: Error position out of range for model ");
//         Serial.println(model);
//         return;
//     }

//     moveOne(servoID, position, pTime, valueLed, model); // Pass model directly

// 	const int maxAttempts = 100;
// 	const int delayMs = 10;
// 	int attempts = 0;

// 	while (attempts++ < maxAttempts) {
// 		uint16_t currentPos = RAMRead(servoID, AbsolutePosition);

// 		if (currentPos == position) {
// 			Serial.println("Achieved Position");
// 			break;
// 		}
// 		delay(delayMs);
// 		}
// }

// write registry in the RAM: one byte 
// void HerkulexClass::writeRegistryRAM(int servoID, int address, int writeByte)
// {
//   pSize = 0x0A;               	// 3.Packet size 7-58
//   pID   = servoID;     			// 4. Servo ID - 253=all servos
//   cmd   = HRAMWRITE;          	// 5. CMD
//   data[0]=address;              // 8. Address
//   data[1]=0x01;               	// 9. Lenght
//   data[2]=writeByte;            // 10. Write error=0
 
//   lenghtString=3;             	// lenghtData

//   ck1=checksum1(data,lenghtString);	//6. Checksum1
//   ck2=checksum2(ck1);				//7. Checksum2

//   dataEx[0] = 0xFF;			// Packet Header
//   dataEx[1] = 0xFF;			// Packet Header	
//   dataEx[2] = pSize;	 	// Packet Size
//   dataEx[3] = pID;			// Servo ID
//   dataEx[4] = cmd;			// Command Ram Write
//   dataEx[5] = ck1;			// Checksum 1
//   dataEx[6] = ck2;			// Checksum 2
//   dataEx[7] = data[0]; 		// Address 52
//   dataEx[8] = data[1]; 		// Length
//   dataEx[9] = data[2]; 		// Value1
//   dataEx[10]= data[3]; 		// Value2

//   sendData(dataEx, pSize);

// }

// // write registry in the EEP memory (ROM): one byte 
// void HerkulexClass::writeRegistryEEP(int servoID, int address, int writeByte)
// {
//   pSize = 0x0A;                  // 3.Packet size 7-58
//   pID   = servoID;     	         // 4. Servo ID - 253=all servos
//   cmd   = HEEPWRITE;             // 5. CMD
//   data[0]=address;               // 8. Address
//   data[1]=0x01;                  // 9. Lenght
//   data[2]=writeByte;             // 10. Write error=0
 
//   lenghtString=3;           	 // lenghtData

//   ck1=checksum1(data,lenghtString);	//6. Checksum1
//   ck2=checksum2(ck1);				//7. Checksum2

//   dataEx[0] = 0xFF;			// Packet Header
//   dataEx[1] = 0xFF;			// Packet Header	
//   dataEx[2] = pSize;		// Packet Size
//   dataEx[3] = pID;			// Servo ID
//   dataEx[4] = cmd;			// Command Ram Write
//   dataEx[5] = ck1;			// Checksum 1
//   dataEx[6] = ck2;			// Checksum 2
//   dataEx[7] = data[0]; 		// Address 52
//   dataEx[8] = data[1]; 		// Length
//   dataEx[9] = data[2]; 		// Value1
//   dataEx[10]= data[3]; 		// Value2

//   sendData(dataEx, pSize);

// }



// Private Methods //////////////////////////////////////////////////////////////

// checksum1
int HerkulexClass::checksum1(byte* data, int lenghtString)
{
  XOR = 0;	
  XOR = XOR ^ pSize ^ pID ^ cmd;
//   XOR = XOR ^ pID;
//   XOR = XOR ^ cmd;
  for (int i = 0; i < lenghtString; i++) 
  {
    XOR = XOR ^ data[i];
  }
  return XOR&0xFE;
}


// checksum2
int HerkulexClass::checksum2(int XOR)
{
  return (~XOR)&0xFE;
}

// add data to variable list servo for syncro execution
void HerkulexClass::addData(int GoalLSB, int GoalMSB, int set, int servoID)
{
  moveData[conta++]=GoalLSB;  
  moveData[conta++]=GoalMSB;
  moveData[conta++]=set;
  moveData[conta++]=servoID;
}

// Sending the buffer long lenght to Serial port
void HerkulexClass::sendData(byte* buffer, int lenght){

	// clearBuffer(); 		//clear the serialport buffer - try to do it!
	(void) uart_write_bytes(port, buffer, lenght);
        // switch (port)
		// {
		// 	// case Serial:
		// 	// 	SwSerial.write(buffer, lenght);
		// 	// 	delay(1);
		// 	// 	break;
		// 	case UART_NUM_0:
		// 		delay(1);
		// 		break;
		// 	case UART_NUM_1:
		// 		delay(1);
		// 		break;
		// 	case UART_NUM_2:
		// 		delay(1);
		// 		break;
		// 	case UART_NUM_MAX:
		// 		delay(1);
		// 		break;
		// 	#if defined (__AVR_ATmega1280__) || defined (__AVR_ATmega128__) || defined (__AVR_ATmega2560__)
		// 	case HSerial1:
		// 		Serial1.write(buffer, lenght);
		// 		delay(1);
		// 		break;
		// 	case HSerial2:
		// 		Serial2.write(buffer, lenght);
		// 		delay(1);
		// 		break;
		// 	case HSerial3:
		// 		Serial3.write(buffer, lenght);
		// 		delay(1);
		// 		break;
		// 	#endif
		// }
}

// * Receiving the lenght of bytes from Serial port

// int HerkulexClass::readData(int size) {
//     int i = 0;
//     int beginsave = 0;
//     int Time_Counter = 0;

//     while ((SwSerial.available() < size) && (Time_Counter < TIME_OUT)) {
//         Time_Counter++;
//         delayMicroseconds(1000);  // wait 1ms
//     }

//     if (SwSerial.available() < size) {
//         Serial.println("readData: Timeout — not enough bytes available");
//         return 0;
//     }

//     while (SwSerial.available() > 0 && i < 128) {  // Increase safety buffer limit
//         byte inchar = (byte)SwSerial.read();

//         if ((inchar == 0xFF) && ((byte)SwSerial.peek() == 0xFF)) {
//             beginsave = 1;
//             i = 0; // Restart buffer on new packet
//         }

//         if (beginsave == 1 && i < sizeof(dataEx)) {
//             dataEx[i] = inchar;
//             i++;
//         }
//     }

//     return i;  // Return number of bytes read
// }

int HerkulexClass::readData(int size)
{
	int i = 0;
	byte* bpt = &dataEx[9];
	(void) uart_read_bytes(port, bpt, size, pdMS_TO_TICKS(10));
    // int beginsave=0;
    // int Time_Counter=0;
    // switch (port)
	// {
	// // case Serial:

    // //     while((SwSerial.available() < size) & (Time_Counter < TIME_OUT)){
    // //     		Time_Counter++;
    // //     		delayMicroseconds(1000);  //wait 1 millisecond for 10 times
	// // 	}
        	
	// // 	while (SwSerial.available() > 0){
	// // 		byte inchar = (byte)SwSerial.read();
	// // 		if ( (inchar == 0xFF) & ((byte)SwSerial.peek() == 0xFF) ){
	// // 				beginsave=1; 
	// // 				i=0; 				 // if found new header, begin again
	// // 		}
	// // 		if (beginsave==1 && i<size) {
	// // 			   dataEx[i] = inchar;
	// // 			   i++;
	// // 		}
	// // 	}
	// // 	SwSerial.flush();
	// // 	break;
	// case UART_NUM_0:
	// 			delay(1);
	// 			break;
	// 		case UART_NUM_1:
	// 			delay(1);
	// 			break;
	// 		case UART_NUM_2:
	// 			delay(1);
	// 			break;
	// 		case UART_NUM_MAX:
	// 			delay(1);
	// 			break;
	
	// #if defined (__AVR_ATmega1280__) || defined (__AVR_ATmega128__) || defined (__AVR_ATmega2560__)
	// case HSerial1:
	// 	while((Serial1.available() < size) & (Time_Counter < TIME_OUT)){
    //     		Time_Counter++;
    //     		delayMicroseconds(1000);
	// 	}      	
	// 	while (Serial1.available() > 0){
    //   		byte inchar = (byte)Serial1.read();
	// 		//printHexByte(inchar);
    //     	if ( (inchar == 0xFF) & ((byte)Serial1.peek() == 0xFF) ){
	// 					beginsave=1;
	// 					i=0; 						
    //          }
    //         if (beginsave==1 && i<size) {
    //                    dataEx[i] = inchar;
    //                    i++;
	// 		}
	// 	}
	// 	break;
	
	// case HSerial2:
	//     while((Serial2.available() < size) & (Time_Counter < TIME_OUT)){
    //     		Time_Counter++;
    //     		delayMicroseconds(1000);
	// 	}
        	
	// 	while (Serial2.available() > 0){
	// 		byte inchar = (byte)Serial2.read();
	// 		if ( (inchar == 0xFF) & ((byte)Serial2.peek() == 0xFF) ){
	// 				beginsave=1;
	// 				i=0; 					
	// 		}
	// 		if (beginsave==1 && i<size) {
	// 			   dataEx[i] = inchar;
	// 			   i++;
	// 		}
	// 	}
	// 	break;

	// case HSerial3:
	// 	while((Serial3.available() < size) & (Time_Counter < TIME_OUT)){
	// 		Time_Counter++;
	// 		delayMicroseconds(1000);
	// 	}
		
	// 	while (Serial3.available() > 0){
	// 		byte inchar = (byte)Serial3.read();
	// 		if ( (inchar == 0xFF) & ((byte)Serial3.peek() == 0xFF) ){
	// 				beginsave=1;
	// 				i=0; 
	// 		}
	// 		if (beginsave==1 && i<size) {
	// 			   dataEx[i] = inchar;
	// 			   i++;
	// 		}
	// 	}
	// 	break;
	// #endif
	// }
	return i;
}

// //clear buffer in the serial port - better - try to do this
// void HerkulexClass::clearBuffer()
// {
//   switch (port)
// 	{
// 	case SSerial:
//                 SwSerial.flush();
//                 delay(1);
//                 break;
// 	#if defined (__AVR_ATmega1280__) || defined (__AVR_ATmega128__) || defined (__AVR_ATmega2560__)
// 	case HSerial1:
// 				Serial1.flush();
// 				while (Serial1.available()){
// 				Serial1.read();
// 				delayMicroseconds(200);
// 				}
//
// 		break;
// 	case HSerial2:
// 	            Serial2.flush();
// 				while (Serial2.available()){
// 				Serial2.read();
// 				delayMicroseconds(200);
// 				}
// 		break;
// 	case HSerial3:
// 	            Serial3.flush();
// 				while (Serial3.available()){
// 					Serial3.read();
// 					delayMicroseconds(200);
// 				}
//
// 		break;
// 	#endif
// 	}
// }

// void HerkulexClass::printHexByte(byte x)
// {
//   Serial.print("0x");
//   if (x < 16) {
//     Serial.print('0');
//   }
//     Serial.print(x, HEX);
//     Serial.print(" ");
//
// }





 HerkulexClass Herkulex;