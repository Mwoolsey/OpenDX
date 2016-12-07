/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


/*	Construct and manage the "frame sequencer guide" to be used as a
 *	popup of the "sequence controller" (alias VCR control)
 *
 *	August 1990
 *	IBM T.J. Watson Research Center
 *	R. T. Maganti
 */

/*
 */

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/Xutil.h>
#include <Xm/DrawingA.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/SeparatoGP.h>

extern void _XmDispatchGadgetInput(Widget g, XEvent *event, Mask mask);

#include "SlideBar.h"
#include "SlideBarP.h"


#define ARROW_WIDTH 16
#define ARROW_HEIGHT 16


static void Initialize( XmSlideBarWidget request, XmSlideBarWidget new );
static Boolean SetValues( XmSlideBarWidget current,
			  XmSlideBarWidget request,
			  XmSlideBarWidget new );
static void Realize( register Widget w, Mask *p_valueMask,
		     XSetWindowAttributes *attributes );
static void Destroy( XmSlideBarWidget w );
static void FreeMarker( XmSlideBarWidget w, Marker* mark, Marker* ref );
static void Redisplay( XmSlideBarWidget w, XEvent* event, Region region );
static void Resize( XmSlideBarWidget w );
static void Help( XmSlideBarWidget w, XEvent* event );
static void Select( XmSlideBarWidget w, XEvent* event );
static void Move( XmSlideBarWidget w, XEvent* event );
static void Release( XmSlideBarWidget w, XButtonEvent* event );
static void MoveMarker( XmSlideBarWidget w, Marker* marker, short x,int reason);
static void Increment( XmSlideBarWidget w, XEvent* event,
		       char** argv, int argc );
static void CallSlideBarValueCallback( XmSlideBarWidget w, Marker* marker,
				       Boolean activate );
static void ClearMarker( XmSlideBarWidget w, Marker* marker,
			 short x, short width );
static void PaintMarker( XmSlideBarWidget w, Marker* marker, Boolean in );
static void DrawMarker( XmSlideBarWidget w, Marker* marker, Boolean in );
static void InitMarkers( XmSlideBarWidget w );
static void SetMarkerParams( Marker* marker, GC cenGC, Arrow arrow,
			     short y, short width, short height );
static void SetArrowCoords( register Arrow arrow, short x, short y );
static Pixmap CreateArrowPixmap( XmSlideBarWidget w, GC gc,
				 short width, short height );
static void FillArrowPixmap( XmSlideBarWidget w, struct ArrowRec* arrow,
                             GC cenGC, Boolean in, Pixmap pixmap );
static void ChangeSlideBarStart( XmSlideBarWidget w, short detent );
static void ChangeSlideBarStop( XmSlideBarWidget w, short detent );
static void ChangeSlideBarNext( XmSlideBarWidget w, short detent );
static void ChangeSlideBarCurrent( XmSlideBarWidget w, short detent );
static void GetArrowDrawRects( int  shadow_thickness,
			       int  core_width,
			       int  core_height,
			       short  *top_count,
			       short  *cent_count,
			       short  *bot_count,
			       XRectangle **top,
			       XRectangle **cent,
			       XRectangle **bot );
static void RecomputeSlideBarPositions( XmSlideBarWidget w );


extern void _XmForegroundColorDefault();
extern void _XmBackgroundColorDefault();


static XtResource resources[] =
{
    {
      XmNvalueCallback, XmCValueCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmSlideBarWidget, slide_bar.value_callback),
      XmRCallback, NULL
    },
    {
      XmNcurrentColor, XmCColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmSlideBarWidget, slide_bar.current_mark_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNlimitColor, XmCColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmSlideBarWidget, slide_bar.limit_mark_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNmarkColor, XmCColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmSlideBarWidget, slide_bar.value_mark_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNstart, XmCStart, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.start_mark.detent),
      XmRImmediate, (XtPointer) 0
    },
    {
      XmNstop, XmCStop, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.stop_mark.detent),
      XmRImmediate, (XtPointer) 10
    },
    {
      XmNnext, XmCNext, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.next_mark.detent),
      XmRImmediate, (XtPointer) 5
    },
    {
      XmNcurrent, XmCCurrent, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.current_mark.detent),
      XmRImmediate, (XtPointer) 5
    },
    {
      XmNminValue, XmCMinValue, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.min_detent),
      XmRImmediate, (XtPointer) 0
    },
    {
      XmNmaxValue, XmCMaxValue, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.max_detent),
      XmRImmediate, (XtPointer) 10
    },
    {
      XmNxMargin, XmCXMargin, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.x_margin),
      XmRImmediate, (XtPointer) 20
    },
    {
      XmNarrowSize, XmCArrowSize, XmRShort, sizeof(short),
      XtOffset(XmSlideBarWidget, slide_bar.arrow_height),
      XmRImmediate, (XtPointer) 16
    },
    {
      XmNalignOnDrop, XmCAlignOnDrop, XmRBoolean, sizeof(Boolean),
      XtOffset(XmSlideBarWidget, slide_bar.drop_to_detent),
      XmRImmediate, (XtPointer) False
    },
    {
      XmNjumpToGrab, XmCJumpToGrab, XmRBoolean, sizeof(Boolean),
      XtOffset(XmSlideBarWidget, slide_bar.jump_to_grab),
      XmRImmediate, (XtPointer) True
    },
    {
      XmNallowInput, XmCAllowInput, XmRBoolean, sizeof(Boolean),
      XtOffset(XmSlideBarWidget, slide_bar.sensitive),
      XmRImmediate, (XtPointer) True
    },
    {
      XmNcurrentVisible, XmCCurrentVisible, XmRBoolean, sizeof(Boolean),
      XtOffset(XmSlideBarWidget, slide_bar.current_visible),
      XmRImmediate, (XtPointer) True
    },
};


extern void _XmPrimitiveEnter();
extern void _XmPrimitiveLeave();
extern void _XmManagerFocusIn();


static char defaultTranslations[] =
    "<Btn1Down>:	Select()\n\
     <Btn1Up>:		Release()\n\
     Button1<PtrMoved>:	Moved()\n\
     <Key>Left:		Increment(-1)\n\
     <Key>Right:	Increment(1)\n\
     <EnterWindow>:	Enter()\n\
     <LeaveWindow>:	Leave()";


static XtActionsRec actionsList[] =
{
  { "Select",    (XtActionProc) Select		  },
  { "Release",   (XtActionProc) Release		  },
  { "Moved",     (XtActionProc) Move		  },
  { "Increment", (XtActionProc) Increment	  },
  { "Help",      (XtActionProc) Help		  },
  { "Enter",     (XtActionProc) _XmPrimitiveEnter },
  { "Leave",     (XtActionProc) _XmPrimitiveLeave },
  { "FocusIn",	 (XtActionProc) _XmManagerFocusIn },
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

static CompositeClassExtensionRec managerCompositeExt =
{
    /* next_extension */  NULL,
    /* record_type    */  NULLQUARK,
    /* version        */  XtCompositeExtensionVersion,
    /* record_size    */  sizeof(CompositeClassExtensionRec),
    /* accepts_objects */ True,
#if XtSpecificationRelease >= 6
    /* allows_change_managed_set */ True
#endif
};
 
XmSlideBarClassRec xmSlideBarClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmDrawingAreaClassRec,	/* superclass         */
      "XmSlideBar",				/* class_name         */
      sizeof(XmSlideBarRec),			/* widget_size        */
      NULL,					/* class_initialize   */
      NULL,					/* class_part_init    */
      FALSE,					/* class_inited       */
      (XtInitProc) Initialize,			/* initialize         */
      NULL,					/* initialize_hook    */
/*      (XtWidgetProc) Realize,			 realize            */
      (XtRealizeProc) Realize,			/* realize            */
      actionsList,				/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      FALSE,					/* compress_motion    */
      FALSE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      True,					/* visible_interest   */
      (XtWidgetProc) Destroy,			/* destroy            */
      (XtWidgetProc) Resize,			/* resize             */
      (XtExposeProc) Redisplay,			/* expose             */
      (XtSetValuesFunc) SetValues,		/* set_values         */
      NULL,					/* set_values_hook    */
      (XtAlmostProc)_XtInherit,			/* set_values_almost  */
      NULL,					/* get_values_hook    */
      NULL,					/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      defaultTranslations,			/* tm_table           */
      XtInheritQueryGeometry,			/* query_geometry     */
      NULL,             	                /* display_accelerator   */
      NULL,		                        /* extension             */
   },

   {		/* composite_class fields */
      XtInheritGeometryManager,			/* geometry_manager   */
      (XtWidgetProc)_XtInherit,			/* change_managed     */
      (XtWidgetProc)_XtInherit,			/* insert_child       */
      (XtWidgetProc)_XtInherit,			/* delete_child       */
      (XtPointer)&managerCompositeExt,          /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      0,					/* constraint size      */   
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

   {		/* drawing area class - none */
      NULL,					/* mumble */
   },

   {		/* slide bar class - none */
      NULL					/* mumble */
   }
};


WidgetClass xmSlideBarWidgetClass = (WidgetClass) &xmSlideBarClassRec;


/*  Subroutine:	Initialize
 *  Effect:	Create and initialize the component widgets
 */
static void Initialize( XmSlideBarWidget request, XmSlideBarWidget new )
{
    Arg wargs[4];

    new->core.height = 3 * new->slide_bar.arrow_height;
    new->slide_bar.width = new->core.width - (2 * new->slide_bar.x_margin);
    new->slide_bar.num_detents = new->slide_bar.max_detent - 
				new->slide_bar.min_detent;
    if(new->slide_bar.num_detents == 0) new->slide_bar.num_detents = 1;
    if( new->slide_bar.stop_mark.detent < new->slide_bar.start_mark.detent )
	new->slide_bar.stop_mark.detent = new->slide_bar.max_detent;
    RecomputeSlideBarPositions(new);
    /*  Set flag to do size dependent initialization after windoe expose  */
    new->slide_bar.init = TRUE;
    new->slide_bar.arrow_width = new->slide_bar.arrow_height;
    /*  Clear some things lest we try to access them  */
    new->slide_bar.start_mark.in_pixmap = None;
    new->slide_bar.start_mark.out_pixmap = None;
    new->slide_bar.start_mark.cenGC = NULL;
    new->slide_bar.start_mark.arrow = NULL;
    new->slide_bar.next_mark.in_pixmap = None;
    new->slide_bar.next_mark.out_pixmap = None;
    new->slide_bar.next_mark.cenGC = NULL;
    new->slide_bar.next_mark.arrow = NULL;
    new->slide_bar.current_mark.in_pixmap = None;
    new->slide_bar.current_mark.out_pixmap = None;
    new->slide_bar.current_mark.cenGC = NULL;
    new->slide_bar.current_mark.arrow = NULL;
    new->slide_bar.stop_mark.in_pixmap = None;
    new->slide_bar.stop_mark.out_pixmap = None;
    new->slide_bar.stop_mark.cenGC = NULL;
    new->slide_bar.stop_mark.arrow = NULL;
    new->slide_bar.provisional_grab = -1;
    new->slide_bar.grabbed = NULL;

    new->slide_bar.y_margin =
      (new->core.height - (new->slide_bar.arrow_height)) / 2;

    XtSetArg(wargs[0], XmNx, new->slide_bar.x_margin);
    XtSetArg(wargs[1], XmNy, new->slide_bar.y_margin+new->slide_bar.arrow_height); 
    XtSetArg(wargs[2], XmNwidth, new->slide_bar.width);
    XtSetArg(wargs[3], XmNshadowThickness, 2);

    new->slide_bar.line = 
	(XmSeparatorGadget)XmCreateSeparatorGadget((Widget)new, 
			"line", wargs, 4);
    XtManageChild((Widget)new->slide_bar.line);
}


/*  Subroutine:	SetValues
 *  Effect:	Handles requests to change things from the application
 */
static Boolean SetValues( XmSlideBarWidget current,
			  XmSlideBarWidget request,
			  XmSlideBarWidget new )
{
    Boolean redraw = FALSE;
    Arrow	arrow;

    if( new->slide_bar.arrow_width != new->slide_bar.arrow_height )
	new->slide_bar.arrow_width = new->slide_bar.arrow_height;
    if (new->slide_bar.start_mark.detent != 
	current->slide_bar.start_mark.detent)
	{
	ChangeSlideBarStart(new,new->slide_bar.start_mark.detent);
	}
    if (new->slide_bar.stop_mark.detent != 
	current->slide_bar.stop_mark.detent)
	{
	ChangeSlideBarStop(new,new->slide_bar.stop_mark.detent);
	}
    if (new->slide_bar.next_mark.detent != 
	current->slide_bar.next_mark.detent)
	{
	ChangeSlideBarNext(new,new->slide_bar.next_mark.detent);
	}
    if (new->slide_bar.current_mark.detent != 
	current->slide_bar.current_mark.detent)
	{
	ChangeSlideBarCurrent(new,new->slide_bar.current_mark.detent);
	}
    if (new->slide_bar.min_detent !=
        current->slide_bar.min_detent)
	{
	new->slide_bar.num_detents = (new->slide_bar.max_detent - 
				      new->slide_bar.min_detent);
	if(new->slide_bar.num_detents == 0) new->slide_bar.num_detents = 1;
	if (new->slide_bar.min_detent > new->slide_bar.start_mark.detent)
	    {
	    new->slide_bar.start_mark.detent = new->slide_bar.min_detent;
	    }
	if (new->slide_bar.min_detent > new->slide_bar.next_mark.detent)
	    {
	    new->slide_bar.next_mark.detent = new->slide_bar.min_detent;
	    }
	Resize(new);
	}
    if (new->slide_bar.max_detent !=
        current->slide_bar.max_detent)
	{
	new->slide_bar.num_detents = (new->slide_bar.max_detent - 
				      new->slide_bar.min_detent);
	if(new->slide_bar.num_detents == 0) new->slide_bar.num_detents = 1;
	if (new->slide_bar.max_detent < new->slide_bar.stop_mark.detent)
	    {
	    new->slide_bar.stop_mark.detent = new->slide_bar.max_detent;
	    }
	if (new->slide_bar.max_detent < new->slide_bar.next_mark.detent)
	    {
	    new->slide_bar.next_mark.detent = new->slide_bar.max_detent;
	    }
	Resize(new);
	}
    if(new->slide_bar.value_mark_color != current->slide_bar.value_mark_color)
	{
	XSetForeground(XtDisplay(new), new->slide_bar.next_mark.cenGC,
			new->slide_bar.value_mark_color);
	arrow = (Arrow)XtCalloc(1, sizeof(struct ArrowRec));
	/*  2 represents shadow thickness and initial offset */
	GetArrowDrawRects(2,new->slide_bar.arrow_width, 
			new->slide_bar.arrow_height, &arrow->top_count, 
			&arrow->cen_count, &arrow->bot_count,
		      	&arrow->top, &arrow->cen, &arrow->bot);
	/*  Set reference values for establishing window location  *
	*  n + base - top[0] puts arrow at n of window	       */
	arrow->base_x = arrow->top[0].x - (new->slide_bar.arrow_width / 2);
	arrow->base_y = arrow->top[0].y;
    	XFillRectangles(XtDisplay(new), new->slide_bar.next_mark.in_pixmap, 
	       new->slide_bar.next_mark.cenGC, arrow->cen, arrow->cen_count);
    	XFillRectangles(XtDisplay(new), new->slide_bar.next_mark.out_pixmap, 
	       new->slide_bar.next_mark.cenGC, arrow->cen, arrow->cen_count);
	XtFree((char*)arrow->top);
	XtFree((char*)arrow->cen);
	XtFree((char*)arrow->bot);
	XtFree((char*)arrow);
	Resize(new);
	}
    if(new->slide_bar.limit_mark_color != current->slide_bar.limit_mark_color)
	{
	XSetForeground(XtDisplay(new), new->slide_bar.start_mark.cenGC,
			new->slide_bar.limit_mark_color);
	arrow = (Arrow)XtCalloc(1, sizeof(struct ArrowRec));
	/*  2 represents shadow thickness and initial offset */
	GetArrowDrawRects(2,new->slide_bar.arrow_width, 
			new->slide_bar.arrow_height, &arrow->top_count, 
			&arrow->cen_count, &arrow->bot_count,
		      	&arrow->top, &arrow->cen, &arrow->bot);
	/*  Set reference values for establishing window location  *
	*  n + base - top[0] puts arrow at n of window	       */
	arrow->base_x = arrow->top[0].x - (new->slide_bar.arrow_width / 2);
	arrow->base_y = arrow->top[0].y;
    	XFillRectangles(XtDisplay(new), new->slide_bar.start_mark.in_pixmap, 
	       new->slide_bar.start_mark.cenGC, arrow->cen, arrow->cen_count);
    	XFillRectangles(XtDisplay(new), new->slide_bar.start_mark.out_pixmap, 
	       new->slide_bar.start_mark.cenGC, arrow->cen, arrow->cen_count);
	XtFree((char*)arrow->top);
	XtFree((char*)arrow->cen);
	XtFree((char*)arrow->bot);
	XtFree((char*)arrow);
	Resize(new);
	}
    if(new->slide_bar.current_mark_color!=current->slide_bar.current_mark_color)
	{
	XSetForeground(XtDisplay(new), new->slide_bar.current_mark.cenGC,
			new->slide_bar.current_mark_color);
	arrow = (Arrow)XtCalloc(1, sizeof(struct ArrowRec));
	/*  2 represents shadow thickness and initial offset */
	GetArrowDrawRects(2,new->slide_bar.arrow_width, 
			new->slide_bar.arrow_height, &arrow->top_count, 
			&arrow->cen_count, &arrow->bot_count,
		      	&arrow->top, &arrow->cen, &arrow->bot);
	/*  Set reference values for establishing window location  *
	*  n + base - top[0] puts arrow at n of window	       */
	arrow->base_x = arrow->top[0].x - (new->slide_bar.arrow_width / 2);
	arrow->base_y = arrow->top[0].y;
    	XFillRectangles(XtDisplay(new), new->slide_bar.current_mark.in_pixmap, 
	       new->slide_bar.current_mark.cenGC, arrow->cen, arrow->cen_count);
    	XFillRectangles(XtDisplay(new), new->slide_bar.current_mark.out_pixmap, 
	       new->slide_bar.current_mark.cenGC, arrow->cen, arrow->cen_count);
	XtFree((char*)arrow->top);
	XtFree((char*)arrow->cen);
	XtFree((char*)arrow->bot);
	XtFree((char*)arrow);
	Resize(new);
	}
    if (new->slide_bar.current_visible != current->slide_bar.current_visible)
	{
	if (XtIsRealized((Widget)new))
	    {
	    if (new->slide_bar.current_visible)
	        {
	        DrawMarker(new, &new->slide_bar.current_mark, FALSE);
	        }
	    else
	        {
	        ClearMarker(new, &new->slide_bar.current_mark, 
			new->slide_bar.current_mark.x, 
			new->slide_bar.next_mark.width);
	        }
	    XFlush(XtDisplay(new));
	    }
	}
    return redraw;
}


/*  Subroutine:	Realize
 *  Purpose:	Create the window with PointerMotion events and None gravity
 */
static void Realize( register Widget w, Mask *p_valueMask,
		     XSetWindowAttributes *attributes )
{
    Mask valueMask = *p_valueMask;

    valueMask |= CWDontPropagate;
    attributes->do_not_propagate_mask = 
      ButtonPressMask | ButtonReleaseMask |
      KeyPressMask | KeyReleaseMask | PointerMotionMask;
    XtCreateWindow(w, InputOutput, CopyFromParent, valueMask, attributes);
    XFlush(XtDisplay(w));
    InitMarkers((XmSlideBarWidget)w);
}


/*  Subroutine:	Destroy
 *  Effect:	Frees resources before widget is destroyed
 */
static void Destroy( XmSlideBarWidget w )
{
    /*  Free memory and GC's  */
    FreeMarker(w, &w->slide_bar.next_mark,    &w->slide_bar.start_mark);
    FreeMarker(w, &w->slide_bar.current_mark, &w->slide_bar.start_mark);
    FreeMarker(w, &w->slide_bar.stop_mark,    &w->slide_bar.start_mark);
    FreeMarker(w, &w->slide_bar.start_mark, NULL);
    XtRemoveAllCallbacks((Widget)w, XmNvalueCallback);

}
static void FreeMarker( XmSlideBarWidget w, Marker* mark, Marker* ref )
{
    if(mark->out_pixmap
       && ((ref == NULL) || (mark->out_pixmap != ref->out_pixmap)) )
    {
	XFreePixmap(XtDisplay(w), mark->out_pixmap);
    }
    if(   mark->in_pixmap
       && ((ref == NULL) || (mark->in_pixmap != ref->in_pixmap)) )
    {
	XFreePixmap(XtDisplay(w), mark->in_pixmap);
    }
    if(   mark->cenGC && ((ref == NULL) || (mark->cenGC != ref->cenGC)) )
    {
	XtReleaseGC((Widget)w, mark->cenGC);
    }
    if(   mark->arrow
       && ((ref == NULL) || (mark->arrow != ref->arrow)) )
    {
	XtFree((char*)mark->arrow->top);
	XtFree((char*)mark->arrow->bot);
	XtFree((char*)mark->arrow->cen);
	XtFree((char*)mark->arrow);
    }
    mark->arrow = NULL; 
    mark->cenGC = (GC)NULL; 
    mark->out_pixmap = (Pixmap)NULL; 
    mark->in_pixmap = (Pixmap)NULL; 
}


/*  Subroutine:	Redisplay
 *  Purpose:	Calculate the drawing area and redraw the widget
 */
static void Redisplay( XmSlideBarWidget w, XEvent* event, Region region )
{
Widget child;

    /*  We could filter on location or index 0.  Location seems to yield     *
     *  fewer cases while still ensuring at least one expose event. 6 v. 14  *
     *  on initial start-up.					             *
     *  Filtering on both does not guarantee at least one redraw / exposure  */
    if( w->slide_bar.init )
    {
	w->slide_bar.init = FALSE;
	Resize(w);
    }
    if( event )
	{
	/*  Redisplay our separator - Motif did not do this by itself, nor
	    would it work by calling _XmRedisplayGadgets, so I took matters
	    into my own hands and called the gadgets expose routine myself - 
	    JGV  */
	child = (Widget)w->slide_bar.line;
        (*(child->core.widget_class->core_class.expose))
                     (child, event, region);

	}
    DrawMarker(w, &w->slide_bar.start_mark, FALSE);
    DrawMarker(w, &w->slide_bar.stop_mark, FALSE);
    DrawMarker(w, &w->slide_bar.next_mark, FALSE);
    DrawMarker(w, &w->slide_bar.current_mark, FALSE);
}


/*  Subroutine:	Resize
 *  Purpose:	Reposition the components for new size and/or parameters
 */
static void Resize( XmSlideBarWidget w )
{
XEvent e;

    if( XtIsRealized((Widget)w) )
    {
	XClearWindow(XtDisplay(w), w->core.window);
	if(   w->slide_bar.width
	   != (w->core.width - (2 * w->slide_bar.x_margin)) )
	{
	    RectObj g = (RectObj) w->slide_bar.line;
	    XmDropSiteStartUpdate((Widget) w->slide_bar.line);
	    w->slide_bar.width = w->core.width - (2 * w->slide_bar.x_margin);
	    XtResizeWidget((Widget) g, w->slide_bar.width,
			    w->slide_bar.line->rectangle.height, 0);
	}
	if(   ((w->core.height - (w->slide_bar.arrow_height)) / 2)
	   != w->slide_bar.y_margin )
	{
	    RectObj g = (RectObj) w->slide_bar.line;
	    XmDropSiteStartUpdate((Widget)w->slide_bar.line);
	    w->slide_bar.y_margin =
	      (w->core.height - (w->slide_bar.arrow_height)) / 2;
	    XtMoveWidget((Widget) g, 
			  w->slide_bar.line->rectangle.x,
			  w->slide_bar.y_margin + w->slide_bar.arrow_height);

	}
	RecomputeSlideBarPositions(w);
	SetArrowCoords(w->slide_bar.start_mark.arrow,
		       w->slide_bar.x_margin, w->slide_bar.y_margin);
	if( w->slide_bar.next_mark.arrow != w->slide_bar.start_mark.arrow )
	    SetArrowCoords(w->slide_bar.next_mark.arrow,
			   w->slide_bar.x_margin, w->slide_bar.y_margin);
	e.xexpose.x = w->core.x;
	e.xexpose.y = w->core.y;
	e.xexpose.width = w->core.width;
	e.xexpose.height = w->core.height;
	e.xexpose.count = 0;
	Redisplay(w, &e, NULL);
    }
}


/*  Subroutine:	Help
 *  Purpose:	Call application supplied callback (not likely to be used)
 */
static void Help( XmSlideBarWidget w, XEvent* event )
{
   XmGadget gadget;
   XmDrawingAreaCallbackStruct cb;

   if (   (gadget = (XmGadget) _XmInputInGadget((Widget)w, 
		event->xbutton.x, event->xbutton.y))
       != NULL)
   {
      _XmDispatchGadgetInput((Widget)gadget, event, XmHELP_EVENT);
   }
   else
   {
      cb.reason = XmCR_HELP;
      cb.event = event;
      cb.window = XtWindow(w);

      XtCallCallbacks((Widget)w, XmNhelpCallback, &cb);
   }
}


/*  Subroutine:	Select
 *  Purpose:	What to do when user tries to grab a position marker (presses
 *		a mouse button
 */
static void Select( XmSlideBarWidget w, XEvent* event )
{
    short x;

   /*  If user input is not allowed, don't respond  */
   if( w->slide_bar.sensitive == False )
       return;
    x = event->xbutton.x - w->slide_bar.x_margin;
    /*  Grabbing "next" marker right on takes precedence over others  */
    if(   (x > (w->slide_bar.next_mark.x
		- w->slide_bar.next_mark.x_edge_offset))
       && (x <= (w->slide_bar.next_mark.x
		 + w->slide_bar.next_mark.x_edge_offset)) )
    { /* We have pressed Btn1 down on the next mark */
	if( w->slide_bar.jump_to_grab )
	{
	    /*  If grab is in and "next" perfectly overlaps a limit  */
	    if( w->slide_bar.next_mark.x == w->slide_bar.start_mark.x )
	    {
		w->slide_bar.grabbed = &w->slide_bar.start_mark;
		w->slide_bar.provisional_grab = x;
		return;
	    }
	    else if(   w->slide_bar.next_mark.x
		    == w->slide_bar.stop_mark.x )
	    {
		w->slide_bar.grabbed = &w->slide_bar.stop_mark;
		w->slide_bar.provisional_grab = x;
		return;
	    }
	    else
	    {
		w->slide_bar.grabbed = &w->slide_bar.next_mark;
		w->slide_bar.grab_x_offset = 0;
	    }
	}
	else	/*  if( w->slide_bar.jump_to_grab ) == FALSE  */
	{
	    w->slide_bar.grabbed = &w->slide_bar.next_mark;
	    w->slide_bar.grab_x_offset = w->slide_bar.next_mark.x - x;
	}
    }
    /*  Anything from the start marker to the left gets the start marker  */
    else if( x < (w->slide_bar.start_mark.x
		  + w->slide_bar.start_mark.x_edge_offset) )
    {
	w->slide_bar.grabbed = &w->slide_bar.start_mark;
	if( w->slide_bar.jump_to_grab )
	{
	    w->slide_bar.grab_x_offset = 0;
	    MoveMarker(w, w->slide_bar.grabbed, x, XmCR_ARM);
	    return;
	}
	else
	    w->slide_bar.grab_x_offset = w->slide_bar.start_mark.x - x;
    }
    /*  Anything from the stop marker to the right grabs the stop marker  */
    else if( x >= (w->slide_bar.stop_mark.x
		   - w->slide_bar.stop_mark.x_edge_offset) )
    {
	w->slide_bar.grabbed = &w->slide_bar.stop_mark;
	if( w->slide_bar.jump_to_grab )
	{
	    w->slide_bar.grab_x_offset = 0;
	    MoveMarker(w, w->slide_bar.grabbed, x, XmCR_ARM);
	    return;
	}
	else
	    w->slide_bar.grab_x_offset = w->slide_bar.stop_mark.x - x;
    }
    /*  Anything between the start and stop grabs the "next" marker  */
    else
    {
	w->slide_bar.grabbed = &w->slide_bar.next_mark;
	if( w->slide_bar.jump_to_grab )
	{
	    w->slide_bar.grab_x_offset = 0;
	    MoveMarker(w, w->slide_bar.grabbed, x, XmCR_ARM);
	    return;
	}
	else
	    w->slide_bar.grab_x_offset = w->slide_bar.next_mark.x - x;
    }
    /*  Redraw the marker in place, but grabbed (shadow as if pushed in)  */
    PaintMarker(w, w->slide_bar.grabbed, TRUE);
}


/*  Subroutine:	Move
 *  Purpose:	Action routine for pointer motion with button down
 */
static void Move( XmSlideBarWidget w, XEvent* event )
{
    /*  If a marker has been grabbed for moving  */
    if( w->slide_bar.grabbed )
    {
	if( w->slide_bar.jump_to_grab )
	{
	    if( w->slide_bar.provisional_grab >= 0 )
	    {
		if( w->slide_bar.grabbed == &w->slide_bar.start_mark )
		{
		    if(  (event->xmotion.x - w->slide_bar.x_margin)
		       > w->slide_bar.provisional_grab )
		    {
			if(   w->slide_bar.start_mark.x
			   == w->slide_bar.stop_mark.x )
			    w->slide_bar.grabbed = &w->slide_bar.stop_mark;
			else
			    w->slide_bar.grabbed = &w->slide_bar.next_mark;
		    }
		}
		else
		{
		    if(  (event->xmotion.x - w->slide_bar.x_margin)
		       < w->slide_bar.provisional_grab )
		    {
			if(   w->slide_bar.start_mark.x
			   == w->slide_bar.stop_mark.x )
			    w->slide_bar.grabbed = &w->slide_bar.start_mark;
			else
			    w->slide_bar.grabbed = &w->slide_bar.next_mark;
		    }
		}
		w->slide_bar.provisional_grab = -1;
	    }
	}
	/*  Convert x coordinate to marker space  */
	MoveMarker(w, w->slide_bar.grabbed,
		   (event->xmotion.x
		    + w->slide_bar.grab_x_offset - w->slide_bar.x_margin), 
			XmCR_DRAG);
    }
}


/*  Subroutine:	Release
 *  Purpose:	What to do when the user releases a position marker
 */
/* ARGSUSED */
static void Release( XmSlideBarWidget w, XButtonEvent* event )
{
    int x;
    if( w->slide_bar.grabbed )
    {
	if( w->slide_bar.drop_to_detent )
	{
	    /*  Adust to detent position  */
	    x = ( (w->slide_bar.grabbed->detent - 
			w->slide_bar.min_detent)* w->slide_bar.width)
	      / w->slide_bar.num_detents;
	    if( x != w->slide_bar.grabbed->x )
		MoveMarker(w, w->slide_bar.grabbed, x, XmCR_DISARM);
	}
	/*  If the start limit was moved after the next one, move next  */
	if( w->slide_bar.grabbed == &w->slide_bar.start_mark )
	{
	    if( w->slide_bar.start_mark.x > w->slide_bar.next_mark.x )
	    {
		ClearMarker(w, &w->slide_bar.next_mark,
			    w->slide_bar.next_mark.x,
			    w->slide_bar.next_mark.width);
		w->slide_bar.next_mark.detent =
		  w->slide_bar.start_mark.detent;
		w->slide_bar.next_mark.x = w->slide_bar.start_mark.x;
		CallSlideBarValueCallback(w, &w->slide_bar.next_mark,
					      XmCR_DISARM);

	    }
	}
	/*  If the stop limit was moved before the current one, move next  */
	else if( w->slide_bar.grabbed == &w->slide_bar.stop_mark )
	{
	    if( w->slide_bar.stop_mark.x < w->slide_bar.next_mark.x )
	    {
		ClearMarker(w, &w->slide_bar.next_mark,
			    w->slide_bar.next_mark.x,
			    w->slide_bar.next_mark.width);
		w->slide_bar.next_mark.detent =
		  w->slide_bar.stop_mark.detent;
		w->slide_bar.next_mark.x = w->slide_bar.stop_mark.x;
		CallSlideBarValueCallback(w, &w->slide_bar.next_mark,
					      XmCR_DISARM);
	    }
	}
	/*  Redraw all four markers just to be sure they appear correctly  */
	PaintMarker(w, &w->slide_bar.start_mark, FALSE);
	DrawMarker(w, &w->slide_bar.stop_mark, FALSE);
	DrawMarker(w, &w->slide_bar.next_mark, FALSE);
	DrawMarker(w, &w->slide_bar.current_mark, FALSE);
	/*  Call callback with final value  */
	CallSlideBarValueCallback(w, w->slide_bar.grabbed, XmCR_DISARM);
	w->slide_bar.grabbed = NULL;
    }
}


/*  Subroutine: MoveMarker
 *  Purpose:	What to do as user tries to move a position marker
 */
static void MoveMarker( XmSlideBarWidget w, Marker* marker, short x,int reason )
{
    short width;
    if( marker )
    {
	/*  Don't go beyond edge of slider bar  */
	if( x < 0 )
	    x = 0;
	else if( x >= w->slide_bar.width )
	    x = w->slide_bar.width - 1;
	/*  Don't go beyond start or stop limit (except the limit, itself)  */
	if( marker != &w->slide_bar.start_mark )
	{
	    if( x < w->slide_bar.start_mark.x )
		x = w->slide_bar.start_mark.x;
	}
	if( marker != &w->slide_bar.stop_mark )
	{
	    if( x > w->slide_bar.stop_mark.x )
		x = w->slide_bar.stop_mark.x;
	}
	/*  Movement of less than a marker width should only clear area ... *
	 *  being exposed.  This eliminates apparent flicker when moving.   */
	width = x - marker->x;
	if( (width < marker->width) && (-width < marker->width) )
	{
	    if( width > 0 )
		ClearMarker(w, marker, marker->x, width);
	    else if( width < 0 )
		ClearMarker(w, marker,
			    (marker->x + (marker->width + width)), -width);
	    else
		/*  Case of no movement  */
		return;
	}
	else
	    /*  Clear entire marker  */
	    ClearMarker(w, marker, marker->x, marker->width);
	w->slide_bar.grabbed->x = x;
	/*  The +(w->slide_bar.width/2) assures rounding for int math div  */
	w->slide_bar.grabbed->detent = w->slide_bar.min_detent +
	  ((x * w->slide_bar.num_detents)
	   + (w->slide_bar.width / 2)) / w->slide_bar.width;
	if( marker == w->slide_bar.grabbed )
	    PaintMarker(w, marker, TRUE);
	else
	    DrawMarker(w, marker, FALSE);
	/*  Call callback with running value  */
	CallSlideBarValueCallback(w, w->slide_bar.grabbed, reason);
    }
}

/*  Subroutine:	Increment
 *  Purpose:	Move the slider by a fixed value (from accelerator key stroke)
 */
static void Increment( XmSlideBarWidget w, XEvent* event,
		       char** argv, int argc )
{
   int increment, new_detent;
   Marker* marker;

   increment = atoi(*argv);

   /*  If user input is not allowed, don't respond  */
   if( w->slide_bar.sensitive == False )
       return;
   if( w->slide_bar.grabbed )
       marker = w->slide_bar.grabbed;
   else
       marker = &w->slide_bar.next_mark;
   new_detent = marker->detent + increment;
   if(   (new_detent < w->slide_bar.min_detent)
      || (new_detent > w->slide_bar.max_detent)
      || (   (marker != &w->slide_bar.start_mark)
	  && (new_detent < w->slide_bar.start_mark.detent))
      || (   (marker != &w->slide_bar.stop_mark)
	  && (new_detent > w->slide_bar.stop_mark.detent)) )
       return;
   if( marker == &w->slide_bar.next_mark )
       ChangeSlideBarNext(w, new_detent);
   else if( marker == &w->slide_bar.start_mark )
       ChangeSlideBarStart(w, new_detent);
   else if( marker == &w->slide_bar.stop_mark )
       ChangeSlideBarStop(w, new_detent);
   CallSlideBarValueCallback(w, marker, XmCR_DISARM);
}


/*  Subroutine:	CallSlidebarValueCallback
 *  Purpose:	Inform client of what is happening as values change
 */
static void CallSlideBarValueCallback( XmSlideBarWidget w, Marker* marker,
				       Boolean reason )
{
    SlideBarCallbackStruct call_data;
    call_data.value = marker->detent;
    call_data.reason = reason;
    if( marker == &w->slide_bar.start_mark )
	call_data.indicator = XmCR_START;
    else if( marker == &w->slide_bar.stop_mark )
	call_data.indicator = XmCR_STOP;
    else if( marker == &w->slide_bar.next_mark )
	call_data.indicator = XmCR_NEXT;
    XtCallCallbacks((Widget)w, XmNvalueCallback, &call_data);
}


/*  Subroutine:	ClearMarker
 *  Purpose:	Clear an area (under a marker being moved) and redraw any
 *		other markers which also occupy that area
 */
static void ClearMarker( XmSlideBarWidget w, Marker* marker,
			 short x, short width )
{
    if (marker->arrow == NULL)
	{
	return;
	}
    XClearArea(XtDisplay(w), w->core.window,
	       x + w->slide_bar.x_margin - marker->x_edge_offset,
	       w->slide_bar.y_margin, width, w->slide_bar.arrow_height, FALSE);
    /*  If part of another marker was erased, redraw it  */
    if(   (marker != &w->slide_bar.start_mark)
       && (x < (w->slide_bar.start_mark.x + w->slide_bar.arrow_width)) )
	DrawMarker(w, &w->slide_bar.start_mark, FALSE);

    if(   (marker != &w->slide_bar.stop_mark)
       && ((x + width) > w->slide_bar.stop_mark.x) )
	DrawMarker(w, &w->slide_bar.stop_mark, FALSE);

    if(   (marker != &w->slide_bar.next_mark)
       && ((x + width) > w->slide_bar.next_mark.x)
       && (x < (w->slide_bar.next_mark.x + w->slide_bar.arrow_width)) )
	DrawMarker(w, &w->slide_bar.next_mark, FALSE);

    if(   (marker != &w->slide_bar.current_mark)
       && ((x + width) > w->slide_bar.current_mark.x)
       && (x < (w->slide_bar.current_mark.x + w->slide_bar.arrow_width)) )
	DrawMarker(w, &w->slide_bar.current_mark, FALSE);
}


/*  Subroutine:	PaintMarker
 *  Purpose:	Draw a mark at given position by copying a square pixmap
 */
static void PaintMarker( XmSlideBarWidget w, Marker* marker, Boolean in )
{
    static GC drawGC = NULL;

    if (!w->core.visible) return;

    /* If the marker is the current marker, but we are not displaying the 
     * current marker, return.
     */
    if ( (marker == &w->slide_bar.current_mark) && 
	(!w->slide_bar.current_visible) )
	{
	return;
	}
    if (marker->arrow == NULL)
	{
	return;
	}
    if( drawGC == NULL )
    {
	XGCValues	values;
	XtGCMask	valueMask;
	valueMask = GCFunction;
	values.function = GXcopy;
	drawGC = XtGetGC((Widget)w, valueMask, &values);
    }
    if( in )
	{
	XCopyArea(XtDisplay(w), marker->in_pixmap, w->core.window,
		  drawGC, 0, 0, marker->width, marker->height,
		  marker->x + w->slide_bar.x_margin - marker->x_edge_offset,
		  w->slide_bar.y_margin + marker->y);
	}
    else
	{
	XCopyArea(XtDisplay(w), marker->out_pixmap, w->core.window,
		  drawGC, 0, 0, marker->width, marker->height,
		  marker->x + w->slide_bar.x_margin - marker->x_edge_offset,
		  w->slide_bar.y_margin + marker->y);
	}
    XFlush(XtDisplay(w));
}


/*  Subroutine:	DrawMarker
 *  Purpose:	Draw a mark at given position by passing the regions to color
 */
static void DrawMarker( XmSlideBarWidget w, Marker* marker, Boolean in )
{
    short temp_count;

    if (!w->core.visible) return;
    /* If the marker is the current marker, but we are not displaying the 
     * current marker, return.
     */
    if ( (marker == &w->slide_bar.current_mark) && 
	(!w->slide_bar.current_visible) )
	{
	return;
	}
    if (marker->arrow == NULL)
	{
	return;
	}
    /*  Reposition arrow coords if needed  */
    if( marker->x != marker->arrow->x )
    {
	register short i;
	register Arrow arrow = marker->arrow;
	register short delta = marker->x - arrow->x;
	for( i=0; i<arrow->top_count; i++ )
	    arrow->top[i].x += delta;
	for( i=0; i<arrow->bot_count; i++ )
	    arrow->bot[i].x += delta;
	for( i=0; i<arrow->cen_count; i++ )
	    arrow->cen[i].x += delta;
	arrow->x = marker->x;
    }
    if( in )
    {
	XFillRectangles(XtDisplay(w), w->core.window,
			w->manager.bottom_shadow_GC,
			marker->arrow->top, marker->arrow->top_count);
	XFillRectangles(XtDisplay(w), w->core.window, w->manager.top_shadow_GC,
			marker->arrow->bot, marker->arrow->bot_count);
    }
    else
    {
	XFillRectangles(XtDisplay(w), w->core.window, w->manager.top_shadow_GC,
			marker->arrow->top, marker->arrow->top_count);
	XFillRectangles(XtDisplay(w), w->core.window,
			w->manager.bottom_shadow_GC,
			marker->arrow->bot, marker->arrow->bot_count);
    }
    temp_count = 0;
    /*  If next marker is over a limit marker, draw only half of center  */
    if(marker == &w->slide_bar.next_mark)
	{
	if ( (w->slide_bar.next_mark.x == w->slide_bar.start_mark.x)
	   || (w->slide_bar.next_mark.x == w->slide_bar.stop_mark.x) )
    	    {
	    temp_count = marker->arrow->cen_count;
	    marker->arrow->cen_count = temp_count / 2;
            }
	}
#ifdef Comment
    else if (marker == &w->slide_bar.current_mark)
	{
	if ( (w->slide_bar.current_mark.x == w->slide_bar.start_mark.x)
	   || (w->slide_bar.current_mark.x == w->slide_bar.stop_mark.x) )
    	    {
	    temp_count = marker->arrow->cen_count;
	    marker->arrow->cen_count = temp_count / 2;
            }
	}
#endif

    XFillRectangles(XtDisplay(w), w->core.window, marker->cenGC,
		    marker->arrow->cen, marker->arrow->cen_count);
    if( temp_count )
	marker->arrow->cen_count = temp_count;
    XFlush(XtDisplay(w));
}


/*  Subroutine:	InitMarkers
 *  Purpose:	Set up pixmaps and some parameters used to render the markers
 */
static void InitMarkers( XmSlideBarWidget w )
{
    Arrow	arrow;
    XGCValues	values;
    XtGCMask	valueMask;
    GC	cenGC;
    
    arrow = (Arrow)XtCalloc(1, sizeof(struct ArrowRec));
    /*  2 represents shadow thickness and initial offset */
    GetArrowDrawRects(2, w->slide_bar.arrow_width, w->slide_bar.arrow_height,
		      &arrow->top_count, &arrow->cen_count, &arrow->bot_count,
		      &arrow->top, &arrow->cen, &arrow->bot);
    /*  Set reference values for establishing window location  *
     *  n + base - top[0] puts arrow at n of window	       */
    arrow->base_x = arrow->top[0].x - (w->slide_bar.arrow_width / 2);
    arrow->base_y = arrow->top[0].y;
    valueMask = GCForeground | GCBackground | GCFillStyle;
    values.foreground = w->core.background_pixel;
    values.background = w->core.background_pixel;
    values.fill_style = FillSolid;
    cenGC = XtGetGC((Widget)w, valueMask, &values);

    w->slide_bar.next_mark.out_pixmap =
      CreateArrowPixmap(w, cenGC, w->slide_bar.arrow_width,
			w->slide_bar.arrow_height);
    w->slide_bar.next_mark.in_pixmap =
      CreateArrowPixmap(w, cenGC, w->slide_bar.arrow_width,
			w->slide_bar.arrow_height);

    w->slide_bar.current_mark.out_pixmap =
      CreateArrowPixmap(w, cenGC, w->slide_bar.arrow_width,
			w->slide_bar.arrow_height);
    w->slide_bar.current_mark.in_pixmap =
      CreateArrowPixmap(w, cenGC, w->slide_bar.arrow_width,
			w->slide_bar.arrow_height);

    w->slide_bar.start_mark.out_pixmap =
      CreateArrowPixmap(w, cenGC, w->slide_bar.arrow_width,
			w->slide_bar.arrow_height);
    w->slide_bar.start_mark.in_pixmap =
      CreateArrowPixmap(w, cenGC, w->slide_bar.arrow_width,
			w->slide_bar.arrow_height);
    XtReleaseGC((Widget)w, cenGC);

    values.foreground = w->slide_bar.limit_mark_color;
    cenGC = XtGetGC((Widget)w, valueMask, &values);
    SetMarkerParams(&w->slide_bar.start_mark, cenGC, arrow,
		    0, w->slide_bar.arrow_width, w->slide_bar.arrow_height);
    FillArrowPixmap(w, arrow, cenGC, FALSE,
		    w->slide_bar.start_mark.out_pixmap);
    FillArrowPixmap(w, arrow, cenGC, TRUE,
		    w->slide_bar.start_mark.in_pixmap);

    w->slide_bar.stop_mark.out_pixmap = w->slide_bar.start_mark.out_pixmap;
    w->slide_bar.stop_mark.in_pixmap = w->slide_bar.start_mark.in_pixmap;
    SetMarkerParams(&w->slide_bar.stop_mark, cenGC, arrow,
		    0, w->slide_bar.arrow_width, w->slide_bar.arrow_height);

    values.foreground = w->slide_bar.value_mark_color;
    cenGC = XtGetGC((Widget)w, valueMask, &values);
    SetMarkerParams(&w->slide_bar.next_mark, cenGC, arrow,
		    0, w->slide_bar.arrow_width, w->slide_bar.arrow_height);
    FillArrowPixmap(w, arrow, w->slide_bar.next_mark.cenGC, FALSE,
		    w->slide_bar.next_mark.out_pixmap);
    FillArrowPixmap(w, arrow, w->slide_bar.next_mark.cenGC, TRUE,
		    w->slide_bar.next_mark.in_pixmap);

    values.foreground = w->slide_bar.current_mark_color;
    cenGC = XtGetGC((Widget)w, valueMask, &values);
    SetMarkerParams(&w->slide_bar.current_mark, cenGC, arrow,
		    0, w->slide_bar.arrow_width, w->slide_bar.arrow_height);
    FillArrowPixmap(w, arrow, w->slide_bar.current_mark.cenGC, FALSE,
		    w->slide_bar.current_mark.out_pixmap);
    FillArrowPixmap(w, arrow, w->slide_bar.current_mark.cenGC, TRUE,
		    w->slide_bar.current_mark.in_pixmap);

    SetArrowCoords(arrow, w->slide_bar.x_margin + w->slide_bar.arrow_x_offset,
		   w->slide_bar.y_margin);
}


/*  Subroutine:	SetMarkerParams
 *  Purpose:	Initialize the marker parameters
 */
static void SetMarkerParams( Marker* marker, GC cenGC, Arrow arrow,
			     short y, short width, short height )
{
    marker->cenGC = cenGC;
    marker->arrow = arrow;
    marker->y = y;
    marker->x_edge_offset = width / 2;
    marker->width = width;
    marker->height = height;
}


/*  Subroutine:	SetArrowCoords
 *  Purpose:	Set arrow drawing position at 0,0 of bar
 */
static
void SetArrowCoords( register Arrow arrow, short x, short y )
{
    register short i;
    register short delta_x;
    register short delta_y;
    delta_x = x + arrow->base_x - arrow->top[0].x;
    delta_y = y + arrow->base_y - arrow->top[0].y;
    for( i=0; i<arrow->top_count; i++ )
    {
	arrow->top[i].x += delta_x;
	arrow->top[i].y += delta_y;
    }
    for( i=0; i<arrow->bot_count; i++ )
    {
	arrow->bot[i].x += delta_x;
	arrow->bot[i].y += delta_y;
    }
    for( i=0; i<arrow->cen_count; i++ )
    {
	arrow->cen[i].x += delta_x;
	arrow->cen[i].y += delta_y;
    }
    arrow->x = 0;
}


/*  Subroutine:	CreateArrowPixmap
 *  Purpose:	Create a pixmap and paint its background
 */
static Pixmap CreateArrowPixmap( XmSlideBarWidget w, GC gc,
				 short width, short height )
{
    Pixmap pixmap;

    pixmap = XCreatePixmap(XtDisplay(w), w->core.window,
			   width, height, w->core.depth);
    XFillRectangle(XtDisplay(w), pixmap, gc, 0, 0, width, height);
    return pixmap;
}


/*  Subroutine:	FillArrowPixmap
 *  Purpose:	Draw the arrow into the pixmap (2 shadow colors and a center)
 */
static void FillArrowPixmap( XmSlideBarWidget w, struct ArrowRec* arrow,
			     GC cenGC, Boolean in, Pixmap pixmap )
{
    if( in )
    {
	XFillRectangles(XtDisplay(w), pixmap, w->manager.bottom_shadow_GC,
			arrow->top, arrow->top_count);
	XFillRectangles(XtDisplay(w), pixmap, w->manager.top_shadow_GC,
		        arrow->bot, arrow->bot_count);
    }
    else
    {
	XFillRectangles(XtDisplay(w), pixmap, w->manager.top_shadow_GC,
			arrow->top, arrow->top_count);
	XFillRectangles(XtDisplay(w), pixmap, w->manager.bottom_shadow_GC,
			arrow->bot, arrow->bot_count);
    }
    XFillRectangles(XtDisplay(w), pixmap, cenGC, arrow->cen, arrow->cen_count);
    /*xi = XGetImage(XtDisplay(w), pixmap, 0, 0, 16, 16, AllPlanes, ZPixmap);*/
}


/*  Subroutine:	GetArrowDrawRects
 *  Purpose:	Create lists of rectangles for drawing a down arrow's ...
 *		shadows and center
 *  Note:	Modified from XmGetArrowDrawRects of Xm ArrowButton widget
 */
static void GetArrowDrawRects( int  shadow_thickness,
			       int  core_width,
			       int  core_height,
			       short  *top_count,
			       short  *cent_count,
			       short  *bot_count,
			       XRectangle **top,
			       XRectangle **cent,
			       XRectangle **bot )
{
    int size, width, start;
    register int y;
    XRectangle *tmp;
    short t = 0;
    short b = 0;
    short c = 0;
    int xOffset = 0;
    int yOffset = 0;

    /*  Get the size and allocate the rectangle lists  */
    if( core_width > core_height ) 
    {
	size = core_height - 2;
    }
    else
    {
	size = core_width - 2;
    }
    if (size < 1)
	return;
    xOffset = yOffset = -shadow_thickness;

    *top  = (XRectangle *) XtMalloc (sizeof (XRectangle) * (size / 2 + 6));
    *cent = (XRectangle *) XtMalloc (sizeof (XRectangle) * (size  / 2 + 6));
    *bot  = (XRectangle *) XtMalloc (sizeof (XRectangle) * (size / 2 + 6));
    /*  Set up a loop to generate the segments.  */
    width = size;
    y = size + shadow_thickness - 1 + yOffset;
    start = shadow_thickness + 1 + xOffset;
    while( width > 0 )
    {
	if( width == 1 )
	{
	    (*top)[t].x = start; (*top)[t].y = y + 1;
	    (*top)[t].width = 1; (*top)[t].height = 1;
	    t++;
	}
	else if( width == 2 )
	{
	    if( size == 2 )
	    {
            (*top)[t].x = start; (*top)[t].y = y;
            (*top)[t].width = 2; (*top)[t].height = 1;
            t++;
            (*top)[t].x = start; (*top)[t].y = y + 1;
            (*top)[t].width = 1; (*top)[t].height = 1;
            t++;
            (*bot)[b].x = start + 1; (*bot)[b].y = y + 1;
            (*bot)[b].width = 1; (*bot)[b].height = 1;
            b++;
	}
	}
      else
      {
	  if( start == (shadow_thickness + 1 + xOffset) )
	  {
	      (*top)[t].x = start; (*top)[t].y = y;
	      (*top)[t].width = 2; (*top)[t].height = 1;
	      t++;
	      (*bot)[b].x = start; (*bot)[b].y = y + 1;
	      (*bot)[b].width = 2; (*bot)[b].height = 1;
	      b++;
	      (*bot)[b].x = start + 2; (*bot)[b].y = y;
	      (*bot)[b].width = width - 2; (*bot)[b].height = 2;
	      b++;
	  }
	  else
	  {
	      (*top)[t].x = start; (*top)[t].y = y;
	      (*top)[t].width = 2; (*top)[t].height = 2;
	      t++;
	      (*bot)[b].x = start + width - 2; (*bot)[b].y = y;
	      (*bot)[b].width = 2; (*bot)[b].height = 2;
	      if (width == 3)
	      {
		  (*bot)[b].width = 1;
		  (*bot)[b].x += 1;
	      }
	      b++;
	      if (width > 4)
	      {
		  (*cent)[c].x = start + 2; (*cent)[c].y = y;
		  (*cent)[c].width = width - 4; (*cent)[c].height = 2;
		  c++;
	      }
	  }
      }
	start++;
	width -= 2;
	y -= 2;
    }
    tmp = *top;
    *top = *bot;
    *bot = tmp;
    *top_count = b;
    *cent_count = c;
    *bot_count = t;
    {
          register int w_down = core_width - 2;
          register int h_down = core_height - 2;
          register int i; 

          i = -1;
          do
          {
             i++;
             if (i < *top_count)
             {
                (*top)[i].x = w_down - (*top)[i].x - (*top)[i].width + 2;
                (*top)[i].y = h_down - (*top)[i].y - (*top)[i].height + 2;
             }
             if (i < *bot_count)
             {
                (*bot)[i].x = w_down - (*bot)[i].x - (*bot)[i].width + 2;
                (*bot)[i].y = h_down - (*bot)[i].y - (*bot)[i].height + 2;
             }
             if (i < *cent_count)
             {
                (*cent)[i].x = w_down - (*cent)[i].x - (*cent)[i].width + 2;
                (*cent)[i].y = h_down - (*cent)[i].y - (*cent)[i].height + 2;
             }
          }
          while (i < *top_count || i < *bot_count || i < *cent_count);
      }
}


/*  Subroutine:	RecomputeSlideBarPositions
 *  Purpose:	Establish correct marker positions for this length of bar
 */
static void RecomputeSlideBarPositions( XmSlideBarWidget w )
{
int min;

    min = w->slide_bar.min_detent;
    w->slide_bar.start_mark.x =
      ((w->slide_bar.start_mark.detent - min) * w->slide_bar.width)
      / (w->slide_bar.num_detents);
    w->slide_bar.next_mark.x =
      ((w->slide_bar.next_mark.detent - min) * w->slide_bar.width)
      / (w->slide_bar.num_detents);
    w->slide_bar.current_mark.x =
      ((w->slide_bar.current_mark.detent - min) * w->slide_bar.width)
      / (w->slide_bar.num_detents);
    w->slide_bar.stop_mark.x =
      ((w->slide_bar.stop_mark.detent - min) * w->slide_bar.width)
      / (w->slide_bar.num_detents);
}


/*  Subroutine:	ChangeSlideBarStart
 *  Purpose:	Convenience routine to reposition the start marker
 */
static void ChangeSlideBarStart( XmSlideBarWidget w, short detent )
{
    short x;

    if(   (w->slide_bar.grabbed != &w->slide_bar.next_mark)
       && (detent <= w->slide_bar.stop_mark.detent) )
    {
	w->slide_bar.start_mark.detent = detent;
	x = ((detent - w->slide_bar.min_detent) * w->slide_bar.width) / 
				w->slide_bar.num_detents;
	/*  Check for adequate initialization  */
	if( XtIsRealized((Widget)w) && w->slide_bar.start_mark.arrow )
	{
	    ClearMarker(w, &w->slide_bar.start_mark,
			w->slide_bar.start_mark.x,
			w->slide_bar.start_mark.width);
	    w->slide_bar.start_mark.x = x;
	    DrawMarker(w, &w->slide_bar.start_mark, FALSE);
	}
	else
	    w->slide_bar.start_mark.x = x;
    }
    if (w->slide_bar.start_mark.detent > w->slide_bar.next_mark.detent)
	{
	ChangeSlideBarNext( w, detent );
	}
    if (w->slide_bar.start_mark.detent > w->slide_bar.stop_mark.detent)
	{
	ChangeSlideBarStop( w, detent );
	}
}


/*  Subroutine:	ChangeSlideBarStop
 *  Purpose:	Convenience routine to reposition the stop marker
 */
static void ChangeSlideBarStop( XmSlideBarWidget w, short detent )
{
    short x;

    if(   (w->slide_bar.grabbed != &w->slide_bar.next_mark)
       && (detent >= w->slide_bar.start_mark.detent) )
    {
	w->slide_bar.stop_mark.detent = detent;
	x = ((detent - w->slide_bar.min_detent) * w->slide_bar.width) / 
				w->slide_bar.num_detents;
	/*  Check for adequate initialization  */
	if( XtIsRealized((Widget)w) && w->slide_bar.stop_mark.arrow )
	{
	    ClearMarker(w, &w->slide_bar.stop_mark,
			w->slide_bar.stop_mark.x, w->slide_bar.stop_mark.width);
	    w->slide_bar.stop_mark.x = x;
	    DrawMarker(w, &w->slide_bar.stop_mark, FALSE);
	}
	else
	    w->slide_bar.stop_mark.x = x;
    }
    if (w->slide_bar.stop_mark.detent < w->slide_bar.next_mark.detent)
	{
	ChangeSlideBarNext( w, detent );
	}
    if (w->slide_bar.stop_mark.detent < w->slide_bar.start_mark.detent)
	{
	ChangeSlideBarStart( w, detent );
	}
}


/*  Subroutine:	ChangeSlideBarNext
 *  Purpose:	Convenience routine to reposition the next marker
 */
static void ChangeSlideBarNext( XmSlideBarWidget w, short detent )
{
    short x;

    if(   (w->slide_bar.grabbed != &w->slide_bar.next_mark)
       && (detent >= w->slide_bar.start_mark.detent)
       && (detent <= w->slide_bar.stop_mark.detent) )
    {
	w->slide_bar.next_mark.detent = detent;
	x = ((detent - w->slide_bar.min_detent) * w->slide_bar.width) / 
			w->slide_bar.num_detents;
	/*  Check for adequate initialization  */
	if( XtIsRealized((Widget)w) && w->slide_bar.next_mark.arrow )
	{
	    ClearMarker(w, &w->slide_bar.next_mark,
			w->slide_bar.next_mark.x,
			w->slide_bar.next_mark.width);
	    w->slide_bar.next_mark.x = x;
	    DrawMarker(w, &w->slide_bar.next_mark, FALSE);
	}
	else
	    w->slide_bar.next_mark.x = x;
    }
    if (w->slide_bar.next_mark.detent < w->slide_bar.start_mark.detent)
	{
	ChangeSlideBarStart( w, detent );
	}
    if (w->slide_bar.next_mark.detent > w->slide_bar.stop_mark.detent)
	{
	ChangeSlideBarStop( w, detent );
	}
}

/*  Subroutine:	ChangeSlideBarCurrent
 *  Purpose:	Convenience routine to reposition the next marker
 */
static void ChangeSlideBarCurrent( XmSlideBarWidget w, short detent )
{
    short x;

    if(   (w->slide_bar.grabbed != &w->slide_bar.current_mark)
       && (detent >= w->slide_bar.start_mark.detent)
       && (detent <= w->slide_bar.stop_mark.detent) )
    {
	w->slide_bar.current_mark.detent = detent;
	x = ((detent - w->slide_bar.min_detent) * w->slide_bar.width) / 
			w->slide_bar.num_detents;
	/*  Check for adequate initialization  */
	if( XtIsRealized((Widget)w) && w->slide_bar.current_mark.arrow )
	{
	    ClearMarker(w, &w->slide_bar.current_mark,
			w->slide_bar.current_mark.x,
			w->slide_bar.current_mark.width);
	    w->slide_bar.current_mark.x = x;
	    DrawMarker(w, &w->slide_bar.current_mark, FALSE);
	}
	else
	    w->slide_bar.current_mark.x = x;
    }
    if (w->slide_bar.current_mark.detent > w->slide_bar.stop_mark.detent)
	{
	XtWarning("Current value must be less than or equal to stop value.");
	}
    if (w->slide_bar.next_mark.detent > w->slide_bar.stop_mark.detent)
	{
	XtWarning
	    ("Current value must be greater than or equal to start value.");
	}
}

/*  Subroutine:	XmCreateSlideBar
 *  Purpose:	This function creates and returns a SlideBar widget.
 */
Widget XmCreateSlideBar( Widget parent, String name,
			 ArgList args, Cardinal num_args )
#ifdef EXPLAIN
     Widget	parent;		/* Parent widget		 */
     String	name;		/* Name for this widget instance */
     ArgList	args;		/* Widget specification arg list */
     Cardinal	num_args;	/* SlideBar of args in the ArgList */
#endif
{
    return XtCreateWidget(name, xmSlideBarWidgetClass, parent, args, num_args);
}
