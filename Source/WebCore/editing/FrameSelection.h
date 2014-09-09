/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef FrameSelection_h
#define FrameSelection_h

#include "EditingStyle.h"
#include "IntRect.h"
#include "LayoutRect.h"
#include "Range.h"
#include "ScrollBehavior.h"
#include "Timer.h"
#include "VisibleSelection.h"
#include <wtf/Noncopyable.h>

#if PLATFORM(IOS)
#include "Color.h"
#endif

namespace WebCore {

class CharacterData;
class Frame;
class GraphicsContext;
class HTMLFormElement;
class MutableStyleProperties;
class RenderObject;
class RenderView;
class Settings;
class VisiblePosition;

enum EUserTriggered { NotUserTriggered = 0, UserTriggered = 1 };

enum RevealExtentOption {
    RevealExtent,
    DoNotRevealExtent
};

class CaretBase {
    WTF_MAKE_NONCOPYABLE(CaretBase);
    WTF_MAKE_FAST_ALLOCATED;
protected:
    enum CaretVisibility { Visible, Hidden };
    explicit CaretBase(CaretVisibility = Hidden);

    void invalidateCaretRect(Node*, bool caretRectChanged = false);
    void clearCaretRect();
    bool updateCaretRect(Document*, const VisiblePosition& caretPosition);
    bool shouldRepaintCaret(const RenderView*, bool isContentEditable) const;
    void paintCaret(Node*, GraphicsContext*, const LayoutPoint&, const LayoutRect& clipRect) const;

    const LayoutRect& localCaretRectWithoutUpdate() const { return m_caretLocalRect; }

    bool shouldUpdateCaretRect() const { return m_caretRectNeedsUpdate; }
    void setCaretRectNeedsUpdate() { m_caretRectNeedsUpdate = true; }

    void setCaretVisibility(CaretVisibility visibility) { m_caretVisibility = visibility; }
    bool caretIsVisible() const { return m_caretVisibility == Visible; }
    CaretVisibility caretVisibility() const { return m_caretVisibility; }

private:
    LayoutRect m_caretLocalRect; // caret rect in coords local to the renderer responsible for painting the caret
    bool m_caretRectNeedsUpdate; // true if m_caretRect (and m_absCaretBounds in FrameSelection) need to be calculated
    CaretVisibility m_caretVisibility;
};

class DragCaretController : private CaretBase {
    WTF_MAKE_NONCOPYABLE(DragCaretController);
    WTF_MAKE_FAST_ALLOCATED;
public:
    DragCaretController();

    RenderObject* caretRenderer() const;
    void paintDragCaret(Frame*, GraphicsContext*, const LayoutPoint&, const LayoutRect& clipRect) const;

    bool isContentEditable() const { return m_position.rootEditableElement(); }
    bool isContentRichlyEditable() const;

    bool hasCaret() const { return m_position.isNotNull(); }
    const VisiblePosition& caretPosition() { return m_position; }
    void setCaretPosition(const VisiblePosition&);
    void clear() { setCaretPosition(VisiblePosition()); }

    void nodeWillBeRemoved(Node*);

private:
    VisiblePosition m_position;
};

class FrameSelection : private CaretBase {
    WTF_MAKE_NONCOPYABLE(FrameSelection);
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum EAlteration { AlterationMove, AlterationExtend };
    enum CursorAlignOnScroll { AlignCursorOnScrollIfNeeded,
                               AlignCursorOnScrollAlways };
    enum SetSelectionOption {
        FireSelectEvent = 1 << 0,
        CloseTyping = 1 << 1,
        ClearTypingStyle = 1 << 2,
        SpellCorrectionTriggered = 1 << 3,
        DoNotSetFocus = 1 << 4,
        DictationTriggered = 1 << 5,
        RevealSelection = 1 << 6,
    };
    typedef unsigned SetSelectionOptions; // Union of values in SetSelectionOption and EUserTriggered
    static inline SetSelectionOptions defaultSetSelectionOptions(EUserTriggered userTriggered = NotUserTriggered)
    {
        return CloseTyping | ClearTypingStyle | (userTriggered ? (RevealSelection | FireSelectEvent) : 0);
    }

    explicit FrameSelection(Frame* = 0);

    Element* rootEditableElementOrDocumentElement() const;
     
    void moveTo(const Range*);
    void moveTo(const VisiblePosition&, EUserTriggered = NotUserTriggered, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded);
    void moveTo(const VisiblePosition&, const VisiblePosition&, EUserTriggered = NotUserTriggered);
    void moveTo(const Position&, EAffinity, EUserTriggered = NotUserTriggered);
    void moveTo(const Position&, const Position&, EAffinity, EUserTriggered = NotUserTriggered);
    void moveWithoutValidationTo(const Position&, const Position&, bool selectionHasDirection, bool shouldSetFocus);

    const VisibleSelection& selection() const { return m_selection; }
    void setSelection(const VisibleSelection&, SetSelectionOptions = defaultSetSelectionOptions(), CursorAlignOnScroll = AlignCursorOnScrollIfNeeded, TextGranularity = CharacterGranularity);
    bool setSelectedRange(Range*, EAffinity, bool closeTyping);
    void selectAll();
    void clear();
    void prepareForDestruction();

    void didLayout();
    void setNeedsSelectionUpdate();

    bool contains(const LayoutPoint&);

    bool modify(EAlteration, SelectionDirection, TextGranularity, EUserTriggered = NotUserTriggered);
    enum VerticalDirection { DirectionUp, DirectionDown };
    bool modify(EAlteration, unsigned verticalDistance, VerticalDirection, EUserTriggered = NotUserTriggered, CursorAlignOnScroll = AlignCursorOnScrollIfNeeded);

    TextGranularity granularity() const { return m_granularity; }

    void setStart(const VisiblePosition &, EUserTriggered = NotUserTriggered);
    void setEnd(const VisiblePosition &, EUserTriggered = NotUserTriggered);
    
    void setBase(const VisiblePosition&, EUserTriggered = NotUserTriggered);
    void setBase(const Position&, EAffinity, EUserTriggered = NotUserTriggered);
    void setExtent(const VisiblePosition&, EUserTriggered = NotUserTriggered);
    void setExtent(const Position&, EAffinity, EUserTriggered = NotUserTriggered);

    // Return the renderer that is responsible for painting the caret (in the selection start node)
    RenderObject* caretRendererWithoutUpdatingLayout() const;

    // Bounds of (possibly transformed) caret in absolute coords
    IntRect absoluteCaretBounds();
    void setCaretRectNeedsUpdate() { CaretBase::setCaretRectNeedsUpdate(); }

    void willBeModified(EAlteration, SelectionDirection);

    bool isNone() const { return m_selection.isNone(); }
    bool isCaret() const { return m_selection.isCaret(); }
    bool isRange() const { return m_selection.isRange(); }
    bool isCaretOrRange() const { return m_selection.isCaretOrRange(); }
    bool isAll(EditingBoundaryCrossingRule rule = CannotCrossEditingBoundary) const { return m_selection.isAll(rule); }
    
    PassRefPtr<Range> toNormalizedRange() const { return m_selection.toNormalizedRange(); }

    void debugRenderer(RenderObject*, bool selected) const;

    void nodeWillBeRemoved(Node*);
    void textWasReplaced(CharacterData*, unsigned offset, unsigned oldLength, unsigned newLength);

    void setCaretVisible(bool caretIsVisible) { setCaretVisibility(caretIsVisible ? Visible : Hidden); }
    void paintCaret(GraphicsContext*, const LayoutPoint&, const LayoutRect& clipRect);

    // Used to suspend caret blinking while the mouse is down.
    void setCaretBlinkingSuspended(bool suspended) { m_isCaretBlinkingSuspended = suspended; }
    bool isCaretBlinkingSuspended() const { return m_isCaretBlinkingSuspended; }

    // Focus
    void setFocused(bool);
    bool isFocused() const { return m_focused; }
    bool isFocusedAndActive() const;
    void pageActivationChanged();

    // Painting.
    void updateAppearance();

#ifndef NDEBUG
    void formatForDebugger(char* buffer, unsigned length) const;
    void showTreeForThis() const;
#endif

#if PLATFORM(IOS)
public:
    void expandSelectionToElementContainingCaretSelection();
    PassRefPtr<Range> elementRangeContainingCaretSelection() const;
    void expandSelectionToWordContainingCaretSelection();
    PassRefPtr<Range> wordRangeContainingCaretSelection();
    void expandSelectionToStartOfWordContainingCaretSelection();
    UChar characterInRelationToCaretSelection(int amount) const;
    UChar characterBeforeCaretSelection() const;
    UChar characterAfterCaretSelection() const;
    int wordOffsetInRange(const Range*) const;
    bool spaceFollowsWordInRange(const Range*) const;
    bool selectionAtDocumentStart() const;
    bool selectionAtSentenceStart() const;
    bool selectionAtWordStart() const;
    PassRefPtr<Range> rangeByMovingCurrentSelection(int amount) const;
    PassRefPtr<Range> rangeByExtendingCurrentSelection(int amount) const;
    void selectRangeOnElement(unsigned location, unsigned length, Node*);
    void clearCurrentSelection();
    void setCaretBlinks(bool caretBlinks = true);
    void setCaretColor(const Color&);
    static VisibleSelection wordSelectionContainingCaretSelection(const VisibleSelection&);
    void setUpdateAppearanceEnabled(bool enabled) { m_updateAppearanceEnabled = enabled; }
    void suppressScrolling() { ++m_scrollingSuppressCount; }
    void restoreScrolling()
    {
        ASSERT(m_scrollingSuppressCount);
        --m_scrollingSuppressCount;
    }
private:
    bool actualSelectionAtSentenceStart(const VisibleSelection&) const;
    PassRefPtr<Range> rangeByAlteringCurrentSelection(EAlteration, int amount) const;
public:
#endif

    bool shouldChangeSelection(const VisibleSelection&) const;
    bool shouldDeleteSelection(const VisibleSelection&) const;
    enum EndPointsAdjustmentMode { AdjustEndpointsAtBidiBoundary, DoNotAdjsutEndpoints };
    void setSelectionByMouseIfDifferent(const VisibleSelection&, TextGranularity, EndPointsAdjustmentMode = DoNotAdjsutEndpoints);

    EditingStyle* typingStyle() const;
    PassRefPtr<MutableStyleProperties> copyTypingStyle() const;
    void setTypingStyle(PassRefPtr<EditingStyle>);
    void clearTypingStyle();

    FloatRect selectionBounds(bool clipToVisibleContent = true) const;

    void getClippedVisibleTextRectangles(Vector<FloatRect>&) const;

    HTMLFormElement* currentForm() const;

    void revealSelection(const ScrollAlignment& = ScrollAlignment::alignCenterIfNeeded, RevealExtentOption = DoNotRevealExtent);
    void setSelectionFromNone();

    bool shouldShowBlockCursor() const { return m_shouldShowBlockCursor; }
    void setShouldShowBlockCursor(bool);

private:
    enum EPositionType { START, END, BASE, EXTENT };

    void updateAndRevealSelection();
    void updateDataDetectorsForSelection();

    bool setSelectionWithoutUpdatingAppearance(const VisibleSelection&, SetSelectionOptions, CursorAlignOnScroll, TextGranularity);

    void respondToNodeModification(Node*, bool baseRemoved, bool extentRemoved, bool startRemoved, bool endRemoved);
    TextDirection directionOfEnclosingBlock();
    TextDirection directionOfSelection();

    VisiblePosition positionForPlatform(bool isGetStart) const;
    VisiblePosition startForPlatform() const;
    VisiblePosition endForPlatform() const;
    VisiblePosition nextWordPositionForPlatform(const VisiblePosition&);

    VisiblePosition modifyExtendingRight(TextGranularity);
    VisiblePosition modifyExtendingForward(TextGranularity);
    VisiblePosition modifyMovingRight(TextGranularity);
    VisiblePosition modifyMovingForward(TextGranularity);
    VisiblePosition modifyExtendingLeft(TextGranularity);
    VisiblePosition modifyExtendingBackward(TextGranularity);
    VisiblePosition modifyMovingLeft(TextGranularity);
    VisiblePosition modifyMovingBackward(TextGranularity);

    LayoutUnit lineDirectionPointForBlockDirectionNavigation(EPositionType);

#if HAVE(ACCESSIBILITY)
    void notifyAccessibilityForSelectionChange();
#else
    void notifyAccessibilityForSelectionChange() { }
#endif

    void updateSelectionCachesIfSelectionIsInsideTextFormControl(EUserTriggered);

    void selectFrameElementInParentIfFullySelected();

    void setFocusedElementIfNeeded();
    void focusedOrActiveStateChanged();

    void caretBlinkTimerFired(Timer<FrameSelection>&);

    void setCaretVisibility(CaretVisibility);
    bool recomputeCaretRect();
    void invalidateCaretRect();

    bool dispatchSelectStart();

    Frame* m_frame;

    LayoutUnit m_xPosForVerticalArrowNavigation;

    VisibleSelection m_selection;
    VisiblePosition m_originalBase; // Used to store base before the adjustment at bidi boundary
    TextGranularity m_granularity;

    RefPtr<Node> m_previousCaretNode; // The last node which painted the caret. Retained for clearing the old caret when it moves.

    RefPtr<EditingStyle> m_typingStyle;

    Timer<FrameSelection> m_caretBlinkTimer;
    // The painted bounds of the caret in absolute coordinates
    IntRect m_absCaretBounds;
    bool m_absCaretBoundsDirty : 1;
    bool m_caretPaint : 1;
    bool m_isCaretBlinkingSuspended : 1;
    bool m_focused : 1;
    bool m_shouldShowBlockCursor : 1;
    bool m_pendingSelectionUpdate : 1;
    bool m_shouldRevealSelection : 1;
    bool m_alwaysAlignCursorOnScrollWhenRevealingSelection : 1;

#if PLATFORM(IOS)
    bool m_updateAppearanceEnabled : 1;
    bool m_caretBlinks : 1;
    Color m_caretColor;
    int m_scrollingSuppressCount;
#endif
};

inline EditingStyle* FrameSelection::typingStyle() const
{
    return m_typingStyle.get();
}

inline void FrameSelection::clearTypingStyle()
{
    m_typingStyle.clear();
}

inline void FrameSelection::setTypingStyle(PassRefPtr<EditingStyle> style)
{
    m_typingStyle = style;
}

#if !(PLATFORM(COCOA) || PLATFORM(GTK) || PLATFORM(EFL))
#if HAVE(ACCESSIBILITY)
inline void FrameSelection::notifyAccessibilityForSelectionChange()
{
}
#endif
#endif

} // namespace WebCore

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showTree(const WebCore::FrameSelection&);
void showTree(const WebCore::FrameSelection*);
#endif

#endif // FrameSelection_h
