/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#ifndef WebSocketChannel_h
#define WebSocketChannel_h

#if ENABLE(WEB_SOCKETS)

#include "SocketStreamHandleClient.h"
#include "ThreadableWebSocketChannel.h"
#include "Timer.h"
#include "WebSocketHandshake.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

    class ScriptExecutionContext;
    class SocketStreamHandle;
    class SocketStreamError;
    class WebSocketChannelClient;

    class WebSocketChannel : public RefCounted<WebSocketChannel>, public SocketStreamHandleClient, public ThreadableWebSocketChannel {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static PassRefPtr<WebSocketChannel> create(ScriptExecutionContext* context, WebSocketChannelClient* client) { return adoptRef(new WebSocketChannel(context, client)); }
        virtual ~WebSocketChannel();

        virtual bool useHixie76Protocol();
        virtual void connect(const KURL&, const String& protocol);
        virtual String subprotocol();
        virtual bool send(const String& message);
        virtual unsigned long bufferedAmount() const;
        virtual void close(); // Start closing handshake.
        virtual void fail(const String& reason);
        virtual void disconnect();

        virtual void suspend();
        virtual void resume();

        // SocketStreamHandleClient functions.
        virtual void didOpenSocketStream(SocketStreamHandle*);
        virtual void didCloseSocketStream(SocketStreamHandle*);
        virtual void didReceiveSocketStreamData(SocketStreamHandle*, const char*, int);
        virtual void didFailSocketStream(SocketStreamHandle*, const SocketStreamError&);
        virtual void didReceiveAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&);
        virtual void didCancelAuthenticationChallenge(SocketStreamHandle*, const AuthenticationChallenge&);

        using RefCounted<WebSocketChannel>::ref;
        using RefCounted<WebSocketChannel>::deref;

    protected:
        virtual void refThreadableWebSocketChannel() { ref(); }
        virtual void derefThreadableWebSocketChannel() { deref(); }

    private:
        WebSocketChannel(ScriptExecutionContext*, WebSocketChannelClient*);

        bool appendToBuffer(const char* data, size_t len);
        void skipBuffer(size_t len);
        bool processBuffer();
        void resumeTimerFired(Timer<WebSocketChannel>* timer);
        void startClosingHandshake();
        void closingTimerFired(Timer<WebSocketChannel>*);

        // Hybi-10 opcodes.
        typedef unsigned int OpCode;
        static const OpCode OpCodeContinuation;
        static const OpCode OpCodeText;
        static const OpCode OpCodeBinary;
        static const OpCode OpCodeClose;
        static const OpCode OpCodePing;
        static const OpCode OpCodePong;

        static bool isNonControlOpCode(OpCode opCode) { return opCode == OpCodeContinuation || opCode == OpCodeText || opCode == OpCodeBinary; }
        static bool isControlOpCode(OpCode opCode) { return opCode == OpCodeClose || opCode == OpCodePing || opCode == OpCodePong; }
        static bool isReservedOpCode(OpCode opCode) { return !isNonControlOpCode(opCode) && !isControlOpCode(opCode); }

        enum ParseFrameResult {
            FrameOK,
            FrameIncomplete,
            FrameError
        };

        struct FrameData {
            OpCode opCode;
            bool final;
            bool reserved1;
            bool reserved2;
            bool reserved3;
            bool masked;
            const char* payload;
            size_t payloadLength;
            const char* frameEnd;
        };

        ParseFrameResult parseFrame(FrameData&); // May modify part of m_buffer to unmask the frame.

        bool processFrame();
        bool processFrameHixie76();

        bool sendFrame(OpCode, const char* data, size_t dataLength);
        bool sendFrameHixie76(const char* data, size_t dataLength);

        ScriptExecutionContext* m_context;
        WebSocketChannelClient* m_client;
        OwnPtr<WebSocketHandshake> m_handshake;
        RefPtr<SocketStreamHandle> m_handle;
        char* m_buffer;
        size_t m_bufferSize;

        Timer<WebSocketChannel> m_resumeTimer;
        bool m_suspended;
        bool m_closing;
        bool m_receivedClosingHandshake;
        Timer<WebSocketChannel> m_closingTimer;
        bool m_closed;
        bool m_shouldDiscardReceivedData;
        unsigned long m_unhandledBufferedAmount;

        unsigned long m_identifier; // m_identifier == 0 means that we could not obtain a valid identifier.

        bool m_useHixie76Protocol;

        // Private members only for hybi-10 protocol.
        bool m_hasContinuousFrame;
        OpCode m_continuousFrameOpCode;
        Vector<char> m_continuousFrameData;
    };

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketChannel_h
