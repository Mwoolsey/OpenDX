/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



/********************************************
 * Dial.c: The Dial Widget Methods.
 ********************************************/
#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "DialP.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#if (HAS_M_PI == 0)
#define M_PI    3.1415926535897931160E0
#endif

#if !defined(HAVE_TRUNC)
# ifndef trunc
#  define trunc(c)	((double)((int)(c)))
# endif
#endif

#define	TWO_PI		(double)(M_PI * 2.0)
#define QTR_PI		(double)(M_PI / 4.0)
#define	RADIANS(x)	(M_PI * 2.0 * (x) / 360.0)
#define DEGREES(x)      ((x) / (M_PI * 2.0) * 360.0)
#define ABS(a)          (((a) >= 0) ? (a) : -(a))
#define MAX_SHORT 	65535

#define ROUND(a) 	( (a) > 0 ? (a) + 0.5 : (a) - 0.5 )

#if defined(hp700) || defined(OS2)
#   define rint(x) ((double)((int)((x) + 0.5)))
#   define trunc(x) ((double)((int)(x)))
#endif
#if defined(solaris) || defined(sun4) || defined(aviion)
#   define trunc(x) ((double)((int)(x)))
#endif

static  double  _dxf_round();
static	void	xm_cvt_str_to_dbladdr();
static  void    xm_cvt_str_to_clock();
static	void	ClassInitialize();
static  void    Initialize();
static  void    Redisplay();
static  void    Resize();
static  Boolean	SetValues();
static  void    Destroy();
static  void    select_dial();
static  void    shading_on_dial();
static  void    shading_off_dial();
static  void    motion_dial();
static  void    validate_max();
static  void    validate_incr();
static  void    validate_per_marker();
static  void    validate_num_markers();
static  void    validate_maj_width();
static  void    validate_min_width();
static  void    validate_maj_pos();
static  void    validate_start_pos();
static  void    validate_pos();
static  void    validate_ind_width();
static	void	calc_markers();
static	void	calculate_indicator_pos();
static	void	draw_3dshadow();
static  void    draw_indicator();
static	void	draw_shading();
static	void	get_shade_GCs();
static 	double 	calculate_angle_diff();
static	double	norm_angle();

static	char	defaultTranslations[] = " <Btn1Down>: 	select() 	\n\
					<Btn1Motion>: 	select() 	\n\
					<Enter>:	shading_on() 	\n\
					<Leave>:	shading_off() 	\n\
					<Btn1Up>:	shading_on()	";
static	XtActionsRec 	actionsList[] =
	{
		{"select",	(XtActionProc) select_dial},
		{"shading_on",	(XtActionProc) shading_on_dial},
		{"shading_off",	(XtActionProc) shading_off_dial}
	};

static	double	DefaultMin = -100.0;
static	double	DefaultMax =  100.0;
static	double	DefaultInc =    1.0;
static	double	DefaultPos =    0.0;

static	XtResource	resources[] =
	{
		{
                XmNselectCallback,XmCCallback,XmRCallback,sizeof(XtPointer),
                XtOffset(XmDialWidget,dial.select),XmRCallback,NULL},

		{
		XmNminimum,XmCMin,XmRDouble,sizeof(double),
		XtOffset(XmDialWidget,dial.minimum),XmRDouble,
							(XtPointer)&DefaultMin},
		{
		XmNmaximum,XmCMax,XmRDouble,sizeof(double), 
                XtOffset(XmDialWidget,dial.maximum),XmRDouble,
                                                        (XtPointer)&DefaultMax},
		{
		XmNincrement,XmCInc,XmRDouble,sizeof(double),
                XtOffset(XmDialWidget,dial.increment),XmRDouble,
                                                        (XtPointer)&DefaultInc},
		{
		XmNdecimalPlaces,XmCDecimalPlaces,XmRInt,sizeof(int),
                XtOffset(XmDialWidget,dial.decimal_places),XmRImmediate,
                                                        (XtPointer)5},
		{
		XmNincrementsPerMarker,XmCMarkers,XmRInt,sizeof(int),
		XtOffset(XmDialWidget,dial.increments_per_marker),
						XmRString,"1"},
		{
                XmNnumMarkers,XmCMarkers,XmRInt,sizeof(int),
                XtOffset(XmDialWidget,dial.num_markers),XmRString,"30"},
		{
		XmNmajorMarkerWidth,XmCWidth,XmRDimension,sizeof(Dimension),	
		XtOffset(XmDialWidget,dial.major_marker_width),XmRString,"5"},
		{
		XmNminorMarkerWidth,XmCWidth,XmRDimension,sizeof(Dimension),   
                XtOffset(XmDialWidget,dial.minor_marker_width),XmRString,"2"},
		{
		XmNmajorPosition,XmCMarkers,XmRInt,sizeof(int),
                XtOffset(XmDialWidget,dial.major_position),XmRString,"5"},
		{
		XmNstartingMarkerPos,XmCMarkers,XmRInt,sizeof(int),
               	XtOffset(XmDialWidget,dial.starting_marker_pos),XmRString,"0"},
		{
                XmNmajorMarkerColor,XmCColor,XmRPixel,sizeof(Pixel),
            	XtOffset(XmDialWidget,dial.major_marker_color),
							XmRString,"Black"},
		{
                XmNminorMarkerColor,XmCColor,XmRPixel,sizeof(Pixel),
                XtOffset(XmDialWidget,dial.minor_marker_color),
                 	                               XmRString,"Black"},
		{
                XmNmajorMarkerThickness,XmCThickness,XmRDimension,
                                                        sizeof(Dimension),
                XtOffset(XmDialWidget,dial.major_marker_thickness),
							XmRString,"0"},
		{
                XmNminorMarkerThickness,XmCThickness,XmRDimension,
                                                        sizeof(Dimension),
                XtOffset(XmDialWidget,dial.minor_marker_thickness),
                                                	XmRString,"0"},
		{
		XmNposition,XmCPos,XmRDouble,sizeof(double),
		XtOffset(XmDialWidget,dial.position),XmRDouble,
                                                        (XtPointer)&DefaultPos},
		{
		XmNindicatorWidth,XmCWidth,XmRDimension,sizeof(Dimension),
                XtOffset(XmDialWidget,dial.indicator_width),XmRString,"0"},
		{
                XmNindicatorColor,XmCColor,XmRPixel,sizeof(Pixel),
                XtOffset(XmDialWidget,dial.indicator_color),XmRString,"Black"},
		{
		XmNshading,XmCBoolean,XmRBoolean,sizeof(Boolean),
		XtOffset(XmDialWidget,dial.shading),XmRString,"TRUE"},
		{
		XmNshadePercentShadow,XmCColor,XmRInt,sizeof(int),
		XtOffset(XmDialWidget,dial.shade_percent_shadow),
							XmRString,"15"},
		{
		XmNshadeIncreasingColor,XmCColor,XmRPixel,sizeof(Pixel),
		XtOffset(XmDialWidget,dial.shade_increasing_color),
							XmRString,"White"},
		{
		XmNshadeDecreasingColor,XmCColor,XmRPixel,sizeof(Pixel),
                XtOffset(XmDialWidget,dial.shade_decreasing_color),
                                                        XmRString,"Black"},
		{
       	      	XmNincreasingDirection,XmCClockDirection,XmRClockDirection,
								sizeof(int),
                XtOffset(XmDialWidget,dial.increasing_direction),XmRString,
								"CLOCKWISE"},
	};
	

XmDialClassRec	XmdialClassRec =
{	/* CoreClassPart */
	{
	(WidgetClass) &xmPrimitiveClassRec, /* superclass	*/
	"XmDial",			/* class_name		*/
	sizeof(XmDialRec),		/* widget_size		*/
	ClassInitialize, 		/* class_initialize	*/
	NULL,				/* class_part_initialize */
	FALSE,				/* class_inited		*/
	Initialize,			/* initialize		*/
	NULL,				/* initialize_hook	*/
	XtInheritRealize,		/* realize		*/
	actionsList,			/* actions		*/
	XtNumber(actionsList),		/* num_actions		*/
	resources,			/* resources		*/
	XtNumber(resources),		/* num_resources	*/
	NULLQUARK,			/* xrm_class		*/
	TRUE,				/* compress_motion	*/
	TRUE,				/* compress_exposure	*/
	TRUE,       			/* compress_enterleave 	*/
	FALSE,       			/* visible_interest	*/
	Destroy,			/* destroy		*/
	Resize,				/* resize		*/
	Redisplay,			/* expose		*/
	SetValues,			/* set_values		*/
	NULL,				/* set_values_hook	*/
	XtInheritSetValuesAlmost,	/* set_values_almost	*/
	NULL,				/* get_values_hook	*/
	NULL,				/* accept_focus		*/
	XtVersion,			/* version		*/
	NULL,				/* callback private	*/
	defaultTranslations,		/* tm_table		*/
	NULL,				/* query_geometry	*/
        NULL,                           /* display_accelerator  */
        NULL,                           /* extension		*/
	},
	/* Primitive class fields				*/
	{
	(XtWidgetProc)_XtInherit,		/* border_highlight	*/
	(XtWidgetProc)_XtInherit,		/* border_unhighlight	*/
#if (XmVersion >= 1001)
      XtInheritTranslations,     		/* translations           */
#else
      (XtTranslations)_XtInherit,     		/* translations           */
#endif
#if !defined(ibm6000) || XmVersion >= 1002
	(XtActionProc)_XtInherit,		/* arm and activate	*/
#else
	(XmArmAndActivate)_XtInherit,		/* arm and activate	*/
#endif
	NULL,				/* get_resources	*/
	0, 				/* num_get_resources	*/
	NULL				/* extension		*/
	},
	/* Dial class fields */
	{
	NULL,				/* ignore		*/
	}
};

WidgetClass	xmDialWidgetClass = (WidgetClass) &XmdialClassRec;

static	void	ClassInitialize(wc)
WidgetClass	wc;
	{
	/*
         * Add the string-to-address-of-double type converter
	 *     and string-to-clock-direction type converter.
         */
	XtAddConverter(XmRString,XmRDouble,xm_cvt_str_to_dbladdr,NULL,0);
	XtAddConverter(XmRString,XmRClockDirection,xm_cvt_str_to_clock,NULL,0);
	}

static	void	Initialize(request,new)
XmDialWidget	request,new;
	{
	XGCValues	values;
	XtGCMask	valueMask;

	/*
	 * Make sure the window size is not zero.  The Core Initialize()
	 * and Primitive Initialize() methods don't do this.
	 */
	if (request->core.width == 0)		
		new->core.width = 150;
	if (request->core.height == 0)
		new->core.height = 150;

	new->dial.minimum = 
			_dxf_round(new->dial.minimum, new->dial.decimal_places);
	new->dial.maximum = 
			_dxf_round(new->dial.maximum, new->dial.decimal_places);
	new->dial.increment = 
			_dxf_round(new->dial.increment, new->dial.decimal_places);
	new->dial.position = 
			_dxf_round(new->dial.position, new->dial.decimal_places);

	/*
	 * Validate resources.
	 */
	validate_max(new,new->dial.maximum);
	validate_incr(new,new->dial.increment);
	validate_num_markers(new,new->dial.num_markers);
	validate_maj_width(new,new->dial.major_marker_width);
	validate_min_width(new,new->dial.minor_marker_width);
	validate_maj_pos(new,new->dial.major_position);
	validate_start_pos(new,new->dial.starting_marker_pos);
	validate_pos(new,new->dial.position);
	validate_ind_width(new,new->dial.indicator_width);


	/*
	 * Create the graphics contexts used for the dial face markers
	 * and the indicator.
	 */
	valueMask = GCForeground | GCBackground | GCLineWidth;
	values.foreground = new->dial.major_marker_color;
	values.line_width = new->dial.major_marker_thickness;
	values.background = new->core.background_pixel;
	new->dial.major_marker_GC = XtGetGC((Widget)new,valueMask,&values);

	valueMask = GCForeground | GCBackground | GCLineWidth;
        values.foreground = new->dial.minor_marker_color;
	values.line_width = new->dial.minor_marker_thickness;
        new->dial.minor_marker_GC = XtGetGC((Widget)new,valueMask,&values);

	valueMask = GCForeground | GCBackground;
	values.foreground = new->dial.indicator_color;
	new->dial.indicator_GC = XtGetGC((Widget)new,valueMask,&values);

	values.foreground = new->core.background_pixel;
	values.background = new->dial.indicator_color;
	new->dial.inverse_GC = XtGetGC((Widget)new,valueMask,&values);

	/*
	 * Allocate the shading GC.
	 */
	valueMask = GCForeground;
        values.foreground = new->dial.shade_increasing_color;
        new->dial.shade_incr_GC =  XtGetGC((Widget)new,valueMask,&values);

        values.foreground = new->dial.shade_decreasing_color;
        new->dial.shade_decr_GC =  XtGetGC((Widget)new,valueMask,&values);

	/*
	 * Allocate the default shading GC's.
	 */
	if (new->dial.shade_percent_shadow == 0)
		{
		valueMask = GCForeground;
		values.foreground = new->dial.shade_increasing_color;
		new->dial.shade_def_incr_GC = XtGetGC((Widget)new,valueMask,&values);

		values.foreground = new->dial.shade_decreasing_color;
		new->dial.shade_def_decr_GC = XtGetGC((Widget)new,valueMask,&values);
		}
	else
		get_shade_GCs(new);

	/*
	 * Initialize number of "increments" per revolution.
	 */
	new->dial.increments_per_rev = (new->dial.num_markers
					* new->dial.increments_per_marker);

	/*
	 * Initialize starting_marker_angle.
	 */
	new->dial.starting_marker_angle = 
			RADIANS(new->dial.starting_marker_pos);

	Resize(new);
	}

static	void	Resize(w)
XmDialWidget    w;
        {

	/*
	 * Calculate the center of the widget.
	 */
	w->dial.center_x = w->core.width/2;
	w->dial.center_y = w->core.height/2;

	/*
	 * Calculate marker positions and generate segment array.
	 */
	calc_markers(w);

	/*
	 * Calculate the indicator position.
	 */
	calculate_indicator_pos(w);
	}

static	void	Redisplay(w,event,region)
XmDialWidget	w;
XEvent		*event;
Region		region;
	{
	/*
	 * Draw the arcs for 3d effect.
	 */
	if (w->primitive.shadow_thickness > 0)
		draw_3dshadow(w);

	/*
	 * Draw the major and minor markers used for the dial face.
	 */
	XDrawSegments(XtDisplay(w),XtWindow(w),
		w->dial.major_marker_GC,
		(XSegment*)w->dial.major_segments,
		w->dial.num_major_markers);

	if (w->dial.major_position != 1)
		XDrawSegments(XtDisplay(w),XtWindow(w),
                       	w->dial.minor_marker_GC,
                       	(XSegment*)w->dial.minor_segments,
                       	w->dial.num_minor_markers);

	/* 
	 * Draw the indicator at its current position.
	 */
	draw_indicator(w,w->dial.indicator_GC);
	}

static	Boolean	SetValues(current,request,new)
XmDialWidget	current,request,new;
	{
	XGCValues	values;
	XtGCMask	valueMask;
	Boolean		redraw = FALSE;
	Boolean		redraw_indicator = FALSE;
	Boolean		recalc_indicator = FALSE;
	Boolean         recalc_markers = FALSE;

	new->dial.minimum = 
			_dxf_round(new->dial.minimum, new->dial.decimal_places);
	new->dial.maximum = 
			_dxf_round(new->dial.maximum, new->dial.decimal_places);
	new->dial.increment = 
			_dxf_round(new->dial.increment, new->dial.decimal_places);
	new->dial.position = 
			_dxf_round(new->dial.position, new->dial.decimal_places);

	/*
         * Initialize the four resource variables that have addresses
	 * 	passed instead of values.  If they have changed,	
         *      then obtain the new values and reset the addresses.  
	 */
	if (new->dial.minimum != current->dial.minimum)
		{
		recalc_indicator = TRUE;
		redraw_indicator = TRUE;
		}
	if (new->dial.maximum != current->dial.maximum)
                {
		validate_max(new,new->dial.maximum);
		recalc_indicator = TRUE;
                redraw_indicator = TRUE;
		}
	if (new->dial.position != current->dial.position)
                {
		validate_pos(new,new->dial.position);
		recalc_indicator = TRUE;
                redraw_indicator = TRUE;
		}
	if (new->dial.increment != current->dial.increment)
		{
		validate_incr(new,new->dial.increment);
		recalc_indicator = TRUE;
                redraw_indicator = TRUE;
		}

	/*
	 * Recalculate the increments_per_rev private variable.
	 */
	if ((new->dial.increments_per_marker != 
				current->dial.increments_per_marker) ||	
	    (new->dial.num_markers != current->dial.num_markers))
		{
		validate_per_marker(new,new->dial.increments_per_marker);
		validate_num_markers(new,new->dial.num_markers);
		new->dial.increments_per_rev = (new->dial.num_markers
                                        * new->dial.increments_per_marker);
		recalc_indicator = TRUE;
		redraw_indicator = TRUE;
		}

	/*
	 * Recalculate the marker positions and redraw.
	 */
	if ((new->dial.num_markers != current->dial.num_markers) ||
	    (new->dial.major_marker_width != 
				current->dial.major_marker_width) ||
	    (new->dial.minor_marker_width != 
                                current->dial.minor_marker_width) ||
	    (new->dial.major_position != current->dial.major_position))
		{
		validate_maj_width(new,new->dial.major_marker_width);
		validate_min_width(new,new->dial.minor_marker_width);
		validate_maj_pos(new,new->dial.major_position);
		recalc_markers = TRUE;
		recalc_indicator = TRUE;
		redraw = TRUE;
		}
 
	/*
	 * Recalculate the starting_marker_angle in radians.	
	 */
	if (new->dial.starting_marker_pos != current->dial.starting_marker_pos)
		{
		validate_start_pos(new,new->dial.starting_marker_pos);
		new->dial.starting_marker_angle =
                        RADIANS(new->dial.starting_marker_pos);
		recalc_markers = TRUE;
		recalc_indicator = TRUE;
		redraw = TRUE;
		}

	/*
	 * If the shadow thickness has been changed, recalculate the 
	 * marker positions and redraw.
	 */
	if (new->primitive.shadow_thickness 
				!= current->primitive.shadow_thickness)
		{
		recalc_markers = TRUE;
		recalc_indicator = TRUE;
		redraw = TRUE;
                }

	/*
         * If the major marker color or thickness has changed, 
	 * regenerate the GC.
         */
        if ((new->dial.major_marker_color != 
					current->dial.major_marker_color) ||
	    (new->dial.major_marker_thickness !=
					current->dial.major_marker_thickness))
		{
                XtReleaseGC((Widget)new,new->dial.major_marker_GC);
                valueMask = GCForeground | GCBackground | GCLineWidth;
                values.foreground = new->dial.major_marker_color;
		values.line_width = new->dial.major_marker_thickness;
                values.background = new->core.background_pixel;
                new->dial.major_marker_GC = XtGetGC((Widget)new,valueMask,&values);

                redraw = TRUE;
                }

	/*
         * If the minor marker color or thickness has changed, 
         * regenerate the GC.
         */
        if ((new->dial.minor_marker_color != 
                                        current->dial.minor_marker_color) ||
            (new->dial.minor_marker_thickness !=
                                        current->dial.minor_marker_thickness))
                {
                XtReleaseGC((Widget)new,new->dial.minor_marker_GC);
                valueMask = GCForeground | GCBackground | GCLineWidth;
                values.foreground = new->dial.minor_marker_color;
                values.line_width = new->dial.minor_marker_thickness;
                values.background = new->core.background_pixel;
                new->dial.minor_marker_GC = XtGetGC((Widget)new,valueMask,&values);

                redraw = TRUE;
                }

	/*
	 * If the indicator color or background color has changed,
	 *     regenerate GC's.
	 */
	if ((new->dial.indicator_color != current->dial.indicator_color) ||
	    (new->core.background_pixel != current->core.background_pixel))
		{
		validate_ind_width(new,new->dial.indicator_width);
		XtReleaseGC((Widget)new,new->dial.indicator_GC);
		valueMask = GCForeground | GCBackground;
		values.foreground = new->dial.indicator_color;
		values.background = new->core.background_pixel;
		new->dial.indicator_GC = XtGetGC((Widget)new,valueMask,&values);

		XtReleaseGC((Widget)new,new->dial.inverse_GC);
		values.foreground = new->core.background_pixel;
		values.background = new->dial.indicator_color;
		new->dial.inverse_GC = XtGetGC((Widget)new,valueMask,&values);

		redraw = TRUE;
		}

	/*
	 * If the indicator width has changed, recalc & redraw the indicator.
	 */
	if (new->dial.indicator_width != current->dial.indicator_width)
		{
		recalc_indicator = TRUE;
                redraw_indicator = TRUE;
		}

	/*
	 * If the shading color has changed, reallocate the GC.
	 */
	if (new->dial.shade_increasing_color != 
					current->dial.shade_increasing_color)
		{
		XtReleaseGC((Widget)new,new->dial.shade_incr_GC);
		valueMask = GCForeground;
                values.foreground = new->dial.shade_increasing_color;
                new->dial.shade_incr_GC =  XtGetGC((Widget)new,valueMask,&values);
                
		redraw = TRUE;
		}
	if (new->dial.shade_decreasing_color != 
                                        current->dial.shade_decreasing_color)
                {
                XtReleaseGC((Widget)new,new->dial.shade_decr_GC);
                valueMask = GCForeground;
                values.foreground = new->dial.shade_decreasing_color;
                new->dial.shade_decr_GC =  XtGetGC((Widget)new,valueMask,&values);

                redraw = TRUE;
                }

	/* 
	 * If the background color or shade percent has changed, reallocate
	 * the GC's.
	 */
	if ((new->core.background_pixel != current->core.background_pixel) ||
	    (new->dial.shade_percent_shadow != 
					current->dial.shade_percent_shadow))
		{
		XtReleaseGC((Widget)new,new->dial.shade_def_incr_GC);
		XtReleaseGC((Widget)new,new->dial.shade_def_decr_GC);
		get_shade_GCs(new);
		}
		
        /*
         * If increasing_direction has changed, recalc indicator pos * redraw.
         */
        if (new->dial.increasing_direction !=
                        current->dial.increasing_direction)
                {
                recalc_indicator = TRUE;
                redraw_indicator = TRUE;
                }

	/*
	 * Recalculate the Dial face markers, if required.
	 */
	if (recalc_markers)
		calc_markers(new);

	/*
	 * Recalculate the indicator position, if required.
	 */
	if (recalc_indicator)
		calculate_indicator_pos(new);

	/*
 	 * If only the indicator needs to be redrawn and the widget is
	 * realized, erase the current indicator and draw the new one.
	 */
	if (redraw_indicator && ! redraw && XtIsRealized((Widget)new)
			&& new->core.visible)
		{
		draw_indicator(current,current->dial.inverse_GC);
		draw_indicator(new,new->dial.indicator_GC);
	 	}
	return redraw;
	}

static  void    Destroy(w)
XmDialWidget    w;
        {
	/*
	 * Release all GC's.
	 */
	XtReleaseGC((Widget)w,w->dial.major_marker_GC);
	XtReleaseGC((Widget)w,w->dial.minor_marker_GC);
        XtReleaseGC((Widget)w,w->dial.indicator_GC);
        XtReleaseGC((Widget)w,w->dial.inverse_GC);
       	XtReleaseGC((Widget)w,w->dial.shade_incr_GC);
	XtReleaseGC((Widget)w,w->dial.shade_decr_GC);
	XtReleaseGC((Widget)w,w->dial.shade_def_incr_GC);
	XtReleaseGC((Widget)w,w->dial.shade_def_decr_GC);

	/*
	 * Remove Callbacks.
	 */
        XtRemoveAllCallbacks((Widget)w,XmNselectCallback);
        }

static	void	select_dial(w,event,args,n_args)
XmDialWidget	w;
XEvent		*event;
char		*args[];
int		n_args;
	{
	double 		pos,angle,delta_angle,delta_increment;

	XmDialCallbackStruct	cb;

	/*
	 * Initialize "pos" - the value sent to the user application 
	 * in the callback - to the current indicator position.
	 */
	pos = w->dial.position;

	if (event->type == ButtonPress || event->type == MotionNotify)
		{
		/*
	 	 * Turn off shading if it is active.
		 */
		if (w->dial.shading_active)
			shading_off_dial(w,NULL,NULL,0);

		/*
	 	 * Get the angle in radians.
	 	 */
		angle = atan2((double)(event->xbutton.y - w->dial.center_y),
			      (double)(event->xbutton.x - w->dial.center_x));
		angle += (M_PI / 2);
		angle = norm_angle(angle);

		/*
		 * Calculate the delta from the previous position in radians.
		 */
		delta_angle = calculate_angle_diff(angle,w->dial.prev_angle);

		/*
		 * Calculate the equivalent change in value, in increments,
		 * of the change in the indicator angle. 
		 */
		delta_increment = w->dial.increments_per_rev *
				w->dial.increment *
				(delta_angle / TWO_PI);

		/*
		 * Add or subtract the change in value to the current 
		 * value of the indicator, depending upon whether
		 * the indicator is oriented Clock or CounterClockwise.
		 */
		if (w->dial.increasing_direction == COUNTERCLOCKWISE)
			pos = w->dial.position - delta_increment;
		else
			pos = w->dial.position + delta_increment;

		/*
		 * Round the new indicator position to the nearest 
		 * valid indicator value.  
		 */
		if(w->dial.increment != 0)
#ifndef ibm6000
		    pos = w->dial.increment * rint(pos / w->dial.increment);
#else
		    pos = w->dial.increment * nearest(pos / w->dial.increment);
#endif
		else
		    pos = 0;
		
		/*
		 * The indicator position must remain within the maximum
		 * and minimum bounds.
		 */
		if (pos < w->dial.minimum)
			pos = w->dial.minimum;
		if (pos > w->dial.maximum)
			pos = w->dial.maximum;

		/*
 	         * Recalculate indicator position and redraw indicator.
		 */
		draw_indicator(w,w->dial.inverse_GC);
		w->dial.position = pos;
                calculate_indicator_pos(w);
                draw_indicator(w,w->dial.indicator_GC);
		}

	/*
	 * Invoke the callback, report the address of the position
	 * in the call_data structure.
	 */
	if (event->type == ButtonRelease)
	    cb.reason   = XmCR_ACTIVATE; 
	else
	    cb.reason   = XmCR_DRAG; 
	cb.event    = event;
	cb.position = pos;
	XtCallCallbacks((Widget)w,XmNselectCallback,&cb);
	}

static  void    shading_on_dial(w,event,args,n_args)
XmDialWidget    w;
XEvent          *event;
char            *args[];
int             n_args;
	{
	/*
	 * If shading is desired, add an event handler to track PointerMotion
	 * and turn on the Boolean to indicate shading is currently active.
	 */
	if (w->dial.shading)
		{
		XtAddEventHandler((Widget)w,PointerMotionMask,FALSE,motion_dial,NULL);
		w->dial.shading_active = TRUE;
		}
        if (event->type == ButtonRelease)
                {
                select_dial(w,event,args,n_args);
                }
	}

static  void    shading_off_dial(w,event,args,n_args)
XmDialWidget    w;
XEvent          *event;
char            *args[];
int             n_args;
        {
	/*
	 * If shading is currently active, remove the PointerMotion event
	 * event handler, erase the current shading, and turn off the 
	 * shading_active boolean.
	 */
	if (w->dial.shading_active)
		{
		XtRemoveEventHandler((Widget)w,PointerMotionMask,FALSE,
			motion_dial,NULL);
		draw_shading(w,w->dial.inverse_GC);
		draw_indicator(w,w->dial.indicator_GC);
		w->dial.shading_active = FALSE;
		}
        }

static	void	motion_dial(w,client_data,event)
XmDialWidget    w;
XtPointer		client_data;
XEvent          *event;
        {
	double	angle1,angle2,delta_angle,rounded_value;
	Sign	before_rounding = POSITIVE;
	Sign	after_rounding = POSITIVE;
	int	delta_degree;
	
	/*
	 * "angle1" is the current position of the indicator in radians 
	 * oriented from the 3 o'clock position in a counterclockwise 
	 * direction.  "angle2" is the current position of the mouse pointer
	 * with the same orientation.  This orientation is required to do
	 * the XFillArc.
	 */
	angle1 = w->dial.indicator_angle - (M_PI / 2);
	angle1 = norm_angle(TWO_PI - angle1);

	angle2 = atan2((double)(event->xbutton.y - w->dial.center_y),
                       (double)(event->xbutton.x - w->dial.center_x));
	angle2 = norm_angle(TWO_PI - angle2);


	/*
	 * Calculate the difference between the two angles.  Then round 
	 * the difference so that it is equivalent to the nearest valid 
	 * "increment" value.  If the sign of the difference changes
	 * because of the rounding, then the pointer must be straddling
	 * the 180 degree mark from indicator - change the direction of 
	 * the angle.
	 */
	delta_angle = calculate_angle_diff(angle2,angle1);
	if (delta_angle < 0.0)
		before_rounding = NEGATIVE;
#ifndef ibm6000
	rounded_value = rint((double)w->dial.increments_per_rev *
				delta_angle / TWO_PI);
#else
	rounded_value = nearest((double)w->dial.increments_per_rev *
				delta_angle / TWO_PI);
#endif
	delta_angle = rounded_value * TWO_PI / w->dial.increments_per_rev;
	if (delta_angle < 0.0)
                after_rounding = NEGATIVE;
	if (before_rounding != after_rounding)
                delta_angle = -delta_angle;

	/*
	 * Convert the angle difference to degrees*64 so that it is 
	 * usable by XFillArc.  If the shading will now be different, 
	 * save the values used by XFillArc in private variables, erase 
	 * the previous shading, draw the new shading and redraw the indicator.
	 */
	delta_degree = 64 * DEGREES(delta_angle);
	if (delta_degree != w->dial.shading_angle2)
		{
		if (w->dial.shading_active)
			draw_shading(w,w->dial.inverse_GC);
		w->dial.shading_angle1 = 64 * DEGREES(angle1);
		w->dial.shading_angle2 = delta_degree;
		if (w->dial.shade_percent_shadow != 0)
		       if (((delta_degree < 0) && 
			    (w->dial.increasing_direction == CLOCKWISE)) ||
			   ((delta_degree > 0) &&
			    (w->dial.increasing_direction == COUNTERCLOCKWISE)))
				draw_shading(w,w->dial.shade_def_incr_GC);
			else
				draw_shading(w,w->dial.shade_def_decr_GC);
		else
			if (((delta_degree < 0) && 
                            (w->dial.increasing_direction == CLOCKWISE)) ||
                           ((delta_degree > 0) &&
                            (w->dial.increasing_direction == COUNTERCLOCKWISE)))
                                draw_shading(w,w->dial.shade_incr_GC);
			else
				draw_shading(w,w->dial.shade_decr_GC);
		draw_indicator(w,w->dial.indicator_GC);
		}
	}

static	void validate_max(w,max)
XmDialWidget    w;
double		max;
	{
	if (max < w->dial.minimum)
		{
                XtWarning("XmNmaximum must be greater than XmNminimum.");
                w->dial.maximum = w->dial.minimum + w->dial.increment;
		}
	}
	
static  void validate_incr(w,incr)
XmDialWidget    w;
double		incr;
	{
	if (incr > (w->dial.maximum - w->dial.minimum))
		{
		XtWarning("XmNincrement is greater than Max to Min range.");
		w->dial.increment = w->dial.maximum - w->dial.minimum;
		}
	}

static  void validate_per_marker(w,per_marker)
XmDialWidget    w;
int		per_marker;
	{
	if (per_marker < 1)
		{
		XtWarning("XmNincrementsPerMarker must be greater than zero.");
		w->dial.increments_per_marker = 1;
		}
	}

static  void validate_num_markers(w,num_markers)
XmDialWidget    w;
int		num_markers;
	{
	if (num_markers < 1)
		{
		XtWarning("XmNnumMarkers must be greater than zero.");
		w->dial.num_markers = 1;
		}
        if (num_markers > MAXSEGMENTS / 2)
                {
                XtWarning("XmNnumMarkers: Too many markers.");
                w->dial.num_markers = MAXSEGMENTS / 2;
                }
	}

static  void validate_maj_width(w,maj_width)
XmDialWidget    w;
int		maj_width;
	{
	if (maj_width < 1)
		{
                XtWarning("XmNmajorMarkerWidth must be greater than zero.");
		w->dial.major_marker_width = 1;
		}
	}

static  void validate_min_width(w,min_width)
XmDialWidget    w;
int		min_width;
	{
	if (min_width < 1)
		{
		XtWarning("XmNminorMarkerWidth must be greater than zero.");
		w->dial.minor_marker_width = 1;
		}
	if (min_width > (int)w->dial.major_marker_width)
		{
		XtWarning("XmNminorMarkerWidth must not be greater than "
			  "XmNmajorMarkerWidth.");
		w->dial.minor_marker_width = w->dial.major_marker_width;
		}
	}

static  void validate_maj_pos(w,maj_pos)
XmDialWidget    w;
int		maj_pos;
	{
	if (maj_pos < 1)
		{
		XtWarning("XmNmajorPosition must be greater than zero.");
		w->dial.major_position = 1;
		}
	}

static  void validate_start_pos(w,start_pos)
XmDialWidget    w;
int		start_pos;
	{
	while (start_pos < 0)
		start_pos += 360;
	while (start_pos >= 360)
		start_pos -=360;
	w->dial.starting_marker_pos = start_pos;
	}

static  void validate_pos(w,pos)
XmDialWidget    w;
double		pos;
	{
	if (pos < w->dial.minimum)
		{
		XtWarning("XmNposition is less than XmNminimum.");
		w->dial.position = w->dial.minimum;
		}
	if (pos > w->dial.maximum)
		{
                XtWarning("XmNposition is greater than XmNmaximum.");
		w->dial.position = w->dial.maximum;
		}
	}

static  void validate_ind_width(w,ind_width)
XmDialWidget    w;
int             ind_width;
        {
        if (ind_width < 0)
                {
                XtWarning("XmNindicatorWidth must not be less than zero.");
                w->dial.indicator_width = 0;
                }
        }

static	void calc_markers(w)
XmDialWidget    w;
        {
        double  angle,cosine,sine,incr;
        int     i,j;
        XPoint  *major_ptr;
	XPoint	*minor_ptr;
	/*
         * Generate the segment arrays containing the face of the dial.
         */

        /*
         * Get the address of the first line segments.
         */
        major_ptr = w->dial.major_segments;
	minor_ptr = w->dial.minor_segments;

	/*
 	 * Initialize marker counts.
	 */
	w->dial.num_major_markers = 0;
	w->dial.num_minor_markers = 0;

	/*
	 * Determine the increment value between markers in radians.
	 */
        incr = TWO_PI /(double)w->dial.num_markers;
        
	/*
	 * Determine the radii of the circle formed by the outsides of 
	 * the major and minor markers and the inside of the markers.
	 * The actual marker widths are a percentage of the size of 
	 * the widget.
	 */
	w->dial.outer_diam = ((int)MIN(w->core.width,w->core.height) / 2)
				- (2 * w->primitive.shadow_thickness) - 2;
#ifndef ibm6000
	w->dial.major_width = (int) rint((double)w->dial.outer_diam
					  * (double)w->dial.major_marker_width
					  / 100.0);
#else
	w->dial.major_width = (int) nearest((double)w->dial.outer_diam
					  * (double)w->dial.major_marker_width
					  / 100.0);
#endif

#ifndef ibm6000
	w->dial.minor_width = (int) rint((double)w->dial.outer_diam
					  * (double)w->dial.minor_marker_width
                                          / 100.0);
#else
	w->dial.minor_width = (int) nearest((double)w->dial.outer_diam
					  * (double)w->dial.minor_marker_width
                                          / 100.0);
#endif
        w->dial.inner_diam =
                        w->dial.outer_diam - w->dial.major_width;
        w->dial.minor_diam =
                        w->dial.inner_diam + w->dial.minor_width;

	/*
	 * Starting at the "starting_marker_angle", calculate the xy
	 * coordinates of each end of the major and minor markers.
	 */
        angle = (double)w->dial.starting_marker_angle;
        for (j=0; j < w->dial.num_markers; j+=w->dial.major_position)
                {
                cosine = cos(angle);
                sine   = sin(angle);
                major_ptr->x = w->dial.center_x + w->dial.outer_diam * sine;
                major_ptr->y = w->dial.center_y - w->dial.outer_diam * cosine;
                major_ptr++;
                major_ptr->x = w->dial.center_x + w->dial.inner_diam * sine;
                major_ptr->y = w->dial.center_y - w->dial.inner_diam * cosine;
                major_ptr++;
		w->dial.num_major_markers++;
                angle += incr;
 	        for (i=j+1; i < j+w->dial.major_position; i++)
			{
                        if (i >= w->dial.num_markers)
                       	        break;
                        cosine = cos(angle);
                        sine   = sin(angle);
                        minor_ptr->x = w->dial.center_x
                                        + w->dial.minor_diam * sine;
                        minor_ptr->y = w->dial.center_y
                                        - w->dial.minor_diam * cosine;
                        minor_ptr++;
                        minor_ptr->x = w->dial.center_x
                                        + w->dial.inner_diam * sine;
                        minor_ptr->y = w->dial.center_y
                                        - w->dial.inner_diam * cosine;
                        minor_ptr++;
			w->dial.num_minor_markers++;
                        angle += incr;
                        }
                }
 	}

static  void calculate_indicator_pos(w)
XmDialWidget    w;
        {
        double          angle;
        int             nbr_increments;
        Position        indicator_length;
	int		needle_width;

        /*
         * Make the indicator two pixels shorter than the inner edge
         * of the markers.
         */
        indicator_length = w->dial.inner_diam - 2;

        /*
         * Find the number of increments that the indicator should be
         * pointing to.  Convert the number to equivalent radians and
         * then add the starting angle.
         */
	if(w->dial.increment != 0)
#ifndef ibm6000
	    nbr_increments = (int) rint(w->dial.position / w->dial.increment)
		    % w->dial.increments_per_rev;
#else
	    nbr_increments = (int) nearest(w->dial.position / w->dial.increment)
		    % w->dial.increments_per_rev;
#endif
	else
	    nbr_increments = 0;
	if (w->dial.increasing_direction == COUNTERCLOCKWISE)
		nbr_increments = -nbr_increments;
        angle = TWO_PI * ((double)nbr_increments /
                                (double)w->dial.increments_per_rev);
        angle += w->dial.starting_marker_angle;
        angle = norm_angle(angle);
        w->dial.indicator_angle = angle;
        w->dial.prev_angle = angle;

        /*
         * Find the x,y coordinates for the indicator polygon.
         */
	needle_width = (float)((int)indicator_length * (int)w->dial.indicator_width) /
				(float)(100.0 * sqrt(2.0));
	w->dial.indicator[0].x = w->dial.center_x;
	w->dial.indicator[0].y = w->dial.center_y;
	w->dial.indicator[1].x = w->dial.center_x + 
			needle_width *
			sin(angle - QTR_PI);
	w->dial.indicator[1].y = w->dial.center_y - 
			needle_width *
			cos(angle - QTR_PI);
        w->dial.indicator[2].x = w->dial.center_x + 
			indicator_length * sin(angle);
        w->dial.indicator[2].y = w->dial.center_y - 
			indicator_length * cos(angle);
	w->dial.indicator[3].x = w->dial.center_x + 
                        needle_width *
                        sin(angle + QTR_PI);
        w->dial.indicator[3].y = w->dial.center_y - 
                        needle_width *
                        cos(angle + QTR_PI);
        }

static	void	draw_3dshadow(w)
XmDialWidget	w;
	{
	int		x,y,face_diam,shadow_face_diam;

        face_diam = MIN(w->core.width,w->core.height) -
					(4 * w->primitive.shadow_thickness);
        shadow_face_diam = face_diam +  (2 * w->primitive.shadow_thickness);
	
	x = y = 0;
	if (w->core.width > w->core.height)
		x = (int)(w->core.width - w->core.height) / 2;
	if (w->core.width < w->core.height)
		y = (int)(w->core.height - w->core.width) / 2;

	XFillArc(XtDisplay(w),XtWindow(w),w->primitive.top_shadow_GC,
                x,y,
                shadow_face_diam,shadow_face_diam,
                0,360*64);

        XFillArc(XtDisplay(w),XtWindow(w),w->primitive.bottom_shadow_GC,
                x + (2 * w->primitive.shadow_thickness),
                y + (2 * w->primitive.shadow_thickness),
                shadow_face_diam,shadow_face_diam,
                0,360*64);

        XFillArc(XtDisplay(w),XtWindow(w),w->dial.inverse_GC,
		x + (2 * w->primitive.shadow_thickness),
                y + (2 * w->primitive.shadow_thickness),
                face_diam,face_diam,
                0,360*64);
	}

static  void    draw_indicator(w,gc)
XmDialWidget    w;
GC              gc;
        {
	/*
	 * Draw the indicator at its current position.
	 */
	if (w->dial.indicator_width < 2)
		XDrawLine(XtDisplay(w),XtWindow(w),gc,
			w->dial.indicator[0].x,
			w->dial.indicator[0].y,
			w->dial.indicator[2].x,
			w->dial.indicator[2].y);
	else
        	XFillPolygon(XtDisplay(w),XtWindow(w),gc,
                        w->dial.indicator,4,Convex,CoordModeOrigin);
        }

static	void draw_shading(w,gc)
XmDialWidget    w;
GC              gc;
        {
	/*
	 * Draw the filled arc for shading between the current 
	 * indicator position and the current mouse pointer position.
	 */
	XFillArc(XtDisplay(w),XtWindow(w),gc,
                        w->dial.center_x - w->dial.inner_diam + 2,
                        w->dial.center_y - w->dial.inner_diam + 2,
                        2 * w->dial.inner_diam - 4,
                        2 * w->dial.inner_diam - 4,
                        w->dial.shading_angle1,
                        w->dial.shading_angle2);

	}

static	void	get_shade_GCs(w)
XmDialWidget	w;
	{
	float		fcolor;
	XColor		bg_color;
	XColor		ts_color;
	XColor		bs_color;
	XGCValues	values;
	XtGCMask	valueMask;

	/*
         * Create the shade default increasing & decreasing GC's as a
         * percentage of the top & bottom shadow colors.
         */
        bg_color.pixel = w->core.background_pixel;
        XQueryColor(XtDisplay(w),
        		DefaultColormapOfScreen(XtScreen(w)),&bg_color);
		/* red		*/
	fcolor = (float)bg_color.red;
	fcolor += fcolor * (float)w->dial.shade_percent_shadow * .004;
	if (fcolor > MAX_SHORT)
		ts_color.red   = MAX_SHORT;
	else
		ts_color.red   = fcolor;
		/* green	*/
	fcolor = (float)bg_color.green;
        fcolor += fcolor * (float)w->dial.shade_percent_shadow * .004;
        if (fcolor > MAX_SHORT)
                ts_color.green = MAX_SHORT;
        else
                ts_color.green = fcolor;
		/* blue		*/
	fcolor = (float)bg_color.blue;
        fcolor += fcolor * (float)w->dial.shade_percent_shadow * .004;
        if (fcolor > MAX_SHORT)
                ts_color.blue  = MAX_SHORT;
        else
                ts_color.blue  = fcolor;
	if (XAllocColor(XtDisplay(w),
			DefaultColormapOfScreen(XtScreen(w)),&ts_color) == 0)
		ts_color.pixel = WhitePixelOfScreen(XtScreen(w));
	valueMask = GCForeground;
	values.foreground = ts_color.pixel;
	w->dial.shade_def_incr_GC = XtGetGC((Widget)w,valueMask,&values);

		/* red		*/
	fcolor = (float)bg_color.red;
	fcolor -= fcolor * (float)w->dial.shade_percent_shadow * .0045;
	bs_color.red   = fcolor;
		/* green	*/
	fcolor = (float)bg_color.green;
        fcolor -= fcolor * (float)w->dial.shade_percent_shadow * .0045;
        bs_color.green = fcolor;
		/* blue		*/
	fcolor = (float)bg_color.blue;
        fcolor -= fcolor * (float)w->dial.shade_percent_shadow * .0045;
        bs_color.blue  = fcolor;
        if (XAllocColor(XtDisplay(w),
                        DefaultColormapOfScreen(XtScreen(w)),&bs_color) == 0)
                ts_color.pixel = BlackPixelOfScreen(XtScreen(w));
	valueMask = GCForeground;
        values.foreground = bs_color.pixel;
        w->dial.shade_def_decr_GC = XtGetGC((Widget)w,valueMask,&values);
	}

static double calculate_angle_diff(angle1,angle2)
double  angle1,angle2;
        {
        double  diff_angle;

	/*
	 * Calculate the difference between two angles specified as
	 * double_precision in radians. "Normalize" the value to 
	 * within the -PI to +PI range.
	 */ 
        diff_angle = angle1 - angle2;
        while (diff_angle < -M_PI)
                diff_angle += TWO_PI;
        while (diff_angle > M_PI)
                diff_angle -= TWO_PI;
        return diff_angle;
        }

static double norm_angle(angle)
double  angle;
        {
	/*
	 * "Normalize" an angle specified as double_precision in radians 
	 * to within the 0 to 2PI range.
	 */
        while (angle < 0.0)
                angle += TWO_PI;
        while (angle > TWO_PI)
                angle -= TWO_PI;
        return angle;
        }

Widget XmCreateDial(parent,name,args,num_args)
Widget     parent;
String     name;
ArgList    args;
Cardinal   num_args;
	{
	/*
	 * Convenience routine to create Dial Widget.
	 */
    	return XtCreateWidget(name,xmDialWidgetClass,parent,args,num_args);
	}

/**************************************************
 * str2dba.c: Convert a string to a double address.
 **************************************************/

static void	xm_cvt_str_to_dbladdr(args,nargs,fromVal,toVal)
XrmValue	*args;
Cardinal	*nargs;
XrmValue  	*fromVal, *toVal;
 	{
	static double	result;

	/*
	 * Make sure the number of args is correct.
	 */
	if (*nargs!=0)
		XtWarning("String to Double conversion needs no arguments.");

	/*
	 * Convert the string in fromVal to a double precision 
	 * floating point address.
	 */
	if (sscanf((char *)fromVal->addr,"%lf",&result) == 1)
		{
		/*
	 	 * make toVal point to the result.
	 	 */
		toVal->size = sizeof(double);
		toVal->addr = (XtPointer) &result;
		}
	else
		/*
		 * If sscanf fails, issue a warning that something is wrong.
		 */
		XtStringConversionWarning((char *) fromVal->addr,"Double");
	}

/***************************************************
 * str2clk.c: Convert a string to a Clock Direction.
 ***************************************************/

static void	xm_cvt_str_to_clock(args,nargs,fromVal,toVal)
XrmValue	*args;
Cardinal	*nargs;
XrmValue  	*fromVal, *toVal;
 	{
	static  int     result;
	char		*resource_input = (char *) (fromVal->addr);
	XmString	res_string, cwise_string;

	/*
	 * Make sure the number of args is correct.
	 */
	if (*nargs!=0)
		XtWarning("String to Clock conversion needs no arguments.");

	/*
	 * Convert strings to compound strings.
	 */
	res_string = XmStringCreate(resource_input,XmSTRING_DEFAULT_CHARSET);
	cwise_string = XmStringCreate("CLOCKWISE",XmSTRING_DEFAULT_CHARSET);

	/*
	 * Convert the string in fromVal to a Clock Direction.
	 */
	toVal->size = sizeof(int);
	if (XmStringCompare(cwise_string,res_string))
		result = CLOCKWISE;
	else
		result = COUNTERCLOCKWISE;
	toVal->addr = (XtPointer) &result;
	}
static double _dxf_round(double a, int decimal_places)
{
    double value;
    double expon;
    double remember;

    remember = a;

    /*
     * Round the value if decimal_places != -1.
     */
    expon = pow((double)10, (double)decimal_places);
    a = a * expon;
    if (a < 0)
    {
	a = a - 0.5;
    }
    else
    {
	a = a + 0.5;
    }
    if (fabs(a) < INT_MAX)
    {
	value = trunc(a);
	return (value / expon);
    }
    else
    {
	return (remember);
    }
}
