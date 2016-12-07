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
/*
 * Templates of the local subroutines
 */
#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/MessageB.h>
#include "Stepper.h"
#include "Number.h"
#include "SlideBar.h"
#include "FrameControl.h"
#include "FrameControlP.h"

static void Initialize( XmFrameControlWidget request, XmFrameControlWidget new );
static Boolean SetValues( XmFrameControlWidget current,
                          XmFrameControlWidget request,
                          XmFrameControlWidget new );
static void Destroy( XmFrameControlWidget sw );

static void NumberFromGuide( XmSlideBarWidget        slide_bar, 
			     XmFrameControlWidget    frame_control,
                             SlideBarCallbackStruct* call_data);
static void CallbackFromNumber	(XmNumberWidget w,
				 XmFrameControlWidget frame_control,
				 XmDoubleCallbackStruct* call_data);
static XmNumberWidget CreateFrameControlNumber( int x,int y,int label_offset,
					  int min,int value,
					  int max, char* title,
					  Widget *label, 
				 	  XmFrameControlWidget frame_control,
					  Boolean above, 
					  Boolean editable,
					  Boolean make_stepper,
					  Boolean label_is_widget,
					  Boolean create_arrow,
					  Pixel color );
static Widget CreateNumberArrow(XmFrameControlWidget 	parent, 
			      Widget	 		ref, 
			      int 			xpos,
			      Pixel 			color);

static void	AdjustIncrementLimits	(XmFrameControlWidget frame_control);
static void ChangeSlideBarValue( XmFrameControlWidget frame_control, int value,
			  int which, Boolean passed_from_outside );

extern void _XmForegroundColorDefault();
extern void _XmBackgroundColorDefault();

/* Declare defaults */
#define DEF_MIN 1
#define DEF_MAX 100
#define DEF_STEP 1

static XtResource resources[] =
{
    {
      XmNstart, XmCStart, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.start_value),
      XmRImmediate, (XtPointer) DEF_MIN
    },
    {
      XmNnext, XmCNext, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.next_value),
      XmRImmediate, (XtPointer) DEF_MIN
    },
    {
      XmNcurrent, XmCCurrent, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.current_value),
      XmRImmediate, (XtPointer) (DEF_MIN + 1)
    },
    {
      XmNstop, XmCStop, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.stop_value),
      XmRImmediate, (XtPointer) DEF_MIN
    },
    {
      XmNminimum, XmCMinimum, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.min_value),
      XmRImmediate, (XtPointer) DEF_MIN
    },
    {
      XmNincrement, XmCIncrement, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.increment),
      XmRImmediate, (XtPointer) DEF_MIN
    },
    {
      XmNmaximum, XmCMaximum, XmRShort, sizeof(short),
      XtOffset(XmFrameControlWidget, frame_control.max_value),
      XmRImmediate, (XtPointer) DEF_MIN
    },
    {
      XmNvalueCallback, XmCValueCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmFrameControlWidget, frame_control.value_callback),
      XmRCallback, NULL
    },
    {
      XmNlimitColor, XmCLimitColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmFrameControlWidget, frame_control.limit_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNnextColor, XmCNextColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmFrameControlWidget, frame_control.next_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNcurrentColor, XmCCurrentColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmFrameControlWidget, frame_control.current_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNcurrentVisible, XmCCurrentVisible, XmRBoolean, sizeof(Boolean),
      XtOffset(XmFrameControlWidget, frame_control.current_visible),
      XmRImmediate, (XtPointer) True
    },
    {
      XmNminSensitive, XmCMinSensitive, XmRBoolean, sizeof(Boolean),
      XtOffset(XmFrameControlWidget, frame_control.min_sensitive),
      XmRImmediate, (XtPointer) TRUE
    },
    {
      XmNmaxSensitive, XmCMaxSensitive, XmRBoolean, sizeof(Boolean),
      XtOffset(XmFrameControlWidget, frame_control.max_sensitive),
      XmRImmediate, (XtPointer) TRUE
    },
    {
      XmNincSensitive, XmCIncSensitive, XmRBoolean, sizeof(Boolean),
      XtOffset(XmFrameControlWidget, frame_control.inc_sensitive),
      XmRImmediate, (XtPointer) TRUE
    },
};


/*  Maintain actions support for basic manager functioning  */
static char defaultTranslations[] =
    "<EnterWindow>:  Enter() \n\
     <FocusIn>:      FocusIn()";

extern void _XmManagerEnter();
extern void _XmManagerFocusIn();

static XtActionsRec actionsList[] =
{
   { "Enter",    (XtActionProc) _XmManagerEnter },
   { "FocusIn",  (XtActionProc) _XmManagerFocusIn },
};


/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

XmFrameControlClassRec xmFrameControlClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmFormClassRec,		/* superclass         */
      "XmFrameControl",				/* class_name         */
      sizeof(XmFrameControlRec),			/* widget_size        */
      NULL,					/* class_initialize   */
      NULL,					/* class_part_init    */
      FALSE,					/* class_inited       */
      (XtInitProc) Initialize,			/* initialize         */
      NULL,					/* initialize_hook    */
      (XtRealizeProc)_XtInherit,		/* realize            */
      actionsList,				/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      FALSE,					/* compress_motion    */
      FALSE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      (XtWidgetProc) Destroy,			/* destroy            */
      (XtWidgetProc)_XtInherit,		/* resize             */
      (XtExposeProc)_XtInherit,			/* expose             */
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
      (XtWidgetProc)_XtInherit,	/* from BulletinBoard *//* change_managed */
      (XtWidgetProc)_XtInherit,			/* insert_child       */
      (XtWidgetProc)_XtInherit,			/* delete_child       */
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      sizeof(XmFormConstraintRec),		/* constraint size      */   
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

   {		/* form class - none */
      NULL,					/* mumble */
   },

   {		/* frame_control class - none */
      NULL,						/* mumble */
   }
};

WidgetClass xmFrameControlWidgetClass = (WidgetClass) &xmFrameControlClassRec;


/*  Subroutine:	Initialize
 *  Purpose:	Create and Initialize the component widgets
 */

static void Initialize( XmFrameControlWidget request, XmFrameControlWidget new ) 
{
Arg wargs[32];
int n;
char string[100];
XmString xmstring;
Widget arrow;

    /*  Windows are mapped in reverse order, those created first are on top  */

    new->frame_control.next_number =
    CreateFrameControlNumber(38, 15, 3, new->frame_control.start_value, 
                               new->frame_control.next_value,
                               new->frame_control.stop_value, 
                               "Next", 
                               (Widget *)&new->frame_control.next_label,
		               new, TRUE, TRUE, TRUE, TRUE, TRUE,
			       new->frame_control.next_color);

    new->frame_control.start_number =
    CreateFrameControlNumber(5, 15, 3, new->frame_control.min_value, 
                              new->frame_control.start_value, 
                              new->frame_control.stop_value,
			      "Start", 
                              (Widget *)&new->frame_control.start_label, 
                              new, TRUE, TRUE,
			      TRUE, FALSE, TRUE,
			      new->frame_control.limit_color);

    new->frame_control.stop_number =
    CreateFrameControlNumber(70, 15, 3, new->frame_control.start_value, 
                               new->frame_control.stop_value,
			       new->frame_control.max_value, 
                               "End", 
                               (Widget *)&new->frame_control.stop_label,
			       new, TRUE, TRUE, 
                               TRUE, FALSE, TRUE,
			       new->frame_control.limit_color);

    new->frame_control.inc_stepper = (XmStepperWidget)
    CreateFrameControlNumber(38, 75, 3, 1, 
                              new->frame_control.increment,
			      16383, 
                              "Increment",
			      (Widget *)&new->frame_control.inc_label, 
                              new, FALSE, TRUE, 
                              TRUE, TRUE, FALSE, None);

    new->frame_control.min_number =
    CreateFrameControlNumber(12, 75, 2, -16383, 
                               new->frame_control.min_value, 
                               16383, 
                               "Min",
			       (Widget *)&new->frame_control.min_label, 
                               new, FALSE, TRUE, 
                               FALSE, FALSE, FALSE, None);

    new->frame_control.max_number =
    CreateFrameControlNumber(79, 75, 2, -16383, 
                                new->frame_control.max_value, 
                                16383,
			        "Max", 
                                (Widget *)&new->frame_control.max_label, 
                                new, FALSE, TRUE, 
                                FALSE, FALSE, FALSE, None);

    /*  Create the actual frame_control part of the popup  */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment,    XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNtopPosition,      47);                n++;
    XtSetArg(wargs[n], XmNleftAttachment,   XmATTACH_FORM);     n++;
    XtSetArg(wargs[n], XmNleftOffset,       5);                 n++;
    XtSetArg(wargs[n], XmNrightAttachment,  XmATTACH_FORM);     n++;
    XtSetArg(wargs[n], XmNrightOffset,      5);                 n++;
#ifdef COMMENT
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_NONE);     n++;
#endif

    XtSetArg(wargs[n], XmNstart,          new->frame_control.start_value); n++;
    XtSetArg(wargs[n], XmNstop,           new->frame_control.stop_value);  n++;
    XtSetArg(wargs[n], XmNnext,           new->frame_control.next_value);  n++;
    XtSetArg(wargs[n], XmNmaxValue,       new->frame_control.max_value);   n++;
    XtSetArg(wargs[n], XmNmarkColor,      new->frame_control.next_color);  n++;
    XtSetArg(wargs[n], XmNcurrentColor,   new->frame_control.current_color);
    n++;
    XtSetArg(wargs[n], XmNlimitColor,     new->frame_control.limit_color); n++;
    XtSetArg(wargs[n], XmNminValue,       new->frame_control.min_value);   n++;
    XtSetArg(wargs[n], XmNcurrent,        new->frame_control.current_value);
    n++;
    XtSetArg(wargs[n], XmNcurrentVisible, new->frame_control.current_visible);
    n++;

    new->frame_control.bar =
	(XmSlideBarWidget)XmCreateSlideBar
	    ((Widget)new, "frame_control_bar", wargs, n); 

    XtAddCallback
	((Widget)new->frame_control.bar, XmNvalueCallback, (XtCallbackProc)NumberFromGuide, new);
    XtManageChild((Widget)new->frame_control.bar);

    /* Set up "current" arrow */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNleftPosition, 39); n++;
    XtSetArg(wargs[n], XmNtopPosition, 36); n++;
    XtSetArg(wargs[n], XmNwidth, 16); n++;
    XtSetArg(wargs[n], XmNheight, 16); n++;
    XtSetArg(wargs[n], XmNforeground,  new->frame_control.current_color); n++;
    XtSetArg(wargs[n], XmNarrowDirection, XmARROW_DOWN); n++;
    XtSetArg(wargs[n], XmNshadowThickness, 0); n++;
    arrow = XmCreateArrowButton((Widget)new, "arrow2", wargs, n);
    XtManageChild(arrow);

    /* Set up "current" label */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNresizable, False); n++;
    XtSetArg(wargs[n], XmNleftWidget, arrow); n++;
    XtSetArg(wargs[n], XmNtopPosition, 35); n++;

    /* Create a large label, so subsequent resizes are not a problem */
    if (new->frame_control.current_visible)
	{
	sprintf(string,"Current: %d    ",new->frame_control.current_value);
	}
    else
	{
	sprintf(string,"Current:        ");
	}
    xmstring = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET); 
    XtSetArg(wargs[n], XmNlabelString, xmstring); n++;
    new->frame_control.current_label = 
	(XmLabelWidget)XmCreateLabel((Widget)new,"label", wargs, n);
    XmStringFree(xmstring);
    XtManageChild((Widget)new->frame_control.current_label);

    n = 0;
    XtSetArg(wargs[n], XmNtitle, "Frame Control Warning"); n++;
    new->frame_control.warning_dialog = 
	XmCreateWarningDialog(XtParent(new), "FCWarning", wargs, n);
    XtUnmanageChild(XmMessageBoxGetChild(new->frame_control.warning_dialog, 
		XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(new->frame_control.warning_dialog, 
		XmDIALOG_HELP_BUTTON));
}

static Boolean SetValues( XmFrameControlWidget current,
                          XmFrameControlWidget request,
                          XmFrameControlWidget new )
{
Arg wargs[16];
double dval;
FrameControlCallbackStruct callback_data;
int n;
char string[100];
XmString xmstring;

    if(new->frame_control.current_visible != 
	current->frame_control.current_visible)
	{
	XtSetArg(wargs[0], XmNcurrentVisible, 
			new->frame_control.current_visible);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);
	if (new->frame_control.current_visible)
	    {
	    sprintf(string,"Current: %d",new->frame_control.current_value);
	    }
	else
	    {
	    sprintf(string,"Current:");
	    }
	xmstring = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET); 
	n = 0;
	XtSetArg(wargs[n], XmNlabelString, xmstring); n++;
	XtSetValues((Widget)new->frame_control.current_label, wargs, n);
	XmStringFree(xmstring);
	}
    if(new->frame_control.current_value != current->frame_control.current_value)
	{
	XtSetArg(wargs[0], XmNcurrent, new->frame_control.current_value);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);
	if (new->frame_control.current_visible)
	    {
	    sprintf(string,"Current: %d",new->frame_control.current_value);
	    }
	else
	    {
	    sprintf(string,"Current:");
	    }
	xmstring = XmStringCreate(string, XmSTRING_DEFAULT_CHARSET); 
	n = 0;
	XtSetArg(wargs[n], XmNlabelString, xmstring); n++;
	XtSetValues((Widget)new->frame_control.current_label, wargs, n);
	XmStringFree(xmstring);
	}
    if(new->frame_control.start_value != current->frame_control.start_value)
	{
	XtSetArg(wargs[0], XmNstart, new->frame_control.start_value);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);
	dval = new->frame_control.start_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)new->frame_control.start_number, wargs, 1);
	DoubleSetArg(wargs[0], XmNdMinimum, dval);
	XtSetValues((Widget)new->frame_control.next_number, wargs, 1);
	XtSetValues((Widget)new->frame_control.stop_number, wargs, 1);
	}
    if(new->frame_control.stop_value != current->frame_control.stop_value)
	{
	XtSetArg(wargs[0], XmNstop, new->frame_control.stop_value);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);
	dval = new->frame_control.stop_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)new->frame_control.stop_number, wargs, 1);
	DoubleSetArg(wargs[0], XmNdMaximum, dval);
	XtSetValues((Widget)new->frame_control.start_number, wargs, 1);
	XtSetValues((Widget)new->frame_control.next_number, wargs, 1);
	}
    if(new->frame_control.next_value != current->frame_control.next_value)
	{
	XtSetArg(wargs[0], XmNnext, new->frame_control.next_value);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);
	dval = new->frame_control.next_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)new->frame_control.next_number, wargs, 1);
	}
    if(new->frame_control.increment != current->frame_control.increment)
	{
	dval = new->frame_control.increment;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)new->frame_control.inc_stepper, wargs, 1);
	}
    if(new->frame_control.min_value != current->frame_control.min_value)
	{
	dval = (double)new->frame_control.min_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)new->frame_control.min_number, wargs, 1);
	if (new->frame_control.start_value < new->frame_control.min_value)
	    {
	    n = 0;
	    new->frame_control.start_value = new->frame_control.min_value;
	    DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	    DoubleSetArg(wargs[n], XmNdMinimum, dval); n++;
	    if (new->frame_control.min_value > new->frame_control.stop_value)
		{
	    	DoubleSetArg(wargs[n], XmNdMaximum, dval); n++;
		}
	    XtSetValues((Widget)new->frame_control.start_number, wargs, n);
	    DoubleSetArg(wargs[0], XmNdMinimum, dval);
	    XtSetValues((Widget)new->frame_control.next_number, wargs, 1);
	    XtSetValues((Widget)new->frame_control.stop_number, wargs, 1);

	    callback_data.reason = XmCR_START;
	    callback_data.value = new->frame_control.min_value;
	    XtCallCallbacks((Widget)new, XmNvalueCallback, &callback_data);
	    if (new->frame_control.next_value < new->frame_control.min_value)
		{
	        n = 0;
		new->frame_control.next_value = 
				new->frame_control.start_value;
	    	DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	    	DoubleSetArg(wargs[n], XmNdMinimum, dval); n++;
	    	if (new->frame_control.min_value > 
				new->frame_control.stop_value)
		    {
	    	    DoubleSetArg(wargs[n], XmNdMaximum, dval); n++;
		    }
	    	XtSetValues((Widget)new->frame_control.next_number, wargs, n);
		callback_data.reason = XmCR_NEXT;
		callback_data.value = new->frame_control.start_value;
		XtCallCallbacks((Widget)new, XmNvalueCallback, &callback_data);
	        if (new->frame_control.stop_value <new->frame_control.min_value)
		    {
		    new->frame_control.stop_value =new->frame_control.min_value;
	            n = 0;
	    	    DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	    	    DoubleSetArg(wargs[n], XmNdMinimum, dval); n++;
		    if (new->frame_control.min_value > 
					new->frame_control.max_value)
			{
			DoubleSetArg(wargs[n], XmNdMaximum, dval); n++;
			}
	    	    XtSetValues((Widget)new->frame_control.stop_number, wargs, n);
		    callback_data.reason = XmCR_STOP;
		    callback_data.value = new->frame_control.stop_value;
		    XtCallCallbacks((Widget)new, XmNvalueCallback, &callback_data);
	            if (new->frame_control.min_value > 
					new->frame_control.max_value)
		        {
		        new->frame_control.max_value = 
					new->frame_control.min_value;
			DoubleSetArg(wargs[0], XmNdValue, dval);
			XtSetValues((Widget)new->frame_control.max_number, wargs, 1);
		        }
		    }
		}
	    }
	DoubleSetArg(wargs[0], XmNdMinimum, dval);
	XtSetValues((Widget)new->frame_control.start_number, wargs, 1);

	XtSetArg(wargs[0], XmNminValue, new->frame_control.min_value);
	XtSetArg(wargs[1], XmNmaxValue, new->frame_control.max_value);
	XtSetArg(wargs[2], XmNstart, new->frame_control.start_value);
	XtSetArg(wargs[3], XmNstop, new->frame_control.stop_value);
	XtSetArg(wargs[4], XmNnext, new->frame_control.next_value);
	XtSetValues((Widget)new->frame_control.bar, wargs, 5);
	}
    if(new->frame_control.max_value != current->frame_control.max_value)
	{
	dval = (double)new->frame_control.max_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)new->frame_control.max_number, wargs, 1);
	if (new->frame_control.stop_value > new->frame_control.max_value)
	    {
	    n = 0;
	    new->frame_control.stop_value = new->frame_control.max_value;

	    DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	    DoubleSetArg(wargs[n], XmNdMaximum, dval); n++;
	    if (new->frame_control.max_value < new->frame_control.start_value)
		{
	        DoubleSetArg(wargs[n], XmNdMinimum, dval); n++;
		}
	    XtSetValues((Widget)new->frame_control.stop_number, wargs, n);
	    DoubleSetArg(wargs[0], XmNdMaximum, dval);
	    XtSetValues((Widget)new->frame_control.start_number, wargs, 1);
	    XtSetValues((Widget)new->frame_control.next_number, wargs, 1);
	    callback_data.reason = XmCR_STOP;
	    callback_data.value = new->frame_control.max_value;
	    XtCallCallbacks((Widget)new, XmNvalueCallback, &callback_data);
	    if (new->frame_control.next_value > new->frame_control.max_value)
		{
		n = 0;
		new->frame_control.next_value = 
					new->frame_control.stop_value;
	        DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	        DoubleSetArg(wargs[n], XmNdMaximum, dval); n++;
	        if (new->frame_control.max_value < 
					new->frame_control.start_value)
		    {
	            DoubleSetArg(wargs[n], XmNdMinimum, dval); n++;
		    }
	        XtSetValues((Widget)new->frame_control.next_number, wargs, n);
		callback_data.reason = XmCR_NEXT;
		callback_data.value = new->frame_control.stop_value;
		XtCallCallbacks((Widget)new, XmNvalueCallback, &callback_data);
	        if (new->frame_control.start_value > 
					new->frame_control.max_value)
		    {
		    new->frame_control.start_value = 
					new->frame_control.stop_value;
		    n = 0;
	            DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	            DoubleSetArg(wargs[n], XmNdMaximum, dval); n++;
	            if (new->frame_control.max_value < 
					new->frame_control.min_value)
			{
	                DoubleSetArg(wargs[n], XmNdMinimum, dval); n++;
			}
	            XtSetValues((Widget)new->frame_control.start_number, wargs, n);
		    callback_data.reason = XmCR_START;
		    callback_data.value = new->frame_control.start_value;
		    XtCallCallbacks((Widget)new, XmNvalueCallback, &callback_data);
	            if (new->frame_control.min_value > 
					new->frame_control.max_value)
		        {
		        new->frame_control.min_value = 
					new->frame_control.max_value;
			DoubleSetArg(wargs[0], XmNdValue, dval);
			XtSetValues((Widget)new->frame_control.min_number, wargs, 1);
		        }
		    }
		}
	    }
	DoubleSetArg(wargs[0], XmNdMaximum, dval);
	XtSetValues((Widget)new->frame_control.stop_number, wargs, 1);

	XtSetArg(wargs[0], XmNminValue, new->frame_control.min_value);
	XtSetArg(wargs[1], XmNmaxValue, new->frame_control.max_value);
	XtSetArg(wargs[2], XmNstart, new->frame_control.start_value);
	XtSetArg(wargs[3], XmNstop, new->frame_control.stop_value);
	XtSetArg(wargs[4], XmNnext, new->frame_control.next_value);
	XtSetValues((Widget)new->frame_control.bar, wargs, 5);
	}
    if(new->frame_control.current_color != current->frame_control.current_color)
	{
	XtSetArg(wargs[0], XmNcurrentColor, new->frame_control.current_color);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);

	/* Set the color of the associated label widget */
	XtSetArg(wargs[0], XmNforeground, new->frame_control.current_color);
	XtSetValues((Widget)new->frame_control.current_label, wargs, 1);
	}
    if(new->frame_control.next_color != current->frame_control.next_color)
	{
	XtSetArg(wargs[0], XmNmarkColor, new->frame_control.next_color);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);

	/* Set the color of the associated label widget */
	XtSetArg(wargs[0], XmNforeground, new->frame_control.next_color);
	XtSetValues((Widget)new->frame_control.next_label, wargs, 1);
	}
    if(new->frame_control.limit_color != current->frame_control.limit_color)
	{
	XtSetArg(wargs[0], XmNlimitColor, new->frame_control.limit_color);
	XtSetValues((Widget)new->frame_control.bar, wargs, 1);

	/* Set the color of the associated label widget */
	XtSetArg(wargs[0], XmNforeground, new->frame_control.limit_color);
	XtSetValues((Widget)new->frame_control.start_label, wargs, 1);

	/* Set the color of the associated label widget */
	XtSetArg(wargs[0], XmNforeground, new->frame_control.limit_color);
	XtSetValues((Widget)new->frame_control.stop_label, wargs, 1);
	}
    if(new->frame_control.min_sensitive != current->frame_control.min_sensitive)
	{
	XtSetSensitive((Widget)new->frame_control.min_number, 
		new->frame_control.min_sensitive);
	}
    if(new->frame_control.max_sensitive != current->frame_control.max_sensitive)
	{
	XtSetSensitive((Widget)new->frame_control.max_number, 
		new->frame_control.max_sensitive);
	}
    if(new->frame_control.inc_sensitive != current->frame_control.inc_sensitive)
	{
	XtSetSensitive((Widget)new->frame_control.inc_stepper, 
		new->frame_control.inc_sensitive);
	}
    XFlush(XtDisplay(new));
    return True;
}

static void Destroy( XmFrameControlWidget sw )
{

XtRemoveAllCallbacks((Widget)sw, XmNvalueCallback);

}
/*  Subroutine:	CreateNumber Arrow
 *  Purpose:	 Create the arrow indicator on top of the Stepper
 */
static Widget CreateNumberArrow(XmFrameControlWidget 	parent, 
			      Widget	 		ref, 
			      int 			xpos,
			      Pixel 			color)
{
Arg wargs[30];
int n;
Widget arrow;

    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNleftPosition, xpos); n++;
    XtSetArg(wargs[n], XmNbottomWidget, ref); n++;
    XtSetArg(wargs[n], XmNbottomOffset, 3); n++;
    XtSetArg(wargs[n], XmNwidth, 16); n++;
    XtSetArg(wargs[n], XmNheight, 16); n++;
    XtSetArg(wargs[n], XmNforeground, color); n++;
    XtSetArg(wargs[n], XmNarrowDirection, XmARROW_DOWN); n++;
    XtSetArg(wargs[n], XmNshadowThickness, 0); n++;
    arrow = XmCreateArrowButton((Widget)parent, "arrow", wargs, n); 
    XtManageChild(arrow);
    return arrow;

}

/*  Subroutine:	CreateFrameControlNumber
 *  Purpose:	Handle creation of number (digital readout) widget
 */
static XmNumberWidget CreateFrameControlNumber( int x, int y, int label_offset,
					  int min, 
					  int value,
					  int max, char* title,
					  Widget *label, 
					  XmFrameControlWidget fc,
					  Boolean above, Boolean editable,
					  Boolean make_stepper,
					  Boolean label_is_widget,
					  Boolean create_arrow,
					  Pixel color )
{
int n;
Arg wargs[20];
double dmin, dmax, dval;
Widget number;
Widget arrow=0;
XmString text;

    /*
     *  Create the panel-meter like number display
     */
    /*  The stepper's values are absolute  */
    dmin = (double)min;
    dmax = (double)max;
    dval = (double)value;

    /*  Placement gets recomputed in the expose callback  */
    n = 0;
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_POSITION); n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_NONE); n++;
    XtSetArg(wargs[n], XmNleftPosition, x); n++;
    XtSetArg(wargs[n], XmNtopPosition, y); n++;


    /*  Estimate a comfortably large size to initially create NumberWidget  */
    DoubleSetArg(wargs[n], XmNdMinimum, dmin); n++;
    DoubleSetArg(wargs[n], XmNdMaximum, dmax); n++;
    DoubleSetArg(wargs[n], XmNdValue, dval); n++;
    XtSetArg(wargs[n], XmNdecimalPlaces, 0); n++;

    /*  Center text in the field and resize for tighter fit of font  */
    XtSetArg(wargs[n], XmNcenter, TRUE); n++;
    XtSetArg(wargs[n], XmNrecomputeSize, FALSE); n++;
    if( editable )
    {
	XtSetArg(wargs[n], XmNeditable, TRUE); n++;
    }
    else
    {
	XtSetArg(wargs[n], XmNeditable, FALSE); n++;
    }
    if( make_stepper )
    {
	XtSetArg(wargs[n], XmNdigits, 7); n++;
	XtSetArg(wargs[n], XmNdataType, DOUBLE); n++;
	number = XmCreateStepper((Widget)fc, title, wargs, n);
    }
    else
    {
	XtSetArg(wargs[n], XmNcharPlaces, 7); n++;
	XtSetArg(wargs[n], XmNdataType, DOUBLE); n++;
	number = XmCreateNumber((Widget)fc, title, wargs, n);
    }
    /*  Callback for when a new value is entered directly  */
    XtAddCallback((Widget)number, XmNactivateCallback, (XtCallbackProc)CallbackFromNumber, fc);
    XtManageChild(number);

    if (create_arrow)
	{
    	arrow = CreateNumberArrow(fc, number, x + label_offset, color);
	}
    if(create_arrow)
	{
	XtSetArg(wargs[0], XmNleftAttachment, XmATTACH_WIDGET);
	XtSetArg(wargs[1], XmNleftWidget, arrow);
	}
    else
	{
	XtSetArg(wargs[0], XmNleftAttachment, XmATTACH_POSITION);
	XtSetArg(wargs[1], XmNleftPosition, x + label_offset);
	}
    XtSetArg(wargs[2], XmNrightAttachment, XmATTACH_NONE);
    if( above )
    {
	XtSetArg(wargs[3], XmNtopAttachment, XmATTACH_NONE);
	XtSetArg(wargs[4], XmNbottomAttachment, XmATTACH_WIDGET);
	XtSetArg(wargs[5], XmNbottomWidget, number);
    }
    else
    {
	XtSetArg(wargs[3], XmNtopAttachment, XmATTACH_WIDGET);
	XtSetArg(wargs[4], XmNbottomAttachment, XmATTACH_NONE);
	XtSetArg(wargs[5], XmNtopWidget, number);
    }
    XtSetArg(wargs[6], XmNleftOffset, label_offset);

    /*  Create the string for the label, and then the label  */
    text = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[7], XmNlabelString, text);
    *label = XmCreateLabel((Widget)fc, "label", wargs, 8);
    XtManageChild(*label);
    XmStringFree(text);
    return (XmNumberWidget)number;
}

/*  Subroutine:	NumberFromGuide
 *  Purpose:	Change all values necessary in response to a value change
 *		from user manipulation of the VCR frame_control
 *  Note:	DISARM is a final value after the frame_control is released
 *  Note:	Numbers and ChangeVCRxxValue use external (min based) values
 */
static void NumberFromGuide( XmSlideBarWidget        slide_bar, 
			     XmFrameControlWidget    fc,
                             SlideBarCallbackStruct* call_data)
{
    Arg wargs[2];
    double dval;
    FrameControlCallbackStruct callback_data;

    if( call_data->indicator == XmCR_START )
    {
	fc->frame_control.start_value = call_data->value;
	dval = fc->frame_control.start_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)fc->frame_control.start_number, wargs, 1);
	if(call_data->reason==XmCR_DISARM)
	{
	    dval = (double)(call_data->value);
	    DoubleSetArg(wargs[0], XmNdMinimum, dval);
	    XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	    XtSetValues((Widget)fc->frame_control.stop_number, wargs, 1);
	    callback_data.reason = XmCR_START;
	    callback_data.value = call_data->value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    AdjustIncrementLimits(fc);
	}
    }
    else if( call_data->indicator == XmCR_STOP )
    {
	fc->frame_control.stop_value = call_data->value;
	dval = fc->frame_control.stop_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)fc->frame_control.stop_number, wargs, 1);
	if(call_data->reason==XmCR_DISARM)
	{
	    dval = (double)(call_data->value);
	    DoubleSetArg(wargs[0], XmNdMaximum, dval);
	    XtSetValues((Widget)fc->frame_control.start_number, wargs, 1);
	    XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	    callback_data.reason = XmCR_STOP;
	    callback_data.value = call_data->value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    AdjustIncrementLimits(fc);
	}
    }
    else
    {
	fc->frame_control.next_value = call_data->value;
	dval = fc->frame_control.next_value;
	DoubleSetArg(wargs[0], XmNdValue, dval);
	XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	if(call_data->reason==XmCR_DISARM)
	    {
	    callback_data.reason = XmCR_NEXT;
	    callback_data.value = call_data->value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    }
    }
}


/*  Subroutine:	CallbackFromNumber
 *  Purpose:	Callback routine from number widget to allow/disallow stepper
 *		to manipulate the value, or to set a new value.
 */
static void CallbackFromNumber( XmNumberWidget	nw,
			        XmFrameControlWidget fc,
			        XmDoubleCallbackStruct* call_data )
{
    int value, which;
    FrameControlCallbackStruct callback_data;
    Arg wargs[10];
    double dval;
    XmString xms;
#ifdef PASSDOUBLEVALUE
XtArgVal dx_l;
#endif

    if( call_data->reason == XmCR_ACTIVATE )
    {
	if (call_data->value > 0)
	    {
	    value = (int)(call_data->value + 0.5);
	    }
	else
	    {
	    value = (int)(call_data->value - 0.5);
	    }
	dval = call_data->value;
	if( nw == fc->frame_control.start_number )
	{
	    fc->frame_control.start_value = value;
	    if (fc->frame_control.start_value > 
			fc->frame_control.next_value)
		{
		fc->frame_control.next_value = value;
		DoubleSetArg(wargs[0], XmNdValue, dval);
		XtSetValues((Widget)fc->frame_control.next_number, 
					wargs, 1);
		callback_data.reason = XmCR_NEXT;
		callback_data.value = value;
    		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
		}
	    if (fc->frame_control.start_value > fc->frame_control.stop_value)
		{
		fc->frame_control.stop_value = value;
		DoubleSetArg(wargs[0], XmNdValue, dval);
		XtSetValues((Widget)fc->frame_control.stop_number, wargs, 1);
		callback_data.reason = XmCR_STOP;
		callback_data.value = value;
    		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
		}
	    which = XmCR_START;
	    callback_data.reason = XmCR_START;
	    callback_data.value = value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    ChangeSlideBarValue(fc, value, which, FALSE);
	}
	else if( nw == fc->frame_control.next_number )
	{
	    fc->frame_control.next_value = value;
	    if (fc->frame_control.next_value > fc->frame_control.stop_value)
		{
		fc->frame_control.stop_value = value;
		DoubleSetArg(wargs[0], XmNdValue, dval);
		XtSetValues((Widget)fc->frame_control.stop_number, wargs, 1);
		callback_data.reason = XmCR_STOP;
		callback_data.value = value;
    		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
		}
	    if (fc->frame_control.next_value < fc->frame_control.start_value)
		{
		fc->frame_control.start_value = value;
		DoubleSetArg(wargs[0], XmNdValue, dval);
		XtSetValues((Widget)fc->frame_control.start_number, wargs, 1);
		callback_data.reason = XmCR_START;
		callback_data.value = value;
    		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
		}
	    which = XmCR_NEXT;
	    callback_data.reason = XmCR_NEXT;
	    callback_data.value = value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    ChangeSlideBarValue(fc, value, which, FALSE);
	}
	else if( nw == fc->frame_control.stop_number )
	{
	    fc->frame_control.stop_value = value;
	    if (fc->frame_control.stop_value < fc->frame_control.next_value)
		{
		fc->frame_control.next_value = value;
		DoubleSetArg(wargs[0], XmNdValue, dval);
		XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
		callback_data.reason = XmCR_NEXT;
		callback_data.value = value;
		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
		}
	    if (fc->frame_control.stop_value < fc->frame_control.start_value)
		{
		fc->frame_control.start_value = value;
		DoubleSetArg(wargs[0], XmNdValue, dval);
		XtSetValues((Widget)fc->frame_control.start_number, wargs, 1);
		callback_data.reason = XmCR_START;
		callback_data.value = value;
    		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
		}
	    which = XmCR_STOP;
	    callback_data.reason = XmCR_STOP;
	    callback_data.value = value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    ChangeSlideBarValue(fc, value, which, FALSE);
	}
	else if( nw == fc->frame_control.min_number )
	{
	    if(!fc->frame_control.max_sensitive)
	    {
		XtVaGetValues((Widget)fc->frame_control.max_number, XmNdValue,
			DoubleVal(dval, dx_l), NULL);
		if(value > dval)
		{
		XtVaSetValues((Widget)fc->frame_control.min_number, XmNdValue,
			DoubleVal(dval, dx_l), NULL);
		value = dval;
		xms = XmStringCreateSimple("Min must be less than data driven max");
		XtVaSetValues((Widget)fc->frame_control.warning_dialog,
			XmNmessageString, xms,
			NULL);
		XmStringFree(xms);
		XtManageChild((Widget)fc->frame_control.warning_dialog);
		}
	    }
	    XtSetArg(wargs[0], XmNminimum, value);
	    XtSetValues((Widget)fc, wargs, 1);
	    callback_data.reason = XmCR_MIN;
	    callback_data.value = value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	}
	else if( nw == fc->frame_control.max_number )
	{
	    if(!fc->frame_control.min_sensitive)
	    {
		XtVaGetValues((Widget)fc->frame_control.min_number, XmNdValue,
			DoubleVal(dval, dx_l), NULL);
		if(value < dval)
		{
		XtVaSetValues((Widget)fc->frame_control.max_number, XmNdValue,
			DoubleVal(dval, dx_l), NULL);
		value = dval;
		xms = XmStringCreateSimple("Max must be greater than data driven min");
		XtVaSetValues((Widget)fc->frame_control.warning_dialog,
			XmNmessageString, xms,
			NULL);
		XmStringFree(xms);
		XtManageChild((Widget)fc->frame_control.warning_dialog);
		}
	    }
	    XtSetArg(wargs[0], XmNmaximum, value);
	    XtSetValues((Widget)fc, wargs, 1);
	    callback_data.reason = XmCR_MAX;
	    callback_data.value = value;
	    XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	}
	else
	{
	    if( (XmStepperWidget)nw == fc->frame_control.inc_stepper )
	    {
		fc->frame_control.increment = value;
		callback_data.reason = XmCR_INCREMENT;
		callback_data.value = value;
    		XtCallCallbacks((Widget)fc, XmNvalueCallback, &callback_data);
	    }
	    return;
	}
    }
}


/*  Subroutine:	ChangeSlideBarValue
 *  Purpose:	Adjust the guidebar markers and digital readouts for a new value
 */
static void ChangeSlideBarValue( XmFrameControlWidget fc, int value,
			  int which, Boolean passed_from_outside )
{
    Arg wargs[2];
    double dval;

    /*  Digital readout numbers are min based and double  */
    dval = (double)(value);
    if( which == XmCR_START )
    {
	fc->frame_control.start_value = value;

	/*  Reposition the frame_control marker  */
	XtSetArg(wargs[0], XmNstart, fc->frame_control.start_value);
	XtSetValues((Widget)fc->frame_control.bar, wargs, 1);

	/*  Change the digital readout if it wasn't the source  */
	if( passed_from_outside )
	    {
	    DoubleSetArg(wargs[0], XmNdValue, dval);
	    XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	    }
	/*  Reposition next value indicators if it was affected  */
	if( fc->frame_control.start_value > fc->frame_control.next_value )
	{
	    fc->frame_control.next_value = fc->frame_control.start_value;
	    XtSetArg(wargs[0], XmNnext, fc->frame_control.next_value);
	    XtSetValues((Widget)fc->frame_control.bar, wargs, 1);
	    DoubleSetArg(wargs[0], XmNdValue, dval);
	    XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	}
	/*  Reset the minimums on the next and stop digital readouts  */
	DoubleSetArg(wargs[0], XmNdMinimum, dval);
	XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	XtSetValues((Widget)fc->frame_control.stop_number, wargs, 1);
	AdjustIncrementLimits(fc);
    }
    else if( which == XmCR_NEXT )
    {
	fc->frame_control.next_value = value;

	/*  Reposition the frame_control marker  */
	XtSetArg(wargs[0], XmNnext, fc->frame_control.next_value);
	XtSetValues((Widget)fc->frame_control.bar, wargs, 1);

	/*  Change the digital readout if it wasn't the source  */
	if( passed_from_outside )
	    {
	    DoubleSetArg(wargs[0], XmNdValue, dval);
	    XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	    }
    }
    else if( which == XmCR_STOP )
    {
	fc->frame_control.stop_value = value;

	/*  Reposition the frame_control marker  */
	XtSetArg(wargs[0], XmNstop, fc->frame_control.stop_value);
	XtSetValues((Widget)fc->frame_control.bar, wargs, 1);

	/*  Change the digital readout if it wasn't the source  */
	if( passed_from_outside )
	    {
	    DoubleSetArg(wargs[0], XmNdValue, dval);
	    XtSetValues((Widget)fc->frame_control.stop_number, wargs, 1);
	    }
	/*  Reposition next value indicators if it was affected  */
	if( fc->frame_control.stop_value < fc->frame_control.next_value )
	{
	    fc->frame_control.next_value = fc->frame_control.stop_value;
	    XtSetArg(wargs[0], XmNnext, fc->frame_control.next_value);
	    XtSetValues((Widget)fc->frame_control.bar, wargs, 1);
	    DoubleSetArg(wargs[0], XmNdValue, dval);
	    XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	}

	/*  Reset the maximums on the next and start digital readouts  */
	DoubleSetArg(wargs[0], XmNdMaximum, dval);
	XtSetValues((Widget)fc->frame_control.start_number, wargs, 1);
	XtSetValues((Widget)fc->frame_control.next_number, wargs, 1);
	AdjustIncrementLimits(fc);
    }
    else if( which == XmCR_INCREMENT )
    {
	/*  Application is responsible for making sure this is within range  */
	XtSetArg(wargs[0], XmNiValue, value);
	XtSetValues((Widget)fc->frame_control.inc_stepper, wargs, 1);
    }
}


/*  Subroutine:	AdjustIncrementLimits
 *  Purpose:	Adjust the max limit of the increment stepper to reflect
 *		the current range
 */
static void AdjustIncrementLimits( XmFrameControlWidget fc )
{
#ifdef Comment
    Arg wargs[2];
    int ival;
    /*  Set the upper bound on the increment, but not less than the inc  */
    ival = fc->frame_control.stop_value - fc->frame_control.start_value;
    if( ival < fc->frame_control.increment )
	ival = fc->frame_control.increment;
    XtSetArg(wargs[0], XmNiMaximum, ival);
    XtSetValues((Widget)fc->frame_control.inc_stepper, wargs, 1);
#endif
}
 
/*   Subroutine: XmCreateFrameControl
 *   Purpose: This function creates and returns a FrameControl widget.
*/

Widget XmCreateFrameControl( Widget parent, char *name, ArgList args,
                           Cardinal num_args)
{
	return (XtCreateWidget(name, xmFrameControlWidgetClass, parent, args, 
				num_args));

}

