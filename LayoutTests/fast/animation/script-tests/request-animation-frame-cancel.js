description("This tests cancelling a webkitRequestAnimationFrame callback");

var callbackFired = false;
var e = document.getElementById("e");
var id = window.webkitRequestAnimationFrame(function() {
}, e);

window.webkitCancelRequestAnimationFrame(id);

if (window.layoutTestController)
    layoutTestController.display();

setTimeout(function() {
    shouldBeFalse("callbackFired");
}, 100);

if (window.layoutTestController)
    layoutTestController.waitUntilDone();

var successfullyParsed = true;

setTimeout(function() {
    isSuccessfullyParsed();
    if (window.layoutTestController)
        layoutTestController.notifyDone();
}, 200);
