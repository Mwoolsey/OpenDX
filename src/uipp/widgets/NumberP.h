/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _NumberP_H
#define _NumberP_H

#include <Xm/Xm.h>
#if (XmVersion >= 1002)
#include <Xm/PrimitiveP.h>
#endif
#include <Xm/XmP.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*  Number class structure  */

typedef struct _XmNumberWidgetClassPart
{
   XtPointer extension;
} XmNumberWidgetClassPart;


/*  Full class record declaration for Arrow class  */

typedef struct _XmNumberWidgetClassRec
{
   CoreClassPart		core_class;
   XmPrimitiveClassPart		primitive_class;
   XmNumberWidgetClassPart	number_class;
} XmNumberWidgetClassRec;

extern XmNumberWidgetClassRec xmNumberWidgetClassRec;



/*  The Number instance record  */

/*  Support multiple value types (non-doubles needed to recognize new input)  */
typedef struct
{
    double	d;
    float	f;
    int		i;
} MultitypeData;

typedef struct _XmNumberWidgetPart
{
   MultitypeData value;
   MultitypeData value_minimum;
   MultitypeData value_maximum;
   XmFontList	font_list;
   XtCallbackList overflow_callback;	/* Call if text would overflow field  */
   XtCallbackList arm_callback;		/* Going into editor mode	      */
   XtCallbackList activate_callback;
   XtCallbackList disarm_callback;	/* No longer in editor mode	      */
   XtCallbackList warning_callback;	/* No longer in editor mode	      */
   short	char_places;	/* Number of spaces (incl. decimal point)     */
   short	decimal_places; /* 0=integer, <0=%g, else fixed point	      */
   short	h_margin;	/* Pixels horizontal between shadow and text  */
   short	v_margin;	/* Pixels between shadow and bottom of text   */
   short	repeat_rate;	/* Milliseconds between auto-repeats	      */
   Boolean	center;		/* Center reduced length strings in field     */
   Boolean	raised;		/* Draw raised border, else depressed border  */
   Boolean	visible;	/* Text should be visible, else suppressed    */
   Boolean	allow_key_entry_mode; /* Keyboard entry of values allowed     */
   Boolean	propogate_events;    /* Pass key and pointer events to parent */
   /*  Private section  */
   Boolean	proportional;	/* Font proportional ("." smaller than "5")   */
   Boolean	initialized;
   Boolean	call_only_if_change;
   Boolean	key_entry_mode;	/* Currently in key entry mode		      */
   char		format[12];	/* Print format for fixed point or integer    */
   Position	x, y;
   Dimension	width, height;	/* Area occupied by char_places string	      */
   XFontStruct*	font_struct;
   GC		gc;
   GC		inv_gc;
   XtIntervalId	timer;
   short	char_width;	/* Pixel width of a number (e.g. "5")	      */
   short	dot_width;	/* Pixel width of a decimal point	      */
   short	last_char_cnt;	/* Number of characters most recently printed */
   short	data_type;	/* How the application feeds me data          */
   Boolean      recompute_size; /* Should the number widget resize to its min
                                   and max value? */
   double	cross_over;     /* Value above which we should represent the
				   number in scientific notation 	      */
   double	cross_under;    /* Value below which we should represent the
				   number in scientific notation 	      */
} XmNumberWidgetPart;


typedef struct _XmNumberEditorPart
{
   double value;
   double decimal_minimum;	/* The smallest fraction allowed as input    */
   short string_len;
   short insert_position;
   Boolean is_float;		/* Fractional: allow use of decimal point    */
   Boolean is_fixed;		/* Fixed: don't allow scientific notation    */
   Boolean is_signed;		/* (min < 0.0): allow '-' sign		     */
#define MAX_EDITOR_STRLEN       63      
   char string[MAX_EDITOR_STRLEN+1];
} XmNumberEditorPart;

/*  Full instance record declaration  */

typedef struct _XmNumberWidgetRec
{
   CorePart		core;
   XmPrimitivePart	primitive;
   XmNumberWidgetPart	number;
   XmNumberEditorPart	editor;
} XmNumberWidgetRec;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif


