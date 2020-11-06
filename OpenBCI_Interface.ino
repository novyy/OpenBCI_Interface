
// Name: OpenBCI_Interface.ino
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#define ARDUINO_ARCH_ESP8266
#define ESP8266

#include "WebServer.h"
#include "Board.h"
#include "TcpStreamer.h"
#include "StatusSender.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#define VERSION "0.0.1"
#define APP_NAME "OpenBCI"


WebServer _webServer(webServer::Config{VERSION, APP_NAME, 80});
Board _board;
TcpStreamer _tcpStreamer;
StatusSender _statusSender;

void connect()
{
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    const auto runSoftAP = []()
    {
        WiFi.softAP(APP_NAME);
        WiFi.mode(WIFI_AP);
    };

    if(WiFi.SSID() == "") runSoftAP();
    else if(false == WiFiManager().autoConnect(APP_NAME)) runSoftAP(); 
}

String getStreamStatus()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["connected"] = _tcpStreamer.connected();
    jsonDoc["type"] = "TCP";
    jsonDoc["ip"] = _tcpStreamer.getConfig().ipAddress.toString();
    jsonDoc["port"] = _tcpStreamer.getConfig().port;
    jsonDoc["latency"] = _board.getLatency();

    String output;
    serializeJson(jsonDoc, output);
    return output;
} 

String getFullStatus()
{
    return "[{\"type\":\"stream\", \"message\":" + getStreamStatus() + "}]";
}

struct
{
    bool streamConnected = false;
}
lastStatus;

String getChangedStatus()
{
    const bool connected = _tcpStreamer.connected();
    if(lastStatus.streamConnected != connected)
    {
        lastStatus.streamConnected = connected;
        return "[{\"type\":\"stream\", \"message\":" + getStreamStatus() + "}]";
    }
    return "";
}


//=============================================================================
void setup()
{
    Serial.begin(115200);
    connect();

    _webServer.callbacks().onBoardInfoRequest = []() {  return _board.getInfo(); };
    _webServer.callbacks().onUdpStreamSetupRequest = []() {  };
    _webServer.callbacks().onCommandRequest = [](const String& command){ _webServer.notifyCommandResult(Result(true, "")); };

    _webServer.callbacks().onTcpStreamSetupRequest = [](const IPAddress& ip, uint16_t port, uint32_t latency) 
    {
        const Result result = _tcpStreamer.connect(tcpStreamer::Config{ip, port});
        if(result == false) return result;
        else
        {
            if(latency > 0) _board.setLatency(latency);
            return Result(true, getStreamStatus());
        }
    };

    _webServer.callbacks().onTcpStreamInfoRequest = []()
    {
        StaticJsonDocument<256> jsonDoc;
        jsonDoc["connected"] = _tcpStreamer.connected();
        jsonDoc["ip"] = _tcpStreamer.getConfig().ipAddress.toString();
        jsonDoc["port"] = _tcpStreamer.getConfig().port;

        String result;
        serializeJson(jsonDoc, result);
        return result;
    };

    _webServer.callbacks().onTcpStreamDeleteRequest = []()
    {
        _tcpStreamer.disconnect();
        return getStreamStatus();
    };

    _statusSender.getCallbacks().onWelcomeMessageRequest = [] { return getFullStatus(); };

    _webServer.begin();
    _statusSender.begin();
}


void loop()
{
    _webServer.loop();
    _statusSender.loop();

    String status = getChangedStatus();
    if(status != "") _statusSender.send(status);
}
