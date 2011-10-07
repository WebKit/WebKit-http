/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef DFGOperations_h
#define DFGOperations_h

#if ENABLE(DFG_JIT)

#include <dfg/DFGJITCompiler.h>

namespace JSC {

struct GlobalResolveInfo;

namespace DFG {

enum PutKind { Direct, NotDirect };

typedef intptr_t RegisterSizedBoolean;

extern "C" {

#if CALLING_CONVENTION_IS_CDECL
#define DFG_OPERATION __attribute__ ((stdcall))
#else
#define DFG_OPERATION
#endif

// These typedefs provide typechecking when generating calls out to helper routines;
// this helps prevent calling a helper routine with the wrong arguments!
typedef JSCell* DFG_OPERATION (*C_DFGOperation_E)(ExecState*);
typedef JSCell* DFG_OPERATION (*C_DFGOperation_EC)(ExecState*, JSCell*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EA)(ExecState*, JSArray*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJA)(ExecState*, EncodedJSValue, JSArray*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJJ)(ExecState*, EncodedJSValue, EncodedJSValue);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJP)(ExecState*, EncodedJSValue, void*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EJI)(ExecState*, EncodedJSValue, Identifier*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EP)(ExecState*, void*);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EPS)(ExecState*, void*, size_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_ESS)(ExecState*, size_t, size_t);
typedef EncodedJSValue DFG_OPERATION (*J_DFGOperation_EI)(ExecState*, Identifier*);
typedef RegisterSizedBoolean DFG_OPERATION (*Z_DFGOperation_EJ)(ExecState*, EncodedJSValue);
typedef RegisterSizedBoolean DFG_OPERATION (*Z_DFGOperation_EJJ)(ExecState*, EncodedJSValue, EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_EJJJ)(ExecState*, EncodedJSValue, EncodedJSValue, EncodedJSValue);
typedef void DFG_OPERATION (*V_DFGOperation_EJJP)(ExecState*, EncodedJSValue, EncodedJSValue, void*);
typedef void DFG_OPERATION (*V_DFGOperation_EJJI)(ExecState*, EncodedJSValue, EncodedJSValue, Identifier*);
typedef double (*D_DFGOperation_DD)(double, double); // Using default calling conventions!
typedef void* DFG_OPERATION (*P_DFGOperation_E)(ExecState*);

// These routines are provide callbacks out to C++ implementations of operations too complex to JIT.
JSCell* DFG_OPERATION operationNewObject(ExecState*);
JSCell* DFG_OPERATION operationCreateThis(ExecState*, JSCell* encodedOp1);
EncodedJSValue DFG_OPERATION operationConvertThis(ExecState*, EncodedJSValue encodedOp1);
EncodedJSValue DFG_OPERATION operationValueAdd(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationValueAddNotNumber(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationArithAdd(EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationArithSub(EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationArithMul(EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationArithDiv(EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationArithMod(EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
EncodedJSValue DFG_OPERATION operationGetByVal(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty);
EncodedJSValue DFG_OPERATION operationGetById(ExecState*, EncodedJSValue encodedBase, Identifier*);
EncodedJSValue DFG_OPERATION operationGetByIdBuildList(ExecState*, EncodedJSValue encodedBase, Identifier*);
EncodedJSValue DFG_OPERATION operationGetByIdProtoBuildList(ExecState*, EncodedJSValue encodedBase, Identifier*);
EncodedJSValue DFG_OPERATION operationGetByIdOptimize(ExecState*, EncodedJSValue encodedBase, Identifier*);
EncodedJSValue DFG_OPERATION operationGetMethodOptimize(ExecState*, EncodedJSValue encodedBase, Identifier*);
EncodedJSValue DFG_OPERATION operationInstanceOf(ExecState*, EncodedJSValue value, EncodedJSValue base, EncodedJSValue prototype);
EncodedJSValue DFG_OPERATION operationResolve(ExecState*, Identifier*);
EncodedJSValue DFG_OPERATION operationResolveBase(ExecState*, Identifier*);
EncodedJSValue DFG_OPERATION operationResolveBaseStrictPut(ExecState*, Identifier*);
EncodedJSValue DFG_OPERATION operationResolveGlobal(ExecState*, GlobalResolveInfo*, Identifier*);
EncodedJSValue DFG_OPERATION operationToPrimitive(ExecState*, EncodedJSValue);
EncodedJSValue DFG_OPERATION operationStrCat(ExecState*, void* start, size_t);
EncodedJSValue DFG_OPERATION operationNewArray(ExecState*, void* start, size_t);
EncodedJSValue DFG_OPERATION operationNewArrayBuffer(ExecState*, size_t, size_t);
EncodedJSValue DFG_OPERATION operationNewRegexp(ExecState*, void*);
void DFG_OPERATION operationThrowHasInstanceError(ExecState*, EncodedJSValue base);
void DFG_OPERATION operationPutByValStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue);
void DFG_OPERATION operationPutByValNonStrict(ExecState*, EncodedJSValue encodedBase, EncodedJSValue encodedProperty, EncodedJSValue encodedValue);
void DFG_OPERATION operationPutByValBeyondArrayBounds(ExecState*, JSArray*, int32_t index, EncodedJSValue encodedValue);
EncodedJSValue DFG_OPERATION operationArrayPush(ExecState*, EncodedJSValue encodedValue, JSArray*);
EncodedJSValue DFG_OPERATION operationArrayPop(ExecState*, JSArray*);
void DFG_OPERATION operationPutByIdStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdNonStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdDirectStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdDirectNonStrict(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdStrictOptimize(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdNonStrictOptimize(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdDirectStrictOptimize(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
void DFG_OPERATION operationPutByIdDirectNonStrictOptimize(ExecState*, EncodedJSValue encodedValue, EncodedJSValue encodedBase, Identifier*);
RegisterSizedBoolean DFG_OPERATION operationCompareLess(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
RegisterSizedBoolean DFG_OPERATION operationCompareLessEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
RegisterSizedBoolean DFG_OPERATION operationCompareGreater(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
RegisterSizedBoolean DFG_OPERATION operationCompareGreaterEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
RegisterSizedBoolean DFG_OPERATION operationCompareEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
RegisterSizedBoolean DFG_OPERATION operationCompareStrictEqCell(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
RegisterSizedBoolean DFG_OPERATION operationCompareStrictEq(ExecState*, EncodedJSValue encodedOp1, EncodedJSValue encodedOp2);
void* DFG_OPERATION operationVirtualCall(ExecState*);
void* DFG_OPERATION operationLinkCall(ExecState*);
void* DFG_OPERATION operationVirtualConstruct(ExecState*);
void* DFG_OPERATION operationLinkConstruct(ExecState*);

// This method is used to lookup an exception hander, keyed by faultLocation, which is
// the return location from one of the calls out to one of the helper operations above.
struct DFGHandler {
    DFGHandler(ExecState* exec, void* handler)
        : exec(exec)
        , handler(handler)
    {
    }

    ExecState* exec;
    void* handler;
};
DFGHandler DFG_OPERATION lookupExceptionHandler(ExecState*, ReturnAddressPtr faultLocation);

// These operations implement the implicitly called ToInt32, ToNumber, and ToBoolean conversions from ES5.
double DFG_OPERATION dfgConvertJSValueToNumber(ExecState*, EncodedJSValue);
int32_t DFG_OPERATION dfgConvertJSValueToInt32(ExecState*, EncodedJSValue);
RegisterSizedBoolean DFG_OPERATION dfgConvertJSValueToBoolean(ExecState*, EncodedJSValue);

#if ENABLE(DFG_VERBOSE_SPECULATION_FAILURE)
void DFG_OPERATION debugOperationPrintSpeculationFailure(ExecState*, void*);
#endif

} // extern "C"
} } // namespace JSC::DFG

#endif
#endif
