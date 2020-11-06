
// Name: StatusSender.h
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include <WebSocketsServer.h>

namespace statusSender
{
    struct Callbacks
    {
        std::function<String()> onWelcomeMessageRequest;
    };
}
class StatusSender
{
public:
  void begin();
  void send(String message);
  void stop();
  void loop();
  statusSender::Callbacks& getCallbacks();

private:
  std::unique_ptr<WebSocketsServer> m_pWebSocket;
  uint32_t m_numberOfClients = 0;
  statusSender::Callbacks m_callbacks;
};