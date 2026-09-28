//
// Created by gabri on 6/21/2026.
//

#ifndef D_BOX_DOLPHIN_H
#define D_BOX_DOLPHIN_H
#include <fstream>
#include <string>
#include <Transport-Interface/IPort.h>
#include <climits>
#include <cstddef>
#include <Discovery/Discovery.h>
#include <RingBuffer/RingBufferException.hpp>

namespace DBOXProtocol {
#define GeneratorPolynomial 0x9eb2
#define MAX_CRC_Payload_BIT 135
#define WaitForDataMS 20
#define MAX_CRC_Payload_BYTE ((MAX_CRC_Payload_BIT) / CHAR_BIT)

    int ComputeChecksumCRC(uint16_t *CRC,std::span<uint8_t> Payload);
    uint16_t span_to_uint16_native(std::span<const uint8_t, 2> bytes);
    struct DBOX {
        uint8_t VPID{};
        uint8_t Stream{};
        std::vector<uint8_t> Payload{};

        DBOX(uint8_t vpid,
             uint8_t stream,
             std::string_view text)
            : VPID(vpid),
              Stream(stream),
              Payload(
                  reinterpret_cast<const uint8_t*>(text.data()),
                  reinterpret_cast<const uint8_t*>(text.data()) + text.size())
        {}

        DBOX() = default;

        bool operator==(const DBOX& other) const {
            return VPID == other.VPID &&
                   Stream == other.Stream &&
                   Payload == other.Payload;
        }
    };
#pragma pack(push, 1)
    struct DBOXraw {
        uint8_t VPID;
        uint8_t Stream;
        uint16_t PayloadSize;
        uint16_t ChecksumHeaderCRC;
        //uint16_t ChecksumPlayloadCRC;
        std::vector<uint8_t> Payload{};

        DBOXraw(const DBOX& packet)
     : VPID(packet.VPID),
       Stream(packet.Stream),
       PayloadSize(static_cast<uint16_t>(packet.Payload.size())),
       Payload(packet.Payload)
        {
            std::vector<uint8_t> bytes;
            bytes.reserve(sizeof(VPID) + sizeof(Stream) + sizeof(PayloadSize));

            bytes.push_back(VPID);
            bytes.push_back(Stream);

            // Little-endian uint16_t
            bytes.push_back(static_cast<uint8_t>(PayloadSize & 0xFF));
            bytes.push_back(static_cast<uint8_t>((PayloadSize >> 8) & 0xFF));

            std::span<uint8_t> byte_view{
                bytes.data(),
                bytes.size()
            };
            ComputeChecksumCRC(&ChecksumHeaderCRC, byte_view);
        }
        DBOXraw() = default;

        operator std::vector<uint8_t>() const
        {
            std::vector<uint8_t> bytes;
            bytes.reserve(7 + Payload.size());

            bytes.push_back(7); // bell char for delimataer
            bytes.push_back(VPID);
            bytes.push_back(Stream);

            // Little-endian uint16_t
            bytes.push_back(static_cast<uint8_t>(PayloadSize & 0xFF));
            bytes.push_back(static_cast<uint8_t>((PayloadSize >> 8) & 0xFF));

            bytes.push_back(static_cast<uint8_t>(ChecksumHeaderCRC & 0xFF));
            bytes.push_back(static_cast<uint8_t>((ChecksumHeaderCRC >> 8) & 0xFF));

            bytes.insert(bytes.end(), Payload.begin(), Payload.end());

            return bytes;
        }


    };
#pragma pack(pop)

    class DBOXprotocol
    {
    public:



        explicit DBOXprotocol(IPort* port, std::string_view dump_path)
            : port(port) ,dump(std::string(dump_path), std::ios::binary | std::ios::app) {


        };



        /**
         * Function to send a DBOX packet
         * @param packet The DBOX packet
         * @return True if the packet was sent False otherwise
         */
        bool send(DBOX *packet);

        /**
         *
         * @param packet
         * @return
         */
        bool receive(DBOX *packet);

        /**
         *
         * @param port
         */
        void connect(std::string_view port);

        /**
         *
         */
        void disconnect() const {port->close();}


        /**
         *
         * @return
         */
        int start() const {
            return 0;
            //return this->port->start();
        }


        /**
         *
         * @return
         */
        int end() const {
            return 0;
            //return this->port->end();
        }

        /**
         * The communication interface
         */
        IPort* port;



    private:


        /**
         *
         * @param packet
         * @return
         */
        bool ship(DBOXraw *packet);
        std::ofstream dump;
    };

}
#endif //D_BOX_DOLPHIN_H