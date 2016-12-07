/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(DX_NATIVE_WINDOWS)

#include <dx/dx.h>
Error _dxf_ExInitStatus (int n, int flag)
{
    DXSetError(ERROR_INTERNAL, "No X stuff!!");
    return ERROR;
}

#else /* !DX_NATIVE_WINDOWS */

#define Screen DXScreenHack	/* hack to get around libdx Screen def */
#include <dx/dx.h>
#undef Screen

#include <stdlib.h>

#if DXD_PROCESSOR_STATUS
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#if sgi
#include <limits.h>
#endif

#if defined(HAVE_SYS_SIGNAL_H)
#include <sys/signal.h>
#endif

#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "utils.h"
#include "status.h"

int             *_dxd_exProcessorStatus=NULL;
static int	*old_status		= NULL;
static int	nproc			= 4;
int		_dxd_exStatusPID		= 0;

void _dxf_ExInitStatusW (int n);
void _dxf_ExReportStatus ();

/*
 * Initialize the processor status window.  Fork a subprocess to watch
 * for status changes and update the window.
 */
Error _dxf_ExInitStatus (int n, int flag)
{
    int i;

    nproc = n;

    _dxd_exProcessorStatus = (int *) DXAllocate (n * sizeof (int));
    old_status = (int *) DXAllocate (n * sizeof (int));

    if (_dxd_exProcessorStatus == NULL || old_status == NULL)
	return (ERROR);


    for (i = 0; i < n; i++)
    {
	_dxd_exProcessorStatus[i] = PS_EXECUTIVE;
	old_status[i] = PS_NONE;
    }
    
    if (flag)
    {
	/*
	 * This was calling memfork... that's unnecessary; memfork handles the
	 * run queue pipes, which this doesn't need. GDA
	 *
	 *      switch (_dxd_exStatusPID = DXmemfork (-1))
	 */
	switch (_dxd_exStatusPID = fork())
	{
	    case -1:
		_dxd_exStatusPID = 0;
		perror ("_dxf_ExInitStatus:  fork");
		break;

	    case 0:
		_dxf_ExInitStatusW (n);
		_dxf_ExReportStatus ();
		break;

	    default:
		break;
	}
    }

    return (OK);
}


/*
 * DXFree global memory and kill of the window update process.
 */
void _dxf_ExCleanupStatus ()
{
    if (_dxd_exStatusPID)
#ifdef _WIN32
	kill (_dxd_exStatusPID, SIGTERM);
#else
	kill (_dxd_exStatusPID, SIGKILL);
#endif
    if (_dxd_exProcessorStatus)
	DXFree ((Pointer) _dxd_exProcessorStatus);
    if (old_status)
	DXFree ((Pointer) old_status);
}


/**************************************************************************/

typedef struct
{
    int		status;
    char	*name;
    long	pixel;
} s_color;

static s_color	s_data[] =
{
    {PS_MIN,		"White"},
    {PS_NONE,		"White"},
    {PS_EXECUTIVE,	"Blue"},
    {PS_PARSE,		"Turquoise"},
    {PS_GRAPHGEN,	"MediumTurquoise"},
    {PS_GRAPHQUEUE,	"DarkTurquoise"},
    {PS_RUN,		"Green"},
    {PS_RECLAIM,	"Red"},
    {PS_JOINWAIT,	"Yellow"},
    {PS_NAPPING,	"NavyBlue"},
    {PS_DEAD,		"Magenta"},
    {PS_MAX,		"White"}
};

#define	INITX		30
#define	INITY		50
#define	TITLE		"Processor Status"
#define	OBORDER		2
#define	SEPARATE	10
#define	LBORDER		SEPARATE
#define	RBORDER		SEPARATE
#define	TBORDER		SEPARATE
#define	BBORDER		SEPARATE
#define	WIDTH		20
#define	HEIGHT		20

#define	RX(i)		(LBORDER + i * (WIDTH + SEPARATE))
#define	RY(i)		TBORDER
#define	RW(i)		WIDTH
#define	RH(i)		HEIGHT


static Display		*s_disp = NULL;
static Window		s_wind  = 0;
static GC		s_gc	= NULL;

void _dxf_ExCleanupStatusW (void);

/*
 * Open a connection to the X server.  The environment variable DISPLAY_S
 * can be use to specify a specific display server for the status display.
 * Otherwise, the DISPLAY environment variable is used.
 */
static Display *open_status_display (void)
{
    char	*name;
    Display	*disp;

    if ((name = getenv ("DISPLAY_S")) == (char *) NULL)
	if ((name = getenv ("DISPLAY")) == (char *) NULL)
	    name = "unix:0.0";
    
    if ((disp = XOpenDisplay (name)) == (Display *) NULL)
    {
	fprintf (stderr, "%2d: _dxf_ExInitStatus:  can't open display '%s'\n",
		_dxd_exMyPID, name);
	return ((Display *) NULL);
    }

    return (disp);
}


/*
 * Initialize the status process.  Open an X window and allocate the color
 * cells we need to show the different processor states.
 */
void _dxf_ExInitStatusW (int n)
{
    int		x, y;
    int		w, h;
    int		border;
    XSizeHints	hint;
    static char	title[] = TITLE;
    Colormap	colors;
    XColor	shade;
    XColor	exact;
    int		i;
    XGCValues	val;
    long	flags;


#if 0
    ExSignal (SIGTERM, _dxf_ExCleanupStatusW);
    ExSignal (SIGINT,  _dxf_ExCleanupStatusW);
#endif

    if ((s_disp = open_status_display ()) == (Display *) NULL)
	return;
    
    x 	   = INITX;
    y 	   = INITY;
    w 	   = LBORDER + n * WIDTH + (n - 1) * SEPARATE + RBORDER;
    h	   = TBORDER + HEIGHT + BBORDER;
    border = OBORDER;

    hint.x		= x;
    hint.y		= y;
    hint.width		= w;
    hint.height		= h;
    hint.min_width	= w;
    hint.min_height	= h;
    hint.max_width	= w;
    hint.max_height	= h;
    hint.flags		= PPosition | PSize | USPosition | USSize;

    s_wind = XCreateSimpleWindow (s_disp,
				  DefaultRootWindow (s_disp),
				  x,
				  y,
				  w,
				  h,
				  border,
				  WhitePixel (s_disp, DefaultScreen (s_disp)),
				  BlackPixel (s_disp, DefaultScreen (s_disp)));

    XSetStandardProperties (s_disp, s_wind, title, title, None, NULL, 0, &hint);
    XMapRaised (s_disp, s_wind);

    /*
     * Now get the pixel values for the colormaps and create the GCs
     */

    val.background = BlackPixel (s_disp, DefaultScreen (s_disp));
    flags = GCForeground | GCBackground;
    colors = DefaultColormap (s_disp, DefaultScreen (s_disp));

    s_gc    = XCreateGC (s_disp, s_wind, flags, &val);

    for (i = PS_MIN; i < PS_MAX; i++)
    {
	if (XAllocNamedColor (s_disp, colors, s_data[i].name, &shade, &exact))
	{
	    s_data[i].pixel = shade.pixel;
	}
	else
	{
	    fprintf (stderr,
		    "_dxf_ExInitStatus:  can't allocate color '%s', using white\n",
		    s_data[i].name);
	    s_data[i].pixel = WhitePixel (s_disp, DefaultScreen (s_disp));
	}
    }

    XSync (s_disp, 0);
}

/*
 * Close the display window and free associated X paraphenalia.
 */
void _dxf_ExCleanupStatusW (void)
{
    if (s_wind == (Window)NULL)
	return;

    XFreeGC (s_disp, s_gc);

    XDestroyWindow (s_disp, s_wind);
    XCloseDisplay (s_disp);

    exit (0);
}

/*
 * Run forever.  This process watches for changes in the processors' status
 * flags and updates the window appropriately.
 */
void _dxf_ExReportStatus ()
{
    int		i;
    int		status;
    int		didSomething;
#if sgi
    int	naptime;

    naptime = CLK_TCK / 10;
#endif

    /*
     * Couldn't open the status display, thus we can't report anything.
     */

    if (s_disp == (Display *) NULL)
#if defined(ibmpvs) 
#define LONGTIME 0x7FFFFFFF  /* largest positive signed integer */
        sleep (LONGTIME) ;   /* sleep a long time */
#else
	pause ();            /* sleep forever */
#endif

    for (;;)
    {
#if sgi
	sginap (naptime);
#endif

	/*
	 * Loop over the processor status flags and update window if anything
	 * has changes since the last time we looked.
	 */
	didSomething = FALSE;
	for (i=0; i < nproc; i++)
	{
	    status = _dxd_exProcessorStatus[i];

	    if (old_status[i] == status || status <= PS_MIN || status >= PS_MAX)
		continue;
	
	    didSomething = TRUE;

	    XSetForeground (s_disp, s_gc, s_data[status].pixel);

	    XFillRectangle (s_disp, s_wind, s_gc, RX(i), RY(i), RW(i), RH(i));

	    old_status[i] = status;
	}

	if (didSomething)
	{
	    DXsyncmem ();
	    XSync(s_disp, 0);
	}
    }
}
#endif

#endif /* DX_NATIVE_WINDOWS */
