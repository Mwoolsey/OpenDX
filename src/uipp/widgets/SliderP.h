/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*	Construct and manage the "frame sequencer guide" to be used as a
 *	popup of the "sequence controller" (alias VCR control)
 *
 *	August 1990
 *	IBM T.J. Watson Research Center
 *	R. T. Maganti
 */

/*
 */

#ifndef XmSliderP_H_
#define XmSliderP_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <Number.h>
#include <Stepper.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/ArrowB.h>
#include <Xm/Scale.h>
#include <Xm/BulletinB.h>
#include <Xm/BulletinBP.h>
#include <Xm/Form.h>
#include <Xm/FormP.h>


/*  New fields for the DrawingArea widget class record  */

typedef struct
{
   XtPointer mumble;   /* No new procedures */
} XmSliderClassPart;


/* Full class record declaration */

typedef struct _XmSliderClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	XmBulletinBoardClassPart bulletin_board_class;
	XmFormClassPart 	form_class;
	XmSliderClassPart	slider_class;
} XmSliderClassRec;

extern XmSliderClassRec xmSliderClassRec;

typedef struct _XmSliderPart
{
    XmNumberWidget current_number;
    XmLabelGadget start_label;
    XmLabelWidget current_label;
    XmLabelGadget stop_label;
    XmLabelWidget inc_label;

    XmScaleWidget bar;
    XmArrowButtonWidget    left_arrow;
    XmArrowButtonWidget    right_arrow;

    unsigned long current_color;
    unsigned long limit_color;

    int max_value;/* Scale widget's maxmin */ 
    int min_value;	
    double application_max_value;
    double application_min_value;

    int current_value; /* XmnValue of Scale */
    double increment;	


    double application_current_value;

    double  wslope; /* widget */
    double  aslope; /* application */ 


    int     number_type;
    int     decimal_places;


    void *	user_data;
    XtCallbackList value_callback;      /* Callback list for any change    */
} XmSliderPart;

typedef struct _XmSliderRec
{
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	XmBulletinBoardPart	bulletin_board;
	XmFormPart		form;
	XmSliderPart		slider;
} XmSliderRec;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
