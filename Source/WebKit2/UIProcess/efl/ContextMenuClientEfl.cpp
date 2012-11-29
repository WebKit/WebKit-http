/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#include "config.h"
#include "ContextMenuClientEfl.h"

#include "EwkViewImpl.h"
#include "NotImplemented.h"
#include "WKArray.h"
#include "WKPage.h"
#include "WebContextMenuItem.h"   
#include <Evas.h>

using namespace WebKit;

static inline ContextMenuClientEfl* toContextClientEfl(const void* clientInfo)
{
    return static_cast<ContextMenuClientEfl*>(const_cast<void*>(clientInfo));
}

ContextMenuClientEfl::ContextMenuClientEfl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
{
    WKPageRef pageRef = m_viewImpl->wkPage();
    ASSERT(pageRef);

    WKPageContextMenuClient contextMenuClient;
    memset(&contextMenuClient, 0, sizeof(WKPageContextMenuClient));
    contextMenuClient.version = kWKPageContextMenuClientCurrentVersion;
    contextMenuClient.clientInfo = this;
    contextMenuClient.getContextMenuFromProposedMenu_deprecatedForUseWithV0 = 0;
    contextMenuClient.customContextMenuItemSelected = 0;
    contextMenuClient.contextMenuDismissed = 0;
    contextMenuClient.getContextMenuFromProposedMenu = 0;

    WKPageSetPageContextMenuClient(pageRef, &contextMenuClient);
}

void ContextMenuClientEfl::getContextMenuFromProposedMenu(WKPageRef, WKArrayRef, WKArrayRef*, WKHitTestResultRef, WKTypeRef, const void*)
{
    notImplemented();
}
