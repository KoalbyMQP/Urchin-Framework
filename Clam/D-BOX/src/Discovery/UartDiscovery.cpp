//
// Created by gabri on 6/26/26.
//

#include <Discovery/UartDiscovery.h>
#include <Transport-Interface/UART/UARTSeralPort.h>
namespace DBOXProtocol {
    void UartDiscovery::init() {

    }

    // Active side (PC/Pi): try every serial port; on each one say our
    // identity and wait for the same identity back. Keep the first port that
    // answers. Returns 1 if found (port left open on it), 0 if not.
    int UartDiscovery::ActiveDiscover() {
        //gather list of ports
        std::vector<std::string> COMS = this->port->find();

        for (const std::string& COM : COMS) {
            if (ActiveDiscoverSingle(COM, attemptsPerPort)) {
                // Remember the port that answered and reopen it for normal use.
                this->config->Port = COM;
                return this->port->openraw(COM.c_str(), this->config->baud, this->config->timeout) ? 1 : 0;
            }
        }
        return 0;
    }

    bool UartDiscovery::ActiveDiscoverSingle(std::string COM, int attempts) {
        // open the port we were asked to try (not the default one)
        if (!this->port->openraw(COM.c_str(), this->config->baud, this->config->timeout))
            return false;
        this->port->flush();

        for (int i = 0; i < attempts; ++i) {
            SendValidate(this->port);
            if (ReceiveValidate(this->port)) {
                this->port->close();
                return true;
            }
        }

        this->port->close();
        return false;
    }

    // Send our identity: VPID 0, stream 'S', payload = identity.
    void UartDiscovery::SendValidate(IPort *port) {
        DBOX packet = {0, 'S', this->config->identity};
        this->port->protocol->send(&packet);
    }

    // Wait (up to handshakeTimeoutMs) for VPID 0, stream 'S', payload == identity.
    bool UartDiscovery::ReceiveValidate(IPort *port) {
        DBOX packet;
        if (!this->port->protocol->receive(&packet, handshakeTimeoutMs))
            return false;
        return packet.VPID == 0 &&
               packet.Stream == 'S' &&
               std::string_view(reinterpret_cast<const char *>(packet.Payload.data()),
                                packet.Payload.size()) == this->config->identity;
    }

    // Passive side (ESP32 or a device waiting to be found): open the
    // configured port, wait for our identity, then answer with it.
    int UartDiscovery::PassiveDiscover() {
        if (!this->port->openraw(
                this->config->Port.c_str(),
                this->config->baud,
                this->config->timeout))
            return 0;

        // start listening for the name (no timeout: waits until found)
        DBOX packet;
        while (true) {
            if (!this->port->protocol->receive(&packet))
                continue;
            if (packet.VPID == 0 &&
                packet.Stream == 'S' &&
                std::string_view(reinterpret_cast<const char *>(packet.Payload.data()),
                                 packet.Payload.size()) == this->config->identity)
                break;
        }

        SendValidate(this->port);   // answer so the active side knows it found us
        return 1;
    }
    int shutdown(){return 0;}
}