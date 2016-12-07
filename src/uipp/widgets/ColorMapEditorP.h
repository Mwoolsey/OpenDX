/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _XmColorMapEditorP_H
#define _XmColorMapEditorP_H

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/BulletinBP.h>
#include <Xm/FormP.h>
#include <Xm/Form.h>
#include "Stepper.h"
#include "ControlField.h"
#include "ColorMapEditor.h"
#include <Xm/XmP.h>
#include "Color.h"

#define NUM_LEVELS 225

/*  New fields for the ColorMapEditor widget class record  */

typedef struct _XmColorMapEditorClassPart
{
    ControlPoint *cut_points;
    int num_cut_points;
    ControlColor* color;
    XtPointer extension;
} XmColorMapEditorClassPart;


/* Full class record declaration */

typedef struct _XmColorMapEditorClassRec
{
	CoreClassPart       core_class;
	CompositeClassPart  composite_class;
	ConstraintClassPart constraint_class;
	XmManagerClassPart  manager_class;
	XmBulletinBoardClassPart  bulletin_board_class;
	XmFormClassPart     form_class;
	XmColorMapEditorClassPart color_map_editor_class;
} XmColorMapEditorClassRec;

extern XmColorMapEditorClassRec xmColorMapEditorClassRec;

typedef struct _XmColorMapEditorPart
{
    double		value_minimum;
    double		value_maximum;
    double		value_current;
    Boolean		min_editable;
    Boolean		max_editable;
    XtPointer		user_data;
    String		default_colormap;
    struct  groupRec
	{
	ControlField* field[4];
	ControlColor* color;
	ColorBar *colorbar;
	ControlField* rgb;
	Widget open_filesb, save_filesb;	/* file selection widgets */
    	XtCallbackProc activate_callback;
	}g;
    Widget controlpointdialog;
    XmStepperWidget stepper;
    XmNumberWidget min_num, max_num;		/* Min and max data values */
    char *save_filename;
    char *open_filename;
    Boolean add_hue_cp;				/* Flags used in Set Values */
    Boolean add_sat_cp;
    Boolean add_val_cp;
    Boolean add_op_cp;
    Boolean constrain_vert;
    Boolean constrain_hor;
    Boolean trigger_callback;
    Boolean display_opacity;
    int color_bar_background_style;
    Widget label[30];
    Cursor field_cursor_id;
    int draw_mode;
    int *bins;
    int *log_bins;
    int num_bins;
    Widget grid_w; 
    Pixmap grid_pixmap;
    XFontStruct *font;
    GC		fontgc;
    Boolean     first_grid_expose;
    int         print_points;
} XmColorMapEditorPart;

typedef struct _XmColorMapEditorRec
{
	CorePart       core;
	CompositePart  composite;
	ConstraintPart constraint;
	XmManagerPart  manager;
	XmBulletinBoardPart  bulletin_board;
	XmFormPart     form;
	XmColorMapEditorPart	color_map_editor;
} XmColorMapEditorRec;

#endif
