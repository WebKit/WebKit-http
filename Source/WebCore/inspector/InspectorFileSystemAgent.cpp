/*
 * Copyright (C) 2011, 2012 Google Inc. All rights reserved.
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

#if ENABLE(INSPECTOR) && ENABLE(FILE_SYSTEM)

#include "InspectorFileSystemAgent.h"

#include "Base64.h"
#include "DOMFileSystem.h"
#include "DirectoryEntry.h"
#include "DirectoryReader.h"
#include "Document.h"
#include "EntriesCallback.h"
#include "Entry.h"
#include "EntryArray.h"
#include "EntryCallback.h"
#include "ErrorCallback.h"
#include "FileCallback.h"
#include "FileEntry.h"
#include "FileError.h"
#include "FileReader.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "Frame.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "KURL.h"
#include "LocalFileSystem.h"
#include "MIMETypeRegistry.h"
#include "Metadata.h"
#include "MetadataCallback.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

using WebCore::TypeBuilder::Array;

namespace WebCore {

namespace FileSystemAgentState {
static const char fileSystemAgentEnabled[] = "fileSystemAgentEnabled";
}

class InspectorFileSystemAgent::FrontendProvider : public RefCounted<FrontendProvider> {
    WTF_MAKE_NONCOPYABLE(FrontendProvider);
public:
    static PassRefPtr<FrontendProvider> create(InspectorFileSystemAgent* agent, InspectorFrontend::FileSystem* frontend)
    {
        return adoptRef(new FrontendProvider(agent, frontend));
    }

    InspectorFrontend::FileSystem* frontend() const
    {
        if (m_agent && m_agent->m_enabled)
            return m_frontend;
        return 0;
    }

    void clear()
    {
        m_agent = 0;
        m_frontend = 0;
    }

private:
    FrontendProvider(InspectorFileSystemAgent* agent, InspectorFrontend::FileSystem* frontend)
        : m_agent(agent)
        , m_frontend(frontend) { }

    InspectorFileSystemAgent* m_agent;
    InspectorFrontend::FileSystem* m_frontend;
};

typedef InspectorFileSystemAgent::FrontendProvider FrontendProvider;

namespace {

template<typename BaseCallback, typename Handler, typename Argument>
class CallbackDispatcher : public BaseCallback {
public:
    typedef bool (Handler::*HandlingMethod)(Argument*);

    static PassRefPtr<CallbackDispatcher> create(PassRefPtr<Handler> handler, HandlingMethod handlingMethod)
    {
        return adoptRef(new CallbackDispatcher(handler, handlingMethod));
    }

    virtual bool handleEvent(Argument* argument) OVERRIDE
    {
        return (m_handler.get()->*m_handlingMethod)(argument);
    }

private:
    CallbackDispatcher(PassRefPtr<Handler> handler, HandlingMethod handlingMethod)
        : m_handler(handler)
        , m_handlingMethod(handlingMethod) { }

    RefPtr<Handler> m_handler;
    HandlingMethod m_handlingMethod;
};

template<typename BaseCallback>
class CallbackDispatcherFactory {
public:
    template<typename Handler, typename Argument>
    static PassRefPtr<CallbackDispatcher<BaseCallback, Handler, Argument> > create(Handler* handler, bool (Handler::*handlingMethod)(Argument*))
    {
        return CallbackDispatcher<BaseCallback, Handler, Argument>::create(PassRefPtr<Handler>(handler), handlingMethod);
    }
};

class GetFileSystemRootTask : public RefCounted<GetFileSystemRootTask> {
    WTF_MAKE_NONCOPYABLE(GetFileSystemRootTask);
public:
    static PassRefPtr<GetFileSystemRootTask> create(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& type)
    {
        return adoptRef(new GetFileSystemRootTask(frontendProvider, requestId, type));
    }

    void start(ScriptExecutionContext*);

private:
    bool didHitError(FileError*);
    bool didGetEntry(Entry*);

    void reportResult(FileError::ErrorCode errorCode, PassRefPtr<TypeBuilder::FileSystem::Entry> entry)
    {
        if (!m_frontendProvider || !m_frontendProvider->frontend())
            return;
        m_frontendProvider->frontend()->fileSystemRootReceived(m_requestId, static_cast<int>(errorCode), entry);
        m_frontendProvider = 0;
    }

    GetFileSystemRootTask(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& type)
        : m_frontendProvider(frontendProvider)
        , m_requestId(requestId)
        , m_type(type) { }

    RefPtr<FrontendProvider> m_frontendProvider;
    int m_requestId;
    String m_type;
};

bool GetFileSystemRootTask::didHitError(FileError* error)
{
    reportResult(error->code(), 0);
    return true;
}

void GetFileSystemRootTask::start(ScriptExecutionContext* scriptExecutionContext)
{
    FileSystemType type;
    if (m_type == DOMFileSystemBase::persistentPathPrefix)
        type = FileSystemTypePersistent;
    else if (m_type == DOMFileSystemBase::temporaryPathPrefix)
        type = FileSystemTypeTemporary;
    else {
        reportResult(FileError::SYNTAX_ERR, 0);
        return;
    }

    RefPtr<EntryCallback> successCallback = CallbackDispatcherFactory<EntryCallback>::create(this, &GetFileSystemRootTask::didGetEntry);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &GetFileSystemRootTask::didHitError);
    OwnPtr<ResolveURICallbacks> fileSystemCallbacks = ResolveURICallbacks::create(successCallback, errorCallback, scriptExecutionContext, type, "/");

    LocalFileSystem::localFileSystem().readFileSystem(scriptExecutionContext, type, fileSystemCallbacks.release());
}

bool GetFileSystemRootTask::didGetEntry(Entry* entry)
{
    RefPtr<TypeBuilder::FileSystem::Entry> result(TypeBuilder::FileSystem::Entry::create().setUrl(entry->toURL()).setName("/").setIsDirectory(true));
    reportResult(static_cast<FileError::ErrorCode>(0), result);
    return true;
}

class ReadDirectoryTask : public RefCounted<ReadDirectoryTask> {
    WTF_MAKE_NONCOPYABLE(ReadDirectoryTask);
public:
    static PassRefPtr<ReadDirectoryTask> create(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& url)
    {
        return adoptRef(new ReadDirectoryTask(frontendProvider, requestId, url));
    }

    virtual ~ReadDirectoryTask()
    {
        reportResult(FileError::ABORT_ERR, 0);
    }

    void start(ScriptExecutionContext*);

private:
    bool didHitError(FileError* error)
    {
        reportResult(error->code(), 0);
        return true;
    }

    bool didGetEntry(Entry*);
    bool didReadDirectoryEntries(EntryArray*);

    void reportResult(FileError::ErrorCode errorCode, PassRefPtr<Array<TypeBuilder::FileSystem::Entry> > entries)
    {
        if (!m_frontendProvider || !m_frontendProvider->frontend())
            return;
        m_frontendProvider->frontend()->directoryContentReceived(m_requestId, static_cast<int>(errorCode), entries);
        m_frontendProvider = 0;
    }

    ReadDirectoryTask(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& url)
        : m_frontendProvider(frontendProvider)
        , m_requestId(requestId)
        , m_url(ParsedURLString, url) { }

    void readDirectoryEntries();

    RefPtr<FrontendProvider> m_frontendProvider;
    int m_requestId;
    KURL m_url;
    RefPtr<Array<TypeBuilder::FileSystem::Entry> > m_entries;
    RefPtr<DirectoryReader> m_directoryReader;
};

void ReadDirectoryTask::start(ScriptExecutionContext* scriptExecutionContext)
{
    ASSERT(scriptExecutionContext);

    FileSystemType type;
    String path;
    if (!DOMFileSystemBase::crackFileSystemURL(m_url, type, path)) {
        reportResult(FileError::SYNTAX_ERR, 0);
        return;
    }

    RefPtr<EntryCallback> successCallback = CallbackDispatcherFactory<EntryCallback>::create(this, &ReadDirectoryTask::didGetEntry);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &ReadDirectoryTask::didHitError);
    OwnPtr<ResolveURICallbacks> fileSystemCallbacks = ResolveURICallbacks::create(successCallback, errorCallback, scriptExecutionContext, type, path);

    LocalFileSystem::localFileSystem().readFileSystem(scriptExecutionContext, type, fileSystemCallbacks.release());
}

bool ReadDirectoryTask::didGetEntry(Entry* entry)
{
    if (!entry->isDirectory()) {
        reportResult(FileError::TYPE_MISMATCH_ERR, 0);
        return true;
    }

    m_directoryReader = static_cast<DirectoryEntry*>(entry)->createReader();
    m_entries = Array<TypeBuilder::FileSystem::Entry>::create();
    readDirectoryEntries();
    return true;
}

void ReadDirectoryTask::readDirectoryEntries()
{
    if (!m_directoryReader->filesystem()->scriptExecutionContext()) {
        reportResult(FileError::ABORT_ERR, 0);
        return;
    }

    RefPtr<EntriesCallback> successCallback = CallbackDispatcherFactory<EntriesCallback>::create(this, &ReadDirectoryTask::didReadDirectoryEntries);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &ReadDirectoryTask::didHitError);
    m_directoryReader->readEntries(successCallback, errorCallback);
}

bool ReadDirectoryTask::didReadDirectoryEntries(EntryArray* entries)
{
    if (!entries->length()) {
        reportResult(static_cast<FileError::ErrorCode>(0), m_entries);
        return true;
    }

    for (unsigned i = 0; i < entries->length(); ++i) {
        Entry* entry = entries->item(i);
        RefPtr<TypeBuilder::FileSystem::Entry> entryForFrontend = TypeBuilder::FileSystem::Entry::create().setUrl(entry->toURL()).setName(entry->name()).setIsDirectory(entry->isDirectory());

        using TypeBuilder::Page::ResourceType;
        if (!entry->isDirectory()) {
            String mimeType = MIMETypeRegistry::getMIMETypeForPath(entry->name());
            ResourceType::Enum resourceType;
            if (MIMETypeRegistry::isSupportedImageMIMEType(mimeType))
                resourceType = ResourceType::Image;
            else if (MIMETypeRegistry::isSupportedJavaScriptMIMEType(mimeType))
                resourceType = ResourceType::Script;
            else if (MIMETypeRegistry::isSupportedNonImageMIMEType(mimeType))
                resourceType = ResourceType::Document;
            else
                resourceType = ResourceType::Other;

            entryForFrontend->setMimeType(mimeType);
            entryForFrontend->setResourceType(resourceType);
        }

        m_entries->addItem(entryForFrontend);
    }
    readDirectoryEntries();
    return true;
}

class GetMetadataTask : public RefCounted<GetMetadataTask> {
    WTF_MAKE_NONCOPYABLE(GetMetadataTask);
public:
    static PassRefPtr<GetMetadataTask> create(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& url)
    {
        return adoptRef(new GetMetadataTask(frontendProvider, requestId, url));
    }

    virtual ~GetMetadataTask()
    {
        reportResult(FileError::ABORT_ERR, 0);
    }

    bool didHitError(FileError* error)
    {
        reportResult(error->code(), 0);
        return true;
    }

    void start(ScriptExecutionContext*);
    bool didGetEntry(Entry*);
    bool didGetMetadata(Metadata*);

    void reportResult(FileError::ErrorCode errorCode, PassRefPtr<TypeBuilder::FileSystem::Metadata> metadata)
    {
        if (!m_frontendProvider || !m_frontendProvider->frontend())
            return;
        m_frontendProvider->frontend()->metadataReceived(m_requestId, static_cast<int>(errorCode), metadata);
        m_frontendProvider = 0;
    }

private:
    GetMetadataTask(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& url)
        : m_frontendProvider(frontendProvider)
        , m_requestId(requestId)
        , m_url(ParsedURLString, url) { }

    RefPtr<FrontendProvider> m_frontendProvider;
    int m_requestId;
    KURL m_url;
    String m_path;
    bool m_isDirectory;
};

void GetMetadataTask::start(ScriptExecutionContext* scriptExecutionContext)
{
    FileSystemType type;
    DOMFileSystemBase::crackFileSystemURL(m_url, type, m_path);

    RefPtr<EntryCallback> successCallback = CallbackDispatcherFactory<EntryCallback>::create(this, &GetMetadataTask::didGetEntry);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &GetMetadataTask::didHitError);

    OwnPtr<ResolveURICallbacks> fileSystemCallbacks = ResolveURICallbacks::create(successCallback, errorCallback, scriptExecutionContext, type, m_path);
    LocalFileSystem::localFileSystem().readFileSystem(scriptExecutionContext, type, fileSystemCallbacks.release());
}

bool GetMetadataTask::didGetEntry(Entry* entry)
{
    if (!entry->filesystem()->scriptExecutionContext()) {
        reportResult(FileError::ABORT_ERR, 0);
        return true;
    }

    RefPtr<MetadataCallback> successCallback = CallbackDispatcherFactory<MetadataCallback>::create(this, &GetMetadataTask::didGetMetadata);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &GetMetadataTask::didHitError);
    entry->getMetadata(successCallback, errorCallback);
    m_isDirectory = entry->isDirectory();
    return true;
}

bool GetMetadataTask::didGetMetadata(Metadata* metadata)
{
    using TypeBuilder::FileSystem::Metadata;
    RefPtr<Metadata> result = Metadata::create().setModificationTime(metadata->modificationTime()).setSize(metadata->size());
    reportResult(static_cast<FileError::ErrorCode>(0), result);
    return true;
}

class ReadFileTask : public EventListener {
    WTF_MAKE_NONCOPYABLE(ReadFileTask);
public:
    static PassRefPtr<ReadFileTask> create(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& url, long long start, long long end)
    {
        return adoptRef(new ReadFileTask(frontendProvider, requestId, url, start, end));
    }

    virtual ~ReadFileTask()
    {
        reportResult(FileError::ABORT_ERR, 0);
    }

    void start(ScriptExecutionContext*);


    virtual bool operator==(const EventListener& other) OVERRIDE
    {
        return this == &other;
    }

    virtual void handleEvent(ScriptExecutionContext*, Event* event) OVERRIDE
    {
        if (event->type() == eventNames().loadEvent)
            didRead();
        else if (event->type() == eventNames().errorEvent)
            didHitError(m_reader->error().get());
    }

private:
    bool didHitError(FileError* error)
    {
        reportResult(error->code(), 0);
        return true;
    }

    bool didGetEntry(Entry*);
    bool didGetFile(File*);
    void didRead();

    void reportResult(FileError::ErrorCode errorCode, const String* result)
    {
        if (!m_frontendProvider || !m_frontendProvider->frontend())
            return;
        m_frontendProvider->frontend()->fileContentReceived(m_requestId, static_cast<int>(errorCode), result);
        m_frontendProvider = 0;
    }

    ReadFileTask(PassRefPtr<FrontendProvider> frontendProvider, int requestId, const String& url, long long start, long long end)
        : EventListener(EventListener::CPPEventListenerType)
        , m_frontendProvider(frontendProvider)
        , m_requestId(requestId)
        , m_url(ParsedURLString, url)
        , m_start(start)
        , m_end(end) { }

    RefPtr<FrontendProvider> m_frontendProvider;
    int m_requestId;
    KURL m_url;
    int m_start;
    long long m_end;

    RefPtr<FileReader> m_reader;
};

void ReadFileTask::start(ScriptExecutionContext* scriptExecutionContext)
{
    ASSERT(scriptExecutionContext);

    FileSystemType type;
    String path;
    if (!DOMFileSystemBase::crackFileSystemURL(m_url, type, path)) {
        reportResult(FileError::SYNTAX_ERR, 0);
        return;
    }

    RefPtr<EntryCallback> successCallback = CallbackDispatcherFactory<EntryCallback>::create(this, &ReadFileTask::didGetEntry);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &ReadFileTask::didHitError);
    OwnPtr<ResolveURICallbacks> fileSystemCallbacks = ResolveURICallbacks::create(successCallback, errorCallback, scriptExecutionContext, type, path);

    LocalFileSystem::localFileSystem().readFileSystem(scriptExecutionContext, type, fileSystemCallbacks.release());
}

bool ReadFileTask::didGetEntry(Entry* entry)
{
    if (entry->isDirectory()) {
        reportResult(FileError::TYPE_MISMATCH_ERR, 0);
        return true;
    }

    if (!entry->filesystem()->scriptExecutionContext()) {
        reportResult(FileError::ABORT_ERR, 0);
        return true;
    }

    RefPtr<FileCallback> successCallback = CallbackDispatcherFactory<FileCallback>::create(this, &ReadFileTask::didGetFile);
    RefPtr<ErrorCallback> errorCallback = CallbackDispatcherFactory<ErrorCallback>::create(this, &ReadFileTask::didHitError);
    static_cast<FileEntry*>(entry)->file(successCallback, errorCallback);

    m_reader = FileReader::create(entry->filesystem()->scriptExecutionContext());
    return true;
}

bool ReadFileTask::didGetFile(File* file)
{
    RefPtr<Blob> blob = file->slice(m_start, m_end);
    m_reader->setOnload(this);
    m_reader->setOnerror(this);

    ExceptionCode ec = 0;
    m_reader->readAsArrayBuffer(blob.get(), ec);
    return true;
}

void ReadFileTask::didRead()
{
    RefPtr<ArrayBuffer> result = m_reader->arrayBufferResult();
    String encodedResult = base64Encode(static_cast<char*>(result->data()), result->byteLength());
    reportResult(static_cast<FileError::ErrorCode>(0), &encodedResult);
}

}

// static
PassOwnPtr<InspectorFileSystemAgent> InspectorFileSystemAgent::create(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorState* state)
{
    return adoptPtr(new InspectorFileSystemAgent(instrumentingAgents, pageAgent, state));
}

InspectorFileSystemAgent::~InspectorFileSystemAgent()
{
    if (m_frontendProvider)
        m_frontendProvider->clear();
    m_instrumentingAgents->setInspectorFileSystemAgent(0);
}

void InspectorFileSystemAgent::enable(ErrorString*)
{
    if (m_enabled)
        return;
    m_enabled = true;
    m_state->setBoolean(FileSystemAgentState::fileSystemAgentEnabled, m_enabled);
}

void InspectorFileSystemAgent::disable(ErrorString*)
{
    if (!m_enabled)
        return;
    m_enabled = false;
    m_state->setBoolean(FileSystemAgentState::fileSystemAgentEnabled, m_enabled);
}

void InspectorFileSystemAgent::requestFileSystemRoot(ErrorString* error, const String& origin, const String& type, int* requestId)
{
    if (!m_enabled || !m_frontendProvider) {
        *error = "FileSystem agent is not enabled";
        return;
    }
    ASSERT(m_frontendProvider->frontend());

    *requestId = m_nextRequestId++;
    if (ScriptExecutionContext* scriptExecutionContext = scriptExecutionContextForOrigin(SecurityOrigin::createFromString(origin).get()))
        GetFileSystemRootTask::create(m_frontendProvider, *requestId, type)->start(scriptExecutionContext);
    else
        m_frontendProvider->frontend()->fileSystemRootReceived(*requestId, static_cast<int>(FileError::ABORT_ERR), 0);
}

void InspectorFileSystemAgent::requestDirectoryContent(ErrorString* error, const String& url, int* requestId)
{
    if (!m_enabled || !m_frontendProvider) {
        *error = "FileSystem agent is not enabled";
        return;
    }
    ASSERT(m_frontendProvider->frontend());

    *requestId = m_nextRequestId++;

    if (ScriptExecutionContext* scriptExecutionContext = scriptExecutionContextForOrigin(SecurityOrigin::createFromString(url).get()))
        ReadDirectoryTask::create(m_frontendProvider, *requestId, url)->start(scriptExecutionContext);
    else
        m_frontendProvider->frontend()->directoryContentReceived(*requestId, static_cast<int>(FileError::ABORT_ERR), 0);
}

void InspectorFileSystemAgent::requestMetadata(ErrorString* error, const String& url, int* requestId)
{
    if (!m_enabled || !m_frontendProvider) {
        *error = "FileSystem agent is not enabled";
        return;
    }
    ASSERT(m_frontendProvider->frontend());

    *requestId = m_nextRequestId++;

    if (ScriptExecutionContext* scriptExecutionContext = scriptExecutionContextForOrigin(SecurityOrigin::createFromString(url).get()))
        GetMetadataTask::create(m_frontendProvider, *requestId, url)->start(scriptExecutionContext);
    else
        m_frontendProvider->frontend()->metadataReceived(*requestId, static_cast<int>(FileError::ABORT_ERR), 0);
}

void InspectorFileSystemAgent::requestFileContent(ErrorString* error, const String& url, const int* start, const int* end, int* requestId)
{
    if (!m_enabled || !m_frontendProvider) {
        *error = "FileSystem agent is not enabled";
        return;
    }
    ASSERT(m_frontendProvider->frontend());

    *requestId = m_nextRequestId++;

    if (ScriptExecutionContext* scriptExecutionContext = scriptExecutionContextForOrigin(SecurityOrigin::createFromString(url).get()))
        ReadFileTask::create(m_frontendProvider, *requestId, url, start ? *start : 0, end ? *end : std::numeric_limits<long long>::max())->start(scriptExecutionContext);
    else
        m_frontendProvider->frontend()->fileContentReceived(*requestId, static_cast<int>(FileError::ABORT_ERR), 0);
}

void InspectorFileSystemAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(frontend);
    m_frontendProvider = FrontendProvider::create(this, frontend->filesystem());
}

void InspectorFileSystemAgent::clearFrontend()
{
    if (m_frontendProvider) {
        m_frontendProvider->clear();
        m_frontendProvider = 0;
    }
    m_enabled = false;
    m_state->setBoolean(FileSystemAgentState::fileSystemAgentEnabled, m_enabled);
}

void InspectorFileSystemAgent::restore()
{
    m_enabled = m_state->getBoolean(FileSystemAgentState::fileSystemAgentEnabled);
}

InspectorFileSystemAgent::InspectorFileSystemAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorState* state)
    : InspectorBaseAgent<InspectorFileSystemAgent>("FileSystem", instrumentingAgents, state)
    , m_pageAgent(pageAgent)
    , m_enabled(false)
    , m_nextRequestId(1)
{
    ASSERT(instrumentingAgents);
    ASSERT(state);
    ASSERT(m_pageAgent);
    m_instrumentingAgents->setInspectorFileSystemAgent(this);
}

ScriptExecutionContext* InspectorFileSystemAgent::scriptExecutionContextForOrigin(SecurityOrigin* origin)
{
    for (Frame* frame = m_pageAgent->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document() && frame->document()->securityOrigin()->isSameSchemeHostPort(origin))
            return frame->document();
    }
    return 0;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR) && ENABLE(FILE_SYSTEM)
