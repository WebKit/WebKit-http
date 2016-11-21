// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/runtime/InspectorInstrumentationObject.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "JSCBuiltins.h"
#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex inspectorInstrumentationObjectTableIndex[9] = {
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 0, 8 },
    { -1, -1 },
    { 1, -1 },
    { -1, -1 },
    { 2, -1 },
};

static const struct HashTableValue inspectorInstrumentationObjectTableValues[3] = {
   { "log", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(inspectorInstrumentationObjectLog), (intptr_t)(1) } },
   { "promiseFulfilled", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(inspectorInstrumentationObjectPromiseFulfilledCodeGenerator), (intptr_t)3 } },
   { "promiseRejected", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(inspectorInstrumentationObjectPromiseRejectedCodeGenerator), (intptr_t)3 } },
};

static const struct HashTable inspectorInstrumentationObjectTable =
    { 3, 7, false, inspectorInstrumentationObjectTableValues, inspectorInstrumentationObjectTableIndex };

} // namespace JSC
