/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <stdio.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include <dx/UserInteractors.h>

#define Array void *
#include "superwin.h"

extern void  *DXAllocateZero(int); /* from libdx/memory.c */
extern void  DXFree(void *); /* from libdx/memory.c */
extern int   DXRegisterWindowHandlerWithCheckProcWithCheckProc(int (*proc)(int, void *), /* from libdx/xwindow.c */
			int (*check)(int, void *), Display *, void *);

extern int DXRegisterWindowHandlerWithCheckProc();  /* from libdx/xwindow.c */
extern int DXReadyToRun(); /* from libdx/outglue.c */


struct _iwx
{
    Window		parent;
    XtAppContext	appContext;
    Display		*display;
    Window 		windowId;
    Visual 		*visual;
    int 		win_x, win_y;
    Atom 		XA_WM_PROTOCOLS;
    Atom 		XA_WM_DELETE_WINDOW;

};


static int _dxf_getBestVisual(ImageWindow *);
static int _dxf_processEvents(int, void *);

static int
XChecker(int wid, void *d)
{
    ImageWindow *iw = (ImageWindow *)d;

    return XPending(iw->iwx->display);
}

int
_dxf_samplePointer(ImageWindow *iw, int flag)
{
    Window rt, chld;
    int root_x, root_y, win_x, win_y;

    if (flag)
    {
	if (XQueryPointer(iw->iwx->display, iw->iwx->windowId,
		&rt, &chld, &root_x, &root_y, &win_x, &win_y, 
		(unsigned int *) &iw->kstate))
	{
	    iw->x = win_x;
	    iw->y = win_y;
	}
	else
	{
	    iw->x = root_x - iw->iwx->win_x;
	    iw->y = root_y - iw->iwx->win_y;
	}
    }

    iw->time = CurrentTime;

    if (iw->kstate & Button1Mask)
    {
	if (iw->state[0] == BUTTON_DOWN)
	    if (! saveEvent(iw, DXEVENT_LEFT, BUTTON_MOTION))
		return 0;
    }
    else
	iw->state[0] = BUTTON_UP;


    if (iw->kstate & Button2Mask)
    {
	if (iw->state[1] == BUTTON_DOWN)
	    if (! saveEvent(iw, DXEVENT_MIDDLE, BUTTON_MOTION))
		return 0;
    }
    else
	iw->state[1] = BUTTON_UP;

    if (iw->kstate & Button3Mask)
    {
	if (iw->state[2] == BUTTON_DOWN)
	    if (! saveEvent(iw, DXEVENT_RIGHT, BUTTON_MOTION))
		return 0;
    }
    else
	iw->state[2] = BUTTON_UP;
	
    return 1;
}

static int
_dxf_processEvents(int fd, void *d)
{
    ImageWindow *iw = (ImageWindow *)d;
    XEvent ev;
    int run = 0;
    Window rt, chld;
    int root_x, root_y, win_x, win_y, flags;
    int eventType=0;
    char buf[32];
    Window wid;

    while (XPending(iw->iwx->display))
    {
	XNextEvent(iw->iwx->display, &ev);

	wid = ev.xany.window;

	switch (ev.type)
	{
	    case DestroyNotify:
		iw->iwx->parent = 0;
		return 1;

	    case VisibilityNotify:
		if (wid == iw->iwx->parent)
		{
		    XVisibilityEvent *vev = (XVisibilityEvent *)&ev;
		    if (iw->map == WINDOW_OPEN_RAISED && vev->state != VisibilityUnobscured)
			XMapRaised(iw->iwx->display, iw->iwx->parent);
		}
		break;

	    case KeyPress:
		XLookupString((XKeyEvent *)&ev, buf, 32, NULL, NULL);
		if (buf[0])
		{
		    iw->x       = ev.xkey.x;
		    iw->y       = ev.xkey.y;
		    iw->time    = ev.xkey.time;
		    iw->kstate  = ev.xkey.state;
		    saveEvent(iw, DXEVENT_KEYPRESS, (int)buf[0]);
		    run = 1;
		}
		break;

	    case ConfigureNotify:
		if (iw->size[0] != ev.xconfigure.width || iw->size[1] != ev.xconfigure.height)
		{
		    XResizeWindow(iw->iwx->display, iw->iwx->windowId, 
			    ev.xconfigure.width,
			    ev.xconfigure.height);
		    iw->size[0] = ev.xconfigure.width;
		    iw->size[1] = ev.xconfigure.height;
		    run = 1;
		}
		iw->iwx->win_x = ev.xconfigure.x;
		iw->iwx->win_y = ev.xconfigure.y;
		break;
	    
	    case ButtonPress:
		switch (ev.xbutton.button)
		{
		    case 1: eventType = DXEVENT_LEFT;
			    iw->state[0] = BUTTON_DOWN;
			    break;
		    case 2: eventType = DXEVENT_MIDDLE;
			    iw->state[1] = BUTTON_DOWN;
			    break;
		    case 3: eventType = DXEVENT_RIGHT;
			    iw->state[2] = BUTTON_DOWN;
			    break;
		}

		iw->x      = ev.xbutton.x;
		iw->y      = ev.xbutton.y;
		iw->time   = ev.xbutton.time;
		iw->kstate = ev.xbutton.state;
		saveEvent(iw, eventType, BUTTON_DOWN);
		run = 1;
		break;
	    
	    case ButtonRelease:
		switch (ev.xbutton.button)
		{
		    case 1: eventType = DXEVENT_LEFT;
			    iw->state[0] = BUTTON_UP;
			    break;
		    case 2: eventType = DXEVENT_MIDDLE;
			    iw->state[1] = BUTTON_UP;
			    break;
		    case 3: eventType = DXEVENT_RIGHT;
			    iw->state[2] = BUTTON_UP;
			    break;
		}

		iw->x      = ev.xbutton.x;
		iw->y      = ev.xbutton.y;
		iw->time   = ev.xbutton.time;
		iw->kstate = ev.xbutton.state;
		saveEvent(iw, eventType, BUTTON_UP);
		if (iw->pickFlag == 0)
		    run = 1;
		break;
	    
	    case MotionNotify:
		if (XQueryPointer(iw->iwx->display, iw->iwx->windowId,
			&rt, &chld, &root_x, &root_y, &win_x, &win_y, 
			(unsigned int *) &flags))
		{
		    iw->x = win_x;
		    iw->y = win_y;
		}
		else
		{
		    iw->x = root_x - iw->iwx->win_x;
		    iw->y = root_y - iw->iwx->win_y;
		}
		iw->time = CurrentTime;
		iw->kstate = flags;
		if (iw->pickFlag == 0)
		    run = 1;
		break;

	    case ClientMessage:
		if (ev.xclient.message_type == iw->iwx->XA_WM_PROTOCOLS)
		    XUnmapWindow(iw->iwx->display, iw->iwx->parent);
		break;
	    
	    case Expose:
		run = 1;
		break;
	    
	    default:
		break;
	}
    }
    
/* out */
    if (run)
	DXReadyToRun(iw->mod_id);

    return 1;
}

void
_dxf_deleteWindowX(ImageWindow *iw)
{
    XEvent ev;

    XSynchronize(iw->iwx->display, 1);
    DXRegisterWindowHandlerWithCheckProc(NULL, NULL,iw->iwx->display, NULL);
    if (iw->iwx->parent &&
	!XCheckTypedWindowEvent(iw->iwx->display, iw->iwx->parent, DestroyNotify, &ev))
	XDestroyWindow(iw->iwx->display, iw->iwx->parent);
    XCloseDisplay(iw->iwx->display);
}
    
int
_dxf_mapWindowX(ImageWindow *iw, int m)
{
    if (m == WINDOW_OPEN)
	XMapWindow(iw->iwx->display, iw->iwx->parent);
    else if (m == WINDOW_OPEN_RAISED)
	XMapRaised(iw->iwx->display, iw->iwx->parent);
    else
	XUnmapWindow(iw->iwx->display, iw->iwx->parent);

    iw->map = m;

    return 1;
}

int
_dxf_getParentSize(char *displayString, int wid, int *width, int *height)
{
    XWindowAttributes xwa;
    Display *dpy = XOpenDisplay(displayString);
    if (! dpy) 
	return 0;
    
    XGetWindowAttributes(dpy, (Window)wid, &xwa);

    if (width)
	*width = xwa.width;

    if (height)
	*height = xwa.height;

    XCloseDisplay(dpy);
    return 1;
}

void
_dxf_setWindowSize(ImageWindow *iw, int *size)
{
    if (!iw || !iw->iwx || !iw->iwx->display)
	return;
    if (size[0] != iw->size[0] || size[1] != iw->size[1])
    {
	XResizeWindow(iw->iwx->display, iw->iwx->parent, size[0], size[1]);
	XResizeWindow(iw->iwx->display, iw->iwx->windowId, size[0], size[1]);
	iw->size[0] = size[0];
	iw->size[1] = size[1];
    }
}

void
_dxf_setWindowOffset(ImageWindow *iw, int *offset)
{
    if (!iw || !iw->iwx || !iw->iwx->display)
	return;
    XMoveWindow(iw->iwx->display, iw->iwx->parent, offset[0], offset[1]);
    iw->offset[0] = offset[0];
    iw->offset[1] = offset[1];
}

int
_dxf_createWindowX(ImageWindow *iw, int map)
{
    XSetWindowAttributes xswa;
    XColor cdef, hdef;
    XWindowAttributes xwa;
    unsigned long eventMask;
    XWindowChanges xwc;
    XSizeHints *xsh = XAllocSizeHints();

    iw->map = map;

    iw->iwx = (struct _iwx *)DXAllocateZero(sizeof(struct _iwx));
    if (! iw->iwx)
	goto error;

    iw->iwx->display = XOpenDisplay(iw->displayString);
    if (! iw->iwx->display)
	goto error;
    
    DXRegisterWindowHandlerWithCheckProc(_dxf_processEvents, XChecker,
	    iw->iwx->display, (void *)iw);

    iw->iwx->XA_WM_PROTOCOLS =
	XInternAtom(iw->iwx->display, "WM_PROTOCOLS", 0);
    iw->iwx->XA_WM_DELETE_WINDOW =
	XInternAtom(iw->iwx->display, "WM_DELETE_WINDOW", 0);

    if (! _dxf_getBestVisual(iw))
	goto error;

    if (iw->iwx->visual != XDefaultVisual(iw->iwx->display, DefaultScreen(iw->iwx->display)))
	xswa.colormap = XCreateColormap(iw->iwx->display,
				RootWindow(iw->iwx->display, DefaultScreen(iw->iwx->display)),
				iw->iwx->visual, AllocNone);
    else
	xswa.colormap = DefaultColormap(iw->iwx->display, DefaultScreen(iw->iwx->display));

    XLookupColor(iw->iwx->display, xswa.colormap, "black", &cdef, &hdef);
    xswa.background_pixel = hdef.pixel;
    xswa.border_pixel = hdef.pixel;

    if (iw->decorations)
        xswa.override_redirect = 0;
    else
        xswa.override_redirect = 1;

    iw->iwx->parent = XCreateWindow(iw->iwx->display,
	    ((iw->parentId) == -1 ? 
		RootWindow(iw->iwx->display, DefaultScreen(iw->iwx->display)) 
		:
		iw->parentId),
	    iw->offset[0], iw->offset[1],
	    iw->size[0], iw->size[1], 0,
	    iw->depth,
	    InputOutput, iw->iwx->visual,
	    CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel , &xswa);

    XStoreName(iw->iwx->display, iw->iwx->parent, iw->title);

    XChangeProperty(iw->iwx->display, iw->iwx->parent,
	iw->iwx->XA_WM_PROTOCOLS, XA_ATOM, 32, PropModeReplace,
	(unsigned char *)&iw->iwx->XA_WM_DELETE_WINDOW,  1);
    
    xsh->flags = 0;
    if (iw->sizeFlag == 1)
	xsh->flags |= USSize;
    if (iw->sizeFlag == 1)
	xsh->flags |= USPosition;

    XSetWMNormalHints(iw->iwx->display, iw->iwx->parent, xsh);

    XLookupColor(iw->iwx->display, xswa.colormap, "black", &cdef, &hdef);
    xswa.background_pixel = hdef.pixel;
    xswa.border_pixel = hdef.pixel;

    iw->iwx->windowId = XCreateWindow(iw->iwx->display,
		iw->iwx->parent, 0, 0,
		iw->size[0], iw->size[1], 0,
		iw->depth, InputOutput, iw->iwx->visual, 
		CWColormap | CWBackPixel | CWBorderPixel , &xswa);
    
    XSetStandardProperties(iw->iwx->display, iw->iwx->windowId,
		iw->title, iw->title, None, NULL, 0, NULL);
    
    XChangeProperty(iw->iwx->display, iw->iwx->windowId,
	iw->iwx->XA_WM_PROTOCOLS, XA_ATOM, 32, PropModeReplace,
	(unsigned char *)&iw->iwx->XA_WM_DELETE_WINDOW,  1);

    if (map == WINDOW_OPEN)
    {
	XMapWindow(iw->iwx->display, iw->iwx->parent);
	XMapWindow(iw->iwx->display, iw->iwx->windowId);
    }
    else if (map == WINDOW_OPEN_RAISED)
    {
	XMapRaised(iw->iwx->display, iw->iwx->parent);
	XMapRaised(iw->iwx->display, iw->iwx->windowId);
    }

    xwc.stack_mode = Above;
    XConfigureWindow(iw->iwx->display, iw->iwx->windowId, CWStackMode, &xwc);

    eventMask = StructureNotifyMask | VisibilityChangeMask;
    XSelectInput(iw->iwx->display, iw->iwx->parent, eventMask);

    eventMask = ButtonPressMask | ButtonReleaseMask | PointerMotionHintMask |
		ButtonMotionMask | ExposureMask |
		KeyPressMask | EnterWindowMask | LeaveWindowMask;

    XSelectInput(iw->iwx->display, iw->iwx->windowId, eventMask);

    XGetWindowAttributes(iw->iwx->display, iw->iwx->parent, &xwa);

    iw->iwx->win_x = xwa.x;
    iw->iwx->win_y = xwa.y;

    XSync(iw->iwx->display, 0);
    XFree(xsh);

    return 1;

error:
    XFree(xsh);
    if (iw->iwx)
	DXFree(iw->iwx);
    return 0;
}

int
_dxf_getWindowId(ImageWindow *iw)
{
    return (int)iw->iwx->windowId;
}

#define ISACCEPTABLE(ret, dpth, clss)				\
{								\
    ret = 0; 							\
    for(i = 0; !ret && acceptableVisuals[i].depth; i++)		\
	if (acceptableVisuals[i].depth == (dpth) &&		\
	    acceptableVisuals[i].class == (clss))  		\
	     ret = 1;						\
}

static  struct visualLookupS  {
    int depth;
    int class;
}  acceptableVisuals[] = {
    {  4, PseudoColor },
    {  4, StaticColor },
    {  4, GrayScale   },
    {  4, StaticGray  },
    {  8, PseudoColor },
    {  8, StaticColor },
    {  8, TrueColor   },
    {  8, DirectColor },
    {  8, GrayScale   },
    {  8, StaticGray  },
    { 12, TrueColor   },
    { 12, DirectColor },
    { 16, TrueColor   },
    { 16, DirectColor },
    { 24, TrueColor   },
    { 24, DirectColor },
    { 32, TrueColor   },
    { 32, DirectColor },
    { 0,  PseudoColor }
};


int
_dxf_checkDepth(ImageWindow *iw, int d)
{
    XVisualInfo		*vi, template;
    int			screen = XDefaultScreen(iw->iwx->display);
    int 		vismask, count;

    template.depth = d;
    template.screen = screen;
    vismask = VisualDepthMask|VisualScreenMask;
    vi = XGetVisualInfo(iw->iwx->display,vismask,&template,&count);
    if (vi)
    {
	XFree ((void *)vi);
	return 1;
    }
    else
	return 0;
}
	

    
static int
_dxf_getBestVisual (ImageWindow *iw)
{
    int			count,i,j;
    XVisualInfo		*visualInfo=NULL,template;
    Visual		*visual;
    unsigned char	acceptable;
    int 		vismask;
    int			screen = XDefaultScreen(iw->iwx->display);

    if (iw->depth == XDefaultDepth(iw->iwx->display, screen))
    {
	visual =  XDefaultVisual(iw->iwx->display, screen);
	ISACCEPTABLE(acceptable, iw->depth, visual->class);
	if (acceptable)
	{
	    iw->iwx->visual = visual;
	    return 1;
	}
    }

    template.depth = iw->depth;
    template.screen = screen;
    vismask = VisualDepthMask|VisualScreenMask;
    visualInfo = XGetVisualInfo(iw->iwx->display,vismask,&template,&count);

    acceptable = 0;
    for (i = 0; !acceptable && acceptableVisuals[i].depth; i++)
	for (j = 0; j < count && !acceptable; j++)
	    if (acceptableVisuals[i].depth == visualInfo[j].depth &&
	       acceptableVisuals[i].class == visualInfo[j].class)
	    {
	       iw->iwx->visual = visualInfo[j].visual;
	       acceptable = 1;
	    }

    if(visualInfo)
	XFree ((void *)visualInfo);
    visualInfo = NULL;

    if (acceptable)
	return 1;

      
   /* If no depth given, or depth was invalid,
    *	then try the default,
    *	then 8 bit,
    *	then any acceptable visual
    */
    iw->depth = XDefaultDepth(iw->iwx->display, screen);
    visual =  XDefaultVisual(iw->iwx->display, screen);
    ISACCEPTABLE(acceptable, iw->depth, visual->class);
    if(acceptable)
    {
	iw->iwx->visual = visual;
	return 1;
    }

    template.depth = iw->depth = 8;
    template.screen = screen;
    vismask = VisualDepthMask|VisualScreenMask;
    visualInfo = XGetVisualInfo(iw->iwx->display,vismask,&template,&count);

    acceptable = 0;
    for (i = 0; !acceptable && acceptableVisuals[i].depth; i++)
	for (j = 0; j < count && !acceptable; j++)
	    if (acceptableVisuals[i].depth == visualInfo[j].depth &&
	       acceptableVisuals[i].class == visualInfo[j].class)
	    {
	       iw->iwx->visual = visualInfo[j].visual;
	       acceptable = 1;
	    }

    if(visualInfo)
	XFree ((void *)visualInfo);
    visualInfo = NULL;

    if (acceptable)
	return 1;

    template.screen = screen;
    vismask = VisualScreenMask;
    visualInfo = XGetVisualInfo(iw->iwx->display,vismask,&template,&count);

    iw->depth =  visualInfo->depth; 

    acceptable = 0;
    for (i = 0; !acceptable && acceptableVisuals[i].depth; i++)
	for (j = 0; j < count && !acceptable; j++)
	    if (acceptableVisuals[i].depth == visualInfo[j].depth &&
	       acceptableVisuals[i].class == visualInfo[j].class)
	    {
	       iw->iwx->visual = visualInfo[j].visual;
	       acceptable = 1;
	    }

    if(visualInfo)
	XFree ((void *)visualInfo);
    visualInfo = NULL;

    if (acceptable)
	return 1;

    return 0;
}
