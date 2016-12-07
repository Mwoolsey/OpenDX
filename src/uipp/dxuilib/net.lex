%{

/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

/*****************************************************************************/
/* uin.lex -								     */
/*                                                                           */
/* Contains symbols for the SVS script language grammar.		     */
/*                                                                           */
/*****************************************************************************/

/*
 * $Header: /src/master/dx/src/uipp/dxuilib/net.lex,v 1.4 2002/02/06 19:17:29 davidt Exp $
 * $Log: net.lex,v $
 * Revision 1.4  2002/02/06 19:17:29  davidt
 * Have you ever wondered why after loading a network with a missing macro,
 * that you get all kinds of errors while trying to load the next program?
 * This is due to the fact that the lexer was not having its state machine
 * reset. This fix adds the code to do this so now you can open a program
 * with an error, then open a second program without getting a screen full
 * of errors. Tested with flex and Sun's lex.
 *
 * Revision 1.3  1999/05/10 15:46:23  gda
 * Copyright message
 *
 * Revision 1.3  1999/05/10 15:46:23  gda
 * Copyright message
 *
 * Revision 1.2  1999/04/01 20:06:03  gda
 * Added type for bison
 *
 * Revision 1.1.1.1  1999/03/24 15:18:18  gda
 * Initial CVS Version
 *
Revision 9.1  1997/05/22  19:46:49  svs
Copy of release 3.1.4 code

Revision 8.2  1996/01/24  21:48:38  dawood
mbsinvalid defined as null for NT also (for ajay).

Revision 8.1  1995/12/14  19:10:59  dawood
Added support for \r as a white space character, which helps us to
better support the NT and OS/2 ports which right out their .nets with
CR and LF instead of just LF.

Revision 8.0  1995/10/03  19:24:41  nsc
Copy of release 3.1 code

Revision 7.5  1995/03/24  22:33:50  dawood
Add NULL as a keyword (K_NULL) so that we can support static macros
which have input values inside the macro def.  Sometimes these values
are NULL.

Revision 7.4  1994/05/12  22:31:10  suits
Include string.h instead of single prototype of strcpy

Revision 7.3  1994/04/18  16:10:35  suits
OS2 definition of mbsinvalid

Revision 7.2  94/04/05  11:23:57  dawood
Change union to contain a string buffer, and change net.lex to do
strcpy() instead of strdup().  This fixes a memory leak such that
the strdup()'d string was not being freed.

Revision 7.1  94/01/18  19:20:01  svs
changes since release 2.0.1

Revision 6.1  93/11/16  11:09:17  svs
ship level code, release 2.0

Revision 1.3  93/10/07  15:52:07  cpv
Fixed the $command comment handling.

Revision 1.2  93/10/07  15:40:38  cpv
Accept "$.*" as an empty comment.

Revision 1.1  93/01/08  13:22:31  cpv
Initial revision

Revision 5.0  92/11/17  17:03:42  svs
ship level code, internal rev 1.2.2

Revision 4.0  92/08/18  15:54:50  svs
ship level code, rev 1.2

Revision 3.1  92/06/01  15:56:53  jgv
HP port.

Revision 3.0  91/11/27  17:02:50  svs
ship level code, rev 1.0

Revision 1.5  91/11/07  19:00:35  rbk
Added the latest exec's reserved keywords to the language.

Revision 1.4  91/11/06  17:52:28  rbk
Removed obsolete keywords.

Revision 1.3  91/08/20  16:39:11  rbk
Changed "UM" (unary minus) to "US" (unary sign), allowing unary "+"'s.

Revision 1.2  91/04/17  16:55:51  rbk
Added support for the new list constructor syntax.

Revision 1.1  91/03/19  15:55:27  rbk
Initial revision

Revision 1.4  91/03/18  16:26:35  rbk
Modified code to correctly parse long comment strings.

Revision 1.3  91/01/23  10:10:03  rbk
Removed K_FRAME token and added unary minus to numeric constants.

Revision 1.2  90/12/10  15:37:03  rbk
Added "@FRAME" and "@frame" to list of keywords to be lex'd.

Revision 1.1  90/10/11  18:32:48  rbk
Initial revision

 */

#include <math.h>

#if defined(OS2)  || defined(DXD_WIN)
#define mbsinvalid(x) 0
#endif 

#include <string.h>

#ifdef YYLMAX
#undef YYLMAX
#endif

#define YYLMAX 4096

int yylexerstate = 0;

%}

%p 4000

AL                      [a-zA-Z_@]
ALN                     [a-zA-Z_@'0-9]
ws                      [ \t\n\r]

O                       [0-7]
D                       [0-9]
X                       [0-9a-fA-F]

US                      [\-\+]?
E                       [eE]{US}{D}+

%%
	{ /* This code is run whenever yylex() is called */
	  if (yylexerstate == 1) {
	    BEGIN(INITIAL);
	#ifdef FLEX_SCANNER
	    yyrestart( yyin );
	#endif
	    yylexerstate = 0;
	  }
	}

"true"                  { return (V_TRUE); }
"TRUE"                  { return (V_TRUE); }
"false"                 { return (V_FALSE); }
"FALSE"                 { return (V_FALSE); }
"on"                    { return (V_TRUE); }
"ON"                    { return (V_TRUE); }
"off"                   { return (V_FALSE); }
"OFF"                   { return (V_FALSE); }

".AND."                 { return (L_AND); }
".and."                 { return (L_AND); }
".OR."                  { return (L_OR); }
".or."                  { return (L_OR); }
"&&"                    { return (L_AND); }
"||"                    { return (L_OR); }
"AND"                   { return (L_AND); }
"and"                   { return (L_AND); }
"OR"                    { return (L_OR); }
"or"                    { return (L_OR); }

".NOT."                 { return (L_NOT); }
".not."                 { return (L_NOT); }
"!"                     { return (L_NOT); }
"NOT"                   { return (L_NOT); }
"not"                   { return (L_NOT); }

".EQ."                  { return (L_EQ); }
".NE."                  { return (L_NEQ); }
".LE."                  { return (L_LEQ); }
".GE."                  { return (L_GEQ); }
".LT."                  { return (L_LT); }
".GT."                  { return (L_GT); }
"=="                    { return (L_EQ); }
"!="                    { return (L_NEQ); }
"<="                    { return (L_LEQ); }
">="                    { return (L_GEQ); }
"<"                     { return (L_LT); }
">"                     { return (L_GT); }
"<>"                    { return (L_NEQ); }

"+"                     { return (A_PLUS); }
"-"                     { return (A_MINUS); }
"*"                     { return (A_TIMES); }
"/"                     { return (A_DIV); }
"%"                     { return (A_MOD); }
"mod"                   { return (A_MOD); }
"MOD"                   { return (A_MOD); }
"div"                   { return (A_IDIV); }
"DIV"                   { return (A_IDIV); }
"^"                     { return (A_EXP); }
"**"                    { return (A_EXP); }

"|"                     { return (T_BAR); }
"("                     { return (T_LPAR); }
")"                     { return (T_RPAR); }
"{"                     { return (T_LBRA); }
"}"                     { return (T_RBRA); }
"["                     { return (T_LSQB); }
"]"                     { return (T_RSQB); }
"="                     { return (T_ASSIGN); }
":="                    { return (T_ASSIGN); }
"<-"                    { return (T_ASSIGN); }
","                     { return (T_COMMA); }
":"                     { return (T_COLON); }
";"                     { return (T_SEMI); }
"++"                    { return (T_PP); }
"--"                    { return (T_MM); }
"->"                    { return (T_RA); }
".."			{ return (T_DOTDOT); }

"backward"              { return (K_BACKWARD); }
"cancel"                { return (K_CANCEL); }
"else"                  { return (K_ELSE); }
"for"                   { return (K_FOR); }
"forward"               { return (K_FORWARD); }
"if"                    { return (K_IF); }
"include"               { return (K_INCLUDE); }
"loop"                  { return (K_LOOP); }
"macro"                 { return (K_MACRO); }
"module"                { return (K_MODULE); }
"NULL"                  { return (K_NULL); }
"off"                   { return (K_OFF); }
"on"                    { return (K_ON); }
"palindrome"            { return (K_PALINDROME); }
"pause"                 { return (K_PAUSE); }
"play"                  { return (K_PLAY); }
"quit"                  { return (K_QUIT); }
"repeat"                { return (K_REPEAT); }
"resume"                { return (K_RESUME); }
"sequence"              { return (K_SEQUENCE); }
"step"                  { return (K_STEP); }
"stop"                  { return (K_STOP); }
"suspend"               { return (K_SUSPEND); }
"then"                  { return (K_THEN); }
"until"                 { return (K_UNTIL); }
"while"                 { return (K_WHILE); }


"$ack"                  { return (P_ACK); }
"$bac"                  { return (P_BACKGROUND); }
"$dat"                  { return (P_DATA); }
"$err"                  { return (P_ERROR); }
"$for"                  { return (P_FOREGROUND); }
"$inf"                  { return (P_INFO); }
"$int"                  { return (P_INTERRUPT); }
"$lin"                  { return (P_LINQ); }
"$lre"                  { return (P_LRESP); }
"$mac"                  { return (P_MACRO); }
"$mes"                  { return (P_MESSAGE); }
"$sin"                  { return (P_SINQ); }
"$sre"                  { return (P_SRESP); }
"$sys"                  { return (P_SYSTEM); }
"$vin"                  { return (P_VINQ); }
"$vre"                  { return (P_VRESP); }


{AL}{ALN}*              {
	                    strcpy (yylval.ast.s, yytext);
	                    return (T_ID);
	                }

"?"                     |
"?"({ALN}|"?")+         |
{AL}({ALN}|"?")*        {
	                    /* $$$$$ Reduce the expression */
	                    strcpy (yylval.ast.s, yytext);
	                    return (T_EXID);
	                }

0[xX]{X}+               |
0{O}+                   |
{US}{D}+                {
	                    yylval.ast.i = strtol (yytext, NULL, 0);
	                    return (T_INT);
	                }

{US}{D}+"."{D}*({E})?   |
{US}{D}*"."{D}+({E})?   |
{US}{D}+{E}             {
	                    yylval.ast.f = (float) atof (yytext);
	                    return (T_FLOAT);
	                }

\"([^"\n\r]*(\\\")?[^"\n\r]*)*\"    {
	                    /* Strips off enclosing quotes */
	                    /* $$$$$ still need to process the string */
	                    yytext[strlen (yytext) - 1] = '\000';
	                    strcpy (yylval.ast.s, yytext+1);
	                    return (T_STRING);
	                }

"//"[^\n\r]*             	{
	                    strcpy (yylval.ast.s, yytext+2);
	                    return (T_COMMENT);
	                }
"#"[^\n\r]*                   {
	                    strcpy (yylval.ast.s, yytext+1);
	                    return (T_COMMENT);
	                }

"$"[^\n\r]*                   {
	                    strcpy (yylval.ast.s, "");
	                    return (T_COMMENT);
	                }
{ws}+                   { }

%%
int
yywrap()
{
    return (1);
}
