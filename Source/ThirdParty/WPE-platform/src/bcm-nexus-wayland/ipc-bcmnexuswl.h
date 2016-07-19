/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
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

#ifndef wpe_platform_ipc_bcmnexuswl_h
#define wpe_platform_ipc_bcmnexuswl_h

#include <memory>
#include <stdint.h>
#include <cstring>

namespace IPC {

namespace BCMNexusWL {

struct TargetConstruction {
    uint32_t handle;
    uint32_t width;
    uint32_t height;
    uint8_t padding[12];

    static const uint64_t code = 1;
    static void construct(Message& message, uint32_t handle, uint32_t width, uint32_t height)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<TargetConstruction*>(std::addressof(message.messageData));
        messageData.handle = handle;
        messageData.width = width;
        messageData.height = height;
    }
    static TargetConstruction& cast(Message& message)
    {
        return *reinterpret_cast<TargetConstruction*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(TargetConstruction) == Message::dataSize, "TargetConstruction is of correct size");

struct Authentication {
    uint32_t authDataSize;
    uint32_t chunkSize;
    uint8_t data[16];

    static const uint64_t code = 2;
    static const size_t maxChunkSize = 16;
    static void construct(Message& message, size_t authDataSize, size_t chunkSize, const uint8_t* data)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<Authentication*>(std::addressof(message.messageData));
        messageData.authDataSize = authDataSize;
        messageData.chunkSize = chunkSize;
        memcpy(messageData.data, data, chunkSize);
    }
    static Authentication& cast(Message& message)
    {
        return *reinterpret_cast<Authentication*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(Authentication) == Message::dataSize, "Authentication is of correct size");

struct BufferCommit {
    uint32_t width;
    uint32_t height;
    uint8_t padding[16];

    static const uint64_t code = 3;
    static void construct(Message& message, uint32_t width, uint32_t height)
    {
        message.messageCode = code;

        auto& messageData = *reinterpret_cast<BufferCommit*>(std::addressof(message.messageData));
        messageData.width = width;
        messageData.height = height;
    }
    static BufferCommit& cast(Message& message)
    {
        return *reinterpret_cast<BufferCommit*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(BufferCommit) == Message::dataSize, "BufferCommit is of correct size");

struct FrameComplete {
    int8_t padding[24];

    static const uint64_t code = 4;
    static void construct(Message& message)
    {
        message.messageCode = code;
    }
    static FrameComplete& cast(Message& message)
    {
        return *reinterpret_cast<FrameComplete*>(std::addressof(message.messageData));
    }
};
static_assert(sizeof(FrameComplete) == Message::dataSize, "FrameComplete is of correct size");

} // namespace BCMNexusWL

} // namespace IPC

#endif // wpe_platform_ipc_bcmnexuswl_h
