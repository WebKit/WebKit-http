/*
    Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
    Copyright (C) 2007 Staikos Computing Services Inc.  <info@staikos.net>
    Copyright (C) 2008 Holger Hans Peter Freyther

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
#include "config.h"
#include "BUrlProtocolHandler.h"

#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "ResourceRequest.h"
#include <wtf/text/CString.h>

#include <Debug.h>
#include <File.h>
#include <services/Url.h>
#include <services/UrlRequest.h>
#include <services/UrlResult.h>
#include <services/UrlHttpProtocol.h>

static const int gMaxRecursionLimit = 10;

namespace WebCore {
	
BFormDataIO::BFormDataIO(FormData* form)
	: m_formElements(form->elements())
	, m_currentFile(NULL)
	, m_currentOffset(0)
{
	printf("BFormDataIO::__construct() : Form size : %d\n", m_formElements.size());
}

BFormDataIO::~BFormDataIO()
{
	delete m_currentFile;
}
	
ssize_t
BFormDataIO::Read(void* buffer, size_t size)
{
    if (m_formElements.isEmpty())
    	// -1 indicates the end of data
        return -1;

    ssize_t read = 0;
    while (read < (ssize_t)size && !m_formElements.isEmpty()) {
        const FormDataElement& element = m_formElements[0];
        const ssize_t remaining = size - read;
        
        printf("BFormDataIO::Read() : Element type %d\n", element.m_type);

		switch (element.m_type) {
			case FormDataElement::encodedFile:
				{
					read += m_currentFile->Read(reinterpret_cast<char*>(buffer) + read, remaining);

					if (m_currentFile->Position() >= m_currentFileSize 
						|| !m_currentFile->IsReadable())
						_NextElement();
				}
				break;
				
			case FormDataElement::data:
				{
					size_t toCopy = 0;
					
					if (remaining < element.m_data.size() - m_currentOffset)
						toCopy = remaining;
					else
						toCopy = element.m_data.size() - m_currentOffset;
						
					memcpy(reinterpret_cast<char*>(buffer) + read, element.m_data.data() + m_currentOffset, toCopy); 
					m_currentOffset += toCopy;
            		read += toCopy;

           	 		if (m_currentOffset >= element.m_data.size())
						_NextElement();
				}
				break;
				
			default:
				TRESPASS();
				break;
		}
    }

    return read;
}

ssize_t 
BFormDataIO::Write(const void* buffer, size_t size)
{
	// Write isn't implemented since we don't use it
	return -1;
}

void
BFormDataIO::_NextElement()
{        
    m_currentOffset = 0;
    m_currentFileSize = 0;
    m_formElements.remove(0);

    if (m_formElements.isEmpty() || m_formElements[0].m_type == FormDataElement::data)
        return;

    if (m_currentFile == NULL)
        m_currentFile = new BFile;

    m_currentFile->SetTo(BString(m_formElements[0].m_filename).String(), B_READ_ONLY);
    m_currentFile->GetSize(&m_currentFileSize);
}

BUrlProtocolHandler::BUrlProtocolHandler(ResourceHandle* handle)
    : BUrlProtocolAsynchronousListener(true)
    , m_resourceHandle(handle)
    , m_redirected(false)
    , m_responseSent(false)
    , m_responseDataSent(false)
    , m_request(handle->request().toNetworkRequest())
    , m_listener(this)
    , m_shouldStart(true)
    , m_shouldFinish(false)
    , m_shouldSendResponse(false)
    , m_shouldForwardData(false)
    , m_redirectionTries(gMaxRecursionLimit)
{
    const ResourceRequest &r = m_resourceHandle->request();

    if (r.httpMethod() == "GET")
        m_method = B_HTTP_GET;
    else if (r.httpMethod() == "HEAD")
        m_method = B_HTTP_HEAD;
    else if (r.httpMethod() == "POST")
        m_method = B_HTTP_POST;
    else if (r.httpMethod() == "PUT")
        m_method = B_HTTP_PUT;
    else if (r.httpMethod() == "DELETE")
        m_method = B_HTTP_DELETE;
    else if (r.httpMethod() == "OPTIONS")
        m_method = B_HTTP_OPTIONS;
    else
        m_method = B_ERROR;

	start();
}

BUrlProtocolHandler::~BUrlProtocolHandler()
{
}

void BUrlProtocolHandler::abort()
{
	if (m_resourceHandle == NULL)
		return;
		
	printf("UPH[%p]::abort()\n", this);
    m_request.Abort();
    m_resourceHandle = NULL;
}

static bool ignoreHttpError(BUrlRequest* reply, bool receivedData)
{
    int httpStatusCode = reply->Result().StatusCode();

    if (httpStatusCode == 401 || httpStatusCode == 407)
        return true;

    if (receivedData && (httpStatusCode >= 400 && httpStatusCode < 600))
        return true;

    return false;
}

void BUrlProtocolHandler::RequestCompleted(BUrlProtocol* caller, bool success)
{
    printf("UPH[%p]::RequestCompleted()\n", this);
    sendResponseIfNeeded();

    if (!m_resourceHandle)
        return;
        
    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    if (m_redirected) {
        m_request = m_nextRequest.toNetworkRequest();
        resetState();
        start();
    } else if (success || ignoreHttpError(&m_request, m_responseDataSent)) {
        client->didFinishLoading(m_resourceHandle);
    } else {
    	const BUrlResult& result = m_request.Result();
    	int httpStatusCode = result.StatusCode();

        if (httpStatusCode) {
            ResourceError error("HTTP", httpStatusCode, caller->Url().UrlString().String(), caller->StatusString(caller->Status()));
            client->didFail(m_resourceHandle, error);
        } else {
            ResourceError error("BUrlRequest", caller->Status(), caller->Url().UrlString().String(), caller->StatusString(caller->Status()));
            client->didFail(m_resourceHandle, error);
        }
    }
}

void BUrlProtocolHandler::sendResponseIfNeeded()
{
    if (m_request.Status() != B_PROT_SUCCESS && m_request.Status() != B_PROT_RUNNING
    	&& !ignoreHttpError(&m_request, m_responseDataSent))
        return;

    if (m_responseSent || !m_resourceHandle)
        return;
    m_responseSent = true;

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    WebCore::String contentType = m_request.Result().Headers()["Content-Type"];
    WebCore::String encoding = extractCharsetFromMediaType(contentType);
    WebCore::String mimeType = extractMIMETypeFromMediaType(contentType);

    if (mimeType.isEmpty()) {
        // let's try to guess from the extension
        BString extension = m_request.Url().Path();
        int index = extension.FindLast('.');
        
        if (index > 0) {
        	extension.Remove(0, index);
            mimeType = MIMETypeRegistry::getMIMETypeForExtension(extension);
        }
    }

    KURL url(m_request.Url());
    
    int contentLength = 0;
    const char* contentLengthString
    	= m_request.Result().Headers()["Content-Length"];
    if (contentLengthString != NULL)
    	contentLength = atoi(contentLengthString);
    	
    ResourceResponse response(url, mimeType,
    							contentLength,
                            	encoding, String());

    if (url.isLocalFile()) {
        client->didReceiveResponse(m_resourceHandle, response);
        return;
    }


    int statusCode = m_request.Result().StatusCode();
    if (url.protocolInHTTPFamily()) {
        String suggestedFilename = filenameFromHTTPContentDisposition(m_request.Result().Headers()["Content-Disposition"]);

        if (!suggestedFilename.isEmpty())
            response.setSuggestedFilename(suggestedFilename);
        else
            response.setSuggestedFilename(url.lastPathComponent());

        response.setHTTPStatusCode(statusCode);
        response.setHTTPStatusText(m_request.Result().StatusText());

        // Add remaining headers.
        const BHttpHeaders& resultHeaders = m_request.Result().Headers();
        for (int i = 0; i < resultHeaders.CountHeaders(); i++) {
        	BHttpHeader& headerPair = resultHeaders.HeaderAt(i);
        	response.setHTTPHeaderField(headerPair.Name(), headerPair.Value());
        }
    }
    
    
    BString locationString(m_request.Result().Headers()["Location"]);
    if (locationString.Length()) {
    	BUrl location;
    	
    	if (locationString[0] == '/') {
    		location = BUrl(m_request.Url());
    		location.SetPath(locationString);
    	}
    	else
    		location = BUrl(locationString);
    		
    	m_redirectionTries--;
    	
    	if (m_redirectionTries == 0) {
    		ResourceError error(location.Host().String(), 400, location.UrlString().String(),
    			"Redirection limit reached");
    		client->didFail(m_resourceHandle, error);
    		return;
    	}
    	
    	m_redirected = true;
    	
    	m_nextRequest = m_resourceHandle->request();
    	m_nextRequest.setURL(location);
    	
        if (((statusCode >= 301 && statusCode <= 303) || statusCode == 307) && m_method == B_HTTP_POST) {
            m_method = B_HTTP_GET;
            m_nextRequest.setHTTPMethod("GET");
        }
        
        client->willSendRequest(m_resourceHandle, m_nextRequest, response);
    }
    
    client->didReceiveResponse(m_resourceHandle, response);
}

void BUrlProtocolHandler::HeadersReceived(BUrlProtocol* caller)
{
	sendResponseIfNeeded();
}

void BUrlProtocolHandler::DataReceived(BUrlProtocol* caller, const char* data, ssize_t size)
{
    sendResponseIfNeeded();

    // don't emit the "Document has moved here" type of HTML
    if (m_redirected)
        return;

    if (!m_resourceHandle)
        return;

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    if (size > 0) {
        m_responseDataSent = true;
        client->didReceiveData(m_resourceHandle, data, size, size);
    }
}

void BUrlProtocolHandler::UploadProgress(BUrlProtocol* caller, ssize_t bytesSent, ssize_t bytesTotal)
{
    if (!m_resourceHandle)
        return;

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    client->didSendData(m_resourceHandle, bytesSent, bytesTotal);
}

void BUrlProtocolHandler::start()
{
	if (!m_resourceHandle)
		return;
		
    m_shouldStart = false;

    switch (m_method) {
        case B_HTTP_GET:
            break;
        case B_HTTP_POST:
    		delete m_postData;
    		m_postData = new BFormDataIO(m_resourceHandle->getInternal()->m_request.httpBody());
    		m_request.SetProtocolOption(B_HTTPOPT_INPUTDATA, m_postData);
            break;
    }
    
    bool followLocation = false;
    m_request.SetProtocolOption(B_HTTPOPT_FOLLOWLOCATION, &followLocation);
   	m_request.SetProtocolOption(B_HTTPOPT_METHOD, &m_method);
    m_request.SetProtocolListener(this->SynchronousListener());
    
    printf("UPH[%p]::start(%s)\n", this, m_request.Url().UrlString().String());
    if (!m_request.InitCheck() || m_request.Perform() != B_OK) {
    	ResourceHandleClient* client = m_resourceHandle->client();
    	if (!client)
        	return;
        	
    	ResourceError error("BUrlProtocol", 42, m_request.Url().UrlString().String(), 
    		"The request protocol is not handled by Services Kit.");
        client->didFail(m_resourceHandle, error);
    }
}

void BUrlProtocolHandler::resetState()
{
    m_redirected = false;
    m_responseSent = false;
    m_responseDataSent = false;
    m_shouldStart = true;
    m_shouldFinish = false;
    m_shouldSendResponse = false;
    m_shouldForwardData = false;
}

}
