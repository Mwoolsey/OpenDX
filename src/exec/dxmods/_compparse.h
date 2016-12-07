/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_NAME = 258,
     T_FLOAT = 259,
     T_DOUBLE = 260,
     T_INT = 261,
     T_INPUT = 262,
     T_ID = 263,
     T_STRING = 264,
     T_EOF = 265,
     T_LPAR = 266,
     T_RPAR = 267,
     T_LBRA = 268,
     T_RBRA = 269,
     T_LSQB = 270,
     T_RSQB = 271,
     T_COMMA = 272,
     T_PERIOD = 273,
     T_COLON = 274,
     T_LOR = 275,
     T_LAND = 276,
     T_LNOT = 277,
     T_LT = 278,
     T_LE = 279,
     T_GT = 280,
     T_GE = 281,
     T_EQ = 282,
     T_NE = 283,
     T_QUEST = 284,
     T_CROSS = 285,
     T_DOT = 286,
     T_PLUS = 287,
     T_MINUS = 288,
     T_EXP = 289,
     T_TIMES = 290,
     T_DIV = 291,
     T_MOD = 292,
     T_ASSIGN = 293,
     T_SEMI = 294,
     U_PLUS = 295,
     U_MINUS = 296,
     U_LNOT = 297
   };
#endif
#define T_NAME 258
#define T_FLOAT 259
#define T_DOUBLE 260
#define T_INT 261
#define T_INPUT 262
#define T_ID 263
#define T_STRING 264
#define T_EOF 265
#define T_LPAR 266
#define T_RPAR 267
#define T_LBRA 268
#define T_RBRA 269
#define T_LSQB 270
#define T_RSQB 271
#define T_COMMA 272
#define T_PERIOD 273
#define T_COLON 274
#define T_LOR 275
#define T_LAND 276
#define T_LNOT 277
#define T_LT 278
#define T_LE 279
#define T_GT 280
#define T_GE 281
#define T_EQ 282
#define T_NE 283
#define T_QUEST 284
#define T_CROSS 285
#define T_DOT 286
#define T_PLUS 287
#define T_MINUS 288
#define T_EXP 289
#define T_TIMES 290
#define T_DIV 291
#define T_MOD 292
#define T_ASSIGN 293
#define T_SEMI 294
#define U_PLUS 295
#define U_MINUS 296
#define U_LNOT 297




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 27 "./_compparse.y"
typedef union YYSTYPE {
#define MAX_PARSE_STRING_SIZE 512
    char s[MAX_PARSE_STRING_SIZE];
    int i;
    float f;
    double d;
    PTreeNode *a;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 129 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





