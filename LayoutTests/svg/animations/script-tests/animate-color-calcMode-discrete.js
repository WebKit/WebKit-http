description("Test calcMode discrete with from-to animation on colors. You should see a green 100x100 rect and only PASS messages");
createSVGTestCase();

// Setup test document
var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("color", "rgb(0,255,255)");
rect.setAttribute("fill", "currentColor");
rect.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "color");
animate.setAttribute("from", "rgb(255,0,0)");
animate.setAttribute("to", "rgb(0,255,255)");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
animate.setAttribute("calcMode", "discrete");
rect.appendChild(animate);
rootSVGElement.appendChild(rect);

// Setup animation test
function sample1() {
    shouldBeEqualToString("document.defaultView.getComputedStyle(rect).getPropertyValue('color')", "rgb(255, 0, 0)");
}

function sample2() {
    shouldBeEqualToString("document.defaultView.getComputedStyle(rect).getPropertyValue('color')", "rgb(0, 255, 255)");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample2],
        ["animation", 0.001, sample1],
        ["animation", 1.0,   sample1],
        ["animation", 3.0,   sample2],
        ["animation", 3.999, sample2],
        ["animation", 4.0,   sample2]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
