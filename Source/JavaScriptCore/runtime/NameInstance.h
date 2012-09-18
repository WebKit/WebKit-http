/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NameInstance_h
#define NameInstance_h

#include "JSObject.h"
#include "PrivateName.h"

namespace JSC {

class NameInstance : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    static const ClassInfo s_info;

    static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(globalData, globalObject, prototype, TypeInfo(NameInstanceType, StructureFlags), &s_info);
    }

    static NameInstance* create(JSGlobalData& globalData, Structure* structure, JSString* nameString)
    {
        NameInstance* name = new (NotNull, allocateCell<NameInstance>(globalData.heap)) NameInstance(globalData, structure, nameString);
        name->finishCreation(globalData);
        return name;
    }

    const PrivateName& privateName() { return m_privateName; }
    JSString* nameString() { return m_nameString.get(); }

protected:
    static void destroy(JSCell*);

    NameInstance(JSGlobalData&, Structure*, JSString*);

    void finishCreation(JSGlobalData& globalData)
    {
        Base::finishCreation(globalData);
        ASSERT(inherits(&s_info));
    }

    PrivateName m_privateName;
    WriteBarrier<JSString> m_nameString;
};

inline bool isName(JSValue v)
{
    return v.isCell() && v.asCell()->structure()->typeInfo().isName();
}

} // namespace JSC

#endif // NameInstance_h
