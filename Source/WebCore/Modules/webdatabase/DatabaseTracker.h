/*
 * Copyright (C) 2007, 2008, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DatabaseTracker_h
#define DatabaseTracker_h

#if ENABLE(SQL_DATABASE)

#include "DatabaseDetails.h"
#include "DatabaseError.h"
#include "SQLiteDatabase.h"
#include "SecurityOriginHash.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class DatabaseBackendBase;
class DatabaseBackendContext;
class DatabaseManagerClient;
class OriginLock;
class SecurityOrigin;

class DatabaseTracker {
    WTF_MAKE_NONCOPYABLE(DatabaseTracker); WTF_MAKE_FAST_ALLOCATED;
public:
    static void initializeTracker(const String& databasePath);

    static DatabaseTracker& tracker();
    // This singleton will potentially be used from multiple worker threads and the page's context thread simultaneously.  To keep this safe, it's
    // currently using 4 locks.  In order to avoid deadlock when taking multiple locks, you must take them in the correct order:
    // m_databaseGuard before quotaManager if both locks are needed.
    // m_openDatabaseMapGuard before quotaManager if both locks are needed.
    // m_databaseGuard and m_openDatabaseMapGuard currently don't overlap.
    // notificationMutex() is currently independent of the other locks.

    bool canEstablishDatabase(DatabaseBackendContext*, const String& name, unsigned long estimatedSize, DatabaseError&);
    bool retryCanEstablishDatabase(DatabaseBackendContext*, const String& name, unsigned long estimatedSize, DatabaseError&);

    void setDatabaseDetails(SecurityOrigin*, const String& name, const String& displayName, unsigned long estimatedSize);
    String fullPathForDatabase(SecurityOrigin*, const String& name, bool createIfDoesNotExist = true);

    void addOpenDatabase(DatabaseBackendBase*);
    void removeOpenDatabase(DatabaseBackendBase*);
    void getOpenDatabases(SecurityOrigin*, const String& name, HashSet<RefPtr<DatabaseBackendBase>>* databases);

    unsigned long long getMaxSizeForDatabase(const DatabaseBackendBase*);

    void interruptAllDatabasesForContext(const DatabaseBackendContext*);

private:
    explicit DatabaseTracker(const String& databasePath);

    bool hasAdequateQuotaForOrigin(SecurityOrigin*, unsigned long estimatedSize, DatabaseError&);

public:
    void setDatabaseDirectoryPath(const String&);
    String databaseDirectoryPath() const;

    void origins(Vector<RefPtr<SecurityOrigin>>& result);
    bool databaseNamesForOrigin(SecurityOrigin*, Vector<String>& result);

    DatabaseDetails detailsForNameAndOrigin(const String&, SecurityOrigin*);

    unsigned long long usageForOrigin(SecurityOrigin*);
    unsigned long long quotaForOrigin(SecurityOrigin*);
    void setQuota(SecurityOrigin*, unsigned long long);
    PassRefPtr<OriginLock> originLockFor(SecurityOrigin*);

    void deleteAllDatabases();
    bool deleteOrigin(SecurityOrigin*);
    bool deleteDatabase(SecurityOrigin*, const String& name);

#if PLATFORM(IOS)
    void removeDeletedOpenedDatabases();
    static bool deleteDatabaseFileIfEmpty(const String&);

    // MobileSafari will grab this mutex on the main thread before dispatching the task to 
    // clean up zero byte database files.  Any operations to open new database will have to
    // wait for that task to finish by waiting on this mutex.
    static Mutex& openDatabaseMutex();
    
    static void emptyDatabaseFilesRemovalTaskWillBeScheduled();
    static void emptyDatabaseFilesRemovalTaskDidFinish();
    
    void setDatabasesPaused(bool);
#endif
    
    void setClient(DatabaseManagerClient*);

    // From a secondary thread, must be thread safe with its data
    void scheduleNotifyDatabaseChanged(SecurityOrigin*, const String& name);

    bool hasEntryForOrigin(SecurityOrigin*);

    void doneCreatingDatabase(DatabaseBackendBase*);

private:
    bool hasEntryForOriginNoLock(SecurityOrigin* origin);
    String fullPathForDatabaseNoLock(SecurityOrigin*, const String& name, bool createIfDoesNotExist);
    bool databaseNamesForOriginNoLock(SecurityOrigin* origin, Vector<String>& resultVector);
    unsigned long long quotaForOriginNoLock(SecurityOrigin* origin);

    String trackerDatabasePath() const;

    enum TrackerCreationAction {
        DontCreateIfDoesNotExist,
        CreateIfDoesNotExist
    };
    void openTrackerDatabase(TrackerCreationAction);

    String originPath(SecurityOrigin*) const;

    bool hasEntryForDatabase(SecurityOrigin*, const String& databaseIdentifier);

    bool addDatabase(SecurityOrigin*, const String& name, const String& path);

    bool deleteDatabaseFile(SecurityOrigin*, const String& name);

    void deleteOriginLockFor(SecurityOrigin*);

    typedef HashSet<DatabaseBackendBase*> DatabaseSet;
    typedef HashMap<String, DatabaseSet*> DatabaseNameMap;
    typedef HashMap<RefPtr<SecurityOrigin>, DatabaseNameMap*> DatabaseOriginMap;

    Mutex m_openDatabaseMapGuard;
    mutable std::unique_ptr<DatabaseOriginMap> m_openDatabaseMap;

    // This lock protects m_database, m_originLockMap, m_databaseDirectoryPath, m_originsBeingDeleted, m_beingCreated, and m_beingDeleted.
    Mutex m_databaseGuard;
    SQLiteDatabase m_database;

    typedef HashMap<String, RefPtr<OriginLock>> OriginLockMap;
    OriginLockMap m_originLockMap;

    String m_databaseDirectoryPath;

    DatabaseManagerClient* m_client;

    typedef HashMap<String, long> NameCountMap;
    typedef HashMap<RefPtr<SecurityOrigin>, NameCountMap*, SecurityOriginHash> CreateSet;
    CreateSet m_beingCreated;
    typedef HashSet<String> NameSet;
    HashMap<RefPtr<SecurityOrigin>, NameSet*> m_beingDeleted;
    HashSet<RefPtr<SecurityOrigin>> m_originsBeingDeleted;
    bool isDeletingDatabaseOrOriginFor(SecurityOrigin*, const String& name);
    void recordCreatingDatabase(SecurityOrigin*, const String& name);
    void doneCreatingDatabase(SecurityOrigin*, const String& name);
    bool creatingDatabase(SecurityOrigin*, const String& name);
    bool canDeleteDatabase(SecurityOrigin*, const String& name);
    void recordDeletingDatabase(SecurityOrigin*, const String& name);
    void doneDeletingDatabase(SecurityOrigin*, const String& name);
    bool isDeletingDatabase(SecurityOrigin*, const String& name);
    bool canDeleteOrigin(SecurityOrigin*);
    bool isDeletingOrigin(SecurityOrigin*);
    void recordDeletingOrigin(SecurityOrigin*);
    void doneDeletingOrigin(SecurityOrigin*);

    static void scheduleForNotification();
    static void notifyDatabasesChanged(void*);
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // DatabaseTracker_h
