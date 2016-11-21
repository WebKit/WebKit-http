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

#include "WebPage.h"

#include "ArgumentCoders.h"
#include "DataReference.h"
#include "DownloadID.h"
#if PLATFORM(MAC) || PLATFORM(COCOA)
#include "EditingRange.h"
#endif
#include "HandleMessage.h"
#if PLATFORM(IOS)
#include "InteractionInformationAtPosition.h"
#endif
#include "MessageDecoder.h"
#include "PrintInfo.h"
#include "SandboxExtension.h"
#include "SessionState.h"
#if PLATFORM(COCOA)
#include "SharedMemory.h"
#endif
#include "UserData.h"
#if ENABLE(CONTEXT_MENUS)
#include "WebContextMenuItemData.h"
#endif
#include "WebCoreArgumentCoders.h"
#include "WebEvent.h"
#include "WebPageMessages.h"
#include "WebPreferencesStore.h"
#include <WebCore/Color.h>
#if PLATFORM(MAC)
#include <WebCore/DictationAlternative.h>
#endif
#if PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
#include <WebCore/DragData.h>
#endif
#if PLATFORM(COCOA) || PLATFORM(EFL) || PLATFORM(GTK)
#include <WebCore/Editor.h>
#endif
#if PLATFORM(MAC) || PLATFORM(COCOA) || PLATFORM(IOS)
#include <WebCore/FloatPoint.h>
#endif
#if PLATFORM(COCOA) || PLATFORM(IOS)
#include <WebCore/FloatRect.h>
#endif
#if PLATFORM(IOS)
#include <WebCore/FloatSize.h>
#endif
#include <WebCore/IntPoint.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
#include <WebCore/MediaPlaybackTargetContext.h>
#endif
#if PLATFORM(MAC)
#include <WebCore/PageOverlay.h>
#endif
#include <WebCore/ResourceRequest.h>
#include <WebCore/SessionID.h>
#include <WebCore/TextCheckerClient.h>
#include <wtf/Optional.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace Messages {

namespace WebPage {

#if (PLATFORM(COCOA) && PLATFORM(IOS))

ComputePagesForPrintingAndDrawToPDF::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::MessageEncoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

ComputePagesForPrintingAndDrawToPDF::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool ComputePagesForPrintingAndDrawToPDF::DelayedReply::send(uint32_t pageCount)
{
    ASSERT(m_encoder);
    *m_encoder << pageCount;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

#endif

} // namespace WebPage

} // namespace Messages

namespace WebKit {

void WebPage::didReceiveWebPageMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebPage::SetInitialFocus::name()) {
        IPC::handleMessage<Messages::WebPage::SetInitialFocus>(decoder, this, &WebPage::setInitialFocus);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetViewState::name()) {
        IPC::handleMessage<Messages::WebPage::SetViewState>(decoder, this, &WebPage::setViewState);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetLayerHostingMode::name()) {
        IPC::handleMessage<Messages::WebPage::SetLayerHostingMode>(decoder, this, &WebPage::setLayerHostingMode);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetSessionID::name()) {
        IPC::handleMessage<Messages::WebPage::SetSessionID>(decoder, this, &WebPage::setSessionID);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetDrawsBackground::name()) {
        IPC::handleMessage<Messages::WebPage::SetDrawsBackground>(decoder, this, &WebPage::setDrawsBackground);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetTopContentInset::name()) {
        IPC::handleMessage<Messages::WebPage::SetTopContentInset>(decoder, this, &WebPage::setTopContentInset);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetUnderlayColor::name()) {
        IPC::handleMessage<Messages::WebPage::SetUnderlayColor>(decoder, this, &WebPage::setUnderlayColor);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ViewWillStartLiveResize::name()) {
        IPC::handleMessage<Messages::WebPage::ViewWillStartLiveResize>(decoder, this, &WebPage::viewWillStartLiveResize);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ViewWillEndLiveResize::name()) {
        IPC::handleMessage<Messages::WebPage::ViewWillEndLiveResize>(decoder, this, &WebPage::viewWillEndLiveResize);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::KeyEvent::name()) {
        IPC::handleMessage<Messages::WebPage::KeyEvent>(decoder, this, &WebPage::keyEvent);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::MouseEvent::name()) {
        IPC::handleMessage<Messages::WebPage::MouseEvent>(decoder, this, &WebPage::mouseEvent);
        return;
    }
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetViewportConfigurationMinimumLayoutSize::name()) {
        IPC::handleMessage<Messages::WebPage::SetViewportConfigurationMinimumLayoutSize>(decoder, this, &WebPage::setViewportConfigurationMinimumLayoutSize);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetMaximumUnobscuredSize::name()) {
        IPC::handleMessage<Messages::WebPage::SetMaximumUnobscuredSize>(decoder, this, &WebPage::setMaximumUnobscuredSize);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetDeviceOrientation::name()) {
        IPC::handleMessage<Messages::WebPage::SetDeviceOrientation>(decoder, this, &WebPage::setDeviceOrientation);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::DynamicViewportSizeUpdate::name()) {
        IPC::handleMessage<Messages::WebPage::DynamicViewportSizeUpdate>(decoder, this, &WebPage::dynamicViewportSizeUpdate);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::HandleTap::name()) {
        IPC::handleMessage<Messages::WebPage::HandleTap>(decoder, this, &WebPage::handleTap);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::PotentialTapAtPosition::name()) {
        IPC::handleMessage<Messages::WebPage::PotentialTapAtPosition>(decoder, this, &WebPage::potentialTapAtPosition);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::CommitPotentialTap::name()) {
        IPC::handleMessage<Messages::WebPage::CommitPotentialTap>(decoder, this, &WebPage::commitPotentialTap);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::CancelPotentialTap::name()) {
        IPC::handleMessage<Messages::WebPage::CancelPotentialTap>(decoder, this, &WebPage::cancelPotentialTap);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::TapHighlightAtPosition::name()) {
        IPC::handleMessage<Messages::WebPage::TapHighlightAtPosition>(decoder, this, &WebPage::tapHighlightAtPosition);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::InspectorNodeSearchMovedToPosition::name()) {
        IPC::handleMessage<Messages::WebPage::InspectorNodeSearchMovedToPosition>(decoder, this, &WebPage::inspectorNodeSearchMovedToPosition);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::InspectorNodeSearchEndedAtPosition::name()) {
        IPC::handleMessage<Messages::WebPage::InspectorNodeSearchEndedAtPosition>(decoder, this, &WebPage::inspectorNodeSearchEndedAtPosition);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::BlurAssistedNode::name()) {
        IPC::handleMessage<Messages::WebPage::BlurAssistedNode>(decoder, this, &WebPage::blurAssistedNode);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SelectWithGesture::name()) {
        IPC::handleMessage<Messages::WebPage::SelectWithGesture>(decoder, this, &WebPage::selectWithGesture);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::UpdateSelectionWithTouches::name()) {
        IPC::handleMessage<Messages::WebPage::UpdateSelectionWithTouches>(decoder, this, &WebPage::updateSelectionWithTouches);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::UpdateBlockSelectionWithTouch::name()) {
        IPC::handleMessage<Messages::WebPage::UpdateBlockSelectionWithTouch>(decoder, this, &WebPage::updateBlockSelectionWithTouch);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SelectWithTwoTouches::name()) {
        IPC::handleMessage<Messages::WebPage::SelectWithTwoTouches>(decoder, this, &WebPage::selectWithTwoTouches);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ExtendSelection::name()) {
        IPC::handleMessage<Messages::WebPage::ExtendSelection>(decoder, this, &WebPage::extendSelection);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SelectWordBackward::name()) {
        IPC::handleMessage<Messages::WebPage::SelectWordBackward>(decoder, this, &WebPage::selectWordBackward);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::MoveSelectionByOffset::name()) {
        IPC::handleMessage<Messages::WebPage::MoveSelectionByOffset>(decoder, this, &WebPage::moveSelectionByOffset);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SelectTextWithGranularityAtPoint::name()) {
        IPC::handleMessage<Messages::WebPage::SelectTextWithGranularityAtPoint>(decoder, this, &WebPage::selectTextWithGranularityAtPoint);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SelectPositionAtBoundaryWithDirection::name()) {
        IPC::handleMessage<Messages::WebPage::SelectPositionAtBoundaryWithDirection>(decoder, this, &WebPage::selectPositionAtBoundaryWithDirection);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::MoveSelectionAtBoundaryWithDirection::name()) {
        IPC::handleMessage<Messages::WebPage::MoveSelectionAtBoundaryWithDirection>(decoder, this, &WebPage::moveSelectionAtBoundaryWithDirection);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SelectPositionAtPoint::name()) {
        IPC::handleMessage<Messages::WebPage::SelectPositionAtPoint>(decoder, this, &WebPage::selectPositionAtPoint);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::BeginSelectionInDirection::name()) {
        IPC::handleMessage<Messages::WebPage::BeginSelectionInDirection>(decoder, this, &WebPage::beginSelectionInDirection);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::UpdateSelectionWithExtentPoint::name()) {
        IPC::handleMessage<Messages::WebPage::UpdateSelectionWithExtentPoint>(decoder, this, &WebPage::updateSelectionWithExtentPoint);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::UpdateSelectionWithExtentPointAndBoundary::name()) {
        IPC::handleMessage<Messages::WebPage::UpdateSelectionWithExtentPointAndBoundary>(decoder, this, &WebPage::updateSelectionWithExtentPointAndBoundary);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::RequestDictationContext::name()) {
        IPC::handleMessage<Messages::WebPage::RequestDictationContext>(decoder, this, &WebPage::requestDictationContext);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ReplaceDictatedText::name()) {
        IPC::handleMessage<Messages::WebPage::ReplaceDictatedText>(decoder, this, &WebPage::replaceDictatedText);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ReplaceSelectedText::name()) {
        IPC::handleMessage<Messages::WebPage::ReplaceSelectedText>(decoder, this, &WebPage::replaceSelectedText);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::RequestAutocorrectionData::name()) {
        IPC::handleMessage<Messages::WebPage::RequestAutocorrectionData>(decoder, this, &WebPage::requestAutocorrectionData);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ApplyAutocorrection::name()) {
        IPC::handleMessage<Messages::WebPage::ApplyAutocorrection>(decoder, this, &WebPage::applyAutocorrection);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::RequestAutocorrectionContext::name()) {
        IPC::handleMessage<Messages::WebPage::RequestAutocorrectionContext>(decoder, this, &WebPage::requestAutocorrectionContext);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::RequestPositionInformation::name()) {
        IPC::handleMessage<Messages::WebPage::RequestPositionInformation>(decoder, this, &WebPage::requestPositionInformation);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::StartInteractionWithElementAtPosition::name()) {
        IPC::handleMessage<Messages::WebPage::StartInteractionWithElementAtPosition>(decoder, this, &WebPage::startInteractionWithElementAtPosition);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::StopInteraction::name()) {
        IPC::handleMessage<Messages::WebPage::StopInteraction>(decoder, this, &WebPage::stopInteraction);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::PerformActionOnElement::name()) {
        IPC::handleMessage<Messages::WebPage::PerformActionOnElement>(decoder, this, &WebPage::performActionOnElement);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::FocusNextAssistedNode::name()) {
        IPC::handleMessage<Messages::WebPage::FocusNextAssistedNode>(decoder, this, &WebPage::focusNextAssistedNode);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetAssistedNodeValue::name()) {
        IPC::handleMessage<Messages::WebPage::SetAssistedNodeValue>(decoder, this, &WebPage::setAssistedNodeValue);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetAssistedNodeValueAsNumber::name()) {
        IPC::handleMessage<Messages::WebPage::SetAssistedNodeValueAsNumber>(decoder, this, &WebPage::setAssistedNodeValueAsNumber);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetAssistedNodeSelectedIndex::name()) {
        IPC::handleMessage<Messages::WebPage::SetAssistedNodeSelectedIndex>(decoder, this, &WebPage::setAssistedNodeSelectedIndex);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ApplicationWillResignActive::name()) {
        IPC::handleMessage<Messages::WebPage::ApplicationWillResignActive>(decoder, this, &WebPage::applicationWillResignActive);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ApplicationDidEnterBackground::name()) {
        IPC::handleMessage<Messages::WebPage::ApplicationDidEnterBackground>(decoder, this, &WebPage::applicationDidEnterBackground);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ApplicationWillEnterForeground::name()) {
        IPC::handleMessage<Messages::WebPage::ApplicationWillEnterForeground>(decoder, this, &WebPage::applicationWillEnterForeground);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ApplicationDidBecomeActive::name()) {
        IPC::handleMessage<Messages::WebPage::ApplicationDidBecomeActive>(decoder, this, &WebPage::applicationDidBecomeActive);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ContentSizeCategoryDidChange::name()) {
        IPC::handleMessage<Messages::WebPage::ContentSizeCategoryDidChange>(decoder, this, &WebPage::contentSizeCategoryDidChange);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::ExecuteEditCommandWithCallback::name()) {
        IPC::handleMessage<Messages::WebPage::ExecuteEditCommandWithCallback>(decoder, this, &WebPage::executeEditCommandWithCallback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::GetSelectionContext::name()) {
        IPC::handleMessage<Messages::WebPage::GetSelectionContext>(decoder, this, &WebPage::getSelectionContext);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetAllowsMediaDocumentInlinePlayback::name()) {
        IPC::handleMessage<Messages::WebPage::SetAllowsMediaDocumentInlinePlayback>(decoder, this, &WebPage::setAllowsMediaDocumentInlinePlayback);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::HandleTwoFingerTapAtPoint::name()) {
        IPC::handleMessage<Messages::WebPage::HandleTwoFingerTapAtPoint>(decoder, this, &WebPage::handleTwoFingerTapAtPoint);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::UpdateForceAlwaysUserScalable::name()) {
        IPC::handleMessage<Messages::WebPage::UpdateForceAlwaysUserScalable>(decoder, this, &WebPage::updateForceAlwaysUserScalable);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetControlledByAutomation::name()) {
        IPC::handleMessage<Messages::WebPage::SetControlledByAutomation>(decoder, this, &WebPage::setControlledByAutomation);
        return;
    }
#if ENABLE(REMOTE_INSPECTOR)
    if (decoder.messageName() == Messages::WebPage::SetAllowsRemoteInspection::name()) {
        IPC::handleMessage<Messages::WebPage::SetAllowsRemoteInspection>(decoder, this, &WebPage::setAllowsRemoteInspection);
        return;
    }
#endif
#if ENABLE(REMOTE_INSPECTOR)
    if (decoder.messageName() == Messages::WebPage::SetRemoteInspectionNameOverride::name()) {
        IPC::handleMessage<Messages::WebPage::SetRemoteInspectionNameOverride>(decoder, this, &WebPage::setRemoteInspectionNameOverride);
        return;
    }
#endif
#if !ENABLE(IOS_TOUCH_EVENTS) && ENABLE(TOUCH_EVENTS)
    if (decoder.messageName() == Messages::WebPage::TouchEvent::name()) {
        IPC::handleMessage<Messages::WebPage::TouchEvent>(decoder, this, &WebPage::touchEvent);
        return;
    }
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (decoder.messageName() == Messages::WebPage::DidEndColorPicker::name()) {
        IPC::handleMessage<Messages::WebPage::DidEndColorPicker>(decoder, this, &WebPage::didEndColorPicker);
        return;
    }
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    if (decoder.messageName() == Messages::WebPage::DidChooseColor::name()) {
        IPC::handleMessage<Messages::WebPage::DidChooseColor>(decoder, this, &WebPage::didChooseColor);
        return;
    }
#endif
#if ENABLE(CONTEXT_MENUS)
    if (decoder.messageName() == Messages::WebPage::ContextMenuHidden::name()) {
        IPC::handleMessage<Messages::WebPage::ContextMenuHidden>(decoder, this, &WebPage::contextMenuHidden);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::ScrollBy::name()) {
        IPC::handleMessage<Messages::WebPage::ScrollBy>(decoder, this, &WebPage::scrollBy);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::CenterSelectionInVisibleArea::name()) {
        IPC::handleMessage<Messages::WebPage::CenterSelectionInVisibleArea>(decoder, this, &WebPage::centerSelectionInVisibleArea);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GoBack::name()) {
        IPC::handleMessage<Messages::WebPage::GoBack>(decoder, this, &WebPage::goBack);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GoForward::name()) {
        IPC::handleMessage<Messages::WebPage::GoForward>(decoder, this, &WebPage::goForward);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GoToBackForwardItem::name()) {
        IPC::handleMessage<Messages::WebPage::GoToBackForwardItem>(decoder, this, &WebPage::goToBackForwardItem);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::TryRestoreScrollPosition::name()) {
        IPC::handleMessage<Messages::WebPage::TryRestoreScrollPosition>(decoder, this, &WebPage::tryRestoreScrollPosition);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadURLInFrame::name()) {
        IPC::handleMessage<Messages::WebPage::LoadURLInFrame>(decoder, this, &WebPage::loadURLInFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadRequest::name()) {
        IPC::handleMessage<Messages::WebPage::LoadRequest>(decoder, this, &WebPage::loadRequest);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadData::name()) {
        IPC::handleMessage<Messages::WebPage::LoadData>(decoder, this, &WebPage::loadData);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadHTMLString::name()) {
        IPC::handleMessage<Messages::WebPage::LoadHTMLString>(decoder, this, &WebPage::loadHTMLString);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadAlternateHTMLString::name()) {
        IPC::handleMessage<Messages::WebPage::LoadAlternateHTMLString>(decoder, this, &WebPage::loadAlternateHTMLString);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadPlainTextString::name()) {
        IPC::handleMessage<Messages::WebPage::LoadPlainTextString>(decoder, this, &WebPage::loadPlainTextString);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::LoadWebArchiveData::name()) {
        IPC::handleMessage<Messages::WebPage::LoadWebArchiveData>(decoder, this, &WebPage::loadWebArchiveData);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::NavigateToPDFLinkWithSimulatedClick::name()) {
        IPC::handleMessage<Messages::WebPage::NavigateToPDFLinkWithSimulatedClick>(decoder, this, &WebPage::navigateToPDFLinkWithSimulatedClick);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::Reload::name()) {
        IPC::handleMessage<Messages::WebPage::Reload>(decoder, this, &WebPage::reload);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::StopLoading::name()) {
        IPC::handleMessage<Messages::WebPage::StopLoading>(decoder, this, &WebPage::stopLoading);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::StopLoadingFrame::name()) {
        IPC::handleMessage<Messages::WebPage::StopLoadingFrame>(decoder, this, &WebPage::stopLoadingFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::RestoreSession::name()) {
        IPC::handleMessage<Messages::WebPage::RestoreSession>(decoder, this, &WebPage::restoreSession);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::DidRemoveBackForwardItem::name()) {
        IPC::handleMessage<Messages::WebPage::DidRemoveBackForwardItem>(decoder, this, &WebPage::didRemoveBackForwardItem);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::DidReceivePolicyDecision::name()) {
        IPC::handleMessage<Messages::WebPage::DidReceivePolicyDecision>(decoder, this, &WebPage::didReceivePolicyDecision);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ClearSelection::name()) {
        IPC::handleMessage<Messages::WebPage::ClearSelection>(decoder, this, &WebPage::clearSelection);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::RestoreSelectionInFocusedEditableElement::name()) {
        IPC::handleMessage<Messages::WebPage::RestoreSelectionInFocusedEditableElement>(decoder, this, &WebPage::restoreSelectionInFocusedEditableElement);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetContentsAsString::name()) {
        IPC::handleMessage<Messages::WebPage::GetContentsAsString>(decoder, this, &WebPage::getContentsAsString);
        return;
    }
#if ENABLE(MHTML)
    if (decoder.messageName() == Messages::WebPage::GetContentsAsMHTMLData::name()) {
        IPC::handleMessage<Messages::WebPage::GetContentsAsMHTMLData>(decoder, this, &WebPage::getContentsAsMHTMLData);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::GetMainResourceDataOfFrame::name()) {
        IPC::handleMessage<Messages::WebPage::GetMainResourceDataOfFrame>(decoder, this, &WebPage::getMainResourceDataOfFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetResourceDataFromFrame::name()) {
        IPC::handleMessage<Messages::WebPage::GetResourceDataFromFrame>(decoder, this, &WebPage::getResourceDataFromFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetRenderTreeExternalRepresentation::name()) {
        IPC::handleMessage<Messages::WebPage::GetRenderTreeExternalRepresentation>(decoder, this, &WebPage::getRenderTreeExternalRepresentation);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetSelectionOrContentsAsString::name()) {
        IPC::handleMessage<Messages::WebPage::GetSelectionOrContentsAsString>(decoder, this, &WebPage::getSelectionOrContentsAsString);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetSelectionAsWebArchiveData::name()) {
        IPC::handleMessage<Messages::WebPage::GetSelectionAsWebArchiveData>(decoder, this, &WebPage::getSelectionAsWebArchiveData);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetSourceForFrame::name()) {
        IPC::handleMessage<Messages::WebPage::GetSourceForFrame>(decoder, this, &WebPage::getSourceForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetWebArchiveOfFrame::name()) {
        IPC::handleMessage<Messages::WebPage::GetWebArchiveOfFrame>(decoder, this, &WebPage::getWebArchiveOfFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::RunJavaScriptInMainFrame::name()) {
        IPC::handleMessage<Messages::WebPage::RunJavaScriptInMainFrame>(decoder, this, &WebPage::runJavaScriptInMainFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ForceRepaint::name()) {
        IPC::handleMessage<Messages::WebPage::ForceRepaint>(decoder, this, &WebPage::forceRepaint);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::PerformDictionaryLookupAtLocation::name()) {
        IPC::handleMessage<Messages::WebPage::PerformDictionaryLookupAtLocation>(decoder, this, &WebPage::performDictionaryLookupAtLocation);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::PerformDictionaryLookupOfCurrentSelection::name()) {
        IPC::handleMessage<Messages::WebPage::PerformDictionaryLookupOfCurrentSelection>(decoder, this, &WebPage::performDictionaryLookupOfCurrentSelection);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::SetFont::name()) {
        IPC::handleMessage<Messages::WebPage::SetFont>(decoder, this, &WebPage::setFont);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::PreferencesDidChange::name()) {
        IPC::handleMessage<Messages::WebPage::PreferencesDidChange>(decoder, this, &WebPage::preferencesDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetUserAgent::name()) {
        IPC::handleMessage<Messages::WebPage::SetUserAgent>(decoder, this, &WebPage::setUserAgent);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetCustomTextEncodingName::name()) {
        IPC::handleMessage<Messages::WebPage::SetCustomTextEncodingName>(decoder, this, &WebPage::setCustomTextEncodingName);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SuspendActiveDOMObjectsAndAnimations::name()) {
        IPC::handleMessage<Messages::WebPage::SuspendActiveDOMObjectsAndAnimations>(decoder, this, &WebPage::suspendActiveDOMObjectsAndAnimations);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ResumeActiveDOMObjectsAndAnimations::name()) {
        IPC::handleMessage<Messages::WebPage::ResumeActiveDOMObjectsAndAnimations>(decoder, this, &WebPage::resumeActiveDOMObjectsAndAnimations);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::Close::name()) {
        IPC::handleMessage<Messages::WebPage::Close>(decoder, this, &WebPage::close);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::TryClose::name()) {
        IPC::handleMessage<Messages::WebPage::TryClose>(decoder, this, &WebPage::tryClose);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetEditable::name()) {
        IPC::handleMessage<Messages::WebPage::SetEditable>(decoder, this, &WebPage::setEditable);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ValidateCommand::name()) {
        IPC::handleMessage<Messages::WebPage::ValidateCommand>(decoder, this, &WebPage::validateCommand);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ExecuteEditCommand::name()) {
        IPC::handleMessage<Messages::WebPage::ExecuteEditCommand>(decoder, this, &WebPage::executeEditCommand);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::DidRemoveEditCommand::name()) {
        IPC::handleMessage<Messages::WebPage::DidRemoveEditCommand>(decoder, this, &WebPage::didRemoveEditCommand);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ReapplyEditCommand::name()) {
        IPC::handleMessage<Messages::WebPage::ReapplyEditCommand>(decoder, this, &WebPage::reapplyEditCommand);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::UnapplyEditCommand::name()) {
        IPC::handleMessage<Messages::WebPage::UnapplyEditCommand>(decoder, this, &WebPage::unapplyEditCommand);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetPageAndTextZoomFactors::name()) {
        IPC::handleMessage<Messages::WebPage::SetPageAndTextZoomFactors>(decoder, this, &WebPage::setPageAndTextZoomFactors);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetPageZoomFactor::name()) {
        IPC::handleMessage<Messages::WebPage::SetPageZoomFactor>(decoder, this, &WebPage::setPageZoomFactor);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetTextZoomFactor::name()) {
        IPC::handleMessage<Messages::WebPage::SetTextZoomFactor>(decoder, this, &WebPage::setTextZoomFactor);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::WindowScreenDidChange::name()) {
        IPC::handleMessage<Messages::WebPage::WindowScreenDidChange>(decoder, this, &WebPage::windowScreenDidChange);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ScalePage::name()) {
        IPC::handleMessage<Messages::WebPage::ScalePage>(decoder, this, &WebPage::scalePage);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ScalePageInViewCoordinates::name()) {
        IPC::handleMessage<Messages::WebPage::ScalePageInViewCoordinates>(decoder, this, &WebPage::scalePageInViewCoordinates);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ScaleView::name()) {
        IPC::handleMessage<Messages::WebPage::ScaleView>(decoder, this, &WebPage::scaleView);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetUseFixedLayout::name()) {
        IPC::handleMessage<Messages::WebPage::SetUseFixedLayout>(decoder, this, &WebPage::setUseFixedLayout);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetFixedLayoutSize::name()) {
        IPC::handleMessage<Messages::WebPage::SetFixedLayoutSize>(decoder, this, &WebPage::setFixedLayoutSize);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ListenForLayoutMilestones::name()) {
        IPC::handleMessage<Messages::WebPage::ListenForLayoutMilestones>(decoder, this, &WebPage::listenForLayoutMilestones);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetSuppressScrollbarAnimations::name()) {
        IPC::handleMessage<Messages::WebPage::SetSuppressScrollbarAnimations>(decoder, this, &WebPage::setSuppressScrollbarAnimations);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetEnableVerticalRubberBanding::name()) {
        IPC::handleMessage<Messages::WebPage::SetEnableVerticalRubberBanding>(decoder, this, &WebPage::setEnableVerticalRubberBanding);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetEnableHorizontalRubberBanding::name()) {
        IPC::handleMessage<Messages::WebPage::SetEnableHorizontalRubberBanding>(decoder, this, &WebPage::setEnableHorizontalRubberBanding);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetBackgroundExtendsBeyondPage::name()) {
        IPC::handleMessage<Messages::WebPage::SetBackgroundExtendsBeyondPage>(decoder, this, &WebPage::setBackgroundExtendsBeyondPage);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetPaginationMode::name()) {
        IPC::handleMessage<Messages::WebPage::SetPaginationMode>(decoder, this, &WebPage::setPaginationMode);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetPaginationBehavesLikeColumns::name()) {
        IPC::handleMessage<Messages::WebPage::SetPaginationBehavesLikeColumns>(decoder, this, &WebPage::setPaginationBehavesLikeColumns);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetPageLength::name()) {
        IPC::handleMessage<Messages::WebPage::SetPageLength>(decoder, this, &WebPage::setPageLength);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetGapBetweenPages::name()) {
        IPC::handleMessage<Messages::WebPage::SetGapBetweenPages>(decoder, this, &WebPage::setGapBetweenPages);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetPaginationLineGridEnabled::name()) {
        IPC::handleMessage<Messages::WebPage::SetPaginationLineGridEnabled>(decoder, this, &WebPage::setPaginationLineGridEnabled);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::PostInjectedBundleMessage::name()) {
        IPC::handleMessage<Messages::WebPage::PostInjectedBundleMessage>(decoder, this, &WebPage::postInjectedBundleMessage);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::FindString::name()) {
        IPC::handleMessage<Messages::WebPage::FindString>(decoder, this, &WebPage::findString);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::FindStringMatches::name()) {
        IPC::handleMessage<Messages::WebPage::FindStringMatches>(decoder, this, &WebPage::findStringMatches);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetImageForFindMatch::name()) {
        IPC::handleMessage<Messages::WebPage::GetImageForFindMatch>(decoder, this, &WebPage::getImageForFindMatch);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SelectFindMatch::name()) {
        IPC::handleMessage<Messages::WebPage::SelectFindMatch>(decoder, this, &WebPage::selectFindMatch);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::HideFindUI::name()) {
        IPC::handleMessage<Messages::WebPage::HideFindUI>(decoder, this, &WebPage::hideFindUI);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::CountStringMatches::name()) {
        IPC::handleMessage<Messages::WebPage::CountStringMatches>(decoder, this, &WebPage::countStringMatches);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::AddMIMETypeWithCustomContentProvider::name()) {
        IPC::handleMessage<Messages::WebPage::AddMIMETypeWithCustomContentProvider>(decoder, this, &WebPage::addMIMETypeWithCustomContentProvider);
        return;
    }
#if PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPage::PerformDragControllerAction::name()) {
        IPC::handleMessage<Messages::WebPage::PerformDragControllerAction>(decoder, this, &WebPage::performDragControllerAction);
        return;
    }
#endif
#if !PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPage::PerformDragControllerAction::name()) {
        IPC::handleMessage<Messages::WebPage::PerformDragControllerAction>(decoder, this, &WebPage::performDragControllerAction);
        return;
    }
#endif
#if ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPage::DidStartDrag::name()) {
        IPC::handleMessage<Messages::WebPage::DidStartDrag>(decoder, this, &WebPage::didStartDrag);
        return;
    }
#endif
#if ENABLE(DRAG_SUPPORT)
    if (decoder.messageName() == Messages::WebPage::DragEnded::name()) {
        IPC::handleMessage<Messages::WebPage::DragEnded>(decoder, this, &WebPage::dragEnded);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::DidChangeSelectedIndexForActivePopupMenu::name()) {
        IPC::handleMessage<Messages::WebPage::DidChangeSelectedIndexForActivePopupMenu>(decoder, this, &WebPage::didChangeSelectedIndexForActivePopupMenu);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetTextForActivePopupMenu::name()) {
        IPC::handleMessage<Messages::WebPage::SetTextForActivePopupMenu>(decoder, this, &WebPage::setTextForActivePopupMenu);
        return;
    }
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPage::FailedToShowPopupMenu::name()) {
        IPC::handleMessage<Messages::WebPage::FailedToShowPopupMenu>(decoder, this, &WebPage::failedToShowPopupMenu);
        return;
    }
#endif
#if ENABLE(CONTEXT_MENUS)
    if (decoder.messageName() == Messages::WebPage::DidSelectItemFromActiveContextMenu::name()) {
        IPC::handleMessage<Messages::WebPage::DidSelectItemFromActiveContextMenu>(decoder, this, &WebPage::didSelectItemFromActiveContextMenu);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::DidChooseFilesForOpenPanelWithDisplayStringAndIcon::name()) {
        IPC::handleMessage<Messages::WebPage::DidChooseFilesForOpenPanelWithDisplayStringAndIcon>(decoder, this, &WebPage::didChooseFilesForOpenPanelWithDisplayStringAndIcon);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::DidChooseFilesForOpenPanel::name()) {
        IPC::handleMessage<Messages::WebPage::DidChooseFilesForOpenPanel>(decoder, this, &WebPage::didChooseFilesForOpenPanel);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::DidCancelForOpenPanel::name()) {
        IPC::handleMessage<Messages::WebPage::DidCancelForOpenPanel>(decoder, this, &WebPage::didCancelForOpenPanel);
        return;
    }
#if ENABLE(SANDBOX_EXTENSIONS)
    if (decoder.messageName() == Messages::WebPage::ExtendSandboxForFileFromOpenPanel::name()) {
        IPC::handleMessage<Messages::WebPage::ExtendSandboxForFileFromOpenPanel>(decoder, this, &WebPage::extendSandboxForFileFromOpenPanel);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::AdvanceToNextMisspelling::name()) {
        IPC::handleMessage<Messages::WebPage::AdvanceToNextMisspelling>(decoder, this, &WebPage::advanceToNextMisspelling);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ChangeSpellingToWord::name()) {
        IPC::handleMessage<Messages::WebPage::ChangeSpellingToWord>(decoder, this, &WebPage::changeSpellingToWord);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::DidFinishCheckingText::name()) {
        IPC::handleMessage<Messages::WebPage::DidFinishCheckingText>(decoder, this, &WebPage::didFinishCheckingText);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::DidCancelCheckingText::name()) {
        IPC::handleMessage<Messages::WebPage::DidCancelCheckingText>(decoder, this, &WebPage::didCancelCheckingText);
        return;
    }
#if USE(APPKIT)
    if (decoder.messageName() == Messages::WebPage::UppercaseWord::name()) {
        IPC::handleMessage<Messages::WebPage::UppercaseWord>(decoder, this, &WebPage::uppercaseWord);
        return;
    }
#endif
#if USE(APPKIT)
    if (decoder.messageName() == Messages::WebPage::LowercaseWord::name()) {
        IPC::handleMessage<Messages::WebPage::LowercaseWord>(decoder, this, &WebPage::lowercaseWord);
        return;
    }
#endif
#if USE(APPKIT)
    if (decoder.messageName() == Messages::WebPage::CapitalizeWord::name()) {
        IPC::handleMessage<Messages::WebPage::CapitalizeWord>(decoder, this, &WebPage::capitalizeWord);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::SetSmartInsertDeleteEnabled::name()) {
        IPC::handleMessage<Messages::WebPage::SetSmartInsertDeleteEnabled>(decoder, this, &WebPage::setSmartInsertDeleteEnabled);
        return;
    }
#endif
#if ENABLE(GEOLOCATION)
    if (decoder.messageName() == Messages::WebPage::DidReceiveGeolocationPermissionDecision::name()) {
        IPC::handleMessage<Messages::WebPage::DidReceiveGeolocationPermissionDecision>(decoder, this, &WebPage::didReceiveGeolocationPermissionDecision);
        return;
    }
#endif
#if ENABLE(MEDIA_STREAM)
    if (decoder.messageName() == Messages::WebPage::DidReceiveUserMediaPermissionDecision::name()) {
        IPC::handleMessage<Messages::WebPage::DidReceiveUserMediaPermissionDecision>(decoder, this, &WebPage::didReceiveUserMediaPermissionDecision);
        return;
    }
#endif
#if ENABLE(MEDIA_STREAM)
    if (decoder.messageName() == Messages::WebPage::DidCompleteUserMediaPermissionCheck::name()) {
        IPC::handleMessage<Messages::WebPage::DidCompleteUserMediaPermissionCheck>(decoder, this, &WebPage::didCompleteUserMediaPermissionCheck);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::DidReceiveNotificationPermissionDecision::name()) {
        IPC::handleMessage<Messages::WebPage::DidReceiveNotificationPermissionDecision>(decoder, this, &WebPage::didReceiveNotificationPermissionDecision);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::BeginPrinting::name()) {
        IPC::handleMessage<Messages::WebPage::BeginPrinting>(decoder, this, &WebPage::beginPrinting);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::EndPrinting::name()) {
        IPC::handleMessage<Messages::WebPage::EndPrinting>(decoder, this, &WebPage::endPrinting);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::ComputePagesForPrinting::name()) {
        IPC::handleMessage<Messages::WebPage::ComputePagesForPrinting>(decoder, this, &WebPage::computePagesForPrinting);
        return;
    }
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::DrawRectToImage::name()) {
        IPC::handleMessage<Messages::WebPage::DrawRectToImage>(decoder, this, &WebPage::drawRectToImage);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::DrawPagesToPDF::name()) {
        IPC::handleMessage<Messages::WebPage::DrawPagesToPDF>(decoder, this, &WebPage::drawPagesToPDF);
        return;
    }
#endif
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPage::DrawPagesForPrinting::name()) {
        IPC::handleMessage<Messages::WebPage::DrawPagesForPrinting>(decoder, this, &WebPage::drawPagesForPrinting);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetMediaVolume::name()) {
        IPC::handleMessage<Messages::WebPage::SetMediaVolume>(decoder, this, &WebPage::setMediaVolume);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetMuted::name()) {
        IPC::handleMessage<Messages::WebPage::SetMuted>(decoder, this, &WebPage::setMuted);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetMayStartMediaWhenInWindow::name()) {
        IPC::handleMessage<Messages::WebPage::SetMayStartMediaWhenInWindow>(decoder, this, &WebPage::setMayStartMediaWhenInWindow);
        return;
    }
#if ENABLE(MEDIA_SESSION)
    if (decoder.messageName() == Messages::WebPage::HandleMediaEvent::name()) {
        IPC::handleMessage<Messages::WebPage::HandleMediaEvent>(decoder, this, &WebPage::handleMediaEvent);
        return;
    }
#endif
#if ENABLE(MEDIA_SESSION)
    if (decoder.messageName() == Messages::WebPage::SetVolumeOfMediaElement::name()) {
        IPC::handleMessage<Messages::WebPage::SetVolumeOfMediaElement>(decoder, this, &WebPage::setVolumeOfMediaElement);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetCanRunBeforeUnloadConfirmPanel::name()) {
        IPC::handleMessage<Messages::WebPage::SetCanRunBeforeUnloadConfirmPanel>(decoder, this, &WebPage::setCanRunBeforeUnloadConfirmPanel);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetCanRunModal::name()) {
        IPC::handleMessage<Messages::WebPage::SetCanRunModal>(decoder, this, &WebPage::setCanRunModal);
        return;
    }
#if PLATFORM(EFL)
    if (decoder.messageName() == Messages::WebPage::SetThemePath::name()) {
        IPC::handleMessage<Messages::WebPage::SetThemePath>(decoder, this, &WebPage::setThemePath);
        return;
    }
#endif
#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    if (decoder.messageName() == Messages::WebPage::CommitPageTransitionViewport::name()) {
        IPC::handleMessage<Messages::WebPage::CommitPageTransitionViewport>(decoder, this, &WebPage::commitPageTransitionViewport);
        return;
    }
#endif
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPage::SetComposition::name()) {
        IPC::handleMessage<Messages::WebPage::SetComposition>(decoder, this, &WebPage::setComposition);
        return;
    }
#endif
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPage::ConfirmComposition::name()) {
        IPC::handleMessage<Messages::WebPage::ConfirmComposition>(decoder, this, &WebPage::confirmComposition);
        return;
    }
#endif
#if PLATFORM(GTK)
    if (decoder.messageName() == Messages::WebPage::CancelComposition::name()) {
        IPC::handleMessage<Messages::WebPage::CancelComposition>(decoder, this, &WebPage::cancelComposition);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::SendComplexTextInputToPlugin::name()) {
        IPC::handleMessage<Messages::WebPage::SendComplexTextInputToPlugin>(decoder, this, &WebPage::sendComplexTextInputToPlugin);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::WindowAndViewFramesChanged::name()) {
        IPC::handleMessage<Messages::WebPage::WindowAndViewFramesChanged>(decoder, this, &WebPage::windowAndViewFramesChanged);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::SetMainFrameIsScrollable::name()) {
        IPC::handleMessage<Messages::WebPage::SetMainFrameIsScrollable>(decoder, this, &WebPage::setMainFrameIsScrollable);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::RegisterUIProcessAccessibilityTokens::name()) {
        IPC::handleMessage<Messages::WebPage::RegisterUIProcessAccessibilityTokens>(decoder, this, &WebPage::registerUIProcessAccessibilityTokens);
        return;
    }
#endif
#if (PLATFORM(COCOA) && ENABLE(SERVICE_CONTROLS))
    if (decoder.messageName() == Messages::WebPage::ReplaceSelectionWithPasteboardData::name()) {
        IPC::handleMessage<Messages::WebPage::ReplaceSelectionWithPasteboardData>(decoder, this, &WebPage::replaceSelectionWithPasteboardData);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::InsertTextAsync::name()) {
        IPC::handleMessage<Messages::WebPage::InsertTextAsync>(decoder, this, &WebPage::insertTextAsync);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::GetMarkedRangeAsync::name()) {
        IPC::handleMessage<Messages::WebPage::GetMarkedRangeAsync>(decoder, this, &WebPage::getMarkedRangeAsync);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::GetSelectedRangeAsync::name()) {
        IPC::handleMessage<Messages::WebPage::GetSelectedRangeAsync>(decoder, this, &WebPage::getSelectedRangeAsync);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::CharacterIndexForPointAsync::name()) {
        IPC::handleMessage<Messages::WebPage::CharacterIndexForPointAsync>(decoder, this, &WebPage::characterIndexForPointAsync);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::FirstRectForCharacterRangeAsync::name()) {
        IPC::handleMessage<Messages::WebPage::FirstRectForCharacterRangeAsync>(decoder, this, &WebPage::firstRectForCharacterRangeAsync);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::SetCompositionAsync::name()) {
        IPC::handleMessage<Messages::WebPage::SetCompositionAsync>(decoder, this, &WebPage::setCompositionAsync);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::ConfirmCompositionAsync::name()) {
        IPC::handleMessage<Messages::WebPage::ConfirmCompositionAsync>(decoder, this, &WebPage::confirmCompositionAsync);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::InsertDictatedTextAsync::name()) {
        IPC::handleMessage<Messages::WebPage::InsertDictatedTextAsync>(decoder, this, &WebPage::insertDictatedTextAsync);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::AttributedSubstringForCharacterRangeAsync::name()) {
        IPC::handleMessage<Messages::WebPage::AttributedSubstringForCharacterRangeAsync>(decoder, this, &WebPage::attributedSubstringForCharacterRangeAsync);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::FontAtSelection::name()) {
        IPC::handleMessage<Messages::WebPage::FontAtSelection>(decoder, this, &WebPage::fontAtSelection);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetMinimumLayoutSize::name()) {
        IPC::handleMessage<Messages::WebPage::SetMinimumLayoutSize>(decoder, this, &WebPage::setMinimumLayoutSize);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetAutoSizingShouldExpandToViewHeight::name()) {
        IPC::handleMessage<Messages::WebPage::SetAutoSizingShouldExpandToViewHeight>(decoder, this, &WebPage::setAutoSizingShouldExpandToViewHeight);
        return;
    }
#if PLATFORM(EFL)
    if (decoder.messageName() == Messages::WebPage::ConfirmComposition::name()) {
        IPC::handleMessage<Messages::WebPage::ConfirmComposition>(decoder, this, &WebPage::confirmComposition);
        return;
    }
#endif
#if PLATFORM(EFL)
    if (decoder.messageName() == Messages::WebPage::SetComposition::name()) {
        IPC::handleMessage<Messages::WebPage::SetComposition>(decoder, this, &WebPage::setComposition);
        return;
    }
#endif
#if PLATFORM(EFL)
    if (decoder.messageName() == Messages::WebPage::CancelComposition::name()) {
        IPC::handleMessage<Messages::WebPage::CancelComposition>(decoder, this, &WebPage::cancelComposition);
        return;
    }
#endif
#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
    if (decoder.messageName() == Messages::WebPage::FindZoomableAreaForPoint::name()) {
        IPC::handleMessage<Messages::WebPage::FindZoomableAreaForPoint>(decoder, this, &WebPage::findZoomableAreaForPoint);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::HandleAlternativeTextUIResult::name()) {
        IPC::handleMessage<Messages::WebPage::HandleAlternativeTextUIResult>(decoder, this, &WebPage::handleAlternativeTextUIResult);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::WillStartUserTriggeredZooming::name()) {
        IPC::handleMessage<Messages::WebPage::WillStartUserTriggeredZooming>(decoder, this, &WebPage::willStartUserTriggeredZooming);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetScrollPinningBehavior::name()) {
        IPC::handleMessage<Messages::WebPage::SetScrollPinningBehavior>(decoder, this, &WebPage::setScrollPinningBehavior);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetScrollbarOverlayStyle::name()) {
        IPC::handleMessage<Messages::WebPage::SetScrollbarOverlayStyle>(decoder, this, &WebPage::setScrollbarOverlayStyle);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::GetBytecodeProfile::name()) {
        IPC::handleMessage<Messages::WebPage::GetBytecodeProfile>(decoder, this, &WebPage::getBytecodeProfile);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::TakeSnapshot::name()) {
        IPC::handleMessage<Messages::WebPage::TakeSnapshot>(decoder, this, &WebPage::takeSnapshot);
        return;
    }
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::PerformImmediateActionHitTestAtLocation::name()) {
        IPC::handleMessage<Messages::WebPage::PerformImmediateActionHitTestAtLocation>(decoder, this, &WebPage::performImmediateActionHitTestAtLocation);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::ImmediateActionDidUpdate::name()) {
        IPC::handleMessage<Messages::WebPage::ImmediateActionDidUpdate>(decoder, this, &WebPage::immediateActionDidUpdate);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::ImmediateActionDidCancel::name()) {
        IPC::handleMessage<Messages::WebPage::ImmediateActionDidCancel>(decoder, this, &WebPage::immediateActionDidCancel);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::ImmediateActionDidComplete::name()) {
        IPC::handleMessage<Messages::WebPage::ImmediateActionDidComplete>(decoder, this, &WebPage::immediateActionDidComplete);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::DataDetectorsDidPresentUI::name()) {
        IPC::handleMessage<Messages::WebPage::DataDetectorsDidPresentUI>(decoder, this, &WebPage::dataDetectorsDidPresentUI);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::DataDetectorsDidChangeUI::name()) {
        IPC::handleMessage<Messages::WebPage::DataDetectorsDidChangeUI>(decoder, this, &WebPage::dataDetectorsDidChangeUI);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::DataDetectorsDidHideUI::name()) {
        IPC::handleMessage<Messages::WebPage::DataDetectorsDidHideUI>(decoder, this, &WebPage::dataDetectorsDidHideUI);
        return;
    }
#endif
#if PLATFORM(MAC)
    if (decoder.messageName() == Messages::WebPage::HandleAcceptedCandidate::name()) {
        IPC::handleMessage<Messages::WebPage::HandleAcceptedCandidate>(decoder, this, &WebPage::handleAcceptedCandidate);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetShouldDispatchFakeMouseMoveEvents::name()) {
        IPC::handleMessage<Messages::WebPage::SetShouldDispatchFakeMouseMoveEvents>(decoder, this, &WebPage::setShouldDispatchFakeMouseMoveEvents);
        return;
    }
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::PlaybackTargetSelected::name()) {
        IPC::handleMessage<Messages::WebPage::PlaybackTargetSelected>(decoder, this, &WebPage::playbackTargetSelected);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::PlaybackTargetAvailabilityDidChange::name()) {
        IPC::handleMessage<Messages::WebPage::PlaybackTargetAvailabilityDidChange>(decoder, this, &WebPage::playbackTargetAvailabilityDidChange);
        return;
    }
#endif
#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SetShouldPlayToPlaybackTarget::name()) {
        IPC::handleMessage<Messages::WebPage::SetShouldPlayToPlaybackTarget>(decoder, this, &WebPage::setShouldPlayToPlaybackTarget);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::ClearWheelEventTestTrigger::name()) {
        IPC::handleMessage<Messages::WebPage::ClearWheelEventTestTrigger>(decoder, this, &WebPage::clearWheelEventTestTrigger);
        return;
    }
    if (decoder.messageName() == Messages::WebPage::SetShouldScaleViewToFitDocument::name()) {
        IPC::handleMessage<Messages::WebPage::SetShouldScaleViewToFitDocument>(decoder, this, &WebPage::setShouldScaleViewToFitDocument);
        return;
    }
#if ENABLE(VIDEO) && USE(GSTREAMER)
    if (decoder.messageName() == Messages::WebPage::DidEndRequestInstallMissingMediaPlugins::name()) {
        IPC::handleMessage<Messages::WebPage::DidEndRequestInstallMissingMediaPlugins>(decoder, this, &WebPage::didEndRequestInstallMissingMediaPlugins);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::SetResourceCachingDisabled::name()) {
        IPC::handleMessage<Messages::WebPage::SetResourceCachingDisabled>(decoder, this, &WebPage::setResourceCachingDisabled);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebPage::didReceiveSyncWebPageMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder, std::unique_ptr<IPC::MessageEncoder>& replyEncoder)
{
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SynchronizeDynamicViewportUpdate::name()) {
        IPC::handleMessage<Messages::WebPage::SynchronizeDynamicViewportUpdate>(decoder, *replyEncoder, this, &WebPage::synchronizeDynamicViewportUpdate);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::SyncApplyAutocorrection::name()) {
        IPC::handleMessage<Messages::WebPage::SyncApplyAutocorrection>(decoder, *replyEncoder, this, &WebPage::syncApplyAutocorrection);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::GetAutocorrectionContext::name()) {
        IPC::handleMessage<Messages::WebPage::GetAutocorrectionContext>(decoder, *replyEncoder, this, &WebPage::getAutocorrectionContext);
        return;
    }
#endif
#if PLATFORM(IOS)
    if (decoder.messageName() == Messages::WebPage::GetPositionInformation::name()) {
        IPC::handleMessage<Messages::WebPage::GetPositionInformation>(decoder, *replyEncoder, this, &WebPage::getPositionInformation);
        return;
    }
#endif
#if ENABLE(IOS_TOUCH_EVENTS)
    if (decoder.messageName() == Messages::WebPage::TouchEventSync::name()) {
        IPC::handleMessage<Messages::WebPage::TouchEventSync>(decoder, *replyEncoder, this, &WebPage::touchEventSync);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::PostSynchronousInjectedBundleMessage::name()) {
        IPC::handleMessage<Messages::WebPage::PostSynchronousInjectedBundleMessage>(decoder, *replyEncoder, this, &WebPage::postSynchronousInjectedBundleMessage);
        return;
    }
#if (PLATFORM(COCOA) && PLATFORM(IOS))
    if (decoder.messageName() == Messages::WebPage::ComputePagesForPrintingAndDrawToPDF::name()) {
        IPC::handleMessageDelayed<Messages::WebPage::ComputePagesForPrintingAndDrawToPDF>(connection, decoder, replyEncoder, this, &WebPage::computePagesForPrintingAndDrawToPDF);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebPage::Dummy::name()) {
        IPC::handleMessage<Messages::WebPage::Dummy>(decoder, *replyEncoder, this, &WebPage::dummy);
        return;
    }
#if PLATFORM (GTK) && HAVE(GTK_GESTURES)
    if (decoder.messageName() == Messages::WebPage::GetCenterForZoomGesture::name()) {
        IPC::handleMessage<Messages::WebPage::GetCenterForZoomGesture>(decoder, *replyEncoder, this, &WebPage::getCenterForZoomGesture);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::GetStringSelectionForPasteboard::name()) {
        IPC::handleMessage<Messages::WebPage::GetStringSelectionForPasteboard>(decoder, *replyEncoder, this, &WebPage::getStringSelectionForPasteboard);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::GetDataSelectionForPasteboard::name()) {
        IPC::handleMessage<Messages::WebPage::GetDataSelectionForPasteboard>(decoder, *replyEncoder, this, &WebPage::getDataSelectionForPasteboard);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::ReadSelectionFromPasteboard::name()) {
        IPC::handleMessage<Messages::WebPage::ReadSelectionFromPasteboard>(decoder, *replyEncoder, this, &WebPage::readSelectionFromPasteboard);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::ShouldDelayWindowOrderingEvent::name()) {
        IPC::handleMessage<Messages::WebPage::ShouldDelayWindowOrderingEvent>(decoder, *replyEncoder, this, &WebPage::shouldDelayWindowOrderingEvent);
        return;
    }
#endif
#if PLATFORM(COCOA)
    if (decoder.messageName() == Messages::WebPage::AcceptsFirstMouse::name()) {
        IPC::handleMessage<Messages::WebPage::AcceptsFirstMouse>(decoder, *replyEncoder, this, &WebPage::acceptsFirstMouse);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
