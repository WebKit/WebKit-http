/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "StyleResolver.h"

#include "Attribute.h"
#include "CSSBorderImage.h"
#include "CSSCalculationValue.h"
#include "CSSCursorImageValue.h"
#include "CSSFontFaceRule.h"
#include "CSSFontSelector.h"
#include "CSSImportRule.h"
#include "CSSLineBoxContainValue.h"
#include "CSSMediaRule.h"
#include "CSSPageRule.h"
#include "CSSParser.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSPropertyNames.h"
#include "CSSReflectValue.h"
#include "CSSRuleList.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "CSSTimingFunctionValue.h"
#include "CSSValueList.h"
#if ENABLE(CSS_VARIABLES)
#include "CSSVariableValue.h"
#endif
#include "CachedImage.h"
#include "CalculationValue.h"
#include "ContentData.h"
#include "ContextFeatures.h"
#include "Counter.h"
#include "CounterContent.h"
#include "CursorList.h"
#include "DocumentStyleSheetCollection.h"
#include "ElementShadow.h"
#include "FontFeatureValue.h"
#include "FontValue.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "FrameView.h"
#include "HTMLDocument.h"
#include "HTMLIFrameElement.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLOptionElement.h"
#include "HTMLProgressElement.h"
#include "HTMLStyleElement.h"
#include "HTMLTextAreaElement.h"
#include "InsertionPoint.h"
#include "InspectorInstrumentation.h"
#include "KeyframeList.h"
#include "LinkHash.h"
#include "LocaleToScriptMapping.h"
#include "MathMLNames.h"
#include "Matrix3DTransformOperation.h"
#include "MatrixTransformOperation.h"
#include "MediaList.h"
#include "MediaQueryEvaluator.h"
#include "NodeRenderStyle.h"
#include "NodeRenderingContext.h"
#include "Page.h"
#include "PageGroup.h"
#include "Pair.h"
#include "PerspectiveTransformOperation.h"
#include "QuotesData.h"
#include "Rect.h"
#include "RenderRegion.h"
#include "RenderScrollbar.h"
#include "RenderScrollbarTheme.h"
#include "RenderStyleConstants.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RotateTransformOperation.h"
#include "RuleSet.h"
#include "SVGDocumentExtensions.h"
#include "SVGFontFaceElement.h"
#include "ScaleTransformOperation.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "ShadowData.h"
#include "ShadowRoot.h"
#include "ShadowValue.h"
#include "SiblingTraversalStrategies.h"
#include "SkewTransformOperation.h"
#include "StyleBuilder.h"
#include "StyleCachedImage.h"
#include "StyleGeneratedImage.h"
#include "StylePendingImage.h"
#include "StyleRule.h"
#include "StyleRuleImport.h"
#include "StyleSheetContents.h"
#include "StyleSheetList.h"
#include "Text.h"
#include "TransformationMatrix.h"
#include "TranslateTransformOperation.h"
#include "UserAgentStyleSheets.h"
#include "ViewportStyleResolver.h"
#include "WebCoreMemoryInstrumentation.h"
#include "WebKitCSSKeyframeRule.h"
#include "WebKitCSSKeyframesRule.h"
#include "WebKitCSSRegionRule.h"
#include "WebKitCSSTransformValue.h"
#include "WebKitFontFamilyNames.h"
#include "XMLNames.h"
#include <wtf/MemoryInstrumentationHashMap.h>
#include <wtf/MemoryInstrumentationHashSet.h>
#include <wtf/MemoryInstrumentationVector.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

#if ENABLE(CSS_FILTERS)
#include "FilterOperation.h"
#include "WebKitCSSFilterValue.h"
#endif

#if ENABLE(DASHBOARD_SUPPORT)
#include "DashboardRegion.h"
#endif

#if ENABLE(SVG)
#include "CachedSVGDocument.h"
#include "CachedSVGDocumentReference.h"
#include "SVGDocument.h"
#include "SVGElement.h"
#include "SVGNames.h"
#include "SVGURIReference.h"
#include "WebKitCSSSVGDocumentValue.h"
#endif

#if ENABLE(CSS_SHADERS)
#include "CustomFilterArrayParameter.h"
#include "CustomFilterConstants.h"
#include "CustomFilterNumberParameter.h"
#include "CustomFilterOperation.h"
#include "CustomFilterParameter.h"
#include "CustomFilterTransformParameter.h"
#include "StyleCachedShader.h"
#include "StyleCustomFilterProgram.h"
#include "StylePendingShader.h"
#include "StyleShader.h"
#include "WebKitCSSMixFunctionValue.h"
#include "WebKitCSSShaderValue.h"
#endif

#if ENABLE(CSS_IMAGE_SET)
#include "CSSImageSetValue.h"
#include "StyleCachedImageSet.h"
#endif

using namespace std;

namespace WTF {

template<> struct SequenceMemoryInstrumentationTraits<const WebCore::RuleData*> {
    template <typename I> static void reportMemoryUsage(I, I, MemoryClassInfo&) { }
};

}

namespace WebCore {

using namespace HTMLNames;

#define HANDLE_INHERIT(prop, Prop) \
if (isInherit) { \
    m_style->set##Prop(m_parentStyle->prop()); \
    return; \
}

#define HANDLE_INHERIT_AND_INITIAL(prop, Prop) \
HANDLE_INHERIT(prop, Prop) \
if (isInitial) { \
    m_style->set##Prop(RenderStyle::initial##Prop()); \
    return; \
}

static RuleSet* defaultStyle;
static RuleSet* defaultQuirksStyle;
static RuleSet* defaultPrintStyle;
static RuleSet* defaultViewSourceStyle;
static StyleSheetContents* simpleDefaultStyleSheet;
static StyleSheetContents* defaultStyleSheet;
static StyleSheetContents* quirksStyleSheet;
static StyleSheetContents* svgStyleSheet;
static StyleSheetContents* mathMLStyleSheet;
static StyleSheetContents* mediaControlsStyleSheet;
static StyleSheetContents* fullscreenStyleSheet;

RenderStyle* StyleResolver::s_styleNotYetAvailable;

static void loadFullDefaultStyle();
static void loadSimpleDefaultStyle();
template <class ListType>
static void collectCSSOMWrappers(HashMap<StyleRule*, RefPtr<CSSStyleRule> >&, ListType*);

// FIXME: It would be nice to use some mechanism that guarantees this is in sync with the real UA stylesheet.
static const char* simpleUserAgentStyleSheet = "html,body,div{display:block}head{display:none}body{margin:8px}div:focus,span:focus{outline:auto 5px -webkit-focus-ring-color}a:-webkit-any-link{color:-webkit-link;text-decoration:underline}a:-webkit-any-link:active{color:-webkit-activelink}";

static inline bool elementCanUseSimpleDefaultStyle(Element* e)
{
    return e->hasTagName(htmlTag) || e->hasTagName(headTag) || e->hasTagName(bodyTag) || e->hasTagName(divTag) || e->hasTagName(spanTag) || e->hasTagName(brTag) || e->hasTagName(aTag);
}

static const MediaQueryEvaluator& screenEval()
{
    DEFINE_STATIC_LOCAL(const MediaQueryEvaluator, staticScreenEval, ("screen"));
    return staticScreenEval;
}

static const MediaQueryEvaluator& printEval()
{
    DEFINE_STATIC_LOCAL(const MediaQueryEvaluator, staticPrintEval, ("print"));
    return staticPrintEval;
}

static StylePropertySet* leftToRightDeclaration()
{
    DEFINE_STATIC_LOCAL(RefPtr<StylePropertySet>, leftToRightDecl, (StylePropertySet::create()));
    if (leftToRightDecl->isEmpty())
        leftToRightDecl->setProperty(CSSPropertyDirection, CSSValueLtr);
    return leftToRightDecl.get();
}

static StylePropertySet* rightToLeftDeclaration()
{
    DEFINE_STATIC_LOCAL(RefPtr<StylePropertySet>, rightToLeftDecl, (StylePropertySet::create()));
    if (rightToLeftDecl->isEmpty())
        rightToLeftDecl->setProperty(CSSPropertyDirection, CSSValueRtl);
    return rightToLeftDecl.get();
}

StyleResolver::StyleResolver(Document* document, bool matchAuthorAndUserStyles)
    : m_hasUAAppearance(false)
    , m_backgroundData(BackgroundFillLayer)
    , m_matchedPropertiesCacheAdditionsSinceLastSweep(0)
    , m_matchedPropertiesCacheSweepTimer(this, &StyleResolver::sweepMatchedPropertiesCache)
    , m_checker(document, !document->inQuirksMode())
    , m_parentStyle(0)
    , m_rootElementStyle(0)
    , m_element(0)
    , m_styledElement(0)
    , m_regionForStyling(0)
    , m_elementLinkState(NotInsideLink)
    , m_parentNode(0)
    , m_lineHeightValue(0)
    , m_fontDirty(false)
    , m_matchAuthorAndUserStyles(matchAuthorAndUserStyles)
    , m_sameOriginOnly(false)
    , m_distributedToInsertionPoint(false)
    , m_fontSelector(CSSFontSelector::create(document))
#if ENABLE(CSS_DEVICE_ADAPTATION)
    , m_viewportStyleResolver(ViewportStyleResolver::create(document))
#endif
    , m_applyPropertyToRegularStyle(true)
    , m_applyPropertyToVisitedLinkStyle(false)
    , m_styleBuilder(StyleBuilder::sharedStyleBuilder())
#if ENABLE(CSS_SHADERS)
    , m_hasPendingShaders(false)
#endif
    , m_styleMap(this)
{
    Element* root = document->documentElement();

    if (!defaultStyle) {
        if (!root || elementCanUseSimpleDefaultStyle(root))
            loadSimpleDefaultStyle();
        else
            loadFullDefaultStyle();
    }

    FrameView* view = document->view();
    m_medium = adoptPtr(new MediaQueryEvaluator(view ? view->mediaType() : "all"));

    if (root)
        m_rootDefaultStyle = styleForElement(root, 0, DisallowStyleSharing, MatchOnlyUserAgentRules);

    if (m_rootDefaultStyle && view)
        m_medium = adoptPtr(new MediaQueryEvaluator(view->mediaType(), view->frame(), m_rootDefaultStyle.get()));

    resetAuthorStyle();

#if ENABLE(SVG_FONTS)
    if (document->svgExtensions()) {
        const HashSet<SVGFontFaceElement*>& svgFontFaceElements = document->svgExtensions()->svgFontFaceElements();
        HashSet<SVGFontFaceElement*>::const_iterator end = svgFontFaceElements.end();
        for (HashSet<SVGFontFaceElement*>::const_iterator it = svgFontFaceElements.begin(); it != end; ++it)
            fontSelector()->addFontFaceRule((*it)->fontFaceRule());
    }
#endif

    DocumentStyleSheetCollection* styleSheetCollection = document->styleSheetCollection();
    collectRulesFromUserStyleSheets(styleSheetCollection->activeUserStyleSheets());
    appendAuthorStyleSheets(0, styleSheetCollection->activeAuthorStyleSheets());
}

void StyleResolver::collectRulesFromUserStyleSheets(const Vector<RefPtr<CSSStyleSheet> >& userSheets)
{
    OwnPtr<RuleSet> userStyleRuleSet = RuleSet::create();
    for (unsigned i = 0; i < userSheets.size(); ++i) {
        ASSERT(userSheets[i]->contents()->isUserStyleSheet());
        userStyleRuleSet->addRulesFromSheet(userSheets[i]->contents(), *m_medium, this);
    }
    if (userStyleRuleSet->m_ruleCount > 0 || userStyleRuleSet->m_pageRules.size() > 0)
        m_userStyle = userStyleRuleSet.release();
}

static PassOwnPtr<RuleSet> makeRuleSet(const Vector<RuleFeature>& rules)
{
    size_t size = rules.size();
    if (!size)
        return nullptr;
    OwnPtr<RuleSet> ruleSet = RuleSet::create();
    for (size_t i = 0; i < size; ++i)
        ruleSet->addRule(rules[i].rule, rules[i].selectorIndex, rules[i].hasDocumentSecurityOrigin ? RuleHasDocumentSecurityOrigin : RuleHasNoSpecialState);
    ruleSet->shrinkToFit();
    return ruleSet.release();
}

void StyleResolver::collectFeatures()
{
    m_features.clear();
    // Collect all ids and rules using sibling selectors (:first-child and similar)
    // in the current set of stylesheets. Style sharing code uses this information to reject
    // sharing candidates.
    m_features.add(defaultStyle->features());
    m_features.add(m_authorStyle->features());
    if (m_scopeResolver)
        m_scopeResolver->collectFeaturesTo(m_features);
    if (m_userStyle)
        m_features.add(m_userStyle->features());

    m_siblingRuleSet = makeRuleSet(m_features.siblingRules);
    m_uncommonAttributeRuleSet = makeRuleSet(m_features.uncommonAttributeRules);
}

void StyleResolver::resetAuthorStyle()
{
    m_authorStyle = RuleSet::create();
    m_authorStyle->disableAutoShrinkToFit();
}

void StyleResolver::appendAuthorStyleSheets(unsigned firstNew, const Vector<RefPtr<CSSStyleSheet> >& styleSheets)
{
    // This handles sheets added to the end of the stylesheet list only. In other cases the style resolver
    // needs to be reconstructed. To handle insertions too the rule order numbers would need to be updated.
    unsigned size = styleSheets.size();
    for (unsigned i = firstNew; i < size; ++i) {
        CSSStyleSheet* cssSheet = styleSheets[i].get();
        ASSERT(!cssSheet->disabled());
        if (cssSheet->mediaQueries() && !m_medium->eval(cssSheet->mediaQueries(), this))
            continue;
        StyleSheetContents* sheet = cssSheet->contents();
#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)
        if (const ContainerNode* scope = StyleScopeResolver::scopeFor(cssSheet)) {
            ensureScopeResolver()->ensureRuleSetFor(scope)->addRulesFromSheet(sheet, *m_medium, this, scope);
            continue;
        }
#endif

        m_authorStyle->addRulesFromSheet(sheet, *m_medium, this);
        if (!m_styleRuleToCSSOMWrapperMap.isEmpty())
            collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, cssSheet);
    }
    m_authorStyle->shrinkToFit();
    collectFeatures();
    
    if (document()->renderer() && document()->renderer()->style())
        document()->renderer()->style()->font().update(fontSelector());

#if ENABLE(CSS_DEVICE_ADAPTATION)
    viewportStyleResolver()->resolve();
#endif
}

void StyleResolver::pushParentElement(Element* parent)
{
    const ContainerNode* parentsParent = parent->parentOrHostElement();

    // We are not always invoked consistently. For example, script execution can cause us to enter
    // style recalc in the middle of tree building. We may also be invoked from somewhere within the tree.
    // Reset the stack in this case, or if we see a new root element.
    // Otherwise just push the new parent.
    if (!parentsParent || m_checker.parentStackIsEmpty())
        m_checker.setupParentStack(parent);
    else
        m_checker.pushParent(parent);

    // Note: We mustn't skip ShadowRoot nodes for the scope stack.
    if (m_scopeResolver)
        m_scopeResolver->push(parent, parent->parentOrHostNode());
}

void StyleResolver::popParentElement(Element* parent)
{
    // Note that we may get invoked for some random elements in some wacky cases during style resolve.
    // Pause maintaining the stack in this case.
    if (m_checker.parentStackIsConsistent(parent))
        m_checker.popParent();
    if (m_scopeResolver)
        m_scopeResolver->pop(parent);
}

void StyleResolver::pushParentShadowRoot(const ShadowRoot* shadowRoot)
{
    ASSERT(shadowRoot->host());
    if (m_scopeResolver)
        m_scopeResolver->push(shadowRoot, shadowRoot->host());
}

void StyleResolver::popParentShadowRoot(const ShadowRoot* shadowRoot)
{
    ASSERT(shadowRoot->host());
    if (m_scopeResolver)
        m_scopeResolver->pop(shadowRoot);
}

// This is a simplified style setting function for keyframe styles
void StyleResolver::addKeyframeStyle(PassRefPtr<StyleRuleKeyframes> rule)
{
    AtomicString s(rule->name());
    m_keyframesRuleMap.set(s.impl(), rule);
}

StyleResolver::~StyleResolver()
{
    m_fontSelector->clearDocument();

#if ENABLE(CSS_DEVICE_ADAPTATION)
    m_viewportStyleResolver->clearDocument();
#endif
}

void StyleResolver::sweepMatchedPropertiesCache(Timer<StyleResolver>*)
{
    // Look for cache entries containing a style declaration with a single ref and remove them.
    // This may happen when an element attribute mutation causes it to generate a new inlineStyle()
    // or presentationAttributeStyle(), potentially leaving this cache with the last ref on the old one.
    Vector<unsigned, 16> toRemove;
    MatchedPropertiesCache::iterator it = m_matchedPropertiesCache.begin();
    MatchedPropertiesCache::iterator end = m_matchedPropertiesCache.end();
    for (; it != end; ++it) {
        Vector<MatchedProperties>& matchedProperties = it->value.matchedProperties;
        for (size_t i = 0; i < matchedProperties.size(); ++i) {
            if (matchedProperties[i].properties->hasOneRef()) {
                toRemove.append(it->key);
                break;
            }
        }
    }
    for (size_t i = 0; i < toRemove.size(); ++i)
        m_matchedPropertiesCache.remove(toRemove[i]);

    m_matchedPropertiesCacheAdditionsSinceLastSweep = 0;
}

static StyleSheetContents* parseUASheet(const String& str)
{
    StyleSheetContents* sheet = StyleSheetContents::create().leakRef(); // leak the sheet on purpose
    sheet->parseString(str);
    return sheet;
}

static StyleSheetContents* parseUASheet(const char* characters, unsigned size)
{
    return parseUASheet(String(characters, size));
}

static void loadFullDefaultStyle()
{
    if (simpleDefaultStyleSheet) {
        ASSERT(defaultStyle);
        ASSERT(defaultPrintStyle == defaultStyle);
        delete defaultStyle;
        simpleDefaultStyleSheet->deref();
        defaultStyle = RuleSet::create().leakPtr();
        defaultPrintStyle = RuleSet::create().leakPtr();
        simpleDefaultStyleSheet = 0;
    } else {
        ASSERT(!defaultStyle);
        defaultStyle = RuleSet::create().leakPtr();
        defaultPrintStyle = RuleSet::create().leakPtr();
        defaultQuirksStyle = RuleSet::create().leakPtr();
    }

    // Strict-mode rules.
    String defaultRules = String(htmlUserAgentStyleSheet, sizeof(htmlUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraDefaultStyleSheet();
    defaultStyleSheet = parseUASheet(defaultRules);
    defaultStyle->addRulesFromSheet(defaultStyleSheet, screenEval());
    defaultPrintStyle->addRulesFromSheet(defaultStyleSheet, printEval());

    // Quirks-mode rules.
    String quirksRules = String(quirksUserAgentStyleSheet, sizeof(quirksUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraQuirksStyleSheet();
    quirksStyleSheet = parseUASheet(quirksRules);
    defaultQuirksStyle->addRulesFromSheet(quirksStyleSheet, screenEval());
}

static void loadSimpleDefaultStyle()
{
    ASSERT(!defaultStyle);
    ASSERT(!simpleDefaultStyleSheet);

    defaultStyle = RuleSet::create().leakPtr();
    // There are no media-specific rules in the simple default style.
    defaultPrintStyle = defaultStyle;
    defaultQuirksStyle = RuleSet::create().leakPtr();

    simpleDefaultStyleSheet = parseUASheet(simpleUserAgentStyleSheet, strlen(simpleUserAgentStyleSheet));
    defaultStyle->addRulesFromSheet(simpleDefaultStyleSheet, screenEval());

    // No need to initialize quirks sheet yet as there are no quirk rules for elements allowed in simple default style.
}

static void loadViewSourceStyle()
{
    ASSERT(!defaultViewSourceStyle);
    defaultViewSourceStyle = RuleSet::create().leakPtr();
    defaultViewSourceStyle->addRulesFromSheet(parseUASheet(sourceUserAgentStyleSheet, sizeof(sourceUserAgentStyleSheet)), screenEval());
}

static void ensureDefaultStyleSheetsForElement(Element* element)
{
    if (simpleDefaultStyleSheet && !elementCanUseSimpleDefaultStyle(element))
        loadFullDefaultStyle();

#if ENABLE(SVG)
    if (element->isSVGElement() && !svgStyleSheet) {
        // SVG rules.
        svgStyleSheet = parseUASheet(svgUserAgentStyleSheet, sizeof(svgUserAgentStyleSheet));
        defaultStyle->addRulesFromSheet(svgStyleSheet, screenEval());
        defaultPrintStyle->addRulesFromSheet(svgStyleSheet, printEval());
    }
#endif

#if ENABLE(MATHML)
    if (element->isMathMLElement() && !mathMLStyleSheet) {
        // MathML rules.
        mathMLStyleSheet = parseUASheet(mathmlUserAgentStyleSheet, sizeof(mathmlUserAgentStyleSheet));
        defaultStyle->addRulesFromSheet(mathMLStyleSheet, screenEval());
        defaultPrintStyle->addRulesFromSheet(mathMLStyleSheet, printEval());
    }
#endif

#if ENABLE(VIDEO)
    if (!mediaControlsStyleSheet && (element->hasTagName(videoTag) || element->hasTagName(audioTag))) {
        String mediaRules = String(mediaControlsUserAgentStyleSheet, sizeof(mediaControlsUserAgentStyleSheet)) + RenderTheme::themeForPage(element->document()->page())->extraMediaControlsStyleSheet();
        mediaControlsStyleSheet = parseUASheet(mediaRules);
        defaultStyle->addRulesFromSheet(mediaControlsStyleSheet, screenEval());
        defaultPrintStyle->addRulesFromSheet(mediaControlsStyleSheet, printEval());
    }
#endif

#if ENABLE(FULLSCREEN_API)
    if (!fullscreenStyleSheet && element->document()->webkitIsFullScreen()) {
        String fullscreenRules = String(fullscreenUserAgentStyleSheet, sizeof(fullscreenUserAgentStyleSheet)) + RenderTheme::defaultTheme()->extraFullScreenStyleSheet();
        fullscreenStyleSheet = parseUASheet(fullscreenRules);
        defaultStyle->addRulesFromSheet(fullscreenStyleSheet, screenEval());
        defaultQuirksStyle->addRulesFromSheet(fullscreenStyleSheet, screenEval());
    }
#endif

    ASSERT(defaultStyle->features().idsInRules.isEmpty());
    ASSERT(mathMLStyleSheet || defaultStyle->features().siblingRules.isEmpty());
}

void StyleResolver::addMatchedProperties(MatchResult& matchResult, const StylePropertySet* properties, StyleRule* rule, unsigned linkMatchType, bool inRegionRule)
{
    matchResult.matchedProperties.grow(matchResult.matchedProperties.size() + 1);
    MatchedProperties& newProperties = matchResult.matchedProperties.last();
    newProperties.properties = const_cast<StylePropertySet*>(properties);
    newProperties.linkMatchType = linkMatchType;
    newProperties.isInRegionRule = inRegionRule;
    matchResult.matchedRules.append(rule);
}

inline void StyleResolver::addElementStyleProperties(MatchResult& result, const StylePropertySet* propertySet, bool isCacheable)
{
    if (!propertySet)
        return;
    result.ranges.lastAuthorRule = result.matchedProperties.size();
    if (result.ranges.firstAuthorRule == -1)
        result.ranges.firstAuthorRule = result.ranges.lastAuthorRule;
    addMatchedProperties(result, propertySet);
    if (!isCacheable)
        result.isCacheable = false;
}

class MatchingUARulesScope {
public:
    MatchingUARulesScope();
    ~MatchingUARulesScope();

    static bool isMatchingUARules();

private:
    static bool m_matchingUARules;
};

MatchingUARulesScope::MatchingUARulesScope()
{
    ASSERT(!m_matchingUARules);
    m_matchingUARules = true;
}

MatchingUARulesScope::~MatchingUARulesScope()
{
    m_matchingUARules = false;
}

inline bool MatchingUARulesScope::isMatchingUARules()
{
    return m_matchingUARules;
}

bool MatchingUARulesScope::m_matchingUARules = false;

void StyleResolver::collectMatchingRules(RuleSet* rules, int& firstRuleIndex, int& lastRuleIndex, const MatchOptions& options)
{
    ASSERT(rules);
    ASSERT(m_element);

    const AtomicString& pseudoId = m_element->shadowPseudoId();
    if (!pseudoId.isEmpty()) {
        ASSERT(m_styledElement);
        collectMatchingRulesForList(rules->shadowPseudoElementRules(pseudoId.impl()), firstRuleIndex, lastRuleIndex, options);
    }

    // Check whether other types of rules are applicable in the current tree scope. Criteria for this:
    // a) it's a UA rule
    // b) the tree scope allows author rules
    // c) the rules comes from a scoped style sheet within the same tree scope
    TreeScope* treeScope = m_element->treeScope();
    if (!MatchingUARulesScope::isMatchingUARules()
        && !treeScope->applyAuthorStyles()
        && (!options.scope || options.scope->treeScope() != treeScope))
        return;

    // We need to collect the rules for id, class, tag, and everything else into a buffer and
    // then sort the buffer.
    if (m_element->hasID())
        collectMatchingRulesForList(rules->idRules(m_element->idForStyleResolution().impl()), firstRuleIndex, lastRuleIndex, options);
    if (m_styledElement && m_styledElement->hasClass()) {
        for (size_t i = 0; i < m_styledElement->classNames().size(); ++i)
            collectMatchingRulesForList(rules->classRules(m_styledElement->classNames()[i].impl()), firstRuleIndex, lastRuleIndex, options);
    }

    if (m_element->isLink())
        collectMatchingRulesForList(rules->linkPseudoClassRules(), firstRuleIndex, lastRuleIndex, options);
    if (m_checker.matchesFocusPseudoClass(m_element))
        collectMatchingRulesForList(rules->focusPseudoClassRules(), firstRuleIndex, lastRuleIndex, options);
    collectMatchingRulesForList(rules->tagRules(m_element->localName().impl()), firstRuleIndex, lastRuleIndex, options);
    collectMatchingRulesForList(rules->universalRules(), firstRuleIndex, lastRuleIndex, options);
}

void StyleResolver::collectMatchingRulesForRegion(RuleSet* rules, int& firstRuleIndex, int& lastRuleIndex, const MatchOptions& options)
{
    if (!m_regionForStyling)
        return;

    unsigned size = rules->m_regionSelectorsAndRuleSets.size();
    for (unsigned i = 0; i < size; ++i) {
        CSSSelector* regionSelector = rules->m_regionSelectorsAndRuleSets.at(i).selector;
        if (checkRegionSelector(regionSelector, static_cast<Element*>(m_regionForStyling->node()))) {
            RuleSet* regionRules = rules->m_regionSelectorsAndRuleSets.at(i).ruleSet.get();
            ASSERT(regionRules);
            collectMatchingRules(regionRules, firstRuleIndex, lastRuleIndex, options);
        }
    }
}

void StyleResolver::sortAndTransferMatchedRules(MatchResult& result)
{
    if (m_matchedRules.isEmpty())
        return;

    sortMatchedRules();

    if (m_checker.mode() == SelectorChecker::CollectingRules) {
        if (!m_ruleList)
            m_ruleList = StaticCSSRuleList::create();
        for (unsigned i = 0; i < m_matchedRules.size(); ++i)
            m_ruleList->rules().append(m_matchedRules[i]->rule()->createCSSOMWrapper());
        return;
    }

    // Now transfer the set of matched rules over to our list of declarations.
    // FIXME: This sucks, the inspector should get the style from the visited style itself.
    bool swapVisitedUnvisited = InspectorInstrumentation::forcePseudoState(m_element, CSSSelector::PseudoVisited);
    for (unsigned i = 0; i < m_matchedRules.size(); i++) {
        if (m_style && m_matchedRules[i]->containsUncommonAttributeSelector())
            m_style->setUnique();
        unsigned linkMatchType = m_matchedRules[i]->linkMatchType();
        if (swapVisitedUnvisited && linkMatchType && linkMatchType != SelectorChecker::MatchAll)
            linkMatchType = (linkMatchType == SelectorChecker::MatchVisited) ? SelectorChecker::MatchLink : SelectorChecker::MatchVisited;
        addMatchedProperties(result, m_matchedRules[i]->rule()->properties(), m_matchedRules[i]->rule(), linkMatchType, m_matchedRules[i]->isInRegionRule());
    }
}

void StyleResolver::matchScopedAuthorRules(MatchResult& result, bool includeEmptyRules)
{
#if ENABLE(STYLE_SCOPED) || ENABLE(SHADOW_DOM)
    if (!m_scopeResolver)
        return;

    matchHostRules(result, includeEmptyRules);

    if (!m_scopeResolver->hasScopedStyles())
        return;

    // Match scoped author rules by traversing the scoped element stack (rebuild it if it got inconsistent).
    if (!m_scopeResolver->ensureStackConsistency(m_element))
        return;

    bool applyAuthorStyles = m_element->treeScope()->applyAuthorStyles();
    bool documentScope = true;
    unsigned scopeSize = m_scopeResolver->stackSize();
    for (unsigned i = 0; i < scopeSize; ++i) {
        const StyleScopeResolver::StackFrame& frame = m_scopeResolver->stackFrameAt(i);
        documentScope = documentScope && !frame.m_scope->isInShadowTree();
        if (documentScope) {
            if (!applyAuthorStyles)
                continue;
        } else {
            if (!m_scopeResolver->matchesStyleBounds(frame))
                continue;
        }
           
        MatchOptions options(includeEmptyRules, frame.m_scope);
        collectMatchingRules(frame.m_ruleSet, result.ranges.firstAuthorRule, result.ranges.lastAuthorRule, options);
        collectMatchingRulesForRegion(frame.m_ruleSet, result.ranges.firstAuthorRule, result.ranges.lastAuthorRule, options);
    }

#else
    UNUSED_PARAM(result);
    UNUSED_PARAM(includeEmptyRules);
#endif
}

inline bool StyleResolver::styleSharingCandidateMatchesHostRules()
{
#if ENABLE(SHADOW_DOM)
    return m_scopeResolver && m_scopeResolver->styleSharingCandidateMatchesHostRules(m_element);
#else
    return false;
#endif
}

void StyleResolver::matchHostRules(MatchResult& result, bool includeEmptyRules)
{
#if ENABLE(SHADOW_DOM)
    ASSERT(m_scopeResolver);

    Vector<RuleSet*> matchedRules;
    m_scopeResolver->matchHostRules(m_element, matchedRules);
    if (matchedRules.isEmpty())
        return;

    MatchOptions options(includeEmptyRules);
    options.scope = m_element;
    for (unsigned i = matchedRules.size(); i > 0; --i)
        collectMatchingRules(matchedRules.at(i-1), result.ranges.firstAuthorRule, result.ranges.lastAuthorRule, options);
#else
    UNUSED_PARAM(result);
    UNUSED_PARAM(includeEmptyRules);
#endif
}

void StyleResolver::matchAuthorRules(MatchResult& result, bool includeEmptyRules)
{
    m_matchedRules.clear();
    result.ranges.lastAuthorRule = result.matchedProperties.size() - 1;

    if (!m_element)
        return;

    // Match global author rules.
    MatchOptions options(includeEmptyRules);
    collectMatchingRules(m_authorStyle.get(), result.ranges.firstAuthorRule, result.ranges.lastAuthorRule, options);
    collectMatchingRulesForRegion(m_authorStyle.get(), result.ranges.firstAuthorRule, result.ranges.lastAuthorRule, options);

    matchScopedAuthorRules(result, includeEmptyRules);

    sortAndTransferMatchedRules(result);
}

void StyleResolver::matchUserRules(MatchResult& result, bool includeEmptyRules)
{
    if (!m_userStyle)
        return;
    
    m_matchedRules.clear();

    result.ranges.lastUserRule = result.matchedProperties.size() - 1;
    collectMatchingRules(m_userStyle.get(), result.ranges.firstUserRule, result.ranges.lastUserRule, includeEmptyRules);
    collectMatchingRulesForRegion(m_userStyle.get(), result.ranges.firstUserRule, result.ranges.lastUserRule, includeEmptyRules);

    sortAndTransferMatchedRules(result);
}

void StyleResolver::matchUARules(MatchResult& result, RuleSet* rules)
{
    m_matchedRules.clear();
    
    result.ranges.lastUARule = result.matchedProperties.size() - 1;
    collectMatchingRules(rules, result.ranges.firstUARule, result.ranges.lastUARule, false);

    sortAndTransferMatchedRules(result);
}

void StyleResolver::collectMatchingRulesForList(const Vector<RuleData>* rules, int& firstRuleIndex, int& lastRuleIndex, const MatchOptions& options)
{
    if (!rules)
        return;

    // In some cases we may end up looking up style for random elements in the middle of a recursive tree resolve.
    // Ancestor identifier filter won't be up-to-date in that case and we can't use the fast path.
    bool canUseFastReject = m_checker.parentStackIsConsistent(m_parentNode);

    unsigned size = rules->size();
    for (unsigned i = 0; i < size; ++i) {
        const RuleData& ruleData = rules->at(i);
        if (canUseFastReject && m_checker.fastRejectSelector<RuleData::maximumIdentifierCount>(ruleData.descendantSelectorIdentifierHashes()))
            continue;

        StyleRule* rule = ruleData.rule();
        InspectorInstrumentationCookie cookie = InspectorInstrumentation::willMatchRule(document(), rule);
        if (checkSelector(ruleData, options.scope)) {
            // If the rule has no properties to apply, then ignore it in the non-debug mode.
            const StylePropertySet* properties = rule->properties();
            if (!properties || (properties->isEmpty() && !options.includeEmptyRules)) {
                InspectorInstrumentation::didMatchRule(cookie, false);
                continue;
            }
            // FIXME: Exposing the non-standard getMatchedCSSRules API to web is the only reason this is needed.
            if (m_sameOriginOnly && !ruleData.hasDocumentSecurityOrigin()) {
                InspectorInstrumentation::didMatchRule(cookie, false);
                continue;
            }
            // If we're matching normal rules, set a pseudo bit if
            // we really just matched a pseudo-element.
            if (m_dynamicPseudo != NOPSEUDO && m_pseudoStyle == NOPSEUDO) {
                if (m_checker.mode() == SelectorChecker::CollectingRules) {
                    InspectorInstrumentation::didMatchRule(cookie, false);
                    continue;
                }
                if (m_dynamicPseudo < FIRST_INTERNAL_PSEUDOID)
                    m_style->setHasPseudoStyle(m_dynamicPseudo);
            } else {
                // Update our first/last rule indices in the matched rules array.
                ++lastRuleIndex;
                if (firstRuleIndex == -1)
                    firstRuleIndex = lastRuleIndex;

                // Add this rule to our list of matched rules.
                addMatchedRule(&ruleData);
                InspectorInstrumentation::didMatchRule(cookie, true);
                continue;
            }
        }
        InspectorInstrumentation::didMatchRule(cookie, false);
    }
}

static inline bool compareRules(const RuleData* r1, const RuleData* r2)
{
    unsigned specificity1 = r1->specificity();
    unsigned specificity2 = r2->specificity();
    return (specificity1 == specificity2) ? r1->position() < r2->position() : specificity1 < specificity2;
}

void StyleResolver::sortMatchedRules()
{
    std::sort(m_matchedRules.begin(), m_matchedRules.end(), compareRules);
}

void StyleResolver::matchAllRules(MatchResult& result, bool includeSMILProperties)
{
    matchUARules(result);

    // Now we check user sheet rules.
    if (m_matchAuthorAndUserStyles)
        matchUserRules(result, false);
        
    // Now check author rules, beginning first with presentational attributes mapped from HTML.
    if (m_styledElement) {
        addElementStyleProperties(result, m_styledElement->presentationAttributeStyle());

        // Now we check additional mapped declarations.
        // Tables and table cells share an additional mapped rule that must be applied
        // after all attributes, since their mapped style depends on the values of multiple attributes.
        addElementStyleProperties(result, m_styledElement->additionalPresentationAttributeStyle());

        if (m_styledElement->isHTMLElement()) {
            bool isAuto;
            TextDirection textDirection = toHTMLElement(m_styledElement)->directionalityIfhasDirAutoAttribute(isAuto);
            if (isAuto)
                addMatchedProperties(result, textDirection == LTR ? leftToRightDeclaration() : rightToLeftDeclaration());
        }
    }
    
    // Check the rules in author sheets next.
    if (m_matchAuthorAndUserStyles)
        matchAuthorRules(result, false);

    // Now check our inline style attribute.
    if (m_matchAuthorAndUserStyles && m_styledElement && m_styledElement->inlineStyle()) {
        // Inline style is immutable as long as there is no CSSOM wrapper.
        // FIXME: Media control shadow trees seem to have problems with caching.
        bool isInlineStyleCacheable = !m_styledElement->inlineStyle()->isMutable() && !m_styledElement->isInShadowTree();
        // FIXME: Constify.
        addElementStyleProperties(result, m_styledElement->inlineStyle(), isInlineStyleCacheable);
    }

#if ENABLE(SVG)
    // Now check SMIL animation override style.
    if (includeSMILProperties && m_matchAuthorAndUserStyles && m_styledElement && m_styledElement->isSVGElement())
        addElementStyleProperties(result, static_cast<SVGElement*>(m_styledElement)->animatedSMILStyleProperties(), false /* isCacheable */);
#else
    UNUSED_PARAM(includeSMILProperties);
#endif
}

inline void StyleResolver::initElement(Element* e)
{
    if (m_element != e) {
        m_element = e;
        m_styledElement = m_element && m_element->isStyledElement() ? static_cast<StyledElement*>(m_element) : 0;
        m_elementLinkState = m_checker.determineLinkState(m_element);
        if (e && e == e->document()->documentElement()) {
            e->document()->setDirectionSetOnDocumentElement(false);
            e->document()->setWritingModeSetOnDocumentElement(false);
        }
    }
}

inline bool shouldResetStyleInheritance(NodeRenderingContext& context)
{
    if (context.resetStyleInheritance())
        return true;

    if (InsertionPoint* insertionPoint = context.insertionPoint())
        return insertionPoint->resetStyleInheritance();

    return false;
}

inline void StyleResolver::initForStyleResolve(Element* e, RenderStyle* parentStyle, PseudoId pseudoID)
{
    m_pseudoStyle = pseudoID;

    if (e) {
        NodeRenderingContext context(e);
        m_parentNode = context.parentNodeForRenderingAndStyle();
        m_parentStyle = shouldResetStyleInheritance(context) ? 0 :
            parentStyle ? parentStyle :
            m_parentNode ? m_parentNode->renderStyle() : 0;
        m_distributedToInsertionPoint = context.insertionPoint();
    } else {
        m_parentNode = 0;
        m_parentStyle = parentStyle;
        m_distributedToInsertionPoint = false;
    }

    Node* docElement = e ? e->document()->documentElement() : 0;
    RenderStyle* docStyle = m_checker.document()->renderStyle();
    m_rootElementStyle = docElement && e != docElement ? docElement->renderStyle() : docStyle;

    m_style = 0;

    m_pendingImageProperties.clear();

    m_ruleList = 0;

    m_fontDirty = false;
}

static const unsigned cStyleSearchThreshold = 10;
static const unsigned cStyleSearchLevelThreshold = 10;

Node* StyleResolver::locateCousinList(Element* parent, unsigned& visitedNodeCount) const
{
    if (visitedNodeCount >= cStyleSearchThreshold * cStyleSearchLevelThreshold)
        return 0;
    if (!parent || !parent->isStyledElement())
        return 0;
    if (parent->hasScopedHTMLStyleChild())
        return 0;
    StyledElement* p = static_cast<StyledElement*>(parent);
    if (p->inlineStyle())
        return 0;
#if ENABLE(SVG)
    if (p->isSVGElement() && static_cast<SVGElement*>(p)->animatedSMILStyleProperties())
        return 0;
#endif
    if (p->hasID() && m_features.idsInRules.contains(p->idForStyleResolution().impl()))
        return 0;

    RenderStyle* parentStyle = p->renderStyle();
    unsigned subcount = 0;
    Node* thisCousin = p;
    Node* currentNode = p->previousSibling();

    // Reserve the tries for this level. This effectively makes sure that the algorithm
    // will never go deeper than cStyleSearchLevelThreshold levels into recursion.
    visitedNodeCount += cStyleSearchThreshold;
    while (thisCousin) {
        while (currentNode) {
            ++subcount;
            if (currentNode->renderStyle() == parentStyle && currentNode->lastChild()) {
                // Adjust for unused reserved tries.
                visitedNodeCount -= cStyleSearchThreshold - subcount;
                return currentNode->lastChild();
            }
            if (subcount >= cStyleSearchThreshold)
                return 0;
            currentNode = currentNode->previousSibling();
        }
        currentNode = locateCousinList(thisCousin->parentElement(), visitedNodeCount);
        thisCousin = currentNode;
    }

    return 0;
}

bool StyleResolver::styleSharingCandidateMatchesRuleSet(RuleSet* ruleSet)
{
    if (!ruleSet)
        return false;
    m_matchedRules.clear();

    int firstRuleIndex = -1, lastRuleIndex = -1;
    m_checker.setMode(SelectorChecker::SharingRules);
    collectMatchingRules(ruleSet, firstRuleIndex, lastRuleIndex, false);
    m_checker.setMode(SelectorChecker::ResolvingStyle);
    if (m_matchedRules.isEmpty())
        return false;
    m_matchedRules.clear();
    return true;
}

bool StyleResolver::canShareStyleWithControl(StyledElement* element) const
{
    HTMLInputElement* thisInputElement = element->toInputElement();
    HTMLInputElement* otherInputElement = m_element->toInputElement();

    if (!thisInputElement || !otherInputElement)
        return false;

    if (thisInputElement->attributeData() != otherInputElement->attributeData()) {
        if (thisInputElement->fastGetAttribute(typeAttr) != otherInputElement->fastGetAttribute(typeAttr))
            return false;
        if (thisInputElement->fastGetAttribute(readonlyAttr) != otherInputElement->fastGetAttribute(readonlyAttr))
            return false;
    }

    if (thisInputElement->isAutofilled() != otherInputElement->isAutofilled())
        return false;
    if (thisInputElement->shouldAppearChecked() != otherInputElement->shouldAppearChecked())
        return false;
    if (thisInputElement->isIndeterminate() != otherInputElement->isIndeterminate())
        return false;
    if (thisInputElement->required() != otherInputElement->required())
        return false;

    if (element->isEnabledFormControl() != m_element->isEnabledFormControl())
        return false;

    if (element->isDefaultButtonForForm() != m_element->isDefaultButtonForForm())
        return false;

    if (m_element->document()->containsValidityStyleRules()) {
        bool willValidate = element->willValidate();

        if (willValidate != m_element->willValidate())
            return false;

        if (willValidate && (element->isValidFormControlElement() != m_element->isValidFormControlElement()))
            return false;

        if (element->isInRange() != m_element->isInRange())
            return false;

        if (element->isOutOfRange() != m_element->isOutOfRange())
            return false;
    }

    return true;
}

static inline bool elementHasDirectionAuto(Element* element)
{
    // FIXME: This line is surprisingly hot, we may wish to inline hasDirectionAuto into StyleResolver.
    return element->isHTMLElement() && toHTMLElement(element)->hasDirectionAuto();
}

static inline bool haveIdenticalStyleAffectingAttributes(StyledElement* a, StyledElement* b)
{
    if (a->attributeData() == b->attributeData())
        return true;
    if (a->fastGetAttribute(XMLNames::langAttr) != b->fastGetAttribute(XMLNames::langAttr))
        return false;
    if (a->fastGetAttribute(langAttr) != b->fastGetAttribute(langAttr))
        return false;
    if (a->hasClass()) {
#if ENABLE(SVG)
        // SVG elements require a (slow!) getAttribute comparision because "class" is an animatable attribute for SVG.
        if (a->isSVGElement()) {
            if (a->getAttribute(classAttr) != b->getAttribute(classAttr))
                return false;
        } else
#endif
        if (a->attributeData()->classNames() != b->attributeData()->classNames())
            return false;
    }

    if (a->presentationAttributeStyle() != b->presentationAttributeStyle())
        return false;

#if ENABLE(PROGRESS_ELEMENT)
    if (a->hasTagName(progressTag)) {
        if (static_cast<HTMLProgressElement*>(a)->isDeterminate() != static_cast<HTMLProgressElement*>(b)->isDeterminate())
            return false;
    }
#endif

    return true;
}

bool StyleResolver::canShareStyleWithElement(StyledElement* element) const
{
    RenderStyle* style = element->renderStyle();

    if (!style)
        return false;
    if (style->unique())
        return false;
    if (element->tagQName() != m_element->tagQName())
        return false;
    if (element->hasClass() != m_element->hasClass())
        return false;
    if (element->inlineStyle())
        return false;
    if (element->needsStyleRecalc())
        return false;
#if ENABLE(SVG)
    if (element->isSVGElement() && static_cast<SVGElement*>(element)->animatedSMILStyleProperties())
        return false;
#endif
    if (element->isLink() != m_element->isLink())
        return false;
    if (element->hovered() != m_element->hovered())
        return false;
    if (element->active() != m_element->active())
        return false;
    if (element->focused() != m_element->focused())
        return false;
    if (element->shadowPseudoId() != m_element->shadowPseudoId())
        return false;
    if (element == element->document()->cssTarget())
        return false;
    if (!haveIdenticalStyleAffectingAttributes(element, m_styledElement))
        return false;
    if (element->additionalPresentationAttributeStyle() != m_styledElement->additionalPresentationAttributeStyle())
        return false;

    if (element->hasID() && m_features.idsInRules.contains(element->idForStyleResolution().impl()))
        return false;
    if (element->hasScopedHTMLStyleChild())
        return false;

    // FIXME: We should share style for option and optgroup whenever possible.
    // Before doing so, we need to resolve issues in HTMLSelectElement::recalcListItems
    // and RenderMenuList::setText. See also https://bugs.webkit.org/show_bug.cgi?id=88405
    if (element->hasTagName(optionTag) || element->hasTagName(optgroupTag))
        return false;

    bool isControl = element->isFormControlElement();

    if (isControl != m_element->isFormControlElement())
        return false;

    if (isControl && !canShareStyleWithControl(element))
        return false;

    if (style->transitions() || style->animations())
        return false;

#if USE(ACCELERATED_COMPOSITING)
    // Turn off style sharing for elements that can gain layers for reasons outside of the style system.
    // See comments in RenderObject::setStyle().
    if (element->hasTagName(iframeTag) || element->hasTagName(frameTag) || element->hasTagName(embedTag) || element->hasTagName(objectTag) || element->hasTagName(appletTag) || element->hasTagName(canvasTag)
#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
        // With proxying, the media elements are backed by a RenderEmbeddedObject.
        || element->hasTagName(videoTag) || element->hasTagName(audioTag)
#endif
        )
        return false;
#endif

    if (elementHasDirectionAuto(element))
        return false;

    if (element->isLink() && m_elementLinkState != style->insideLink())
        return false;

    return true;
}

inline StyledElement* StyleResolver::findSiblingForStyleSharing(Node* node, unsigned& count) const
{
    for (; node; node = node->previousSibling()) {
        if (!node->isStyledElement())
            continue;
        if (canShareStyleWithElement(static_cast<StyledElement*>(node)))
            break;
        if (count++ == cStyleSearchThreshold)
            return 0;
    }
    return static_cast<StyledElement*>(node);
}

static inline bool parentElementPreventsSharing(const Element* parentElement)
{
    if (!parentElement)
        return false;
    return parentElement->childrenAffectedByPositionalRules()
        || parentElement->childrenAffectedByFirstChildRules()
        || parentElement->childrenAffectedByLastChildRules()
        || parentElement->childrenAffectedByDirectAdjacentRules();
}

RenderStyle* StyleResolver::locateSharedStyle()
{
    if (!m_styledElement || !m_parentStyle)
        return 0;
    // If the element has inline style it is probably unique.
    if (m_styledElement->inlineStyle())
        return 0;
#if ENABLE(SVG)
    if (m_styledElement->isSVGElement() && static_cast<SVGElement*>(m_styledElement)->animatedSMILStyleProperties())
        return 0;
#endif
    // Ids stop style sharing if they show up in the stylesheets.
    if (m_styledElement->hasID() && m_features.idsInRules.contains(m_styledElement->idForStyleResolution().impl()))
        return 0;
    if (parentElementPreventsSharing(m_element->parentElement()))
        return 0;
    if (m_styledElement->hasScopedHTMLStyleChild())
        return 0;
    if (m_element == m_element->document()->cssTarget())
        return 0;
    if (elementHasDirectionAuto(m_element))
        return 0;

    // Check previous siblings and their cousins.
    unsigned count = 0;
    unsigned visitedNodeCount = 0;
    StyledElement* shareElement = 0;
    Node* cousinList = m_styledElement->previousSibling();
    while (cousinList) {
        shareElement = findSiblingForStyleSharing(cousinList, count);
        if (shareElement)
            break;
        cousinList = locateCousinList(cousinList->parentElement(), visitedNodeCount);
    }

    // If we have exhausted all our budget or our cousins.
    if (!shareElement)
        return 0;

    // Can't share if sibling rules apply. This is checked at the end as it should rarely fail.
    if (styleSharingCandidateMatchesRuleSet(m_siblingRuleSet.get()))
        return 0;
    // Can't share if attribute rules apply.
    if (styleSharingCandidateMatchesRuleSet(m_uncommonAttributeRuleSet.get()))
        return 0;
    // Can't share if @host @-rules apply.
    if (styleSharingCandidateMatchesHostRules())
        return 0;
    // Tracking child index requires unique style for each node. This may get set by the sibling rule match above.
    if (parentElementPreventsSharing(m_element->parentElement()))
        return 0;
    return shareElement->renderStyle();
}

void StyleResolver::matchUARules(MatchResult& result)
{
    MatchingUARulesScope scope;

    // First we match rules from the user agent sheet.
    if (simpleDefaultStyleSheet)
        result.isCacheable = false;
    RuleSet* userAgentStyleSheet = m_medium->mediaTypeMatchSpecific("print")
        ? defaultPrintStyle : defaultStyle;
    matchUARules(result, userAgentStyleSheet);

    // In quirks mode, we match rules from the quirks user agent sheet.
    if (!m_checker.strictParsing())
        matchUARules(result, defaultQuirksStyle);

    // If document uses view source styles (in view source mode or in xml viewer mode), then we match rules from the view source style sheet.
    if (m_checker.document()->isViewSource()) {
        if (!defaultViewSourceStyle)
            loadViewSourceStyle();
        matchUARules(result, defaultViewSourceStyle);
    }
}

static void setStylesForPaginationMode(Pagination::Mode paginationMode, RenderStyle* style)
{
    if (paginationMode == Pagination::Unpaginated)
        return;
        
    switch (paginationMode) {
    case Pagination::LeftToRightPaginated:
        style->setColumnAxis(HorizontalColumnAxis);
        if (style->isHorizontalWritingMode())
            style->setColumnProgression(style->isLeftToRightDirection() ? NormalColumnProgression : ReverseColumnProgression);
        else
            style->setColumnProgression(style->isFlippedBlocksWritingMode() ? ReverseColumnProgression : NormalColumnProgression);
        break;
    case Pagination::RightToLeftPaginated:
        style->setColumnAxis(HorizontalColumnAxis);
        if (style->isHorizontalWritingMode())
            style->setColumnProgression(style->isLeftToRightDirection() ? ReverseColumnProgression : NormalColumnProgression);
        else
            style->setColumnProgression(style->isFlippedBlocksWritingMode() ? NormalColumnProgression : ReverseColumnProgression);
        break;
    case Pagination::TopToBottomPaginated:
        style->setColumnAxis(VerticalColumnAxis);
        if (style->isHorizontalWritingMode())
            style->setColumnProgression(style->isFlippedBlocksWritingMode() ? ReverseColumnProgression : NormalColumnProgression);
        else
            style->setColumnProgression(style->isLeftToRightDirection() ? NormalColumnProgression : ReverseColumnProgression);
        break;
    case Pagination::BottomToTopPaginated:
        style->setColumnAxis(VerticalColumnAxis);
        if (style->isHorizontalWritingMode())
            style->setColumnProgression(style->isFlippedBlocksWritingMode() ? NormalColumnProgression : ReverseColumnProgression);
        else
            style->setColumnProgression(style->isLeftToRightDirection() ? ReverseColumnProgression : NormalColumnProgression);
        break;
    case Pagination::Unpaginated:
        ASSERT_NOT_REACHED();
        break;
    }
}

PassRefPtr<RenderStyle> StyleResolver::styleForDocument(Document* document, CSSFontSelector* fontSelector)
{
    Frame* frame = document->frame();

    // HTML5 states that seamless iframes should replace default CSS values
    // with values inherited from the containing iframe element. However,
    // some values (such as the case of designMode = "on") still need to
    // be set by this "document style".
    RefPtr<RenderStyle> documentStyle = RenderStyle::create();
    bool seamlessWithParent = document->shouldDisplaySeamlesslyWithParent();
    if (seamlessWithParent) {
        RenderStyle* iframeStyle = document->seamlessParentIFrame()->renderStyle();
        if (iframeStyle)
            documentStyle->inheritFrom(iframeStyle);
    }

    // FIXME: It's not clear which values below we want to override in the seamless case!
    documentStyle->setDisplay(BLOCK);
    if (!seamlessWithParent) {
        documentStyle->setRTLOrdering(document->visuallyOrdered() ? VisualOrder : LogicalOrder);
        documentStyle->setZoom(frame && !document->printing() ? frame->pageZoomFactor() : 1);
        documentStyle->setPageScaleTransform(frame ? frame->frameScaleFactor() : 1);
        documentStyle->setLocale(document->contentLanguage());
    }
    // This overrides any -webkit-user-modify inherited from the parent iframe.
    documentStyle->setUserModify(document->inDesignMode() ? READ_WRITE : READ_ONLY);

    Element* docElement = document->documentElement();
    RenderObject* docElementRenderer = docElement ? docElement->renderer() : 0;
    if (docElementRenderer) {
        // Use the direction and writing-mode of the body to set the
        // viewport's direction and writing-mode unless the property is set on the document element.
        // If there is no body, then use the document element.
        RenderObject* bodyRenderer = document->body() ? document->body()->renderer() : 0;
        if (bodyRenderer && !document->writingModeSetOnDocumentElement())
            documentStyle->setWritingMode(bodyRenderer->style()->writingMode());
        else
            documentStyle->setWritingMode(docElementRenderer->style()->writingMode());
        if (bodyRenderer && !document->directionSetOnDocumentElement())
            documentStyle->setDirection(bodyRenderer->style()->direction());
        else
            documentStyle->setDirection(docElementRenderer->style()->direction());
    }

    if (frame) {
        if (FrameView* frameView = frame->view()) {
            const Pagination& pagination = frameView->pagination();
            if (pagination.mode != Pagination::Unpaginated) {
                setStylesForPaginationMode(pagination.mode, documentStyle.get());
                documentStyle->setColumnGap(pagination.gap);
                if (RenderView* view = document->renderView()) {
                    if (view->hasColumns())
                        view->updateColumnInfoFromStyle(documentStyle.get());
                }
            }
        }
    }

    // Seamless iframes want to inherit their font from their parent iframe, so early return before setting the font.
    if (seamlessWithParent)
        return documentStyle.release();

    FontDescription fontDescription;
    fontDescription.setScript(localeToScriptCodeForFontSelection(documentStyle->locale()));
    if (Settings* settings = document->settings()) {
        fontDescription.setUsePrinterFont(document->printing() || !settings->screenFontSubstitutionEnabled());
        fontDescription.setRenderingMode(settings->fontRenderingMode());
        const AtomicString& standardFont = settings->standardFontFamily(fontDescription.script());
        if (!standardFont.isEmpty()) {
            fontDescription.setGenericFamily(FontDescription::StandardFamily);
            fontDescription.firstFamily().setFamily(standardFont);
            fontDescription.firstFamily().appendFamily(0);
        }
        fontDescription.setKeywordSize(CSSValueMedium - CSSValueXxSmall + 1);
        int size = StyleResolver::fontSizeForKeyword(document, CSSValueMedium, false);
        fontDescription.setSpecifiedSize(size);
        bool useSVGZoomRules = document->isSVGDocument();
        fontDescription.setComputedSize(StyleResolver::getComputedSizeFromSpecifiedSize(document, documentStyle.get(), fontDescription.isAbsoluteSize(), size, useSVGZoomRules));
    } else
        fontDescription.setUsePrinterFont(document->printing());

    documentStyle->setFontDescription(fontDescription);
    documentStyle->font().update(fontSelector);

    return documentStyle.release();
}

static inline bool isAtShadowBoundary(const Element* element)
{
    if (!element)
        return false;
    ContainerNode* parentNode = element->parentNode();
    return parentNode && parentNode->isShadowRoot();
}

PassRefPtr<RenderStyle> StyleResolver::styleForElement(Element* element, RenderStyle* defaultParent,
    StyleSharingBehavior sharingBehavior, RuleMatchingBehavior matchingBehavior, RenderRegion* regionForStyling)
{
    // Once an element has a renderer, we don't try to destroy it, since otherwise the renderer
    // will vanish if a style recalc happens during loading.
    if (sharingBehavior == AllowStyleSharing && !element->document()->haveStylesheetsLoaded() && !element->renderer()) {
        if (!s_styleNotYetAvailable) {
            s_styleNotYetAvailable = RenderStyle::create().leakRef();
            s_styleNotYetAvailable->setDisplay(NONE);
            s_styleNotYetAvailable->font().update(m_fontSelector);
        }
        element->document()->setHasNodesWithPlaceholderStyle();
        return s_styleNotYetAvailable;
    }

    initElement(element);
    initForStyleResolve(element, defaultParent);
    m_regionForStyling = regionForStyling;
    if (sharingBehavior == AllowStyleSharing && !m_distributedToInsertionPoint) {
        RenderStyle* sharedStyle = locateSharedStyle();
        if (sharedStyle)
            return sharedStyle;
    }

    m_style = RenderStyle::create();

    RefPtr<RenderStyle> cloneForParent;

    if (m_parentStyle)
        m_style->inheritFrom(m_parentStyle, isAtShadowBoundary(element) ? RenderStyle::AtShadowBoundary : RenderStyle::NotAtShadowBoundary);
    else {
        // Make sure our fonts are initialized if we don't inherit them from our parent style.
        if (Settings* settings = documentSettings()) {
            initializeFontStyle(settings);
            m_style->font().update(fontSelector());
        } else
            m_style->font().update(0);
        cloneForParent = RenderStyle::clone(style());
        m_parentStyle = cloneForParent.get();
    }
    // contenteditable attribute (implemented by -webkit-user-modify) should
    // be propagated from shadow host to distributed node.
    if (m_distributedToInsertionPoint) {
        if (Element* parent = element->parentElement()) {
            if (RenderStyle* styleOfShadowHost = parent->renderStyle())
                m_style->setUserModify(styleOfShadowHost->userModify());
        }
    }

    if (element->isLink()) {
        m_style->setIsLink(true);
        m_style->setInsideLink(m_elementLinkState);
    }

    ensureDefaultStyleSheetsForElement(element);

    MatchResult matchResult;
    if (matchingBehavior == MatchOnlyUserAgentRules)
        matchUARules(matchResult);
    else
        matchAllRules(matchResult, matchingBehavior != MatchAllRulesExcludingSMIL);

    applyMatchedProperties(matchResult, element);

    // Clean up our style object's display and text decorations (among other fixups).
    adjustRenderStyle(style(), m_parentStyle, element);

    initElement(0); // Clear out for the next resolve.

    if (cloneForParent)
        m_parentStyle = 0;

    // Now return the style.
    return m_style.release();
}

PassRefPtr<RenderStyle> StyleResolver::styleForKeyframe(const RenderStyle* elementStyle, const StyleKeyframe* keyframe, KeyframeValue& keyframeValue)
{
    MatchResult result;
    if (keyframe->properties())
        addMatchedProperties(result, keyframe->properties());

    ASSERT(!m_style);

    // Create the style
    m_style = RenderStyle::clone(elementStyle);

    m_lineHeightValue = 0;

    // We don't need to bother with !important. Since there is only ever one
    // decl, there's nothing to override. So just add the first properties.
    bool inheritedOnly = false;
    if (keyframe->properties())
        applyMatchedProperties<HighPriorityProperties>(result, false, 0, result.matchedProperties.size() - 1, inheritedOnly);

    // If our font got dirtied, go ahead and update it now.
    updateFont();

    // Line-height is set when we are sure we decided on the font-size
    if (m_lineHeightValue)
        applyProperty(CSSPropertyLineHeight, m_lineHeightValue);

    // Now do rest of the properties.
    if (keyframe->properties())
        applyMatchedProperties<LowPriorityProperties>(result, false, 0, result.matchedProperties.size() - 1, inheritedOnly);

    // If our font got dirtied by one of the non-essential font props,
    // go ahead and update it a second time.
    updateFont();

    // Start loading resources referenced by this style.
    loadPendingResources();
    
    // Add all the animating properties to the keyframe.
    if (const StylePropertySet* styleDeclaration = keyframe->properties()) {
        unsigned propertyCount = styleDeclaration->propertyCount();
        for (unsigned i = 0; i < propertyCount; ++i) {
            CSSPropertyID property = styleDeclaration->propertyAt(i).id();
            // Timing-function within keyframes is special, because it is not animated; it just
            // describes the timing function between this keyframe and the next.
            if (property != CSSPropertyWebkitAnimationTimingFunction)
                keyframeValue.addProperty(property);
        }
    }

    return m_style.release();
}

void StyleResolver::keyframeStylesForAnimation(Element* e, const RenderStyle* elementStyle, KeyframeList& list)
{
    list.clear();

    // Get the keyframesRule for this name
    if (!e || list.animationName().isEmpty())
        return;

    m_keyframesRuleMap.checkConsistency();

    KeyframesRuleMap::iterator it = m_keyframesRuleMap.find(list.animationName().impl());
    if (it == m_keyframesRuleMap.end())
        return;

    const StyleRuleKeyframes* keyframesRule = it->value.get();

    // Construct and populate the style for each keyframe
    const Vector<RefPtr<StyleKeyframe> >& keyframes = keyframesRule->keyframes();
    for (unsigned i = 0; i < keyframes.size(); ++i) {
        // Apply the declaration to the style. This is a simplified version of the logic in styleForElement
        initElement(e);
        initForStyleResolve(e);

        const StyleKeyframe* keyframe = keyframes[i].get();

        KeyframeValue keyframeValue(0, 0);
        keyframeValue.setStyle(styleForKeyframe(elementStyle, keyframe, keyframeValue));

        // Add this keyframe style to all the indicated key times
        Vector<float> keys;
        keyframe->getKeys(keys);
        for (size_t keyIndex = 0; keyIndex < keys.size(); ++keyIndex) {
            keyframeValue.setKey(keys[keyIndex]);
            list.insert(keyframeValue);
        }
    }

    // If the 0% keyframe is missing, create it (but only if there is at least one other keyframe)
    int initialListSize = list.size();
    if (initialListSize > 0 && list[0].key()) {
        static StyleKeyframe* zeroPercentKeyframe;
        if (!zeroPercentKeyframe) {
            zeroPercentKeyframe = StyleKeyframe::create().leakRef();
            zeroPercentKeyframe->setKeyText("0%");
        }
        KeyframeValue keyframeValue(0, 0);
        keyframeValue.setStyle(styleForKeyframe(elementStyle, zeroPercentKeyframe, keyframeValue));
        list.insert(keyframeValue);
    }

    // If the 100% keyframe is missing, create it (but only if there is at least one other keyframe)
    if (initialListSize > 0 && (list[list.size() - 1].key() != 1)) {
        static StyleKeyframe* hundredPercentKeyframe;
        if (!hundredPercentKeyframe) {
            hundredPercentKeyframe = StyleKeyframe::create().leakRef();
            hundredPercentKeyframe->setKeyText("100%");
        }
        KeyframeValue keyframeValue(1, 0);
        keyframeValue.setStyle(styleForKeyframe(elementStyle, hundredPercentKeyframe, keyframeValue));
        list.insert(keyframeValue);
    }
}

PassRefPtr<RenderStyle> StyleResolver::pseudoStyleForElement(PseudoId pseudo, Element* e, RenderStyle* parentStyle)
{
    ASSERT(m_parentStyle);
    if (!e)
        return 0;

    initElement(e);

    initForStyleResolve(e, parentStyle, pseudo);
    m_style = RenderStyle::create();
    m_style->inheritFrom(m_parentStyle);

    // Since we don't use pseudo-elements in any of our quirk/print user agent rules, don't waste time walking
    // those rules.

    // Check UA, user and author rules.
    MatchResult matchResult;
    matchUARules(matchResult);

    if (m_matchAuthorAndUserStyles) {
        matchUserRules(matchResult, false);
        matchAuthorRules(matchResult, false);
    }

    if (matchResult.matchedProperties.isEmpty())
        return 0;

    m_style->setStyleType(pseudo);

    applyMatchedProperties(matchResult, e);

    // Clean up our style object's display and text decorations (among other fixups).
    adjustRenderStyle(style(), parentStyle, 0);

    // Start loading resources referenced by this style.
    loadPendingResources();

    // Now return the style.
    return m_style.release();
}

PassRefPtr<RenderStyle> StyleResolver::styleForPage(int pageIndex)
{
    initForStyleResolve(m_checker.document()->documentElement()); // m_rootElementStyle will be set to the document style.

    m_style = RenderStyle::create();
    m_style->inheritFrom(m_rootElementStyle);

    const bool isLeft = isLeftPage(pageIndex);
    const bool isFirst = isFirstPage(pageIndex);
    const String page = pageName(pageIndex);
    
    MatchResult result;
    matchPageRules(result, defaultPrintStyle, isLeft, isFirst, page);
    matchPageRules(result, m_userStyle.get(), isLeft, isFirst, page);
    // Only consider the global author RuleSet for @page rules, as per the HTML5 spec.
    matchPageRules(result, m_authorStyle.get(), isLeft, isFirst, page);
    m_lineHeightValue = 0;
    bool inheritedOnly = false;
#if ENABLE(CSS_VARIABLES)
    applyMatchedProperties<VariableDefinitions>(result, false, 0, result.matchedProperties.size() - 1, inheritedOnly);
#endif
    applyMatchedProperties<HighPriorityProperties>(result, false, 0, result.matchedProperties.size() - 1, inheritedOnly);

    // If our font got dirtied, go ahead and update it now.
    updateFont();

    // Line-height is set when we are sure we decided on the font-size.
    if (m_lineHeightValue)
        applyProperty(CSSPropertyLineHeight, m_lineHeightValue);

    applyMatchedProperties<LowPriorityProperties>(result, false, 0, result.matchedProperties.size() - 1, inheritedOnly);

    // Start loading resources referenced by this style.
    loadPendingResources();

    // Now return the style.
    return m_style.release();
}

static void addIntrinsicMargins(RenderStyle* style)
{
    // Intrinsic margin value.
    const int intrinsicMargin = 2 * style->effectiveZoom();

    // FIXME: Using width/height alone and not also dealing with min-width/max-width is flawed.
    // FIXME: Using "quirk" to decide the margin wasn't set is kind of lame.
    if (style->width().isIntrinsicOrAuto()) {
        if (style->marginLeft().quirk())
            style->setMarginLeft(Length(intrinsicMargin, Fixed));
        if (style->marginRight().quirk())
            style->setMarginRight(Length(intrinsicMargin, Fixed));
    }

    if (style->height().isAuto()) {
        if (style->marginTop().quirk())
            style->setMarginTop(Length(intrinsicMargin, Fixed));
        if (style->marginBottom().quirk())
            style->setMarginBottom(Length(intrinsicMargin, Fixed));
    }
}

static EDisplay equivalentBlockDisplay(EDisplay display, bool isFloating, bool strictParsing)
{
    switch (display) {
    case BLOCK:
    case TABLE:
    case BOX:
    case FLEX:
    case GRID:
        return display;

    case LIST_ITEM:
        // It is a WinIE bug that floated list items lose their bullets, so we'll emulate the quirk, but only in quirks mode.
        if (!strictParsing && isFloating)
            return BLOCK;
        return display;
    case INLINE_TABLE:
        return TABLE;
    case INLINE_BOX:
        return BOX;
    case INLINE_FLEX:
        return FLEX;
    case INLINE_GRID:
        return GRID;

    case INLINE:
    case RUN_IN:
    case COMPACT:
    case INLINE_BLOCK:
    case TABLE_ROW_GROUP:
    case TABLE_HEADER_GROUP:
    case TABLE_FOOTER_GROUP:
    case TABLE_ROW:
    case TABLE_COLUMN_GROUP:
    case TABLE_COLUMN:
    case TABLE_CELL:
    case TABLE_CAPTION:
        return BLOCK;
    case NONE:
        ASSERT_NOT_REACHED();
        return NONE;
    }
    ASSERT_NOT_REACHED();
    return BLOCK;
}

// CSS requires text-decoration to be reset at each DOM element for tables, 
// inline blocks, inline tables, run-ins, shadow DOM crossings, floating elements,
// and absolute or relatively positioned elements.
static bool doesNotInheritTextDecoration(RenderStyle* style, Element* e)
{
    return style->display() == TABLE || style->display() == INLINE_TABLE || style->display() == RUN_IN
        || style->display() == INLINE_BLOCK || style->display() == INLINE_BOX || isAtShadowBoundary(e)
        || style->isFloating() || style->hasOutOfFlowPosition();
}

static bool isDisplayFlexibleBox(EDisplay display)
{
    return display == FLEX || display == INLINE_FLEX;
}

void StyleResolver::adjustRenderStyle(RenderStyle* style, RenderStyle* parentStyle, Element *e)
{
    ASSERT(parentStyle);

    // Cache our original display.
    style->setOriginalDisplay(style->display());

    if (style->display() != NONE) {
        // If we have a <td> that specifies a float property, in quirks mode we just drop the float
        // property.
        // Sites also commonly use display:inline/block on <td>s and <table>s. In quirks mode we force
        // these tags to retain their display types.
        if (!m_checker.strictParsing() && e) {
            if (e->hasTagName(tdTag)) {
                style->setDisplay(TABLE_CELL);
                style->setFloating(NoFloat);
            } else if (e->hasTagName(tableTag))
                style->setDisplay(style->isDisplayInlineType() ? INLINE_TABLE : TABLE);
        }

        if (e && (e->hasTagName(tdTag) || e->hasTagName(thTag))) {
            if (style->whiteSpace() == KHTML_NOWRAP) {
                // Figure out if we are really nowrapping or if we should just
                // use normal instead. If the width of the cell is fixed, then
                // we don't actually use NOWRAP.
                if (style->width().isFixed())
                    style->setWhiteSpace(NORMAL);
                else
                    style->setWhiteSpace(NOWRAP);
            }
        }

        // Tables never support the -webkit-* values for text-align and will reset back to the default.
        if (e && e->hasTagName(tableTag) && (style->textAlign() == WEBKIT_LEFT || style->textAlign() == WEBKIT_CENTER || style->textAlign() == WEBKIT_RIGHT))
            style->setTextAlign(TASTART);

        // Frames and framesets never honor position:relative or position:absolute. This is necessary to
        // fix a crash where a site tries to position these objects. They also never honor display.
        if (e && (e->hasTagName(frameTag) || e->hasTagName(framesetTag))) {
            style->setPosition(StaticPosition);
            style->setDisplay(BLOCK);
        }

        // Ruby text does not support float or position. This might change with evolution of the specification.
        if (e && e->hasTagName(rtTag)) {
            style->setPosition(StaticPosition);
            style->setFloating(NoFloat);
        }

        // FIXME: We shouldn't be overriding start/-webkit-auto like this. Do it in html.css instead.
        // Table headers with a text-align of -webkit-auto will change the text-align to center.
        if (e && e->hasTagName(thTag) && style->textAlign() == TASTART)
            style->setTextAlign(CENTER);

        if (e && e->hasTagName(legendTag))
            style->setDisplay(BLOCK);

        // Absolute/fixed positioned elements, floating elements and the document element need block-like outside display.
        if (style->hasOutOfFlowPosition() || style->isFloating() || (e && e->document()->documentElement() == e))
            style->setDisplay(equivalentBlockDisplay(style->display(), style->isFloating(), m_checker.strictParsing()));

        // FIXME: Don't support this mutation for pseudo styles like first-letter or first-line, since it's not completely
        // clear how that should work.
        if (style->display() == INLINE && style->styleType() == NOPSEUDO && style->writingMode() != parentStyle->writingMode())
            style->setDisplay(INLINE_BLOCK);

        // After performing the display mutation, check table rows. We do not honor position:relative on
        // table rows or cells. This has been established in CSS2.1 (and caused a crash in containingBlock()
        // on some sites).
        if ((style->display() == TABLE_HEADER_GROUP || style->display() == TABLE_ROW_GROUP
             || style->display() == TABLE_FOOTER_GROUP || style->display() == TABLE_ROW)
             && style->position() == RelativePosition)
            style->setPosition(StaticPosition);

        // writing-mode does not apply to table row groups, table column groups, table rows, and table columns.
        // FIXME: Table cells should be allowed to be perpendicular or flipped with respect to the table, though.
        if (style->display() == TABLE_COLUMN || style->display() == TABLE_COLUMN_GROUP || style->display() == TABLE_FOOTER_GROUP
            || style->display() == TABLE_HEADER_GROUP || style->display() == TABLE_ROW || style->display() == TABLE_ROW_GROUP
            || style->display() == TABLE_CELL)
            style->setWritingMode(parentStyle->writingMode());

        // FIXME: Since we don't support block-flow on flexible boxes yet, disallow setting
        // of block-flow to anything other than TopToBottomWritingMode.
        // https://bugs.webkit.org/show_bug.cgi?id=46418 - Flexible box support.
        if (style->writingMode() != TopToBottomWritingMode && (style->display() == BOX || style->display() == INLINE_BOX))
            style->setWritingMode(TopToBottomWritingMode);

        if (isDisplayFlexibleBox(parentStyle->display())) {
            style->setFloating(NoFloat);
            style->setDisplay(equivalentBlockDisplay(style->display(), style->isFloating(), m_checker.strictParsing()));
        }
    }

    // Make sure our z-index value is only applied if the object is positioned.
    if (style->position() == StaticPosition && !isDisplayFlexibleBox(parentStyle->display()))
        style->setHasAutoZIndex();

    // Auto z-index becomes 0 for the root element and transparent objects. This prevents
    // cases where objects that should be blended as a single unit end up with a non-transparent
    // object wedged in between them. Auto z-index also becomes 0 for objects that specify transforms/masks/reflections.
    if (style->hasAutoZIndex() && ((e && e->document()->documentElement() == e)
        || style->opacity() < 1.0f
        || style->hasTransformRelatedProperty()
        || style->hasMask()
        || style->clipPath()
        || style->boxReflect()
        || style->hasFilter()
        || style->hasBlendMode()
        || style->position() == StickyPosition
        || (style->position() == FixedPosition && e && e->document()->page() && e->document()->page()->settings()->fixedPositionCreatesStackingContext())
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
        // Touch overflow scrolling creates a stacking context.
        || ((style->overflowX() != OHIDDEN || style->overflowY() != OHIDDEN) && style->useTouchOverflowScrolling())
#endif
#if ENABLE(DIALOG_ELEMENT)
        || (e && e->isInTopLayer())
#endif
        ))
        style->setZIndex(0);

    // Textarea considers overflow visible as auto.
    if (e && e->hasTagName(textareaTag)) {
        style->setOverflowX(style->overflowX() == OVISIBLE ? OAUTO : style->overflowX());
        style->setOverflowY(style->overflowY() == OVISIBLE ? OAUTO : style->overflowY());
    }

    if (doesNotInheritTextDecoration(style, e))
        style->setTextDecorationsInEffect(style->textDecoration());
    else
        style->addToTextDecorationsInEffect(style->textDecoration());

    // If either overflow value is not visible, change to auto.
    if (style->overflowX() == OMARQUEE && style->overflowY() != OMARQUEE)
        style->setOverflowY(OMARQUEE);
    else if (style->overflowY() == OMARQUEE && style->overflowX() != OMARQUEE)
        style->setOverflowX(OMARQUEE);
    else if (style->overflowX() == OVISIBLE && style->overflowY() != OVISIBLE) {
        // FIXME: Once we implement pagination controls, overflow-x should default to hidden
        // if overflow-y is set to -webkit-paged-x or -webkit-page-y. For now, we'll let it
        // default to auto so we can at least scroll through the pages.
        style->setOverflowX(OAUTO);
    } else if (style->overflowY() == OVISIBLE && style->overflowX() != OVISIBLE)
        style->setOverflowY(OAUTO);

    // Call setStylesForPaginationMode() if a pagination mode is set for any non-root elements. If these
    // styles are specified on a root element, then they will be incorporated in
    // StyleResolver::styleForDocument().
    if ((style->overflowY() == OPAGEDX || style->overflowY() == OPAGEDY) && !(e->hasTagName(htmlTag) || e->hasTagName(bodyTag)))
        setStylesForPaginationMode(WebCore::paginationModeForRenderStyle(style), style);

    // Table rows, sections and the table itself will support overflow:hidden and will ignore scroll/auto.
    // FIXME: Eventually table sections will support auto and scroll.
    if (style->display() == TABLE || style->display() == INLINE_TABLE
        || style->display() == TABLE_ROW_GROUP || style->display() == TABLE_ROW) {
        if (style->overflowX() != OVISIBLE && style->overflowX() != OHIDDEN)
            style->setOverflowX(OVISIBLE);
        if (style->overflowY() != OVISIBLE && style->overflowY() != OHIDDEN)
            style->setOverflowY(OVISIBLE);
    }

    // Menulists should have visible overflow
    if (style->appearance() == MenulistPart) {
        style->setOverflowX(OVISIBLE);
        style->setOverflowY(OVISIBLE);
    }

    // Cull out any useless layers and also repeat patterns into additional layers.
    style->adjustBackgroundLayers();
    style->adjustMaskLayers();

    // Do the same for animations and transitions.
    style->adjustAnimations();
    style->adjustTransitions();

    // Important: Intrinsic margins get added to controls before the theme has adjusted the style, since the theme will
    // alter fonts and heights/widths.
    if (e && e->isFormControlElement() && style->fontSize() >= 11) {
        // Don't apply intrinsic margins to image buttons. The designer knows how big the images are,
        // so we have to treat all image buttons as though they were explicitly sized.
        if (!e->hasTagName(inputTag) || !static_cast<HTMLInputElement*>(e)->isImageButton())
            addIntrinsicMargins(style);
    }

    // Let the theme also have a crack at adjusting the style.
    if (style->hasAppearance())
        RenderTheme::defaultTheme()->adjustStyle(this, style, e, m_hasUAAppearance, m_borderData, m_backgroundData, m_backgroundColor);

    // If we have first-letter pseudo style, do not share this style.
    if (style->hasPseudoStyle(FIRST_LETTER))
        style->setUnique();

    // FIXME: when dropping the -webkit prefix on transform-style, we should also have opacity < 1 cause flattening.
    if (style->preserves3D() && (style->overflowX() != OVISIBLE
        || style->overflowY() != OVISIBLE
        || style->hasFilter()))
        style->setTransformStyle3D(TransformStyle3DFlat);

    // Seamless iframes behave like blocks. Map their display to inline-block when marked inline.
    if (e && e->hasTagName(iframeTag) && style->display() == INLINE && static_cast<HTMLIFrameElement*>(e)->shouldDisplaySeamlessly())
        style->setDisplay(INLINE_BLOCK);

#if ENABLE(SVG)
    if (e && e->isSVGElement()) {
        // Spec: http://www.w3.org/TR/SVG/masking.html#OverflowProperty
        if (style->overflowY() == OSCROLL)
            style->setOverflowY(OHIDDEN);
        else if (style->overflowY() == OAUTO)
            style->setOverflowY(OVISIBLE);

        if (style->overflowX() == OSCROLL)
            style->setOverflowX(OHIDDEN);
        else if (style->overflowX() == OAUTO)
            style->setOverflowX(OVISIBLE);

        // Only the root <svg> element in an SVG document fragment tree honors css position
        if (!(e->hasTagName(SVGNames::svgTag) && e->parentNode() && !e->parentNode()->isSVGElement()))
            style->setPosition(RenderStyle::initialPosition());

        // RenderSVGRoot handles zooming for the whole SVG subtree, so foreignObject content should
        // not be scaled again.
        if (e->hasTagName(SVGNames::foreignObjectTag))
            style->setEffectiveZoom(RenderStyle::initialZoom());
    }
#endif
}

bool StyleResolver::checkRegionStyle(Element* regionElement)
{
    // FIXME (BUG 72472): We don't add @-webkit-region rules of scoped style sheets for the moment,
    // so all region rules are global by default. Verify whether that can stand or needs changing.

    unsigned rulesSize = m_authorStyle->m_regionSelectorsAndRuleSets.size();
    for (unsigned i = 0; i < rulesSize; ++i) {
        ASSERT(m_authorStyle->m_regionSelectorsAndRuleSets.at(i).ruleSet.get());
        if (checkRegionSelector(m_authorStyle->m_regionSelectorsAndRuleSets.at(i).selector, regionElement))
            return true;
    }

    if (m_userStyle) {
        rulesSize = m_userStyle->m_regionSelectorsAndRuleSets.size();
        for (unsigned i = 0; i < rulesSize; ++i) {
            ASSERT(m_userStyle->m_regionSelectorsAndRuleSets.at(i).ruleSet.get());
            if (checkRegionSelector(m_userStyle->m_regionSelectorsAndRuleSets.at(i).selector, regionElement))
                return true;
        }
    }

    return false;
}

void StyleResolver::updateFont()
{
    if (!m_fontDirty)
        return;

    checkForTextSizeAdjust();
    checkForGenericFamilyChange(style(), m_parentStyle);
    checkForZoomChange(style(), m_parentStyle);
    m_style->font().update(m_fontSelector);
    m_fontDirty = false;
}

void StyleResolver::cacheBorderAndBackground()
{
    m_hasUAAppearance = m_style->hasAppearance();
    if (m_hasUAAppearance) {
        m_borderData = m_style->border();
        m_backgroundData = *m_style->backgroundLayers();
        m_backgroundColor = m_style->backgroundColor();
    }
}

PassRefPtr<CSSRuleList> StyleResolver::styleRulesForElement(Element* e, unsigned rulesToInclude)
{
    return pseudoStyleRulesForElement(e, NOPSEUDO, rulesToInclude);
}

PassRefPtr<CSSRuleList> StyleResolver::pseudoStyleRulesForElement(Element* e, PseudoId pseudoId, unsigned rulesToInclude)
{
    if (!e || !e->document()->haveStylesheetsLoaded())
        return 0;

    m_checker.setMode(SelectorChecker::CollectingRules);

    initElement(e);
    initForStyleResolve(e, 0, pseudoId);

    MatchResult dummy;
    if (rulesToInclude & UAAndUserCSSRules) {
        // First we match rules from the user agent sheet.
        matchUARules(dummy);

        // Now we check user sheet rules.
        if (m_matchAuthorAndUserStyles)
            matchUserRules(dummy, rulesToInclude & EmptyCSSRules);
    }

    if (m_matchAuthorAndUserStyles && (rulesToInclude & AuthorCSSRules)) {
        m_sameOriginOnly = !(rulesToInclude & CrossOriginCSSRules);

        // Check the rules in author sheets.
        matchAuthorRules(dummy, rulesToInclude & EmptyCSSRules);

        m_sameOriginOnly = false;
    }

    m_checker.setMode(SelectorChecker::ResolvingStyle);

    return m_ruleList.release();
}

inline bool StyleResolver::checkSelector(const RuleData& ruleData, const ContainerNode* scope)
{
    m_dynamicPseudo = NOPSEUDO;

    if (ruleData.hasFastCheckableSelector()) {
        // We know this selector does not include any pseudo elements.
        if (m_pseudoStyle != NOPSEUDO)
            return false;
        // We know a sufficiently simple single part selector matches simply because we found it from the rule hash.
        // This is limited to HTML only so we don't need to check the namespace.
        if (ruleData.hasRightmostSelectorMatchingHTMLBasedOnRuleHash() && m_element->isHTMLElement()) {
            if (!ruleData.hasMultipartSelector())
                return true;
        } else if (!SelectorChecker::tagMatches(m_element, ruleData.selector()))
            return false;
        if (!SelectorChecker::fastCheckRightmostAttributeSelector(m_element, ruleData.selector()))
            return false;
        return m_checker.fastCheckSelector(ruleData.selector(), m_element);
    }

    // Slow path.
    SelectorChecker::SelectorCheckingContext context(ruleData.selector(), m_element, SelectorChecker::VisitedMatchEnabled);
    context.elementStyle = style();
    context.elementParentStyle = m_parentNode ? m_parentNode->renderStyle() : 0;
    context.scope = scope;
    context.pseudoStyle = m_pseudoStyle;
    SelectorChecker::SelectorMatch match = m_checker.checkSelector(context, m_dynamicPseudo);
    if (match != SelectorChecker::SelectorMatches)
        return false;
    if (m_pseudoStyle != NOPSEUDO && m_pseudoStyle != m_dynamicPseudo)
        return false;
    return true;
}

bool StyleResolver::checkRegionSelector(CSSSelector* regionSelector, Element* regionElement)
{
    if (!regionSelector || !regionElement)
        return false;

    m_pseudoStyle = NOPSEUDO;

    for (CSSSelector* s = regionSelector; s; s = CSSSelectorList::next(s))
        if (m_checker.checkSelector(s, regionElement))
            return true;

    return false;
}

// -------------------------------------------------------------------------------------
// this is mostly boring stuff on how to apply a certain rule to the renderstyle...

Length StyleResolver::convertToIntLength(CSSPrimitiveValue* primitiveValue, RenderStyle* style, RenderStyle* rootStyle, double multiplier)
{
    return primitiveValue ? primitiveValue->convertToLength<FixedIntegerConversion | PercentConversion | FractionConversion | ViewportPercentageConversion>(style, rootStyle, multiplier) : Length(Undefined);
}

Length StyleResolver::convertToFloatLength(CSSPrimitiveValue* primitiveValue, RenderStyle* style, RenderStyle* rootStyle, double multiplier)
{
    return primitiveValue ? primitiveValue->convertToLength<FixedFloatConversion | PercentConversion | FractionConversion | ViewportPercentageConversion>(style, rootStyle, multiplier) : Length(Undefined);
}

template <StyleResolver::StyleApplicationPass pass>
void StyleResolver::applyProperties(const StylePropertySet* properties, StyleRule* rule, bool isImportant, bool inheritedOnly, bool filterRegionProperties)
{
    ASSERT(!filterRegionProperties || m_regionForStyling);
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willProcessRule(document(), rule);

    unsigned propertyCount = properties->propertyCount();
    for (unsigned i = 0; i < propertyCount; ++i) {
        StylePropertySet::PropertyReference current = properties->propertyAt(i);
        if (isImportant != current.isImportant())
            continue;
        if (inheritedOnly && !current.isInherited()) {
            // If the property value is explicitly inherited, we need to apply further non-inherited properties
            // as they might override the value inherited here. For this reason we don't allow declarations with
            // explicitly inherited properties to be cached.
            ASSERT(!current.value()->isInheritedValue());
            continue;
        }
        CSSPropertyID property = current.id();

        if (filterRegionProperties && !StyleResolver::isValidRegionStyleProperty(property))
            continue;

        switch (pass) {
#if ENABLE(CSS_VARIABLES)
        case VariableDefinitions:
            COMPILE_ASSERT(CSSPropertyVariable < firstCSSProperty, CSS_variable_is_before_first_property);
            if (property == CSSPropertyVariable)
                applyProperty(current.id(), current.value());
            break;
#endif
        case HighPriorityProperties:
            COMPILE_ASSERT(firstCSSProperty == CSSPropertyColor, CSS_color_is_first_property);
            COMPILE_ASSERT(CSSPropertyZoom == CSSPropertyColor + 18, CSS_zoom_is_end_of_first_prop_range);
            COMPILE_ASSERT(CSSPropertyLineHeight == CSSPropertyZoom + 1, CSS_line_height_is_after_zoom);
#if ENABLE(CSS_VARIABLES)
            if (property == CSSPropertyVariable)
                continue;
#endif
            // give special priority to font-xxx, color properties, etc
            if (property < CSSPropertyLineHeight)
                applyProperty(current.id(), current.value());
            // we apply line-height later
            else if (property == CSSPropertyLineHeight)
                m_lineHeightValue = current.value();
            break;
        case LowPriorityProperties:
            if (property > CSSPropertyLineHeight)
                applyProperty(current.id(), current.value());
        }
    }
    InspectorInstrumentation::didProcessRule(cookie);
}

template <StyleResolver::StyleApplicationPass pass>
void StyleResolver::applyMatchedProperties(const MatchResult& matchResult, bool isImportant, int startIndex, int endIndex, bool inheritedOnly)
{
    if (startIndex == -1)
        return;

    if (m_style->insideLink() != NotInsideLink) {
        for (int i = startIndex; i <= endIndex; ++i) {
            const MatchedProperties& matchedProperties = matchResult.matchedProperties[i];
            unsigned linkMatchType = matchedProperties.linkMatchType;
            // FIXME: It would be nicer to pass these as arguments but that requires changes in many places.
            m_applyPropertyToRegularStyle = linkMatchType & SelectorChecker::MatchLink;
            m_applyPropertyToVisitedLinkStyle = linkMatchType & SelectorChecker::MatchVisited;

            applyProperties<pass>(matchedProperties.properties.get(), matchResult.matchedRules[i], isImportant, inheritedOnly, matchedProperties.isInRegionRule);
        }
        m_applyPropertyToRegularStyle = true;
        m_applyPropertyToVisitedLinkStyle = false;
        return;
    }
    for (int i = startIndex; i <= endIndex; ++i) {
        const MatchedProperties& matchedProperties = matchResult.matchedProperties[i];
        applyProperties<pass>(matchedProperties.properties.get(), matchResult.matchedRules[i], isImportant, inheritedOnly, matchedProperties.isInRegionRule);
    }
}

unsigned StyleResolver::computeMatchedPropertiesHash(const MatchedProperties* properties, unsigned size)
{
    
    return StringHasher::hashMemory(properties, sizeof(MatchedProperties) * size);
}

bool operator==(const StyleResolver::MatchRanges& a, const StyleResolver::MatchRanges& b)
{
    return a.firstUARule == b.firstUARule
        && a.lastUARule == b.lastUARule
        && a.firstAuthorRule == b.firstAuthorRule
        && a.lastAuthorRule == b.lastAuthorRule
        && a.firstUserRule == b.firstUserRule
        && a.lastUserRule == b.lastUserRule;
}

bool operator!=(const StyleResolver::MatchRanges& a, const StyleResolver::MatchRanges& b)
{
    return !(a == b);
}

bool operator==(const StyleResolver::MatchedProperties& a, const StyleResolver::MatchedProperties& b)
{
    return a.properties == b.properties && a.linkMatchType == b.linkMatchType;
}

bool operator!=(const StyleResolver::MatchedProperties& a, const StyleResolver::MatchedProperties& b)
{
    return !(a == b);
}

const StyleResolver::MatchedPropertiesCacheItem* StyleResolver::findFromMatchedPropertiesCache(unsigned hash, const MatchResult& matchResult)
{
    ASSERT(hash);

    MatchedPropertiesCache::iterator it = m_matchedPropertiesCache.find(hash);
    if (it == m_matchedPropertiesCache.end())
        return 0;
    MatchedPropertiesCacheItem& cacheItem = it->value;

    size_t size = matchResult.matchedProperties.size();
    if (size != cacheItem.matchedProperties.size())
        return 0;
    for (size_t i = 0; i < size; ++i) {
        if (matchResult.matchedProperties[i] != cacheItem.matchedProperties[i])
            return 0;
    }
    if (cacheItem.ranges != matchResult.ranges)
        return 0;
    return &cacheItem;
}

void StyleResolver::addToMatchedPropertiesCache(const RenderStyle* style, const RenderStyle* parentStyle, unsigned hash, const MatchResult& matchResult)
{
    static const unsigned matchedDeclarationCacheAdditionsBetweenSweeps = 100;
    if (++m_matchedPropertiesCacheAdditionsSinceLastSweep >= matchedDeclarationCacheAdditionsBetweenSweeps) {
        static const unsigned matchedDeclarationCacheSweepTimeInSeconds = 60;
        m_matchedPropertiesCacheSweepTimer.startOneShot(matchedDeclarationCacheSweepTimeInSeconds);
    }

    ASSERT(hash);
    MatchedPropertiesCacheItem cacheItem;
    cacheItem.matchedProperties.append(matchResult.matchedProperties);
    cacheItem.ranges = matchResult.ranges;
    // Note that we don't cache the original RenderStyle instance. It may be further modified.
    // The RenderStyle in the cache is really just a holder for the substructures and never used as-is.
    cacheItem.renderStyle = RenderStyle::clone(style);
    cacheItem.parentRenderStyle = RenderStyle::clone(parentStyle);
    m_matchedPropertiesCache.add(hash, cacheItem);
}

void StyleResolver::invalidateMatchedPropertiesCache()
{
    m_matchedPropertiesCache.clear();
}

static bool isCacheableInMatchedPropertiesCache(const Element* element, const RenderStyle* style, const RenderStyle* parentStyle)
{
    // FIXME: CSSPropertyWebkitWritingMode modifies state when applying to document element. We can't skip the applying by caching.
    if (element == element->document()->documentElement() && element->document()->writingModeSetOnDocumentElement())
        return false;
    if (style->unique() || (style->styleType() != NOPSEUDO && parentStyle->unique()))
        return false;
    if (style->hasAppearance())
        return false;
    if (style->zoom() != RenderStyle::initialZoom())
        return false;
    if (style->writingMode() != RenderStyle::initialWritingMode())
        return false;
    // The cache assumes static knowledge about which properties are inherited.
    if (parentStyle->hasExplicitlyInheritedProperties())
        return false;
    return true;
}

void StyleResolver::applyMatchedProperties(const MatchResult& matchResult, const Element* element)
{
    ASSERT(element);
    unsigned cacheHash = matchResult.isCacheable ? computeMatchedPropertiesHash(matchResult.matchedProperties.data(), matchResult.matchedProperties.size()) : 0;
    bool applyInheritedOnly = false;
    const MatchedPropertiesCacheItem* cacheItem = 0;
    if (cacheHash && (cacheItem = findFromMatchedPropertiesCache(cacheHash, matchResult))) {
        // We can build up the style by copying non-inherited properties from an earlier style object built using the same exact
        // style declarations. We then only need to apply the inherited properties, if any, as their values can depend on the 
        // element context. This is fast and saves memory by reusing the style data structures.
        m_style->copyNonInheritedFrom(cacheItem->renderStyle.get());
        if (m_parentStyle->inheritedDataShared(cacheItem->parentRenderStyle.get()) && !isAtShadowBoundary(element)) {
            EInsideLink linkStatus = m_style->insideLink();
            // If the cache item parent style has identical inherited properties to the current parent style then the
            // resulting style will be identical too. We copy the inherited properties over from the cache and are done.
            m_style->inheritFrom(cacheItem->renderStyle.get());

            // Unfortunately the link status is treated like an inherited property. We need to explicitly restore it.
            m_style->setInsideLink(linkStatus);
            return;
        }
        applyInheritedOnly = true; 
    }

#if ENABLE(CSS_VARIABLES)
    // First apply all variable definitions, as they may be used during application of later properties.
    applyMatchedProperties<VariableDefinitions>(matchResult, false, 0, matchResult.matchedProperties.size() - 1, applyInheritedOnly);
    applyMatchedProperties<VariableDefinitions>(matchResult, true, matchResult.ranges.firstAuthorRule, matchResult.ranges.lastAuthorRule, applyInheritedOnly);
    applyMatchedProperties<VariableDefinitions>(matchResult, true, matchResult.ranges.firstUserRule, matchResult.ranges.lastUserRule, applyInheritedOnly);
    applyMatchedProperties<VariableDefinitions>(matchResult, true, matchResult.ranges.firstUARule, matchResult.ranges.lastUARule, applyInheritedOnly);
#endif

    // Now we have all of the matched rules in the appropriate order. Walk the rules and apply
    // high-priority properties first, i.e., those properties that other properties depend on.
    // The order is (1) high-priority not important, (2) high-priority important, (3) normal not important
    // and (4) normal important.
    m_lineHeightValue = 0;
    applyMatchedProperties<HighPriorityProperties>(matchResult, false, 0, matchResult.matchedProperties.size() - 1, applyInheritedOnly);
    applyMatchedProperties<HighPriorityProperties>(matchResult, true, matchResult.ranges.firstAuthorRule, matchResult.ranges.lastAuthorRule, applyInheritedOnly);
    applyMatchedProperties<HighPriorityProperties>(matchResult, true, matchResult.ranges.firstUserRule, matchResult.ranges.lastUserRule, applyInheritedOnly);
    applyMatchedProperties<HighPriorityProperties>(matchResult, true, matchResult.ranges.firstUARule, matchResult.ranges.lastUARule, applyInheritedOnly);

    if (cacheItem && cacheItem->renderStyle->effectiveZoom() != m_style->effectiveZoom()) {
        m_fontDirty = true;
        applyInheritedOnly = false;
    }

    // If our font got dirtied, go ahead and update it now.
    updateFont();

    // Line-height is set when we are sure we decided on the font-size.
    if (m_lineHeightValue)
        applyProperty(CSSPropertyLineHeight, m_lineHeightValue);

    // Many properties depend on the font. If it changes we just apply all properties.
    if (cacheItem && cacheItem->renderStyle->fontDescription() != m_style->fontDescription())
        applyInheritedOnly = false;

    // Now do the normal priority UA properties.
    applyMatchedProperties<LowPriorityProperties>(matchResult, false, matchResult.ranges.firstUARule, matchResult.ranges.lastUARule, applyInheritedOnly);
    
    // Cache our border and background so that we can examine them later.
    cacheBorderAndBackground();
    
    // Now do the author and user normal priority properties and all the !important properties.
    applyMatchedProperties<LowPriorityProperties>(matchResult, false, matchResult.ranges.lastUARule + 1, matchResult.matchedProperties.size() - 1, applyInheritedOnly);
    applyMatchedProperties<LowPriorityProperties>(matchResult, true, matchResult.ranges.firstAuthorRule, matchResult.ranges.lastAuthorRule, applyInheritedOnly);
    applyMatchedProperties<LowPriorityProperties>(matchResult, true, matchResult.ranges.firstUserRule, matchResult.ranges.lastUserRule, applyInheritedOnly);
    applyMatchedProperties<LowPriorityProperties>(matchResult, true, matchResult.ranges.firstUARule, matchResult.ranges.lastUARule, applyInheritedOnly);
   
    // Start loading resources referenced by this style.
    loadPendingResources();
    
    ASSERT(!m_fontDirty);
    
    if (cacheItem || !cacheHash)
        return;
    if (!isCacheableInMatchedPropertiesCache(m_element, m_style.get(), m_parentStyle))
        return;
    addToMatchedPropertiesCache(m_style.get(), m_parentStyle, cacheHash, matchResult);
}

static inline bool comparePageRules(const StyleRulePage* r1, const StyleRulePage* r2)
{
    return r1->selector()->specificity() < r2->selector()->specificity();
}

void StyleResolver::matchPageRules(MatchResult& result, RuleSet* rules, bool isLeftPage, bool isFirstPage, const String& pageName)
{
    if (!rules)
        return;

    Vector<StyleRulePage*> matchedPageRules;
    matchPageRulesForList(matchedPageRules, rules->pageRules(), isLeftPage, isFirstPage, pageName);
    if (matchedPageRules.isEmpty())
        return;

    std::stable_sort(matchedPageRules.begin(), matchedPageRules.end(), comparePageRules);

    for (unsigned i = 0; i < matchedPageRules.size(); i++)
        addMatchedProperties(result, matchedPageRules[i]->properties());
}

void StyleResolver::matchPageRulesForList(Vector<StyleRulePage*>& matchedRules, const Vector<StyleRulePage*>& rules, bool isLeftPage, bool isFirstPage, const String& pageName)
{
    unsigned size = rules.size();
    for (unsigned i = 0; i < size; ++i) {
        StyleRulePage* rule = rules[i];
        const AtomicString& selectorLocalName = rule->selector()->tag().localName();
        if (selectorLocalName != starAtom && selectorLocalName != pageName)
            continue;
        CSSSelector::PseudoType pseudoType = rule->selector()->pseudoType();
        if ((pseudoType == CSSSelector::PseudoLeftPage && !isLeftPage)
            || (pseudoType == CSSSelector::PseudoRightPage && isLeftPage)
            || (pseudoType == CSSSelector::PseudoFirstPage && !isFirstPage))
            continue;

        // If the rule has no properties to apply, then ignore it.
        const StylePropertySet* properties = rule->properties();
        if (!properties || properties->isEmpty())
            continue;

        // Add this rule to our list of matched rules.
        matchedRules.append(rule);
    }
}

bool StyleResolver::isLeftPage(int pageIndex) const
{
    bool isFirstPageLeft = false;
    if (!m_rootElementStyle->isLeftToRightDirection())
        isFirstPageLeft = true;

    return (pageIndex + (isFirstPageLeft ? 1 : 0)) % 2;
}

bool StyleResolver::isFirstPage(int pageIndex) const
{
    // FIXME: In case of forced left/right page, page at index 1 (not 0) can be the first page.
    return (!pageIndex);
}

String StyleResolver::pageName(int /* pageIndex */) const
{
    // FIXME: Implement page index to page name mapping.
    return "";
}

template <class ListType>
static void collectCSSOMWrappers(HashMap<StyleRule*, RefPtr<CSSStyleRule> >& wrapperMap, ListType* listType)
{
    if (!listType)
        return;
    unsigned size = listType->length();
    for (unsigned i = 0; i < size; ++i) {
        CSSRule* cssRule = listType->item(i);
        switch (cssRule->type()) {
        case CSSRule::IMPORT_RULE:
            collectCSSOMWrappers(wrapperMap, static_cast<CSSImportRule*>(cssRule)->styleSheet());
            break;
        case CSSRule::MEDIA_RULE:
            collectCSSOMWrappers(wrapperMap, static_cast<CSSMediaRule*>(cssRule));
            break;
#if ENABLE(CSS_REGIONS)
        case CSSRule::WEBKIT_REGION_RULE:
            collectCSSOMWrappers(wrapperMap, static_cast<WebKitCSSRegionRule*>(cssRule));
            break;
#endif
        case CSSRule::STYLE_RULE:
            wrapperMap.add(static_cast<CSSStyleRule*>(cssRule)->styleRule(), static_cast<CSSStyleRule*>(cssRule));
            break;
        default:
            break;
        }
    }
}

static void collectCSSOMWrappers(HashMap<StyleRule*, RefPtr<CSSStyleRule> >& wrapperMap, HashSet<RefPtr<CSSStyleSheet> >& sheetWrapperSet, StyleSheetContents* styleSheet)
{
    if (!styleSheet)
        return;
    RefPtr<CSSStyleSheet> styleSheetWrapper = CSSStyleSheet::create(styleSheet);
    sheetWrapperSet.add(styleSheetWrapper);
    collectCSSOMWrappers(wrapperMap, styleSheetWrapper.get());
}

static void collectCSSOMWrappers(HashMap<StyleRule*, RefPtr<CSSStyleRule> >& wrapperMap, const Vector<RefPtr<CSSStyleSheet> >& sheets)
{
    for (unsigned i = 0; i < sheets.size(); ++i)
        collectCSSOMWrappers(wrapperMap, sheets[i].get());
}

static void collectCSSOMWrappers(HashMap<StyleRule*, RefPtr<CSSStyleRule> >& wrapperMap, DocumentStyleSheetCollection* styleSheetCollection)
{
    collectCSSOMWrappers(wrapperMap, styleSheetCollection->activeAuthorStyleSheets());
    collectCSSOMWrappers(wrapperMap, styleSheetCollection->activeUserStyleSheets());
}

CSSStyleRule* StyleResolver::ensureFullCSSOMWrapperForInspector(StyleRule* rule)
{
    if (m_styleRuleToCSSOMWrapperMap.isEmpty()) {
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, simpleDefaultStyleSheet);
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, defaultStyleSheet);
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, quirksStyleSheet);
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, svgStyleSheet);
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, mathMLStyleSheet);
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, mediaControlsStyleSheet);
        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, m_styleSheetCSSOMWrapperSet, fullscreenStyleSheet);

        collectCSSOMWrappers(m_styleRuleToCSSOMWrapperMap, document()->styleSheetCollection());
    }
    return m_styleRuleToCSSOMWrapperMap.get(rule).get();
}

void StyleResolver::applyPropertyToStyle(CSSPropertyID id, CSSValue* value, RenderStyle* style)
{
    initElement(0);
    initForStyleResolve(0, style);
    m_style = style;
    applyPropertyToCurrentStyle(id, value);
}

void StyleResolver::applyPropertyToCurrentStyle(CSSPropertyID id, CSSValue* value)
{
    if (value)
        applyProperty(id, value);
}

inline bool isValidVisitedLinkProperty(CSSPropertyID id)
{
    switch (id) {
    case CSSPropertyBackgroundColor:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderTopColor:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyColor:
    case CSSPropertyOutlineColor:
    case CSSPropertyWebkitColumnRuleColor:
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextStrokeColor:
    // Also allow shorthands so that inherit/initial still work.
    case CSSPropertyBackground:
    case CSSPropertyBorderLeft:
    case CSSPropertyBorderRight:
    case CSSPropertyBorderTop:
    case CSSPropertyBorderBottom:
    case CSSPropertyOutline:
    case CSSPropertyWebkitColumnRule:
#if ENABLE(SVG)
    case CSSPropertyFill:
    case CSSPropertyStroke:
#endif
        return true;
    default:
        break;
    }

    return false;
}

// http://dev.w3.org/csswg/css3-regions/#the-at-region-style-rule
// FIXME: add incremental support for other region styling properties.
inline bool StyleResolver::isValidRegionStyleProperty(CSSPropertyID id)
{
    switch (id) {
    case CSSPropertyBackgroundColor:
    case CSSPropertyColor:
        return true;
    default:
        break;
    }

    return false;
}

// SVG handles zooming in a different way compared to CSS. The whole document is scaled instead
// of each individual length value in the render style / tree. CSSPrimitiveValue::computeLength*()
// multiplies each resolved length with the zoom multiplier - so for SVG we need to disable that.
// Though all CSS values that can be applied to outermost <svg> elements (width/height/border/padding...)
// need to respect the scaling. RenderBox (the parent class of RenderSVGRoot) grabs values like
// width/height/border/padding/... from the RenderStyle -> for SVG these values would never scale,
// if we'd pass a 1.0 zoom factor everyhwere. So we only pass a zoom factor of 1.0 for specific
// properties that are NOT allowed to scale within a zoomed SVG document (letter/word-spacing/font-size).
bool StyleResolver::useSVGZoomRules()
{
    return m_element && m_element->isSVGElement();
}

static bool createGridTrackBreadth(CSSPrimitiveValue* primitiveValue, StyleResolver* selector, GridTrackSize& trackSize)
{
    Length workingLength = primitiveValue->convertToLength<FixedIntegerConversion | PercentConversion | ViewportPercentageConversion | AutoConversion>(selector->style(), selector->rootElementStyle(), selector->style()->effectiveZoom());
    if (workingLength.isUndefined())
        return false;

    if (primitiveValue->isLength())
        workingLength.setQuirk(primitiveValue->isQuirkValue());

    trackSize.setLength(workingLength);
    return true;
}

static bool createGridTrackList(CSSValue* value, Vector<GridTrackSize>& trackSizes, StyleResolver* selector)
{
    // Handle 'none'.
    if (value->isPrimitiveValue()) {
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        return primitiveValue->getIdent() == CSSValueNone;
    }

    if (value->isValueList()) {
        for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
            CSSValue* currValue = i.value();
            if (!currValue->isPrimitiveValue())
                return false;

            GridTrackSize trackSize;
            if (!createGridTrackBreadth(static_cast<CSSPrimitiveValue*>(currValue), selector, trackSize))
                return false;

            trackSizes.append(trackSize);
        }
        return true;
    }

    return false;
}


static bool createGridPosition(CSSValue* value, GridPosition& position)
{
    // For now, we only accept: <integer> | 'auto'
    if (!value->isPrimitiveValue())
        return false;

    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
    if (primitiveValue->getIdent() == CSSValueAuto)
        return true;

    ASSERT(primitiveValue->isNumber());
    position.setIntegerPosition(primitiveValue->getIntValue());
    return true;
}

#if ENABLE(CSS_VARIABLES)
static bool hasVariableReference(CSSValue* value)
{
    if (value->isPrimitiveValue()) {
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        return primitiveValue->hasVariableReference();
    }

    if (value->isCalculationValue())
        return static_cast<CSSCalcValue*>(value)->hasVariableReference();

    for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
        if (hasVariableReference(i.value()))
            return true;
    }

    return false;
}

void StyleResolver::resolveVariables(CSSPropertyID id, CSSValue* value, Vector<std::pair<CSSPropertyID, String> >& knownExpressions)
{
    std::pair<CSSPropertyID, String> expression(id, value->serializeResolvingVariables(*style()->variables()));

    if (knownExpressions.contains(expression))
        return; // cycle detected.

    knownExpressions.append(expression);

    // FIXME: It would be faster not to re-parse from strings, but for now CSS property validation lives inside the parser so we do it there.
    RefPtr<StylePropertySet> resultSet = StylePropertySet::create();
    if (!CSSParser::parseValue(resultSet.get(), id, expression.second, false, document()))
        return; // expression failed to parse.

    for (unsigned i = 0; i < resultSet->propertyCount(); i++) {
        StylePropertySet::PropertyReference property = resultSet->propertyAt(i);
        if (property.id() != CSSPropertyVariable && hasVariableReference(property.value()))
            resolveVariables(property.id(), property.value(), knownExpressions);
        else
            applyProperty(property.id(), property.value());
    }
}
#endif

void StyleResolver::applyProperty(CSSPropertyID id, CSSValue* value)
{
#if ENABLE(CSS_VARIABLES)
    if (id != CSSPropertyVariable && hasVariableReference(value)) {
        Vector<std::pair<CSSPropertyID, String> > knownExpressions;
        resolveVariables(id, value, knownExpressions);
        return;
    }
#endif

    bool isInherit = m_parentNode && value->isInheritedValue();
    bool isInitial = value->isInitialValue() || (!m_parentNode && value->isInheritedValue());

    ASSERT(!isInherit || !isInitial); // isInherit -> !isInitial && isInitial -> !isInherit
    ASSERT(!isInherit || (m_parentNode && m_parentStyle)); // isInherit -> (m_parentNode && m_parentStyle)

    if (!applyPropertyToRegularStyle() && (!applyPropertyToVisitedLinkStyle() || !isValidVisitedLinkProperty(id))) {
        // Limit the properties that can be applied to only the ones honored by :visited.
        return;
    }

    if (isInherit && !m_parentStyle->hasExplicitlyInheritedProperties() && !CSSProperty::isInheritedProperty(id))
        m_parentStyle->setHasExplicitlyInheritedProperties();

#if ENABLE(CSS_VARIABLES)
    if (id == CSSPropertyVariable) {
        ASSERT(value->isVariableValue());
        CSSVariableValue* variable = static_cast<CSSVariableValue*>(value);
        ASSERT(!variable->name().isEmpty());
        ASSERT(!variable->value().isEmpty());
        m_style->setVariable(variable->name(), variable->value());
        return;
    }
#endif

    // Check lookup table for implementations and use when available.
    const PropertyHandler& handler = m_styleBuilder.propertyHandler(id);
    if (handler.isValid()) {
        if (isInherit)
            handler.applyInheritValue(id, this);
        else if (isInitial)
            handler.applyInitialValue(id, this);
        else
            handler.applyValue(id, this, value);
        return;
    }

    CSSPrimitiveValue* primitiveValue = value->isPrimitiveValue() ? static_cast<CSSPrimitiveValue*>(value) : 0;

    float zoomFactor = m_style->effectiveZoom();

    // What follows is a list that maps the CSS properties into their corresponding front-end
    // RenderStyle values. Shorthands (e.g. border, background) occur in this list as well and
    // are only hit when mapping "inherit" or "initial" into front-end values.
    switch (id) {
    // lists
    case CSSPropertyContent:
        // list of string, uri, counter, attr, i
        {
            // FIXME: In CSS3, it will be possible to inherit content. In CSS2 it is not. This
            // note is a reminder that eventually "inherit" needs to be supported.

            if (isInitial) {
                m_style->clearContent();
                return;
            }

            if (!value->isValueList())
                return;

            bool didSet = false;
            for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
                CSSValue* item = i.value();
                if (item->isImageGeneratorValue()) {
                    if (item->isGradientValue())
                        m_style->setContent(StyleGeneratedImage::create(static_cast<CSSGradientValue*>(item)->gradientWithStylesResolved(this).get()), didSet);
                    else
                        m_style->setContent(StyleGeneratedImage::create(static_cast<CSSImageGeneratorValue*>(item)), didSet);
                    didSet = true;
#if ENABLE(CSS_IMAGE_SET)
                } else if (item->isImageSetValue()) {
                    m_style->setContent(setOrPendingFromValue(CSSPropertyContent, static_cast<CSSImageSetValue*>(item)), didSet);
                    didSet = true;
#endif
                }

                if (item->isImageValue()) {
                    m_style->setContent(cachedOrPendingFromValue(CSSPropertyContent, static_cast<CSSImageValue*>(item)), didSet);
                    didSet = true;
                    continue;
                }

                if (!item->isPrimitiveValue())
                    continue;

                CSSPrimitiveValue* contentValue = static_cast<CSSPrimitiveValue*>(item);

                if (contentValue->isString()) {
                    m_style->setContent(contentValue->getStringValue().impl(), didSet);
                    didSet = true;
                } else if (contentValue->isAttr()) {
                    // FIXME: Can a namespace be specified for an attr(foo)?
                    if (m_style->styleType() == NOPSEUDO)
                        m_style->setUnique();
                    else
                        m_parentStyle->setUnique();
                    QualifiedName attr(nullAtom, contentValue->getStringValue().impl(), nullAtom);
                    const AtomicString& value = m_element->getAttribute(attr);
                    m_style->setContent(value.isNull() ? emptyAtom : value.impl(), didSet);
                    didSet = true;
                    // register the fact that the attribute value affects the style
                    m_features.attrsInRules.add(attr.localName().impl());
                } else if (contentValue->isCounter()) {
                    Counter* counterValue = contentValue->getCounterValue();
                    EListStyleType listStyleType = NoneListStyle;
                    int listStyleIdent = counterValue->listStyleIdent();
                    if (listStyleIdent != CSSValueNone)
                        listStyleType = static_cast<EListStyleType>(listStyleIdent - CSSValueDisc);
                    OwnPtr<CounterContent> counter = adoptPtr(new CounterContent(counterValue->identifier(), listStyleType, counterValue->separator()));
                    m_style->setContent(counter.release(), didSet);
                    didSet = true;
                } else {
                    switch (contentValue->getIdent()) {
                    case CSSValueOpenQuote:
                        m_style->setContent(OPEN_QUOTE, didSet);
                        didSet = true;
                        break;
                    case CSSValueCloseQuote:
                        m_style->setContent(CLOSE_QUOTE, didSet);
                        didSet = true;
                        break;
                    case CSSValueNoOpenQuote:
                        m_style->setContent(NO_OPEN_QUOTE, didSet);
                        didSet = true;
                        break;
                    case CSSValueNoCloseQuote:
                        m_style->setContent(NO_CLOSE_QUOTE, didSet);
                        didSet = true;
                        break;
                    default:
                        // normal and none do not have any effect.
                        { }
                    }
                }
            }
            if (!didSet)
                m_style->clearContent();
            return;
        }
    case CSSPropertyQuotes:
        if (isInherit) {
            m_style->setQuotes(m_parentStyle->quotes());
            return;
        }
        if (isInitial) {
            m_style->setQuotes(0);
            return;
        }
        if (value->isValueList()) {
            CSSValueList* list = static_cast<CSSValueList*>(value);
            RefPtr<QuotesData> quotes = QuotesData::create();
            for (size_t i = 0; i < list->length(); i += 2) {
                CSSValue* first = list->itemWithoutBoundsCheck(i);
                // item() returns null if out of bounds so this is safe.
                CSSValue* second = list->item(i + 1);
                if (!second)
                    continue;
                ASSERT(first->isPrimitiveValue());
                ASSERT(second->isPrimitiveValue());
                String startQuote = static_cast<CSSPrimitiveValue*>(first)->getStringValue();
                String endQuote = static_cast<CSSPrimitiveValue*>(second)->getStringValue();
                quotes->addPair(std::make_pair(startQuote, endQuote));
            }
            m_style->setQuotes(quotes);
            return;
        }
        if (primitiveValue) {
            if (primitiveValue->getIdent() == CSSValueNone)
                m_style->setQuotes(QuotesData::create());
        }
        return;
    case CSSPropertyFontFamily: {
        // list of strings and ids
        if (isInherit) {
            FontDescription parentFontDescription = m_parentStyle->fontDescription();
            FontDescription fontDescription = m_style->fontDescription();
            fontDescription.setGenericFamily(parentFontDescription.genericFamily());
            fontDescription.setFamily(parentFontDescription.firstFamily());
            fontDescription.setIsSpecifiedFont(parentFontDescription.isSpecifiedFont());
            setFontDescription(fontDescription);
            return;
        }

        if (isInitial) {
            FontDescription initialDesc = FontDescription();
            FontDescription fontDescription = m_style->fontDescription();
            // We need to adjust the size to account for the generic family change from monospace
            // to non-monospace.
            if (fontDescription.keywordSize() && fontDescription.useFixedDefaultSize())
                setFontSize(fontDescription, fontSizeForKeyword(m_checker.document(), CSSValueXxSmall + fontDescription.keywordSize() - 1, false));
            fontDescription.setGenericFamily(initialDesc.genericFamily());
            if (!initialDesc.firstFamily().familyIsEmpty())
                fontDescription.setFamily(initialDesc.firstFamily());
            setFontDescription(fontDescription);
            return;
        }

        if (!value->isValueList())
            return;
        FontDescription fontDescription = m_style->fontDescription();
        FontFamily& firstFamily = fontDescription.firstFamily();
        FontFamily* currFamily = 0;

        // Before mapping in a new font-family property, we should reset the generic family.
        bool oldFamilyUsedFixedDefaultSize = fontDescription.useFixedDefaultSize();
        fontDescription.setGenericFamily(FontDescription::NoFamily);

        for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
            CSSValue* item = i.value();
            if (!item->isPrimitiveValue())
                continue;
            CSSPrimitiveValue* contentValue = static_cast<CSSPrimitiveValue*>(item);
            AtomicString face;
            Settings* settings = documentSettings();
            if (contentValue->isString())
                face = contentValue->getStringValue();
            else if (settings) {
                switch (contentValue->getIdent()) {
                case CSSValueWebkitBody:
                    face = settings->standardFontFamily();
                    break;
                case CSSValueSerif:
                    face = serifFamily;
                    fontDescription.setGenericFamily(FontDescription::SerifFamily);
                    break;
                case CSSValueSansSerif:
                    face = sansSerifFamily;
                    fontDescription.setGenericFamily(FontDescription::SansSerifFamily);
                    break;
                case CSSValueCursive:
                    face = cursiveFamily;
                    fontDescription.setGenericFamily(FontDescription::CursiveFamily);
                    break;
                case CSSValueFantasy:
                    face = fantasyFamily;
                    fontDescription.setGenericFamily(FontDescription::FantasyFamily);
                    break;
                case CSSValueMonospace:
                    face = monospaceFamily;
                    fontDescription.setGenericFamily(FontDescription::MonospaceFamily);
                    break;
                case CSSValueWebkitPictograph:
                    face = pictographFamily;
                    fontDescription.setGenericFamily(FontDescription::PictographFamily);
                    break;
                }
            }

            if (!face.isEmpty()) {
                if (!currFamily) {
                    // Filling in the first family.
                    firstFamily.setFamily(face);
                    firstFamily.appendFamily(0); // Remove any inherited family-fallback list.
                    currFamily = &firstFamily;
                    fontDescription.setIsSpecifiedFont(fontDescription.genericFamily() == FontDescription::NoFamily);
                } else {
                    RefPtr<SharedFontFamily> newFamily = SharedFontFamily::create();
                    newFamily->setFamily(face);
                    currFamily->appendFamily(newFamily);
                    currFamily = newFamily.get();
                }
            }
        }

        // We can't call useFixedDefaultSize() until all new font families have been added
        // If currFamily is non-zero then we set at least one family on this description.
        if (currFamily) {
            if (fontDescription.keywordSize() && fontDescription.useFixedDefaultSize() != oldFamilyUsedFixedDefaultSize)
                setFontSize(fontDescription, fontSizeForKeyword(m_checker.document(), CSSValueXxSmall + fontDescription.keywordSize() - 1, !oldFamilyUsedFixedDefaultSize));

            setFontDescription(fontDescription);
        }
        return;
    }
// shorthand properties
    case CSSPropertyBackground:
        if (isInitial) {
            m_style->clearBackgroundLayers();
            m_style->setBackgroundColor(Color());
        } else if (isInherit) {
            m_style->inheritBackgroundLayers(*m_parentStyle->backgroundLayers());
            m_style->setBackgroundColor(m_parentStyle->backgroundColor());
        }
        return;
    case CSSPropertyWebkitMask:
        if (isInitial)
            m_style->clearMaskLayers();
        else if (isInherit)
            m_style->inheritMaskLayers(*m_parentStyle->maskLayers());
        return;
    case CSSPropertyFont:
        if (isInherit) {
            FontDescription fontDescription = m_parentStyle->fontDescription();
            m_style->setLineHeight(m_parentStyle->specifiedLineHeight());
            m_lineHeightValue = 0;
            setFontDescription(fontDescription);
        } else if (isInitial) {
            Settings* settings = documentSettings();
            ASSERT(settings); // If we're doing style resolution, this document should always be in a frame and thus have settings
            if (!settings)
                return;
            initializeFontStyle(settings);
        } else if (primitiveValue) {
            m_style->setLineHeight(RenderStyle::initialLineHeight());
            m_lineHeightValue = 0;

            FontDescription fontDescription;
            RenderTheme::defaultTheme()->systemFont(primitiveValue->getIdent(), fontDescription);

            // Double-check and see if the theme did anything. If not, don't bother updating the font.
            if (fontDescription.isAbsoluteSize()) {
                // Make sure the rendering mode and printer font settings are updated.
                Settings* settings = documentSettings();
                ASSERT(settings); // If we're doing style resolution, this document should always be in a frame and thus have settings
                if (!settings)
                    return;
                fontDescription.setRenderingMode(settings->fontRenderingMode());
                fontDescription.setUsePrinterFont(m_checker.document()->printing() || !settings->screenFontSubstitutionEnabled());

                // Handle the zoom factor.
                fontDescription.setComputedSize(getComputedSizeFromSpecifiedSize(m_checker.document(), m_style.get(), fontDescription.isAbsoluteSize(), fontDescription.specifiedSize(), useSVGZoomRules()));
                setFontDescription(fontDescription);
            }
        } else if (value->isFontValue()) {
            FontValue* font = static_cast<FontValue*>(value);
            if (!font->style || !font->variant || !font->weight
                || !font->size || !font->lineHeight || !font->family)
                return;
            applyProperty(CSSPropertyFontStyle, font->style.get());
            applyProperty(CSSPropertyFontVariant, font->variant.get());
            applyProperty(CSSPropertyFontWeight, font->weight.get());
            // The previous properties can dirty our font but they don't try to read the font's
            // properties back, which is safe. However if font-size is using the 'ex' unit, it will
            // need query the dirtied font's x-height to get the computed size. To be safe in this
            // case, let's just update the font now.
            updateFont();
            applyProperty(CSSPropertyFontSize, font->size.get());

            m_lineHeightValue = font->lineHeight.get();

            applyProperty(CSSPropertyFontFamily, font->family.get());
        }
        return;

    // CSS3 Properties
    case CSSPropertyTextShadow:
    case CSSPropertyBoxShadow:
    case CSSPropertyWebkitBoxShadow: {
        if (isInherit) {
            if (id == CSSPropertyTextShadow)
                return m_style->setTextShadow(m_parentStyle->textShadow() ? adoptPtr(new ShadowData(*m_parentStyle->textShadow())) : nullptr);
            return m_style->setBoxShadow(m_parentStyle->boxShadow() ? adoptPtr(new ShadowData(*m_parentStyle->boxShadow())) : nullptr);
        }
        if (isInitial || primitiveValue) // initial | none
            return id == CSSPropertyTextShadow ? m_style->setTextShadow(nullptr) : m_style->setBoxShadow(nullptr);

        if (!value->isValueList())
            return;

        for (CSSValueListIterator i = value; i.hasMore(); i.advance()) {
            CSSValue* currValue = i.value();
            if (!currValue->isShadowValue())
                continue;
            ShadowValue* item = static_cast<ShadowValue*>(currValue);
            int x = item->x->computeLength<int>(style(), m_rootElementStyle, zoomFactor);
            int y = item->y->computeLength<int>(style(), m_rootElementStyle, zoomFactor);
            int blur = item->blur ? item->blur->computeLength<int>(style(), m_rootElementStyle, zoomFactor) : 0;
            int spread = item->spread ? item->spread->computeLength<int>(style(), m_rootElementStyle, zoomFactor) : 0;
            ShadowStyle shadowStyle = item->style && item->style->getIdent() == CSSValueInset ? Inset : Normal;
            Color color;
            if (item->color)
                color = colorFromPrimitiveValue(item->color.get());
            else if (m_style)
                color = m_style->color();

            OwnPtr<ShadowData> shadowData = adoptPtr(new ShadowData(IntPoint(x, y), blur, spread, shadowStyle, id == CSSPropertyWebkitBoxShadow, color.isValid() ? color : Color::transparent));
            if (id == CSSPropertyTextShadow)
                m_style->setTextShadow(shadowData.release(), i.index()); // add to the list if this is not the first entry
            else
                m_style->setBoxShadow(shadowData.release(), i.index()); // add to the list if this is not the first entry
        }
        return;
    }
    case CSSPropertyWebkitBoxReflect: {
        HANDLE_INHERIT_AND_INITIAL(boxReflect, BoxReflect)
        if (primitiveValue) {
            m_style->setBoxReflect(RenderStyle::initialBoxReflect());
            return;
        }

        if (!value->isReflectValue())
            return;

        CSSReflectValue* reflectValue = static_cast<CSSReflectValue*>(value);
        RefPtr<StyleReflection> reflection = StyleReflection::create();
        reflection->setDirection(reflectValue->direction());
        if (reflectValue->offset())
            reflection->setOffset(reflectValue->offset()->convertToLength<FixedIntegerConversion | PercentConversion | CalculatedConversion>(style(), m_rootElementStyle, zoomFactor));
        NinePieceImage mask;
        mask.setMaskDefaults();
        m_styleMap.mapNinePieceImage(id, reflectValue->mask(), mask);
        reflection->setMask(mask);

        m_style->setBoxReflect(reflection.release());
        return;
    }
    case CSSPropertySrc: // Only used in @font-face rules.
        return;
    case CSSPropertyUnicodeRange: // Only used in @font-face rules.
        return;
    case CSSPropertyWebkitColumnRule:
        if (isInherit) {
            m_style->setColumnRuleColor(m_parentStyle->columnRuleColor().isValid() ? m_parentStyle->columnRuleColor() : m_parentStyle->color());
            m_style->setColumnRuleStyle(m_parentStyle->columnRuleStyle());
            m_style->setColumnRuleWidth(m_parentStyle->columnRuleWidth());
        } else if (isInitial)
            m_style->resetColumnRule();
        return;
    case CSSPropertyWebkitMarquee:
        if (!isInherit)
            return;
        m_style->setMarqueeDirection(m_parentStyle->marqueeDirection());
        m_style->setMarqueeIncrement(m_parentStyle->marqueeIncrement());
        m_style->setMarqueeSpeed(m_parentStyle->marqueeSpeed());
        m_style->setMarqueeLoopCount(m_parentStyle->marqueeLoopCount());
        m_style->setMarqueeBehavior(m_parentStyle->marqueeBehavior());
        return;
    case CSSPropertyWebkitMarqueeRepetition: {
        HANDLE_INHERIT_AND_INITIAL(marqueeLoopCount, MarqueeLoopCount)
        if (!primitiveValue)
            return;
        if (primitiveValue->getIdent() == CSSValueInfinite)
            m_style->setMarqueeLoopCount(-1); // -1 means repeat forever.
        else if (primitiveValue->isNumber())
            m_style->setMarqueeLoopCount(primitiveValue->getIntValue());
        return;
    }
    case CSSPropertyWebkitMarqueeSpeed: {
        HANDLE_INHERIT_AND_INITIAL(marqueeSpeed, MarqueeSpeed)
        if (!primitiveValue)
            return;
        if (int ident = primitiveValue->getIdent()) {
            switch (ident) {
            case CSSValueSlow:
                m_style->setMarqueeSpeed(500); // 500 msec.
                break;
            case CSSValueNormal:
                m_style->setMarqueeSpeed(85); // 85msec. The WinIE default.
                break;
            case CSSValueFast:
                m_style->setMarqueeSpeed(10); // 10msec. Super fast.
                break;
            }
        } else if (primitiveValue->isTime())
            m_style->setMarqueeSpeed(primitiveValue->computeTime<int, CSSPrimitiveValue::Milliseconds>());
        else if (primitiveValue->isNumber()) // For scrollamount support.
            m_style->setMarqueeSpeed(primitiveValue->getIntValue());
        return;
    }
    case CSSPropertyWebkitMarqueeIncrement: {
        HANDLE_INHERIT_AND_INITIAL(marqueeIncrement, MarqueeIncrement)
        if (!primitiveValue)
            return;
        if (primitiveValue->getIdent()) {
            switch (primitiveValue->getIdent()) {
            case CSSValueSmall:
                m_style->setMarqueeIncrement(Length(1, Fixed)); // 1px.
                break;
            case CSSValueNormal:
                m_style->setMarqueeIncrement(Length(6, Fixed)); // 6px. The WinIE default.
                break;
            case CSSValueLarge:
                m_style->setMarqueeIncrement(Length(36, Fixed)); // 36px.
                break;
            }
        } else {
            Length marqueeLength = convertToIntLength(primitiveValue, style(), m_rootElementStyle);
            if (!marqueeLength.isUndefined())
                m_style->setMarqueeIncrement(marqueeLength);
        }
        return;
    }
    case CSSPropertyWebkitLocale: {
        HANDLE_INHERIT_AND_INITIAL(locale, Locale);
        if (!primitiveValue)
            return;
        if (primitiveValue->getIdent() == CSSValueAuto)
            m_style->setLocale(nullAtom);
        else
            m_style->setLocale(primitiveValue->getStringValue());
        FontDescription fontDescription = m_style->fontDescription();
        fontDescription.setScript(localeToScriptCodeForFontSelection(m_style->locale()));
        setFontDescription(fontDescription);
        return;
    }
    case CSSPropertyWebkitTextSizeAdjust: {
        HANDLE_INHERIT_AND_INITIAL(textSizeAdjust, TextSizeAdjust)
        if (!primitiveValue || !primitiveValue->getIdent())
            return;
        setTextSizeAdjust(primitiveValue->getIdent() == CSSValueAuto);
        return;
    }
#if ENABLE(DASHBOARD_SUPPORT)
    case CSSPropertyWebkitDashboardRegion:
    {
        HANDLE_INHERIT_AND_INITIAL(dashboardRegions, DashboardRegions)
        if (!primitiveValue)
            return;

        if (primitiveValue->getIdent() == CSSValueNone) {
            m_style->setDashboardRegions(RenderStyle::noneDashboardRegions());
            return;
        }

        DashboardRegion* region = primitiveValue->getDashboardRegionValue();
        if (!region)
            return;

        DashboardRegion* first = region;
        while (region) {
            Length top = convertToIntLength(region->top(), style(), m_rootElementStyle);
            Length right = convertToIntLength(region->right(), style(), m_rootElementStyle);
            Length bottom = convertToIntLength(region->bottom(), style(), m_rootElementStyle);
            Length left = convertToIntLength(region->left(), style(), m_rootElementStyle);

            if (top.isUndefined())
                top = Length();
            if (right.isUndefined())
                right = Length();
            if (bottom.isUndefined())
                bottom = Length();
            if (left.isUndefined())
                left = Length();

            if (region->m_isCircle)
                m_style->setDashboardRegion(StyleDashboardRegion::Circle, region->m_label, top, right, bottom, left, region == first ? false : true);
            else if (region->m_isRectangle)
                m_style->setDashboardRegion(StyleDashboardRegion::Rectangle, region->m_label, top, right, bottom, left, region == first ? false : true);
            region = region->m_next.get();
        }

        m_element->document()->setHasAnnotatedRegions(true);

        return;
    }
#endif
#if ENABLE(DRAGGABLE_REGION)
    case CSSPropertyWebkitAppRegion: {
        if (!primitiveValue || !primitiveValue->getIdent())
            return;
        m_style->setDraggableRegionMode(primitiveValue->getIdent() == CSSValueDrag ? DraggableRegionDrag : DraggableRegionNoDrag);
        m_element->document()->setHasAnnotatedRegions(true);
        return;
    }
#endif
    case CSSPropertyWebkitTextStrokeWidth: {
        HANDLE_INHERIT_AND_INITIAL(textStrokeWidth, TextStrokeWidth)
        float width = 0;
        switch (primitiveValue->getIdent()) {
        case CSSValueThin:
        case CSSValueMedium:
        case CSSValueThick: {
            double result = 1.0 / 48;
            if (primitiveValue->getIdent() == CSSValueMedium)
                result *= 3;
            else if (primitiveValue->getIdent() == CSSValueThick)
                result *= 5;
            width = CSSPrimitiveValue::create(result, CSSPrimitiveValue::CSS_EMS)->computeLength<float>(style(), m_rootElementStyle, zoomFactor);
            break;
        }
        default:
            width = primitiveValue->computeLength<float>(style(), m_rootElementStyle, zoomFactor);
            break;
        }
        m_style->setTextStrokeWidth(width);
        return;
    }
    case CSSPropertyWebkitTransform: {
        HANDLE_INHERIT_AND_INITIAL(transform, Transform);
        TransformOperations operations;
        createTransformOperations(value, style(), m_rootElementStyle, operations);
        m_style->setTransform(operations);
        return;
    }
    case CSSPropertyWebkitPerspective: {
        HANDLE_INHERIT_AND_INITIAL(perspective, Perspective)

        if (!primitiveValue)
            return;

        if (primitiveValue->getIdent() == CSSValueNone) {
            m_style->setPerspective(0);
            return;
        }

        float perspectiveValue;
        if (primitiveValue->isLength())
            perspectiveValue = primitiveValue->computeLength<float>(style(), m_rootElementStyle, zoomFactor);
        else if (primitiveValue->isNumber()) {
            // For backward compatibility, treat valueless numbers as px.
            perspectiveValue = CSSPrimitiveValue::create(primitiveValue->getDoubleValue(), CSSPrimitiveValue::CSS_PX)->computeLength<float>(style(), m_rootElementStyle, zoomFactor);
        } else
            return;

        if (perspectiveValue >= 0.0f)
            m_style->setPerspective(perspectiveValue);
        return;
    }
    case CSSPropertyWebkitAnimation:
        if (isInitial)
            m_style->clearAnimations();
        else if (isInherit)
            m_style->inheritAnimations(m_parentStyle->animations());
        return;
    case CSSPropertyWebkitTransition:
        if (isInitial)
            m_style->clearTransitions();
        else if (isInherit)
            m_style->inheritTransitions(m_parentStyle->transitions());
        return;
#if ENABLE(TOUCH_EVENTS)
    case CSSPropertyWebkitTapHighlightColor: {
        HANDLE_INHERIT_AND_INITIAL(tapHighlightColor, TapHighlightColor);
        if (!primitiveValue)
            break;

        Color col = colorFromPrimitiveValue(primitiveValue);
        m_style->setTapHighlightColor(col);
        return;
    }
#endif
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING)
    case CSSPropertyWebkitOverflowScrolling: {
        HANDLE_INHERIT_AND_INITIAL(useTouchOverflowScrolling, UseTouchOverflowScrolling);
        if (!primitiveValue)
            break;
        m_style->setUseTouchOverflowScrolling(primitiveValue->getIdent() == CSSValueTouch);
        return;
    }
#endif
    case CSSPropertyInvalid:
        return;
    // Directional properties are resolved by resolveDirectionAwareProperty() before the switch.
    case CSSPropertyWebkitBorderEnd:
    case CSSPropertyWebkitBorderEndColor:
    case CSSPropertyWebkitBorderEndStyle:
    case CSSPropertyWebkitBorderEndWidth:
    case CSSPropertyWebkitBorderStart:
    case CSSPropertyWebkitBorderStartColor:
    case CSSPropertyWebkitBorderStartStyle:
    case CSSPropertyWebkitBorderStartWidth:
    case CSSPropertyWebkitBorderBefore:
    case CSSPropertyWebkitBorderBeforeColor:
    case CSSPropertyWebkitBorderBeforeStyle:
    case CSSPropertyWebkitBorderBeforeWidth:
    case CSSPropertyWebkitBorderAfter:
    case CSSPropertyWebkitBorderAfterColor:
    case CSSPropertyWebkitBorderAfterStyle:
    case CSSPropertyWebkitBorderAfterWidth:
    case CSSPropertyWebkitMarginEnd:
    case CSSPropertyWebkitMarginStart:
    case CSSPropertyWebkitMarginBefore:
    case CSSPropertyWebkitMarginAfter:
    case CSSPropertyWebkitMarginCollapse:
    case CSSPropertyWebkitMarginBeforeCollapse:
    case CSSPropertyWebkitMarginTopCollapse:
    case CSSPropertyWebkitMarginAfterCollapse:
    case CSSPropertyWebkitMarginBottomCollapse:
    case CSSPropertyWebkitPaddingEnd:
    case CSSPropertyWebkitPaddingStart:
    case CSSPropertyWebkitPaddingBefore:
    case CSSPropertyWebkitPaddingAfter:
    case CSSPropertyWebkitLogicalWidth:
    case CSSPropertyWebkitLogicalHeight:
    case CSSPropertyWebkitMinLogicalWidth:
    case CSSPropertyWebkitMinLogicalHeight:
    case CSSPropertyWebkitMaxLogicalWidth:
    case CSSPropertyWebkitMaxLogicalHeight:
    {
        CSSPropertyID newId = CSSProperty::resolveDirectionAwareProperty(id, m_style->direction(), m_style->writingMode());
        ASSERT(newId != id);
        return applyProperty(newId, value);
    }
    case CSSPropertyFontStretch:
    case CSSPropertyPage:
    case CSSPropertyTextLineThrough:
    case CSSPropertyTextLineThroughColor:
    case CSSPropertyTextLineThroughMode:
    case CSSPropertyTextLineThroughStyle:
    case CSSPropertyTextLineThroughWidth:
    case CSSPropertyTextOverline:
    case CSSPropertyTextOverlineColor:
    case CSSPropertyTextOverlineMode:
    case CSSPropertyTextOverlineStyle:
    case CSSPropertyTextOverlineWidth:
    case CSSPropertyTextUnderline:
    case CSSPropertyTextUnderlineColor:
    case CSSPropertyTextUnderlineMode:
    case CSSPropertyTextUnderlineStyle:
    case CSSPropertyTextUnderlineWidth:
    case CSSPropertyWebkitFontSizeDelta:
    case CSSPropertyWebkitTextDecorationsInEffect:
    case CSSPropertyWebkitTextStroke:
    case CSSPropertyWebkitTextEmphasis:
        return;

    // CSS Text Layout Module Level 3: Vertical writing support
    case CSSPropertyWebkitWritingMode: {
        HANDLE_INHERIT_AND_INITIAL(writingMode, WritingMode);
        
        if (primitiveValue)
            m_style->setWritingMode(*primitiveValue);
        
        // FIXME: It is not ok to modify document state while applying style.
        if (m_element && m_element == m_element->document()->documentElement())
            m_element->document()->setWritingModeSetOnDocumentElement(true);
        FontDescription fontDescription = m_style->fontDescription();
        fontDescription.setOrientation(m_style->isHorizontalWritingMode() ? Horizontal : Vertical);
        setFontDescription(fontDescription);
        return;
    }

    case CSSPropertyWebkitLineBoxContain: {
        HANDLE_INHERIT_AND_INITIAL(lineBoxContain, LineBoxContain)
        if (primitiveValue && primitiveValue->getIdent() == CSSValueNone) {
            m_style->setLineBoxContain(LineBoxContainNone);
            return;
        }

        if (!value->isCSSLineBoxContainValue())
            return;

        CSSLineBoxContainValue* lineBoxContainValue = static_cast<CSSLineBoxContainValue*>(value);
        m_style->setLineBoxContain(lineBoxContainValue->value());
        return;
    }

    // CSS Fonts Module Level 3
    case CSSPropertyWebkitFontFeatureSettings: {
        if (primitiveValue && primitiveValue->getIdent() == CSSValueNormal) {
            setFontDescription(m_style->fontDescription().makeNormalFeatureSettings());
            return;
        }

        if (!value->isValueList())
            return;

        FontDescription fontDescription = m_style->fontDescription();
        CSSValueList* list = static_cast<CSSValueList*>(value);
        RefPtr<FontFeatureSettings> settings = FontFeatureSettings::create();
        int len = list->length();
        for (int i = 0; i < len; ++i) {
            CSSValue* item = list->itemWithoutBoundsCheck(i);
            if (!item->isFontFeatureValue())
                continue;
            FontFeatureValue* feature = static_cast<FontFeatureValue*>(item);
            settings->append(FontFeature(feature->tag(), feature->value()));
        }
        fontDescription.setFeatureSettings(settings.release());
        setFontDescription(fontDescription);
        return;
    }

#if ENABLE(CSS_FILTERS)
    case CSSPropertyWebkitFilter: {
        HANDLE_INHERIT_AND_INITIAL(filter, Filter);
        FilterOperations operations;
        if (createFilterOperations(value, style(), m_rootElementStyle, operations))
            m_style->setFilter(operations);
        return;
    }
#endif
    case CSSPropertyWebkitGridColumns: {
        Vector<GridTrackSize> trackSizes;
        if (!createGridTrackList(value, trackSizes, this))
            return;
        m_style->setGridColumns(trackSizes);
        return;
    }
    case CSSPropertyWebkitGridRows: {
        Vector<GridTrackSize> trackSizes;
        if (!createGridTrackList(value, trackSizes, this))
            return;
        m_style->setGridRows(trackSizes);
        return;
    }

    case CSSPropertyWebkitGridColumn: {
        GridPosition column;
        if (!createGridPosition(value, column))
            return;
        m_style->setGridItemColumn(column);
        return;
    }
    case CSSPropertyWebkitGridRow: {
        GridPosition row;
        if (!createGridPosition(value, row))
            return;
        m_style->setGridItemRow(row);
        return;
    }
    // These properties are implemented in the StyleBuilder lookup table.
    case CSSPropertyBackgroundAttachment:
    case CSSPropertyBackgroundClip:
    case CSSPropertyBackgroundColor:
    case CSSPropertyBackgroundImage:
    case CSSPropertyBackgroundOrigin:
    case CSSPropertyBackgroundPosition:
    case CSSPropertyBackgroundPositionX:
    case CSSPropertyBackgroundPositionY:
    case CSSPropertyBackgroundRepeat:
    case CSSPropertyBackgroundRepeatX:
    case CSSPropertyBackgroundRepeatY:
    case CSSPropertyBackgroundSize:
    case CSSPropertyBorder:
    case CSSPropertyBorderBottom:
    case CSSPropertyBorderBottomColor:
    case CSSPropertyBorderBottomLeftRadius:
    case CSSPropertyBorderBottomRightRadius:
    case CSSPropertyBorderBottomStyle:
    case CSSPropertyBorderBottomWidth:
    case CSSPropertyBorderCollapse:
    case CSSPropertyBorderColor:
    case CSSPropertyBorderImage:
    case CSSPropertyBorderImageOutset:
    case CSSPropertyBorderImageRepeat:
    case CSSPropertyBorderImageSlice:
    case CSSPropertyBorderImageSource:
    case CSSPropertyBorderImageWidth:
    case CSSPropertyBorderLeft:
    case CSSPropertyBorderLeftColor:
    case CSSPropertyBorderLeftStyle:
    case CSSPropertyBorderLeftWidth:
    case CSSPropertyBorderRadius:
    case CSSPropertyBorderRight:
    case CSSPropertyBorderRightColor:
    case CSSPropertyBorderRightStyle:
    case CSSPropertyBorderRightWidth:
    case CSSPropertyBorderSpacing:
    case CSSPropertyBorderStyle:
    case CSSPropertyBorderTop:
    case CSSPropertyBorderTopColor:
    case CSSPropertyBorderTopLeftRadius:
    case CSSPropertyBorderTopRightRadius:
    case CSSPropertyBorderTopStyle:
    case CSSPropertyBorderTopWidth:
    case CSSPropertyBorderWidth:
    case CSSPropertyBottom:
    case CSSPropertyBoxSizing:
    case CSSPropertyCaptionSide:
    case CSSPropertyClear:
    case CSSPropertyClip:
    case CSSPropertyColor:
    case CSSPropertyCounterIncrement:
    case CSSPropertyCounterReset:
    case CSSPropertyCursor:
    case CSSPropertyDirection:
    case CSSPropertyDisplay:
    case CSSPropertyEmptyCells:
    case CSSPropertyFloat:
    case CSSPropertyFontSize:
    case CSSPropertyFontStyle:
    case CSSPropertyFontVariant:
    case CSSPropertyFontWeight:
    case CSSPropertyHeight:
#if ENABLE(CSS_IMAGE_ORIENTATION)
    case CSSPropertyImageOrientation:
#endif
    case CSSPropertyImageRendering:
#if ENABLE(CSS_IMAGE_RESOLUTION)
    case CSSPropertyImageResolution:
#endif
    case CSSPropertyLeft:
    case CSSPropertyLetterSpacing:
    case CSSPropertyLineHeight:
    case CSSPropertyListStyle:
    case CSSPropertyListStyleImage:
    case CSSPropertyListStylePosition:
    case CSSPropertyListStyleType:
    case CSSPropertyMargin:
    case CSSPropertyMarginBottom:
    case CSSPropertyMarginLeft:
    case CSSPropertyMarginRight:
    case CSSPropertyMarginTop:
    case CSSPropertyMaxHeight:
    case CSSPropertyMaxWidth:
    case CSSPropertyMinHeight:
    case CSSPropertyMinWidth:
    case CSSPropertyOpacity:
    case CSSPropertyOrphans:
    case CSSPropertyOutline:
    case CSSPropertyOutlineColor:
    case CSSPropertyOutlineOffset:
    case CSSPropertyOutlineStyle:
    case CSSPropertyOutlineWidth:
    case CSSPropertyOverflow:
    case CSSPropertyOverflowWrap:
    case CSSPropertyOverflowX:
    case CSSPropertyOverflowY:
    case CSSPropertyPadding:
    case CSSPropertyPaddingBottom:
    case CSSPropertyPaddingLeft:
    case CSSPropertyPaddingRight:
    case CSSPropertyPaddingTop:
    case CSSPropertyPageBreakAfter:
    case CSSPropertyPageBreakBefore:
    case CSSPropertyPageBreakInside:
    case CSSPropertyPointerEvents:
    case CSSPropertyPosition:
    case CSSPropertyResize:
    case CSSPropertyRight:
    case CSSPropertySize:
    case CSSPropertySpeak:
    case CSSPropertyTabSize:
    case CSSPropertyTableLayout:
    case CSSPropertyTextAlign:
    case CSSPropertyTextDecoration:
    case CSSPropertyTextIndent:
    case CSSPropertyTextOverflow:
    case CSSPropertyTextRendering:
    case CSSPropertyTextTransform:
    case CSSPropertyTop:
    case CSSPropertyUnicodeBidi:
#if ENABLE(CSS_VARIABLES)
    case CSSPropertyVariable:
#endif
    case CSSPropertyVerticalAlign:
    case CSSPropertyVisibility:
    case CSSPropertyWebkitAnimationDelay:
    case CSSPropertyWebkitAnimationDirection:
    case CSSPropertyWebkitAnimationDuration:
    case CSSPropertyWebkitAnimationFillMode:
    case CSSPropertyWebkitAnimationIterationCount:
    case CSSPropertyWebkitAnimationName:
    case CSSPropertyWebkitAnimationPlayState:
    case CSSPropertyWebkitAnimationTimingFunction:
    case CSSPropertyWebkitAppearance:
    case CSSPropertyWebkitAspectRatio:
    case CSSPropertyWebkitBackfaceVisibility:
    case CSSPropertyWebkitBackgroundClip:
    case CSSPropertyWebkitBackgroundComposite:
    case CSSPropertyWebkitBackgroundOrigin:
    case CSSPropertyWebkitBackgroundSize:
    case CSSPropertyWebkitBorderFit:
    case CSSPropertyWebkitBorderHorizontalSpacing:
    case CSSPropertyWebkitBorderImage:
    case CSSPropertyWebkitBorderRadius:
    case CSSPropertyWebkitBorderVerticalSpacing:
    case CSSPropertyWebkitBoxAlign:
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    case CSSPropertyWebkitBoxDecorationBreak:
#endif
    case CSSPropertyWebkitBoxDirection:
    case CSSPropertyWebkitBoxFlex:
    case CSSPropertyWebkitBoxFlexGroup:
    case CSSPropertyWebkitBoxLines:
    case CSSPropertyWebkitBoxOrdinalGroup:
    case CSSPropertyWebkitBoxOrient:
    case CSSPropertyWebkitBoxPack:
    case CSSPropertyWebkitColorCorrection:
    case CSSPropertyWebkitColumnAxis:
    case CSSPropertyWebkitColumnBreakAfter:
    case CSSPropertyWebkitColumnBreakBefore:
    case CSSPropertyWebkitColumnBreakInside:
    case CSSPropertyWebkitColumnCount:
    case CSSPropertyWebkitColumnGap:
    case CSSPropertyWebkitColumnProgression:
    case CSSPropertyWebkitColumnRuleColor:
    case CSSPropertyWebkitColumnRuleStyle:
    case CSSPropertyWebkitColumnRuleWidth:
    case CSSPropertyWebkitColumns:
    case CSSPropertyWebkitColumnSpan:
    case CSSPropertyWebkitColumnWidth:
    case CSSPropertyWebkitAlignContent:
    case CSSPropertyWebkitAlignItems:
    case CSSPropertyWebkitAlignSelf:
    case CSSPropertyWebkitFlex:
    case CSSPropertyWebkitFlexBasis:
    case CSSPropertyWebkitFlexDirection:
    case CSSPropertyWebkitFlexFlow:
    case CSSPropertyWebkitFlexGrow:
    case CSSPropertyWebkitFlexShrink:
    case CSSPropertyWebkitFlexWrap:
    case CSSPropertyWebkitJustifyContent:
    case CSSPropertyWebkitOrder:
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitFlowFrom:
    case CSSPropertyWebkitFlowInto:
#endif
    case CSSPropertyWebkitFontKerning:
    case CSSPropertyWebkitFontSmoothing:
    case CSSPropertyWebkitFontVariantLigatures:
    case CSSPropertyWebkitHighlight:
    case CSSPropertyWebkitHyphenateCharacter:
    case CSSPropertyWebkitHyphenateLimitAfter:
    case CSSPropertyWebkitHyphenateLimitBefore:
    case CSSPropertyWebkitHyphenateLimitLines:
    case CSSPropertyWebkitHyphens:
    case CSSPropertyWebkitLineAlign:
    case CSSPropertyWebkitLineBreak:
    case CSSPropertyWebkitLineClamp:
    case CSSPropertyWebkitLineGrid:
    case CSSPropertyWebkitLineSnap:
    case CSSPropertyWebkitMarqueeDirection:
    case CSSPropertyWebkitMarqueeStyle:
    case CSSPropertyWebkitMaskBoxImage:
    case CSSPropertyWebkitMaskBoxImageOutset:
    case CSSPropertyWebkitMaskBoxImageRepeat:
    case CSSPropertyWebkitMaskBoxImageSlice:
    case CSSPropertyWebkitMaskBoxImageSource:
    case CSSPropertyWebkitMaskBoxImageWidth:
    case CSSPropertyWebkitMaskClip:
    case CSSPropertyWebkitMaskComposite:
    case CSSPropertyWebkitMaskImage:
    case CSSPropertyWebkitMaskOrigin:
    case CSSPropertyWebkitMaskPosition:
    case CSSPropertyWebkitMaskPositionX:
    case CSSPropertyWebkitMaskPositionY:
    case CSSPropertyWebkitMaskRepeat:
    case CSSPropertyWebkitMaskRepeatX:
    case CSSPropertyWebkitMaskRepeatY:
    case CSSPropertyWebkitMaskSize:
    case CSSPropertyWebkitNbspMode:
    case CSSPropertyWebkitPerspectiveOrigin:
    case CSSPropertyWebkitPerspectiveOriginX:
    case CSSPropertyWebkitPerspectiveOriginY:
    case CSSPropertyWebkitPrintColorAdjust:
#if ENABLE(CSS_REGIONS)
    case CSSPropertyWebkitRegionBreakAfter:
    case CSSPropertyWebkitRegionBreakBefore:
    case CSSPropertyWebkitRegionBreakInside:
    case CSSPropertyWebkitRegionOverflow:
#endif
    case CSSPropertyWebkitRtlOrdering:
    case CSSPropertyWebkitRubyPosition:
    case CSSPropertyWebkitTextCombine:
#if ENABLE(CSS3_TEXT)
    case CSSPropertyWebkitTextDecorationLine:
    case CSSPropertyWebkitTextDecorationStyle:
    case CSSPropertyWebkitTextAlignLast:
#endif // CSS3_TEXT
    case CSSPropertyWebkitTextEmphasisColor:
    case CSSPropertyWebkitTextEmphasisPosition:
    case CSSPropertyWebkitTextEmphasisStyle:
    case CSSPropertyWebkitTextFillColor:
    case CSSPropertyWebkitTextOrientation:
    case CSSPropertyWebkitTextSecurity:
    case CSSPropertyWebkitTextStrokeColor:
    case CSSPropertyWebkitTransformOrigin:
    case CSSPropertyWebkitTransformOriginX:
    case CSSPropertyWebkitTransformOriginY:
    case CSSPropertyWebkitTransformOriginZ:
    case CSSPropertyWebkitTransformStyle:
    case CSSPropertyWebkitTransitionDelay:
    case CSSPropertyWebkitTransitionDuration:
    case CSSPropertyWebkitTransitionProperty:
    case CSSPropertyWebkitTransitionTimingFunction:
    case CSSPropertyWebkitUserDrag:
    case CSSPropertyWebkitUserModify:
    case CSSPropertyWebkitUserSelect:
    case CSSPropertyWebkitClipPath:
#if ENABLE(CSS_EXCLUSIONS)
    case CSSPropertyWebkitWrap:
    case CSSPropertyWebkitWrapFlow:
    case CSSPropertyWebkitShapeMargin:
    case CSSPropertyWebkitShapePadding:
    case CSSPropertyWebkitWrapThrough:
    case CSSPropertyWebkitShapeInside:
    case CSSPropertyWebkitShapeOutside:
#endif
    case CSSPropertyWhiteSpace:
    case CSSPropertyWidows:
    case CSSPropertyWidth:
    case CSSPropertyWordBreak:
    case CSSPropertyWordSpacing:
    case CSSPropertyWordWrap:
    case CSSPropertyZIndex:
    case CSSPropertyZoom:
#if ENABLE(CSS_DEVICE_ADAPTATION)
    case CSSPropertyMaxZoom:
    case CSSPropertyMinZoom:
    case CSSPropertyOrientation:
    case CSSPropertyUserZoom:
#endif
        ASSERT_NOT_REACHED();
        return;
#if ENABLE(SVG)
    default:
        // Try the SVG properties
        applySVGProperty(id, value);
        return;
#endif
    }
}

PassRefPtr<StyleImage> StyleResolver::styleImage(CSSPropertyID property, CSSValue* value)
{
    if (value->isImageValue())
        return cachedOrPendingFromValue(property, static_cast<CSSImageValue*>(value));

    if (value->isImageGeneratorValue()) {
        if (value->isGradientValue())
            return generatedOrPendingFromValue(property, static_cast<CSSGradientValue*>(value)->gradientWithStylesResolved(this).get());
        return generatedOrPendingFromValue(property, static_cast<CSSImageGeneratorValue*>(value));
    }

#if ENABLE(CSS_IMAGE_SET)
    if (value->isImageSetValue())
        return setOrPendingFromValue(property, static_cast<CSSImageSetValue*>(value));
#endif

    return 0;
}

PassRefPtr<StyleImage> StyleResolver::cachedOrPendingFromValue(CSSPropertyID property, CSSImageValue* value)
{
    RefPtr<StyleImage> image = value->cachedOrPendingImage();
    if (image && image->isPendingImage())
        m_pendingImageProperties.set(property, value);
    return image.release();
}

PassRefPtr<StyleImage> StyleResolver::generatedOrPendingFromValue(CSSPropertyID property, CSSImageGeneratorValue* value)
{
    if (value->isPending()) {
        m_pendingImageProperties.set(property, value);
        return StylePendingImage::create(value);
    }
    return StyleGeneratedImage::create(value);
}

#if ENABLE(CSS_IMAGE_SET)
PassRefPtr<StyleImage> StyleResolver::setOrPendingFromValue(CSSPropertyID property, CSSImageSetValue* value)
{
    RefPtr<StyleImage> image = value->cachedOrPendingImageSet(document());
    if (image && image->isPendingImage())
        m_pendingImageProperties.set(property, value);
    return image.release();
}
#endif

void StyleResolver::checkForTextSizeAdjust()
{
    if (m_style->textSizeAdjust())
        return;

    FontDescription newFontDescription(m_style->fontDescription());
    newFontDescription.setComputedSize(newFontDescription.specifiedSize());
    m_style->setFontDescription(newFontDescription);
}

void StyleResolver::checkForZoomChange(RenderStyle* style, RenderStyle* parentStyle)
{
    if (style->effectiveZoom() == parentStyle->effectiveZoom())
        return;

    const FontDescription& childFont = style->fontDescription();
    FontDescription newFontDescription(childFont);
    setFontSize(newFontDescription, childFont.specifiedSize());
    style->setFontDescription(newFontDescription);
}

void StyleResolver::checkForGenericFamilyChange(RenderStyle* style, RenderStyle* parentStyle)
{
    const FontDescription& childFont = style->fontDescription();

    if (childFont.isAbsoluteSize() || !parentStyle)
        return;

    const FontDescription& parentFont = parentStyle->fontDescription();
    if (childFont.useFixedDefaultSize() == parentFont.useFixedDefaultSize())
        return;

    // For now, lump all families but monospace together.
    if (childFont.genericFamily() != FontDescription::MonospaceFamily
        && parentFont.genericFamily() != FontDescription::MonospaceFamily)
        return;

    // We know the parent is monospace or the child is monospace, and that font
    // size was unspecified. We want to scale our font size as appropriate.
    // If the font uses a keyword size, then we refetch from the table rather than
    // multiplying by our scale factor.
    float size;
    if (childFont.keywordSize())
        size = fontSizeForKeyword(m_checker.document(), CSSValueXxSmall + childFont.keywordSize() - 1, childFont.useFixedDefaultSize());
    else {
        Settings* settings = documentSettings();
        float fixedScaleFactor = (settings && settings->defaultFixedFontSize() && settings->defaultFontSize())
            ? static_cast<float>(settings->defaultFixedFontSize()) / settings->defaultFontSize()
            : 1;
        size = parentFont.useFixedDefaultSize() ?
                childFont.specifiedSize() / fixedScaleFactor :
                childFont.specifiedSize() * fixedScaleFactor;
    }

    FontDescription newFontDescription(childFont);
    setFontSize(newFontDescription, size);
    style->setFontDescription(newFontDescription);
}

void StyleResolver::initializeFontStyle(Settings* settings)
{
    FontDescription fontDescription;
    fontDescription.setGenericFamily(FontDescription::StandardFamily);
    fontDescription.setRenderingMode(settings->fontRenderingMode());
    fontDescription.setUsePrinterFont(m_checker.document()->printing() || !settings->screenFontSubstitutionEnabled());
    const AtomicString& standardFontFamily = documentSettings()->standardFontFamily();
    if (!standardFontFamily.isEmpty()) {
        fontDescription.firstFamily().setFamily(standardFontFamily);
        fontDescription.firstFamily().appendFamily(0);
    }
    fontDescription.setKeywordSize(CSSValueMedium - CSSValueXxSmall + 1);
    setFontSize(fontDescription, fontSizeForKeyword(m_checker.document(), CSSValueMedium, false));
    m_style->setLineHeight(RenderStyle::initialLineHeight());
    m_lineHeightValue = 0;
    setFontDescription(fontDescription);
}

void StyleResolver::setFontSize(FontDescription& fontDescription, float size)
{
    fontDescription.setSpecifiedSize(size);
    fontDescription.setComputedSize(getComputedSizeFromSpecifiedSize(m_checker.document(), m_style.get(), fontDescription.isAbsoluteSize(), size, useSVGZoomRules()));
}

float StyleResolver::getComputedSizeFromSpecifiedSize(Document* document, RenderStyle* style, bool isAbsoluteSize, float specifiedSize, bool useSVGZoomRules)
{
    float zoomFactor = 1.0f;
    if (!useSVGZoomRules) {
        zoomFactor = style->effectiveZoom();
        if (Frame* frame = document->frame())
            zoomFactor *= frame->textZoomFactor();
    }

    return StyleResolver::getComputedSizeFromSpecifiedSize(document, zoomFactor, isAbsoluteSize, specifiedSize);
}

float StyleResolver::getComputedSizeFromSpecifiedSize(Document* document, float zoomFactor, bool isAbsoluteSize, float specifiedSize, ESmartMinimumForFontSize useSmartMinimumForFontSize)
{
    // Text with a 0px font size should not be visible and therefore needs to be
    // exempt from minimum font size rules. Acid3 relies on this for pixel-perfect
    // rendering. This is also compatible with other browsers that have minimum
    // font size settings (e.g. Firefox).
    if (fabsf(specifiedSize) < std::numeric_limits<float>::epsilon())
        return 0.0f;

    // We support two types of minimum font size. The first is a hard override that applies to
    // all fonts. This is "minSize." The second type of minimum font size is a "smart minimum"
    // that is applied only when the Web page can't know what size it really asked for, e.g.,
    // when it uses logical sizes like "small" or expresses the font-size as a percentage of
    // the user's default font setting.

    // With the smart minimum, we never want to get smaller than the minimum font size to keep fonts readable.
    // However we always allow the page to set an explicit pixel size that is smaller,
    // since sites will mis-render otherwise (e.g., http://www.gamespot.com with a 9px minimum).

    Settings* settings = document->settings();
    if (!settings)
        return 1.0f;

    int minSize = settings->minimumFontSize();
    int minLogicalSize = settings->minimumLogicalFontSize();
    float zoomedSize = specifiedSize * zoomFactor;

    // Apply the hard minimum first. We only apply the hard minimum if after zooming we're still too small.
    if (zoomedSize < minSize)
        zoomedSize = minSize;

    // Now apply the "smart minimum." This minimum is also only applied if we're still too small
    // after zooming. The font size must either be relative to the user default or the original size
    // must have been acceptable. In other words, we only apply the smart minimum whenever we're positive
    // doing so won't disrupt the layout.
    if (useSmartMinimumForFontSize && zoomedSize < minLogicalSize && (specifiedSize >= minLogicalSize || !isAbsoluteSize))
        zoomedSize = minLogicalSize;

    // Also clamp to a reasonable maximum to prevent insane font sizes from causing crashes on various
    // platforms (I'm looking at you, Windows.)
    return min(1000000.0f, zoomedSize);
}

const int fontSizeTableMax = 16;
const int fontSizeTableMin = 9;
const int totalKeywords = 8;

// WinIE/Nav4 table for font sizes. Designed to match the legacy font mapping system of HTML.
static const int quirksFontSizeTable[fontSizeTableMax - fontSizeTableMin + 1][totalKeywords] =
{
      { 9,    9,     9,     9,    11,    14,    18,    28 },
      { 9,    9,     9,    10,    12,    15,    20,    31 },
      { 9,    9,     9,    11,    13,    17,    22,    34 },
      { 9,    9,    10,    12,    14,    18,    24,    37 },
      { 9,    9,    10,    13,    16,    20,    26,    40 }, // fixed font default (13)
      { 9,    9,    11,    14,    17,    21,    28,    42 },
      { 9,   10,    12,    15,    17,    23,    30,    45 },
      { 9,   10,    13,    16,    18,    24,    32,    48 } // proportional font default (16)
};
// HTML       1      2      3      4      5      6      7
// CSS  xxs   xs     s      m      l     xl     xxl
//                          |
//                      user pref

// Strict mode table matches MacIE and Mozilla's settings exactly.
static const int strictFontSizeTable[fontSizeTableMax - fontSizeTableMin + 1][totalKeywords] =
{
      { 9,    9,     9,     9,    11,    14,    18,    27 },
      { 9,    9,     9,    10,    12,    15,    20,    30 },
      { 9,    9,    10,    11,    13,    17,    22,    33 },
      { 9,    9,    10,    12,    14,    18,    24,    36 },
      { 9,   10,    12,    13,    16,    20,    26,    39 }, // fixed font default (13)
      { 9,   10,    12,    14,    17,    21,    28,    42 },
      { 9,   10,    13,    15,    18,    23,    30,    45 },
      { 9,   10,    13,    16,    18,    24,    32,    48 } // proportional font default (16)
};
// HTML       1      2      3      4      5      6      7
// CSS  xxs   xs     s      m      l     xl     xxl
//                          |
//                      user pref

// For values outside the range of the table, we use Todd Fahrner's suggested scale
// factors for each keyword value.
static const float fontSizeFactors[totalKeywords] = { 0.60f, 0.75f, 0.89f, 1.0f, 1.2f, 1.5f, 2.0f, 3.0f };

float StyleResolver::fontSizeForKeyword(Document* document, int keyword, bool shouldUseFixedDefaultSize)
{
    Settings* settings = document->settings();
    if (!settings)
        return 1.0f;

    bool quirksMode = document->inQuirksMode();
    int mediumSize = shouldUseFixedDefaultSize ? settings->defaultFixedFontSize() : settings->defaultFontSize();
    if (mediumSize >= fontSizeTableMin && mediumSize <= fontSizeTableMax) {
        // Look up the entry in the table.
        int row = mediumSize - fontSizeTableMin;
        int col = (keyword - CSSValueXxSmall);
        return quirksMode ? quirksFontSizeTable[row][col] : strictFontSizeTable[row][col];
    }

    // Value is outside the range of the table. Apply the scale factor instead.
    float minLogicalSize = max(settings->minimumLogicalFontSize(), 1);
    return max(fontSizeFactors[keyword - CSSValueXxSmall]*mediumSize, minLogicalSize);
}

template<typename T>
static int findNearestLegacyFontSize(int pixelFontSize, const T* table, int multiplier)
{
    // Ignore table[0] because xx-small does not correspond to any legacy font size.
    for (int i = 1; i < totalKeywords - 1; i++) {
        if (pixelFontSize * 2 < (table[i] + table[i + 1]) * multiplier)
            return i;
    }
    return totalKeywords - 1;
}

int StyleResolver::legacyFontSize(Document* document, int pixelFontSize, bool shouldUseFixedDefaultSize)
{
    Settings* settings = document->settings();
    if (!settings)
        return 1;

    bool quirksMode = document->inQuirksMode();
    int mediumSize = shouldUseFixedDefaultSize ? settings->defaultFixedFontSize() : settings->defaultFontSize();
    if (mediumSize >= fontSizeTableMin && mediumSize <= fontSizeTableMax) {
        int row = mediumSize - fontSizeTableMin;
        return findNearestLegacyFontSize<int>(pixelFontSize, quirksMode ? quirksFontSizeTable[row] : strictFontSizeTable[row], 1);
    }

    return findNearestLegacyFontSize<float>(pixelFontSize, fontSizeFactors, mediumSize);
}

static Color colorForCSSValue(int cssValueId)
{
    struct ColorValue {
        int cssValueId;
        RGBA32 color;
    };

    static const ColorValue colorValues[] = {
        { CSSValueAqua, 0xFF00FFFF },
        { CSSValueBlack, 0xFF000000 },
        { CSSValueBlue, 0xFF0000FF },
        { CSSValueFuchsia, 0xFFFF00FF },
        { CSSValueGray, 0xFF808080 },
        { CSSValueGreen, 0xFF008000  },
        { CSSValueGrey, 0xFF808080 },
        { CSSValueLime, 0xFF00FF00 },
        { CSSValueMaroon, 0xFF800000 },
        { CSSValueNavy, 0xFF000080 },
        { CSSValueOlive, 0xFF808000  },
        { CSSValueOrange, 0xFFFFA500 },
        { CSSValuePurple, 0xFF800080 },
        { CSSValueRed, 0xFFFF0000 },
        { CSSValueSilver, 0xFFC0C0C0 },
        { CSSValueTeal, 0xFF008080  },
        { CSSValueTransparent, 0x00000000 },
        { CSSValueWhite, 0xFFFFFFFF },
        { CSSValueYellow, 0xFFFFFF00 },
        { 0, 0 }
    };

    for (const ColorValue* col = colorValues; col->cssValueId; ++col) {
        if (col->cssValueId == cssValueId)
            return col->color;
    }
    return RenderTheme::defaultTheme()->systemColor(cssValueId);
}

bool StyleResolver::colorFromPrimitiveValueIsDerivedFromElement(CSSPrimitiveValue* value)
{
    int ident = value->getIdent();
    switch (ident) {
    case CSSValueWebkitText:
    case CSSValueWebkitLink:
    case CSSValueWebkitActivelink:
    case CSSValueCurrentcolor:
        return true;
    default:
        return false;
    }
}

Color StyleResolver::colorFromPrimitiveValue(CSSPrimitiveValue* value, bool forVisitedLink) const
{
    if (value->isRGBColor())
        return Color(value->getRGBA32Value());

    int ident = value->getIdent();
    switch (ident) {
    case 0:
        return Color();
    case CSSValueWebkitText:
        return m_element->document()->textColor();
    case CSSValueWebkitLink:
        return (m_element->isLink() && forVisitedLink) ? m_element->document()->visitedLinkColor() : m_element->document()->linkColor();
    case CSSValueWebkitActivelink:
        return m_element->document()->activeLinkColor();
    case CSSValueWebkitFocusRingColor:
        return RenderTheme::focusRingColor();
    case CSSValueCurrentcolor:
        return m_style->color();
    default:
        return colorForCSSValue(ident);
    }
}

void StyleResolver::addViewportDependentMediaQueryResult(const MediaQueryExp* expr, bool result)
{
    m_viewportDependentMediaQueryResults.append(adoptPtr(new MediaQueryResult(*expr, result)));
}

bool StyleResolver::affectedByViewportChange() const
{
    unsigned s = m_viewportDependentMediaQueryResults.size();
    for (unsigned i = 0; i < s; i++) {
        if (m_medium->eval(&m_viewportDependentMediaQueryResults[i]->m_expression) != m_viewportDependentMediaQueryResults[i]->m_result)
            return true;
    }
    return false;
}

static TransformOperation::OperationType getTransformOperationType(WebKitCSSTransformValue::TransformOperationType type)
{
    switch (type) {
    case WebKitCSSTransformValue::ScaleTransformOperation: return TransformOperation::SCALE;
    case WebKitCSSTransformValue::ScaleXTransformOperation: return TransformOperation::SCALE_X;
    case WebKitCSSTransformValue::ScaleYTransformOperation: return TransformOperation::SCALE_Y;
    case WebKitCSSTransformValue::ScaleZTransformOperation: return TransformOperation::SCALE_Z;
    case WebKitCSSTransformValue::Scale3DTransformOperation: return TransformOperation::SCALE_3D;
    case WebKitCSSTransformValue::TranslateTransformOperation: return TransformOperation::TRANSLATE;
    case WebKitCSSTransformValue::TranslateXTransformOperation: return TransformOperation::TRANSLATE_X;
    case WebKitCSSTransformValue::TranslateYTransformOperation: return TransformOperation::TRANSLATE_Y;
    case WebKitCSSTransformValue::TranslateZTransformOperation: return TransformOperation::TRANSLATE_Z;
    case WebKitCSSTransformValue::Translate3DTransformOperation: return TransformOperation::TRANSLATE_3D;
    case WebKitCSSTransformValue::RotateTransformOperation: return TransformOperation::ROTATE;
    case WebKitCSSTransformValue::RotateXTransformOperation: return TransformOperation::ROTATE_X;
    case WebKitCSSTransformValue::RotateYTransformOperation: return TransformOperation::ROTATE_Y;
    case WebKitCSSTransformValue::RotateZTransformOperation: return TransformOperation::ROTATE_Z;
    case WebKitCSSTransformValue::Rotate3DTransformOperation: return TransformOperation::ROTATE_3D;
    case WebKitCSSTransformValue::SkewTransformOperation: return TransformOperation::SKEW;
    case WebKitCSSTransformValue::SkewXTransformOperation: return TransformOperation::SKEW_X;
    case WebKitCSSTransformValue::SkewYTransformOperation: return TransformOperation::SKEW_Y;
    case WebKitCSSTransformValue::MatrixTransformOperation: return TransformOperation::MATRIX;
    case WebKitCSSTransformValue::Matrix3DTransformOperation: return TransformOperation::MATRIX_3D;
    case WebKitCSSTransformValue::PerspectiveTransformOperation: return TransformOperation::PERSPECTIVE;
    case WebKitCSSTransformValue::UnknownTransformOperation: return TransformOperation::NONE;
    }
    return TransformOperation::NONE;
}

bool StyleResolver::createTransformOperations(CSSValue* inValue, RenderStyle* style, RenderStyle* rootStyle, TransformOperations& outOperations)
{
    if (!inValue || !inValue->isValueList()) {
        outOperations.clear();
        return false;
    }

    float zoomFactor = style ? style->effectiveZoom() : 1;
    TransformOperations operations;
    for (CSSValueListIterator i = inValue; i.hasMore(); i.advance()) {
        CSSValue* currValue = i.value();

        if (!currValue->isWebKitCSSTransformValue())
            continue;

        WebKitCSSTransformValue* transformValue = static_cast<WebKitCSSTransformValue*>(i.value());
        if (!transformValue->length())
            continue;

        bool haveNonPrimitiveValue = false;
        for (unsigned j = 0; j < transformValue->length(); ++j) {
            if (!transformValue->itemWithoutBoundsCheck(j)->isPrimitiveValue()) {
                haveNonPrimitiveValue = true;
                break;
            }
        }
        if (haveNonPrimitiveValue)
            continue;

        CSSPrimitiveValue* firstValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(0));

        switch (transformValue->operationType()) {
        case WebKitCSSTransformValue::ScaleTransformOperation:
        case WebKitCSSTransformValue::ScaleXTransformOperation:
        case WebKitCSSTransformValue::ScaleYTransformOperation: {
            double sx = 1.0;
            double sy = 1.0;
            if (transformValue->operationType() == WebKitCSSTransformValue::ScaleYTransformOperation)
                sy = firstValue->getDoubleValue();
            else {
                sx = firstValue->getDoubleValue();
                if (transformValue->operationType() != WebKitCSSTransformValue::ScaleXTransformOperation) {
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        sy = secondValue->getDoubleValue();
                    } else
                        sy = sx;
                }
            }
            operations.operations().append(ScaleTransformOperation::create(sx, sy, 1.0, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::ScaleZTransformOperation:
        case WebKitCSSTransformValue::Scale3DTransformOperation: {
            double sx = 1.0;
            double sy = 1.0;
            double sz = 1.0;
            if (transformValue->operationType() == WebKitCSSTransformValue::ScaleZTransformOperation)
                sz = firstValue->getDoubleValue();
            else if (transformValue->operationType() == WebKitCSSTransformValue::ScaleYTransformOperation)
                sy = firstValue->getDoubleValue();
            else {
                sx = firstValue->getDoubleValue();
                if (transformValue->operationType() != WebKitCSSTransformValue::ScaleXTransformOperation) {
                    if (transformValue->length() > 2) {
                        CSSPrimitiveValue* thirdValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2));
                        sz = thirdValue->getDoubleValue();
                    }
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        sy = secondValue->getDoubleValue();
                    } else
                        sy = sx;
                }
            }
            operations.operations().append(ScaleTransformOperation::create(sx, sy, sz, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::TranslateTransformOperation:
        case WebKitCSSTransformValue::TranslateXTransformOperation:
        case WebKitCSSTransformValue::TranslateYTransformOperation: {
            Length tx = Length(0, Fixed);
            Length ty = Length(0, Fixed);
            if (transformValue->operationType() == WebKitCSSTransformValue::TranslateYTransformOperation)
                ty = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else {
                tx = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
                if (transformValue->operationType() != WebKitCSSTransformValue::TranslateXTransformOperation) {
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        ty = convertToFloatLength(secondValue, style, rootStyle, zoomFactor);
                    }
                }
            }

            if (tx.isUndefined() || ty.isUndefined())
                return false;

            operations.operations().append(TranslateTransformOperation::create(tx, ty, Length(0, Fixed), getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::TranslateZTransformOperation:
        case WebKitCSSTransformValue::Translate3DTransformOperation: {
            Length tx = Length(0, Fixed);
            Length ty = Length(0, Fixed);
            Length tz = Length(0, Fixed);
            if (transformValue->operationType() == WebKitCSSTransformValue::TranslateZTransformOperation)
                tz = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else if (transformValue->operationType() == WebKitCSSTransformValue::TranslateYTransformOperation)
                ty = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else {
                tx = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
                if (transformValue->operationType() != WebKitCSSTransformValue::TranslateXTransformOperation) {
                    if (transformValue->length() > 2) {
                        CSSPrimitiveValue* thirdValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2));
                        tz = convertToFloatLength(thirdValue, style, rootStyle, zoomFactor);
                    }
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        ty = convertToFloatLength(secondValue, style, rootStyle, zoomFactor);
                    }
                }
            }

            if (tx.isUndefined() || ty.isUndefined() || tz.isUndefined())
                return false;

            operations.operations().append(TranslateTransformOperation::create(tx, ty, tz, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::RotateTransformOperation: {
            double angle = firstValue->computeDegrees();
            operations.operations().append(RotateTransformOperation::create(0, 0, 1, angle, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::RotateXTransformOperation:
        case WebKitCSSTransformValue::RotateYTransformOperation:
        case WebKitCSSTransformValue::RotateZTransformOperation: {
            double x = 0;
            double y = 0;
            double z = 0;
            double angle = firstValue->computeDegrees();

            if (transformValue->operationType() == WebKitCSSTransformValue::RotateXTransformOperation)
                x = 1;
            else if (transformValue->operationType() == WebKitCSSTransformValue::RotateYTransformOperation)
                y = 1;
            else
                z = 1;
            operations.operations().append(RotateTransformOperation::create(x, y, z, angle, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::Rotate3DTransformOperation: {
            if (transformValue->length() < 4)
                break;
            CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
            CSSPrimitiveValue* thirdValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2));
            CSSPrimitiveValue* fourthValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(3));
            double x = firstValue->getDoubleValue();
            double y = secondValue->getDoubleValue();
            double z = thirdValue->getDoubleValue();
            double angle = fourthValue->computeDegrees();
            operations.operations().append(RotateTransformOperation::create(x, y, z, angle, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::SkewTransformOperation:
        case WebKitCSSTransformValue::SkewXTransformOperation:
        case WebKitCSSTransformValue::SkewYTransformOperation: {
            double angleX = 0;
            double angleY = 0;
            double angle = firstValue->computeDegrees();
            if (transformValue->operationType() == WebKitCSSTransformValue::SkewYTransformOperation)
                angleY = angle;
            else {
                angleX = angle;
                if (transformValue->operationType() == WebKitCSSTransformValue::SkewTransformOperation) {
                    if (transformValue->length() > 1) {
                        CSSPrimitiveValue* secondValue = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1));
                        angleY = secondValue->computeDegrees();
                    }
                }
            }
            operations.operations().append(SkewTransformOperation::create(angleX, angleY, getTransformOperationType(transformValue->operationType())));
            break;
        }
        case WebKitCSSTransformValue::MatrixTransformOperation: {
            if (transformValue->length() < 6)
                break;
            double a = firstValue->getDoubleValue();
            double b = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1))->getDoubleValue();
            double c = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2))->getDoubleValue();
            double d = static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(3))->getDoubleValue();
            double e = zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(4))->getDoubleValue();
            double f = zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(5))->getDoubleValue();
            operations.operations().append(MatrixTransformOperation::create(a, b, c, d, e, f));
            break;
        }
        case WebKitCSSTransformValue::Matrix3DTransformOperation: {
            if (transformValue->length() < 16)
                break;
            TransformationMatrix matrix(static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(0))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(1))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(2))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(3))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(4))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(5))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(6))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(7))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(8))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(9))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(10))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(11))->getDoubleValue(),
                               zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(12))->getDoubleValue(),
                               zoomFactor * static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(13))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(14))->getDoubleValue(),
                               static_cast<CSSPrimitiveValue*>(transformValue->itemWithoutBoundsCheck(15))->getDoubleValue());
            operations.operations().append(Matrix3DTransformOperation::create(matrix));
            break;
        }
        case WebKitCSSTransformValue::PerspectiveTransformOperation: {
            Length p = Length(0, Fixed);
            if (firstValue->isLength())
                p = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            else {
                // This is a quirk that should go away when 3d transforms are finalized.
                double val = firstValue->getDoubleValue();
                p = val >= 0 ? Length(clampToPositiveInteger(val), Fixed) : Length(Undefined);
            }

            if (p.isUndefined())
                return false;

            operations.operations().append(PerspectiveTransformOperation::create(p));
            break;
        }
        case WebKitCSSTransformValue::UnknownTransformOperation:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    outOperations = operations;
    return true;
}

#if ENABLE(CSS_FILTERS)
static FilterOperation::OperationType filterOperationForType(WebKitCSSFilterValue::FilterOperationType type)
{
    switch (type) {
    case WebKitCSSFilterValue::ReferenceFilterOperation:
        return FilterOperation::REFERENCE;
    case WebKitCSSFilterValue::GrayscaleFilterOperation:
        return FilterOperation::GRAYSCALE;
    case WebKitCSSFilterValue::SepiaFilterOperation:
        return FilterOperation::SEPIA;
    case WebKitCSSFilterValue::SaturateFilterOperation:
        return FilterOperation::SATURATE;
    case WebKitCSSFilterValue::HueRotateFilterOperation:
        return FilterOperation::HUE_ROTATE;
    case WebKitCSSFilterValue::InvertFilterOperation:
        return FilterOperation::INVERT;
    case WebKitCSSFilterValue::OpacityFilterOperation:
        return FilterOperation::OPACITY;
    case WebKitCSSFilterValue::BrightnessFilterOperation:
        return FilterOperation::BRIGHTNESS;
    case WebKitCSSFilterValue::ContrastFilterOperation:
        return FilterOperation::CONTRAST;
    case WebKitCSSFilterValue::BlurFilterOperation:
        return FilterOperation::BLUR;
    case WebKitCSSFilterValue::DropShadowFilterOperation:
        return FilterOperation::DROP_SHADOW;
#if ENABLE(CSS_SHADERS)
    case WebKitCSSFilterValue::CustomFilterOperation:
        return FilterOperation::CUSTOM;
#endif
    case WebKitCSSFilterValue::UnknownFilterOperation:
        return FilterOperation::NONE;
    }
    return FilterOperation::NONE;
}

#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
void StyleResolver::loadPendingSVGDocuments()
{
    if (!m_style->hasFilter() || m_pendingSVGDocuments.isEmpty())
        return;

    CachedResourceLoader* cachedResourceLoader = m_element->document()->cachedResourceLoader();
    Vector<RefPtr<FilterOperation> >& filterOperations = m_style->mutableFilter().operations();
    for (unsigned i = 0; i < filterOperations.size(); ++i) {
        RefPtr<FilterOperation> filterOperation = filterOperations.at(i);
        if (filterOperation->getOperationType() == FilterOperation::REFERENCE) {
            ReferenceFilterOperation* referenceFilter = static_cast<ReferenceFilterOperation*>(filterOperation.get());

            WebKitCSSSVGDocumentValue* value = m_pendingSVGDocuments.get(referenceFilter).get();
            if (!value)
                continue;
            CachedSVGDocument* cachedDocument = value->load(cachedResourceLoader);
            if (!cachedDocument)
                continue;

            // Stash the CachedSVGDocument on the reference filter.
            referenceFilter->setCachedSVGDocumentReference(adoptPtr(new CachedSVGDocumentReference(cachedDocument)));
        }
    }
    m_pendingSVGDocuments.clear();
}
#endif

#if ENABLE(CSS_SHADERS)
StyleShader* StyleResolver::styleShader(CSSValue* value)
{
    if (value->isWebKitCSSShaderValue())
        return cachedOrPendingStyleShaderFromValue(static_cast<WebKitCSSShaderValue*>(value));
    return 0;
}

StyleShader* StyleResolver::cachedOrPendingStyleShaderFromValue(WebKitCSSShaderValue* value)
{
    StyleShader* shader = value->cachedOrPendingShader();
    if (shader && shader->isPendingShader())
        m_hasPendingShaders = true;
    return shader;
}

void StyleResolver::loadPendingShaders()
{
    if (!m_style->hasFilter() || !m_hasPendingShaders)
        return;

    CachedResourceLoader* cachedResourceLoader = m_element->document()->cachedResourceLoader();

    Vector<RefPtr<FilterOperation> >& filterOperations = m_style->mutableFilter().operations();
    for (unsigned i = 0; i < filterOperations.size(); ++i) {
        RefPtr<FilterOperation> filterOperation = filterOperations.at(i);
        if (filterOperation->getOperationType() == FilterOperation::CUSTOM) {
            CustomFilterOperation* customFilter = static_cast<CustomFilterOperation*>(filterOperation.get());
            ASSERT(customFilter->program());
            StyleCustomFilterProgram* program = static_cast<StyleCustomFilterProgram*>(customFilter->program());
            if (program->vertexShader() && program->vertexShader()->isPendingShader()) {
                WebKitCSSShaderValue* shaderValue = static_cast<StylePendingShader*>(program->vertexShader())->cssShaderValue();
                program->setVertexShader(shaderValue->cachedShader(cachedResourceLoader));
            }
            if (program->fragmentShader() && program->fragmentShader()->isPendingShader()) {
                WebKitCSSShaderValue* shaderValue = static_cast<StylePendingShader*>(program->fragmentShader())->cssShaderValue();
                program->setFragmentShader(shaderValue->cachedShader(cachedResourceLoader));
            }
        }
    }
    m_hasPendingShaders = false;
}

static bool sortParametersByNameComparator(const RefPtr<CustomFilterParameter>& a, const RefPtr<CustomFilterParameter>& b)
{
    return codePointCompareLessThan(a->name(), b->name());
}

PassRefPtr<CustomFilterParameter> StyleResolver::parseCustomFilterArrayParameter(const String& name, CSSValueList* values)
{
    RefPtr<CustomFilterArrayParameter> arrayParameter = CustomFilterArrayParameter::create(name);
    for (unsigned i = 0, length = values->length(); i < length; ++i) {
        CSSValue* value = values->itemWithoutBoundsCheck(i);
        if (!value->isPrimitiveValue())
            return 0;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_NUMBER)
            return 0;
        arrayParameter->addValue(primitiveValue->getDoubleValue());
    }
    return arrayParameter.release();
}

PassRefPtr<CustomFilterParameter> StyleResolver::parseCustomFilterNumberParameter(const String& name, CSSValueList* values)
{
    RefPtr<CustomFilterNumberParameter> numberParameter = CustomFilterNumberParameter::create(name);
    for (unsigned i = 0; i < values->length(); ++i) {
        CSSValue* value = values->itemWithoutBoundsCheck(i);
        if (!value->isPrimitiveValue())
            return 0;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(value);
        if (primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_NUMBER)
            return 0;
        numberParameter->addValue(primitiveValue->getDoubleValue());
    }
    return numberParameter.release();
}

PassRefPtr<CustomFilterParameter> StyleResolver::parseCustomFilterTransformParameter(const String& name, CSSValueList* values)
{
    RefPtr<CustomFilterTransformParameter> transformParameter = CustomFilterTransformParameter::create(name);
    TransformOperations operations;
    createTransformOperations(values, style(), m_rootElementStyle, operations);
    transformParameter->setOperations(operations);
    return transformParameter.release();
}

PassRefPtr<CustomFilterParameter> StyleResolver::parseCustomFilterParameter(const String& name, CSSValue* parameterValue)
{
    // FIXME: Implement other parameters types parsing.
    // booleans: https://bugs.webkit.org/show_bug.cgi?id=76438
    // textures: https://bugs.webkit.org/show_bug.cgi?id=71442
    // mat2, mat3, mat4: https://bugs.webkit.org/show_bug.cgi?id=71444
    // Number parameters are wrapped inside a CSSValueList and all
    // the other functions values inherit from CSSValueList.
    if (!parameterValue->isValueList())
        return 0;

    CSSValueList* values = static_cast<CSSValueList*>(parameterValue);
    if (!values->length())
        return 0;

    if (parameterValue->isWebKitCSSArrayFunctionValue())
        return parseCustomFilterArrayParameter(name, values);

    // If the first value of the list is a transform function,
    // then we could safely assume that all the remaining items
    // are transforms. parseCustomFilterTransformParameter will
    // return 0 if that assumption is incorrect.
    if (values->itemWithoutBoundsCheck(0)->isWebKitCSSTransformValue())
        return parseCustomFilterTransformParameter(name, values);

    // We can have only arrays of booleans or numbers, so use the first value to choose between those two.
    // We need up to 4 values (all booleans or all numbers).
    if (!values->itemWithoutBoundsCheck(0)->isPrimitiveValue() || values->length() > 4)
        return 0;
    
    CSSPrimitiveValue* firstPrimitiveValue = static_cast<CSSPrimitiveValue*>(values->itemWithoutBoundsCheck(0));
    if (firstPrimitiveValue->primitiveType() == CSSPrimitiveValue::CSS_NUMBER)
        return parseCustomFilterNumberParameter(name, values);

    // FIXME: Implement the boolean array parameter here.
    // https://bugs.webkit.org/show_bug.cgi?id=76438

    return 0;
}

bool StyleResolver::parseCustomFilterParameterList(CSSValue* parametersValue, CustomFilterParameterList& parameterList)
{
    HashSet<String> knownParameterNames;
    CSSValueListIterator parameterIterator(parametersValue);
    for (; parameterIterator.hasMore(); parameterIterator.advance()) {
        if (!parameterIterator.value()->isValueList())
            return false;
        CSSValueListIterator iterator(parameterIterator.value());
        if (!iterator.isPrimitiveValue())
            return false;
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(iterator.value());
        if (primitiveValue->primitiveType() != CSSPrimitiveValue::CSS_STRING)
            return false;
        
        String name = primitiveValue->getStringValue();
        // Do not allow duplicate parameter names.
        if (knownParameterNames.contains(name))
            return false;
        knownParameterNames.add(name);
        
        iterator.advance();
        
        if (!iterator.hasMore())
            return false;
        
        RefPtr<CustomFilterParameter> parameter = parseCustomFilterParameter(name, iterator.value());
        if (!parameter)
            return false;
        parameterList.append(parameter.release());
    }
    
    // Make sure we sort the parameters before passing them down to the CustomFilterOperation.
    std::sort(parameterList.begin(), parameterList.end(), sortParametersByNameComparator);
    
    return true;
}

PassRefPtr<CustomFilterOperation> StyleResolver::createCustomFilterOperation(WebKitCSSFilterValue* filterValue)
{
    ASSERT(filterValue->length());

    CSSValue* shadersValue = filterValue->itemWithoutBoundsCheck(0);
    ASSERT(shadersValue->isValueList());
    CSSValueList* shadersList = static_cast<CSSValueList*>(shadersValue);

    unsigned shadersListLength = shadersList->length();
    ASSERT(shadersListLength);

    RefPtr<StyleShader> vertexShader = styleShader(shadersList->itemWithoutBoundsCheck(0));
    RefPtr<StyleShader> fragmentShader;
    CustomFilterProgramType programType = PROGRAM_TYPE_BLENDS_ELEMENT_TEXTURE;
    CustomFilterProgramMixSettings mixSettings;

    if (shadersListLength > 1) {
        CSSValue* fragmentShaderOrMixFunction = shadersList->itemWithoutBoundsCheck(1);

        if (fragmentShaderOrMixFunction->isWebKitCSSMixFunctionValue()) {
            WebKitCSSMixFunctionValue* mixFunction = static_cast<WebKitCSSMixFunctionValue*>(fragmentShaderOrMixFunction);
            CSSValueListIterator iterator(mixFunction);

            ASSERT(mixFunction->length());
            fragmentShader = styleShader(iterator.value());
            iterator.advance();

            ASSERT(mixFunction->length() <= 3);
            while (iterator.hasMore()) {
                CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(iterator.value());
                if (CSSParser::isBlendMode(primitiveValue->getIdent()))
                    mixSettings.blendMode = *primitiveValue;
                else if (CSSParser::isCompositeOperator(primitiveValue->getIdent()))
                    mixSettings.compositeOperator = *primitiveValue;
                else
                    ASSERT_NOT_REACHED();
                iterator.advance();
            }
        } else {
            programType = PROGRAM_TYPE_NO_ELEMENT_TEXTURE;
            fragmentShader = styleShader(fragmentShaderOrMixFunction);
        }
    }
    
    unsigned meshRows = 1;
    unsigned meshColumns = 1;
    CustomFilterMeshBoxType meshBoxType = MeshBoxTypeFilter;
    CustomFilterMeshType meshType = MeshTypeAttached;
    
    CSSValue* parametersValue = 0;
    
    if (filterValue->length() > 1) {
        CSSValueListIterator iterator(filterValue->itemWithoutBoundsCheck(1));
        
        // The second value might be the mesh box or the list of parameters:
        // If it starts with a number or any of the mesh-box identifiers it is 
        // the mesh-box list, if not it means it is the parameters list.

        if (iterator.hasMore() && iterator.isPrimitiveValue()) {
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(iterator.value());
            if (primitiveValue->isNumber()) {
                // If only one integer value is specified, it will set both
                // the rows and the columns.
                meshColumns = meshRows = primitiveValue->getIntValue();
                iterator.advance();
                
                // Try to match another number for the rows.
                if (iterator.hasMore() && iterator.isPrimitiveValue()) {
                    CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(iterator.value());
                    if (primitiveValue->isNumber()) {
                        meshRows = primitiveValue->getIntValue();
                        iterator.advance();
                    }
                }
            }
        }
        
        if (iterator.hasMore() && iterator.isPrimitiveValue()) {
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(iterator.value());
            if (primitiveValue->getIdent() == CSSValueBorderBox
                || primitiveValue->getIdent() == CSSValuePaddingBox
                || primitiveValue->getIdent() == CSSValueContentBox
                || primitiveValue->getIdent() == CSSValueFilterBox) {
                meshBoxType = *primitiveValue;
                iterator.advance();
            }
        }
        
        if (iterator.hasMore() && iterator.isPrimitiveValue()) {
            CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(iterator.value());
            if (primitiveValue->getIdent() == CSSValueDetached) {
                meshType = MeshTypeDetached;
                iterator.advance();
            }
        }
        
        if (!iterator.index()) {
            // If no value was consumed from the mesh value, then it is just a parameter list, meaning that we end up
            // having just two CSSListValues: list of shaders and list of parameters.
            ASSERT(filterValue->length() == 2);
            parametersValue = filterValue->itemWithoutBoundsCheck(1);
        }
    }
    
    if (filterValue->length() > 2 && !parametersValue)
        parametersValue = filterValue->itemWithoutBoundsCheck(2);
    
    CustomFilterParameterList parameterList;
    if (parametersValue && !parseCustomFilterParameterList(parametersValue, parameterList))
        return 0;
    
    RefPtr<StyleCustomFilterProgram> program = StyleCustomFilterProgram::create(vertexShader.release(), fragmentShader.release(), programType, mixSettings, meshType);
    return CustomFilterOperation::create(program.release(), parameterList, meshRows, meshColumns, meshBoxType, meshType);
}
#endif

bool StyleResolver::createFilterOperations(CSSValue* inValue, RenderStyle* style, RenderStyle* rootStyle, FilterOperations& outOperations)
{
    ASSERT(outOperations.isEmpty());
    
    if (!inValue)
        return false;
    
    if (inValue->isPrimitiveValue()) {
        CSSPrimitiveValue* primitiveValue = static_cast<CSSPrimitiveValue*>(inValue);
        if (primitiveValue->getIdent() == CSSValueNone)
            return true;
    }
    
    if (!inValue->isValueList())
        return false;

    float zoomFactor = style ? style->effectiveZoom() : 1;
    FilterOperations operations;
    for (CSSValueListIterator i = inValue; i.hasMore(); i.advance()) {
        CSSValue* currValue = i.value();
        if (!currValue->isWebKitCSSFilterValue())
            continue;

        WebKitCSSFilterValue* filterValue = static_cast<WebKitCSSFilterValue*>(i.value());
        FilterOperation::OperationType operationType = filterOperationForType(filterValue->operationType());

#if ENABLE(CSS_SHADERS)
        if (operationType == FilterOperation::VALIDATED_CUSTOM) {
            // ValidatedCustomFilterOperation is not supposed to end up in the RenderStyle.
            ASSERT_NOT_REACHED();
            continue;
        }
        if (operationType == FilterOperation::CUSTOM) {
            RefPtr<CustomFilterOperation> operation = createCustomFilterOperation(filterValue);
            if (!operation)
                return false;
            
            operations.operations().append(operation);
            continue;
        }
#endif
        if (operationType == FilterOperation::REFERENCE) {
#if ENABLE(SVG)
            if (filterValue->length() != 1)
                continue;
            CSSValue* argument = filterValue->itemWithoutBoundsCheck(0);

            if (!argument->isWebKitCSSSVGDocumentValue())
                continue;

            WebKitCSSSVGDocumentValue* svgDocumentValue = static_cast<WebKitCSSSVGDocumentValue*>(argument);
            KURL url = m_element->document()->completeURL(svgDocumentValue->url());

            RefPtr<ReferenceFilterOperation> operation = ReferenceFilterOperation::create(svgDocumentValue->url(), url.fragmentIdentifier(), operationType);
            if (SVGURIReference::isExternalURIReference(svgDocumentValue->url(), m_element->document())) {
                if (!svgDocumentValue->loadRequested())
                    m_pendingSVGDocuments.set(operation.get(), svgDocumentValue);
                else if (svgDocumentValue->cachedSVGDocument())
                    operation->setCachedSVGDocumentReference(adoptPtr(new CachedSVGDocumentReference(svgDocumentValue->cachedSVGDocument())));
            }
            operations.operations().append(operation);
#endif
            continue;
        }

        // Check that all parameters are primitive values, with the
        // exception of drop shadow which has a ShadowValue parameter.
        if (operationType != FilterOperation::DROP_SHADOW) {
            bool haveNonPrimitiveValue = false;
            for (unsigned j = 0; j < filterValue->length(); ++j) {
                if (!filterValue->itemWithoutBoundsCheck(j)->isPrimitiveValue()) {
                    haveNonPrimitiveValue = true;
                    break;
                }
            }
            if (haveNonPrimitiveValue)
                continue;
        }

        CSSPrimitiveValue* firstValue = filterValue->length() ? static_cast<CSSPrimitiveValue*>(filterValue->itemWithoutBoundsCheck(0)) : 0;
        switch (filterValue->operationType()) {
        case WebKitCSSFilterValue::GrayscaleFilterOperation:
        case WebKitCSSFilterValue::SepiaFilterOperation:
        case WebKitCSSFilterValue::SaturateFilterOperation: {
            double amount = 1;
            if (filterValue->length() == 1) {
                amount = firstValue->getDoubleValue();
                if (firstValue->isPercentage())
                    amount /= 100;
            }

            operations.operations().append(BasicColorMatrixFilterOperation::create(amount, operationType));
            break;
        }
        case WebKitCSSFilterValue::HueRotateFilterOperation: {
            double angle = 0;
            if (filterValue->length() == 1)
                angle = firstValue->computeDegrees();

            operations.operations().append(BasicColorMatrixFilterOperation::create(angle, operationType));
            break;
        }
        case WebKitCSSFilterValue::InvertFilterOperation:
        case WebKitCSSFilterValue::BrightnessFilterOperation:
        case WebKitCSSFilterValue::ContrastFilterOperation:
        case WebKitCSSFilterValue::OpacityFilterOperation: {
            double amount = (filterValue->operationType() == WebKitCSSFilterValue::BrightnessFilterOperation) ? 0 : 1;
            if (filterValue->length() == 1) {
                amount = firstValue->getDoubleValue();
                if (firstValue->isPercentage())
                    amount /= 100;
            }

            operations.operations().append(BasicComponentTransferFilterOperation::create(amount, operationType));
            break;
        }
        case WebKitCSSFilterValue::BlurFilterOperation: {
            Length stdDeviation = Length(0, Fixed);
            if (filterValue->length() >= 1)
                stdDeviation = convertToFloatLength(firstValue, style, rootStyle, zoomFactor);
            if (stdDeviation.isUndefined())
                return false;

            operations.operations().append(BlurFilterOperation::create(stdDeviation, operationType));
            break;
        }
        case WebKitCSSFilterValue::DropShadowFilterOperation: {
            if (filterValue->length() != 1)
                return false;

            CSSValue* cssValue = filterValue->itemWithoutBoundsCheck(0);
            if (!cssValue->isShadowValue())
                continue;

            ShadowValue* item = static_cast<ShadowValue*>(cssValue);
            IntPoint location(item->x->computeLength<int>(style, rootStyle, zoomFactor),
                              item->y->computeLength<int>(style, rootStyle, zoomFactor));
            int blur = item->blur ? item->blur->computeLength<int>(style, rootStyle, zoomFactor) : 0;
            Color color;
            if (item->color)
                color = colorFromPrimitiveValue(item->color.get());

            operations.operations().append(DropShadowFilterOperation::create(location, blur, color.isValid() ? color : Color::transparent, operationType));
            break;
        }
        case WebKitCSSFilterValue::UnknownFilterOperation:
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    outOperations = operations;
    return true;
}

#endif

PassRefPtr<StyleImage> StyleResolver::loadPendingImage(StylePendingImage* pendingImage)
{
    CachedResourceLoader* cachedResourceLoader = m_element->document()->cachedResourceLoader();

    if (pendingImage->cssImageValue()) {
        CSSImageValue* imageValue = pendingImage->cssImageValue();
        return imageValue->cachedImage(cachedResourceLoader, m_element);
    }

    if (pendingImage->cssImageGeneratorValue()) {
        CSSImageGeneratorValue* imageGeneratorValue = pendingImage->cssImageGeneratorValue();
        imageGeneratorValue->loadSubimages(cachedResourceLoader);
        return StyleGeneratedImage::create(imageGeneratorValue);
    }

#if ENABLE(CSS_IMAGE_SET)
    if (pendingImage->cssImageSetValue()) {
        CSSImageSetValue* imageSetValue = pendingImage->cssImageSetValue();
        return imageSetValue->cachedImageSet(cachedResourceLoader);
    }
#endif

    return 0;
}

void StyleResolver::loadPendingImages()
{
    if (m_pendingImageProperties.isEmpty())
        return;

    PendingImagePropertyMap::const_iterator::Keys end = m_pendingImageProperties.end().keys();
    for (PendingImagePropertyMap::const_iterator::Keys it = m_pendingImageProperties.begin().keys(); it != end; ++it) {
        CSSPropertyID currentProperty = *it;

        switch (currentProperty) {
        case CSSPropertyBackgroundImage: {
            for (FillLayer* backgroundLayer = m_style->accessBackgroundLayers(); backgroundLayer; backgroundLayer = backgroundLayer->next()) {
                if (backgroundLayer->image() && backgroundLayer->image()->isPendingImage())
                    backgroundLayer->setImage(loadPendingImage(static_cast<StylePendingImage*>(backgroundLayer->image())));
            }
            break;
        }
        case CSSPropertyContent: {
            for (ContentData* contentData = const_cast<ContentData*>(m_style->contentData()); contentData; contentData = contentData->next()) {
                if (contentData->isImage()) {
                    StyleImage* image = static_cast<ImageContentData*>(contentData)->image();
                    if (image->isPendingImage()) {
                        RefPtr<StyleImage> loadedImage = loadPendingImage(static_cast<StylePendingImage*>(image));
                        if (loadedImage)
                            static_cast<ImageContentData*>(contentData)->setImage(loadedImage.release());
                    }
                }
            }
            break;
        }
        case CSSPropertyCursor: {
            if (CursorList* cursorList = m_style->cursors()) {
                for (size_t i = 0; i < cursorList->size(); ++i) {
                    CursorData& currentCursor = cursorList->at(i);
                    if (StyleImage* image = currentCursor.image()) {
                        if (image->isPendingImage())
                            currentCursor.setImage(loadPendingImage(static_cast<StylePendingImage*>(image)));
                    }
                }
            }
            break;
        }
        case CSSPropertyListStyleImage: {
            if (m_style->listStyleImage() && m_style->listStyleImage()->isPendingImage())
                m_style->setListStyleImage(loadPendingImage(static_cast<StylePendingImage*>(m_style->listStyleImage())));
            break;
        }
        case CSSPropertyBorderImageSource: {
            if (m_style->borderImageSource() && m_style->borderImageSource()->isPendingImage())
                m_style->setBorderImageSource(loadPendingImage(static_cast<StylePendingImage*>(m_style->borderImageSource())));
            break;
        }
        case CSSPropertyWebkitBoxReflect: {
            if (StyleReflection* reflection = m_style->boxReflect()) {
                const NinePieceImage& maskImage = reflection->mask();
                if (maskImage.image() && maskImage.image()->isPendingImage()) {
                    RefPtr<StyleImage> loadedImage = loadPendingImage(static_cast<StylePendingImage*>(maskImage.image()));
                    reflection->setMask(NinePieceImage(loadedImage.release(), maskImage.imageSlices(), maskImage.fill(), maskImage.borderSlices(), maskImage.outset(), maskImage.horizontalRule(), maskImage.verticalRule()));
                }
            }
            break;
        }
        case CSSPropertyWebkitMaskBoxImageSource: {
            if (m_style->maskBoxImageSource() && m_style->maskBoxImageSource()->isPendingImage())
                m_style->setMaskBoxImageSource(loadPendingImage(static_cast<StylePendingImage*>(m_style->maskBoxImageSource())));
            break;
        }
        case CSSPropertyWebkitMaskImage: {
            for (FillLayer* maskLayer = m_style->accessMaskLayers(); maskLayer; maskLayer = maskLayer->next()) {
                if (maskLayer->image() && maskLayer->image()->isPendingImage())
                    maskLayer->setImage(loadPendingImage(static_cast<StylePendingImage*>(maskLayer->image())));
            }
            break;
        }
        default:
            ASSERT_NOT_REACHED();
        }
    }

    m_pendingImageProperties.clear();
}

void StyleResolver::loadPendingResources()
{
    // Start loading images referenced by this style.
    loadPendingImages();

#if ENABLE(CSS_SHADERS)
    // Start loading the shaders referenced by this style.
    loadPendingShaders();
#endif
    
#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
    // Start loading the SVG Documents referenced by this style.
    loadPendingSVGDocuments();
#endif
}

void StyleResolver::MatchedProperties::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(properties);
}

void StyleResolver::MatchedPropertiesCacheItem::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(matchedProperties);
}

void MediaQueryResult::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(m_expression);
}

void StyleResolver::reportMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(m_style);
    info.addMember(m_authorStyle);
    info.addMember(m_userStyle);
    info.addMember(m_siblingRuleSet);
    info.addMember(m_uncommonAttributeRuleSet);
    info.addMember(m_keyframesRuleMap);
    info.addMember(m_matchedPropertiesCache);
    info.addMember(m_matchedRules);

    info.addMember(m_ruleList);
    info.addMember(m_pendingImageProperties);
    info.addMember(m_lineHeightValue);
    info.addMember(m_viewportDependentMediaQueryResults);
    info.addMember(m_styleRuleToCSSOMWrapperMap);
    info.addMember(m_styleSheetCSSOMWrapperSet);
#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
    info.addMember(m_pendingSVGDocuments);
#endif
    info.addMember(m_scopeResolver);

    // FIXME: move this to a place where it would be called only once?
    info.addMember(defaultStyle);
    info.addMember(defaultQuirksStyle);
    info.addMember(defaultPrintStyle);
    info.addMember(defaultViewSourceStyle);
}

} // namespace WebCore
