// [Name] SVGLineElement-svgdom-x1-prop.js
// [Expected rendering result] green line from 10x10 to 200x200 - and a series of PASS messages

description("Tests dynamic updates of the 'x1' property of the SVGLineElement object")
createSVGTestCase();

var lineElement = createSVGElement("line");
lineElement.setAttribute("x1", "100");
lineElement.setAttribute("y1", "10");
lineElement.setAttribute("x2", "200");
lineElement.setAttribute("y2", "200");
lineElement.setAttribute("stroke", "green");
lineElement.setAttribute("stroke-width", "10");
rootSVGElement.appendChild(lineElement);

shouldBe("lineElement.x1.baseVal.value", "100");

function repaintTest() {
    lineElement.x1.baseVal.value = 10;
    shouldBe("lineElement.x1.baseVal.value", "10");

    completeTest();
}

var successfullyParsed = true;
