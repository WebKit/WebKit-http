/*
 * Copyright 2011 Hewlett-Packard Development Company, L.P. All rights reserved.
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
 * 3.  Neither the name of Hewlett-Packard nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "InitWebCoreQt.h"

#include "PlatformStrategiesQt.h"
#include "qwebelement_p.h"

#include <JavaScriptCore/InitializeThreading.h>
#include <QPixmap>
#include <WebCore/Image.h>
#include <WebCore/LogInitialization.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/RuntimeEnabledFeatures.h>
#include <WebCore/SecurityPolicy.h>
#include <wtf/MainThread.h>
#include <wtf/RunLoop.h>

namespace WebKit {

static QtStyleFacadeFactoryFunction initCallback = 0;

Q_DECL_EXPORT void setWebKitWidgetsInitCallback(QtStyleFacadeFactoryFunction callback)
{
    initCallback = callback;
}

//static WebCore::QStyleFacade* createStyleForPage(WebCore::Page* page)
//{
//    QWebPageAdapter* pageAdapter = 0;
//    if (page)
//        pageAdapter = static_cast<WebCore::ChromeClientQt&>(page->chrome().client()).m_webPage;
//    return initCallback(pageAdapter);
//}

// Called also from WebKit2's WebProcess
Q_DECL_EXPORT void initializeWebKitQt()
{
    // QTFIXME
//    if (initCallback) {
//        WebCore::RenderThemeQStyle::setStyleFactoryFunction(createStyleForPage);
//        WebCore::RenderThemeQt::setCustomTheme(WebCore::RenderThemeQStyle::create, new WebCore::ScrollbarThemeQStyle);
//    }
}

Q_DECL_EXPORT void setImagePlatformResource(const char* name, const QPixmap& pixmap)
{
    WebCore::Image::setPlatformResource(name, pixmap.toImage());
}

}

namespace WebCore {

Q_DECL_EXPORT void initializeWebCoreQt()
{
    static bool initialized = false;
    if (initialized)
        return;

#if !LOG_DISABLED
    WebCore::initializeLogChannelsIfNecessary();
#endif // !LOG_DISABLED
    JSC::initializeThreading();
    WTF::initializeMainThread();
    RunLoop::initializeMainRunLoop();
    WebCore::SecurityPolicy::setLocalLoadPolicy(WebCore::SecurityPolicy::AllowLocalLoadsForLocalAndSubstituteData);

    PlatformStrategiesQt::initialize();
    QtWebElementRuntime::initialize();

    auto& memoryCache = MemoryCache::singleton();
    if (!memoryCache.disabled())
        memoryCache.setDeadDecodedDataDeletionInterval(60_s);

    initialized = true;
}

}
