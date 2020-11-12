// Name: DeadlineTimer.h
// Date: 04-Nov-2020
// Purpose: execute callback after specific number of milliseconds
// Author: Piotr Nowinski

#include <functional>
#include <Arduino.h>

class DeadlineTimer
{
public:
    void expiresIn(uint32 inMs, const std::function<void()> task);
    void cancel();
    void loop();
private:
    uint32_t m_expiresAt = 0;
    std::function<void()> m_task;
};