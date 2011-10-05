/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @constructor
 * @extends {WebInspector.Object}
 */
WebInspector.DebuggerPresentationModel = function()
{
    // FIXME: apply formatter from outside as a generic mapping.
    this._formatter = new WebInspector.ScriptFormatter();
    this._rawSourceCode = {};
    this._presentationCallFrames = [];

    this._breakpointManager = new WebInspector.BreakpointManager(WebInspector.settings.breakpoints, this._breakpointAdded.bind(this), this._breakpointRemoved.bind(this), WebInspector.debuggerModel);

    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.ParsedScriptSource, this._parsedScriptSource, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.FailedToParseScriptSource, this._failedToParseScriptSource, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerPaused, this._debuggerPaused, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerResumed, this._debuggerResumed, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.Reset, this._debuggerReset, this);

    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.MessageAdded, this._consoleMessageAdded, this);
    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.ConsoleCleared, this._consoleCleared, this);

    new WebInspector.DebuggerPresentationModelResourceBinding(this);
}

WebInspector.DebuggerPresentationModel.Events = {
    UISourceCodeAdded: "source-file-added",
    UISourceCodeReplaced: "source-file-replaced",
    ConsoleMessageAdded: "console-message-added",
    ConsoleMessagesCleared: "console-messages-cleared",
    BreakpointAdded: "breakpoint-added",
    BreakpointRemoved: "breakpoint-removed",
    DebuggerPaused: "debugger-paused",
    DebuggerResumed: "debugger-resumed",
    CallFrameSelected: "call-frame-selected",
    ExecutionLineChanged: "execution-line-changed"
}

WebInspector.DebuggerPresentationModel.prototype = {
    createLinkifier: function()
    {
        return new WebInspector.DebuggerPresentationModel.Linkifier(this);
    },

    createPlacard: function(callFrame)
    {
        var title = callFrame._callFrame.functionName || WebInspector.UIString("(anonymous function)");
        var placard = new WebInspector.Placard(title, "");

        var rawSourceCode = callFrame._rawSourceCode;
        function updatePlacard()
        {
            var uiLocation = rawSourceCode.sourceMapping.rawLocationToUILocation(callFrame._callFrame.location);
            placard.subtitle = WebInspector.displayNameForURL(uiLocation.uiSourceCode.url) + ":" + (uiLocation.lineNumber + 1);
            placard._text = WebInspector.UIString("%s() at %s", placard.title, placard.subtitle);
        }
        if (rawSourceCode.sourceMapping)
            updatePlacard.call(this);
        rawSourceCode.addEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, updatePlacard, this);
        return placard;
    },

    _parsedScriptSource: function(event)
    {
        this._addScript(event.data);
    },

    _failedToParseScriptSource: function(event)
    {
        this._addScript(event.data);
    },

    _addScript: function(script)
    {
        var rawSourceCodeId = this._createRawSourceCodeId(script);
        var rawSourceCode = this._rawSourceCode[rawSourceCodeId];
        if (rawSourceCode) {
            rawSourceCode.addScript(script);
            return;
        }

        var resource;
        if (script.sourceURL)
            resource = WebInspector.networkManager.inflightResourceForURL(script.sourceURL) || WebInspector.resourceForURL(script.sourceURL);
        rawSourceCode = new WebInspector.RawSourceCode(rawSourceCodeId, script, resource, this._formatter, this._formatSource);
        this._rawSourceCode[rawSourceCodeId] = rawSourceCode;
        if (rawSourceCode.sourceMapping)
            this._updateSourceMapping(rawSourceCode, null);
        rawSourceCode.addEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, this._sourceMappingUpdated, this);
    },

    _sourceMappingUpdated: function(event)
    {
        var rawSourceCode = event.target;
        var oldUISourceCode = event.data.oldUISourceCode;
        this._updateSourceMapping(rawSourceCode, oldUISourceCode);
    },

    _updateSourceMapping: function(rawSourceCode, oldUISourceCode)
    {
        if (oldUISourceCode) {
            var breakpoints = this._breakpointManager.breakpointsForUISourceCode(oldUISourceCode);
            for (var lineNumber in breakpoints) {
                var breakpoint = breakpoints[lineNumber];
                this._breakpointRemoved(breakpoint);
                delete breakpoint.uiSourceCode;
            }
        }

        this._restoreBreakpoints(rawSourceCode);
        this._restoreConsoleMessages(rawSourceCode);

        var uiSourceCode = rawSourceCode.sourceMapping.uiSourceCode;
        if (!oldUISourceCode)
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.UISourceCodeAdded, uiSourceCode);
        else {
            var eventData = { uiSourceCode: uiSourceCode, oldUISourceCode: oldUISourceCode };
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.UISourceCodeReplaced, eventData);
        }
    },

    _restoreBreakpoints: function(rawSourceCode)
    {
        var uiSourceCode = rawSourceCode.sourceMapping.uiSourceCode;
        this._breakpointManager.uiSourceCodeAdded(uiSourceCode);
        var breakpoints = this._breakpointManager.breakpointsForUISourceCode(uiSourceCode);
        for (var lineNumber in breakpoints)
            this._breakpointAdded(breakpoints[lineNumber]);
    },

    _restoreConsoleMessages: function(rawSourceCode)
    {
        var messages = rawSourceCode.messages;
        for (var i = 0; i < messages.length; ++i)
            messages[i]._presentationMessage = this._createPresentationMessage(messages[i], rawSourceCode.sourceMapping);
    },

    canEditScriptSource: function(uiSourceCode)
    {
        if (!Preferences.canEditScriptSource || this._formatSource)
            return false;
        var rawSourceCode = uiSourceCode.rawSourceCode;
        var script = this._scriptForRawSourceCode(rawSourceCode);
        return script && !script.lineOffset && !script.columnOffset;
    },

    setScriptSource: function(uiSourceCode, newSource, callback)
    {
        var rawSourceCode = uiSourceCode.rawSourceCode;
        var script = this._scriptForRawSourceCode(rawSourceCode);

        function didEditScriptSource(error)
        {
            callback(error);
            if (error)
                return;

            var resource = WebInspector.resourceForURL(rawSourceCode.url);
            if (resource)
                resource.addRevision(newSource);

            rawSourceCode.contentEdited();

            if (WebInspector.debuggerModel.callFrames)
                this._debuggerPaused();
        }
        WebInspector.debuggerModel.setScriptSource(script.scriptId, newSource, didEditScriptSource.bind(this));
    },

    _updateBreakpointsAfterLiveEdit: function(uiSourceCode, oldSource, newSource)
    {
        var breakpoints = this._breakpointManager.breakpointsForUISourceCode(uiSourceCode);

        // Clear and re-create breakpoints according to text diff.
        var diff = Array.diff(oldSource.split("\n"), newSource.split("\n"));
        for (var lineNumber in breakpoints) {
            var breakpoint = breakpoints[lineNumber];

            this.removeBreakpoint(uiSourceCode, lineNumber);

            var newLineNumber = diff.left[lineNumber].row;
            if (newLineNumber === undefined) {
                for (var i = lineNumber - 1; i >= 0; --i) {
                    if (diff.left[i].row === undefined)
                        continue;
                    var shiftedLineNumber = diff.left[i].row + lineNumber - i;
                    if (shiftedLineNumber < diff.right.length) {
                        var originalLineNumber = diff.right[shiftedLineNumber].row;
                        if (originalLineNumber === lineNumber || originalLineNumber === undefined)
                            newLineNumber = shiftedLineNumber;
                    }
                    break;
                }
            }
            if (newLineNumber !== undefined)
                this.setBreakpoint(uiSourceCode, newLineNumber, breakpoint.condition, breakpoint.enabled);
        }
    },

    setFormatSource: function(formatSource)
    {
        if (this._formatSource === formatSource)
            return;

        this._formatSource = formatSource;
        this._breakpointManager.reset();
        for (var id in this._rawSourceCode)
            this._rawSourceCode[id].setFormatted(this._formatSource);

        if (WebInspector.debuggerModel.callFrames)
            this._debuggerPaused();
    },

    _consoleMessageAdded: function(event)
    {
        var message = event.data;
        if (!message.url || !message.isErrorOrWarning() || !message.message)
            return;

        var rawSourceCode = this._rawSourceCodeForScriptWithURL(message.url);
        if (!rawSourceCode)
            return;

        rawSourceCode.messages.push(message);
        if (rawSourceCode.sourceMapping) {
            message._presentationMessage = this._createPresentationMessage(message, rawSourceCode.sourceMapping);
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ConsoleMessageAdded, message._presentationMessage);
        }
    },

    _createPresentationMessage: function(message, sourceMapping)
    {
        // FIXME(62725): stack trace line/column numbers are one-based.
        var lineNumber = message.stackTrace ? message.stackTrace[0].lineNumber - 1 : message.line - 1;
        var columnNumber = message.stackTrace ? message.stackTrace[0].columnNumber - 1 : 0;
        var uiLocation = sourceMapping.rawLocationToUILocation({ lineNumber: lineNumber, columnNumber: columnNumber });
        var presentationMessage = {};
        presentationMessage.uiSourceCode = uiLocation.uiSourceCode;
        presentationMessage.lineNumber = uiLocation.lineNumber;
        presentationMessage.originalMessage = message;
        return presentationMessage;
    },

    _consoleCleared: function()
    {
        for (var id in this._rawSourceCode)
            this._rawSourceCode[id].messages = [];
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ConsoleMessagesCleared);
    },

    continueToLine: function(uiSourceCode, lineNumber)
    {
        // FIXME: use RawSourceCode.uiLocationToRawLocation.
        var rawLocation = uiSourceCode.rawSourceCode.sourceMapping.uiLocationToRawLocation(uiSourceCode, lineNumber);
        WebInspector.debuggerModel.continueToLocation(rawLocation);
    },

    breakpointsForUISourceCode: function(uiSourceCode)
    {
        var breakpointsMap = this._breakpointManager.breakpointsForUISourceCode(uiSourceCode);
        var breakpointsList = [];
        for (var lineNumber in breakpointsMap)
            breakpointsList.push(breakpointsMap[lineNumber]);
        return breakpointsList;
    },

    messagesForUISourceCode: function(uiSourceCode)
    {
        var rawSourceCode = uiSourceCode.rawSourceCode;
        var messages = [];
        for (var i = 0; i < rawSourceCode.messages.length; ++i)
            messages.push(rawSourceCode.messages[i]._presentationMessage);
        return messages;
    },

    setBreakpoint: function(uiSourceCode, lineNumber, condition, enabled)
    {
        this._breakpointManager.setBreakpoint(uiSourceCode, lineNumber, condition, enabled);
    },

    setBreakpointEnabled: function(uiSourceCode, lineNumber, enabled)
    {
        var breakpoint = this.findBreakpoint(uiSourceCode, lineNumber);
        if (!breakpoint)
            return;
        this._breakpointManager.removeBreakpoint(uiSourceCode, lineNumber);
        this._breakpointManager.setBreakpoint(uiSourceCode, lineNumber, breakpoint.condition, enabled);
    },

    updateBreakpoint: function(uiSourceCode, lineNumber, condition, enabled)
    {
        this._breakpointManager.removeBreakpoint(uiSourceCode, lineNumber);
        this._breakpointManager.setBreakpoint(uiSourceCode, lineNumber, condition, enabled);
    },

    removeBreakpoint: function(uiSourceCode, lineNumber)
    {
        this._breakpointManager.removeBreakpoint(uiSourceCode, lineNumber);
    },

    findBreakpoint: function(uiSourceCode, lineNumber)
    {
        return this._breakpointManager.breakpointsForUISourceCode(uiSourceCode)[lineNumber];
    },

    _breakpointAdded: function(breakpoint)
    {
        if (breakpoint.uiSourceCode)
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.BreakpointAdded, breakpoint);
    },

    _breakpointRemoved: function(breakpoint)
    {
        if (breakpoint.uiSourceCode)
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.BreakpointRemoved, breakpoint);
    },

    _debuggerPaused: function()
    {
        var callFrames = WebInspector.debuggerModel.callFrames;
        this._presentationCallFrames = [];
        for (var i = 0; i < callFrames.length; ++i) {
            var callFrame = callFrames[i];
            var script = WebInspector.debuggerModel.scriptForSourceID(callFrame.location.scriptId);
            if (!script)
                continue;
            var rawSourceCode = this._rawSourceCodeForScript(script);
            this._presentationCallFrames.push(new WebInspector.PresentationCallFrame(callFrame, i, this, rawSourceCode));
        }
        var details = WebInspector.debuggerModel.debuggerPausedDetails;
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.DebuggerPaused, { callFrames: this._presentationCallFrames, details: details });

        this.selectedCallFrame = this._presentationCallFrames[0];
    },

    _debuggerResumed: function()
    {
        this._presentationCallFrames = [];
        this.selectedCallFrame = null;
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.DebuggerResumed);
    },

    set selectedCallFrame(callFrame)
    {
        if (this._selectedCallFrame)
            this._selectedCallFrame.rawSourceCode.removeEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, this._dispatchExecutionLineChanged, this);
        this._selectedCallFrame = callFrame;
        if (!this._selectedCallFrame)
            return;

        this._selectedCallFrame.rawSourceCode.forceUpdateSourceMapping();
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.CallFrameSelected, callFrame);

        if (this._selectedCallFrame.rawSourceCode.sourceMapping)
            this._dispatchExecutionLineChanged(null);
        this._selectedCallFrame.rawSourceCode.addEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, this._dispatchExecutionLineChanged, this);
    },

    get selectedCallFrame()
    {
        return this._selectedCallFrame;
    },

    _dispatchExecutionLineChanged: function(event)
    {
        var rawLocation = this._selectedCallFrame._callFrame.location;
        var uiLocation = this._selectedCallFrame.rawSourceCode.sourceMapping.rawLocationToUILocation(rawLocation);
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ExecutionLineChanged, uiLocation);
    },

    /**
     * @param {string} sourceURL
     */
    _rawSourceCodeForScriptWithURL: function(sourceURL)
    {
        return this._rawSourceCode[sourceURL];
    },

    /**
     * @param {WebInspector.Script} script
     */
    _rawSourceCodeForScript: function(script)
    {
        return this._rawSourceCode[this._createRawSourceCodeId(script)];
    },

    _scriptForRawSourceCode: function(rawSourceCode)
    {
        function filter(script)
        {
            return this._createRawSourceCodeId(script) === rawSourceCode.id;
        }
        return WebInspector.debuggerModel.queryScripts(filter.bind(this))[0];
    },

    _createRawSourceCodeId: function(script)
    {
        return script.sourceURL || script.scriptId;
    },

    _debuggerReset: function()
    {
        for (var id in this._rawSourceCode)
            this._rawSourceCode[id].removeAllListeners();
        this._rawSourceCode = {};
        this._presentationCallFrames = [];
        this._selectedCallFrame = null;
        this._breakpointManager.debuggerReset();
    }
}

WebInspector.DebuggerPresentationModel.prototype.__proto__ = WebInspector.Object.prototype;

/**
 * @constructor
 */
WebInspector.PresentationCallFrame = function(callFrame, index, model, rawSourceCode)
{
    this._callFrame = callFrame;
    this._index = index;
    this._model = model;
    this._rawSourceCode = rawSourceCode;
}

WebInspector.PresentationCallFrame.prototype = {
    get type()
    {
        return this._callFrame.type;
    },

    get scopeChain()
    {
        return this._callFrame.scopeChain;
    },

    get this()
    {
        return this._callFrame.this;
    },

    get index()
    {
        return this._index;
    },

    get rawSourceCode()
    {
        return this._rawSourceCode;
    },

    evaluate: function(code, objectGroup, includeCommandLineAPI, returnByValue, callback)
    {
        function didEvaluateOnCallFrame(error, result, wasThrown)
        {
            if (error) {
                console.error(error);
                callback(null);
                return;
            }

            if (returnByValue && !wasThrown)
                callback(result, wasThrown);
            else
                callback(WebInspector.RemoteObject.fromPayload(result), wasThrown);
        }
        DebuggerAgent.evaluateOnCallFrame(this._callFrame.callFrameId, code, objectGroup, includeCommandLineAPI, returnByValue, didEvaluateOnCallFrame.bind(this));
    },

    uiLocation: function(callback)
    {
        function sourceMappingReady()
        {
            this._rawSourceCode.removeEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, sourceMappingReady, this);
            callback(this._rawSourceCode.sourceMapping.rawLocationToUILocation(this._callFrame.location));
        }
        if (this._rawSourceCode.sourceMapping)
            sourceMappingReady.call(this);
        else
            this._rawSourceCode.addEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, sourceMappingReady, this);
    }
}

/**
 * @constructor
 * @implements {WebInspector.ResourceDomainModelBinding}
 */
WebInspector.DebuggerPresentationModelResourceBinding = function(model)
{
    this._presentationModel = model;
    WebInspector.Resource.registerDomainModelBinding(WebInspector.Resource.Type.Script, this);
}

WebInspector.DebuggerPresentationModelResourceBinding.prototype = {
    canSetContent: function(resource)
    {
        var rawSourceCode = this._presentationModel._rawSourceCodeForScriptWithURL(resource.url)
        if (!rawSourceCode)
            return false;
        return this._presentationModel.canEditScriptSource(rawSourceCode.sourceMapping.uiSourceCode);
    },

    setContent: function(resource, content, majorChange, userCallback)
    {
        if (!majorChange)
            return;

        var rawSourceCode = this._presentationModel._rawSourceCodeForScriptWithURL(resource.url);
        if (!rawSourceCode) {
            userCallback("Resource is not editable");
            return;
        }

        resource.requestContent(this._setContentWithInitialContent.bind(this, rawSourceCode.sourceMapping.uiSourceCode, content, userCallback));
    },

    _setContentWithInitialContent: function(uiSourceCode, content, userCallback, oldContent)
    {
        function callback(error)
        {
            if (userCallback)
                userCallback(error);
            if (!error)
                this._presentationModel._updateBreakpointsAfterLiveEdit(uiSourceCode, oldContent, content);
        }
        this._presentationModel.setScriptSource(uiSourceCode, content, callback.bind(this));
    }
}

/**
 * @constructor
 */
WebInspector.DebuggerPresentationModel.Linkifier = function(model)
{
    this._model = model;
    this._anchorsForRawSourceCode = {};
}

WebInspector.DebuggerPresentationModel.Linkifier.prototype = {
    linkifyLocation: function(sourceURL, lineNumber, columnNumber, classes)
    {
        var linkText = WebInspector.formatLinkText(sourceURL, lineNumber);
        var anchor = WebInspector.linkifyURLAsNode(sourceURL, linkText, classes, false);
        anchor.rawLocation = { lineNumber: lineNumber, columnNumber: columnNumber };

        var rawSourceCode = this._model._rawSourceCodeForScriptWithURL(sourceURL);
        if (!rawSourceCode) {
            anchor.setAttribute("preferred_panel", "resources");
            anchor.setAttribute("line_number", lineNumber);
            return anchor;
        }

        var anchors = this._anchorsForRawSourceCode[rawSourceCode.id];
        if (!anchors) {
            anchors = [];
            this._anchorsForRawSourceCode[rawSourceCode.id] = anchors;
            rawSourceCode.addEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, this._updateSourceAnchors, this);
        }

        if (rawSourceCode.sourceMapping)
            this._updateAnchor(rawSourceCode, anchor);
        anchors.push(anchor);
        return anchor;
    },

    reset: function()
    {
        for (var id in this._anchorsForRawSourceCode)
            this._model._rawSourceCode[id].removeEventListener(WebInspector.RawSourceCode.Events.SourceMappingUpdated, this._updateSourceAnchors, this);
        this._anchorsForRawSourceCode = {};
    },

    _updateSourceAnchors: function(event)
    {
        var rawSourceCode = event.target;
        var anchors = this._anchorsForRawSourceCode[rawSourceCode.id];
        for (var i = 0; i < anchors.length; ++i)
            this._updateAnchor(rawSourceCode, anchors[i]);
    },

    _updateAnchor: function(rawSourceCode, anchor)
    {
        var uiLocation = rawSourceCode.sourceMapping.rawLocationToUILocation(anchor.rawLocation);
        anchor.textContent = WebInspector.formatLinkText(uiLocation.uiSourceCode.url, uiLocation.lineNumber);
        anchor.setAttribute("preferred_panel", "scripts");
        anchor.uiSourceCode = uiLocation.uiSourceCode;
        anchor.lineNumber = uiLocation.lineNumber;
    }
}

WebInspector.DebuggerPresentationModelResourceBinding.prototype.__proto__ = WebInspector.ResourceDomainModelBinding.prototype;

/**
 * @type {?WebInspector.DebuggerPresentationModel}
 */
WebInspector.debuggerPresentationModel = null;
