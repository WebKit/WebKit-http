// [Name] SVGRadialGradientElement-dom-fx-attr.js
// [Expected rendering result] green ellipse, red radial gradient centered in the middle of the ellipse - and a series of PASS messages

description("Tests dynamic updates of the 'fx' attribute of the SVGRadialGradientElement object")
createSVGTestCase();

var ellipseElement = createSVGElement("ellipse");
ellipseElement.setAttribute("cx", "150");
ellipseElement.setAttribute("cy", "150");
ellipseElement.setAttribute("rx", "100");
ellipseElement.setAttribute("ry", "150");
ellipseElement.setAttribute("fill", "url(#gradient)");

var defsElement = createSVGElement("defs");
rootSVGElement.appendChild(defsElement);

var radialGradientElement = createSVGElement("radialGradient");
radialGradientElement.setAttribute("id", "gradient");
radialGradientElement.setAttribute("fx", "0%");

var firstStopElement = createSVGElement("stop");
firstStopElement.setAttribute("offset", "0");
firstStopElement.setAttribute("stop-color", "red");
radialGradientElement.appendChild(firstStopElement);

var lastStopElement = createSVGElement("stop");
lastStopElement.setAttribute("offset", "1");
lastStopElement.setAttribute("stop-color", "green");
radialGradientElement.appendChild(lastStopElement);

defsElement.appendChild(radialGradientElement);
rootSVGElement.appendChild(ellipseElement);

shouldBeEqualToString("radialGradientElement.getAttribute('fx')", "0%");

function repaintTest() {
    radialGradientElement.setAttribute("fx", "50%");
    shouldBeEqualToString("radialGradientElement.getAttribute('fx')", "50%");

    completeTest();
}

var successfullyParsed = true;
