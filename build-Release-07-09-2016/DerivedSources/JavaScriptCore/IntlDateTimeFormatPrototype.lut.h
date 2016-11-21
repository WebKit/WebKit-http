// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/runtime/IntlDateTimeFormatPrototype.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex dateTimeFormatPrototypeTableIndex[4] = {
    { -1, -1 },
    { 0, -1 },
    { -1, -1 },
    { 1, -1 },
};

static const struct HashTableValue dateTimeFormatPrototypeTableValues[2] = {
   { "format", DontEnum|Accessor, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(IntlDateTimeFormatPrototypeGetterFormat), (intptr_t)static_cast<NativeFunction>(nullptr) } },
   { "resolvedOptions", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(IntlDateTimeFormatPrototypeFuncResolvedOptions), (intptr_t)(0) } },
};

static const struct HashTable dateTimeFormatPrototypeTable =
    { 2, 3, true, dateTimeFormatPrototypeTableValues, dateTimeFormatPrototypeTableIndex };

} // namespace JSC
