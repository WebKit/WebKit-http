//@ skip if $buildType == "debug"

let code = `
    function foo() {
      return [Date.prototype];
    }

    for (let i = 0; i < 50; ++i) {
      foo();
    }

    let [proto] = foo();
    Object.defineProperty(proto, '0', {});
`;

for (let i = 0; i < 2000; i++) {
    runString(code);
}
