if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB transaction basics.");

function test()
{
    removeVendorPrefixes();

    request = evalAndLog("indexedDB.open('transaction-basics')");
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    debug("openSuccess():");
    self.db = evalAndLog("db = event.target.result");
    request = evalAndLog("request = db.setVersion('version 1')");
    request.onsuccess = cleanDatabase;
    request.onerror = unexpectedErrorCallback;
}

function cleanDatabase()
{
    deleteAllObjectStores(db, checkMetadataEmpty);
    event.target.result.oncomplete = testSetVersionAbort1;
}

function testSetVersionAbort1()
{
    checkMetadataEmpty();
    request = evalAndLog("request = startSetVersion('version fail')");
    request.onsuccess = addRemoveIDBObjects;
}

function addRemoveIDBObjects()
{
    debug("addRemoveIDBObjects():");
    var trans = evalAndLog("trans = event.target.result");
    shouldBeNonNull("trans");
    trans.addEventListener('abort', testSetVersionAbort2, true);
    trans.oncomplete = unexpectedCompleteCallback;

    var store = evalAndLog("store = db.createObjectStore('storeFail', null)");
    var index = evalAndLog("index = store.createIndex('indexFail', 'x')");

    evalAndLog("db.deleteObjectStore('storeFail')");
    evalAndExpectException("store.deleteIndex('indexFail')", "DOMException.INVALID_STATE_ERR", "'InvalidStateError'");

    trans.abort();
}

function testSetVersionAbort2()
{
    debug("");
    debug("testSetVersionAbort2():");
    checkMetadataEmpty();
    request = evalAndLog("request = startSetVersion('version fail')");
    request.onsuccess = addRemoveAddIDBObjects;
}

function addRemoveAddIDBObjects()
{
    debug("addRemoveAddIDBObjects():");
    var trans = evalAndLog("trans = event.target.result");
    shouldBeNonNull("trans");
    trans.addEventListener('abort', testSetVersionAbort3, false);
    trans.oncomplete = unexpectedCompleteCallback;

    var store = evalAndLog("store = db.createObjectStore('storeFail', null)");
    var index = evalAndLog("index = store.createIndex('indexFail', 'x')");

    evalAndLog("db.deleteObjectStore('storeFail')");
    evalAndExpectException("store.deleteIndex('indexFail')", "DOMException.INVALID_STATE_ERR", "'InvalidStateError'");

    var store = evalAndLog("store = db.createObjectStore('storeFail', null)");
    var index = evalAndLog("index = store.createIndex('indexFail', 'x')");

    trans.abort();
}

function testSetVersionAbort3()
{
    debug("");
    debug("testSetVersionAbort3():");
    shouldBeFalse("event.cancelable");
    checkMetadataEmpty();
    request = evalAndLog("request = startSetVersion('version fail')");
    request.onsuccess = addIDBObjects;
}

function addIDBObjects()
{
    debug("addIDBObjects():");
    shouldBeFalse("event.cancelable");
    var trans = evalAndLog("trans = event.target.result");
    shouldBeNonNull("trans");
    trans.onabort = testInactiveAbortedTransaction;
    trans.oncomplete = unexpectedCompleteCallback;

    store = evalAndLog("store = db.createObjectStore('storeFail', null)");
    index = evalAndLog("index = store.createIndex('indexFail', 'x')");

    trans.abort();
}

function testInactiveAbortedTransaction()
{
    debug("");
    debug("testInactiveAbortedTransaction():");
    evalAndExpectException("index.openCursor()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.openKeyCursor()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.get(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.getKey(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.count()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");

    evalAndExpectException("store.put(0, 0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.add(0, 0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.delete(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.clear()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.get(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.openCursor()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");

    testSetVersionAbort4();
}

function testSetVersionAbort4()
{
    debug("");
    debug("testSetVersionAbort4():");
    checkMetadataEmpty();
    request = evalAndLog("request = startSetVersion('version fail')");
    request.onsuccess = addIDBObjectsAndCommit;
}

function addIDBObjectsAndCommit()
{
    debug("addIDBObjectsAndCommit():");
    var trans = evalAndLog("trans = event.target.result");
    shouldBeNonNull("trans");
    trans.onabort = unexpectedAbortCallback;

    store = evalAndLog("store = db.createObjectStore('storeFail', null)");
    index = evalAndLog("index = store.createIndex('indexFail', 'x')");

    trans.oncomplete = testInactiveCompletedTransaction;
}

function testInactiveCompletedTransaction()
{
    debug("");
    debug("testInactiveCompletedTransaction():");
    evalAndExpectException("index.openCursor()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.openKeyCursor()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.get(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.getKey(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("index.count()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");

    evalAndExpectException("store.put(0, 0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.add(0, 0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.delete(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.clear()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.get(0)", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");
    evalAndExpectException("store.openCursor()", "IDBDatabaseException.TRANSACTION_INACTIVE_ERR", "'TransactionInactiveError'");

    testSetVersionAbort5();
}

function testSetVersionAbort5()
{
    debug("");
    debug("testSetVersionAbort5():");
    checkMetadataExistingObjectStore();
    request = evalAndLog("request = startSetVersion('version fail')");
    request.onsuccess = removeIDBObjects;
}

function removeIDBObjects()
{
    debug("removeIDBObjects():");
    var trans = evalAndLog("trans = event.target.result");
    shouldBeNonNull("trans");
    trans.onabort = testSetVersionAbort6;
    trans.oncomplete = unexpectedCompleteCallback;

    var store = evalAndLog("store = trans.objectStore('storeFail')");
    evalAndLog("store.deleteIndex('indexFail')");
    evalAndLog("db.deleteObjectStore('storeFail')");

    trans.abort();
}

function testSetVersionAbort6()
{
    debug("");
    debug("testSetVersionAbort6():");
    checkMetadataExistingObjectStore();
    setNewVersion();
}

function startSetVersion(versionName)
{
    request = db.setVersion(versionName);
    request.onerror = unexpectedErrorCallback;
    return request;
}

function checkMetadataEmpty()
{
    shouldBe("self.db.objectStoreNames", "[]");
    shouldBe("self.db.objectStoreNames.length", "0");
    shouldBe("self.db.objectStoreNames.contains('storeFail')", "false");
}

function checkMetadataExistingObjectStore()
{
    shouldBe("db.objectStoreNames", "['storeFail']");
    shouldBe("db.objectStoreNames.length", "1");
    shouldBe("db.objectStoreNames.contains('storeFail')", "true");
}

function setNewVersion()
{
    request = evalAndLog("db.setVersion('new version')");
    request.onsuccess = setVersionSuccess;
    request.onerror = unexpectedErrorCallback;
}

function setVersionSuccess()
{
    debug("");
    debug("setVersionSuccess():");
    self.trans = evalAndLog("trans = event.target.result");
    shouldBeNonNull("trans");
    trans.onabort = unexpectedAbortCallback;
    trans.addEventListener('complete', completeCallback, false);
    self.completeEventFired = false;

    deleteAllObjectStores(db);

    evalAndLog("db.createObjectStore('storeName', null)");
}

function completeCallback()
{
    shouldBeFalse("event.cancelable");
    testPassed("complete event fired");
    transaction = evalAndLog("db.transaction(['storeName'])");
    transaction.oncomplete = emptyCompleteCallback;
    var store = evalAndLog("store = transaction.objectStore('storeName')");
    shouldBeEqualToString("store.name", "storeName");
}

function emptyCompleteCallback()
{
    testPassed("complete event fired");
    testDOMStringList();
}

function testDOMStringList()
{
    debug("");
    debug("Verifying DOMStringList works as argument for IDBDatabase.transaction()");
    debug("db.objectStoreNames is " + db.objectStoreNames);
    debug("... which contains: " + JSON.stringify(Array.prototype.slice.call(db.objectStoreNames)));
    evalAndLog("transaction = db.transaction(db.objectStoreNames)");
    testPassed("no exception thrown");
    for (var i = 0; i < db.objectStoreNames.length; ++i) {
      shouldBeNonNull("transaction.objectStore(" + JSON.stringify(db.objectStoreNames[i]) + ")");
    }
    testPassed("all stores present in transaction");
    testInvalidMode();
}

function testInvalidMode()
{
    debug("");
    debug("Verify that specifying an invalid mode raises an exception");
    evalAndExpectExceptionClass("db.transaction(['storeName'], 'lsakjdf')", "TypeError");
    testDegenerateNames();
}

function testDegenerateNames()
{
    debug("");
    debug("Test that null and undefined are treated as strings");

    evalAndExpectException("db.transaction(null)", "DOMException.NOT_FOUND_ERR", "'NotFoundError'");
    evalAndExpectException("db.transaction(undefined)", "DOMException.NOT_FOUND_ERR", "'NotFoundError'");

    request = evalAndLog("db.setVersion('funny names')");
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function () {
        var trans = request.result;
        evalAndLog("db.createObjectStore('null')");
        evalAndLog("db.createObjectStore('undefined')");
        trans.oncomplete = verifyDegenerateNames;
    };
    function verifyDegenerateNames() {
        shouldNotThrow("transaction = db.transaction(null)");
        shouldBeNonNull("transaction.objectStore('null')");
        shouldNotThrow("transaction = db.transaction(undefined)");
        shouldBeNonNull("transaction.objectStore('undefined')");
        finishJSTest();
    }
}

test();
