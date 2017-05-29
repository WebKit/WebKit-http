/*
 * Copyright (C) 2007 Kevin Ollivier
 * Copyright (C) 2009 Maxime Simon
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MainThread.h"

#include "WorkQueue.h"

#include <Application.h>
#include <Handler.h>

namespace WTF {

class MainThreadHandler : public BHandler {
public:
    static const uint32 kDispatchCommand = 'dpch';

    MainThreadHandler()
        : BHandler("WebCore main thread handler")
    {
    }

    virtual void MessageReceived(BMessage* message)
    {
        if (message->what == kDispatchCommand)
            dispatchFunctionsFromMainThread();
        else
            BHandler::MessageReceived(message);
    }
};

MainThreadHandler* mainThreadHandler;
static BLooper* mainThreadLooper;

void initializeMainThreadPlatform()
{
	// This handler is leaked at application exit time.
	mainThreadHandler = new MainThreadHandler();
	// initializeMainThreadPlatform() needs to be called on a valid BLooper thread.
	mainThreadLooper = BLooper::LooperForThread(find_thread(0));
    if (!mainThreadLooper)
        mainThreadLooper = be_app;
	mainThreadLooper->AddHandler(mainThreadHandler);
}

void scheduleDispatchFunctionsOnMainThread()
{
	// This method shall allow to process user events on the main thread
	// before calling dispatchFunctionsFromMainThread(). The message
	// we sent here will be inserted at the end of the message queue, so
	// even if we are the main thread itself, the purpose of this method
	// is achieved.
	mainThreadLooper->PostMessage(MainThreadHandler::kDispatchCommand,
	                              mainThreadHandler);
}

} // namespace WTF

