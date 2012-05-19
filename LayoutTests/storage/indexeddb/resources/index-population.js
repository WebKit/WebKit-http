if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB index population.");

function test()
{
    removeVendorPrefixes();

    request = evalAndLog("indexedDB.deleteDatabase('index-population')");
    request.onsuccess = deleteSuccess;
    request.onerror = unexpectedErrorCallback;
}

function deleteSuccess()
{
    request = evalAndLog("indexedDB.open('index-population')");
    request.onsuccess = doSetVersion1;
    request.onerror = unexpectedErrorCallback;
}

function doSetVersion1()
{
    debug("");
    debug("doSetVersion1():");
    self.db = evalAndLog("db = event.target.result");
    request = evalAndLog("request = db.setVersion('version 1')");
    request.onsuccess = setVersion1;
    request.onerror = unexpectedErrorCallback;
}

function setVersion1()
{
    debug("");
    debug("setVersion1():");
    transaction = evalAndLog("transaction = request.result");
    transaction.onerror = unexpectedErrorCallback;
    transaction.onabort = unexpectedAbortCallback;
    store = evalAndLog("store = db.createObjectStore('store1')");
    evalAndLog("store.put({data: 'a', indexKey: 10}, 1)");
    evalAndLog("store.put({data: 'b', indexKey: 20}, 2)");
    evalAndLog("store.put({data: 'c', indexKey: 10}, 3)");
    evalAndLog("store.put({data: 'd', indexKey: 20}, 4)");
    evalAndLog("index = store.createIndex('index1', 'indexKey')");
    shouldBeTrue("index instanceof IDBIndex");
    shouldBeFalse("index.unique");
    request = evalAndLog("request = index.count(IDBKeyRange.bound(-Infinity, Infinity))");
    request.onerror = unexpectedErrorCallback;
    request.onsuccess = function () {
        shouldBe("request.result", "4");
    };
    transaction.oncomplete = doSetVersion2;
}

function doSetVersion2() {
    debug("");
    debug("doSetVersion2():");
    request = evalAndLog("request = db.setVersion('version 2')");
    request.onsuccess = setVersion2;
    request.onerror = unexpectedErrorCallback;
}

function setVersion2()
{
    debug("");
    debug("setVersion2():");
    transaction2 = evalAndLog("transaction = request.result");
    transaction2.onabort = setVersion2Abort;
    transaction2.oncomplete = unexpectedCompleteCallback;
    store = evalAndLog("store = db.createObjectStore('store2')");
    evalAndLog("store.put({data: 'a', indexKey: 10}, 1)");
    evalAndLog("store.put({data: 'b', indexKey: 20}, 2)");
    evalAndLog("store.put({data: 'c', indexKey: 10}, 3)");
    evalAndLog("store.put({data: 'd', indexKey: 20}, 4)");
    evalAndLog("index2 = store.createIndex('index2', 'indexKey', { unique: true })");
    shouldBeTrue("index2 instanceof IDBIndex");
    shouldBeTrue("index2.unique");
}

function setVersion2Abort()
{
    debug("");
    debug("setVersion2Abort():");
    shouldBe("db.objectStoreNames.length", "1");
    shouldBeEqualToString("db.objectStoreNames[0]", "store1");
    finishJSTest();
}

test();