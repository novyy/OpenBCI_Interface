// Name: Board.h
// Date: 04-Nov-2020
// Purpose: Streams Cyton data to tcp server
// Author: Piotr Nowinski

#include "Result.h"
#include "Endpoint.h"

#include <Arduino.h>
#include <WiFiClient.h>
#include <vector>

class TcpStreamer
{
public:
  ~TcpStreamer();
  Result connect(const Endpoint& endpoint);
  void disconnect();
  bool connected();
  const Endpoint& getRemoteEndpoint() const;

  template<class Container>
  typename std::enable_if<std::is_same<typename Container::value_type, uint8>::value, Result>::type send(const Container& buffer);

  Result send(const uint8_t* pData, size_t size);
  
private:
  std::unique_ptr<WiFiClient> m_pClient;
  Endpoint m_remoteEndpoint;
};

template<class Container>
typename std::enable_if<std::is_same<typename Container::value_type, uint8>::value, Result>::type TcpStreamer::send(const Container& buffer)
{
    return send(buffer.data(), buffer.size());
}
