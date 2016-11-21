// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/runtime/NumberPrototype.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex numberPrototypeTableIndex[17] = {
    { -1, -1 },
    { -1, -1 },
    { 2, 16 },
    { 5, -1 },
    { 4, -1 },
    { -1, -1 },
    { 1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 0, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 3, -1 },
};

static const struct HashTableValue numberPrototypeTableValues[6] = {
   { "toString", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(numberProtoFuncToString), (intptr_t)(1) } },
   { "toLocaleString", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(numberProtoFuncToLocaleString), (intptr_t)(0) } },
   { "valueOf", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(numberProtoFuncValueOf), (intptr_t)(0) } },
   { "toFixed", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(numberProtoFuncToFixed), (intptr_t)(1) } },
   { "toExponential", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(numberProtoFuncToExponential), (intptr_t)(1) } },
   { "toPrecision", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(numberProtoFuncToPrecision), (intptr_t)(1) } },
};

static const struct HashTable numberPrototypeTable =
    { 6, 15, false, numberPrototypeTableValues, numberPrototypeTableIndex };

} // namespace JSC
