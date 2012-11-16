if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB opening database connections during transactions");

indexedDBTest(prepareDatabase, startTransaction);
function prepareDatabase()
{
    dbc1 = event.target.result;
    evalAndLog("dbc1.createObjectStore('storeName')");
    event.target.transaction.oncomplete = function (e) {
        debug("database preparation complete");
        debug("");
    };
}

function startTransaction()
{
    debug("starting transaction");
    evalAndLog("state = 'starting'");
    evalAndLog("trans = dbc1.transaction('storeName', 'readwrite')");
    evalAndLog("trans.objectStore('storeName').put('value', 'key')");
    trans.onabort = unexpectedAbortCallback;
    trans.onerror = unexpectedErrorCallback;
    trans.oncomplete = function (e) {
        debug("transaction complete");
        shouldBeEqualToString("state", "open2complete");
        debug("");
    };

    debug("");
    tryOpens();
}

function tryOpens()
{
    debug("trying to open the same database");
    evalAndLog("openreq2 = indexedDB.open(dbname)");
    openreq2.onerror = unexpectedErrorCallback;
    openreq2.onsuccess = function (e) {
        debug("openreq2.onsuccess");
        shouldBeEqualToString("state", "starting");
        evalAndLog("state = 'open2complete'");
        debug("");
    }
    debug("");

    debug("trying to open a different database");
    evalAndLog("openreq3 = indexedDB.open(dbname + '2')");
    openreq3.onerror = unexpectedErrorCallback;
    openreq3.onsuccess = function (e) {
        debug("openreq3.onsuccess");
        shouldBeEqualToString("state", "open2complete");
        evalAndLog("state = 'open3complete'");
        finishJSTest();
    }
    debug("");
}
