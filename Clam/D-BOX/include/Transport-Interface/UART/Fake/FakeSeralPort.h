//
// Created by gabri on 6/21/2026.
//

#ifndef D_BOX_FAKESERIALPORT_H
#define D_BOX_FAKESERIALPORT_H

#include <Transport-Interface/UART/UARTSeralPort.h>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>
#include <Transport-Interface/UART/Fake/FakeSeralHost.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <condition_variable>
#include <thread>
#include <chrono>





namespace DBOXProtocol {
    class UartDiscovery;
    class FakeSerialPort final : public UARTSerialPort<UartDiscovery>{
    public:


        explicit FakeSerialPort(std::shared_ptr<FakeSerialLink> link = {},
                                FakeSeralHost::Side side = FakeSeralHost::Side::A)
            : m_link(std::move(link)), m_side(side), UARTSerialPort(), host(nullptr)
        {}

        explicit FakeSerialPort(UartDiscovery *Discovery ,std::shared_ptr<FakeSerialLink> link = {},
                                FakeSeralHost::Side side = FakeSeralHost::Side::A)
            : m_link(std::move(link)), m_side(side), UARTSerialPort(Discovery),host(nullptr)
        {}


        explicit FakeSerialPort(FakeSeralHost* host)
            : host(host), UARTSerialPort()
        {}

        explicit FakeSerialPort(UartDiscovery *Discovery ,FakeSeralHost* host)
            : host(host), UARTSerialPort(Discovery)
        {}

        // ---------------- OPEN ----------------


        bool open() override {
            const char* device = this->Discovery->config->Port.c_str();
            uint32_t baudRate = this->Discovery->config->baud;
            uint32_t timeoutMs = this->Discovery->config->timeout;
            return openraw(device,baudRate,timeoutMs);
        }

        bool openraw(const char* device, uint32_t baudRate, uint32_t timeoutMs)
        {
            if (host == nullptr) {

                std::scoped_lock lock(m_mutex);

            }else {
                //checks to see if "device" is not empty
                if (!device)
                    return false;
                //find link
                std::pair<std::shared_ptr<FakeSerialLink>,FakeSeralHost::Side> pair = host->GetLink(device);
                if (pair.first == nullptr) {return false;}


                std::scoped_lock lock(m_mutex);
                pair = host->open(device,baudRate);
                m_link = pair.first;
                const std::string dev = device;
            }



            //m_device = dev;
            //m_baudRate = baudRate;
            m_readTimeout = std::chrono::milliseconds(timeoutMs);

            m_isOpen.store(true);
            return true;
        }

        // ---------------- CLOSE ----------------

        void close() override
        {
            m_isOpen.store(false);

            {
                std::scoped_lock lock(m_mutex);
                m_tx.clear();
            }

            m_txCv.notify_all();

        }

        bool isOpen() const override
        {
            return m_isOpen.load();
        }

        // ---------------- TX ----------------

        std::size_t rawwrite(std::span<const uint8_t> data,
                             std::size_t size) override
        {
            if (!m_isOpen.load() || !m_link || data.empty())
                return 0;

            size = std::min(size, data.size());
            auto view = data.subspan(0, size);

            std::size_t written =
                (m_side == FakeSeralHost::Side::A)
                    ? m_link->writeA(view)
                    : m_link->writeB(view);

            if (written > 0)
            {
                {
                    std::scoped_lock lock(m_mutex);
                    m_tx.insert(m_tx.end(), view.begin(), view.begin() + written);
                }

                m_txCv.notify_all();
            }

            return written;
        }

        // ---------------- RX (timeout owned here) ----------------

        std::size_t rawread(std::span<uint8_t> buffer,
                            std::size_t size) override
        {
            if (!m_isOpen.load() || !m_link || buffer.empty() || size == 0)
                return 0;

            size = std::min(size, buffer.size());
            auto start = std::chrono::steady_clock::now();

            while (true)
            {
                std::size_t got =
                    (m_side == FakeSeralHost::Side::A)
                        ? m_link->readA(buffer, size)
                        : m_link->readB(buffer, size);

                if (got > 0)
                    return got;

                if (!m_isOpen.load())
                    return 0;

                if (std::chrono::steady_clock::now() - start >= m_readTimeout)
                    return 0;

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        // ---------------- helpers ----------------

        std::vector<uint8_t> takeWritten()
        {
            std::scoped_lock lock(m_mutex);
            return std::exchange(m_tx, {});
        }

    bool waitForWrite(std::size_t minBytes, std::chrono::milliseconds timeout){
        std::unique_lock lock(m_mutex);

        bool awakened = m_txCv.wait_for(lock, timeout, [&]
        {
            return !m_isOpen.load() || m_tx.size() >= minBytes;
        });

        return awakened && m_tx.size() >= minBytes;
    }


        std::vector<std::string> find()
        {
            if (!m_link)
                return {};

            const std::string expected =
                (m_side == FakeSeralHost::Side::A)
                    ? m_link->getAName()
                    : m_link->getBName();

            return {expected};
        }

        void flushBuss() override
        {
            if (!m_link)
                return;

            {
                std::scoped_lock lock(m_mutex);
                m_tx.clear();
            }

            if (m_side == FakeSeralHost::Side::A)
                m_link->clearA();
            else
                m_link->clearB();
        }
        void injectFromPeer(std::span<const uint8_t> data)
        {
            if (!m_link || data.empty())
                return;

            // Inject into the opposite side's incoming queue
            if (m_side == FakeSeralHost::Side::A)
            {
                // A receives what B "injects"
                m_link->writeB(data);
            }
            else
            {
                // B receives what A "injects"
                m_link->writeA(data);
            }
        }

        void wait(uint32_t time_ms)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(time_ms));
        }

    private:
        FakeSeralHost* host;

        mutable std::mutex m_mutex;
        std::condition_variable m_txCv;

        std::shared_ptr<FakeSerialLink> m_link;
        FakeSeralHost::Side m_side{FakeSeralHost::Side::A};

        std::atomic_bool m_isOpen{false};
        std::chrono::milliseconds m_readTimeout{100};

        std::vector<uint8_t> m_tx;
    };
}
#endif //D_BOX_FAKESERIALPORT_H