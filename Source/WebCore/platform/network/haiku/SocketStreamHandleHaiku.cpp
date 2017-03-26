/*
The MIT License (MIT)

Copyright (c) 2014 Akshay Jaggi.
Copyright (c) 2014 Haiku, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "config.h"
#include "SocketStreamHandle.h"
#include "SocketStreamError.h"

#include "URL.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamHandleClient.h"

#include <wtf/text/CString.h>
#include <wtf/MainThread.h>
#include <kernel/OS.h>

namespace WebCore {

static const int kReadBufferSize = 1024;

SocketStreamHandle::SocketStreamHandle(const URL& url, SocketStreamHandleClient* client)
    : 
    SocketStreamHandleBase(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
	fReadThreadId = 0;
	fWriteThreadId = 0;
    unsigned int port = url.hasPort() ? url.port() : (url.protocolIs("wss") ? 443 : 80);    
    BNetworkAddress peer(url.host().utf8().data(),port);
    socket = new BSocket();
    status_t error = socket->Connect(peer);
    
    liveObjects.insert(this);

	if(error != B_OK)
		m_client->didFailSocketStream(this,SocketStreamError(error));
	else {
		fReadThreadId = spawn_thread(AsyncReadThread, "AsyncReadThread",
            B_NORMAL_PRIORITY, (void*)this);
    	resume_thread(fReadThreadId);
    	m_state = Open;
    	m_client->didOpenSocketStream(this);
	}
}


SocketStreamHandle::~SocketStreamHandle()
{
    liveObjects.erase(this);
    LOG(Network, "SocketStreamHandle %p delete", this);

    setClient(0);
    socket->Disconnect();

    delete socket;
}


int SocketStreamHandle::platformSend(const char* buffer, int length)
{
	int32 writtenLength = 0;
	bool flagForPending = false;
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    status_t response=socket->WaitForWritable(0);
    if(response == B_OK) {
    	writtenLength=socket->Write(buffer,length);
    	if(writtenLength < length)
    		flagForPending = true;
    } else if(response == B_TIMED_OUT || response == B_WOULD_BLOCK)
    	flagForPending = true;
    else
    	m_client->didFailSocketStream(this,SocketStreamError(response));
    
    if(flagForPending) {
    	fWriteThreadId = spawn_thread(AsyncWriteThread, "AsyncWriteThread",
            B_NORMAL_PRIORITY, (void*)this);
    	resume_thread(fWriteThreadId);
    }
    return writtenLength;
}


void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    if (socket)
        socket->Disconnect();
    if (m_client)
        m_client->didCloseSocketStream(this);
    setClient(0);
}


void SocketStreamHandle::didReceiveAuthenticationChallenge(const AuthenticationChallenge&)
{
    notImplemented();
}


void SocketStreamHandle::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
    notImplemented();
}


void SocketStreamHandle::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
    notImplemented();
}


void SocketStreamHandle::receivedCancellation(const AuthenticationChallenge&)
{
    notImplemented();
}


void SocketStreamHandle::receivedRequestToPerformDefaultHandling(const AuthenticationChallenge&)
{
    notImplemented();
}


void SocketStreamHandle::receivedChallengeRejection(const AuthenticationChallenge&)
{
    notImplemented();
}


// #pragma mark - Private static methods


std::set<void*> SocketStreamHandle::liveObjects;


int32 SocketStreamHandle::AsyncWriteThread(void* object)
{
	SocketStreamHandle* handle = (SocketStreamHandle*)object;
	status_t response = handle->socket->WaitForWritable(B_INFINITE_TIMEOUT);
	if(isMainThread())
	{
	    handle->sendPendingData();
		return response;
	}
	sem_id sem;
	sem = create_sem(0,"AsyncHandleWrite");
	callOnMainThread([&]{
		handle->sendPendingData();
		release_sem(sem);
		});
	acquire_sem(sem);
	delete_sem(sem);

    return response;    
}


struct Packet {
    SocketStreamHandle* handle;
    char*				readBuffer;
    ssize_t             length;
    sem_id              sem;
};


void SocketStreamHandle::AsyncHandleRead(void* object)
{
    Packet* packet = (Packet*)object;
    if (liveObjects.find(packet->handle) == liveObjects.end()) {
        release_sem(packet->sem);
        return;
    }

    SocketStreamHandle* handle = packet->handle;

    if(!handle->m_client) {
        release_sem(packet->sem);
        return;
    }

	if (packet->length < 0) {
		handle->m_client->didFailSocketStream(handle,
            SocketStreamError(packet->length));
    } else {
		handle->m_client->didReceiveSocketStreamData(handle,
            packet->readBuffer, packet->length);
    }

    release_sem(packet->sem);
}


int32 SocketStreamHandle::AsyncReadThread(void* data)
{
    Packet p;
    p.handle = (SocketStreamHandle*)data;
    p.readBuffer = new char[kReadBufferSize];

    p.sem = create_sem(0, "AsyncRead");
    while(liveObjects.find(p.handle) != liveObjects.end()
        && p.handle->socket->IsConnected())
    {
        p.length = p.handle->socket->Read(p.readBuffer,kReadBufferSize);

        if(isMainThread())
        {
            AsyncHandleRead(&p);
        } else {
            callOnMainThread([&]{AsyncHandleRead(&p);});
            acquire_sem(p.sem);
        }
    }
    delete_sem(p.sem);
    delete p.readBuffer;
    return 0;
}


}  // namespace WebCore
