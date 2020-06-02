/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PerformanceEntry.h"

#include "RuntimeEnabledFeatures.h"
#include <wtf/Optional.h>

namespace WebCore {

DEFINE_ALLOCATOR_WITH_HEAP_IDENTIFIER(PerformanceEntry);

PerformanceEntry::PerformanceEntry(const String& name, double startTime, double finishTime)
    : m_name(name)
    , m_startTime(startTime)
    , m_duration(finishTime - startTime)
{
}

PerformanceEntry::~PerformanceEntry() = default;

Optional<PerformanceEntry::Type> PerformanceEntry::parseEntryTypeString(const String& entryType)
{
    if (entryType == "navigation")
        return Optional<Type>(Type::Navigation);

    if (RuntimeEnabledFeatures::sharedFeatures().userTimingEnabled()) {
        if (entryType == "mark")
            return Optional<Type>(Type::Mark);
        if (entryType == "measure")
            return Optional<Type>(Type::Measure);
    }

    if (RuntimeEnabledFeatures::sharedFeatures().resourceTimingEnabled()) {
        if (entryType == "resource")
            return Optional<Type>(Type::Resource);
    }

    if (RuntimeEnabledFeatures::sharedFeatures().paintTimingEnabled()) {
        if (entryType == "paint")
            return Optional<Type>(Type::Paint);
    }

    return WTF::nullopt;
}

} // namespace WebCore
