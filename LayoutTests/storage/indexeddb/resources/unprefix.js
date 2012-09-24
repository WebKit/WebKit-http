if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Check that IDBFactory is available through the prefixed or unprefixed entry point.");

function test()
{
    shouldBeEqualToString("String(self.indexedDB)", "[object IDBFactory]");
    shouldBeEqualToString("String(self.webkitIndexedDB)", "[object IDBFactory]");
    shouldBeNonNull("IDBCursor");
    shouldBeNonNull("IDBDatabase");
    shouldBeNonNull("IDBFactory");
    shouldBeNonNull("IDBIndex");
    shouldBeNonNull("IDBKeyRange");
    shouldBeNonNull("IDBObjectStore");
    shouldBeNonNull("IDBRequest");
    shouldBeNonNull("IDBTransaction");
    finishJSTest();
}

test();
