/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef wpe_mesa_ipc_gbm_h
#define wpe_mesa_ipc_gbm_h

#include <memory>
#include <stdint.h>

namespace IPC {

namespace GBM {

struct BufferCommit {
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t format;
    uint8_t padding[4];

    static const uint64_t code = 42;
    static void construct(Message& message, uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<BufferCommit*>(std::addressof(message.messageData));
        messageData.handle = handle;
        messageData.width = width;
        messageData.height = height;
        messageData.stride = stride;
        messageData.format = format;
    }
    static BufferCommit& cast(Message& message)
    {
        return *reinterpret_cast<BufferCommit*>(message.messageData);
    }
};
static_assert(sizeof(BufferCommit) == Message::dataSize, "BufferCommit is of correct size");

struct FrameComplete {
    uint8_t padding[24];

    static const uint64_t code = 23;
    static void construct(Message& message)
    {
        message.messageCode = code;
    }
};
static_assert(sizeof(FrameComplete) == Message::dataSize, "FrameComplete is of correct size");

struct ReleaseBuffer {
    uint32_t handle;
    uint8_t padding[20];

    static const uint64_t code = 16;
    static void construct(Message& message, uint32_t handle)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<ReleaseBuffer*>(std::addressof(message.messageData));
        messageData.handle = handle;
    }
    static ReleaseBuffer& cast(Message& message)
    {
        return *reinterpret_cast<ReleaseBuffer*>(message.messageData);
    }
};
static_assert(sizeof(ReleaseBuffer) == Message::dataSize, "ReleaseBuffer is of correct size");

} // namespace GBM

} // namespace IPC

#endif // wpe_mesa_ipc_gbm_h
