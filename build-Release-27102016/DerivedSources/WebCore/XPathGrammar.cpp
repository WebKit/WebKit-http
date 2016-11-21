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
#define yyparse         xpathyyparse
#define yylex           xpathyylex
#define yyerror         xpathyyerror
#define yydebug         xpathyydebug
#define yynerrs         xpathyynerrs


/* Copy the first part of user declarations.  */
#line 28 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:339  */


#include "config.h"

#include "XPathFunctions.h"
#include "XPathParser.h"
#include "XPathPath.h"
#include "XPathVariableReference.h"

#define YYMALLOC fastMalloc
#define YYFREE fastFree

#define YYENABLE_NLS 0
#define YYLTYPE_IS_TRIVIAL 1
#define YYDEBUG 0
#define YYMAXDEPTH 10000

using namespace WebCore;
using namespace XPath;


#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:339  */

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
   by #include "XPathGrammar.hpp".  */
#ifndef YY_XPATHYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_622E996FC528212BD0453A076571261D28328D34_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_XPATHGRAMMAR_HPP_INCLUDED
# define YY_XPATHYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_622E996FC528212BD0453A076571261D28328D34_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_XPATHGRAMMAR_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int xpathyydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    MULOP = 258,
    EQOP = 259,
    RELOP = 260,
    PLUS = 261,
    MINUS = 262,
    OR = 263,
    AND = 264,
    FUNCTIONNAME = 265,
    LITERAL = 266,
    NAMETEST = 267,
    NUMBER = 268,
    NODETYPE = 269,
    VARIABLEREFERENCE = 270,
    AXISNAME = 271,
    COMMENT = 272,
    DOTDOT = 273,
    PI = 274,
    NODE = 275,
    SLASHSLASH = 276,
    TEXT = 277,
    XPATH_ERROR = 278
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 54 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:355  */
 
    NumericOp::Opcode numericOpcode; 
    EqTestOp::Opcode equalityTestOpcode;
    StringImpl* string;
    Step::Axis axis;
    LocationPath* locationPath;
    Step::NodeTest* nodeTest;
    Vector<std::unique_ptr<Expression>>* expressionVector;
    Step* step;
    Expression* expression; 

#line 170 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int xpathyyparse (Parser& parser);

#endif /* !YY_XPATHYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_622E996FC528212BD0453A076571261D28328D34_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_XPATHGRAMMAR_HPP_INCLUDED  */

/* Copy the second part of user declarations.  */
#line 98 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:358  */


static int xpathyylex(YYSTYPE* yylval, Parser& parser) { return parser.lex(*yylval); }
static void xpathyyerror(Parser&, const char*) { }


#line 192 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:358  */

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
#define YYFINAL  52
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   134

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  33
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  28
/* YYNRULES -- Number of rules.  */
#define YYNRULES  64
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  101

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   278

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      26,    27,     2,     2,    31,     2,    30,    24,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    25,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    28,     2,    29,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    32,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   108,   108,   115,   119,   125,   129,   134,   139,   147,
     153,   159,   168,   178,   196,   207,   225,   229,   231,   238,
     243,   248,   253,   258,   267,   271,   275,   281,   289,   296,
     303,   308,   315,   321,   326,   332,   338,   342,   350,   361,
     367,   375,   379,   381,   388,   393,   395,   401,   410,   412,
     420,   422,   429,   431,   438,   440,   447,   449,   456,   458,
     463,   470,   472,   479,   481
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "MULOP", "EQOP", "RELOP", "PLUS",
  "MINUS", "OR", "AND", "FUNCTIONNAME", "LITERAL", "NAMETEST", "NUMBER",
  "NODETYPE", "VARIABLEREFERENCE", "AXISNAME", "COMMENT", "DOTDOT", "PI",
  "NODE", "SLASHSLASH", "TEXT", "XPATH_ERROR", "'/'", "'@'", "'('", "')'",
  "'['", "']'", "'.'", "','", "'|'", "$accept", "Top", "Expr",
  "LocationPath", "AbsoluteLocationPath", "RelativeLocationPath", "Step",
  "AxisSpecifier", "NodeTest", "OptionalPredicateList", "PredicateList",
  "Predicate", "DescendantOrSelf", "AbbreviatedStep", "PrimaryExpr",
  "FunctionCall", "ArgumentList", "Argument", "UnionExpr", "PathExpr",
  "FilterExpr", "OrExpr", "AndExpr", "EqualityExpr", "RelationalExpr",
  "AdditiveExpr", "MultiplicativeExpr", "UnaryExpr", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,    47,    64,    40,    41,    91,    93,
      46,    44,   124
};
# endif

#define YYPACT_NINF -37

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-37)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int8 yypact[] =
{
      83,    83,    -8,   -37,    -7,   -37,   -37,   -37,    14,   -37,
      17,    20,   -37,    21,     8,   -37,    83,   -37,    48,   -37,
     -37,   -37,   -17,   -37,    22,    -7,     8,   -37,    -7,   -37,
      23,   -37,    -9,     2,    44,    50,    51,    10,    54,   -37,
     -37,    62,    83,   -37,    -7,   -37,    31,    -5,    32,    33,
     -17,    34,   -37,     8,     8,    -7,    -7,   -37,   -17,    -7,
     104,     8,     8,    83,    83,    83,    83,    83,    83,    83,
     -37,   -37,   -18,   -37,    35,   -37,   -37,    36,   -37,   -37,
     -37,   -37,   -37,   -37,   -37,   -37,   -37,   -17,   -17,    44,
      50,    51,    10,    54,    54,   -37,   -37,    83,   -37,   -37,
     -37
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,    34,    24,    35,    32,    17,     0,    31,
       0,     0,    29,     0,     6,    18,     0,    30,     0,     2,
      44,     4,     5,     9,     0,    24,     0,    16,    48,    36,
      63,    42,    45,     3,    50,    52,    54,    56,    58,    61,
      64,     0,     0,    13,    25,    26,     0,     0,     0,     0,
       7,     0,     1,     0,     0,    24,    24,    12,     8,    49,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      37,    41,     0,    39,     0,    27,    21,     0,    22,    19,
      20,    33,    10,    11,    15,    14,    43,    46,    47,    51,
      53,    55,    57,    59,    60,    62,    38,     0,    28,    23,
      40
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -37,   -37,     3,   -37,   -37,   -12,   -22,   -37,    38,   -20,
      37,   -36,   -21,   -37,   -37,   -37,   -37,   -27,   -37,    11,
     -37,   -37,    13,    27,    41,    19,   -16,    -1
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,    18,    71,    20,    21,    22,    23,    24,    25,    43,
      44,    45,    26,    27,    28,    29,    72,    73,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      40,    54,    50,    19,    12,    57,    77,    53,    75,    96,
      63,    62,    12,    97,    58,    61,    67,    68,    41,    51,
       4,    42,    78,    75,     7,     8,     9,    10,    11,    54,
      13,    82,    83,    15,    55,    84,    85,    54,    17,     8,
      46,    10,    11,    47,    13,    74,    48,    49,    52,    87,
      88,    93,    94,    64,    65,    60,    66,    69,    76,    79,
      80,    81,    56,    99,    98,    59,    54,    54,    95,     1,
     100,    86,     2,     3,     4,     5,    89,     6,     7,     8,
       9,    10,    11,    12,    13,    92,    14,    15,    16,    70,
       1,    90,    17,     2,     3,     4,     5,     0,     6,     7,
       8,     9,    10,    11,    12,    13,    91,    14,    15,    16,
       0,     0,     0,    17,     2,     3,     4,     5,     0,     6,
       7,     8,     9,    10,    11,    12,    13,     0,    14,    15,
      16,     0,     0,     0,    17
};

static const yytype_int8 yycheck[] =
{
       1,    22,    14,     0,    21,    25,    11,    24,    44,    27,
       8,    32,    21,    31,    26,    24,     6,     7,    26,    16,
      12,    28,    27,    59,    16,    17,    18,    19,    20,    50,
      22,    53,    54,    25,    12,    55,    56,    58,    30,    17,
      26,    19,    20,    26,    22,    42,    26,    26,     0,    61,
      62,    67,    68,     9,     4,    32,     5,     3,    27,    27,
      27,    27,    24,    27,    29,    28,    87,    88,    69,     7,
      97,    60,    10,    11,    12,    13,    63,    15,    16,    17,
      18,    19,    20,    21,    22,    66,    24,    25,    26,    27,
       7,    64,    30,    10,    11,    12,    13,    -1,    15,    16,
      17,    18,    19,    20,    21,    22,    65,    24,    25,    26,
      -1,    -1,    -1,    30,    10,    11,    12,    13,    -1,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    24,    25,
      26,    -1,    -1,    -1,    30
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     7,    10,    11,    12,    13,    15,    16,    17,    18,
      19,    20,    21,    22,    24,    25,    26,    30,    34,    35,
      36,    37,    38,    39,    40,    41,    45,    46,    47,    48,
      51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
      60,    26,    28,    42,    43,    44,    26,    26,    26,    26,
      38,    35,     0,    24,    45,    12,    41,    42,    38,    43,
      32,    24,    45,     8,     9,     4,     5,     6,     7,     3,
      27,    35,    49,    50,    35,    44,    27,    11,    27,    27,
      27,    27,    39,    39,    42,    42,    52,    38,    38,    55,
      56,    57,    58,    59,    59,    60,    27,    31,    29,    27,
      50
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    33,    34,    35,    36,    36,    37,    37,    37,    38,
      38,    38,    39,    39,    39,    39,    39,    40,    40,    41,
      41,    41,    41,    41,    42,    42,    43,    43,    44,    45,
      46,    46,    47,    47,    47,    47,    47,    48,    48,    49,
      49,    50,    51,    51,    52,    52,    52,    52,    53,    53,
      54,    54,    55,    55,    56,    56,    57,    57,    58,    58,
      58,    59,    59,    60,    60
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     2,     2,     1,
       3,     3,     2,     2,     3,     3,     1,     1,     1,     3,
       3,     3,     3,     4,     0,     1,     1,     2,     3,     1,
       1,     1,     1,     3,     1,     1,     1,     3,     4,     1,
       3,     1,     1,     3,     1,     1,     3,     3,     1,     2,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     3,
       3,     1,     3,     1,     2
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
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Parser& parser)
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
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, Parser& parser)
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
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule, Parser& parser)
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
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, Parser& parser)
{
  YYUSE (yyvaluep);
  YYUSE (parser);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  switch (yytype)
    {
          case 10: /* FUNCTIONNAME  */
#line 74 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).string)) ((*yyvaluep).string)->deref(); }
#line 1098 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 11: /* LITERAL  */
#line 74 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).string)) ((*yyvaluep).string)->deref(); }
#line 1104 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 12: /* NAMETEST  */
#line 74 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).string)) ((*yyvaluep).string)->deref(); }
#line 1110 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 13: /* NUMBER  */
#line 74 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).string)) ((*yyvaluep).string)->deref(); }
#line 1116 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 14: /* NODETYPE  */
#line 74 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).string)) ((*yyvaluep).string)->deref(); }
#line 1122 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 15: /* VARIABLEREFERENCE  */
#line 74 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { if (((*yyvaluep).string)) ((*yyvaluep).string)->deref(); }
#line 1128 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 35: /* Expr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1134 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 36: /* LocationPath  */
#line 82 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).locationPath); }
#line 1140 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 37: /* AbsoluteLocationPath  */
#line 82 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).locationPath); }
#line 1146 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 38: /* RelativeLocationPath  */
#line 82 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).locationPath); }
#line 1152 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 39: /* Step  */
#line 91 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).step); }
#line 1158 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 41: /* NodeTest  */
#line 85 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).nodeTest); }
#line 1164 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 42: /* OptionalPredicateList  */
#line 88 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expressionVector); }
#line 1170 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 43: /* PredicateList  */
#line 88 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expressionVector); }
#line 1176 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 44: /* Predicate  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1182 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 45: /* DescendantOrSelf  */
#line 91 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).step); }
#line 1188 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 46: /* AbbreviatedStep  */
#line 91 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).step); }
#line 1194 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 47: /* PrimaryExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1200 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 48: /* FunctionCall  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1206 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 49: /* ArgumentList  */
#line 88 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expressionVector); }
#line 1212 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 50: /* Argument  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1218 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 51: /* UnionExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1224 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 52: /* PathExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1230 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 53: /* FilterExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1236 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 54: /* OrExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1242 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 55: /* AndExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1248 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 56: /* EqualityExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1254 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 57: /* RelationalExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1260 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 58: /* AdditiveExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1266 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 59: /* MultiplicativeExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1272 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
        break;

    case 60: /* UnaryExpr  */
#line 94 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1257  */
      { delete ((*yyvaluep).expression); }
#line 1278 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1257  */
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
yyparse (Parser& parser)
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
        case 2:
#line 109 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        parser.setParseResult(std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1548 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 4:
#line 120 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = (yyvsp[0].locationPath);
        (yyval.locationPath)->setAbsolute();
    }
#line 1557 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 6:
#line 130 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = new LocationPath;
    }
#line 1565 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 7:
#line 135 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = (yyvsp[0].locationPath);
    }
#line 1573 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 8:
#line 140 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = (yyvsp[0].locationPath);
        (yyval.locationPath)->prependStep(std::unique_ptr<Step>((yyvsp[-1].step)));
    }
#line 1582 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 9:
#line 148 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = new LocationPath;
        (yyval.locationPath)->appendStep(std::unique_ptr<Step>((yyvsp[0].step)));
    }
#line 1591 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 10:
#line 154 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = (yyvsp[-2].locationPath);
        (yyval.locationPath)->appendStep(std::unique_ptr<Step>((yyvsp[0].step)));
    }
#line 1600 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 11:
#line 160 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.locationPath) = (yyvsp[-2].locationPath);
        (yyval.locationPath)->appendStep(std::unique_ptr<Step>((yyvsp[-1].step)));
        (yyval.locationPath)->appendStep(std::unique_ptr<Step>((yyvsp[0].step)));
    }
#line 1610 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 12:
#line 169 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<Step::NodeTest> nodeTest((yyvsp[-1].nodeTest));
        std::unique_ptr<Vector<std::unique_ptr<Expression>>> predicateList((yyvsp[0].expressionVector));
        if (predicateList)
            (yyval.step) = new Step(Step::ChildAxis, WTFMove(*nodeTest), WTFMove(*predicateList));
        else
            (yyval.step) = new Step(Step::ChildAxis, WTFMove(*nodeTest));
    }
#line 1623 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 13:
#line 179 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String nametest = adoptRef((yyvsp[-1].string));
        std::unique_ptr<Vector<std::unique_ptr<Expression>>> predicateList((yyvsp[0].expressionVector));

        String localName;
        String namespaceURI;
        if (!parser.expandQualifiedName(nametest, localName, namespaceURI)) {
            (yyval.step) = nullptr;
            YYABORT;
        }

        if (predicateList)
            (yyval.step) = new Step(Step::ChildAxis, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI), WTFMove(*predicateList));
        else
            (yyval.step) = new Step(Step::ChildAxis, Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI));
    }
#line 1644 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 14:
#line 197 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<Step::NodeTest> nodeTest((yyvsp[-1].nodeTest));
        std::unique_ptr<Vector<std::unique_ptr<Expression>>> predicateList((yyvsp[0].expressionVector));

        if (predicateList)
            (yyval.step) = new Step((yyvsp[-2].axis), WTFMove(*nodeTest), WTFMove(*predicateList));
        else
            (yyval.step) = new Step((yyvsp[-2].axis), WTFMove(*nodeTest));
    }
#line 1658 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 15:
#line 208 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String nametest = adoptRef((yyvsp[-1].string));
        std::unique_ptr<Vector<std::unique_ptr<Expression>>> predicateList((yyvsp[0].expressionVector));

        String localName;
        String namespaceURI;
        if (!parser.expandQualifiedName(nametest, localName, namespaceURI)) {
            (yyval.step) = nullptr;
            YYABORT;
        }

        if (predicateList)
            (yyval.step) = new Step((yyvsp[-2].axis), Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI), WTFMove(*predicateList));
        else
            (yyval.step) = new Step((yyvsp[-2].axis), Step::NodeTest(Step::NodeTest::NameTest, localName, namespaceURI));
    }
#line 1679 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 18:
#line 232 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.axis) = Step::AttributeAxis;
    }
#line 1687 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 19:
#line 239 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.nodeTest) = new Step::NodeTest(Step::NodeTest::AnyNodeTest);
    }
#line 1695 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 20:
#line 244 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.nodeTest) = new Step::NodeTest(Step::NodeTest::TextNodeTest);
    }
#line 1703 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 21:
#line 249 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.nodeTest) = new Step::NodeTest(Step::NodeTest::CommentNodeTest);
    }
#line 1711 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 22:
#line 254 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.nodeTest) = new Step::NodeTest(Step::NodeTest::ProcessingInstructionNodeTest);
    }
#line 1719 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 23:
#line 259 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String literal = adoptRef((yyvsp[-1].string));
        (yyval.nodeTest) = new Step::NodeTest(Step::NodeTest::ProcessingInstructionNodeTest, literal.stripWhiteSpace());
    }
#line 1728 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 24:
#line 267 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expressionVector) = nullptr;
    }
#line 1736 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 26:
#line 276 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expressionVector) = new Vector<std::unique_ptr<Expression>>;
        (yyval.expressionVector)->append(std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1745 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 27:
#line 282 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expressionVector) = (yyvsp[-1].expressionVector);
        (yyval.expressionVector)->append(std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1754 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 28:
#line 290 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = (yyvsp[-1].expression);
    }
#line 1762 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 29:
#line 297 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.step) = new Step(Step::DescendantOrSelfAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
    }
#line 1770 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 30:
#line 304 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.step) = new Step(Step::SelfAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
    }
#line 1778 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 31:
#line 309 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.step) = new Step(Step::ParentAxis, Step::NodeTest(Step::NodeTest::AnyNodeTest));
    }
#line 1786 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 32:
#line 316 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String name = adoptRef((yyvsp[0].string));
        (yyval.expression) = new VariableReference(name);
    }
#line 1795 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 33:
#line 322 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = (yyvsp[-1].expression);
    }
#line 1803 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 34:
#line 327 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String literal = adoptRef((yyvsp[0].string));
        (yyval.expression) = new StringExpression(WTFMove(literal));
    }
#line 1812 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 35:
#line 333 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String numeral = adoptRef((yyvsp[0].string));
        (yyval.expression) = new Number(numeral.toDouble());
    }
#line 1821 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 37:
#line 343 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String name = adoptRef((yyvsp[-2].string));
        (yyval.expression) = XPath::Function::create(name).release();
        if (!(yyval.expression))
            YYABORT;
    }
#line 1832 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 38:
#line 351 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        String name = adoptRef((yyvsp[-3].string));
        std::unique_ptr<Vector<std::unique_ptr<Expression>>> argumentList((yyvsp[-1].expressionVector));
        (yyval.expression) = XPath::Function::create(name, WTFMove(*argumentList)).release();
        if (!(yyval.expression))
            YYABORT;
    }
#line 1844 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 39:
#line 362 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expressionVector) = new Vector<std::unique_ptr<Expression>>;
        (yyval.expressionVector)->append(std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1853 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 40:
#line 368 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expressionVector) = (yyvsp[-2].expressionVector);
        (yyval.expressionVector)->append(std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1862 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 43:
#line 382 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new Union(std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1870 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 44:
#line 389 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = (yyvsp[0].locationPath);
    }
#line 1878 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 46:
#line 396 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyvsp[0].locationPath)->setAbsolute();
        (yyval.expression) = new Path(std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<LocationPath>((yyvsp[0].locationPath)));
    }
#line 1887 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 47:
#line 402 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyvsp[0].locationPath)->prependStep(std::unique_ptr<Step>((yyvsp[-1].step)));
        (yyvsp[0].locationPath)->setAbsolute();
        (yyval.expression) = new Path(std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<LocationPath>((yyvsp[0].locationPath)));
    }
#line 1897 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 49:
#line 413 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        std::unique_ptr<Vector<std::unique_ptr<Expression>>> predicateList((yyvsp[0].expressionVector));
        (yyval.expression) = new Filter(std::unique_ptr<Expression>((yyvsp[-1].expression)), WTFMove(*predicateList));
    }
#line 1906 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 51:
#line 423 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new LogicalOp(LogicalOp::OP_Or, std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1914 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 53:
#line 432 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new LogicalOp(LogicalOp::OP_And, std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1922 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 55:
#line 441 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new EqTestOp((yyvsp[-1].equalityTestOpcode), std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1930 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 57:
#line 450 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new EqTestOp((yyvsp[-1].equalityTestOpcode), std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1938 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 59:
#line 459 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new NumericOp(NumericOp::OP_Add, std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1946 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 60:
#line 464 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new NumericOp(NumericOp::OP_Sub, std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1954 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 62:
#line 473 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new NumericOp((yyvsp[-1].numericOpcode), std::unique_ptr<Expression>((yyvsp[-2].expression)), std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1962 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;

  case 64:
#line 482 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1646  */
    {
        (yyval.expression) = new Negative(std::unique_ptr<Expression>((yyvsp[0].expression)));
    }
#line 1970 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
    break;


#line 1974 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/build-Release/DerivedSources/WebCore/XPathGrammar.cpp" /* yacc.c:1646  */
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
#line 487 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-622e996fc528212bd0453a076571261d28328d34/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1906  */

