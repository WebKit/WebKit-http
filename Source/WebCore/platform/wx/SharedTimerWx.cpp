/*
 * Copyright (C) 2007 Kevin Ollivier <kevino@theolliviers.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include "SharedTimer.h"
#include "Widget.h"

#include <wtf/Assertions.h>
#include <wtf/CurrentTime.h>
#include <stdio.h>

#include "wx/defs.h"
#include "wx/timer.h"

namespace WebCore {

static void (*sharedTimerFiredFunction)();

class WebKitTimer: public wxTimer
{
    public:
        WebKitTimer();
        ~WebKitTimer();
        virtual void Notify();
};

WebKitTimer::WebKitTimer()
{
}

WebKitTimer::~WebKitTimer()
{
}

void WebKitTimer::Notify()
{
    sharedTimerFiredFunction();
}

static WebKitTimer* wkTimer; 

void setSharedTimerFiredFunction(void (*f)())
{
    sharedTimerFiredFunction = f;
}

void setSharedTimerFireInterval(double interval)
{
    ASSERT(sharedTimerFiredFunction);
    
    if (!wkTimer)
        wkTimer = new WebKitTimer();
        
    int intervalInMS = interval * 1000;

    // sanity check
    if (intervalInMS < 1)
        intervalInMS = 1;

    wkTimer->Start(intervalInMS, wxTIMER_ONE_SHOT);
}

void stopSharedTimer()
{
    if (wkTimer)
        wkTimer->Stop();
}

}
