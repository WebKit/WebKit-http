// FIXME: This test will become useful once we have basic animVal support. For now it's just testing the SVG animation test infrastructure
description("Trivial animVal testcase, to see wheter we support it at all. Should result in a 200x200 rect and only PASS messages");
createSVGTestCase();

// Setup test document
var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("width", "200");
rect.setAttribute("height", "200");
rect.setAttribute("fill", "green");
rect.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "width");
animate.setAttribute("from", "200");
animate.setAttribute("to", "100");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
rect.appendChild(animate);
rootSVGElement.appendChild(rect);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBe("rect.width.animVal.value", "200");
    shouldBe("rect.width.baseVal.value", "200");
}

function sample2() {
    // Check half-time conditions
    shouldBe("rect.width.animVal.value", "150");
    shouldBe("rect.width.baseVal.value", "150");
}

function sample3() {
    // Check just before-end conditions
    shouldBeCloseEnough("rect.width.animVal.value", "100");
    shouldBeCloseEnough("rect.width.baseVal.value", "100");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample1],
        ["animation", 2.0,   sample2],
        ["animation", 3.999, sample3],
        ["animation", 4.0,   sample1]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
