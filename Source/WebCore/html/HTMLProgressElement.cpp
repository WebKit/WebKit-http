/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 *
 */

#include "config.h"
#include "HTMLProgressElement.h"

#if ENABLE(PROGRESS_ELEMENT)
#include "Attribute.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "NodeRenderingContext.h"
#include "HTMLDivElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "ProgressShadowElement.h"
#include "RenderProgress.h"
#include "ShadowRoot.h"
#include <wtf/StdLibExtras.h>

namespace WebCore {

using namespace HTMLNames;

const double HTMLProgressElement::IndeterminatePosition = -1;
const double HTMLProgressElement::InvalidPosition = -2;

HTMLProgressElement::HTMLProgressElement(const QualifiedName& tagName, Document* document)
    : LabelableElement(tagName, document)
    , m_value(0)
    , m_hasAuthorShadowRoot(false)
{
    ASSERT(hasTagName(progressTag));
}

HTMLProgressElement::~HTMLProgressElement()
{
}

PassRefPtr<HTMLProgressElement> HTMLProgressElement::create(const QualifiedName& tagName, Document* document)
{
    RefPtr<HTMLProgressElement> progress = adoptRef(new HTMLProgressElement(tagName, document));
    progress->createShadowSubtree();
    return progress;
}

RenderObject* HTMLProgressElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    if (!style->hasAppearance() || hasAuthorShadowRoot())
        return RenderObject::createObject(this, style);

    return new (arena) RenderProgress(this);
}

bool HTMLProgressElement::childShouldCreateRenderer(const NodeRenderingContext& childContext) const
{
    return childContext.isOnUpperEncapsulationBoundary() && HTMLElement::childShouldCreateRenderer(childContext);
}

RenderProgress* HTMLProgressElement::renderProgress() const
{
    if (renderer() && renderer()->isProgress())
        return static_cast<RenderProgress*>(renderer());

    RenderObject* renderObject = userAgentShadowRoot()->firstChild()->renderer();
    ASSERT(!renderObject || renderObject->isProgress());
    return static_cast<RenderProgress*>(renderObject);
}

void HTMLProgressElement::willAddAuthorShadowRoot()
{
    m_hasAuthorShadowRoot = true;
}

bool HTMLProgressElement::supportsFocus() const
{
    return Node::supportsFocus() && !disabled();
}

void HTMLProgressElement::parseAttribute(const Attribute& attribute)
{
    if (attribute.name() == valueAttr)
        didElementStateChange();
    else if (attribute.name() == maxAttr)
        didElementStateChange();
    else
        LabelableElement::parseAttribute(attribute);
}

void HTMLProgressElement::attach()
{
    LabelableElement::attach();
    didElementStateChange();
}

double HTMLProgressElement::value() const
{
    double value = parseToDoubleForNumberType(fastGetAttribute(valueAttr));
    return !isfinite(value) || value < 0 ? 0 : std::min(value, max());
}

void HTMLProgressElement::setValue(double value, ExceptionCode& ec)
{
    if (!isfinite(value)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    setAttribute(valueAttr, String::number(value >= 0 ? value : 0));
}

double HTMLProgressElement::max() const
{
    double max = parseToDoubleForNumberType(getAttribute(maxAttr));
    return !isfinite(max) || max <= 0 ? 1 : max;
}

void HTMLProgressElement::setMax(double max, ExceptionCode& ec)
{
    if (!isfinite(max)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }
    setAttribute(maxAttr, String::number(max > 0 ? max : 1));
}

double HTMLProgressElement::position() const
{
    if (!isDeterminate())
        return HTMLProgressElement::IndeterminatePosition;
    return value() / max();
}

bool HTMLProgressElement::isDeterminate() const
{
    return fastHasAttribute(valueAttr);
}
    
void HTMLProgressElement::didElementStateChange()
{
    m_value->setWidthPercentage(position() * 100);
    if (RenderProgress* render = renderProgress()) {
        bool wasDeterminate = render->isDeterminate();
        render->updateFromElement();
        if (wasDeterminate != isDeterminate())
            setNeedsStyleRecalc();
    }
}

void HTMLProgressElement::createShadowSubtree()
{
    ASSERT(!userAgentShadowRoot());
    ASSERT(!m_value);
           
    RefPtr<ShadowRoot> root = ShadowRoot::create(this, ShadowRoot::UserAgentShadowRoot, ASSERT_NO_EXCEPTION);

    RefPtr<ProgressInnerElement> inner = ProgressInnerElement::create(document());
    root->appendChild(inner);

    RefPtr<ProgressBarElement> bar = ProgressBarElement::create(document());
    RefPtr<ProgressValueElement> value = ProgressValueElement::create(document());
    m_value = value.get();
    bar->appendChild(m_value, ASSERT_NO_EXCEPTION);

    inner->appendChild(bar, ASSERT_NO_EXCEPTION);
}

} // namespace
#endif
