/*
 * Copyright (C) 2007, 2008 Apple Inc.  All rights reserved.
 * Copyright (C) 2008, 2009 Anthony Ricaud <rik@webkit.org>
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * @constructor
 * @extends {WebInspector.View}
 */
WebInspector.NetworkLogView = function()
{
    WebInspector.View.call(this);
    this.registerRequiredCSS("networkLogView.css");

    this._allowRequestSelection = false;
    this._requests = [];
    this._requestsById = {};
    this._requestsByURL = {};
    this._staleRequests = {};
    this._requestGridNodes = {};
    this._lastRequestGridNodeId = 0;
    this._mainRequestLoadTime = -1;
    this._mainRequestDOMContentTime = -1;
    this._hiddenCategories = {};
    this._matchedRequests = [];
    this._matchedRequestsMap = {};
    this._currentMatchedRequestIndex = -1;

    this._createStatusbarButtons();
    this._createFilterStatusBarItems();
    this._linkifier = new WebInspector.Linkifier();

    WebInspector.networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestStarted, this._onRequestStarted, this);
    WebInspector.networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestUpdated, this._onRequestUpdated, this);
    WebInspector.networkManager.addEventListener(WebInspector.NetworkManager.EventTypes.RequestFinished, this._onRequestUpdated, this);

    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.MainFrameNavigated, this._mainFrameNavigated, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.OnLoad, this._onLoadEventFired, this);
    WebInspector.resourceTreeModel.addEventListener(WebInspector.ResourceTreeModel.EventTypes.DOMContentLoaded, this._domContentLoadedEventFired, this);

    this._initializeView();
    function onCanClearBrowserCache(error, result)
    {
        this._canClearBrowserCache = result;
    }
    NetworkAgent.canClearBrowserCache(onCanClearBrowserCache.bind(this));

    function onCanClearBrowserCookies(error, result)
    {
        this._canClearBrowserCookies = result;
    }
    NetworkAgent.canClearBrowserCookies(onCanClearBrowserCookies.bind(this));
}

WebInspector.NetworkLogView.prototype = {
    _initializeView: function()
    {
        this.element.id = "network-container";

        this._createSortingFunctions();
        this._createTable();
        this._createTimelineGrid();
        this._createSummaryBar();

        if (!this.useLargeRows)
            this._setLargerRequests(this.useLargeRows);

        this._allowPopover = true;
        this._popoverHelper = new WebInspector.PopoverHelper(this.element, this._getPopoverAnchor.bind(this), this._showPopover.bind(this));
        // Enable faster hint.
        this._popoverHelper.setTimeout(100);

        this.calculator = new WebInspector.NetworkTransferTimeCalculator();
        this._filter(this._filterAllElement, false);

        this.switchToDetailedView();
    },

    get statusBarItems()
    {
        return [this._largerRequestsButton.element, this._preserveLogToggle.element, this._clearButton.element, this._filterBarElement];
    },

    get useLargeRows()
    {
        return WebInspector.settings.resourcesLargeRows.get();
    },

    set allowPopover(flag)
    {
        this._allowPopover = flag;
    },

    elementsToRestoreScrollPositionsFor: function()
    {
        if (!this._dataGrid) // Not initialized yet.
            return [];
        return [this._dataGrid.scrollContainer];
    },

    onResize: function()
    {
        this._updateOffscreenRows();
    },

    _createTimelineGrid: function()
    {
        this._timelineGrid = new WebInspector.TimelineGrid();
        this._timelineGrid.element.addStyleClass("network-timeline-grid");
        this._dataGrid.element.appendChild(this._timelineGrid.element);
    },

    _createTable: function()
    {
        var columns = {name: {}, method: {}, status: {}, type: {}, initiator: {}, size: {}, time: {}, timeline: {}};

        columns.name.titleDOMFragment = this._makeHeaderFragment(WebInspector.UIString("Name"), WebInspector.UIString("Path"));
        columns.name.sortable = true;
        columns.name.width = "20%";
        columns.name.disclosure = true;

        columns.method.title = WebInspector.UIString("Method");
        columns.method.sortable = true;
        columns.method.width = "6%";

        columns.status.titleDOMFragment = this._makeHeaderFragment(WebInspector.UIString("Status"), WebInspector.UIString("Text"));
        columns.status.sortable = true;
        columns.status.width = "6%";

        columns.type.title = WebInspector.UIString("Type");
        columns.type.sortable = true;
        columns.type.width = "6%";

        columns.initiator.title = WebInspector.UIString("Initiator");
        columns.initiator.sortable = true;
        columns.initiator.width = "10%";

        columns.size.titleDOMFragment = this._makeHeaderFragment(WebInspector.UIString("Size"), WebInspector.UIString("Content"));
        columns.size.sortable = true;
        columns.size.width = "6%";
        columns.size.aligned = "right";

        columns.time.titleDOMFragment = this._makeHeaderFragment(WebInspector.UIString("Time"), WebInspector.UIString("Latency"));
        columns.time.sortable = true;
        columns.time.width = "6%";
        columns.time.aligned = "right";

        columns.timeline.title = "";
        columns.timeline.sortable = false;
        columns.timeline.width = "40%";
        columns.timeline.sort = "ascending";

        this._dataGrid = new WebInspector.DataGrid(columns);
        this._dataGrid.resizeMethod = WebInspector.DataGrid.ResizeMethod.Last;
        this._dataGrid.element.addStyleClass("network-log-grid");
        this._dataGrid.element.addEventListener("contextmenu", this._contextMenu.bind(this), true);
        this._dataGrid.show(this.element);

        // Event listeners need to be added _after_ we attach to the document, so that owner document is properly update.
        this._dataGrid.addEventListener("sorting changed", this._sortItems, this);
        this._dataGrid.addEventListener("width changed", this._updateDividersIfNeeded, this);
        this._dataGrid.scrollContainer.addEventListener("scroll", this._updateOffscreenRows.bind(this));

        this._patchTimelineHeader();
    },

    _makeHeaderFragment: function(title, subtitle)
    {
        var fragment = document.createDocumentFragment();
        fragment.appendChild(document.createTextNode(title));
        var subtitleDiv = document.createElement("div");
        subtitleDiv.className = "network-header-subtitle";
        subtitleDiv.textContent = subtitle;
        fragment.appendChild(subtitleDiv);
        return fragment;
    },

    _patchTimelineHeader: function()
    {
        var timelineSorting = document.createElement("select");

        var option = document.createElement("option");
        option.value = "startTime";
        option.label = WebInspector.UIString("Timeline");
        timelineSorting.appendChild(option);

        option = document.createElement("option");
        option.value = "startTime";
        option.label = WebInspector.UIString("Start Time");
        timelineSorting.appendChild(option);

        option = document.createElement("option");
        option.value = "responseTime";
        option.label = WebInspector.UIString("Response Time");
        timelineSorting.appendChild(option);

        option = document.createElement("option");
        option.value = "endTime";
        option.label = WebInspector.UIString("End Time");
        timelineSorting.appendChild(option);

        option = document.createElement("option");
        option.value = "duration";
        option.label = WebInspector.UIString("Duration");
        timelineSorting.appendChild(option);

        option = document.createElement("option");
        option.value = "latency";
        option.label = WebInspector.UIString("Latency");
        timelineSorting.appendChild(option);

        var header = this._dataGrid.headerTableHeader("timeline");
        header.replaceChild(timelineSorting, header.firstChild);

        timelineSorting.addEventListener("click", function(event) { event.consume() }, false);
        timelineSorting.addEventListener("change", this._sortByTimeline.bind(this), false);
        this._timelineSortSelector = timelineSorting;
    },

    _createSortingFunctions: function()
    {
        this._sortingFunctions = {};
        this._sortingFunctions.name = WebInspector.NetworkDataGridNode.NameComparator;
        this._sortingFunctions.method = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "method", false);
        this._sortingFunctions.status = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "statusCode", false);
        this._sortingFunctions.type = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "mimeType", false);
        this._sortingFunctions.initiator = WebInspector.NetworkDataGridNode.InitiatorComparator;
        this._sortingFunctions.size = WebInspector.NetworkDataGridNode.SizeComparator;
        this._sortingFunctions.time = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "duration", false);
        this._sortingFunctions.timeline = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "startTime", false);
        this._sortingFunctions.startTime = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "startTime", false);
        this._sortingFunctions.endTime = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "endTime", false);
        this._sortingFunctions.responseTime = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "responseReceivedTime", false);
        this._sortingFunctions.duration = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "duration", true);
        this._sortingFunctions.latency = WebInspector.NetworkDataGridNode.RequestPropertyComparator.bind(null, "latency", true);

        var timeCalculator = new WebInspector.NetworkTransferTimeCalculator();
        var durationCalculator = new WebInspector.NetworkTransferDurationCalculator();

        this._calculators = {};
        this._calculators.timeline = timeCalculator;
        this._calculators.startTime = timeCalculator;
        this._calculators.endTime = timeCalculator;
        this._calculators.responseTime = timeCalculator;
        this._calculators.duration = durationCalculator;
        this._calculators.latency = durationCalculator;
    },

    _sortItems: function()
    {
        this._removeAllNodeHighlights();
        var columnIdentifier = this._dataGrid.sortColumnIdentifier;
        if (columnIdentifier === "timeline") {
            this._sortByTimeline();
            return;
        }
        var sortingFunction = this._sortingFunctions[columnIdentifier];
        if (!sortingFunction)
            return;

        this._dataGrid.sortNodes(sortingFunction, this._dataGrid.sortOrder === "descending");
        this._timelineSortSelector.selectedIndex = 0;
        this._updateOffscreenRows();

        this.performSearch(null);
    },

    _sortByTimeline: function()
    {
        this._removeAllNodeHighlights();
        var selectedIndex = this._timelineSortSelector.selectedIndex;
        if (!selectedIndex)
            selectedIndex = 1; // Sort by start time by default.
        var selectedOption = this._timelineSortSelector[selectedIndex];
        var value = selectedOption.value;

        var sortingFunction = this._sortingFunctions[value];
        this._dataGrid.sortNodes(sortingFunction);
        this.calculator = this._calculators[value];
        if (this.calculator.startAtZero)
            this._timelineGrid.hideEventDividers();
        else
            this._timelineGrid.showEventDividers();
        this._dataGrid.markColumnAsSortedBy("timeline", "ascending");
        this._updateOffscreenRows();
    },

    _createFilterStatusBarItems: function()
    {
        var filterBarElement = document.createElement("div");
        filterBarElement.className = "scope-bar status-bar-item";

        /**
         * @param {string} typeName
         * @param {string} label
         */
        function createFilterElement(typeName, label)
        {
            var categoryElement = document.createElement("li");
            categoryElement.typeName = typeName;
            categoryElement.className = typeName;
            categoryElement.appendChild(document.createTextNode(label));
            categoryElement.addEventListener("click", this._updateFilter.bind(this), false);
            filterBarElement.appendChild(categoryElement);

            return categoryElement;
        }

        this._filterAllElement = createFilterElement.call(this, "all", WebInspector.UIString("All"));

        // Add a divider
        var dividerElement = document.createElement("div");
        dividerElement.addStyleClass("scope-bar-divider");
        filterBarElement.appendChild(dividerElement);

        for (var typeId in WebInspector.resourceTypes) {
            var type = WebInspector.resourceTypes[typeId];
            createFilterElement.call(this, type.name(), type.categoryTitle());
        }
        this._filterBarElement = filterBarElement;
    },

    _createSummaryBar: function()
    {
        var tbody = this._dataGrid.dataTableBody;
        var tfoot = document.createElement("tfoot");
        var tr = tfoot.createChild("tr", "revealed network-summary-bar");
        var td = tr.createChild("td");
        td.setAttribute("colspan", 7);
        tbody.parentNode.insertBefore(tfoot, tbody);
        this._summaryBarElement = td;
    },

    _updateSummaryBar: function()
    {
        var requestsNumber = this._requests.length;

        if (!requestsNumber) {
            if (this._summaryBarElement._isDisplayingWarning)
                return;
            this._summaryBarElement._isDisplayingWarning = true;

            var img = document.createElement("img");
            img.src = "Images/warningIcon.png";
            this._summaryBarElement.removeChildren();
            this._summaryBarElement.appendChild(img);
            this._summaryBarElement.appendChild(document.createTextNode(
                WebInspector.UIString("No requests captured. Reload the page to see detailed information on the network activity.")));
            return;
        }
        delete this._summaryBarElement._isDisplayingWarning;

        var transferSize = 0;
        var selectedRequestsNumber = 0;
        var selectedTransferSize = 0;
        var baseTime = -1;
        var maxTime = -1;
        for (var i = 0; i < this._requests.length; ++i) {
            var request = this._requests[i];
            var requestTransferSize = (request.cached || !request.transferSize) ? 0 : request.transferSize;
            transferSize += requestTransferSize;
            if (!this._hiddenCategories.all || !this._hiddenCategories[request.type.name()]) {
                selectedRequestsNumber++;
                selectedTransferSize += requestTransferSize;
            }
            if (request.url === WebInspector.inspectedPageURL)
                baseTime = request.startTime;
            if (request.endTime > maxTime)
                maxTime = request.endTime;
        }
        var text = "";
        if (this._hiddenCategories.all) {
            text += String.sprintf(WebInspector.UIString("%d / %d requests"), selectedRequestsNumber, requestsNumber);
            text += "  \u2758  " + String.sprintf(WebInspector.UIString("%s / %s transferred"), Number.bytesToString(selectedTransferSize), Number.bytesToString(transferSize));
        } else {
            text += String.sprintf(WebInspector.UIString("%d requests"), requestsNumber);
            text += "  \u2758  " + String.sprintf(WebInspector.UIString("%s transferred"), Number.bytesToString(transferSize));
        }
        if (baseTime !== -1 && this._mainRequestLoadTime !== -1 && this._mainRequestDOMContentTime !== -1 && this._mainRequestDOMContentTime > baseTime) {
            text += "  \u2758  " + String.sprintf(WebInspector.UIString("%s (onload: %s, DOMContentLoaded: %s)"),
                        Number.secondsToString(maxTime - baseTime),
                        Number.secondsToString(this._mainRequestLoadTime - baseTime),
                        Number.secondsToString(this._mainRequestDOMContentTime - baseTime));
        }
        this._summaryBarElement.textContent = text;
    },

    _showCategory: function(typeName)
    {
        this._dataGrid.element.addStyleClass("filter-" + typeName);
        delete this._hiddenCategories[typeName];
    },

    _hideCategory: function(typeName)
    {
        this._dataGrid.element.removeStyleClass("filter-" + typeName);
        this._hiddenCategories[typeName] = true;
    },

    _updateFilter: function(e)
    {
        this._removeAllNodeHighlights();
        var isMac = WebInspector.isMac();
        var selectMultiple = false;
        if (isMac && e.metaKey && !e.ctrlKey && !e.altKey && !e.shiftKey)
            selectMultiple = true;
        if (!isMac && e.ctrlKey && !e.metaKey && !e.altKey && !e.shiftKey)
            selectMultiple = true;

        this._filter(e.target, selectMultiple);
        this.performSearch(null);
        this._updateSummaryBar();
    },

    _filter: function(target, selectMultiple)
    {
        function unselectAll()
        {
            for (var i = 0; i < this._filterBarElement.childNodes.length; ++i) {
                var child = this._filterBarElement.childNodes[i];
                if (!child.typeName)
                    continue;

                child.removeStyleClass("selected");
                this._hideCategory(child.typeName);
            }
        }

        if (target === this._filterAllElement) {
            if (target.hasStyleClass("selected")) {
                // We can't unselect All, so we break early here
                return;
            }

            // If All wasn't selected, and now is, unselect everything else.
            unselectAll.call(this);
        } else {
            // Something other than All is being selected, so we want to unselect All.
            if (this._filterAllElement.hasStyleClass("selected")) {
                this._filterAllElement.removeStyleClass("selected");
                this._hideCategory("all");
            }
        }

        if (!selectMultiple) {
            // If multiple selection is off, we want to unselect everything else
            // and just select ourselves.
            unselectAll.call(this);

            target.addStyleClass("selected");
            this._showCategory(target.typeName);
            this._updateOffscreenRows();
            return;
        }

        if (target.hasStyleClass("selected")) {
            // If selectMultiple is turned on, and we were selected, we just
            // want to unselect ourselves.
            target.removeStyleClass("selected");
            this._hideCategory(target.typeName);
        } else {
            // If selectMultiple is turned on, and we weren't selected, we just
            // want to select ourselves.
            target.addStyleClass("selected");
            this._showCategory(target.typeName);
        }
        this._updateOffscreenRows();
    },

    _defaultRefreshDelay: 500,

    _scheduleRefresh: function()
    {
        if (this._needsRefresh)
            return;

        this._needsRefresh = true;

        if (this.isShowing() && !this._refreshTimeout)
            this._refreshTimeout = setTimeout(this.refresh.bind(this), this._defaultRefreshDelay);
    },

    _updateDividersIfNeeded: function()
    {
        if (!this._dataGrid)
            return;
        var timelineColumn = this._dataGrid.columns.timeline;
        for (var i = 0; i < this._dataGrid.resizers.length; ++i) {
            if (timelineColumn.ordinal === this._dataGrid.resizers[i].rightNeighboringColumnID) {
                // Position timline grid location.
                this._timelineGrid.element.style.left = this._dataGrid.resizers[i].style.left;
                this._timelineGrid.element.style.right = "18px";
            }
        }

        var proceed = true;
        if (!this.isShowing()) {
            this._scheduleRefresh();
            proceed = false;
        } else {
            this.calculator.setDisplayWindow(this._timelineGrid.element.clientWidth);
            proceed = this._timelineGrid.updateDividers(this.calculator);
        }
        if (!proceed)
            return;

        if (this.calculator.startAtZero || !this.calculator.computePercentageFromEventTime) {
            // If our current sorting method starts at zero, that means it shows all
            // requests starting at the same point, and so onLoad event and DOMContent
            // event lines really wouldn't make much sense here, so don't render them.
            // Additionally, if the calculator doesn't have the computePercentageFromEventTime
            // function defined, we are probably sorting by size, and event times aren't relevant
            // in this case.
            return;
        }

        this._timelineGrid.removeEventDividers();
        if (this._mainRequestLoadTime !== -1) {
            var percent = this.calculator.computePercentageFromEventTime(this._mainRequestLoadTime);

            var loadDivider = document.createElement("div");
            loadDivider.className = "network-event-divider network-red-divider";

            var loadDividerPadding = document.createElement("div");
            loadDividerPadding.className = "network-event-divider-padding";
            loadDividerPadding.title = WebInspector.UIString("Load event fired");
            loadDividerPadding.appendChild(loadDivider);
            loadDividerPadding.style.left = percent + "%";
            this._timelineGrid.addEventDivider(loadDividerPadding);
        }

        if (this._mainRequestDOMContentTime !== -1) {
            var percent = this.calculator.computePercentageFromEventTime(this._mainRequestDOMContentTime);

            var domContentDivider = document.createElement("div");
            domContentDivider.className = "network-event-divider network-blue-divider";

            var domContentDividerPadding = document.createElement("div");
            domContentDividerPadding.className = "network-event-divider-padding";
            domContentDividerPadding.title = WebInspector.UIString("DOMContent event fired");
            domContentDividerPadding.appendChild(domContentDivider);
            domContentDividerPadding.style.left = percent + "%";
            this._timelineGrid.addEventDivider(domContentDividerPadding);
        }
    },

    _refreshIfNeeded: function()
    {
        if (this._needsRefresh)
            this.refresh();
    },

    _invalidateAllItems: function()
    {
        for (var i = 0; i < this._requests.length; ++i) {
            var request = this._requests[i];
            this._staleRequests[request.requestId] = request;
        }
    },

    get calculator()
    {
        return this._calculator;
    },

    set calculator(x)
    {
        if (!x || this._calculator === x)
            return;

        this._calculator = x;
        this._calculator.reset();

        this._invalidateAllItems();
        this.refresh();
    },

    _requestGridNode: function(request)
    {
        return this._requestGridNodes[request.__gridNodeId];
    },

    _createRequestGridNode: function(request)
    {
        var node = new WebInspector.NetworkDataGridNode(this, request);
        request.__gridNodeId = this._lastRequestGridNodeId++;
        this._requestGridNodes[request.__gridNodeId] = node;
        return node;
    },

    _createStatusbarButtons: function()
    {
        this._preserveLogToggle = new WebInspector.StatusBarButton(WebInspector.UIString("Preserve Log upon Navigation"), "record-profile-status-bar-item");
        this._preserveLogToggle.addEventListener("click", this._onPreserveLogClicked, this);

        this._clearButton = new WebInspector.StatusBarButton(WebInspector.UIString("Clear"), "clear-status-bar-item");
        this._clearButton.addEventListener("click", this._reset, this);

        this._largerRequestsButton = new WebInspector.StatusBarButton(WebInspector.UIString("Use small resource rows."), "network-larger-resources-status-bar-item");
        this._largerRequestsButton.toggled = WebInspector.settings.resourcesLargeRows.get();
        this._largerRequestsButton.addEventListener("click", this._toggleLargerRequests, this);
    },

    _onLoadEventFired: function(event)
    {
        this._mainRequestLoadTime = event.data || -1;
        // Schedule refresh to update boundaries and draw the new line.
        this._scheduleRefresh();
    },

    _domContentLoadedEventFired: function(event)
    {
        this._mainRequestDOMContentTime = event.data || -1;
        // Schedule refresh to update boundaries and draw the new line.
        this._scheduleRefresh();
    },

    wasShown: function()
    {
        this._refreshIfNeeded();
    },

    willHide: function()
    {
        this._popoverHelper.hidePopover();
    },

    refresh: function()
    {
        this._needsRefresh = false;
        if (this._refreshTimeout) {
            clearTimeout(this._refreshTimeout);
            delete this._refreshTimeout;
        }

        this._removeAllNodeHighlights();
        var wasScrolledToLastRow = this._dataGrid.isScrolledToLastRow();
        var boundariesChanged = false;
        if (this.calculator.updateBoundariesForEventTime) {
            boundariesChanged = this.calculator.updateBoundariesForEventTime(this._mainRequestLoadTime) || boundariesChanged;
            boundariesChanged = this.calculator.updateBoundariesForEventTime(this._mainRequestDOMContentTime) || boundariesChanged;
        }

        for (var requestId in this._staleRequests) {
            var request = this._staleRequests[requestId];
            var node = this._requestGridNode(request);
            if (!node) {
                // Create the timeline tree element and graph.
                node = this._createRequestGridNode(request);
                this._dataGrid.rootNode().appendChild(node);
            }
            node.refreshRequest();

            if (this.calculator.updateBoundaries(request))
                boundariesChanged = true;

            if (!node.isFilteredOut())
                this._updateHighlightIfMatched(request);
        }

        if (boundariesChanged) {
            // The boundaries changed, so all item graphs are stale.
            this._invalidateAllItems();
        }

        for (var requestId in this._staleRequests)
            this._requestGridNode(this._staleRequests[requestId]).refreshGraph(this.calculator);

        this._staleRequests = {};
        this._sortItems();
        this._updateSummaryBar();
        this._dataGrid.updateWidths();
        // FIXME: evaluate performance impact of moving this before a call to sortItems()
        if (wasScrolledToLastRow)
            this._dataGrid.scrollToLastRow();
    },

    _onPreserveLogClicked: function(e)
    {
        this._preserveLogToggle.toggled = !this._preserveLogToggle.toggled;
    },

    _reset: function()
    {
        this.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.ViewCleared);

        this._clearSearchMatchedList();
        if (this._popoverHelper)
            this._popoverHelper.hidePopover();

        if (this._calculator)
            this._calculator.reset();

        this._requests = [];
        this._requestsById = {};
        this._requestsByURL = {};
        this._staleRequests = {};
        this._requestGridNodes = {};

        if (this._dataGrid) {
            this._dataGrid.rootNode().removeChildren();
            this._updateDividersIfNeeded();
            this._updateSummaryBar();
        }

        this._mainRequestLoadTime = -1;
        this._mainRequestDOMContentTime = -1;
        this._linkifier.reset();
    },

    get requests()
    {
        return this._requests;
    },

    requestById: function(id)
    {
        return this._requestsById[id];
    },

    _onRequestStarted: function(event)
    {
        this._appendRequest(event.data);
    },

    _appendRequest: function(request)
    {
        this._requests.push(request);

        // In case of redirect request id is reassigned to a redirected
        // request and we need to update _requestsById ans search results.
        if (this._requestsById[request.requestId]) {
            var oldRequest = request.redirects[request.redirects.length - 1];
            this._requestsById[oldRequest.requestId] = oldRequest;

            this._updateSearchMatchedListAfterRequestIdChanged(request.requestId, oldRequest.requestId);
        }
        this._requestsById[request.requestId] = request;

        this._requestsByURL[request.url] = request;

        // Pull all the redirects of the main request upon commit load.
        if (request.redirects) {
            for (var i = 0; i < request.redirects.length; ++i)
                this._refreshRequest(request.redirects[i]);
        }

        this._refreshRequest(request);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _onRequestUpdated: function(event)
    {
        var request = /** @type {WebInspector.NetworkRequest} */ event.data;
        this._refreshRequest(request);
    },

    /**
     * @param {WebInspector.NetworkRequest} request
     */
    _refreshRequest: function(request)
    {
        this._staleRequests[request.requestId] = request;
        this._scheduleRefresh();
    },

    clear: function()
    {
        if (this._preserveLogToggle.toggled)
            return;
        this._reset();
    },

    _mainFrameNavigated: function(event)
    {
        if (this._preserveLogToggle.toggled)
            return;

        var frame = /** @type {WebInspector.ResourceTreeFrame} */ event.data;
        var loaderId = frame.loaderId;

        // Preserve provisional load requests.
        var requestsToPreserve = [];
        for (var i = 0; i < this._requests.length; ++i) {
            var request = this._requests[i];
            if (request.loaderId === loaderId)
                requestsToPreserve.push(request);
        }

        this._reset();

        // Restore preserved items.
        for (var i = 0; i < requestsToPreserve.length; ++i)
            this._appendRequest(requestsToPreserve[i]);
    },

    switchToDetailedView: function()
    {
        if (!this._dataGrid)
            return;
        if (this._dataGrid.selectedNode)
            this._dataGrid.selectedNode.selected = false;

        this.element.removeStyleClass("brief-mode");

        this._dataGrid.showColumn("method");
        this._dataGrid.showColumn("status");
        this._dataGrid.showColumn("type");
        this._dataGrid.showColumn("initiator");
        this._dataGrid.showColumn("size");
        this._dataGrid.showColumn("time");
        this._dataGrid.showColumn("timeline");

        var widths = {};
        widths.name = 20;
        widths.method = 6;
        widths.status = 6;
        widths.type = 6;
        widths.initiator = 10;
        widths.size = 6;
        widths.time = 6;
        widths.timeline = 40;

        this._dataGrid.applyColumnWidthsMap(widths);
    },

    switchToBriefView: function()
    {
        this.element.addStyleClass("brief-mode");
        this._removeAllNodeHighlights();

        this._dataGrid.hideColumn("method");
        this._dataGrid.hideColumn("status");
        this._dataGrid.hideColumn("type");
        this._dataGrid.hideColumn("initiator");
        this._dataGrid.hideColumn("size");
        this._dataGrid.hideColumn("time");
        this._dataGrid.hideColumn("timeline");

        var widths = {};
        widths.name = 100;
        this._dataGrid.applyColumnWidthsMap(widths);

        this._popoverHelper.hidePopover();
    },

    _toggleLargerRequests: function()
    {
        WebInspector.settings.resourcesLargeRows.set(!WebInspector.settings.resourcesLargeRows.get());
        this._setLargerRequests(WebInspector.settings.resourcesLargeRows.get());
    },

    _setLargerRequests: function(enabled)
    {
        this._largerRequestsButton.toggled = enabled;
        if (!enabled) {
            this._largerRequestsButton.title = WebInspector.UIString("Use large resource rows.");
            this._dataGrid.element.addStyleClass("small");
            this._timelineGrid.element.addStyleClass("small");
        } else {
            this._largerRequestsButton.title = WebInspector.UIString("Use small resource rows.");
            this._dataGrid.element.removeStyleClass("small");
            this._timelineGrid.element.removeStyleClass("small");
        }
        this.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.RowSizeChanged, { largeRows: enabled });
        this._updateOffscreenRows();
    },

    _getPopoverAnchor: function(element)
    {
        if (!this._allowPopover)
            return;
        var anchor = element.enclosingNodeOrSelfWithClass("network-graph-bar") || element.enclosingNodeOrSelfWithClass("network-graph-label");
        if (!anchor)
            return null;
        var request = anchor.parentElement.request;
        return request && request.timing ? anchor : null;
    },

    /**
     * @param {Element} anchor
     * @param {WebInspector.Popover} popover
     */
    _showPopover: function(anchor, popover)
    {
        var request = anchor.parentElement.request;
        var tableElement = WebInspector.RequestTimingView.createTimingTable(request);
        popover.show(tableElement, anchor);
    },

    _contextMenu: function(event)
    {
        var contextMenu = new WebInspector.ContextMenu();
        var gridNode = this._dataGrid.dataGridNodeFromNode(event.target);
        var request = gridNode && gridNode._request;

        if (request) {
            contextMenu.appendItem(WebInspector.openLinkExternallyLabel(), WebInspector.openResource.bind(WebInspector, request.url, false));
            contextMenu.appendSeparator();
            contextMenu.appendItem(WebInspector.copyLinkAddressLabel(), this._copyLocation.bind(this, request));
            if (request.requestHeadersText)
                contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Copy request headers" : "Copy Request Headers"), this._copyRequestHeaders.bind(this, request));
            if (request.responseHeadersText)
                contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Copy response headers" : "Copy Response Headers"), this._copyResponseHeaders.bind(this, request));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Copy entry as HAR" : "Copy Entry as HAR"), this._copyRequest.bind(this, request));
        }
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Copy all as HAR" : "Copy All as HAR"), this._copyAll.bind(this));

        if (InspectorFrontendHost.canSave()) {
            contextMenu.appendSeparator();
            if (request)
                contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Save entry as HAR" : "Save Entry as HAR"), this._exportRequest.bind(this, request));
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Save all as HAR" : "Save All as HAR"), this._exportAll.bind(this));
        }

        if (this._canClearBrowserCache || this._canClearBrowserCookies)
            contextMenu.appendSeparator();
        if (this._canClearBrowserCache)
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Clear browser cache" : "Clear Browser Cache"), this._clearBrowserCache.bind(this));
        if (this._canClearBrowserCookies)
            contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Clear browser cookies" : "Clear Browser Cookies"), this._clearBrowserCookies.bind(this));

        contextMenu.show(event);
    },

    _copyAll: function()
    {
        var harArchive = {
            log: (new WebInspector.HARLog(this._requests)).build()
        };
        InspectorFrontendHost.copyText(JSON.stringify(harArchive, null, 2));
    },

    _copyRequest: function(request)
    {
        var har = (new WebInspector.HAREntry(request)).build();
        InspectorFrontendHost.copyText(JSON.stringify(har, null, 2));
    },

    _copyLocation: function(request)
    {
        InspectorFrontendHost.copyText(request.url);
    },

    _copyRequestHeaders: function(request)
    {
        InspectorFrontendHost.copyText(request.requestHeadersText);
    },

    _copyResponseHeaders: function(request)
    {
        InspectorFrontendHost.copyText(request.responseHeadersText);
    },

    _exportAll: function()
    {
        var harArchive = {
            log: (new WebInspector.HARLog(this._requests)).build()
        };
        
        WebInspector.fileManager.save(WebInspector.inspectedPageDomain + ".har", JSON.stringify(harArchive, null, 2), true);
    },

    _exportRequest: function(request)
    {
        var har = (new WebInspector.HAREntry(request)).build();
        WebInspector.fileManager.save(request.displayName + ".har", JSON.stringify(har, null, 2), true);
    },

    _clearBrowserCache: function(event)
    {
        if (confirm(WebInspector.UIString("Are you sure you want to clear browser cache?")))
            NetworkAgent.clearBrowserCache();
    },

    _clearBrowserCookies: function(event)
    {
        if (confirm(WebInspector.UIString("Are you sure you want to clear browser cookies?")))
            NetworkAgent.clearBrowserCookies();
    },

    _updateOffscreenRows: function()
    {
        var dataTableBody = this._dataGrid.dataTableBody;
        var rows = dataTableBody.children;
        var recordsCount = rows.length;
        if (recordsCount < 2)
            return;  // Filler row only.

        var visibleTop = this._dataGrid.scrollContainer.scrollTop;
        var visibleBottom = visibleTop + this._dataGrid.scrollContainer.offsetHeight;

        var rowHeight = 0;

        // Filler is at recordsCount - 1.
        var unfilteredRowIndex = 0;
        for (var i = 0; i < recordsCount - 1; ++i) {
            var row = rows[i];

            var dataGridNode = this._dataGrid.dataGridNodeFromNode(row);
            if (dataGridNode.isFilteredOut()) {
                row.removeStyleClass("offscreen");
                continue;
            }

            if (!rowHeight)
                rowHeight = row.offsetHeight;

            var rowIsVisible = unfilteredRowIndex * rowHeight < visibleBottom && (unfilteredRowIndex + 1) * rowHeight > visibleTop;
            if (rowIsVisible !== row.rowIsVisible) {
                if (rowIsVisible)
                    row.removeStyleClass("offscreen");
                else
                    row.addStyleClass("offscreen");
                row.rowIsVisible = rowIsVisible;
            }
            unfilteredRowIndex++;
        }
    },

    _matchRequest: function(request)
    {
        if (!this._searchRegExp)
            return -1;

        if ((!request.displayName || !request.displayName.match(this._searchRegExp)) && !request.folder.match(this._searchRegExp))
            return -1;

        if (request.requestId in this._matchedRequestsMap)
            return this._matchedRequestsMap[request.requestId];

        var matchedRequestIndex = this._matchedRequests.length;
        this._matchedRequestsMap[request.requestId] = matchedRequestIndex;
        this._matchedRequests.push(request.requestId);

        return matchedRequestIndex;
    },

    _clearSearchMatchedList: function()
    {
        this._matchedRequests = [];
        this._matchedRequestsMap = {};
        this._highlightNthMatchedRequest(-1, false);
    },

    _updateSearchMatchedListAfterRequestIdChanged: function(oldRequestId, newRequestId)
    {
        var requestIndex = this._matchedRequestsMap[oldRequestId];
        if (requestIndex) {
            delete this._matchedRequestsMap[oldRequestId];
            this._matchedRequestsMap[newRequestId] = requestIndex;
            this._matchedRequests[requestIndex] = newRequestId;
        }
    },

    _updateHighlightIfMatched: function(request)
    {
        var matchedRequestIndex = this._matchRequest(request);
        if (matchedRequestIndex === -1)
            return;

        this.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.SearchCountUpdated, this._matchedRequests.length);

        if (this._currentMatchedRequestIndex !== -1 && this._currentMatchedRequestIndex !== matchedRequestIndex)
            return;

        this._highlightNthMatchedRequest(matchedRequestIndex, false);
    },

    _highlightNthMatchedRequest: function(matchedRequestIndex, reveal)
    {
        if (this._highlightedSubstringChanges) {
            WebInspector.revertDomChanges(this._highlightedSubstringChanges);
            this._highlightedSubstringChanges = null;
        }

        if (matchedRequestIndex === -1) {
            this._currentMatchedRequestIndex = matchedRequestIndex;
            return;
        }

        var request = this._requestsById[this._matchedRequests[matchedRequestIndex]];
        if (!request)
            return;

        var nameMatched = request.displayName && request.displayName.match(this._searchRegExp);
        var pathMatched = request.parsedURL.path && request.folder.match(this._searchRegExp);
        if (!nameMatched && pathMatched && !this._largerRequestsButton.toggled)
            this._toggleLargerRequests();

        var node = this._requestGridNode(request);
        if (node) {
            this._highlightedSubstringChanges = node._highlightMatchedSubstring(this._searchRegExp);
            if (reveal)
                node.reveal();
            this._currentMatchedRequestIndex = matchedRequestIndex;
        }
        this.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.SearchIndexUpdated, this._currentMatchedRequestIndex);
    },

    performSearch: function(searchQuery)
    {
        var newMatchedRequestIndex = 0;
        var currentMatchedRequestId;
        if (this._currentMatchedRequestIndex !== -1)
            currentMatchedRequestId = this._matchedRequests[this._currentMatchedRequestIndex];

        this._searchRegExp = createPlainTextSearchRegex(searchQuery, "i");

        this._clearSearchMatchedList();

        var childNodes = this._dataGrid.dataTableBody.childNodes;
        var requestNodes = Array.prototype.slice.call(childNodes, 0, childNodes.length - 1); // drop the filler row.

        for (var i = 0; i < requestNodes.length; ++i) {
            var dataGridNode = this._dataGrid.dataGridNodeFromNode(requestNodes[i]);
            if (dataGridNode.isFilteredOut())
                continue;

            if (this._matchRequest(dataGridNode._request) !== -1 && dataGridNode._request.requestId === currentMatchedRequestId)
                newMatchedRequestIndex = this._matchedRequests.length - 1;
        }

        this.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.SearchCountUpdated, this._matchedRequests.length);
        this._highlightNthMatchedRequest(newMatchedRequestIndex, false);
    },

    jumpToPreviousSearchResult: function()
    {
        if (!this._matchedRequests.length)
            return;
        this._highlightNthMatchedRequest((this._currentMatchedRequestIndex + this._matchedRequests.length - 1) % this._matchedRequests.length, true);
    },

    jumpToNextSearchResult: function()
    {
        if (!this._matchedRequests.length)
            return;
        this._highlightNthMatchedRequest((this._currentMatchedRequestIndex + 1) % this._matchedRequests.length, true);
    },

    searchCanceled: function()
    {
        this._clearSearchMatchedList();
        this.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.SearchCountUpdated, 0);
    },

    revealAndHighlightRequest: function(request)
    {
        this._removeAllNodeHighlights();

        var node = this._requestGridNode(request);
        if (node) {
            this._dataGrid.element.focus();
            node.reveal();
            this._highlightNode(node);
        }
    },

    _removeAllNodeHighlights: function()
    {
        if (this._highlightedNode) {
            this._highlightedNode.element.removeStyleClass("highlighted-row");
            delete this._highlightedNode;
        }
    },

    _highlightNode: function(node)
    {
        node.element.addStyleClass("highlighted-row");
        this._highlightedNode = node;
    }
};

WebInspector.NetworkLogView.prototype.__proto__ = WebInspector.View.prototype;

WebInspector.NetworkLogView.EventTypes = {
    ViewCleared: "ViewCleared",
    RowSizeChanged: "RowSizeChanged",
    RequestSelected: "RequestSelected",
    SearchCountUpdated: "SearchCountUpdated",
    SearchIndexUpdated: "SearchIndexUpdated"
};

/**
 * @constructor
 * @extends {WebInspector.Panel}
 * @implements {WebInspector.ContextMenu.Provider}
 */
WebInspector.NetworkPanel = function()
{
    WebInspector.Panel.call(this, "network");
    this.registerRequiredCSS("networkPanel.css");

    this.createSplitView();
    this.splitView.hideMainElement();

    this._networkLogView = new WebInspector.NetworkLogView();
    this._networkLogView.show(this.sidebarElement);

    this._viewsContainerElement = this.splitView.mainElement;
    this._viewsContainerElement.id = "network-views";
    this._viewsContainerElement.addStyleClass("hidden");
    if (!this._networkLogView.useLargeRows)
        this._viewsContainerElement.addStyleClass("small");

    this._networkLogView.addEventListener(WebInspector.NetworkLogView.EventTypes.ViewCleared, this._onViewCleared, this);
    this._networkLogView.addEventListener(WebInspector.NetworkLogView.EventTypes.RowSizeChanged, this._onRowSizeChanged, this);
    this._networkLogView.addEventListener(WebInspector.NetworkLogView.EventTypes.RequestSelected, this._onRequestSelected, this);
    this._networkLogView.addEventListener(WebInspector.NetworkLogView.EventTypes.SearchCountUpdated, this._onSearchCountUpdated, this);
    this._networkLogView.addEventListener(WebInspector.NetworkLogView.EventTypes.SearchIndexUpdated, this._onSearchIndexUpdated, this);

    this._closeButtonElement = document.createElement("button");
    this._closeButtonElement.id = "network-close-button";
    this._closeButtonElement.addEventListener("click", this._toggleGridMode.bind(this), false);
    this._viewsContainerElement.appendChild(this._closeButtonElement);

    function viewGetter()
    {
        return this.visibleView;
    }
    WebInspector.GoToLineDialog.install(this, viewGetter.bind(this));
    WebInspector.ContextMenu.registerProvider(this);
}

WebInspector.NetworkPanel.prototype = {
    get toolbarItemLabel()
    {
        return WebInspector.UIString("Network");
    },

    get statusBarItems()
    {
        return this._networkLogView.statusBarItems;
    },

    elementsToRestoreScrollPositionsFor: function()
    {
        return this._networkLogView.elementsToRestoreScrollPositionsFor();
    },

    // FIXME: only used by the layout tests, should not be exposed.
    _reset: function()
    {
        this._networkLogView._reset();
    },

    handleShortcut: function(event)
    {
        if (this._viewingRequestMode && event.keyCode === WebInspector.KeyboardShortcut.Keys.Esc.code) {
            this._toggleGridMode();
            event.handled = true;
            return;
        }

        WebInspector.Panel.prototype.handleShortcut.call(this, event);
    },

    wasShown: function()
    {
        WebInspector.Panel.prototype.wasShown.call(this);
    },

    get requests()
    {
        return this._networkLogView.requests;
    },

    requestById: function(id)
    {
        return this._networkLogView.requestById(id);
    },

    _requestByAnchor: function(anchor)
    {
        return anchor.requestId ? this.requestById(anchor.requestId) : this._networkLogView._requestsByURL[anchor.href];
    },

    canShowAnchorLocation: function(anchor)
    {
        return !!this._requestByAnchor(anchor);
    },

    showAnchorLocation: function(anchor)
    {
        var request = this._requestByAnchor(anchor);
        this.revealAndHighlightRequest(request)
    },

    revealAndHighlightRequest: function(request)
    {
        this._toggleGridMode();
        if (request)
            this._networkLogView.revealAndHighlightRequest(request);
    },

    _onViewCleared: function(event)
    {
        this._closeVisibleRequest();
        this._toggleGridMode();
        this._viewsContainerElement.removeChildren();
        this._viewsContainerElement.appendChild(this._closeButtonElement);
    },

    _onRowSizeChanged: function(event)
    {
        if (event.data.largeRows)
            this._viewsContainerElement.removeStyleClass("small");
        else
            this._viewsContainerElement.addStyleClass("small");
    },

    _onSearchCountUpdated: function(event)
    {
        WebInspector.searchController.updateSearchMatchesCount(event.data, this);
    },

    _onSearchIndexUpdated: function(event)
    {
        WebInspector.searchController.updateCurrentMatchIndex(event.data, this);
    },

    _onRequestSelected: function(event)
    {
        this._showRequest(event.data);
    },

    _showRequest: function(request)
    {
        if (!request)
            return;

        this._toggleViewingRequestMode();

        if (this.visibleView) {
            this.visibleView.detach();
            delete this.visibleView;
        }

        var view = new WebInspector.NetworkItemView(request);
        view.show(this._viewsContainerElement);
        this.visibleView = view;
    },

    _closeVisibleRequest: function()
    {
        this.element.removeStyleClass("viewing-resource");

        if (this.visibleView) {
            this.visibleView.detach();
            delete this.visibleView;
        }
    },

    _toggleGridMode: function()
    {
        if (this._viewingRequestMode) {
            this._viewingRequestMode = false;
            this.element.removeStyleClass("viewing-resource");
            this.splitView.hideMainElement();
        }

        this._networkLogView.switchToDetailedView();
        this._networkLogView.allowPopover = true;
        this._networkLogView._allowRequestSelection = false;
    },

    _toggleViewingRequestMode: function()
    {
        if (this._viewingRequestMode)
            return;
        this._viewingRequestMode = true;

        this.element.addStyleClass("viewing-resource");
        this.splitView.showMainElement();
        this._networkLogView.allowPopover = false;
        this._networkLogView._allowRequestSelection = true;
        this._networkLogView.switchToBriefView();
    },

    performSearch: function(searchQuery)
    {
        this._networkLogView.performSearch(searchQuery);
    },

    jumpToPreviousSearchResult: function()
    {
        this._networkLogView.jumpToPreviousSearchResult();
    },

    jumpToNextSearchResult: function()
    {
        this._networkLogView.jumpToNextSearchResult();
    },

    searchCanceled: function()
    {
        this._networkLogView.searchCanceled();
    },

    /** 
     * @param {WebInspector.ContextMenu} contextMenu
     * @param {Object} target
     */
    appendApplicableItems: function(contextMenu, target)
    {
        if (!(target instanceof WebInspector.NetworkRequest))
            return;
        if (this.visibleView && this.visibleView.isShowing() && this.visibleView.request() === target)
            return;

        function reveal()
        {
            WebInspector.inspectorView.setCurrentPanel(this);
            this.revealAndHighlightRequest(/** @type {WebInspector.NetworkRequest} */ target);
        }
        contextMenu.appendItem(WebInspector.UIString(WebInspector.useLowerCaseMenuTitles() ? "Reveal in network panel" : "Reveal in Network Panel"), reveal.bind(this));
    }
}

WebInspector.NetworkPanel.prototype.__proto__ = WebInspector.Panel.prototype;

/**
 * @constructor
 */
WebInspector.NetworkBaseCalculator = function()
{
}

WebInspector.NetworkBaseCalculator.prototype = {
    computePosition: function(time)
    {
        return (time - this.minimumBoundary) / this.boundarySpan * this._workingArea;
    },

    computeBarGraphPercentages: function(item)
    {
        return {start: 0, middle: 0, end: (this._value(item) / this.boundarySpan) * 100};
    },

    computeBarGraphLabels: function(item)
    {
        const label = this.formatTime(this._value(item));
        return {left: label, right: label, tooltip: label};
    },

    get boundarySpan()
    {
        return this.maximumBoundary - this.minimumBoundary;
    },

    updateBoundaries: function(item)
    {
        this.minimumBoundary = 0;

        var value = this._value(item);
        if (typeof this.maximumBoundary === "undefined" || value > this.maximumBoundary) {
            this.maximumBoundary = value;
            return true;
        }
        return false;
    },

    reset: function()
    {
        delete this.minimumBoundary;
        delete this.maximumBoundary;
    },

    _value: function(item)
    {
        return 0;
    },

    formatTime: function(value)
    {
        return value.toString();
    },

    setDisplayWindow: function(clientWidth)
    {
        this._workingArea = clientWidth;
        this.paddingLeft = 0;
    }
}

/**
 * @constructor
 * @extends {WebInspector.NetworkBaseCalculator}
 */
WebInspector.NetworkTimeCalculator = function(startAtZero)
{
    WebInspector.NetworkBaseCalculator.call(this);
    this.startAtZero = startAtZero;
}

WebInspector.NetworkTimeCalculator.prototype = {
    computeBarGraphPercentages: function(request)
    {
        if (request.startTime !== -1)
            var start = ((request.startTime - this.minimumBoundary) / this.boundarySpan) * 100;
        else
            var start = 0;

        if (request.responseReceivedTime !== -1)
            var middle = ((request.responseReceivedTime - this.minimumBoundary) / this.boundarySpan) * 100;
        else
            var middle = (this.startAtZero ? start : 100);

        if (request.endTime !== -1)
            var end = ((request.endTime - this.minimumBoundary) / this.boundarySpan) * 100;
        else
            var end = (this.startAtZero ? middle : 100);

        if (this.startAtZero) {
            end -= start;
            middle -= start;
            start = 0;
        }

        return {start: start, middle: middle, end: end};
    },

    computePercentageFromEventTime: function(eventTime)
    {
        // This function computes a percentage in terms of the total loading time
        // of a specific event. If startAtZero is set, then this is useless, and we
        // want to return 0.
        if (eventTime !== -1 && !this.startAtZero)
            return ((eventTime - this.minimumBoundary) / this.boundarySpan) * 100;

        return 0;
    },

    updateBoundariesForEventTime: function(eventTime)
    {
        if (eventTime === -1 || this.startAtZero)
            return false;

        if (typeof this.maximumBoundary === "undefined" || eventTime > this.maximumBoundary) {
            this.maximumBoundary = eventTime;
            return true;
        }
        return false;
    },

    computeBarGraphLabels: function(request)
    {
        var rightLabel = "";
        if (request.responseReceivedTime !== -1 && request.endTime !== -1)
            rightLabel = this.formatTime(request.endTime - request.responseReceivedTime);

        var hasLatency = request.latency > 0;
        if (hasLatency)
            var leftLabel = this.formatTime(request.latency);
        else
            var leftLabel = rightLabel;

        if (request.timing)
            return {left: leftLabel, right: rightLabel};

        if (hasLatency && rightLabel) {
            var total = this.formatTime(request.duration);
            var tooltip = WebInspector.UIString("%s latency, %s download (%s total)", leftLabel, rightLabel, total);
        } else if (hasLatency)
            var tooltip = WebInspector.UIString("%s latency", leftLabel);
        else if (rightLabel)
            var tooltip = WebInspector.UIString("%s download", rightLabel);

        if (request.cached)
            tooltip = WebInspector.UIString("%s (from cache)", tooltip);
        return {left: leftLabel, right: rightLabel, tooltip: tooltip};
    },

    updateBoundaries: function(request)
    {
        var didChange = false;

        var lowerBound;
        if (this.startAtZero)
            lowerBound = 0;
        else
            lowerBound = this._lowerBound(request);

        if (lowerBound !== -1 && (typeof this.minimumBoundary === "undefined" || lowerBound < this.minimumBoundary)) {
            this.minimumBoundary = lowerBound;
            didChange = true;
        }

        var upperBound = this._upperBound(request);
        if (upperBound !== -1 && (typeof this.maximumBoundary === "undefined" || upperBound > this.maximumBoundary)) {
            this.maximumBoundary = upperBound;
            didChange = true;
        }

        return didChange;
    },

    formatTime: function(value)
    {
        return Number.secondsToString(value);
    },

    _lowerBound: function(request)
    {
        return 0;
    },

    _upperBound: function(request)
    {
        return 0;
    }
}

WebInspector.NetworkTimeCalculator.prototype.__proto__ = WebInspector.NetworkBaseCalculator.prototype;

/**
 * @constructor
 * @extends {WebInspector.NetworkTimeCalculator}
 */
WebInspector.NetworkTransferTimeCalculator = function()
{
    WebInspector.NetworkTimeCalculator.call(this, false);
}

WebInspector.NetworkTransferTimeCalculator.prototype = {
    formatTime: function(value)
    {
        return Number.secondsToString(value);
    },

    _lowerBound: function(request)
    {
        return request.startTime;
    },

    _upperBound: function(request)
    {
        return request.endTime;
    }
}

WebInspector.NetworkTransferTimeCalculator.prototype.__proto__ = WebInspector.NetworkTimeCalculator.prototype;

/**
 * @constructor
 * @extends {WebInspector.NetworkTimeCalculator}
 */
WebInspector.NetworkTransferDurationCalculator = function()
{
    WebInspector.NetworkTimeCalculator.call(this, true);
}

WebInspector.NetworkTransferDurationCalculator.prototype = {
    formatTime: function(value)
    {
        return Number.secondsToString(value);
    },

    _upperBound: function(request)
    {
        return request.duration;
    }
}

WebInspector.NetworkTransferDurationCalculator.prototype.__proto__ = WebInspector.NetworkTimeCalculator.prototype;

/**
 * @constructor
 * @extends {WebInspector.DataGridNode}
 */
WebInspector.NetworkDataGridNode = function(parentView, request)
{
    WebInspector.DataGridNode.call(this, {});
    this._parentView = parentView;
    this._request = request;
}

WebInspector.NetworkDataGridNode.prototype = {
    createCells: function()
    {
        // Out of sight, out of mind: create nodes offscreen to save on render tree update times when running updateOffscreenRows()
        this._element.addStyleClass("offscreen");
        this._nameCell = this._createDivInTD("name");
        this._methodCell = this._createDivInTD("method");
        this._statusCell = this._createDivInTD("status");
        this._typeCell = this._createDivInTD("type");
        this._initiatorCell = this._createDivInTD("initiator");
        this._sizeCell = this._createDivInTD("size");
        this._timeCell = this._createDivInTD("time");
        this._createTimelineCell();
        this._nameCell.addEventListener("click", this.select.bind(this), false);
        this._nameCell.addEventListener("dblclick", this._openInNewTab.bind(this), false);
    },

    isFilteredOut: function()
    {
        if (!this._parentView._hiddenCategories.all)
            return false;
        return this._request.type.name() in this._parentView._hiddenCategories;
    },

    select: function()
    {
        this._parentView.dispatchEventToListeners(WebInspector.NetworkLogView.EventTypes.RequestSelected, this._request);
        WebInspector.DataGridNode.prototype.select.apply(this, arguments);
    },

    _highlightMatchedSubstring: function(regexp)
    {
        var domChanges = [];
        var matchInfo = this._nameCell.textContent.match(regexp);
        WebInspector.highlightSearchResult(this._nameCell, matchInfo.index, matchInfo[0].length, domChanges);
        return domChanges;
    },

    _openInNewTab: function()
    {
        InspectorFrontendHost.openInNewTab(this._request.url);
    },

    get selectable()
    {
        return this._parentView._allowRequestSelection && !this.isFilteredOut();
    },

    _createDivInTD: function(columnIdentifier)
    {
        var td = document.createElement("td");
        td.className = columnIdentifier + "-column";
        var div = document.createElement("div");
        td.appendChild(div);
        this._element.appendChild(td);
        return div;
    },

    _createTimelineCell: function()
    {
        this._graphElement = document.createElement("div");
        this._graphElement.className = "network-graph-side";

        this._barAreaElement = document.createElement("div");
        //    this._barAreaElement.className = "network-graph-bar-area hidden";
        this._barAreaElement.className = "network-graph-bar-area";
        this._barAreaElement.request = this._request;
        this._graphElement.appendChild(this._barAreaElement);

        this._barLeftElement = document.createElement("div");
        this._barLeftElement.className = "network-graph-bar waiting";
        this._barAreaElement.appendChild(this._barLeftElement);

        this._barRightElement = document.createElement("div");
        this._barRightElement.className = "network-graph-bar";
        this._barAreaElement.appendChild(this._barRightElement);


        this._labelLeftElement = document.createElement("div");
        this._labelLeftElement.className = "network-graph-label waiting";
        this._barAreaElement.appendChild(this._labelLeftElement);

        this._labelRightElement = document.createElement("div");
        this._labelRightElement.className = "network-graph-label";
        this._barAreaElement.appendChild(this._labelRightElement);

        this._graphElement.addEventListener("mouseover", this._refreshLabelPositions.bind(this), false);

        this._timelineCell = document.createElement("td");
        this._timelineCell.className = "timeline-column";
        this._element.appendChild(this._timelineCell);
        this._timelineCell.appendChild(this._graphElement);
    },

    refreshRequest: function()
    {
        this._refreshNameCell();

        this._methodCell.setTextAndTitle(this._request.requestMethod);

        this._refreshStatusCell();
        this._refreshTypeCell();
        this._refreshInitiatorCell();
        this._refreshSizeCell();
        this._refreshTimeCell();

        if (this._request.cached)
            this._graphElement.addStyleClass("resource-cached");

        this._element.addStyleClass("network-item");
        if (!this._element.hasStyleClass("network-type-" + this._request.type.name())) {
            this._element.removeMatchingStyleClasses("network-type-\\w+");
            this._element.addStyleClass("network-type-" + this._request.type.name());
        }
    },

    _refreshNameCell: function()
    {
        this._nameCell.removeChildren();

        if (this._request.type === WebInspector.resourceTypes.Image) {
            var previewImage = document.createElement("img");
            previewImage.className = "image-network-icon-preview";
            this._request.populateImageSource(previewImage);

            var iconElement = document.createElement("div");
            iconElement.className = "icon";
            iconElement.appendChild(previewImage);
        } else {
            var iconElement = document.createElement("img");
            iconElement.className = "icon";
        }
        this._nameCell.appendChild(iconElement);
        this._nameCell.appendChild(document.createTextNode(this._fileName()));


        var subtitle = WebInspector.displayDomain(this._request.parsedURL.host);

        if (this._request.parsedURL.path)
            subtitle += this._request.folder;

        this._appendSubtitle(this._nameCell, subtitle);
        this._nameCell.title = this._request.url;
    },

    _fileName: function()
    {
        var fileName = this._request.displayName;
        if (this._request.queryString)
            fileName += "?" + this._request.queryString;
        return fileName;
    },

    _refreshStatusCell: function()
    {
        this._statusCell.removeChildren();

        if (this._request.failed) {
            var failText = this._request.canceled ? WebInspector.UIString("(canceled)") : WebInspector.UIString("(failed)");
            if (this._request.localizedFailDescription) {
                this._statusCell.appendChild(document.createTextNode(failText));
                this._appendSubtitle(this._statusCell, this._request.localizedFailDescription);
                this._statusCell.title = failText + " " + this._request.localizedFailDescription;
            } else {
                this._statusCell.setTextAndTitle(failText);
            }
            this._statusCell.addStyleClass("network-dim-cell");
            this.element.addStyleClass("network-error-row");
            return;
        }

        this._statusCell.removeStyleClass("network-dim-cell");
        this.element.removeStyleClass("network-error-row");

        if (this._request.statusCode) {
            this._statusCell.appendChild(document.createTextNode(this._request.statusCode));
            this._appendSubtitle(this._statusCell, this._request.statusText);
            this._statusCell.title = this._request.statusCode + " " + this._request.statusText;
            if (this._request.statusCode >= 400)
                this.element.addStyleClass("network-error-row");
            if (this._request.cached)
                this._statusCell.addStyleClass("network-dim-cell");
        } else {
            if (!this._request.isHttpFamily() && this._request.finished)
                this._statusCell.setTextAndTitle(WebInspector.UIString("Success"));
            else if (this._request.isPingRequest())
                this._statusCell.setTextAndTitle(WebInspector.UIString("(ping)"));
            else
                this._statusCell.setTextAndTitle(WebInspector.UIString("(pending)"));
            this._statusCell.addStyleClass("network-dim-cell");
        }
    },

    _refreshTypeCell: function()
    {
        if (this._request.mimeType) {
            this._typeCell.removeStyleClass("network-dim-cell");
            this._typeCell.setTextAndTitle(this._request.mimeType);
        } else if (this._request.isPingRequest()) {
            this._typeCell.removeStyleClass("network-dim-cell");
            this._typeCell.setTextAndTitle(this._request.requestContentType());
        } else {
            this._typeCell.addStyleClass("network-dim-cell");
            this._typeCell.setTextAndTitle(WebInspector.UIString("Pending"));
        }
    },

    _refreshInitiatorCell: function()
    {
        var initiator = this._request.initiator;
        if ((initiator && initiator.type !== "other") || this._request.redirectSource) {
            this._initiatorCell.removeStyleClass("network-dim-cell");
            this._initiatorCell.removeChildren();
            if (this._request.redirectSource) {
                var redirectSource = this._request.redirectSource;
                this._initiatorCell.title = redirectSource.url;
                this._initiatorCell.appendChild(WebInspector.linkifyRequestAsNode(redirectSource));
                this._appendSubtitle(this._initiatorCell, WebInspector.UIString("Redirect"));
            } else if (initiator.type === "script") {
                var topFrame = initiator.stackTrace[0];
                // This could happen when request loading was triggered by console.
                if (!topFrame.url) {
                    this._initiatorCell.addStyleClass("network-dim-cell");
                    this._initiatorCell.setTextAndTitle(WebInspector.UIString("Other"));
                    return;
                }
                this._initiatorCell.title = topFrame.url + ":" + topFrame.lineNumber;
                var urlElement = this._parentView._linkifier.linkifyLocation(topFrame.url, topFrame.lineNumber - 1, 0);
                this._initiatorCell.appendChild(urlElement);
                this._appendSubtitle(this._initiatorCell, WebInspector.UIString("Script"));
            } else { // initiator.type === "parser"
                this._initiatorCell.title = initiator.url + ":" + initiator.lineNumber;
                this._initiatorCell.appendChild(WebInspector.linkifyResourceAsNode(initiator.url, initiator.lineNumber - 1));
                this._appendSubtitle(this._initiatorCell, WebInspector.UIString("Parser"));
            }
        } else {
            this._initiatorCell.addStyleClass("network-dim-cell");
            this._initiatorCell.setTextAndTitle(WebInspector.UIString("Other"));
        }
    },

    _refreshSizeCell: function()
    {
        if (this._request.cached) {
            this._sizeCell.setTextAndTitle(WebInspector.UIString("(from cache)"));
            this._sizeCell.addStyleClass("network-dim-cell");
        } else {
            var resourceSize = typeof this._request.resourceSize === "number" ? Number.bytesToString(this._request.resourceSize) : "?";
            var transferSize = typeof this._request.transferSize === "number" ? Number.bytesToString(this._request.transferSize) : "?";
            this._sizeCell.setTextAndTitle(transferSize);
            this._sizeCell.removeStyleClass("network-dim-cell");
            this._appendSubtitle(this._sizeCell, resourceSize);
        }
    },

    _refreshTimeCell: function()
    {
        if (this._request.duration > 0) {
            this._timeCell.removeStyleClass("network-dim-cell");
            this._timeCell.setTextAndTitle(Number.secondsToString(this._request.duration));
            this._appendSubtitle(this._timeCell, Number.secondsToString(this._request.latency));
        } else {
            this._timeCell.addStyleClass("network-dim-cell");
            this._timeCell.setTextAndTitle(WebInspector.UIString("Pending"));
        }
    },

    _appendSubtitle: function(cellElement, subtitleText)
    {
        var subtitleElement = document.createElement("div");
        subtitleElement.className = "network-cell-subtitle";
        subtitleElement.textContent = subtitleText;
        cellElement.appendChild(subtitleElement);
    },

    refreshGraph: function(calculator)
    {
        var percentages = calculator.computeBarGraphPercentages(this._request);
        this._percentages = percentages;

        this._barAreaElement.removeStyleClass("hidden");

        if (!this._graphElement.hasStyleClass("network-type-" + this._request.type.name())) {
            this._graphElement.removeMatchingStyleClasses("network-type-\\w+");
            this._graphElement.addStyleClass("network-type-" + this._request.type.name());
        }

        this._barLeftElement.style.setProperty("left", percentages.start + "%");
        this._barRightElement.style.setProperty("right", (100 - percentages.end) + "%");

        this._barLeftElement.style.setProperty("right", (100 - percentages.end) + "%");
        this._barRightElement.style.setProperty("left", percentages.middle + "%");

        var labels = calculator.computeBarGraphLabels(this._request);
        this._labelLeftElement.textContent = labels.left;
        this._labelRightElement.textContent = labels.right;

        var tooltip = (labels.tooltip || "");
        this._barLeftElement.title = tooltip;
        this._labelLeftElement.title = tooltip;
        this._labelRightElement.title = tooltip;
        this._barRightElement.title = tooltip;
    },

    _refreshLabelPositions: function()
    {
        if (!this._percentages)
            return;
        this._labelLeftElement.style.removeProperty("left");
        this._labelLeftElement.style.removeProperty("right");
        this._labelLeftElement.removeStyleClass("before");
        this._labelLeftElement.removeStyleClass("hidden");

        this._labelRightElement.style.removeProperty("left");
        this._labelRightElement.style.removeProperty("right");
        this._labelRightElement.removeStyleClass("after");
        this._labelRightElement.removeStyleClass("hidden");

        const labelPadding = 10;
        const barRightElementOffsetWidth = this._barRightElement.offsetWidth;
        const barLeftElementOffsetWidth = this._barLeftElement.offsetWidth;

        if (this._barLeftElement) {
            var leftBarWidth = barLeftElementOffsetWidth - labelPadding;
            var rightBarWidth = (barRightElementOffsetWidth - barLeftElementOffsetWidth) - labelPadding;
        } else {
            var leftBarWidth = (barLeftElementOffsetWidth - barRightElementOffsetWidth) - labelPadding;
            var rightBarWidth = barRightElementOffsetWidth - labelPadding;
        }

        const labelLeftElementOffsetWidth = this._labelLeftElement.offsetWidth;
        const labelRightElementOffsetWidth = this._labelRightElement.offsetWidth;

        const labelBefore = (labelLeftElementOffsetWidth > leftBarWidth);
        const labelAfter = (labelRightElementOffsetWidth > rightBarWidth);
        const graphElementOffsetWidth = this._graphElement.offsetWidth;

        if (labelBefore && (graphElementOffsetWidth * (this._percentages.start / 100)) < (labelLeftElementOffsetWidth + 10))
            var leftHidden = true;

        if (labelAfter && (graphElementOffsetWidth * ((100 - this._percentages.end) / 100)) < (labelRightElementOffsetWidth + 10))
            var rightHidden = true;

        if (barLeftElementOffsetWidth == barRightElementOffsetWidth) {
            // The left/right label data are the same, so a before/after label can be replaced by an on-bar label.
            if (labelBefore && !labelAfter)
                leftHidden = true;
            else if (labelAfter && !labelBefore)
                rightHidden = true;
        }

        if (labelBefore) {
            if (leftHidden)
                this._labelLeftElement.addStyleClass("hidden");
            this._labelLeftElement.style.setProperty("right", (100 - this._percentages.start) + "%");
            this._labelLeftElement.addStyleClass("before");
        } else {
            this._labelLeftElement.style.setProperty("left", this._percentages.start + "%");
            this._labelLeftElement.style.setProperty("right", (100 - this._percentages.middle) + "%");
        }

        if (labelAfter) {
            if (rightHidden)
                this._labelRightElement.addStyleClass("hidden");
            this._labelRightElement.style.setProperty("left", this._percentages.end + "%");
            this._labelRightElement.addStyleClass("after");
        } else {
            this._labelRightElement.style.setProperty("left", this._percentages.middle + "%");
            this._labelRightElement.style.setProperty("right", (100 - this._percentages.end) + "%");
        }
    }
}

WebInspector.NetworkDataGridNode.NameComparator = function(a, b)
{
    var aFileName = a._request.displayName + (a._request.queryString ? a._request.queryString : "");
    var bFileName = b._request.displayName + (b._request.queryString ? b._request.queryString : "");
    if (aFileName > bFileName)
        return 1;
    if (bFileName > aFileName)
        return -1;
    return 0;
}

WebInspector.NetworkDataGridNode.SizeComparator = function(a, b)
{
    if (b._request.cached && !a._request.cached)
        return 1;
    if (a._request.cached && !b._request.cached)
        return -1;

    if (a._request.resourceSize === b._request.resourceSize)
        return 0;

    return a._request.resourceSize - b._request.resourceSize;
}

WebInspector.NetworkDataGridNode.InitiatorComparator = function(a, b)
{
    if (!a._request.initiator || a._request.initiator.type === "Other")
        return -1;
    if (!b._request.initiator || b._request.initiator.type === "Other")
        return 1;

    if (a._request.initiator.url < b._request.initiator.url)
        return -1;
    if (a._request.initiator.url > b._request.initiator.url)
        return 1;

    return a._request.initiator.lineNumber - b._request.initiator.lineNumber;
}

WebInspector.NetworkDataGridNode.RequestPropertyComparator = function(propertyName, revert, a, b)
{
    var aValue = a._request[propertyName];
    var bValue = b._request[propertyName];
    if (aValue > bValue)
        return revert ? -1 : 1;
    if (bValue > aValue)
        return revert ? 1 : -1;
    return 0;
}

WebInspector.NetworkDataGridNode.prototype.__proto__ = WebInspector.DataGridNode.prototype;
