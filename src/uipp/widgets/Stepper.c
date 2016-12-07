/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



/* Stepper resize logic:

	1. Default - XmNrecomputeSize = TRUE;  This means that the stepper 
	   widget will automatically resize itself based on the min and max
	   values of the widget, and the number of decimal places.  This was
	   done to prevent frequent resizes of steppers in the slider widget.

	2. XmNrecomputeSize = FALSE;  In this case, the widget sizes itself
	   based on the XmNdigits resource.  This delimits the total size
	   of the widget.  This mode presents potentially unsightly operation
	   of the widget if the size in not adequate to handle the min/max
	   values of the widget
*/

#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/XmP.h>
#include <Xm/Xm.h>
#include <Xm/BulletinB.h>
#include <Xm/ArrowB.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include "Number.h"
#include "StepperP.h"
#include "Stepper.h"
#include "gamma.h"
#include "findcolor.h"

#if !defined(HAVE_TRUNC)
#define trunc(c)        ((double)((int)(c)))
#endif


/*
 *  Error messages:
 */
#define MESSAGE1	"The Maximum value must be greater than the Minimum."
#define MESSAGE2	"Meter value exceeds the Maximum value."
#define MESSAGE3	"Meter value is less then the Minimum value."
#define MESSAGE4	"The step value will be rounded.  Check decimal places."

/*  A locally used constant  */
#define XmCR_REPEAT_ARM 11522

/*  Define struct for commonly used group of dimension and coordinate values  */
struct layout {
    int number_x, number_y;
    Dimension number_width, number_height;
    int up_left_x, up_left_y;
    int down_right_x, down_right_y;
    Dimension arrow_width, arrow_height;
    Dimension field_width, field_height;
};


/* External Functions */
void SyncMultitypeData( MultitypeData* new, short type ); /* from Number.c */


/*
 *  Forward local subroutine and function declarations:
 */
static void	ClassInitialize	    ();
static void	Initialize	    (XmStepperWidget		request,
				     XmStepperWidget		new);
static Boolean	SetValues	    (XmStepperWidget		current,
				     XmStepperWidget		request,
				     XmStepperWidget		new);
static void	Destroy		    (XmStepperWidget		sw);
static void	Resize		    (XmStepperWidget		sw);
static void	ChangeManaged	    (XmStepperWidget		sw);
static void	Redisplay	    (XmStepperWidget		sw,
				     XEvent *			event,
				     Region			region);
static void	Arm		    (XmStepperWidget		sw,
				     XButtonPressedEvent *	event);
static void	Activate	    (XmStepperWidget		sw,
				     XButtonPressedEvent *	event);
static void	CallFromNumber	    (Widget			w,
				     XmStepperWidget		sw,
				     XmDoubleCallbackStruct *	call_data);
static void	CallbackActivate    (XmStepperWidget		sw);
static void	CallbackStep	    (XmStepperWidget		sw,
				     Boolean			increase);
static void	VerifyValueRange    (XmStepperWidget		sw, Boolean);
static void	ComputeLayout	    (XmStepperWidget		sw,
				     struct layout *		loc);
static void	LayoutStepper	    (XmStepperWidget		sw);
static Boolean	IncrementStepper    (XmStepperWidget		sw,
				     Boolean			increase,
				     double			step_size);
static void	ButtonIncrease	    (Widget			w,
				     XmStepperWidget		sw,
				     XmAnyCallbackStruct *	call_data);
static void	ButtonDecrease	    (Widget			w,
				     XmStepperWidget		sw,
				     XmAnyCallbackStruct *	call_data);
static void	ButtonIsReleased    (Widget			w,
				     XmStepperWidget		sw,
				     XmAnyCallbackStruct *	call_data);
static Boolean	ButtonArmEvent      (Widget			w,
				     XmStepperWidget		sw,
				     XtCallbackProc		callback_proc);
static void	RepeatDigitButton   (XmStepperWidget		sw,
				     XtIntervalId *		id);
static XtGeometryResult 
PreferredSize (XmStepperWidget , XtWidgetGeometry *, XtWidgetGeometry *);
static XtGeometryResult
                GeometryManager         (Widget w,
                                         XtWidgetGeometry *request,
                                         XtWidgetGeometry *reply);

static void 	WarningCallback	    ( Widget w, XmStepperWidget sw,
				     XmNumberWarningCallbackStruct *call_data );
static
Boolean	UpdateMultitypeData (XmStepperWidget sw, MultitypeData* new, 
		MultitypeData* current);


/*  Default translation table and action list  */
static char defaultTranslations[] =
    "<Btn1Down>:     Arm()\n\
     <Btn1Up>:	     Activate()";
/*
    "<Btn1Down>:     Arm()\n\
     <Btn1Up>:	     Activate()\n\
     <EnterWindow>:  Enter() \n\
     <FocusIn>:      FocusIn()";
 */

static XtActionsRec actionsList[] =
{
   { "Arm",      (XtActionProc) Arm  },
   { "Activate", (XtActionProc) Activate },
};
/*
   { "Arm",      (XtActionProc) Arm  },
   { "Activate", (XtActionProc) Activate },
   { "Enter",    (XtActionProc) _XmManagerEnter },
   { "FocusIn",  (XtActionProc) _XmManagerFocusIn },
};
 */


/*
 *  Resource definitions for Stepper class:
 */

/*  Declare defaults that can be assigned through pointers  */
#if defined(aviion)
/* FLT_MAX is not a constant, but an external variable.  these must be
 *  initialized at run time instead on the data general.
 */
static int DefaultInitialized = FALSE;
#define DOUBLE_MIN  0.0
#define DOUBLE_MAX  0.0
#define FLOAT_MIN   0.0
#define FLOAT_MAX   0.0
#else
#define DOUBLE_MIN -FLT_MAX
#define DOUBLE_MAX  FLT_MAX
#define FLOAT_MIN  -FLT_MAX
#define FLOAT_MAX   FLT_MAX
#endif

#define DEF_MIN FLOAT_MIN
#define DEF_MAX FLOAT_MAX
#define DEF_VALUE 0
#define DEF_STEP 1

#if defined(hp700) || defined(solaris) || defined(sun4) || defined(aviion) || defined (intelnt) || defined(OS2)
#   define trunc(x) ((double)((int)(x)))
#endif                                                                          

static double DefaultMinDbl = (double)DEF_MIN;
static double DefaultMaxDbl = (double)DEF_MAX;
static double DefaultValDbl = (double)DEF_VALUE;
static double DefaultStepDbl = (double)DEF_STEP;
static float DefaultMinFlt = (float)FLOAT_MIN;
static float DefaultMaxFlt = (float)FLOAT_MAX;
static float DefaultValFlt = (float)DEF_VALUE;
static float DefaultStepFlt = (float)DEF_STEP;

static XtResource resources[] =
{
    {
      XmNdataType, XmCDataType, XmRShort, sizeof(short),
      XtOffset(XmStepperWidget, stepper.data_type),
      XmRImmediate, (XtPointer) INTEGER
    },
    {
      XmNdValue, XmCDValue, XmRDouble, sizeof(double),
      XtOffset(XmStepperWidget, stepper.value.d),
      XmRDouble, (XtPointer) &DefaultValDbl
    },
    {
      XmNfValue, XmCFValue, XmRFloat, sizeof(float),
      XtOffset(XmStepperWidget, stepper.value.f),
      XmRFloat, (XtPointer) &DefaultValFlt
    },
    {
      XmNiValue, XmCIValue, XmRInt, sizeof(int),
      XtOffset(XmStepperWidget, stepper.value.i),
      XmRImmediate, (XtPointer) DEF_VALUE
    },
    {
      XmNdMinimum, XmCDMinimum, XmRDouble, sizeof(double),
      XtOffset(XmStepperWidget, stepper.value_minimum.d),
      XmRDouble, (XtPointer) &DefaultMinDbl
    },
    {
      XmNfMinimum, XmCFMinimum, XmRFloat, sizeof(float),
      XtOffset(XmStepperWidget, stepper.value_minimum.f),
      XmRFloat, (XtPointer) &DefaultMinFlt
    },
    {
      XmNiMinimum, XmCIMinimum, XmRInt, sizeof(int),
      XtOffset(XmStepperWidget, stepper.value_minimum.i),
      XmRImmediate, (XtPointer) -1000000
    },
    {
      XmNdMaximum, XmCDMaximum, XmRDouble, sizeof(double),
      XtOffset(XmStepperWidget, stepper.value_maximum.d),
      XmRDouble, (XtPointer) &DefaultMaxDbl
    },
    {
      XmNfMaximum, XmCFMaximum, XmRFloat, sizeof(float),
      XtOffset(XmStepperWidget, stepper.value_maximum.f),
      XmRFloat, (XtPointer) &DefaultMaxFlt
    },
    {
      XmNiMaximum, XmCIMaximum, XmRInt, sizeof(int),
      XtOffset(XmStepperWidget, stepper.value_maximum.i),
      XmRImmediate, (XtPointer) 1000000
    },
    {
      XmNdValueStep, XmCDValueStep, XmRDouble, sizeof(double),
      XtOffset(XmStepperWidget, stepper.value_step.d),
      XmRDouble, (XtPointer) &DefaultStepDbl
    },
    {
      XmNfValueStep, XmCFValueStep, XmRFloat, sizeof(float),
      XtOffset(XmStepperWidget, stepper.value_step.f),
      XmRFloat, (XtPointer) &DefaultStepFlt
    },
    {
      XmNiValueStep, XmCIValueStep, XmRInt, sizeof(int),
      XtOffset(XmStepperWidget, stepper.value_step.i),
      XmRImmediate, (XtPointer) 1
    },
    {
      XmNdigits, XmCDigits, XmRCardinal, sizeof(Cardinal),
      XtOffset(XmStepperWidget, stepper.num_digits),
      XmRImmediate, (XtPointer) 8
    },
    {
      XmNdecimalPlaces, XmCDecimalPlaces, XmRInt, sizeof(int),
      XtOffset(XmStepperWidget, stepper.decimal_places),
      XmRImmediate, (XtPointer) 0
    },
    {
      XmNtime, XmCTime, XmRCardinal, sizeof(Cardinal),
      XtOffset(XmStepperWidget, stepper.time_interval),
      XmRImmediate, (XtPointer) 200
    },
    {
      XmNtimeDelta, XmCTimeDelta, XmRCardinal, sizeof(Cardinal),
      XtOffset(XmStepperWidget, stepper.time_ddelta),
      XmRImmediate, (XtPointer) 8
    },
    {
      XmNarmCallback, XmCArmCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmStepperWidget, stepper.step_callback),
      XmRCallback, NULL
    },
    {
      XmNactivateCallback, XmCActivateCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmStepperWidget, stepper.activate_callback),
      XmRCallback, NULL
    },
    {
      XmNincreaseDirection, XmCIncreaseDirection, XmRArrowDirection, 
      sizeof(unsigned char),
      XtOffset(XmStepperWidget, stepper.increase_direction), 
      XmRImmediate, (XtPointer) XmARROW_RIGHT
    },
    {
      XmNcenter, XmCCenter, XmRBoolean, sizeof(Boolean),
      XtOffset(XmStepperWidget, stepper.center_text),
      XmRImmediate, (XtPointer) TRUE
    },
    {
      XmNeditable, XmCEditable, XmRBoolean, sizeof(Boolean),
      XtOffset(XmStepperWidget, stepper.editable),
      XmRImmediate, (XtPointer) TRUE
    },
    {
      XmNrollOver, XmCRollOver, XmRBoolean, sizeof(Boolean),
      XtOffset(XmStepperWidget, stepper.roll_over),
      XmRImmediate, (XtPointer) FALSE
    },
    {
      XmNrecomputeSize, XmCRecomputeSize, XmRBoolean, sizeof(Boolean),
      XtOffset(XmStepperWidget, stepper.resize_to_number),
      XmRImmediate, (XtPointer) TRUE
    },
    {
      XmNfixedNotation, XmCFixedNotation, XmRBoolean, sizeof(Boolean),
      XtOffset(XmStepperWidget, stepper.is_fixed),
      XmRImmediate, (XtPointer) TRUE
    },
    {
      XmNwarningCallback, XmCCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmStepperWidget, stepper.warning_callback),
      XmRCallback, NULL
    },
    {
      XmNalignment, XmCAlignment, XmRAlignment, sizeof(unsigned char),
      XtOffset(XmStepperWidget, stepper.alignment),
      XmRImmediate, XmALIGNMENT_BEGINNING
    },
};


/*
 *  Stepper class record definition:
 */
XmStepperClassRec xmStepperClassRec = {
    /*
     *  CoreClassPart:
     */
    {
	(WidgetClass)&xmManagerClassRec, /* superclass			*/
	"XmStepper",			/* class_name			*/
	sizeof(XmStepperRec),		/* widget_size			*/
	(XtProc) ClassInitialize,	/* class_initialize		*/
	(XtWidgetClassProc) NULL,	/* class_part_initialize	*/
	FALSE,				/* class_inited			*/
	(XtInitProc) Initialize,	/* initialize			*/
	(XtArgsProc) NULL,		/* initialize_hook		*/
	(XtRealizeProc)_XtInherit,	/* realize			*/
	actionsList,			/* actions			*/
	XtNumber(actionsList),		/* num_actions			*/
	resources,			/* resources			*/
	XtNumber(resources),		/* num_resources		*/
	NULLQUARK,			/* xrm_class			*/
	FALSE,				/* compress_motion		*/
	FALSE,				/* compress_exposure		*/
	TRUE,				/* compress_enterleave		*/
	FALSE,				/* visible_interest		*/
	(XtWidgetProc)Destroy,		/* destroy			*/
	(XtWidgetProc)Resize,		/* resize			*/
	(XtExposeProc)Redisplay,	/* expose			*/
	(XtSetValuesFunc)SetValues,	/* set_values			*/
	(XtArgsFunc) NULL,		/* set_values_hook		*/
	XtInheritSetValuesAlmost,	/* set_values_almost		*/
	(XtArgsProc) NULL,		/* get_values_hook		*/
	XtInheritAcceptFocus,		/* accept_focus			*/
	XtVersion,			/* version			*/
	NULL,				/* callback private		*/
	defaultTranslations,		/* tm_table			*/
	(XtGeometryHandler)PreferredSize,/* query_geometry		*/
	(XtStringProc) NULL,		/* display_accelerator		*/
	(void *) NULL,			/* extension			*/
    },
    /*
     *  CompositeClassPart:
     */
    {
	GeometryManager,		/* geometry_manager		*/
	(XtWidgetProc)ChangeManaged,	/* change_managed		*/
	(XtWidgetProc)XtInheritInsertChild,	/* insert_child		*/
	(XtWidgetProc)XtInheritDeleteChild,	/* delete_child		*/
	(void *) NULL,			/* extension			*/
    },
    /*
     *  ConstraintClassPart:
     */
    {
	NULL,				/* resource list		*/
	0,				/* num resources		*/
	0,				/* constraint size		*/
	NULL,				/* init proc			*/
	NULL,				/* destroy proc			*/
	NULL,				/* set values proc		*/
	NULL,				/* extension			*/
    },
    /*
     *  XmManagerClassPart:
     */
    {
#if (XmVersion >= 1001)
      XtInheritTranslations,     		/* translations           */
#else
      (XtTranslations)_XtInherit,     		/* translations           */
#endif
	NULL,				/* get resources		*/
	0,				/* num get_resources		*/
	NULL,				/* get_cont_resources		*/
	0,				/* num_get_cont_resources	*/
	(XmParentProcessProc)NULL,      /* parent_process         */
	NULL,				/* extension			*/
    },
    /*
     *  XmStepperClassPart:
     */
    {
	10,				/* minimum_time_interval	*/
	180,				/* initial_time_interval	*/
  	NULL,				/* font				*/
  	NULL,
    }
};

WidgetClass xmStepperWidgetClass = (WidgetClass)&xmStepperClassRec;


/*  Subroutine:	ClassInitialize
 *  Purpose:	Install non-standard type converters needed by this widget class
 */
static void ClassInitialize()
{
#if defined(aviion)
    if (!DefaultInitialized) {
	DefaultMinDbl = (double)-FLT_MAX;
	DefaultMaxDbl = (double)FLT_MAX;
	DefaultMinFlt = (float)-FLT_MAX;
	DefaultMaxFlt = (float)FLT_MAX;
	DefaultInitialized = FALSE;
    }
#endif

    /*  Install converters for type XmRDouble, XmRFloat  */
    XmAddFloatConverters();
}

static XtGeometryResult
PreferredSize (XmStepperWidget step, XtWidgetGeometry *prop, XtWidgetGeometry *resp)
{
struct layout needs;

    ComputeLayout (step, &needs);
    memcpy (prop, resp, sizeof(XtWidgetGeometry));

    /* this is the minimum required */
    resp->height = needs.field_height;
    resp->width = needs.field_width;
    resp->request_mode = CWWidth|CWHeight;

    if (prop->request_mode & CWHeight) {
	if (prop->height == resp->height) return XtGeometryYes;

    } else if ((resp->width == prop->width) && (resp->height == prop->height)) {
	return XtGeometryNo;
    } else
	return XtGeometryAlmost;

    return XtGeometryNo;
}


/*  Subroutine:	Initialize
 *  Effect:	Create and initialize the component widgets
 */
static void Initialize( XmStepperWidget request, XmStepperWidget new )
{
    Arg wargs[25];
    struct layout loc;
    XColor fg_cell_def, bg_cell_def;

    /*  Check value and limits for consistency and adjust if nescessary  */
    SyncMultitypeData(&new->stepper.value, new->stepper.data_type);
    SyncMultitypeData(&new->stepper.value_minimum, new->stepper.data_type);
    SyncMultitypeData(&new->stepper.value_maximum, new->stepper.data_type);
    SyncMultitypeData(&new->stepper.value_step, new->stepper.data_type);
    VerifyValueRange(new, False);
    (void)UpdateMultitypeData(new, &new->stepper.value_minimum,
				 &request->stepper.value_minimum);
    (void)UpdateMultitypeData(new, &new->stepper.value,
				 &request->stepper.value);
    (void)UpdateMultitypeData(new, &new->stepper.value_maximum,
				 &request->stepper.value_maximum);
    (void)UpdateMultitypeData(new, &new->stepper.value_step,
				 &request->stepper.value_step);

    /*
     *  Create the panel-meter like number display
     */
    XtSetArg(wargs[0], XmNx, 0);
    XtSetArg(wargs[1], XmNy, 0);
    /*  Estimate a comfortably large size to initially create NumberWidget  */
    if(   (new->stepper.increase_direction == XmARROW_LEFT)
       || (new->stepper.increase_direction == XmARROW_RIGHT) )
    {
	if( (new->core.height <= 0) || (new->core.height > 4096) )
	    loc.number_height = 19;
	else
	    loc.number_height = new->core.height;
	if( (new->core.width <= 0) || (new->core.width > 4096) )
	    loc.number_width = (new->stepper.num_digits * 6) + 4;
	else
	    loc.number_width = new->core.width - ((2 * loc.number_height) + 2);
    }
    else
    {
	if( new->core.height <= 0 )
	    loc.number_height = 19;
	else
	    loc.number_height = new->core.height / 3;
	if( new->core.width <= 0 )
	    loc.number_width = (new->stepper.num_digits * 6) + 4;
	else
	    loc.number_width = new->core.width;
    }
    XtSetArg(wargs[2], XmNheight, loc.number_height);
    XtSetArg(wargs[3], XmNwidth, loc.number_width);
    XtSetArg(wargs[4], XtNinsertPosition, 0);
    DoubleSetArg(wargs[5], XmNdMinimum, new->stepper.value_minimum.d);
    DoubleSetArg(wargs[6], XmNdMaximum, new->stepper.value_maximum.d);
    DoubleSetArg(wargs[7], XmNdValue, new->stepper.value.d);
    if( new->stepper.decimal_places == 0 )
	XtSetArg(wargs[8], XmNcharPlaces, new->stepper.num_digits);
    else
	XtSetArg(wargs[8], XmNcharPlaces, new->stepper.num_digits + 1);
    XtSetArg(wargs[9], XmNdecimalPlaces, new->stepper.decimal_places);
    XtSetArg(wargs[10], XmNcenter, new->stepper.center_text);
    XtSetArg(wargs[11], XmNeditable, new->stepper.editable);
    XtSetArg(wargs[12], XmNdataType, DOUBLE);
    XtSetArg(wargs[13], XmNrecomputeSize, new->stepper.resize_to_number);
    XtSetArg(wargs[14], XmNfixedNotation, new->stepper.is_fixed);
    new->stepper.child[0] = XmCreateNumber((Widget)new, "label", wargs, 15);
    XtAddCallback(new->stepper.child[0],XmNwarningCallback,(XtCallbackProc)WarningCallback,new);

    /*  Now that the number exists, Calculate the geometries  */
    ComputeLayout(new, &loc);
    new->core.width = loc.field_width;
    new->core.height = loc.field_height;

    /*  Now reconfigure the number widget correctly  */
    XtSetArg(wargs[0], XmNx, loc.number_x);
    XtSetArg(wargs[1], XmNy, loc.number_y);
    XtSetArg(wargs[2], XmNwidth, loc.number_width);
    XtSetArg(wargs[3], XmNheight, loc.number_height);
    XtSetValues(new->stepper.child[0], wargs, 4);

    /*  Create the upper or left arrow  */
    XtSetArg(wargs[0], XmNx, loc.up_left_x);
    XtSetArg(wargs[1], XmNy, loc.up_left_x);
    XtSetArg(wargs[2], XmNwidth, loc.arrow_width);
    XtSetArg(wargs[3], XmNheight, loc.arrow_height);
    if( (new->stepper.increase_direction == XmARROW_LEFT)
       || (new->stepper.increase_direction == XmARROW_RIGHT) )
	XtSetArg(wargs[4], XmNarrowDirection, XmARROW_LEFT);
    else
	XtSetArg(wargs[4], XmNarrowDirection, XmARROW_UP);
    XtSetArg(wargs[5], XtNinsertPosition, 1);
    XtSetArg(wargs[6], XmNshadowThickness, new->manager.shadow_thickness);
    new->stepper.child[1] = XmCreateArrowButton((Widget)new, "left", wargs, 7);

    /*
     *  Create the lower or right arrow
     */
    XtSetArg(wargs[0], XmNx, loc.down_right_x);
    XtSetArg(wargs[1], XmNy, loc.down_right_y);
    if( (new->stepper.increase_direction == XmARROW_LEFT)
       || (new->stepper.increase_direction == XmARROW_RIGHT) )
	XtSetArg(wargs[4], XmNarrowDirection, XmARROW_RIGHT);
    else
	XtSetArg(wargs[4], XmNarrowDirection, XmARROW_DOWN);
    XtSetArg(wargs[5], XtNinsertPosition, 2);
    new->stepper.child[2] = XmCreateArrowButton((Widget)new, "right", wargs, 7);

    /*
     *  Get the foreground and background colors, and use the average
     *  for the insensitive color
     */
    fg_cell_def.flags = DoRed | DoGreen | DoBlue;
    bg_cell_def.flags = DoRed | DoGreen | DoBlue;
    fg_cell_def.pixel = new->manager.foreground;
    XQueryColor(XtDisplay(new), 
		DefaultColormapOfScreen(XtScreen(new)),
		&fg_cell_def);

    bg_cell_def.pixel = new->core.background_pixel;
    XQueryColor(XtDisplay(new),
                DefaultColormapOfScreen(XtScreen(new)),
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
    if(!XAllocColor(XtDisplay(new),
                DefaultColormapOfScreen(XtScreen(new)),
		&fg_cell_def))
	{
	find_color((Widget)new, &fg_cell_def);
	}
    new->stepper.insens_foreground = fg_cell_def.pixel;
    if (!(new->core.sensitive && new->core.ancestor_sensitive))
    {
	XtSetArg(wargs[0], XmNforeground, new->stepper.insens_foreground);
	XtSetValues(new->stepper.child[1], wargs, 1);
	XtSetValues(new->stepper.child[2], wargs, 1);
    }

    if( (new->stepper.increase_direction == XmARROW_DOWN)
       || (new->stepper.increase_direction == XmARROW_RIGHT) )
    {
	XtAddCallback(new->stepper.child[2],XmNarmCallback,(XtCallbackProc)ButtonIncrease, new);
	XtAddCallback(new->stepper.child[1],XmNarmCallback,(XtCallbackProc)ButtonDecrease, new);
    }
    else
    {
	XtAddCallback(new->stepper.child[1],XmNarmCallback,(XtCallbackProc)ButtonIncrease, new);
	XtAddCallback(new->stepper.child[2],XmNarmCallback,(XtCallbackProc)ButtonDecrease, new);
    }
    XtAddCallback(new->stepper.child[1],XmNdisarmCallback,(XtCallbackProc)ButtonIsReleased,new);
    XtAddCallback(new->stepper.child[2],XmNdisarmCallback,(XtCallbackProc)ButtonIsReleased,new);
    XtAddCallback(new->stepper.child[0],XmNactivateCallback,(XtCallbackProc)CallFromNumber,new);
    XtAddCallback(new->stepper.child[0],XmNarmCallback,(XtCallbackProc)CallFromNumber, new);
    XtAddCallback(new->stepper.child[0],XmNdisarmCallback,(XtCallbackProc)CallFromNumber, new);

    new->stepper.allow_input = TRUE;
    new->stepper.timer = (XtIntervalId)NULL;
    XtManageChildren(new->stepper.child, 3);
}

/*  Subroutine: XtGeometryManager
 *  Purpose:    Get informed of a request to change a child widget's geometry.
 */
static XtGeometryResult GeometryManager( Widget w, XtWidgetGeometry *request,
                                         XtWidgetGeometry *reply )
{
Dimension ret_w, ret_h, new_width, new_height;
Widget stepper;
Dimension child_width, child_height;
XmNumberWidget nw;

    nw = (XmNumberWidget)w;
    if (request->request_mode & CWWidth)
	{
	child_width = request->width;
	}
    else
	{
	child_width = nw->core.width;
	}
    if (request->request_mode & CWHeight)
	{
	child_height = request->height;
	}
    else
	{
	child_height = nw->core.height;
	}
    if ((request->width != w->core.width) || 
	(request->height != w->core.height))
	{
	stepper = XtParent(w);
	new_width = stepper->core.width;
	if (request->request_mode & CWWidth)
	    {
	    new_width += (request->width - w->core.width);
	    }
	new_height = stepper->core.height;
	if (request->request_mode & CWHeight)
	    {
	    new_height += (request->height - w->core.height);
	    }
	if (XtMakeResizeRequest(stepper, new_width, new_height, 
				&ret_w, &ret_h) == XtGeometryYes)
	    {
	    XtResizeWidget(w, child_width, child_height, 0);
	    return XtGeometryYes;
	    }
	else
	    {
	    return XtGeometryNo;
	    }
	}
    return XtGeometryYes;
}

/*  Subroutine:	SetValues
 *  Effect:	Handles requests to change things from the application
 */
static Boolean SetValues( XmStepperWidget current,
			  XmStepperWidget request,
			  XmStepperWidget new )
{
    Arg wargs[6];
    int i;
    struct layout loc;
    Boolean need_new_width;
    Boolean need_new_height;

    /*
     * set these to true based on a new resource setting we'll honor which
     * results in a need for more pixels.  This really ought to include new
     * font.
     */
    need_new_width = need_new_height = False;

    if( (new->core.sensitive != current->core.sensitive) ||
        (new->core.ancestor_sensitive != current->core.ancestor_sensitive) )
    {
	if ((new->core.sensitive && new->core.ancestor_sensitive))
	{
	    XtSetArg(wargs[0], XmNforeground, new->manager.foreground);
	    XtSetValues(new->stepper.child[1], wargs, 1);
	    XtSetValues(new->stepper.child[2], wargs, 1);
	}
	else
	{
	    XtSetArg(wargs[0], XmNforeground, new->stepper.insens_foreground);
	    XtSetValues(new->stepper.child[1], wargs, 1);
	    XtSetValues(new->stepper.child[2], wargs, 1);
	}
    }

    SyncMultitypeData(&new->stepper.value, new->stepper.data_type);
    SyncMultitypeData(&new->stepper.value_minimum, new->stepper.data_type);
    SyncMultitypeData(&new->stepper.value_maximum, new->stepper.data_type);
    SyncMultitypeData(&new->stepper.value_step, new->stepper.data_type);
    VerifyValueRange(new, new->stepper.value_step.d != current->stepper.value_step.d);
    i = 0;
    if(   (new->stepper.decimal_places != current->stepper.decimal_places)
       || (new->stepper.num_digits != current->stepper.num_digits) )
    {
	need_new_width = True;
	if( new->stepper.decimal_places == 0 )
	    XtSetArg(wargs[i], XmNcharPlaces, new->stepper.num_digits);
	else
	    XtSetArg(wargs[i], XmNcharPlaces, new->stepper.num_digits + 1);
	i++;
	XtSetArg(wargs[i], XmNdecimalPlaces, new->stepper.decimal_places);
	i++;
	DoubleSetArg(wargs[i], XmNdValue, new->stepper.value.d);
	i++;
    }
    if( UpdateMultitypeData(new, &new->stepper.value_minimum,
				 &current->stepper.value_minimum) )
    {
	DoubleSetArg(wargs[i], XmNdMinimum, new->stepper.value_minimum.d);
	i++;
    }
    if( UpdateMultitypeData(new, &new->stepper.value_maximum,
				 &current->stepper.value_maximum) )
    {
	DoubleSetArg(wargs[i], XmNdMaximum, new->stepper.value_maximum.d);
	i++;
    }
    /*  Finally, check if just the contents have changed  */
    if( UpdateMultitypeData(new, &new->stepper.value, &current->stepper.value) )
    {
	/*  If we must set values anyway, include this one  */
	if( i > 0 )
	{
	    DoubleSetArg(wargs[i], XmNdValue, new->stepper.value.d);
	    i++;
	}
	/*  If value is only thing to pass, do it more efficiently  */
	else if( new->composite.children[0] )
	    XmChangeNumberValue((XmNumberWidget)new->composite.children[0],
				 new->stepper.value.d);
    }
    /*  Field the step_size input, if any, but don't do anything special  */
    (void)UpdateMultitypeData(new, &new->stepper.value_step,
			      &current->stepper.value_step);
    /*  Check value and limits for consistency and adjust if nescessary  */
    if( (i>0) && (new->composite.num_children > 0) )
	XtSetValues(new->composite.children[0], wargs, i);

    ComputeLayout(new, &loc);
    if (XtIsRealized ((Widget)new)) LayoutStepper (new);

    if (need_new_width) new->core.width = loc.field_width;
    if (need_new_height) new->core.height = loc.field_height;
    return TRUE;
}


/*  Subroutine:	Destroy
 *  Effect:	Frees resources before widget is destroyed
 */
static void Destroy( XmStepperWidget sw )
{
    XtRemoveAllCallbacks((Widget)sw, XmNarmCallback);
    XtRemoveAllCallbacks((Widget)sw, XmNactivateCallback);
}


/*  Subroutine:	Resize
 *  Purpose:	Calculate the drawing area and redraw the widget
 */
static void Resize( XmStepperWidget sw )
{
    LayoutStepper(sw);
}


/*  Subroutine:	ChangeManaged
 *  Effect:	Makes sure everyone is in their places before the curtain opens
 */
static void ChangeManaged( XmStepperWidget sw )
{
    LayoutStepper(sw);
}


/*  Subroutine:	Redisplay
 *  Effect:	Does redisplays on the component gadgets (gadgets must be
 *		handled differently from widgets)
 */
static void Redisplay( XmStepperWidget sw, XEvent* event, Region region)
{
}


/*  Subroutine:	Arm
 *  Effect:	Processes "arm" type actions in the stepper widget field that
 *		might belong to its gadgets.
 */
static void Arm( XmStepperWidget sw, XButtonEvent* event )
{
}


/*  Subroutine:	Activate
 *  Effect:	Processes "activate" and "disarm" type actions in the stepper
 *		widget field that might belong to its gadgets.
 */
static void Activate( XmStepperWidget sw, XButtonPressedEvent* event )
{
}


/*  Subroutine:	CallFromNumber
 *  Purpose:	Callback routine from number widget to allow/disallow stepper
 *		to manipulate the value, or to set a new value.
 */
static void CallFromNumber( Widget	w,
			    XmStepperWidget sw,
			    XmDoubleCallbackStruct* call_data )
{

    if( call_data->reason == XmCR_ARM )
    {
	if( sw->stepper.timer )
	    XtRemoveTimeOut(sw->stepper.timer);
	sw->stepper.allow_input = FALSE;
    }
    else if( call_data->reason == XmCR_DISARM )
    {
	sw->stepper.allow_input = TRUE;
    }
    else if( call_data->reason == XmCR_ACTIVATE )
    {
	sw->stepper.value.d = call_data->value;
	if ((sw->stepper.data_type == FLOAT) || 
	    (sw->stepper.data_type == INTEGER) )
	    sw->stepper.value.f = (float)call_data->value;
	if (sw->stepper.data_type == INTEGER)
	{
	    if( call_data->value >= 0.0 )
		sw->stepper.value.i = (int)(call_data->value + 0.5);
	    else
		sw->stepper.value.i = (int)(call_data->value - 0.5);
	}
    }
    if (call_data->reason == XmCR_ACTIVATE)
    {
	sw->stepper.value_changed = TRUE;
	CallbackActivate(sw);
    }
}


#define MAX_VAL(a,b) ((a)>(b)?(a):(b))
/*  Subroutine:	ComputeLayout
 *  Effect:	Calculates the sizes and positions for all components
 */
static void ComputeLayout( XmStepperWidget sw, struct layout *loc )
{
    XtWidgetGeometry request, reply;

    if( sw->composite.num_children > 0 )
    {
	/*  Calculate the size of the number widget  */
	request.request_mode = CWWidth | CWHeight;
	request.width = 1;
	request.height = 1;
	(void)XtQueryGeometry(sw->composite.children[0], &request, &reply);
	if( sw->stepper.resize_to_number )
	{
	    loc->number_width = reply.width;
	    loc->number_height = reply.height;
	}
	else
	{
/*	    loc->number_width =
	      MAX_VAL(reply.width, sw->composite.children[0]->core.width);
	    loc->number_height =
	      MAX_VAL(reply.height, sw->composite.children[0]->core.height); */
	    loc->number_width = sw->composite.children[0]->core.width;
	    loc->number_height = sw->composite.children[0]->core.height;
	}
    }
    else if( loc->number_height <= 4 )
    {
	loc->number_width = (sw->stepper.num_digits * 6) + 4;
	loc->number_height = 19;
    }

    /*  Arrows have height of label and are square or full width  */
    loc->arrow_height = loc->number_height;
    loc->up_left_x = loc->up_left_y = 0;
    if( (sw->stepper.increase_direction == XmARROW_LEFT)
       || (sw->stepper.increase_direction == XmARROW_RIGHT) )
    {
	loc->field_height = loc->number_height;
	loc->arrow_width = loc->number_height;
	loc->number_y = 0;
	loc->down_right_y = 0;
	loc->field_width = loc->number_width + (2*loc->number_height) + 2;
	if (sw->stepper.alignment == XmALIGNMENT_CENTER) {

	   int midpt;
	   loc->field_width = MAX_VAL(loc->field_width, sw->core.width);
	   /*midpt = loc->field_width/2;*/
	   midpt = sw->core.width >> 1;

	   loc->number_x = midpt - (loc->number_width/2);
	   loc->down_right_x = loc->number_x + loc->number_width + 2;
	   loc->up_left_x = loc->number_x - (loc->arrow_width + 2);

	   loc->up_left_y = loc->number_y = loc->down_right_y = 0;
	   if (sw->core.height > loc->number_height) {
	   	loc->up_left_y = loc->number_y = loc->down_right_y = 
		(sw->core.height - loc->number_height)>>1;
	   }
	} else if (sw->stepper.alignment == XmALIGNMENT_BEGINNING) {
	    loc->down_right_x = loc->field_width - loc->arrow_width;
	    loc->number_x = loc->arrow_width + 1;

	} else { /* must == XmALIGNMENT_END */
	    loc->down_right_x = sw->core.width - (loc->arrow_width );
	    loc->number_x = loc->down_right_x - (2+loc->number_width);
	    loc->up_left_x = loc->number_x - (loc->arrow_width + 2);
	    loc->up_left_y = 0;
	}
    }
    else
    {
	loc->field_height = (3 * loc->number_height) + 2;
	loc->field_width = loc->number_width;
	loc->arrow_width = loc->number_width;
	loc->down_right_x = 0;
	loc->down_right_y = loc->field_height - loc->arrow_height;
	loc->number_x = 0;
	loc->number_y = loc->arrow_height + 1;
    }
}
#undef MAX_VAL


/*  Subroutine:	LayoutStepper
 *  Effect:	Moves and resizes anything that is out of shape or position
 */
static void LayoutStepper( XmStepperWidget sw )
{
    Widget number, up_left, down_right;
    struct layout loc;

    ComputeLayout(sw, &loc);
    if( sw->composite.num_children > 0 )
    {
	number = sw->composite.children[0];
	if( (loc.number_x != number->core.x) ||
	    (loc.number_y != number->core.y) )
	    XtMoveWidget(number, loc.number_x, loc.number_y);
	if( (loc.number_width != number->core.width) ||
	    (loc.number_height != number->core.height) )
	    XtResizeWidget(number, loc.number_width, loc.number_height,0);
    }
    if( sw->composite.num_children > 1 )
    {
	up_left = sw->composite.children[1];
	if( (loc.up_left_x != up_left->core.x) ||
	    (loc.up_left_y != up_left->core.y) )
	    XtMoveWidget(up_left, loc.up_left_x, loc.up_left_y);
	if( (loc.arrow_width != up_left->core.width) ||
	    (loc.arrow_height != up_left->core.height) )
	    XtResizeWidget(up_left, loc.arrow_width, loc.arrow_height,0);
    }
    if( sw->composite.num_children > 2 )
    {
	down_right = sw->composite.children[2];
	if( (loc.down_right_x != down_right->core.x) ||
	    (loc.down_right_y != down_right->core.y) )
	    XtMoveWidget(down_right, loc.down_right_x, loc.down_right_y);
	if( (loc.arrow_width != down_right->core.width) ||
	    (loc.arrow_height != down_right->core.height) )
	    XtResizeWidget(down_right, loc.arrow_width, loc.arrow_height,0);
    }
}


/*  Subroutine:	VerifyValueRange
 *  Effect:	Checks limits and then value for consistency with rules.
 */
static void VerifyValueRange( XmStepperWidget sw , Boolean checkStep)
{
int d1,d2, expon;
char string[32];
    /*
     *  Make sure the minimum and maximum value settings are valid.
     */
    if( sw->stepper.value_minimum.d > sw->stepper.value_maximum.d )
    {
/*        XtWarning(MESSAGE1); */
	sw->stepper.value_minimum.d = sw->stepper.value_maximum.d;
	sw->stepper.value_minimum.f = sw->stepper.value_minimum.f;
	sw->stepper.value_minimum.i = sw->stepper.value_maximum.i;
    }
    if( sw->stepper.value.d > sw->stepper.value_maximum.d )
    {
/*        XtWarning(MESSAGE2); */
	sw->stepper.value.d = sw->stepper.value_maximum.d;
	sw->stepper.value.f = sw->stepper.value_maximum.f;
	sw->stepper.value.i = sw->stepper.value_maximum.i;
    }
    if( sw->stepper.value.d < sw->stepper.value_minimum.d )
    {
/*        XtWarning(MESSAGE3); */
	sw->stepper.value.d = sw->stepper.value_minimum.d;
	sw->stepper.value.f = sw->stepper.value_minimum.f;
	sw->stepper.value.i = sw->stepper.value_minimum.i;
    }

    /*
     * Check that the incr/decr amount is not overly precise
     * because the rounding arithmetic would make it either 0 or 1.
     * ... but don't do anything about it!
     */
    if (checkStep) {
       sprintf(string, "%8.1e", sw->stepper.value_step.d);
       sscanf(string, "%d.%de%d", &d1, &d2, &expon);
       if ((expon<0)&&(-expon>sw->stepper.decimal_places)) 
       {
	   XmNumberWarningCallbackStruct nws;
	   nws.message = MESSAGE4;
	   nws.event = 0;
	   nws.reason = XmCR_NUM_WARN_EXCESS_PRECISION;
	   WarningCallback ((Widget)sw, sw, &nws);
       }
    }
}


/*  Subroutine:	IncrementStepper
 *  Effect:	Increment or decrement the stepper value and display by the
 *		 given step, up to the range limits.
 */
static Boolean IncrementStepper( XmStepperWidget sw,
				 Boolean increase, double step_size )
{
    double 	rvalue;
    double 	new_value;
    double 	value = sw->stepper.value.d;
    double 	expon;
    double 	cross_under;
    double 	cross_over;
    int         expon2;
    int 	d1;
    int 	d2;
    char   	string[100];
    Arg		wargs[5];

    if( increase )
    {
	value += step_size;
    }
    else
    {
	value -= step_size;
    }
    new_value = value;

#if 0
    double	ddv, diff;
    int dv;
    Boolean neg;
/* This code was added so that stepper and slider would be consistent.
   Taking it out because it's undesirable behavior for a stepper.  Leaving
   it in slider though.
*/
    /*
     * round the value to a multiple of step_size - gresh395
     */
    if (step_size)
    {
       if (new_value < 0.0) neg = True; else neg = False;
       ddv = new_value / step_size;
       dv = (int)ddv;
       diff = ddv - dv;
       if (diff >= 0.5) dv++;
       new_value = step_size * dv;
       if ((neg) && (new_value > 0.0)) new_value = -new_value;
       value = new_value;
    }
#endif

    /*
     * Round the value if decimal_places != -1.
     */
    
    if (sw->stepper.decimal_places != -1)
    {
	XtSetArg(wargs[0], XmNcrossUnder, &cross_under);
	XtSetArg(wargs[1], XmNcrossOver,  &cross_over);
	XtGetValues(sw->stepper.child[0], wargs, 2);
	sprintf(string, "%8.1e", value);
	sscanf(string, "%d.%de%d", &d1, &d2, &expon2);
	if (expon2 < 0)
	{
	    if (( -expon2 > sw->stepper.decimal_places) &&
		(((value < cross_under)&&(value > 0)) ||
		 ((-value < cross_under)&&(value < 0))) )
		expon = pow((double)10,
			(double)sw->stepper.decimal_places-expon2);
	    else
		expon = pow((double)10, (double)sw->stepper.decimal_places);
	}
	else
	{
	    if (( expon2 > sw->stepper.decimal_places) &&
		(((value > cross_over)&&(value > 0)) ||
		 ((-value > cross_over)&&(value < 0))) )
		expon = 1/pow((double)10,
			(double)expon2 - sw->stepper.decimal_places);
	    else
		expon = pow((double)10, (double)sw->stepper.decimal_places);
	}
	value = value * expon;
	if (value < 0) 
	{
	    value = value - 0.5;
	}
	else
	{
	    value = value + 0.5;
	}
	if (fabs(value) < INT_MAX)
	{
	    rvalue = trunc(value);
	    value = rvalue / expon;
	}
	else /* Too large to round */
	{
	    value = new_value;
	}
    }

    if( increase )
    {
	if( value > sw->stepper.value_maximum.d )
	{
	    if( sw->stepper.roll_over )
		value = sw->stepper.value_minimum.d 
		    + (value - sw->stepper.value_maximum.d);
	    else
		value = sw->stepper.value_maximum.d;
	}
	if( value < sw->stepper.value_minimum.d )
	{
	    if( sw->stepper.roll_over )
		value = sw->stepper.value_maximum.d 
		    + (value - sw->stepper.value_minimum.d);
	    else
		value = sw->stepper.value_minimum.d;
	}
    } else {
	if( value < sw->stepper.value_minimum.d )
	{
	    if( sw->stepper.roll_over )
		value = sw->stepper.value_maximum.d 
		  + (value - sw->stepper.value_minimum.d);
	    else
		value = sw->stepper.value_minimum.d;
	}
	if( value > sw->stepper.value_maximum.d )
	{
	    if( sw->stepper.roll_over )
		value = sw->stepper.value_minimum.d 
		  + (value - sw->stepper.value_maximum.d);
	    else
		value = sw->stepper.value_maximum.d;
	}
    }
    if( value != sw->stepper.value.d )
    {
	sw->stepper.value.d = value;
	sw->stepper.value_changed  = TRUE;
	if ((sw->stepper.data_type == FLOAT) || 
	    (sw->stepper.data_type == INTEGER) )
	    sw->stepper.value.f = (float)sw->stepper.value.d;
	if (sw->stepper.data_type == INTEGER)
	{
	    if( sw->stepper.value.d >= 0.0 )
		sw->stepper.value.i = (int)(sw->stepper.value.d + 0.5);
	    else
		sw->stepper.value.i = (int)(sw->stepper.value.d - 0.5);
	}
	if( sw->composite.children[0] )
	    XmChangeNumberValue((XmNumberWidget)sw->composite.children[0], 
				value);
    }

    return TRUE;
}

/*  Subroutine:	ButtonArmEvent
 *  Effect:	Check event that this arming (activation) can be accepted and
 *		 set up the next auto-repeat for an armed button.
 */
static Boolean ButtonArmEvent( Widget w, XmStepperWidget sw,
			       XtCallbackProc callback_proc )
{
    if( (sw->stepper.timer == (XtIntervalId)NULL) ||
        (sw->stepper.timeout_proc != callback_proc) )
    {
	sw->stepper.repeat_count = 0;
	sw->stepper.timeout_proc = callback_proc;
	sw->stepper.active_widget = w;
	sw->stepper.interval = sw->stepper.time_interval;
    }
    else
    {
	if( (sw->stepper.interval -=
	     (sw->stepper.interval / sw->stepper.time_ddelta))
	    < xmStepperClassRec.stepper_class.minimum_time_interval )
	    sw->stepper.interval =
	      xmStepperClassRec.stepper_class.minimum_time_interval;
    }
    sw->stepper.timer =
	XtAppAddTimeOut(XtWidgetToApplicationContext(w),
			sw->stepper.interval, (XtTimerCallbackProc)RepeatDigitButton, sw);
    return TRUE;
}


/*  Subroutine:	WarningCallback
 *  Effect:	Increase stepper value when increase button is "armed"
 */
/* ARGSUSED */
static void WarningCallback( Widget w, XmStepperWidget sw,
			    XmNumberWarningCallbackStruct *call_data )
{
    XtCallCallbacks((Widget)sw, XmNwarningCallback, call_data);
}
/*  Subroutine:	ButtonIncrease
 *  Effect:	Increase stepper value when increase button is "armed"
 */
/* ARGSUSED */
static void ButtonIncrease( Widget w, XmStepperWidget sw,
			    XmAnyCallbackStruct *call_data )
{
    if(   ButtonArmEvent(w, sw, (XtCallbackProc)ButtonIncrease)
       && IncrementStepper(sw, TRUE, sw->stepper.value_step.d) )
	CallbackStep(sw, TRUE);
}


/*  Subroutine:	ButtonDecrease
 *  Effect:	Decrease stepper value when increase button is "armed"
 */
/* ARGSUSED */
static void ButtonDecrease( Widget w, XmStepperWidget sw,
			    XmAnyCallbackStruct *call_data )
{
    if(   ButtonArmEvent(w, sw, (XtCallbackProc)ButtonDecrease)
       && IncrementStepper(sw, FALSE, sw->stepper.value_step.d) )
	CallbackStep(sw, FALSE);
}


/*  Subroutine:	ButtonIsReleased
 *  Effect:	Release auto-repeat when button is disarmed.
 */
/* ARGSUSED */
static void ButtonIsReleased( Widget w, XmStepperWidget sw,
			      XmAnyCallbackStruct *call_data )
{
    /*  Button press or release for another button is not of interest  */
    if( sw->stepper.timer )
    {
	if( sw->stepper.timer )
	    {
	    XtRemoveTimeOut(sw->stepper.timer);
	    }
	sw->stepper.active_widget = NULL;
	sw->stepper.repeat_count = 0;
	sw->stepper.timeout_proc = NULL;
	sw->stepper.timer = (XtIntervalId)NULL;
	CallbackActivate(sw);
    }
}


/*  Subroutine:	CallbackActivate
 *  Purpose:	Call registered callbacks to report final value change
 */
static void CallbackActivate( XmStepperWidget sw )
{
    XmDoubleCallbackStruct call_value;

    /* don't call the callbacks if value doesn't change */
    if(!sw->stepper.value_changed)
	return;

    call_value.value = sw->stepper.value.d;
    call_value.reason = XmCR_ACTIVATE;
    XtCallCallbacks((Widget)sw, XmNactivateCallback, &call_value);
    sw->stepper.value_changed = FALSE;
}


/*  Subroutine:	CallbackStep
 *  Purpose:	Call registered callbacks to report value change while stepping
 */
static void CallbackStep( XmStepperWidget sw, Boolean increase )
{
    XmDoubleCallbackStruct call_value;

    call_value.value = sw->stepper.value.d;
    call_value.reason = increase ? XmCR_INCREMENT : XmCR_DECREMENT;
    XtCallCallbacks((Widget)sw, XmNactivateCallback, &call_value);
}


/*  Subroutine:	RepeatDigitButton
 *  Effect:	Fake a button activate event for the currently active button
 *		 (called by the timer for autorepeat).
 */
static void RepeatDigitButton( XmStepperWidget sw, XtIntervalId *id )
{
    XmAnyCallbackStruct call_data;
    
    /*  If not our event or repetition button was cancelled, ignore  */
    if( *id != sw->stepper.timer )
	{
	return;
	}
    call_data.event = NULL;
    call_data.reason = XmCR_REPEAT_ARM;
    (sw->stepper.timeout_proc)(sw->stepper.active_widget, sw, &call_data);
}

static
Boolean UpdateMultitypeData(XmStepperWidget sw, MultitypeData* new, 
		MultitypeData* current )
{

    if (sw->stepper.data_type == INTEGER)
    {
        if( new->i != current->i )
        {
            new->d = (double)new->i;
            new->f = (float)new->i;
	    return True;
        }
    }
    else if (sw->stepper.data_type == FLOAT)
    {
        if( new->f != current->f )
        {
            new->d = (double)new->f;
            new->i = 0;
	    return True;
        }
    }
    else if (sw->stepper.data_type == DOUBLE)
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



/*  Subroutine:	XmCreateStepper
 *  Purpose:	This function creates and returns a Stepper widget.
 */
Widget XmCreateStepper( Widget parent, String name,
			ArgList args, Cardinal num_args )
{
    return XtCreateWidget(name, xmStepperWidgetClass, parent, args, num_args);
}
