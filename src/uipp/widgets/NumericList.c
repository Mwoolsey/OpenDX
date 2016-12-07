/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

/*
 */

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/CoreP.h>
#include <Xm/DrawingA.h>
#include <Xm/BulletinBP.h>
#include <Xm/DrawingAP.h>
#include <Xm/BulletinB.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrolledWP.h>
#include <Xm/Label.h>
#include "NumericList.h"
#include "NumericListP.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define superclass (&widgetClassRec)

static void Initialize( XmNumericListWidget request, XmNumericListWidget new );
static Boolean SetValues( XmNumericListWidget current,
			  XmNumericListWidget request,
			  XmNumericListWidget new );
static void Realize( register XmNumericListWidget w, Mask *p_valueMask,
		     XSetWindowAttributes *attributes );
static void Redisplay(XmNumericListWidget nlw, XEvent *event);
static void Destroy(XmNumericListWidget nlw);
static void print_vector(XmNumericListWidget nlw, int vector_number, 
			Boolean inverse, Boolean clear);
static void do_layout(XmNumericListWidget nlw);
static void MakeGC ( XmNumericListWidget w);
static void copy_vectors(XmNumericListWidget nlw, int old_n_vectors);
static void Arm();
static void Activate();
static void decrement_if_possible();
static void increment_if_possible();
static void jump_to_end(XmNumericListWidget nlw);
static void set_decimal_places(XmNumericListWidget nlw);
static void BtnMotion();

#define STRCMP(a,b)  ((a) ? ((b) ? strcmp(a,b) : strcmp(a,"")) : \
			    ((b) ? strcmp("",b) : 0))

#define STRLEN(s)  	((s) ? strlen(s) : 0)


/* Default translation table and action list */
static char defaultTranslations[] =
    "<Btn1Down>:     Arm()\n\
     <Btn1Up>:       Activate()\n\
     <Btn1Motion>:   BtnMotion()\n\
     <EnterWindow>:  Enter() \n\
     <FocusIn>:      FocusIn()";

extern void _XmManagerEnter();
extern void _XmManagerFocusIn();

static XtActionsRec actionsList[] =
{
   { "Arm",      (XtActionProc) Arm  },
   { "Activate", (XtActionProc) Activate },
   { "BtnMotion",(XtActionProc) BtnMotion },
   { "Enter",    (XtActionProc) _XmManagerEnter },
   { "FocusIn",  (XtActionProc) _XmManagerFocusIn },
};


static XtResource resources[] =
{
    {
	XmNtuples,XmCValue,XmRInt,sizeof(int),
	XtOffset(XmNumericListWidget,numeric_list.tuples),
	XmRImmediate, (XtPointer)3
    },
    {
	/* This should be a readonly resource */
	XmNreadonlyVectors,XmCVector,XmRVectorList,sizeof(VectorList),
	XtOffset(XmNumericListWidget,numeric_list.local_vectors),
	XmRImmediate, (XtPointer)-1
    },
    {
	XmNvectors,XmCVector,XmRVectorList,sizeof(VectorList),
	XtOffset(XmNumericListWidget,numeric_list.vectors),
	XmRImmediate, (XtPointer)-1
    },
    {
	XmNvectorCount,XmCValue,XmRInt,sizeof(int),
	XtOffset(XmNumericListWidget,numeric_list.vector_count),
	XmRImmediate, (XtPointer)1
    },
    {
	XmNvectorSpacing,XmCValue,XmRDimension,sizeof(Dimension),
	XtOffset(XmNumericListWidget,numeric_list.vector_spacing),
	XmRImmediate, (XtPointer)10
    },
    {
	XmNdecimalPlaces,XmCValue,XmRPointer,sizeof(int *),
	XtOffset(XmNumericListWidget,numeric_list.decimal_places),
	XmRImmediate, (XtPointer)NULL
    },
    {
	XmNselectCallback,XmCCallback,XmRCallback,sizeof(XtPointer),
	XtOffset(XmNumericListWidget,numeric_list.select_callback),
	XmRCallback,(XtPointer)NULL
    },
    {
	XmNminColumnWidth,XmCMinColumnWidth,XmRDimension,sizeof(Dimension),
	XtOffset(XmNumericListWidget,numeric_list.min_column_width),
	XmRImmediate, (XtPointer)20
    },
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XmNumericListClassRec xmNumericListClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmBulletinBoardClassRec,	/* superclass         */
      "XmNumericList",				/* class_name         */
      sizeof(XmNumericListRec),			/* widget_size        */
      NULL,					/* class_initialize   */
      NULL,					/* class_part_init    */
      FALSE,					/* class_inited       */
      (XtInitProc) Initialize,			/* initialize         */
      NULL,					/* initialize_hook    */
      (XtRealizeProc)Realize,			/* realize            */
      (XtActionList)actionsList,		/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      TRUE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      (XtWidgetProc)Destroy,			/* destroy            */
      (XtWidgetProc)_XtInherit,		/* resize             */
      (XtExposeProc)Redisplay,			/* expose             */
      (XtSetValuesFunc) SetValues,		/* set_values         */
      NULL,					/* set_values_hook    */
      (XtAlmostProc)_XtInherit,			/* set_values_almost  */
      NULL,					/* get_values_hook    */
      NULL,					/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      defaultTranslations,			/* tm_table           */
      XtInheritQueryGeometry,				/* query_geometry     */
      NULL,             	                /* display_accelerator   */
      NULL,		                        /* extension             */
   },

   {		/* composite_class fields */
      XtInheritGeometryManager,			/* geometry_manager   */
      (XtWidgetProc)_XtInherit,			/* change_managed     */
      (XtWidgetProc)_XtInherit,			/* insert_child       */
      (XtWidgetProc)_XtInherit,			/* delete_child       */
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      sizeof (XmManagerConstraintRec),		/* constraint size      */   
      NULL,					/* init proc            */   
      NULL,					/* destroy proc         */   
      NULL,					/* set values proc      */   
      NULL,                                     /* extension            */
   },

   {		/* manager_class fields */
#if (XmVersion >= 1001)
      XtInheritTranslations,     		/* translations           */
#else
      (XtTranslations)_XtInherit,     		/* translations           */
#endif
      NULL,					/* get resources      	  */
      0,					/* num get_resources 	  */
      NULL,					/* get_cont_resources     */
      0,					/* num_get_cont_resources */
      (XmParentProcessProc)NULL,                /* parent_process         */
      NULL,					/* extension           */
   },

   {		/* bulletin board class - none */
      False,					/* mumble */
      NULL,					/* mumble */
      NULL,					/* mumble */
      NULL,					/* mumble */
   },

   {		/* numeric_list class - none */
      NULL					/* mumble */
   }
};


WidgetClass xmNumericListWidgetClass = (WidgetClass) &xmNumericListClassRec;

/*  Subroutine:	Initialize
 *  Effect:	Create and initialize the component widgets
 */
static void Initialize( XmNumericListWidget request, XmNumericListWidget new )
{
XmFontList	font_list;
Widget		label;
Arg		wargs[100];
unsigned long	space;
#if (XmVersion >= 1001)
XmFontContext   context;
XmStringCharSet charset;
#endif


    /*
     * Create a label widget in order to extract the current font list.
     */
    label = (Widget)XmCreateLabel((Widget)(new), "Label", wargs, 0);
    XtSetArg(wargs[0], XmNfontList, &font_list);
    XtGetValues(label, wargs, 1);
#if (XmVersion < 1001)
    new->numeric_list.font = font_list->font;
#else
    XmFontListInitFontContext(&context, font_list);
    XmFontListGetNextFont(context, &charset, &new->numeric_list.font);
    XmFontListFreeFontContext(context);
#endif

    /*
     * Check to see if there is a XA_NORM_SPACE font property defined
     * for this font.  If there is, save it.
     */
     if (XGetFontProperty(new->numeric_list.font, XA_NORM_SPACE, &space))
	{
	new->numeric_list.norm_space = space;
	}
    else
	{
	new->numeric_list.norm_space = 0;
	}
    /*
     * Move the vector pointer to a private variable and reset the resource
     * variable to NULL so we can detect a SetValues on this resource;
     */
    copy_vectors(new, 0);
    
    /*
     * Setting up the decimal places MUST happen before do_layout.
    */
    new->numeric_list.local_decimal_places = NULL;
    set_decimal_places(new);
    new->numeric_list.decimal_places = NULL;
    
    /*
     * Start with none of the vectors selected
     */
    new->numeric_list.selected      = -1;
    new->numeric_list.first_selected = -1;

    if ( XtClass(XtParent(new)) == xmScrolledWindowWidgetClass)
	{
	XtSetArg(wargs[0], XmNverticalScrollBar, &new->numeric_list.vsb);
	XtGetValues(XtParent(new), wargs, 1);

	XtSetArg(wargs[0], XmNhorizontalScrollBar, &new->numeric_list.hsb);
	XtGetValues(XtParent(new), wargs, 1);
	}
    else
	{
	new->numeric_list.hsb = NULL;
	new->numeric_list.vsb = NULL;
	}

    new->numeric_list.tid = (XtIntervalId)NULL;
    new->numeric_list.scrolled_window = False;
    new->numeric_list.sw = NULL;
    new->numeric_list.decrementing = False;
    new->numeric_list.incrementing = False;
    new->numeric_list.vectors = NULL;
    new->numeric_list.normal_gc = (GC)NULL;
    new->numeric_list.inverse_gc = (GC)NULL;
    new->numeric_list.vector_width_left = NULL;
    new->numeric_list.vector_width_right = NULL;
}

/*  Subroutine:	SetValues
 *  Effect:	Handles requests to change things from the application
 */
static Boolean SetValues( XmNumericListWidget current,
			  XmNumericListWidget request,
			  XmNumericListWidget new )
{
Boolean dolayout = False;
Boolean redraw = False;

    if (new->numeric_list.decimal_places != 
	current->numeric_list.decimal_places)
	{
	set_decimal_places(new);
	dolayout = True;
	redraw = True;
	new->numeric_list.decimal_places = NULL;
	}
    if(new->numeric_list.vectors != current->numeric_list.vectors)

	{
	copy_vectors(new, current->numeric_list.vector_count);
	dolayout = True;
	redraw = True;
	new->numeric_list.vectors = NULL;
	}

   if(new->numeric_list.vector_count < current->numeric_list.vector_count)
	{
	new->numeric_list.selected = -1;
	}
   if ((new->manager.foreground != current->manager.foreground)||
       (new->core.background_pixel != current->core.background_pixel))
	{
	MakeGC (new);
	redraw = True;
	}

    if (dolayout)
	do_layout(new);

    return redraw;
}


/*  Subroutine:	Realize
 *  Purpose:	Create the window with PointerMotion events and None gravity
 */
static void Realize( register XmNumericListWidget w, Mask *p_valueMask,
		     XSetWindowAttributes *attributes )
{

    /*
     * Call the superclass realize method
     */
    (*superclass->core_class.realize)((Widget)w, p_valueMask, attributes);
    MakeGC (w);
    do_layout(w);

}

/*
 * Recompute the GC's.  Moved into a subroutine so that it can be called
 * from SetValues when the colors change.
 */
static void MakeGC ( XmNumericListWidget w)
{
unsigned long	valuemask;
XGCValues	values;

    if(w->numeric_list.normal_gc) 
	XtReleaseGC((Widget)w, w->numeric_list.normal_gc);
    if(w->numeric_list.inverse_gc)
	XtReleaseGC((Widget)w, w->numeric_list.inverse_gc);
    /*
     * Create a normal video and inverse video GC.
     */
    valuemask = GCForeground | GCBackground | GCFont;
    values.foreground = w->manager.foreground;
    values.background = w->core.background_pixel;
    values.font = w->numeric_list.font->fid;
    w->numeric_list.normal_gc = 
	XtGetGC((Widget)w, valuemask, &values);
		
    values.foreground = w->core.background_pixel;
    values.background = w->manager.foreground;
    values.font = w->numeric_list.font->fid;
    w->numeric_list.inverse_gc = 
	XtGetGC((Widget)w, valuemask, &values);
}


static void Destroy( XmNumericListWidget nlw)
{
int i;

    if(nlw->numeric_list.normal_gc) 
	XtReleaseGC((Widget)nlw, nlw->numeric_list.normal_gc);
    if(nlw->numeric_list.inverse_gc)
	XtReleaseGC((Widget)nlw, nlw->numeric_list.inverse_gc);

    if (nlw->numeric_list.local_decimal_places)
	XtFree((char*)nlw->numeric_list.local_decimal_places);

    if (nlw->numeric_list.vector_width_left) 
	XtFree((char*)nlw->numeric_list.vector_width_left);
    if (nlw->numeric_list.vector_width_right) 
	XtFree((char*)nlw->numeric_list.vector_width_right);

    /* 
     * Free the old vectors
     */
    if (nlw->numeric_list.vector_count > 0)
	{
        if(nlw->numeric_list.local_vectors)
	    {
	    for (i=0; i < nlw->numeric_list.vector_count; i++)
	        {
	        XtFree((char*)nlw->numeric_list.local_vectors[i]);
	        }
	    XtFree((char*)nlw->numeric_list.local_vectors);
	    }
	}
}

/*********************************************************************
* print_vector - clears the background (if necessary) and draws the
*  		 string.
**********************************************************************/
static void print_vector(XmNumericListWidget nlw, int vector_number, 
			Boolean inverse, Boolean clear)
{
int i;
char 			string[100];
char			**e_format;
char			**f_format;
char			tmp[100];
GC			gc;
Dimension		x;
Dimension		y;
Dimension		width;
int			k;

    y = vector_number * nlw->numeric_list.line_height;	
    if (!inverse)
	{
	/*
	* Draw a deselected item
	*/
	if(clear || inverse)
	    {
	    XFillRectangle(XtDisplay(nlw),
			   XtWindow(nlw),
			   nlw->numeric_list.inverse_gc,
			   0,y,
			   nlw->core.width, 
			   nlw->numeric_list.line_height);
	    }
	gc = nlw->numeric_list.normal_gc;
	}
    else
	{
	/*
	* Select the item
	*/
	if(clear || inverse)
	    {
	    XFillRectangle(XtDisplay(nlw),
			   XtWindow(nlw),
			   nlw->numeric_list.normal_gc,
			   0,y,
			   nlw->core.width, 
			   nlw->numeric_list.line_height);
	    }
	gc = nlw->numeric_list.inverse_gc;
	}
    e_format = (char **)XtCalloc(nlw->numeric_list.tuples , sizeof(char *));
    f_format = (char **)XtCalloc(nlw->numeric_list.tuples , sizeof(char *));
    for(i = 0; i < nlw->numeric_list.tuples; i++)
	{
	e_format[i] = (char *)XtCalloc(100 , sizeof(char *));
	f_format[i] = (char *)XtCalloc(100 , sizeof(char *));

	*(e_format[i] + 0) = '%';
	*(e_format[i] + 1) = '.';
	*(e_format[i] + 2) = '\0';
	sprintf(tmp, "%de", nlw->numeric_list.local_decimal_places[i]);
	strcat(e_format[i], tmp);

	*(f_format[i] + 0) = '%';
	*(f_format[i] + 1) = '.';
	*(f_format[i] + 2) = '\0';
	sprintf(tmp, "%df", nlw->numeric_list.local_decimal_places[i]);
	strcat(f_format[i], tmp);
	}

    x = nlw->numeric_list.vector_spacing;
    for(i = 0; i < nlw->numeric_list.tuples; i++)
	{
    	memset(string, 0, 100);
	if (((*(nlw->numeric_list.local_vectors[vector_number] + i) > 0.0) &&
	     (*(nlw->numeric_list.local_vectors[vector_number] + i) < .001)) ||
	    ((*(nlw->numeric_list.local_vectors[vector_number] + i) < 0.0) &&
	     (*(nlw->numeric_list.local_vectors[vector_number] + i) > -0.001)))
	{
	    sprintf(string, e_format[i], 
		    *(nlw->numeric_list.local_vectors[vector_number] + i));
	    /*fixed_format = False;*/
	}
	else
	{
	    sprintf(string, f_format[i], 
		    *(nlw->numeric_list.local_vectors[vector_number] + i));
	    /*fixed_format = True;*/
	}
	if (STRLEN(string) > nlw->numeric_list.local_decimal_places[i] + 7)
	{
	    sprintf(string, e_format[i], 
		    *(nlw->numeric_list.local_vectors[vector_number] + i));
	    /*fixed_format = False;*/
	}

	k = 0;
	while( (string[k] != '\0') && (string[k] != '.') ) k++;
	width = XTextWidth(nlw->numeric_list.font, string, k+1);
	width += nlw->numeric_list.norm_space * (k+1);
	XDrawString(XtDisplay(nlw),
		    XtWindow(nlw),
		    gc,
		    x + (nlw->numeric_list.vector_width_left[i] - width),
		    y + nlw->numeric_list.line_height - 
		    		nlw->numeric_list.descent,
		    string,
		    STRLEN(string));
       
	/*
	 * Move to the next component of the vector.
	 */
	x = x + nlw->numeric_list.vector_spacing + 
		nlw->numeric_list.vector_width_left[i] +
		nlw->numeric_list.vector_width_right[i];
	}

    for(i = 0; i < nlw->numeric_list.tuples; i++)
	{
	XtFree(f_format[i]);
	XtFree(e_format[i]);
	}
    XtFree((char*)f_format);
    XtFree((char*)e_format);
}

static void Redisplay(XmNumericListWidget nlw, XEvent *event)
{
Boolean	inverse;
int	i;

    for(i = 0; i < nlw->numeric_list.vector_count; i++)
	{
	if (nlw->numeric_list.selected == i)
	    {
	    inverse = True;
	    }
	else
	    {
	    inverse = False;
	    }
	print_vector(nlw, i, inverse, False);
	}
}

static void Arm(XmNumericListWidget nlw, XEvent *event, String *params, 
		Cardinal *num_params)
{
int vector_number;

    if (nlw->numeric_list.vector_count == 0) return;
    vector_number = (event->xbutton.y/nlw->numeric_list.line_height);
    nlw->numeric_list.first_selected = vector_number;
    /*
     * Deselect the item if the user pressed a button on a selected item
     */
    if (nlw->numeric_list.selected == vector_number)
	{
    	print_vector(nlw, vector_number, False, True);
	nlw->numeric_list.selected = -1;
	return;
	}
    /* 
     * If an item was selected, deselect it.
     */
    if (nlw->numeric_list.selected != -1)
	{
    	print_vector(nlw, nlw->numeric_list.selected, False, True);
	}
    /*
     * Select the item
     */
    print_vector(nlw, vector_number, True, True);
    nlw->numeric_list.selected = vector_number;
}

static void BtnMotion(XmNumericListWidget nlw, XEvent *event, String *params, 
		Cardinal *num_params)
{
int	vector_number;
int	sw_x;
int	sw_y;
int sw_height;
Window	child;
Widget  clip_window;
Arg     wargs[10];

    if (nlw->numeric_list.vector_count == 0) return;
    /*
     * Get the coords relative to the scrolled window.
     */
    if (nlw->numeric_list.scrolled_window)
	{
	XtSetArg(wargs[0], XmNclipWindow, &clip_window);
	XtGetValues((Widget)nlw->numeric_list.sw, wargs, 1);

	XtSetArg(wargs[0], XmNheight, &sw_height);
	XtGetValues(clip_window, wargs, 1);

	XTranslateCoordinates( XtDisplay(nlw), 
			       XtWindow(nlw), 
			       XtWindow(clip_window),
			       event->xbutton.x, 
			       event->xbutton.y, 
			       &sw_x, 
			       &sw_y, 
			       &child);

	/*
	 * Turn off Timer if we have moved back in between the top & bottom.
	 */
	if  ( (nlw->numeric_list.tid != (XtIntervalId)NULL) &&
	      (sw_y > 0) &&
	      (sw_y < sw_height) )
	    {
	    XtRemoveTimeOut(nlw->numeric_list.tid);
	    nlw->numeric_list.tid = (XtIntervalId)NULL;
	    nlw->numeric_list.decrementing = False;
	    nlw->numeric_list.incrementing = False;
	    }

	/*
	 *  Turn on decrement timer.
	 */
	if( (sw_y < 0) && (nlw->numeric_list.tid == (XtIntervalId)NULL) )
	    {
	    nlw->numeric_list.tid = 
			XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)nlw),
				100, decrement_if_possible, nlw);
	    nlw->numeric_list.decrementing = True;
	    return;
	    }

	/*
	 *  Turn on increment timer.
	 */
	if ( (sw_y > sw_height) && 
	     (nlw->numeric_list.tid == (XtIntervalId)NULL) )
	    {
	    nlw->numeric_list.tid = 
			XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)nlw),
				100, increment_if_possible, nlw);
	    nlw->numeric_list.incrementing = True;
	    return;
	    }

	/*
	 *  Turn off increment time and turn on decrement timer.
	 */
	if ( (sw_y < 0) && (nlw->numeric_list.incrementing) )
	    {
	    XtRemoveTimeOut(nlw->numeric_list.tid);
	    nlw->numeric_list.tid = 
			XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)nlw),
				100, decrement_if_possible, nlw);
	    nlw->numeric_list.incrementing = False;
	    nlw->numeric_list.decrementing = True;
	    return;
	    }

	/*
	 *  Turn off decrement time and turn on increment timer.
	 */
	if ( (sw_y > sw_height) &&
	     (nlw->numeric_list.decrementing) )
	    {
	    XtRemoveTimeOut(nlw->numeric_list.tid);
	    nlw->numeric_list.tid = 
			XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)nlw),
				100, increment_if_possible, nlw);
	    nlw->numeric_list.decrementing = False;
	    nlw->numeric_list.incrementing = True;
	    return;
	    }
	}

    vector_number = (event->xbutton.y/nlw->numeric_list.line_height);

    if(vector_number < 0)
	{
	vector_number = 0;
	}
    if(vector_number >= nlw->numeric_list.vector_count)
	{
	vector_number = nlw->numeric_list.vector_count - 1;
	}

    if (vector_number == nlw->numeric_list.first_selected)
	{
	return;
	}
    else
	{
	nlw->numeric_list.first_selected = -1;
	}

    /*
     * Ignore the item if it was already selected
     */
    if (nlw->numeric_list.selected == vector_number)
	{
	return;
	}
    /* 
     * If an item was selected, deselect it.
     */
    if (nlw->numeric_list.selected != -1)
	{
    	print_vector(nlw, nlw->numeric_list.selected, False, True);
	}
    /*
     * Select the item
     */
    print_vector(nlw, vector_number, True, True);
    nlw->numeric_list.selected = vector_number;
}

static void Activate(XmNumericListWidget nlw, XEvent *event, String *params, 
		Cardinal *num_params)
{
XmNumericListCallbackStruct cb;

    if (nlw->numeric_list.vector_count == 0) return;

    if  (nlw->numeric_list.tid != (XtIntervalId)NULL)
	{
	XtRemoveTimeOut(nlw->numeric_list.tid);
	nlw->numeric_list.tid = (XtIntervalId)NULL;
	}
    nlw->numeric_list.first_selected = -1;
    /*
     * Invoke the select callback.
     */
    cb.reason       = XmCR_SINGLE_SELECT;
    cb.event        = event;
    cb.position     = nlw->numeric_list.selected;
    cb.vector_list  = nlw->numeric_list.local_vectors;
    XtCallCallbacks((Widget)nlw,XmNselectCallback,&cb);

}
/*********************************************************************
* copy_vectors - allocates an internal buffer and copies the vector
*                list to the internal buffer 
*	       
**********************************************************************/
static void copy_vectors(XmNumericListWidget nlw, int old_n_vectors)
{
int		i;
int		j;

    /* 
     * Free the old vectors
     */
    if (old_n_vectors > 0)
	{
        if(nlw->numeric_list.local_vectors)
	    {
	    for (i=0; i < old_n_vectors; i++)
	        {
	        XtFree((char*)nlw->numeric_list.local_vectors[i]);
	        }
	    XtFree((char*)nlw->numeric_list.local_vectors);
	    }
	}
    
    /* 
     * Alloc the new memory and copy the vector over
     */
    nlw->numeric_list.local_vectors = (VectorList)XtMalloc(sizeof(Vector) * 
		nlw->numeric_list.vector_count);
    for (i=0; i < nlw->numeric_list.vector_count; i++)
	{
	nlw->numeric_list.local_vectors[i] = 
		(Vector)XtCalloc(nlw->numeric_list.tuples , sizeof(double));
	/*
	 * It is possible that the user has not provided a VectorList,
	 * so check to see if it exists first.
	 */
	if (nlw->numeric_list.vectors)
	    {
	    for (j=0; j < nlw->numeric_list.tuples; j++)
	        {
	        *(nlw->numeric_list.local_vectors[i] + j) = 
			    *(nlw->numeric_list.vectors[i] + j);
	        }
	    }
	}

    nlw->numeric_list.vectors = NULL;
}

/*********************************************************************
* set_decimal_places - allocates an internal buffer and copies the 
*                decimal places to the internal buffer 
*	       
**********************************************************************/
static void set_decimal_places(XmNumericListWidget nlw)
{
int	i;

    if (nlw->numeric_list.local_decimal_places)
	{
	XtFree((char*)nlw->numeric_list.local_decimal_places);
	}

    nlw->numeric_list.local_decimal_places = 
    		(int *)XtCalloc(nlw->numeric_list.tuples, sizeof(int));
 
    for (i = 0; i < nlw->numeric_list.tuples; i++)
	{
	if(nlw->numeric_list.decimal_places == NULL)
	    {
	    nlw->numeric_list.local_decimal_places[i] = 2;
	    }
	else
	    {
	    nlw->numeric_list.local_decimal_places[i] = 
			nlw->numeric_list.decimal_places[i];
	    }
	}
}
/*********************************************************************
* do_layout -  calculates the overall width and height required for the
*	       widget based on the font information
**********************************************************************/
static void do_layout(XmNumericListWidget nlw)
{
Dimension	line_width;
int		dir;
Dimension	width;
Dimension	height;
XCharStruct	overall;
int		i;
int		j;
char		tmp[100];
char		**e_format;
char		**f_format;
char		string[100];
int		k;
Dimension	sw_width;
Arg		wargs[20];
int		n;
XtWidgetGeometry request;
XtWidgetGeometry reply;
Widget          clip_window;

    /*
     * Calculate the height required for each line.
     */
    string[0] = '5';
    string[1] = '\0';
    XTextExtents(nlw->numeric_list.font, string, STRLEN(string),
		 &dir, &nlw->numeric_list.ascent, 
		 &nlw->numeric_list.descent, &overall);
    nlw->numeric_list.line_height =  
		nlw->numeric_list.ascent +  nlw->numeric_list.descent;

    height = MAX(nlw->numeric_list.line_height * 
		nlw->numeric_list.vector_count, 10);

    /*
     * Unmanage and re-manage the vertical scrollbar to skirt and *apparent*
     * bug in the scrolled window widget.
     */
    if(nlw->numeric_list.vsb) XtUnmanageChild(nlw->numeric_list.vsb);
    request.request_mode = CWHeight;
    request.height = height;
    XtMakeGeometryRequest((Widget)nlw, &request, &reply);
    if(nlw->numeric_list.vsb) XtManageChild(nlw->numeric_list.vsb);

    e_format = (char **)XtCalloc(nlw->numeric_list.tuples, sizeof(char *));
    f_format = (char **)XtCalloc(nlw->numeric_list.tuples, sizeof(char *));
    if (nlw->numeric_list.vector_width_left) 
	XtFree((char*)nlw->numeric_list.vector_width_left);
    if (nlw->numeric_list.vector_width_right) 
	XtFree((char*)nlw->numeric_list.vector_width_right);
    nlw->numeric_list.vector_width_left = 
	(Dimension *)XtCalloc(nlw->numeric_list.tuples, sizeof(Dimension));
    nlw->numeric_list.vector_width_right = 
	(Dimension *)XtCalloc(nlw->numeric_list.tuples, sizeof(Dimension));
    for(i = 0; i < nlw->numeric_list.tuples; i++)
	{
	/*
	 * Create a format string for each column
	 */
	e_format[i] = (char *)XtCalloc(100 , sizeof(char *));
	f_format[i] = (char *)XtCalloc(100 , sizeof(char *));

	*(e_format[i] + 0) = '%';
	*(e_format[i] + 1) = '.';
	*(e_format[i] + 2) = '\0';
	sprintf(tmp, "%de", nlw->numeric_list.local_decimal_places[i]);
	strcat(e_format[i], tmp);

	*(f_format[i] + 0) = '%';
	*(f_format[i] + 1) = '.';
	*(f_format[i] + 2) = '\0';
	sprintf(tmp, "%df", nlw->numeric_list.local_decimal_places[i]);
	strcat(f_format[i], tmp);

	/*
	 * Init vector width for each column to 0 
	 */
	nlw->numeric_list.vector_width_left[i] = nlw->numeric_list.min_column_width;
	nlw->numeric_list.vector_width_right[i] =nlw->numeric_list.min_column_width;
	}
    for(i = 0; i < nlw->numeric_list.vector_count; i++)
	{
	for(j = 0; j < nlw->numeric_list.tuples; j++)
	    {
    	    memset(string, 0, 100);
	    if (((*(nlw->numeric_list.local_vectors[i] + j) > 0.0) &&
		 (*(nlw->numeric_list.local_vectors[i] + j) < .001)) ||
		((*(nlw->numeric_list.local_vectors[i] + j) < 0.0) &&
		 (*(nlw->numeric_list.local_vectors[i] + j) > -0.001)))
	    {
		sprintf(string, e_format[j], 
			*(nlw->numeric_list.local_vectors[i] + j));
		/*fixed_format = False;*/
	    }
	    else
	    {
		sprintf(string, f_format[j], 
			*(nlw->numeric_list.local_vectors[i] + j));
		/*fixed_format = True;*/
	    }
	    if (STRLEN(string) > nlw->numeric_list.local_decimal_places[j] + 7)
	    {
		sprintf(string, e_format[j], 
			*(nlw->numeric_list.local_vectors[i] + j));
		/*fixed_format = False;*/
	    }
	    
	    k = 0;
	    while( (string[k] != '\0') && (string[k] != '.') ) k++;
	    width = XTextWidth(nlw->numeric_list.font, string, k+1);
	    width += nlw->numeric_list.norm_space * (k+1);
	    if(nlw->numeric_list.vector_width_left[j] < width)
		{
		nlw->numeric_list.vector_width_left[j] = width;
		}
	    if(string[k] == '.')
		{
		width = XTextWidth(nlw->numeric_list.font, &string[k+1], 
			STRLEN(&string[k+1]));
		width += nlw->numeric_list.norm_space * STRLEN(&string[k+1]);
		if(nlw->numeric_list.vector_width_right[j] < width)
		    {
		    nlw->numeric_list.vector_width_right[j] = width;
		    }
		}
	    }
	}

    /*
     * Calc the overall line width
     */
    line_width = 0;
    for (i = 0; i < nlw->numeric_list.tuples; i++)
	{
	line_width += nlw->numeric_list.vector_width_left[i];
	line_width += nlw->numeric_list.vector_width_right[i];
	line_width += nlw->numeric_list.vector_spacing;
	}
    line_width += nlw->numeric_list.vector_spacing;

     if ( (XtClass(XtParent(nlw)) == xmScrolledWindowWidgetClass) ||
	  (nlw->numeric_list.scrolled_window) )
	{
	if (nlw->numeric_list.hsb && XtIsManaged(nlw->numeric_list.hsb))
	    {
	    if (nlw->numeric_list.sw)
		{
		clip_window = XtParent(nlw);
		}
	    else
		{
		n = 0;
		XtSetArg(wargs[n], XmNclipWindow, &clip_window);
		XtGetValues(XtParent(nlw), wargs, n); n++;
		}
	    n = 0;
	    XtSetArg(wargs[n], XmNwidth, &sw_width); n++;
	    XtGetValues(clip_window, wargs, n);
	    }
	else
	    {
	    n = 0;
	    XtSetArg(wargs[n], XmNwidth, &sw_width); n++;
	    if (nlw->numeric_list.sw)
		{
		XtGetValues((Widget)nlw->numeric_list.sw, wargs, n);
		}
	    else
		{
		XtGetValues(XtParent(nlw), wargs, n); n++;
		}
	    }
	if (line_width < sw_width)
	    {
	    line_width = sw_width-2;
	    }
	}
    width = MAX(line_width, 10);

    if(nlw->numeric_list.vsb) XtUnmanageChild(nlw->numeric_list.vsb);
    request.request_mode = CWWidth;
    request.width = width;
    XtMakeGeometryRequest((Widget)nlw, &request, &reply);
    if(nlw->numeric_list.vsb) XtManageChild(nlw->numeric_list.vsb);

    /*
     * Free the format strings
     */
    for(i = 0; i < nlw->numeric_list.tuples; i++)
	{
	XtFree(e_format[i]);
	XtFree(f_format[i]);
	}
    XtFree((char*)e_format);
    XtFree((char*)f_format);
}

/***************************************************************************/
/*  Subroutine: decrement_if_possible                                      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
static void decrement_if_possible(XmNumericListWidget nlw, XtIntervalId *id)
{
Arg	wargs[10];
int	n;
int	value;
int	blank_lines;
XmScrolledWindowWidget window;
Boolean inverse;
int	i;

    nlw->numeric_list.tid = (XtIntervalId)NULL;
    if (!nlw->numeric_list.scrolled_window) return;

    window = (XmScrolledWindowWidget)XtParent(XtParent(nlw));
    XtVaGetValues(XtParent(nlw), XmNverticalScrollBar, &nlw->numeric_list.vsb,
	NULL);
    if(!nlw->numeric_list.vsb) return;

    n = 0;
    XtSetArg(wargs[n], XmNvalue, &value); n++;
    XtGetValues(nlw->numeric_list.vsb, wargs, n);
    
    value = value - nlw->numeric_list.line_height;
    if (value < 0) value = 0;

    /*
     * Align the vectors on a line border
     */
    if (value != 0)
	{
    	blank_lines = value/nlw->numeric_list.line_height;
    	if (value != blank_lines * nlw->numeric_list.line_height)
	    {
	    value = blank_lines * nlw->numeric_list.line_height;
	    }
	}
    else
	{
	blank_lines = 0;
	}

    /*
     * Reset the scrolled window's (x,y) origin.
     */
    window->swindow.vOrigin = -value;

    /*
     * Move the work window to reflect the new state.
     */
    XtMoveWidget(window->swindow.WorkWindow,
                 (Position)(window->swindow.hOrigin),
                 (Position)(window->swindow.vOrigin));

    XtSetArg(wargs[0], XmNvalue, value);
    XtSetValues(nlw->numeric_list.vsb, wargs, 1);

    nlw->numeric_list.selected = blank_lines;

    for(i = 0; i < nlw->numeric_list.vector_count; i++)
	{
	if (nlw->numeric_list.selected == i)
	    {
	    inverse = True;
	    }
	else
	    {
	    inverse = False;
	    }
	print_vector(nlw, i, inverse, True);
	}
    if (value != 0)
	{
	nlw->numeric_list.tid = 
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)nlw),
			100, (XtTimerCallbackProc)decrement_if_possible, nlw);
	}
}

/***************************************************************************/
/*  Subroutine: jump_to_end                    		                   */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
static void jump_to_end(XmNumericListWidget nlw)
{
Arg	wargs[10];
int	n;
int	value;
int	max;
int	slider_size;
int	increment;
int	page_increment;
XmScrolledWindowWidget window;
Boolean inverse;
int	i;

    if (!nlw->numeric_list.scrolled_window) return;

    window = (XmScrolledWindowWidget)XtParent(XtParent(nlw));

    XtVaGetValues(XtParent(nlw), XmNverticalScrollBar, &nlw->numeric_list.vsb,
	NULL);
    if(!nlw->numeric_list.vsb) return;

    n = 0;
    XtSetArg(wargs[n], XmNmaximum, &max); n++;
    XtSetArg(wargs[n], XmNsliderSize, &slider_size); n++;
    XtGetValues(nlw->numeric_list.vsb, wargs, n);

    XmScrollBarGetValues(nlw->numeric_list.vsb, &value, &slider_size, 
		&increment, &page_increment);
    value = max - slider_size;
    XmScrollBarSetValues(nlw->numeric_list.vsb, value, slider_size, 
		increment, page_increment, True);
    
    /*
     * Move the work window to reflect the new state.
     */
    XtMoveWidget(window->swindow.WorkWindow,
                 (Position)(window->swindow.hOrigin),
                 (Position)-value);

    /*
     * Reset the scrolled window's (x,y) origin.
     */
    window->swindow.vOrigin = -value;

    for(i = 0; i < nlw->numeric_list.vector_count; i++)
	{
	if (nlw->numeric_list.selected == i)
	    {
	    inverse = True;
	    }
	else
	    {
	    inverse = False;
	    }
	print_vector(nlw, i, inverse, True);
	}
}
/***************************************************************************/
/*  Subroutine: increment_if_possible                                      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
static void increment_if_possible(XmNumericListWidget nlw, XtIntervalId *id)
{
Arg	wargs[10];
int	n;
int	value;
int	max;
int	slider_size;
int	blank_lines;
XmScrolledWindowWidget window;
Boolean inverse;
int	i;

    nlw->numeric_list.tid = (XtIntervalId)NULL;
    if (!nlw->numeric_list.scrolled_window) return;

    window = (XmScrolledWindowWidget)XtParent(XtParent(nlw));
    XtVaGetValues(XtParent(nlw), XmNverticalScrollBar, &nlw->numeric_list.vsb,
	NULL);
    if(!nlw->numeric_list.vsb) return;

    n = 0;
    XtSetArg(wargs[n], XmNvalue, &value); n++;
    XtSetArg(wargs[n], XmNmaximum, &max); n++;
    XtSetArg(wargs[n], XmNsliderSize, &slider_size); n++;
    XtGetValues(nlw->numeric_list.vsb, wargs, n);
    
    max = max - slider_size;
    value = value + nlw->numeric_list.line_height;
    if (value > max) value = max;

    /*
     * Align the vectors on a line border
     */
    if ( (max - value) != 0)
	{
    	blank_lines = (max - value)/nlw->numeric_list.line_height;
    	if ((max - value) != blank_lines * nlw->numeric_list.line_height)
	    {
	    value = max - blank_lines * nlw->numeric_list.line_height;
	    }
	}
    else
	{
	blank_lines = 0;
	}

    /*
     * Reset the scrolled window's (x,y) origin.
     */
    window->swindow.vOrigin = -value;

    /*
     * Move the work window to reflect the new state.
     */
    XtMoveWidget(window->swindow.WorkWindow,
                 (Position)(window->swindow.hOrigin),
                 (Position)(window->swindow.vOrigin));

    XtSetArg(wargs[0], XmNvalue, value);
    XtSetValues(nlw->numeric_list.vsb, wargs, 1);

    nlw->numeric_list.selected = nlw->numeric_list.vector_count -
		 (blank_lines + 1);

    for(i = 0; i < nlw->numeric_list.vector_count; i++)
	{
	if (nlw->numeric_list.selected == i)
	    {
	    inverse = True;
	    }
	else
	    {
	    inverse = False;
	    }
	print_vector(nlw, i, inverse, True);
	}
    if (value != max)
	{
	nlw->numeric_list.tid = XtAppAddTimeOut(
		XtWidgetToApplicationContext((Widget)nlw),
		    100, (XtTimerCallbackProc)increment_if_possible, nlw);
	}
}

/*  Subroutine:	XmCreateNumericList
 *  Purpose:	This function creates and returns a NumericList widget.
 */
Widget XmCreateNumericList( Widget parent, String name,
			 ArgList args, Cardinal num_args )
{
    return XtCreateWidget(name, xmNumericListWidgetClass, parent, args, num_args);
}
/************************************************************************
 *                                                                      *
 * XmCreateScrolledNumericList - create a list inside of a scrolled window.*
 *                                                                      *
 ************************************************************************/
Widget XmCreateScrolledNumericList(parent, name, args, argCount)
Widget   parent;
char     *name;
ArgList  args;
int      argCount;

{
XmNumericListWidget nlw;
XmScrolledWindowWidget 	sw;
int 	n;
int 	i;
char 	*s;
Arg	wargs[100];
Arg	wargs2[100];

    s = XtMalloc(STRLEN(name) + 3);     /* Name + NULL + "SW" */
    strcpy(s, name);
    strcat(s, "SW");

    i = 0;
    for(n = 0; n < argCount; n++)
	{
	wargs[n] = args[n];
	if ( (STRCMP(wargs[n].name,XmNwidth) == 0) || 
	     (STRCMP(wargs[n].name,XmNheight) == 0) )
	    {
	    wargs2[i] = args[n]; i++;
	    }
	}

    XtSetArg (wargs2[i], XmNscrollingPolicy, XmAUTOMATIC); i++;
    XtSetArg (wargs2[i], XmNscrollBarDisplayPolicy, (XtArgVal )XmAS_NEEDED);i++;
    sw = (XmScrolledWindowWidget)XmCreateScrolledWindow(parent, s, wargs2, i);
    XtManageChild((Widget)sw);
    XtFree(s);

    nlw = (XmNumericListWidget)XmCreateNumericList((Widget)sw, name, wargs, n);

    nlw->numeric_list.scrolled_window = True;
    nlw->numeric_list.sw = sw;
    return ((Widget)nlw);
}
/*********************************************************************
* XmNumericListAddVector - Add a vector ABOVE the indicated vector
*	       
**********************************************************************/
Boolean XmNumericListAddVector(XmNumericListWidget nlw, 
				Vector vector, int vector_num)
{
int		i;
int		j;
int		k;
VectorList	vlist;
XEvent		*event=NULL;
Boolean		do_jump = False;
    
    /* 
     *  Check that the vector_num is in range
     */
    if (vector_num >= nlw->numeric_list.vector_count)
	{
	XtWarning("Illegal position value encountered in "
		  "XmNumericListAddVector.");
	return(0);
	}

    /* 
     * Alloc the new memory and copy the vectors over, inserting the new vector
     */
    vlist = (VectorList)XtMalloc(sizeof(Vector) * 
		(nlw->numeric_list.vector_count + 1));

    /*
     * Special Case - when vector_num is -1, add the vector at the end of the
     *		vector list.
     */
    if (vector_num == -1)
	{
	for (i=0; i < nlw->numeric_list.vector_count; i++)
	    {
	    vlist[i] = nlw->numeric_list.local_vectors[i];
	    }
	vlist[i] = (Vector)XtMalloc(sizeof(double) * nlw->numeric_list.tuples);
	for (j=0; j < nlw->numeric_list.tuples; j++)
	    {
	    *(vlist[i] + j) = vector[j];
	    }
	vector_num = nlw->numeric_list.vector_count;
	do_jump = True;
	}
    else
	{
	k = 0;
	for (i=0; i < nlw->numeric_list.vector_count; i++)
	    {
	    if (i == vector_num)
		{
	    	vlist[k] = 
		    (Vector)XtMalloc(sizeof(double) * nlw->numeric_list.tuples);
		for (j=0; j < nlw->numeric_list.tuples; j++)
		    {
		    *(vlist[k] + j) = vector[j];
		    }
		k++;
		}
	    vlist[k] = nlw->numeric_list.local_vectors[i];
	    k++;
	    }
	}
    /* 
     * Free the old vectors
     */
    XtFree((char*)nlw->numeric_list.local_vectors);

    /*
     * Install the new vectors
     */
    nlw->numeric_list.local_vectors = vlist;

    /* 
     *  Bump up the vector count
     */
    nlw->numeric_list.vector_count++;

    nlw->numeric_list.vectors = NULL;

    /*
     * Regenerate the display 
     */
    XClearWindow(XtDisplay(nlw), XtWindow(nlw));
    do_layout(nlw);
    Redisplay(nlw, event);
    if (do_jump)
	{
	jump_to_end(nlw);
	}
    return(1);
}
/*********************************************************************
* XmNumericListDeleteVector - Delete the indicated vector
*	       
**********************************************************************/
Boolean XmNumericListDeleteVector(XmNumericListWidget nlw, int vector_num)
{
int		i;
int		j;
int		k;
XEvent		*event=NULL;
VectorList	vlist;
    
    /* 
     *  Check that the vector_num is in range
     */
    if (vector_num > nlw->numeric_list.vector_count - 1)
	{
	XtWarning("Illegal position value encountered in "
		  "XmNumericListDeleteVector.");
	return(0);
	}

    /* 
     *  Special case where #vectors = 1
     */
    if (nlw->numeric_list.vector_count == 1)
	{
	XtFree((char*)nlw->numeric_list.local_vectors[0]);
	XtFree((char*)nlw->numeric_list.local_vectors);
	vlist = NULL;
	}
    else
	{
	/* 
	 * Alloc the new memory and copy the vectors over, delete the vector
	 */
	vlist = (VectorList)XtMalloc(sizeof(Vector) * 
		(nlw->numeric_list.vector_count - 1));
	k = 0;
	for (i=0; i < nlw->numeric_list.vector_count; i++)
	    {
	    if (i == vector_num)
		{
		continue;
		}
	    vlist[k] = 
		(Vector)XtMalloc(sizeof(double) * nlw->numeric_list.tuples);
	    for (j=0; j < nlw->numeric_list.tuples; j++)
		{
		*(vlist[k] + j) = *(nlw->numeric_list.local_vectors[i] + j);
		}
	    k++;
	    }
	/* 
	 * Free the old vectors
	 */
	if(nlw->numeric_list.local_vectors)
	    {
	    for (i=0; i < nlw->numeric_list.vector_count; i++)
		{
		XtFree((char*)nlw->numeric_list.local_vectors[i]);
		}
	    XtFree((char*)nlw->numeric_list.local_vectors);
	    }
	}
    /* 
     *  Knock down the vector count
     */
    nlw->numeric_list.vector_count--;

    nlw->numeric_list.local_vectors = vlist;
    nlw->numeric_list.vectors = NULL;

    /*
     * Regenerate the display (good candidate for optimization)
     */
    if (nlw->numeric_list.selected >= nlw->numeric_list.vector_count)
	{
	nlw->numeric_list.selected = nlw->numeric_list.vector_count - 1;
	}

    XClearWindow(XtDisplay(nlw), XtWindow(nlw));
    do_layout(nlw);
    Redisplay(nlw, event);
    return(1);
}
/*********************************************************************
* XmNumericListReplaceSelectedVector - Delete the indicated vector
*	       
**********************************************************************/
Boolean XmNumericListReplaceSelectedVector(XmNumericListWidget nlw, 
						Vector newvec)
{
int		j, vecnum;
XEvent		*event=NULL;
    
    /* 
     *  Check that there is a selected item 
     */
    vecnum = nlw->numeric_list.selected;
    if (vecnum == -1)
	{
	XtWarning("No selected item in XmNumericListReplaceSelectedVector.");
	return(0);
	}
 
    for (j=0; j < nlw->numeric_list.tuples; j++)
    {
	nlw->numeric_list.local_vectors[vecnum][j] = newvec[j];  
    }

    XClearWindow(XtDisplay(nlw), XtWindow(nlw));
    do_layout(nlw);
    Redisplay(nlw, event);
    return(1);
}

#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif

