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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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
     V_TRUE = 275,
     V_FALSE = 276,
     T_BAR = 277,
     T_LPAR = 278,
     T_RPAR = 279,
     T_LBRA = 280,
     T_RBRA = 281,
     T_LSQB = 282,
     T_RSQB = 283,
     T_ASSIGN = 284,
     T_COMMA = 285,
     T_COLON = 286,
     T_SEMI = 287,
     T_PP = 288,
     T_MM = 289,
     T_RA = 290,
     T_DOTDOT = 291,
     T_COMMENT = 292,
     T_ID = 293,
     T_EXID = 294,
     T_INT = 295,
     T_STRING = 296,
     T_FLOAT = 297,
     K_BACKWARD = 298,
     K_CANCEL = 299,
     K_ELSE = 300,
     K_FOR = 301,
     K_FORWARD = 302,
     K_IF = 303,
     K_INCLUDE = 304,
     K_LOOP = 305,
     K_MACRO = 306,
     K_MODULE = 307,
     K_NULL = 308,
     K_OFF = 309,
     K_ON = 310,
     K_PALINDROME = 311,
     K_PAUSE = 312,
     K_PLAY = 313,
     K_QUIT = 314,
     K_REPEAT = 315,
     K_RESUME = 316,
     K_SEQUENCE = 317,
     K_STEP = 318,
     K_STOP = 319,
     K_SUSPEND = 320,
     K_THEN = 321,
     K_UNTIL = 322,
     K_WHILE = 323,
     P_INTERRUPT = 324,
     P_SYSTEM = 325,
     P_ACK = 326,
     P_MACRO = 327,
     P_FOREGROUND = 328,
     P_BACKGROUND = 329,
     P_ERROR = 330,
     P_MESSAGE = 331,
     P_INFO = 332,
     P_LINQ = 333,
     P_SINQ = 334,
     P_VINQ = 335,
     P_LRESP = 336,
     P_SRESP = 337,
     P_VRESP = 338,
     P_DATA = 339
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
#define V_TRUE 275
#define V_FALSE 276
#define T_BAR 277
#define T_LPAR 278
#define T_RPAR 279
#define T_LBRA 280
#define T_RBRA 281
#define T_LSQB 282
#define T_RSQB 283
#define T_ASSIGN 284
#define T_COMMA 285
#define T_COLON 286
#define T_SEMI 287
#define T_PP 288
#define T_MM 289
#define T_RA 290
#define T_DOTDOT 291
#define T_COMMENT 292
#define T_ID 293
#define T_EXID 294
#define T_INT 295
#define T_STRING 296
#define T_FLOAT 297
#define K_BACKWARD 298
#define K_CANCEL 299
#define K_ELSE 300
#define K_FOR 301
#define K_FORWARD 302
#define K_IF 303
#define K_INCLUDE 304
#define K_LOOP 305
#define K_MACRO 306
#define K_MODULE 307
#define K_NULL 308
#define K_OFF 309
#define K_ON 310
#define K_PALINDROME 311
#define K_PAUSE 312
#define K_PLAY 313
#define K_QUIT 314
#define K_REPEAT 315
#define K_RESUME 316
#define K_SEQUENCE 317
#define K_STEP 318
#define K_STOP 319
#define K_SUSPEND 320
#define K_THEN 321
#define K_UNTIL 322
#define K_WHILE 323
#define P_INTERRUPT 324
#define P_SYSTEM 325
#define P_ACK 326
#define P_MACRO 327
#define P_FOREGROUND 328
#define P_BACKGROUND 329
#define P_ERROR 330
#define P_MESSAGE 331
#define P_INFO 332
#define P_LINQ 333
#define P_SINQ 334
#define P_VINQ 335
#define P_LRESP 336
#define P_SRESP 337
#define P_VRESP 338
#define P_DATA 339




/* Copy the first part of user declarations.  */
#line 1 "./net.y"


/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*****************************************************************************/
/* uin.yacc -								     */
/*                                                                           */
/* Contains SVS script language grammar for yacc.			     */
/*                                                                           */
/*****************************************************************************/

/*
 * $Header: /src/master/dx/src/uipp/dxuilib/net.y,v 1.1 2002/06/13 20:56:27 gda Exp $
 */

#include "Parse.h"



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 27 "./net.y"
typedef union YYSTYPE {
    union 
    {
	char                c;
	int                 i;
	float               f;
	char                s[4096];	/* 4096 == YYLMAX in net.lex */
    } ast;
} YYSTYPE;
/* Line 191 of yacc.c.  */
#line 278 "y.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 290 "y.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  30
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   205

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  85
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  47
/* YYNRULES -- Number of rules. */
#define YYNRULES  96
/* YYNRULES -- Number of states. */
#define YYNSTATES  172

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   339

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
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
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     6,     7,     9,    11,    13,    25,    33,
      35,    37,    40,    45,    47,    51,    55,    61,    65,    67,
      69,    71,    74,    76,    80,    84,    86,    88,    91,    93,
      95,    99,   103,   106,   109,   111,   113,   117,   119,   121,
     123,   127,   131,   135,   136,   142,   144,   146,   148,   152,
     154,   158,   160,   162,   164,   166,   170,   172,   174,   178,
     180,   182,   184,   186,   188,   192,   196,   200,   206,   214,
     216,   219,   223,   227,   231,   233,   236,   240,   242,   244,
     246,   248,   250,   256,   262,   272,   282,   284,   286,   288,
     291,   295,   297,   300,   305,   307,   309
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const short yyrhs[] =
{
      86,     0,    -1,    88,    87,    -1,    -1,    96,    -1,    89,
      -1,   131,    -1,    51,    38,    23,    90,    24,    35,    23,
      90,    24,   101,    93,    -1,    51,    38,    23,    90,    24,
     101,    93,    -1,   131,    -1,    91,    -1,    92,   101,    -1,
      91,    30,    92,   101,    -1,    38,    -1,    38,    29,    38,
      -1,    38,    29,   115,    -1,    38,    29,    38,    29,   115,
      -1,    25,    94,    26,    -1,   131,    -1,    95,    -1,    96,
      -1,    95,    96,    -1,    93,    -1,    98,   101,    32,    -1,
     106,   101,    32,    -1,    97,    -1,   112,    -1,    49,   127,
      -1,    99,    -1,   100,    -1,   128,    29,   106,    -1,   128,
      29,   113,    -1,    38,    33,    -1,    38,    34,    -1,   131,
      -1,   102,    -1,    27,   103,    28,    -1,   131,    -1,   104,
      -1,   105,    -1,   104,    30,   105,    -1,    38,    31,   127,
      -1,    38,    31,   124,    -1,    -1,    38,   107,    23,   108,
      24,    -1,   131,    -1,   109,    -1,   110,    -1,   109,    30,
     110,    -1,   111,    -1,    38,    29,   111,    -1,   115,    -1,
      38,    -1,    37,    -1,   114,    -1,   113,    30,   114,    -1,
     115,    -1,   130,    -1,    23,   114,    24,    -1,   120,    -1,
     118,    -1,   116,    -1,   127,    -1,    53,    -1,    25,   119,
      26,    -1,    25,   117,    26,    -1,    25,   126,    26,    -1,
      25,   121,    36,   121,    26,    -1,    25,   121,    36,   121,
      31,   121,    26,    -1,   118,    -1,   117,   118,    -1,   117,
      30,   118,    -1,    27,   119,    28,    -1,    27,   117,    28,
      -1,   120,    -1,   119,   120,    -1,   119,    30,   120,    -1,
     121,    -1,   122,    -1,   123,    -1,   124,    -1,   125,    -1,
      23,   124,    30,   124,    24,    -1,    23,   125,    30,   125,
      24,    -1,    23,   124,    30,   124,    30,   124,    30,   124,
      24,    -1,    23,   125,    30,   125,    30,   125,    30,   125,
      24,    -1,    40,    -1,    42,    -1,   127,    -1,   126,   127,
      -1,   126,    30,   127,    -1,    41,    -1,   129,   101,    -1,
     128,    30,   129,   101,    -1,    38,    -1,    38,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   198,   198,   201,   203,   204,   205,   212,   219,   227,
     228,   231,   232,   235,   236,   237,   238,   245,   252,   253,
     256,   257,   264,   265,   266,   267,   268,   274,   282,   283,
     286,   289,   290,   291,   298,   299,   302,   305,   306,   309,
     310,   315,   321,   332,   331,   342,   343,   346,   347,   350,
     351,   354,   358,   368,   378,   379,   382,   383,   384,   391,
     392,   393,   394,   395,   398,   399,   400,   401,   402,   405,
     406,   407,   410,   411,   414,   415,   416,   419,   420,   421,
     424,   425,   428,   429,   432,   433,   436,   439,   442,   443,
     444,   447,   450,   451,   454,   460,   467
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "L_OR", "L_AND", "L_NOT", "L_NEQ", "L_EQ", 
  "L_GEQ", "L_LEQ", "L_GT", "L_LT", "A_MINUS", "A_PLUS", "A_MOD", 
  "A_IDIV", "A_DIV", "A_TIMES", "A_EXP", "U_MINUS", "V_TRUE", "V_FALSE", 
  "T_BAR", "T_LPAR", "T_RPAR", "T_LBRA", "T_RBRA", "T_LSQB", "T_RSQB", 
  "T_ASSIGN", "T_COMMA", "T_COLON", "T_SEMI", "T_PP", "T_MM", "T_RA", 
  "T_DOTDOT", "T_COMMENT", "T_ID", "T_EXID", "T_INT", "T_STRING", 
  "T_FLOAT", "K_BACKWARD", "K_CANCEL", "K_ELSE", "K_FOR", "K_FORWARD", 
  "K_IF", "K_INCLUDE", "K_LOOP", "K_MACRO", "K_MODULE", "K_NULL", "K_OFF", 
  "K_ON", "K_PALINDROME", "K_PAUSE", "K_PLAY", "K_QUIT", "K_REPEAT", 
  "K_RESUME", "K_SEQUENCE", "K_STEP", "K_STOP", "K_SUSPEND", "K_THEN", 
  "K_UNTIL", "K_WHILE", "P_INTERRUPT", "P_SYSTEM", "P_ACK", "P_MACRO", 
  "P_FOREGROUND", "P_BACKGROUND", "P_ERROR", "P_MESSAGE", "P_INFO", 
  "P_LINQ", "P_SINQ", "P_VINQ", "P_LRESP", "P_SRESP", "P_VRESP", "P_DATA", 
  "$accept", "start", "c_0", "top", "macro", "formal_s0", "formal_s", 
  "formal", "block", "statement_s0", "statement_s", "statement", 
  "include", "assignment", "f_assignment", "s_assignment", "attributes_0", 
  "attributes", "attribute_s0", "attribute_s", "attribute", 
  "function_call", "@1", "argument_s0", "argument_s", "argument", "value", 
  "comment", "expression_s", "expression", "constant", "list", "tensor_s", 
  "tensor", "scalar_s", "scalar", "real", "complex", "quaternion", "int", 
  "float", "string_s", "string", "l_value_s", "l_value", "r_value", 
  "empty", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    85,    86,    87,    88,    88,    88,    89,    89,    90,
      90,    91,    91,    92,    92,    92,    92,    93,    94,    94,
      95,    95,    96,    96,    96,    96,    96,    97,    98,    98,
      99,   100,   100,   100,   101,   101,   102,   103,   103,   104,
     104,   105,   105,   107,   106,   108,   108,   109,   109,   110,
     110,   111,   111,   112,   113,   113,   114,   114,   114,   115,
     115,   115,   115,   115,   116,   116,   116,   116,   116,   117,
     117,   117,   118,   118,   119,   119,   119,   120,   120,   120,
     121,   121,   122,   122,   123,   123,   124,   125,   126,   126,
     126,   127,   128,   128,   129,   130,   131
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     2,     0,     1,     1,     1,    11,     7,     1,
       1,     2,     4,     1,     3,     3,     5,     3,     1,     1,
       1,     2,     1,     3,     3,     1,     1,     2,     1,     1,
       3,     3,     2,     2,     1,     1,     3,     1,     1,     1,
       3,     3,     3,     0,     5,     1,     1,     1,     3,     1,
       3,     1,     1,     1,     1,     3,     1,     1,     3,     1,
       1,     1,     1,     1,     3,     3,     3,     5,     7,     1,
       2,     3,     3,     3,     1,     2,     3,     1,     1,     1,
       1,     1,     5,     5,     9,     9,     1,     1,     1,     2,
       3,     1,     2,     4,     1,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
      96,    96,    53,    94,     0,     0,     0,     3,     5,    22,
       4,    25,    96,    28,    29,    96,    26,     0,    96,     6,
       0,    19,    20,    18,    32,    33,     0,    91,    27,     0,
       1,     2,    96,     0,    35,    34,     0,     0,     0,    92,
      17,    21,    96,    96,     0,     0,    38,    39,    37,    23,
      24,     0,     0,     0,    95,    86,    87,    63,    30,    31,
      54,    56,    61,    60,    59,    77,    78,    79,    80,    81,
      62,    57,    94,    96,     0,    52,     0,    46,    47,    49,
      51,    45,    13,     0,    10,    96,     9,     0,    36,     0,
      95,     0,    80,    81,     0,    69,     0,    74,    77,     0,
      88,     0,     0,     0,    93,     0,     0,     0,    44,     0,
       0,    96,     0,    11,    42,    41,    40,    58,     0,     0,
      65,     0,    70,    64,     0,    75,     0,    66,     0,    89,
      73,    72,    55,    52,    50,    48,    14,    15,     0,     0,
      96,     0,     0,    71,    76,     0,    90,     0,    96,     8,
      12,    82,     0,    83,     0,    67,     0,    16,     0,     0,
       0,     0,    96,     0,     0,    68,     0,     0,     0,     7,
      84,    85
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     6,    31,     7,     8,    83,    84,    85,     9,    20,
      21,    10,    11,    12,    13,    14,    33,    34,    45,    46,
      47,    15,    26,    76,    77,    78,    79,    16,    59,    60,
      61,    62,    94,    63,    96,    64,    65,    66,    67,    68,
      69,    99,    70,    17,    18,    71,    35
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -126
static const short yypact[] =
{
      54,     9,  -126,    -6,   -31,   -14,    30,  -126,  -126,  -126,
    -126,  -126,    39,  -126,  -126,    39,  -126,    70,    39,  -126,
      37,     9,  -126,  -126,  -126,  -126,    45,  -126,  -126,    47,
    -126,  -126,    52,    75,  -126,  -126,    78,    -5,    56,  -126,
    -126,  -126,    55,    80,    92,    97,   101,  -126,  -126,  -126,
    -126,    94,    46,   148,   110,  -126,  -126,  -126,  -126,   108,
    -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,  -126,
    -126,  -126,  -126,    39,    10,   114,   121,   120,  -126,  -126,
    -126,  -126,   124,   131,   126,    39,  -126,    88,  -126,    52,
    -126,   140,   133,   135,   147,  -126,   128,  -126,   130,   150,
    -126,    85,   139,    94,  -126,   133,   135,    99,  -126,    55,
     119,    18,    80,  -126,  -126,  -126,  -126,  -126,   138,   141,
    -126,   155,  -126,  -126,    62,  -126,    10,  -126,   -31,  -126,
    -126,  -126,  -126,  -126,  -126,  -126,   156,  -126,   161,   162,
      39,   -11,    35,  -126,  -126,    36,  -126,    -2,    80,  -126,
    -126,  -126,   138,  -126,   141,  -126,    10,  -126,   165,   163,
     164,   160,    39,   138,   141,  -126,   162,   168,   171,  -126,
    -126,  -126
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
    -126,  -126,  -126,  -126,  -126,    48,  -126,    86,  -125,  -126,
    -126,    28,  -126,  -126,  -126,  -126,   -13,  -126,  -126,  -126,
     111,   166,  -126,  -126,  -126,    90,    95,  -126,  -126,   -47,
     -33,  -126,   144,   -37,   151,   -41,   -45,  -126,  -126,   -43,
     -48,  -126,     2,  -126,   167,  -126,     0
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -44
static const short yytable[] =
{
      19,    23,    36,    93,    91,    39,    28,    98,    92,    80,
      27,    97,    97,   151,   149,    95,    95,   -43,    51,   152,
      52,    74,    53,    52,    29,    53,   106,    24,    25,    22,
      30,   105,    48,    54,     1,    55,    27,    56,    55,    27,
      56,   169,    81,    86,   114,    32,     2,     3,    57,    41,
      55,    57,    56,   138,   100,   125,   132,   122,     4,   153,
     104,   125,   155,    40,   122,   154,    32,   156,    42,    74,
      43,   142,   113,    53,    80,   141,    80,   137,    74,     1,
      52,   145,    53,   144,   143,    74,    55,    27,    56,   115,
      44,     2,     3,    75,    72,    55,    27,    56,   139,    37,
      38,   129,    55,     4,    56,     5,   160,    49,    57,   159,
      50,   161,    53,   130,   157,   121,   168,    51,    82,    52,
     167,    53,    74,    87,    52,    88,    53,   150,    55,    27,
     146,    89,    90,   -43,    55,    27,    56,   133,   103,    55,
      27,    56,    74,   107,    52,   108,    53,    57,    86,   166,
     109,    74,    57,   110,   123,   111,   112,   136,   124,    55,
      27,    56,    74,   118,   117,   119,   126,   131,    55,   124,
      56,    74,    57,   120,    53,    53,   127,   121,    55,    55,
     128,    56,    53,    56,   148,   147,   165,     1,    55,   162,
      56,    27,   170,   163,   164,   171,   158,   101,   140,   135,
     116,     0,   134,    58,   102,    73
};

static const short yycheck[] =
{
       0,     1,    15,    51,    51,    18,     4,    52,    51,    42,
      41,    52,    53,    24,   139,    52,    53,    23,    23,    30,
      25,    23,    27,    25,    38,    27,    74,    33,    34,     1,
       0,    74,    32,    38,    25,    40,    41,    42,    40,    41,
      42,   166,    42,    43,    87,    27,    37,    38,    53,    21,
      40,    53,    42,    35,    52,    96,   103,    94,    49,    24,
      73,   102,    26,    26,   101,    30,    27,    31,    23,    23,
      23,   119,    85,    27,   107,   118,   109,   110,    23,    25,
      25,   126,    27,   124,   121,    23,    40,    41,    42,    87,
      38,    37,    38,    38,    38,    40,    41,    42,   111,    29,
      30,    99,    40,    49,    42,    51,   154,    32,    53,   152,
      32,   156,    27,    28,   147,    30,   164,    23,    38,    25,
     163,    27,    23,    31,    25,    28,    27,   140,    40,    41,
     128,    30,    38,    23,    40,    41,    42,    38,    30,    40,
      41,    42,    23,    29,    25,    24,    27,    53,   148,   162,
      30,    23,    53,    29,    26,    24,    30,    38,    30,    40,
      41,    42,    23,    30,    24,    30,    36,    28,    40,    30,
      42,    23,    53,    26,    27,    27,    26,    30,    40,    40,
      30,    42,    27,    42,    23,    29,    26,    25,    40,    24,
      42,    41,    24,    30,    30,    24,   148,    53,   112,   109,
      89,    -1,   107,    37,    53,    38
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    25,    37,    38,    49,    51,    86,    88,    89,    93,
      96,    97,    98,    99,   100,   106,   112,   128,   129,   131,
      94,    95,    96,   131,    33,    34,   107,    41,   127,    38,
       0,    87,    27,   101,   102,   131,   101,    29,    30,   101,
      26,    96,    23,    23,    38,   103,   104,   105,   131,    32,
      32,    23,    25,    27,    38,    40,    42,    53,   106,   113,
     114,   115,   116,   118,   120,   121,   122,   123,   124,   125,
     127,   130,    38,   129,    23,    38,   108,   109,   110,   111,
     115,   131,    38,    90,    91,    92,   131,    31,    28,    30,
      38,   114,   124,   125,   117,   118,   119,   120,   121,   126,
     127,   117,   119,    30,   101,   124,   125,    29,    24,    30,
      29,    24,    30,   101,   124,   127,   105,    24,    30,    30,
      26,    30,   118,    26,    30,   120,    36,    26,    30,   127,
      28,    28,   114,    38,   111,   110,    38,   115,    35,   101,
      92,   124,   125,   118,   120,   121,   127,    29,    23,    93,
     101,    24,    30,    24,    30,    26,    31,   115,    90,   124,
     125,   121,    24,    30,    30,    26,   101,   124,   125,    93,
      24,    24
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

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

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
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
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 201 "./net.y"
    { return (0); }
    break;

  case 7:
#line 216 "./net.y"
    {
		    ParseEndOfMacroDefinition();
		}
    break;

  case 8:
#line 222 "./net.y"
    {
		    ParseEndOfMacroDefinition();
		}
    break;

  case 41:
#line 316 "./net.y"
    {
			ParseStringAttribute(yyvsp[-2].ast.s, yyvsp[0].ast.s);
		    }
    break;

  case 42:
#line 322 "./net.y"
    {
			ParseIntAttribute(yyvsp[-2].ast.s, yyvsp[0].ast.i);
		    }
    break;

  case 43:
#line 332 "./net.y"
    {
	            ParseFunctionID(yyvsp[0].ast.s);
	        }
    break;

  case 51:
#line 355 "./net.y"
    {
	            ParseArgument(yyvsp[0].ast.s,0);
	        }
    break;

  case 52:
#line 359 "./net.y"
    {
	            ParseArgument(yyvsp[0].ast.s,1);
	        }
    break;

  case 53:
#line 369 "./net.y"
    {
	            ParseComment(yyvsp[0].ast.s);
	        }
    break;

  case 94:
#line 455 "./net.y"
    {
		    ParseLValue(yyvsp[0].ast.s);
		}
    break;

  case 95:
#line 461 "./net.y"
    {
		    ParseRValue(yyvsp[0].ast.s);
		}
    break;

  case 96:
#line 467 "./net.y"
    {
		}
    break;


    }

/* Line 991 of yacc.c.  */
#line 1432 "y.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
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

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


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

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 471 "./net.y"



#include "netlex.c"


