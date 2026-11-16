//
// Created by gabri on 6/26/26.
//

#ifndef D_BOX_DISCOVERY_H
#define D_BOX_DISCOVERY_H
#include <concepts>
#include <Dconfig/Dconfig.h>
#include <Transport-Interface/IPort.h>
#include <Discovery/IDiscovery.h>
namespace DBOXProtocol {

    // Forward declarations
    //----------------------
    class DBOXprotocol;
    //----------------------


    /**
     * @brief Concept restricting types to classes derived from IPort.
     *
     * A type satisfies this concept if it inherits from the IPort interface.
     *
     * @tparam T The type to check.
     */
    template<typename T>
    concept IPortType = std::derived_from<T, IPort>;

    /**
     * @brief Concept restricting types to classes derived from Dconfig.
     *
     * A type satisfies this concept if it inherits from the Dconfig interface.
     *
     * @tparam T The type to check.
     */
    template<typename T>
    concept DConfigType = std::derived_from<T, Dconfig>;

    /**
     * @class   Discovery
     * @brief   Discovery defines the methods and tools to locate the protocol's pair
     * @tparam IportType A class that inherits from the IPort interface
     * @tparam DConfigType A class that inherits from the Dconfig interface
     *
     * @author  Gabriel Weaver
     * @date    2026-07-26
     */
    template<typename IportType,typename DConfigType>
    class Discovery : public IDiscovery{
        public:
            /**
            * Constructor
            * @param config The config
            */
            explicit Discovery<IportType,DConfigType>(DConfigType* config)
            : config(config){}


            /**
             * Adds the IPort to the Discovery instance
             * @param port_in pointer to the IPort
             */
            void BindPort(IportType* port_in){this->port=port_in;}


            /**
             * A class that inherits from the Dconfig interface
             * This holds config info the discovery class
             */
            DConfigType* config;

            /**
             * A class that inherits from the IPort interface
             */
            IportType* port;


    };

}
#endif //D_BOX_DISCOVERY_H