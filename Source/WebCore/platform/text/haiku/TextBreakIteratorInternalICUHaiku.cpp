/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "TextBreakIteratorInternalICU.h"

#include <mutex>

#include "Language.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

static std::once_flag defaultLanguageCacheOnceFlag;
static CString defaultLanguageCache;

static void defaultLanguageCacheOnce()
{
    defaultLanguageCache = defaultLanguage().utf8();
}

const char* currentSearchLocaleID()
{
    std::call_once(defaultLanguageCacheOnceFlag, defaultLanguageCacheOnce);
    return defaultLanguageCache.data();
}

const char* currentTextBreakLocaleID()
{
    std::call_once(defaultLanguageCacheOnceFlag, defaultLanguageCacheOnce);
    return defaultLanguageCache.data();
}

} // namespace WebCore

