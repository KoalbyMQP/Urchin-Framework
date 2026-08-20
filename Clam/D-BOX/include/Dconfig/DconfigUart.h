//
// Created by gabri on 6/26/26.
//

#ifndef D_BOX_DCONFIGUART_H
#define D_BOX_DCONFIGUART_H
#include <string>
#include <Dconfig/Dconfig.h>
namespace DBOXProtocol {
    class DconfigUart : public Dconfig {
    public:
        /**
         * Constructor
         * @param identity The identity of the protocol instance
         * @param Port Port name
         * @param baud Baudrate
         * @param timeout UART Timeout
         */
        DconfigUart(std::string identity, std::string Port,u_int32_t baud,u_int32_t timeout):
        Port(Port),baud(baud),timeout(timeout), Dconfig(identity){}

        /**
         * The serial port to connect to if the "UartDiscovery->active" == false
         */
        std::string Port;

        /**
         * The baudrate for the UART port
         */
        u_int32_t baud;

        /**
         * The Timeout value for the UART port
         */
        u_int32_t timeout;
    };
}
#endif //D_BOX_DCONFIGUART_H