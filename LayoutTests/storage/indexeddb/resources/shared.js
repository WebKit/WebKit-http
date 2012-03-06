var jsTestIsAsync = true;
if (self.importScripts && !self.postMessage) {
    // Shared worker.  Make postMessage send to the newest client, which in
    // our tests is the only client.

    // Store messages for sending until we have somewhere to send them.
    self.postMessage = function(message)
    {
        if (typeof self.pendingMessages === "undefined")
            self.pendingMessages = [];
        self.pendingMessages.push(message);
    };
    self.onconnect = function(event)
    {
        self.postMessage = function(message)
        {
            event.ports[0].postMessage(message);
        };
        // Offload any stored messages now that someone has connected to us.
        if (typeof self.pendingMessages === "undefined")
            return;
        while (self.pendingMessages.length)
            event.ports[0].postMessage(self.pendingMessages.shift());
    };
}

function done()
{
    isSuccessfullyParsed();
    if (self.layoutTestController)
        layoutTestController.notifyDone()
}

function unexpectedSuccessCallback()
{
    testFailed("Success function called unexpectedly.");
    done();
}

function unexpectedErrorCallback(event)
{
    testFailed("Error function called unexpectedly: (" + event.target.errorCode + ") " + event.target.webkitErrorMessage);
    done();
}

function unexpectedAbortCallback()
{
    testFailed("Abort function called unexpectedly!");
    done();
}

function unexpectedCompleteCallback()
{
    testFailed("oncomplete function called unexpectedly!");
    done();
}

function unexpectedBlockedCallback()
{
    testFailed("onblocked called unexpectedly");
    done();
}

function evalAndExpectException(cmd, expected)
{
    debug("Expecting exception from " + cmd);
    try {
        eval(cmd);
        testFailed("No exception thrown! Should have been " + expected);
    } catch (e) {
        code = e.code;
        testPassed("Exception was thrown.");
        shouldBe("code", expected);
    }
}

function evalAndExpectExceptionClass(cmd, expected)
{
    debug("Expecting " + expected + " exception from " + cmd);
    try {
        eval(cmd);
        testFailed("No exception thrown!" );
    } catch (e) {
		testPassed("Exception was thrown.");
		if (eval("e instanceof " + expected))
			testPassed(cmd + " threw " + e);
		else
			testFailed("Expected " + expected + " but saw " + e);
    }
}

function deleteAllObjectStores(db)
{
    while (db.objectStoreNames.length)
        db.deleteObjectStore(db.objectStoreNames.item(0));
    debug("Deleted all object stores.");
}
