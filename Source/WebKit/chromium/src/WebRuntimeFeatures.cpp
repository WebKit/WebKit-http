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

#include "config.h"
#include "WebRuntimeFeatures.h"

#include "AbstractDatabase.h"
#include "RuntimeEnabledFeatures.h"
#include "WebMediaPlayerClientImpl.h"
#include "WebSocket.h"

#include <wtf/UnusedParam.h>

using namespace WebCore;

namespace WebKit {

void WebRuntimeFeatures::enableDatabase(bool enable)
{
#if ENABLE(DATABASE)
    AbstractDatabase::setIsAvailable(enable);
#endif
}

bool WebRuntimeFeatures::isDatabaseEnabled()
{
#if ENABLE(DATABASE)
    return AbstractDatabase::isAvailable();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableLocalStorage(bool enable)
{
#if ENABLE(DOM_STORAGE)
    RuntimeEnabledFeatures::setLocalStorageEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isLocalStorageEnabled()
{
#if ENABLE(DOM_STORAGE)
    return RuntimeEnabledFeatures::localStorageEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableSessionStorage(bool enable)
{
#if ENABLE(DOM_STORAGE)
    RuntimeEnabledFeatures::setSessionStorageEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isSessionStorageEnabled()
{
#if ENABLE(DOM_STORAGE)
    return RuntimeEnabledFeatures::sessionStorageEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableMediaPlayer(bool enable)
{
#if ENABLE(VIDEO)
    WebMediaPlayerClientImpl::setIsEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isMediaPlayerEnabled()
{
#if ENABLE(VIDEO)
    return WebMediaPlayerClientImpl::isEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableSockets(bool enable)
{
#if ENABLE(WEB_SOCKETS)
    WebSocket::setIsAvailable(enable);
#endif
}

bool WebRuntimeFeatures::isSocketsEnabled()
{
#if ENABLE(WEB_SOCKETS)
    return WebSocket::isAvailable();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableNotifications(bool enable)
{
#if ENABLE(NOTIFICATIONS)
    RuntimeEnabledFeatures::setWebkitNotificationsEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isNotificationsEnabled()
{
#if ENABLE(NOTIFICATIONS)
    return RuntimeEnabledFeatures::webkitNotificationsEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableApplicationCache(bool enable)
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    RuntimeEnabledFeatures::setApplicationCacheEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isApplicationCacheEnabled()
{
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
    return RuntimeEnabledFeatures::applicationCacheEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableDataTransferItems(bool enable)
{
#if ENABLE(DATA_TRANSFER_ITEMS)
    RuntimeEnabledFeatures::setDataTransferItemsEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isDataTransferItemsEnabled()
{
#if ENABLE(DATA_TRANSFER_ITEMS)
    return RuntimeEnabledFeatures::dataTransferItemsEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableGeolocation(bool enable)
{
#if ENABLE(GEOLOCATION)
    RuntimeEnabledFeatures::setGeolocationEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isGeolocationEnabled()
{
#if ENABLE(GEOLOCATION)
    return RuntimeEnabledFeatures::geolocationEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableIndexedDatabase(bool enable)
{
#if ENABLE(INDEXED_DATABASE)
    RuntimeEnabledFeatures::setWebkitIndexedDBEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isIndexedDatabaseEnabled()
{
#if ENABLE(INDEXED_DATABASE)
    return RuntimeEnabledFeatures::webkitIndexedDBEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableWebAudio(bool enable)
{
#if ENABLE(WEB_AUDIO)
    RuntimeEnabledFeatures::setWebkitAudioContextEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isWebAudioEnabled()
{
#if ENABLE(WEB_AUDIO)
    return RuntimeEnabledFeatures::webkitAudioContextEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enablePushState(bool enable)
{
    RuntimeEnabledFeatures::setPushStateEnabled(enable);
}

bool WebRuntimeFeatures::isPushStateEnabled(bool enable)
{
    return RuntimeEnabledFeatures::pushStateEnabled();
}

void WebRuntimeFeatures::enableTouch(bool enable)
{
#if ENABLE(TOUCH_EVENTS)
    RuntimeEnabledFeatures::setTouchEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isTouchEnabled()
{
#if ENABLE(TOUCH_EVENTS)
    return RuntimeEnabledFeatures::touchEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableDeviceMotion(bool enable)
{
    RuntimeEnabledFeatures::setDeviceMotionEnabled(enable);
}

bool WebRuntimeFeatures::isDeviceMotionEnabled()
{
    return RuntimeEnabledFeatures::deviceMotionEnabled();
}

void WebRuntimeFeatures::enableDeviceOrientation(bool enable)
{
    RuntimeEnabledFeatures::setDeviceOrientationEnabled(enable);
}

bool WebRuntimeFeatures::isDeviceOrientationEnabled()
{
    return RuntimeEnabledFeatures::deviceOrientationEnabled();
}

void WebRuntimeFeatures::enableSpeechInput(bool enable)
{
    RuntimeEnabledFeatures::setSpeechInputEnabled(enable);
}

bool WebRuntimeFeatures::isSpeechInputEnabled()
{
    return RuntimeEnabledFeatures::speechInputEnabled();
}

void WebRuntimeFeatures::enableXHRResponseBlob(bool enable)
{
#if ENABLE(XHR_RESPONSE_BLOB)
    RuntimeEnabledFeatures::setXHRResponseBlobEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isXHRResponseBlobEnabled()
{
#if ENABLE(XHR_RESPONSE_BLOB)
    return RuntimeEnabledFeatures::xhrResponseBlobEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableFileSystem(bool enable)
{
#if ENABLE(FILE_SYSTEM)
    RuntimeEnabledFeatures::setFileSystemEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isFileSystemEnabled()
{
#if ENABLE(FILE_SYSTEM)
    return RuntimeEnabledFeatures::fileSystemEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableJavaScriptI18NAPI(bool enable)
{
#if ENABLE(JAVASCRIPT_I18N_API)
    RuntimeEnabledFeatures::setJavaScriptI18NAPIEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isJavaScriptI18NAPIEnabled()
{
#if ENABLE(JAVASCRIPT_I18N_API)
    return RuntimeEnabledFeatures::javaScriptI18NAPIEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableQuota(bool enable)
{
#if ENABLE(QUOTA)
    RuntimeEnabledFeatures::setQuotaEnabled(enable);
#endif
}

bool WebRuntimeFeatures::isQuotaEnabled()
{
#if ENABLE(QUOTA)
    return RuntimeEnabledFeatures::quotaEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableMediaStream(bool enable)
{
#if ENABLE(MEDIA_STREAM)
    RuntimeEnabledFeatures::setMediaStreamEnabled(enable);
#else
    UNUSED_PARAM(enable);
#endif
}

bool WebRuntimeFeatures::isMediaStreamEnabled()
{
#if ENABLE(MEDIA_STREAM)
    return RuntimeEnabledFeatures::mediaStreamEnabled();
#else
    return false;
#endif
}

void WebRuntimeFeatures::enableFullScreenAPI(bool enable)
{
#if ENABLE(FULLSCREEN_API)
    RuntimeEnabledFeatures::setWebkitFullScreenAPIEnabled(enable);
#else
    UNUSED_PARAM(enable);
#endif
}

bool WebRuntimeFeatures::isFullScreenAPIEnabled()
{
#if ENABLE(FULLSCREEN_API)
    return RuntimeEnabledFeatures::webkitFullScreenAPIEnabled();
#else
    return false;
#endif
}

} // namespace WebKit
