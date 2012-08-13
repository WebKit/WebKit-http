/**
 * Copyright (C) 2011 Nokia Inc.  All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "QuotesData.h"

namespace WebCore {

PassRefPtr<QuotesData> QuotesData::create(String open, String close)
{
    RefPtr<QuotesData> data = QuotesData::create();
    data->addPair(std::make_pair(open, close));
    return data;
}

PassRefPtr<QuotesData> QuotesData::create(String open1, String close1, String open2, String close2)
{
    RefPtr<QuotesData> data = QuotesData::create();
    data->addPair(std::make_pair(open1, close1));
    data->addPair(std::make_pair(open2, close2));
    return data;
}

void QuotesData::addPair(std::pair<String, String> quotePair)
{
    m_quotePairs.append(quotePair);
}

const String QuotesData::getOpenQuote(int index) const
{
    ASSERT(index >= 0);
    if (!m_quotePairs.size() || index < 0)
        return emptyString();
    if ((size_t)index >= m_quotePairs.size())
        return m_quotePairs.last().first;
    return m_quotePairs.at(index).first;
}

const String QuotesData::getCloseQuote(int index) const
{
    ASSERT(index >= -1);
    if (!m_quotePairs.size() || index < 0)
        return emptyString();
    if ((size_t)index >= m_quotePairs.size())
        return m_quotePairs.last().second;
    return m_quotePairs.at(index).second;
}

bool QuotesData::equals(const QuotesData* a, const QuotesData* b)
{
    if (a == b)
        return true;
    if (!a || !b)
        return false;
    return a->m_quotePairs == b->m_quotePairs;
}

} // namespace WebCore
