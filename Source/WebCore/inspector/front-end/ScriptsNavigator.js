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
 * @extends {WebInspector.TabbedPane}
 * @constructor
 */
WebInspector.ScriptsNavigator = function(presentationModel)
{
    WebInspector.TabbedPane.call(this);

    this._presentationModel = presentationModel;

    this.element.id = "scripts-navigator-tabbed-pane";
  
    this._navigatorScriptsTreeElement = document.createElement("ol");
    var scriptsView = new WebInspector.View();
    scriptsView.element.addStyleClass("outline-disclosure");
    scriptsView.element.addStyleClass("navigator");
    scriptsView.element.appendChild(this._navigatorScriptsTreeElement);
    this._navigatorScriptsTree = new WebInspector.NavigatorTreeOutline(this._navigatorScriptsTreeElement);
    this.appendTab(WebInspector.ScriptsNavigator.ScriptsTab, WebInspector.UIString("Scripts"), scriptsView);
    this.selectTab(WebInspector.ScriptsNavigator.ScriptsTab);

    this._navigatorContentScriptsTreeElement = document.createElement("ol");
    var contentScriptsView = new WebInspector.View();
    contentScriptsView.element.addStyleClass("outline-disclosure");
    contentScriptsView.element.addStyleClass("navigator");
    contentScriptsView.element.appendChild(this._navigatorContentScriptsTreeElement);
    this._navigatorContentScriptsTree = new WebInspector.NavigatorTreeOutline(this._navigatorContentScriptsTreeElement);
    this.appendTab(WebInspector.ScriptsNavigator.ContentScriptsTab, WebInspector.UIString("Content scripts"), contentScriptsView);

    this._folderTreeElements = {};
    
    this._scriptTreeElementsByUISourceCode = new Map();
    
    WebInspector.settings.showScriptFolders.addChangeListener(this._showScriptFoldersSettingChanged.bind(this));
    
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerWasDisabled, this._reset, this);
    this._presentationModel.addEventListener(WebInspector.DebuggerPresentationModel.Events.DebuggerReset, this._reset, this);
}

WebInspector.ScriptsNavigator.ScriptsTab = "scripts";
WebInspector.ScriptsNavigator.ContentScriptsTab = "contentScripts";

WebInspector.ScriptsNavigator.Events = {
    ScriptSelected: "ScriptSelected"
}

WebInspector.ScriptsNavigator.prototype = {
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    addUISourceCode: function(uiSourceCode)
    {
        var scriptTitle = uiSourceCode.displayName ? uiSourceCode.displayName : WebInspector.UIString("(program)");
        var scriptTreeElement = new WebInspector.NavigatorScriptTreeElement(this, uiSourceCode, scriptTitle);
        this._scriptTreeElementsByUISourceCode.put(uiSourceCode, scriptTreeElement);
        
        var folderTreeElement = this._getOrCreateFolderTreeElement(uiSourceCode.isContentScript, uiSourceCode.domain, uiSourceCode.folderName);
        folderTreeElement.appendChild(scriptTreeElement);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    revealUISourceCode: function(uiSourceCode)
    {
        this._lastSelectedUISourceCode = uiSourceCode;
        this.selectTab(uiSourceCode.isContentScript ? WebInspector.ScriptsNavigator.ContentScriptsTab : WebInspector.ScriptsNavigator.ScriptsTab);
        this._scriptTreeElementsByUISourceCode.get(uiSourceCode).revealAndSelect();
    },
    
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    scriptSelected: function(uiSourceCode)
    {
        this._lastSelectedUISourceCode = uiSourceCode;
        this.dispatchEventToListeners(WebInspector.ScriptsNavigator.Events.ScriptSelected, uiSourceCode);
    },
    
    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     */
    removeUISourceCode: function(uiSourceCode)
    {
        var treeElement = this._scriptTreeElementsByUISourceCode.get(uiSourceCode);
        while (treeElement) {
            var parent = treeElement.parent;
            if (parent) {
                if (treeElement instanceof WebInspector.NavigatorFolderTreeElement)
                    delete this._folderTreeElements[treeElement.folderIdentifier];
                parent.removeChild(treeElement);
                if (parent.children.length)
                    break;
            }
            treeElement = parent;
        }
    },
    
    _showScriptFoldersSettingChanged: function()
    {
        var uiSourceCodes = this._navigatorScriptsTree.scriptTreeElements();
        uiSourceCodes = uiSourceCodes.concat(this._navigatorContentScriptsTree.scriptTreeElements());
        this._reset();
        for (var i = 0; i < uiSourceCodes.length; ++i)
            this.addUISourceCode(uiSourceCodes[i]);
        
        this.revealUISourceCode(this._lastSelectedUISourceCode);
    },
    
    _reset: function()
    {
        this._navigatorScriptsTree.removeChildren();
        this._navigatorContentScriptsTree.removeChildren();
        this._folderTreeElements = {};
    },

    /**
     * @param {boolean} isContentScript
     * @param {string} domain
     * @param {string} folderName
     */
    _folderIdentifier: function(isContentScript, domain, folderName)
    {
        var contentScriptPrefix = isContentScript ? "0" : "1";
        return contentScriptPrefix + ":" + domain + folderName;
    },
    
    /**
     * @param {boolean} isContentScript
     * @param {string} domain
     * @param {string} folderName
     */
    _getOrCreateFolderTreeElement: function(isContentScript, domain, folderName)
    {
        var folderIdentifier = this._folderIdentifier(isContentScript, domain, folderName);
        
        if (this._folderTreeElements[folderIdentifier])
            return this._folderTreeElements[folderIdentifier];

        var showScriptFolders = WebInspector.settings.showScriptFolders.get();
        
        if ((domain === "" && folderName === "") || !showScriptFolders)
            return isContentScript ? this._navigatorContentScriptsTree : this._navigatorScriptsTree;
        
        var folderTreeElement = new WebInspector.NavigatorFolderTreeElement(folderIdentifier, domain, folderName);
        
        var parentFolderElement;
        if (folderName === "")
            parentFolderElement = isContentScript ? this._navigatorContentScriptsTree : this._navigatorScriptsTree;
        else
            parentFolderElement = this._getOrCreateFolderTreeElement(isContentScript, domain, "");
        
        parentFolderElement.appendChild(folderTreeElement);
        
        this._folderTreeElements[folderIdentifier] = folderTreeElement;
        return folderTreeElement;
    }
}

WebInspector.ScriptsNavigator.prototype.__proto__ = WebInspector.TabbedPane.prototype;

/**
 * @constructor
 * @extends {TreeOutline}
 * @param {Element} element
 */
WebInspector.NavigatorTreeOutline = function(element)
{
    TreeOutline.call(this, element);
    this.comparator = WebInspector.NavigatorTreeOutline._treeElementsCompare;
}

WebInspector.NavigatorTreeOutline._treeElementsCompare = function compare(treeElement1, treeElement2)
{
    // Insert in the alphabetical order, first domains, then folders, then scripts.
    function typeWeight(treeElement)
    {
        if (treeElement instanceof WebInspector.NavigatorFolderTreeElement) {
            if (treeElement.isDomain)
                return 1;
            return 2;
        }
        return 3;
    }

    var typeWeight1 = typeWeight(treeElement1);
    var typeWeight2 = typeWeight(treeElement2);

    var result;
    if (typeWeight1 > typeWeight2)
        result = 1;
    else if (typeWeight1 < typeWeight2)
        result = -1;
    else {
        var title1 = treeElement1.titleText;
        var title2 = treeElement2.titleText;
        result = title1.localeCompare(title2);
    }
    return result;
}

WebInspector.NavigatorTreeOutline.prototype = {
   /**
    * @return {Array.<TreeElement>}
    */
   scriptTreeElements: function()
   {
       var result = [];
       if (this.children.length) {
           for (var treeElement = this.children[0]; treeElement; treeElement = treeElement.traverseNextTreeElement(false, this, true)) {
               if (treeElement instanceof WebInspector.NavigatorScriptTreeElement)
                   result.push(treeElement.uiSourceCode);
           }
       }
       return result;
   }
}

WebInspector.NavigatorTreeOutline.prototype.__proto__ = TreeOutline.prototype;

/**
 * @constructor
 * @extends {TreeElement}
 * @param {string} title
 * @param {Array.<string>} iconClasses
 * @param {boolean} hasChildren
 * @param {boolean=} noIcon
 */
WebInspector.BaseNavigatorTreeElement = function(title, iconClasses, hasChildren, noIcon)
{
    TreeElement.call(this, "", null, hasChildren);
    this._titleText = title;
    this._iconClasses = iconClasses;
    this._noIcon = noIcon;
}

WebInspector.BaseNavigatorTreeElement.prototype = {
    onattach: function()
    {
        this.listItemElement.removeChildren();
        if (this._iconClasses) {
            for (var i = 0; i < this._iconClasses.length; ++i)
                this.listItemElement.addStyleClass(this._iconClasses[i]);
        }

        var selectionElement = document.createElement("div");
        selectionElement.className = "selection";
        this.listItemElement.appendChild(selectionElement);

        if (!this._noIcon) {
            this.imageElement = document.createElement("img");
            this.imageElement.className = "icon";
            this.listItemElement.appendChild(this.imageElement);
        }
        
        this.titleElement = document.createElement("div");
        this.titleElement.className = "base-navigator-tree-element-title";
        this._titleTextNode = document.createTextNode("");
        this._titleTextNode.textContent = this._titleText;
        this.titleElement.appendChild(this._titleTextNode);
        this.listItemElement.appendChild(this.titleElement);
        
        this.expand();
    },

    onreveal: function()
    {
        if (this.listItemElement)
            this.listItemElement.scrollIntoViewIfNeeded(false);
    },

    /**
     * @type {string}
     */
    get titleText()
    {
        return this._titleText;
    },

    set titleText(titleText)
    {
        this._titleText = titleText || "";
        this._titleTextNode.textContent = this._titleText;
    }
}

WebInspector.BaseNavigatorTreeElement.prototype.__proto__ = TreeElement.prototype;

/**
 * @constructor
 * @extends {WebInspector.BaseNavigatorTreeElement}
 * @param {string} folderIdentifier
 * @param {string} domain
 * @param {string} folderName
 */
WebInspector.NavigatorFolderTreeElement = function(folderIdentifier, domain, folderName)
{
    this._folderIdentifier = folderIdentifier;
    this._folderName = folderName;
    
    var iconClass = this.isDomain ? "scripts-navigator-domain-tree-item" : "scripts-navigator-folder-tree-item";
    var title = this.isDomain ? domain : folderName.substring(1);
    WebInspector.BaseNavigatorTreeElement.call(this, title, [iconClass], true);
}

WebInspector.NavigatorFolderTreeElement.prototype = {
    /**
     * @type {string}
     */
    get folderIdentifier()
    {
        return this._folderIdentifier;
    },

    /**
     * @type {boolean}
     */
    get isDomain()
    {
        return this._folderName === "";
    },
    
    onattach: function()
    {
        WebInspector.BaseNavigatorTreeElement.prototype.onattach.call(this);
        if (this._isDomain)
            this.collapse();
        else
            this.expand();
    }
}

WebInspector.NavigatorFolderTreeElement.prototype.__proto__ = WebInspector.BaseNavigatorTreeElement.prototype;

/**
 * @constructor
 * @extends {WebInspector.BaseNavigatorTreeElement}
 * @param {WebInspector.ScriptsNavigator} navigator
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {string} title
 */
WebInspector.NavigatorScriptTreeElement = function(navigator, uiSourceCode, title)
{
    WebInspector.BaseNavigatorTreeElement.call(this, title, ["scripts-navigator-script-tree-item"], false);
    this._navigator = navigator;
    this._uiSourceCode = uiSourceCode;
    this.tooltip = uiSourceCode.url;
}

WebInspector.NavigatorScriptTreeElement.prototype = {
    /**
     * @type {WebInspector.UISourceCode}
     */
    get uiSourceCode()
    {
        return this._uiSourceCode;
    },

    /**
     * @param {TreeElement} treeElement
     * @param {boolean=} selectedByUser
     */
    onselect: function(treeElement, selectedByUser)
    {
        if (selectedByUser)
            this._navigator.scriptSelected(this.uiSourceCode);
    }
}

WebInspector.NavigatorScriptTreeElement.prototype.__proto__ = WebInspector.BaseNavigatorTreeElement.prototype;
