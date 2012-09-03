/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "WebPageProxy.h"

#include "NotImplemented.h"
#include "PageClientImpl.h"
#include "WebKitVersion.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"

#include <sys/utsname.h>

namespace WebKit {

Evas_Object* WebPageProxy::viewWidget()
{
    return static_cast<PageClientImpl*>(m_pageClient)->viewWidget();
}

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    WTF::String platform;
    WTF::String version;
    WTF::String osVersion;

#if PLATFORM(X11)
    platform = "X11";
#else
    platform = "Unknown";
#endif
    version = String::number(WEBKIT_MAJOR_VERSION) + '.' + String::number(WEBKIT_MINOR_VERSION) + '+';
    struct utsname name;
    if (uname(&name) != -1)
        osVersion = WTF::String(name.sysname) + " " + WTF::String(name.machine);
    else
        osVersion = "Unknown";

    return "Mozilla/5.0 (" + platform + "; " + osVersion + ") AppleWebKit/" + version
        + " (KHTML, like Gecko) Version/5.0 Safari/" + version;
}

void WebPageProxy::getEditorCommandsForKeyEvent(Vector<WTF::String>& commandsList)
{
    notImplemented();
}

void WebPageProxy::saveRecentSearches(const String&, const Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::loadRecentSearches(const String&, Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::setThemePath(const String& themePath)
{
    process()->send(Messages::WebPage::SetThemePath(themePath), m_pageID, 0);
}

} // namespace WebKit
