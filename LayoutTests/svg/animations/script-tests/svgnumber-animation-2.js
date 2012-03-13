description("Test for SVGNumber animation with different exponents.");
createSVGTestCase();

// Setup test document
var rect = createSVGElement("rect");
rect.setAttribute("id", "rect");
rect.setAttribute("x", "0");
rect.setAttribute("width", "100");
rect.setAttribute("height", "100");
rect.setAttribute("fill", "green");
rect.setAttribute("opacity", "0");
rect.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "opacity");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
animate.setAttribute("from", "0e10");
animate.setAttribute("to", ".1e1");
rect.appendChild(animate);
rootSVGElement.appendChild(rect);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBeCloseEnough("getComputedStyle(rect).getPropertyCSSValue('opacity').getFloatValue(CSSPrimitiveValue.CSS_NUMBER)", "0");
}

function sample2() {
    shouldBeCloseEnough("getComputedStyle(rect).getPropertyCSSValue('opacity').getFloatValue(CSSPrimitiveValue.CSS_NUMBER)", "0.5");
}

function sample3() {
    shouldBeCloseEnough("getComputedStyle(rect).getPropertyCSSValue('opacity').getFloatValue(CSSPrimitiveValue.CSS_NUMBER)", "1");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample1],
        ["animation", 2.0,   sample2],
        ["animation", 3.999, sample3],
        ["animation", 4.001, sample1]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
