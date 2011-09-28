/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
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

#if ENABLE(FILE_SYSTEM)

#include "FileWriter.h"

#include "AsyncFileWriter.h"
#include "Blob.h"
#include "ExceptionCode.h"
#include "FileError.h"
#include "FileException.h"
#include "ProgressEvent.h"

namespace WebCore {

static const int kMaxRecursionDepth = 3;

FileWriter::FileWriter(ScriptExecutionContext* context)
    : ActiveDOMObject(context, this)
    , m_readyState(INIT)
    , m_isOperationInProgress(false)
    , m_queuedOperation(OperationNone)
    , m_bytesWritten(0)
    , m_bytesToWrite(0)
    , m_truncateLength(-1)
    , m_numAborts(0)
    , m_recursionDepth(0)
{
}

FileWriter::~FileWriter()
{
    ASSERT(!m_recursionDepth);
    if (m_readyState == WRITING)
        stop();
}

bool FileWriter::hasPendingActivity() const
{
    return m_readyState == WRITING || ActiveDOMObject::hasPendingActivity();
}

bool FileWriter::canSuspend() const
{
    // FIXME: It is not currently possible to suspend a FileWriter, so pages with FileWriter can not go into page cache.
    return false;
}

void FileWriter::stop()
{
    // Make sure we've actually got something to stop, and haven't already called abort().
    if (writer() && m_readyState == WRITING && m_isOperationInProgress && m_queuedOperation == OperationNone)
        writer()->abort();
    m_blobBeingWritten.clear();
    m_readyState = DONE;
}

void FileWriter::write(Blob* data, ExceptionCode& ec)
{
    ASSERT(writer());
    ASSERT(data);
    ASSERT(m_truncateLength == -1);
    if (m_readyState == WRITING) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }
    if (!data) {
        setError(FileError::TYPE_MISMATCH_ERR, ec);
        return;
    }
    if (m_recursionDepth > kMaxRecursionDepth) {
        setError(FileError::SECURITY_ERR, ec);
        return;
    }
    m_blobBeingWritten = data;
    m_readyState = WRITING;
    m_bytesWritten = 0;
    m_bytesToWrite = data->size();
    ASSERT(m_queuedOperation == OperationNone);
    if (m_isOperationInProgress) // We must be waiting for an abort to complete, since m_readyState wasn't WRITING.
        m_queuedOperation = OperationWrite;
    else
        writer()->write(position(), m_blobBeingWritten.get());
    m_isOperationInProgress = true;
    fireEvent(eventNames().writestartEvent);
}

void FileWriter::seek(long long position, ExceptionCode& ec)
{
    ASSERT(writer());
    if (m_readyState == WRITING) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }

    ASSERT(m_truncateLength == -1);
    ASSERT(m_queuedOperation == OperationNone);
    seekInternal(position);
}

void FileWriter::truncate(long long position, ExceptionCode& ec)
{
    ASSERT(writer());
    ASSERT(m_truncateLength == -1);
    if (m_readyState == WRITING || position < 0) {
        setError(FileError::INVALID_STATE_ERR, ec);
        return;
    }
    if (m_recursionDepth > kMaxRecursionDepth) {
        setError(FileError::SECURITY_ERR, ec);
        return;
    }
    m_readyState = WRITING;
    m_bytesWritten = 0;
    m_bytesToWrite = 0;
    m_truncateLength = position;
    ASSERT(m_queuedOperation == OperationNone);
    if (m_isOperationInProgress) // We must be waiting for an abort to complete.
        m_queuedOperation = OperationTruncate;
    else
        writer()->truncate(m_truncateLength);
    m_isOperationInProgress = true;
    fireEvent(eventNames().writestartEvent);
}

void FileWriter::abort(ExceptionCode& ec)
{
    ASSERT(writer());
    if (m_readyState != WRITING) {
        return;
    }
    ++m_numAborts;

    if (m_isOperationInProgress)
        writer()->abort();
    m_queuedOperation = OperationNone;
    m_blobBeingWritten.clear();
    m_truncateLength = -1;
    signalCompletion(FileError::ABORT_ERR);
}

void FileWriter::didWrite(long long bytes, bool complete)
{
    ASSERT(m_readyState == WRITING);
    ASSERT(m_truncateLength == -1);
    ASSERT(m_isOperationInProgress);
    ASSERT(bytes + m_bytesWritten > 0);
    ASSERT(bytes + m_bytesWritten <= m_bytesToWrite);
    m_bytesWritten += bytes;
    ASSERT((m_bytesWritten == m_bytesToWrite) || !complete);
    setPosition(position() + bytes);
    if (position() > length())
        setLength(position());
    // TODO: Throttle to no more frequently than every 50ms.
    int numAborts = m_numAborts;
    fireEvent(eventNames().progressEvent);
    // We could get an abort in the handler for this event. If we do, it's
    // already handled the cleanup and signalCompletion call.
    if (complete && numAborts == m_numAborts) {
        m_blobBeingWritten.clear();
        m_isOperationInProgress = false;
        signalCompletion(FileError::OK);
    }
}

void FileWriter::didTruncate()
{
    ASSERT(m_truncateLength >= 0);
    setLength(m_truncateLength);
    if (position() > length())
        setPosition(length());
    m_isOperationInProgress = false;
    signalCompletion(FileError::OK);
}

void FileWriter::didFail(FileError::ErrorCode code)
{
    ASSERT(m_isOperationInProgress);
    m_isOperationInProgress = false;
    ASSERT(code != FileError::OK);
    if (code == FileError::ABORT_ERR) {
        Operation operation = m_queuedOperation;
        m_queuedOperation = OperationNone;
        doOperation(operation);
    } else {
        ASSERT(m_queuedOperation == OperationNone);
        ASSERT(m_readyState == WRITING);
        m_blobBeingWritten.clear();
        signalCompletion(code);
    }
}

void FileWriter::doOperation(Operation operation)
{
    ASSERT(m_queuedOperation == OperationNone);
    ASSERT(!m_isOperationInProgress);
    ASSERT(operation == OperationNone || operation == OperationWrite || operation == OperationTruncate);
    switch (operation) {
    case OperationWrite:
        ASSERT(m_truncateLength == -1);
        ASSERT(m_blobBeingWritten.get());
        ASSERT(m_readyState == WRITING);
        writer()->write(position(), m_blobBeingWritten.get());
        m_isOperationInProgress = true;
        break;
    case OperationTruncate:
        ASSERT(m_truncateLength >= 0);
        ASSERT(m_readyState == WRITING);
        writer()->truncate(m_truncateLength);
        m_isOperationInProgress = true;
        break;
    case OperationNone:
        ASSERT(m_truncateLength == -1);
        ASSERT(!m_blobBeingWritten.get());
        ASSERT(m_readyState == DONE);
        break;
    }
}

void FileWriter::signalCompletion(FileError::ErrorCode code)
{
    m_readyState = DONE;
    m_truncateLength = -1;
    if (FileError::OK != code) {
        m_error = FileError::create(code);
        if (FileError::ABORT_ERR == code)
            fireEvent(eventNames().abortEvent);
        else
            fireEvent(eventNames().errorEvent);
    } else
        fireEvent(eventNames().writeEvent);
    fireEvent(eventNames().writeendEvent);
}

void FileWriter::fireEvent(const AtomicString& type)
{
    ++m_recursionDepth;
    dispatchEvent(ProgressEvent::create(type, true, m_bytesWritten, m_bytesToWrite));
    --m_recursionDepth;
    ASSERT(m_recursionDepth >= 0);
}

void FileWriter::setError(FileError::ErrorCode errorCode, ExceptionCode& ec)
{
    ec = FileException::ErrorCodeToExceptionCode(errorCode);
    m_error = FileError::create(errorCode);
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
