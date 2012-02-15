// [Name] SVGImageElement-svgdom-x-prop.js
// [Expected rendering result] image at 0x0 size 200x200 - and a series of PASS messages

description("Tests dynamic updates of the 'x' property of the SVGImageElement object")
createSVGTestCase();

var imageElement = createSVGElement("image");
imageElement.setAttributeNS(xlinkNS, "xlink:href", "../custom/resources/green-checker.png");
imageElement.setAttribute("preserveAspectRatio", "none");
imageElement.setAttribute("x", "-190");
imageElement.setAttribute("y", "0");
imageElement.setAttribute("width", "200");
imageElement.setAttribute("height", "200");
rootSVGElement.appendChild(imageElement);

shouldBe("imageElement.x.baseVal.value", "-190");

function repaintTest() {
    imageElement.x.baseVal.value = 0;
    shouldBe("imageElement.x.baseVal.value", "0");

    completeTest();
}

var successfullyParsed = true;
