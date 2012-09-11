/*
 * Copyright (C) 2009, 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "NetworkManager.h"

#include "Chrome.h"
#include "CredentialStorage.h"
#include "Frame.h"
#include "FrameLoaderClientBlackBerry.h"
#include "NetworkJob.h"
#include "Page.h"
#include "ReadOnlyLatin1String.h"
#include "ResourceHandleInternal.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"

#include <BlackBerryPlatformLog.h>
#include <BuildInformation.h>
#include <network/FilterStream.h>
#include <network/NetworkRequest.h>

namespace WebCore {

NetworkManager* NetworkManager::instance()
{
    static NetworkManager* sInstance;
    if (!sInstance) {
        sInstance = new NetworkManager;
        ASSERT(sInstance);
    }
    return sInstance;
}

bool NetworkManager::startJob(int playerId, PassRefPtr<ResourceHandle> job, const Frame& frame, bool defersLoading)
{
    ASSERT(job.get());
    // We shouldn't call methods on PassRefPtr so make a new RefPt.
    RefPtr<ResourceHandle> refJob(job);
    return startJob(playerId, refJob, refJob->firstRequest(), frame, defersLoading);
}

bool NetworkManager::startJob(int playerId, PassRefPtr<ResourceHandle> job, const ResourceRequest& request, const Frame& frame, bool defersLoading)
{
    Page* page = frame.page();
    if (!page)
        return false;
    BlackBerry::Platform::NetworkStreamFactory* streamFactory = page->chrome()->platformPageClient()->networkStreamFactory();
    return startJob(playerId, page->groupName(), job, request, streamFactory, frame, defersLoading ? 1 : 0);
}

bool NetworkManager::startJob(int playerId, const String& pageGroupName, PassRefPtr<ResourceHandle> job, const ResourceRequest& request, BlackBerry::Platform::NetworkStreamFactory* streamFactory, const Frame& frame, int deferLoadingCount, int redirectCount)
{
    // Make sure the ResourceHandle doesn't go out of scope while calling callbacks.
    RefPtr<ResourceHandle> guardJob(job);

    KURL url = request.url();

    // Only load the initial url once.
    bool isInitial = (url == m_initialURL);
    if (isInitial)
        m_initialURL = KURL();

    BlackBerry::Platform::NetworkRequest platformRequest;
    request.initializePlatformRequest(platformRequest, frame.loader() && frame.loader()->client() && static_cast<FrameLoaderClientBlackBerry*>(frame.loader()->client())->cookiesEnabled(), isInitial, redirectCount);

    const String& documentUrl = frame.document()->url().string();
    if (!documentUrl.isEmpty()) {
        ReadOnlyLatin1String referrer(documentUrl);
        platformRequest.setReferrer(referrer.data(), referrer.length());
    }

    ReadOnlyLatin1String securityOrigin(frame.document()->securityOrigin()->toRawString());
    platformRequest.setSecurityOrigin(securityOrigin.data(), securityOrigin.length());

    // Attach any applicable auth credentials to the NetworkRequest.
    AuthenticationChallenge& challenge = guardJob->getInternal()->m_currentWebChallenge;
    if (!challenge.isNull()) {
        Credential credential = challenge.proposedCredential();
        ProtectionSpace protectionSpace = challenge.protectionSpace();
        ProtectionSpaceServerType type = protectionSpace.serverType();

        String username = credential.user();
        String password = credential.password();

        BlackBerry::Platform::NetworkRequest::AuthType authType = BlackBerry::Platform::NetworkRequest::AuthNone;
        if (type == ProtectionSpaceServerHTTP) {
            switch (protectionSpace.authenticationScheme()) {
            case ProtectionSpaceAuthenticationSchemeHTTPBasic:
                authType = BlackBerry::Platform::NetworkRequest::AuthHTTPBasic;
                break;
            case ProtectionSpaceAuthenticationSchemeHTTPDigest:
                authType = BlackBerry::Platform::NetworkRequest::AuthHTTPDigest;
                break;
            case ProtectionSpaceAuthenticationSchemeNegotiate:
                authType = BlackBerry::Platform::NetworkRequest::AuthNegotiate;
                break;
            case ProtectionSpaceAuthenticationSchemeNTLM:
                authType = BlackBerry::Platform::NetworkRequest::AuthHTTPNTLM;
                break;
            // Lots more cases to handle.
            default:
                // Defaults to AuthNone as per above.
                break;
            }
        } else if (type == ProtectionSpaceServerFTP)
            authType = BlackBerry::Platform::NetworkRequest::AuthFTP;
        else if (type == ProtectionSpaceProxyHTTP)
            authType = BlackBerry::Platform::NetworkRequest::AuthProxy;

        if (authType != BlackBerry::Platform::NetworkRequest::AuthNone)
            platformRequest.setCredentials(username.utf8().data(), password.utf8().data(), authType);
    } else if (url.protocolIsInHTTPFamily()) {
        // For URLs that match the paths of those previously challenged for HTTP Basic authentication,
        // try and reuse the credential preemptively, as allowed by RFC 2617.
        Credential credential = CredentialStorage::get(url);
        if (!credential.isEmpty())
            platformRequest.setCredentials(credential.user().utf8().data(), credential.password().utf8().data(), BlackBerry::Platform::NetworkRequest::AuthHTTPBasic);
    }

    if (!request.overrideContentType().isEmpty())
        platformRequest.setOverrideContentType(request.overrideContentType().latin1().data());

    NetworkJob* networkJob = new NetworkJob;
    if (!networkJob)
        return false;
    if (!networkJob->initialize(playerId, pageGroupName, url, platformRequest, guardJob, streamFactory, frame, deferLoadingCount, redirectCount)) {
        delete networkJob;
        return false;
    }

    // Make sure we have only one NetworkJob for one ResourceHandle.
    ASSERT(!findJobForHandle(guardJob));

    m_jobs.append(networkJob);

    int result = networkJob->streamOpen();
    if (result)
        return false;

    return true;
}

bool NetworkManager::stopJob(PassRefPtr<ResourceHandle> job)
{
    if (NetworkJob* networkJob = findJobForHandle(job))
        return !networkJob->cancelJob();
    return false;
}

NetworkJob* NetworkManager::findJobForHandle(PassRefPtr<ResourceHandle> job)
{
    for (unsigned i = 0; i < m_jobs.size(); ++i) {
        NetworkJob* networkJob = m_jobs[i];
        // We have only one job for one handle (not including cancelled jobs which may hang
        // around briefly), so return the first non-cancelled job.
        if (!networkJob->isCancelled() && networkJob->handle() == job)
            return networkJob;
    }
    return 0;
}

void NetworkManager::deleteJob(NetworkJob* job)
{
    ASSERT(!job->isRunning());
    size_t position = m_jobs.find(job);
    if (position != notFound)
        m_jobs.remove(position);
    delete job;
}

void NetworkManager::setDefersLoading(PassRefPtr<ResourceHandle> job, bool defersLoading)
{
    if (NetworkJob* networkJob = findJobForHandle(job))
        networkJob->updateDeferLoadingCount(defersLoading ? 1 : -1);
}

void NetworkManager::pauseLoad(PassRefPtr<ResourceHandle> job, bool pause)
{
    if (NetworkJob* networkJob = findJobForHandle(job))
        networkJob->streamPause(pause);
}

BlackBerry::Platform::FilterStream* NetworkManager::streamForHandle(PassRefPtr<ResourceHandle> job)
{
    NetworkJob* networkJob = findJobForHandle(job);
    return networkJob ? networkJob->wrappedStream() : 0;
}

} // namespace WebCore
