/*
 * Copyright (C) 2006, 2007, 2008, 2009 Google Inc. All rights reserved.
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

#ifndef V8LazyEventListener_h
#define V8LazyEventListener_h

#include "PlatformString.h"
#include "V8AbstractEventListener.h"
#include <v8.h>
#include <wtf/text/TextPosition.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class Event;
    class Frame;

    // V8LazyEventListener is a wrapper for a JavaScript code string that is compiled and evaluated when an event is fired.
    // A V8LazyEventListener is always a HTML event handler.
    class V8LazyEventListener : public V8AbstractEventListener {
    public:
        static PassRefPtr<V8LazyEventListener> create(const String& functionName, bool isSVGEvent, const String& code, const String& sourceURL, const TextPosition& position, const WorldContextHandle& worldContext)
        {
            return adoptRef(new V8LazyEventListener(functionName, isSVGEvent, code, sourceURL, position, worldContext));
        }

        virtual bool isLazy() const { return true; }

    protected:
        virtual void prepareListenerObject(ScriptExecutionContext*);

    private:
        V8LazyEventListener(const String& functionName, bool isSVGEvent, const String& code, const String sourceURL, const TextPosition&, const WorldContextHandle&);

        virtual v8::Local<v8::Value> callListenerFunction(ScriptExecutionContext*, v8::Handle<v8::Value> jsEvent, Event*);

        // Needs to return true for all event handlers implemented in JavaScript so that
        // the SVG code does not add the event handler in both
        // SVGUseElement::buildShadowTree and again in
        // SVGUseElement::transferEventListenersToShadowTree
        virtual bool wasCreatedFromMarkup() const { return true; }

        String m_functionName;
        bool m_isSVGEvent;
        String m_code;
        String m_sourceURL;
        TextPosition m_position;
    };

} // namespace WebCore

#endif // V8LazyEventListener_h
