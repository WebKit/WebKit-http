description(
"This test thoroughly checks the behaviour of the 'arguments' object."
);

function access_1(a, b, c)
{
    return arguments[0];
}

function access_2(a, b, c)
{
    return arguments[1];
}

function access_3(a, b, c)
{
    return arguments[2];
}

function access_4(a, b, c)
{
    return arguments[3];
}

function access_5(a, b, c)
{
    return arguments[4];
}

shouldBe("access_1(1, 2, 3)", "1");
shouldBe("access_2(1, 2, 3)", "2");
shouldBe("access_3(1, 2, 3)", "3");
shouldBe("access_4(1, 2, 3)", "undefined");
shouldBe("access_5(1, 2, 3)", "undefined");

shouldBe("access_1(1)", "1");
shouldBe("access_2(1)", "undefined");
shouldBe("access_3(1)", "undefined");
shouldBe("access_4(1)", "undefined");
shouldBe("access_5(1)", "undefined");

shouldBe("access_1(1, 2, 3, 4, 5)", "1");
shouldBe("access_2(1, 2, 3, 4, 5)", "2");
shouldBe("access_3(1, 2, 3, 4, 5)", "3");
shouldBe("access_4(1, 2, 3, 4, 5)", "4");
shouldBe("access_5(1, 2, 3, 4, 5)", "5");

function f(a, b, c)
{
    return arguments;
}

function tear_off_equal_access_1(a, b, c)
{
    return f(a, b, c)[0];
}

function tear_off_equal_access_2(a, b, c)
{
    return f(a, b, c)[1];
}

function tear_off_equal_access_3(a, b, c)
{
    return f(a, b, c)[2];
}

function tear_off_equal_access_4(a, b, c)
{
    return f(a, b, c)[3];
}

function tear_off_equal_access_5(a, b, c)
{
    return f(a, b, c)[4];
}

shouldBe("tear_off_equal_access_1(1, 2, 3)", "1");
shouldBe("tear_off_equal_access_2(1, 2, 3)", "2");
shouldBe("tear_off_equal_access_3(1, 2, 3)", "3");
shouldBe("tear_off_equal_access_4(1, 2, 3)", "undefined");
shouldBe("tear_off_equal_access_5(1, 2, 3)", "undefined");

function tear_off_too_few_access_1(a)
{
    return f(a)[0];
}

function tear_off_too_few_access_2(a)
{
    return f(a)[1];
}

function tear_off_too_few_access_3(a)
{
    return f(a)[2];
}

function tear_off_too_few_access_4(a)
{
    return f(a)[3];
}

function tear_off_too_few_access_5(a)
{
    return f(a)[4];
}

shouldBe("tear_off_too_few_access_1(1)", "1");
shouldBe("tear_off_too_few_access_2(1)", "undefined");
shouldBe("tear_off_too_few_access_3(1)", "undefined");
shouldBe("tear_off_too_few_access_4(1)", "undefined");
shouldBe("tear_off_too_few_access_5(1)", "undefined");

function tear_off_too_many_access_1(a, b, c, d, e)
{
    return f(a, b, c, d, e)[0];
}

function tear_off_too_many_access_2(a, b, c, d, e)
{
    return f(a, b, c, d, e)[1];
}

function tear_off_too_many_access_3(a, b, c, d, e)
{
    return f(a, b, c, d, e)[2];
}

function tear_off_too_many_access_4(a, b, c, d, e)
{
    return f(a, b, c, d, e)[3];
}

function tear_off_too_many_access_5(a, b, c, d, e)
{
    return f(a, b, c, d, e)[4];
}

shouldBe("tear_off_too_many_access_1(1, 2, 3, 4, 5)", "1");
shouldBe("tear_off_too_many_access_2(1, 2, 3, 4, 5)", "2");
shouldBe("tear_off_too_many_access_3(1, 2, 3, 4, 5)", "3");
shouldBe("tear_off_too_many_access_4(1, 2, 3, 4, 5)", "4");
shouldBe("tear_off_too_many_access_5(1, 2, 3, 4, 5)", "5");

function live_1(a, b, c)
{
    arguments[0] = 1;
    return a;
}

function live_2(a, b, c)
{
    arguments[1] = 2;
    return b;
}

function live_3(a, b, c)
{
    arguments[2] = 3;
    return c;
}

shouldBe("live_1(0, 2, 3)", "1");
shouldBe("live_2(1, 0, 3)", "2");
shouldBe("live_3(1, 2, 0)", "3");

// Arguments that were not provided are not live
shouldBe("live_1(0)", "1");
shouldBe("live_2(1)", "undefined");
shouldBe("live_3(1)", "undefined");

shouldBe("live_1(0, 2, 3, 4, 5)", "1");
shouldBe("live_2(1, 0, 3, 4, 5)", "2");
shouldBe("live_3(1, 2, 0, 4, 5)", "3");

function extra_args_modify_4(a, b, c)
{
    arguments[3] = 4;
    return arguments[3];
}

function extra_args_modify_5(a, b, c)
{
    arguments[4] = 5;
    return arguments[4];
}

shouldBe("extra_args_modify_4(1, 2, 3, 0, 5)", "4");
shouldBe("extra_args_modify_5(1, 2, 3, 4, 0)", "5");

function tear_off_live_1(a, b, c)
{
    var args = arguments;
    return function()
    {
        args[0] = 1;
        return a;
    };
}

function tear_off_live_2(a, b, c)
{
    var args = arguments;
    return function()
    {
        args[1] = 2;
        return b;
    };
}

function tear_off_live_3(a, b, c)
{
    var args = arguments;
    return function()
    {
        args[2] = 3;
        return c;
    };
}

shouldBe("tear_off_live_1(0, 2, 3)()", "1");
shouldBe("tear_off_live_2(1, 0, 3)()", "2");
shouldBe("tear_off_live_3(1, 2, 0)()", "3");

// Arguments that were not provided are not live
shouldBe("tear_off_live_1(0)()", "1");
shouldBe("tear_off_live_2(1)()", "undefined");
shouldBe("tear_off_live_3(1)()", "undefined");

shouldBe("tear_off_live_1(0, 2, 3, 4, 5)()", "1");
shouldBe("tear_off_live_2(1, 0, 3, 4, 5)()", "2");
shouldBe("tear_off_live_3(1, 2, 0, 4, 5)()", "3");

function tear_off_extra_args_modify_4(a, b, c)
{
    return function()
    {
        arguments[3] = 4;
        return arguments[3];
    }
}

function tear_off_extra_args_modify_5(a, b, c)
{
    return function()
    {
        arguments[4] = 5;
        return arguments[4];
    }
}

shouldBe("tear_off_extra_args_modify_4(1, 2, 3, 0, 5)()", "4");
shouldBe("tear_off_extra_args_modify_5(1, 2, 3, 4, 0)()", "5");

function delete_1(a, b, c)
{
    delete arguments[0];
    return arguments[0];
}

function delete_2(a, b, c)
{
    delete arguments[1];
    return arguments[1];
}

function delete_3(a, b, c)
{
    delete arguments[2];
    return arguments[2];
}

function delete_4(a, b, c)
{
    delete arguments[3];
    return arguments[3];
}

function delete_5(a, b, c)
{
    delete arguments[4];
    return arguments[4];
}

shouldBe("delete_1(1, 2, 3)", "undefined");
shouldBe("delete_2(1, 2, 3)", "undefined");
shouldBe("delete_3(1, 2, 3)", "undefined");
shouldBe("delete_4(1, 2, 3)", "undefined");
shouldBe("delete_5(1, 2, 3)", "undefined");

shouldBe("delete_1(1)", "undefined");
shouldBe("delete_2(1)", "undefined");
shouldBe("delete_3(1)", "undefined");
shouldBe("delete_4(1)", "undefined");
shouldBe("delete_5(1)", "undefined");

shouldBe("delete_1(1, 2, 3, 4, 5)", "undefined");
shouldBe("delete_2(1, 2, 3, 4, 5)", "undefined");
shouldBe("delete_3(1, 2, 3, 4, 5)", "undefined");
shouldBe("delete_4(1, 2, 3, 4, 5)", "undefined");
shouldBe("delete_5(1, 2, 3, 4, 5)", "undefined");

function tear_off_delete_1(a, b, c)
{
    return function()
    {
        delete arguments[0];
        return arguments[0];
    };
}

function tear_off_delete_2(a, b, c)
{
    return function()
    {
        delete arguments[1];
        return arguments[1];
    };
}

function tear_off_delete_3(a, b, c)
{
    return function()
    {
        delete arguments[2];
        return arguments[2];
    };
}

function tear_off_delete_4(a, b, c)
{
    return function()
    {
        delete arguments[3];
        return arguments[3];
    };
}

function tear_off_delete_5(a, b, c)
{
    return function()
    {
        delete arguments[4];
        return arguments[4];
    };
}

shouldBe("tear_off_delete_1(1, 2, 3)()", "undefined");
shouldBe("tear_off_delete_2(1, 2, 3)()", "undefined");
shouldBe("tear_off_delete_3(1, 2, 3)()", "undefined");
shouldBe("tear_off_delete_4(1, 2, 3)()", "undefined");
shouldBe("tear_off_delete_5(1, 2, 3)()", "undefined");

shouldBe("tear_off_delete_1(1)()", "undefined");
shouldBe("tear_off_delete_2(1)()", "undefined");
shouldBe("tear_off_delete_3(1)()", "undefined");
shouldBe("tear_off_delete_4(1)()", "undefined");
shouldBe("tear_off_delete_5(1)()", "undefined");

shouldBe("tear_off_delete_1(1, 2, 3, 4, 5)()", "undefined");
shouldBe("tear_off_delete_2(1, 2, 3, 4, 5)()", "undefined");
shouldBe("tear_off_delete_3(1, 2, 3, 4, 5)()", "undefined");
shouldBe("tear_off_delete_4(1, 2, 3, 4, 5)()", "undefined");
shouldBe("tear_off_delete_5(1, 2, 3, 4, 5)()", "undefined");

function delete_not_live_1(a, b, c)
{
    delete arguments[0];
    return a;
}

function delete_not_live_2(a, b, c)
{
    delete arguments[1];
    return b;
}

function delete_not_live_3(a, b, c)
{
    delete arguments[2];
    return c;
}

shouldBe("delete_not_live_1(1, 2, 3)", "1");
shouldBe("delete_not_live_2(1, 2, 3)", "2");
shouldBe("delete_not_live_3(1, 2, 3)", "3");

shouldBe("delete_not_live_1(1)", "1");
shouldBe("delete_not_live_2(1)", "undefined");
shouldBe("delete_not_live_3(1)", "undefined");

shouldBe("delete_not_live_1(1, 2, 3, 4, 5)", "1");
shouldBe("delete_not_live_2(1, 2, 3, 4, 5)", "2");
shouldBe("delete_not_live_3(1, 2, 3, 4, 5)", "3");

function tear_off_delete_not_live_1(a, b, c)
{
    return function()
    {
        delete arguments[0];
        return a;
    };
}

function tear_off_delete_not_live_2(a, b, c)
{
    return function ()
    {
        delete arguments[1];
        return b;
    };
}

function tear_off_delete_not_live_3(a, b, c)
{
    return function()
    {
        delete arguments[2];
        return c;
    };
}

shouldBe("tear_off_delete_not_live_1(1, 2, 3)()", "1");
shouldBe("tear_off_delete_not_live_2(1, 2, 3)()", "2");
shouldBe("tear_off_delete_not_live_3(1, 2, 3)()", "3");

shouldBe("tear_off_delete_not_live_1(1)()", "1");
shouldBe("tear_off_delete_not_live_2(1)()", "undefined");
shouldBe("tear_off_delete_not_live_3(1)()", "undefined");

shouldBe("tear_off_delete_not_live_1(1, 2, 3, 4, 5)()", "1");
shouldBe("tear_off_delete_not_live_2(1, 2, 3, 4, 5)()", "2");
shouldBe("tear_off_delete_not_live_3(1, 2, 3, 4, 5)()", "3");

function access_after_delete_named_2(a, b, c)
{
    delete arguments[0];
    return b;
}

function access_after_delete_named_3(a, b, c)
{
    delete arguments[0];
    return c;
}

function access_after_delete_named_4(a, b, c)
{
    delete arguments[0];
    return arguments[3];
}

shouldBe("access_after_delete_named_2(1, 2, 3)", "2");
shouldBe("access_after_delete_named_3(1, 2, 3)", "3");
shouldBe("access_after_delete_named_4(1, 2, 3)", "undefined");

shouldBe("access_after_delete_named_2(1)", "undefined");
shouldBe("access_after_delete_named_3(1)", "undefined");
shouldBe("access_after_delete_named_4(1)", "undefined");

shouldBe("access_after_delete_named_2(1, 2, 3, 4)", "2");
shouldBe("access_after_delete_named_3(1, 2, 3, 4)", "3");
shouldBe("access_after_delete_named_4(1, 2, 3, 4)", "4");

function access_after_delete_extra_1(a, b, c)
{
    delete arguments[3];
    return a;
}

function access_after_delete_extra_2(a, b, c)
{
    delete arguments[3];
    return b;
}

function access_after_delete_extra_3(a, b, c)
{
    delete arguments[3];
    return c;
}

function access_after_delete_extra_5(a, b, c)
{
    delete arguments[3];
    return arguments[4];
}

shouldBe("access_after_delete_extra_1(1, 2, 3)", "1");
shouldBe("access_after_delete_extra_2(1, 2, 3)", "2");
shouldBe("access_after_delete_extra_3(1, 2, 3)", "3");
shouldBe("access_after_delete_extra_5(1, 2, 3)", "undefined");

shouldBe("access_after_delete_extra_1(1)", "1");
shouldBe("access_after_delete_extra_2(1)", "undefined");
shouldBe("access_after_delete_extra_3(1)", "undefined");
shouldBe("access_after_delete_extra_5(1)", "undefined");

shouldBe("access_after_delete_extra_1(1, 2, 3, 4, 5)", "1");
shouldBe("access_after_delete_extra_2(1, 2, 3, 4, 5)", "2");
shouldBe("access_after_delete_extra_3(1, 2, 3, 4, 5)", "3");
shouldBe("access_after_delete_extra_5(1, 2, 3, 4, 5)", "5");

function argumentsParam(arguments)
{
    return arguments;
}
shouldBeTrue("argumentsParam(true)");

var argumentsFunctionConstructorParam = new Function("arguments", "return arguments;");
shouldBeTrue("argumentsFunctionConstructorParam(true)");

function argumentsVarUndefined()
{
    var arguments;
    return String(arguments);
}
shouldBe("argumentsVarUndefined()", "'[object Arguments]'");

function argumentsConstUndefined()
{
    const arguments;
    return String(arguments);
}
shouldBe("argumentsConstUndefined()", "'[object Arguments]'");

function argumentCalleeInException() {
    try {
        throw "";
    } catch (e) {
        return arguments.callee;
    }
}
shouldBe("argumentCalleeInException()", "argumentCalleeInException")

function shadowedArgumentsApply(arguments) {
    return function(a){ return a; }.apply(null, arguments);
}

function shadowedArgumentsLength(arguments) {
    return arguments.length;
}

function shadowedArgumentsCallee(arguments) {
    return arguments.callee;
}

function shadowedArgumentsIndex(arguments) {
    return arguments[0]
}

shouldBeTrue("shadowedArgumentsApply([true])");
shouldBe("shadowedArgumentsLength([])", '0');
shouldThrow("shadowedArgumentsLength()");
shouldBeUndefined("shadowedArgumentsCallee([])");
shouldBeTrue("shadowedArgumentsIndex([true])");

descriptor = (function(){ return Object.getOwnPropertyDescriptor(arguments, 1); })("zero","one","two");
shouldBe("descriptor.value", '"one"');
shouldBe("descriptor.writable", 'true');
shouldBe("descriptor.enumerable", 'true');
shouldBe("descriptor.configurable", 'true');
