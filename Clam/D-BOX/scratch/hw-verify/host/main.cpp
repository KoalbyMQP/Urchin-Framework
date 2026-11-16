// New file: local hardware verification test for DBOX over a real Windows
// COM port talking to a real ESP32 running env:DBoxEcho. Not part of
// Gabe's Clam/D-BOX code, never committed.
//
// Usage: dbox_hw_test.exe COM5

#include <DBOXprotocol.h>
#include "WinIPort.h"
#include <iostream>
#include <iomanip>

static void printHex(const std::vector<uint8_t>& bytes) {
    for (auto b : bytes)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)b << ' ';
    std::cout << std::dec << '\n';
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <COM port, e.g. COM5>\n";
        return 1;
    }

    std::string comPort = argv[1];
    WinIPort port(comPort, 115200);

    if (!port.open()) {
        std::cerr << "Failed to open " << comPort << "\n";
        return 1;
    }

    DBOXProtocol::DBOXprotocol proto(&port, "dboxdump.bin");

    DBOXProtocol::DBOX outgoing(7, 'T', "Testing a different payload!");

    std::cout << "Sending DBOX packet: VPID=" << (int)outgoing.VPID
              << " Stream=" << outgoing.Stream
              << " Payload=\"" << std::string(outgoing.Payload.begin(), outgoing.Payload.end())
              << "\"\n";

    if (!proto.send(&outgoing)) {
        std::cerr << "Send failed\n";
        return 1;
    }

    std::cout << "Waiting for echo back from ESP32...\n";

    DBOXProtocol::DBOX incoming;
    if (!proto.receive(&incoming)) {
        std::cerr << "Receive failed\n";
        return 1;
    }

    std::cout << "Received: VPID=" << (int)incoming.VPID
              << " Stream=" << incoming.Stream
              << " Payload=\"" << std::string(incoming.Payload.begin(), incoming.Payload.end())
              << "\"\n";

    if (incoming == outgoing) {
        std::cout << "PASS: round-trip packet matches exactly.\n";
        return 0;
    }

    std::cout << "FAIL: round-trip packet did not match.\n";
    return 1;
}
