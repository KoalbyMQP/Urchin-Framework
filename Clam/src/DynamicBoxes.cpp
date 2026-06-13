//
// Created by gabri on 6/12/26.
//
#include "../MarshalType.h"
#include <vector>
#include <cstdint>

class Box : virtual public MarshalType {
public:
    unsigned int VPID;
    unsigned char stream;
    unsigned int length;
    unsigned int checksum_postheader;
    unsigned int checksum_postpayload;

    std::vector<int8_t> payload;
    std::vector<int8_t> header_payload;


    Box() {
        VPID = 0;
        stream = 0;
        length = 0;
        checksum_postheader = 0;
        checksum_postpayload = 0;
    }
};

