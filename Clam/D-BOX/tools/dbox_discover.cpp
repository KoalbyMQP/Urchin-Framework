//
// dbox_discover: find the robot's ESP32 on a real serial port using D-BOX
// discovery, then send it one command and print the reply.
//
//   ./dbox_discover                   try every serial port find() lists
//   ./dbox_discover /dev/ttyUSB0 ...  try only these ports
//
// Works with either ESP32 test firmware:
//   DBoxMotor  answers S "Urchin", then replies to N "Info" with E "OK DBoxMotor v1 ..."
//   DBoxEcho   echoes every packet, so both come straight back
//
// Linux only (Raspberry Pi, or WSL with the ESP32 attached via usbipd).
//

#include <DBOXprotocol.h>
#include <Dconfig/DconfigUart.h>
#include <Discovery/UartDiscovery.h>
#include <Transport-Interface/UART/Linux/linux-sys-seral.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace DBOXProtocol;

int main(int argc, char** argv) {
    DconfigUart cfg("Urchin", "", 115200, 20);
    UartDiscovery disco(true, &cfg);
    LinuxSerialPort port(&disco);
    DBOXprotocol proto(&port, "/dev/null");

    std::vector<std::string> ports;
    for (int i = 1; i < argc; ++i) ports.push_back(argv[i]);
    if (ports.empty()) ports = port.find();

    if (ports.empty()) {
        std::printf("No serial ports found. Is the ESP32 plugged in (and attached to WSL)?\n");
        return 2;
    }

    // Same steps as UartDiscovery::ActiveDiscover(), one port at a time so
    // progress can be printed.
    std::string found;
    for (const auto& p : ports) {
        std::printf("Trying %-22s ... ", p.c_str());
        std::fflush(stdout);
        if (disco.ActiveDiscoverSingle(p, disco.attemptsPerPort)) {
            std::printf("\"%s\" answered\n", cfg.identity.c_str());
            found = p;
            break;
        }
        std::printf("no answer\n");
    }

    if (found.empty()) {
        std::printf("\nRobot not found on any port.\n");
        return 1;
    }

    std::printf("\nFound the robot on %s\n", found.c_str());
    if (!port.openraw(found.c_str(), cfg.baud, cfg.timeout)) {
        std::printf("...but could not reopen it.\n");
        return 1;
    }

    // Give a DBoxMotor board a moment in case opening the port reset it.
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    DBOX cmd(0, 'N', "Info");
    proto.send(&cmd);
    std::printf("Sent:  N \"Info\"\n");

    DBOX reply;
    if (proto.receive(&reply, 2000)) {
        std::string text(reply.Payload.begin(), reply.Payload.end());
        std::printf("Got:   %c \"%s\"\n", (char)reply.Stream, text.c_str());
        return 0;
    }
    std::printf("No reply to Info within 2 s.\n");
    return 1;
}
