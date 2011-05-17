var initialize_InspectorTest = function() {

var results = [];
var resultsSynchronized = false;

function consoleOutputHook(messageType)
{
    InspectorTest.addResult(messageType + ": " + Array.prototype.slice.call(arguments, 1));
}

console.log = consoleOutputHook.bind(InspectorTest, "log");
console.error = consoleOutputHook.bind(InspectorTest, "error");
console.info = consoleOutputHook.bind(InspectorTest, "info");

InspectorTest.completeTest = function()
{
    RuntimeAgent.evaluate("didEvaluateForTestInFrontend(" + InspectorTest.completeTestCallId + ", \"\")", "test");
}

InspectorTest.evaluateInConsole = function(code, callback)
{
    callback = InspectorTest.safeWrap(callback);

    WebInspector.console.visible = true;
    WebInspector.console.prompt.text = code;
    var event = document.createEvent("KeyboardEvent");
    event.initKeyboardEvent("keydown", true, true, null, "Enter", "");
    WebInspector.console.promptElement.dispatchEvent(event);
    InspectorTest.addSniffer(WebInspector.ConsoleView.prototype, "addMessage",
        function(commandResult) {
            callback(commandResult.toMessageElement().textContent);
        });
}

InspectorTest.evaluateInConsoleAndDump = function(code, callback)
{
    callback = InspectorTest.safeWrap(callback);

    function mycallback(text)
    {
        InspectorTest.addResult(code + " = " + text);
        callback(text);
    }
    InspectorTest.evaluateInConsole(code, mycallback);
}

InspectorTest.evaluateInPage = function(code, callback)
{
    callback = InspectorTest.safeWrap(callback);

    function mycallback(error, result, wasThrown)
    {
        if (!error)
            callback(WebInspector.RemoteObject.fromPayload(result), wasThrown);
    }
    RuntimeAgent.evaluate(code, "console", false, mycallback);
}

InspectorTest.evaluateInPageWithTimeout = function(code)
{
    InspectorTest.evaluateInPage("setTimeout(unescape('" + escape(code) + "'))");
}

InspectorTest.addResult = function(text)
{
    results.push(text);
    if (resultsSynchronized)
        addResultToPage(text);
    else {
        clearResults();
        for (var i = 0; i < results.length; ++i)
            addResultToPage(results[i]);
        resultsSynchronized = true;
    }

    function clearResults()
    {
        InspectorTest.evaluateInPage("clearOutput()");
    }

    function addResultToPage(text)
    {
        InspectorTest.evaluateInPage("output(unescape('" + escape(text) + "'))");
    }
}

InspectorTest.addResults = function(textArray)
{
    if (!textArray)
        return;
    for (var i = 0, size = textArray.length; i < size; ++i)
        InspectorTest.addResult(textArray[i]);
}

function onError(event)
{
    window.removeEventListener("error", onError);
    InspectorTest.addResult("Uncaught exception in inspector front-end: " + event.message + " [" + event.filename + ":" + event.lineno + "]");
    InspectorTest.completeTest();
}

window.addEventListener("error", onError);

InspectorTest.addObject = function(object, nondeterministicProps, prefix, firstLinePrefix)
{
    prefix = prefix || "";
    firstLinePrefix = firstLinePrefix || prefix;
    InspectorTest.addResult(firstLinePrefix + "{");
    for (var prop in object) {
        if (typeof object.hasOwnProperty === "function" && !object.hasOwnProperty(prop))
            continue;
        var prefixWithName = "    " + prefix + prop + " : ";
        var propValue = object[prop];
        if (nondeterministicProps && prop in nondeterministicProps)
            InspectorTest.addResult(prefixWithName + "<" + typeof propValue + ">");
        else
            InspectorTest.dump(propValue, nondeterministicProps, "    " + prefix, prefixWithName);
    }
    InspectorTest.addResult(prefix + "}");
}

InspectorTest.addArray = function(array, nondeterministicProps, prefix, firstLinePrefix)
{
    prefix = prefix || "";
    firstLinePrefix = firstLinePrefix || prefix;
    InspectorTest.addResult(firstLinePrefix + "[");
    for (var i = 0; i < array.length; ++i)
        InspectorTest.dump(array[i], nondeterministicProps, prefix + "    ");
    InspectorTest.addResult(prefix + "]");
}

InspectorTest.dump = function(value, nondeterministicProps, prefix, prefixWithName)
{
    prefixWithName = prefixWithName || prefix;
    if (value === null)
        InspectorTest.addResult(prefixWithName + "null");
    else if (value instanceof Array)
        InspectorTest.addArray(value, nondeterministicProps, prefix, prefixWithName);
    else if (typeof value === "object")
        InspectorTest.addObject(value, nondeterministicProps, prefix, prefixWithName);
    else if (typeof value === "string")
        InspectorTest.addResult(prefixWithName + "\"" + value + "\"");
    else
        InspectorTest.addResult(prefixWithName + value);
}

InspectorTest.assertGreaterOrEqual = function(expected, actual, message)
{
    if (actual < expected)
        InspectorTest.addResult("FAILED: " + (message ? message + ": " : "") + actual + " < " + expected);
}

InspectorTest.navigate = function(url, callback)
{
    InspectorTest._pageLoadedCallback = InspectorTest.safeWrap(callback);

    if (WebInspector.panels.network)
        WebInspector.panels.network._reset();
    InspectorTest.evaluateInConsole("window.location = '" + url + "'");
}

InspectorTest.reloadPage = function(callback)
{
    InspectorTest._pageLoadedCallback = InspectorTest.safeWrap(callback);

    if (WebInspector.panels.network)
        WebInspector.panels.network._reset();
    PageAgent.reload(false);
}

InspectorTest.pageLoaded = function()
{
    resultsSynchronized = false;
    InspectorTest.addResult("Page reloaded.");
    if (InspectorTest._pageLoadedCallback) {
        var callback = InspectorTest._pageLoadedCallback;
        delete InspectorTest._pageLoadedCallback;
        callback();
    }
}

InspectorTest.runWhenPageLoads = function(callback)
{
    var oldCallback = InspectorTest._pageLoadedCallback;
    function chainedCallback()
    {
        if (oldCallback)
            oldCallback();
        callback();
    }
    InspectorTest._pageLoadedCallback = InspectorTest.safeWrap(chainedCallback);
}

InspectorTest.runAfterPendingDispatches = function(callback)
{
    callback = InspectorTest.safeWrap(callback);
    InspectorBackend.runAfterPendingDispatches(callback);
}

InspectorTest.createKeyEvent = function(keyIdentifier, ctrlKey, altKey, shiftKey, metaKey)
{
    var evt = document.createEvent("KeyboardEvent");
    evt.initKeyboardEvent("keydown", true /* can bubble */, true /* can cancel */, null /* view */, keyIdentifier, "", ctrlKey, altKey, shiftKey, metaKey);
    return evt;
}

InspectorTest.runTestSuite = function(testSuite)
{
    var testSuiteTests = testSuite.slice();

    function runner()
    {
        if (!testSuiteTests.length) {
            InspectorTest.completeTest();
            return;
        }
        var nextTest = testSuiteTests.shift();
        InspectorTest.addResult("");
        InspectorTest.addResult("Running: " + /function\s([^(]*)/.exec(nextTest)[1]);
        InspectorTest.safeWrap(nextTest)(runner, runner);
    }
    runner();
}

InspectorTest.assertEquals = function(expected, found, message)
{
    if (expected === found)
        return;

    var error;
    if (message)
        error = "Failure (" + message + "):";
    else
        error = "Failure:";
    throw new Error(error + " expected <" + expected + "> found <" + found + ">");
}

InspectorTest.safeWrap = function(func, onexception)
{
    function result()
    {
        if (!func)
            return;
        var wrapThis = this;
        try {
            return func.apply(wrapThis, arguments);
        } catch(e) {
            InspectorTest.addResult("Exception while running: " + func + "\n" + (e.stack || e));
            if (onexception)
                InspectorTest.safeWrap(onexception)();
            else
                InspectorTest.completeTest();
        }
    }
    return result;
}

InspectorTest.addSniffer = function(receiver, methodName, override, opt_sticky)
{
    override = InspectorTest.safeWrap(override);

    var original = receiver[methodName];
    if (typeof original !== "function")
        throw ("Cannot find method to override: " + methodName);

    receiver[methodName] = function(var_args) {
        try {
            var result = original.apply(this, arguments);
        } finally {
            if (!opt_sticky)
                receiver[methodName] = original;
        }
        // In case of exception the override won't be called.
        try {
            override.apply(this, arguments);
        } catch (e) {
            throw ("Exception in overriden method '" + methodName + "': " + e);
        }
        return result;
    };
}

InspectorTest.override = function(receiver, methodName, override, opt_sticky)
{
    override = InspectorTest.safeWrap(override);

    var original = receiver[methodName];
    if (typeof original !== "function")
        throw ("Cannot find method to override: " + methodName);

    receiver[methodName] = function(var_args) {
        try {
            try {
                var result = override.apply(this, arguments);
            } catch (e) {
                throw ("Exception in overriden method '" + methodName + "': " + e);
            }
        } finally {
            if (!opt_sticky)
                receiver[methodName] = original;
        }
        return result;
    };

    return original;
}

InspectorTest.textContentWithLineBreaks = function(node)
{
    var buffer = "";
    var currentNode = node;
    while (currentNode = currentNode.traverseNextNode(node)) {
        if (currentNode.nodeType === Node.TEXT_NODE)
            buffer += currentNode.nodeValue;
        else if (currentNode.nodeName === "LI")
            buffer += "\n    ";
        else if (currentNode.classList.contains("console-message"))
            buffer += "\n\n";
    }
    return buffer;
}

};

var runTestCallId = 0;
var completeTestCallId = 1;

function runAfterIframeIsLoaded()
{
    if (window.layoutTestController)
        layoutTestController.waitUntilDone();
    function step()
    {
        if (!window.iframeLoaded)
            setTimeout(step, 100);
        else
            runTest();
    }
    setTimeout(step, 100);
}

function runTest(enableWatchDogWhileDebugging)
{
    if (!window.layoutTestController)
        return;

    layoutTestController.dumpAsText();
    layoutTestController.waitUntilDone();

    function runTestInFrontend(initializationFunctions, testFunction, completeTestCallId)
    {
        if (window.InspectorTest) {
            InspectorTest.pageLoaded();
            return;
        }

        InspectorTest = {};
        InspectorTest.completeTestCallId = completeTestCallId;

        for (var i = 0; i < initializationFunctions.length; ++i) {
            try {
                initializationFunctions[i]();
            } catch (e) {
                console.error("Exception in test initialization: " + e);
                InspectorTest.completeTest();
            }
        }

        WebInspector.showPanel("console");
        try {
            testFunction();
        } catch (e) {
            console.error("Exception during test execution: " + e);
            InspectorTest.completeTest();
        }
    }

    var initializationFunctions = [];
    for (var name in window) {
        if (name.indexOf("initialize_") === 0 && typeof window[name] === "function")
            initializationFunctions.push(window[name].toString());
    }
    var parameters = ["[" + initializationFunctions + "]", test, completeTestCallId];
    var toEvaluate = "(" + runTestInFrontend + ")(" + parameters.join(", ") + ");";
    layoutTestController.evaluateInWebInspector(runTestCallId, toEvaluate);

    if (enableWatchDogWhileDebugging) {
        function watchDog()
        {
            console.log("Internal watchdog triggered at 10 seconds. Test timed out.");
            closeInspectorAndNotifyDone();
        }
        window._watchDogTimer = setTimeout(watchDog, 10000);
    }
}

function didEvaluateForTestInFrontend(callId)
{
    if (callId !== completeTestCallId)
        return;
    delete window.completeTestCallId;
    // Close inspector asynchrously to allow caller of this
    // function send response before backend dispatcher and frontend are destroyed.
    setTimeout(closeInspectorAndNotifyDone, 0);
}

function closeInspectorAndNotifyDone()
{
    if (window._watchDogTimer)
        clearTimeout(window._watchDogTimer);

    layoutTestController.closeWebInspector();
    setTimeout(function() {
        layoutTestController.notifyDone();
    }, 0);
}

var outputElement;

function output(text)
{
    if (!outputElement) {
        var intermediate = document.createElement("div");
        document.body.appendChild(intermediate);

        var intermediate2 = document.createElement("div");
        intermediate.appendChild(intermediate2);

        outputElement = document.createElement("div");
        outputElement.className = "output";
        outputElement.style.whiteSpace = "pre";
        intermediate2.appendChild(outputElement);
    }
    outputElement.appendChild(document.createTextNode(text));
    outputElement.appendChild(document.createElement("br"));
}

function clearOutput()
{
    if (outputElement) {
        outputElement.parentNode.removeChild(outputElement);
        outputElement = null;
    }
}
