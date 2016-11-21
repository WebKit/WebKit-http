// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/runtime/StringIteratorPrototype.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "JSCBuiltins.h"
#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex stringIteratorPrototypeTableIndex[2] = {
    { -1, -1 },
    { 0, -1 },
};

static const struct HashTableValue stringIteratorPrototypeTableValues[1] = {
   { "next", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(stringIteratorPrototypeNextCodeGenerator), (intptr_t)0 } },
};

static const struct HashTable stringIteratorPrototypeTable =
    { 1, 1, false, stringIteratorPrototypeTableValues, stringIteratorPrototypeTableIndex };

} // namespace JSC
