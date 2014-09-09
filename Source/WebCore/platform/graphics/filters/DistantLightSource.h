/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
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

#ifndef DistantLightSource_h
#define DistantLightSource_h

#if ENABLE(FILTERS)
#include "LightSource.h"

namespace WebCore {

class DistantLightSource : public LightSource {
public:
    static PassRefPtr<DistantLightSource> create(float azimuth, float elevation)
    {
        return adoptRef(new DistantLightSource(azimuth, elevation));
    }

    float azimuth() const { return m_azimuth; }
    float elevation() const { return m_elevation; }

    virtual bool setAzimuth(float) override;
    virtual bool setElevation(float) override;

    virtual void initPaintingData(PaintingData&);
    virtual void updatePaintingData(PaintingData&, int x, int y, float z);

    virtual TextStream& externalRepresentation(TextStream&) const;

private:
    DistantLightSource(float azimuth, float elevation)
        : LightSource(LS_DISTANT)
        , m_azimuth(azimuth)
        , m_elevation(elevation)
    {
    }

    float m_azimuth;
    float m_elevation;
};

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // DistantLightSource_h
