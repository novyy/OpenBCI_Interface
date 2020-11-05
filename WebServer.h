
// Name: WebServer.h
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include "Result.h"
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <functional>

namespace webServer
{
  struct Config
  {
    String appVersion;
    uint16_t port;
  };
    
  struct Callbacks
  {
    std::function<String()> onBoardInfoRequest;
    std::function<Result(DynamicJsonDocument&)> onTcpStreamSetupRequest;
    std::function<void()> onUdpStreamSetupRequest;
    std::function<void(const String&)> onCommandRequest;
  };
  
  static const uint32_t JSON_BUFFER = 256;
}

class WebServer
{
public:
  WebServer(const webServer::Config& config);
  void begin();
  void process();
  webServer::Callbacks& callbacks();
  void notifyCommandResult(const Result& result);

private:
  void registerHandlers();
  void sendHeadersForOptions();
  void handleRouteGet();
  void handleVersionGet();
  void handleWiFiGet();
  void handleBoardGet();
  void handleTcpStreamSetup();
  void handleCommandPost();

  String getLocalIP();
  
private:
  ESP8266WebServer m_server;
  bool m_startWiFiManagerRequest = false;
  webServer::Callbacks m_callbacks;
  const webServer::Config m_config;
};
