description(
'Test of JavaScript date parsing.'
);

function testDateParseExpr(dateExpr, numericResult)
{
    if (numericResult == "NaN") {
        shouldBeNaN('Date.parse(' + dateExpr + ')');
        shouldBeNaN('Date.parse((' + dateExpr + ').toUpperCase())');
        shouldBeNaN('Date.parse((' + dateExpr + ').toLowerCase())');
    } else {
        shouldBeTrue('Date.parse(' + dateExpr + ') == ' + numericResult);
        shouldBeTrue('Date.parse((' + dateExpr + ').toUpperCase()) == ' + numericResult);
        shouldBeTrue('Date.parse((' + dateExpr + ').toLowerCase()) == ' + numericResult);
    }
}

function testDateParse(date, numericResult)
{
    if (numericResult == "NaN") {
        shouldBeNaN('Date.parse("' + date + '")');
        shouldBeNaN('Date.parse("' + date.toUpperCase() + '")');
        shouldBeNaN('Date.parse("' + date.toLowerCase() + '")');
    } else {
        shouldBeTrue('Date.parse("' + date + '") == ' + numericResult.toString());
        shouldBeTrue('Date.parse("' + date.toUpperCase() + '") == ' + numericResult);
        shouldBeTrue('Date.parse("' + date.toLowerCase() + '") == ' + numericResult);
    }
}

function testDateParseExact(date, numericResult)
{
    if (numericResult == "NaN") {
        shouldBeNaN('Date.parse("' + date + '")');
    } else {
        shouldBeTrue('Date.parse("' + date + '") == ' + numericResult.toString());
    }
}


// test ECMAScript 5 standard date parsing
testDateParseExact("1995-12-25T01:30:00Z", "819855000000");
testDateParseExact("1995-12-25T01:30:00.5Z", "819855000500");
testDateParseExact("1995-12-25T01:30:00.009Z", "819855000009");
testDateParseExact("1995-12-25T01:30:00+00:00", "819855000000");
testDateParseExact("1995-12-25T01:30:00.0+00:01", "819854940000");
testDateParseExact("1995-12-25T01:30:00.0-00:01", "819855060000");
testDateParseExact("1995-12-25T01:30:00.0+01:01", "819851340000");
testDateParseExact("1995-12-25T01:30:00.0-01:01", "819858660000");

testDateParseExact("0000-01-01T00:00:00Z", "-62167219200000");
testDateParseExact("+99999-12-31T24:00:00Z", "3093527980800000");
testDateParseExact("-99999-01-01T00:00:00Z", "-3217830796800000");
testDateParseExact("1995-12-31T23:59:60Z", "820454400000");
testDateParseExact("1995-12-31T23:59:60.5Z", "820454400000");

testDateParseExact("1995-13-25T01:30:00Z", "NaN");
testDateParseExact("1995-00-25T01:30:00Z", "NaN");
testDateParseExact("1995--1-25T01:30:00Z", "NaN");
testDateParseExact("1995-01-25T01:05:-0.3Z", "NaN");
testDateParseExact("1995/12/25T01:30:00Z", "NaN");
testDateParseExact("1995-12-25T1:30:00Z", "NaN");
testDateParseExact("1995-12-25T01:30:00.Z", "NaN");
testDateParseExact("1995-12-25T01:30:00.+1Z", "NaN");
testDateParseExact("1995-12-25T01:30:00Z ", "NaN");
testDateParseExact("1995-12-25T01:30:00+00:00 ", "NaN");
testDateParseExact("1995-02-29T00:00:00Z", "NaN");
testDateParseExact("1995-12-25 01:30:00Z", "NaN");
testDateParseExact("1995-12-25T01:30:00z", "NaN");


// test old implementation fallback

var timeZoneOffset = Date.parse(" Dec 25 1995 1:30 ") - Date.parse(" Dec 25 1995 1:30 GMT ");

testDateParse("Dec 25 1995 GMT", "819849600000");
testDateParse("Dec 25 1995", "819849600000 + timeZoneOffset");

testDateParse("Dec 25 1995 1:30 GMT", "819855000000");
testDateParse("Dec 25 1995 1:30", "819855000000 + timeZoneOffset");
testDateParse("Dec 25 1995 1:30 ", "819855000000 + timeZoneOffset");
testDateParse("Dec 25 1995 1:30 AM GMT", "819855000000");
testDateParse("Dec 25 1995 1:30 AM", "819855000000 + timeZoneOffset");
testDateParse("Dec 25 1995 1:30AM", "NaN");
testDateParse("Dec 25 1995 1:30 AM ", "819855000000 + timeZoneOffset");

testDateParse("Dec 25 1995 13:30 GMT", "819898200000");
testDateParse("Dec 25 1995 13:30", "819898200000 + timeZoneOffset");
testDateParse("Dec 25 13:30 1995", "819898200000 + timeZoneOffset");
testDateParse("Dec 25 1995 13:30 ", "819898200000 + timeZoneOffset");
testDateParse("Dec 25 1995 1:30 PM GMT", "819898200000");
testDateParse("Dec 25 1995 1:30 PM", "819898200000 + timeZoneOffset");
testDateParse("Dec 25 1995 1:30PM", "NaN");
testDateParse("Dec 25 1995 1:30 PM ", "819898200000 + timeZoneOffset");

testDateParse("Dec 25 1995 UTC", "819849600000");
testDateParse("Dec 25 1995 UT", "819849600000");
testDateParse("Dec 25 1995 PST", "819878400000");
testDateParse("Dec 25 1995 PDT", "819874800000");

testDateParse("Dec 25 1995 1:30 UTC", "819855000000");
testDateParse("Dec 25 1995 1:30 UT", "819855000000");
testDateParse("Dec 25 1995 1:30 PST", "819883800000");
testDateParse("Dec 25 1995 1:30 PDT", "819880200000");

testDateParse("Dec 25 1995 1:30 PM UTC", "819898200000");
testDateParse("Dec 25 1995 1:30 PM UT", "819898200000");
testDateParse("Dec 25 1995 1:30 PM PST", "819927000000");
testDateParse("Dec 25 1995 1:30 PM PDT", "819923400000");

testDateParse("Dec 25 1995 XXX", "NaN");
testDateParse("Dec 25 1995 1:30 XXX", "NaN");

testDateParse("Dec 25 1995 1:30 U", "NaN");
testDateParse("Dec 25 1995 1:30 V", "NaN");
testDateParse("Dec 25 1995 1:30 W", "NaN");
testDateParse("Dec 25 1995 1:30 X", "NaN");

testDateParse("Dec 25 1995 0:30 GMT", "819851400000");
testDateParse("Dec 25 1995 0:30 AM GMT", "819851400000");
testDateParse("Dec 25 1995 0:30 PM GMT", "819894600000");
testDateParse("Dec 25 1995 12:30 AM GMT", "819851400000");
testDateParse("Dec 25 1995 12:30 PM GMT", "819894600000");
testDateParse("Dec 25 1995 13:30 AM GMT", "NaN");
testDateParse("Dec 25 1995 13:30 PM GMT", "NaN");

testDateParse("Anf 25 1995 GMT", "NaN");

testDateParse("Wed Dec 25 1995 1:30 GMT", "819855000000");

// RFC 2822
testDateParse("Wed Dec 25 1995 01:30 +0000", "819855000000");
testDateParse("Dec 25 1995 1:30 AM -0000", "819855000000");
testDateParse("Wed Dec 25 1995 13:30 -0800", "819927000000");
testDateParse("Dec 25 1995 01:30 +1700", "819793800000");
testDateParse("Wed Dec 25 1:30 PM -0800 1995", "819927000000");
testDateParseExact("Wed Dec 25 1995 01:30 &1700", "NaN");
testDateParseExact("Wed Dec 25 1995 &1700 01:30", "NaN");


testDateParseExpr('"Dec 25" + String.fromCharCode(9) + "1995 13:30 GMT"', "819898200000");
testDateParseExpr('"Dec 25" + String.fromCharCode(10) + "1995 13:30 GMT"', "819898200000");

// Firefox compatibility
testDateParse("Dec 25, 1995 13:30", "819898200000 + timeZoneOffset");
testDateParse("Dec 25,1995 13:30", "819898200000 + timeZoneOffset");

testDateParse("Dec 25 1995, 13:30", "819898200000 + timeZoneOffset");
testDateParse("Dec 25 1995,13:30", "819898200000 + timeZoneOffset");

testDateParse("Dec 25, 1995, 13:30", "819898200000 + timeZoneOffset");
testDateParse("Dec 25,1995,13:30", "819898200000 + timeZoneOffset");

testDateParse("Mon Jun 20 11:00:00 CDT 2011", "1308585600000");

var successfullyParsed = true;
