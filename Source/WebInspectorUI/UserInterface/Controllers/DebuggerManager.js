/*
 * Copyright (C) 2013-2016 Apple Inc. All rights reserved.
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

WI.DebuggerManager = class DebuggerManager extends WI.Object
{
    constructor()
    {
        super();

        WI.Breakpoint.addEventListener(WI.Breakpoint.Event.DisplayLocationDidChange, this._breakpointDisplayLocationDidChange, this);
        WI.Breakpoint.addEventListener(WI.Breakpoint.Event.DisabledStateDidChange, this._breakpointDisabledStateDidChange, this);
        WI.Breakpoint.addEventListener(WI.Breakpoint.Event.ConditionDidChange, this._breakpointEditablePropertyDidChange, this);
        WI.Breakpoint.addEventListener(WI.Breakpoint.Event.IgnoreCountDidChange, this._breakpointEditablePropertyDidChange, this);
        WI.Breakpoint.addEventListener(WI.Breakpoint.Event.AutoContinueDidChange, this._breakpointEditablePropertyDidChange, this);
        WI.Breakpoint.addEventListener(WI.Breakpoint.Event.ActionsDidChange, this._handleBreakpointActionsDidChange, this);

        WI.timelineManager.addEventListener(WI.TimelineManager.Event.CapturingStateChanged, this._handleTimelineCapturingStateChanged, this);

        WI.auditManager.addEventListener(WI.AuditManager.Event.TestScheduled, this._handleAuditManagerTestScheduled, this);
        WI.auditManager.addEventListener(WI.AuditManager.Event.TestCompleted, this._handleAuditManagerTestCompleted, this);

        WI.targetManager.addEventListener(WI.TargetManager.Event.TargetRemoved, this._targetRemoved, this);

        if (WI.isEngineeringBuild) {
            WI.settings.engineeringShowInternalScripts.addEventListener(WI.Setting.Event.Changed, this._handleEngineeringShowInternalScriptsSettingChanged, this);
            WI.settings.engineeringPauseForInternalScripts.addEventListener(WI.Setting.Event.Changed, this._handleEngineeringPauseForInternalScriptsSettingChanged, this);
        }

        WI.Frame.addEventListener(WI.Frame.Event.MainResourceDidChange, this._mainResourceDidChange, this);

        this._breakpointsEnabledSetting = new WI.Setting("breakpoints-enabled", true);
        this._asyncStackTraceDepthSetting = new WI.Setting("async-stack-trace-depth", 200);

        const specialBreakpointLocation = new WI.SourceCodeLocation(null, Infinity, Infinity);

        this._debuggerStatementsBreakpointEnabledSetting = new WI.Setting("break-on-debugger-statements", true);
        this._debuggerStatementsBreakpoint = new WI.Breakpoint(specialBreakpointLocation, {
            disabled: !this._debuggerStatementsBreakpointEnabledSetting.value,
        });
        this._debuggerStatementsBreakpoint.resolved = true;

        this._allExceptionsBreakpointEnabledSetting = new WI.Setting("break-on-all-exceptions", false);
        this._allExceptionsBreakpoint = new WI.Breakpoint(specialBreakpointLocation, {
            disabled: !this._allExceptionsBreakpointEnabledSetting.value,
        });
        this._allExceptionsBreakpoint.resolved = true;

        this._uncaughtExceptionsBreakpointEnabledSetting = new WI.Setting("break-on-uncaught-exceptions", false);
        this._uncaughtExceptionsBreakpoint = new WI.Breakpoint(specialBreakpointLocation, {
            disabled: !this._uncaughtExceptionsBreakpointEnabledSetting.value,
        });
        this._uncaughtExceptionsBreakpoint.resolved = true;

        this._assertionFailuresBreakpointEnabledSetting = new WI.Setting("break-on-assertion-failures", false);
        this._assertionFailuresBreakpoint = new WI.Breakpoint(specialBreakpointLocation, {
            disabled: !this._assertionFailuresBreakpointEnabledSetting.value,
        });
        this._assertionFailuresBreakpoint.resolved = true;

        this._allMicrotasksBreakpointEnabledSetting = new WI.Setting("break-on-all-microtasks", false);
        this._allMicrotasksBreakpoint = new WI.Breakpoint(specialBreakpointLocation, {
            disabled: !this._allMicrotasksBreakpointEnabledSetting.value,
        });
        this._allMicrotasksBreakpoint.resolved = true;

        this._breakpoints = [];
        this._breakpointContentIdentifierMap = new Multimap;
        this._breakpointScriptIdentifierMap = new Multimap;
        this._breakpointIdMap = new Map;

        this._breakOnExceptionsState = "none";
        this._updateBreakOnExceptionsState();

        this._nextBreakpointActionIdentifier = 1;

        this._blackboxedURLsSetting = new WI.Setting("debugger-blackboxed-urls", []);
        this._blackboxedPatternsSetting = new WI.Setting("debugger-blackboxed-patterns", []);
        this._blackboxedPatternDataMap = new Map;

        this._activeCallFrame = null;

        this._internalWebKitScripts = [];
        this._targetDebuggerDataMap = new Map;

        // Used to detect deleted probe actions.
        this._knownProbeIdentifiersForBreakpoint = new Map;

        // Main lookup tables for probes and probe sets.
        this._probesByIdentifier = new Map;
        this._probeSetsByBreakpoint = new Map;

        // Restore the correct breakpoints enabled setting if Web Inspector had
        // previously been left in a state where breakpoints were temporarily disabled.
        this._temporarilyDisabledBreakpointsRestoreSetting = new WI.Setting("temporarily-disabled-breakpoints-restore", null);
        if (this._temporarilyDisabledBreakpointsRestoreSetting.value !== null) {
            this._breakpointsEnabledSetting.value = this._temporarilyDisabledBreakpointsRestoreSetting.value;
            this._temporarilyDisabledBreakpointsRestoreSetting.value = null;
        }
        this._temporarilyDisableBreakpointsRequestCount = 0;

        this._ignoreBreakpointDisplayLocationDidChangeEvent = false;

        WI.Target.registerInitializationPromise((async () => {
            let existingSerializedBreakpoints = WI.Setting.migrateValue("breakpoints");
            if (existingSerializedBreakpoints) {
                for (let existingSerializedBreakpoint of existingSerializedBreakpoints)
                    await WI.objectStores.breakpoints.putObject(WI.Breakpoint.fromJSON(existingSerializedBreakpoint));
            }

            let serializedBreakpoints = await WI.objectStores.breakpoints.getAll();

            this._restoringBreakpoints = true;
            for (let serializedBreakpoint of serializedBreakpoints) {
                let breakpoint = WI.Breakpoint.fromJSON(serializedBreakpoint);

                const key = null;
                WI.objectStores.breakpoints.associateObject(breakpoint, key, serializedBreakpoint);

                this.addBreakpoint(breakpoint);
            }
            this._restoringBreakpoints = false;
        })());
    }

    // Target

    initializeTarget(target)
    {
        let targetData = this.dataForTarget(target);

        // Initialize global state.
        target.DebuggerAgent.enable();
        target.DebuggerAgent.setBreakpointsActive(this._breakpointsEnabledSetting.value);

        // COMPATIBILITY (iOS 13.1): Debugger.setPauseOnDebuggerStatements did not exist yet.
        if (target.hasCommand("Debugger.setPauseOnDebuggerStatements"))
            target.DebuggerAgent.setPauseOnDebuggerStatements(!this._debuggerStatementsBreakpoint.disabled);

        target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
        target.DebuggerAgent.setPauseOnAssertions(!this._assertionFailuresBreakpoint.disabled);

        // COMPATIBILITY (iOS 13): DebuggerAgent.setPauseOnMicrotasks did not exist yet.
        if (target.hasCommand("Debugger.setPauseOnMicrotasks"))
            target.DebuggerAgent.setPauseOnMicrotasks(!this._allMicrotasksBreakpoint.disabled);

        target.DebuggerAgent.setAsyncStackTraceDepth(this._asyncStackTraceDepthSetting.value);

        // COMPATIBILITY (iOS 13): Debugger.setShouldBlackboxURL did not exist yet.
        if (target.hasCommand("Debugger.setShouldBlackboxURL")) {
            const shouldBlackbox = true;

            {
                const caseSensitive = true;
                for (let url of this._blackboxedURLsSetting.value)
                    target.DebuggerAgent.setShouldBlackboxURL(url, shouldBlackbox, caseSensitive);
            }

            {
                const isRegex = true;
                for (let data of this._blackboxedPatternsSetting.value) {
                    this._blackboxedPatternDataMap.set(new RegExp(data.url, !data.caseSensitive ? "i" : ""), data);
                    target.DebuggerAgent.setShouldBlackboxURL(data.url, shouldBlackbox, data.caseSensitive, isRegex);
                }
            }
        }

        if (WI.isEngineeringBuild) {
            // COMPATIBILITY (iOS 12): DebuggerAgent.setPauseForInternalScripts did not exist yet.
            if (target.hasCommand("Debugger.setPauseForInternalScripts"))
                target.DebuggerAgent.setPauseForInternalScripts(WI.settings.engineeringPauseForInternalScripts.value);
        }

        if (this.paused)
            targetData.pauseIfNeeded();

        // Initialize breakpoints.
        this._restoringBreakpoints = true;
        for (let breakpoint of this._breakpoints) {
            if (breakpoint.disabled)
                continue;
            if (!breakpoint.contentIdentifier)
                continue;
            this._setBreakpoint(breakpoint, target);
        }
        this._restoringBreakpoints = false;
    }

    // Static

    static supportsBlackboxingScripts()
    {
        return InspectorBackend.hasCommand("Debugger.setShouldBlackboxURL");
    }

    static pauseReasonFromPayload(payload)
    {
        switch (payload) {
        case InspectorBackend.Enum.Debugger.PausedReason.AnimationFrame:
            return WI.DebuggerManager.PauseReason.AnimationFrame;
        case InspectorBackend.Enum.Debugger.PausedReason.Assert:
            return WI.DebuggerManager.PauseReason.Assertion;
        case InspectorBackend.Enum.Debugger.PausedReason.BlackboxedScript:
            return WI.DebuggerManager.PauseReason.BlackboxedScript;
        case InspectorBackend.Enum.Debugger.PausedReason.Breakpoint:
            return WI.DebuggerManager.PauseReason.Breakpoint;
        case InspectorBackend.Enum.Debugger.PausedReason.CSPViolation:
            return WI.DebuggerManager.PauseReason.CSPViolation;
        case InspectorBackend.Enum.Debugger.PausedReason.DOM:
            return WI.DebuggerManager.PauseReason.DOM;
        case InspectorBackend.Enum.Debugger.PausedReason.DebuggerStatement:
            return WI.DebuggerManager.PauseReason.DebuggerStatement;
        case InspectorBackend.Enum.Debugger.PausedReason.EventListener:
            return WI.DebuggerManager.PauseReason.EventListener;
        case InspectorBackend.Enum.Debugger.PausedReason.Exception:
            return WI.DebuggerManager.PauseReason.Exception;
        case InspectorBackend.Enum.Debugger.PausedReason.Fetch:
            return WI.DebuggerManager.PauseReason.Fetch;
        case InspectorBackend.Enum.Debugger.PausedReason.Interval:
            return WI.DebuggerManager.PauseReason.Interval;
        case InspectorBackend.Enum.Debugger.PausedReason.Listener:
            return WI.DebuggerManager.PauseReason.Listener;
        case InspectorBackend.Enum.Debugger.PausedReason.Microtask:
            return WI.DebuggerManager.PauseReason.Microtask;
        case InspectorBackend.Enum.Debugger.PausedReason.PauseOnNextStatement:
            return WI.DebuggerManager.PauseReason.PauseOnNextStatement;
        case InspectorBackend.Enum.Debugger.PausedReason.Timeout:
            return WI.DebuggerManager.PauseReason.Timeout;
        case InspectorBackend.Enum.Debugger.PausedReason.Timer:
            return WI.DebuggerManager.PauseReason.Timer;
        case InspectorBackend.Enum.Debugger.PausedReason.XHR:
            return WI.DebuggerManager.PauseReason.XHR;
        default:
            return WI.DebuggerManager.PauseReason.Other;
        }
    }

    // Public

    get paused()
    {
        for (let [target, targetData] of this._targetDebuggerDataMap) {
            if (targetData.paused)
                return true;
        }

        return false;
    }

    get activeCallFrame()
    {
        return this._activeCallFrame;
    }

    set activeCallFrame(callFrame)
    {
        if (callFrame === this._activeCallFrame)
            return;

        this._activeCallFrame = callFrame || null;

        this.dispatchEventToListeners(WI.DebuggerManager.Event.ActiveCallFrameDidChange);
    }

    dataForTarget(target)
    {
        let targetData = this._targetDebuggerDataMap.get(target);
        if (targetData)
            return targetData;

        targetData = new WI.DebuggerData(target);
        this._targetDebuggerDataMap.set(target, targetData);
        return targetData;
    }

    get debuggerStatementsBreakpoint() { return this._debuggerStatementsBreakpoint; }
    get allExceptionsBreakpoint() { return this._allExceptionsBreakpoint; }
    get uncaughtExceptionsBreakpoint() { return this._uncaughtExceptionsBreakpoint; }
    get assertionFailuresBreakpoint() { return this._assertionFailuresBreakpoint; }
    get allMicrotasksBreakpoint() { return this._allMicrotasksBreakpoint; }
    get breakpoints() { return this._breakpoints; }

    breakpointForIdentifier(id)
    {
        return this._breakpointIdMap.get(id) || null;
    }

    breakpointsForSourceCode(sourceCode)
    {
        console.assert(sourceCode instanceof WI.Resource || sourceCode instanceof WI.Script);

        if (sourceCode instanceof WI.SourceMapResource)
            return Array.from(this.breakpointsForSourceCode(sourceCode.sourceMap.originalSourceCode)).filter((breakpoint) => breakpoint.sourceCodeLocation.displaySourceCode === sourceCode);

        let contentIdentifierBreakpoints = this._breakpointContentIdentifierMap.get(sourceCode.contentIdentifier);
        if (contentIdentifierBreakpoints) {
            this._associateBreakpointsWithSourceCode(contentIdentifierBreakpoints, sourceCode);
            return contentIdentifierBreakpoints;
        }

        if (sourceCode instanceof WI.Script) {
            let scriptIdentifierBreakpoints = this._breakpointScriptIdentifierMap.get(sourceCode.id);
            if (scriptIdentifierBreakpoints) {
                this._associateBreakpointsWithSourceCode(scriptIdentifierBreakpoints, sourceCode);
                return scriptIdentifierBreakpoints;
            }
        }

        return [];
    }

    breakpointForSourceCodeLocation(sourceCodeLocation)
    {
        console.assert(sourceCodeLocation instanceof WI.SourceCodeLocation);

        for (let breakpoint of this.breakpointsForSourceCode(sourceCodeLocation.sourceCode)) {
            if (breakpoint.sourceCodeLocation.isEqual(sourceCodeLocation))
                return breakpoint;
        }

        return null;
    }

    isBreakpointRemovable(breakpoint)
    {
        return breakpoint !== this._debuggerStatementsBreakpoint
            && breakpoint !== this._allExceptionsBreakpoint
            && breakpoint !== this._uncaughtExceptionsBreakpoint;
    }

    isBreakpointSpecial(breakpoint)
    {
        return !this.isBreakpointRemovable(breakpoint)
            || breakpoint === this._assertionFailuresBreakpoint
            || breakpoint === this._allMicrotasksBreakpoint;
    }

    isBreakpointEditable(breakpoint)
    {
        return !this.isBreakpointSpecial(breakpoint);
    }

    get breakpointsEnabled()
    {
        return this._breakpointsEnabledSetting.value;
    }

    set breakpointsEnabled(enabled)
    {
        if (this._breakpointsEnabledSetting.value === enabled)
            return;

        console.assert(!(enabled && this.breakpointsDisabledTemporarily), "Should not enable breakpoints when we are temporarily disabling breakpoints.");
        if (enabled && this.breakpointsDisabledTemporarily)
            return;

        this._breakpointsEnabledSetting.value = enabled;

        this._updateBreakOnExceptionsState();

        for (let target of WI.targets) {
            target.DebuggerAgent.setBreakpointsActive(enabled);
            target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
        }

        this.dispatchEventToListeners(WI.DebuggerManager.Event.BreakpointsEnabledDidChange);
    }

    get breakpointsDisabledTemporarily()
    {
        return this._temporarilyDisabledBreakpointsRestoreSetting.value !== null;
    }

    scriptForIdentifier(id, target)
    {
        console.assert(target instanceof WI.Target);
        return this.dataForTarget(target).scriptForIdentifier(id);
    }

    scriptsForURL(url, target)
    {
        // FIXME: This may not be safe. A Resource's URL may differ from a Script's URL.
        console.assert(target instanceof WI.Target);
        return this.dataForTarget(target).scriptsForURL(url);
    }

    get searchableScripts()
    {
        return this.knownNonResourceScripts.filter((script) => !!script.contentIdentifier);
    }

    get knownNonResourceScripts()
    {
        let knownScripts = [];

        for (let targetData of this._targetDebuggerDataMap.values()) {
            for (let script of targetData.scripts) {
                if (script.resource)
                    continue;
                if (!WI.settings.debugShowConsoleEvaluations.value && isWebInspectorConsoleEvaluationScript(script.sourceURL))
                    continue;
                if (!WI.settings.engineeringShowInternalScripts.value && isWebKitInternalScript(script.sourceURL))
                    continue;
                knownScripts.push(script);
            }
        }

        return knownScripts;
    }

    blackboxDataForSourceCode(sourceCode)
    {
        for (let regex of this._blackboxedPatternDataMap.keys()) {
            if (regex.test(sourceCode.contentIdentifier))
                return {type: DebuggerManager.BlackboxType.Pattern, regex};
        }

        if (this._blackboxedURLsSetting.value.includes(sourceCode.contentIdentifier))
            return {type: DebuggerManager.BlackboxType.URL};

        return null;
    }

    get blackboxPatterns()
    {
        return Array.from(this._blackboxedPatternDataMap.keys());
    }

    setShouldBlackboxScript(sourceCode, shouldBlackbox)
    {
        console.assert(DebuggerManager.supportsBlackboxingScripts());
        console.assert(sourceCode instanceof WI.SourceCode);
        console.assert(sourceCode.contentIdentifier);
        console.assert(!isWebKitInjectedScript(sourceCode.contentIdentifier));
        console.assert(shouldBlackbox !== ((this.blackboxDataForSourceCode(sourceCode) || {}).type === DebuggerManager.BlackboxType.URL));

        this._blackboxedURLsSetting.value.toggleIncludes(sourceCode.contentIdentifier, shouldBlackbox);
        this._blackboxedURLsSetting.save();

        const caseSensitive = true;
        for (let target of WI.targets) {
            // COMPATIBILITY (iOS 13): Debugger.setShouldBlackboxURL did not exist yet.
            if (target.hasCommand("Debugger.setShouldBlackboxURL"))
                target.DebuggerAgent.setShouldBlackboxURL(sourceCode.contentIdentifier, !!shouldBlackbox, caseSensitive);
        }

        this.dispatchEventToListeners(DebuggerManager.Event.BlackboxChanged);
    }

    setShouldBlackboxPattern(regex, shouldBlackbox)
    {
        console.assert(DebuggerManager.supportsBlackboxingScripts());
        console.assert(regex instanceof RegExp);

        if (shouldBlackbox) {
            console.assert(!this._blackboxedPatternDataMap.has(regex));

            let data = {
                url: regex.source,
                caseSensitive: !regex.ignoreCase,
            };
            this._blackboxedPatternDataMap.set(regex, data);
            this._blackboxedPatternsSetting.value.push(data);
        } else {
            console.assert(this._blackboxedPatternDataMap.has(regex));
            this._blackboxedPatternsSetting.value.remove(this._blackboxedPatternDataMap.take(regex));
        }

        this._blackboxedPatternsSetting.save();

        const isRegex = true;
        for (let target of WI.targets) {
            // COMPATIBILITY (iOS 13): Debugger.setShouldBlackboxURL did not exist yet.
            if (target.hasCommand("Debugger.setShouldBlackboxURL"))
                target.DebuggerAgent.setShouldBlackboxURL(regex.source, !!shouldBlackbox, !regex.ignoreCase, isRegex);
        }

        this.dispatchEventToListeners(DebuggerManager.Event.BlackboxChanged);
    }

    get asyncStackTraceDepth()
    {
        return this._asyncStackTraceDepthSetting.value;
    }

    set asyncStackTraceDepth(x)
    {
        if (this._asyncStackTraceDepthSetting.value === x)
            return;

        this._asyncStackTraceDepthSetting.value = x;

        for (let target of WI.targets)
            target.DebuggerAgent.setAsyncStackTraceDepth(this._asyncStackTraceDepthSetting.value);
    }

    get probeSets()
    {
        return [...this._probeSetsByBreakpoint.values()];
    }

    probeForIdentifier(identifier)
    {
        return this._probesByIdentifier.get(identifier);
    }

    pause()
    {
        if (this.paused)
            return Promise.resolve();

        this.dispatchEventToListeners(WI.DebuggerManager.Event.WaitingToPause);

        let listener = new WI.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WI.debuggerManager, WI.DebuggerManager.Event.Paused, resolve);
        });

        let promises = [];
        for (let [target, targetData] of this._targetDebuggerDataMap)
            promises.push(targetData.pauseIfNeeded());

        return Promise.all([managerResult, ...promises]);
    }

    resume()
    {
        if (!this.paused)
            return Promise.resolve();

        let listener = new WI.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WI.debuggerManager, WI.DebuggerManager.Event.Resumed, resolve);
        });

        let promises = [];
        for (let [target, targetData] of this._targetDebuggerDataMap)
            promises.push(targetData.resumeIfNeeded());

        return Promise.all([managerResult, ...promises]);
    }

    stepNext()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step next because debugger is not paused."));

        let listener = new WI.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WI.debuggerManager, WI.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepNext()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepNext failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    stepOver()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step over because debugger is not paused."));

        let listener = new WI.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WI.debuggerManager, WI.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepOver()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepOver failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    stepInto()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step into because debugger is not paused."));

        let listener = new WI.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WI.debuggerManager, WI.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepInto()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepInto failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    stepOut()
    {
        if (!this.paused)
            return Promise.reject(new Error("Cannot step out because debugger is not paused."));

        let listener = new WI.EventListener(this, true);

        let managerResult = new Promise(function(resolve, reject) {
            listener.connect(WI.debuggerManager, WI.DebuggerManager.Event.ActiveCallFrameDidChange, resolve);
        });

        let protocolResult = this._activeCallFrame.target.DebuggerAgent.stepOut()
            .catch(function(error) {
                listener.disconnect();
                console.error("DebuggerManager.stepOut failed: ", error);
                throw error;
            });

        return Promise.all([managerResult, protocolResult]);
    }

    continueUntilNextRunLoop(target)
    {
        return this.dataForTarget(target).continueUntilNextRunLoop();
    }

    continueToLocation(script, lineNumber, columnNumber)
    {
        return script.target.DebuggerAgent.continueToLocation({scriptId: script.id, lineNumber, columnNumber});
    }

    addBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.Breakpoint);
        if (!breakpoint)
            return;

        if (this.isBreakpointSpecial(breakpoint)) {
            this.dispatchEventToListeners(WI.DebuggerManager.Event.BreakpointAdded, {breakpoint});
            return;
        }

        if (breakpoint.contentIdentifier)
            this._breakpointContentIdentifierMap.add(breakpoint.contentIdentifier, breakpoint);

        if (breakpoint.scriptIdentifier)
            this._breakpointScriptIdentifierMap.add(breakpoint.scriptIdentifier, breakpoint);

        this._breakpoints.push(breakpoint);

        if (!breakpoint.disabled)
            this._setBreakpoint(breakpoint);

        if (!this._restoringBreakpoints)
            WI.objectStores.breakpoints.putObject(breakpoint);

        this._addProbesForBreakpoint(breakpoint);

        this.dispatchEventToListeners(WI.DebuggerManager.Event.BreakpointAdded, {breakpoint});
    }

    removeBreakpoint(breakpoint)
    {
        console.assert(breakpoint instanceof WI.Breakpoint);
        if (!breakpoint)
            return;

        console.assert(this.isBreakpointRemovable(breakpoint));
        if (!this.isBreakpointRemovable(breakpoint))
            return;

        if (this.isBreakpointSpecial(breakpoint)) {
            breakpoint.disabled = true;
            this.dispatchEventToListeners(WI.DebuggerManager.Event.BreakpointRemoved, {breakpoint});
            return;
        }

        this._breakpoints.remove(breakpoint);

        if (breakpoint.identifier)
            this._removeBreakpoint(breakpoint);

        if (breakpoint.contentIdentifier)
            this._breakpointContentIdentifierMap.delete(breakpoint.contentIdentifier, breakpoint);

        if (breakpoint.scriptIdentifier)
            this._breakpointScriptIdentifierMap.delete(breakpoint.scriptIdentifier, breakpoint);

        // Disable the breakpoint first, so removing actions doesn't re-add the breakpoint.
        breakpoint.disabled = true;
        breakpoint.clearActions();

        if (!this._restoringBreakpoints)
            WI.objectStores.breakpoints.deleteObject(breakpoint);

        this._removeProbesForBreakpoint(breakpoint);

        this.dispatchEventToListeners(WI.DebuggerManager.Event.BreakpointRemoved, {breakpoint});
    }

    nextBreakpointActionIdentifier()
    {
        return this._nextBreakpointActionIdentifier++;
    }

    // DebuggerObserver

    breakpointResolved(target, breakpointIdentifier, location)
    {
        let breakpoint = this._breakpointIdMap.get(breakpointIdentifier);
        console.assert(breakpoint);
        if (!breakpoint)
            return;

        console.assert(breakpoint.identifier === breakpointIdentifier);

        if (!breakpoint.sourceCodeLocation.sourceCode) {
            let sourceCodeLocation = this._sourceCodeLocationFromPayload(target, location);
            breakpoint.sourceCodeLocation.sourceCode = sourceCodeLocation.sourceCode;
        }

        breakpoint.resolved = true;
    }

    globalObjectCleared(target)
    {
        let wasPaused = this.paused;

        WI.Script.resetUniqueDisplayNameNumbers(target);

        this._internalWebKitScripts = [];
        this._targetDebuggerDataMap.clear();

        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        // Mark all the breakpoints as unresolved. They will be reported as resolved when
        // breakpointResolved is called as the page loads.
        for (let breakpoint of this._breakpoints) {
            breakpoint.resolved = false;
            if (breakpoint.sourceCodeLocation.sourceCode)
                breakpoint.sourceCodeLocation.sourceCode = null;
        }

        this._ignoreBreakpointDisplayLocationDidChangeEvent = false;

        this.dispatchEventToListeners(WI.DebuggerManager.Event.ScriptsCleared);

        if (wasPaused)
            this.dispatchEventToListeners(WI.DebuggerManager.Event.Resumed);
    }

    debuggerDidPause(target, callFramesPayload, reason, data, asyncStackTracePayload)
    {
        if (this._delayedResumeTimeout) {
            clearTimeout(this._delayedResumeTimeout);
            this._delayedResumeTimeout = undefined;
        }

        let wasPaused = this.paused;
        let targetData = this._targetDebuggerDataMap.get(target);

        let callFrames = [];
        let pauseReason = DebuggerManager.pauseReasonFromPayload(reason);
        let pauseData = data || null;

        for (var i = 0; i < callFramesPayload.length; ++i) {
            var callFramePayload = callFramesPayload[i];
            var sourceCodeLocation = this._sourceCodeLocationFromPayload(target, callFramePayload.location);
            // FIXME: There may be useful call frames without a source code location (native callframes), should we include them?
            if (!sourceCodeLocation)
                continue;
            if (!sourceCodeLocation.sourceCode)
                continue;

            // Exclude the case where the call frame is in the inspector code.
            if (!WI.settings.engineeringShowInternalScripts.value && isWebKitInternalScript(sourceCodeLocation.sourceCode.sourceURL))
                continue;

            let scopeChain = this._scopeChainFromPayload(target, callFramePayload.scopeChain);
            let callFrame = WI.CallFrame.fromDebuggerPayload(target, callFramePayload, scopeChain, sourceCodeLocation);
            callFrames.push(callFrame);
        }

        let activeCallFrame = callFrames[0];

        if (!activeCallFrame) {
            // FIXME: This may not be safe for multiple threads/targets.
            // This indicates we were pausing in internal scripts only (Injected Scripts).
            // Just resume and skip past this pause. We should be fixing the backend to
            // not send such pauses.
            if (wasPaused)
                target.DebuggerAgent.continueUntilNextRunLoop();
            else
                target.DebuggerAgent.resume();
            this._didResumeInternal(target);
            return;
        }

        let asyncStackTrace = WI.StackTrace.fromPayload(target, asyncStackTracePayload);
        targetData.updateForPause(callFrames, pauseReason, pauseData, asyncStackTrace);

        // Pause other targets because at least one target has paused.
        // FIXME: Should this be done on the backend?
        for (let [otherTarget, otherTargetData] of this._targetDebuggerDataMap)
            otherTargetData.pauseIfNeeded();

        let activeCallFrameDidChange = this._activeCallFrame && this._activeCallFrame.target === target;
        if (activeCallFrameDidChange)
            this._activeCallFrame = activeCallFrame;
        else if (!wasPaused) {
            this._activeCallFrame = activeCallFrame;
            activeCallFrameDidChange = true;
        }

        if (!wasPaused)
            this.dispatchEventToListeners(WI.DebuggerManager.Event.Paused);

        this.dispatchEventToListeners(WI.DebuggerManager.Event.CallFramesDidChange, {target});

        if (activeCallFrameDidChange)
            this.dispatchEventToListeners(WI.DebuggerManager.Event.ActiveCallFrameDidChange);
    }

    debuggerDidResume(target)
    {
        this._didResumeInternal(target);
    }

    playBreakpointActionSound(breakpointActionIdentifier)
    {
        InspectorFrontendHost.beep();
    }

    scriptDidParse(target, scriptIdentifier, url, startLine, startColumn, endLine, endColumn, isModule, isContentScript, sourceURL, sourceMapURL)
    {
        // Don't add the script again if it is already known.
        let targetData = this.dataForTarget(target);
        let existingScript = targetData.scriptForIdentifier(scriptIdentifier);
        if (existingScript) {
            console.assert(existingScript.url === (url || null));
            console.assert(existingScript.range.startLine === startLine);
            console.assert(existingScript.range.startColumn === startColumn);
            console.assert(existingScript.range.endLine === endLine);
            console.assert(existingScript.range.endColumn === endColumn);
            return;
        }

        if (!WI.settings.engineeringShowInternalScripts.value && isWebKitInternalScript(sourceURL))
            return;

        let range = new WI.TextRange(startLine, startColumn, endLine, endColumn);
        let sourceType = isModule ? WI.Script.SourceType.Module : WI.Script.SourceType.Program;
        let script = new WI.Script(target, scriptIdentifier, range, url, sourceType, isContentScript, sourceURL, sourceMapURL);

        targetData.addScript(script);

        // FIXME: <https://webkit.org/b/164427> Web Inspector: WorkerTarget's mainResource should be a Resource not a Script
        // We make the main resource of a WorkerTarget the Script instead of the Resource
        // because the frontend may not be informed of the Resource. We should guarantee
        // the frontend is informed of the Resource.
        if (WI.sharedApp.debuggableType === WI.DebuggableType.ServiceWorker) {
            // A ServiceWorker starts with a LocalScript for the main resource but we can replace it during initialization.
            if (target.mainResource instanceof WI.LocalScript) {
                if (script.url === target.name)
                    target.mainResource = script;
            }
        } else if (!target.mainResource && target !== WI.mainTarget) {
            // A Worker starts without a main resource and we insert one.
            if (script.url === target.name) {
                target.mainResource = script;
                if (script.resource)
                    target.resourceCollection.remove(script.resource);
            }
        }

        if (isWebKitInternalScript(script.sourceURL)) {
            this._internalWebKitScripts.push(script);
            if (!WI.settings.engineeringShowInternalScripts.value)
                return;
        }

        if (!WI.settings.debugShowConsoleEvaluations.value && isWebInspectorConsoleEvaluationScript(script.sourceURL))
            return;

        this.dispatchEventToListeners(WI.DebuggerManager.Event.ScriptAdded, {script});

        if ((target !== WI.mainTarget || WI.sharedApp.debuggableType === WI.DebuggableType.ServiceWorker) && !script.isMainResource() && !script.resource)
            target.addScript(script);
    }

    scriptDidFail(target, url, scriptSource)
    {
        const sourceURL = null;
        const sourceType = WI.Script.SourceType.Program;
        let script = new WI.LocalScript(target, url, sourceURL, sourceType, scriptSource);

        // If there is already a resource we don't need to have the script anymore,
        // we only need a script to use for parser error location links.
        if (script.resource)
            return;

        let targetData = this.dataForTarget(target);
        targetData.addScript(script);
        target.addScript(script);

        this.dispatchEventToListeners(WI.DebuggerManager.Event.ScriptAdded, {script});
    }

    didSampleProbe(target, sample)
    {
        console.assert(this._probesByIdentifier.has(sample.probeId), "Unknown probe identifier specified for sample: ", sample);
        let probe = this._probesByIdentifier.get(sample.probeId);
        let elapsedTime = WI.timelineManager.computeElapsedTime(sample.timestamp);
        let object = WI.RemoteObject.fromPayload(sample.payload, target);
        probe.addSample(new WI.ProbeSample(sample.sampleId, sample.batchId, elapsedTime, object));
    }

    // Private

    _sourceCodeLocationFromPayload(target, payload)
    {
        let targetData = this.dataForTarget(target);
        let script = targetData.scriptForIdentifier(payload.scriptId);
        if (!script)
            return null;

        return script.createSourceCodeLocation(payload.lineNumber, payload.columnNumber);
    }

    _scopeChainFromPayload(target, payload)
    {
        let scopeChain = [];
        for (let i = 0; i < payload.length; ++i)
            scopeChain.push(this._scopeChainNodeFromPayload(target, payload[i]));
        return scopeChain;
    }

    _scopeChainNodeFromPayload(target, payload)
    {
        var type = null;
        switch (payload.type) {
        case InspectorBackend.Enum.Debugger.ScopeType.Global:
            type = WI.ScopeChainNode.Type.Global;
            break;
        case InspectorBackend.Enum.Debugger.ScopeType.With:
            type = WI.ScopeChainNode.Type.With;
            break;
        case InspectorBackend.Enum.Debugger.ScopeType.Closure:
            type = WI.ScopeChainNode.Type.Closure;
            break;
        case InspectorBackend.Enum.Debugger.ScopeType.Catch:
            type = WI.ScopeChainNode.Type.Catch;
            break;
        case InspectorBackend.Enum.Debugger.ScopeType.FunctionName:
            type = WI.ScopeChainNode.Type.FunctionName;
            break;
        case InspectorBackend.Enum.Debugger.ScopeType.NestedLexical:
            type = WI.ScopeChainNode.Type.Block;
            break;
        case InspectorBackend.Enum.Debugger.ScopeType.GlobalLexicalEnvironment:
            type = WI.ScopeChainNode.Type.GlobalLexicalEnvironment;
            break;
        default:
            console.error("Unknown type: " + payload.type);
            break;
        }

        let object = WI.RemoteObject.fromPayload(payload.object, target);
        return new WI.ScopeChainNode(type, [object], payload.name, payload.location, payload.empty);
    }

    _debuggerBreakpointActionType(type)
    {
        switch (type) {
        case WI.BreakpointAction.Type.Log:
            return InspectorBackend.Enum.Debugger.BreakpointActionType.Log;
        case WI.BreakpointAction.Type.Evaluate:
            return InspectorBackend.Enum.Debugger.BreakpointActionType.Evaluate;
        case WI.BreakpointAction.Type.Sound:
            return InspectorBackend.Enum.Debugger.BreakpointActionType.Sound;
        case WI.BreakpointAction.Type.Probe:
            return InspectorBackend.Enum.Debugger.BreakpointActionType.Probe;
        default:
            console.assert(false);
            return InspectorBackend.Enum.Debugger.BreakpointActionType.Log;
        }
    }

    _debuggerBreakpointOptions(breakpoint)
    {
        let actions = breakpoint.actions;
        actions = actions.map((action) => action.toProtocol());
        actions = actions.filter((action) => {
            if (action.type !== WI.BreakpointAction.Type.Log)
                return true;

            if (!/\$\{.*?\}/.test(action.data))
                return true;

            let lexer = new WI.BreakpointLogMessageLexer;
            let tokens = lexer.tokenize(action.data);
            if (!tokens)
                return false;

            let templateLiteral = tokens.reduce((text, token) => {
                if (token.type === WI.BreakpointLogMessageLexer.TokenType.PlainText)
                    return text + token.data.escapeCharacters("`\\");
                if (token.type === WI.BreakpointLogMessageLexer.TokenType.Expression)
                    return text + "${" + token.data + "}";
                return text;
            }, "");

            action.data = "console.log(`" + templateLiteral + "`)";
            action.type = WI.BreakpointAction.Type.Evaluate;
            return true;
        });

        return {
            condition: breakpoint.condition,
            ignoreCount: breakpoint.ignoreCount,
            autoContinue: breakpoint.autoContinue,
            actions,
        };
    }

    _setBreakpoint(breakpoint, specificTarget)
    {
        console.assert(!breakpoint.disabled);

        if (breakpoint.disabled)
            return;

        if (!this._restoringBreakpoints && !this.breakpointsDisabledTemporarily) {
            // Enable breakpoints since a breakpoint is being set. This eliminates
            // a multi-step process for the user that can be confusing.
            this.breakpointsEnabled = true;
        }

        function didSetBreakpoint(target, error, breakpointIdentifier, locations) {
            if (error) {
                WI.reportInternalError(error);
                return;
            }

            this._breakpointIdMap.set(breakpointIdentifier, breakpoint);

            breakpoint.identifier = breakpointIdentifier;

            // Debugger.setBreakpoint returns a single location.
            if (!(locations instanceof Array))
                locations = [locations];

            for (let location of locations)
                this.breakpointResolved(target, breakpointIdentifier, location);
        }

        // The breakpoint will be resolved again by calling DebuggerAgent, so mark it as unresolved.
        // If something goes wrong it will stay unresolved and show up as such in the user interface.
        // When setting for a new target, don't change the resolved target.
        if (!specificTarget)
            breakpoint.resolved = false;

        // Convert BreakpointAction types to DebuggerAgent protocol types.
        // NOTE: Breakpoint.options returns new objects each time, so it is safe to modify.
        let options = this._debuggerBreakpointOptions(breakpoint);
        for (let action of options.actions)
            action.type = this._debuggerBreakpointActionType(action.type);

        if (breakpoint.contentIdentifier) {
            let targets = specificTarget ? [specificTarget] : WI.targets;
            for (let target of targets) {
                target.DebuggerAgent.setBreakpointByUrl.invoke({
                    lineNumber: breakpoint.sourceCodeLocation.lineNumber,
                    url: breakpoint.contentIdentifier,
                    urlRegex: undefined,
                    columnNumber: breakpoint.sourceCodeLocation.columnNumber,
                    options
                }, didSetBreakpoint.bind(this, target));
            }
        } else if (breakpoint.scriptIdentifier) {
            let target = breakpoint.target;
            target.DebuggerAgent.setBreakpoint.invoke({
                location: {scriptId: breakpoint.scriptIdentifier, lineNumber: breakpoint.sourceCodeLocation.lineNumber, columnNumber: breakpoint.sourceCodeLocation.columnNumber},
                options
            }, didSetBreakpoint.bind(this, target));
        } else
            WI.reportInternalError("Unknown source for breakpoint.");
    }

    _removeBreakpoint(breakpoint, callback)
    {
        if (!breakpoint.identifier)
            return;

        function didRemoveBreakpoint(target, error)
        {
            if (error) {
                WI.reportInternalError(error);
                return;
            }

            this._breakpointIdMap.delete(breakpoint.identifier);

            breakpoint.identifier = null;

            // Don't reset resolved here since we want to keep disabled breakpoints looking like they
            // are resolved in the user interface. They will get marked as unresolved in reset.

            if (callback)
                callback(target);
        }

        if (breakpoint.contentIdentifier) {
            for (let target of WI.targets)
                target.DebuggerAgent.removeBreakpoint(breakpoint.identifier, didRemoveBreakpoint.bind(this, target));
        } else if (breakpoint.scriptIdentifier) {
            let target = breakpoint.target;
            target.DebuggerAgent.removeBreakpoint(breakpoint.identifier, didRemoveBreakpoint.bind(this, target));
        }
    }

    _breakpointDisplayLocationDidChange(event)
    {
        if (this._ignoreBreakpointDisplayLocationDidChangeEvent)
            return;

        let breakpoint = event.target;
        if (!breakpoint.identifier || breakpoint.disabled)
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, (target) => {
            // Add the breakpoint at its new lineNumber and get a new id.
            this._restoringBreakpoints = true;
            this._setBreakpoint(breakpoint, target);
            this._restoringBreakpoints = false;

            this.dispatchEventToListeners(WI.DebuggerManager.Event.BreakpointMoved, {breakpoint});
        });
    }

    _breakpointDisabledStateDidChange(event)
    {
        let breakpoint = event.target;
        switch (breakpoint) {
        case this._debuggerStatementsBreakpoint:
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._debuggerStatementsBreakpointEnabledSetting.value = !breakpoint.disabled;
            for (let target of WI.targets)
                target.DebuggerAgent.setPauseOnDebuggerStatements(!breakpoint.disabled);
            return;

        case this._allExceptionsBreakpoint:
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._allExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            for (let target of WI.targets)
                target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
            return;

        case this._uncaughtExceptionsBreakpoint:
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._uncaughtExceptionsBreakpointEnabledSetting.value = !breakpoint.disabled;
            this._updateBreakOnExceptionsState();
            for (let target of WI.targets)
                target.DebuggerAgent.setPauseOnExceptions(this._breakOnExceptionsState);
            return;

        case this._assertionFailuresBreakpoint:
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._assertionFailuresBreakpointEnabledSetting.value = !breakpoint.disabled;
            for (let target of WI.targets)
                target.DebuggerAgent.setPauseOnAssertions(!breakpoint.disabled);
            return;

        case this._allMicrotasksBreakpoint:
            if (!breakpoint.disabled && !this.breakpointsDisabledTemporarily)
                this.breakpointsEnabled = true;
            this._allMicrotasksBreakpointEnabledSetting.value = !breakpoint.disabled;
            for (let target of WI.targets)
                target.DebuggerAgent.setPauseOnMicrotasks(!breakpoint.disabled);
            return;
        }

        if (!this._restoringBreakpoints)
            WI.objectStores.breakpoints.putObject(breakpoint);

        if (breakpoint.disabled)
            this._removeBreakpoint(breakpoint);
        else
            this._setBreakpoint(breakpoint);
    }

    _breakpointEditablePropertyDidChange(event)
    {
        let breakpoint = event.target;

        if (!this._restoringBreakpoints)
            WI.objectStores.breakpoints.putObject(breakpoint);

        if (breakpoint.disabled)
            return;

        console.assert(this.isBreakpointEditable(breakpoint));
        if (!this.isBreakpointEditable(breakpoint))
            return;

        // Remove the breakpoint with its old id.
        this._removeBreakpoint(breakpoint, (target) => {
            // Add the breakpoint with its new properties and get a new id.
            this._restoringBreakpoints = true;
            this._setBreakpoint(breakpoint, target);
            this._restoringBreakpoints = false;
        });
    }

    _handleBreakpointActionsDidChange(event)
    {
        this._breakpointEditablePropertyDidChange(event);

        this._updateProbesForBreakpoint(event.target);
    }

    _startDisablingBreakpointsTemporarily()
    {
        if (++this._temporarilyDisableBreakpointsRequestCount > 1)
            return;

        console.assert(!this.breakpointsDisabledTemporarily, "Already temporarily disabling breakpoints.");
        if (this.breakpointsDisabledTemporarily)
            return;


        this._temporarilyDisabledBreakpointsRestoreSetting.value = this._breakpointsEnabledSetting.value;

        this.breakpointsEnabled = false;
    }

    _stopDisablingBreakpointsTemporarily()
    {
        this._temporarilyDisableBreakpointsRequestCount = Math.max(0, this._temporarilyDisableBreakpointsRequestCount - 1);
        if (this._temporarilyDisableBreakpointsRequestCount > 0)
            return;

        console.assert(this.breakpointsDisabledTemporarily, "Was not temporarily disabling breakpoints.");
        if (!this.breakpointsDisabledTemporarily)
            return;

        let restoreState = this._temporarilyDisabledBreakpointsRestoreSetting.value;
        this._temporarilyDisabledBreakpointsRestoreSetting.value = null;

        this.breakpointsEnabled = restoreState;
    }

    _handleTimelineCapturingStateChanged(event)
    {
        switch (WI.timelineManager.capturingState) {
        case WI.TimelineManager.CapturingState.Starting:
            this._startDisablingBreakpointsTemporarily();
            if (this.paused)
                this.resume();
            break;

        case WI.TimelineManager.CapturingState.Inactive:
            this._stopDisablingBreakpointsTemporarily();
            break;
        }
    }

    _handleAuditManagerTestScheduled(event)
    {
        this._startDisablingBreakpointsTemporarily();

        if (this.paused)
            this.resume();
    }

    _handleAuditManagerTestCompleted(event)
    {
        this._stopDisablingBreakpointsTemporarily();
    }

    _targetRemoved(event)
    {
        let wasPaused = this.paused;

        this._targetDebuggerDataMap.delete(event.data.target);

        if (!this.paused && wasPaused)
            this.dispatchEventToListeners(WI.DebuggerManager.Event.Resumed);
    }

    _handleEngineeringShowInternalScriptsSettingChanged(event)
    {
        let eventType = WI.settings.engineeringShowInternalScripts.value ? WI.DebuggerManager.Event.ScriptAdded : WI.DebuggerManager.Event.ScriptRemoved;
        for (let script of this._internalWebKitScripts)
            this.dispatchEventToListeners(eventType, {script});
    }

    _handleEngineeringPauseForInternalScriptsSettingChanged(event)
    {
        for (let target of WI.targets) {
            if (target.hasCommand("Debugger.setPauseForInternalScripts"))
                target.DebuggerAgent.setPauseForInternalScripts(WI.settings.engineeringPauseForInternalScripts.value);
        }
    }

    _mainResourceDidChange(event)
    {
        if (!event.target.isMainFrame())
            return;

        this._didResumeInternal(WI.mainTarget);
    }

    _didResumeInternal(target)
    {
        if (!this.paused)
            return;

        if (this._delayedResumeTimeout) {
            clearTimeout(this._delayedResumeTimeout);
            this._delayedResumeTimeout = undefined;
        }

        let activeCallFrameDidChange = false;
        if (this._activeCallFrame && this._activeCallFrame.target === target) {
            this._activeCallFrame = null;
            activeCallFrameDidChange = true;
        }

        this.dataForTarget(target).updateForResume();

        if (!this.paused)
            this.dispatchEventToListeners(WI.DebuggerManager.Event.Resumed);

        this.dispatchEventToListeners(WI.DebuggerManager.Event.CallFramesDidChange, {target});

        if (activeCallFrameDidChange)
            this.dispatchEventToListeners(WI.DebuggerManager.Event.ActiveCallFrameDidChange);
    }

    _updateBreakOnExceptionsState()
    {
        let state = "none";

        if (this._breakpointsEnabledSetting.value) {
            if (!this._allExceptionsBreakpoint.disabled)
                state = "all";
            else if (!this._uncaughtExceptionsBreakpoint.disabled)
                state = "uncaught";
        }

        this._breakOnExceptionsState = state;

        switch (state) {
        case "all":
            // Mark the uncaught breakpoint as unresolved since "all" includes "uncaught".
            // That way it is clear in the user interface that the breakpoint is ignored.
            this._uncaughtExceptionsBreakpoint.resolved = false;
            break;
        case "uncaught":
        case "none":
            // Mark the uncaught breakpoint as resolved again.
            this._uncaughtExceptionsBreakpoint.resolved = true;
            break;
        }
    }

    _associateBreakpointsWithSourceCode(breakpoints, sourceCode)
    {
        this._ignoreBreakpointDisplayLocationDidChangeEvent = true;

        for (let breakpoint of breakpoints) {
            if (!breakpoint.sourceCodeLocation.sourceCode)
                breakpoint.sourceCodeLocation.sourceCode = sourceCode;
            // SourceCodes can be unequal if the SourceCodeLocation is associated with a Script and we are looking at the Resource.
            console.assert(breakpoint.sourceCodeLocation.sourceCode === sourceCode || breakpoint.sourceCodeLocation.sourceCode.contentIdentifier === sourceCode.contentIdentifier);
        }

        this._ignoreBreakpointDisplayLocationDidChangeEvent = false;
    }

    _addProbesForBreakpoint(breakpoint)
    {
        if (this._knownProbeIdentifiersForBreakpoint.has(breakpoint))
            return;

        this._knownProbeIdentifiersForBreakpoint.set(breakpoint, new Set);

        this._updateProbesForBreakpoint(breakpoint);
    }

    _removeProbesForBreakpoint(breakpoint)
    {
        console.assert(this._knownProbeIdentifiersForBreakpoint.has(breakpoint));

        this._updateProbesForBreakpoint(breakpoint);
        this._knownProbeIdentifiersForBreakpoint.delete(breakpoint);
    }

    _updateProbesForBreakpoint(breakpoint)
    {
        let knownProbeIdentifiers = this._knownProbeIdentifiersForBreakpoint.get(breakpoint);
        if (!knownProbeIdentifiers) {
            // Sometimes actions change before the added breakpoint is fully dispatched.
            this._addProbesForBreakpoint(breakpoint);
            return;
        }

        let seenProbeIdentifiers = new Set;

        for (let probeAction of breakpoint.probeActions) {
            let probeIdentifier = probeAction.id;
            console.assert(probeIdentifier, "Probe added without breakpoint action identifier: ", breakpoint);

            seenProbeIdentifiers.add(probeIdentifier);
            if (!knownProbeIdentifiers.has(probeIdentifier)) {
                // New probe; find or create relevant probe set.
                knownProbeIdentifiers.add(probeIdentifier);
                let probeSet = this._probeSetForBreakpoint(breakpoint);
                let newProbe = new WI.Probe(probeIdentifier, breakpoint, probeAction.data);
                this._probesByIdentifier.set(probeIdentifier, newProbe);
                probeSet.addProbe(newProbe);
                break;
            }

            let probe = this._probesByIdentifier.get(probeIdentifier);
            console.assert(probe, "Probe known but couldn't be found by identifier: ", probeIdentifier);
            // Update probe expression; if it differed, change events will fire.
            probe.expression = probeAction.data;
        }

        // Look for missing probes based on what we saw last.
        for (let probeIdentifier of knownProbeIdentifiers) {
            if (seenProbeIdentifiers.has(probeIdentifier))
                break;

            // The probe has gone missing, remove it.
            let probeSet = this._probeSetForBreakpoint(breakpoint);
            let probe = this._probesByIdentifier.get(probeIdentifier);
            this._probesByIdentifier.delete(probeIdentifier);
            knownProbeIdentifiers.delete(probeIdentifier);
            probeSet.removeProbe(probe);

            // Remove the probe set if it has become empty.
            if (!probeSet.probes.length) {
                this._probeSetsByBreakpoint.delete(probeSet.breakpoint);
                probeSet.willRemove();
                this.dispatchEventToListeners(WI.DebuggerManager.Event.ProbeSetRemoved, {probeSet});
            }
        }
    }

    _probeSetForBreakpoint(breakpoint)
    {
        let probeSet = this._probeSetsByBreakpoint.get(breakpoint);
        if (!probeSet) {
            probeSet = new WI.ProbeSet(breakpoint);
            this._probeSetsByBreakpoint.set(breakpoint, probeSet);
            this.dispatchEventToListeners(WI.DebuggerManager.Event.ProbeSetAdded, {probeSet});
        }
        return probeSet;
    }
};

WI.DebuggerManager.Event = {
    BreakpointAdded: "debugger-manager-breakpoint-added",
    BreakpointRemoved: "debugger-manager-breakpoint-removed",
    BreakpointMoved: "debugger-manager-breakpoint-moved",
    WaitingToPause: "debugger-manager-waiting-to-pause",
    Paused: "debugger-manager-paused",
    Resumed: "debugger-manager-resumed",
    CallFramesDidChange: "debugger-manager-call-frames-did-change",
    ActiveCallFrameDidChange: "debugger-manager-active-call-frame-did-change",
    ScriptAdded: "debugger-manager-script-added",
    ScriptRemoved: "debugger-manager-script-removed",
    ScriptsCleared: "debugger-manager-scripts-cleared",
    BreakpointsEnabledDidChange: "debugger-manager-breakpoints-enabled-did-change",
    ProbeSetAdded: "debugger-manager-probe-set-added",
    ProbeSetRemoved: "debugger-manager-probe-set-removed",
    BlackboxChanged: "blackboxed-urls-changed",
};

WI.DebuggerManager.PauseReason = {
    AnimationFrame: "animation-frame",
    Assertion: "assertion",
    BlackboxedScript: "blackboxed-script",
    Breakpoint: "breakpoint",
    CSPViolation: "CSP-violation",
    DebuggerStatement: "debugger-statement",
    DOM: "DOM",
    Exception: "exception",
    Fetch: "fetch",
    Interval: "interval",
    Listener: "listener",
    Microtask: "microtask",
    PauseOnNextStatement: "pause-on-next-statement",
    Timeout: "timeout",
    XHR: "xhr",
    Other: "other",

    // COMPATIBILITY (iOS 13): DOMDebugger.EventBreakpointType.Timer was replaced by DOMDebugger.EventBreakpointType.Interval and DOMDebugger.EventBreakpointType.Timeout.
    Timer: "timer",

    // COMPATIBILITY (iOS 13): DOMDebugger.EventBreakpointType.EventListener was replaced by DOMDebugger.EventBreakpointType.Listener.
    EventListener: "event-listener",
};

WI.DebuggerManager.BlackboxType = {
    Pattern: "pattern",
    URL: "url",
};
