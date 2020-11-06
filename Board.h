
// Name: Board.h
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include <Arduino.h>

class Board
{
public:
  String getInfo();
  String getTcpStreamInfo();
  void setLatency(uint32 latency);
  uint32 getLatency() const;

private:
  uint32 m_latency;
};

