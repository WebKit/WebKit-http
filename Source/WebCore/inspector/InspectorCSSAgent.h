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

#ifndef InspectorCSSAgent_h
#define InspectorCSSAgent_h

#include "CSSSelector.h"
#include "Document.h"
#include "InspectorBaseAgent.h"
#include "InspectorDOMAgent.h"
#include "InspectorStyleSheet.h"
#include "InspectorValues.h"
#include "PlatformString.h"

#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSRule;
class CSSRuleList;
class CSSStyleDeclaration;
class CSSStyleRule;
class CSSStyleSheet;
class Document;
class Element;
class InspectorFrontend;
class InstrumentingAgents;
class NameNodeMap;
class Node;
class SelectorProfile;

#if ENABLE(INSPECTOR)

class InspectorCSSAgent
    : public InspectorBaseAgent<InspectorCSSAgent>
    , public InspectorDOMAgent::DOMListener
    , public InspectorBackendDispatcher::CSSCommandHandler
    , public InspectorStyleSheet::Listener {
    WTF_MAKE_NONCOPYABLE(InspectorCSSAgent);
public:
    static CSSStyleRule* asCSSStyleRule(CSSRule*);

    static PassOwnPtr<InspectorCSSAgent> create(InstrumentingAgents* instrumentingAgents, InspectorState* state, InspectorDOMAgent* domAgent)
    {
        return adoptPtr(new InspectorCSSAgent(instrumentingAgents, state, domAgent));
    }
    ~InspectorCSSAgent();

    bool forcePseudoState(Element*, CSSSelector::PseudoType);
    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();
    virtual void discardAgent();
    virtual void restore();
    virtual void enable(ErrorString*);
    virtual void disable(ErrorString*);
    void reset();
    void mediaQueryResultChanged();

    virtual void getComputedStyleForNode(ErrorString*, int nodeId, const RefPtr<InspectorArray>* forcedPseudoClasses, RefPtr<InspectorArray>& style);
    virtual void getInlineStylesForNode(ErrorString*, int nodeId, RefPtr<InspectorObject>& inlineStyle, RefPtr<InspectorObject>& attributes);
    virtual void getMatchedStylesForNode(ErrorString*, int nodeId, const RefPtr<InspectorArray>* forcedPseudoClasses, const bool* includePseudo, const bool* includeInherited, RefPtr<InspectorArray>& matchedCSSRules, RefPtr<InspectorArray>& pseudoIdRules, RefPtr<InspectorArray>& inheritedEntries);
    virtual void getAllStyleSheets(ErrorString*, RefPtr<InspectorArray>& styleSheetInfos);
    virtual void getStyleSheet(ErrorString*, const String& styleSheetId, RefPtr<InspectorObject>& result);
    virtual void getStyleSheetText(ErrorString*, const String& styleSheetId, String* result);
    virtual void setStyleSheetText(ErrorString*, const String& styleSheetId, const String& text);
    virtual void setPropertyText(ErrorString*, const RefPtr<InspectorObject>& styleId, int propertyIndex, const String& text, bool overwrite, RefPtr<InspectorObject>& result);
    virtual void toggleProperty(ErrorString*, const RefPtr<InspectorObject>& styleId, int propertyIndex, bool disable, RefPtr<InspectorObject>& result);
    virtual void setRuleSelector(ErrorString*, const RefPtr<InspectorObject>& ruleId, const String& selector, RefPtr<InspectorObject>& result);
    virtual void addRule(ErrorString*, int contextNodeId, const String& selector, RefPtr<InspectorObject>& result);
    virtual void getSupportedCSSProperties(ErrorString*, RefPtr<InspectorArray>& result);

    virtual void startSelectorProfiler(ErrorString*);
    virtual void stopSelectorProfiler(ErrorString*, RefPtr<InspectorObject>&);

    void stopSelectorProfilerImpl(ErrorString*, RefPtr<InspectorObject>* = 0);
    void willMatchRule(const CSSStyleRule*);
    void didMatchRule(bool);
    void willProcessRule(const CSSStyleRule*);
    void didProcessRule();

private:
    class StyleSheetAction;
    class SetStyleSheetTextAction;
    class SetPropertyTextAction;
    class TogglePropertyAction;
    class SetRuleSelectorAction;
    class AddRuleAction;

    InspectorCSSAgent(InstrumentingAgents*, InspectorState*, InspectorDOMAgent*);

    typedef HashMap<String, RefPtr<InspectorStyleSheet> > IdToInspectorStyleSheet;
    typedef HashMap<CSSStyleSheet*, RefPtr<InspectorStyleSheet> > CSSStyleSheetToInspectorStyleSheet;
    typedef HashMap<Node*, RefPtr<InspectorStyleSheetForInlineStyle> > NodeToInspectorStyleSheet; // bogus "stylesheets" with elements' inline styles
    typedef HashMap<RefPtr<Document>, RefPtr<InspectorStyleSheet> > DocumentToViaInspectorStyleSheet; // "via inspector" stylesheets

    void recalcStyleForPseudoStateIfNeeded(Element*, InspectorArray* forcedPseudoClasses);
    InspectorStyleSheetForInlineStyle* asInspectorStyleSheet(Element* element);
    Element* elementForId(ErrorString*, int nodeId);
    void collectStyleSheets(CSSStyleSheet*, InspectorArray*);

    InspectorStyleSheet* bindStyleSheet(CSSStyleSheet*);
    InspectorStyleSheet* viaInspectorStyleSheet(Document*, bool createIfAbsent);
    InspectorStyleSheet* assertStyleSheetForId(ErrorString*, const String&);
    String detectOrigin(CSSStyleSheet* pageStyleSheet, Document* ownerDocument);

    PassRefPtr<InspectorArray> buildArrayForRuleList(CSSRuleList* ruleList);
    PassRefPtr<InspectorObject> buildObjectForAttributesStyle(Element*);

    // InspectorDOMAgent::DOMListener implementation
    virtual void didRemoveDocument(Document*);
    virtual void didRemoveDOMNode(Node*);
    virtual void didModifyDOMAttr(Element*);

    // InspectorCSSAgent::Listener implementation
    virtual void styleSheetChanged(InspectorStyleSheet*);

    void clearPseudoState(bool recalcStyles);

    InspectorFrontend::CSS* m_frontend;
    InspectorDOMAgent* m_domAgent;

    IdToInspectorStyleSheet m_idToInspectorStyleSheet;
    CSSStyleSheetToInspectorStyleSheet m_cssStyleSheetToInspectorStyleSheet;
    NodeToInspectorStyleSheet m_nodeToInspectorStyleSheet;
    DocumentToViaInspectorStyleSheet m_documentToInspectorStyleSheet;

    RefPtr<Element> m_lastElementWithPseudoState;
    unsigned m_lastPseudoState;

    int m_lastStyleSheetId;
    int m_lastRuleId;
    int m_lastStyleId;

    OwnPtr<SelectorProfile> m_currentSelectorProfile;
};

#endif

} // namespace WebCore

#endif // !defined(InspectorCSSAgent_h)
