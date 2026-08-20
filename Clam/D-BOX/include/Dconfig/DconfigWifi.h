//
// Created by gabri on 6/26/26.
//

#ifndef D_BOX_DCONFIGWIFI_H
#define D_BOX_DCONFIGWIFI_H
#include <Dconfig/Dconfig.h>
namespace DBOXProtocol {
    class DconfigUart : public Dconfig {
    public
        std::string Password;
    };
}
#endif //D_BOX_DCONFIGWIFI_H