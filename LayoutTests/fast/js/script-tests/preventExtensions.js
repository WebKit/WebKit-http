description(
"This test checks whether various seal/freeze/preventExtentions work on a regular object."
);

function obj()
{
    // Add an accessor property to check 'isFrozen' returns the correct result for objects with accessors.
    return Object.defineProperty({ a: 1, b: 2 }, 'g', { get: function() { return "getter"; } });
}

function test(obj)
{
    obj.c =3;
    obj.b =4;
    delete obj.a;

    var result = "";
    for (key in obj)
        result += ("(" + key + ":" + obj[key] + ")");
    if (Object.isSealed(obj))
        result += "S";
    if (Object.isFrozen(obj))
        result += "F";
    if (Object.isExtensible(obj))
        result += "E";
    return result;
}

function seal(obj)
{
    Object.seal(obj);
    return obj;
}

function freeze(obj)
{
    Object.freeze(obj);
    return obj;
}

function preventExtensions(obj)
{
    Object.preventExtensions(obj);
    return obj;
}

function inextensible(){}
function sealed(){}
function frozen(){};
preventExtensions(inextensible);
seal(sealed);
freeze(frozen);
new inextensible;
new sealed;
new frozen;
inextensible.prototype.prototypeExists = true;
sealed.prototype.prototypeExists = true;
frozen.prototype.prototypeExists = true;

shouldBeTrue("(new inextensible).prototypeExists");
shouldBeTrue("(new sealed).prototypeExists");
shouldBeTrue("(new frozen).prototypeExists");

shouldBe('test(obj())', '"(b:4)(c:3)E"'); // extensible, can delete a, can modify b, and can add c
shouldBe('test(preventExtensions(obj()))', '"(b:4)"'); // <nothing>, can delete a, can modify b, and CANNOT add c
shouldBe('test(seal(obj()))', '"(a:1)(b:4)S"'); // sealed, CANNOT delete a, can modify b, and CANNOT add c
shouldBe('test(freeze(obj()))', '"(a:1)(b:2)SF"'); // sealed and frozen, CANNOT delete a, CANNOT modify b, and CANNOT add c

// check that we can preventExtensions on a host function.
shouldBe('Object.preventExtensions(Math.sin)', 'Math.sin');

shouldThrow('var o = {}; Object.preventExtensions(o); o.__proto__ = { newProp: "Should not see this" }; o.newProp;');
shouldThrow('"use strict"; var o = {}; Object.preventExtensions(o); o.__proto__ = { newProp: "Should not see this" }; o.newProp;');

// check that we can still access static properties on an object after calling preventExtensions.
shouldBe('Object.preventExtensions(Math); Math.sqrt(4)', '2');

// Should not be able to add properties to a preventExtensions array.
shouldBeUndefined('var arr = Object.preventExtensions([]); arr[0] = 42; arr[0]');
shouldBe('var arr = Object.preventExtensions([]); arr[0] = 42; arr.length', '0');

// A read-only property on the prototype should prevent a [[Put]] .
function Constructor() {}
Constructor.prototype.foo = 1;
Object.freeze(Constructor.prototype);
var obj = new Constructor();
obj.foo = 2;
shouldBe('obj.foo', '1');

// Check that freezing array objects works correctly.
var array = freeze([0,1,2]);
array[0] = 3;
shouldBe('array[0]', '0');
shouldBeFalse('Object.getOwnPropertyDescriptor(array, "length").writable')

// Check that freezing arguments objects works correctly.
var args = freeze((function(){ return arguments; })(0,1,2));
args[0] = 3;
shouldBe('args[0]', '0');
shouldBeFalse('Object.getOwnPropertyDescriptor(args, "length").writable')
shouldBeFalse('Object.getOwnPropertyDescriptor(args, "callee").writable')
