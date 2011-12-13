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
 * @param {string} id
 * @param {string} url
 * @param {boolean} isContentScript
 * @param {WebInspector.RawSourceCode} rawSourceCode
 * @param {WebInspector.ContentProvider} contentProvider
 */
WebInspector.UISourceCode = function(id, url, isContentScript, rawSourceCode, contentProvider)
{
    this._id = id;
    this._url = url;
    this._isContentScript = isContentScript;
    this._rawSourceCode = rawSourceCode;
    this._contentProvider = contentProvider;
    /**
     * @type Array.<function(string,string)>
     */
    this._requestContentCallbacks = [];
}

WebInspector.UISourceCode.Events = {
    ContentChanged: "content-changed"
}

WebInspector.UISourceCode.prototype = {
    /**
     * @return {string}
     */
    get id()
    {
        return this._id;
    },

    /**
     * @return {string}
     */
    get url()
    {
        return this._url;
    },

    /**
     * @return {boolean}
     */
    get isContentScript()
    {
        return this._isContentScript;
    },

    /**
     * @return {WebInspector.RawSourceCode}
     */
    get rawSourceCode()
    {
        return this._rawSourceCode;
    },

    /**
     * @param {function(string,string)} callback
     */
    requestContent: function(callback)
    {
        if (this._contentLoaded) {
            callback(this._mimeType, this._content);
            return;
        }

        this._requestContentCallbacks.push(callback);
        if (this._requestContentCallbacks.length === 1)
            this._contentProvider.requestContent(this._didRequestContent.bind(this));
    },

    /**
     * @param {string} newContent
     */
    contentChanged: function(newContent)
    {
        console.assert(this._contentLoaded);
        this._content = newContent;
        this.dispatchEventToListeners(WebInspector.UISourceCode.Events.ContentChanged);
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
     * @type {string}
     */
    get domain()
    {
        if (!this._domain)
            this._parseURL();
        
        return this._domain;
    },
    
    /**
     * @type {string}
     */
    get folderName()
    {
        if (!this._folderName)
            this._parseURL();
        
        return this._folderName;
    },
    
    /**
     * @type {string}
     */
    get displayName()
    {
        if (!this._displayName)
            this._parseURL();
        
        return this._displayName;
    },
    
    _parseURL: function()
    {
        var parsedURL = this.url.asParsedURL();
        var url = parsedURL ? parsedURL.path : this.url;

        var folderName = "";
        var displayName = url;

        var pathLength = displayName.indexOf("?");
        if (pathLength === -1)
            pathLength = displayName.length;

        var fromIndex = displayName.lastIndexOf("/", pathLength - 2);
        if (fromIndex !== -1) {
            folderName = displayName.substring(0, fromIndex);
            displayName = displayName.substring(fromIndex + 1);
        }

        if (displayName.length > 80)
            displayName = "\u2026" + displayName.substring(displayName.length - 80);

        if (folderName.length > 80)
            folderName = "\u2026" + folderName.substring(folderName.length - 80);

        this._domain = parsedURL ? parsedURL.host : "";
        this._folderName = folderName;
        this._displayName = displayName;
    },

    /**
     * @param {string} mimeType
     * @param {string} content
     */
    _didRequestContent: function(mimeType, content)
    {
        this._contentLoaded = true;
        this._mimeType = mimeType;
        this._content = content;

        for (var i = 0; i < this._requestContentCallbacks.length; ++i)
            this._requestContentCallbacks[i](mimeType, content);
        this._requestContentCallbacks = [];
    }
}

WebInspector.UISourceCode.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @interface
 */
WebInspector.ContentProvider = function() { }
WebInspector.ContentProvider.prototype = {
    /**
     * @param {function(string,string)} callback
     */
    requestContent: function(callback) { },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInContent: function(query, caseSensitive, isRegex, callback) { }
}

/**
 * @constructor
 * @param {number} lineNumber
 * @param {string} lineContent
 */
WebInspector.ContentProvider.SearchMatch = function(lineNumber, lineContent) {
    this.lineNumber = lineNumber;
    this.lineContent = lineContent;
}
