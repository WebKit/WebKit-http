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

#include "WebPageProxy.h"

#include "ArgumentCoders.h"
#if PLATFORM(IOS)
#include "AssistedNodeInformation.h"
#endif
#if PLATFORM(MAC)
#include "AttributedString.h"
#endif
#if ENABLE(CONTEXT_MENUS)
#include "ContextMenuContextData.h"
#endif
#if ENABLE(DATA_DETECTION)
#include "DataDetectionResult.h"
#endif
#include "DataReference.h"
#include "Decoder.h"
#include "DownloadID.h"
#include "EditingRange.h"
#include "EditorState.h"
#include "HandleMessage.h"
#if PLATFORM(IOS)
#include "InteractionInformationAtPosition.h"
#endif
#include "NavigationActionData.h"
#include "PlatformPopupMenuData.h"
#if USE(QUICK_LOOK)
#include "QuickLookDocumentData.h"
#endif
#include "SandboxExtension.h"
#include "ShareableBitmap.h"
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT) || PLATFORM(IOS)
#include "SharedMemory.h"
#endif
#include "UserData.h"
#include "WebCoreArgumentCoders.h"
#include "WebHitTestResultData.h"
#include "WebNavigationDataStore.h"
#include "WebPageCreationParameters.h"
#include "WebPageProxyMessages.h"
#include "WebPopupItem.h"
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/CertificateInfo.h>
#include <WebCore/Color.h>
#if ENABLE(CONTENT_FILTERING)
#include <WebCore/ContentFilterUnblockHandler.h>
#endif
#if !PLATFORM(IOS)
#include <WebCore/Cursor.h>
#endif
#if PLATFORM(COCOA)
#include <WebCore/DictionaryPopupInfo.h>
#endif
#if PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
#include <WebCore/DragData.h>
#endif
#include <WebCore/FileChooser.h>
#if PLATFORM(IOS)
#include <WebCore/FloatPoint.h>
#endif
#if PLATFORM(IOS)
#include <WebCore/FloatQuad.h>
#endif
#include <WebCore/FloatRect.h>
#if PLATFORM(IOS)
#include <WebCore/FloatSize.h>
#endif
#if PLATFORM(IOS)
#include <WebCore/InspectorOverlay.h>
#endif
#include <WebCore/IntPoint.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#include <WebCore/JSDOMBinding.h>
#if PLATFORM(COCOA)
#include <WebCore/MachSendRight.h>
#endif
#if ENABLE(MEDIA_SESSION)
#include <WebCore/MediaSessionMetadata.h>
#endif
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/ResourceResponse.h>
#include <WebCore/SearchPopupMenu.h>
#include <WebCore/SecurityOriginData.h>
#include <WebCore/TextCheckerClient.h>
#include <WebCore/TextChecking.h>
#include <WebCore/TextIndicator.h>
#include <WebCore/ViewportArguments.h>
#include <WebCore/WindowFeatures.h>
#include <utility>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace Messages {

namespace WebPageProxy {

RunJavaScriptAlert::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

RunJavaScriptAlert::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool RunJavaScriptAlert::DelayedReply::send()
{
    ASSERT(m_encoder);
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

RunJavaScriptConfirm::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

RunJavaScriptConfirm::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool RunJavaScriptConfirm::DelayedReply::send(bool result)
{
    ASSERT(m_encoder);
    *m_encoder << result;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

RunJavaScriptPrompt::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

RunJavaScriptPrompt::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool RunJavaScriptPrompt::DelayedReply::send(const String& result)
{
    ASSERT(m_encoder);
    *m_encoder << result;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

RunBeforeUnloadConfirmPanel::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

RunBeforeUnloadConfirmPanel::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool RunBeforeUnloadConfirmPanel::DelayedReply::send(bool shouldClose)
{
    ASSERT(m_encoder);
    *m_encoder << shouldClose;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

ExceededDatabaseQuota::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

ExceededDatabaseQuota::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool ExceededDatabaseQuota::DelayedReply::send(uint64_t newQuota)
{
    ASSERT(m_encoder);
    *m_encoder << newQuota;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

ReachedApplicationCacheOriginQuota::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

ReachedApplicationCacheOriginQuota::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool ReachedApplicationCacheOriginQuota::DelayedReply::send(uint64_t newQuota)
{
    ASSERT(m_encoder);
    *m_encoder << newQuota;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

} // namespace WebPageProxy

} // namespace Messages

namespace WebKit {

void WebPageProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebPageProxy::ShowPage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowPage>(decoder, this, &WebPageProxy::showPage);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ClosePage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ClosePage>(decoder, this, &WebPageProxy::closePage);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::MouseDidMoveOverElement::name()) {
        IPC::handleMessage<Messages::WebPageProxy::MouseDidMoveOverElement>(decoder, this, &WebPageProxy::mouseDidMoveOverElement);
        return;
    }
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (decoder.messageName() == Messages::WebPageProxy::UnavailablePluginButtonClicked::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UnavailablePluginButtonClicked>(decoder, this, &WebPageProxy::unavailablePluginButtonClicked);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::DidChangeViewportProperties::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangeViewportProperties>(decoder, this, &WebPageProxy::didChangeViewportProperties);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidReceiveEvent::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidReceiveEvent>(decoder, this, &WebPageProxy::didReceiveEvent);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::StopResponsivenessTimer::name()) {
        IPC::handleMessage<Messages::WebPageProxy::StopResponsivenessTimer>(decoder, this, &WebPageProxy::stopResponsivenessTimer);
        return;
    }
#if !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::SetCursor::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetCursor>(decoder, this, &WebPageProxy::setCursor);
        return;
    }
#endif
#if !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::SetCursorHiddenUntilMouseMoves::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetCursorHiddenUntilMouseMoves>(decoder, this, &WebPageProxy::setCursorHiddenUntilMouseMoves);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::SetStatusText::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetStatusText>(decoder, this, &WebPageProxy::setStatusText);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetToolTip::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetToolTip>(decoder, this, &WebPageProxy::setToolTip);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetFocus::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetFocus>(decoder, this, &WebPageProxy::setFocus);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::TakeFocus::name()) {
        IPC::handleMessage<Messages::WebPageProxy::TakeFocus>(decoder, this, &WebPageProxy::takeFocus);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::FocusedFrameChanged::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FocusedFrameChanged>(decoder, this, &WebPageProxy::focusedFrameChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::FrameSetLargestFrameChanged::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FrameSetLargestFrameChanged>(decoder, this, &WebPageProxy::frameSetLargestFrameChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetRenderTreeSize::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetRenderTreeSize>(decoder, this, &WebPageProxy::setRenderTreeSize);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetToolbarsAreVisible::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetToolbarsAreVisible>(decoder, this, &WebPageProxy::setToolbarsAreVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetMenuBarIsVisible::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetMenuBarIsVisible>(decoder, this, &WebPageProxy::setMenuBarIsVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetStatusBarIsVisible::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetStatusBarIsVisible>(decoder, this, &WebPageProxy::setStatusBarIsVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetIsResizable::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetIsResizable>(decoder, this, &WebPageProxy::setIsResizable);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetWindowFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetWindowFrame>(decoder, this, &WebPageProxy::setWindowFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::PageDidScroll::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PageDidScroll>(decoder, this, &WebPageProxy::pageDidScroll);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RunOpenPanel::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RunOpenPanel>(decoder, this, &WebPageProxy::runOpenPanel);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RunModal::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RunModal>(decoder, this, &WebPageProxy::runModal);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::NotifyScrollerThumbIsVisibleInRect::name()) {
        IPC::handleMessage<Messages::WebPageProxy::NotifyScrollerThumbIsVisibleInRect>(decoder, this, &WebPageProxy::notifyScrollerThumbIsVisibleInRect);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RecommendedScrollbarStyleDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RecommendedScrollbarStyleDidChange>(decoder, this, &WebPageProxy::recommendedScrollbarStyleDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidChangeScrollbarsForMainFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangeScrollbarsForMainFrame>(decoder, this, &WebPageProxy::didChangeScrollbarsForMainFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidChangeScrollOffsetPinningForMainFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangeScrollOffsetPinningForMainFrame>(decoder, this, &WebPageProxy::didChangeScrollOffsetPinningForMainFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidChangePageCount::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangePageCount>(decoder, this, &WebPageProxy::didChangePageCount);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::PageExtendedBackgroundColorDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PageExtendedBackgroundColorDidChange>(decoder, this, &WebPageProxy::pageExtendedBackgroundColorDidChange);
        return;
    }
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (decoder.messageName() == Messages::WebPageProxy::DidFailToInitializePlugin::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFailToInitializePlugin>(decoder, this, &WebPageProxy::didFailToInitializePlugin);
        return;
    }
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (decoder.messageName() == Messages::WebPageProxy::DidBlockInsecurePluginVersion::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidBlockInsecurePluginVersion>(decoder, this, &WebPageProxy::didBlockInsecurePluginVersion);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::SetCanShortCircuitHorizontalWheelEvents::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetCanShortCircuitHorizontalWheelEvents>(decoder, this, &WebPageProxy::setCanShortCircuitHorizontalWheelEvents);
        return;
    }
#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    if (decoder.messageName() == Messages::WebPageProxy::PageDidRequestScroll::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PageDidRequestScroll>(decoder, this, &WebPageProxy::pageDidRequestScroll);
        return;
    }
#endif
#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    if (decoder.messageName() == Messages::WebPageProxy::PageTransitionViewportReady::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PageTransitionViewportReady>(decoder, this, &WebPageProxy::pageTransitionViewportReady);
        return;
    }
#endif
#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    if (decoder.messageName() == Messages::WebPageProxy::DidFindZoomableArea::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFindZoomableArea>(decoder, this, &WebPageProxy::didFindZoomableArea);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::DidChangeContentSize::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangeContentSize>(decoder, this, &WebPageProxy::didChangeContentSize);
        return;
    }
#if ENABLE(INPUT_TYPE_COLOR)
    if (decoder.messageName() == Messages::WebPageProxy::ShowColorPicker::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowColorPicker>(decoder, this, &WebPageProxy::showColorPicker);
        return;
    }
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (decoder.messageName() == Messages::WebPageProxy::SetColorPickerColor::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetColorPickerColor>(decoder, this, &WebPageProxy::setColorPickerColor);
        return;
    }
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (decoder.messageName() == Messages::WebPageProxy::EndColorPicker::name()) {
        IPC::handleMessage<Messages::WebPageProxy::EndColorPicker>(decoder, this, &WebPageProxy::endColorPicker);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::WillAddDetailedMessageToConsole::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WillAddDetailedMessageToConsole>(decoder, this, &WebPageProxy::willAddDetailedMessageToConsole);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DecidePolicyForNewWindowAction::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DecidePolicyForNewWindowAction>(decoder, this, &WebPageProxy::decidePolicyForNewWindowAction);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::UnableToImplementPolicy::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UnableToImplementPolicy>(decoder, this, &WebPageProxy::unableToImplementPolicy);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidChangeProgress::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangeProgress>(decoder, this, &WebPageProxy::didChangeProgress);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFinishProgress::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFinishProgress>(decoder, this, &WebPageProxy::didFinishProgress);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidStartProgress::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidStartProgress>(decoder, this, &WebPageProxy::didStartProgress);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetNetworkRequestsInProgress::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetNetworkRequestsInProgress>(decoder, this, &WebPageProxy::setNetworkRequestsInProgress);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidCreateMainFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidCreateMainFrame>(decoder, this, &WebPageProxy::didCreateMainFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidCreateSubframe::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidCreateSubframe>(decoder, this, &WebPageProxy::didCreateSubframe);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidStartProvisionalLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidStartProvisionalLoadForFrame>(decoder, this, &WebPageProxy::didStartProvisionalLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidReceiveServerRedirectForProvisionalLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidReceiveServerRedirectForProvisionalLoadForFrame>(decoder, this, &WebPageProxy::didReceiveServerRedirectForProvisionalLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidChangeProvisionalURLForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidChangeProvisionalURLForFrame>(decoder, this, &WebPageProxy::didChangeProvisionalURLForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFailProvisionalLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFailProvisionalLoadForFrame>(decoder, this, &WebPageProxy::didFailProvisionalLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidCommitLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidCommitLoadForFrame>(decoder, this, &WebPageProxy::didCommitLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFailLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFailLoadForFrame>(decoder, this, &WebPageProxy::didFailLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFinishDocumentLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFinishDocumentLoadForFrame>(decoder, this, &WebPageProxy::didFinishDocumentLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFinishLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFinishLoadForFrame>(decoder, this, &WebPageProxy::didFinishLoadForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFirstLayoutForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFirstLayoutForFrame>(decoder, this, &WebPageProxy::didFirstLayoutForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFirstVisuallyNonEmptyLayoutForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFirstVisuallyNonEmptyLayoutForFrame>(decoder, this, &WebPageProxy::didFirstVisuallyNonEmptyLayoutForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidReachLayoutMilestone::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidReachLayoutMilestone>(decoder, this, &WebPageProxy::didReachLayoutMilestone);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidReceiveTitleForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidReceiveTitleForFrame>(decoder, this, &WebPageProxy::didReceiveTitleForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidDisplayInsecureContentForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidDisplayInsecureContentForFrame>(decoder, this, &WebPageProxy::didDisplayInsecureContentForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidRunInsecureContentForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidRunInsecureContentForFrame>(decoder, this, &WebPageProxy::didRunInsecureContentForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidDetectXSSForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidDetectXSSForFrame>(decoder, this, &WebPageProxy::didDetectXSSForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidSameDocumentNavigationForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidSameDocumentNavigationForFrame>(decoder, this, &WebPageProxy::didSameDocumentNavigationForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidDestroyNavigation::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidDestroyNavigation>(decoder, this, &WebPageProxy::didDestroyNavigation);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::MainFramePluginHandlesPageScaleGestureDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::MainFramePluginHandlesPageScaleGestureDidChange>(decoder, this, &WebPageProxy::mainFramePluginHandlesPageScaleGestureDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::FrameDidBecomeFrameSet::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FrameDidBecomeFrameSet>(decoder, this, &WebPageProxy::frameDidBecomeFrameSet);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidNavigateWithNavigationData::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidNavigateWithNavigationData>(decoder, this, &WebPageProxy::didNavigateWithNavigationData);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidPerformClientRedirect::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidPerformClientRedirect>(decoder, this, &WebPageProxy::didPerformClientRedirect);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidPerformServerRedirect::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidPerformServerRedirect>(decoder, this, &WebPageProxy::didPerformServerRedirect);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidUpdateHistoryTitle::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidUpdateHistoryTitle>(decoder, this, &WebPageProxy::didUpdateHistoryTitle);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFinishLoadingDataForCustomContentProvider::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFinishLoadingDataForCustomContentProvider>(decoder, this, &WebPageProxy::didFinishLoadingDataForCustomContentProvider);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::WillSubmitForm::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WillSubmitForm>(decoder, this, &WebPageProxy::willSubmitForm);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::VoidCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::VoidCallback>(decoder, this, &WebPageProxy::voidCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DataCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DataCallback>(decoder, this, &WebPageProxy::dataCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ImageCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ImageCallback>(decoder, this, &WebPageProxy::imageCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::StringCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::StringCallback>(decoder, this, &WebPageProxy::stringCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ScriptValueCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ScriptValueCallback>(decoder, this, &WebPageProxy::scriptValueCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ComputedPagesCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ComputedPagesCallback>(decoder, this, &WebPageProxy::computedPagesCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ValidateCommandCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ValidateCommandCallback>(decoder, this, &WebPageProxy::validateCommandCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::EditingRangeCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::EditingRangeCallback>(decoder, this, &WebPageProxy::editingRangeCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::UnsignedCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UnsignedCallback>(decoder, this, &WebPageProxy::unsignedCallback);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RectForCharacterRangeCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RectForCharacterRangeCallback>(decoder, this, &WebPageProxy::rectForCharacterRangeCallback);
        return;
    }
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::AttributedStringForCharacterRangeCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::AttributedStringForCharacterRangeCallback>(decoder, this, &WebPageProxy::attributedStringForCharacterRangeCallback);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::FontAtSelectionCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FontAtSelectionCallback>(decoder, this, &WebPageProxy::fontAtSelectionCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::GestureCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GestureCallback>(decoder, this, &WebPageProxy::gestureCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::TouchesCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::TouchesCallback>(decoder, this, &WebPageProxy::touchesCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::AutocorrectionDataCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::AutocorrectionDataCallback>(decoder, this, &WebPageProxy::autocorrectionDataCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::AutocorrectionContextCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::AutocorrectionContextCallback>(decoder, this, &WebPageProxy::autocorrectionContextCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::SelectionContextCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SelectionContextCallback>(decoder, this, &WebPageProxy::selectionContextCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DidReceivePositionInformation::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidReceivePositionInformation>(decoder, this, &WebPageProxy::didReceivePositionInformation);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::SaveImageToLibrary::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SaveImageToLibrary>(decoder, this, &WebPageProxy::saveImageToLibrary);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DidUpdateBlockSelectionWithTouch::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidUpdateBlockSelectionWithTouch>(decoder, this, &WebPageProxy::didUpdateBlockSelectionWithTouch);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::ShowPlaybackTargetPicker::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowPlaybackTargetPicker>(decoder, this, &WebPageProxy::showPlaybackTargetPicker);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::ZoomToRect::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ZoomToRect>(decoder, this, &WebPageProxy::zoomToRect);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::CommitPotentialTapFailed::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CommitPotentialTapFailed>(decoder, this, &WebPageProxy::commitPotentialTapFailed);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DidNotHandleTapAsClick::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidNotHandleTapAsClick>(decoder, this, &WebPageProxy::didNotHandleTapAsClick);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DisableDoubleTapGesturesDuringTapIfNecessary::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DisableDoubleTapGesturesDuringTapIfNecessary>(decoder, this, &WebPageProxy::disableDoubleTapGesturesDuringTapIfNecessary);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DrawToPDFCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DrawToPDFCallback>(decoder, this, &WebPageProxy::drawToPDFCallback);
        return;
    }
#endif
#if ENABLE(DATA_DETECTION)
    if (decoder.messageName() == Messages::WebPageProxy::SetDataDetectionResult::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetDataDetectionResult>(decoder, this, &WebPageProxy::setDataDetectionResult);
        return;
    }
#endif
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPageProxy::PrintFinishedCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PrintFinishedCallback>(decoder, this, &WebPageProxy::printFinishedCallback);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::MachSendRightCallback::name()) {
        IPC::handleMessage<Messages::WebPageProxy::MachSendRightCallback>(decoder, this, &WebPageProxy::machSendRightCallback);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::PageScaleFactorDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PageScaleFactorDidChange>(decoder, this, &WebPageProxy::pageScaleFactorDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::PluginScaleFactorDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PluginScaleFactorDidChange>(decoder, this, &WebPageProxy::pluginScaleFactorDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::PluginZoomFactorDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PluginZoomFactorDidChange>(decoder, this, &WebPageProxy::pluginZoomFactorDidChange);
        return;
    }
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPageProxy::BindAccessibilityTree::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BindAccessibilityTree>(decoder, this, &WebPageProxy::bindAccessibilityTree);
        return;
    }
#endif
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPageProxy::SetInputMethodState::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetInputMethodState>(decoder, this, &WebPageProxy::setInputMethodState);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::BackForwardAddItem::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BackForwardAddItem>(decoder, this, &WebPageProxy::backForwardAddItem);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::BackForwardClear::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BackForwardClear>(decoder, this, &WebPageProxy::backForwardClear);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::WillGoToBackForwardListItem::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WillGoToBackForwardListItem>(decoder, this, &WebPageProxy::willGoToBackForwardListItem);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RegisterEditCommandForUndo::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RegisterEditCommandForUndo>(decoder, this, &WebPageProxy::registerEditCommandForUndo);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ClearAllEditCommands::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ClearAllEditCommands>(decoder, this, &WebPageProxy::clearAllEditCommands);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RegisterInsertionUndoGrouping::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RegisterInsertionUndoGrouping>(decoder, this, &WebPageProxy::registerInsertionUndoGrouping);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::LogSampledDiagnosticMessage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::LogSampledDiagnosticMessage>(decoder, this, &WebPageProxy::logSampledDiagnosticMessage);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::LogSampledDiagnosticMessageWithResult::name()) {
        IPC::handleMessage<Messages::WebPageProxy::LogSampledDiagnosticMessageWithResult>(decoder, this, &WebPageProxy::logSampledDiagnosticMessageWithResult);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::LogSampledDiagnosticMessageWithValue::name()) {
        IPC::handleMessage<Messages::WebPageProxy::LogSampledDiagnosticMessageWithValue>(decoder, this, &WebPageProxy::logSampledDiagnosticMessageWithValue);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::EditorStateChanged::name()) {
        IPC::handleMessage<Messages::WebPageProxy::EditorStateChanged>(decoder, this, &WebPageProxy::editorStateChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::CompositionWasCanceled::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CompositionWasCanceled>(decoder, this, &WebPageProxy::compositionWasCanceled);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetHasHadSelectionChangesFromUserInteraction::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetHasHadSelectionChangesFromUserInteraction>(decoder, this, &WebPageProxy::setHasHadSelectionChangesFromUserInteraction);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidCountStringMatches::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidCountStringMatches>(decoder, this, &WebPageProxy::didCountStringMatches);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SetTextIndicator::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetTextIndicator>(decoder, this, &WebPageProxy::setTextIndicator);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ClearTextIndicator::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ClearTextIndicator>(decoder, this, &WebPageProxy::clearTextIndicator);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFindString::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFindString>(decoder, this, &WebPageProxy::didFindString);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFailToFindString::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFailToFindString>(decoder, this, &WebPageProxy::didFailToFindString);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidFindStringMatches::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFindStringMatches>(decoder, this, &WebPageProxy::didFindStringMatches);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidGetImageForFindMatch::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidGetImageForFindMatch>(decoder, this, &WebPageProxy::didGetImageForFindMatch);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ShowPopupMenu::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowPopupMenu>(decoder, this, &WebPageProxy::showPopupMenu);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::HidePopupMenu::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HidePopupMenu>(decoder, this, &WebPageProxy::hidePopupMenu);
        return;
    }
#if ENABLE(CONTEXT_MENUS)
    if (decoder.messageName() == Messages::WebPageProxy::ShowContextMenu::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowContextMenu>(decoder, this, &WebPageProxy::showContextMenu);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::DidReceiveAuthenticationChallenge::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidReceiveAuthenticationChallenge>(decoder, this, &WebPageProxy::didReceiveAuthenticationChallenge);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RequestGeolocationPermissionForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RequestGeolocationPermissionForFrame>(decoder, this, &WebPageProxy::requestGeolocationPermissionForFrame);
        return;
    }
#if ENABLE(MEDIA_STREAM)
    if (decoder.messageName() == Messages::WebPageProxy::RequestUserMediaPermissionForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RequestUserMediaPermissionForFrame>(decoder, this, &WebPageProxy::requestUserMediaPermissionForFrame);
        return;
    }
#endif
#if ENABLE(MEDIA_STREAM)
    if (decoder.messageName() == Messages::WebPageProxy::CheckUserMediaPermissionForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CheckUserMediaPermissionForFrame>(decoder, this, &WebPageProxy::checkUserMediaPermissionForFrame);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::RequestNotificationPermission::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RequestNotificationPermission>(decoder, this, &WebPageProxy::requestNotificationPermission);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ShowNotification::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowNotification>(decoder, this, &WebPageProxy::showNotification);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::CancelNotification::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CancelNotification>(decoder, this, &WebPageProxy::cancelNotification);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ClearNotifications::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ClearNotifications>(decoder, this, &WebPageProxy::clearNotifications);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidDestroyNotification::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidDestroyNotification>(decoder, this, &WebPageProxy::didDestroyNotification);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::UpdateSpellingUIWithMisspelledWord::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UpdateSpellingUIWithMisspelledWord>(decoder, this, &WebPageProxy::updateSpellingUIWithMisspelledWord);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::UpdateSpellingUIWithGrammarString::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UpdateSpellingUIWithGrammarString>(decoder, this, &WebPageProxy::updateSpellingUIWithGrammarString);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::LearnWord::name()) {
        IPC::handleMessage<Messages::WebPageProxy::LearnWord>(decoder, this, &WebPageProxy::learnWord);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::IgnoreWord::name()) {
        IPC::handleMessage<Messages::WebPageProxy::IgnoreWord>(decoder, this, &WebPageProxy::ignoreWord);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RequestCheckingOfString::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RequestCheckingOfString>(decoder, this, &WebPageProxy::requestCheckingOfString);
        return;
    }
#if ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPageProxy::DidPerformDragControllerAction::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidPerformDragControllerAction>(decoder, this, &WebPageProxy::didPerformDragControllerAction);
        return;
    }
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPageProxy::SetDragImage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetDragImage>(decoder, this, &WebPageProxy::setDragImage);
        return;
    }
#endif
#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPageProxy::SetPromisedDataForImage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetPromisedDataForImage>(decoder, this, &WebPageProxy::setPromisedDataForImage);
        return;
    }
#endif
#if ((PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)) && ENABLE(ATTACHMENT_ELEMENT))
    if (decoder.messageName() == Messages::WebPageProxy::SetPromisedDataForAttachment::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetPromisedDataForAttachment>(decoder, this, &WebPageProxy::setPromisedDataForAttachment);
        return;
    }
#endif
#if PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPageProxy::StartDrag::name()) {
        IPC::handleMessage<Messages::WebPageProxy::StartDrag>(decoder, this, &WebPageProxy::startDrag);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::DidPerformDictionaryLookup::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidPerformDictionaryLookup>(decoder, this, &WebPageProxy::didPerformDictionaryLookup);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::RegisterWebProcessAccessibilityToken::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RegisterWebProcessAccessibilityToken>(decoder, this, &WebPageProxy::registerWebProcessAccessibilityToken);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::PluginFocusOrWindowFocusChanged::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PluginFocusOrWindowFocusChanged>(decoder, this, &WebPageProxy::pluginFocusOrWindowFocusChanged);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::SetPluginComplexTextInputState::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetPluginComplexTextInputState>(decoder, this, &WebPageProxy::setPluginComplexTextInputState);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::Speak::name()) {
        IPC::handleMessage<Messages::WebPageProxy::Speak>(decoder, this, &WebPageProxy::speak);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::StopSpeaking::name()) {
        IPC::handleMessage<Messages::WebPageProxy::StopSpeaking>(decoder, this, &WebPageProxy::stopSpeaking);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::MakeFirstResponder::name()) {
        IPC::handleMessage<Messages::WebPageProxy::MakeFirstResponder>(decoder, this, &WebPageProxy::makeFirstResponder);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::SearchWithSpotlight::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SearchWithSpotlight>(decoder, this, &WebPageProxy::searchWithSpotlight);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::SearchTheWeb::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SearchTheWeb>(decoder, this, &WebPageProxy::searchTheWeb);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::ShowCorrectionPanel::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowCorrectionPanel>(decoder, this, &WebPageProxy::showCorrectionPanel);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::DismissCorrectionPanel::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DismissCorrectionPanel>(decoder, this, &WebPageProxy::dismissCorrectionPanel);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::RecordAutocorrectionResponse::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RecordAutocorrectionResponse>(decoder, this, &WebPageProxy::recordAutocorrectionResponse);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::SetEditableElementIsFocused::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetEditableElementIsFocused>(decoder, this, &WebPageProxy::setEditableElementIsFocused);
        return;
    }
#endif
#if USE(DICTATION_ALTERNATIVES)
    if (decoder.messageName() == Messages::WebPageProxy::ShowDictationAlternativeUI::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowDictationAlternativeUI>(decoder, this, &WebPageProxy::showDictationAlternativeUI);
        return;
    }
#endif
#if USE(DICTATION_ALTERNATIVES)
    if (decoder.messageName() == Messages::WebPageProxy::RemoveDictationAlternatives::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RemoveDictationAlternatives>(decoder, this, &WebPageProxy::removeDictationAlternatives);
        return;
    }
#endif
#if PLUGIN_ARCHITECTURE(X11)
    if (decoder.messageName() == Messages::WebPageProxy::WindowedPluginGeometryDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WindowedPluginGeometryDidChange>(decoder, this, &WebPageProxy::windowedPluginGeometryDidChange);
        return;
    }
#endif
#if PLUGIN_ARCHITECTURE(X11)
    if (decoder.messageName() == Messages::WebPageProxy::WindowedPluginVisibilityDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WindowedPluginVisibilityDidChange>(decoder, this, &WebPageProxy::windowedPluginVisibilityDidChange);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DynamicViewportUpdateChangedTarget::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DynamicViewportUpdateChangedTarget>(decoder, this, &WebPageProxy::dynamicViewportUpdateChangedTarget);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::CouldNotRestorePageState::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CouldNotRestorePageState>(decoder, this, &WebPageProxy::couldNotRestorePageState);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::RestorePageState::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RestorePageState>(decoder, this, &WebPageProxy::restorePageState);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::RestorePageCenterAndScale::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RestorePageCenterAndScale>(decoder, this, &WebPageProxy::restorePageCenterAndScale);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DidGetTapHighlightGeometries::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidGetTapHighlightGeometries>(decoder, this, &WebPageProxy::didGetTapHighlightGeometries);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::StartAssistingNode::name()) {
        IPC::handleMessage<Messages::WebPageProxy::StartAssistingNode>(decoder, this, &WebPageProxy::startAssistingNode);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::StopAssistingNode::name()) {
        IPC::handleMessage<Messages::WebPageProxy::StopAssistingNode>(decoder, this, &WebPageProxy::stopAssistingNode);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::OverflowScrollWillStartScroll::name()) {
        IPC::handleMessage<Messages::WebPageProxy::OverflowScrollWillStartScroll>(decoder, this, &WebPageProxy::overflowScrollWillStartScroll);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::OverflowScrollDidEndScroll::name()) {
        IPC::handleMessage<Messages::WebPageProxy::OverflowScrollDidEndScroll>(decoder, this, &WebPageProxy::overflowScrollDidEndScroll);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::ShowInspectorHighlight::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowInspectorHighlight>(decoder, this, &WebPageProxy::showInspectorHighlight);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::HideInspectorHighlight::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HideInspectorHighlight>(decoder, this, &WebPageProxy::hideInspectorHighlight);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::ShowInspectorIndication::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowInspectorIndication>(decoder, this, &WebPageProxy::showInspectorIndication);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::HideInspectorIndication::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HideInspectorIndication>(decoder, this, &WebPageProxy::hideInspectorIndication);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::EnableInspectorNodeSearch::name()) {
        IPC::handleMessage<Messages::WebPageProxy::EnableInspectorNodeSearch>(decoder, this, &WebPageProxy::enableInspectorNodeSearch);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::DisableInspectorNodeSearch::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DisableInspectorNodeSearch>(decoder, this, &WebPageProxy::disableInspectorNodeSearch);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::SaveRecentSearches::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SaveRecentSearches>(decoder, this, &WebPageProxy::saveRecentSearches);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SavePDFToFileInDownloadsFolder::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SavePDFToFileInDownloadsFolder>(decoder, this, &WebPageProxy::savePDFToFileInDownloadsFolder);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::SavePDFToTemporaryFolderAndOpenWithNativeApplication::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SavePDFToTemporaryFolderAndOpenWithNativeApplication>(decoder, this, &WebPageProxy::savePDFToTemporaryFolderAndOpenWithNativeApplication);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::OpenPDFFromTemporaryFolderWithNativeApplication::name()) {
        IPC::handleMessage<Messages::WebPageProxy::OpenPDFFromTemporaryFolderWithNativeApplication>(decoder, this, &WebPageProxy::openPDFFromTemporaryFolderWithNativeApplication);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::DidUpdateViewState::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidUpdateViewState>(decoder, this, &WebPageProxy::didUpdateViewState);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DidSaveToPageCache::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidSaveToPageCache>(decoder, this, &WebPageProxy::didSaveToPageCache);
        return;
    }
#if (ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(MAC))
    if (decoder.messageName() == Messages::WebPageProxy::ShowTelephoneNumberMenu::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowTelephoneNumberMenu>(decoder, this, &WebPageProxy::showTelephoneNumberMenu);
        return;
    }
#endif
#if USE(QUICK_LOOK)
    if (decoder.messageName() == Messages::WebPageProxy::DidStartLoadForQuickLookDocumentInMainFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidStartLoadForQuickLookDocumentInMainFrame>(decoder, this, &WebPageProxy::didStartLoadForQuickLookDocumentInMainFrame);
        return;
    }
#endif
#if USE(QUICK_LOOK)
    if (decoder.messageName() == Messages::WebPageProxy::DidFinishLoadForQuickLookDocumentInMainFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidFinishLoadForQuickLookDocumentInMainFrame>(decoder, this, &WebPageProxy::didFinishLoadForQuickLookDocumentInMainFrame);
        return;
    }
#endif
#if ENABLE(CONTENT_FILTERING)
    if (decoder.messageName() == Messages::WebPageProxy::ContentFilterDidBlockLoadForFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ContentFilterDidBlockLoadForFrame>(decoder, this, &WebPageProxy::contentFilterDidBlockLoadForFrame);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::IsPlayingMediaDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::IsPlayingMediaDidChange>(decoder, this, &WebPageProxy::isPlayingMediaDidChange);
        return;
    }
#if ENABLE(MEDIA_SESSION)
    if (decoder.messageName() == Messages::WebPageProxy::HasMediaSessionWithActiveMediaElementsDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HasMediaSessionWithActiveMediaElementsDidChange>(decoder, this, &WebPageProxy::hasMediaSessionWithActiveMediaElementsDidChange);
        return;
    }
#endif
#if ENABLE(MEDIA_SESSION)
    if (decoder.messageName() == Messages::WebPageProxy::MediaSessionMetadataDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::MediaSessionMetadataDidChange>(decoder, this, &WebPageProxy::mediaSessionMetadataDidChange);
        return;
    }
#endif
#if ENABLE(MEDIA_SESSION)
    if (decoder.messageName() == Messages::WebPageProxy::FocusedContentMediaElementDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FocusedContentMediaElementDidChange>(decoder, this, &WebPageProxy::focusedContentMediaElementDidChange);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::DidPerformImmediateActionHitTest::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidPerformImmediateActionHitTest>(decoder, this, &WebPageProxy::didPerformImmediateActionHitTest);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::HandleMessage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HandleMessage>(connection, decoder, this, &WebPageProxy::handleMessage);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::HandleAutoFillButtonClick::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HandleAutoFillButtonClick>(decoder, this, &WebPageProxy::handleAutoFillButtonClick);
        return;
    }
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::AddPlaybackTargetPickerClient::name()) {
        IPC::handleMessage<Messages::WebPageProxy::AddPlaybackTargetPickerClient>(decoder, this, &WebPageProxy::addPlaybackTargetPickerClient);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::RemovePlaybackTargetPickerClient::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RemovePlaybackTargetPickerClient>(decoder, this, &WebPageProxy::removePlaybackTargetPickerClient);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::ShowPlaybackTargetPicker::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ShowPlaybackTargetPicker>(decoder, this, &WebPageProxy::showPlaybackTargetPicker);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::PlaybackTargetPickerClientStateDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PlaybackTargetPickerClientStateDidChange>(decoder, this, &WebPageProxy::playbackTargetPickerClientStateDidChange);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::SetMockMediaPlaybackTargetPickerEnabled::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetMockMediaPlaybackTargetPickerEnabled>(decoder, this, &WebPageProxy::setMockMediaPlaybackTargetPickerEnabled);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::SetMockMediaPlaybackTargetPickerState::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SetMockMediaPlaybackTargetPickerState>(decoder, this, &WebPageProxy::setMockMediaPlaybackTargetPickerState);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::ImageOrMediaDocumentSizeChanged::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ImageOrMediaDocumentSizeChanged>(decoder, this, &WebPageProxy::imageOrMediaDocumentSizeChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::UseFixedLayoutDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UseFixedLayoutDidChange>(decoder, this, &WebPageProxy::useFixedLayoutDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::FixedLayoutSizeDidChange::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FixedLayoutSizeDidChange>(decoder, this, &WebPageProxy::fixedLayoutSizeDidChange);
        return;
    }
#if ENABLE(VIDEO) && USE(GSTREAMER)
    if (decoder.messageName() == Messages::WebPageProxy::RequestInstallMissingMediaPlugins::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RequestInstallMissingMediaPlugins>(decoder, this, &WebPageProxy::requestInstallMissingMediaPlugins);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::DidRestoreScrollPosition::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidRestoreScrollPosition>(decoder, this, &WebPageProxy::didRestoreScrollPosition);
        return;
    }
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::DidHandleAcceptedCandidate::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DidHandleAcceptedCandidate>(decoder, this, &WebPageProxy::didHandleAcceptedCandidate);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebPageProxy::didReceiveSyncMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::WebPageProxy::CreateNewPage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CreateNewPage>(decoder, *replyEncoder, this, &WebPageProxy::createNewPage);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RunJavaScriptAlert::name()) {
        IPC::handleMessageDelayed<Messages::WebPageProxy::RunJavaScriptAlert>(connection, decoder, replyEncoder, this, &WebPageProxy::runJavaScriptAlert);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RunJavaScriptConfirm::name()) {
        IPC::handleMessageDelayed<Messages::WebPageProxy::RunJavaScriptConfirm>(connection, decoder, replyEncoder, this, &WebPageProxy::runJavaScriptConfirm);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RunJavaScriptPrompt::name()) {
        IPC::handleMessageDelayed<Messages::WebPageProxy::RunJavaScriptPrompt>(connection, decoder, replyEncoder, this, &WebPageProxy::runJavaScriptPrompt);
        return;
    }
#if ENABLE(WEBGL)
    if (decoder.messageName() == Messages::WebPageProxy::WebGLPolicyForURL::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WebGLPolicyForURL>(decoder, *replyEncoder, this, &WebPageProxy::webGLPolicyForURL);
        return;
    }
#endif
#if ENABLE(WEBGL)
    if (decoder.messageName() == Messages::WebPageProxy::ResolveWebGLPolicyForURL::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ResolveWebGLPolicyForURL>(decoder, *replyEncoder, this, &WebPageProxy::resolveWebGLPolicyForURL);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::GetToolbarsAreVisible::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetToolbarsAreVisible>(decoder, *replyEncoder, this, &WebPageProxy::getToolbarsAreVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::GetMenuBarIsVisible::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetMenuBarIsVisible>(decoder, *replyEncoder, this, &WebPageProxy::getMenuBarIsVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::GetStatusBarIsVisible::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetStatusBarIsVisible>(decoder, *replyEncoder, this, &WebPageProxy::getStatusBarIsVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::GetIsResizable::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetIsResizable>(decoder, *replyEncoder, this, &WebPageProxy::getIsResizable);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::GetWindowFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetWindowFrame>(decoder, *replyEncoder, this, &WebPageProxy::getWindowFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ScreenToRootView::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ScreenToRootView>(decoder, *replyEncoder, this, &WebPageProxy::screenToRootView);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::RootViewToScreen::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RootViewToScreen>(decoder, *replyEncoder, this, &WebPageProxy::rootViewToScreen);
        return;
    }
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::AccessibilityScreenToRootView::name()) {
        IPC::handleMessage<Messages::WebPageProxy::AccessibilityScreenToRootView>(decoder, *replyEncoder, this, &WebPageProxy::accessibilityScreenToRootView);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::RootViewToAccessibilityScreen::name()) {
        IPC::handleMessage<Messages::WebPageProxy::RootViewToAccessibilityScreen>(decoder, *replyEncoder, this, &WebPageProxy::rootViewToAccessibilityScreen);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::RunBeforeUnloadConfirmPanel::name()) {
        IPC::handleMessageDelayed<Messages::WebPageProxy::RunBeforeUnloadConfirmPanel>(connection, decoder, replyEncoder, this, &WebPageProxy::runBeforeUnloadConfirmPanel);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::PrintFrame::name()) {
        IPC::handleMessage<Messages::WebPageProxy::PrintFrame>(decoder, *replyEncoder, this, &WebPageProxy::printFrame);
        return;
    }
#if PLATFORM(EFL)
    if (decoder.messageName() == Messages::WebPageProxy::HandleInputMethodKeydown::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HandleInputMethodKeydown>(decoder, *replyEncoder, this, &WebPageProxy::handleInputMethodKeydown);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::DecidePolicyForResponseSync::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DecidePolicyForResponseSync>(decoder, *replyEncoder, this, &WebPageProxy::decidePolicyForResponseSync);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::DecidePolicyForNavigationAction::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DecidePolicyForNavigationAction>(decoder, *replyEncoder, this, &WebPageProxy::decidePolicyForNavigationAction);
        return;
    }
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPageProxy::InterpretKeyEvent::name()) {
        IPC::handleMessage<Messages::WebPageProxy::InterpretKeyEvent>(decoder, *replyEncoder, this, &WebPageProxy::interpretKeyEvent);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::BackForwardGoToItem::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BackForwardGoToItem>(decoder, *replyEncoder, this, &WebPageProxy::backForwardGoToItem);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::BackForwardItemAtIndex::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BackForwardItemAtIndex>(decoder, *replyEncoder, this, &WebPageProxy::backForwardItemAtIndex);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::BackForwardBackListCount::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BackForwardBackListCount>(decoder, *replyEncoder, this, &WebPageProxy::backForwardBackListCount);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::BackForwardForwardListCount::name()) {
        IPC::handleMessage<Messages::WebPageProxy::BackForwardForwardListCount>(decoder, *replyEncoder, this, &WebPageProxy::backForwardForwardListCount);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::CanUndoRedo::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CanUndoRedo>(decoder, *replyEncoder, this, &WebPageProxy::canUndoRedo);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ExecuteUndoRedo::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ExecuteUndoRedo>(decoder, *replyEncoder, this, &WebPageProxy::executeUndoRedo);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ExceededDatabaseQuota::name()) {
        IPC::handleMessageDelayed<Messages::WebPageProxy::ExceededDatabaseQuota>(connection, decoder, replyEncoder, this, &WebPageProxy::exceededDatabaseQuota);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::ReachedApplicationCacheOriginQuota::name()) {
        IPC::handleMessageDelayed<Messages::WebPageProxy::ReachedApplicationCacheOriginQuota>(connection, decoder, replyEncoder, this, &WebPageProxy::reachedApplicationCacheOriginQuota);
        return;
    }
#if USE(UNIFIED_TEXT_CHECKING)
    if (decoder.messageName() == Messages::WebPageProxy::CheckTextOfParagraph::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CheckTextOfParagraph>(decoder, *replyEncoder, this, &WebPageProxy::checkTextOfParagraph);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::CheckSpellingOfString::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CheckSpellingOfString>(decoder, *replyEncoder, this, &WebPageProxy::checkSpellingOfString);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::CheckGrammarOfString::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CheckGrammarOfString>(decoder, *replyEncoder, this, &WebPageProxy::checkGrammarOfString);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::SpellingUIIsShowing::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SpellingUIIsShowing>(decoder, *replyEncoder, this, &WebPageProxy::spellingUIIsShowing);
        return;
    }
    if (decoder.messageName() == Messages::WebPageProxy::GetGuessesForWord::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetGuessesForWord>(decoder, *replyEncoder, this, &WebPageProxy::getGuessesForWord);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::ExecuteSavedCommandBySelector::name()) {
        IPC::handleMessage<Messages::WebPageProxy::ExecuteSavedCommandBySelector>(decoder, *replyEncoder, this, &WebPageProxy::executeSavedCommandBySelector);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPageProxy::GetIsSpeaking::name()) {
        IPC::handleMessage<Messages::WebPageProxy::GetIsSpeaking>(decoder, *replyEncoder, this, &WebPageProxy::getIsSpeaking);
        return;
    }
#endif
#if USE(APPKIT)
    if (decoder.messageName() == Messages::WebPageProxy::SubstitutionsPanelIsShowing::name()) {
        IPC::handleMessage<Messages::WebPageProxy::SubstitutionsPanelIsShowing>(decoder, *replyEncoder, this, &WebPageProxy::substitutionsPanelIsShowing);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPageProxy::DismissCorrectionPanelSoon::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DismissCorrectionPanelSoon>(decoder, *replyEncoder, this, &WebPageProxy::dismissCorrectionPanelSoon);
        return;
    }
#endif
#if USE(DICTATION_ALTERNATIVES)
    if (decoder.messageName() == Messages::WebPageProxy::DictationAlternatives::name()) {
        IPC::handleMessage<Messages::WebPageProxy::DictationAlternatives>(decoder, *replyEncoder, this, &WebPageProxy::dictationAlternatives);
        return;
    }
#endif
#if PLUGIN_ARCHITECTURE(X11)
    if (decoder.messageName() == Messages::WebPageProxy::CreatePluginContainer::name()) {
        IPC::handleMessage<Messages::WebPageProxy::CreatePluginContainer>(decoder, *replyEncoder, this, &WebPageProxy::createPluginContainer);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::LoadRecentSearches::name()) {
        IPC::handleMessage<Messages::WebPageProxy::LoadRecentSearches>(decoder, *replyEncoder, this, &WebPageProxy::loadRecentSearches);
        return;
    }
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (decoder.messageName() == Messages::WebPageProxy::FindPlugin::name()) {
        IPC::handleMessage<Messages::WebPageProxy::FindPlugin>(decoder, *replyEncoder, this, &WebPageProxy::findPlugin);
        return;
    }
#endif
#if ENABLE(SUBTLE_CRYPTO)
    if (decoder.messageName() == Messages::WebPageProxy::WrapCryptoKey::name()) {
        IPC::handleMessage<Messages::WebPageProxy::WrapCryptoKey>(decoder, *replyEncoder, this, &WebPageProxy::wrapCryptoKey);
        return;
    }
#endif
#if ENABLE(SUBTLE_CRYPTO)
    if (decoder.messageName() == Messages::WebPageProxy::UnwrapCryptoKey::name()) {
        IPC::handleMessage<Messages::WebPageProxy::UnwrapCryptoKey>(decoder, *replyEncoder, this, &WebPageProxy::unwrapCryptoKey);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPageProxy::HandleSynchronousMessage::name()) {
        IPC::handleMessage<Messages::WebPageProxy::HandleSynchronousMessage>(connection, decoder, *replyEncoder, this, &WebPageProxy::handleSynchronousMessage);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
