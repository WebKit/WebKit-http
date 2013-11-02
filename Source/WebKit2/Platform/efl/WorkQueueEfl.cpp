/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WorkQueue.h"

#include <DispatchQueueEfl.h>
#include <DispatchQueueWorkItemEfl.h>

void WorkQueue::platformInitialize(const char* name)
{
    m_dispatchQueue = DispatchQueue::create(name);
}

void WorkQueue::platformInvalidate()
{
    RefPtr<DispatchQueue> dispatchQueue = m_dispatchQueue.release();
    dispatchQueue->stopThread();
}

void WorkQueue::registerSocketEventHandler(int fileDescriptor, const Function<void()>& function)
{
    if (!m_dispatchQueue)
        return;

    m_dispatchQueue->setSocketEventHandler(fileDescriptor, function);
}

void WorkQueue::unregisterSocketEventHandler(int fileDescriptor)
{
    UNUSED_PARAM(fileDescriptor);

    if (!m_dispatchQueue)
        return;

    m_dispatchQueue->clearSocketEventHandler();
}

void WorkQueue::dispatch(const Function<void()>& function)
{
    if (!m_dispatchQueue)
        return;

    m_dispatchQueue->dispatch(WorkItem::create(this, function));
}

void WorkQueue::dispatchAfterDelay(const Function<void()>& function, double delay)
{
    if (!m_dispatchQueue)
        return;

    m_dispatchQueue->dispatch(TimerWorkItem::create(this, function, delay));
}
