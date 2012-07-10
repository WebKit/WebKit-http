/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef MessageID_h
#define MessageID_h

namespace CoreIPC {

enum MessageClass {
    MessageClassReserved = 0,

    // Messages sent by Core IPC.
    MessageClassCoreIPC,

    // Messages sent by the UI process to the web process.
    MessageClassAuthenticationManager,
    MessageClassDrawingArea,
    MessageClassInjectedBundle,
    MessageClassLayerTreeCoordinator,
    MessageClassWebApplicationCacheManager,
    MessageClassWebBatteryManagerProxy,
    MessageClassWebCookieManager,
    MessageClassWebDatabaseManager,
    MessageClassWebFullScreenManager,
    MessageClassWebGeolocationManagerProxy,
    MessageClassWebIconDatabaseProxy,
    MessageClassWebInspector,
    MessageClassWebKeyValueStorageManager,
    MessageClassWebMediaCacheManager,
    MessageClassWebNetworkInfoManagerProxy,
    MessageClassWebNotificationManager,
    MessageClassWebPage,
    MessageClassWebProcess,
    MessageClassWebResourceCacheManager,
    MessageClassEventDispatcher,
#if USE(SOUP)
    MessageClassWebSoupRequestManager,
#endif

    // Messages sent by the web process to the UI process.
    MessageClassDownloadProxy,
    MessageClassDrawingAreaProxy,
    MessageClassLayerTreeCoordinatorProxy,
    MessageClassWebApplicationCacheManagerProxy,
    MessageClassWebBatteryManager,
    MessageClassWebContext,
    MessageClassWebContextLegacy,
    MessageClassWebCookieManagerProxy,
    MessageClassWebDatabaseManagerProxy,
    MessageClassWebFullScreenManagerProxy,
    MessageClassWebGeolocationManager,
    MessageClassWebIconDatabase,
    MessageClassWebInspectorProxy,
    MessageClassWebKeyValueStorageManagerProxy,
    MessageClassWebMediaCacheManagerProxy,
    MessageClassWebNetworkInfoManager,
    MessageClassWebNotificationManagerProxy,
    MessageClassWebPageProxy,
    MessageClassWebProcessProxy,
    MessageClassWebResourceCacheManagerProxy,
#if USE(SOUP)
    MessageClassWebSoupRequestManagerProxy,
#endif
    MessageClassWebVibrationProxy,

    // Messages sent to a WebConnection
    MessageClassWebConnectionLegacy,

    // Messages sent by the UI process to the plug-in process.
    MessageClassPluginProcess,

    // Messages sent by the plug-in process to the UI process.
    MessageClassPluginProcessProxy,

    // Messages sent by the web process to the plug-in process.
    MessageClassWebProcessConnection,
    MessageClassPluginControllerProxy,

    // Messages sent by the plug-in process to the web process.
    MessageClassPluginProcessConnection,
    MessageClassPluginProxy,

    // NPObject messages sent by both the plug-in process and the web process.
    MessageClassNPObjectMessageReceiver,
};

template<typename> struct MessageKindTraits { };


/*
    MessageID Layout (the most significant bit is reserved and therefore always zero)

    ---------
    | Flags | 7 bits
    |-------|
    | Class | 8 bits
    |-------|
    |  Msg  | 16 bits
    |  Kind |
    ---------
*/

class MessageID {
public:
    enum Flags {
        SyncMessage = 1 << 0,
        DispatchMessageWhenWaitingForSyncReply = 1 << 1,
    };

    MessageID()
        : m_messageID(0)
    {
    }

    template <typename EnumType>
    explicit MessageID(EnumType messageKind, unsigned char flags = 0)
        : m_messageID(stripMostSignificantBit(flags << 24 | (MessageKindTraits<EnumType>::messageClass) << 16 | messageKind))
    {
    }

    MessageID messageIDWithAddedFlags(unsigned char flags)
    {
        MessageID messageID;

        messageID.m_messageID = stripMostSignificantBit(m_messageID | (flags << 24));
        return messageID;
    }

    template <typename EnumType>
    EnumType get() const
    {
        ASSERT(getClass() == MessageKindTraits<EnumType>::messageClass);
        return static_cast<EnumType>(m_messageID & 0xffff);
    }

    template <MessageClass K>
    bool is() const
    {
        return getClass() == K;
    }
    
    template <typename EnumType>
    bool operator==(EnumType messageKind) const
    {
        return m_messageID == MessageID(messageKind).m_messageID;
    }

    static MessageID fromInt(unsigned i)
    {
        MessageID messageID;
        messageID.m_messageID = stripMostSignificantBit(i);
        
        return messageID;
    }
    
    unsigned toInt() const { return m_messageID; }

    bool shouldDispatchMessageWhenWaitingForSyncReply() const { return getFlags() & DispatchMessageWhenWaitingForSyncReply; }
    bool isSync() const { return getFlags() & SyncMessage; }

private:
    static inline unsigned stripMostSignificantBit(unsigned value)
    {
        return value & 0x7fffffff;
    }

    unsigned char getFlags() const { return (m_messageID & 0xff000000) >> 24; }
    unsigned char getClass() const { return (m_messageID & 0x00ff0000) >> 16; }

    unsigned m_messageID;
};

} // namespace CoreIPC
    
#endif // MessageID_h
