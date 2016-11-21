#ifndef CSSGRAMMAR_H
#define CSSGRAMMAR_H
/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

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

#ifndef YY_XPATHYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_9D4C9C472778D89386FA3533F95F6B8E8C203894_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_XPATHGRAMMAR_HPP_INCLUDED
# define YY_XPATHYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_9D4C9C472778D89386FA3533F95F6B8E8C203894_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_XPATHGRAMMAR_HPP_INCLUDED
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
#line 54 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/Source/WebCore/xml/XPathGrammar.y" /* yacc.c:1909  */
 
    NumericOp::Opcode numericOpcode; 
    EqTestOp::Opcode equalityTestOpcode;
    StringImpl* string;
    Step::Axis axis;
    LocationPath* locationPath;
    Step::NodeTest* nodeTest;
    Vector<std::unique_ptr<Expression>>* expressionVector;
    Step* step;
    Expression* expression; 

#line 90 "/home/naveen/workspace/ML_03/buildroot-wpe/output/build/wpe-9d4c9c472778d89386fa3533f95f6b8e8c203894/build-Release/DerivedSources/WebCore/XPathGrammar.hpp" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif



int xpathyyparse (Parser& parser);

#endif /* !YY_XPATHYY_HOME_NAVEEN_WORKSPACE_ML_03_BUILDROOT_WPE_OUTPUT_BUILD_WPE_9D4C9C472778D89386FA3533F95F6B8E8C203894_BUILD_RELEASE_DERIVEDSOURCES_WEBCORE_XPATHGRAMMAR_HPP_INCLUDED  */
#endif
