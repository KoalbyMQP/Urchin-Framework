//
// Created by gabri on 6/21/2026.
//

#ifndef D_BOX_DOLPHIN_H
#define D_BOX_DOLPHIN_H
#include <fstream>
#include <string>
#include "../Transport-Interface/sys-seral.h"

struct DBOX {
    uint8_t VPID;
    uint8_t Stream;
    std::span<uint8_t> Payload;
};

struct DBOXraw {
    uint8_t VPID;
    uint8_t Stream;
    uint16_t PayloadSize;
    uint16_t ChecksumHeaderCRC;
    uint16_t ChecksumPlayloadCRC;
    std::span<uint8_t> Payload;
};

class DBOXprotocol
{
public:

    explicit DBOXprotocol(ISerialPort& port,std::string_view validater, std::string_view dump_path)
        : port(port), validater(validater) ,dump(dump_path, std::ios::binary | std::ios::app){}

    bool send(DBOX *packet);

    bool ship(DBOXraw *packet);

    bool receive(DBOX *packet);

    std::string HuntPair();
    bool SendValidater(std::string_view port);
    bool CheckValidater();

    uint16_t ComputeChecksumCRC(std::span<uint8_t> Payload);








    void connect(std::string_view port);
    void disconnect() const {port.close();}

private:
    ISerialPort& port;
    std::string validater;
    std::ofstream dump;
};
#endif //D_BOX_DOLPHIN_H