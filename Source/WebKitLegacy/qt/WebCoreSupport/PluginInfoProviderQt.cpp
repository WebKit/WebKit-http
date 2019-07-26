/*
 * Copyright (C) 2019 Konstantin Tokarev <annulen@yandex.ru>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PluginInfoProviderQt.h"

#include "ChromeClientQt.h"
#include "QWebPageAdapter.h"
#include "qwebpluginfactory.h"

#include <PluginDatabase.h>
#include <WebCore/Chrome.h>

using namespace WebCore;

namespace WebKit {

PluginInfoProviderQt& PluginInfoProviderQt::singleton()
{
    static PluginInfoProviderQt& pluginInfoProvider = adoptRef(*new PluginInfoProviderQt).leakRef();
    return pluginInfoProvider;
}

void PluginInfoProviderQt::refreshPlugins()
{
    PluginDatabase::installedPlugins()->refresh();
}

Vector<WebCore::PluginInfo> PluginInfoProviderQt::pluginInfo(WebCore::Page& page, Optional<Vector<WebCore::SupportedPluginIdentifier>>& /*outPlugins*/)
{
    // QTFIXME: Handle optional argument?
    Vector<WebCore::PluginInfo> outPlugins;
    QWebPageAdapter* qPage = 0;
    if (!page.chrome().client().isEmptyChromeClient())
        qPage = static_cast<ChromeClientQt&>(page.chrome().client()).m_webPage;

    QWebPluginFactory* factory;
    if (qPage && (factory = qPage->pluginFactory)) {

        QList<QWebPluginFactory::Plugin> qplugins = factory->plugins();
        for (int i = 0; i < qplugins.count(); ++i) {
            const QWebPluginFactory::Plugin& qplugin = qplugins.at(i);
            PluginInfo info;
            info.name = qplugin.name;
            info.desc = qplugin.description;

            for (int j = 0; j < qplugin.mimeTypes.count(); ++j) {
                const QWebPluginFactory::MimeType& mimeType = qplugin.mimeTypes.at(j);

                MimeClassInfo mimeInfo;
                mimeInfo.type = mimeType.name;
                mimeInfo.desc = mimeType.description;
                for (int k = 0; k < mimeType.fileExtensions.count(); ++k)
                    mimeInfo.extensions.append(mimeType.fileExtensions.at(k));

                info.mimes.append(mimeInfo);
            }
            outPlugins.append(info);
        }
    }

    PluginDatabase* db = PluginDatabase::installedPlugins();
    const Vector<PluginPackage*> &plugins = db->plugins();

    for (auto* package : plugins) {
        PluginInfo info;
        info.name = package->name();
        info.file = package->fileName();
        info.desc = package->description();

        const auto& mimeToDescriptions = package->mimeToDescriptions();
        for (auto it = mimeToDescriptions.begin(); it != mimeToDescriptions.end(); ++it) {
            MimeClassInfo mime;

            mime.type = it->key;
            mime.desc = it->value;
            mime.extensions = package->mimeToExtensions().get(mime.type);

            info.mimes.append(mime);
        }

        outPlugins.append(info);
    }
    return outPlugins;
}

Vector<PluginInfo> PluginInfoProviderQt::webVisiblePluginInfo(Page& page, const WTF::URL&)
{
    // QTFIXME: Handle URL? Refactor implementation
    Optional<Vector<WebCore::SupportedPluginIdentifier>> supportedPluginIdentifiers;
    return pluginInfo(page, supportedPluginIdentifiers);
}

}
