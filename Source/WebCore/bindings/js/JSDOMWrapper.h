/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
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
 */

#ifndef JSDOMWrapper_h
#define JSDOMWrapper_h

#include "JSDOMGlobalObject.h"
#include <runtime/JSObjectWithGlobalObject.h>

namespace WebCore {

class ScriptExecutionContext;

class JSDOMWrapper : public JSC::JSObjectWithGlobalObject {
public:
    JSDOMGlobalObject* globalObject() const
    {
        return static_cast<JSDOMGlobalObject*>(JSC::JSObjectWithGlobalObject::globalObject());
    }

    ScriptExecutionContext* scriptExecutionContext() const
    {
        // FIXME: Should never be 0, but can be due to bug 27640.
        return globalObject()->scriptExecutionContext();
    }

    static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype)
    {
        return JSC::Structure::create(globalData, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }

protected:
    explicit JSDOMWrapper(JSC::Structure* structure, JSC::JSGlobalObject* globalObject) 
        : JSObjectWithGlobalObject(globalObject, structure)
    {
        // FIXME: This ASSERT is valid, but fires in fast/dom/gc-6.html when trying to create
        // new JavaScript objects on detached windows due to DOMWindow::document()
        // needing to reach through the frame to get to the Document*.  See bug 27640.
        // ASSERT(globalObject->scriptExecutionContext());
    }

#ifndef NDEBUG
    virtual ~JSDOMWrapper();
#endif
};

} // namespace WebCore

#endif // JSDOMWrapper_h
