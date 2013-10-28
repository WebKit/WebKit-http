/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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
#include "ResourceRequest.h"

#include "CookieJar.h"

#include <UrlProtocolRoster.h>
#include <UrlRequest.h>
#include <HttpRequest.h>
#include <wtf/text/CString.h>

namespace WebCore {

BUrlRequest* ResourceRequest::toNetworkRequest() const
{
    BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url());

    if(!request)
        return NULL;

    request->SetContext(&networkContext());

    BHttpRequest* httpRequest = dynamic_cast<BHttpRequest*>(request);
    if (httpRequest != NULL) {
        const HTTPHeaderMap &headers = httpHeaderFields();
        BHttpHeaders* requestHeaders = new BHttpHeaders();

        for (HTTPHeaderMap::const_iterator it = headers.begin(),
                end = headers.end(); it != end; ++it)
        {
            requestHeaders->AddHeader(it->first.string().utf8().data(),
                it->second.utf8().data());
        }

        if(!fUsername.IsEmpty()) {
            httpRequest->SetUserName(fUsername);
            httpRequest->SetPassword(fPassword);
        }

        httpRequest->AdoptHeaders(requestHeaders);
    }

    return request;
}


void ResourceRequest::setCredentials(const char* username, const char* password)
{
    fUsername = username;
    fPassword = password;
}

}

