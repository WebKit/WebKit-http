/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef WebRuntimeFeatures_h
#define WebRuntimeFeatures_h

#define HAS_WEBAUDIO_RUNTIMEFEATURES 1

#include "WebCommon.h"

namespace WebKit {

// This class is used to enable runtime features of WebKit.  It is unspecified
// whether a feature is enabled by default.  In the future, a feature may be
// promoted from disabled by default to enabled by default once it reaches a
// certain level of maturity.
class WebRuntimeFeatures {
public:
    WEBKIT_API static void enableDatabase(bool);
    WEBKIT_API static bool isDatabaseEnabled();

    WEBKIT_API static void enableLocalStorage(bool);
    WEBKIT_API static bool isLocalStorageEnabled();

    WEBKIT_API static void enableSessionStorage(bool);
    WEBKIT_API static bool isSessionStorageEnabled();

    WEBKIT_API static void enableMediaPlayer(bool);
    WEBKIT_API static bool isMediaPlayerEnabled();

    WEBKIT_API static void enableSockets(bool);
    WEBKIT_API static bool isSocketsEnabled();

    WEBKIT_API static void enableNotifications(bool);
    WEBKIT_API static bool isNotificationsEnabled();

    WEBKIT_API static void enableApplicationCache(bool);
    WEBKIT_API static bool isApplicationCacheEnabled();
    
    WEBKIT_API static void enableDataTransferItems(bool);
    WEBKIT_API static bool isDataTransferItemsEnabled();

    WEBKIT_API static void enableGeolocation(bool);
    WEBKIT_API static bool isGeolocationEnabled();

    WEBKIT_API static void enableIndexedDatabase(bool);
    WEBKIT_API static bool isIndexedDatabaseEnabled();

    WEBKIT_API static void enableWebAudio(bool);
    WEBKIT_API static bool isWebAudioEnabled();

    WEBKIT_API static void enablePushState(bool);
    WEBKIT_API static bool isPushStateEnabled(bool);

    WEBKIT_API static void enableTouch(bool);
    WEBKIT_API static bool isTouchEnabled();

    WEBKIT_API static void enableDeviceMotion(bool);
    WEBKIT_API static bool isDeviceMotionEnabled();

    WEBKIT_API static void enableDeviceOrientation(bool);
    WEBKIT_API static bool isDeviceOrientationEnabled();

    WEBKIT_API static void enableSpeechInput(bool);
    WEBKIT_API static bool isSpeechInputEnabled();

    WEBKIT_API static void enableXHRResponseBlob(bool);
    WEBKIT_API static bool isXHRResponseBlobEnabled();

    WEBKIT_API static void enableFileSystem(bool);
    WEBKIT_API static bool isFileSystemEnabled();
    
    WEBKIT_API static void enableJavaScriptI18NAPI(bool);
    WEBKIT_API static bool isJavaScriptI18NAPIEnabled();

    WEBKIT_API static void enableQuota(bool);
    WEBKIT_API static bool isQuotaEnabled();

    WEBKIT_API static void enableSpellCheckAPI(bool);
    WEBKIT_API static bool isSpellCheckAPIEnabled();

private:
    WebRuntimeFeatures();
};

} // namespace WebKit

#endif
