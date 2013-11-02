/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef CSSStyleSheet_h
#define CSSStyleSheet_h

#include "CSSParserMode.h"
#include "CSSRule.h"
#include "StyleSheet.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class CSSCharsetRule;
class CSSImportRule;
class CSSParser;
class CSSRule;
class CSSRuleList;
class CSSStyleSheet;
class CachedCSSStyleSheet;
class Document;
class MediaQuerySet;
class SecurityOrigin;
class StyleRuleKeyframes;
class StyleSheetContents;

typedef int ExceptionCode;

class CSSStyleSheet FINAL : public StyleSheet {
public:
    static PassRef<CSSStyleSheet> create(PassRef<StyleSheetContents>, CSSImportRule* ownerRule = 0);
    static PassRef<CSSStyleSheet> create(PassRef<StyleSheetContents>, Node* ownerNode);
    static PassRef<CSSStyleSheet> createInline(Node&, const URL&, const String& encoding = String());

    virtual ~CSSStyleSheet();

    virtual CSSStyleSheet* parentStyleSheet() const OVERRIDE;
    virtual Node* ownerNode() const OVERRIDE { return m_ownerNode; }
    virtual MediaList* media() const OVERRIDE;
    virtual String href() const OVERRIDE;
    virtual String title() const OVERRIDE { return m_title; }
    virtual bool disabled() const OVERRIDE { return m_isDisabled; }
    virtual void setDisabled(bool) OVERRIDE;
    
    PassRefPtr<CSSRuleList> cssRules();
    unsigned insertRule(const String& rule, unsigned index, ExceptionCode&);
    void deleteRule(unsigned index, ExceptionCode&);
    
    // IE Extensions
    PassRefPtr<CSSRuleList> rules();
    int addRule(const String& selector, const String& style, int index, ExceptionCode&);
    int addRule(const String& selector, const String& style, ExceptionCode&);
    void removeRule(unsigned index, ExceptionCode& ec) { deleteRule(index, ec); }
    
    // For CSSRuleList.
    unsigned length() const;
    CSSRule* item(unsigned index);

    virtual void clearOwnerNode() OVERRIDE;
    virtual CSSImportRule* ownerRule() const OVERRIDE { return m_ownerRule; }
    virtual URL baseURL() const OVERRIDE;
    virtual bool isLoading() const OVERRIDE;
    
    void clearOwnerRule() { m_ownerRule = 0; }
    Document* ownerDocument() const;
    MediaQuerySet* mediaQueries() const { return m_mediaQueries.get(); }
    void setMediaQueries(PassRefPtr<MediaQuerySet>);
    void setTitle(const String& title) { m_title = title; }

    enum RuleMutationType { OtherMutation, RuleInsertion };
    enum WhetherContentsWereClonedForMutation { ContentsWereNotClonedForMutation = 0, ContentsWereClonedForMutation };

    class RuleMutationScope {
        WTF_MAKE_NONCOPYABLE(RuleMutationScope);
    public:
        RuleMutationScope(CSSStyleSheet*, RuleMutationType = OtherMutation, StyleRuleKeyframes* insertedKeyframesRule = nullptr);
        RuleMutationScope(CSSRule*);
        ~RuleMutationScope();

    private:
        CSSStyleSheet* m_styleSheet;
        RuleMutationType m_mutationType;
        WhetherContentsWereClonedForMutation m_contentsWereClonedForMutation;
        StyleRuleKeyframes* m_insertedKeyframesRule;
    };

    WhetherContentsWereClonedForMutation willMutateRules();
    void didMutateRules(RuleMutationType, WhetherContentsWereClonedForMutation, StyleRuleKeyframes* insertedKeyframesRule);
    void didMutateRuleFromCSSStyleDeclaration();
    void didMutate();
    
    void clearChildRuleCSSOMWrappers();
    void reattachChildRuleCSSOMWrappers();

    StyleSheetContents* contents() { return &m_contents.get(); }

    void detachFromDocument() { m_ownerNode = nullptr; }

private:
    CSSStyleSheet(PassRef<StyleSheetContents>, CSSImportRule* ownerRule);
    CSSStyleSheet(PassRef<StyleSheetContents>, Node* ownerNode, bool isInlineStylesheet);

    virtual bool isCSSStyleSheet() const OVERRIDE { return true; }
    virtual String type() const OVERRIDE { return ASCIILiteral("text/css"); }

    bool canAccessRules() const;
    
    Ref<StyleSheetContents> m_contents;
    bool m_isInlineStylesheet;
    bool m_isDisabled;
    String m_title;
    RefPtr<MediaQuerySet> m_mediaQueries;

    Node* m_ownerNode;
    CSSImportRule* m_ownerRule;

    mutable RefPtr<MediaList> m_mediaCSSOMWrapper;
    mutable Vector<RefPtr<CSSRule>> m_childRuleCSSOMWrappers;
    mutable OwnPtr<CSSRuleList> m_ruleListCSSOMWrapper;
};

} // namespace

#endif
