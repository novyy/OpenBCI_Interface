
// Name: StatusSender.cpp
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include "StatusSender.h"

void StatusSender::begin()
{
    m_numberOfClients = 0;
    auto pWebServer = std::unique_ptr<WebSocketsServer>(new WebSocketsServer(81));
    pWebServer->begin();
    pWebServer->onEvent([this](uint8_t num, WStype_t type, uint8_t*, size_t)
    {
        if(type == WStype_CONNECTED)
        {
            m_numberOfClients++;
            String message = m_callbacks.onWelcomeMessageRequest();
            if(m_callbacks.onWelcomeMessageRequest != nullptr) m_pWebSocket->sendTXT(num, message);
        }
        else if(type == WStype_DISCONNECTED) m_numberOfClients--;
    });
    m_pWebSocket = std::move(pWebServer);
}

statusSender::Callbacks& StatusSender::getCallbacks()
{
    return m_callbacks;
}

void StatusSender::send(String message)
{
    if((m_pWebSocket != nullptr) && (m_numberOfClients > 0)) m_pWebSocket->broadcastTXT(message);
}

void StatusSender::loop()
{
  if(m_pWebSocket != nullptr) m_pWebSocket->loop();
}

void StatusSender::stop()
{
    if(m_pWebSocket != nullptr) m_pWebSocket->disconnect();
    m_pWebSocket.reset();
}