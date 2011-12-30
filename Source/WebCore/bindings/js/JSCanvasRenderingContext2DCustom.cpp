/*
 * Copyright (C) 2006, 2007, 2009 Apple Inc. All rights reserved.
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
#include "JSCanvasRenderingContext2D.h"

#include "CanvasGradient.h"
#include "CanvasPattern.h"
#include "CanvasRenderingContext2D.h"
#include "CanvasStyle.h"
#include "ExceptionCode.h"
#include "HTMLCanvasElement.h"
#include "HTMLImageElement.h"
#include "ImageData.h"
#include "JSCanvasGradient.h"
#include "JSCanvasPattern.h"
#include "JSHTMLCanvasElement.h"
#include "JSHTMLImageElement.h"
#include "JSImageData.h"

using namespace JSC;

namespace WebCore {

static JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, CanvasStyle* style)
{
    if (style->canvasGradient())
        return toJS(exec, globalObject, style->canvasGradient());
    if (style->canvasPattern())
        return toJS(exec, globalObject, style->canvasPattern());
    return jsString(exec, style->color());
}

static PassRefPtr<CanvasStyle> toHTMLCanvasStyle(ExecState*, JSValue value)
{
    if (!value.isObject())
        return 0;
    JSObject* object = asObject(value);
    if (object->inherits(&JSCanvasGradient::s_info))
        return CanvasStyle::createFromGradient(jsCast<JSCanvasGradient*>(object)->impl());
    if (object->inherits(&JSCanvasPattern::s_info))
        return CanvasStyle::createFromPattern(jsCast<JSCanvasPattern*>(object)->impl());
    return 0;
}

JSValue JSCanvasRenderingContext2D::strokeStyle(ExecState* exec) const
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    return toJS(exec, globalObject(), context->strokeStyle());        
}

void JSCanvasRenderingContext2D::setStrokeStyle(ExecState* exec, JSValue value)
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    if (value.isString()) {
        context->setStrokeColor(ustringToString(asString(value)->value(exec)));
        return;
    }
    context->setStrokeStyle(toHTMLCanvasStyle(exec, value));
}

JSValue JSCanvasRenderingContext2D::fillStyle(ExecState* exec) const
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    return toJS(exec, globalObject(), context->fillStyle());
}

void JSCanvasRenderingContext2D::setFillStyle(ExecState* exec, JSValue value)
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    if (value.isString()) {
        context->setFillColor(ustringToString(asString(value)->value(exec)));
        return;
    }
    context->setFillStyle(toHTMLCanvasStyle(exec, value));
}

JSValue JSCanvasRenderingContext2D::webkitLineDash(ExecState* exec) const
{
    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    const DashArray* dash = context->webkitLineDash();

    MarkedArgumentBuffer list;
    DashArray::const_iterator end = dash->end();
    for (DashArray::const_iterator it = dash->begin(); it != end; ++it)
        list.append(JSValue(*it));
    return constructArray(exec, globalObject(), list);
}

void JSCanvasRenderingContext2D::setWebkitLineDash(ExecState* exec, JSValue value)
{
    if (!isJSArray(value))
        return;

    DashArray dash;
    JSArray* valueArray = asArray(value);
    for (unsigned i = 0; i < valueArray->length(); ++i) {
        float elem = valueArray->getIndex(i).toFloat(exec);
        if (elem <= 0 || !isfinite(elem))
            return;

        dash.append(elem);
    }

    CanvasRenderingContext2D* context = static_cast<CanvasRenderingContext2D*>(impl());
    context->setWebkitLineDash(dash);
}

} // namespace WebCore
