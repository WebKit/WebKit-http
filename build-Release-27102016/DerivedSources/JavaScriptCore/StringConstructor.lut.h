// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/runtime/StringConstructor.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "JSCBuiltins.h"
#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex stringConstructorTableIndex[8] = {
    { -1, -1 },
    { 2, -1 },
    { -1, -1 },
    { -1, -1 },
    { 1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 0, -1 },
};

static const struct HashTableValue stringConstructorTableValues[3] = {
   { "fromCharCode", DontEnum|Function, FromCharCodeIntrinsic, { (intptr_t)static_cast<NativeFunction>(stringFromCharCode), (intptr_t)(1) } },
   { "fromCodePoint", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(stringFromCodePoint), (intptr_t)(1) } },
   { "raw", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(stringConstructorRawCodeGenerator), (intptr_t)1 } },
};

static const struct HashTable stringConstructorTable =
    { 3, 7, false, stringConstructorTableValues, stringConstructorTableIndex };

} // namespace JSC
