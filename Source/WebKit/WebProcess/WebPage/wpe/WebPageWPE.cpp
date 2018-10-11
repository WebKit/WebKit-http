/*
 * Copyright (C) 2014 Igalia S.L.
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
#include "WebPage.h"

#include "RuntimeEnabledFeatures.h"
#include "WebPreferencesKeys.h"
#include "WebPreferencesStore.h"
#include "WindowsKeyboardCodes.h"
#include <WebCore/NotImplemented.h>
#include <WebCore/Settings.h>
#include <WebCore/SharedBuffer.h>

namespace WebKit {
using namespace WebCore;

void WebPage::platformInitialize()
{
}

void WebPage::platformDetach()
{
}

void WebPage::platformEditorState(Frame&, EditorState&, IncludePostLayoutDataHint) const
{
    notImplemented();
}

bool WebPage::performDefaultBehaviorForKeyEvent(const WebKeyboardEvent& keyboardEvent)
{
    if (keyboardEvent.type() != WebEvent::KeyDown && keyboardEvent.type() != WebEvent::RawKeyDown)
        return false;

    switch (keyboardEvent.windowsVirtualKeyCode()) {
    case VK_PRIOR:
        scroll(m_page.get(), ScrollUp, ScrollByPage);
        break;
    case VK_NEXT:
        scroll(m_page.get(), ScrollDown, ScrollByPage);
        break;
    default:
        break;
    }

    return false;
}

bool WebPage::platformCanHandleRequest(const ResourceRequest&)
{
    notImplemented();
    return false;
}

String WebPage::platformUserAgent(const URL&) const
{
    notImplemented();
    return String();
}

} // namespace WebKit
