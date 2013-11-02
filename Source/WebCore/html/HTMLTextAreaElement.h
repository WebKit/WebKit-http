/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Apple Inc. All rights reserved.
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

#ifndef HTMLTextAreaElement_h
#define HTMLTextAreaElement_h

#include "HTMLTextFormControlElement.h"

namespace WebCore {

class BeforeTextInsertedEvent;
class VisibleSelection;

class HTMLTextAreaElement FINAL : public HTMLTextFormControlElement {
public:
    static PassRefPtr<HTMLTextAreaElement> create(const QualifiedName&, Document&, HTMLFormElement*);

    int cols() const { return m_cols; }
    int rows() const { return m_rows; }

    bool shouldWrapText() const { return m_wrap != NoWrap; }

    virtual String value() const OVERRIDE;
    void setValue(const String&);
    String defaultValue() const;
    void setDefaultValue(const String&);
    int textLength() const { return value().length(); }
    virtual int maxLength() const OVERRIDE;
    void setMaxLength(int, ExceptionCode&);
    // For ValidityState
    virtual String validationMessage() const OVERRIDE;
    virtual bool valueMissing() const OVERRIDE;
    virtual bool tooLong() const OVERRIDE;
    bool isValidValue(const String&) const;
    
    virtual TextControlInnerTextElement* innerTextElement() const OVERRIDE;

    void rendererWillBeDestroyed();

    void setCols(int);
    void setRows(int);

private:
    HTMLTextAreaElement(const QualifiedName&, Document&, HTMLFormElement*);

    enum WrapMethod { NoWrap, SoftWrap, HardWrap };

    virtual void didAddUserAgentShadowRoot(ShadowRoot*) OVERRIDE;
    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    void handleBeforeTextInsertedEvent(BeforeTextInsertedEvent*) const;
    static String sanitizeUserInputValue(const String&, unsigned maxLength);
    void updateValue() const;
    void setNonDirtyValue(const String&);
    void setValueCommon(const String&);

    virtual bool supportsPlaceholder() const OVERRIDE { return true; }
    virtual HTMLElement* placeholderElement() const OVERRIDE;
    virtual void updatePlaceholderText() OVERRIDE;
    virtual bool isEmptyValue() const OVERRIDE { return value().isEmpty(); }

    virtual bool isOptionalFormControl() const OVERRIDE { return !isRequiredFormControl(); }
    virtual bool isRequiredFormControl() const OVERRIDE { return isRequired(); }

    virtual void defaultEventHandler(Event*) OVERRIDE;
    
    virtual void subtreeHasChanged() OVERRIDE;

    virtual bool isEnumeratable() const OVERRIDE { return true; }
    virtual bool supportLabels() const OVERRIDE { return true; }

    virtual const AtomicString& formControlType() const OVERRIDE;

    virtual FormControlState saveFormControlState() const OVERRIDE;
    virtual void restoreFormControlState(const FormControlState&) OVERRIDE;

    virtual bool isTextFormControl() const OVERRIDE { return true; }

    virtual void childrenChanged(const ChildChange&) OVERRIDE;
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;
    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;
    virtual void collectStyleForPresentationAttribute(const QualifiedName&, const AtomicString&, MutableStylePropertySet*) OVERRIDE;
    virtual RenderElement* createRenderer(PassRef<RenderStyle>) OVERRIDE;
    virtual bool appendFormData(FormDataList&, bool) OVERRIDE;
    virtual void reset() OVERRIDE;
    virtual bool hasCustomFocusLogic() const OVERRIDE;
    virtual bool isMouseFocusable() const OVERRIDE;
    virtual bool isKeyboardFocusable(KeyboardEvent*) const OVERRIDE;
    virtual void updateFocusAppearance(bool restorePreviousSelection) OVERRIDE;

    virtual void accessKeyAction(bool sendMouseEvents) OVERRIDE;

    virtual bool shouldUseInputMethod() OVERRIDE;
    virtual bool matchesReadOnlyPseudoClass() const OVERRIDE;
    virtual bool matchesReadWritePseudoClass() const OVERRIDE;

    bool valueMissing(const String& value) const { return isRequiredFormControl() && !isDisabledOrReadOnly() && value.isEmpty(); }
    bool tooLong(const String&, NeedsToCheckDirtyFlag) const;

    int m_rows;
    int m_cols;
    WrapMethod m_wrap;
    HTMLElement* m_placeholder;
    mutable String m_value;
    mutable bool m_isDirty;
    mutable bool m_wasModifiedByUser;
};

NODE_TYPE_CASTS(HTMLTextAreaElement)

} //namespace

#endif
