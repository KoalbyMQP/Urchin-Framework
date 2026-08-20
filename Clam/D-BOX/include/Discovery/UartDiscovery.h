//
// Created by gabri on 6/26/26.
//

#ifndef D_BOX_UARTDISCOVERY_H
#define D_BOX_UARTDISCOVERY_H
#include <Dconfig/DconfigUart.h>
#include <Discovery/Discovery.h>
//#include <Transport-Interface/UART/UARTSeralPort.h>
#include <DBOXprotocol.h>
#include <span>

#include <vector>
namespace DBOXProtocol {

    // Forward declarations
    //----------------------
    template<typename DiscoveryType>
    class UARTSerialPort;
    //----------------------

    /**
     * @brief UART-based device discovery implementation.
     *
     * This class provides discovery functionality using the UARTSerialPort class or derivatives
     *
     * The class inherits from Discovery with:
     * - UARTSerialPort<UartDiscovery> as the port type
     * - DconfigUart as the configuration type
     *
     * @note The UART serial port implementation must satisfy the IPortType concept.
     * @note The configuration type must satisfy the DConfigType concept.
     */
    class UartDiscovery : public Discovery<UARTSerialPort<UartDiscovery>,DconfigUart>{
    public:
        /**
         * Constructor
         * @param active If true, discovery actively searches for its protocol pair.
         *               If false, discovery waits to be found.
         * @param config The config
         */
        explicit UartDiscovery(bool active, DconfigUart* config)
           : Discovery<UARTSerialPort<UartDiscovery>, DconfigUart>(config) {}

        /**
         * Initializer
         */
        void init() final;

        /**
         * Uses the transfer medium to find the protocol's pair
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        int discover() override {
            if (active) {
                return ActiveDiscover();
            }else {
                return PassiveDiscover();
            }
        }

        /**
         * Actively search for the protocol pair on all available UART ports.
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        int ActiveDiscover();

        /**
         * Sets the peer to start lissening for the actitive peer.
         * @
         * @retval 0 on a failure
         * @retval 1 on a succuss
         * @note This is a blocking call and will not return immediately.
         */
        int PassiveDiscover();

        /**
         * Attempts to probe a COM port a number a times to see if it's a peer.
         * @param COM The device port to connect to
         * @param attempts The number of attempts to try
         * @retval True on a succuss
         * @retval False on a failure
         */
        bool ActiveDiscoverSingle(std::string COM, int attempts);

        /**
         *
         * @param port
         * @return
         */
        bool ReceiveValidate(IPort *port);

        /**
         *
         * @param port
         */
        void SendValidate(IPort *port);

        /**
         * No cleanup required.
         * @return 0
         */
        int stop() override {return 0;}



    private:
        bool active;





    };
}
#endif //D_BOX_UARTDISCOVERY_H