description("Test that XHR.responseType = 'blob' gives you back a Blob.");

if (window.testRunner)
    testRunner.waitUntilDone();

function testBlob(blobURL, blobType, doneFunction) {
    window.xhr = new XMLHttpRequest();
    xhr.open("GET", blobURL);
    xhr.responseType = "blob";
    shouldBeEqualToString("xhr.responseType", "blob");
    xhr.send();
    xhr.onreadystatechange = function() {
        if (xhr.readyState != 4) {
            shouldBeNull("xhr.response");
            return;
        }
        shouldBeTrue("xhr.response instanceof Blob");
        shouldBeEqualToString("xhr.response.type", blobType);
        doneFunction();
    }
}

testBlob("resources/UTF8.txt", "text/plain", function() {
    testBlob("resources/does_not_exist.txt", "", function() {
        testBlob("resources/empty-file", "", function() {
            if (window.testRunner)
                testRunner.notifyDone();
        })
    })
});
