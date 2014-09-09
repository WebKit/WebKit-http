/*
 * Copyright (C) 2008, 2014 Apple Inc. All Rights Reserved.
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

#ifndef TypeSet_h
#define TypeSet_h

#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>
#include <wtf/Vector.h>

namespace JSC {

class JSValue;

enum RuntimeType {
    TypeNothing            = 0x0,
    TypeFunction           = 0x1,
    TypeUndefined          = 0x2,
    TypeNull               = 0x4,
    TypeBoolean            = 0x8,
    TypeMachineInt         = 0x10,
    TypeNumber             = 0x20,
    TypeString             = 0x40,
    TypePrimitive          = 0x80,
    TypeObject             = 0x100
};

class StructureShape : public RefCounted<StructureShape> {
    friend class TypeSet;

public:
    StructureShape();

    static PassRefPtr<StructureShape> create() { return adoptRef(new StructureShape); }
    String propertyHash();
    void markAsFinal();
    void addProperty(RefPtr<StringImpl>);
    static String leastUpperBound(Vector<RefPtr<StructureShape>>*);
    String stringRepresentation();

private:
    HashMap<RefPtr<StringImpl>, bool> m_fields;         
    std::unique_ptr<String> m_propertyHash;
    bool m_final;
};

class TypeSet : public RefCounted<TypeSet> {

public:
    static PassRefPtr<TypeSet> create() { return adoptRef(new TypeSet); }
    TypeSet();
    void addTypeForValue(JSValue v, PassRefPtr<StructureShape>);
    static RuntimeType getRuntimeTypeForValue(JSValue);
    JS_EXPORT_PRIVATE String seenTypes();

private:
    uint32_t m_seenTypes;
    void dumpSeenTypes();
    Vector<RefPtr<StructureShape>>* m_structureHistory;
    bool m_mightHaveDuplicatesInStructureHistory;
    void removeDuplicatesInStructureHistory();

};

} //namespace JSC

#endif //TypeSet_h
