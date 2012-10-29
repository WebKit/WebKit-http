/* This is the helper function to run animation tests:

Test page requirements:
- The body must contain an empty div with id "result"
- Call this function directly from the <script> inside the test page

Function parameters:
    expected [required]: an array of arrays defining a set of CSS properties that must have given values at specific times (see below)
    callback [optional]: a function to be executed just before the test starts (none by default)
    event [optional]: which DOM event to wait for before starting the test ("webkitAnimationStart" by default)

    Each sub-array must contain these items in this order:
    - the name of the CSS animation (may be null) [1]
    - the time in seconds at which to snapshot the CSS property
    - the id of the element on which to get the CSS property value [2]
    - the name of the CSS property to get [3]
    - the expected value for the CSS property
    - the tolerance to use when comparing the effective CSS property value with its expected value

    [1] If null is passed, a regular setTimeout() will be used instead to snapshot the animated property in the future,
    instead of fast forwarding using the pauseAnimationAtTimeOnElementWithId() JS API from DRT
    
    [2] If a single string is passed, it is the id of the element to test. If an array with 2 elements is passed they
    are the ids of 2 elements, whose values are compared for equality. In this case the expected value is ignored
    but the tolerance is used in the comparison. If the second element is prefixed with "static:", no animation on that
    element is required, allowing comparison with an unanimated "expected value" element.
    
    If a string with a '.' is passed, this is an element in an iframe. The string before the dot is the iframe id
    and the string after the dot is the element name in that iframe.

    [3] If the CSS property name is "webkitTransform", expected value must be an array of 1 or more numbers corresponding to the matrix elements,
    or a string which will be compared directly (useful if the expected value is "none")
    If the CSS property name is "webkitTransform.N", expected value must be a number corresponding to the Nth element of the matrix

*/

function isCloseEnough(actual, desired, tolerance)
{
    var diff = Math.abs(actual - desired);
    return diff <= tolerance;
}

function matrixStringToArray(s)
{
    if (s == "none")
        return [ 1, 0, 0, 1, 0, 0 ];
    var m = s.split("(");
    m = m[1].split(")");
    return m[0].split(",");
}

function parseCrossFade(s)
{
    var matches = s.match("-webkit-cross-fade\\((.*)\\s*,\\s*(.*)\\s*,\\s*(.*)\\)");

    if (!matches)
        return null;

    return {"from": matches[1], "to": matches[2], "percent": parseFloat(matches[3])}
}

// Return an array of numeric filter params in 0-1.
function getFilterParameters(s)
{
    var filterResult = s.match(/(\w+)\((.+)\)/);
    if (!filterResult)
        throw new Error("There's no filter in \"" + s + "\"");
    var filterParams = filterResult[2];
    if (filterResult[1] == "custom") {
        if (!window.getCustomFilterParameters)
            throw new Error("getCustomFilterParameters not found. Did you include custom-filter-parser.js?");
        return getCustomFilterParameters(filterParams);
    }
    var paramList = filterParams.split(' '); // FIXME: the spec may allow comma separation at some point.
    
    // Normalize percentage values.
    for (var i = 0; i < paramList.length; ++i) {
        var param = paramList[i];
        paramList[i] = parseFloat(paramList[i]);
        if (param.indexOf('%') != -1)
            paramList[i] = paramList[i] / 100;
    }

    return paramList;
}

function customFilterParameterMatch(param1, param2, tolerance)
{
    if (param1.type != "parameter") {
        // Checking for shader uris and other keywords. They need to be exactly the same.
        return (param1.type == param2.type && param1.value == param2.value);
    }

    if (param1.name != param2.name || param1.value.length != param2.value.length)
        return false;

    for (var j = 0; j < param1.value.length; ++j) {
        var val1 = param1.value[j],
            val2 = param2.value[j];
        if (val1.type != val2.type)
            return false;
        switch (val1.type) {
        case "function":
            if (val1.name != val2.name)
                return false;
            if (val1.arguments.length != val2.arguments.length) {
                console.error("Arguments length mismatch: ", val1.arguments.length, "/", val2.arguments.length);
                return false;
            }
            for (var t = 0; t < val1.arguments.length; ++t) {
                if (val1.arguments[t].type != "number" || val2.arguments[t].type != "number")
                    return false;
                if (!isCloseEnough(val1.arguments[t].value, val2.arguments[t].value, tolerance))
                    return false;
            }
            break;
        case "number":
            if (!isCloseEnough(val1.value, val2.value, tolerance))
                return false;
            break;
        default:
            console.error("Unsupported parameter type ", val1.type);
            return false;
        }
    }

    return true;
}

function filterParametersMatch(paramList1, paramList2, tolerance)
{
    if (paramList1.length != paramList2.length)
        return false;
    for (var i = 0; i < paramList1.length; ++i) {
        var param1 = paramList1[i], 
            param2 = paramList2[i];
        if (typeof param1 == "object") {
            // This is a custom filter parameter.
            if (!customFilterParameterMatch(param1, param2, tolerance))
                return false;
            continue;
        }
        var match = isCloseEnough(param1, param2, tolerance);
        if (!match)
            return false;
    }
    return true;
}

function checkExpectedValue(expected, index)
{
    var animationName = expected[index][0];
    var time = expected[index][1];
    var elementId = expected[index][2];
    var property = expected[index][3];
    var expectedValue = expected[index][4];
    var tolerance = expected[index][5];

    // Check for a pair of element Ids
    var compareElements = false;
    var element2Static = false;
    var elementId2;
    if (typeof elementId != "string") {
        if (elementId.length != 2)
            return;
            
        elementId2 = elementId[1];
        elementId = elementId[0];

        if (elementId2.indexOf("static:") == 0) {
            elementId2 = elementId2.replace("static:", "");
            element2Static = true;
        }

        compareElements = true;
    }
    
    // Check for a dot separated string
    var iframeId;
    if (!compareElements) {
        var array = elementId.split('.');
        if (array.length == 2) {
            iframeId = array[0];
            elementId = array[1];
        }
    }

    if (animationName && hasPauseAnimationAPI && !testRunner.pauseAnimationAtTimeOnElementWithId(animationName, time, elementId)) {
        result += "FAIL - animation \"" + animationName + "\" is not running" + "<br>";
        return;
    }
    
    if (compareElements && !element2Static && animationName && hasPauseAnimationAPI && !testRunner.pauseAnimationAtTimeOnElementWithId(animationName, time, elementId2)) {
        result += "FAIL - animation \"" + animationName + "\" is not running" + "<br>";
        return;
    }
    
    var computedValue, computedValue2;
    if (compareElements) {
        computedValue = getPropertyValue(property, elementId, iframeId);
        computedValue2 = getPropertyValue(property, elementId2, iframeId);

        if (comparePropertyValue(property, computedValue, computedValue2, tolerance))
            result += "PASS - \"" + property + "\" property for \"" + elementId + "\" and \"" + elementId2 + 
                            "\" elements at " + time + "s are close enough to each other" + "<br>";
        else
            result += "FAIL - \"" + property + "\" property for \"" + elementId + "\" and \"" + elementId2 + 
                            "\" elements at " + time + "s saw: \"" + computedValue + "\" and \"" + computedValue2 + 
                                            "\" which are not close enough to each other" + "<br>";
    } else {
        var elementName;
        if (iframeId)
            elementName = iframeId + '.' + elementId;
        else
            elementName = elementId;

        computedValue = getPropertyValue(property, elementId, iframeId);

        if (comparePropertyValue(property, computedValue, expectedValue, tolerance))
            result += "PASS - \"" + property + "\" property for \"" + elementName + "\" element at " + time + 
                            "s saw something close to: " + expectedValue + "<br>";
        else
            result += "FAIL - \"" + property + "\" property for \"" + elementName + "\" element at " + time + 
                            "s expected: " + expectedValue + " but saw: " + computedValue + "<br>";
    }
}


function getPropertyValue(property, elementId, iframeId)
{
    var computedValue;
    var element;
    if (iframeId)
        element = document.getElementById(iframeId).contentDocument.getElementById(elementId);
    else
        element = document.getElementById(elementId);

    if (property == "lineHeight")
        computedValue = parseInt(window.getComputedStyle(element).lineHeight);
    else if (property == "backgroundImage"
               || property == "borderImageSource"
               || property == "listStyleImage"
               || property == "webkitMaskImage"
               || property == "webkitMaskBoxImage"
               || property == "webkitFilter"
               || !property.indexOf("webkitTransform")) {
        computedValue = window.getComputedStyle(element)[property.split(".")[0]];
    } else {
        var computedStyle = window.getComputedStyle(element).getPropertyCSSValue(property);
        computedValue = computedStyle.getFloatValue(CSSPrimitiveValue.CSS_NUMBER);
    }

    return computedValue;
}

function comparePropertyValue(property, computedValue, expectedValue, tolerance)
{
    var result = true;

    if (!property.indexOf("webkitTransform")) {
        if (typeof expectedValue == "string")
            result = (computedValue == expectedValue);
        else if (typeof expectedValue == "number") {
            var m = matrixStringToArray(computedValue);
            result = isCloseEnough(parseFloat(m[parseInt(property.substring(16))]), expectedValue, tolerance);
        } else {
            var m = matrixStringToArray(computedValue);
            for (i = 0; i < expectedValue.length; ++i) {
                result = isCloseEnough(parseFloat(m[i]), expectedValue[i], tolerance);
                if (!result)
                    break;
            }
        }
    } else if (property == "webkitFilter") {
        var filterParameters = getFilterParameters(computedValue);
        var filter2Parameters = getFilterParameters(expectedValue);
        result = filterParametersMatch(filterParameters, filter2Parameters, tolerance);
    } else if (property == "backgroundImage"
               || property == "borderImageSource"
               || property == "listStyleImage"
               || property == "webkitMaskImage"
               || property == "webkitMaskBoxImage") {
        var computedCrossFade = parseCrossFade(computedValue);

        if (!computedCrossFade) {
            result = false;
        } else {
            if (typeof expectedValue == "string") {
                var computedCrossFade2 = parseCrossFade(expectedValue);
                result = isCloseEnough(computedCrossFade.percent, computedCrossFade2.percent, tolerance) && computedCrossFade.from == computedCrossFade2.from && computedCrossFade.to == computedCrossFade2.to;
            } else {
                result = isCloseEnough(computedCrossFade.percent, expectedValue, tolerance)
            }
        }
    } else {
        result = isCloseEnough(computedValue, expectedValue, tolerance);
    }
    return result;
}

function endTest()
{
    document.getElementById('result').innerHTML = result;

    if (window.testRunner)
        testRunner.notifyDone();
}

function checkExpectedValueCallback(expected, index)
{
    return function() { checkExpectedValue(expected, index); };
}

var testStarted = false;
function startTest(expected, callback)
{
    if (testStarted) return;
    testStarted = true;

    if (callback)
        callback();

    var maxTime = 0;

    for (var i = 0; i < expected.length; ++i) {
        var animationName = expected[i][0];
        var time = expected[i][1];

        // We can only use the animation fast-forward mechanism if there's an animation name
        // and DRT implements pauseAnimationAtTimeOnElementWithId()
        if (animationName && hasPauseAnimationAPI)
            checkExpectedValue(expected, i);
        else {
            if (time > maxTime)
                maxTime = time;

            window.setTimeout(checkExpectedValueCallback(expected, i), time * 1000);
        }
    }

    if (maxTime > 0)
        window.setTimeout(endTest, maxTime * 1000 + 50);
    else
        endTest();
}

var result = "";
var hasPauseAnimationAPI;

function runAnimationTest(expected, callback, event, disablePauseAnimationAPI, doPixelTest)
{
    hasPauseAnimationAPI = ('testRunner' in window) && ('pauseAnimationAtTimeOnElementWithId' in testRunner);
    if (disablePauseAnimationAPI)
        hasPauseAnimationAPI = false;

    if (window.testRunner) {
        if (!doPixelTest)
            testRunner.dumpAsText();
        testRunner.waitUntilDone();
    }
    
    if (!expected)
        throw("Expected results are missing!");
    
    var target = document;
    if (event == undefined)
        waitForAnimationToStart(target, function() { startTest(expected, callback); });
    else if (event == "load")
        window.addEventListener(event, function() {
            startTest(expected, callback);
        }, false);
}

function waitForAnimationToStart(element, callback)
{
    element.addEventListener('webkitAnimationStart', function() {
        window.setTimeout(callback, 0); // delay to give hardware animations a chance to start
    }, false);
}
