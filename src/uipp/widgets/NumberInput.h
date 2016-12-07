/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <ctype.h>	/*  issspace(), isdigit(), isalnum()  */
#include <stdlib.h>	/*  strtod()  */
#include <errno.h>	/*  errno, ERANGE  */
#include <X11/keysym.h>
#ifndef XK_MISCELLANY
#define XK_MISCELLANY 1
#endif

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
 *  Define locally used macros
 */
#define KeyValue(c) ((double)((c)-'0'))
#define IsExponentSymbol(c) (((c)=='e')||((c)=='E'))
#define DECIMAL_POINT '.'


/*
 *  Define structure used locally to handle a related set of parameters
 *  (These are provisional new values not installed until all tests passed)
 */
struct TestStruct {
    double value;
    short insert_position;
    short string_len;
    Boolean mantissa_negative;
    Boolean exponent_negative;
    char string[MAX_EDITOR_STRLEN];
};


/*
 *  Forward declarations of locally defined subroutines
 */
static void	Activate		(XmNumberWidget	nw,
					 XEvent *	event);
static void	ArmOrDisarm		(XmNumberWidget	nw,
					 XEvent *	event);
/*  Set up number widget for keybourd input.				      */
static void	Arm			(XmNumberWidget	nw,
					 XEvent *	event);
/*  Take number widget out of keyboard input mode			      */
static void	Disarm			(XmNumberWidget	nw,
					 XEvent *	event);
/*  Remove number widget from Xt grab list 				      */
static void	UnGrabKbd		(XmNumberWidget	nw);

/*  Register new value and deactivate keyboard input mode		      */
static void	ActivateAndDisarm	(XmNumberWidget	nw,
					 XEvent *	event);
/*  Call a help routine if one was registered				      */
static void	Help			(XmNumberWidget nw, XEvent* event);
static void	SelfInsert		(XmNumberWidget	nw,
					 XEvent *	event,
					 char **	params,
					 Cardinal *	num_params);
static void	DeleteForwardChar	(XmNumberWidget	nw,
					 XEvent *	event,
					 char **	params,
					 Cardinal*	num_params);
static void	DeleteBackwardChar	(XmNumberWidget	nw,
					 XEvent *	event,
					 char **	params,
					 Cardinal*	num_params);
static void	MoveBackwardChar	(XmNumberWidget	nw,
					 XEvent*	event,
					 char**		params,
					 Cardinal*	num_params);
static void	MoveForwardChar		(XmNumberWidget	nw,
					 XEvent*	event,
					 char**		params,
					 Cardinal*	num_params);
static void	MoveToLineEnd		(XmNumberWidget	nw,
					 XEvent*	event,
					 char**		params,
					 Cardinal*	num_params);
static void	MoveToLineStart		(XmNumberWidget	nw,
					 XEvent*	event,
					 char**		params,
					 Cardinal*	num_params);
/*  Check editor value against registered Number limits			      */
static Boolean	ValueWithinLimits	(XmNumberWidget nw);
/*  Generic move insert position					      */
static Boolean	MovePosition		(XmNumberWidget nw,
					 int		num_chars);
/*  Generic insertion into the editor string				      */
static Boolean	InsertChars		(XmNumberWidget nw,
					 char*		in_string,
					 int		num_chars);
/*  Generic deletion from the editor string				      */
static Boolean	DeleteChars		(XmNumberWidget	nw,
					 int		num_chars);
/*  Test string for acceptability and install in nw->editor if acceptable     */
static Boolean	TestParse		(XmNumberWidget nw,
					 char*		in_string,
					 int		insert_position);
/*  Report type of error						      */
static Boolean	DXEditError		(XmNumberWidget	nw,
					 char*		string,
					 int		msg,
				 	 int		char_place);

static void ReleaseKbd( XmNumberWidget nw, XEvent* event );


/*
 *  Define codes for the error types
 */
#define BadInput	0
#define NoNegative	1
#define InvalidChar	2
#define NoFloat		3
#define NoExponent	4
#define ExcessPrecision	5
#define StringEmpty	6
#define AboveMaximum	7
#define BelowMinimum	8


#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif
