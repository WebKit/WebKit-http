/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef CustomProtocolManager_h
#define CustomProtocolManager_h

#if ENABLE(CUSTOM_PROTOCOLS)

#include "MessageID.h"
#include <wtf/HashSet.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
OBJC_CLASS WKCustomProtocol;
#endif


namespace CoreIPC {
class Connection;
class DataReference;
class MessageDecoder;
} // namespace CoreIPC

namespace WebCore {
class ResourceError;
class ResourceResponse;
} // namespace WebCore

namespace WebKit {

class CustomProtocolManager {
    WTF_MAKE_NONCOPYABLE(CustomProtocolManager);

public:
    static CustomProtocolManager& shared();
    
    void registerScheme(const String&);
    void unregisterScheme(const String&);
    bool supportsScheme(const String&);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::MessageDecoder&);
    void didFailWithError(uint64_t customProtocolID, const WebCore::ResourceError&);
    void didLoadData(uint64_t customProtocolID, const CoreIPC::DataReference&);
    void didReceiveResponse(uint64_t customProtocolID, const WebCore::ResourceResponse&, uint32_t cacheStoragePolicy);
    void didFinishLoading(uint64_t customProtocolID);
    
#if PLATFORM(MAC)
    static void registerCustomProtocolClass();
    void addCustomProtocol(WKCustomProtocol *);
    void removeCustomProtocol(WKCustomProtocol *);
#endif

private:
    CustomProtocolManager() { }
    void didReceiveCustomProtocolManagerMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::MessageDecoder&);
    
    HashSet<String> m_registeredSchemes;

#if PLATFORM(MAC)
    typedef HashMap<uint64_t, RetainPtr<WKCustomProtocol> > CustomProtocolMap;
    CustomProtocolMap m_customProtocolMap;
    WKCustomProtocol *protocolForID(uint64_t customProtocolID);
#endif
};

} // namespace WebKit

#endif // ENABLE(CUSTOM_PROTOCOLS)

#endif // CustomProtocolManager_h
