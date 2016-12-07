/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


/*	Widget that will display that final ADX images.  The widget will
 *	detect the depth of the display and, if depth == 24, it will assume
 *	that we have a HIPPI frame buffer.  There is a standard colormap 
 *	available as a property from the server.  We will obtain and use the
 *	standard colormap.
 *
 *	Resources:
 *		XmNmotionCallback - Callback to get ButtonMotion events.
 *		XmNsendMotion - Boolean that controls whether a detected
 *				ButtonMotion event should be reported via
 *				a registered callback. (On/Off switch)
 *		 
 */

/*
 */

#define FORCE_VISUAL


#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Xm/DrawingA.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#if (XmVersion >= 1002)
#include <Xm/GadgetP.h>
#endif

extern void _XmDispatchGadgetInput(Widget g, XEvent *event, Mask mask);

#include "../widgets/Image.h"
#include "../widgets/ImageP.h"
#include "../widgets/clipnotify.h"

#define superclass (&widgetClassRec)
static  struct visualLookupS  { 
  int depth; 
  int class; 
#ifdef FORCE_VISUAL
}  *acceptableVisuals, _acceptableVisuals[] = {
#else
}  acceptableVisuals[] = {
#endif
  {  4, PseudoColor },
  {  4, StaticColor },
  {  4, GrayScale   },
  {  4, StaticGray  },
  {  8, PseudoColor },
  {  8, StaticColor },
  {  8, TrueColor },
  {  8, DirectColor },
  {  8, GrayScale   },
  {  8, StaticGray  },
/*{ 12, PseudoColor },*/
  { 12, TrueColor   },
  { 12, DirectColor },
  { 15, TrueColor   },
  { 16, TrueColor   },
  { 24, TrueColor   },
  { 24, DirectColor },
  { 32, TrueColor   },
  { 32, DirectColor },
};

#ifdef FORCE_VISUAL
static struct classes
{
  int   class;
  char  *name;
} Classes[] = {
  { PseudoColor, "PseudoColor" },
  { StaticColor, "StaticColor" },
  { GrayScale,   "GrayScale"   },
  { StaticGray,  "StaticGray"  },
  { TrueColor,   "TrueColor"   },
  { DirectColor, "DirectColor" },
  { -1, NULL}
};
#endif

#define ISACCEPTABLE(ret,dpth,clss) \
  ret = 0; \
  for(i = 0; i < nAcceptable; i++) \
  {	\
    if(acceptableVisuals[i].depth == (dpth) && \
       acceptableVisuals[i].class == (clss)) { \
	ret = 1; \
	break; \
      } \
  }

static void Initialize( XmImageWidget request, XmImageWidget new );
static void Realize( register XmImageWidget w, Mask *p_valueMask,
		     XSetWindowAttributes *attributes );

static void Arm();
static void Activate();
static void BtnMotion();
static Visual *getBestVisual (Display *dpy, int *depth, int screen);


/* Default translation table and action list */
static char defaultTranslations[] =
    "<Btn1Down>:     Arm()\n\
     <Btn1Up>:       Activate()\n\
     <BtnMotion>:    BtnMotion()\n\
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
      XmNmotionCallback, XmCCallback, XmRCallback, sizeof (XtCallbackList),
      XtOffset (XmImageWidget, image.motion_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {
      XmNsendMotion, XmCSendMotion, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.send_motion_events),
      XmRImmediate, (XtPointer) False
    },
    {
      XmNframeBuffer, XmCFrameBuffer, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.frame_buffer),
      XmRImmediate, (XtPointer) False
    },
    {
      XmN8supported, XmCSupported, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.supported8),
      XmRImmediate, (XtPointer) False
    },
    {
      XmN12supported, XmCSupported, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.supported12),
      XmRImmediate, (XtPointer) False
    },
    {
      XmN15supported, XmCSupported, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.supported15),
      XmRImmediate, (XtPointer) False
    },
    {
      XmN16supported, XmCSupported, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.supported16),
      XmRImmediate, (XtPointer) False
    },
    {
      XmN24supported, XmCSupported, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.supported24),
      XmRImmediate, (XtPointer) False
    },
    {
      XmN32supported, XmCSupported, XmRBoolean, sizeof(Boolean),
      XtOffset(XmImageWidget, image.supported32),
      XmRImmediate, (XtPointer) False
    },
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XmImageClassRec xmImageClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmDrawingAreaClassRec,	/* superclass         */
      "XmImage",				/* class_name         */
      sizeof(XmImageRec),			/* widget_size        */
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
      TRUE,					/* compress_motion    */
      TRUE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* VISIBle_interest   */
      NULL,					/* destroy            */
      (XtWidgetProc)_XtInherit,			/* resize             */
      (XtExposeProc)_XtInherit,			/* expose             */
      NULL,					/* set_values         */
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
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      sizeof(XmManagerConstraintRec),		/* constraint size      */   
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

   {		/* image class - none */
      NULL					/* mumble */
   }
};


WidgetClass xmImageWidgetClass = (WidgetClass) &xmImageClassRec;


/*  Subroutine:	Initialize
 *  Effect:	Create and initialize the component widgets
 */
static void Initialize( XmImageWidget request, XmImageWidget new )
{
int    depth;
int    screen = XScreenNumberOfScreen(XtScreen(request));

    depth = 8;
    /*vis =*/ getBestVisual(XtDisplay(new), &depth, screen);
    if(depth == 8) new->image.supported8 = True;

    depth = 12;
    /*vis =*/ getBestVisual(XtDisplay(new), &depth, screen);
    if(depth == 12) new->image.supported12 = True;

    depth = 15;
    /*vis =*/ getBestVisual(XtDisplay(new), &depth, screen);
    if(depth == 15) new->image.supported15 = True;

    depth = 16;
    /*vis =*/ getBestVisual(XtDisplay(new), &depth, screen);
    if(depth == 16) new->image.supported16 = True;

    depth = 24;
    /*vis =*/ getBestVisual(XtDisplay(new), &depth, screen);
    if(depth == 24) new->image.supported24 = True;

    depth = 32;
    /*vis =*/ getBestVisual(XtDisplay(new), &depth, screen);
    if(depth == 32) new->image.supported32 = True;
}


#if 0
/*  Subroutine:	SetValues
 *  Effect:	Handles requests to change things from the application
 */
static Boolean SetValues( XmImageWidget current,
			  XmImageWidget request,
			  XmImageWidget new );

static Boolean SetValues( XmImageWidget current,
			  XmImageWidget request,
			  XmImageWidget new )
{
    Boolean redraw = FALSE;

    return redraw;
}
#endif

/*  Subroutine:	Realize
 *  Purpose:	Create the window with PointerMotion events and None gravity
 */
static void Realize( register XmImageWidget w, Mask *p_valueMask,
		     XSetWindowAttributes *attributes )
{
Mask valueMask = 	*p_valueMask;
Window 			root;
XStandardColormap 	cmap_info;
XVisualInfo 		visual_info;    /* temp storage for a visual data */
unsigned int 		black, white;
int			major_code;
int			event_code;
int			error_code;
Visual			*vis;
int			depth;
Display 		*dpy = XtDisplay(w);
int			screen = XScreenNumberOfScreen(XtScreen(w));

    w->image.frame_buffer = XQueryExtension
				    (dpy,
				    CLIPNOTIFY_PROTOCOL_NAME,
				    &major_code,
				    &event_code,
				    &error_code);

    black = BlackPixel(dpy, screen); 
    white = WhitePixel(dpy, screen); 
    depth = w->core.depth;
    vis = getBestVisual(dpy, &depth, screen);
    root = XRootWindowOfScreen(XtScreen(w));
    if (w->image.frame_buffer)
	{
	XGetStandardColormap(dpy, root, &cmap_info, XA_RGB_BEST_MAP);
	valueMask |= CWBitGravity | CWDontPropagate | 
			CWColormap | CWBorderPixel | CWBackPixmap;
	attributes->background_pixmap =  None;
	attributes->colormap = cmap_info.colormap;
	attributes->border_pixel = white;
	attributes->bit_gravity = NorthWestGravity;
	attributes->do_not_propagate_mask = 
	      ButtonPressMask | ButtonReleaseMask |
	      KeyPressMask | KeyReleaseMask | PointerMotionMask;
	w->core.depth = 24;
	XMatchVisualInfo(dpy, screen, 24, TrueColor, &visual_info);
	XtCreateWindow((Widget)w,InputOutput, visual_info.visual,valueMask, attributes);
	XChangeWindowAttributes(dpy, XtWindow(w), CWBackPixmap,
		attributes);
	}
    else
	{
	valueMask |= CWBitGravity | CWDontPropagate | CWBackPixel | CWColormap;
	attributes->bit_gravity = NorthWestGravity;
	attributes->background_pixel = black;
	attributes->do_not_propagate_mask = 
	      ButtonPressMask | ButtonReleaseMask |
	      KeyPressMask | KeyReleaseMask | PointerMotionMask;
	if (vis != XDefaultVisual(dpy, screen))
	{
	    attributes->colormap = 
		XCreateColormap(dpy, root, vis, AllocNone);
	}
	else
	{
	    attributes->colormap = 
		DefaultColormap(dpy, screen);
	}
	XtCreateWindow((Widget)w, InputOutput, vis, valueMask, attributes);
	}
    XSync(dpy,0);
}


/*  Subroutine:	XmCreateImage
 *  Purpose:	This function creates and returns a Image widget.
 */
Widget XmCreateImage( Widget parent, String name,
			 ArgList args, Cardinal num_args )
{
    return XtCreateWidget(name, xmImageWidgetClass, parent, args, num_args);
}

/*  Subroutine:	XmImageSetBackgroundPixel
 *  Purpose:	This function  sets the windows background pixel attribute
 */
void XmImageSetBackgroundPixel( Widget image, Pixel bg)
{
XSetWindowAttributes attributes;
 
    attributes.background_pixel = bg;
    XChangeWindowAttributes(XtDisplay(image), XtWindow(image), CWBackPixel, 
				&attributes);
}

/*  Subroutine:	XmImageSetBackgroundPixmap
 *  Purpose:	This function  sets the windows background pixel attribute
 */
void XmImageSetBackgroundPixmap( Widget image, Pixmap pixmap)
{
XSetWindowAttributes attributes;

    attributes.background_pixmap = pixmap;
    XChangeWindowAttributes(XtDisplay(image), XtWindow(image), CWBackPixmap, 
				&attributes);
}

/************************************************************************
 *
 *  Arm
 *      This function processes arming actions occuring on the
 *      drawing area that may need to be sent to a gadget.
 *
 ************************************************************************/

static void Arm (da, event)
XmDrawingAreaWidget da;
XButtonPressedEvent * event;

{
   XmGadget gadget;

   if ((gadget = (XmGadget) _XmInputInGadget((Widget)da, event->x, event->y)) != NULL)
   {
      _XmDispatchGadgetInput ((Widget)gadget, (XEvent *)event, XmARM_EVENT);
      da->manager.selected_gadget = gadget;
   }
}
/************************************************************************
 *
 *  Activate
 *      This function processes activation and disarm actions occuring
 *      on the drawing that may need to be sent to a gadget.
 *
 ************************************************************************/

static void Activate (da, event)
XmDrawingAreaWidget da;
XButtonPressedEvent * event;

{
   if (da->manager.selected_gadget != NULL)
   {
      _XmDispatchGadgetInput ((Widget)da->manager.selected_gadget,
            (XEvent *)event, XmACTIVATE_EVENT);
      da->manager.selected_gadget = NULL;
   }
}
/************************************************************************
 *
 *  BtnMotion
 *      This function processes motion events occuring on the
 *      drawing area that may need to be sent to a gadget.
 *
 ************************************************************************/

static void BtnMotion (iw, event)
XmImageWidget iw;
XEvent *event;

{
   XmDrawingAreaCallbackStruct cb;

   cb.reason = XmCR_DRAG;
   cb.event = event;
   cb.window = XtWindow (iw);

   if (iw->image.send_motion_events)
      {
      XtCallCallbacks ((Widget)iw, XmNmotionCallback, &cb);
      }
}

static Visual *
getBestVisual (Display *dpy, int *depth, int screen)
{
  int		count,i,j;
  XVisualInfo	*visualInfo=NULL,template;
  Visual	*visual,*ret=NULL;
  unsigned char	acceptable;
  int		nAcceptable;
  int 		vismask;

#ifdef FORCE_VISUAL
    {
	char *str;
	int  depth;

	str = (char *)getenv("DXVISUAL");
	if (str)
	{
	    acceptableVisuals = (struct visualLookupS *)
			XtMalloc(sizeof(struct visualLookupS));

	    if (1 != sscanf(str, "%d", &depth))
		return NULL;

	    for (i = 0; str[i] && str[i] != ','; i++);

	    if (! str[i])
		return NULL;

	    for (j = 0; Classes[j].class != -1; j++)
		if (! strcmp(Classes[j].name, str+i+1))
		    break;

	    if (Classes[j].class == -1)
		return NULL;

	    acceptableVisuals[0].depth = depth;
	    acceptableVisuals[0].class = Classes[j].class;

	    nAcceptable = 1;
	}
	else
	{
	    acceptableVisuals = &_acceptableVisuals[0];
	    nAcceptable = sizeof(_acceptableVisuals)
				/ sizeof(struct visualLookupS);
	}
    }
#endif

   /*
    * If a depth was requested
    *	see if the default depth matches the requested one,
    * 	then try any visual of that depth,
    */
    if(*depth) 
    {
	if(*depth == XDefaultDepth(dpy, screen))
	{
	    visual =  XDefaultVisual(dpy, screen);
	    ISACCEPTABLE(acceptable,*depth,visual->class);
	    if(acceptable)
		return visual;
	}
	template.depth = *depth;
	template.screen = screen;

	vismask = VisualDepthMask|VisualScreenMask;
	visualInfo = XGetVisualInfo(dpy,vismask,&template,&count);

	for(i = 0; i < nAcceptable; i++)
	{
	    for(j=0;j<count && !ret;j++)
	    {
		if(acceptableVisuals[i].depth == visualInfo[j].depth &&
		   acceptableVisuals[i].class == visualInfo[j].class)
		{
		    ret = visualInfo[j].visual;
		}
	    }
	}

	if(visualInfo)
	    XFree ((void *)visualInfo);
	visualInfo = NULL;
	if (ret)
	    return ret;
    }

      
   /* If no depth given, or depth was invalid,
    *	then try the default,
    *	then 8 bit,
    *	then any acceptable visual
    */
    *depth = XDefaultDepth(dpy, screen);
    visual =  XDefaultVisual(dpy, screen);

    ISACCEPTABLE(acceptable,*depth,visual->class);
    if(acceptable)
	return visual;

    template.depth = *depth = 8;
    template.screen = screen;
    vismask = VisualDepthMask|VisualScreenMask;
    visualInfo = XGetVisualInfo(dpy,vismask,&template,&count);
    for(i = 0; i < nAcceptable; i++)
    {
	for(j=0;j<count && !ret;j++)
	{
	    if(acceptableVisuals[i].depth == visualInfo[j].depth &&
	       acceptableVisuals[i].class == visualInfo[j].class)
	    {
		ret = visualInfo[j].visual;
	    }
	}
    }

    if(visualInfo)
	XFree ((void *)visualInfo);
    visualInfo = NULL;
    if (ret)
	return ret;

    template.screen = screen;
    vismask = VisualScreenMask;
    visualInfo = XGetVisualInfo(dpy,vismask,&template,&count);
    *depth =  visualInfo->depth; 
    for(i = 0; i < nAcceptable; i++)
    {
	for(j=0;j<count && !ret;j++)
	{
	    if(acceptableVisuals[i].depth == visualInfo[j].depth &&
	       acceptableVisuals[i].class == visualInfo[j].class)
	    {
		ret = visualInfo[j].visual;
	    }
	}
    }
    if(visualInfo)
	XFree ((void *)visualInfo);
    visualInfo = NULL;
    if (ret)
	return ret;

  return NULL;
}
