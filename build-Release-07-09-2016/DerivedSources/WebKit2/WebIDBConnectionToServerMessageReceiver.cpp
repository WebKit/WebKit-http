/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INDEXED_DATABASE) && ENABLE(DATABASE_PROCESS)

#include "WebIDBConnectionToServer.h"

#include "ArgumentCoders.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "SandboxExtension.h"
#include "WebCoreArgumentCoders.h"
#include "WebIDBConnectionToServerMessages.h"
#include <WebCore/IDBError.h>
#include <WebCore/IDBResourceIdentifier.h>
#include <WebCore/IDBResultData.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebIDBConnectionToServer::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidDeleteDatabase::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidDeleteDatabase>(decoder, this, &WebIDBConnectionToServer::didDeleteDatabase);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidOpenDatabase::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidOpenDatabase>(decoder, this, &WebIDBConnectionToServer::didOpenDatabase);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidAbortTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidAbortTransaction>(decoder, this, &WebIDBConnectionToServer::didAbortTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidCommitTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidCommitTransaction>(decoder, this, &WebIDBConnectionToServer::didCommitTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidCreateObjectStore::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidCreateObjectStore>(decoder, this, &WebIDBConnectionToServer::didCreateObjectStore);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidDeleteObjectStore::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidDeleteObjectStore>(decoder, this, &WebIDBConnectionToServer::didDeleteObjectStore);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidClearObjectStore::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidClearObjectStore>(decoder, this, &WebIDBConnectionToServer::didClearObjectStore);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidCreateIndex::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidCreateIndex>(decoder, this, &WebIDBConnectionToServer::didCreateIndex);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidDeleteIndex::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidDeleteIndex>(decoder, this, &WebIDBConnectionToServer::didDeleteIndex);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidPutOrAdd::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidPutOrAdd>(decoder, this, &WebIDBConnectionToServer::didPutOrAdd);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidGetRecord::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidGetRecord>(decoder, this, &WebIDBConnectionToServer::didGetRecord);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidGetRecordWithSandboxExtensions::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidGetRecordWithSandboxExtensions>(decoder, this, &WebIDBConnectionToServer::didGetRecordWithSandboxExtensions);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidGetCount::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidGetCount>(decoder, this, &WebIDBConnectionToServer::didGetCount);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidDeleteRecord::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidDeleteRecord>(decoder, this, &WebIDBConnectionToServer::didDeleteRecord);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidOpenCursor::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidOpenCursor>(decoder, this, &WebIDBConnectionToServer::didOpenCursor);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidIterateCursor::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidIterateCursor>(decoder, this, &WebIDBConnectionToServer::didIterateCursor);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::FireVersionChangeEvent::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::FireVersionChangeEvent>(decoder, this, &WebIDBConnectionToServer::fireVersionChangeEvent);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidStartTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidStartTransaction>(decoder, this, &WebIDBConnectionToServer::didStartTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidCloseFromServer::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidCloseFromServer>(decoder, this, &WebIDBConnectionToServer::didCloseFromServer);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::NotifyOpenDBRequestBlocked::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::NotifyOpenDBRequestBlocked>(decoder, this, &WebIDBConnectionToServer::notifyOpenDBRequestBlocked);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToServer::DidGetAllDatabaseNames::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToServer::DidGetAllDatabaseNames>(decoder, this, &WebIDBConnectionToServer::didGetAllDatabaseNames);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE) && ENABLE(DATABASE_PROCESS)
