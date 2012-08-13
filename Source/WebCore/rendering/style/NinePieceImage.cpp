/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NinePieceImage.h"

namespace WebCore {

bool NinePieceImageData::operator==(const NinePieceImageData& o) const
{
    return StyleImage::imagesEquivalent(m_image.get(), o.m_image.get()) && m_imageSlices == o.m_imageSlices && m_fill == o.m_fill
           && m_borderSlices == o.m_borderSlices && m_outset == o.m_outset && m_horizontalRule == o.m_horizontalRule && m_verticalRule == o.m_verticalRule;
}

const NinePieceImageData& NinePieceImage::defaultData()
{
    DEFINE_STATIC_LOCAL(NinePieceImageData, data, ());
    return data;
}

}
