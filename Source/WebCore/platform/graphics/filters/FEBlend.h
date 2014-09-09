/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2014 Adobe Systems Incorporated. All rights reserved.
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

#ifndef FEBlend_h
#define FEBlend_h

#if ENABLE(FILTERS)
#include "FilterEffect.h"

#include "Filter.h"

namespace WebCore {

class FEBlend : public FilterEffect {
public:
    static PassRefPtr<FEBlend> create(Filter*, BlendMode);

    BlendMode blendMode() const;
    bool setBlendMode(BlendMode);

    void platformApplyGeneric(unsigned char* srcPixelArrayA, unsigned char* srcPixelArrayB, unsigned char* dstPixelArray,
                           unsigned colorArrayLength);
    void platformApplyNEON(unsigned char* srcPixelArrayA, unsigned char* srcPixelArrayB, unsigned char* dstPixelArray,
                           unsigned colorArrayLength);
    virtual void platformApplySoftware();
    virtual void dump();

    virtual TextStream& externalRepresentation(TextStream&, int indention) const;

private:
    FEBlend(Filter*, BlendMode);

    BlendMode m_mode;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // FEBlend_h
