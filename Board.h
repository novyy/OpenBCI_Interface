
// Name: Board.h
// Date: 04-Nov-2020
// Purpose: Cyton board communication interface
// Author: Piotr Nowinski

#include <Arduino.h>
#include <vector>
#include <functional>
namespace board
{
    static const uint32_t MESSAGE_SIZE = 32;
}

class Board
{
public:
    Board();
    void begin();
    void loop();
    String getInfo();
    bool connected() const;
    void command(const String& command);
    void onData(const std::function<void(const std::vector<uint8>&)> callback);

private:
    std::vector<uint8> m_buffer;
    std::function<void(const std::vector<uint8>&)> m_onDataCallback;

    // wave mock
    uint32 m_lastSentTime = 0;
    uint8 m_seqNum = 0;
    bool m_streaming = false;
    bool m_connected = false;
};

