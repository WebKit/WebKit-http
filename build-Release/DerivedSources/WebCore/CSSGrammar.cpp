/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         cssyyparse
#define yylex           cssyylex
#define yyerror         cssyyerror
#define yydebug         cssyydebug
#define yynerrs         cssyynerrs


/* Copy the first part of user declarations.  */
#line 1 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:339  */


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
#include "CSSKeyframeRule.h"
#include "CSSKeyframesRule.h"
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

#if YYDEBUG > 0
#include <wtf/text/CString.h>
#define YYPRINT(File,Type,Value) if (isCSSTokenAString(Type)) YYFPRINTF(File, "%s", String((Value).string).utf8().data())
#endif


#line 134 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* In a future release of Bison, this section will be replaced
   by #include "CSSGrammar.hpp".  */
#ifndef YY_CSSYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_622E996FC528212BD0453A076571261D28328D34_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_CSSGRAMMAR_HPP_INCLUDED
# define YY_CSSYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_622E996FC528212BD0453A076571261D28328D34_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_CSSGRAMMAR_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int cssyydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TOKEN_EOF = 0,
    LOWEST_PREC = 258,
    UNIMPORTANT_TOK = 259,
    WHITESPACE = 260,
    SGML_CD = 261,
    INCLUDES = 262,
    DASHMATCH = 263,
    BEGINSWITH = 264,
    ENDSWITH = 265,
    CONTAINS = 266,
    STRING = 267,
    IDENT = 268,
    NTH = 269,
    NTHCHILDSELECTORSEPARATOR = 270,
    HEX = 271,
    IDSEL = 272,
    IMPORT_SYM = 273,
    PAGE_SYM = 274,
    MEDIA_SYM = 275,
    FONT_FACE_SYM = 276,
    CHARSET_SYM = 277,
    KEYFRAME_RULE_SYM = 278,
    KEYFRAMES_SYM = 279,
    NAMESPACE_SYM = 280,
    WEBKIT_RULE_SYM = 281,
    WEBKIT_DECLS_SYM = 282,
    WEBKIT_VALUE_SYM = 283,
    WEBKIT_MEDIAQUERY_SYM = 284,
    WEBKIT_SIZESATTR_SYM = 285,
    WEBKIT_SELECTOR_SYM = 286,
    WEBKIT_REGION_RULE_SYM = 287,
    WEBKIT_VIEWPORT_RULE_SYM = 288,
    TOPLEFTCORNER_SYM = 289,
    TOPLEFT_SYM = 290,
    TOPCENTER_SYM = 291,
    TOPRIGHT_SYM = 292,
    TOPRIGHTCORNER_SYM = 293,
    BOTTOMLEFTCORNER_SYM = 294,
    BOTTOMLEFT_SYM = 295,
    BOTTOMCENTER_SYM = 296,
    BOTTOMRIGHT_SYM = 297,
    BOTTOMRIGHTCORNER_SYM = 298,
    LEFTTOP_SYM = 299,
    LEFTMIDDLE_SYM = 300,
    LEFTBOTTOM_SYM = 301,
    RIGHTTOP_SYM = 302,
    RIGHTMIDDLE_SYM = 303,
    RIGHTBOTTOM_SYM = 304,
    ATKEYWORD = 305,
    IMPORTANT_SYM = 306,
    MEDIA_ONLY = 307,
    MEDIA_NOT = 308,
    MEDIA_AND = 309,
    REMS = 310,
    CHS = 311,
    QEMS = 312,
    EMS = 313,
    EXS = 314,
    PXS = 315,
    CMS = 316,
    MMS = 317,
    INS = 318,
    PTS = 319,
    PCS = 320,
    DEGS = 321,
    RADS = 322,
    GRADS = 323,
    TURNS = 324,
    MSECS = 325,
    SECS = 326,
    HERTZ = 327,
    KHERTZ = 328,
    DIMEN = 329,
    INVALIDDIMEN = 330,
    PERCENTAGE = 331,
    FLOATTOKEN = 332,
    INTEGER = 333,
    VW = 334,
    VH = 335,
    VMIN = 336,
    VMAX = 337,
    DPPX = 338,
    DPI = 339,
    DPCM = 340,
    FR = 341,
    URI = 342,
    FUNCTION = 343,
    ANYFUNCTION = 344,
    NOTFUNCTION = 345,
    CALCFUNCTION = 346,
    MATCHESFUNCTION = 347,
    MAXFUNCTION = 348,
    MINFUNCTION = 349,
    NTHCHILDFUNCTIONS = 350,
    LANGFUNCTION = 351,
    VARFUNCTION = 352,
    DIRFUNCTION = 353,
    ROLEFUNCTION = 354,
    CUSTOM_PROPERTY = 355,
    UNICODERANGE = 356,
    SUPPORTS_AND = 357,
    SUPPORTS_NOT = 358,
    SUPPORTS_OR = 359,
    SUPPORTS_SYM = 360,
    WEBKIT_SUPPORTS_CONDITION_SYM = 361,
    CUEFUNCTION = 362,
    SLOTTEDFUNCTION = 363,
    HOSTFUNCTION = 364
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 65 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:355  */

    double number;
    CSSParserString string;
    CSSSelector::MarginBoxType marginBox;
    CSSParserValue value;
    CSSParserSelectorCombinator relation;
    StyleRuleBase* rule;
    Vector<RefPtr<StyleRuleBase>>* ruleList;
    MediaQuerySet* mediaList;
    MediaQuery* mediaQuery;
    MediaQuery::Restrictor mediaQueryRestrictor;
    MediaQueryExpression* mediaQueryExpression;
    Vector<CSSParser::SourceSize>* sourceSizeList;
    Vector<MediaQueryExpression>* mediaQueryExpressionList;
    StyleKeyframe* keyframe;
    Vector<RefPtr<StyleKeyframe>>* keyframeRuleList;
    CSSPropertyID id;
    CSSParserSelector* selector;
    Vector<std::unique_ptr<CSSParserSelector>>* selectorList;
    bool boolean;
    CSSSelector::Match match;
    int integer;
    char character;
    CSSParserValueList* valueList;
    Vector<CSSParserString>* stringList;
    CSSParser::Location location;

#line 313 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int cssyyparse (CSSParser* parser);

#endif /* !YY_CSSYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_622E996FC528212BD0453A076571261D28328D34_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_CSSGRAMMAR_HPP_INCLUDED  */

/* Copy the second part of user declarations.  */
#line 92 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:358  */

static inline int cssyyerror(void*, const char*)
{
    return 1;
}
static inline CSSParserValue makeIdentValue(CSSParserString string)
{
    CSSParserValue v;
    v.id = cssValueKeywordID(string);
    v.unit = CSSPrimitiveValue::CSS_IDENT;
    v.string = string;
    return v;
}
static bool selectorListDoesNotMatchAnyPseudoElement(const Vector<std::unique_ptr<CSSParserSelector>>* selectorVector)
{
    if (!selectorVector)
        return true;
    for (unsigned i = 0; i < selectorVector->size(); ++i) {
        for (const CSSParserSelector* selector = selectorVector->at(i).get(); selector; selector = selector->tagHistory()) {
            if (selector->matchesPseudoElement())
                return false;
        }
    }
    return true;
}

#line 355 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  27
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2144

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  131
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  146
/* YYNRULES -- Number of rules.  */
#define YYNRULES  417
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  788

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   364

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   130,     2,   128,     2,   129,     2,     2,
     121,   117,    21,   122,   120,   125,    19,   127,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    18,   119,
       2,   126,   124,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    20,     2,   118,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   115,    22,   116,   123,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    97,    98,    99,
     100,   101,   102,   103,   104,   105,   106,   107,   108,   109,
     110,   111,   112,   113,   114
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   289,   289,   290,   291,   292,   293,   294,   295,   296,
     297,   299,   300,   301,   303,   313,   315,   323,   324,   324,
     325,   325,   326,   326,   326,   327,   327,   328,   328,   329,
     329,   330,   330,   332,   338,   339,   341,   341,   342,   343,
     351,   352,   353,   354,   355,   356,   357,   358,   359,   362,
     366,   367,   368,   371,   372,   382,   383,   393,   394,   395,
     396,   397,   398,   400,   400,   400,   400,   400,   400,   402,
     408,   413,   416,   419,   425,   429,   435,   436,   437,   438,
     440,   440,   441,   441,   442,   442,   444,   449,   455,   463,
     466,   467,   467,   468,   475,   484,   489,   496,   499,   504,
     507,   510,   515,   520,   526,   526,   528,   534,   542,   549,
     554,   559,   564,   567,   570,   576,   579,   586,   589,   594,
     600,   605,   605,   605,   605,   606,   608,   609,   612,   613,
     616,   617,   618,   621,   636,   653,   658,   662,   662,   664,
     665,   671,   673,   677,   684,   690,   703,   708,   713,   723,
     727,   733,   737,   744,   749,   754,   754,   756,   756,   763,
     766,   769,   772,   775,   778,   781,   784,   787,   790,   793,
     796,   799,   802,   805,   808,   813,   818,   821,   825,   831,
     836,   847,   848,   849,   850,   852,   852,   853,   853,   854,
     856,   861,   862,   864,   869,   871,   879,   889,   894,   895,
     897,   901,   901,   903,   907,   912,   918,   920,   921,   922,
     931,   940,   946,   950,   951,   954,   957,   964,   969,   972,
     977,   984,   991,  1000,  1006,  1009,  1015,  1016,  1023,  1029,
    1036,  1047,  1048,  1049,  1052,  1061,  1066,  1073,  1078,  1087,
    1093,  1097,  1100,  1103,  1106,  1109,  1112,  1116,  1116,  1118,
    1122,  1125,  1128,  1136,  1139,  1142,  1145,  1148,  1151,  1162,
    1173,  1184,  1193,  1202,  1217,  1232,  1247,  1256,  1265,  1282,
    1298,  1299,  1300,  1301,  1302,  1303,  1306,  1310,  1314,  1320,
    1326,  1331,  1348,  1364,  1365,  1370,  1373,  1378,  1379,  1380,
    1380,  1382,  1386,  1392,  1395,  1398,  1405,  1417,  1419,  1419,
    1420,  1420,  1422,  1426,  1442,  1443,  1443,  1443,  1445,  1446,
    1447,  1448,  1449,  1450,  1451,  1452,  1453,  1454,  1455,  1456,
    1457,  1458,  1459,  1462,  1465,  1466,  1467,  1468,  1469,  1470,
    1471,  1472,  1473,  1474,  1475,  1476,  1477,  1478,  1479,  1480,
    1481,  1482,  1483,  1484,  1485,  1492,  1493,  1494,  1495,  1496,
    1497,  1498,  1499,  1500,  1503,  1511,  1519,  1529,  1537,  1545,
    1553,  1558,  1563,  1568,  1574,  1581,  1581,  1583,  1584,  1585,
    1588,  1591,  1594,  1597,  1601,  1601,  1603,  1617,  1617,  1619,
    1623,  1638,  1652,  1655,  1656,  1672,  1680,  1686,  1686,  1688,
    1696,  1702,  1702,  1703,  1703,  1704,  1705,  1706,  1707,  1709,
    1709,  1709,  1709,  1709,  1709,  1709,  1709,  1709,  1709,  1710,
    1710,  1711,  1713,  1714,  1715,  1716,  1717,  1718
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "TOKEN_EOF", "error", "$undefined", "LOWEST_PREC", "UNIMPORTANT_TOK",
  "WHITESPACE", "SGML_CD", "INCLUDES", "DASHMATCH", "BEGINSWITH",
  "ENDSWITH", "CONTAINS", "STRING", "IDENT", "NTH",
  "NTHCHILDSELECTORSEPARATOR", "HEX", "IDSEL", "':'", "'.'", "'['", "'*'",
  "'|'", "IMPORT_SYM", "PAGE_SYM", "MEDIA_SYM", "FONT_FACE_SYM",
  "CHARSET_SYM", "KEYFRAME_RULE_SYM", "KEYFRAMES_SYM", "NAMESPACE_SYM",
  "WEBKIT_RULE_SYM", "WEBKIT_DECLS_SYM", "WEBKIT_VALUE_SYM",
  "WEBKIT_MEDIAQUERY_SYM", "WEBKIT_SIZESATTR_SYM", "WEBKIT_SELECTOR_SYM",
  "WEBKIT_REGION_RULE_SYM", "WEBKIT_VIEWPORT_RULE_SYM",
  "TOPLEFTCORNER_SYM", "TOPLEFT_SYM", "TOPCENTER_SYM", "TOPRIGHT_SYM",
  "TOPRIGHTCORNER_SYM", "BOTTOMLEFTCORNER_SYM", "BOTTOMLEFT_SYM",
  "BOTTOMCENTER_SYM", "BOTTOMRIGHT_SYM", "BOTTOMRIGHTCORNER_SYM",
  "LEFTTOP_SYM", "LEFTMIDDLE_SYM", "LEFTBOTTOM_SYM", "RIGHTTOP_SYM",
  "RIGHTMIDDLE_SYM", "RIGHTBOTTOM_SYM", "ATKEYWORD", "IMPORTANT_SYM",
  "MEDIA_ONLY", "MEDIA_NOT", "MEDIA_AND", "REMS", "CHS", "QEMS", "EMS",
  "EXS", "PXS", "CMS", "MMS", "INS", "PTS", "PCS", "DEGS", "RADS", "GRADS",
  "TURNS", "MSECS", "SECS", "HERTZ", "KHERTZ", "DIMEN", "INVALIDDIMEN",
  "PERCENTAGE", "FLOATTOKEN", "INTEGER", "VW", "VH", "VMIN", "VMAX",
  "DPPX", "DPI", "DPCM", "FR", "URI", "FUNCTION", "ANYFUNCTION",
  "NOTFUNCTION", "CALCFUNCTION", "MATCHESFUNCTION", "MAXFUNCTION",
  "MINFUNCTION", "NTHCHILDFUNCTIONS", "LANGFUNCTION", "VARFUNCTION",
  "DIRFUNCTION", "ROLEFUNCTION", "CUSTOM_PROPERTY", "UNICODERANGE",
  "SUPPORTS_AND", "SUPPORTS_NOT", "SUPPORTS_OR", "SUPPORTS_SYM",
  "WEBKIT_SUPPORTS_CONDITION_SYM", "CUEFUNCTION", "SLOTTEDFUNCTION",
  "HOSTFUNCTION", "'{'", "'}'", "')'", "']'", "';'", "','", "'('", "'+'",
  "'~'", "'>'", "'-'", "'='", "'/'", "'#'", "'%'", "'!'", "$accept",
  "stylesheet", "webkit_rule", "webkit_keyframe_rule", "webkit_decls",
  "webkit_value", "webkit_mediaquery", "webkit_selector",
  "webkit_supports_condition", "space", "maybe_space", "maybe_sgml",
  "maybe_charset", "closing_brace", "closing_parenthesis",
  "closing_bracket", "charset", "ignored_charset", "rule_list",
  "valid_rule", "rule", "block_rule_list", "block_valid_rule_list",
  "block_valid_rule", "block_rule", "at_import_header_end_maybe_space",
  "before_import_rule", "import", "namespace", "maybe_ns_prefix",
  "string_or_uri", "maybe_media_value", "webkit_source_size_list",
  "source_size_list", "maybe_source_media_query_expression",
  "source_size_length", "base_media_query_expression",
  "media_query_expression", "media_query_expression_list",
  "maybe_and_media_query_expression_list", "maybe_media_restrictor",
  "media_query", "maybe_media_list", "media_list", "at_rule_body_start",
  "before_media_rule", "at_rule_header_end_maybe_space", "media",
  "supports", "supports_error", "before_supports_rule",
  "at_supports_rule_header_end", "supports_condition", "supports_negation",
  "supports_conjunction", "supports_disjunction",
  "supports_condition_in_parens", "supports_declaration_condition",
  "before_keyframes_rule", "keyframes", "keyframe_name", "keyframes_rule",
  "keyframe_rule", "key_list", "key", "before_page_rule", "page",
  "page_selector", "declarations_and_margins", "margin_box", "$@1",
  "margin_sym", "before_font_face_rule", "font_face", "before_region_rule",
  "region", "combinator", "maybe_unary_operator", "unary_operator",
  "maybe_space_before_declaration", "before_selector_list",
  "at_rule_header_end", "at_selector_end", "ruleset",
  "before_selector_group_item", "selector_list",
  "before_nested_selector_list", "after_nested_selector_list",
  "nested_selector_list", "lang_range", "comma_separated_lang_ranges",
  "complex_selector_with_trailing_whitespace", "complex_selector",
  "namespace_selector", "compound_selector", "simple_selector_list",
  "element_name", "specifier_list", "specifier", "class", "attrib",
  "attrib_flags", "match", "ident_or_string", "pseudo_page",
  "nth_selector_ending", "pseudo", "declaration_list", "decl_list",
  "decl_list_recovery", "declaration", "declaration_recovery", "property",
  "priority", "ident_list", "track_names_list", "whitespace_or_expr",
  "maybe_expr", "expr", "valid_expr", "expr_recovery", "operator", "term",
  "unary_term", "function", "variable_function", "invalid_var_fallback",
  "calc_func_term", "calc_func_operator", "calc_maybe_space",
  "calc_func_paren_expr", "calc_func_expr", "valid_calc_func_expr",
  "calc_func_expr_list", "calc_function", "min_or_max",
  "min_or_max_function", "save_block", "invalid_at", "invalid_rule",
  "invalid_block", "invalid_square_brackets_block",
  "invalid_parentheses_block", "opening_parenthesis", "error_location",
  "error_recovery", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,    58,    46,
      91,    42,   124,   273,   274,   275,   276,   277,   278,   279,
     280,   281,   282,   283,   284,   285,   286,   287,   288,   289,
     290,   291,   292,   293,   294,   295,   296,   297,   298,   299,
     300,   301,   302,   303,   304,   305,   306,   307,   308,   309,
     310,   311,   312,   313,   314,   315,   316,   317,   318,   319,
     320,   321,   322,   323,   324,   325,   326,   327,   328,   329,
     330,   331,   332,   333,   334,   335,   336,   337,   338,   339,
     340,   341,   342,   343,   344,   345,   346,   347,   348,   349,
     350,   351,   352,   353,   354,   355,   356,   357,   358,   359,
     360,   361,   362,   363,   364,   123,   125,    41,    93,    59,
      44,    40,    43,   126,    62,    45,    61,    47,    35,    37,
      33
};
# endif

#define YYPACT_NINF -631

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-631)))

#define YYTABLE_NINF -378

#define yytable_value_is_error(Yytable_value) \
  (!!((Yytable_value) == (-378)))

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     903,   -42,   -40,   -12,   154,   100,   365,   279,   403,   413,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,    62,  -631,  -631,
    -631,  -631,  -631,  -631,   320,  -631,  -631,  -631,   456,   456,
     456,   456,   456,   456,   456,  -631,   680,  -631,  -631,   456,
     171,   958,   456,   664,  1241,   354,  -631,  -631,  1984,  -631,
    1086,   347,   -13,   661,   383,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,   395,  -631,   690,  -631,   441,  -631,  -631,   509,
    -631,  -631,   399,   549,  -631,   570,  -631,   610,  -631,   615,
    -631,  1472,  -631,  -631,  -631,  -631,  -631,   541,  1147,   544,
     557,   579,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  2021,  -631,   584,   827,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
      61,  -631,   603,   642,   410,  -631,  -631,  -631,   456,   665,
    -631,  -631,  1257,   708,  -631,   700,  -631,   263,  1472,   241,
    1643,  -631,   842,   466,  -631,  -631,  -631,  -631,  -631,    50,
     608,  -631,   626,   627,   235,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  1888,   412,   381,  -631,   571,   662,   503,   829,
    -631,    77,  -631,   692,   536,  -631,   234,  -631,   456,   599,
     149,  -631,   625,   632,  -631,  -631,  -631,  -631,  -631,   456,
     456,   456,   125,   456,   456,   957,  1337,  1053,   456,   456,
     456,  -631,  -631,   456,  -631,  -631,  -631,  -631,  -631,  1660,
     456,   456,   456,   456,  1337,   456,    63,   310,  -631,  -631,
    -631,  -631,  -631,  -631,   586,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,   713,  -631,  -631,  -631,   618,
    -631,  -631,  -631,  -631,  -631,   622,  1472,  -631,  -631,   842,
     526,   591,  -631,  -631,    69,   635,   311,  -631,  -631,  -631,
    -631,  -631,   519,   131,   645,  -631,   764,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,   547,  -631,
     652,   456,   547,   453,   655,   658,   347,  -631,   620,   716,
     580,   723,   456,   670,  1086,   618,  1429,  -631,   761,  -631,
    -631,   456,   456,  -631,  1149,  -631,  -631,  -631,  -631,   312,
      38,  -631,  -631,  -631,    52,    52,  -631,  2053,    52,  -631,
    -631,  -631,  -631,   771,   317,  -631,    52,    52,   456,   456,
    -631,   456,   456,  -631,    52,   771,   177,  -631,  -631,  -631,
     531,   119,  -631,  -631,  -631,   105,  1086,   456,   456,   321,
     704,   683,   699,  1086,   665,   700,   773,  -631,   456,   456,
    -631,   456,  -631,   973,  -631,  -631,  -631,   705,   803,    69,
      69,    69,    69,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,   328,   735,   383,
    1431,   171,   456,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
     336,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,   806,   823,  -631,  -631,  -631,  -631,  -631,   350,   722,
    1660,  1241,   789,   456,   456,  1464,  -631,   789,   456,  -631,
    -631,  -631,  -631,  -631,  1564,  -631,  -631,   176,   228,  1918,
      48,  -631,   343,  -631,  -631,  1598,  -631,   141,  -631,  -631,
     354,    72,   456,  -631,   731,  1984,  1086,  1086,  -631,  -631,
     768,  -631,   182,  1472,  -631,  -631,    49,    49,   770,  -631,
    -631,  -631,   361,  -631,  -631,  -631,   546,  -631,   456,   456,
     651,  -631,  -631,  -631,  -631,  -631,  -631,   660,  1499,  -631,
    -631,  -631,  -631,   163,  -631,   472,   437,  -631,  -631,   748,
    -631,   749,   750,  -631,  -631,   769,  -631,   792,  -631,  -631,
    -631,  -631,  -631,  -631,   959,   456,   771,  -631,   880,   880,
    -631,  -631,  -631,  -631,  -631,    52,  -631,  1564,  -631,  -631,
    -631,  -631,  1241,  -631,  -631,   182,  -631,   109,   243,  -631,
    -631,   386,   380,   316,   401,  -631,  -631,   545,  -631,  -631,
      49,  -631,   444,   484,   485,   527,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,   619,  1472,   761,   456,  1241,  -631,
    -631,   134,  -631,  -631,  -631,   372,   280,   354,  -631,   456,
    -631,  -631,  -631,   479,  -631,  -631,   456,  -631,    48,   917,
     917,   456,   456,   727,  -631,   771,   119,   354,  -631,   457,
     561,  -631,  -631,   562,  -631,  -631,  -631,  -631,  -631,   880,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,   736,  -631,  -631,
     305,   789,   789,  -631,  -631,  -631,  -631,  -631,  -631,  1817,
    -631,  -631,  -631,   163,  -631,   479,  -631,  -631,  -631,  -631,
      52,    52,  -631,   141,    61,   456,  -631,  -631,  -631,  1086,
     917,   704,  -631,  -631,  -631,   736,   807,   810,   456,   277,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,   456,   456,  1243,  -631,   456,
     163,  -631,  -631,   112,  -631,  -631,  -631,   718,  -631,  -631,
    -631,  1817,  -631,   383,  1817,   160,  1335,  -631,  1861,  -631,
    -631,   566,  -631,   812,   718,   456,   456,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,
     163,  -631,  -631,  -631,   456,  -631,   814,   456,  1760,  -631,
     383,  -631,  -631,   543,  -631,  1431,   163,  -631
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint16 yydefact[] =
{
      20,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      20,    20,    20,    20,    20,    20,    20,    25,    20,    20,
      20,    20,    20,    20,    89,    20,    20,     1,     3,     9,
       4,     5,     6,     8,    10,    21,     0,    22,    26,     7,
       0,   190,   189,     0,     0,    99,    20,    20,     0,    20,
       0,     0,     0,     0,    38,   146,   145,   188,   187,    20,
      20,   142,     0,   185,     0,    20,     0,    46,    45,     0,
      41,    48,     0,     0,    44,     0,    42,     0,    43,     0,
      47,     0,    40,   412,    20,    20,    20,     0,     0,   274,
     271,     0,    20,    20,    20,    20,   344,   345,   342,   341,
     343,   327,   328,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   339,   340,    20,   326,   325,   324,   346,
     347,   348,   349,   350,   351,   352,   353,    20,    20,    20,
     388,   387,    20,    20,    20,    20,     0,    20,     0,     0,
     302,    20,    20,    20,    20,    20,    20,   100,   101,    95,
     102,    20,     0,     0,     0,    87,    91,    92,    90,   224,
     230,   229,     0,     0,    20,   225,   212,     0,   208,     0,
       0,   207,   215,     0,   226,   231,   232,   233,    20,     0,
       0,   122,   123,   124,   121,   131,   413,    35,    34,    20,
      24,    23,     0,     0,     0,   144,     0,    80,     0,     0,
      20,     0,    20,     0,     0,    20,     0,   413,   288,     0,
       0,    13,   275,   272,    20,    20,   412,    20,   283,   310,
     311,   316,     0,   312,   314,     0,     0,     0,   315,   317,
     322,    20,    20,   323,    14,   412,    20,    20,   301,     0,
     308,   318,   320,   319,     0,   321,     0,     0,    15,    20,
      86,    20,   214,   253,     0,    20,    20,    20,    20,    20,
      20,    20,    20,    20,   234,     0,   213,   197,    16,     0,
     209,   211,   206,    20,    20,    20,     0,   224,   225,   218,
       0,     0,   228,   227,     0,     0,     0,    17,    20,    20,
      20,    20,     0,     0,     0,    20,     0,    50,    49,    22,
      52,    51,    12,    20,    20,    79,    78,    20,     0,    11,
       0,    69,     0,    99,     0,     0,     0,   116,     0,     0,
     154,     0,   111,     0,     0,   191,     0,   285,    20,    20,
      20,   277,   276,   413,     0,    32,    20,    31,   293,     0,
       0,    30,    29,   355,     0,     0,    20,     0,     0,   367,
     369,   379,   382,   374,     0,    20,     0,     0,   313,   309,
     413,   306,   305,   303,     0,   374,     0,    20,    20,    20,
      84,    89,   254,    20,    20,   186,     0,   198,   198,   186,
       0,     0,     0,     0,    20,     0,     0,    20,   181,   182,
      20,   183,   210,     0,   125,   132,    20,     0,     0,     0,
       0,     0,     0,    28,   414,   413,   400,   406,   407,   402,
     403,   404,   405,   408,   401,   409,   410,   411,    27,   399,
     396,   415,   416,   417,   413,    33,   395,     0,     0,    39,
       0,     0,    81,    82,    83,    20,    74,    75,    20,   106,
       0,   109,   114,   117,   118,   120,   138,   137,    20,   150,
     149,   151,     0,    20,   153,   178,   177,   109,     0,     0,
     298,     0,   290,   279,   278,     0,   286,   290,   291,    20,
     294,   295,   354,   356,     0,   368,   386,   375,     0,     0,
       0,   378,     0,   363,   364,   304,   390,   383,    20,   389,
      99,    97,    94,    20,     0,     0,     0,     0,    20,    20,
       0,   221,     0,     0,    20,    20,     0,     0,     0,   202,
     201,   203,     0,    20,    20,    20,     0,    20,   194,   184,
       0,    20,    20,   127,   129,   126,   128,     0,     0,    20,
      37,   394,   393,     0,   143,     0,   104,   108,    20,     0,
      20,     0,     0,   152,   249,     0,    20,     0,   109,   296,
     299,   297,    20,   281,     0,   292,   374,    19,     0,     0,
     380,   381,    20,    20,   385,    20,   357,     0,    96,    20,
      20,   103,     0,    93,    88,     0,    20,     0,     0,    20,
     223,     0,     0,     0,     0,    18,   250,     0,   265,   263,
       0,   205,     0,     0,     0,     0,   242,   243,   244,   245,
     246,   235,   241,    20,     0,     0,    20,   130,     0,   397,
     398,     0,   141,    76,    77,     0,     0,    99,   109,    53,
     109,   109,   109,     0,   109,    20,   289,   284,     0,   370,
     371,   372,   373,     0,   360,   374,     0,    99,    20,     0,
       0,   268,   266,     0,   258,    20,   200,   269,   259,     0,
     251,   264,   260,    20,   261,   262,   257,     0,   237,    20,
       0,   290,   290,    36,    72,    71,    73,   107,    20,     0,
      20,    20,    20,     0,    20,     0,   376,   366,   365,   359,
       0,     0,    20,   384,    98,    85,   255,   256,   267,     0,
     198,     0,   248,   247,    20,     0,     0,     0,    53,     0,
     391,    63,    22,    67,    66,    60,    62,    61,    58,    59,
      68,    57,   113,    65,    64,    53,   139,     0,   176,    55,
       0,   362,   358,     0,   222,    20,   204,   240,    20,    20,
      20,     0,   392,    54,     0,     0,     0,   155,     0,   193,
     361,     0,    20,     0,   240,   134,   133,   112,   115,   136,
      20,   159,   160,   161,   162,   163,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   148,    20,   157,
       0,    22,   180,   252,   239,   236,     0,   140,     0,    20,
      56,   238,   156,     0,    20,     0,     0,   158
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -631,  -410,
       0,  -297,  -631,  -263,  -257,  -299,  -631,  -631,  -631,   753,
    -631,   -61,  -631,   190,  -631,  -631,  -631,   -33,   -24,  -631,
     628,  -631,  -631,  -631,   577,   446,  -238,   452,   313,  -631,
    -455,   -44,  -631,   415,  -427,  -631,  -170,    -4,    15,  -631,
    -631,  -631,  -233,  -631,  -631,  -631,     2,  -631,  -631,    17,
    -631,  -631,   218,  -631,   530,  -631,    20,  -631,  -631,  -631,
    -631,  -631,  -631,    21,  -631,    29,  -631,   507,   -34,  -502,
    -631,  -368,   758,    30,  -631,   -77,  -631,  -631,  -371,   274,
    -631,  -631,   362,   701,  -165,   475,   798,   237,  -128,  -631,
    -631,   231,   368,   281,   528,  -394,  -631,  -403,  -631,   892,
     897,  -177,   710,  -417,  -631,  -631,   388,  -631,  -194,  -631,
    -146,  -631,   747,   -20,  -631,  -187,  -631,   518,  -631,  -322,
     537,  -220,  -631,  -631,   -43,  -631,  -631,  -630,   845,   859,
     -10,  -631,  -631,  -631,   104,  -186
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     9,    10,    11,    12,    13,    14,    15,    16,   478,
      42,    54,    37,   700,   343,   338,    38,   297,   192,    65,
     299,   669,   738,   701,   702,   312,    66,   703,   704,   308,
     435,   494,    18,    47,    48,   155,    49,   149,   150,   571,
     151,   439,   615,   440,   540,    69,   314,   705,   706,   317,
      72,   541,   180,   181,   182,   183,   184,   185,    73,   707,
     448,   735,    59,    60,    61,    75,   708,   453,   736,   768,
     779,   769,    77,   709,    79,   710,   276,    62,   136,    43,
      81,   459,   269,   711,   605,   167,   503,   646,   504,   511,
     512,   168,   169,   170,   171,   502,   172,   173,   174,   175,
     176,   743,   603,   694,   454,   588,   177,    87,    88,    89,
      90,   218,    91,   553,   339,   137,   462,   549,   138,   139,
     238,   239,   140,   141,   142,   143,   682,   351,   479,   480,
     352,   353,   354,   366,   144,   145,   146,   712,   713,   714,
     421,   422,   423,   424,   207,   292
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      17,   152,   429,   270,   206,   157,    63,   505,    67,   369,
      28,    29,    30,    31,    32,    33,    34,    68,    39,    40,
      41,   326,    44,    45,   365,    50,    51,   533,   156,   420,
     546,   344,   327,   356,   323,   569,    53,    70,   335,   350,
     470,   471,   188,   487,   623,   283,   153,   154,   341,   158,
     554,   285,   341,   397,   585,   -20,    71,   350,    74,   193,
     194,    76,    78,   -20,   197,   198,   -20,    35,    35,   562,
      80,    82,   539,    19,    35,    20,   340,    35,   315,   345,
     348,   357,   -20,   445,   208,   209,   210,   472,   473,    36,
     547,   476,   219,   220,   221,   222,   587,   587,   364,   483,
     484,   747,   186,    21,   748,    23,   187,   486,   772,   489,
      35,   392,   341,   589,    35,   223,   232,    35,   498,   499,
     -20,   625,   367,   675,    35,   335,   235,   224,   225,   226,
      35,   570,   227,   228,   229,   230,    35,   233,   336,    35,
     467,   240,   241,   242,   243,   244,   245,   465,   629,   630,
     246,   247,   283,   283,    35,   -20,   337,   466,   -20,    67,
     403,    55,   562,   403,   265,   342,   586,   328,    68,   342,
     717,   -20,    55,    56,   485,   563,    35,   341,   284,   286,
     587,   -18,   569,   580,    56,   -20,   306,   -20,    70,   293,
     179,   668,   347,   670,   671,   672,   651,   674,   -20,   311,
     313,   316,   318,   320,   322,   324,   349,    71,   481,    74,
     347,   501,    76,    78,   331,   332,   680,   334,   515,   527,
     673,    80,    82,   564,   349,   566,   641,    57,   609,   342,
      58,   358,   359,   557,   628,   267,   361,   362,   528,   690,
      46,  -186,   271,   337,   696,   697,   272,   458,    35,   370,
     425,   371,  -186,   663,   556,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   267,   283,   550,   551,   563,    22,
     612,   610,   720,   388,   389,   391,   418,   403,   542,   418,
    -105,   537,    57,   545,   426,    58,   394,   350,   399,   400,
     401,   402,   350,    57,   342,   427,    58,   488,   -18,   -20,
     437,   -18,   -20,   430,   431,   444,   271,   432,   634,   450,
     272,   456,   335,   683,   737,    35,    35,  -377,   235,   725,
     333,    35,  -377,   368,    84,   469,    35,   475,   461,   463,
     464,   501,   576,    35,   506,   507,   468,   537,  -377,   360,
     529,    63,   290,   341,   291,    63,   474,   635,    35,  -192,
     558,   267,    35,   559,  -192,   482,  -195,  -195,  -195,    35,
     642,  -195,   591,   273,   274,   275,   -20,   490,   491,   492,
      24,   676,   664,   496,   497,   782,   679,   627,   638,   268,
     350,   267,   786,  -192,   516,  -199,    35,   518,   190,   191,
     519,    35,   186,   418,    25,  -105,   520,    63,   369,  -105,
     538,   523,   524,   525,   526,   733,    35,   280,    26,   281,
     718,   147,   148,    27,   662,    35,   396,    35,   532,   178,
    -196,  -196,  -196,   721,   722,  -196,   582,   273,   274,   275,
     337,    46,   179,   647,  -377,   535,   732,  -377,   536,   681,
     347,    46,    35,    57,  -377,   347,    58,   530,   322,    35,
     -99,  -191,   157,   322,   349,   178,   538,   739,    35,   349,
     342,    35,    35,   565,   199,  -191,   740,   282,   179,   555,
    -192,  -217,   749,   767,   780,   156,   195,    35,   -20,  -270,
      83,   -20,   160,   161,   162,   163,   164,   186,   567,    35,
      35,   665,    84,   572,   147,   148,   303,  -199,   577,   578,
    -192,   304,   581,   644,   583,   584,   645,   732,    35,   201,
     147,   148,   592,   593,   594,   595,   393,   604,   648,   403,
     404,   607,   608,   787,   724,   614,   250,   282,   302,   611,
     251,  -220,    35,   347,   200,    85,    35,   321,   617,   405,
     619,   -20,   160,   161,   162,   163,   164,   349,    35,   493,
     557,    35,   626,   596,   597,   598,   599,   600,   -99,   433,
     649,   652,   631,   632,   653,   633,    35,    35,  -111,   636,
     637,    35,  -111,   667,   686,   639,   640,   645,   202,   643,
     216,  -217,  -217,  -217,    86,    35,  -217,   186,  -217,  -217,
    -217,   613,   282,   451,   203,  -270,  -216,   217,   452,   372,
     216,   654,   655,   657,    35,   666,   461,   160,   161,   162,
     163,   164,   406,   407,   408,   409,   410,   411,   412,   309,
     413,   414,   415,   416,    35,    35,   596,   597,   598,   599,
     600,   417,   446,   447,   186,   418,   204,   731,   685,   434,
     419,  -220,  -220,  -220,   656,   689,  -220,    35,  -220,  -220,
    -220,   -20,   205,   691,   734,   249,    35,   211,   784,   695,
     335,   404,   650,   214,   601,    83,    35,    35,   698,   606,
     715,   716,   602,   189,   719,   307,   215,    84,   687,   688,
     405,    52,   723,   773,   246,   -20,   186,   252,    35,   426,
     305,   196,   -20,   319,   727,   -20,   513,   -20,   373,   374,
     234,    63,   -20,   -20,    35,   -20,  -216,  -216,  -216,    35,
     -20,  -216,   514,  -216,  -216,  -216,   509,   510,    35,   248,
      85,   264,   266,    35,   287,   741,   384,   341,   744,   745,
     746,   742,    35,   288,   385,   166,   289,   658,   387,    92,
      93,    35,   774,    94,   329,   602,   390,    95,   692,   693,
     777,   330,   395,   406,   407,   408,   409,   410,   411,   412,
     186,   413,   414,   415,   416,   428,   460,   186,   778,    86,
     441,   436,   417,   186,   442,   186,   477,   443,   337,   783,
    -270,   419,   -20,   552,   785,   457,   517,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   -20,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   522,   521,   129,   452,   130,   131,  -300,   235,   132,
     310,   186,  -300,   133,   -20,   449,   544,   548,   186,  -307,
    -307,   -20,   455,  -307,   342,   552,   677,  -307,   573,    57,
     186,   579,    58,   590,   531,   134,   135,   678,   160,   161,
     162,   163,   164,   618,   620,   621,  -300,  -300,  -300,  -300,
    -300,  -300,  -300,  -300,  -300,  -300,  -300,  -300,  -300,  -300,
    -300,  -300,   500,  -300,   622,   585,   508,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,   624,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,   -20,   557,  -307,   729,  -307,  -307,   730,   771,  -307,
     775,     1,   781,  -307,     2,     3,     4,     5,     6,     7,
     438,   574,   568,  -300,  -300,   298,  -300,   236,   495,  -307,
     684,   616,  -307,   750,   237,  -307,  -307,   341,   235,  -282,
     216,   534,    35,    35,   325,   726,   386,   660,   279,    92,
      93,   575,   659,    94,   282,   776,   728,    95,  -219,   543,
     212,   -70,  -147,  -110,  -175,   213,   363,  -135,    64,   160,
     161,   162,   163,   164,   661,  -179,   398,   560,  -282,  -282,
    -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,  -282,
    -282,  -282,  -282,  -282,     8,     0,   561,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,   115,   300,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
     128,   301,     0,   129,   235,   130,   131,     0,    35,   132,
       0,     0,     0,   133,     0,    92,    93,     0,  -119,    94,
       0,     0,     0,    95,   342,  -282,     0,     0,  -282,    57,
       0,     0,    58,     0,     0,   134,   135,     0,  -219,  -219,
    -219,    35,     0,  -219,     0,  -219,  -219,  -219,     0,   159,
       0,     0,   160,   161,   162,   163,   164,   165,   166,     0,
       0,     0,     0,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,   115,     0,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,  -273,    83,   129,
     216,   130,   131,     0,    35,   132,     0,     0,   355,   133,
      84,    92,    93,     0,     0,    94,     0,     0,     0,    95,
       0,     0,     0,     0,     0,    57,     0,     0,    58,     0,
       0,   134,   135,     0,     0,     0,  -273,  -273,  -273,  -273,
    -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,  -273,
    -273,  -273,     0,    85,     0,     0,     0,     0,     0,    96,
      97,    98,    99,   100,   101,   102,   103,   104,   105,   106,
     107,   108,   109,   110,   111,   112,   113,   114,   115,     0,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,   128,  -270,    83,   129,    35,   130,   131,     0,
       0,   132,    86,    92,    93,   133,    84,    94,     0,     0,
       0,    95,     0,  -273,     0,     0,     0,     0,     0,     0,
     253,    57,     0,     0,    58,   254,     0,   134,   135,     0,
       0,     0,  -270,  -270,  -270,  -270,  -270,  -270,  -270,  -270,
    -270,  -270,  -270,  -270,  -270,  -270,  -270,  -270,     0,    85,
       0,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,     0,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   403,     0,   129,   235,   130,
     131,     0,    35,   132,     0,     0,     0,   133,    86,     0,
     255,   256,   257,     0,   258,     0,     0,   259,   260,  -270,
     261,   262,     0,    57,     0,     0,    58,     0,     0,   134,
     135,   263,     0,     0,   751,   752,   753,   754,   755,   756,
     757,   758,   759,   760,   761,   762,   763,   764,   765,   766,
       0,     0,     0,     0,     0,     0,     0,    96,    97,    98,
      99,   100,   101,   102,   103,   104,   105,   106,   107,   108,
     109,   110,   111,   112,   113,   114,     0,     0,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,  -280,
     404,  -270,    83,     0,     0,     0,    35,     0,     0,   132,
       0,     0,     0,     0,    84,     0,     0,     0,     0,   405,
       0,   418,     0,     0,     0,     0,     0,     0,   346,    57,
       0,     0,    58,     0,  -287,   404,     0,     0,  -280,  -280,
    -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,  -280,
    -280,  -280,  -280,  -280,   405,   159,     0,    85,   160,   161,
     162,   163,   164,   165,   166,     0,     0,     0,     0,   341,
     404,     0,     0,  -287,  -287,  -287,  -287,  -287,  -287,  -287,
    -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,  -287,   405,
       0,     0,   406,   407,   408,   409,   410,   411,   412,     0,
     413,   414,   415,   416,     0,     0,    86,     0,     0,     0,
       0,   417,     0,     0,   186,  -280,     0,  -270,  -280,     0,
     419,     0,     0,     0,     0,     0,     0,   406,   407,   408,
     409,   410,   411,   412,     0,   413,   414,   415,   416,    35,
       0,     0,     0,     0,     0,     0,   417,     0,     0,   186,
    -287,     0,     0,  -287,     0,   419,     0,     0,     0,     0,
       0,     0,   406,   407,   408,   409,   410,   411,   412,  -378,
     413,   414,   415,   416,     0,     0,     0,     0,     0,     0,
       0,   417,     0,     0,   186,     0,   342,     0,   405,     0,
     419,     0,     0,     0,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,     0,     0,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   277,     0,     0,   160,
     161,   162,   163,   164,   278,     0,   132,     0,     0,     0,
       0,     0,    92,    93,     0,     0,    94,     0,     0,     0,
      95,     0,     0,     0,     0,   346,    57,     0,     0,    58,
       0,   406,   407,   408,   409,   410,   411,   412,     0,   413,
     414,   415,   416,     0,     0,     0,     0,     0,     0,     0,
     417,     0,     0,   186,     0,     0,     0,     0,     0,   419,
      96,    97,    98,    99,   100,   101,   102,   103,   104,   105,
     106,   107,   108,   109,   110,   111,   112,   113,   114,   115,
       0,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,     0,     0,   129,     0,   130,   131,
    -270,    83,   132,     0,     0,    35,   133,     0,     0,     0,
       0,     0,     0,    84,     0,     0,     0,     0,     0,     0,
       0,     0,    57,     0,     0,    58,     0,     0,   134,   135,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -270,
    -270,  -270,  -270,  -270,  -270,  -270,  -270,  -270,  -270,  -270,
    -270,  -270,  -270,  -270,  -270,     0,    85,   403,   699,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    -190,     0,     0,  -190,  -190,  -190,  -190,  -190,  -190,  -190,
     -70,  -147,  -110,  -175,     0,     0,  -135,    64,     0,     0,
       0,     0,     0,     0,  -179,     0,     0,     0,     0,     0,
       0,   403,   770,     0,     0,    86,     0,     0,     0,     0,
       0,     0,   296,     0,  -190,     0,  -270,  -190,  -190,  -190,
    -190,  -190,  -190,  -190,     0,  -147,  -110,  -175,    -2,   294,
    -135,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  -190,     0,     0,  -190,  -190,  -190,  -190,  -190,  -190,
    -190,   -70,  -147,  -110,  -175,   295,     0,  -135,    64,     0,
       0,     0,     0,     0,     0,  -179,     0,  -119,     0,     0,
       0,     0,     0,   418,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   296,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  -119,     0,     0,     0,     0,     0,   418,    96,    97,
      98,    99,   100,   101,   102,   103,   104,   105,   106,   107,
     108,   109,   110,   111,   112,   113,   114,     0,  -119,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     132,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   346,
      57,     0,     0,    58,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   107,   108,   109,   110,   111,
     112,   113,   114,     0,     0,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,     0,     0,     0,     0,
     129,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     231,     0,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,   106,   107,   108,   109,   110,   111,   112,
     113,   114,     0,     0,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126
};

static const yytype_int16 yycheck[] =
{
       0,    45,   299,   168,    81,    48,    40,   378,    41,   247,
      10,    11,    12,    13,    14,    15,    16,    41,    18,    19,
      20,   207,    22,    23,   244,    25,    26,   430,    48,   292,
     457,   225,   209,   227,   204,   490,    36,    41,     0,   226,
     339,   340,    52,   365,   546,   173,    46,    47,     0,    49,
     467,     1,     0,   286,     5,     5,    41,   244,    41,    59,
      60,    41,    41,    13,    64,    65,     5,     5,     5,    21,
      41,    41,   440,   115,     5,   115,   222,     5,     1,   225,
     226,   227,     5,   316,    84,    85,    86,   344,   345,    27,
     458,   348,    92,    93,    94,    95,   506,   507,   244,   356,
     357,   731,   115,   115,   734,     5,   119,   364,   738,   366,
       5,   276,     0,   507,     5,   115,   136,     5,    13,    14,
      59,   548,    59,   625,     5,     0,     1,   127,   128,   129,
       5,    59,   132,   133,   134,   135,     5,   137,    13,     5,
     334,   141,   142,   143,   144,   145,   146,   333,   558,   559,
     150,   151,   280,   281,     5,   105,   118,   334,   108,   192,
       0,     1,    21,     0,   164,   117,   117,    18,   192,   117,
     672,   121,     1,    13,   360,   127,     5,     0,   178,   179,
     590,     5,   637,     1,    13,   108,   196,     5,   192,   189,
     121,   618,   226,   620,   621,   622,   590,   624,   121,   199,
     200,   201,   202,   203,   204,   205,   226,   192,   354,   192,
     244,   376,   192,   192,   214,   215,   633,   217,   383,   405,
     623,   192,   192,   480,   244,   482,   117,   122,   527,   117,
     125,   231,   232,     5,   556,     1,   236,   237,   424,   649,
     121,    81,     1,   118,   661,   662,     5,   324,     5,   249,
     119,   251,    81,   119,   474,   255,   256,   257,   258,   259,
     260,   261,   262,   263,     1,   393,   460,   461,   127,   115,
     533,   528,   675,   273,   274,   275,   116,     0,   448,   116,
       0,     1,   122,   453,   294,   125,   284,   474,   288,   289,
     290,   291,   479,   122,   117,   295,   125,   120,   122,   117,
     310,   125,   120,   303,   304,   315,     1,   307,   565,   319,
       5,   321,     0,   635,   717,     5,     5,     0,     1,   690,
     216,     5,     5,    13,    13,    13,     5,   347,   328,   329,
     330,   496,   497,     5,    13,    14,   336,     1,    21,   235,
      12,   375,   107,     0,   109,   379,   346,   567,     5,   115,
     122,     1,     5,   125,   120,   355,   115,   116,   117,     5,
     117,   120,     1,   122,   123,   124,     5,   367,   368,   369,
       5,   628,     0,   373,   374,   778,   633,   554,   572,   116,
     567,     1,   785,   120,   384,     5,     5,   387,     5,     6,
     390,     5,   115,   116,   115,   115,   396,   431,   636,   119,
     120,   399,   400,   401,   402,   702,     5,   170,     5,   172,
     673,    57,    58,     0,   608,     5,   105,     5,   428,   108,
     115,   116,   117,   680,   681,   120,   503,   122,   123,   124,
     118,   121,   121,   117,   117,   435,   699,   120,   438,   633,
     474,   121,     5,   122,   127,   479,   125,   119,   448,     5,
      13,   115,   495,   453,   474,   108,   120,   720,     5,   479,
     117,     5,     5,   120,    23,   115,   723,     1,   121,   469,
     120,     5,   735,   736,   771,   495,    81,     5,   117,     0,
       1,   120,    16,    17,    18,    19,    20,   115,   488,     5,
       5,   119,    13,   493,    57,    58,   115,   117,   498,   499,
     120,   120,   502,   117,   504,   505,   120,   770,     5,   110,
      57,    58,   512,   513,   514,   515,   279,   517,   117,     0,
       1,   521,   522,   786,   689,   535,   116,     1,   116,   529,
     120,     5,     5,   567,    25,    56,     5,     1,   538,    20,
     540,     5,    16,    17,    18,    19,    20,   567,     5,    18,
       5,     5,   552,     7,     8,     9,    10,    11,   121,    12,
      15,   117,   562,   563,   120,   565,     5,     5,   115,   569,
     570,     5,   119,   617,   117,   575,   576,   120,    29,   579,
       1,   115,   116,   117,   105,     5,   120,   115,   122,   123,
     124,   119,     1,    13,    24,   116,     5,    18,    18,    13,
       1,   117,   117,   603,     5,   615,   606,    16,    17,    18,
      19,    20,    93,    94,    95,    96,    97,    98,    99,   116,
     101,   102,   103,   104,     5,     5,     7,     8,     9,    10,
      11,   112,    12,    13,   115,   116,    26,   698,   638,    92,
     121,   115,   116,   117,   117,   645,   120,     5,   122,   123,
     124,   115,    37,   653,   715,    13,     5,   116,   115,   659,
       0,     1,   117,   119,   118,     1,     5,     5,   668,    18,
     670,   671,   126,    12,   674,    13,   119,    13,   117,   117,
      20,     1,   682,   117,   684,     5,   115,    22,     5,   699,
     119,     1,    12,     1,   694,     5,    13,     5,   112,   113,
     116,   735,    12,    13,     5,    13,   115,   116,   117,     5,
      18,   120,    13,   122,   123,   124,    12,    13,     5,   116,
      56,    13,    22,     5,   116,   725,    13,     0,   728,   729,
     730,    13,     5,   107,    21,    22,   109,   118,   120,    12,
      13,     5,   742,    16,   119,   126,   124,    20,    12,    13,
     750,   119,   117,    93,    94,    95,    96,    97,    98,    99,
     115,   101,   102,   103,   104,     1,     5,   115,   768,   105,
     115,   119,   112,   115,   119,   115,     5,   119,   118,   779,
     116,   121,    92,    56,   784,   115,    13,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,   115,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    18,   117,    96,    18,    98,    99,     0,     1,   102,
       1,   115,     5,   106,     5,   119,    13,   115,   115,    12,
      13,    12,   119,    16,   117,    56,   119,    20,   117,   122,
     115,    83,   125,    83,   119,   128,   129,   130,    16,    17,
      18,    19,    20,   115,   115,   115,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,   375,    56,   115,     5,   379,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,   115,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    92,     5,    96,   117,    98,    99,   117,   738,   102,
     118,    28,   118,   106,    31,    32,    33,    34,    35,    36,
     312,   495,   490,   116,   117,   192,   119,   120,   371,   122,
     637,   536,   125,   735,   127,   128,   129,     0,     1,     0,
       1,   431,     5,     5,   206,   691,   265,   605,   170,    12,
      13,   496,   604,    16,     1,   744,   695,    20,     5,   451,
      88,    23,    24,    25,    26,    88,   239,    29,    30,    16,
      17,    18,    19,    20,   606,    37,   286,   479,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,   111,    -1,   479,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,   192,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,   192,    -1,    96,     1,    98,    99,    -1,     5,   102,
      -1,    -1,    -1,   106,    -1,    12,    13,    -1,   110,    16,
      -1,    -1,    -1,    20,   117,   116,    -1,    -1,   119,   122,
      -1,    -1,   125,    -1,    -1,   128,   129,    -1,   115,   116,
     117,     5,    -1,   120,    -1,   122,   123,   124,    -1,    13,
      -1,    -1,    16,    17,    18,    19,    20,    21,    22,    -1,
      -1,    -1,    -1,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    -1,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91,    92,    93,     0,     1,    96,
       1,    98,    99,    -1,     5,   102,    -1,    -1,   105,   106,
      13,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,    20,
      -1,    -1,    -1,    -1,    -1,   122,    -1,    -1,   125,    -1,
      -1,   128,   129,    -1,    -1,    -1,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    -1,    -1,    -1,    -1,    -1,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    -1,
      81,    82,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,     0,     1,    96,     5,    98,    99,    -1,
      -1,   102,   105,    12,    13,   106,    13,    16,    -1,    -1,
      -1,    20,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      13,   122,    -1,    -1,   125,    18,    -1,   128,   129,    -1,
      -1,    -1,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      -1,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    92,    93,     0,    -1,    96,     1,    98,
      99,    -1,     5,   102,    -1,    -1,    -1,   106,   105,    -1,
      93,    94,    95,    -1,    97,    -1,    -1,   100,   101,   116,
     103,   104,    -1,   122,    -1,    -1,   125,    -1,    -1,   128,
     129,   114,    -1,    -1,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    -1,    -1,    81,    82,
      83,    84,    85,    86,    87,    88,    89,    90,    91,     0,
       1,     0,     1,    -1,    -1,    -1,     5,    -1,    -1,   102,
      -1,    -1,    -1,    -1,    13,    -1,    -1,    -1,    -1,    20,
      -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,   121,   122,
      -1,    -1,   125,    -1,     0,     1,    -1,    -1,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    20,    13,    -1,    56,    16,    17,
      18,    19,    20,    21,    22,    -1,    -1,    -1,    -1,     0,
       1,    -1,    -1,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    20,
      -1,    -1,    93,    94,    95,    96,    97,    98,    99,    -1,
     101,   102,   103,   104,    -1,    -1,   105,    -1,    -1,    -1,
      -1,   112,    -1,    -1,   115,   116,    -1,   116,   119,    -1,
     121,    -1,    -1,    -1,    -1,    -1,    -1,    93,    94,    95,
      96,    97,    98,    99,    -1,   101,   102,   103,   104,     5,
      -1,    -1,    -1,    -1,    -1,    -1,   112,    -1,    -1,   115,
     116,    -1,    -1,   119,    -1,   121,    -1,    -1,    -1,    -1,
      -1,    -1,    93,    94,    95,    96,    97,    98,    99,     1,
     101,   102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   112,    -1,    -1,   115,    -1,   117,    -1,    20,    -1,
     121,    -1,    -1,    -1,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    -1,    -1,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    13,    -1,    -1,    16,
      17,    18,    19,    20,    21,    -1,   102,    -1,    -1,    -1,
      -1,    -1,    12,    13,    -1,    -1,    16,    -1,    -1,    -1,
      20,    -1,    -1,    -1,    -1,   121,   122,    -1,    -1,   125,
      -1,    93,    94,    95,    96,    97,    98,    99,    -1,   101,
     102,   103,   104,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     112,    -1,    -1,   115,    -1,    -1,    -1,    -1,    -1,   121,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      -1,    81,    82,    83,    84,    85,    86,    87,    88,    89,
      90,    91,    92,    93,    -1,    -1,    96,    -1,    98,    99,
       0,     1,   102,    -1,    -1,     5,   106,    -1,    -1,    -1,
      -1,    -1,    -1,    13,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   122,    -1,    -1,   125,    -1,    -1,   128,   129,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    -1,    56,     0,     1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      13,    -1,    -1,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    -1,    -1,    29,    30,    -1,    -1,
      -1,    -1,    -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,
      -1,     0,     1,    -1,    -1,   105,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    -1,    13,    -1,   116,    16,    17,    18,
      19,    20,    21,    22,    -1,    24,    25,    26,     0,     1,
      29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    13,    -1,    -1,    16,    17,    18,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    -1,    29,    30,    -1,
      -1,    -1,    -1,    -1,    -1,    37,    -1,   110,    -1,    -1,
      -1,    -1,    -1,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   110,    -1,    -1,    -1,    -1,    -1,   116,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    -1,   110,    81,
      82,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     102,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   121,
     122,    -1,    -1,   125,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    -1,    -1,    81,    82,    83,    84,    85,
      86,    87,    88,    89,    90,    91,    -1,    -1,    -1,    -1,
      96,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    -1,    81,    82,    83,    84,    85,    86,    87,    88,
      89,    90,    91,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    -1,    -1,    81,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    91
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint16 yystos[] =
{
       0,    28,    31,    32,    33,    34,    35,    36,   111,   132,
     133,   134,   135,   136,   137,   138,   139,   141,   163,   115,
     115,   115,   115,     5,     5,   115,     5,     0,   141,   141,
     141,   141,   141,   141,   141,     5,    27,   143,   147,   141,
     141,   141,   141,   210,   141,   141,   121,   164,   165,   167,
     141,   141,     1,   141,   142,     1,    13,   122,   125,   193,
     194,   195,   208,   209,    30,   150,   157,   158,   159,   176,
     178,   179,   181,   189,   190,   196,   197,   203,   204,   205,
     206,   211,   214,     1,    13,    56,   105,   238,   239,   240,
     241,   243,    12,    13,    16,    20,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    96,
      98,    99,   102,   106,   128,   129,   209,   246,   249,   250,
     253,   254,   255,   256,   265,   266,   267,    57,    58,   168,
     169,   171,   172,   141,   141,   166,   254,   265,   141,    13,
      16,    17,    18,    19,    20,    21,    22,   216,   222,   223,
     224,   225,   227,   228,   229,   230,   231,   237,   108,   121,
     183,   184,   185,   186,   187,   188,   115,   119,   271,    12,
       5,     6,   149,   141,   141,    81,     1,   141,   141,    23,
      25,   110,    29,    24,    26,    37,   216,   275,   141,   141,
     141,   116,   240,   241,   119,   119,     1,    18,   242,   141,
     141,   141,   141,   141,   141,   141,   141,   141,   141,   141,
     141,    79,   254,   141,   116,     1,   120,   127,   251,   252,
     141,   141,   141,   141,   141,   141,   141,   141,   116,    13,
     116,   120,    22,    13,    18,    93,    94,    95,    97,   100,
     101,   103,   104,   114,    13,   141,    22,     1,   116,   213,
     225,     1,     5,   122,   123,   124,   207,    13,    21,   227,
     228,   228,     1,   229,   141,     1,   141,   116,   107,   109,
     107,   109,   276,   141,     1,    27,    55,   148,   150,   151,
     269,   270,   116,   115,   120,   119,   271,    13,   160,   116,
       1,   141,   156,   141,   177,     1,   141,   180,   141,     1,
     141,     1,   141,   177,   141,   213,   276,   242,    18,   119,
     119,   141,   141,   275,   141,     0,    13,   118,   146,   245,
     251,     0,   117,   145,   249,   251,   121,   209,   251,   254,
     256,   258,   261,   262,   263,   105,   249,   251,   141,   141,
     275,   141,   141,   253,   251,   262,   264,    59,    13,   167,
     141,   141,    13,   112,   113,   141,   141,   141,   141,   141,
     141,   141,   141,   141,    13,    21,   224,   120,   141,   141,
     124,   141,   225,   228,   187,   117,   105,   183,   243,   141,
     141,   141,   141,     0,     1,    20,    93,    94,    95,    96,
      97,    98,    99,   101,   102,   103,   104,   112,   116,   121,
     144,   271,   272,   273,   274,   119,   271,   141,     1,   142,
     141,   141,   141,    12,    92,   161,   119,   271,   161,   172,
     174,   115,   119,   119,   271,   183,    12,    13,   191,   119,
     271,    13,    18,   198,   235,   119,   271,   115,   216,   212,
       5,   141,   247,   141,   141,   276,   242,   249,   141,    13,
     146,   146,   145,   145,   141,   254,   145,     5,   140,   259,
     260,   251,   141,   145,   145,   276,   145,   260,   120,   145,
     141,   141,   141,    18,   162,   165,   141,   141,    13,    14,
     208,   225,   226,   217,   219,   219,    13,    14,   208,    12,
      13,   220,   221,    13,    13,   225,   141,    13,   141,   141,
     141,   117,    18,   187,   187,   187,   187,   276,   276,    12,
     119,   119,   271,   238,   195,   141,   141,     1,   120,   212,
     175,   182,   177,   235,    13,   177,   175,   212,   115,   248,
     249,   249,    56,   244,   244,   141,   262,     5,   122,   125,
     258,   261,    21,   127,   145,   120,   145,   141,   168,   171,
      59,   170,   141,   117,   166,   226,   225,   141,   141,    83,
       1,   141,   216,   141,   141,     5,   117,   140,   236,   236,
      83,     1,   141,   141,   141,   141,     7,     8,     9,    10,
      11,   118,   126,   233,   141,   215,    18,   141,   141,   146,
     145,   141,   144,   119,   271,   173,   174,   141,   115,   141,
     115,   115,   115,   210,   115,   175,   141,   242,   260,   140,
     140,   141,   141,   141,   145,   262,   141,   141,   249,   141,
     141,   117,   117,   141,   117,   120,   218,   117,   117,    15,
     117,   236,   117,   120,   117,   117,   117,   141,   118,   233,
     223,   247,   249,   119,     0,   119,   271,   172,   175,   152,
     175,   175,   175,   238,   175,   210,   145,   119,   130,   145,
     244,   249,   257,   260,   169,   141,   117,   117,   117,   141,
     140,   141,    12,    13,   234,   141,   244,   244,   141,     1,
     144,   154,   155,   158,   159,   178,   179,   190,   197,   204,
     206,   214,   268,   269,   270,   141,   141,   210,   144,   141,
     238,   145,   145,   141,   225,   219,   220,   141,   234,   117,
     117,   152,   144,   142,   152,   192,   199,   238,   153,   144,
     145,   141,    13,   232,   141,   141,   141,   268,   268,   144,
     193,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,   144,   200,   202,
       1,   154,   268,   117,   141,   118,   232,   141,   141,   201,
     142,   118,   238,   141,   115,   141,   238,   144
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint16 yyr1[] =
{
       0,   131,   132,   132,   132,   132,   132,   132,   132,   132,
     132,   133,   134,   135,   136,   137,   138,   139,   140,   140,
     141,   141,   142,   142,   142,   143,   143,   144,   144,   145,
     145,   146,   146,   147,   147,   147,   148,   148,   149,   149,
     150,   150,   150,   150,   150,   150,   150,   150,   150,   151,
     151,   151,   151,   152,   152,   153,   153,   154,   154,   154,
     154,   154,   154,   155,   155,   155,   155,   155,   155,   156,
     157,   158,   158,   158,   158,   158,   159,   159,   159,   159,
     160,   160,   161,   161,   162,   162,   163,   164,   164,   165,
     165,   166,   166,   167,   168,   169,   169,   170,   170,   171,
     171,   171,   172,   172,   173,   173,   174,   174,   174,   175,
     176,   177,   178,   178,   178,   179,   179,   180,   180,   181,
     182,   183,   183,   183,   183,   184,   185,   185,   186,   186,
     187,   187,   187,   188,   188,   189,   190,   191,   191,   192,
     192,   193,   194,   194,   195,   195,   195,   196,   197,   197,
     197,   198,   198,   198,   198,   199,   199,   201,   200,   202,
     202,   202,   202,   202,   202,   202,   202,   202,   202,   202,
     202,   202,   202,   202,   202,   203,   204,   204,   204,   205,
     206,   207,   207,   207,   207,   208,   208,   209,   209,   210,
     211,   212,   213,   214,   215,   216,   216,   216,   217,   218,
     219,   220,   220,   221,   221,   221,   222,   223,   223,   223,
     223,   223,   224,   224,   224,   225,   225,   225,   225,   225,
     225,   226,   226,   226,   227,   227,   228,   228,   228,   229,
     229,   229,   229,   229,   230,   231,   231,   231,   231,   232,
     232,   233,   233,   233,   233,   233,   233,   234,   234,   235,
     236,   236,   236,   237,   237,   237,   237,   237,   237,   237,
     237,   237,   237,   237,   237,   237,   237,   237,   237,   237,
     238,   238,   238,   238,   238,   238,   239,   239,   239,   239,
     240,   241,   241,   241,   241,   241,   241,   242,   243,   244,
     244,   245,   245,   246,   246,   246,   247,   247,   248,   248,
     249,   249,   250,   250,   251,   252,   252,   252,   253,   253,
     253,   253,   253,   253,   253,   253,   253,   253,   253,   253,
     253,   253,   253,   253,   254,   254,   254,   254,   254,   254,
     254,   254,   254,   254,   254,   254,   254,   254,   254,   254,
     254,   254,   254,   254,   254,   254,   254,   254,   254,   254,
     254,   254,   254,   254,   255,   255,   255,   256,   256,   256,
     256,   256,   256,   256,   256,   257,   257,   258,   258,   258,
     259,   259,   259,   259,   260,   260,   261,   262,   262,   263,
     263,   263,   263,   264,   264,   265,   265,   266,   266,   267,
     267,   268,   268,   269,   269,   270,   271,   272,   273,   274,
     274,   274,   274,   274,   274,   274,   274,   274,   274,   274,
     274,   274,   275,   276,   276,   276,   276,   276
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     4,     2,     2,     2,     2,     2,     2,     2,
       2,     6,     6,     5,     5,     5,     5,     5,     1,     2,
       0,     2,     0,     2,     2,     0,     1,     1,     1,     1,
       1,     1,     1,     5,     3,     3,     5,     3,     0,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     3,     0,     3,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     7,     7,     7,     4,     4,     6,     6,     3,     3,
       0,     2,     1,     1,     0,     4,     5,     2,     6,     0,
       2,     1,     1,     6,     4,     1,     5,     0,     3,     0,
       1,     1,     1,     5,     0,     1,     1,     4,     2,     0,
       0,     1,    10,     8,     4,    10,     3,     2,     2,     0,
       0,     1,     1,     1,     1,     3,     4,     4,     4,     4,
       5,     1,     3,     9,     9,     0,    10,     1,     1,     0,
       3,     6,     1,     5,     2,     1,     1,     0,    10,     4,
       4,     1,     2,     1,     0,     1,     4,     0,     7,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     0,     8,     4,     4,     0,
      10,     2,     2,     2,     3,     1,     0,     1,     1,     1,
       0,     0,     0,     9,     0,     1,     6,     2,     0,     0,
       3,     1,     1,     1,     5,     2,     2,     1,     1,     2,
       3,     2,     1,     2,     2,     1,     2,     1,     2,     3,
       2,     1,     5,     2,     1,     1,     1,     2,     2,     1,
       1,     1,     1,     1,     2,     5,    10,     6,    11,     2,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     2,
       1,     2,     6,     2,     3,     7,     7,     6,     6,     6,
       6,     6,     6,     5,     6,     5,     6,     7,     6,     6,
       0,     1,     2,     1,     1,     2,     3,     3,     4,     4,
       3,     5,     5,     2,     6,     3,     4,     3,     2,     2,
       0,     2,     3,     3,     4,     4,     2,     2,     0,     1,
       1,     2,     1,     3,     3,     2,     2,     0,     2,     3,
       2,     2,     2,     3,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     3,     4,     5,     8,     7,
       6,     9,     8,     4,     4,     1,     1,     1,     2,     1,
       3,     3,     3,     3,     0,     1,     5,     1,     2,     1,
       3,     3,     1,     2,     5,     5,     4,     1,     1,     4,
       4,     1,     2,     3,     3,     2,     3,     3,     3,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     0,     2,     2,     2,     2
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (parser, YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, parser); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, CSSParser* parser)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  YYUSE (parser);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, CSSParser* parser)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, parser);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, CSSParser* parser)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              , parser);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, parser); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, CSSParser* parser)
{
  YYUSE (yyvaluep);
  YYUSE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yytype)
    {
          case 150: /* valid_rule  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2075 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 151: /* rule  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2081 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 152: /* block_rule_list  */
#line 228 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).ruleList); }
#line 2087 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 153: /* block_valid_rule_list  */
#line 228 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).ruleList); }
#line 2093 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 154: /* block_valid_rule  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2099 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 155: /* block_rule  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2105 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 158: /* import  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2111 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 162: /* maybe_media_value  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2117 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 164: /* source_size_list  */
#line 239 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).sourceSizeList); }
#line 2123 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 165: /* maybe_source_media_query_expression  */
#line 241 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).mediaQueryExpression); }
#line 2129 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 166: /* source_size_length  */
#line 243 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2135 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 167: /* base_media_query_expression  */
#line 237 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).mediaQueryExpression); }
#line 2141 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 168: /* media_query_expression  */
#line 237 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).mediaQueryExpression); }
#line 2147 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 169: /* media_query_expression_list  */
#line 245 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).mediaQueryExpressionList); }
#line 2153 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 170: /* maybe_and_media_query_expression_list  */
#line 245 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).mediaQueryExpressionList); }
#line 2159 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 172: /* media_query  */
#line 234 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).mediaQuery); }
#line 2165 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 173: /* maybe_media_list  */
#line 232 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).mediaList)) ((*yyvaluep).mediaList)->deref(); }
#line 2171 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 174: /* media_list  */
#line 232 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).mediaList)) ((*yyvaluep).mediaList)->deref(); }
#line 2177 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 178: /* media  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2183 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 179: /* supports  */
#line 282 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2189 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 190: /* keyframes  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2195 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 192: /* keyframes_rule  */
#line 250 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).keyframeRuleList); }
#line 2201 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 193: /* keyframe_rule  */
#line 248 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).keyframe)) ((*yyvaluep).keyframe)->deref(); }
#line 2207 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 194: /* key_list  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2213 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 197: /* page  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2219 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 198: /* page_selector  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2225 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 204: /* font_face  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2231 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 206: /* region  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2237 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 214: /* ruleset  */
#line 226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).rule)) ((*yyvaluep).rule)->deref(); }
#line 2243 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 216: /* selector_list  */
#line 258 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selectorList); }
#line 2249 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 219: /* nested_selector_list  */
#line 258 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selectorList); }
#line 2255 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 221: /* comma_separated_lang_ranges  */
#line 268 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).stringList); }
#line 2261 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 222: /* complex_selector_with_trailing_whitespace  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2267 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 223: /* complex_selector  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2273 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 225: /* compound_selector  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2279 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 226: /* simple_selector_list  */
#line 258 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selectorList); }
#line 2285 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 228: /* specifier_list  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2291 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 229: /* specifier  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2297 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 230: /* class  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2303 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 231: /* attrib  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2309 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 235: /* pseudo_page  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2315 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 236: /* nth_selector_ending  */
#line 259 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selectorList); }
#line 2321 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 237: /* pseudo  */
#line 256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).selector); }
#line 2327 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 245: /* ident_list  */
#line 273 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2333 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 246: /* track_names_list  */
#line 275 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2339 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 247: /* whitespace_or_expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2345 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 248: /* maybe_expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2351 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 249: /* expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2357 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 250: /* valid_expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2363 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 253: /* term  */
#line 253 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2369 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 255: /* function  */
#line 253 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2375 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 256: /* variable_function  */
#line 253 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2381 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 258: /* calc_func_term  */
#line 253 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2387 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 261: /* calc_func_paren_expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2393 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 262: /* calc_func_expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2399 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 263: /* valid_calc_func_expr  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2405 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 264: /* calc_func_expr_list  */
#line 265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).valueList); }
#line 2411 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 265: /* calc_function  */
#line 253 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2417 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;

    case 267: /* min_or_max_function  */
#line 253 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1257  */
      { destroy(((*yyvaluep).value)); }
#line 2423 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1257  */
        break;


      default:
        break;
    }
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (CSSParser* parser)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
                  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, parser);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 11:
#line 299 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->m_rule = adoptRef((yyvsp[-2].rule)); }
#line 2691 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 12:
#line 300 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->m_keyframe = adoptRef((yyvsp[-2].keyframe)); }
#line 2697 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 14:
#line 303 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[-1].valueList)) {
            parser->m_valueList = std::unique_ptr<CSSParserValueList>((yyvsp[-1].valueList));
            int oldParsedProperties = parser->m_parsedProperties.size();
            if (!parser->parseValue(parser->m_id, parser->m_important))
                parser->rollbackLastProperties(parser->m_parsedProperties.size() - oldParsedProperties);
            parser->m_valueList = nullptr;
        }
    }
#line 2711 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 15:
#line 313 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->m_mediaQuery = std::unique_ptr<MediaQuery>((yyvsp[-1].mediaQuery)); }
#line 2717 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 16:
#line 315 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[-1].selectorList)) {
            if (parser->m_selectorListForParseSelector)
                parser->m_selectorListForParseSelector->adoptSelectorVector(*(yyvsp[-1].selectorList));
            parser->recycleSelectorVector(std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>>((yyvsp[-1].selectorList)));
        }
    }
#line 2729 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 17:
#line 323 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->m_supportsCondition = (yyvsp[-1].boolean); }
#line 2735 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 33:
#line 332 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
     if (parser->m_styleSheet)
         parser->m_styleSheet->parserSetEncodingFromCharsetRule((yyvsp[-2].string));
     if (parser->isExtractingSourceData() && parser->m_currentRuleDataStack->isEmpty() && parser->m_ruleSourceDataResult)
         parser->addNewRuleToSourceTree(CSSRuleSourceData::createUnknown());
  }
#line 2746 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 39:
#line 343 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if (RefPtr<StyleRuleBase> rule = adoptRef((yyvsp[-1].rule))) {
            if (parser->m_styleSheet)
                parser->m_styleSheet->parserAppendRule(rule.releaseNonNull());
        }
    }
#line 2757 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 45:
#line 356 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2763 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 49:
#line 362 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = (yyvsp[0].rule);
        parser->m_hadSyntacticallyValidCSSRule = true;
    }
#line 2772 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 50:
#line 366 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2778 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 51:
#line 367 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2784 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 52:
#line 368 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2790 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 53:
#line 371 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.ruleList) = nullptr; }
#line 2796 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 54:
#line 372 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
      (yyval.ruleList) = (yyvsp[-2].ruleList);
      if (RefPtr<StyleRuleBase> rule = adoptRef((yyvsp[-1].rule))) {
          if (!(yyval.ruleList))
              (yyval.ruleList) = new Vector<RefPtr<StyleRuleBase>>;
          (yyval.ruleList)->append(WTFMove(rule));
      }
  }
#line 2809 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 55:
#line 382 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.ruleList) = nullptr; }
#line 2815 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 56:
#line 383 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
      (yyval.ruleList) = (yyvsp[-2].ruleList);
      if (RefPtr<StyleRuleBase> rule = adoptRef((yyvsp[-1].rule))) {
          if (!(yyval.ruleList))
              (yyval.ruleList) = new Vector<RefPtr<StyleRuleBase>>;
          (yyval.ruleList)->append(WTFMove(rule));
      }
  }
#line 2828 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 64:
#line 400 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2834 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 65:
#line 400 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2840 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 66:
#line 400 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.rule) = nullptr; }
#line 2846 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 69:
#line 402 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderEnd();
        parser->markRuleBodyStart();
    }
#line 2855 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 70:
#line 408 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::IMPORT_RULE);
    }
#line 2863 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 71:
#line 413 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = parser->createImportRule((yyvsp[-3].string), adoptRef((yyvsp[-1].mediaList))).leakRef();
    }
#line 2871 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 72:
#line 416 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = parser->createImportRule((yyvsp[-3].string), adoptRef((yyvsp[-1].mediaList))).leakRef();
    }
#line 2879 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 73:
#line 419 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
        if ((yyvsp[-1].mediaList))
            (yyvsp[-1].mediaList)->deref();
    }
#line 2890 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 74:
#line 425 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
    }
#line 2899 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 75:
#line 429 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
    }
#line 2908 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 76:
#line 435 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->addNamespace((yyvsp[-3].string), (yyvsp[-2].string)); }
#line 2914 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 80:
#line 440 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.string).clear(); }
#line 2920 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 84:
#line 442 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = nullptr; }
#line 2926 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 85:
#line 442 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = (yyvsp[-1].valueList); }
#line 2932 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 86:
#line 444 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->m_sourceSizeList = std::unique_ptr<Vector<CSSParser::SourceSize>>((yyvsp[-2].sourceSizeList));
    }
#line 2940 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 87:
#line 449 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.sourceSizeList) = new Vector<CSSParser::SourceSize>;
        if (auto result = parser->sourceSize(WTFMove(*(yyvsp[-1].mediaQueryExpression)), (yyvsp[0].value)))
            (yyval.sourceSizeList)->append(WTFMove(result.value()));
        delete (yyvsp[-1].mediaQueryExpression);
    }
#line 2951 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 88:
#line 455 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.sourceSizeList) = (yyvsp[-5].sourceSizeList);
        if (auto result = parser->sourceSize(WTFMove(*(yyvsp[-1].mediaQueryExpression)), (yyvsp[0].value)))
            (yyval.sourceSizeList)->append(WTFMove(result.value()));
        delete (yyvsp[-1].mediaQueryExpression);
    }
#line 2962 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 89:
#line 463 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryExpression) = new MediaQueryExpression;
    }
#line 2970 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 93:
#line 468 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserValueList> mediaValue((yyvsp[-1].valueList));
        (yyvsp[-3].string).convertToASCIILowercaseInPlace();
        (yyval.mediaQueryExpression) = new MediaQueryExpression((yyvsp[-3].string), mediaValue.get());
    }
#line 2980 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 94:
#line 475 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[-3].mediaQueryRestrictor) != MediaQuery::None) {
            delete (yyvsp[-1].mediaQueryExpression);
            (yyval.mediaQueryExpression) = new MediaQueryExpression;
        } else
            (yyval.mediaQueryExpression) = (yyvsp[-1].mediaQueryExpression);
    }
#line 2992 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 95:
#line 484 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryExpressionList) = new Vector<MediaQueryExpression>;
        (yyval.mediaQueryExpressionList)->append(WTFMove(*(yyvsp[0].mediaQueryExpression)));
        delete (yyvsp[0].mediaQueryExpression);
    }
#line 3002 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 96:
#line 489 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryExpressionList) = (yyvsp[-4].mediaQueryExpressionList);
        (yyval.mediaQueryExpressionList)->append(WTFMove(*(yyvsp[0].mediaQueryExpression)));
        delete (yyvsp[0].mediaQueryExpression);
    }
#line 3012 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 97:
#line 496 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryExpressionList) = new Vector<MediaQueryExpression>;
    }
#line 3020 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 98:
#line 499 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryExpressionList) = (yyvsp[0].mediaQueryExpressionList);
    }
#line 3028 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 99:
#line 504 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryRestrictor) = MediaQuery::None;
    }
#line 3036 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 100:
#line 507 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryRestrictor) = MediaQuery::Only;
    }
#line 3044 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 101:
#line 510 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQueryRestrictor) = MediaQuery::Not;
    }
#line 3052 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 102:
#line 515 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaQuery) = new MediaQuery(MediaQuery::None, "all", WTFMove(*(yyvsp[0].mediaQueryExpressionList)));
        delete (yyvsp[0].mediaQueryExpressionList);
    }
#line 3061 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 103:
#line 520 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyvsp[-2].string).convertToASCIILowercaseInPlace();
        (yyval.mediaQuery) = new MediaQuery((yyvsp[-4].mediaQueryRestrictor), (yyvsp[-2].string), WTFMove(*(yyvsp[0].mediaQueryExpressionList)));
        delete (yyvsp[0].mediaQueryExpressionList);
    }
#line 3071 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 104:
#line 526 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.mediaList) = &MediaQuerySet::create().leakRef(); }
#line 3077 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 106:
#line 528 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaList) = &MediaQuerySet::create().leakRef();
        (yyval.mediaList)->addMediaQuery(WTFMove(*(yyvsp[0].mediaQuery)));
        delete (yyvsp[0].mediaQuery);
        parser->updateLastMediaLine(*(yyval.mediaList));
    }
#line 3088 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 107:
#line 534 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaList) = (yyvsp[-3].mediaList);
        if ((yyval.mediaList)) {
            (yyval.mediaList)->addMediaQuery(WTFMove(*(yyvsp[0].mediaQuery)));
            parser->updateLastMediaLine(*(yyval.mediaList));
        }
        delete (yyvsp[0].mediaQuery);
    }
#line 3101 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 108:
#line 542 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.mediaList) = nullptr;
        if ((yyvsp[-1].mediaList))
            (yyvsp[-1].mediaList)->deref();
    }
#line 3111 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 109:
#line 549 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleBodyStart();
    }
#line 3119 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 110:
#line 554 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::MEDIA_RULE);
    }
#line 3127 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 111:
#line 559 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderEnd();
    }
#line 3135 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 112:
#line 564 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = &parser->createMediaRule(adoptRef((yyvsp[-6].mediaList)), std::unique_ptr<Vector<RefPtr<StyleRuleBase>>>((yyvsp[-1].ruleList)).get()).leakRef();
    }
#line 3143 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 113:
#line 567 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = &parser->createEmptyMediaRule(std::unique_ptr<Vector<RefPtr<StyleRuleBase>>>((yyvsp[-1].ruleList)).get()).leakRef();
    }
#line 3151 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 114:
#line 570 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
    }
#line 3160 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 115:
#line 576 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = &parser->createSupportsRule((yyvsp[-6].boolean), std::unique_ptr<Vector<RefPtr<StyleRuleBase>>>((yyvsp[-1].ruleList)).get()).leakRef();
    }
#line 3168 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 116:
#line 579 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
        parser->popSupportsRuleData();
    }
#line 3178 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 117:
#line 586 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
    }
#line 3186 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 118:
#line 589 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
    }
#line 3194 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 119:
#line 594 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::SUPPORTS_RULE);
        parser->markSupportsRuleHeaderStart();
    }
#line 3203 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 120:
#line 600 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderEnd();
        parser->markSupportsRuleHeaderEnd();
    }
#line 3212 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 125:
#line 606 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = !(yyvsp[0].boolean); }
#line 3218 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 126:
#line 608 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[-3].boolean) && (yyvsp[0].boolean); }
#line 3224 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 127:
#line 609 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[-3].boolean) && (yyvsp[0].boolean); }
#line 3230 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 128:
#line 612 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[-3].boolean) || (yyvsp[0].boolean); }
#line 3236 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 129:
#line 613 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[-3].boolean) || (yyvsp[0].boolean); }
#line 3242 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 130:
#line 616 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[-2].boolean); }
#line 3248 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 131:
#line 617 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[0].boolean); }
#line 3254 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 132:
#line 618 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = false; }
#line 3260 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 133:
#line 621 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
        CSSParser* p = static_cast<CSSParser*>(parser);
        std::unique_ptr<CSSParserValueList> propertyValue((yyvsp[-3].valueList));
        if ((yyvsp[-6].id) && propertyValue) {
            p->m_valueList = WTFMove(propertyValue);
            int oldParsedProperties = p->m_parsedProperties.size();
            (yyval.boolean) = p->parseValue((yyvsp[-6].id), (yyvsp[-2].boolean));
            if ((yyval.boolean))
                p->rollbackLastProperties(p->m_parsedProperties.size() - oldParsedProperties);
            p->m_valueList = nullptr;
        }
        p->markPropertyEnd((yyvsp[-2].boolean), false);
    }
#line 3279 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 134:
#line 636 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
        CSSParser* p = static_cast<CSSParser*>(parser);
        std::unique_ptr<CSSParserValueList> propertyValue((yyvsp[-3].valueList));
        if (propertyValue) {
            parser->m_valueList = WTFMove(propertyValue);
            int oldParsedProperties = p->m_parsedProperties.size();
            p->setCustomPropertyName((yyvsp[-6].string));
            (yyval.boolean) = p->parseValue(CSSPropertyCustom, (yyvsp[-2].boolean));
            if ((yyval.boolean))
                p->rollbackLastProperties(p->m_parsedProperties.size() - oldParsedProperties);
            p->m_valueList = nullptr;
        }
        p->markPropertyEnd((yyvsp[-2].boolean), false);
    }
#line 3299 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 135:
#line 653 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::KEYFRAMES_RULE);
    }
#line 3307 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 136:
#line 658 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = &parser->createKeyframesRule((yyvsp[-6].string), std::unique_ptr<Vector<RefPtr<StyleKeyframe>>>((yyvsp[-1].keyframeRuleList))).leakRef();
    }
#line 3315 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 139:
#line 664 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.keyframeRuleList) = new Vector<RefPtr<StyleKeyframe>>; }
#line 3321 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 140:
#line 665 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.keyframeRuleList) = (yyvsp[-2].keyframeRuleList);
        if (RefPtr<StyleKeyframe> keyframe = adoptRef((yyvsp[-1].keyframe)))
            (yyval.keyframeRuleList)->append(WTFMove(keyframe));
    }
#line 3331 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 141:
#line 671 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.keyframe) = parser->createKeyframe(*std::unique_ptr<CSSParserValueList>((yyvsp[-5].valueList))).leakRef(); }
#line 3337 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 142:
#line 673 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = new CSSParserValueList;
        (yyval.valueList)->addValue((yyvsp[0].value));
    }
#line 3346 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 143:
#line 677 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = (yyvsp[-4].valueList);
        if ((yyval.valueList))
            (yyval.valueList)->addValue((yyvsp[0].value));
    }
#line 3356 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 144:
#line 684 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).isInt = false;
        (yyval.value).fValue = (yyvsp[-1].integer) * (yyvsp[0].number);
        (yyval.value).unit = CSSPrimitiveValue::CSS_NUMBER;
    }
#line 3367 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 145:
#line 690 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).isInt = false;
        if (equalLettersIgnoringASCIICase((yyvsp[0].string), "from"))
            (yyval.value).fValue = 0;
        else if (equalLettersIgnoringASCIICase((yyvsp[0].string), "to"))
            (yyval.value).fValue = 100;
        else {
            (yyval.value).unit = 0;
            YYERROR;
        }
        (yyval.value).unit = CSSPrimitiveValue::CSS_NUMBER;
    }
#line 3385 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 146:
#line 703 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).unit = 0;
    }
#line 3393 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 147:
#line 708 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::PAGE_RULE);
    }
#line 3401 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 148:
#line 714 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[-6].selector))
            (yyval.rule) = parser->createPageRule(std::unique_ptr<CSSParserSelector>((yyvsp[-6].selector))).leakRef();
        else {
            parser->clearProperties();
            (yyval.rule) = nullptr;
            parser->popRuleData();
        }
    }
#line 3415 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 149:
#line 723 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->popRuleData();
        (yyval.rule) = nullptr;
    }
#line 3424 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 150:
#line 727 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->popRuleData();
        (yyval.rule) = nullptr;
    }
#line 3433 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 151:
#line 733 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector(QualifiedName(nullAtom, (yyvsp[0].string), parser->m_defaultNamespace));
        (yyval.selector)->setForPage();
    }
#line 3442 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 152:
#line 737 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = (yyvsp[0].selector);
        if ((yyval.selector)) {
            (yyval.selector)->prependTagSelector(QualifiedName(nullAtom, (yyvsp[-1].string), parser->m_defaultNamespace));
            (yyval.selector)->setForPage();
        }
    }
#line 3454 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 153:
#line 744 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = (yyvsp[0].selector);
        if ((yyval.selector))
            (yyval.selector)->setForPage();
    }
#line 3464 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 154:
#line 749 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setForPage();
    }
#line 3473 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 157:
#line 756 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->startDeclarationsForMarginBox();
    }
#line 3481 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 158:
#line 758 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->createMarginAtRule((yyvsp[-6].marginBox));
    }
#line 3489 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 159:
#line 763 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::TopLeftCornerMarginBox;
    }
#line 3497 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 160:
#line 766 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::TopLeftMarginBox;
    }
#line 3505 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 161:
#line 769 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::TopCenterMarginBox;
    }
#line 3513 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 162:
#line 772 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::TopRightMarginBox;
    }
#line 3521 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 163:
#line 775 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::TopRightCornerMarginBox;
    }
#line 3529 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 164:
#line 778 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::BottomLeftCornerMarginBox;
    }
#line 3537 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 165:
#line 781 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::BottomLeftMarginBox;
    }
#line 3545 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 166:
#line 784 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::BottomCenterMarginBox;
    }
#line 3553 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 167:
#line 787 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::BottomRightMarginBox;
    }
#line 3561 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 168:
#line 790 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::BottomRightCornerMarginBox;
    }
#line 3569 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 169:
#line 793 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::LeftTopMarginBox;
    }
#line 3577 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 170:
#line 796 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::LeftMiddleMarginBox;
    }
#line 3585 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 171:
#line 799 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::LeftBottomMarginBox;
    }
#line 3593 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 172:
#line 802 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::RightTopMarginBox;
    }
#line 3601 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 173:
#line 805 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::RightMiddleMarginBox;
    }
#line 3609 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 174:
#line 808 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.marginBox) = CSSSelector::RightBottomMarginBox;
    }
#line 3617 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 175:
#line 813 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::FONT_FACE_RULE);
    }
#line 3625 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 176:
#line 818 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = parser->createFontFaceRule().leakRef();
    }
#line 3633 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 177:
#line 821 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
    }
#line 3642 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 178:
#line 825 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = nullptr;
        parser->popRuleData();
    }
#line 3651 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 179:
#line 831 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::REGION_RULE);
    }
#line 3659 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 180:
#line 836 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<Vector<RefPtr<StyleRuleBase>>> ruleList((yyvsp[-1].ruleList));
        if ((yyvsp[-6].selectorList))
            (yyval.rule) = parser->createRegionRule(std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>>((yyvsp[-6].selectorList)).get(), ruleList.get()).leakRef();
        else {
            (yyval.rule) = nullptr;
            parser->popRuleData();
        }
    }
#line 3673 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 181:
#line 847 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.relation) = CSSParserSelectorCombinator::DirectAdjacent; }
#line 3679 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 182:
#line 848 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.relation) = CSSParserSelectorCombinator::IndirectAdjacent; }
#line 3685 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 183:
#line 849 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.relation) = CSSParserSelectorCombinator::Child; }
#line 3691 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 184:
#line 850 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.relation) = CSSParserSelectorCombinator::DescendantDoubleChild; }
#line 3697 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 186:
#line 852 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.integer) = 1; }
#line 3703 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 187:
#line 853 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.integer) = -1; }
#line 3709 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 188:
#line 853 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.integer) = 1; }
#line 3715 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 189:
#line 854 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->markPropertyStart(); }
#line 3721 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 190:
#line 856 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markRuleHeaderStart(CSSRuleSourceData::STYLE_RULE);
        parser->markSelectorStart();
    }
#line 3730 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 191:
#line 861 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->markRuleHeaderEnd(); }
#line 3736 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 192:
#line 862 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->markSelectorEnd(); }
#line 3742 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 193:
#line 864 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.rule) = parser->createStyleRule((yyvsp[-7].selectorList)).leakRef();
        parser->recycleSelectorVector(std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>>((yyvsp[-7].selectorList)));
    }
#line 3751 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 194:
#line 869 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->markSelectorStart(); }
#line 3757 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 195:
#line 871 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = nullptr;
        if ((yyvsp[0].selector)) {
            (yyval.selectorList) = parser->createSelectorVector().release();
            (yyval.selectorList)->append(std::unique_ptr<CSSParserSelector>((yyvsp[0].selector)));
            parser->updateLastSelectorLineAndPosition();
        }
    }
#line 3770 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 196:
#line 879 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> selectorList((yyvsp[-5].selectorList));
        std::unique_ptr<CSSParserSelector> selector((yyvsp[0].selector));
        (yyval.selectorList) = nullptr;
        if (selectorList && selector) {
            (yyval.selectorList) = selectorList.release();
            (yyval.selectorList)->append(WTFMove(selector));
            parser->updateLastSelectorLineAndPosition();
        }
    }
#line 3785 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 197:
#line 889 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = nullptr;
        delete (yyvsp[-1].selectorList);
    }
#line 3794 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 198:
#line 894 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->startNestedSelectorList(); }
#line 3800 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 199:
#line 895 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->endNestedSelectorList(); }
#line 3806 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 200:
#line 897 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = (yyvsp[-1].selectorList);
    }
#line 3814 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 203:
#line 903 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.stringList) = new Vector<CSSParserString>;
        (yyval.stringList)->append((yyvsp[0].string));
    }
#line 3823 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 204:
#line 907 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.stringList) = (yyvsp[-4].stringList);
        if ((yyval.stringList))
            (yyvsp[-4].stringList)->append((yyvsp[0].string));
    }
#line 3833 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 205:
#line 912 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.stringList) = nullptr;
        delete (yyvsp[-1].stringList);
    }
#line 3842 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 209:
#line 922 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserSelector> left((yyvsp[-1].selector));
        std::unique_ptr<CSSParserSelector> right((yyvsp[0].selector));
        (yyval.selector) = nullptr;
        if (left && right) {
            right->appendTagHistory(CSSParserSelectorCombinator::DescendantSpace, WTFMove(left));
            (yyval.selector) = right.release();
        }
    }
#line 3856 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 210:
#line 931 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserSelector> left((yyvsp[-2].selector));
        std::unique_ptr<CSSParserSelector> right((yyvsp[0].selector));
        (yyval.selector) = nullptr;
        if (left && right) {
            right->appendTagHistory((yyvsp[-1].relation), WTFMove(left));
            (yyval.selector) = right.release();
        }
    }
#line 3870 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 211:
#line 940 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        delete (yyvsp[-1].selector);
    }
#line 3879 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 212:
#line 946 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        static LChar emptyString = '\0';
        (yyval.string).init(&emptyString, 0);
    }
#line 3888 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 213:
#line 950 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { static LChar star = '*'; (yyval.string).init(&star, 1); }
#line 3894 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 215:
#line 954 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector(QualifiedName(nullAtom, (yyvsp[0].string), parser->m_defaultNamespace));
    }
#line 3902 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 216:
#line 957 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = (yyvsp[0].selector);
        if ((yyval.selector)) {
            QualifiedName elementName(nullAtom, (yyvsp[-1].string), parser->m_defaultNamespace);
            parser->rewriteSpecifiersWithElementName(elementName, *(yyval.selector));
        }
    }
#line 3914 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 217:
#line 964 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = (yyvsp[0].selector);
        if ((yyval.selector))
            parser->rewriteSpecifiersWithNamespaceIfNeeded(*(yyval.selector));
    }
#line 3924 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 218:
#line 969 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector(parser->determineNameInNamespace((yyvsp[-1].string), (yyvsp[0].string)));
    }
#line 3932 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 219:
#line 972 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = (yyvsp[0].selector);
        if ((yyval.selector))
            parser->rewriteSpecifiersWithElementName((yyvsp[-2].string), (yyvsp[-1].string), *(yyval.selector));
    }
#line 3942 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 220:
#line 977 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = (yyvsp[0].selector);
        if ((yyval.selector))
            parser->rewriteSpecifiersWithElementName((yyvsp[-1].string), starAtom, *(yyval.selector));
    }
#line 3952 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 221:
#line 984 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = nullptr;
        if ((yyvsp[0].selector)) {
            (yyval.selectorList) = parser->createSelectorVector().release();
            (yyval.selectorList)->append(std::unique_ptr<CSSParserSelector>((yyvsp[0].selector)));
        }
    }
#line 3964 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 222:
#line 991 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> list((yyvsp[-4].selectorList));
        std::unique_ptr<CSSParserSelector> selector((yyvsp[0].selector));
        (yyval.selectorList) = nullptr;
        if (list && selector) {
            (yyval.selectorList) = list.release();
            (yyval.selectorList)->append(WTFMove(selector));
        }
    }
#line 3978 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 223:
#line 1000 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = nullptr;
        delete (yyvsp[-1].selectorList);
    }
#line 3987 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 224:
#line 1006 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.string) = (yyvsp[0].string);
    }
#line 3995 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 225:
#line 1009 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        static LChar star = '*';
        (yyval.string).init(&star, 1);
    }
#line 4004 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 227:
#line 1016 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserSelector> list((yyvsp[-1].selector));
        std::unique_ptr<CSSParserSelector> specifier((yyvsp[0].selector));
        (yyval.selector) = nullptr;
        if (list && specifier)
            (yyval.selector) = parser->rewriteSpecifiers(WTFMove(list), WTFMove(specifier)).release();
    }
#line 4016 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 228:
#line 1023 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        delete (yyvsp[-1].selector);
    }
#line 4025 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 229:
#line 1029 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setMatch(CSSSelector::Id);
        if (parser->m_context.mode == CSSQuirksMode)
            (yyvsp[0].string).convertToASCIILowercaseInPlace();
        (yyval.selector)->setValue((yyvsp[0].string));
    }
#line 4037 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 230:
#line 1036 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[0].string)[0] >= '0' && (yyvsp[0].string)[0] <= '9')
            (yyval.selector) = nullptr;
        else {
            (yyval.selector) = new CSSParserSelector;
            (yyval.selector)->setMatch(CSSSelector::Id);
            if (parser->m_context.mode == CSSQuirksMode)
                (yyvsp[0].string).convertToASCIILowercaseInPlace();
            (yyval.selector)->setValue((yyvsp[0].string));
        }
    }
#line 4053 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 234:
#line 1052 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setMatch(CSSSelector::Class);
        if (parser->m_context.mode == CSSQuirksMode)
            (yyvsp[0].string).convertToASCIILowercaseInPlace();
        (yyval.selector)->setValue((yyvsp[0].string));
    }
#line 4065 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 235:
#line 1061 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setAttribute(QualifiedName(nullAtom, (yyvsp[-2].string), nullAtom), parser->m_context.isHTMLDocument);
        (yyval.selector)->setMatch(CSSSelector::Set);
    }
#line 4075 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 236:
#line 1066 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setAttribute(QualifiedName(nullAtom, (yyvsp[-7].string), nullAtom), parser->m_context.isHTMLDocument);
        (yyval.selector)->setMatch((yyvsp[-5].match));
        (yyval.selector)->setValue((yyvsp[-3].string));
        (yyval.selector)->setAttributeValueMatchingIsCaseInsensitive((yyvsp[-1].boolean));
    }
#line 4087 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 237:
#line 1073 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setAttribute(parser->determineNameInNamespace((yyvsp[-3].string), (yyvsp[-2].string)), parser->m_context.isHTMLDocument);
        (yyval.selector)->setMatch(CSSSelector::Set);
    }
#line 4097 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 238:
#line 1078 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = new CSSParserSelector;
        (yyval.selector)->setAttribute(parser->determineNameInNamespace((yyvsp[-8].string), (yyvsp[-7].string)), parser->m_context.isHTMLDocument);
        (yyval.selector)->setMatch((yyvsp[-5].match));
        (yyval.selector)->setValue((yyvsp[-3].string));
        (yyval.selector)->setAttributeValueMatchingIsCaseInsensitive((yyvsp[-1].boolean));
    }
#line 4109 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 239:
#line 1087 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if (UNLIKELY((yyvsp[-1].string).length() != 1 || !isASCIIAlphaCaselessEqual((yyvsp[-1].string)[0], 'i')))
            YYERROR;
        (yyval.boolean) = true;
    }
#line 4119 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 240:
#line 1093 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
    }
#line 4127 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 241:
#line 1097 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.match) = CSSSelector::Exact;
    }
#line 4135 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 242:
#line 1100 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.match) = CSSSelector::List;
    }
#line 4143 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 243:
#line 1103 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.match) = CSSSelector::Hyphen;
    }
#line 4151 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 244:
#line 1106 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.match) = CSSSelector::Begin;
    }
#line 4159 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 245:
#line 1109 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.match) = CSSSelector::End;
    }
#line 4167 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 246:
#line 1112 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.match) = CSSSelector::Contain;
    }
#line 4175 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 249:
#line 1118 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = CSSParserSelector::parsePagePseudoSelector((yyvsp[0].string));
    }
#line 4183 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 250:
#line 1122 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = nullptr;
    }
#line 4191 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 251:
#line 1125 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selectorList) = nullptr;
    }
#line 4199 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 252:
#line 1128 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[-2].selectorList))
            (yyval.selectorList) = (yyvsp[-2].selectorList);
        else
            YYERROR;
    }
#line 4210 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 253:
#line 1136 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = CSSParserSelector::parsePseudoClassAndCompatibilityElementSelector((yyvsp[0].string));
    }
#line 4218 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 254:
#line 1139 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = CSSParserSelector::parsePseudoElementSelector((yyvsp[0].string));
    }
#line 4226 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 255:
#line 1142 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = CSSParserSelector::parsePseudoElementCueFunctionSelector((yyvsp[-4].string), (yyvsp[-2].selectorList));
    }
#line 4234 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 256:
#line 1145 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = CSSParserSelector::parsePseudoElementSlottedFunctionSelector((yyvsp[-4].string), (yyvsp[-2].selector));
    }
#line 4242 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 257:
#line 1148 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = CSSParserSelector::parsePseudoClassHostFunctionSelector((yyvsp[-4].string), (yyvsp[-2].selector));
    }
#line 4250 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 258:
#line 1151 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        if ((yyvsp[-2].selectorList)) {
            auto selector = std::make_unique<CSSParserSelector>();
            selector->setMatch(CSSSelector::PseudoClass);
            selector->adoptSelectorVector(*std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>>((yyvsp[-2].selectorList)));
            selector->setPseudoClassValue((yyvsp[-4].string));
            if (selector->pseudoClassType() == CSSSelector::PseudoClassAny)
                (yyval.selector) = selector.release();
        }
    }
#line 4266 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 259:
#line 1162 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        if ((yyvsp[-2].selectorList)) {
            auto selector = std::make_unique<CSSParserSelector>();
            selector->setMatch(CSSSelector::PseudoClass);
            selector->adoptSelectorVector(*std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>>((yyvsp[-2].selectorList)));
            selector->setPseudoClassValue((yyvsp[-4].string));
            if (selector->pseudoClassType() == CSSSelector::PseudoClassMatches)
                (yyval.selector) = selector.release();
        }
    }
#line 4282 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 260:
#line 1173 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        if ((yyvsp[-2].stringList)) {
            auto selector = std::make_unique<CSSParserSelector>();
            selector->setMatch(CSSSelector::PseudoClass);
            selector->setLangArgumentList(*std::unique_ptr<Vector<CSSParserString>>((yyvsp[-2].stringList)));
            selector->setPseudoClassValue((yyvsp[-4].string));
            if (selector->pseudoClassType() == CSSSelector::PseudoClassLang)
                (yyval.selector) = selector.release();
        }
    }
#line 4298 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 261:
#line 1184 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        auto selector = std::make_unique<CSSParserSelector>();
        selector->setMatch(CSSSelector::PseudoClass);
        selector->setArgument((yyvsp[-2].string));
        selector->setPseudoClassValue((yyvsp[-4].string));
        if (selector->pseudoClassType() == CSSSelector::PseudoClassDir)
            (yyval.selector) = selector.release();
    }
#line 4312 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 262:
#line 1193 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        auto selector = std::make_unique<CSSParserSelector>();
        selector->setMatch(CSSSelector::PseudoClass);
        selector->setArgument((yyvsp[-2].string));
        selector->setPseudoClassValue((yyvsp[-4].string));
        if (selector->pseudoClassType() == CSSSelector::PseudoClassRole)
            (yyval.selector) = selector.release();
    }
#line 4326 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 263:
#line 1202 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> ending((yyvsp[0].selectorList));
        if (selectorListDoesNotMatchAnyPseudoElement(ending.get())) {
            auto selector = std::make_unique<CSSParserSelector>();
            selector->setMatch(CSSSelector::PseudoClass);
            selector->setArgument((yyvsp[-1].string));
            selector->setPseudoClassValue((yyvsp[-3].string));
            if (ending)
                selector->adoptSelectorVector(*ending);
            CSSSelector::PseudoClassType pseudoClassType = selector->pseudoClassType();
            if (pseudoClassType == CSSSelector::PseudoClassNthChild || pseudoClassType == CSSSelector::PseudoClassNthLastChild)
                (yyval.selector) = selector.release();
        }
    }
#line 4346 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 264:
#line 1217 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> ending((yyvsp[0].selectorList));
        if (selectorListDoesNotMatchAnyPseudoElement(ending.get())) {
            auto selector = std::make_unique<CSSParserSelector>();
            selector->setMatch(CSSSelector::PseudoClass);
            selector->setArgument(AtomicString::number((yyvsp[-2].integer) * (yyvsp[-1].number)));
            selector->setPseudoClassValue((yyvsp[-4].string));
            if (ending)
                selector->adoptSelectorVector(*ending);
            CSSSelector::PseudoClassType pseudoClassType = selector->pseudoClassType();
            if (pseudoClassType == CSSSelector::PseudoClassNthChild || pseudoClassType == CSSSelector::PseudoClassNthLastChild)
                (yyval.selector) = selector.release();
        }
    }
#line 4366 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 265:
#line 1232 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> ending((yyvsp[0].selectorList));
        if (isValidNthToken((yyvsp[-1].string)) && selectorListDoesNotMatchAnyPseudoElement(ending.get())) {
            auto selector = std::make_unique<CSSParserSelector>();
            selector->setMatch(CSSSelector::PseudoClass);
            selector->setArgument((yyvsp[-1].string));
            selector->setPseudoClassValue((yyvsp[-3].string));
            if (ending)
               selector->adoptSelectorVector(*ending);
            CSSSelector::PseudoClassType pseudoClassType = selector->pseudoClassType();
            if (pseudoClassType == CSSSelector::PseudoClassNthChild || pseudoClassType == CSSSelector::PseudoClassNthLastChild)
                (yyval.selector) = selector.release();
        }
    }
#line 4386 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 266:
#line 1247 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        auto selector = std::make_unique<CSSParserSelector>();
        selector->setMatch(CSSSelector::PseudoClass);
        selector->setArgument((yyvsp[-2].string));
        selector->setPseudoClassValue((yyvsp[-4].string));
        if (selector->pseudoClassType() != CSSSelector::PseudoClassUnknown)
            (yyval.selector) = selector.release();
    }
#line 4400 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 267:
#line 1256 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        auto selector = std::make_unique<CSSParserSelector>();
        selector->setMatch(CSSSelector::PseudoClass);
        selector->setArgument(AtomicString::number((yyvsp[-3].integer) * (yyvsp[-2].number)));
        selector->setPseudoClassValue((yyvsp[-5].string));
        if (selector->pseudoClassType() != CSSSelector::PseudoClassUnknown)
            (yyval.selector) = selector.release();
    }
#line 4414 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 268:
#line 1265 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        auto selector = std::make_unique<CSSParserSelector>();
        selector->setMatch(CSSSelector::PseudoClass);
        selector->setArgument((yyvsp[-2].string));
        selector->setPseudoClassValue((yyvsp[-4].string));
        CSSSelector::PseudoClassType type = selector->pseudoClassType();
        if (type == CSSSelector::PseudoClassUnknown)
            selector = nullptr;
        else if (type == CSSSelector::PseudoClassNthChild ||
                 type == CSSSelector::PseudoClassNthOfType ||
                 type == CSSSelector::PseudoClassNthLastChild ||
                 type == CSSSelector::PseudoClassNthLastOfType) {
            if (!isValidNthToken((yyvsp[-2].string)))
                selector = nullptr;
        }
        (yyval.selector) = selector.release();
    }
#line 4436 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 269:
#line 1282 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.selector) = nullptr;
        if ((yyvsp[-2].selectorList)) {
            std::unique_ptr<Vector<std::unique_ptr<CSSParserSelector>>> list((yyvsp[-2].selectorList));
            if (selectorListDoesNotMatchAnyPseudoElement(list.get())) {
                auto selector = std::make_unique<CSSParserSelector>();
                selector->setMatch(CSSSelector::PseudoClass);
                selector->setPseudoClassValue((yyvsp[-4].string));
                selector->adoptSelectorVector(*list);
                if (selector->pseudoClassType() == CSSSelector::PseudoClassNot)
                    (yyval.selector) = selector.release();
            }
        }
    }
#line 4455 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 270:
#line 1298 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = false; }
#line 4461 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 272:
#line 1300 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = (yyvsp[-1].boolean) || (yyvsp[0].boolean); }
#line 4467 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 274:
#line 1302 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = false; }
#line 4473 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 276:
#line 1306 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markPropertyStart();
        (yyval.boolean) = (yyvsp[-2].boolean);
    }
#line 4482 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 277:
#line 1310 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markPropertyStart();
        (yyval.boolean) = false;
    }
#line 4491 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 278:
#line 1314 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markPropertyStart();
        (yyval.boolean) = (yyvsp[-3].boolean);
        if ((yyvsp[-2].boolean))
            (yyval.boolean) = (yyvsp[-2].boolean);
    }
#line 4502 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 279:
#line 1320 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markPropertyStart();
        (yyval.boolean) = (yyvsp[-3].boolean);
    }
#line 4511 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 280:
#line 1326 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->syntaxError((yyvsp[-1].location), CSSParser::PropertyDeclarationError);
    }
#line 4519 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 281:
#line 1331 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
        bool isPropertyParsed = false;
        std::unique_ptr<CSSParserValueList> propertyValue((yyvsp[-1].valueList));
        if (propertyValue) {
            parser->m_valueList = WTFMove(propertyValue);
            int oldParsedProperties = parser->m_parsedProperties.size();
            parser->setCustomPropertyName((yyvsp[-4].string));
            (yyval.boolean) = parser->parseValue(CSSPropertyCustom, (yyvsp[0].boolean));
            if (!(yyval.boolean))
                parser->rollbackLastProperties(parser->m_parsedProperties.size() - oldParsedProperties);
            else
                isPropertyParsed = true;
            parser->m_valueList = nullptr;
        }
        parser->markPropertyEnd((yyvsp[0].boolean), isPropertyParsed);
    }
#line 4541 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 282:
#line 1348 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
        bool isPropertyParsed = false;
        std::unique_ptr<CSSParserValueList> propertyValue((yyvsp[-1].valueList));
        if ((yyvsp[-4].id) && propertyValue) {
            parser->m_valueList = WTFMove(propertyValue);
            int oldParsedProperties = parser->m_parsedProperties.size();
            (yyval.boolean) = parser->parseValue((yyvsp[-4].id), (yyvsp[0].boolean));
            if (!(yyval.boolean))
                parser->rollbackLastProperties(parser->m_parsedProperties.size() - oldParsedProperties);
            else
                isPropertyParsed = true;
            parser->m_valueList = nullptr;
        }
        parser->markPropertyEnd((yyvsp[0].boolean), isPropertyParsed);
    }
#line 4562 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 283:
#line 1364 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = false; }
#line 4568 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 284:
#line 1365 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markPropertyEnd(false, false);
        delete (yyvsp[-2].valueList);
        (yyval.boolean) = false;
    }
#line 4578 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 285:
#line 1370 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.boolean) = false;
    }
#line 4586 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 286:
#line 1373 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        parser->markPropertyEnd(false, false);
        (yyval.boolean) = false;
    }
#line 4595 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 287:
#line 1378 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->syntaxError((yyvsp[-1].location)); }
#line 4601 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 288:
#line 1379 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.id) = cssPropertyID((yyvsp[-1].string)); }
#line 4607 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 289:
#line 1380 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = true; }
#line 4613 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 290:
#line 1380 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.boolean) = false; }
#line 4619 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 291:
#line 1382 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = new CSSParserValueList;
        (yyval.valueList)->addValue(makeIdentValue((yyvsp[-1].string)));
    }
#line 4628 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 292:
#line 1386 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = (yyvsp[-2].valueList);
        (yyval.valueList)->addValue(makeIdentValue((yyvsp[-1].string)));
    }
#line 4637 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 293:
#line 1392 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).setFromValueList(std::make_unique<CSSParserValueList>());
    }
#line 4645 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 294:
#line 1395 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).setFromValueList(std::unique_ptr<CSSParserValueList>((yyvsp[-1].valueList)));
    }
#line 4653 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 295:
#line 1398 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 4663 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 296:
#line 1405 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        if ((yyvsp[0].valueList))
            (yyval.valueList) = (yyvsp[0].valueList);
        else {
            CSSParserValue val;
            val.id = CSSValueInvalid;
            val.unit = CSSPrimitiveValue::CSS_PARSER_WHITESPACE;
            val.string.init(emptyString());
            (yyval.valueList) = new CSSParserValueList;
            (yyval.valueList)->addValue(val);
        }
    }
#line 4680 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 297:
#line 1417 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = (yyvsp[0].valueList); }
#line 4686 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 298:
#line 1419 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = nullptr; }
#line 4692 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 299:
#line 1419 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = (yyvsp[0].valueList); }
#line 4698 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 301:
#line 1420 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = nullptr; delete (yyvsp[-1].valueList); }
#line 4704 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 302:
#line 1422 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = new CSSParserValueList;
        (yyval.valueList)->addValue((yyvsp[0].value));
    }
#line 4713 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 303:
#line 1426 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = (yyvsp[-2].valueList);
        if (!(yyval.valueList))
            destroy((yyvsp[0].value));
        else {
            if ((yyvsp[-1].character)) {
                CSSParserValue v;
                v.id = CSSValueInvalid;
                v.unit = CSSParserValue::Operator;
                v.iValue = (yyvsp[-1].character);
                (yyval.valueList)->addValue(v);
            }
            (yyval.valueList)->addValue((yyvsp[0].value));
        }
    }
#line 4733 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 305:
#line 1443 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.character) = '/'; }
#line 4739 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 306:
#line 1443 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.character) = ','; }
#line 4745 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 307:
#line 1443 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.character) = 0; }
#line 4751 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 309:
#line 1446 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value) = (yyvsp[-1].value); (yyval.value).fValue *= (yyvsp[-2].integer); }
#line 4757 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 310:
#line 1447 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = (yyvsp[-1].string); (yyval.value).unit = CSSPrimitiveValue::CSS_STRING; }
#line 4763 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 311:
#line 1448 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value) = makeIdentValue((yyvsp[-1].string)); }
#line 4769 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 312:
#line 1449 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = (yyvsp[-1].string); (yyval.value).unit = CSSPrimitiveValue::CSS_DIMENSION; }
#line 4775 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 313:
#line 1450 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = (yyvsp[-1].string); (yyval.value).unit = CSSPrimitiveValue::CSS_DIMENSION; }
#line 4781 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 314:
#line 1451 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = (yyvsp[-1].string); (yyval.value).unit = CSSPrimitiveValue::CSS_URI; }
#line 4787 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 315:
#line 1452 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = (yyvsp[-1].string); (yyval.value).unit = CSSPrimitiveValue::CSS_UNICODE_RANGE; }
#line 4793 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 316:
#line 1453 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = (yyvsp[-1].string); (yyval.value).unit = CSSPrimitiveValue::CSS_PARSER_HEXCOLOR; }
#line 4799 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 317:
#line 1454 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).string = CSSParserString(); (yyval.value).unit = CSSPrimitiveValue::CSS_PARSER_HEXCOLOR; }
#line 4805 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 322:
#line 1459 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
      (yyval.value).id = CSSValueInvalid; (yyval.value).unit = 0;
  }
#line 4813 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 324:
#line 1465 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).isInt = true; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_NUMBER; }
#line 4819 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 325:
#line 1466 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).isInt = false; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_NUMBER; }
#line 4825 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 326:
#line 1467 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_PERCENTAGE; }
#line 4831 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 327:
#line 1468 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_PX; }
#line 4837 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 328:
#line 1469 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_CM; }
#line 4843 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 329:
#line 1470 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_MM; }
#line 4849 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 330:
#line 1471 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_IN; }
#line 4855 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 331:
#line 1472 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_PT; }
#line 4861 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 332:
#line 1473 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_PC; }
#line 4867 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 333:
#line 1474 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_DEG; }
#line 4873 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 334:
#line 1475 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_RAD; }
#line 4879 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 335:
#line 1476 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_GRAD; }
#line 4885 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 336:
#line 1477 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_TURN; }
#line 4891 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 337:
#line 1478 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_MS; }
#line 4897 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 338:
#line 1479 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_S; }
#line 4903 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 339:
#line 1480 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_HZ; }
#line 4909 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 340:
#line 1481 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_KHZ; }
#line 4915 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 341:
#line 1482 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_EMS; }
#line 4921 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 342:
#line 1483 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSParserValue::Q_EMS; }
#line 4927 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 343:
#line 1484 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_EXS; }
#line 4933 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 344:
#line 1485 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
      (yyval.value).id = CSSValueInvalid;
      (yyval.value).fValue = (yyvsp[0].number);
      (yyval.value).unit = CSSPrimitiveValue::CSS_REMS;
      if (parser->m_styleSheet)
          parser->m_styleSheet->parserSetUsesRemUnits();
  }
#line 4945 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 345:
#line 1492 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_CHS; }
#line 4951 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 346:
#line 1493 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_VW; }
#line 4957 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 347:
#line 1494 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_VH; }
#line 4963 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 348:
#line 1495 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_VMIN; }
#line 4969 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 349:
#line 1496 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_VMAX; }
#line 4975 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 350:
#line 1497 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_DPPX; }
#line 4981 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 351:
#line 1498 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_DPI; }
#line 4987 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 352:
#line 1499 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_DPCM; }
#line 4993 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 353:
#line 1500 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value).id = CSSValueInvalid; (yyval.value).fValue = (yyvsp[0].number); (yyval.value).unit = CSSPrimitiveValue::CSS_FR; }
#line 4999 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 354:
#line 1503 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserFunction* f = new CSSParserFunction;
        f->name = (yyvsp[-3].string);
        f->args = std::unique_ptr<CSSParserValueList>((yyvsp[-1].valueList));
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Function;
        (yyval.value).function = f;
    }
#line 5012 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 355:
#line 1511 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserFunction* f = new CSSParserFunction;
        f->name = (yyvsp[-2].string);
        f->args = std::make_unique<CSSParserValueList>();
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Function;
        (yyval.value).function = f;
    }
#line 5025 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 356:
#line 1519 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserFunction* f = new CSSParserFunction;
        f->name = (yyvsp[-3].string);
        f->args = nullptr;
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Function;
        (yyval.value).function = f;
  }
#line 5038 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 357:
#line 1529 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserVariable* var = new CSSParserVariable;
        var->name = (yyvsp[-2].string);
        var->args = nullptr;
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Variable;
        (yyval.value).variable = var;
    }
#line 5051 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 358:
#line 1537 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserVariable* var = new CSSParserVariable;
        var->name = (yyvsp[-5].string);
        var->args = std::unique_ptr<CSSParserValueList>((yyvsp[-1].valueList));
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Variable;
        (yyval.value).variable = var;
    }
#line 5064 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 359:
#line 1545 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserVariable* var = new CSSParserVariable;
        var->name = (yyvsp[-4].string);
        var->args = std::make_unique<CSSParserValueList>();
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Variable;
        (yyval.value).variable = var;
    }
#line 5077 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 360:
#line 1553 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 5087 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 361:
#line 1558 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 5097 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 362:
#line 1563 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 5107 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 363:
#line 1568 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        delete (yyvsp[-1].valueList);
        YYERROR;
    }
#line 5118 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 364:
#line 1574 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 5128 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 368:
#line 1584 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.value) = (yyvsp[0].value); (yyval.value).fValue *= (yyvsp[-1].integer); }
#line 5134 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 370:
#line 1588 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.character) = '+';
    }
#line 5142 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 371:
#line 1591 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.character) = '-';
    }
#line 5150 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 372:
#line 1594 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.character) = '*';
    }
#line 5158 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 373:
#line 1597 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.character) = '/';
    }
#line 5166 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 376:
#line 1603 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = nullptr;
        if ((yyvsp[-2].valueList)) {
            (yyval.valueList) = (yyvsp[-2].valueList);
            CSSParserValue v;
            v.id = CSSValueInvalid;
            v.unit = CSSParserValue::Operator;
            v.iValue = '(';
            (yyval.valueList)->insertValueAt(0, v);
            v.iValue = ')';
            (yyval.valueList)->addValue(v);
        }
    }
#line 5184 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 378:
#line 1617 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.valueList) = nullptr; delete (yyvsp[-1].valueList); }
#line 5190 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 379:
#line 1619 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.valueList) = new CSSParserValueList;
        (yyval.valueList)->addValue((yyvsp[0].value));
    }
#line 5199 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 380:
#line 1623 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserValueList> expression((yyvsp[-2].valueList));
        (yyval.valueList) = nullptr;
        if (expression && (yyvsp[-1].character)) {
            (yyval.valueList) = expression.release();
            CSSParserValue v;
            v.id = CSSValueInvalid;
            v.unit = CSSParserValue::Operator;
            v.iValue = (yyvsp[-1].character);
            (yyval.valueList)->addValue(v);
            (yyval.valueList)->addValue((yyvsp[0].value));
        } else {
            destroy((yyvsp[0].value));
        }
    }
#line 5219 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 381:
#line 1638 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserValueList> left((yyvsp[-2].valueList));
        std::unique_ptr<CSSParserValueList> right((yyvsp[0].valueList));
        (yyval.valueList) = nullptr;
        if (left && (yyvsp[-1].character) && right) {
            CSSParserValue v;
            v.id = CSSValueInvalid;
            v.unit = CSSParserValue::Operator;
            v.iValue = (yyvsp[-1].character);
            left->addValue(v);
            left->extend(*right);
            (yyval.valueList) = left.release();
        }
    }
#line 5238 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 384:
#line 1656 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<CSSParserValueList> list((yyvsp[-4].valueList));
        std::unique_ptr<CSSParserValueList> expression((yyvsp[-1].valueList));
        (yyval.valueList) = nullptr;
        if (list && expression) {
            (yyval.valueList) = list.release();
            CSSParserValue v;
            v.id = CSSValueInvalid;
            v.unit = CSSParserValue::Operator;
            v.iValue = ',';
            (yyval.valueList)->addValue(v);
            (yyval.valueList)->extend(*expression);
        }
    }
#line 5257 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 385:
#line 1672 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserFunction* f = new CSSParserFunction;
        f->name = (yyvsp[-4].string);
        f->args = std::unique_ptr<CSSParserValueList>((yyvsp[-2].valueList));
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Function;
        (yyval.value).function = f;
    }
#line 5270 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 386:
#line 1680 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 5280 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 389:
#line 1688 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        CSSParserFunction* f = new CSSParserFunction;
        f->name = (yyvsp[-3].string);
        f->args = std::unique_ptr<CSSParserValueList>((yyvsp[-1].valueList));
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = CSSParserValue::Function;
        (yyval.value).function = f;
    }
#line 5293 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 390:
#line 1696 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    {
        (yyval.value).id = CSSValueInvalid;
        (yyval.value).unit = 0;
        YYERROR;
    }
#line 5303 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 396:
#line 1705 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { parser->invalidBlockHit(); }
#line 5309 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;

  case 412:
#line 1713 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1646  */
    { (yyval.location) = parser->currentLocation(); }
#line 5315 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
    break;


#line 5319 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.cpp" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (parser, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (parser, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, parser);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp, parser);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (parser, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, parser);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp, parser);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 1720 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/CSSGrammar.y" /* yacc.c:1906  */

