// Copyright (C) 2020 Rick Waldron. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-atomics.waitasync
description: >
  Waiter does not spuriously notify on index which is subject to Or operation
info: |
  AddWaiter ( WL, waiterRecord )

  5. Append waiterRecord as the last element of WL.[[Waiters]]
  6. If waiterRecord.[[Timeout]] is finite, then in parallel,
    a. Wait waiterRecord.[[Timeout]] milliseconds.
    b. Perform TriggerTimeout(WL, waiterRecord).

  TriggerTimeout( WL, waiterRecord )

  3. If waiterRecord is in WL.[[Waiters]], then
    a. Set waiterRecord.[[Result]] to "timed-out".
    b. Perform RemoveWaiter(WL, waiterRecord).
    c. Perform NotifyWaiter(WL, waiterRecord).
  4. Perform LeaveCriticalSection(WL).

flags: [async]
includes: [atomicsHelper.js]
features: [Atomics.waitAsync, SharedArrayBuffer, TypedArray, Atomics]
---*/
assert.sameValue(typeof Atomics.waitAsync, 'function');

const RUNNING = 1;
const TIMEOUT = $262.agent.timeouts.small;

const i32a = new Int32Array(
  new SharedArrayBuffer(Int32Array.BYTES_PER_ELEMENT * 4)
);

$262.agent.start(`
  $262.agent.receiveBroadcast(async (sab) => {
    const i32a = new Int32Array(sab);
    Atomics.add(i32a, ${RUNNING}, 1);

    const before = $262.agent.monotonicNow();
    const unpark = await Atomics.waitAsync(i32a, 0, 0, ${TIMEOUT}).value;
    const duration = $262.agent.monotonicNow() - before;

    $262.agent.report(duration);
    $262.agent.report(unpark);
    $262.agent.leaving();
  });
`);

$262.agent.safeBroadcastAsync(i32a, RUNNING, 1).then(async (agentCount) => {

  assert.sameValue(agentCount, 1);

  Atomics.or(i32a, 0, 1);

  const lapse = await $262.agent.getReportAsync();

  assert(
    lapse >= TIMEOUT,
    'The result of `(lapse >= TIMEOUT)` is true'
  );

  const result = await $262.agent.getReportAsync();

  assert.sameValue(
    result,
    'timed-out',
    'await Atomics.waitAsync(i32a, 0, 0, ${TIMEOUT}).value resolves to "timed-out"'
  );
  assert.sameValue(Atomics.notify(i32a, 0), 0, 'Atomics.notify(i32a, 0) returns 0');
}).then($DONE, $DONE);
