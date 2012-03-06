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
 * @extends {WebInspector.ScriptMapping}
 */
WebInspector.DebuggerPresentationModel = function()
{
    // FIXME: apply formatter from outside as a generic mapping.
    this._formatter = new WebInspector.ScriptFormatter();
    this._rawSourceCodes = [];
    this._rawSourceCodeForScriptId = {};
    this._rawSourceCodeForURL = {};
    this._rawSourceCodeForDocumentURL = {};
    this._rawSourceCodeForUISourceCode = new Map();
    this._presentationCallFrames = [];
    this._formatSource = false;

    this._breakpointManager = new WebInspector.BreakpointManager(WebInspector.settings.breakpoints, this._breakpointAdded.bind(this), this._breakpointRemoved.bind(this), WebInspector.debuggerModel, this);

    this._pendingConsoleMessages = {};
    this._consoleMessageLiveLocations = [];
    this._presentationConsoleMessages = [];

    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.ParsedScriptSource, this._parsedScriptSource, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.FailedToParseScriptSource, this._failedToParseScriptSource, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerPaused, this._debuggerPaused, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.DebuggerResumed, this._debuggerResumed, this);
    WebInspector.debuggerModel.addEventListener(WebInspector.DebuggerModel.Events.GlobalObjectCleared, this._debuggerReset, this);

    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.MessageAdded, this._consoleMessageAdded, this);
    WebInspector.console.addEventListener(WebInspector.ConsoleModel.Events.ConsoleCleared, this._consoleCleared, this);

    new WebInspector.DebuggerPresentationModelResourceBinding(this);
}

WebInspector.DebuggerPresentationModel.Events = {
    UISourceCodeAdded: "source-file-added",
    UISourceCodeReplaced: "source-file-replaced",
    UISourceCodeRemoved: "source-file-removed",
    ConsoleMessageAdded: "console-message-added",
    ConsoleMessagesCleared: "console-messages-cleared",
    BreakpointAdded: "breakpoint-added",
    BreakpointRemoved: "breakpoint-removed",
    DebuggerPaused: "debugger-paused",
    DebuggerResumed: "debugger-resumed",
    DebuggerReset: "debugger-reset",
    CallFrameSelected: "call-frame-selected",
    ConsoleCommandEvaluatedInSelectedCallFrame: "console-command-evaluated-in-selected-call-frame",
    ExecutionLineChanged: "execution-line-changed"
}

WebInspector.DebuggerPresentationModel.prototype = {
    /**
     * @param {WebInspector.DebuggerPresentationModel.LinkifierFormatter=} formatter
     */
    createLinkifier: function(formatter)
    {
        return new WebInspector.DebuggerPresentationModel.Linkifier(this, formatter);
    },

    /**
     * @param {WebInspector.PresentationCallFrame} callFrame
     * @return {WebInspector.DebuggerPresentationModel.CallFramePlacard}
     */
    createPlacard: function(callFrame)
    {
        return new WebInspector.DebuggerPresentationModel.CallFramePlacard(callFrame, this);
    },

    /**
     * @param {DebuggerAgent.Location} rawLocation
     * @return {?WebInspector.UILocation}
     */
    rawLocationToUILocation: function(rawLocation)
    {
        var rawSourceCode = this._rawSourceCodeForScriptId[rawLocation.scriptId];
        return rawSourceCode.rawLocationToUILocation(rawLocation);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {number} columnNumber
     * @return {DebuggerAgent.Location}
     */
    uiLocationToRawLocation: function(uiSourceCode, lineNumber, columnNumber)
    {
        var rawSourceCode = this._rawSourceCodeForUISourceCode.get(uiSourceCode);
        return rawSourceCode.uiLocationToRawLocation(uiSourceCode, lineNumber, columnNumber);
    },

    /**
     * @param {DebuggerAgent.Location} rawLocation
     * @param {function(WebInspector.UILocation)} updateDelegate
     * @return {WebInspector.RawSourceCode.LiveLocation}
     */
    createLiveLocation: function(rawLocation, updateDelegate)
    {
        var rawSourceCode = this._rawSourceCodeForScriptId[rawLocation.scriptId];
        return new WebInspector.RawSourceCode.LiveLocation(rawSourceCode, rawLocation, updateDelegate);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _parsedScriptSource: function(event)
    {
        var script = /** @type {WebInspector.Script} */ event.data;
        this._addScript(script);
        this._addPendingConsoleMessagesToScript(script);
    },

    /**
     * @param {WebInspector.Event} event
     */
    _failedToParseScriptSource: function(event)
    {
        this._parsedScriptSource(event);
    },

    /**
     * @param {WebInspector.Script} script
     */
    _addScript: function(script)
    {
        var resource = null;
        var isInlineScript = false;
        if (script.isInlineScript()) {
            resource = WebInspector.networkManager.inflightResourceForURL(script.sourceURL) || WebInspector.resourceForURL(script.sourceURL);
            if (resource && resource.type === WebInspector.Resource.Type.Document) {
                isInlineScript = true;
                var rawSourceCode = this._rawSourceCodeForDocumentURL[script.sourceURL];
                if (rawSourceCode) {
                    rawSourceCode.addScript(script);
                    this._bindScriptToRawSourceCode(script, rawSourceCode);
                    return;
                }
            }
        }

        var compilerSourceMapping = null;
        if (WebInspector.settings.sourceMapsEnabled.get() && script.sourceMapURL)
            compilerSourceMapping = new WebInspector.ClosureCompilerSourceMapping(script.sourceMapURL, script.sourceURL);

        var rawSourceCode = new WebInspector.RawSourceCode(script.scriptId, script, resource, this._formatter, this._formatSource, compilerSourceMapping);
        this._rawSourceCodes.push(rawSourceCode);
        this._bindScriptToRawSourceCode(script, rawSourceCode);

        if (isInlineScript)
            this._rawSourceCodeForDocumentURL[script.sourceURL] = rawSourceCode;

        if (rawSourceCode.uiSourceCodeList().length)
            this._uiSourceCodeListChanged(rawSourceCode, [], rawSourceCode.uiSourceCodeList());
        rawSourceCode.addEventListener(WebInspector.RawSourceCode.Events.UISourceCodeListChanged, this._handleUISourceCodeListChanged, this);
    },

    /**
     * @param {WebInspector.Script} script
     * @param {WebInspector.RawSourceCode} rawSourceCode
     */
    _bindScriptToRawSourceCode: function(script, rawSourceCode)
    {
        this._rawSourceCodeForScriptId[script.scriptId] = rawSourceCode;
        this._rawSourceCodeForURL[script.sourceURL] = rawSourceCode;
    },

    /**
     * @return {Array.<WebInspector.UISourceCode>}
     */
    uiSourceCodes: function()
    {
        var result = [];
        for (var i = 0; i < this._rawSourceCodes.length; ++i) {
            var uiSourceCodeList = this._rawSourceCodes[i].uiSourceCodeList();
            for (var j = 0; j < uiSourceCodeList.length; ++j)
                result.push(uiSourceCodeList[j]);
        }
        return result;
    },

    /**
     * @param {WebInspector.Event} event
     */
    _handleUISourceCodeListChanged: function(event)
    {
        var rawSourceCode = /** @type {WebInspector.RawSourceCode} */ event.target;
        var oldUISourceCodeList = /** @type {Array.<WebInspector.UISourceCode>} */ event.data["oldUISourceCodeList"];
        this._uiSourceCodeListChanged(rawSourceCode, oldUISourceCodeList, rawSourceCode.uiSourceCodeList());
    },

    /**
     * @param {WebInspector.RawSourceCode} rawSourceCode
     * @param {Array.<WebInspector.UISourceCode>} removedItems
     * @param {Array.<WebInspector.UISourceCode>} addedItems
     */
    _uiSourceCodeListChanged: function(rawSourceCode, removedItems, addedItems)
    {
        for (var i = 0; i < removedItems.length; ++i)
            this._rawSourceCodeForUISourceCode.remove(removedItems[i]);
        for (var i = 0; i < addedItems.length; ++i)
            this._rawSourceCodeForUISourceCode.put(addedItems[i], rawSourceCode);

        for (var i = 0; i < removedItems.length; ++i) {
            var breakpoints = this._breakpointManager.breakpointsForUISourceCode(removedItems[i]);
            for (var lineNumber in breakpoints) {
                var breakpoint = breakpoints[lineNumber];
                this._breakpointRemoved(breakpoint);
                delete breakpoint.uiSourceCode;
            }
        }

        this._restoreBreakpoints(addedItems);

        if (!removedItems.length) {
            for (var i = 0; i < addedItems.length; ++i)
                this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.UISourceCodeAdded, addedItems[i]);
        } else if (!addedItems.length) {
            for (var i = 0; i < addedItems.length; ++i)
                this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.UISourceCodeRemoved, removedItems[i]);
        } else {
            var eventData = { uiSourceCodeList: addedItems, oldUISourceCodeList: removedItems };
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.UISourceCodeReplaced, eventData);
        }
    },

    /**
     * @param {Array.<WebInspector.UISourceCode>} uiSourceCodeList
     */
    _restoreBreakpoints: function(uiSourceCodeList)
    {
        for (var i = 0; i < uiSourceCodeList.length; ++i) {
            var uiSourceCode = uiSourceCodeList[i];
            this._breakpointManager.uiSourceCodeAdded(uiSourceCode);
            var breakpoints = this._breakpointManager.breakpointsForUISourceCode(uiSourceCode);
            for (var lineNumber in breakpoints)
                this._breakpointAdded(breakpoints[lineNumber]);
        }
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {boolean}
     */
    canEditScriptSource: function(uiSourceCode)
    {
        return WebInspector.debuggerModel.canSetScriptSource() && uiSourceCode.isEditable;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} newSource
     * @param {function(?Protocol.Error)} callback
     */
    setScriptSource: function(uiSourceCode, newSource, callback)
    {
        var rawLocation = this.uiLocationToRawLocation(uiSourceCode, 0, 0);
        var script = WebInspector.debuggerModel.scriptForSourceID(rawLocation.scriptId);

        /**
         * @this {WebInspector.DebuggerPresentationModel}
         * @param {?Protocol.Error} error
         */
        function didEditScriptSource(error)
        {
            callback(error);
            if (error)
                return;

            var resource = WebInspector.resourceForURL(script.sourceURL);
            if (resource)
                resource.addRevision(newSource);

            uiSourceCode.contentChanged(newSource);

            if (WebInspector.debuggerModel.callFrames)
                this._debuggerPaused();
        }
        WebInspector.debuggerModel.setScriptSource(script.scriptId, newSource, didEditScriptSource.bind(this));
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} oldSource
     * @param {string} newSource
     */
    _updateBreakpointsAfterLiveEdit: function(uiSourceCode, oldSource, newSource)
    {
        var breakpoints = this._breakpointManager.breakpointsForUISourceCode(uiSourceCode);

        // Clear and re-create breakpoints according to text diff.
        var diff = Array.diff(oldSource.split("\n"), newSource.split("\n"));
        for (var lineNumber in breakpoints) {
            var breakpoint = breakpoints[lineNumber];

            this.removeBreakpoint(uiSourceCode, parseInt(lineNumber, 10));

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

    /**
     * @param {boolean} formatSource
     */
    setFormatSource: function(formatSource)
    {
        if (this._formatSource === formatSource)
            return;

        this._formatSource = formatSource;
        this._breakpointManager.reset();
        for (var i = 0; i < this._rawSourceCodes.length; ++i)
            this._rawSourceCodes[i].setFormatted(this._formatSource);
    },

    /**
     * @param {DebuggerAgent.Location} rawLocation
     */
    _forceUpdateSourceMapping: function(rawLocation)
    {
        var rawSourceCode = this._rawSourceCodeForScriptId[rawLocation.scriptId];
        rawSourceCode.forceUpdateSourceMapping();
    },

    /**
     * @param {WebInspector.Event} event
     */
    _consoleMessageAdded: function(event)
    {
        var message = /** @type {WebInspector.ConsoleMessage} */ event.data;
        if (!message.url || !message.isErrorOrWarning())
            return;

        var script = this._scriptForURLAndLocation(message.url, message.location);
        if (script)
            this._addConsoleMessageToScript(message, script);
        else
            this._addPendingConsoleMessage(message);
    },

    /**
     * @param {WebInspector.ConsoleMessage} message
     * @param {WebInspector.Script} script
     */
    _addConsoleMessageToScript: function(message, script)
    {
        function updateLocation(uiLocation)
        {
            var presentationMessage = new WebInspector.PresentationConsoleMessage(uiLocation.uiSourceCode, uiLocation.lineNumber, message);
            this._presentationConsoleMessages.push(presentationMessage);
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ConsoleMessageAdded, presentationMessage);
        }
        var rawLocation = new WebInspector.DebuggerModel.Location(message.location.lineNumber, message.location.columnNumber);
        rawLocation.scriptId = script.scriptId;
        var liveLocation = this.createLiveLocation(rawLocation, updateLocation.bind(this));
        liveLocation.init();
        this._consoleMessageLiveLocations.push(liveLocation);
    },

    /**
     * @param {WebInspector.ConsoleMessage} message
     */
    _addPendingConsoleMessage: function(message)
    {
        if (!this._pendingConsoleMessages[message.url])
            this._pendingConsoleMessages[message.url] = [];
        this._pendingConsoleMessages[message.url].push(message);
    },

    /**
     * @param {WebInspector.Script} script
     */
    _addPendingConsoleMessagesToScript: function(script)
    {
        var messages = this._pendingConsoleMessages[script.sourceURL];
        if (!messages)
            return;

        var pendingMessages = [];
        for (var i = 0; i < messages.length; i++) {
            var message = messages[i];
            if (script === this._scriptForURLAndLocation(message.url, message.location))
                this._addConsoleMessageToScript(messages, script);
            else
                pendingMessages.push(message);
        }

        if (pendingMessages.length)
            this._pendingConsoleMessages[script.sourceURL] = pendingMessages;
        else
            delete this._pendingConsoleMessages[script.sourceURL];
    },

    _consoleCleared: function()
    {
        this._pendingConsoleMessages = {};
        for (var i = 0; i < this._consoleMessageLiveLocations.length; ++i)
            this._consoleMessageLiveLocations[i].dispose();
        this._consoleMessageLiveLocations = [];
        this._presentationConsoleMessages = [];
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ConsoleMessagesCleared);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     */
    continueToLine: function(uiSourceCode, lineNumber)
    {
        var rawLocation = this.uiLocationToRawLocation(uiSourceCode, lineNumber, 0);
        WebInspector.debuggerModel.continueToLocation(rawLocation);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {Array.<WebInspector.Breakpoint>}
     */
    breakpointsForUISourceCode: function(uiSourceCode)
    {
        var breakpointsMap = this._breakpointManager.breakpointsForUISourceCode(uiSourceCode);
        var breakpointsList = [];
        for (var lineNumber in breakpointsMap)
            breakpointsList.push(breakpointsMap[lineNumber]);
        return breakpointsList;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @return {Array.<WebInspector.PresentationConsoleMessage>}
     */
    messagesForUISourceCode: function(uiSourceCode)
    {
        var messages = [];
        for (var i = 0; i < this._presentationConsoleMessages.length; ++i) {
            var message = this._presentationConsoleMessages[i];
            if (message.uiSourceCode === uiSourceCode)
                messages.push(message);
        }
        return messages;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {string} condition
     * @param {boolean} enabled
     */
    setBreakpoint: function(uiSourceCode, lineNumber, condition, enabled)
    {
        this._breakpointManager.setBreakpoint(uiSourceCode, lineNumber, condition, enabled);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {boolean} enabled
     */
    setBreakpointEnabled: function(uiSourceCode, lineNumber, enabled)
    {
        var breakpoint = this.findBreakpoint(uiSourceCode, lineNumber);
        if (!breakpoint)
            return;
        this._breakpointManager.removeBreakpoint(uiSourceCode, lineNumber);
        this._breakpointManager.setBreakpoint(uiSourceCode, lineNumber, breakpoint.condition, enabled);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @param {string} condition
     * @param {boolean} enabled
     */
    updateBreakpoint: function(uiSourceCode, lineNumber, condition, enabled)
    {
        this._breakpointManager.removeBreakpoint(uiSourceCode, lineNumber);
        this._breakpointManager.setBreakpoint(uiSourceCode, lineNumber, condition, enabled);
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     */
    removeBreakpoint: function(uiSourceCode, lineNumber)
    {
        this._breakpointManager.removeBreakpoint(uiSourceCode, lineNumber);
    },

    /**
     */
    removeAllBreakpoints: function()
    {
        this._breakpointManager.removeAllBreakpoints();
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {number} lineNumber
     * @return {WebInspector.Breakpoint|undefined}
     */
    findBreakpoint: function(uiSourceCode, lineNumber)
    {
        return this._breakpointManager.breakpointsForUISourceCode(uiSourceCode)[lineNumber];
    },

    /**
     * @param {WebInspector.Breakpoint} breakpoint
     */
    _breakpointAdded: function(breakpoint)
    {
        if (breakpoint.uiSourceCode)
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.BreakpointAdded, breakpoint);
    },

    /**
     * @param {WebInspector.Breakpoint} breakpoint
     */
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
            if (WebInspector.debuggerModel.scriptForSourceID(callFrame.location.scriptId))
                this._presentationCallFrames.push(new WebInspector.PresentationCallFrame(callFrame, i, this));
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

    get paused()
    {
        return !!WebInspector.debuggerModel.debuggerPausedDetails;
    },

    set selectedCallFrame(callFrame)
    {
        if (this._executionLineLiveLocation)
            this._executionLineLiveLocation.dispose();
        delete this._executionLineLiveLocation;

        this._selectedCallFrame = callFrame;
        if (!this._selectedCallFrame)
            return;

        this._forceUpdateSourceMapping(callFrame._callFrame.location);
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.CallFrameSelected, callFrame);

        function updateExecutionLine(uiLocation)
        {
            this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ExecutionLineChanged, uiLocation);
        }
        this._executionLineLiveLocation = this.createLiveLocation(callFrame._callFrame.location, updateExecutionLine.bind(this));
        this._executionLineLiveLocation.init();
    },

    get selectedCallFrame()
    {
        return this._selectedCallFrame;
    },

    /**
     * @param {function(?WebInspector.RemoteObject, boolean, RuntimeAgent.RemoteObject=)} callback
     */
    evaluateInSelectedCallFrame: function(code, objectGroup, includeCommandLineAPI, returnByValue, callback)
    {
        /**
         * @param {?RuntimeAgent.RemoteObject} result
         * @param {boolean} wasThrown
         */
        function didEvaluate(result, wasThrown)
        {
            if (returnByValue)
                callback(null, wasThrown, wasThrown ? null : result);
            else
                callback(WebInspector.RemoteObject.fromPayload(result), wasThrown);

            if (objectGroup === "console")
                this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.ConsoleCommandEvaluatedInSelectedCallFrame);
        }

        this.selectedCallFrame.evaluate(code, objectGroup, includeCommandLineAPI, returnByValue, didEvaluate.bind(this));
    },

    /**
     * @param {function(Object)} callback
     */
    getSelectedCallFrameVariables: function(callback)
    {
        var result = { this: true };

        var selectedCallFrame = this.selectedCallFrame;
        if (!selectedCallFrame)
            callback(result);

        var pendingRequests = 0;

        function propertiesCollected(properties)
        {
            for (var i = 0; properties && i < properties.length; ++i)
                result[properties[i].name] = true;
            if (--pendingRequests == 0)
                callback(result);
        }

        for (var i = 0; i < selectedCallFrame.scopeChain.length; ++i) {
            var scope = selectedCallFrame.scopeChain[i];
            var object = WebInspector.RemoteObject.fromPayload(scope.object);
            pendingRequests++;
            object.getAllProperties(propertiesCollected);
        }
    },

    /**
     * @param {string} url
     * @param {DebuggerAgent.Location} rawLocation
     * @return {WebInspector.Script}
     */
    _scriptForURLAndLocation: function(url, rawLocation)
    {
        var scripts = WebInspector.debuggerModel.scriptsForURL(url);
        for (var i = 0; i < scripts.length; ++i) {
            var script = scripts[i];
            if (script.lineOffset > rawLocation.lineNumber || (script.lineOffset === rawLocation.lineNumber && script.columnOffset > rawLocation.columnNumber))
                continue;
            if (script.endLine < rawLocation.lineNumber || (script.endLine === rawLocation.lineNumber && script.endColumn <= rawLocation.columnNumber))
                continue;
            return script;
        }
        return null;
    },

    _debuggerReset: function()
    {
        for (var i = 0; i < this._rawSourceCodes.length; ++i) {
            var rawSourceCode = this._rawSourceCodes[i];
            this._uiSourceCodeListChanged(rawSourceCode, rawSourceCode.uiSourceCodeList(), []);
            rawSourceCode.removeAllListeners();
        }
        this._rawSourceCodes = [];
        this._rawSourceCodeForScriptId = {};
        this._rawSourceCodeForURL = {};
        this._rawSourceCodeForDocumentURL = {};
        this._rawSourceCodeForUISourceCode.clear();
        this._presentationCallFrames = [];
        this._selectedCallFrame = null;
        this._breakpointManager.debuggerReset();
        this._pendingConsoleMessages = {};
        this._consoleMessageLiveLocations = [];
        this._presentationConsoleMessages = [];
        this.dispatchEventToListeners(WebInspector.DebuggerPresentationModel.Events.DebuggerReset);
    }
}

WebInspector.DebuggerPresentationModel.prototype.__proto__ = WebInspector.ScriptMapping.prototype;

/**
 * @constructor
 * @param {WebInspector.UISourceCode} uiSourceCode
 * @param {number} lineNumber
 * @param {WebInspector.ConsoleMessage} originalMessage
 */
WebInspector.PresentationConsoleMessage = function(uiSourceCode, lineNumber, originalMessage)
{
    this.uiSourceCode = uiSourceCode;
    this.lineNumber = lineNumber;
    this.originalMessage = originalMessage;
}

/**
 * @constructor
 * @param {DebuggerAgent.CallFrame} callFrame
 * @param {number} index
 * @param {WebInspector.ScriptMapping} scriptMapping
 */
WebInspector.PresentationCallFrame = function(callFrame, index, scriptMapping)
{
    this._callFrame = callFrame;
    this._index = index;
    this._scriptMapping = scriptMapping;
}

WebInspector.PresentationCallFrame.prototype = {
    /**
     * @return {string}
     */
    get type()
    {
        return this._callFrame.type;
    },

    /**
     * @return {Array.<DebuggerAgent.Scope>}
     */
    get scopeChain()
    {
        return this._callFrame.scopeChain;
    },

    /**
     * @return {RuntimeAgent.RemoteObject}
     */
    get this()
    {
        return this._callFrame.this;
    },

    /**
     * @return {number}
     */
    get index()
    {
        return this._index;
    },

    /**
     * @param {string} code
     * @param {string} objectGroup
     * @param {boolean} includeCommandLineAPI
     * @param {boolean} returnByValue
     * @param {function(?RuntimeAgent.RemoteObject, boolean=)=} callback
     */
    evaluate: function(code, objectGroup, includeCommandLineAPI, returnByValue, callback)
    {
        /**
         * @this {WebInspector.PresentationCallFrame}
         * @param {?Protocol.Error} error
         * @param {RuntimeAgent.RemoteObject} result
         * @param {boolean=} wasThrown
         */
        function didEvaluateOnCallFrame(error, result, wasThrown)
        {
            if (error) {
                console.error(error);
                callback(null, false);
                return;
            }
            callback(result, wasThrown);
        }
        DebuggerAgent.evaluateOnCallFrame(this._callFrame.callFrameId, code, objectGroup, includeCommandLineAPI, returnByValue, didEvaluateOnCallFrame.bind(this));
    },

    /**
     * @param {function(WebInspector.UILocation)} callback
     */
    uiLocation: function(callback)
    {
        function locationUpdated(uiLocation)
        {
            callback(uiLocation);
            liveLocation.dispose();
        }
        var liveLocation = this._scriptMapping.createLiveLocation(this._callFrame.location, locationUpdated.bind(this));
        liveLocation.init();
    }
}

/**
 * @constructor
 * @extends {WebInspector.Placard}
 * @param {WebInspector.PresentationCallFrame} callFrame
 * @param {WebInspector.ScriptMapping} scriptMapping
 */
WebInspector.DebuggerPresentationModel.CallFramePlacard = function(callFrame, scriptMapping)
{
    WebInspector.Placard.call(this, callFrame._callFrame.functionName || WebInspector.UIString("(anonymous function)"), "");
    this._liveLocation = scriptMapping.createLiveLocation(callFrame._callFrame.location, this._update.bind(this));
    this._liveLocation.init();
}

WebInspector.DebuggerPresentationModel.CallFramePlacard.prototype = {
    discard: function()
    {
        this._liveLocation.dispose();
    },

    _update: function(uiLocation)
    {
        this.subtitle = WebInspector.displayNameForURL(uiLocation.uiSourceCode.url) + ":" + (uiLocation.lineNumber + 1);
    }
}

WebInspector.DebuggerPresentationModel.CallFramePlacard.prototype.__proto__ = WebInspector.Placard.prototype;

/**
 * @constructor
 * @implements {WebInspector.ResourceDomainModelBinding}
 * @param {WebInspector.DebuggerPresentationModel} model
 */
WebInspector.DebuggerPresentationModelResourceBinding = function(model)
{
    this._presentationModel = model;
    WebInspector.Resource.registerDomainModelBinding(WebInspector.Resource.Type.Script, this);
}

WebInspector.DebuggerPresentationModelResourceBinding.prototype = {
    /**
     * @param {WebInspector.Resource} resource
     */
    canSetContent: function(resource)
    {
        var uiSourceCode = this._uiSourceCodeForResource(resource);
        return uiSourceCode && this._presentationModel.canEditScriptSource(uiSourceCode);
    },

    /**
     * @param {WebInspector.Resource} resource
     * @param {string} content
     * @param {boolean} majorChange
     * @param {function(?string)} userCallback
     */
    setContent: function(resource, content, majorChange, userCallback)
    {
        if (!majorChange)
            return;

        var uiSourceCode = this._uiSourceCodeForResource(resource);
        if (!uiSourceCode) {
            userCallback("Resource is not editable");
            return;
        }

        resource.requestContent(this._setContentWithInitialContent.bind(this, uiSourceCode, content, userCallback));
    },

    /**
     * @param {WebInspector.Resource} resource
     * @return {WebInspector.UISourceCode}
     */
    _uiSourceCodeForResource: function(resource)
    {
        var script = WebInspector.debuggerModel.scriptsForURL(resource.url)[0];
        if (!script)
            return null;
        var rawLocation = new WebInspector.DebuggerModel.Location(0, 0);
        rawLocation.scriptId = script.scriptId;
        var uiLocation = this._presentationModel.rawLocationToUILocation(rawLocation);
        return uiLocation ? uiLocation.uiSourceCode : null;
    },

    /**
     * @param {WebInspector.UISourceCode} uiSourceCode
     * @param {string} content
     * @param {function(?string)} userCallback
     * @param {?string} oldContent
     * @param {?string} oldContentEncoded
     */
    _setContentWithInitialContent: function(uiSourceCode, content, userCallback, oldContent, oldContentEncoded)
    {
        /**
         * @this {WebInspector.DebuggerPresentationModelResourceBinding}
         * @param {?string} error
         */
        function callback(error)
        {
            if (userCallback)
                userCallback(error);
            if (!error)
                this._presentationModel._updateBreakpointsAfterLiveEdit(uiSourceCode, oldContent || "", content);
        }
        this._presentationModel.setScriptSource(uiSourceCode, content, callback.bind(this));
    }
}

WebInspector.DebuggerPresentationModelResourceBinding.prototype.__proto__ = WebInspector.ResourceDomainModelBinding.prototype;

/**
 * @interface
 */
WebInspector.DebuggerPresentationModel.LinkifierFormatter = function()
{
}

WebInspector.DebuggerPresentationModel.LinkifierFormatter.prototype = {
    /**
     * @param {Element} anchor
     * @param {WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation) { },
}

/**
 * @constructor
 * @implements {WebInspector.DebuggerPresentationModel.LinkifierFormatter}
 * @param {number=} maxLength
 */
WebInspector.DebuggerPresentationModel.DefaultLinkifierFormatter = function(maxLength)
{
    this._maxLength = maxLength;
}

WebInspector.DebuggerPresentationModel.DefaultLinkifierFormatter.prototype = {
    /**
     * @param {Element} anchor
     * @param {WebInspector.UILocation} uiLocation
     */
    formatLiveAnchor: function(anchor, uiLocation)
    {
        anchor.textContent = WebInspector.formatLinkText(uiLocation.uiSourceCode.url, uiLocation.lineNumber);

        var text = WebInspector.formatLinkText(uiLocation.uiSourceCode.url, uiLocation.lineNumber);
        if (this._maxLength)
            text = text.trimMiddle(this._maxLength);
        anchor.textContent = text;
    }
}

WebInspector.DebuggerPresentationModel.DefaultLinkifierFormatter.prototype.__proto__ = WebInspector.DebuggerPresentationModel.LinkifierFormatter.prototype;

/**
 * @constructor
 * @param {WebInspector.DebuggerPresentationModel} model
 * @param {WebInspector.DebuggerPresentationModel.LinkifierFormatter=} formatter
 */
WebInspector.DebuggerPresentationModel.Linkifier = function(model, formatter)
{
    this._model = model;
    this._formatter = formatter || new WebInspector.DebuggerPresentationModel.DefaultLinkifierFormatter();
    this._liveLocations = [];
}

WebInspector.DebuggerPresentationModel.Linkifier.prototype = {
    /**
     * @param {string} sourceURL
     * @param {number} lineNumber
     * @param {number=} columnNumber
     * @param {string=} classes
     */
    linkifyLocation: function(sourceURL, lineNumber, columnNumber, classes)
    {
        var rawLocation = new WebInspector.DebuggerModel.Location(lineNumber, columnNumber || 0);
        var script = this._model._scriptForURLAndLocation(sourceURL, rawLocation);
        if (!script)
            return WebInspector.linkifyResourceAsNode(sourceURL, lineNumber, classes);
        rawLocation.scriptId = script.scriptId;
        return this.linkifyRawLocation(rawLocation, classes);
    },

    /**
     * @param {WebInspector.DebuggerModel.Location} rawLocation
     * @param {string=} classes
     */
    linkifyRawLocation: function(rawLocation, classes)
    {
        var anchor = WebInspector.linkifyURLAsNode("", "", classes, false);
        var liveLocation = this._model.createLiveLocation(rawLocation, this._updateAnchor.bind(this, anchor));
        liveLocation.init();
        this._liveLocations.push(liveLocation);
        return anchor;
    },

    reset: function()
    {
        for (var i = 0; i < this._liveLocations.length; ++i)
            this._liveLocations[i].dispose();
        this._liveLocations = [];
    },

    /**
     * @param {Element} anchor
     * @param {WebInspector.UILocation} uiLocation
     */
    _updateAnchor: function(anchor, uiLocation)
    {
        anchor.preferredPanel = "scripts";
        anchor.href = uiLocation.uiSourceCode.url;
        anchor.uiSourceCode = uiLocation.uiSourceCode;
        anchor.lineNumber = uiLocation.lineNumber;
        this._formatter.formatLiveAnchor(anchor, uiLocation);
    }
}

/**
 * @type {?WebInspector.DebuggerPresentationModel}
 */
WebInspector.debuggerPresentationModel = null;
