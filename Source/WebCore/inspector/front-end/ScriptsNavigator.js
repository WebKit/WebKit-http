/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @extends {WebInspector.Object}
 * @constructor
 */
WebInspector.ScriptsNavigator = function()
{
    WebInspector.Object.call(this);
    
    this._tabbedPane = new WebInspector.TabbedPane();
    this._tabbedPane.shrinkableTabs = true;
    this._tabbedPane.element.addStyleClass("navigator-tabbed-pane");

    this._scriptsView = new WebInspector.NavigatorView();
    this._scriptsView.addEventListener(WebInspector.NavigatorView.Events.ItemSelected, this._scriptSelected, this);

    this._contentScriptsView = new WebInspector.NavigatorView();
    this._contentScriptsView.addEventListener(WebInspector.NavigatorView.Events.ItemSelected, this._scriptSelected, this);

    this._snippetsView = new WebInspector.SnippetsNavigatorView();
    this._snippetsView.addEventListener(WebInspector.NavigatorView.Events.ItemSelected, this._scriptSelected, this);

    this._tabbedPane.appendTab(WebInspector.ScriptsNavigator.ScriptsTab, WebInspector.UIString("Scripts"), this._scriptsView);
    this._tabbedPane.selectTab(WebInspector.ScriptsNavigator.ScriptsTab);
    this._tabbedPane.appendTab(WebInspector.ScriptsNavigator.ContentScriptsTab, WebInspector.UIString("Content scripts"), this._contentScriptsView);
    if (WebInspector.experimentsSettings.snippetsSupport.isEnabled())
        this._tabbedPane.appendTab(WebInspector.ScriptsNavigator.SnippetsTab, WebInspector.UIString("Snippets"), this._snippetsView);
}

WebInspector.ScriptsNavigator.Events = {
    ScriptSelected: "ScriptSelected"
}

WebInspector.ScriptsNavigator.ScriptsTab = "scripts";
WebInspector.ScriptsNavigator.ContentScriptsTab = "contentScripts";
WebInspector.ScriptsNavigator.SnippetsTab = "snippets";

WebInspector.ScriptsNavigator.prototype = {
    /*
     * @return {WebInspector.View}
     */
    get view()
    {
        return this._tabbedPane;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    addUISourceCode: function(uiSourceCode)
    {
        if (uiSourceCode.isContentScript)
            this._contentScriptsView.addUISourceCode(uiSourceCode);
        else if (uiSourceCode.isSnippet || uiSourceCode.isSnippetEvaluation)
            this._snippetsView.addUISourceCode(uiSourceCode);
        else
            this._scriptsView.addUISourceCode(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {boolean}
     */
    isScriptSourceAdded: function(uiSourceCode)
    {
        if (uiSourceCode.isContentScript)
            return this._contentScriptsView.isScriptSourceAdded(uiSourceCode);
        if (uiSourceCode.isSnippet || uiSourceCode.isSnippetEvaluation)
            return this._snippetsView.isScriptSourceAdded(uiSourceCode);
        return this._scriptsView.isScriptSourceAdded(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    revealUISourceCode: function(uiSourceCode)
    {
        if (uiSourceCode.isContentScript) {
            this._contentScriptsView.revealUISourceCode(uiSourceCode);
            this._tabbedPane.selectTab(WebInspector.ScriptsNavigator.ContentScriptsTab);
        } else if (uiSourceCode.isSnippet || uiSourceCode.isSnippetEvaluation) {
            this._snippetsView.revealUISourceCode(uiSourceCode);
            this._tabbedPane.selectTab(WebInspector.ScriptsNavigator.SnippetsTab);
        } else {
            this._scriptsView.revealUISourceCode(uiSourceCode);
            this._tabbedPane.selectTab(WebInspector.ScriptsNavigator.ScriptsTab);
        }
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {boolean} isDirty
     */
    setScriptSourceIsDirty: function(uiSourceCode, isDirty)
    {
        // Do nothing.
    },

    /**
     * @param {Array.<WebInspector.UISourceCode>} oldUISourceCodeList
     * @param {Array.<WebInspector.UISourceCode>} uiSourceCodeList
     */
    replaceUISourceCodes: function(oldUISourceCodeList, uiSourceCodeList)
    {
        this._scriptsView.replaceUISourceCodes(oldUISourceCodeList, uiSourceCodeList);
        this._contentScriptsView.replaceUISourceCodes(oldUISourceCodeList, uiSourceCodeList);
        this._snippetsView.replaceUISourceCodes(oldUISourceCodeList, uiSourceCodeList);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _scriptSelected: function(event)
    {
        this.dispatchEventToListeners(WebInspector.ScriptsNavigator.Events.ScriptSelected, event.data);
    },

    reset: function()
    {
        this._scriptsView.reset();
        this._contentScriptsView.reset();
        this._snippetsView.reset();
    }
}

WebInspector.ScriptsNavigator.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @constructor
 * @extends {WebInspector.NavigatorView}
 */
WebInspector.SnippetsNavigatorView = function()
{
    WebInspector.NavigatorView.call(this);
    this.element.addEventListener("contextmenu", this.handleContextMenu.bind(this), false);
}

WebInspector.SnippetsNavigatorView.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    getOrCreateFolderTreeElement: function(uiSourceCode)
    {
        if (uiSourceCode.isSnippet)
            return this._scriptsTree;
        if (uiSourceCode.isSnippetEvaluation)
            return this._getOrCreateSnippetEvaluationsFolderTreeElement();
        return WebInspector.NavigatorView.prototype.getOrCreateFolderTreeElement.call(this, uiSourceCode);
    },

    _getOrCreateSnippetEvaluationsFolderTreeElement: function()
    {
        const snippetEvaluationsFolderIdentifier = "snippetEvaluationsFolder";
        var folderTreeElement = this._folderTreeElements[snippetEvaluationsFolderIdentifier];
        if (folderTreeElement)
            return folderTreeElement;
        return this.createFolderTreeElement(this._scriptsTree, snippetEvaluationsFolderIdentifier, "", WebInspector.UIString("Evaluated snippets"));
    },

    /**
     * @param {Event} event
     * @param {WebInspector.UISourceCode=} uiSourceCode
     */
    handleContextMenu: function(event, uiSourceCode)
    {
        var contextMenu = new WebInspector.ContextMenu();
        if (uiSourceCode) {
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Evaluate snippet" : "Evaluate Snippet"), this._handleEvaluateSnippet.bind(this, uiSourceCode));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Rename snippet" : "Rename Snippet"), this._handleRenameSnippet.bind(this, uiSourceCode));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Remove snippet" : "Remove Snippet"), this._handleRemoveSnippet.bind(this, uiSourceCode));
            contextMenu.appendSeparator();
        }
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Create snippet" : "Create Snippet"), this._handleCreateSnippet.bind(this));
        contextMenu.show(event);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {Event} event
     */
    _handleEvaluateSnippet: function(uiSourceCode, event)
    {
        // FIXME: To be implemented.
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {Event} event
     */
    _handleRenameSnippet: function(uiSourceCode, event)
    {
        this.rename(uiSourceCode);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {Event} event
     */
    _handleRemoveSnippet: function(uiSourceCode, event)
    {
        // FIXME: To be implemented.
    },

    /**
     * @param {Event} event
     */
    _handleCreateSnippet: function(event)
    {
        // FIXME: To be implemented.
    }
}

WebInspector.SnippetsNavigatorView.prototype.__proto__ = WebInspector.NavigatorView.prototype;
