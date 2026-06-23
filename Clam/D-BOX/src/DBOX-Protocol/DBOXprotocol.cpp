//
// Created by gabri on 6/21/2026.
//

#include "DBOXprotocol.h"

bool send(DBOX *packet) {
    //check the size against max payload size
    //compute ChecksumHeader
    //compute ChecksumHeader2
    //compute ChecksumWhole
    //build struct
    // use ship on struct
}

bool ship(DBOXraw *packet) {
    // convert DBOXraw to span
    // then use port->write(span);
}

bool receive(DBOX *packet) {

    //loop looking for delimiter

    // once found get bytes with peek into header buffer
    // if caught RingBufferUnderflowException then try again later

    // else
    // check the header buffer with the two checksums
    // if failed restart and look for next delimater and append byte to the discard file pointer with dump << btye";,dump.flush();
    // else use  to get the whole packet and cast it to the striped down struct
    //return struct


}

std::string HuntPair() {
    // will get the list of comm ports and try to connect to them
    // if connected wait for a port to stabilise then run SendValidater and CheckValidater 3 times to see if they get back to us
    // if they sent it back correctly then we are good and close the port and return the port name

    // use python as losse example
    //    def WhoaAreYou(self, COM: str) -> bool:
    // print("checking a com port")
    // VPID: int = 0
    //
    // try:
    //     #Open COM connection
    //     bus = serial.Serial(
    //         COM,
    //         115200,
    //         timeout=1,
    //         dsrdtr=False,
    //         rtscts=False
    //     )
    //
    //     time.sleep(0.5)  # give CP2102 + MCU time to settle
    //
    //     # flush any garbage from boot/reset
    //     bus.reset_input_buffer()
    //     bus.reset_output_buffer()
    //
    //     message = b'Validate'
    //
    //     if self.Debug:
    //         print("sending Validate")
    //
    //     # retry a few times (devices often miss first packet after connect)
    //     for _ in range(3):
    //         self.send_packet_internal(VPID, message, bus)
    //
    //         output = self.CheckForValidate(bus)
    //         if output:
    //             return True
    //
    //         time.sleep(0.2)
    //
    //     return False
    //
    // except serial.SerialException as e:
    //     if self.Debug:
    //         print(f"Serial error on {COM}: {e}")
    //     return False
    //
    // finally:
    //     try:
    //         bus.close()
    //     except:
    //         pass
}

bool CheckValidater() {
    //use receive to get the incoming packet
    // then check to see if the string they send back matches the validater
    // and the steam is 'S' for system
    // retrun the "and" of the two
}


// generator polynomial from https://users.ece.cmu.edu/~koopman/crc/
//
uint16_t ComputeChecksumCRC(std::span<uint8_t> Payload) {
    return 0;
}
