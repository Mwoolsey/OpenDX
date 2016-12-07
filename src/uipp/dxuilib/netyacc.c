#ifndef lint
static const char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#endif

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20140101

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)

#define YYPREFIX "yy"

#define YYPURE 0

#line 2 "./net.y"

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

#line 26 "./net.y"
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union
{
    union 
    {
	char                c;
	int                 i;
	float               f;
	char                s[4096];	/* 4096 == YYLMAX in net.lex */
    } ast;
} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 60 "y.tab.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define L_OR 257
#define L_AND 258
#define L_NOT 259
#define L_EQ 260
#define L_NEQ 261
#define L_LT 262
#define L_GT 263
#define L_LEQ 264
#define L_GEQ 265
#define A_PLUS 266
#define A_MINUS 267
#define A_TIMES 268
#define A_DIV 269
#define A_IDIV 270
#define A_MOD 271
#define A_EXP 272
#define U_MINUS 273
#define V_TRUE 274
#define V_FALSE 275
#define T_BAR 276
#define T_LPAR 277
#define T_RPAR 278
#define T_LBRA 279
#define T_RBRA 280
#define T_LSQB 281
#define T_RSQB 282
#define T_ASSIGN 283
#define T_COMMA 284
#define T_COLON 285
#define T_SEMI 286
#define T_PP 287
#define T_MM 288
#define T_RA 289
#define T_DOTDOT 290
#define T_COMMENT 291
#define T_ID 292
#define T_EXID 293
#define T_INT 294
#define T_STRING 295
#define T_FLOAT 296
#define K_BACKWARD 297
#define K_CANCEL 298
#define K_ELSE 299
#define K_FOR 300
#define K_FORWARD 301
#define K_IF 302
#define K_INCLUDE 303
#define K_LOOP 304
#define K_MACRO 305
#define K_MODULE 306
#define K_NULL 307
#define K_OFF 308
#define K_ON 309
#define K_PALINDROME 310
#define K_PAUSE 311
#define K_PLAY 312
#define K_QUIT 313
#define K_REPEAT 314
#define K_RESUME 315
#define K_SEQUENCE 316
#define K_STEP 317
#define K_STOP 318
#define K_SUSPEND 319
#define K_THEN 320
#define K_UNTIL 321
#define K_WHILE 322
#define P_INTERRUPT 323
#define P_SYSTEM 324
#define P_ACK 325
#define P_MACRO 326
#define P_FOREGROUND 327
#define P_BACKGROUND 328
#define P_ERROR 329
#define P_MESSAGE 330
#define P_INFO 331
#define P_LINQ 332
#define P_SINQ 333
#define P_VINQ 334
#define P_LRESP 335
#define P_SRESP 336
#define P_VRESP 337
#define P_DATA 338
#define YYERRCODE 256
static const short yylhs[] = {                           -1,
    0,   11,   43,   43,   43,   29,   29,   22,   22,   21,
   21,   20,   20,   20,   20,   10,   38,   38,   37,   37,
   36,   36,   36,   36,   36,   25,    4,    4,   18,   33,
   33,   33,    9,    9,    8,    7,    7,    6,    6,    5,
    5,   45,   23,    3,    3,    2,    2,    1,    1,   44,
   44,   12,   17,   17,   16,   16,   16,   14,   14,   14,
   14,   14,   28,   28,   28,   28,   28,   42,   42,   42,
   41,   41,   35,   35,   35,   34,   34,   34,   32,   32,
   13,   13,   30,   30,   24,   19,   40,   40,   40,   39,
   27,   27,   26,   31,   15,
};
static const short yylen[] = {                            2,
    2,    0,    1,    1,    1,   11,    7,    1,    1,    2,
    4,    1,    3,    3,    5,    3,    1,    1,    1,    2,
    1,    3,    3,    1,    1,    2,    1,    1,    3,    3,
    2,    2,    1,    1,    3,    1,    1,    1,    3,    3,
    3,    0,    5,    1,    1,    1,    3,    1,    3,    1,
    1,    1,    1,    3,    1,    1,    3,    1,    1,    1,
    1,    1,    3,    3,    3,    5,    7,    1,    2,    3,
    3,    3,    1,    2,    3,    1,    1,    1,    1,    1,
    5,    5,    9,    9,    1,    1,    1,    2,    3,    1,
    2,    4,    1,    1,    0,
};
static const short yydefred[] = {                         0,
    0,   52,    0,    0,    0,    0,    0,   21,   25,    5,
   27,    0,   24,    0,    0,    4,   28,    3,    2,   17,
   19,    0,    0,   31,   32,    0,   90,   26,    0,    0,
   34,    0,   33,    0,   91,    0,    0,    1,   20,   16,
    0,    0,    0,   38,    0,    0,   36,   22,   23,    0,
    0,    0,    0,   85,   86,   62,   77,   55,   53,    0,
   80,   29,   79,   60,   78,   56,   76,   58,   61,   59,
   93,    0,    0,    0,   46,    0,    0,   50,   44,   48,
    0,    8,    0,    0,    0,    0,    0,   35,   94,    0,
    0,    0,    0,   73,    0,   87,    0,   68,    0,    0,
    0,    0,   92,    0,    0,    0,    0,   43,    0,   10,
    0,    0,   41,   40,   39,   57,    0,    0,    0,   63,
    0,   74,   65,    0,   88,   64,    0,   69,   71,   72,
   54,   51,   49,   47,    0,   14,    0,    0,    0,    0,
    0,    0,   75,   89,   70,    0,   11,    0,    7,   82,
    0,   81,    0,   66,    0,   15,    0,    0,    0,    0,
    0,    0,    0,   67,    0,    0,    0,    6,   84,   83,
};
static const short yydgoto[] = {                          6,
   75,   76,   77,    7,   44,   45,   46,   31,   32,    8,
   38,    9,   57,   58,   33,   59,   60,   11,   61,   83,
   84,   85,   12,   63,   13,   14,   15,   64,   16,   65,
   66,   67,   17,   68,   95,   18,   22,   23,   69,   97,
   70,   99,   19,   80,   26,
};
static const short yysindex[] = {                      -186,
 -145,    0, -236, -283, -269,    0, -242,    0,    0,    0,
    0, -242,    0, -242, -181,    0,    0,    0,    0,    0,
    0, -145, -218,    0,    0, -212,    0,    0, -165, -168,
    0, -154,    0, -143,    0, -260, -147,    0,    0,    0,
 -239, -126,  -87,    0, -112, -103,    0,    0,    0, -197,
 -133, -227,    0,    0,    0,    0,    0,    0,    0,  -89,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -242, -222,  -86,    0,  -85,  -78,    0,    0,    0,
  -82,    0, -242,  -81,  -76, -172, -168,    0,    0,  -74,
  -79,  -77,  -84,    0, -113,    0, -214,    0, -193, -108,
  -90, -197,    0,  -79,  -77, -166, -239,    0, -142,    0,
 -126, -252,    0,    0,    0,    0,  -83,  -80, -222,    0,
 -270,    0,    0, -283,    0,    0,  -73,    0,    0,    0,
    0,    0,    0,    0,  -72,    0, -242,  -68,  -69, -235,
 -192, -149,    0,    0,    0, -139,    0, -126,    0,    0,
  -83,    0,  -80,    0, -222,    0,  -66,  -67,  -65,  -64,
 -242,  -83,  -80,    0,  -69,  -63,  -60,    0,    0,    0,
};
static const short yyrindex[] = {                       220,
  -59,    0,  -99,    0,    0,    0,  -62,    0,    0,    0,
    0,  -62,    0, -124,    0,    0,    0,    0,    0,    0,
    0,  -58,    0,    0,    0,    0,    0,    0,    0,  -57,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -55,  -55,    0,    0,  -56,    0,    0,    0,    0,    0,
    0,    0, -259,    0,    0,    0,    0,    0,    0, -106,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0, -124,    0, -188,    0,  -51,    0,    0,    0,    0,
 -177,    0, -151,  -50,    0,    0,    0,    0,    0,    0,
  -49,  -48, -107,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,  -47,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -88,    0, -151,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -55,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
  -47,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
static const short yygindex[] = {                         0,
  124,    0,    0,    0,  146,    0,    0,    0,  -12, -119,
    0,    0,    0,  -30,    3,  -44,    0,    0,  -42,  123,
    0,   87,  200,  -45,    0,  201,    0,    0,    0,    0,
    0,  -41,    0,  -36,  185,    8,    0,    0,   -3,    0,
  -38,  187,    0,  134,    0,
};
#define YYTABLESIZE 240
static const short yytable[] = {                         34,
   28,   35,   10,   20,   92,   90,   73,   91,   21,   93,
   78,   27,   98,   98,   94,   94,   50,   42,   51,  149,
   52,   94,   29,   54,   94,   55,   94,  105,   30,   39,
  104,   53,   47,   54,   27,   55,  138,   73,   30,   51,
  113,   52,  150,   79,   82,  168,   56,   96,  151,   73,
   24,   25,   74,   52,   54,   27,   55,  131,  122,  103,
  128,   40,  128,  122,   41,  123,   54,   56,   55,  124,
  110,   54,  141,   55,  140,   78,   78,  142,  136,   50,
   27,   51,  114,   52,  143,  152,  126,   52,  145,   51,
  127,  153,    1,  125,   89,   51,   54,   27,   55,  139,
   12,   36,   37,   12,    2,    3,   12,  159,  158,   56,
   73,   42,   51,  160,   52,  156,    4,  167,    5,  166,
  144,   54,   27,   43,  147,  132,   95,   54,   27,   55,
  154,   48,   95,    1,   73,  155,   51,   73,   52,   51,
   56,   52,   49,   73,   71,    2,    3,   52,  165,  135,
   82,   54,   27,   55,   54,   27,   55,    4,   95,   95,
   54,   27,   55,   73,   56,   81,  120,   56,   73,   76,
  121,   87,   76,  129,   30,  121,   76,   42,   88,   30,
   54,   93,   55,   93,   93,   54,   76,   55,   76,   13,
   52,  130,   13,  127,  102,   13,  106,   86,  107,  108,
  109,  112,  111,  116,  117,  119,  118,   52,  148,    1,
  146,  161,   55,   54,  169,  164,  162,  170,  163,   95,
   95,   18,   95,   95,   95,   37,   45,    9,   80,   79,
  134,   95,  115,  137,  157,   62,  100,   72,  101,  133,
};
static const short yycheck[] = {                         12,
    4,   14,    0,    1,   50,   50,  277,   50,    1,   51,
   41,  295,   51,   52,   51,   52,  277,  277,  279,  139,
  281,  281,  292,  294,  284,  296,  286,   73,  281,   22,
   73,  292,   30,  294,  295,  296,  289,  277,  281,  279,
   86,  281,  278,   41,   42,  165,  307,   51,  284,  277,
  287,  288,  292,  281,  294,  295,  296,  102,   95,   72,
   99,  280,  101,  100,  277,  280,  294,  307,  296,  284,
   83,  294,  118,  296,  117,  106,  107,  119,  109,  277,
  295,  279,   86,  281,  121,  278,  280,  281,  127,  278,
  284,  284,  279,   97,  292,  284,  294,  295,  296,  112,
  278,  283,  284,  281,  291,  292,  284,  153,  151,  307,
  277,  277,  279,  155,  281,  146,  303,  163,  305,  162,
  124,  294,  295,  292,  137,  292,  278,  294,  295,  296,
  280,  286,  284,  279,  277,  285,  279,  277,  281,  279,
  307,  281,  286,  277,  292,  291,  292,  281,  161,  292,
  148,  294,  295,  296,  294,  295,  296,  303,  283,  284,
  294,  295,  296,  277,  307,  292,  280,  307,  277,  277,
  284,  284,  280,  282,  281,  284,  284,  277,  282,  286,
  294,  281,  296,  283,  284,  294,  294,  296,  296,  278,
  281,  282,  281,  284,  284,  284,  283,  285,  284,  278,
  283,  278,  284,  278,  284,  290,  284,  281,  277,  279,
  283,  278,  296,  294,  278,  280,  284,  278,  284,    0,
  280,  280,  278,  286,  282,  282,  278,  278,  278,  278,
  107,  279,   87,  111,  148,   36,   52,   37,   52,  106,
};
#define YYFINAL 6
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 338
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? (YYMAXTOKEN + 1) : (a))
#if YYDEBUG
static const char *yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"L_OR","L_AND","L_NOT","L_EQ",
"L_NEQ","L_LT","L_GT","L_LEQ","L_GEQ","A_PLUS","A_MINUS","A_TIMES","A_DIV",
"A_IDIV","A_MOD","A_EXP","U_MINUS","V_TRUE","V_FALSE","T_BAR","T_LPAR","T_RPAR",
"T_LBRA","T_RBRA","T_LSQB","T_RSQB","T_ASSIGN","T_COMMA","T_COLON","T_SEMI",
"T_PP","T_MM","T_RA","T_DOTDOT","T_COMMENT","T_ID","T_EXID","T_INT","T_STRING",
"T_FLOAT","K_BACKWARD","K_CANCEL","K_ELSE","K_FOR","K_FORWARD","K_IF",
"K_INCLUDE","K_LOOP","K_MACRO","K_MODULE","K_NULL","K_OFF","K_ON",
"K_PALINDROME","K_PAUSE","K_PLAY","K_QUIT","K_REPEAT","K_RESUME","K_SEQUENCE",
"K_STEP","K_STOP","K_SUSPEND","K_THEN","K_UNTIL","K_WHILE","P_INTERRUPT",
"P_SYSTEM","P_ACK","P_MACRO","P_FOREGROUND","P_BACKGROUND","P_ERROR",
"P_MESSAGE","P_INFO","P_LINQ","P_SINQ","P_VINQ","P_LRESP","P_SRESP","P_VRESP",
"P_DATA","illegal-symbol",
};
static const char *yyrule[] = {
"$accept : start",
"start : top c_0",
"c_0 :",
"top : statement",
"top : macro",
"top : empty",
"macro : K_MACRO T_ID T_LPAR formal_s0 T_RPAR T_RA T_LPAR formal_s0 T_RPAR attributes_0 block",
"macro : K_MACRO T_ID T_LPAR formal_s0 T_RPAR attributes_0 block",
"formal_s0 : empty",
"formal_s0 : formal_s",
"formal_s : formal attributes_0",
"formal_s : formal_s T_COMMA formal attributes_0",
"formal : T_ID",
"formal : T_ID T_ASSIGN T_ID",
"formal : T_ID T_ASSIGN constant",
"formal : T_ID T_ASSIGN T_ID T_ASSIGN constant",
"block : T_LBRA statement_s0 T_RBRA",
"statement_s0 : empty",
"statement_s0 : statement_s",
"statement_s : statement",
"statement_s : statement_s statement",
"statement : block",
"statement : assignment attributes_0 T_SEMI",
"statement : function_call attributes_0 T_SEMI",
"statement : include",
"statement : comment",
"include : K_INCLUDE string",
"assignment : f_assignment",
"assignment : s_assignment",
"f_assignment : l_value_s T_ASSIGN function_call",
"s_assignment : l_value_s T_ASSIGN expression_s",
"s_assignment : T_ID T_PP",
"s_assignment : T_ID T_MM",
"attributes_0 : empty",
"attributes_0 : attributes",
"attributes : T_LSQB attribute_s0 T_RSQB",
"attribute_s0 : empty",
"attribute_s0 : attribute_s",
"attribute_s : attribute",
"attribute_s : attribute_s T_COMMA attribute",
"attribute : T_ID T_COLON string",
"attribute : T_ID T_COLON int",
"$$1 :",
"function_call : T_ID $$1 T_LPAR argument_s0 T_RPAR",
"argument_s0 : empty",
"argument_s0 : argument_s",
"argument_s : argument",
"argument_s : argument_s T_COMMA argument",
"argument : value",
"argument : T_ID T_ASSIGN value",
"value : constant",
"value : T_ID",
"comment : T_COMMENT",
"expression_s : expression",
"expression_s : expression_s T_COMMA expression",
"expression : constant",
"expression : r_value",
"expression : T_LPAR expression T_RPAR",
"constant : scalar",
"constant : tensor",
"constant : list",
"constant : string",
"constant : K_NULL",
"list : T_LBRA scalar_s T_RBRA",
"list : T_LBRA tensor_s T_RBRA",
"list : T_LBRA string_s T_RBRA",
"list : T_LBRA real T_DOTDOT real T_RBRA",
"list : T_LBRA real T_DOTDOT real T_COLON real T_RBRA",
"tensor_s : tensor",
"tensor_s : tensor_s tensor",
"tensor_s : tensor_s T_COMMA tensor",
"tensor : T_LSQB scalar_s T_RSQB",
"tensor : T_LSQB tensor_s T_RSQB",
"scalar_s : scalar",
"scalar_s : scalar_s scalar",
"scalar_s : scalar_s T_COMMA scalar",
"scalar : real",
"scalar : complex",
"scalar : quaternion",
"real : int",
"real : float",
"complex : T_LPAR int T_COMMA int T_RPAR",
"complex : T_LPAR float T_COMMA float T_RPAR",
"quaternion : T_LPAR int T_COMMA int T_COMMA int T_COMMA int T_RPAR",
"quaternion : T_LPAR float T_COMMA float T_COMMA float T_COMMA float T_RPAR",
"int : T_INT",
"float : T_FLOAT",
"string_s : string",
"string_s : string_s string",
"string_s : string_s T_COMMA string",
"string : T_STRING",
"l_value_s : l_value attributes_0",
"l_value_s : l_value_s T_COMMA l_value attributes_0",
"l_value : T_ID",
"r_value : T_ID",
"empty :",

};
#endif

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#define YYINITSTACKSIZE 200

typedef struct {
    unsigned stacksize;
    short    *s_base;
    short    *s_mark;
    short    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 472 "./net.y"


#include "netlex.c"
#line 489 "y.tab.c"

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (short *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return -1;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return -1;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack)) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    yyerror("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 2:
#line 201 "./net.y"
	{ return (0); }
break;
case 6:
#line 216 "./net.y"
	{
		    ParseEndOfMacroDefinition();
		}
break;
case 7:
#line 222 "./net.y"
	{
		    ParseEndOfMacroDefinition();
		}
break;
case 40:
#line 316 "./net.y"
	{
			ParseStringAttribute(yystack.l_mark[-2].ast.s, yystack.l_mark[0].ast.s);
		    }
break;
case 41:
#line 322 "./net.y"
	{
			ParseIntAttribute(yystack.l_mark[-2].ast.s, yystack.l_mark[0].ast.i);
		    }
break;
case 42:
#line 332 "./net.y"
	{
	            ParseFunctionID(yystack.l_mark[0].ast.s);
	        }
break;
case 50:
#line 355 "./net.y"
	{
	            ParseArgument(yystack.l_mark[0].ast.s,0);
	        }
break;
case 51:
#line 359 "./net.y"
	{
	            ParseArgument(yystack.l_mark[0].ast.s,1);
	        }
break;
case 52:
#line 369 "./net.y"
	{
	            ParseComment(yystack.l_mark[0].ast.s);
	        }
break;
case 93:
#line 455 "./net.y"
	{
		    ParseLValue(yystack.l_mark[0].ast.s);
		}
break;
case 94:
#line 461 "./net.y"
	{
		    ParseRValue(yystack.l_mark[0].ast.s);
		}
break;
case 95:
#line 467 "./net.y"
	{
		}
break;
#line 760 "y.tab.c"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = yyname[YYTRANSLATE(yychar)];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack))
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (short) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    yyerror("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}
