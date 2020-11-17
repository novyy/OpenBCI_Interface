#pragma once
#include <Arduino.h>

namespace spiMessage
{
    static const uint8 LAST = 0x01;
    static const uint8 MULTI = 0x02;
    static const uint8 GAIN = 0x03;
    static const uint32 SIZE = 32;
    static const uint32 STATUS = 209;
}