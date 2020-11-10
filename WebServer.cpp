
// Name: WebServer.cpp
// Date: 04-Nov-2020
// Purpose: Handle REST requests
// Author: Piotr Nowinski

#include "WebServer.h"

WebServer::WebServer(const webServer::Config& config)
  : m_config(config)
  , m_pServer(new ESP8266WebServer(config.port))
{ }

webServer::Callbacks& WebServer::callbacks()
{
  return m_callbacks;
}

void WebServer::begin()
{
  registerHandlers();
  m_pServer->enableCORS(true);
  m_pServer->begin();
}

String WebServer::getLocalIP()
{
  return (WiFi.localIP() == IPAddress() ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
}

void WebServer::loop()
{
  m_pServer->handleClient();

  if(m_startWiFiManagerRequest)
  {
    m_startWiFiManagerRequest = false;
    m_pServer->sendHeader("Location", "/");
    m_pServer->send(302);

    m_pServer->stop();
        
    WiFiManager wifiManager;
    wifiManager.startConfigPortal(m_config.appName.c_str());

    begin();
  }
}

void WebServer::notifyCommandResult(const Result& result)
{
  if(result) m_pServer->send(200, "text/json", result.message());
  else m_pServer->send(400, "text/json", errorToJson(result.message()));
}

void WebServer::registerHandlers()
{
  m_pServer->on("/", HTTP_GET, [this]() { handleRouteGet(); });
  m_pServer->on("/", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_pServer->on("/tcp", HTTP_GET, [this]() { handleTcpStreamInfo(); });
  m_pServer->on("/tcp", HTTP_POST, [this]() { handleTcpStreamSetup(); });
  m_pServer->on("/tcp", HTTP_OPTIONS, [this]() { });
  m_pServer->on("/tcp", HTTP_DELETE, [this]() { });
  m_pServer->on("/udp", HTTP_GET, [this]() { });
  m_pServer->on("/udp", HTTP_POST, [this]() { });
  m_pServer->on("/udp", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_pServer->on("/udp", HTTP_DELETE, [this]() { });
  m_pServer->on("/stream/start", HTTP_GET, [this]() { handleStreamStart(); });
  m_pServer->on("/stream/start", HTTP_POST, [this]() { handleStreamStart(); });
  m_pServer->on("/stream/start", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_pServer->on("/stream/stop", HTTP_GET, [this]() { handleStreamStop(); });
  m_pServer->on("/stream/stop", HTTP_POST, [this]() { handleStreamStop(); });
  m_pServer->on("/stream/stop", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_pServer->on("/version", HTTP_GET, [this]() { handleVersionGet(); });
  m_pServer->on("/wifi", HTTP_GET, [this]() { handleWiFiGet(); });
  m_pServer->on("/version", HTTP_GET, [this]() { handleVersionGet(); });
  m_pServer->on("/board", HTTP_GET, [this]() { handleBoardGet(); });
  m_pServer->on("/board", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_pServer->on("/command", HTTP_POST, [this]() { handleCommandPost(); });
  m_pServer->on("/command", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });

  m_pServer->onNotFound([this]() { m_pServer->send(404, "text/plain", "Not Found"); });

/* Not supported yet
  m_server.addHook([this](const String& method, const String&, WiFiClient*, ESP8266WebServer::ContentTypeFunction)
  {
    if(method == "OPTIONS")
    {
      sendHeadersForOptions();
      return CLIENT_REQUEST_IS_HANDLED;
    }
    return CLIENT_REQUEST_CAN_CONTINUE;
  });
  */
};

void WebServer::handleRouteGet()
{
  String out = F("<!DOCTYPE html><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">");
  out += "<html lang=\"en\"><h1 style=\"margin: auto;width: 90%;text-align: center;\">OpenBCI Interface</h1><br>";
  out += "<p style=\"margin:  auto;width: 80%;text-align: center;\"><a href='http://" + getLocalIP() + "/wifi'";
  out += F("'>Click to Configure Wifi</a>");
  out += "</html>";

  m_pServer->send(200, "text/html", out);
}

void WebServer::handleVersionGet()
{
  m_pServer->send(200, "text/html", m_config.appVersion);
}

void WebServer::handleWiFiGet()
{
  m_startWiFiManagerRequest = true;
}

void WebServer::handleBoardGet()
{
  if(m_callbacks.onBoardInfoRequest != nullptr)
  {
    const String info = m_callbacks.onBoardInfoRequest();
    m_pServer->send(200, "text/json", info);
  }
  else m_pServer->send(404, F("text/json"), errorToJson(F("Board info not available")));
}

void WebServer::handleTcpStreamSetup() 
{
    if(m_callbacks.onTcpStreamSetupRequest == nullptr) return m_pServer->send(404, F("text/json"), errorToJson(F("TCP stream not available")));
    
    DynamicJsonDocument jsonDoc(webServer::JSON_BUFFER);
    const DeserializationError err = deserializeJson(jsonDoc, m_pServer->arg(0));

    if(err) return m_pServer->send(400, F("text/json"), errorToJson(err.c_str()));
     
    if(false == jsonDoc.containsKey("ip")) return m_pServer->send(400, F("text/json"), errorToJson("Cannot find 'ip' element"));
    if(false == jsonDoc.containsKey("port")) return m_pServer->send(400, F("text/json"), errorToJson("Cannot find 'port' element"));
    
    IPAddress ip;
    if(false == ip.fromString(jsonDoc["ip"].as<String>())) return m_pServer->send(400, F("text/json"), errorToJson("Invalid format of IP address"));

    const uint32_t latency = (jsonDoc.containsKey("latency") ? jsonDoc["latency"] : 0);
    
    const Result result = m_callbacks.onTcpStreamSetupRequest(ip, jsonDoc["port"], latency);
    if(result) m_pServer->send(200, "text/json", result.message());
    else m_pServer->send(400, "text/json", errorToJson(result.message()));
  
}

void WebServer::handleTcpStreamInfo()
{
  if(m_callbacks.onTcpStreamInfoRequest == nullptr) m_pServer->send(404, "text/json", errorToJson(F("TCP stream info not available")));
  else m_pServer->send(200, "text/json", m_callbacks.onTcpStreamInfoRequest());
}

void WebServer::handleTcpStreamDelete()
{
  if(m_callbacks.onTcpStreamDeleteRequest == nullptr) m_pServer->send(404, "text/json", errorToJson(F("TCP stream delete not available")));
  else m_pServer->send(200, "text/json", m_callbacks.onTcpStreamDeleteRequest());
}

void WebServer::handleStreamStop()
{
    if(m_callbacks.onStreamStopRequest == nullptr) return m_pServer->send(404, "text/json", errorToJson(F("TCP stream stop not available")));
    const Result result = m_callbacks.onStreamStopRequest();
    if(result) m_pServer->send(200, "text/json", result.message());
    else m_pServer->send(400, "text/json", errorToJson(result.message()));
}

void WebServer::handleStreamStart()
{
    if(m_callbacks.onStreamStartRequest == nullptr) return m_pServer->send(404, "text/json", errorToJson(F("TCP stream start not available")));
    const Result result = m_callbacks.onStreamStartRequest();
    if(result) m_pServer->send(200, "text/json", result.message());
    else m_pServer->send(400, "text/json", errorToJson(result.message()));
}

void WebServer::handleCommandPost()
{
  if(m_callbacks.onCommandRequest == nullptr)
  {
    m_pServer->send(404, F("text/json"), errorToJson(F("Command procedure not available")));
    return;
  }

  DynamicJsonDocument jsonDoc(webServer::JSON_BUFFER);
  const DeserializationError err = deserializeJson(jsonDoc, m_pServer->arg(0));

  if(err) m_pServer->send(400, F("text/plain"), errorToJson(err.c_str()));
  else
  {
    if(false == jsonDoc.containsKey("command")) m_pServer->send(400, F("text/json"), errorToJson("JSON doesn't contains 'command' element"));
    else m_callbacks.onCommandRequest(jsonDoc["command"]);
  }
}

void WebServer::sendHeadersForOptions()
{
  m_pServer->sendHeader(F("Access-Control-Allow-Origin"), "*");
  m_pServer->sendHeader(F("Access-Control-Allow-Methods"), F("POST,DELETE,GET,OPTIONS"));
  m_pServer->sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type"));
  m_pServer->send(200, "text/plain", "");
}
