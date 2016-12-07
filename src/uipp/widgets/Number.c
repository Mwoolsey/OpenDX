/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


/*  Similar to a text widget, but only allows strings representing
 *  numerical values (base 10).
 *  (supports integer, fixed point, or floating point | scientific)
 *  IBM T.J. Watson Research Center	July 16, 1990
 */

/*
 */

/* Number resize logic:

	1. Default - XmNrecomputeSize = TRUE;  This means that the number 
	   widget will automatically resize itself based on the min and max
	   values of the widget, and the number of decimal places.  This was
	   done to prevent frequent resizes of steppers in the slider widget.

	2. XmNrecomputeSize = FALSE;  In this case, the widget sizes itself
	   based on the XmNcharPlaces resource.  This delimits the total size
	   of the widget.  This mode presents potentially unsightly operation
	   of the widget if the size in not adequate to handle the min/max
	   values of the widget
*/

#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif

#if defined(HAVE_TYPES_H)
#include <types.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#include <stdio.h>
#include <float.h>
#include <Xm/Xm.h>
#include <Xm/DrawP.h>
#include <Xm/GadgetP.h>
#include <math.h>	/* Define pow */
#include "FFloat.h"
#include "NumberP.h"
#include "Number.h"
#include "gamma.h"
#include "findcolor.h"

#if !defined(Max)
#define Max(a,b)  ((a>b)?a:b)
#endif

#define superclass (&widgetClassRec)

/*  Hidden routine in NumberOutput.c called from here			     */
extern void	SetNumberSize			(XmNumberWidget nw,
						 Dimension*	width,
						 Dimension*	height);
/*  Forward declare routines used in NumberInput.h  */
static void	UpdateNumberEditorDisplay	(XmNumberWidget	nw);
static void	DisplayNumberShadow		(XmNumberWidget	nw);
void SyncMultitypeData( MultitypeData* new, short type );

static
Boolean	UpdateMultitypeData (XmNumberWidget nw, MultitypeData* new, 
		MultitypeData* current);


/*
 *  Forward declaration of locally defined and used subroutines
 */
/*  Install action table stuff in class core_class record		      */
static void	ClassInitialize		();
/*  Initialize parameters and resources					      */
static void	Initialize		(XmNumberWidget	request,
					 XmNumberWidget	new);
/*  Create the window							      */
static void	Realize			(register XmNumberWidget nw,
					 Mask *p_valueMask,
					 XSetWindowAttributes *attributes);
/*  Redraw the borders and display					      */
static void	Redisplay		(XmNumberWidget	nw,
					 XEvent	*		event,
					 Region			region);
/*  Adjust locations after widget size has changed			      */
static void	Resize			(XmNumberWidget	nw);
/*  Reviews requested widget size and indicates Number's own preferrences    */
static XtGeometryResult
		PreferredSize		(XmNumberWidget	nw,
					 XtWidgetGeometry *	request,
					 XtWidgetGeometry *	preferred);
/*  Free all resources used by an instance				      */
static void	Destroy			(XmNumberWidget nw);
/*  Respond to changes in client accessible parameters			      */
static Boolean	SetValues		(XmNumberWidget	current,
					 XmNumberWidget	request,
					 XmNumberWidget	new);
/*  Grab a display server Graphics Context for printing the text string	      */
static void	GetNumberGC		(XmNumberWidget	nw);
/*  Draw cursor at insert position					      */
static void	DrawCursor		(XmNumberWidget nw,
					 int		x,
					 int		y,
					 int		type);


/*
 *  Load the code for the input actions
 *   (The subroutines must be local to be available as constants for
 *    preinitializing the class record structure).
 */
#include "NumberInput.h"

/*
 *  Define number keyboard-input specific translations
 */
/*  This translation table should look like that of the Text Widget  */
#if (XmVersion < 1001)
static char defaultNumberActionsBindings[] =
       "<Btn1Down>:		arm-or-disarm()\n\
	Ctrl<Key>Right:		forward-word()\n\
	Ctrl<Key>Left:		backward-word()\n\
	<Key>BackSpace:		delete-previous-character()\n\
	<Key>Delete:		delete-next-character()\n\
	<Key>Return:		activate-insert()\n\
	<Key>KP_Enter:		activate-insert()\n\
	<Key>Escape:		cancel-insert()\n\
	<Key>Right:		forward-character()\n\
	<Key>Left:		backward-character()\n\
	~Ctrl ~Meta <Key>:	self-insert()";
#else
static char defaultNumberActionsBindings[] =
       "<Btn1Down>:		arm-or-disarm()\n\
	Ctrl<Key>osfRight:	forward-word()\n\
	Ctrl<Key>osfLeft:	backward-word()\n\
	<Key>osfBackSpace:	delete-previous-character()\n\
	<Key>osfDelete:		delete-next-character()\n\
	<Key>Return:		activate-insert()\n\
	<Key>osfActivate:	activate-insert()\n\
	<Key>KP_Enter:		activate-insert()\n\
	<Key>Escape:		release-kbd() cancel-insert()\n\
	<Key>osfRight:		forward-character()\n\
	<Key>osfLeft:		backward-character()\n\
	~Ctrl ~Meta <Key>:	self-insert()";
#endif

/*  The actions should have the same string identifiers as those of Text  */
XtActionsRec defaultNumberActionsTable[] = {
    {"delete-previous-character",	(XtActionProc)DeleteBackwardChar},
    {"delete-next-character",		(XtActionProc)DeleteForwardChar},
    {"self-insert",			(XtActionProc)SelfInsert},
    {"activate-insert",			(XtActionProc)ActivateAndDisarm},
    {"PrimitiveParentActivate",		(XtActionProc)ActivateAndDisarm},
    {"arm-or-disarm",			(XtActionProc)ArmOrDisarm},
    {"cancel-insert",			(XtActionProc)Disarm},
    {"release-kbd",			(XtActionProc)ReleaseKbd},
    {"forward-character",		(XtActionProc)MoveForwardChar},
    {"backward-character",		(XtActionProc)MoveBackwardChar},
    {"backward-word",			(XtActionProc)MoveToLineStart},
    {"forward-word",			(XtActionProc)MoveToLineEnd},
    {"beginning-of-line",		(XtActionProc)MoveToLineStart},
    {"end-of-line",			(XtActionProc)MoveToLineEnd},
    {"Help",				(XtActionProc)Help}
};


/*
 *  Resource list for Number
 */

/*  Declare defaults that can be assigned through values */
#ifndef MAXINT
#define MAXINT 2147483647
#endif
#define DEF_IVAL 0
#define DEF_IMIN -MAXINT 
#define DEF_IMAX  MAXINT 

#define DOUBLE_MAX  DBL_MAX
#define DOUBLE_MIN -DBL_MAX
#define FLOAT_MAX   FLT_MAX
#define FLOAT_MIN  -FLT_MAX


/*  Declare defaults that can be assigned through pointers  */
#if !defined(aviion)
/* on the data general, these 'constants' are defined to be other
 *  variables which are set in some assembly file.  the point is they
 *  cannot be used as static initializers, so they will be initialized
 *  in the startup code below.
 */
static double DefaultMinDbl = DOUBLE_MIN;
static double DefaultMaxDbl = DOUBLE_MAX;
static float DefaultMinFlt = FLOAT_MIN;
static float DefaultMaxFlt = FLOAT_MAX;
#else
static double DefaultMinDbl;
static double DefaultMaxDbl;
static float DefaultMinFlt;
static float DefaultMaxFlt;
static int DefaultInitialized = FALSE;
#endif

static double DefaultValueDbl = 0.0;
static float DefaultValueFlt = 0.0;
static double DefaultCrossOver = 1000000.0;
static double DefaultCrossUnder = 0.0001;

static XtResource resources[] = 
{
   {
      XmNdValue, XmCDValue, XmRDouble, sizeof(double),
      XtOffset(XmNumberWidget, number.value.d),
      XmRDouble, (XtPointer) &DefaultValueDbl
   },
   {
      XmNfValue, XmCFValue, XmRFloat, sizeof(float),
      XtOffset(XmNumberWidget, number.value.f),
      XmRFloat, (XtPointer) &DefaultValueFlt
   },
   {
      XmNiValue, XmCIValue, XmRInt, sizeof(int),
      XtOffset(XmNumberWidget, number.value.i),
      XmRImmediate, (XtPointer) DEF_IVAL
   },
   {
      XmNdMinimum, XmCDMinimum, XmRDouble, sizeof(double),
      XtOffset(XmNumberWidget, number.value_minimum.d),
      XmRDouble, (XtPointer) &DefaultMinDbl
   },
   {
      XmNfMinimum, XmCFMinimum, XmRFloat, sizeof(float),
      XtOffset(XmNumberWidget, number.value_minimum.f),
      XmRFloat, (XtPointer) &DefaultMinFlt
   },
   {
      XmNiMinimum, XmCIMinimum, XmRInt, sizeof(int),
      XtOffset(XmNumberWidget, number.value_minimum.i),
      XmRImmediate, (XtPointer) DEF_IMIN
   },
   {
      XmNdMaximum, XmCDMaximum, XmRDouble, sizeof(double),
      XtOffset(XmNumberWidget, number.value_maximum.d),
      XmRDouble, (XtPointer) &DefaultMaxDbl
   },
   {
      XmNfMaximum, XmCFMaximum, XmRFloat, sizeof(float),
      XtOffset(XmNumberWidget, number.value_maximum.f),
      XmRFloat, (XtPointer) &DefaultMaxFlt
   },
   {
      XmNiMaximum, XmCIMaximum, XmRInt, sizeof(int),
      XtOffset(XmNumberWidget, number.value_maximum.i),
      XmRImmediate, (XtPointer) DEF_IMAX
   },
   {
      XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
      XtOffset(XmNumberWidget, number.font_list),
      XmRString, "Fixed"
   },
   {
      XmNcharPlaces, XmCCharPlaces, XmRShort, sizeof(short),
      XtOffset(XmNumberWidget, number.char_places),
      XmRImmediate, (XtPointer) 4
   },
   {
      XmNdecimalPlaces, XmCDecimalPlaces, XmRShort, sizeof(short),
      XtOffset(XmNumberWidget, number.decimal_places),
      XmRImmediate, (XtPointer) 0
   },
   {
      XmNdataType, XmCDataType, XmRShort, sizeof(short),
      XtOffset(XmNumberWidget, number.data_type),
      XmRImmediate, (XtPointer) INTEGER
   },
   {
      XmNoverflowCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffset(XmNumberWidget, number.overflow_callback),
      XmRPointer, (XtPointer) NULL
   },
   {
      XmNarmCallback, XmCArmCallback, XmRCallback,  sizeof(XtCallbackList),
      XtOffset(XmNumberWidget, number.arm_callback),
      XmRPointer, (XtPointer) NULL
   },
   {
      XmNactivateCallback, XmCActivateCallback, XmRCallback,
      sizeof(XtCallbackList),
      XtOffset(XmNumberWidget, number.activate_callback),
      XmRPointer, (XtPointer) NULL
   },
   {
      XmNdisarmCallback, XmCDisarmCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffset(XmNumberWidget, number.disarm_callback),
      XmRPointer, (XtPointer) NULL
   },
   {
      XmNwarningCallback, XmCCallback,XmRCallback,sizeof(XtCallbackList),
      XtOffset(XmNumberWidget, number.warning_callback),
      XmRPointer, (XtPointer) NULL
   },
   {
      XmNcenter, XmCCenter, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, number.center),
      XmRImmediate, (XtPointer) True
   },
   {
      XmNraised, XmCRaised, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, number.raised),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNvisible, XmCVisible, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, number.visible),
      XmRImmediate, (XtPointer) True
   },
   {
      XmNeditable, XmCEditable, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, number.allow_key_entry_mode),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNnoEvents, XmCNoEvents, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, number.propogate_events),
      XmRImmediate, (XtPointer) False
   },
   {
      XmNrecomputeSize, XmCRecomputeSize, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, number.recompute_size),
      XmRImmediate, (XtPointer) TRUE
   },
   {
      XmNfixedNotation, XmCFixedNotation, XmRBoolean, sizeof(Boolean),
      XtOffset(XmNumberWidget, editor.is_fixed),
      XmRImmediate, (XtPointer) True
   },
   {
      XmNcrossOver, XmCCrossOver, XmRDouble, sizeof(double),
      XtOffset(XmNumberWidget, number.cross_over),
      XmRDouble, (XtPointer) &DefaultCrossOver
   },
   {
      XmNcrossUnder, XmCCrossUnder, XmRDouble, sizeof(double),
      XtOffset(XmNumberWidget, number.cross_under),
      XmRDouble, (XtPointer) &DefaultCrossUnder
   },
};


/*
 *  The Number class record definition
 */
XmNumberWidgetClassRec xmNumberWidgetClassRec =
{
    /*
     *  CoreClassPart:
     */
    {
      (WidgetClass) &xmPrimitiveClassRec,  /* superclass	    */
      "XmNumber",			   /* class_name	    */
      sizeof(XmNumberWidgetRec),	   /* widget_size	    */
      ClassInitialize,			   /* class_initialize      */
      NULL,				   /* class_part_initialize */
      FALSE,				   /* class_inited	    */
      (XtInitProc)Initialize,		   /* initialize	    */
      NULL,				   /* initialize_hook	    */
      (XtRealizeProc)Realize,		   /* realize		    */
      defaultNumberActionsTable,	   /* actions		    */
      XtNumber(defaultNumberActionsTable), /* num_actions    	    */
      resources,			   /* resources		    */
      XtNumber(resources),		   /* num_resources	    */
      NULLQUARK,			   /* xrm_class		    */
      TRUE,				   /* compress_motion	    */
      FALSE,				   /* compress_exposure	    */
      TRUE,				   /* compress_enterleave   */
      FALSE,				   /* visible_interest	    */	
      (XtWidgetProc) Destroy,		   /* destroy		    */	
      (XtWidgetProc) Resize, 		   /* resize		    */
      (XtExposeProc) Redisplay,		   /* expose		    */	
      (XtSetValuesFunc) SetValues,	   /* set_values	    */	
      NULL,				   /* set_values_hook 	    */
      XtInheritSetValuesAlmost,		   /* set_values_almost	    */
      NULL,				   /* get_values_hook	    */
      NULL,				   /* accept_focus	    */	
      XtVersion,			   /* version		    */
      NULL,				   /* callback private	    */
      defaultNumberActionsBindings,	   /* tm_table		    */
      (XtGeometryHandler)PreferredSize,	   /* query_geometry	    */
      NULL,				   /* display_accelerator   */
      NULL,				   /* extension		    */
    },
    /*
     *  XmPrimitiveClassPart:
     */
    {
      (XtWidgetProc)_XtInherit,		   /* Border highlight	    */
      (XtWidgetProc)_XtInherit,		   /* Border_unhighlight    */
#if (XmVersion >= 1001)
      XtInheritTranslations,     		/* translations           */
#else
      (XtTranslations)_XtInherit,     		/* translations           */
#endif
      NULL,				   /* Arm_and_activate	    */
      NULL,				   /* Get_resources	    */
      0,				   /* Num_get_resources	    */
      NULL,		 		   /* Extension		    */
    },
    /*
     *  XmStepperClassPart:
     */
    {
      NULL,	 			   /* extension		    */
    }
};


WidgetClass xmNumberWidgetClass = (WidgetClass)&xmNumberWidgetClassRec;


/*  Subroutine:	ClassInitialize
 *  Purpose:	Install non-standard type converters needed by this widget class
 */
static void ClassInitialize()
{
#if defined(aviion)
    if (!DefaultInitialized) {
	DefaultMinDbl = -DBL_MAX;
	DefaultMaxDbl = DBL_MAX;
	DefaultMinFlt = -FLT_MAX;
	DefaultMaxFlt = FLT_MAX;
	DefaultInitialized = TRUE;
    }
#endif

    /*  Install converters for type XmRDouble and XmRFloat  */
    XmAddFloatConverters();
}

/* Replace private obsolete Xm functions with our own copies */
static Boolean DifferentBackground(
        Widget w,
        Widget parent )
{
    if (XmIsPrimitive (w) && XmIsManager (parent)) {
        if (w->core.background_pixel != parent->core.background_pixel ||
            w->core.background_pixmap != parent->core.background_pixmap)
            return (True);
    }
 
    return (False);
}
 
static void HighlightBorder(
        Widget w )
{
    if(    XmIsPrimitive( w)    ) {
        (*(xmPrimitiveClassRec.primitive_class.border_highlight))( w) ;
    }  else  {
        if(    XmIsGadget( w)    ) {  
            (*(xmGadgetClassRec.gadget_class.border_highlight))( w) ;
        } 
    }
    return ; 
}
 
static void UnhighlightBorder( 
        Widget w ) 
{
    if(    XmIsPrimitive( w)    )
    {
        (*(xmPrimitiveClassRec.primitive_class.border_unhighlight))( w) ;
        }
    else
    {   if(    XmIsGadget( w)    )
        {
            (*(xmGadgetClassRec.gadget_class.border_unhighlight))( w) ;
            }
        }
    return ;
    }




/*  Subroutine:	Activate
 *  Effect:	Processes "activate" and "disarm" type actions in the stepper
 *		widget field that might belong to its gadgets.
 */
/* ARGSUSED */
static void Activate( XmNumberWidget nw, XEvent* event )
{
    XmDoubleCallbackStruct call_value;

    /*  Register the new value if in editor mode  */
    if( nw->number.key_entry_mode && ValueWithinLimits(nw) )
    {
	nw->number.value.d = nw->editor.value;
	if (nw->number.data_type == FLOAT)
	    nw->number.value.f = (float)nw->number.value.d;
	if (nw->number.data_type == INTEGER)
	{
	    if( nw->number.value.d >= 0.0 )
		nw->number.value.i = (int)(nw->number.value.d + 0.5);
	    else
		nw->number.value.i = (int)(nw->number.value.d - 0.5);
	}
    }
    if( XtHasCallbacks((Widget)nw, XmNactivateCallback)
        == XtCallbackHasSome )
    {
	call_value.value = nw->number.value.d;
	call_value.reason = XmCR_ACTIVATE;
	XtCallCallbacks((Widget)nw, XmNactivateCallback, &call_value);
    }
}


/*  Subroutine:	ArmOrDisarm
 *  Purpose:	Toggle arm/disarm with button clicking
 */
static void ArmOrDisarm( XmNumberWidget nw, XEvent* event )
{
    if( nw->number.key_entry_mode ) {
	UnGrabKbd(nw);
	Disarm(nw, event);
    } else
	Arm(nw, event);
}


/*  Subroutine:	Arm
 *  Purpose:	Set up number widget for keybourd input.
 */
static void Arm( XmNumberWidget nw, XEvent* event )
{
    XmDoubleCallbackStruct call_value;
    Boolean		   is_fixed;
#if (XmVersion > 1001)
    Window                 window;
#endif


    if( (!nw->number.visible) || (!nw->number.allow_key_entry_mode ) )
	return;
#if (XmVersion >= 1001)
    XmProcessTraversal((Widget)nw, XmTRAVERSE_CURRENT);
#if (XmVersion > 1001)
    window = XtWindow(nw);
    XSetInputFocus(XtDisplay(nw),window,RevertToPointerRoot,CurrentTime);
#endif
    XtAddGrab((Widget)nw, True, True);
#else
    _XmGrabTheFocus(nw);
#endif
    if( (XtHasCallbacks((Widget)nw, XmNarmCallback) == XtCallbackHasSome) )
    {
	call_value.value = nw->number.value.d;
	call_value.reason = XmCR_ARM;
	XtCallCallbacks((Widget)nw, XmNarmCallback, &call_value);
    }
    /*  Perform all initializations to simplify synchronization issues  */
    is_fixed = nw->editor.is_fixed;
    (void)memset(&(nw->editor), 0, sizeof(XmNumberEditorPart));
    nw->editor.is_fixed = is_fixed;
    if( nw->number.decimal_places == 0 ) 
	{
	nw->editor.is_float = False;
	} 
    else 
	{
	nw->editor.is_float = True;
	if( nw->number.decimal_places > 0 ) 
	    {
	    nw->editor.decimal_minimum =
	      pow(0.1, (double)(nw->number.decimal_places));
	    } 
	}
    if( nw->number.value_minimum.d < 0.0 ) {
	nw->editor.is_signed = True;
    }
    nw->number.key_entry_mode = True;
    if( nw->primitive.shadow_thickness > 0 )
	DisplayNumberShadow(nw);
    UpdateNumberEditorDisplay(nw);
}

/*
 * release the keyboard
 * split out from Disarm 2/15/95 because you don't always have it grabbed
 * when calling Disarm()
 */
static void ReleaseKbd( XmNumberWidget nw, XEvent* event ) 
{ if( nw->number.key_entry_mode ) UnGrabKbd(nw); }
static void UnGrabKbd( XmNumberWidget nw)
{
#if (XmVersion >= 1001)
    XtRemoveGrab((Widget)nw);
#if (XmVersion > 1001)
    XSetInputFocus(XtDisplay(nw),PointerRoot,RevertToPointerRoot,CurrentTime);
#endif
#endif
}

/*  Subroutine:	Disarm
 *  Purpose:	Take number widget out of keyboard input mode
 */
static void Disarm( XmNumberWidget nw, XEvent* event )
{
    XmDoubleCallbackStruct call_value;


    if( nw->number.key_entry_mode ) {
	nw->number.key_entry_mode = False;
	if( nw->primitive.shadow_thickness > 0 )
	    DisplayNumberShadow(nw);
	XmShowNumberValue(nw);
	if( (XtHasCallbacks((Widget)nw, XmNdisarmCallback)
	     == XtCallbackHasSome) )
	{
	    call_value.value = nw->number.value.d;
	    call_value.reason = XmCR_DISARM;
	    XtCallCallbacks((Widget)nw, XmNdisarmCallback, &call_value);
	}
    }
}


/*  Subroutine:	ActivateAndDisarm
 *  Purpose:	Register new value and deactivate keyboard input mode
 */
static void ActivateAndDisarm( XmNumberWidget nw, XEvent* event )
{
    if( nw->number.key_entry_mode && ValueWithinLimits(nw) ) {
	UnGrabKbd(nw);
	Activate(nw, event);
	Disarm(nw, event);
    }
}


/* Subroutine:	Help
 * Purpose:	Call the application registered help callbacks
 * Note:	Uses Xm efficient alternative extensions to Xt (XtWidget...)
 */
static void Help( XmNumberWidget nw, XEvent* event )
{
    XmAnyCallbackStruct call_value;

    if( XtHasCallbacks((Widget)nw, XmNhelpCallback) == XtCallbackHasSome )
    {
	call_value.reason = XmCR_HELP;
	call_value.event = event;
	XtCallCallbacks((Widget)nw, XmNhelpCallback, &call_value);
    }
}


/*  Subroutine:	SelfInsert
 *  Purpose:	Insert characters as they are typed from keyboard input
 *		(In response to KeyPress events)
 */
/* ARGSUSED */
static void SelfInsert
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
#ifdef EXPLAIN
     XmNumberWidget nw;
     XEvent *event;
     char **params;
     Cardinal *num_params;
#endif
{
    KeySym keysym;			/* X code for key that was struck    */
    XComposeStatus compose;
    int num_chars;
    char string[2*MAX_EDITOR_STRLEN];	/* buffer to recieve ASCII code	     */

    if( nw->number.key_entry_mode ) {
	num_chars = XLookupString((XKeyEvent *)event, string, sizeof(string),
				  &keysym, &compose);
	/*  Trivially omit modifiers and multi-char entries, then try input  */
	/* ((keysym & 0xff00) == 0xff00) */
	if( (!IsFunctionKey(keysym)) && (!IsMiscFunctionKey(keysym)) &&
	    (!IsPFKey(keysym)) && (!IsModifierKey(keysym)) &&
	    (InsertChars(nw, string, num_chars)) )
	    UpdateNumberEditorDisplay(nw);
    }
}


/* ARGSUSED */
static void DeleteForwardChar
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
{
    if( nw->number.key_entry_mode ) {
	if( DeleteChars(nw, 1) )
	    UpdateNumberEditorDisplay(nw);
    }
}


/* ARGSUSED */
static void DeleteBackwardChar
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
{
    if( nw->number.key_entry_mode ) {
	if( DeleteChars(nw, -1) )
	    UpdateNumberEditorDisplay(nw);
    }
}


/* ARGSUSED */
static void MoveBackwardChar
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
{
    if( nw->number.key_entry_mode && MovePosition(nw, -1) )
	UpdateNumberEditorDisplay(nw);
}


/* ARGSUSED */
static void MoveForwardChar
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
{
    if( nw->number.key_entry_mode && MovePosition(nw, 1) )
	UpdateNumberEditorDisplay(nw);
}


/* ARGSUSED */
static void MoveToLineStart
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
{
    if( nw->number.key_entry_mode && MovePosition(nw, -nw->editor.string_len) )
	UpdateNumberEditorDisplay(nw);
}


/* ARGSUSED */
static void MoveToLineEnd
  ( XmNumberWidget nw, XEvent *event, char **params, Cardinal *num_params )
{
    if( nw->number.key_entry_mode && MovePosition(nw, nw->editor.string_len) )
	UpdateNumberEditorDisplay(nw);
}


/*  Subroutine:	ValueWithinLimits
 *  Purpose:	Check the editor value to verify that it is within the limits
 */
static Boolean ValueWithinLimits( XmNumberWidget nw )
{
     if (nw->number.data_type == INTEGER)
        {
	if( nw->editor.value < nw->number.value_minimum.i )
	    return DXEditError(nw, nw->editor.string, BelowMinimum, -1);
	else if( nw->editor.value > nw->number.value_maximum.i )
	    return DXEditError(nw, nw->editor.string, AboveMaximum, -1);
	else
	    return TRUE;
        }
    if (nw->number.data_type == FLOAT)
        {
	if( nw->editor.value < nw->number.value_minimum.f )
	    return DXEditError(nw, nw->editor.string, BelowMinimum, -1);
	else if( nw->editor.value > nw->number.value_maximum.f )
	    return DXEditError(nw, nw->editor.string, AboveMaximum, -1);
	else
	    return TRUE;
        }
    if (nw->number.data_type == DOUBLE)
        {
	if( nw->editor.value < nw->number.value_minimum.d )
	    return DXEditError(nw, nw->editor.string, BelowMinimum, -1);
	else if( nw->editor.value > nw->number.value_maximum.d )
	    return DXEditError(nw, nw->editor.string, AboveMaximum, -1);
	else
	    return TRUE;
        }
    return FALSE;
}


/*  Subroutine:	MovePosition
 *  Purpose:	Generic routine to move the insert position
 */
static Boolean MovePosition( XmNumberWidget nw, int num_chars )
{
    int old_position = nw->editor.insert_position;
    nw->editor.insert_position += num_chars;
    if( nw->editor.insert_position < 0 )
	nw->editor.insert_position = 0;
    else if( nw->editor.insert_position > nw->editor.string_len )
	nw->editor.insert_position = nw->editor.string_len;
    if( nw->editor.insert_position != old_position )
	return TRUE;
    else
	return FALSE;
}


/*  Subroutine:	InsertChars
 *  Purpose:	Generic routine to insert a character or string into the
 *		Number.Editor string.
 *  Note:	Leading and trailing spaces are stripped from the inserted
 *		string.
 */
static Boolean InsertChars( XmNumberWidget nw, char* in_string, int num_chars )
{
    char temp_string[MAX_EDITOR_STRLEN];
    int old, new, temp, insert;
    int insert_position;

    if (nw->editor.string_len + num_chars > MAX_EDITOR_STRLEN) {
	XBell(XtDisplay(nw), 0);
	return False;
    }

    for( old=0; old<nw->editor.insert_position; old++ )
	temp_string[old] = nw->editor.string[old];
    for( new=0; isspace(in_string[new]); new++ );
    for( insert=0, temp=old;
	   (new < num_chars)
	&& (in_string[new] != '\0')
	&& (!isspace(in_string[new]));
	 new++, temp++, insert++ )
	temp_string[temp] = in_string[new];
    for( ; old<nw->editor.string_len; old++, temp++ )
	temp_string[temp] = nw->editor.string[old];
    temp_string[temp] = '\0';
    insert_position = nw->editor.insert_position + insert;
    return TestParse(nw, temp_string, insert_position);
}


/*  Subroutine:	DeleteChars
 *  Purpose:	Generic routine to remove consecutive characters from the
 *		Number.Editor string.
 *  Note:	Positive num_chars deletes from the insert_position forward.
 *		Negative num_chars deletes before the insert_position.
 */
static Boolean DeleteChars( XmNumberWidget nw, int num_chars )
{
    int i, j;
    int start_position, end_position, insert_position;

    char temp_string[MAX_EDITOR_STRLEN];
    if( num_chars > 0 )
    {
	start_position = nw->editor.insert_position;
	end_position = start_position + num_chars;
    }
    else if( num_chars < 0 )
    {
	end_position = nw->editor.insert_position;
	start_position = end_position + num_chars;
    }
    else
	/*  No deletion requested  */
	return FALSE;
    /*  If string is already empty, just beep  */
    if( nw->editor.string_len <= 0 )
	return DXEditError(nw, nw->editor.string, StringEmpty, -1);
    if( start_position < 0 )
	start_position = 0;
    if( end_position > nw->editor.string_len )
	end_position = nw->editor.string_len;
    if( start_position >= end_position )
	return FALSE;
    for( i=0; i<start_position; i++ )
	temp_string[i] = nw->editor.string[i];
    for( j=end_position; j<nw->editor.string_len; j++, i++ )
	temp_string[i] = nw->editor.string[j];
    temp_string[i] = '\0';
    if( num_chars < 0 )
	insert_position = Max(0, nw->editor.insert_position + num_chars);
    else
	insert_position = nw->editor.insert_position;
    if( temp_string[0] == '\0' )
    {
	nw->editor.string_len = 0;
	nw->editor.insert_position = 0;
	nw->editor.string[0] = '\0';
	nw->editor.value = 0.0;
    } else
	return TestParse(nw, temp_string, insert_position);
    return FALSE;
}


/*  Subroutine:	TestParse
 *  Purpose:	Parse through the string checking for specific errors
 *		and install in nw->editor if no errors found.
 *  Returns:	TRUE if new string was installed, else FALSE
 */
static Boolean
  TestParse( XmNumberWidget nw, char* in_string, int insert_position )
{
    int i, j;
    struct TestStruct edit;

    edit.insert_position = insert_position;

    /* If the leading char is a '.', prepend a zero */
    if( in_string[0] == '.' ) {
	for (i=strlen(in_string); i >= 0; i--)
	    {
	    in_string[i+1] = in_string[i];
	    }
	in_string[0] = '0';
	insert_position++;
    }

    /*  Initialize input string and remove leading spaces  */
    for( i=0; isspace(in_string[i]); i++ )
	--insert_position;
    /*  Parse for a sign  */
    if( !isdigit(in_string[i]) )
    {
	if( in_string[i] == '-' ) {
	    if( nw->editor.is_signed )
		edit.mantissa_negative = TRUE;
	    else
		return DXEditError(nw, in_string, NoNegative, i);
	} else if( in_string[i] == '+' ) {
	    --insert_position;
	    edit.mantissa_negative = FALSE;
	} else
	    return DXEditError(nw, in_string, InvalidChar, i);
	i = 1;
    }
    else
	edit.mantissa_negative = FALSE;
    /*  Initialize output string and handle mantissa sign indication  */
    if( nw->editor.is_signed && edit.mantissa_negative ) {
	edit.string[0] = '-';
	j = 1;
    } else
	j = 0;
    /*  Parse through the mantissa integer part  */
    while( isdigit(in_string[i]) ) {
	edit.string[j] = in_string[i];
	++j;
	++i;
    }
    /*  Parse for mantissa fractional part  */
    if( in_string[i] == DECIMAL_POINT )
    {
	double decimal = 0.1;
	int unwarned = 1;

	if( !nw->editor.is_float )
	    return DXEditError(nw, in_string, NoFloat, i);
	/*  Copy over the decimal 'point'  */
	edit.string[j] = in_string[i];
	++i;
	++j;
	/*  Parse the mantissa fraction  */
	while( isdigit(in_string[i]) ) {
	    if( nw->editor.is_fixed
	       && (decimal < nw->editor.decimal_minimum) ) {
		/*  Omit all excess, but give warning only on first case  */
		if( unwarned )
		    DXEditError(nw, in_string, ExcessPrecision, i);
		/*  Realign position with shortened string  */
		if( insert_position > i )
		    --edit.insert_position;
	    } else {
		edit.string[j] = in_string[i];
		j++;
	    }
	    decimal *= 0.1;
	    ++i;
	}
    }
    /*  Parse for Exponent  */
    if( IsExponentSymbol(in_string[i]) )
    {
	if( nw->editor.is_fixed )
	    return DXEditError(nw, in_string, NoExponent, i);
	edit.string[j] = in_string[i];
	j++;
	i++;
	/*  Check for exponent sign or other garbage  */
	if( !isdigit(in_string[i]) )
	{
	    if( in_string[i] == '-' ) {
		if( !nw->editor.is_float )
		    return DXEditError(nw, in_string, NoFloat, i);
		edit.string[j] = '-';
		j++;
		i++;
	    } else if( in_string[i] == '+' ) {
		++i;
	    } else if( in_string[i] == '\0' ) {
	    } else
		return DXEditError(nw, in_string, InvalidChar, i);
	}
	/*  Parse exponent value (or initialize to 0  */
	if( isdigit(in_string[i]) ) {
	    while( isdigit(in_string[i]) ) {
		edit.string[j] = in_string[i];
		++j;
		++i;
	    }
	    edit.string[j] = '\0';
	} else {
	    edit.string[j] = '0';
	    edit.string[j+1] = '\0';
	}
    }
    else
	edit.string[j] = '\0';
    /*  Make sure that string ends reasonably  */
    if( (!isspace(in_string[i])) && (in_string[i] != '\0') )
	return DXEditError(nw, in_string, InvalidChar, i);
    edit.string_len = j;
    if( insert_position < 0 )
	edit.insert_position = 0;
    else if( insert_position > j )
	edit.insert_position = j;
    else
	edit.insert_position = insert_position;
    edit.value = strtod(edit.string, NULL);
    if( errno == ERANGE )
	return DXEditError(nw, in_string, BadInput, -1);
    /*  Check limits if subsequent entries won't bring us back into range  */
    if( nw->editor.is_fixed ) 
    {
	if( edit.value < 0.0 ) 
	{
	    if( edit.value < nw->number.value_minimum.d )
		return DXEditError(nw, in_string, BelowMinimum, -1);
	} 
	else 
	{
	    if( nw->number.value_maximum.d > 0.0 )
		if( edit.value > nw->number.value_maximum.d )
		    return DXEditError(nw, in_string, AboveMaximum, -1);
	}
    }
    (void)strcpy(nw->editor.string, edit.string);
    nw->editor.insert_position = edit.insert_position;
    nw->editor.value = edit.value;
    nw->editor.string_len = edit.string_len;
    return TRUE;
}



/*  Subroutine:	DXEditError
 *  Purpose:	Centralized handling of parsing errors to provide meaningful
 *		feedback.
 */
static Boolean
  DXEditError( XmNumberWidget nw, char* string, int msg, int char_place )
{
    char message[100];
    XmNumberWarningCallbackStruct call_value;

    /*
     * Don't generate error messages for now...
     */
    switch(msg)
    {
      case NoNegative:
	call_value.reason = XmCR_NUM_WARN_NO_NEG;
	(void)sprintf(message, "Negative value not permitted.");
	break;

      case InvalidChar:
#ifdef JUST_BELL
{
    int place;
	call_value.reason = XmCR_NUM_WARN_INVALID_CHAR;
	place = sprintf(message, "Invalid character.");
	if( char_place >= 0 ) {
	    if( isalnum(string[char_place]) )
		(void)sprintf(&message[place - 1], ": '%c'\n",
			      string[char_place]);
	    else
		(void)sprintf(&message[place - 1], ": <%o>\n",
			      string[char_place]);
	}
}
#endif
	XBell(XtDisplay(nw), 0);
	return FALSE;
	break;

      case StringEmpty:
	 return FALSE;
	 break;

      case NoFloat:
	call_value.reason = XmCR_NUM_WARN_NO_FLOAT;
	(void)sprintf(message, "Value must be integer.");
	break;

      case NoExponent:
	call_value.reason = XmCR_NUM_WARN_NO_EXPONENT;
	(void)sprintf(message, "Exponential notation not permitted.");
	break;

      case ExcessPrecision:
	call_value.reason = XmCR_NUM_WARN_EXCESS_PRECISION;
	(void)sprintf(message, 
		      "Precision of entered value exceeds decimal places: %d",
                      nw->number.decimal_places);
	break;

      case AboveMaximum:
	call_value.reason = XmCR_NUM_WARN_ABOVE_MAX;
	(void)sprintf(message, "Value is greater than the maximum: %g.",
		      nw->number.value_maximum.d);
	break;

      case BelowMinimum:
	call_value.reason = XmCR_NUM_WARN_BELOW_MIN;
	(void)sprintf(message, "Value is less than the minimum: %g.",
		      nw->number.value_minimum.d);
	break;

      case BadInput:
	call_value.reason = XmCR_NUM_WARN_BAD_INPUT;

      default:
	(void)sprintf(message, "Input not parseable.");
	break;
    }
    call_value.message = message;

    XBell(XtDisplay(nw), 0);
    UnGrabKbd(nw);
    Disarm(nw, NULL);
    XtCallCallbacks((Widget)nw, XmNwarningCallback, &call_value);
    return FALSE;
}



/*  Subroutine:	Initialize
 *  Purpose:	Initialize number resources and internal parameters.
 */
static void Initialize( XmNumberWidget request, XmNumberWidget new )
{
    Dimension width, height;

    /*  Clear things needing to be clear  */
    new->number.timer = 0;
    new->number.key_entry_mode = 0;
    new->number.last_char_cnt = 0;
    /*  Set the value parameters  */
    SyncMultitypeData(&new->number.value, new->number.data_type);
    SyncMultitypeData(&new->number.value_minimum, new->number.data_type);
    SyncMultitypeData(&new->number.value_maximum, new->number.data_type);
    (void)UpdateMultitypeData(new, &new->number.value, &request->number.value);
    (void)UpdateMultitypeData(new, &new->number.value_minimum,
			      &request->number.value_minimum);
    (void)UpdateMultitypeData(new, &new->number.value_maximum,
			      &request->number.value_maximum); 
    /*  Set the fontinfo and GC  */
    new->number.font_struct = NULL;
    GetNumberGC(new);

    if (new->number.decimal_places < 0)
	new->editor.is_fixed = False;
    /*  Calculate best sizes and impose them only if needed  */
    SetNumberSize(new, &width, &height);
    if( width > new->core.width )
	new->core.width = width;
    if( height > new->core.height )
	new->core.height = height;
    Resize(new);
    new->number.initialized = True;
#if (XmVersion < 1001)
    XmAddTabGroup(new);
#endif
    new->number.cross_under = pow(0.1,(double)new->number.decimal_places);
}


/*  Subroutine: Realize
 *  Purpose:	Create the widget's window with a special twist for the Number
 *		widget.
 *  Note:	Based on XmPrimitive's Realize (which lets bit gravity default
 *		to Forget) but with the DontPropogate switchable.
 */
static void Realize( register XmNumberWidget nw, Mask *p_valueMask,
		     XSetWindowAttributes *attributes )
{
    register Mask eventMask, valueMask;

    valueMask = *p_valueMask;
    eventMask = ButtonPressMask | ButtonReleaseMask
	| KeyPressMask | KeyReleaseMask | PointerMotionMask;
    if( nw->number.propogate_events == FALSE )
    {
	valueMask |= CWDontPropagate;
	attributes->do_not_propagate_mask = eventMask;
    }
    else
    {
	/*  I don't care what Xt expects, I don't want these events!  */
	attributes->event_mask &= ~eventMask;
    }
    XtCreateWindow((Widget)nw, InputOutput, CopyFromParent, valueMask, attributes);
}


/*  Subroutine:	Redisplay
 *  Purpose:	General redisplay function called on exposure events.
 */
/* ARGSUSED */
static void Redisplay( XmNumberWidget nw, XEvent *event, Region region )
{
    /*  Draw the frame  */
    if( nw->primitive.shadow_thickness > 0 )
	DisplayNumberShadow(nw);
    if( nw->primitive.highlight_thickness > 0 )
    {
	/*  Do (or undo) any highlighting  */
	if( nw->primitive.highlighted )
	    HighlightBorder((Widget)nw);
	else if( DifferentBackground((Widget)nw, XtParent(nw)) )
	    UnhighlightBorder((Widget)nw);
    }
    /*  Finally, print the value  */
    if( nw->number.visible )
    {
	if( nw->number.key_entry_mode )
	    UpdateNumberEditorDisplay(nw);
	else
	    XmShowNumberValue(nw);
    }
}


/*  Subroutine:	DisplayNumberShadow
 *  Purpose:	Render the shadowed frame appropriate for the occasion
 */
static
void DisplayNumberShadow( XmNumberWidget nw )
{
#if !defined(cygwin)
    register int border, borders;
    border = nw->primitive.highlight_thickness;
    borders = border + border;
    if( nw->number.raised != nw->number.key_entry_mode ) {
#if (OLD_LESSTIF != 1)
#if    (XmVersion < 2000)
        _XmDrawShadow(XtDisplay(nw), XtWindow(nw),
                      nw->primitive.top_shadow_GC,
                      nw->primitive.bottom_shadow_GC,
                      nw->primitive.shadow_thickness, border, border,
                      nw->core.width - borders, nw->core.height - borders);
      } else {
        _XmDrawShadow(XtDisplay(nw), XtWindow(nw), 
                      nw->primitive.bottom_shadow_GC,
                      nw->primitive.top_shadow_GC,
                      nw->primitive.shadow_thickness, border, border,
                      nw->core.width - borders, nw->core.height - borders);
#else    /* XmVersion >= 2000 */
	XmeDrawShadows(XtDisplay(nw), XtWindow(nw),
		      nw->primitive.top_shadow_GC,
		      nw->primitive.bottom_shadow_GC,
		      border, border,
		      nw->core.width - borders, nw->core.height - borders,
		      nw->primitive.shadow_thickness,
		      XmSHADOW_IN);
    } else {
	XmeDrawShadows(XtDisplay(nw), XtWindow(nw), 
		      nw->primitive.bottom_shadow_GC,
		      nw->primitive.top_shadow_GC,
		      border, border,
		      nw->core.width - borders, nw->core.height - borders,
		      nw->primitive.shadow_thickness,
		      XmSHADOW_IN );
#endif    /* XmVersion */
#endif /* OLD_LESSTIF */
    }
#endif
}


/*  Subroutine:	Resize
 *  Purpose:	Calculate the drawing area and redraw the widget
 */
static void Resize( XmNumberWidget nw )
{
    Dimension width, height;

    /*  Make sure widget was properly initialized  */
    if( nw->number.font_struct == NULL )
	return;
    SetNumberSize(nw, &width, &height);
    /*  Center the text field  */
    nw->number.x = (int)((int)nw->core.width - (int)nw->number.width) / 2;
    nw->number.x = Max(0, nw->number.x);
    nw->number.y = (int)((int)nw->core.height - (int)nw->number.height) / 2;
    nw->number.y = Max(0, nw->number.y);
}


/*  Subroutine:	PreferredSize
 *  Purpose:	Calculate the drawing area and report what it should be
 */
static XtGeometryResult PreferredSize( XmNumberWidget nw,
				       XtWidgetGeometry* request,
				       XtWidgetGeometry* preferred )
{
    Dimension min_width, min_height, borders;

    /*  If no size changes are being requested, just agree  */
    if( ((request->request_mode & CWWidth) == 0) &&
        ((request->request_mode & CWHeight) == 0) )
	return XtGeometryYes;
    /*  Make sure widget was properly initialized  */
    if( nw->number.font_struct == NULL )
    {
	if( nw->number.initialized == True )
	    /*  If record shows an initialize was attempted, it's no dice  */
	    return XtGeometryNo;
	GetNumberGC(nw);
	SetNumberSize(nw, &min_width, &min_height);
    }
    preferred->request_mode = CWWidth | CWHeight;
    /*  If no size change would result, tell requestor to give up  */
    if( (request->width > 0) && (request->width == nw->core.width) &&
        (request->height > 0) && (request->height == nw->core.height) )
	{
	preferred->width = request->width;
	preferred->height = request->height;
	return XtGeometryNo;
	}
    borders =
      2 * (nw->primitive.highlight_thickness + nw->primitive.shadow_thickness);
    min_width = nw->number.width + borders;
#if 0
    min_height = nw->number.height + borders;
#else
    min_height = 
	nw->number.font_struct->ascent + nw->number.font_struct->descent + 2 + borders;
#endif
    preferred->width = min_width + (2 * nw->number.h_margin);
    preferred->height = min_height + (2 * nw->number.v_margin);
    if( (request->request_mode & CWWidth) &&
        (request->request_mode & CWHeight) )
    {
	if( (request->width >= preferred->width) &&
	    (request->height >= preferred->height) )
	{
	    /*  If both dimensions are acceptable, say yes  */
	    return XtGeometryAlmost;
	}
	else if( (request->width <  preferred->width) &&
		 (request->height < preferred->height) )
	    /*  If both are unacceptable, say no  */
	    return XtGeometryAlmost;
    }
    else if( request->request_mode & CWWidth )
    {
	preferred->width = request->width;
	if( request->width != preferred->width )
	    return XtGeometryAlmost;
    }
    else
    {
	preferred->height = request->height;
	if( request->height != preferred->height )
	    return XtGeometryAlmost;
    }
    return XtGeometryNo;
}


/*  Subroutine:	Destroy
 *  Purpose:	Clean up allocated resources when the widget is destroyed.
 */
static void Destroy( XmNumberWidget nw )
{
    XtReleaseGC((Widget)nw, nw->number.gc);
    XtRemoveAllCallbacks((Widget)nw, XmNoverflowCallback);
    XtRemoveAllCallbacks((Widget)nw, XmNactivateCallback);
}


/*  Subroutine:	SetValues
 *  Purpose:	Make sure that everything changes correctly if the client
 *		makes any changes on things available as resources
 */
/* ARGSUSED */
static Boolean SetValues( XmNumberWidget current, XmNumberWidget request,
			  XmNumberWidget new )

{
    Boolean resize_width_flag = FALSE;
    Boolean resize_height_flag = FALSE;
    Boolean redraw_flag = FALSE;
    XmNumberWarningCallbackStruct call_value;

    /*
     * Check for a change of sensitivity
     */
    if( (new->core.sensitive != current->core.sensitive) ||
        (new->core.ancestor_sensitive != current->core.ancestor_sensitive) )
    {
	redraw_flag = TRUE;
    }
    /*  Check for a change of font  */
    if( new->number.font_list != current->number.font_list )
    {
	XtReleaseGC((Widget)new, new->number.gc);
	new->number.font_struct = NULL;
	GetNumberGC(new);
	resize_width_flag = TRUE;
	resize_height_flag = TRUE;
	redraw_flag = TRUE;
    }
    /*  Check for change of color  */
    if(   (new->primitive.foreground != current->primitive.foreground)
	    || (new->core.background_pixel != current->core.background_pixel) )
    {
	XtReleaseGC((Widget)new, new->number.gc);
	GetNumberGC(new);
	redraw_flag = TRUE;
    }
    /*  Check all the other things that could affect the size  */
    if(   (new->number.char_places != current->number.char_places)
	    || (new->primitive.shadow_thickness
		!= current->primitive.shadow_thickness)
	    || (new->primitive.highlight_thickness
		!= current->primitive.highlight_thickness)
	    || (new->number.decimal_places != current->number.decimal_places)
	    || (new->number.v_margin != current->number.v_margin)
	    || (new->number.h_margin != current->number.h_margin))
    {
	resize_width_flag = TRUE;
	redraw_flag = TRUE;
    }
    if ( (new->number.data_type == INTEGER) &&
	(new->number.decimal_places != current->number.decimal_places) )
	{
	call_value.reason = XmCR_NUM_WARN_DP_INT;
	call_value.message = "XmNdecimalPlaces resource should not be modified\nif XmNdataType resource is set to INTEGER.";
	XtCallCallbacks((Widget)new, XmNwarningCallback, &call_value);
	new->number.decimal_places = 0;
	}
    if(new->number.decimal_places != current->number.decimal_places)
	{
	new->number.cross_under = pow(0.1,(double)new->number.decimal_places);
	}
    /*  Just check resize on things not affecting visuals  */
    SyncMultitypeData(&new->number.value, new->number.data_type);
    SyncMultitypeData(&new->number.value_minimum, new->number.data_type);
    SyncMultitypeData(&new->number.value_maximum, new->number.data_type);
    if(   UpdateMultitypeData(new, &new->number.value_minimum,
				   &current->number.value_minimum) )
	resize_width_flag = TRUE;
    if(   UpdateMultitypeData(new, &new->number.value_maximum,
				   &current->number.value_maximum) )
	resize_width_flag = TRUE;
    if (resize_width_flag || resize_height_flag)
    {
	Dimension width, height;

	/*  Negotiate over size  */
	SetNumberSize(new, &width, &height);
	/* If actual resize is warranted, try for it but redraw in any case  */
	if( (new->core.width != width) || (new->core.height != height) )
	{
#if 0
 /* Don't do this... You're supposed to change the values in your own
  * core struct and then fini.  Let others negotiate over it.
  */
  		Dimension reply_w, reply_h;
	    if (XtMakeResizeRequest((Widget)new, width, height, &reply_w, &reply_h) == 
								XtGeometryYes)
#else
            if (1)
#endif
	    {	
		if (resize_width_flag) new->core.width = width;
		if (resize_height_flag) new->core.height = height;
		redraw_flag = TRUE;
	    }
	    else
	    {
		/* This only appears to happen when the number is not w/in
		   the bounds of its parent.  In this case, simply roll back
		   all changes */
		*new = *current;
		SetNumberSize(new, &width, &height);
		XtWarning("Number widget resize request denied by parent.");
		redraw_flag = TRUE;
	    }
	}
    }
    if( UpdateMultitypeData(new, &new->number.value, &current->number.value) )
	XmShowNumberValue(new);
    return redraw_flag;
}



/*  Subroutine:	SyncMultitypeData
 *  Purpose:	Change all elements of the struct to the type specified in the
 *		call
 */
void SyncMultitypeData( MultitypeData* new, short type )
{
    if (type == INTEGER)
        {
        new->d = (double)new->i;
        new->f = (float)new->i;
        }
    if (type == FLOAT)
        {
        new->d = (double)new->f;
        new->i = 0;
        }
    if (type == DOUBLE)
        {
        new->f = 0;
        new->i = 0;
        }
}
/*  Subroutine: UpdateMultitypeData
 *  Purpose:    Check for and handle a value change following priority for
 *              the value's optional data types
 */
static
Boolean UpdateMultitypeData(XmNumberWidget nw, MultitypeData* new, 
		MultitypeData* current )
{

    if (nw->number.data_type == INTEGER)
    {
        if( new->i != current->i )
        {
            new->d = (double)new->i;
            new->f = (float)new->i;
	    return True;
        }
    }
    else if (nw->number.data_type == FLOAT)
    {
        if( new->f != current->f )
        {
            new->d = (double)new->f;
            new->i = 0;
	    return True;
        }
    }
    else if (nw->number.data_type == DOUBLE)
    {
        if( new->d != current->d )
        {
            new->f = 0;
            new->i = 0;
	    return True;
        }
    }
    return False;
}


/*  Subroutine:	GetNumberGC
 *  Purpose:	Get a GC with the right resources for drawing the text string
 */
static void GetNumberGC( XmNumberWidget nw )
{
    XGCValues values;
    XtGCMask  valueMask;
    XColor    fg_cell_def, bg_cell_def;

    if( nw->number.font_struct == NULL )
    {
	if( nw->number.font_list )
	{
#if (XmVersion < 1001)
            int index;
            _XmFontListSearch(nw->number.font_list, XmSTRING_DEFAULT_CHARSET,
                              &index, &nw->number.font_struct);
#else
            XmFontContext   context;
	    XmFontListEntry flent;
	    XmFontType flent_type;
	    XtPointer xtp;
            XmFontListInitFontContext(&context, nw->number.font_list);
	    flent = XmFontListNextEntry (context);
	    xtp = XmFontListEntryGetFont (flent, &flent_type);
	    if (flent_type != XmFONT_IS_FONT) {
		XtWarning("Unable to access font list.");
		nw->number.font_struct = XLoadQueryFont(XtDisplay(nw), "fixed");
	    } else {
		nw->number.font_struct = xtp;
	    }
            XmFontListFreeFontContext(context);
#endif
	}
	else
	    nw->number.font_struct = XLoadQueryFont(XtDisplay(nw), "fixed");
    }
    values.graphics_exposures = False;
    values.foreground = nw->primitive.foreground;
    values.background = nw->core.background_pixel;
    values.font = nw->number.font_struct->fid;
    valueMask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
    nw->number.gc = XtGetGC((Widget)nw, valueMask, &values);
    /*
     *  Get the foreground and background colors, and use the average
     *  for the insensitive color
     */
    fg_cell_def.flags = DoRed | DoGreen | DoBlue;
    bg_cell_def.flags = DoRed | DoGreen | DoBlue;
    fg_cell_def.pixel = nw->primitive.foreground;
    XQueryColor(XtDisplay(nw), 
		DefaultColormapOfScreen(XtScreen(nw)),
		&fg_cell_def);

    bg_cell_def.pixel = nw->core.background_pixel;
    XQueryColor(XtDisplay(nw),
                DefaultColormapOfScreen(XtScreen(nw)),
                &bg_cell_def);
    fg_cell_def.red   = fg_cell_def.red/2   + bg_cell_def.red/2;
    fg_cell_def.green = fg_cell_def.green/2 + bg_cell_def.green/2;
    fg_cell_def.blue  = fg_cell_def.blue/2  + bg_cell_def.blue/2;
    
    /*
     * Fudge factor to offset the gamma correction
     */
    fg_cell_def.red = fg_cell_def.red/2;
    fg_cell_def.green = fg_cell_def.green/2;
    fg_cell_def.blue = fg_cell_def.blue/2;
    gamma_correct(&fg_cell_def);
    if(!XAllocColor(XtDisplay(nw),
                DefaultColormapOfScreen(XtScreen(nw)),
		&fg_cell_def))
	{
	find_color((Widget)nw, &fg_cell_def);
	}
    values.foreground = fg_cell_def.pixel;
    nw->number.inv_gc = XtGetGC((Widget)nw, valueMask, &values);
    
}


/*  Subroutine:	XmCreateNumber
 *  Purpose:	This function creates and returns a Number widget.
 */
Widget XmCreateNumber( Widget parent, String name,
		       ArgList args, Cardinal num_args )
#ifdef EXPLAIN
     Widget	parent;		/* Parent widget		 */
     String	name;		/* Name for this widget instance */
     ArgList	args;		/* Widget specification arg list */
     Cardinal	num_args;	/* Number of args in the ArgList */
#endif
{
    return XtCreateWidget(name, xmNumberWidgetClass, parent, args, num_args);
}


/*  Subroutine:	UpdateNumberEditorDisplay
 *  Purpose:	Draw the string being entered in the widget
 */
static
void UpdateNumberEditorDisplay( XmNumberWidget nw )
{
    int x, y, pad;
    pad = nw->number.char_places - nw->editor.string_len;
    if( nw->editor.string_len < nw->number.last_char_cnt )
	XClearArea(XtDisplay(nw), XtWindow(nw),
		   nw->number.x, nw->number.y,
		   nw->number.width + nw->number.h_margin, 
		   nw->number.height, False);
    y = nw->number.y + nw->number.font_struct->ascent + 1;
    /*  Calculate the text position x coordinate and print  */
    if( pad <= 0 )
    {
	x = nw->number.x;
    }
    else
    {
	if( nw->number.center && (pad > 0) )
	    x = nw->number.x + ((pad * nw->number.char_width) / 2) + 
			nw->number.h_margin;
	else
	    x = nw->number.x + (pad * nw->number.char_width);
    }
    XDrawImageString(XtDisplay(nw), XtWindow(nw), nw->number.gc,
		     x, y, nw->editor.string, nw->editor.string_len);
    x += XTextWidth(nw->number.font_struct,
		    nw->editor.string, nw->editor.insert_position);
    DrawCursor(nw, x, y, 2);
    nw->number.last_char_cnt = nw->editor.string_len + 1;
}


/*  Subroutine:	DrawCursor
 *  Purpose:	Draw a cursor to indicate the insert position (offering
 *		several different types of appearances).
 */
static void DrawCursor( XmNumberWidget nw, int x, int y, int type )
{
    XSegment segment[3];

    switch( type ) {
      case 0:
	/*  Underbar  */
	XDrawLine(XtDisplay(nw), XtWindow(nw), nw->number.gc,
		  x, y, x + nw->number.char_width, y);
	break;
      case 1:
	/*  Vertical line  */
	XDrawLine(XtDisplay(nw), XtWindow(nw), nw->number.gc,
		  x, nw->number.y, x, nw->number.y + nw->number.height - 1);
	break;
      case 2:
	/*  I beam  */
	segment[0].x1 = segment[2].x1 = x - 1;
	segment[1].x1 = segment[1].x2 = x;
	segment[0].x2 = segment[2].x2 = x + 1;
	segment[0].y1 = segment[0].y2 = segment[1].y1 = nw->number.y+1;
	segment[1].y2 = segment[2].y1 = segment[2].y2 =
	  nw->number.y + nw->number.height - 2;
	XDrawSegments(XtDisplay(nw), XtWindow(nw), nw->number.gc, segment, 3);
	break;
    }
}

void XmUnlimitNumberBounds(XmNumberWidget w)
{
    XmSetNumberBounds(w,1.0,0.0);
}
void XmSetNumberBounds(XmNumberWidget w, double dmin, double dmax)
{
    short type;
    Arg wargs[4];
    float fmin, fmax;
    int   imin, imax, n = 0;

    XtVaGetValues((Widget)w, XmNdataType, &type, NULL);

    if (dmin > dmax) { 		/* Implies an unlimit */
	switch (type) {
	  case DOUBLE:
	    dmin = DOUBLE_MIN;
	    dmax = DOUBLE_MAX;
	    break;
	  case FLOAT:
	    dmin = FLOAT_MIN;
	    dmax = FLOAT_MAX;
	    break;
	  case INTEGER:
	    dmin = -MAXINT;
	    dmax =  MAXINT;
	    break;
 	}
    } 

    if (type == DOUBLE) {
        DoubleSetArg(wargs[n], XmNdMinimum, dmin); n++;
        DoubleSetArg(wargs[n], XmNdMaximum, dmax); n++;
    } else if (type == FLOAT) {
	fmin = dmin;
	fmax = dmax;
        XtSetArg(wargs[n], XmNfMinimum, &fmin); n++;
        XtSetArg(wargs[n], XmNfMaximum, &fmax); n++;
    } else {
	imin = dmin;
	imax = dmax;
        XtSetArg(wargs[n], XmNiMinimum, imin); n++;
        XtSetArg(wargs[n], XmNiMaximum, imax); n++;
    }
    XtSetValues((Widget)w, wargs, n);

}

void XmGetNumberBounds(XmNumberWidget w, double *dmin, double *dmax)
{
    short type;
    Arg wargs[4];
    float fmin, fmax;
    int   imin, imax, n = 0;

    XtVaGetValues((Widget)w, XmNdataType, &type, NULL);

    switch (type) {
      case INTEGER:
	XtSetArg(wargs[n], XmNiMinimum, &imin); n++;
	XtSetArg(wargs[n], XmNiMaximum, &imax); n++;
	XtGetValues((Widget)w,wargs,n);
	*dmin = imin; 
	*dmax = imax;
	break;
      case FLOAT:
	XtSetArg(wargs[n], XmNfMinimum, &fmin); n++;
	XtSetArg(wargs[n], XmNfMaximum, &fmax); n++;
	XtGetValues((Widget)w,wargs,n);
	*dmin = fmin; 
	*dmax = fmax;
	break;
      case DOUBLE:
	XtSetArg(wargs[n], XmNdMinimum, dmin); n++;
	XtSetArg(wargs[n], XmNdMaximum, dmax); n++;
	XtGetValues((Widget)w,wargs,n);
	break;
    }
}
double XmGetNumberValue(XmNumberWidget w)
{
    short type;
    int ivalue;
    float fvalue;
    double dvalue;

    XtVaGetValues((Widget)w, XmNdataType, &type, NULL);

    switch (type) {
        case INTEGER:
            XtVaGetValues((Widget)w, XmNiValue, &ivalue, NULL);
            dvalue = (double) ivalue;
	    break;
        case FLOAT:
            XtVaGetValues((Widget)w, XmNfValue, &fvalue, NULL);
            dvalue = (double) fvalue;
	    break;
        case DOUBLE:
            XtVaGetValues((Widget)w, XmNdValue, &dvalue, NULL);
	    break;
    }

    return dvalue;
}


