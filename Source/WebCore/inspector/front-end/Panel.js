/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @extends {WebInspector.View}
 * @constructor
 */
WebInspector.Panel = function(name)
{
    WebInspector.View.call(this);

    this.element.addStyleClass("panel");
    this.element.addStyleClass(name);
    this._panelName = name;

    this._shortcuts = {};

    WebInspector.settings[this._sidebarWidthSettingName()] = WebInspector.settings.createSetting(this._sidebarWidthSettingName(), undefined);
}

// Should by in sync with style declarations.
WebInspector.Panel.counterRightMargin = 25;

WebInspector.Panel.prototype = {
    get toolbarItem()
    {
        if (this._toolbarItem)
            return this._toolbarItem;

        this._toolbarItem = WebInspector.Toolbar.createPanelToolbarItem(this);
        return this._toolbarItem;
    },

    get name()
    {
        return this._panelName;
    },

    show: function()
    {
        WebInspector.View.prototype.show.call(this);

        var statusBarItems = this.statusBarItems;
        if (statusBarItems) {
            this._statusBarItemContainer = document.createElement("div");
            for (var i = 0; i < statusBarItems.length; ++i)
                this._statusBarItemContainer.appendChild(statusBarItems[i]);
            document.getElementById("main-status-bar").appendChild(this._statusBarItemContainer);
        }

        if ("_toolbarItem" in this)
            this._toolbarItem.addStyleClass("toggled-on");

        WebInspector.currentFocusElement = this.defaultFocusedElement;

        this.restoreSidebarWidth();
        WebInspector.extensionServer.notifyPanelShown(this.name);
    },

    hide: function()
    {
        WebInspector.View.prototype.hide.call(this);

        if (this._statusBarItemContainer && this._statusBarItemContainer.parentNode)
            this._statusBarItemContainer.parentNode.removeChild(this._statusBarItemContainer);
        delete this._statusBarItemContainer;
        if ("_toolbarItem" in this)
            this._toolbarItem.removeStyleClass("toggled-on");
        WebInspector.extensionServer.notifyPanelHidden(this.name);
    },

    reset: function()
    {
        this.searchCanceled();
    },

    get defaultFocusedElement()
    {
        return this.sidebarTreeElement || this.element;
    },

    attach: function()
    {
        if (!this.element.parentNode)
            document.getElementById("main-panels").appendChild(this.element);
    },

    searchCanceled: function()
    {
        WebInspector.searchController.updateSearchMatchesCount(0, this);
    },

    performSearch: function(query)
    {
        // Call searchCanceled since it will reset everything we need before doing a new search.
        this.searchCanceled();
    },

    jumpToNextSearchResult: function()
    {
    },

    jumpToPreviousSearchResult: function()
    {
    },

    /**
     * @param {Element=} parentElement
     * @param {Element=} resizerParentElement
     */
    createSidebar: function(parentElement, resizerParentElement)
    {
        if (this.sidebarElement)
            return;

        if (!parentElement)
            parentElement = this.element;

        if (!resizerParentElement)
            resizerParentElement = parentElement;

        this.sidebarElement = document.createElement("div");
        this.sidebarElement.className = "sidebar";
        parentElement.appendChild(this.sidebarElement);

        this.sidebarResizeElement = document.createElement("div");
        this.sidebarResizeElement.className = "sidebar-resizer-vertical";
        this.sidebarResizeElement.addEventListener("mousedown", this._startSidebarDragging.bind(this), false);
        resizerParentElement.appendChild(this.sidebarResizeElement);

        this.sidebarTreeElement = document.createElement("ol");
        this.sidebarTreeElement.className = "sidebar-tree";
        this.sidebarElement.appendChild(this.sidebarTreeElement);

        this.sidebarTree = new TreeOutline(this.sidebarTreeElement);
        this.sidebarTree.panel = this;
    },

    _sidebarWidthSettingName: function()
    {
        return this._panelName + "SidebarWidth";
    },

    _startSidebarDragging: function(event)
    {
        WebInspector.elementDragStart(this.sidebarResizeElement, this._sidebarDragging.bind(this), this._endSidebarDragging.bind(this), event, "ew-resize");
    },

    _sidebarDragging: function(event)
    {
        this.updateSidebarWidth(event.pageX);

        event.preventDefault();
    },

    _endSidebarDragging: function(event)
    {
        WebInspector.elementDragEnd(event);
        this.saveSidebarWidth();
    },

    updateSidebarWidth: function(width)
    {
        if (!this.sidebarElement)
            return;

        if (this.sidebarElement.offsetWidth <= 0) {
            // The stylesheet hasn't loaded yet or the window is closed,
            // so we can't calculate what is need. Return early.
            return;
        }

        if (!("_currentSidebarWidth" in this))
            this._currentSidebarWidth = this.sidebarElement.offsetWidth;

        if (typeof width === "undefined")
            width = this._currentSidebarWidth;

        var maxWidth = window.innerWidth / 2;
        if (!maxWidth)
            maxWidth = width;
        width = Number.constrain(width, Preferences.minSidebarWidth, maxWidth);

        this._currentSidebarWidth = width;
        this.setSidebarWidth(width);
        this.updateMainViewWidth(width);
        this.doResize();
    },

    setSidebarWidth: function(width)
    {
        this.sidebarElement.style.width = width + "px";
        this.sidebarResizeElement.style.left = (width - 3) + "px";
    },

    preferredSidebarWidth: function()
    {
        return WebInspector.settings[this._sidebarWidthSettingName()].get();
    },

    restoreSidebarWidth: function()
    {
        this.updateSidebarWidth(this.preferredSidebarWidth());
    },

    saveSidebarWidth: function()
    {
        if (!this.sidebarElement)
            return;
        WebInspector.settings[this._sidebarWidthSettingName()].set(this.sidebarElement.offsetWidth);
    },

    // Should be implemented by ancestors.

    get toolbarItemLabel()
    {
    },

    get statusBarItems()
    {
    },

    updateMainViewWidth: function(width)
    {
    },

    statusBarResized: function()
    {
    },

    canShowAnchorLocation: function(anchor)
    {
        return false;
    },

    showAnchorLocation: function(anchor)
    {
        return false;
    },

    elementsToRestoreScrollPositionsFor: function()
    {
        return [];
    },

    handleShortcut: function(event)
    {
        var shortcutKey = WebInspector.KeyboardShortcut.makeKeyFromEvent(event);
        var handler = this._shortcuts[shortcutKey];
        if (handler) {
            handler(event);
            event.handled = true;
        }
    },

    registerShortcut: function(key, handler)
    {
        this._shortcuts[key] = handler;
    }
}

WebInspector.Panel.prototype.__proto__ = WebInspector.View.prototype;
