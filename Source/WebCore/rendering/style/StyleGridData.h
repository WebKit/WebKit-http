/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 *  THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef StyleGridData_h
#define StyleGridData_h

#include "GridCoordinate.h"
#include "GridTrackSize.h"
#include "RenderStyleConstants.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

typedef HashMap<String, Vector<size_t>> NamedGridLinesMap;

class StyleGridData : public RefCounted<StyleGridData> {
public:
    static PassRef<StyleGridData> create() { return adoptRef(*new StyleGridData); }
    PassRef<StyleGridData> copy() const { return adoptRef(*new StyleGridData(*this)); }

    bool operator==(const StyleGridData& o) const
    {
        // FIXME: comparing two hashes doesn't look great for performance. Something to keep in mind going forward.
        return m_gridColumns == o.m_gridColumns && m_gridRows == o.m_gridRows && m_gridAutoFlow == o.m_gridAutoFlow && m_gridAutoRows == o.m_gridAutoRows && m_gridAutoColumns == o.m_gridAutoColumns && m_namedGridColumnLines == o.m_namedGridColumnLines && m_namedGridRowLines == o.m_namedGridRowLines && m_namedGridArea == o.m_namedGridArea && m_namedGridArea == o.m_namedGridArea && m_namedGridAreaRowCount == o.m_namedGridAreaRowCount && m_namedGridAreaColumnCount == o.m_namedGridAreaColumnCount;
    }

    bool operator!=(const StyleGridData& o) const
    {
        return !(*this == o);
    }

    // FIXME: Update the naming of the following variables.
    Vector<GridTrackSize> m_gridColumns;
    Vector<GridTrackSize> m_gridRows;

    NamedGridLinesMap m_namedGridColumnLines;
    NamedGridLinesMap m_namedGridRowLines;

    GridAutoFlow m_gridAutoFlow;

    GridTrackSize m_gridAutoRows;
    GridTrackSize m_gridAutoColumns;

    NamedGridAreaMap m_namedGridArea;
    // Because m_namedGridArea doesn't store the unnamed grid areas, we need to keep track
    // of the explicit grid size defined by both named and unnamed grid areas.
    size_t m_namedGridAreaRowCount;
    size_t m_namedGridAreaColumnCount;

private:
    StyleGridData();
    StyleGridData(const StyleGridData&);
};

} // namespace WebCore

#endif // StyleGridData_h
