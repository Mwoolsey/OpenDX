/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef Picture_H
#define Picture_H

#include "XmDX.h"
#include "Image.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


extern WidgetClass xmPictureWidgetClass;

extern Widget XmCreatePicture( Widget parent, String name,
			 ArgList args, Cardinal num_args );

typedef struct _XmPictureClassRec * XmPictureWidgetClass;
typedef struct _XmPictureRec      * XmPictureWidget;

typedef struct {
    int		reason;
    XEvent 	*event;
    int		mode;
    double 	x;
    double 	y;
    double 	z;
    double 	from_x;
    double 	from_y;
    double 	from_z;
    double 	up_x;
    double 	up_y;
    double 	up_z;
    int		cursor_num;
    int 	screen_x;
    int 	screen_y;
    double 	screen_z;
    int 	tag;
    double	zoom_factor;
    double	autocamera_width;
    int		translate_speed;
    int		rotate_speed;
    int		projection;
    double      view_angle;
    KeySym	keysym;
} XmPictureCallbackStruct;

typedef struct
{
	double	Bs[4][4];
	double	Bw[4][4];
} XmBasis;

typedef struct
{
	double x;
	double y;
	double z;
} XmRotationCenter;

/*  Declare types for convenience routine to create the widget  */
extern Widget XmCreatePicture
  (Widget parent, String name, ArgList args, Cardinal num_args);

extern Boolean XmPictureUndoable ( XmPictureWidget picture );
extern Boolean XmPictureRedoable ( XmPictureWidget picture );
extern void XmPicturePushUndoCamera ( XmPictureWidget picture );

extern void XmPictureSetView(XmPictureWidget w, int direction, double *from_x,
				double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z);

extern void XmPictureTurnCamera(XmPictureWidget w, int direction, double angle,
				double *to_x, double *to_y, double *to_z,
				double *from_x, double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z,
				double *autocamera_width);

extern void XmPictureChangeLookAt(XmPictureWidget w, int direction, double angle,
	double *to_x, double *to_y, double *to_z,
	double *dir_x, double *dir_y, double *dir_z, 
	double *up_x, double *up_y, double *up_z, 
	double *autocamera_width);

extern Boolean XmPictureGetUndoCamera(XmPictureWidget w, 
				double *to_x, double *to_y, double *to_z,
				double *from_x, double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z,
				double *autocamera_width, int *projection,
				double *view_angle);

extern Boolean XmPictureGetRedoCamera(XmPictureWidget w, 
				double *to_x, double *to_y, double *to_z,
				double *from_x, double *from_y, double *from_z,
				double *up_x, double *up_y, double *up_z,
				double *autocamera_width, int *projection,
				double *view_angle);

extern Boolean
XmPictureNewCamera(XmPictureWidget w,
		   XmBasis basis, double matrix[4][4],
	       	   double from_x, double from_y, double from_z,
	       	   double to_x, double to_y, double to_z,
		   double up_x, double up_y, double up_z,
		   int image_width, int image_height, double autocamera_width,
		   Boolean autoaxis_enabled, int projection, double view_angle);

extern Boolean
XmPictureInitializeNavigateMode(XmPictureWidget w);

extern void
XmPictureExecutionState(XmPictureWidget w, Boolean begin);

extern void
XmPictureReset(XmPictureWidget w);

extern void
XmPictureResetCursor(XmPictureWidget w);

extern void
XmPictureAlign(XmPictureWidget w);

extern void
XmPictureDeleteCursors(XmPictureWidget w, int cursor_num);

extern void
XmPictureLoadCursors(XmPictureWidget w,
	       	     int n_cursors,
		     double **vector_list);

#define Xaxis       0
#define Yaxis       1
#define Zaxis       2

#define   XmSQUARE    0
#define   XmSPHERE    1 

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
