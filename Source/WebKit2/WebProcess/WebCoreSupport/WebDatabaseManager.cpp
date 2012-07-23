/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebDatabaseManager.h"

#if ENABLE(SQL_DATABASE)

#include "Connection.h"
#include "MessageID.h"
#include "OriginAndDatabases.h"
#include "WebCoreArgumentCoders.h"
#include "WebDatabaseManagerProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/DatabaseDetails.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/SecurityOrigin.h>

using namespace WebCore;

namespace WebKit {

WebDatabaseManager& WebDatabaseManager::shared()
{
    static WebDatabaseManager& shared = *new WebDatabaseManager;
    return shared;
}

void WebDatabaseManager::initialize(const String& databaseDirectory)
{
    DatabaseTracker::initializeTracker(databaseDirectory);
}

WebDatabaseManager::WebDatabaseManager()
{
    DatabaseTracker::tracker().setClient(this);
}

WebDatabaseManager::~WebDatabaseManager()
{
}

void WebDatabaseManager::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebDatabaseManagerMessage(connection, messageID, arguments);
}

void WebDatabaseManager::getDatabasesByOrigin(uint64_t callbackID) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    // FIXME: This could be made more efficient by adding a function to DatabaseTracker
    // to get both the origins and the Vector of DatabaseDetails for each origin in one
    // shot.  That would avoid taking the numerous locks this requires.

    Vector<RefPtr<SecurityOrigin> > origins;
    DatabaseTracker::tracker().origins(origins);

    Vector<OriginAndDatabases> originAndDatabasesVector;
    originAndDatabasesVector.reserveInitialCapacity(origins.size());

    for (size_t i = 0; i < origins.size(); ++i) {
        OriginAndDatabases originAndDatabases;

        Vector<String> nameVector;
        if (!DatabaseTracker::tracker().databaseNamesForOrigin(origins[i].get(), nameVector))
            continue;

        Vector<DatabaseDetails> detailsVector;
        detailsVector.reserveInitialCapacity(nameVector.size());
        for (size_t j = 0; j < nameVector.size(); j++) {
            DatabaseDetails details = DatabaseTracker::tracker().detailsForNameAndOrigin(nameVector[j], origins[i].get());
            if (details.name().isNull())
                continue;

            detailsVector.append(details);
        }

        if (detailsVector.isEmpty())
            continue;

        originAndDatabases.originIdentifier = origins[i]->databaseIdentifier();
        originAndDatabases.originQuota = DatabaseTracker::tracker().quotaForOrigin(origins[i].get());
        originAndDatabases.originUsage = DatabaseTracker::tracker().usageForOrigin(origins[i].get());
        originAndDatabases.databases.swap(detailsVector); 
        originAndDatabasesVector.append(originAndDatabases);
    }

    WebProcess::shared().connection()->send(Messages::WebDatabaseManagerProxy::DidGetDatabasesByOrigin(originAndDatabasesVector, callbackID), 0);
}

void WebDatabaseManager::getDatabaseOrigins(uint64_t callbackID) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    Vector<RefPtr<SecurityOrigin> > origins;
    DatabaseTracker::tracker().origins(origins);

    size_t numOrigins = origins.size();

    Vector<String> identifiers(numOrigins);
    for (size_t i = 0; i < numOrigins; ++i)
        identifiers[i] = origins[i]->databaseIdentifier();
    WebProcess::shared().connection()->send(Messages::WebDatabaseManagerProxy::DidGetDatabaseOrigins(identifiers, callbackID), 0);
}

void WebDatabaseManager::deleteDatabaseWithNameForOrigin(const String& databaseIdentifier, const String& originIdentifier) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);
    if (!origin)
        return;

    DatabaseTracker::tracker().deleteDatabase(origin.get(), databaseIdentifier);
}

void WebDatabaseManager::deleteDatabasesForOrigin(const String& originIdentifier) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);
    if (!origin)
        return;

    DatabaseTracker::tracker().deleteOrigin(origin.get());
}

void WebDatabaseManager::deleteAllDatabases() const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    DatabaseTracker::tracker().deleteAllDatabases();
}

void WebDatabaseManager::setQuotaForOrigin(const String& originIdentifier, unsigned long long quota) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    // If the quota is set to a value lower than the current usage, that quota will
    // "stick" but no data will be purged to meet the new quota. This will simply
    // prevent new data from being added to databases in that origin.

    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromDatabaseIdentifier(originIdentifier);
    if (!origin)
        return;

    DatabaseTracker::tracker().setQuota(origin.get(), quota);
}

void WebDatabaseManager::dispatchDidModifyOrigin(SecurityOrigin* origin)
{
    // NOTE: This may be called on a non-main thread.
    WebProcess::shared().connection()->send(Messages::WebDatabaseManagerProxy::DidModifyOrigin(origin->databaseIdentifier()), 0);
}

void WebDatabaseManager::dispatchDidModifyDatabase(WebCore::SecurityOrigin* origin, const String& databaseIdentifier)
{
    // NOTE: This may be called on a non-main thread.
    WebProcess::shared().connection()->send(Messages::WebDatabaseManagerProxy::DidModifyDatabase(origin->databaseIdentifier(), databaseIdentifier), 0);
}

} // namespace WebKit

#endif // ENABLE(SQL_DATABASE)
