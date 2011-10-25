description("Tests when line breaking is allowed after a question mark character.");

var div = document.body.appendChild(document.createElement("div"));
div.style.width = "1px";
div.style.fontFamily = "ahem";
div.style.fontSize = "25px";

function allowsBreakBefore(i)
{
    div.innerText = "A?" + String.fromCharCode(i);
    return div.offsetHeight > 25;
}

var unallowedBreaks = [0x21, 0x22, 0x27, 0x29, 0x2c, 0x2e, 0x2f, 0x3a, 0x3b, 0x3f, 0x5d, 0x7d];

for (var i = 33; i < 128; ++i)
    shouldBe("allowsBreakBefore(" + i + ")", unallowedBreaks.indexOf(i) >= 0 ? "false" : "true");

document.body.removeChild(div);
