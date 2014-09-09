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

WebInspector.TimelineRecording = function(identifier, displayName)
{
    WebInspector.Object.call(this);

    this._identifier = identifier;
    this._timelines = new Map;
    this._displayName = displayName;
    this._isWritable = true;

    for (var key of Object.keys(WebInspector.TimelineRecord.Type)) {
        var type = WebInspector.TimelineRecord.Type[key];
        var timeline = new WebInspector.Timeline(type);
        this._timelines.set(type, timeline);
        timeline.addEventListener(WebInspector.Timeline.Event.TimesUpdated, this._timelineTimesUpdated, this);
    }

    this.reset(true);
};

WebInspector.TimelineRecording.Event = {
    Reset: "timeline-recording-reset",
    Unloaded: "timeline-recording-unloaded",
    SourceCodeTimelineAdded: "timeline-recording-source-code-timeline-added",
    TimesUpdated: "timeline-recording-times-updated"
};

WebInspector.TimelineRecording.prototype = {
    constructor: WebInspector.TimelineRecording,
    __proto__: WebInspector.Object.prototype,

    // Public

    get displayName()
    {
        return this._displayName;
    },

    get identifier()
    {
        return this._identifier;
    },

    get timelines()
    {
        return this._timelines;
    },

    get startTime()
    {
        return this._startTime;
    },

    get endTime()
    {
        return this._endTime;
    },

    saveIdentityToCookie: function()
    {
        // Do nothing. Timeline recordings are not persisted when the inspector is
        // re-opened, so do not attempt to restore by identifier or display name.
    },

    isWritable: function()
    {
        return this._isWritable;
    },

    isEmpty: function()
    {
        for (var timeline of this._timelines.values()) {
            if (timeline.records.length)
                return false;
        }

        return true;
    },

    unloaded: function()
    {
        console.assert(!this.isEmpty(), "Shouldn't unload an empty recording; it should be reused instead.");

        this._isWritable = false;

        this.dispatchEventToListeners(WebInspector.TimelineRecording.Event.Unloaded);
    },

    reset: function(suppressEvents)
    {
        console.assert(this._isWritable, "Can't reset a read-only recording.");

        this._sourceCodeTimelinesMap = new Map;
        this._eventMarkers = [];
        this._startTime = NaN;
        this._endTime = NaN;

        for (var timeline of this._timelines.values())
            timeline.reset(suppressEvents);

        if (!suppressEvents) {
            this.dispatchEventToListeners(WebInspector.TimelineRecording.Event.Reset);
            this.dispatchEventToListeners(WebInspector.TimelineRecording.Event.TimesUpdated);
        }
    },

    sourceCodeTimelinesForSourceCode: function(sourceCode)
    {
        var timelines = this._sourceCodeTimelinesMap.get(sourceCode);
        if (!timelines)
            return [];
        return timelines.values();
    },

    addEventMarker: function(eventMarker)
    {
        this._eventMarkers.push(eventMarker);
    },

    addRecord: function(record)
    {
        // Add the record to the global timeline by type.
        this._timelines.get(record.type).addRecord(record);

        // Network records don't have source code timelines.
        if (record.type === WebInspector.TimelineRecord.Type.Network)
            return;

        // Add the record to the source code timelines.
        var activeMainResource = WebInspector.frameResourceManager.mainFrame.provisionalMainResource || WebInspector.frameResourceManager.mainFrame.mainResource;
        var sourceCode = record.sourceCodeLocation ? record.sourceCodeLocation.sourceCode : activeMainResource;

        var sourceCodeTimelines = this._sourceCodeTimelinesMap.get(sourceCode);
        if (!sourceCodeTimelines) {
            sourceCodeTimelines = new Map;
            this._sourceCodeTimelinesMap.set(sourceCode, sourceCodeTimelines);
        }

        var newTimeline = false;
        var key = this._keyForRecord(record);
        var sourceCodeTimeline = sourceCodeTimelines.get(key);
        if (!sourceCodeTimeline) {
            sourceCodeTimeline = new WebInspector.SourceCodeTimeline(sourceCode, record.sourceCodeLocation, record.type, record.eventType);
            sourceCodeTimelines.set(key, sourceCodeTimeline);
            newTimeline = true;
        }

        sourceCodeTimeline.addRecord(record);

        if (newTimeline)
            this.dispatchEventToListeners(WebInspector.TimelineRecording.Event.SourceCodeTimelineAdded, {sourceCodeTimeline: sourceCodeTimeline});
    },

    // Private

    _keyForRecord: function(record)
    {
        var key = record.type;
        if (record instanceof WebInspector.ScriptTimelineRecord || record instanceof WebInspector.LayoutTimelineRecord)
            key += ":" + record.eventType;
        if (record instanceof WebInspector.ScriptTimelineRecord && record.eventType === WebInspector.ScriptTimelineRecord.EventType.EventDispatched)
            key += ":" + record.details;
        if (record.sourceCodeLocation)
            key += ":" + record.sourceCodeLocation.lineNumber + ":" + record.sourceCodeLocation.columnNumber;
        return key;
    },

    _timelineTimesUpdated: function(event)
    {
        var timeline = event.target;
        var changed = false;

        if (isNaN(this._startTime) || timeline.startTime < this._startTime) {
            this._startTime = timeline.startTime;
            changed = true;
        }

        if (isNaN(this._endTime) || this._endTime < timeline.endTime) {
            this._endTime = timeline.endTime;
            changed = true;
        }

        if (changed)
            this.dispatchEventToListeners(WebInspector.TimelineRecording.Event.TimesUpdated);
    }
};
