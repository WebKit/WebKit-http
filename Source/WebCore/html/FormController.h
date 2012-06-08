/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010, 2011, 2012 Google Inc. All rights reserved.
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

#ifndef FormController_h
#define FormController_h

#include "CheckedRadioButtons.h"
#include <wtf/Forward.h>
#include <wtf/ListHashSet.h>
#include <wtf/Vector.h>

namespace WebCore {

class FormAssociatedElement;
class HTMLFormControlElementWithState;

class FormElementKey {
public:
    FormElementKey(AtomicStringImpl* = 0, AtomicStringImpl* = 0);
    ~FormElementKey();
    FormElementKey(const FormElementKey&);
    FormElementKey& operator=(const FormElementKey&);

    AtomicStringImpl* name() const { return m_name; }
    AtomicStringImpl* type() const { return m_type; }

    // Hash table deleted values, which are only constructed and never copied or destroyed.
    FormElementKey(WTF::HashTableDeletedValueType) : m_name(hashTableDeletedValue()) { }
    bool isHashTableDeletedValue() const { return m_name == hashTableDeletedValue(); }

private:
    void ref() const;
    void deref() const;

    static AtomicStringImpl* hashTableDeletedValue() { return reinterpret_cast<AtomicStringImpl*>(-1); }

    AtomicStringImpl* m_name;
    AtomicStringImpl* m_type;
};

inline bool operator==(const FormElementKey& a, const FormElementKey& b)
{
    return a.name() == b.name() && a.type() == b.type();
}

struct FormElementKeyHash {
    static unsigned hash(const FormElementKey&);
    static bool equal(const FormElementKey& a, const FormElementKey& b) { return a == b; }
    static const bool safeToCompareToEmptyOrDeleted = true;
};

struct FormElementKeyHashTraits : WTF::GenericHashTraits<FormElementKey> {
    static void constructDeletedValue(FormElementKey& slot) { new (NotNull, &slot) FormElementKey(WTF::HashTableDeletedValue); }
    static bool isDeletedValue(const FormElementKey& value) { return value.isHashTableDeletedValue(); }
};

class FormController {
public:
    static PassOwnPtr<FormController> create()
    {
        return adoptPtr(new FormController);
    }
    ~FormController();

    CheckedRadioButtons& checkedRadioButtons() { return m_checkedRadioButtons; }
    
    void registerFormElementWithState(HTMLFormControlElementWithState* control) { m_formElementsWithState.add(control); }
    void unregisterFormElementWithState(HTMLFormControlElementWithState* control) { m_formElementsWithState.remove(control); }
    // This should be callled only by Document::formElementsState().
    Vector<String> formElementsState() const;
    // This should be callled only by Document::setStateForNewFormElements().
    void setStateForNewFormElements(const Vector<String>&);
    bool hasStateForNewFormElements() const;
    bool takeStateForFormElement(AtomicStringImpl* name, AtomicStringImpl* type, String& state);

    void registerFormElementWithFormAttribute(FormAssociatedElement*);
    void unregisterFormElementWithFormAttribute(FormAssociatedElement*);
    void resetFormElementsOwner();

private:
    FormController();

    CheckedRadioButtons m_checkedRadioButtons;

    typedef ListHashSet<HTMLFormControlElementWithState*, 64> FormElementListHashSet;
    FormElementListHashSet m_formElementsWithState;
    typedef ListHashSet<RefPtr<FormAssociatedElement>, 32> FormAssociatedElementListHashSet;
    FormAssociatedElementListHashSet m_formElementsWithFormAttribute;

    typedef HashMap<FormElementKey, Vector<String>, FormElementKeyHash, FormElementKeyHashTraits> FormElementStateMap;
    FormElementStateMap m_stateForNewFormElements;
    
};

} // namespace WebCore
#endif
