/*
 * Copyright (C) 2005, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UserObjectImp_h
#define UserObjectImp_h

#include "JSUtils.h"
#include "JSBase.h"
#include "JSObject.h"
#include <JavaScriptCore/JSType.h>

class UserObjectImp : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    static UserObjectImp* create(JSGlobalData& globalData, Structure* structure, JSUserObject* userObject)
    {
        return new (allocateCell<UserObjectImp>(globalData.heap)) UserObjectImp(globalData, structure, userObject);
    }
    
    virtual ~UserObjectImp();

    static const ClassInfo s_info;

    virtual CallType getCallData(CallData&);

    virtual void getOwnPropertyNames(ExecState*, PropertyNameArray&, EnumerationMode mode = ExcludeDontEnumProperties);

    virtual JSValue callAsFunction(ExecState *exec);
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue value, PutPropertySlot&);

    JSValue toPrimitive(ExecState*, PreferredPrimitiveType preferredType = NoPreference) const;
    virtual bool toBoolean(ExecState *exec) const;
    virtual double toNumber(ExecState *exec) const;
    virtual UString toString(ExecState *exec) const;

    virtual void visitChildren(SlotVisitor&);

    JSUserObject *GetJSUserObject() const;

    static Structure* createStructure(JSGlobalData& globalData, JSValue prototype)
    {
        return Structure::create(globalData, prototype, TypeInfo(ObjectType, OverridesGetOwnPropertySlot | OverridesVisitChildren | OverridesGetPropertyNames), AnonymousSlotCount, &s_info);
    }

private:
    UserObjectImp(JSGlobalData&, Structure*, JSUserObject*);
    static JSValue userObjectGetter(ExecState*, JSValue, const Identifier& propertyName);

    JSUserObject* fJSUserObject;
};

#endif
