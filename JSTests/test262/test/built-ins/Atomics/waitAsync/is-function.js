// Copyright (C) 2020 Rick Waldron. All rights reserved.
// This code is governed by the BSD license found in the LICENSE file.

/*---
esid: sec-atomics.waitasync
description: Atomics.waitAsync is callable
features: [Atomics.waitAsync]
---*/

assert.sameValue(typeof Atomics.waitAsync, 'function');
