// [Name] SVGRadialGradientElement-dom-gradientTransform-attr.js
// [Expected rendering result] green ellipse, no red visible - and a series of PASS messages

description("Tests dynamic updates of the 'gradientTransform' attribute of the SVGRadialGradientElement object")
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
radialGradientElement.setAttribute("gradientTransform", "matrix(1,0,0,1,0,0)");

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

shouldBeEqualToString("radialGradientElement.getAttribute('gradientTransform')", "matrix(1,0,0,1,0,0)");

function repaintTest() {
    radialGradientElement.setAttribute("gradientTransform", "matrix(1,0,0,1,50,0)");
    shouldBeEqualToString("radialGradientElement.getAttribute('gradientTransform')", "matrix(1,0,0,1,50,0)");

    completeTest();
}

var successfullyParsed = true;
