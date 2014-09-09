/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef ErrorPrototype_h
#define ErrorPrototype_h

#include "ErrorInstance.h"

namespace JSC {

    class ObjectPrototype;

    class ErrorPrototype : public ErrorInstance {
    public:
        typedef ErrorInstance Base;

        static ErrorPrototype* create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
        {
            ErrorPrototype* prototype = new (NotNull, allocateCell<ErrorPrototype>(vm.heap)) ErrorPrototype(vm, structure);
            prototype->finishCreation(vm, globalObject);
            return prototype;
        }
        
        DECLARE_INFO;

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
        {
            return Structure::create(vm, globalObject, prototype, TypeInfo(ErrorInstanceType, StructureFlags), info());
        }

    protected:
        ErrorPrototype(VM&, Structure*);
        void finishCreation(VM&, JSGlobalObject*);

        static const unsigned StructureFlags = OverridesGetOwnPropertySlot | ErrorInstance::StructureFlags;

    private:
        static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    };

} // namespace JSC

#endif // ErrorPrototype_h
