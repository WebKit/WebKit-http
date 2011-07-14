/*
 * Copyright (C) 2007-2009 Google Inc. All rights reserved.
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
#if ENABLE(INSPECTOR)
#include "V8InspectorFrontendHost.h"

#include "InspectorController.h"
#include "InspectorFrontendClient.h"
#include "InspectorFrontendHost.h"
#if !PLATFORM(QT)
#include "PlatformBridge.h"
#endif
#include "PlatformString.h"

#include "V8Binding.h"
#include "V8MouseEvent.h"
#include "V8Proxy.h"

namespace WebCore {

v8::Handle<v8::Value> V8InspectorFrontendHost::platformCallback(const v8::Arguments&)
{
#if defined(OS_MACOSX)
    return v8String("mac");
#elif defined(OS_LINUX)
    return v8String("linux");
#elif defined(OS_FREEBSD)
    return v8String("freebsd");
#elif defined(OS_OPENBSD)
    return v8String("openbsd");
#elif defined(OS_SOLARIS)
    return v8String("solaris");
#elif defined(OS_WIN)
    return v8String("windows");
#else
    return v8String("unknown");
#endif
}

v8::Handle<v8::Value> V8InspectorFrontendHost::portCallback(const v8::Arguments&)
{
    return v8::Undefined();
}

v8::Handle<v8::Value> V8InspectorFrontendHost::showContextMenuCallback(const v8::Arguments& args)
{
    if (args.Length() < 2)
        return v8::Undefined();

    v8::Local<v8::Object> eventWrapper = v8::Local<v8::Object>::Cast(args[0]);
    if (!V8MouseEvent::info.equals(V8DOMWrapper::domWrapperType(eventWrapper)))
        return v8::Undefined();

    Event* event = V8Event::toNative(eventWrapper);
    if (!args[1]->IsArray())
        return v8::Undefined();

    v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args[1]);
    Vector<ContextMenuItem*> items;

    for (size_t i = 0; i < array->Length(); ++i) {
        v8::Local<v8::Object> item = v8::Local<v8::Object>::Cast(array->Get(i));
        v8::Local<v8::Value> type = item->Get(v8::String::New("type"));
        v8::Local<v8::Value> id = item->Get(v8::String::New("id"));
        v8::Local<v8::Value> label = item->Get(v8::String::New("label"));
        v8::Local<v8::Value> enabled = item->Get(v8::String::New("enabled"));
        v8::Local<v8::Value> checked = item->Get(v8::String::New("checked"));
        if (!type->IsString())
            continue;
        String typeString = toWebCoreStringWithNullCheck(type);
        if (typeString == "separator") {
            items.append(new ContextMenuItem(SeparatorType,
                                             ContextMenuItemCustomTagNoAction,
                                             String()));
        } else {
            ContextMenuAction typedId = static_cast<ContextMenuAction>(ContextMenuItemBaseCustomTag + id->ToInt32()->Value());
            ContextMenuItem* menuItem = new ContextMenuItem((typeString == "checkbox" ? CheckableActionType : ActionType), typedId, toWebCoreStringWithNullCheck(label));
            if (checked->IsBoolean())
                menuItem->setChecked(checked->ToBoolean()->Value());
            if (enabled->IsBoolean())
                menuItem->setEnabled(enabled->ToBoolean()->Value());
            items.append(menuItem);
        }
    }

    InspectorFrontendHost* frontendHost = V8InspectorFrontendHost::toNative(args.Holder());
    frontendHost->showContextMenu(event, items);

    return v8::Undefined();
}

#if !PLATFORM(QT)
static v8::Handle<v8::Value> histogramEnumeration(const char* name, const v8::Arguments& args, int boundaryValue)
{
    if (args.Length() < 1 || !args[0]->IsInt32())
        return v8::Undefined();

    int sample = args[0]->ToInt32()->Value();
    if (sample < boundaryValue)
        PlatformBridge::histogramEnumeration(name, sample, boundaryValue);

    return v8::Undefined();
}
#endif

v8::Handle<v8::Value> V8InspectorFrontendHost::recordActionTakenCallback(const v8::Arguments& args)
{
#if !PLATFORM(QT)
    return histogramEnumeration("DevTools.ActionTaken", args, 100);
#else
    return v8::Undefined();
#endif
}

v8::Handle<v8::Value> V8InspectorFrontendHost::recordPanelShownCallback(const v8::Arguments& args)
{
#if !PLATFORM(QT)
    return histogramEnumeration("DevTools.PanelShown", args, 20);
#else
    return v8::Undefined();
#endif
}

v8::Handle<v8::Value> V8InspectorFrontendHost::recordSettingChangedCallback(const v8::Arguments& args)
{
#if !PLATFORM(QT)
    return histogramEnumeration("DevTools.SettingChanged", args, 100);
#else
    return v8::Undefined();
#endif
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
