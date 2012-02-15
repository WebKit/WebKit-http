// [Name] SVGLineElement-svgdom-y2-prop.js
// [Expected rendering result] green line from 10x10 to 200x200 - and a series of PASS messages

description("Tests dynamic updates of the 'y2' property of the SVGLineElement object")
createSVGTestCase();

var lineElement = createSVGElement("line");
lineElement.setAttribute("x1", "10");
lineElement.setAttribute("y1", "10");
lineElement.setAttribute("x2", "200");
lineElement.setAttribute("y2", "100");
lineElement.setAttribute("stroke", "green");
lineElement.setAttribute("stroke-width", "10");
rootSVGElement.appendChild(lineElement);

shouldBe("lineElement.y2.baseVal.value", "100");

function repaintTest() {
    lineElement.y2.baseVal.value = 200;
    shouldBe("lineElement.y2.baseVal.value", "200");

    completeTest();
}

var successfullyParsed = true;
