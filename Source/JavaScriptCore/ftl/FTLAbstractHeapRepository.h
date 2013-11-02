/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef FTLAbstractHeapRepository_h
#define FTLAbstractHeapRepository_h

#include <wtf/Platform.h>

#if ENABLE(FTL_JIT)

#include "FTLAbstractHeap.h"

namespace JSC { namespace FTL {

#define FOR_EACH_ABSTRACT_HEAP(macro) \
    macro(length) \
    macro(typedArrayProperties)

#define FOR_EACH_ABSTRACT_FIELD(macro) \
    macro(Butterfly_publicLength, Butterfly::offsetOfPublicLength()) \
    macro(Butterfly_vectorLength, Butterfly::offsetOfVectorLength()) \
    macro(CallFrame_callerFrame, CallFrame::callerFrameOffset()) \
    macro(JSArrayBufferView_length, JSArrayBufferView::offsetOfLength()) \
    macro(JSArrayBufferView_mode, JSArrayBufferView::offsetOfMode()) \
    macro(JSArrayBufferView_vector, JSArrayBufferView::offsetOfVector()) \
    macro(JSCell_structure, JSCell::structureOffset()) \
    macro(JSObject_butterfly, JSObject::butterflyOffset()) \
    macro(JSScope_next, JSScope::offsetOfNext()) \
    macro(JSString_length, JSString::offsetOfLength()) \
    macro(JSString_value, JSString::offsetOfValue()) \
    macro(JSVariableObject_registers, JSVariableObject::offsetOfRegisters()) \
    macro(StringImpl_data, StringImpl::dataOffset()) \
    macro(StringImpl_hashAndFlags, StringImpl::flagsOffset()) \
    macro(Structure_classInfo, Structure::classInfoOffset()) \
    macro(Structure_globalObject, Structure::globalObjectOffset()) \
    macro(Structure_indexingType, Structure::indexingTypeOffset()) \
    macro(Structure_typeInfoFlags, Structure::typeInfoFlagsOffset())

#define FOR_EACH_INDEXED_ABSTRACT_HEAP(macro) \
    macro(characters8, sizeof(LChar)) \
    macro(characters16, sizeof(UChar)) \
    macro(indexedInt32Properties, sizeof(EncodedJSValue)) \
    macro(indexedDoubleProperties, sizeof(double)) \
    macro(indexedContiguousProperties, sizeof(EncodedJSValue)) \
    macro(indexedArrayStorageProperties, sizeof(EncodedJSValue)) \
    macro(variables, sizeof(Register))
    
#define FOR_EACH_NUMBERED_ABSTRACT_HEAP(macro) \
    macro(properties)
    
// This class is meant to be cacheable between compilations, but it doesn't have to be.
// Doing so saves on creation of nodes. But clearing it will save memory.

class AbstractHeapRepository {
    WTF_MAKE_NONCOPYABLE(AbstractHeapRepository);
public:
    AbstractHeapRepository(LContext);
    ~AbstractHeapRepository();
    
    AbstractHeap root;
    
#define ABSTRACT_HEAP_DECLARATION(name) AbstractHeap name;
    FOR_EACH_ABSTRACT_HEAP(ABSTRACT_HEAP_DECLARATION)
#undef ABSTRACT_HEAP_DECLARATION

#define ABSTRACT_FIELD_DECLARATION(name, offset) AbstractField name;
    FOR_EACH_ABSTRACT_FIELD(ABSTRACT_FIELD_DECLARATION)
#undef ABSTRACT_FIELD_DECLARATION
    
#define INDEXED_ABSTRACT_HEAP_DECLARATION(name, size) IndexedAbstractHeap name;
    FOR_EACH_INDEXED_ABSTRACT_HEAP(INDEXED_ABSTRACT_HEAP_DECLARATION)
#undef INDEXED_ABSTRACT_HEAP_DECLARATION
    
#define NUMBERED_ABSTRACT_HEAP_DECLARATION(name) NumberedAbstractHeap name;
    FOR_EACH_NUMBERED_ABSTRACT_HEAP(NUMBERED_ABSTRACT_HEAP_DECLARATION)
#undef NUMBERED_ABSTRACT_HEAP_DECLARATION

    AbsoluteAbstractHeap absolute;

private:
    friend class AbstractHeap;
    
    LContext m_context;
    unsigned m_tbaaKind;
};

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

#endif // FTLAbstractHeapRepository_h

