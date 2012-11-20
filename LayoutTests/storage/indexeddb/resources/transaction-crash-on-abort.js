if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB transaction does not crash on abort.");

indexedDBTest(prepareDatabase, setVersionComplete);
function prepareDatabase()
{
    db = event.target.result;
    event.target.transaction.onabort = unexpectedAbortCallback;
    evalAndLog("db.createObjectStore('foo')");
}

function setVersionComplete()
{
    evalAndLog("db.transaction('foo')");
    evalAndLog("self.gc()");
    finishJSTest();
}
