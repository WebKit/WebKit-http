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

#include "BlobRegistryImpl.h"
#include "FormData.h"
#include "HTTPParsers.h"
#include "MIMETypeRegistry.h"
#include "ProtectionSpace.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "ResourceRequest.h"
#include <wtf/text/CString.h>

#include <Debug.h>
#include <File.h>
#include <Url.h>
#include <UrlRequest.h>
#include <HttpRequest.h>

#include <assert.h>

static const int gMaxRecursionLimit = 10;

namespace WebCore {
	
BFormDataIO::BFormDataIO(FormData& form)
	: m_formElements(form.elements())
	, m_currentFile(NULL)
	, m_currentOffset(0)
{
    _ParseCurrentElement();
}

BFormDataIO::~BFormDataIO()
{
	delete m_currentFile;
}

ssize_t BFormDataIO::Size()
{
    ssize_t size = 0;
    for(int i = m_formElements.size() - 1; i >= 0; i--)
    {
        FormDataElement& element = m_formElements[i];
        switch(element.m_type)
        {
            case FormDataElement::Type::Data:
                size += element.m_data.size();
                break;
            case FormDataElement::Type::EncodedFile:
            {
                BNode node(BString(element.m_filename).String());
                off_t filesize = 0;
                node.GetSize(&filesize);
                size += filesize;
                break;
            }
            case FormDataElement::Type::EncodedBlob:
                size += element.m_fileLength;
                break;
        }
    }

    return size;
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

        switch (element.m_type) {
            case FormDataElement::Type::EncodedBlob:
            {
                BlobRegistryImpl& registry = static_cast<BlobRegistryImpl&>(blobRegistry());
                RefPtr<BlobData> blobData = registry.getBlobDataFromURL(URL(ParsedURLString, element.m_url));
                const BlobDataItem& item = blobData->items()[0];
                if (item.type == BlobDataItem::Data) {
                    size_t toCopy = 0;

                    if (remaining < item.length() - m_currentOffset)
                        toCopy = remaining;
                    else
                        toCopy = item.length() - m_currentOffset;

                    memcpy(reinterpret_cast<char*>(buffer) + read,
                        item.data->data() + item.offset() + m_currentOffset, toCopy);
                    m_currentOffset += toCopy;
                    read += toCopy;

                    if (m_currentOffset >= item.length())
                        _NextElement();

                }
                // fallthrough for file blobs
            }
            case FormDataElement::Type::EncodedFile:
                {
                    ssize_t result = m_currentFile->Read(
                        reinterpret_cast<char*>(buffer) + read, remaining);

                    if (result < 0)
                        return read;

                    read += result;

                    if (m_currentFile->Position() >= m_currentFileSize 
                            || !m_currentFile->IsReadable())
                        _NextElement();
                }
                break;

            case FormDataElement::Type::Data:
                {
                    size_t toCopy = 0;

                    if (remaining < element.m_data.size() - m_currentOffset)
                        toCopy = remaining;
                    else
                        toCopy = element.m_data.size() - m_currentOffset;

                    memcpy(reinterpret_cast<char*>(buffer) + read,
                        element.m_data.data() + m_currentOffset, toCopy); 
                    m_currentOffset += toCopy;
                    read += toCopy;

                    if (m_currentOffset >= element.m_data.size())
                        _NextElement();
                }
                break;
        }
    }

    return read;
}

ssize_t 
BFormDataIO::Write(const void* /*buffer*/, size_t /*size*/)
{
    // Write isn't implemented since we don't use it
    return B_NOT_SUPPORTED;
}

void
BFormDataIO::_NextElement()
{
    m_currentOffset = 0;
    m_currentFileSize = 0;
    m_formElements.remove(0);

    _ParseCurrentElement();
}


void
BFormDataIO::_ParseCurrentElement()
{
    if (m_formElements.isEmpty())
        return;

    if (m_currentFile == NULL)
        m_currentFile = new BFile();

    FormDataElement& currentElement = m_formElements[0];

    switch (currentElement.m_type) {
        case FormDataElement::Type::EncodedFile:
            m_currentFile->SetTo(BString(currentElement.m_filename).String(), B_READ_ONLY);
            m_currentFile->GetSize(&m_currentFileSize);
            return;
        case FormDataElement::Type::EncodedBlob:
        {
            BlobRegistryImpl& registry = static_cast<BlobRegistryImpl&>(blobRegistry());
            RefPtr<BlobData> blobData = registry.getBlobDataFromURL(URL(ParsedURLString, currentElement.m_url));
            if (!blobData)
                debugger("Empty blob! This shouldn't happen!");

            if (blobData->items().size() != 1)
                debugger("Composite blobs are currently unsupported. Please submit a bugreport with the webpage that triggered this.");

            // FIXME handle blobs with multiple items
            const BlobDataItem& item = blobData->items()[0];

            // FIXME handle BlobDataItem::Data too!
            if (item.type == BlobDataItem::File)
            {
                m_currentFile->SetTo(item.file->path().utf8().data(), B_READ_ONLY);
                m_currentFile->Seek(item.offset(), SEEK_SET);
                if (item.length() == BlobDataItem::toEndOfFile)
                    m_currentFile->GetSize(&m_currentFileSize);
                else
                    m_currentFileSize = item.length();
                return;
            } else if(item.type == BlobDataItem::Data) {

            } else {
                debugger("Unhandled blob type. Please submit a bugreport with the webpage that triggered this.");
            }
        }
        default:
            // Nothing to do for other types.
            return;
    }
}


BUrlProtocolHandler::BUrlProtocolHandler(NetworkingContext* context,
        ResourceHandle* handle, bool synchronous)
    : BUrlProtocolAsynchronousListener(!synchronous)
    , m_resourceHandle(handle)
    , m_redirected(false)
    , m_responseDataSent(false)
    , m_postData(NULL)
    , m_request(handle->firstRequest().toNetworkRequest(
        context ? context->context() : NULL))
    , m_redirectionTries(gMaxRecursionLimit)
{
    m_method = BString(m_resourceHandle->firstRequest().httpMethod());

    if (!m_resourceHandle)
        return;

    m_postData = NULL;

    if (m_request == NULL)
        return;

    BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(m_request);
    if(httpRequest) {
        // TODO maybe we have data to send in other cases ?
        if(m_method == B_HTTP_POST || m_method == B_HTTP_PUT) {
            FormData* form = m_resourceHandle->firstRequest().httpBody();
            if(form) {
                m_postData = new BFormDataIO(*form);
                httpRequest->AdoptInputData(m_postData, m_postData->Size());
            }
        }

        httpRequest->SetMethod(m_method.String());
    }

    // In synchronous mode, call this listener directly.
    // In asynchronous mode, go through a BMessage
    if(this->SynchronousListener()) {
        m_request->SetListener(this->SynchronousListener());
    } else {
        m_request->SetListener(this);
    }

    if (m_request->Run() < B_OK) {
        ResourceHandleClient* client = m_resourceHandle->client();
        if (!client)
            return;

        ResourceError error("BUrlProtocol", 42,
            handle->firstRequest().url().string().utf8().data(),
            "The service kit failed to start the request.");
        client->didFail(m_resourceHandle, error);
    }
}

BUrlProtocolHandler::~BUrlProtocolHandler()
{
    abort();
    if (m_request)
        m_request->SetListener(NULL);
    delete m_request;
}

void BUrlProtocolHandler::abort()
{
    if (m_resourceHandle != NULL && m_request != NULL)
        m_request->Stop();

    m_resourceHandle = NULL;
}

static bool ignoreHttpError(BHttpRequest* reply, bool receivedData)
{
    int httpStatusCode = static_cast<const BHttpResult&>(reply->Result()).StatusCode();

    if (httpStatusCode == 401 || httpStatusCode == 407)
        return false;

    if (receivedData && (httpStatusCode >= 400 && httpStatusCode < 600))
        return true;

    return false;
}

void BUrlProtocolHandler::RequestCompleted(BUrlRequest* caller, bool success)
{
    if (!m_resourceHandle)
        return;

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(m_request);

    if (success || (httpRequest && ignoreHttpError(httpRequest, m_responseDataSent))) {
        client->didFinishLoading(m_resourceHandle, 0);
            // TODO put the actual finish time instead of 0
            // (this isn't done on other platforms either...)
    } else if(httpRequest) {
        const BHttpResult& result = static_cast<const BHttpResult&>(httpRequest->Result());
        int httpStatusCode = result.StatusCode();

        if (httpStatusCode != 0) {
            ResourceError error("HTTP", httpStatusCode,
                caller->Url().UrlString().String(), strerror(caller->Status()));

            client->didFail(m_resourceHandle, error);
        }
    } else {
        ResourceError error("BUrlRequest", caller->Status(), caller->Url().UrlString().String(), strerror(caller->Status()));
        client->didFail(m_resourceHandle, error);
    }
}


bool BUrlProtocolHandler::CertificateVerificationFailed(BUrlRequest*,
    BCertificate& certificate, const char* message)
{
    return m_resourceHandle->didReceiveInvalidCertificate(certificate, message);
}


void BUrlProtocolHandler::AuthenticationNeeded(BHttpRequest* request, ResourceResponse& response)
{
    if (!m_resourceHandle)
        return;

    ResourceHandleInternal* d = m_resourceHandle->getInternal();
    unsigned failureCount = 0;

    const URL& url = m_resourceHandle->firstRequest().url();
    ProtectionSpaceServerType serverType = ProtectionSpaceServerHTTP;
    if (url.protocolIs("https"))
        serverType = ProtectionSpaceServerHTTPS;

    String challenge = static_cast<const BHttpResult&>(request->Result()).Headers()["WWW-Authenticate"];
    ProtectionSpaceAuthenticationScheme scheme = ProtectionSpaceAuthenticationSchemeDefault;

    ResourceHandleClient* client = m_resourceHandle->client();

    if (challenge.startsWith("Digest", false))
        scheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
    else if (challenge.startsWith("Basic", false))
        scheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
    else if (challenge.startsWith("Bearer", false)) {
        // OAuth bearer token - will most likely be handled in Javascript?
        client->didFinishLoading(m_resourceHandle, 0);
        return;
    } else if (challenge.isEmpty()) {
        // This shouldn't happen according to the spec, but Google Drive does
        // it...
        client->didFinishLoading(m_resourceHandle, 0);
        return;
    } else {
        debugger("Unknown http authentication token received. Please submit a bugreport with the page that triggered this");
    }

    String realm;
    int realmStart = challenge.find("realm=\"", 0, false);
    if (realmStart > 0) {
        realmStart += 7;
        int realmEnd = challenge.find("\"", realmStart);
        if (realmEnd >= 0)
            realm = challenge.substring(realmStart, realmEnd - realmStart);
    }

    ProtectionSpace protectionSpace(url.host(), url.port(), serverType, realm, scheme);
    ResourceError resourceError(url.host(), 401, url.string(), String());

    m_redirectionTries--;
    if(m_redirectionTries == 0)
    {
        client->didFinishLoading(m_resourceHandle, 0);
        return;
    }

    ResourceRequest& currentRequest = m_resourceHandle->firstRequest();

    Credential proposedCredential(d->m_user, d->m_pass, CredentialPersistenceForSession);

    AuthenticationChallenge authenticationChallenge(protectionSpace,
        proposedCredential, failureCount++, response, resourceError);
    authenticationChallenge.m_authenticationClient = m_resourceHandle;
    m_resourceHandle->didReceiveAuthenticationChallenge(authenticationChallenge);
            // will set m_user and m_pass in ResourceHandleInternal

    if (d->m_user != "") {
        // Handle this just like redirects.
        m_redirected = true;

        currentRequest.setCredentials(d->m_user.utf8().data(), d->m_pass.utf8().data());
        client->willSendRequest(m_resourceHandle, currentRequest, response);
    } else {
        client->didFinishLoading(m_resourceHandle, 0);
    }
}


void BUrlProtocolHandler::ConnectionOpened(BUrlRequest*)
{
    BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(m_request);
    if (httpRequest != NULL)
        m_redirected = true;
    else
        m_redirected = false;
    m_responseDataSent = false;
}


void BUrlProtocolHandler::HeadersReceived(BUrlRequest* /*caller*/)
{
    BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(m_request);

    WTF::String contentType = m_request->Result().ContentType();
    int contentLength = m_request->Result().Length();
    URL url(m_request->Url());

    WTF::String encoding = extractCharsetFromMediaType(contentType);
    WTF::String mimeType = extractMIMETypeFromMediaType(contentType);

    URL location;

    if (httpRequest) {
        const BHttpResult& result = static_cast<const BHttpResult&>(
            httpRequest->Result());
        BString locationString(result.Headers()["Location"]);
        if (locationString.Length())
            m_redirected = true;
        else
            m_redirected = false;
        location = URL(url, locationString);
    }

    ResourceResponse response(location, mimeType, contentLength, encoding);

    if (httpRequest) {
        const BHttpResult& result = static_cast<const BHttpResult&>(
            httpRequest->Result());
        int statusCode = result.StatusCode();

        String suggestedFilename = filenameFromHTTPContentDisposition(
            result.Headers()["Content-Disposition"]);

        if (!suggestedFilename.isEmpty())
            response.setSuggestedFilename(suggestedFilename);

        response.setHTTPStatusCode(statusCode);
        response.setHTTPStatusText(result.StatusText());

        // Add remaining headers.
        const BHttpHeaders& resultHeaders = result.Headers();
        for (int i = 0; i < resultHeaders.CountHeaders(); i++) {
            BHttpHeader& headerPair = resultHeaders.HeaderAt(i);
            response.setHTTPHeaderField(headerPair.Name(), headerPair.Value());
        }

        if (statusCode == 401)
            AuthenticationNeeded(httpRequest, response);
    }

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    if (m_redirected) {
        m_redirectionTries--;

        if (m_redirectionTries == 0) {
            ResourceError error(location.host(), 400, location.string(),
                "Redirection limit reached");
            client->didFail(m_resourceHandle, error);
            return;
        }

        // Notify the client that we are redirecting.
        ResourceRequest& request = m_resourceHandle->firstRequest();
        request.setURL(location);

        client->willSendRequest(m_resourceHandle, request, response);
    } else {
        client->didReceiveResponse(m_resourceHandle, response);
    }
}

void BUrlProtocolHandler::DataReceived(BUrlRequest* /*caller*/, const char* data, off_t /*position*/, ssize_t size)
{
    if (!m_resourceHandle)
        return;

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    // don't emit the "Document has moved here" type of HTML
    if (m_redirected)
        return;

    if (size > 0) {
        m_responseDataSent = true;
        client->didReceiveData(m_resourceHandle, data, size, size);
    }
}

void BUrlProtocolHandler::UploadProgress(BUrlRequest* /*caller*/, ssize_t bytesSent, ssize_t bytesTotal)
{
    if (!m_resourceHandle)
        return;

    ResourceHandleClient* client = m_resourceHandle->client();
    if (!client)
        return;

    client->didSendData(m_resourceHandle, bytesSent, bytesTotal);
}


}
