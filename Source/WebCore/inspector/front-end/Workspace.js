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
 */
WebInspector.WorkspaceController = function(workspace)
{
    this._workspace = workspace;
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, this._mainFrameNavigated, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.FrameAdded, this._frameAdded, this);
}

WebInspector.WorkspaceController.prototype = {
    _mainFrameNavigated: function()
    {
        WebInspector.Revision.filterOutStaleRevisions();
        this._workspace.dispatchEventToListeners(WebInspector.Workspace.Events.ProjectWillReset, this._workspace.project());
        this._workspace.project().reset();
        this._workspace.reset();
        this._workspace.dispatchEventToListeners(WebInspector.Workspace.Events.ProjectDidReset, this._workspace.project());
    },

    _frameAdded: function(event)
    {
        var frame = /** @type {WebInspector.ResourceTreeFrame} */ event.data;
        if (frame.isMainFrame())
            WebInspector.Revision.filterOutStaleRevisions();
    }
}

/**
 * @constructor
 * @param {string} path
 * @param {WebInspector.ResourceType} contentType
 * @param {boolean} isEditable
 * @param {boolean=} isContentScript
 * @param {boolean=} isSnippet
 */
WebInspector.FileDescriptor = function(path, contentType, isEditable, isContentScript, isSnippet)
{
    this.path = path;
    this.contentType = contentType;
    this.isEditable = isEditable;
    this.isContentScript = isContentScript || false;
    this.isSnippet = isSnippet || false;
}

/**
 * @interface
 */
WebInspector.WorkspaceProvider = function() { }

WebInspector.WorkspaceProvider.Events = {
    FileAdded: "FileAdded",
    FileRemoved: "FileRemoved"
}

WebInspector.WorkspaceProvider.prototype = {
    /**
     * @param {string} path
     * @param {function(?string,boolean,string)} callback
     */
    requestFileContent: function(path, callback) { },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInFileContent: function(path, query, caseSensitive, isRegex, callback) { },

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
 * @type {?WebInspector.WorkspaceController}
 */
WebInspector.workspaceController = null;

/**
 * @param {WebInspector.Workspace} workspace
 * @param {WebInspector.WorkspaceProvider} workspaceProvider
 * @constructor
 */
WebInspector.Project = function(workspace, workspaceProvider)
{
    this._uiSourceCodes = [];
    this._workspace = workspace;
    this._workspaceProvider = workspaceProvider;
    this._workspaceProvider.addEventListener(WebInspector.WorkspaceProvider.Events.FileAdded, this._fileAdded, this);
    this._workspaceProvider.addEventListener(WebInspector.WorkspaceProvider.Events.FileRemoved, this._fileRemoved, this);
}

WebInspector.Project.prototype = {
    reset: function()
    {
        this._workspaceProvider.reset();
        this._uiSourceCodes = [];
    },

    _fileAdded: function(event)
    {
        var fileDescriptor = /** @type {WebInspector.FileDescriptor} */ event.data;
        var uiSourceCode = this.uiSourceCodeForURL(fileDescriptor.path);
        if (uiSourceCode) {
            // FIXME: Implement
            return;
        }
        uiSourceCode = new WebInspector.UISourceCode(this._workspace, fileDescriptor.path, fileDescriptor.contentType, fileDescriptor.isEditable);
        uiSourceCode.isContentScript = fileDescriptor.isContentScript;
        uiSourceCode.isSnippet = fileDescriptor.isSnippet;
        this._uiSourceCodes.push(uiSourceCode);
        this._workspace.dispatchEventToListeners(WebInspector.UISourceCodeProvider.Events.UISourceCodeAdded, uiSourceCode);
    },

    _fileRemoved: function(event)
    {
        var path = /** @type {string} */ event.data;
        var uiSourceCode = this.uiSourceCodeForURL(path);
        if (!uiSourceCode)
            return;
        this._uiSourceCodes.splice(this._uiSourceCodes.indexOf(uiSourceCode), 1);
        this._workspace.dispatchEventToListeners(WebInspector.UISourceCodeProvider.Events.UISourceCodeRemoved, uiSourceCode);
    },

    /**
     * @param {string} url
     * @return {?WebInspector.UISourceCode}
     */
    uiSourceCodeForURL: function(url)
    {
        for (var i = 0; i < this._uiSourceCodes.length; ++i) {
            if (this._uiSourceCodes[i].url === url)
                return this._uiSourceCodes[i];
        }
        return null;
    },

    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function()
    {
        return this._uiSourceCodes;
    },

    /**
     * @param {string} path
     * @param {function(?string,boolean,string)} callback
     */
    requestFileContent: function(path, callback)
    {
        this._workspaceProvider.requestFileContent(path, callback);
    },

    /**
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInFileContent: function(path, query, caseSensitive, isRegex, callback)
    {
        this._workspaceProvider.searchInFileContent(path, query, caseSensitive, isRegex, callback);
    }
}

/**
 * @constructor
 * @implements {WebInspector.UISourceCodeProvider}
 * @extends {WebInspector.Object}
 */
WebInspector.Workspace = function()
{
    /** @type {Object.<string, WebInspector.ContentProvider>} */
    this._temporaryContentProviders = new Map();
}

WebInspector.Workspace.Events = {
    UISourceCodeContentCommitted: "UISourceCodeContentCommitted",
    ProjectWillReset: "ProjectWillReset",
    ProjectDidReset: "ProjectDidReset"
}

WebInspector.Workspace.prototype = {
    /**
     * @param {string} url
     * @return {?WebInspector.UISourceCode}
     */
    uiSourceCodeForURL: function(url)
    {
        return this._project.uiSourceCodeForURL(url);
    },

    /**
     * @param {string} projectName
     * @param {WebInspector.WorkspaceProvider} workspaceProvider
     */
    addProject: function(projectName, workspaceProvider)
    {
        // FIXME: support multiple projects identified by project name.
        this._project = new WebInspector.Project(this, workspaceProvider);
    },

    /**
     * @return {WebInspector.Project}
     */
    project: function()
    {
        return this._project;
    },

    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function()
    {
        return this._project.uiSourceCodes();
    },

    /**
     * @param {string} path
     * @param {WebInspector.ContentProvider} contentProvider
     * @param {boolean} isEditable
     * @param {boolean=} isContentScript
     * @param {boolean=} isSnippet
     */
    addTemporaryUISourceCode: function(path, contentProvider, isEditable, isContentScript, isSnippet)
    {
        var uiSourceCode = new WebInspector.UISourceCode(this, path, contentProvider.contentType(), isEditable);
        this._temporaryContentProviders.put(uiSourceCode, contentProvider);
        uiSourceCode.isContentScript = isContentScript;
        uiSourceCode.isSnippet = isSnippet;
        uiSourceCode.isTemporary = true;
        this.dispatchEventToListeners(WebInspector.UISourceCodeProvider.Events.TemporaryUISourceCodeAdded, uiSourceCode);
        return uiSourceCode;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    removeTemporaryUISourceCode: function(uiSourceCode)
    {
        this._temporaryContentProviders.remove(uiSourceCode.url);
        this.dispatchEventToListeners(WebInspector.UISourceCodeProvider.Events.TemporaryUISourceCodeRemoved, uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {function(?string,boolean,string)} callback
     */
    requestFileContent: function(uiSourceCode, callback)
    {
        if (this._temporaryContentProviders.get(uiSourceCode)) {
            this._temporaryContentProviders.get(uiSourceCode).requestContent(callback);
            return;
        }
        this._project.requestFileContent(uiSourceCode.url, callback);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} query
     * @param {boolean} caseSensitive
     * @param {boolean} isRegex
     * @param {function(Array.<WebInspector.ContentProvider.SearchMatch>)} callback
     */
    searchInFileContent: function(uiSourceCode, query, caseSensitive, isRegex, callback)
    {
        if (this._temporaryContentProviders.get(uiSourceCode)) {
            this._temporaryContentProviders.get(uiSourceCode).searchInContent(query, caseSensitive, isRegex, callback);
            return;
        }
        this._project.searchInFileContent(uiSourceCode.url, query, caseSensitive, isRegex, callback);
    },

    reset: function()
    {
        this._temporaryContentProviders = new Map();
    },

    __proto__: WebInspector.Object.prototype
}

/**
 * @type {?WebInspector.Workspace}
 */
WebInspector.workspace = null;
