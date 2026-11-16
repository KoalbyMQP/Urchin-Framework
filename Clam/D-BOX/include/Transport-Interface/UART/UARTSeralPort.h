//
// Created by gabri on 7/21/26.
//

#ifndef D_BOX_SERALPORT_H
#define D_BOX_SERALPORT_H
#include <Transport-Interface/IPort.h>
#include <Dconfig/DconfigUart.h>
#include <Discovery/UartDiscovery.h>


namespace DBOXProtocol {
    template<typename DiscoveryType>
    class UARTSerialPort : public IPort {
    public:

        UARTSerialPort(UartDiscovery *Discovery):Discovery(Discovery) {
            this->Discovery->BindPort(this);
        }

        UARTSerialPort(): Discovery(nullptr) {
        }

        void BindProto(DBOXprotocol* protocol_in) {
            this->protocol=protocol_in;
        }

        virtual bool openraw(const char* device, uint32_t baudRate, uint32_t timeoutMs) = 0;

        UartDiscovery *Discovery;
        int start() override;
        int end() override;

    };
}

#endif //D_BOX_SERALPORT_H