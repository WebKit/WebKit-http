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

WebInspector.ScriptTimelineDataGridNode = function(scriptTimelineRecord, baseStartTime, rangeStartTime, rangeEndTime)
{
    WebInspector.TimelineDataGridNode.call(this, false, null);

    this._record = scriptTimelineRecord;
    this._baseStartTime = baseStartTime || 0;
    this._rangeStartTime = rangeStartTime || 0;
    this._rangeEndTime = typeof rangeEndTime === "number" ? rangeEndTime : Infinity;
};

WebInspector.Object.addConstructorFunctions(WebInspector.ScriptTimelineDataGridNode);

WebInspector.ScriptTimelineDataGridNode.IconStyleClassName = "icon";

WebInspector.ScriptTimelineDataGridNode.prototype = {
    constructor: WebInspector.ScriptTimelineDataGridNode,
    __proto__: WebInspector.TimelineDataGridNode.prototype,

    // Public

    get record()
    {
        return this._record;
    },

    get records()
    {
        return [this._record];
    },

    get baseStartTime()
    {
        return this._baseStartTime;
    },

    get rangeStartTime()
    {
        return this._rangeStartTime;
    },

    set rangeStartTime(x)
    {
        if (this._rangeStartTime === x)
            return;

        this._rangeStartTime = x;
        this.needsRefresh();
    },

    get rangeEndTime()
    {
        return this._rangeEndTime;
    },

    set rangeEndTime(x)
    {
        if (this._rangeEndTime === x)
            return;

        this._rangeEndTime = x;
        this.needsRefresh();
    },

    get data()
    {
        var startTime = Math.max(this._rangeStartTime, this._record.startTime);
        var duration = Math.min(this._record.startTime + this._record.duration, this._rangeEndTime) - startTime;
        var callFrameOrSourceCodeLocation = this._record.initiatorCallFrame || this._record.sourceCodeLocation;

        return {eventType: this._record.eventType, startTime: startTime, selfTime: duration, totalTime: duration,
            averageTime: duration, callCount: 1, location: callFrameOrSourceCodeLocation};
    },

    createCellContent: function(columnIdentifier, cell)
    {
        const emptyValuePlaceholderString = "\u2014";
        var value = this.data[columnIdentifier];

        switch (columnIdentifier) {
        case "eventType":
            return WebInspector.ScriptTimelineRecord.EventType.displayName(value, this._record.details);

        case "startTime":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value - this._baseStartTime, true);

        case "selfTime":
        case "totalTime":
        case "averageTime":
            return isNaN(value) ? emptyValuePlaceholderString : Number.secondsToString(value, true);
        }

        return WebInspector.TimelineDataGridNode.prototype.createCellContent.call(this, columnIdentifier, cell);
    }
};
