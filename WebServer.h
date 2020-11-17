
// Name: WebServer.h
// Date: 04-Nov-2020
// Purpose: Handle REST requests
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
        String appName;
        uint16_t port;
    };
    
    struct Callbacks
    {
        std::function<String()> onBoardInfoRequest;
        std::function<String()> onAllInfoRequest;
        std::function<Result(const IPAddress& ip, uint16_t port, uint32_t latency)> onTcpStreamSetupRequest;
        std::function<String()> onTcpStreamInfoRequest;
        std::function<String()> onTcpStreamDeleteRequest;
        std::function<Result(const IPAddress& ip, uint16_t port, uint32_t latency)> onUdpStreamSetupRequest;
        std::function<void()> onUdpStreamInfoRequest;
        std::function<void(const String&)> onCommandRequest;
        std::function<Result()> onStreamStartRequest;
        std::function<Result()> onStreamStopRequest;
    };
  
  static const uint32_t JSON_BUFFER = 256;
}

class WebServer
{
public:
  WebServer(const webServer::Config& config);
  void begin();
  void loop();
  webServer::Callbacks& callbacks();
  void notifyCommandResult(const Result& result);
  String getLocalIP();

private:
  void registerHandlers();
  void sendHeadersForOptions();
  void handleRouteGet();
  void handleVersionGet();
  void handleWiFiGet();
  void handleBoardGet();
  void handleTcpStreamSetup();
  void handleUdpStreamSetup();
  void handleTcpStreamDelete();
  void handleTcpStreamInfo();
  void handleCommandPost();
  void handleStreamStart();
  void handleStreamStop();
  void handleGetAllStatus();
  Result parseStreamParams(const String& input, IPAddress& ip, uint16& port, uint32& latency);
  
private:
  std::unique_ptr<ESP8266WebServer> m_pServer;
  bool m_startWiFiManagerRequest = false;
  webServer::Callbacks m_callbacks;
  const webServer::Config m_config;
};
