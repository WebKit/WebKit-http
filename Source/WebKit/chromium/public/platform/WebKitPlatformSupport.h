/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebKitPlatformSupport_h
#define WebKitPlatformSupport_h

#include "WebCommon.h"
#include "WebGraphicsContext3D.h"
#include "WebSerializedScriptValue.h"
#include "WebString.h"
#include "WebURL.h"
#include "WebVector.h"
#include <time.h>
#include "../../../../Platform/chromium/public/Platform.h"

#ifdef WIN32
typedef void *HANDLE;
#endif

namespace WebKit {

class WebApplicationCacheHost; // FIXME: Does this belong in platform?
class WebApplicationCacheHostClient; // FIXME: Does this belong in platform?
class WebIDBFactory; // FIXME: Does this belong in platform?
class WebPluginListBuilder; // FIXME: Does this belong in platform?
class WebSharedWorkerRepository; // FIXME: Does this belong in platform?

// FIXME: Eventually all these API will need to move to WebKit::Platform.
class WebKitPlatformSupport : public Platform {
public:
    // Indexed Database ----------------------------------------------------

    virtual WebIDBFactory* idbFactory() { return 0; }


    // Plugins -------------------------------------------------------------

    // If refresh is true, then cached information should not be used to
    // satisfy this call.
    virtual void getPluginList(bool refresh, WebPluginListBuilder*) { }


    // Shared Workers ------------------------------------------------------

    virtual WebSharedWorkerRepository* sharedWorkerRepository() { return 0; }

protected:
    ~WebKitPlatformSupport() { }
};

} // namespace WebKit

#endif
