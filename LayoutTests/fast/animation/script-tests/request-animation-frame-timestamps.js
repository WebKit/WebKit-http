description("Tests the timestamps provided to requestAnimationFrame callbacks");

function busyWait(millis) {
    var start = Date.now();
    while (Date.now()-start < millis) {}
}

var firstTimestamp = undefined;

window.webkitRequestAnimationFrame(function(timestamp) {
    firstTimestamp = timestamp;
    shouldBeDefined("firstTimestamp");
    busyWait(10);
});

var secondTimestamp = undefined;
window.webkitRequestAnimationFrame(function(timestamp) {
    secondTimestamp = timestamp;
    shouldBeDefined("secondTimestamp");
    shouldBe("firstTimestamp", "secondTimestamp");
});

if (window.layoutTestController)
    layoutTestController.display();

if (window.layoutTestController)
    layoutTestController.waitUntilDone();

setTimeout(function() {
    shouldBeDefined("firstTimestamp");
}, 100);

var successfullyParsed = true;

setTimeout(function() {
    isSuccessfullyParsed();
    if (window.layoutTestController)
        layoutTestController.notifyDone();
}, 200);
