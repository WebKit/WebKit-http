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

#include "FileReaderLoaderClient.h"
#include "SocketStreamHandleClient.h"
#include "ThreadableWebSocketChannel.h"
#include "Timer.h"
#include "WebSocketHandshake.h"
#include <wtf/Deque.h>
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Blob;
class FileReaderLoader;
class ScriptExecutionContext;
class SocketStreamHandle;
class SocketStreamError;
class WebSocketChannelClient;

class WebSocketChannel : public RefCounted<WebSocketChannel>, public SocketStreamHandleClient, public ThreadableWebSocketChannel
#if ENABLE(BLOB)
                       , public FileReaderLoaderClient
#endif
{
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<WebSocketChannel> create(ScriptExecutionContext* context, WebSocketChannelClient* client) { return adoptRef(new WebSocketChannel(context, client)); }
    virtual ~WebSocketChannel();

    virtual bool useHixie76Protocol();
    virtual void connect(const KURL&, const String& protocol);
    virtual String subprotocol();
    virtual bool send(const String& message);
    virtual bool send(const ArrayBuffer&);
    virtual bool send(const Blob&);
    virtual bool send(const char* data, int length);
    virtual unsigned long bufferedAmount() const;
    virtual void close(int code, const String& reason); // Start closing handshake.
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

    enum CloseEventCode {
        CloseEventCodeNotSpecified = -1,
        CloseEventCodeNormalClosure = 1000,
        CloseEventCodeGoingAway = 1001,
        CloseEventCodeProtocolError = 1002,
        CloseEventCodeUnsupportedData = 1003,
        CloseEventCodeFrameTooLarge = 1004,
        CloseEventCodeNoStatusRcvd = 1005,
        CloseEventCodeAbnormalClosure = 1006,
        CloseEventCodeInvalidUTF8 = 1007,
        CloseEventCodeMinimumUserDefined = 3000,
        CloseEventCodeMaximumUserDefined = 4999
    };

#if ENABLE(BLOB)
    // FileReaderLoaderClient functions.
    virtual void didStartLoading();
    virtual void didReceiveData();
    virtual void didFinishLoading();
    virtual void didFail(int errorCode);
#endif

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
    void resumeTimerFired(Timer<WebSocketChannel>*);
    void startClosingHandshake(int code, const String& reason);
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

    // It is allowed to send a Blob as a binary frame if hybi-10 protocol is in use. Sending a Blob
    // can be delayed because it must be read asynchronously. Other types of data (String or
    // ArrayBuffer) may also be blocked by preceding sending request of a Blob.
    //
    // To address this situation, messages to be sent need to be stored in a queue. Whenever a new
    // data frame is going to be sent, it first must go to the queue. Items in the queue are processed
    // in the order they were put into the queue. Sending request of a Blob blocks further processing
    // until the Blob is completely read and sent to the socket stream.
    //
    // When hixie-76 protocol is chosen, the queue is not used and messages are sent directly.
    enum QueuedFrameType {
        QueuedFrameTypeString,
        QueuedFrameTypeVector,
        QueuedFrameTypeBlob
    };
    struct QueuedFrame {
        OpCode opCode;
        QueuedFrameType frameType;
        // Only one of the following items is used, according to the value of frameType.
        String stringData;
        Vector<char> vectorData;
        RefPtr<Blob> blobData;
    };
    void enqueueTextFrame(const String&);
    void enqueueRawFrame(OpCode, const char* data, size_t dataLength);
    void enqueueBlobFrame(OpCode, const Blob&);

    void processOutgoingFrameQueue();
    void abortOutgoingFrameQueue();

    enum OutgoingFrameQueueStatus {
        // It is allowed to put a new item into the queue.
        OutgoingFrameQueueOpen,
        // Close frame has already been put into the queue but may not have been sent yet;
        // m_handle->close() will be called as soon as the queue is cleared. It is not
        // allowed to put a new item into the queue.
        OutgoingFrameQueueClosing,
        // Close frame has been sent or the queue was aborted. It is not allowed to put
        // a new item to the queue.
        OutgoingFrameQueueClosed
    };

    // If you are going to send a hybi-10 frame, you need to use the outgoing frame queue
    // instead of call sendFrame() directly.
    bool sendFrame(OpCode, const char* data, size_t dataLength);
    bool sendFrameHixie76(const char* data, size_t dataLength);

#if ENABLE(BLOB)
    enum BlobLoaderStatus {
        BlobLoaderNotStarted,
        BlobLoaderStarted,
        BlobLoaderFinished,
        BlobLoaderFailed
    };
#endif

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
    unsigned short m_closeEventCode;
    String m_closeEventReason;

    Deque<OwnPtr<QueuedFrame> > m_outgoingFrameQueue;
    OutgoingFrameQueueStatus m_outgoingFrameQueueStatus;

#if ENABLE(BLOB)
    // FIXME: Load two or more Blobs simultaneously for better performance.
    OwnPtr<FileReaderLoader> m_blobLoader;
    BlobLoaderStatus m_blobLoaderStatus;
#endif
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketChannel_h
