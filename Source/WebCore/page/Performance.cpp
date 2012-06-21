/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "Performance.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "MemoryInfo.h"
#include "PerformanceEntry.h"
#include "PerformanceNavigation.h"
#include "PerformanceResourceTiming.h"
#include "PerformanceTiming.h"
#include "ResourceResponse.h"
#include <wtf/CurrentTime.h>

#if ENABLE(WEB_TIMING)

#include "Frame.h"

namespace WebCore {

Performance::Performance(Frame* frame)
    : DOMWindowProperty(frame)
{
}

Performance::~Performance()
{
}

const AtomicString& Performance::interfaceName() const
{
    return eventNames().interfaceForPerformance;
}

ScriptExecutionContext* Performance::scriptExecutionContext() const
{
    if (!frame())
        return 0;
    return frame()->document();
}

PassRefPtr<MemoryInfo> Performance::memory() const
{
    return MemoryInfo::create(m_frame);
}

PerformanceNavigation* Performance::navigation() const
{
    if (!m_navigation)
        m_navigation = PerformanceNavigation::create(m_frame);

    return m_navigation.get();
}

PerformanceTiming* Performance::timing() const
{
    if (!m_timing)
        m_timing = PerformanceTiming::create(m_frame);

    return m_timing.get();
}

#if ENABLE(PERFORMANCE_TIMELINE)

PassRefPtr<PerformanceEntryList> Performance::webkitGetEntries() const
{
    RefPtr<PerformanceEntryList> entries = PerformanceEntryList::create();

#if ENABLE(RESOURCE_TIMING)
    entries->appendAll(m_resourceTimingBuffer);
#endif // ENABLE(RESOURCE_TIMING)

    return entries;
}

PassRefPtr<PerformanceEntryList> Performance::webkitGetEntriesByType(const String& entryType)
{
    RefPtr<PerformanceEntryList> entries = PerformanceEntryList::create();

#if ENABLE(RESOURCE_TIMING)
    if (equalIgnoringCase(entryType, "resource"))
        for (Vector<RefPtr<PerformanceEntry> >::const_iterator resource = m_resourceTimingBuffer.begin(); resource != m_resourceTimingBuffer.end(); ++resource)
            entries->append(*resource);
#endif // ENABLE(RESOURCE_TIMING)

    return entries;
}

PassRefPtr<PerformanceEntryList> Performance::webkitGetEntriesByName(const String& name, const String& entryType)
{
    RefPtr<PerformanceEntryList> entries = PerformanceEntryList::create();

#if ENABLE(RESOURCE_TIMING)
    if (entryType.isNull() || equalIgnoringCase(entryType, "resource"))
        for (Vector<RefPtr<PerformanceEntry> >::const_iterator resource = m_resourceTimingBuffer.begin(); resource != m_resourceTimingBuffer.end(); ++resource)
            if ((*resource)->name() == name)
                entries->append(*resource);
#endif // ENABLE(RESOURCE_TIMING)

    return entries;
}

#endif // ENABLE(PERFORMANCE_TIMELINE)

#if ENABLE(RESOURCE_TIMING)

void Performance::webkitClearResourceTimings()
{
    m_resourceTimingBuffer.clear();
}

void Performance::webkitSetResourceTimingBufferSize(unsigned int)
{
    // FIXME: Implement this.
}

void Performance::addResourceTiming(const ResourceRequest& request, const ResourceResponse& response, double finishTime, Document* requestingDocument)
{
    if (!response.resourceLoadTiming())
        return;

    RefPtr<PerformanceEntry> entry = PerformanceResourceTiming::create(request, response, finishTime, requestingDocument);
    // FIXME: Need to enforce buffer limits.
    m_resourceTimingBuffer.append(entry);
}

#endif // ENABLE(RESOURCE_TIMING)

EventTargetData* Performance::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* Performance::ensureEventTargetData()
{
    return &m_eventTargetData;
}

double Performance::webkitNow() const
{
    return 1000.0 * m_frame->document()->loader()->timing()->convertMonotonicTimeToZeroBasedDocumentTime(monotonicallyIncreasingTime());
}

} // namespace WebCore

#endif // ENABLE(WEB_TIMING)
