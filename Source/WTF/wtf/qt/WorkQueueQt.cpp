#include "config.h"
#include "WorkQueue.h"

void WorkQueue::platformInitialize(const char* name, Type, QOS)
{
}

void WorkQueue::platformInvalidate()
{
}

void WorkQueue::dispatch(std::function<void ()> function)
{
}

void WorkQueue::dispatchAfter(std::chrono::nanoseconds duration, std::function<void ()> function)
{
}
