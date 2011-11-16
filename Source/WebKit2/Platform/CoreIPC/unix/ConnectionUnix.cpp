/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2011 Igalia S.L.
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

#include "config.h"
#include "Connection.h"

#include "ArgumentEncoder.h"
#include "WorkItem.h"
#include "SharedMemory.h"
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <wtf/Assertions.h>
#include <wtf/OwnArrayPtr.h>

#if PLATFORM(QT)
#include <QSocketNotifier>
#include <QWeakPointer>
#elif PLATFORM(GTK)
#include <glib.h>
#endif

using namespace std;

namespace CoreIPC {

static const size_t messageMaxSize = 4096;
static const size_t attachmentMaxAmount = 255;

enum {
    MessageBodyIsOOL = 1U << 31
};

class MessageInfo {
public:
    MessageInfo() { }

    MessageInfo(MessageID messageID, size_t bodySize, size_t initialAttachmentCount)
        : m_messageID(messageID.toInt())
        , m_bodySize(bodySize)
        , m_attachmentCount(initialAttachmentCount)
    {
        ASSERT(!(m_messageID & MessageBodyIsOOL));
    }

    void setMessageBodyOOL()
    {
        ASSERT(!isMessageBodyOOL());

        m_messageID |= MessageBodyIsOOL;
        m_attachmentCount++;
    }

    bool isMessageBodyOOL() const { return m_messageID & MessageBodyIsOOL; }

    size_t bodySize() const { return m_bodySize; }

    MessageID messageID() const { return MessageID::fromInt(m_messageID & ~MessageBodyIsOOL); }

    size_t attachmentCount() const { return m_attachmentCount; }

private:
    uint32_t m_messageID;
    size_t m_bodySize;
    size_t m_attachmentCount;
};

class AttachmentInfo {
public:
    AttachmentInfo()
        : m_type(Attachment::Uninitialized)
        , m_size(0)
        , m_isNull(false)
    {
    }

    void setType(Attachment::Type type) { m_type = type; }
    Attachment::Type getType() { return m_type; }
    void setSize(size_t size)
    {
        ASSERT(m_type == Attachment::MappedMemoryType);
        m_size = size;
    }

    size_t getSize()
    {
        ASSERT(m_type == Attachment::MappedMemoryType);
        return m_size;
    }

    // The attachment is not null unless explicitly set.
    void setNull() { m_isNull = true; }
    bool isNull() { return m_isNull; }

private:
    Attachment::Type m_type;
    size_t m_size;
    bool m_isNull;
};

void Connection::platformInitialize(Identifier identifier)
{
    m_socketDescriptor = identifier;
    m_readBuffer.resize(messageMaxSize);
    m_readBufferSize = 0;
    m_fileDescriptors.resize(attachmentMaxAmount);
    m_fileDescriptorsSize = 0;

#if PLATFORM(QT)
    m_socketNotifier = 0;
#endif
}

void Connection::platformInvalidate()
{
    if (m_socketDescriptor != -1)
        while (close(m_socketDescriptor) == -1 && errno == EINTR) { }

    if (!m_isConnected)
        return;

#if PLATFORM(GTK)
    m_connectionQueue.unregisterEventSourceHandler(m_socketDescriptor);
#endif

#if PLATFORM(QT)
    delete m_socketNotifier;
    m_socketNotifier = 0;
#endif

    m_socketDescriptor = -1;
    m_isConnected = false;
}

#if PLATFORM(QT)
class SocketNotifierResourceGuard {
public:
    SocketNotifierResourceGuard(QSocketNotifier* socketNotifier)
        : m_socketNotifier(socketNotifier)
    {
        m_socketNotifier.data()->setEnabled(false);
    }

    ~SocketNotifierResourceGuard()
    {
        if (m_socketNotifier)
            m_socketNotifier.data()->setEnabled(true);
    }

private:
    QWeakPointer<QSocketNotifier> const m_socketNotifier;
};
#endif

template<class T, class iterator>
class AttachmentResourceGuard {
public:
    AttachmentResourceGuard(T& attachments)
        : m_attachments(attachments)
    {
    }
    ~AttachmentResourceGuard()
    {
        iterator end = m_attachments.end();
        for (iterator i = m_attachments.begin(); i != end; ++i)
            i->dispose();
    }
private:
    T& m_attachments;
};

bool Connection::processMessage()
{
    if (m_readBufferSize < sizeof(MessageInfo))
        return false;

    uint8_t* messageData = m_readBuffer.data();
    MessageInfo messageInfo;
    memcpy(&messageInfo, messageData, sizeof(messageInfo));
    messageData += sizeof(messageInfo);

    size_t messageLength = sizeof(MessageInfo) + messageInfo.attachmentCount() * sizeof(AttachmentInfo) + (messageInfo.isMessageBodyOOL() ? 0 : messageInfo.bodySize());
    if (m_readBufferSize < messageLength)
        return false;

    Deque<Attachment> attachments;
    AttachmentResourceGuard<Deque<Attachment>, Deque<Attachment>::iterator> attachementDisposer(attachments);
    RefPtr<WebKit::SharedMemory> oolMessageBody;

    size_t attachmentFileDescriptorCount = 0;
    size_t attachmentCount = messageInfo.attachmentCount();
    if (attachmentCount) {
        OwnArrayPtr<AttachmentInfo> attachmentInfo = adoptArrayPtr(new AttachmentInfo[attachmentCount]);
        memcpy(attachmentInfo.get(), messageData, sizeof(AttachmentInfo) * attachmentCount);
        messageData += sizeof(AttachmentInfo) * attachmentCount;

        for (size_t i = 0; i < attachmentCount; ++i) {
            switch (attachmentInfo[i].getType()) {
            case Attachment::MappedMemoryType:
            case Attachment::SocketType:
                if (!attachmentInfo[i].isNull())
                    attachmentFileDescriptorCount++;
            case Attachment::Uninitialized:
            default:
                break;
            }
        }

        if (messageInfo.isMessageBodyOOL())
            attachmentCount--;

        size_t fdIndex = 0;
        for (size_t i = 0; i < attachmentCount; ++i) {
            int fd = -1;
            switch (attachmentInfo[i].getType()) {
            case Attachment::MappedMemoryType:
                if (!attachmentInfo[i].isNull())
                    fd = m_fileDescriptors[fdIndex++];
                attachments.append(Attachment(fd, attachmentInfo[i].getSize()));
                break;
            case Attachment::SocketType:
                if (!attachmentInfo[i].isNull())
                    fd = m_fileDescriptors[fdIndex++];
                attachments.append(Attachment(fd));
                break;
            case Attachment::Uninitialized:
                attachments.append(Attachment());
            default:
                break;
            }
        }

        if (messageInfo.isMessageBodyOOL()) {
            ASSERT(messageInfo.bodySize());

            if (attachmentInfo[attachmentCount].isNull()) {
                ASSERT_NOT_REACHED();
                return false;
            }

            WebKit::SharedMemory::Handle handle;
            handle.adoptFromAttachment(m_fileDescriptors[attachmentFileDescriptorCount - 1], attachmentInfo[attachmentCount].getSize());

            oolMessageBody = WebKit::SharedMemory::create(handle, WebKit::SharedMemory::ReadOnly);
            if (!oolMessageBody) {
                ASSERT_NOT_REACHED();
                return false;
            }
        }
    }

    ASSERT(attachments.size() == messageInfo.isMessageBodyOOL() ? messageInfo.attachmentCount() - 1 : messageInfo.attachmentCount());

    uint8_t* messageBody = messageData;
    if (messageInfo.isMessageBodyOOL())
        messageBody = reinterpret_cast<uint8_t*>(oolMessageBody->data());

    ArgumentDecoder* argumentDecoder;
    if (attachments.isEmpty())
        argumentDecoder = new ArgumentDecoder(messageBody, messageInfo.bodySize());
    else
        argumentDecoder = new ArgumentDecoder(messageBody, messageInfo.bodySize(), attachments);

    processIncomingMessage(messageInfo.messageID(), adoptPtr(argumentDecoder));

    if (m_readBufferSize > messageLength) {
        memmove(m_readBuffer.data(), m_readBuffer.data() + messageLength, m_readBufferSize - messageLength);
        m_readBufferSize -= messageLength;
    } else
        m_readBufferSize = 0;

    if (attachmentFileDescriptorCount) {
        if (m_fileDescriptorsSize > attachmentFileDescriptorCount) {
            size_t fileDescriptorsLength = attachmentFileDescriptorCount * sizeof(int);
            memmove(m_fileDescriptors.data(), m_fileDescriptors.data() + fileDescriptorsLength, m_fileDescriptorsSize - fileDescriptorsLength);
            m_fileDescriptorsSize -= fileDescriptorsLength;
        } else
            m_fileDescriptorsSize = 0;
    }


    return true;
}

static ssize_t readBytesFromSocket(int socketDescriptor, uint8_t* buffer, int count, int* fileDescriptors, size_t* fileDescriptorsCount)
{
    struct msghdr message;
    memset(&message, 0, sizeof(message));

    struct iovec iov[1];
    memset(&iov, 0, sizeof(iov));

    message.msg_controllen = CMSG_SPACE(sizeof(int) * attachmentMaxAmount);
    OwnArrayPtr<char> attachmentDescriptorBuffer = adoptArrayPtr(new char[message.msg_controllen]);
    memset(attachmentDescriptorBuffer.get(), 0, message.msg_controllen);
    message.msg_control = attachmentDescriptorBuffer.get();

    iov[0].iov_base = buffer;
    iov[0].iov_len = count;

    message.msg_iov = iov;
    message.msg_iovlen = 1;

    while (true) {
        ssize_t bytesRead = recvmsg(socketDescriptor, &message, 0);

        if (bytesRead < 0) {
            if (errno == EINTR)
                continue;

            return -1;
        }

        bool found = false;
        struct cmsghdr* controlMessage;
        for (controlMessage = CMSG_FIRSTHDR(&message); controlMessage; controlMessage = CMSG_NXTHDR(&message, controlMessage)) {
            if (controlMessage->cmsg_level == SOL_SOCKET && controlMessage->cmsg_type == SCM_RIGHTS) {
                *fileDescriptorsCount = (controlMessage->cmsg_len - CMSG_LEN(0)) / sizeof(int);
                memcpy(fileDescriptors, CMSG_DATA(controlMessage), sizeof(int) * *fileDescriptorsCount);

                for (size_t i = 0; i < *fileDescriptorsCount; ++i) {
                    while (fcntl(fileDescriptors[i], F_SETFL, FD_CLOEXEC) == -1) {
                        if (errno != EINTR) {
                            ASSERT_NOT_REACHED();
                            break;
                        }
                    }
                }

                found = true;
                break;
            }
        }

        if (!found)
            *fileDescriptorsCount = 0;

        return bytesRead;
    }

    return -1;
}

void Connection::readyReadHandler()
{
#if PLATFORM(QT)
    SocketNotifierResourceGuard socketNotifierEnabler(m_socketNotifier);
#endif

    while (true) {
        size_t fileDescriptorsCount = 0;
        size_t bytesToRead = m_readBuffer.size() - m_readBufferSize;
        ssize_t bytesRead = readBytesFromSocket(m_socketDescriptor, m_readBuffer.data() + m_readBufferSize, bytesToRead,
                                                m_fileDescriptors.data() + m_fileDescriptorsSize, &fileDescriptorsCount);

        if (bytesRead < 0) {
            // EINTR was already handled by readBytesFromSocket.
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;

            // FIXME: Handle other errors here?
            return;
        }

        m_readBufferSize += bytesRead;
        m_fileDescriptorsSize += fileDescriptorsCount;

        if (!bytesRead) {
            connectionDidClose();
            return;
        }

        // Process messages from data received.
        while (true) {
            if (!processMessage())
                break;
        }
    }
}

bool Connection::open()
{
#if PLATFORM(QT)
    ASSERT(!m_socketNotifier);
#endif

    int flags = fcntl(m_socketDescriptor, F_GETFL, 0);
    while (fcntl(m_socketDescriptor, F_SETFL, flags | O_NONBLOCK) == -1) {
        if (errno != EINTR) {
            ASSERT_NOT_REACHED();
            return false;
        }
    }

    m_isConnected = true;
#if PLATFORM(QT)
    m_socketNotifier = m_connectionQueue.registerSocketEventHandler(m_socketDescriptor, QSocketNotifier::Read, WorkItem::create(this, &Connection::readyReadHandler));
#elif PLATFORM(GTK)
    m_connectionQueue.registerEventSourceHandler(m_socketDescriptor, (G_IO_HUP | G_IO_ERR), WorkItem::create(this, &Connection::connectionDidClose));
    m_connectionQueue.registerEventSourceHandler(m_socketDescriptor, G_IO_IN, WorkItem::create(this, &Connection::readyReadHandler));
#endif

    // Schedule a call to readyReadHandler. Data may have arrived before installation of the signal
    // handler.
    m_connectionQueue.scheduleWork(WorkItem::create(this, &Connection::readyReadHandler));

    return true;
}

bool Connection::platformCanSendOutgoingMessages() const
{
    return m_isConnected;
}

bool Connection::sendOutgoingMessage(MessageID messageID, PassOwnPtr<ArgumentEncoder> arguments)
{
#if PLATFORM(QT)
    ASSERT(m_socketNotifier);
#endif

    COMPILE_ASSERT(sizeof(MessageInfo) + attachmentMaxAmount * sizeof(size_t) <= messageMaxSize, AttachmentsFitToMessageInline);

    Vector<Attachment> attachments = arguments->releaseAttachments();
    AttachmentResourceGuard<Vector<Attachment>, Vector<Attachment>::iterator> attachementDisposer(attachments);

    if (attachments.size() > (attachmentMaxAmount - 1)) {
        ASSERT_NOT_REACHED();
        return false;
    }

    MessageInfo messageInfo(messageID, arguments->bufferSize(), attachments.size());
    size_t messageSizeWithBodyInline = sizeof(messageInfo) + (attachments.size() * sizeof(AttachmentInfo)) + arguments->bufferSize();
    if (messageSizeWithBodyInline > messageMaxSize && arguments->bufferSize()) {
        RefPtr<WebKit::SharedMemory> oolMessageBody = WebKit::SharedMemory::create(arguments->bufferSize());
        if (!oolMessageBody)
            return false;

        WebKit::SharedMemory::Handle handle;
        if (!oolMessageBody->createHandle(handle, WebKit::SharedMemory::ReadOnly))
            return false;

        messageInfo.setMessageBodyOOL();

        memcpy(oolMessageBody->data(), arguments->buffer(), arguments->bufferSize());

        attachments.append(handle.releaseToAttachment());
    }

    struct msghdr message;
    memset(&message, 0, sizeof(message));

    struct iovec iov[3];
    memset(&iov, 0, sizeof(iov));

    message.msg_iov = iov;
    int iovLength = 1;

    iov[0].iov_base = reinterpret_cast<void*>(&messageInfo);
    iov[0].iov_len = sizeof(messageInfo);

    OwnArrayPtr<AttachmentInfo> attachmentInfo = adoptArrayPtr(new AttachmentInfo[attachments.size()]);

    size_t attachmentFDBufferLength = 0;
    if (!attachments.isEmpty()) {
        for (size_t i = 0; i < attachments.size(); ++i) {
            if (attachments[i].fileDescriptor() != -1)
                attachmentFDBufferLength++;
        }
    }
    OwnArrayPtr<char> attachmentFDBuffer = adoptArrayPtr(new char[CMSG_SPACE(sizeof(int) * attachmentFDBufferLength)]);

    if (!attachments.isEmpty()) {
        int* fdPtr = 0;

        if (attachmentFDBufferLength) {
            message.msg_control = attachmentFDBuffer.get();
            message.msg_controllen = CMSG_SPACE(sizeof(int) * attachmentFDBufferLength);
            memset(message.msg_control, 0, message.msg_controllen);

            struct cmsghdr* cmsg = CMSG_FIRSTHDR(&message);
            cmsg->cmsg_level = SOL_SOCKET;
            cmsg->cmsg_type = SCM_RIGHTS;
            cmsg->cmsg_len = CMSG_LEN(sizeof(int) * attachmentFDBufferLength);

            fdPtr = reinterpret_cast<int*>(CMSG_DATA(cmsg));
        }

        int fdIndex = 0;
        for (size_t i = 0; i < attachments.size(); ++i) {
            attachmentInfo[i].setType(attachments[i].type());

            switch (attachments[i].type()) {
            case Attachment::MappedMemoryType:
                attachmentInfo[i].setSize(attachments[i].size());
                // Fall trhough, set file descriptor or null.
            case Attachment::SocketType:
                if (attachments[i].fileDescriptor() != -1) {
                    ASSERT(fdPtr);
                    fdPtr[fdIndex++] = attachments[i].fileDescriptor();
                } else
                    attachmentInfo[i].setNull();
                break;
            case Attachment::Uninitialized:
            default:
                break;
            }
        }

        iov[iovLength].iov_base = attachmentInfo.get();
        iov[iovLength].iov_len = sizeof(AttachmentInfo) * attachments.size();
        ++iovLength;
    }

    if (!messageInfo.isMessageBodyOOL() && arguments->bufferSize()) {
        iov[iovLength].iov_base = reinterpret_cast<void*>(arguments->buffer());
        iov[iovLength].iov_len = arguments->bufferSize();
        ++iovLength;
    }

    message.msg_iovlen = iovLength;

    int bytesSent = 0;
    while ((bytesSent = sendmsg(m_socketDescriptor, &message, 0)) == -1) {
        if (errno != EINTR)
            return false;
    }
    return true;
}

#if PLATFORM(QT)
void Connection::setShouldCloseConnectionOnProcessTermination(WebKit::PlatformProcessIdentifier process)
{
    m_connectionQueue.scheduleWorkOnTermination(process, WorkItem::create(this, &Connection::connectionDidClose));
}
#endif

} // namespace CoreIPC
