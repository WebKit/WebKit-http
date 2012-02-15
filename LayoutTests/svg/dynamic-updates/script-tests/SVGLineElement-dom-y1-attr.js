// [Name] SVGLineElement-dom-y1-attr.js
// [Expected rendering result] green line from 10x10 to 200x200 - and a series of PASS messages

description("Tests dynamic updates of the 'y1' attribute of the SVGLineElement object")
createSVGTestCase();

var lineElement = createSVGElement("line");
lineElement.setAttribute("x1", "10");
lineElement.setAttribute("y1", "100");
lineElement.setAttribute("x2", "200");
lineElement.setAttribute("y2", "200");
lineElement.setAttribute("stroke", "green");
lineElement.setAttribute("stroke-width", "10");
rootSVGElement.appendChild(lineElement);

shouldBeEqualToString("lineElement.getAttribute('y1')", "100");

function repaintTest() {
    lineElement.setAttribute("y1", "10");
    shouldBeEqualToString("lineElement.getAttribute('y1')", "10");

    completeTest();
}

var successfullyParsed = true;
