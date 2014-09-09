description("Test the computed style of the -webkit-scroll-snap-* properties.");

var stylesheet;
var styleElement = document.createElement("style");
document.head.appendChild(styleElement);
stylesheet = styleElement.sheet;

function testComputedScrollSnapRule(description, snapProperty, rule, expected)
{
    debug("");
    debug(description + " : " + rule);

    stylesheet.insertRule("body { -webkit-scroll-snap-" + snapProperty + ": " + rule + "; }", 0);

    shouldBe("window.getComputedStyle(document.body).getPropertyValue('-webkit-scroll-snap-" + snapProperty + "')", "'" + expected + "'");
    stylesheet.deleteRule(0);
}

testComputedScrollSnapRule("invalid snapping type", "type", "potato", "none");
testComputedScrollSnapRule("invalid points along x axis", "points-x", "hello world", "repeat(100%)");
testComputedScrollSnapRule("invalid points along y axis", "points-y", "hello world", "repeat(100%)");
testComputedScrollSnapRule("typo in point definition", "points-x", "repaet(50px)", "repeat(100%)");
testComputedScrollSnapRule("another invalid point definition", "points-x", "??px repeat(50px)", "repeat(100%)");
testComputedScrollSnapRule("invalid destination", "destination", "foo bar", "0px 0px");
testComputedScrollSnapRule("short one destination value", "destination", "50%", "0px 0px");
testComputedScrollSnapRule("extra destination value", "destination", "50px 100% 75px", "0px 0px");
testComputedScrollSnapRule("invalid coordinates", "coordinate", "ben bitdiddle", "none")
testComputedScrollSnapRule("mismatched x coordinate", "coordinate", "50% 100px 75%", "none");

testComputedScrollSnapRule("mandatory type", "type", "mandatory", "mandatory");
testComputedScrollSnapRule("proximity type", "type", "proximity", "proximity");

testComputedScrollSnapRule("element points along x axis", "points-x", "elements", "elements");
testComputedScrollSnapRule("percentage points along x axis", "points-x", "100% 50%", "100% 50%");
testComputedScrollSnapRule("pixel points along x axis", "points-x", "100px 50px", "100px 50px");
testComputedScrollSnapRule("percentage points repeat along x axis", "points-x", "repeat(100%)", "repeat(100%)");
testComputedScrollSnapRule("pixel points repeat along x axis", "points-x", "repeat(25px)", "repeat(25px)");
testComputedScrollSnapRule("percentage points along x axis with percentage repeat", "points-x", "100% repeat(100%)", "100% repeat(100%)");
testComputedScrollSnapRule("pixel points along x axis with percentage repeat", "points-x", "100px 50px repeat(25%)", "100px 50px repeat(25%)");
testComputedScrollSnapRule("percentage points along x axis with pixel repeat", "points-x", "100% 50% repeat(40px)", "100% 50% repeat(40px)");
testComputedScrollSnapRule("pixel points along x axis with pixel repeat", "points-x", "100px repeat(42px)", "100px repeat(42px)");

testComputedScrollSnapRule("element points along y axis", "points-y", "elements", "elements");
testComputedScrollSnapRule("percentage points along y axis", "points-y", "100% 50%", "100% 50%");
testComputedScrollSnapRule("pixel points along y axis", "points-y", "100px 50px", "100px 50px");
testComputedScrollSnapRule("percentage points repeat along y axis", "points-y", "repeat(100%)", "repeat(100%)");
testComputedScrollSnapRule("pixel points repeat along y axis", "points-y", "repeat(25px)", "repeat(25px)");
testComputedScrollSnapRule("percentage points along y axis with percentage repeat", "points-y", "100% repeat(100%)", "100% repeat(100%)");
testComputedScrollSnapRule("pixel points along y axis with percentage repeat", "points-y", "100px 50px repeat(25%)", "100px 50px repeat(25%)");
testComputedScrollSnapRule("percentage points along y axis with pixel repeat", "points-y", "100% 50% repeat(40px)", "100% 50% repeat(40px)");
testComputedScrollSnapRule("pixel points along y axis with pixel repeat", "points-y", "100px repeat(42px)", "100px repeat(42px)");

testComputedScrollSnapRule("pixel/pixel destination", "destination", "10px 50px", "10px 50px");
testComputedScrollSnapRule("pixel/percentage destination", "destination", "20px 40%", "20px 40%");
testComputedScrollSnapRule("percentage/pixel destination", "destination", "0% 0px", "0% 0px");
testComputedScrollSnapRule("percentage/percentage destination", "destination", "5% 100%", "5% 100%");

testComputedScrollSnapRule("single pixel coordinate", "coordinate", "50px 100px", "50px 100px");
testComputedScrollSnapRule("single percentage coordinate", "coordinate", "50% 100%", "50% 100%");
testComputedScrollSnapRule("multiple pixel coordinates", "coordinate", "50px 100px 150px 100px 200px 100px", "50px 100px, 150px 100px, 200px 100px");
testComputedScrollSnapRule("multiple percentage coordinates", "coordinate", "50% 100% 150% 100% 200% 100%", "50% 100%, 150% 100%, 200% 100%");

testComputedScrollSnapRule("mm along x axis with pixel repeat", "points-x", "10mm repeat(42mm)", "37.78125px repeat(158.734375px)");
testComputedScrollSnapRule("in along x axis with pixel repeat", "points-x", "10in repeat(4in)", "960px repeat(384px)");
testComputedScrollSnapRule("pt along x axis with pixel repeat", "points-x", "10pt repeat(42pt)", "13.328125px repeat(56px)");
testComputedScrollSnapRule("in/cm destination", "destination", "2in 5cm", "192px 188.96875px");
testComputedScrollSnapRule("in/cm coordinate", "coordinate", "2in 5cm 5in 2cm", "192px 188.96875px, 480px 75.578125px");

testComputedScrollSnapRule("subpixel along x axis with pixel repeat", "points-x", "100.5px repeat(50.25px)", "100.5px repeat(50.25px)");
testComputedScrollSnapRule("subpixel destination", "destination", "0.125px 2.4375px", "0.125px 2.4375px");
testComputedScrollSnapRule("subpixel cordinate", "coordinate", "110.125px 25.4375px", "110.125px 25.4375px");

successfullyParsed = true;
