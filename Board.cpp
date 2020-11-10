
// Name: Board.cpp
// Date: 04-Nov-2020
// Purpose: Cyton board communication interface
// Author: Piotr Nowinski

#include "Board.h"
#include "CytonMessageDefs.h"

Board::Board()
    : m_buffer(board::MESSAGE_SIZE)
{

}

void Board::begin()
{

}

void Board::command(const String& command)
{
    if(command == "b") m_streaming = true;
    else if(command == "s") m_streaming = false;
}

String Board::getInfo()
{
  return "{\"board_connected\":true,\"board_type\":\"cyton\",\"num_channels\":8,\"gains\":[24,24,24,24,24,24,24,24]}";
}

String Board::getTcpStreamInfo()
{
  return "{\"connected\":true,\"delimiter\":true,\"ip\":\"192.168.1.111\",\"output\":\"raw\",\"port\":6677,\"latency\":10000}";
}

void Board::onData(const std::function<void(const std::vector<uint8>&)> callback)
{
    m_onDataCallback = callback;
}

#include <cmath>

void Board::loop()
{
    const double amplitude = 150.0;
    const uint32_t frequency = 15.0;

    const uint32 usec = micros();
    if(m_streaming && (usec >= m_lastSentTime + 4000))
    {
        const int32_t value = amplitude * sin(2 * PI * frequency * double(usec) * 0.000001);
        uint32 index = 0;

        m_buffer[index++] = cytonMessage::END;
        m_buffer[index++] = m_seqNum++;

        uint8 bitMove = 0;
        for(int i = 0; i < 24; i++) m_buffer[index++] = 0xff & (value >> ((2 - (bitMove++ % 3)) * 8));

        if(m_onDataCallback != nullptr) m_onDataCallback(m_buffer);
    }
}
