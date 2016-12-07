/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




#ifndef _Picture_h
#define _Picture_h

#include <Xm/DrawingA.h>
#include <Xm/DrawingAP.h>
#include <Xm/Label.h>
#include <Xm/LabelP.h>
#include <Xm/PushB.h>
#include <Xm/PushBP.h>
#include "Image.h"
#include "ImageP.h"
#include "Picture.h"

/*  New fields for the Picture widget class record  */

typedef struct
{
   XtPointer none;   /* No new procedures */
} XmPictureClassPart;


/* Full class record declaration */

typedef struct _XmPictureClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	XmDrawingAreaClassPart	drawing_area_class;
	XmImageClassPart	image_class;
	XmPictureClassPart	picture_class;
} XmPictureClassRec;

extern XmPictureClassRec xmPictureClassRec;


#define Z_LEVELS 16		/* Number of levels for depth cueing */
#define UNDO_STACK_DEPTH 10


struct point 
{
	double    xcoor;
	double    ycoor;
	double    zcoor;


	double    txcoor;
	double    tycoor;
	double    tzcoor;

  	int       sxcoor;
	int 	  sycoor;
	int       szcoor;

	short     visible;
};

struct line 
{
  	int       x1;
	int 	  y1;
	int       x2;
	int       y2;

	Boolean   rejected;
	Boolean   dotted;
	Boolean   greyed;
};


#define   EQNUM     10 
#define   ORBNUM    7 

struct globe
{
	int       x;
	int       y;

	double	  first_x;	/* The x position of the cursor at selection*/
	double	  first_y;	/* The y position of the cursor at selection*/
	double	  first_z;

	double	  last_x;	/* The prev x position of the cursor */
	double	  last_y;	/* The prev y position of the cursor */
	double	  last_l;

	int       px;
	int       py;
	double    pt;

	double	  paz;	/* prev angle about the Z axis */

	double    xcoor;
	double    ycoor;
	double    zcoor;

	double    radi;

	struct point      equator[EQNUM][ORBNUM];
	struct point      orbit[ORBNUM][EQNUM];

	struct point      tequator[EQNUM][ORBNUM];
	struct point      torbit[ORBNUM][EQNUM];

	int               visible[EQNUM][ORBNUM];
	double            Wtrans[4][4];
	Boolean		  on_sphere;
	double		  circumference;
        Pixmap  	  pixmap;
};



#define  SHADES     50
typedef struct _XmPicturePart
{
    /*  Public (resource accessable)  */
    Boolean 		display_globe;
    int 		mode;
    Pixmap 		pixmap,  marker;
    XmBasis		basis;
    int			globe_radius;
    Boolean		show_rotating_bbox;
    Window		overlay_wid;
    Boolean		overlay_exposure;
    int			cursor_size;
    int			cursor_speed;
    int			constrain_cursor;
    Boolean		new_image;
    int			translate_speed_factor;
    int			rotate_speed_factor;
    int			navigate_direction;
    int			look_at_direction;
    double		look_at_angle;

    XtCallbackList 	pick_callback;
    XtCallbackList 	cursor_callback;
    XtCallbackList 	rotation_callback;
    XtCallbackList 	zoom_callback;
    XtCallbackList 	roam_callback;
    XtCallbackList 	navigate_callback;
    XtCallbackList 	client_message_callback;
    XtCallbackList 	property_notify_callback;
    XtCallbackList 	mode_callback;
    XtCallbackList 	undo_callback;
    XtCallbackList 	key_callback;

    /*  Private (local use)  */
    Time last_time;

    XColor		box_grey;
    XColor		selected_in_cursor_color;
    XColor		selected_out_cursor_color;
    XColor		unselected_in_cursor_color;
    XColor		unselected_out_cursor_color;

/* Stuff to make the 3D cursor work */

    Pixel	white;
    Pixel	black;
    double	DW;
    double	DH;
    double	zscale_width;
    double	zscale_height;
    double	view_angle;
    int		projection;


    int		undo_stk_ptr;
    int		redo_stk_ptr;
    int		undo_count;
    int		redo_count;
    struct _undo_stk
    {
	double	up_x;
	double	up_y;
	double	up_z;
	double	from_x;
	double	from_y;
	double	from_z;
	double	to_x;
	double	to_y;
	double	to_z;
	double	width;
	double	view_angle;
	int	projection;
    } undo_stack[UNDO_STACK_DEPTH], redo_stack[UNDO_STACK_DEPTH];

    Boolean	camera_defined;
    Boolean	first_step;
    int		button_pressed;
    int         grab_keyboard_count;
    KeySym	keysym;
    double	arrow_roam_x;
    double	arrow_roam_y;
    double	arrow_roam_z;
    int		old_x;
    int		old_y;
    Boolean	first_key_press;
    int		ignore_new_camera;
    Boolean	double_click;
    Boolean	good_at_select;
    Boolean	disable_temp;
    double	navigate_to_x;	/* Navigate parameters	*/
    double	navigate_to_y;	/* Navigate parameters	*/
    double	navigate_to_z;	/* Navigate parameters	*/
    double	navigate_from_x;/* Navigate parameters	*/
    double	navigate_from_y;/* Navigate parameters	*/
    double	navigate_from_z;/* Navigate parameters	*/
    double	navigate_up_x;	/* Navigate parameters	*/
    double	navigate_up_y;	/* Navigate parameters	*/
    double	navigate_up_z;	/* Navigate parameters	*/
    double	to_x;		/* Autocamera parameters	*/
    double	to_y;		/* Autocamera parameters	*/
    double	to_z;		/* Autocamera parameters	*/
    double	from_x;		/* Autocamera parameters	*/
    double	from_y;		/* Autocamera parameters	*/
    double	from_z;		/* Autocamera parameters	*/
    double	up_x;		/* Autocamera parameters	*/
    double	up_y;		/* Autocamera parameters	*/
    double	up_z;		/* Autocamera parameters	*/
    int		image_width;	/* Autocamera parameters	*/
    int		image_height;	/* Autocamera parameters	*/
    double	autocamera_width;/* Autocamera parameters	*/
    Boolean	autoaxis_enabled;/* Autocamera parameters	*/
    int		n_cursors;
    XmPushButtonWidget	pb;
    Widget	popup;
    Boolean 	popped_up;
    XFontStruct *font;
    GC		fontgc;
    XtIntervalId tid;
    XtIntervalId key_tid;
    int		cursor_num;
    int 	PIXMAPWIDTH, 
		PIXMAPHEIGHT;
    short 	i_pixmap[20][20];
    float  	z_pixmap[20][20] ;
    double 	X, 
		Y, 
		Z;		/* Current (X,Y,Z) of the 3D cursor */
    int 	K;			/* Length of pointer history buffer */
    int 	px, 
		py; 		/* Pointer history "buffer" */
    int		ppx, 
		ppy; 		/* Pointer history "buffer" */
    int		pppx, 
		pppy; 		/* Pointer history "buffer" */
    int 	pcx, 
		pcy;		/* last active cursor location */
    int 	iMark, 
		piMark;
    double   	Wtrans[4][4];
    double	WItrans[4][4]; 
    double   	PureWtrans[4][4];
    double	PureWItrans[4][4]; 
    double 	Xmark[3], 
		Ymark[3], 
		Zmark[3], 
		pXmark[3], 
		pYmark[3], 
		pZmark[3];
    struct point Face1[4], 
		Face2[4];
    double	Zmax;
    struct line box_line[12];
    double	Zmin;
    double	FacetXmax;
    double	FacetYmax;
    double	FacetZmax;
    double	FacetXmin;
    double	FacetYmin;
    double	FacetZmin;
    Boolean 	CursorBlank;
    Cursor	cursor;
    int 	FirstTime;
    int		FirstTimeMotion;
    double 	Ox;
    double	Oy;
    double	Oz;
    double	zangle;
    double	xangle;
    double	yangle;
    int		*xbuff;
    int		*ybuff;
    double 	*zbuff;
    double	*cxbuff; /* Cursor positions in canonical form */
    double	*cybuff;
    double 	*czbuff;
    int		*selected;
    int		roam_xbuff;	/* Roam cursor info */
    int		roam_ybuff;
    double 	roam_zbuff;
    double	roam_cxbuff;
    double	roam_cybuff;
    double 	roam_czbuff;
    int		roam_selected;
    int    	cursor_shape;
    double	angle_posx;
    double	angle_posy;
    double	angle_posz;
    double	angle_negx;
    double	angle_negy;
    double	angle_negz;
    Boolean	x_movement_allowed; /* Used in XmCURSOR_MODE2 */
    Boolean	y_movement_allowed;
    Boolean	z_movement_allowed;

   GC  		gc;
   GC		gcovl;			/* GC for the overlay window */
   GC  		gc_dash;
   GC		gcovl_dash;			/* GC for the overlay window */
   Pixmap  	ActiveSquareCursor[Z_LEVELS];
   Pixmap	PassiveSquareCursor[Z_LEVELS];
   struct globe	*globe;

   int		gnomon_center_x;
   int		gnomon_center_y;
   int		gnomon_xaxis_x;
   int		gnomon_xaxis_y;
   int		gnomon_yaxis_x;
   int		gnomon_yaxis_y;
   int		gnomon_zaxis_x;
   int		gnomon_zaxis_y;
   int		gnomon_xaxis_label_x;
   int		gnomon_xaxis_label_y;
   int		gnomon_yaxis_label_x;
   int		gnomon_yaxis_label_y;
   int		gnomon_zaxis_label_x;
   int		gnomon_zaxis_label_y;

   struct
   {
   int		old_x;
   int		old_y;
   int		old_center_x;
   int		old_center_y;
   int		old_width;
   int		old_height;
   GC		gc;
   int		center_x;
   int		center_y;
   double	aspect;			/* aspect ratio of the window */
   }		rubber_band;
    
} XmPicturePart;


typedef struct _XmPictureRec
{
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	XmDrawingAreaPart	drawing_area;
	XmImagePart		image;
	XmPicturePart		picture;
} XmPictureRec;

#endif
