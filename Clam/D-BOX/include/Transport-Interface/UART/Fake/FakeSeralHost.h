//
// Created by gabri on 7/23/26.
//

#ifndef D_BOX_FAKESERALHOST_H
#define D_BOX_FAKESERALHOST_H
#include <vector>
#include <utility>
#include <Transport-Interface/UART/Fake/FakeSerialLink.h>

namespace DBOXProtocol {


    class FakeSeralHost {
        public:
        enum class Side { A, B };

            FakeSeralHost(){}



            std::vector<std::string> list() {
                std::vector<std::string> result;
                for (auto plug : this->plugs) {
                    if (plug.second == Side::A) {
                        result.push_back(plug.first->GetNameA());
                    }
                    if (plug.second == Side::B) {
                        result.push_back(plug.first->GetNameB());
                    }
                }
                return result;
            }

        std::pair<std::shared_ptr<FakeSerialLink>,Side> open(std::string serialPortName, uint32_t baud) {
                std::pair<std::shared_ptr<FakeSerialLink>,Side> link = GetLink(serialPortName);
                if (link.first == nullptr) {return link;}
                Side side = link.second;
                if (side == Side::A) {
                    link.first->setABaud(baud);
                }
                if (side == Side::B) {
                    link.first->setBBaud(baud);
                }
                return link;

            }

        std::pair<std::shared_ptr<FakeSerialLink>,Side> GetLink(std::string serialPortName) {
                for (auto plug : this->plugs) {
                    if (plug.second == Side::A && plug.first->GetNameA()==serialPortName) {
                        return plug;
                    }
                    if (plug.second == Side::B && plug.first->GetNameB()==serialPortName) {
                        return plug;
                    }
                }
                return std::pair<std::shared_ptr<FakeSerialLink>,Side>(nullptr,Side::A);
            }

        void AddPlug(std::shared_ptr<FakeSerialLink> link, Side side)
            {
                plugs.push_back(std::make_pair(link, side));
            }
        private:
         std::vector<std::pair<std::shared_ptr<FakeSerialLink>,Side>> plugs;



    };
}


#endif //D_BOX_FAKESERALHOST_H