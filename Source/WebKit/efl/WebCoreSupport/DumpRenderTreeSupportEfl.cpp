/*
    Copyright (C) 2011 ProFUSION embedded systems
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "DumpRenderTreeSupportEfl.h"

#include "FrameLoaderClientEfl.h"
#include "ewk_frame_private.h"
#include "ewk_history_private.h"
#include "ewk_intent_private.h"
#include "ewk_private.h"
#include "ewk_view_private.h"

#include <APICast.h>
#include <AnimationController.h>
#include <CSSComputedStyleDeclaration.h>
#include <DocumentLoader.h>
#include <EditorClientEfl.h>
#include <Eina.h>
#include <Evas.h>
#include <FindOptions.h>
#include <FloatSize.h>
#include <FocusController.h>
#include <FrameView.h>
#include <HTMLInputElement.h>
#include <InspectorController.h>
#include <IntRect.h>
#include <Intent.h>
#include <JSCSSStyleDeclaration.h>
#include <JSElement.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <MemoryCache.h>
#include <MutationObserver.h>
#include <PageGroup.h>
#include <PrintContext.h>
#include <RenderTreeAsText.h>
#include <ResourceLoadScheduler.h>
#include <SchemeRegistry.h>
#include <ScriptValue.h>
#include <Settings.h>
#include <TextIterator.h>
#include <bindings/js/GCController.h>
#include <history/HistoryItem.h>
#include <workers/WorkerThread.h>
#include <wtf/HashMap.h>

unsigned DumpRenderTreeSupportEfl::activeAnimationsCount(const Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return 0;

    WebCore::AnimationController* animationController = frame->animation();

    if (!animationController)
        return 0;

    return animationController->numberOfActiveAnimations(frame->document());
}

bool DumpRenderTreeSupportEfl::callShouldCloseOnWebView(Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return false;

    return frame->loader()->shouldClose();
}

void DumpRenderTreeSupportEfl::clearFrameName(Evas_Object* ewkFrame)
{
    if (WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame))
        frame->tree()->clearName();
}

void DumpRenderTreeSupportEfl::clearOpener(Evas_Object* ewkFrame)
{
    if (WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame))
        frame->loader()->setOpener(0);
}

bool DumpRenderTreeSupportEfl::elementDoesAutoCompleteForElementWithId(const Evas_Object* ewkFrame, const String& elementId)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return false;

    WebCore::Document* document = frame->document();
    ASSERT(document);

    WebCore::HTMLInputElement* inputElement = static_cast<WebCore::HTMLInputElement*>(document->getElementById(elementId));

    if (!inputElement)
        return false;

    return inputElement->isTextField() && !inputElement->isPasswordField() && inputElement->shouldAutocomplete();
}

Eina_List* DumpRenderTreeSupportEfl::frameChildren(const Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return 0;

    Eina_List* childFrames = 0;

    for (unsigned index = 0; index < frame->tree()->childCount(); index++) {
        WebCore::Frame *childFrame = frame->tree()->child(index);
        WebCore::FrameLoaderClientEfl *client = static_cast<WebCore::FrameLoaderClientEfl*>(childFrame->loader()->client());

        if (!client)
            continue;

        childFrames = eina_list_append(childFrames, client->webFrame());
    }

    return childFrames;
}

WebCore::Frame* DumpRenderTreeSupportEfl::frameParent(const Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return 0;

    return frame->tree()->parent();
}

void DumpRenderTreeSupportEfl::layoutFrame(Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return;

    WebCore::FrameView* frameView = frame->view();

    if (!frameView)
        return;

    frameView->layout();
}

int DumpRenderTreeSupportEfl::numberOfPages(const Evas_Object* ewkFrame, float pageWidth, float pageHeight)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return 0;

    return WebCore::PrintContext::numberOfPages(frame, WebCore::FloatSize(pageWidth, pageHeight));
}

String DumpRenderTreeSupportEfl::pageSizeAndMarginsInPixels(const Evas_Object* ewkFrame, int pageNumber, int width, int height, int marginTop, int marginRight, int marginBottom, int marginLeft)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return String();

    return WebCore::PrintContext::pageSizeAndMarginsInPixels(frame, pageNumber, width, height, marginTop, marginRight, marginBottom, marginLeft);
}

String DumpRenderTreeSupportEfl::pageProperty(const Evas_Object* ewkFrame, const char* propertyName, int pageNumber)
{
    WebCore::Frame* coreFrame = EWKPrivate::coreFrame(ewkFrame);
    if (!coreFrame)
        return String();

    return WebCore::PrintContext::pageProperty(coreFrame, propertyName, pageNumber);
}

bool DumpRenderTreeSupportEfl::pauseAnimation(Evas_Object* ewkFrame, const char* name, const char* elementId, double time)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return false;

    WebCore::Element* element = frame->document()->getElementById(elementId);

    if (!element || !element->renderer())
        return false;

    return frame->animation()->pauseAnimationAtTime(element->renderer(), name, time);
}

bool DumpRenderTreeSupportEfl::pauseTransition(Evas_Object* ewkFrame, const char* name, const char* elementId, double time)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return false;

    WebCore::Element* element = frame->document()->getElementById(elementId);

    if (!element || !element->renderer())
        return false;

    return frame->animation()->pauseTransitionAtTime(element->renderer(), name, time);
}

unsigned DumpRenderTreeSupportEfl::pendingUnloadEventCount(const Evas_Object* ewkFrame)
{
    if (WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame))
        return frame->domWindow()->pendingUnloadEventListeners();

    return 0;
}

String DumpRenderTreeSupportEfl::renderTreeDump(Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return String();

    WebCore::FrameView *frameView = frame->view();

    if (frameView && frameView->layoutPending())
        frameView->layout();

    return WebCore::externalRepresentation(frame);
}

String DumpRenderTreeSupportEfl::responseMimeType(const Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return String();

    WebCore::DocumentLoader *documentLoader = frame->loader()->documentLoader();

    if (!documentLoader)
        return String();

    return documentLoader->responseMIMEType();
}

WebCore::IntRect DumpRenderTreeSupportEfl::selectionRectangle(const Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return WebCore::IntRect();

    return enclosingIntRect(frame->selection()->bounds());
}

// Compare with "WebKit/Tools/DumpRenderTree/mac/FrameLoadDelegate.mm
String DumpRenderTreeSupportEfl::suitableDRTFrameName(const Evas_Object* ewkFrame)
{
    WebCore::Frame* frame = EWKPrivate::coreFrame(ewkFrame);

    if (!frame)
        return String();

    const String frameName(ewk_frame_name_get(ewkFrame));

    if (ewkFrame == ewk_view_frame_main_get(ewk_frame_view_get(ewkFrame))) {
        if (!frameName.isEmpty())
            return String("main frame \"") + frameName + String("\"");

        return String("main frame");
    }

    if (!frameName.isEmpty())
        return String("frame \"") + frameName + String("\"");

    return String("frame (anonymous)");
}

void DumpRenderTreeSupportEfl::setValueForUser(JSContextRef context, JSValueRef nodeObject, const String& value)
{
    JSC::ExecState* exec = toJS(context);
    WebCore::Element* element = WebCore::toElement(toJS(exec, nodeObject));
    if (!element)
        return;
    WebCore::HTMLInputElement* inputElement = element->toInputElement();
    if (!inputElement)
        return;

    inputElement->setValueForUser(value);
}

void DumpRenderTreeSupportEfl::setAutofilled(JSContextRef context, JSValueRef nodeObject, bool autofilled)
{
    JSC::ExecState* exec = toJS(context);
    WebCore::Element* element = WebCore::toElement(toJS(exec, nodeObject));
    if (!element)
        return;
    WebCore::HTMLInputElement* inputElement = element->toInputElement();
    if (!inputElement)
        return;

    inputElement->setAutofilled(autofilled);
}

void DumpRenderTreeSupportEfl::setDefersLoading(Evas_Object* ewkView, bool defers)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);

    if (!page)
        return;

    page->setDefersLoading(defers);
}

void DumpRenderTreeSupportEfl::setLoadsSiteIconsIgnoringImageLoadingSetting(Evas_Object* ewkView, bool loadsSiteIconsIgnoringImageLoadingPreferences)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    page->settings()->setLoadsSiteIconsIgnoringImageLoadingSetting(loadsSiteIconsIgnoringImageLoadingPreferences);
}

void DumpRenderTreeSupportEfl::addUserScript(const Evas_Object* ewkView, const String& sourceCode, bool runAtStart, bool allFrames)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    page->group().addUserScriptToWorld(WebCore::mainThreadNormalWorld(), sourceCode, WebCore::KURL(),
                                       nullptr, nullptr, runAtStart ? WebCore::InjectAtDocumentStart : WebCore::InjectAtDocumentEnd,
                                       allFrames ? WebCore::InjectInAllFrames : WebCore::InjectInTopFrameOnly);
}

void DumpRenderTreeSupportEfl::clearUserScripts(const Evas_Object* ewkView)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    page->group().removeUserScriptsFromWorld(WebCore::mainThreadNormalWorld());
}

void DumpRenderTreeSupportEfl::addUserStyleSheet(const Evas_Object* ewkView, const String& sourceCode, bool allFrames)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    page->group().addUserStyleSheetToWorld(WebCore::mainThreadNormalWorld(), sourceCode, WebCore::KURL(), nullptr, nullptr, allFrames ? WebCore::InjectInAllFrames : WebCore::InjectInTopFrameOnly);
}

void DumpRenderTreeSupportEfl::clearUserStyleSheets(const Evas_Object* ewkView)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    page->group().removeUserStyleSheetsFromWorld(WebCore::mainThreadNormalWorld());
}

void DumpRenderTreeSupportEfl::executeCoreCommandByName(const Evas_Object* ewkView, const char* name, const char* value)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    page->focusController()->focusedOrMainFrame()->editor()->command(name).execute(value);
}

bool DumpRenderTreeSupportEfl::findString(const Evas_Object* ewkView, const String& text, WebCore::FindOptions options)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);

    if (!page)
        return false;

    return page->findString(text, options);
}

void DumpRenderTreeSupportEfl::setCSSGridLayoutEnabled(const Evas_Object* ewkView, bool enabled)
{
    WebCore::Page* corePage = EWKPrivate::corePage(ewkView);
    if (corePage)
        corePage->settings()->setCSSGridLayoutEnabled(enabled);
}

bool DumpRenderTreeSupportEfl::isCommandEnabled(const Evas_Object* ewkView, const char* name)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return false;

    return page->focusController()->focusedOrMainFrame()->editor()->command(name).isEnabled();
}

void DumpRenderTreeSupportEfl::setSmartInsertDeleteEnabled(Evas_Object* ewkView, bool enabled)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    WebCore::EditorClientEfl* editorClient = static_cast<WebCore::EditorClientEfl*>(page->editorClient());
    if (!editorClient)
        return;

    editorClient->setSmartInsertDeleteEnabled(enabled);
}

void DumpRenderTreeSupportEfl::setSelectTrailingWhitespaceEnabled(Evas_Object* ewkView, bool enabled)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    WebCore::EditorClientEfl* editorClient = static_cast<WebCore::EditorClientEfl*>(page->editorClient());
    if (!editorClient)
        return;

    editorClient->setSelectTrailingWhitespaceEnabled(enabled);
}

void DumpRenderTreeSupportEfl::garbageCollectorCollect()
{
    WebCore::gcController().garbageCollectNow();
}

void DumpRenderTreeSupportEfl::garbageCollectorCollectOnAlternateThread(bool waitUntilDone)
{
    WebCore::gcController().garbageCollectOnAlternateThreadForDebugging(waitUntilDone);
}

size_t DumpRenderTreeSupportEfl::javaScriptObjectsCount()
{
    return WebCore::JSDOMWindow::commonJSGlobalData()->heap.objectCount();
}

unsigned DumpRenderTreeSupportEfl::workerThreadCount()
{
#if ENABLE(WORKERS)
    return WebCore::WorkerThread::workerThreadCount();
#else
    return 0;
#endif
}

void DumpRenderTreeSupportEfl::setDeadDecodedDataDeletionInterval(double interval)
{
    WebCore::memoryCache()->setDeadDecodedDataDeletionInterval(interval);
}

HistoryItemChildrenVector DumpRenderTreeSupportEfl::childHistoryItems(const Ewk_History_Item* ewkHistoryItem)
{
    WebCore::HistoryItem* historyItem = EWKPrivate::coreHistoryItem(ewkHistoryItem);
    HistoryItemChildrenVector kids;

    if (!historyItem)
        return kids;

    const WebCore::HistoryItemVector& children = historyItem->children();
    const unsigned size = children.size();

    for (unsigned i = 0; i < size; ++i) {
        Ewk_History_Item* kid = ewk_history_item_new_from_core(children[i].get());
        kids.append(kid);
    }

    return kids;
}

String DumpRenderTreeSupportEfl::historyItemTarget(const Ewk_History_Item* ewkHistoryItem)
{
    WebCore::HistoryItem* historyItem = EWKPrivate::coreHistoryItem(ewkHistoryItem);

    if (!historyItem)
        return String();

    return historyItem->target();
}

bool DumpRenderTreeSupportEfl::isTargetItem(const Ewk_History_Item* ewkHistoryItem)
{
    WebCore::HistoryItem* historyItem = EWKPrivate::coreHistoryItem(ewkHistoryItem);

    if (!historyItem)
        return false;

    return historyItem->isTargetItem();
}

void DumpRenderTreeSupportEfl::evaluateInWebInspector(const Evas_Object* ewkView, long callId, const String& script)
{
#if ENABLE(INSPECTOR)
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page)
        return;

    if (page->inspectorController())
        page->inspectorController()->evaluateForTestInFrontend(callId, script);
#endif
}

void DumpRenderTreeSupportEfl::evaluateScriptInIsolatedWorld(const Evas_Object* ewkFrame, int worldID, JSObjectRef globalObject, const String& script)
{
    WebCore::Frame* coreFrame = EWKPrivate::coreFrame(ewkFrame);
    if (!coreFrame)
        return;

    // Comment from mac: Start off with some guess at a frame and a global object, we'll try to do better...!
    WebCore::JSDOMWindow* anyWorldGlobalObject = coreFrame->script()->globalObject(WebCore::mainThreadNormalWorld());

    // Comment from mac: The global object is probably a shell object? - if so, we know how to use this!
    JSC::JSObject* globalObjectObj = toJS(globalObject);
    if (!strcmp(globalObjectObj->classInfo()->className, "JSDOMWindowShell"))
        anyWorldGlobalObject = static_cast<WebCore::JSDOMWindowShell*>(globalObjectObj)->window();

    // Comment from mac: Get the frame from the global object we've settled on.
    WebCore::Frame* globalFrame = anyWorldGlobalObject->impl()->frame();
    if (!globalFrame)
        return;

    WebCore::ScriptController* proxy = globalFrame->script();
    if (!proxy)
        return;

    static WTF::HashMap<int, WTF::RefPtr<WebCore::DOMWrapperWorld > > worldMap;

    WTF::RefPtr<WebCore::DOMWrapperWorld> scriptWorld;
    if (!worldID)
        scriptWorld = WebCore::ScriptController::createWorld();
    else {
        WTF::HashMap<int, RefPtr<WebCore::DOMWrapperWorld > >::const_iterator it = worldMap.find(worldID);
        if (it != worldMap.end())
            scriptWorld = (*it).second;
        else {
            scriptWorld = WebCore::ScriptController::createWorld();
            worldMap.set(worldID, scriptWorld);
        }
    }

    // The code below is only valid for JSC, V8 specific code is to be added
    // when V8 will be supported in EFL port. See Qt implemenation.
    proxy->executeScriptInWorld(scriptWorld.get(), script, true);
}

JSGlobalContextRef DumpRenderTreeSupportEfl::globalContextRefForFrame(const Evas_Object* ewkFrame)
{
    WebCore::Frame* coreFrame = EWKPrivate::coreFrame(ewkFrame);
    if (!coreFrame)
        return 0;

    return toGlobalRef(coreFrame->script()->globalObject(WebCore::mainThreadNormalWorld())->globalExec());
}

void DumpRenderTreeSupportEfl::setMockScrollbarsEnabled(bool enable)
{
    WebCore::Settings::setMockScrollbarsEnabled(enable);
}

void DumpRenderTreeSupportEfl::deliverAllMutationsIfNecessary()
{
#if ENABLE(MUTATION_OBSERVERS)
    WebCore::MutationObserver::deliverAllMutations();
#endif
}

String DumpRenderTreeSupportEfl::markerTextForListItem(JSContextRef context, JSValueRef nodeObject)
{
    JSC::ExecState* exec = toJS(context);
    WebCore::Element* element = WebCore::toElement(toJS(exec, nodeObject));
    if (!element)
        return String();

    return WebCore::markerTextForListItem(element);
}

void DumpRenderTreeSupportEfl::setInteractiveFormValidationEnabled(Evas_Object* ewkView, bool enabled)
{
    WebCore::Page* corePage = EWKPrivate::corePage(ewkView);
    if (corePage)
        corePage->settings()->setInteractiveFormValidationEnabled(enabled);
}

void DumpRenderTreeSupportEfl::setValidationMessageTimerMagnification(Evas_Object* ewkView, int value)
{
    WebCore::Page* corePage = EWKPrivate::corePage(ewkView);
    if (corePage)
        corePage->settings()->setValidationMessageTimerMagnification(value);
}

JSValueRef DumpRenderTreeSupportEfl::computedStyleIncludingVisitedInfo(JSContextRef context, JSValueRef value)
{
    if (!value)
        return JSValueMakeUndefined(context);

    JSC::ExecState* exec = toJS(context);
    JSC::JSValue jsValue = toJS(exec, value);
    if (!jsValue.inherits(&WebCore::JSElement::s_info))
        return JSValueMakeUndefined(context);

    WebCore::JSElement* jsElement = static_cast<WebCore::JSElement*>(asObject(jsValue));
    WebCore::Element* element = jsElement->impl();
    RefPtr<WebCore::CSSComputedStyleDeclaration> style = WebCore::CSSComputedStyleDeclaration::create(element, true);
    return toRef(exec, toJS(exec, jsElement->globalObject(), style.get()));
}

void DumpRenderTreeSupportEfl::setAuthorAndUserStylesEnabled(Evas_Object* ewkView, bool enabled)
{
    WebCore::Page* corePage = EWKPrivate::corePage(ewkView);
    if (!corePage)
        return;

    corePage->settings()->setAuthorAndUserStylesEnabled(enabled);
}

void DumpRenderTreeSupportEfl::setSerializeHTTPLoads(bool enabled)
{
    WebCore::resourceLoadScheduler()->setSerialLoadingEnabled(enabled);
}

void DumpRenderTreeSupportEfl::sendWebIntentResponse(Ewk_Intent_Request* request, JSStringRef response)
{
#if ENABLE(WEB_INTENTS)
    JSC::UString responseString = response->ustring();
    if (responseString.isNull())
        ewk_intent_request_failure_post(request, WebCore::SerializedScriptValue::create(String::fromUTF8("ERROR")));
    else
        ewk_intent_request_result_post(request, WebCore::SerializedScriptValue::create(String(responseString.impl())));
#endif
}

WebCore::MessagePortChannelArray* DumpRenderTreeSupportEfl::intentMessagePorts(const Ewk_Intent* intent)
{
#if ENABLE(WEB_INTENTS)
    const WebCore::Intent* coreIntent = EWKPrivate::coreIntent(intent);
    return coreIntent ? coreIntent->messagePorts() : 0;
#else
    return 0;
#endif
}

void DumpRenderTreeSupportEfl::deliverWebIntent(Evas_Object* ewkFrame, JSStringRef action, JSStringRef type, JSStringRef data)
{
#if ENABLE(WEB_INTENTS)
    RefPtr<WebCore::SerializedScriptValue> serializedData = WebCore::SerializedScriptValue::create(String(data->ustring().impl()));
    WebCore::ExceptionCode ec = 0;
    WebCore::MessagePortArray ports;
    RefPtr<WebCore::Intent> coreIntent = WebCore::Intent::create(String(action->ustring().impl()), String(type->ustring().impl()), serializedData.get(), ports, ec);
    if (ec)
        return;
    Ewk_Intent* ewkIntent = ewk_intent_new(coreIntent.get());
    ewk_frame_intent_deliver(ewkFrame, ewkIntent);
    ewk_intent_free(ewkIntent);
#endif
}

void DumpRenderTreeSupportEfl::setComposition(Evas_Object* ewkView, const char* text, int start, int length)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page || !page->focusController() || !page->focusController()->focusedOrMainFrame())
        return;

    WebCore::Editor* editor = page->focusController()->focusedOrMainFrame()->editor();
    if (!editor || (!editor->canEdit() && !editor->hasComposition()))
        return;

    const String compositionString = String::fromUTF8(text);
    Vector<WebCore::CompositionUnderline> underlines;
    underlines.append(WebCore::CompositionUnderline(0, compositionString.length(), WebCore::Color(0, 0, 0), false));
    editor->setComposition(compositionString, underlines, start, start + length);
}

bool DumpRenderTreeSupportEfl::hasComposition(const Evas_Object* ewkView)
{
    const WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page || !page->focusController() || !page->focusController()->focusedOrMainFrame())
        return false;

    const WebCore::Editor* editor = page->focusController()->focusedOrMainFrame()->editor();
    if (!editor)
        return false;

    return editor->hasComposition();
}

bool DumpRenderTreeSupportEfl::compositionRange(Evas_Object* ewkView, int* start, int* length)
{
    *start = *length = 0;

    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page || !page->focusController() || !page->focusController()->focusedOrMainFrame())
        return false;

    WebCore::Editor* editor = page->focusController()->focusedOrMainFrame()->editor();
    if (!editor || !editor->hasComposition())
        return false;

    *start = editor->compositionStart();
    *length = editor->compositionEnd() - *start;
    return true;
}

void DumpRenderTreeSupportEfl::confirmComposition(Evas_Object* ewkView, const char* text)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page || !page->focusController() || !page->focusController()->focusedOrMainFrame())
        return;

    WebCore::Editor* editor = page->focusController()->focusedOrMainFrame()->editor();
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

WebCore::IntRect DumpRenderTreeSupportEfl::firstRectForCharacterRange(Evas_Object* ewkView, int location, int length)
{
    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page || !page->focusController() || !page->focusController()->focusedOrMainFrame() || !page->focusController()->focusedOrMainFrame()->editor())
        return WebCore::IntRect();

    if ((location + length < location) && (location + length))
        length = 0;

    WebCore::Frame* frame = page->focusController()->focusedOrMainFrame();
    WebCore::Editor* editor = frame->editor();

    RefPtr<WebCore::Range> range = WebCore::TextIterator::rangeFromLocationAndLength(frame->selection()->rootEditableElementOrDocumentElement(), location, length);
    if (!range)
        return WebCore::IntRect();

    return editor->firstRectForRange(range.get());
}

bool DumpRenderTreeSupportEfl::selectedRange(Evas_Object* ewkView, int* start, int* length)
{
    if (!(start && length))
        return false;

    WebCore::Page* page = EWKPrivate::corePage(ewkView);
    if (!page || !page->focusController() || !page->focusController()->focusedOrMainFrame())
        return false;

    WebCore::Frame* frame = page->focusController()->focusedOrMainFrame();
    RefPtr<WebCore::Range> range = frame->selection()->toNormalizedRange().get();
    if (!range)
        return false;

    WebCore::Element* selectionRoot = frame->selection()->rootEditableElement();
    WebCore::Element* scope = selectionRoot ? selectionRoot : frame->document()->documentElement();

    RefPtr<WebCore::Range> testRange = WebCore::Range::create(scope->document(), scope, 0, range->startContainer(), range->startOffset());
    *start = WebCore::TextIterator::rangeLength(testRange.get());

    WebCore::ExceptionCode ec;
    testRange->setEnd(range->endContainer(), range->endOffset(), ec);
    *length = WebCore::TextIterator::rangeLength(testRange.get());

    return true;
}

void DumpRenderTreeSupportEfl::setDomainRelaxationForbiddenForURLScheme(bool forbidden, const String& scheme)
{
    WebCore::SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(forbidden, scheme);
}
