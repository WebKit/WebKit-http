if (this.importScripts) {
    importScripts('../../../fast/js/resources/js-test-pre.js');
    importScripts('shared.js');
}

description("Test IndexedDB cursor.advance().");

var objectStoreData = [
    { key: "237-23-7732", value: { name: "Bob", height: 60, weight: 120 } },
    { key: "237-23-7733", value: { name: "Ann", height: 52, weight: 110 } },
    { key: "237-23-7734", value: { name: "Ron", height: 73, weight: 180 } },
    { key: "237-23-7735", value: { name: "Sue", height: 58, weight: 130 } },
    { key: "237-23-7736", value: { name: "Joe", height: 65, weight: 150 } },
    { key: "237-23-7737", value: { name: "Pat", height: 65, weight: 100 } },
    { key: "237-23-7738", value: { name: "Leo", height: 65, weight: 180 } },
    { key: "237-23-7739", value: { name: "Jef", height: 65, weight: 120 } },
    { key: "237-23-7740", value: { name: "Sam", height: 71, weight: 110 } },
    { key: "237-23-7741", value: { name: "Bug", height: 63, weight: 100 } },
    { key: "237-23-7742", value: { name: "Tub", height: 69, weight: 180 } },
    { key: "237-23-7743", value: { name: "Rug", height: 77, weight: 120 } },
    { key: "237-23-7744", value: { name: "Pug", height: 66, weight: 110 } },
];

var indexData = [
    { name: "name", keyPath: "name", options: { unique: true } },
    { name: "height", keyPath: "height", options: { } },
    { name: "weight", keyPath: "weight", options: { unique: false } }
];

function test()
{
    removeVendorPrefixes();
    name = window.location.pathname;
    request = evalAndLog("indexedDB.open(name)");
    request.onsuccess = openSuccess;
    request.onerror = unexpectedErrorCallback;
}

function openSuccess()
{
    debug("openSuccess():");
    db = evalAndLog("db = event.target.result");

    objectStoreName = "People";

    request = evalAndLog("request = db.setVersion('1')");
    request.onsuccess = createObjectStore;
    request.onerror = unexpectedErrorCallback;
}

function createObjectStore(request)
{
    deleteAllObjectStores(db);
    trans = evalAndLog("trans = request.result");
    objectStore = evalAndLog("objectStore = db.createObjectStore(objectStoreName);");
    createIndexes();
    trans.oncomplete = function() {
        evalAndLog("trans = db.transaction(objectStoreName, 'readwrite')");
        evalAndLog("objectStore = trans.objectStore(objectStoreName)");
        populateObjectStore();
    };
}

function populateObjectStore()
{
    debug("First, add all our data to the object store.");
    addedData = 0;
    for (i in objectStoreData) {
        request = evalAndLog("request = objectStore.add(objectStoreData[i].value, objectStoreData[i].key);");
        request.onerror = unexpectedErrorCallback;
    }
    request.onsuccess = testSimple;
}

function createIndexes()
{
    debug("Now create the indexes.");
    for (i in indexData) {
      evalAndLog("objectStore.createIndex(indexData[i].name, indexData[i].keyPath, indexData[i].options);");
    }
}

// Make a test that uses continue() to get to startPos, then passes
// 'count' to get to a position.
function makeAdvanceTest(startPos, direction, count, expectedValue, indexName)
{
    result = {};
    evalAndLog("trans = db.transaction(objectStoreName)");
    store = evalAndLog("store = trans.objectStore(objectStoreName)");
    if (indexName)
        store = store.index(indexName);

    if (direction)
        evalAndLog("request = store.openCursor(null, " + direction + ")");
    else
        evalAndLog("request = store.openCursor()");
    var currentPos = 0;
    function continueToTest(event)
    {
        cursor = event.target.result;
        if (currentPos == startPos) {
            evalAndLog("cursor.advance(" + count + ")");
        } else if (currentPos == startPos + 1) {
            runTest(cursor, expectedValue);
            result.onsuccess();
        } else {
            if (cursor == null)
                result.onsuccess();
            else {
                evalAndLog("cursor.continue();");
            }
        }
        currentPos++;
    }

    request.onsuccess = continueToTest;
    request.onerror = function(e)
    {
        result.onerror(e);
    };

    return result;
}

function simplifyCursor(cursor)
{
    var obj = {};
    if (cursor === null) {
        return null;
    }
    if ('key' in cursor) {
        obj.key = cursor.key;
    }

    if ('value' in cursor) {
        obj.value = cursor.value;
    }

    if ('primaryKey' in cursor) {
        obj.primaryKey = cursor.primaryKey;
    }
    return obj;
}

function runTest(cursor, expectedValue)
{
    expected = JSON.stringify(expectedValue);
    shouldBeEqualToString("expected", JSON.stringify(simplifyCursor(cursor)));
}


function testSimple()
{
    debug("testSimple()");
    makeAdvanceTest(0, null, 1,
                    {key: objectStoreData[1].key,
                     value: objectStoreData[1].value,
                     primaryKey: objectStoreData[1].key})
                         .onsuccess= testContinueThenAdvance;
}

function testContinueThenAdvance()
{
    debug("testContinueThenAdvance()");
    makeAdvanceTest(3, null, 1,
                    // Joe
                    {key: objectStoreData[4].key,
                     value: objectStoreData[4].value,
                     primaryKey: objectStoreData[4].key})
                         .onsuccess= testAdvanceMultiple;
}

function testAdvanceMultiple()
{
    debug("testAdvanceMultiple()");
    makeAdvanceTest(0, null, 3,
                    {key: objectStoreData[3].key,
                     value: objectStoreData[3].value,
                     primaryKey: objectStoreData[3].key})
                         .onsuccess = testAdvanceIndex;
}

function testAdvanceIndex()
{
    debug("testAdvanceIndex()");
    makeAdvanceTest(0, null, 3,
                    // Jef
                    {key: objectStoreData[7].value.name,
                     value: objectStoreData[7].value,
                     primaryKey: objectStoreData[7].key},
                    "name").onsuccess = testAdvanceIndexNoDupe;
}

function testAdvanceIndexNoDupe()
{
    debug("testAdvanceIndexNoDupe()");
    makeAdvanceTest(0, "'nextunique'", 3,
                    // Sue (weight 130 - skipping weight 100, 110, 120)
                    {key: objectStoreData[3].value.weight,
                     value: objectStoreData[3].value,
                     primaryKey: objectStoreData[3].key},
                    "weight").onsuccess = testAdvanceIndexPrev;
}

function testAdvanceIndexPrev()
{
    debug("testAdvanceIndexPrev()");
    makeAdvanceTest(0, "'prev'", 3,
                    // Joe (weight 150 - skipping 180, 180, 180)
                    {key: objectStoreData[4].value.weight,
                     value: objectStoreData[4].value,
                     primaryKey: objectStoreData[4].key},
                    "weight").onsuccess = testAdvanceIndexPrevNoDupe;
}

function testAdvanceIndexPrevNoDupe()
{
    debug("testAdvanceIndexPrevNoDupe()");
    makeAdvanceTest(0, "'prevunique'", 3,
                    // Bob (weight 120 - skipping weights 180, 150, 130)
                    {key: objectStoreData[0].value.weight,
                     value: objectStoreData[0].value,
                     primaryKey: objectStoreData[0].key},
                    "weight").onsuccess = testAdvanceToEnd;
}

function testAdvanceToEnd()
{
    debug("testAdvanceToEnd()");
    makeAdvanceTest(0, null, 100, null)
        .onsuccess = testPrefetchInRange;
}

// Make sure the prefetching that exists on some platforms (chromium)
// doesn't mess with advance(), or vice versa.
function testPrefetchInRange()
{
    debug("testPrefetchInRange()");
    var kPrefetchContinueThreshold = 2;
    evalAndLog("trans = db.transaction(objectStoreName)");
    objectStore = evalAndLog("objectStore = trans.objectStore(objectStoreName)");

    evalAndLog("request = objectStore.openCursor()");

    var currentPos = 0;
    function prefetch()
    {
        cursor = event.target.result;

        if (!cursor) {
            testPrefetchOutOfRange();
            return;
        }

        runTest(cursor, {key: objectStoreData[currentPos].key,
                         value: objectStoreData[currentPos].value,
                         primaryKey: objectStoreData[currentPos].key});

        // force some prefetching,
        if (currentPos < (kPrefetchContinueThreshold+1)) {
            evalAndLog("cursor.continue()");
            currentPos++;
        } else if (currentPos == (kPrefetchContinueThreshold+1)) {
            // stay within the prefetch range
            evalAndLog("cursor.advance(2)");
            currentPos += 2;
        } else {
            // we're just past the threshold
            evalAndLog("cursor.continue()");
            currentPos++;
        }
    }
    request.onsuccess = prefetch;
    request.onerror = unexpectedErrorCallback;
}

// Make sure the prefetching that exists on some platforms (chromium)
// doesn't mess with advance(), or vice versa.
function testPrefetchOutOfRange()
{
    debug("testPrefetchOutOfRange()");
    var kPrefetchContinueThreshold = 2;
    evalAndLog("trans = db.transaction(objectStoreName)");
    objectStore = evalAndLog("objectStore = trans.objectStore(objectStoreName)");

    evalAndLog("request = objectStore.openCursor()");

    var currentPos = 0;
    function prefetch()
    {
        cursor = event.target.result;

        if (!cursor) {
            testBadAdvance();
            return;
        }

        runTest(cursor, {key: objectStoreData[currentPos].key,
                         value: objectStoreData[currentPos].value,
                         primaryKey: objectStoreData[currentPos].key});

        // force some prefetching,
        if (currentPos < (kPrefetchContinueThreshold+1)) {
            evalAndLog("cursor.continue()");
            currentPos++;
        } else if (currentPos == (kPrefetchContinueThreshold+1)) {
            // advance past the prefetch range
            evalAndLog("cursor.advance(7)");
            currentPos += 7;
        } else {
            // we're past the threshold
            evalAndLog("cursor.continue()");
            currentPos++;
        }
    }
    request.onsuccess = prefetch;
    request.onerror = unexpectedErrorCallback;
}

function testBadAdvance()
{
    debug("testBadAdvance()");
    evalAndLog("trans = db.transaction(objectStoreName, 'readwrite')");
    objectStore = evalAndLog("objectStore = trans.objectStore(objectStoreName)");

    evalAndLog("request = objectStore.openCursor()");

    function advanceBadly()
    {
        cursor = event.target.result;

        evalAndExpectExceptionClass("cursor.advance(0)", "TypeError");
        testDelete();
    }
    request.onsuccess = advanceBadly;
    request.onerror = unexpectedErrorCallback;

}

function testDelete()
{
    debug("testDelete()");
    evalAndLog("trans = db.transaction(objectStoreName, 'readwrite')");
    objectStore = evalAndLog("objectStore = trans.objectStore(objectStoreName)");

    evalAndLog("request = objectStore.openCursor()");

    var currentPos = 0;
    function deleteSecond()
    {
        cursor = event.target.result;

        if (!cursor) {
            finishJSTest();
            return;
        }

        runTest(cursor, {key: objectStoreData[currentPos].key,
                         value: objectStoreData[currentPos].value,
                         primaryKey: objectStoreData[currentPos].key});


        // this is in the middle of the data, so it will test validity
        if (currentPos == 2) {
            evalAndLog("cursor.delete()");
            cursor.advance(4);
            currentPos += 4;
        } else {
            evalAndLog("cursor.advance(1)");
            currentPos++;
        }

    }
    request.onsuccess = deleteSecond;
    request.onerror = unexpectedErrorCallback;
}

test();
