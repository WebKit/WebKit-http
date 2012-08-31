%{

/*
 *  Copyright (C) 2002-2003 Lars Knoll (knoll@kde.org)
 *  Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 *  Copyright (C) 2008 Eric Seidel <eric@webkit.org>
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
 *
 */

#include "config.h"

#include "CSSParser.h"
#include "CSSParserMode.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyNames.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "Document.h"
#include "HTMLNames.h"
#include "MediaList.h"
#include "MediaQueryExp.h"
#include "StyleRule.h"
#include "StyleSheetContents.h"
#include "WebKitCSSKeyframeRule.h"
#include "WebKitCSSKeyframesRule.h"
#include <wtf/FastMalloc.h>
#include <stdlib.h>
#include <string.h>

using namespace WebCore;
using namespace HTMLNames;

#define YYMALLOC fastMalloc
#define YYFREE fastFree

#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1
#define YYMAXDEPTH 10000
#define YYDEBUG 0

%}

%pure_parser

%parse-param { CSSParser* parser }
%lex-param { CSSParser* parser }

%union {
    bool boolean;
    char character;
    int integer;
    double number;
    CSSParserString string;

    StyleRuleBase* rule;
    Vector<RefPtr<StyleRuleBase> >* ruleList;
    CSSParserSelector* selector;
    CSSSelectorVector* selectorList;
    CSSSelector::MarginBoxType marginBox;
    CSSSelector::Relation relation;
    MediaQuerySet* mediaList;
    MediaQuery* mediaQuery;
    MediaQuery::Restrictor mediaQueryRestrictor;
    MediaQueryExp* mediaQueryExp;
    CSSParserValue value;
    CSSParserValueList* valueList;
    Vector<OwnPtr<MediaQueryExp> >* mediaQueryExpList;
    StyleKeyframe* keyframe;
    Vector<RefPtr<StyleKeyframe> >* keyframeRuleList;
    float val;
    CSSPropertyID id;
}

%{

static inline int cssyyerror(void*, const char*)
{
    return 1;
}

%}

%expect 63

%nonassoc LOWEST_PREC

%left UNIMPORTANT_TOK

%token WHITESPACE SGML_CD
%token TOKEN_EOF 0

%token INCLUDES
%token DASHMATCH
%token BEGINSWITH
%token ENDSWITH
%token CONTAINS

%token <string> STRING
%right <string> IDENT
%token <string> NTH

%nonassoc <string> HEX
%nonassoc <string> IDSEL
%nonassoc ':'
%nonassoc '.'
%nonassoc '['
%nonassoc <string> '*'
%nonassoc error
%left '|'

%token IMPORT_SYM
%token PAGE_SYM
%token MEDIA_SYM
%token FONT_FACE_SYM
%token CHARSET_SYM
%token NAMESPACE_SYM
%token VARFUNCTION
%token WEBKIT_RULE_SYM
%token WEBKIT_DECLS_SYM
%token WEBKIT_KEYFRAME_RULE_SYM
%token WEBKIT_KEYFRAMES_SYM
%token WEBKIT_VALUE_SYM
%token WEBKIT_MEDIAQUERY_SYM
%token WEBKIT_SELECTOR_SYM
%token WEBKIT_REGION_RULE_SYM
%token <marginBox> TOPLEFTCORNER_SYM
%token <marginBox> TOPLEFT_SYM
%token <marginBox> TOPCENTER_SYM
%token <marginBox> TOPRIGHT_SYM
%token <marginBox> TOPRIGHTCORNER_SYM
%token <marginBox> BOTTOMLEFTCORNER_SYM
%token <marginBox> BOTTOMLEFT_SYM
%token <marginBox> BOTTOMCENTER_SYM
%token <marginBox> BOTTOMRIGHT_SYM
%token <marginBox> BOTTOMRIGHTCORNER_SYM
%token <marginBox> LEFTTOP_SYM
%token <marginBox> LEFTMIDDLE_SYM
%token <marginBox> LEFTBOTTOM_SYM
%token <marginBox> RIGHTTOP_SYM
%token <marginBox> RIGHTMIDDLE_SYM
%token <marginBox> RIGHTBOTTOM_SYM

%token ATKEYWORD

%token IMPORTANT_SYM
%token MEDIA_ONLY
%token MEDIA_NOT
%token MEDIA_AND

%token <number> REMS
%token <number> QEMS
%token <number> EMS
%token <number> EXS
%token <number> PXS
%token <number> CMS
%token <number> MMS
%token <number> INS
%token <number> PTS
%token <number> PCS
%token <number> DEGS
%token <number> RADS
%token <number> GRADS
%token <number> TURNS
%token <number> MSECS
%token <number> SECS
%token <number> HERTZ
%token <number> KHERTZ
%token <string> DIMEN
%token <string> INVALIDDIMEN
%token <number> PERCENTAGE
%token <number> FLOATTOKEN
%token <number> INTEGER
%token <number> VW
%token <number> VH
%token <number> VMIN
%token <number> DPPX
%token <number> DPI
%token <number> DPCM

%token <string> URI
%token <string> FUNCTION
%token <string> ANYFUNCTION
%token <string> NOTFUNCTION
%token <string> CALCFUNCTION
%token <string> MINFUNCTION
%token <string> MAXFUNCTION
%token <string> VAR_DEFINITION

%token <string> UNICODERANGE

%type <relation> combinator

%type <rule> charset
%type <rule> ignored_charset
%type <rule> ruleset
%type <rule> media
%type <rule> import
%type <rule> namespace
%type <rule> page
%type <rule> margin_box
%type <rule> font_face
%type <rule> keyframes
%type <rule> invalid_rule
%type <rule> save_block
%type <rule> invalid_at
%type <rule> rule
%type <rule> valid_rule
%type <ruleList> block_rule_list 
%type <rule> block_rule
%type <rule> block_valid_rule
%type <rule> region

%type <string> maybe_ns_prefix

%type <string> namespace_selector

%type <string> string_or_uri
%type <string> ident_or_string
%type <string> medium
%type <marginBox> margin_sym

%type <string> media_feature
%type <mediaList> media_list
%type <mediaList> maybe_media_list
%type <mediaQuery> media_query
%type <mediaQueryRestrictor> maybe_media_restrictor
%type <valueList> maybe_media_value
%type <mediaQueryExp> media_query_exp
%type <mediaQueryExpList> media_query_exp_list
%type <mediaQueryExpList> maybe_and_media_query_exp_list

%type <string> keyframe_name
%type <keyframe> keyframe_rule
%type <keyframeRuleList> keyframes_rule
%type <valueList> key_list
%type <value> key

%type <id> property

%type <selector> specifier
%type <selector> specifier_list
%type <selector> simple_selector
%type <selector> selector
%type <selectorList> selector_list
%type <selectorList> simple_selector_list
%type <selectorList> region_selector
%type <selector> selector_with_trailing_whitespace
%type <selector> class
%type <selector> attrib
%type <selector> pseudo
%type <selector> pseudo_page
%type <selector> page_selector

%type <boolean> declaration_list
%type <boolean> decl_list
%type <boolean> declaration
%type <boolean> declarations_and_margins

%type <boolean> prio

%type <integer> match
%type <integer> unary_operator
%type <integer> maybe_unary_operator
%type <character> operator

%type <valueList> expr
%type <value> term
%type <value> unary_term
%type <value> function
%type <value> calc_func_term
%type <character> calc_func_operator
%type <valueList> calc_func_expr
%type <valueList> calc_func_expr_list
%type <valueList> calc_func_paren_expr
%type <value> calc_function
%type <string> min_or_max
%type <value> min_or_max_function

%type <string> element_name
%type <string> attr_name

%%

stylesheet:
    maybe_space maybe_charset maybe_sgml rule_list
  | webkit_rule maybe_space
  | webkit_decls maybe_space
  | webkit_value maybe_space
  | webkit_mediaquery maybe_space
  | webkit_selector maybe_space
  | webkit_keyframe_rule maybe_space
  ;

webkit_rule:
    WEBKIT_RULE_SYM '{' maybe_space valid_rule maybe_space '}' {
        parser->m_rule = $4;
    }
;

webkit_keyframe_rule:
    WEBKIT_KEYFRAME_RULE_SYM '{' maybe_space keyframe_rule maybe_space '}' {
        parser->m_keyframe = $4;
    }
;

webkit_decls:
    WEBKIT_DECLS_SYM '{' maybe_space_before_declaration declaration_list '}' {
        /* can be empty */
    }
;

webkit_value:
    WEBKIT_VALUE_SYM '{' maybe_space expr '}' {
        if ($4) {
            parser->m_valueList = parser->sinkFloatingValueList($4);
            int oldParsedProperties = parser->m_parsedProperties->size();
            if (!parser->parseValue(parser->m_id, parser->m_important))
                parser->rollbackLastProperties(parser->m_parsedProperties->size() - oldParsedProperties);
            parser->m_valueList = nullptr;
        }
    }
;

webkit_mediaquery:
     WEBKIT_MEDIAQUERY_SYM WHITESPACE maybe_space media_query '}' {
         parser->m_mediaQuery = parser->sinkFloatingMediaQuery($4);
     }
;

webkit_selector:
    WEBKIT_SELECTOR_SYM '{' maybe_space selector_list '}' {
        if ($4) {
            if (parser->m_selectorListForParseSelector)
                parser->m_selectorListForParseSelector->adoptSelectorVector(*$4);
        }
    }
;

maybe_space:
    /* empty */ %prec UNIMPORTANT_TOK
  | maybe_space WHITESPACE
  ;

maybe_sgml:
    /* empty */
  | maybe_sgml SGML_CD
  | maybe_sgml WHITESPACE
  ;

maybe_charset:
   /* empty */
  | charset {
  }
  ;

closing_brace:
    '}'
  | %prec LOWEST_PREC TOKEN_EOF
  ;

charset:
  CHARSET_SYM maybe_space STRING maybe_space ';' {
     if (parser->m_styleSheet)
         parser->m_styleSheet->parserSetEncodingFromCharsetRule($3);
     if (parser->isExtractingSourceData() && parser->m_currentRuleDataStack->isEmpty() && parser->m_ruleSourceDataResult)
         parser->addNewRuleToSourceTree(CSSRuleSourceData::createUnknown());
     $$ = 0;
  }
  | CHARSET_SYM error invalid_block {
  }
  | CHARSET_SYM error ';' {
  }
;

ignored_charset:
    CHARSET_SYM maybe_space STRING maybe_space ';' {
        // Ignore any @charset rule not at the beginning of the style sheet.
        $$ = 0;
    }
    | CHARSET_SYM maybe_space ';' {
        $$ = 0;
    }
;

rule_list:
   /* empty */
 | rule_list rule maybe_sgml {
     if ($2 && parser->m_styleSheet)
         parser->m_styleSheet->parserAppendRule($2);
 }
 ;

valid_rule:
    ruleset
  | media
  | page
  | font_face
  | keyframes
  | namespace
  | import
  | region
  ;

rule:
    valid_rule {
        parser->m_hadSyntacticallyValidCSSRule = true;
    }
  | ignored_charset
  | invalid_rule
  | invalid_at
  ;

block_rule_list: 
    /* empty */ { $$ = 0; }
  | block_rule_list block_rule maybe_sgml {
      $$ = $1;
      if ($2) {
          if (!$$)
              $$ = parser->createRuleList();
          $$->append($2);
      }
  }
  ;

block_valid_rule:
    ruleset
  | page
  | font_face
  | keyframes
  ;

block_rule:
    block_valid_rule
  | invalid_rule
  | invalid_at
  | namespace
  | import
  | media
  ;

at_import_header_end_maybe_space:
    maybe_space {
        parser->markRuleHeaderEnd();
        parser->markRuleBodyStart();
    }
    ;

before_import_rule:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::IMPORT_RULE);
    }
    ;

import:
    before_import_rule IMPORT_SYM at_import_header_end_maybe_space string_or_uri maybe_space maybe_media_list ';' {
        $$ = parser->createImportRule($4, $6);
    }
  | before_import_rule IMPORT_SYM at_import_header_end_maybe_space string_or_uri maybe_space maybe_media_list TOKEN_EOF {
        $$ = parser->createImportRule($4, $6);
    }
  | before_import_rule IMPORT_SYM at_import_header_end_maybe_space string_or_uri maybe_space maybe_media_list invalid_block {
        $$ = 0;
        parser->popRuleData();
    }
  | before_import_rule IMPORT_SYM error ';' {
        $$ = 0;
        parser->popRuleData();
    }
  | before_import_rule IMPORT_SYM error invalid_block {
        $$ = 0;
        parser->popRuleData();
    }
  ;

namespace:
NAMESPACE_SYM maybe_space maybe_ns_prefix string_or_uri maybe_space ';' {
    parser->addNamespace($3, $4);
    $$ = 0;
}
| NAMESPACE_SYM maybe_space maybe_ns_prefix string_or_uri maybe_space invalid_block {
    $$ = 0;
}
| NAMESPACE_SYM error invalid_block {
    $$ = 0;
}
| NAMESPACE_SYM error ';' {
    $$ = 0;
}
;

maybe_ns_prefix:
/* empty */ { $$.clear(); }
| IDENT maybe_space { $$ = $1; }
;

string_or_uri:
STRING
| URI
;

media_feature:
    IDENT maybe_space {
        $$ = $1;
    }
    ;

maybe_media_value:
    /*empty*/ {
        $$ = 0;
    }
    | ':' maybe_space expr maybe_space {
        $$ = $3;
    }
    ;

media_query_exp:
    '(' maybe_space media_feature maybe_space maybe_media_value ')' maybe_space {
        $3.lower();
        $$ = parser->createFloatingMediaQueryExp($3, $5);
    }
    ;

media_query_exp_list:
    media_query_exp {
        $$ = parser->createFloatingMediaQueryExpList();
        $$->append(parser->sinkFloatingMediaQueryExp($1));
    }
    | media_query_exp_list maybe_space MEDIA_AND maybe_space media_query_exp {
        $$ = $1;
        $$->append(parser->sinkFloatingMediaQueryExp($5));
    }
    ;

maybe_and_media_query_exp_list:
    /*empty*/ {
        $$ = parser->createFloatingMediaQueryExpList();
    }
    | MEDIA_AND maybe_space media_query_exp_list {
        $$ = $3;
    }
    ;

maybe_media_restrictor:
    /*empty*/ {
        $$ = MediaQuery::None;
    }
    | MEDIA_ONLY {
        $$ = MediaQuery::Only;
    }
    | MEDIA_NOT {
        $$ = MediaQuery::Not;
    }
    ;

media_query:
    media_query_exp_list {
        $$ = parser->createFloatingMediaQuery(parser->sinkFloatingMediaQueryExpList($1));
    }
    |
    maybe_media_restrictor maybe_space medium maybe_and_media_query_exp_list {
        $3.lower();
        $$ = parser->createFloatingMediaQuery($1, $3, parser->sinkFloatingMediaQueryExpList($4));
    }
    ;

maybe_media_list:
     /* empty */ {
        $$ = parser->createMediaQuerySet();
     }
     | media_list
     ;

media_list:
    media_query {
        $$ = parser->createMediaQuerySet();
        $$->addMediaQuery(parser->sinkFloatingMediaQuery($1));
        parser->updateLastMediaLine($$);
    }
    | media_list ',' maybe_space media_query {
        $$ = $1;
        if ($$) {
            $$->addMediaQuery(parser->sinkFloatingMediaQuery($4));
            parser->updateLastMediaLine($$);
        }
    }
    | media_list error {
        $$ = 0;
    }
    ;

at_rule_body_start:
    /* empty */ {
        parser->markRuleBodyStart();
    }
    ;

before_media_rule:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::MEDIA_RULE);
    }
    ;

at_rule_header_end_maybe_space:
    maybe_space {
        parser->markRuleHeaderEnd();
    }
    ;

media:
    before_media_rule MEDIA_SYM maybe_space media_list at_rule_header_end '{' at_rule_body_start maybe_space block_rule_list save_block {
        $$ = parser->createMediaRule($4, $9);
    }
    | before_media_rule MEDIA_SYM at_rule_header_end_maybe_space '{' at_rule_body_start maybe_space block_rule_list save_block {
        $$ = parser->createMediaRule(0, $7);
    }
    | before_media_rule MEDIA_SYM at_rule_header_end_maybe_space ';' {
        $$ = 0;
        parser->popRuleData();
    }
    ;

medium:
  IDENT maybe_space {
      $$ = $1;
  }
  ;

before_keyframes_rule:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::KEYFRAMES_RULE);
    }
    ;

keyframes:
    before_keyframes_rule WEBKIT_KEYFRAMES_SYM maybe_space keyframe_name at_rule_header_end_maybe_space '{' at_rule_body_start maybe_space keyframes_rule closing_brace {
        $$ = parser->createKeyframesRule($4, parser->sinkFloatingKeyframeVector($9));
    }
    ;
  
keyframe_name:
    IDENT
    | STRING
    ;

keyframes_rule:
    /* empty */ { $$ = parser->createFloatingKeyframeVector(); }
    | keyframes_rule keyframe_rule maybe_space {
        $$ = $1;
        if ($2)
            $$->append($2);
    }
    ;

keyframe_rule:
    key_list maybe_space '{' maybe_space declaration_list '}' {
        $$ = parser->createKeyframe($1);
    }
    ;

key_list:
    key {
        $$ = parser->createFloatingValueList();
        $$->addValue(parser->sinkFloatingValue($1));
    }
    | key_list maybe_space ',' maybe_space key {
        $$ = $1;
        if ($$)
            $$->addValue(parser->sinkFloatingValue($5));
    }
    ;

key:
    PERCENTAGE { $$.id = 0; $$.isInt = false; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_NUMBER; }
    | IDENT {
        $$.id = 0; $$.isInt = false; $$.unit = CSSPrimitiveValue::CSS_NUMBER;
        CSSParserString& str = $1;
        if (str.equalIgnoringCase("from"))
            $$.fValue = 0;
        else if (str.equalIgnoringCase("to"))
            $$.fValue = 100;
        else
            YYERROR;
    }
    ;

before_page_rule:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::PAGE_RULE);
    }
    ;

page:
    before_page_rule PAGE_SYM maybe_space page_selector at_rule_header_end_maybe_space
    '{' at_rule_body_start maybe_space_before_declaration declarations_and_margins closing_brace {
        if ($4)
            $$ = parser->createPageRule(parser->sinkFloatingSelector($4));
        else {
            // Clear properties in the invalid @page rule.
            parser->clearProperties();
            // Also clear margin at-rules here once we fully implement margin at-rules parsing.
            $$ = 0;
            parser->popRuleData();
        }
    }
    | before_page_rule PAGE_SYM error invalid_block {
      parser->popRuleData();
      $$ = 0;
    }
    | before_page_rule PAGE_SYM error ';' {
      parser->popRuleData();
      $$ = 0;
    }
    ;

page_selector:
    IDENT {
        $$ = parser->createFloatingSelector();
        $$->setTag(QualifiedName(nullAtom, $1, parser->m_defaultNamespace));
        $$->setForPage();
    }
    | IDENT pseudo_page {
        $$ = $2;
        if ($$) {
            $$->setTag(QualifiedName(nullAtom, $1, parser->m_defaultNamespace));
            $$->setForPage();
        }
    }
    | pseudo_page {
        $$ = $1;
        if ($$)
            $$->setForPage();
    }
    | /* empty */ {
        $$ = parser->createFloatingSelector();
        $$->setForPage();
    }
    ;

declarations_and_margins:
    declaration_list
    | declarations_and_margins margin_box maybe_space declaration_list
    ;

margin_box:
    margin_sym {
        parser->startDeclarationsForMarginBox();
    } maybe_space '{' maybe_space declaration_list closing_brace {
        $$ = parser->createMarginAtRule($1);
    }
    ;

margin_sym :
    TOPLEFTCORNER_SYM {
        $$ = CSSSelector::TopLeftCornerMarginBox;
    }
    | TOPLEFT_SYM {
        $$ = CSSSelector::TopLeftMarginBox;
    }
    | TOPCENTER_SYM {
        $$ = CSSSelector::TopCenterMarginBox;
    }
    | TOPRIGHT_SYM {
        $$ = CSSSelector::TopRightMarginBox;
    }
    | TOPRIGHTCORNER_SYM {
        $$ = CSSSelector::TopRightCornerMarginBox;
    }
    | BOTTOMLEFTCORNER_SYM {
        $$ = CSSSelector::BottomLeftCornerMarginBox;
    }
    | BOTTOMLEFT_SYM {
        $$ = CSSSelector::BottomLeftMarginBox;
    }
    | BOTTOMCENTER_SYM {
        $$ = CSSSelector::BottomCenterMarginBox;
    }
    | BOTTOMRIGHT_SYM {
        $$ = CSSSelector::BottomRightMarginBox;
    }
    | BOTTOMRIGHTCORNER_SYM {
        $$ = CSSSelector::BottomRightCornerMarginBox;
    }
    | LEFTTOP_SYM {
        $$ = CSSSelector::LeftTopMarginBox;
    }
    | LEFTMIDDLE_SYM {
        $$ = CSSSelector::LeftMiddleMarginBox;
    }
    | LEFTBOTTOM_SYM {
        $$ = CSSSelector::LeftBottomMarginBox;
    }
    | RIGHTTOP_SYM {
        $$ = CSSSelector::RightTopMarginBox;
    }
    | RIGHTMIDDLE_SYM {
        $$ = CSSSelector::RightMiddleMarginBox;
    }
    | RIGHTBOTTOM_SYM {
        $$ = CSSSelector::RightBottomMarginBox;
    }
    ;

before_font_face_rule:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::FONT_FACE_RULE);
    }
    ;

font_face:
    before_font_face_rule FONT_FACE_SYM at_rule_header_end_maybe_space
    '{' at_rule_body_start maybe_space_before_declaration declaration_list closing_brace {
        $$ = parser->createFontFaceRule();
    }
    | before_font_face_rule FONT_FACE_SYM error invalid_block {
      $$ = 0;
      parser->popRuleData();
    }
    | before_font_face_rule FONT_FACE_SYM error ';' {
      $$ = 0;
      parser->popRuleData();
    }
;

region_selector:
    selector_list {
        if ($1) {
            parser->setReusableRegionSelectorVector($1);
            $$ = parser->reusableRegionSelectorVector();
        }
        else
            $$ = 0;
    }
;

before_region_rule:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::REGION_RULE);
    }
    ;

region:
    before_region_rule WEBKIT_REGION_RULE_SYM WHITESPACE region_selector at_rule_header_end '{' at_rule_body_start maybe_space block_rule_list save_block {
        if ($4)
            $$ = parser->createRegionRule($4, $9);
        else {
            $$ = 0;
            parser->popRuleData();
        }
    }
;

combinator:
    '+' maybe_space { $$ = CSSSelector::DirectAdjacent; }
  | '~' maybe_space { $$ = CSSSelector::IndirectAdjacent; }
  | '>' maybe_space { $$ = CSSSelector::Child; }
  ;

maybe_unary_operator:
    unary_operator { $$ = $1; }
    | { $$ = 1; }
    ;

unary_operator:
    '-' { $$ = -1; }
  | '+' { $$ = 1; }
  ;

maybe_space_before_declaration:
    maybe_space {
        parser->markPropertyStart();
    }
  ;

before_selector_list:
    /* empty */ {
        parser->markRuleHeaderStart(CSSRuleSourceData::STYLE_RULE);
    }
  ;

at_rule_header_end:
    /* empty */ {
        parser->markRuleHeaderEnd();
    }
  ;

ruleset:
    before_selector_list selector_list at_rule_header_end '{' at_rule_body_start maybe_space_before_declaration declaration_list closing_brace {
        $$ = parser->createStyleRule($2);
    }
  ;

selector_list:
    selector %prec UNIMPORTANT_TOK {
        if ($1) {
            $$ = parser->selectorVector();
            $$->shrink(0);
            $$->append(parser->sinkFloatingSelector($1));
            parser->updateLastSelectorLineAndPosition();
        }
    }
    | selector_list ',' maybe_space selector %prec UNIMPORTANT_TOK {
        if ($1 && $4) {
            $$ = $1;
            $$->append(parser->sinkFloatingSelector($4));
            parser->updateLastSelectorLineAndPosition();
        } else
            $$ = 0;
    }
  | selector_list error {
        $$ = 0;
    }
   ;

selector_with_trailing_whitespace:
    selector WHITESPACE {
        $$ = $1;
    }
    ;

selector:
    simple_selector {
        $$ = $1;
    }
    | selector_with_trailing_whitespace
    {
        $$ = $1;
    }
    | selector_with_trailing_whitespace simple_selector
    {
        $$ = $2;
        if (!$1)
            $$ = 0;
        else if ($$) {
            CSSParserSelector* end = $$;
            while (end->tagHistory())
                end = end->tagHistory();
            end->setRelation(CSSSelector::Descendant);
            end->setTagHistory(parser->sinkFloatingSelector($1));
        }
    }
    | selector combinator simple_selector {
        $$ = $3;
        if (!$1)
            $$ = 0;
        else if ($$) {
            CSSParserSelector* end = $$;
            while (end->tagHistory())
                end = end->tagHistory();
            end->setRelation($2);
            end->setTagHistory(parser->sinkFloatingSelector($1));
        }
    }
    | selector error {
        $$ = 0;
    }
    ;

namespace_selector:
    /* empty */ '|' { $$.clear(); }
    | '*' '|' { static LChar star = '*'; $$.init(&star, 1); }
    | IDENT '|' { $$ = $1; }
;
    
simple_selector:
    element_name {
        $$ = parser->createFloatingSelector();
        $$->setTag(QualifiedName(nullAtom, $1, parser->m_defaultNamespace));
    }
    | element_name specifier_list {
        $$ = $2;
        if ($$)
            parser->updateSpecifiersWithElementName(nullAtom, $1, $$);
    }
    | specifier_list {
        $$ = $1;
        if ($$)
            parser->updateSpecifiersWithElementName(nullAtom, starAtom, $$);
    }
    | namespace_selector element_name {
        $$ = parser->createFloatingSelector();
        $$->setTag(parser->determineNameInNamespace($1, $2));
    }
    | namespace_selector element_name specifier_list {
        $$ = $3;
        if ($$)
            parser->updateSpecifiersWithElementName($1, $2, $$);
    }
    | namespace_selector specifier_list {
        $$ = $2;
        if ($$)
            parser->updateSpecifiersWithElementName($1, starAtom, $$);
    }
  ;

simple_selector_list:
    simple_selector %prec UNIMPORTANT_TOK {
        if ($1) {
            $$ = parser->createFloatingSelectorVector();
            $$->append(parser->sinkFloatingSelector($1));
        } else
            $$ = 0;
    }
    | simple_selector_list maybe_space ',' maybe_space simple_selector %prec UNIMPORTANT_TOK {
        if ($1 && $5) {
            $$ = $1;
            $$->append(parser->sinkFloatingSelector($5));
        } else
            $$ = 0;
    }
    | simple_selector_list error {
        $$ = 0;
    }
  ;

element_name:
    IDENT {
        CSSParserString& str = $1;
        if (parser->m_context.isHTMLDocument)
            str.lower();
        $$ = str;
    }
    | '*' {
        static LChar star = '*';
        $$.init(&star, 1);
    }
  ;

specifier_list:
    specifier {
        $$ = $1;
    }
    | specifier_list specifier {
        if (!$2)
            $$ = 0;
        else if ($1)
            $$ = parser->updateSpecifiers($1, $2);
    }
    | specifier_list error {
        $$ = 0;
    }
;

specifier:
    IDSEL {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::Id);
        if (parser->m_context.mode == CSSQuirksMode)
            $1.lower();
        $$->setValue($1);
    }
  | HEX {
        if ($1[0] >= '0' && $1[0] <= '9') {
            $$ = 0;
        } else {
            $$ = parser->createFloatingSelector();
            $$->setMatch(CSSSelector::Id);
            if (parser->m_context.mode == CSSQuirksMode)
                $1.lower();
            $$->setValue($1);
        }
    }
  | class
  | attrib
  | pseudo
    ;

class:
    '.' IDENT {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::Class);
        if (parser->m_context.mode == CSSQuirksMode)
            $2.lower();
        $$->setValue($2);
    }
  ;

attr_name:
    IDENT maybe_space {
        CSSParserString& str = $1;
        if (parser->m_context.isHTMLDocument)
            str.lower();
        $$ = str;
    }
    ;

attrib:
    '[' maybe_space attr_name ']' {
        $$ = parser->createFloatingSelector();
        $$->setAttribute(QualifiedName(nullAtom, $3, nullAtom));
        $$->setMatch(CSSSelector::Set);
    }
    | '[' maybe_space attr_name match maybe_space ident_or_string maybe_space ']' {
        $$ = parser->createFloatingSelector();
        $$->setAttribute(QualifiedName(nullAtom, $3, nullAtom));
        $$->setMatch((CSSSelector::Match)$4);
        $$->setValue($6);
    }
    | '[' maybe_space namespace_selector attr_name ']' {
        $$ = parser->createFloatingSelector();
        $$->setAttribute(parser->determineNameInNamespace($3, $4));
        $$->setMatch(CSSSelector::Set);
    }
    | '[' maybe_space namespace_selector attr_name match maybe_space ident_or_string maybe_space ']' {
        $$ = parser->createFloatingSelector();
        $$->setAttribute(parser->determineNameInNamespace($3, $4));
        $$->setMatch((CSSSelector::Match)$5);
        $$->setValue($7);
    }
  ;

match:
    '=' {
        $$ = CSSSelector::Exact;
    }
    | INCLUDES {
        $$ = CSSSelector::List;
    }
    | DASHMATCH {
        $$ = CSSSelector::Hyphen;
    }
    | BEGINSWITH {
        $$ = CSSSelector::Begin;
    }
    | ENDSWITH {
        $$ = CSSSelector::End;
    }
    | CONTAINS {
        $$ = CSSSelector::Contain;
    }
    ;

ident_or_string:
    IDENT
  | STRING
    ;

pseudo_page:
    ':' IDENT {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::PagePseudoClass);
        $2.lower();
        $$->setValue($2);
        CSSSelector::PseudoType type = $$->pseudoType();
        if (type == CSSSelector::PseudoUnknown)
            $$ = 0;
    }

pseudo:
    ':' IDENT {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::PseudoClass);
        $2.lower();
        $$->setValue($2);
        CSSSelector::PseudoType type = $$->pseudoType();
        if (type == CSSSelector::PseudoUnknown)
            $$ = 0;
    }
    | ':' ':' IDENT {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::PseudoElement);
        $3.lower();
        $$->setValue($3);
        // FIXME: This call is needed to force selector to compute the pseudoType early enough.
        $$->pseudoType();
    }
    // use by :-webkit-any.
    // FIXME: should we support generic selectors here or just simple_selectors?
    // Use simple_selector_list for now to match -moz-any.
    // See http://lists.w3.org/Archives/Public/www-style/2010Sep/0566.html for some
    // related discussion with respect to :not.
    | ':' ANYFUNCTION maybe_space simple_selector_list maybe_space ')' {
        if ($4) {
            $$ = parser->createFloatingSelector();
            $$->setMatch(CSSSelector::PseudoClass);
            $$->adoptSelectorVector(*parser->sinkFloatingSelectorVector($4));
            $2.lower();
            $$->setValue($2);
            CSSSelector::PseudoType type = $$->pseudoType();
            if (type != CSSSelector::PseudoAny)
                $$ = 0;
        } else
            $$ = 0;
    }
    // used by :nth-*(ax+b)
    | ':' FUNCTION maybe_space NTH maybe_space ')' {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::PseudoClass);
        $$->setArgument($4);
        $$->setValue($2);
        CSSSelector::PseudoType type = $$->pseudoType();
        if (type == CSSSelector::PseudoUnknown)
            $$ = 0;
    }
    // used by :nth-*
    | ':' FUNCTION maybe_space maybe_unary_operator INTEGER maybe_space ')' {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::PseudoClass);
        $$->setArgument(String::number($4 * $5));
        $$->setValue($2);
        CSSSelector::PseudoType type = $$->pseudoType();
        if (type == CSSSelector::PseudoUnknown)
            $$ = 0;
    }
    // used by :nth-*(odd/even) and :lang
    | ':' FUNCTION maybe_space IDENT maybe_space ')' {
        $$ = parser->createFloatingSelector();
        $$->setMatch(CSSSelector::PseudoClass);
        $$->setArgument($4);
        $2.lower();
        $$->setValue($2);
        CSSSelector::PseudoType type = $$->pseudoType();
        if (type == CSSSelector::PseudoUnknown)
            $$ = 0;
        else if (type == CSSSelector::PseudoNthChild ||
                 type == CSSSelector::PseudoNthOfType ||
                 type == CSSSelector::PseudoNthLastChild ||
                 type == CSSSelector::PseudoNthLastOfType) {
            if (!isValidNthToken($4))
                $$ = 0;
        }
    }
    // used by :not
    | ':' NOTFUNCTION maybe_space simple_selector maybe_space ')' {
        if (!$4 || !$4->isSimple())
            $$ = 0;
        else {
            $$ = parser->createFloatingSelector();
            $$->setMatch(CSSSelector::PseudoClass);

            CSSSelectorVector selectorVector;
            selectorVector.append(parser->sinkFloatingSelector($4));
            $$->adoptSelectorVector(selectorVector);

            $2.lower();
            $$->setValue($2);
        }
    }
  ;

declaration_list:
    declaration {
        $$ = $1;
    }
    | decl_list declaration {
        $$ = $1;
        if ( $2 )
            $$ = $2;
    }
    | decl_list {
        $$ = $1;
    }
    | error invalid_block_list error {
        $$ = false;
    }
    | error {
        $$ = false;
    }
    | decl_list error {
        $$ = $1;
    }
    | decl_list invalid_block_list {
        $$ = $1;
    }
    ;

decl_list:
    declaration ';' maybe_space {
        parser->markPropertyStart();
        $$ = $1;
    }
    | declaration invalid_block_list maybe_space {
        $$ = false;
    }
    | declaration invalid_block_list ';' maybe_space {
        $$ = false;
    }
    | error ';' maybe_space {
        parser->markPropertyStart();
        $$ = false;
    }
    | error invalid_block_list error ';' maybe_space {
        $$ = false;
    }
    | decl_list declaration ';' maybe_space {
        parser->markPropertyStart();
        $$ = $1;
        if ($2)
            $$ = $2;
    }
    | decl_list error ';' maybe_space {
        parser->markPropertyStart();
        $$ = $1;
    }
    | decl_list error invalid_block_list error ';' maybe_space {
        parser->markPropertyStart();
        $$ = $1;
    }
    ;

declaration:
    VAR_DEFINITION ':' maybe_space expr prio {
#if ENABLE(CSS_VARIABLES)
        parser->storeVariableDeclaration($1, parser->sinkFloatingValueList($4), $5);
        $$ = true;
        parser->markPropertyEnd($5, true);
#else
        $$ = false;
#endif
    }
    |
    property ':' maybe_space expr prio {
        $$ = false;
        bool isPropertyParsed = false;
        if ($1 && $4) {
            parser->m_valueList = parser->sinkFloatingValueList($4);
            int oldParsedProperties = parser->m_parsedProperties->size();
            $$ = parser->parseValue(static_cast<CSSPropertyID>($1), $5);
            if (!$$)
                parser->rollbackLastProperties(parser->m_parsedProperties->size() - oldParsedProperties);
            else
                isPropertyParsed = true;
            parser->m_valueList = nullptr;
        }
        parser->markPropertyEnd($5, isPropertyParsed);
    }
    |
    property error {
        $$ = false;
    }
    |
    property ':' maybe_space error expr prio {
        /* The default movable type template has letter-spacing: .none;  Handle this by looking for
        error tokens at the start of an expr, recover the expr and then treat as an error, cleaning
        up and deleting the shifted expr.  */
        parser->markPropertyEnd(false, false);
        $$ = false;
    }
    |
    property ':' maybe_space expr prio error {
        /* When we encounter something like p {color: red !important fail;} we should drop the declaration */
        parser->markPropertyEnd(false, false);
        $$ = false;
    }
    |
    IMPORTANT_SYM maybe_space {
        /* Handle this case: div { text-align: center; !important } Just reduce away the stray !important. */
        $$ = false;
    }
    |
    property ':' maybe_space {
        /* div { font-family: } Just reduce away this property with no value. */
        parser->markPropertyEnd(false, false);
        $$ = false;
    }
    |
    property ':' maybe_space error {
        /* if we come across rules with invalid values like this case: p { weight: *; }, just discard the rule */
        parser->markPropertyEnd(false, false);
        $$ = false;
    }
    |
    property invalid_block {
        /* if we come across: div { color{;color:maroon} }, ignore everything within curly brackets */
        $$ = false;
    }
  ;

property:
    IDENT maybe_space {
        $$ = cssPropertyID($1);
    }
  ;

prio:
    IMPORTANT_SYM maybe_space { $$ = true; }
    | /* empty */ { $$ = false; }
  ;

expr:
    term {
        $$ = parser->createFloatingValueList();
        $$->addValue(parser->sinkFloatingValue($1));
    }
    | expr operator term {
        $$ = $1;
        if ($$) {
            if ($2) {
                CSSParserValue v;
                v.id = 0;
                v.unit = CSSParserValue::Operator;
                v.iValue = $2;
                $$->addValue(v);
            }
            $$->addValue(parser->sinkFloatingValue($3));
        }
    }
    | expr invalid_block_list {
        $$ = 0;
    }
    | expr invalid_block_list error {
        $$ = 0;
    }
    | expr error {
        $$ = 0;
    }
  ;

operator:
    '/' maybe_space {
        $$ = '/';
    }
  | ',' maybe_space {
        $$ = ',';
    }
  | /* empty */ {
        $$ = 0;
  }
  ;

term:
  unary_term { $$ = $1; }
  | unary_operator unary_term { $$ = $2; $$.fValue *= $1; }
  | STRING maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_STRING; }
  | IDENT maybe_space {
      $$.id = cssValueKeywordID($1);
      $$.unit = CSSPrimitiveValue::CSS_IDENT;
      $$.string = $1;
  }
  /* We might need to actually parse the number from a dimension, but we can't just put something that uses $$.string into unary_term. */
  | DIMEN maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_DIMENSION; }
  | unary_operator DIMEN maybe_space { $$.id = 0; $$.string = $2; $$.unit = CSSPrimitiveValue::CSS_DIMENSION; }
  | URI maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_URI; }
  | UNICODERANGE maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_UNICODE_RANGE; }
  | HEX maybe_space { $$.id = 0; $$.string = $1; $$.unit = CSSPrimitiveValue::CSS_PARSER_HEXCOLOR; }
  | '#' maybe_space { $$.id = 0; $$.string = CSSParserString(); $$.unit = CSSPrimitiveValue::CSS_PARSER_HEXCOLOR; } /* Handle error case: "color: #;" */
  | VARFUNCTION maybe_space IDENT ')' maybe_space {
#if ENABLE(CSS_VARIABLES)
      $$.id = 0;
      $$.string = $3;
      $$.unit = CSSPrimitiveValue::CSS_VARIABLE_NAME;
#endif
  }
  /* FIXME: according to the specs a function can have a unary_operator in front. I know no case where this makes sense */
  | function {
      $$ = $1;
  }
  | calc_function {
      $$ = $1;
  }
  | min_or_max_function {
      $$ = $1;
  }
  | '%' maybe_space { /* Handle width: %; */
      $$.id = 0; $$.unit = 0;
  }
  ;

unary_term:
  INTEGER maybe_space { $$.id = 0; $$.isInt = true; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_NUMBER; }
  | FLOATTOKEN maybe_space { $$.id = 0; $$.isInt = false; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_NUMBER; }
  | PERCENTAGE maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PERCENTAGE; }
  | PXS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PX; }
  | CMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_CM; }
  | MMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_MM; }
  | INS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_IN; }
  | PTS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PT; }
  | PCS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_PC; }
  | DEGS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DEG; }
  | RADS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_RAD; }
  | GRADS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_GRAD; }
  | TURNS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_TURN; }
  | MSECS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_MS; }
  | SECS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_S; }
  | HERTZ maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_HZ; }
  | KHERTZ maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_KHZ; }
  | EMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_EMS; }
  | QEMS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSParserValue::Q_EMS; }
  | EXS maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_EXS; }
  | REMS maybe_space {
      $$.id = 0;
      $$.fValue = $1;
      $$.unit = CSSPrimitiveValue::CSS_REMS;
      if (parser->m_styleSheet)
          parser->m_styleSheet->parserSetUsesRemUnits(true);
  }
  | VW maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_VW; }
  | VH maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_VH; }
  | VMIN maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_VMIN; }
  | DPPX maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DPPX; }
  | DPI maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DPI; }
  | DPCM maybe_space { $$.id = 0; $$.fValue = $1; $$.unit = CSSPrimitiveValue::CSS_DPCM; }
  ;

function:
    FUNCTION maybe_space expr ')' maybe_space {
        CSSParserFunction* f = parser->createFloatingFunction();
        f->name = $1;
        f->args = parser->sinkFloatingValueList($3);
        $$.id = 0;
        $$.unit = CSSParserValue::Function;
        $$.function = f;
    } |
    FUNCTION maybe_space expr TOKEN_EOF {
        CSSParserFunction* f = parser->createFloatingFunction();
        f->name = $1;
        f->args = parser->sinkFloatingValueList($3);
        $$.id = 0;
        $$.unit = CSSParserValue::Function;
        $$.function = f;
    } |
    FUNCTION maybe_space ')' maybe_space {
        CSSParserFunction* f = parser->createFloatingFunction();
        f->name = $1;
        CSSParserValueList* valueList = parser->createFloatingValueList();
        f->args = parser->sinkFloatingValueList(valueList);
        $$.id = 0;
        $$.unit = CSSParserValue::Function;
        $$.function = f;
    } |
    FUNCTION maybe_space error {
        CSSParserFunction* f = parser->createFloatingFunction();
        f->name = $1;
        f->args = nullptr;
        $$.id = 0;
        $$.unit = CSSParserValue::Function;
        $$.function = f;
  }
  ;
 
calc_func_term:
  unary_term { $$ = $1; }
  | VARFUNCTION maybe_space IDENT ')' maybe_space {
#if ENABLE(CSS_VARIABLES)
      $$.id = 0;
      $$.string = $3;
      $$.unit = CSSPrimitiveValue::CSS_VARIABLE_NAME;
#endif
  }
  | unary_operator unary_term { $$ = $2; $$.fValue *= $1; }
  ;

calc_func_operator:
    '+' WHITESPACE {
        $$ = '+';
    }
    | '-' WHITESPACE {
        $$ = '-';
    }
    | '*' maybe_space {
        $$ = '*';
    }
    | '/' maybe_space {
        $$ = '/';
    }
  ;

calc_func_paren_expr:
    '(' maybe_space calc_func_expr maybe_space ')' maybe_space {
        if ($3) {
            $$ = $3;
            CSSParserValue v;
            v.id = 0;
            v.unit = CSSParserValue::Operator;
            v.iValue = '(';
            $$->insertValueAt(0, v);
            v.iValue = ')';
            $$->addValue(v);
        } else
            $$ = 0;
    }

calc_func_expr:
    calc_func_term maybe_space {
        $$ = parser->createFloatingValueList();
        $$->addValue(parser->sinkFloatingValue($1));
    }
    | calc_func_expr calc_func_operator calc_func_term {
        if ($1 && $2) {
            $$ = $1;
            CSSParserValue v;
            v.id = 0;
            v.unit = CSSParserValue::Operator;
            v.iValue = $2;
            $$->addValue(v);
            $$->addValue(parser->sinkFloatingValue($3));
        } else
            $$ = 0;

    }
    | calc_func_expr calc_func_operator calc_func_paren_expr {
        if ($1 && $2 && $3) {
            $$ = $1;
            CSSParserValue v;
            v.id = 0;
            v.unit = CSSParserValue::Operator;
            v.iValue = $2;
            $$->addValue(v);
            $$->extend(*($3));
        } else 
            $$ = 0;
    }
    | calc_func_paren_expr
    | calc_func_expr error {
        $$ = 0;
    }
  ;

calc_func_expr_list:
    calc_func_expr  {
        $$ = $1;
    }    
    | calc_func_expr_list ',' maybe_space calc_func_expr {
        if ($1 && $4) {
            $$ = $1;
            CSSParserValue v;
            v.id = 0;
            v.unit = CSSParserValue::Operator;
            v.iValue = ',';
            $$->addValue(v);
            $$->extend(*($4));
        } else
            $$ = 0;
    }
    

calc_function:
    CALCFUNCTION maybe_space calc_func_expr ')' maybe_space {
        CSSParserFunction* f = parser->createFloatingFunction();
        f->name = $1;
        f->args = parser->sinkFloatingValueList($3);
        $$.id = 0;
        $$.unit = CSSParserValue::Function;
        $$.function = f;
    }
    | CALCFUNCTION maybe_space error {
        YYERROR;
    }
    ;


min_or_max:
    MINFUNCTION {
        $$ = $1;
    }
    | MAXFUNCTION {
        $$ = $1;
    }
    ;

min_or_max_function:
    min_or_max maybe_space calc_func_expr_list ')' maybe_space {
        CSSParserFunction* f = parser->createFloatingFunction();
        f->name = $1;
        f->args = parser->sinkFloatingValueList($3);
        $$.id = 0;
        $$.unit = CSSParserValue::Function;
        $$.function = f;
    } 
    | min_or_max maybe_space error {
        YYERROR;
    }
    ;

/* error handling rules */

save_block:
    closing_brace {
        $$ = 0;
    }
  | error closing_brace {
        $$ = 0;
    }
    ;

invalid_at:
    ATKEYWORD error invalid_block {
        $$ = 0;
    }
  | ATKEYWORD error ';' {
        $$ = 0;
    }
    ;

invalid_rule:
    error invalid_block {
        $$ = 0;
    }

/*
  Seems like the two rules below are trying too much and violating
  http://www.hixie.ch/tests/evil/mixed/csserrorhandling.html

  | error ';' {
        $$ = 0;
    }
  | error '}' {
        $$ = 0;
    }
*/
    ;

invalid_block:
    '{' error invalid_block_list error closing_brace {
        parser->invalidBlockHit();
    }
  | '{' error closing_brace {
        parser->invalidBlockHit();
    }
    ;

invalid_block_list:
    invalid_block
  | invalid_block_list error invalid_block
;

%%
