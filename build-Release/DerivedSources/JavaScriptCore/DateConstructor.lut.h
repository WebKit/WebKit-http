// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/runtime/DateConstructor.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex dateConstructorTableIndex[9] = {
    { -1, -1 },
    { 0, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 1, 8 },
    { -1, -1 },
    { -1, -1 },
    { 2, -1 },
};

static const struct HashTableValue dateConstructorTableValues[3] = {
   { "parse", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(dateParse), (intptr_t)(1) } },
   { "UTC", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(dateUTC), (intptr_t)(7) } },
   { "now", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(dateNow), (intptr_t)(0) } },
};

static const struct HashTable dateConstructorTable =
    { 3, 7, false, dateConstructorTableValues, dateConstructorTableIndex };

} // namespace JSC
