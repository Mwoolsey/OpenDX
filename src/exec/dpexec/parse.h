/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/parse.h,v 1.6 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	_PARSE_H
#define	_PARSE_H

#include <dx/dx.h>
#include "exobject.h"
#include "utils.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
 * So that the exec's parser does not collide with any others that
 * may also be resident.
 */

#define	yyCharno	_dxd_exYYCharno
#define	yyLeng		_dxd_exYYLeng
#define	yyText		_dxd_exYYText
#define	yyact		_dxd_exYYact
#define	yychar		_dxd_exYYchar
#define	yycharno	_dxd_exYYcharno
#define	yychk		_dxd_exYYchk
#define	yydebug		_dxd_exYYdebug
#define	yydef		_dxd_exYYdef
#define	yyerrflag	_dxd_exYYerrflag
#define	yyexca		_dxd_exYYexca
#define	yyin		_dxd_exYYin
#define	yylineno	_dxd_exYYlineno
#define	yylval		_dxd_exYYlval
#define	yynerrs		_dxd_exYYnerrs
#define	yypact		_dxd_exYYpact
#define	yypgo		_dxd_exYYpgo
#define	yyps		_dxd_exYYps
#define	yypv		_dxd_exYYpv
#define	yypvt		_dxd_exYYpvt
#define	yyr1		_dxd_exYYr1
#define	yyr2		_dxd_exYYr2
#define	yyreds		_dxd_exYYreds
#define	yys		_dxd_exYYs
#define	yystate		_dxd_exYYstate
#define	yytmp		_dxd_exYYtmp
#define	yytoks		_dxd_exYYtoks
#define	yyv		_dxd_exYYv
#define	yyval		_dxd_exYYval

#define	yyerror		_dxf_ExYYError
#define	yygrabdata	_dxf_ExYYGrabData
#define yylex		_dxf_ExYYLex
#define yyparse		_dxf_ExYYParse

extern int yyparse();

#define	LIST_CREATE(_e)	      	if(_e) _e->last = _e;
#define	LIST_APPEND(_l, _e)\
{\
    if (!_l) {\
	_l       = _e;\
	_l->last = _l;\
    } else {\
        _e->prev       = _l->last;\
        _l->last->next = _e;\
        _l->last       = _e;\
    }\
}

typedef enum
{
    NT_MACRO,
    NT_MODULE,
    NT_ASSIGNMENT,
    NT_PRINT,
    NT_ATTRIBUTE,
    NT_CALL,
    NT_ARGUMENT,
    NT_LOGICAL,
    NT_ARITHMETIC,
    NT_CONSTANT,
    NT_ID,
    NT_EXID,
    NT_BACKGROUND,
    NT_PACKET,
    NT_DATA
} _ntype;


#define RUNTIMELOAD 0x8000   /* bit 32, set if module was loaded at run time */

typedef struct
{
    struct node			*id;		/* name			*/
    int				flags;		/* module flags		*/
    int				nin;		/* number of inputs	*/
    int				nout;		/* number of outputs	*/
    struct node			*in;		/* input  formals	*/
    int				varargs;	/* ... allowed		*/
    struct node			*out;		/* output formals	*/
    union {
        struct node		*stmt;		/* macro definition	*/
        PFI			func;		/* code pointer		*/
    } def;
    /* hidden inputs, before and/or after the list the user knows about */
    int				prehidden;      /* hidden inputs before */
    int				posthidden;     /* hidden inputs after  */
#if 0
    char			led[4];		/* 3 character led code	*/
#endif
    int				index;          /* ref number for macro */
} node_macro, node_module, node_function;


typedef enum
{
    LT_FOR,
    LT_REPEAT,
    LT_WHILE
} _ltype;


typedef struct
{
    _ltype			type;		/* loop type		*/
    struct node			*init;		/* initialization	*/
    struct node			*ttest;		/* top test		*/
    struct node			*stmt;		/* statement		*/
    struct node			*incr;		/* incr for next iteration */
    struct node			*btest;		/* bottom test		*/
} node_loop;


typedef struct
{
    struct node			*lval;		/* left  hand side	*/
    struct node			*rval;		/* right hand side	*/
} node_assignment;


typedef enum
{
    PT_LIST,			/* list things that match the names	*/
    PT_STRUCTURE,		/* describe the structure of things	*/
    PT_VALUE			/* give the value of things		*/
} _ptype;

typedef struct
{
    _ptype			type;		/* type of printing 	*/
    struct node			*val;		/* values to print	*/
} node_print;


typedef struct
{
    struct node			*id;
    struct node			*val;
} node_attribute;


typedef struct
{
    struct node			*id;		/* function name	*/
    struct node			*arg;		/* function args	*/
} node_call;


typedef struct
{
    struct node			*id;
    struct node			*val;
} node_argument;


typedef enum
{
    LO_LT,			/* logical ops				*/
    LO_GT,
    LO_LEQ,
    LO_GEQ,
    LO_EQ,
    LO_NEQ,
    LO_AND,
    LO_OR,
    LO_NOT,
    AO_EXP,			/* arithmetic ops			*/
    AO_TIMES,
    AO_DIV,
    AO_IDIV,
    AO_MOD,
    AO_PLUS,
    AO_MINUS,
    AO_NEGATE			/* unary minus				*/
} _op;


typedef struct
{
    _op				op;		/* operation		*/
    struct node			*lhs;		/* left  hand side	*/
    struct node			*rhs;		/* right hand side	*/
} node_logical, node_arithmetic, node_expression;

#define	LOCAL_SHAPE		4
#define	LOCAL_DATA		64

typedef struct
{
    struct node			*cnext;		/* constant resolution	*/
    struct node			*cprev;		/* chain pointers	*/

    Array			obj;		/* Object holding val	*/
    int				items;		/* number of items	*/
    Type			type;		/* type of each item	*/
    Category			cat;		/* the items category	*/
    int				rank;		/* its rank		*/
    int				nused;		/* data bytes used	*/
    int				nalloc;		/* data bytes available	*/
    int				salloc;		/* shape ints available	*/
    int				*shape;		/* shape pointer	*/
    Pointer			data;		/* data pointer		*/
    int				lshape[LOCAL_SHAPE];
    char			ldata [LOCAL_DATA];
    Pointer                     origin;
    Pointer                     delta;
} node_constant;


typedef struct
{
    char			*id;
    struct node			*dflt;		/* Object holding val	*/
} node_id;


typedef struct
{
    char			*id;
} node_exid;

typedef enum
{
    BG_STATEMENT,
    BG_LOOP,
    BG_CANCEL,
    BG_SUSPEND,
    BG_RESUME
} _bg;

typedef struct
{
    _bg				type;
    int				handle;
    struct node			*statement;
} node_background;

typedef enum
{
    PK_INTERRUPT,
    PK_SYSTEM,
    PK_ACK,
    PK_MACRO,
    PK_FOREGROUND,
    PK_BACKGROUND,
    PK_ERROR,
    PK_MESSAGE,
    PK_INFO,
    PK_LINQ,
    PK_SINQ,
    PK_VINQ,
    PK_LRESP,
    PK_SRESP,
    PK_VRESP,
    PK_DATA,
    PK_IMPORT,
    PK_IMPORTINFO
} _pack;

typedef struct
{
    _pack			type;
    int				number;
    struct node			*packet;
    int				size;
    Pointer			data;
} node_packet;

typedef struct
{
    int				len;
    Pointer			data;
} node_data;

/*
 * The top level node declaration.
 */
extern struct node	*_dxd_exParseTree; /* from yuiif.c */

typedef struct node
{
    struct exo_object		object;		/* object preamble	*/
    _ntype			type;
    struct node			*next;		/* next in list		*/
    struct node			*prev;		/* prev in list		*/
    struct node			*last;		/* last in list		*/
    struct node			*attr;		/* associated attributes*/
    union
    {
	node_function		function;	/* alias for the next 2	*/
	node_macro		macro;
	node_module		module;

	node_loop		loop;
	node_assignment		assign;
	node_call		call;
	node_print		print;
	node_attribute		attr;
	node_argument		arg;

	node_expression		expr;		/* alias for the next 2	*/
	node_logical		logic;
	node_arithmetic		arith;

	node_constant		constant;
	node_id			id;
	node_exid		exid;
	node_background		background;
	node_packet		packet;
	node_data		data;
    } v;
} node;


/*
 * Parsing Functions
 */

void	_dxf_ExPDestroyNode (node *n);

node	*_dxf_ExAppendConst	(node *list, node *elem);
node	*_dxf_ExPExtendConst	(node *n);

node	*_dxf_ExPDotDotList	(node *n1, node *n2, node *n3);

node	*_dxf_ExPCreateArgument (node *id, node *val);
node	*_dxf_ExPCreateArith	(_op op, node *lhs, node *rhs);
node	*_dxf_ExPCreateAssign	(node *lval, node *rval);
node	*_dxf_ExPCreateAttribute (node *id, node *val);
node	*_dxf_ExPCreateCall	(node *id, node *arg);
node	*_dxf_ExPCreateConst	(Type type, Category category,
					 int cnt, Pointer p);
node	*_dxf_ExPCreateData     (Pointer data, int len);
node	*_dxf_ExPCreateExid	(char *id);
node	*_dxf_ExPCreateId	(char *id);
node	*_dxf_ExPCreateLogical	(_op op, node *lhs, node *rhs);
node	*_dxf_ExPCreateLoop	(_ltype type, node *init, node *top,
				         node *stmt, node *incr, node *bot);
node	*_dxf_ExPCreateMacro	(node *id, node *in, node *out, 
					 node *stmt);
node	*_dxf_ExPCreateNode	(_ntype type);
node	*_dxf_ExPCreatePrint	(_ptype type, node *val);
node	*_dxf_ExPCreateBackground (int handle, _bg type, node *stmt);
node	*_dxf_ExPCreatePacket   (_pack type, int number, int size, 
					 node *packet);

node	*_dxf_ExPNegateConst (node *elem);

void	_dxf_ExEvaluateConstants (int);
void     _dxf_ExEvalConstant (node *n);

/* for starting a task on one or all processors.  private to the exec */
Error _dxf_ExRunOnAll (PFE func, Pointer arg, int size);
Error _dxf_ExRunOn (int JID, PFE func, Pointer arg, int size);

/*
 * Parsing Variables
 */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif	/* _PARSE_H */
