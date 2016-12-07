/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _ControlField_h
#define _ControlField_h

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/DrawingAP.h>
#include "ColorMapEditor.h"
#ifdef OS2
#include <types.h>
#endif

/*  Size of undo stack (number of undoable operations */
#define UNDO_STACK_SIZE 10

typedef struct CoordListRec
{
    short		low;
    short		mid;
    short		high;
} LineCoord;

typedef struct ControlFieldRec ControlField;

struct ControlFieldRec	/* One for each of the 4 graphs */
{
    XmDrawingAreaWidget	w;
    XmColorMapEditorWidget cmew;
    struct ControlMapRec* map[4];
    struct ControlLineRec* line[4];
    double*		levels;		/* num_levels array of level centers  */
    LineCoord*		coords;		/* num_levels graph level coords      */
    XtPointer		user_data;
    void 		(* user_callback)(ControlField *);
    void		(* expose_callback)(Widget, ControlField *,
					    XmDrawingAreaCallbackStruct *);
    short		num_maps;
    short		num_lines;
    short		num_levels;	/* number of levels		      */
    short		input_mode;	/* which map now controlled by user   */
    short		left_margin;	/* Margins around graph in window     */
    short		right_margin;
    short		top_margin;
    short		bottom_margin;
    short		width;		/* Size of graph area		      */
    short		height;
    Boolean		vertical;	/* Is axis of level parameter	      */
    Boolean		init;		/* looked at in the expose callback */
    GC			rubber_band_gc;		/* GC for drawing boxes	      */
    Widget		toggle_b;
};


typedef struct ControlLineRec	/* For graph lines and RGB lines */
{
    ControlField*	field;
    XPoint*		points;		/* num_levels*2 line drawing elements */
    double*		values;		/* num_levels array of data values    */
    short*		load;		/* num_levels measures of overlap     */
    GC			gc;		/* GC for drawing lines		      */
    GC			erase_gc;	/* GC for drawing lines		      */
    Pixel		color;		/* color in which to draw line	      */
    short		num_points;	/* number of points used for drawing  */
    short		num_levels;	/* number of levels (225 in this case)*/
    short		load_limit;	/* load beyond which is an overload   */
    short		last_level_index; /* often used value: num_levels - 1 */
} ControlLine;


typedef struct ControlPointRec	/* For each Control Point */
{
    double	level;
    double	value;
    int		wrap_around;		/* Wrap arounds above 1 or below 0    */
    int		x, y;
    Boolean	multi_point_grab;	/* Is this point part of a multi point
					   grab? */
    int		old_print_x;
    int		old_print_y;
    int		old_print_width;
    int		old_print_height;
    short	alignment_dummy;
} ControlPoint;


typedef struct ControlMapRec
{
    ControlField*	field;
    ControlLine*	line;
    Time		double_click_interval;  /* time tollerance */
    ControlPoint*	points;         /* Array holding control points - jgv */
    XRectangle*	 	boxes;
    GC			gc;		/* GC for drawing boxes		      */
    GC			erase_gc;	/* GC for drawing lines		      */
    Pixel		color;		/* color in which to draw boxes	      */
    short		num_points;
    short		size_buf;
    short		grabbed;	/* Index of grabbed point	      */
    short		end_policy;	/* How fill between end points & edge */
    Boolean		clip_value;	/* Restrict value to range of 0-1     */
    Boolean		multi_point_grab;
    Boolean		multi_point_move;
    Boolean		rubber_banding;
    Boolean		first_rubber_band;
    int			rubber_band_base_y;
    int			multi_move_base_y;
    int			multi_move_base_x;
    int			rubber_band_current_y;
    int			rubber_band_old_y;
    int			delta_y_min;
    int			delta_y_max;
    Boolean		toggle_on;
    struct
    {
	ControlPoint*   undo_points;
	XRectangle*     undo_boxes;
	short           num_undo_points;
	short           undo_size_buf;
    } undo_stack[UNDO_STACK_SIZE];
    int			num_undoable;
    int			undo_top;

} ControlMap;

/*  Indexes of maps and lines in field's arrays  */
#define HUE 0
#define SATURATION 1
#define VALUE 2
#define OPACITY 3

/*  End file policy  */
#define FILL_ZERO 0
#define FILL_ONE 1
#define FILL_LEVEL 2

/*  Increment by which to expand capacity for control points  */
#define ADDITION 16

/*  Parameters for drawing and pointing at control points  */
#define HASH_OFFSET 2
#define HASH_SIZE 4

/*  Space between graph and edges of its window  */
#define MARGIN HASH_OFFSET+1

/*  Grab range is x**2+y**2.  Good grab_ranges for the following radii are:
 * (1): 2, (2): 5, (3): 10, 13, (4): 17, 18, 20, (5): 29, 32, 34
 * (6): 41, 45, (7) 53, (8) 69
 */
#define GRAB_RANGE 29


/*  Externed routines in ControlField.c  */
ControlField* CreateControlField( Widget parent, char *name, short num_levels,
				  short x, short y, short width, short height,
				  Arg* args, int num_args );

/*  Externed routines in ControlLine.c  */
ControlLine* CreateControlLine( ControlField* field,
			        int load_limit, Pixel color );
void InitializeLevels( ControlField* field, int num_levels );
void SetLineCoords( ControlLine* line );
void DrawLine( ControlLine* line, GC gc );

/*  Externed routines in ControlPoint.c  */
ControlMap* CreateControlMap( ControlField* field, ControlLine* line,
			      Pixel color, Boolean clip_value );
void RedrawControlBoxes( ControlMap* map );
void DrawControlBoxes( ControlMap* map, GC gc );
void ResizeControlBoxes( ControlMap* map );
Boolean MapInput( Widget w, ControlMap* map,
		  XmDrawingAreaCallbackStruct* call_data );

/*  Externed routines in ControlValue.c  */
void SetArrayValues( ControlMap* map, ControlLine* line );

extern void PushUndoPoints(XmColorMapEditorWidget cmew);
#endif
