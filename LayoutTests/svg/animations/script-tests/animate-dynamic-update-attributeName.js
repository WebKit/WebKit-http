description("Test behavior on dynamic-update of attributeName");
createSVGTestCase();

// Setup test document
var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("width", "200");
rect.setAttribute("height", "200");
rect.setAttribute("fill", "red");
rect.setAttribute("color", "red");
rect.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "color");
animate.setAttribute("from", "green");
animate.setAttribute("to", "green");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "3s");
animate.setAttribute("fill", "freeze");
animate.setAttribute("calcMode", "discrete");
rect.appendChild(animate);
rootSVGElement.appendChild(rect);

// Setup animation test
function sample1() {
    shouldBe("getComputedStyle(rect).color", "'rgb(0, 128, 0)'");
    shouldBeEqualToString("rect.style.color", "");
}

function sample2() {
    // Set 'attributeName' from 'color' to 'fill'
    animate.setAttribute("attributeName", "fill");
}

function sample3() {
    shouldBe("getComputedStyle(rect).fill", "'#008000'");
    shouldBeEqualToString("rect.style.fill", "");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.001, sample1],
        ["animation", 1.5, sample2],
        ["animation", 3.0, sample3],
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
