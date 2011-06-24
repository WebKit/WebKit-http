/*
 * Copyright (C) 2008 Apple Ltd.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
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
#include "AccessibilityObject.h"
#include "RenderObject.h"
#include "RenderText.h"

#include <glib-object.h>

#if HAVE(ACCESSIBILITY)

namespace WebCore {

bool AccessibilityObject::accessibilityIgnoreAttachment() const
{
    return false;
}

AccessibilityObjectInclusion AccessibilityObject::accessibilityPlatformIncludesObject() const
{
    AccessibilityObject* parent = parentObject();
    if (!parent)
        return DefaultBehavior;

    AccessibilityRole role = roleValue();
    if (role == SplitterRole)
        return IncludeObject;

    // We expose the slider as a whole but not its value indicator.
    if (role == SliderThumbRole)
        return IgnoreObject;

    // When a list item is made up entirely of children (e.g. paragraphs)
    // the list item gets ignored. We need it.
    if (isGroup() && parent->isList())
        return IncludeObject;

    // Entries and password fields have extraneous children which we want to ignore.
    if (parent->isPasswordField() || parent->isTextControl())
        return IgnoreObject;

    // Include all tables, even layout tables. The AT can decide what to do with each.
    if (role == CellRole || role == TableRole)
        return IncludeObject;

    // The object containing the text should implement AtkText itself.
    if (role == StaticTextRole)
        return IgnoreObject;

    // Include all list items, regardless they have or not inline children
    if (role == ListItemRole)
        return IncludeObject;

    // Bullets/numbers for list items shouldn't be exposed as AtkObjects.
    if (role == ListMarkerRole)
        return IgnoreObject;

    return DefaultBehavior;
}

AccessibilityObjectWrapper* AccessibilityObject::wrapper() const
{
    return m_wrapper;
}

void AccessibilityObject::setWrapper(AccessibilityObjectWrapper* wrapper)
{
    if (wrapper == m_wrapper)
        return;

    if (m_wrapper)
        g_object_unref(m_wrapper);

    m_wrapper = wrapper;

    if (m_wrapper)
        g_object_ref(m_wrapper);
}

bool AccessibilityObject::allowsTextRanges() const
{
    // Check type for the AccessibilityObject.
    if (isTextControl() || isWebArea() || isGroup() || isLink() || isHeading() || isListItem())
        return true;

    // Check roles as the last fallback mechanism.
    AccessibilityRole role = roleValue();
    return role == ParagraphRole || role == LabelRole || role == DivRole || role == FormRole;
}

unsigned AccessibilityObject::getLengthForTextRange() const
{
    unsigned textLength = text().length();

    if (textLength)
        return textLength;

    // Gtk ATs need this for all text objects; not just text controls.
    Node* node = this->node();
    RenderObject* renderer = node ? node->renderer() : 0;
    if (renderer && renderer->isText()) {
        RenderText* renderText = toRenderText(renderer);
        textLength = renderText ? renderText->textLength() : 0;
    }

    // Get the text length from the elements under the
    // accessibility object if the value is still zero.
    if (!textLength && allowsTextRanges())
        textLength = textUnderElement().length();

    return textLength;
}

} // namespace WebCore

#endif // HAVE(ACCESSIBILITY)
