// original test:
// http://mxr.mozilla.org/mozilla2.0/source/dom/indexedDB/test/test_key_requirements.html
// license of original test:
// " Any copyright is dedicated to the Public Domain.
//   http://creativecommons.org/publicdomain/zero/1.0/ "

if (this.importScripts) {
    importScripts('../../../../fast/js/resources/js-test-pre.js');
    importScripts('../../resources/shared.js');
}

description("Test IndexedDB's event.target.result after add() and put()");

function test()
{
    removeVendorPrefixes();

    name = self.location.pathname;
    request = evalAndLog("indexedDB.open(name)");
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    db = evalAndLog("db = event.target.result");

    request = evalAndLog("request = db.setVersion('1')");
    request.onsuccess = cleanDatabase;
    request.onerror = unexpectedErrorCallback;
}

function cleanDatabase()
{
    deleteAllObjectStores(db);
    objectStore = evalAndLog("objectStore = db.createObjectStore('foo', { autoIncrement: true });");
    request = evalAndLog("request = objectStore.add({});");
    request.onsuccess = postAdd;
    request.onerror = unexpectedErrorCallback;
}

function postAdd()
{
    key1 = evalAndLog("key1 = event.target.result;");
    request = evalAndLog("request = objectStore.put({}, key1);");
    request.onsuccess = postPut1;
    request.onerror = unexpectedErrorCallback;
}

function postPut1()
{
    shouldBe("event.target.result", "key1");
    key2 = evalAndLog("key2 = 10;");
    request = evalAndLog("request = objectStore.put({}, key2);");
    request.onsuccess = postPut2;
    request.onerror = unexpectedErrorCallback;
}

function postPut2()
{
    shouldBe("event.target.result", "key2");
    key2 = evalAndLog("key2 = 100;");
    request = evalAndLog("request = objectStore.put({}, key2);");
    request.onsuccess = postPut3;
    request.onerror = unexpectedErrorCallback;
}

function postPut3()
{
    shouldBe("event.target.result", "key2");
    finishJSTest();
}

test();