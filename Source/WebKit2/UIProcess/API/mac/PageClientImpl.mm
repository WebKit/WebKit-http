/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#import "config.h"
#import "PageClientImpl.h"

#import "DataReference.h"
#import "DictionaryPopupInfo.h"
#import "FindIndicator.h"
#import "NativeWebKeyboardEvent.h"
#import "WKAPICast.h"
#import "WKStringCF.h"
#import "WKViewInternal.h"
#import "WebContextMenuProxyMac.h"
#import "WebEditCommandProxy.h"
#import "WebPopupMenuProxyMac.h"
#import <WebCore/Cursor.h>
#import <WebCore/FloatRect.h>
#import <WebCore/FoundationExtras.h>
#import <WebCore/GraphicsContext.h>
#import <WebCore/KeyboardEvent.h>
#import <WebCore/NotImplemented.h>
#import <wtf/PassOwnPtr.h>
#import <wtf/text/CString.h>
#import <wtf/text/WTFString.h>
#import <WebKitSystemInterface.h>

@interface NSApplication (WebNSApplicationDetails)
- (NSCursor *)_cursorRectCursor;
@end

using namespace WebCore;
using namespace WebKit;

@interface WKEditCommandObjC : NSObject
{
    RefPtr<WebEditCommandProxy> m_command;
}
- (id)initWithWebEditCommandProxy:(PassRefPtr<WebEditCommandProxy>)command;
- (WebEditCommandProxy*)command;
@end

@interface WKEditorUndoTargetObjC : NSObject
- (void)undoEditing:(id)sender;
- (void)redoEditing:(id)sender;
@end

@implementation WKEditCommandObjC

- (id)initWithWebEditCommandProxy:(PassRefPtr<WebEditCommandProxy>)command
{
    self = [super init];
    if (!self)
        return nil;

    m_command = command;
    return self;
}

- (WebEditCommandProxy*)command
{
    return m_command.get();
}

@end

@implementation WKEditorUndoTargetObjC

- (void)undoEditing:(id)sender
{
    ASSERT([sender isKindOfClass:[WKEditCommandObjC class]]);
    [sender command]->unapply();
}

- (void)redoEditing:(id)sender
{
    ASSERT([sender isKindOfClass:[WKEditCommandObjC class]]);
    [sender command]->reapply();
}

@end

namespace WebKit {

NSString* nsStringFromWebCoreString(const String& string)
{
    return string.impl() ? HardAutorelease(WKStringCopyCFString(0, toAPI(string.impl()))) : @"";
}

PassOwnPtr<PageClientImpl> PageClientImpl::create(WKView* wkView)
{
    return adoptPtr(new PageClientImpl(wkView));
}

PageClientImpl::PageClientImpl(WKView* wkView)
    : m_wkView(wkView)
    , m_undoTarget(AdoptNS, [[WKEditorUndoTargetObjC alloc] init])
{
}

PageClientImpl::~PageClientImpl()
{
}

PassOwnPtr<DrawingAreaProxy> PageClientImpl::createDrawingAreaProxy()
{
    return [m_wkView _createDrawingAreaProxy];
}

void PageClientImpl::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
    [m_wkView setNeedsDisplayInRect:rect];
}

void PageClientImpl::displayView()
{
    [m_wkView displayIfNeeded];
}

void PageClientImpl::scrollView(const IntRect& scrollRect, const IntSize& scrollOffset)
{
    NSRect clippedScrollRect = NSIntersectionRect(scrollRect, NSOffsetRect(scrollRect, -scrollOffset.width(), -scrollOffset.height()));

    [m_wkView _cacheWindowBottomCornerRect];

    [m_wkView translateRectsNeedingDisplayInRect:clippedScrollRect by:scrollOffset];
    [m_wkView scrollRect:clippedScrollRect by:scrollOffset];
}

IntSize PageClientImpl::viewSize()
{
    return IntSize([m_wkView bounds].size);
}

bool PageClientImpl::isViewWindowActive()
{
    return [[m_wkView window] isKeyWindow] || [NSApp keyWindow] == [m_wkView window];
}

bool PageClientImpl::isViewFocused()
{
    return [m_wkView _isFocused];
}

void PageClientImpl::makeFirstResponder()
{
     [[m_wkView window] makeFirstResponder:m_wkView];
}
    
bool PageClientImpl::isViewVisible()
{
    if (![m_wkView window])
        return false;

    if (![[m_wkView window] isVisible])
        return false;

    if ([m_wkView isHiddenOrHasHiddenAncestor])
        return false;

    return true;
}

bool PageClientImpl::isViewInWindow()
{
    return [m_wkView window];
}

void PageClientImpl::processDidCrash()
{
    [m_wkView _processDidCrash];
}
    
void PageClientImpl::pageClosed()
{
    [m_wkView _pageClosed];
}

void PageClientImpl::didRelaunchProcess()
{
    [m_wkView _didRelaunchProcess];
}

void PageClientImpl::toolTipChanged(const String& oldToolTip, const String& newToolTip)
{
    [m_wkView _toolTipChangedFrom:nsStringFromWebCoreString(oldToolTip) to:nsStringFromWebCoreString(newToolTip)];
}

void PageClientImpl::setCursor(const WebCore::Cursor& cursor)
{
    if (![NSApp _cursorRectCursor])
        [m_wkView _setCursor:cursor.platformCursor()];
}

void PageClientImpl::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    [NSCursor setHiddenUntilMouseMoves:hiddenUntilMouseMoves];
}

void PageClientImpl::didChangeViewportProperties(const WebCore::ViewportArguments&)
{
}

void PageClientImpl::registerEditCommand(PassRefPtr<WebEditCommandProxy> prpCommand, WebPageProxy::UndoOrRedo undoOrRedo)
{
    RefPtr<WebEditCommandProxy> command = prpCommand;

    RetainPtr<WKEditCommandObjC> commandObjC(AdoptNS, [[WKEditCommandObjC alloc] initWithWebEditCommandProxy:command]);
    String actionName = WebEditCommandProxy::nameForEditAction(command->editAction());

    NSUndoManager *undoManager = [m_wkView undoManager];
    [undoManager registerUndoWithTarget:m_undoTarget.get() selector:((undoOrRedo == WebPageProxy::Undo) ? @selector(undoEditing:) : @selector(redoEditing:)) object:commandObjC.get()];
    if (!actionName.isEmpty())
        [undoManager setActionName:(NSString *)actionName];
}

void PageClientImpl::clearAllEditCommands()
{
    [[m_wkView undoManager] removeAllActionsWithTarget:m_undoTarget.get()];
}

bool PageClientImpl::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return (undoOrRedo == WebPageProxy::Undo) ? [[m_wkView undoManager] canUndo] : [[m_wkView undoManager] canRedo];
}

void PageClientImpl::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    return (undoOrRedo == WebPageProxy::Undo) ? [[m_wkView undoManager] undo] : [[m_wkView undoManager] redo];
}

bool PageClientImpl::interpretKeyEvent(const NativeWebKeyboardEvent& event, Vector<WebCore::KeypressCommand>& commands)
{
    return [m_wkView _interpretKeyEvent:event.nativeEvent() savingCommandsTo:commands];
}

void PageClientImpl::setDragImage(const IntPoint& clientPosition, PassRefPtr<ShareableBitmap> dragImage, bool isLinkDrag)
{
    RetainPtr<CGImageRef> dragCGImage = dragImage->makeCGImage();
    RetainPtr<NSImage> dragNSImage(AdoptNS, [[NSImage alloc] initWithCGImage:dragCGImage.get() size:dragImage->size()]);

    [m_wkView _setDragImage:dragNSImage.get() at:clientPosition linkDrag:isLinkDrag];
}

void PageClientImpl::updateTextInputState(bool updateSecureInputState)
{
    [m_wkView _updateTextInputStateIncludingSecureInputState:updateSecureInputState];
}

void PageClientImpl::resetTextInputState()
{
    [m_wkView _resetTextInputState];
}

FloatRect PageClientImpl::convertToDeviceSpace(const FloatRect& rect)
{
    return [m_wkView _convertToDeviceSpace:rect];
}

FloatRect PageClientImpl::convertToUserSpace(const FloatRect& rect)
{
    return [m_wkView _convertToUserSpace:rect];
}
   
IntPoint PageClientImpl::screenToWindow(const IntPoint& point)
{
    NSPoint windowCoord = [[m_wkView window] convertScreenToBase:point];
    return IntPoint([m_wkView convertPoint:windowCoord fromView:nil]);
}
    
IntRect PageClientImpl::windowToScreen(const IntRect& rect)
{
    NSRect tempRect = rect;
    tempRect = [m_wkView convertRect:tempRect toView:nil];
    tempRect.origin = [[m_wkView window] convertBaseToScreen:tempRect.origin];
    return enclosingIntRect(tempRect);
}

void PageClientImpl::doneWithKeyEvent(const NativeWebKeyboardEvent& event, bool eventWasHandled)
{
    [m_wkView _doneWithKeyEvent:event.nativeEvent() eventWasHandled:eventWasHandled];
}

PassRefPtr<WebPopupMenuProxy> PageClientImpl::createPopupMenuProxy(WebPageProxy* page)
{
    return WebPopupMenuProxyMac::create(m_wkView, page);
}

PassRefPtr<WebContextMenuProxy> PageClientImpl::createContextMenuProxy(WebPageProxy* page)
{
    return WebContextMenuProxyMac::create(m_wkView, page);
}

void PageClientImpl::setFindIndicator(PassRefPtr<FindIndicator> findIndicator, bool fadeOut, bool animate)
{
    [m_wkView _setFindIndicator:findIndicator fadeOut:fadeOut animate:animate];
}

void PageClientImpl::accessibilityWebProcessTokenReceived(const CoreIPC::DataReference& data)
{
    NSData* remoteToken = [NSData dataWithBytes:data.data() length:data.size()];
    [m_wkView _setAccessibilityWebProcessToken:remoteToken];
}
    
#if USE(ACCELERATED_COMPOSITING)
void PageClientImpl::enterAcceleratedCompositingMode(const LayerTreeContext& layerTreeContext)
{
    [m_wkView _enterAcceleratedCompositingMode:layerTreeContext];
}

void PageClientImpl::exitAcceleratedCompositingMode()
{
    [m_wkView _exitAcceleratedCompositingMode];
}
#endif // USE(ACCELERATED_COMPOSITING)

void PageClientImpl::pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus)
{
    [m_wkView _pluginFocusOrWindowFocusChanged:pluginHasFocusAndWindowHasFocus pluginComplexTextInputIdentifier:pluginComplexTextInputIdentifier];
}

void PageClientImpl::setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, PluginComplexTextInputState pluginComplexTextInputState)
{
    [m_wkView _setPluginComplexTextInputState:pluginComplexTextInputState pluginComplexTextInputIdentifier:pluginComplexTextInputIdentifier];
}

CGContextRef PageClientImpl::containingWindowGraphicsContext()
{
    NSWindow *window = [m_wkView window];

    // Don't try to get the graphics context if the NSWindow doesn't have a window device.
    if ([window windowNumber] <= 0)
        return 0;

    return static_cast<CGContextRef>([[window graphicsContext] graphicsPort]);
}

void PageClientImpl::didChangeScrollbarsForMainFrame() const
{
    [m_wkView _didChangeScrollbarsForMainFrame];
}

void PageClientImpl::didCommitLoadForMainFrame(bool useCustomRepresentation)
{
    [m_wkView _setPageHasCustomRepresentation:useCustomRepresentation];
}

void PageClientImpl::didFinishLoadingDataForCustomRepresentation(const String& suggestedFilename, const CoreIPC::DataReference& dataReference)
{
    [m_wkView _didFinishLoadingDataForCustomRepresentationWithSuggestedFilename:suggestedFilename dataReference:dataReference];
}

double PageClientImpl::customRepresentationZoomFactor()
{
    return [m_wkView _customRepresentationZoomFactor];
}

void PageClientImpl::setCustomRepresentationZoomFactor(double zoomFactor)
{
    [m_wkView _setCustomRepresentationZoomFactor:zoomFactor];
}

void PageClientImpl::findStringInCustomRepresentation(const String& string, FindOptions options, unsigned maxMatchCount)
{
    [m_wkView _findStringInCustomRepresentation:string withFindOptions:options maxMatchCount:maxMatchCount];
}

void PageClientImpl::countStringMatchesInCustomRepresentation(const String& string, FindOptions options, unsigned maxMatchCount)
{
    [m_wkView _countStringMatchesInCustomRepresentation:string withFindOptions:options maxMatchCount:maxMatchCount];
}

void PageClientImpl::flashBackingStoreUpdates(const Vector<IntRect>&)
{
    notImplemented();
}

void PageClientImpl::didPerformDictionaryLookup(const String& text, double scaleFactor, const DictionaryPopupInfo& dictionaryPopupInfo)
{
    NSFontDescriptor *fontDescriptor = [NSFontDescriptor fontDescriptorWithFontAttributes:(NSDictionary *)dictionaryPopupInfo.fontInfo.fontAttributeDictionary.get()];
    NSFont *font = [NSFont fontWithDescriptor:fontDescriptor size:((scaleFactor != 1) ? [fontDescriptor pointSize] * scaleFactor : 0)];

    RetainPtr<NSMutableAttributedString> attributedString(AdoptNS, [[NSMutableAttributedString alloc] initWithString:nsStringFromWebCoreString(text)]);
    [attributedString.get() addAttribute:NSFontAttributeName value:font range:NSMakeRange(0, [attributedString.get() length])];

    NSPoint textBaselineOrigin = dictionaryPopupInfo.origin;

#if !defined(BUILDING_ON_SNOW_LEOPARD)
    // Convert to screen coordinates.
    textBaselineOrigin = [m_wkView convertPoint:textBaselineOrigin toView:nil];
    textBaselineOrigin = [m_wkView.window convertRectToScreen:NSMakeRect(textBaselineOrigin.x, textBaselineOrigin.y, 0, 0)].origin;

    WKShowWordDefinitionWindow(attributedString.get(), textBaselineOrigin, (NSDictionary *)dictionaryPopupInfo.options.get());
#else
    // If the dictionary lookup is being triggered by a hot key, force the overlay style.
    NSDictionary *options = (dictionaryPopupInfo.type == DictionaryPopupInfo::HotKey) ? [NSDictionary dictionaryWithObject:NSDefinitionPresentationTypeOverlay forKey:NSDefinitionPresentationTypeKey] : 0;
    [m_wkView showDefinitionForAttributedString:attributedString.get() range:NSMakeRange(0, [attributedString.get() length]) options:options baselineOriginProvider:^(NSRange adjustedRange) { return (NSPoint)textBaselineOrigin; }];
#endif
}

void PageClientImpl::dismissDictionaryLookupPanel()
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    WKHideWordDefinitionWindow();
#endif
}

void PageClientImpl::showCorrectionPanel(CorrectionPanelInfo::PanelType type, const FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings)
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    if (!isViewVisible() || !isViewInWindow())
        return;
    m_correctionPanel.show(m_wkView, type, boundingBoxOfReplacedString, replacedString, replacementString, alternativeReplacementStrings);
#endif
}

void PageClientImpl::dismissCorrectionPanel(ReasonForDismissingCorrectionPanel reason)
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    m_correctionPanel.dismiss(reason);
#endif
}

String PageClientImpl::dismissCorrectionPanelSoon(WebCore::ReasonForDismissingCorrectionPanel reason)
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    return m_correctionPanel.dismiss(reason);
#else
    return String();
#endif
}

void PageClientImpl::recordAutocorrectionResponse(EditorClient::AutocorrectionResponseType responseType, const String& replacedString, const String& replacementString)
{
#if !defined(BUILDING_ON_SNOW_LEOPARD)
    NSCorrectionResponse response = responseType == EditorClient::AutocorrectionReverted ? NSCorrectionResponseReverted : NSCorrectionResponseEdited;
    CorrectionPanel::recordAutocorrectionResponse(m_wkView, response, replacedString, replacementString);
#endif
}

bool PageClientImpl::executeSavedCommandBySelector(const String& selectorString)
{
    return [m_wkView _executeSavedCommandBySelector:NSSelectorFromString(selectorString)];
}

} // namespace WebKit
