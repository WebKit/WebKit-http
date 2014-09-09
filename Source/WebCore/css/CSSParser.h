/*
 * Copyright (C) 2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 - 2010  Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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

#ifndef CSSParser_h
#define CSSParser_h

#include "CSSCalculationValue.h"
#include "CSSGradientValue.h"
#include "CSSParserMode.h"
#include "CSSParserValues.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSValueKeywords.h"
#include "Color.h"
#include "MediaQuery.h"
#include "SourceSizeList.h"
#include <memory>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>

#if ENABLE(CSS_FILTERS)
#include "WebKitCSSFilterValue.h"
#endif

#if ENABLE(CSS_GRID_LAYOUT)
#include "CSSGridTemplateAreasValue.h"
#endif

namespace WebCore {

class AnimationParseContext;
class CSSBorderImageSliceValue;
class CSSPrimitiveValue;
class CSSSelectorList;
class CSSValue;
class CSSValueList;
class CSSBasicShape;
class CSSBasicShapeInset;
class CSSGridLineNamesValue;
class Document;
class Element;
class ImmutableStyleProperties;
class MediaQueryExp;
class MediaQuerySet;
class MutableStyleProperties;
class StyleKeyframe;
class StylePropertyShorthand;
class StyleRuleBase;
class StyleRuleKeyframes;
class StyleKeyframe;
class StyleSheetContents;
class StyledElement;

class CSSParser {
    friend inline int cssyylex(void*, CSSParser*);

public:
    struct Location;
    enum SyntaxErrorType {
        PropertyDeclarationError,
        GeneralSyntaxError
    };

    CSSParser(const CSSParserContext&);

    ~CSSParser();

    void parseSheet(StyleSheetContents*, const String&, int startLineNumber = 0, RuleSourceDataList* = 0, bool = false);
    PassRefPtr<StyleRuleBase> parseRule(StyleSheetContents*, const String&);
    PassRefPtr<StyleKeyframe> parseKeyframeRule(StyleSheetContents*, const String&);
#if ENABLE(CSS3_CONDITIONAL_RULES)
    bool parseSupportsCondition(const String&);
#endif
    static bool parseValue(MutableStyleProperties*, CSSPropertyID, const String&, bool important, CSSParserMode, StyleSheetContents*);
    static bool parseColor(RGBA32& color, const String&, bool strict = false);
    static bool parseSystemColor(RGBA32& color, const String&, Document*);
    static PassRefPtr<CSSValueList> parseFontFaceValue(const AtomicString&);
    PassRefPtr<CSSPrimitiveValue> parseValidPrimitive(CSSValueID ident, CSSParserValue*);
    bool parseDeclaration(MutableStyleProperties*, const String&, PassRefPtr<CSSRuleSourceData>, StyleSheetContents* contextStyleSheet);
    static PassRef<ImmutableStyleProperties> parseInlineStyleDeclaration(const String&, Element*);
    std::unique_ptr<MediaQuery> parseMediaQuery(const String&);
#if ENABLE(PICTURE_SIZES)
    std::unique_ptr<SourceSizeList> parseSizesAttribute(const String&);
#endif

    void addPropertyWithPrefixingVariant(CSSPropertyID, PassRefPtr<CSSValue>, bool important, bool implicit = false);
    void addProperty(CSSPropertyID, PassRefPtr<CSSValue>, bool important, bool implicit = false);
    void rollbackLastProperties(int num);
    bool hasProperties() const { return !m_parsedProperties.isEmpty(); }
    void addExpandedPropertyForValue(CSSPropertyID propId, PassRefPtr<CSSValue>, bool);

    bool parseValue(CSSPropertyID, bool important);
    bool parseShorthand(CSSPropertyID, const StylePropertyShorthand&, bool important);
    bool parse4Values(CSSPropertyID, const CSSPropertyID* properties, bool important);
    bool parseContent(CSSPropertyID, bool important);
    bool parseQuotes(CSSPropertyID, bool important);
    bool parseAlt(CSSPropertyID, bool important);
    
    PassRefPtr<CSSValue> parseAttr(CSSParserValueList* args);

    PassRefPtr<CSSValue> parseBackgroundColor();

    bool parseFillImage(CSSParserValueList*, RefPtr<CSSValue>&);

    enum FillPositionFlag { InvalidFillPosition = 0, AmbiguousFillPosition = 1, XFillPosition = 2, YFillPosition = 4 };
    enum FillPositionParsingMode { ResolveValuesAsPercent = 0, ResolveValuesAsKeyword = 1 };
    PassRefPtr<CSSPrimitiveValue> parseFillPositionComponent(CSSParserValueList*, unsigned& cumulativeFlags, FillPositionFlag& individualFlag, FillPositionParsingMode = ResolveValuesAsPercent);
    PassRefPtr<CSSValue> parseFillPositionX(CSSParserValueList*);
    PassRefPtr<CSSValue> parseFillPositionY(CSSParserValueList*);
    void parse2ValuesFillPosition(CSSParserValueList*, RefPtr<CSSValue>&, RefPtr<CSSValue>&);
    bool isPotentialPositionValue(CSSParserValue*);
    void parseFillPosition(CSSParserValueList*, RefPtr<CSSValue>&, RefPtr<CSSValue>&);
    void parse3ValuesFillPosition(CSSParserValueList*, RefPtr<CSSValue>&, RefPtr<CSSValue>&, PassRefPtr<CSSPrimitiveValue>, PassRefPtr<CSSPrimitiveValue>);
    void parse4ValuesFillPosition(CSSParserValueList*, RefPtr<CSSValue>&, RefPtr<CSSValue>&, PassRefPtr<CSSPrimitiveValue>, PassRefPtr<CSSPrimitiveValue>);

    void parseFillRepeat(RefPtr<CSSValue>&, RefPtr<CSSValue>&);
    PassRefPtr<CSSValue> parseFillSize(CSSPropertyID, bool &allowComma);

    bool parseFillProperty(CSSPropertyID propId, CSSPropertyID& propId1, CSSPropertyID& propId2, RefPtr<CSSValue>&, RefPtr<CSSValue>&);
    bool parseFillShorthand(CSSPropertyID, const CSSPropertyID* properties, int numProperties, bool important);

    void addFillValue(RefPtr<CSSValue>& lval, PassRefPtr<CSSValue> rval);

    void addAnimationValue(RefPtr<CSSValue>& lval, PassRefPtr<CSSValue> rval);

    PassRefPtr<CSSValue> parseAnimationDelay();
    PassRefPtr<CSSValue> parseAnimationDirection();
    PassRefPtr<CSSValue> parseAnimationDuration();
    PassRefPtr<CSSValue> parseAnimationFillMode();
    PassRefPtr<CSSValue> parseAnimationIterationCount();
    PassRefPtr<CSSValue> parseAnimationName();
    PassRefPtr<CSSValue> parseAnimationPlayState();
    PassRefPtr<CSSValue> parseAnimationProperty(AnimationParseContext&);
    PassRefPtr<CSSValue> parseAnimationTimingFunction();

    bool parseTransformOriginShorthand(RefPtr<CSSValue>&, RefPtr<CSSValue>&, RefPtr<CSSValue>&);
    bool parseCubicBezierTimingFunctionValue(CSSParserValueList*& args, double& result);
    bool parseAnimationProperty(CSSPropertyID, RefPtr<CSSValue>&, AnimationParseContext&);
    bool parseTransitionShorthand(CSSPropertyID, bool important);
    bool parseAnimationShorthand(bool important);

#if ENABLE(CSS_GRID_LAYOUT)
    PassRefPtr<CSSValue> parseGridPosition();
    bool parseGridItemPositionShorthand(CSSPropertyID, bool important);
    bool parseGridTemplateRowsAndAreas(PassRefPtr<CSSValue>, bool important);
    bool parseGridTemplateShorthand(bool important);
    bool parseGridShorthand(bool important);
    bool parseGridAreaShorthand(bool important);
    bool parseSingleGridAreaLonghand(RefPtr<CSSValue>&);
    PassRefPtr<CSSValue> parseGridTrackList();
    bool parseGridTrackRepeatFunction(CSSValueList&);
    PassRefPtr<CSSValue> parseGridTrackSize(CSSParserValueList& inputList);
    PassRefPtr<CSSPrimitiveValue> parseGridBreadth(CSSParserValue*);
    bool parseGridTemplateAreasRow(NamedGridAreaMap&, const unsigned, unsigned&);
    PassRefPtr<CSSValue> parseGridTemplateAreas();
    void parseGridLineNames(CSSParserValueList&, CSSValueList&, CSSGridLineNamesValue* = nullptr);
    PassRefPtr<CSSValue> parseGridAutoFlow(CSSParserValueList&);
#endif

    bool parseDashboardRegions(CSSPropertyID, bool important);

    bool parseClipShape(CSSPropertyID, bool important);

    bool parseJustifySelf(CSSPropertyID, bool important);

#if ENABLE(CSS_SHAPES)
    PassRefPtr<CSSValue> parseShapeProperty(CSSPropertyID);
#endif

    PassRefPtr<CSSValue> parseBasicShapeAndOrBox(CSSPropertyID propId);
    PassRefPtr<CSSPrimitiveValue> parseBasicShape();
    PassRefPtr<CSSPrimitiveValue> parseShapeRadius(CSSParserValue*);
    PassRefPtr<CSSBasicShape> parseBasicShapeRectangle(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseBasicShapeCircle(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseDeprecatedBasicShapeCircle(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseBasicShapeEllipse(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseDeprecatedBasicShapeEllipse(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseBasicShapePolygon(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseBasicShapeInsetRectangle(CSSParserValueList*);
    PassRefPtr<CSSBasicShape> parseBasicShapeInset(CSSParserValueList*);

    bool parseFont(bool important);
    PassRefPtr<CSSValueList> parseFontFamily();

    bool parseCounter(CSSPropertyID, int defaultValue, bool important);
    PassRefPtr<CSSValue> parseCounterContent(CSSParserValueList* args, bool counters);

    bool parseColorParameters(CSSParserValue*, int* colorValues, bool parseAlpha);
    bool parseHSLParameters(CSSParserValue*, double* colorValues, bool parseAlpha);
    PassRefPtr<CSSPrimitiveValue> parseColor(CSSParserValue* = 0);
    bool parseColorFromValue(CSSParserValue*, RGBA32&);
    void parseSelector(const String&, CSSSelectorList&);

    template<typename StringType>
    static bool fastParseColor(RGBA32&, const StringType&, bool strict);

    bool parseLineHeight(bool important);
    bool parseFontSize(bool important);
    bool parseFontVariant(bool important);
    bool parseFontWeight(bool important);
    bool parseFontFaceSrc();
    bool parseFontFaceUnicodeRange();

    bool parseSVGValue(CSSPropertyID propId, bool important);
    PassRefPtr<CSSValue> parseSVGPaint();
    PassRefPtr<CSSValue> parseSVGColor();
    PassRefPtr<CSSValue> parseSVGStrokeDasharray();
    PassRefPtr<CSSValue> parsePaintOrder();

    // CSS3 Parsing Routines (for properties specific to CSS3)
    PassRefPtr<CSSValueList> parseShadow(CSSParserValueList*, CSSPropertyID);
    bool parseBorderImage(CSSPropertyID, RefPtr<CSSValue>&, bool important = false);
    bool parseBorderImageRepeat(RefPtr<CSSValue>&);
    bool parseBorderImageSlice(CSSPropertyID, RefPtr<CSSBorderImageSliceValue>&);
    bool parseBorderImageWidth(RefPtr<CSSPrimitiveValue>&);
    bool parseBorderImageOutset(RefPtr<CSSPrimitiveValue>&);
    bool parseBorderRadius(CSSPropertyID, bool important);

    bool parseAspectRatio(bool important);

    bool parseReflect(CSSPropertyID, bool important);

    bool parseFlex(CSSParserValueList* args, bool important);

    // Image generators
    bool parseCanvas(CSSParserValueList*, RefPtr<CSSValue>&);

    bool parseDeprecatedGradient(CSSParserValueList*, RefPtr<CSSValue>&);
    bool parseDeprecatedLinearGradient(CSSParserValueList*, RefPtr<CSSValue>&, CSSGradientRepeat repeating);
    bool parseDeprecatedRadialGradient(CSSParserValueList*, RefPtr<CSSValue>&, CSSGradientRepeat repeating);
    bool parseLinearGradient(CSSParserValueList*, RefPtr<CSSValue>&, CSSGradientRepeat repeating);
    bool parseRadialGradient(CSSParserValueList*, RefPtr<CSSValue>&, CSSGradientRepeat repeating);
    bool parseGradientColorStops(CSSParserValueList*, CSSGradientValue*, bool expectComma);

    bool parseCrossfade(CSSParserValueList*, RefPtr<CSSValue>&);

#if ENABLE(CSS_IMAGE_RESOLUTION)
    PassRefPtr<CSSValue> parseImageResolution();
#endif

#if ENABLE(CSS_IMAGE_SET)
    PassRefPtr<CSSValue> parseImageSet();
#endif

#if ENABLE(CSS_FILTERS)
    bool parseFilterImage(CSSParserValueList*, RefPtr<CSSValue>&);

    bool parseFilter(CSSParserValueList*, RefPtr<CSSValue>&);
    PassRefPtr<WebKitCSSFilterValue> parseBuiltinFilterArguments(CSSParserValueList*, WebKitCSSFilterValue::FilterOperationType);
#endif

    PassRefPtr<CSSValue> parseClipPath();

    static bool isBlendMode(CSSValueID);
    static bool isCompositeOperator(CSSValueID);

    PassRefPtr<CSSValueList> parseTransform();
    PassRefPtr<CSSValue> parseTransformValue(CSSParserValue*);
    bool parseTransformOrigin(CSSPropertyID propId, CSSPropertyID& propId1, CSSPropertyID& propId2, CSSPropertyID& propId3, RefPtr<CSSValue>&, RefPtr<CSSValue>&, RefPtr<CSSValue>&);
    bool parsePerspectiveOrigin(CSSPropertyID propId, CSSPropertyID& propId1, CSSPropertyID& propId2,  RefPtr<CSSValue>&, RefPtr<CSSValue>&);

    bool parseTextEmphasisStyle(bool important);
    bool parseTextEmphasisPosition(bool important);

    void addTextDecorationProperty(CSSPropertyID, PassRefPtr<CSSValue>, bool important);
    bool parseTextDecoration(CSSPropertyID propId, bool important);
    bool parseTextDecorationSkip(bool important);
    bool parseTextUnderlinePosition(bool important);

    PassRefPtr<CSSValue> parseTextIndent();
    
    bool parseLineBoxContain(bool important);
    bool parseCalculation(CSSParserValue*, CalculationPermittedValueRange);

    bool parseFontFeatureTag(CSSValueList*);
    bool parseFontFeatureSettings(bool important);

    bool cssRegionsEnabled() const;
    bool cssCompositingEnabled() const;
    bool parseFlowThread(CSSPropertyID, bool important);
    bool parseRegionThread(CSSPropertyID, bool important);

    bool parseFontVariantLigatures(bool important);

    // Faster than doing a new/delete each time since it keeps one vector.
    std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> createSelectorVector();
    void recycleSelectorVector(std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>>);

    PassRefPtr<StyleRuleBase> createImportRule(const CSSParserString&, PassRefPtr<MediaQuerySet>);
    PassRefPtr<StyleKeyframe> createKeyframe(CSSParserValueList&);
    PassRefPtr<StyleRuleKeyframes> createKeyframesRule(const String&, std::unique_ptr<Vector<RefPtr<StyleKeyframe>>>);

    typedef Vector<RefPtr<StyleRuleBase>> RuleList;
    PassRefPtr<StyleRuleBase> createMediaRule(PassRefPtr<MediaQuerySet>, RuleList*);
    PassRefPtr<StyleRuleBase> createEmptyMediaRule(RuleList*);
    PassRefPtr<StyleRuleBase> createStyleRule(Vector<std::unique_ptr<CSSParserSelector>>* selectors);
    PassRefPtr<StyleRuleBase> createFontFaceRule();
    PassRefPtr<StyleRuleBase> createPageRule(std::unique_ptr<CSSParserSelector> pageSelector);
    PassRefPtr<StyleRuleBase> createRegionRule(Vector<std::unique_ptr<CSSParserSelector>>* regionSelector, RuleList* rules);
    void createMarginAtRule(CSSSelector::MarginBoxType);
#if ENABLE(CSS3_CONDITIONAL_RULES)
    PassRefPtr<StyleRuleBase> createSupportsRule(bool conditionIsSupported, RuleList*);
    void markSupportsRuleHeaderStart();
    void markSupportsRuleHeaderEnd();
    PassRefPtr<CSSRuleSourceData> popSupportsRuleData();
#endif

    void startDeclarationsForMarginBox();
    void endDeclarationsForMarginBox();

    void addNamespace(const AtomicString& prefix, const AtomicString& uri);
    QualifiedName determineNameInNamespace(const AtomicString& prefix, const AtomicString& localName);

    void rewriteSpecifiersWithElementName(const AtomicString& namespacePrefix, const AtomicString& elementName, CSSParserSelector&, bool isNamespacePlaceholder = false);
    void rewriteSpecifiersWithNamespaceIfNeeded(CSSParserSelector&);
    std::unique_ptr<CSSParserSelector> rewriteSpecifiers(std::unique_ptr<CSSParserSelector>, std::unique_ptr<CSSParserSelector>);

    void invalidBlockHit();

    void updateLastSelectorLineAndPosition();
    void updateLastMediaLine(MediaQuerySet*);

    void clearProperties();

    PassRef<ImmutableStyleProperties> createStyleProperties();

    CSSParserContext m_context;

    bool m_important;
    CSSPropertyID m_id;
    StyleSheetContents* m_styleSheet;
    RefPtr<StyleRuleBase> m_rule;
    RefPtr<StyleKeyframe> m_keyframe;
    std::unique_ptr<MediaQuery> m_mediaQuery;
#if ENABLE(PICTURE_SIZES)
    std::unique_ptr<SourceSizeList> m_sourceSizeList;
#endif
    std::unique_ptr<CSSParserValueList> m_valueList;
#if ENABLE(CSS3_CONDITIONAL_RULES)
    bool m_supportsCondition;
#endif

    typedef Vector<CSSProperty, 256> ParsedPropertyVector;
    ParsedPropertyVector m_parsedProperties;
    CSSSelectorList* m_selectorListForParseSelector;

    unsigned m_numParsedPropertiesBeforeMarginBox;

    int m_inParseShorthand;
    CSSPropertyID m_currentShorthand;
    bool m_implicitShorthand;

    bool m_hasFontFaceOnlyValues;
    bool m_hadSyntacticallyValidCSSRule;
    bool m_logErrors;
    bool m_ignoreErrorsInDeclaration;

    AtomicString m_defaultNamespace;

    // tokenizer methods and data
    size_t m_parsedTextPrefixLength;
    SourceRange m_selectorRange;
    SourceRange m_propertyRange;
    std::unique_ptr<RuleSourceDataList> m_currentRuleDataStack;
    RefPtr<CSSRuleSourceData> m_currentRuleData;
    RuleSourceDataList* m_ruleSourceDataResult;

    void fixUnparsedPropertyRanges(CSSRuleSourceData*);
    void markRuleHeaderStart(CSSRuleSourceData::Type);
    void markRuleHeaderEnd();
    void markSelectorStart();
    void markSelectorEnd();
    void markRuleBodyStart();
    void markRuleBodyEnd();
    void markPropertyStart();
    void markPropertyEnd(bool isImportantFound, bool isPropertyParsed);
    void processAndAddNewRuleToSourceTreeIfNeeded();
    void addNewRuleToSourceTree(PassRefPtr<CSSRuleSourceData>);
    PassRefPtr<CSSRuleSourceData> popRuleData();
    void resetPropertyRange() { m_propertyRange.start = m_propertyRange.end = UINT_MAX; }
    bool isExtractingSourceData() const { return !!m_currentRuleDataStack; }
    void syntaxError(const Location&, SyntaxErrorType = GeneralSyntaxError);

    inline int lex(void* yylval) { return (this->*m_lexFunc)(yylval); }

    int token() { return m_token; }

#if ENABLE(CSS_DEVICE_ADAPTATION)
    void markViewportRuleBodyStart() { m_inViewport = true; }
    void markViewportRuleBodyEnd() { m_inViewport = false; }
    PassRefPtr<StyleRuleBase> createViewportRule();
#endif

    PassRefPtr<CSSPrimitiveValue> createPrimitiveNumericValue(CSSParserValue*);
    PassRefPtr<CSSPrimitiveValue> createPrimitiveStringValue(CSSParserValue*);

    static URL completeURL(const CSSParserContext&, const String& url);

    Location currentLocation();
    static bool isCalculation(CSSParserValue*);

private:
    bool is8BitSource() { return m_is8BitSource; }

    template <typename SourceCharacterType>
    int realLex(void* yylval);

    UChar*& currentCharacter16();

    template <typename CharacterType>
    inline CharacterType*& currentCharacter();

    template <typename CharacterType>
    inline CharacterType* tokenStart();

    template <typename CharacterType>
    inline void setTokenStart(CharacterType*);

    inline unsigned tokenStartOffset();
    inline UChar tokenStartChar();

    template <typename CharacterType>
    inline bool isIdentifierStart();

    template <typename CharacterType>
    unsigned parseEscape(CharacterType*&);
    template <typename DestCharacterType>
    inline void UnicodeToChars(DestCharacterType*&, unsigned);
    template <typename SrcCharacterType, typename DestCharacterType>
    inline bool parseIdentifierInternal(SrcCharacterType*&, DestCharacterType*&, bool&);

    template <typename CharacterType>
    inline void parseIdentifier(CharacterType*&, CSSParserString&, bool&);

    template <typename SrcCharacterType, typename DestCharacterType>
    inline bool parseStringInternal(SrcCharacterType*&, DestCharacterType*&, UChar);

    template <typename CharacterType>
    inline void parseString(CharacterType*&, CSSParserString& resultString, UChar);

    template <typename CharacterType>
    inline bool findURI(CharacterType*& start, CharacterType*& end, UChar& quote);

    template <typename SrcCharacterType, typename DestCharacterType>
    inline bool parseURIInternal(SrcCharacterType*&, DestCharacterType*&, UChar quote);

    template <typename CharacterType>
    inline void parseURI(CSSParserString&);
    template <typename CharacterType>
    inline bool parseUnicodeRange();
    template <typename CharacterType>
    bool parseNthChild();
    template <typename CharacterType>
    bool parseNthChildExtra();
    template <typename CharacterType>
    inline bool detectFunctionTypeToken(int);
    template <typename CharacterType>
    inline void detectMediaQueryToken(int);
    template <typename CharacterType>
    inline void detectNumberToken(CharacterType*, int);
    template <typename CharacterType>
    inline void detectDashToken(int);
    template <typename CharacterType>
    inline void detectAtToken(int, bool);
#if ENABLE(CSS3_CONDITIONAL_RULES)
    template <typename CharacterType>
    inline void detectSupportsToken(int);
#endif

    template <typename CharacterType>
    inline void setRuleHeaderEnd(const CharacterType*);

    void setStyleSheet(StyleSheetContents* styleSheet) { m_styleSheet = styleSheet; }

    inline bool inStrictMode() const { return m_context.mode == CSSStrictMode || m_context.mode == SVGAttributeMode; }
    inline bool inQuirksMode() const { return m_context.mode == CSSQuirksMode; }
    
    URL completeURL(const String& url) const;

    void recheckAtKeyword(const UChar* str, int len);

    template<unsigned prefixLength, unsigned suffixLength>
    inline void setupParser(const char (&prefix)[prefixLength], const String& string, const char (&suffix)[suffixLength])
    {
        setupParser(prefix, prefixLength - 1, string, suffix, suffixLength - 1);
    }
    void setupParser(const char* prefix, unsigned prefixLength, const String&, const char* suffix, unsigned suffixLength);
    bool inShorthand() const { return m_inParseShorthand; }

    bool validWidth(CSSParserValue*);
    bool validHeight(CSSParserValue*);

    void deleteFontFaceOnlyValues();

    bool isGeneratedImageValue(CSSParserValue*) const;
    bool parseGeneratedImage(CSSParserValueList*, RefPtr<CSSValue>&);

    bool parseValue(MutableStyleProperties*, CSSPropertyID, const String&, bool important, StyleSheetContents* contextStyleSheet);
    PassRef<ImmutableStyleProperties> parseDeclaration(const String&, StyleSheetContents* contextStyleSheet);

    PassRefPtr<CSSBasicShape> parseInsetRoundedCorners(PassRefPtr<CSSBasicShapeInset>, CSSParserValueList*);

    enum SizeParameterType {
        None,
        Auto,
        Length,
        PageSize,
        Orientation,
    };

    bool parsePage(CSSPropertyID propId, bool important);
    bool parseSize(CSSPropertyID propId, bool important);
    SizeParameterType parseSizeParameter(CSSValueList* parsedValues, CSSParserValue* value, SizeParameterType prevParamType);

    bool parseFontFaceSrcURI(CSSValueList*);
    bool parseFontFaceSrcLocal(CSSValueList*);

    bool parseColor(const String&);

#if ENABLE(CSS_GRID_LAYOUT)
    bool parseIntegerOrCustomIdentFromGridPosition(RefPtr<CSSPrimitiveValue>& numericValue, RefPtr<CSSPrimitiveValue>& gridLineName);
#endif

    enum ParsingMode {
        NormalMode,
        MediaQueryMode,
#if ENABLE(CSS3_CONDITIONAL_RULES)
        SupportsMode,
#endif
        NthChildMode
    };

    ParsingMode m_parsingMode;
    bool m_is8BitSource;
    std::unique_ptr<LChar[]> m_dataStart8;
    std::unique_ptr<UChar[]> m_dataStart16;
    LChar* m_currentCharacter8;
    UChar* m_currentCharacter16;
    union {
        LChar* ptr8;
        UChar* ptr16;
    } m_tokenStart;
    unsigned m_length;
    int m_token;
    int m_lineNumber;
    int m_tokenStartLineNumber;
    int m_lastSelectorLineNumber;

    bool m_allowImportRules;
    bool m_allowNamespaceDeclarations;

#if ENABLE(CSS_DEVICE_ADAPTATION)
    bool parseViewportProperty(CSSPropertyID propId, bool important);
    bool parseViewportShorthand(CSSPropertyID propId, CSSPropertyID first, CSSPropertyID second, bool important);

    bool inViewport() const { return m_inViewport; }
    bool m_inViewport;
#endif

    bool useLegacyBackgroundSizeShorthandBehavior() const;

    int (CSSParser::*m_lexFunc)(void*);

    std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> m_recycledSelectorVector;

    RefPtr<CSSCalcValue> m_parsedCalculation;

#if ENABLE(CSS3_CONDITIONAL_RULES)
    std::unique_ptr<RuleSourceDataList> m_supportsRuleDataStack;
#endif

    // defines units allowed for a certain property, used in parseUnit
    enum Units {
        FUnknown = 0x0000,
        FInteger = 0x0001,
        FNumber = 0x0002, // Real Numbers
        FPercent = 0x0004,
        FLength = 0x0008,
        FAngle = 0x0010,
        FTime = 0x0020,
        FFrequency = 0x0040,
        FPositiveInteger = 0x0080,
        FRelative = 0x0100,
#if ENABLE(CSS_IMAGE_RESOLUTION) || ENABLE(RESOLUTION_MEDIA_QUERY)
        FResolution = 0x0200,
#endif
        FNonNeg = 0x0400
    };

    friend inline Units operator|(Units a, Units b)
    {
        return static_cast<Units>(static_cast<unsigned>(a) | static_cast<unsigned>(b));
    }

    enum ReleaseParsedCalcValueCondition {
        ReleaseParsedCalcValue,
        DoNotReleaseParsedCalcValue
    };

    bool isLoggingErrors();
    void logError(const String& message, int lineNumber);

    bool validCalculationUnit(CSSParserValue*, Units, ReleaseParsedCalcValueCondition releaseCalc = DoNotReleaseParsedCalcValue);

    bool shouldAcceptUnitLessValues(CSSParserValue*, Units, CSSParserMode);

    inline bool validUnit(CSSParserValue* value, Units unitflags, ReleaseParsedCalcValueCondition releaseCalc = DoNotReleaseParsedCalcValue) { return validUnit(value, unitflags, m_context.mode, releaseCalc); }
    bool validUnit(CSSParserValue*, Units, CSSParserMode, ReleaseParsedCalcValueCondition releaseCalc = DoNotReleaseParsedCalcValue);

    bool parseBorderImageQuad(Units, RefPtr<CSSPrimitiveValue>&);
    int colorIntFromValue(CSSParserValue*);
    double parsedDouble(CSSParserValue*, ReleaseParsedCalcValueCondition releaseCalc = DoNotReleaseParsedCalcValue);
    
    friend class TransformOperationInfo;
#if ENABLE(CSS_FILTERS)
    friend class FilterOperationInfo;
#endif
};

CSSPropertyID cssPropertyID(const CSSParserString&);
CSSPropertyID cssPropertyID(const String&);
CSSValueID cssValueKeywordID(const CSSParserString&);
#if PLATFORM(IOS)
void cssPropertyNameIOSAliasing(const char* propertyName, const char*& propertyNameAlias, unsigned& newLength);
#endif

class ShorthandScope {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ShorthandScope(CSSParser* parser, CSSPropertyID propId) : m_parser(parser)
    {
        if (!(m_parser->m_inParseShorthand++))
            m_parser->m_currentShorthand = propId;
    }
    ~ShorthandScope()
    {
        if (!(--m_parser->m_inParseShorthand))
            m_parser->m_currentShorthand = CSSPropertyInvalid;
    }

private:
    CSSParser* m_parser;
};

struct CSSParser::Location {
    int lineNumber;
    CSSParserString token;
};

String quoteCSSString(const String&);
String quoteCSSStringIfNeeded(const String&);
String quoteCSSURLIfNeeded(const String&);

bool isValidNthToken(const CSSParserString&);

template <>
inline void CSSParser::setTokenStart<LChar>(LChar* tokenStart)
{
    m_tokenStart.ptr8 = tokenStart;
}

template <>
inline void CSSParser::setTokenStart<UChar>(UChar* tokenStart)
{
    m_tokenStart.ptr16 = tokenStart;
}

inline unsigned CSSParser::tokenStartOffset()
{
    if (is8BitSource())
        return m_tokenStart.ptr8 - m_dataStart8.get();
    return m_tokenStart.ptr16 - m_dataStart16.get();
}

inline UChar CSSParser::tokenStartChar()
{
    if (is8BitSource())
        return *m_tokenStart.ptr8;
    return *m_tokenStart.ptr16;
}

inline int cssyylex(void* yylval, CSSParser* parser)
{
    return parser->lex(yylval);
}

} // namespace WebCore

#endif // CSSParser_h
