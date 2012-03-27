if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test the basics of IndexedDB's IDBFactory.");

function test()
{
    removeVendorPrefixes();

    shouldBeTrue("typeof indexedDB.open === 'function'");
    shouldBeTrue("typeof indexedDB.cmp === 'function'");
    shouldBeTrue("typeof indexedDB.getDatabaseNames === 'function'");

    shouldBeTrue("typeof indexedDB.deleteDatabase === 'function'");

    name = 'storage/indexeddb/factory-basics';
    description = "My Test Database";

    request = evalAndLog("indexedDB.getDatabaseNames()");
    request.onsuccess = getDatabaseNamesSuccess1;
    request.onerror = unexpectedErrorCallback;
}

function getDatabaseNamesSuccess1()
{
    var databaseNames;
    evalAndLog("databaseNames = event.target.result");
    shouldBeFalse("databaseNames.contains('" + name + "')");
    shouldBeFalse("databaseNames.contains('DATABASE THAT DOES NOT EXIST')");

    request = evalAndLog("indexedDB.open(name, description)");
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    evalAndLog("event.target.result.close()");
    request = evalAndLog("indexedDB.getDatabaseNames()");
    request.onsuccess = getDatabaseNamesSuccess2;
    request.onerror = unexpectedErrorCallback;
}

function getDatabaseNamesSuccess2()
{
    var databaseNames;
    evalAndLog("databaseNames = event.target.result");
    shouldBeTrue("databaseNames.contains('" + name + "')");
    shouldBeFalse("databaseNames.contains('DATABASE THAT DOES NOT EXIST')");

    request = evalAndLog("indexedDB.deleteDatabase('" + name + "')");
    request.onsuccess = deleteDatabaseSuccess;
    request.onerror = unexpectedErrorCallback;
}

function deleteDatabaseSuccess()
{
    request = evalAndLog("indexedDB.getDatabaseNames()");
    request.onsuccess = getDatabaseNamesSuccess3;
    request.onerror = unexpectedErrorCallback;
}

function getDatabaseNamesSuccess3()
{
    var databaseNames;
    evalAndLog("databaseNames = event.target.result");
    shouldBeFalse("databaseNames.contains('" + name + "')");
    shouldBeFalse("databaseNames.contains('DATABASE THAT DOES NOT EXIST')");

    request = evalAndLog("indexedDB.deleteDatabase('DATABASE THAT DOES NOT EXIST')");
    request.onsuccess = deleteDatabaseSuccess2;
    request.onerror = unexpectedErrorCallback;
}

function deleteDatabaseSuccess2()
{
    finishJSTest();
}

test();