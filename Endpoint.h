// Name: UdpStreamer.h
// Date: 04-Nov-2020
// Purpose: Streams Cyton data to tcp server
// Author: Piotr Nowinski

#pragma once

#include <Arduino.h>
#include <IPAddress.h>

struct Endpoint
{
    IPAddress ipAddress;
    uint16_t port;
};
