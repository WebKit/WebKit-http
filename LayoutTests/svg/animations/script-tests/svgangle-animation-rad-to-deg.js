description("Tests SVGAngle animation from rad to deg.");
createSVGTestCase();

// Setup test document
var defs = createSVGElement("defs");

var marker = createSVGElement("marker");
marker.setAttribute("id", "marker");
marker.setAttribute("viewBox", "0 0 10 10");
marker.setAttribute("markerWidth", "4");
marker.setAttribute("markerHeight", "3");
marker.setAttribute("markerUnits", "strokeWidth");
marker.setAttribute("refX", "1");
marker.setAttribute("refY", "5");
marker.setAttribute("orient", "0deg");
defs.appendChild(marker);

var polyline = createSVGElement("polyline");
polyline.setAttribute("id", "polyline");
polyline.setAttribute("points", "0,0 10,5 0,10 1,5");
polyline.setAttribute("fill", "green");
marker.appendChild(polyline);

var path = createSVGElement("path");
path.setAttribute("id", "path");
path.setAttribute("d", "M45,50 L55,50");
path.setAttribute("stroke-width","10");
path.setAttribute("stroke", "green");
path.setAttribute("marker-end", "url(#marker)");
path.setAttribute("onclick", "executeTest()");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "orient");
animate.setAttribute("begin", "path.click");
animate.setAttribute("dur", "4s");
animate.setAttribute("from", "0rad");
animate.setAttribute("to", "180deg");
marker.appendChild(animate);
rootSVGElement.appendChild(defs);
rootSVGElement.appendChild(path);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBe("marker.orientAngle.animVal.value", "0");
    shouldBe("marker.orientAngle.baseVal.value", "0");
}

function sample2() {
    shouldBeCloseEnough("marker.orientAngle.animVal.value", "90", 0.1);
    shouldBeCloseEnough("marker.orientAngle.baseVal.value", "90", 0.1);
}

function sample3() {
    shouldBeCloseEnough("marker.orientAngle.animVal.value", "180", 0.1);
    shouldBeCloseEnough("marker.orientAngle.baseVal.value", "180", 0.1);
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, elementId, sampleCallback]
        ["animation", 0.0,    "marker", sample1],
        ["animation", 2.0,    "marker", sample2],
        ["animation", 3.9999, "marker", sample3],
        ["animation", 4.0 ,   "marker", sample1]
    ];

    runAnimationTest(expectedValues);
}

// Begin test async
window.setTimeout("triggerUpdate(50, 50)", 0);
var successfullyParsed = true;
