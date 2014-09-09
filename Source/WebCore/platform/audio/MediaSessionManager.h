/*
 * Copyright (C) 2013-2014 Apple Inc. All rights reserved.
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

#ifndef MediaSessionManager_h
#define MediaSessionManager_h

#include "AudioHardwareListener.h"
#include "MediaSession.h"
#include "RemoteCommandListener.h"
#include "Settings.h"
#include "SystemSleepListener.h"
#include <map>
#include <wtf/Vector.h>

namespace WebCore {

class HTMLMediaElement;
class MediaSession;
class RemoteCommandListener;

class MediaSessionManagerClient {
public:
    virtual ~MediaSessionManagerClient() { }

    virtual bool isListeningForRemoteControlCommands() = 0;
    virtual void startListeningForRemoteControlCommands() = 0;
    virtual void stopListeningForRemoteControlCommands() = 0;
    virtual void didBeginPlayback() = 0;

protected:
    MediaSessionManagerClient() { }
};

class MediaSessionManager : private RemoteCommandListenerClient, private SystemSleepListener::Client, private AudioHardwareListener::Client {
public:
    static MediaSessionManager& sharedManager();
    virtual ~MediaSessionManager() { }

    bool has(MediaSession::MediaType) const;
    int count(MediaSession::MediaType) const;
    bool activeAudioSessionRequired() const;

    void beginInterruption(MediaSession::InterruptionType);
    void endInterruption(MediaSession::EndInterruptionFlags);

    void applicationWillEnterForeground() const;
    void applicationWillEnterBackground() const;
    void wirelessRoutesAvailableChanged();

    enum SessionRestrictionFlags {
        NoRestrictions = 0,
        ConcurrentPlaybackNotPermitted = 1 << 0,
        InlineVideoPlaybackRestricted = 1 << 1,
        MetadataPreloadingNotPermitted = 1 << 2,
        AutoPreloadingNotPermitted = 1 << 3,
        BackgroundPlaybackNotPermitted = 1 << 4,
    };
    typedef unsigned SessionRestrictions;
    
    void addRestriction(MediaSession::MediaType, SessionRestrictions);
    void removeRestriction(MediaSession::MediaType, SessionRestrictions);
    SessionRestrictions restrictions(MediaSession::MediaType);
    virtual void resetRestrictions();

    virtual void sessionWillBeginPlayback(MediaSession&);
    virtual void sessionWillEndPlayback(MediaSession&);
    
    bool sessionRestrictsInlineVideoPlayback(const MediaSession&) const;

#if ENABLE(IOS_AIRPLAY)
    virtual bool hasWirelessTargetsAvailable() { return false; }
    virtual void startMonitoringAirPlayRoutes() { }
    virtual void stopMonitoringAirPlayRoutes() { }
#endif

    void addClient(MediaSessionManagerClient*);
    void removeClient(MediaSessionManagerClient*);

protected:
    friend class MediaSession;
    explicit MediaSessionManager();

    void addSession(MediaSession&);
    void removeSession(MediaSession&);
    
    void setCurrentSession(MediaSession&);
    MediaSession* currentSession();
    
private:
    friend class Internals;

    void updateSessionState();


    // RemoteCommandListenerClient
    virtual void didReceiveRemoteControlCommand(MediaSession::RemoteControlCommandType) override;

    // AudioHardwareListenerClient
    virtual void audioHardwareDidBecomeActive() override { }
    virtual void audioHardwareDidBecomeInactive() override { }
    virtual void audioOutputDeviceChanged() override;

    // SystemSleepListener
    virtual void systemWillSleep() override;
    virtual void systemDidWake() override;

    SessionRestrictions m_restrictions[MediaSession::WebAudio + 1];

    Vector<MediaSession*> m_sessions;
    Vector<MediaSessionManagerClient*> m_clients;
    std::unique_ptr<RemoteCommandListener> m_remoteCommandListener;
    std::unique_ptr<SystemSleepListener> m_systemSleepListener;
    RefPtr<AudioHardwareListener> m_audioHardwareListener;
    bool m_interrupted;
};

}

#endif // MediaSessionManager_h
