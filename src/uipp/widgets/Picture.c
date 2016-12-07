/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Shell.h>
#include <X11/keysym.h>
#if defined(aviion)
#include <X11/Xos.h>      /* definitions for time variables */
#endif
#include <Xm/DrawingA.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <math.h>
#include <float.h>
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#include "../widgets/FFloat.h"
#include "../widgets/Image.h"
#include "../widgets/ImageP.h"
#include "../widgets/Picture.h"
#include "../widgets/PictureP.h"

#define SCALE .85

/* External Functions */
void find_color(Widget w, XColor *target); /* From findcolor.c */
void gamma_correct(XColor *cell_def); /* From gamma.c */

/* Local Functions */

static void   alloc_drawing_colors(XmPictureWidget w);
static void   convert_color(XmPictureWidget w, XColor *color);
static int    delete_cursor ( XmPictureWidget  w, int id );
static int    select_cursor ( XmPictureWidget    w , int id);
static int    deselect_cursor (XmPictureWidget w);
static int    constrain( XmPictureWidget w, double *s0, double *s1 ,double *s2,
		double dx, double dy, double dz, 
		double WItrans[4][4], double Wtrans[4][4],
		int sdx, int sdy );
static int    add2historybuffer(XmPictureWidget w, int x, int y);
static void   generate_globe(XmPictureWidget w, double radi);
static void   keyboard_grab(XmPictureWidget w, Boolean grab);
static void   restore_rectangle(XmPictureWidget w);
static void   erase_image(XmPictureWidget w);
static double line_of_sight();
static void   setup_bounding_box(XmPictureWidget w);
static void   calc_projected_axis(XmPictureWidget w);
static void   setup_gnomon(XmPictureWidget w, Boolean rotate);
static void   draw_gnomon (XmPictureWidget w);
static void   restore_cursors_from_canonical_form (XmPictureWidget w);
static void   save_cursors_in_canonical_form (XmPictureWidget w, int i);
static void   draw_rotated_gnomon(XmPictureWidget w);
static void   create_cursor_pixmaps(XmPictureWidget  new);
static int    create_cursor (XmPictureWidget  w);
static void   XmDrawBbox (XmPictureWidget w);
static void   XmDrawGlobe (XmPictureWidget w);
static void   CallCursorCallbacks(XmPictureWidget w, int reason, int cursor_num,
		double screen_x, double screen_y, double screen_z);
static void   CallNavigateCallbacks(XmPictureWidget w, 
		int screen_x, int screen_y, int reason);
XtTimerCallbackProc ButtonTimeOut( XmPictureWidget w, XtIntervalId *id);
XtTimerCallbackProc AutoRepeatTimer( XmPictureWidget w, XtIntervalId *id);
static void draw_arrow_head(XmPictureWidget w, int x, int y, 
				int center_x, int center_y); 
static Boolean  trivial_reject(XmPictureWidget w,
			       double x1, double y1, double z1,
			       double x2, double y2, double z2,
			       double sod_width, double sod_height);
static Boolean  clip_line(XmPictureWidget w, double x1, double y1, double z1,
		       double x2, double y2, double z2,
		       double sod_width, double sod_height,
		       double *new_x1, double *new_y1, double *new_z1,
		       double *new_x2, double *new_y2, double *new_z2);
static void
XmPictureMoveCamera( XmPictureWidget w, 
		 double Wtrans[4][4], 
		 double *from_newx, double *from_newy, double *from_newz,
		 double *up_newx, double *up_newy, double *up_newz);


static int    inverse(), mult44();
static double cofac(), det33() ;
static int    find_closest(), draw_cursors();
static int    set_rot ( double res[4][4],  double s0, short s1 );
static int    I44(double w[4][4]);
static int    set_trans(double res[4][4] , double s0, short s1);
int 	      draw_marker ( XmPictureWidget w );
int 	      project ( XmPictureWidget w, double s0, double s1 ,double s2, 
		double Wtrans[4][4], double WItrans[4][4] );
int move_selected_cursor ( XmPictureWidget  w, int x, int y, double z );


#define superclass (&xmImageClassRec)
#if !defined(HAS_M_PI)
#define M_PI    3.1415926535897931160E0
#endif
#define COS05  0.996
#define SIN15  0.259
#define COS15  0.966
#define SIN25  0.423
#define COS25  0.906
#define SIN45  0.707
#define SIN35  0.574

static void    Initialize( XmPictureWidget request, XmPictureWidget new );
static Boolean SetValues( XmPictureWidget current,
			  XmPictureWidget request,
			  XmPictureWidget new );

static void    ClassInitialize();
static void    Destroy();

static void    Select();
static void    Deselect();
static void    CreateDelete();
static void    BtnMotion();
static void    KeyProc();
#if 0
static void    KeyMode();
#endif
static void    ServerMessage (XmPictureWidget w, XEvent *event);
static void    PropertyNotifyAR (XmPictureWidget w, XEvent *event);
static void    Realize();
static void    Redisplay (XmPictureWidget ww,
			  XExposeEvent* event,
			  Region region);
static void Resize( XmPictureWidget w );
static void
xform_coords( double Wtrans[4][4], 
		 double x, double y, double z,
		 double *newx, double *newy, double *newz);

static void
perspective_divide( XmPictureWidget w,
	double x, double y, double z, double *newx, double *newy);

static void
perspective_divide_inverse( XmPictureWidget w,
	double x, double y, double z, double *newx, double *newy);
static void push_redo_camera(XmPictureWidget w);
static void push_undo_camera(XmPictureWidget w);

/* Default translation table and action list */
#if (XmVersion < 1001)
static char defaultTranslations[] =
    "<BtnDown>:      Select()\n\
     <BtnUp>:        Deselect()\n\
     <BtnMotion>:    BtnMotion()\n\
     <Key>Right:     KeyAct()\n\
     <Key>Left:      KeyAct()\n\
     <Key>Up:        KeyAct()\n\
     <Key>Down:      KeyAct()\n\
     <KeyUp>Right:   KeyAct()\n\
     <KeyUp>Left:    KeyAct()\n\
     <KeyUp>Up:      KeyAct()\n\
     <KeyUp>Down:    KeyAct()\n\
     <ClientMessage>:	ClientMessage()\n\
     <PropertyNotify>:	PropertyNotify()";
#else
static char defaultTranslations[] =
    "<BtnDown>:       Select()\n\
     <BtnUp>:         Deselect()\n\
     <BtnMotion>:     BtnMotion()\n\
     <Key>osfRight:   KeyAct()\n\
     <Key>osfLeft:    KeyAct()\n\
     <Key>osfUp:      KeyAct()\n\
     <Key>osfDown:    KeyAct()\n\
     <KeyUp>osfRight: KeyAct()\n\
     <KeyUp>osfLeft:  KeyAct()\n\
     <KeyUp>osfUp:    KeyAct()\n\
     <KeyUp>osfDown:  KeyAct()\n\
     <ClientMessage>: ClientMessage()\n\
     <PropertyNotify>:	PropertyNotify()";
#endif

/*
     Ctrl<Key>I:     KeyMode()\n\
     Ctrl<Key>G:     KeyMode()\n\
     Ctrl<Key>F:     KeyMode()\n\
     Ctrl<Key>W:     KeyMode()\n\
     Ctrl<Key>X:     KeyMode()\n\
     Ctrl<Key>R:     KeyMode()\n\
     Ctrl<Key>Z:     KeyMode()\n\
     Ctrl<Key>N:     KeyMode()\n\
     Ctrl<Key>U:     KeyMode()\n\
     Ctrl<Key>K:     KeyMode()\n\

     Ctrl<Key>I:     KeyMode()\n\
     Ctrl<Key>G:     KeyMode()\n\
     Ctrl<Key>F:      KeyMode()\n\
     Ctrl<Key>W:      KeyMode()\n\
     Ctrl<Key>X:      KeyMode()\n\
     Ctrl<Key>R:      KeyMode()\n\
     Ctrl<Key>Z:      KeyMode()\n\
     Ctrl<Key>N:      KeyMode()\n\
     Ctrl<Key>U:      KeyMode()\n\
     Ctrl<Key>K:      KeyMode()\n\
*/

static XtActionsRec actionsList[] =
{
   { "Select",        (XtActionProc) Select  },
   { "Deselect",      (XtActionProc) Deselect  },
   { "BtnMotion",     (XtActionProc) BtnMotion },
   { "KeyAct",        (XtActionProc) KeyProc },
/*   { "KeyMode",       (XtActionProc) KeyMode }, */
   { "ClientMessage", (XtActionProc) ServerMessage },
   { "PropertyNotify",(XtActionProc) PropertyNotifyAR },
};


extern void _XmForegroundColorDefault();
extern void _XmBackgroundColorDefault();

static double DefaultAngle = 0.0;

static XtResource resources[] =
{
    { 
      XmNdisplayGlobe, XmCDisplayGlobe, XmRBoolean, sizeof(Boolean),
      XtOffset(XmPictureWidget, picture.display_globe),
      XmRImmediate, (XtPointer) False
    },
   {
      XmNlookAtAngle, XmCLookAtAngle, XmRDouble, sizeof(double),
      XtOffset(XmPictureWidget, picture.look_at_angle),
      XmRDouble, (XtPointer) &DefaultAngle
   },
    {
      XmNlookAtDirection, XmCLookAtDirection, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.look_at_direction),
      XmRImmediate, (XtPointer)XmLOOK_FORWARD 
    },
    {
      XmNnavigateDirection, XmCNavigateDirection, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.navigate_direction),
      XmRImmediate, XmFORWARD 
    },
    {
      XmNtranslateSpeed, XmCTranslateSpeed, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.translate_speed_factor),
      XmRImmediate, (XtPointer)25 
    },
    {
      XmNrotateSpeed, XmCRotateSpeed, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.rotate_speed_factor),
      XmRImmediate, (XtPointer)25 
    },
    {
      XmNconstrainCursor, XmCConstrainCursor, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.constrain_cursor),
      XmRInt, XmCONSTRAIN_NONE 
    },
    { XmNoverlayExposure, XmCOverlayExposure, XmRBoolean, sizeof(Boolean),
      XtOffset(XmPictureWidget, picture.overlay_exposure),
      XmRImmediate, (XtPointer) FALSE
    },
    { 
      XmNoverlayWid, XmCOverlayWid, XmRWindow, sizeof(Window),
      XtOffset(XmPictureWidget, picture.overlay_wid),
      XmRImmediate, NULL 
    },
    { 
      XmNpicturePixmap, XmCPicturePixmap, XmRPixmap, sizeof(Pixmap),
      XtOffset(XmPictureWidget, picture.pixmap),
      XmRImmediate, (XtPointer)XmUNSPECIFIED_PIXMAP 
    },
    {
      XmNmode, XmCMode, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.mode),
      XmRInt, XmNULL_MODE
    },
    { XmNshowRotatingBBox, XmCShowRotatingBBox, XmRBoolean, sizeof(Boolean),
      XtOffset(XmPictureWidget, picture.show_rotating_bbox),
      XmRImmediate, (XtPointer) FALSE
    },
    { XmNnewImage, XmCNewImage, XmRBoolean, sizeof(Boolean),
      XtOffset(XmPictureWidget, picture.new_image),
      XmRImmediate, (XtPointer) FALSE
    },
    {
      XmNcursorShape, XmCCursorShape, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.cursor_shape),
      XmRInt, XmSQUARE 
    },
    {
      XmNglobeRadius, XmCGlobeRadius, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.globe_radius),
      XmRImmediate, (XtPointer)50 
    },
    {
      XmNpictureCursorSize, XmCPictureCursorSize, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.cursor_size),
      XmRImmediate, (XtPointer)5 
    },
    {
      XmNpictureCursorSpeed, XmCPictureCursorSpeed, XmRInt, sizeof(int),
      XtOffset(XmPictureWidget, picture.cursor_speed),
      XmRImmediate, (XtPointer)XmMEDIUM_CURSOR 
    },
    {
      XmNcursorCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.cursor_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {
      XmNpickCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.pick_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {
      XmNmodeCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.mode_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {
      XmNundoCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.undo_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {
      XmNkeyCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.key_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {  
      XmNrotationCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.rotation_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {  
      XmNzoomCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.zoom_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {  
      XmNroamCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.roam_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {  
      XmNnavigateCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.navigate_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {  
      XmNclientMessageCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.client_message_callback),
      XmRImmediate, (XtPointer) NULL
    },
    {  
      XmNpropertyNotifyCallback, XmCCallback, XmRCallback, 
      sizeof (XtCallbackList),
      XtOffset (XmPictureWidget, picture.property_notify_callback),
      XmRImmediate, (XtPointer) NULL
    },
};

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XmPictureClassRec xmPictureClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmImageClassRec,		/* superclass         */
      "XmPicture",				/* class_name         */
      sizeof(XmPictureRec),			/* widget_size        */
      ClassInitialize,				/* class_initialize   */
      NULL,					/* class_part_init    */
      FALSE,					/* class_inited       */
      (XtInitProc) Initialize,			/* initialize         */
      NULL,					/* initialize_hook    */
      Realize,				        /* realize            */
      actionsList,				/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      FALSE,					/* compress_motion    */
      TRUE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      Destroy,					/* destroy            */
      (XtWidgetProc) Resize, 		   	/* resize	      */
      (XtExposeProc)Redisplay,			/* expose             */
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
      NULL,                                     /* extension          */
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
      NULL,     		/* translations           */
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
      NULL,						/* mumble */
   },

   {		/* picture class - none */
      NULL,						/* mumble */
   }
};

WidgetClass xmPictureWidgetClass = (WidgetClass) &xmPictureClassRec;

static void Resize( XmPictureWidget w )
{
double 	radi;

    (*superclass->core_class.resize) ((Widget)w);
    if (!XtIsRealized((Widget)w)) return;
    /* Get the pixmap and its sizes */
    w->picture.PIXMAPWIDTH  = w->core.width;
    w->picture.PIXMAPHEIGHT = w->core.height;

    radi = (double)(w->picture.globe_radius);

    generate_globe ( w, radi ); 
    setup_gnomon(w, False);

    if (w->image.frame_buffer)
	{
	XResizeWindow(XtDisplay(w), 
		  w->picture.overlay_wid, 
		  w->core.width, 
		  w->core.height);

	}
    else
	{
	w->picture.pixmap = XmUNSPECIFIED_PIXMAP;
	}
    w->picture.disable_temp = True;
}


static void Realize ( XmPictureWidget  new, XtValueMask  *value_mask,
		      XSetWindowAttributes *attributes )
{
unsigned long  	     	gc_value_mask;
XGCValues 	     	gc_values;
XSetWindowAttributes 	att;
unsigned 	     	long mask;
char			dash_list[2];

    (*superclass->core_class.realize) ((Widget)new,value_mask, attributes);

    /*
     * FIXME:
     * This chunk of code lives in the top of Initialize also.  It should probably
     * be here only.  I needed to move it here because dx -image has never worked
     * on aviion.  But so close to a release, I didn't want to be making noncrucial
     * changes in this file.  So after the release, erase these ifdefs and the chunk
     * in Initialize.  Alternative: update the dgux and X/Motif on shade and then
     * remove this delta.
     */
#if defined(aviion)
{
Arg	   	wargs[20];
int	   	n;
unsigned long 	valuemask;
XGCValues  	values;
XmFontContext   context;
XmStringCharSet charset;
XmFontList 	font_list;

    /*
     * Create a Popup shell for the pushbutton
     */
    n = 0;
    XtSetArg(wargs[0], XmNtitle, "Cursor X Y Z Position");
    new->picture.popup =
       XtCreatePopupShell("shell", overrideShellWidgetClass, XtParent(new), 
		wargs, 1);
    new->picture.popped_up = False;

    /*
     * Create a PushButton widget to display the x, y, z value of a cursor
     */
    n = 0;
    XtSetArg(wargs[n], XmNx, 0); 				n++;
    XtSetArg(wargs[n], XmNy, 0); 				n++;
    XtSetArg(wargs[n], XmNshadowThickness, 2); 			n++;
    XtSetArg(wargs[n], XmNrecomputeSize, False); 		n++;
    XtSetArg(wargs[n], XmNalignment, XmALIGNMENT_BEGINNING); 	n++;
    XtSetArg(wargs[n], XmNwidth, 350); 				n++;

    new->picture.pb = (XmPushButtonWidget)XmCreatePushButton(new->picture.popup,
				"PushButton", wargs, n);
    XtManageChild((Widget)new->picture.pb);

    /*
     * Set up font inforamtion
     */
    XtSetArg(wargs[0], XmNfontList, &font_list);
    XtGetValues((Widget)new->picture.pb, wargs, 1);

    XmFontListInitFontContext(&context, font_list);
    XmFontListGetNextFont(context, &charset, &new->picture.font);
    XmFontListFreeFontContext(context);

    valuemask = GCFont;
    values.font = new->picture.font->fid;
    new->picture.fontgc = XtGetGC((Widget)new->picture.pb, valuemask, &values);
}
#endif

    /* Get the pixmap and its sizes */
    new->picture.PIXMAPWIDTH  = new->core.width;
    new->picture.PIXMAPHEIGHT = new->core.height;
    new->picture.pixmap = XmUNSPECIFIED_PIXMAP;
    new->picture.globe = NULL;

    /* 
     * If we are on the frame buffer, create an overlay window.
     */
    if(new->image.frame_buffer)
    {
	XFlush(XtDisplay(new));
	mask = CWBackPixel | CWColormap | CWBorderPixel;
	att.background_pixel = new->picture.black;
	att.border_pixel = new->picture.white + 1;
	att.colormap = 
	    DefaultColormap(XtDisplay(new),XScreenNumberOfScreen(XtScreen(new)));
	new->picture.overlay_wid = XCreateWindow(XtDisplay(new),XtWindow(new),
		0, 0, new->core.width, new->core.height, 0, 8, InputOutput,
		DefaultVisual(XtDisplay(new), XScreenNumberOfScreen(XtScreen(new))), 
		mask, &att);
	XFlush(XtDisplay(new));
	XSelectInput(XtDisplay(new), new->picture.overlay_wid, ExposureMask);
	XMapWindow(XtDisplay(new), new->picture.overlay_wid);

	gc_value_mask = GCForeground;
	gc_values.foreground = new->picture.white;
	new->picture.gcovl = 
		XCreateGC(XtDisplay(new), new->picture.overlay_wid,
				gc_value_mask, &gc_values);

	gc_value_mask = GCForeground | GCBackground| GCLineStyle;
	gc_values.foreground = new->picture.white;
	gc_values.background = new->picture.black;
	gc_values.line_style = LineDoubleDash;
	new->picture.gcovl_dash = 
		XCreateGC(XtDisplay(new), new->picture.overlay_wid,
				gc_value_mask, &gc_values);
	dash_list[0] = 2; dash_list[1] = 3;
	XSetDashes(XtDisplay(new), new->picture.gcovl_dash, 0, dash_list, 2);
    }
}


/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	ClassInitialize						     */
/*  Effect:	Install non-standard type converters needed by this widget   */
/*		class							     */
/*                                                                           */
/*****************************************************************************/
static void ClassInitialize()
{
    /*  Install converters for type XmRDouble and XmRFloat  */
    XmAddFloatConverters();
}



/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	Initialize						     */
/*  Effect:	Create and initialize the component widgets		     */
/*                                                                           */
/*****************************************************************************/

static void Initialize( XmPictureWidget request, XmPictureWidget new )
{

int	   	i, j;
Arg	   	wargs[20];
int	   	n;
XmFontList 	font_list;
unsigned long 	valuemask;
XGCValues  	values;
char		dash_list[2];

#if (XmVersion >= 1001)
XmFontContext   context;
XmStringCharSet charset;
#endif


    /*
     * FIXME:
     * This chunk of code lives in the top of Realize also.  See the comment.
     */
#if !defined(aviion)
    /*
     * Create a Popup shell for the pushbutton
     */
    n = 0;
    XtSetArg(wargs[0], XmNtitle, "Cursor X Y Z Position");
    new->picture.popup =
       XtCreatePopupShell("shell", overrideShellWidgetClass, XtParent(new), 
		wargs, 1);
    new->picture.popped_up = False;

    /*
     * Create a PushButton widget to display the x, y, z value of a cursor
     */
    n = 0;
    XtSetArg(wargs[n], XmNx, 0); 				n++;
    XtSetArg(wargs[n], XmNy, 0); 				n++;
    XtSetArg(wargs[n], XmNshadowThickness, 2); 			n++;
    XtSetArg(wargs[n], XmNrecomputeSize, False); 		n++;
    XtSetArg(wargs[n], XmNalignment, XmALIGNMENT_BEGINNING); 	n++;
    XtSetArg(wargs[n], XmNwidth, 350); 				n++;

    new->picture.pb = (XmPushButtonWidget)XmCreatePushButton(new->picture.popup,
				"PushButton", wargs, n);
    XtManageChild((Widget)new->picture.pb);

    /*
     * Set up font inforamtion
     */
    XtSetArg(wargs[0], XmNfontList, &font_list);
    XtGetValues((Widget)new->picture.pb, wargs, 1);

#if (XmVersion < 1001)
    new->picture.font = font_list->font;
#else
    XmFontListInitFontContext(&context, font_list);
    XmFontListGetNextFont(context, &charset, &new->picture.font);
    XmFontListFreeFontContext(context);
#endif
    
    valuemask = GCFont;
    values.font = new->picture.font->fid;
    new->picture.fontgc = XtGetGC((Widget)new->picture.pb, valuemask, &values);
#endif

    /* Indicate that none of the cursors are currently active or selected */
    new->picture.n_cursors = 0;
    new->picture.selected = NULL;
    new->picture.xbuff = NULL;
    new->picture.ybuff = NULL;
    new->picture.zbuff = NULL;
    new->picture.cxbuff = NULL;
    new->picture.cybuff = NULL;
    new->picture.czbuff = NULL;

    new->picture.CursorBlank = False;
    new->picture.FirstTime = True;
    new->picture.FirstTimeMotion = True;

    /* Default the xform to the identity */
    for ( i = 0 ; i < 4 ; i++ )
	{
	for ( j = 0 ; j < 4 ; j++ )
	    {
	    if ( i == j )
		{
		new->picture.Wtrans[i][j] = 1.0;
		}
	    else
		{
		new->picture.Wtrans[i][j] = 0.0;
		}
	    }
	}

    /*screen = XScreenNumberOfScreen(XtScreen(new));*/
    new->picture.rubber_band.gc = NULL;
    new->picture.gcovl = NULL;
    new->picture.gc = XtGetGC((Widget)new, 0, NULL);

    valuemask = GCForeground | GCBackground| GCLineStyle;
    values.foreground = new->picture.white;
    values.background = new->picture.black;
    values.line_style = LineDoubleDash;
    new->picture.gc_dash = XtGetGC((Widget)new, valuemask, &values);
    dash_list[0] = 2; dash_list[1] = 3;
    XSetDashes(XtDisplay(new), new->picture.gc_dash, 0, dash_list, 2);


    new->picture.ActiveSquareCursor[0] = None;
    if ( (new->picture.mode == XmCURSOR_MODE) ||
	 (new->picture.mode == XmROAM_MODE)   ||
	 (new->picture.mode == XmPICK_MODE) )
	{
	create_cursor_pixmaps(new);
	}

    new->picture.tid = (XtIntervalId)NULL;
    new->picture.key_tid = (XtIntervalId)NULL;

    /*
     * disable_temp is set when we call back the application with a 
     * message that will cause an execution.  Subsequent button presses,
     * button motion and button release events are ignored, until a
     * new image is loaded.  This is detected when XmPictureNewCamera is
     * called.
     */
    new->picture.good_at_select = True;
    new->picture.disable_temp = False;
    new->picture.double_click = False;
    new->picture.ignore_new_camera = 0;
    new->picture.globe = NULL;
    new->picture.first_key_press = True;
    new->picture.grab_keyboard_count = 0;
    new->picture.button_pressed = 0;
    new->picture.undo_count = 0;
    new->picture.redo_count = 0;
    new->picture.undo_stk_ptr = -1;
    new->picture.redo_stk_ptr = -1;
    new->picture.camera_defined = False;
    new->picture.piMark = 0;
    new->picture.cursor = None;

    new->picture.white = 0;
    new->picture.black = 0;
    new->picture.box_grey.pixel = 0;

    /* Colors - used to be resources */
    new->picture.box_grey.red = 0x7e7e;
    new->picture.box_grey.green = 0x7e7e;
    new->picture.box_grey.blue = 0x7e7e;
    new->picture.selected_out_cursor_color.red = 0x0000;
    new->picture.selected_out_cursor_color.green = 0xffff;
    new->picture.selected_out_cursor_color.blue = 0x7e7e;
    new->picture.unselected_out_cursor_color.red = 0xffff;
    new->picture.unselected_out_cursor_color.green =0xffff;
    new->picture.unselected_out_cursor_color.blue = 0xffff;

}


/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	Redisplay						     */
/*  Effect:	                                                             */
/*                                                                           */
/*****************************************************************************/
static void Redisplay (XmPictureWidget w,
		       XExposeEvent* event,
		       Region region)
{
   XmDrawingAreaCallbackStruct cb;
   
    cb.reason = XmCR_EXPOSE;
    cb.event = (XEvent *)event;
    cb.window = XtWindow(w);

    XtCallCallbacks ((Widget)w, XmNexposeCallback, &cb);

    if (w->picture.mode == XmROTATION_MODE)
	{
	XmDrawGlobe (w); 
	draw_gnomon (w); 
	}
    if ((w->picture.mode == XmCURSOR_MODE) ||
	(w->picture.mode == XmROAM_MODE) )
	{
	draw_gnomon(w);
	XmDrawGlobe (w); 
	XmDrawBbox (w);    
	draw_cursors(w);
	}
    if (w->picture.mode == XmPICK_MODE)
	{
	draw_cursors(w);
	}
}

/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	SetValues						     */
/*  Effect:	Handles requests to change things from the application	     */
/*                                                                           */
/*****************************************************************************/
static Boolean SetValues( XmPictureWidget current,
			  XmPictureWidget request,
			  XmPictureWidget new )
{
    Boolean redraw = False;

    if ( (new->picture.mode != current->picture.mode) ||
	 (new->picture.display_globe != current->picture.display_globe) )
	{
	if(new->picture.black == new->picture.white)
	    alloc_drawing_colors(new);
	/*
	* Erase the old image by restoring the background image
	*/
	if (current->picture.mode != XmNULL_MODE)
	    {
	    erase_image(new);
	    }
	if (new->picture.mode == XmNULL_MODE) return redraw;
	if ( (new->picture.mode == XmCURSOR_MODE) ||
	     (new->picture.mode == XmROAM_MODE) )
	    {
	    create_cursor_pixmaps(new);
	    setup_bounding_box(new);
	    calc_projected_axis(new);
	    setup_gnomon(new, False);
	    draw_gnomon(new);
	    XmDrawBbox (new);    
	    XmDrawGlobe (new); 
	    restore_cursors_from_canonical_form(new);
	    draw_cursors(new);
	    }
	if (new->picture.mode == XmPICK_MODE)
	    {
	    setup_bounding_box(new);
	    calc_projected_axis(new);
	    XmPictureDeleteCursors(new, -1);
	    create_cursor_pixmaps(new);
	    draw_cursors(new);
	    }
	if (new->picture.mode == XmROTATION_MODE)
	    {
	    setup_bounding_box(new);
	    calc_projected_axis(new);
	    setup_gnomon(new, False);
	    XmDrawGlobe (new); 
	    draw_gnomon (new); 
	    }
	}
    if (new->picture.overlay_exposure)
	{
	if (new->picture.mode == XmROTATION_MODE)
	    {
	    XmDrawGlobe (new);
	    draw_gnomon (new);
	    }
	if ( (new->picture.mode == XmCURSOR_MODE) ||
	     (new->picture.mode == XmROAM_MODE) )
	    {
	    if ( (new->picture.ignore_new_camera == 0) &&
		 (!new->image.frame_buffer) )
		{
		draw_gnomon(new);
		XmDrawBbox (new);
		draw_cursors(new);
		XmDrawGlobe (new);
		}
	    }
	if (new->picture.mode == XmPICK_MODE)
	    {
	    if ( (new->picture.ignore_new_camera == 0) ||
		 (!new->image.frame_buffer) )
		{
		draw_cursors(new);
		}
	    }
	new->picture.overlay_exposure = False;
	}
    if (new->picture.constrain_cursor != current->picture.constrain_cursor)
	{
	setup_bounding_box(new);
	calc_projected_axis(new);
	if ( (new->picture.mode == XmCURSOR_MODE) ||
	     (new->picture.mode == XmROAM_MODE) )
	    {
	    draw_gnomon(new);
	    XmDrawBbox (new);
	    draw_cursors(new);
	    }
	if (new->picture.mode == XmROTATION_MODE)
	    {
	    draw_gnomon (new); 
	    }
	}
    if (new->picture.new_image)
	{
	new->picture.disable_temp = False;
	new->picture.new_image = False;
	}
    return redraw;
}

/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	Destroy						             */
/*  Effect:	Clean up any allocated resources                             */
/*                                                                           */
/*****************************************************************************/
static void Destroy( XmPictureWidget w)
{
    if (w->picture.gc) 
	XtReleaseGC((Widget)w, w->picture.gc);
    if (w->picture.fontgc)
	XtReleaseGC((Widget)w, w->picture.fontgc);
    if (w->picture.rubber_band.gc)
	XtReleaseGC((Widget)w, w->picture.rubber_band.gc);
    if(w->picture.gcovl)
	XFreeGC(XtDisplay(w), w->picture.gcovl);
    if(w->picture.cursor)
	XFreeCursor ( XtDisplay(w), w->picture.cursor );

    if(w->picture.selected) XtFree((char*)w->picture.selected);
    if(w->picture.xbuff)    XtFree((char*)w->picture.xbuff);
    if(w->picture.ybuff)    XtFree((char*)w->picture.ybuff);
    if(w->picture.zbuff)    XtFree((char*)w->picture.zbuff);
    if(w->picture.cxbuff)   XtFree((char*)w->picture.cxbuff);
    if(w->picture.cybuff)   XtFree((char*)w->picture.cybuff);
    if(w->picture.czbuff)   XtFree((char*)w->picture.czbuff);
    if(w->picture.globe)    XtFree((char*)w->picture.globe);
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: restore_rectangle					     */
/* Effect:     erase a "zoom" rectangle by restoring the image               */
/*                                                                           */
/*****************************************************************************/
static void   restore_rectangle(XmPictureWidget w)
{
int x;
int y;
int width;
int height;

    x = w->picture.rubber_band.old_x;
    y = w->picture.rubber_band.old_y;
    width = w->picture.rubber_band.old_width;
    height = w->picture.rubber_band.old_height;

    if((!w->image.frame_buffer) && (w->picture.pixmap != XmUNSPECIFIED_PIXMAP))
	{
	/*
	 * Weird (extra wide) copies are to compensate for GTO bug
	 */
	XCopyArea(XtDisplay(w), w->picture.pixmap, XtWindow(w),
		  w->picture.rubber_band.gc,
		  MAX(x-1,0),
		  MAX(y-1,0),
		  4,
		  height+2,
		  MAX(x-1,0),
		  MAX(y-1,0));

	XCopyArea(XtDisplay(w), w->picture.pixmap, XtWindow(w),
		  w->picture.rubber_band.gc,
		  MAX(x-1,0),
		  MAX(y-1,0),
		  width+2,
		  4,
		  MAX(x-1,0),
		  MAX(y-1,0));

	XCopyArea(XtDisplay(w), w->picture.pixmap, XtWindow(w),
		  w->picture.rubber_band.gc,
		  MAX(x + width-2,0),
		  MAX(y-1,0),
		  4,
		  height+4,
		  MAX(x + width-2,0),
		  MAX(y-1,0));

	XCopyArea(XtDisplay(w), w->picture.pixmap, XtWindow(w),
		  w->picture.rubber_band.gc,
		  MAX(x-1,0),
		  MAX(y + height-2,0),
		  width+4,
		  4,
		  MAX(x-1,0),
		  MAX(y + height-2,0));
	}
    else if (w->image.frame_buffer)
	{
	XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.white + 1 );

	XDrawRectangle(XtDisplay(w), w->picture.overlay_wid, 
		w->picture.gcovl, x, y, width, height);

	XDrawRectangle(XtDisplay(w), w->picture.overlay_wid, 
		w->picture.gcovl, x+1, y+1, width-2, height-2);

	XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.white);
	}
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: erase_image						     */
/* Effect:     Routine to erase the direct interactors                       */
/*                                                                           */
/*****************************************************************************/
static void erase_image(XmPictureWidget w)
{

    /*
     * Erase the old image by restoring the background image
     */
    if(!w->image.frame_buffer)
	{
	if (w->picture.pixmap != XmUNSPECIFIED_PIXMAP)
	    {
	    XCopyArea (XtDisplay(w), 
	       w->picture.pixmap, XtWindow(w), w->picture.gc,
	       (int)(0) ,
	       (int)(0) ,
	       (int)(w->picture.PIXMAPWIDTH),  
	       (int)(w->picture.PIXMAPHEIGHT),  
	       (int)(0), 
	       (int)(0)); 
	    }
	else
	    {
	    XClearArea (XtDisplay(w), 
	       XtWindow(w),
	       (int)(0) ,
	       (int)(0) ,
	       (int)(w->picture.PIXMAPWIDTH),  
	       (int)(w->picture.PIXMAPHEIGHT),  
	       False);
	    }
	}
    else if(w->image.frame_buffer)
	{
	/* Clear the overlay plane to the transparency color */
	XClearArea (XtDisplay(w), 
	       w->picture.overlay_wid,
	       (int)(0) ,
	       (int)(0) ,
	       (int)(w->picture.PIXMAPWIDTH),  
	       (int)(w->picture.PIXMAPHEIGHT),  
	       False);
	}
}

/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	XmPictureReset						     */
/*  Effect:	Reset some important params                	 	     */
/*                                                                           */
/*****************************************************************************/

void XmPictureReset(XmPictureWidget w)
{
    if (w->picture.tid) 
	{
	XtRemoveTimeOut(w->picture.tid);
	}
	w->picture.tid = (XtIntervalId)NULL;
    if (w->picture.key_tid) 
	{
	XtRemoveTimeOut(w->picture.key_tid);
	w->picture.key_tid = (XtIntervalId)NULL;
	}
    if ( w->picture.CursorBlank )
	{
	XUndefineCursor(XtDisplay(w), XtWindow(w));
	w->picture.CursorBlank = False;
	}
    /*
     * Just in case...
     */
    XtUngrabKeyboard((Widget)w, CurrentTime);

    w->picture.mode = XmNULL_MODE;
    w->picture.black = w->picture.white;
    erase_image(w);
}
/*****************************************************************************/
/*                                                                           */
/*  Subroutine:	XmPictureResetCursor					     */
/*  Effect:	Reset some important params                	 	     */
/*                                                                           */
/*****************************************************************************/

void XmPictureResetCursor(XmPictureWidget w)
{
    w->picture.grab_keyboard_count = 0;
    w->picture.ignore_new_camera = 0;
/*     if (XtIsManaged(w->picture.popup)) */
/* 	XtUnmanageChild(w->picture.popup); */
    if (w->picture.popped_up)
	{
	XtPopdown(w->picture.popup);
	w->picture.popped_up = False;
	}

    if ( w->picture.CursorBlank )
	{
	XUndefineCursor(XtDisplay(w), XtWindow(w));
	w->picture.CursorBlank = False;
	}
}
    
/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureSetView						     */
/* Effect:     Convieniece routine to calc new camera direction vector	     */
/*****************************************************************************/
void XmPictureSetView(XmPictureWidget w, int direction, double *from_x,
			double *from_y, double *from_z, double *up_x,
			double *up_y, double *up_z)
{
double	length;
double	tmp_length;
double  dir_x;
double  dir_y;
double  dir_z;


    /*
     * It is possible that an image was created and we were in NULL_MODE,
     * hence its bounding box has not been set up, sooo...
     */
    setup_bounding_box(w);

    dir_x = w->picture.from_x - w->picture.to_x;
    dir_y = w->picture.from_y - w->picture.to_y;
    dir_z = w->picture.from_z - w->picture.to_z;
    length = sqrt(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);

    *up_x = 0.0;
    *up_y = 1.0;
    *up_z = 0.0;

    switch(direction)
	{
	case FRONT:
	  dir_x = 0.0;
	  dir_y = 0.0;
	  dir_z = 1.0;
	  break;
	case OFF_FRONT:
	  dir_x = SIN15;
	  dir_y = SIN15;
	  dir_z = COS15;
	  break;
	case BACK:
	  dir_x = 0.0;
	  dir_y = 0.0;
	  dir_z = -1.0;
	  break;
	case OFF_BACK:
	  dir_x = SIN15;
	  dir_y = SIN15;
	  dir_z = -COS15;
	  break;
	case TOP:
	  dir_x = 0.0;
	  dir_y = 1.0;
	  dir_z = 0.0;
	  *up_x = 0.0;
	  *up_y = 0.0;
	  *up_z = -1.0;
	  break;
	case OFF_TOP:
	  dir_x = -SIN15;
	  dir_y = COS15;
	  dir_z = -SIN15;
	  *up_x = 0.0;
	  *up_y = 0.0;
	  *up_z = -1.0;
	  break;
	case BOTTOM:
	  dir_x = 0.0;
	  dir_y = -1.0;
	  dir_z = 0.0;
	  *up_x = 0.0;
	  *up_y = 0.0;
	  *up_z = 1.0;
	  break;
	case OFF_BOTTOM:
	  dir_x = SIN15;
	  dir_y = -COS15;
	  dir_z = SIN15;
	  *up_x = 0.0;
	  *up_y = 0.0;
	  *up_z = 1.0;
	  break;
	case RIGHT:
	  dir_x = 1.0;
	  dir_y = 0.0;
	  dir_z = 0.0;
	  break;
	case OFF_RIGHT:
	  dir_x = COS05;
	  dir_y = SIN15;
	  dir_z = SIN15;
	  break;
	case LEFT:
	  dir_x = -1.0;
	  dir_y = 0.0;
	  dir_z = 0.0;
	  break;
	case OFF_LEFT:
	  dir_x = -COS05;
	  dir_y = SIN15;
	  dir_z = SIN15;
	  break;
	case DIAGONAL:
	  dir_x = SIN45;
	  dir_y = SIN45;
	  dir_z = SIN45;
	  break;
	case OFF_DIAGONAL:
	  dir_x = SIN35;
	  dir_y = SIN35;
	  dir_z = SIN45;
	  break;
	}

    /*
     * Normalize the length...
     */
    tmp_length = sqrt(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
    dir_x = dir_x/tmp_length;
    dir_y = dir_y/tmp_length;
    dir_z = dir_z/tmp_length;

    /*
     * And restore the original length
     */
    dir_x *=length;
    dir_y *=length;
    dir_z *=length;

    *from_x = dir_x + w->picture.to_x;
    *from_y = dir_y + w->picture.to_y;
    *from_z = dir_z + w->picture.to_z;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureGetUndoCamera					     */
/*****************************************************************************/
extern Boolean XmPictureGetUndoCamera(XmPictureWidget w, 
				double *to_x, double *to_y, double *to_z,
				double *from_x, double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z,
				double *autocamera_width, int *projection,
				double *view_angle)

{
int    undo_stk_ptr;

    if (w->picture.undo_count < 1)
	{
	return False;
	}

    /*
     * Push the current camera onto the redo stack
     */
    push_redo_camera(w);

    undo_stk_ptr = w->picture.undo_stk_ptr;

    *up_x             = w->picture.undo_stack[undo_stk_ptr].up_x;
    *up_y             = w->picture.undo_stack[undo_stk_ptr].up_y;
    *up_z             = w->picture.undo_stack[undo_stk_ptr].up_z;
    *from_x           = w->picture.undo_stack[undo_stk_ptr].from_x;
    *from_y           = w->picture.undo_stack[undo_stk_ptr].from_y;
    *from_z           = w->picture.undo_stack[undo_stk_ptr].from_z;
    *to_x             = w->picture.undo_stack[undo_stk_ptr].to_x;
    *to_y             = w->picture.undo_stack[undo_stk_ptr].to_y;
    *to_z             = w->picture.undo_stack[undo_stk_ptr].to_z;
    *autocamera_width = w->picture.undo_stack[undo_stk_ptr].width;
    *projection	      = w->picture.undo_stack[undo_stk_ptr].projection;
    *view_angle       = w->picture.undo_stack[undo_stk_ptr].view_angle;

    w->picture.undo_stk_ptr = --w->picture.undo_stk_ptr;
    if (w->picture.undo_stk_ptr < 0) 
	{
	w->picture.undo_stk_ptr = UNDO_STACK_DEPTH-1;
	}
    w->picture.undo_count--;

    return True;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureGetRedoCamera					     */
/*****************************************************************************/
extern Boolean XmPictureGetRedoCamera(XmPictureWidget w, 
				double *to_x, double *to_y, double *to_z,
				double *from_x, double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z,
				double *autocamera_width, int *projection,
				double *view_angle)

{
int    redo_stk_ptr;

    if (w->picture.redo_count < 1)
	{
	return False;
	}

    redo_stk_ptr = w->picture.redo_stk_ptr;

    *up_x             = w->picture.redo_stack[redo_stk_ptr].up_x;
    *up_y             = w->picture.redo_stack[redo_stk_ptr].up_y;
    *up_z             = w->picture.redo_stack[redo_stk_ptr].up_z;
    *from_x           = w->picture.redo_stack[redo_stk_ptr].from_x;
    *from_y           = w->picture.redo_stack[redo_stk_ptr].from_y;
    *from_z           = w->picture.redo_stack[redo_stk_ptr].from_z;
    *to_x             = w->picture.redo_stack[redo_stk_ptr].to_x;
    *to_y             = w->picture.redo_stack[redo_stk_ptr].to_y;
    *to_z             = w->picture.redo_stack[redo_stk_ptr].to_z;
    *autocamera_width = w->picture.redo_stack[redo_stk_ptr].width;
    *projection	      = w->picture.redo_stack[redo_stk_ptr].projection;
    *view_angle       = w->picture.redo_stack[redo_stk_ptr].view_angle;

    w->picture.redo_stk_ptr = --w->picture.redo_stk_ptr;
    if (w->picture.redo_stk_ptr < 0) 
	{
	w->picture.redo_stk_ptr = UNDO_STACK_DEPTH-1;
	}
    w->picture.redo_count--;

    w->picture.undo_stk_ptr = (w->picture.undo_stk_ptr + 1) % UNDO_STACK_DEPTH;
    w->picture.undo_count = MIN(UNDO_STACK_DEPTH, w->picture.undo_count+1);

    return True;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: push_undo_camera						     */
/*****************************************************************************/
static void push_undo_camera(XmPictureWidget w)

{
int    undo_stk_ptr;

    /*
     * Reset the redo stack
     */
    w->picture.redo_stk_ptr = -1;
    w->picture.redo_count = 0;

    w->picture.undo_stk_ptr = (w->picture.undo_stk_ptr + 1) % UNDO_STACK_DEPTH;
    undo_stk_ptr = w->picture.undo_stk_ptr;

    w->picture.undo_count = MIN(UNDO_STACK_DEPTH, w->picture.undo_count+1);

    w->picture.undo_stack[undo_stk_ptr].up_x  = w->picture.up_x;
    w->picture.undo_stack[undo_stk_ptr].up_y  = w->picture.up_y;
    w->picture.undo_stack[undo_stk_ptr].up_z  = w->picture.up_z;
    w->picture.undo_stack[undo_stk_ptr].from_x = w->picture.from_x;
    w->picture.undo_stack[undo_stk_ptr].from_y = w->picture.from_y;
    w->picture.undo_stack[undo_stk_ptr].from_z = w->picture.from_z;
    w->picture.undo_stack[undo_stk_ptr].to_x  = w->picture.to_x;
    w->picture.undo_stack[undo_stk_ptr].to_y  = w->picture.to_y;
    w->picture.undo_stack[undo_stk_ptr].to_z  = w->picture.to_z;
    w->picture.undo_stack[undo_stk_ptr].width = w->picture.autocamera_width;
    w->picture.undo_stack[undo_stk_ptr].projection = w->picture.projection;
    w->picture.undo_stack[undo_stk_ptr].view_angle = w->picture.view_angle;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: push_redo_camera						     */
/*****************************************************************************/
static void push_redo_camera(XmPictureWidget w)

{
int    redo_stk_ptr;

    w->picture.redo_stk_ptr = (w->picture.redo_stk_ptr + 1) % UNDO_STACK_DEPTH;
    redo_stk_ptr = w->picture.redo_stk_ptr;

    w->picture.redo_count = MIN(UNDO_STACK_DEPTH, w->picture.redo_count+1);

    w->picture.redo_stack[redo_stk_ptr].up_x  = w->picture.up_x;
    w->picture.redo_stack[redo_stk_ptr].up_y  = w->picture.up_y;
    w->picture.redo_stack[redo_stk_ptr].up_z  = w->picture.up_z;
    w->picture.redo_stack[redo_stk_ptr].from_x = w->picture.from_x;
    w->picture.redo_stack[redo_stk_ptr].from_y = w->picture.from_y;
    w->picture.redo_stack[redo_stk_ptr].from_z = w->picture.from_z;
    w->picture.redo_stack[redo_stk_ptr].to_x  = w->picture.to_x;
    w->picture.redo_stack[redo_stk_ptr].to_y  = w->picture.to_y;
    w->picture.redo_stack[redo_stk_ptr].to_z  = w->picture.to_z;
    w->picture.redo_stack[redo_stk_ptr].width = w->picture.autocamera_width;
    w->picture.redo_stack[redo_stk_ptr].projection = w->picture.projection;
    w->picture.redo_stack[redo_stk_ptr].view_angle = w->picture.view_angle;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: set_nav_camera_from_camera                                    */
/* Effect:     Calculate a navigation direction based on the current camera  */
/*	       and the current "look at" parameters			     */
/*****************************************************************************/
int set_nav_camera_from_camera(XmPictureWidget w)
{
double  l;
double  angle1;
double  angle2;
double  angle3;
double  xform[4][4];
double  dx, dy, dz;
double  up_xp, up_yp, up_zp;
double  dir_x, dir_y, dir_z;
double  dir_xp, dir_yp, dir_zp;
double  dir_xpp, dir_ypp, dir_zpp;
double  new_dir_x, new_dir_y, new_dir_z;
int     direction;
double  angle;

    direction = w->picture.look_at_direction;
    angle = w->picture.look_at_angle;

    /*
     * Note that the normal interpretation of "direction" is reversed here
     * for the convience of calculations.  It is reversed when the final
     * results are calculated.
     */
    dir_x = w->picture.to_x - w->picture.from_x;
    dir_y = w->picture.to_y - w->picture.from_y;
    dir_z = w->picture.to_z - w->picture.from_z;

    /*
     * Rotate the up vector so it is aligned with the Y axis
     */
    l = sqrt(w->picture.up_x*w->picture.up_x + 
	     w->picture.up_z*w->picture.up_z);
    if (l != 0.0)
	{
	angle1 = acos(w->picture.up_z/l);
	}
    else
	{
	angle1 = 0.0;
	}
    if (w->picture.up_x > 0)
	{
	angle1 = -angle1;
	}
    I44(xform);
    set_rot(xform, angle1, Yaxis);
    xform_coords( xform, w->picture.up_x, 
			 w->picture.up_y, 
			 w->picture.up_z, 
			 &up_xp, 
			 &up_yp, 
			 &up_zp);
    xform_coords( xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

    /*
     * Rotation about the x axis
     */
    l = sqrt(up_yp*up_yp + up_zp*up_zp);
    if (l != 0.0)
	{
	angle2 = acos(up_yp/l);
	}
    else
	{
	angle2 = 0.0;
	}
    if (up_zp > 0)
	{
	angle2 = -angle2;
	}

    I44(xform);
    set_rot(xform, angle2, Xaxis);
    xform_coords( xform, dir_xp, dir_yp, dir_zp, &dir_xpp, &dir_ypp, &dir_zpp);

    /*
     * Now, rotate the dir_pp vector about the Y axis so it lies 
     * in the x=0 plane
     */
    l = sqrt(dir_xpp*dir_xpp + dir_zpp*dir_zpp);
    if (l != 0.0)
	{
	angle3 = acos(dir_zpp/l);
	}
    else
	{
	angle3 = 0.0;
	}
    if (dir_xpp > 0)
	{
	angle3 = -angle3;
	}

    I44(xform);

    switch(direction)
	{
	case XmLOOK_LEFT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, -angle,  Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    w->picture.navigate_to_x  = w->picture.from_x + new_dir_x;
	    w->picture.navigate_to_y  = w->picture.from_y + new_dir_y;
	    w->picture.navigate_to_z  = w->picture.from_z + new_dir_z;
	    w->picture.navigate_up_x  = w->picture.up_x;
	    w->picture.navigate_up_y  = w->picture.up_y;
	    w->picture.navigate_up_z  = w->picture.up_z;
	    break;

	case XmLOOK_RIGHT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle,  Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    w->picture.navigate_to_x  = w->picture.from_x + new_dir_x;
	    w->picture.navigate_to_y  = w->picture.from_y + new_dir_y;
	    w->picture.navigate_to_z  = w->picture.from_z + new_dir_z;
	    w->picture.navigate_up_x  = w->picture.up_x;
	    w->picture.navigate_up_y  = w->picture.up_y;
	    w->picture.navigate_up_z  = w->picture.up_z;
	    break;

	case XmLOOK_UP:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, angle,  Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords(xform, w->picture.up_x, 
			w->picture.up_y, 
			w->picture.up_z, 
			&w->picture.navigate_up_x, 
			&w->picture.navigate_up_y, 
			&w->picture.navigate_up_z);

	    w->picture.navigate_to_x  = w->picture.from_x + new_dir_x;
	    w->picture.navigate_to_y  = w->picture.from_y + new_dir_y;
	    w->picture.navigate_to_z  = w->picture.from_z + new_dir_z;
	    break;

	case XmLOOK_DOWN:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, -angle,  Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords(xform, w->picture.up_x, 
			w->picture.up_y, 
			w->picture.up_z, 
			&w->picture.navigate_up_x, 
			&w->picture.navigate_up_y, 
			&w->picture.navigate_up_z);


	    w->picture.navigate_to_x  = w->picture.from_x + new_dir_x;
	    w->picture.navigate_to_y  = w->picture.from_y + new_dir_y;
	    w->picture.navigate_to_z  = w->picture.from_z + new_dir_z;
	    break;

	case XmLOOK_BACKWARD:
	    dx = w->picture.from_x - w->picture.to_x;
	    dy = w->picture.from_y - w->picture.to_y;
	    dz = w->picture.from_z - w->picture.to_z;

	    w->picture.navigate_to_x  = w->picture.from_x + dx;
	    w->picture.navigate_to_y  = w->picture.from_y + dy;
	    w->picture.navigate_to_z  = w->picture.from_z + dz;
	    w->picture.navigate_up_x  = w->picture.up_x;
	    w->picture.navigate_up_y  = w->picture.up_y;
	    w->picture.navigate_up_z  = w->picture.up_z;
	    break;

	case XmLOOK_FORWARD:
	    w->picture.navigate_to_x  = w->picture.to_x;
	    w->picture.navigate_to_y  = w->picture.to_y;
	    w->picture.navigate_to_z  = w->picture.to_z;
	    w->picture.navigate_up_x  = w->picture.up_x;
	    w->picture.navigate_up_y  = w->picture.up_y;
	    w->picture.navigate_up_z  = w->picture.up_z;
	    break;
	}

    w->picture.navigate_from_x = w->picture.from_x;
    w->picture.navigate_from_y = w->picture.from_y;
    w->picture.navigate_from_z = w->picture.from_z;

    return 0;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: set_camera_from_nav_camera                                    */
/* Effect:     Calculate a navigation direction based on the current camera  */
/*	       and the current "look at" parameters			     */
/*****************************************************************************/
int set_camera_from_nav_camera(XmPictureWidget w)
{
double  l;
double  angle1;
double  angle2;
double  angle3;
double  xform[4][4];
double  dx, dy, dz;
double  up_xp, up_yp, up_zp;
double  dir_x, dir_y, dir_z;
double  dir_xp, dir_yp, dir_zp;
double  dir_xpp, dir_ypp, dir_zpp;
double  new_dir_x, new_dir_y, new_dir_z;
int     direction;
double  angle;

    direction = w->picture.look_at_direction;
    angle = w->picture.look_at_angle;

    /*
     * Note that the normal interpretation of "direction" is reversed here
     * for the convience of calculations.  It is reversed when the final
     * results are calculated.
     */
    dir_x = w->picture.navigate_to_x - w->picture.navigate_from_x;
    dir_y = w->picture.navigate_to_y - w->picture.navigate_from_y;
    dir_z = w->picture.navigate_to_z - w->picture.navigate_from_z;

    /*
     * Rotate the up vector so it is aligned with the Y axis
     */
    l = sqrt(w->picture.navigate_up_x*w->picture.navigate_up_x + 
	     w->picture.navigate_up_z*w->picture.navigate_up_z);
    if (l != 0.0)
	{
	angle1 = acos(w->picture.navigate_up_z/l);
	}
    else
	{
	angle1 = 0.0;
	}
    if (w->picture.up_x > 0)
	{
	angle1 = -angle1;
	}
    I44(xform);
    set_rot(xform, angle1, Yaxis);
    xform_coords( xform, w->picture.navigate_up_x, 
			 w->picture.navigate_up_y, 
			 w->picture.navigate_up_z, 
			 &up_xp, 
			 &up_yp, 
			 &up_zp);
    xform_coords( xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

    /*
     * Rotation about the x axis
     */
    l = sqrt(up_yp*up_yp + up_zp*up_zp);
    if (l != 0.0)
	{
	angle2 = acos(up_yp/l);
	}
    else
	{
	angle2 = 0.0;
	}
    if (up_zp > 0)
	{
	angle2 = -angle2;
	}

    I44(xform);
    set_rot(xform, angle2, Xaxis);
    xform_coords( xform, dir_xp, dir_yp, dir_zp, &dir_xpp, &dir_ypp, &dir_zpp);

    /*
     * Now, rotate the dir_pp vector about the Y axis so it lies 
     * in the x=0 plane
     */
    l = sqrt(dir_xpp*dir_xpp + dir_zpp*dir_zpp);
    if (l != 0.0)
	{
	angle3 = acos(dir_zpp/l);
	}
    else
	{
	angle3 = 0.0;
	}
    if (dir_xpp > 0)
	{
	angle3 = -angle3;
	}

    I44(xform);

    switch(direction)
	{
	case XmLOOK_LEFT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle,  Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    w->picture.to_x  = w->picture.navigate_from_x + new_dir_x;
	    w->picture.to_y  = w->picture.navigate_from_y + new_dir_y;
	    w->picture.to_z  = w->picture.navigate_from_z + new_dir_z;
	    w->picture.up_x  = w->picture.navigate_up_x;
	    w->picture.up_y  = w->picture.navigate_up_y;
	    w->picture.up_z  = w->picture.navigate_up_z;
	    break;

	case XmLOOK_RIGHT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, -angle,  Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    w->picture.to_x  = w->picture.navigate_from_x + new_dir_x;
	    w->picture.to_y  = w->picture.navigate_from_y + new_dir_y;
	    w->picture.to_z  = w->picture.navigate_from_z + new_dir_z;
	    w->picture.up_x  = w->picture.navigate_up_x;
	    w->picture.up_y  = w->picture.navigate_up_y;
	    w->picture.up_z  = w->picture.navigate_up_z;
	    break;

	case XmLOOK_UP:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, -angle,  Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords(xform, w->picture.navigate_up_x, 
			w->picture.navigate_up_y, 
			w->picture.navigate_up_z, 
			&w->picture.up_x, 
			&w->picture.up_y, 
			&w->picture.up_z);

	    w->picture.to_x  = w->picture.navigate_from_x + new_dir_x;
	    w->picture.to_y  = w->picture.navigate_from_y + new_dir_y;
	    w->picture.to_z  = w->picture.navigate_from_z + new_dir_z;
	    break;

	case XmLOOK_DOWN:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, angle,  Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords(xform, w->picture.navigate_up_x, 
			w->picture.navigate_up_y, 
			w->picture.navigate_up_z, 
			&w->picture.up_x, 
			&w->picture.up_y, 
			&w->picture.up_z);


	    w->picture.to_x  = w->picture.navigate_from_x + new_dir_x;
	    w->picture.to_y  = w->picture.navigate_from_y + new_dir_y;
	    w->picture.to_z  = w->picture.navigate_from_z + new_dir_z;
	    break;

	case XmLOOK_BACKWARD:
	    dx = w->picture.navigate_from_x - w->picture.navigate_to_x;
	    dy = w->picture.navigate_from_y - w->picture.navigate_to_y;
	    dz = w->picture.navigate_from_z - w->picture.navigate_to_z;

	    w->picture.to_x  = w->picture.navigate_from_x + dx;
	    w->picture.to_y  = w->picture.navigate_from_y + dy;
	    w->picture.to_z  = w->picture.navigate_from_z + dz;
	    w->picture.up_x  = w->picture.navigate_up_x;
	    w->picture.up_y  = w->picture.navigate_up_y;
	    w->picture.up_z  = w->picture.navigate_up_z;
	    break;

	case XmLOOK_FORWARD:
	    w->picture.to_x  = w->picture.navigate_to_x;
	    w->picture.to_y  = w->picture.navigate_to_y;
	    w->picture.to_z  = w->picture.navigate_to_z;
	    w->picture.up_x  = w->picture.navigate_up_x;
	    w->picture.up_y  = w->picture.navigate_up_y;
	    w->picture.up_z  = w->picture.navigate_up_z;
	    break;
	}

    w->picture.from_x = w->picture.navigate_from_x;
    w->picture.from_y = w->picture.navigate_from_y;
    w->picture.from_z = w->picture.navigate_from_z;

    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureChangeLookAt                                         */
/* Effect:     Convieniece routine to calc new camera relative to the current*/
/*	       navigation direction			                     */
/*****************************************************************************/
extern void XmPictureChangeLookAt(XmPictureWidget w, 
	int direction, double angle,
	double *to_x, double *to_y, double *to_z,
	double *from_x, double *from_y, double *from_z, 
	double *up_x, double *up_y, double *up_z, 
	double *autocamera_width)
{
double  l;
double  angle1;
double  angle2;
double  angle3;
double  xform[4][4];
double  dx, dy, dz;
double  up_xp, up_yp, up_zp;
double  dir_x, dir_y, dir_z;
double  dir_xp, dir_yp, dir_zp;
double  dir_xpp, dir_ypp, dir_zpp;
double  new_dir_x, new_dir_y, new_dir_z;

    w->picture.look_at_direction = direction;
    w->picture.look_at_angle = angle;

    /*
     * Note that the normal interpretation of "direction" is reversed here
     * for the convience of calculations.  It is reversed when the final
     * results are calculated.
     */
    dir_x = w->picture.navigate_to_x - w->picture.navigate_from_x;
    dir_y = w->picture.navigate_to_y - w->picture.navigate_from_y;
    dir_z = w->picture.navigate_to_z - w->picture.navigate_from_z;

    /*
     * Rotate the up vector so it is aligned with the Y axis
     */
    l = sqrt(w->picture.navigate_up_x*w->picture.navigate_up_x + 
	     w->picture.navigate_up_z*w->picture.navigate_up_z);
    if (l != 0.0)
	{
	angle1 = acos(w->picture.navigate_up_z/l);
	}
    else
	{
	angle1 = 0.0;
	}
    if (w->picture.navigate_up_x > 0)
	{
	angle1 = -angle1;
	}
    I44(xform);
    set_rot(xform, angle1, Yaxis);
    xform_coords( xform, w->picture.navigate_up_x, 
			 w->picture.navigate_up_y, 
			 w->picture.navigate_up_z, 
			 &up_xp, 
			 &up_yp, 
			 &up_zp);
    xform_coords( xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

    /*
     * Rotation about the x axis
     */
    l = sqrt(up_yp*up_yp + up_zp*up_zp);
    if (l != 0.0)
	{
	angle2 = acos(up_yp/l);
	}
    else
	{
	angle2 = 0.0;
	}
    if (up_zp > 0)
	{
	angle2 = -angle2;
	}

    I44(xform);
    set_rot(xform, angle2, Xaxis);
    xform_coords( xform, dir_xp, dir_yp, dir_zp, &dir_xpp, &dir_ypp, &dir_zpp);

    /*
     * Now, rotate the dir_pp vector about the Y axis so it lies 
     * in the x=0 plane
     */
    l = sqrt(dir_xpp*dir_xpp + dir_zpp*dir_zpp);
    if (l != 0.0)
	{
	angle3 = acos(dir_zpp/l);
	}
    else
	{
	angle3 = 0.0;
	}
    if (dir_xpp > 0)
	{
	angle3 = -angle3;
	}

    I44(xform);
    switch(direction)
	{
	case XmLOOK_LEFT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle,   Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    *to_x             = w->picture.navigate_from_x + new_dir_x;
	    *to_y             = w->picture.navigate_from_y + new_dir_y;
	    *to_z             = w->picture.navigate_from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *up_x             = w->picture.navigate_up_x;
	    *up_y             = w->picture.navigate_up_y;
	    *up_z             = w->picture.navigate_up_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmLOOK_RIGHT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, -angle,  Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    *to_x             = w->picture.navigate_from_x + new_dir_x;
	    *to_y             = w->picture.navigate_from_y + new_dir_y;
	    *to_z             = w->picture.navigate_from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *up_x             = w->picture.navigate_up_x;
	    *up_y             = w->picture.navigate_up_y;
	    *up_z             = w->picture.navigate_up_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmLOOK_UP:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, -angle,  Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords(xform, w->picture.navigate_up_x, 
			w->picture.navigate_up_y, 
			w->picture.navigate_up_z, up_x, up_y, up_z);

	    *to_x             = w->picture.navigate_from_x + new_dir_x;
	    *to_y             = w->picture.navigate_from_y + new_dir_y;
	    *to_z             = w->picture.navigate_from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmLOOK_DOWN:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, angle,   Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords( xform, w->picture.navigate_up_x, 
			w->picture.navigate_up_y, 
			w->picture.navigate_up_z, 
			up_x, up_y, up_z);

	    *to_x             = w->picture.navigate_from_x + new_dir_x;
	    *to_y             = w->picture.navigate_from_y + new_dir_y;
	    *to_z             = w->picture.navigate_from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmLOOK_BACKWARD:
	    dx = w->picture.navigate_from_x - w->picture.navigate_to_x;
	    dy = w->picture.navigate_from_y - w->picture.navigate_to_y;
	    dz = w->picture.navigate_from_z - w->picture.navigate_to_z;

	    *to_x             = w->picture.navigate_from_x + dx;
	    *to_y             = w->picture.navigate_from_y + dy;
	    *to_z             = w->picture.navigate_from_z + dz;
	    dir_x            = w->picture.navigate_from_x - *to_x;
	    dir_y            = w->picture.navigate_from_y - *to_y;
	    dir_z            = w->picture.navigate_from_z - *to_z;
	    *up_x             = w->picture.navigate_up_x;
	    *up_y             = w->picture.navigate_up_y;
	    *up_z             = w->picture.navigate_up_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmLOOK_FORWARD:
	    *to_x             = w->picture.navigate_to_x;
	    *to_y             = w->picture.navigate_to_y;
	    *to_z             = w->picture.navigate_to_z;
	    dir_x            = w->picture.navigate_from_x - *to_x;
	    dir_y            = w->picture.navigate_from_y - *to_y;
	    dir_z            = w->picture.navigate_from_z - *to_z;
	    *up_x             = w->picture.navigate_up_x;
	    *up_y             = w->picture.navigate_up_y;
	    *up_z             = w->picture.navigate_up_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	}
    *from_x = dir_x + *to_x;
    *from_y = dir_y + *to_y;
    *from_z = dir_z + *to_z;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureUndoable					     */
/* Effect:     Indicate whether the picture widget is capable of undoing itsef*/
/*****************************************************************************/
void
XmPicturePushUndoCamera(XmPictureWidget w)
{
    push_undo_camera(w);
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureUndoable					     */
/* Effect:     Indicate whether the picture widget is capable of undoing itsef*/
/*****************************************************************************/
Boolean
XmPictureUndoable(XmPictureWidget w)
{
    return ( (w->picture.undo_count > 0) ? True : False);
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureRedoable					     */
/* Effect:     Indicate whether the picture widget is capable of redoing itsef*/
/*****************************************************************************/
Boolean
XmPictureRedoable(XmPictureWidget w)
{
    return ( (w->picture.redo_count > 0) ? True : False);
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureAlign						     */
/* Effect:     Align the navigation camera with the current view camera	     */
/*****************************************************************************/
void
XmPictureAlign(XmPictureWidget w)
{
    w->picture.look_at_direction   = XmLOOK_FORWARD;
    w->picture.look_at_angle       = 0.0;
    w->picture.navigate_to_x   = w->picture.to_x;;
    w->picture.navigate_to_y   = w->picture.to_y;;
    w->picture.navigate_to_z   = w->picture.to_z;;
    w->picture.navigate_from_x = w->picture.from_x;
    w->picture.navigate_from_y = w->picture.from_y;
    w->picture.navigate_from_z = w->picture.from_z;
    w->picture.navigate_up_x   = w->picture.up_x;
    w->picture.navigate_up_y   = w->picture.up_y;
    w->picture.navigate_up_z   = w->picture.up_z;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureTurnCamera					     */
/* Effect:     Convieniece routine to calc new camera 	     		     */
/*****************************************************************************/
void XmPictureTurnCamera(XmPictureWidget w, int direction, double angle,
				double *to_x, double *to_y, double *to_z,
				double *from_x, double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z,
				double *autocamera_width)

{
double  l;
double  angle1;
double  angle2;
double  angle3;
double  xform[4][4];
double  dx, dy, dz;
double  up_xp, up_yp, up_zp;
double  dir_x, dir_y, dir_z;
double  dir_xp, dir_yp, dir_zp;
double  dir_xpp, dir_ypp, dir_zpp;
double  new_dir_x, new_dir_y, new_dir_z;

    /*
     * Note that the normal interpretation of "direction" is reversed here
     * for the convience of calculations.  It is reversed when the final
     * results are calculated.
     */
    dir_x = w->picture.to_x - w->picture.from_x;
    dir_y = w->picture.to_y - w->picture.from_y;
    dir_z = w->picture.to_z - w->picture.from_z;

    /*
     * Rotate the up vector so it is aligned with the Y axis
     */
    l = sqrt(w->picture.up_x*w->picture.up_x + w->picture.up_z*w->picture.up_z);
    if (l != 0.0)
	{
	angle1 = acos(w->picture.up_z/l);
	}
    else
	{
	angle1 = 0.0;
	}
    if (w->picture.up_x > 0)
	{
	angle1 = -angle1;
	}
    I44(xform);
    set_rot(xform, angle1, Yaxis);
    xform_coords( xform, w->picture.up_x, w->picture.up_y, w->picture.up_z, 
		&up_xp, &up_yp, &up_zp);
    xform_coords( xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

    /*
     * Rotation about the x axis
     */
    l = sqrt(up_yp*up_yp + up_zp*up_zp);
    if (l != 0.0)
	{
	angle2 = acos(up_yp/l);
	}
    else
	{
	angle2 = 0.0;
	}
    if (up_zp > 0)
	{
	angle2 = -angle2;
	}

    I44(xform);
    set_rot(xform, angle2, Xaxis);
    xform_coords( xform, dir_xp, dir_yp, dir_zp, &dir_xpp, &dir_ypp, &dir_zpp);

    /*
     * Now, rotate the dir_pp vector about the Y axis so it lies 
     * in the x=0 plane
     */
    l = sqrt(dir_xpp*dir_xpp + dir_zpp*dir_zpp);
    if (l != 0.0)
	{
	angle3 = acos(dir_zpp/l);
	}
    else
	{
	angle3 = 0.0;
	}
    if (dir_xpp > 0)
	{
	angle3 = -angle3;
	}

    I44(xform);
    switch(direction)
	{
	case XmTURN_LEFT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle,   Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    *to_x             = w->picture.from_x + new_dir_x;
	    *to_y             = w->picture.from_y + new_dir_y;
	    *to_z             = w->picture.from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *up_x             = w->picture.up_x;
	    *up_y             = w->picture.up_y;
	    *up_z             = w->picture.up_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmTURN_RIGHT:
	    /*
	     * Rotate about the z axis
	     */
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, -angle,  Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    *to_x             = w->picture.from_x + new_dir_x;
	    *to_y             = w->picture.from_y + new_dir_y;
	    *to_z             = w->picture.from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *up_x             = w->picture.up_x;
	    *up_y             = w->picture.up_y;
	    *up_z             = w->picture.up_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmTURN_UP:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, -angle,  Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords(xform, w->picture.up_x, w->picture.up_y, 
			w->picture.up_z, up_x, up_y, up_z);

	    *to_x             = w->picture.from_x + new_dir_x;
	    *to_y             = w->picture.from_y + new_dir_y;
	    *to_z             = w->picture.from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmTURN_DOWN:
	    set_rot(xform, angle1,  Yaxis);
	    set_rot(xform, angle2,  Xaxis);
	    set_rot(xform, angle3,  Yaxis);
	    set_rot(xform, angle,   Xaxis);
	    set_rot(xform, -angle3, Yaxis);
	    set_rot(xform, -angle2, Xaxis);
	    set_rot(xform, -angle1, Yaxis);
	    
	    xform_coords( xform, dir_x, dir_y, dir_z,
		 &new_dir_x, &new_dir_y, &new_dir_z);

	    xform_coords( xform, w->picture.up_x, w->picture.up_y, 
			w->picture.up_z, up_x, up_y, up_z);

	    *to_x             = w->picture.from_x + new_dir_x;
	    *to_y             = w->picture.from_y + new_dir_y;
	    *to_z             = w->picture.from_z + new_dir_z;
	    dir_x            = -new_dir_x;
	    dir_y            = -new_dir_y;
	    dir_z            = -new_dir_z;
	    *autocamera_width = w->picture.autocamera_width;
	    break;
	case XmTURN_BACK:
	    dx                = w->picture.from_x - w->picture.to_x;
	    dy                = w->picture.from_y - w->picture.to_y;
	    dz                = w->picture.from_z - w->picture.to_z;

	    *to_x             = w->picture.from_x + dx;
	    *to_y             = w->picture.from_y + dy;
	    *to_z             = w->picture.from_z + dz;
	    dir_x            = w->picture.from_x - *to_x;
	    dir_y            = w->picture.from_y - *to_y;
	    dir_z            = w->picture.from_z - *to_z;
	    *up_x             = w->picture.up_x;
	    *up_y             = w->picture.up_y;
	    *up_z             = w->picture.up_z;
	    *autocamera_width = w->picture.autocamera_width;
	  break;
	}
    *from_x = dir_x + *to_x;
    *from_y = dir_y + *to_y;
    *from_z = dir_z + *to_z;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureLoadCursors					     */
/* Effect:     Convieniece routine to  load a new set of cursors	     */
/*****************************************************************************/
void
XmPictureLoadCursors(XmPictureWidget w,
		     int n_cursors,
		     double **vector_list)
{
int	i;
double	x;
double	y;
double	z;
double	xp;
double	yp;
double	zp;
int	n_cur;

    if (!w->picture.camera_defined) return;
    /*
     * Blow away the old ones.
     */
    /*
     * Set n_cur locally since delete_cursor decrements 
     * 	w->picture.n_cursors.
     */
    n_cur = w->picture.n_cursors;
    for (i = 0; i < n_cur; i++)
	{
	delete_cursor(w, 0);
	}

    /*
     * Create the cursor
     */
    w->picture.selected = (int *)XtMalloc(n_cursors * sizeof(int));
    w->picture.xbuff    = (int *)XtMalloc(n_cursors * sizeof(int));
    w->picture.ybuff    = (int *)XtMalloc(n_cursors * sizeof(int));
    w->picture.zbuff    = (double *)XtMalloc(n_cursors * sizeof(double));
    w->picture.cxbuff   = (double *)XtMalloc(n_cursors * sizeof(double));
    w->picture.cybuff   = (double *)XtMalloc(n_cursors * sizeof(double));
    w->picture.czbuff   = (double *)XtMalloc(n_cursors * sizeof(double));

    /*
     * Load the new ones.
     */
    for (i = 0; i < n_cursors; i++)
	{
	x = *(vector_list[i] + 0);
	y = *(vector_list[i] + 1);
	z = *(vector_list[i] + 2);
	
	/*
	 * xform from world coords --> screen coords
	 */
	xform_coords( w->picture.PureWtrans, x, y, z, &xp, &yp, &zp);
	perspective_divide(w, xp, yp, zp, &xp, &yp);
	w->picture.xbuff[i] = xp;
	w->picture.ybuff[i] = yp;
	w->picture.zbuff[i] = zp;
	save_cursors_in_canonical_form(w, i);
	}

    w->picture.n_cursors = n_cursors;

    /*
     * Grow the bounding box if necessary.
     */
    setup_bounding_box(w);
    if ( (w->picture.mode == XmCURSOR_MODE) ||
	 (w->picture.mode == XmROAM_MODE) )
	{
	erase_image(w);
	draw_gnomon(w);
	XmDrawGlobe(w);
	XmDrawBbox(w);
	draw_cursors(w);
	}
    if (w->picture.mode == XmPICK_MODE)
	{
	erase_image(w);
	draw_cursors(w);
	}
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureDeleteCursor					     */
/* Effect:     Convieniece routine to delete cursors			     */
/*****************************************************************************/
void
XmPictureDeleteCursors(XmPictureWidget w,
		       int cursor_num)
{
int	i;
int	n_cursors;

    /*
     * Blow away the old one(s).
     */
    if (cursor_num == -1)
	{
	/*
	 * Set n_cursors locally since delete_cursor decrements 
	 * 	w->picture.n_cursors.
	 */
	n_cursors = w->picture.n_cursors;
	for (i = 0; i < n_cursors; i++)
	    {
	    delete_cursor(w, 0);
	    }
	}
    else
	{
	if (cursor_num < w->picture.n_cursors)
	    {
	    delete_cursor(w, cursor_num);
	    }
	else
	    {
	    XtWarning ("Bad cursor number in XmPictureDeleteCursors");
	    return;
	    }
	}

    /*
     * Display if required
     */
    if ( (w->picture.mode == XmCURSOR_MODE) ||
	 (w->picture.mode == XmPICK_MODE) )
	{
	draw_cursors(w);
	}

}


/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureNewCamera					     */
/* Effect:     Convieniece routine to set new camera 			     */
/*             if ignore_new_camera == 0, then update all the internal       */
/*                 camera information, and calculate a new transform.  If    */
/*                 ignore_new_camera > 0, then only calc a new transform.    */
/*****************************************************************************/
Boolean
XmPictureNewCamera(XmPictureWidget w,
		   XmBasis basis, double matrix[4][4],
		   double from_x, double from_y, double from_z,
		   double to_x, double to_y, double to_z,
		   double up_x, double up_y, double up_z,
		   int image_width, int image_height, double autocamera_width,
		   Boolean autoaxis_enabled, int projection, double view_angle)
{
double angle1;
double angle2;
double angle3;
double xform[4][4];
double Wtrans[4][4];
double up_xp, up_yp, up_zp;		/* x', y', z' */
double dir_x, dir_y, dir_z;		/* x', y', z' */
double dir_xp, dir_yp, dir_zp;		/* x', y', z' */
double up_xpp, up_ypp, up_zpp;		/* x'', y'', z'' */
double l;
double new_to_x, new_to_y, new_to_z;
int    i;
int    j;
Boolean was_camera_defined = w->picture.camera_defined;
Boolean new_basis;
Boolean new_camera;
struct 
    {
    double x;
    double y;
    double z;
    } v, zaxis, xaxis, yaxis, screen_from;
Boolean good_bbox = True;
double center_x;
double center_y;

    if (w->picture.ignore_new_camera < 0)
	{
#ifdef Comment
	XtWarning("Internal Error - XmPicture");
#endif
	w->picture.ignore_new_camera = 0;
	}
    w->picture.camera_defined = True;
    if (was_camera_defined) {
	new_basis = False;
	for (i = 0; i < 4; i++)
	    {
	    for (j = 0; j < 4; j++)
		{
		if (basis.Bw[i][j] != w->picture.basis.Bw[i][j])
		    {
		    new_basis = True;
		    break;
		    }
		}
	    }
    } else {
	new_basis = True;
    }
    new_camera = False;
    if (w->picture.ignore_new_camera == 0) 
	{
	if( !was_camera_defined ||
	    (from_x           != w->picture.from_x) ||
	    (from_y           != w->picture.from_y) ||
	    (from_z           != w->picture.from_z) ||
	    (up_x             != w->picture.up_x  ) ||
	    (up_y             != w->picture.up_y  ) ||
	    (up_z             != w->picture.up_z  ) ||
	    (to_x             != w->picture.to_x  ) ||
	    (to_y             != w->picture.to_y  ) ||
	    (to_z             != w->picture.to_z  ) ||
	    (autocamera_width != w->picture.autocamera_width) ||
	    (autoaxis_enabled != w->picture.autoaxis_enabled) ||
	    (image_width      != w->picture.image_width) ||
	    (image_height     != w->picture.image_height) )
	    {
	    w->picture.from_x           = from_x;
	    w->picture.from_y           = from_y;
	    w->picture.from_z           = from_z;
	    w->picture.to_x             = to_x;
	    w->picture.to_y             = to_y;
	    w->picture.to_z             = to_z;
	    w->picture.up_x             = up_x;
	    w->picture.up_y             = up_y;
	    w->picture.up_z             = up_z;
	    w->picture.image_height     = image_height;
	    w->picture.image_width      = image_width;
	    w->picture.autocamera_width = autocamera_width;
	    w->picture.autoaxis_enabled = autoaxis_enabled;
	    w->picture.roam_cxbuff      = to_x;
	    w->picture.roam_cybuff      = to_y;
	    w->picture.roam_czbuff      = to_z;

	    /*
	     * If we are not currently navigating, update the navigation camera
	     */
	    if ((w->picture.mode != XmNAVIGATE_MODE)  ||
		 w->picture.first_step)
		{
		set_nav_camera_from_camera(w);
		/*
		 * Handling ignore_new_camera like this allows the first
		 * camera of a navigation interaction to flow into the
		 * widget (this is required since we automatically go into
		 * a perspective camera, and we need to update the data
		 * structure to reflect this).  This means that when starting 
		 * a navigation sequence, we will wait for the first image to
		 * come in before requesting a second.  After the first image,
		 * we stay one ahead of the exec by queueing an execution
		 * after we are informed that the exec has started to work on the
		 * previous request (this is done in XmPictureExecutionState).
		 */
		if ( (w->picture.mode == XmNAVIGATE_MODE) &&
		     (w->picture.button_pressed != 0) )
		    {
		    w->picture.ignore_new_camera++;
		    w->picture.first_step = False;
		    CallNavigateCallbacks(w, w->picture.old_x, w->picture.old_y,
					  XmPCR_DRAG);
		    }
		}

	    new_camera = True;
	    }
	}

    if (new_camera)
	{
	if (!w->picture.globe) generate_globe(w, (double)w->picture.globe_radius);
	dir_x = from_x - to_x;
	dir_y = from_y - to_y;
	dir_z = from_z - to_z;

	/*
	 * Set the globe to match the current image if we are not ignoring it
	 * (i.e. during interactive rotation)
	 */
	if (w->picture.ignore_new_camera == 0) 
	    {
	    /*
	     * Rotation about the y axis
	     */
	    l = sqrt(dir_x*dir_x + dir_z*dir_z);
	    if (l != 0.0)
		{
		angle1 = acos(dir_z/l);
		}
	    else
		{
		angle1 = 0.0;
		}
	    if (dir_x > 0)
		{
		angle1 = -angle1;
		}
	    I44(xform);
	    set_rot(xform, angle1, Yaxis);
	    xform_coords(xform, up_x, up_y, up_z, &up_xp, &up_yp, &up_zp);
	    xform_coords(xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

	    /*
	     * Rotation about the x axis
	     */
	    l = sqrt(dir_yp*dir_yp + dir_zp*dir_zp);
	    if (l != 0.0)
		{
		angle2 = acos(dir_zp/l);
		}
	    else
		{
		angle2 = 0.0;
		}
	    if (dir_yp < 0)
		{
		angle2 = -angle2;
		}

	    I44(xform);
	    set_rot(xform, angle2, Xaxis);
	    xform_coords(xform, up_xp, up_yp, up_zp, &up_xpp, &up_ypp, &up_zpp);

	    /*
	     * Rotate about the z axis so that the up vector is aligned 
	     * w/ the y axis
	     */
	    l = sqrt(up_xpp*up_xpp+ up_ypp*up_ypp);
	    if (l != 0.0)
		{
		angle3 = acos(up_ypp/l);
		}
	    else
		{
		angle3 = 0.0;
		}
	    if (up_xpp < 0)
		{
		angle3 = -angle3;
		}

	    I44(w->picture.globe->Wtrans);
	    set_rot(w->picture.globe->Wtrans, angle1, Yaxis);
	    set_rot(w->picture.globe->Wtrans, -angle2, Xaxis);
	    set_rot(w->picture.globe->Wtrans, -angle3, Zaxis);
	    }
	}

    /*
     * Calculate the new world to screen transform
     */
    v.x = from_x - to_x;
    v.y = from_y - to_y;
    v.z = from_z - to_z;
    
    l = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    zaxis.x = v.x/l;
    zaxis.y = v.y/l;
    zaxis.z = v.z/l;

    /* up X zaxis */
    v.x = up_y * zaxis.z - up_z * zaxis.y;
    v.y = up_z * zaxis.x - up_x * zaxis.z;
    v.z = up_x * zaxis.y - up_y * zaxis.x;

    l = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    xaxis.x = v.x/l;
    xaxis.y = v.y/l;
    xaxis.z = v.z/l;

    /* zaxis X xaxis */
    v.x = zaxis.y * xaxis.z - zaxis.z * xaxis.y;
    v.y = zaxis.z * xaxis.x - zaxis.x * xaxis.z;
    v.z = zaxis.x * xaxis.y - zaxis.y * xaxis.x;

    l = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    yaxis.x = v.x/l;
    yaxis.y = v.y/l;
    yaxis.z = v.z/l;

    for (i = 0; i < 4; i++)
	for (j = 0; j < 4; j++)
	    w->picture.PureWtrans[i][j] = matrix[i][j];
	
    if (new_camera || new_basis)
    {
	inverse(matrix, xform);
	for (i = 0; i < 4; i++)
	    xform_coords(xform, basis.Bw[i][0], basis.Bw[i][1], basis.Bw[i][2],
			&basis.Bw[i][0], &basis.Bw[i][1], &basis.Bw[i][2]);
    }

    I44(Wtrans);

    I44(xform);
    xform[3][0] = -from_x;
    xform[3][1] = -from_y;
    xform[3][2] = -from_z;

    mult44(Wtrans, xform);

    I44(xform);
    xform[0][0] = xaxis.x; xform[0][1] = -yaxis.x; xform[0][2] = zaxis.x;
    xform[1][0] = xaxis.y; xform[1][1] = -yaxis.y; xform[1][2] = zaxis.y;
    xform[2][0] = xaxis.z; xform[2][1] = -yaxis.z; xform[2][2] = zaxis.z;

    mult44(Wtrans, xform);
    
    I44(xform);
    xform[0][0] = (double)image_width/autocamera_width;
    xform[1][1] = (double)image_width/autocamera_width;
    mult44(Wtrans, xform);


    xform_coords(Wtrans, to_x, to_y, to_z, &new_to_x, &new_to_y, &new_to_z);
    
    I44(xform);
    xform[3][0] += image_width/2;
    xform[3][1] += image_height/2;
    xform[3][2] -= new_to_z;

    mult44(Wtrans, xform);

    mult44(w->picture.PureWtrans, Wtrans);

    inverse ( w->picture.PureWtrans, w->picture.PureWItrans );

    /*
     * Pre-calculate values needed for perspective
     */
    if (projection == 1)
	{
	w->picture.DW = (image_width/2)/tan(2*M_PI*view_angle/360);
	w->picture.DH = (image_height/2)/tan(2*M_PI*view_angle/360);

	/*
	 * w->picture.D is the value the camera SHOULD be at.  Since it is
	 * not, calc a zscale factor, and use it to adjust the z's for
	 * perspective calculations.
	 * 			D = camera_z * zscale
	 */
	xform_coords(w->picture.PureWtrans, from_x, from_y, from_z, 
	    &screen_from.x, &screen_from.y, &screen_from.z);
	w->picture.zscale_width = w->picture.DW/screen_from.z; 
	w->picture.zscale_height = w->picture.DH/screen_from.z; 
	
	}
    w->picture.view_angle = view_angle;
    w->picture.projection = projection;

    if (w->picture.ignore_new_camera == 0) 
	{
	if (new_basis) w->picture.basis = basis;

	if ( (new_basis || new_camera) &&
	     ((w->picture.mode == XmCURSOR_MODE) ||
	      (w->picture.mode == XmROAM_MODE)) )
	    {
	    setup_bounding_box(w);
	    calc_projected_axis(w);
	    setup_gnomon(w, False);
	    restore_cursors_from_canonical_form(w);
	    }
	if ( (w->picture.mode == XmCURSOR_MODE) ||
	     (w->picture.mode == XmROAM_MODE) )
	    {
	    if ( (!w->image.frame_buffer) || new_basis || new_camera)
		{
		erase_image(w);
		draw_gnomon(w);
		XmDrawGlobe (w);
		XmDrawBbox (w);
		draw_cursors(w);
		}
	    }
	if (w->picture.mode == XmROTATION_MODE)
	    {
	    if ( (!w->image.frame_buffer) || new_camera)
		{
		setup_bounding_box(w);
		calc_projected_axis(w);
		setup_gnomon(w, False);
		erase_image(w);
		XmDrawGlobe (w);
		draw_gnomon (w);
		}
	    }
	if (new_basis || new_camera)
	    {
	    for(i = 0; i < 4; i++)
		{
		xform_coords(  w->picture.PureWtrans,
			    basis.Bw[i][0], basis.Bw[i][1], basis.Bw[i][2],
			   &basis.Bs[i][0],&basis.Bs[i][1],&basis.Bs[i][2]);
		}
	    center_x = w->core.width/2;
	    center_y = w->core.height/2;
	    if ( (fabs(basis.Bs[0][0] - center_x) < 1) &&
		 (fabs(basis.Bs[1][0] - center_x) < 1) &&
		 (fabs(basis.Bs[2][0] - center_x) < 1) &&
		 (fabs(basis.Bs[3][0] - center_x) < 1) &&
		 (fabs(basis.Bs[0][1] - center_y) < 1) &&
		 (fabs(basis.Bs[1][1] - center_y) < 1) &&
		 (fabs(basis.Bs[2][1] - center_y) < 1) &&
		 (fabs(basis.Bs[3][1] - center_y) < 1) )
		good_bbox = False;
	    }
	    
	}
    if (w->image.frame_buffer)
	{
	w->picture.disable_temp = False;
	}

    return good_bbox;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureExecutionState					     */
/* Effect:     Find out EXACTLY when the exec begins a new execution or      */
/*	       ends an execution when in background mode.		     */
/*****************************************************************************/
void
XmPictureExecutionState(XmPictureWidget w, Boolean begin)
{
    if( !((w->picture.mode == XmNAVIGATE_MODE) &&
	 (w->picture.button_pressed != 0)) )
	{
	return;
	}

    if (!w->picture.first_step)
	{
	if(begin)
	    {
	    CallNavigateCallbacks(w, w->picture.old_x, w->picture.old_y,
				  XmPCR_DRAG);
	    }
	}
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmPictureMoveCamera					     */
/* Effect:     Routine to calc new direction and up vector given a rot matrix*/
/*****************************************************************************/
static void
XmPictureMoveCamera( XmPictureWidget w, 
		     double Wtrans[4][4], 
		     double *from_newx, double *from_newy, double *from_newz,
		     double *up_newx, double *up_newy, double *up_newz)
{
double angle1;
double angle2;
double angle3;
double l;
double xform[4][4];
double up_x, up_y, up_z;		/* x, y, z */
double dir_x, dir_y, dir_z;		/* x, y, z */
double up_xp, up_yp, up_zp;		/* x', y', z' */
double dir_xp, dir_yp, dir_zp;		/* x', y', z' */
double up_xpp, up_ypp, up_zpp;		/* x'', y'', z'' */
double dir_newx, dir_newy, dir_newz;	/* x'', y'', z'' */

    /*
     * The overall stategy is to rotate the direction vector so it is aligned
     * with the positive Z axis, then rotate the up vector so it is aligned 
     * with the positive y axis.  Build the matrix that performs these rotates,
     * apply it to a camera dir of (0, 0, 1) and an up of (0, 1, 0)
     */

    xform_coords( Wtrans, 0.0, 1.0, 0.0, &up_x, &up_y, &up_z);
    xform_coords( Wtrans, 0.0, 0.0, 1.0, &dir_x, &dir_y, &dir_z);

    /*
     * Rotation about the y axis
     */
    l = sqrt(dir_x*dir_x + dir_z*dir_z);
    if (l != 0.0)
	{
	angle1 = acos(dir_z/l);
	}
    else
	{
	angle1 = 0.0;
	}
    if (dir_x > 0)
	{
	angle1 = -angle1;
	}
    I44(xform);
    set_rot(xform, angle1, Yaxis);
    xform_coords( xform, up_x, up_y, up_z, &up_xp, &up_yp, &up_zp);
    xform_coords( xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

    /*
     * Rotation about the x axis
     */
    l = sqrt(dir_yp*dir_yp + dir_zp*dir_zp);
    if (l != 0.0)
	{
	angle2 = acos(dir_zp/l);
	}
    else
	{
	angle2 = 0.0;
	}
    if (dir_yp < 0)
	{
	angle2 = -angle2;
	}

    I44(xform);
    set_rot(xform, angle2, Xaxis);
    xform_coords(xform, up_xp, up_yp, up_zp, &up_xpp, &up_ypp, &up_zpp);

    /*
     * Rotate about the z axis so that the up vector is aligned w/ the y axis
     */
    l = sqrt(up_xpp*up_xpp+ up_ypp*up_ypp);
    if (l != 0.0)
	{
	angle3 = acos(up_ypp/l);
	}
    else
	{
	angle3 = 0.0;
	}
    if (up_xpp < 0)
	{
	angle3 = -angle3;
	}

    I44(xform);
    set_rot(xform, angle1, Yaxis);
    set_rot(xform, -angle2, Xaxis);
    set_rot(xform, -angle3, Zaxis);
    
    /*
     * We want to preserve the original length of the
     * vector between the from point and the to point.
     */
    l = sqrt( (w->picture.from_x - w->picture.to_x) *
	      (w->picture.from_x - w->picture.to_x) +
	      (w->picture.from_y - w->picture.to_y) *
	      (w->picture.from_y - w->picture.to_y) +
	      (w->picture.from_z - w->picture.to_z) *
	      (w->picture.from_z - w->picture.to_z) );

    xform_coords( xform, 0.0, 1.0, 0.0, up_newx, up_newy, up_newz);
    xform_coords( xform, 0.0, 0.0, l, &dir_newx, &dir_newy, &dir_newz);

    *from_newx = dir_newx + w->picture.to_x;
    *from_newy = dir_newy + w->picture.to_y;
    *from_newz = dir_newz + w->picture.to_z;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: xform_coords						     */
/* Effect:     Convieniece routine to rotate a point given a rotation matrix*/
/*****************************************************************************/
static void
xform_coords( double Wtrans[4][4], 
		 double x, double y, double z,
		 double *newx, double *newy, double *newz)
{
    *newx = x*Wtrans[0][Xaxis] +
	    y*Wtrans[1][Xaxis] +
	    z*Wtrans[2][Xaxis] +
	      Wtrans[3][Xaxis];
    *newy = x*Wtrans[0][Yaxis] +
	    y*Wtrans[1][Yaxis] +
	    z*Wtrans[2][Yaxis] +
	      Wtrans[3][Yaxis];
    *newz = x*Wtrans[0][Zaxis] +
	    y*Wtrans[1][Zaxis] +
	    z*Wtrans[2][Zaxis] +
	      Wtrans[3][Zaxis];

}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: perspective_divide					     */
/* Effect:     								     */
/*****************************************************************************/
static void
perspective_divide( XmPictureWidget w,
	double x, double y, double z, double *newx, double *newy)
{
    if(w->picture.projection == 0)
	{
	*newx = x;
	*newy = y;
	return;
	}
    x = x - w->core.width/2;
    y = y - w->core.height/2;

    *newx = x*-w->picture.DW/(z*w->picture.zscale_width - w->picture.DW);
    *newy = y*-w->picture.DH/(z*w->picture.zscale_height - w->picture.DH);

    *newx += w->core.width/2;
    *newy += w->core.height/2;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: perspective_divide_inverse				     */
/* Effect:     								     */
/*****************************************************************************/
static void
perspective_divide_inverse( XmPictureWidget w,
	double x, double y, double z, double *newx, double *newy)
{
    if(w->picture.projection == 0)
	{
	*newx = x;
	*newy = y;
	return;
	}
    x = x - w->core.width/2;
    y = y - w->core.height/2;

    *newx = x / (-w->picture.DW/(z*w->picture.zscale_width - w->picture.DW));
    *newy = y / (-w->picture.DH/(z*w->picture.zscale_height - w->picture.DH));

    *newx += w->core.width/2;
    *newy += w->core.height/2;
}

#if 0
/************************************************************************
 *
 *  KeyMode
 *
 ************************************************************************/
static void KeyMode (w, event)
XmPictureWidget w;
XKeyEvent *event;

{
XComposeStatus compose;
int            charcount;
char           buffer[20];
int            bufsize = 20;
XmPictureCallbackStruct cb;

    charcount = 
	XLookupString(event, buffer, bufsize, &w->picture.keysym, &compose);

    cb.reason = XmPCR_MODE;
    switch(w->picture.keysym)
	{
	case XK_w:
	case XK_W:
	    cb.mode = XmROAM_MODE;
	    break;
	case XK_i:
	case XK_I:
	    cb.mode = XmPICK_MODE;
	    break;
	case XK_x:
	case XK_X:
	    cb.mode = XmCURSOR_MODE;
	    break;
	case XK_r:
	case XK_R:
	    cb.mode = XmROTATION_MODE;
	    break;
	case XK_z:
	case XK_Z:
	    cb.mode = XmZOOM_MODE;
	    break;
	case XK_n:
	case XK_N:
	    cb.mode = XmNAVIGATE_MODE;
	    break;
	case XK_g:
	case XK_G:
	    cb.mode = XmPANZOOM_MODE;
	    break;
	case XK_u:
	case XK_U:
	    cb.reason = XmPCR_UNDO;
	    break;
	case XK_k:
	case XK_K:
	case XK_f:
	case XK_F:
	    cb.reason = XmPCR_KEY;
	    cb.keysym = w->picture.keysym;
	    break;
	}
    if(cb.reason == XmPCR_MODE)
	{ 
	XtCallCallbacks ((Widget)w, XmNmodeCallback, &cb);
	}
    else if(cb.reason == XmPCR_KEY)
	{
	XtCallCallbacks ((Widget)w, XmNkeyCallback, &cb);
	}
    else
	{ 
	XtCallCallbacks ((Widget)w, XmNundoCallback, &cb);
	}
}
#endif


/************************************************************************
 *
 *  KeyProc
 *
 ************************************************************************/
static void KeyProc (w, event)
XmPictureWidget w;
XKeyEvent *event;

{
XComposeStatus compose;
char           buffer[20];
int            bufsize = 20;
XmPictureCallbackStruct cb;

	XLookupString(event, buffer, bufsize, &w->picture.keysym, &compose);
    if (w->picture.mode == XmNAVIGATE_MODE)
	{
	/*
	 * Do not allow "roll" if we are not looking forward.
	 */
	if ( (w->picture.look_at_direction != XmLOOK_FORWARD) &&
	     ( (w->picture.keysym == XK_Left) || 
	       (w->picture.keysym == XK_Right) ) )
	    {
	    return;
	    }
	if (event->type == KeyPress)
	    {
	    keyboard_grab(w, True);
	    if (w->picture.first_key_press)
		{
		XAutoRepeatOff(XtDisplay(w));
		w->picture.key_tid = 
		    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
			(unsigned long)400, (XtTimerCallbackProc)AutoRepeatTimer, w);
		}
	    switch(w->picture.keysym)
		{
		case XK_Up:
		    w->picture.translate_speed_factor += 1;
		    break;

		case XK_Down:
		    w->picture.translate_speed_factor -= 1;
		    break;

		case XK_Left:
		    w->picture.rotate_speed_factor -= 1;
		    break;

		case XK_Right:
		    w->picture.rotate_speed_factor += 1;
		    break;

		default:
		    break;
		}

	    w->picture.rotate_speed_factor = 
		    MAX(0, w->picture.rotate_speed_factor);
	    w->picture.rotate_speed_factor = 
		    MIN(100, w->picture.rotate_speed_factor);
	    w->picture.translate_speed_factor = 
		    MAX(0, w->picture.translate_speed_factor);
	    w->picture.translate_speed_factor = 
		    MIN(100, w->picture.translate_speed_factor);

	    if ((w->picture.keysym == XK_Up) || (w->picture.keysym == XK_Down))
		{
		cb.reason = XmPCR_TRANSLATE_SPEED;
		cb.translate_speed = w->picture.translate_speed_factor;
		XtCallCallbacks ((Widget)w, XmNnavigateCallback, &cb);
		}
	    if ( (w->picture.keysym == XK_Left) || 
		 (w->picture.keysym == XK_Right) )
		{
		cb.reason = XmPCR_ROTATE_SPEED;
		cb.rotate_speed = w->picture.rotate_speed_factor;
		XtCallCallbacks ((Widget)w, XmNnavigateCallback, &cb);
		}
	    }
	if (event->type == KeyRelease)
	    {
	    keyboard_grab(w, False);
	    XAutoRepeatOn(XtDisplay(w));
	    if (w->picture.key_tid)
		{
		XtRemoveTimeOut(w->picture.key_tid);
		w->picture.key_tid = (XtIntervalId)NULL;
		}
	    w->picture.first_key_press = True;
	    }
	else
	    {
	    w->picture.first_key_press = False;
	    }
	}
    else if (w->picture.mode == XmROAM_MODE)
	{
#ifdef Comment
	if (w->picture.first_key_press)
	    {
	    XAutoRepeatOff(XtDisplay(w));
	    xform_coords( w->picture.PureWtrans, 
		w->picture.to_x, w->picture.to_y, w->picture.to_z,
		&w->picture.arrow_roam_x, &w->picture.arrow_roam_y, 
		&w->picture.arrow_roam_z);
	    w->picture.key_tid = 
		XtAppAddTimeOut(XtWidgetToApplicationContext(w),
			(unsigned long)400, AutoRepeatTimer, w);
	    }
	switch(w->picture.keysym)
	    {
	    case XK_Up:
		w->picture.arrow_roam_y += 5;
		break;

	    case XK_Down:
		w->picture.arrow_roam_y -= 5;
		break;

	    case XK_Left:
		w->picture.arrow_roam_x += 5;
		break;

	    case XK_Right:
		w->picture.arrow_roam_x -= 5;
		break;
	    }
	if (event->type == KeyRelease)
	    {
	    XAutoRepeatOn(XtDisplay(w));
	    if (w->picture.key_tid)
		{
		XtRemoveTimeOut(w->picture.key_tid);
		w->picture.key_tid = NULL;
		}
	    w->picture.first_key_press = True;
	    }
	else
	    {
	    w->picture.first_key_press = False;
	    }
	if (event->type == KeyPress)
	    {
	    CallCursorCallbacks(w, XmPCR_MOVE, 0, 
				w->picture.arrow_roam_x, 
				w->picture.arrow_roam_y,
				w->picture.arrow_roam_z);
	    }
#endif
	}
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: AutoRepeatTimer						     */
/* Effect:     								     */
/*                                                                           */
/*****************************************************************************/
XtTimerCallbackProc AutoRepeatTimer( XmPictureWidget w, XtIntervalId *id)
{
XmPictureCallbackStruct cb;

    w->picture.key_tid = 
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
			(unsigned long)100, (XtTimerCallbackProc)AutoRepeatTimer, w);

    if (w->picture.mode == XmNAVIGATE_MODE)
	{
	switch(w->picture.keysym)
	    {
	    case XK_Up:
		w->picture.translate_speed_factor += 3;
		break;

	    case XK_Down:
		w->picture.translate_speed_factor -= 3;
		break;

	    case XK_Left:
		w->picture.rotate_speed_factor -= 3;
		break;

	    case XK_Right:
		w->picture.rotate_speed_factor += 3;
		break;

	    default:
		break;
	    }

	w->picture.rotate_speed_factor = 
		MAX(0, w->picture.rotate_speed_factor);
	w->picture.rotate_speed_factor = 
		MIN(100, w->picture.rotate_speed_factor);
	w->picture.translate_speed_factor = 
		MAX(0, w->picture.translate_speed_factor);
	w->picture.translate_speed_factor = 
		MIN(100, w->picture.translate_speed_factor);

	if ( (w->picture.keysym == XK_Up) || (w->picture.keysym == XK_Down) )
	    {
	    cb.reason = XmPCR_TRANSLATE_SPEED;
	    cb.translate_speed = w->picture.translate_speed_factor;
	    XtCallCallbacks ((Widget)w, XmNnavigateCallback, &cb);
	    }
	if ( (w->picture.keysym == XK_Left) || (w->picture.keysym == XK_Right) )
	    {
	    cb.reason = XmPCR_ROTATE_SPEED;
	    cb.rotate_speed = w->picture.rotate_speed_factor;
	    XtCallCallbacks ((Widget)w, XmNnavigateCallback, &cb);
	    }

	}
    else if (w->picture.mode == XmROAM_MODE)
	{
#ifdef Comment
	switch(w->picture.keysym)
	    {
	    case XK_Up:
		w->picture.arrow_roam_y += 5;
		break;

	    case XK_Down:
		w->picture.arrow_roam_y -= 5;
		break;

	    case XK_Left:
		w->picture.arrow_roam_x += 5;
		break;

	    case XK_Right:
		w->picture.arrow_roam_x -= 5;
		break;
	    }
	CallCursorCallbacks(w, XmPCR_MOVE, 0, 
			    w->picture.arrow_roam_x, 
			    w->picture.arrow_roam_y,
			    w->picture.arrow_roam_z);
#endif
	}
    return NULL;
}

/************************************************************************
 *
 *  BtnMotion
 *      This function processes motion events occuring on the
 *      drawing area. 
 *
 ************************************************************************/

static void BtnMotion (w, event)
XmPictureWidget w;
XEvent *event;

{

int     i, x, y; 
double  dx, dy, dz;
double  major_dx, major_dy;
struct  globe    *globe;
double  ay, az;
double  l;
double  angle;
double  angle_tol;
Boolean warp;
double  aspect;
int	width;
int	height;
int	trans_x;
int	trans_y;
int	id;
XEvent  ev;
XmPictureCallbackStruct    cb;

    if (w->picture.button_pressed == 0) return;
    if ( (w->picture.disable_temp) || (!w->picture.good_at_select) ) 
	{
	return;
	}

    x = event->xmotion.x ;
    y = event->xmotion.y ;
    while (XCheckMaskEvent(XtDisplay(w), ButtonMotionMask, &ev))
	{
	x = ev.xmotion.x;
	y = ev.xmotion.y;
	}

    if ( w->picture.mode == XmPICK_MODE )
	{
	return;
	}
    if ( w->picture.mode == XmZOOM_MODE )
	{
	if (w->picture.rubber_band.gc == NULL) return;

	if ( (!(event->xmotion.state & Button1Mask)) &&
	     (!(event->xmotion.state & Button3Mask)) )
	    {
	    return;
	    }
	/*
	 * Restrict the zoom box to the window
	 */
	if (x < 0) x = 0; 
	if (x > w->core.width-1) x = w->core.width - 1; 
	if (y < 0) y = 0; 
	if (y > w->core.height-1) y = w->core.height - 1; 

	/*
	 * Clear the old rectangle.
	 */
	restore_rectangle(w);

	aspect = w->picture.rubber_band.aspect;
	width  = w->core.width;
	height = w->core.height;

	trans_x = x - w->picture.rubber_band.center_x;
	trans_y = y - w->picture.rubber_band.center_y;

	if( ((trans_y >= trans_x*aspect) && (trans_y >= -trans_x*aspect)) ||
	    ((trans_y <= -trans_x*aspect)&& (trans_y <= trans_x*aspect)) )
	    {
	    y = height/2 - abs(trans_y);
	    x = y/aspect;
	    }
	else
	    {
	    x = width/2 - abs(trans_x);
	    y = x*aspect;
	    }
	    
	height = height - 2*y;
	width  = width  - 2*x;

	height = MAX(height, 3);
	width = MAX(width, 3);
	/*
	 * Draw the two rectangles (one in black, one in white)
	 */
	if (!w->image.frame_buffer)
	    {
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x+1, y+1, width-2, height-2);
	    }
	else
	    {
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x+1, y+1, width-2, height-2);
	    }

	/*
	 * Remember the position so we can erase it next time.
	 */
	w->picture.rubber_band.old_x      = x;
	w->picture.rubber_band.old_y      = y;
	w->picture.rubber_band.old_width  = width;
	w->picture.rubber_band.old_height = height;
	}
    if ( w->picture.mode == XmPANZOOM_MODE )
	{
	if (w->picture.rubber_band.gc == NULL) return;

	if ( (!(event->xmotion.state & Button1Mask)) &&
	     (!(event->xmotion.state & Button3Mask)) )
	    {
	    return;
	    }

	/*
	 * Clear the old rectangle.
	 */
	restore_rectangle(w);

	aspect = w->picture.rubber_band.aspect;

	trans_x = x - w->picture.rubber_band.center_x;
	trans_y = y - w->picture.rubber_band.center_y;

	if( ((trans_y >= trans_x*aspect) && (trans_y >= -trans_x*aspect)) ||
	    ((trans_y <= -trans_x*aspect)&& (trans_y <= trans_x*aspect)) )
	    {
	    y = w->picture.rubber_band.center_y - abs(trans_y);
	    height = 2*(w->picture.rubber_band.center_y - y);
	    width  = height/aspect;
	    x = w->picture.rubber_band.center_x - width/2;
	    }
	else
	    {
	    x = w->picture.rubber_band.center_x - abs(trans_x);
	    width  = 2*(w->picture.rubber_band.center_x - x); 
	    height = width*aspect;
	    y = w->picture.rubber_band.center_y - height/2;
	    }
	    
	/*
	 * Make the width and height at least two so some X servers don't die.
	 */
	width = MAX(width, 3);
	height = MAX(height, 3);

	/*
	 * Draw the two rectangles (one in black, one in white)
	 */
	if (!w->image.frame_buffer)
	    {
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x+1, y+1, width-2, height-2);
	    }
	else
	    {
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x+1, y+1, width-2, height-2);
	    }

	/*
	 * Remember the position so we can erase it next time.
	 */
	w->picture.rubber_band.old_x      = x;
	w->picture.rubber_band.old_y      = y;
	w->picture.rubber_band.old_width  = width;
	w->picture.rubber_band.old_height = height;
	}

    if ( (w->picture.mode == XmROTATION_MODE ) ||
	 (((w->picture.mode == XmCURSOR_MODE) ||
	   (w->picture.mode == XmROAM_MODE)) &&
	  (w->picture.button_pressed != Button1)) )
	{
#ifdef Comment
	if ( ((w->picture.mode == XmCURSOR_MODE) ||
	      (w->picture.mode == XmROAM_MODE)) &&
	      (event->xmotion.state & Button1Mask))
	    {
	    return;
	    }
#endif
	globe = w->picture.globe;

	x = x - w->core.width/2;
	y = (y - w->core.height/2);

	if (w->picture.K < 2)
	    {
	    add2historybuffer(w, x, y);
	    return;
	    }

	major_dx = x - w->picture.px; 
	major_dy = y - w->picture.py;
	if ((major_dx == 0.0) && (major_dy == 0.0) ) return;

	if (!globe->on_sphere)
	    {
	    l = sqrt(x*x+y*y);
	    if (l == 0.0) return;
	    az = acos((double)x/l);
	    if (y > 0)
		{
		az = 2*M_PI - az;
		}
	    /*
	     * The last point was also off the sphere
	     */
	    set_rot(globe->Wtrans, -(az - globe->paz), Zaxis);
	    globe->paz = az;
	    }
	else /* On the sphere... */
	    {
	    dx = major_dx;
	    dy = major_dy;
	    az = acos(dx/sqrt(dx*dx + dy*dy));
	    if (dy > 0.0) az = 2*M_PI - az;
	    set_rot(globe->Wtrans, az, Zaxis);

	    l = sqrt((w->picture.px - x)*(w->picture.px - x) + 
		     (w->picture.py - y)*(w->picture.py - y) );

	    ay = (-2*M_PI*l/globe->circumference);
	    set_rot(globe->Wtrans, -ay, Yaxis);

	    set_rot(globe->Wtrans, -az, Zaxis);
	    add2historybuffer(w, x, y);
	    }
	draw_rotated_gnomon(w);
	XmDrawGlobe (w); 

	XmPictureMoveCamera( w, w->picture.globe->Wtrans,
		 &cb.from_x, &cb.from_y, &cb.from_z,
		 &cb.up_x, &cb.up_y, &cb.up_z);

	cb.event = event;
	cb.reason = XmPCR_DRAG;
	XtCallCallbacks ((Widget) w, XmNrotationCallback, &cb);
	}

    if ( ((w->picture.mode == XmCURSOR_MODE) ||
	 (w->picture.mode == XmROAM_MODE)) &&
	 (w->picture.button_pressed == Button1) )
	{
	if (!w->image.frame_buffer)
	    {
	    if (w->picture.pixmap == XmUNSPECIFIED_PIXMAP) return;
	    }
	/*
	 * If we have no selected cursors, return
	 */
	if(w->picture.mode == XmCURSOR_MODE)
	    {
	    for(i = 0; i < w->picture.n_cursors; i++)
		{
		if (w->picture.selected[i]) break;
		}
	    if (i == w->picture.n_cursors) return;
	    }
	else if(w->picture.mode == XmROAM_MODE)
	    {
	    if (!w->picture.roam_selected) return;
	    }
	
	warp = False;
	if (x <= 0)
	    {
	    x = w->core.width/2;
	    w->picture.px += x;
	    w->picture.ppx += x;
	    w->picture.pppx += x;
	    warp = True;
	    }
	if (x >= w->core.width-1)
	    {
	    x = w->core.width/2;
	    w->picture.px -= x;
	    w->picture.ppx -= x;
	    w->picture.pppx -= x;
	    warp = True;
	    }
	if (y <= 0)
	    {
	    y = w->core.width/2;
	    w->picture.py += y;
	    w->picture.ppy += y;
	    w->picture.pppy += y;
	    warp = True;
	    }
	if (y >= w->core.width-1)
	    {
	    y = w->core.width/2;
	    w->picture.py -= y;
	    w->picture.ppy -= y;
	    w->picture.pppy -= y;
	    warp = True;
	    }
	if(warp)
	    {
	    XWarpPointer(XtDisplay(w), None, XtWindow(w), 0,0, 0,0, x,y);
	    }

	if ( w->picture.K < 4 )
	    {
	    add2historybuffer(w, x, y);
	    return;
	    }

	major_dx = x - w->picture.pppx; 
	major_dy = y - w->picture.pppy;
	if ((major_dx == 0.0) && (major_dy == 0.0) ) return;

	angle = acos( major_dx/sqrt(major_dx*major_dx + major_dy*major_dy));
	if( major_dy  > 0 )
	    {
	    angle = 2*M_PI - angle;
	    }

	angle_tol = 0.2;

	dz = 0.0;
	if ( (fabs(w->picture.angle_posz - angle) < angle_tol) ||
	     (2*M_PI - fabs(w->picture.angle_posz - angle) < angle_tol) )
	    {
	    dz = 1.0;
	    }
	if ( (fabs(w->picture.angle_negz - angle) < angle_tol) ||
	     (2*M_PI - fabs(w->picture.angle_negz - angle) < angle_tol) )
	    {
	    dz = -1.0;
	    }

	dx = 0.0;
	if ( (fabs(w->picture.angle_posx - angle) < angle_tol) ||
	     (2*M_PI - fabs(w->picture.angle_posx - angle) < angle_tol) )
	    {
	    dx = 1.0;
	    }
	if ( (fabs(w->picture.angle_negx - angle) < angle_tol) ||
	     (2*M_PI - fabs(w->picture.angle_negx - angle) < angle_tol) )
	    {
	    dx = -1.0;
	    }

	dy = 0.0;
	if ( (fabs(w->picture.angle_posy - angle) < angle_tol) ||
	     (2*M_PI - fabs(w->picture.angle_posy - angle) < angle_tol) )
	    {
	    dy = 1.0;
	    }
	if ( (fabs(w->picture.angle_negy - angle) < angle_tol) ||
	     (2*M_PI - fabs(w->picture.angle_negy - angle) < angle_tol) )
	    {
	    dy = -1.0;
	    }

	/*
	 * Constrain motion along a single axis
	 */
	switch(w->picture.constrain_cursor)
	    {
	    case XmCONSTRAIN_NONE:
		break;

	    case XmCONSTRAIN_X:
		dy = dz = 0.0;
		break;

	    case XmCONSTRAIN_Y:
		dx = dz = 0.0;
		break;

	    case XmCONSTRAIN_Z:
		dx = dy = 0.0;
		break;
	    }
	if (!w->picture.x_movement_allowed) dx = 0.0;
	if (!w->picture.y_movement_allowed) dy = 0.0;
	if (!w->picture.z_movement_allowed) dz = 0.0;

	/*
	 * Constrain motion within the bounding box and the widget
	 */
	constrain(w, &w->picture.X, &w->picture.Y, &w->picture.Z, dx, dy, dz,
			w->picture.WItrans, w->picture.Wtrans,
			x - w->picture.px, y - w->picture.py);

	/* Restore the image over the previous marks */
	if ( (!w->image.frame_buffer)  && 
	     (w->picture.pixmap != XmUNSPECIFIED_PIXMAP) )
	    {
	    for ( i = 0 ; i < w->picture.piMark ; i++ )
		{
		XCopyArea (XtDisplay(w), w->picture.pixmap, XtWindow(w), 
		  w->picture.gc,
		  (int)(w->picture.pXmark[i]) - 1,
		  (int)(w->picture.pYmark[i]) - 1, 3, 3, 
		  (int)(w->picture.pXmark[i]) - 1, 
		  (int)(w->picture.pYmark[i]) - 1); 
		}

	    XCopyArea (XtDisplay(w), w->picture.pixmap, XtWindow(w), 
		   w->picture.gc, 
		   w->picture.pcx - w->picture.cursor_size/2, 
		   w->picture.pcy - w->picture.cursor_size/2,
		   w->picture.cursor_size, w->picture.cursor_size, 
		   w->picture.pcx - w->picture.cursor_size/2, 
		   w->picture.pcy - w->picture.cursor_size/2); 
	    }
	else if (w->image.frame_buffer)
	    {
	    for ( i = 0 ; i < w->picture.piMark ; i++ )
		{
		XClearArea (XtDisplay(w), w->picture.overlay_wid,
		  (int)(w->picture.pXmark[i]) - 1,
		  (int)(w->picture.pYmark[i]) - 1, 3, 3, 
		  False); 
		}

	    XClearArea (XtDisplay(w), w->picture.overlay_wid,
		   w->picture.pcx - w->picture.cursor_size/2, 
		   w->picture.pcy - w->picture.cursor_size/2,
		   w->picture.cursor_size, w->picture.cursor_size, 
		   False); 
	    }

	id = move_selected_cursor ( w, (int)w->picture.X, (int)w->picture.Y, 
				w->picture.Z);

	CallCursorCallbacks(w, XmPCR_DRAG, id, w->picture.X, w->picture.Y,
				w->picture.Z);
	/* record prev cursor x,y so we can erase it next time it is moved */
	w->picture.pcx = (int)w->picture.X;
	w->picture.pcy = (int)w->picture.Y;

	if ( w->picture.FirstTimeMotion != True )
	    {
	    draw_cursors( w );
	    }

	w->picture.FirstTimeMotion = False;

	XmDrawBbox (w);    
	project ( w,  (double)w->picture.X, (double)w->picture.Y, 
			(double)w->picture.Z, w->picture.Wtrans, 
			w->picture.WItrans );

	draw_marker ( w );
 
	add2historybuffer(w, x, y);
	} /* if cursor mode */
    if (w->picture.mode == XmNAVIGATE_MODE)
	{
	w->picture.old_x = x;
	w->picture.old_y = y;
	}
} 

/*****************************************************************************/
/*                                                                           */
/* Subroutine: draw_arrow_head				     		     */
/* Effect:     draw the arrow head for the Roam mode arrow		     */
/*                                                                           */
/*****************************************************************************/
static void draw_arrow_head(XmPictureWidget w, int x, int y, 
				int center_x, int center_y)
{
double angle;
double hyp;
int    x1;
int    y1;
double length;
Display *dpy;
Window  dw;
GC      gc;

    if ( (x == center_x) && (y == center_y) ) return;
    dpy = XtDisplay(w);
    if(!w->image.frame_buffer)
	{
	dw = XtWindow(w);
	gc = w->picture.gc;
	}
    else
	{
	dw = w->picture.overlay_wid;
	gc = w->picture.gcovl;
	}

    if ( sqrt( (center_x - x)*(center_x - x) + (center_y - y)*(center_y - y) ) 
	< 7) 
	{
	length = 4;
	}
    else
	{
	length = 7;
	}
    /*
     * Normalize things.
     */
    x = x - center_x;
    y = y - center_y;

    hyp = sqrt(x*x + y*y);
    angle = acos((double)(x/hyp));
    if (y < 0) angle = 2*M_PI - angle;

    angle = angle + M_PI/6;
    if (angle > 2*M_PI) angle = angle - 2*M_PI;
    x1 = cos(angle)*length;
    y1 = sin(angle)*length;
    XDrawLine(dpy, dw, gc,
		center_x, center_y, center_x + x1, center_y + y1);

    angle = angle - M_PI/3;
    if (angle < 0) angle = angle + 2*M_PI;
    x1 = cos(angle)*length;
    y1 = sin(angle)*length;
    XDrawLine(dpy, dw, gc,
		center_x, center_y, center_x + x1, center_y + y1);

}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: draw_rotated_gnomon			     		     */
/* Effect:     draw the gnomon based on the current rotation of the globe    */
/*                                                                           */
/*****************************************************************************/
static void draw_rotated_gnomon(XmPictureWidget w)
{
    /*
     * Recalc the xformed screen coords based on the new rotation xform
     */
    setup_gnomon(w, True);
    draw_gnomon (w);    
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: move_selected_cursor			     		     */
/* Effect:     Update the x,y position of any selected cursors to the current*/
/*             x,y.                                                          */
/*                                                                           */
/*****************************************************************************/

int move_selected_cursor ( XmPictureWidget  w, int x, int y, double z )
{
int i;

    if ((w->picture.mode == XmCURSOR_MODE) ||
	(w->picture.mode == XmPICK_MODE))
	{
	for ( i = 0 ; i < w->picture.n_cursors ; i++ )
	    {
	    if ( w->picture.selected[i] == True )
		{
		w->picture.xbuff[i] = x;
		w->picture.ybuff[i] = y;
		w->picture.zbuff[i] = z;
		
		save_cursors_in_canonical_form(w, i);
		return(i);
		}
	    }
	}
    else if (w->picture.mode == XmROAM_MODE)
	{
	if (w->picture.roam_selected)
	    {
	    w->picture.roam_xbuff = x;
	    w->picture.roam_ybuff = y;
	    w->picture.roam_zbuff = z;
		
	    save_cursors_in_canonical_form(w, -1);
	    return(0);
	    }
	}
    return(-1);
}


/************************************************************************
 *
 *  Select
 *
 ************************************************************************/

static void Select (w, event)
XmPictureWidget w;
XEvent *event;

{
int  	x, y;
int  	id;
int     i;
double 	l;
int 	screen;
Pixmap 	p1, p2;
XColor	fc, bc;
double	aspect;
int	trans_x;
int	trans_y;
int	width;
int	height;
XmPictureCallbackStruct cb;
unsigned long  value_mask;
XGCValues values;
char tmp_bits[8];


    for (i = 0; i < 8; i++)
	tmp_bits[i] = 0;

    w->picture.button_pressed = event->xbutton.button;
    if (w->picture.disable_temp)
	{
	w->picture.good_at_select = False;
	if (w->picture.mode == XmROTATION_MODE)
	    {
	    w->picture.ignore_new_camera++;
	    }
	return;
	}
    w->picture.good_at_select = True;
    if ( w->picture.mode == XmZOOM_MODE )
	{
	if ( (event->xbutton.button != Button1) &&
	     (event->xbutton.button != Button3) )
	    {
	    return;
	    }
	/*
	 * Create the GC if it doen not already exist.
	 */
	if(w->picture.rubber_band.gc == NULL)
	    {
	    value_mask = GCForeground;
	    values.foreground = w->picture.white;
	    w->picture.rubber_band.gc = 
		XtGetGC((Widget)w, value_mask, &values);
	    }
	width = w->core.width;
	height = w->core.height;
	w->picture.rubber_band.center_x = width/2;
	w->picture.rubber_band.center_y = height/2;
	w->picture.rubber_band.aspect   = (float)height/(float)width;
	aspect                          = w->picture.rubber_band.aspect;

	/*
	 * Calculate the rectangle coords
	 */
	trans_x = event->xbutton.x - w->picture.rubber_band.center_x;
	trans_y = event->xbutton.y - w->picture.rubber_band.center_y;

	if( ((trans_y >= trans_x*aspect) && (trans_y >= -trans_x*aspect)) ||
	    ((trans_y <= -trans_x*aspect)&& (trans_y <= trans_x*aspect)) )
	    {
	    y = height/2 - abs(trans_y);
	    x = y/aspect;
	    }
	else
	    {
	    x = width/2 - abs(trans_x);
	    y = x*aspect;
	    }
	    
	height = height - 2*y;
	width  = width  - 2*x;

	height = MAX(height, 3);
	width = MAX(width, 3);
	/*
	 * Draw the 2 rectangles.
	 */
	if (!w->image.frame_buffer)
	    {
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x+1, y+1, width-2, height-2);
	    }
	else
	    {
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x+1, y+1, width-2, height-2);
	    }

	w->picture.rubber_band.old_x      = x;
	w->picture.rubber_band.old_y      = y;
	w->picture.rubber_band.old_width  = width;
	w->picture.rubber_band.old_height = height;
	}
    if ( w->picture.mode == XmPANZOOM_MODE )
	{
	if ( (event->xbutton.button != Button1) &&
	     (event->xbutton.button != Button3) )
	    {
	    return;
	    }
	/*
	 * Create the GC if it doen not already exist.
	 */
	if(w->picture.rubber_band.gc == NULL)
	    {
	    value_mask = GCForeground;
	    values.foreground = w->picture.white;
	    w->picture.rubber_band.gc = 
		XtGetGC((Widget)w, value_mask, &values);
	    }
	width = w->core.width;
	height = w->core.height;
	w->picture.rubber_band.center_x = event->xbutton.x;
	w->picture.rubber_band.center_y = event->xbutton.y;
	x = event->xbutton.x;
	y = event->xbutton.y;
	w->picture.rubber_band.aspect   = (float)height/(float)width;
	aspect                          = w->picture.rubber_band.aspect;

	height = 3;
	width  = 3;

	/*
	 * Draw the 2 rectangles.
	 */
	if (!w->image.frame_buffer)
	    {
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.rubber_band.gc,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), XtWindow(w), 
		w->picture.rubber_band.gc, x+1, y+1, width-2, height-2);
	    }
	else
	    {
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.white );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x, y, width, height);
	    XSetForeground(XtDisplay(w), w->picture.gcovl,
			w->picture.black );
	    XDrawRectangle(XtDisplay(w), w->picture.overlay_wid,
		w->picture.gcovl, x+1, y+1, width-2, height-2);
	    }

	w->picture.rubber_band.old_x      = x;
	w->picture.rubber_band.old_y      = y;
	w->picture.rubber_band.old_width  = width;
	w->picture.rubber_band.old_height = height;
	}
    if ( ((w->picture.CursorBlank == False) &&
	 ((w->picture.mode == XmCURSOR_MODE) ||
	  (w->picture.mode == XmROAM_MODE)) &&
	  (w->picture.button_pressed == Button1)) )
	{
	if (w->picture.cursor == None)
	    {
	    screen = XScreenNumberOfScreen(XtScreen(w));
	    p1 = XCreatePixmapFromBitmapData ( XtDisplay(w), 
				 RootWindow(XtDisplay(w),screen), 
				 tmp_bits, 4, 4, 0, 0, 1);

	    p2 = XCreatePixmapFromBitmapData ( XtDisplay(w), 
				 RootWindow(XtDisplay(w),screen), 
				 tmp_bits, 4, 4, 0, 0, 1);

	    w->picture.cursor = 
		    XCreatePixmapCursor ( XtDisplay(w), p1, p2, 
					  &fc, &bc, 1, 1 );
	    XFreePixmap ( XtDisplay(w), p1 );
	    XFreePixmap ( XtDisplay(w), p2 );
	    }

	XDefineCursor( XtDisplay(w), XtWindow(w), w->picture.cursor);
	w->picture.CursorBlank = True;
	}
    /* Double click processing */

    /*
     * If the tid is not NULL, that means we have a pending "move"
     * callback that needs to be removed (so we only report a delete).
     */
    if (w->picture.tid != (XtIntervalId)NULL)
	{
	XtRemoveTimeOut(w->picture.tid);
	w->picture.tid = (XtIntervalId)NULL;
	}
    if ( (event->xbutton.time - w->picture.last_time) < 350)
	{
	if ( ((w->picture.mode == XmCURSOR_MODE) ||
	     (w->picture.mode == XmROAM_MODE)) &&
	     (w->picture.button_pressed == Button1) )
	    {
	    w->picture.double_click = True;
	    CreateDelete(w, event);
	    }
	if ( (w->picture.mode == XmROTATION_MODE) ||
	     (w->picture.mode == XmNAVIGATE_MODE) )
	     
	    {
	    w->picture.ignore_new_camera++;
	    keyboard_grab(w, True);
	    }
	}
    else
	/* Single click processing */
	{
	w->picture.last_time = event->xbutton.time;

	if (w->picture.mode == XmPICK_MODE)
	    {
	    if (event->xbutton.button != Button1)
		return;
	    CreateDelete(w, event);
	    }
	if ( (w->picture.mode == XmROTATION_MODE) ||
	      (((w->picture.mode == XmCURSOR_MODE) ||
	       (w->picture.mode == XmROAM_MODE)) &&
		(w->picture.button_pressed != Button1)))
	    {
	    w->picture.K = 0;

	    x = event->xbutton.x ;
	    y = event->xbutton.y ;

	    /* 
	     * Normalize the x,y coordinates
	     */
	    x = x - w->core.width/2;
	    y = y - w->core.height/2;

	    /*
	     * record the angles
	     */
	    l = sqrt(x*x + y*y);
	    if (event->xbutton.button == Button3)
		{
		w->picture.globe->on_sphere = False;
		w->picture.globe->paz = acos((double)x/l);
		if (y > 0)
		    {
		    w->picture.globe->paz = 2*M_PI - w->picture.globe->paz;
		    }
		}
	    else if ( (event->xbutton.button == Button1) ||
		      (event->xbutton.button == Button2) )
		{
		w->picture.globe->on_sphere = True;
		w->picture.globe->paz = -1;
		}
	    else
		{
		return;
		}
	    w->picture.ignore_new_camera++;

	    /*
	     * Adjust the globe circumference to be based on the window size.
	     */
	    l = MAX(w->core.width, w->core.height)/4;
	    w->picture.globe->circumference = M_PI*(l + l);

	    /*
	     * So we only see the globe and gnomon while rotating(i.e. not
	     * the boubding box and cursors).
	     */
	    erase_image(w);
	    draw_gnomon(w);
	    XmDrawGlobe (w);

	    /*
	     * Added to support button up/down approximation
	     */
	    cb.event = event;
	    cb.reason = XmPCR_SELECT;
	    XtCallCallbacks ((Widget) w, XmNrotationCallback, &cb);
	    }	

	if ( ((w->picture.mode == XmCURSOR_MODE) ||
	      (w->picture.mode == XmROAM_MODE)) &&
	      (w->picture.button_pressed == Button1) )
	    {
	    w->picture.ignore_new_camera++;
	    w->picture.FirstTimeMotion = True;
	    w->picture.FirstTime = True;
	    /*
	     * Indicate that there are no elements in the history buffer
	     */
	    w->picture.K = 0;

	    x = event->xbutton.x ;
	    y = event->xbutton.y ;

	    id = find_closest ( w, x, y );

	    /*
	     * If we have not selected a cursor, do nothing.
	     */
	    if ( id == -1 ) 
		{
		deselect_cursor ( w );
		return;
		}
	    else
		{
		if (w->picture.mode == XmCURSOR_MODE)
		    {
		    w->picture.Z = w->picture.zbuff[id];
		    w->picture.pcx = x = w->picture.X = w->picture.xbuff[id];
		    w->picture.pcy = y = w->picture.Y = w->picture.ybuff[id];
		    }
		else if (w->picture.mode == XmROAM_MODE)
		    {
		    w->picture.Z = w->picture.roam_zbuff;
		    w->picture.pcx = x = w->picture.X = w->picture.roam_xbuff;
		    w->picture.pcy = y = w->picture.Y = w->picture.roam_ybuff;
		    }
		deselect_cursor ( w );
		select_cursor ( w, id );
		}
	    draw_cursors( w ); 
	    project ( w, (double)x, (double)y, 
			(double)w->picture.Z, w->picture.Wtrans, 
			w->picture.WItrans );
	    draw_marker (w);


	    CallCursorCallbacks(w, XmPCR_SELECT, id, w->picture.X, w->picture.Y,
				w->picture.Z);
	    } /* if cursor mode */
	if (w->picture.mode == XmNAVIGATE_MODE)
	    {
	    if (event->xbutton.button == Button1)
		{
		w->picture.navigate_direction = XmFORWARD;
		}
	    else if (event->xbutton.button == Button3)
		{
		w->picture.navigate_direction = XmBACKWARD;
		}
	    keyboard_grab(w, True);

	    w->picture.old_x = event->xbutton.x;
	    w->picture.old_y = event->xbutton.y;
	    w->picture.first_step = True;
	    w->picture.ignore_new_camera = 0;

	    /*
	     * Save the current camera in case the user wants to undo this
	     * interaction.
	     */
	    push_undo_camera(w);

	    CallNavigateCallbacks(w, event->xbutton.x, event->xbutton.y, 
				  XmPCR_SELECT);
	    }
	}
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: draw_cursors						     */
/* Effect:     Draws Non-selected cursors???                                 */
/*                                                                           */
/*****************************************************************************/
static int
draw_cursors( XmPictureWidget  w )
{
int  i;
int depth=0;
Window  dw;
GC      gc;

    /*dpy = XtDisplay(w);*/
    if(!w->image.frame_buffer)
	{
	dw = XtWindow(w);
	gc = w->picture.gc;
	}
    else
	{
	dw = w->picture.overlay_wid;
	gc = w->picture.gcovl;
	}

    /* For each potential cursor... */
    if ((w->picture.mode == XmCURSOR_MODE) ||
	(w->picture.mode == XmPICK_MODE))
	{
	for (i = 0; i < w->picture.n_cursors; i++)
	    {
	    switch ( w->picture.cursor_shape )
		{
		case XmSQUARE :
		    if(w->picture.mode == XmCURSOR_MODE)
			depth = (Z_LEVELS-1)*
				    (w->picture.zbuff[i] - w->picture.Zmin)/
				    (w->picture.Zmax - w->picture.Zmin);
		    else if(w->picture.mode == XmPICK_MODE)
			depth = (Z_LEVELS-1);
		    if( w->picture.selected[i] == True )
			{
			XCopyArea (XtDisplay(w),
				w->picture.ActiveSquareCursor[depth], 
				dw,
				gc,
				0,
				0,
				w->picture.cursor_size,
				w->picture.cursor_size,
				w->picture.xbuff[i] -
				(w->picture.cursor_size / 2),
				w->picture.ybuff[i] - 
				(w->picture.cursor_size / 2) ); 
			}
		    else
			{
			XCopyArea (XtDisplay(w),
				w->picture.PassiveSquareCursor[depth], 
				dw,
				gc,
				0,
				0, 
				w->picture.cursor_size,
				w->picture.cursor_size,
				w->picture.xbuff[i] -
				(w->picture.cursor_size / 2),
				w->picture.ybuff[i] - 
				(w->picture.cursor_size / 2) ); 
			}
		    break;
		}
	    }
	}
    else if (w->picture.mode == XmROAM_MODE)
	{
	switch ( w->picture.cursor_shape )
	    {
	    case XmSQUARE :
		depth = (Z_LEVELS-1)*
				(w->picture.roam_zbuff - w->picture.Zmin)/
				(w->picture.Zmax - w->picture.Zmin);
		if(w->picture.roam_selected)
		    {
		    XCopyArea (XtDisplay(w),
				w->picture.ActiveSquareCursor[depth], 
				dw,
				gc,
				0,
				0,
				w->picture.cursor_size,
				w->picture.cursor_size,
				w->picture.roam_xbuff -
				(w->picture.cursor_size / 2),
				w->picture.roam_ybuff - 
				(w->picture.cursor_size / 2) ); 
			}
		else
		    {
		    XCopyArea (XtDisplay(w),
				w->picture.PassiveSquareCursor[depth], 
				dw,
				gc,
				0,
				0, 
				w->picture.cursor_size,
				w->picture.cursor_size,
				w->picture.roam_xbuff -
				(w->picture.cursor_size / 2),
				w->picture.roam_ybuff - 
				(w->picture.cursor_size / 2) ); 
		    }
		break;
	    }
	}
    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: find_closest						     */
/* Effect:     Returns the 1st selected cursor w/in 4 pixels of x,y          */
/*                                                                           */
/*****************************************************************************/
static int 
find_closest ( XmPictureWidget  w, int x, int y )
{
int i;

    if ((w->picture.mode == XmCURSOR_MODE) ||
	(w->picture.mode == XmPICK_MODE) )
	{
	for ( i = 0 ; i < w->picture.n_cursors ; i++ )
	    {
	    if ( (abs (w->picture.xbuff[i] - x) < w->picture.cursor_size/2+1) &&
		 (abs (w->picture.ybuff[i] - y) < w->picture.cursor_size/2+1) )
	    return i;
	    }
	}
    else if (w->picture.mode == XmROAM_MODE)
	{
	if ( (abs (w->picture.roam_xbuff - x) < w->picture.cursor_size/2+1) &&
	     (abs (w->picture.roam_ybuff - y) < w->picture.cursor_size/2+1) )
	return 0;
	}

    return -1;
} 



/************************************************************************
 *
 *  create_cursor_pixmaps
 *
 ************************************************************************/
static void create_cursor_pixmaps(XmPictureWidget  new)
{
int i;
int screen;
GC  gc;
Window dw;
XColor colorcell_def;
XColor selected_out_cell;
XColor unselected_out_cell;
Pixel unselected_out;
Pixel selected_out;
float level;
XWindowAttributes win_att;
Colormap cm;
int depth;

    if (new->picture.ActiveSquareCursor[0] != None) return;

    if(!new->image.frame_buffer)
	{
	dw = XtWindow(new);
	gc = new->picture.gc;
	depth = new->core.depth;
	}
    else
	{
	dw = new->picture.overlay_wid;
	gc = new->picture.gcovl;
	depth = 8;
	}

    colorcell_def.flags = DoRed | DoGreen | DoBlue;

    /*
     * Set the RGB's of the selected outside cursor color
     */
    selected_out_cell   = new->picture.selected_out_cursor_color;
    unselected_out_cell = new->picture.unselected_out_cursor_color;

    /*
     * Set the colormap
     */
    screen = XScreenNumberOfScreen(XtScreen(new));

    XGetWindowAttributes(XtDisplay(new), dw, &win_att);
    if((win_att.colormap != None) && (!new->image.frame_buffer))
	cm = win_att.colormap;
    else
	cm = DefaultColormap(XtDisplay(new), screen);

    for (i = 0; i < Z_LEVELS; i++)
	{
	/*
	 * Note: we will depth cue only the outside color of the cursor
	 */

	/*
	 * First calc the selected outside color
	 */
	level = (float)(Z_LEVELS/2) + (float)(i)/2;
	colorcell_def.red = level * (float)(selected_out_cell.red/Z_LEVELS);
	colorcell_def.green = level * (float)(selected_out_cell.green/Z_LEVELS);
	colorcell_def.blue = level * (float)(selected_out_cell.blue/Z_LEVELS);
	gamma_correct(&colorcell_def);
	if (!XAllocColor(XtDisplay(new), cm, &colorcell_def))
	    {
	    find_color((Widget)new, &colorcell_def);
	    } 
	selected_out = colorcell_def.pixel;

	/*
	 * Then calc the unselected outside color
	 */
	colorcell_def.red = level*(float)(unselected_out_cell.red/Z_LEVELS);
	colorcell_def.green = level*(float)(unselected_out_cell.green/Z_LEVELS);
	colorcell_def.blue = level*(float)(unselected_out_cell.blue/Z_LEVELS);
	gamma_correct(&colorcell_def);
	if(!XAllocColor(XtDisplay(new), cm, &colorcell_def))
	    {
	    find_color((Widget)new, &colorcell_def);
	    } 
	unselected_out = colorcell_def.pixel;

	/*
	 * Create the Pixmaps
	 */
	new->picture.ActiveSquareCursor[i] = 
		XCreatePixmap ( XtDisplay(new),
				dw,
				new->picture.cursor_size,
				new->picture.cursor_size, depth );

	new->picture.PassiveSquareCursor[i] = 
		XCreatePixmap ( XtDisplay(new),
				dw, 
				new->picture.cursor_size,
				new->picture.cursor_size, depth );

	/*
	 * Fill in active and passive cursor pixmaps w/ their 2 colors
	 */
	XSetForeground(XtDisplay(new), gc, selected_out);

	XFillRectangle(XtDisplay(new), new->picture.ActiveSquareCursor[i], gc,
		0, 0, new->picture.cursor_size, new->picture.cursor_size);

	XSetForeground(XtDisplay(new), gc, new->picture.black);

	XFillRectangle(XtDisplay(new), new->picture.ActiveSquareCursor[i], gc,
		1, 1, 
		new->picture.cursor_size - 2, new->picture.cursor_size - 2);

	XSetForeground(XtDisplay(new), gc, unselected_out);

	XFillRectangle(XtDisplay(new), new->picture.PassiveSquareCursor[i], gc,
		0, 0, new->picture.cursor_size, new->picture.cursor_size);

	XSetForeground(XtDisplay(new), gc, new->picture.black);

	XFillRectangle(XtDisplay(new), new->picture.PassiveSquareCursor[i], gc,
		1, 1, 
		new->picture.cursor_size - 2, new->picture.cursor_size - 2);

	}


    new->picture.marker = 
		XCreatePixmap ( XtDisplay(new),
				dw, 
				3, 3, depth );

    XSetForeground(XtDisplay(new), gc, new->picture.white);
    XFillRectangle(XtDisplay(new), new->picture.marker, gc, 0, 0, 3, 3);

}
/************************************************************************
 *
 *  PropertyNotify
 *
 ************************************************************************/

static void PropertyNotifyAR (w, event)
XmPictureWidget w;
XEvent *event;

{
XmPictureCallbackStruct    cb;

    cb.event = event;
    XtCallCallbacks ((Widget) w, XmNpropertyNotifyCallback, &cb);

}
/************************************************************************
 *
 *  ServerMessage
 *
 ************************************************************************/

static void ServerMessage (w, event)
XmPictureWidget w;
XEvent *event;

{
XmPictureCallbackStruct    cb;

    cb.event = event;
    XtCallCallbacks ((Widget) w, XmNclientMessageCallback, &cb);

}
/************************************************************************
 *
 *  Deselect
 *
 ************************************************************************/

static void Deselect (w, event)
XmPictureWidget w;
XEvent *event;
{

XmPictureCallbackStruct    cb;
int     i;
double  to_x, to_y, to_z;
double  dir_x, dir_y, dir_z;
int	width;
double  dist;
double  new_width;


    if (w->picture.button_pressed == 0) return;
    w->picture.button_pressed = 0;
    if ( w->picture.mode == XmPICK_MODE )
	{
	if (event->xbutton.button == Button1)
	    w->picture.ignore_new_camera--;
	return;
	}
    if ( w->picture.mode == XmZOOM_MODE )
	{
	if (w->picture.rubber_band.gc == NULL) return;
	if ( (w->picture.disable_temp) || (!w->picture.good_at_select) ) 
	    {
	    return;
	    }

	if ( (event->xbutton.button != Button1) &&
	     (event->xbutton.button != Button3) )
	    {
	    return;
	    }

	/*
	 * Save the current camera in case the user wants to undo this
	 * interaction.
	 */
	push_undo_camera(w);
	restore_rectangle(w);
	cb.event = event;
	width = w->core.width;
	
	/*
	 * Orthographic projection
	 */
	cb.zoom_factor = 
	   ((double)width-2*(double)w->picture.rubber_band.old_x)/(double)width;
	if (event->xbutton.button == Button3)
	    {
	    cb.zoom_factor = 1/cb.zoom_factor;
	    }

	/*
	 * If we are in a perspective projection, calc a new view angle
	 */
	if (w->picture.projection == 1)
	    {
	    dist = (w->picture.autocamera_width/2)/
		    (tan((w->picture.view_angle/2)*2*M_PI/360.0));
	    new_width = (w->picture.autocamera_width/2)*cb.zoom_factor;
	    cb.view_angle = (360/M_PI)*atan(new_width/dist);
	    cb.view_angle = (cb.view_angle < 0.001) ? 0.001 : cb.view_angle;
	    cb.view_angle = (cb.view_angle > 179.999) ? 179.999 : cb.view_angle;
	    }
	cb.projection = w->picture.projection;
	cb.x = w->picture.to_x;
	cb.y = w->picture.to_y;
	cb.z = w->picture.to_z;
	cb.from_x = w->picture.from_x;
	cb.from_y = w->picture.from_y;
	cb.from_z = w->picture.from_z;

	XtCallCallbacks ((Widget) w, XmNzoomCallback, &cb);
	w->picture.disable_temp = True;
	if (w->picture.double_click)
	    {
	    w->picture.double_click = False;
	    }
	}
    if ( w->picture.mode == XmPANZOOM_MODE )
	{
	if (w->picture.rubber_band.gc == NULL) return;
	if ( (w->picture.disable_temp) || (!w->picture.good_at_select) ) 
	    {
	    return;
	    }

	if ( (event->xbutton.button != Button1) &&
	     (event->xbutton.button != Button3) )
	    {
	    return;
	    }

	/*
	 * Save the current camera in case the user wants to undo this
	 * interaction.
	 */
	push_undo_camera(w);
	restore_rectangle(w);
	cb.event = event;
	width = w->core.width;
	
	/*
	 * Calculate the new camera "to" point
	 */
	dir_x = w->picture.from_x - w->picture.to_x;
	dir_y = w->picture.from_y - w->picture.to_y;
	dir_z = w->picture.from_z - w->picture.to_z;
	xform_coords(w->picture.PureWtrans, 
		w->picture.to_x, w->picture.to_y, w->picture.to_z,
		&to_x, &to_y, &to_z);
	perspective_divide(w, to_x, to_y, to_z, &to_x, &to_y);
	to_x = w->picture.rubber_band.center_x;
	to_y = w->picture.rubber_band.center_y;
	perspective_divide_inverse(w, to_x, to_y, to_z, &to_x, &to_y);
	xform_coords(w->picture.PureWItrans, to_x, to_y, to_z,
		&cb.x, &cb.y, &cb.z);

	if (w->picture.projection == 1)
	    {
	    cb.from_x = w->picture.from_x;
	    cb.from_y = w->picture.from_y;
	    cb.from_z = w->picture.from_z;
	    }
	else
	    {
	    cb.from_x = cb.x + dir_x;
	    cb.from_y = cb.y + dir_y;
	    cb.from_z = cb.z + dir_z;
	    }
	/*
	 * Don't allow infinite zoom
	 */
	if(w->picture.rubber_band.center_x == w->picture.rubber_band.old_x)
		w->picture.rubber_band.old_x--;

	/*
	 * Orthographic projection
	 */
	cb.zoom_factor = 
		(2*(double)(w->picture.rubber_band.center_x - 
			    w->picture.rubber_band.old_x))/
			    (double)w->core.width;
	if (event->xbutton.button == Button3)
	    {
	    cb.zoom_factor = 1/cb.zoom_factor;
	    }

	/*
	 * If we are in a perspective projection, calc a new view angle
	 */
	if (w->picture.projection == 1)
	    {
	    dist = (w->picture.autocamera_width/2)/
		    (tan((w->picture.view_angle/2)*2*M_PI/360.0));
	    new_width = (w->picture.autocamera_width/2)*cb.zoom_factor;
	    cb.view_angle = (360/M_PI)*atan(new_width/dist);
	    cb.view_angle = (cb.view_angle < 0.001) ? 0.001 : cb.view_angle;
	    cb.view_angle = (cb.view_angle > 179.999) ? 179.999 : cb.view_angle;
	    }
	cb.projection = w->picture.projection;

	XtCallCallbacks ((Widget) w, XmNzoomCallback, &cb);
	w->picture.disable_temp = True;
	if (w->picture.double_click)
	    {
	    w->picture.double_click = False;
	    }
	}

    if ( (w->picture.mode == XmROTATION_MODE) ||
	 (((w->picture.mode == XmCURSOR_MODE) ||
	  (w->picture.mode == XmROAM_MODE)) &&
	  (event->xbutton.button != Button1)) )
	{
#ifdef Comment
	if ( (event->xbutton.button != Button1) &&
	     (event->xbutton.button != Button3) )
	    {
	    return;
	    }
#endif
	w->picture.ignore_new_camera--;
	if ( (w->picture.disable_temp) || (!w->picture.good_at_select) )
	    {
	    return;
	    }

	/*
	 * Save the current camera in case the user wants to undo this
	 * interaction.
	 */
	push_undo_camera(w);

	XmPictureMoveCamera( w, w->picture.globe->Wtrans,
		 &cb.from_x, &cb.from_y, &cb.from_z,
		 &cb.up_x, &cb.up_y, &cb.up_z);

	cb.event = event;
	cb.reason = XmPCR_MOVE;
	XtCallCallbacks ((Widget) w, XmNrotationCallback, &cb);

	w->picture.disable_temp = True;
	w->picture.K = 0;
	if (w->picture.double_click)
	    {
	    w->picture.double_click = False;
	    }
	}

    if ( ((w->picture.mode == XmCURSOR_MODE) ||
	 (w->picture.mode == XmROAM_MODE)) &&
	 (event->xbutton.button == Button1))
	{
	w->picture.ignore_new_camera--;
	if (!w->image.frame_buffer)
	    {
	    if (w->picture.pixmap == XmUNSPECIFIED_PIXMAP) return;
	    }

	if (w->picture.mode == XmCURSOR_MODE)
	    {
	    for ( i = 0 ; i < w->picture.n_cursors ; i++ )
		{
		if ( w->picture.selected[i] == True )
		    {
		    if ( w->picture.CursorBlank )
			{
			XWarpPointer ( XtDisplay(w), None, XtWindow(w), 
				    0, 0, 0, 0, 
				    w->picture.xbuff[i] , w->picture.ybuff[i] );
			XUndefineCursor(XtDisplay(w), XtWindow(w));
			w->picture.CursorBlank = False;
			}
		    /*
		     * Save the cursor id so the TimeOut routine can use it.
		     */
		    w->picture.cursor_num = i;
		    w->picture.tid = XtAppAddTimeOut(
			XtWidgetToApplicationContext((Widget)w),
			370, (XtTimerCallbackProc)ButtonTimeOut, w);
		    break;
		    }
		}
	    /*
	     * If none of the cursors were selected...
	     */
	    if (i == w->picture.n_cursors)
		{
		w->picture.cursor_num = -1;
		w->picture.tid = XtAppAddTimeOut(
			XtWidgetToApplicationContext((Widget)w),
			370, (XtTimerCallbackProc)ButtonTimeOut, w);
		}
	    }
	else if (w->picture.mode == XmROAM_MODE)
	    {
	    if ( w->picture.CursorBlank ) 
		{
		if (w->picture.roam_selected)
		    {
		    XWarpPointer ( XtDisplay(w), None, XtWindow(w), 
				   0, 0, 0, 0,
				   w->picture.roam_xbuff,w->picture.roam_ybuff);
		    }
		XUndefineCursor(XtDisplay(w), XtWindow(w));
		w->picture.CursorBlank = False;
		}
	    /*
	     * Save the cursor id so the TimeOut routine can use it.
	     */
	    w->picture.cursor_num = 0;
	    if (!w->picture.double_click)
		{
		w->picture.tid = XtAppAddTimeOut(
			XtWidgetToApplicationContext((Widget)w),
			370, (XtTimerCallbackProc)ButtonTimeOut, w);
		}
	    }

	/*x = event->xbutton.x;g*/
	/*y = event->xbutton.y;g*/

	w->picture.FirstTimeMotion = True;
	w->picture.FirstTime = True;
	w->picture.K = 0;

	for ( i = 0 ; i < w->picture.piMark ; i++ )
	    {
	    if ( (!w->image.frame_buffer)  && 
		 (w->picture.pixmap != XmUNSPECIFIED_PIXMAP) )
		{
		XCopyArea (XtDisplay(w), 
			w->picture.pixmap, 
			XtWindow(w), 
			w->picture.gc,
			(int)(w->picture.pXmark[i]) - 1,
			(int)(w->picture.pYmark[i]) - 1, 3, 3,
			(int)(w->picture.pXmark[i]) - 1,
			(int)(w->picture.pYmark[i]) - 1); 
		}
	    else
		{
		XClearArea (XtDisplay(w), 
			w->picture.overlay_wid, 
			(int)(w->picture.pXmark[i]) - 1,
			(int)(w->picture.pYmark[i]) - 1, 3, 3,
			False); 
		}
	    }
	draw_gnomon(w);
	XmDrawBbox (w);    
	draw_cursors( w );
	XtPopdown(w->picture.popup);
	w->picture.popped_up = False;
	if (w->picture.double_click)
	    {
	    w->picture.double_click = False;
	    }
	w->picture.piMark = 0;
	} /* if cursor mode */
    if (w->picture.mode == XmNAVIGATE_MODE)
	{
	keyboard_grab(w, False);
	/*
	 * Set first_set = True so incoming cameras will set the nav_camera
	 */
	w->picture.first_step = True;
	w->picture.ignore_new_camera = 0;
	CallNavigateCallbacks(w, event->xbutton.x, event->xbutton.y, 
			      XmPCR_MOVE);
	}

}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: ButtonTimeOut						     */
/* Effect:     This routine is set up the first time a button release occurs.*/
/*             If the user double clicks, the time out is removed, otherwise,*/
/*             this routine is executed and the application is called back   */
/*             indicating that a move has occured.                           */
/*                                                                           */
/*****************************************************************************/
XtTimerCallbackProc ButtonTimeOut( XmPictureWidget w, XtIntervalId *id)
{
int i;

    if ( (w->picture.disable_temp) || (!w->picture.good_at_select) ) 
	{
	w->picture.tid = (XtIntervalId)NULL;
	return NULL;
	}

    if ( ((w->picture.mode == XmROAM_MODE) &&
	  (w->picture.roam_selected)) ||
	 ((w->picture.mode == XmCURSOR_MODE) &&
	  (w->picture.cursor_num != -1)) )
	{
	CallCursorCallbacks(w, XmPCR_MOVE, w->picture.cursor_num, 
			    w->picture.X, w->picture.Y, w->picture.Z);
	}

    w->picture.tid = (XtIntervalId)NULL;
    if ( w->picture.CursorBlank )
	{
	XUndefineCursor(XtDisplay(w), XtWindow(w));
	if ( (w->picture.mode == XmROAM_MODE) &&
	     (w->picture.roam_selected) )
	    {
	    XWarpPointer ( XtDisplay(w), None, XtWindow(w), 
			   0, 0, 0, 0,
			   w->picture.roam_xbuff, w->picture.roam_ybuff);
	    }
	else if (w->picture.mode == XmCURSOR_MODE)
	    {
	    for (i = 0; i < w->picture.n_cursors; i++)
		{
		if (w->picture.selected[i])
		    {
		    XWarpPointer ( XtDisplay(w), None, XtWindow(w), 
				   0, 0, 0, 0,
				   w->picture.xbuff[i], w->picture.ybuff[i]);
		    break;
		    }
		}
	    }
	w->picture.CursorBlank = False;
	}
    return NULL;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: select_cursor						     */
/* Effect:     Set Select flag for this cursor                               */
/*                                                                           */
/*****************************************************************************/
static int 
select_cursor ( XmPictureWidget    w , int id)
{
    if ((w->picture.mode == XmCURSOR_MODE) ||
	(w->picture.mode == XmPICK_MODE) )
	{
	w->picture.selected[id] = True;
	}
    else
	{
	w->picture.roam_selected = True;
	}

    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: deselect_cursor						     */
/* Effect:     deselect all 32 cursors                                       */
/*                                                                           */
/*****************************************************************************/
static int 
deselect_cursor (XmPictureWidget w)
{
int i;

    if ((w->picture.mode == XmCURSOR_MODE) ||
	(w->picture.mode == XmPICK_MODE) )
	{
	for ( i = 0 ; i < w->picture.n_cursors ; i++ )
	    {
	    w->picture.selected[i] = False;
	    }
	}
    else
	{
	w->picture.roam_selected = False;
	}

    return 0;
} 
	 
/*****************************************************************************/
/*                                                                           */
/* Subroutine: create_cursor						     */
/* Effect:     Realloc all of the required arrays		             */
/*                                                                           */
/*****************************************************************************/
static int 
create_cursor (XmPictureWidget  w)
{
int 	*itmp;
double 	*dtmp;  
int	i;
    
    /*
     * Realloc the appropriate arrays
     */
    itmp = (int *)XtMalloc((w->picture.n_cursors + 1)*sizeof(int));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	itmp[i] = w->picture.xbuff[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.xbuff);
	}
    w->picture.xbuff = itmp;

    itmp = (int *)XtMalloc((w->picture.n_cursors + 1)*sizeof(int));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	itmp[i] = w->picture.ybuff[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.ybuff);
	}
    w->picture.ybuff = itmp;

    dtmp = (double *)XtMalloc((w->picture.n_cursors + 1)*sizeof(double));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	dtmp[i] = w->picture.zbuff[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.zbuff);
	}
    w->picture.zbuff = dtmp;

    dtmp = (double *)XtMalloc((w->picture.n_cursors + 1)*sizeof(double));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	dtmp[i] = w->picture.cxbuff[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.cxbuff);
	}
    w->picture.cxbuff = dtmp;

    dtmp = (double *)XtMalloc((w->picture.n_cursors + 1)*sizeof(double));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	dtmp[i] = w->picture.cybuff[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.cybuff);
	}
    w->picture.cybuff = dtmp;

    dtmp = (double *)XtMalloc((w->picture.n_cursors + 1)*sizeof(double));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	dtmp[i] = w->picture.czbuff[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.czbuff);
	}
    w->picture.czbuff = dtmp;

    itmp = (int *)XtMalloc((w->picture.n_cursors + 1)*sizeof(int));
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	itmp[i] = w->picture.selected[i];
	}
    if (w->picture.n_cursors > 0)
	{
	XtFree((char*)w->picture.selected);
	}
    w->picture.selected = itmp;

    w->picture.selected[w->picture.n_cursors] = False;
    w->picture.n_cursors++;

    return (w->picture.n_cursors - 1);
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: delete_cursor						     */
/* Effect:     make cursor inactive by setting appropriate flags             */
/*                                                                           */
/*****************************************************************************/
static int 
delete_cursor ( XmPictureWidget  w, int id )
{
int	*itmp;
double	*dtmp;
int	i;
int	k;
  
    w->picture.selected[id] = False;

    /*
     * Erase the cursor.
     */
    if (!w->image.frame_buffer)
	{
	if (w->picture.pixmap != XmUNSPECIFIED_PIXMAP) 
	    XCopyArea (XtDisplay(w), w->picture.pixmap, XtWindow(w), w->picture.gc,
			     w->picture.xbuff[id] - w->picture.cursor_size/2,
			     w->picture.ybuff[id] - w->picture.cursor_size/2,
			     w->picture.cursor_size, w->picture.cursor_size,
			     w->picture.xbuff[id] - w->picture.cursor_size/2,
			     w->picture.ybuff[id] - w->picture.cursor_size/2); 
	}
    else
	{
	XClearArea (XtDisplay(w), w->picture.overlay_wid,
			     w->picture.xbuff[id] - w->picture.cursor_size/2,
			     w->picture.ybuff[id] - w->picture.cursor_size/2, 
			     w->picture.cursor_size, w->picture.cursor_size,
			     False); 
	}

    /*
     * Realloc the arrays
     */

    /*
     * xbuff
     */
    itmp = (int *)XtMalloc((w->picture.n_cursors - 1)*sizeof(int));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) itmp[k++] = w->picture.xbuff[i];
	}
    XtFree((char*)w->picture.xbuff);
    w->picture.xbuff = itmp;
    
    /*
     * ybuff
     */
    itmp = (int *)XtMalloc((w->picture.n_cursors - 1)*sizeof(int));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) itmp[k++] = w->picture.ybuff[i];
	}
    XtFree((char*)w->picture.ybuff);
    w->picture.ybuff = itmp;
    
    /*
     * zbuff
     */
    dtmp = (double *)XtMalloc((w->picture.n_cursors - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) dtmp[k++] = w->picture.zbuff[i];
	}
    XtFree((char*)w->picture.zbuff);
    w->picture.zbuff = dtmp;
    
    /*
     * cxbuff
     */
    dtmp = (double *)XtMalloc((w->picture.n_cursors - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) dtmp[k++] = w->picture.cxbuff[i];
	}
    XtFree((char*)w->picture.cxbuff);
    w->picture.cxbuff = dtmp;

    /*
     * cybuff
     */
    dtmp = (double *)XtMalloc((w->picture.n_cursors - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) dtmp[k++] = w->picture.cybuff[i];
	}
    XtFree((char*)w->picture.cybuff);
    w->picture.cybuff = dtmp;

    /*
     * czbuff
     */
    dtmp = (double *)XtMalloc((w->picture.n_cursors - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) dtmp[k++] = w->picture.czbuff[i];
	}
    XtFree((char*)w->picture.czbuff);
    w->picture.czbuff = dtmp;

    /*
     * selected
     */
    itmp = (int *)XtMalloc((w->picture.n_cursors - 1)*sizeof(int));
    k = 0;
    for (i = 0; i < w->picture.n_cursors; i++)
	{
	if (id != i) itmp[k++] = w->picture.selected[i];
	}
    XtFree((char*)w->picture.selected);
    w->picture.selected = itmp;

    w->picture.n_cursors--;
    if (w->picture.pixmap != XmUNSPECIFIED_PIXMAP) {
	draw_cursors( w ); 
	XFlush(XtDisplay(w));
    }
    return 0;
}

/************************************************************************
 *
 *  CreateDelete
 *
 ************************************************************************/

static void CreateDelete (w, event)
XmPictureWidget w;
XEvent *event;

{
int	i;
int	x;
int	y;
int	id;
Boolean new_box;

    x = event->xbutton.x ;
    y = event->xbutton.y ;

    id = find_closest ( w, x, y );
  
    if ( id != -1 ) 
	{
	if ((w->picture.mode == XmROAM_MODE) ||
	    (w->picture.mode == XmPICK_MODE)) 
	    return;

	delete_cursor ( w, id ); 
	CallCursorCallbacks(w, XmPCR_DELETE, id, 0, 0, 0);
	return;
	}
    else
	{
	/*
	 * This should not fail, but check the result just in case...
	 */
	w->picture.Z = line_of_sight ( w, (double)(x), 
					     (double)(y),
					     w->picture.Wtrans,
					     w->picture.WItrans );

	/*
	 * See if we did not hit the box
	 */
	if ( w->picture.Z == -DBL_MAX )
	    {
	    /*
	     * Choose a reasonable Z value
	     */
	    w->picture.Z = w->picture.Zmin +
			   (w->picture.Zmax - w->picture.Zmin)/2;
	    new_box = True;
	    }
	else
	    {
	    new_box = False;
	    }

	/*
	 * Got a good position, so procede...
	 */
	if ((w->picture.mode == XmCURSOR_MODE) ||
	    (w->picture.mode == XmPICK_MODE))
	    {
	    w->picture.X = x;
	    w->picture.Y = y;

	    deselect_cursor (w);
	    id = create_cursor(w);	
	    w->picture.xbuff[id] = x;
	    w->picture.ybuff[id] = y;
	    w->picture.zbuff[id] = w->picture.Z;
	    if(w->picture.mode == XmCURSOR_MODE)
		select_cursor ( w , id );
	    else if(w->picture.mode == XmPICK_MODE)
		deselect_cursor(w);
	    save_cursors_in_canonical_form(w, id);
	    }
	else if (w->picture.mode == XmROAM_MODE)
	    {
	    /*
	     * Erase the roam cursor.
	     */
	    if (!w->image.frame_buffer)
		{
		if (w->picture.pixmap == XmUNSPECIFIED_PIXMAP) return;
		XCopyArea (XtDisplay(w), w->picture.pixmap, XtWindow(w), 
			     w->picture.gc,
			     w->picture.roam_xbuff - w->picture.cursor_size/2,
			     w->picture.roam_ybuff - w->picture.cursor_size/2,
			     w->picture.cursor_size, w->picture.cursor_size,
			     w->picture.roam_xbuff - w->picture.cursor_size/2,
			     w->picture.roam_ybuff - w->picture.cursor_size/2); 
		}
	    else
		{
		XClearArea (XtDisplay(w), w->picture.overlay_wid,
			     w->picture.roam_xbuff - w->picture.cursor_size/2,
			     w->picture.roam_ybuff - w->picture.cursor_size/2, 
			     w->picture.cursor_size, w->picture.cursor_size,
			     False); 
		}

	    w->picture.X = x;
	    w->picture.Y = y;

	    w->picture.roam_xbuff = x;
	    w->picture.roam_ybuff = y;
	    w->picture.roam_zbuff = w->picture.Z;
	    select_cursor ( w , 0 );
	    save_cursors_in_canonical_form(w, -1);
	    }

	if (w->picture.mode == XmROAM_MODE)
	    {
	    w->picture.ignore_new_camera++;
	    CallCursorCallbacks(w, XmPCR_MOVE, id, w->picture.X, w->picture.Y,
				w->picture.Z);
	    }
	else
	    {
	    w->picture.ignore_new_camera++;
	    CallCursorCallbacks(w, XmPCR_CREATE, id, w->picture.X, w->picture.Y,
				w->picture.Z);
	    }

	if ( w->picture.FirstTime != True )
	    {
	    for ( i = 0 ; i < w->picture.piMark ; i++ )
		{
		if (!w->image.frame_buffer)
		    {
		    XCopyArea (XtDisplay(w), w->picture.pixmap, XtWindow(w),
				  w->picture.gc,
				  (int)(w->picture.pXmark[i]) - 1,
				  (int)(w->picture.pYmark[i]) - 1, 3, 3, 
				  (int)(w->picture.pXmark[i]) - 1,
				  (int)(w->picture.pYmark[i]) - 1); 
		    }
		else
		    {
		    XClearArea (XtDisplay(w), w->picture.overlay_wid,
				  (int)(w->picture.pXmark[i]) - 1,
				  (int)(w->picture.pYmark[i]) - 1, 3, 3, 
				  False); 
		    }
		}
	    }

	if (new_box && w->picture.mode == XmCURSOR_MODE) erase_image(w);
	draw_cursors(w);

	if(w->picture.mode == XmPICK_MODE) return;
	/*
	 * reset the pointer history buffer
	 */
	w->picture.K = 0;
	add2historybuffer(w, x, y);

	draw_gnomon(w);
	if (new_box) setup_bounding_box(w);
	XmDrawBbox (w);    

	project ( w, (double)x, (double)y, 
			(double)w->picture.Z, w->picture.Wtrans, 
			w->picture.WItrans );

	draw_marker (w);

	w->picture.FirstTime = False;
	w->picture.FirstTimeMotion = True;
	id = move_selected_cursor ( w, (int)w->picture.X, (int)w->picture.Y, 
				w->picture.Z);

	/* record prev cursor x,y so we can erase it next time it is moved */
	w->picture.pcx = (int)w->picture.X;
	w->picture.pcy = (int)w->picture.Y;
	}
}

/* ------------- The Geometry and Rendering code ----------------------- */

/*****************************************************************************/
/*                                                                           */
/* Subroutine: draw_gnomon        					     */
/* Effect:     Draw the gnomon						     */
/*                                                                           */
/*****************************************************************************/
	
static void draw_gnomon (w)
XmPictureWidget     w;
{
Display *dpy;
Window  dw;
GC      gc;
int	width;
int	height;

    dpy = XtDisplay(w);
    if(!w->image.frame_buffer)
	{
	dw = XtWindow(w);
	gc = w->picture.gc;
	}
    else
	{
	dw = w->picture.overlay_wid;
	gc = w->picture.gcovl;
	}

    if(w->picture.box_grey.pixel == 0)
	convert_color(w, &w->picture.box_grey);
    /*
     * Erase the old image by restoring the background image
     */
    width = w->core.width - w->picture.gnomon_center_x;
    height = w->core.height - w->picture.gnomon_center_y;
    if((!w->image.frame_buffer) && (w->picture.pixmap != XmUNSPECIFIED_PIXMAP))
	{
	XCopyArea (XtDisplay(w), w->picture.pixmap, XtWindow(w), w->picture.gc,
		   w->picture.gnomon_center_x - (width + 20), 
		   w->picture.gnomon_center_y - (height + 20), 
		   2*width + 20, 2*height + 20,  
		   (w->picture.gnomon_center_x - width) - 20,
		   (w->picture.gnomon_center_y - height) - 20); 
	}
    else if (w->image.frame_buffer)
	{
	XClearArea(XtDisplay(w), w->picture.overlay_wid, 
		   (w->picture.gnomon_center_x - width) - 20, 
		   (w->picture.gnomon_center_y - height) - 20, 
		   2*width + 20, 2*height + 20, False);
	}

    /*
     * Draw black projected X axis
     */
    if ( (w->picture.x_movement_allowed) &&
	 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
	   (w->picture.constrain_cursor == XmCONSTRAIN_X) ) )
	{
	XSetForeground( dpy, gc, w->picture.black);

	XDrawLine( dpy, dw, gc,
		   w->picture.gnomon_center_x-1,
		   w->picture.gnomon_center_y-1,
		   w->picture.gnomon_xaxis_x-1,
		   w->picture.gnomon_xaxis_y-1);
	draw_arrow_head(w, w->picture.gnomon_center_x-1,
		       w->picture.gnomon_center_y-1,
		       w->picture.gnomon_xaxis_x-1,
		       w->picture.gnomon_xaxis_y-1);
    
	XDrawString( dpy, dw, gc,
		w->picture.gnomon_xaxis_label_x-1, 
		w->picture.gnomon_xaxis_label_y-1, 
		"X", 1);
        }
    /*
     * Draw white projected X axis
     */
    if ( (w->picture.x_movement_allowed) &&
	 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
	   (w->picture.constrain_cursor == XmCONSTRAIN_X) ) )
	{
	XSetForeground( dpy, gc, w->picture.white);
	}
    else
	{
	XSetForeground( dpy, gc, w->picture.box_grey.pixel);
	}
    XDrawLine( dpy, dw, gc,
		   w->picture.gnomon_center_x,
		   w->picture.gnomon_center_y,
		   w->picture.gnomon_xaxis_x,
		   w->picture.gnomon_xaxis_y);
    draw_arrow_head(w, w->picture.gnomon_center_x,
		       w->picture.gnomon_center_y,
		       w->picture.gnomon_xaxis_x,
		       w->picture.gnomon_xaxis_y);
    
    XDrawString( dpy, dw, gc,
		w->picture.gnomon_xaxis_label_x, 
		w->picture.gnomon_xaxis_label_y, 
		"X", 1);
    /*
     * Draw black projected Y axis
     */
    if ( (w->picture.y_movement_allowed) &&
	 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
	   (w->picture.constrain_cursor == XmCONSTRAIN_Y) ) )
	{
	XSetForeground( dpy, gc, w->picture.black);

	XDrawLine( dpy, dw, gc,
		   w->picture.gnomon_center_x-1,
		   w->picture.gnomon_center_y-1,
		   w->picture.gnomon_yaxis_x-1,
		   w->picture.gnomon_yaxis_y-1);
	draw_arrow_head(w, w->picture.gnomon_center_x-1,
		       w->picture.gnomon_center_y-1,
		       w->picture.gnomon_yaxis_x-1,
		       w->picture.gnomon_yaxis_y-1);
	XDrawString( dpy, dw, gc,
		w->picture.gnomon_yaxis_label_x-1, 
		w->picture.gnomon_yaxis_label_y-1, 
		"Y", 1);
	}
    /*
     * Draw white projected Y axis
     */
    if ( (w->picture.y_movement_allowed) &&
	 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
	   (w->picture.constrain_cursor == XmCONSTRAIN_Y) ) )
	{
	XSetForeground( dpy, gc, w->picture.white);
	}
    else
	{
	XSetForeground( dpy, gc, w->picture.box_grey.pixel);
	}
    XDrawLine( dpy, dw, gc,
		   w->picture.gnomon_center_x,
		   w->picture.gnomon_center_y,
		   w->picture.gnomon_yaxis_x,
		   w->picture.gnomon_yaxis_y);
    draw_arrow_head(w, w->picture.gnomon_center_x,
		       w->picture.gnomon_center_y,
		       w->picture.gnomon_yaxis_x,
		       w->picture.gnomon_yaxis_y);
    XDrawString( dpy, dw, gc,
		w->picture.gnomon_yaxis_label_x, 
		w->picture.gnomon_yaxis_label_y, 
		"Y", 1);
    /*
     * Draw black projected Z axis
     */
    if ( (w->picture.z_movement_allowed) &&
	 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
	   (w->picture.constrain_cursor == XmCONSTRAIN_Z) ) )
	{
	XSetForeground( dpy, gc, w->picture.black);

	XDrawLine( dpy, dw, gc,
		  w->picture.gnomon_center_x-1,
		  w->picture.gnomon_center_y-1,
		  w->picture.gnomon_zaxis_x-1,
		  w->picture.gnomon_zaxis_y-1);

	draw_arrow_head(w, w->picture.gnomon_center_x-1,
			w->picture.gnomon_center_y-1,
			w->picture.gnomon_zaxis_x-1,
			w->picture.gnomon_zaxis_y-1);
	XDrawString( dpy, dw, gc,
		    w->picture.gnomon_zaxis_label_x-1, 
		    w->picture.gnomon_zaxis_label_y-1, 
		    "Z", 1);
        }
    /*
     * Draw white projected Z axis
     */
    if ( (w->picture.z_movement_allowed) &&
	 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
	   (w->picture.constrain_cursor == XmCONSTRAIN_Z) ) )
	{
	XSetForeground( dpy, gc, w->picture.white);
	}
    else
	{
	XSetForeground( dpy, gc, w->picture.box_grey.pixel);
	}
    XDrawLine( dpy, dw, gc,
		   w->picture.gnomon_center_x,
		   w->picture.gnomon_center_y,
		   w->picture.gnomon_zaxis_x,
		   w->picture.gnomon_zaxis_y);

    draw_arrow_head(w, w->picture.gnomon_center_x,
		       w->picture.gnomon_center_y,
		       w->picture.gnomon_zaxis_x,
		       w->picture.gnomon_zaxis_y);
    XDrawString( dpy, dw, gc,
		w->picture.gnomon_zaxis_label_x, 
		w->picture.gnomon_zaxis_label_y, 
		"Z", 1);

    XFlush(XtDisplay(w));
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmDrawBbox        					     */
/* Effect:     Draw the bounding box                                         */
/*                                                                           */
/*****************************************************************************/
	
static void XmDrawBbox (w)
XmPictureWidget     w;
{
Display *dpy;
Window  dw;
GC      gc;
GC      gc_solid;
GC      gc_dash;
int     i;

    dpy = XtDisplay(w);
    if(!w->image.frame_buffer)
	{
	dw = XtWindow(w);
	gc_solid = w->picture.gc;
	gc_dash = w->picture.gc_dash;
	}
    else
	{
	dw = w->picture.overlay_wid;
	gc_solid = w->picture.gcovl;
	gc_dash = w->picture.gcovl_dash;
	}

    if(w->picture.box_grey.pixel == 0)
	convert_color(w, &w->picture.box_grey);

    /*
     * Draw all the dashed lines first, in case they are coincident with a
     * solid line.
     */
    gc = gc_dash;
    for (i = 0; i < 12; i++)
	{
	if (w->picture.box_line[i].greyed) 
	    XSetForeground( dpy, gc, w->picture.box_grey.pixel);
	else
	    XSetForeground( dpy, gc, w->picture.white);
	
	if ( (!w->picture.box_line[i].rejected) && 
	     (w->picture.box_line[i].dotted) )
	XDrawLine ( dpy, dw, gc,
			    w->picture.box_line[i].x1,
			    w->picture.box_line[i].y1,
			    w->picture.box_line[i].x2,
			    w->picture.box_line[i].y2);
	}
    /*
     * Draw all the solid lines last, in case they are coincident with a
     * dashed line.
     */
    gc = gc_solid;
    for (i = 0; i < 12; i++)
	{
	if (w->picture.box_line[i].greyed) 
	    XSetForeground( dpy, gc, w->picture.box_grey.pixel);
	else
	    XSetForeground( dpy, gc, w->picture.white);
	
	if ( (!w->picture.box_line[i].rejected) &&
	     (!w->picture.box_line[i].dotted) )
	XDrawLine ( dpy, dw, gc,
			    w->picture.box_line[i].x1,
			    w->picture.box_line[i].y1,
			    w->picture.box_line[i].x2,
			    w->picture.box_line[i].y2);
	}
    XFlush(XtDisplay(w));
}


/*****************************************************************************/
/*                                                                           */
/* Subroutine: constrain           	 				     */
/* Effect:     force a transformed point to be  inside the bounding box      */
/*                                                                           */
/*****************************************************************************/
static int constrain ( w, s0, s1 ,s2, dx, dy, dz, WItrans, Wtrans, sdx, sdy )
XmPictureWidget  w;
double     *s0;
double     *s1;
double     *s2;
double     dx;
double     dy;
double     dz;
double     WItrans[4][4];
double     Wtrans[4][4];
int	   sdx;
int	   sdy;
{
double	sxcoor;
double	sycoor;
double	szcoor;
double	original_dist;
double	new_dist;
double	normalize;
double 	x, y, z;
double 	sav_x, sav_y, sav_z;
double  old_x, old_y, old_z;

    /*
     * Remember the old positions
     */
    old_x = *s0;
    old_y = *s1;
    old_z = *s2;
    original_dist = sqrt(sdx*sdx + sdy*sdy);

    /* xform point back to cannonocal coords */
    perspective_divide_inverse(w, *s0, *s1, *s2, s0, s1);
    xform_coords( WItrans, *s0, *s1, *s2, &x, &y, &z);

    sav_x = x;
    sav_y = y;
    sav_z = z;

    /*
     * Start out by moving 1/100 of the distance in world coords.
     */
    x += dx*(w->picture.FacetXmax - w->picture.FacetXmin)/100;
    y += dy*(w->picture.FacetYmax - w->picture.FacetYmin)/100;
    z += dz*(w->picture.FacetZmax - w->picture.FacetZmin)/100;

    /*
     * Now, xform back to screen coords to see how far we have moved.
     */
    xform_coords( w->picture.Wtrans, x, y, z, &sxcoor, &sycoor, &szcoor);
    perspective_divide(w, sxcoor, sycoor, szcoor, &sxcoor, &sycoor);

    /*
     * See how far we have moved.
     */
    new_dist = sqrt((sxcoor - w->picture.pcx)*(sxcoor - w->picture.pcx) + 
		    (sycoor - w->picture.pcy)*(sycoor - w->picture.pcy));

    if (new_dist != 0.0)
	{
	normalize = original_dist/new_dist;
	dx = dx * normalize; 
	dy = dy * normalize; 
	dz = dz * normalize; 
	}
    /*
     * Modify the deltas as appropriate. 
     */
    if (w->picture.cursor_speed == XmSLOW_CURSOR)
	{
	dx = dx*(w->picture.FacetXmax - w->picture.FacetXmin)/300; 
	dy = dy*(w->picture.FacetYmax - w->picture.FacetYmin)/300; 
	dz = dz*(w->picture.FacetZmax - w->picture.FacetZmin)/300;
	}
    if (w->picture.cursor_speed == XmMEDIUM_CURSOR)
	{
	dx = dx*(w->picture.FacetXmax - w->picture.FacetXmin)/100; 
	dy = dy*(w->picture.FacetYmax - w->picture.FacetYmin)/100; 
	dz = dz*(w->picture.FacetZmax - w->picture.FacetZmin)/100;
	}
    if (w->picture.cursor_speed == XmFAST_CURSOR)
	{
	dx = dx*(w->picture.FacetXmax - w->picture.FacetXmin)/33; 
	dy = dy*(w->picture.FacetYmax - w->picture.FacetYmin)/33; 
	dz = dz*(w->picture.FacetZmax - w->picture.FacetZmin)/33;
	}

    /*
     * Apply the new deltas.
     */
    x = dx + sav_x;
    y = dy + sav_y;
    z = dz + sav_z;

    /* adjust the point so it lies w/in the bounding box */
    if (x < 0.0) 
	{
	x = 0.0;
	}
    if (y < 0.0) 
	{
	y = 0.0;
	}
    if (z < 0.0) 
	{
	z = 0.0;
	}
    if (x > w->picture.FacetXmax) 
	{
	x = w->picture.FacetXmax;
	}
    if (y > w->picture.FacetYmax) 
	{
	y = w->picture.FacetYmax;
	}
    if (z > w->picture.FacetZmax) 
	{
	z = w->picture.FacetZmax;
	}

    /* xform the point back to image coord system */
    xform_coords( Wtrans, x, y, z, s0, s1, s2);
    perspective_divide(w, *s0, *s1, *s2, s0, s1);

    /*
     * If we have attempted to move out of the widow, throw away the 
     * change by restoring the original position.
     */
    if ( (*s0 < 0.0) || (*s0 > w->core.width  - w->picture.cursor_size) ||
	 (*s1 < 0.0) || (*s1 > w->core.height - w->picture.cursor_size) )
	{
	*s0 = old_x;
	*s1 = old_y;
	*s2 = old_z;
	}
    return 0;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: save_cursors_in_canonical form				     */
/* Effect:     xform the cursor points to world coords and save		     */
/*                                                                           */
/*****************************************************************************/
static void save_cursors_in_canonical_form (XmPictureWidget w, int i)
{
double x, y, z;

    if (i >= 0)
	{
	perspective_divide_inverse(w, (double)w->picture.xbuff[i],
				   (double)w->picture.ybuff[i],
				   w->picture.zbuff[i],
				   &x, &y);
	xform_coords( w->picture.PureWItrans, 
		      x, y, w->picture.zbuff[i], &x, &y, &z);
	w->picture.cxbuff[i] = x;
	w->picture.cybuff[i] = y;
	w->picture.czbuff[i] = z;
	}
    else
	{
	perspective_divide_inverse(w, (double)w->picture.roam_xbuff,
				   (double)w->picture.roam_ybuff,
				   w->picture.roam_zbuff,
				   &x, &y);
	xform_coords( w->picture.PureWItrans, 
		      x, y, w->picture.roam_zbuff, &x, &y, &z);
	w->picture.roam_cxbuff = x;
	w->picture.roam_cybuff = y;
	w->picture.roam_czbuff = z;
	}
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: restore_cursors_from_canonical form			     */
/* Effect:     xform the cursor points from world to screen coords	     */
/*                                                                           */
/*****************************************************************************/
static void restore_cursors_from_canonical_form (XmPictureWidget w)
{
int i;
double x;
double y;
double z;

    /* xform point back from canonical coords */
    for(i = 0; i < w->picture.n_cursors; i++)
	{
	xform_coords( w->picture.PureWtrans, 
		      w->picture.cxbuff[i],
		      w->picture.cybuff[i],
		      w->picture.czbuff[i],
		      &x, &y, &z);
	perspective_divide(w, x, y, z, &x, &y);
	w->picture.xbuff[i] = (int) x;
	w->picture.ybuff[i] = (int) y;
	w->picture.zbuff[i] = z;
	}

    xform_coords( w->picture.PureWtrans, 
		  w->picture.roam_cxbuff,
		  w->picture.roam_cybuff,
		  w->picture.roam_czbuff,
		  &x, &y, &z);
    perspective_divide(w, x, y, z, &x, &y);
    w->picture.roam_xbuff = (int) x;
    w->picture.roam_ybuff = (int) y;
    w->picture.roam_zbuff = z;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: line_of_sight    	 				     */
/* Effect:     when the mouse is clicked on the bounding box, calculate where*/
/*             the intersection is and return z                               */
/*                                                                           */
/*****************************************************************************/
static double line_of_sight ( w, s0, s1, Wtrans, WItrans )
XmPictureWidget   w;
double    s0;
double    s1;
double    Wtrans[4][4];
double    WItrans[4][4];

{
int  i = -1, II;
double x, y, z, lx, ly, lz, minz, maxz, t, xt, yt, zt;
double xmark[8], ymark[8], zmark[8];

    /* as far as possible from the box */
    z = (double)(w->picture.Zmax + 100);
    xform_coords( WItrans, s0, s1, z, &x, &y, &z);
	
    lx = (0.0 * WItrans[0][Xaxis]) + (0.0 * WItrans[1][Xaxis]) +
	 (-1.0 * WItrans[2][Xaxis]) ;

    ly = (0.0 * WItrans[0][Yaxis]) + (0.0 * WItrans[1][Yaxis]) +
	 (-1.0 * WItrans[2][Yaxis]);

    lz = (0.0 * WItrans[0][Zaxis]) + (0.0 * WItrans[1][Zaxis]) +
	 (-1.0 * WItrans[2][Zaxis]);

	/* line of sight equation
	   X = x + t*lx;
	   Y = Y + t*ly;
	   Z = z + t*ly;
	*/
	

    if (ly != 0.0)
	{
	t  = ((w->picture.Face1[0].ycoor - y) / ly) + 0.00001;
	xt = x + ( t * lx );
	yt = y + ( t * ly );
	zt = z + ( t * lz );

	if ( (xt >= 0.0) && (xt <= w->picture.Face1[2].xcoor) &&
	     (zt >= 0.0) && (zt <= w->picture.Face1[2].zcoor) )
	    {
	    i++;
	    xform_coords( Wtrans, xt, yt, zt, &xmark[i], &ymark[i], &zmark[i]);
	    }
	}

	
    if (ly != 0.0)
	{
	t  = ((w->picture.Face2[0].ycoor - y) / ly) + 0.00001;
	xt = x + ( t * lx );
	yt = y + ( t * ly );
	zt = z + ( t * lz );

	if ( (xt >= 0.0) && (xt <= w->picture.Face1[2].xcoor) &&
	     (zt >= 0.0) && (zt <= w->picture.Face1[2].zcoor) )
	    {
	    i++;
	    xform_coords( Wtrans, xt, yt, zt, &xmark[i], &ymark[i], &zmark[i]);
	    }
	}


    if (lx != 0.0)
	{
	t  = ((w->picture.Face2[0].xcoor - x) / lx) + 0.00001;
	xt = x + ( t * lx );
	yt = y + ( t * ly );
	zt = z + ( t * lz );

	if ( (yt >= 0.0) && (yt <= w->picture.Face1[2].ycoor) &&
	     (zt >= 0.0) && (zt <= w->picture.Face1[2].zcoor) )
	    {
	    i++;
	    xform_coords( Wtrans, xt, yt, zt, &xmark[i], &ymark[i], &zmark[i]);
	    }
	}


    if (lx != 0.0)
	{
	t  = ((w->picture.Face2[3].xcoor - x) / lx) + 0.00001;
	xt = x + ( t * lx );
	yt = y + ( t * ly );
	zt = z + ( t * lz );

	if ( (yt >= 0.0) && (yt <= w->picture.Face1[2].ycoor) &&
	     (zt >= 0.0) && (zt <= w->picture.Face1[2].zcoor) )
	    {
	    i++;
	    xform_coords( Wtrans, xt, yt, zt, &xmark[i], &ymark[i], &zmark[i]);
	    }
	}


    if (lz != 0.0)
	{
	t  = ((w->picture.Face1[1].zcoor - z) / lz) + 0.00001;
	xt = x + ( t * lx );
	yt = y + ( t * ly );
	zt = z + ( t * lz );

	if ( (xt >= 0.0) && (xt <= w->picture.Face1[2].xcoor) &&
	     (yt >= 0.0) && (yt <= w->picture.Face1[2].ycoor) )
	    {
	    i++;
	    xform_coords( Wtrans, xt, yt, zt, &xmark[i], &ymark[i], &zmark[i]);
	    }
	}


    if (lz != 0.0)
	{
	t  = ((w->picture.Face1[0].zcoor - z) / lz) + 0.00001;
	xt = x + ( t * lx );
	yt = y + ( t * ly );
	zt = z + ( t * lz );

	if ( (xt >= 0.0) && (xt <= w->picture.Face1[2].xcoor) &&
	     (yt >= 0.0) && (yt <= w->picture.Face1[2].ycoor) )
	   {
	   i++;
	   xform_coords( Wtrans, xt, yt, zt, &xmark[i], &ymark[i], &zmark[i]);
	   }
	}

    II = i;

    maxz = -DBL_MAX;
    minz = DBL_MAX;
    for ( i = 0 ; i <= II ; i++ )
	{	    
	if ( zmark[i] > maxz )
	    {
	    maxz = zmark[i];
	    }
	if ( zmark[i] < minz )
	    {
	    minz = zmark[i];
	    }
	}
    if ( II == -1 )
	return ( -DBL_MAX );

    z = minz + (maxz - minz)/2;
    return (z);	
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: project          	 				     */
/* Effect:     project the 3D cursor on to the 3 visible faces of the        */
/*             bounding box                                                  */
/*                                                                           */
/*****************************************************************************/
int project ( XmPictureWidget w, double s0, double s1 ,double s2, 
	double Wtrans[4][4], double WItrans[4][4] )
{
int  i = -1;
double x, y, z;

    perspective_divide_inverse(w, s0, s1, s2, &s0, &s1);
    xform_coords( WItrans, s0, s1, s2, &x, &y, &z);

    if ( (w->picture.Face1[0].visible == True) && 
	 (w->picture.Face1[1].visible == True) &&
	 (w->picture.Face1[2].visible == True) &&
	 (w->picture.Face1[3].visible == True) )
	{
	i++;
	xform_coords( Wtrans, 
		     x, 
		     w->picture.Face1[0].ycoor, 
		     z, 
		     &w->picture.Xmark[i], 
		     &w->picture.Ymark[i], 
		     &w->picture.Zmark[i]);
	perspective_divide(w, w->picture.Xmark[i], w->picture.Ymark[i],
	    w->picture.Zmark[i], &w->picture.Xmark[i], &w->picture.Ymark[i]);
	}

    if ( (w->picture.Face2[0].visible == True) &&
	 (w->picture.Face2[1].visible == True) &&
	 (w->picture.Face2[2].visible == True) &&
	 (w->picture.Face2[3].visible == True) )
	{
	i++;
	xform_coords( Wtrans, 
		     x, 
		     0, 
		     z, 
		     &w->picture.Xmark[i], 
		     &w->picture.Ymark[i], 
		     &w->picture.Zmark[i]);
	perspective_divide(w, w->picture.Xmark[i], w->picture.Ymark[i],
	    w->picture.Zmark[i], &w->picture.Xmark[i], &w->picture.Ymark[i]);
	}


    if ( (w->picture.Face1[0].visible == True) &&
	 (w->picture.Face1[1].visible == True) &&
	 (w->picture.Face2[1].visible == True) && 
	 (w->picture.Face2[0].visible == True) )
	{
	i++;
	xform_coords( Wtrans, 
		     0, 
		     y, 
		     z, 
		     &w->picture.Xmark[i], 
		     &w->picture.Ymark[i], 
		     &w->picture.Zmark[i]);
	perspective_divide(w, w->picture.Xmark[i], w->picture.Ymark[i],
	    w->picture.Zmark[i], &w->picture.Xmark[i], &w->picture.Ymark[i]);
	}

    if ( (w->picture.Face1[2].visible == True) &&
	 (w->picture.Face1[3].visible == True) &&
	 (w->picture.Face2[2].visible == True) &&
	 (w->picture.Face2[3].visible == True) )
	{
	i++;
	xform_coords( Wtrans, 
		     w->picture.Face2[3].xcoor, 
		     y, 
		     z, 
		     &w->picture.Xmark[i], 
		     &w->picture.Ymark[i], 
		     &w->picture.Zmark[i]);
	perspective_divide(w, w->picture.Xmark[i], w->picture.Ymark[i],
	    w->picture.Zmark[i], &w->picture.Xmark[i], &w->picture.Ymark[i]);
	}

    if ( (w->picture.Face1[1].visible == True) &&
	 (w->picture.Face1[2].visible == True) &&
	 (w->picture.Face2[1].visible == True) &&
	 (w->picture.Face2[2].visible == True) )
	{
	i++;
	xform_coords( Wtrans, 
		     x, 
		     y, 
		     w->picture.Face2[1].zcoor, 
		     &w->picture.Xmark[i], 
		     &w->picture.Ymark[i], 
		     &w->picture.Zmark[i]);
	perspective_divide(w, w->picture.Xmark[i], w->picture.Ymark[i],
	    w->picture.Zmark[i], &w->picture.Xmark[i], &w->picture.Ymark[i]);
	}

    if ( (w->picture.Face1[0].visible == True) &&
	 (w->picture.Face1[3].visible == True) &&
	 (w->picture.Face2[0].visible == True) &&
	 (w->picture.Face2[3].visible == True) )
	{
	i++;
	xform_coords( Wtrans, 
		     x, 
		     y, 
		     0.0, 
		     &w->picture.Xmark[i], 
		     &w->picture.Ymark[i], 
		     &w->picture.Zmark[i]);
	perspective_divide(w, w->picture.Xmark[i], w->picture.Ymark[i],
	    w->picture.Zmark[i], &w->picture.Xmark[i], &w->picture.Ymark[i]);
	}
    w->picture.iMark = i + 1;
    
    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: add2historybuffer      	 				     */
/* Effect:     record the (x,y) position of the cursor in the history buffer */
/*                                                                           */
/*****************************************************************************/
static int add2historybuffer(XmPictureWidget w, int x, int y)
{
    /* Shuffle the points back */
    w->picture.pppx = w->picture.ppx;
    w->picture.pppy = w->picture.ppy;
    
    w->picture.ppx = w->picture.px;
    w->picture.ppy = w->picture.py;

    w->picture.px = x;
    w->picture.py = y;
    if (w->picture.K < 4) w->picture.K++;
    
    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: draw_marker      	 				     */
/* Effect:     Draw the current markers                                      */
/*                                                                           */
/*****************************************************************************/
int draw_marker ( XmPictureWidget w )
{
	
int i;
Display *dpy;
Window  dw;
GC      gc;

    dpy = XtDisplay(w);
    if(!w->image.frame_buffer)
	{
	dw = XtWindow(w);
	gc = w->picture.gc;
	}
    else
	{
	dw = w->picture.overlay_wid;
	gc = w->picture.gcovl;
	}

   for ( i = 0 ; i < w->picture.iMark ; i++ ) 
   {
      w->picture.pXmark[i] = w->picture.Xmark[i];
      w->picture.pYmark[i] = w->picture.Ymark[i];
   }
   w->picture.piMark = w->picture.iMark;



      for ( i = 0 ; i < w->picture.iMark ; i++ )
       XCopyArea (dpy, w->picture.marker,
		  dw, gc, 0, 0, 3, 3,
		  (int)(w->picture.Xmark[i]) - 1, 
		  (int)(w->picture.Ymark[i]) - 1); 
    XFlush(XtDisplay(w));
    
    return 0;
}

/* --------------- Geometry and rendering for globe rotation ---------- */

/*****************************************************************************/
/*                                                                           */
/* Subroutine: generate_globe     	 				     */
/* Effect:     generate x,y,z's for the facet verticies of the globe         */
/*                                                                           */
/*****************************************************************************/
static void
generate_globe ( XmPictureWidget w , double  radi)    
{
int i, j;
struct point arc[ORBNUM];
struct globe   *globe;
int    screen;
int depth;

    radi +=  + w->core.width/15;
    globe = (struct globe *) XtCalloc(1, sizeof(struct globe));

    globe->radi = radi;
    globe->circumference = M_PI*(radi + radi);

    I44 ( globe->Wtrans );
   
    /* make one half of an equator */
    for ( i = 0 ; i < ORBNUM ; i++ )
	{
	arc[i].xcoor = 0.0;
	arc[i].ycoor = globe->radi
	     * cos(0.314 + (i  * (M_PI - 0.624) / (double)(ORBNUM - 1 )) ); 

	arc[i].zcoor = globe->radi
	     * sin(0.314 + (i  * (M_PI - 0.624) / (double)(ORBNUM - 1)) ); 

	}

   /* first generate the equators by rotating around Y axis */
    for ( i = 0 ; i < EQNUM ; i++ )
	{
	set_rot ( globe->Wtrans, M_PI * 2.0 / (double)(EQNUM) , Yaxis ); 
	for ( j = 0 ; j < ORBNUM ; j++ )
	    {
	    globe->equator[i][j].xcoor = (arc[j].xcoor * 
					  globe->Wtrans[0][Xaxis]) +
					 (arc[j].ycoor * 
					  globe->Wtrans[1][Xaxis]) +
					 (arc[j].zcoor * 
					  globe->Wtrans[2][Xaxis]) ;

	    globe->equator[i][j].ycoor = (arc[j].xcoor * 
					  globe->Wtrans[0][Yaxis]) +
					 (arc[j].ycoor * 
					  globe->Wtrans[1][Yaxis]) +
					 (arc[j].zcoor * 
					  globe->Wtrans[2][Yaxis]) ;

	    globe->equator[i][j].zcoor = (arc[j].xcoor * 
					  globe->Wtrans[0][Zaxis]) +
					 (arc[j].ycoor * 
					  globe->Wtrans[1][Zaxis]) +
					 (arc[j].zcoor * 
					  globe->Wtrans[2][Zaxis]) ;
	    }

	}

    if (w->picture.globe) 
	{
	for (i = 0; i < 4; i++)
	    for (j = 0; j < 4; j++)
		globe->Wtrans[i][j] = w->picture.globe->Wtrans[i][j];
	globe->pixmap = w->picture.globe->pixmap;
	/*  free(w->picture.globe);	SHOULD be XtFree AJ  */
	XtFree((char *)w->picture.globe);	
	}
    w->picture.globe = globe;

    w->picture.globe->x = 10;
    w->picture.globe->y = (int)((w->picture.PIXMAPHEIGHT - 
			(2.0 * w->picture.globe->radi)) - 10);
    if (globe->pixmap)
	{
	XFreePixmap(XtDisplay(w), globe->pixmap);
	}
    screen = XScreenNumberOfScreen(XtScreen(w));
    if(!w->image.frame_buffer)
	depth = w->core.depth;
    else
	depth = 8;
    globe->pixmap = XCreatePixmap( XtDisplay(w),
				   RootWindow(XtDisplay(w),screen),
				   (int)w->picture.globe->radi*2,
				   (int)w->picture.globe->radi*2,
				   depth);
}


/*****************************************************************************/
/*                                                                           */
/* Subroutine: XmDrawGlobe	     	 				     */
/* Effect:                                                                   */
/*                                                                           */
/*****************************************************************************/
static void XmDrawGlobe (XmPictureWidget w)
{

int 	i, 
	j, 
	visible_orbit[EQNUM + 1];
double  x1, 
	y1, 
	x2, 
	y2;
XPoint  arc[ORBNUM], 
	orbit[EQNUM  + 1]; 
struct globe *globe;
Display *dpy;
Window  dw;
GC      gc;
Pixel   white;
Pixel   black;

    if(!w->picture.display_globe) return;

    /*
     * Generate the globe on an as-needed basis instead of in the
     * realize method because the XtUnrealizeWidget call made by
     * MainWindow's window placment is causing us to lose track of
     * the 1st globe created.
     */
    if (!w->picture.globe) generate_globe(w, (double)w->picture.globe_radius);
    globe = w->picture.globe;
    dpy = XtDisplay(w);
    if(!w->image.frame_buffer)
	{
	gc = w->picture.gc;
	}
    else
	{
	gc = w->picture.gcovl;
	}

    dw = globe->pixmap;

    white = w->picture.white;
    black = w->picture.black;

    /*
     * Erase the old globe by restoring the background image
     */
    if (!w->image.frame_buffer)
	{
	if (w->picture.pixmap != XmUNSPECIFIED_PIXMAP)
	    {
	    XCopyArea (dpy, 
	       w->picture.pixmap, globe->pixmap, w->picture.gc,
	       (int)(w->picture.globe->x) ,
	       (int)(w->picture.globe->y) ,
	       (int)(2.0 * w->picture.globe->radi),  
	       (int)(2.0 * w->picture.globe->radi),  
	       (int)0, 
	       (int)0 ); 
	    }
	else
	    {
	    XSetForeground( dpy, gc, black);
	    XFillRectangle (dpy, 
			    globe->pixmap,
			    gc,
			    0 ,
			    0 ,
			    (int)(2.0 * w->picture.globe->radi),  
			    (int)(2.0 * w->picture.globe->radi));
	    }
	}
    else if (w->image.frame_buffer)
	{
	XSetForeground(dpy, gc, white + 1);
	XFillRectangle (dpy, 
		    globe->pixmap,
		    gc,
		    (int)0,
		    (int)0,
		    (int)(2.0 * w->picture.globe->radi),  
		    (int)(2.0 * w->picture.globe->radi));
	}
    XSetForeground( dpy, gc, white);

    /* let's first transform the equator and orbits */

    for ( i = 0 ; i < EQNUM ; i++ )
       for ( j = 0 ; j < ORBNUM ; j++ )
	  globe->visible[i][j] = False;

      for ( i = 0 ; i < EQNUM ; i++ )
       for ( j = 0 ; j < ORBNUM ; j++ )
       { 
	globe->tequator[i][j].xcoor = (globe->equator[i][j].xcoor *
				      globe->Wtrans[0][Xaxis]) +
				     (globe->equator[i][j].ycoor *
				      globe->Wtrans[1][Xaxis]) +
				     (globe->equator[i][j].zcoor *
				      globe->Wtrans[2][Xaxis]) ;

	globe->tequator[i][j].ycoor = (globe->equator[i][j].xcoor *
				      globe->Wtrans[0][Yaxis]) +
				     (globe->equator[i][j].ycoor *
				      globe->Wtrans[1][Yaxis]) +
				     (globe->equator[i][j].zcoor *
				      globe->Wtrans[2][Yaxis]) ;

	globe->tequator[i][j].zcoor = (globe->equator[i][j].xcoor *
				      globe->Wtrans[0][Zaxis]) +
				     (globe->equator[i][j].ycoor *
				      globe->Wtrans[1][Zaxis]) +
				     (globe->equator[i][j].zcoor *
				      globe->Wtrans[2][Zaxis]) ;
    }
	   
      for ( i = 0 ; i < EQNUM ; i++ )
       for ( j = 0 ; j < ORBNUM ; j++ )
	if ( (j < (ORBNUM - 1)) && (i < (EQNUM - 1)) )
	  {
	    x2 = globe->tequator[i][j + 1].xcoor - 
		 globe->tequator[i][j].xcoor;

	    y2 = globe->tequator[i][j + 1].ycoor - 
		 globe->tequator[i][j].ycoor;

	    x1 = globe->tequator[i + 1][j].xcoor - 
		 globe->tequator[i][j].xcoor;

	    y1 = globe->tequator[i + 1][j].ycoor - 
		 globe->tequator[i][j].ycoor;

	    /* Cross Product in Z only */
	    /* Negative Z ==> not visible */
	    if ( ((x1 * y2) - (x2 * y1)) <= 0.0 )
	    {
		globe->visible[i][j] = True;
		globe->visible[i + 1][j] = True;
		globe->visible[i][j + 1] = True;
		globe->visible[i + 1][j + 1] = True;
	    }

	}



    /* Repeat where the two edges meet */
    for ( j = 0 ; j  < ORBNUM - 1 ; j++)
    {
	    x2 = globe->tequator[EQNUM - 1][j + 1].xcoor - 
		 globe->tequator[EQNUM - 1][j].xcoor;

	    y2 = globe->tequator[EQNUM - 1][j + 1].ycoor - 
		 globe->tequator[EQNUM - 1][j].ycoor;

	    x1 = globe->tequator[0][j].xcoor - 
		 globe->tequator[EQNUM - 1][j].xcoor;

	    y1 = globe->tequator[0][j].ycoor - 
		 globe->tequator[EQNUM - 1][j].ycoor;

	    if ( ((x1 * y2) - (x2 * y1)) <= 0.0 )
	    {
		globe->visible[EQNUM - 1][j] = True;
		globe->visible[0][j] = True;
		globe->visible[EQNUM - 1][j + 1] = True;
		globe->visible[0][j + 1] = True;
	    }
     }


      for ( i = 0 ; i < EQNUM ; i++ )
      {
       for ( j = 0 ; j < ORBNUM ; j++ )  
	{

	   arc[j].x = (short)(globe->tequator[i][j].xcoor + 
			 w->picture.globe->radi);
	   arc[j].y = (short)(globe->tequator[i][j].ycoor +
			w->picture.globe->radi);


	}



	for (j = 0 ; j < ORBNUM  - 1; j++ ) 
	  if ((globe->visible[i][j] == True) && 
	      (globe->visible[i][j + 1] == True) ) {
	    XSetForeground( dpy, gc, black);
	    XDrawLine ( dpy, dw, gc,  arc[j].x-1, arc[j].y-1,
					     arc[j + 1].x-1 , arc[j + 1].y-1 );
	    XSetForeground( dpy, gc, white);
	    XDrawLine ( dpy, dw, gc,  arc[j].x, arc[j].y,
						 arc[j + 1].x , arc[j + 1].y );
	  }
      }

      for ( i = 0 ; i < ORBNUM ; i++ )
      {
       for ( j = 0 ; j < EQNUM ; j++ )  
	{


	   orbit[j].x = (short)(globe->tequator[j][i].xcoor +
			 w->picture.globe->radi);
	   orbit[j].y = (short)(globe->tequator[j][i].ycoor + 
			 w->picture.globe->radi);


	   if ( j == (EQNUM - 1) )
	   {
		orbit[EQNUM].x = orbit[0].x;
		orbit[EQNUM].y = orbit[0].y;
		visible_orbit[EQNUM] = visible_orbit[0];
	   }
	}

	for (j = 0 ; j < EQNUM - 1 ; j++ ) 
	  if ((globe->visible[j][i] == True) && 
	      (globe->visible[j + 1][i] == True) ) {
	    XSetForeground( dpy, gc, black);
	    XDrawLine ( dpy, dw, gc,  orbit[j].x-1, orbit[j].y-1,
				 orbit[j + 1].x-1 , orbit[j + 1].y-1 );
	    XSetForeground( dpy, gc, white);
	    XDrawLine ( dpy, dw, gc,  orbit[j].x, orbit[j].y,
				 orbit[j + 1].x , orbit[j + 1].y );
	  }
	  if ((globe->visible[EQNUM - 1][i] == True) && 
	      (globe->visible[0][i] == True) ) {
	    XSetForeground( dpy, gc, black);
	    XDrawLine ( dpy, dw, gc,  orbit[0].x-1, orbit[0].y-1,
				 orbit[EQNUM - 1].x-1 , orbit[EQNUM - 1].y-1 );
	    XSetForeground( dpy, gc, white);
	    XDrawLine ( dpy, dw, gc,  orbit[0].x, orbit[0].y,
				 orbit[EQNUM - 1].x , orbit[EQNUM - 1].y );
	  }

    } /* for */
      

    if(!w->image.frame_buffer)
	{
	dw = XtWindow(w);
	}
    else
	{
	dw = w->picture.overlay_wid;
	}

    XCopyArea(XtDisplay(w), globe->pixmap, dw, gc, 0, 0, 
		(int)w->picture.globe->radi*2, (int)w->picture.globe->radi*2,
		w->picture.globe->x, w->picture.globe->y);
    XFlush(dpy);
}

/*  Subroutine:	calc_projected_axis
 *  Purpose:    For Cursor Mode 2 - calculate the angles of the projected 
 * 		x, y, and x axes.  The calculated angles represent the 
 *		angle off the screen X axis that is create while moving 
 *		along the associated axis in the positive direction
 */
static void calc_projected_axis(XmPictureWidget w)
{
double	x1;
double  y1;
double  x2;
double  y2;
double  delta_x;
double  delta_y;
double  hyp_x;
double  hyp_y;
double  hyp_z;
int     i;

    /* X axis */
    x1 = w->picture.Face2[0].sxcoor;
    y1 = w->picture.Face2[0].sycoor;
    x2 = w->picture.Face2[3].sxcoor;
    y2 = w->picture.Face2[3].sycoor;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    hyp_x = sqrt(delta_x*delta_x + delta_y*delta_y);
    if (hyp_x != 0)
	{
	w->picture.angle_posx = acos( delta_x/hyp_x);
	}
    else
	{
	w->picture.angle_posx = -100.0;
	}
    if( delta_y  > 0 )
	{
	w->picture.angle_posx = 2*M_PI - w->picture.angle_posx;
	}
    if (w->picture.angle_posx < M_PI)
	{
	w->picture.angle_negx = w->picture.angle_posx + M_PI;
	}
    else
	{
	w->picture.angle_negx = w->picture.angle_posx - M_PI;
	}

    /* Y axis */
    x1 = w->picture.Face2[0].sxcoor;
    y1 = w->picture.Face2[0].sycoor;
    x2 = w->picture.Face1[0].sxcoor;
    y2 = w->picture.Face1[0].sycoor;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    hyp_y = sqrt(delta_x*delta_x + delta_y*delta_y);
    if (hyp_y != 0)
	{
	w->picture.angle_posy = acos( delta_x/hyp_y);
	}
    else
	{
	w->picture.angle_posy = -100.0;
	}
    if( delta_y  > 0 )
	{
	w->picture.angle_posy = 2*M_PI - w->picture.angle_posy;
	}
    if (w->picture.angle_posy < M_PI)
	{
	w->picture.angle_negy = w->picture.angle_posy + M_PI;
	}
    else
	{
	w->picture.angle_negy = w->picture.angle_posy - M_PI;
	}

    /* Z axis */
    x1 = w->picture.Face2[0].sxcoor;
    y1 = w->picture.Face2[0].sycoor;
    x2 = w->picture.Face2[1].sxcoor;
    y2 = w->picture.Face2[1].sycoor;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    hyp_z = sqrt(delta_x*delta_x + delta_y*delta_y);
    if (hyp_z != 0)
	{
	w->picture.angle_posz = acos( delta_x/hyp_z);
	}
    else
	{
	w->picture.angle_posz = -100.0;
	}
    if( delta_y  > 0 )
	{
	w->picture.angle_posz = 2*M_PI - w->picture.angle_posz;
	}
    if (w->picture.angle_posz < M_PI)
	{
	w->picture.angle_negz = w->picture.angle_posz + M_PI;
	}
    else
	{
	w->picture.angle_negz = w->picture.angle_posz - M_PI;
	}

    /* Check to see if the projected axis have similar angles.  If they are
     * very close, disable the one with the shortest projected length. Also,
     * disable movement along an axis if the length of the projected axis
     * is less than 10 pixels.
     */
    w->picture.x_movement_allowed = True;
    w->picture.y_movement_allowed = True;
    w->picture.z_movement_allowed = True;

    if ( (fabs(w->picture.angle_posx - w->picture.angle_posy) < 0.2) ||
	 (fabs(w->picture.angle_posx - w->picture.angle_negy) < 0.2) )
	{
	if ( hyp_x < hyp_y)
	    {
	    w->picture.x_movement_allowed = False;
	    }
	else 
	    {
	    w->picture.y_movement_allowed = False;
	    }
	}

    if ( (fabs(w->picture.angle_posx - w->picture.angle_posz) < 0.2) ||
	 (fabs(w->picture.angle_posx - w->picture.angle_negz) < 0.2) )
	{
	if ( hyp_x < hyp_z)
	    {
	    w->picture.x_movement_allowed = False;
	    }
	else 
	    {
	    w->picture.z_movement_allowed = False;
	    }
	}

    if ( (fabs(w->picture.angle_posy - w->picture.angle_posz) < 0.2) ||
	 (fabs(w->picture.angle_posy - w->picture.angle_negz) < 0.2) )
	{
	if ( hyp_y < hyp_z)
	    {
	    w->picture.y_movement_allowed = False;
	    }
	else 
	    {
	    w->picture.z_movement_allowed = False;
	    }
	}
    if (hyp_x < 10)
	{
	w->picture.x_movement_allowed = False;
	}
    if (hyp_y < 10)
	{
	w->picture.y_movement_allowed = False;
	}
    if (hyp_z < 10)
	{
	w->picture.z_movement_allowed = False;
	}

    for (i = 0; i < 4; i++)
	{
	if ((i == 0) || (i == 2))
	    {
	    if ( (w->picture.z_movement_allowed) &&
		     ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
		       (w->picture.constrain_cursor == XmCONSTRAIN_Z) ) )
		{
		w->picture.box_line[i].greyed = False;
		w->picture.box_line[i+4].greyed = False;
		}
	    else
		{
		w->picture.box_line[i].greyed = True;
		w->picture.box_line[i+4].greyed = True;
		}
	    }
	else
	    {
	    if ( (w->picture.x_movement_allowed) &&
		     ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
		       (w->picture.constrain_cursor == XmCONSTRAIN_X) ) )
		{
		w->picture.box_line[i].greyed = False;
		w->picture.box_line[i+4].greyed = False;
		}
	    else
		{
		w->picture.box_line[i].greyed = True;
		w->picture.box_line[i+4].greyed = True;
		}
	    }
	if ( (w->picture.y_movement_allowed) &&
		 ( (w->picture.constrain_cursor == XmCONSTRAIN_NONE) ||
		   (w->picture.constrain_cursor == XmCONSTRAIN_Y) ) )
	    w->picture.box_line[i+8].greyed = False;
	else
	    w->picture.box_line[i+8].greyed = True;
	}

}

/*  Subroutine:	setup_bounding_box
 *  Purpose:	Get the bounding box coordinates in canonical form 
 *              Note that Face1 and Face2 are parallel to the XZ plane
 *
 *	The matrix Bw is assumed to be in the following format:
 *
 *		Ux   Uy   Uz   0
 *		Vx   Vy   Vz   0
 *		Wx   Wy   Wz   0
 *		Ox   Oy   Oz   1
 *
 * 	Where U, V and W are the endpoints of the axis of the bounding box
 *	and O is the origin of the bounding box.  To get the bounding box
 *	in canonical form, it must be translated to the origin of the world
 *	coordinate system.  This is done by subtracting Ox, Oy and Oz from
 *	the associated components of the other points.
 */
static void  setup_bounding_box(XmPictureWidget w)
{
double x, y, z;
double vdelta_x, vdelta_y, vdelta_z;
double wdelta_x, wdelta_y, wdelta_z;
double xmax, xmin, ymax, ymin, zmax, zmin;
int    i;
int    j;
double xlen, ylen, zlen, maxlen;
double cam_screen_x, cam_screen_y, cam_screen_z;
double sod_width; 	/* S/D vis. a vis. Newman and Sproull */
double sod_height; 	/* S/D vis. a vis. Newman and Sproull */
double ho2;     /* image_height/2 */
double wo2;     /* image_width/2 */
double zscale;
double new_x1, new_y1, new_z1;
double new_x2, new_y2, new_z2;

    w->picture.Zmax = -DBL_MAX;
    w->picture.Zmin = DBL_MAX;
    xmax = ymax = zmax = -DBL_MAX;
    xmin = ymin = zmin = DBL_MAX;

    xmin = MIN(xmin, w->picture.basis.Bw[0][0]);
    xmin = MIN(xmin, w->picture.basis.Bw[1][0]);
    xmin = MIN(xmin, w->picture.basis.Bw[2][0]);
    xmin = MIN(xmin, w->picture.basis.Bw[3][0]);

    ymin = MIN(ymin, w->picture.basis.Bw[0][1]);
    ymin = MIN(ymin, w->picture.basis.Bw[1][1]);
    ymin = MIN(ymin, w->picture.basis.Bw[2][1]);
    ymin = MIN(ymin, w->picture.basis.Bw[3][1]);

    zmin = MIN(zmin, w->picture.basis.Bw[0][2]);
    zmin = MIN(zmin, w->picture.basis.Bw[1][2]);
    zmin = MIN(zmin, w->picture.basis.Bw[2][2]);
    zmin = MIN(zmin, w->picture.basis.Bw[3][2]);

    xmax = MAX(xmax, w->picture.basis.Bw[0][0]);
    xmax = MAX(xmax, w->picture.basis.Bw[1][0]);
    xmax = MAX(xmax, w->picture.basis.Bw[2][0]);
    xmax = MAX(xmax, w->picture.basis.Bw[3][0]);

    ymax = MAX(ymax, w->picture.basis.Bw[0][1]);
    ymax = MAX(ymax, w->picture.basis.Bw[1][1]);
    ymax = MAX(ymax, w->picture.basis.Bw[2][1]);
    ymax = MAX(ymax, w->picture.basis.Bw[3][1]);

    zmax = MAX(zmax, w->picture.basis.Bw[0][2]);
    zmax = MAX(zmax, w->picture.basis.Bw[1][2]);
    zmax = MAX(zmax, w->picture.basis.Bw[2][2]);
    zmax = MAX(zmax, w->picture.basis.Bw[3][2]);

    /*udelta_x = w->picture.basis.Bw[0][0] - w->picture.basis.Bw[3][0];*/
    /*udelta_y = w->picture.basis.Bw[0][1] - w->picture.basis.Bw[3][1];*/
    /*udelta_z = w->picture.basis.Bw[0][2] - w->picture.basis.Bw[3][2];*/

    vdelta_x = w->picture.basis.Bw[1][0] - w->picture.basis.Bw[3][0];
    vdelta_y = w->picture.basis.Bw[1][1] - w->picture.basis.Bw[3][1];
    vdelta_z = w->picture.basis.Bw[1][2] - w->picture.basis.Bw[3][2];

    wdelta_x = w->picture.basis.Bw[2][0] - w->picture.basis.Bw[3][0];
    wdelta_y = w->picture.basis.Bw[2][1] - w->picture.basis.Bw[3][1];
    wdelta_z = w->picture.basis.Bw[2][2] - w->picture.basis.Bw[3][2];

    /*
     * U + V
     */
    xmin = MIN(xmin, w->picture.basis.Bw[0][0] + vdelta_x);
    xmax = MAX(xmax, w->picture.basis.Bw[0][0] + vdelta_x);
    ymin = MIN(ymin, w->picture.basis.Bw[0][1] + vdelta_y);
    ymax = MAX(ymax, w->picture.basis.Bw[0][1] + vdelta_y);
    zmin = MIN(zmin, w->picture.basis.Bw[0][2] + vdelta_z);
    zmax = MAX(zmax, w->picture.basis.Bw[0][2] + vdelta_z);

    /*
     * U + W
     */
    xmin = MIN(xmin, w->picture.basis.Bw[0][0] + wdelta_x);
    xmax = MAX(xmax, w->picture.basis.Bw[0][0] + wdelta_x);
    ymin = MIN(ymin, w->picture.basis.Bw[0][1] + wdelta_y);
    ymax = MAX(ymax, w->picture.basis.Bw[0][1] + wdelta_y);
    zmin = MIN(zmin, w->picture.basis.Bw[0][2] + wdelta_z);
    zmax = MAX(zmax, w->picture.basis.Bw[0][2] + wdelta_z);

    /*
     * V + W
     */
    xmin = MIN(xmin, w->picture.basis.Bw[1][0] + wdelta_x);
    xmax = MAX(xmax, w->picture.basis.Bw[1][0] + wdelta_x);
    ymin = MIN(ymin, w->picture.basis.Bw[1][1] + wdelta_y);
    ymax = MAX(ymax, w->picture.basis.Bw[1][1] + wdelta_y);
    zmin = MIN(zmin, w->picture.basis.Bw[1][2] + wdelta_z);
    zmax = MAX(zmax, w->picture.basis.Bw[1][2] + wdelta_z);

    /*
     * U + V + W
     */
    xmin = MIN(xmin, w->picture.basis.Bw[0][0] + vdelta_x + wdelta_x);
    xmax = MAX(xmax, w->picture.basis.Bw[0][0] + vdelta_x + wdelta_x);
    ymin = MIN(ymin, w->picture.basis.Bw[0][1] + vdelta_y + wdelta_y);
    ymax = MAX(ymax, w->picture.basis.Bw[0][1] + vdelta_y + wdelta_y);
    zmin = MIN(zmin, w->picture.basis.Bw[0][2] + vdelta_z + wdelta_z);
    zmax = MAX(zmax, w->picture.basis.Bw[0][2] + vdelta_z + wdelta_z);

    /*
     * Grow the bounding box to include the current set of cursors.
     */
    if (w->picture.mode == XmCURSOR_MODE)
	{
	for (i = 0; i < w->picture.n_cursors; i++)
	    {
	    xmin = MIN(xmin, w->picture.cxbuff[i]);
	    xmax = MAX(xmax, w->picture.cxbuff[i]);
	    ymin = MIN(ymin, w->picture.cybuff[i]);
	    ymax = MAX(ymax, w->picture.cybuff[i]);
	    zmin = MIN(zmin, w->picture.czbuff[i]);
	    zmax = MAX(zmax, w->picture.czbuff[i]);
	    }
	}
    else if (w->picture.mode == XmROAM_MODE)
	{
	xmin = MIN(xmin, w->picture.roam_cxbuff);
	xmax = MAX(xmax, w->picture.roam_cxbuff);
	ymin = MIN(ymin, w->picture.roam_cybuff);
	ymax = MAX(ymax, w->picture.roam_cybuff);
	zmin = MIN(zmin, w->picture.roam_czbuff);
	zmax = MAX(zmax, w->picture.roam_czbuff);
	}

    /*
     * Handle 2D images by forcing a minimum value for each dimension
     */
    xlen = xmax - xmin;
    ylen = ymax - ymin;
    zlen = zmax - zmin;
    maxlen = MAX(xlen, ylen);
    maxlen = MAX(maxlen, zlen);
    if(maxlen == 0.0) XtWarning("Degenerate Bounding Box in Picture Widget");

    if (xmin == xmax)
	{
	xmax = xmin + maxlen/2000;
	}
    if (ymin == ymax)
	{
	ymax = ymin + maxlen/2000;
	}
    if (zmin == zmax)
	{
	zmax = zmin + maxlen/2000;
	}

    /*
     * Save the bounding box coords in cannonical form.
     */
    w->picture.FacetXmax = xmax - xmin;
    w->picture.FacetYmax = ymax - ymin;
    w->picture.FacetZmax = zmax - zmin;

    w->picture.FacetXmin =  0.0;
    w->picture.FacetYmin =  0.0;
    w->picture.FacetZmin =  0.0;

    w->picture.Face1[0].xcoor = 0.0;
    w->picture.Face1[0].ycoor = w->picture.FacetYmax;
    w->picture.Face1[0].zcoor = 0.0;

    w->picture.Face1[1].xcoor = 0.0;
    w->picture.Face1[1].ycoor = w->picture.FacetYmax;
    w->picture.Face1[1].zcoor = w->picture.FacetZmax;

    w->picture.Face1[2].xcoor = w->picture.FacetXmax;
    w->picture.Face1[2].ycoor = w->picture.FacetYmax;
    w->picture.Face1[2].zcoor = w->picture.FacetZmax;

    w->picture.Face1[3].xcoor = w->picture.FacetXmax;
    w->picture.Face1[3].ycoor = w->picture.FacetYmax;
    w->picture.Face1[3].zcoor = 0.0;

    w->picture.Face2[0].xcoor = 0.0;
    w->picture.Face2[0].ycoor = 0.0;
    w->picture.Face2[0].zcoor = 0.0;

    w->picture.Face2[1].xcoor = 0.0;
    w->picture.Face2[1].ycoor = 0.0;
    w->picture.Face2[1].zcoor = w->picture.FacetZmax;

    w->picture.Face2[2].xcoor = w->picture.FacetXmax;
    w->picture.Face2[2].ycoor = 0.0;
    w->picture.Face2[2].zcoor = w->picture.FacetZmax;

    w->picture.Face2[3].xcoor = w->picture.FacetXmax;
    w->picture.Face2[3].ycoor = 0.0 ;
    w->picture.Face2[3].zcoor = 0.0;

    /*
     * Save the origin of the bounding box for CallCursorCallbacks.
     */
    w->picture.Ox = xmin;
    w->picture.Oy = ymin;
    w->picture.Oz = zmin;


    /*
     * Build matrix for cannonical form transformations.
     */
    I44(w->picture.Wtrans);

    /* Pre-concatonate the translation to canonical form */
    set_trans(w->picture.Wtrans, xmin, Xaxis);
    set_trans(w->picture.Wtrans, ymin, Yaxis);
    set_trans(w->picture.Wtrans, zmin, Zaxis);

    mult44(w->picture.Wtrans, w->picture.PureWtrans);

    inverse ( w->picture.Wtrans, w->picture.WItrans );

    /* transform the canonical form into the SCREEN XYZ space */
    for ( i = 0 ; i < 4 ; i++ )
	{
	xform_coords( w->picture.Wtrans, 
		      w->picture.Face1[i].xcoor, 
		      w->picture.Face1[i].ycoor, 
		      w->picture.Face1[i].zcoor, 
		      &w->picture.Face1[i].txcoor, 
		      &w->picture.Face1[i].tycoor, 
		      &w->picture.Face1[i].tzcoor);

	perspective_divide( w, 
			    w->picture.Face1[i].txcoor,
			    w->picture.Face1[i].tycoor,
			    w->picture.Face1[i].tzcoor,
			    &w->picture.Face1[i].txcoor,
			    &w->picture.Face1[i].tycoor);

	w->picture.Face1[i].sxcoor = (int)(w->picture.Face1[i].txcoor);
	w->picture.Face1[i].sycoor = (int)(w->picture.Face1[i].tycoor);

	if ( w->picture.Face1[i].tzcoor > w->picture.Zmax )
	    {
	    w->picture.Zmax = w->picture.Face1[i].tzcoor;
	    }
	if ( w->picture.Face1[i].tzcoor < w->picture.Zmin )	
	    {
	    w->picture.Zmin = w->picture.Face1[i].tzcoor;
	    }
	}

    for ( i = 0 ; i < 4 ; i++ )
	{
	xform_coords( w->picture.Wtrans, 
		      w->picture.Face2[i].xcoor, 
		      w->picture.Face2[i].ycoor, 
		      w->picture.Face2[i].zcoor, 
		      &w->picture.Face2[i].txcoor, 
		      &w->picture.Face2[i].tycoor, 
		      &w->picture.Face2[i].tzcoor);

	perspective_divide( w, 
			    w->picture.Face2[i].txcoor,
			    w->picture.Face2[i].tycoor,
			    w->picture.Face2[i].tzcoor,
			    &w->picture.Face2[i].txcoor,
			    &w->picture.Face2[i].tycoor);

	w->picture.Face2[i].sxcoor = (int)(w->picture.Face2[i].txcoor);
	w->picture.Face2[i].sycoor = (int)(w->picture.Face2[i].tycoor);

	if ( w->picture.Face2[i].tzcoor > w->picture.Zmax )
	    {
	    w->picture.Zmax = w->picture.Face2[i].tzcoor;
	    }
	if ( w->picture.Face2[i].tzcoor < w->picture.Zmin )
	    {
	    w->picture.Zmin = w->picture.Face2[i].tzcoor;
	    }
	}

    /* remove the hidden lines of the bounding box */
    /* Shoot a ray from the eye to a vertex. Use a z value that
       is slightly less than the vertex.  Then, xform the point
       back into cannonical form.  If the point is outside the box,
       then the vertex is visible */
    for ( i = 0 ; i < 4 ; i++ )
	{

	/* see which vertex is visible */
	z = (double)(w->picture.Face1[i].tzcoor +
		((w->picture.Zmax - w->picture.Zmin) / 5000.0));
	perspective_divide_inverse( w, 
			    w->picture.Face1[i].txcoor,
			    w->picture.Face1[i].tycoor,
			    z,
			    &x,
			    &y);
	xform_coords( w->picture.WItrans, 
		      x, 
		      y, 
		      z, 
		      &x, &y, &z);

	if ( (x >= w->picture.FacetXmin) && (x <= w->picture.FacetXmax) &&
		 (y >= w->picture.FacetYmin) && (y <= w->picture.FacetYmax) &&
		 (z >= w->picture.FacetZmin) && (z <= w->picture.FacetZmax) )
		       w->picture.Face1[i].visible = False; 
	else 
	    w->picture.Face1[i].visible = True;

	z = (double)(w->picture.Face2[i].tzcoor +
		   ((w->picture.Zmax - w->picture.Zmin) / 5000.0));

	perspective_divide_inverse( w, 
			    w->picture.Face2[i].txcoor,
			    w->picture.Face2[i].tycoor,
			    z,
			    &x,
			    &y);
	xform_coords( w->picture.WItrans, 
		      x, 
		      y, 
		      z, 
		      &x, &y, &z);

	/* the x, y, z are now in the original coordinate system 
			see if they are inside the box */

	if ( (x >= w->picture.FacetXmin) && (x <= w->picture.FacetXmax) &&
		 (y >= w->picture.FacetYmin) && (y <= w->picture.FacetYmax) &&
		 (z >= w->picture.FacetZmin) && (z <= w->picture.FacetZmax) )
		    w->picture.Face2[i].visible = False; 
	else  
	     w->picture.Face2[i].visible = True;
	}

    /* Clipping calcs. */
    /* transform the camera into 3D screen coords */
    xform_coords( w->picture.PureWtrans,
		  w->picture.from_x,
		  w->picture.from_y,
		  w->picture.from_z,
		  &cam_screen_x,
		  &cam_screen_y,
		  &cam_screen_z);
    /* S/D */
    sod_width = tan(M_PI*w->picture.view_angle/360.0);
    ho2 = (double)(w->picture.image_height)/2;
    wo2 = (double)(w->picture.image_width)/2;
    sod_height = sod_width*(ho2/wo2);

    zscale = wo2/(sod_width*cam_screen_z);
    cam_screen_z *= zscale;

    /*
     * Line array setup
     *
     *	Line #	Point1		Point2		Axis
     *	--------------------------------------------
     *	0	Face1[0]	Face1[1]	Z
     *	1	Face1[1]	Face1[2]	X
     *	2	Face1[2]	Face1[3]	Z
     *	3	Face1[3]	Face1[0]	X
     *
     *	4	Face2[0]	Face2[1]	Z
     *	5	Face2[1]	Face2[2]	X
     *	6	Face2[2]	Face2[3]	Z
     *	7	Face2[3]	Face2[0]	X
     *
     *	8	Face1[0]	Face2[0]	Y
     *	9	Face1[1]	Face2[1]	Y
     *	10	Face1[2]	Face2[2]	Y
     *	11	Face1[3]	Face2[3]	Y
     */

    for (i = 0; i < 4; i++)
	{
	xform_coords( w->picture.Wtrans, 
		      w->picture.Face1[i].xcoor, 
		      w->picture.Face1[i].ycoor, 
		      w->picture.Face1[i].zcoor, 
		      &w->picture.Face1[i].txcoor, 
		      &w->picture.Face1[i].tycoor, 
		      &w->picture.Face1[i].tzcoor);
	xform_coords( w->picture.Wtrans, 
		      w->picture.Face2[i].xcoor, 
		      w->picture.Face2[i].ycoor, 
		      w->picture.Face2[i].zcoor, 
		      &w->picture.Face2[i].txcoor, 
		      &w->picture.Face2[i].tycoor, 
		      &w->picture.Face2[i].tzcoor);
	w->picture.Face1[i].tzcoor = w->picture.Face1[i].tzcoor * zscale;
	w->picture.Face2[i].tzcoor = w->picture.Face2[i].tzcoor * zscale;
	}

    /* 3D Screen coordinate system.  The two diagonal lines are the top
     * and bottom of the viewing frustum. zdepth = (z - camera_z) is the
     * distance in z from the camera to the point in question.

		    \ y = image_height/2
		    |\
		    | \
		    |  \ y = -sod_height*(z - camera_z)
		    |   \
		    |S   \
		    |     \
	    Z Axis  |      \
____________________|__D____\ camera z
		    |       /
		    |      /
		    |     /
		    |    /
	    Y Axis  |   /y = sod_height*(z - camera_z)
		    |  /
		    | /
		    |/y = -image_height/2
	*
	*/
	
    for (i = 0; i < 4; i++)
	{
	j = (i+1)%4;

	/*
	 * Face1
	 */
	w->picture.box_line[i].x1   = w->picture.Face1[i].sxcoor;
	w->picture.box_line[i].y1   = w->picture.Face1[i].sycoor;
	w->picture.box_line[i].x2   = w->picture.Face1[j].sxcoor;
	w->picture.box_line[i].y2   = w->picture.Face1[j].sycoor;
	if (w->picture.Face1[i].visible && w->picture.Face1[j].visible)
	    w->picture.box_line[i].dotted = False;
	else
	    w->picture.box_line[i].dotted = True;

	w->picture.box_line[i].rejected = 
		trivial_reject(w,
			       w->picture.Face1[i].txcoor - wo2,
			       w->picture.Face1[i].tycoor - ho2,
			       w->picture.Face1[i].tzcoor - cam_screen_z,
			       w->picture.Face1[j].txcoor - wo2,
			       w->picture.Face1[j].tycoor - ho2,
			       w->picture.Face1[j].tzcoor - cam_screen_z,
			       sod_width, sod_height);
	if (!w->picture.box_line[i].rejected)
	    {
	    if (clip_line(w,
		      w->picture.Face1[i].txcoor - wo2,
		      w->picture.Face1[i].tycoor - ho2,
		      w->picture.Face1[i].tzcoor - cam_screen_z,
		      w->picture.Face1[j].txcoor - wo2,
		      w->picture.Face1[j].tycoor - ho2,
		      w->picture.Face1[j].tzcoor - cam_screen_z,
		      sod_width, sod_height,
		      &new_x1, &new_y1, &new_z1,
		      &new_x2, &new_y2, &new_z2))
		{
		new_x1 += wo2; new_y1 += ho2; new_z1 += cam_screen_z;
		new_x2 += wo2; new_y2 += ho2; new_z2 += cam_screen_z;
		perspective_divide( w, new_x1, new_y1, new_z1/zscale, &x, &y);
		w->picture.box_line[i].x1 = x;
		w->picture.box_line[i].y1 = y;
		perspective_divide( w, new_x2, new_y2, new_z2/zscale, &x, &y);
		w->picture.box_line[i].x2 = x;
		w->picture.box_line[i].y2 = y;
		}
	    }
	/*
	 * Face2
	 */
	w->picture.box_line[i+4].x1 = w->picture.Face2[i].sxcoor;
	w->picture.box_line[i+4].y1 = w->picture.Face2[i].sycoor;
	w->picture.box_line[i+4].x2 = w->picture.Face2[j].sxcoor;
	w->picture.box_line[i+4].y2 = w->picture.Face2[j].sycoor;
	if (w->picture.Face2[i].visible && w->picture.Face2[j].visible)
	    w->picture.box_line[i+4].dotted = False;
	else
	    w->picture.box_line[i+4].dotted = True;

	w->picture.box_line[i+4].rejected = 
		trivial_reject(w,
			       w->picture.Face2[i].txcoor - wo2,
			       w->picture.Face2[i].tycoor - ho2,
			       w->picture.Face2[i].tzcoor - cam_screen_z,
			       w->picture.Face2[j].txcoor - wo2,
			       w->picture.Face2[j].tycoor - ho2,
			       w->picture.Face2[j].tzcoor - cam_screen_z,
			       sod_width, sod_height);

	if (!w->picture.box_line[i+4].rejected)
	    {
	    if (clip_line(w,
		      w->picture.Face2[i].txcoor - wo2,
		      w->picture.Face2[i].tycoor - ho2,
		      w->picture.Face2[i].tzcoor - cam_screen_z,
		      w->picture.Face2[j].txcoor - wo2,
		      w->picture.Face2[j].tycoor - ho2,
		      w->picture.Face2[j].tzcoor - cam_screen_z,
		      sod_width, sod_height,
		      &new_x1, &new_y1, &new_z1,
		      &new_x2, &new_y2, &new_z2))
		{
		new_x1 += wo2; new_y1 += ho2; new_z1 += cam_screen_z;
		new_x2 += wo2; new_y2 += ho2; new_z2 += cam_screen_z;
		perspective_divide( w, new_x1, new_y1, new_z1/zscale, &x, &y);
		w->picture.box_line[i+4].x1 = x;
		w->picture.box_line[i+4].y1 = y;
		perspective_divide( w, new_x2, new_y2, new_z2/zscale, &x, &y);
		w->picture.box_line[i+4].x2 = x;
		w->picture.box_line[i+4].y2 = y;
		}
	    }
	/*
	 * Face1 - Face2
	 */
	w->picture.box_line[i+8].x1 = w->picture.Face1[i].sxcoor;
	w->picture.box_line[i+8].y1 = w->picture.Face1[i].sycoor;
	w->picture.box_line[i+8].x2 = w->picture.Face2[i].sxcoor;
	w->picture.box_line[i+8].y2 = w->picture.Face2[i].sycoor;
	if (w->picture.Face1[i].visible && w->picture.Face2[i].visible)
	    w->picture.box_line[i+8].dotted = False;
	else
	    w->picture.box_line[i+8].dotted = True;

	w->picture.box_line[i+8].rejected =
		trivial_reject(w,
			       w->picture.Face1[i].txcoor - wo2,
			       w->picture.Face1[i].tycoor - ho2,
			       w->picture.Face1[i].tzcoor - cam_screen_z,
			       w->picture.Face2[i].txcoor - wo2,
			       w->picture.Face2[i].tycoor - ho2,
			       w->picture.Face2[i].tzcoor - cam_screen_z,
			       sod_width, sod_height);
	if (!w->picture.box_line[i+8].rejected)
	    {
	    if (clip_line(w,
		      w->picture.Face1[i].txcoor - wo2,
		      w->picture.Face1[i].tycoor - ho2,
		      w->picture.Face1[i].tzcoor - cam_screen_z,
		      w->picture.Face2[i].txcoor - wo2,
		      w->picture.Face2[i].tycoor - ho2,
		      w->picture.Face2[i].tzcoor - cam_screen_z,
		      sod_width, sod_height,
		      &new_x1, &new_y1, &new_z1,
		      &new_x2, &new_y2, &new_z2))
		{
		new_x1 += wo2; new_y1 += ho2; new_z1 += cam_screen_z;
		new_x2 += wo2; new_y2 += ho2; new_z2 += cam_screen_z;
		perspective_divide( w, new_x1, new_y1, new_z1/zscale, &x, &y);
		w->picture.box_line[i+8].x1 = x;
		w->picture.box_line[i+8].y1 = y;
		perspective_divide( w, new_x2, new_y2, new_z2/zscale, &x, &y);
		w->picture.box_line[i+8].x2 = x;
		w->picture.box_line[i+8].y2 = y;
		}
	    }
	}
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: clip_line 			       			     */
/* Effect:     clip the line segment to the viewing frustum		     */
/*****************************************************************************/
static Boolean  clip_line(XmPictureWidget w,
		       double x1, double y1, double z1,
		       double x2, double y2, double z2,
		       double sod_width, double sod_height,
		       double *new_x1, double *new_y1, double *new_z1,
		       double *new_x2, double *new_y2, double *new_z2)
{
Boolean clipped = False;
Boolean y1_above;
Boolean y2_above;
Boolean x1_left;
Boolean x2_left;
Boolean y1_below;
Boolean y2_below;
Boolean x1_right;
Boolean x2_right;
double  m;
double  b;
double  new_x;
double  new_y;
double  new_z;
double  old_length_x;
double  old_length_y;
double  old_length_z=0;
double  new_length_x;
double  new_length_y;
double  new_length_z;

    /*
     * No clipping in orthographic projections.
     */
    if (w->picture.projection == 0) return False;

    /*
     * No need to clip if we are looking straight down the z axis
     */
    if(z1 == z2) return False;

    /*
     * If both points are inside the of the viewing frustum,
     * the line is trivially accepted.
     */
    if ( ((y1 <= -sod_height * z1) && (y2 <= -sod_height * z2)) &&
	 ((y1 >=  sod_height * z1) && (y2 >=  sod_height * z2)) &&
	 ((x1 <= -sod_width  * z1) && (x2 <= -sod_width  * z2)) &&
	 ((x1 >=  sod_width  * z1) && (x2 >=  sod_width  * z2)) )
	{
	return False;
	}

    /*
     * If we get here, the line has not been trivially accepted or rejected.
     */
    y1_below = (y1 < sod_height * z1);
    y2_below = (y2 < sod_height * z2);
    y1_above = (y1 > -sod_height * z1);
    y2_above = (y2 > -sod_height * z2);

    /*
     * Equation of the line segment --> y = m*z +b where b = y1 - m*z1
     */
    m = (y1 - y2)/(z1 - z2);
    b = y1 - m*z1;

    /*
     * If the y value of a point is both above and below the viewing frustum,
     * it is behind the camera.  In order to clip the point to the proper
     * plane, look at the y intercept.  If it is greater than zero, clip
     * to the "above" plane, else clip to the "below" plane.
     */
    if (y1_above && y1_below)
	{
	if (b > 0.0) y1_below = False; 
	if (b < 0.0) y1_above = False; 
	}
    if (y2_above && y2_below)
	{
	if (b > 0.0) y2_below = False; 
	if (b < 0.0) y2_above = False; 
	}
    if ( (y1_above && !y2_above) || (!y1_above && y2_above) )
	{
	new_z = -b/(m + sod_height);
	new_y = m*new_z + b;
	old_length_y = y2 - y1;
	/*
	 * If the old y length is 0.0, use z to recalc the clipped x.
	 */
	if(old_length_y == 0.0) 
	    old_length_z = z2 - z1;
	old_length_x = x2 - x1;
	if (y1_above)
	    {
	    y1 = new_y; z1 = new_z;
	    if(old_length_y != 0.0)
		{
		new_length_y = y2 - y1;
		x1 = x2 - old_length_x*(new_length_y/old_length_y);
		}
	    else
		{
		new_length_z = z2 - z1;
		x1 = x2 - old_length_x*(new_length_z/old_length_z);
		}
	    }
	else
	    {
	    y2 = new_y; z2 = new_z;
	    if(old_length_y != 0.0)
		{
		new_length_y = y2 - y1;
		x2 = x1 + old_length_x*(new_length_y/old_length_y);
		}
	    else
		{
		new_length_z = z2 - z1;
		x2 = x1 + old_length_x*(new_length_z/old_length_z);
		}
	    }
	clipped = True;
	}
    if ( (y1_below && !y2_below) || (!y1_below && y2_below) )
	{
	new_z = b/(sod_height - m);
	new_y = m*new_z + b;
	old_length_y = y2 - y1;
	/*
	 * If the old y length is 0.0, use z to recalc the clipped x.
	 */
	if(old_length_y == 0.0) 
	    old_length_z = z2 - z1;
	old_length_x = x2 - x1;
	if (y1_below)
	    {
	    y1 = new_y; z1 = new_z;
	    if(old_length_y != 0.0)
		{
		new_length_y = y2 - y1;
		x1 = x2 - old_length_x*(new_length_y/old_length_y);
		}
	    else
		{
		new_length_z = z2 - z1;
		x1 = x2 - old_length_x*(new_length_z/old_length_z);
		}
	    }
	else
	    {
	    y2 = new_y; z2 = new_z;
	    if(old_length_y != 0.0)
		{
		new_length_y = y2 - y1;
		x2 = x1 + old_length_x*(new_length_y/old_length_y);
		}
	    else
		{
		new_length_z = z2 - z1;
		x2 = x1 + old_length_x*(new_length_z/old_length_z);
		}
	    }
	clipped = True;
	}

    x1_left  = (x1 <  sod_width  * z1);
    x2_left  = (x2 <  sod_width  * z2);
    x1_right = (x1 > -sod_width  * z1);
    x2_right = (x2 > -sod_width  * z2);

    /*
     * Equation of the line segment --> x = m*z +b where b = x1 - m*z1
     */
    m = (x1 - x2)/(z1 - z2);
    b = x1 - m*z1;

    /*
     * If the x value of a point is both to the left and right of the viewing 
     *frustum, it is behind the camera.  In order to clip the point to the 
     * proper plane, look at the x intercept.  If it is greater than zero, clip
     * to the "left" plane, else clip to the "right" plane.
     */
    if (x1_left && x1_right)
	{
	if (b > 0.0) x1_right = False; 
	if (b < 0.0) x1_left  = False; 
	}
    if (x2_left  && x2_right)
	{
	if (b > 0.0) x2_right = False; 
	if (b < 0.0) x2_left  = False; 
	}
    if ( (x1_left && !x2_left) || (!x1_left && x2_left) )
	{
	new_z = b/(sod_width - m);
	new_x = m*new_z + b;
	old_length_y = y2 - y1;
	old_length_x = x2 - x1;
	/*
	 * If the old x length is 0.0, use z to recalc the clipped y.
	 */
	if(old_length_x == 0.0) 
	    old_length_z = z2 - z1;
	if (x1_left)
	    {
	    x1 = new_x; z1 = new_z;
	    if(old_length_x != 0.0)
		{
		new_length_x = x2 - x1;
		y1 = y2 - old_length_y * (new_length_x / old_length_x);
		}
	    else
		{
		new_length_z = z2 - z1;
		y1 = y2 - old_length_y * (new_length_z / old_length_z);
		}
	    }
	else
	    {
	    x2 = new_x; z2 = new_z;
	    if(old_length_x != 0.0)
		{
		new_length_x = x2 - x1;
		y2 = y1 + old_length_y * (new_length_x / old_length_x);
		}
	    else
		{
		new_length_z = z2 - z1;
		y2 = y1 + old_length_y * (new_length_z / old_length_z);
		}
	    }
	clipped = True;
	}
    if ( (x1_right && !x2_right) || (!x1_right && x2_right) )
	{
	new_z = -b/(m + sod_width);
	new_x = m*new_z + b;
	old_length_y = y2 - y1;
	old_length_x = x2 - x1;
	/*
	 * If the old x length is 0.0, use z to recalc the clipped y.
	 */
	if(old_length_x == 0.0) 
	    old_length_z = z2 - z1;
	if (x1_right)
	    {
	    x1 = new_x; z1 = new_z;
	    if(old_length_x != 0.0)
		{
		new_length_x = x2 - x1;
		y1 = y2 - old_length_y * (new_length_x / old_length_x);
		}
	    else
		{
		new_length_z = z2 - z1;
		y1 = y2 - old_length_y * (new_length_z / old_length_z);
		}
	    }
	else
	    {
	    x2 = new_x; z2 = new_z;
	    if(old_length_x != 0.0)
		{
		new_length_x = x2 - x1;
		y2 = y1 + old_length_y * (new_length_x / old_length_x);
		}
	    else
		{
		new_length_z = z2 - z1;
		y2 = y1 + old_length_y * (new_length_z / old_length_z);
		}
	    }
	clipped = True;
	}

    *new_x1 = x1; *new_y1 = y1; *new_z1 = z1;
    *new_x2 = x2; *new_y2 = y2; *new_z2 = z2;

    return clipped;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: trivial_reject						     */
/* Effect:     See if the line is completely outside the viewing frustum    */
/*****************************************************************************/
static Boolean  trivial_reject(XmPictureWidget w,
			       double x1, double y1, double z1,
			       double x2, double y2, double z2,
			       double sod_width, double sod_height)
{
    
    /*
     * No clipping in orthographic projections.
     */
     if (w->picture.projection == 0) return False;

    /*
     * If both points are behind the camera, the line is trivially rejected.
     */
     if ( (z1 > 0.0) && (z2 > 0.0) )
	{
	return True;
	}

    /*
     * If both points are outside the same plane of the viewing frustum,
     * the line is trivially rejected.
     */
    if ( ((y1 >= -sod_height * z1) && (y2 >= -sod_height * z2)) ||
	 ((y1 <=  sod_height * z1) && (y2 <=  sod_height * z2)) ||
	 ((x1 >= -sod_width  * z1) && (x2 >= -sod_width  * z2)) ||
	 ((x1 <=  sod_width  * z1) && (x2 <=  sod_width  * z2)) )
	{
	return True;
	}

    return False;
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: setup_gnomon						     */
/* Effect:     Calculate the screen coords of the gnomon (to be drawn later) */
/*****************************************************************************/
static void  setup_gnomon(XmPictureWidget w, Boolean rotate)
{
double      angle;
double      delta_x;
double      delta_y;
double      length;

double      origin_sxcoor;
double      origin_sycoor;
double      origin_szcoor;
double      xaxis_sxcoor;
double      xaxis_sycoor;
double      xaxis_szcoor;
double      yaxis_sxcoor;
double      yaxis_sycoor;
double      yaxis_szcoor;
double      zaxis_sxcoor;
double      zaxis_sycoor;
double      zaxis_szcoor;

double      xaxis_length;
double      yaxis_length;
double      zaxis_length;
double      total_length;
int         direction;
int         ascent;
int         descent;
XCharStruct overall;
char	    string[10];
int	    origin_offset_x;
int	    origin_offset_y;

    if (!w->picture.camera_defined) return;
    string[0] = 'Z';
    string[1] = '\0';
    XTextExtents(w->picture.font, string, 1,
		&direction, &ascent, &descent, &overall);
    
    /*
     * Set up gnomon
     */
    if (rotate)
	{
	xform_coords( w->picture.globe->Wtrans,
		    0.0, 0.0, 0.0,
		    &origin_sxcoor, &origin_sycoor, &origin_szcoor);

	xform_coords( w->picture.globe->Wtrans,
		    1.0, 0.0, 0.0,
		    &xaxis_sxcoor, &xaxis_sycoor, &xaxis_szcoor);

	xform_coords( w->picture.globe->Wtrans,
		    0.0, -1.0, 0.0,
		    &yaxis_sxcoor, &yaxis_sycoor, &yaxis_szcoor);

	xform_coords( w->picture.globe->Wtrans,
		    0.0, 0.0, 1.0,
		    &zaxis_sxcoor, &zaxis_sycoor, &zaxis_szcoor);
	}
    else
	{
	xform_coords( w->picture.PureWtrans,
		    0.0, 0.0, 0.0,
		    &origin_sxcoor, &origin_sycoor, &origin_szcoor);

	xform_coords( w->picture.PureWtrans,
		    1.0, 0.0, 0.0,
		    &xaxis_sxcoor, &xaxis_sycoor, &xaxis_szcoor);

	xform_coords( w->picture.PureWtrans,
		    0.0, 1.0, 0.0,
		    &yaxis_sxcoor, &yaxis_sycoor, &yaxis_szcoor);

	xform_coords( w->picture.PureWtrans,
		    0.0, 0.0, 1.0,
		    &zaxis_sxcoor, &zaxis_sycoor, &zaxis_szcoor);
	}

    xaxis_length = sqrt( (xaxis_sxcoor-origin_sxcoor) * 
			 (xaxis_sxcoor-origin_sxcoor) +
			 (xaxis_sycoor-origin_sycoor) *
			 (xaxis_sycoor-origin_sycoor) );
    yaxis_length = sqrt( (yaxis_sxcoor-origin_sxcoor) * 
			 (yaxis_sxcoor-origin_sxcoor) +
			 (yaxis_sycoor-origin_sycoor) *
			 (yaxis_sycoor-origin_sycoor) );
    zaxis_length = sqrt( (zaxis_sxcoor-origin_sxcoor) * 
			 (zaxis_sxcoor-origin_sxcoor) +
			 (zaxis_sycoor-origin_sycoor) *
			 (zaxis_sycoor-origin_sycoor) );

    total_length = xaxis_length + yaxis_length + zaxis_length;

    origin_offset_x = (w->core.width - 60) - origin_sxcoor;
    origin_offset_y = (w->core.height - 60) - origin_sycoor;

    /*
     * Calc the projected X axis
     */
    length = 80 * (xaxis_length/total_length);
    if (xaxis_length > 0)
	{
	angle =  acos((xaxis_sxcoor - origin_sxcoor)/xaxis_length);
	}
    else
	{
	angle = 0.0;
	}
    if (xaxis_sycoor > origin_sycoor) angle = 2*M_PI - angle;
    delta_x = length * cos(angle);
    delta_y = -length * sin(angle);

    w->picture.gnomon_center_x = origin_sxcoor + origin_offset_x;
    w->picture.gnomon_center_y = origin_sycoor + origin_offset_y;
    w->picture.gnomon_xaxis_x = w->picture.gnomon_center_x + delta_x;
    w->picture.gnomon_xaxis_y = w->picture.gnomon_center_y + delta_y;

    /*
     * Calc the placement of the gnomon label
     */
    length = length + 5;
    delta_x = length * cos(angle);
    delta_y = -length * sin(angle);
    w->picture.gnomon_xaxis_label_x = w->picture.gnomon_center_x + delta_x;
    w->picture.gnomon_xaxis_label_y = w->picture.gnomon_center_y + delta_y;
    if (delta_x < 0) w->picture.gnomon_xaxis_label_x -= overall.width;
    if (delta_y > 0) w->picture.gnomon_xaxis_label_y += (ascent+descent);

    /*
     * Calc the projected Y axis
     */
    length = 80 * (yaxis_length/total_length);
    if (yaxis_length > 0)
	{
	angle =  acos((yaxis_sxcoor - origin_sxcoor)/yaxis_length);
	}
    else
	{
	angle = 0.0;
	}
    if (yaxis_sycoor > origin_sycoor) angle = 2*M_PI - angle;
    delta_x = length * cos(angle);
    delta_y = -length * sin(angle);
    w->picture.gnomon_yaxis_x = w->picture.gnomon_center_x + delta_x;
    w->picture.gnomon_yaxis_y = w->picture.gnomon_center_y + delta_y;

    /*
     * Calc the placement of the gnomon label
     */
    length = length + 5;
    delta_x = length * cos(angle);
    delta_y = -length * sin(angle);
    w->picture.gnomon_yaxis_label_x = w->picture.gnomon_center_x + delta_x;
    w->picture.gnomon_yaxis_label_y = w->picture.gnomon_center_y + delta_y;
    if (delta_x < 0) w->picture.gnomon_yaxis_label_x -= overall.width;
    if (delta_y > 0) w->picture.gnomon_yaxis_label_y += (ascent+descent);

    /*
     * Calc the projected Z axis
     */
    length = 80 * (zaxis_length/total_length);
    if (zaxis_length > 0)
	{
	angle =  acos((zaxis_sxcoor - origin_sxcoor)/zaxis_length);
	}
    else
	{
	angle = 0.0;
	}
    if (zaxis_sycoor > origin_sycoor) angle = 2*M_PI - angle;
    delta_x = length * cos(angle);
    delta_y = -length * sin(angle);
    w->picture.gnomon_zaxis_x = w->picture.gnomon_center_x + delta_x;
    w->picture.gnomon_zaxis_y = w->picture.gnomon_center_y + delta_y;

    /*
     * Calc the placement of the gnomon label
     */
    length = length + 5;
    delta_x = length * cos(angle);
    delta_y = -length * sin(angle);
    w->picture.gnomon_zaxis_label_x = w->picture.gnomon_center_x + delta_x;
    w->picture.gnomon_zaxis_label_y = w->picture.gnomon_center_y + delta_y;
    if (delta_x < 0) w->picture.gnomon_zaxis_label_x -= overall.width;
    if (delta_y > 0) w->picture.gnomon_zaxis_label_y += (ascent+descent);

}



/*****************************************************************************/
/*                                                                           */
/* Subroutine: CallCursorCallbacks					     */
/* Effect:     xform screen x,y,z back to world coords and callback 	     */
/*             application.                                                  */
/*****************************************************************************/
static void CallCursorCallbacks(XmPictureWidget w, int reason, int cursor_num, 
		double screen_x, double screen_y, double screen_z)
{
XmPictureCallbackStruct cb;
char	text[256];
Arg	wargs[20];
XmString xmstring;
int     height;
int	dest_x;
int	dest_y;
Widget	child;

    cb.reason = reason;
    cb.cursor_num = cursor_num;

    if (w->picture.mode == XmPICK_MODE)
	{
	cb.x = screen_x;
	cb.y = w->core.height - screen_y;
	XtCallCallbacks ((Widget)w, XmNpickCallback, &cb);
	return;
	}
    perspective_divide_inverse(w, screen_x, screen_y, screen_z, 
		&screen_x, &screen_y); 
    xform_coords( w->picture.WItrans, screen_x, screen_y, screen_z, 
		  &cb.x, &cb.y, &cb.z);

    /*
     * Correct for the fact that we have xformed back to cannonical coords.
     */
    cb.x += w->picture.Ox;
    cb.y += w->picture.Oy;
    cb.z += w->picture.Oz;

    sprintf(text, "( %8g, %8g, %8g )", cb.x, cb.y, cb.z);
    if ( (reason == XmPCR_CREATE) ||
	 (reason == XmPCR_SELECT) )
	{
	xmstring = XmStringCreate(text, XmSTRING_DEFAULT_CHARSET);
	XtSetArg(wargs[0], XmNlabelString, xmstring);
	XtSetValues((Widget)w->picture.pb, wargs, 1);

	/*
	 * Find a good place to put the popup.
	 */
	XTranslateCoordinates
	    (XtDisplay(w),
	     XtWindow(w),
	     XRootWindowOfScreen(XtScreen(w)),
	     w->core.width - 350,
	     -35,
	     &dest_x,
	     &dest_y,
	     (Window*)&child);
	XtMoveWidget(w->picture.popup,
		     dest_x,
		     dest_y);
	XtPopup(w->picture.popup, XtGrabNone);
	w->picture.popped_up = True;
	XmStringFree(xmstring);
	}
    if ( (reason == XmPCR_DELETE) ||
	 (reason == XmPCR_MOVE) )
	{
	XtPopdown(w->picture.popup);
	w->picture.popped_up = False;
	}
    if (XtIsRealized((Widget)w->picture.pb))
	{
	XClearArea( XtDisplay(w), 
		    XtWindow(w->picture.pb), 
		    w->picture.pb->primitive.shadow_thickness,
		    w->picture.pb->primitive.shadow_thickness,
		    w->picture.pb->core.width - 
			    2*w->picture.pb->primitive.shadow_thickness,
		    w->picture.pb->core.height - 
			    2*w->picture.pb->primitive.shadow_thickness,
		    False);
	height = (w->picture.pb->core.height +
		    (w->picture.font->descent + w->picture.font->ascent))/2 - 3;
	XDrawString(XtDisplay(w),
		    XtWindow(w->picture.pb),
		    w->picture.fontgc,
		    10,
		    height,
		    text,
		    text != NULL? strlen(text): 0);
	}
    XFlush(XtDisplay(w));

    if (w->picture.mode == XmCURSOR_MODE)
	{
	XtCallCallbacks ((Widget)w, XmNcursorCallback, &cb);
	}
    else if (w->picture.mode == XmROAM_MODE)
	{
	/*
	 * Save the current camera in case the user wants to undo this
	 * interaction, if this is a "Move".
	 */
	if (reason == XmPCR_MOVE) 
	    {
	    push_undo_camera(w);
	    XtCallCallbacks ((Widget)w, XmNroamCallback, &cb);
	    }
	if (reason == XmPCR_MOVE)
	    {
	    w->picture.disable_temp = True;
	    }
	}
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: CallNavigateCallbacks					     */
/* Effect:     								     */
/*****************************************************************************/
static void CallNavigateCallbacks(XmPictureWidget w,
		int screen_x, int screen_y, int reason)
{
XmPictureCallbackStruct cb;
double                  dx;
double                  dy;
double                  dz;
double			length;
double			orig_length;
double			ratio;
double			tmp;
double			dxs;
double			dys;
double			dzs;
double			sfx;	/* Screen from x */
double			sfy;	/* Screen from y */
double			sfz;	/* Screen from z */
double			stx;	/* Screen to x */
double			sty;	/* Screen to y */
double			stz;	/* Screen to z */
double			dir_x;
double			dir_y;
double			dir_z;
double			dir_xp;
double			dir_yp;
double			dir_zp;
double			up_xp;
double			up_yp;
double			up_zp;
double			up_xpp;
double			up_ypp;
double			up_zpp;
double			up_xppp;
double			up_yppp;
double			up_zppp;
double			xform[4][4];
double			angle1;
double			angle2;
double			angle3;

    cb.reason = reason;

    /*
     * Adjust the direction the camera is pointed towards the indicated
     * screen (x,y) postition.
     */
    if(w->picture.look_at_direction == XmLOOK_FORWARD)
	{
	dxs = (w->picture.navigate_to_x - w->picture.navigate_from_x)*
	      (w->picture.navigate_to_x - w->picture.navigate_from_x);
	dys = (w->picture.navigate_to_y - w->picture.navigate_from_y)*
	      (w->picture.navigate_to_y - w->picture.navigate_from_y);
	dzs = (w->picture.navigate_to_z - w->picture.navigate_from_z)*
	      (w->picture.navigate_to_z - w->picture.navigate_from_z);
	orig_length = sqrt(dxs + dys + dzs);

	dx = (double)screen_x - (double)w->core.width/2;
	dy = (double)w->core.height/2 - (double)screen_y;

	/*
	 * Do this in screen coords, it's much simpler.
	 */
	xform_coords(w->picture.PureWtrans, 
		    w->picture.navigate_from_x, 
		    w->picture.navigate_from_y,
		    w->picture.navigate_from_z, 
		    &sfx, &sfy, &sfz);
	xform_coords(w->picture.PureWtrans, 
		    w->picture.navigate_to_x, 
		    w->picture.navigate_to_y,
		    w->picture.navigate_to_z, 
		    &stx, &sty, &stz);
	length = fabs(sfz - stz);

	tmp = ((double)(w->picture.rotate_speed_factor)*
	       (double)(w->picture.rotate_speed_factor)*
	       (double)(w->picture.rotate_speed_factor))/1000000.0;

	stx += (dx * tmp);
	sty -= (dy * tmp);

	/*
	 * Back to the real world (coords)
	 */
	xform_coords(w->picture.PureWItrans, 
		    stx, sty, stz, 
		    &w->picture.navigate_to_x, 
		    &w->picture.navigate_to_y,
		    &w->picture.navigate_to_z);

	/*
	 * Length of vector between from and (new)to points
	 */
	dxs = (w->picture.navigate_to_x - w->picture.navigate_from_x)*
	      (w->picture.navigate_to_x - w->picture.navigate_from_x);
	dys = (w->picture.navigate_to_y - w->picture.navigate_from_y)*
	      (w->picture.navigate_to_y - w->picture.navigate_from_y);
	dzs = (w->picture.navigate_to_z - w->picture.navigate_from_z)*
	      (w->picture.navigate_to_z - w->picture.navigate_from_z);
	length = sqrt(dxs + dys + dzs);

	/*
	 * We have to adjust the z position of the "to" point so
	 * the length of the vector(from-to) does not change from the original.
	 */
	ratio = orig_length/length;
	dx = ratio*(w->picture.navigate_to_x - 
		    w->picture.navigate_from_x);
	dy = ratio*(w->picture.navigate_to_y - 
		    w->picture.navigate_from_y);
	dz = ratio*(w->picture.navigate_to_z - 
		    w->picture.navigate_from_z);
	w->picture.navigate_to_x = w->picture.navigate_from_x + dx;
	w->picture.navigate_to_y = w->picture.navigate_from_y + dy;
	w->picture.navigate_to_z = w->picture.navigate_from_z + dz;
    }    

    /*
     * If we are navigating w/ button 2, do not move the camera (x,y,z).
     */
    if (w->picture.button_pressed != 2)
	{
	/*
	 * Translate the to and from points along the vector
	 */
	length = orig_length;

	tmp = ((double)(w->picture.translate_speed_factor)*
	       (double)(w->picture.translate_speed_factor)*
	       (double)(w->picture.translate_speed_factor))/1000000.0;

	dx=tmp*(w->picture.navigate_to_x - w->picture.navigate_from_x);
	dy=tmp*(w->picture.navigate_to_y - w->picture.navigate_from_y);
	dz=tmp*(w->picture.navigate_to_z - w->picture.navigate_from_z);
	if (w->picture.navigate_direction == XmBACKWARD)
	    {
	    dx = -dx;
	    dy = -dy;
	    dz = -dz;
	    }
	w->picture.navigate_from_x += dx;
	w->picture.navigate_from_y += dy;
	w->picture.navigate_from_z += dz;
	w->picture.navigate_to_x += dx;
	w->picture.navigate_to_y += dy;
	w->picture.navigate_to_z += dz;
	}

    dir_x =  w->picture.navigate_from_x - w->picture.navigate_to_x;
    dir_y =  w->picture.navigate_from_y - w->picture.navigate_to_y;
    dir_z =  w->picture.navigate_from_z - w->picture.navigate_to_z;

    /*
     *  Adjust the up vector to be strait up in screen space
     *  General stategy:
     *		1) Rotate the direction vector about the Y axis
     *		   so it is aligned with the Z axis.
     *		2) Rotate the direction vector so it lies on the
     *		   XZ plane.
     *		3) Apply these rotations to the up vector.
     *		4) Rotate the resulting up vector so it lies in
     *		   the YZ plane.
     *		5) Zero out the z component of the resulting up
     *		   vector, and apply the inverse rotations.
     */
    /*
     * Rotation about the y axis
     */
    length = sqrt(dir_x*dir_x + dir_z*dir_z);
    if (length != 0.0)
	{
	angle1 = acos(dir_z/length);
	}
    else
	{
	angle1 = 0.0;
	}
    if (dir_x > 0)
	{
	angle1 = -angle1;
	}
    I44(xform);
    set_rot(xform, angle1, Yaxis);
    xform_coords( xform, 
		w->picture.navigate_up_x, 
		w->picture.navigate_up_y, 
		w->picture.navigate_up_z, 
		&up_xp, &up_yp, &up_zp);
    xform_coords( xform, dir_x, dir_y, dir_z, &dir_xp, &dir_yp, &dir_zp);

    /*
     * Rotation about the x axis
     */
    length = sqrt(dir_yp*dir_yp + dir_zp*dir_zp);
    if (length != 0.0)
	{
	angle2 = acos(dir_zp/length);
	}
    else
	{
	angle2 = 0.0;
	}
    if (dir_yp < 0)
	{
	angle2 = -angle2;
	}

    I44(xform);
    set_rot(xform, angle2, Xaxis);
    xform_coords(xform, up_xp, up_yp, up_zp, &up_xpp, &up_ypp, &up_zpp);

    /*
     * Rotate about the z axis so that the up vector is aligned w/ the y axis
     */
    length = sqrt(up_xpp*up_xpp+ up_ypp*up_ypp);
    if (length != 0.0)
	{
	angle3 = acos(up_ypp/length);
	}
    else
	{
	angle3 = 0.0;
	}
    if (up_xpp < 0)
	{
	angle3 = -angle3;
	}

    I44(xform);
    set_rot(xform, angle3, Zaxis);
    xform_coords(xform, up_xpp, up_ypp, up_zpp, &up_xppp, &up_yppp, &up_zppp);

    I44(xform);
    set_rot(xform, -angle3, Zaxis);
    xform_coords(xform, up_xppp, up_yppp, 0.0, &up_xpp, &up_ypp, &up_zpp);

    I44(xform);
    set_rot(xform, -angle2, Xaxis);
    xform_coords(xform, up_xpp, up_ypp, 0.0, &up_xp, &up_yp, &up_zp);
    
    I44(xform);
    set_rot(xform, -angle1, Yaxis);
    xform_coords(xform, up_xp, up_yp, up_zp, &w->picture.navigate_up_x,
		&w->picture.navigate_up_y, &w->picture.navigate_up_z);
    
    /*
     * Re-normalize the up vector
     */
    length = sqrt(w->picture.navigate_up_x * w->picture.navigate_up_x +
		  w->picture.navigate_up_y * w->picture.navigate_up_y +
		  w->picture.navigate_up_z * w->picture.navigate_up_z);
    w->picture.navigate_up_x = w->picture.navigate_up_x/length;
    w->picture.navigate_up_y = w->picture.navigate_up_y/length;
    w->picture.navigate_up_z = w->picture.navigate_up_z/length;

    /*
     * Now that we have the new navigation camera parameters, calculate the
     * actual camera params (based on the current "look at" direction and
     * angle.
     */
    set_camera_from_nav_camera(w);

    cb.x = w->picture.to_x;
    cb.y = w->picture.to_y;
    cb.z = w->picture.to_z;

    cb.from_x =  w->picture.from_x;
    cb.from_y =  w->picture.from_y;
    cb.from_z =  w->picture.from_z;

    cb.up_x =  w->picture.up_x;
    cb.up_y =  w->picture.up_y;
    cb.up_z =  w->picture.up_z;

    cb.autocamera_width = w->picture.autocamera_width;

    XtCallCallbacks ((Widget)w, XmNnavigateCallback, &cb);
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: keyboard_grab						     */
/* Effect:     								     */
/*****************************************************************************/
static void keyboard_grab(XmPictureWidget w, Boolean grab)
{
    if (grab)
	{
	w->picture.grab_keyboard_count++;
	if (w->picture.grab_keyboard_count == 1)
	    {
	    XtGrabKeyboard((Widget)w, False, GrabModeAsync, GrabModeAsync, CurrentTime);
	    }
	}
    else
	{
	if (w->picture.grab_keyboard_count != 0) 
	    w->picture.grab_keyboard_count--;
	if (w->picture.grab_keyboard_count == 0)
	    {
	    XtUngrabKeyboard((Widget)w, CurrentTime);
	    }
	}
}

/*****************************************************************************/
/*                                                                           */
/* Subroutine: alloc_drawing_colors					     */
/* Effect: allocate black and white from the colormap associated with this win*/
/*****************************************************************************/
static void alloc_drawing_colors(XmPictureWidget new)
{
XWindowAttributes	win_att;
XColor			white;
XColor			black;
XColor			rgb_db_def;

    if(new->image.frame_buffer)
    {
	new->picture.white = WhitePixel(XtDisplay(new), 0);
	new->picture.black = BlackPixel(XtDisplay(new), 0);
    }
    else
    {
	XGetWindowAttributes(XtDisplay(new), XtWindow(new), &win_att);
	if(win_att.colormap != None)
	{
	    if(win_att.depth == 8)
	    {
		if (!XAllocNamedColor(XtDisplay(new), win_att.colormap, "white",
					&white, &rgb_db_def))
		    find_color((Widget)new, &white);
		new->picture.white = white.pixel;

		if (!XAllocNamedColor(XtDisplay(new), win_att.colormap, "black",
					&black, &rgb_db_def))
		    find_color((Widget)new, &black);
		new->picture.black = black.pixel;
	    }
	    else
	    {
		white.red = white.green = white.blue = 0xffff;
		black.red = black.green = black.blue = 0x0;
		if(!XAllocColor(XtDisplay(new), win_att.colormap, &white))
		    find_color((Widget)new, &white);
		if(!XAllocColor(XtDisplay(new), win_att.colormap, &black))
		    find_color((Widget)new, &white);
		new->picture.white = white.pixel;
		new->picture.black = black.pixel;
	    }
	}
	else
	{
	    new->picture.white = WhitePixel(XtDisplay(new), 0);
	    new->picture.black = BlackPixel(XtDisplay(new), 0);
	}
    }
}
/*****************************************************************************/
/*                                                                           */
/* Subroutine: convert color						     */
/* Effect: Converts a pixel value from the default colormap to the current cm*/
/*****************************************************************************/
static void convert_color(XmPictureWidget w, XColor *color)
{
int                     screen;
Colormap                cm;
XWindowAttributes       att;

    color->flags = DoRed | DoGreen | DoBlue;
    if(w->image.frame_buffer)
    {
	screen = DefaultScreen(XtDisplay(w));
	cm = DefaultColormap(XtDisplay(w), screen);

	XGetWindowAttributes(XtDisplay(w), w->picture.overlay_wid, &att);
	if(att.colormap != None)
	{
	    cm = att.colormap;
	}
	if (!XAllocColor(XtDisplay(w), cm, color))
	{
	    find_color((Widget)w, color);
	}
    }
    else
    {
	screen = XScreenNumberOfScreen(XtScreen(w));
	cm = DefaultColormap(XtDisplay(w), screen);

	XGetWindowAttributes(XtDisplay(w), XtWindow(w), &att);
	if(att.colormap != None)
	{
	    cm = att.colormap;
	}
	if (!XAllocColor(XtDisplay(w), cm, color))
	{
	    find_color((Widget)w, color);
	}
    }
}

/* -------------------------- Transformation library ----------------------- */

static int 
set_trans ( double res[4][4] , double s0, short s1 )

{
int  i, j;
double w[4][4];

	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    if ( i == j )     w[i][i] = 1.0;
	    else 	      w[i][j] = 0.0;



	switch ( s1 )
	{
	     case Xaxis :    w[3][0] = s0;
			     break;

	     case Yaxis :    w[3][1] = s0;
			     break;

	     case Zaxis :    w[3][2] = s0;
			     break;
	}

	mult44 ( res, w );
    return 0;
}


static int set_rot ( double res[4][4],  double s0, short s1 )

{
int  i, j;
double w[4][4];

	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    if ( i == j )     w[i][i] = 1.0;
	    else 	      w[i][j] = 0.0;



	switch ( s1 )
	{
	     case Xaxis :    w[1][1] = cos ( s0 );
			     w[1][2] = sin ( s0 );
			     w[2][1] = -sin ( s0 );
			     w[2][2] = cos ( s0 );
			     break;

	     case Yaxis :    w[0][0] = cos ( s0 );
			     w[2][0] = sin ( s0 );
			     w[0][2] = -sin ( s0 );
			     w[2][2] = cos ( s0 );
			     break;

	    case Zaxis :     w[0][0] = cos ( s0 );
			     w[0][1] = sin ( s0 );
			     w[1][0] = -sin ( s0 );
			     w[1][1] = cos ( s0 );
			     break;
	}

	mult44 ( res, w );

    return 0;
}


/* --------------------------------- linear Algebra ------------------------ */




static int
mult44 ( s0, s1 )
double s0[4][4];
double s1[4][4];

{

int i, j, k;
double res[4][4];

	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    res[i][j] = 0.0;

	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    for ( k = 0 ; k < 4 ; k++ )
	      res[i][j] += (s0[i][k] * s1[k][j]);


	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    s0[i][j] = res[i][j];

    return 0;
}


static double
det33 ( s0 )
double s0[4][4];

{

double    det;

	det = ( s0[0][0] * ((s0[1][1] * s0[2][2]) - (s0[2][1] * s0[1][2])) ) +
	      (-s0[0][1] * ((s0[1][0] * s0[2][2]) - (s0[2][0] * s0[1][2])) ) +
	      ( s0[0][2] * ((s0[1][0] * s0[2][1]) - (s0[2][0] * s0[1][1])) );


return det;

}



static double
cofac ( s0 , s1, s2 )
double    s0[4][4];
int       s1;
int       s2;

{

int i, j, I = -1, J = 0;
double cofac[4][4];

	for ( i = 0 ; i < 4 ; i++ )
	  if ( i != s1 )
	  {
	   I++;
	   J = 0;
	   for ( j = 0 ; j < 4 ; j++ )
	    {
		if ( j != s2 )
		{
		   cofac[I][J] = s0[i][j];
		   J++;
		}
	    }
	   }

return ( det33( cofac ) );
}



static int 
inverse ( s0, s1 )
double   s0[4][4];
double   s1[4][4];

{

int i, j;
double det;

	det = (s0[0][0] * cofac(s0, 0, 0)) + (-s0[0][1] * cofac(s0, 0, 1)) + 
	      (s0[0][2] * cofac(s0, 0, 2)) + (-s0[0][3] * cofac(s0, 0, 3));

	det = 1.0 / det;

	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    s1[j][i] = ( (((i + j + 1) % 2) * 2) - 1 ) * cofac(s0, i, j ) * det;

    return 0;

}


static int
I44 (double   w[4][4] )
{
int   i, j;

	for ( i = 0 ; i < 4 ; i++ )
	  for ( j = 0 ; j < 4 ; j++ )
	    if ( i == j )    w[i][j] = 1.0;
	    else
			     w[i][j] = 0.0;
  return 0;
}

/*  Subroutine:	XmCreatePicture
 *  Purpose:	This function creates and returns a Picture widget.
 */
Widget XmCreatePicture( Widget parent, String name,
			 ArgList args, Cardinal num_args )
{
    return XtCreateWidget(name, xmPictureWidgetClass, parent, args, num_args);
}


/*
 *  Subroutine: XmSetNavigateMode
 *  Purpose:    set up Picture widget so that first image software
 *            rendered in Navigate mode initializes the navigation camera
 */

Boolean
XmPictureInitializeNavigateMode(XmPictureWidget w)
{
    w->picture.first_step = True;
    w->picture.ignore_new_camera = 0;
    return True;
}

