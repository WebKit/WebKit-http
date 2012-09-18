/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2008, 2009, 2011, 2012 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) Research In Motion Limited 2010-2011. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Document.h"

#include "AXObjectCache.h"
#include "AnimationController.h"
#include "Attr.h"
#include "Attribute.h"
#include "CDATASection.h"
#include "CSSParser.h"
#include "CSSStyleDeclaration.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CachedCSSStyleSheet.h"
#include "CachedResourceLoader.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "Comment.h"
#include "Console.h"
#include "ContentSecurityPolicy.h"
#include "ContextFeatures.h"
#include "CookieJar.h"
#include "DOMImplementation.h"
#include "DOMNamedFlowCollection.h"
#include "DOMSelection.h"
#include "DOMWindow.h"
#include "DateComponents.h"
#include "DeviceMotionController.h"
#include "DeviceOrientationController.h"
#include "DocumentEventQueue.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "DocumentMarkerController.h"
#include "DocumentType.h"
#include "EditingText.h"
#include "Editor.h"
#include "Element.h"
#include "ElementShadow.h"
#include "EntityReference.h"
#include "Event.h"
#include "EventFactory.h"
#include "EventHandler.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "FlowThreadController.h"
#include "FocusController.h"
#include "FormController.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "GeolocationController.h"
#include "HashChangeEvent.h"
#include "HistogramSupport.h"
#include "History.h"
#include "HTMLAllCollection.h"
#include "HTMLAnchorElement.h"
#include "HTMLBodyElement.h"
#include "HTMLCanvasElement.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "HTMLElementFactory.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLHeadElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLLinkElement.h"
#include "HTMLMapElement.h"
#include "HTMLNameCollection.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLStyleElement.h"
#include "HTMLTitleElement.h"
#include "HTTPParsers.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "ImageLoader.h"
#include "InspectorCounters.h"
#include "InspectorInstrumentation.h"
#include "Language.h"
#include "Localizer.h"
#include "Logging.h"
#include "MediaQueryList.h"
#include "MediaQueryMatcher.h"
#include "MouseEventWithHitTestResults.h"
#include "NameNodeList.h"
#include "NamedFlowCollection.h"
#include "NestingLevelIncrementer.h"
#include "NewXMLDocumentParser.h"
#include "NodeFilter.h"
#include "NodeIterator.h"
#include "NodeRareData.h"
#include "NodeWithIndex.h"
#include "Page.h"
#include "PageGroup.h"
#include "PageTransitionEvent.h"
#include "PlatformKeyboardEvent.h"
#include "PluginDocument.h"
#include "PointerLockController.h"
#include "PopStateEvent.h"
#include "ProcessingInstruction.h"
#include "RegisteredEventListener.h"
#include "RenderArena.h"
#include "RenderNamedFlowThread.h"
#include "RenderTextControl.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "RuntimeEnabledFeatures.h"
#include "SchemeRegistry.h"
#include "ScopedEventQueue.h"
#include "ScriptCallStack.h"
#include "ScriptController.h"
#include "ScriptElement.h"
#include "ScriptEventListener.h"
#include "ScriptRunner.h"
#include "ScrollingCoordinator.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "SegmentedString.h"
#include "SelectorQuery.h"
#include "Settings.h"
#include "ShadowRoot.h"
#include "StaticHashSetNodeList.h"
#include "StyleResolver.h"
#include "StyleSheetContents.h"
#include "StyleSheetList.h"
#include "TextResourceDecoder.h"
#include "Timer.h"
#include "TransformSource.h"
#include "TreeWalker.h"
#include "UndoManager.h"
#include "UserContentURLPattern.h"
#include "WebCoreMemoryInstrumentation.h"
#include "WebKitNamedFlow.h"
#include "XMLDocumentParser.h"
#include "XMLHttpRequest.h"
#include "XMLNSNames.h"
#include "XMLNames.h"
#include "XPathEvaluator.h"
#include "XPathExpression.h"
#include "XPathNSResolver.h"
#include "XPathResult.h"
#include "htmlediting.h"
#include <wtf/CurrentTime.h>
#include <wtf/HashFunctions.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuffer.h>

#if ENABLE(SHARED_WORKERS)
#include "SharedWorkerRepository.h"
#endif

#if ENABLE(XSLT)
#include "XSLTProcessor.h"
#endif

#if ENABLE(SVG)
#include "SVGDocumentExtensions.h"
#include "SVGElementFactory.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"
#include "SVGStyleElement.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "TouchList.h"
#endif

#if ENABLE(MATHML)
#include "MathMLElement.h"
#include "MathMLElementFactory.h"
#include "MathMLNames.h"
#endif

#if ENABLE(FULLSCREEN_API)
#include "RenderFullScreen.h"
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
#include "RequestAnimationFrameCallback.h"
#include "ScriptedAnimationController.h"
#endif

#if ENABLE(MICRODATA)
#include "MicroDataItemList.h"
#include "NodeRareData.h"
#endif

#if ENABLE(LINK_PRERENDER)
#include "Prerenderer.h"
#endif

#if ENABLE(TEXT_AUTOSIZING)
#include "TextAutosizer.h"
#endif

#if ENABLE(CSP_NEXT)
#include "DOMSecurityPolicy.h"
#endif

using namespace std;
using namespace WTF;
using namespace Unicode;

namespace WebCore {

using namespace HTMLNames;

// #define INSTRUMENT_LAYOUT_SCHEDULING 1

static const unsigned cMaxWriteRecursionDepth = 21;

// This amount of time must have elapsed before we will even consider scheduling a layout without a delay.
// FIXME: For faster machines this value can really be lowered to 200.  250 is adequate, but a little high
// for dual G5s. :)
static const int cLayoutScheduleThreshold = 250;

// DOM Level 2 says (letters added):
//
// a) Name start characters must have one of the categories Ll, Lu, Lo, Lt, Nl.
// b) Name characters other than Name-start characters must have one of the categories Mc, Me, Mn, Lm, or Nd.
// c) Characters in the compatibility area (i.e. with character code greater than #xF900 and less than #xFFFE) are not allowed in XML names.
// d) Characters which have a font or compatibility decomposition (i.e. those with a "compatibility formatting tag" in field 5 of the database -- marked by field 5 beginning with a "<") are not allowed.
// e) The following characters are treated as name-start characters rather than name characters, because the property file classifies them as Alphabetic: [#x02BB-#x02C1], #x0559, #x06E5, #x06E6.
// f) Characters #x20DD-#x20E0 are excluded (in accordance with Unicode, section 5.14).
// g) Character #x00B7 is classified as an extender, because the property list so identifies it.
// h) Character #x0387 is added as a name character, because #x00B7 is its canonical equivalent.
// i) Characters ':' and '_' are allowed as name-start characters.
// j) Characters '-' and '.' are allowed as name characters.
//
// It also contains complete tables. If we decide it's better, we could include those instead of the following code.

static inline bool isValidNameStart(UChar32 c)
{
    // rule (e) above
    if ((c >= 0x02BB && c <= 0x02C1) || c == 0x559 || c == 0x6E5 || c == 0x6E6)
        return true;

    // rule (i) above
    if (c == ':' || c == '_')
        return true;

    // rules (a) and (f) above
    const uint32_t nameStartMask = Letter_Lowercase | Letter_Uppercase | Letter_Other | Letter_Titlecase | Number_Letter;
    if (!(Unicode::category(c) & nameStartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static inline bool isValidNamePart(UChar32 c)
{
    // rules (a), (e), and (i) above
    if (isValidNameStart(c))
        return true;

    // rules (g) and (h) above
    if (c == 0x00B7 || c == 0x0387)
        return true;

    // rule (j) above
    if (c == '-' || c == '.')
        return true;

    // rules (b) and (f) above
    const uint32_t otherNamePartMask = Mark_NonSpacing | Mark_Enclosing | Mark_SpacingCombining | Letter_Modifier | Number_DecimalDigit;
    if (!(Unicode::category(c) & otherNamePartMask))
        return false;

    // rule (c) above
    if (c >= 0xF900 && c < 0xFFFE)
        return false;

    // rule (d) above
    DecompositionType decompType = decompositionType(c);
    if (decompType == DecompositionFont || decompType == DecompositionCompat)
        return false;

    return true;
}

static bool shouldInheritSecurityOriginFromOwner(const KURL& url)
{
    // http://www.whatwg.org/specs/web-apps/current-work/#origin-0
    //
    // If a Document has the address "about:blank"
    //     The origin of the Document is the origin it was assigned when its browsing context was created.
    //
    // Note: We generalize this to all "blank" URLs and invalid URLs because we
    // treat all of these URLs as about:blank.
    //
    return !url.isValid() || url.isBlankURL();
}

static Widget* widgetForNode(Node* focusedNode)
{
    if (!focusedNode)
        return 0;
    RenderObject* renderer = focusedNode->renderer();
    if (!renderer || !renderer->isWidget())
        return 0;
    return toRenderWidget(renderer)->widget();
}

static bool acceptsEditingFocus(Node* node)
{
    ASSERT(node);
    ASSERT(node->rendererIsEditable());

    Node* root = node->rootEditableElement();
    Frame* frame = node->document()->frame();
    if (!frame || !root)
        return false;

    return frame->editor()->shouldBeginEditing(rangeOfContents(root).get());
}

static bool canAccessAncestor(const SecurityOrigin* activeSecurityOrigin, Frame* targetFrame)
{
    // targetFrame can be 0 when we're trying to navigate a top-level frame
    // that has a 0 opener.
    if (!targetFrame)
        return false;

    const bool isLocalActiveOrigin = activeSecurityOrigin->isLocal();
    for (Frame* ancestorFrame = targetFrame; ancestorFrame; ancestorFrame = ancestorFrame->tree()->parent()) {
        Document* ancestorDocument = ancestorFrame->document();
        // FIXME: Should be an ASSERT? Frames should alway have documents.
        if (!ancestorDocument)
            return true;

        const SecurityOrigin* ancestorSecurityOrigin = ancestorDocument->securityOrigin();
        if (activeSecurityOrigin->canAccess(ancestorSecurityOrigin))
            return true;
        
        // Allow file URL descendant navigation even when allowFileAccessFromFileURLs is false.
        // FIXME: It's a bit strange to special-case local origins here. Should we be doing
        // something more general instead?
        if (isLocalActiveOrigin && ancestorSecurityOrigin->isLocal())
            return true;
    }

    return false;
}

static void printNavigationErrorMessage(Frame* frame, const KURL& activeURL)
{
    // FIXME: this error message should contain more specifics of why the navigation change is not allowed.
    String message = "Unsafe JavaScript attempt to initiate a navigation change for frame with URL " +
                     frame->document()->url().string() + " from frame with URL " + activeURL.string() + ".\n";

    // FIXME: should we print to the console of the document performing the navigation instead?
    frame->document()->domWindow()->printErrorMessage(message);
}

static HashSet<Document*>* documentsThatNeedStyleRecalc = 0;

class DocumentWeakReference : public ThreadSafeRefCounted<DocumentWeakReference> {
public:
    static PassRefPtr<DocumentWeakReference> create(Document* document)
    {
        return adoptRef(new DocumentWeakReference(document));
    }

    Document* document()
    {
        ASSERT(isMainThread());
        return m_document;
    }

    void clear()
    {
        ASSERT(isMainThread());
        m_document = 0;
    }

private:
    DocumentWeakReference(Document* document)
        : m_document(document)
    {
        ASSERT(isMainThread());
    }

    Document* m_document;
};

uint64_t Document::s_globalTreeVersion = 0;

Document::Document(Frame* frame, const KURL& url, bool isXHTML, bool isHTML)
    : ContainerNode(0, CreateDocument)
    , TreeScope(this)
    , m_guardRefCount(0)
    , m_contextFeatures(ContextFeatures::defaultSwitch())
    , m_compatibilityMode(NoQuirksMode)
    , m_compatibilityModeLocked(false)
    , m_domTreeVersion(++s_globalTreeVersion)
#if ENABLE(MUTATION_OBSERVERS)
    , m_mutationObserverTypes(0)
#endif
    , m_styleSheets(StyleSheetList::create(this))
    , m_hadActiveLoadingStylesheet(false)
    , m_readyState(Complete)
    , m_styleRecalcTimer(this, &Document::styleRecalcTimerFired)
    , m_pendingStyleRecalcShouldForce(false)
    , m_frameElementsShouldIgnoreScrolling(false)
    , m_containsValidityStyleRules(false)
    , m_updateFocusAppearanceRestoresSelection(false)
    , m_ignoreDestructiveWriteCount(0)
    , m_titleSetExplicitly(false)
    , m_updateFocusAppearanceTimer(this, &Document::updateFocusAppearanceTimerFired)
    , m_loadEventFinished(false)
    , m_startTime(currentTime())
    , m_overMinimumLayoutThreshold(false)
    , m_scriptRunner(ScriptRunner::create(this))
    , m_xmlVersion("1.0")
    , m_xmlStandalone(StandaloneUnspecified)
    , m_hasXMLDeclaration(0)
    , m_savedRenderer(0)
    , m_designMode(inherit)
#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(WIDGET_REGION)
    , m_hasDashboardRegions(false)
    , m_dashboardRegionsDirty(false)
#endif
    , m_createRenderers(true)
    , m_inPageCache(false)
    , m_accessKeyMapValid(false)
    , m_useSecureKeyboardEntryWhenActive(false)
    , m_isXHTML(isXHTML)
    , m_isHTML(isHTML)
    , m_isViewSource(false)
    , m_sawElementsInKnownNamespaces(false)
    , m_isSrcdocDocument(false)
    , m_documentRareData(0)
    , m_eventQueue(DocumentEventQueue::create(this))
    , m_weakReference(DocumentWeakReference::create(this))
    , m_idAttributeName(idAttr)
#if ENABLE(FULLSCREEN_API)
    , m_areKeysEnabledInFullScreen(0)
    , m_fullScreenRenderer(0)
    , m_fullScreenChangeDelayTimer(this, &Document::fullScreenChangeDelayTimerFired)
    , m_isAnimatingFullScreen(false)
#endif
    , m_loadEventDelayCount(0)
    , m_loadEventDelayTimer(this, &Document::loadEventDelayTimerFired)
    , m_referrerPolicy(ReferrerPolicyDefault)
    , m_directionSetOnDocumentElement(false)
    , m_writingModeSetOnDocumentElement(false)
    , m_writeRecursionIsTooDeep(false)
    , m_writeRecursionDepth(0)
    , m_wheelEventHandlerCount(0)
#if ENABLE(TOUCH_EVENTS)
    , m_touchEventHandlerCount(0)
#endif
#if ENABLE(UNDO_MANAGER)
    , m_undoManager(0)
#endif
    , m_pendingTasksTimer(this, &Document::pendingTasksTimerFired)
    , m_scheduledTasksAreSuspended(false)
    , m_visualUpdatesAllowed(true)
    , m_visualUpdatesSuppressionTimer(this, &Document::visualUpdatesSuppressionTimerFired)
#ifndef NDEBUG
    , m_didDispatchViewportPropertiesChanged(false)
#endif
{
    m_document = this;

    m_pageGroupUserSheetCacheValid = false;

    m_printing = false;
    m_paginatedForScreen = false;

    m_ignoreAutofocus = false;

    m_frame = frame;
    if (m_frame)
        provideContextFeaturesToDocumentFrom(this, m_frame->page());

    // We depend on the url getting immediately set in subframes, but we
    // also depend on the url NOT getting immediately set in opened windows.
    // See fast/dom/early-frame-url.html
    // and fast/dom/location-new-window-no-crash.html, respectively.
    // FIXME: Can/should we unify this behavior?
    if ((frame && frame->ownerElement()) || !url.isEmpty())
        setURL(url);

    m_axObjectCache = 0;

    m_markers = adoptPtr(new DocumentMarkerController);

    m_cachedResourceLoader = adoptPtr(new CachedResourceLoader(this));
#if ENABLE(LINK_PRERENDER)
    m_prerenderer = Prerenderer::create(this);
#endif
#if ENABLE(TEXT_AUTOSIZING)
    m_textAutosizer = TextAutosizer::create(this);
#endif
    m_visuallyOrdered = false;
    m_bParsing = false;
    m_wellFormed = false;

    m_textColor = Color::black;
    m_listenerTypes = 0;
    m_inStyleRecalc = false;
    m_closeAfterStyleRecalc = false;

    m_usesSiblingRules = false;
    m_usesSiblingRulesOverride = false;
    m_usesFirstLineRules = false;
    m_usesFirstLetterRules = false;
    m_usesBeforeAfterRules = false;
    m_usesBeforeAfterRulesOverride = false;
    m_usesRemUnits = false;
    m_usesLinkRules = false;

    m_gotoAnchorNeededAfterStylesheetsLoad = false;

    m_didCalculateStyleResolver = false;
    m_hasDirtyStyleResolver = false;
    m_pendingStylesheets = 0;
    m_ignorePendingStylesheets = false;
    m_needsNotifyRemoveAllPendingStylesheet = false;
    m_hasNodesWithPlaceholderStyle = false;
    m_pendingSheetLayout = NoLayoutWithPendingSheets;

    m_cssTarget = 0;

    resetLinkColor();
    resetVisitedLinkColor();
    resetActiveLinkColor();

    m_processingLoadEvent = false;
    
    initSecurityContext();
    initDNSPrefetch();

    static int docID = 0;
    m_docID = docID++;

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(m_nodeListCounts); i++)
        m_nodeListCounts[i] = 0;

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(m_collections); i++)
        m_collections[i] = 0;

    InspectorCounters::incrementCounter(InspectorCounters::DocumentCounter);
}

static void histogramMutationEventUsage(const unsigned short& listenerTypes)
{
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMSubtreeModified", static_cast<bool>(listenerTypes & Document::DOMSUBTREEMODIFIED_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeInserted", static_cast<bool>(listenerTypes & Document::DOMNODEINSERTED_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeRemoved", static_cast<bool>(listenerTypes & Document::DOMNODEREMOVED_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeRemovedFromDocument", static_cast<bool>(listenerTypes & Document::DOMNODEREMOVEDFROMDOCUMENT_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMNodeInsertedIntoDocument", static_cast<bool>(listenerTypes & Document::DOMNODEINSERTEDINTODOCUMENT_LISTENER), 2);
    HistogramSupport::histogramEnumeration("DOMAPI.PerDocumentMutationEventUsage.DOMCharacterDataModified", static_cast<bool>(listenerTypes & Document::DOMCHARACTERDATAMODIFIED_LISTENER), 2);
}

#if ENABLE(FULLSCREEN_API)
static bool isAttributeOnAllOwners(const WebCore::QualifiedName& attribute, const HTMLFrameOwnerElement* owner)
{
    if (!owner)
        return true;
    do {
        if (!owner->hasAttribute(attribute))
            return false;
    } while ((owner = owner->document()->ownerElement()));
    return true;
}
#endif

Document::~Document()
{
    ASSERT(!renderer());
    ASSERT(!m_inPageCache);
    ASSERT(!m_savedRenderer);
    ASSERT(m_ranges.isEmpty());
    ASSERT(!m_styleRecalcTimer.isActive());
    ASSERT(!m_parentTreeScope);
    ASSERT(!m_guardRefCount);

    // FIXME: Should we reset m_domWindow when we detach from the Frame?
    if (m_domWindow)
        m_domWindow->resetUnlessSuspendedForPageCache();

    m_scriptRunner.clear();

    histogramMutationEventUsage(m_listenerTypes);

    removeAllEventListeners();

    // Currently we believe that Document can never outlive the parser.
    // Although the Document may be replaced synchronously, DocumentParsers
    // generally keep at least one reference to an Element which would in turn
    // has a reference to the Document.  If you hit this ASSERT, then that
    // assumption is wrong.  DocumentParser::detach() should ensure that even
    // if the DocumentParser outlives the Document it won't cause badness.
    ASSERT(!m_parser || m_parser->refCount() == 1);
    detachParser();

    m_renderArena.clear();

    clearAXObjectCache();

    m_decoder = 0;

    if (m_styleSheets)
        m_styleSheets->documentDestroyed();

    if (m_namedFlows)
        m_namedFlows->documentDestroyed();

    if (m_elemSheet)
        m_elemSheet->clearOwnerNode();
    if (m_pageUserSheet)
        m_pageUserSheet->clearOwnerNode();
    if (m_pageGroupUserSheets) {
        for (size_t i = 0; i < m_pageGroupUserSheets->size(); ++i)
            (*m_pageGroupUserSheets)[i]->clearOwnerNode();
    }
    if (m_userSheets) {
        for (size_t i = 0; i < m_userSheets->size(); ++i)
            (*m_userSheets)[i]->clearOwnerNode();
    }

    deleteCustomFonts();

    m_weakReference->clear();

    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->documentDestroyed();

    clearStyleResolver(); // We need to destory CSSFontSelector before destroying m_cachedResourceLoader.
    m_cachedResourceLoader.clear();

    // We must call clearRareData() here since a Document class inherits TreeScope
    // as well as Node. See a comment on TreeScope.h for the reason.
    if (hasRareData())
        clearRareData();

    ASSERT(!m_listsInvalidatedAtDocument.size());

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(m_nodeListCounts); i++)
        ASSERT(!m_nodeListCounts[i]);

    for (unsigned i = 0; i < WTF_ARRAY_LENGTH(m_collections); i++)
        ASSERT(!m_collections[i]);

    m_document = 0;

    InspectorCounters::decrementCounter(InspectorCounters::DocumentCounter);
}

void Document::removedLastRef()
{
    ASSERT(!m_deletionHasBegun);
    if (m_guardRefCount) {
        // If removing a child removes the last self-only ref, we don't
        // want the scope to be destructed until after
        // removeAllChildren returns, so we guard ourselves with an
        // extra self-only ref.
        guardRef();

        // We must make sure not to be retaining any of our children through
        // these extra pointers or we will create a reference cycle.
        m_docType = 0;
        m_focusedNode = 0;
        m_hoverNode = 0;
        m_activeNode = 0;
        m_titleElement = 0;
        m_documentElement = 0;
        m_contextFeatures = ContextFeatures::defaultSwitch();
#if ENABLE(FULLSCREEN_API)
        m_fullScreenElement = 0;
        m_fullScreenElementStack.clear();
#endif

        detachParser();

        // removeAllChildren() doesn't always unregister IDs,
        // so tear down scope information upfront to avoid having stale references in the map.
        destroyTreeScopeData();
        removeAllChildren();

        m_markers->detach();

        m_cssCanvasElements.clear();

#if ENABLE(REQUEST_ANIMATION_FRAME)
        // FIXME: consider using ActiveDOMObject.
        if (m_scriptedAnimationController)
            m_scriptedAnimationController->clearDocumentPointer();
        m_scriptedAnimationController.clear();
#endif

#ifndef NDEBUG
        m_inRemovedLastRefFunction = false;
#endif

        guardDeref();
    } else {
#ifndef NDEBUG 
        m_deletionHasBegun = true; 
#endif 
        delete this;
    }
}

Element* Document::getElementById(const AtomicString& id) const
{
    return TreeScope::getElementById(id);
}

Element* Document::getElementByAccessKey(const String& key)
{
    if (key.isEmpty())
        return 0;
    if (!m_accessKeyMapValid) {
        buildAccessKeyMap(this);
        m_accessKeyMapValid = true;
    }
    return m_elementsByAccessKey.get(key.impl());
}

void Document::buildAccessKeyMap(TreeScope* scope)
{
    ASSERT(scope);
    Node* rootNode = scope->rootNode();
    for (Node* node = rootNode; node; node = node->traverseNextNode(rootNode)) {
        if (!node->isElementNode())
            continue;
        Element* element = static_cast<Element*>(node);
        const AtomicString& accessKey = element->getAttribute(accesskeyAttr);
        if (!accessKey.isEmpty())
            m_elementsByAccessKey.set(accessKey.impl(), element);

        for (ShadowRoot* root = node->youngestShadowRoot(); root; root = root->olderShadowRoot())
            buildAccessKeyMap(root);
    }
}

void Document::invalidateAccessKeyMap()
{
    m_accessKeyMapValid = false;
    m_elementsByAccessKey.clear();
}

SelectorQueryCache* Document::selectorQueryCache()
{
    if (!m_selectorQueryCache)
        m_selectorQueryCache = adoptPtr(new SelectorQueryCache());
    return m_selectorQueryCache.get();
}

MediaQueryMatcher* Document::mediaQueryMatcher()
{
    if (!m_mediaQueryMatcher)
        m_mediaQueryMatcher = MediaQueryMatcher::create(this);
    return m_mediaQueryMatcher.get();
}

void Document::setCompatibilityMode(CompatibilityMode mode)
{
    if (m_compatibilityModeLocked || mode == m_compatibilityMode)
        return;
    ASSERT(!m_styleSheets->length());
    bool wasInQuirksMode = inQuirksMode();
    m_compatibilityMode = mode;
    selectorQueryCache()->invalidate();
    if (inQuirksMode() != wasInQuirksMode) {
        // All user stylesheets have to reparse using the different mode.
        clearPageUserSheet();
        clearPageGroupUserSheets();
    }
}

String Document::compatMode() const
{
    return inQuirksMode() ? "BackCompat" : "CSS1Compat";
}

void Document::resetLinkColor()
{
    m_linkColor = Color(0, 0, 238);
}

void Document::resetVisitedLinkColor()
{
    m_visitedLinkColor = Color(85, 26, 139);    
}

void Document::resetActiveLinkColor()
{
    m_activeLinkColor.setNamedColor("red");
}

void Document::setDocType(PassRefPtr<DocumentType> docType)
{
    // This should never be called more than once.
    ASSERT(!m_docType || !docType);
    m_docType = docType;
    if (m_docType) {
        this->adoptIfNeeded(m_docType.get());
#if ENABLE(LEGACY_VIEWPORT_ADAPTION)
        if (m_docType->publicId().startsWith("-//wapforum//dtd xhtml mobile 1.", /* caseSensitive */ false))
            processViewport("width=device-width, height=device-height", ViewportArguments::XHTMLMobileProfile);
#endif
    }
    // Doctype affects the interpretation of the stylesheets.
    clearStyleResolver();
}

DOMImplementation* Document::implementation()
{
    if (!m_implementation)
        m_implementation = DOMImplementation::create(this);
    return m_implementation.get();
}

void Document::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    ContainerNode::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    
    Element* newDocumentElement = firstElementChild(this);
    if (newDocumentElement == m_documentElement)
        return;
    m_documentElement = newDocumentElement;
    // The root style used for media query matching depends on the document element.
    clearStyleResolver();
}

PassRefPtr<Element> Document::createElement(const AtomicString& name, ExceptionCode& ec)
{
    if (!isValidName(name)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }

    if (m_isXHTML)
        return HTMLElementFactory::createHTMLElement(QualifiedName(nullAtom, name, xhtmlNamespaceURI), this, 0, false);

    return createElement(QualifiedName(nullAtom, name, nullAtom), false);
}

PassRefPtr<DocumentFragment> Document::createDocumentFragment()
{
    return DocumentFragment::create(document());
}

PassRefPtr<Text> Document::createTextNode(const String& data)
{
    return Text::create(this, data);
}

PassRefPtr<Comment> Document::createComment(const String& data)
{
    return Comment::create(this, data);
}

PassRefPtr<CDATASection> Document::createCDATASection(const String& data, ExceptionCode& ec)
{
    if (isHTMLDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return CDATASection::create(this, data);
}

PassRefPtr<ProcessingInstruction> Document::createProcessingInstruction(const String& target, const String& data, ExceptionCode& ec)
{
    if (!isValidName(target)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }
    if (isHTMLDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return ProcessingInstruction::create(this, target, data);
}

PassRefPtr<EntityReference> Document::createEntityReference(const String& name, ExceptionCode& ec)
{
    if (!isValidName(name)) {
        ec = INVALID_CHARACTER_ERR;
        return 0;
    }
    if (isHTMLDocument()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return EntityReference::create(this, name);
}

PassRefPtr<EditingText> Document::createEditingTextNode(const String& text)
{
    return EditingText::create(this, text);
}

PassRefPtr<CSSStyleDeclaration> Document::createCSSStyleDeclaration()
{
    return StylePropertySet::create()->ensureCSSStyleDeclaration();
}

PassRefPtr<Node> Document::importNode(Node* importedNode, bool deep, ExceptionCode& ec)
{
    ec = 0;
    
    if (!importedNode) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    switch (importedNode->nodeType()) {
    case TEXT_NODE:
        return createTextNode(importedNode->nodeValue());
    case CDATA_SECTION_NODE:
        return createCDATASection(importedNode->nodeValue(), ec);
    case ENTITY_REFERENCE_NODE:
        return createEntityReference(importedNode->nodeName(), ec);
    case PROCESSING_INSTRUCTION_NODE:
        return createProcessingInstruction(importedNode->nodeName(), importedNode->nodeValue(), ec);
    case COMMENT_NODE:
        return createComment(importedNode->nodeValue());
    case ELEMENT_NODE: {
        Element* oldElement = static_cast<Element*>(importedNode);
        // FIXME: The following check might be unnecessary. Is it possible that
        // oldElement has mismatched prefix/namespace?
        if (!hasValidNamespaceForElements(oldElement->tagQName())) {
            ec = NAMESPACE_ERR;
            return 0;
        }
        RefPtr<Element> newElement = createElement(oldElement->tagQName(), ec);
        if (ec)
            return 0;

        newElement->cloneDataFromElement(*oldElement);

        if (deep) {
            for (Node* oldChild = oldElement->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
                RefPtr<Node> newChild = importNode(oldChild, true, ec);
                if (ec)
                    return 0;
                newElement->appendChild(newChild.release(), ec);
                if (ec)
                    return 0;
            }
        }

        return newElement.release();
    }
    case ATTRIBUTE_NODE:
        return Attr::create(this, QualifiedName(nullAtom, static_cast<Attr*>(importedNode)->name(), nullAtom), static_cast<Attr*>(importedNode)->value());
    case DOCUMENT_FRAGMENT_NODE: {
        if (importedNode->isShadowRoot()) {
            // ShadowRoot nodes should not be explicitly importable.
            // Either they are imported along with their host node, or created implicitly.
            break;
        }
        DocumentFragment* oldFragment = static_cast<DocumentFragment*>(importedNode);
        RefPtr<DocumentFragment> newFragment = createDocumentFragment();
        if (deep) {
            for (Node* oldChild = oldFragment->firstChild(); oldChild; oldChild = oldChild->nextSibling()) {
                RefPtr<Node> newChild = importNode(oldChild, true, ec);
                if (ec)
                    return 0;
                newFragment->appendChild(newChild.release(), ec);
                if (ec)
                    return 0;
            }
        }
        
        return newFragment.release();
    }
    case ENTITY_NODE:
    case NOTATION_NODE:
        // FIXME: It should be possible to import these node types, however in DOM3 the DocumentType is readonly, so there isn't much sense in doing that.
        // Ability to add these imported nodes to a DocumentType will be considered for addition to a future release of the DOM.
    case DOCUMENT_NODE:
    case DOCUMENT_TYPE_NODE:
    case XPATH_NAMESPACE_NODE:
        break;
    }
    ec = NOT_SUPPORTED_ERR;
    return 0;
}


PassRefPtr<Node> Document::adoptNode(PassRefPtr<Node> source, ExceptionCode& ec)
{
    if (!source) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    if (source->isReadOnlyNode()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return 0;
    }

    EventQueueScope scope;

    switch (source->nodeType()) {
    case ENTITY_NODE:
    case NOTATION_NODE:
    case DOCUMENT_NODE:
    case DOCUMENT_TYPE_NODE:
    case XPATH_NAMESPACE_NODE:
        ec = NOT_SUPPORTED_ERR;
        return 0;            
    case ATTRIBUTE_NODE: {                   
        Attr* attr = static_cast<Attr*>(source.get());
        if (attr->ownerElement())
            attr->ownerElement()->removeAttributeNode(attr, ec);
        attr->setSpecified(true);
        break;
    }       
    default:
        if (source->isShadowRoot()) {
            // ShadowRoot cannot disconnect itself from the host node.
            ec = HIERARCHY_REQUEST_ERR;
            return 0;
        }

        // FIXME: What about <frame> and <object>?
        if (source->hasTagName(iframeTag)) {
            HTMLIFrameElement* iframe = static_cast<HTMLIFrameElement*>(source.get());
            if (frame() && frame()->tree()->isDescendantOf(iframe->contentFrame())) {
                ec = HIERARCHY_REQUEST_ERR;
                return 0;
            }
        }
        if (source->parentNode())
            source->parentNode()->removeChild(source.get(), ec);
    }

    this->adoptIfNeeded(source.get());

    return source;
}

bool Document::hasValidNamespaceForElements(const QualifiedName& qName)
{
    // These checks are from DOM Core Level 2, createElementNS
    // http://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-DocCrElNS
    if (!qName.prefix().isEmpty() && qName.namespaceURI().isNull()) // createElementNS(null, "html:div")
        return false;
    if (qName.prefix() == xmlAtom && qName.namespaceURI() != XMLNames::xmlNamespaceURI) // createElementNS("http://www.example.com", "xml:lang")
        return false;

    // Required by DOM Level 3 Core and unspecified by DOM Level 2 Core:
    // http://www.w3.org/TR/2004/REC-DOM-Level-3-Core-20040407/core.html#ID-DocCrElNS
    // createElementNS("http://www.w3.org/2000/xmlns/", "foo:bar"), createElementNS(null, "xmlns:bar")
    if ((qName.prefix() == xmlnsAtom && qName.namespaceURI() != XMLNSNames::xmlnsNamespaceURI) || (qName.prefix() != xmlnsAtom && qName.namespaceURI() == XMLNSNames::xmlnsNamespaceURI))
        return false;

    return true;
}

bool Document::hasValidNamespaceForAttributes(const QualifiedName& qName)
{
    // Spec: DOM Level 2 Core: http://www.w3.org/TR/DOM-Level-2-Core/core.html#ID-ElSetAttrNS
    if (qName.prefix().isEmpty() && qName.localName() == xmlnsAtom) {
        // Note: The case of an "xmlns" qualified name with a namespace of
        // xmlnsNamespaceURI is specifically allowed (See <http://www.w3.org/2000/xmlns/>).
        return qName.namespaceURI() == XMLNSNames::xmlnsNamespaceURI;
    }
    return hasValidNamespaceForElements(qName);
}

// FIXME: This should really be in a possible ElementFactory class
PassRefPtr<Element> Document::createElement(const QualifiedName& qName, bool createdByParser)
{
    RefPtr<Element> e;

    // FIXME: Use registered namespaces and look up in a hash to find the right factory.
    if (qName.namespaceURI() == xhtmlNamespaceURI)
        e = HTMLElementFactory::createHTMLElement(qName, this, 0, createdByParser);
#if ENABLE(SVG)
    else if (qName.namespaceURI() == SVGNames::svgNamespaceURI)
        e = SVGElementFactory::createSVGElement(qName, this, createdByParser);
#endif
#if ENABLE(MATHML)
    else if (qName.namespaceURI() == MathMLNames::mathmlNamespaceURI)
        e = MathMLElementFactory::createMathMLElement(qName, this, createdByParser);
#endif

    if (e)
        m_sawElementsInKnownNamespaces = true;
    else
        e = Element::create(qName, document());

    // <image> uses imgTag so we need a special rule.
    ASSERT((qName.matches(imageTag) && e->tagQName().matches(imgTag) && e->tagQName().prefix() == qName.prefix()) || qName == e->tagQName());

    return e.release();
}

bool Document::regionBasedColumnsEnabled() const
{
    return settings() && settings()->regionBasedColumnsEnabled(); 
}

bool Document::cssStickyPositionEnabled() const
{
    return settings() && settings()->cssStickyPositionEnabled(); 
}

bool Document::cssRegionsEnabled() const
{
    return settings() && settings()->cssRegionsEnabled(); 
}

bool Document::cssGridLayoutEnabled() const
{
    return settings() && settings()->cssGridLayoutEnabled();
}

#if ENABLE(CSS_REGIONS)

PassRefPtr<WebKitNamedFlow> Document::webkitGetFlowByName(const String& flowName)
{
    if (!cssRegionsEnabled() || !renderer())
        return 0;

    // It's possible to have pending styles not applied that affect the existing flows.
    updateStyleIfNeeded();

    return namedFlows()->flowByName(flowName);
}

PassRefPtr<DOMNamedFlowCollection> Document::webkitGetNamedFlows()
{
    if (!cssRegionsEnabled() || !renderer())
        return 0;

    updateStyleIfNeeded();

    return namedFlows()->createCSSOMSnapshot();
}

#endif

NamedFlowCollection* Document::namedFlows()
{
    if (!m_namedFlows)
        m_namedFlows = NamedFlowCollection::create(this);

    return m_namedFlows.get();
}

PassRefPtr<Element> Document::createElementNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode& ec)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);
    if (!hasValidNamespaceForElements(qName)) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    return createElement(qName, false);
}

String Document::readyState() const
{
    DEFINE_STATIC_LOCAL(const String, loading, (ASCIILiteral("loading")));
    DEFINE_STATIC_LOCAL(const String, interactive, (ASCIILiteral("interactive")));
    DEFINE_STATIC_LOCAL(const String, complete, (ASCIILiteral("complete")));

    switch (m_readyState) {
    case Loading:
        return loading;
    case Interactive:
        return interactive;
    case Complete:
        return complete;
    }

    ASSERT_NOT_REACHED();
    return String();
}

void Document::setReadyState(ReadyState readyState)
{
    if (readyState == m_readyState)
        return;

    switch (readyState) {
    case Loading:
        if (!m_documentTiming.domLoading)
            m_documentTiming.domLoading = monotonicallyIncreasingTime();
        break;
    case Interactive:
        if (!m_documentTiming.domInteractive)
            m_documentTiming.domInteractive = monotonicallyIncreasingTime();
        break;
    case Complete:
        if (!m_documentTiming.domComplete)
            m_documentTiming.domComplete = monotonicallyIncreasingTime();
        break;
    }

    m_readyState = readyState;
    dispatchEvent(Event::create(eventNames().readystatechangeEvent, false, false));
    
    if (settings() && settings()->suppressesIncrementalRendering())
        setVisualUpdatesAllowed(readyState);
}

void Document::setVisualUpdatesAllowed(ReadyState readyState)
{
    ASSERT(settings() && settings()->suppressesIncrementalRendering());
    switch (readyState) {
    case Loading:
        ASSERT(!m_visualUpdatesSuppressionTimer.isActive());
        ASSERT(m_visualUpdatesAllowed);
        m_visualUpdatesSuppressionTimer.startOneShot(settings()->incrementalRenderingSuppressionTimeoutInSeconds());
        setVisualUpdatesAllowed(false);
        break;
    case Interactive:
        ASSERT(m_visualUpdatesSuppressionTimer.isActive() || m_visualUpdatesAllowed);
        break;
    case Complete:
        if (m_visualUpdatesSuppressionTimer.isActive()) {
            ASSERT(!m_visualUpdatesAllowed);
            m_visualUpdatesSuppressionTimer.stop();
            setVisualUpdatesAllowed(true);
        } else
            ASSERT(m_visualUpdatesAllowed);
        break;
    }
}
    
void Document::setVisualUpdatesAllowed(bool visualUpdatesAllowed)
{
    if (m_visualUpdatesAllowed == visualUpdatesAllowed)
        return;

    m_visualUpdatesAllowed = visualUpdatesAllowed;

    if (!visualUpdatesAllowed)
        return;

#if USE(ACCELERATED_COMPOSITING)
    if (view())
        view()->updateCompositingLayersAfterLayout();
#endif

    if (renderer())
        renderer()->repaint();
}

void Document::visualUpdatesSuppressionTimerFired(Timer<Document>*)
{
    ASSERT(!m_visualUpdatesAllowed);
    setVisualUpdatesAllowed(true);
}

String Document::encoding() const
{
    if (TextResourceDecoder* d = decoder())
        return d->encoding().domName();
    return String();
}

String Document::defaultCharset() const
{
    if (Settings* settings = this->settings())
        return settings->defaultTextEncodingName();
    return String();
}

void Document::setCharset(const String& charset)
{
    if (!decoder())
        return;
    decoder()->setEncoding(charset, TextResourceDecoder::UserChosenEncoding);
}

void Document::setContentLanguage(const String& language)
{
    if (m_contentLanguage == language)
        return;
    m_contentLanguage = language;

    // Recalculate style so language is used when selecting the initial font.
    styleResolverChanged(DeferRecalcStyle);
}

void Document::setXMLVersion(const String& version, ExceptionCode& ec)
{
    if (!implementation()->hasFeature("XML", String())) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    if (!XMLDocumentParser::supportsXMLVersion(version)) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_xmlVersion = version;
}

void Document::setXMLStandalone(bool standalone, ExceptionCode& ec)
{
    if (!implementation()->hasFeature("XML", String())) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_xmlStandalone = standalone ? Standalone : NotStandalone;
}

void Document::setDocumentURI(const String& uri)
{
    // This property is read-only from JavaScript, but writable from Objective-C.
    m_documentURI = uri;
    updateBaseURL();
}

KURL Document::baseURI() const
{
    return m_baseURL;
}

void Document::setContent(const String& content)
{
    open();
    m_parser->append(content);
    close();
}

String Document::suggestedMIMEType() const
{
    if (m_document->isXHTMLDocument())
        return "application/xhtml+xml";
    if (m_document->isSVGDocument())
        return "image/svg+xml";
    if (m_document->xmlStandalone())
        return "text/xml";
    if (m_document->isHTMLDocument())
        return "text/html";

    if (DocumentLoader* documentLoader = loader())
        return documentLoader->responseMIMEType();
    return String();
}

// FIXME: We need to discuss the DOM API here at some point. Ideas:
// * making it receive a rect as parameter, i.e. nodesFromRect(x, y, w, h);
// * making it receive the expading size of each direction separately,
//   i.e. nodesFromRect(x, y, topSize, rightSize, bottomSize, leftSize);
PassRefPtr<NodeList> Document::nodesFromRect(int centerX, int centerY, unsigned topPadding, unsigned rightPadding, unsigned bottomPadding, unsigned leftPadding, bool ignoreClipping, bool allowShadowContent) const
{
    // FIXME: Share code between this, elementFromPoint and caretRangeFromPoint.
    if (!renderer())
        return 0;
    Frame* frame = this->frame();
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;

    float zoomFactor = frame->pageZoomFactor();
    LayoutPoint point = roundedLayoutPoint(FloatPoint(centerX * zoomFactor + view()->scrollX(), centerY * zoomFactor + view()->scrollY()));

    int type = HitTestRequest::ReadOnly | HitTestRequest::Active;

    // When ignoreClipping is false, this method returns null for coordinates outside of the viewport.
    if (ignoreClipping)
        type |= HitTestRequest::IgnoreClipping;
    else if (!frameView->visibleContentRect().intersects(HitTestResult::rectForPoint(point, topPadding, rightPadding, bottomPadding, leftPadding)))
        return 0;
    if (allowShadowContent)
        type |= HitTestRequest::AllowShadowContent;

    HitTestRequest request(type);

    // Passing a zero padding will trigger a rect hit test, however for the purposes of nodesFromRect,
    // we special handle this case in order to return a valid NodeList.
    if (!topPadding && !rightPadding && !bottomPadding && !leftPadding) {
        HitTestResult result(point);
        return handleZeroPadding(request, result);
    }

    HitTestResult result(point, topPadding, rightPadding, bottomPadding, leftPadding);
    renderView()->hitTest(request, result);

    return StaticHashSetNodeList::adopt(result.rectBasedTestResult());
}

PassRefPtr<NodeList> Document::handleZeroPadding(const HitTestRequest& request, HitTestResult& result) const
{
    renderView()->hitTest(request, result);

    Node* node = result.innerNode();
    if (!node)
        return 0;

    node = node->shadowAncestorNode();
    ListHashSet<RefPtr<Node> > list;
    list.add(node);
    return StaticHashSetNodeList::adopt(list);
}

static Node* nodeFromPoint(Frame* frame, RenderView* renderView, int x, int y, LayoutPoint* localPoint = 0)
{
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;

    float zoomFactor = frame->pageZoomFactor();
    IntPoint point = roundedIntPoint(FloatPoint(x * zoomFactor  + frameView->scrollX(), y * zoomFactor + frameView->scrollY()));

    if (!frameView->visibleContentRect().contains(point))
        return 0;

    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active);
    HitTestResult result(point);
    renderView->hitTest(request, result);

    if (localPoint)
        *localPoint = result.localPoint();

    return result.innerNode();
}

Element* Document::elementFromPoint(int x, int y) const
{
    if (!renderer())
        return 0;
    Node* node = nodeFromPoint(frame(), renderView(), x, y);
    while (node && !node->isElementNode())
        node = node->parentNode();
    if (node)
        node = ancestorInThisScope(node);
    return static_cast<Element*>(node);
}

PassRefPtr<Range> Document::caretRangeFromPoint(int x, int y)
{
    if (!renderer())
        return 0;
    LayoutPoint localPoint;
    Node* node = nodeFromPoint(frame(), renderView(), x, y, &localPoint);
    if (!node)
        return 0;

    Node* shadowAncestorNode = ancestorInThisScope(node);
    if (shadowAncestorNode != node) {
        unsigned offset = shadowAncestorNode->nodeIndex();
        ContainerNode* container = shadowAncestorNode->parentNode();
        return Range::create(this, container, offset, container, offset);
    }

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;
    VisiblePosition visiblePosition = renderer->positionForPoint(localPoint);
    if (visiblePosition.isNull())
        return 0;

    Position rangeCompliantPosition = visiblePosition.deepEquivalent().parentAnchoredEquivalent();
    return Range::create(this, rangeCompliantPosition, rangeCompliantPosition);
}

/*
 * Performs three operations:
 *  1. Convert control characters to spaces
 *  2. Trim leading and trailing spaces
 *  3. Collapse internal whitespace.
 */
static inline StringWithDirection canonicalizedTitle(Document* document, const StringWithDirection& titleWithDirection)
{
    const String& title = titleWithDirection.string();
    const UChar* characters = title.characters();
    unsigned length = title.length();
    unsigned i;

    StringBuffer<UChar> buffer(length);
    unsigned builderIndex = 0;

    // Skip leading spaces and leading characters that would convert to spaces
    for (i = 0; i < length; ++i) {
        UChar c = characters[i];
        if (!(c <= 0x20 || c == 0x7F))
            break;
    }

    if (i == length)
        return StringWithDirection();

    // Replace control characters with spaces, and backslashes with currency symbols, and collapse whitespace.
    bool previousCharWasWS = false;
    for (; i < length; ++i) {
        UChar c = characters[i];
        if (c <= 0x20 || c == 0x7F || (WTF::Unicode::category(c) & (WTF::Unicode::Separator_Line | WTF::Unicode::Separator_Paragraph))) {
            if (previousCharWasWS)
                continue;
            buffer[builderIndex++] = ' ';
            previousCharWasWS = true;
        } else {
            buffer[builderIndex++] = c;
            previousCharWasWS = false;
        }
    }

    // Strip trailing spaces
    while (builderIndex > 0) {
        --builderIndex;
        if (buffer[builderIndex] != ' ')
            break;
    }

    if (!builderIndex && buffer[builderIndex] == ' ')
        return StringWithDirection();

    buffer.shrink(builderIndex + 1);

    // Replace the backslashes with currency symbols if the encoding requires it.
    document->displayBufferModifiedByEncoding(buffer.characters(), buffer.length());
    
    return StringWithDirection(String::adopt(buffer), titleWithDirection.direction());
}

void Document::updateTitle(const StringWithDirection& title)
{
    if (m_rawTitle == title)
        return;

    m_rawTitle = title;
    m_title = canonicalizedTitle(this, m_rawTitle);
    if (Frame* f = frame())
        f->loader()->setTitle(m_title);
}

void Document::setTitle(const String& title)
{
    // Title set by JavaScript -- overrides any title elements.
    m_titleSetExplicitly = true;
    if (!isHTMLDocument() && !isXHTMLDocument())
        m_titleElement = 0;
    else if (!m_titleElement) {
        if (HTMLElement* headElement = head()) {
            m_titleElement = createElement(titleTag, false);
            ExceptionCode ec = 0;
            headElement->appendChild(m_titleElement, ec);
            ASSERT(!ec);
        }
    }

    // The DOM API has no method of specifying direction, so assume LTR.
    updateTitle(StringWithDirection(title, LTR));

    if (m_titleElement) {
        ASSERT(m_titleElement->hasTagName(titleTag));
        if (m_titleElement->hasTagName(titleTag))
            static_cast<HTMLTitleElement*>(m_titleElement.get())->setText(title);
    }
}

void Document::setTitleElement(const StringWithDirection& title, Element* titleElement)
{
    if (titleElement != m_titleElement) {
        if (m_titleElement || m_titleSetExplicitly)
            // Only allow the first title element to change the title -- others have no effect.
            return;
        m_titleElement = titleElement;
    }

    updateTitle(title);
}

void Document::removeTitle(Element* titleElement)
{
    if (m_titleElement != titleElement)
        return;

    m_titleElement = 0;
    m_titleSetExplicitly = false;

    // Update title based on first title element in the head, if one exists.
    if (HTMLElement* headElement = head()) {
        for (Node* e = headElement->firstChild(); e; e = e->nextSibling())
            if (e->hasTagName(titleTag)) {
                HTMLTitleElement* titleElement = static_cast<HTMLTitleElement*>(e);
                setTitleElement(titleElement->textWithDirection(), titleElement);
                break;
            }
    }

    if (!m_titleElement)
        updateTitle(StringWithDirection());
}

#if ENABLE(PAGE_VISIBILITY_API)
PageVisibilityState Document::visibilityState() const
{
    // The visibility of the document is inherited from the visibility of the
    // page. If there is no page associated with the document, we will assume
    // that the page is visible i.e. invisibility has to be explicitly
    // specified by the embedder.
    if (!m_frame || !m_frame->page())
        return PageVisibilityStateVisible;
    return m_frame->page()->visibilityState();
}

String Document::webkitVisibilityState() const
{
    return pageVisibilityStateString(visibilityState());
}

bool Document::webkitHidden() const
{
    return visibilityState() != PageVisibilityStateVisible;
}

void Document::dispatchVisibilityStateChangeEvent()
{
    dispatchEvent(Event::create(eventNames().webkitvisibilitychangeEvent, false, false));
}
#endif

#if ENABLE(CSP_NEXT)
DOMSecurityPolicy* Document::securityPolicy()
{
    if (!m_domSecurityPolicy)
        m_domSecurityPolicy = DOMSecurityPolicy::create(this);
    return m_domSecurityPolicy.get();
}
#endif

String Document::nodeName() const
{
    return "#document";
}

Node::NodeType Document::nodeType() const
{
    return DOCUMENT_NODE;
}

FormController* Document::formController()
{
    if (!m_formController)
        m_formController = FormController::create();
    return m_formController.get();
}

Vector<String> Document::formElementsState() const
{
    if (!m_formController)
        return Vector<String>();
    return m_formController->formElementsState();
}

void Document::setStateForNewFormElements(const Vector<String>& stateVector)
{
    if (!stateVector.size() && !m_formController)
        return;
    formController()->setStateForNewFormElements(stateVector);
}

FrameView* Document::view() const
{
    return m_frame ? m_frame->view() : 0;
}

Page* Document::page() const
{
    return m_frame ? m_frame->page() : 0;    
}

Settings* Document::settings() const
{
    return m_frame ? m_frame->settings() : 0;
}

PassRefPtr<Range> Document::createRange()
{
    return Range::create(this);
}

PassRefPtr<NodeIterator> Document::createNodeIterator(Node* root, unsigned whatToShow, 
    PassRefPtr<NodeFilter> filter, bool expandEntityReferences, ExceptionCode& ec)
{
    if (!root) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return NodeIterator::create(root, whatToShow, filter, expandEntityReferences);
}

PassRefPtr<TreeWalker> Document::createTreeWalker(Node* root, unsigned whatToShow, 
    PassRefPtr<NodeFilter> filter, bool expandEntityReferences, ExceptionCode& ec)
{
    if (!root) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }
    return TreeWalker::create(root, whatToShow, filter, expandEntityReferences);
}

void Document::scheduleForcedStyleRecalc()
{
    m_pendingStyleRecalcShouldForce = true;
    scheduleStyleRecalc();
}

void Document::scheduleStyleRecalc()
{
    if (shouldDisplaySeamlesslyWithParent()) {
        // When we're seamless, our parent document manages our style recalcs.
        ownerElement()->setNeedsStyleRecalc();
        ownerElement()->document()->scheduleStyleRecalc();
        return;
    }

    if (m_styleRecalcTimer.isActive() || inPageCache())
        return;

    ASSERT(childNeedsStyleRecalc() || m_pendingStyleRecalcShouldForce);

    if (!documentsThatNeedStyleRecalc)
        documentsThatNeedStyleRecalc = new HashSet<Document*>;
    documentsThatNeedStyleRecalc->add(this);
    
    // FIXME: Why on earth is this here? This is clearly misplaced.
    invalidateAccessKeyMap();
    
    m_styleRecalcTimer.startOneShot(0);

    InspectorInstrumentation::didScheduleStyleRecalculation(this);
}

void Document::unscheduleStyleRecalc()
{
    ASSERT(!childNeedsStyleRecalc());

    if (documentsThatNeedStyleRecalc)
        documentsThatNeedStyleRecalc->remove(this);

    m_styleRecalcTimer.stop();
    m_pendingStyleRecalcShouldForce = false;
}

bool Document::isPendingStyleRecalc() const
{
    return m_styleRecalcTimer.isActive() && !m_inStyleRecalc;
}

void Document::styleRecalcTimerFired(Timer<Document>*)
{
    updateStyleIfNeeded();
}

bool Document::childNeedsAndNotInStyleRecalc()
{
    return childNeedsStyleRecalc() && !m_inStyleRecalc;
}

void Document::recalcStyle(StyleChange change)
{
    // we should not enter style recalc while painting
    ASSERT(!view() || !view()->isPainting());
    if (view() && view()->isPainting())
        return;
    
    if (m_inStyleRecalc)
        return; // Guard against re-entrancy. -dwh

    // FIXME: We should update style on our ancestor chain before proceeding (especially for seamless),
    // however doing so currently causes several tests to crash, as Frame::setDocument calls Document::attach
    // before setting the DOMWindow on the Frame, or the SecurityOrigin on the document. The attach, in turn
    // resolves style (here) and then when we resolve style on the parent chain, we may end up
    // re-attaching our containing iframe, which when asked HTMLFrameElementBase::isURLAllowed
    // hits a null-dereference due to security code always assuming the document has a SecurityOrigin.

    if (m_hasDirtyStyleResolver)
        updateActiveStylesheets(RecalcStyleImmediately);

    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willRecalculateStyle(this);

    if (m_elemSheet && m_elemSheet->contents()->usesRemUnits())
        m_usesRemUnits = true;

    m_inStyleRecalc = true;
    suspendPostAttachCallbacks();
    RenderWidget::suspendWidgetHierarchyUpdates();
    
    RefPtr<FrameView> frameView = view();
    if (frameView) {
        frameView->pauseScheduledEvents();
        frameView->beginDeferredRepaints();
    }

    ASSERT(!renderer() || renderArena());
    if (!renderer() || !renderArena())
        goto bail_out;

    if (m_pendingStyleRecalcShouldForce)
        change = Force;

    // Recalculating the root style (on the document) is not needed in the common case.
    if ((change == Force) || (shouldDisplaySeamlesslyWithParent() && (change >= Inherit))) {
        // style selector may set this again during recalc
        m_hasNodesWithPlaceholderStyle = false;
        
        RefPtr<RenderStyle> documentStyle = StyleResolver::styleForDocument(this, m_styleResolver ? m_styleResolver->fontSelector() : 0);
        StyleChange ch = Node::diff(documentStyle.get(), renderer()->style(), this);
        if (ch != NoChange)
            renderer()->setStyle(documentStyle.release());
    }

    for (Node* n = firstChild(); n; n = n->nextSibling()) {
        if (!n->isElementNode())
            continue;
        Element* element = static_cast<Element*>(n);
        if (change >= Inherit || element->childNeedsStyleRecalc() || element->needsStyleRecalc())
            element->recalcStyle(change);
    }

#if USE(ACCELERATED_COMPOSITING)
    if (view()) {
        bool layoutPending = view()->layoutPending() || renderer()->needsLayout();
        // If we didn't update compositing layers because of layout(), we need to do so here.
        if (!layoutPending)
            view()->updateCompositingLayersAfterStyleChange();
    }
#endif

bail_out:
    clearNeedsStyleRecalc();
    clearChildNeedsStyleRecalc();
    unscheduleStyleRecalc();

    m_inStyleRecalc = false;
    
    // Pseudo element removal and similar may only work with these flags still set. Reset them after the style recalc.
    if (m_styleResolver)
        resetCSSFeatureFlags();

    if (frameView) {
        frameView->resumeScheduledEvents();
        frameView->endDeferredRepaints();
    }
    RenderWidget::resumeWidgetHierarchyUpdates();
    resumePostAttachCallbacks();

    // If we wanted to call implicitClose() during recalcStyle, do so now that we're finished.
    if (m_closeAfterStyleRecalc) {
        m_closeAfterStyleRecalc = false;
        implicitClose();
    }

    InspectorInstrumentation::didRecalculateStyle(cookie);
}

void Document::updateStyleIfNeeded()
{
    ASSERT(isMainThread());
    ASSERT(!view() || (!view()->isInLayout() && !view()->isPainting()));
    
    if ((!m_pendingStyleRecalcShouldForce && !childNeedsStyleRecalc()) || inPageCache())
        return;

    AnimationUpdateBlock animationUpdateBlock(m_frame ? m_frame->animation() : 0);
    recalcStyle(NoChange);
}

void Document::updateStyleForAllDocuments()
{
    ASSERT(isMainThread());
    if (!documentsThatNeedStyleRecalc)
        return;

    while (documentsThatNeedStyleRecalc->size()) {
        HashSet<Document*>::iterator it = documentsThatNeedStyleRecalc->begin();
        Document* doc = *it;
        documentsThatNeedStyleRecalc->remove(doc);
        doc->updateStyleIfNeeded();
    }
}

void Document::updateLayout()
{
    ASSERT(isMainThread());
    if (Element* oe = ownerElement())
        oe->document()->updateLayout();

    updateStyleIfNeeded();

    // Only do a layout if changes have occurred that make it necessary.      
    FrameView* v = view();
    if (v && renderer() && (v->layoutPending() || renderer()->needsLayout()))
        v->layout();
}

// FIXME: This is a bad idea and needs to be removed eventually.
// Other browsers load stylesheets before they continue parsing the web page.
// Since we don't, we can run JavaScript code that needs answers before the
// stylesheets are loaded. Doing a layout ignoring the pending stylesheets
// lets us get reasonable answers. The long term solution to this problem is
// to instead suspend JavaScript execution.
void Document::updateLayoutIgnorePendingStylesheets()
{
    bool oldIgnore = m_ignorePendingStylesheets;
    
    if (!haveStylesheetsLoaded()) {
        m_ignorePendingStylesheets = true;
        // FIXME: We are willing to attempt to suppress painting with outdated style info only once.  Our assumption is that it would be
        // dangerous to try to stop it a second time, after page content has already been loaded and displayed
        // with accurate style information.  (Our suppression involves blanking the whole page at the
        // moment.  If it were more refined, we might be able to do something better.)
        // It's worth noting though that this entire method is a hack, since what we really want to do is
        // suspend JS instead of doing a layout with inaccurate information.
        HTMLElement* bodyElement = body();
        if (bodyElement && !bodyElement->renderer() && m_pendingSheetLayout == NoLayoutWithPendingSheets) {
            m_pendingSheetLayout = DidLayoutWithPendingSheets;
            styleResolverChanged(RecalcStyleImmediately);
        } else if (m_hasNodesWithPlaceholderStyle)
            // If new nodes have been added or style recalc has been done with style sheets still pending, some nodes 
            // may not have had their real style calculated yet. Normally this gets cleaned when style sheets arrive 
            // but here we need up-to-date style immediately.
            recalcStyle(Force);
    }

    updateLayout();

    m_ignorePendingStylesheets = oldIgnore;
}

PassRefPtr<RenderStyle> Document::styleForElementIgnoringPendingStylesheets(Element* element)
{
    ASSERT_ARG(element, element->document() == this);

    bool oldIgnore = m_ignorePendingStylesheets;
    m_ignorePendingStylesheets = true;
    RefPtr<RenderStyle> style = styleResolver()->styleForElement(element, element->parentNode() ? element->parentNode()->computedStyle() : 0);
    m_ignorePendingStylesheets = oldIgnore;
    return style.release();
}

PassRefPtr<RenderStyle> Document::styleForPage(int pageIndex)
{
    RefPtr<RenderStyle> style = styleResolver()->styleForPage(pageIndex);
    return style.release();
}

void Document::registerCustomFont(PassOwnPtr<FontData> fontData)
{
    m_customFonts.append(fontData);
}

void Document::deleteCustomFonts()
{
    size_t size = m_customFonts.size();
    for (size_t i = 0; i < size; ++i)
        GlyphPageTreeNode::pruneTreeCustomFontData(m_customFonts[i].get());

    m_customFonts.clear();
}

bool Document::isPageBoxVisible(int pageIndex)
{
    RefPtr<RenderStyle> style = styleForPage(pageIndex);
    return style->visibility() != HIDDEN; // display property doesn't apply to @page.
}

void Document::pageSizeAndMarginsInPixels(int pageIndex, IntSize& pageSize, int& marginTop, int& marginRight, int& marginBottom, int& marginLeft)
{
    RefPtr<RenderStyle> style = styleForPage(pageIndex);
    RenderView* view = renderView();

    int width = pageSize.width();
    int height = pageSize.height();
    switch (style->pageSizeType()) {
    case PAGE_SIZE_AUTO:
        break;
    case PAGE_SIZE_AUTO_LANDSCAPE:
        if (width < height)
            std::swap(width, height);
        break;
    case PAGE_SIZE_AUTO_PORTRAIT:
        if (width > height)
            std::swap(width, height);
        break;
    case PAGE_SIZE_RESOLVED: {
        LengthSize size = style->pageSize();
        ASSERT(size.width().isFixed());
        ASSERT(size.height().isFixed());
        width = valueForLength(size.width(), 0, view);
        height = valueForLength(size.height(), 0, view);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
    pageSize = IntSize(width, height);

    // The percentage is calculated with respect to the width even for margin top and bottom.
    // http://www.w3.org/TR/CSS2/box.html#margin-properties
    marginTop = style->marginTop().isAuto() ? marginTop : intValueForLength(style->marginTop(), width, view);
    marginRight = style->marginRight().isAuto() ? marginRight : intValueForLength(style->marginRight(), width, view);
    marginBottom = style->marginBottom().isAuto() ? marginBottom : intValueForLength(style->marginBottom(), width, view);
    marginLeft = style->marginLeft().isAuto() ? marginLeft : intValueForLength(style->marginLeft(), width, view);
}

void Document::setDocumentRareData(NodeRareData* rareData)
{
    m_documentRareData = rareData;
}

void Document::setIsViewSource(bool isViewSource)
{
    m_isViewSource = isViewSource;
    if (!m_isViewSource)
        return;

    setSecurityOrigin(SecurityOrigin::createUnique());
    didUpdateSecurityOrigin();
}

void Document::combineCSSFeatureFlags()
{
    // Delay resetting the flags until after next style recalc since unapplying the style may not work without these set (this is true at least with before/after).
    m_usesSiblingRules = m_usesSiblingRules || m_styleResolver->usesSiblingRules();
    m_usesFirstLineRules = m_usesFirstLineRules || m_styleResolver->usesFirstLineRules();
    m_usesBeforeAfterRules = m_usesBeforeAfterRules || m_styleResolver->usesBeforeAfterRules();
    m_usesLinkRules = m_usesLinkRules || m_styleResolver->usesLinkRules();
}

void Document::resetCSSFeatureFlags()
{
    m_usesSiblingRules = m_styleResolver->usesSiblingRules();
    m_usesFirstLineRules = m_styleResolver->usesFirstLineRules();
    m_usesBeforeAfterRules = m_styleResolver->usesBeforeAfterRules();
    m_usesLinkRules = m_styleResolver->usesLinkRules();
}

void Document::createStyleResolver()
{
    bool matchAuthorAndUserStyles = true;
    if (Settings* docSettings = settings())
        matchAuthorAndUserStyles = docSettings->authorAndUserStylesEnabled();
    m_styleResolver = adoptPtr(new StyleResolver(this, matchAuthorAndUserStyles));
    combineCSSFeatureFlags();
}

inline void Document::clearStyleResolver()
{
    m_styleResolver.clear();
}

void Document::attach()
{
    ASSERT(!attached());
    ASSERT(!m_inPageCache);
    ASSERT(!m_axObjectCache || this != topDocument());

    if (!m_renderArena)
        m_renderArena = adoptPtr(new RenderArena);
    
    // Create the rendering tree
    setRenderer(new (m_renderArena.get()) RenderView(this, view()));
#if USE(ACCELERATED_COMPOSITING)
    renderView()->didMoveOnscreen();
#endif

    recalcStyle(Force);

    RenderObject* render = renderer();
    setRenderer(0);

    ContainerNode::attach();

    setRenderer(render);
}

void Document::detach()
{
    ASSERT(attached());
    ASSERT(!m_inPageCache);

#if ENABLE(POINTER_LOCK)
    if (page())
        page()->pointerLockController()->documentDetached(this);
#endif

    if (this == topDocument())
        clearAXObjectCache();

    stopActiveDOMObjects();
    m_eventQueue->close();
#if ENABLE(FULLSCREEN_API)
    m_fullScreenChangeEventTargetQueue.clear();
    m_fullScreenErrorEventTargetQueue.clear();
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME)
    // FIXME: consider using ActiveDOMObject.
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->clearDocumentPointer();
    m_scriptedAnimationController.clear();
#endif

    RenderObject* render = renderer();

    documentWillBecomeInactive();

#if ENABLE(SHARED_WORKERS)
    SharedWorkerRepository::documentDetached(this);
#endif

    if (m_frame) {
        FrameView* view = m_frame->view();
        if (view)
            view->detachCustomScrollbars();

    }

    // indicate destruction mode,  i.e. attached() but renderer == 0
    setRenderer(0);
    
#if ENABLE(FULLSCREEN_API)
    if (m_fullScreenRenderer)
        setFullScreenRenderer(0);
#endif

    m_hoverNode = 0;
    m_focusedNode = 0;
    m_activeNode = 0;

    ContainerNode::detach();

    unscheduleStyleRecalc();

    if (render)
        render->destroy();

    // This is required, as our Frame might delete itself as soon as it detaches
    // us. However, this violates Node::detach() semantics, as it's never
    // possible to re-attach. Eventually Document::detach() should be renamed,
    // or this setting of the frame to 0 could be made explicit in each of the
    // callers of Document::detach().
    m_frame = 0;
    m_renderArena.clear();
}

void Document::prepareForDestruction()
{
    disconnectDescendantFrames();
    if (DOMWindow* window = this->domWindow())
        window->willDetachDocumentFromFrame();
    detach();
}

void Document::removeAllEventListeners()
{
    EventTarget::removeAllEventListeners();

    if (DOMWindow* domWindow = this->domWindow())
        domWindow->removeAllEventListeners();
    for (Node* node = firstChild(); node; node = node->traverseNextNode())
        node->removeAllEventListeners();
}

void Document::suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension why)
{
    ScriptExecutionContext::suspendActiveDOMObjects(why);

#if ENABLE(DEVICE_ORIENTATION)
    if (!page())
        return;

    if (DeviceMotionController* controller = DeviceMotionController::from(page()))
        controller->suspendEventsForAllListeners(domWindow());
    if (DeviceOrientationController* controller = DeviceOrientationController::from(page()))
        controller->suspendEventsForAllListeners(domWindow());

#endif
}

void Document::resumeActiveDOMObjects()
{
    ScriptExecutionContext::resumeActiveDOMObjects();

#if ENABLE(DEVICE_ORIENTATION)
    if (!page())
        return;

    if (DeviceMotionController* controller = DeviceMotionController::from(page()))
        controller->resumeEventsForAllListeners(domWindow());
    if (DeviceOrientationController* controller = DeviceOrientationController::from(page()))
        controller->resumeEventsForAllListeners(domWindow());
#endif
}

RenderView* Document::renderView() const
{
    return toRenderView(renderer());
}

void Document::clearAXObjectCache()
{
    // clear cache in top document
    if (m_axObjectCache) {
        // Clear the cache member variable before calling delete because attempts
        // are made to access it during destruction.
        AXObjectCache* axObjectCache = m_axObjectCache;
        m_axObjectCache = 0;
        delete axObjectCache;
        return;
    }
    
    // ask the top-level document to clear its cache
    Document* doc = topDocument();
    if (doc != this)
        doc->clearAXObjectCache();
}

bool Document::axObjectCacheExists() const
{
    if (m_axObjectCache)
        return true;
    
    Document* doc = topDocument();
    if (doc != this)
        return doc->axObjectCacheExists();
    
    return false;
}
    
AXObjectCache* Document::axObjectCache() const
{
    // The only document that actually has a AXObjectCache is the top-level
    // document.  This is because we need to be able to get from any WebCoreAXObject
    // to any other WebCoreAXObject on the same page.  Using a single cache allows
    // lookups across nested webareas (i.e. multiple documents).
    
    if (m_axObjectCache) {
        // return already known top-level cache
        if (!ownerElement())
            return m_axObjectCache;
        
        // In some pages with frames, the cache is created before the sub-webarea is
        // inserted into the tree.  Here, we catch that case and just toss the old
        // cache and start over.
        // NOTE: This recovery may no longer be needed. I have been unable to trigger
        // it again. See rdar://5794454
        // FIXME: Can this be fixed when inserting the subframe instead of now?
        // FIXME: If this function was called to get the cache in order to remove
        // an AXObject, we are now deleting the cache as a whole and returning a
        // new empty cache that does not contain the AXObject! That should actually
        // be OK. I am concerned about other cases like this where accessing the
        // cache blows away the AXObject being operated on.
        delete m_axObjectCache;
        m_axObjectCache = 0;
    }

    // ask the top-level document for its cache
    Document* doc = topDocument();
    if (doc != this)
        return doc->axObjectCache();
    
    // this is the top-level document, so install a new cache
    m_axObjectCache = new AXObjectCache(this);
    return m_axObjectCache;
}

void Document::setVisuallyOrdered()
{
    m_visuallyOrdered = true;
    if (renderer())
        renderer()->style()->setRTLOrdering(VisualOrder);
}

PassRefPtr<DocumentParser> Document::createParser()
{
    // FIXME: this should probably pass the frame instead
#if ENABLE(NEW_XML)
    return NewXMLDocumentParser::create(this);
#else
    return XMLDocumentParser::create(this, view());
#endif
}

ScriptableDocumentParser* Document::scriptableDocumentParser() const
{
    return parser() ? parser()->asScriptableDocumentParser() : 0;
}

void Document::open(Document* ownerDocument)
{
    if (ownerDocument) {
        setURL(ownerDocument->url());
        m_cookieURL = ownerDocument->cookieURL();
        setSecurityOrigin(ownerDocument->securityOrigin());
    }

    if (m_frame) {
        if (ScriptableDocumentParser* parser = scriptableDocumentParser()) {
            if (parser->isParsing()) {
                // FIXME: HTML5 doesn't tell us to check this, it might not be correct.
                if (parser->isExecutingScript())
                    return;

                if (!parser->wasCreatedByScript() && parser->hasInsertionPoint())
                    return;
            }
        }

        if (m_frame->loader()->state() == FrameStateProvisional)
            m_frame->loader()->stopAllLoaders();
    }

    removeAllEventListeners();
    implicitOpen();
    if (ScriptableDocumentParser* parser = scriptableDocumentParser())
        parser->setWasCreatedByScript(true);

    if (DOMWindow* domWindow = this->domWindow())
        domWindow->removeAllEventListeners();

    if (m_frame)
        m_frame->loader()->didExplicitOpen();
}

void Document::detachParser()
{
    if (!m_parser)
        return;
    m_parser->detach();
    m_parser.clear();
}

void Document::cancelParsing()
{
    if (!m_parser)
        return;

    // We have to clear the parser to avoid possibly triggering
    // the onload handler when closing as a side effect of a cancel-style
    // change, such as opening a new document or closing the window while
    // still parsing
    detachParser();
    explicitClose();
}

void Document::implicitOpen()
{
    cancelParsing();

    removeChildren();

    setCompatibilityMode(NoQuirksMode);

    m_parser = createParser();
    setParsing(true);
    setReadyState(Loading);
}

HTMLElement* Document::body() const
{
    Node* de = documentElement();
    if (!de)
        return 0;
    
    // try to prefer a FRAMESET element over BODY
    Node* body = 0;
    for (Node* i = de->firstChild(); i; i = i->nextSibling()) {
        if (i->hasTagName(framesetTag))
            return toHTMLElement(i);
        
        if (i->hasTagName(bodyTag) && !body)
            body = i;
    }
    return toHTMLElement(body);
}

void Document::setBody(PassRefPtr<HTMLElement> prpNewBody, ExceptionCode& ec)
{
    RefPtr<HTMLElement> newBody = prpNewBody;

    if (!newBody || !documentElement() || !newBody->hasTagName(bodyTag)) { 
        ec = HIERARCHY_REQUEST_ERR;
        return;
    }

    if (newBody->document() && newBody->document() != this) {
        ec = 0;
        RefPtr<Node> node = importNode(newBody.get(), true, ec);
        if (ec)
            return;
        
        newBody = toHTMLElement(node.get());
    }

    HTMLElement* b = body();
    if (!b)
        documentElement()->appendChild(newBody.release(), ec);
    else
        documentElement()->replaceChild(newBody.release(), b, ec);
}

HTMLHeadElement* Document::head()
{
    Node* de = documentElement();
    if (!de)
        return 0;

    for (Node* e = de->firstChild(); e; e = e->nextSibling())
        if (e->hasTagName(headTag))
            return static_cast<HTMLHeadElement*>(e);

    return 0;
}

void Document::close()
{
    // FIXME: We should follow the specification more closely:
    //        http://www.whatwg.org/specs/web-apps/current-work/#dom-document-close

    if (!scriptableDocumentParser() || !scriptableDocumentParser()->wasCreatedByScript() || !scriptableDocumentParser()->isParsing())
        return;

    explicitClose();
}

void Document::explicitClose()
{
    if (m_parser)
        m_parser->finish();

    if (!m_frame) {
        // Because we have no frame, we don't know if all loading has completed,
        // so we just call implicitClose() immediately. FIXME: This might fire
        // the load event prematurely <http://bugs.webkit.org/show_bug.cgi?id=14568>.
        implicitClose();
        return;
    }

    m_frame->loader()->checkCompleted();
}

void Document::implicitClose()
{
    // If we're in the middle of recalcStyle, we need to defer the close until the style information is accurate and all elements are re-attached.
    if (m_inStyleRecalc) {
        m_closeAfterStyleRecalc = true;
        return;
    }

    bool wasLocationChangePending = frame() && frame()->navigationScheduler()->locationChangePending();
    bool doload = !parsing() && m_parser && !m_processingLoadEvent && !wasLocationChangePending;
    
    if (!doload)
        return;

    m_processingLoadEvent = true;

    ScriptableDocumentParser* parser = scriptableDocumentParser();
    m_wellFormed = parser && parser->wellFormed();

    // We have to clear the parser, in case someone document.write()s from the
    // onLoad event handler, as in Radar 3206524.
    detachParser();

    // Parser should have picked up all preloads by now
    m_cachedResourceLoader->clearPreloads();

    // FIXME: We kick off the icon loader when the Document is done parsing.
    // There are earlier opportunities we could start it:
    //  -When the <head> finishes parsing
    //  -When any new HTMLLinkElement is inserted into the document
    // But those add a dynamic component to the favicon that has UI 
    // ramifications, and we need to decide what is the Right Thing To Do(tm)
    Frame* f = frame();
    if (f) {
        f->loader()->icon()->startLoader();
        f->animation()->resumeAnimationsForDocument(this);
    }

    ImageLoader::dispatchPendingBeforeLoadEvents();
    ImageLoader::dispatchPendingLoadEvents();
    ImageLoader::dispatchPendingErrorEvents();

    HTMLLinkElement::dispatchPendingLoadEvents();
    HTMLStyleElement::dispatchPendingLoadEvents();

#if ENABLE(SVG)
    // To align the HTML load event and the SVGLoad event for the outermost <svg> element, fire it from
    // here, instead of doing it from SVGElement::finishedParsingChildren (if externalResourcesRequired="false",
    // which is the default, for ='true' its fired at a later time, once all external resources finished loading).
    if (svgExtensions())
        accessSVGExtensions()->dispatchSVGLoadEventToOutermostSVGElements();
#endif

    dispatchWindowLoadEvent();
    enqueuePageshowEvent(PageshowEventNotPersisted);
    enqueuePopstateEvent(m_pendingStateObject ? m_pendingStateObject.release() : SerializedScriptValue::nullValue());
    
    if (f)
        f->loader()->handledOnloadEvents();
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("onload fired at %d\n", elapsedTime());
#endif

    // An event handler may have removed the frame
    if (!frame()) {
        m_processingLoadEvent = false;
        return;
    }

    // Make sure both the initial layout and reflow happen after the onload
    // fires. This will improve onload scores, and other browsers do it.
    // If they wanna cheat, we can too. -dwh

    if (frame()->navigationScheduler()->locationChangePending() && elapsedTime() < cLayoutScheduleThreshold) {
        // Just bail out. Before or during the onload we were shifted to another page.
        // The old i-Bench suite does this. When this happens don't bother painting or laying out.        
        m_processingLoadEvent = false;
        view()->unscheduleRelayout();
        return;
    }

    frame()->loader()->checkCallImplicitClose();
    RenderObject* renderObject = renderer();
    
    // We used to force a synchronous display and flush here.  This really isn't
    // necessary and can in fact be actively harmful if pages are loading at a rate of > 60fps
    // (if your platform is syncing flushes and limiting them to 60fps).
    m_overMinimumLayoutThreshold = true;
    if (!ownerElement() || (ownerElement()->renderer() && !ownerElement()->renderer()->needsLayout())) {
        updateStyleIfNeeded();
        
        // Always do a layout after loading if needed.
        if (view() && renderObject && (!renderObject->firstChild() || renderObject->needsLayout()))
            view()->layout();
    }

    m_processingLoadEvent = false;

#if PLATFORM(MAC) || PLATFORM(CHROMIUM)
    if (f && renderObject && AXObjectCache::accessibilityEnabled()) {
        // The AX cache may have been cleared at this point, but we need to make sure it contains an
        // AX object to send the notification to. getOrCreate will make sure that an valid AX object
        // exists in the cache (we ignore the return value because we don't need it here). This is 
        // only safe to call when a layout is not in progress, so it can not be used in postNotification.    
        axObjectCache()->getOrCreate(renderObject);
        if (this == topDocument())
            axObjectCache()->postNotification(renderObject, AXObjectCache::AXLoadComplete, true);
        else {
            // AXLoadComplete can only be posted on the top document, so if it's a document
            // in an iframe that just finished loading, post AXLayoutComplete instead.
            axObjectCache()->postNotification(renderObject, AXObjectCache::AXLayoutComplete, true);
        }
    }
#endif

#if ENABLE(SVG)
    if (svgExtensions())
        accessSVGExtensions()->startAnimations();
#endif
}

void Document::setParsing(bool b)
{
    m_bParsing = b;
    if (!m_bParsing && view())
        view()->scheduleRelayout();

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement() && !m_bParsing)
        printf("Parsing finished at %d\n", elapsedTime());
#endif
}

bool Document::shouldScheduleLayout()
{
    // This function will only be called when FrameView thinks a layout is needed.
    // This enforces a couple extra rules.
    //
    //    (a) Only schedule a layout once the stylesheets are loaded.
    //    (b) Only schedule layout once we have a body element.

    return (haveStylesheetsLoaded() && body())
        || (documentElement() && !documentElement()->hasTagName(htmlTag));
}
    
bool Document::isLayoutTimerActive()
{
    return view() && view()->layoutPending() && !minimumLayoutDelay();
}

int Document::minimumLayoutDelay()
{
    if (m_overMinimumLayoutThreshold)
        return 0;
    
    int elapsed = elapsedTime();
    m_overMinimumLayoutThreshold = elapsed > cLayoutScheduleThreshold;
    
    // We'll want to schedule the timer to fire at the minimum layout threshold.
    return max(0, cLayoutScheduleThreshold - elapsed);
}

int Document::elapsedTime() const
{
    return static_cast<int>((currentTime() - m_startTime) * 1000);
}

void Document::write(const SegmentedString& text, Document* ownerDocument)
{
    NestingLevelIncrementer nestingLevelIncrementer(m_writeRecursionDepth);

    m_writeRecursionIsTooDeep = (m_writeRecursionDepth > 1) && m_writeRecursionIsTooDeep;
    m_writeRecursionIsTooDeep = (m_writeRecursionDepth > cMaxWriteRecursionDepth) || m_writeRecursionIsTooDeep;

    if (m_writeRecursionIsTooDeep)
       return;

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Beginning a document.write at %d\n", elapsedTime());
#endif

    bool hasInsertionPoint = m_parser && m_parser->hasInsertionPoint();
    if (!hasInsertionPoint && m_ignoreDestructiveWriteCount)
        return;

    if (!hasInsertionPoint)
        open(ownerDocument);

    ASSERT(m_parser);
    m_parser->insert(text);

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Ending a document.write at %d\n", elapsedTime());
#endif    
}

void Document::write(const String& text, Document* ownerDocument)
{
    write(SegmentedString(text), ownerDocument);
}

void Document::writeln(const String& text, Document* ownerDocument)
{
    write(text, ownerDocument);
    write("\n", ownerDocument);
}

const KURL& Document::virtualURL() const
{
    return m_url;
}

KURL Document::virtualCompleteURL(const String& url) const
{
    return completeURL(url);
}

double Document::minimumTimerInterval() const
{
    Page* p = page();
    if (!p)
        return ScriptExecutionContext::minimumTimerInterval();
    return p->settings()->minDOMTimerInterval();
}

EventTarget* Document::errorEventTarget()
{
    return domWindow();
}

void Document::logExceptionToConsole(const String& errorMessage, const String& sourceURL, int lineNumber, PassRefPtr<ScriptCallStack> callStack)
{
    addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, errorMessage, sourceURL, lineNumber, callStack);
}

void Document::setURL(const KURL& url)
{
    const KURL& newURL = url.isEmpty() ? blankURL() : url;
    if (newURL == m_url)
        return;

    m_url = newURL;
    m_documentURI = m_url.string();
    updateBaseURL();
    contextFeatures()->urlDidChange(this);
}

void Document::updateBaseURL()
{
    KURL oldBaseURL = m_baseURL;
    // DOM 3 Core: When the Document supports the feature "HTML" [DOM Level 2 HTML], the base URI is computed using
    // first the value of the href attribute of the HTML BASE element if any, and the value of the documentURI attribute
    // from the Document interface otherwise.
    if (!m_baseElementURL.isEmpty())
        m_baseURL = m_baseElementURL;
    else if (!m_baseURLOverride.isEmpty())
        m_baseURL = m_baseURLOverride;
    else {
        // The documentURI attribute is read-only from JavaScript, but writable from Objective C, so we need to retain
        // this fallback behavior. We use a null base URL, since the documentURI attribute is an arbitrary string
        // and DOM 3 Core does not specify how it should be resolved.
        m_baseURL = KURL(KURL(), documentURI());
    }
    selectorQueryCache()->invalidate();

    if (!m_baseURL.isValid())
        m_baseURL = KURL();

    if (m_elemSheet) {
        // Element sheet is silly. It never contains anything.
        ASSERT(!m_elemSheet->contents()->ruleCount());
        bool usesRemUnits = m_elemSheet->contents()->usesRemUnits();
        m_elemSheet = CSSStyleSheet::createInline(this, m_baseURL);
        // FIXME: So we are not really the parser. The right fix is to eliminate the element sheet completely.
        m_elemSheet->contents()->parserSetUsesRemUnits(usesRemUnits);
    }

    if (!equalIgnoringFragmentIdentifier(oldBaseURL, m_baseURL)) {
        // Base URL change changes any relative visited links.
        // FIXME: There are other URLs in the tree that would need to be re-evaluated on dynamic base URL change. Style should be invalidated too.
        for (Node* node = firstChild(); node; node = node->traverseNextNode()) {
            if (node->hasTagName(aTag))
                static_cast<HTMLAnchorElement*>(node)->invalidateCachedVisitedLinkHash();
        }
    }
}

void Document::setBaseURLOverride(const KURL& url)
{
    m_baseURLOverride = url;
    updateBaseURL();
}

void Document::processBaseElement()
{
    // Find the first href attribute in a base element and the first target attribute in a base element.
    const AtomicString* href = 0;
    const AtomicString* target = 0;
    for (Node* node = document()->firstChild(); node && (!href || !target); node = node->traverseNextNode()) {
        if (node->hasTagName(baseTag)) {
            if (!href) {
                const AtomicString& value = static_cast<Element*>(node)->fastGetAttribute(hrefAttr);
                if (!value.isNull())
                    href = &value;
            }
            if (!target) {
                const AtomicString& value = static_cast<Element*>(node)->fastGetAttribute(targetAttr);
                if (!value.isNull())
                    target = &value;
            }
        }
    }

    // FIXME: Since this doesn't share code with completeURL it may not handle encodings correctly.
    KURL baseElementURL;
    if (href) {
        String strippedHref = stripLeadingAndTrailingHTMLSpaces(*href);
        if (!strippedHref.isEmpty())
            baseElementURL = KURL(url(), strippedHref);
    }
    if (m_baseElementURL != baseElementURL) {
        m_baseElementURL = baseElementURL;
        updateBaseURL();
    }

    m_baseTarget = target ? *target : nullAtom;
}

String Document::userAgent(const KURL& url) const
{
    return frame() ? frame()->loader()->userAgent(url) : String();
}

void Document::disableEval(const String& errorMessage)
{
    if (!frame())
        return;

    frame()->script()->disableEval(errorMessage);
}

bool Document::canNavigate(Frame* targetFrame)
{
    if (!m_frame)
        return false;

    // FIXME: We shouldn't call this function without a target frame, but
    // fast/forms/submit-to-blank-multiple-times.html depends on this function
    // returning true when supplied with a 0 targetFrame.
    if (!targetFrame)
        return true;

    // Frame-busting is generally allowed (unless we're sandboxed and prevent from frame-busting).
    if (!isSandboxed(SandboxTopNavigation) && targetFrame == m_frame->tree()->top())
        return true;

    if (isSandboxed(SandboxNavigation)) {
        if (targetFrame->tree()->isDescendantOf(m_frame))
            return true;

        printNavigationErrorMessage(targetFrame, url());
        return false;
    }

    // This is the normal case. A document can navigate its decendant frames,
    // or, more generally, a document can navigate a frame if the document is
    // in the same origin as any of that frame's ancestors (in the frame
    // hierarchy).
    //
    // See http://www.adambarth.com/papers/2008/barth-jackson-mitchell.pdf for
    // historical information about this security check.
    if (canAccessAncestor(securityOrigin(), targetFrame))
        return true;

    // Top-level frames are easier to navigate than other frames because they
    // display their URLs in the address bar (in most browsers). However, there
    // are still some restrictions on navigation to avoid nuisance attacks.
    // Specifically, a document can navigate a top-level frame if that frame
    // opened the document or if the document is the same-origin with any of
    // the top-level frame's opener's ancestors (in the frame hierarchy).
    //
    // In both of these cases, the document performing the navigation is in
    // some way related to the frame being navigate (e.g., by the "opener"
    // and/or "parent" relation). Requiring some sort of relation prevents a
    // document from navigating arbitrary, unrelated top-level frames.
    if (!targetFrame->tree()->parent()) {
        if (targetFrame == m_frame->loader()->opener())
            return true;

        if (canAccessAncestor(securityOrigin(), targetFrame->loader()->opener()))
            return true;
    }

    printNavigationErrorMessage(targetFrame, url());
    return false;
}

Frame* Document::findUnsafeParentScrollPropagationBoundary()
{
    Frame* currentFrame = m_frame;
    Frame* ancestorFrame = currentFrame->tree()->parent(); 

    while (ancestorFrame) {
        if (!ancestorFrame->document()->securityOrigin()->canAccess(securityOrigin()))
            return currentFrame;
        currentFrame = ancestorFrame;
        ancestorFrame = ancestorFrame->tree()->parent();
    }
    return 0;
}

CSSStyleSheet* Document::pageUserSheet()
{
    if (m_pageUserSheet)
        return m_pageUserSheet.get();
    
    Page* owningPage = page();
    if (!owningPage)
        return 0;
    
    String userSheetText = owningPage->userStyleSheet();
    if (userSheetText.isEmpty())
        return 0;
    
    // Parse the sheet and cache it.
    m_pageUserSheet = CSSStyleSheet::createInline(this, settings()->userStyleSheetLocation());
    m_pageUserSheet->contents()->setIsUserStyleSheet(true);
    m_pageUserSheet->contents()->parseString(userSheetText);
    return m_pageUserSheet.get();
}

void Document::clearPageUserSheet()
{
    if (m_pageUserSheet) {
        m_pageUserSheet = 0;
        styleResolverChanged(DeferRecalcStyle);
    }
}

void Document::updatePageUserSheet()
{
    clearPageUserSheet();
    if (pageUserSheet())
        styleResolverChanged(RecalcStyleImmediately);
}

const Vector<RefPtr<CSSStyleSheet> >* Document::pageGroupUserSheets() const
{
    if (m_pageGroupUserSheetCacheValid)
        return m_pageGroupUserSheets.get();
    
    m_pageGroupUserSheetCacheValid = true;
    
    Page* owningPage = page();
    if (!owningPage)
        return 0;
        
    const PageGroup& pageGroup = owningPage->group();
    const UserStyleSheetMap* sheetsMap = pageGroup.userStyleSheets();
    if (!sheetsMap)
        return 0;

    UserStyleSheetMap::const_iterator end = sheetsMap->end();
    for (UserStyleSheetMap::const_iterator it = sheetsMap->begin(); it != end; ++it) {
        const UserStyleSheetVector* sheets = it->second.get();
        for (unsigned i = 0; i < sheets->size(); ++i) {
            const UserStyleSheet* sheet = sheets->at(i).get();
            if (sheet->injectedFrames() == InjectInTopFrameOnly && ownerElement())
                continue;
            if (!UserContentURLPattern::matchesPatterns(url(), sheet->whitelist(), sheet->blacklist()))
                continue;
            RefPtr<CSSStyleSheet> groupSheet = CSSStyleSheet::createInline(const_cast<Document*>(this), sheet->url());
            if (!m_pageGroupUserSheets)
                m_pageGroupUserSheets = adoptPtr(new Vector<RefPtr<CSSStyleSheet> >);
            m_pageGroupUserSheets->append(groupSheet);
            groupSheet->contents()->setIsUserStyleSheet(sheet->level() == UserStyleUserLevel);
            groupSheet->contents()->parseString(sheet->source());
        }
    }

    return m_pageGroupUserSheets.get();
}

void Document::clearPageGroupUserSheets()
{
    m_pageGroupUserSheetCacheValid = false;
    if (m_pageGroupUserSheets && m_pageGroupUserSheets->size()) {
        m_pageGroupUserSheets->clear();
        styleResolverChanged(DeferRecalcStyle);
    }
}

void Document::updatePageGroupUserSheets()
{
    clearPageGroupUserSheets();
    if (pageGroupUserSheets() && pageGroupUserSheets()->size())
        styleResolverChanged(RecalcStyleImmediately);
}

void Document::addUserSheet(PassRefPtr<StyleSheetContents> userSheet)
{
    if (!m_userSheets)
        m_userSheets = adoptPtr(new Vector<RefPtr<CSSStyleSheet> >);
    m_userSheets->append(CSSStyleSheet::create(userSheet, this));
    styleResolverChanged(RecalcStyleImmediately);
}

void Document::seamlessParentUpdatedStylesheets()
{
    styleResolverChanged(RecalcStyleImmediately);
}

CSSStyleSheet* Document::elementSheet()
{
    if (!m_elemSheet)
        m_elemSheet = CSSStyleSheet::createInline(this, m_baseURL);
    return m_elemSheet.get();
}

int Document::nodeAbsIndex(Node *node)
{
    ASSERT(node->document() == this);

    int absIndex = 0;
    for (Node* n = node; n && n != this; n = n->traversePreviousNode())
        absIndex++;
    return absIndex;
}

Node* Document::nodeWithAbsIndex(int absIndex)
{
    Node* n = this;
    for (int i = 0; n && (i < absIndex); i++)
        n = n->traverseNextNode();
    return n;
}

void Document::processHttpEquiv(const String& equiv, const String& content)
{
    ASSERT(!equiv.isNull() && !content.isNull());

    Frame* frame = this->frame();

    if (equalIgnoringCase(equiv, "default-style")) {
        // The preferred style set has been overridden as per section 
        // 14.3.2 of the HTML4.0 specification.  We need to update the
        // sheet used variable and then update our style selector. 
        // For more info, see the test at:
        // http://www.hixie.ch/tests/evil/css/import/main/preferred.html
        // -dwh
        m_selectedStylesheetSet = content;
        m_preferredStylesheetSet = content;
        styleResolverChanged(DeferRecalcStyle);
    } else if (equalIgnoringCase(equiv, "refresh")) {
        double delay;
        String url;
        if (frame && parseHTTPRefresh(content, true, delay, url)) {
            if (url.isEmpty())
                url = m_url.string();
            else
                url = completeURL(url).string();
            frame->navigationScheduler()->scheduleRedirect(delay, url);
        }
    } else if (equalIgnoringCase(equiv, "set-cookie")) {
        // FIXME: make setCookie work on XML documents too; e.g. in case of <html:meta .....>
        if (isHTMLDocument()) {
            ExceptionCode ec; // Exception (for sandboxed documents) ignored.
            static_cast<HTMLDocument*>(this)->setCookie(content, ec);
        }
    } else if (equalIgnoringCase(equiv, "content-language"))
        setContentLanguage(content);
    else if (equalIgnoringCase(equiv, "x-dns-prefetch-control"))
        parseDNSPrefetchControlHeader(content);
    else if (equalIgnoringCase(equiv, "x-frame-options")) {
        if (frame) {
            FrameLoader* frameLoader = frame->loader();
            if (frameLoader->shouldInterruptLoadForXFrameOptions(content, url())) {
                frameLoader->stopAllLoaders();
                frame->navigationScheduler()->scheduleLocationChange(securityOrigin(), blankURL(), String());

                DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to display document because display forbidden by X-Frame-Options.\n")));
                addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, consoleMessage);
            }
        }
    } else if (equalIgnoringCase(equiv, "x-webkit-csp"))
        contentSecurityPolicy()->didReceiveHeader(content, ContentSecurityPolicy::EnforcePolicy);
    else if (equalIgnoringCase(equiv, "x-webkit-csp-report-only"))
        contentSecurityPolicy()->didReceiveHeader(content, ContentSecurityPolicy::ReportOnly);
}

// Though isspace() considers \t and \v to be whitespace, Win IE doesn't.
static bool isSeparator(UChar c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '=' || c == ',' || c == '\0';
}

void Document::processArguments(const String& features, void* data, ArgumentsCallback callback)
{
    // Tread lightly in this code -- it was specifically designed to mimic Win IE's parsing behavior.
    int keyBegin, keyEnd;
    int valueBegin, valueEnd;

    int i = 0;
    int length = features.length();
    String buffer = features.lower();
    while (i < length) {
        // skip to first non-separator, but don't skip past the end of the string
        while (isSeparator(buffer[i])) {
            if (i >= length)
                break;
            i++;
        }
        keyBegin = i;

        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        keyEnd = i;

        // skip to first '=', but don't skip past a ',' or the end of the string
        while (buffer[i] != '=') {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }

        // skip to first non-separator, but don't skip past a ',' or the end of the string
        while (isSeparator(buffer[i])) {
            if (buffer[i] == ',' || i >= length)
                break;
            i++;
        }
        valueBegin = i;

        // skip to first separator
        while (!isSeparator(buffer[i]))
            i++;
        valueEnd = i;

        ASSERT(i <= length);

        String keyString = buffer.substring(keyBegin, keyEnd - keyBegin);
        String valueString = buffer.substring(valueBegin, valueEnd - valueBegin);
        callback(keyString, valueString, this, data);
    }
}

void Document::processViewport(const String& features, ViewportArguments::Type origin)
{
    ASSERT(!features.isNull());

    if (origin < m_viewportArguments.type)
        return;

    m_viewportArguments = ViewportArguments(origin);
    processArguments(features, (void*)&m_viewportArguments, &setViewportFeature);

    updateViewportArguments();
}

void Document::updateViewportArguments()
{
    if (page() && page()->mainFrame() == frame()) {
#ifndef NDEBUG
        m_didDispatchViewportPropertiesChanged = true;
#endif
        page()->chrome()->dispatchViewportPropertiesDidChange(m_viewportArguments);
    }
}

void Document::processReferrerPolicy(const String& policy)
{
    ASSERT(!policy.isNull());

    m_referrerPolicy = ReferrerPolicyDefault;

    if (equalIgnoringCase(policy, "never"))
        m_referrerPolicy = ReferrerPolicyNever;
    else if (equalIgnoringCase(policy, "always"))
        m_referrerPolicy = ReferrerPolicyAlways;
    else if (equalIgnoringCase(policy, "origin"))
        m_referrerPolicy = ReferrerPolicyOrigin;
}

MouseEventWithHitTestResults Document::prepareMouseEvent(const HitTestRequest& request, const LayoutPoint& documentPoint, const PlatformMouseEvent& event)
{
    ASSERT(!renderer() || renderer()->isRenderView());

    if (!renderer())
        return MouseEventWithHitTestResults(event, HitTestResult(LayoutPoint()));

    HitTestResult result(documentPoint);
    renderView()->hitTest(request, result);

    if (!request.readOnly())
        updateStyleIfNeeded();

    return MouseEventWithHitTestResults(event, result);
}

// DOM Section 1.1.1
bool Document::childTypeAllowed(NodeType type) const
{
    switch (type) {
    case ATTRIBUTE_NODE:
    case CDATA_SECTION_NODE:
    case DOCUMENT_FRAGMENT_NODE:
    case DOCUMENT_NODE:
    case ENTITY_NODE:
    case ENTITY_REFERENCE_NODE:
    case NOTATION_NODE:
    case TEXT_NODE:
    case XPATH_NAMESPACE_NODE:
        return false;
    case COMMENT_NODE:
    case PROCESSING_INSTRUCTION_NODE:
        return true;
    case DOCUMENT_TYPE_NODE:
    case ELEMENT_NODE:
        // Documents may contain no more than one of each of these.
        // (One Element and one DocumentType.)
        for (Node* c = firstChild(); c; c = c->nextSibling())
            if (c->nodeType() == type)
                return false;
        return true;
    }
    return false;
}

bool Document::canReplaceChild(Node* newChild, Node* oldChild)
{
    if (!oldChild)
        // ContainerNode::replaceChild will raise a NOT_FOUND_ERR.
        return true;

    if (oldChild->nodeType() == newChild->nodeType())
        return true;

    int numDoctypes = 0;
    int numElements = 0;

    // First, check how many doctypes and elements we have, not counting
    // the child we're about to remove.
    for (Node* c = firstChild(); c; c = c->nextSibling()) {
        if (c == oldChild)
            continue;
        
        switch (c->nodeType()) {
        case DOCUMENT_TYPE_NODE:
            numDoctypes++;
            break;
        case ELEMENT_NODE:
            numElements++;
            break;
        default:
            break;
        }
    }
    
    // Then, see how many doctypes and elements might be added by the new child.
    if (newChild->nodeType() == DOCUMENT_FRAGMENT_NODE) {
        for (Node* c = newChild->firstChild(); c; c = c->nextSibling()) {
            switch (c->nodeType()) {
            case ATTRIBUTE_NODE:
            case CDATA_SECTION_NODE:
            case DOCUMENT_FRAGMENT_NODE:
            case DOCUMENT_NODE:
            case ENTITY_NODE:
            case ENTITY_REFERENCE_NODE:
            case NOTATION_NODE:
            case TEXT_NODE:
            case XPATH_NAMESPACE_NODE:
                return false;
            case COMMENT_NODE:
            case PROCESSING_INSTRUCTION_NODE:
                break;
            case DOCUMENT_TYPE_NODE:
                numDoctypes++;
                break;
            case ELEMENT_NODE:
                numElements++;
                break;
            }
        }
    } else {
        switch (newChild->nodeType()) {
        case ATTRIBUTE_NODE:
        case CDATA_SECTION_NODE:
        case DOCUMENT_FRAGMENT_NODE:
        case DOCUMENT_NODE:
        case ENTITY_NODE:
        case ENTITY_REFERENCE_NODE:
        case NOTATION_NODE:
        case TEXT_NODE:
        case XPATH_NAMESPACE_NODE:
            return false;
        case COMMENT_NODE:
        case PROCESSING_INSTRUCTION_NODE:
            return true;
        case DOCUMENT_TYPE_NODE:
            numDoctypes++;
            break;
        case ELEMENT_NODE:
            numElements++;
            break;
        }                
    }
        
    if (numElements > 1 || numDoctypes > 1)
        return false;
    
    return true;
}

PassRefPtr<Node> Document::cloneNode(bool /*deep*/)
{
    // Spec says cloning Document nodes is "implementation dependent"
    // so we do not support it...
    return 0;
}

StyleSheetList* Document::styleSheets()
{
    return m_styleSheets.get();
}

String Document::preferredStylesheetSet() const
{
    return m_preferredStylesheetSet;
}

String Document::selectedStylesheetSet() const
{
    return m_selectedStylesheetSet;
}

void Document::setSelectedStylesheetSet(const String& aString)
{
    m_selectedStylesheetSet = aString;
    styleResolverChanged(DeferRecalcStyle);
}

// This method is called whenever a top-level stylesheet has finished loading.
void Document::removePendingSheet(RemovePendingSheetNotificationType notification)
{
    // Make sure we knew this sheet was pending, and that our count isn't out of sync.
    ASSERT(m_pendingStylesheets > 0);

    m_pendingStylesheets--;
    
#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Stylesheet loaded at time %d. %d stylesheets still remain.\n", elapsedTime(), m_pendingStylesheets);
#endif

    if (m_pendingStylesheets)
        return;

    if (notification == RemovePendingSheetNotifyLater) {
        setNeedsNotifyRemoveAllPendingStylesheet();
        return;
    }
    
    didRemoveAllPendingStylesheet();
}

void Document::didRemoveAllPendingStylesheet()
{
    m_needsNotifyRemoveAllPendingStylesheet = false;

    styleResolverChanged(RecalcStyleIfNeeded);

    if (ScriptableDocumentParser* parser = scriptableDocumentParser())
        parser->executeScriptsWaitingForStylesheets();

    if (m_gotoAnchorNeededAfterStylesheetsLoad && view())
        view()->scrollToFragment(m_url);
}


void Document::evaluateMediaQueryList()
{
    if (m_mediaQueryMatcher)
        m_mediaQueryMatcher->styleResolverChanged();
}

void Document::styleResolverChanged(StyleResolverUpdateFlag updateFlag)
{
    // Don't bother updating, since we haven't loaded all our style info yet
    // and haven't calculated the style selector for the first time.
    if (!attached() || (!m_didCalculateStyleResolver && !haveStylesheetsLoaded())) {
        m_styleResolver.clear();
        return;
    }

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Beginning update of style selector at time %d.\n", elapsedTime());
#endif

    bool stylesheetChangeRequiresStyleRecalc = updateActiveStylesheets(updateFlag);

    if (updateFlag == DeferRecalcStyle) {
        scheduleForcedStyleRecalc();
        return;
    }

    if (didLayoutWithPendingStylesheets() && m_pendingStylesheets <= 0) {
        m_pendingSheetLayout = IgnoreLayoutWithPendingSheets;
        if (renderer())
            renderer()->repaint();
    }

    if (!stylesheetChangeRequiresStyleRecalc)
        return;

    // This recalcStyle initiates a new recalc cycle. We need to bracket it to
    // make sure animations get the correct update time
    {
        AnimationUpdateBlock animationUpdateBlock(m_frame ? m_frame->animation() : 0);
        recalcStyle(Force);
    }

#ifdef INSTRUMENT_LAYOUT_SCHEDULING
    if (!ownerElement())
        printf("Finished update of style selector at time %d\n", elapsedTime());
#endif

    if (renderer()) {
        renderer()->setNeedsLayoutAndPrefWidthsRecalc();
        if (view())
            view()->scheduleRelayout();
    }

    evaluateMediaQueryList();
}

void Document::addStyleSheetCandidateNode(Node* node, bool createdByParser)
{
    if (!node->inDocument())
        return;
    
    // Until the <body> exists, we have no choice but to compare document positions,
    // since styles outside of the body and head continue to be shunted into the head
    // (and thus can shift to end up before dynamically added DOM content that is also
    // outside the body).
    if ((createdByParser && body()) || m_styleSheetCandidateNodes.isEmpty()) {
        m_styleSheetCandidateNodes.add(node);
        return;
    }

    // Determine an appropriate insertion point.
    StyleSheetCandidateListHashSet::iterator begin = m_styleSheetCandidateNodes.begin();
    StyleSheetCandidateListHashSet::iterator end = m_styleSheetCandidateNodes.end();
    StyleSheetCandidateListHashSet::iterator it = end;
    Node* followingNode = 0;
    do {
        --it;
        Node* n = *it;
        unsigned short position = n->compareDocumentPosition(node);
        if (position == DOCUMENT_POSITION_FOLLOWING) {
            m_styleSheetCandidateNodes.insertBefore(followingNode, node);
            return;
        }
        followingNode = n;
    } while (it != begin);
    
    m_styleSheetCandidateNodes.insertBefore(followingNode, node);
}

void Document::removeStyleSheetCandidateNode(Node* node)
{
    m_styleSheetCandidateNodes.remove(node);
}

void Document::collectActiveStylesheets(Vector<RefPtr<StyleSheet> >& sheets)
{
    if (settings() && !settings()->authorAndUserStylesEnabled())
        return;

    StyleSheetCandidateListHashSet::iterator begin = m_styleSheetCandidateNodes.begin();
    StyleSheetCandidateListHashSet::iterator end = m_styleSheetCandidateNodes.end();
    for (StyleSheetCandidateListHashSet::iterator it = begin; it != end; ++it) {
        Node* n = *it;
        StyleSheet* sheet = 0;
        if (n->nodeType() == PROCESSING_INSTRUCTION_NODE) {
            // Processing instruction (XML documents only).
            // We don't support linking to embedded CSS stylesheets, see <https://bugs.webkit.org/show_bug.cgi?id=49281> for discussion.
            ProcessingInstruction* pi = static_cast<ProcessingInstruction*>(n);
            sheet = pi->sheet();
#if ENABLE(XSLT)
            // Don't apply XSL transforms to already transformed documents -- <rdar://problem/4132806>
            if (pi->isXSL() && !transformSourceDocument()) {
                // Don't apply XSL transforms until loading is finished.
                if (!parsing())
                    applyXSLTransform(pi);
                return;
            }
#endif
        } else if ((n->isHTMLElement() && (n->hasTagName(linkTag) || n->hasTagName(styleTag)))
#if ENABLE(SVG)
                   ||  (n->isSVGElement() && n->hasTagName(SVGNames::styleTag))
#endif
                   ) {
            Element* e = static_cast<Element*>(n);
            AtomicString title = e->getAttribute(titleAttr);
            bool enabledViaScript = false;
            if (e->hasLocalName(linkTag)) {
                // <LINK> element
                HTMLLinkElement* linkElement = static_cast<HTMLLinkElement*>(n);
                if (linkElement->isDisabled())
                    continue;
                enabledViaScript = linkElement->isEnabledViaScript();
                if (linkElement->styleSheetIsLoading()) {
                    // it is loading but we should still decide which style sheet set to use
                    if (!enabledViaScript && !title.isEmpty() && m_preferredStylesheetSet.isEmpty()) {
                        const AtomicString& rel = e->getAttribute(relAttr);
                        if (!rel.contains("alternate")) {
                            m_preferredStylesheetSet = title;
                            m_selectedStylesheetSet = title;
                        }
                    }
                    continue;
                }
                if (!linkElement->sheet())
                    title = nullAtom;
            }
            // Get the current preferred styleset.  This is the
            // set of sheets that will be enabled.
#if ENABLE(SVG)
            if (n->isSVGElement() && n->hasTagName(SVGNames::styleTag))
                sheet = static_cast<SVGStyleElement*>(n)->sheet();
            else
#endif
            if (e->hasLocalName(linkTag))
                sheet = static_cast<HTMLLinkElement*>(n)->sheet();
            else
                // <STYLE> element
                sheet = static_cast<HTMLStyleElement*>(n)->sheet();
            // Check to see if this sheet belongs to a styleset
            // (thus making it PREFERRED or ALTERNATE rather than
            // PERSISTENT).
            AtomicString rel = e->getAttribute(relAttr);
            if (!enabledViaScript && !title.isEmpty()) {
                // Yes, we have a title.
                if (m_preferredStylesheetSet.isEmpty()) {
                    // No preferred set has been established.  If
                    // we are NOT an alternate sheet, then establish
                    // us as the preferred set.  Otherwise, just ignore
                    // this sheet.
                    if (e->hasLocalName(styleTag) || !rel.contains("alternate"))
                        m_preferredStylesheetSet = m_selectedStylesheetSet = title;
                }
                if (title != m_preferredStylesheetSet)
                    sheet = 0;
            }

            if (rel.contains("alternate") && title.isEmpty())
                sheet = 0;
        }
        if (sheet)
            sheets.append(sheet);
    }
}

bool Document::testAddedStylesheetRequiresStyleRecalc(StyleSheetContents* stylesheet)
{
    // See if all rules on the sheet are scoped to some specific ids or classes.
    // Then test if we actually have any of those in the tree at the moment.
    HashSet<AtomicStringImpl*> idScopes; 
    HashSet<AtomicStringImpl*> classScopes;
    if (!StyleResolver::determineStylesheetSelectorScopes(stylesheet, idScopes, classScopes))
        return true;
    // Invalidate the subtrees that match the scopes.
    Node* node = firstChild();
    while (node) {
        if (!node->isStyledElement()) {
            node = node->traverseNextNode();
            continue;
        }
        StyledElement* element = static_cast<StyledElement*>(node);
        if (SelectorChecker::elementMatchesSelectorScopes(element, idScopes, classScopes)) {
            element->setNeedsStyleRecalc();
            // The whole subtree is now invalidated, we can skip to the next sibling.
            node = node->traverseNextSibling();
            continue;
        }
        node = node->traverseNextNode();
    }
    return false;
}

void Document::analyzeStylesheetChange(StyleResolverUpdateFlag updateFlag, const Vector<RefPtr<StyleSheet> >& newStylesheets, bool& requiresStyleResolverReset, bool& requiresFullStyleRecalc)
{
    requiresStyleResolverReset = true;
    requiresFullStyleRecalc = true;
    
    // Stylesheets of <style> elements that @import stylesheets are active but loading. We need to trigger a full recalc when such loads are done.
    bool hasActiveLoadingStylesheet = false;
    unsigned newStylesheetCount = newStylesheets.size();
    for (unsigned i = 0; i < newStylesheetCount; ++i) {
        if (newStylesheets[i]->isLoading())
            hasActiveLoadingStylesheet = true;
    }
    if (m_hadActiveLoadingStylesheet && !hasActiveLoadingStylesheet) {
        m_hadActiveLoadingStylesheet = false;
        return;
    }
    m_hadActiveLoadingStylesheet = hasActiveLoadingStylesheet;

    if (updateFlag != RecalcStyleIfNeeded)
        return;
    if (!m_styleResolver)
        return;

    // See if we are just adding stylesheets.
    unsigned oldStylesheetCount = m_styleSheets->length();
    if (newStylesheetCount < oldStylesheetCount)
        return;
    for (unsigned i = 0; i < oldStylesheetCount; ++i) {
        if (m_styleSheets->item(i) != newStylesheets[i])
            return;
    }
    requiresStyleResolverReset = false;

    // If we are already parsing the body and so may have significant amount of elements, put some effort into trying to avoid style recalcs.
    if (!body() || m_hasNodesWithPlaceholderStyle)
        return;
    for (unsigned i = oldStylesheetCount; i < newStylesheetCount; ++i) {
        if (!newStylesheets[i]->isCSSStyleSheet())
            return;
        if (newStylesheets[i]->disabled())
            continue;
        if (testAddedStylesheetRequiresStyleRecalc(static_cast<CSSStyleSheet*>(newStylesheets[i].get())->contents()))
            return;
    }
    requiresFullStyleRecalc = false;
}

static bool styleSheetsUseRemUnits(const Vector<RefPtr<StyleSheet> >& sheets)
{
    for (unsigned i = 0; i < sheets.size(); ++i) {
        if (!sheets[i]->isCSSStyleSheet())
            continue;
        if (static_cast<CSSStyleSheet*>(sheets[i].get())->contents()->usesRemUnits())
            return true;
    }
    return false;
}

void Document::notifySeamlessChildDocumentsOfStylesheetUpdate() const
{
    // If we're not in a frame yet any potential child documents won't have a StyleResolver to update.
    if (!frame())
        return;

    // Seamless child frames are expected to notify their seamless children recursively, so we only do direct children.
    for (Frame* child = frame()->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        Document* childDocument = child->document();
        if (childDocument->shouldDisplaySeamlesslyWithParent()) {
            ASSERT(childDocument->seamlessParentIFrame()->document() == this);
            childDocument->seamlessParentUpdatedStylesheets();
        }
    }
}

bool Document::updateActiveStylesheets(StyleResolverUpdateFlag updateFlag)
{
    if (m_inStyleRecalc) {
        // SVG <use> element may manage to invalidate style selector in the middle of a style recalc.
        // https://bugs.webkit.org/show_bug.cgi?id=54344
        // FIXME: This should be fixed in SVG and this code replaced with ASSERT(!m_inStyleRecalc).
        m_hasDirtyStyleResolver = true;
        scheduleForcedStyleRecalc();
        return false;
    }
    if (!renderer() || !attached())
        return false;

    StyleSheetVector newStylesheets;
    collectActiveStylesheets(newStylesheets);

    bool requiresStyleResolverReset;
    bool requiresFullStyleRecalc;
    analyzeStylesheetChange(updateFlag, newStylesheets, requiresStyleResolverReset, requiresFullStyleRecalc);

    if (requiresStyleResolverReset)
        clearStyleResolver();
    else {
        m_styleResolver->appendAuthorStylesheets(m_styleSheets->length(), newStylesheets);
        resetCSSFeatureFlags();
    }
    m_styleSheets->swap(newStylesheets);

    m_usesRemUnits = styleSheetsUseRemUnits(m_styleSheets->vector());
    m_didCalculateStyleResolver = true;
    m_hasDirtyStyleResolver = false;

    notifySeamlessChildDocumentsOfStylesheetUpdate();

    return requiresFullStyleRecalc;
}

void Document::setHoverNode(PassRefPtr<Node> newHoverNode)
{
    m_hoverNode = newHoverNode;
}

void Document::setActiveNode(PassRefPtr<Node> newActiveNode)
{
    m_activeNode = newActiveNode;
}

void Document::focusedNodeRemoved()
{
    setFocusedNode(0);
}

void Document::removeFocusedNodeOfSubtree(Node* node, bool amongChildrenOnly)
{
    if (!m_focusedNode || this->inPageCache()) // If the document is in the page cache, then we don't need to clear out the focused node.
        return;
        
    bool nodeInSubtree = false;
    if (amongChildrenOnly)
        nodeInSubtree = m_focusedNode->isDescendantOf(node);
    else
        nodeInSubtree = (m_focusedNode == node) || m_focusedNode->isDescendantOf(node);
    
    if (nodeInSubtree)
        document()->focusedNodeRemoved();
}

void Document::hoveredNodeDetached(Node* node)
{
    if (!m_hoverNode || (node != m_hoverNode && (!m_hoverNode->isTextNode() || node != m_hoverNode->parentNode())))
        return;

    m_hoverNode = node->parentNode();
    while (m_hoverNode && !m_hoverNode->renderer())
        m_hoverNode = m_hoverNode->parentNode();
    if (frame())
        frame()->eventHandler()->scheduleHoverStateUpdate();
}

void Document::activeChainNodeDetached(Node* node)
{
    if (!m_activeNode || (node != m_activeNode && (!m_activeNode->isTextNode() || node != m_activeNode->parentNode())))
        return;

    m_activeNode = node->parentNode();
    while (m_activeNode && !m_activeNode->renderer())
        m_activeNode = m_activeNode->parentNode();
}

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(WIDGET_REGION)
const Vector<DashboardRegionValue>& Document::dashboardRegions() const
{
    return m_dashboardRegions;
}

void Document::setDashboardRegions(const Vector<DashboardRegionValue>& regions)
{
    m_dashboardRegions = regions;
    setDashboardRegionsDirty(false);
}
#endif

bool Document::setFocusedNode(PassRefPtr<Node> prpNewFocusedNode)
{
    RefPtr<Node> newFocusedNode = prpNewFocusedNode;

    // Make sure newFocusedNode is actually in this document
    if (newFocusedNode && (newFocusedNode->document() != this))
        return true;

    if (m_focusedNode == newFocusedNode)
        return true;

    if (m_inPageCache)
        return false;

    bool focusChangeBlocked = false;
    RefPtr<Node> oldFocusedNode = m_focusedNode;
    m_focusedNode = 0;

    // Remove focus from the existing focus node (if any)
    if (oldFocusedNode) {
        ASSERT(!oldFocusedNode->inDetach());

        if (oldFocusedNode->active())
            oldFocusedNode->setActive(false);

        oldFocusedNode->setFocus(false);

        // Dispatch a change event for text fields or textareas that have been edited
        if (oldFocusedNode->isElementNode()) {
            Element* element = static_cast<Element*>(oldFocusedNode.get());
            if (element->wasChangedSinceLastFormControlChangeEvent())
                element->dispatchFormControlChangeEvent();
        }

        // Dispatch the blur event and let the node do any other blur related activities (important for text fields)
        oldFocusedNode->dispatchBlurEvent(newFocusedNode);

        if (m_focusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            newFocusedNode = 0;
        }
        
        oldFocusedNode->dispatchFocusOutEvent(eventNames().focusoutEvent, newFocusedNode); // DOM level 3 name for the bubbling blur event.
        // FIXME: We should remove firing DOMFocusOutEvent event when we are sure no content depends
        // on it, probably when <rdar://problem/8503958> is resolved.
        oldFocusedNode->dispatchFocusOutEvent(eventNames().DOMFocusOutEvent, newFocusedNode); // DOM level 2 name for compatibility.

        if (m_focusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            newFocusedNode = 0;
        }
        if (oldFocusedNode == this && oldFocusedNode->hasOneRef())
            return true;
            
        if (oldFocusedNode->isRootEditableElement())
            frame()->editor()->didEndEditing();

        if (view()) {
            Widget* oldWidget = widgetForNode(oldFocusedNode.get());
            if (oldWidget)
                oldWidget->setFocus(false);
            else
                view()->setFocus(false);
        }
    }

    if (newFocusedNode) {
        if (newFocusedNode->isRootEditableElement() && !acceptsEditingFocus(newFocusedNode.get())) {
            // delegate blocks focus change
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }
        // Set focus on the new node
        m_focusedNode = newFocusedNode;

        // Dispatch the focus event and let the node do any other focus related activities (important for text fields)
        m_focusedNode->dispatchFocusEvent(oldFocusedNode);

        if (m_focusedNode != newFocusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }

        m_focusedNode->dispatchFocusInEvent(eventNames().focusinEvent, oldFocusedNode); // DOM level 3 bubbling focus event.

        if (m_focusedNode != newFocusedNode) {
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }

        // FIXME: We should remove firing DOMFocusInEvent event when we are sure no content depends
        // on it, probably when <rdar://problem/8503958> is m.
        m_focusedNode->dispatchFocusInEvent(eventNames().DOMFocusInEvent, oldFocusedNode); // DOM level 2 for compatibility.

        if (m_focusedNode != newFocusedNode) { 
            // handler shifted focus
            focusChangeBlocked = true;
            goto SetFocusedNodeDone;
        }
        m_focusedNode->setFocus(true);

        if (m_focusedNode->isRootEditableElement())
            frame()->editor()->didBeginEditing();

        // eww, I suck. set the qt focus correctly
        // ### find a better place in the code for this
        if (view()) {
            Widget* focusWidget = widgetForNode(m_focusedNode.get());
            if (focusWidget) {
                // Make sure a widget has the right size before giving it focus.
                // Otherwise, we are testing edge cases of the Widget code.
                // Specifically, in WebCore this does not work well for text fields.
                updateLayout();
                // Re-get the widget in case updating the layout changed things.
                focusWidget = widgetForNode(m_focusedNode.get());
            }
            if (focusWidget)
                focusWidget->setFocus(true);
            else
                view()->setFocus(true);
        }
    }

#if PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(GTK) || PLATFORM(CHROMIUM)
    if (!focusChangeBlocked && m_focusedNode && AXObjectCache::accessibilityEnabled())
        axObjectCache()->handleFocusedUIElementChanged(oldFocusedNode.get(), newFocusedNode.get());
#endif
    if (!focusChangeBlocked)
        page()->chrome()->focusedNodeChanged(m_focusedNode.get());

SetFocusedNodeDone:
    updateStyleIfNeeded();
    return !focusChangeBlocked;
}
    
void Document::getFocusableNodes(Vector<RefPtr<Node> >& nodes)
{
    updateLayout();

    for (Node* node = firstChild(); node; node = node->traverseNextNode()) {
        if (node->isFocusable())
            nodes.append(node);
    }
}
  
void Document::setCSSTarget(Element* n)
{
    if (m_cssTarget)
        m_cssTarget->setNeedsStyleRecalc();
    m_cssTarget = n;
    if (n)
        n->setNeedsStyleRecalc();
}

void Document::registerNodeListCache(DynamicNodeListCacheBase* list)
{
    if (list->type() != NodeListCollectionType)
        m_nodeListCounts[InvalidateOnIdNameAttrChange]++;
    m_nodeListCounts[list->invalidationType()]++;
    if (list->isRootedAtDocument())
        m_listsInvalidatedAtDocument.add(list);
}

void Document::unregisterNodeListCache(DynamicNodeListCacheBase* list)
{
    if (list->type() != NodeListCollectionType)
        m_nodeListCounts[InvalidateOnIdNameAttrChange]--;
    m_nodeListCounts[list->invalidationType()]--;
    if (list->isRootedAtDocument()) {
        ASSERT(m_listsInvalidatedAtDocument.contains(list));
        m_listsInvalidatedAtDocument.remove(list);
    }
}

void Document::attachNodeIterator(NodeIterator* ni)
{
    m_nodeIterators.add(ni);
}

void Document::detachNodeIterator(NodeIterator* ni)
{
    // The node iterator can be detached without having been attached if its root node didn't have a document
    // when the iterator was created, but has it now.
    m_nodeIterators.remove(ni);
}

void Document::moveNodeIteratorsToNewDocument(Node* node, Document* newDocument)
{
    HashSet<NodeIterator*> nodeIteratorsList = m_nodeIterators;
    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = nodeIteratorsList.end();
    for (HashSet<NodeIterator*>::const_iterator it = nodeIteratorsList.begin(); it != nodeIteratorsEnd; ++it) {
        if ((*it)->root() == node) {
            detachNodeIterator(*it);
            newDocument->attachNodeIterator(*it);
        }
    }
}

void Document::updateRangesAfterChildrenChanged(ContainerNode* container)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenChanged(container);
    }
}

void Document::nodeChildrenWillBeRemoved(ContainerNode* container)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->nodeChildrenWillBeRemoved(container);
    }

    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = m_nodeIterators.end();
    for (HashSet<NodeIterator*>::const_iterator it = m_nodeIterators.begin(); it != nodeIteratorsEnd; ++it) {
        for (Node* n = container->firstChild(); n; n = n->nextSibling())
            (*it)->nodeWillBeRemoved(n);
    }

    if (Frame* frame = this->frame()) {
        for (Node* n = container->firstChild(); n; n = n->nextSibling()) {
            frame->eventHandler()->nodeWillBeRemoved(n);
            frame->selection()->nodeWillBeRemoved(n);
            frame->page()->dragCaretController()->nodeWillBeRemoved(n);
        }
    }
}

void Document::nodeWillBeRemoved(Node* n)
{
    HashSet<NodeIterator*>::const_iterator nodeIteratorsEnd = m_nodeIterators.end();
    for (HashSet<NodeIterator*>::const_iterator it = m_nodeIterators.begin(); it != nodeIteratorsEnd; ++it)
        (*it)->nodeWillBeRemoved(n);

    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator rangesEnd = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != rangesEnd; ++it)
            (*it)->nodeWillBeRemoved(n);
    }

    if (Frame* frame = this->frame()) {
        frame->eventHandler()->nodeWillBeRemoved(n);
        frame->selection()->nodeWillBeRemoved(n);
        frame->page()->dragCaretController()->nodeWillBeRemoved(n);
    }
}

void Document::textInserted(Node* text, unsigned offset, unsigned length)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textInserted(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->shiftMarkers(text, offset, length);
}

void Document::textRemoved(Node* text, unsigned offset, unsigned length)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textRemoved(text, offset, length);
    }

    // Update the markers for spelling and grammar checking.
    m_markers->removeMarkers(text, offset, length);
    m_markers->shiftMarkers(text, offset + length, 0 - length);
}

void Document::textNodesMerged(Text* oldNode, unsigned offset)
{
    if (!m_ranges.isEmpty()) {
        NodeWithIndex oldNodeWithIndex(oldNode);
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textNodesMerged(oldNodeWithIndex, offset);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

void Document::textNodeSplit(Text* oldNode)
{
    if (!m_ranges.isEmpty()) {
        HashSet<Range*>::const_iterator end = m_ranges.end();
        for (HashSet<Range*>::const_iterator it = m_ranges.begin(); it != end; ++it)
            (*it)->textNodeSplit(oldNode);
    }

    // FIXME: This should update markers for spelling and grammar checking.
}

void Document::createDOMWindow()
{
    ASSERT(m_frame);
    ASSERT(!m_domWindow);

    m_domWindow = DOMWindow::create(this);

    ASSERT(m_domWindow->document() == this);
    ASSERT(m_domWindow->frame() == m_frame);
}

void Document::takeDOMWindowFrom(Document* document)
{
    ASSERT(m_frame);
    ASSERT(!m_domWindow);
    ASSERT(document->domWindow());

    m_domWindow = document->m_domWindow.release();
    m_domWindow->didSecureTransitionTo(this);

    ASSERT(m_domWindow->document() == this);
    ASSERT(m_domWindow->frame() == m_frame);
}

void Document::setWindowAttributeEventListener(const AtomicString& eventType, PassRefPtr<EventListener> listener)
{
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->setAttributeEventListener(eventType, listener);
}

EventListener* Document::getWindowAttributeEventListener(const AtomicString& eventType)
{
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return 0;
    return domWindow->getAttributeEventListener(eventType);
}

void Document::dispatchWindowEvent(PassRefPtr<Event> event,  PassRefPtr<EventTarget> target)
{
    ASSERT(!eventDispatchForbidden());
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->dispatchEvent(event, target);
}

void Document::dispatchWindowLoadEvent()
{
    ASSERT(!eventDispatchForbidden());
    DOMWindow* domWindow = this->domWindow();
    if (!domWindow)
        return;
    domWindow->dispatchLoadEvent();
    m_loadEventFinished = true;
}

void Document::enqueueWindowEvent(PassRefPtr<Event> event)
{
    event->setTarget(domWindow());
    m_eventQueue->enqueueEvent(event);
}

void Document::enqueueDocumentEvent(PassRefPtr<Event> event)
{
    event->setTarget(this);
    m_eventQueue->enqueueEvent(event);
}

PassRefPtr<Event> Document::createEvent(const String& eventType, ExceptionCode& ec)
{
    RefPtr<Event> event = EventFactory::create(eventType);
    if (event)
        return event.release();

    ec = NOT_SUPPORTED_ERR;
    return 0;
}

void Document::addMutationEventListenerTypeIfEnabled(ListenerType listenerType)
{
    if (ContextFeatures::mutationEventsEnabled(this))
        addListenerType(listenerType);
}

void Document::addListenerTypeIfNeeded(const AtomicString& eventType)
{
    if (eventType == eventNames().DOMSubtreeModifiedEvent)
        addMutationEventListenerTypeIfEnabled(DOMSUBTREEMODIFIED_LISTENER);
    else if (eventType == eventNames().DOMNodeInsertedEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEINSERTED_LISTENER);
    else if (eventType == eventNames().DOMNodeRemovedEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEREMOVED_LISTENER);
    else if (eventType == eventNames().DOMNodeRemovedFromDocumentEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEREMOVEDFROMDOCUMENT_LISTENER);
    else if (eventType == eventNames().DOMNodeInsertedIntoDocumentEvent)
        addMutationEventListenerTypeIfEnabled(DOMNODEINSERTEDINTODOCUMENT_LISTENER);
    else if (eventType == eventNames().DOMAttrModifiedEvent)
        addMutationEventListenerTypeIfEnabled(DOMATTRMODIFIED_LISTENER);
    else if (eventType == eventNames().DOMCharacterDataModifiedEvent)
        addMutationEventListenerTypeIfEnabled(DOMCHARACTERDATAMODIFIED_LISTENER);
    else if (eventType == eventNames().overflowchangedEvent)
        addListenerType(OVERFLOWCHANGED_LISTENER);
    else if (eventType == eventNames().webkitAnimationStartEvent)
        addListenerType(ANIMATIONSTART_LISTENER);
    else if (eventType == eventNames().webkitAnimationEndEvent)
        addListenerType(ANIMATIONEND_LISTENER);
    else if (eventType == eventNames().webkitAnimationIterationEvent)
        addListenerType(ANIMATIONITERATION_LISTENER);
    else if (eventType == eventNames().webkitTransitionEndEvent)
        addListenerType(TRANSITIONEND_LISTENER);
    else if (eventType == eventNames().beforeloadEvent)
        addListenerType(BEFORELOAD_LISTENER);
    else if (eventType == eventNames().scrollEvent)
        addListenerType(SCROLL_LISTENER);
}

CSSStyleDeclaration* Document::getOverrideStyle(Element*, const String&)
{
    return 0;
}

HTMLFrameOwnerElement* Document::ownerElement() const
{
    if (!frame())
        return 0;
    return frame()->ownerElement();
}

String Document::cookie(ExceptionCode& ec) const
{
    if (page() && !page()->settings()->cookieEnabled())
        return String();

    // FIXME: The HTML5 DOM spec states that this attribute can raise an
    // INVALID_STATE_ERR exception on getting if the Document has no
    // browsing context.

    if (!securityOrigin()->canAccessCookies()) {
        ec = SECURITY_ERR;
        return String();
    }

    KURL cookieURL = this->cookieURL();
    if (cookieURL.isEmpty())
        return String();

    return cookies(this, cookieURL);
}

void Document::setCookie(const String& value, ExceptionCode& ec)
{
    if (page() && !page()->settings()->cookieEnabled())
        return;

    // FIXME: The HTML5 DOM spec states that this attribute can raise an
    // INVALID_STATE_ERR exception on setting if the Document has no
    // browsing context.

    if (!securityOrigin()->canAccessCookies()) {
        ec = SECURITY_ERR;
        return;
    }

    KURL cookieURL = this->cookieURL();
    if (cookieURL.isEmpty())
        return;

    setCookies(this, cookieURL, value);
}

String Document::referrer() const
{
    if (frame())
        return frame()->loader()->referrer();
    return String();
}

String Document::domain() const
{
    return securityOrigin()->domain();
}

void Document::setDomain(const String& newDomain, ExceptionCode& ec)
{
    if (SchemeRegistry::isDomainRelaxationForbiddenForURLScheme(securityOrigin()->protocol())) {
        ec = SECURITY_ERR;
        return;
    }

    // Both NS and IE specify that changing the domain is only allowed when
    // the new domain is a suffix of the old domain.

    // FIXME: We should add logging indicating why a domain was not allowed.

    // If the new domain is the same as the old domain, still call
    // securityOrigin()->setDomainForDOM. This will change the
    // security check behavior. For example, if a page loaded on port 8000
    // assigns its current domain using document.domain, the page will
    // allow other pages loaded on different ports in the same domain that
    // have also assigned to access this page.
    if (equalIgnoringCase(domain(), newDomain)) {
        securityOrigin()->setDomainFromDOM(newDomain);
        if (m_frame)
            m_frame->script()->updateSecurityOrigin();
        return;
    }

    int oldLength = domain().length();
    int newLength = newDomain.length();
    // e.g. newDomain = webkit.org (10) and domain() = www.webkit.org (14)
    if (newLength >= oldLength) {
        ec = SECURITY_ERR;
        return;
    }

    String test = domain();
    // Check that it's a subdomain, not e.g. "ebkit.org"
    if (test[oldLength - newLength - 1] != '.') {
        ec = SECURITY_ERR;
        return;
    }

    // Now test is "webkit.org" from domain()
    // and we check that it's the same thing as newDomain
    test.remove(0, oldLength - newLength);
    if (test != newDomain) {
        ec = SECURITY_ERR;
        return;
    }

    securityOrigin()->setDomainFromDOM(newDomain);
    if (m_frame)
        m_frame->script()->updateSecurityOrigin();
}

// http://www.whatwg.org/specs/web-apps/current-work/#dom-document-lastmodified
String Document::lastModified() const
{
    DateComponents date;
    bool foundDate = false;
    if (m_frame) {
        String httpLastModified;
        if (DocumentLoader* documentLoader = loader()) 
            httpLastModified = documentLoader->response().httpHeaderField("Last-Modified");
        if (!httpLastModified.isEmpty()) {
            date.setMillisecondsSinceEpochForDateTime(parseDate(httpLastModified));
            foundDate = true;
        }
    }
    // FIXME: If this document came from the file system, the HTML5
    // specificiation tells us to read the last modification date from the file
    // system.
    if (!foundDate)
        date.setMillisecondsSinceEpochForDateTime(currentTimeMS());
    return String::format("%02d/%02d/%04d %02d:%02d:%02d", date.month() + 1, date.monthDay(), date.fullYear(), date.hour(), date.minute(), date.second());
}

static bool isValidNameNonASCII(const UChar* characters, unsigned length)
{
    unsigned i = 0;

    UChar32 c;
    U16_NEXT(characters, i, length, c)
    if (!isValidNameStart(c))
        return false;

    while (i < length) {
        U16_NEXT(characters, i, length, c)
        if (!isValidNamePart(c))
            return false;
    }

    return true;
}

template<typename CharType>
static inline bool isValidNameASCII(const CharType* characters, unsigned length)
{
    CharType c = characters[0];
    if (!(isASCIIAlpha(c) || c == ':' || c == '_'))
        return false;

    for (unsigned i = 1; i < length; ++i) {
        c = characters[i];
        if (!(isASCIIAlphanumeric(c) || c == ':' || c == '_' || c == '-' || c == '.'))
            return false;
    }

    return true;
}

bool Document::isValidName(const String& name)
{
    unsigned length = name.length();
    if (!length)
        return false;

    const UChar* characters;
    if (name.is8Bit()) {
        if (isValidNameASCII(name.characters8(), length))
            return true;
        characters = name.characters();
    } else {
        characters = name.characters16();
        if (isValidNameASCII(characters, length))
            return true;
    }
    return isValidNameNonASCII(characters, length);
}

bool Document::parseQualifiedName(const String& qualifiedName, String& prefix, String& localName, ExceptionCode& ec)
{
    unsigned length = qualifiedName.length();

    if (!length) {
        ec = INVALID_CHARACTER_ERR;
        return false;
    }

    bool nameStart = true;
    bool sawColon = false;
    int colonPos = 0;

    const UChar* s = qualifiedName.characters();
    for (unsigned i = 0; i < length;) {
        UChar32 c;
        U16_NEXT(s, i, length, c)
        if (c == ':') {
            if (sawColon) {
                ec = NAMESPACE_ERR;
                return false; // multiple colons: not allowed
            }
            nameStart = true;
            sawColon = true;
            colonPos = i - 1;
        } else if (nameStart) {
            if (!isValidNameStart(c)) {
                ec = INVALID_CHARACTER_ERR;
                return false;
            }
            nameStart = false;
        } else {
            if (!isValidNamePart(c)) {
                ec = INVALID_CHARACTER_ERR;
                return false;
            }
        }
    }

    if (!sawColon) {
        prefix = String();
        localName = qualifiedName;
    } else {
        prefix = qualifiedName.substring(0, colonPos);
        if (prefix.isEmpty()) {
            ec = NAMESPACE_ERR;
            return false;
        }
        localName = qualifiedName.substring(colonPos + 1);
    }

    if (localName.isEmpty()) {
        ec = NAMESPACE_ERR;
        return false;
    }

    return true;
}

void Document::setDecoder(PassRefPtr<TextResourceDecoder> decoder)
{
    m_decoder = decoder;
}

KURL Document::completeURL(const String& url, const KURL& baseURLOverride) const
{
    // Always return a null URL when passed a null string.
    // FIXME: Should we change the KURL constructor to have this behavior?
    // See also [CSS]StyleSheet::completeURL(const String&)
    if (url.isNull())
        return KURL();
    const KURL& baseURL = ((baseURLOverride.isEmpty() || baseURLOverride == blankURL()) && parentDocument()) ? parentDocument()->baseURL() : baseURLOverride;
    if (!m_decoder)
        return KURL(baseURL, url);
    return KURL(baseURL, url, m_decoder->encoding());
}

KURL Document::completeURL(const String& url) const
{
    return completeURL(url, m_baseURL);
}

void Document::setInPageCache(bool flag)
{
    if (m_inPageCache == flag)
        return;

    m_inPageCache = flag;

    FrameView* v = view();
    if (flag) {
        ASSERT(!m_savedRenderer);
        m_savedRenderer = renderer();
        if (v) {
            v->cacheCurrentScrollPosition();
            if (page() && page()->mainFrame() == m_frame)
                v->resetScrollbarsAndClearContentsSize();
            else
                v->resetScrollbars();
        }
        m_styleRecalcTimer.stop();
    } else {
        ASSERT(!renderer() || renderer() == m_savedRenderer);
        ASSERT(m_renderArena);
        setRenderer(m_savedRenderer);
        m_savedRenderer = 0;

        if (childNeedsStyleRecalc())
            scheduleStyleRecalc();
    }
}

void Document::documentWillBecomeInactive()
{
#if USE(ACCELERATED_COMPOSITING)
    if (renderer())
        renderView()->willMoveOffscreen();
#endif
}

void Document::documentWillSuspendForPageCache()
{
    documentWillBecomeInactive();

    HashSet<Element*>::iterator end = m_documentSuspensionCallbackElements.end();
    for (HashSet<Element*>::iterator i = m_documentSuspensionCallbackElements.begin(); i != end; ++i)
        (*i)->documentWillSuspendForPageCache();

#ifndef NDEBUG
    // Clear the update flag to be able to check if the viewport arguments update
    // is dispatched, after the document is restored from the page cache.
    m_didDispatchViewportPropertiesChanged = false;
#endif
}

void Document::documentDidResumeFromPageCache() 
{
    Vector<Element*> elements;
    copyToVector(m_documentSuspensionCallbackElements, elements);
    Vector<Element*>::iterator end = elements.end();
    for (Vector<Element*>::iterator i = elements.begin(); i != end; ++i)
        (*i)->documentDidResumeFromPageCache();

#if USE(ACCELERATED_COMPOSITING)
    if (renderer())
        renderView()->didMoveOnscreen();
#endif

    if (FrameView* frameView = view())
        frameView->setAnimatorsAreActive();

    ASSERT(m_frame);
    m_frame->loader()->client()->dispatchDidBecomeFrameset(isFrameSet());
}

void Document::registerForPageCacheSuspensionCallbacks(Element* e)
{
    m_documentSuspensionCallbackElements.add(e);
}

void Document::unregisterForPageCacheSuspensionCallbacks(Element* e)
{
    m_documentSuspensionCallbackElements.remove(e);
}

void Document::mediaVolumeDidChange() 
{
    HashSet<Element*>::iterator end = m_mediaVolumeCallbackElements.end();
    for (HashSet<Element*>::iterator i = m_mediaVolumeCallbackElements.begin(); i != end; ++i)
        (*i)->mediaVolumeDidChange();
}

void Document::registerForMediaVolumeCallbacks(Element* e)
{
    m_mediaVolumeCallbackElements.add(e);
}

void Document::unregisterForMediaVolumeCallbacks(Element* e)
{
    m_mediaVolumeCallbackElements.remove(e);
}

void Document::storageBlockingStateDidChange()
{
    if (Settings* settings = this->settings())
        securityOrigin()->setStorageBlockingPolicy(settings->storageBlockingPolicy());
}

void Document::privateBrowsingStateDidChange() 
{
    HashSet<Element*>::iterator end = m_privateBrowsingStateChangedElements.end();
    for (HashSet<Element*>::iterator it = m_privateBrowsingStateChangedElements.begin(); it != end; ++it)
        (*it)->privateBrowsingStateDidChange();
}

void Document::registerForPrivateBrowsingStateChangedCallbacks(Element* e)
{
    m_privateBrowsingStateChangedElements.add(e);
}

void Document::unregisterForPrivateBrowsingStateChangedCallbacks(Element* e)
{
    m_privateBrowsingStateChangedElements.remove(e);
}

void Document::setShouldCreateRenderers(bool f)
{
    m_createRenderers = f;
}

bool Document::shouldCreateRenderers()
{
    return m_createRenderers;
}

// Support for Javascript execCommand, and related methods

static Editor::Command command(Document* document, const String& commandName, bool userInterface = false)
{
    Frame* frame = document->frame();
    if (!frame || frame->document() != document)
        return Editor::Command();

    document->updateStyleIfNeeded();

    return frame->editor()->command(commandName,
        userInterface ? CommandFromDOMWithUserInterface : CommandFromDOM);
}

bool Document::execCommand(const String& commandName, bool userInterface, const String& value)
{
    return command(this, commandName, userInterface).execute(value);
}

bool Document::queryCommandEnabled(const String& commandName)
{
    return command(this, commandName).isEnabled();
}

bool Document::queryCommandIndeterm(const String& commandName)
{
    return command(this, commandName).state() == MixedTriState;
}

bool Document::queryCommandState(const String& commandName)
{
    return command(this, commandName).state() == TrueTriState;
}

bool Document::queryCommandSupported(const String& commandName)
{
    return command(this, commandName).isSupported();
}

String Document::queryCommandValue(const String& commandName)
{
    return command(this, commandName).value();
}

KURL Document::openSearchDescriptionURL()
{
    static const char* const openSearchMIMEType = "application/opensearchdescription+xml";
    static const char* const openSearchRelation = "search";

    // FIXME: Why do only top-level frames have openSearchDescriptionURLs?
    if (!frame() || frame()->tree()->parent())
        return KURL();

    // FIXME: Why do we need to wait for FrameStateComplete?
    if (frame()->loader()->state() != FrameStateComplete)
        return KURL();

    if (!head())
        return KURL();

    RefPtr<HTMLCollection> children = head()->children();
    for (unsigned i = 0; Node* child = children->item(i); i++) {
        if (!child->hasTagName(linkTag))
            continue;
        HTMLLinkElement* linkElement = static_cast<HTMLLinkElement*>(child);
        if (!equalIgnoringCase(linkElement->type(), openSearchMIMEType) || !equalIgnoringCase(linkElement->rel(), openSearchRelation))
            continue;
        if (linkElement->href().isEmpty())
            continue;
        return linkElement->href();
    }

    return KURL();
}

#if ENABLE(XSLT)

void Document::applyXSLTransform(ProcessingInstruction* pi)
{
    RefPtr<XSLTProcessor> processor = XSLTProcessor::create();
    processor->setXSLStyleSheet(static_cast<XSLStyleSheet*>(pi->sheet()));
    String resultMIMEType;
    String newSource;
    String resultEncoding;
    if (!processor->transformToString(this, resultMIMEType, newSource, resultEncoding))
        return;
    // FIXME: If the transform failed we should probably report an error (like Mozilla does).
    processor->createDocumentFromSource(newSource, resultEncoding, resultMIMEType, this, frame());
}

void Document::setTransformSource(PassOwnPtr<TransformSource> source)
{
    m_transformSource = source;
}

#endif

void Document::setDesignMode(InheritedBool value)
{
    m_designMode = value;
    for (Frame* frame = m_frame; frame && frame->document(); frame = frame->tree()->traverseNext(m_frame))
        frame->document()->scheduleForcedStyleRecalc();
}

Document::InheritedBool Document::getDesignMode() const
{
    return m_designMode;
}

bool Document::inDesignMode() const
{
    for (const Document* d = this; d; d = d->parentDocument()) {
        if (d->m_designMode != inherit)
            return d->m_designMode;
    }
    return false;
}

Document* Document::parentDocument() const
{
    if (!m_frame)
        return 0;
    Frame* parent = m_frame->tree()->parent();
    if (!parent)
        return 0;
    return parent->document();
}

Document* Document::topDocument() const
{
    Document* doc = const_cast<Document *>(this);
    Element* element;
    while ((element = doc->ownerElement()))
        doc = element->document();
    
    return doc;
}

PassRefPtr<Attr> Document::createAttribute(const String& name, ExceptionCode& ec)
{
    return createAttributeNS(String(), name, ec, true);
}

PassRefPtr<Attr> Document::createAttributeNS(const String& namespaceURI, const String& qualifiedName, ExceptionCode& ec, bool shouldIgnoreNamespaceChecks)
{
    String prefix, localName;
    if (!parseQualifiedName(qualifiedName, prefix, localName, ec))
        return 0;

    QualifiedName qName(prefix, localName, namespaceURI);

    if (!shouldIgnoreNamespaceChecks && !hasValidNamespaceForAttributes(qName)) {
        ec = NAMESPACE_ERR;
        return 0;
    }

    return Attr::create(this, qName, emptyString());
}

#if ENABLE(SVG)
const SVGDocumentExtensions* Document::svgExtensions()
{
    return m_svgExtensions.get();
}

SVGDocumentExtensions* Document::accessSVGExtensions()
{
    if (!m_svgExtensions)
        m_svgExtensions = adoptPtr(new SVGDocumentExtensions(this));
    return m_svgExtensions.get();
}

bool Document::hasSVGRootNode() const
{
    return documentElement() && documentElement()->hasTagName(SVGNames::svgTag);
}
#endif

// FIXME: This caching mechanism should be merged that of DynamicNodeList in NodeRareData.
PassRefPtr<HTMLCollection> Document::cachedCollection(CollectionType type)
{
    ASSERT(static_cast<unsigned>(type) < NumUnnamedDocumentCachedTypes);
    if (m_collections[type])
        return m_collections[type];

    RefPtr<HTMLCollection> collection;
    if (type == DocAll)
        collection = HTMLAllCollection::create(this);
    else
        collection = HTMLCollection::create(this, type);
    m_collections[type] = collection.get();

    return collection.release();
}

void Document::removeCachedHTMLCollection(HTMLCollection* collection, CollectionType type)
{
    ASSERT_UNUSED(collection, m_collections[type] == collection);
    m_collections[type] = 0;
}

PassRefPtr<HTMLCollection> Document::images()
{
    return cachedCollection(DocImages);
}

PassRefPtr<HTMLCollection> Document::applets()
{
    return cachedCollection(DocApplets);
}

PassRefPtr<HTMLCollection> Document::embeds()
{
    return cachedCollection(DocEmbeds);
}

PassRefPtr<HTMLCollection> Document::plugins()
{
    // This is an alias for embeds() required for the JS DOM bindings.
    return cachedCollection(DocEmbeds);
}

PassRefPtr<HTMLCollection> Document::objects()
{
    return cachedCollection(DocObjects);
}

PassRefPtr<HTMLCollection> Document::scripts()
{
    return cachedCollection(DocScripts);
}

PassRefPtr<HTMLCollection> Document::links()
{
    return cachedCollection(DocLinks);
}

PassRefPtr<HTMLCollection> Document::forms()
{
    return cachedCollection(DocForms);
}

PassRefPtr<HTMLCollection> Document::anchors()
{
    return cachedCollection(DocAnchors);
}

PassRefPtr<HTMLCollection> Document::all()
{
    return cachedCollection(DocAll);
}

PassRefPtr<HTMLCollection> Document::windowNamedItems(const AtomicString& name)
{
    NamedCollectionMap::AddResult result = m_windowNamedItemCollections.add(name, 0);
    if (!result.isNewEntry)
        return result.iterator->second;

    RefPtr<HTMLNameCollection> collection = HTMLNameCollection::create(this, WindowNamedItems, name);
    result.iterator->second = collection.get();
    return collection.release();
}

PassRefPtr<HTMLCollection> Document::documentNamedItems(const AtomicString& name)
{
    NamedCollectionMap::AddResult result = m_documentNamedItemCollections.add(name, 0);
    if (!result.isNewEntry)
        return result.iterator->second;

    RefPtr<HTMLNameCollection> collection = HTMLNameCollection::create(this, DocumentNamedItems, name);
    result.iterator->second = collection.get();
    return collection.release();
}

// FIXME: This caching mechanism should be merged that of DynamicNodeList in NodeRareData.
void Document::removeWindowNamedItemCache(HTMLCollection* collection, const AtomicString& name)
{
    ASSERT_UNUSED(collection, m_windowNamedItemCollections.get(name) == collection);
    m_windowNamedItemCollections.remove(name);
}

void Document::removeDocumentNamedItemCache(HTMLCollection* collection, const AtomicString& name)
{
    ASSERT_UNUSED(collection, m_documentNamedItemCollections.get(name) == collection);
    m_documentNamedItemCollections.remove(name);
}

void Document::finishedParsing()
{
    ASSERT(!scriptableDocumentParser() || !m_parser->isParsing());
    ASSERT(!scriptableDocumentParser() || m_readyState != Loading);
    setParsing(false);
    if (!m_documentTiming.domContentLoadedEventStart)
        m_documentTiming.domContentLoadedEventStart = monotonicallyIncreasingTime();
    dispatchEvent(Event::create(eventNames().DOMContentLoadedEvent, true, false));
    if (!m_documentTiming.domContentLoadedEventEnd)
        m_documentTiming.domContentLoadedEventEnd = monotonicallyIncreasingTime();

    if (RefPtr<Frame> f = frame()) {
        // FrameLoader::finishedParsing() might end up calling Document::implicitClose() if all
        // resource loads are complete. HTMLObjectElements can start loading their resources from
        // post attach callbacks triggered by recalcStyle().  This means if we parse out an <object>
        // tag and then reach the end of the document without updating styles, we might not have yet
        // started the resource load and might fire the window load event too early.  To avoid this
        // we force the styles to be up to date before calling FrameLoader::finishedParsing().
        // See https://bugs.webkit.org/show_bug.cgi?id=36864 starting around comment 35.
        updateStyleIfNeeded();

        f->loader()->finishedParsing();

        InspectorInstrumentation::domContentLoadedEventFired(f.get());
    }

    // The ElementAttributeData sharing cache is only used during parsing since
    // that's when the majority of immutable attribute data will be created.
    m_immutableAttributeDataCache.clear();
}

PassRefPtr<XPathExpression> Document::createExpression(const String& expression,
                                                       XPathNSResolver* resolver,
                                                       ExceptionCode& ec)
{
    if (!m_xpathEvaluator)
        m_xpathEvaluator = XPathEvaluator::create();
    return m_xpathEvaluator->createExpression(expression, resolver, ec);
}

PassRefPtr<XPathNSResolver> Document::createNSResolver(Node* nodeResolver)
{
    if (!m_xpathEvaluator)
        m_xpathEvaluator = XPathEvaluator::create();
    return m_xpathEvaluator->createNSResolver(nodeResolver);
}

PassRefPtr<XPathResult> Document::evaluate(const String& expression,
                                           Node* contextNode,
                                           XPathNSResolver* resolver,
                                           unsigned short type,
                                           XPathResult* result,
                                           ExceptionCode& ec)
{
    if (!m_xpathEvaluator)
        m_xpathEvaluator = XPathEvaluator::create();
    return m_xpathEvaluator->evaluate(expression, contextNode, resolver, type, result, ec);
}

const Vector<IconURL>& Document::iconURLs()
{
    m_iconURLs.clear();

    if (!head() || !(head()->children()))
        return m_iconURLs;

    // Include any icons where type = link, rel = "shortcut icon".
    RefPtr<HTMLCollection> children = head()->children();
    unsigned int length = children->length();
    for (unsigned int i = 0; i < length; ++i) {
        Node* child = children->item(i);
        if (!child->hasTagName(linkTag))
            continue;
        HTMLLinkElement* linkElement = static_cast<HTMLLinkElement*>(child);
        if (linkElement->iconType() != Favicon)
            continue;
        if (linkElement->href().isEmpty())
            continue;

        // Put it at the front to ensure that icons seen later take precedence as required by the spec.
        IconURL newURL(linkElement->href(), linkElement->iconSizes(), linkElement->type(), linkElement->iconType());
        m_iconURLs.prepend(newURL);
    }

    return m_iconURLs;
}

void Document::addIconURL(const String& url, const String& mimeType, const String& sizes, IconType iconType)
{
    if (url.isEmpty())
        return;

    // FIXME - <rdar://problem/4727645> - At some point in the future, we might actually honor the "mimeType"
    IconURL newURL(KURL(ParsedURLString, url), sizes, mimeType, iconType);

    if (Frame* f = frame()) {
        IconURL iconURL = f->loader()->icon()->iconURL(iconType);
        if (iconURL == newURL)
            f->loader()->didChangeIcons(iconType);
    }
}

void Document::setUseSecureKeyboardEntryWhenActive(bool usesSecureKeyboard)
{
    if (m_useSecureKeyboardEntryWhenActive == usesSecureKeyboard)
        return;

    m_useSecureKeyboardEntryWhenActive = usesSecureKeyboard;
    m_frame->selection()->updateSecureKeyboardEntryIfActive();
}

bool Document::useSecureKeyboardEntryWhenActive() const
{
    return m_useSecureKeyboardEntryWhenActive;
}

static bool isEligibleForSeamless(Document* parent, Document* child)
{
    // It should not matter what we return for the top-most document.
    if (!parent)
        return false;
    if (parent->isSandboxed(SandboxSeamlessIframes))
        return false;
    if (child->isSrcdocDocument())
        return true;
    if (parent->securityOrigin()->canAccess(child->securityOrigin()))
        return true;
    return parent->securityOrigin()->canRequest(child->url());
}

void Document::initSecurityContext()
{
    if (haveInitializedSecurityOrigin()) {
        ASSERT(securityOrigin());
        return;
    }

    if (!m_frame) {
        // No source for a security context.
        // This can occur via document.implementation.createDocument().
        m_cookieURL = KURL(ParsedURLString, emptyString());
        setSecurityOrigin(SecurityOrigin::createUnique());
        setContentSecurityPolicy(ContentSecurityPolicy::create(this));
        return;
    }

    // In the common case, create the security context from the currently
    // loading URL with a fresh content security policy.
    m_cookieURL = m_url;
    enforceSandboxFlags(m_frame->loader()->effectiveSandboxFlags());
    setSecurityOrigin(isSandboxed(SandboxOrigin) ? SecurityOrigin::createUnique() : SecurityOrigin::create(m_url));
    setContentSecurityPolicy(ContentSecurityPolicy::create(this));

    if (SecurityPolicy::allowSubstituteDataAccessToLocal()) {
        // If this document was loaded with substituteData, then the document can
        // load local resources.  See https://bugs.webkit.org/show_bug.cgi?id=16756
        // and https://bugs.webkit.org/show_bug.cgi?id=19760 for further
        // discussion.
        
        DocumentLoader* documentLoader = loader();
        if (documentLoader && documentLoader->substituteData().isValid())
            securityOrigin()->grantLoadLocalResources();
    }

    if (Settings* settings = this->settings()) {
        if (!settings->isWebSecurityEnabled()) {
            // Web security is turned off. We should let this document access every other document. This is used primary by testing
            // harnesses for web sites.
            securityOrigin()->grantUniversalAccess();
        } else if (securityOrigin()->isLocal()) {
            if (settings->allowUniversalAccessFromFileURLs() || m_frame->loader()->client()->shouldForceUniversalAccessFromLocalURL(m_url)) {
                // Some clients want local URLs to have universal access, but that setting is dangerous for other clients.
                securityOrigin()->grantUniversalAccess();
            } else if (!settings->allowFileAccessFromFileURLs()) {
                // Some clients want local URLs to have even tighter restrictions by default, and not be able to access other local files.
                // FIXME 81578: The naming of this is confusing. Files with restricted access to other local files
                // still can have other privileges that can be remembered, thereby not making them unique origins.
                securityOrigin()->enforceFilePathSeparation();
            }
        }
        securityOrigin()->setStorageBlockingPolicy(settings->storageBlockingPolicy());
    }

    Document* parentDocument = ownerElement() ? ownerElement()->document() : 0;
    if (parentDocument && m_frame->loader()->shouldTreatURLAsSrcdocDocument(url())) {
        m_isSrcdocDocument = true;
        setBaseURLOverride(parentDocument->baseURL());
    }

    // FIXME: What happens if we inherit the security origin? This check may need to be later.
    // <iframe seamless src="about:blank"> likely won't work as-is.
    m_mayDisplaySeamlessWithParent = isEligibleForSeamless(parentDocument, this);

    if (!shouldInheritSecurityOriginFromOwner(m_url))
        return;

    // If we do not obtain a meaningful origin from the URL, then we try to
    // find one via the frame hierarchy.

    Frame* ownerFrame = m_frame->tree()->parent();
    if (!ownerFrame)
        ownerFrame = m_frame->loader()->opener();

    if (!ownerFrame) {
        didFailToInitializeSecurityOrigin();
        return;
    }

    if (isSandboxed(SandboxOrigin)) {
        // If we're supposed to inherit our security origin from our owner,
        // but we're also sandboxed, the only thing we inherit is the ability
        // to load local resources. This lets about:blank iframes in file://
        // URL documents load images and other resources from the file system.
        if (ownerFrame->document()->securityOrigin()->canLoadLocalResources())
            securityOrigin()->grantLoadLocalResources();
        return;
    }

    m_cookieURL = ownerFrame->document()->cookieURL();
    // We alias the SecurityOrigins to match Firefox, see Bug 15313
    // https://bugs.webkit.org/show_bug.cgi?id=15313
    setSecurityOrigin(ownerFrame->document()->securityOrigin());
}

void Document::initContentSecurityPolicy()
{
    if (!m_frame->tree()->parent() || (!shouldInheritSecurityOriginFromOwner(m_url) && !isPluginDocument()))
        return;

    contentSecurityPolicy()->copyStateFrom(m_frame->tree()->parent()->document()->contentSecurityPolicy());
}

void Document::didUpdateSecurityOrigin()
{
    if (!m_frame)
        return;
    m_frame->script()->updateSecurityOrigin();
}

bool Document::isContextThread() const
{
    return isMainThread();
}

void Document::updateURLForPushOrReplaceState(const KURL& url)
{
    Frame* f = frame();
    if (!f)
        return;

    setURL(url);
    f->loader()->setOutgoingReferrer(url);

    if (DocumentLoader* documentLoader = loader())
        documentLoader->replaceRequestURLForSameDocumentNavigation(url);
}

void Document::statePopped(PassRefPtr<SerializedScriptValue> stateObject)
{
    if (!frame())
        return;
    
    // Per step 11 of section 6.5.9 (history traversal) of the HTML5 spec, we 
    // defer firing of popstate until we're in the complete state.
    if (m_readyState == Complete)
        enqueuePopstateEvent(stateObject);
    else
        m_pendingStateObject = stateObject;
}

void Document::updateFocusAppearanceSoon(bool restorePreviousSelection)
{
    m_updateFocusAppearanceRestoresSelection = restorePreviousSelection;
    if (!m_updateFocusAppearanceTimer.isActive())
        m_updateFocusAppearanceTimer.startOneShot(0);
}

void Document::cancelFocusAppearanceUpdate()
{
    m_updateFocusAppearanceTimer.stop();
}

void Document::updateFocusAppearanceTimerFired(Timer<Document>*)
{
    Node* node = focusedNode();
    if (!node)
        return;
    if (!node->isElementNode())
        return;

    updateLayout();

    Element* element = static_cast<Element*>(node);
    if (element->isFocusable())
        element->updateFocusAppearance(m_updateFocusAppearanceRestoresSelection);
}

void Document::attachRange(Range* range)
{
    ASSERT(!m_ranges.contains(range));
    m_ranges.add(range);
}

void Document::detachRange(Range* range)
{
    // We don't ASSERT m_ranges.contains(range) to allow us to call this
    // unconditionally to fix: https://bugs.webkit.org/show_bug.cgi?id=26044
    m_ranges.remove(range);
}

CanvasRenderingContext* Document::getCSSCanvasContext(const String& type, const String& name, int width, int height)
{
    HTMLCanvasElement* element = getCSSCanvasElement(name);
    if (!element)
        return 0;
    element->setSize(IntSize(width, height));
    return element->getContext(type);
}

HTMLCanvasElement* Document::getCSSCanvasElement(const String& name)
{
    RefPtr<HTMLCanvasElement>& element = m_cssCanvasElements.add(name, 0).iterator->second;
    if (!element)
        element = HTMLCanvasElement::create(this);
    return element.get();
}

void Document::initDNSPrefetch()
{
    Settings* settings = this->settings();

    m_haveExplicitlyDisabledDNSPrefetch = false;
    m_isDNSPrefetchEnabled = settings && settings->dnsPrefetchingEnabled() && securityOrigin()->protocol() == "http";

    // Inherit DNS prefetch opt-out from parent frame    
    if (Document* parent = parentDocument()) {
        if (!parent->isDNSPrefetchEnabled())
            m_isDNSPrefetchEnabled = false;
    }
}

void Document::parseDNSPrefetchControlHeader(const String& dnsPrefetchControl)
{
    if (equalIgnoringCase(dnsPrefetchControl, "on") && !m_haveExplicitlyDisabledDNSPrefetch) {
        m_isDNSPrefetchEnabled = true;
        return;
    }

    m_isDNSPrefetchEnabled = false;
    m_haveExplicitlyDisabledDNSPrefetch = true;
}

void Document::addMessage(MessageSource source, MessageType type, MessageLevel level, const String& message, const String& sourceURL, unsigned lineNumber, PassRefPtr<ScriptCallStack> callStack)
{
    if (!isContextThread()) {
        postTask(AddConsoleMessageTask::create(source, type, level, message));
        return;
    }

    if (DOMWindow* window = domWindow())
        window->console()->addMessage(source, type, level, message, sourceURL, lineNumber, callStack);
}

struct PerformTaskContext {
    WTF_MAKE_NONCOPYABLE(PerformTaskContext); WTF_MAKE_FAST_ALLOCATED;
public:
    PerformTaskContext(PassRefPtr<DocumentWeakReference> documentReference, PassOwnPtr<ScriptExecutionContext::Task> task)
        : documentReference(documentReference)
        , task(task)
    {
    }

    RefPtr<DocumentWeakReference> documentReference;
    OwnPtr<ScriptExecutionContext::Task> task;
};

void Document::didReceiveTask(void* untypedContext)
{
    ASSERT(isMainThread());

    OwnPtr<PerformTaskContext> context = adoptPtr(static_cast<PerformTaskContext*>(untypedContext));
    ASSERT(context);

    Document* document = context->documentReference->document();
    if (!document)
        return;

    Page* page = document->page();
    if ((page && page->defersLoading()) || !document->m_pendingTasks.isEmpty()) {
        document->m_pendingTasks.append(context->task.release());
        return;
    }

    context->task->performTask(document);
}

void Document::postTask(PassOwnPtr<Task> task)
{
    callOnMainThread(didReceiveTask, new PerformTaskContext(m_weakReference, task));
}

void Document::pendingTasksTimerFired(Timer<Document>*)
{
    while (!m_pendingTasks.isEmpty()) {
        OwnPtr<Task> task = m_pendingTasks[0].release();
        m_pendingTasks.remove(0);
        task->performTask(this);
    }
}

void Document::suspendScheduledTasks(ActiveDOMObject::ReasonForSuspension reason)
{
    ASSERT(!m_scheduledTasksAreSuspended);

    suspendScriptedAnimationControllerCallbacks();
    suspendActiveDOMObjects(reason);
    scriptRunner()->suspend();
    m_pendingTasksTimer.stop();
    if (m_parser)
        m_parser->suspendScheduledTasks();

    m_scheduledTasksAreSuspended = true;
}

void Document::resumeScheduledTasks()
{
    ASSERT(m_scheduledTasksAreSuspended);

    if (m_parser)
        m_parser->resumeScheduledTasks();
    if (!m_pendingTasks.isEmpty())
        m_pendingTasksTimer.startOneShot(0);
    scriptRunner()->resume();
    resumeActiveDOMObjects();
    resumeScriptedAnimationControllerCallbacks();
    
    m_scheduledTasksAreSuspended = false;
}

void Document::suspendScriptedAnimationControllerCallbacks()
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->suspend();
#endif
}

void Document::resumeScriptedAnimationControllerCallbacks()
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->resume();
#endif
}

void Document::windowScreenDidChange(PlatformDisplayID displayID)
{
#if ENABLE(REQUEST_ANIMATION_FRAME)
    if (m_scriptedAnimationController)
        m_scriptedAnimationController->windowScreenDidChange(displayID);
#else
    UNUSED_PARAM(displayID);
#endif
}


String Document::displayStringModifiedByEncoding(const String& str) const
{
    if (m_decoder)
        return m_decoder->encoding().displayString(str.impl());
    return str;
}

PassRefPtr<StringImpl> Document::displayStringModifiedByEncoding(PassRefPtr<StringImpl> str) const
{
    if (m_decoder)
        return m_decoder->encoding().displayString(str);
    return str;
}

void Document::displayBufferModifiedByEncoding(UChar* buffer, unsigned len) const
{
    if (m_decoder)
        m_decoder->encoding().displayBuffer(buffer, len);
}

void Document::enqueuePageshowEvent(PageshowEventPersistence persisted)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36334 Pageshow event needs to fire asynchronously.
    dispatchWindowEvent(PageTransitionEvent::create(eventNames().pageshowEvent, persisted), this);
}

void Document::enqueueHashchangeEvent(const String& oldURL, const String& newURL)
{
    enqueueWindowEvent(HashChangeEvent::create(oldURL, newURL));
}

void Document::enqueuePopstateEvent(PassRefPtr<SerializedScriptValue> stateObject)
{
    // FIXME: https://bugs.webkit.org/show_bug.cgi?id=36202 Popstate event needs to fire asynchronously
    dispatchWindowEvent(PopStateEvent::create(stateObject, domWindow() ? domWindow()->history() : 0));
}

void Document::addMediaCanStartListener(MediaCanStartListener* listener)
{
    ASSERT(!m_mediaCanStartListeners.contains(listener));
    m_mediaCanStartListeners.add(listener);
}

void Document::removeMediaCanStartListener(MediaCanStartListener* listener)
{
    ASSERT(m_mediaCanStartListeners.contains(listener));
    m_mediaCanStartListeners.remove(listener);
}

MediaCanStartListener* Document::takeAnyMediaCanStartListener()
{
    HashSet<MediaCanStartListener*>::iterator slot = m_mediaCanStartListeners.begin();
    if (slot == m_mediaCanStartListeners.end())
        return 0;
    MediaCanStartListener* listener = *slot;
    m_mediaCanStartListeners.remove(slot);
    return listener;
}

#if ENABLE(FULLSCREEN_API)
bool Document::fullScreenIsAllowedForElement(Element* element) const
{
    ASSERT(element);
    return isAttributeOnAllOwners(webkitallowfullscreenAttr, element->document()->ownerElement());
}

void Document::requestFullScreenForElement(Element* element, unsigned short flags, FullScreenCheckType checkType)
{
    // The Mozilla Full Screen API <https://wiki.mozilla.org/Gecko:FullScreenAPI> has different requirements
    // for full screen mode, and do not have the concept of a full screen element stack.
    bool inLegacyMozillaMode = (flags & Element::LEGACY_MOZILLA_REQUEST);

    do {
        if (!element)
            element = documentElement();
 
        // 1. If any of the following conditions are true, terminate these steps and queue a task to fire
        // an event named fullscreenerror with its bubbles attribute set to true on the context object's 
        // node document:

        // The context object is not in a document.
        if (!element->inDocument())
            break;

        // The context object's node document, or an ancestor browsing context's document does not have
        // the fullscreen enabled flag set.
        if (checkType == EnforceIFrameAllowFullScreenRequirement && !fullScreenIsAllowedForElement(element))
            break;

        // The context object's node document fullscreen element stack is not empty and its top element
        // is not an ancestor of the context object. (NOTE: Ignore this requirement if the request was
        // made via the legacy Mozilla-style API.)
        if (!m_fullScreenElementStack.isEmpty() && !m_fullScreenElementStack.first()->contains(element) && !inLegacyMozillaMode)
            break;

        // A descendant browsing context's document has a non-empty fullscreen element stack.
        bool descendentHasNonEmptyStack = false;
        for (Frame* descendant = frame() ? frame()->tree()->traverseNext() : 0; descendant; descendant = descendant->tree()->traverseNext()) {
            if (descendant->document()->webkitFullscreenElement()) {
                descendentHasNonEmptyStack = true;
                break;
            }
        }
        if (descendentHasNonEmptyStack && !inLegacyMozillaMode)
            break;

        // This algorithm is not allowed to show a pop-up:
        //   An algorithm is allowed to show a pop-up if, in the task in which the algorithm is running, either:
        //   - an activation behavior is currently being processed whose click event was trusted, or
        //   - the event listener for a trusted click event is being handled.
        if (!ScriptController::processingUserGesture())
            break;

        // There is a previously-established user preference, security risk, or platform limitation.
        if (!page() || !page()->settings()->fullScreenEnabled())
            break;
        
        if (!page()->chrome()->client()->supportsFullScreenForElement(element, flags & Element::ALLOW_KEYBOARD_INPUT))
            break;

        // 2. Let doc be element's node document. (i.e. "this")
        Document* currentDoc = this;

        // 3. Let docs be all doc's ancestor browsing context's documents (if any) and doc.
        Deque<Document*> docs;

        do {
            docs.prepend(currentDoc);
            currentDoc = currentDoc->ownerElement() ? currentDoc->ownerElement()->document() : 0;
        } while (currentDoc);

        // 4. For each document in docs, run these substeps:
        Deque<Document*>::iterator current = docs.begin(), following = docs.begin();

        do {
            ++following;

            // 1. Let following document be the document after document in docs, or null if there is no
            // such document.
            Document* currentDoc = *current;
            Document* followingDoc = following != docs.end() ? *following : 0;

            // 2. If following document is null, push context object on document's fullscreen element
            // stack, and queue a task to fire an event named fullscreenchange with its bubbles attribute
            // set to true on the document.
            if (!followingDoc) {
                currentDoc->pushFullscreenElementStack(element);
                addDocumentToFullScreenChangeEventQueue(currentDoc);
                continue;
            }

            // 3. Otherwise, if document's fullscreen element stack is either empty or its top element
            // is not following document's browsing context container,
            Element* topElement = currentDoc->webkitFullscreenElement();
            if (!topElement || topElement != followingDoc->ownerElement()) {
                // ...push following document's browsing context container on document's fullscreen element
                // stack, and queue a task to fire an event named fullscreenchange with its bubbles attribute
                // set to true on document.
                currentDoc->pushFullscreenElementStack(followingDoc->ownerElement());
                addDocumentToFullScreenChangeEventQueue(currentDoc);
                continue;
            }

            // 4. Otherwise, do nothing for this document. It stays the same.
        } while (++current != docs.end());

        // 5. Return, and run the remaining steps asynchronously.
        // 6. Optionally, perform some animation.
        m_areKeysEnabledInFullScreen = flags & Element::ALLOW_KEYBOARD_INPUT;
        page()->chrome()->client()->enterFullScreenForElement(element);

        // 7. Optionally, display a message indicating how the user can exit displaying the context object fullscreen.
        return;
    } while (0);

    m_fullScreenErrorEventTargetQueue.append(element ? element : documentElement());
    m_fullScreenChangeDelayTimer.startOneShot(0);
}

void Document::webkitCancelFullScreen()
{
    // The Mozilla "cancelFullScreen()" API behaves like the W3C "fully exit fullscreen" behavior, which
    // is defined as: 
    // "To fully exit fullscreen act as if the exitFullscreen() method was invoked on the top-level browsing
    // context's document and subsequently empty that document's fullscreen element stack."
    if (!topDocument()->webkitFullscreenElement())
        return;

    // To achieve that aim, remove all the elements from the top document's stack except for the first before
    // calling webkitExitFullscreen():
    Deque<RefPtr<Element> > replacementFullscreenElementStack;
    replacementFullscreenElementStack.prepend(topDocument()->webkitFullscreenElement());
    topDocument()->m_fullScreenElementStack.swap(replacementFullscreenElementStack);

    topDocument()->webkitExitFullscreen();
}

void Document::webkitExitFullscreen()
{
    // The exitFullscreen() method must run these steps:
    
    // 1. Let doc be the context object. (i.e. "this")
    Document* currentDoc = this;

    // 2. If doc's fullscreen element stack is empty, terminate these steps.
    if (m_fullScreenElementStack.isEmpty())
        return;
    
    // 3. Let descendants be all the doc's descendant browsing context's documents with a non-empty fullscreen
    // element stack (if any), ordered so that the child of the doc is last and the document furthest
    // away from the doc is first.
    Deque<RefPtr<Document> > descendants;
    for (Frame* descendant = frame() ? frame()->tree()->traverseNext() : 0; descendant; descendant = descendant->tree()->traverseNext()) {
        if (descendant->document()->webkitFullscreenElement())
            descendants.prepend(descendant->document());
    }
        
    // 4. For each descendant in descendants, empty descendant's fullscreen element stack, and queue a
    // task to fire an event named fullscreenchange with its bubbles attribute set to true on descendant.
    for (Deque<RefPtr<Document> >::iterator i = descendants.begin(); i != descendants.end(); ++i) {
        (*i)->clearFullscreenElementStack();
        addDocumentToFullScreenChangeEventQueue(i->get());
    }

    // 5. While doc is not null, run these substeps:
    Element* newTop = 0;
    while (currentDoc) {
        // 1. Pop the top element of doc's fullscreen element stack.
        currentDoc->popFullscreenElementStack();

        //    If doc's fullscreen element stack is non-empty and the element now at the top is either
        //    not in a document or its node document is not doc, repeat this substep.
        newTop = currentDoc->webkitFullscreenElement();
        if (newTop && (!newTop->inDocument() || newTop->document() != currentDoc))
            continue;

        // 2. Queue a task to fire an event named fullscreenchange with its bubbles attribute set to true
        // on doc.
        addDocumentToFullScreenChangeEventQueue(currentDoc);

        // 3. If doc's fullscreen element stack is empty and doc's browsing context has a browsing context
        // container, set doc to that browsing context container's node document.
        if (!newTop && currentDoc->ownerElement()) {
            currentDoc = currentDoc->ownerElement()->document();
            continue;
        }

        // 4. Otherwise, set doc to null.
        currentDoc = 0;
    }

    // 6. Return, and run the remaining steps asynchronously.
    // 7. Optionally, perform some animation.

    // Only exit out of full screen window mode if there are no remaining elements in the 
    // full screen stack.
    if (!newTop) {
        page()->chrome()->client()->exitFullScreenForElement(m_fullScreenElement.get());
        return;
    }

    // Otherwise, notify the chrome of the new full screen element.
    page()->chrome()->client()->enterFullScreenForElement(newTop);      
}

bool Document::webkitFullscreenEnabled() const
{
    // 4. The fullscreenEnabled attribute must return true if the context object and all ancestor
    // browsing context's documents have their fullscreen enabled flag set, or false otherwise.

    // Top-level browsing contexts are implied to have their allowFullScreen attribute set.
    return isAttributeOnAllOwners(webkitallowfullscreenAttr, ownerElement());
}

void Document::webkitWillEnterFullScreenForElement(Element* element)
{
    if (!attached() || inPageCache())
        return;

    ASSERT(element);

    // Protect against being called after the document has been removed from the page.
    if (!page())
        return;

    ASSERT(page()->settings()->fullScreenEnabled());

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->unwrapRenderer();

    m_fullScreenElement = element;

#if USE(NATIVE_FULLSCREEN_VIDEO)
    if (element && element->isMediaElement())
        return;
#endif

    // Create a placeholder block for a the full-screen element, to keep the page from reflowing
    // when the element is removed from the normal flow.  Only do this for a RenderBox, as only 
    // a box will have a frameRect.  The placeholder will be created in setFullScreenRenderer()
    // during layout.
    RenderObject* renderer = m_fullScreenElement->renderer();
    bool shouldCreatePlaceholder = renderer && renderer->isBox();
    if (shouldCreatePlaceholder) {
        m_savedPlaceholderFrameRect = toRenderBox(renderer)->frameRect();
        m_savedPlaceholderRenderStyle = RenderStyle::clone(renderer->style());
    }

    if (m_fullScreenElement != documentElement())
        RenderFullScreen::wrapRenderer(renderer, renderer ? renderer->parent() : 0, this);

    m_fullScreenElement->setContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(true);
    
    recalcStyle(Force);
}

void Document::webkitDidEnterFullScreenForElement(Element*)
{
    if (!m_fullScreenElement)
        return;

    if (!attached() || inPageCache())
        return;

    m_fullScreenElement->didBecomeFullscreenElement();

    m_fullScreenChangeDelayTimer.startOneShot(0);
}

void Document::webkitWillExitFullScreenForElement(Element*)
{
    if (!m_fullScreenElement)
        return;

    if (!attached() || inPageCache())
        return;

    m_fullScreenElement->willStopBeingFullscreenElement();
}

void Document::webkitDidExitFullScreenForElement(Element*)
{
    if (!m_fullScreenElement)
        return;

    if (!attached() || inPageCache())
        return;

    m_fullScreenElement->setContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(false);

    m_areKeysEnabledInFullScreen = false;
    
    if (m_fullScreenRenderer)
        m_fullScreenRenderer->unwrapRenderer();

    m_fullScreenElement = 0;
    scheduleForcedStyleRecalc();
    
    // When webkitCancelFullScreen is called, we call webkitExitFullScreen on the topDocument(). That
    // means that the events will be queued there. So if we have no events here, start the timer on
    // the exiting document.
    Document* exitingDocument = this;
    if (m_fullScreenChangeEventTargetQueue.isEmpty() && m_fullScreenErrorEventTargetQueue.isEmpty())
        exitingDocument = topDocument();
    exitingDocument->m_fullScreenChangeDelayTimer.startOneShot(0);
}
    
void Document::setFullScreenRenderer(RenderFullScreen* renderer)
{
    if (renderer == m_fullScreenRenderer)
        return;

    if (renderer && m_savedPlaceholderRenderStyle) 
        renderer->createPlaceholder(m_savedPlaceholderRenderStyle.release(), m_savedPlaceholderFrameRect);
    else if (renderer && m_fullScreenRenderer && m_fullScreenRenderer->placeholder()) {
        RenderBlock* placeholder = m_fullScreenRenderer->placeholder();
        renderer->createPlaceholder(RenderStyle::clone(placeholder->style()), placeholder->frameRect());
    }

    if (m_fullScreenRenderer)
        m_fullScreenRenderer->destroy();
    ASSERT(!m_fullScreenRenderer);

    m_fullScreenRenderer = renderer;
    
    // This notification can come in after the page has been destroyed.
    if (page())
        page()->chrome()->client()->fullScreenRendererChanged(m_fullScreenRenderer);
}

void Document::fullScreenRendererDestroyed()
{
    m_fullScreenRenderer = 0;

    if (page())
        page()->chrome()->client()->fullScreenRendererChanged(0);
}

void Document::setFullScreenRendererSize(const IntSize& size)
{
    ASSERT(m_fullScreenRenderer);
    if (!m_fullScreenRenderer)
        return;
    
    if (m_fullScreenRenderer) {
        RefPtr<RenderStyle> newStyle = RenderStyle::clone(m_fullScreenRenderer->style());
        newStyle->setWidth(Length(size.width(), WebCore::Fixed));
        newStyle->setHeight(Length(size.height(), WebCore::Fixed));
        newStyle->setTop(Length(0, WebCore::Fixed));
        newStyle->setLeft(Length(0, WebCore::Fixed));
        m_fullScreenRenderer->setStyle(newStyle);
        updateLayout();
    }
}
    
void Document::setFullScreenRendererBackgroundColor(Color backgroundColor)
{
    if (!m_fullScreenRenderer)
        return;
    
    RefPtr<RenderStyle> newStyle = RenderStyle::clone(m_fullScreenRenderer->style());
    newStyle->setBackgroundColor(backgroundColor);
    m_fullScreenRenderer->setStyle(newStyle);
}
    
void Document::fullScreenChangeDelayTimerFired(Timer<Document>*)
{
    Deque<RefPtr<Node> > changeQueue;
    m_fullScreenChangeEventTargetQueue.swap(changeQueue);

    while (!changeQueue.isEmpty()) {
        RefPtr<Node> node = changeQueue.takeFirst();
        if (!node)
            node = documentElement();

        // If the element was removed from our tree, also message the documentElement. Since we may
        // have a document hierarchy, check that node isn't in another document.
        if (!contains(node.get()) && !node->inDocument())
            changeQueue.append(documentElement());
        
        node->dispatchEvent(Event::create(eventNames().webkitfullscreenchangeEvent, true, false));
    }

    Deque<RefPtr<Node> > errorQueue;
    m_fullScreenErrorEventTargetQueue.swap(errorQueue);
    
    while (!errorQueue.isEmpty()) {
        RefPtr<Node> node = errorQueue.takeFirst();
        if (!node)
            node = documentElement();
        
        // If the element was removed from our tree, also message the documentElement. Since we may
        // have a document hierarchy, check that node isn't in another document.
        if (!contains(node.get()) && !node->inDocument())
            errorQueue.append(documentElement());
        
        node->dispatchEvent(Event::create(eventNames().webkitfullscreenerrorEvent, true, false));
    }
}

void Document::fullScreenElementRemoved()
{
    m_fullScreenElement->setContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(false);
    webkitCancelFullScreen();
}

void Document::removeFullScreenElementOfSubtree(Node* node, bool amongChildrenOnly)
{
    if (!m_fullScreenElement)
        return;
    
    bool elementInSubtree = false;
    if (amongChildrenOnly)
        elementInSubtree = m_fullScreenElement->isDescendantOf(node);
    else
        elementInSubtree = (m_fullScreenElement == node) || m_fullScreenElement->isDescendantOf(node);
    
    if (elementInSubtree)
        fullScreenElementRemoved();
}

bool Document::isAnimatingFullScreen() const
{
    return m_isAnimatingFullScreen;
}

void Document::setAnimatingFullScreen(bool flag)
{
    if (m_isAnimatingFullScreen == flag)
        return;
    m_isAnimatingFullScreen = flag;

    if (m_fullScreenElement && m_fullScreenElement->isDescendantOf(this)) {
        m_fullScreenElement->setNeedsStyleRecalc();
        scheduleForcedStyleRecalc();
    }
}

void Document::clearFullscreenElementStack()
{
    m_fullScreenElementStack.clear();
}

void Document::popFullscreenElementStack()
{
    if (m_fullScreenElementStack.isEmpty())
        return;

    m_fullScreenElementStack.removeFirst();
}

void Document::pushFullscreenElementStack(Element* element)
{
    m_fullScreenElementStack.prepend(element);
}

void Document::addDocumentToFullScreenChangeEventQueue(Document* doc)
{
    ASSERT(doc);
    Node* target = doc->webkitFullscreenElement();
    if (!target)
        target = doc->webkitCurrentFullScreenElement();
    if (!target)
        target = doc;
    m_fullScreenChangeEventTargetQueue.append(target);
}
#endif

#if ENABLE(POINTER_LOCK)
void Document::webkitExitPointerLock()
{
    if (!page())
        return;
    if (Element* target = page()->pointerLockController()->element()) {
        if (target->document() != this)
            return;
    }
    page()->pointerLockController()->requestPointerUnlock();
}

Element* Document::webkitPointerLockElement() const
{
    if (!page() || page()->pointerLockController()->lockPending())
        return 0;
    if (Element* element = page()->pointerLockController()->element()) {
        if (element->document() == this)
            return element;
    }
    return 0;
}
#endif

void Document::decrementLoadEventDelayCount()
{
    ASSERT(m_loadEventDelayCount);
    --m_loadEventDelayCount;

    if (frame() && !m_loadEventDelayCount && !m_loadEventDelayTimer.isActive())
        m_loadEventDelayTimer.startOneShot(0);
}

void Document::loadEventDelayTimerFired(Timer<Document>*)
{
    if (frame())
        frame()->loader()->checkCompleted();
}

#if ENABLE(REQUEST_ANIMATION_FRAME)
int Document::webkitRequestAnimationFrame(PassRefPtr<RequestAnimationFrameCallback> callback)
{
    if (!m_scriptedAnimationController) {
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
        m_scriptedAnimationController = ScriptedAnimationController::create(this, page() ? page()->displayID() : 0);
#else
        m_scriptedAnimationController = ScriptedAnimationController::create(this, 0);
#endif
        // It's possible that the Page may have suspended scripted animations before
        // we were created. We need to make sure that we don't start up the animation
        // controller on a background tab, for example.
        if (!page() || page()->scriptedAnimationsSuspended())
            m_scriptedAnimationController->suspend();
    }

    return m_scriptedAnimationController->registerCallback(callback);
}

void Document::webkitCancelAnimationFrame(int id)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->cancelCallback(id);
}

void Document::serviceScriptedAnimations(DOMTimeStamp time)
{
    if (!m_scriptedAnimationController)
        return;
    m_scriptedAnimationController->serviceScriptedAnimations(time);
}
#endif

#if ENABLE(TOUCH_EVENTS)
PassRefPtr<Touch> Document::createTouch(DOMWindow* window, EventTarget* target, int identifier, int pageX, int pageY, int screenX, int screenY, int radiusX, int radiusY, float rotationAngle, float force, ExceptionCode&) const
{
    // FIXME: It's not clear from the documentation at
    // http://developer.apple.com/library/safari/#documentation/UserExperience/Reference/DocumentAdditionsReference/DocumentAdditions/DocumentAdditions.html
    // when this method should throw and nor is it by inspection of iOS behavior. It would be nice to verify any cases where it throws under iOS
    // and implement them here. See https://bugs.webkit.org/show_bug.cgi?id=47819
    Frame* frame = window ? window->frame() : this->frame();
    return Touch::create(frame, target, identifier, screenX, screenY, pageX, pageY, radiusX, radiusY, rotationAngle, force);
}
#endif

static void wheelEventHandlerCountChanged(Document* document)
{
    Page* page = document->page();
    if (!page)
        return;

    ScrollingCoordinator* scrollingCoordinator = page->scrollingCoordinator();
    if (!scrollingCoordinator)
        return;

    FrameView* frameView = document->view();
    if (!frameView)
        return;

    scrollingCoordinator->frameViewWheelEventHandlerCountChanged(frameView);
}

void Document::didAddWheelEventHandler()
{
    ++m_wheelEventHandlerCount;
    Frame* mainFrame = page() ? page()->mainFrame() : 0;
    if (mainFrame)
        mainFrame->notifyChromeClientWheelEventHandlerCountChanged();

    wheelEventHandlerCountChanged(this);
}

void Document::didRemoveWheelEventHandler()
{
    ASSERT(m_wheelEventHandlerCount > 0);
    --m_wheelEventHandlerCount;
    Frame* mainFrame = page() ? page()->mainFrame() : 0;
    if (mainFrame)
        mainFrame->notifyChromeClientWheelEventHandlerCountChanged();

    wheelEventHandlerCountChanged(this);
}

void Document::didAddTouchEventHandler()
{
#if ENABLE(TOUCH_EVENTS)
    ++m_touchEventHandlerCount;
    if (m_touchEventHandlerCount > 1)
        return;
    if (Page* page = this->page())
        page->chrome()->client()->needTouchEvents(true);
#endif
}

void Document::didRemoveTouchEventHandler()
{
#if ENABLE(TOUCH_EVENTS)
    ASSERT(m_touchEventHandlerCount);
    --m_touchEventHandlerCount;
    if (m_touchEventHandlerCount)
        return;

    Page* page = this->page();
    if (!page)
        return;
    for (const Frame* frame = page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (frame->document() && frame->document()->touchEventHandlerCount())
            return;
    }
    page->chrome()->client()->needTouchEvents(false);
#endif
}

HTMLIFrameElement* Document::seamlessParentIFrame() const
{
    if (!shouldDisplaySeamlesslyWithParent())
        return 0;

    HTMLFrameOwnerElement* ownerElement = this->ownerElement();
    ASSERT(ownerElement->hasTagName(iframeTag));
    return static_cast<HTMLIFrameElement*>(ownerElement);
}

bool Document::shouldDisplaySeamlesslyWithParent() const
{
#if ENABLE(IFRAME_SEAMLESS)
    HTMLFrameOwnerElement* ownerElement = this->ownerElement();
    if (!ownerElement)
        return false;
    return m_mayDisplaySeamlessWithParent && ownerElement->hasTagName(iframeTag) && ownerElement->fastHasAttribute(seamlessAttr);
#else
    return false;
#endif
}

DocumentLoader* Document::loader() const
{
    if (!m_frame)
        return 0;
    
    DocumentLoader* loader = m_frame->loader()->documentLoader();
    if (!loader)
        return 0;
    
    if (m_frame->document() != this)
        return 0;
    
    return loader;
}

#if ENABLE(MICRODATA)
PassRefPtr<NodeList> Document::getItems(const String& typeNames)
{
    // Since documet.getItem() is allowed for microdata, typeNames will be null string.
    // In this case we need to create an empty string identifier to map such request in the cache.
    String localTypeNames = typeNames.isNull() ? MicroDataItemList::undefinedItemType() : typeNames;

    return ensureRareData()->ensureNodeLists()->addCacheWithName<MicroDataItemList>(this, DynamicNodeList::MicroDataItemListType, localTypeNames);
}
#endif

IntSize Document::viewportSize() const
{
    if (!view())
        return IntSize();
    return view()->visibleContentRect(/* includeScrollbars */ true).size();
}

Node* eventTargetNodeForDocument(Document* doc)
{
    if (!doc)
        return 0;
    Node* node = doc->focusedNode();
    if (!node && doc->isPluginDocument()) {
        PluginDocument* pluginDocument = static_cast<PluginDocument*>(doc);
        node =  pluginDocument->pluginNode();
    }
    if (!node && doc->isHTMLDocument())
        node = doc->body();
    if (!node)
        node = doc->documentElement();
    return node;
}

void Document::adjustFloatQuadsForScrollAndAbsoluteZoomAndFrameScale(Vector<FloatQuad>& quads, RenderObject* renderer)
{
    if (!view())
        return;

    float inverseFrameScale = 1;
    if (frame())
        inverseFrameScale = 1 / frame()->frameScaleFactor();

    LayoutRect visibleContentRect = view()->visibleContentRect();
    for (size_t i = 0; i < quads.size(); ++i) {
        quads[i].move(-visibleContentRect.x(), -visibleContentRect.y());
        adjustFloatQuadForAbsoluteZoom(quads[i], renderer);
        if (inverseFrameScale != 1)
            quads[i].scale(inverseFrameScale, inverseFrameScale);
    }
}

void Document::adjustFloatRectForScrollAndAbsoluteZoomAndFrameScale(FloatRect& rect, RenderObject* renderer)
{
    if (!view())
        return;

    float inverseFrameScale = 1;
    if (frame())
        inverseFrameScale = 1 / frame()->frameScaleFactor();

    LayoutRect visibleContentRect = view()->visibleContentRect();
    rect.move(-visibleContentRect.x(), -visibleContentRect.y());
    adjustFloatRectForAbsoluteZoom(rect, renderer);
    if (inverseFrameScale != 1)
        rect.scale(inverseFrameScale);
}

void Document::setContextFeatures(PassRefPtr<ContextFeatures> features)
{
    m_contextFeatures = features;
}

static RenderObject* nearestCommonHoverAncestor(RenderObject* obj1, RenderObject* obj2)
{
    if (!obj1 || !obj2)
        return 0;

    for (RenderObject* currObj1 = obj1; currObj1; currObj1 = currObj1->hoverAncestor()) {
        for (RenderObject* currObj2 = obj2; currObj2; currObj2 = currObj2->hoverAncestor()) {
            if (currObj1 == currObj2)
                return currObj1;
        }
    }

    return 0;
}

void Document::updateHoverActiveState(const HitTestRequest& request, HitTestResult& result)
{
    // We don't update :hover/:active state when the result is marked as readOnly.
    if (request.readOnly())
        return;

    Node* innerNodeInDocument = result.innerNode();
    ASSERT(!innerNodeInDocument || innerNodeInDocument->document() == this);

    Node* oldActiveNode = activeNode();
    if (oldActiveNode && !request.active()) {
        // We are clearing the :active chain because the mouse has been released.
        for (RenderObject* curr = oldActiveNode->renderer(); curr; curr = curr->parent()) {
            if (curr->node() && !curr->isText()) {
                curr->node()->setActive(false);
                curr->node()->clearInActiveChain();
            }
        }
        setActiveNode(0);
    } else {
        Node* newActiveNode = innerNodeInDocument;
        if (!oldActiveNode && newActiveNode && request.active() && !request.touchMove()) {
            // We are setting the :active chain and freezing it. If future moves happen, they
            // will need to reference this chain.
            for (RenderObject* curr = newActiveNode->renderer(); curr; curr = curr->parent()) {
                if (curr->node() && !curr->isText())
                    curr->node()->setInActiveChain();
            }
            setActiveNode(newActiveNode);
        }
    }
    // If the mouse has just been pressed, set :active on the chain. Those (and only those)
    // nodes should remain :active until the mouse is released.
    bool allowActiveChanges = !oldActiveNode && activeNode();

    // If the mouse is down and if this is a mouse move event, we want to restrict changes in
    // :hover/:active to only apply to elements that are in the :active chain that we froze
    // at the time the mouse went down.
    bool mustBeInActiveChain = request.active() && request.move();

    RefPtr<Node> oldHoverNode = hoverNode();
    // Clear the :hover chain when the touch gesture is over.
    if (request.touchRelease()) {
        if (oldHoverNode) {
            for (RenderObject* curr = oldHoverNode->renderer(); curr; curr = curr->hoverAncestor()) {
                if (curr->node() && !curr->isText())
                    curr->node()->setHovered(false);
            }
            setHoverNode(0);
        }
        // A touch release can not set new hover or active target.
        return;
    }

    // Check to see if the hovered node has changed.
    // If it hasn't, we do not need to do anything.
    Node* newHoverNode = innerNodeInDocument;
    while (newHoverNode && !newHoverNode->renderer())
        newHoverNode = newHoverNode->parentOrHostNode();

    // Update our current hover node.
    setHoverNode(newHoverNode);

    // We have two different objects. Fetch their renderers.
    RenderObject* oldHoverObj = oldHoverNode ? oldHoverNode->renderer() : 0;
    RenderObject* newHoverObj = newHoverNode ? newHoverNode->renderer() : 0;

    // Locate the common ancestor render object for the two renderers.
    RenderObject* ancestor = nearestCommonHoverAncestor(oldHoverObj, newHoverObj);

    Vector<RefPtr<Node>, 32> nodesToRemoveFromChain;
    Vector<RefPtr<Node>, 32> nodesToAddToChain;

    if (oldHoverObj != newHoverObj) {
        // The old hover path only needs to be cleared up to (and not including) the common ancestor;
        for (RenderObject* curr = oldHoverObj; curr && curr != ancestor; curr = curr->hoverAncestor()) {
            if (curr->node() && !curr->isText() && (!mustBeInActiveChain || curr->node()->inActiveChain()))
                nodesToRemoveFromChain.append(curr->node());
        }
    }

    // Now set the hover state for our new object up to the root.
    for (RenderObject* curr = newHoverObj; curr; curr = curr->hoverAncestor()) {
        if (curr->node() && !curr->isText() && (!mustBeInActiveChain || curr->node()->inActiveChain()))
            nodesToAddToChain.append(curr->node());
    }

    size_t removeCount = nodesToRemoveFromChain.size();
    for (size_t i = 0; i < removeCount; ++i)
        nodesToRemoveFromChain[i]->setHovered(false);

    size_t addCount = nodesToAddToChain.size();
    for (size_t i = 0; i < addCount; ++i) {
        if (allowActiveChanges)
            nodesToAddToChain[i]->setActive(true);
        nodesToAddToChain[i]->setHovered(true);
    }
}

void Document::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::DOM);
    info.addMember(m_styleResolver);
    ContainerNode::reportMemoryUsage(memoryObjectInfo);
    info.addVector(m_customFonts);
    info.addMember(m_url);
    info.addMember(m_baseURL);
    info.addMember(m_baseURLOverride);
    info.addMember(m_baseElementURL);
    info.addMember(m_cookieURL);
    info.addMember(m_firstPartyForCookies);
    info.addMember(m_documentURI);
    info.addMember(m_baseTarget);
    info.addMember(m_frame);
    info.addMember(m_cachedResourceLoader);
    info.addMember(m_elemSheet);
    info.addMember(m_pageUserSheet);
    if (m_pageGroupUserSheets)
        info.addInstrumentedVectorPtr(m_pageGroupUserSheets);
    if (m_userSheets)
        info.addInstrumentedVectorPtr(m_userSheets);
    info.addHashSet(m_nodeIterators);
    info.addHashSet(m_ranges);
    info.addListHashSet(m_styleSheetCandidateNodes);
    info.addMember(m_preferredStylesheetSet);
    info.addMember(m_selectedStylesheetSet);
    info.addMember(m_title.string());
    info.addMember(m_rawTitle.string());
    info.addMember(m_xmlEncoding);
    info.addMember(m_xmlVersion);
    info.addMember(m_contentLanguage);
    info.addHashMap(m_documentNamedItemCollections);
    info.addHashMap(m_windowNamedItemCollections);
#if ENABLE(DASHBOARD_SUPPORT)
    info.addVector(m_dashboardRegions);
#endif
    info.addHashMap(m_cssCanvasElements);
    info.addVector(m_iconURLs);
    info.addHashSet(m_documentSuspensionCallbackElements);
    info.addHashSet(m_mediaVolumeCallbackElements);
    info.addHashSet(m_privateBrowsingStateChangedElements);
    info.addHashMap(m_elementsByAccessKey);
    info.addMember(m_eventQueue);
    info.addHashSet(m_mediaCanStartListeners);
    info.addVector(m_pendingTasks);
}

#if ENABLE(UNDO_MANAGER)
PassRefPtr<UndoManager> Document::undoManager()
{
    if (!m_undoManager)
        m_undoManager = UndoManager::create(this);
    return m_undoManager;
}
#endif

class ImmutableAttributeDataCacheKey {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ImmutableAttributeDataCacheKey()
        : m_localName(0)
        , m_attributes(0)
        , m_attributeCount(0)
    { }

    ImmutableAttributeDataCacheKey(const AtomicString& localName, const Attribute* attributes, unsigned attributeCount)
        : m_localName(localName.impl())
        , m_attributes(attributes)
        , m_attributeCount(attributeCount)
    { }

    bool operator!=(const ImmutableAttributeDataCacheKey& other) const
    {
        if (m_localName != other.m_localName)
            return true;
        if (m_attributeCount != other.m_attributeCount)
            return true;
        return memcmp(m_attributes, other.m_attributes, sizeof(Attribute) * m_attributeCount);
    }

    unsigned hash() const
    {
        unsigned attributeHash = StringHasher::hashMemory(m_attributes, m_attributeCount * sizeof(Attribute));
        return WTF::pairIntHash(m_localName->existingHash(), attributeHash);
    }

private:
    AtomicStringImpl* m_localName;
    const Attribute* m_attributes;
    unsigned m_attributeCount;
};

struct ImmutableAttributeDataCacheEntry {
    ImmutableAttributeDataCacheKey key;
    RefPtr<ElementAttributeData> value;
};

PassRefPtr<ElementAttributeData> Document::cachedImmutableAttributeData(const Element* element, const Vector<Attribute>& attributes)
{
    ASSERT(!attributes.isEmpty());

    ImmutableAttributeDataCacheKey cacheKey(element->localName(), attributes.data(), attributes.size());
    unsigned cacheHash = cacheKey.hash();

    ImmutableAttributeDataCache::iterator cacheIterator = m_immutableAttributeDataCache.add(cacheHash, nullptr).iterator;
    if (cacheIterator->second && cacheIterator->second->key != cacheKey)
        cacheHash = 0;

    RefPtr<ElementAttributeData> attributeData;
    if (cacheHash && cacheIterator->second)
        attributeData = cacheIterator->second->value;
    else
        attributeData = ElementAttributeData::createImmutable(attributes);

    if (!cacheHash || cacheIterator->second)
        return attributeData.release();

    OwnPtr<ImmutableAttributeDataCacheEntry> newEntry = adoptPtr(new ImmutableAttributeDataCacheEntry);
    newEntry->key = ImmutableAttributeDataCacheKey(element->localName(), const_cast<const ElementAttributeData*>(attributeData.get())->attributeItem(0), attributeData->length());
    newEntry->value = attributeData;

    cacheIterator->second = newEntry.release();

    return attributeData.release();
}

Localizer& Document::getLocalizer(const AtomicString& locale)
{
    AtomicString localeKey = locale;
    if (locale.isEmpty() || !RuntimeEnabledFeatures::langAttributeAwareFormControlUIEnabled())
        localeKey = defaultLanguage();
    LocaleToLocalizerMap::AddResult result = m_localizerCache.add(localeKey, nullptr);
    if (result.isNewEntry)
        result.iterator->second = Localizer::create(localeKey);
    return *(result.iterator->second);
}

} // namespace WebCore
