//
// Created by gabri on 6/26/26.
//

#include <Discovery/UartDiscovery.h>
#include <Transport-Interface/UART/UARTSeralPort.h>
namespace DBOXProtocol {
    void UartDiscovery::init() {

    }

    int UartDiscovery::ActiveDiscover() {
        //gather list of ports
        std::vector<std::string> COMS =  this->port->find();
        bool found = false;
        std::string FoundStr;
        for (int i = 0; i < COMS.size(); ++i) {
            if (!found) {
                found = ActiveDiscoverSingle(COMS[i],3);
                FoundStr = COMS[i];
            }
        }
        this->port->open();

        return found ? 1 : 0;
    }

    bool UartDiscovery::ActiveDiscoverSingle(std::string COM, int attempts) {
        // enable the Uart port
        this->port->open();

        bool found = false;
        for (int i = 0; i < attempts; ++i) {
            found = ReceiveValidate(this->port);
            if (!found) {this->port->wait(20);}
            else {
                this->port->close();
                return true;
            }
        }

        this->port->close();
        return false;



    }

    void UartDiscovery::SendValidate(IPort *port) {
        DBOX packet ={0,'S',this->config->identity.data()};
        this->port->protocol->send(&packet);
    }

    bool UartDiscovery::ReceiveValidate(IPort *port) {
        DBOX *packet;

        this->port->protocol ->receive(packet);
        return (packet->VPID==0 &
                 packet->Stream== 'S'&
                 std::string_view(reinterpret_cast<const char *>(packet->Payload.data()),
                 packet->Payload.size()) == this->config->identity);

    }


    int UartDiscovery::PassiveDiscover() {
        // enable the Uart port
        this->port->openraw(
            this->config->Port.c_str(),
            this->config->baud,
            this->config->timeout);

        // start listening for the name
        DBOX *packet;
        do {
            this->port->protocol->receive(packet);
        }while (!(packet->VPID==0 &
                  packet->Stream== 'S'&
                  std::string_view(reinterpret_cast<const char *>(packet->Payload.data()),
                      packet->Payload.size()) == this->config->identity));

        return 0;
    }
    int shutdown(){return 0;}
}