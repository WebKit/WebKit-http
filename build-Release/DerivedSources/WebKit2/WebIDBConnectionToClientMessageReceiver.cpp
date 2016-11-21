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

#include "WebIDBConnectionToClient.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "WebCoreArgumentCoders.h"
#include "WebIDBConnectionToClientMessages.h"
#include <WebCore/IDBCursorInfo.h>
#include <WebCore/IDBGetRecordData.h>
#include <WebCore/IDBIndexInfo.h>
#include <WebCore/IDBKeyData.h>
#include <WebCore/IDBKeyRangeData.h>
#include <WebCore/IDBObjectStoreInfo.h>
#include <WebCore/IDBRequestData.h>
#include <WebCore/IDBResourceIdentifier.h>
#include <WebCore/IDBTransactionInfo.h>
#include <WebCore/IDBValue.h>
#include <WebCore/SecurityOriginData.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebIDBConnectionToClient::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DeleteDatabase::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DeleteDatabase>(decoder, this, &WebIDBConnectionToClient::deleteDatabase);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::OpenDatabase::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::OpenDatabase>(decoder, this, &WebIDBConnectionToClient::openDatabase);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::AbortTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::AbortTransaction>(decoder, this, &WebIDBConnectionToClient::abortTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::CommitTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::CommitTransaction>(decoder, this, &WebIDBConnectionToClient::commitTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DidFinishHandlingVersionChangeTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DidFinishHandlingVersionChangeTransaction>(decoder, this, &WebIDBConnectionToClient::didFinishHandlingVersionChangeTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::CreateObjectStore::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::CreateObjectStore>(decoder, this, &WebIDBConnectionToClient::createObjectStore);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DeleteObjectStore::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DeleteObjectStore>(decoder, this, &WebIDBConnectionToClient::deleteObjectStore);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::ClearObjectStore::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::ClearObjectStore>(decoder, this, &WebIDBConnectionToClient::clearObjectStore);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::CreateIndex::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::CreateIndex>(decoder, this, &WebIDBConnectionToClient::createIndex);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DeleteIndex::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DeleteIndex>(decoder, this, &WebIDBConnectionToClient::deleteIndex);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::PutOrAdd::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::PutOrAdd>(decoder, this, &WebIDBConnectionToClient::putOrAdd);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::GetRecord::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::GetRecord>(decoder, this, &WebIDBConnectionToClient::getRecord);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::GetCount::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::GetCount>(decoder, this, &WebIDBConnectionToClient::getCount);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DeleteRecord::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DeleteRecord>(decoder, this, &WebIDBConnectionToClient::deleteRecord);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::OpenCursor::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::OpenCursor>(decoder, this, &WebIDBConnectionToClient::openCursor);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::IterateCursor::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::IterateCursor>(decoder, this, &WebIDBConnectionToClient::iterateCursor);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::EstablishTransaction::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::EstablishTransaction>(decoder, this, &WebIDBConnectionToClient::establishTransaction);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DatabaseConnectionClosed::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DatabaseConnectionClosed>(decoder, this, &WebIDBConnectionToClient::databaseConnectionClosed);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::AbortOpenAndUpgradeNeeded::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::AbortOpenAndUpgradeNeeded>(decoder, this, &WebIDBConnectionToClient::abortOpenAndUpgradeNeeded);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::DidFireVersionChangeEvent::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::DidFireVersionChangeEvent>(decoder, this, &WebIDBConnectionToClient::didFireVersionChangeEvent);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::OpenDBRequestCancelled::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::OpenDBRequestCancelled>(decoder, this, &WebIDBConnectionToClient::openDBRequestCancelled);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::ConfirmDidCloseFromServer::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::ConfirmDidCloseFromServer>(decoder, this, &WebIDBConnectionToClient::confirmDidCloseFromServer);
        return;
    }
    if (decoder.messageName() == Messages::WebIDBConnectionToClient::GetAllDatabaseNames::name()) {
        IPC::handleMessage<Messages::WebIDBConnectionToClient::GetAllDatabaseNames>(decoder, this, &WebIDBConnectionToClient::getAllDatabaseNames);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE) && ENABLE(DATABASE_PROCESS)
