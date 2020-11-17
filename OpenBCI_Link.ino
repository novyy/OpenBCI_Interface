
// Name: OpenBCI_Interface.ino
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#define ARDUINO_ARCH_ESP8266
#define ESP8266

#include "WebServer.h"
#include "Board.h"
#include "TcpStreamer.h"
#include "UdpStreamer.h"
#include "CytonMessageDefs.h"
#include "SpiMessageDefs.h"
#include "DeadlineTimer.h"
#include <ESP8266WiFi.h>

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
std::unique_ptr<TcpStreamer> _pTcpStreamer;
std::unique_ptr<UdpStreamer> _pUdpStreamer;
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

String getTcpStreamStatus()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["connected"] = (_pTcpStreamer != nullptr) && _pTcpStreamer->connected();
    jsonDoc["type"] = "TCP";
    jsonDoc["latency"] = _systemInfo.latency;
    jsonDoc["delimiter"] = true;
    jsonDoc["output"] = "raw";
    if(_pTcpStreamer != nullptr)
    {
        jsonDoc["ip"] = _pTcpStreamer->getRemoteEndpoint().ipAddress.toString();
        jsonDoc["port"] = _pTcpStreamer->getRemoteEndpoint().port;
    }

    String output;
    serializeJson(jsonDoc, output);
    return output;
}

String getUdpStreamStatus()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["connected"] = (_pUdpStreamer != nullptr);
    jsonDoc["type"] = "UDP";
    jsonDoc["latency"] = _systemInfo.latency;
    jsonDoc["delimiter"] = false;
    jsonDoc["output"] = "raw";
    if(_pUdpStreamer != nullptr)
    {
        jsonDoc["ip"] = _pUdpStreamer->getRemoteEndpoint().ipAddress.toString();
        jsonDoc["port"] = _pUdpStreamer->getRemoteEndpoint().port;
    }

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

String getAllInfo()
{
    StaticJsonDocument<256> jsonDoc;
    jsonDoc["board_connected"] = _board.connected();
    jsonDoc["heap"] = ESP.getFreeHeap();
    jsonDoc["ip"] = _webServer.getLocalIP();
    jsonDoc["mac"] = "";
    jsonDoc["name"] = "OpenBCI";
    jsonDoc["num_channels"] = _systemInfo.channelsNumber;
    jsonDoc["version"] = VERSION;
    jsonDoc["latency"] = _systemInfo.latency;

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
    _webServer.callbacks().onAllInfoRequest = []() {  return getAllInfo(); };
    _webServer.callbacks().onTcpStreamInfoRequest = [] { return getTcpStreamStatus(); };
    _webServer.callbacks().onCommandRequest = [](const String& command)
    {
        Serial.println(String("Command: " + command));
        _board.command(command);
        _commandResponseTimeout.expiresIn(5000, [](){ _webServer.notifyCommandResult(Result(false, "Command timeout")); });
    };

    _webServer.callbacks().onTcpStreamSetupRequest = [](const IPAddress& ip, uint16_t port, uint32_t latency) 
    {
        _pUdpStreamer.reset();
        _pTcpStreamer = std::move(std::unique_ptr<TcpStreamer>(new TcpStreamer()));
        const Result result = _pTcpStreamer->connect(Endpoint{ip, port});
        if(result == false) return result;
        else
        {
            if(latency > 0) _systemInfo.latency = latency;
            return Result(true, getTcpStreamStatus());
        }
    };

    _webServer.callbacks().onUdpStreamSetupRequest = [](const IPAddress& ip, uint16_t port, uint32_t latency) 
    {
        _pTcpStreamer.reset();
        _pUdpStreamer = std::unique_ptr<UdpStreamer>(new UdpStreamer());
        _pUdpStreamer->configure(Endpoint{ip, port});
        
        if(latency > 0) _systemInfo.latency = latency;
        return Result(true, getUdpStreamStatus());
    };

    _webServer.callbacks().onTcpStreamDeleteRequest = []
    {
        if(_pTcpStreamer != nullptr)
        {
            _pTcpStreamer->disconnect();
            _pTcpStreamer.reset();
        }
        return getTcpStreamStatus();
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
            if(_pTcpStreamer != nullptr) _pTcpStreamer->send(_packetBuffer);
            if(_pUdpStreamer != nullptr) _pUdpStreamer->send(_packetBuffer);
             
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
        //_webServer.notifyCommandResult(Result(true, "{\"result\":\"" + data + "\"}"));
        _webServer.notifyCommandResult(Result(true, data));
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
