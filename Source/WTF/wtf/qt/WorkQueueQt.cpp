/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WorkQueue.h"

#include <QObject>
#include <QProcess>
#include <QThread>

namespace WTF {

class WorkQueue::WorkItemQt : public QObject {
    Q_OBJECT
public:
    WorkItemQt(WorkQueue* workQueue, WTF::Function<void()> function)
        : m_queue(workQueue)
        , m_source(0)
        , m_signal(0)
        , m_function(WTFMove(function))
    {
    }

    WorkItemQt(WorkQueue* workQueue, QObject* source, const char* signal, WTF::Function<void()>&& function)
        : m_queue(workQueue)
        , m_source(source)
        , m_signal(signal)
        , m_function(WTFMove(function))
    {
        connect(m_source, m_signal, SLOT(execute()), Qt::QueuedConnection);
    }

    ~WorkItemQt()
    {
    }

    Q_SLOT void execute()
    {
        m_function();
    }

    Q_SLOT void executeAndDelete()
    {
        execute();
        delete this;
    }

    void timerEvent(QTimerEvent*) override
    {
        executeAndDelete();
    }

    RefPtr<WorkQueue> m_queue;
    QObject* m_source;
    const char* m_signal;
    WTF::Function<void()> m_function;
};

QSocketNotifier* WorkQueue::registerSocketEventHandler(int socketDescriptor, QSocketNotifier::Type type, WTF::Function<void()>&& function)
{
    ASSERT(m_workThread);

    QSocketNotifier* notifier = new QSocketNotifier(socketDescriptor, type, 0);
    notifier->setEnabled(false);
    notifier->moveToThread(m_workThread);
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, notifier, SIGNAL(activated(int)), WTFMove(function));
    itemQt->moveToThread(m_workThread);
    QMetaObject::invokeMethod(notifier, "setEnabled", Q_ARG(bool, true));
    return notifier;
}

void WorkQueue::platformInitialize(const char*, Type, QOS)
{
    m_workThread = new QThread();
    m_workThread->start();
}

void WorkQueue::platformInvalidate()
{
    m_workThread->exit();
    m_workThread->wait();
    delete m_workThread;
}

void WorkQueue::dispatch(WTF::Function<void()>&& function)
{
    ref();
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, WTFMove(function));
    itemQt->moveToThread(m_workThread);
    QMetaObject::invokeMethod(itemQt, "executeAndDelete", Qt::QueuedConnection);
}

void WorkQueue::dispatchAfter(WTF::Seconds duration, WTF::Function<void()>&& function)
{
    ref();
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, WTFMove(function));
    itemQt->startTimer(duration.milliseconds());
    itemQt->moveToThread(m_workThread);
}

void WorkQueue::dispatchOnTermination(QProcess* process, WTF::Function<void()>&& function)
{
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, process, SIGNAL(finished(int, QProcess::ExitStatus)), WTFMove(function));
    itemQt->moveToThread(m_workThread);
}

}

#include "WorkQueueQt.moc"
