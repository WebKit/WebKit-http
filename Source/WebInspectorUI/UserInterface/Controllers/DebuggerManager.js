/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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

WebInspector.DebuggerManager = function()
{
    WebInspector.Object.call(this);

    if (window.DebuggerAgent)
        DebuggerAgent.enable();

    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisplayLocationDidChange, this._breakpointDisplayLocationDidChange, this);
    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.DisabledStateDidChange, this._breakpointDisabledStateDidChange, this);
    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.ConditionDidChange, this._breakpointEditablePropertyDidChange, this);
    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.AutoContinueDidChange, this._breakpointEditablePropertyDidChange, this);
    WebInspector.Breakpoint.addEventListener(WebInspector.Breakpoint.Event.ActionsDidChange, this._breakpointEditablePropertyDidChange, this);

    window.addEventListener("pagehide", this._inspectorClosing.bind(this));

    this._allExceptionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-all-exceptions", false);
    this._allUncaughtExceptionsBreakpointEnabledSetting = new WebInspector.Setting("break-on-all-uncaught-exceptions", false);

    var specialBreakpointLocation = new WebInspector.SourceCodeLocation(null, Infinity, Infinity);

    this._allExceptionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._allExceptionsBreakpointEnabledSetting.value);
    this._allExceptionsBreakpoint.resolved = true;

    this._allUncaughtExceptionsBreakpoint = new WebInspector.Breakpoint(specialBreakpointLocation, !this._allUncaughtExceptionsBreakpointEnabledSetting.value);

    this._breakpoints = [];
    this._breakpointURLMap = {};
    this._breakpointScriptIdentifierMap = {};
    this._breakpointIdMap = {};

    this._nextBreakpointActionIdentifier = 1;

    this._scriptIdMap = {};
    this._scriptURLMap = {};

    this._breakpointsSetting = new WebInspector.Setting("breakpoints", []);
    this._breakpointsEnabledSetting = new WebInspector.Setting("breakpoints-enabled", true);

    if (window.DebuggerAgent)
        DebuggerAgent.setBreakpointsActive(this._breakpointsEnabledSetting.value);

    this._updateBreakOnExceptionsState();

    function restoreBreakpointsSoon() {
        this._restoringBreakpoints = true;
        for (var cookie of this._breakpointsSetting.value)
            this.addBreakpoint(new WebInspector.Breakpoint(cookie));
        delete this._restoringBreakpoints;
    }

    // Ensure that all managers learn about restored breakpoints,
    // regardless of their initialization order.
    setTimeout(restoreBreakpointsSoon.bind(this), 0);
};

WebInspector.DebuggerManager.Event = {
    BreakpointAdded: "debugger-manager-breakpoint-added",
    BreakpointRemoved: "debugger-manager-breakpoint-removed",
    BreakpointMoved: "debugger-manager-breakpoint-moved",
    Paused: "debugger-manager-paused",
    Resumed: "debugger-manager-resumed",
    CallFramesDidChange: "debugger-manager-call-frames-did-change",
    ActiveCallFrameDidChange: "debugger-manager-active-call-frame-did-change",
    ScriptAdded: "debugger-manager-script-added",
    ScriptsCleared: "debugger-manager-scripts-cleared",
    BreakpointsEnabledDidChange: "debugger-manager-breakpoints-enabled-did-change"
};

WebInspector.DebuggerManager.prototype = {
    constructor: WebInspector.DebuggerManager,

    // Public

    get breakpointsEnabled()
    {
        return this._breakpointsEnabledSetting.value;
    },

    set breakpointsEnabled(enabled)
    {
        if (this._breakpointsEnabledSetting.value === enabled)
            return;

        this._breakpointsEnabledSetting.value = enabled;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointsEnabledDidChange);

        DebuggerAgent.setBreakpointsActive(enabled);

        this._updateBreakOnExceptionsState();
    },

    get paused()
    {
        return this._paused;
    },

    get callFrames()
    {
        return this._callFrames;
    },

    get activeCallFrame()
    {
        return this._activeCallFrame;
    },

    set activeCallFrame(callFrame)
    {
        if (callFrame === this._activeCallFrame)
            return;

        this._activeCallFrame = callFrame || null;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    },

    pause: function()
    {
        if (this._paused)
            return Promise.resolve();

        var listener = new WebInspector.EventListener(this, true);

        var managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.Paused, resolve);
        });

        var protocolResult = DebuggerAgent.pause()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.pause failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    },

    resume: function()
    {
        if (!this._paused)
            return Promise.resolve();

        var listener = new WebInspector.EventListener(this, true);

        var managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.Resumed, resolve);
        });

        var protocolResult = DebuggerAgent.resume()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.resume failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    },

    stepOver: function()
    {
        if (!this._paused)
            return Promise.reject(new Error("Cannot step over because debugger is not paused."));

        var listener = new WebInspector.EventListener(this, true);

        var managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        var protocolResult = DebuggerAgent.stepOver()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepOver failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    },

    stepInto: function()
    {
        if (!this._paused)
            return Promise.reject(new Error("Cannot step into because debugger is not paused."));

        var listener = new WebInspector.EventListener(this, true);

        var managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        var protocolResult = DebuggerAgent.stepInto()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepInto failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    },

    stepOut: function()
    {
        if (!this._paused)
            return Promise.reject(new Error("Cannot step out because debugger is not paused."));

        var listener = new WebInspector.EventListener(this, true);

        var managerResult = new Promise(function(resolve, reject) {
            listener.connect(WebInspector.debuggerManager, WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        var protocolResult = DebuggerAgent.stepOut()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepOut failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    },

    get allExceptionsBreakpoint()
    {
        return this._allExceptionsBreakpoint;
    },

    get allUncaughtExceptionsBreakpoint()
    {
        return this._allUncaughtExceptionsBreakpoint;
    },

    get breakpoints()
    {
        return this._breakpoints;
    },

    breakpointsForSourceCode: function(sourceCode)
    {
        console.assert(sourceCode instanceof WebInspector.Resource || sourceCode instanceof WebInspector.Script);

        if (sourceCode instanceof WebInspector.SourceMapResource) {
            var mappedResourceBreakpoints = [];
            var originalSourceCodeBreakpoints = this.breakpointsForSourceCode(sourceCode.sourceMap.originalSourceCode);
            return originalSourceCodeBreakpoints.filter(function(breakpoint) {
                return breakpoint.sourceCodeLocation.displaySourceCode === sourceCode;
            });
        }

        if (sourceCode.url in this._breakpointURLMap) {
            var urlBreakpoint = this._breakpointURLMap[sourceCode.url] || [];
            this._associateBreakpointsWithSourceCode(urlBreakpoint, sourceCode);
            return urlBreakpoint;
        }

        if (sourceCode instanceof WebInspector.Script && sourceCode.id in this._breakpointScriptIdentifierMap) {
            var scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[sourceCode.id] || [];
            this._associateBreakpointsWithSourceCode(scriptIdentifierBreakpoints, sourceCode);
            return scriptIdentifierBreakpoints;
        }

        return [];
    },

    scriptForIdentifier: function(id)
    {
        return this._scriptIdMap[id] || null;
    },

    scriptsForURL: function(url)
    {
        // FIXME: This may not be safe. A Resource's URL may differ from a Script's URL.
        return this._scriptURLMap[url] || [];
    },

    continueToLocation: function(scriptIdentifier, lineNumber, columnNumber)
    {
        DebuggerAgent.continueToLocation({scriptId: scriptIdentifier, lineNumber: lineNumber, columnNumber: columnNumber});
    },

    addBreakpoint: function(breakpoint, skipEventDispatch, shouldSpeculativelyResolve)
    {
        console.assert(breakpoint instanceof WebInspector.Breakpoint, "Bad argument to DebuggerManger.addBreakpoint: ", breakpoint);
        if (!breakpoint)
            return;

        if (breakpoint.url) {
            var urlBreakpoints = this._breakpointURLMap[breakpoint.url];
            if (!urlBreakpoints)
                urlBreakpoints = this._breakpointURLMap[breakpoint.url] = [];
            urlBreakpoints.push(breakpoint);
        }

        if (breakpoint.scriptIdentifier) {
            var scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier];
            if (!scriptIdentifierBreakpoints)
                scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier] = [];
            scriptIdentifierBreakpoints.push(breakpoint);
        }

        this._breakpoints.push(breakpoint);

        function speculativelyResolveBreakpoint(breakpoint) {
            breakpoint.resolved = true;
        }

        if (!breakpoint.disabled)
            this._setBreakpoint(breakpoint, shouldSpeculativelyResolve ? speculativelyResolveBreakpoint.bind(null, breakpoint) : null);

        if (!skipEventDispatch)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointAdded, {breakpoint: breakpoint});
    },

    removeBreakpoint: function(breakpoint)
    {
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        console.assert(this.isBreakpointRemovable(breakpoint));
        if (!this.isBreakpointRemovable(breakpoint))
            return;

        this._breakpoints.remove(breakpoint);

        if (breakpoint.identifier)
            this._removeBreakpoint(breakpoint);

        if (breakpoint.url) {
            var urlBreakpoints = this._breakpointURLMap[breakpoint.url];
            if (urlBreakpoints) {
                urlBreakpoints.remove(breakpoint);
                if (!urlBreakpoints.length)
                    delete this._breakpointURLMap[breakpoint.url];
            }
        }

        if (breakpoint.scriptIdentifier) {
            var scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier];
            if (scriptIdentifierBreakpoints) {
                scriptIdentifierBreakpoints.remove(breakpoint);
                if (!scriptIdentifierBreakpoints.length)
                    delete this._breakpointScriptIdentifierMap[breakpoint.scriptIdentifier];
            }
        }

        // Disable the breakpoint first, so removing actions doesn't re-add the breakpoint.
        breakpoint.disabled = true;
        breakpoint.clearActions();

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointRemoved, {breakpoint: breakpoint});
    },

    breakpointResolved: function(breakpointIdentifier, location)
    {
        // Called from WebInspector.DebuggerObserver.

        var breakpoint = this._breakpointIdMap[breakpointIdentifier];
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        console.assert(breakpoint.identifier === breakpointIdentifier);

        if (!breakpoint.sourceCodeLocation.sourceCode) {
            var sourceCodeLocation = this._sourceCodeLocationFromPayload(location);
            breakpoint.sourceCodeLocation.sourceCode = sourceCodeLocation.sourceCode;
        }

        breakpoint.resolved = true;
    },

    reset: function()
    {
        // Called from WebInspector.DebuggerObserver.

        var wasPaused = this._paused;

        WebInspector.Script.resetUniqueDisplayNameNumbers();

        this._paused = false;
        this._scriptIdMap = {};
        this._scriptURLMap = {};

        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        // Mark all the breakpoints as unresolved. They will be reported as resolved when
        // breakpointResolved is called as the page loads.
        for (var i = 0; i < this._breakpoints.length; ++i) {
            var breakpoint = this._breakpoints[i];
            breakpoint.resolved = false;
            if (breakpoint.sourceCodeLocation.sourceCode)
                breakpoint.sourceCodeLocation.sourceCode = null;
        }

        delete this._ignoreBreakpointDisplayLocationDidChangeEvent;

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ScriptsCleared);

        if (wasPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);
    },

    debuggerDidPause: function(callFramesPayload)
    {
        // Called from WebInspector.DebuggerObserver.

        if (this._delayedResumeTimeout) {
            clearTimeout(this._delayedResumeTimeout);
            delete this._delayedResumeTimeout;
        }

        var wasStillPaused = this._paused;

        this._paused = true;
        this._callFrames = [];

        for (var i = 0; i < callFramesPayload.length; ++i) {
            var callFramePayload = callFramesPayload[i];
            var sourceCodeLocation = this._sourceCodeLocationFromPayload(callFramePayload.location);
            // FIXME: There may be useful call frames without a source code location (native callframes), should we include them?
            if (!sourceCodeLocation)
                continue;
            if (!sourceCodeLocation.sourceCode)
                continue;
            // Exclude the case where the call frame is in the inspector code.
            if (sourceCodeLocation.sourceCode.url && sourceCodeLocation.sourceCode.url.startsWith("__WebInspector"))
                continue;
            var thisObject = WebInspector.RemoteObject.fromPayload(callFramePayload.this);
            var scopeChain = this._scopeChainFromPayload(callFramePayload.scopeChain);
            var callFrame = new WebInspector.CallFrame(callFramePayload.callFrameId, sourceCodeLocation, callFramePayload.functionName, thisObject, scopeChain);
            this._callFrames.push(callFrame);
        }

        this._activeCallFrame = this._callFrames[0];

        if (!wasStillPaused)
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Paused);
        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.CallFramesDidChange);
        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
    },

    debuggerDidResume: function()
    {
        // Called from WebInspector.DebuggerObserver.

        function delayedWork()
        {
            delete this._delayedResumeTimeout;

            this._paused = false;
            this._callFrames = null;
            this._activeCallFrame = null;

            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.Resumed);
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.CallFramesDidChange);
            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ActiveCallFrameDidChange);
        }

        // We delay clearing the state and firing events so the user interface does not flash
        // between brief steps or successive breakpoints.
        this._delayedResumeTimeout = setTimeout(delayedWork.bind(this), 50);
    },

    playBreakpointActionSound: function(breakpointActionIdentifier)
    {
        InspectorFrontendHost.beep();
    },

    scriptDidParse: function(scriptIdentifier, url, isContentScript, startLine, startColumn, endLine, endColumn, sourceMapURL)
    {
        // Don't add the script again if it is already known.
        if (this._scriptIdMap[scriptIdentifier]) {
            console.assert(this._scriptIdMap[scriptIdentifier].url === (url || null));
            console.assert(this._scriptIdMap[scriptIdentifier].range.startLine === startLine);
            console.assert(this._scriptIdMap[scriptIdentifier].range.startColumn === startColumn);
            console.assert(this._scriptIdMap[scriptIdentifier].range.endLine === endLine);
            console.assert(this._scriptIdMap[scriptIdentifier].range.endColumn === endColumn);
            return;
        }

        var script = new WebInspector.Script(scriptIdentifier, new WebInspector.TextRange(startLine, startColumn, endLine, endColumn), url, isContentScript, sourceMapURL);

        this._scriptIdMap[scriptIdentifier] = script;

        if (script.url) {
            var scripts = this._scriptURLMap[script.url];
            if (!scripts)
                scripts = this._scriptURLMap[script.url] = [];
            scripts.push(script);
        }

        this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.ScriptAdded, {script: script});
    },

    isBreakpointRemovable: function(breakpoint)
    {
        return breakpoint !== this._allExceptionsBreakpoint && breakpoint !== this._allUncaughtExceptionsBreakpoint;
    },

    isBreakpointEditable: function(breakpoint)
    {
        return this.isBreakpointRemovable(breakpoint);
    },

    get nextBreakpointActionIdentifier()
    {
        return this._nextBreakpointActionIdentifier++;
    },

    // Private

    _sourceCodeLocationFromPayload: function(payload)
    {
        var script = this._scriptIdMap[payload.scriptId];
        console.assert(script);
        if (!script)
            return null;

        return script.createSourceCodeLocation(payload.lineNumber, payload.columnNumber);
    },

    _scopeChainFromPayload: function(payload)
    {
        var scopeChain = [];
        for (var i = 0; i < payload.length; ++i)
            scopeChain.push(this._scopeChainNodeFromPayload(payload[i]));
        return scopeChain;
    },

    _scopeChainNodeFromPayload: function(payload)
    {
        var type = null;
        switch (payload.type) {
        case "local":
            type = WebInspector.ScopeChainNode.Type.Local;
            break;
        case "global":
            type = WebInspector.ScopeChainNode.Type.Global;
            break;
        case "with":
            type = WebInspector.ScopeChainNode.Type.With;
            break;
        case "closure":
            type = WebInspector.ScopeChainNode.Type.Closure;
            break;
        case "catch":
            type = WebInspector.ScopeChainNode.Type.Catch;
            break;
        default:
            console.error("Unknown type: " + payload.type);
        }

        var object = WebInspector.RemoteObject.fromPayload(payload.object);
        return new WebInspector.ScopeChainNode(type, object);
    },

    _debuggerBreakpointActionType: function(type)
    {
        switch (type) {
        case WebInspector.BreakpointAction.Type.Log:
            return DebuggerAgent.BreakpointActionType.Log;
        case WebInspector.BreakpointAction.Type.Evaluate:
            return DebuggerAgent.BreakpointActionType.Evaluate;
        case WebInspector.BreakpointAction.Type.Sound:
            return DebuggerAgent.BreakpointActionType.Sound;
        case WebInspector.BreakpointAction.Type.Probe:
            return DebuggerAgent.BreakpointActionType.Probe;
        default:
            console.assert(false);
            return DebuggerAgent.BreakpointActionType.Log;
        }
    },

    _setBreakpoint: function(breakpoint, callback)
    {
        console.assert(!breakpoint.identifier);
        console.assert(!breakpoint.disabled);

        if (breakpoint.identifier || breakpoint.disabled)
            return;

        if (!this._restoringBreakpoints) {
            // Enable breakpoints since a breakpoint is being set. This eliminates
            // a multi-step process for the user that can be confusing.
            this.breakpointsEnabled = true;
        }

        function didSetBreakpoint(error, breakpointIdentifier, locations)
        {
            if (error)
                return;

            this._breakpointIdMap[breakpointIdentifier] = breakpoint;

            breakpoint.identifier = breakpointIdentifier;

            // Debugger.setBreakpoint returns a single location.
            if (!(locations instanceof Array))
                locations = [locations];

            for (var location of locations)
                this.breakpointResolved(breakpointIdentifier, location);

            if (typeof callback === "function")
                callback();
        }

        // The breakpoint will be resolved again by calling DebuggerAgent, so mark it as unresolved.
        // If something goes wrong it will stay unresolved and show up as such in the user interface.
        breakpoint.resolved = false;

        // Convert BreakpointAction types to DebuggerAgent protocol types.
        // NOTE: Breakpoint.options returns new objects each time, so it is safe to modify.
        var options;
        if (DebuggerAgent.BreakpointActionType) {
            options = breakpoint.options;
            if (options.actions.length) {
                for (var i = 0; i < options.actions.length; ++i)
                    options.actions[i].type = this._debuggerBreakpointActionType(options.actions[i].type);
            }
        }

        // COMPATIBILITY (iOS 7): iOS 7 and earlier, DebuggerAgent.setBreakpoint* took a "condition" string argument.
        // This has been replaced with an "options" BreakpointOptions object.
        if (breakpoint.url) {
            DebuggerAgent.setBreakpointByUrl.invoke({
                lineNumber: breakpoint.sourceCodeLocation.lineNumber,
                url: breakpoint.url,
                urlRegex: undefined,
                columnNumber: breakpoint.sourceCodeLocation.columnNumber,
                condition: breakpoint.condition,
                options: options
            }, didSetBreakpoint.bind(this));
        } else if (breakpoint.scriptIdentifier) {
            DebuggerAgent.setBreakpoint.invoke({
                location: {scriptId: breakpoint.scriptIdentifier, lineNumber: breakpoint.sourceCodeLocation.lineNumber, columnNumber: breakpoint.sourceCodeLocation.columnNumber},
                condition: breakpoint.condition,
                options: options
            }, didSetBreakpoint.bind(this));
        }
    },

    _removeBreakpoint: function(breakpoint, callback)
    {
        if (!breakpoint.identifier)
            return;

        function didRemoveBreakpoint(error)
        {
            if (error)
                console.error(error);

            delete this._breakpointIdMap[breakpoint.identifier];

            breakpoint.identifier = null;

            // Don't reset resolved here since we want to keep disabled breakpoints looking like they
            // are resolved in the user interface. They will get marked as unresolved in reset.

            if (typeof callback === "function")
                callback();
        }

        DebuggerAgent.removeBreakpoint(breakpoint.identifier, didRemoveBreakpoint.bind(this));
    },

    _breakpointDisplayLocationDidChange: function(event)
    {
        if (this._ignoreBreakpointDisplayLocationDidChangeEvent)
            return;

        var breakpoint = event.target;
        if (!breakpoint.identifier || breakpoint.disabled)
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, breakpointRemoved.bind(this));

        function breakpointRemoved()
        {
            // Add the breakpoint at its new lineNumber and get a new id.
            this._setBreakpoint(breakpoint);

            this.dispatchEventToListeners(WebInspector.DebuggerManager.Event.BreakpointMoved, {breakpoint: breakpoint});
        }
    },

    _breakpointDisabledStateDidChange: function(event)
    {
        var breakpoint = event.target;

        if (breakpoint === this._allExceptionsBreakpoint) {
            if (!breakpoint.disabled)
                this.breakpointsEnabled = true;
            this._allExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            return;
        }

        if (breakpoint === this._allUncaughtExceptionsBreakpoint) {
            if (!breakpoint.disabled)
                this.breakpointsEnabled = true;
            this._allUncaughtExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            return;
        }

        if (breakpoint.disabled)
            this._removeBreakpoint(breakpoint);
        else
            this._setBreakpoint(breakpoint);
    },

    _breakpointEditablePropertyDidChange: function(event)
    {
        var breakpoint = event.target;
        if (breakpoint.disabled)
            return;

        console.assert(this.isBreakpointEditable(breakpoint));
        if (!this.isBreakpointEditable(breakpoint))
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, breakpointRemoved.bind(this));

        function breakpointRemoved()
        {
            // Add the breakpoint with its new condition and get a new id.
            this._setBreakpoint(breakpoint);
        }
    },

    _updateBreakOnExceptionsState: function()
    {
        var state = "none";

        if (this._breakpointsEnabledSetting.value) {
            if (!this._allExceptionsBreakpoint.disabled)
                state = "all";
            else if (!this._allUncaughtExceptionsBreakpoint.disabled)
                state = "uncaught";
        }

        switch (state) {
        case "all":
            // Mark the uncaught breakpoint as unresolved since "all" includes "uncaught".
            // That way it is clear in the user interface that the breakpoint is ignored.
            this._allUncaughtExceptionsBreakpoint.resolved = false;
            break;
        case "uncaught":
        case "none":
            // Mark the uncaught breakpoint as resolved again.
            this._allUncaughtExceptionsBreakpoint.resolved = true;
            break;
        }

        DebuggerAgent.setPauseOnExceptions(state);
    },

    _inspectorClosing: function(event)
    {
        this._saveBreakpoints();
    },

    _saveBreakpoints: function()
    {
        var savedBreakpoints = [];

        for (var i = 0; i < this._breakpoints.length; ++i) {
            var breakpoint = this._breakpoints[i];

            // Only breakpoints with URLs can be saved. Breakpoints for transient scripts can't.
            if (!breakpoint.url)
                continue;

            savedBreakpoints.push(breakpoint.info);
        }

        this._breakpointsSetting.value = savedBreakpoints;
    },

    _associateBreakpointsWithSourceCode: function(breakpoints, sourceCode)
    {
        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        for (var i = 0; i < breakpoints.length; ++i) {
            var breakpoint = breakpoints[i];
            if (breakpoint.sourceCodeLocation.sourceCode === null)
                breakpoint.sourceCodeLocation.sourceCode = sourceCode;
            // SourceCodes can be unequal if the SourceCodeLocation is associated with a Script and we are looking at the Resource.
            console.assert(breakpoint.sourceCodeLocation.sourceCode === sourceCode || breakpoint.sourceCodeLocation.sourceCode.url === sourceCode.url);
        }

        delete this._ignoreBreakpointDisplayLocationDidChangeEvent;
    }
};

WebInspector.DebuggerManager.prototype.__proto__ = WebInspector.Object.prototype;
