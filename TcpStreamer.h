// Name: Board.h
// Date: 04-Nov-2020
// Purpose: Streams data to tcp server
// Author: Piotr Nowinski

#include "Result.h"

#include <Arduino.h>
#include <WiFiClient.h>
#include <vector>

namespace tcpStreamer
{
  struct Config
  {
    IPAddress ipAddress;
    uint16_t port;  
  };
}

class TcpStreamer
{
public:
  ~TcpStreamer();
  Result connect(const tcpStreamer::Config& config);
  void disconnect();
  bool connected();
  Result send(const std::vector<uint8>& buffer);
  const tcpStreamer::Config& getConfig() const;
  
private:
  std::unique_ptr<WiFiClient> m_pClient;
  tcpStreamer::Config m_config;
};
