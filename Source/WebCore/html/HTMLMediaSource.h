/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLMediaSource_h
#define HTMLMediaSource_h

#include "URLRegistry.h"
#include <wtf/Forward.h>

namespace WebCore {

class MediaSourcePrivate;
class TimeRanges;

class HTMLMediaSource : public URLRegistrable {
public:
    static void setRegistry(URLRegistry*);
    static HTMLMediaSource* lookup(const String& url) { return s_registry ? static_cast<HTMLMediaSource*>(s_registry->lookup(url)) : 0; }

    void ref() { refHTMLMediaSource(); }
    void deref() { derefHTMLMediaSource(); }

    // Called when an HTMLMediaElement is attempting to attach to this object,
    // and helps enforce attachment to at most one element at a time.
    // If already attached, returns false. Otherwise, must be in
    // 'closed' state, and returns true to indicate attachment success.
    // Reattachment allowed by first calling close() (even if already in 'closed').
    virtual bool attachToElement() = 0;
    virtual void setPrivateAndOpen(PassRef<MediaSourcePrivate>) = 0;
    virtual void close() = 0;
    virtual bool isClosed() const = 0;
    virtual double duration() const = 0;
    virtual PassRefPtr<TimeRanges> buffered() const = 0;
    virtual void refHTMLMediaSource() = 0;
    virtual void derefHTMLMediaSource() = 0;

    // URLRegistrable
    virtual URLRegistry& registry() const OVERRIDE { return *s_registry; }

private:
    static URLRegistry* s_registry;
};

}

#endif
