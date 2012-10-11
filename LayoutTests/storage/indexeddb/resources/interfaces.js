if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB's interfaces.");

function test()
{
    removeVendorPrefixes();
    shouldBeTrue("'IDBCursor' in self");
    shouldBeTrue("'IDBCursorWithValue' in self");
    shouldBeTrue("'IDBDatabase' in self");
    shouldBeTrue("'IDBFactory' in self");
    shouldBeTrue("'IDBIndex' in self");
    shouldBeTrue("'IDBKeyRange' in self");
    shouldBeTrue("'IDBObjectStore' in self");
    shouldBeTrue("'IDBOpenDBRequest' in self");
    shouldBeTrue("'IDBRequest' in self");
    shouldBeTrue("'IDBTransaction' in self");
    shouldBeTrue("'IDBVersionChangeEvent' in self");

    finishJSTest();
}

test();
