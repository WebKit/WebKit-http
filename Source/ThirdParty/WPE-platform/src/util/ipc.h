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

#ifndef wpe_platform_ipc_h
#define wpe_platform_ipc_h

#include <gio/gio.h>
#include <memory>
#include <stdint.h>

namespace IPC {

struct Message {
    static const size_t size = 32;
    static const size_t dataSize = 24;

    uint64_t messageCode { 0 };
    uint8_t messageData[dataSize] { 0, };

    static char* data(Message& message) { return reinterpret_cast<char*>(std::addressof(message)); }
    static Message& cast(char* data) { return *reinterpret_cast<Message*>(data); }
};
static_assert(sizeof(Message) == Message::size, "Message is of correct size");

class Host {
public:
    class Handler {
    public:
        virtual void handleFd(int) = 0;
        virtual void handleMessage(char*, size_t) = 0;
    };

    Host();

    void initialize(Handler&);
    void deinitialize();

    int releaseClientFD();

    void sendMessage(char*, size_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Handler* m_handler;

    GSocket* m_socket;
    GSource* m_source;
    int m_clientFd { -1 };
};

class Client {
public:
    class Handler {
    public:
        virtual void handleMessage(char*, size_t) = 0;
    };

    Client();

    void initialize(Handler&, int);
    void deinitialize();

    void readSynchronously();

    void sendFd(int);
    void sendMessage(char*, size_t);

private:
    static gboolean socketCallback(GSocket*, GIOCondition, gpointer);

    Handler* m_handler;

    GSocket* m_socket;
    GSource* m_source;
};

} // namespace IPC

#endif // wpe_platform_ipc_h
