/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

/**
 * @interface
 */
WebInspector.LinkifierFormatter = function()
{
}

WebInspector.LinkifierFormatter.prototype = {
    /**
     * @param {Element} anchor
     * @param {WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation) { }
}

/**
 * @constructor
 * @param {WebInspector.LinkifierFormatter=} formatter
 */
WebInspector.Linkifier = function(formatter)
{
    this._formatter = formatter || new WebInspector.Linkifier.DefaultFormatter();
    this._liveLocations = [];
}

WebInspector.Linkifier.prototype = {
    /**
     * @param {string} sourceURL
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @param {string=} classes
     */
    linkifyLocation: function(sourceURL, lineNumber, columnNumber, classes)
    {
        var rawLocation = WebInspector.debuggerModel.createRawLocationByURL(sourceURL, lineNumber, columnNumber || 0);
        if (!rawLocation)
            return WebInspector.linkifyResourceAsNode(sourceURL, lineNumber, classes);
        return this.linkifyRawLocation(rawLocation, classes);
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @param {string=} classes
     */
    linkifyRawLocation: function(rawLocation, classes)
    {
        var script = WebInspector.debuggerModel.scriptForId(rawLocation.scriptId);
        if (!script)
            return null;
        var anchor = WebInspector.linkifyURLAsNode("", "", classes, false);
        var liveLocation = script.createLiveLocation(rawLocation, this._updateAnchor.bind(this, anchor));
        this._liveLocations.push(liveLocation);
        return anchor;
    },

    reset: function()
    {
        for (var i = 0; i < this._liveLocations.length; ++i)
            this._liveLocations[i].dispose();
        this._liveLocations = [];
    },

    /**
     * @param {Element} anchor
     * @param {WebInspector.UILocation} uiLocation
     */
    _updateAnchor: function(anchor, uiLocation)
    {
        anchor.preferredPanel = "scripts";
        anchor.href = sanitizeHref(uiLocation.uiSourceCode.url);
        anchor.uiSourceCode = uiLocation.uiSourceCode;
        anchor.lineNumber = uiLocation.lineNumber;
        this._formatter.formatLiveAnchor(anchor, uiLocation);
    }
}

/**
 * @constructor
 * @implements {WebInspector.LinkifierFormatter}
 * @param {number=} maxLength
 */
WebInspector.Linkifier.DefaultFormatter = function(maxLength)
{
    this._maxLength = maxLength;
}

WebInspector.Linkifier.DefaultFormatter.prototype = {
    /**
     * @param {Element} anchor
     * @param {WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation)
    {
        anchor.textContent = WebInspector.formatLinkText(uiLocation.uiSourceCode.url, uiLocation.lineNumber);

        var text = WebInspector.formatLinkText(uiLocation.uiSourceCode.url, uiLocation.lineNumber);
        if (this._maxLength)
            text = text.trimMiddle(this._maxLength);
        anchor.textContent = text;
    }
}

WebInspector.Linkifier.DefaultFormatter.prototype.__proto__ = WebInspector.LinkifierFormatter.prototype;
