/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#if ENABLE(ENCRYPTED_MEDIA_V2)

#include "CDM.h"

#include "CDMPrivateMediaPlayer.h"
#include "CDMSession.h"
#include "MediaKeyError.h"
#include "MediaKeys.h"
#include "MediaPlayer.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC) && ENABLE(MEDIA_SOURCE)
#include "CDMPrivateMediaSourceAVFObjC.h"
#endif

namespace WebCore {

struct CDMFactory {
    WTF_MAKE_NONCOPYABLE(CDMFactory); WTF_MAKE_FAST_ALLOCATED;
public:
    CDMFactory(CreateCDM constructor, CDMSupportsKeySystem supportsKeySystem, CDMSupportsKeySystemAndMimeType supportsKeySystemAndMimeType)
        : constructor(constructor)
        , supportsKeySystem(supportsKeySystem)
        , supportsKeySystemAndMimeType(supportsKeySystemAndMimeType)
    {
    }

    CreateCDM constructor;
    CDMSupportsKeySystem supportsKeySystem;
    CDMSupportsKeySystemAndMimeType supportsKeySystemAndMimeType;
};

static Vector<CDMFactory*>& installedCDMFactories()
{
    static NeverDestroyed<Vector<CDMFactory*>> cdms;
    static bool queriedCDMs = false;
    if (!queriedCDMs) {
        queriedCDMs = true;

        // FIXME: initialize specific UA CDMs. http://webkit.org/b/109318, http://webkit.org/b/109320
        cdms.get().append(new CDMFactory(CDMPrivateMediaPlayer::create, CDMPrivateMediaPlayer::supportsKeySystem, CDMPrivateMediaPlayer::supportsKeySystemAndMimeType));

#if PLATFORM(MAC) && ENABLE(MEDIA_SOURCE)
        cdms.get().append(new CDMFactory(CDMPrivateMediaSourceAVFObjC::create, CDMPrivateMediaSourceAVFObjC::supportsKeySystem, CDMPrivateMediaSourceAVFObjC::supportsKeySystemAndMimeType));
#endif
    }

    return cdms;
}

void CDM::registerCDMFactory(CreateCDM constructor, CDMSupportsKeySystem supportsKeySystem, CDMSupportsKeySystemAndMimeType supportsKeySystemAndMimeType)
{
    installedCDMFactories().append(new CDMFactory(constructor, supportsKeySystem, supportsKeySystemAndMimeType));
}

static CDMFactory* CDMFactoryForKeySystem(const String& keySystem)
{
    Vector<CDMFactory*>& cdmFactories = installedCDMFactories();
    for (size_t i = 0; i < cdmFactories.size(); ++i) {
        if (cdmFactories[i]->supportsKeySystem(keySystem))
            return cdmFactories[i];
    }
    return 0;
}

bool CDM::supportsKeySystem(const String& keySystem)
{
    return CDMFactoryForKeySystem(keySystem);
}

bool CDM::keySystemSupportsMimeType(const String& keySystem, const String& mimeType)
{
    if (CDMFactory* factory = CDMFactoryForKeySystem(keySystem))
        return factory->supportsKeySystemAndMimeType(keySystem, mimeType);
    return false;
}

std::unique_ptr<CDM> CDM::create(const String& keySystem)
{
    if (!supportsKeySystem(keySystem))
        return nullptr;

    return std::make_unique<CDM>(keySystem);
}

CDM::CDM(const String& keySystem)
    : m_keySystem(keySystem)
    , m_client(0)
{
    m_private = CDMFactoryForKeySystem(keySystem)->constructor(this);
}

CDM::~CDM()
{
}

bool CDM::supportsMIMEType(const String& mimeType) const
{
    return m_private->supportsMIMEType(mimeType);
}

std::unique_ptr<CDMSession> CDM::createSession()
{
    std::unique_ptr<CDMSession> session = m_private->createSession();
    if (mediaPlayer())
        mediaPlayer()->setCDMSession(session.get());
    return session;
}

MediaPlayer* CDM::mediaPlayer() const
{
    if (!m_client)
        return 0;
    return m_client->cdmMediaPlayer(this);
}

}

#endif // ENABLE(ENCRYPTED_MEDIA_V2)
