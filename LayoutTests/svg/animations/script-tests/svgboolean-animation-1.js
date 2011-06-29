description("Test 'to' animation of SVGBoolean.");
createSVGTestCase();

// Setup test document
var defs = createSVGElement("defs");
rootSVGElement.appendChild(defs);

var filter = createSVGElement("filter");
filter.setAttribute("id", "filter");
defs.appendChild(filter);

var feConvolveMatrix = createSVGElement("feConvolveMatrix");
feConvolveMatrix.setAttribute("id", "effect");
feConvolveMatrix.setAttribute("kernelMatrix", "0 0 0   0 1 0   0 0 0");
feConvolveMatrix.setAttribute("preserveAlpha", "false");
filter.appendChild(feConvolveMatrix);

var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("fill", "green");
rect.setAttribute("filter", "url(#filter)");
rect.setAttribute("onclick", "executeTest()");
rootSVGElement.appendChild(rect);

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "preserveAlpha");
animate.setAttribute("begin", "rect.click");
animate.setAttribute("dur", "4s");
animate.setAttribute("from", "false");
animate.setAttribute("to", "true");
feConvolveMatrix.appendChild(animate);

// Setup animation test
function sample1() {
	shouldBe("feConvolveMatrix.preserveAlpha.animVal", "false");
}

function sample2() {
	shouldBe("feConvolveMatrix.preserveAlpha.animVal", "false");
}

function sample3() {
	shouldBe("feConvolveMatrix.preserveAlpha.animVal", "true");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, elementId, sampleCallback]
        ["animation", 0.0,    "effect", sample1],
        ["animation", 2.0,    "effect", sample2],
        ["animation", 3.9999, "effect", sample3],
        ["animation", 4.0 ,   "effect", sample1]
    ];

    runAnimationTest(expectedValues);
}

// Begin test async
window.setTimeout("triggerUpdate(50, 50)", 0);
var successfullyParsed = true;
