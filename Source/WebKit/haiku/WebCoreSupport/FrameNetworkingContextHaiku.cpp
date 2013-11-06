/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Samsung Electronics
 * Copyright (C) 2012 ProFUSION embedded systems
 *
 * All rights reserved.
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
#include "FrameNetworkingContextHaiku.h"

#include "NotImplemented.h"
#include "ResourceHandle.h"

#include <UrlContext.h>

namespace WebCore {

PassRefPtr<FrameNetworkingContextHaiku> FrameNetworkingContextHaiku::create(Frame* frame)
{
    return adoptRef(new FrameNetworkingContextHaiku(frame));
}

FrameNetworkingContextHaiku::FrameNetworkingContextHaiku(Frame* frame)
    : FrameNetworkingContext(frame)
{
    m_context = new BUrlContext();
}

FrameNetworkingContextHaiku::~FrameNetworkingContextHaiku()
{
    delete m_context;
}

BUrlContext* FrameNetworkingContextHaiku::context()
{
    return m_context;
}

void FrameNetworkingContextHaiku::setContext(BUrlContext* context)
{
    delete m_context;
    m_context = context;
}

uint64_t FrameNetworkingContextHaiku::initiatingPageID() const
{
    notImplemented();
    return 0;
}

}
