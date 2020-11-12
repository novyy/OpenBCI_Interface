// Name: DeadlineTimer.cpp
// Date: 04-Nov-2020
// Purpose: execute callback after specific number of milliseconds
// Author: Piotr Nowinski

#include "DeadlineTimer.h"

void DeadlineTimer::expiresIn(uint32 inMs, const std::function<void()> task)
{
    m_expiresAt = millis() + inMs;
}

void DeadlineTimer::loop()
{
    if((m_task != nullptr) && (millis() >= m_expiresAt))
    {
        m_task();
        m_task = nullptr;
    } 
}

void DeadlineTimer::cancel()
{
    m_task = nullptr;
}