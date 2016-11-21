// Automatically generated from /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/runtime/ReflectObject.cpp using /home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/JavaScriptCore/create_hash_table. DO NOT EDIT!

#include "JSCBuiltins.h"
#include "Lookup.h"

namespace JSC {

static const struct CompactHashIndex reflectObjectTableIndex[35] = {
    { 13, -1 },
    { 1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 5, -1 },
    { -1, -1 },
    { 12, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 11, -1 },
    { 8, 34 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 2, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 4, -1 },
    { 6, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { -1, -1 },
    { 3, 32 },
    { 0, 33 },
    { -1, -1 },
    { 7, -1 },
    { 9, -1 },
    { 10, -1 },
};

static const struct HashTableValue reflectObjectTableValues[14] = {
   { "apply", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(reflectObjectApplyCodeGenerator), (intptr_t)3 } },
   { "construct", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectConstruct), (intptr_t)(2) } },
   { "defineProperty", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectDefineProperty), (intptr_t)(3) } },
   { "deleteProperty", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(reflectObjectDeletePropertyCodeGenerator), (intptr_t)2 } },
   { "enumerate", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectEnumerate), (intptr_t)(1) } },
   { "get", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectGet), (intptr_t)(2) } },
   { "getOwnPropertyDescriptor", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectGetOwnPropertyDescriptor), (intptr_t)(2) } },
   { "getPrototypeOf", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectGetPrototypeOf), (intptr_t)(1) } },
   { "has", ((DontEnum|Function) & ~Function) | Builtin, NoIntrinsic, { (intptr_t)static_cast<BuiltinGenerator>(reflectObjectHasCodeGenerator), (intptr_t)2 } },
   { "isExtensible", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectIsExtensible), (intptr_t)(1) } },
   { "ownKeys", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectOwnKeys), (intptr_t)(1) } },
   { "preventExtensions", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectPreventExtensions), (intptr_t)(1) } },
   { "set", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectSet), (intptr_t)(3) } },
   { "setPrototypeOf", DontEnum|Function, NoIntrinsic, { (intptr_t)static_cast<NativeFunction>(reflectObjectSetPrototypeOf), (intptr_t)(2) } },
};

static const struct HashTable reflectObjectTable =
    { 14, 31, false, reflectObjectTableValues, reflectObjectTableIndex };

} // namespace JSC
