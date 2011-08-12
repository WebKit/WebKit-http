/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2009 Apple Inc. All rights reserved.
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

#ifndef RenderTableSection_h
#define RenderTableSection_h

#include "RenderTable.h"
#include <wtf/Vector.h>

namespace WebCore {

class RenderTableCell;
class RenderTableRow;

class RenderTableSection : public RenderBox {
public:
    RenderTableSection(Node*);
    virtual ~RenderTableSection();

    const RenderObjectChildList* children() const { return &m_children; }
    RenderObjectChildList* children() { return &m_children; }

    virtual void addChild(RenderObject* child, RenderObject* beforeChild = 0);

    virtual LayoutUnit firstLineBoxBaseline() const;

    void addCell(RenderTableCell*, RenderTableRow* row);

    void setCellLogicalWidths();
    LayoutUnit calcRowLogicalHeight();
    LayoutUnit layoutRows(LayoutUnit logicalHeight);

    RenderTable* table() const { return toRenderTable(parent()); }

    struct CellStruct {
        Vector<RenderTableCell*, 1> cells; 
        bool inColSpan; // true for columns after the first in a colspan

        CellStruct():
          inColSpan(false) {}
        
        RenderTableCell* primaryCell()
        {
            return hasCells() ? cells[cells.size() - 1] : 0;
        }

        const RenderTableCell* primaryCell() const
        {
            return hasCells() ? cells[cells.size() - 1] : 0;
        }

        bool hasCells() const { return cells.size() > 0; }
    };

    typedef Vector<CellStruct> Row;

    struct RowStruct {
        Row* row;
        RenderTableRow* rowRenderer;
        LayoutUnit baseline;
        Length logicalHeight;
    };

    CellStruct& cellAt(int row,  int col) { return (*m_grid[row].row)[col]; }
    const CellStruct& cellAt(int row, int col) const { return (*m_grid[row].row)[col]; }
    RenderTableCell* primaryCellAt(int row, int col)
    {
        CellStruct& c = (*m_grid[row].row)[col];
        return c.primaryCell();
    }

    void appendColumn(int pos);
    void splitColumn(int pos, int first);

    LayoutUnit calcOuterBorderBefore() const;
    LayoutUnit calcOuterBorderAfter() const;
    LayoutUnit calcOuterBorderStart() const;
    LayoutUnit calcOuterBorderEnd() const;
    void recalcOuterBorder();

    LayoutUnit outerBorderBefore() const { return m_outerBorderBefore; }
    LayoutUnit outerBorderAfter() const { return m_outerBorderAfter; }
    LayoutUnit outerBorderStart() const { return m_outerBorderStart; }
    LayoutUnit outerBorderEnd() const { return m_outerBorderEnd; }

    int numRows() const { return m_gridRows; }
    int numColumns() const;
    void recalcCells();
    void recalcCellsIfNeeded()
    {
        if (m_needsCellRecalc)
            recalcCells();
    }

    bool needsCellRecalc() const { return m_needsCellRecalc; }
    void setNeedsCellRecalc();

    LayoutUnit getBaseline(int row) { return m_grid[row].baseline; }

private:
    virtual RenderObjectChildList* virtualChildren() { return children(); }
    virtual const RenderObjectChildList* virtualChildren() const { return children(); }

    virtual const char* renderName() const { return isAnonymous() ? "RenderTableSection (anonymous)" : "RenderTableSection"; }

    virtual bool isTableSection() const { return true; }

    virtual void willBeDestroyed();

    virtual void layout();

    virtual void removeChild(RenderObject* oldChild);

    virtual void paint(PaintInfo&, const LayoutPoint&);
    virtual void paintCell(RenderTableCell*, PaintInfo&, const LayoutPoint&);
    virtual void paintObject(PaintInfo&, const LayoutPoint&);

    virtual void imageChanged(WrappedImagePtr, const IntRect* = 0);

    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, const LayoutPoint& pointInContainer, const LayoutPoint& accumulatedOffset, HitTestAction);

    bool ensureRows(int);
    void clearGrid();

    RenderObjectChildList m_children;

    Vector<RowStruct> m_grid;
    Vector<LayoutUnit> m_rowPos;

    int m_gridRows;

    // the current insertion position
    int m_cCol;
    int m_cRow;

    LayoutUnit m_outerBorderStart;
    LayoutUnit m_outerBorderEnd;
    LayoutUnit m_outerBorderBefore;
    LayoutUnit m_outerBorderAfter;

    bool m_needsCellRecalc;
    bool m_hasOverflowingCell;

    bool m_hasMultipleCellLevels;
};

inline RenderTableSection* toRenderTableSection(RenderObject* object)
{
    ASSERT(!object || object->isTableSection());
    return static_cast<RenderTableSection*>(object);
}

inline const RenderTableSection* toRenderTableSection(const RenderObject* object)
{
    ASSERT(!object || object->isTableSection());
    return static_cast<const RenderTableSection*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderTableSection(const RenderTableSection*);

} // namespace WebCore

#endif // RenderTableSection_h
