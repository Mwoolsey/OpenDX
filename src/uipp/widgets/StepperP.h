/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _XmStepperP_h
#define _XmStepperP_h

#if (XmVersion >= 1002)
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#endif

#include "NumberP.h"	/*  Define MultitypeData  */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef struct _XmStepperClassPart {
  Cardinal	 minimum_time_interval;
  Cardinal	 initial_time_interval;
  String	 font;
  XtPointer	extension;
} XmStepperClassPart;

typedef struct _XmStepperClassRec {
    CoreClassPart	core_class;
    CompositeClassPart	composite_class;	/* Needed to have children */
    ConstraintClassPart constraint_class;	/* Needed by manager	   */
    XmManagerClassPart  manager_class;		/* Needed to use gadgets   */
    XmStepperClassPart	stepper_class;
} XmStepperClassRec;

extern XmStepperClassRec XmstepperClassRec;

typedef struct _XmStepperPart {
  /* Variables settable by SetValue calls */
  MultitypeData	 value;			/* Current value		*/
  MultitypeData	 value_minimum;		/* Minimum value		*/
  MultitypeData	 value_maximum;		/* Maximum value		*/
  MultitypeData	 value_step;		/* Value change per step	*/
  Boolean        value_changed;         /* Value changed since last callback */
  Cardinal	 num_digits;		/* Size of Presentation		*/
  int		 decimal_places;	/* Digits after point		*/
  Cardinal	 time_interval;		/* Basic stepping rate		*/
  Cardinal	 time_ddelta;		/* %rate increase/repeat	*/
  XtCallbackList activate_callback;	/* Callback list (completed)	*/
  XtCallbackList step_callback;		/* Callback list (increment)	*/
  XtCallbackList warning_callback;	/* Callback list (warning)	*/
  unsigned char	 increase_direction;	/* Orientation of arrows	*/
  Boolean	 center_text;		/* Center Number Widget text	*/
  Boolean	 editable;		/* Allow typed entry of value   */
  Boolean	 roll_over;		/* Range folds on self		*/
  Boolean	 resize_to_number;	/* OK to resize if number grows */
  unsigned char  alignment;
  /* Variables for internal use only */
  int		 repeat_count;
  unsigned long  interval;
  XtCallbackProc timeout_proc;
  XtIntervalId	 timer;
  Widget	 active_widget;		/* Which arrow owns down button */
  Boolean	 allow_input;		/* Don't step when editor busy  */
  int		 inc_x;			/* To set rate by location	*/
  int		 dec_x;
  int		 arrow_width;
  Widget	 child[3];
  short          data_type;
  Pixel		 insens_foreground;
  Boolean	 is_fixed;
} XmStepperPart;

typedef struct _XmStepperRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    XmManagerPart	manager;
    XmStepperPart	stepper;
} XmStepperRec;

#if defined(__cplusplus) || defined(c_plusplus)
 }
#endif

#endif
