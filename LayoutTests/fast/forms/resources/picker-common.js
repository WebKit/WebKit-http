window.jsTestIsAsync = true;
if (window.internals)
    internals.setEnableMockPagePopup(true);

var popupWindow = null;

var popupOpenCallback = null;
function openPicker(input, callback) {
    input.offsetTop; // Force to lay out
    if (input.type === "color") {
        input.focus();
        eventSender.keyDown(" ");
    } else {
        sendKey(input, "Down", false, true);
    }
    popupWindow = document.getElementById('mock-page-popup').contentWindow;
    if (typeof callback === "function") {
        popupOpenCallback = callback;
        popupWindow.addEventListener("didOpenPicker", popupOpenCallbackWrapper, false);
    }
}

function popupOpenCallbackWrapper() {
    popupWindow.removeEventListener("didOpenPicker", popupOpenCallbackWrapper);
    popupOpenCallback();
}

function waitUntilClosing(callback) {
    setTimeout(callback, 1);
}

function sendKey(input, keyName, ctrlKey, altKey) {
    var event = document.createEvent('KeyboardEvent');
    event.initKeyboardEvent('keydown', true, true, document.defaultView, keyName, 0, ctrlKey, altKey);
    input.dispatchEvent(event);
}


