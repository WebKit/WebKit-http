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

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "V8NotificationCenter.h"

#include "ExceptionCode.h"
#include "NotImplemented.h"
#include "Notification.h"
#include "NotificationCenter.h"
#include "V8Binding.h"
#include "V8EventListener.h"
#include "V8Notification.h"
#include "V8Utilities.h"
#include "V8VoidCallback.h"
#include "WorkerContext.h"

namespace WebCore {

v8::Handle<v8::Value> V8NotificationCenter::createHTMLNotificationCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.NotificationCenter.CreateHTMLNotification()");
    NotificationCenter* notificationCenter = V8NotificationCenter::toNative(args.Holder());

    ExceptionCode ec = 0;
    String url = toWebCoreString(args[0]);
    RefPtr<Notification> notification = notificationCenter->createHTMLNotification(url, ec);

    if (ec)
        return setDOMException(ec, args.GetIsolate());

    notification->ref();
    return toV8(notification.get(), args.Holder(), args.GetIsolate());
}

v8::Handle<v8::Value> V8NotificationCenter::createNotificationCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.NotificationCenter.CreateNotification()");
    NotificationCenter* notificationCenter = V8NotificationCenter::toNative(args.Holder());

    ExceptionCode ec = 0;
    RefPtr<Notification> notification = notificationCenter->createNotification(toWebCoreString(args[0]), toWebCoreString(args[1]), toWebCoreString(args[2]), ec);

    if (ec)
        return setDOMException(ec, args.GetIsolate());

    notification->ref();
    return toV8(notification.get(), args.Holder(), args.GetIsolate());
}

v8::Handle<v8::Value> V8NotificationCenter::requestPermissionCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.NotificationCenter.RequestPermission()");
    NotificationCenter* notificationCenter = V8NotificationCenter::toNative(args.Holder());
    ScriptExecutionContext* context = notificationCenter->scriptExecutionContext();

    // Make sure that script execution context is valid.
    if (!context)
        return setDOMException(INVALID_STATE_ERR, args.GetIsolate());

    // Requesting permission is only valid from a page context.
    if (context->isWorkerContext())
        return setDOMException(NOT_SUPPORTED_ERR, args.GetIsolate());

    RefPtr<V8VoidCallback> callback;
    if (args.Length() > 0) {
        if (!args[0]->IsObject())
            return throwTypeError("Callback must be of valid type.", args.GetIsolate());
 
        callback = V8VoidCallback::create(args[0], context);
    }

    notificationCenter->requestPermission(callback.release());
    return v8::Undefined();
}


}  // namespace WebCore

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
