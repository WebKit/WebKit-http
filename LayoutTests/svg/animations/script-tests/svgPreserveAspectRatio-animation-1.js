description("Test 'to' animation of SVGPreserveAspectRatio.");
createSVGTestCase();

// Setup test document
rootSVGElement.setAttribute("id", "root");
rootSVGElement.setAttribute("preserveAspectRatio", "xMaxYMin meet");

var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("fill", "green");
rect.setAttribute("onclick", "executeTest()");
rootSVGElement.appendChild(rect);

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "preserveAspectRatio");
animate.setAttribute("begin", "rect.click");
animate.setAttribute("dur", "4s");
animate.setAttribute("from", "xMinYMin meet");
animate.setAttribute("to", "xMaxYMid slice");
rootSVGElement.appendChild(animate);

// Setup animation test
function sample1() {
	shouldBe("rootSVGElement.preserveAspectRatio.animVal.align", "SVGPreserveAspectRatio.SVG_PRESERVEASPECTRATIO_XMAXYMIN");
	shouldBe("rootSVGElement.preserveAspectRatio.animVal.meetOrSlice", "SVGPreserveAspectRatio.SVG_MEETORSLICE_MEET");
}

function sample2() {
	shouldBe("rootSVGElement.preserveAspectRatio.animVal.align", "SVGPreserveAspectRatio.SVG_PRESERVEASPECTRATIO_XMINYMIN");
	shouldBe("rootSVGElement.preserveAspectRatio.animVal.meetOrSlice", "SVGPreserveAspectRatio.SVG_MEETORSLICE_MEET");
}

function sample3() {
	shouldBe("rootSVGElement.preserveAspectRatio.animVal.align", "SVGPreserveAspectRatio.SVG_PRESERVEASPECTRATIO_XMAXYMID");
	shouldBe("rootSVGElement.preserveAspectRatio.animVal.meetOrSlice", "SVGPreserveAspectRatio.SVG_MEETORSLICE_SLICE");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, elementId, sampleCallback]
        ["animation", 0.0,    "root", sample1],
        ["animation", 2.0,    "root", sample2],
        ["animation", 3.9999, "root", sample3],
        ["animation", 4.0 ,   "root", sample1]
    ];

    runAnimationTest(expectedValues);
}

// Begin test async
window.setTimeout("triggerUpdate(50, 50)", 0);
var successfullyParsed = true;
