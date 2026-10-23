//
// Created by gabri on 6/21/2026.
//

#include "DBOXprotocol.h"
// NOTE: this is a vendored copy for the local DBoxEcho hardware-verification
// build only (Urchin_Firmware/lib/D-BOX-vendored). The only intentional
// difference from Gabe's original (Clam/D-BOX/src/DBOX-Protocol/DBOXprotocol.cpp)
// is right below: boost::crc_optimal is swapped for a hand-rolled CRC-16
// using the identical polynomial/init/xorout/reflect settings, since Boost
// isn't available for the ESP32 (xtensa) cross-compiler. Produces
// bit-identical checksums to the original. receive()'s header-peek retry
// loop must otherwise stay behaviorally identical to the original (including
// the try/catch around RingBufferUnderflowException) -- a prior version of
// this file dropped that try/catch by mistake, which crashed on any real
// UART link since peek() throws whenever a header hasn't fully arrived yet.

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
            // IPort::peek() throws RingBufferUnderflowException (rather than
            // returning a short count) whenever fewer than raw.size() bytes
            // have arrived so far -- the normal case while a header is still
            // trickling in over a live UART. This must be caught here and
            // treated as "not ready yet, retry", exactly like Gabe's
            // original boost-based version does; letting it escape crashes
            // the process (and would abort/reboot real hardware, since
            // ESP-IDF builds typically have C++ exceptions disabled).
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
    // CRC-16, MSB-first (non-reflected), init 0xFFFF, xorout 0x0000 --
    // equivalent to boost::crc_optimal<16, GeneratorPolynomial, 0xFFFF, 0x0000, false, false>

    static uint16_t crc16_step(uint16_t crc, uint8_t byte) {
        crc ^= static_cast<uint16_t>(byte) << 8;
        for (int i = 0; i < 8; ++i) {
            crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ GeneratorPolynomial)
                                  : static_cast<uint16_t>(crc << 1);
        }
        return crc;
    }

    int ComputeChecksumCRC(uint16_t *CRC,std::span<uint8_t> Payload) {
        if ((Payload.size() * CHAR_BIT) > MAX_CRC_Payload_BIT)
            return -1;

        uint16_t crc = 0xFFFF;
        for (uint8_t b : Payload)
            crc = crc16_step(crc, b);

        *CRC = crc;
        return 0;
    }


    uint16_t span_to_uint16_native(std::span<const uint8_t, 2> bytes) {
        // Reinterprets the 2-byte span as a uint16_t directly
        return std::bit_cast<uint16_t>(std::array<uint8_t, 2>{bytes[0], bytes[1]});
    }


}