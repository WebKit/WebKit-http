// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/runtime/JSONObject.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex jsonTableIndex[4] = {
    { -1, -1 },
    { 0, -1 },
    { -1, -1 },
    { 1, -1 },
};

static const struct HashTableValue jsonTableValues[2] = {
   { "parse", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(JSONProtoFuncParse), (intptr_t)(2) } },
   { "stringify", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(JSONProtoFuncStringify), (intptr_t)(3) } },
};

static const struct HashTable jsonTable =
    { 2, 3, false, jsonTableValues, jsonTableIndex };

} // namespace JSC
