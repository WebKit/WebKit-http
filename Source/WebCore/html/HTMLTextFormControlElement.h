/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2009, 2010, 2011 Google Inc. All rights reserved.
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

#ifndef HTMLTextFormControlElement_h
#define HTMLTextFormControlElement_h

#include "HTMLFormControlElementWithState.h"

namespace WebCore {

class Position;
class RenderTextControl;
class TextControlInnerTextElement;
class VisiblePosition;

enum TextFieldSelectionDirection { SelectionHasNoDirection, SelectionHasForwardDirection, SelectionHasBackwardDirection };
enum TextFieldEventBehavior { DispatchNoEvent, DispatchChangeEvent, DispatchInputAndChangeEvent };

class HTMLTextFormControlElement : public HTMLFormControlElementWithState {
public:
    // Common flag for HTMLInputElement::tooLong() and HTMLTextAreaElement::tooLong().
    enum NeedsToCheckDirtyFlag {CheckDirtyFlag, IgnoreDirtyFlag};

    virtual ~HTMLTextFormControlElement();

    void didEditInnerTextValue();
    void forwardEvent(Event*);

    virtual InsertionNotificationRequest insertedInto(ContainerNode&) override;

    // The derived class should return true if placeholder processing is needed.
    virtual bool supportsPlaceholder() const = 0;
    String strippedPlaceholder() const;
    bool placeholderShouldBeVisible() const;
    virtual HTMLElement* placeholderElement() const = 0;
    void updatePlaceholderVisibility(bool);

    int indexForVisiblePosition(const VisiblePosition&) const;
    VisiblePosition visiblePositionForIndex(int index) const;
    int selectionStart() const;
    int selectionEnd() const;
    const AtomicString& selectionDirection() const;
    void setSelectionStart(int);
    void setSelectionEnd(int);
    void setSelectionDirection(const String&);
    void select();
    virtual void setRangeText(const String& replacement, ExceptionCode&);
    virtual void setRangeText(const String& replacement, unsigned start, unsigned end, const String& selectionMode, ExceptionCode&);
    void setSelectionRange(int start, int end, const String& direction);
    void setSelectionRange(int start, int end, TextFieldSelectionDirection = SelectionHasNoDirection);
    PassRefPtr<Range> selection() const;
    String selectedText() const;

    virtual void dispatchFormControlChangeEvent() override final;

    virtual int maxLength() const = 0;
    virtual String value() const = 0;

    virtual TextControlInnerTextElement* innerTextElement() const = 0;

    void selectionChanged(bool shouldFireSelectEvent);
    bool lastChangeWasUserEdit() const;
    void setInnerTextValue(const String&);
    String innerTextValue() const;

    String directionForFormData() const;

    void setTextAsOfLastFormControlChangeEvent(const String& text) { m_textAsOfLastFormControlChangeEvent = text; }
#if PLATFORM(IOS)
    void hidePlaceholder();
    void showPlaceholderIfNecessary();
#endif

protected:
    HTMLTextFormControlElement(const QualifiedName&, Document&, HTMLFormElement*);
    bool isPlaceholderEmpty() const;
    virtual void updatePlaceholderText() = 0;

    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;

    virtual void disabledStateChanged() override;
    virtual void readOnlyAttributeChanged() override;
    void updateInnerTextElementEditability();

    void cacheSelection(int start, int end, TextFieldSelectionDirection direction)
    {
        m_cachedSelectionStart = start;
        m_cachedSelectionEnd = end;
        m_cachedSelectionDirection = direction;
    }

    void restoreCachedSelection();
    bool hasCachedSelection() const { return m_cachedSelectionStart >= 0; }

    virtual void subtreeHasChanged() = 0;

    void setLastChangeWasNotUserEdit() { m_lastChangeWasUserEdit = false; }

    String valueWithHardLineBreaks() const;

private:
    int computeSelectionStart() const;
    int computeSelectionEnd() const;
    TextFieldSelectionDirection computeSelectionDirection() const;

    virtual void dispatchFocusEvent(PassRefPtr<Element> oldFocusedElement, FocusDirection) override final;
    virtual void dispatchBlurEvent(PassRefPtr<Element> newFocusedElement) override final;
    virtual bool childShouldCreateRenderer(const Node&) const override;

    unsigned indexForPosition(const Position&) const;

    // Returns true if user-editable value is empty. Used to check placeholder visibility.
    virtual bool isEmptyValue() const = 0;
    // Returns true if suggested value is empty. Used to check placeholder visibility.
    virtual bool isEmptySuggestedValue() const { return true; }
    // Called in dispatchFocusEvent(), after placeholder process, before calling parent's dispatchFocusEvent().
    virtual void handleFocusEvent(Node* /* oldFocusedNode */, FocusDirection) { }
    // Called in dispatchBlurEvent(), after placeholder process, before calling parent's dispatchBlurEvent().
    virtual void handleBlurEvent() { }

    String m_textAsOfLastFormControlChangeEvent;
    bool m_lastChangeWasUserEdit;
    
    int m_cachedSelectionStart;
    int m_cachedSelectionEnd;
    TextFieldSelectionDirection m_cachedSelectionDirection;
};

void isHTMLTextFormControlElement(const HTMLTextFormControlElement&); // Catch unnecessary runtime check of type known at compile time.
inline bool isHTMLTextFormControlElement(const Element& element) { return element.isTextFormControl(); }
inline bool isHTMLTextFormControlElement(const Node& node) { return node.isElementNode() && toElement(node).isTextFormControl(); }
NODE_TYPE_CASTS(HTMLTextFormControlElement)

HTMLTextFormControlElement* enclosingTextFormControl(const Position&);

} // namespace

#endif
