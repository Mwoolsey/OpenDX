%{

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

%}

%union
{
    union 
    {
	char                c;
	int                 i;
	float               f;
	char                s[4096];	/* 4096 == YYLMAX in net.lex */
    } ast;
};

/*
 * Assign precedences to logical and arithmetic symbols.
 */
%left   L_OR
%left   L_AND
%left   L_NOT
%left   L_EQ            L_NEQ
%left   L_LT            L_GT            L_LEQ           L_GEQ
%left   A_PLUS          A_MINUS
%left   A_TIMES         A_DIV           A_IDIV          A_MOD
%left   A_EXP

%left   U_MINUS         /* unary minus */

/*
 * Values:
 */
%token  V_TRUE
%token  V_FALSE

/*
 * Other Symbols:
 */
%token  T_BAR           /*      |       */
%token  T_LPAR          /*      (       */
%token  T_RPAR          /*      )       */
%token  T_LBRA          /*      {       */
%token  T_RBRA          /*      }       */
%token  T_LSQB          /*      [       */
%token  T_RSQB          /*      ]       */
%token  T_ASSIGN        /*      =       */
%token  T_COMMA         /*      ,       */
%token  T_COLON         /*      :       */
%token  T_SEMI          /*      ;       */
%token  T_PP            /*      ++      */
%token  T_MM            /*      --      */
%token  T_RA            /*      ->      */
%token  T_DOTDOT	/*      ..	*/
%token  T_COMMENT
%token  T_ID
%token  T_EXID
%token  T_INT
%token  T_STRING
%token  T_FLOAT

/*
 * Reserved Keywords:
 */
%token  K_BACKWARD
%token  K_CANCEL
%token  K_ELSE
%token  K_FOR
%token  K_FORWARD
%token  K_IF
%token  K_INCLUDE
%token  K_LOOP
%token  K_MACRO
%token  K_MODULE
%token  K_NULL
%token  K_OFF
%token  K_ON
%token  K_PALINDROME
%token  K_PAUSE
%token  K_PLAY
%token  K_QUIT
%token  K_REPEAT
%token  K_RESUME
%token  K_SEQUENCE
%token  K_STEP
%token  K_STOP
%token  K_SUSPEND
%token  K_THEN
%token  K_UNTIL
%token  K_WHILE



/*
 * Packet types:
 */
%token P_INTERRUPT
%token P_SYSTEM
%token P_ACK
%token P_MACRO
%token P_FOREGROUND
%token P_BACKGROUND
%token P_ERROR
%token P_MESSAGE
%token P_INFO
%token P_LINQ
%token P_SINQ
%token P_VINQ
%token P_LRESP
%token P_SRESP
%token P_VRESP
%token P_DATA

%type <ast> T_ID
%type <ast> argument 
%type <ast> argument_s 
%type <ast> argument_s0 
%type <ast> assignment 
%type <ast> attribute 
%type <ast> attribute_s 
%type <ast> attribute_s0 
%type <ast> attributes 
%type <ast> attributes_0 
%type <ast> block 
%type <ast> c_0 
%type <ast> comment 
%type <ast> complex 
%type <ast> constant 
%type <ast> empty 
%type <ast> expression 
%type <ast> expression_s 
%type <ast> f_assignment 
%type <ast> float 
%type <ast> formal
%type <ast> formal_s
%type <ast> formal_s0
%type <ast> function_call 
%type <ast> int 
%type <ast> include 
%type <ast> l_value
%type <ast> l_value_s 
%type <ast> list 
%type <ast> macro 
%type <ast> quaternion 
%type <ast> r_value 
%type <ast> real 
%type <ast> s_assignment 
%type <ast> scalar 
%type <ast> scalar_s 
%type <ast> start 
%type <ast> statement 
%type <ast> statement_s
%type <ast> statement_s0 
%type <ast> string 
%type <ast> string_s
%type <ast> tensor 
%type <ast> tensor_s 
%type <ast> top 
%type <ast> value 

%type <ast> T_LPAR
%type <ast> T_RPAR
%type <ast> T_LBRA
%type <ast> T_RBRA
%type <ast> T_SEMI
%type <ast> T_RSQB
%type <ast> T_LSQB
%type <ast> T_COMMENT
%type <ast> T_INT
%type <ast> T_FLOAT
%type <ast> T_STRING
%type <ast> K_NULL
%type <ast> K_INCLUDE

%%

start           :
	        top c_0
	        ;

c_0             : { return (0); } ;

top             : statement
	        | macro
	        | empty
	        ;

/*
 * Definitions:
 */

macro           : K_MACRO T_ID T_LPAR formal_s0 T_RPAR T_RA
			       T_LPAR formal_s0 T_RPAR
		  attributes_0
		  block
		{
		    ParseEndOfMacroDefinition();
		}
		| K_MACRO T_ID T_LPAR formal_s0 T_RPAR
		  attributes_0
		  block
		{
		    ParseEndOfMacroDefinition();
		}
	        ;

formal_s0	: empty
		| formal_s
		;

formal_s	: formal attributes_0
		| formal_s T_COMMA formal attributes_0
		;

formal		: T_ID
		| T_ID T_ASSIGN T_ID
		| T_ID T_ASSIGN constant
		| T_ID T_ASSIGN T_ID T_ASSIGN constant
		;

/*
 * Block:
 */

block           : T_LBRA statement_s0 T_RBRA
	        ;

/*
 * Statements:
 */

statement_s0    : empty
	        | statement_s
	        ;

statement_s     : statement
	        | statement_s statement
	        ;

/*
 * Put the LIST_CREATE here for the top level (for adding loop increments).
 */

statement       : block
	        | assignment    attributes_0 T_SEMI
	        | function_call attributes_0 T_SEMI
	        | include 
	        | comment
	        ;

/*
 * include
 */
include		: K_INCLUDE string 
		;


/*
 * Assignment
 */

assignment      : f_assignment
	        | s_assignment
	        ;

f_assignment    : l_value_s T_ASSIGN function_call
	        ;

s_assignment    : l_value_s T_ASSIGN expression_s
	        | T_ID T_PP
	        | T_ID T_MM
	        ;

/*
 * Attribute Structure:
 */

attributes_0    : empty
	        | attributes
	        ;

attributes      : T_LSQB attribute_s0 T_RSQB
	        ;

attribute_s0    : empty
	        | attribute_s
	        ;

attribute_s     : attribute
	        | attribute_s T_COMMA attribute
	        ;
/*
 * ddz
 */
attribute       : T_ID T_COLON string 
		    {
			ParseStringAttribute($1.s, $3.s);
		    }
	        ;

attribute       : T_ID T_COLON int 
		    {
			ParseIntAttribute($1.s, $3.i);
		    }
	        ;

/*
 * Function Call:
 */

function_call   : T_ID
	        {
	            ParseFunctionID($1.s);
	        }
		T_LPAR argument_s0 T_RPAR
	        ;

/*
 * Argument Structure:
 */

argument_s0     : empty
	        | argument_s
	        ;

argument_s      : argument
	        | argument_s T_COMMA argument
	        ;

argument        : value
	        | T_ID T_ASSIGN value
	        ;

value           : constant
	        {
	            ParseArgument($1.s,0);
	        }
	        | T_ID
	        {
	            ParseArgument($1.s,1);
	        }
	        ;

/*
 * Comment:
 */

comment         : T_COMMENT
	        {
	            ParseComment($1.s);
	        }
	        ;

/*
 * Arithmetic Expressions:
 */

expression_s    : expression
	        | expression_s T_COMMA expression
	        ;

expression      : constant
		| r_value
	        | T_LPAR expression T_RPAR
	        ;

/*
 * Definitions for constant data types:
 */

constant        : scalar
	        | tensor
	        | list
	        | string
		| K_NULL
	        ;

list            : T_LBRA scalar_s T_RBRA
	        | T_LBRA tensor_s T_RBRA
		| T_LBRA string_s T_RBRA
		| T_LBRA real T_DOTDOT real T_RBRA
		| T_LBRA real T_DOTDOT real T_COLON real T_RBRA
	        ;

tensor_s        : tensor
	        | tensor_s tensor
	        | tensor_s T_COMMA tensor
	        ;

tensor          : T_LSQB scalar_s T_RSQB
	        | T_LSQB tensor_s T_RSQB
	        ;

scalar_s        : scalar
	        | scalar_s scalar
	        | scalar_s T_COMMA scalar
	        ;

scalar          : real
	        | complex
	        | quaternion
	        ;

real            : int
	        | float
	        ;

complex         : T_LPAR int   T_COMMA int   T_RPAR
	        | T_LPAR float T_COMMA float T_RPAR
	        ;

quaternion      : T_LPAR int   T_COMMA int   T_COMMA int   T_COMMA int   T_RPAR
	        | T_LPAR float T_COMMA float T_COMMA float T_COMMA float T_RPAR
	        ;

int             : T_INT
	        ;

float           : T_FLOAT
	        ;

string_s	: string
		| string_s string
		| string_s T_COMMA string
		;

string          : T_STRING
	        ;

l_value_s       : l_value attributes_0
	        | l_value_s T_COMMA l_value attributes_0
	        ;

l_value		: T_ID
		{
		    ParseLValue($1.s);
		}
		;

r_value         : T_ID
		{
		    ParseRValue($1.s);
		}
	        ;

empty           :
		{
		}
	        ;

%%


#include "netlex.c"
