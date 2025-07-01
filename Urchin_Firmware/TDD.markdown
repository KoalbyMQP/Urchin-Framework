# Objective
    The goal of the project is to provide a safe and expandable platform for the Raspberry Pi to interface with motors and sensors with a seamless abstract interface
## Goal
    
    The API will have the least amount of user faceing funtion call changes
    The possabily of expantion of scoop shall be planed for and included in designe
    The framework should be addaped to multipule purpourse will the least amount of changes
    Multiple instances of fermware shall be versiond from one tree with the leat amount of files changes
## Success criteria
# Background

# Scope
# Design
The Urchin framework is divided into four sectors, Each with their own aquatic name
1. Urchin firmware
2. Crab API
3. Mollusk Peer
4. Coral Multiplexor

## Lucid chart
## Overview
## Infrastructure
## Detailed Design
## Expansion / Adaptability
## Dependencies

### C
* [Free RTOS](https://www.freertos.org/ "Title").
* [Espidf](https://idf.espressif.com/ "Title").

### Python
See standards set by 
* Threading
* Serial
# Standards
 ## Python
 ## C/C++
**When should I use C or C++?**
To handle libraries that are already in C++ we will keep ore files in C++ but your code should be writen as simply as possible
Because of that try to use C as much as possible. Only use C++ if you need its exclusive features 
 ### Power of ten
The rules are intended to eliminate certain C coding practices that make code difficult to review or statically analyze.
The bellow rules have been modified in **bold** to better fit this project
1. Avoid complex flow constructs, such as goto and recursion.
2. All loops must have fixed bounds. This prevents runaway code.
3. **Try to** Avoid heap memory allocation after initialization.
4. Restrict functions to a single printed page.
5. Use a minimum of two runtime assertions per function.
6. Restrict the scope of data to the smallest possible.
7. Check the return value of all non-void functions, or cast to void to indicate the return value is useless.
8. Use the preprocessor only for header files and simple macros. **macros to enforce project rules are allowed and will be exclusively contained in the standards.h**
9. Limit pointer use to a single dereference, and do not use function pointers. **They are only allowed in a "static const" context**
10. Compile with all possible warnings active; all warnings should then be addressed before release of the software.

**Custom**
1. Use of "prinf" , "fprintf", "vprintf" , and "vfprintf" are forbidden. Use "PrintToPi" Instead
2. Any change to the Shipping.c and Shipping.h or its instantiation in main.cpp needs to be reviewed by two project managers
3. Use of c++ strings is banded
4. Instead of using "malloc" and "free" use "pvPortMalloc" and "vPortFree"

   **ALL pull requests must meet these standards!**
## Git Policy
<pre>
Git branch policy:<br>
Main                    # Stable, production-ready<br>
├── develop            # Integration branch<br>
├── feature/package-name/feature-description<br>
├── bugfix/package-name/bug-description<br>
└── release/v1.2.0    
</pre>

### Python
<pre>
Python Git project structure <br>
├── README.md<br>
└── packages<br>
├── common<br>
│     └── README.md<br>
└── foo<br>
├── README.md<br>
├── examples<br>
├── scripts<br>
│   ├── build.py<br>
│   ├── release.py<br>
│   └── test.py<br>
└── test<br>
</pre>
### C/C++
<pre>
C/C++ Git project structure <br>
├──"Project".README<br>
├──include<br>
├──lib<br>
└──src<br>
    ├──main.cpp<br>
    └── ...<br>
</pre>
# Crab API
This API provides many comforts to the user 
* Abstraction of sending data to the firmware
* Tools for management of code execution and flow
  * Chained function will be forced to resolve before execution will continue
  * Send can be feed functions to be run if and when certain errors occur
## Function Calls
`InitUrchin()`
`Ticket=SEND(Data,Type,LIST[ITEM],Restraints,LIST[Reaction],Chained)`
`CHECK(Ticket)`
`DROP(Ticket)`

## Reaction
* The API has the ability to run functions appon receiving a completion code or error code from the firmware

##

# Urchin Firmware
This Firmware is the brains of the operation.
The Firmware will run Free RTOS ontop of the espidf framework

## Communication
There are three sides to communication on the esp32
1. PI and Urchin
   * These two communicate with packets to send {VPID (Virtual Process identifier), Stream, Data , Checksum}
   * They use seral UART ports. ESP32 will use UART-0 that is connected to the USB. The PI will use one of there USB ports and will scan to see what "COM" is being used.
   * In the event of a detached instance/installation of the Firmware bluetooth should be used to send packets with the same packet type
2. Motors and Sensors
   * They will use a mix of I2C and Uart
     *  Dynamixel: Will use there packets to pass down the motor chain
     *  HerkuleX: Will use there packets to pass down the motor chain
     *  I2C: 
3. Peer to Peer
   * The main Urchin firmware will communicate to other esp32s over CAN
   * Under the special circumstance of a wireless peer ESPNOW will be used as it has a RTT of ≈6ms 
