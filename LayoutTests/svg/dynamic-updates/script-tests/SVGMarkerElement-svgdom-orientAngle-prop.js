// [Name] SVGMarkerElement-svgdom-orientAngle-prop.js
// [Expected rendering result] start & end markers are visible (and not rotated, compared to the other SVGMarkerElement* tests) - and a series of PASS messages

description("Tests dynamic updates of the 'orientAngle' property of the SVGMarkerElement object")
createSVGTestCase();

var markerElement = createSVGElement("marker");
markerElement.setAttribute("id", "marker");
markerElement.setAttribute("viewBox", "0 0 10 10");
markerElement.setAttribute("markerWidth", "2");
markerElement.setAttribute("markerHeight", "2");
markerElement.setAttribute("refX", "5");
markerElement.setAttribute("refY", "5");
markerElement.setAttribute("markerUnits", "strokeWidth");
markerElement.setAttribute("orient", "45");

var markerPathElement = createSVGElement("path");
markerPathElement.setAttribute("fill", "blue");
markerPathElement.setAttribute("d", "M 5 0 L 10 10 L 0 10 Z");
markerElement.appendChild(markerPathElement);

var defsElement = createSVGElement("defs");
defsElement.appendChild(markerElement);
rootSVGElement.appendChild(defsElement);

var pathElement = createSVGElement("path");
pathElement.setAttribute("fill", "none");
pathElement.setAttribute("stroke", "green");
pathElement.setAttribute("stroke-width", "10");
pathElement.setAttribute("marker-start", "url(#marker)");
pathElement.setAttribute("marker-end", "url(#marker)");
pathElement.setAttribute("d", "M 130 135 L 180 135 L 180 185");
rootSVGElement.appendChild(pathElement);

shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
shouldBe("markerElement.orientAngle.baseVal.value", "45");

function repaintTest() {
    markerElement.orientAngle.baseVal.value = 0;

    shouldBe("markerElement.orientType.baseVal", "SVGMarkerElement.SVG_MARKER_ORIENT_ANGLE");
    shouldBe("markerElement.orientAngle.baseVal.value", "0");

    completeTest();
}

var successfullyParsed = true;
