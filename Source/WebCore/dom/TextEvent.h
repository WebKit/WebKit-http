/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#ifndef TextEvent_h
#define TextEvent_h

#include "TextEventInputType.h"
#include "UIEvent.h"

namespace WebCore {

    class DocumentFragment;

    class TextEvent : public UIEvent {
    public:

        static PassRefPtr<TextEvent> create();
        static PassRefPtr<TextEvent> create(PassRefPtr<AbstractView>, const String& data, TextEventInputType = TextEventInputKeyboard);
        static PassRefPtr<TextEvent> createForPlainTextPaste(PassRefPtr<AbstractView> view, const String& data, bool shouldSmartReplace);
        static PassRefPtr<TextEvent> createForFragmentPaste(PassRefPtr<AbstractView> view, PassRefPtr<DocumentFragment> data, bool shouldSmartReplace, bool shouldMatchStyle);
        static PassRefPtr<TextEvent> createForDrop(PassRefPtr<AbstractView> view, const String& data);

        virtual ~TextEvent();
    
        void initTextEvent(const AtomicString& type, bool canBubble, bool cancelable, PassRefPtr<AbstractView>, const String& data);
    
        String data() const { return m_data; }

        virtual bool isTextEvent() const;

        bool isLineBreak() const { return m_inputType == TextEventInputLineBreak; }
        bool isComposition() const { return m_inputType == TextEventInputComposition; }
        bool isBackTab() const { return m_inputType == TextEventInputBackTab; }
        bool isPaste() const { return m_inputType == TextEventInputPaste; }
        bool isDrop() const { return m_inputType == TextEventInputDrop; }

        bool shouldSmartReplace() const { return m_shouldSmartReplace; }
        bool shouldMatchStyle() const { return m_shouldMatchStyle; }
        DocumentFragment* pastingFragment() const { return m_pastingFragment.get(); }

    private:
        TextEvent();

        TextEvent(PassRefPtr<AbstractView>, const String& data, TextEventInputType = TextEventInputKeyboard);
        TextEvent(PassRefPtr<AbstractView>, const String& data, PassRefPtr<DocumentFragment>,
                  bool shouldSmartReplace, bool shouldMatchStyle);

        TextEventInputType m_inputType;
        String m_data;

        RefPtr<DocumentFragment> m_pastingFragment;
        bool m_shouldSmartReplace;
        bool m_shouldMatchStyle;
    };

} // namespace WebCore

#endif // TextEvent_h
