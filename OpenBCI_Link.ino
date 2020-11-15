
// Name: OpenBCI_Interface.ino
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#define ARDUINO_ARCH_ESP8266
#define ESP8266

#include "WebServer.h"
#include "Board.h"
#include "TcpStreamer.h"
#include "CytonMessageDefs.h"
#include "SpiMessageDefs.h"
#include "DeadlineTimer.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include "SPISlave.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>

#define VERSION "0.1.0"
#define APP_NAME "OpenBCI"

struct SystemInfo
{
    std::vector<uint8> gains;
    uint32_t latency = 0;
    uint8 channelsNumber = 0;
};

WebServer _webServer(webServer::Config{VERSION, APP_NAME, 80});
Board _board;
TcpStreamer _tcpStreamer;
SystemInfo _systemInfo;
DeadlineTimer _commandResponseTimeout;
uint32_t _lastPacketSent = 0;
std::vector<uint8> _packetBuffer;
const uint32 MAX_PACKETS_PER_SEND_TCP = 42;

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

String guessBoardName(uint8 channelsNumber)
{
    return (channelsNumber == 4) ? "ganglion" : (channelsNumber == 8) ? "cyton" : (channelsNumber == 16) ? "daisy" : "unknown";
}

String getStreamStatus()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["connected"] = _tcpStreamer.connected();
    jsonDoc["type"] = "TCP";
    jsonDoc["ip"] = _tcpStreamer.getConfig().ipAddress.toString();
    jsonDoc["port"] = _tcpStreamer.getConfig().port;
    jsonDoc["latency"] = _systemInfo.latency;
    jsonDoc["delimiter"] = true;
    jsonDoc["output"] = "raw";

    String output;
    serializeJson(jsonDoc, output);
    return output;
}

String getBoardInfo()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["board_connected"] = _board.connected();
    jsonDoc["board_type"] = guessBoardName(_systemInfo.channelsNumber);
    jsonDoc["num_channels"] = _systemInfo.channelsNumber;

    JsonArray gains = jsonDoc.createNestedArray("gains");
    for (const auto& gain : _systemInfo.gains) gains.add(gain);

    String output;
    serializeJson(jsonDoc, output);
    return output;
}

//=============================================================================
 void setup()
{
    Serial.begin(115200);
    
    connect();

    _webServer.callbacks().onBoardInfoRequest = []() {  return getBoardInfo(); };
    _webServer.callbacks().onUdpStreamSetupRequest = []() {  };
    _webServer.callbacks().onCommandRequest = [](const String& command)
    {
        Serial.println(String("Command: " + command));
        _board.command(command);
        _commandResponseTimeout.expiresIn(5000, [](){ _webServer.notifyCommandResult(Result(false, "Command timeout")); });
    };

    _webServer.callbacks().onTcpStreamSetupRequest = [](const IPAddress& ip, uint16_t port, uint32_t latency) 
    {
        const Result result = _tcpStreamer.connect(tcpStreamer::Config{ip, port});
        if(result == false) return result;
        else
        {
            if(latency > 0) _systemInfo.latency = latency;
            return Result(true, getStreamStatus());
        }
    };

    _webServer.callbacks().onTcpStreamInfoRequest = []
    {
        StaticJsonDocument<256> jsonDoc;
        jsonDoc["connected"] = _tcpStreamer.connected();
        jsonDoc["ip"] = _tcpStreamer.getConfig().ipAddress.toString();
        jsonDoc["port"] = _tcpStreamer.getConfig().port;

        String result;
        serializeJson(jsonDoc, result);
        return result;
    };

    _webServer.callbacks().onTcpStreamDeleteRequest = []
    {
        _tcpStreamer.disconnect();
        return getStreamStatus();
    };

    _webServer.callbacks().onStreamStartRequest = []
    {
        _board.command("b");
        return Result(true, "{\"result\": \"\"}");
    };

    _webServer.callbacks().onStreamStopRequest = []
    {
        _board.command("s");
        return Result(true, "{\"result\": \"\"}");
    };

    _board.callbacks().onData = [](const std::vector<uint8>& data)
    {
        std::copy(data.begin(), data.end(), std::back_inserter(_packetBuffer));
        const uint32_t currentTime = micros();
        if((currentTime > _lastPacketSent + _systemInfo.latency) || (_packetBuffer.size() >= MAX_PACKETS_PER_SEND_TCP * cytonMessage::SIZE))
        {
            _tcpStreamer.send(_packetBuffer);
            _packetBuffer.clear();
            _lastPacketSent = currentTime;
        }
    };

    _board.callbacks().onGains = [](const std::vector<uint8>& data)
    {
        _systemInfo.gains.assign(data.begin(), data.end());
        _systemInfo.channelsNumber = _systemInfo.gains.size();        
    };

    _board.callbacks().onCommandResult = [](const String& data)
    {
        _webServer.notifyCommandResult(Result(true, "{\"result\":\"" + data + "\"}"));
        _commandResponseTimeout.cancel();
    };
    
    _webServer.begin();
    _board.begin();
}

void loop()
{
    _webServer.loop();
    _board.loop();
    _commandResponseTimeout.loop();
}
