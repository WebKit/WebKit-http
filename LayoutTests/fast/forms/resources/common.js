function $(id) {
    return document.getElementById(id);
}

function getValidationMessageBubbleNode(host) {
    // FIXME: We should search for a pseudo ID.
    return layoutTestController.shadowRoot(host).lastChild;
}

function getAbsoluteRect(element) {
    var rect = element.getBoundingClientRect();
    rect.top += document.body.scrollTop;
    rect.bottom += document.body.scrollTop;
    rect.left += document.body.scrollLeft;
    rect.right += document.body.scrollLeft;
    return rect;
}

function searchCancelButtonPosition(element) {
    var pos = {};
    pos.x = element.offsetLeft + element.offsetWidth - 9;
    pos.y = element.offsetTop + element.offsetHeight / 2;
    return pos;
}
