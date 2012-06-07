/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef WebSoupRequestManager_h
#define WebSoupRequestManager_h

#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/WTFString.h>

typedef struct _GInputStream GInputStream;
typedef struct _GSimpleAsyncResult GSimpleAsyncResult;

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
class DataReference;
class MessageID;
}

namespace WebKit {

class WebProcess;
struct WebSoupRequestAsyncData;

class WebSoupRequestManager {
    WTF_MAKE_NONCOPYABLE(WebSoupRequestManager);
public:
    explicit WebSoupRequestManager(WebProcess*);
    ~WebSoupRequestManager();

    void send(GSimpleAsyncResult*, GCancellable*);
    GInputStream* finish(GSimpleAsyncResult*);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    void didReceiveWebSoupRequestManagerMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    void registerURIScheme(const String& scheme);
    void didHandleURIRequest(const CoreIPC::DataReference&, uint64_t contentLength, const String& mimeType, uint64_t requestID);
    void didReceiveURIRequestData(const CoreIPC::DataReference&, uint64_t requestID);

    WebProcess* m_process;
    GRefPtr<GPtrArray> m_schemes;
    HashMap<uint64_t, OwnPtr<WebSoupRequestAsyncData> > m_requestMap;
};

} // namespace WebKit

#endif
