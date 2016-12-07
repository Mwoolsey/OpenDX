/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	DIALP_H
#define DIALP_H
   
#include "Dial.h"
#if (XmVersion >= 1002)
#include <Xm/PrimitiveP.h>
#endif
#include <Xm/XmP.h>

typedef	struct	_XmDialClassPart
	{
	XtPointer	ignore;
	}	XmDialClassPart;

typedef	struct	_XmDialClassRec
	{
	CoreClassPart		core_class;
	XmPrimitiveClassPart	primitive_class;
	XmDialClassPart		dial_class;
	}	XmDialClassRec;

extern	XmDialClassRec	XmdialClassRec;

#define XmRClockDirection       "ClockDirection"

typedef	int	Sign;
#define	POSITIVE		1
#define	NEGATIVE		-1

#define XmRDouble               "Double"

#define MAXSEGMENTS     200

typedef	struct	_XmDialPart
	{
	XtCallbackList  select;

	double		minimum;		/* XmNminimum 		*/
	double		maximum; 		/* XmNmaximum		*/
	double		increment;		/* XmNincrement		*/
	int		increments_per_marker; 	/* XmNincrementsPerMarker */	
	int		decimal_places;		/* XmNdecimalPlaces	*/
	
	int             num_markers;		/* XmNnumMarkers	*/
	Dimension	major_marker_width;	/* XmNmajorMarkerWidth	*/
	Dimension       minor_marker_width;	/* XmNminorMarkerWidth	*/
	int		major_position;		/* XmNmajorPosition	*/
	int   		starting_marker_pos;	/* XmNstartingMarkerPos	*/
	Pixel		major_marker_color;	/* XmNmajorMarkerColor	*/
	Pixel           minor_marker_color;     /* XmNminorMarkerColor  */
	Dimension       major_marker_thickness; /* XmNmajorMarkerThickness */
	Dimension       minor_marker_thickness; /* XmNminorMarkerThickness */

	double		position;		/* XmNposition		*/
	Dimension	indicator_width;  	/* XmNindicatorWidth 	*/
	Pixel           indicator_color;	/* XmNindicatorColor	*/

	Boolean		shading;		/* XmNshading		*/
	int		shade_percent_shadow;	/* XmNshadePercentShadow */	
	Pixel		shade_increasing_color; /* XmNshadeIncreasingColor */
	Pixel           shade_decreasing_color; /* XmNshadeDecreasingColor */
	ClockDirection	increasing_direction;	/* XmNincreasingDirection */

	double		indicator_angle;	/* angle in radians. 	*/
	double		prev_angle;		/* angle in radians. 	*/
	double		starting_marker_angle;	/* XmNstartingMarkerPos -
							in radians.	*/
	int		major_width;		/* actual maj_marker width */
	int		minor_width;		/* actula min_marker width */
	int		num_major_markers;	/* count of major markers  */
	int		num_minor_markers;	/* count of minor markers  */
	int  		increments_per_rev;	/* #increments in 1 dial -
							    revolution.	*/
	XPoint		indicator[4];	/* Endpoints of indicator polygon */
	Position        center_x;	/* x,y position of -	 	*/
	Position        center_y;	/*    center of dial.		*/
	Position        inner_diam;	/* circle inside markers. 	*/
	Position	minor_diam;	/* circle outside minor markers.*/
	Position        outer_diam;	/* circle outside major markers.*/
	GC		major_marker_GC; /* dial face GC.		*/
	GC		minor_marker_GC; /* dial face GC.               */
	GC		indicator_GC;	/* indicator GC.		*/
	GC		inverse_GC;	/* erases indicator & shading.	*/
	GC		shade_incr_GC; 	/* shade increasing GC.		*/
	GC		shade_decr_GC;	/* shade decreasing GC.		*/
	GC		shade_def_incr_GC; /* Default for increasing.	*/
	GC		shade_def_decr_GC; /* Default for decreasing.	*/
	Boolean		shading_active;	/* is shading currently active? */
	int		shading_angle1;	/* parameters to XFillArk to - 	*/
	int		shading_angle2;	/*    draw shading.		*/
	XPoint		major_segments[MAXSEGMENTS]; /* marker coordinates. */
	XPoint		minor_segments[MAXSEGMENTS]; /* marker coordinates. */
	}	XmDialPart;

typedef	struct	_XmDialRec
	{
	CorePart	core;
	XmPrimitivePart	primitive;
	XmDialPart	dial;
	}	XmDialRec;

#endif
