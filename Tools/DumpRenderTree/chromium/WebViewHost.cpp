/*
 * Copyright (C) 2010, 2011, 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebViewHost.h"

#include "DRTTestRunner.h"
#include "EventSender.h"
#include "MockGrammarCheck.h"
#include "MockWebSpeechInputController.h"
#include "MockWebSpeechRecognizer.h"
#include "TestNavigationController.h"
#include "TestShell.h"
#include "TestWebPlugin.h"
#include "WebConsoleMessage.h"
#include "WebContextMenuData.h"
#include "WebDOMMessageEvent.h"
#include "WebDataSource.h"
#include "WebDeviceOrientationClientMock.h"
#include "WebDocument.h"
#include "WebElement.h"
#include "WebFrame.h"
#include "WebGeolocationClientMock.h"
#include "WebHistoryItem.h"
#include "WebIntent.h"
#include "WebIntentServiceInfo.h"
#include "WebKit.h"
#include "WebNode.h"
#include "WebPluginParams.h"
#include "WebPopupMenu.h"
#include "WebPopupType.h"
#include "WebPrintParams.h"
#include "WebRange.h"
#include "WebScreenInfo.h"
#include "WebStorageNamespace.h"
#include "WebTextCheckingCompletion.h"
#include "WebTextCheckingResult.h"
#include "WebUserMediaClientMock.h"
#include "WebView.h"
#include "WebViewHostOutputSurface.h"
#include "WebWindowFeatures.h"
#include "platform/WebSerializedScriptValue.h"
#include "skia/ext/platform_canvas.h"
#include "webkit/support/test_media_stream_client.h"
#include "webkit/support/webkit_support.h"
#include <public/WebCString.h>
#include <public/WebCompositorOutputSurface.h>
#include <public/WebDragData.h>
#include <public/WebRect.h>
#include <public/WebSize.h>
#include <public/WebThread.h>
#include <public/WebURLRequest.h>
#include <public/WebURLResponse.h>

#include <wtf/Assertions.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Vector.h>

using namespace WebCore;
using namespace WebKit;
using namespace std;

static const int screenWidth = 1920;
static const int screenHeight = 1080;
static const int screenUnavailableBorder = 8;

// WebNavigationType debugging strings taken from PolicyDelegate.mm.
static const char* linkClickedString = "link clicked";
static const char* formSubmittedString = "form submitted";
static const char* backForwardString = "back/forward";
static const char* reloadString = "reload";
static const char* formResubmittedString = "form resubmitted";
static const char* otherString = "other";
static const char* illegalString = "illegal value";

static int nextPageID = 1;

// Used to write a platform neutral file:/// URL by only taking the filename
// (e.g., converts "file:///tmp/foo.txt" to just "foo.txt").
static string urlSuitableForTestResult(const string& url)
{
    if (url.empty() || string::npos == url.find("file://"))
        return url;

    size_t pos = url.rfind('/');
    if (pos == string::npos) {
#if OS(WINDOWS)
        pos = url.rfind('\\');
        if (pos == string::npos)
            pos = 0;
#else
        pos = 0;
#endif
    }
    string filename = url.substr(pos + 1);
    if (filename.empty())
        return "file:"; // A WebKit test has this in its expected output.
    return filename;
}

// Used to write a platform neutral file:/// URL by taking the
// filename and its directory. (e.g., converts
// "file:///tmp/foo/bar.txt" to just "bar.txt").
static string descriptionSuitableForTestResult(const string& url)
{
    if (url.empty() || string::npos == url.find("file://"))
        return url;

    size_t pos = url.rfind('/');
    if (pos == string::npos || !pos)
        return "ERROR:" + url;
    pos = url.rfind('/', pos - 1);
    if (pos == string::npos)
        return "ERROR:" + url;

    return url.substr(pos + 1);
}

// Adds a file called "DRTFakeFile" to dragData (CF_HDROP). Use to fake
// dragging a file.
static void addDRTFakeFileToDataObject(WebDragData* dragData)
{
    WebDragData::Item item;
    item.storageType = WebDragData::Item::StorageTypeFilename;
    item.filenameData = WebString::fromUTF8("DRTFakeFile");
    dragData->addItem(item);
}

// Get a debugging string from a WebNavigationType.
static const char* webNavigationTypeToString(WebNavigationType type)
{
    switch (type) {
    case WebKit::WebNavigationTypeLinkClicked:
        return linkClickedString;
    case WebKit::WebNavigationTypeFormSubmitted:
        return formSubmittedString;
    case WebKit::WebNavigationTypeBackForward:
        return backForwardString;
    case WebKit::WebNavigationTypeReload:
        return reloadString;
    case WebKit::WebNavigationTypeFormResubmitted:
        return formResubmittedString;
    case WebKit::WebNavigationTypeOther:
        return otherString;
    }
    return illegalString;
}

static string URLDescription(const GURL& url)
{
    if (url.SchemeIs("file"))
        return url.ExtractFileName();
    return url.possibly_invalid_spec();
}

static void printResponseDescription(const WebURLResponse& response)
{
    if (response.isNull()) {
        fputs("(null)", stdout);
        return;
    }
    string url = response.url().spec();
    printf("<NSURLResponse %s, http status code %d>",
           descriptionSuitableForTestResult(url).c_str(),
           response.httpStatusCode());
}

static void printNodeDescription(const WebNode& node, int exception)
{
    if (exception) {
        fputs("ERROR", stdout);
        return;
    }
    if (node.isNull()) {
        fputs("(null)", stdout);
        return;
    }
    fputs(node.nodeName().utf8().data(), stdout);
    const WebNode& parent = node.parentNode();
    if (!parent.isNull()) {
        fputs(" > ", stdout);
        printNodeDescription(parent, 0);
    }
}

static void printRangeDescription(const WebRange& range)
{
    if (range.isNull()) {
        fputs("(null)", stdout);
        return;
    }
    printf("range from %d of ", range.startOffset());
    int exception = 0;
    WebNode startNode = range.startContainer(exception);
    printNodeDescription(startNode, exception);
    printf(" to %d of ", range.endOffset());
    WebNode endNode = range.endContainer(exception);
    printNodeDescription(endNode, exception);
}

static string editingActionDescription(WebEditingAction action)
{
    switch (action) {
    case WebKit::WebEditingActionTyped:
        return "WebViewInsertActionTyped";
    case WebKit::WebEditingActionPasted:
        return "WebViewInsertActionPasted";
    case WebKit::WebEditingActionDropped:
        return "WebViewInsertActionDropped";
    }
    return "(UNKNOWN ACTION)";
}

static string textAffinityDescription(WebTextAffinity affinity)
{
    switch (affinity) {
    case WebKit::WebTextAffinityUpstream:
        return "NSSelectionAffinityUpstream";
    case WebKit::WebTextAffinityDownstream:
        return "NSSelectionAffinityDownstream";
    }
    return "(UNKNOWN AFFINITY)";
}

// WebViewClient -------------------------------------------------------------

WebView* WebViewHost::createView(WebFrame* creator, const WebURLRequest& request, const WebWindowFeatures&, const WebString&, WebNavigationPolicy)
{
    if (!testRunner()->canOpenWindows())
        return 0;
    if (testRunner()->shouldDumpCreateView())
        fprintf(stdout, "createView(%s)\n", URLDescription(request.url()).c_str());
    creator->consumeUserGesture();
    return m_shell->createNewWindow(WebURL())->webView();
}

WebWidget* WebViewHost::createPopupMenu(WebPopupType type)
{
    switch (type) {
    case WebKit::WebPopupTypeNone:
    case WebKit::WebPopupTypePage:
    case WebKit::WebPopupTypeHelperPlugin:
        break;
    case WebKit::WebPopupTypeSelect:
    case WebKit::WebPopupTypeSuggestion:
        m_popupmenus.append(WebPopupMenu::create(0));
        return m_popupmenus.last();
    }
    return 0;
}

WebWidget* WebViewHost::createPopupMenu(const WebPopupMenuInfo&)
{
    // Do not use this method. It's been replaced by createExternalPopupMenu.
    ASSERT_NOT_REACHED();
    return 0;
}

WebStorageNamespace* WebViewHost::createSessionStorageNamespace(unsigned quota)
{
    return webkit_support::CreateSessionStorageNamespace(quota);
}

WebKit::WebCompositorOutputSurface* WebViewHost::createOutputSurface()
{
    if (!webView())
        return 0;
    return new WebKit::WebViewHostOutputSurface(adoptPtr(webkit_support::CreateGraphicsContext3D(WebKit::WebGraphicsContext3D::Attributes(), webView())));
}

void WebViewHost::didAddMessageToConsole(const WebConsoleMessage& message, const WebString& sourceName, unsigned sourceLine)
{
    // This matches win DumpRenderTree's UIDelegate.cpp.
    if (!m_logConsoleOutput)
        return;
    string newMessage;
    if (!message.text.isEmpty()) {
        newMessage = message.text.utf8();
        size_t fileProtocol = newMessage.find("file://");
        if (fileProtocol != string::npos) {
            newMessage = newMessage.substr(0, fileProtocol)
                + urlSuitableForTestResult(newMessage.substr(fileProtocol));
        }
    }
    printf("CONSOLE MESSAGE: ");
    if (sourceLine)
        printf("line %d: ", sourceLine);
    printf("%s\n", newMessage.data());
}

void WebViewHost::didStartLoading()
{
    m_shell->setIsLoading(true);
}

void WebViewHost::didStopLoading()
{
    if (testRunner()->shouldDumpProgressFinishedCallback())
        fputs("postProgressFinishedNotification\n", stdout);
    m_shell->setIsLoading(false);
}

// The output from these methods in layout test mode should match that
// expected by the layout tests. See EditingDelegate.m in DumpRenderTree.

bool WebViewHost::shouldBeginEditing(const WebRange& range)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        fputs("EDITING DELEGATE: shouldBeginEditingInDOMRange:", stdout);
        printRangeDescription(range);
        fputs("\n", stdout);
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::shouldEndEditing(const WebRange& range)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        fputs("EDITING DELEGATE: shouldEndEditingInDOMRange:", stdout);
        printRangeDescription(range);
        fputs("\n", stdout);
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::shouldInsertNode(const WebNode& node, const WebRange& range, WebEditingAction action)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        fputs("EDITING DELEGATE: shouldInsertNode:", stdout);
        printNodeDescription(node, 0);
        fputs(" replacingDOMRange:", stdout);
        printRangeDescription(range);
        printf(" givenAction:%s\n", editingActionDescription(action).c_str());
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::shouldInsertText(const WebString& text, const WebRange& range, WebEditingAction action)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldInsertText:%s replacingDOMRange:", text.utf8().data());
        printRangeDescription(range);
        printf(" givenAction:%s\n", editingActionDescription(action).c_str());
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::shouldChangeSelectedRange(
    const WebRange& fromRange, const WebRange& toRange, WebTextAffinity affinity, bool stillSelecting)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        fputs("EDITING DELEGATE: shouldChangeSelectedDOMRange:", stdout);
        printRangeDescription(fromRange);
        fputs(" toDOMRange:", stdout);
        printRangeDescription(toRange);
        printf(" affinity:%s stillSelecting:%s\n",
               textAffinityDescription(affinity).c_str(),
               (stillSelecting ? "TRUE" : "FALSE"));
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::shouldDeleteRange(const WebRange& range)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        fputs("EDITING DELEGATE: shouldDeleteDOMRange:", stdout);
        printRangeDescription(range);
        fputs("\n", stdout);
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::shouldApplyStyle(const WebString& style, const WebRange& range)
{
    if (testRunner()->shouldDumpEditingCallbacks()) {
        printf("EDITING DELEGATE: shouldApplyStyle:%s toElementsInDOMRange:", style.utf8().data());
        printRangeDescription(range);
        fputs("\n", stdout);
    }
    return testRunner()->acceptsEditing();
}

bool WebViewHost::isSmartInsertDeleteEnabled()
{
    return m_smartInsertDeleteEnabled;
}

bool WebViewHost::isSelectTrailingWhitespaceEnabled()
{
    return m_selectTrailingWhitespaceEnabled;
}

void WebViewHost::didBeginEditing()
{
    if (!testRunner()->shouldDumpEditingCallbacks())
        return;
    fputs("EDITING DELEGATE: webViewDidBeginEditing:WebViewDidBeginEditingNotification\n", stdout);
}

void WebViewHost::didChangeSelection(bool isEmptySelection)
{
    if (testRunner()->shouldDumpEditingCallbacks())
        fputs("EDITING DELEGATE: webViewDidChangeSelection:WebViewDidChangeSelectionNotification\n", stdout);
    // No need to update clipboard with the selected text in DRT.
}

void WebViewHost::didChangeContents()
{
    if (!testRunner()->shouldDumpEditingCallbacks())
        return;
    fputs("EDITING DELEGATE: webViewDidChange:WebViewDidChangeNotification\n", stdout);
}

void WebViewHost::didEndEditing()
{
    if (!testRunner()->shouldDumpEditingCallbacks())
        return;
    fputs("EDITING DELEGATE: webViewDidEndEditing:WebViewDidEndEditingNotification\n", stdout);
}

bool WebViewHost::handleCurrentKeyboardEvent()
{
    if (m_editCommandName.empty())
        return false;
    WebFrame* frame = webView()->focusedFrame();
    if (!frame)
        return false;

    return frame->executeCommand(WebString::fromUTF8(m_editCommandName), WebString::fromUTF8(m_editCommandValue));
}

// WebKit::WebPrerendererClient

void WebViewHost::willAddPrerender(WebKit::WebPrerender*)
{
}


// WebKit::WebSpellCheckClient

void WebViewHost::spellCheck(const WebString& text, int& misspelledOffset, int& misspelledLength, WebVector<WebString>* optionalSuggestions)
{
    // Check the spelling of the given text.
    m_spellcheck.spellCheckWord(text, &misspelledOffset, &misspelledLength);
}

void WebViewHost::checkTextOfParagraph(const WebString& text, WebTextCheckingTypeMask mask, WebVector<WebTextCheckingResult>* webResults)
{
    Vector<WebTextCheckingResult> results;
    if (mask & WebTextCheckingTypeSpelling) {
        size_t offset = 0;
        size_t length = text.length();
        const WebUChar* data = text.data();
        while (offset < length) {
            int misspelledPosition = 0;
            int misspelledLength = 0;
            m_spellcheck.spellCheckWord(WebString(&data[offset], length - offset), &misspelledPosition, &misspelledLength);
            if (!misspelledLength)
                break;
            WebTextCheckingResult result;
            result.type = WebTextCheckingTypeSpelling;
            result.location = offset + misspelledPosition;
            result.length = misspelledLength;
            results.append(result);
            offset += misspelledPosition + misspelledLength;
        }
    }
    if (mask & WebTextCheckingTypeGrammar)
        MockGrammarCheck::checkGrammarOfString(text, &results);
    webResults->assign(results);
}

void WebViewHost::requestCheckingOfText(const WebString& text, WebTextCheckingCompletion* completion)
{
    if (text.isEmpty()) {
        if (completion)
            completion->didCancelCheckingText();
        return;
    }

    m_lastRequestedTextCheckingCompletion = completion;
    m_lastRequestedTextCheckString = text;
    postDelayedTask(new HostMethodTask(this, &WebViewHost::finishLastTextCheck), 0);
}

void WebViewHost::finishLastTextCheck()
{
    Vector<WebTextCheckingResult> results;
    int offset = 0;
    String text(m_lastRequestedTextCheckString.data(), m_lastRequestedTextCheckString.length());
    while (text.length()) {
        int misspelledPosition = 0;
        int misspelledLength = 0;
        m_spellcheck.spellCheckWord(WebString(text.characters(), text.length()), &misspelledPosition, &misspelledLength);
        if (!misspelledLength)
            break;
        Vector<WebString> suggestions;
        m_spellcheck.fillSuggestionList(WebString(text.characters() + misspelledPosition, misspelledLength), &suggestions);
        results.append(WebTextCheckingResult(WebTextCheckingTypeSpelling, offset + misspelledPosition, misspelledLength,
                                             suggestions.isEmpty() ? WebString() : suggestions[0]));
        text = text.substring(misspelledPosition + misspelledLength);
        offset += misspelledPosition + misspelledLength;
    }
    MockGrammarCheck::checkGrammarOfString(m_lastRequestedTextCheckString, &results);
    m_lastRequestedTextCheckingCompletion->didFinishCheckingText(results);
    m_lastRequestedTextCheckingCompletion = 0;
}


WebString WebViewHost::autoCorrectWord(const WebString&)
{
    // Returns an empty string as Mac WebKit ('WebKitSupport/WebEditorClient.mm')
    // does. (If this function returns a non-empty string, WebKit replaces the
    // given misspelled string with the result one. This process executes some
    // editor commands and causes layout-test failures.)
    return WebString();
}

void WebViewHost::runModalAlertDialog(WebFrame*, const WebString& message)
{
    printf("ALERT: %s\n", message.utf8().data());
}

bool WebViewHost::runModalConfirmDialog(WebFrame*, const WebString& message)
{
    printf("CONFIRM: %s\n", message.utf8().data());
    return true;
}

bool WebViewHost::runModalPromptDialog(WebFrame* frame, const WebString& message,
                                       const WebString& defaultValue, WebString*)
{
    printf("PROMPT: %s, default text: %s\n", message.utf8().data(), defaultValue.utf8().data());
    return true;
}

bool WebViewHost::runModalBeforeUnloadDialog(WebFrame*, const WebString& message)
{
    printf("CONFIRM NAVIGATION: %s\n", message.utf8().data());
    return !testRunner()->shouldStayOnPageAfterHandlingBeforeUnload();
}

void WebViewHost::showContextMenu(WebFrame*, const WebContextMenuData& contextMenuData)
{
    m_lastContextMenuData = adoptPtr(new WebContextMenuData(contextMenuData));
}

void WebViewHost::clearContextMenuData()
{
    m_lastContextMenuData.clear();
}

WebContextMenuData* WebViewHost::lastContextMenuData() const
{
    return m_lastContextMenuData.get();
}

void WebViewHost::setStatusText(const WebString& text)
{
    if (!testRunner()->shouldDumpStatusCallbacks())
        return;
    // When running tests, write to stdout.
    printf("UI DELEGATE STATUS CALLBACK: setStatusText:%s\n", text.utf8().data());
}

void WebViewHost::startDragging(WebFrame*, const WebDragData& data, WebDragOperationsMask mask, const WebImage&, const WebPoint&)
{
    WebDragData mutableDragData = data;
    if (testRunner()->shouldAddFileToPasteboard()) {
        // Add a file called DRTFakeFile to the drag&drop clipboard.
        addDRTFakeFileToDataObject(&mutableDragData);
    }

    // When running a test, we need to fake a drag drop operation otherwise
    // Windows waits for real mouse events to know when the drag is over.
    m_shell->eventSender()->doDragDrop(mutableDragData, mask);
}

void WebViewHost::didUpdateLayout()
{
#if OS(MAC_OS_X)
    static bool queryingPreferredSize = false;
    if (queryingPreferredSize)
        return;

    queryingPreferredSize = true;
    // Query preferred width to emulate the same functionality in Chromium:
    // see RenderView::CheckPreferredSize (src/content/renderer/render_view.cc)
    // and TabContentsViewMac::RenderViewCreated (src/chrome/browser/tab_contents/tab_contents_view_mac.mm)
    webView()->mainFrame()->contentsPreferredWidth();
    webView()->mainFrame()->documentElementScrollHeight();
    queryingPreferredSize = false;
#endif
}

void WebViewHost::navigateBackForwardSoon(int offset)
{
    navigationController()->goToOffset(offset);
}

int WebViewHost::historyBackListCount()
{
    return navigationController()->lastCommittedEntryIndex();
}

int WebViewHost::historyForwardListCount()
{
    int currentIndex =navigationController()->lastCommittedEntryIndex();
    return navigationController()->entryCount() - currentIndex - 1;
}

void WebViewHost::postAccessibilityNotification(const WebAccessibilityObject& obj, WebAccessibilityNotification notification)
{
    if (notification == WebAccessibilityNotificationFocusedUIElementChanged)
        m_shell->accessibilityController()->setFocusedElement(obj);

    const char* notificationName;
    switch (notification) {
    case WebAccessibilityNotificationActiveDescendantChanged:
        notificationName = "ActiveDescendantChanged";
        break;
    case WebAccessibilityNotificationAutocorrectionOccured:
        notificationName = "AutocorrectionOccured";
        break;
    case WebAccessibilityNotificationCheckedStateChanged:
        notificationName = "CheckedStateChanged";
        break;
    case WebAccessibilityNotificationChildrenChanged:
        notificationName = "ChildrenChanged";
        break;
    case WebAccessibilityNotificationFocusedUIElementChanged:
        notificationName = "FocusedUIElementChanged";
        break;
    case WebAccessibilityNotificationLayoutComplete:
        notificationName = "LayoutComplete";
        break;
    case WebAccessibilityNotificationLoadComplete:
        notificationName = "LoadComplete";
        break;
    case WebAccessibilityNotificationSelectedChildrenChanged:
        notificationName = "SelectedChildrenChanged";
        break;
    case WebAccessibilityNotificationSelectedTextChanged:
        notificationName = "SelectedTextChanged";
        break;
    case WebAccessibilityNotificationValueChanged:
        notificationName = "ValueChanged";
        break;
    case WebAccessibilityNotificationScrolledToAnchor:
        notificationName = "ScrolledToAnchor";
        break;
    case WebAccessibilityNotificationLiveRegionChanged:
        notificationName = "LiveRegionChanged";
        break;
    case WebAccessibilityNotificationMenuListItemSelected:
        notificationName = "MenuListItemSelected";
        break;
    case WebAccessibilityNotificationMenuListValueChanged:
        notificationName = "MenuListValueChanged";
        break;
    case WebAccessibilityNotificationRowCountChanged:
        notificationName = "RowCountChanged";
        break;
    case WebAccessibilityNotificationRowCollapsed:
        notificationName = "RowCollapsed";
        break;
    case WebAccessibilityNotificationRowExpanded:
        notificationName = "RowExpanded";
        break;
    case WebAccessibilityNotificationInvalidStatusChanged:
        notificationName = "InvalidStatusChanged";
        break;
    default:
        notificationName = "UnknownNotification";
        break;
    }

    m_shell->accessibilityController()->notificationReceived(obj, notificationName);

    if (m_shell->accessibilityController()->shouldLogAccessibilityEvents()) {
        printf("AccessibilityNotification - %s", notificationName);

        WebKit::WebNode node = obj.node();
        if (!node.isNull() && node.isElementNode()) {
            WebKit::WebElement element = node.to<WebKit::WebElement>();
            if (element.hasAttribute("id"))
                printf(" - id:%s", element.getAttribute("id").utf8().data());
        }

        printf("\n");
    }
}

#if ENABLE(NOTIFICATIONS)
WebNotificationPresenter* WebViewHost::notificationPresenter()
{
    return m_shell->notificationPresenter();
}
#endif

WebKit::WebGeolocationClient* WebViewHost::geolocationClient()
{
    return geolocationClientMock();
}

WebKit::WebGeolocationClientMock* WebViewHost::geolocationClientMock()
{
    if (!m_geolocationClientMock)
        m_geolocationClientMock = adoptPtr(WebGeolocationClientMock::create());
    return m_geolocationClientMock.get();
}

#if ENABLE(INPUT_SPEECH)
WebSpeechInputController* WebViewHost::speechInputController(WebKit::WebSpeechInputListener* listener)
{
    if (!m_speechInputControllerMock)
        m_speechInputControllerMock = MockWebSpeechInputController::create(listener);
    return m_speechInputControllerMock.get();
}
#endif

#if ENABLE(SCRIPTED_SPEECH)
WebSpeechRecognizer* WebViewHost::speechRecognizer()
{
    if (!m_mockSpeechRecognizer)
        m_mockSpeechRecognizer = MockWebSpeechRecognizer::create();
    return m_mockSpeechRecognizer.get();
}
#endif

WebDeviceOrientationClientMock* WebViewHost::deviceOrientationClientMock()
{
    if (!m_deviceOrientationClientMock.get())
        m_deviceOrientationClientMock = adoptPtr(WebDeviceOrientationClientMock::create());
    return m_deviceOrientationClientMock.get();
}

MockSpellCheck* WebViewHost::mockSpellCheck()
{
    return &m_spellcheck;
}

void WebViewHost::fillSpellingSuggestionList(const WebKit::WebString& word, Vector<WebKit::WebString>* suggestions)
{
    mockSpellCheck()->fillSuggestionList(word, suggestions);
}

WebDeviceOrientationClient* WebViewHost::deviceOrientationClient()
{
    return deviceOrientationClientMock();
}

#if ENABLE(MEDIA_STREAM)
WebUserMediaClient* WebViewHost::userMediaClient()
{
    return userMediaClientMock();
}

WebUserMediaClientMock* WebViewHost::userMediaClientMock()
{
    if (!m_userMediaClientMock.get())
        m_userMediaClientMock = WebUserMediaClientMock::create();
    return m_userMediaClientMock.get();
}
#endif

// WebWidgetClient -----------------------------------------------------------

void WebViewHost::didInvalidateRect(const WebRect& rect)
{
    updatePaintRect(rect);
}

void WebViewHost::didScrollRect(int, int, const WebRect& clipRect)
{
    // This is used for optimizing painting when the renderer is scrolled. We're
    // currently not doing any optimizations so just invalidate the region.
    didInvalidateRect(clipRect);
}

void WebViewHost::didAutoResize(const WebSize& newSize)
{
    // Purposely don't include the virtualWindowBorder in this case so that
    // window.inner[Width|Height] is the same as window.outer[Width|Height]
    setWindowRect(WebRect(0, 0, newSize.width, newSize.height));
}

void WebViewHost::scheduleComposite()
{
    WebSize widgetSize = webWidget()->size();
    WebRect clientRect(0, 0, widgetSize.width, widgetSize.height);
    didInvalidateRect(clientRect);
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
void WebViewHost::serviceAnimation()
{
    if (webView()->settings()->scrollAnimatorEnabled())
        webView()->animate(0.0);
    scheduleComposite();
}

void WebViewHost::scheduleAnimation()
{
    postDelayedTask(new HostMethodTask(this, &WebViewHost::serviceAnimation), 0);
}
#endif

void WebViewHost::didFocus()
{
    m_shell->setFocus(webWidget(), true);
}

void WebViewHost::didBlur()
{
    m_shell->setFocus(webWidget(), false);
}

WebScreenInfo WebViewHost::screenInfo()
{
    // We don't need to set actual values.
    WebScreenInfo info;
    info.depth = 24;
    info.depthPerComponent = 8;
    info.isMonochrome = false;
    info.rect = WebRect(0, 0, screenWidth, screenHeight);
    // Use values different from info.rect for testing.
    info.availableRect = WebRect(screenUnavailableBorder, screenUnavailableBorder,
                                 screenWidth - screenUnavailableBorder * 2,
                                 screenHeight - screenUnavailableBorder * 2);
    return info;
}

#if ENABLE(POINTER_LOCK)
bool WebViewHost::requestPointerLock()
{
    switch (m_pointerLockPlannedResult) {
    case PointerLockWillSucceed:
        postDelayedTask(new HostMethodTask(this, &WebViewHost::didAcquirePointerLock), 0);
        return true;
    case PointerLockWillRespondAsync:
        ASSERT(!m_pointerLocked);
        return true;
    case PointerLockWillFailSync:
        ASSERT(!m_pointerLocked);
        return false;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }
}

void WebViewHost::requestPointerUnlock()
{
    postDelayedTask(new HostMethodTask(this, &WebViewHost::didLosePointerLock), 0);
}

bool WebViewHost::isPointerLocked()
{
    return m_pointerLocked;
}

void WebViewHost::didAcquirePointerLock()
{
    m_pointerLocked = true;
    webWidget()->didAcquirePointerLock();

    // Reset planned result to default.
    m_pointerLockPlannedResult = PointerLockWillSucceed;
}

void WebViewHost::didNotAcquirePointerLock()
{
    ASSERT(!m_pointerLocked);
    m_pointerLocked = false;
    webWidget()->didNotAcquirePointerLock();

    // Reset planned result to default.
    m_pointerLockPlannedResult = PointerLockWillSucceed;
}

void WebViewHost::didLosePointerLock()
{
    bool wasLocked = m_pointerLocked;
    m_pointerLocked = false;
    if (wasLocked)
        webWidget()->didLosePointerLock();
}
#endif

void WebViewHost::show(WebNavigationPolicy)
{
    m_hasWindow = true;
    WebSize size = webWidget()->size();
    updatePaintRect(WebRect(0, 0, size.width, size.height));
}



void WebViewHost::closeWidget()
{
    m_hasWindow = false;
    m_shell->closeWindow(this);
    // No more code here, we should be deleted at this point.
}

void WebViewHost::closeWidgetSoon()
{
    postDelayedTask(new HostMethodTask(this, &WebViewHost::closeWidget), 0);
}

void WebViewHost::didChangeCursor(const WebCursorInfo& cursorInfo)
{
    if (!hasWindow())
        return;
    m_currentCursor = cursorInfo;
}

WebRect WebViewHost::windowRect()
{
    return m_windowRect;
}

void WebViewHost::setWindowRect(const WebRect& rect)
{
    m_windowRect = rect;
    const int border2 = TestShell::virtualWindowBorder * 2;
    if (m_windowRect.width <= border2)
        m_windowRect.width = 1 + border2;
    if (m_windowRect.height <= border2)
        m_windowRect.height = 1 + border2;
    int width = m_windowRect.width - border2;
    int height = m_windowRect.height - border2;
    discardBackingStore();
    webWidget()->resize(WebSize(width, height));
    updatePaintRect(WebRect(0, 0, width, height));
}

WebRect WebViewHost::rootWindowRect()
{
    return windowRect();
}

WebRect WebViewHost::windowResizerRect()
{
    // Not necessary.
    return WebRect();
}

void WebViewHost::runModal()
{
    if (m_shell->isDisplayingModalDialog()) {
        // DumpRenderTree doesn't support real modal dialogs, so a test shouldn't try to start two modal dialogs at the same time.
        ASSERT_NOT_REACHED();
        return;
    }
    // This WebViewHost might get deleted before RunMessageLoop() returns, so keep a copy of the m_shell member variable around.
    ASSERT(m_shell->webViewHost() != this);
    TestShell* shell = m_shell;
    shell->setIsDisplayingModalDialog(true);
    bool oldState = webkit_support::MessageLoopNestableTasksAllowed();
    webkit_support::MessageLoopSetNestableTasksAllowed(true);
    m_inModalLoop = true;
    webkit_support::RunMessageLoop();
    webkit_support::MessageLoopSetNestableTasksAllowed(oldState);
    shell->setIsDisplayingModalDialog(false);
}

bool WebViewHost::enterFullScreen()
{
    postDelayedTask(new HostMethodTask(this, &WebViewHost::enterFullScreenNow), 0);
    return true;
}

void WebViewHost::exitFullScreen()
{
    postDelayedTask(new HostMethodTask(this, &WebViewHost::exitFullScreenNow), 0);
}

// WebFrameClient ------------------------------------------------------------

WebPlugin* WebViewHost::createPlugin(WebFrame* frame, const WebPluginParams& params)
{
    if (params.mimeType == TestWebPlugin::mimeType())
        return new TestWebPlugin(frame, params);

    return webkit_support::CreateWebPlugin(frame, params);
}

WebMediaPlayer* WebViewHost::createMediaPlayer(WebFrame* frame, const WebURL& url, WebMediaPlayerClient* client)
{
#if ENABLE(MEDIA_STREAM)
    return webkit_support::CreateMediaPlayer(frame, url, client, testMediaStreamClient());
#else
    return webkit_support::CreateMediaPlayer(frame, url, client);
#endif
}

WebApplicationCacheHost* WebViewHost::createApplicationCacheHost(WebFrame* frame, WebApplicationCacheHostClient* client)
{
    return webkit_support::CreateApplicationCacheHost(frame, client);
}

void WebViewHost::loadURLExternally(WebFrame* frame, const WebURLRequest& request, WebNavigationPolicy policy)
{
    loadURLExternally(frame, request, policy, WebString());
}

void WebViewHost::loadURLExternally(WebFrame*, const WebURLRequest& request, WebNavigationPolicy policy, const WebString& downloadName)
{
    ASSERT(policy !=  WebKit::WebNavigationPolicyCurrentTab);
    WebViewHost* another = m_shell->createNewWindow(request.url());
    if (another)
        another->show(policy);
}

WebNavigationPolicy WebViewHost::decidePolicyForNavigation(
    WebFrame*, const WebURLRequest& request,
    WebNavigationType type, const WebNode& originatingNode,
    WebNavigationPolicy defaultPolicy, bool isRedirect)
{
    WebNavigationPolicy result;
    if (!m_policyDelegateEnabled)
        return defaultPolicy;

    printf("Policy delegate: attempt to load %s with navigation type '%s'",
           URLDescription(request.url()).c_str(), webNavigationTypeToString(type));
    if (!originatingNode.isNull()) {
        fputs(" originating from ", stdout);
        printNodeDescription(originatingNode, 0);
    }
    fputs("\n", stdout);
    if (m_policyDelegateIsPermissive)
        result = WebKit::WebNavigationPolicyCurrentTab;
    else
        result = WebKit::WebNavigationPolicyIgnore;

    if (m_policyDelegateShouldNotifyDone)
        testRunner()->policyDelegateDone();
    return result;
}

bool WebViewHost::canHandleRequest(WebFrame*, const WebURLRequest& request)
{
    GURL url = request.url();
    // Just reject the scheme used in
    // LayoutTests/http/tests/misc/redirect-to-external-url.html
    return !url.SchemeIs("spaceballs");
}

WebURLError WebViewHost::cannotHandleRequestError(WebFrame*, const WebURLRequest& request)
{
    WebURLError error;
    // A WebKit layout test expects the following values.
    // unableToImplementPolicyWithError() below prints them.
    error.domain = WebString::fromUTF8("WebKitErrorDomain");
    error.reason = 101;
    error.unreachableURL = request.url();
    return error;
}

WebURLError WebViewHost::cancelledError(WebFrame*, const WebURLRequest& request)
{
    return webkit_support::CreateCancelledError(request);
}

void WebViewHost::unableToImplementPolicyWithError(WebFrame* frame, const WebURLError& error)
{
    printf("Policy delegate: unable to implement policy with error domain '%s', "
           "error code %d, in frame '%s'\n",
           error.domain.utf8().data(), error.reason, frame->name().utf8().data());
}

void WebViewHost::willPerformClientRedirect(WebFrame* frame, const WebURL& from, const WebURL& to,
                                            double interval, double fire_time)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        printf(" - willPerformClientRedirectToURL: %s \n", to.spec().data());
    }

    if (m_shell->shouldDumpUserGestureInFrameLoadCallbacks())
        printFrameUserGestureStatus(frame, " - in willPerformClientRedirect\n");
}

void WebViewHost::didCancelClientRedirect(WebFrame* frame)
{
    if (!m_shell->shouldDumpFrameLoadCallbacks())
        return;
    printFrameDescription(frame);
    fputs(" - didCancelClientRedirectForFrame\n", stdout);
}

void WebViewHost::didCreateDataSource(WebFrame*, WebDataSource* ds)
{
    ds->setExtraData(m_pendingExtraData.leakPtr());
    if (!testRunner()->deferMainResourceDataLoad())
        ds->setDeferMainResourceDataLoad(false);
}

void WebViewHost::didStartProvisionalLoad(WebFrame* frame)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didStartProvisionalLoadForFrame\n", stdout);
    }

    if (m_shell->shouldDumpUserGestureInFrameLoadCallbacks())
        printFrameUserGestureStatus(frame, " - in didStartProvisionalLoadForFrame\n");

    if (!m_topLoadingFrame)
        m_topLoadingFrame = frame;

    if (testRunner()->stopProvisionalFrameLoads()) {
        printFrameDescription(frame);
        fputs(" - stopping load in didStartProvisionalLoadForFrame callback\n", stdout);
        frame->stopLoading();
    }
    updateAddressBar(frame->view());
}

void WebViewHost::didReceiveServerRedirectForProvisionalLoad(WebFrame* frame)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didReceiveServerRedirectForProvisionalLoadForFrame\n", stdout);
    }
    updateAddressBar(frame->view());
}

void WebViewHost::didFailProvisionalLoad(WebFrame* frame, const WebURLError& error)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didFailProvisionalLoadWithError\n", stdout);
    }

    locationChangeDone(frame);

    // Don't display an error page if we're running layout tests, because
    // DumpRenderTree doesn't.
}

void WebViewHost::didCommitProvisionalLoad(WebFrame* frame, bool isNewNavigation)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didCommitLoadForFrame\n", stdout);
    }
    updateForCommittedLoad(frame, isNewNavigation);
}

void WebViewHost::didClearWindowObject(WebFrame* frame)
{
    m_shell->bindJSObjectsToWindow(frame);
}

void WebViewHost::didReceiveTitle(WebFrame* frame, const WebString& title, WebTextDirection direction)
{
    WebCString title8 = title.utf8();

    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        printf(" - didReceiveTitle: %s\n", title8.data());
    }

    if (testRunner()->shouldDumpTitleChanges())
        printf("TITLE CHANGED: '%s'\n", title8.data());

    setPageTitle(title);
    testRunner()->setTitleTextDirection(direction);
}

void WebViewHost::didFinishDocumentLoad(WebFrame* frame)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didFinishDocumentLoadForFrame\n", stdout);
    } else {
        unsigned pendingUnloadEvents = frame->unloadListenerCount();
        if (pendingUnloadEvents) {
            printFrameDescription(frame);
            printf(" - has %u onunload handler(s)\n", pendingUnloadEvents);
        }
    }
}

void WebViewHost::didHandleOnloadEvents(WebFrame* frame)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didHandleOnloadEventsForFrame\n", stdout);
    }
}

void WebViewHost::didFailLoad(WebFrame* frame, const WebURLError& error)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didFailLoadWithError\n", stdout);
    }
    locationChangeDone(frame);
}

void WebViewHost::didFinishLoad(WebFrame* frame)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didFinishLoadForFrame\n", stdout);
    }
    updateAddressBar(frame->view());
    locationChangeDone(frame);
}

void WebViewHost::didNavigateWithinPage(WebFrame* frame, bool isNewNavigation)
{
    frame->dataSource()->setExtraData(m_pendingExtraData.leakPtr());

    updateForCommittedLoad(frame, isNewNavigation);
}

void WebViewHost::didChangeLocationWithinPage(WebFrame* frame)
{
    if (m_shell->shouldDumpFrameLoadCallbacks()) {
        printFrameDescription(frame);
        fputs(" - didChangeLocationWithinPageForFrame\n", stdout);
    }
}

void WebViewHost::assignIdentifierToRequest(WebFrame*, unsigned identifier, const WebURLRequest& request)
{
     if (!m_shell->shouldDumpResourceLoadCallbacks())
        return;
    ASSERT(!m_resourceIdentifierMap.contains(identifier));
    m_resourceIdentifierMap.set(identifier, descriptionSuitableForTestResult(request.url().spec()));
}

void WebViewHost::removeIdentifierForRequest(unsigned identifier)
{
    m_resourceIdentifierMap.remove(identifier);
}

static void blockRequest(WebURLRequest& request)
{
    request.setURL(WebURL());
}

static bool isLocalhost(const string& host)
{
    return host == "127.0.0.1" || host == "localhost";
}

static bool hostIsUsedBySomeTestsToGenerateError(const string& host)
{
    return host == "255.255.255.255";
}

void WebViewHost::willSendRequest(WebFrame* frame, unsigned identifier, WebURLRequest& request, const WebURLResponse& redirectResponse)
{
    // Need to use GURL for host() and SchemeIs()
    GURL url = request.url();
    string requestURL = url.possibly_invalid_spec();

    GURL mainDocumentURL = request.firstPartyForCookies();
    if (testRunner()->shouldDumpResourceLoadCallbacks()) {
        printResourceDescription(identifier);
        printf(" - willSendRequest <NSURLRequest URL %s, main document URL %s,"
               " http method %s> redirectResponse ",
               descriptionSuitableForTestResult(requestURL).c_str(),
               URLDescription(mainDocumentURL).c_str(),
               request.httpMethod().utf8().data());
        printResponseDescription(redirectResponse);
        fputs("\n", stdout);
    }

    request.setExtraData(webkit_support::CreateWebURLRequestExtraData(frame->document().referrerPolicy()));

    if (!redirectResponse.isNull() && m_blocksRedirects) {
        fputs("Returning null for this redirect\n", stdout);
        blockRequest(request);
        return;
    }

    if (m_requestReturnNull) {
        blockRequest(request);
        return;
    }

    string host = url.host();
    if (!host.empty() && (url.SchemeIs("http") || url.SchemeIs("https"))) {
        if (!isLocalhost(host) && !hostIsUsedBySomeTestsToGenerateError(host)
            && ((!mainDocumentURL.SchemeIs("http") && !mainDocumentURL.SchemeIs("https")) || isLocalhost(mainDocumentURL.host()))
            && !m_shell->allowExternalPages()) {
            printf("Blocked access to external URL %s\n", requestURL.c_str());
            blockRequest(request);
            return;
        }
    }

    HashSet<String>::const_iterator end = m_clearHeaders.end();
    for (HashSet<String>::const_iterator header = m_clearHeaders.begin(); header != end; ++header)
        request.clearHTTPHeaderField(WebString(header->characters(), header->length()));

    // Set the new substituted URL.
    request.setURL(webkit_support::RewriteLayoutTestsURL(request.url().spec()));
}

void WebViewHost::didReceiveResponse(WebFrame*, unsigned identifier, const WebURLResponse& response)
{
    if (m_shell->shouldDumpResourceLoadCallbacks()) {
        printResourceDescription(identifier);
        fputs(" - didReceiveResponse ", stdout);
        printResponseDescription(response);
        fputs("\n", stdout);
    }
    if (m_shell->shouldDumpResourceResponseMIMETypes()) {
        GURL url = response.url();
        WebString mimeType = response.mimeType();
        printf("%s has MIME type %s\n",
            url.ExtractFileName().c_str(),
            // Simulate NSURLResponse's mapping of empty/unknown MIME types to application/octet-stream
            mimeType.isEmpty() ? "application/octet-stream" : mimeType.utf8().data());
    }
}

void WebViewHost::didFinishResourceLoad(WebFrame*, unsigned identifier)
{
    if (m_shell->shouldDumpResourceLoadCallbacks()) {
        printResourceDescription(identifier);
        fputs(" - didFinishLoading\n", stdout);
    }
    removeIdentifierForRequest(identifier);
}

void WebViewHost::didFailResourceLoad(WebFrame*, unsigned identifier, const WebURLError& error)
{
    if (m_shell->shouldDumpResourceLoadCallbacks()) {
        printResourceDescription(identifier);
        fputs(" - didFailLoadingWithError: ", stdout);
        fputs(webkit_support::MakeURLErrorDescription(error).c_str(), stdout);
        fputs("\n", stdout);
    }
    removeIdentifierForRequest(identifier);
}

void WebViewHost::didDisplayInsecureContent(WebFrame*)
{
    if (m_shell->shouldDumpFrameLoadCallbacks())
        fputs("didDisplayInsecureContent\n", stdout);
}

void WebViewHost::didRunInsecureContent(WebFrame*, const WebSecurityOrigin& origin, const WebURL& insecureURL)
{
    if (m_shell->shouldDumpFrameLoadCallbacks())
        fputs("didRunInsecureContent\n", stdout);
}

void WebViewHost::didDetectXSS(WebFrame*, const WebURL&, bool)
{
    if (m_shell->shouldDumpFrameLoadCallbacks())
        fputs("didDetectXSS\n", stdout);
}

void WebViewHost::openFileSystem(WebFrame* frame, WebFileSystem::Type type, long long size, bool create, WebFileSystemCallbacks* callbacks)
{
    webkit_support::OpenFileSystem(frame, type, size, create, callbacks);
}

void WebViewHost::deleteFileSystem(WebKit::WebFrame* frame, WebKit::WebFileSystem::Type type, WebKit::WebFileSystemCallbacks* callbacks)
{
    webkit_support::DeleteFileSystem(frame, type, callbacks);
}

bool WebViewHost::willCheckAndDispatchMessageEvent(WebFrame* source, WebSecurityOrigin target, WebDOMMessageEvent event)
{
    if (m_shell->testRunner()->shouldInterceptPostMessage()) {
        fputs("intercepted postMessage\n", stdout);
        return true;
    }

    return false;
}

void WebViewHost::registerIntentService(WebKit::WebFrame*, const WebKit::WebIntentServiceInfo& service)
{
    printf("Registered Web Intent Service: action=%s type=%s title=%s url=%s disposition=%s\n",
           service.action().utf8().data(), service.type().utf8().data(), service.title().utf8().data(), service.url().spec().data(), service.disposition().utf8().data());
}

void WebViewHost::dispatchIntent(WebFrame* source, const WebIntentRequest& request)
{
    printf("Received Web Intent: action=%s type=%s\n",
           request.intent().action().utf8().data(),
           request.intent().type().utf8().data());
    WebMessagePortChannelArray* ports = request.intent().messagePortChannelsRelease();
    m_currentRequest = request;
    if (ports) {
        printf("Have %d ports\n", static_cast<int>(ports->size()));
        for (size_t i = 0; i < ports->size(); ++i)
            (*ports)[i]->destroy();
        delete ports;
    }

    if (!request.intent().service().isEmpty())
        printf("Explicit intent service: %s\n", request.intent().service().spec().data());

    WebVector<WebString> extras = request.intent().extrasNames();
    for (size_t i = 0; i < extras.size(); ++i) {
        printf("Extras[%s] = %s\n", extras[i].utf8().data(),
               request.intent().extrasValue(extras[i]).utf8().data());
    }

    WebVector<WebURL> suggestions = request.intent().suggestions();
    for (size_t i = 0; i < suggestions.size(); ++i)
        printf("Have suggestion %s\n", suggestions[i].spec().data());
}

void WebViewHost::deliveredIntentResult(WebFrame* frame, int id, const WebSerializedScriptValue& data)
{
    printf("Web intent success for id %d\n", id);
}

void WebViewHost::deliveredIntentFailure(WebFrame* frame, int id, const WebSerializedScriptValue& data)
{
    printf("Web intent failure for id %d\n", id);
}

// Public functions -----------------------------------------------------------

WebViewHost::WebViewHost(TestShell* shell)
    : m_shell(shell)
    , m_webWidget(0)
    , m_lastRequestedTextCheckingCompletion(0)
{
    reset();
}

WebViewHost::~WebViewHost()
{
    // DevTools frontend page is supposed to be navigated only once and
    // loading another URL in that Page is an error.
    if (m_shell->devToolsWebView() != this) {
        // Navigate to an empty page to fire all the destruction logic for the
        // current page.
        loadURLForFrame(GURL("about:blank"), WebString());
    }

    for (Vector<WebKit::WebWidget*>::iterator it = m_popupmenus.begin();
         it < m_popupmenus.end(); ++it)
        (*it)->close();

    webWidget()->close();
    if (m_inModalLoop)
        webkit_support::QuitMessageLoop();
}

void WebViewHost::setWebWidget(WebKit::WebWidget* widget)
{
    m_webWidget = widget;
    webView()->setSpellCheckClient(this);
    webView()->setPrerendererClient(this);
    webView()->setCompositorSurfaceReady();
}

WebView* WebViewHost::webView() const
{
    ASSERT(m_webWidget);
    // DRT does not support popup widgets. So m_webWidget is always a WebView.
    return static_cast<WebView*>(m_webWidget);
}

WebWidget* WebViewHost::webWidget() const
{
    ASSERT(m_webWidget);
    return m_webWidget;
}

void WebViewHost::reset()
{
    m_policyDelegateEnabled = false;
    m_policyDelegateIsPermissive = false;
    m_policyDelegateShouldNotifyDone = false;
    m_topLoadingFrame = 0;
    m_pageId = -1;
    m_lastPageIdUpdated = -1;
    m_hasWindow = false;
    m_inModalLoop = false;
    m_smartInsertDeleteEnabled = true;
    m_logConsoleOutput = true;
#if OS(WINDOWS)
    m_selectTrailingWhitespaceEnabled = true;
#else
    m_selectTrailingWhitespaceEnabled = false;
#endif
    m_blocksRedirects = false;
    m_requestReturnNull = false;
    m_isPainting = false;
    m_canvas.clear();
#if ENABLE(POINTER_LOCK)
    m_pointerLocked = false;
    m_pointerLockPlannedResult = PointerLockWillSucceed;
#endif

    m_navigationController = adoptPtr(new TestNavigationController(this));

    m_pendingExtraData.clear();
    m_resourceIdentifierMap.clear();
    m_clearHeaders.clear();
    m_editCommandName.clear();
    m_editCommandValue.clear();

    if (m_geolocationClientMock.get())
        m_geolocationClientMock->resetMock();

#if ENABLE(INPUT_SPEECH)
    if (m_speechInputControllerMock.get())
        m_speechInputControllerMock->clearResults();
#endif

    m_currentCursor = WebCursorInfo();
    m_windowRect = WebRect();
    m_paintRect = WebRect();

    if (m_webWidget) {
        webView()->mainFrame()->setName(WebString());
        webView()->settings()->setMinimumTimerInterval(webkit_support::GetForegroundTabTimerInterval());
    }
}

void WebViewHost::setSelectTrailingWhitespaceEnabled(bool enabled)
{
    m_selectTrailingWhitespaceEnabled = enabled;
    // In upstream WebKit, smart insert/delete is mutually exclusive with select
    // trailing whitespace, however, we allow both because Chromium on Windows
    // allows both.
}

void WebViewHost::setSmartInsertDeleteEnabled(bool enabled)
{
    m_smartInsertDeleteEnabled = enabled;
    // In upstream WebKit, smart insert/delete is mutually exclusive with select
    // trailing whitespace, however, we allow both because Chromium on Windows
    // allows both.
}

void WebViewHost::setLogConsoleOutput(bool enabled)
{
    m_logConsoleOutput = enabled;
}

void WebViewHost::setCustomPolicyDelegate(bool isCustom, bool isPermissive)
{
    m_policyDelegateEnabled = isCustom;
    m_policyDelegateIsPermissive = isPermissive;
}

void WebViewHost::waitForPolicyDelegate()
{
    m_policyDelegateEnabled = true;
    m_policyDelegateShouldNotifyDone = true;
}

void WebViewHost::setEditCommand(const string& name, const string& value)
{
    m_editCommandName = name;
    m_editCommandValue = value;
}

void WebViewHost::clearEditCommand()
{
    m_editCommandName.clear();
    m_editCommandValue.clear();
}

void WebViewHost::loadURLForFrame(const WebURL& url, const WebString& frameName)
{
    if (!url.isValid())
        return;
    TestShell::resizeWindowForTest(this, url);
    navigationController()->loadEntry(TestNavigationEntry::create(-1, url, WebString(), frameName).get());
}

bool WebViewHost::navigate(const TestNavigationEntry& entry, bool reload)
{
    // Get the right target frame for the entry.
    WebFrame* frame = webView()->mainFrame();
    if (!entry.targetFrame().isEmpty())
        frame = webView()->findFrameByName(entry.targetFrame());

    // TODO(mpcomplete): should we clear the target frame, or should
    // back/forward navigations maintain the target frame?

    // A navigation resulting from loading a javascript URL should not be
    // treated as a browser initiated event. Instead, we want it to look as if
    // the page initiated any load resulting from JS execution.
    if (!GURL(entry.URL()).SchemeIs("javascript"))
        setPendingExtraData(adoptPtr(new TestShellExtraData(entry.pageID())));

    // If we are reloading, then WebKit will use the state of the current page.
    // Otherwise, we give it the state to navigate to.
    if (reload) {
        frame->reload(false);
    } else if (!entry.contentState().isNull()) {
        ASSERT(entry.pageID() != -1);
        frame->loadHistoryItem(entry.contentState());
    } else {
        ASSERT(entry.pageID() == -1);
        frame->loadRequest(WebURLRequest(entry.URL()));
    }

    // In case LoadRequest failed before DidCreateDataSource was called.
    setPendingExtraData(nullptr);

    // Restore focus to the main frame prior to loading new request.
    // This makes sure that we don't have a focused iframe. Otherwise, that
    // iframe would keep focus when the SetFocus called immediately after
    // LoadRequest, thus making some tests fail (see http://b/issue?id=845337
    // for more details).
    webView()->setFocusedFrame(frame);
    m_shell->setFocus(webView(), true);

    return true;
}

// Private functions ----------------------------------------------------------

DRTTestRunner* WebViewHost::testRunner() const
{
    return m_shell->testRunner();
}

void WebViewHost::updateAddressBar(WebView* webView)
{
    WebFrame* mainFrame = webView->mainFrame();
    WebDataSource* dataSource = mainFrame->dataSource();
    if (!dataSource)
        dataSource = mainFrame->provisionalDataSource();
    if (!dataSource)
        return;

    setAddressBarURL(dataSource->request().url());
}

void WebViewHost::locationChangeDone(WebFrame* frame)
{
    if (frame != m_topLoadingFrame)
        return;
    m_topLoadingFrame = 0;
    testRunner()->locationChangeDone();
}

void WebViewHost::updateForCommittedLoad(WebFrame* frame, bool isNewNavigation)
{
    // Code duplicated from RenderView::DidCommitLoadForFrame.
    TestShellExtraData* extraData = static_cast<TestShellExtraData*>(frame->dataSource()->extraData());
    bool nonBlankPageAfterReset = m_pageId == -1 && strcmp(frame->dataSource()->request().url().spec().data(), "about:blank");

    if (isNewNavigation || nonBlankPageAfterReset) {
        // New navigation.
        updateSessionHistory(frame);
        m_pageId = nextPageID++;
    } else if (extraData && extraData->pendingPageID != -1 && !extraData->requestCommitted) {
        // This is a successful session history navigation!
        updateSessionHistory(frame);
        m_pageId = extraData->pendingPageID;
    }

    // Don't update session history multiple times.
    if (extraData)
        extraData->requestCommitted = true;

    updateURL(frame);
}

void WebViewHost::updateURL(WebFrame* frame)
{
    WebDataSource* ds = frame->dataSource();
    ASSERT(ds);
    const WebURLRequest& request = ds->request();
    RefPtr<TestNavigationEntry> entry(TestNavigationEntry::create());

    // The referrer will be empty on https->http transitions. It
    // would be nice if we could get the real referrer from somewhere.
    entry->setPageID(m_pageId);
    if (ds->hasUnreachableURL())
        entry->setURL(ds->unreachableURL());
    else
        entry->setURL(request.url());

    const WebHistoryItem& historyItem = frame->currentHistoryItem();
    if (!historyItem.isNull())
        entry->setContentState(historyItem);

    navigationController()->didNavigateToEntry(entry.get());
    updateAddressBar(frame->view());
    m_lastPageIdUpdated = max(m_lastPageIdUpdated, m_pageId);
}

void WebViewHost::updateSessionHistory(WebFrame* frame)
{
    // If we have a valid page ID at this point, then it corresponds to the page
    // we are navigating away from. Otherwise, this is the first navigation, so
    // there is no past session history to record.
    if (m_pageId == -1)
        return;

    TestNavigationEntry* entry = navigationController()->entryWithPageID(m_pageId);
    if (!entry)
        return;

    const WebHistoryItem& historyItem = webView()->mainFrame()->previousHistoryItem();
    if (historyItem.isNull())
        return;

    entry->setContentState(historyItem);
}

void WebViewHost::printFrameDescription(WebFrame* webframe)
{
    string name8 = webframe->name().utf8();
    if (webframe == webView()->mainFrame()) {
        if (!name8.length()) {
            fputs("main frame", stdout);
            return;
        }
        printf("main frame \"%s\"", name8.c_str());
        return;
    }
    if (!name8.length()) {
        fputs("frame (anonymous)", stdout);
        return;
    }
    printf("frame \"%s\"", name8.c_str());
}

void WebViewHost::printFrameUserGestureStatus(WebFrame* webframe, const char* msg)
{
    bool isUserGesture = webframe->isProcessingUserGesture();
    printf("Frame with user gesture \"%s\"%s", isUserGesture ? "true" : "false", msg);
}

void WebViewHost::printResourceDescription(unsigned identifier)
{
    ResourceMap::iterator it = m_resourceIdentifierMap.find(identifier);
    printf("%s", it != m_resourceIdentifierMap.end() ? it->second.c_str() : "<unknown>");
}

void WebViewHost::setPendingExtraData(PassOwnPtr<TestShellExtraData> extraData)
{
    m_pendingExtraData = extraData;
}

void WebViewHost::setDeviceScaleFactor(float deviceScaleFactor)
{
    webView()->setDeviceScaleFactor(deviceScaleFactor);
    discardBackingStore();
}

void WebViewHost::setGamepadData(const WebGamepads& pads)
{
    webkit_support::SetGamepadData(pads);
}

void WebViewHost::setPageTitle(const WebString&)
{
    // Nothing to do in layout test.
}

void WebViewHost::setAddressBarURL(const WebURL&)
{
    // Nothing to do in layout test.
}

void WebViewHost::enterFullScreenNow()
{
    if (testRunner()->hasCustomFullScreenBehavior())
        return;

    webView()->willEnterFullScreen();
    webView()->didEnterFullScreen();
}

void WebViewHost::exitFullScreenNow()
{
    if (testRunner()->hasCustomFullScreenBehavior())
        return;

    webView()->willExitFullScreen();
    webView()->didExitFullScreen();
}

#if ENABLE(MEDIA_STREAM)
webkit_support::TestMediaStreamClient* WebViewHost::testMediaStreamClient()
{
    if (!m_testMediaStreamClient.get())
        m_testMediaStreamClient = adoptPtr(new webkit_support::TestMediaStreamClient());
    return m_testMediaStreamClient.get();
}
#endif

// Painting functions ---------------------------------------------------------

void WebViewHost::updatePaintRect(const WebRect& rect)
{
    // m_paintRect = m_paintRect U rect
    if (rect.isEmpty())
        return;
    if (m_paintRect.isEmpty()) {
        m_paintRect = rect;
        return;
    }
    int left = min(m_paintRect.x, rect.x);
    int top = min(m_paintRect.y, rect.y);
    int right = max(m_paintRect.x + m_paintRect.width, rect.x + rect.width);
    int bottom = max(m_paintRect.y + m_paintRect.height, rect.y + rect.height);
    m_paintRect = WebRect(left, top, right - left, bottom - top);
}

void WebViewHost::paintRect(const WebRect& rect)
{
    ASSERT(!m_isPainting);
    ASSERT(canvas());
    m_isPainting = true;
    float deviceScaleFactor = webView()->deviceScaleFactor();
    int scaledX = static_cast<int>(static_cast<float>(rect.x) * deviceScaleFactor);
    int scaledY = static_cast<int>(static_cast<float>(rect.y) * deviceScaleFactor);
    int scaledWidth = static_cast<int>(ceil(static_cast<float>(rect.width) * deviceScaleFactor));
    int scaledHeight = static_cast<int>(ceil(static_cast<float>(rect.height) * deviceScaleFactor));
    WebRect deviceRect(scaledX, scaledY, scaledWidth, scaledHeight);
    webWidget()->paint(canvas(), deviceRect);
    m_isPainting = false;
}

void WebViewHost::paintInvalidatedRegion()
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    webWidget()->animate(0.0);
#endif
    webWidget()->layout();
    WebSize widgetSize = webWidget()->size();
    WebRect clientRect(0, 0, widgetSize.width, widgetSize.height);

    // Paint the canvas if necessary. Allow painting to generate extra rects
    // for the first two calls. This is necessary because some WebCore rendering
    // objects update their layout only when painted.
    // Store the total area painted in total_paint. Then tell the gdk window
    // to update that area after we're done painting it.
    for (int i = 0; i < 3; ++i) {
        // m_paintRect = intersect(m_paintRect , clientRect)
        int left = max(m_paintRect.x, clientRect.x);
        int top = max(m_paintRect.y, clientRect.y);
        int right = min(m_paintRect.x + m_paintRect.width, clientRect.x + clientRect.width);
        int bottom = min(m_paintRect.y + m_paintRect.height, clientRect.y + clientRect.height);
        if (left >= right || top >= bottom)
            m_paintRect = WebRect();
        else
            m_paintRect = WebRect(left, top, right - left, bottom - top);

        if (m_paintRect.isEmpty())
            continue;
        WebRect rect(m_paintRect);
        m_paintRect = WebRect();
        paintRect(rect);
    }
    ASSERT(m_paintRect.isEmpty());
}

void WebViewHost::paintPagesWithBoundaries()
{
    ASSERT(!m_isPainting);
    ASSERT(canvas());
    m_isPainting = true;

    WebSize pageSizeInPixels = webWidget()->size();
    WebFrame* webFrame = webView()->mainFrame();

    int pageCount = webFrame->printBegin(pageSizeInPixels);
    int totalHeight = pageCount * (pageSizeInPixels.height + 1) - 1;

    SkCanvas* testCanvas = skia::TryCreateBitmapCanvas(pageSizeInPixels.width, totalHeight, true);
    if (testCanvas) {
        discardBackingStore();
        m_canvas = adoptPtr(testCanvas);
    } else {
        webFrame->printEnd();
        return;
    }

    webFrame->printPagesWithBoundaries(canvas(), pageSizeInPixels);
    webFrame->printEnd();

    m_isPainting = false;
}

SkCanvas* WebViewHost::canvas()
{
    if (m_canvas)
        return m_canvas.get();
    WebSize widgetSize = webWidget()->size();
    float deviceScaleFactor = webView()->deviceScaleFactor();
    int scaledWidth = static_cast<int>(ceil(static_cast<float>(widgetSize.width) * deviceScaleFactor));
    int scaledHeight = static_cast<int>(ceil(static_cast<float>(widgetSize.height) * deviceScaleFactor));
    resetScrollRect();
    m_canvas = adoptPtr(skia::CreateBitmapCanvas(scaledWidth, scaledHeight, true));
    return m_canvas.get();
}

void WebViewHost::resetScrollRect()
{
}

void WebViewHost::discardBackingStore()
{
    m_canvas.clear();
}

// Paints the entire canvas a semi-transparent black (grayish). This is used
// by the layout tests in fast/repaint. The alpha value matches upstream.
void WebViewHost::displayRepaintMask()
{
    canvas()->drawARGB(167, 0, 0, 0);
}

// Simulate a print by going into print mode and then exit straight away.
void WebViewHost::printPage(WebKit::WebFrame* frame)
{
    WebSize pageSizeInPixels = webWidget()->size();
    WebPrintParams printParams(pageSizeInPixels);
    frame->printBegin(printParams);
    frame->printEnd();
}
