// Name: UdpStreamer.cpp
// Date: 04-Nov-2020
// Purpose: Streams Cyton data via UDP socket
// Author: Piotr Nowinski

#include "UdpStreamer.h"

void UdpStreamer::configure(const Endpoint& remoteEndpoint)
{
    Serial.println(remoteEndpoint.ipAddress.toString() + ":" + remoteEndpoint.port);
    m_remoteEndpoint = remoteEndpoint;
}

Result UdpStreamer::send(const uint8_t* pData, size_t size)
{
    if((m_remoteEndpoint.ipAddress == IPAddress()) || m_remoteEndpoint.port == 0) return Result(false, "UDP remote endpoint not configured");

    int result = m_udpSocket.beginPacket(m_remoteEndpoint.ipAddress, m_remoteEndpoint.port);
    if(result != 1) return Result(false, "Failed to begin UDP packet");
    const int sentBytes = m_udpSocket.write(pData, size);
    result = m_udpSocket.endPacket();
    
    if(sentBytes != size) return Result(false, String("UDP socket doesn't sent enought bytes. ") + String(sentBytes) + " out of " + String(size));
    if(result != 1) return Result(false, "Failed to send UDP packet");
    else return Result();
}

const Endpoint& UdpStreamer::getRemoteEndpoint() const
{
  return m_remoteEndpoint;
}
