/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

(function () {

module("base");

test("joinPath", 1, function() {
    var value = base.joinPath("path/to", "test.html");
    equals(value, "path/to/test.html");
});

test("endsWith", 9, function() {
    ok(base.endsWith("xyz", ""));
    ok(base.endsWith("xyz", "z"));
    ok(base.endsWith("xyz", "yz"));
    ok(base.endsWith("xyz", "xyz"));
    ok(!base.endsWith("xyz", "wxyz"));
    ok(!base.endsWith("xyz", "gwxyz"));
    ok(base.endsWith("", ""));
    ok(!base.endsWith("", "z"));
    ok(!base.endsWith("xyxy", "yx"));
});

test("trimExtension", 6, function() {
    equals(base.trimExtension("xyz"), "xyz");
    equals(base.trimExtension("xy.z"), "xy");
    equals(base.trimExtension("x.yz"), "x");
    equals(base.trimExtension("x.y.z"), "x.y");
    equals(base.trimExtension(".xyz"), "");
    equals(base.trimExtension(""), "");
});

test("joinPath with empty parent", 1, function() {
    var value = base.joinPath("", "test.html");
    equals(value, "test.html");
});

test("uniquifyArray", 5, function() {
    deepEqual(base.uniquifyArray([]), []);
    deepEqual(base.uniquifyArray(["a"]), ["a"]);
    deepEqual(base.uniquifyArray(["a", "b"]), ["a", "b"]);
    deepEqual(base.uniquifyArray(["a", "b", "b"]), ["a", "b"]);
    deepEqual(base.uniquifyArray(["a", "b", "b", "a"]), ["a", "b"]);
});

test("keys", 4, function() {
    deepEqual(base.keys({}), []);
    deepEqual(base.keys({"a": 1}), ["a"]);
    deepEqual(base.keys({"a": 1, "b": 0}), ["a", "b"]);
    deepEqual(base.keys({"a": 1, "b": { "c" : 1}}), ["a", "b"]);
});

test("callInParallel", 4, function() {
    var expectedCall = [true, true, true];
    var expectCompletionCallback = true;

    base.callInParallel([
        function(callback) {
            ok(expectedCall[0]);
            expectedCall[0] = false;
            callback();
        },
        function(callback) {
            ok(expectedCall[1]);
            expectedCall[1] = false;
            callback();
        },
        function(callback) {
            ok(expectedCall[2]);
            expectedCall[2] = false;
            callback();
        },
    ], function() {
        ok(expectCompletionCallback);
        expectCompletionCallback = false;
    })
});

test("callInSequence", 7, function() {
    var expectedArg = 42;
    var expectCompletionCallback = true;

    base.callInSequence(function(arg, callback) {
        ok(arg < 45);
        equals(arg, expectedArg++);
        callback();
    }, [42, 43, 44], function() {
        ok(expectCompletionCallback);
        expectCompletionCallback = false;
    })
});

test("RequestTracker", 3, function() {
    var ready = false;
    var tracker = new base.RequestTracker(1, function() {
        ok(ready);
    });
    ready = true;
    tracker.requestComplete();
    ready = false;

    tracker = new base.RequestTracker(2, function(parameter) {
        ok(ready);
        equals(parameter, 'argument');
    }, ['argument']);
    tracker.requestComplete();
    ready = true;
    tracker.requestComplete();
    ready = false;

    tracker = new base.RequestTracker(0, function() {
        ok(false);
    });
    tracker.requestComplete();
});

test("CallbackIterator", 22, function() {
    var expected = 0;
    var iterator = new base.CallbackIterator(function(a, b) {
        equals(a, 'ArgA' + expected);
        equals(b, 'ArgB' + expected);
        ++expected;
    }, [
        ['ArgA0', 'ArgB0'],
        ['ArgA1', 'ArgB1'],
        ['ArgA2', 'ArgB2'],
    ]);
    ok(iterator.hasNext())
    ok(!iterator.hasPrevious())
    iterator.callNext();
    ok(iterator.hasNext())
    ok(!iterator.hasPrevious())
    iterator.callNext();
    ok(iterator.hasNext())
    ok(iterator.hasPrevious())
    iterator.callNext();
    ok(!iterator.hasNext())
    ok(iterator.hasPrevious())
    expected = 1;
    iterator.callPrevious();
    ok(iterator.hasNext())
    ok(iterator.hasPrevious())
    expected = 0;
    iterator.callPrevious();
    ok(iterator.hasNext())
    ok(!iterator.hasPrevious())
});

test("filterTree", 2, function() {
    var tree = {
        'path': {
            'to': {
                'test.html': {
                    'actual': 'PASS',
                    'expected': 'FAIL'
                }
            },
            'another.html': {
                'actual': 'TEXT',
                'expected': 'PASS'
            }
        }
    }

    function isLeaf(node)
    {
        return !!node.actual;
    }

    function actualIsText(node)
    {
        return node.actual == 'TEXT';
    }

    var all = base.filterTree(tree, isLeaf, function() { return true });
    deepEqual(all, {
        'path/to/test.html': {
            'actual': 'PASS',
            'expected': 'FAIL'
        },
        'path/another.html': {
            'actual': 'TEXT',
            'expected': 'PASS'
        }
    });

    var text = base.filterTree(tree, isLeaf, actualIsText);
    deepEqual(text, {
        'path/another.html': {
            'actual': 'TEXT',
            'expected': 'PASS'
        }
    });
});

test("extends", 17, function() {

    var LikeDiv = base.extends("div", {
        init: function() {
            this.textContent = "awesome";
        },
        method: function(msg) {
            return 42;
        }
    });

    var LikeLikeDiv = base.extends(LikeDiv, {
        init: function() {
            this.className = "like";
        }
    });

    var LikeP = base.extends("p", {
        init: function(content) {
            this.textContent = content
        }
    });

    var LikeProgress = base.extends("progress", {
        init: function() {
            this.max = 100;
            this.value = 10;
        }
    });

    var LikeLikeProgress = base.extends(LikeProgress, {
        completed: function() {
            this.value = 100;
        }
    });

    document.body.appendChild(new LikeDiv());
    equals(document.body.lastChild.tagName, "DIV");
    equals(document.body.lastChild.innerHTML, "awesome");
    equals(document.body.lastChild.method(), 42);
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeLikeDiv());
    equals(document.body.lastChild.tagName, "DIV");
    equals(document.body.lastChild.innerHTML, "awesome");
    equals(document.body.lastChild.method(), 42);
    equals(document.body.lastChild.className, "like");
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeP("super"));
    equals(document.body.lastChild.tagName, "P");
    equals(document.body.lastChild.innerHTML, "super");
    raises(function() {
        document.body.lastChild.method();
    });
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeProgress());
    equals(document.body.lastChild.tagName, "PROGRESS");
    equals(document.body.lastChild.position, 0.1);
    equals(document.body.lastChild.innerHTML, "");
    raises(function() {
        document.body.lastChild.method();
    });
    document.body.removeChild(document.body.lastChild);

    document.body.appendChild(new LikeLikeProgress());
    equals(document.body.lastChild.tagName, "PROGRESS");
    equals(document.body.lastChild.position, 0.1);
    document.body.lastChild.completed();
    equals(document.body.lastChild.position, 1);
    document.body.removeChild(document.body.lastChild);
});

})();
