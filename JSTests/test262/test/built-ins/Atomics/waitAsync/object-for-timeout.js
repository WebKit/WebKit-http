// Copyright (C) 2020 Rick Waldron. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-atomics.waitasync
description: >
  Object valueOf, toString, toPrimitive Zero timeout arg should result in an +0 timeout
info: |
  Atomics.waitAsync( typedArray, index, value, timeout )

  1. Return DoWait(async, typedArray, index, value, timeout).

  DoWait ( mode, typedArray, index, value, timeout )

  6. Let q be ? ToNumber(timeout).

    Object -> Apply the following steps:

      Let primValue be ? ToPrimitive(argument, hint Number).
      Return ? ToNumber(primValue).

features: [Atomics.waitAsync, SharedArrayBuffer, Symbol, Symbol.toPrimitive, TypedArray, computed-property-names, Atomics]
flags: [async]
---*/
const i32a = new Int32Array(
  new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * 4)
);

const valueOf = {
  valueOf() {
    return 0;
  }
};

const toString = {
  toString() {
    return "0";
  }
};

const toPrimitive = {
  [Symbol.toPrimitive]() {
    return 0;
  }
};

assert.sameValue(
  Atomics.waitAsync(i32a, 0, 0, valueOf).value,
  'timed-out',
  'Atomics.waitAsync(i32a, 0, 0, valueOf).value resolves to "timed-out"'
);
assert.sameValue(
  Atomics.waitAsync(i32a, 0, 0, toString).value,
  'timed-out',
  'Atomics.waitAsync(i32a, 0, 0, toString).value resolves to "timed-out"'
);
assert.sameValue(
  Atomics.waitAsync(i32a, 0, 0, toPrimitive).value,
  'timed-out',
  'Atomics.waitAsync(i32a, 0, 0, toPrimitive).value resolves to "timed-out"'
);

Promise.all([
    Atomics.waitAsync(i32a, 0, 0, valueOf).value,
    Atomics.waitAsync(i32a, 0, 0, toString).value,
    Atomics.waitAsync(i32a, 0, 0, toPrimitive).value,
  ]).then(outcomes => {
    assert.sameValue(outcomes[0], "timed-out");
    assert.sameValue(outcomes[1], "timed-out");
    assert.sameValue(outcomes[2], "timed-out");
  }, $DONE).then($DONE, $DONE);
