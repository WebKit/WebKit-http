/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.CSSObserver = function()
{
    WebInspector.Object.call(this);
};

WebInspector.CSSObserver.prototype = {
    constructor: WebInspector.CSSObserver,

    // Events defined by the "CSS" domain (see WebCore/inspector/Inspector.json).

    mediaQueryResultChanged: function()
    {
        WebInspector.cssStyleManager.mediaQueryResultChanged();
    },

    styleSheetChanged: function(styleSheetId)
    {
        WebInspector.cssStyleManager.styleSheetChanged(styleSheetId);
    },

    styleSheetAdded: function(header)
    {
        // FIXME: Not implemented. <rdar://problem/13213680>
    },

    styleSheetRemoved: function(header)
    {
        // FIXME: Not implemented. <rdar://problem/13213680>
    },

    namedFlowCreated: function(namedFlow)
    {
        WebInspector.domTreeManager.namedFlowCreated(namedFlow);
    },

    namedFlowRemoved: function(documentNodeId, flowName)
    {
        WebInspector.domTreeManager.namedFlowRemoved(documentNodeId, flowName);
    },

    regionLayoutUpdated: function(namedFlow)
    {
        WebInspector.domTreeManager.regionLayoutUpdated(namedFlow);
    },

    regionOversetChanged: function(namedFlow)
    {
        WebInspector.domTreeManager.regionOversetChanged(namedFlow);
    },

    registeredNamedFlowContentElement: function(documentNodeId, flowName, contentNodeId, nextContentElementNodeId)
    {
        WebInspector.domTreeManager.registeredNamedFlowContentElement(documentNodeId, flowName, contentNodeId, nextContentElementNodeId);
    },

    unregisteredNamedFlowContentElement: function(documentNodeId, flowName, contentNodeId)
    {
        WebInspector.domTreeManager.unregisteredNamedFlowContentElement(documentNodeId, flowName, contentNodeId);
    }
};

WebInspector.CSSObserver.prototype.__proto__ = WebInspector.Object.prototype;
