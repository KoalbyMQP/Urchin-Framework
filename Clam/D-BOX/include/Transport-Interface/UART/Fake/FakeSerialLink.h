#ifndef D_BOX_FAKESERIALLINK_H
#define D_BOX_FAKESERIALLINK_H
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <string>
#include <algorithm>
namespace DBOXProtocol {
    class FakeSerialLink
    {
    public:
        FakeSerialLink(std::string a,
                       std::string b)
            : Aname(std::move(a)),
              Bname(std::move(b))
        {}

        // ---------------- identity / config ----------------

        const std::string& getAName() const { return Aname; }
        const std::string& getBName() const { return Bname; }
        uint32_t getABaud() const { return Abaudrate; }
        uint32_t getBBaud() const { return Bbaudrate; }

        void setABaud(uint32_t Baud) { this->Abaudrate=Baud; }
        void setBBaud(uint32_t Baud) { this->Bbaudrate=Baud; }

        std::string GetNameA(){return Aname;}
        std::string GetNameB(){return Bname;}
        // ---------------- transport A → B ----------------

        std::size_t writeA(std::span<const uint8_t> data)
        {
            if (data.empty())
                return 0;

            std::scoped_lock lock(m_mutex);

            m_aToB.insert(m_aToB.end(), data.begin(), data.end());
            return data.size();
        }

        std::size_t readB(std::span<uint8_t> buffer, std::size_t max)
        {
            if (buffer.empty() || max == 0)
                return 0;

            std::scoped_lock lock(m_mutex);

            if (m_aToB.empty())
                return 0;

            const std::size_t n = std::min({max, buffer.size(), m_aToB.size()});

            for (std::size_t i = 0; i < n; ++i)
            {
                buffer[i] = m_aToB.front();
                m_aToB.pop_front();
            }

            return n;
        }

        // ---------------- transport B → A ----------------

        std::size_t writeB(std::span<const uint8_t> data)
        {
            if (data.empty())
                return 0;

            std::scoped_lock lock(m_mutex);

            m_bToA.insert(m_bToA.end(), data.begin(), data.end());
            return data.size();
        }

        std::size_t readA(std::span<uint8_t> buffer, std::size_t max)
        {
            if (buffer.empty() || max == 0)
                return 0;

            std::scoped_lock lock(m_mutex);

            if (m_bToA.empty())
                return 0;

            const std::size_t n = std::min({max, buffer.size(), m_bToA.size()});

            for (std::size_t i = 0; i < n; ++i)
            {
                buffer[i] = m_bToA.front();
                m_bToA.pop_front();
            }

            return n;
        }

        // ---------------- clear operations ----------------

        // incoming to A (B→A)
        void clearA()
        {
            std::scoped_lock lock(m_mutex);
            m_bToA.clear();
        }

        // incoming to B (A→B)
        void clearB()
        {
            std::scoped_lock lock(m_mutex);
            m_aToB.clear();
        }

        void clearAll()
        {
            std::scoped_lock lock(m_mutex);
            m_aToB.clear();
            m_bToA.clear();
        }



    private:
        std::string Aname;
        std::string Bname;
        uint32_t Abaudrate = 0;
        uint32_t Bbaudrate = 0;

        mutable std::mutex m_mutex;
        std::deque<uint8_t> m_aToB;
        std::deque<uint8_t> m_bToA;
    };
}
#endif