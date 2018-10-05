//@ skip if $memoryLimited
const s = "123".padStart(1073741823);
try {
    JSON.stringify(s);
} catch (e) {}
