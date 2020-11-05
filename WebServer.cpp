
// Name: WebServer.cpp
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include "WebServer.h"

WebServer::WebServer(const webServer::Config& config)
  : m_config(config)
  , m_server(config.port)
{ }

webServer::Callbacks& WebServer::callbacks()
{
  return m_callbacks;
}

void WebServer::begin()
{
  registerHandlers();
  m_server.enableCORS(true);
  m_server.begin();
}

String WebServer::getLocalIP()
{
  return (WiFi.localIP() == IPAddress() ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
}

void WebServer::process()
{
  m_server.handleClient();

  if(m_startWiFiManagerRequest)
  {
    m_startWiFiManagerRequest = false;
    m_server.sendHeader("Location", "/");
    m_server.send(302);

    m_server.stop();
        
    WiFiManager wifiManager;
    wifiManager.startConfigPortal("OpenBCI");

    begin();
  }
}

void WebServer::notifyCommandResult(const Result& result)
{
  if(result) m_server.send(200, "text/json", result.message());
  else m_server.send(400, "text/plain", result.message());
}

void WebServer::registerHandlers()
{
  m_server.on("/", HTTP_GET, [this]() { handleRouteGet(); });
  m_server.on("/", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_server.on("/tcp", HTTP_GET, [this]() { handleTcpStreamSetup(); });
  m_server.on("/tcp", HTTP_POST, [this]() { handleTcpStreamSetup(); });
  m_server.on("/tcp", HTTP_OPTIONS, [this]() { });
  m_server.on("/tcp", HTTP_DELETE, [this]() { });
  m_server.on("/udp", HTTP_GET, [this]() { });
  m_server.on("/udp", HTTP_POST, [this]() { });
  m_server.on("/udp", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_server.on("/udp", HTTP_DELETE, [this]() { });
  m_server.on("/stream/start", HTTP_GET, [this]() { });
  m_server.on("/stream/start", HTTP_POST, [this]() { });
  m_server.on("/stream/start", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_server.on("/stream/stop", HTTP_GET, [this]() { });
  m_server.on("/stream/stop", HTTP_POST, [this]() { });
  m_server.on("/stream/stop", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_server.on("/version", HTTP_GET, [this]() { handleVersionGet(); });
  m_server.on("/wifi", HTTP_GET, [this]() { handleWiFiGet(); });
  m_server.on("/version", HTTP_GET, [this]() { handleVersionGet(); });
  m_server.on("/board", HTTP_GET, [this]() { handleBoardGet(); });
  m_server.on("/board", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });
  m_server.on("/command", HTTP_POST, [this]() { handleCommandPost(); });
  m_server.on("/command", HTTP_OPTIONS, [this]() { sendHeadersForOptions(); });

  m_server.onNotFound([this]() { m_server.send(404, "text/plain", "Not Found"); });

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
  out += "<html lang=\"en\"><h1 style=\"margin:  auto\;width: 90%\;text-align: center\;\">OpenBCI Interface</h1><br>";
  out += "<p style=\"margin:  auto\;width: 80%\;text-align: center\;\"><a href='http://" + getLocalIP() + "/wifi'";
  out += F("'>Click to Configure Wifi</a>");
  out += "</html>";

  m_server.send(200, "text/html", out);
}

void WebServer::handleVersionGet()
{
  m_server.send(200, "text/html", m_config.appVersion);
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
    m_server.send(200, "text/json", info);
  }
  else m_server.send(404, F("test/plain"), F("Board info not available"));
}

void WebServer::handleTcpStreamSetup()
{
  DynamicJsonDocument jsonDoc(webServer::JSON_BUFFER);
  const DeserializationError err = deserializeJson(jsonDoc, m_server.arg(0));

  if(err) m_server.send(400, F("test/plain"), err.c_str());
  else
  {
    const Result result = m_callbacks.onTcpStreamSetupRequest(jsonDoc);
    if(result) m_server.send(200, "text/json", result.message());
    else m_server.send(400, "text/plain", result.message());
  }
}

void WebServer::handleCommandPost()
{
  DynamicJsonDocument jsonDoc(webServer::JSON_BUFFER);
  const DeserializationError err = deserializeJson(jsonDoc, m_server.arg(0));

  if(err) m_server.send(400, F("test/plain"), err.c_str());
  else
  {
    if(false == jsonDoc.containsKey("command")) m_server.send(400, F("test/plain"), "JSON doesn't contains 'command' element");
    else m_callbacks.onCommandRequest(jsonDoc["command"]);
  }
}

void WebServer::sendHeadersForOptions()
{
  m_server.sendHeader(F("Access-Control-Allow-Origin"), "*");
  m_server.sendHeader(F("Access-Control-Allow-Methods"), F("POST,DELETE,GET,OPTIONS"));
  m_server.sendHeader(F("Access-Control-Allow-Headers"), F("Content-Type"));
  m_server.send(200, "text/plain", "");
}
