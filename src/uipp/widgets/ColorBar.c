/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "../base/defines.h"

#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <Xm/Frame.h>
#include "ColorMapEditorP.h"
#include "Color.h"
#include "Image.h"

#define BLACK 0
#define WHITE 1
#define CELL_SIZE 2

/* External Functions */

void Dither(XColor *, int, int, XColor *, ControlColor *); /* from Dither.c */

/* Internal Functions */

static void BarExposeCallback( XmImageWidget w, ColorBar* bar,
			       ImageCallbackStruct* cb );
void DrawColorBar( ColorBar* bar, XColor* undithered_cells );
static void FillIndexLine ( register XColor* data_line, 
			    register XColor *undithered_cells, int length,
			    int width, unsigned long first, 
			    unsigned long last );
static void FillColorBar( XImage *xi, int x, int y, 
			  int subimage_width, register int width, 
			  int colunm_width, int height,
			  XColor *dithered_cells, Boolean vertical, int depth,
			  unsigned long red_mask, unsigned long green_mask,
			  unsigned long blue_mask);

void OpacityCorrectCells( ControlColor* color, double opacity[],int background,
			  int cb_height, int starting_color);

ColorBar* CreateColorBar( XmColorMapEditorWidget parent,
			  int margin, int min_val, 
			  int max_val, ControlColor *color)
{
ColorBar* bar;
Arg wargs[8];
#if (XmVersion < 1001)
short shadow_thickness;
#else
Dimension shadow_thickness;
#endif

    bar = (ColorBar *)XtCalloc(1, sizeof(struct ColorBarRec));

    /* Get the shadow thickness form the parent */
    XtSetArg(wargs[0], XmNshadowThickness, &shadow_thickness);
    XtGetValues((Widget)parent, wargs, 1);

    XtSetArg(wargs[0], XmNshadowThickness, shadow_thickness);
    XtSetArg(wargs[1], XmNshadowType, XmSHADOW_IN);
#ifndef sgi
    bar->frame = XmCreateFrame((Widget)parent, "frame", wargs, 2);
    bar->parent = parent;

    XtSetArg(wargs[0], XmNshadowThickness, 0);
    bar->w = (XmImageWidget)XmCreateImage(bar->frame, "colorbar", wargs, 1);
#else
    /*
     * Causes "Bailout after 10000 edge synchronization attempts on sgi
     */
    bar->parent = parent;

    XtSetArg(wargs[0], XmNshadowThickness, 0);
    bar->w = (XmImageWidget)XmCreateImage((Widget)bar->parent, "colorbar", wargs, 1);
#endif

    XtAddCallback((Widget)bar->w, XmNexposeCallback, (XtCallbackProc)BarExposeCallback, bar);
    XtAddCallback((Widget)bar->w, XmNresizeCallback, (XtCallbackProc)BarExposeCallback, bar);
    bar->vertical = TRUE;
    bar->descend = TRUE;
    bar->min = min_val;
    bar->max = max_val;
    bar->margin = margin;
    bar->color = color;
    bar->background_style = STRIPES;
    
    return bar;
}


/*  Subroutine:	BarExposeCallback
 *  Purpose:	Redraw things needing to be redrawn when window newly exposed
 */
/* ARGSUSED */
static void BarExposeCallback( XmImageWidget w, ColorBar* bar,
			       ImageCallbackStruct* cb )
{
    bar->width = w->core.width;
    bar->height = w->core.height;
    bar->image_width = bar->width;
    bar->image_height = bar->height;
    if( XtIsRealized((Widget)(bar->w)) )
        {
	XClearArea(XtDisplay(w), XtWindow(w), 0, 0, 
			w->core.width, w->core.height, False);
	DrawColorBar(bar, bar->color->undithered_cells);
        }
}

/*  Subroutine:	DrawColorBar
 *  Purpose:	Draw the color bar image using the mapping of the cells given
 *  Note:	If cells is NULL, a one-one mapping is used.
 *  Note:	The bar->index must already be set up.
 */
void DrawColorBar( ColorBar* bar, XColor* undithered_cells )
{
XGCValues	values;
Display *d;
unsigned long r_mask=0, g_mask=0, b_mask=0;
XWindowAttributes attributes;

    d = XtDisplay(bar->w);
    if(bar->depth != 8)
	{
	XGetWindowAttributes(XtDisplay(bar->w), XtWindow(bar->w), &attributes);
	r_mask = attributes.visual->red_mask;
	g_mask = attributes.visual->green_mask;
	b_mask = attributes.visual->blue_mask;
	}
	
    if( bar->ximage == NULL )
        {
	bar->ximage = XCreateImage(d, 
			   DefaultVisual(d, XScreenNumberOfScreen(XtScreen(bar->w))), 
			   bar->depth, ZPixmap, 0, NULL, 0, 0, 8, 0);
	values.function = GXcopy;
	bar->gc = XtGetGC((Widget)bar->w, GCFunction, &values);
        }
    /* Free Previously allocated chunks of memory */
    if( bar->index_line )
        XtFree((char*)bar->index_line);
    if( bar->dithered_cells )
        XtFree((char*)bar->dithered_cells);

    /*  Free old and Allocate a new image buffer  */
    if( bar->ximage->data )
        XtFree(bar->ximage->data);
    if (bar->depth == 8)
	{
	bar->ximage->data =
      		XtMalloc((bar->image_width) * 
		bar->image_height * sizeof(char));
	}
    else if (bar->depth == 15 || bar->depth == 16)
    {
	bar->ximage->data =
     		XtMalloc(2 * (bar->image_width) * 
		bar->image_height * sizeof(char));
    }
    else
    {
	bar->ximage->data =
      		XtMalloc(4 * (bar->image_width) * 
		bar->image_height * sizeof(char));
    }

    if (bar->depth == 8)
    {
	bar->ximage->bytes_per_line = bar->image_width;
    }
    else if (bar->depth == 15 || bar->depth ==16)
    {
        bar->ximage->bytes_per_line = 2*bar->image_width;
    }
    else
    {
        bar->ximage->bytes_per_line = 4*bar->image_width;
    }
    bar->ximage->width = bar->image_width;
    bar->ximage->height = bar->image_height;

    /*  Determine length of color sweep for color bar, set up template */
    if( bar->vertical )
        {
        bar->index_line = (XColor *)XtMalloc(bar->image_height * 
    				CELL_SIZE * sizeof(XColor));
        bar->dithered_cells = (XColor *)XtMalloc(bar->image_height * 
    				CELL_SIZE * sizeof(XColor));
	OpacityCorrectCells( bar->color, 
		bar->parent->color_map_editor.g.field[3]->line[0]->values, 
		bar->background_style,bar->image_width/2, 1 );
        FillIndexLine(bar->index_line, bar->color->opacity_corrected_cells, 
		bar->image_height, CELL_SIZE, bar->max, bar->min);
        }
    else
        {
        bar->index_line = (XColor *)XtMalloc(bar->image_width * 
				CELL_SIZE * sizeof(XColor));
        bar->dithered_cells = (XColor *)XtMalloc(bar->image_width * 
				CELL_SIZE * sizeof(XColor));
	OpacityCorrectCells( bar->color, 
		bar->parent->color_map_editor.g.field[3]->line[0]->values, 
		bar->background_style, bar->image_width/2, 1);
        FillIndexLine(bar->index_line, bar->color->opacity_corrected_cells, 
		bar->image_width, CELL_SIZE, bar->max, bar->min);
        }

    if(bar->depth == 8)
        {
	Dither(bar->index_line, CELL_SIZE, bar->image_height, 
		bar->dithered_cells, bar->color);
	FillColorBar(bar->ximage, 0, 0, bar->image_width/2, 
		 bar->image_width, CELL_SIZE,
		 bar->image_height, bar->dithered_cells, bar->vertical, 
		 bar->depth, 
		 r_mask, g_mask, b_mask);
	}
    else
	{
	FillColorBar(bar->ximage, 0, 0, bar->image_width/2,
		 bar->image_width, CELL_SIZE,
		 bar->image_height, bar->index_line, bar->vertical, 
		 bar->depth,
		 r_mask, g_mask, b_mask);
	}

    /*
     * Now deal with the right hand side of the color bar
     */
    OpacityCorrectCells( bar->color, 
		bar->parent->color_map_editor.g.field[3]->line[0]->values, 
		bar->background_style, bar->image_width/2, 0 );
    FillIndexLine(bar->index_line, bar->color->opacity_corrected_cells, 
		bar->image_height, CELL_SIZE, bar->max, bar->min);
    if(bar->depth == 8)
        {
	Dither(bar->index_line, CELL_SIZE, bar->image_height, 
		bar->dithered_cells, bar->color);
	FillColorBar(bar->ximage, bar->image_width/2, 0, 
		 bar->image_width - bar->image_width/2,
		 bar->image_width, CELL_SIZE,
		 bar->image_height, bar->dithered_cells, bar->vertical, 
		 bar->depth,
		 r_mask, g_mask, b_mask);
	}
    else
	{
	FillColorBar(bar->ximage, bar->image_width/2, 0, 
		 bar->image_width - bar->image_width/2, bar->image_width, 
		 CELL_SIZE,
		 bar->image_height, bar->index_line, bar->vertical, 
		 bar->depth,
		 r_mask, g_mask, b_mask);
	}
    XPutImage(d, XtWindow(bar->w), bar->gc, bar->ximage, 0, 0,
	      0, 0, bar->image_width, bar->image_height);
    XFlush(d);
}


/*  Subroutine:	FillIndexLine
 *  Purpose:	Create a column (2 pixels wide) of undither colors.
 *		This column is then dithered.
 */
static void FillIndexLine( register XColor* data_line, 
			    register XColor *undithered_cells, int length,
			    int width, unsigned long first, unsigned long last )
{
double range;
double end_value, value_inc;
register double value;
int i;

    value = (double)first;
    end_value = (double)last;
    range = end_value - value;
    value_inc = range / (double)length;
    if( value_inc < 0 )
        {
	do
	    {
	    if( value > end_value )
		{
		for(i = 0; i < width; i++)
		    {
		    *(data_line++) = 
				undithered_cells[(unsigned long)(value + 0.5)];
		    }
		}
	    else
		{
		for(i = 0; i < width; i++)
		    {
		    *(data_line++) = undithered_cells[last];
		    }
		}
	    value += value_inc;
	    }
	while(length-- > 1);
        }
    else
        {
	do
	    {
	    if( value < end_value )
		{
		for(i = 0; i < width; i++)
		    {
		    *(data_line++) = 
				undithered_cells[(unsigned long)(value + 0.5)];
		    }
		}
	    else
		{
		for(i = 0; i < width; i++)
		    {
		    *(data_line++) = undithered_cells[(unsigned long)(last)];
		    }
		}
	    value += value_inc;
	    }
	while( length-- > 1);
        }
}


/*  Subroutine:	FillColorBar
 *  Purpose:	Fill data into a colorbar image by replicating the dithered
 * 		column in dithered_cells.  The dimensions of dithered_cells
 *		is height by CELL_SIZE(width).
 */
static void FillColorBar( XImage* ximage, int x, int y, 
			  int subimage_width, register int width, 
			  int column_width, int height,
			  XColor *cells, Boolean vertical, int depth,
			  unsigned long red_mask, 
			  unsigned long green_mask, 
			  unsigned long blue_mask)
{
Pixel pix24;
int level, i, index;
unsigned long rmult=0, gmult=0, bmult=0;

    if (depth != 8)
    {
	rmult = red_mask & (~red_mask+1);
	gmult = green_mask & (~green_mask+1);
	bmult = blue_mask & (~blue_mask+1);
    }
    index = 0;
    if( vertical )
        {
	for(level = 0; level < height; level++)
	    {
	    for(i = 0; i < subimage_width; i++)
		{
		if(depth == 8)
                {
		    XPutPixel(ximage, x+i, level, 
			cells[index + i%column_width].pixel);
                }
		else if (depth == 15)
                {
                    unsigned char r = cells[index + i%column_width].red * 32 / 256;
                    unsigned char g = cells[index + i%column_width].green * 32 / 256;
                    unsigned char b = cells[index + i%column_width].blue * 32 / 256;
                    pix24 = r * rmult + g * gmult + b * bmult;
                    XPutPixel(ximage, x+i, level, pix24);
                }
                else if (depth == 16)
                {
                    unsigned char r = cells[index + i%column_width].red * 32 / 256;
                    unsigned char g = cells[index + i%column_width].green * 64 / 256;
                    unsigned char b = cells[index + i%column_width].blue * 32 / 256;
                    pix24 = r * rmult + g * gmult + b * bmult;
                    XPutPixel(ximage, x+i, level, pix24);
                }
		else
                {
		    pix24 = (unsigned char)cells[index + i%column_width].red *
				rmult +
			    (unsigned char)cells[index + i%column_width].green *
				gmult +
			    (unsigned char)cells[index + i%column_width].blue *
				bmult;
		    XPutPixel(ximage, x+i, level, pix24);
                }
            }
	    index += column_width;
	  }
	}
}
void OpacityCorrectCells( ControlColor* color, double opacity[], 
			  int background_style, int cb_height, 
			  int starting_color)
{
int i;
int cb_count;
int cb_white;

    if (background_style == STRIPES)
	{
        if (starting_color == BLACK)
	    {
	    for( i=0; i<color->num_cells; i++ )
                {
                color->opacity_corrected_cells[i].red = 
		    (int)(opacity[i] * (double)color->undithered_cells[i].red);
                color->opacity_corrected_cells[i].green = 
		   (int)(opacity[i] * (double)color->undithered_cells[i].green);
                color->opacity_corrected_cells[i].blue = 
		    (int)(opacity[i] * (double)color->undithered_cells[i].blue);
		}
            }
        if (starting_color == WHITE)
	    {
	    for( i=0; i<color->num_cells; i++ )
                {
                color->opacity_corrected_cells[i].red = 
		    color->undithered_cells[i].red +
		    (int) ((double)(255 - (int)color->undithered_cells[i].red) * 
		    (1.0 - opacity[i]));
                color->opacity_corrected_cells[i].green = 
		    color->undithered_cells[i].green +
		    (int) ((double)(255 - (int)color->undithered_cells[i].green) * 
		    (1.0 - opacity[i]));
                color->opacity_corrected_cells[i].blue = 
		    color->undithered_cells[i].blue +
		    (int) ((double)(255 - (int)color->undithered_cells[i].blue) * 
		    (1.0 - opacity[i]));
		}
            }
	}
    if (background_style == CHECKERBOARD)
	{
	cb_count = 0;
	if (starting_color == 0)
	    {
	    cb_white = True;
	    }
	else
	    {
	    cb_white = False;
	    }
	for( i=0; i<color->num_cells; i++ )
            {
	    if (cb_white)
		{
            	color->opacity_corrected_cells[i].red = 
		    color->undithered_cells[i].red +
		    (int) ((double)(255 - (int)color->undithered_cells[i].red) * 
		    (1.0 - opacity[i]));
            	color->opacity_corrected_cells[i].green = 
		    color->undithered_cells[i].green +
		    (int) ((double)(255 - (int)color->undithered_cells[i].green) * 
		    (1.0 - opacity[i]));
            	color->opacity_corrected_cells[i].blue = 
		    color->undithered_cells[i].blue +
		    (int) ((double)(255 - (int)color->undithered_cells[i].blue) * 
		    (1.0 - opacity[i]));
		}
	    else
		{
            	color->opacity_corrected_cells[i].red = 
		    (int)(opacity[i] * (double)color->undithered_cells[i].red);
            	color->opacity_corrected_cells[i].green = 
		   (int)(opacity[i] * (double)color->undithered_cells[i].green);
            	color->opacity_corrected_cells[i].blue = 
		    (int)(opacity[i] * (double)color->undithered_cells[i].blue);
		}
	    cb_count++;
	    if(cb_count > cb_height)
		{
		cb_count = 0;
		cb_white = !cb_white;
		}
	    }
	}
}

