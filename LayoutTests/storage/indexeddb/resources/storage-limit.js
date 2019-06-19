if (this.importScripts) {
    importScripts('../../../resources/js-test.js');
    importScripts('shared.js');
}

if (window.testRunner)
    testRunner.setAllowStorageQuotaIncrease(false);

var quota = 400 * 1024; // default quota for testing.
description("This test makes sure that storage of indexedDB does not grow unboundedly.");

indexedDBTest(prepareDatabase, onOpenSuccess);

function prepareDatabase(event)
{
    preamble(event);
    evalAndLog("db = event.target.result");
    evalAndLog("store = db.createObjectStore('store')");
}

function onOpenSuccess(event)
{
    preamble(event);
    evalAndLog("db = event.target.result");
    evalAndLog("store = db.transaction('store', 'readwrite').objectStore('store')");
    evalAndLog("request = store.add(new Uint8Array(" + (quota + 1) + "), 0)");
    request.onerror = function(event) {
        shouldBeTrue("'error' in request");
        shouldBe("request.error.code", "DOMException.QUOTA_EXCEEDED_ERR");
        shouldBeEqualToString("request.error.name", "QuotaExceededError");
        finishJSTest();
    }

    request.onsuccess = function(event) {
        testFailed("Add operation should fail because storage limit is reached, but succeeded.");
        finishJSTest();
    }
}
