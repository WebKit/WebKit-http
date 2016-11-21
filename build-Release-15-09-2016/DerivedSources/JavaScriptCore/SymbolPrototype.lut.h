// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/runtime/SymbolPrototype.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex symbolPrototypeTableIndex[4] = {
    { 0, -1 },
    { -1, -1 },
    { 1, -1 },
    { -1, -1 },
};

static const struct HashTableValue symbolPrototypeTableValues[2] = {
   { "toString", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(symbolProtoFuncToString), (intptr_t)(0) } },
   { "valueOf", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(symbolProtoFuncValueOf), (intptr_t)(0) } },
};

static const struct HashTable symbolPrototypeTable =
    { 2, 3, false, symbolPrototypeTableValues, symbolPrototypeTableIndex };

} // namespace JSC
