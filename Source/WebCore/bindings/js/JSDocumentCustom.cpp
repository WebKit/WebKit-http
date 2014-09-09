/*
 * Copyright (C) 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSDocument.h"

#include "ExceptionCode.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTMLDocument.h"
#include "JSCanvasRenderingContext2D.h"
#if ENABLE(WEBGL)
#include "JSWebGLRenderingContext.h"
#endif
#include "JSDOMWindowCustom.h"
#include "JSHTMLDocument.h"
#include "JSLocation.h"
#include "JSSVGDocument.h"
#include "JSTouch.h"
#include "JSTouchList.h"
#include "Location.h"
#include "NodeTraversal.h"
#include "ScriptController.h"
#include "SVGDocument.h"
#include "TouchList.h"

#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

JSValue JSDocument::location(ExecState* exec) const
{
    RefPtr<Frame> frame = impl().frame();
    if (!frame)
        return jsNull();

    RefPtr<Location> location = frame->document()->domWindow()->location();
    if (JSObject* wrapper = getCachedWrapper(globalObject()->world(), location.get()))
        return wrapper;

    JSLocation* jsLocation = JSLocation::create(getDOMStructure<JSLocation>(exec->vm(), globalObject()), globalObject(), location.get());
    cacheWrapper(globalObject()->world(), location.get(), jsLocation);
    return jsLocation;
}

void JSDocument::setLocation(ExecState* exec, JSValue value)
{
    String locationString = value.toString(exec)->value(exec);
    if (exec->hadException())
        return;

    RefPtr<Frame> frame = impl().frame();
    if (!frame)
        return;

    if (RefPtr<Location> location = frame->document()->domWindow()->location())
        location->setHref(locationString, activeDOMWindow(exec), firstDOMWindow(exec));
}

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, Document* document)
{
    if (!document)
        return jsNull();

    JSObject* wrapper = getCachedWrapper(globalObject->world(), document);
    if (wrapper)
        return wrapper;

    if (DOMWindow* domWindow = document->domWindow()) {
        globalObject = toJSDOMWindow(toJS(exec, domWindow));
        // Creating a wrapper for domWindow might have created a wrapper for document as well.
        wrapper = getCachedWrapper(globalObject->world(), document);
        if (wrapper)
            return wrapper;
    }

    if (document->isHTMLDocument())
        wrapper = CREATE_DOM_WRAPPER(globalObject, HTMLDocument, document);
    else if (document->isSVGDocument())
        wrapper = CREATE_DOM_WRAPPER(globalObject, SVGDocument, document);
    else
        wrapper = CREATE_DOM_WRAPPER(globalObject, Document, document);

    // Make sure the document is kept around by the window object, and works right with the
    // back/forward cache.
    if (!document->frame()) {
        size_t nodeCount = 0;
        for (Node* n = document; n; n = NodeTraversal::next(n))
            nodeCount++;
        
        exec->heap()->reportExtraMemoryCost(nodeCount * sizeof(Node));
    }

    return wrapper;
}

#if ENABLE(TOUCH_EVENTS)
JSValue JSDocument::createTouchList(ExecState* exec)
{
    RefPtr<TouchList> touchList = TouchList::create();

    for (size_t i = 0; i < exec->argumentCount(); i++)
        touchList->append(toTouch(exec->argument(i)));

    return toJS(exec, globalObject(), touchList.release());
}
#endif

} // namespace WebCore
