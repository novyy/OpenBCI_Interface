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
  const tcpStreamer::Config& getConfig() const;

  template<class Container>
  typename std::enable_if<std::is_same<typename Container::value_type, uint8>::value, Result>::type send(const Container& buffer);

  Result send(const uint8_t* pData, size_t size);
  
private:
  std::unique_ptr<WiFiClient> m_pClient;
  tcpStreamer::Config m_config;
};

template<class Container>
typename std::enable_if<std::is_same<typename Container::value_type, uint8>::value, Result>::type TcpStreamer::send(const Container& buffer)
{
    return send(buffer.data(), buffer.size());
}
