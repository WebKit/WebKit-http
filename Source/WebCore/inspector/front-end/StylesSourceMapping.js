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
 * @constructor
 * @implements {WebInspector.SourceMapping}
 * @param {WebInspector.Workspace} workspace
 */
WebInspector.StylesSourceMapping = function(workspace)
{
    this._workspace = workspace;
    this._workspace.addEventListener(WebInspector.Workspace.Events.ProjectWillReset, this._reset, this);
    this._workspace.addEventListener(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, this._uiSourceCodeAddedToWorkspace, this);

    this._uiSourceCodeForURL = {};
}

WebInspector.StylesSourceMapping.prototype = {
    /**
     * @param {WebInspector.RawLocation} rawLocation
     * @return {WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var location = /** @type WebInspector.CSSLocation */ rawLocation;
        var uiSourceCode = this._uiSourceCodeForURL[location.url];
        return new WebInspector.UILocation(uiSourceCode, location.lineNumber, 0);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {WebInspector.RawLocation}
     */
    uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        return new WebInspector.CSSLocation(uiSourceCode.contentURL() || "", lineNumber);
    },

    _uiSourceCodeAddedToWorkspace: function(event)
    {
        var uiSourceCode = /** @type {WebInspector.UISourceCode} */ event.data;
        if (!uiSourceCode.url || this._uiSourceCodeForURL[uiSourceCode.url])
            return;
        if (uiSourceCode.contentType() !== WebInspector.resourceTypes.Stylesheet)
            return;
        if (!WebInspector.resourceForURL(uiSourceCode.url))
            return;

        this._addUISourceCode(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    _addUISourceCode: function(uiSourceCode)
    {
        this._uiSourceCodeForURL[uiSourceCode.url] = uiSourceCode;
        uiSourceCode.setSourceMapping(this);
        var styleFile = new WebInspector.StyleFile(uiSourceCode);
        uiSourceCode.setStyleFile(styleFile);
        WebInspector.cssModel.setSourceMapping(uiSourceCode.url, this);
    },

    _reset: function()
    {
        this._uiSourceCodeForURL = {};
        WebInspector.cssModel.resetSourceMappings();
    }
}

/**
 * @constructor
 * @param {WebInspector.UISourceCode} uiSourceCode
 */
WebInspector.StyleFile = function(uiSourceCode)
{
    this._uiSourceCode = uiSourceCode;
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyChanged, this._workingCopyChanged, this);
    this._uiSourceCode.addEventListener(WebInspector.UISourceCode.Events.WorkingCopyCommitted, this._workingCopyCommitted, this);
}

WebInspector.StyleFile.updateTimeout = 200;

WebInspector.StyleFile.prototype = {
    _workingCopyCommitted: function(event)
    {
        if (this._isAddingRevision)
            return;

        this._commitIncrementalEdit(true);
    },

    _workingCopyChanged: function(event)
    {
        if (this._isAddingRevision)
            return;

        // FIXME: Extensions tests override updateTimeout because extensions don't have any control over applying changes to domain specific bindings.
        if (WebInspector.StyleFile.updateTimeout >= 0) {
            this._incrementalUpdateTimer = setTimeout(this._commitIncrementalEdit.bind(this, false), WebInspector.StyleFile.updateTimeout)
        } else
            this._commitIncrementalEdit(false);
    },

    /**
     * @param {boolean} majorChange
     */
    _commitIncrementalEdit: function(majorChange)
    {
        this._clearIncrementalUpdateTimer();
        WebInspector.styleContentBinding.setStyleContent(this._uiSourceCode, this._uiSourceCode.workingCopy(), majorChange, this._styleContentSet.bind(this));
    },

    /**
     * @param {?string} error
     */
    _styleContentSet: function(error)
    {
        if (error)
            WebInspector.showErrorMessage(error);
    },

    _clearIncrementalUpdateTimer: function()
    {
        if (!this._incrementalUpdateTimer)
            return;
        clearTimeout(this._incrementalUpdateTimer);
        delete this._incrementalUpdateTimer;
    },

    /**
     * @param {string} content
     */
    addRevision: function(content)
    {
        this._isAddingRevision = true;
        this._uiSourceCode.addRevision(content);
        delete this._isAddingRevision;
    },
}


/**
 * @constructor
 * @param {WebInspector.CSSStyleModel} cssModel
 */
WebInspector.StyleContentBinding = function(cssModel)
{
    this._cssModel = cssModel;
    this._cssModel.addEventListener(WebInspector.CSSStyleModel.Events.StyleSheetChanged, this._styleSheetChanged, this);
}

WebInspector.StyleContentBinding.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} content
     * @param {boolean} majorChange
     * @param {function(?string)} userCallback
     */
    setStyleContent: function(uiSourceCode, content, majorChange, userCallback)
    {
        var resource = WebInspector.resourceForURL(uiSourceCode.url);
        if (!resource) {
            userCallback("No resource found: " + uiSourceCode.url);
            return;
        }
            
        this._cssModel.resourceBinding().requestStyleSheetIdForResource(resource, callback.bind(this));

        /**
         * @param {?CSSAgent.StyleSheetId} styleSheetId
         */
        function callback(styleSheetId)
        {
            if (!styleSheetId) {
                userCallback("No stylesheet found: " + resource.frameId + ":" + resource.url);
                return;
            }

            this._innerSetContent(styleSheetId, content, majorChange, userCallback, null);
        }
    },

    /**
     * @param {CSSAgent.StyleSheetId} styleSheetId
     * @param {string} content
     * @param {boolean} majorChange
     * @param {function(?string)} userCallback
     */
    _innerSetContent: function(styleSheetId, content, majorChange, userCallback)
    {
        this._isSettingContent = true;
        function callback(error)
        {
            userCallback(error);
            delete this._isSettingContent;
        }
        this._cssModel.setStyleSheetText(styleSheetId, content, majorChange, callback.bind(this));
    },

    /**
     * @param {WebInspector.Event} event
     */
    _styleSheetChanged: function(event)
    {
        if (this._isSettingContent)
            return;

        if (!event.data.majorChange)
            return;

        /**
         * @param {?string} error
         * @param {string} content
         */
        function callback(error, content)
        {
            if (!error)
                this._innerStyleSheetChanged(event.data.styleSheetId, content);
        }
        CSSAgent.getStyleSheetText(event.data.styleSheetId, callback.bind(this));
    },

    /**
     * @param {CSSAgent.StyleSheetId} styleSheetId
     * @param {string} content
     */
    _innerStyleSheetChanged: function(styleSheetId, content)
    {
        /**
         * @param {?string} styleSheetURL
         */
        function callback(styleSheetURL)
        {
            if (typeof styleSheetURL !== "string")
                return;

            var uiSourceCode = WebInspector.workspace.uiSourceCodeForURL(styleSheetURL);
            if (!uiSourceCode)
                return;

            if (uiSourceCode.styleFile())
                uiSourceCode.styleFile().addRevision(content);
        }

        this._cssModel.resourceBinding().requestResourceURLForStyleSheetId(styleSheetId, callback.bind(this));
    },
}

/**
 * @type {?WebInspector.StyleContentBinding}
 */
WebInspector.styleContentBinding = null;
