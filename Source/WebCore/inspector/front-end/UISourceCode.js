/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @constructor
 * @extends {WebInspector.Object}
 * @implements {WebInspector.ContentProvider}
 * @param {string} url
 * @param {WebInspector.Resource} resource
 * @param {WebInspector.ContentProvider} contentProvider
 * @param {WebInspector.SourceMapping=} sourceMapping
 */
WebInspector.UISourceCode = function(url, resource, contentProvider, sourceMapping)
{
    this._url = url;
    this._resource = resource;
    this._parsedURL = new WebInspector.ParsedURL(url);
    this._contentProvider = contentProvider;
    this._sourceMapping = sourceMapping;
    this.isContentScript = false;
    /**
     * @type Array.<function(?string,boolean,string)>
     */
    this._requestContentCallbacks = [];
    this._liveLocations = [];
    /**
     * @type {Array.<WebInspector.PresentationConsoleMessage>}
     */
    this._consoleMessages = [];
}

WebInspector.UISourceCode.Events = {
    ContentChanged: "ContentChanged",
    WorkingCopyChanged: "WorkingCopyChanged",
    TitleChanged: "TitleChanged",
    ConsoleMessageAdded: "ConsoleMessageAdded",
    ConsoleMessageRemoved: "ConsoleMessageRemoved",
    ConsoleMessagesCleared: "ConsoleMessagesCleared"
}

WebInspector.UISourceCode.prototype = {
    /**
     * @return {string}
     */
    get url()
    {
        return this._url;
    },

    /**
     * @param {string} url
     */
    urlChanged: function(url)
    {
        this._url = url;
        this._parsedURL = new WebInspector.ParsedURL(this._url);
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.TitleChanged, null);
    },

    /**
     * @return {WebInspector.Resource}
     */
    resource: function()
    {
        return this._resource;
    },

    /**
     * @return {WebInspector.ParsedURL}
     */
    get parsedURL()
    {
        return this._parsedURL;
    },

    /**
     * @return {?string}
     */
    contentURL: function()
    {
        return this._url;
    },

    /**
     * @return {WebInspector.ResourceType}
     */
    contentType: function()
    {
        return this._contentProvider.contentType();
    },

    /**
     * @param {function(?string,boolean,string)} callback
     */
    requestContent: function(callback)
    {
        if (this._contentLoaded) {
            callback(this._content, false, this._mimeType);
            return;
        }
        this._requestContentCallbacks.push(callback);
        if (this._requestContentCallbacks.length === 1)
            this._contentProvider.requestContent(this.fireContentAvailable.bind(this));
    },

    /**
     * @param {string} newContent
     */
    contentChanged: function(newContent)
    {
        console.assert(this._contentLoaded);
        var oldContent = this._content;
        this._content = newContent;
        delete this._workingCopy;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ContentChanged, {oldContent: oldContent, content: newContent});
    },

    /**
     * @return {boolean}
     */
    isEditable: function()
    {
        return false;
    },

    /**
     * @return {string}
     */
    workingCopy: function()
    {
        console.assert(this._contentLoaded);
        return this._workingCopy;
    },

    /**
     * @param {string} newWorkingCopy
     */
    setWorkingCopy: function(newWorkingCopy)
    {
        console.assert(this._contentLoaded);
        var oldWorkingCopy = this._workingCopy;
        if (this._content === newWorkingCopy)
            delete this._workingCopy;
        else
            this._workingCopy = newWorkingCopy;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.WorkingCopyChanged, {oldWorkingCopy: oldWorkingCopy, workingCopy: newWorkingCopy});
    },

    /**
     * @return {boolean}
     */
    isDirty: function()
    {
        return this._contentLoaded && typeof this._workingCopy !== "undefined" && this._workingCopy !== this._content;
    },

    /**
     * @return {string}
     */
    mimeType: function()
    {
        return this._mimeType;
    },

    /**
     * @return {?string}
     */
    content: function()
    {
        return this._content;
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback)
    {
        this._contentProvider.searchInContent(query, caseSensitive, isRegex, callback);
    },

    /**
     * @param {?string} content
     * @param {boolean} contentEncoded
     * @param {string} mimeType
     */
    fireContentAvailable: function(content, contentEncoded, mimeType)
    {
        this._contentLoaded = true;
        this._mimeType = mimeType;
        this._content = content;

        var callbacks = this._requestContentCallbacks.slice();
        this._requestContentCallbacks = [];
        for (var i = 0; i < callbacks.length; ++i)
            callbacks[i](content, contentEncoded, mimeType);
    },

    /**
     * @return {boolean}
     */
    contentLoaded: function()
    {
        return this._contentLoaded;
    },

    /**
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {DebuggerAgent.Location}
     */
    uiLocationToRawLocation: function(lineNumber, columnNumber)
    {
        return this._sourceMapping.uiLocationToRawLocation(this, lineNumber, columnNumber);
    },

    /**
     * @param {WebInspector.Script.Location} liveLocation
     */
    addLiveLocation: function(liveLocation)
    {
        this._liveLocations.push(liveLocation);
    },

    /**
     * @param {WebInspector.Script.Location} liveLocation
     */
    removeLiveLocation: function(liveLocation)
    {
        this._liveLocations.remove(liveLocation);
    },

    updateLiveLocations: function()
    {
        var locationsCopy = this._liveLocations.slice();
        for (var i = 0; i < locationsCopy.length; ++i)
            locationsCopy[i].update();
    },

    /**
     * @param {WebInspector.UILocation} uiLocation
     * @return {WebInspector.UILocation}
     */
    overrideLocation: function(uiLocation)
    {
        return uiLocation;
    },

    /**
     * @return {Array.<WebInspector.PresentationConsoleMessage>}
     */
    consoleMessages: function()
    {
        return this._consoleMessages;
    },

    /**
     * @param {WebInspector.PresentationConsoleMessage} message
     */
    consoleMessageAdded: function(message)
    {
        this._consoleMessages.push(message);
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ConsoleMessageAdded, message);
    },

    /**
     * @param {WebInspector.PresentationConsoleMessage} message
     */
    consoleMessageRemoved: function(message)
    {
        this._consoleMessages.remove(message);
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ConsoleMessageRemoved, message);
    },

    consoleMessagesCleared: function()
    {
        this._consoleMessages = [];
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ConsoleMessagesCleared);
    }
}

WebInspector.UISourceCode.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @interface
 */
WebInspector.UISourceCodeProvider = function()
{
}

WebInspector.UISourceCodeProvider.Events = {
    UISourceCodeAdded: "UISourceCodeAdded",
    UISourceCodeReplaced: "UISourceCodeReplaced",
    UISourceCodeRemoved: "UISourceCodeRemoved"
}

WebInspector.UISourceCodeProvider.prototype = {
    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function() {},

    /**
     * @param {string} eventType
     * @param {function(WebInspector.Event)} listener
     * @param {Object=} thisObject
     */
    addEventListener: function(eventType, listener, thisObject) { },

    /**
     * @param {string} eventType
     * @param {function(WebInspector.Event)} listener
     * @param {Object=} thisObject
     */
    removeEventListener: function(eventType, listener, thisObject) { }
}

/**
 * @constructor
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {number} lineNumber
 * @param {number} columnNumber
 */
WebInspector.UILocation = function(uiSourceCode, lineNumber, columnNumber)
{
    this.uiSourceCode = uiSourceCode;
    this.lineNumber = lineNumber;
    this.columnNumber = columnNumber;
}

WebInspector.UILocation.prototype = {
    /**
     * @return {DebuggerAgent.Location}
     */
    uiLocationToRawLocation: function()
    {
        return this.uiSourceCode.uiLocationToRawLocation(this.lineNumber, this.columnNumber);
    }
}
