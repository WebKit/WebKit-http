/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSHTMLOptionsCollection.h"

#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLOptionsCollection.h"
#include "HTMLSelectElement.h"
#include "JSHTMLOptionElement.h"
#include "JSHTMLSelectElement.h"
#include "JSHTMLSelectElementCustom.h"
#include "JSNodeList.h"
#include "StaticNodeList.h"

#include <wtf/MathExtras.h>

using namespace JSC;

namespace WebCore {

static JSValue getNamedItems(ExecState* exec, JSHTMLOptionsCollection* collection, PropertyName propertyName)
{
    Vector<RefPtr<Node> > namedItems;
    const AtomicString& name = propertyNameToAtomicString(propertyName);
    collection->impl()->namedItems(name, namedItems);

    if (namedItems.isEmpty())
        return jsUndefined();
    if (namedItems.size() == 1)
        return toJS(exec, collection->globalObject(), namedItems[0].get());

    // FIXME: HTML5 specifies that this should be a LiveNodeList.
    return toJS(exec, collection->globalObject(), StaticNodeList::adopt(namedItems).get());
}

bool JSHTMLOptionsCollection::canGetItemsForName(ExecState*, HTMLOptionsCollection* collection, PropertyName propertyName)
{
    return collection->hasNamedItem(propertyNameToAtomicString(propertyName));
}

JSValue JSHTMLOptionsCollection::nameGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSHTMLOptionsCollection* thisObj = jsCast<JSHTMLOptionsCollection*>(asObject(slotBase));
    return getNamedItems(exec, thisObj, propertyName);
}

JSValue JSHTMLOptionsCollection::namedItem(ExecState* exec)
{
    return getNamedItems(exec, this, Identifier(exec, exec->argument(0).toString(exec)->value(exec)));
}

void JSHTMLOptionsCollection::setLength(ExecState* exec, JSValue value)
{
    HTMLOptionsCollection* imp = static_cast<HTMLOptionsCollection*>(impl());
    ExceptionCode ec = 0;
    unsigned newLength = 0;
    double lengthValue = value.toNumber(exec);
    if (!isnan(lengthValue) && !isinf(lengthValue)) {
        if (lengthValue < 0.0)
            ec = INDEX_SIZE_ERR;
        else if (lengthValue > static_cast<double>(UINT_MAX))
            newLength = UINT_MAX;
        else
            newLength = static_cast<unsigned>(lengthValue);
    }
    if (!ec)
        imp->setLength(newLength, ec);
    setDOMException(exec, ec);
}

void JSHTMLOptionsCollection::indexSetter(ExecState* exec, unsigned index, JSValue value)
{
    HTMLOptionsCollection* imp = static_cast<HTMLOptionsCollection*>(impl());
    HTMLSelectElement* base = toHTMLSelectElement(imp->base());
    selectIndexSetter(base, exec, index, value);
}

JSValue JSHTMLOptionsCollection::add(ExecState* exec)
{
    HTMLOptionsCollection* imp = static_cast<HTMLOptionsCollection*>(impl());
    HTMLOptionElement* option = toHTMLOptionElement(exec->argument(0));
    ExceptionCode ec = 0;
    if (exec->argumentCount() < 2)
        imp->add(option, ec);
    else {
        bool ok;
        int index = finiteInt32Value(exec->argument(1), exec, ok);
        if (exec->hadException())
            return jsUndefined();
        if (!ok)
            ec = TYPE_MISMATCH_ERR;
        else
            imp->add(option, index, ec);
    }
    setDOMException(exec, ec);
    return jsUndefined();
}

JSValue JSHTMLOptionsCollection::remove(ExecState* exec)
{
    HTMLOptionsCollection* imp = static_cast<HTMLOptionsCollection*>(impl());
    JSHTMLSelectElement* base = jsCast<JSHTMLSelectElement*>(asObject(toJS(exec, globalObject(), imp->base())));
    return base->remove(exec);
}

}
