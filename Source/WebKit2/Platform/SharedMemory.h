/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SharedMemory_h
#define SharedMemory_h

#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if PLATFORM(QT) || PLATFORM(GTK)
#include "Attachment.h"
#include <wtf/text/WTFString.h>
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

class SharedMemory : public RefCounted<SharedMemory> {
public:
    enum Protection {
        ReadOnly,
        ReadWrite
    };

    class Handle {
        WTF_MAKE_NONCOPYABLE(Handle);
    public:
        Handle();
        ~Handle();

        bool isNull() const;

        void encode(CoreIPC::ArgumentEncoder*) const;
        static bool decode(CoreIPC::ArgumentDecoder*, Handle&);

#if USE(UNIX_DOMAIN_SOCKETS)
        CoreIPC::Attachment releaseToAttachment() const;
        void adoptFromAttachment(int fileDescriptor, size_t);
#endif
    private:
        friend class SharedMemory;
#if PLATFORM(MAC)
        mutable mach_port_t m_port;
#elif PLATFORM(WIN)
        mutable HANDLE m_handle;
#elif USE(UNIX_DOMAIN_SOCKETS)
        mutable int m_fileDescriptor;
#endif
        size_t m_size;
    };
    
    // Create a shared memory object with the given size. Will return 0 on failure.
    static PassRefPtr<SharedMemory> create(size_t);

    // Create a shared memory object from the given handle and the requested protection. Will return 0 on failure.
    static PassRefPtr<SharedMemory> create(const Handle&, Protection);
    
#if PLATFORM(WIN)
    static PassRefPtr<SharedMemory> adopt(HANDLE, size_t, Protection);
#endif

    ~SharedMemory();

    bool createHandle(Handle&, Protection);

    size_t size() const { return m_size; }
    void* data() const { return m_data; }
#if PLATFORM(WIN)
    HANDLE handle() const { return m_handle; }
#endif

    // Creates a copy-on-write copy of the first |size| bytes.
    PassRefPtr<SharedMemory> createCopyOnWriteCopy(size_t) const;

    // Return the system page size in bytes.
    static unsigned systemPageSize();

private:
    size_t m_size;
    void* m_data;
#if PLATFORM(MAC)
    mach_port_t m_port;
#elif PLATFORM(WIN)
    HANDLE m_handle;
#elif USE(UNIX_DOMAIN_SOCKETS)
    int m_fileDescriptor;
#endif
};

};

#endif // SharedMemory_h
