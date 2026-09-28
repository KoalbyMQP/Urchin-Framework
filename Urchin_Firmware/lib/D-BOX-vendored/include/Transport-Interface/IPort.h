//
// Created by gabri on 6/18/2026.
//

#ifndef D_BOX_SYS_SERAL_H
#define D_BOX_SYS_SERAL_H



#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <RingBuffer/RingBuffer.h>
#include <RingBuffer/RingBuffer.tpp>
#include <RingBuffer/RingBufferException.hpp>
#include <typeindex>
#include <typeinfo>
#include <Discovery/IDiscovery.h>
#include <Dconfig/Dconfig.h>
namespace DBOXProtocol {

    // Forward declarations
    //---------------------------------------------------------
    class DBOXprotocol;
    class IPort;
    template<typename IportType, typename DConfigType>
    class Discovery;
    //---------------------------------------------------------

    /**
     * @class   IPort
     * @brief   Iport acts as a class interface for transfer mediums.
     *
     * @details Iport acts as a class interface for transfer mediums such as (UART, SPI, WIFI, Ethernet, Bluetooth, ESP-NOW, CAN, I2C, ect).
     * @details Iport Implements higher level functions for all derived classes while leaving system dependent functions to the derived classes
     *
     * @note    Ring buffer is statically sized at compile time
     *
     * @author  Gabriel Weaver
     * @date    2026-07-26
     */
    class IPort{
    public:
        /**
         * Deconstructor
         */
        virtual ~IPort() = default;

        /**
         * @brief Opens a connection
         * This function usually gains settings from the DConfigType then call a "RawOpen" function as defined in a subclass
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        virtual bool open() = 0;

        /**
         * @brief Opens a connection
         * This function usually gains settings from the DConfigType then call a "RawClose" function as defined in a subclass
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        virtual void close() = 0;

        /**
         * @brief Check if a connection is open
         * @retval 0 on a failure
         * @retval 1 on a succuss
         */
        virtual bool isOpen() const = 0;

        /**
         * @brief Writes to the transfer medium
         * @param data The data to be sent
         * @param size How many bytes of data to be sent
         *
         * @return How many bytes were sent
         */
        std::size_t write(std::span<const uint8_t> data, std::size_t size);


        /**
         * @brief Reads from the transfer medium
         * @param buffer A buffer to save the data to
         * @param size How many bytes to read
         * @warning This function will read bytes while removing them.
         * @return How many bytes were read
         */
        std::size_t read(std::span<uint8_t> buffer, std::size_t size);

        /**
         * @brief Peeks the transfer medium
         *
         * @param buffer A buffer to save the data to
         * @param size How many bytes to peek
         *
         * @warning This function will read bytes without removing them
         * @warning when peeking large amounts of data the ring buffer can run out of space and lose old data.
         * See "ringBuffer" in private area of class for max size.
         * @return How many bytes were peeked
         */
        std::size_t peek(std::span<uint8_t> buffer, std::size_t size);

        /**
         * @brief Discards bytes from the transfer medium
         * @param size How many bytes to drop
         * @return How many bytes were dropped
         */
        std::size_t drop(std::size_t size);

        /**
         * @brief Dumps all bytes from the transfer medium
         */
        void flush();

        /**
         * @brief Dumps all bytes from the Ringbuffer
         */
        void flushPeek();

        /**
         * @brief Dumps all bytes from the raw transfer medium
         */
        virtual void flushBuss() = 0;


        /**
         * @brief lists all possible connections on the transfer medium
         * @return A list of possible connections on the transfer medium
         */
        virtual std::vector<std::string> find() = 0;

        //removed
        //virtual int start() = 0;
        //virtual int end() = 0;

        /**
         * Pointer to the controling comuncation protocol
         */
        DBOXprotocol *protocol;

        /**
         * An interface to system dependent functions for sleep
         * @param time_ms
         */
        virtual void wait(uint32_t time_ms) = 0;

    private:
        /**
         * The ring buffer use for peek and drop
         */
        RingBuffer::RingBuffer<uint8_t,1024> ringBuffer;



        /**
         * Pointer to the Discovery protocol
         */
        IDiscovery* Disco;



    protected:
        /**
         * @brief Writes to the raw transfer medium
         * @param data The data to be sent
         * @param size How many bytes of data to be sent
         *
         * @return How many bytes were sent
         */
        virtual std::size_t rawwrite(std::span<const uint8_t> data, std::size_t size) = 0;


        /**
         * @brief Reads from the raw transfer medium
         * @param buffer A buffer to save the data to
         * @param size How many bytes to read
         * @warning This function will read bytes while removing them.
         * @return How many bytes were read
         */
        virtual std::size_t rawread(std::span<uint8_t> buffer, std::size_t size) = 0;


    };
}

#endif //D_BOX_SYS_SERAL_H