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
#include <math.h>
#include <limits.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/Xm.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/ArrowB.h>
#include <Xm/Scale.h>
#include "Number.h"
#include "Slider.h"
#include "SliderP.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif



#define  zero  pow(0.1,6)

#if !defined(HAVE_TRUNC)
#define trunc(c)        ((double)((int)(c)))
#endif

/*
 * Hi,
 * Subclassing off an XmForm is dangerous.  Changing basic widget methods here 
 * is like messing with someone's dna.  
 * 
 * This widget attempted not only to use the attachments of the parent class 
 * but also to redine the resize method.  oops.  Owning an XmForm might have 
 * been better than being a type of XmForm.  
 *
 * So be careful.  
 */

/*
 * Templates of the local subroutines
 */
static void Initialize( XmSliderWidget request, XmSliderWidget new );
static Boolean SetValues( XmSliderWidget current,
                          XmSliderWidget request,
                          XmSliderWidget new );
static void Destroy( XmSliderWidget sw );
static void CenterNumber( XmSliderWidget sw );

static void CallbackFromNumber	(XmNumberWidget w,
				 XmSliderWidget slider,
				 XmDoubleCallbackStruct* call_data);

static void CallbackFromArrow ( XmArrowButtonWidget   arrow,
				XmSliderWidget        slider,
				XmAnyCallbackStruct   *call_data );

static void CallbackFromScale ( XmScaleWidget scale,
	   			XmSliderWidget   slider,
				XmScaleCallbackStruct  *call_data );

static XmNumberWidget CreateSliderNumber( int x,int min,double value,
					  int max, char* title,
					  Widget *label, 
				 	  XmSliderWidget slider,
					  Boolean above, 
					  Boolean editable,
					  Boolean make_stepper,
					  Boolean label_is_widget );

static double widgetnumber2appl( XmSliderWidget slider,
		                 int number );
static int appl2widgetnumber( XmSliderWidget slider,
		              double application_number );
static  double  _dxf_round();


extern void _XmForegroundColorDefault();
extern void _XmBackgroundColorDefault();

/* Declare defaults */
#define DEF_MIN 1
#define DEF_MAX 100
#define DEF_STEP 1

#if defined(hp700) || defined(aviion) || defined(solaris) || defined(sun4) || defined (intelnt) || defined(OS2)
#define trunc(x) ((double)((int)(x)))
#endif

static double DefaultMinDbl = DEF_MIN;
static double DefaultMaxDbl = DEF_MAX;
static double DefaultCurrentDbl = (DEF_MIN + DEF_MAX) / 2; 

static XtResource resources[] =
{

   {
      XmNcurrent, XmCCurrent, XmRDouble, sizeof(double),
      XtOffset(XmSliderWidget, slider.application_current_value),
      XmRDouble, (XtPointer) &DefaultCurrentDbl 
   },
   {
      XmNmaximum, XmCMaximum, XmRDouble, sizeof(double),
      XtOffset(XmSliderWidget, slider.application_max_value),
      XmRDouble, (XtPointer) &DefaultMaxDbl
   },
   {
      XmNminimum, XmCMinimum, XmRDouble, sizeof(double),
      XtOffset(XmSliderWidget, slider.application_min_value),
      XmRDouble, (XtPointer)  &DefaultMinDbl
   },
    { XmNdataType, XmCDataType, XmRInt, sizeof(int),
      XtOffset(XmSliderWidget, slider.number_type),
      XmRImmediate, (XtPointer) INTEGER 
    },
    { XmNdecimalPlaces, XmCDecimalPlaces, XmRInt, sizeof(int),
      XtOffset(XmSliderWidget, slider.decimal_places),
      XmRImmediate, (XtPointer) 0 
    },
    {
      XmNincrement, XmCIncrement, XmRDouble, sizeof(double),
      XtOffset(XmSliderWidget, slider.increment),
      XmRDouble, (XtPointer) &DefaultMinDbl
    },
    {
      XmNvalueCallback, XmCValueCallback, XmRCallback, sizeof(XtPointer),
      XtOffset(XmSliderWidget, slider.value_callback),
      XmRCallback, NULL
    },
    {
      XmNlimitColor, XmCLimitColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmSliderWidget, slider.limit_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
    },
    {
      XmNcurrentColor, XmCCurrentColor, XmRPixel, sizeof(Pixel),
      XtOffset(XmSliderWidget, slider.current_color),
      XmRCallProc, (XtPointer) _XmForegroundColorDefault
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

XmSliderClassRec xmSliderClassRec = 
{
   {			/* core_class fields      */
      (WidgetClass) &xmFormClassRec,		/* superclass         */
      "XmSlider",				/* class_name         */
      (Cardinal)sizeof(XmSliderRec),		/* widget_size        */
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
      (XrmClass)NULLQUARK,			/* xrm_class          */
      False,					/* compress_motion    */
      False,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      (XtWidgetProc) Destroy,			/* destroy            */
      (XtWidgetProc)_XtInherit,			/* resize             */
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
      (XtStringProc)NULL,                       /* display_accelerator   */
      (XtPointer)NULL,		                /* extension             */
   },

   {		/* composite_class fields */
      XtInheritGeometryManager,	/* geometry_manager   */
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

   {		/* slider class - none */
      NULL,						/* mumble */
   }
};

WidgetClass xmSliderWidgetClass = (WidgetClass) &xmSliderClassRec;


/*  Subroutine:	Initialize
 *  Purpose:	Create and Initialize the component widgets
 */

static void Initialize( XmSliderWidget request, XmSliderWidget new ) 
{
#if (XmVersion < 1001)
short thick;
#else
Dimension thick;
#endif
int n = 0;
Dimension h;
Arg wargs[40], wargs2[40];
	
    if (new->core.width < 200)  new->core.width = 200;
    if (new->core.height < 55)  new->core.height = 55;

    if (new->slider.application_max_value == new->slider.application_min_value)
	new->slider.aslope = 1.0 / zero;
    else
        new->slider.aslope = 1.0 / (new->slider.application_max_value - 
				new->slider.application_min_value);
    new->slider.min_value = 0;
    if (new->slider.increment == 0)
    	new->slider.max_value = new->slider.min_value;	
    else 
    {  
        int places;
	places = new->slider.decimal_places;
	places = (places<2?2:places);
	places = (places>7?7:places);
    	new->slider.max_value =  pow ((double)10, (double)places); 
    }
    if (new->slider.max_value==new->slider.min_value)
    	new->slider.wslope = 1.0 / zero; 
    else
    	new->slider.wslope = 1.0 / (double)(new->slider.max_value - 
				new->slider.min_value);
    new->slider.current_number =
    CreateSliderNumber(7, new->slider.min_value, 
                          new->slider.application_current_value,
                          new->slider.max_value, 
                          "    current", 
                          (Widget *)&new->slider.current_label,
		          new, FALSE, TRUE, FALSE, FALSE);


    n = 0;
    XtSetArg ( wargs2[n], XmNarrowDirection, XmARROW_LEFT ); n++;
    
    new->slider.left_arrow = (XmArrowButtonWidget)
		XmCreateArrowButton((Widget)new, "left_arrow", wargs2, n);

    XtAddCallback((Widget)new->slider.left_arrow, XmNactivateCallback, (XtCallbackProc)CallbackFromArrow, new);
    XtManageChild((Widget)new->slider.left_arrow);

    n = 0;
    XtSetArg ( wargs2[n], XmNarrowDirection, XmARROW_RIGHT ); n++;
    
    new->slider.right_arrow = (XmArrowButtonWidget)
		XmCreateArrowButton((Widget)new, "right_arrow", wargs2, n );

    XtAddCallback((Widget)new->slider.right_arrow, XmNactivateCallback, (XtCallbackProc)CallbackFromArrow, new);
    XtManageChild((Widget)new->slider.right_arrow);

    XtSetArg ( wargs2[0], XmNheight, &h );
    XtGetValues ( (Widget)new->slider.right_arrow, wargs2, 1 );

    n = 0;
    new->slider.current_value = 
		appl2widgetnumber(new, new->slider.application_current_value);

    XtSetArg(wargs[n], XmNvalue, new->slider.current_value); n++;
    XtSetArg(wargs[n], XmNmaximum, new->slider.max_value); n++; 
    XtSetArg(wargs[n], XmNminimum, 0 ); n++; 
    XtSetArg(wargs[n], XmNscaleHeight, h); n++;  
    XtSetArg(wargs[n], XmNtopAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNleftOffset, 30); n++;
    XtSetArg(wargs[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNbottomOffset, 36); n++;
    XtSetArg(wargs[n], XmNrightAttachment, XmATTACH_FORM); n++;
    XtSetArg(wargs[n], XmNrightOffset, 30); n++;
    XtSetArg(wargs[n], XmNorientation, XmHORIZONTAL ); n++;
    XtSetArg(wargs[n], XmNprocessingDirection, XmMAX_ON_RIGHT ); n++;
#if (XmVersion < 1001)
    XtSetArg(wargs[n], XmNshadowThickness, new->manager.shadow_thickness); n++;
#else
    XtSetArg(wargs[n], XmNshadowThickness, 1); n++;
#endif


    new->slider.bar = 
	(XmScaleWidget)XmCreateScale((Widget)new, "scale", wargs, n); 

    XtAddCallback((Widget)new->slider.bar, XmNdragCallback,(XtCallbackProc)CallbackFromScale , new);
    XtAddCallback((Widget)new->slider.bar, XmNvalueChangedCallback,(XtCallbackProc)CallbackFromScale, 	new);
    XtManageChild((Widget)new->slider.bar);

    XtSetArg ( wargs2[0], XmNshadowThickness, &thick );
    XtGetValues ((Widget) new->slider.bar, wargs2, 1 );

#if (XmVersion < 1001)
   XtSetArg ( wargs2[0], XmNshadowThickness, thick );
#else
   XtSetArg ( wargs2[0], XmNshadowThickness, 0 );
#endif
   XtSetValues ((Widget) new->slider.left_arrow, wargs2, 1 );                     

#if (XmVersion < 1001)
   XtSetArg ( wargs2[0], XmNshadowThickness, thick );
#else
   XtSetArg ( wargs2[0], XmNshadowThickness, 0 );
#endif
   XtSetValues ((Widget) new->slider.right_arrow, wargs2, 1 );                     

   XtSetArg ( wargs2[0], XmNshadowThickness, thick );
   XtSetValues ((Widget) new->slider.current_number, wargs2, 1 );

   XtVaSetValues((Widget)new, XmNresizePolicy, XmRESIZE_NONE, NULL);

   /*
    * attach the arrows to the slider
    */
   XtVaSetValues ((Widget)new->slider.left_arrow, XmNleftAttachment, XmATTACH_NONE,
	XmNrightAttachment, XmATTACH_WIDGET, XmNrightWidget, new->slider.bar,
	XmNrightOffset, 2, XmNtopAttachment, XmATTACH_NONE,
	XmNtopWidget, new->slider.bar, XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget, new->slider.bar, XmNtopOffset, 0, XmNbottomOffset,
	1, NULL);
   XtVaSetValues ((Widget)new->slider.right_arrow, XmNrightAttachment, XmATTACH_NONE,
	XmNleftAttachment, XmATTACH_WIDGET, XmNleftWidget, new->slider.bar,
	XmNleftOffset, 2, XmNtopAttachment, XmATTACH_NONE,
	XmNtopWidget, new->slider.bar, XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	XmNbottomWidget, new->slider.bar, XmNtopOffset, 0, XmNbottomOffset,
	1, NULL);
   {Widget num = (Widget)new->slider.current_number;
   XtVaSetValues (num, XmNleftAttachment, XmATTACH_POSITION, XmNleftPosition, 50, 
	XmNleftOffset, -(num->core.width>>1), XmNbottomAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_NONE, 
	NULL);}
}

/* converts the integer from scale widget to the actucal number for the application */
static double widgetnumber2appl( XmSliderWidget slider,
				int widget_number ) 
{
double res;

	res = slider->slider.wslope *
		 ( ((widget_number - slider->slider.min_value) *  	
		    slider->slider.application_max_value)  +     
		   ((slider->slider.max_value - widget_number) *
		    slider->slider.application_min_value) ); 

	res = _dxf_round(res, slider->slider.decimal_places);
	if(res < slider->slider.application_min_value) 
		res = slider->slider.application_min_value;
	if(res > slider->slider.application_max_value) 
		res = slider->slider.application_max_value;
return ( res );
}	
				 



static int appl2widgetnumber( XmSliderWidget slider,
		              double application_number ) 
{
double res;

	res = 0.5 + slider->slider.aslope *
	 ( ((application_number - slider->slider.application_min_value) *  
	    slider->slider.max_value)  +     
	   ((slider->slider.application_max_value - application_number) *
	    slider->slider.min_value) ); 

return ( (int)(res) );
}	
				 

static Boolean SetValues( XmSliderWidget current,
                          XmSliderWidget request,
                          XmSliderWidget new )
{
Arg wargs[16];
double dval;
int n;
Boolean center_number = False;

    if(new->slider.application_current_value != 
       current->slider.application_current_value)
    {

        if (new->slider.application_current_value > 
	    new->slider.application_max_value)
	{
		n = 0;
		new->slider.application_current_value = 
					new->slider.application_max_value;
		dval = new->slider.application_max_value;
	        DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	        XtSetValues((Widget)new->slider.current_number, wargs, n);
	}

        else if (new->slider.application_current_value < 
		 new->slider.application_min_value)
	{
		n = 0;
		new->slider.application_current_value = 
					new->slider.application_min_value;
		dval = new->slider.application_min_value;
	        DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	        XtSetValues((Widget)new->slider.current_number, wargs, n);
	}
        else
	{
	DoubleSetArg(wargs[0],XmNdValue,new->slider.application_current_value);
	XtSetValues((Widget)new->slider.current_number, wargs, 1); 	
	}
    }


    if(new->slider.application_max_value != 
       current->slider.application_max_value)
    {
	dval = new->slider.application_max_value;

        /* update the max for all the widget numbers */
	DoubleSetArg(wargs[0], XmNdMaximum, dval );
	XtSetValues((Widget)new->slider.current_number, wargs, 1);
	
        if (new->slider.application_current_value > 
	    new->slider.application_max_value)
	{
		n = 0;
		new->slider.application_current_value = 
					new->slider.application_max_value;
		dval = new->slider.application_max_value;
	        DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	        XtSetValues((Widget)new->slider.current_number, wargs, n);
	}
	center_number = True;
    }

    if(new->slider.application_min_value != 
       current->slider.application_min_value)
    {
	dval = new->slider.application_min_value;

	DoubleSetArg(wargs[0], XmNdMinimum, dval );
	XtSetValues((Widget)new->slider.current_number, wargs, 1);
	
        if (new->slider.application_current_value < 
	    new->slider.application_min_value)
	{
		n = 0;
		new->slider.application_current_value = 
					new->slider.application_min_value;
		dval = new->slider.application_min_value;
	        DoubleSetArg(wargs[n], XmNdValue, dval); n++;
	        XtSetValues((Widget)new->slider.current_number, wargs, n);
	}
	center_number = True;
    }

    if ( new->slider.number_type != current->slider.number_type )
    {
	XtSetArg(wargs[0], XmNdataType, new->slider.number_type);
	XtSetValues((Widget)new->slider.current_number, wargs, 1);
	center_number = True;
    }

    if ( new->slider.decimal_places != current->slider.decimal_places )
    {
	XtSetArg(wargs[0], XmNdecimalPlaces, new->slider.decimal_places);
        XtSetValues((Widget)new->slider.current_number, wargs, 1 );
	center_number = True;
    }

    /* Update the scale widget if min, max, current or inc vals have changed */
    if( (new->slider.application_current_value != 
         current->slider.application_current_value) ||
	(new->slider.application_max_value !=
	 current->slider.application_max_value) ||
	(new->slider.application_min_value !=
	 current->slider.application_min_value) ||
	(new->slider.decimal_places !=
	 current->slider.decimal_places) ||
	(new->slider.increment !=
	 current->slider.increment) )
    {
	if (new->slider.increment==0)
	  new->slider.max_value = new->slider.min_value; 
	else
        { 
            int places;
	    places = new->slider.decimal_places;
	    places = (places<2?2:places);
	    places = (places>7?7:places);
	    new->slider.max_value = pow((double)10, (double)places); 
       }

	if (new->slider.application_max_value==new->slider.application_min_value)
	   new->slider.aslope = 1.0 / zero;
	else
	   new->slider.aslope = 1.0 / (new->slider.application_max_value - 
				new->slider.application_min_value);
	if (new->slider.max_value==new->slider.min_value)
	   new->slider.wslope = 1.0 / zero;
	else
	   new->slider.wslope = 1.0 / (double)(new->slider.max_value - 
				new->slider.min_value);

	new->slider.current_value = 
		appl2widgetnumber(new, new->slider.application_current_value );
	XtSetArg(wargs[0], XmNmaximum, new->slider.max_value);
	XtSetArg(wargs[1], XmNvalue, new->slider.current_value);
	XtSetValues((Widget)new->slider.bar, wargs, 2);
    }

    if(center_number)
	CenterNumber(new);

    XFlush(XtDisplay(new)); 
    return TRUE;
}

static void Destroy( XmSliderWidget sw )
{
    XtRemoveAllCallbacks((Widget)sw, XmNvalueCallback);
}

static void CenterNumber(XmSliderWidget sw )
{
}


/*  Subroutine:	CreateSliderNumber
 *  Purpose:	Handle creation of number (digital readout) widget
 */
static XmNumberWidget CreateSliderNumber(  int x, int min, double value,
					  int max, char* title,
					  Widget *label, XmSliderWidget slider,
					  Boolean above, Boolean editable,
					  Boolean make_stepper,
					  Boolean label_is_widget )
{
    Arg wargs[30];
    double dmin, dmax, dval;
    Widget number;
    XmString text;
    int n;

    /*
     *  Create the panel-meter like number display
     */
    /*  The stepper's values are absolute  */
    dmin = slider->slider.application_min_value;
    dmax = slider->slider.application_max_value;
    dval = value;

    n = 0;

    /*  Estimate a comfortably large size to initially create NumberWidget  */
    XtSetArg(wargs[n], XmNheight, 19); n++;
    DoubleSetArg(wargs[n], XmNdMinimum, dmin); n++;
    DoubleSetArg(wargs[n], XmNdMaximum, dmax); n++;
    DoubleSetArg(wargs[n], XmNdValue, dval); n++;
    XtSetArg(wargs[n], XmNdecimalPlaces, slider->slider.decimal_places ); n++;

    /*  Center text in the field and resize for tighter fit of font  */
    XtSetArg(wargs[n], XmNcenter, TRUE); n++;
    XtSetArg(wargs[n], XmNrecomputeSize, True); n++;

    if( editable )
    {
	XtSetArg(wargs[n], XmNeditable, TRUE); n++;
    }
    else
    {
	XtSetArg(wargs[n], XmNeditable, FALSE); n++;
    }

    XtSetArg(wargs[n], XmNdataType, DOUBLE); n++;
    XtSetArg(wargs[n], XmNcharPlaces, 15); n++;
    XtSetArg(wargs[n], XmNshadowThickness, slider->manager.shadow_thickness); n++;
    XtSetArg(wargs[n], XmNfixedNotation, False); n++;
    number = XmCreateNumber((Widget)slider, title, wargs, n);

    /*  Callback for when a new value is entered directly  */
    XtAddCallback(number, XmNactivateCallback, 
		(XtCallbackProc)CallbackFromNumber, slider);
    XtManageChild(number);

    XtSetArg(wargs[0], XmNleftAttachment, XmATTACH_POSITION);
    XtSetArg(wargs[1], XmNrightAttachment, XmATTACH_NONE);
    XtSetArg(wargs[2], XmNleftPosition, x);
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

    /*  Create the string for the label, and then the label  */
    text = XmStringCreate(title, XmSTRING_DEFAULT_CHARSET);
    XtSetArg(wargs[6], XmNlabelString, text);
    *label = XmCreateLabel((Widget)slider, "label", wargs, 7);
    XmStringFree(text);

    slider->slider.current_number = (XmNumberWidget)number;
    CenterNumber (slider);
    return (XmNumberWidget)number;
}


static void CallbackFromScale ( XmScaleWidget scale,
	   			XmSliderWidget   slider,
				XmScaleCallbackStruct  *call_data )
{

Arg wargs[10];
double dval;
XmSliderCallbackStruct callback_data;



	if ( (call_data->reason != XmCR_DRAG) &&
	     (call_data->reason != XmCR_VALUE_CHANGED) )    return;

	slider->slider.current_value = call_data->value;
	dval = widgetnumber2appl(slider, call_data->value);
	slider->slider.application_current_value = dval;
	slider->slider.current_value = 
		appl2widgetnumber(slider, slider->slider.application_current_value );
	
	DoubleSetArg( wargs[0], XmNdValue, dval );
	XtSetValues((Widget)slider->slider.current_number, wargs, 1);

	callback_data.reason = call_data->reason;
	callback_data.value = dval;
	XtCallCallbacks((Widget)slider, XmNvalueCallback, &callback_data);
}



static double
gridify (double dval, double inc)
{
Boolean posval;
double ddv, diff, newval;
int dv;

   if (dval >= 0.0) posval = True; else posval = False;
   ddv = dval / inc;
   dv = (int)ddv;
   diff = ddv - dv;
   if (diff >= 0.5 ) dv++;
   newval = inc * dv;
   if ((posval) && (newval < 0.0)) newval = -newval; 
   return newval;
}

static void CallbackFromArrow ( XmArrowButtonWidget   arrow,
				XmSliderWidget        slider,
				XmAnyCallbackStruct   *call_data )
{

XmSliderCallbackStruct callback_data;
Arg wargs[10];
double dval, old_value;



	if ( call_data->reason != XmCR_ACTIVATE )    return;

	old_value = slider->slider.application_current_value;

	if( arrow == slider->slider.left_arrow )
	{
            dval = slider->slider.application_current_value - 
			slider->slider.increment;
		 
	    dval = _dxf_round(dval, slider->slider.decimal_places);
            if (slider->slider.increment != 0.0) 
               dval = gridify (dval, slider->slider.increment);
	    
	    DoubleSetArg ( wargs[0], XmNcurrent, dval );
	    XtSetValues ((Widget) slider, wargs, 1 );

	    callback_data.reason = call_data->reason;
	    callback_data.value = slider->slider.application_current_value;

	    if (callback_data.value != old_value)
	    	XtCallCallbacks((Widget)slider, XmNvalueCallback, &callback_data);
	    
	}
	 


	if( arrow == slider->slider.right_arrow )
	{
	    dval = slider->slider.application_current_value +  slider->slider.increment; 
	    
	    dval = _dxf_round(dval, slider->slider.decimal_places);
            if (slider->slider.increment != 0.0) 
               dval = gridify (dval, slider->slider.increment);

	    DoubleSetArg ( wargs[0], XmNcurrent, dval );
	    XtSetValues ((Widget) slider, wargs, 1 );

	    callback_data.reason = call_data->reason;
	    callback_data.value = slider->slider.application_current_value;

	    if (callback_data.value != old_value)
	    	XtCallCallbacks((Widget)slider, XmNvalueCallback, &callback_data);
	}
	 

}
/*  Subroutine:	CallbackFromNumber
 *  Purpose:	Callback routine from number widget to allow/disallow stepper
 *		to manipulate the value, or to set a new value.
 */
static void CallbackFromNumber( XmNumberWidget		nw,
			        XmSliderWidget		slider,
			        XmDoubleCallbackStruct* call_data )
{
    XmSliderCallbackStruct callback_data;
    Arg wargs[2];

    if( call_data->reason == XmCR_ACTIVATE )
    {
	if( nw == slider->slider.current_number )
	{
	    slider->slider.current_value = call_data->value;
	
	    DoubleSetArg(wargs[0], XmNcurrent, call_data->value );
	    XtSetValues((Widget)slider, wargs, 1 );

	    callback_data.reason = call_data->reason;
	    callback_data.value = call_data->value;
	    XtCallCallbacks((Widget)slider, XmNvalueCallback, &callback_data);
	} 

    }
}





/*   Subroutine: XmCreateSlider
 *   Purpose: This function creates and returns a Slider widget.
*/

Widget XmCreateSlider( Widget parent, char *name, ArgList args,
                           Cardinal num_args)
{
	return (XtCreateWidget(name, xmSliderWidgetClass, parent, args, 
				num_args));

}
static double _dxf_round(double a, int decimal_places)
{
    double value;
    double expon;
    double remember;

    remember = a;
    /*
     * Round the value 
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
/*
 * Add a warning callback to the number widget within the slider
 */
void
XmSliderAddWarningCallback(Widget w, XtCallbackProc proc, XtPointer clientData)
{
    XmSliderWidget sw =  (XmSliderWidget)w;
    XtAddCallback((Widget)sw->slider.current_number, 
			XmNwarningCallback, proc, clientData);
}
