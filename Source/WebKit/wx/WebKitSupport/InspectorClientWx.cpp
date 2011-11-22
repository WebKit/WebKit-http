/*
 * Copyright (C) 2007 Kevin Ollivier  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "InspectorClientWx.h"

#include "NotImplemented.h"
#include "Page.h"
#include "PlatformString.h"

namespace WebCore {

InspectorClientWx::InspectorClientWx()
{
    notImplemented();
}

InspectorClientWx::~InspectorClientWx()
{
    notImplemented();
}

void InspectorClientWx::inspectorDestroyed()
{
    notImplemented();
}

void InspectorClientWx::openInspectorFrontend(WebCore::InspectorController*)
{
    notImplemented();
}

void InspectorClientWx::bringFrontendToFront()
{
    notImplemented();
}

void InspectorClientWx::highlight()
{
    notImplemented();
}

void InspectorClientWx::hideHighlight()
{
    notImplemented();
}

void InspectorClientWx::populateSetting(const String& key, String* setting)
{
    notImplemented();
}

void InspectorClientWx::storeSetting(const String& key, const String& setting)
{
    notImplemented();
}

bool InspectorClientWx::sendMessageToFrontend(const String&)
{
    notImplemented();
    return false;
}

};
