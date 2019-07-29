/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef FrameNetworkingContextQt_h
#define FrameNetworkingContextQt_h

#include <WebCore/FrameNetworkingContext.h>

namespace WebCore {

class FrameNetworkingContextQt : public FrameNetworkingContext {
public:
    static Ref<FrameNetworkingContextQt> create(Frame*, QObject* originatingObject, bool mimeSniffingEnabled);

private:
    FrameNetworkingContextQt(Frame*, QObject* originatingObject, bool mimeSniffingEnabled);
    void setSession(std::unique_ptr<NetworkStorageSession>&&);

    NetworkStorageSession* storageSession() const override;

    QObject* originatingObject() const override;
    QNetworkAccessManager* networkAccessManager() const override;
    bool mimeSniffingEnabled() const override;
    bool thirdPartyCookiePolicyPermission(const QUrl&) const override;

    mutable std::unique_ptr<NetworkStorageSession> m_session;
    QObject* m_originatingObject;
    bool m_mimeSniffingEnabled;
};

}

#endif // FrameNetworkingContextQt_h
