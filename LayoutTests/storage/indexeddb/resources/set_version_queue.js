if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("4 open connections try to setVersion at the same time.  3 connections eventually close, allowing 1 setVersion call to proceed.");

connections = []
function test()
{
    removeVendorPrefixes();

    openDBConnection();
}

function openDBConnection()
{
    result = evalAndLog("indexedDB.open('set-version-queue')");
    result.onsuccess = openSuccess;
    result.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    connection = event.target.result;
    connection.onversionchange = generateVersionChangeHandler();
    connections.push(connection);
    if (connections.length < 4)
        openDBConnection();
    else {
        request = evalAndLog("connections[0].setVersion('version 0')");
        request.onerror = function(event){ connectionError(event, 0) };
        request.onsuccess = unexpectedSuccessCallback;
        request.onblocked = blocked0;
        request1 = evalAndLog("connections[1].setVersion('version 1')");
        request1.onerror = unexpectedErrorCallback;
        request1.onsuccess = inSetVersion1;
        request1.onblocked = blocked1;
        request2 = evalAndLog("connections[2].setVersion('version 2')");
        request2.onerror = function(event){ connectionError(event, 2) };
        request2.onsuccess = unexpectedSuccessCallback;
        request2.onblocked = blocked2;
        request3 = evalAndLog("connections[3].setVersion('version 3')");
        request3.onerror = function(event){ connectionError(event, 3) };
        request3.onsuccess = unexpectedSuccessCallback;
        request3.onblocked = blocked3;
        debug("");
    }
}

function generateVersionChangeHandler()
{
    var connectionNum = connections.length;
    return function(event)
    {
        shouldBeTrue("event.version.length > 0");
        debug("connection[" + connectionNum + "] received versionChangeEvent: " + event.version);
    }
}

blocked0fired = false;
blocked2fired = false;
function blocked0(event)
{
    debug("");
    testPassed("connection[0] got blocked event");
    shouldBeEqualToString("event.version", "version 0");
    debug("Close the connection that received the block event:");
    evalAndLog("connections[0].close()");
    debug("Close another connection as well, to test 4.7.4-note:");
    evalAndLog("connections[3].close()");
    evalAndLog("blocked0fired = true");
    debug("");
}

function blocked1(event)
{
    debug("")
    testPassed("connection[1] got blocked event");
    debug("Ensure that this blocked event is in order:");
    shouldBeTrue("blocked0fired");
    shouldBeFalse("blocked2fired");
    debug("")
}

function blocked2(event)
{
    debug("")
    testPassed("connection[2] got blocked event");
    shouldBeEqualToString("event.version", "version 2");
    evalAndLog("connections[2].close()");
    evalAndLog("blocked2fired = true");
    debug("")
}

function blocked3(event)
{
    debug("")
    testPassed("connection[3] got blocked event");
    debug("Note: This means that a connection can receive a blocked event after its close() method has been called.  Spec is silent on the issue and this is easiest to implement.");
    shouldBeEqualToString("event.version", "version 3");
}

function connectionError(event, connectionId)
{
    debug("");
    testPassed("connection[" + connectionId + "] got error event");
    shouldBe("event.target.errorCode", "DOMException.ABORT_ERR");
    shouldBe("event.target.error.name", "'AbortError'");
    if ('webkitErrorMessage' in event.target) {
        shouldBe("event.target.webkitErrorMessage.length > 0", "true");
        debug(event.target.webkitErrorMessage);
    }
}

function inSetVersion1(event)
{
    debug("")
    testPassed("connection[1] got into SetVersion");
    finishJSTest();
}

test();
