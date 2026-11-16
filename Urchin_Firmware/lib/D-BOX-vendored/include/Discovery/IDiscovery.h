//
// Created by gabri on 7/25/26.
//

#ifndef D_BOX_IDISCOVERY_H
#define D_BOX_IDISCOVERY_H
namespace DBOXProtocol {

    // Forward declarations
    //----------------------
    class DBOXprotocol;
    //----------------------

    /**
     * @class   IDiscovery
     * @brief   IDiscovery acts as a class interface for the template class Discovery.
     *
     * @author  Gabriel Weaver
     * @date    2026-07-26
     */
    class IDiscovery {
    public:
        /**
         * Constructor
         */
        explicit IDiscovery(){}

        /**
         * Initializer
         */
        virtual void init() = 0;

        /**
         * Uses the transfer medium to find the protocol's pair
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        virtual int discover() = 0;

        /**
         * Disconnects from the protocol's pair and runs any cleanup required
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        virtual int stop() = 0;

        /**
         * Deconstructor
         */
        virtual ~IDiscovery() {}


    };
}
#endif //D_BOX_IDISCOVERY_H