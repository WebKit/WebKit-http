description("Test to ensure correct behaviour of Object.defineProperty");

shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {}), 'foo'))",
         "JSON.stringify({writable: false, enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {get:undefined}), 'foo'))",
         "JSON.stringify({enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1, writable: false}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1, writable: true}), 'foo'))",
         "JSON.stringify({value: 1, writable: true, enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1, enumerable: false}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1, enumerable: true}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: true, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1, configurable: false}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: false})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty({}, 'foo', {value:1, configurable: true}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: true})");
shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty([1,2,3], 'foo', {value:1, configurable: true}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: true})");
shouldBe("Object.getOwnPropertyDescriptor(Object.defineProperty([1,2,3], '1', {value:'foo', configurable: true}), '1').value", "'foo'");
shouldBe("a=[1,2,3], Object.defineProperty(a, '1', {value:'foo', configurable: true}), a[1]", "'foo'");
shouldBe("Object.getOwnPropertyDescriptor(Object.defineProperty([1,2,3], '1', {get:getter, configurable: true}), '1').get", "getter");
shouldThrow("Object.defineProperty([1,2,3], '1', {get:getter, configurable: true})[1]", "'called getter'");

shouldThrow("Object.defineProperty()");
shouldThrow("Object.defineProperty(null)");
shouldThrow("Object.defineProperty('foo')");
shouldThrow("Object.defineProperty({})");
shouldThrow("Object.defineProperty({}, 'foo')");
shouldThrow("Object.defineProperty({}, 'foo', {get:undefined, value:true}).foo");
shouldBeTrue("Object.defineProperty({get foo() { return true; } }, 'foo', {configurable:false}).foo");

function createUnconfigurableProperty(o, prop, writable, enumerable) {
    writable = writable || false;
    enumerable = enumerable || false;
    return Object.defineProperty(o, prop, {value:1, configurable:false, writable: writable, enumerable: enumerable});
}
shouldThrow("Object.defineProperty(createUnconfigurableProperty({}, 'foo'), 'foo', {configurable: true})");
shouldThrow("Object.defineProperty(createUnconfigurableProperty({}, 'foo'), 'foo', {writable: true})");
shouldThrow("Object.defineProperty(createUnconfigurableProperty({}, 'foo'), 'foo', {enumerable: true})");
shouldThrow("Object.defineProperty(createUnconfigurableProperty({}, 'foo', false, true), 'foo', {enumerable: false}), 'foo'");

shouldBe("JSON.stringify(Object.getOwnPropertyDescriptor(Object.defineProperty(createUnconfigurableProperty({}, 'foo', true), 'foo', {writable: false}), 'foo'))",
         "JSON.stringify({value: 1, writable: false, enumerable: false, configurable: false})");

shouldThrow("Object.defineProperty({}, 'foo', {value:1, get: function(){}})");
shouldThrow("Object.defineProperty({}, 'foo', {value:1, set: function(){}})");
shouldThrow("Object.defineProperty({}, 'foo', {writable:true, get: function(){}})");
shouldThrow("Object.defineProperty({}, 'foo', {writable:true, set: function(){}})");
shouldThrow("Object.defineProperty({}, 'foo', {get: null})");
shouldThrow("Object.defineProperty({}, 'foo', {set: null})");
function getter(){ throw "called getter"; }
function getter1(){ throw "called getter1"; }
function setter(){ throw "called setter"; }
function setter1(){ throw "called setter1"; }
shouldThrow("Object.defineProperty({}, 'foo', {set: setter}).foo='test'", "'called setter'");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter}), 'foo', {set: setter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter}), 'foo', {get: getter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter}), 'foo', {value: 1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter, configurable: true}), 'foo', {set: setter1}).foo='test'", "'called setter1'");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter, configurable: true}), 'foo', {get: getter1}).foo", "'called getter1'");
shouldBeTrue("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter, configurable: true}), 'foo', {value: true}).foo");
shouldBeTrue("'foo' in Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter, configurable: true}), 'foo', {writable: true})");
shouldBeUndefined("Object.defineProperty(Object.defineProperty({}, 'foo', {set: setter, configurable: true}), 'foo', {writable: true}).foo");
shouldThrow("Object.defineProperty({}, 'foo', {get: getter}).foo", "'called getter'");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter}), 'foo', {get: getter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter}), 'foo', {set: setter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter, configurable: true}), 'foo', {get: getter1}).foo", "'called getter1'");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter, configurable: true}), 'foo', {set: setter1}).foo='test'", "'called setter1'");
shouldBeTrue("Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter, configurable: true}), 'foo', {value: true}).foo");
shouldBeUndefined("Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter, configurable: true}), 'foo', {writable: true}).foo");
shouldBeTrue("'foo' in Object.defineProperty(Object.defineProperty({}, 'foo', {get: getter, configurable: true}), 'foo', {writable: true})");

shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1}), 'foo', {set: setter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1, configurable: true}), 'foo', {set: setter1}).foo='test'", "'called setter1'");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1}), 'foo', {get: getter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1}), 'foo', {set: setter1})");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1, configurable: true}), 'foo', {get: getter1}).foo", "'called getter1'");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1, configurable: true}), 'foo', {set: setter1}).foo='test'", "'called setter1'");

// Should be able to redefine an non-writable property, if it is configurable.
shouldBe("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1, configurable: true, writable: false}), 'foo', {value: 2, configurable: true, writable: true}).foo", "2");
shouldBe("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1, configurable: true, writable: false}), 'foo', {value: 2, configurable: true, writable: false}).foo", "2");

// Should be able to redefine an non-writable, non-configurable property, with the same value and attributes.
shouldBe("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1, configurable: false, writable: false}), 'foo', {value: 1, configurable: false, writable: false}).foo", "1");

// Should be able to redefine a configurable accessor, with the same or with different attributes.
// Check the accessor function changed.
shouldBe("Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true, enumerable: true}), 'foo', {get: function() { return 2; }, configurable: true, enumerable: false}).foo", "2");
shouldBe("Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true, enumerable: false}), 'foo', {get: function() { return 2; }, configurable: true, enumerable: false}).foo", "2");
// Check the attributes changed.
shouldBeFalse("var result = false; var o = Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true, enumerable: true}), 'foo', {get: function() { return 2; }, configurable: true, enumerable: false}); for (x in o) result = true; result");
shouldBeTrue("var result = false; var o = Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true, enumerable: false}), 'foo', {get: function() { return 2; }, configurable: true, enumerable: true}); for (x in o) result = true; result");

// Should be able to define an non-configurable accessor.
shouldBeUndefined("var o = Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true}); delete o.foo; o.foo");
shouldBe("var o = Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: false}); delete o.foo; o.foo", '1');
shouldBe("Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true}), 'foo', {value:2}).foo", "2");
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: false}), 'foo', {value:2}).foo");

// Defining a data descriptor over an accessor should result in a read only property.
shouldBe("var o = Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return 1; }, configurable: true}), 'foo', {value:2}); o.foo = 3; o.foo", "2");

// Trying to redefine a non-configurable property as configurable should throw.
shouldThrow("Object.defineProperty(Object.defineProperty({}, 'foo', {value: 1}), 'foo', {value:2, configurable: true})");
shouldThrow("Object.defineProperty(Object.defineProperty([], 'foo', {value: 1}), 'foo', {value:2, configurable: true})");

// Test an object with a getter setter.
// Either accessor may be omitted or replaced with undefined, or both may be replaced with undefined.
shouldBe("var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}); o.foo", '42')
shouldBe("var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}); o.foo = 42; o.result;", '42');
shouldBe("var o = Object.defineProperty({}, 'foo', {get:undefined, set:function(x){this.result = x;}}); o.foo", 'undefined')
shouldBe("var o = Object.defineProperty({}, 'foo', {get:undefined, set:function(x){this.result = x;}}); o.foo = 42; o.result;", '42');
shouldBe("var o = Object.defineProperty({}, 'foo', {set:function(x){this.result = x;}}); o.foo", 'undefined')
shouldBe("var o = Object.defineProperty({}, 'foo', {set:function(x){this.result = x;}}); o.foo = 42; o.result;", '42');
shouldBe("var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:undefined}); o.foo", '42')
shouldBe("var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:undefined}); o.foo = 42; o.result;", 'undefined');
shouldBe("var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}}); o.foo", '42')
shouldBe("var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}}); o.foo = 42; o.result;", 'undefined');
shouldBe("var o = Object.defineProperty({}, 'foo', {get:undefined, set:undefined}); o.foo", 'undefined')
shouldBe("var o = Object.defineProperty({}, 'foo', {get:undefined, set:undefined}); o.foo = 42; o.result;", 'undefined');
// Test replacing or removing either the getter or setter individually.
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:function(){return 13;}}); o.foo", '13')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:function(){return 13;}}); o.foo = 42; o.result;", '42')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:undefined}); o.foo", 'undefined')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:undefined}); o.foo = 42; o.result;", '42')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:function(){this.result = 13;}}); o.foo", '42')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:function(){this.result = 13;}}); o.foo = 42; o.result;", '13')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:undefined}); o.foo", '42')
shouldBe("var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:undefined}); o.foo = 42; o.result;", 'undefined')

// Repeat of the above, in strict mode.
// Either accessor may be omitted or replaced with undefined, or both may be replaced with undefined.
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}); o.foo", '42')
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}); o.foo = 42; o.result;", '42');
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:undefined, set:function(x){this.result = x;}}); o.foo", 'undefined')
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:undefined, set:function(x){this.result = x;}}); o.foo = 42; o.result;", '42');
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {set:function(x){this.result = x;}}); o.foo", 'undefined')
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {set:function(x){this.result = x;}}); o.foo = 42; o.result;", '42');
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:undefined}); o.foo", '42')
shouldThrow("'use strict'; var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}, set:undefined}); o.foo = 42; o.result;");
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}}); o.foo", '42')
shouldThrow("'use strict'; var o = Object.defineProperty({}, 'foo', {get:function(){return 42;}}); o.foo = 42; o.result;");
shouldBe("'use strict'; var o = Object.defineProperty({}, 'foo', {get:undefined, set:undefined}); o.foo", 'undefined')
shouldThrow("'use strict'; var o = Object.defineProperty({}, 'foo', {get:undefined, set:undefined}); o.foo = 42; o.result;");
// Test replacing or removing either the getter or setter individually.
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:function(){return 13;}}); o.foo", '13')
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:function(){return 13;}}); o.foo = 42; o.result;", '42')
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:undefined}); o.foo", 'undefined')
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {get:undefined}); o.foo = 42; o.result;", '42')
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:function(){this.result = 13;}}); o.foo", '42')
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:function(){this.result = 13;}}); o.foo = 42; o.result;", '13')
shouldBe("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:undefined}); o.foo", '42')
shouldThrow("'use strict'; var o = Object.defineProperty(Object.defineProperty({foo:1}, 'foo', {get:function(){return 42;}, set:function(x){this.result = x;}}), 'foo', {set:undefined}); o.foo = 42; o.result;")

Object.defineProperty(Object.prototype, 0, {get:function(){ return false; }, configurable:true})
shouldBeTrue("0 in Object.prototype");
shouldBeTrue("'0' in Object.prototype");
delete Object.prototype[0];

Object.defineProperty(Object.prototype, 'readOnly', {value:true, configurable:true, writable:false})
shouldBeTrue("var o = {}; o.readOnly = false; o.readOnly");
shouldThrow("'use strict'; var o = {}; o.readOnly = false; o.readOnly");
delete Object.prototype.readOnly;

// Check the writable attribute is set correctly when redefining an accessor as a data descriptor.
shouldBeFalse("Object.getOwnPropertyDescriptor(Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return false; }, configurable: true}), 'foo', {value:false}), 'foo').writable");
shouldBeFalse("Object.getOwnPropertyDescriptor(Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return false; }, configurable: true}), 'foo', {value:false, writable: false}), 'foo').writable");
shouldBeTrue("Object.getOwnPropertyDescriptor(Object.defineProperty(Object.defineProperty({}, 'foo', {get: function() { return false; }, configurable: true}), 'foo', {value:false, writable: true}), 'foo').writable");
