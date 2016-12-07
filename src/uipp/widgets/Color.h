/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef _Color_h
#define _Color_h

#include "Image.h"
#include "ImageP.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif


typedef struct ControlColorRec
{
    XmImageWidget	w;
    int		screen_number;
    Display*	display;
    Visual*	visual;
    Colormap	default_colormap;
    Colormap	colormap;
    short	depth;
    short	map_size;
    short	num_cells;
    short	start;
    XColor	undithered_cells[256]; /* RGB - valid; 0 <= rgb <= 255 */
    XColor	opacity_corrected_cells[256]; /* RGB - valid; 0 <= rgb <= 255 */
    XColor	rgb[5][9][5];	       /* Pixel - valid */
} ControlColor;


typedef struct ColorBarRec
{
    Widget		frame;
    XmImageWidget	w;
    XmColorMapEditorWidget parent;
    ControlColor	*color;
    XImage*		ximage;
    XColor*		index_line;  /* Undithered column height x CELL_SIZE */ 
    XColor*		dithered_cells;/* Dithered column height x CELL_SIZE */
    GC			gc;
    short		width;
    short		height;
    short		image_width;
    short		image_height;
    short		margin;
    short		min;
    short		max;
    Boolean		vertical;
    Boolean		descend;
    int			depth;
    int			background_style;
} ColorBar;


ColorBar* CreateColorBar( XmColorMapEditorWidget parent,
			  int margin, int min_val, 
			  int max_val, ControlColor *color);
ControlColor* CreateColorMap( Widget w, int num_cells,
			      Boolean reserve_new_map );
void SetControlColor( ControlColor* color,
		      double hue[], double saturation[], double value[] );

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
