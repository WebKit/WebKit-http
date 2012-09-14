/*
 *  Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *  Copyright (C) 2010 Joone Hur <joone@kldp.org>
 *  Copyright (C) 2009 Google Inc. All rights reserved.
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "DumpRenderTreeSupportGtk.h"

#include "APICast.h"
#include "AXObjectCache.h"
#include "AccessibilityObject.h"
#include "AnimationController.h"
#include "ApplicationCacheStorage.h"
#include "CSSComputedStyleDeclaration.h"
#include "Chrome.h"
#include "ChromeClientGtk.h"
#include "DOMWrapperWorld.h"
#include "Document.h"
#include "EditorClientGtk.h"
#include "Element.h"
#include "FocusController.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GCController.h"
#include "GeolocationClientMock.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "JSCSSStyleDeclaration.h"
#include "JSDOMWindow.h"
#include "JSDocument.h"
#include "JSElement.h"
#include "JSLock.h"
#include "JSNodeList.h"
#include "JSValue.h"
#include "MemoryCache.h"
#include "MutationObserver.h"
#include "NodeList.h"
#include "PageGroup.h"
#include "PrintContext.h"
#include "RenderListItem.h"
#include "RenderTreeAsText.h"
#include "RenderView.h"
#include "ResourceLoadScheduler.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "Settings.h"
#include "TextIterator.h"
#include "WebKitAccessibleWrapperAtk.h"
#include "WorkerThread.h"
#include "webkitglobalsprivate.h"
#include "webkitwebframe.h"
#include "webkitwebframeprivate.h"
#include "webkitwebview.h"
#include "webkitwebviewprivate.h"
#include <JavaScriptCore/APICast.h>
#include <wtf/text/WTFString.h>

using namespace JSC;
using namespace WebCore;
using namespace WebKit;

bool DumpRenderTreeSupportGtk::s_drtRun = false;
bool DumpRenderTreeSupportGtk::s_linksIncludedInTabChain = true;
bool DumpRenderTreeSupportGtk::s_selectTrailingWhitespaceEnabled = false;

DumpRenderTreeSupportGtk::DumpRenderTreeSupportGtk()
{
}

DumpRenderTreeSupportGtk::~DumpRenderTreeSupportGtk()
{
}

void DumpRenderTreeSupportGtk::setDumpRenderTreeModeEnabled(bool enabled)
{
    s_drtRun = enabled;
}

bool DumpRenderTreeSupportGtk::dumpRenderTreeModeEnabled()
{
    return s_drtRun;
}
void DumpRenderTreeSupportGtk::setLinksIncludedInFocusChain(bool enabled)
{
    s_linksIncludedInTabChain = enabled;
}

bool DumpRenderTreeSupportGtk::linksIncludedInFocusChain()
{
    return s_linksIncludedInTabChain;
}

void DumpRenderTreeSupportGtk::setSelectTrailingWhitespaceEnabled(bool enabled)
{
    s_selectTrailingWhitespaceEnabled = enabled;
}

bool DumpRenderTreeSupportGtk::selectTrailingWhitespaceEnabled()
{
    return s_selectTrailingWhitespaceEnabled;
}

/**
 * getFrameChildren:
 * @frame: a #WebKitWebFrame
 *
 * Return value: child frames of @frame
 */
GSList* DumpRenderTreeSupportGtk::getFrameChildren(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    GSList* children = 0;
    for (Frame* child = coreFrame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        WebKitWebFrame* kitFrame = kit(child);
        if (kitFrame)
          children = g_slist_append(children, kitFrame);
    }

    return children;
}

/**
 * getInnerText:
 * @frame: a #WebKitWebFrame
 *
 * Return value: inner text of @frame
 */
CString DumpRenderTreeSupportGtk::getInnerText(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), CString(""));

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return CString("");

    FrameView* view = coreFrame->view();
    if (view && view->layoutPending())
        view->layout();

    Element* documentElement = coreFrame->document()->documentElement();
    if (!documentElement)
        return CString("");
    return documentElement->innerText().utf8();
}

/**
 * dumpRenderTree:
 * @frame: a #WebKitWebFrame
 *
 * Return value: Non-recursive render tree dump of @frame
 */
CString DumpRenderTreeSupportGtk::dumpRenderTree(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), CString(""));

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return CString("");

    FrameView* view = coreFrame->view();

    if (view && view->layoutPending())
        view->layout();

    return externalRepresentation(coreFrame).utf8();
}

/**
 * addUserStyleSheet
 * @frame: a #WebKitWebFrame
 * @sourceCode: code of a user stylesheet
 *
 */
void DumpRenderTreeSupportGtk::addUserStyleSheet(WebKitWebFrame* frame, const char* sourceCode, bool allFrames)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    WebKitWebView* webView = getViewFromFrame(frame);
    Page* page = core(webView);
    page->group().addUserStyleSheetToWorld(mainThreadNormalWorld(), sourceCode, KURL(), nullptr, nullptr, allFrames ? InjectInAllFrames : InjectInTopFrameOnly); 
}

/**
 * getPendingUnloadEventCount:
 * @frame: a #WebKitWebFrame
 *
 * Return value: number of pending unload events
 */
guint DumpRenderTreeSupportGtk::getPendingUnloadEventCount(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    return core(frame)->document()->domWindow()->pendingUnloadEventListeners();
}

bool DumpRenderTreeSupportGtk::pauseAnimation(WebKitWebFrame* frame, const char* name, double time, const char* element)
{
    ASSERT(core(frame));
    Element* coreElement = core(frame)->document()->getElementById(AtomicString(element));
    if (!coreElement || !coreElement->renderer())
        return false;
    return core(frame)->animation()->pauseAnimationAtTime(coreElement->renderer(), AtomicString(name), time);
}

bool DumpRenderTreeSupportGtk::pauseTransition(WebKitWebFrame* frame, const char* name, double time, const char* element)
{
    ASSERT(core(frame));
    Element* coreElement = core(frame)->document()->getElementById(AtomicString(element));
    if (!coreElement || !coreElement->renderer())
        return false;
    return core(frame)->animation()->pauseTransitionAtTime(coreElement->renderer(), AtomicString(name), time);
}

CString DumpRenderTreeSupportGtk::markerTextForListItem(WebKitWebFrame* frame, JSContextRef context, JSValueRef nodeObject)
{
    JSC::ExecState* exec = toJS(context);
    Element* element = toElement(toJS(exec, nodeObject));
    if (!element)
        return CString();

    return WebCore::markerTextForListItem(element).utf8();
}

unsigned int DumpRenderTreeSupportGtk::numberOfActiveAnimations(WebKitWebFrame* frame)
{
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return 0;

    return coreFrame->animation()->numberOfActiveAnimations(coreFrame->document());
}

void DumpRenderTreeSupportGtk::clearMainFrameName(WebKitWebFrame* frame)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    core(frame)->tree()->clearName();
}

AtkObject* DumpRenderTreeSupportGtk::getRootAccessibleElement(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

#if HAVE(ACCESSIBILITY)
    if (!AXObjectCache::accessibilityEnabled())
        AXObjectCache::enableAccessibility();

    WebKitWebFramePrivate* priv = frame->priv;
    if (!priv->coreFrame || !priv->coreFrame->document())
        return 0;

    AtkObject* wrapper =  priv->coreFrame->document()->axObjectCache()->rootObject()->wrapper();
    if (!wrapper)
        return 0;

    return wrapper;
#else
    return 0;
#endif
}

AtkObject* DumpRenderTreeSupportGtk::getFocusedAccessibleElement(WebKitWebFrame* frame)
{
#if HAVE(ACCESSIBILITY)
    AtkObject* wrapper = getRootAccessibleElement(frame);
    if (!wrapper)
        return 0;

    return webkitAccessibleGetFocusedElement(WEBKIT_ACCESSIBLE(wrapper));
#else
    return 0;
#endif
}

void DumpRenderTreeSupportGtk::executeCoreCommandByName(WebKitWebView* webView, const gchar* name, const gchar* value)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(name);
    g_return_if_fail(value);

    core(webView)->focusController()->focusedOrMainFrame()->editor()->command(name).execute(value);
}

bool DumpRenderTreeSupportGtk::isCommandEnabled(WebKitWebView* webView, const gchar* name)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), FALSE);
    g_return_val_if_fail(name, FALSE);

    return core(webView)->focusController()->focusedOrMainFrame()->editor()->command(name).isEnabled();
}

void DumpRenderTreeSupportGtk::setComposition(WebKitWebView* webView, const char* text, int start, int length)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(text);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Editor* editor = frame->editor();
    if (!editor || (!editor->canEdit() && !editor->hasComposition()))
        return;

    String compositionString = String::fromUTF8(text);
    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, compositionString.length(), Color(0, 0, 0), false));
    editor->setComposition(compositionString, underlines, start, start + length);
}

bool DumpRenderTreeSupportGtk::hasComposition(WebKitWebView* webView)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), false);
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return false;
    Editor* editor = frame->editor();
    if (!editor)
        return false;

    return editor->hasComposition();
}

bool DumpRenderTreeSupportGtk::compositionRange(WebKitWebView* webView, int* start, int* length)
{
    g_return_val_if_fail(start && length, false);
    *start = *length = 0;

    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), false);
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return false;

    Editor* editor = frame->editor();
    if (!editor || !editor->hasComposition())
        return false;

    *start = editor->compositionStart();
    *length = editor->compositionEnd() - *start;
    return true;
}

void DumpRenderTreeSupportGtk::confirmComposition(WebKitWebView* webView, const char* text)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Editor* editor = frame->editor();
    if (!editor)
        return;

    if (!editor->hasComposition()) {
        editor->insertText(String::fromUTF8(text), 0);
        return;
    }
    if (text) {
        editor->confirmComposition(String::fromUTF8(text));
        return;
    }
    editor->confirmComposition();
}

void DumpRenderTreeSupportGtk::doCommand(WebKitWebView* webView, const char* command)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Editor* editor = frame->editor();
    if (!editor)
        return;

    String commandString(command);
    // Remove ending : here.
    if (commandString.endsWith(":", true))
        commandString = commandString.left(commandString.length() - 1);

    // Make the first char in upper case.
    String firstChar = commandString.left(1);
    commandString = commandString.right(commandString.length() - 1);
    firstChar.makeUpper();
    commandString.insert(firstChar, 0);

    editor->command(commandString).execute();
}

bool DumpRenderTreeSupportGtk::firstRectForCharacterRange(WebKitWebView* webView, int location, int length, cairo_rectangle_int_t* rect)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), false);
    g_return_val_if_fail(rect, false);

    if ((location + length < location) && (location + length))
        length = 0;

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return false;

    Editor* editor = frame->editor();
    if (!editor)
        return false;

    RefPtr<Range> range = TextIterator::rangeFromLocationAndLength(frame->selection()->rootEditableElementOrDocumentElement(), location, length);
    if (!range)
        return false;

    *rect = editor->firstRectForRange(range.get());
    return true;
}

bool DumpRenderTreeSupportGtk::selectedRange(WebKitWebView* webView, int* start, int* length)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_VIEW(webView), false);
    g_return_val_if_fail(start && length, false);

    Frame* frame = core(webView)->focusController()->focusedOrMainFrame();
    if (!frame)
        return false;

    RefPtr<Range> range = frame->selection()->toNormalizedRange().get();
    if (!range)
        return false;

    Element* selectionRoot = frame->selection()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : frame->document()->documentElement();

    RefPtr<Range> testRange = Range::create(scope->document(), scope, 0, range->startContainer(), range->startOffset());
    ASSERT(testRange->startContainer() == scope);
    *start = TextIterator::rangeLength(testRange.get());

    ExceptionCode ec;
    testRange->setEnd(range->endContainer(), range->endOffset(), ec);
    ASSERT(testRange->startContainer() == scope);
    *length = TextIterator::rangeLength(testRange.get());

    return true;
}

void DumpRenderTreeSupportGtk::setDefersLoading(WebKitWebView* webView, bool defers)
{
    core(webView)->setDefersLoading(defers);
}

void DumpRenderTreeSupportGtk::setSmartInsertDeleteEnabled(WebKitWebView* webView, bool enabled)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));
    g_return_if_fail(webView);

    WebKit::EditorClient* client = static_cast<WebKit::EditorClient*>(core(webView)->editorClient());
    client->setSmartInsertDeleteEnabled(enabled);
}

void DumpRenderTreeSupportGtk::forceWebViewPaint(WebKitWebView* webView)
{
    g_return_if_fail(WEBKIT_IS_WEB_VIEW(webView));

    static_cast<WebKit::ChromeClient*>(core(webView)->chrome()->client())->forcePaint();
}

void DumpRenderTreeSupportGtk::whiteListAccessFromOrigin(const gchar* sourceOrigin, const gchar* destinationProtocol, const gchar* destinationHost, bool allowDestinationSubdomains)
{
    SecurityPolicy::addOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void DumpRenderTreeSupportGtk::removeWhiteListAccessFromOrigin(const char* sourceOrigin, const char* destinationProtocol, const char* destinationHost, bool allowDestinationSubdomains)
{
    SecurityPolicy::removeOriginAccessWhitelistEntry(*SecurityOrigin::createFromString(sourceOrigin), destinationProtocol, destinationHost, allowDestinationSubdomains);
}

void DumpRenderTreeSupportGtk::resetOriginAccessWhiteLists()
{
    SecurityPolicy::resetOriginAccessWhitelists();
}

void DumpRenderTreeSupportGtk::gcCollectJavascriptObjects()
{
    gcController().garbageCollectNow();
}

void DumpRenderTreeSupportGtk::gcCollectJavascriptObjectsOnAlternateThread(bool waitUntilDone)
{
    gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

unsigned long DumpRenderTreeSupportGtk::gcCountJavascriptObjects()
{
    JSC::JSLockHolder lock(JSDOMWindow::commonJSGlobalData());
    return JSDOMWindow::commonJSGlobalData()->heap.objectCount();
}

void DumpRenderTreeSupportGtk::layoutFrame(WebKitWebFrame* frame)
{
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    FrameView* view = coreFrame->view();
    if (!view)
        return;

    view->layout();
}

void DumpRenderTreeSupportGtk::clearOpener(WebKitWebFrame* frame)
{
    Frame* coreFrame = core(frame);
    if (coreFrame)
        coreFrame->loader()->setOpener(0);
}

unsigned int DumpRenderTreeSupportGtk::workerThreadCount()
{
#if ENABLE(WORKERS)
    return WebCore::WorkerThread::workerThreadCount();
#else
    return 0;
#endif
}

bool DumpRenderTreeSupportGtk::findString(WebKitWebView* webView, const gchar* targetString, WebKitFindOptions findOptions)
{
    return core(webView)->findString(String::fromUTF8(targetString), findOptions);
}

double DumpRenderTreeSupportGtk::defaultMinimumTimerInterval()
{
    return Settings::defaultMinDOMTimerInterval();
}

void DumpRenderTreeSupportGtk::setMinimumTimerInterval(WebKitWebView* webView, double interval)
{
    core(webView)->settings()->setMinDOMTimerInterval(interval);
}

CString DumpRenderTreeSupportGtk::accessibilityHelpText(AtkObject* axObject)
{
    if (!axObject || !WEBKIT_IS_ACCESSIBLE(axObject))
        return CString();

    AccessibilityObject* coreObject = webkitAccessibleGetAccessibilityObject(WEBKIT_ACCESSIBLE(axObject));
    if (!coreObject)
        return CString();

    return coreObject->helpText().utf8();
}

void DumpRenderTreeSupportGtk::setAutofilled(JSContextRef context, JSValueRef nodeObject, bool autofilled)
{
    JSC::ExecState* exec = toJS(context);
    Element* element = toElement(toJS(exec, nodeObject));
    if (!element)
        return;
    HTMLInputElement* inputElement = element->toInputElement();
    if (!inputElement)
        return;

    inputElement->setAutofilled(autofilled);
}

void DumpRenderTreeSupportGtk::setValueForUser(JSContextRef context, JSValueRef nodeObject, JSStringRef value)
{
    JSC::ExecState* exec = toJS(context);
    Element* element = toElement(toJS(exec, nodeObject));
    if (!element)
        return;
    HTMLInputElement* inputElement = element->toInputElement();
    if (!inputElement)
        return;

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(value);
    GOwnPtr<gchar> valueBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(value, valueBuffer.get(), bufferSize);
    inputElement->setValueForUser(String::fromUTF8(valueBuffer.get()));
}

void DumpRenderTreeSupportGtk::rectangleForSelection(WebKitWebFrame* frame, cairo_rectangle_int_t* rectangle)
{
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return;

    IntRect bounds = enclosingIntRect(coreFrame->selection()->bounds());
    rectangle->x = bounds.x();
    rectangle->y = bounds.y();
    rectangle->width = bounds.width();
    rectangle->height = bounds.height();
}

bool DumpRenderTreeSupportGtk::shouldClose(WebKitWebFrame* frame)
{
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return true;
    return coreFrame->loader()->shouldClose();
}

void DumpRenderTreeSupportGtk::scalePageBy(WebKitWebView* webView, float scaleFactor, float x, float y)
{
    core(webView)->setPageScaleFactor(scaleFactor, IntPoint(x, y));
}

void DumpRenderTreeSupportGtk::resetGeolocationClientMock(WebKitWebView* webView)
{
#if ENABLE(GEOLOCATION)
    GeolocationClientMock* mock = static_cast<GeolocationClientMock*>(GeolocationController::from(core(webView))->client());
    mock->reset();
#endif
}

void DumpRenderTreeSupportGtk::setMockGeolocationPermission(WebKitWebView* webView, bool allowed)
{
#if ENABLE(GEOLOCATION)
    GeolocationClientMock* mock = static_cast<GeolocationClientMock*>(GeolocationController::from(core(webView))->client());
    mock->setPermission(allowed);
#endif
}

void DumpRenderTreeSupportGtk::setMockGeolocationPosition(WebKitWebView* webView, double latitude, double longitude, double accuracy)
{
#if ENABLE(GEOLOCATION)
    GeolocationClientMock* mock = static_cast<GeolocationClientMock*>(GeolocationController::from(core(webView))->client());

    double timestamp = g_get_real_time() / 1000000.0;
    mock->setPosition(GeolocationPosition::create(timestamp, latitude, longitude, accuracy));
#endif
}

void DumpRenderTreeSupportGtk::setMockGeolocationError(WebKitWebView* webView, int errorCode, const gchar* errorMessage)
{
#if ENABLE(GEOLOCATION)
    GeolocationClientMock* mock = static_cast<GeolocationClientMock*>(GeolocationController::from(core(webView))->client());

    GeolocationError::ErrorCode code;
    switch (errorCode) {
    case PositionError::PERMISSION_DENIED:
        code = GeolocationError::PermissionDenied;
        break;
    case PositionError::POSITION_UNAVAILABLE:
    default:
        code = GeolocationError::PositionUnavailable;
        break;
    }

    mock->setError(GeolocationError::create(code, errorMessage));
#endif
}

int DumpRenderTreeSupportGtk::numberOfPendingGeolocationPermissionRequests(WebKitWebView* webView)
{
#if ENABLE(GEOLOCATION)
    GeolocationClientMock* mock = static_cast<GeolocationClientMock*>(GeolocationController::from(core(webView))->client());
    return mock->numberOfPendingPermissionRequests();
#endif
}

void DumpRenderTreeSupportGtk::setPageCacheSupportsPlugins(WebKitWebView* webView, bool enabled)
{
    core(webView)->settings()->setPageCacheSupportsPlugins(enabled);
}

void DumpRenderTreeSupportGtk::setCSSGridLayoutEnabled(WebKitWebView* webView, bool enabled)
{
    core(webView)->settings()->setCSSGridLayoutEnabled(enabled);
}

void DumpRenderTreeSupportGtk::setCSSRegionsEnabled(WebKitWebView* webView, bool enabled)
{
    core(webView)->settings()->setCSSRegionsEnabled(enabled);
}

bool DumpRenderTreeSupportGtk::elementDoesAutoCompleteForElementWithId(WebKitWebFrame* frame, JSStringRef id)
{
    Frame* coreFrame = core(frame);
    if (!coreFrame)
        return false;

    Document* document = coreFrame->document();
    ASSERT(document);

    size_t bufferSize = JSStringGetMaximumUTF8CStringSize(id);
    GOwnPtr<gchar> idBuffer(static_cast<gchar*>(g_malloc(bufferSize)));
    JSStringGetUTF8CString(id, idBuffer.get(), bufferSize);
    Node* coreNode = document->getElementById(String::fromUTF8(idBuffer.get()));
    if (!coreNode || !coreNode->renderer())
        return false;

    HTMLInputElement* inputElement = static_cast<HTMLInputElement*>(coreNode);
    if (!inputElement)
        return false;

    return inputElement->isTextField() && !inputElement->isPasswordField() && inputElement->shouldAutocomplete();
}

JSValueRef DumpRenderTreeSupportGtk::computedStyleIncludingVisitedInfo(JSContextRef context, JSValueRef nodeObject)
{
    JSC::ExecState* exec = toJS(context);
    if (!nodeObject)
        return JSValueMakeUndefined(context);

    JSValue jsValue = toJS(exec, nodeObject);
    if (!jsValue.inherits(&JSElement::s_info))
        return JSValueMakeUndefined(context);

    JSElement* jsElement = static_cast<JSElement*>(asObject(jsValue));
    Element* element = jsElement->impl();
    RefPtr<CSSComputedStyleDeclaration> style = CSSComputedStyleDeclaration::create(element, true);
    return toRef(exec, toJS(exec, jsElement->globalObject(), style.get()));
}

void DumpRenderTreeSupportGtk::deliverAllMutationsIfNecessary()
{
#if ENABLE(MUTATION_OBSERVERS)
    MutationObserver::deliverAllMutations();
#endif
}

void DumpRenderTreeSupportGtk::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const char* urlScheme)
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, String::fromUTF8(urlScheme));
}

void DumpRenderTreeSupportGtk::setSerializeHTTPLoads(bool enabled)
{
    resourceLoadScheduler()->setSerialLoadingEnabled(enabled);
}

void DumpRenderTreeSupportGtk::setTracksRepaints(WebKitWebFrame* frame, bool tracks)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    Frame* coreFrame = core(frame);
    if (coreFrame && coreFrame->view())
        coreFrame->view()->setTracksRepaints(tracks);
}

bool DumpRenderTreeSupportGtk::isTrackingRepaints(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), false);

    Frame* coreFrame = core(frame);
    if (coreFrame && coreFrame->view())
        return coreFrame->view()->isTrackingRepaints();

    return false;
}

GSList* DumpRenderTreeSupportGtk::trackedRepaintRects(WebKitWebFrame* frame)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_FRAME(frame), 0);

    Frame* coreFrame = core(frame);
    if (!coreFrame || !coreFrame->view())
        return 0;

    GSList* rects = 0;
    const Vector<IntRect>& repaintRects = coreFrame->view()->trackedRepaintRects();
    for (unsigned i = 0; i < repaintRects.size(); i++) {
        GdkRectangle* rect = g_new0(GdkRectangle, 1);
        rect->x = repaintRects[i].x();
        rect->y = repaintRects[i].y();
        rect->width = repaintRects[i].width();
        rect->height = repaintRects[i].height();
        rects = g_slist_append(rects, rect);
    }

    return rects;
}

void DumpRenderTreeSupportGtk::resetTrackedRepaints(WebKitWebFrame* frame)
{
    g_return_if_fail(WEBKIT_IS_WEB_FRAME(frame));

    Frame* coreFrame = core(frame);
    if (coreFrame && coreFrame->view())
        coreFrame->view()->resetTrackedRepaints();
}

void DumpRenderTreeSupportGtk::clearMemoryCache()
{
    memoryCache()->evictResources();
}

void DumpRenderTreeSupportGtk::clearApplicationCache()
{
    cacheStorage().empty();
    cacheStorage().vacuumDatabaseFile();
}
