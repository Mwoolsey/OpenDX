/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "trace.h"

#if defined(WIN32)
#include <dx/dx.h>

Error 
DXVisualizeMemory(int which, int procid)
{
    DXMessage("visual memory display only available in X11 version of DX.");
    return OK;
}

#else   /* 1 */

#include <sys/types.h>
#include <sys/stat.h>

#define Screen DXScreenHack		/* get around libdx Screen def */
#include <dx/dx.h>
#undef Screen

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <signal.h>

typedef Error (*Handler)(int, Pointer);
typedef int   (*Checker)(int, Pointer);

/*
 * calling interface:  VisualizeMemory(int which), where which is a bitmap of:
 *  0 = close existing windows
 *  1 = create and/or update small arena window
 *  2 = create and/or update large arena window
 *  4 = create and/or update local arena window (then how is proc#, -1 is all)
 *
 * things i want to add:
 *  combined small/large in single window?
 *  lower resolution option for server?
 *  allow user-setable pixel per square?
 */

    
#define BORDER 		1		/* outside border width */

#define COLOR_ALLOC	"Brown"	        /* allocated blocks */
#define COLOR_SMALLOC	"Blue2"	        /* small allocated blocks, lg arena */
#define COLOR_FREE	"SteelBlue"	/* free blocks */
#define COLOR_TRACE	"CadetBlue"	/* blocks allocated by us */
#define COLOR_EXTRA	"Black" 	/* trailing pixels at window end */

#define WIN_WIDTH	1024		/* default window width */
#define WIN_HEIGHT	768		/* max window height */
#define PIX_SIZE	4		/* real pixels per edge of virtual p */

static ubyte	color_alloc	= 0;
static ubyte	color_smallalloc= 1;
static ubyte	color_free	= 2;
static ubyte	color_trace	= 3;
static ubyte	color_extra	= 4;


struct dispinfo {
    int active;			/* whether window is active */
    Window wind;		/* Xwindows parameters */
    Display *disp;
    GC gc;
    XImage *memory_image;
    ubyte *memory_pixel;
    char title[128];		/* window title */
    int fd;			/* id for this connection number */
    int refresh;		/* set to cause refresh */
    int query;			/* set to query pointer location */
    int mouse_x;		/* pointer location */
    int mouse_y;
    Pointer query_start;	/* start of mem corresponding to a cell */
    Pointer query_end;          /* end of mem corresponding to a cell */
    int error;			/* whether X error occurred */
    int win_width;		/* current window width and height */
    int win_height;
    int bytes_vp;		/* each virtual pixel represents N bytes */
    int pixel_xmult;		/* real pixels along each each of virtual p */
    int pixel_ymult;		/* real pixels along each each of virtual p */
    int alloc_unit;		/* minimum arena allocation size in bytes */
    Pointer arena_base;		/* first address in this arena */
    ulong arena_size;		/* arena size in bytes (-1 = uninitialized) */
    int which;			/* which arena */
    int nproc;			/* for local, which processor */
    int smallsize;              /* color smaller blocks differently */
    uint *ourmem;		/* memory blocks we have in use */
};


#define WinSize(d)   		(d->win_width * d->win_height)
#define PixelSize(d) 		(d->pixel_xmult * d->pixel_ymult)
#define DivideRoundedUp(x, y)	(((x) + (y) - 1) / (y))
#define RoundUp(x, y) 		(((x) % (y)) ? (x) + (y) - (x) % (y) : (x))
#define RoundDown(x, y) 	((x) - ((x) % (y)))

#define MAXARENAS 34

static int xerror = 0;

static struct dispinfo *d_small = NULL;
static struct dispinfo *d_large = NULL;
static struct dispinfo *d_local[32] = { NULL };


/* prototypes */

static void init_dispinfo(struct dispinfo *d);
static void pix_set(struct dispinfo *d, Pointer start, ulong size, ubyte color);
static Error report_memory(struct dispinfo *d);
static Error query_memory(struct dispinfo *d);
static Display *open_memory_visual_display();
static Error init_memory_visual (struct dispinfo *d);
static void cleanup_memory_visual (struct dispinfo *d);
static int error_handler(Display *dpy, XErrorEvent *event);
static Error handler_script(int fd, struct dispinfo *d);
static Error memsee(int blocktype, Pointer start, ulong size, Pointer p);
static Error memsee1(int blocktype, Pointer start, ulong size, Pointer p);
static Error memquery(int blocktype, Pointer start, ulong size, Pointer p);

void _dxf_sigcatch();
#define NSECONDS 2    /* automatic update */


/* external entry point */

Error 
DXVisualizeMemory(int which, int procid)
{
    int nprocs = 0;
    int i, start, end;

    nprocs = DXProcessors(0);

    if (which == 0) {		/* delete all windows cleanly */
	cleanup_memory_visual(d_small);
	cleanup_memory_visual(d_large);
	
	for (i=0; i<nprocs; i++)
	    cleanup_memory_visual(d_local[i]);
	
	return OK;
    }

    if (which & 1) {		/* small arena */

	/* if already active, just update again and return */
	if (d_small)
	    d_small->refresh = 1;

	else {
	    d_small = 
		(struct dispinfo *)DXAllocateZero(sizeof(struct dispinfo));
	    if (!d_small)
		return ERROR;
	    
	    if (DXGetMemorySize(&d_small->arena_size, NULL, NULL)
		&& DXGetMemoryBase(&d_small->arena_base, NULL, NULL)) {

		d_small->which = MEMORY_SMALL;
		d_small->alloc_unit = 16;
		strcpy(d_small->title, "DX Small Arena Memory");
		
		init_dispinfo(d_small);
		
		if (! init_memory_visual(d_small))
		    return ERROR;
		
		d_small->active = 1;
	    }
	}
    }

    if (which & 2) {		/* large arena */

	/* if already active, just update again and return */
	if (d_large)
	    d_large->refresh = 1;
	
	else {
	    
	    d_large = 
		(struct dispinfo *)DXAllocateZero(sizeof(struct dispinfo));
	    if (!d_large)
		return ERROR;
	    
	    
	    if (DXGetMemorySize(NULL, &d_large->arena_size, NULL) 
		&& DXGetMemoryBase(NULL, &d_large->arena_base, NULL)) {
	
		d_large->which = MEMORY_LARGE;
#if ibmpvs
		d_large->alloc_unit = 1024;
#else
		d_large->alloc_unit = 16;
#endif
		strcpy(d_large->title, "DX Large Arena Memory");
		
		init_dispinfo(d_large);
		
		if (! init_memory_visual(d_large))
		    return ERROR;

		/* if blocks smaller than this are in the large arena,
                 * they are overflow blocks.
		 */
		d_large->smallsize = 1024;  

		d_large->active = 1;
	    }
	}
    }

    if (which & 4) {			/* local memory */
	ulong local_size;

	if (!DXGetMemorySize(NULL, NULL, &local_size) || local_size == 0) {
	    DXSetError(ERROR_DATA_INVALID, "local memory not supported");
	    return ERROR;
	}
	
	nprocs = DXProcessors(0);
	if (procid >= 0 && procid < nprocs)
	    goto done;
	
	/* -1 means all, N means just that specific processor */
	if (procid < 0) {
	    start = 0;
	    end = nprocs;
	} else {
	    start = procid;
	    end = procid + 1;
	}
	
	for (i=start; i < end; i++) {
	    
	    /* if already active, just update again and return */
	    if (d_local[i]) {
		d_local[i]->refresh = 1;
		continue;
	    }
	    
	    
	    d_local[i] = 
		(struct dispinfo *)DXAllocateZero(sizeof(struct dispinfo));
	    if (!d_local[i])
		return ERROR;
	    
	    
	    if (!DXGetMemorySize(NULL, NULL, &d_local[i]->arena_size) 
		|| d_local[i]->arena_size == 0)
		continue;
	    if (!DXGetMemoryBase(NULL, NULL, &d_local[i]->arena_base))
		continue;
	
	    d_local[i]->which = MEMORY_LOCAL;
	    d_local[i]->nproc = i;
	    d_local[i]->alloc_unit = 16;
	    sprintf(d_local[i]->title, "DX Local Arena Memory, Processor %d", i);
	    
	    init_dispinfo(d_local[i]);
	    
	    if (! init_memory_visual(d_local[i]))
		return ERROR;
	    
	    d_local[i]->active = 1;
	}
    }


  done:
    if (d_small && d_small->refresh)
	report_memory(d_small);
    
    if (d_large && d_large->refresh)
	report_memory(d_large);
    
    for (i=0; i<nprocs; i++)
	if (d_local[i] && d_local[i]->refresh)
	    report_memory(d_local[i]);

#if !defined(intelnt)
    signal(SIGALRM, _dxf_sigcatch);
    alarm(NSECONDS);    
#endif
    return OK;
}

/* internal routines */
static void 
init_dispinfo(struct dispinfo *d)
{
    int total_squares;
    ulong h;

    d->win_width = WIN_WIDTH;
    d->win_height = WIN_HEIGHT;

    d->pixel_xmult = PIX_SIZE;
    d->pixel_ymult = PIX_SIZE;

    total_squares = WinSize(d) / PixelSize(d);
    d->bytes_vp = d->alloc_unit;

    /* compute how many bytes of memory one square on the display should 
     * represent so you have the smallest granularity without the window size 
     * exceeding the initial height.
     */
    while (total_squares < DivideRoundedUp(d->arena_size, d->bytes_vp))
	d->bytes_vp *= 2;
    
    h = DivideRoundedUp(d->arena_size, d->bytes_vp) * PixelSize(d);
    d->win_height = DivideRoundedUp(h, d->win_width);
    d->win_height = RoundUp(d->win_height, d->pixel_ymult);
    
    sprintf(d->title + strlen(d->title), " (1 square = %d bytes)", 
	    d->bytes_vp);
}

static void 
resize_display(struct dispinfo *d, int new_width, int new_height)
{
    int total_squares;
    uint h;
    char *cp;

    /* round down to even multiple of square size */
    new_width = RoundDown(new_width, d->pixel_xmult);
    new_height = RoundDown(new_height, d->pixel_ymult);

    if (new_width <= 0 || new_height <= 0)
	return;

    total_squares = (new_width * new_height) / PixelSize(d);
    d->bytes_vp = d->alloc_unit;

    /* compute how many bytes of memory one square on the display should 
     * represent, so you have the smallest granularity without the window size
     * exceeding the initial height.
     */
    while (total_squares < DivideRoundedUp(d->arena_size, d->bytes_vp))
	d->bytes_vp *= 2;
    
    h = DivideRoundedUp(d->arena_size, d->bytes_vp) * PixelSize(d);
    new_height = DivideRoundedUp(h, new_width);
    new_height = RoundUp(new_height, d->pixel_ymult);

    cp = strchr(d->title, '=');
    if (cp)
	sprintf(cp, "= %d bytes)", d->bytes_vp);

    if (!(d->memory_pixel = DXReAllocate((Pointer)d->memory_pixel, 
					 new_width * new_height))) {
	xerror = 1;
	return;
    }
	
    d->win_width = new_width;
    d->win_height = new_height;
    memset (d->memory_pixel, color_free, d->win_width * d->win_height);

    /* don't let X free our pixel buffer while destroying old image */
    d->memory_image->data = NULL;
    XDestroyImage(d->memory_image);

    d->memory_image = XCreateImage(d->disp,
			     XDefaultVisual(d->disp, XDefaultScreen(d->disp)),
			     8, ZPixmap, 0, (char *) d->memory_pixel,
			     d->win_width, d->win_height, 8, 0);

    XResizeWindow(d->disp, d->wind, d->win_width, d->win_height);
    XStoreName(d->disp, d->wind, d->title);

}

static void 
pix_set(struct dispinfo *d, Pointer memstart, ulong memsize, ubyte color)
{
    ulong start, size;			/* start, num to set in 'squares' */
    ulong row, col;			/* window column, row to set */
    ulong part;				/* wrap around edge of window */
    ulong where;
    int i;

    /* round size up.  is start also off by one? */
    start = (((ubyte *)memstart) - 
	     ((ubyte *)d->arena_base))/d->bytes_vp * d->pixel_xmult;
    size = DivideRoundedUp(memsize, d->bytes_vp) * d->pixel_xmult;

    col = start % d->win_width;
    row = (start / d->win_width) * d->pixel_ymult;

    /* turn on pixels for each square, taking care of line wrap */
    while (size > 0) {
	if (col + size >= d->win_width)
	    part = d->win_width - col;
	else
	    part = size;

	for (i=0; i < d->pixel_ymult; i++) {
	    
	    where = (ulong)d->memory_pixel + (row + i) * d->win_width + col;
	    if (where >= (ulong)d->memory_pixel + WinSize(d))
		break;
	    
	    if ((where + part) >= (ulong)d->memory_pixel + WinSize(d))
		part -= (where + part) - 
		    ((ulong)d->memory_pixel + WinSize(d) + 1);

	    memset ((ubyte *)where, color, part);
	}
	col = 0;
	row += d->pixel_ymult;
	size -= part;
    } 

}
	    
/* callbacks from memory manager */

/* turn on pixels corresponding to this memory range */
static Error 
memsee(int blocktype, Pointer start, ulong size, Pointer p)
{
    pix_set((struct dispinfo *)p, start, size, color_alloc);
    return OK;
}

static Error 
memsee1(int blocktype, Pointer start, ulong size, Pointer p)
{
    if (size >= ((struct dispinfo *)p)->smallsize)
	return OK;

    pix_set((struct dispinfo *)p, start, size, color_smallalloc);
    return OK;
}

/* check to see if this block is in the query range */
static Error 
memquery(int blocktype, Pointer start, ulong size, Pointer p)
{
    struct dispinfo *d = (struct dispinfo *)p;
    Pointer end = (Pointer)(((ubyte *)start) + size);
    char *cp;

    if (end < d->query_start || start > d->query_end)
	return OK;

    cp = ((size < d->smallsize) && (blocktype == MEMORY_ALLOCATED)) ? 
	  " overflow " : " ";

    DXMessage("  %4d%s%15s from 0x%08x to 0x%08x", 
	      size, cp,
	      (blocktype == MEMORY_ALLOCATED)?"allocated bytes":"free bytes", 
	      start, end);
#if DB_MEMTRACE
    if (blocktype == MEMORY_ALLOCATED)
	DXMessage("%s", start);
#endif

    return OK;
}


/* the main routine which arranges for the memory manager to call us back
 *  for each allocated block, and then asks the x server to display our
 *  updated image.
 */
static Error    
report_memory (struct dispinfo *d)
{
    int wsquares;		/* count of window squares */
    int mblocks;		/* count of memory blocks */

    /* mark all of memory free
     */
    memset (d->memory_pixel, color_free, WinSize(d));

    /* arrange for the memory manager to call us back with each allocated 
     *  block.  the memsee() routines set the pixels as allocated.
     */
    if (d->which == MEMORY_LOCAL) {
	DXDebugLocalAlloc(d->nproc, MEMORY_ALLOCATED, memsee, (Pointer)d);
    } else {
	DXDebugAlloc(d->which, MEMORY_ALLOCATED, memsee, (Pointer)d);
	if (d->smallsize > 0)
	    DXDebugAlloc(d->which, MEMORY_ALLOCATED, memsee1, (Pointer)d);
    }
	
    /* and finally, see if there are any squares at the end of the window
     *  which are beyond the end of the arena and are only there because
     *  the window is square and the number of rows had to be rounded up.
     */
    wsquares = WinSize(d) / PixelSize(d);
    mblocks = DivideRoundedUp(d->arena_size, d->bytes_vp);
    if (wsquares > mblocks)
	pix_set(d, (Pointer)((ubyte *)d->arena_base + d->arena_size), 
		(wsquares - mblocks) * d->bytes_vp, color_extra);

    
    /* finished image.  make it available to be displayed.
     */
    XPutImage (d->disp, d->wind, d->gc, d->memory_image, 0, 0, 0, 0, 
	       d->win_width, d->win_height);
    XFlush(d->disp);

    return OK;
}

/* if you click the mouse on an allocated block, print out all the regions
 *  of memory which are allocated in that block.  if it's not allocated,
 *  print the start and end addresses which that block represents. 
 */
static Error    
query_memory (struct dispinfo *d)
{
    int pixel;				/* the real pixel they picked */
    int block;				/* the virtual pixel they picked */
    Pointer qstart, qend;		/* the corresponding mem addresses */

    pixel = d->mouse_y * d->win_width + d->mouse_x;
    block = (d->mouse_y / d->pixel_ymult) * (d->win_width / d->pixel_ymult) 
	    + (d->mouse_x / d->pixel_xmult);

    qstart = ((ubyte *)d->arena_base) + block * d->bytes_vp;
    qend = ((ubyte *)qstart) + d->bytes_vp - 1;


    if (d->memory_pixel[pixel] == color_free)
	DXMessage("no allocated memory blocks between 0x%08x and 0x%08x", 
		  qstart, qend);
    else if ((d->memory_pixel[pixel] == color_alloc) || 
             (d->memory_pixel[pixel] == color_smallalloc)) {
	DXMessage("allocated memory blocks in the range: 0x%08x to 0x%08x", 
		  qstart, qend);
	d->query_start = qstart;
	d->query_end = qend;

	if (d->which == MEMORY_LOCAL)
	    DXDebugLocalAlloc(d->nproc, MEMORY_ALLOCATED|MEMORY_FREE, 
			      memquery, (Pointer)d);
	else
	    DXDebugAlloc(d->which, MEMORY_ALLOCATED|MEMORY_FREE, 
			 memquery, (Pointer)d);
	
    } else if (d->memory_pixel[pixel] == color_trace)
	DXMessage("memory block in use by this tracing code");
    else if (d->memory_pixel[pixel] == color_extra)
	DXMessage("past end of allocated memory pool");
    else
	DXMessage("unknown memory block type");

    return OK;
}



static Display *
open_memory_visual_display ()
{
    char	*name;
    Display	*disp;

    if ((name = getenv ("DISPLAY_M")) == (char *) NULL)
	if ((name = getenv ("DISPLAY")) == (char *) NULL)
	    name = "unix:0.0";

    if ((disp = XOpenDisplay (name)) == (Display *) NULL)
    {
	DXSetError (ERROR_INTERNAL, "can't open display '%s'\n", name);
	return ((Display *) NULL);
    }

    return (disp);
}

static int
XChecker(int i, void *d)
{
    struct dispinfo *di = (struct dispinfo *)d;
    return XPending(di->disp);
}

static Error
init_memory_visual (struct dispinfo *d)
{
    int		x, y;
    int		w, h;
    int		border;
    XSizeHints	hint;
    Colormap	colors;
    XColor	shade;
    XColor	exact;
    XGCValues	val;
    long	flags;


    if ((d->disp = open_memory_visual_display ()) == (Display *) NULL)
	return ERROR;
    
    d->fd = ConnectionNumber(d->disp);

    x 	   = 0;	     	/* default to opening window in upper left */
    y 	   = 0;
    w 	   = d->win_width;
    h	   = d->win_height;
    border = BORDER;

    hint.x		= x;
    hint.y		= y;
    hint.width		= w;
    hint.height		= h;
    hint.min_width	= w;
    hint.min_height	= h;
    hint.max_width	= w;
    hint.max_height	= h;
    hint.flags		= PPosition | PSize | USPosition | USSize;

    d->wind = XCreateSimpleWindow (d->disp, DefaultRootWindow (d->disp),
			        x, y, w, h, border,
				WhitePixel (d->disp, DefaultScreen (d->disp)),
				BlackPixel (d->disp, DefaultScreen (d->disp)));

    XSetStandardProperties (d->disp, d->wind, d->title, d->title, None, NULL, 
			    0, &hint);

    XSelectInput(d->disp, d->wind, 
		 ExposureMask | 
		 StructureNotifyMask |
		 ButtonPressMask |
		 ButtonReleaseMask);

    /*
     * Now get the pixel values for the colormaps and create the GCs
     */

    val.background = BlackPixel (d->disp, DefaultScreen (d->disp));
    flags = GCForeground | GCBackground;
    colors = DefaultColormap (d->disp, DefaultScreen (d->disp));

    d->gc = XCreateGC (d->disp, d->wind, flags, &val);

    XAllocNamedColor (d->disp, colors, COLOR_ALLOC, &shade, &exact);
    color_alloc = (ubyte) shade.pixel;
    XAllocNamedColor (d->disp, colors, COLOR_SMALLOC, &shade, &exact);
    color_smallalloc = (ubyte) shade.pixel;
    XAllocNamedColor (d->disp, colors, COLOR_FREE, &shade, &exact);
    color_free = (ubyte) shade.pixel;
    XAllocNamedColor (d->disp, colors, COLOR_TRACE, &shade, &exact);
    color_trace = (ubyte) shade.pixel;
    XAllocNamedColor (d->disp, colors, COLOR_EXTRA, &shade, &exact);
    color_extra = (ubyte) shade.pixel;


    d->memory_pixel = (ubyte *) DXAllocate(w * h);
    memset (d->memory_pixel, color_free, w * h);

    d->memory_image = XCreateImage (d->disp,
			   XDefaultVisual (d->disp, XDefaultScreen (d->disp)),
			   8, ZPixmap, 0, (char *) d->memory_pixel, 
			   w, h, 8, 0);
    
    XSetErrorHandler(error_handler);

    /* refresh image once */
    d->refresh = 1;
    if (!handler_script(d->fd, d))
	goto error;

    /* arrange for it to appear, and appear on top */
    XMapRaised(d->disp, d->wind);
    XFlush(d->disp);

    /* register with executive to get X events */
    DXRegisterInputHandlerWithCheckProc(
	(Handler)handler_script, (Checker)XChecker, d->fd, (Pointer)d);

    if (xerror) goto error;
    return OK;

  error:
    xerror = 0;
    return ERROR;
}

static void 
cleanup_memory_visual (struct dispinfo *d)
{
    if (!d || !d->active)
	return;
    
    /* stop getting X events first
     */
    DXRegisterInputHandlerWithCheckProc(NULL, NULL, d->fd, NULL);

    /* now dump memory and X resources.  i can't find anyplace which
     *  really explains which ones of these get automatically deleted
     *  by XCloseDisplay() and which ones i have to release myself.
     */
    DXFree(d->memory_pixel);

    if (d->memory_image) {		/* really needed? */
	d->memory_image->data = NULL;
	XDestroyImage(d->memory_image);
    }

    XDestroyWindow(d->disp, d->wind);	/* really needed? */
    XFreeGC (d->disp, d->gc);		/* really needed? */

    XCloseDisplay (d->disp);


    /* now free the dispinfo data structure itself */
    DXFree((Pointer)d);
    d = NULL;
}


/* code to support an independently refreshed window
 * mostly stolen from displayx.c
 */



static int
error_handler(Display *dpy, XErrorEvent *event)
{
#define ERRSIZE 100
    char buffer[ERRSIZE];
    DXDebug("X", "error handler");
    XGetErrorText(dpy, event->error_code, buffer, ERRSIZE);
    DXSetError(ERROR_INTERNAL, "X Error: %s", buffer);
    xerror = 1;
    return 0;
}



static char *names[] = {
    "nothing",
    "event 1",
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify"
};
    



static Error
handler_script(int fd, struct dispinfo *d)
{
    XEvent event;
    extern char *names[];

    /* check for broken connection */
    XSync(d->disp, 0);
    if (xerror) goto error;

  again:
    /* while there are events waiting for us? */
    while (XPending(d->disp)) {

	XNextEvent(d->disp, &event);
	DXDebug("X", "event %s", names[event.type]);
	if (xerror) goto error;

	switch (event.type) {
	  case Expose:
	    d->refresh = 1;
	    break;

	  case ButtonPress:
	    d->mouse_x = event.xbutton.x;
	    d->mouse_y = event.xbutton.y;
	    d->refresh = 1;
	    switch (event.xbutton.button) {
	      case 1:
		d->query = 1;
		break;
	      case 2:
		DXPrintAlloc(d->which);
		break;
	      default:
		break;
	    }
	    break;

	  case ConfigureNotify:
	    if ((d->win_width != event.xconfigure.width) ||
	        (d->win_height != event.xconfigure.height)) {
		resize_display(d, event.xconfigure.width, 
			       event.xconfigure.height);
		if (xerror) goto error;
		d->refresh = 1;
	    }
	    break;

	  case DestroyNotify:
	    cleanup_memory_visual(d);
	    if (xerror) goto error;
	    return OK;

	  default:
	    break;
	}
    }

    /* no more events pending.  do we need to refresh the window?
     * query what blocks are underneath the mouse location?
     */
    if (d->refresh) {
	report_memory(d);
	d->refresh = 0;
    }
    if (d->query) {
	query_memory(d);
	d->query = 0;
    }

    /* did more events happen while we were reporting or querying memory? */
    if (XPending(d->disp))
	goto again;

    if (xerror) goto error;
    return OK;

error:
    xerror = 0;
    return ERROR;
}


void _dxf_sigcatch()
{
    if (d_small)
	report_memory(d_small);
    
    if (d_large)
	report_memory(d_large);
    

#if !defined(intelnt)
    signal(SIGALRM, _dxf_sigcatch);
    alarm(NSECONDS);
#endif
}

#endif   /* !(WIN32) */

