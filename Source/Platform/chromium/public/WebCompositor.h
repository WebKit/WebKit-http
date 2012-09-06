/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebCompositor_h
#define WebCompositor_h

#include "WebCommon.h"

namespace WebKit {

class WebThread;

// This class contains global routines for interacting with the
// compositor.
//
// All calls to the WebCompositor must be made from the main thread.
class WebCompositor {
public:
    // Initializes the compositor. Threaded compositing is enabled by passing in
    // a non-null WebThread. No compositor classes or methods should be used
    // prior to calling initialize.
    WEBKIT_EXPORT static void initialize(WebThread*);

    // Returns whether the compositor was initialized with threading enabled.
    WEBKIT_EXPORT static bool isThreadingEnabled();

    // Shuts down the compositor. This must be called when all compositor data
    // types have been deleted. No compositor classes or methods should be used
    // after shutdown.
    WEBKIT_EXPORT static void shutdown();

    // These may only be called before initialize.
    WEBKIT_EXPORT static void setPerTilePaintingEnabled(bool);
    WEBKIT_EXPORT static void setPartialSwapEnabled(bool);
    WEBKIT_EXPORT static void setAcceleratedAnimationEnabled(bool);

protected:
    virtual ~WebCompositor() { }
};

} // namespace WebKit

#endif
