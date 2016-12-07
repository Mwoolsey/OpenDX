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
     L_OR = 258,
     L_AND = 259,
     L_NOT = 260,
     L_NEQ = 261,
     L_EQ = 262,
     L_GEQ = 263,
     L_LEQ = 264,
     L_GT = 265,
     L_LT = 266,
     A_MINUS = 267,
     A_PLUS = 268,
     A_MOD = 269,
     A_IDIV = 270,
     A_DIV = 271,
     A_TIMES = 272,
     A_EXP = 273,
     U_MINUS = 274,
     LEX_ERROR = 275,
     V_DXTRUE = 276,
     V_DXFALSE = 277,
     T_EOF = 278,
     T_EOL = 279,
     T_BAR = 280,
     T_LPAR = 281,
     T_RPAR = 282,
     T_LBRA = 283,
     T_RBRA = 284,
     T_LSQB = 285,
     T_RSQB = 286,
     T_ASSIGN = 287,
     T_COMMA = 288,
     T_DOT = 289,
     T_DOTDOT = 290,
     T_COLON = 291,
     T_SEMI = 292,
     T_PP = 293,
     T_MM = 294,
     T_RA = 295,
     K_BEGIN = 296,
     K_CANCEL = 297,
     K_DESCRIBE = 298,
     K_ELSE = 299,
     K_END = 300,
     K_IF = 301,
     K_INCLUDE = 302,
     K_LIST = 303,
     K_MACRO = 304,
     K_PRINT = 305,
     K_QUIT = 306,
     K_THEN = 307,
     K_BACKWARD = 308,
     K_FORWARD = 309,
     K_LOOP = 310,
     K_OFF = 311,
     K_ON = 312,
     K_PALINDROME = 313,
     K_PAUSE = 314,
     K_PLAY = 315,
     K_STEP = 316,
     K_STOP = 317,
     K_VCR = 318,
     P_INTERRUPT = 319,
     P_SYSTEM = 320,
     P_ACK = 321,
     P_MACRO = 322,
     P_FOREGROUND = 323,
     P_BACKGROUND = 324,
     P_ERROR = 325,
     P_MESSAGE = 326,
     P_INFO = 327,
     P_LINQ = 328,
     P_SINQ = 329,
     P_VINQ = 330,
     P_LRESP = 331,
     P_SRESP = 332,
     P_VRESP = 333,
     P_DATA = 334,
     P_COMPLETE = 335,
     P_IMPORT = 336,
     P_IMPORTINFO = 337,
     T_INT = 338,
     T_FLOAT = 339,
     T_STRING = 340,
     T_ID = 341,
     T_EXID = 342
   };
#endif
#define L_OR 258
#define L_AND 259
#define L_NOT 260
#define L_NEQ 261
#define L_EQ 262
#define L_GEQ 263
#define L_LEQ 264
#define L_GT 265
#define L_LT 266
#define A_MINUS 267
#define A_PLUS 268
#define A_MOD 269
#define A_IDIV 270
#define A_DIV 271
#define A_TIMES 272
#define A_EXP 273
#define U_MINUS 274
#define LEX_ERROR 275
#define V_DXTRUE 276
#define V_DXFALSE 277
#define T_EOF 278
#define T_EOL 279
#define T_BAR 280
#define T_LPAR 281
#define T_RPAR 282
#define T_LBRA 283
#define T_RBRA 284
#define T_LSQB 285
#define T_RSQB 286
#define T_ASSIGN 287
#define T_COMMA 288
#define T_DOT 289
#define T_DOTDOT 290
#define T_COLON 291
#define T_SEMI 292
#define T_PP 293
#define T_MM 294
#define T_RA 295
#define K_BEGIN 296
#define K_CANCEL 297
#define K_DESCRIBE 298
#define K_ELSE 299
#define K_END 300
#define K_IF 301
#define K_INCLUDE 302
#define K_LIST 303
#define K_MACRO 304
#define K_PRINT 305
#define K_QUIT 306
#define K_THEN 307
#define K_BACKWARD 308
#define K_FORWARD 309
#define K_LOOP 310
#define K_OFF 311
#define K_ON 312
#define K_PALINDROME 313
#define K_PAUSE 314
#define K_PLAY 315
#define K_STEP 316
#define K_STOP 317
#define K_VCR 318
#define P_INTERRUPT 319
#define P_SYSTEM 320
#define P_ACK 321
#define P_MACRO 322
#define P_FOREGROUND 323
#define P_BACKGROUND 324
#define P_ERROR 325
#define P_MESSAGE 326
#define P_INFO 327
#define P_LINQ 328
#define P_SINQ 329
#define P_VINQ 330
#define P_LRESP 331
#define P_SRESP 332
#define P_VRESP 333
#define P_DATA 334
#define P_COMPLETE 335
#define P_IMPORT 336
#define P_IMPORTINFO 337
#define T_INT 338
#define T_FLOAT 339
#define T_STRING 340
#define T_ID 341
#define T_EXID 342




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 420 "./yuiif.y"
typedef union YYSTYPE {
    char		c;
    int			i;
    float		f;
    char		*s;
    void		*v;
    node		*n;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 219 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





