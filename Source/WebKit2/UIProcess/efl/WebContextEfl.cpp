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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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
#include "WebContext.h"

#include <Efreet.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/NotImplemented.h>

namespace WebKit {

String WebContext::applicationCacheDirectory()
{
    return String::fromUTF8(efreet_cache_home_get()) + "/WebKitEfl/Applications";
}

void WebContext::platformInitializeWebProcess(WebProcessCreationParameters&)
{
    notImplemented();
}

void WebContext::platformInvalidateContext()
{
    notImplemented();
}

String WebContext::platformDefaultDatabaseDirectory() const
{
    return String::fromUTF8(efreet_data_home_get()) + "/WebKitEfl/Databases";
}

String WebContext::platformDefaultIconDatabasePath() const
{
    notImplemented();
    return "";
}

String WebContext::platformDefaultLocalStorageDirectory() const
{
    return String::fromUTF8(efreet_data_home_get()) + "/WebKitEfl/LocalStorage";
}

} // namespace WebKit
