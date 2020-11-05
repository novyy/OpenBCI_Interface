
// Name: OpenBCI_Interface.ino
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#define ARDUINO_ARCH_ESP8266
#define ESP8266

#include "WebServer.h"
#include "Board.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#define VERSION "0.0.1"


// TcpStreamer ==================================
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
  Result connect(const tcpStreamer::Config& config);
  
private:
  WiFiClient m_client;    
};

Result TcpStreamer::connect(const tcpStreamer::Config& config)
{
  if(m_client.connect(config.ipAddress, config.port))
  {
    m_client.setNoDelay(1);
    return Result(true, "");
  }
  else return Result(false, "Failed to connect to server");
}

//===============================================

WebServer _webServer(webServer::Config{VERSION, 80});
Board _board;
TcpStreamer _tcpStreamer;

void setup()
{
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  _webServer.callbacks().onBoardInfoRequest = []() {  return _board.getInfo(); };
  _webServer.callbacks().onUdpStreamSetupRequest = []() {  };
  _webServer.callbacks().onCommandRequest = [](const String& command){ _webServer.notifyCommandResult(Result(true, "")); };

  _webServer.callbacks().onTcpStreamSetupRequest = [](DynamicJsonDocument& jsonDoc) 
  {
    if(false == jsonDoc.containsKey("ip")) return Result(false, "Cannot be find 'ip' element");
    if(false == jsonDoc.containsKey("port")) return Result(false, "Cannot be find 'port' element");
    IPAddress address;
    address.fromString(jsonDoc["ip"].as<String>());
    const Result result = _tcpStreamer.connect(tcpStreamer::Config{address, jsonDoc["port"]});
    if(result == false) return result;
    else return Result(true, _board.getTcpStreamInfo());
  };
  
  _webServer.begin();
}

void loop()
{
  _webServer.process();
}
