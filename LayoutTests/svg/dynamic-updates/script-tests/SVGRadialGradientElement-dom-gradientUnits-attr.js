// [Name] SVGRadialGradientElement-dom-gradientUnits-attr.js
// [Expected rendering result] radial gradient filled from left edge to right edge of green ellipse - and a series of PASS messages

description("Tests dynamic updates of the 'gradientUnits' attribute of the SVGRadialGradientElement object")
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

shouldBeNull("radialGradientElement.getAttribute('gradientUnits')");

function repaintTest() {
    radialGradientElement.setAttribute("gradientUnits", "userSpaceOnUse");
    shouldBeEqualToString("radialGradientElement.getAttribute('gradientUnits')", "userSpaceOnUse");
 
    completeTest();
}

var successfullyParsed = true;
