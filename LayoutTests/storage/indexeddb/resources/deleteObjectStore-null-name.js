if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB deleteObjectStore required argument");

indexedDBTest(prepareDatabase);
function prepareDatabase()
{
    db = event.target.result;

    evalAndLog("db.createObjectStore('null');");
    evalAndLog("db.deleteObjectStore(null);");
    finishJSTest();
}
