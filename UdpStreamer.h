// Name: UdpStreamer.h
// Date: 04-Nov-2020
// Purpose: Streams Cyton data to tcp server
// Author: Piotr Nowinski

#include "Result.h"
#include "Endpoint.h"
#include <Arduino.h>
#include <vector>

#include <WiFiUdp.h>

class UdpStreamer
{
public:
    const Endpoint& getRemoteEndpoint() const;

    template<class Container>
    typename std::enable_if<std::is_same<typename Container::value_type, uint8>::value, Result>::type send(const Container& buffer);

    void configure(const Endpoint& remoteEndpoint);
    Result send(const uint8_t* pData, size_t size);
  
private:
    WiFiUDP m_udpSocket;
    Endpoint m_remoteEndpoint;
};

template<class Container>
typename std::enable_if<std::is_same<typename Container::value_type, uint8>::value, Result>::type UdpStreamer::send(const Container& buffer)
{
    return send(buffer.data(), buffer.size());
}
