
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
uint32_t _latency;
std::vector<uint8> _buffer;
std::vector<uint8> _commandResponse;

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
    jsonDoc["latency"] = _latency;
    
    jsonDoc["delimiter"] = true;
    jsonDoc["output"] = "raw";

    String output;
    serializeJson(jsonDoc, output);
    return output;
} 

void storeCommandResponse(const std::vector<uint8>& command)
{
    for(uint32_t i = 1; i < command.size(); i++)
    {
        if(command[i] == 0x00) break;
        _commandResponse.push_back(command[i]);
    }
}

uint8 parseGain(uint8 value)
{
    static const std::vector<uint8> GAINS = {1, 2, 4, 6, 8, 12, 24};
    return (value > GAINS.size() - 1) ? GAINS[value] : GAINS[GAINS.size() - 1];
}

//=============================================================================
 void setup()
{
    Serial.begin(115200);
    
    connect();

    _webServer.callbacks().onBoardInfoRequest = []() {  return _board.getInfo(); };
    _webServer.callbacks().onUdpStreamSetupRequest = []() {  };
    _webServer.callbacks().onCommandRequest = [](const String& command)
    {
        _board.command(command);
        _webServer.notifyCommandResult(Result(true, ""));
    };

    _webServer.callbacks().onTcpStreamSetupRequest = [](const IPAddress& ip, uint16_t port, uint32_t latency) 
    {
        const Result result = _tcpStreamer.connect(tcpStreamer::Config{ip, port});
        if(result == false) return result;
        else
        {
            if(latency > 0) _latency = latency;
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
        Serial.println(ESP.getFreeHeap(),DEC);
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
        return Result(true, "{\"command\": \"b\"}");
    };

    _webServer.callbacks().onStreamStopRequest = []
    {
        _board.command("s");
        return Result(true, "{\"command\": \"s\"}");
    };

    _board.onData([](const std::vector<uint8>& data)
    {
        if((data.at(0) & 0xF0) == cytonMessage::END)
        {
            const uint8 packetType = data[0];
            std::copy(data.begin(), data.end(), std::back_inserter(_buffer));
            _buffer[0] = cytonMessage::BEGIN;
            _buffer.push_back(packetType);
            _tcpStreamer.send(_buffer);
            _buffer.clear();
        }
        else if((data.at(0) == spiMessage::GAIN) && (data.at(1) == spiMessage::GAIN))
        {
            std::vector<uint8> gains;
            gains.reserve(data.size() - 2);
            for(uint32_t i = 2; i < data.size(); i++) gains.push_back(parseGain(data[i]));
            _tcpStreamer.send(gains);
        }
        else if(data.at(0) == spiMessage::MULTI) storeCommandResponse(data);
        else if(data.at(0) == spiMessage::LAST)
        {
            storeCommandResponse(data);
            _tcpStreamer.send(_commandResponse);
            _commandResponse.clear();
        } 
    });

    _webServer.begin();
    _board.begin();
}

void loop()
{
    _webServer.loop();
    _board.loop();
}
