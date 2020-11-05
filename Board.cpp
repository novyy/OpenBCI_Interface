
// Name: Board.cpp
// Date: 04-Nov-2020
// Purpose:
// Author: Piotr Nowinski

#include "Board.h"

String Board::getInfo()
{
  return "{\"board_connected\":true,\"board_type\":\"cyton\",\"num_channels\":8,\"gains\":[24,24,24,24,24,24,24,24]}";
}

String Board::getTcpStreamInfo()
{
  return "{\"connected\":true,\"delimiter\":true,\"ip\":\"192.168.1.111\",\"output\":\"raw\",\"port\":6677,\"latency\":10000}";
}
