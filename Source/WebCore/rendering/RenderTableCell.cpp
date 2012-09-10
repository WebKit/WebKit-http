/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "RenderTableCell.h"

#include "CollapsedBorderValue.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "HTMLNames.h"
#include "HTMLTableCellElement.h"
#include "PaintInfo.h"
#include "RenderTableCol.h"
#include "RenderView.h"
#include "StyleInheritedData.h"
#include "TransformState.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderTableCell::RenderTableCell(Node* node)
    : RenderBlock(node)
    , m_column(unsetColumnIndex)
    , m_cellWidthChanged(false)
    , m_hasAssociatedTableCellElement(node && (node->hasTagName(tdTag) || node->hasTagName(thTag)))
    , m_intrinsicPaddingBefore(0)
    , m_intrinsicPaddingAfter(0)
{
}

void RenderTableCell::willBeRemovedFromTree()
{
    RenderBlock::willBeRemovedFromTree();

    section()->setNeedsCellRecalc();
    section()->removeCachedCollapsedBorders(this);
}

unsigned RenderTableCell::colSpan() const
{
    if (UNLIKELY(!m_hasAssociatedTableCellElement))
        return 1;

    return toHTMLTableCellElement(node())->colSpan();
}

unsigned RenderTableCell::rowSpan() const
{
    if (UNLIKELY(!m_hasAssociatedTableCellElement))
        return 1;

    return toHTMLTableCellElement(node())->rowSpan();
}

void RenderTableCell::colSpanOrRowSpanChanged()
{
    ASSERT(m_hasAssociatedTableCellElement);
    ASSERT(node());
    ASSERT(node()->hasTagName(tdTag) || node()->hasTagName(thTag));

    setNeedsLayoutAndPrefWidthsRecalc();
    if (parent() && section())
        section()->setNeedsCellRecalc();
}

LayoutUnit RenderTableCell::logicalHeightForRowSizing() const
{
    LayoutUnit adjustedLogicalHeight = logicalHeight() - (intrinsicPaddingBefore() + intrinsicPaddingAfter());

    LayoutUnit styleLogicalHeight = valueForLength(style()->logicalHeight(), 0, view());
    if (document()->inQuirksMode() || style()->boxSizing() == BORDER_BOX) {
        // Explicit heights use the border box in quirks mode.
        // Don't adjust height.
    } else {
        // In strict mode, box-sizing: content-box do the right
        // thing and actually add in the border and padding.
        LayoutUnit adjustedPaddingBefore = paddingBefore() - intrinsicPaddingBefore();
        LayoutUnit adjustedPaddingAfter = paddingAfter() - intrinsicPaddingAfter();
        styleLogicalHeight += adjustedPaddingBefore + adjustedPaddingAfter + borderBefore() + borderAfter();
    }

    return max(styleLogicalHeight, adjustedLogicalHeight);
}

Length RenderTableCell::styleOrColLogicalWidth() const
{
    Length w = style()->logicalWidth();
    if (!w.isAuto())
        return w;

    if (RenderTableCol* tableCol = table()->colElement(col())) {
        unsigned colSpanCount = colSpan();

        Length colWidthSum = Length(0, Fixed);
        for (unsigned i = 1; i <= colSpanCount; i++) {
            Length colWidth = tableCol->style()->logicalWidth();

            // Percentage value should be returned only for colSpan == 1.
            // Otherwise we return original width for the cell.
            if (!colWidth.isFixed()) {
                if (colSpanCount > 1)
                    return w;
                return colWidth;
            }

            colWidthSum = Length(colWidthSum.value() + colWidth.value(), Fixed);

            tableCol = tableCol->nextColumn();
            // If no next <col> tag found for the span we just return what we have for now.
            if (!tableCol)
                break;
        }

        // Column widths specified on <col> apply to the border box of the cell.
        // Percentages don't need to be handled since they're always treated this way (even when specified on the cells).
        // See Bugzilla bug 8126 for details.
        if (colWidthSum.isFixed() && colWidthSum.value() > 0)
            colWidthSum = Length(max(0.0f, colWidthSum.value() - borderAndPaddingLogicalWidth()), Fixed);
        return colWidthSum;
    }

    return w;
}

void RenderTableCell::computePreferredLogicalWidths()
{
    // The child cells rely on the grids up in the sections to do their computePreferredLogicalWidths work.  Normally the sections are set up early, as table
    // cells are added, but relayout can cause the cells to be freed, leaving stale pointers in the sections'
    // grids.  We must refresh those grids before the child cells try to use them.
    table()->recalcSectionsIfNeeded();

    RenderBlock::computePreferredLogicalWidths();
    if (node() && style()->autoWrap()) {
        // See if nowrap was set.
        Length w = styleOrColLogicalWidth();
        String nowrap = static_cast<Element*>(node())->getAttribute(nowrapAttr);
        if (!nowrap.isNull() && w.isFixed())
            // Nowrap is set, but we didn't actually use it because of the
            // fixed width set on the cell.  Even so, it is a WinIE/Moz trait
            // to make the minwidth of the cell into the fixed width.  They do this
            // even in strict mode, so do not make this a quirk.  Affected the top
            // of hiptop.com.
            m_minPreferredLogicalWidth = max<LayoutUnit>(w.value(), m_minPreferredLogicalWidth);
    }
}

void RenderTableCell::updateLogicalWidth()
{
}

void RenderTableCell::setCellLogicalWidth(LayoutUnit w)
{
    if (w == logicalWidth())
        return;

    setLogicalWidth(w);
    setCellWidthChanged(true);
}

void RenderTableCell::layout()
{
    updateFirstLetter();
    layoutBlock(cellWidthChanged());
    setCellWidthChanged(false);
}

LayoutUnit RenderTableCell::paddingTop() const
{
    int result = computedCSSPaddingTop();
    if (!isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == TopToBottomWritingMode ? intrinsicPaddingBefore() : intrinsicPaddingAfter());
}

LayoutUnit RenderTableCell::paddingBottom() const
{
    int result = computedCSSPaddingBottom();
    if (!isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == TopToBottomWritingMode ? intrinsicPaddingAfter() : intrinsicPaddingBefore());
}

LayoutUnit RenderTableCell::paddingLeft() const
{
    int result = computedCSSPaddingLeft();
    if (isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == LeftToRightWritingMode ? intrinsicPaddingBefore() : intrinsicPaddingAfter());
}

LayoutUnit RenderTableCell::paddingRight() const
{   
    int result = computedCSSPaddingRight();
    if (isHorizontalWritingMode())
        return result;
    return result + (style()->writingMode() == LeftToRightWritingMode ? intrinsicPaddingAfter() : intrinsicPaddingBefore());
}

LayoutUnit RenderTableCell::paddingBefore() const
{
    return static_cast<int>(computedCSSPaddingBefore()) + intrinsicPaddingBefore();
}

LayoutUnit RenderTableCell::paddingAfter() const
{
    return static_cast<int>(computedCSSPaddingAfter()) + intrinsicPaddingAfter();
}

void RenderTableCell::setOverrideLogicalContentHeightFromRowHeight(LayoutUnit rowHeight)
{
    clearIntrinsicPadding();
    setOverrideLogicalContentHeight(max<LayoutUnit>(0, rowHeight - borderAndPaddingLogicalHeight()));
}

LayoutSize RenderTableCell::offsetFromContainer(RenderObject* o, const LayoutPoint& point, bool* offsetDependsOnPoint) const
{
    ASSERT(o == container());

    LayoutSize offset = RenderBlock::offsetFromContainer(o, point, offsetDependsOnPoint);
    if (parent())
        offset -= parentBox()->locationOffset();

    return offset;
}

LayoutRect RenderTableCell::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer) const
{
    // If the table grid is dirty, we cannot get reliable information about adjoining cells,
    // so we ignore outside borders. This should not be a problem because it means that
    // the table is going to recalculate the grid, relayout and repaint its current rect, which
    // includes any outside borders of this cell.
    if (!table()->collapseBorders() || table()->needsSectionRecalc())
        return RenderBlock::clippedOverflowRectForRepaint(repaintContainer);

    bool rtl = !styleForCellFlow()->isLeftToRightDirection();
    int outlineSize = style()->outlineSize();
    int left = max(borderHalfLeft(true), outlineSize);
    int right = max(borderHalfRight(true), outlineSize);
    int top = max(borderHalfTop(true), outlineSize);
    int bottom = max(borderHalfBottom(true), outlineSize);
    if ((left && !rtl) || (right && rtl)) {
        if (RenderTableCell* before = table()->cellBefore(this)) {
            top = max(top, before->borderHalfTop(true));
            bottom = max(bottom, before->borderHalfBottom(true));
        }
    }
    if ((left && rtl) || (right && !rtl)) {
        if (RenderTableCell* after = table()->cellAfter(this)) {
            top = max(top, after->borderHalfTop(true));
            bottom = max(bottom, after->borderHalfBottom(true));
        }
    }
    if (top) {
        if (RenderTableCell* above = table()->cellAbove(this)) {
            left = max(left, above->borderHalfLeft(true));
            right = max(right, above->borderHalfRight(true));
        }
    }
    if (bottom) {
        if (RenderTableCell* below = table()->cellBelow(this)) {
            left = max(left, below->borderHalfLeft(true));
            right = max(right, below->borderHalfRight(true));
        }
    }
    LayoutPoint location(max<LayoutUnit>(left, -visualOverflowRect().x()), max<LayoutUnit>(top, -visualOverflowRect().y()));
    LayoutRect r(-location.x(), -location.y(), location.x() + max(width() + right, visualOverflowRect().maxX()), location.y() + max(height() + bottom, visualOverflowRect().maxY()));

    if (RenderView* v = view()) {
        // FIXME: layoutDelta needs to be applied in parts before/after transforms and
        // repaint containers. https://bugs.webkit.org/show_bug.cgi?id=23308
        r.move(v->layoutDelta());
    }
    computeRectForRepaint(repaintContainer, r);
    return r;
}

void RenderTableCell::computeRectForRepaint(RenderBoxModelObject* repaintContainer, LayoutRect& r, bool fixed) const
{
    if (repaintContainer == this)
        return;
    r.setY(r.y());
    RenderView* v = view();
    if ((!v || !v->layoutStateEnabled() || repaintContainer) && parent())
        r.moveBy(-parentBox()->location()); // Rows are in the same coordinate space, so don't add their offset in.
    RenderBlock::computeRectForRepaint(repaintContainer, r, fixed);
}

LayoutUnit RenderTableCell::cellBaselinePosition() const
{
    // <http://www.w3.org/TR/2007/CR-CSS21-20070719/tables.html#height-layout>: The baseline of a cell is the baseline of
    // the first in-flow line box in the cell, or the first in-flow table-row in the cell, whichever comes first. If there
    // is no such line box or table-row, the baseline is the bottom of content edge of the cell box.
    LayoutUnit firstLineBaseline = firstLineBoxBaseline();
    if (firstLineBaseline != -1)
        return firstLineBaseline;
    return paddingBefore() + borderBefore() + contentLogicalHeight();
}

void RenderTableCell::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    ASSERT(style()->display() == TABLE_CELL);
    ASSERT(!row() || row()->rowIndexWasSet());

    RenderBlock::styleDidChange(diff, oldStyle);
    setHasBoxDecorations(true);

    if (parent() && section() && oldStyle && style()->height() != oldStyle->height())
        section()->rowLogicalHeightChanged(rowIndex());

    // If border was changed, notify table.
    if (parent()) {
        RenderTable* table = this->table();
        if (table && !table->selfNeedsLayout() && !table->normalChildNeedsLayout()&& oldStyle && oldStyle->border() != style()->border())
            table->invalidateCollapsedBorders();
    }
}

// The following rules apply for resolving conflicts and figuring out which border
// to use.
// (1) Borders with the 'border-style' of 'hidden' take precedence over all other conflicting 
// borders. Any border with this value suppresses all borders at this location.
// (2) Borders with a style of 'none' have the lowest priority. Only if the border properties of all 
// the elements meeting at this edge are 'none' will the border be omitted (but note that 'none' is 
// the default value for the border style.)
// (3) If none of the styles are 'hidden' and at least one of them is not 'none', then narrow borders 
// are discarded in favor of wider ones. If several have the same 'border-width' then styles are preferred 
// in this order: 'double', 'solid', 'dashed', 'dotted', 'ridge', 'outset', 'groove', and the lowest: 'inset'.
// (4) If border styles differ only in color, then a style set on a cell wins over one on a row, 
// which wins over a row group, column, column group and, lastly, table. It is undefined which color 
// is used when two elements of the same type disagree.
static int compareBorders(const CollapsedBorderValue& border1, const CollapsedBorderValue& border2)
{
    // Sanity check the values passed in. The null border have lowest priority.
    if (!border2.exists()) {
        if (!border1.exists())
            return 0;
        return 1;
    }
    if (!border1.exists())
        return -1;

    // Rule #1 above.
    if (border2.style() == BHIDDEN) {
        if (border1.style() == BHIDDEN)
            return 0;
        return -1;
    }
    if (border1.style() == BHIDDEN)
        return 1;
    
    // Rule #2 above.  A style of 'none' has lowest priority and always loses to any other border.
    if (border2.style() == BNONE) {
        if (border1.style() == BNONE)
            return 0;
        return 1;
    }
    if (border1.style() == BNONE)
        return -1;

    // The first part of rule #3 above. Wider borders win.
    if (border1.width() != border2.width())
        return border1.width() < border2.width() ? -1 : 1;
    
    // The borders have equal width.  Sort by border style.
    if (border1.style() != border2.style())
        return border1.style() < border2.style() ? -1 : 1;
    
    // The border have the same width and style.  Rely on precedence (cell over row over row group, etc.)
    if (border1.precedence() == border2.precedence())
        return 0;
    return border1.precedence() < border2.precedence() ? -1 : 1;
}

static CollapsedBorderValue chooseBorder(const CollapsedBorderValue& border1, const CollapsedBorderValue& border2)
{
    const CollapsedBorderValue& border = compareBorders(border1, border2) < 0 ? border2 : border1;
    return border.style() == BHIDDEN ? CollapsedBorderValue() : border;
}

CollapsedBorderValue RenderTableCell::collapsedStartBorder(IncludeBorderColorOrNot includeColor) const
{
    CollapsedBorderValue result = computeCollapsedStartBorder(includeColor);
    if (includeColor)
        section()->setCachedCollapsedBorder(this, CBSStart, result);
    return result;
}

CollapsedBorderValue RenderTableCell::computeCollapsedStartBorder(IncludeBorderColorOrNot includeColor) const
{
    RenderTable* table = this->table();
    bool isStartColumn = col() == 0;

    // For the start border, we need to check, in order of precedence:
    // (1) Our start border.
    int startColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderStartColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    int endColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderEndColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    CollapsedBorderValue result(style()->borderStart(), includeColor ? style()->visitedDependentColor(startColorProperty) : Color(), BCELL);

    // (2) The end border of the preceding cell.
    if (RenderTableCell* prevCell = table->cellBefore(this)) {
        CollapsedBorderValue prevCellBorder = CollapsedBorderValue(prevCell->style()->borderEnd(), includeColor ? prevCell->style()->visitedDependentColor(endColorProperty) : Color(), BCELL);
        result = chooseBorder(prevCellBorder, result);
        if (!result.exists())
            return result;
    } else if (isStartColumn) {
        // (3) Our row's start border.
        result = chooseBorder(result, CollapsedBorderValue(parent()->style()->borderStart(), includeColor ? parent()->style()->visitedDependentColor(startColorProperty) : Color(), BROW));
        if (!result.exists())
            return result;
        
        // (4) Our row group's start border.
        result = chooseBorder(result, CollapsedBorderValue(section()->style()->borderStart(), includeColor ? section()->style()->visitedDependentColor(startColorProperty) : Color(), BROWGROUP));
        if (!result.exists())
            return result;
    }
    
    // (5) Our column and column group's start borders.
    bool startColEdge;
    bool endColEdge;
    RenderTableCol* colElt = table->colElement(col(), &startColEdge, &endColEdge);
    if (colElt && startColEdge) {
        result = chooseBorder(result, CollapsedBorderValue(colElt->style()->borderStart(), includeColor ? colElt->style()->visitedDependentColor(startColorProperty) : Color(), BCOL));
        if (!result.exists())
            return result;
        if (RenderTableCol* enclosingColumnGroup = colElt->enclosingColumnGroupIfAdjacentBefore()) {
            result = chooseBorder(result, CollapsedBorderValue(enclosingColumnGroup->style()->borderStart(), includeColor ? enclosingColumnGroup->style()->visitedDependentColor(startColorProperty) : Color(), BCOLGROUP));
            if (!result.exists())
                return result;
        }
    }
    
    // (6) The end border of the preceding column.
    if (!isStartColumn) {
        colElt = table->colElement(col() -1, &startColEdge, &endColEdge);
        if (colElt && endColEdge) {
            CollapsedBorderValue endBorder = CollapsedBorderValue(colElt->style()->borderEnd(), includeColor ? colElt->style()->visitedDependentColor(endColorProperty) : Color(), BCOL);
            result = chooseBorder(endBorder, result);
            if (!result.exists())
                return result;
        }
    } else {
        // (7) The table's start border.
        result = chooseBorder(result, CollapsedBorderValue(table->tableStartBorderAdjoiningCell(this), includeColor ? table->style()->visitedDependentColor(startColorProperty) : Color(), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedEndBorder(IncludeBorderColorOrNot includeColor) const
{
    CollapsedBorderValue result = computeCollapsedEndBorder(includeColor);
    if (includeColor)
        section()->setCachedCollapsedBorder(this, CBSEnd, result);
    return result;
}

CollapsedBorderValue RenderTableCell::computeCollapsedEndBorder(IncludeBorderColorOrNot includeColor) const
{
    RenderTable* table = this->table();
    bool isEndColumn = table->colToEffCol(col() + colSpan() - 1) == table->numEffCols() - 1;

    // For end border, we need to check, in order of precedence:
    // (1) Our end border.
    int startColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderStartColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    int endColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderEndColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    CollapsedBorderValue result = CollapsedBorderValue(style()->borderEnd(), includeColor ? style()->visitedDependentColor(endColorProperty) : Color(), BCELL);
    
    // (2) The start border of the following cell.
    if (!isEndColumn) {
        RenderTableCell* nextCell = table->cellAfter(this);
        if (nextCell && nextCell->style()) {
            CollapsedBorderValue startBorder = CollapsedBorderValue(nextCell->style()->borderStart(), includeColor ? nextCell->style()->visitedDependentColor(startColorProperty) : Color(), BCELL);
            result = chooseBorder(result, startBorder);
            if (!result.exists())
                return result;
        }
    } else {
        // (3) Our row's end border.
        result = chooseBorder(result, CollapsedBorderValue(parent()->style()->borderEnd(), includeColor ? parent()->style()->visitedDependentColor(endColorProperty) : Color(), BROW));
        if (!result.exists())
            return result;
        
        // (4) Our row group's end border.
        result = chooseBorder(result, CollapsedBorderValue(section()->style()->borderEnd(), includeColor ? section()->style()->visitedDependentColor(endColorProperty) : Color(), BROWGROUP));
        if (!result.exists())
            return result;
    }
    
    // (5) Our column and column group's end borders.
    bool startColEdge;
    bool endColEdge;
    RenderTableCol* colElt = table->colElement(col() + colSpan() - 1, &startColEdge, &endColEdge);
    if (colElt && endColEdge) {
        result = chooseBorder(result, CollapsedBorderValue(colElt->style()->borderEnd(), includeColor ? colElt->style()->visitedDependentColor(endColorProperty) : Color(), BCOL));
        if (!result.exists())
            return result;
        if (RenderTableCol* enclosingColumnGroup = colElt->enclosingColumnGroupIfAdjacentAfter()) {
            result = chooseBorder(result, CollapsedBorderValue(enclosingColumnGroup->style()->borderEnd(), includeColor ? enclosingColumnGroup->style()->visitedDependentColor(endColorProperty) : Color(), BCOLGROUP));
            if (!result.exists())
                return result;
        }
    }
    
    // (6) The start border of the next column.
    if (!isEndColumn) {
        colElt = table->colElement(col() + colSpan(), &startColEdge, &endColEdge);
        if (colElt && startColEdge) {
            CollapsedBorderValue startBorder = CollapsedBorderValue(colElt->style()->borderStart(), includeColor ? colElt->style()->visitedDependentColor(startColorProperty) : Color(), BCOL);
            result = chooseBorder(result, startBorder);
            if (!result.exists())
                return result;
        }
    } else {
        // (7) The table's end border.
        result = chooseBorder(result, CollapsedBorderValue(table->tableEndBorderAdjoiningCell(this), includeColor ? table->style()->visitedDependentColor(endColorProperty) : Color(), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedBeforeBorder(IncludeBorderColorOrNot includeColor) const
{
    CollapsedBorderValue result = computeCollapsedBeforeBorder(includeColor);
    if (includeColor)
        section()->setCachedCollapsedBorder(this, CBSBefore, result);
    return result;
}

CollapsedBorderValue RenderTableCell::computeCollapsedBeforeBorder(IncludeBorderColorOrNot includeColor) const
{
    RenderTable* table = this->table();

    // For before border, we need to check, in order of precedence:
    // (1) Our before border.
    int beforeColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderBeforeColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    int afterColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderAfterColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    CollapsedBorderValue result = CollapsedBorderValue(style()->borderBefore(), includeColor ? style()->visitedDependentColor(beforeColorProperty) : Color(), BCELL);
    
    RenderTableCell* prevCell = table->cellAbove(this);
    if (prevCell) {
        // (2) A before cell's after border.
        result = chooseBorder(CollapsedBorderValue(prevCell->style()->borderAfter(), includeColor ? prevCell->style()->visitedDependentColor(afterColorProperty) : Color(), BCELL), result);
        if (!result.exists())
            return result;
    }
    
    // (3) Our row's before border.
    result = chooseBorder(result, CollapsedBorderValue(parent()->style()->borderBefore(), includeColor ? parent()->style()->visitedDependentColor(beforeColorProperty) : Color(), BROW));
    if (!result.exists())
        return result;
    
    // (4) The previous row's after border.
    if (prevCell) {
        RenderObject* prevRow = 0;
        if (prevCell->section() == section())
            prevRow = parent()->previousSibling();
        else
            prevRow = prevCell->section()->lastChild();
    
        if (prevRow) {
            result = chooseBorder(CollapsedBorderValue(prevRow->style()->borderAfter(), includeColor ? prevRow->style()->visitedDependentColor(afterColorProperty) : Color(), BROW), result);
            if (!result.exists())
                return result;
        }
    }
    
    // Now check row groups.
    RenderTableSection* currSection = section();
    if (!rowIndex()) {
        // (5) Our row group's before border.
        result = chooseBorder(result, CollapsedBorderValue(currSection->style()->borderBefore(), includeColor ? currSection->style()->visitedDependentColor(beforeColorProperty) : Color(), BROWGROUP));
        if (!result.exists())
            return result;
        
        // (6) Previous row group's after border.
        currSection = table->sectionAbove(currSection, SkipEmptySections);
        if (currSection) {
            result = chooseBorder(CollapsedBorderValue(currSection->style()->borderAfter(), includeColor ? currSection->style()->visitedDependentColor(afterColorProperty) : Color(), BROWGROUP), result);
            if (!result.exists())
                return result;
        }
    }
    
    if (!currSection) {
        // (8) Our column and column group's before borders.
        RenderTableCol* colElt = table->colElement(col());
        if (colElt) {
            result = chooseBorder(result, CollapsedBorderValue(colElt->style()->borderBefore(), includeColor ? colElt->style()->visitedDependentColor(beforeColorProperty) : Color(), BCOL));
            if (!result.exists())
                return result;
            if (RenderTableCol* enclosingColumnGroup = colElt->enclosingColumnGroup()) {
                result = chooseBorder(result, CollapsedBorderValue(enclosingColumnGroup->style()->borderBefore(), includeColor ? enclosingColumnGroup->style()->visitedDependentColor(beforeColorProperty) : Color(), BCOLGROUP));
                if (!result.exists())
                    return result;
            }
        }
        
        // (9) The table's before border.
        result = chooseBorder(result, CollapsedBorderValue(table->style()->borderBefore(), includeColor ? table->style()->visitedDependentColor(beforeColorProperty) : Color(), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedAfterBorder(IncludeBorderColorOrNot includeColor) const
{
    CollapsedBorderValue result = computeCollapsedAfterBorder(includeColor);
    if (includeColor)
        section()->setCachedCollapsedBorder(this, CBSAfter, result);
    return result;
}

CollapsedBorderValue RenderTableCell::computeCollapsedAfterBorder(IncludeBorderColorOrNot includeColor) const
{
    RenderTable* table = this->table();

    // For after border, we need to check, in order of precedence:
    // (1) Our after border.
    int beforeColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderBeforeColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    int afterColorProperty = includeColor ? CSSProperty::resolveDirectionAwareProperty(CSSPropertyWebkitBorderAfterColor, styleForCellFlow()->direction(), styleForCellFlow()->writingMode()) : 0;
    CollapsedBorderValue result = CollapsedBorderValue(style()->borderAfter(), includeColor ? style()->visitedDependentColor(afterColorProperty) : Color(), BCELL);
    
    RenderTableCell* nextCell = table->cellBelow(this);
    if (nextCell) {
        // (2) An after cell's before border.
        result = chooseBorder(result, CollapsedBorderValue(nextCell->style()->borderBefore(), includeColor ? nextCell->style()->visitedDependentColor(beforeColorProperty) : Color(), BCELL));
        if (!result.exists())
            return result;
    }
    
    // (3) Our row's after border. (FIXME: Deal with rowspan!)
    result = chooseBorder(result, CollapsedBorderValue(parent()->style()->borderAfter(), includeColor ? parent()->style()->visitedDependentColor(afterColorProperty) : Color(), BROW));
    if (!result.exists())
        return result;
    
    // (4) The next row's before border.
    if (nextCell) {
        result = chooseBorder(result, CollapsedBorderValue(nextCell->parent()->style()->borderBefore(), includeColor ? nextCell->parent()->style()->visitedDependentColor(beforeColorProperty) : Color(), BROW));
        if (!result.exists())
            return result;
    }
    
    // Now check row groups.
    RenderTableSection* currSection = section();
    if (rowIndex() + rowSpan() >= currSection->numRows()) {
        // (5) Our row group's after border.
        result = chooseBorder(result, CollapsedBorderValue(currSection->style()->borderAfter(), includeColor ? currSection->style()->visitedDependentColor(afterColorProperty) : Color(), BROWGROUP));
        if (!result.exists())
            return result;
        
        // (6) Following row group's before border.
        currSection = table->sectionBelow(currSection, SkipEmptySections);
        if (currSection) {
            result = chooseBorder(result, CollapsedBorderValue(currSection->style()->borderBefore(), includeColor ? currSection->style()->visitedDependentColor(beforeColorProperty) : Color(), BROWGROUP));
            if (!result.exists())
                return result;
        }
    }
    
    if (!currSection) {
        // (8) Our column and column group's after borders.
        RenderTableCol* colElt = table->colElement(col());
        if (colElt) {
            result = chooseBorder(result, CollapsedBorderValue(colElt->style()->borderAfter(), includeColor ? colElt->style()->visitedDependentColor(afterColorProperty) : Color(), BCOL));
            if (!result.exists()) return result;
            if (RenderTableCol* enclosingColumnGroup = colElt->enclosingColumnGroup()) {
                result = chooseBorder(result, CollapsedBorderValue(enclosingColumnGroup->style()->borderAfter(), includeColor ? enclosingColumnGroup->style()->visitedDependentColor(afterColorProperty) : Color(), BCOLGROUP));
                if (!result.exists())
                    return result;
            }
        }
        
        // (9) The table's after border.
        result = chooseBorder(result, CollapsedBorderValue(table->style()->borderAfter(), includeColor ? table->style()->visitedDependentColor(afterColorProperty) : Color(), BTABLE));
        if (!result.exists())
            return result;
    }
    
    return result;    
}

inline CollapsedBorderValue RenderTableCell::cachedCollapsedLeftBorder(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isLeftToRightDirection() ? section()->cachedCollapsedBorder(this, CBSStart) : section()->cachedCollapsedBorder(this, CBSEnd);
    return styleForCellFlow->isFlippedBlocksWritingMode() ? section()->cachedCollapsedBorder(this, CBSAfter) : section()->cachedCollapsedBorder(this, CBSBefore);
}

inline CollapsedBorderValue RenderTableCell::cachedCollapsedRightBorder(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isLeftToRightDirection() ? section()->cachedCollapsedBorder(this, CBSEnd) : section()->cachedCollapsedBorder(this, CBSStart);
    return styleForCellFlow->isFlippedBlocksWritingMode() ? section()->cachedCollapsedBorder(this, CBSBefore) : section()->cachedCollapsedBorder(this, CBSAfter);
}

inline CollapsedBorderValue RenderTableCell::cachedCollapsedTopBorder(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isFlippedBlocksWritingMode() ? section()->cachedCollapsedBorder(this, CBSAfter) : section()->cachedCollapsedBorder(this, CBSBefore);
    return styleForCellFlow->isLeftToRightDirection() ? section()->cachedCollapsedBorder(this, CBSStart) : section()->cachedCollapsedBorder(this, CBSEnd);
}

inline CollapsedBorderValue RenderTableCell::cachedCollapsedBottomBorder(const RenderStyle* styleForCellFlow) const
{
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isFlippedBlocksWritingMode() ? section()->cachedCollapsedBorder(this, CBSBefore) : section()->cachedCollapsedBorder(this, CBSAfter);
    return styleForCellFlow->isLeftToRightDirection() ? section()->cachedCollapsedBorder(this, CBSEnd) : section()->cachedCollapsedBorder(this, CBSStart);
}

int RenderTableCell::borderLeft() const
{
    return table()->collapseBorders() ? borderHalfLeft(false) : RenderBlock::borderLeft();
}

int RenderTableCell::borderRight() const
{
    return table()->collapseBorders() ? borderHalfRight(false) : RenderBlock::borderRight();
}

int RenderTableCell::borderTop() const
{
    return table()->collapseBorders() ? borderHalfTop(false) : RenderBlock::borderTop();
}

int RenderTableCell::borderBottom() const
{
    return table()->collapseBorders() ? borderHalfBottom(false) : RenderBlock::borderBottom();
}

// FIXME: https://bugs.webkit.org/show_bug.cgi?id=46191, make the collapsed border drawing
// work with different block flow values instead of being hard-coded to top-to-bottom.
int RenderTableCell::borderStart() const
{
    return table()->collapseBorders() ? borderHalfStart(false) : RenderBlock::borderStart();
}

int RenderTableCell::borderEnd() const
{
    return table()->collapseBorders() ? borderHalfEnd(false) : RenderBlock::borderEnd();
}

int RenderTableCell::borderBefore() const
{
    return table()->collapseBorders() ? borderHalfBefore(false) : RenderBlock::borderBefore();
}

int RenderTableCell::borderAfter() const
{
    return table()->collapseBorders() ? borderHalfAfter(false) : RenderBlock::borderAfter();
}

int RenderTableCell::borderHalfLeft(bool outer) const
{
    const RenderStyle* styleForCellFlow = this->styleForCellFlow();
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isLeftToRightDirection() ? borderHalfStart(outer) : borderHalfEnd(outer);
    return styleForCellFlow->isFlippedBlocksWritingMode() ? borderHalfAfter(outer) : borderHalfBefore(outer);
}

int RenderTableCell::borderHalfRight(bool outer) const
{
    const RenderStyle* styleForCellFlow = this->styleForCellFlow();
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isLeftToRightDirection() ? borderHalfEnd(outer) : borderHalfStart(outer);
    return styleForCellFlow->isFlippedBlocksWritingMode() ? borderHalfBefore(outer) : borderHalfAfter(outer);
}

int RenderTableCell::borderHalfTop(bool outer) const
{
    const RenderStyle* styleForCellFlow = this->styleForCellFlow();
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isFlippedBlocksWritingMode() ? borderHalfAfter(outer) : borderHalfBefore(outer);
    return styleForCellFlow->isLeftToRightDirection() ? borderHalfStart(outer) : borderHalfEnd(outer);
}

int RenderTableCell::borderHalfBottom(bool outer) const
{
    const RenderStyle* styleForCellFlow = this->styleForCellFlow();
    if (styleForCellFlow->isHorizontalWritingMode())
        return styleForCellFlow->isFlippedBlocksWritingMode() ? borderHalfBefore(outer) : borderHalfAfter(outer);
    return styleForCellFlow->isLeftToRightDirection() ? borderHalfEnd(outer) : borderHalfStart(outer);
}

int RenderTableCell::borderHalfStart(bool outer) const
{
    CollapsedBorderValue border = collapsedStartBorder(DoNotIncludeBorderColor);
    if (border.exists())
        return (border.width() + ((styleForCellFlow()->isLeftToRightDirection() ^ outer) ? 1 : 0)) / 2; // Give the extra pixel to top and left.
    return 0;
}
    
int RenderTableCell::borderHalfEnd(bool outer) const
{
    CollapsedBorderValue border = collapsedEndBorder(DoNotIncludeBorderColor);
    if (border.exists())
        return (border.width() + ((styleForCellFlow()->isLeftToRightDirection() ^ outer) ? 0 : 1)) / 2;
    return 0;
}

int RenderTableCell::borderHalfBefore(bool outer) const
{
    CollapsedBorderValue border = collapsedBeforeBorder(DoNotIncludeBorderColor);
    if (border.exists())
        return (border.width() + ((styleForCellFlow()->isFlippedBlocksWritingMode() ^ outer) ? 0 : 1)) / 2; // Give the extra pixel to top and left.
    return 0;
}

int RenderTableCell::borderHalfAfter(bool outer) const
{
    CollapsedBorderValue border = collapsedAfterBorder(DoNotIncludeBorderColor);
    if (border.exists())
        return (border.width() + ((styleForCellFlow()->isFlippedBlocksWritingMode() ^ outer) ? 1 : 0)) / 2;
    return 0;
}

void RenderTableCell::paint(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    ASSERT(paintInfo.phase != PaintPhaseCollapsedTableBorders);
    RenderBlock::paint(paintInfo, paintOffset);
}

static EBorderStyle collapsedBorderStyle(EBorderStyle style)
{
    if (style == OUTSET)
        return GROOVE;
    if (style == INSET)
        return RIDGE;
    return style;
}

struct CollapsedBorder {
    CollapsedBorderValue borderValue;
    BoxSide side;
    bool shouldPaint;
    int x1;
    int y1;
    int x2;
    int y2;
    EBorderStyle style;
};

class CollapsedBorders {
public:
    CollapsedBorders()
        : m_count(0)
    {
    }
    
    void addBorder(const CollapsedBorderValue& borderValue, BoxSide borderSide, bool shouldPaint,
                   int x1, int y1, int x2, int y2, EBorderStyle borderStyle)
    {
        if (borderValue.exists() && shouldPaint) {
            m_borders[m_count].borderValue = borderValue;
            m_borders[m_count].side = borderSide;
            m_borders[m_count].shouldPaint = shouldPaint;
            m_borders[m_count].x1 = x1;
            m_borders[m_count].x2 = x2;
            m_borders[m_count].y1 = y1;
            m_borders[m_count].y2 = y2;
            m_borders[m_count].style = borderStyle;
            m_count++;
        }
    }

    CollapsedBorder* nextBorder()
    {
        for (unsigned i = 0; i < m_count; i++) {
            if (m_borders[i].borderValue.exists() && m_borders[i].shouldPaint) {
                m_borders[i].shouldPaint = false;
                return &m_borders[i];
            }
        }
        
        return 0;
    }
    
    CollapsedBorder m_borders[4];
    unsigned m_count;
};

static void addBorderStyle(RenderTable::CollapsedBorderValues& borderValues,
                           CollapsedBorderValue borderValue)
{
    if (!borderValue.exists())
        return;
    size_t count = borderValues.size();
    for (size_t i = 0; i < count; ++i)
        if (borderValues[i].isSameIgnoringColor(borderValue))
            return;
    borderValues.append(borderValue);
}

void RenderTableCell::collectBorderValues(RenderTable::CollapsedBorderValues& borderValues) const
{
    addBorderStyle(borderValues, collapsedStartBorder());
    addBorderStyle(borderValues, collapsedEndBorder());
    addBorderStyle(borderValues, collapsedBeforeBorder());
    addBorderStyle(borderValues, collapsedAfterBorder());
}

static int compareBorderValuesForQSort(const void* pa, const void* pb)
{
    const CollapsedBorderValue* a = static_cast<const CollapsedBorderValue*>(pa);
    const CollapsedBorderValue* b = static_cast<const CollapsedBorderValue*>(pb);
    if (a->isSameIgnoringColor(*b))
        return 0;
    return compareBorders(*a, *b);
}

void RenderTableCell::sortBorderValues(RenderTable::CollapsedBorderValues& borderValues)
{
    qsort(borderValues.data(), borderValues.size(), sizeof(CollapsedBorderValue),
        compareBorderValuesForQSort);
}

void RenderTableCell::paintCollapsedBorders(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    ASSERT(paintInfo.phase == PaintPhaseCollapsedTableBorders);

    if (!paintInfo.shouldPaintWithinRoot(this) || style()->visibility() != VISIBLE)
        return;

    LayoutRect localRepaintRect = paintInfo.rect;
    localRepaintRect.inflate(maximalOutlineSize(paintInfo.phase));

    LayoutRect paintRect = LayoutRect(paintOffset + location(), size());
    if (paintRect.y() - table()->outerBorderTop() >= localRepaintRect.maxY())
        return;

    if (paintRect.maxY() + table()->outerBorderBottom() <= localRepaintRect.y())
        return;

    GraphicsContext* graphicsContext = paintInfo.context;
    if (!table()->currentBorderValue() || graphicsContext->paintingDisabled())
        return;

    const RenderStyle* styleForCellFlow = this->styleForCellFlow();
    CollapsedBorderValue leftVal = cachedCollapsedLeftBorder(styleForCellFlow);
    CollapsedBorderValue rightVal = cachedCollapsedRightBorder(styleForCellFlow);
    CollapsedBorderValue topVal = cachedCollapsedTopBorder(styleForCellFlow);
    CollapsedBorderValue bottomVal = cachedCollapsedBottomBorder(styleForCellFlow);
     
    // Adjust our x/y/width/height so that we paint the collapsed borders at the correct location.
    int topWidth = topVal.width();
    int bottomWidth = bottomVal.width();
    int leftWidth = leftVal.width();
    int rightWidth = rightVal.width();

    IntRect borderRect = pixelSnappedIntRect(paintRect.x() - leftWidth / 2,
            paintRect.y() - topWidth / 2,
            paintRect.width() + leftWidth / 2 + (rightWidth + 1) / 2,
            paintRect.height() + topWidth / 2 + (bottomWidth + 1) / 2);

    EBorderStyle topStyle = collapsedBorderStyle(topVal.style());
    EBorderStyle bottomStyle = collapsedBorderStyle(bottomVal.style());
    EBorderStyle leftStyle = collapsedBorderStyle(leftVal.style());
    EBorderStyle rightStyle = collapsedBorderStyle(rightVal.style());
    
    bool renderTop = topStyle > BHIDDEN && !topVal.isTransparent();
    bool renderBottom = bottomStyle > BHIDDEN && !bottomVal.isTransparent();
    bool renderLeft = leftStyle > BHIDDEN && !leftVal.isTransparent();
    bool renderRight = rightStyle > BHIDDEN && !rightVal.isTransparent();

    // We never paint diagonals at the joins.  We simply let the border with the highest
    // precedence paint on top of borders with lower precedence.  
    CollapsedBorders borders;
    borders.addBorder(topVal, BSTop, renderTop, borderRect.x(), borderRect.y(), borderRect.maxX(), borderRect.y() + topWidth, topStyle);
    borders.addBorder(bottomVal, BSBottom, renderBottom, borderRect.x(), borderRect.maxY() - bottomWidth, borderRect.maxX(), borderRect.maxY(), bottomStyle);
    borders.addBorder(leftVal, BSLeft, renderLeft, borderRect.x(), borderRect.y(), borderRect.x() + leftWidth, borderRect.maxY(), leftStyle);
    borders.addBorder(rightVal, BSRight, renderRight, borderRect.maxX() - rightWidth, borderRect.y(), borderRect.maxX(), borderRect.maxY(), rightStyle);

    bool antialias = shouldAntialiasLines(graphicsContext);
    
    for (CollapsedBorder* border = borders.nextBorder(); border; border = borders.nextBorder()) {
        if (border->borderValue.isSameIgnoringColor(*table()->currentBorderValue()))
            drawLineForBoxSide(graphicsContext, border->x1, border->y1, border->x2, border->y2, border->side, 
                               border->borderValue.color(), border->style, 0, 0, antialias);
    }
}

void RenderTableCell::paintBackgroundsBehindCell(PaintInfo& paintInfo, const LayoutPoint& paintOffset, RenderObject* backgroundObject)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    if (!backgroundObject)
        return;

    if (style()->visibility() != VISIBLE)
        return;

    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;

    LayoutPoint adjustedPaintOffset = paintOffset;
    if (backgroundObject != this)
        adjustedPaintOffset.moveBy(location());

    Color c = backgroundObject->style()->visitedDependentColor(CSSPropertyBackgroundColor);
    const FillLayer* bgLayer = backgroundObject->style()->backgroundLayers();

    if (bgLayer->hasImage() || c.isValid()) {
        // We have to clip here because the background would paint
        // on top of the borders otherwise.  This only matters for cells and rows.
        bool shouldClip = backgroundObject->hasLayer() && (backgroundObject == this || backgroundObject == parent()) && tableElt->collapseBorders();
        GraphicsContextStateSaver stateSaver(*paintInfo.context, shouldClip);
        if (shouldClip) {
            LayoutRect clipRect(adjustedPaintOffset.x() + borderLeft(), adjustedPaintOffset.y() + borderTop(),
                width() - borderLeft() - borderRight(), height() - borderTop() - borderBottom());
            paintInfo.context->clip(clipRect);
        }
        paintFillLayers(paintInfo, c, bgLayer, LayoutRect(adjustedPaintOffset, size()), BackgroundBleedNone, CompositeSourceOver, backgroundObject);
    }
}

void RenderTableCell::paintBoxDecorations(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (!paintInfo.shouldPaintWithinRoot(this))
        return;

    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;

    LayoutRect paintRect = LayoutRect(paintOffset, size());
    paintBoxShadow(paintInfo, paintRect, style(), Normal);
    
    // Paint our cell background.
    paintBackgroundsBehindCell(paintInfo, paintOffset, this);

    paintBoxShadow(paintInfo, paintRect, style(), Inset);

    if (!style()->hasBorder() || tableElt->collapseBorders())
        return;

    paintBorder(paintInfo, paintRect, style());
}

void RenderTableCell::paintMask(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    if (style()->visibility() != VISIBLE || paintInfo.phase != PaintPhaseMask)
        return;

    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;
   
    paintMaskImages(paintInfo, LayoutRect(paintOffset, size()));
}

bool RenderTableCell::boxShadowShouldBeAppliedToBackground(BackgroundBleedAvoidance, InlineFlowBox*) const
{
    return false;
}

void RenderTableCell::scrollbarsChanged(bool horizontalScrollbarChanged, bool verticalScrollbarChanged)
{
    LayoutUnit scrollbarHeight = scrollbarLogicalHeight();
    if (!scrollbarHeight)
        return; // Not sure if we should be doing something when a scrollbar goes away or not.
    
    // We only care if the scrollbar that affects our intrinsic padding has been added.
    if ((isHorizontalWritingMode() && !horizontalScrollbarChanged) ||
        (!isHorizontalWritingMode() && !verticalScrollbarChanged))
        return;

    // Shrink our intrinsic padding as much as possible to accommodate the scrollbar.
    if (style()->verticalAlign() == MIDDLE) {
        LayoutUnit totalHeight = logicalHeight();
        LayoutUnit heightWithoutIntrinsicPadding = totalHeight - intrinsicPaddingBefore() - intrinsicPaddingAfter();
        totalHeight -= scrollbarHeight;
        LayoutUnit newBeforePadding = (totalHeight - heightWithoutIntrinsicPadding) / 2;
        LayoutUnit newAfterPadding = totalHeight - heightWithoutIntrinsicPadding - newBeforePadding;
        setIntrinsicPaddingBefore(newBeforePadding);
        setIntrinsicPaddingAfter(newAfterPadding);
    } else
        setIntrinsicPaddingAfter(intrinsicPaddingAfter() - scrollbarHeight);
}

RenderTableCell* RenderTableCell::createAnonymousWithParentRenderer(const RenderObject* parent)
{
    RefPtr<RenderStyle> newStyle = RenderStyle::createAnonymousStyleWithDisplay(parent->style(), TABLE_CELL);
    RenderTableCell* newCell = new (parent->renderArena()) RenderTableCell(parent->document() /* is anonymous */);
    newCell->setStyle(newStyle.release());
    return newCell;
}

} // namespace WebCore
