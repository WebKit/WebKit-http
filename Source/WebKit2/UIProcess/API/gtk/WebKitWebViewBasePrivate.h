/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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

#ifndef WebKitWebViewBasePrivate_h
#define WebKitWebViewBasePrivate_h

#include "WebContextMenuProxyGtk.h"
#include "WebKitPrivate.h"
#include "WebKitWebViewBase.h"
#include "WebPageProxy.h"

using namespace WebKit;

WebKitWebViewBase* webkitWebViewBaseCreate(WebContext*, WebPageGroup*);
GtkIMContext* webkitWebViewBaseGetIMContext(WebKitWebViewBase*);
WebPageProxy* webkitWebViewBaseGetPage(WebKitWebViewBase*);
void webkitWebViewBaseCreateWebPage(WebKitWebViewBase*, WKContextRef, WKPageGroupRef);
void webkitWebViewBaseSetTooltipText(WebKitWebViewBase*, const char*);
void webkitWebViewBaseSetTooltipArea(WebKitWebViewBase*, const WebCore::IntRect&);
void webkitWebViewBaseForwardNextKeyEvent(WebKitWebViewBase*);
void webkitWebViewBaseStartDrag(WebKitWebViewBase*, const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage);
void webkitWebViewBaseChildMoveResize(WebKitWebViewBase*, GtkWidget*, const WebCore::IntRect&);
void webkitWebViewBaseEnterFullScreen(WebKitWebViewBase*);
void webkitWebViewBaseExitFullScreen(WebKitWebViewBase*);
void webkitWebViewBaseInitializeFullScreenClient(WebKitWebViewBase*, const WKFullScreenClientGtk*);
void webkitWebViewBaseSetInspectorViewHeight(WebKitWebViewBase*, unsigned height);
void webkitWebViewBaseSetActiveContextMenuProxy(WebKitWebViewBase*, WebContextMenuProxyGtk*);
WebContextMenuProxyGtk* webkitWebViewBaseGetActiveContextMenuProxy(WebKitWebViewBase*);
GdkEvent* webkitWebViewBaseTakeContextMenuEvent(WebKitWebViewBase*);

#if USE(TEXTURE_MAPPER_GL)
void webkitWebViewBaseQueueDrawOfAcceleratedCompositingResults(WebKitWebViewBase*);
#endif

#endif // WebKitWebViewBasePrivate_h
