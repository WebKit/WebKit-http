// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/runtime/GeneratorPrototype.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "JSCBuiltins.h"
#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex generatorPrototypeTableIndex[8] = {
    { -1, -1 },
    { 0, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 2, -1 },
    { 1, -1 },
};

static const struct HashTableValue generatorPrototypeTableValues[3] = {
   { "next", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(generatorPrototypeNextCodeGenerator), (intptr_t)1 } },
   { "return", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(generatorPrototypeReturnCodeGenerator), (intptr_t)1 } },
   { "throw", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(generatorPrototypeThrowCodeGenerator), (intptr_t)1 } },
};

static const struct HashTable generatorPrototypeTable =
    { 3, 7, false, generatorPrototypeTableValues, generatorPrototypeTableIndex };

} // namespace JSC
