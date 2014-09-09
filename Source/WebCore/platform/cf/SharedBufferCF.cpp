/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "config.h"
#include "SharedBuffer.h"

#include "PurgeableBuffer.h"
#include <wtf/cf/TypeCasts.h>

#if ENABLE(DISK_IMAGE_CACHE)
#include "DiskImageCacheIOS.h"
#endif

namespace WebCore {

SharedBuffer::SharedBuffer(CFDataRef cfData)
    : m_size(0)
    , m_buffer(adoptRef(new DataBuffer))
    , m_shouldUsePurgeableMemory(false)
#if ENABLE(DISK_IMAGE_CACHE)
    , m_isMemoryMapped(false)
    , m_diskImageCacheId(DiskImageCache::invalidDiskCacheId)
    , m_notifyMemoryMappedCallback(nullptr)
    , m_notifyMemoryMappedCallbackData(nullptr)
#endif
    , m_cfData(cfData)
{
}

// Using Foundation allows for an even more efficient implementation of this function,
// so only use this version for non-Foundation.
#if !USE(FOUNDATION)
RetainPtr<CFDataRef> SharedBuffer::createCFData()
{
    if (m_cfData)
        return m_cfData;

    // Internal data in SharedBuffer can be segmented. We need to get the contiguous buffer.
    const Vector<char>& contiguousBuffer = buffer();
    return adoptCF(CFDataCreate(0, reinterpret_cast<const UInt8*>(contiguousBuffer.data()), contiguousBuffer.size()));
}
#endif

PassRefPtr<SharedBuffer> SharedBuffer::wrapCFData(CFDataRef data)
{
    return adoptRef(new SharedBuffer(data));
}

bool SharedBuffer::hasPlatformData() const
{
    return m_cfData;
}

const char* SharedBuffer::platformData() const
{
    return reinterpret_cast<const char*>(CFDataGetBytePtr(m_cfData.get()));
}

unsigned SharedBuffer::platformDataSize() const
{
    return CFDataGetLength(m_cfData.get());
}

void SharedBuffer::maybeTransferPlatformData()
{
    if (!m_cfData)
        return;
    
    ASSERT(!m_size);
    
    // Hang on to the m_cfData pointer in a local pointer as append() will re-enter maybeTransferPlatformData()
    // and we need to make sure to early return when it does.
    RetainPtr<CFDataRef> cfData = adoptCF(m_cfData.leakRef());

    append(reinterpret_cast<const char*>(CFDataGetBytePtr(cfData.get())), CFDataGetLength(cfData.get()));
}

void SharedBuffer::clearPlatformData()
{
    m_cfData = 0;
}

void SharedBuffer::tryReplaceContentsWithPlatformBuffer(SharedBuffer* newContents)
{
    if (!newContents->m_cfData)
        return;

    clear();
    m_cfData = newContents->m_cfData;
}

bool SharedBuffer::maybeAppendPlatformData(SharedBuffer* newContents)
{
    if (size() || !newContents->m_cfData)
        return false;
    m_cfData = newContents->m_cfData;
    return true;
}

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
PassRefPtr<SharedBuffer> SharedBuffer::wrapCFDataArray(CFArrayRef cfDataArray)
{
    return adoptRef(new SharedBuffer(cfDataArray));
}

SharedBuffer::SharedBuffer(CFArrayRef cfDataArray)
    : m_size(0)
    , m_buffer(adoptRef(new DataBuffer))
    , m_shouldUsePurgeableMemory(false)
#if ENABLE(DISK_IMAGE_CACHE)
    , m_isMemoryMapped(false)
    , m_diskImageCacheId(DiskImageCache::invalidDiskCacheId)
    , m_notifyMemoryMappedCallback(nullptr)
    , m_notifyMemoryMappedCallbackData(nullptr)
#endif
    , m_cfData(nullptr)
{
    CFIndex dataArrayCount = CFArrayGetCount(cfDataArray);
    for (CFIndex index = 0; index < dataArrayCount; ++index)
        append(checked_cf_cast<CFDataRef>(CFArrayGetValueAtIndex(cfDataArray, index)));
}

void SharedBuffer::append(CFDataRef data)
{
    ASSERT(data);
    m_dataArray.append(data);
    m_size += CFDataGetLength(data);
}

void SharedBuffer::copyBufferAndClear(char* destination, unsigned bytesToCopy) const
{
    if (m_dataArray.isEmpty())
        return;

    CFIndex bytesLeft = bytesToCopy;
    Vector<RetainPtr<CFDataRef>>::const_iterator end = m_dataArray.end();
    for (Vector<RetainPtr<CFDataRef>>::const_iterator it = m_dataArray.begin(); it != end; ++it) {
        CFIndex dataLen = CFDataGetLength(it->get());
        ASSERT(bytesLeft >= dataLen);
        memcpy(destination, CFDataGetBytePtr(it->get()), dataLen);
        destination += dataLen;
        bytesLeft -= dataLen;
    }
    m_dataArray.clear();
}

unsigned SharedBuffer::copySomeDataFromDataArray(const char*& someData, unsigned position) const
{
    Vector<RetainPtr<CFDataRef>>::const_iterator end = m_dataArray.end();
    unsigned totalOffset = 0;
    for (Vector<RetainPtr<CFDataRef>>::const_iterator it = m_dataArray.begin(); it != end; ++it) {
        unsigned dataLen = static_cast<unsigned>(CFDataGetLength(it->get()));
        ASSERT(totalOffset <= position);
        unsigned localOffset = position - totalOffset;
        if (localOffset < dataLen) {
            someData = reinterpret_cast<const char *>(CFDataGetBytePtr(it->get())) + localOffset;
            return dataLen - localOffset;
        }
        totalOffset += dataLen;
    }
    return 0;
}

const char *SharedBuffer::singleDataArrayBuffer() const
{
    // If we had previously copied data into m_buffer in copyDataArrayAndClear() or some other
    // function, then we can't return a pointer to the CFDataRef buffer.
    if (m_buffer->data.size())
        return 0;

    if (m_dataArray.size() != 1)
        return 0;

    return reinterpret_cast<const char*>(CFDataGetBytePtr(m_dataArray.at(0).get()));
}

bool SharedBuffer::maybeAppendDataArray(SharedBuffer* data)
{
    if (m_buffer->data.size() || m_cfData || !data->m_dataArray.size())
        return false;
#if !ASSERT_DISABLED
    unsigned originalSize = size();
#endif
    for (auto cfData : data->m_dataArray)
        append(cfData.get());
    ASSERT(size() == originalSize + data->size());
    return true;
}
#endif

}
