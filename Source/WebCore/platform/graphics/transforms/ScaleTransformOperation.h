/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#ifndef ScaleTransformOperation_h
#define ScaleTransformOperation_h

#include "TransformOperation.h"

namespace WebCore {

class ScaleTransformOperation : public TransformOperation {
public:
    static PassRefPtr<ScaleTransformOperation> create(double sx, double sy, OperationType type)
    {
        return adoptRef(new ScaleTransformOperation(sx, sy, 1, type));
    }

    static PassRefPtr<ScaleTransformOperation> create(double sx, double sy, double sz, OperationType type)
    {
        return adoptRef(new ScaleTransformOperation(sx, sy, sz, type));
    }

    double x() const { return m_x; }
    double y() const { return m_y; }
    double z() const { return m_z; }

private:
    virtual bool isIdentity() const { return m_x == 1 &&  m_y == 1 &&  m_z == 1; }

    virtual OperationType type() const { return m_type; }
    virtual bool isSameType(const TransformOperation& o) const { return o.type() == m_type; }

    virtual bool operator==(const TransformOperation& o) const
    {
        if (!isSameType(o))
            return false;
        const ScaleTransformOperation* s = static_cast<const ScaleTransformOperation*>(&o);
        return m_x == s->m_x && m_y == s->m_y && m_z == s->m_z;
    }

    virtual bool apply(TransformationMatrix& transform, const FloatSize&) const
    {
        transform.scale3d(m_x, m_y, m_z);
        return false;
    }

    virtual PassRefPtr<TransformOperation> blend(const TransformOperation* from, double progress, bool blendToIdentity = false);

    ScaleTransformOperation(double sx, double sy, double sz, OperationType type)
        : m_x(sx)
        , m_y(sy)
        , m_z(sz)
        , m_type(type)
    {
        ASSERT(type == SCALE_X || type == SCALE_Y || type == SCALE_Z || type == SCALE || type == SCALE_3D);
    }
        
    double m_x;
    double m_y;
    double m_z;
    OperationType m_type;
};

} // namespace WebCore

#endif // ScaleTransformOperation_h
