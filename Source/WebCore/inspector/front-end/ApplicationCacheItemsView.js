/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

WebInspector.ApplicationCacheItemsView = function(treeElement, appcacheDomain)
{
    WebInspector.View.call(this);

    this.element.addStyleClass("storage-view");
    this.element.addStyleClass("table");

    // FIXME: Needs better tooltip. (Localized)
    this.deleteButton = new WebInspector.StatusBarButton(WebInspector.UIString("Delete"), "delete-storage-status-bar-item");
    this.deleteButton.visible = false;
    this.deleteButton.addEventListener("click", this._deleteButtonClicked.bind(this), false);

    // FIXME: Needs better tooltip. (Localized)
    this.refreshButton = new WebInspector.StatusBarButton(WebInspector.UIString("Refresh"), "refresh-storage-status-bar-item");
    this.refreshButton.addEventListener("click", this._refreshButtonClicked.bind(this), false);

    if (Preferences.onlineDetectionEnabled) {
        this.connectivityIcon = document.createElement("img");
        this.connectivityIcon.className = "storage-application-cache-connectivity-icon";
        this.connectivityIcon.src = "";
        this.connectivityMessage = document.createElement("span");
        this.connectivityMessage.className = "storage-application-cache-connectivity";
        this.connectivityMessage.textContent = "";
    }

    this.divider = document.createElement("span");
    this.divider.className = "status-bar-item status-bar-divider";

    this.statusIcon = document.createElement("img");
    this.statusIcon.className = "storage-application-cache-status-icon";
    this.statusIcon.src = "";
    this.statusMessage = document.createElement("span");
    this.statusMessage.className = "storage-application-cache-status";
    this.statusMessage.textContent = "";

    this._treeElement = treeElement;
    this._appcacheDomain = appcacheDomain;

    this._emptyView = new WebInspector.EmptyView(WebInspector.UIString("No Application Cache information available."));
    this._emptyView.show(this.element);

    this.updateStatus(applicationCache.UNCACHED);

    // FIXME: Status bar items don't work well enough yet, so they are being hidden.
    // http://webkit.org/b/41637 Web Inspector: Give Semantics to "Refresh" and "Delete" Buttons in ApplicationCache DataGrid
    // http://webkit.org/b/60590 Application cache status always displayed as UNCACHED at first
    // http://webkit.org/b/60793 Application cache status indicator gets stuck at DOWNLOADING after a failure
    this.deleteButton.element.style.display = "none";
    this.refreshButton.element.style.display = "none";
    if (Preferences.onlineDetectionEnabled) {
        this.connectivityIcon.style.display = "none";
        this.connectivityMessage.style.display = "none";
    }
    this.divider.style.display = "none";
    this.statusIcon.style.display = "none";
    this.statusMessage.style.display = "none";
    
}

WebInspector.ApplicationCacheItemsView.prototype = {
    get statusBarItems()
    {
        if (Preferences.onlineDetectionEnabled) {
            return [
                this.refreshButton.element, this.deleteButton.element,
                this.connectivityIcon, this.connectivityMessage, this.divider,
                this.statusIcon, this.statusMessage
            ];
        } else {
            return [
                this.refreshButton.element, this.deleteButton.element, this.divider,
                this.statusIcon, this.statusMessage
            ];
        }
    },

    show: function(parentElement)
    {
        WebInspector.View.prototype.show.call(this, parentElement);
        this.updateNetworkState(navigator.onLine);
        this._update();
    },

    hide: function()
    {
        WebInspector.View.prototype.hide.call(this);
        this.deleteButton.visible = false;
    },

    updateStatus: function(status)
    {
        var statusInformation = {};
        statusInformation[applicationCache.UNCACHED]    = { src: "Images/warningOrangeDot.png", text: "UNCACHED"    };
        statusInformation[applicationCache.IDLE]        = { src: "Images/warningOrangeDot.png", text: "IDLE"        };
        statusInformation[applicationCache.CHECKING]    = { src: "Images/successGreenDot.png",  text: "CHECKING"    };
        statusInformation[applicationCache.DOWNLOADING] = { src: "Images/successGreenDot.png",  text: "DOWNLOADING" };
        statusInformation[applicationCache.UPDATEREADY] = { src: "Images/successGreenDot.png",  text: "UPDATEREADY" };
        statusInformation[applicationCache.OBSOLETE]    = { src: "Images/errorRedDot.png",      text: "OBSOLETE"    };

        var info = statusInformation[status];
        if (!info) {
            console.error("Unknown Application Cache Status was Not Handled: %d", status);
            return;
        }

        this.statusIcon.src = info.src;
        this.statusMessage.textContent = info.text;
    },

    updateNetworkState: function(isNowOnline)
    {
        if (Preferences.onlineDetectionEnabled) {
            if (isNowOnline) {
                this.connectivityIcon.src = "Images/successGreenDot.png";
                this.connectivityMessage.textContent = WebInspector.UIString("Online");
            } else {
                this.connectivityIcon.src = "Images/errorRedDot.png";
                this.connectivityMessage.textContent = WebInspector.UIString("Offline");
            }
        }
    },

    _update: function()
    {
        WebInspector.ApplicationCacheDispatcher.getApplicationCachesAsync(this._updateCallback.bind(this));
    },

    _updateCallback: function(applicationCaches)
    {
        // FIXME: applicationCaches is just one cache.
        // FIXME: are these variables needed anywhere else?
        this._manifest = applicationCaches.manifest;
        this._creationTime = applicationCaches.creationTime;
        this._updateTime = applicationCaches.updateTime;
        this._size = applicationCaches.size;
        this._resources = applicationCaches.resources;
        var lastPathComponent = applicationCaches.lastPathComponent;

        if (!this._manifest) {
            this._emptyView.show(this.element);
            this.deleteButton.visible = false;
            if (this._dataGrid)
                this._dataGrid.element.addStyleClass("hidden");
            return;
        }

        if (!this._dataGrid)
            this._createDataGrid();

        this._populateDataGrid();
        this._dataGrid.autoSizeColumns(20, 80);
        this._dataGrid.element.removeStyleClass("hidden");
        this._emptyView.hide();
        this.deleteButton.visible = true;

        var totalSizeString = Number.bytesToString(this._size);
        this._treeElement.subtitle = WebInspector.UIString("%s (%s)", lastPathComponent, totalSizeString);

        // FIXME: For Chrome, put creationTime and updateTime somewhere.
        // NOTE: localizedString has not yet been added.
        // WebInspector.UIString("(%s) Created: %s Updated: %s", this._size, this._creationTime, this._updateTime);
    },

    _createDataGrid: function()
    {
        var columns = { 0: {}, 1: {}, 2: {} };
        columns[0].title = WebInspector.UIString("Resource");
        columns[0].sort = "ascending";
        columns[0].sortable = true;
        columns[1].title = WebInspector.UIString("Type");
        columns[1].sortable = true;
        columns[2].title = WebInspector.UIString("Size");
        columns[2].aligned = "right";
        columns[2].sortable = true;
        this._dataGrid = new WebInspector.DataGrid(columns);
        this.element.appendChild(this._dataGrid.element);
        this._dataGrid.addEventListener("sorting changed", this._populateDataGrid, this);
        this._dataGrid.updateWidths();
    },

    _populateDataGrid: function()
    {
        var selectedResource = this._dataGrid.selectedNode ? this._dataGrid.selectedNode.resource : null;
        var sortDirection = this._dataGrid.sortOrder === "ascending" ? 1 : -1;

        function numberCompare(field, resource1, resource2)
        {
            return sortDirection * (resource1[field] - resource2[field]);
        }
        function localeCompare(field, resource1, resource2)
        {
             return sortDirection * (resource1[field] + "").localeCompare(resource2[field] + "")
        }

        var comparator;
        switch (parseInt(this._dataGrid.sortColumnIdentifier)) {
            case 0: comparator = localeCompare.bind(this, "name"); break;
            case 1: comparator = localeCompare.bind(this, "type"); break;
            case 2: comparator = numberCompare.bind(this, "size"); break;
            default: localeCompare.bind(this, "resource"); // FIXME: comparator = ?
        }

        this._resources.sort(comparator);
        this._dataGrid.removeChildren();

        var nodeToSelect;
        for (var i = 0; i < this._resources.length; ++i) {
            var data = {};
            var resource = this._resources[i];
            data[0] = resource.name;
            data[1] = resource.type;
            data[2] = Number.bytesToString(resource.size);
            var node = new WebInspector.DataGridNode(data);
            node.resource = resource;
            node.selectable = true;
            this._dataGrid.appendChild(node);
            if (resource === selectedResource) {
                nodeToSelect = node;
                nodeToSelect.selected = true;
            }
        }

        if (!nodeToSelect)
            this._dataGrid.children[0].selected = true;
    },

    resize: function()
    {
        if (this._dataGrid)
            this._dataGrid.updateWidths();
    },

    _deleteButtonClicked: function(event)
    {
        if (!this._dataGrid || !this._dataGrid.selectedNode)
            return;

        // FIXME: Delete Button semantics are not yet defined. (Delete a single, or all?)
        this._deleteCallback(this._dataGrid.selectedNode);
    },

    _deleteCallback: function(node)
    {
        // FIXME: Should we delete a single (selected) resource or all resources?
        // InspectorBackend.deleteCachedResource(...)
        // this._update();
    },

    _refreshButtonClicked: function(event)
    {
        // FIXME: Is this a refresh button or a re-fetch manifest button?
        // this._update();
    }
}

WebInspector.ApplicationCacheItemsView.prototype.__proto__ = WebInspector.View.prototype;

WebInspector.ApplicationCacheDispatcher = function()
{
}

WebInspector.ApplicationCacheDispatcher.getApplicationCachesAsync = function(callback)
{
    function mycallback(error, applicationCaches)
    {
        // FIXME: Currently, this list only returns a single application cache.
        if (!error && applicationCaches)
            callback(applicationCaches);
    }

    ApplicationCacheAgent.getApplicationCaches(mycallback);
}

WebInspector.ApplicationCacheDispatcher.prototype = {
    updateApplicationCacheStatus: function(status)
    {
        WebInspector.panels.resources.updateApplicationCacheStatus(status);
    },

    updateNetworkState: function(isNowOnline)
    {
        WebInspector.panels.resources.updateNetworkState(isNowOnline);
    }
}

InspectorBackend.registerDomainDispatcher("ApplicationCache", new WebInspector.ApplicationCacheDispatcher());
