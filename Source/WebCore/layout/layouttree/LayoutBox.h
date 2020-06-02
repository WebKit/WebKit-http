/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "LayoutUnits.h"
#include "RenderStyle.h"
#include <wtf/IsoMalloc.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

namespace Display {
class Box;
}

namespace Layout {

class ContainerBox;
class InitialContainingBlock;
class LayoutState;
class TreeBuilder;

class Box : public CanMakeWeakPtr<Box> {
    WTF_MAKE_ISO_ALLOCATED(Box);
public:
    enum class ElementType {
        Document,
        Body,
        TableWrapperBox, // The table generates a principal block container box called the table wrapper box that contains the table box and any caption boxes.
        TableBox, // The table box is a block-level box that contains the table's internal table boxes.
        Image,
        IFrame,
        GenericElement
    };

    struct ElementAttributes {
        ElementType elementType;
    };

    enum BaseTypeFlag {
        BoxFlag                    = 1 << 0,
        InlineTextBoxFlag          = 1 << 1,
        LineBreakBoxFlag           = 1 << 2,
        ReplacedBoxFlag            = 1 << 3,
        InitialContainingBlockFlag = 1 << 4,
        ContainerBoxFlag           = 1 << 5
    };
    typedef unsigned BaseTypeFlags;

    virtual ~Box();

    bool establishesFormattingContext() const;
    bool establishesBlockFormattingContext() const;
    bool establishesInlineFormattingContext() const;
    bool establishesTableFormattingContext() const;
    bool establishesIndependentFormattingContext() const;

    bool isInFlow() const { return !isFloatingOrOutOfFlowPositioned(); }
    bool isPositioned() const { return isInFlowPositioned() || isOutOfFlowPositioned(); }
    bool isInFlowPositioned() const { return isRelativelyPositioned() || isStickyPositioned(); }
    bool isOutOfFlowPositioned() const { return isAbsolutelyPositioned(); }
    bool isRelativelyPositioned() const;
    bool isStickyPositioned() const;
    bool isAbsolutelyPositioned() const;
    bool isFixedPositioned() const;
    bool isFloatingPositioned() const;
    bool isLeftFloatingPositioned() const;
    bool isRightFloatingPositioned() const;
    bool hasFloatClear() const;
    bool isFloatAvoider() const;

    bool isFloatingOrOutOfFlowPositioned() const { return isFloatingPositioned() || isOutOfFlowPositioned(); }

    const ContainerBox& containingBlock() const;
    const ContainerBox& formattingContextRoot() const;
    const InitialContainingBlock& initialContainingBlock() const;

    bool isInFormattingContextOf(const ContainerBox&) const;

    bool isAnonymous() const { return m_isAnonymous; }

    bool isBlockLevelBox() const;
    bool isInlineLevelBox() const;
    bool isInlineBox() const;
    bool isAtomicInlineLevelBox() const;
    bool isInlineBlockBox() const;
    bool isInlineTableBox() const;
    bool isBlockContainerBox() const;
    bool isInitialContainingBlock() const { return m_baseTypeFlags & InitialContainingBlockFlag; }

    bool isDocumentBox() const { return m_elementAttributes && m_elementAttributes.value().elementType == ElementType::Document; }
    bool isBodyBox() const { return m_elementAttributes && m_elementAttributes.value().elementType == ElementType::Body; }
    bool isTableWrapperBox() const { return m_elementAttributes && m_elementAttributes.value().elementType == ElementType::TableWrapperBox; }
    bool isTableBox() const { return m_elementAttributes && m_elementAttributes.value().elementType == ElementType::TableBox; }
    bool isTableCaption() const { return style().display() == DisplayType::TableCaption; }
    bool isTableHeader() const { return style().display() == DisplayType::TableHeaderGroup; }
    bool isTableBody() const { return style().display() == DisplayType::TableRowGroup; }
    bool isTableFooter() const { return style().display() == DisplayType::TableFooterGroup; }
    bool isTableRow() const { return style().display() == DisplayType::TableRow; }
    bool isTableColumnGroup() const { return style().display() == DisplayType::TableColumnGroup; }
    bool isTableColumn() const { return style().display() == DisplayType::TableColumn; }
    bool isTableCell() const { return style().display() == DisplayType::TableCell; }
    bool isIFrame() const { return m_elementAttributes && m_elementAttributes.value().elementType == ElementType::IFrame; }
    bool isImage() const { return m_elementAttributes && m_elementAttributes.value().elementType == ElementType::Image; }

    const ContainerBox& parent() const { return *m_parent; }
    const Box* nextSibling() const { return m_nextSibling; }
    const Box* nextInFlowSibling() const;
    const Box* nextInFlowOrFloatingSibling() const;
    const Box* previousSibling() const { return m_previousSibling; }
    const Box* previousInFlowSibling() const;
    const Box* previousInFlowOrFloatingSibling() const;

    // FIXME: This is currently needed for style updates.
    Box* nextSibling() { return m_nextSibling; }

    bool isContainerBox() const { return m_baseTypeFlags & ContainerBoxFlag; }
    bool isInlineTextBox() const { return m_baseTypeFlags & InlineTextBoxFlag; }
    bool isLineBreakBox() const { return m_baseTypeFlags & LineBreakBoxFlag; }
    bool isReplacedBox() const { return m_baseTypeFlags & ReplacedBoxFlag; }

    bool isPaddingApplicable() const;
    bool isOverflowVisible() const;

    void updateStyle(const RenderStyle& newStyle);
    const RenderStyle& style() const { return m_style; }

    // FIXME: Find a better place for random DOM things.
    void setRowSpan(size_t);
    size_t rowSpan() const;

    void setColumnSpan(size_t);
    size_t columnSpan() const;

    void setColumnWidth(LayoutUnit);
    Optional<LayoutUnit> columnWidth() const;

    void setParent(ContainerBox& parent) { m_parent = &parent; }
    void setNextSibling(Box& nextSibling) { m_nextSibling = &nextSibling; }
    void setPreviousSibling(Box& previousSibling) { m_previousSibling = &previousSibling; }

    void setIsAnonymous() { m_isAnonymous = true; }

    bool canCacheForLayoutState(const LayoutState&) const;
    Display::Box* cachedDisplayBoxForLayoutState(const LayoutState&) const;
    void setCachedDisplayBoxForLayoutState(LayoutState&, std::unique_ptr<Display::Box>) const;

protected:
    Box(Optional<ElementAttributes>, RenderStyle&&, BaseTypeFlags);

private:
    class BoxRareData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        BoxRareData() = default;

        CellSpan tableCellSpan;
        Optional<LayoutUnit> columnWidth;
    };

    bool hasRareData() const { return m_hasRareData; }
    void setHasRareData(bool hasRareData) { m_hasRareData = hasRareData; }
    const BoxRareData& rareData() const;
    BoxRareData& ensureRareData();
    void removeRareData();

    typedef HashMap<const Box*, std::unique_ptr<BoxRareData>> RareDataMap;

    static RareDataMap& rareDataMap();

    RenderStyle m_style;
    Optional<ElementAttributes> m_elementAttributes;

    ContainerBox* m_parent { nullptr };
    Box* m_previousSibling { nullptr };
    Box* m_nextSibling { nullptr };
    
    // First LayoutState gets a direct cache.
    mutable WeakPtr<LayoutState> m_cachedLayoutState;
    mutable std::unique_ptr<Display::Box> m_cachedDisplayBoxForLayoutState;

    unsigned m_baseTypeFlags : 6;
    bool m_hasRareData : 1;
    bool m_isAnonymous : 1;
};

}
}

#define SPECIALIZE_TYPE_TRAITS_LAYOUT_BOX(ToValueTypeName, predicate) \
SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::Layout::ToValueTypeName) \
    static bool isType(const WebCore::Layout::Box& box) { return box.predicate; } \
SPECIALIZE_TYPE_TRAITS_END()

#endif
