/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebIconDatabase_h
#define WebIconDatabase_h

#include "APIObject.h"

#include "Connection.h"
#include "WebIconDatabaseClient.h"
#include <WebCore/IconDatabaseClient.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>

namespace CoreIPC {
class ArgumentDecoder;
class DataReference;
class MessageID;
}

namespace WebCore {
class IconDatabase;
class Image;
}

namespace WebKit {

class WebContext;

class WebIconDatabase : public APIObject, public WebCore::IconDatabaseClient {
public:
    static const Type APIType = TypeIconDatabase;

    static PassRefPtr<WebIconDatabase> create(WebContext*);
    virtual ~WebIconDatabase();

    void invalidate();
    void clearContext() { m_webContext = 0; }
    void setDatabasePath(const String&);
    void enableDatabaseCleanup();

    void retainIconForPageURL(const String&);
    void releaseIconForPageURL(const String&);
    void setIconURLForPageURL(const String&, const String&);
    void setIconDataForIconURL(const CoreIPC::DataReference&, const String&);
    
    void synchronousIconDataForPageURL(const String&, CoreIPC::DataReference&);
    void synchronousIconURLForPageURL(const String&, String&);
    void synchronousIconDataKnownForIconURL(const String&, bool&) const;
    void synchronousLoadDecisionForIconURL(const String&, int&) const;
    
    void getLoadDecisionForIconURL(const String&, uint64_t callbackID);

    WebCore::Image* imageForPageURL(const String&);
    
    void removeAllIcons();
    void checkIntegrityBeforeOpening();
    void close();

    void initializeIconDatabaseClient(const WKIconDatabaseClient*);

    // WebCore::IconDatabaseClient
    virtual bool performImport();
    virtual void didImportIconURLForPageURL(const String&);
    virtual void didImportIconDataForPageURL(const String&);
    virtual void didChangeIconForPageURL(const String&);
    virtual void didRemoveAllIcons();
    virtual void didFinishURLImport();
    
    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*, OwnPtr<CoreIPC::ArgumentEncoder>&);

private:
    WebIconDatabase(WebContext*);

    virtual Type type() const { return APIType; }

    void didReceiveWebIconDatabaseMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void didReceiveSyncWebIconDatabaseMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*, OwnPtr<CoreIPC::ArgumentEncoder>&);

    WebContext* m_webContext;
    
    OwnPtr<WebCore::IconDatabase> m_iconDatabaseImpl;
    bool m_urlImportCompleted;
    bool m_databaseCleanupDisabled;
    HashMap<uint64_t, String> m_pendingLoadDecisionURLMap;

    WebIconDatabaseClient m_iconDatabaseClient;
};

} // namespace WebKit

#endif // WebIconDatabase_h
