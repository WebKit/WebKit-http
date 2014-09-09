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

WebInspector.LegacyProfileManager = function()
{
    WebInspector.Object.call(this);

    this._javaScriptProfileType = new WebInspector.LegacyJavaScriptProfileType;

    if (window.ProfilerAgent) {
        ProfilerAgent.enable();
        ProfilerAgent.getProfileHeaders();
    }

    WebInspector.Frame.addEventListener(WebInspector.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

    this.initialize();
};

WebInspector.LegacyProfileManager.Event = {
    ProfileWasAdded: "profile-manager-profile-was-added",
    ProfileWasUpdated: "profile-manager-profile-was-updated",
    ProfilingStarted: "profile-manager-profiling-started",
    ProfilingEnded: "profile-manager-profiling-ended",
    ProfilingInterrupted: "profile-manager-profiling-interrupted",
    Cleared: "profile-manager-cleared"
};

WebInspector.LegacyProfileManager.UserInitiatedProfileName = "org.webkit.profiles.user-initiated";

WebInspector.LegacyProfileManager.prototype = {
    constructor: WebInspector.LegacyProfileManager,

    // Public

    initialize: function()
    {
        this._checkForInterruptions();

        this._recordingJavaScriptProfile = null;

        this._isProfiling = false;

        this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.Cleared);
    },

    isProfilingJavaScript: function()
    {
        return this._javaScriptProfileType.isRecordingProfile();
    },

    startProfilingJavaScript: function()
    {
        this._javaScriptProfileType.startRecordingProfile();
    },

    stopProfilingJavaScript: function()
    {
        this._javaScriptProfileType.stopRecordingProfile();
    },

    profileWasStartedFromConsole: function(title)
    {
        this.setRecordingJavaScriptProfile(true, true);

        if (title.indexOf(WebInspector.LegacyProfileManager.UserInitiatedProfileName) === -1) {
            this._recordingJavaScriptProfile.title = title;
            this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.ProfileWasUpdated, {profile: this._recordingJavaScriptProfile});
        }
    },

    profileWasEndedFromConsole: function()
    {
        this.setRecordingJavaScriptProfile(false, true);
    },

    addJavaScriptProfile: function(profile)
    {
        console.assert(this._recordingJavaScriptProfile);
        if (!this._recordingJavaScriptProfile)
            return;

        this._recordingJavaScriptProfile.type = profile.typeId;
        this._recordingJavaScriptProfile.title = profile.title;
        this._recordingJavaScriptProfile.id = profile.uid;
        this._recordingJavaScriptProfile.recording = false;

        this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.ProfileWasUpdated, {profile: this._recordingJavaScriptProfile});

        // We want to reset _recordingJavaScriptProfile so that we can identify
        // interruptions, but we also want to keep track of the last profile
        // we've recorded so that we can provide it as data to the ProfilingEnded event
        // we'll dispatch in setRecordingJavaScriptProfile().
        this._lastJavaScriptProfileAdded = this._recordingJavaScriptProfile;
        this._recordingJavaScriptProfile = null;
    },

    setRecordingJavaScriptProfile: function(isProfiling, fromConsole)
    {
        if (this._isProfiling === isProfiling)
            return;

        this._isProfiling = isProfiling;

        // We've interrupted the current JS profile due to a page reload. Return
        // now and _attemptToResumeProfiling will pick things up after the reload.
        if (!isProfiling && !!this._recordingJavaScriptProfile)
            return;

        if (isProfiling && !this._recordingJavaScriptProfile)
            this._recordingJavaScriptProfile = new WebInspector.LegacyJavaScriptProfileObject(WebInspector.LegacyProfileManager.UserInitiatedProfileName, -1, true);

        if (isProfiling) {
            this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.ProfileWasAdded, {profile: this._recordingJavaScriptProfile});
            if (!fromConsole)
                this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.ProfilingStarted);
        } else {
            this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.ProfilingEnded, {
                profile: this._lastJavaScriptProfileAdded,
                fromConsole: fromConsole
            });
            this._lastJavaScriptProfileAdded = null;
        }
    },

    // Private
    
    _mainResourceDidChange: function(event)
    {
        console.assert(event.target instanceof WebInspector.Frame);

        if (!event.target.isMainFrame())
            return;

        var oldMainResource = event.data.oldMainResource;
        var newMainResource = event.target.mainResource;
        if (oldMainResource.url !== newMainResource.url)
            this.initialize();
        else
            this._attemptToResumeProfiling();
    },

    _checkForInterruptions: function()
    {
        if (this._recordingJavaScriptProfile) {
            this.dispatchEventToListeners(WebInspector.LegacyProfileManager.Event.ProfilingInterrupted, {profile: this._recordingJavaScriptProfile});
            this._javaScriptProfileType.setRecordingProfile(false);
        }
    },

    _attemptToResumeProfiling: function()
    {
        this._checkForInterruptions();

        if (this._recordingJavaScriptProfile)
            this.startProfilingJavaScript();
    }
};

WebInspector.LegacyProfileManager.prototype.__proto__ = WebInspector.Object.prototype;
