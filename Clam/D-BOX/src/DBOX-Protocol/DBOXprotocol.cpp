//
// Created by gabri on 6/21/2026.
//

#include "DBOXprotocol.h"
#include <boost/crc.hpp>

namespace DBOXProtocol {
    bool DBOXprotocol::send(DBOX *packet) {

        //check the size against max payload size
        if (std::numeric_limits<uint16_t>::max() < packet->Payload.size()) {
            return false;
        }

        //Start filling the box
        DBOXraw FullBox = DBOXraw(*packet);

        // use ship on struct
        return ship(&FullBox);
    }

    bool DBOXprotocol::ship(DBOXraw *packet){
        std::vector<uint8_t> bytes = static_cast<std::vector<uint8_t>>(*packet);

        std::span<const uint8_t> byte_view{
            bytes.data(),
            bytes.size()
        };

        size_t bytes_sent = this->port->write(byte_view, byte_view.size());

        return bytes_sent == byte_view.size();
    }

    bool DBOXprotocol::receive(DBOX* packet) {
        if (!packet)
            return false;

        uint16_t CRC = 0;

        constexpr size_t HeaderSize =
            sizeof(DBOXraw::VPID) +
            sizeof(DBOXraw::Stream) +
            sizeof(DBOXraw::PayloadSize);

        std::array<uint8_t, HeaderSize> headerBuffer{};
        std::array<uint8_t, 2> crcBuffer{};;

        std::span<uint8_t> Header(headerBuffer);
        std::span<uint8_t, 2> HeaderCRC(crcBuffer);

        // Look for Delimiter
        while (true)
        {
            uint8_t byte = 0;
            std::span<uint8_t> buffer(&byte, 1);

            while (true)
            {
                if (port->read(buffer, 1) == 1)
                    break;

                port->wait(WaitForDataMS);
            }

            if (byte == '\a')
                break;

            dump.write(reinterpret_cast<const char*>(&byte), 1);
        }

        //found an instance of a start marker

        // Wait for / read header + CRC

        while (true)
        {
            try
            {
                std::array<uint8_t, HeaderSize + sizeof(uint16_t)> raw{};

                std::span<uint8_t> rawSpan(raw);
                //Assuming we don't have frame alignment, peek the header
                // if data is still ariveing then wha
                if (port->peek(rawSpan, raw.size()) == raw.size()) {
                    std::copy_n(
                    raw.begin(),
                    HeaderSize,
                    Header.begin()
                );

                    std::copy_n(
                        raw.begin() + HeaderSize,
                        sizeof(uint16_t),
                        HeaderCRC.begin()
                    );

                    // genarate teh CRC from the received header
                    if (ComputeChecksumCRC(&CRC, Header) == -1){
                        return false;
                    }

                    // Verify header CRC
                    // if the CRC failed then move to the next delimiter
                    if (CRC == span_to_uint16_native(HeaderCRC)){
                        break;
                    }
                }


            }
            catch (const RingBuffer::RingBufferUnderflowException&)
            {
                port->wait(WaitForDataMS);
            }
        }



        //At this point we have proved frame alignment from the CRC so the payload size is correct
        //We can safely drop the head and read the payload.

        // Extract header fields

        packet->VPID = Header[0];
        packet->Stream = Header[1];

        uint16_t payloadSize =
            Header[2] |
            (static_cast<uint16_t>(Header[3]) << 8);


        // Drop the header to prep for reading the paylaod
        port->drop(HeaderSize + sizeof(uint16_t));


        // Read payload
        //DONOT peek the payload thr ring buffer cant fit the max sized payload.
        packet->Payload.resize(payloadSize);

        size_t offset = 0;

        while (offset < payloadSize)
        {
            auto remaining = payloadSize - offset;

            std::span<uint8_t> destination(
                packet->Payload.data() + offset,
                remaining
            );

            size_t got = port->read(destination, remaining);

            if (got == 0)
            {
                port->wait(WaitForDataMS);
                continue;
            }

            offset += got;
        }


        return true;
    }



    // generator polynomial from https://users.ece.cmu.edu/~koopman/crc/
    //

    using my_crc = boost::crc_optimal<
        16,       // CRC width in bits
        GeneratorPolynomial,   // generator polynomial without the top x^16 term
        0xFFFF,   // initial remainder
        0x0000,   // final xor value
        false,    // reflect input bytes
        false     // reflect final remainder
    >;

    int ComputeChecksumCRC(uint16_t *CRC,std::span<uint8_t> Payload) {
        if ((Payload.size() * CHAR_BIT) > MAX_CRC_Payload_BIT)
            return -1;

        my_crc crc;
        crc.process_bytes(Payload.data(), Payload.size());

        *CRC = crc.checksum();
        return 0;
    }


    uint16_t span_to_uint16_native(std::span<const uint8_t, 2> bytes) {
        // Reinterprets the 2-byte span as a uint16_t directly
        return std::bit_cast<uint16_t>(std::array<uint8_t, 2>{bytes[0], bytes[1]});
    }


}