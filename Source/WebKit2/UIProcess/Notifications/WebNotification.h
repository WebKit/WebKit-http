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

#ifndef WebNotification_h
#define WebNotification_h

#include "APIObject.h"
#include "WebSecurityOrigin.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {

class ArgumentDecoder;
class ArgumentEncoder;

} // namespace CoreIPC

namespace WebKit {

class WebNotification : public APIObject {
public:
    static const Type APIType = TypeNotification;
    
    static PassRefPtr<WebNotification> create(const String& title, const String& body, const String& iconURL, const String& originString, uint64_t notificationID)
    {
        return adoptRef(new WebNotification(title, body, iconURL, originString, notificationID));
    }
    
    const String& title() const { return m_title; }
    const String& body() const { return m_body; }
    const String& iconURL() const { return m_iconURL; }
    WebSecurityOrigin* origin() const { return m_origin.get(); }
    
    uint64_t notificationID() const { return m_notificationID; }

private:
    WebNotification(const String& title, const String& body, const String& iconURL, const String& originString, uint64_t notificationID);

    virtual Type type() const { return APIType; }
    
    String m_title;
    String m_body;
    String m_iconURL;
    RefPtr<WebSecurityOrigin> m_origin;
    uint64_t m_notificationID;
};

inline bool isNotificationIDValid(uint64_t id)
{
    // This check makes sure that the ID is not equal to values needed by
    // HashMap for bucketing.
    return id && id != static_cast<uint64_t>(-1);
}

} // namespace WebKit

#endif // WebNotification_h
