/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "../base/defines.h"

#include <math.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include "ColorMapEditorP.h"
#include "Color.h"
#include "clipnotify.h"
#include "gamma.h"
#include "findcolor.h"

#define RR 5
#define GG 9
#define BB 5
#define MIX(r,g,b)(((r)*GG+(g))*BB+(b))
#define MAXCMAPSIZE 2048
struct sort 
    {
    int i;              /* index in R*G*B color map */
    int j;              /* nearest index in current X color map */
    int diff;           /* difference */
    };

#ifndef OS2
static int
#else
static int _Optlink
#endif
compare(const void *c, const void *d)
{
    struct sort *a = (struct sort *)c;
    struct sort *b = (struct sort *)d;

    if (a->diff<b->diff)
        return 1;
    else
        return -1;
}

#define HI(x) (((x)>>8)&255)

static void ConvertHSVtoRGB( double hue, double saturation, double value,
			     double scale, XColor* color );

/*  Subroutine:	CreateColorMap
 *  Purpose:	Allocate and install color map with variable color cells
 */
ControlColor* CreateColorMap( Widget w, int num_cells, Boolean reserve_new_map )
{
ControlColor* 	color;
int 		i, j;
int 		r, g, b;
XColor 		cell_def;
int             major_code;
int             event_code;
int             error_code;
unsigned char 	uncomp[256];
unsigned char 	comp[256];
struct 
    {
    unsigned char r, g, b;
    } 		map[RR*GG*BB];
struct sort 	sort[RR*GG*BB];
XColor		colors[MAXCMAPSIZE];
int		cmapsize;
int		n, nn;
Colormap 	cmap;
Display		*display;
unsigned long pixels[MAXCMAPSIZE];
int		ii;
struct
    {
    int	red;
    int green;
    int blue;
    } 		index[256];

    
    /*
     * Set up index array so we can go from indicies to (r g b)
     */
    for (r=0; r<RR; r++) 
    {
        for (g=0; g<GG; g++) 
	{
            for (b=0; b<BB; b++) 
	    {
                index[MIX(r,g,b)].red = r;
                index[MIX(r,g,b)].green = g;
                index[MIX(r,g,b)].blue = b;
	    }
	}
    }

    color = (ControlColor *)XtMalloc(sizeof(struct ControlColorRec));
    color->display = XtDisplay(w);
    display = XtDisplay(w);
    color->screen_number = XScreenNumberOfScreen(XtScreen(w));
    color->depth = DisplayPlanes(display, color->screen_number);
    
    if( (color->map_size = 1<<color->depth) > 256 )
	color->map_size = 256;
    color->num_cells = num_cells;
    color->start = color->map_size - num_cells;  /* Use "end" of the colormap */
    color->default_colormap = DefaultColormap(display, color->screen_number);
    i = 0;
    color->colormap = color->default_colormap;
    cmap = color->colormap;

    cmapsize = XDefaultVisual(display, color->screen_number)->map_entries;
    cmapsize = cmapsize > MAXCMAPSIZE? MAXCMAPSIZE: cmapsize;

    if (!XQueryExtension (display,
                         CLIPNOTIFY_PROTOCOL_NAME,
                         &major_code,
                         &event_code,
                         &error_code))
	{
	/*depth_is_24 = False;*/
	}
    else
	{
	/*depth_is_24 = True;*/
	}

    /* gamma compensation; corresponds to xpic, so we can coexist w/ xpic */
    for (i=0; i<256; i++) 
	{
	comp[i] = sqrt(i/256.0)*256;
	uncomp[i] = i*i/256;
	}

    /* the set of colors we need */
    for (r=0; r<RR; r++) 
    {
        for (g=0; g<GG; g++) 
	{
            for (b=0; b<BB; b++) 
	    {
                i = MIX(r,g,b);
                map[i].r = comp[r*255/(RR-1)];
                map[i].g = comp[g*255/(GG-1)];
                map[i].b = comp[b*255/(BB-1)];
                sort[i].i = i;
                sort[i].j = -1;
                sort[i].diff = 1000000;
            }
        }
    }

    /* set non-allocated color cells to black so we can distinguish them */
    for (n=cmapsize, nn=0; n>0; n/=2) 
	{
	while (XAllocColorCells(display, cmap, False, NULL, 0, pixels+nn, n)) 
	    {
	    for (j=0; j<n; j++) {
		colors[j].pixel = (pixels+nn)[j];
		colors[j].red = 0;
		colors[j].green = 0;
		colors[j].blue = 0;
		colors[j].flags = DoRed | DoGreen | DoBlue;
	    }
	    XStoreColors(display, cmap, colors, n);
	    nn += n;
	}
    }
    XFreeColors(display, cmap, pixels, nn, 0);

    /* query the colors */
    for (j=0; j<cmapsize; j++) {
	colors[j].pixel = j;
	colors[j].flags = DoRed | DoGreen | DoBlue;
    }
    XQueryColors(display, cmap, colors, cmapsize);

    /* find out how colors match the R*G*B colors needed */
    /* for each entry in color map */

    for (j=0; j<cmapsize; j++) 
	{
	/* if it appears to be allocated r/o */
	if (colors[j].red || colors[j].green || colors[j].blue) 
	    {
	    int rr, gg, bb;

	    /* closest match among ones we want (0<=r<RR, 0<=g<GG, 0<=b<BB) */
	    r = ((float)uncomp[HI(colors[j].red)]/255.0) * (RR-1) + 0.5;
	    g = ((float)uncomp[HI(colors[j].green)]/255.0) * (GG-1) + 0.5;
	    b = ((float)uncomp[HI(colors[j].blue)]/255.0) * (BB-1) + 0.5;

	    /* for 27 of colors neighboring the closest match */
	    for (rr=r-1; rr<=r+1; rr++) 
		{
		for (gg=g-1; gg<=g+1; gg++) 
		    {
		    for (bb=b-1; bb<=b+1; bb++) 
			{
			if (rr>=0&&rr<RR && gg>=0&&gg<GG && bb>=0&&bb<BB)
			    {
			    /* compute how well it matches one we want */
			    int i = MIX(rr,gg,bb);
			    int dr = map[i].r-HI(colors[j].red);
			    int dg = map[i].g-HI(colors[j].green);
			    int db = map[i].b-HI(colors[j].blue);
			    int diff = dr*dr + dg*dg + db*db;

			    cell_def = colors[j];

			    /* if it improves match */
			    if (diff < sort[i].diff
			         && XAllocColor(display, cmap, &cell_def)) 
				{
				/* and actually is r/o */
				if (cell_def.pixel==j) 
				    {
				    sort[i].diff = diff;
				    sort[i].j = j;
				    } 
				else 
				    {
					/* else it is r/w, i.e. a false alarm */
					unsigned long p = cell_def.pixel;
					XFreeColors(display, cmap, &p, 1, 0);
				    }
				}
			    }
			}
		    }
		}
	    }
	}

    /* sort them */
    qsort((char *)sort, RR*GG*BB, sizeof(sort[0]), compare);

    /* allocate colors, worst match first */
    for (i=0; i<RR*GG*BB; i++) 
	{
	ii = sort[i].i;
	cell_def.red = map[ii].r<<8;
	cell_def.green = map[ii].g<<8;
	cell_def.blue = map[ii].b<<8;
	j = sort[i].j;

	/* have already seen an exact match? */
	if (sort[i].diff==0) 
	    {
	    cell_def = colors[j];
	    } 
	    /* allocate an exact match? */
	else if (XAllocColor(display, cmap, &cell_def)) 
	    {
	    } 
	/* have already seen an approximate match? */
	else if (j>=0) 
	    {
	    cell_def = colors[j];
	    } 
	/* can't do it */
	else 
	    {
	    /* XXX - should make a last-ditch scan here */
	    }
	r = index[ii].red;
	g = index[ii].green;
	b = index[ii].blue;
	color->rgb[r][g][b].red = cell_def.red;
	color->rgb[r][g][b].green = cell_def.green;
	color->rgb[r][g][b].blue = cell_def.blue;
	color->rgb[r][g][b].pixel = cell_def.pixel;
	}
    return color;
}


/*  Subroutine:	SetControlColor
 *  Purpose:	Convert HSV to RGB and place the results in undithered_cells 
 *  Note:	The RGB values are scaled from 16 bits to 8 bits.  The
 *		dithering routine requires 8 bits for each component, and
 *		the 24 bit frame buffer needs the 8 bits for each component
 */
void SetControlColor( ControlColor* color,
		      double hue[], double saturation[], double value[] )
{
int i;
XColor cell_def = {0, 0, 0};
unsigned short red, green, blue;

    for( i=0; i<color->num_cells; i++ )
	{
	ConvertHSVtoRGB(hue[i], saturation[i], value[i], 65535.0,
			&cell_def);
	red = (cell_def.red)/256;
	green = (cell_def.green)/256;
	blue = (cell_def.blue)/256;
	color->undithered_cells[i].red = red;
	color->undithered_cells[i].green = green;
	color->undithered_cells[i].blue = blue;
	}
}

#define RGB_P(v,s) ((v)*(1.0-(s)))
#define RGB_Q(v,s,f) ((v)*(1.0-((s)*(f))))
#define RGB_T(v,s,f) ((v)*(1.0-((s)*(1.0-(f)))))

/*  Subroutine:	ConvertHSVtoRGB
 *  Purpose:	Compute rgb from hsv
 *  Note:	h varies from 0.0 to 1.0 (unlike the convention of 0-360)
 */
static void ConvertHSVtoRGB( double hue, double saturation, double value,
			     double scale, XColor* cell )
{
    double hue_fraction;
    double scaled_value;
    int i;

    scaled_value = value * scale;
    if( saturation == 0.0 )
    {
	/*  Achromatic color (force no hue)  */
	cell->red = (int)(scaled_value + 0.5);
	cell->green = (int)(scaled_value + 0.5);
	cell->blue = (int)(scaled_value + 0.5);
    }
    else
    {
	if( (hue >= 1.0) || (hue <= 0.0) )
	{
	    i = 0;
	    hue_fraction = 0.0;
	}
	else
	{
	    double h = hue * 6.0;
	    i = (int)h;
	    hue_fraction = h - (double)i;
	}
	switch( i )
	{
	  case 0:
	    cell->red = (int)(scaled_value + 0.5);
	    cell->green = (int)
	      (RGB_T(scaled_value,saturation,hue_fraction) + 0.5);
	    cell->blue = (int)(RGB_P(scaled_value,saturation) + 0.5);
	    break;
	  case 1:
	    cell->red = (int)
	      (RGB_Q(scaled_value,saturation,hue_fraction) + 0.5);
	    cell->green = (int)(scaled_value + 0.5);
	    cell->blue = (int)(RGB_P(scaled_value,saturation) + 0.5);
	    break;
	  case 2:
	    cell->red = (int)(RGB_P(scaled_value,saturation) + 0.5);
	    cell->green = (int)(scaled_value + 0.5);
	    cell->blue = (int)
	      (RGB_T(scaled_value,saturation,hue_fraction) + 0.5);
	    break;
	case 3:
	    cell->red = (int)(RGB_P(scaled_value,saturation) + 0.5);
	    cell->green = (int)
	      (RGB_Q(scaled_value,saturation,hue_fraction) + 0.5);
	    cell->blue = (int)(scaled_value + 0.5);
	    break;
	  case 4:
	    cell->red = (int)
	      (RGB_T(scaled_value,saturation,hue_fraction) + 0.5);
	    cell->green = (int)(RGB_P(scaled_value,saturation) + 0.5);
	    cell->blue = (int)(scaled_value + 0.5);
	    break;
	  case 5:
	    cell->red = (int)(scaled_value + 0.5);
	    cell->green = (int)(RGB_P(scaled_value,saturation) + 0.5);
	    cell->blue = (int)
	      (RGB_Q(scaled_value,saturation,hue_fraction) + 0.5);
	    break;
	}
    }
}
