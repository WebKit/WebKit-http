/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebProcessMessages_h
#define WebProcessMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "SandboxExtension.h"
#include "StringReference.h"
#include "WebsiteDataType.h"
#include <WebCore/SecurityOriginData.h>
#include <WebCore/SessionID.h>
#include <chrono>
#include <wtf/HashMap.h>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class DataReference;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class SessionID;
}

namespace WebKit {
    struct WebPageCreationParameters;
    struct WebProcessCreationParameters;
    struct TextCheckerState;
    class UserData;
}

namespace Messages {
namespace WebProcess {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebProcess");
}

class InitializeWebProcess {
public:
    typedef std::tuple<WebKit::WebProcessCreationParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InitializeWebProcess"); }
    static const bool isSync = false;

    explicit InitializeWebProcess(const WebKit::WebProcessCreationParameters& processCreationParameters)
        : m_arguments(processCreationParameters)
    {
    }

    const std::tuple<const WebKit::WebProcessCreationParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::WebProcessCreationParameters&> m_arguments;
};

class CreateWebPage {
public:
    typedef std::tuple<uint64_t, WebKit::WebPageCreationParameters> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateWebPage"); }
    static const bool isSync = false;

    CreateWebPage(uint64_t newPageID, const WebKit::WebPageCreationParameters& pageCreationParameters)
        : m_arguments(newPageID, pageCreationParameters)
    {
    }

    const std::tuple<uint64_t, const WebKit::WebPageCreationParameters&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::WebPageCreationParameters&> m_arguments;
};

class SetCacheModel {
public:
    typedef std::tuple<uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCacheModel"); }
    static const bool isSync = false;

    explicit SetCacheModel(uint32_t cacheModel)
        : m_arguments(cacheModel)
    {
    }

    const std::tuple<uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t> m_arguments;
};

class RegisterURLSchemeAsEmptyDocument {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsEmptyDocument"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsEmptyDocument(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class RegisterURLSchemeAsSecure {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsSecure"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsSecure(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class RegisterURLSchemeAsBypassingContentSecurityPolicy {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsBypassingContentSecurityPolicy"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsBypassingContentSecurityPolicy(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class SetDomainRelaxationForbiddenForURLScheme {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDomainRelaxationForbiddenForURLScheme"); }
    static const bool isSync = false;

    explicit SetDomainRelaxationForbiddenForURLScheme(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class RegisterURLSchemeAsLocal {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsLocal"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsLocal(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class RegisterURLSchemeAsNoAccess {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsNoAccess"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsNoAccess(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class RegisterURLSchemeAsDisplayIsolated {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsDisplayIsolated"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsDisplayIsolated(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class RegisterURLSchemeAsCORSEnabled {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsCORSEnabled"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsCORSEnabled(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

#if ENABLE(CACHE_PARTITIONING)
class RegisterURLSchemeAsCachePartitioned {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterURLSchemeAsCachePartitioned"); }
    static const bool isSync = false;

    explicit RegisterURLSchemeAsCachePartitioned(const String& scheme)
        : m_arguments(scheme)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

class SetDefaultRequestTimeoutInterval {
public:
    typedef std::tuple<double> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDefaultRequestTimeoutInterval"); }
    static const bool isSync = false;

    explicit SetDefaultRequestTimeoutInterval(double timeoutInterval)
        : m_arguments(timeoutInterval)
    {
    }

    const std::tuple<double>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<double> m_arguments;
};

class SetAlwaysUsesComplexTextCodePath {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAlwaysUsesComplexTextCodePath"); }
    static const bool isSync = false;

    explicit SetAlwaysUsesComplexTextCodePath(bool alwaysUseComplexText)
        : m_arguments(alwaysUseComplexText)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class SetShouldUseFontSmoothing {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetShouldUseFontSmoothing"); }
    static const bool isSync = false;

    explicit SetShouldUseFontSmoothing(bool useFontSmoothing)
        : m_arguments(useFontSmoothing)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class SetResourceLoadStatisticsEnabled {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetResourceLoadStatisticsEnabled"); }
    static const bool isSync = false;

    explicit SetResourceLoadStatisticsEnabled(bool resourceLoadStatisticsEnabled)
        : m_arguments(resourceLoadStatisticsEnabled)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class UserPreferredLanguagesChanged {
public:
    typedef std::tuple<Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UserPreferredLanguagesChanged"); }
    static const bool isSync = false;

    explicit UserPreferredLanguagesChanged(const Vector<String>& languages)
        : m_arguments(languages)
    {
    }

    const std::tuple<const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&> m_arguments;
};

class FullKeyboardAccessModeChanged {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FullKeyboardAccessModeChanged"); }
    static const bool isSync = false;

    explicit FullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled)
        : m_arguments(fullKeyboardAccessEnabled)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class ClearCachedCredentials {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearCachedCredentials"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class EnsurePrivateBrowsingSession {
public:
    typedef std::tuple<WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnsurePrivateBrowsingSession"); }
    static const bool isSync = false;

    explicit EnsurePrivateBrowsingSession(const WebCore::SessionID& sessionID)
        : m_arguments(sessionID)
    {
    }

    const std::tuple<const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&> m_arguments;
};

class DestroyPrivateBrowsingSession {
public:
    typedef std::tuple<WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DestroyPrivateBrowsingSession"); }
    static const bool isSync = false;

    explicit DestroyPrivateBrowsingSession(const WebCore::SessionID& sessionID)
        : m_arguments(sessionID)
    {
    }

    const std::tuple<const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&> m_arguments;
};

class DidAddPlugInAutoStartOriginHash {
public:
    typedef std::tuple<uint32_t, double, WebCore::SessionID> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidAddPlugInAutoStartOriginHash"); }
    static const bool isSync = false;

    DidAddPlugInAutoStartOriginHash(uint32_t hash, double expirationTime, const WebCore::SessionID& sessionID)
        : m_arguments(hash, expirationTime, sessionID)
    {
    }

    const std::tuple<uint32_t, double, const WebCore::SessionID&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t, double, const WebCore::SessionID&> m_arguments;
};

class ResetPlugInAutoStartOriginDefaultHashes {
public:
    typedef std::tuple<HashMap<uint32_t,double>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResetPlugInAutoStartOriginDefaultHashes"); }
    static const bool isSync = false;

    explicit ResetPlugInAutoStartOriginDefaultHashes(const HashMap<uint32_t,double>& hashes)
        : m_arguments(hashes)
    {
    }

    const std::tuple<const HashMap<uint32_t,double>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const HashMap<uint32_t,double>&> m_arguments;
};

class ResetPlugInAutoStartOriginHashes {
public:
    typedef std::tuple<HashMap<WebCore::SessionID, HashMap<uint32_t,double>>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResetPlugInAutoStartOriginHashes"); }
    static const bool isSync = false;

    explicit ResetPlugInAutoStartOriginHashes(const HashMap<WebCore::SessionID, HashMap<uint32_t,double>>& hashes)
        : m_arguments(hashes)
    {
    }

    const std::tuple<const HashMap<WebCore::SessionID, HashMap<uint32_t,double>>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const HashMap<WebCore::SessionID, HashMap<uint32_t,double>>&> m_arguments;
};

class SetPluginLoadClientPolicy {
public:
    typedef std::tuple<uint8_t, String, String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPluginLoadClientPolicy"); }
    static const bool isSync = false;

    SetPluginLoadClientPolicy(uint8_t policy, const String& host, const String& bundleIdentifier, const String& versionString)
        : m_arguments(policy, host, bundleIdentifier, versionString)
    {
    }

    const std::tuple<uint8_t, const String&, const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint8_t, const String&, const String&, const String&> m_arguments;
};

class ClearPluginClientPolicies {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearPluginClientPolicies"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class StartMemorySampler {
public:
    typedef std::tuple<WebKit::SandboxExtension::Handle, String, double> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StartMemorySampler"); }
    static const bool isSync = false;

    StartMemorySampler(const WebKit::SandboxExtension::Handle& sampleLogFileHandle, const String& sampleLogFilePath, double interval)
        : m_arguments(sampleLogFileHandle, sampleLogFilePath, interval)
    {
    }

    const std::tuple<const WebKit::SandboxExtension::Handle&, const String&, double>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::SandboxExtension::Handle&, const String&, double> m_arguments;
};

class StopMemorySampler {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopMemorySampler"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class SetTextCheckerState {
public:
    typedef std::tuple<WebKit::TextCheckerState> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetTextCheckerState"); }
    static const bool isSync = false;

    explicit SetTextCheckerState(const WebKit::TextCheckerState& textCheckerState)
        : m_arguments(textCheckerState)
    {
    }

    const std::tuple<const WebKit::TextCheckerState&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::TextCheckerState&> m_arguments;
};

class SetEnhancedAccessibility {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetEnhancedAccessibility"); }
    static const bool isSync = false;

    explicit SetEnhancedAccessibility(bool flag)
        : m_arguments(flag)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class GetWebCoreStatistics {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetWebCoreStatistics"); }
    static const bool isSync = false;

    explicit GetWebCoreStatistics(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class GarbageCollectJavaScriptObjects {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GarbageCollectJavaScriptObjects"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class SetJavaScriptGarbageCollectorTimerEnabled {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetJavaScriptGarbageCollectorTimerEnabled"); }
    static const bool isSync = false;

    explicit SetJavaScriptGarbageCollectorTimerEnabled(bool enable)
        : m_arguments(enable)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

class SetInjectedBundleParameter {
public:
    typedef std::tuple<String, IPC::DataReference> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetInjectedBundleParameter"); }
    static const bool isSync = false;

    SetInjectedBundleParameter(const String& parameter, const IPC::DataReference& value)
        : m_arguments(parameter, value)
    {
    }

    const std::tuple<const String&, const IPC::DataReference&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const IPC::DataReference&> m_arguments;
};

class SetInjectedBundleParameters {
public:
    typedef std::tuple<IPC::DataReference> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetInjectedBundleParameters"); }
    static const bool isSync = false;

    explicit SetInjectedBundleParameters(const IPC::DataReference& parameters)
        : m_arguments(parameters)
    {
    }

    const std::tuple<const IPC::DataReference&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const IPC::DataReference&> m_arguments;
};

class HandleInjectedBundleMessage {
public:
    typedef std::tuple<String, WebKit::UserData> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleInjectedBundleMessage"); }
    static const bool isSync = false;

    HandleInjectedBundleMessage(const String& messageName, const WebKit::UserData& messageBody)
        : m_arguments(messageName, messageBody)
    {
    }

    const std::tuple<const String&, const WebKit::UserData&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const WebKit::UserData&> m_arguments;
};

class ReleasePageCache {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReleasePageCache"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class FetchWebsiteData {
public:
    typedef std::tuple<WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FetchWebsiteData"); }
    static const bool isSync = false;

    FetchWebsiteData(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, callbackID)
    {
    }

    const std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, uint64_t> m_arguments;
};

class DeleteWebsiteData {
public:
    typedef std::tuple<WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, std::chrono::system_clock::time_point, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteData"); }
    static const bool isSync = false;

    DeleteWebsiteData(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const std::chrono::system_clock::time_point& modifiedSince, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, modifiedSince, callbackID)
    {
    }

    const std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const std::chrono::system_clock::time_point&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const std::chrono::system_clock::time_point&, uint64_t> m_arguments;
};

class DeleteWebsiteDataForOrigins {
public:
    typedef std::tuple<WebCore::SessionID, OptionSet<WebKit::WebsiteDataType>, Vector<WebCore::SecurityOriginData>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteWebsiteDataForOrigins"); }
    static const bool isSync = false;

    DeleteWebsiteDataForOrigins(const WebCore::SessionID& sessionID, const OptionSet<WebKit::WebsiteDataType>& websiteDataTypes, const Vector<WebCore::SecurityOriginData>& origins, uint64_t callbackID)
        : m_arguments(sessionID, websiteDataTypes, origins, callbackID)
    {
    }

    const std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const Vector<WebCore::SecurityOriginData>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::SessionID&, const OptionSet<WebKit::WebsiteDataType>&, const Vector<WebCore::SecurityOriginData>&, uint64_t> m_arguments;
};

class SetHiddenPageTimerThrottlingIncreaseLimit {
public:
    typedef std::tuple<int> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetHiddenPageTimerThrottlingIncreaseLimit"); }
    static const bool isSync = false;

    explicit SetHiddenPageTimerThrottlingIncreaseLimit(const int& milliseconds)
        : m_arguments(milliseconds)
    {
    }

    const std::tuple<const int&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const int&> m_arguments;
};

class SetProcessSuppressionEnabled {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetProcessSuppressionEnabled"); }
    static const bool isSync = false;

    explicit SetProcessSuppressionEnabled(bool flag)
        : m_arguments(flag)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

#if PLATFORM(COCOA)
class SetQOS {
public:
    typedef std::tuple<int, int> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetQOS"); }
    static const bool isSync = false;

    SetQOS(const int& latencyQOS, const int& throughputQOS)
        : m_arguments(latencyQOS, throughputQOS)
    {
    }

    const std::tuple<const int&, const int&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const int&, const int&> m_arguments;
};
#endif

class SetMemoryCacheDisabled {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMemoryCacheDisabled"); }
    static const bool isSync = false;

    explicit SetMemoryCacheDisabled(bool disabled)
        : m_arguments(disabled)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

#if ENABLE(SERVICE_CONTROLS)
class SetEnabledServices {
public:
    typedef std::tuple<bool, bool, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetEnabledServices"); }
    static const bool isSync = false;

    SetEnabledServices(bool hasImageServices, bool hasSelectionServices, bool hasRichContentServices)
        : m_arguments(hasImageServices, hasSelectionServices, hasRichContentServices)
    {
    }

    const std::tuple<bool, bool, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool, bool, bool> m_arguments;
};
#endif

class EnsureAutomationSessionProxy {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnsureAutomationSessionProxy"); }
    static const bool isSync = false;

    explicit EnsureAutomationSessionProxy(const String& sessionIdentifier)
        : m_arguments(sessionIdentifier)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

class DestroyAutomationSessionProxy {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DestroyAutomationSessionProxy"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class ProcessWillSuspendImminently {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProcessWillSuspendImminently"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class PrepareToSuspend {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PrepareToSuspend"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class CancelPrepareToSuspend {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelPrepareToSuspend"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class ProcessDidResume {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProcessDidResume"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class MainThreadPing {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MainThreadPing"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

} // namespace WebProcess
} // namespace Messages

#endif // WebProcessMessages_h
