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

#ifndef DumpRenderTreeSupportEfl_h
#define DumpRenderTreeSupportEfl_h

#include <Eina.h>
#include <FindOptions.h>
#include <IntRect.h>
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSStringRef.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

typedef struct _Evas_Object Evas_Object;
typedef struct _Ewk_History_Item Ewk_History_Item;
typedef struct _Ewk_Intent Ewk_Intent;
typedef struct _Ewk_Intent_Request Ewk_Intent_Request;

typedef Vector<Ewk_History_Item*> HistoryItemChildrenVector;

namespace WebCore {
class Frame;
class MessagePortChannel;
typedef Vector<OwnPtr<MessagePortChannel>, 1> MessagePortChannelArray;
}

class EAPI DumpRenderTreeSupportEfl {
public:
    DumpRenderTreeSupportEfl() { }

    ~DumpRenderTreeSupportEfl() { }

    static unsigned activeAnimationsCount(const Evas_Object* ewkFrame);
    static bool callShouldCloseOnWebView(Evas_Object* ewkFrame);
    static void clearFrameName(Evas_Object* ewkFrame);
    static void clearOpener(Evas_Object* ewkFrame);
    static String counterValueByElementId(const Evas_Object* ewkFrame, const char* elementId);
    static bool elementDoesAutoCompleteForElementWithId(const Evas_Object* ewkFrame, const String& elementId);
    static Eina_List* frameChildren(const Evas_Object* ewkFrame);
    static WebCore::Frame* frameParent(const Evas_Object* ewkFrame);
    static void layoutFrame(Evas_Object* ewkFrame);
    static bool pauseAnimation(Evas_Object* ewkFrame, const char* name, const char* elementId, double time);
    static bool pauseTransition(Evas_Object* ewkFrame, const char* name, const char* elementId, double time);
    static unsigned pendingUnloadEventCount(const Evas_Object* ewkFrame);
    static String renderTreeDump(Evas_Object* ewkFrame);
    static String responseMimeType(const Evas_Object* ewkFrame);
    static WebCore::IntRect selectionRectangle(const Evas_Object* ewkFrame);
    static String suitableDRTFrameName(const Evas_Object* ewkFrame);
    static String layerTreeAsText(const Evas_Object* ewkFrame);
    static void setValueForUser(JSContextRef, JSValueRef nodeObject, const String& value);
    static void setAutofilled(JSContextRef, JSValueRef nodeObject, bool autofilled);
    static void setDefersLoading(Evas_Object* ewkView, bool defers);
    static void setLoadsSiteIconsIgnoringImageLoadingSetting(Evas_Object* ewkView, bool loadsSiteIconsIgnoringImageLoadingPreferences);
    static void setMinimumLogicalFontSize(Evas_Object* ewkView, int size);

    static void addUserScript(const Evas_Object* ewkView, const String& sourceCode, bool runAtStart, bool allFrames);
    static void clearUserScripts(const Evas_Object* ewkView);
    static void addUserStyleSheet(const Evas_Object* ewkView, const String& sourceCode, bool allFrames);
    static void clearUserStyleSheets(const Evas_Object* ewkView);
    static void executeCoreCommandByName(const Evas_Object* ewkView, const char* name, const char* value);
    static bool findString(const Evas_Object* ewkView, const String& text, WebCore::FindOptions);
    static bool isCommandEnabled(const Evas_Object* ewkView, const char* name);
    static void setCSSGridLayoutEnabled(const Evas_Object* ewkView, bool enabled);
    static void setCSSRegionsEnabled(const Evas_Object* ewkView, bool enabled);
    static void setSmartInsertDeleteEnabled(Evas_Object* ewkView, bool enabled);
    static void setSelectTrailingWhitespaceEnabled(Evas_Object* ewkView, bool enabled);

    static void garbageCollectorCollect();
    static void garbageCollectorCollectOnAlternateThread(bool waitUntilDone);
    static size_t javaScriptObjectsCount();
    static unsigned workerThreadCount();

    static void setDeadDecodedDataDeletionInterval(double);

    static HistoryItemChildrenVector childHistoryItems(const Ewk_History_Item*);
    static String historyItemTarget(const Ewk_History_Item*);
    static bool isTargetItem(const Ewk_History_Item*);
    static void evaluateInWebInspector(const Evas_Object* ewkView, long callId, const String& script);
    static void evaluateScriptInIsolatedWorld(const Evas_Object* ewkFrame, int worldID, JSObjectRef globalObject, const String& script);
    static JSGlobalContextRef globalContextRefForFrame(const Evas_Object* ewkFrame);

    static void setMockScrollbarsEnabled(bool);

    static void deliverAllMutationsIfNecessary();
    static String markerTextForListItem(JSContextRef, JSValueRef nodeObject);
    static void setInteractiveFormValidationEnabled(Evas_Object* ewkView, bool enabled);
    static void setValidationMessageTimerMagnification(Evas_Object* ewkView, int value);
    static JSValueRef computedStyleIncludingVisitedInfo(JSContextRef, JSValueRef);
    static void setAuthorAndUserStylesEnabled(Evas_Object* ewkView, bool);
    static void setDomainRelaxationForbiddenForURLScheme(bool forbidden, const String& scheme);
    static void setSerializeHTTPLoads(bool);
    
    // Web Intents
    static void sendWebIntentResponse(Ewk_Intent_Request*, JSStringRef response);
    static WebCore::MessagePortChannelArray* intentMessagePorts(const Ewk_Intent*);
    static void deliverWebIntent(Evas_Object* ewkFrame, JSStringRef action, JSStringRef type, JSStringRef data);

    // TextInputController
    static void setComposition(Evas_Object*, const char*, int, int);
    static bool hasComposition(const Evas_Object*);
    static bool compositionRange(Evas_Object*, int*, int*);
    static void confirmComposition(Evas_Object*, const char*);
    static WebCore::IntRect firstRectForCharacterRange(Evas_Object*, int, int);
    static bool selectedRange(Evas_Object*, int*, int*);
};

#endif // DumpRenderTreeSupportEfl_h
