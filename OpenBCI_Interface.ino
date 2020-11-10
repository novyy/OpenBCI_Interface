
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
uint32_t _latency = 0;
uint32_t _lastPacketSent = 0;
std::vector<uint8> _packetBuffer;
std::vector<uint8> _commandResponse;
std::vector<uint8> _gains;

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

String getBoardInfo()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["board_connected"] = _board.connected();
    jsonDoc["board_type"] = "cyton";
    jsonDoc["num_channels"] = 8;

    JsonArray gains = jsonDoc.createNestedArray("gains");
    for (const auto& gain : _gains) gains.add(gain);

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

void processPacket(const std::vector<uint8>& data)
{
    const uint8 packetType = data[0];
    _packetBuffer.push_back(cytonMessage::BEGIN);
    std::copy(std::next(data.begin()), data.end(), std::back_inserter(_packetBuffer));
    _packetBuffer.push_back(packetType);
    const uint32_t currentTime = micros();
    if((currentTime > _lastPacketSent + _latency) || (_packetBuffer.size() >= MAX_PACKETS_PER_SEND_TCP * cytonMessage::SIZE))
    {
        _tcpStreamer.send(_packetBuffer);
        _packetBuffer.clear();
        _lastPacketSent = currentTime;
    }
}

void saveGain(const std::vector<uint8>& data)
{
    static const std::vector<uint8> GAINS = {1, 2, 4, 6, 8, 12, 24};
    static const auto parseGain = [](uint8 value) { return (value > GAINS.size() - 1) ? GAINS[value] : GAINS[GAINS.size() - 1]; };

    _gains.clear();
    for(uint32_t i = 2; i < data.size(); i++) _gains.push_back(parseGain(data[i]));
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
        if((data.at(0) & 0xF0) == cytonMessage::END) processPacket(data);
        else if((data.at(0) == spiMessage::GAIN) && (data.at(1) == spiMessage::GAIN)) saveGain(data);
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
