description("Test path animation where coordinate modes of start and end differ. You should see PASS messages");
createSVGTestCase();
// FIXME: We should move to animatedPathSegList, once it is implemented.

// Setup test document
var path = createSVGElement("path");
path.setAttribute("id", "path");
path.setAttribute("d", "M 30 30 l -60 -30 v -30 h 30 Z");
path.setAttribute("fill", "green");
path.setAttribute("onclick", "executeTest()");
path.setAttribute("transform", "translate(50, 50)");

var animate = createSVGElement("animate");
animate.setAttribute("id", "animation");
animate.setAttribute("attributeName", "d");
animate.setAttribute("from", "M 30 30 l -60 -30 v -30 h 30 Z");
animate.setAttribute("to", "M -30 -30 L 30 0 V 30 H 0 Z");
animate.setAttribute("begin", "click");
animate.setAttribute("dur", "4s");
path.appendChild(animate);
rootSVGElement.appendChild(path);

// Setup animation test
function sample1() {
    // Check initial/end conditions
    shouldBe("path.pathSegList.getItem(0).pathSegTypeAsLetter", "'M'");
    shouldBe("path.pathSegList.getItem(0).x", "30");
    shouldBe("path.pathSegList.getItem(0).y", "30");
    shouldBe("path.pathSegList.getItem(1).pathSegTypeAsLetter", "'l'");
    shouldBe("path.pathSegList.getItem(1).x", "-60");
    shouldBe("path.pathSegList.getItem(1).y", "-30");
    shouldBe("path.pathSegList.getItem(2).pathSegTypeAsLetter", "'v'");
    shouldBe("path.pathSegList.getItem(2).y", "-30");
    shouldBe("path.pathSegList.getItem(3).pathSegTypeAsLetter", "'h'");
    shouldBe("path.pathSegList.getItem(3).x", "30");
}

function sample2() {
    shouldBe("path.pathSegList.getItem(0).pathSegTypeAsLetter", "'M'");
    shouldBe("path.pathSegList.getItem(0).x", "15");
    shouldBe("path.pathSegList.getItem(0).y", "15");
    shouldBe("path.pathSegList.getItem(1).pathSegTypeAsLetter", "'l'");
    shouldBe("path.pathSegList.getItem(1).x", "-30");
    shouldBe("path.pathSegList.getItem(1).y", "-15");
    shouldBe("path.pathSegList.getItem(2).pathSegTypeAsLetter", "'v'");
    shouldBe("path.pathSegList.getItem(2).y", "-15");
    shouldBe("path.pathSegList.getItem(3).pathSegTypeAsLetter", "'h'");
    shouldBe("path.pathSegList.getItem(3).x", "15");
}

function sample3() {
    shouldBe("path.pathSegList.getItem(0).pathSegTypeAsLetter", "'M'");
    shouldBe("path.pathSegList.getItem(0).x", "-15");
    shouldBe("path.pathSegList.getItem(0).y", "-15");
    shouldBe("path.pathSegList.getItem(1).pathSegTypeAsLetter", "'L'");
    shouldBe("path.pathSegList.getItem(1).x", "15");
    shouldBe("path.pathSegList.getItem(1).y", "0");
    shouldBe("path.pathSegList.getItem(2).pathSegTypeAsLetter", "'V'");
    shouldBe("path.pathSegList.getItem(2).y", "15");
    shouldBe("path.pathSegList.getItem(3).pathSegTypeAsLetter", "'H'");
    shouldBe("path.pathSegList.getItem(3).x", "0");
}

function sample4() {
    shouldBe("path.pathSegList.getItem(0).pathSegTypeAsLetter", "'M'");
    shouldBeCloseEnough("path.pathSegList.getItem(0).x", "-30");
    shouldBeCloseEnough("path.pathSegList.getItem(0).y", "-30");
    shouldBe("path.pathSegList.getItem(1).pathSegTypeAsLetter", "'L'");
    shouldBeCloseEnough("path.pathSegList.getItem(1).x", "30");
    shouldBeCloseEnough("path.pathSegList.getItem(1).y", "0");
    shouldBe("path.pathSegList.getItem(2).pathSegTypeAsLetter", "'V'");
    shouldBeCloseEnough("path.pathSegList.getItem(2).y", "30");
    shouldBe("path.pathSegList.getItem(3).pathSegTypeAsLetter", "'H'");
    shouldBeCloseEnough("path.pathSegList.getItem(3).x", "0");
}

function executeTest() {
    const expectedValues = [
        // [animationId, time, sampleCallback]
        ["animation", 0.0,   sample1],
        ["animation", 1.0,   sample2],
        ["animation", 3.0,   sample3],
        ["animation", 3.999, sample4],
        ["animation", 4.0,   sample1]
    ];

    runAnimationTest(expectedValues);
}

var successfullyParsed = true;
