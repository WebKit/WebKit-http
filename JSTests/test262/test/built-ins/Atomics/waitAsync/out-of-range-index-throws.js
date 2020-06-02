// Copyright (C) 2020 Rick Waldron. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-atomics.waitasync
description: >
  Throws a RangeError if value of index arg is out of range
info: |
  Atomics.waitAsync( typedArray, index, value, timeout )

  1. Return DoWait(async, typedArray, index, value, timeout).

  DoWait ( mode, typedArray, index, value, timeout )

  2. Let i be ? ValidateAtomicAccess(typedArray, index).

    ...
    2.Let accessIndex be ? ToIndex(requestIndex).
    ...
    5. If accessIndex ≥ length, throw a RangeError exception.
features: [Atomics.waitAsync, SharedArrayBuffer, TypedArray, Atomics]
---*/
const i32a = new Int32Array(
  new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * 4)
);

const poisoned = {
  valueOf() {
    throw new Test262Error('should not evaluate this code');
  }
};

assert.throws(RangeError, function() {
  Atomics.wait(i32a, Infinity, poisoned, poisoned);
}, '`Atomics.wait(i32a, Infinity, poisoned, poisoned)` throws RangeError');
assert.throws(RangeError, function() {
  Atomics.wait(i32a, -1, poisoned, poisoned);
}, '`Atomics.wait(i32a, -1, poisoned, poisoned)` throws RangeError');
assert.throws(RangeError, function() {
  Atomics.wait(i32a, 4, poisoned, poisoned);
}, '`Atomics.wait(i32a, 4, poisoned, poisoned)` throws RangeError');
assert.throws(RangeError, function() {
  Atomics.wait(i32a, 200, poisoned, poisoned);
}, '`Atomics.wait(i32a, 200, poisoned, poisoned)` throws RangeError');
