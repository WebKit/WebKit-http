/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
InspectorFrontendAPI = {};

InspectorTest = {};
InspectorTest._dispatchTable = [];
InspectorTest._requestId = -1;
InspectorTest.eventHandler = {};

/**
 * @param {string} method
 * @param {object} params
 * @param {function({object} messageObject)=} handler
 */
InspectorTest.sendCommand = function(method, params, handler)
{
    this._dispatchTable[++this._requestId] = handler;

    var messageObject = { "method": method,
                          "params": params,
                          "id": this._requestId };

    InspectorFrontendHost.sendMessageToBackend(JSON.stringify(messageObject));

    return this._requestId;
}

/**
 * @param {object} messageObject
 */
InspectorFrontendAPI.dispatchMessageAsync = function(messageObject)
{
    var messageId = messageObject["id"];
    if (typeof messageId === "number") {
        var handler = InspectorTest._dispatchTable[messageId];
        if (handler && typeof handler === "function")
            handler(messageObject);
    } else {
        var eventName = messageObject["method"];
        var eventHandler = InspectorTest.eventHandler[eventName];
        if (eventHandler)
            eventHandler(messageObject);
    }
}

/**
* Logs message to document.
* @param {string} message
*/
InspectorTest.log = function(message)
{
    this.sendCommand("Runtime.evaluate", { "expression": "log(" + JSON.stringify(message) + ")" } );
}

/**
* Logs an assert message to document.
* @param {boolean} condition
* @param {string} message
*/
InspectorTest.assert = function(condition, message)
{
    var status = condition ? "PASS" : "FAIL";
    this.sendCommand("Runtime.evaluate", { "expression": "log(" + JSON.stringify(status + ": " + message) + ")" } );
}

/**
* Logs message directly to process stdout via alert function (hopefully followed by flush call).
* This message should survive process crash or kill by timeout.
* @param {string} message
*/
InspectorTest.debugLog = function(message)
{
    this.sendCommand("Runtime.evaluate", { "expression": "debugLog(" + JSON.stringify(message) + ")" } );
}

InspectorTest.completeTest = function()
{
    this.sendCommand("Runtime.evaluate", { "expression": "closeTest();"} );
}

InspectorTest.checkForError = function(responseObject)
{
    if (responseObject.error) {
        InspectorTest.log("PROTOCOL ERROR: " + responseObject.error.message);
        InspectorTest.completeTest();
        throw "PROTOCOL ERROR";
    }
}

/**
 * @param {string} scriptName
 */
InspectorTest.importScript = function(scriptName)
{
    var xhr = new XMLHttpRequest();
    xhr.open("GET", scriptName, false);
    xhr.send(null);
    if (xhr.status !== 0 && xhr.status !== 200)
        throw new Error("Invalid script URL: " + scriptName);
    var script = "try { " + xhr.responseText + "} catch (e) { alert(" + JSON.stringify("Error in: " + scriptName) + "); throw e; }";
    window.eval(script);
}

InspectorTest.importInspectorScripts = function()
{
    // Note: This function overwrites the InspectorFrontendAPI, so there's currently no
    // way to intercept the messages from the backend.

    var inspectorScripts = [
        "Utilities",
        "WebInspector",
        "Object",
        "InspectorBackend",
        "InspectorFrontendAPI",
        "InspectorFrontendHostStub",
        "InspectorBackendCommands",
        "URLUtilities",
        "MessageDispatcher",
        "Setting",
        "PageObserver",
        "DOMObserver",
        "CSSObserver",
        "FrameResourceManager",
        "RuntimeManager",
        "Frame",
        "Revision",
        "SourceCodeRevision",
        "SourceCode",
        "Resource",
        "ResourceCollection",
        "DOMTreeManager",
        "DOMNode",
        "ContentFlow",
        "DOMTree",
        "ExecutionContext",
        "ExecutionContextList",
        "CSSStyleManager",
        "Color"
    ];
    for (var i = 0; i < inspectorScripts.length; ++i)
        InspectorTest.importScript("../../../../../Source/WebInspectorUI/UserInterface/" + inspectorScripts[i] + ".js");

    // The initialization should be in sync with WebInspector.loaded in Main.js.
    // FIXME: As soon as we can support all the observers and managers we should remove UI related tasks 
    // from WebInspector.loaded, so that it can be used from the LayoutTests.

    InspectorBackend.registerPageDispatcher(new WebInspector.PageObserver);
    InspectorBackend.registerDOMDispatcher(new WebInspector.DOMObserver);
    InspectorBackend.registerCSSDispatcher(new WebInspector.CSSObserver);

    WebInspector.frameResourceManager = new WebInspector.FrameResourceManager;
    WebInspector.domTreeManager = new WebInspector.DOMTreeManager;
    WebInspector.cssStyleManager = new WebInspector.CSSStyleManager;

    InspectorFrontendHost.loaded();
}


window.addEventListener("message", function(event) {
    try {
        eval(event.data);
    } catch (e) {
        alert(e.stack);
        InspectorTest.completeTest();
        throw e;
    }
});
