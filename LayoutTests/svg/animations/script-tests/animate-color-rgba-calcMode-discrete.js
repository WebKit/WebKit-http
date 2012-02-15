description("Test calcMode discrete with from-to animation on colors with alpha channel. You should see a green 100x100 rect and only PASS messages");
createSVGTestCase();

// Setup test document
var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("color", "rgba(0,255,255, 0.4)");
rect.setAttribute("fill", "currentColor");
rect.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "color");
animate.setAttribute("from", "rgba(255,0,0,0.6)");
animate.setAttribute("to", "rgba(0,255,255,0.8)");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
animate.setAttribute("calcMode", "discrete");
rect.appendChild(animate);
rootSVGElement.appendChild(rect);

// Setup animation test
function sample1() {
    shouldBeEqualToString("document.defaultView.getComputedStyle(rect).getPropertyValue('color')", "rgba(255, 0, 0, 0.597656)");
}

function sample2() {
    shouldBeEqualToString("document.defaultView.getComputedStyle(rect).getPropertyValue('color')", "rgba(0, 255, 255, 0.796875)");
}

function sample3() {
    shouldBeEqualToString("document.defaultView.getComputedStyle(rect).getPropertyValue('color')", "rgba(0, 255, 255, 0.398438)");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample3],
        ["animation", 0.001, sample1],
        ["animation", 1.0,   sample1],
        ["animation", 3.0,   sample2],
        ["animation", 3.999, sample2],
        ["animation", 4.0,   sample3]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
