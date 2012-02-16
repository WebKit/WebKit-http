/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorCSSAgent.h"

#if ENABLE(INSPECTOR)

#include "CSSComputedStyleDeclaration.h"
#include "CSSImportRule.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "DOMWindow.h"
#include "HTMLHeadElement.h"
#include "InspectorDOMAgent.h"
#include "InspectorHistory.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "Node.h"
#include "NodeList.h"
#include "StylePropertySet.h"
#include "StyleSheetList.h"

#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

namespace CSSAgentState {
static const char cssAgentEnabled[] = "cssAgentEnabled";
static const char isSelectorProfiling[] = "isSelectorProfiling";
}

namespace WebCore {

enum ForcePseudoClassFlags {
    PseudoNone = 0,
    PseudoHover = 1 << 0,
    PseudoFocus = 1 << 1,
    PseudoActive = 1 << 2,
    PseudoVisited = 1 << 3
};

struct RuleMatchData {
    String selector;
    String url;
    unsigned lineNumber;
    double startTime;
};

struct RuleMatchingStats {
    RuleMatchingStats()
        : lineNumber(0), totalTime(0.0), hits(0), matches(0)
    {
    }
    RuleMatchingStats(const RuleMatchData& data, double totalTime, unsigned hits, unsigned matches)
        : selector(data.selector), url(data.url), lineNumber(data.lineNumber), totalTime(totalTime), hits(hits), matches(matches)
    {
    }

    String selector;
    String url;
    unsigned lineNumber;
    double totalTime;
    unsigned hits;
    unsigned matches;
};

class SelectorProfile {
public:
    SelectorProfile()
        : m_totalMatchingTimeMs(0.0)
    {
    }
    virtual ~SelectorProfile()
    {
    }

    double totalMatchingTimeMs() const { return m_totalMatchingTimeMs; }

    String makeKey();
    void startSelector(const CSSStyleRule*);
    void commitSelector(bool);
    void commitSelectorTime();
    PassRefPtr<InspectorObject> toInspectorObject() const;

private:

    // Key is "selector?url:line".
    typedef HashMap<String, RuleMatchingStats> RuleMatchingStatsMap;

    double m_totalMatchingTimeMs;
    RuleMatchingStatsMap m_ruleMatchingStats;
    RuleMatchData m_currentMatchData;
};


static unsigned computePseudoClassMask(InspectorArray* pseudoClassArray)
{
    DEFINE_STATIC_LOCAL(String, active, ("active"));
    DEFINE_STATIC_LOCAL(String, hover, ("hover"));
    DEFINE_STATIC_LOCAL(String, focus, ("focus"));
    DEFINE_STATIC_LOCAL(String, visited, ("visited"));
    if (!pseudoClassArray || !pseudoClassArray->length())
        return PseudoNone;

    unsigned result = PseudoNone;
    for (size_t i = 0; i < pseudoClassArray->length(); ++i) {
        RefPtr<InspectorValue> pseudoClassValue = pseudoClassArray->get(i);
        String pseudoClass;
        bool success = pseudoClassValue->asString(&pseudoClass);
        if (!success)
            continue;
        if (pseudoClass == active)
            result |= PseudoActive;
        else if (pseudoClass == hover)
            result |= PseudoHover;
        else if (pseudoClass == focus)
            result |= PseudoFocus;
        else if (pseudoClass == visited)
            result |= PseudoVisited;
    }

    return result;
}

inline String SelectorProfile::makeKey()
{
    return makeString(m_currentMatchData.selector, "?", m_currentMatchData.url, ":", String::number(m_currentMatchData.lineNumber));
}

inline void SelectorProfile::startSelector(const CSSStyleRule* rule)
{
    m_currentMatchData.selector = rule->selectorText();
    CSSStyleSheet* styleSheet = rule->parentStyleSheet();
    String url = emptyString();
    if (styleSheet) {
        url = InspectorStyleSheet::styleSheetURL(styleSheet);
        if (url.isEmpty())
            url = InspectorDOMAgent::documentURLString(styleSheet->findDocument());
    }
    m_currentMatchData.url = url;
    m_currentMatchData.lineNumber = rule->sourceLine();
    m_currentMatchData.startTime = WTF::currentTimeMS();
}

inline void SelectorProfile::commitSelector(bool matched)
{
    double matchTimeMs = WTF::currentTimeMS() - m_currentMatchData.startTime;
    m_totalMatchingTimeMs += matchTimeMs;

    pair<RuleMatchingStatsMap::iterator, bool> result = m_ruleMatchingStats.add(makeKey(), RuleMatchingStats(m_currentMatchData, matchTimeMs, 1, matched ? 1 : 0));
    if (!result.second) {
        result.first->second.totalTime += matchTimeMs;
        result.first->second.hits += 1;
        if (matched)
            result.first->second.matches += 1;
    }
}

inline void SelectorProfile::commitSelectorTime()
{
    double processingTimeMs = WTF::currentTimeMS() - m_currentMatchData.startTime;
    m_totalMatchingTimeMs += processingTimeMs;

    RuleMatchingStatsMap::iterator it = m_ruleMatchingStats.find(makeKey());
    if (it == m_ruleMatchingStats.end())
        return;

    it->second.totalTime += processingTimeMs;
}

PassRefPtr<InspectorObject> SelectorProfile::toInspectorObject() const
{
    RefPtr<InspectorArray> selectorProfileData = InspectorArray::create();
    for (RuleMatchingStatsMap::const_iterator it = m_ruleMatchingStats.begin(); it != m_ruleMatchingStats.end(); ++it) {
        RefPtr<TypeBuilder::CSS::SelectorProfileEntry> entry = TypeBuilder::CSS::SelectorProfileEntry::create()
            .setSelector(it->second.selector)
            .setUrl(it->second.url)
            .setLineNumber(it->second.lineNumber)
            .setTime(it->second.totalTime)
            .setHitCount(it->second.hits)
            .setMatchCount(it->second.matches);
        selectorProfileData->pushObject(entry.release());
    }

    RefPtr<TypeBuilder::CSS::SelectorProfile> result = TypeBuilder::CSS::SelectorProfile::create()
        .setTotalTime(totalMatchingTimeMs())
        .setData(selectorProfileData);
    return result.release();
}

class InspectorCSSAgent::StyleSheetAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(StyleSheetAction);
public:
    StyleSheetAction(const String& name, InspectorStyleSheet* styleSheet)
        : InspectorHistory::Action(name)
        , m_styleSheet(styleSheet)
    {
    }

protected:
    RefPtr<InspectorStyleSheet> m_styleSheet;
};

class InspectorCSSAgent::SetStyleSheetTextAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetStyleSheetTextAction);
public:
    SetStyleSheetTextAction(InspectorStyleSheet* styleSheet, const String& text)
        : InspectorCSSAgent::StyleSheetAction("SetStyleSheetText", styleSheet)
        , m_text(text)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        if (!m_styleSheet->getText(&m_oldText))
            return false;
        return redo(ec);
    }

    virtual bool undo(ExceptionCode&)
    {
        if (m_styleSheet->setText(m_oldText)) {
            m_styleSheet->reparseStyleSheet(m_oldText);
            return true;
        }
        return false;
    }

    virtual bool redo(ExceptionCode&)
    {
        if (m_styleSheet->setText(m_text)) {
            m_styleSheet->reparseStyleSheet(m_text);
            return true;
        }
        return false;
    }

    virtual String mergeId()
    {
        return String::format("SetStyleSheetText %s", m_styleSheet->id().utf8().data());
    }

    virtual void merge(PassOwnPtr<Action> action)
    {
        ASSERT(action->mergeId() == mergeId());

        SetStyleSheetTextAction* other = static_cast<SetStyleSheetTextAction*>(action.get());
        m_text = other->m_text;
    }

private:
    String m_text;
    String m_oldText;
};

class InspectorCSSAgent::SetPropertyTextAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetPropertyTextAction);
public:
    SetPropertyTextAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, unsigned propertyIndex, const String& text, bool overwrite)
        : InspectorCSSAgent::StyleSheetAction("SetPropertyText", styleSheet)
        , m_cssId(cssId)
        , m_propertyIndex(propertyIndex)
        , m_text(text)
        , m_overwrite(overwrite)
    {
    }

    virtual String toString()
    {
        return mergeId() + ": " + m_oldText + " -> " + m_text;
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        String placeholder;
        return m_styleSheet->setPropertyText(m_cssId, m_propertyIndex, m_overwrite ? m_oldText : "", true, &placeholder, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        String oldText;
        bool result = m_styleSheet->setPropertyText(m_cssId, m_propertyIndex, m_text, m_overwrite, &oldText, ec);
        m_oldText = oldText.stripWhiteSpace();
        // FIXME: remove this once the model handles this case.
        if (!m_oldText.endsWith(";"))
            m_oldText += ";";
        return result;
    }

    virtual String mergeId()
    {
        return String::format("SetPropertyText %s:%u:%s", m_styleSheet->id().utf8().data(), m_propertyIndex, m_overwrite ? "true" : "false");
    }

    virtual void merge(PassOwnPtr<Action> action)
    {
        ASSERT(action->mergeId() == mergeId());

        SetPropertyTextAction* other = static_cast<SetPropertyTextAction*>(action.get());
        m_text = other->m_text;
    }

private:
    InspectorCSSId m_cssId;
    unsigned m_propertyIndex;
    String m_text;
    String m_oldText;
    bool m_overwrite;
};

class InspectorCSSAgent::TogglePropertyAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(TogglePropertyAction);
public:
    TogglePropertyAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, unsigned propertyIndex, bool disable)
        : InspectorCSSAgent::StyleSheetAction("ToggleProperty", styleSheet)
        , m_cssId(cssId)
        , m_propertyIndex(propertyIndex)
        , m_disable(disable)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->toggleProperty(m_cssId, m_propertyIndex, !m_disable, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        return m_styleSheet->toggleProperty(m_cssId, m_propertyIndex, m_disable, ec);
    }

private:
    InspectorCSSId m_cssId;
    unsigned m_propertyIndex;
    bool m_disable;
};

class InspectorCSSAgent::SetRuleSelectorAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(SetRuleSelectorAction);
public:
    SetRuleSelectorAction(InspectorStyleSheet* styleSheet, const InspectorCSSId& cssId, const String& selector)
        : InspectorCSSAgent::StyleSheetAction("SetRuleSelector", styleSheet)
        , m_cssId(cssId)
        , m_selector(selector)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        m_oldSelector = m_styleSheet->ruleSelector(m_cssId, ec);
        if (ec)
            return false;
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->setRuleSelector(m_cssId, m_oldSelector, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        return m_styleSheet->setRuleSelector(m_cssId, m_selector, ec);
    }

private:
    InspectorCSSId m_cssId;
    String m_selector;
    String m_oldSelector;
};

class InspectorCSSAgent::AddRuleAction : public InspectorCSSAgent::StyleSheetAction {
    WTF_MAKE_NONCOPYABLE(AddRuleAction);
public:
    AddRuleAction(InspectorStyleSheet* styleSheet, const String& selector)
        : InspectorCSSAgent::StyleSheetAction("AddRule", styleSheet)
        , m_selector(selector)
    {
    }

    virtual bool perform(ExceptionCode& ec)
    {
        return redo(ec);
    }

    virtual bool undo(ExceptionCode& ec)
    {
        return m_styleSheet->deleteRule(m_newId, ec);
    }

    virtual bool redo(ExceptionCode& ec)
    {
        CSSStyleRule* cssStyleRule = m_styleSheet->addRule(m_selector, ec);
        if (ec)
            return false;
        m_newId = m_styleSheet->ruleId(cssStyleRule);
        return true;
    }

    InspectorCSSId newRuleId() { return m_newId; }

private:
    InspectorCSSId m_newId;
    String m_selector;
    String m_oldSelector;
};

// static
CSSStyleRule* InspectorCSSAgent::asCSSStyleRule(CSSRule* rule)
{
    if (!rule->isStyleRule())
        return 0;
    return static_cast<CSSStyleRule*>(rule);
}

InspectorCSSAgent::InspectorCSSAgent(InstrumentingAgents* instrumentingAgents, InspectorState* state, InspectorDOMAgent* domAgent)
    : InspectorBaseAgent<InspectorCSSAgent>("CSS", instrumentingAgents, state)
    , m_frontend(0)
    , m_domAgent(domAgent)
    , m_lastPseudoState(0)
    , m_lastStyleSheetId(1)
    , m_lastRuleId(1)
    , m_lastStyleId(1)
{
    m_domAgent->setDOMListener(this);
    m_instrumentingAgents->setInspectorCSSAgent(this);
}

InspectorCSSAgent::~InspectorCSSAgent()
{
    ASSERT(!m_domAgent);
    m_instrumentingAgents->setInspectorCSSAgent(0);
    reset();
}

void InspectorCSSAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(!m_frontend);
    m_frontend = frontend->css();
}

void InspectorCSSAgent::clearFrontend()
{
    ASSERT(m_frontend);
    m_frontend = 0;
    clearPseudoState(true);
    String errorString;
    stopSelectorProfilerImpl(&errorString);
}

void InspectorCSSAgent::discardAgent()
{
    m_domAgent->setDOMListener(0);
    m_domAgent = 0;
}

void InspectorCSSAgent::restore()
{
    if (m_state->getBoolean(CSSAgentState::cssAgentEnabled)) {
        ErrorString error;
        enable(&error);
    }
    if (m_state->getBoolean(CSSAgentState::isSelectorProfiling)) {
        String errorString;
        startSelectorProfiler(&errorString);
    }
}

void InspectorCSSAgent::reset()
{
    m_idToInspectorStyleSheet.clear();
    m_cssStyleSheetToInspectorStyleSheet.clear();
    m_nodeToInspectorStyleSheet.clear();
    m_documentToInspectorStyleSheet.clear();
}

void InspectorCSSAgent::enable(ErrorString*)
{
    m_state->setBoolean(CSSAgentState::cssAgentEnabled, true);
}

void InspectorCSSAgent::disable(ErrorString*)
{
    m_state->setBoolean(CSSAgentState::cssAgentEnabled, false);
}

void InspectorCSSAgent::mediaQueryResultChanged()
{
    if (m_frontend)
        m_frontend->mediaQueryResultChanged();
}

bool InspectorCSSAgent::forcePseudoState(Element* element, CSSSelector::PseudoType pseudoType)
{
    if (m_lastElementWithPseudoState != element)
        return false;

    switch (pseudoType) {
    case CSSSelector::PseudoActive:
        return m_lastPseudoState & PseudoActive;
    case CSSSelector::PseudoFocus:
        return m_lastPseudoState & PseudoFocus;
    case CSSSelector::PseudoHover:
        return m_lastPseudoState & PseudoHover;
    case CSSSelector::PseudoVisited:
        return m_lastPseudoState & PseudoVisited;
    default:
        return false;
    }
}

void InspectorCSSAgent::recalcStyleForPseudoStateIfNeeded(Element* element, InspectorArray* forcedPseudoClasses)
{
    unsigned forcePseudoState = computePseudoClassMask(forcedPseudoClasses);
    bool needStyleRecalc = element != m_lastElementWithPseudoState || forcePseudoState != m_lastPseudoState;
    m_lastPseudoState = forcePseudoState;
    m_lastElementWithPseudoState = element;
    if (needStyleRecalc)
        element->ownerDocument()->styleSelectorChanged(RecalcStyleImmediately);
}

void InspectorCSSAgent::getMatchedStylesForNode(ErrorString* errorString, int nodeId, const RefPtr<InspectorArray>* forcedPseudoClasses, const bool* needPseudo, const bool* needInherited, RefPtr<InspectorArray>& matchedCSSRules, RefPtr<InspectorArray>& pseudoIdRules, RefPtr<InspectorArray>& inheritedEntries)
{
    Element* element = elementForId(errorString, nodeId);
    if (!element)
        return;

    recalcStyleForPseudoStateIfNeeded(element, forcedPseudoClasses ? forcedPseudoClasses->get() : 0);

    // Matched rules.
    CSSStyleSelector* selector = element->ownerDocument()->styleSelector();
    RefPtr<CSSRuleList> matchedRules = selector->styleRulesForElement(element, CSSStyleSelector::AllCSSRules);
    matchedCSSRules = buildArrayForRuleList(matchedRules.get());

    // Pseudo elements.
    if (!needPseudo || *needPseudo) {
        RefPtr<InspectorArray> pseudoElements = InspectorArray::create();
        for (PseudoId pseudoId = FIRST_PUBLIC_PSEUDOID; pseudoId < AFTER_LAST_INTERNAL_PSEUDOID; pseudoId = static_cast<PseudoId>(pseudoId + 1)) {
            RefPtr<CSSRuleList> matchedRules = selector->pseudoStyleRulesForElement(element, pseudoId, CSSStyleSelector::AllCSSRules);
            if (matchedRules && matchedRules->length()) {
                RefPtr<InspectorObject> pseudoStyles = InspectorObject::create();
                pseudoStyles->setNumber("pseudoId", static_cast<int>(pseudoId));
                pseudoStyles->setArray("rules", buildArrayForRuleList(matchedRules.get()));
                pseudoElements->pushObject(pseudoStyles.release());
            }
        }

        pseudoIdRules = pseudoElements.release();
    }

    // Inherited styles.
    if (!needInherited || *needInherited) {
        RefPtr<InspectorArray> inheritedStyles = InspectorArray::create();
        Element* parentElement = element->parentElement();
        while (parentElement) {
            RefPtr<InspectorObject> parentStyle = InspectorObject::create();
            if (parentElement->style() && parentElement->style()->length()) {
                InspectorStyleSheetForInlineStyle* styleSheet = asInspectorStyleSheet(parentElement);
                if (styleSheet)
                    parentStyle->setObject("inlineStyle", styleSheet->buildObjectForStyle(styleSheet->styleForId(InspectorCSSId(styleSheet->id(), 0))));
            }

            CSSStyleSelector* parentSelector = parentElement->ownerDocument()->styleSelector();
            RefPtr<CSSRuleList> parentMatchedRules = parentSelector->styleRulesForElement(parentElement, CSSStyleSelector::AllCSSRules);
            parentStyle->setArray("matchedCSSRules", buildArrayForRuleList(parentMatchedRules.get()));
            inheritedStyles->pushObject(parentStyle.release());
            parentElement = parentElement->parentElement();
        }

        inheritedEntries = inheritedStyles.release();
    }
}

void InspectorCSSAgent::getInlineStylesForNode(ErrorString* errorString, int nodeId, RefPtr<InspectorObject>& inlineStyle, RefPtr<InspectorObject>& attributesStyle)
{
    Element* element = elementForId(errorString, nodeId);
    if (!element)
        return;

    InspectorStyleSheetForInlineStyle* styleSheet = asInspectorStyleSheet(element);
    if (!styleSheet)
        return;

    inlineStyle = styleSheet->buildObjectForStyle(element->style());
    RefPtr<InspectorObject> attributes = buildObjectForAttributesStyle(element);
    attributesStyle = attributes ? attributes.release() : 0;
}

void InspectorCSSAgent::getComputedStyleForNode(ErrorString* errorString, int nodeId, const RefPtr<InspectorArray>* forcedPseudoClasses, RefPtr<InspectorArray>& style)
{
    Element* element = elementForId(errorString, nodeId);
    if (!element)
        return;

    recalcStyleForPseudoStateIfNeeded(element, forcedPseudoClasses ? forcedPseudoClasses->get() : 0);

    RefPtr<CSSComputedStyleDeclaration> computedStyleInfo = CSSComputedStyleDeclaration::create(element, true);
    RefPtr<InspectorStyle> inspectorStyle = InspectorStyle::create(InspectorCSSId(), computedStyleInfo, 0);
    style = inspectorStyle->buildArrayForComputedStyle();
}

void InspectorCSSAgent::getAllStyleSheets(ErrorString*, RefPtr<InspectorArray>& styleInfos)
{
    Vector<Document*> documents = m_domAgent->documents();
    for (Vector<Document*>::iterator it = documents.begin(); it != documents.end(); ++it) {
        StyleSheetList* list = (*it)->styleSheets();
        for (unsigned i = 0; i < list->length(); ++i) {
            StyleSheet* styleSheet = list->item(i);
            if (styleSheet->isCSSStyleSheet())
                collectStyleSheets(static_cast<CSSStyleSheet*>(styleSheet), styleInfos.get());
        }
    }
}

void InspectorCSSAgent::getStyleSheet(ErrorString* errorString, const String& styleSheetId, RefPtr<InspectorObject>& styleSheetObject)
{
    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, styleSheetId);
    if (!inspectorStyleSheet)
        return;

    styleSheetObject = inspectorStyleSheet->buildObjectForStyleSheet();
}

void InspectorCSSAgent::getStyleSheetText(ErrorString* errorString, const String& styleSheetId, String* result)
{
    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, styleSheetId);
    if (!inspectorStyleSheet)
        return;

    inspectorStyleSheet->getText(result);
}

void InspectorCSSAgent::setStyleSheetText(ErrorString* errorString, const String& styleSheetId, const String& text)
{
    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, styleSheetId);
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    m_domAgent->history()->perform(adoptPtr(new SetStyleSheetTextAction(inspectorStyleSheet, text)), ec);
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::setPropertyText(ErrorString* errorString, const RefPtr<InspectorObject>& fullStyleId, int propertyIndex, const String& text, bool overwrite, RefPtr<InspectorObject>& result)
{
    InspectorCSSId compoundId(fullStyleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new SetPropertyTextAction(inspectorStyleSheet, compoundId, propertyIndex, text, overwrite)), ec);
    if (success)
        result = inspectorStyleSheet->buildObjectForStyle(inspectorStyleSheet->styleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::toggleProperty(ErrorString* errorString, const RefPtr<InspectorObject>& fullStyleId, int propertyIndex, bool disable, RefPtr<InspectorObject>& result)
{
    InspectorCSSId compoundId(fullStyleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new TogglePropertyAction(inspectorStyleSheet, compoundId, propertyIndex, disable)), ec);
    if (success)
        result = inspectorStyleSheet->buildObjectForStyle(inspectorStyleSheet->styleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);
}

void InspectorCSSAgent::setRuleSelector(ErrorString* errorString, const RefPtr<InspectorObject>& fullRuleId, const String& selector, RefPtr<InspectorObject>& result)
{
    InspectorCSSId compoundId(fullRuleId);
    ASSERT(!compoundId.isEmpty());

    InspectorStyleSheet* inspectorStyleSheet = assertStyleSheetForId(errorString, compoundId.styleSheetId());
    if (!inspectorStyleSheet)
        return;

    ExceptionCode ec = 0;
    bool success = m_domAgent->history()->perform(adoptPtr(new SetRuleSelectorAction(inspectorStyleSheet, compoundId, selector)), ec);
    if (success)
        result = inspectorStyleSheet->buildObjectForStyle(inspectorStyleSheet->styleForId(compoundId));
    *errorString = InspectorDOMAgent::toErrorString(ec);

    if (success)
        result = inspectorStyleSheet->buildObjectForRule(inspectorStyleSheet->ruleForId(compoundId));
}

void InspectorCSSAgent::addRule(ErrorString* errorString, const int contextNodeId, const String& selector, RefPtr<InspectorObject>& result)
{
    Node* node = m_domAgent->assertNode(errorString, contextNodeId);
    if (!node)
        return;

    InspectorStyleSheet* inspectorStyleSheet = viaInspectorStyleSheet(node->document(), true);
    if (!inspectorStyleSheet) {
        *errorString = "No target stylesheet found";
        return;
    }

    ExceptionCode ec = 0;
    OwnPtr<AddRuleAction> action = adoptPtr(new AddRuleAction(inspectorStyleSheet, selector));
    AddRuleAction* rawAction = action.get();
    bool success = m_domAgent->history()->perform(action.release(), ec);
    if (!success) {
        *errorString = InspectorDOMAgent::toErrorString(ec);
        return;
    }

    InspectorCSSId ruleId = rawAction->newRuleId();
    CSSStyleRule* rule = inspectorStyleSheet->ruleForId(ruleId);
    result = inspectorStyleSheet->buildObjectForRule(rule);
}

void InspectorCSSAgent::getSupportedCSSProperties(ErrorString*, RefPtr<InspectorArray>& cssProperties)
{
    RefPtr<InspectorArray> properties = InspectorArray::create();
    for (int i = 0; i < numCSSProperties; ++i)
        properties->pushString(propertyNameStrings[i]);

    cssProperties = properties.release();
}

void InspectorCSSAgent::startSelectorProfiler(ErrorString*)
{
    m_currentSelectorProfile = adoptPtr(new SelectorProfile());
    m_state->setBoolean(CSSAgentState::isSelectorProfiling, true);
}

void InspectorCSSAgent::stopSelectorProfiler(ErrorString* errorString, RefPtr<InspectorObject>& result)
{
    stopSelectorProfilerImpl(errorString, &result);
}

void InspectorCSSAgent::stopSelectorProfilerImpl(ErrorString*, RefPtr<InspectorObject>* result)
{
    if (!m_state->getBoolean(CSSAgentState::isSelectorProfiling))
        return;
    m_state->setBoolean(CSSAgentState::isSelectorProfiling, false);
    if (m_frontend && result)
        *result = m_currentSelectorProfile->toInspectorObject();
    m_currentSelectorProfile.clear();
}

void InspectorCSSAgent::willMatchRule(const CSSStyleRule* rule)
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->startSelector(rule);
}

void InspectorCSSAgent::didMatchRule(bool matched)
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->commitSelector(matched);
}

void InspectorCSSAgent::willProcessRule(const CSSStyleRule* rule)
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->startSelector(rule);
}

void InspectorCSSAgent::didProcessRule()
{
    if (m_currentSelectorProfile)
        m_currentSelectorProfile->commitSelectorTime();
}

InspectorStyleSheetForInlineStyle* InspectorCSSAgent::asInspectorStyleSheet(Element* element)
{
    NodeToInspectorStyleSheet::iterator it = m_nodeToInspectorStyleSheet.find(element);
    if (it == m_nodeToInspectorStyleSheet.end()) {
        CSSStyleDeclaration* style = element->isStyledElement() ? element->style() : 0;
        if (!style)
            return 0;

        String newStyleSheetId = String::number(m_lastStyleSheetId++);
        RefPtr<InspectorStyleSheetForInlineStyle> inspectorStyleSheet = InspectorStyleSheetForInlineStyle::create(newStyleSheetId, element, "regular", this);
        m_idToInspectorStyleSheet.set(newStyleSheetId, inspectorStyleSheet);
        m_nodeToInspectorStyleSheet.set(element, inspectorStyleSheet);
        return inspectorStyleSheet.get();
    }

    return it->second.get();
}

Element* InspectorCSSAgent::elementForId(ErrorString* errorString, int nodeId)
{
    Node* node = m_domAgent->nodeForId(nodeId);
    if (!node) {
        *errorString = "No node with given id found";
        return 0;
    }
    if (node->nodeType() != Node::ELEMENT_NODE) {
        *errorString = "Not an element node";
        return 0;
    }
    return static_cast<Element*>(node);
}

void InspectorCSSAgent::collectStyleSheets(CSSStyleSheet* styleSheet, InspectorArray* result)
{
    InspectorStyleSheet* inspectorStyleSheet = bindStyleSheet(static_cast<CSSStyleSheet*>(styleSheet));
    result->pushObject(inspectorStyleSheet->buildObjectForStyleSheetInfo());
    for (unsigned i = 0, size = styleSheet->length(); i < size; ++i) {
        CSSRule* rule = styleSheet->item(i);
        if (rule->isImportRule()) {
            CSSStyleSheet* importedStyleSheet = static_cast<CSSImportRule*>(rule)->styleSheet();
            if (importedStyleSheet)
                collectStyleSheets(importedStyleSheet, result);
        }
    }
}

InspectorStyleSheet* InspectorCSSAgent::bindStyleSheet(CSSStyleSheet* styleSheet)
{
    RefPtr<InspectorStyleSheet> inspectorStyleSheet = m_cssStyleSheetToInspectorStyleSheet.get(styleSheet);
    if (!inspectorStyleSheet) {
        String id = String::number(m_lastStyleSheetId++);
        Document* document = styleSheet->findDocument();
        inspectorStyleSheet = InspectorStyleSheet::create(id, styleSheet, detectOrigin(styleSheet, document), InspectorDOMAgent::documentURLString(document), this);
        m_idToInspectorStyleSheet.set(id, inspectorStyleSheet);
        m_cssStyleSheetToInspectorStyleSheet.set(styleSheet, inspectorStyleSheet);
    }
    return inspectorStyleSheet.get();
}

InspectorStyleSheet* InspectorCSSAgent::viaInspectorStyleSheet(Document* document, bool createIfAbsent)
{
    if (!document) {
        ASSERT(!createIfAbsent);
        return 0;
    }

    RefPtr<InspectorStyleSheet> inspectorStyleSheet = m_documentToInspectorStyleSheet.get(document);
    if (inspectorStyleSheet || !createIfAbsent)
        return inspectorStyleSheet.get();

    ExceptionCode ec = 0;
    RefPtr<Element> styleElement = document->createElement("style", ec);
    if (!ec)
        styleElement->setAttribute("type", "text/css", ec);
    if (!ec) {
        ContainerNode* targetNode;
        // HEAD is absent in ImageDocuments, for example.
        if (document->head())
            targetNode = document->head();
        else if (document->body())
            targetNode = document->body();
        else
            return 0;
        targetNode->appendChild(styleElement, ec);
    }
    if (ec)
        return 0;
    StyleSheetList* styleSheets = document->styleSheets();
    StyleSheet* styleSheet = styleSheets->item(styleSheets->length() - 1);
    if (!styleSheet->isCSSStyleSheet())
        return 0;
    CSSStyleSheet* cssStyleSheet = static_cast<CSSStyleSheet*>(styleSheet);
    String id = String::number(m_lastStyleSheetId++);
    inspectorStyleSheet = InspectorStyleSheet::create(id, cssStyleSheet, "inspector", InspectorDOMAgent::documentURLString(document), this);
    m_idToInspectorStyleSheet.set(id, inspectorStyleSheet);
    m_cssStyleSheetToInspectorStyleSheet.set(cssStyleSheet, inspectorStyleSheet);
    m_documentToInspectorStyleSheet.set(document, inspectorStyleSheet);
    return inspectorStyleSheet.get();
}

InspectorStyleSheet* InspectorCSSAgent::assertStyleSheetForId(ErrorString* errorString, const String& styleSheetId)
{
    IdToInspectorStyleSheet::iterator it = m_idToInspectorStyleSheet.find(styleSheetId);
    if (it == m_idToInspectorStyleSheet.end()) {
        *errorString = "No style sheet with given id found";
        return 0;
    }
    return it->second.get();
}

String InspectorCSSAgent::detectOrigin(CSSStyleSheet* pageStyleSheet, Document* ownerDocument)
{
    DEFINE_STATIC_LOCAL(String, userAgent, ("user-agent"));
    DEFINE_STATIC_LOCAL(String, user, ("user"));
    DEFINE_STATIC_LOCAL(String, inspector, ("inspector"));

    String origin("regular");
    if (pageStyleSheet && !pageStyleSheet->ownerNode() && pageStyleSheet->href().isEmpty())
        origin = userAgent;
    else if (pageStyleSheet && pageStyleSheet->ownerNode() && pageStyleSheet->ownerNode()->nodeName() == "#document")
        origin = user;
    else {
        InspectorStyleSheet* viaInspectorStyleSheetForOwner = viaInspectorStyleSheet(ownerDocument, false);
        if (viaInspectorStyleSheetForOwner && pageStyleSheet == viaInspectorStyleSheetForOwner->pageStyleSheet())
            origin = inspector;
    }
    return origin;
}

PassRefPtr<InspectorArray> InspectorCSSAgent::buildArrayForRuleList(CSSRuleList* ruleList)
{
    RefPtr<InspectorArray> result = InspectorArray::create();
    if (!ruleList)
        return result.release();

    for (unsigned i = 0, size = ruleList->length(); i < size; ++i) {
        CSSStyleRule* rule = asCSSStyleRule(ruleList->item(i));
        if (!rule)
            continue;

        InspectorStyleSheet* styleSheet = bindStyleSheet(rule->parentStyleSheet());
        if (styleSheet)
            result->pushObject(styleSheet->buildObjectForRule(rule));
    }
    return result.release();
}

PassRefPtr<InspectorObject> InspectorCSSAgent::buildObjectForAttributesStyle(Element* element)
{
    if (!element->isStyledElement())
        return 0;

    StylePropertySet* attributeStyle = static_cast<StyledElement*>(element)->attributeStyle();
    if (!attributeStyle)
        return 0;

    RefPtr<InspectorStyle> inspectorStyle = InspectorStyle::create(InspectorCSSId(), attributeStyle->ensureCSSStyleDeclaration(), 0);
    return inspectorStyle->buildObjectForStyle();
}

void InspectorCSSAgent::didRemoveDocument(Document* document)
{
    if (document)
        m_documentToInspectorStyleSheet.remove(document);
    clearPseudoState(false);
}

void InspectorCSSAgent::didRemoveDOMNode(Node* node)
{
    if (!node)
        return;

    if (m_lastElementWithPseudoState.get() == node) {
        clearPseudoState(false);
        return;
    }

    NodeToInspectorStyleSheet::iterator it = m_nodeToInspectorStyleSheet.find(node);
    if (it == m_nodeToInspectorStyleSheet.end())
        return;

    m_idToInspectorStyleSheet.remove(it->second->id());
    m_nodeToInspectorStyleSheet.remove(node);
}

void InspectorCSSAgent::didModifyDOMAttr(Element* element)
{
    if (!element)
        return;

    NodeToInspectorStyleSheet::iterator it = m_nodeToInspectorStyleSheet.find(element);
    if (it == m_nodeToInspectorStyleSheet.end())
        return;

    it->second->didModifyElementAttribute();
}

void InspectorCSSAgent::styleSheetChanged(InspectorStyleSheet* styleSheet)
{
    if (m_frontend)
        m_frontend->styleSheetChanged(styleSheet->id());
}

void InspectorCSSAgent::clearPseudoState(bool recalcStyles)
{
    RefPtr<Element> element = m_lastElementWithPseudoState;
    m_lastElementWithPseudoState = 0;
    m_lastPseudoState = 0;
    if (recalcStyles && element) {
        Document* document = element->ownerDocument();
        if (document)
            document->styleSelectorChanged(RecalcStyleImmediately);
    }
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
