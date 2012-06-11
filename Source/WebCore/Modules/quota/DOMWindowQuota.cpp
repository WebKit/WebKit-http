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
#include "DOMWindowQuota.h"

#if ENABLE(QUOTA)

#include "DOMWindow.h"
#include "StorageInfo.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

DOMWindowQuota::DOMWindowQuota(DOMWindow* window)
    : DOMWindowProperty(window->frame())
{
}

DOMWindowQuota::~DOMWindowQuota()
{
}

// static
DOMWindowQuota* DOMWindowQuota::from(DOMWindow* window)
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("DOMWindowQuota"));
    DOMWindowQuota* supplement = static_cast<DOMWindowQuota*>(Supplement<DOMWindow>::from(window, name));
    if (!supplement) {
        supplement = new DOMWindowQuota(window);
        provideTo(window, name, adoptPtr(supplement));
    }
    return supplement;
}

// static
StorageInfo* DOMWindowQuota::webkitStorageInfo(DOMWindow* window)
{
    return DOMWindowQuota::from(window)->webkitStorageInfo();
}

StorageInfo* DOMWindowQuota::webkitStorageInfo() const
{
    if (!m_storageInfo && frame())
        m_storageInfo = StorageInfo::create();
    return m_storageInfo.get();
}

} // namespace WebCore

#endif // ENABLE(QUOTA)
