/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "HTMLFormControlElementWithState.h"

#include "FormController.h"
#include "HTMLFormElement.h"

namespace WebCore {

HTMLFormControlElementWithState::HTMLFormControlElementWithState(const QualifiedName& tagName, Document* doc, HTMLFormElement* f)
    : HTMLFormControlElement(tagName, doc, f)
{
    document()->formController()->registerFormElementWithState(this);
}

HTMLFormControlElementWithState::~HTMLFormControlElementWithState()
{
    document()->formController()->unregisterFormElementWithState(this);
}

void HTMLFormControlElementWithState::didMoveToNewDocument(Document* oldDocument)
{
    if (oldDocument)
        oldDocument->formController()->unregisterFormElementWithState(this);
    document()->formController()->registerFormElementWithState(this);
    HTMLFormControlElement::didMoveToNewDocument(oldDocument);
}

bool HTMLFormControlElementWithState::shouldAutocomplete() const
{
    if (!form())
        return true;
    return form()->shouldAutocomplete();
}

bool HTMLFormControlElementWithState::shouldSaveAndRestoreFormControlState() const
{
    // We don't save/restore control state in a form with autocomplete=off.
    return attached() && shouldAutocomplete();
}

FormControlState HTMLFormControlElementWithState::saveFormControlState() const
{
    return FormControlState();
}

void HTMLFormControlElementWithState::finishParsingChildren()
{
    HTMLFormControlElement::finishParsingChildren();
    document()->formController()->restoreControlStateFor(*this);
}

bool HTMLFormControlElementWithState::isFormControlElementWithState() const
{
    return true;
}

} // namespace Webcore
