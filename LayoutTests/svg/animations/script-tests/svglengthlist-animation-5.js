description("Test additive='sum' animation of SVGLengthList.");
createSVGTestCase();

// Setup test document
var text = createSVGElement("text");
text.setAttribute("id", "text");
text.textContent = "ABCD";
text.setAttribute("x", "50 60 70 80");
text.setAttribute("y", "50");
text.setAttribute("onclick", "executeTest()");
rootSVGElement.appendChild(text);

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "x");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
animate.setAttribute("additive", "sum");
animate.setAttribute("from", "0 0 0 0");
animate.setAttribute("to", "20 20 20 20");
text.appendChild(animate);

// Setup animation test
function sample1() {
	shouldBe("text.x.animVal.getItem(0).value", "50");
	shouldBe("text.x.animVal.getItem(1).value", "60");
	shouldBe("text.x.animVal.getItem(2).value", "70");
	shouldBe("text.x.animVal.getItem(3).value", "80");
}

function sample2() {
	shouldBe("text.x.animVal.getItem(0).value", "60");
	shouldBe("text.x.animVal.getItem(1).value", "70");
	shouldBe("text.x.animVal.getItem(2).value", "80");
	shouldBe("text.x.animVal.getItem(3).value", "90");
}

function sample3() {
	shouldBeCloseEnough("text.x.animVal.getItem(0).value", "70", 0.01);
	shouldBeCloseEnough("text.x.animVal.getItem(1).value", "80", 0.01);
	shouldBeCloseEnough("text.x.animVal.getItem(2).value", "90", 0.01);
	shouldBeCloseEnough("text.x.animVal.getItem(3).value", "100", 0.01);
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, elementId, sampleCallback]
        ["animation", 0.0,    "text", sample1],
        ["animation", 2.0,    "text", sample2],
        ["animation", 3.9999, "text", sample3],
        ["animation", 4.0 ,   "text", sample1]
    ];

    runAnimationTest(expectedValues);
}

// Begin test async
window.setTimeout("triggerUpdate(51, 49)", 0);
var successfullyParsed = true;
