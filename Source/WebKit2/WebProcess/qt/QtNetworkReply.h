/*
 * Copyright (C) 2011 Zeno Albisser <zeno@webkit.org>
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

#ifndef QtNetworkReply_h
#define QtNetworkReply_h

#include "QtNetworkReplyData.h"
#include "SharedMemory.h"
#include <QByteArray>
#include <QDateTime>
#include <QNetworkReply>

namespace WebKit {

class QtNetworkAccessManager;

class QtNetworkReply final : public QNetworkReply {
public:
    QtNetworkReply(const QNetworkRequest&, QtNetworkAccessManager* parent);

    qint64 readData(char *data, qint64 maxlen) final;
    qint64 bytesAvailable() const final;
    void setReplyData(const QtNetworkReplyData&);
    void finalize();

protected:
    void setData(const SharedMemory::Handle&, qint64 dataSize);

    void abort() final;
    void close() final;
    void setReadBufferSize(qint64) final;
    bool canReadLine() const final;

private:
    qint64 m_bytesAvailable;
    RefPtr<SharedMemory> m_sharedMemory;
    qint64 m_sharedMemorySize;
};

} // namespace WebKit

#endif // QtNetworkReply_h
