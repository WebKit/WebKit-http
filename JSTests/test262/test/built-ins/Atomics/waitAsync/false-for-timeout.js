// Copyright (C) 2020 Rick Waldron. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-atomics.waitasync
description: >
  False timeout arg should result in an +0 timeout
info: |
  Atomics.waitAsync( typedArray, index, value, timeout )

  1. Return DoWait(async, typedArray, index, value, timeout).

  DoWait ( mode, typedArray, index, value, timeout )

  6. Let q be ? ToNumber(timeout).

flags: [async]
features: [Atomics.waitAsync, SharedArrayBuffer, TypedArray, Atomics]
---*/
const i32a = new Int32Array(
  new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * 4)
);

const valueOf = {
  valueOf() {
    return false;
  }
};

const toPrimitive = {
  [Symbol.toPrimitive]() {
    return false;
  }
};

assert.sameValue(
  Atomics.waitAsync(i32a, 0, 0, false).value,
  'timed-out',
  'Atomics.waitAsync(i32a, 0, 0, false).value resolves to "timed-out"'
);

assert.sameValue(
  Atomics.waitAsync(i32a, 0, 0, valueOf).value,
  'timed-out',
  'Atomics.waitAsync(i32a, 0, 0, false).value resolves to "timed-out"'
);

assert.sameValue(
  Atomics.waitAsync(i32a, 0, 0, toPrimitive).value,
  'timed-out',
  'Atomics.waitAsync(i32a, 0, 0, false).value resolves to "timed-out"'
);

Promise.all([
    Atomics.waitAsync(i32a, 0, 0, false).value,
    Atomics.waitAsync(i32a, 0, 0, valueOf).value,
    Atomics.waitAsync(i32a, 0, 0, toPrimitive).value,
  ]).then(outcomes => {
    assert.sameValue(outcomes[0], 'timed-out');
    assert.sameValue(outcomes[1], 'timed-out');
    assert.sameValue(outcomes[2], 'timed-out');
  }, $DONE).then($DONE, $DONE);
