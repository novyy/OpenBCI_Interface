
// Name: Board.h
// Date: 04-Nov-2020
// Purpose: Cyton board communication interface
// Author: Piotr Nowinski

#include <Arduino.h>
#include <vector>
#include <functional>

namespace board
{
    struct Callbacks
    {
        std::function<void(const std::vector<uint8>&)> onData;
        std::function<void(const std::vector<uint8>&)> onGains;
        std::function<void(const String&)> onCommandResult;
    };
}

class Board
{
public:
    Board();
    void begin();
    void loop();
    bool connected() const;
    void command(const String& command);
    board::Callbacks& callbacks();
    
private:
    void processPacket(const std::vector<uint8>& data);
    void processSamples(const std::vector<uint8>& data);
    void processGains(const std::vector<uint8>& data);
    void storeCommandResponse(const std::vector<uint8>& command);

private:
    std::vector<uint8> m_buffer;
    String m_commandResult;
    board::Callbacks m_callbacks;
    
    // wave mock
    std::vector<uint8> m_spiSlaveBuffer;
    uint32 m_nextSentTime = 0;
    uint8 m_seqNum = 0;
    bool m_streaming = false;
    bool m_connected = false;
    bool m_responseExpected = false;
    uint32_t m_packetInterval = 0;
};

