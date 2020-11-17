// Name: OpenBCI_Interface.ino
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski
#pragma once

#include <Arduino.h>

namespace cytonMessage
{
    static const uint8 BEGIN = 0xA0;
    static const uint8 END = 0xC0;
    static const uint32 SIZE = 33;
}