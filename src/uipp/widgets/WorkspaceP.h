/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _WorkspaceP_h
#define _WorkspaceP_h
#define LTOL 2	/* number of line thicknesses between lines */
#define WTOL 1	/* number of line thicknesses between lines and a widget */

#define POLICY(ww) ww->workspace.placement_policy

#if (XmVersion >= 1002)
#include <Xm/PrimitiveP.h>
#include <Xm/ManagerP.h>
#include <Xm/BulletinBP.h>
#include <Xm/FormP.h>
#endif

#include "WorkspaceCallback.h"

/*
 *  Structure for building lines in the Manhattan algorithm
 */
typedef struct _LineElement
	{
	int x;
	int y;
	struct _LineElement *next;
	}LineElement;

typedef struct _MyWidgetList {
    short xmin, xmax, ymin, ymax;
    Widget w;
    struct _MyWidgetList *next;
} MyWidgetList;

typedef struct _CollideList {
    int min;
    int max;
    Widget child;
	 XmWorkspaceLine line;
	 int type;					/* Line or Widget */
    struct _CollideList *next;
} CollideList;

/*
 *  Structure for line management (one struct per line segment)
 */
typedef struct _XmWorkspaceLine {
    struct _XmWorkspaceLine *next;	/*  For main link list		      */
    struct _XmWorkspaceLine *src_next;  /*  For source widget constraints     */
    struct _XmWorkspaceLine *dst_next;	/*  For destination constraints list  */
    struct _XmWorkspaceLine *temp_next;	/*  For main link list		      */
    Widget	source;			/*  Widgets to which line attaches    */
    Widget	destination;
    Pixel	color;			/*  Color of line		      */
    short	src_x;			/*  Line drawing coords		      */
    short	src_y;
    short	dst_x;
    short	dst_y;
    short	src_x_offset;
    short	dst_x_offset;
    short	x_left;			/*  Rectangular bounding limits	      */
    short	x_right;
    short	y_upper;
    short	y_lower;
    XPoint	point[50];		/*  Array of x,y for drawing 50 lines */
    short	num_points;		/*  Number of points used	      */
    Boolean	is_to_be_moved;		/*  Flag between deciding and doing   */
    Boolean	is_to_be_drawn;		/*  Flag to identify yet to be drawn  */
    Boolean	is_to_be_collapsed;	/*  Flag to identify to be collapsed  */
    Boolean 	reverse_on_copy;     /*  Used by CopyPointsToLine          */
    Boolean 	two_phase_draw;	     /*  Used by RefreshLines          */
} XmWorkspaceLineRec;

/*
 *  Contents of constraint field hung off each child
 */
typedef struct _XmWorkspaceConstraintPart {
    WsCallbackList  resizing_callbacks;
    WsCallbackList  select_callbacks;
    WsCallbackList  accent_callbacks;
    XmWorkspaceLineRec *source_lines;
    XmWorkspaceLineRec *destination_lines;
    Pixel	accent_uncolor;
    short	x_left;
    short	x_right;
    short	y_upper;
    short	y_lower;
    short	x_center;
    short	y_center;
    short	x_delta;		/*  Provisional parameters for move   	*/
    short	y_delta;
    short	x_index;		/*  x coordinate for sort order       	*/
    Boolean	is_selectable;		/*  Can make child insensitive	      	*/
    Boolean	is_selected;		/*  Last callback had selection True  	*/
    Boolean	is_accented;		/*  Last highlight action was on      	*/
    Boolean	will_be_selected;	/*  Would be selected if action ended 	*/
    Boolean	is_within_band;		/*  Is within current rubber_band     	*/
    Boolean	is_managed;		/*  Reference to detect change	      	*/
    Boolean	is_newly_managed;
    char	id;			/*  Used as debug val for easier ID   	*/
    Boolean	is_v_resizable;		/*  Is this child resizable verti.    	*/
    Boolean	is_h_resizable;		/*  Is this child resizable horiz.    		*/
    XRectangle  orig_base;		/*  size before changing modes		*/
    Boolean	pin_to_sides_lr;/* true -> widget always attached to left/right sides */
    Boolean	pin_to_sides_tb;/* true -> widget always attached to top/bottom sides */
    XmFormAttachmentRec att[4]; /* store form attachments when !autoArrange	*/
    Boolean     line_invisibility;
} XmWorkspaceConstraintPart;

typedef struct _XmWorkspaceConstraintRec {
    XmManagerConstraintPart	manager;
    XmFormConstraintPart	form;
    XmWorkspaceConstraintPart	workspace;
} XmWorkspaceConstraintRec, * XmWorkspaceConstraints;

#define WORKSPACE_CONSTRAINT(w) \
 ((XmWorkspaceConstraints)((w)->core.constraints))


typedef struct {
    GC		lineGC;			/* GC for drawing managed lines	      */
    GC		lineGC2;		/* GC for drawing managed lines	      */
    Cursor	hour_glass;		/* Cursor to indicate wait state      */
    Cursor	move_cursor;
    Cursor	size_cursor[4];		/* 4 corners of rubber banding	      */
    XtPointer	extension;
} XmWorkspaceClassPart;

typedef struct _XmWorkspaceClassRec {
    CoreClassPart	    core_class;
    CompositeClassPart	    composite_class;	/* Needed to have children */
    ConstraintClassPart     constraint_class;	/* Needed by manager	   */
    XmManagerClassPart      manager_class;	/* Needed to use gadgets   */
    XmBulletinBoardClassPart bulletin_board_class;
    XmFormClassPart	    form_class;
    XmWorkspaceClassPart    workspace_class;
} XmWorkspaceClassRec;

extern XmWorkspaceClassRec XmworkspaceClassRec;


typedef struct _XmWorkspacePart {
    XtCallbackList position_change_callback;
    XtCallbackList error_callback;
    XtCallbackList background_callback;
    XtCallbackList action_callback;
    XtCallbackList selection_change_callback;
    XtCallbackList resize_callback; 	/* anything you want - see transltn tbl */
    XtEventHandler button_tracker;	/* Client's event handler to call     */
    Widget	track_widget;		/* Client's widget for event handler  */
    short	track_x;		/* Offsets for handled button event   */
    short	track_y;
    void *	track_client_data;	/* Pointer to pass with call	      */
    XmWorkspaceLineRec *lines;		/* Link list of managed lines	      */
    int		num_lines;		/* Number of managed lines	      */
    int		double_click_interval;	/* Max millisecs between button press */
    Time	time;			/* Unsigned long of last event time   */
    GC		gridGC;			/* GC for drawing grid lines (dots)   */
    GC		bandGC;			/* GC for drawing rubber band (Xor)   */
    GC		badBandGC;		/* GC for drawing rubber band (Xor)   */
    int		num_selected;		/* # of children currently selected   */
    Pixel	accent_color;
    XRectangle	rubberband[2];		/* Static space for rubberband	      */
    XRectangle* surrogates;		/* Array of rectangles when moving    */
    int		num_surrogates;
    short	base_x;			/* Reference point for rubberbanding  */
    short	base_y;			/*   and moving with mouse	      */
    short	grab_x;			/* Reference point of pointer move    */
    short	grab_y;
    short	min_x;			/* Limits of displaced children used  */
    short	max_x;			/*   to check need to resize	      */
    short	min_y;
    short	max_y;
    short	expose_left;		/* Limits of displaced lines used     */
    short	expose_right;		/*   to restrict effect of clearing   */
    short	expose_upper;
    short	expose_lower;
    short	grid_width;		/* Grid spacing */
    short	grid_height;
    short	line_thickness;
    unsigned char accent_policy;	/* Mode to accent selected children   */
    unsigned char selection_policy;
    unsigned char inclusion_policy;	/* When rubberband captures child     */
    unsigned char surrogate_type;	/* What to draw when moving children  */
    unsigned char placement_policy;	/* Which rule set applies to overlaps */
    unsigned char horizontal_alignment;	/* Mode of grid alignment of child    */
    unsigned char horizontal_draw_grid;	/* XmDRAW_ NONE, HASH, LINE, OUTLINE  */
    unsigned char vertical_alignment;
    unsigned char vertical_draw_grid;	/* XmDRAW_ NONE, HASH, LINE, OUTLINE  */
    unsigned char sort_policy;		/* Part of child to determine x order */
    Boolean	snap_to_grid;		/* Force children placement as moved  */
    Boolean	manhattan_route;	/* Draw lines in orthogonal channels  */
    Boolean	overlap_is_allowed;	/* Allow children to overlap 1another */
    Boolean	movement_is_allowed;	/* Allow user to move children	      */
    Boolean	is_rubberbanding;	/* ButtonMotion means resize band     */
    Boolean	is_moving;		/* ButtonMotion means move selected   */
    Boolean	is_resizing;		/* ButtonMotion means resize selected */
    Boolean	band_is_visible;	/* Rubberand last draw as visible     */
    Boolean	surrogate_is_visible;	/* Surrogates last drawn as visible   */
    Boolean	shift_is_applied;	/* Action started with shift key down */
    Boolean	move_cursor_installed;	/* Fleur cursor is installed	      */
    unsigned char size_xx_installed;	/* Rubberbanding cursor is installed  */
    int         collide_width;		/* allocated entries in collide_list_x 	*/
    int         collide_height;		/* allocated entries in collide_list_x 	*/
    CollideList **collide_list_x;	/* Bins of occupied x positions       	*/
    CollideList **collide_list_y;	/* Bins of occupied y positions       	*/
    MyWidgetList  *widget_list;		/* A list of all children for collision
					   detection 				*/
    Boolean	button1_press_mode;	/* Either Rubberbanding (0) or calling
					   background callback (1)	      	*/
    int		wtol;			/* Min dist between line and a widget 	*/
    int		ltol;			/* Min dist between two lines 		*/
    Boolean	line_drawing_enabled;	/* Draw lines upon their creaation?	*/
    Boolean	force_route;		/* Force a manhattan route if it is
					   un-routable				*/
    int		halo_thickness;
    int		collision_spacing;	/* Inter-widget spacing in 
					   MoveAToRightofB		     	*/
    Boolean	suppress_callbacks;
    Boolean	is_selectable;		/* Selectability for the whole 
					   workspace			     	*/
    Boolean	auto_arrange;		/* True if I should repack children if
					   one of them resizes himself.      	*/
    Dimension   orig_width;		/* "redunant" versions widget size	*/
    Dimension   orig_height;		/* used by Resize method to determine	*/
					/* wether we're growing or shrinking	*/
    XPoint	start;
    XPoint	corner;
    Boolean	overlaps;		/* were overlapping children detected	*/
    Boolean	check_overlap;		/* should we check for collisions	*/
} XmWorkspacePart;


typedef struct _XmWorkspaceRec {
    CorePart		core;
    CompositePart	composite;
    ConstraintPart	constraint;
    XmManagerPart	manager;
    XmBulletinBoardPart bulletin_board;
    XmFormPart		form;
    XmWorkspacePart	workspace;
} XmWorkspaceRec;

extern void FreeLineElementList(LineElement *list);

#if (OLD_LESSTIF == 1)
typedef struct _XmFormConstraintPart *XmFormConstraint;
typedef struct _XmFormConstraintRec *XmFormConstraintPtr;
#endif

#endif
