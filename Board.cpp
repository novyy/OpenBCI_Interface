
// Name: Board.cpp
// Date: 04-Nov-2020
// Purpose: Cyton board communication interface
// Author: Piotr Nowinski

#include "Board.h"
#include "CytonMessageDefs.h"
#include "SpiMessageDefs.h"

#include <SPISlave.h>

Board::Board()
{}

board::Callbacks& Board::callbacks()
{
    return m_callbacks;
}

void Board::begin()
{
    /*
    SPISlave.onData([this](uint8* data, size_t len) { processPacket(data, len); });
    SPISlave.onDataSent([this] 
    {
        m_currentComman.clear();
        SPISlave.setData(m_currentComman.c_str());
    });

    SPISlave.onStatusSent([this] { SPISlave.setStatus(spiMessage::STATUS); });

    SPISlave.setStatus(spiMessage::STATUS);
    SPISlave.setData(m_currentComman.c_str());
    SPISlave.begin();
    */
}

bool Board::connected() const
{
    return m_connected;
}

void Board::command(const String& command)
{
    m_currentComman = command;
    //SPISlave.setData(command.c_str());

    if(command == "b")
    {
        m_streaming = true;
        m_nextSentTime = 0;
        m_gainSet = false;
    } 
    else if(command == "s") m_streaming = false;
    else if(command == "V") m_response = "v3.1.2$$$";
    else if(command.startsWith("~"))
    {
        if(command.endsWith("6"))
        {
            m_packetInterval = 4000;
            m_response = "Success: Sample rate is 250Hz$$$";
        } 
        else if(command.endsWith("5"))
        {
            m_packetInterval = 2000;
            m_response = "Success: Sample rate is 500Hz$$$\n";
        } 
        else if(command.endsWith("4"))
        {
            m_packetInterval = 1000;
            m_response = "Success: Sample rate is 1000Hz$$$\n";
        } 
    }
    else m_response = "x";
}

void Board::processSamples(const uint8* pData, uint32 size)
{
    const uint8 packetType = pData[0];
    m_buffer.push_back(cytonMessage::BEGIN);
    for (uint32 i = 1; i < size; i++) m_buffer.push_back(pData[i]);
    m_buffer.push_back(packetType);
    
    if(m_callbacks.onData != nullptr) m_callbacks.onData(m_buffer);
    m_buffer.clear();
}

void Board::processGains(const uint8* pData, uint32 size)
{
    if(size < 4) return;

    uint32 index = 2;
    const uint8 channelsNumber = pData[index++];
    
    static const std::vector<uint8> GAINS = {1, 2, 4, 6, 8, 12, 24};
    static const auto parseGain = [](uint8 value) { return (value > GAINS.size() - 1) ? GAINS[value] : GAINS[GAINS.size() - 1]; };

    for(uint32_t i = 0; i < channelsNumber; i++) m_buffer.push_back(parseGain(pData[index++]));

    if(m_callbacks.onGains != nullptr) m_callbacks.onGains(m_buffer);
    m_buffer.clear();
}

void Board::storeCommandResponse(const uint8* pData, uint32 size)
{
    for(uint32_t i = 1; i < size; i++)
    {
        if(pData[i] == 0x00) break;
        m_commandResult.concat(static_cast<const char>(pData[i]));
    }
}

void Board::processPacket(const uint8* pData, uint32 size)
{
    if((pData[0] & 0xF0) == cytonMessage::END) processSamples(pData, size);
    else if((pData[0] == spiMessage::GAIN) && (pData[1] == spiMessage::GAIN)) processGains(pData, size);
    else if(pData[0] == spiMessage::MULTI) storeCommandResponse(pData, size);
    else if(pData[0] == spiMessage::LAST)
    {
        storeCommandResponse(pData, size);
        m_callbacks.onCommandResult(m_commandResult);
        m_commandResult.clear();
    }
}

#include <cmath>

void Board::loop()
{
    
    const double amplitude = 350.0;
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

        processPacket(m_spiSlaveBuffer.data(), m_spiSlaveBuffer.size());
        m_spiSlaveBuffer.clear();
    }
    else if(m_response.length() > 0)
    {        
        if(m_callbacks.onCommandResult != nullptr) m_callbacks.onCommandResult(m_response);
        m_response.clear();
    }

    if(m_gainSet == false)
    {
        const uint8 gains[] = {spiMessage::GAIN, spiMessage::GAIN, 8, 6, 6, 6, 6, 6, 6, 6, 6};
        processPacket(gains, sizeof(gains));
        m_gainSet = true;
    }
}
