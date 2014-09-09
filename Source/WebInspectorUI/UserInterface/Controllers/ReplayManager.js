/*
 * Copyright (C) 2013 University of Washington. All rights reserved.
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

WebInspector.ReplayManager = function()
{
    WebInspector.Object.call(this);

    this._sessionState = WebInspector.ReplayManager.SessionState.Inactive;
    this._segmentState = WebInspector.ReplayManager.SegmentState.Unloaded;

    this._activeSessionIdentifier = null;
    this._activeSegmentIdentifier = null;
    this._currentPosition = new WebInspector.ReplayPosition(0, 0);
    this._initialized = false;

    // These hold actual instances of sessions and segments.
    this._sessions = new Map;
    this._segments = new Map;
    // These hold promises that resolve when the instance data is recieved.
    this._sessionPromises = new Map;
    this._segmentPromises = new Map;

    // Playback speed is specified in replayToPosition commands, and persists
    // for the duration of the playback command until another playback begins.
    this._playbackSpeed = WebInspector.ReplayManager.PlaybackSpeed.RealTime;

    if (!window.ReplayAgent)
        return;

    var instance = this;

    this._initializationPromise = ReplayAgent.currentReplayState.promise()
        .then(function(payload) {
            console.assert(payload.sessionState in WebInspector.ReplayManager.SessionState, "Unknown session state: " + payload.sessionState);
            console.assert(payload.segmentState in WebInspector.ReplayManager.SegmentState, "Unknown segment state: " + payload.segmentState);

            instance._activeSessionIdentifier = payload.sessionIdentifier;
            instance._activeSegmentIdentifier = payload.segmentIdentifier;
            instance._sessionState = WebInspector.ReplayManager.SessionState[payload.sessionState];
            instance._segmentState = WebInspector.ReplayManager.SegmentState[payload.segmentState];
            instance._currentPosition = payload.replayPosition;

            instance._initialized = true;
        }).then(function() {
            return ReplayAgent.getAvailableSessions.promise();
        }).then(function(payload) {
            for (var sessionId of payload.ids)
                instance.sessionCreated(sessionId);
        }).catch(function(err) {
            console.error("ReplayManager initialization failed: ", err);
        });
};

WebInspector.ReplayManager.Event = {
    CaptureStarted: "replay-manager-capture-started",
    CaptureStopped: "replay-manager-capture-stopped",

    PlaybackStarted: "replay-manager-playback-started",
    PlaybackPaused: "replay-manager-playback-paused",
    PlaybackFinished: "replay-manager-playback-finished",
    PlaybackPositionChanged: "replay-manager-play-back-position-changed",

    ActiveSessionChanged: "replay-manager-active-session-changed",
    ActiveSegmentChanged: "replay-manager-active-segment-changed",

    SessionSegmentAdded: "replay-manager-session-segment-added",
    SessionSegmentRemoved: "replay-manager-session-segment-removed",

    SessionAdded: "replay-manager-session-added",
    SessionRemoved: "replay-manager-session-removed",
};

WebInspector.ReplayManager.SessionState = {
    Capturing: "replay-manager-session-state-capturing",
    Inactive: "replay-manager-session-state-inactive",
    Replaying: "replay-manager-session-state-replaying",
};

WebInspector.ReplayManager.SegmentState = {
    Appending: "replay-manager-segment-state-appending",
    Unloaded: "replay-manager-segment-state-unloaded",
    Loaded: "replay-manager-segment-state-loaded",
    Dispatching: "replay-manager-segment-state-dispatching",
};

WebInspector.ReplayManager.PlaybackSpeed = {
    RealTime: "replay-manager-playback-speed-real-time",
    FastForward: "replay-manager-playback-speed-fast-forward",
};

WebInspector.ReplayManager.prototype = {
    constructor: WebInspector.ReplayManager,
    __proto__: WebInspector.Object.prototype,

    // Public

    // The following state is invalid unless called from a function that's chained
    // to the (resolved) ReplayManager.waitUntilInitialized promise.
    get sessionState()
    {
        console.assert(this._initialized);
        return this._sessionState;
    },

    get segmentState()
    {
        console.assert(this._initialized);
        return this._segmentState;
    },

    get activeSessionIdentifier()
    {
        console.assert(this._initialized);
        return this._activeSessionIdentifier;
    },

    get activeSegmentIdentifier()
    {
        console.assert(this._initialized);
        return this._activeSegmentIdentifier;
    },

    get playbackSpeed()
    {
        console.assert(this._initialized);
        return this._playbackSpeed;
    },

    set playbackSpeed(value)
    {
        console.assert(this._initialized);
        this._playbackSpeed = value;
    },

    get currentPosition()
    {
        console.assert(this._initialized);
        return this._currentPosition;
    },

    // These return promises even if the relevant instance is already created.
    waitUntilInitialized: function()
    {
        return this._initializationPromise;
    },

    getSession: function(sessionId)
    {
        if (this._sessionPromises.has(sessionId))
            return this._sessionPromises.get(sessionId);

        var newPromise = ReplayAgent.getSessionData.promise(sessionId)
            .then(function(payload) {
                return Promise.resolve(WebInspector.ReplaySession.fromPayload(sessionId, payload));
            });

        this._sessionPromises.set(sessionId, newPromise);
        return newPromise;
    },

    getSegment: function(segmentId)
    {
        if (this._segmentPromises.has(segmentId))
            return this._segmentPromises.get(segmentId);

        var newPromise = ReplayAgent.getSegmentData.promise(segmentId)
            .then(function(payload) {
                return Promise.resolve(new WebInspector.ReplaySessionSegment(segmentId, payload));
            });

        this._segmentPromises.set(segmentId, newPromise);
        return newPromise;
    },

    // Protected (called by ReplayObserver)

    // Since these methods update session and segment state, they depend on the manager
    // being properly initialized. So, each function body is prepended with a retry guard.
    // This makes call sites simpler and avoids an extra event loop turn in the common case.

    captureStarted: function()
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.captureStarted.bind(this));

        this._changeSessionState(WebInspector.ReplayManager.SessionState.Capturing);

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.CaptureStarted);
    },

    captureStopped: function()
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.captureStopped.bind(this));

        this._changeSessionState(WebInspector.ReplayManager.SessionState.Inactive);
        this._changeSegmentState(WebInspector.ReplayManager.SegmentState.Unloaded);

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.CaptureStopped);
    },

    playbackStarted: function()
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.playbackStarted.bind(this));

        if (this.sessionState === WebInspector.ReplayManager.SessionState.Inactive)
            this._changeSessionState(WebInspector.ReplayManager.SessionState.Replaying);

        this._changeSegmentState(WebInspector.ReplayManager.SegmentState.Dispatching);

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.PlaybackStarted);
    },

    playbackHitPosition: function(replayPosition, timestamp)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.playbackHitPosition.bind(this, replayPosition, timestamp));

        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Replaying);
        console.assert(this.segmentState === WebInspector.ReplayManager.SegmentState.Dispatching);
        console.assert(replayPosition instanceof WebInspector.ReplayPosition);

        this._currentPosition = replayPosition;
        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.PlaybackPositionChanged);
    },

    playbackPaused: function(position)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.playbackPaused.bind(this, position));

        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Replaying);
        this._changeSegmentState(WebInspector.ReplayManager.SegmentState.Loaded);

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.PlaybackPaused);
    },

    playbackFinished: function()
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.playbackFinished.bind(this));

        this._changeSessionState(WebInspector.ReplayManager.SessionState.Inactive);
        console.assert(this.segmentState === WebInspector.ReplayManager.SegmentState.Unloaded);

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.PlaybackFinished);
    },

    sessionCreated: function(sessionId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.sessionCreated.bind(this, sessionId));

        console.assert(!this._sessions.has(sessionId), "Tried to add duplicate session identifier:", sessionId);
        var sessionMap = this._sessions;
        this.getSession(sessionId)
            .then(function(session) {
                sessionMap.set(sessionId, session);
            }).catch(function(error) {
                console.error("Error obtaining session data: ", error);
            });

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.SessionAdded, {sessionId: sessionId});
    },

    sessionModified: function(sessionId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.sessionModified.bind(this, sessionId));

        this.getSession(sessionId).then(function(session) {
            session.segmentsChanged();
        });
    },

    sessionRemoved: function(sessionId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.sessionRemoved.bind(this, sessionId));

        console.assert(this._sessions.has(sessionId), "Unknown session identifier:", sessionId);

        if (!this._sessionPromises.has(sessionId))
            return;

        var manager = this;

        // Wait for any outstanding promise to settle so it doesn't get re-added.
        this.getSession(sessionId)
            .catch(function(error) {
                return Promise.resolve();
            }).then(function() {
                manager._sessionPromises.delete(sessionId);
                var removedSession = manager._sessions.take(sessionId);
                console.assert(removedSession);
                manager.dispatchEventToListeners(WebInspector.ReplayManager.Event.SessionRemoved, {removedSession: removedSession});
            });
    },

    segmentCreated: function(segmentId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.segmentCreated.bind(this, segmentId));

        console.assert(!this._segments.has(segmentId), "Tried to add duplicate segment identifier:", segmentId);

        this._changeSegmentState(WebInspector.ReplayManager.SegmentState.Appending);

        // Create a dummy segment, and don't try to load any data for it. It will
        // be removed once the segment is complete, and then its data will be fetched.
        var incompleteSegment = new WebInspector.IncompleteSessionSegment(segmentId);
        this._segments.set(segmentId, incompleteSegment);
        this._segmentPromises.set(segmentId, Promise.resolve(incompleteSegment));

        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.SessionSegmentAdded, {segmentIdentifier: segmentId});
    },

    segmentCompleted: function(segmentId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.segmentCompleted.bind(this, segmentId));

        var placeholderSegment = this._segments.take(segmentId);
        console.assert(placeholderSegment instanceof WebInspector.IncompleteSessionSegment);
        this._segmentPromises.delete(segmentId);

        var segmentMap = this._segments;
        this.getSegment(segmentId)
            .then(function(segment) {
                segmentMap.set(segmentId, segment);
            }).catch(function(error) {
                console.error("Error obtaining segment data: ", error);
            });
    },

    segmentRemoved: function(segmentId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.segmentRemoved.bind(this, segmentId));

        console.assert(this._segments.has(segmentId), "Unknown segment identifier:", segmentId);

        if (!this._segmentPromises.has(segmentId))
            return;

        var manager = this;

        // Wait for any outstanding promise to settle so it doesn't get re-added.
        this.getSegment(segmentId)
            .catch(function(error) {
                return Promise.resolve();
            }).then(function() {
                manager._segmentPromises.delete(segmentId);
                var removedSegment = manager._segments.take(segmentId);
                console.assert(removedSegment);
                manager.dispatchEventToListeners(WebInspector.ReplayManager.Event.SessionSegmentRemoved, {removedSegment: removedSegment});
            });
    },

    segmentLoaded: function(segmentId)
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.segmentLoaded.bind(this, segmentId));

        console.assert(this._segments.has(segmentId), "Unknown segment identifier:", segmentId);

        console.assert(this.sessionState !== WebInspector.ReplayManager.SessionState.Capturing);
        this._changeSegmentState(WebInspector.ReplayManager.SegmentState.Loaded);

        var previousIdentifier = this._activeSegmentIdentifier;
        this._activeSegmentIdentifier = segmentId;
        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.ActiveSegmentChanged, {previousSegmentIdentifier: previousIdentifier});
   },

    segmentUnloaded: function()
    {
        if (!this._initialized)
            return this.waitUntilInitialized().then(this.segmentUnloaded.bind(this));

        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Replaying);
        this._changeSegmentState(WebInspector.ReplayManager.SegmentState.Unloaded);

        var previousIdentifier = this._activeSegmentIdentifier;
        this._activeSegmentIdentifier = null;
        this.dispatchEventToListeners(WebInspector.ReplayManager.Event.ActiveSegmentChanged, {previousSegmentIdentifier: previousIdentifier});
    },

    // Private

    // FIXME: these methods should be private and only accessed by implementations of the public API,
    // which should properly chain async calls leading up to these low-level backend calls. Otherwise,
    // their preconditions on session and segment state are too strong for scenarios triggered by the UI.

    startCapturing: function()
    {
        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Inactive);
        console.assert(this.segmentState === WebInspector.ReplayManager.SegmentState.Unloaded);

        ReplayAgent.startCapturing();
    },

    stopCapturing: function()
    {
        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Capturing);
        console.assert(this.segmentState === WebInspector.ReplayManager.SegmentState.Appending);

        ReplayAgent.stopCapturing();
    },

    replayToMarkIndex: function(replayPosition)
    {
        console.assert(replayPosition instanceof WebInspector.ReplayPosition);

        console.assert(this.sessionState !== WebInspector.ReplayManager.SessionState.Capturing);
        // FIXME: Once the public API is asynchronous, we should assert that segmentState is Loaded.

        ReplayAgent.replayToPosition(replayPosition, this.playbackSpeed === WebInspector.ReplayManager.PlaybackSpeed.FastForward);
    },

    replayToCompletion: function()
    {
        console.assert(this.sessionState !== WebInspector.ReplayManager.SessionState.Capturing);
        // FIXME: Once the public API is asynchronous, we should assert that segmentState is Loaded.

        ReplayAgent.replayToCompletion(this.playbackSpeed === WebInspector.ReplayManager.PlaybackSpeed.FastForward);
    },

    pausePlayback: function()
    {
        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Replaying);
        console.assert(this.segmentState === WebInspector.ReplayManager.SegmentState.Dispatching);

        ReplayAgent.pausePlayback();
    },

    stopPlayback: function()
    {
        console.assert(this.sessionState === WebInspector.ReplayManager.SessionState.Replaying);
        console.assert(this.segmentState === WebInspector.ReplayManager.SegmentState.Loaded);

        ReplayAgent.stopPlayback();
    },

    // Private

    _changeSessionState: function(newState)
    {
        // Warn about no-op state changes. We shouldn't be seeing them.
        var isAllowed = this._sessionState !== newState;

        switch (this._sessionState) {
        case WebInspector.ReplayManager.SessionState.Capturing:
            isAllowed &= newState === WebInspector.ReplayManager.SessionState.Inactive;
            break;

        case WebInspector.ReplayManager.SessionState.Replaying:
            isAllowed &= newState === WebInspector.ReplayManager.SessionState.Inactive;
            break;
        }

        console.assert(isAllowed, "Invalid session state change: ", this._sessionState, " to ", newState);
        if (isAllowed)
            this._sessionState = newState;
    },

    _changeSegmentState: function(newState)
    {
        // Warn about no-op state changes. We shouldn't be seeing them.
        var isAllowed = this._segmentState !== newState;

        switch (this._segmentState) {
            case WebInspector.ReplayManager.SegmentState.Appending:
                isAllowed &= newState === WebInspector.ReplayManager.SegmentState.Unloaded;
                break;
            case WebInspector.ReplayManager.SegmentState.Unloaded:
                isAllowed &= newState === WebInspector.ReplayManager.SegmentState.Appending || newState === WebInspector.ReplayManager.SegmentState.Loaded;
                break;
            case WebInspector.ReplayManager.SegmentState.Loaded:
                isAllowed &= newState === WebInspector.ReplayManager.SegmentState.Unloaded || newState === WebInspector.ReplayManager.SegmentState.Dispatching;
                break;
            case WebInspector.ReplayManager.SegmentState.Dispatching:
                isAllowed &= newState === WebInspector.ReplayManager.SegmentState.Loaded;
                break;
        }

        console.assert(isAllowed, "Invalid segment state change: ", this._segmentState, " to ", newState);
        if (isAllowed)
            this._segmentState = newState;
    }
};

