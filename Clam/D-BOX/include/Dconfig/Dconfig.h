//
// Created by gabri on 6/26/26.
//

#ifndef D_BOX_DCONFIG_H
#define D_BOX_DCONFIG_H
#include <concepts>
#include <type_traits>
#include <utility>
#include <string>

namespace DBOXProtocol {
    /**
     * @class   Dconfig
     * @brief   Dconfig defines the configuration values for the Discovery class
     * @author  Gabriel Weaver
     * @date    2026-07-26
     */
    class Dconfig {
    public:
        /**
         * Constructor
         * @param identity
         */
        explicit Dconfig(std::string identity): identity(identity) {}

        /**
         * Deconstructor
         */
        virtual ~Dconfig() = default;

        /**
         * The identity of the protocol instance
         */
        std::string identity;


    };
}
#endif //D_BOX_DCONFIG_H