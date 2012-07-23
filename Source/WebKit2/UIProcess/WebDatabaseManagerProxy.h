/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WebDatabaseManagerProxy_h
#define WebDatabaseManagerProxy_h

#if ENABLE(SQL_DATABASE)

#include "APIObject.h"
#include "Arguments.h"
#include "GenericCallback.h"
#include "OriginAndDatabases.h"
#include "WebDatabaseManagerProxyClient.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
class MessageID;
}

namespace WebKit {

class WebContext;
class WebProcessProxy;
class WebSecurityOrigin;

typedef GenericCallback<WKArrayRef> ArrayCallback;

class WebDatabaseManagerProxy : public APIObject {
public:
    static const Type APIType = TypeDatabaseManager;

    static PassRefPtr<WebDatabaseManagerProxy> create(WebContext*);
    virtual ~WebDatabaseManagerProxy();

    void invalidate();
    void clearContext() { m_webContext = 0; }

    void initializeClient(const WKDatabaseManagerClient*);

    void getDatabasesByOrigin(PassRefPtr<ArrayCallback>);
    void getDatabaseOrigins(PassRefPtr<ArrayCallback>);
    void deleteDatabaseWithNameForOrigin(const String& databaseIdentifier, WebSecurityOrigin*);
    void deleteDatabasesForOrigin(WebSecurityOrigin*);
    void deleteAllDatabases();
    void setQuotaForOrigin(WebSecurityOrigin*, uint64_t quota);
    
    static String originKey();
    static String originQuotaKey();
    static String originUsageKey();
    static String databaseDetailsKey();
    static String databaseDetailsNameKey();
    static String databaseDetailsDisplayNameKey();
    static String databaseDetailsExpectedUsageKey();
    static String databaseDetailsCurrentUsageKey();

    void didReceiveWebDatabaseManagerProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    bool shouldTerminate(WebProcessProxy*) const;

private:
    explicit WebDatabaseManagerProxy(WebContext*);

    virtual Type type() const { return APIType; }

    // Message handlers.
    void didGetDatabasesByOrigin(const Vector<OriginAndDatabases>& originAndDatabases, uint64_t callbackID);
    void didGetDatabaseOrigins(const Vector<String>& originIdentifiers, uint64_t callbackID);
    void didModifyOrigin(const String& originIdentifier);
    void didModifyDatabase(const String& originIdentifier, const String& databaseIdentifier);

    WebContext* m_webContext;
    HashMap<uint64_t, RefPtr<ArrayCallback> > m_arrayCallbacks;

    WebDatabaseManagerProxyClient m_client;
};

} // namespace WebKit

#endif // ENABLE(SQL_DATABASE)

#endif // DatabaseManagerProxy_h
