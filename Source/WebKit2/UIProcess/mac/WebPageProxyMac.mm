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
#import "WebPageProxy.h"

#if PLATFORM(MAC)

#import "APIUIClient.h"
#import "AttributedString.h"
#import "ColorSpaceData.h"
#import "DataReference.h"
#import "DictionaryPopupInfo.h"
#import "EditingRange.h"
#import "EditorState.h"
#import "MenuUtilities.h"
#import "NativeWebKeyboardEvent.h"
#import "PageClient.h"
#import "PageClientImpl.h"
#import "PluginComplexTextInputState.h"
#import "StringUtilities.h"
#import "TextChecker.h"
#import "WKBrowsingContextControllerInternal.h"
#import "WebContext.h"
#import "WebPageMessages.h"
#import "WebProcessProxy.h"
#import <WebCore/DictationAlternative.h>
#import <WebCore/GraphicsLayer.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/SharedBuffer.h>
#import <WebCore/TextAlternativeWithRange.h>
#import <WebCore/UserAgent.h>
#import <mach-o/dyld.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/text/StringConcatenate.h>

@interface NSApplication (Details)
- (void)speakString:(NSString *)string;
@end

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, process().connection())

using namespace WebCore;

namespace WebKit {

static inline bool expectsLegacyImplicitRubberBandControl()
{
    if (applicationIsSafari()) {
        const int32_t firstVersionOfSafariNotExpectingImplicitRubberBandControl = 0x021A0F00; // 538.15.0
        bool linkedAgainstSafariExpectingImplicitRubberBandControl = NSVersionOfLinkTimeLibrary("Safari") < firstVersionOfSafariNotExpectingImplicitRubberBandControl;
        return linkedAgainstSafariExpectingImplicitRubberBandControl;
    }

    const int32_t firstVersionOfWebKit2WithNoImplicitRubberBandControl = 0x021A0200; // 538.2.0
    int32_t linkedWebKit2Version = NSVersionOfLinkTimeLibrary("WebKit2");
    return linkedWebKit2Version != -1 && linkedWebKit2Version < firstVersionOfWebKit2WithNoImplicitRubberBandControl;
}

void WebPageProxy::platformInitialize()
{
    static bool clientExpectsLegacyImplicitRubberBandControl = expectsLegacyImplicitRubberBandControl();
    setShouldUseImplicitRubberBandControl(clientExpectsLegacyImplicitRubberBandControl);
}

static String webKitBundleVersionString()
{
    return [[NSBundle bundleForClass:NSClassFromString(@"WKView")] objectForInfoDictionaryKey:(NSString *)kCFBundleVersionKey];
}

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    return standardUserAgentWithApplicationName(applicationNameForUserAgent, webKitBundleVersionString());
}

void WebPageProxy::getIsSpeaking(bool& isSpeaking)
{
    isSpeaking = [NSApp isSpeaking];
}

void WebPageProxy::speak(const String& string)
{
    [NSApp speakString:nsStringFromWebCoreString(string)];
}

void WebPageProxy::stopSpeaking()
{
    [NSApp stopSpeaking:nil];
}

void WebPageProxy::searchWithSpotlight(const String& string)
{
    [[NSWorkspace sharedWorkspace] showSearchResultsForQueryString:nsStringFromWebCoreString(string)];
}
    
void WebPageProxy::searchTheWeb(const String& string)
{
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithUniqueName];
    [pasteboard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    [pasteboard setString:string forType:NSStringPboardType];
    
    NSPerformService(@"Search With %WebSearchProvider@", pasteboard);
}

void WebPageProxy::windowAndViewFramesChanged(const FloatRect& viewFrameInWindowCoordinates, const FloatPoint& accessibilityViewCoordinates)
{
    if (!isValid())
        return;

    // In case the UI client overrides getWindowFrame(), we call it here to make sure we send the appropriate window frame.
    FloatRect windowFrameInScreenCoordinates = m_uiClient->windowFrame(this);
    FloatRect windowFrameInUnflippedScreenCoordinates = m_pageClient.convertToUserSpace(windowFrameInScreenCoordinates);

    process().send(Messages::WebPage::WindowAndViewFramesChanged(windowFrameInScreenCoordinates, windowFrameInUnflippedScreenCoordinates, viewFrameInWindowCoordinates, accessibilityViewCoordinates), m_pageID);
}

void WebPageProxy::setMainFrameIsScrollable(bool isScrollable)
{
    if (!isValid())
        return;

    process().send(Messages::WebPage::SetMainFrameIsScrollable(isScrollable), m_pageID);
}

#if !USE(ASYNC_NSTEXTINPUTCLIENT)

void WebPageProxy::setComposition(const String& text, Vector<CompositionUnderline> underlines, const EditingRange& selectionRange, const EditingRange& replacementRange)
{
    if (!isValid()) {
        // If this fails, we should call -discardMarkedText on input context to notify the input method.
        // This will happen naturally later, as part of reloading the page.
        return;
    }

    process().sendSync(Messages::WebPage::SetComposition(text, underlines, selectionRange, replacementRange), Messages::WebPage::SetComposition::Reply(m_editorState), m_pageID);
}

void WebPageProxy::confirmComposition()
{
    if (!isValid())
        return;

    process().sendSync(Messages::WebPage::ConfirmComposition(), Messages::WebPage::ConfirmComposition::Reply(m_editorState), m_pageID);
}

bool WebPageProxy::insertText(const String& text, const EditingRange& replacementRange)
{
    if (!isValid())
        return true;

    bool handled = true;
    process().sendSync(Messages::WebPage::InsertText(text, replacementRange), Messages::WebPage::InsertText::Reply(handled, m_editorState), m_pageID);
#if PLATFORM(MAC) && !USE(ASYNC_NSTEXTINPUTCLIENT)
    m_temporarilyClosedComposition = false;
#endif

    return handled;
}

bool WebPageProxy::insertDictatedText(const String& text, const EditingRange& replacementRange, const Vector<TextAlternativeWithRange>& dictationAlternativesWithRange)
{
#if USE(DICTATION_ALTERNATIVES)
    if (dictationAlternativesWithRange.isEmpty())
        return insertText(text, replacementRange);

    if (!isValid())
        return true;

    Vector<DictationAlternative> dictationAlternatives;

    for (size_t i = 0; i < dictationAlternativesWithRange.size(); ++i) {
        const TextAlternativeWithRange& alternativeWithRange = dictationAlternativesWithRange[i];
        uint64_t dictationContext = m_pageClient.addDictationAlternatives(alternativeWithRange.alternatives);
        if (dictationContext)
            dictationAlternatives.append(DictationAlternative(alternativeWithRange.range.location, alternativeWithRange.range.length, dictationContext));
    }

    if (dictationAlternatives.isEmpty())
        return insertText(text, replacementRange);

    bool handled = true;
    process().sendSync(Messages::WebPage::InsertDictatedText(text, replacementRange, dictationAlternatives), Messages::WebPage::InsertDictatedText::Reply(handled, m_editorState), m_pageID);
    return handled;
#else
    return insertText(text, replacementRange);
#endif
}

void WebPageProxy::getMarkedRange(EditingRange& result)
{
    result = EditingRange();

    if (!isValid())
        return;

    process().sendSync(Messages::WebPage::GetMarkedRange(), Messages::WebPage::GetMarkedRange::Reply(result), m_pageID);
    MESSAGE_CHECK(result.isValid());
}

void WebPageProxy::getSelectedRange(EditingRange& result)
{
    result = EditingRange();

    if (!isValid())
        return;

    process().sendSync(Messages::WebPage::GetSelectedRange(), Messages::WebPage::GetSelectedRange::Reply(result), m_pageID);
    MESSAGE_CHECK(result.isValid());
}

void WebPageProxy::getAttributedSubstringFromRange(const EditingRange& range, AttributedString& result)
{
    if (!isValid())
        return;
    process().sendSync(Messages::WebPage::GetAttributedSubstringFromRange(range), Messages::WebPage::GetAttributedSubstringFromRange::Reply(result), m_pageID);
}

uint64_t WebPageProxy::characterIndexForPoint(const IntPoint point)
{
    if (!isValid())
        return 0;

    uint64_t result = 0;
    process().sendSync(Messages::WebPage::CharacterIndexForPoint(point), Messages::WebPage::CharacterIndexForPoint::Reply(result), m_pageID);
    return result;
}

IntRect WebPageProxy::firstRectForCharacterRange(const EditingRange& range)
{
    if (!isValid())
        return IntRect();

    IntRect resultRect;
    process().sendSync(Messages::WebPage::FirstRectForCharacterRange(range), Messages::WebPage::FirstRectForCharacterRange::Reply(resultRect), m_pageID);
    return resultRect;
}

bool WebPageProxy::executeKeypressCommands(const Vector<WebCore::KeypressCommand>& commands)
{
    if (!isValid())
        return false;

    bool result = false;
    process().sendSync(Messages::WebPage::ExecuteKeypressCommands(commands), Messages::WebPage::ExecuteKeypressCommands::Reply(result, m_editorState), m_pageID);
#if PLATFORM(MAC) && !USE(ASYNC_NSTEXTINPUTCLIENT)
    m_temporarilyClosedComposition = false;
#endif
    return result;
}

void WebPageProxy::cancelComposition()
{
    if (!isValid())
        return;

    process().sendSync(Messages::WebPage::CancelComposition(), Messages::WebPage::CancelComposition::Reply(m_editorState), m_pageID);
}

#endif // !USE(ASYNC_NSTEXTINPUTCLIENT)

void WebPageProxy::insertDictatedTextAsync(const String& text, const EditingRange& replacementRange, const Vector<TextAlternativeWithRange>& dictationAlternativesWithRange, bool registerUndoGroup)
{
#if USE(DICTATION_ALTERNATIVES)
    if (!isValid())
        return;

    Vector<DictationAlternative> dictationAlternatives;

    for (const TextAlternativeWithRange& alternativeWithRange : dictationAlternativesWithRange) {
        uint64_t dictationContext = m_pageClient.addDictationAlternatives(alternativeWithRange.alternatives);
        if (dictationContext)
            dictationAlternatives.append(DictationAlternative(alternativeWithRange.range.location, alternativeWithRange.range.length, dictationContext));
    }

    if (dictationAlternatives.isEmpty()) {
        insertTextAsync(text, replacementRange, registerUndoGroup);
        return;
    }

    process().send(Messages::WebPage::InsertDictatedTextAsync(text, replacementRange, dictationAlternatives, registerUndoGroup), m_pageID);
#else
    insertTextAsync(text, replacementRange, registerUndoGroup);
#endif
}

void WebPageProxy::attributedSubstringForCharacterRangeAsync(const EditingRange& range, std::function<void (const AttributedString&, const EditingRange&, CallbackBase::Error)> callbackFunction)
{
    if (!isValid()) {
        callbackFunction(AttributedString(), EditingRange(), CallbackBase::Error::Unknown);
        return;
    }

    uint64_t callbackID = m_callbacks.put(WTF::move(callbackFunction), std::make_unique<ProcessThrottler::BackgroundActivityToken>(m_process->throttler()));

    process().send(Messages::WebPage::AttributedSubstringForCharacterRangeAsync(range, callbackID), m_pageID);
}

void WebPageProxy::attributedStringForCharacterRangeCallback(const AttributedString& string, const EditingRange& actualRange, uint64_t callbackID)
{
    MESSAGE_CHECK(actualRange.isValid());

    auto callback = m_callbacks.take<AttributedStringForCharacterRangeCallback>(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        // this can validly happen if a load invalidated the callback, though
        return;
    }

    callback->performCallbackWithReturnValue(string, actualRange);
}

String WebPageProxy::stringSelectionForPasteboard()
{
    String value;
    if (!isValid())
        return value;
    
    const auto messageTimeout = std::chrono::seconds(20);
    process().sendSync(Messages::WebPage::GetStringSelectionForPasteboard(), Messages::WebPage::GetStringSelectionForPasteboard::Reply(value), m_pageID, messageTimeout);
    return value;
}

PassRefPtr<WebCore::SharedBuffer> WebPageProxy::dataSelectionForPasteboard(const String& pasteboardType)
{
    if (!isValid())
        return 0;
    SharedMemory::Handle handle;
    uint64_t size = 0;
    const auto messageTimeout = std::chrono::seconds(20);
    process().sendSync(Messages::WebPage::GetDataSelectionForPasteboard(pasteboardType),
                                                Messages::WebPage::GetDataSelectionForPasteboard::Reply(handle, size), m_pageID, messageTimeout);
    if (handle.isNull())
        return 0;
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(handle, SharedMemory::ReadOnly);
    return SharedBuffer::create(static_cast<unsigned char *>(sharedMemoryBuffer->data()), size);
}

bool WebPageProxy::readSelectionFromPasteboard(const String& pasteboardName)
{
    if (!isValid())
        return false;

    bool result = false;
    const auto messageTimeout = std::chrono::seconds(20);
    process().sendSync(Messages::WebPage::ReadSelectionFromPasteboard(pasteboardName), Messages::WebPage::ReadSelectionFromPasteboard::Reply(result), m_pageID, messageTimeout);
    return result;
}

#if ENABLE(SERVICE_CONTROLS)
void WebPageProxy::replaceSelectionWithPasteboardData(const Vector<String>& types, const IPC::DataReference& data)
{
    process().send(Messages::WebPage::ReplaceSelectionWithPasteboardData(types, data), m_pageID);
}
#endif

#if ENABLE(DRAG_SUPPORT)
void WebPageProxy::setDragImage(const WebCore::IntPoint& clientPosition, const ShareableBitmap::Handle& dragImageHandle, bool isLinkDrag)
{
    RefPtr<ShareableBitmap> dragImage = ShareableBitmap::create(dragImageHandle);
    if (!dragImage)
        return;
    
    m_pageClient.setDragImage(clientPosition, dragImage.release(), isLinkDrag);
}

void WebPageProxy::setPromisedData(const String& pasteboardName, const SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension,
                                   const String& title, const String& url, const String& visibleURL, const SharedMemory::Handle& archiveHandle, uint64_t archiveSize)
{
    RefPtr<SharedMemory> sharedMemoryImage = SharedMemory::create(imageHandle, SharedMemory::ReadOnly);
    RefPtr<SharedBuffer> imageBuffer = SharedBuffer::create(static_cast<unsigned char*>(sharedMemoryImage->data()), imageSize);
    RefPtr<SharedBuffer> archiveBuffer;
    
    if (!archiveHandle.isNull()) {
        RefPtr<SharedMemory> sharedMemoryArchive = SharedMemory::create(archiveHandle, SharedMemory::ReadOnly);;
        archiveBuffer = SharedBuffer::create(static_cast<unsigned char*>(sharedMemoryArchive->data()), archiveSize);
    }
    m_pageClient.setPromisedData(pasteboardName, imageBuffer, filename, extension, title, url, visibleURL, archiveBuffer);
}
#endif

void WebPageProxy::performDictionaryLookupAtLocation(const WebCore::FloatPoint& point)
{
    if (!isValid())
        return;

    process().send(Messages::WebPage::PerformDictionaryLookupAtLocation(point), m_pageID);
}

// Complex text input support for plug-ins.
void WebPageProxy::sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput)
{
    if (!isValid())
        return;
    
    process().send(Messages::WebPage::SendComplexTextInputToPlugin(pluginComplexTextInputIdentifier, textInput), m_pageID);
}

void WebPageProxy::uppercaseWord()
{
    process().send(Messages::WebPage::UppercaseWord(), m_pageID);
}

void WebPageProxy::lowercaseWord()
{
    process().send(Messages::WebPage::LowercaseWord(), m_pageID);
}

void WebPageProxy::capitalizeWord()
{
    process().send(Messages::WebPage::CapitalizeWord(), m_pageID);
}

void WebPageProxy::setSmartInsertDeleteEnabled(bool isSmartInsertDeleteEnabled)
{
    if (m_isSmartInsertDeleteEnabled == isSmartInsertDeleteEnabled)
        return;

    TextChecker::setSmartInsertDeleteEnabled(isSmartInsertDeleteEnabled);
    m_isSmartInsertDeleteEnabled = isSmartInsertDeleteEnabled;
    process().send(Messages::WebPage::SetSmartInsertDeleteEnabled(isSmartInsertDeleteEnabled), m_pageID);
}

void WebPageProxy::didPerformDictionaryLookup(const AttributedString& text, const DictionaryPopupInfo& dictionaryPopupInfo)
{
    m_pageClient.didPerformDictionaryLookup(text, dictionaryPopupInfo);
}
    
void WebPageProxy::registerWebProcessAccessibilityToken(const IPC::DataReference& data)
{
    if (!isValid())
        return;
    
    m_pageClient.accessibilityWebProcessTokenReceived(data);
}    
    
void WebPageProxy::makeFirstResponder()
{
    m_pageClient.makeFirstResponder();
}

ColorSpaceData WebPageProxy::colorSpace()
{
    return m_pageClient.colorSpace();
}

void WebPageProxy::registerUIProcessAccessibilityTokens(const IPC::DataReference& elementToken, const IPC::DataReference& windowToken)
{
    if (!isValid())
        return;

    process().send(Messages::WebPage::RegisterUIProcessAccessibilityTokens(elementToken, windowToken), m_pageID);
}

void WebPageProxy::pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus)
{
    m_pageClient.pluginFocusOrWindowFocusChanged(pluginComplexTextInputIdentifier, pluginHasFocusAndWindowHasFocus);
}

void WebPageProxy::setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t pluginComplexTextInputState)
{
    MESSAGE_CHECK(isValidPluginComplexTextInputState(pluginComplexTextInputState));

    m_pageClient.setPluginComplexTextInputState(pluginComplexTextInputIdentifier, static_cast<PluginComplexTextInputState>(pluginComplexTextInputState));
}

void WebPageProxy::executeSavedCommandBySelector(const String& selector, bool& handled)
{
    MESSAGE_CHECK(isValidKeypressCommandName(selector));

    handled = m_pageClient.executeSavedCommandBySelector(selector);
}

bool WebPageProxy::shouldDelayWindowOrderingForEvent(const WebKit::WebMouseEvent& event)
{
    if (process().state() != WebProcessProxy::State::Running)
        return false;

    bool result = false;
    const auto messageTimeout = std::chrono::seconds(3);
    process().sendSync(Messages::WebPage::ShouldDelayWindowOrderingEvent(event), Messages::WebPage::ShouldDelayWindowOrderingEvent::Reply(result), m_pageID, messageTimeout);
    return result;
}

bool WebPageProxy::acceptsFirstMouse(int eventNumber, const WebKit::WebMouseEvent& event)
{
    if (!isValid())
        return false;

    bool result = false;
    const auto messageTimeout = std::chrono::seconds(3);
    process().sendSync(Messages::WebPage::AcceptsFirstMouse(eventNumber, event), Messages::WebPage::AcceptsFirstMouse::Reply(result), m_pageID, messageTimeout);
    return result;
}

WKView* WebPageProxy::wkView() const
{
    return m_pageClient.wkView();
}

void WebPageProxy::intrinsicContentSizeDidChange(const IntSize& intrinsicContentSize)
{
    m_pageClient.intrinsicContentSizeDidChange(intrinsicContentSize);
}

void WebPageProxy::setAcceleratedCompositingRootLayer(LayerOrView* rootLayer)
{
    m_pageClient.setAcceleratedCompositingRootLayer(rootLayer);
}

LayerOrView* WebPageProxy::acceleratedCompositingRootLayer() const
{
    return m_pageClient.acceleratedCompositingRootLayer();
}

static NSString *temporaryPDFDirectoryPath()
{
    static NSString *temporaryPDFDirectoryPath;

    if (!temporaryPDFDirectoryPath) {
        NSString *temporaryDirectoryTemplate = [NSTemporaryDirectory() stringByAppendingPathComponent:@"WebKitPDFs-XXXXXX"];
        CString templateRepresentation = [temporaryDirectoryTemplate fileSystemRepresentation];

        if (mkdtemp(templateRepresentation.mutableData()))
            temporaryPDFDirectoryPath = [[[NSFileManager defaultManager] stringWithFileSystemRepresentation:templateRepresentation.data() length:templateRepresentation.length()] copy];
    }

    return temporaryPDFDirectoryPath;
}

static NSString *pathToPDFOnDisk(const String& suggestedFilename)
{
    NSString *pdfDirectoryPath = temporaryPDFDirectoryPath();
    if (!pdfDirectoryPath) {
        WTFLogAlways("Cannot create temporary PDF download directory.");
        return nil;
    }

    NSString *path = [pdfDirectoryPath stringByAppendingPathComponent:suggestedFilename];

    NSFileManager *fileManager = [NSFileManager defaultManager];
    if ([fileManager fileExistsAtPath:path]) {
        NSString *pathTemplatePrefix = [pdfDirectoryPath stringByAppendingPathComponent:@"XXXXXX-"];
        NSString *pathTemplate = [pathTemplatePrefix stringByAppendingString:suggestedFilename];
        CString pathTemplateRepresentation = [pathTemplate fileSystemRepresentation];

        int fd = mkstemps(pathTemplateRepresentation.mutableData(), pathTemplateRepresentation.length() - strlen([pathTemplatePrefix fileSystemRepresentation]) + 1);
        if (fd < 0) {
            WTFLogAlways("Cannot create PDF file in the temporary directory (%s).", suggestedFilename.utf8().data());
            return nil;
        }

        close(fd);
        path = [fileManager stringWithFileSystemRepresentation:pathTemplateRepresentation.data() length:pathTemplateRepresentation.length()];
    }

    return path;
}

void WebPageProxy::savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(const String& suggestedFilename, const String& originatingURLString, const uint8_t* data, unsigned long size, const String& pdfUUID)
{
    // FIXME: Write originatingURLString to the file's originating URL metadata (perhaps WKSetMetadataURL?).
    UNUSED_PARAM(originatingURLString);

    if (!suggestedFilename.endsWith(".pdf", false)) {
        WTFLogAlways("Cannot save file without .pdf extension to the temporary directory.");
        return;
    }

    if (!size) {
        WTFLogAlways("Cannot save empty PDF file to the temporary directory.");
        return;
    }

    NSString *nsPath = pathToPDFOnDisk(suggestedFilename);

    if (!nsPath)
        return;

    RetainPtr<NSNumber> permissions = adoptNS([[NSNumber alloc] initWithInt:S_IRUSR]);
    RetainPtr<NSDictionary> fileAttributes = adoptNS([[NSDictionary alloc] initWithObjectsAndKeys:permissions.get(), NSFilePosixPermissions, nil]);
    RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytesNoCopy:(void*)data length:size freeWhenDone:NO]);

    if (![[NSFileManager defaultManager] createFileAtPath:nsPath contents:nsData.get() attributes:fileAttributes.get()]) {
        WTFLogAlways("Cannot create PDF file in the temporary directory (%s).", suggestedFilename.utf8().data());
        return;
    }

    m_temporaryPDFFiles.add(pdfUUID, nsPath);

    [[NSWorkspace sharedWorkspace] openFile:nsPath];
}

void WebPageProxy::savePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const IPC::DataReference& data, const String& pdfUUID)
{
    if (data.isEmpty()) {
        WTFLogAlways("Cannot save empty PDF file to the temporary directory.");
        return;
    }

    savePDFToTemporaryFolderAndOpenWithNativeApplicationRaw(suggestedFilename, originatingURLString, data.data(), data.size(), pdfUUID);
}

void WebPageProxy::openPDFFromTemporaryFolderWithNativeApplication(const String& pdfUUID)
{
    String pdfFilename = m_temporaryPDFFiles.get(pdfUUID);

    if (!pdfFilename.endsWith(".pdf", false))
        return;

    [[NSWorkspace sharedWorkspace] openFile:pdfFilename];
}

#if ENABLE(TELEPHONE_NUMBER_DETECTION)
void WebPageProxy::showTelephoneNumberMenu(const String& telephoneNumber, const WebCore::IntPoint& point)
{
    NSArray *menuItems = menuItemsForTelephoneNumber(telephoneNumber);

    Vector<WebContextMenuItemData> items;
    for (NSMenuItem *item in menuItems) {
        RetainPtr<NSMenuItem> retainedItem = item;
        std::function<void()> handler = [retainedItem]() {
            NSMenuItem *item = retainedItem.get();
            [[item target] performSelector:[item action] withObject:item];
        };
        
        items.append(WebContextMenuItemData(ContextMenuItem(item), handler));
    }
    
    ContextMenuContextData contextData(TelephoneNumberContext);
    internalShowContextMenu(point, contextData, items, ContextMenuClientEligibility::NotEligibleForClient, nullptr);
}
#endif

#if ENABLE(SERVICE_CONTROLS)
void WebPageProxy::showSelectionServiceMenu(const IPC::DataReference& selectionAsRTFD, const Vector<String>& telephoneNumbers, bool isEditable, const IntPoint& point)
{
    Vector<WebContextMenuItemData> items;
    ContextMenuContextData contextData(selectionAsRTFD.vector(), telephoneNumbers, isEditable);

    internalShowContextMenu(point, contextData, items, ContextMenuClientEligibility::NotEligibleForClient, nullptr);
}
#endif

CGRect WebPageProxy::boundsOfLayerInLayerBackedWindowCoordinates(CALayer *layer) const
{
    return m_pageClient.boundsOfLayerInLayerBackedWindowCoordinates(layer);
}

} // namespace WebKit

#endif // PLATFORM(MAC)
