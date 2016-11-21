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

#include "config.h"

#include "WebProcess.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "Decoder.h"
#if ENABLE(GAMEPAD)
#include "GamepadData.h"
#endif
#include "HandleMessage.h"
#include "SandboxExtension.h"
#include "TextCheckerState.h"
#include "UserData.h"
#include "WebCoreArgumentCoders.h"
#include "WebPageCreationParameters.h"
#include "WebProcessCreationParameters.h"
#include "WebProcessMessages.h"
#include "WebsiteDataType.h"
#include <WebCore/SecurityOriginData.h>
#include <WebCore/SessionID.h>
#include <chrono>
#include <wtf/HashMap.h>
#include <wtf/OptionSet.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebProcess::didReceiveWebProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebProcess::InitializeWebProcess::name()) {
        IPC::handleMessage<Messages::WebProcess::InitializeWebProcess>(decoder, this, &WebProcess::initializeWebProcess);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::CreateWebPage::name()) {
        IPC::handleMessage<Messages::WebProcess::CreateWebPage>(decoder, this, &WebProcess::createWebPage);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetCacheModel::name()) {
        IPC::handleMessage<Messages::WebProcess::SetCacheModel>(decoder, this, &WebProcess::setCacheModel);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsEmptyDocument::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsEmptyDocument>(decoder, this, &WebProcess::registerURLSchemeAsEmptyDocument);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsSecure::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsSecure>(decoder, this, &WebProcess::registerURLSchemeAsSecure);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsBypassingContentSecurityPolicy::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsBypassingContentSecurityPolicy>(decoder, this, &WebProcess::registerURLSchemeAsBypassingContentSecurityPolicy);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetDomainRelaxationForbiddenForURLScheme::name()) {
        IPC::handleMessage<Messages::WebProcess::SetDomainRelaxationForbiddenForURLScheme>(decoder, this, &WebProcess::setDomainRelaxationForbiddenForURLScheme);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsLocal::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsLocal>(decoder, this, &WebProcess::registerURLSchemeAsLocal);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsNoAccess::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsNoAccess>(decoder, this, &WebProcess::registerURLSchemeAsNoAccess);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsDisplayIsolated::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsDisplayIsolated>(decoder, this, &WebProcess::registerURLSchemeAsDisplayIsolated);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsCORSEnabled::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsCORSEnabled>(decoder, this, &WebProcess::registerURLSchemeAsCORSEnabled);
        return;
    }
#if ENABLE(CACHE_PARTITIONING)
    if (decoder.messageName() == Messages::WebProcess::RegisterURLSchemeAsCachePartitioned::name()) {
        IPC::handleMessage<Messages::WebProcess::RegisterURLSchemeAsCachePartitioned>(decoder, this, &WebProcess::registerURLSchemeAsCachePartitioned);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebProcess::SetDefaultRequestTimeoutInterval::name()) {
        IPC::handleMessage<Messages::WebProcess::SetDefaultRequestTimeoutInterval>(decoder, this, &WebProcess::setDefaultRequestTimeoutInterval);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetAlwaysUsesComplexTextCodePath::name()) {
        IPC::handleMessage<Messages::WebProcess::SetAlwaysUsesComplexTextCodePath>(decoder, this, &WebProcess::setAlwaysUsesComplexTextCodePath);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetShouldUseFontSmoothing::name()) {
        IPC::handleMessage<Messages::WebProcess::SetShouldUseFontSmoothing>(decoder, this, &WebProcess::setShouldUseFontSmoothing);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetResourceLoadStatisticsEnabled::name()) {
        IPC::handleMessage<Messages::WebProcess::SetResourceLoadStatisticsEnabled>(decoder, this, &WebProcess::setResourceLoadStatisticsEnabled);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::UserPreferredLanguagesChanged::name()) {
        IPC::handleMessage<Messages::WebProcess::UserPreferredLanguagesChanged>(decoder, this, &WebProcess::userPreferredLanguagesChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::FullKeyboardAccessModeChanged::name()) {
        IPC::handleMessage<Messages::WebProcess::FullKeyboardAccessModeChanged>(decoder, this, &WebProcess::fullKeyboardAccessModeChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::ClearCachedCredentials::name()) {
        IPC::handleMessage<Messages::WebProcess::ClearCachedCredentials>(decoder, this, &WebProcess::clearCachedCredentials);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::EnsurePrivateBrowsingSession::name()) {
        IPC::handleMessage<Messages::WebProcess::EnsurePrivateBrowsingSession>(decoder, this, &WebProcess::ensurePrivateBrowsingSession);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::DestroyPrivateBrowsingSession::name()) {
        IPC::handleMessage<Messages::WebProcess::DestroyPrivateBrowsingSession>(decoder, this, &WebProcess::destroyPrivateBrowsingSession);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::DidAddPlugInAutoStartOriginHash::name()) {
        IPC::handleMessage<Messages::WebProcess::DidAddPlugInAutoStartOriginHash>(decoder, this, &WebProcess::didAddPlugInAutoStartOriginHash);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::ResetPlugInAutoStartOriginDefaultHashes::name()) {
        IPC::handleMessage<Messages::WebProcess::ResetPlugInAutoStartOriginDefaultHashes>(decoder, this, &WebProcess::resetPlugInAutoStartOriginDefaultHashes);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::ResetPlugInAutoStartOriginHashes::name()) {
        IPC::handleMessage<Messages::WebProcess::ResetPlugInAutoStartOriginHashes>(decoder, this, &WebProcess::resetPlugInAutoStartOriginHashes);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetPluginLoadClientPolicy::name()) {
        IPC::handleMessage<Messages::WebProcess::SetPluginLoadClientPolicy>(decoder, this, &WebProcess::setPluginLoadClientPolicy);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::ClearPluginClientPolicies::name()) {
        IPC::handleMessage<Messages::WebProcess::ClearPluginClientPolicies>(decoder, this, &WebProcess::clearPluginClientPolicies);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::StartMemorySampler::name()) {
        IPC::handleMessage<Messages::WebProcess::StartMemorySampler>(decoder, this, &WebProcess::startMemorySampler);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::StopMemorySampler::name()) {
        IPC::handleMessage<Messages::WebProcess::StopMemorySampler>(decoder, this, &WebProcess::stopMemorySampler);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetTextCheckerState::name()) {
        IPC::handleMessage<Messages::WebProcess::SetTextCheckerState>(decoder, this, &WebProcess::setTextCheckerState);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetEnhancedAccessibility::name()) {
        IPC::handleMessage<Messages::WebProcess::SetEnhancedAccessibility>(decoder, this, &WebProcess::setEnhancedAccessibility);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::GetWebCoreStatistics::name()) {
        IPC::handleMessage<Messages::WebProcess::GetWebCoreStatistics>(decoder, this, &WebProcess::getWebCoreStatistics);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::GarbageCollectJavaScriptObjects::name()) {
        IPC::handleMessage<Messages::WebProcess::GarbageCollectJavaScriptObjects>(decoder, this, &WebProcess::garbageCollectJavaScriptObjects);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetJavaScriptGarbageCollectorTimerEnabled::name()) {
        IPC::handleMessage<Messages::WebProcess::SetJavaScriptGarbageCollectorTimerEnabled>(decoder, this, &WebProcess::setJavaScriptGarbageCollectorTimerEnabled);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetInjectedBundleParameter::name()) {
        IPC::handleMessage<Messages::WebProcess::SetInjectedBundleParameter>(decoder, this, &WebProcess::setInjectedBundleParameter);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetInjectedBundleParameters::name()) {
        IPC::handleMessage<Messages::WebProcess::SetInjectedBundleParameters>(decoder, this, &WebProcess::setInjectedBundleParameters);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::HandleInjectedBundleMessage::name()) {
        IPC::handleMessage<Messages::WebProcess::HandleInjectedBundleMessage>(decoder, this, &WebProcess::handleInjectedBundleMessage);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::ReleasePageCache::name()) {
        IPC::handleMessage<Messages::WebProcess::ReleasePageCache>(decoder, this, &WebProcess::releasePageCache);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::FetchWebsiteData::name()) {
        IPC::handleMessage<Messages::WebProcess::FetchWebsiteData>(decoder, this, &WebProcess::fetchWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::DeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::WebProcess::DeleteWebsiteData>(decoder, this, &WebProcess::deleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::DeleteWebsiteDataForOrigins::name()) {
        IPC::handleMessage<Messages::WebProcess::DeleteWebsiteDataForOrigins>(decoder, this, &WebProcess::deleteWebsiteDataForOrigins);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetHiddenPageTimerThrottlingIncreaseLimit::name()) {
        IPC::handleMessage<Messages::WebProcess::SetHiddenPageTimerThrottlingIncreaseLimit>(decoder, this, &WebProcess::setHiddenPageTimerThrottlingIncreaseLimit);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::SetProcessSuppressionEnabled::name()) {
        IPC::handleMessage<Messages::WebProcess::SetProcessSuppressionEnabled>(decoder, this, &WebProcess::setProcessSuppressionEnabled);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebProcess::SetQOS::name()) {
        IPC::handleMessage<Messages::WebProcess::SetQOS>(decoder, this, &WebProcess::setQOS);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebProcess::SetMemoryCacheDisabled::name()) {
        IPC::handleMessage<Messages::WebProcess::SetMemoryCacheDisabled>(decoder, this, &WebProcess::setMemoryCacheDisabled);
        return;
    }
#if ENABLE(SERVICE_CONTROLS)
    if (decoder.messageName() == Messages::WebProcess::SetEnabledServices::name()) {
        IPC::handleMessage<Messages::WebProcess::SetEnabledServices>(decoder, this, &WebProcess::setEnabledServices);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebProcess::EnsureAutomationSessionProxy::name()) {
        IPC::handleMessage<Messages::WebProcess::EnsureAutomationSessionProxy>(decoder, this, &WebProcess::ensureAutomationSessionProxy);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::DestroyAutomationSessionProxy::name()) {
        IPC::handleMessage<Messages::WebProcess::DestroyAutomationSessionProxy>(decoder, this, &WebProcess::destroyAutomationSessionProxy);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::PrepareToSuspend::name()) {
        IPC::handleMessage<Messages::WebProcess::PrepareToSuspend>(decoder, this, &WebProcess::prepareToSuspend);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::CancelPrepareToSuspend::name()) {
        IPC::handleMessage<Messages::WebProcess::CancelPrepareToSuspend>(decoder, this, &WebProcess::cancelPrepareToSuspend);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::ProcessDidResume::name()) {
        IPC::handleMessage<Messages::WebProcess::ProcessDidResume>(decoder, this, &WebProcess::processDidResume);
        return;
    }
    if (decoder.messageName() == Messages::WebProcess::MainThreadPing::name()) {
        IPC::handleMessage<Messages::WebProcess::MainThreadPing>(decoder, this, &WebProcess::mainThreadPing);
        return;
    }
#if ENABLE(GAMEPAD)
    if (decoder.messageName() == Messages::WebProcess::SetInitialGamepads::name()) {
        IPC::handleMessage<Messages::WebProcess::SetInitialGamepads>(decoder, this, &WebProcess::setInitialGamepads);
        return;
    }
#endif
#if ENABLE(GAMEPAD)
    if (decoder.messageName() == Messages::WebProcess::GamepadConnected::name()) {
        IPC::handleMessage<Messages::WebProcess::GamepadConnected>(decoder, this, &WebProcess::gamepadConnected);
        return;
    }
#endif
#if ENABLE(GAMEPAD)
    if (decoder.messageName() == Messages::WebProcess::GamepadDisconnected::name()) {
        IPC::handleMessage<Messages::WebProcess::GamepadDisconnected>(decoder, this, &WebProcess::gamepadDisconnected);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebProcess::didReceiveSyncWebProcessMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::WebProcess::ProcessWillSuspendImminently::name()) {
        IPC::handleMessage<Messages::WebProcess::ProcessWillSuspendImminently>(decoder, *replyEncoder, this, &WebProcess::processWillSuspendImminently);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
