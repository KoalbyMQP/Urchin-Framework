#include <cassert>
#include <array>
#include <iostream>
//#include  "linux-sys-seral.h"

#include "tests/Transport-Interface/Fake/TestFakeSeral.h"
#include "tests/RingBuffer/TestRingBuffer.h"
#include "tests/TestISerialPort.h"
int main()
{

    TestFakeSeral();
    TestRingBuffer();
    TestISerialPort();
    return 0;


}