
// Name: Board.cpp
// Date: 04-Nov-2020
// Purpose: Cyton board communication interface
// Author: Piotr Nowinski

#include "Board.h"
#include "CytonMessageDefs.h"
#include "SpiMessageDefs.h"

Board::Board()
{}

board::Callbacks& Board::callbacks()
{
    return m_callbacks;
}

void Board::begin()
{

}

bool Board::connected() const
{
    return m_connected;
}

void Board::command(const String& command)
{
    if(command == "b")
    {
        m_streaming = true;
        m_nextSentTime = 0;
    } 
    else if(command == "s") m_streaming = false;
    else if(command.startsWith("~"))
    {
        if(command.endsWith("6")) m_packetInterval = 4000;
        else if(command.endsWith("5")) m_packetInterval = 2000;
        else if(command.endsWith("4")) m_packetInterval = 1000;
        m_responseExpected = true;
    }
    else m_responseExpected = true;
}

void Board::processSamples(const std::vector<uint8>& data)
{
    const uint8 packetType = data[0];
    m_buffer.push_back(cytonMessage::BEGIN);
    std::copy(std::next(data.begin()), data.end(), std::back_inserter(m_buffer));
    m_buffer.push_back(packetType);
    
    if(m_callbacks.onData != nullptr) m_callbacks.onData(m_buffer);
    m_buffer.clear();
}

void Board::processGains(const std::vector<uint8>& data)
{
    if(data.size() < 4) return;

    const uint8 channelsNumber = data[2];
    
    static const std::vector<uint8> GAINS = {1, 2, 4, 6, 8, 12, 24};
    static const auto parseGain = [](uint8 value) { return (value > GAINS.size() - 1) ? GAINS[value] : GAINS[GAINS.size() - 1]; };

    for(uint32_t i = 3; i < channelsNumber; i++) m_buffer.push_back(parseGain(data[i]));

    if(m_callbacks.onGains != nullptr) m_callbacks.onGains(m_buffer);
    m_buffer.clear();
}

void Board::storeCommandResponse(const std::vector<uint8>& command)
{
    for(uint32_t i = 1; i < command.size(); i++)
    {
        if(command[i] == 0x00) break;
        m_commandResult.concat(static_cast<const char>(command[i]));
    }
}

void Board::processPacket(const std::vector<uint8>& data)
{
    if((data.at(0) & 0xF0) == cytonMessage::END) processSamples(data);
    else if((data.at(0) == spiMessage::GAIN) && (data.at(1) == spiMessage::GAIN)) processGains(data);
    else if(data.at(0) == spiMessage::MULTI) storeCommandResponse(data);
    else if(data.at(0) == spiMessage::LAST)
    {
        storeCommandResponse(data);
        m_callbacks.onCommandResult(m_commandResult);
        m_commandResult.clear();
    }
}

#include <cmath>

void Board::loop()
{
    const double amplitude = 150.0;
    const uint32_t frequency = 15.0;

    const uint32 usec = micros();
    if(m_streaming && (usec >= m_nextSentTime))
    {
        const uint32 timeSlip = (m_nextSentTime == 0) ? 0 : (usec - m_nextSentTime);
        m_nextSentTime = usec + m_packetInterval - timeSlip;

        const int32_t value = amplitude * sin(2 * PI * frequency * double(usec) * 0.000001);       

        m_spiSlaveBuffer.push_back(cytonMessage::END);
        m_spiSlaveBuffer.push_back(m_seqNum++);

        uint8 bitMove = 0;
        for(int i = 0; i < 24; i++) m_spiSlaveBuffer.push_back(0xff & (value >> ((2 - (bitMove++ % 3)) * 8)));
        m_spiSlaveBuffer.resize(spiMessage::SIZE);
        
        processPacket(m_spiSlaveBuffer);
        m_spiSlaveBuffer.clear();
    }
    else if(m_responseExpected)
    {        
        if(m_callbacks.onCommandResult != nullptr) m_callbacks.onCommandResult("x");
        m_responseExpected = false;
    }
}
