/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef StringConstructor_h
#define StringConstructor_h

#include "InternalFunction.h"

namespace JSC {

    class StringPrototype;

    class StringConstructor : public InternalFunction {
    public:
        typedef InternalFunction Base;

        static StringConstructor* create(ExecState* exec, JSGlobalObject* globalObject , Structure* structure, StringPrototype* stringPrototype)
        {
            StringConstructor* constructor = new (allocateCell<StringConstructor>(*exec->heap())) StringConstructor(globalObject, structure);
            constructor->finishCreation(exec, stringPrototype);
            return constructor;
        }

        static const ClassInfo s_info;

        static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
        }

    protected:
        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InternalFunction::StructureFlags;

    private:
        StringConstructor(JSGlobalObject*, Structure*);
        void finishCreation(ExecState*, StringPrototype*);
        virtual ConstructType getConstructData(ConstructData&);
        virtual CallType getCallDataVirtual(CallData&);
        static CallType getCallData(JSCell*, CallData&);

        virtual bool getOwnPropertySlot(ExecState*, const Identifier&, PropertySlot&);
        virtual bool getOwnPropertyDescriptor(ExecState*, const Identifier&, PropertyDescriptor&);
    };

} // namespace JSC

#endif // StringConstructor_h
