/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.TimelineContentView = function(recording)
{
    WebInspector.ContentView.call(this, recording);

    this._recording = recording;

    this.element.classList.add(WebInspector.TimelineContentView.StyleClassName);

    this._discreteTimelineOverviewGraphMap = new Map;
    for (var [identifier, timeline] of recording.timelines)
        this._discreteTimelineOverviewGraphMap.set(timeline, new WebInspector.TimelineOverviewGraph(timeline));

    this._timelineOverview = new WebInspector.TimelineOverview(this._discreteTimelineOverviewGraphMap);
    this._timelineOverview.addEventListener(WebInspector.TimelineOverview.Event.TimeRangeSelectionChanged, this._timeRangeSelectionChanged, this);
    this.element.appendChild(this._timelineOverview.element);

    this._viewContainer = document.createElement("div");
    this._viewContainer.classList.add(WebInspector.TimelineContentView.ViewContainerStyleClassName);
    this.element.appendChild(this._viewContainer);

    var trashImage;
    if (WebInspector.Platform.isLegacyMacOS)
        trashImage = {src: "Images/Legacy/NavigationItemTrash.svg", width: 16, height: 16};
    else
        trashImage = {src: "Images/NavigationItemTrash.svg", width: 15, height: 15};

    this._clearTimelineNavigationItem = new WebInspector.ButtonNavigationItem("clear-timeline", WebInspector.UIString("Clear Timeline"), trashImage.src, trashImage.width, trashImage.height);
    this._clearTimelineNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._clearTimeline, this);

    this._overviewTimelineView = new WebInspector.OverviewTimelineView(recording);

    this._discreteTimelineViewMap = new Map;
    for (var [identifier, timeline] of recording.timelines)
        this._discreteTimelineViewMap.set(timeline, new WebInspector.TimelineView(timeline));

    function createPathComponent(displayName, className, representedObject)
    {
        var pathComponent = new WebInspector.HierarchicalPathComponent(displayName, className, representedObject);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this._pathComponentSelected, this);
        return pathComponent;
    }

    var networkTimeline = recording.timelines.get(WebInspector.TimelineRecord.Type.Network);
    var layoutTimeline = recording.timelines.get(WebInspector.TimelineRecord.Type.Layout);
    var scriptTimeline = recording.timelines.get(WebInspector.TimelineRecord.Type.Script);

    this._pathComponentMap = new Map;
    this._pathComponentMap.set(networkTimeline, createPathComponent.call(this, WebInspector.UIString("Network Requests"), WebInspector.TimelineSidebarPanel.NetworkIconStyleClass, networkTimeline));
    this._pathComponentMap.set(layoutTimeline, createPathComponent.call(this, WebInspector.UIString("Layout & Rendering"), WebInspector.TimelineSidebarPanel.ColorsIconStyleClass, layoutTimeline));
    this._pathComponentMap.set(scriptTimeline, createPathComponent.call(this, WebInspector.UIString("JavaScript & Events"), WebInspector.TimelineSidebarPanel.ScriptIconStyleClass, scriptTimeline));

    var previousPathComponent = null;
    for (var pathComponent of this._pathComponentMap.values()) {
        if (previousPathComponent) {
            previousPathComponent.nextSibling = pathComponent;
            pathComponent.previousSibling = previousPathComponent;
        }

        previousPathComponent = pathComponent;
    }

    this._currentTimelineView = null;
    this._currentTimelineViewIdentifier = null;

    this._updating = false;
    this._currentTime = NaN;
    this._lastUpdateTimestamp = NaN;
    this._startTimeNeedsReset = true;

    recording.addEventListener(WebInspector.TimelineRecording.Event.Reset, this._recordingReset, this);
    recording.addEventListener(WebInspector.TimelineRecording.Event.Unloaded, this._recordingUnloaded, this);

    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.CapturingStarted, this._capturingStarted, this);
    WebInspector.timelineManager.addEventListener(WebInspector.TimelineManager.Event.CapturingStopped, this._capturingStopped, this);

    this.showOverviewTimelineView();
};

WebInspector.TimelineContentView.StyleClassName = "timeline";
WebInspector.TimelineContentView.ViewContainerStyleClassName = "view-container";

WebInspector.TimelineContentView.SelectedTimelineTypeCookieKey = "timeline-content-view-selected-timeline-type";
WebInspector.TimelineContentView.OverviewTimelineViewCookieValue = "timeline-content-view-overview-timeline-view";

WebInspector.TimelineContentView.prototype = {
    constructor: WebInspector.TimelineContentView,
    __proto__: WebInspector.ContentView.prototype,

    // Public

    showOverviewTimelineView: function()
    {
        this._showTimelineView(this._overviewTimelineView);
    },

    showTimelineViewForTimeline: function(timeline)
    {
        console.assert(timeline instanceof WebInspector.Timeline, timeline);
        console.assert(this._discreteTimelineViewMap.has(timeline), timeline);
        if (!this._discreteTimelineViewMap.has(timeline))
            return;

        this._showTimelineView(this._discreteTimelineViewMap.get(timeline));
    },

    get allowedNavigationSidebarPanels()
    {
        return ["timeline"];
    },

    get supportsSplitContentBrowser()
    {
        // The layout of the overview and split content browser don't work well.
        return false;
    },

    get selectionPathComponents()
    {
        var pathComponents = [];
        if (this._currentTimelineView.representedObject instanceof WebInspector.Timeline)
            pathComponents.push(this._pathComponentMap.get(this._currentTimelineView.representedObject));
        pathComponents = pathComponents.concat(this._currentTimelineView.selectionPathComponents || []);
        return pathComponents;
    },

    get navigationItems()
    {
        return [this._clearTimelineNavigationItem];
    },

    get currentTimelineView()
    {
        return this._currentTimelineView;
    },

    shown: function()
    {
        if (!this._currentTimelineView)
            return;

        this._currentTimelineView.shown();
        this._clearTimelineNavigationItem.enabled = this._recording.isWritable();
    },

    hidden: function()
    {
        if (!this._currentTimelineView)
            return;

        this._currentTimelineView.hidden();
    },

    updateLayout: function()
    {
        this._timelineOverview.updateLayoutForResize();

        if (!this._currentTimelineView)
            return;

        this._currentTimelineView.updateLayout();
    },

    saveToCookie: function(cookie)
    {
        cookie.type = WebInspector.ContentViewCookieType.Timelines;

        if (!this._currentTimelineView || this._currentTimelineView === this._overviewTimelineView)
            cookie[WebInspector.TimelineContentView.SelectedTimelineTypeCookieKey] = WebInspector.TimelineContentView.OverviewTimelineViewCookieValue;
        else
            cookie[WebInspector.TimelineContentView.SelectedTimelineTypeCookieKey] = this._currentTimelineView.representedObject.type;
    },

    restoreFromCookie: function(cookie)
    {
        var timelineType = cookie[WebInspector.TimelineContentView.SelectedTimelineTypeCookieKey];

        if (timelineType === WebInspector.TimelineContentView.OverviewTimelineViewCookieValue)
            this.showOverviewTimelineView();
        else
            this.showTimelineViewForTimeline(this.representedObject.timelines.get(timelineType));
    },

    matchTreeElementAgainstCustomFilters: function(treeElement)
    {
        if (this._currentTimelineView && !this._currentTimelineView.matchTreeElementAgainstCustomFilters(treeElement))
            return false;

        var startTime = this._timelineOverview.selectionStartTime;
        var endTime = this._timelineOverview.selectionStartTime + this._timelineOverview.selectionDuration;
        var currentTime = this._currentTime || this._recording.startTime;

        function checkTimeBounds(itemStartTime, itemEndTime)
        {
            itemStartTime = itemStartTime || currentTime;
            itemEndTime = itemEndTime || currentTime;

            return startTime <= itemEndTime && itemStartTime <= endTime;
        }

        if (treeElement instanceof WebInspector.ResourceTreeElement) {
            var resource = treeElement.resource;
            return checkTimeBounds(resource.requestSentTimestamp, resource.finishedOrFailedTimestamp);
        }

        if (treeElement instanceof WebInspector.SourceCodeTimelineTreeElement) {
            var sourceCodeTimeline = treeElement.sourceCodeTimeline;

            // Do a quick check of the timeline bounds before we check each record.
            if (!checkTimeBounds(sourceCodeTimeline.startTime, sourceCodeTimeline.endTime))
                return false;

            for (var record of sourceCodeTimeline.records) {
                if (checkTimeBounds(record.startTime, record.endTime))
                    return true;
            }

            return false;
        }

        if (treeElement instanceof WebInspector.ProfileNodeTreeElement) {
            var profileNode = treeElement.profileNode;
            for (var call of profileNode.calls) {
                if (checkTimeBounds(call.startTime, call.endTime))
                    return true;
            }

            return false;
        }

        if (treeElement instanceof WebInspector.TimelineRecordTreeElement) {
            var record = treeElement.record;
            return checkTimeBounds(record.startTime, record.endTime);
        }

        console.error("Unknown TreeElement, can't filter by time.");
        return true;
    },

    // Private

    _pathComponentSelected: function(event)
    {
        WebInspector.timelineSidebarPanel.showTimelineViewForType(event.data.pathComponent.representedObject.type);
    },

    _timelineViewSelectionPathComponentsDidChange: function()
    {
        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
    },

    _showTimelineView: function(timelineView)
    {
        console.assert(timelineView instanceof WebInspector.TimelineView);
        console.assert(timelineView.representedObject === this._recording || this._recording.timelines.has(timelineView.representedObject.type));

        // If the content view is shown and then hidden, we must reattach the content tree outline and timeline view.
        if (timelineView.visible && timelineView === this._currentTimelineView)
            return;

        if (this._currentTimelineView) {
            this._currentTimelineView.removeEventListener(WebInspector.TimelineView.Event.SelectionPathComponentsDidChange, this._timelineViewSelectionPathComponentsDidChange, this);

            this._currentTimelineView.hidden();
            this._currentTimelineView.element.remove();
        }

        this._currentTimelineView = timelineView;

        WebInspector.timelineSidebarPanel.contentTreeOutline = timelineView && timelineView.navigationSidebarTreeOutline;
        WebInspector.timelineSidebarPanel.contentTreeOutlineLabel = timelineView && timelineView.navigationSidebarTreeOutlineLabel;

        if (this._currentTimelineView) {
            this._currentTimelineView.addEventListener(WebInspector.TimelineView.Event.SelectionPathComponentsDidChange, this._timelineViewSelectionPathComponentsDidChange, this);

            this._viewContainer.appendChild(this._currentTimelineView.element);

            this._currentTimelineView.startTime = this._timelineOverview.selectionStartTime;
            this._currentTimelineView.endTime = this._timelineOverview.selectionStartTime + this._timelineOverview.selectionDuration;
            this._currentTimelineView.currentTime = this._currentTime;

            this._currentTimelineView.shown();
            this._currentTimelineView.updateLayout();
        }

        this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
    },

    _update: function(timestamp)
    {
        if (this._waitingToResetCurrentTime) {
            requestAnimationFrame(this._updateCallback);
            return;
        }

        var startTime = this._recording.startTime;
        var currentTime = this._currentTime || startTime;
        var endTime = this._recording.endTime;
        var timespanSinceLastUpdate = (timestamp - this._lastUpdateTimestamp) / 1000 || 0;

        currentTime += timespanSinceLastUpdate;

        this._updateTimes(startTime, currentTime, endTime);

        // Only stop updating if the current time is greater than the end time.
        if (!this._updating && currentTime >= endTime) {
            this._lastUpdateTimestamp = NaN;
            return;
        }

        this._lastUpdateTimestamp = timestamp;

        requestAnimationFrame(this._updateCallback);
    },

    _updateTimes: function(startTime, currentTime, endTime)
    {
        if (this._startTimeNeedsReset && !isNaN(startTime)) {
            var selectionOffset = this._timelineOverview.selectionStartTime - this._timelineOverview.startTime;

            this._timelineOverview.startTime = startTime;
            this._timelineOverview.selectionStartTime = startTime + selectionOffset;

            this._overviewTimelineView.zeroTime = startTime;
            for (var timelineView of this._discreteTimelineViewMap.values())
                timelineView.zeroTime = startTime;

            delete this._startTimeNeedsReset;
        }

        this._timelineOverview.endTime = Math.max(endTime, currentTime);

        this._currentTime = currentTime;
        this._timelineOverview.currentTime = currentTime;
        this._currentTimelineView.currentTime = currentTime;

        // Force a layout now since we are already in an animation frame and don't need to delay it until the next.
        this._timelineOverview.updateLayoutIfNeeded();
        this._currentTimelineView.updateLayoutIfNeeded();
    },

    _startUpdatingCurrentTime: function()
    {
        console.assert(!this._updating);
        if (this._updating)
            return;

        if (!isNaN(this._currentTime)) {
            // We have a current time already, so we likely need to jump into the future to a better current time.
            // This happens when you stop and later restart recording.
            console.assert(!this._waitingToResetCurrentTime);
            this._waitingToResetCurrentTime = true;
            this._recording.addEventListener(WebInspector.TimelineRecording.Event.TimesUpdated, this._recordingTimesUpdated, this);
        }

        this._updating = true;

        if (!this._updateCallback)
            this._updateCallback = this._update.bind(this);

        requestAnimationFrame(this._updateCallback);
    },

    _stopUpdatingCurrentTime: function()
    {
        console.assert(this._updating);
        this._updating = false;

        if (this._waitingToResetCurrentTime) {
            // Did not get any event while waiting for the current time, but we should stop waiting.
            this._recording.removeEventListener(WebInspector.TimelineRecording.Event.TimesUpdated, this._recordingTimesUpdated, this);
            this._waitingToResetCurrentTime = false;            
        }
    },

    _capturingStarted: function(event)
    {
        this._startUpdatingCurrentTime();
    },

    _capturingStopped: function(event)
    {
        this._stopUpdatingCurrentTime();
    },

    _recordingTimesUpdated: function(event)
    {
        if (!this._waitingToResetCurrentTime)
            return;

        // Make the current time be the start time of the last added record. This is the best way
        // currently to jump to the right period of time after recording starts.
        // FIXME: If no activity is happening we can sit for a while until a record is added.
        // We might want to have the backend send a "start" record to get current time moving.

        for (var timeline of this._recording.timelines.values()) {
            var lastRecord = timeline.records.lastValue;
            if (!lastRecord)
                continue;
            this._currentTime = Math.max(this._currentTime, lastRecord.startTime);
        }

        this._recording.removeEventListener(WebInspector.TimelineRecording.Event.TimesUpdated, this._recordingTimesUpdated, this);
        this._waitingToResetCurrentTime = false;
    },

    _clearTimeline: function(event)
    {
        this._recording.reset();
    },

    _recordingReset: function(event)
    {
        this._currentTime = NaN;

        if (!this._updating) {
            // Force the time ruler and views to reset to 0.
            this._startTimeNeedsReset = true;
            this._updateTimes(0, 0, 0);
        }

        this._lastUpdateTimestamp = NaN;
        this._startTimeNeedsReset = true;

        this._recording.removeEventListener(WebInspector.TimelineRecording.Event.TimesUpdated, this._recordingTimesUpdated, this);
        this._waitingToResetCurrentTime = false;

        this._overviewTimelineView.reset();
        for (var timelineView of this._discreteTimelineViewMap.values())
            timelineView.reset();

        for (var timelineOverviewGraph of this._discreteTimelineOverviewGraphMap.values())
            timelineOverviewGraph.reset();
    },

    _recordingUnloaded: function(event)
    {
        console.assert(!this._updating);

        WebInspector.timelineManager.removeEventListener(WebInspector.TimelineManager.Event.CapturingStarted, this._capturingStarted, this);
        WebInspector.timelineManager.removeEventListener(WebInspector.TimelineManager.Event.CapturingStopped, this._capturingStopped, this);
    },

    _timeRangeSelectionChanged: function(event)
    {
        this._currentTimelineView.startTime = this._timelineOverview.selectionStartTime;
        this._currentTimelineView.endTime = this._timelineOverview.selectionStartTime + this._timelineOverview.selectionDuration;

        // Delay until the next frame to stay in sync with the current timeline view's time-based layout changes.
        requestAnimationFrame(function() {
            var selectedTreeElement = this._currentTimelineView && this._currentTimelineView.navigationSidebarTreeOutline ? this._currentTimelineView.navigationSidebarTreeOutline.selectedTreeElement : null;
            var selectionWasHidden = selectedTreeElement && selectedTreeElement.hidden;

            WebInspector.timelineSidebarPanel.updateFilter();

            if (selectedTreeElement && selectedTreeElement.hidden !== selectionWasHidden)
                this.dispatchEventToListeners(WebInspector.ContentView.Event.SelectionPathComponentsDidChange);
        }.bind(this));
    }
};
