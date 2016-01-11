#include "config.h"
#include "RunLoop.h"

namespace WTF {

RunLoop::RunLoop()
{
}

RunLoop::~RunLoop()
{
}

void RunLoop::run()
{
}

void RunLoop::stop()
{
}

void RunLoop::wakeUp()
{
}

RunLoop::TimerBase::TimerBase(RunLoop& runLoop)
    : m_runLoop(runLoop)
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

void RunLoop::TimerBase::start(double nextFireInterval, bool repeat)
{
}

void RunLoop::TimerBase::stop()
{
}

bool RunLoop::TimerBase::isActive() const
{
    return false;
}

} // namespace WTF
