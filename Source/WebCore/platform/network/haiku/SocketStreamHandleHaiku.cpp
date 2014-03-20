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

int32 SocketStreamHandle::AsyncHandleRead(int32 length)
{
	if(length<0)
	{
		m_client->didFailSocketStream(this,SocketStreamError(length));
	}
	else
	{
		m_client->didReceiveSocketStreamData(this,readBuffer,length);
	}
	return 0;
}


int32 SocketStreamHandle::AsyncHandleWrite()
{
	sendPendingData();
	return 0;
}


void CallOnMainThreadAndWaitRead(SocketStreamHandle* handle, int32 length)
{
	if(isMainThread())
	{
		handle->AsyncHandleRead(length);
		return;
	}
	sem_id sem;
	sem = create_sem(1,"AsyncHandleRead");
	acquire_sem(sem);
	callOnMainThread([&]{
		handle->AsyncHandleRead(length);		
		release_sem(sem);
		});
	acquire_sem(sem);
	release_sem(sem);		
}


void CallOnMainThreadAndWaitConnect(SocketStreamHandle* handle, int32 error)
{
	if(isMainThread())
	{
		handle->AsyncHandleConnect(error);
		return;
	}
	sem_id sem;
	sem = create_sem(1,"AsyncHandleConnect");
	acquire_sem(sem);
	callOnMainThread([&]{
		handle->AsyncHandleConnect(error);
		release_sem(sem);
		});
	acquire_sem(sem);
	release_sem(sem);		
}


void CallOnMainThreadAndWaitWrite(SocketStreamHandle* handle)
{
	if(isMainThread())
	{
		handle->AsyncHandleWrite();
		return;
	}
	sem_id sem;
	sem = create_sem(1,"AsyncHandleWrite");
	acquire_sem(sem);
	callOnMainThread([&]{
		handle->AsyncHandleWrite();
		release_sem(sem);
		});
	acquire_sem(sem);
	release_sem(sem);		
}


int32 AsyncReadThread(void* data)
{
	thread_id sender;
	int32 code;
	SocketStreamHandle* handle = (SocketStreamHandle*)data;
	while(true)
	{
		if(has_data(find_thread(NULL)))
		{
			code = receive_data(&sender,NULL,0);
			if(code==1)
				exit_thread(0);
		}
    	int32 numberReadBytes = (handle->socket)->Read(handle->readBuffer,kReadBufferSize);   	
    	CallOnMainThreadAndWaitRead(handle,numberReadBytes);    	
	}
	return 0;
}


int32 SocketStreamHandle::AsyncHandleConnect(int32 error)
{
	if(error != B_OK)
	{
		m_client->didFailSocketStream(this,SocketStreamError(error));
	}
	else
	{
		fReadThreadId = spawn_thread(AsyncReadThread,"AsyncReadThread",63,(void*)this);
    	resume_thread(fReadThreadId);
    	m_state = Open;
    	m_client->didOpenSocketStream(this);
	}
	return error;
}


int32 AsyncWriteThread(void* data)
{
	SocketStreamHandle* handle = (SocketStreamHandle*)data;
	status_t response=(handle->socket)->WaitForWritable(B_INFINITE_TIMEOUT);
	CallOnMainThreadAndWaitWrite(handle);
	return response;
}


int32 AsyncConnectThread(void* data)
{
	SocketStreamHandle* handle = (SocketStreamHandle*)data;
	status_t error=(handle->socket)->Connect(*(handle->peer));
	CallOnMainThreadAndWaitConnect(handle, error);
	return 0;
}

SocketStreamHandle::SocketStreamHandle(const URL& url, SocketStreamHandleClient* client)
    : 
    SocketStreamHandleBase(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, m_client);
	fReadThreadId = 0;
	fWriteThreadId = 0;
	fConnectThreadId=0;
	readBuffer = new char[kReadBufferSize];
    unsigned int port = url.hasPort() ? url.port() : (url.protocolIs("wss") ? 443 : 80);    
    peer = new BNetworkAddress(url.host().utf8().data(),port);
    socket = new BSocket();
    fConnectThreadId = spawn_thread(AsyncConnectThread,"AsyncConnectThread",63,(void*)this);
    resume_thread(fConnectThreadId);
}


SocketStreamHandle::~SocketStreamHandle()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    delete readBuffer;
    delete peer;
    delete socket;
    setClient(0);
}


int SocketStreamHandle::platformSend(const char* buffer, int length)
{
	int32 writtenLength = 0;
	bool flagForPending = false;
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    status_t response=socket->WaitForWritable(0);
    if(response == B_OK)
    {
    	writtenLength=socket->Write(buffer,length);
    	if(writtenLength < length)
    		flagForPending = true;
    }
    else if(response == B_TIMED_OUT
    			|| response == B_WOULD_BLOCK)
    {
    	flagForPending = true;
    }
    else
    {
    	m_client->didFailSocketStream(this,SocketStreamError(response));
    }
    if(flagForPending)
    {
    	fWriteThreadId = spawn_thread(AsyncWriteThread,"AsyncWriteThread",63,(void*)this);
    	resume_thread(fWriteThreadId);
    }
    return writtenLength;
}


void SocketStreamHandle::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    send_data(fReadThreadId,1,NULL,0);
    socket->Disconnect();
	m_client->didCloseSocketStream(this);
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


}  // namespace WebCore
