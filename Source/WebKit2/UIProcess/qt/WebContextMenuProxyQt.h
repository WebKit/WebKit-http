/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef WebContextMenuProxyQt_h
#define WebContextMenuProxyQt_h

#include "WebContextMenuProxy.h"
#include <PassOwnPtr.h>
#include <QtCore/QObject>

class QMenu;
class QtWebPageProxy;

namespace WebKit {
class WebContextMenuItemData;
}
class QtWebPageProxy;

namespace WebKit {

class WebContextMenuProxyQt : public QObject, public WebContextMenuProxy {
    Q_OBJECT
public:
    static PassRefPtr<WebContextMenuProxyQt> create(QtWebPageProxy*);

private Q_SLOTS:
    void actionTriggered(bool);

private:
    WebContextMenuProxyQt(QtWebPageProxy*);

    virtual void showContextMenu(const WebCore::IntPoint&, const Vector<WebContextMenuItemData>&);
    virtual void hideContextMenu();

    PassOwnPtr<QMenu> createContextMenu(const Vector<WebContextMenuItemData>& items) const;

    QtWebPageProxy* const m_webPageProxy;
};

} // namespace WebKit

#endif // WebContextMenuProxyQt_h
