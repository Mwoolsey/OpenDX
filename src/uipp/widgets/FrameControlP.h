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
 * $Log: FrameControlP.h,v $
 * Revision 1.7  2000/05/16 18:53:33  gda
 * Changes to compile using MS compilers under Cygwin
 *
 * Revision 1.6  1999/11/25 16:05:42  scottr
 *
 * Some of this may explain the recently reported problems with Motif 2.1. If
 * BulletinBoard has an extension record in 2.1, then the class initialization
 * of a lot of these things would have messing messing up the class structures.
 *
 * It is probably a good idea to do a make clean in the top then make
 * everything from scratch. This is changing some of the structures hidden
 * very deep within the beast.....
 *
 *
 *
 * ColorMapEditor.c: The initialization of the ClassRecord had the wrong number
 * 	of fields for the BB class.
 *
 * ColorMapEditorP.h: Add an extension pointer to the class record
 *
 * DialP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * ImageP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * FrameControlP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * FrameControl.c: The initialization of the ClassRecord had the wrong number
 * 	of fields for the BB class.
 *
 * 	- no pixel should be specified as None, not NULL.
 *
 * NumericList.c: Timer id's are not pointers
 *
 * MultiText.c: No pixmap should be specified as None, not NULL.
 *
 * MultiTextP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * NumberP.h: the ClassPart had the extension field as an caddr_t instead
 * 	of XtPointer. (which was probably okay, but in the sake of
 * 	consistency.....)
 *
 * NumericListP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * Picture.c: value_mask, in a call of XtGetGC is not a pointer
 * 	- No pixmap should be specified as None, not NULL.
 * 	- timer id's are not pointers
 * 	- No cursor should be None, not NULL
 * 	- A pixel color is an unsigned short, so full intensity is 0xffff, not
 * 	  0xffffffff
 *
 * PictureP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * Slider.c: The initialization of the ClassRecord had the wrong number
 * 	of fields for the BB class.
 *
 * SliderP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * SlideBar.c: No pixmap should be specified as None, not NULL.
 *
 * SlideBarP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * Stepper.c: timer id's are not pointers
 *
 * StepperP.h: Add an extension pointer to the class record
 *
 * VCRControl.c: The initialization of the ClassRecord had the wrong number
 * 	  of fields for the BB class.
 * 	- timer id's are not pointers
 *
 * VCRControlP.h: the ClassPart had the extension field as an int instead
 * 	of XtPointer.
 *
 * WorkspaceW.c: No cursor should be None, not NULL
 *
 * WorkspaceP.h: Add an extension pointer to the class record
 *
 * Revision 1.5  1999/05/10 15:46:31  gda
 * Copyright message
 *
 * Revision 1.5  1999/05/10 15:46:31  gda
 * Copyright message
 *
 * Revision 1.4  1999/05/03 14:09:01  gda
 * dxconfig.h, plus other post-new-shapshot fixes
 *
 * Revision 1.3  1999/04/30 13:10:06  gda
 * After re-snapshotting, ripping out headers and adding linux changes
 *
 * Revision 9.1  1997/05/22 18:47:25  svs
 * Copy of release 3.1.4 code
 *
 * Revision 8.0  1995/10/03  18:49:26  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.1  1994/01/18  19:02:23  svs
 * changes since release 2.0.1
 *
 * Revision 6.1  93/11/16  10:50:40  svs
 * ship level code, release 2.0
 * 
 * Revision 1.4  93/09/24  09:27:15  jvergo
 * Clamp min to data driven max and visa versa.
 * 
 * Revision 1.3  93/08/10  14:39:58  jvergo
 * Added XmNincSensitive resource.
 * 
 * Revision 1.2  93/07/16  15:05:18  jvergo
 * Added min/max sensitive control.
 * 
 * Revision 1.1  93/02/26  15:46:50  danz
 * Initial revision
 * 
 * 
 */

#ifndef XmFrameControlP_H_
#define XmFrameControlP_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <Number.h>
#include <Stepper.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <SlideBar.h>
#include <Xm/BulletinB.h>
#include <Xm/BulletinBP.h>
#include <Xm/Form.h>
#include <Xm/FormP.h>


/*  New fields for the DrawingArea widget class record  */

typedef struct
{
   XtPointer mumble;   /* No new procedures */
} XmFrameControlClassPart;


/* Full class record declaration */

typedef struct _XmFrameControlClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	XmBulletinBoardClassPart bulletin_board_class;
	XmFormClassPart 	form_class;
	XmFrameControlClassPart	frame_control_class;
} XmFrameControlClassRec;

extern XmFrameControlClassRec xmFrameControlClassRec;

typedef struct _XmFrameControlPart
{
    XmNumberWidget start_number;
    XmNumberWidget next_number;
    XmNumberWidget current_number;
    XmNumberWidget stop_number;
    XmNumberWidget min_number;
    XmNumberWidget max_number;
    XmStepperWidget inc_stepper;
    XmLabelGadget start_label;
    XmLabelWidget next_label;
    XmLabelWidget current_label;
    XmLabelGadget stop_label;
    XmLabelGadget min_label;
    XmLabelGadget max_label;
    XmLabelWidget inc_label;
    XmSlideBarWidget bar;
    unsigned long next_color;
    unsigned long current_color;
    unsigned long limit_color;
    short max_value;			/* total number of frames covered  */
    short min_value;			/* 0 to actual offset (usually 1)  */
    short start_value;			/* first frame of current sequence */
    short next_value;			/* last frame shown or queued (!!) */
    short current_value;		/* last frame shown or queued (!!) */
    short stop_value;			/* last frame of current sequence  */
    short increment;			/* number of frames per advance    */
    void *user_data;
    XtCallbackList value_callback;      /* Callback list for any change    */
    Boolean current_visible;		/* Display the current info ?      */
    Boolean min_sensitive;		/* Min editiable?		   */
    Boolean max_sensitive;		/* Max editiable?		   */
    Boolean inc_sensitive;		/* Inc editiable?		   */
    Widget  warning_dialog;
} XmFrameControlPart;

typedef struct _XmFrameControlRec
{
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	XmBulletinBoardPart	bulletin_board;
	XmFormPart		form;
	XmFrameControlPart	frame_control;
} XmFrameControlRec;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
