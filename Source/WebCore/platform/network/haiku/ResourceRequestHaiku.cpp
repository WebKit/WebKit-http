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

// FIXME must handle other types of requests (at least Ftp and File), and
// callers should be prepared to handle that
BHttpRequest* ResourceRequest::toNetworkRequest() const
{
    BHttpRequest* request = dynamic_cast<BHttpRequest*>(
        BUrlProtocolRoster::MakeRequest(url()));

    if (request != NULL) {
        const HTTPHeaderMap &headers = httpHeaderFields();
        BHttpHeaders* requestHeaders = new BHttpHeaders();
            // FIXME this is leaked - maybe the request should own or copy it

        for (HTTPHeaderMap::const_iterator it = headers.begin(),
                end = headers.end(); it != end; ++it)
        {
            requestHeaders->AddHeader(it->first.string().utf8().data(),
                it->second.utf8().data());
        }

        request->SetHeaders(requestHeaders);
        request->SetContext(&networkContext());
    }

    return request;
}

}

