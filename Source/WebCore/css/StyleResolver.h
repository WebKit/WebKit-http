/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 *
 */

#ifndef StyleResolver_h
#define StyleResolver_h

#include "CSSRule.h"
#include "CSSToStyleMap.h"
#include "CSSValueList.h"
#include "LinkHash.h"
#include "MediaQueryExp.h"
#include "RenderStyle.h"
#include "SelectorChecker.h"
#include "StyleInheritedData.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

enum ESmartMinimumForFontSize { DoNotUseSmartMinimumForFontSize, UseSmartMinimumForFontFize };

class CSSFontSelector;
class CSSPageRule;
class CSSPrimitiveValue;
class CSSProperty;
class CSSRuleList;
class CSSFontFace;
class CSSFontFaceRule;
class CSSImageGeneratorValue;
class CSSImageSetValue;
class CSSImageValue;
class CSSSelector;
class CSSStyleRule;
class CSSStyleSheet;
class CSSValue;
class ContainerNode;
class CustomFilterOperation;
class CustomFilterParameter;
class Document;
class Element;
class Frame;
class FrameView;
class KURL;
class KeyframeList;
class KeyframeValue;
class MediaQueryEvaluator;
class Node;
class RenderRegion;
class RuleData;
class RuleSet;
class Settings;
class StaticCSSRuleList;
class StyleBuilder;
class StyleImage;
class StyleKeyframe;
class StylePendingImage;
class StylePropertySet;
class StyleRule;
class StyleRuleKeyframes;
class StyleRulePage;
class StyleRuleRegion;
class StyleShader;
class StyleSheet;
class StyleSheetContents;
class StyleSheetList;
class StyledElement;
class WebKitCSSFilterValue;
class WebKitCSSShaderValue;
class WebKitCSSSVGDocumentValue;

#if ENABLE(CSS_SHADERS)
typedef Vector<RefPtr<CustomFilterParameter> > CustomFilterParameterList;
#endif

class MediaQueryResult {
    WTF_MAKE_NONCOPYABLE(MediaQueryResult); WTF_MAKE_FAST_ALLOCATED;
public:
    MediaQueryResult(const MediaQueryExp& expr, bool result)
        : m_expression(expr)
        , m_result(result)
    {
    }
    void reportMemoryUsage(MemoryObjectInfo*) const;

    MediaQueryExp m_expression;
    bool m_result;
};

enum StyleSharingBehavior {
    AllowStyleSharing,
    DisallowStyleSharing,
};

// MatchOnlyUserAgentRules is used in media queries, where relative units
// are interpreted according to the document root element style, and styled only
// from the User Agent Stylesheet rules.

enum RuleMatchingBehavior {
    MatchAllRules,
    MatchAllRulesExcludingSMIL,
    MatchOnlyUserAgentRules,
};

// This class selects a RenderStyle for a given element based on a collection of stylesheets.
class StyleResolver {
    WTF_MAKE_NONCOPYABLE(StyleResolver); WTF_MAKE_FAST_ALLOCATED;
public:
    StyleResolver(Document*, bool matchAuthorAndUserStyles);
    ~StyleResolver();

    // Using these during tree walk will allow style selector to optimize child and descendant selector lookups.
    void pushParentElement(Element*);
    void popParentElement(Element*);
    void pushParentShadowRoot(const ShadowRoot*);
    void popParentShadowRoot(const ShadowRoot*);

    PassRefPtr<RenderStyle> styleForElement(Element*, RenderStyle* parentStyle = 0, StyleSharingBehavior = AllowStyleSharing,
        RuleMatchingBehavior = MatchAllRules, RenderRegion* regionForStyling = 0);

    void keyframeStylesForAnimation(Element*, const RenderStyle*, KeyframeList&);

    PassRefPtr<RenderStyle> pseudoStyleForElement(PseudoId, Element*, RenderStyle* parentStyle);

    PassRefPtr<RenderStyle> styleForPage(int pageIndex);

    static PassRefPtr<RenderStyle> styleForDocument(Document*, CSSFontSelector* = 0);

    RenderStyle* style() const { return m_style.get(); }
    RenderStyle* parentStyle() const { return m_parentStyle; }
    RenderStyle* rootElementStyle() const { return m_rootElementStyle; }
    Element* element() const { return m_element; }
    Document* document() const { return m_checker.document(); }
    const FontDescription& fontDescription() { return style()->fontDescription(); }
    const FontDescription& parentFontDescription() { return parentStyle()->fontDescription(); }
    void setFontDescription(const FontDescription& fontDescription) { m_fontDirty |= style()->setFontDescription(fontDescription); }
    void setZoom(float f) { m_fontDirty |= style()->setZoom(f); }
    void setEffectiveZoom(float f) { m_fontDirty |= style()->setEffectiveZoom(f); }
    void setTextSizeAdjust(bool b) { m_fontDirty |= style()->setTextSizeAdjust(b); }
    bool hasParentNode() const { return m_parentNode; }
    
    void appendAuthorStylesheets(unsigned firstNew, const Vector<RefPtr<StyleSheet> >&);
    
    // Find the ids or classes the selectors on a stylesheet are scoped to. The selectors only apply to elements in subtrees where the root element matches the scope.
    static bool determineStylesheetSelectorScopes(StyleSheetContents*, HashSet<AtomicStringImpl*>& idScopes, HashSet<AtomicStringImpl*>& classScopes);

private:
    void initForStyleResolve(Element*, RenderStyle* parentStyle = 0, PseudoId = NOPSEUDO);
    void initElement(Element*);
    void collectFeatures();
    RenderStyle* locateSharedStyle();
    bool matchesRuleSet(RuleSet*);
    Node* locateCousinList(Element* parent, unsigned& visitedNodeCount) const;
    StyledElement* findSiblingForStyleSharing(Node*, unsigned& count) const;
    bool canShareStyleWithElement(StyledElement*) const;

    PassRefPtr<RenderStyle> styleForKeyframe(const RenderStyle*, const StyleKeyframe*, KeyframeValue&);

#if ENABLE(STYLE_SCOPED)
    void pushScope(const ContainerNode* scope, const ContainerNode* scopeParent);
    void popScope(const ContainerNode* scope);
#else
    void pushScope(const ContainerNode*, const ContainerNode*) { }
    void popScope(const ContainerNode*) { }
#endif

public:
    // These methods will give back the set of rules that matched for a given element (or a pseudo-element).
    enum CSSRuleFilter {
        UAAndUserCSSRules   = 1 << 1,
        AuthorCSSRules      = 1 << 2,
        EmptyCSSRules       = 1 << 3,
        CrossOriginCSSRules = 1 << 4,
        AllButEmptyCSSRules = UAAndUserCSSRules | AuthorCSSRules | CrossOriginCSSRules,
        AllCSSRules         = AllButEmptyCSSRules | EmptyCSSRules,
    };
    PassRefPtr<CSSRuleList> styleRulesForElement(Element*, unsigned rulesToInclude = AllButEmptyCSSRules);
    PassRefPtr<CSSRuleList> pseudoStyleRulesForElement(Element*, PseudoId, unsigned rulesToInclude = AllButEmptyCSSRules);

    // Given a CSS keyword in the range (xx-small to -webkit-xxx-large), this function will return
    // the correct font size scaled relative to the user's default (medium).
    static float fontSizeForKeyword(Document*, int keyword, bool shouldUseFixedDefaultSize);

    // Given a font size in pixel, this function will return legacy font size between 1 and 7.
    static int legacyFontSize(Document*, int pixelFontSize, bool shouldUseFixedDefaultSize);

public:
    void applyPropertyToStyle(CSSPropertyID, CSSValue*, RenderStyle*);

    void applyPropertyToCurrentStyle(CSSPropertyID, CSSValue*);

    void updateFont();
    void initializeFontStyle(Settings*);

    static float getComputedSizeFromSpecifiedSize(Document*, float zoomFactor, bool isAbsoluteSize, float specifiedSize, ESmartMinimumForFontSize = UseSmartMinimumForFontFize);

    void setFontSize(FontDescription&, float size);

private:
    static float getComputedSizeFromSpecifiedSize(Document*, RenderStyle*, bool isAbsoluteSize, float specifiedSize, bool useSVGZoomRules);

public:
    bool useSVGZoomRules();

    static bool colorFromPrimitiveValueIsDerivedFromElement(CSSPrimitiveValue*);
    Color colorFromPrimitiveValue(CSSPrimitiveValue*, bool forVisitedLink = false) const;

    bool hasSelectorForAttribute(const AtomicString&) const;

    CSSFontSelector* fontSelector() const { return m_fontSelector.get(); }

    void addViewportDependentMediaQueryResult(const MediaQueryExp*, bool result);

    bool affectedByViewportChange() const;

    void allVisitedStateChanged() { m_checker.allVisitedStateChanged(); }
    void visitedStateChanged(LinkHash visitedHash) { m_checker.visitedStateChanged(visitedHash); }

    void addKeyframeStyle(PassRefPtr<StyleRuleKeyframes>);

    bool checkRegionStyle(Element* regionElement);

    bool usesSiblingRules() const { return !m_features.siblingRules.isEmpty(); }
    bool usesFirstLineRules() const { return m_features.usesFirstLineRules; }
    bool usesBeforeAfterRules() const { return m_features.usesBeforeAfterRules; }
    bool usesLinkRules() const { return m_features.usesLinkRules; }

    static bool createTransformOperations(CSSValue* inValue, RenderStyle* inStyle, RenderStyle* rootStyle, TransformOperations& outOperations);
    
    void invalidateMatchedPropertiesCache();
    
    // WARNING. This will construct CSSOM wrappers for all style rules and cache then in a map for significant memory cost.
    // It is here to support inspector. Don't use for any regular engine functions.
    CSSStyleRule* ensureFullCSSOMWrapperForInspector(StyleRule*);

#if ENABLE(CSS_FILTERS)
    bool createFilterOperations(CSSValue* inValue, RenderStyle* inStyle, RenderStyle* rootStyle, FilterOperations& outOperations);
#if ENABLE(CSS_SHADERS)
    StyleShader* styleShader(CSSValue*);
    StyleShader* cachedOrPendingStyleShaderFromValue(WebKitCSSShaderValue*);
    bool parseCustomFilterParameterList(CSSValue*, CustomFilterParameterList&);
    PassRefPtr<CustomFilterParameter> parseCustomFilterParameter(const String& name, CSSValue*);
    PassRefPtr<CustomFilterParameter> parseCustomFilterArrayParameter(const String& name, CSSValueList*);
    PassRefPtr<CustomFilterParameter> parseCustomFilterNumberParameter(const String& name, CSSValueList*);
    PassRefPtr<CustomFilterParameter> parseCustomFilterTransformParameter(const String& name, CSSValueList*);
    PassRefPtr<CustomFilterOperation> createCustomFilterOperation(WebKitCSSFilterValue*);
    void loadPendingShaders();
#endif
#if ENABLE(SVG)
    void loadPendingSVGDocuments();
#endif
#endif // ENABLE(CSS_FILTERS)

    void loadPendingResources();

    struct RuleFeature {
        RuleFeature(StyleRule* rule, unsigned selectorIndex, bool hasDocumentSecurityOrigin)
            : rule(rule)
            , selectorIndex(selectorIndex)
            , hasDocumentSecurityOrigin(hasDocumentSecurityOrigin) 
        { 
        }
        StyleRule* rule;
        unsigned selectorIndex;
        bool hasDocumentSecurityOrigin;
    };
    struct Features {
        Features();
        ~Features();
        void add(const StyleResolver::Features&);
        void clear();
        void reportMemoryUsage(MemoryObjectInfo*) const;
        HashSet<AtomicStringImpl*> idsInRules;
        HashSet<AtomicStringImpl*> attrsInRules;
        Vector<RuleFeature> siblingRules;
        Vector<RuleFeature> uncommonAttributeRules;
        bool usesFirstLineRules;
        bool usesBeforeAfterRules;
        bool usesLinkRules;
    };

private:
    // This function fixes up the default font size if it detects that the current generic font family has changed. -dwh
    void checkForGenericFamilyChange(RenderStyle*, RenderStyle* parentStyle);
    void checkForZoomChange(RenderStyle*, RenderStyle* parentStyle);
    void checkForTextSizeAdjust();

    void adjustRenderStyle(RenderStyle* styleToAdjust, RenderStyle* parentStyle, Element*);

    void addMatchedRule(const RuleData* rule) { m_matchedRules.append(rule); }

    struct MatchRanges {
        MatchRanges() : firstUARule(-1), lastUARule(-1), firstAuthorRule(-1), lastAuthorRule(-1), firstUserRule(-1), lastUserRule(-1) { }
        int firstUARule;
        int lastUARule;
        int firstAuthorRule;
        int lastAuthorRule;
        int firstUserRule;
        int lastUserRule;
    };

    struct MatchedProperties {
        MatchedProperties() : possiblyPaddedMember(0) { }
        void reportMemoryUsage(MemoryObjectInfo*) const;
        
        RefPtr<StylePropertySet> properties;
        union {
            struct {
                unsigned linkMatchType : 2;
                unsigned isInRegionRule : 1;
            };
            // Used to make sure all memory is zero-initialized since we compute the hash over the bytes of this object.
            void* possiblyPaddedMember;
        };
    };

    struct MatchResult {
        MatchResult() : isCacheable(true) { }
        Vector<MatchedProperties, 64> matchedProperties;
        Vector<StyleRule*, 64> matchedRules;
        MatchRanges ranges;
        bool isCacheable;
    };

    struct MatchOptions {
        MatchOptions(bool includeEmptyRules, const ContainerNode* scope = 0) : scope(scope), includeEmptyRules(includeEmptyRules) { }
        const ContainerNode* scope;
        bool includeEmptyRules;
    };

    static void addMatchedProperties(MatchResult&, const StylePropertySet* properties, StyleRule* = 0, unsigned linkMatchType = SelectorChecker::MatchAll, bool inRegionRule = false);
    void addElementStyleProperties(MatchResult&, const StylePropertySet*, bool isCacheable = true);

    void matchAllRules(MatchResult&, bool includeSMILProperties);
    void matchUARules(MatchResult&);
    void matchUARules(MatchResult&, RuleSet*);
    void matchAuthorRules(MatchResult&, bool includeEmptyRules);
    void matchUserRules(MatchResult&, bool includeEmptyRules);
    void matchScopedAuthorRules(MatchResult&, bool includeEmptyRules);
    void collectMatchingRules(RuleSet*, int& firstRuleIndex, int& lastRuleIndex, const MatchOptions&);
    void collectMatchingRulesForRegion(RuleSet*, int& firstRuleIndex, int& lastRuleIndex, const MatchOptions&);
    void collectMatchingRulesForList(const Vector<RuleData>*, int& firstRuleIndex, int& lastRuleIndex, const MatchOptions&);
    bool fastRejectSelector(const RuleData&) const;
    void sortMatchedRules();
    void sortAndTransferMatchedRules(MatchResult&);

    bool checkSelector(const RuleData&, const ContainerNode* scope = 0);
    bool checkRegionSelector(CSSSelector* regionSelector, Element* regionElement);
    void applyMatchedProperties(const MatchResult&, const Element*);
    enum StyleApplicationPass {
#if ENABLE(CSS_VARIABLES)
        VariableDefinitions,
#endif
        HighPriorityProperties,
        LowPriorityProperties
    };
    template <StyleApplicationPass pass>
    void applyMatchedProperties(const MatchResult&, bool important, int startIndex, int endIndex, bool inheritedOnly);
    template <StyleApplicationPass pass>
    void applyProperties(const StylePropertySet* properties, StyleRule*, bool isImportant, bool inheritedOnly, bool filterRegionProperties);
#if ENABLE(CSS_VARIABLES)
    void resolveVariables(CSSPropertyID, CSSValue*, Vector<std::pair<CSSPropertyID, String> >& knownExpressions);
#endif
    static bool isValidRegionStyleProperty(CSSPropertyID);

    void matchPageRules(MatchResult&, RuleSet*, bool isLeftPage, bool isFirstPage, const String& pageName);
    void matchPageRulesForList(Vector<StyleRulePage*>& matchedRules, const Vector<StyleRulePage*>&, bool isLeftPage, bool isFirstPage, const String& pageName);
    Settings* documentSettings() { return m_checker.document()->settings(); }

    bool isLeftPage(int pageIndex) const;
    bool isRightPage(int pageIndex) const { return !isLeftPage(pageIndex); }
    bool isFirstPage(int pageIndex) const;
    String pageName(int pageIndex) const;

    OwnPtr<RuleSet> m_authorStyle;
    OwnPtr<RuleSet> m_userStyle;

    Features m_features;
    OwnPtr<RuleSet> m_siblingRuleSet;
    OwnPtr<RuleSet> m_uncommonAttributeRuleSet;

    bool m_hasUAAppearance;
    BorderData m_borderData;
    FillLayer m_backgroundData;
    Color m_backgroundColor;

    typedef HashMap<AtomicStringImpl*, RefPtr<StyleRuleKeyframes> > KeyframesRuleMap;
    KeyframesRuleMap m_keyframesRuleMap;

public:
    static RenderStyle* styleNotYetAvailable() { return s_styleNotYetAvailable; }

    PassRefPtr<StyleImage> styleImage(CSSPropertyID, CSSValue*);
    PassRefPtr<StyleImage> cachedOrPendingFromValue(CSSPropertyID, CSSImageValue*);
    PassRefPtr<StyleImage> generatedOrPendingFromValue(CSSPropertyID, CSSImageGeneratorValue*);
#if ENABLE(CSS_IMAGE_SET)
    PassRefPtr<StyleImage> setOrPendingFromValue(CSSPropertyID, CSSImageSetValue*);
#endif

    bool applyPropertyToRegularStyle() const { return m_applyPropertyToRegularStyle; }
    bool applyPropertyToVisitedLinkStyle() const { return m_applyPropertyToVisitedLinkStyle; }

    static Length convertToIntLength(CSSPrimitiveValue*, RenderStyle*, RenderStyle* rootStyle, double multiplier = 1);
    static Length convertToFloatLength(CSSPrimitiveValue*, RenderStyle*, RenderStyle* rootStyle, double multiplier = 1);

    CSSToStyleMap* styleMap() { return &m_styleMap; }

    void reportMemoryUsage(MemoryObjectInfo*) const;
    
private:
    static RenderStyle* s_styleNotYetAvailable;

    void addStylesheetsFromSeamlessParents();
    void addAuthorRulesAndCollectUserRulesFromSheets(const Vector<RefPtr<CSSStyleSheet> >*, RuleSet& userStyle);

    void cacheBorderAndBackground();

private:
    bool canShareStyleWithControl(StyledElement*) const;

    void applyProperty(CSSPropertyID, CSSValue*);

#if ENABLE(SVG)
    void applySVGProperty(CSSPropertyID, CSSValue*);
#endif

    PassRefPtr<StyleImage> loadPendingImage(StylePendingImage*);
    void loadPendingImages();

    static unsigned computeMatchedPropertiesHash(const MatchedProperties*, unsigned size);
    struct MatchedPropertiesCacheItem {
        void reportMemoryUsage(MemoryObjectInfo*) const;
        Vector<MatchedProperties> matchedProperties;
        MatchRanges ranges;
        RefPtr<RenderStyle> renderStyle;
        RefPtr<RenderStyle> parentRenderStyle;
    };
    const MatchedPropertiesCacheItem* findFromMatchedPropertiesCache(unsigned hash, const MatchResult&);
    void addToMatchedPropertiesCache(const RenderStyle*, const RenderStyle* parentStyle, unsigned hash, const MatchResult&);

    // Every N additions to the matched declaration cache trigger a sweep where entries holding
    // the last reference to a style declaration are garbage collected.
    void sweepMatchedPropertiesCache();

    unsigned m_matchedPropertiesCacheAdditionsSinceLastSweep;

    typedef HashMap<unsigned, MatchedPropertiesCacheItem> MatchedPropertiesCache;
    MatchedPropertiesCache m_matchedPropertiesCache;

    // A buffer used to hold the set of matched rules for an element, and a temporary buffer used for
    // merge sorting.
    Vector<const RuleData*, 32> m_matchedRules;

    RefPtr<StaticCSSRuleList> m_ruleList;

    typedef HashMap<CSSPropertyID, RefPtr<CSSValue> > PendingImagePropertyMap;
    PendingImagePropertyMap m_pendingImageProperties;

    OwnPtr<MediaQueryEvaluator> m_medium;
    RefPtr<RenderStyle> m_rootDefaultStyle;

    PseudoId m_dynamicPseudo;
    PseudoId m_pseudoStyle;

    SelectorChecker m_checker;

    RefPtr<RenderStyle> m_style;
    RenderStyle* m_parentStyle;
    RenderStyle* m_rootElementStyle;
    Element* m_element;
    StyledElement* m_styledElement;
    RenderRegion* m_regionForStyling;
    EInsideLink m_elementLinkState;
    ContainerNode* m_parentNode;
    CSSValue* m_lineHeightValue;
    bool m_fontDirty;
    bool m_matchAuthorAndUserStyles;
    bool m_sameOriginOnly;
    bool m_distributedToInsertionPoint;
    bool m_hasUnknownPseudoElements;

    RefPtr<CSSFontSelector> m_fontSelector;
    Vector<OwnPtr<MediaQueryResult> > m_viewportDependentMediaQueryResults;

    bool m_applyPropertyToRegularStyle;
    bool m_applyPropertyToVisitedLinkStyle;
    const StyleBuilder& m_styleBuilder;
    
    HashMap<StyleRule*, RefPtr<CSSStyleRule> > m_styleRuleToCSSOMWrapperMap;
    HashSet<RefPtr<CSSStyleSheet> > m_styleSheetCSSOMWrapperSet;

#if ENABLE(CSS_SHADERS)
    bool m_hasPendingShaders;
#endif

#if ENABLE(CSS_FILTERS) && ENABLE(SVG)
    HashMap<FilterOperation*, WebKitCSSSVGDocumentValue*> m_pendingSVGDocuments;
#endif

#if ENABLE(STYLE_SCOPED)
    const ContainerNode* determineScope(const CSSStyleSheet*);

    typedef HashMap<const ContainerNode*, OwnPtr<RuleSet> > ScopedRuleSetMap;

    RuleSet* ruleSetForScope(const ContainerNode*) const;

    void setupScopeStack(const ContainerNode*);
    bool scopeStackIsConsistent(const ContainerNode* parent) const { return parent && parent == m_scopeStackParent; }

    ScopedRuleSetMap m_scopedAuthorStyles;
    
    struct ScopeStackFrame {
        ScopeStackFrame() : m_scope(0), m_authorStyleBoundsIndex(0), m_ruleSet(0) { }
        ScopeStackFrame(const ContainerNode* scope, int authorStyleBoundsIndex, RuleSet* ruleSet) : m_scope(scope), m_authorStyleBoundsIndex(authorStyleBoundsIndex), m_ruleSet(ruleSet) { }
        const ContainerNode* m_scope;
        int m_authorStyleBoundsIndex;
        RuleSet* m_ruleSet;
    };
    // Vector (used as stack) that keeps track of scoping elements (i.e., elements with a <style scoped> child)
    // encountered during tree iteration for style resolution.
    Vector<ScopeStackFrame> m_scopeStack;
    // Element last seen as parent element when updating m_scopingElementStack.
    // This is used to decide whether m_scopingElementStack is consistent, separately from SelectorChecker::m_parentStack.
    const ContainerNode* m_scopeStackParent;
    int m_scopeStackParentBoundsIndex;
#endif

    CSSToStyleMap m_styleMap;

    friend class StyleBuilder;
    friend bool operator==(const MatchedProperties&, const MatchedProperties&);
    friend bool operator!=(const MatchedProperties&, const MatchedProperties&);
    friend bool operator==(const MatchRanges&, const MatchRanges&);
    friend bool operator!=(const MatchRanges&, const MatchRanges&);
};

} // namespace WebCore

#endif // StyleResolver_h
