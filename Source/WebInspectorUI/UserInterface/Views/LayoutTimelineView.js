/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

WebInspector.LayoutTimelineView = function(timeline)
{
    WebInspector.TimelineView.call(this, timeline);

    console.assert(timeline.type === WebInspector.TimelineRecord.Type.Layout);

    this.navigationSidebarTreeOutline.onselect = this._treeElementSelected.bind(this);
    this.navigationSidebarTreeOutline.ondeselect = this._treeElementDeselected.bind(this);
    this.navigationSidebarTreeOutline.element.classList.add(WebInspector.NavigationSidebarPanel.HideDisclosureButtonsStyleClassName);
    this.navigationSidebarTreeOutline.element.classList.add(WebInspector.LayoutTimelineView.TreeOutlineStyleClassName);

    var columns = {eventType: {}, initiatorCallFrame: {}, width: {}, height: {}, startTime: {}, duration: {}};

    columns.eventType.title = WebInspector.UIString("Type");
    columns.eventType.width = "15%";
    columns.eventType.scopeBar = WebInspector.TimelineDataGrid.createColumnScopeBar("layout", WebInspector.LayoutTimelineRecord.EventType);
    columns.eventType.hidden = true;

    columns.initiatorCallFrame.title = WebInspector.UIString("Initiator");
    columns.initiatorCallFrame.width = "25%";

    columns.width.title = WebInspector.UIString("Width");
    columns.width.width = "8%";

    columns.height.title = WebInspector.UIString("Height");
    columns.height.width = "8%";

    columns.startTime.title = WebInspector.UIString("Start Time");
    columns.startTime.width = "8%";
    columns.startTime.aligned = "right";

    columns.duration.title = WebInspector.UIString("Duration");
    columns.duration.width = "8%";
    columns.duration.aligned = "right";

    for (var column in columns)
        columns[column].sortable = true;

    this._dataGrid = new WebInspector.LayoutTimelineDataGrid(this.navigationSidebarTreeOutline, columns);
    this._dataGrid.addEventListener(WebInspector.TimelineDataGrid.Event.FiltersDidChange, this._dataGridFiltersDidChange, this);
    this._dataGrid.addEventListener(WebInspector.DataGrid.Event.SelectedNodeChanged, this._dataGridNodeSelected, this);

    this._dataGrid.sortColumnIdentifier = "startTime";
    this._dataGrid.sortOrder = WebInspector.DataGrid.SortOrder.Ascending;

    this.element.classList.add(WebInspector.LayoutTimelineView.StyleClassName);
    this.element.appendChild(this._dataGrid.element);

    timeline.addEventListener(WebInspector.Timeline.Event.RecordAdded, this._layoutTimelineRecordAdded, this);

    this._pendingRecords = [];
};

WebInspector.LayoutTimelineView.StyleClassName = "layout";
WebInspector.LayoutTimelineView.TreeOutlineStyleClassName = "layout";

WebInspector.LayoutTimelineView.prototype = {
    constructor: WebInspector.LayoutTimelineView,
    __proto__: WebInspector.TimelineView.prototype,

    // Public

    get navigationSidebarTreeOutlineLabel()
    {
        return WebInspector.UIString("Records");
    },

    shown: function()
    {
        WebInspector.TimelineView.prototype.shown.call(this);

        this._dataGrid.shown();
    },

    hidden: function()
    {
        this._dataGrid.hidden();

        WebInspector.TimelineView.prototype.hidden.call(this);
    },

    updateLayout: function()
    {
        WebInspector.TimelineView.prototype.updateLayout.call(this);

        this._dataGrid.updateLayout();

        this._processPendingRecords();
    },

    matchTreeElementAgainstCustomFilters: function(treeElement)
    {
        return this._dataGrid.treeElementMatchesActiveScopeFilters(treeElement);
    },

    reset: function()
    {
        WebInspector.TimelineView.prototype.reset.call(this);

        this._dataGrid.reset();
    },

    // Protected

    treeElementPathComponentSelected: function(event)
    {
        var dataGridNode = this._dataGrid.dataGridNodeForTreeElement(event.data.pathComponent.generalTreeElement);
        if (!dataGridNode)
            return;
        dataGridNode.revealAndSelect();
    },

    // Private

    _processPendingRecords: function()
    {
        if (!this._pendingRecords.length)
            return;

        for (var layoutTimelineRecord of this._pendingRecords) {
            var treeElement = new WebInspector.TimelineRecordTreeElement(layoutTimelineRecord, WebInspector.SourceCodeLocation.NameStyle.Short);
            var dataGridNode = new WebInspector.LayoutTimelineDataGridNode(layoutTimelineRecord, this.zeroTime);

            this._dataGrid.addRowInSortOrder(treeElement, dataGridNode);
        }

        this._pendingRecords = [];
    },

    _layoutTimelineRecordAdded: function(event)
    {
        var layoutTimelineRecord = event.data.record;
        console.assert(layoutTimelineRecord instanceof WebInspector.LayoutTimelineRecord);

        this._pendingRecords.push(layoutTimelineRecord);

        this.needsLayout();
    },

    _dataGridFiltersDidChange: function(event)
    {
        WebInspector.timelineSidebarPanel.updateFilter();
    },

    _dataGridNodeSelected: function(event)
    {
        this.dispatchEventToListeners(WebInspector.TimelineView.Event.SelectionPathComponentsDidChange);
    },

    _treeElementDeselected: function(treeElement)
    {
        if (treeElement.status)
            treeElement.status = "";
    },

    _treeElementSelected: function(treeElement, selectedByUser)
    {
        if (this._dataGrid.shouldIgnoreSelectionEvent())
            return;

        if (!WebInspector.timelineSidebarPanel.canShowDifferentContentView())
            return;

        if (treeElement instanceof WebInspector.FolderTreeElement)
            return;

        if (!(treeElement instanceof WebInspector.TimelineRecordTreeElement)) {
            console.error("Unknown tree element selected.");
            return;
        }

        if (!treeElement.record.sourceCodeLocation) {
            WebInspector.timelineSidebarPanel.showTimelineViewForType(WebInspector.TimelineRecord.Type.Layout);
            return;
        }

        WebInspector.resourceSidebarPanel.showOriginalOrFormattedSourceCodeLocation(treeElement.record.sourceCodeLocation);
        this._updateTreeElementWithCloseButton(treeElement);
    },

    _updateTreeElementWithCloseButton: function(treeElement)
    {
        if (this._closeStatusButton) {
            treeElement.status = this._closeStatusButton.element;
            return;
        }

        wrappedSVGDocument(platformImagePath("Close.svg"), null, WebInspector.UIString("Close resource view"), function(element) {
            this._closeStatusButton = new WebInspector.TreeElementStatusButton(element);
            this._closeStatusButton.addEventListener(WebInspector.TreeElementStatusButton.Event.Clicked, this._closeStatusButtonClicked, this);
            if (treeElement === this.navigationSidebarTreeOutline.selectedTreeElement)
                this._updateTreeElementWithCloseButton(treeElement);
        }.bind(this));
    },

    _closeStatusButtonClicked: function(event)
    {
        this.navigationSidebarTreeOutline.selectedTreeElement.deselect();
        WebInspector.timelineSidebarPanel.showTimelineViewForType(WebInspector.TimelineRecord.Type.Layout);
    }
};
