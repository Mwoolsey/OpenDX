/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#include <stdio.h>
#include <X11/X.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <math.h>

/***************************************************************************/
/* find_color								   */
/* 									   */
/* Read out the default colormap and find the closest cell to the target   */
/***************************************************************************/
static void _find_color(Widget w, XColor *target, char *dontuse)
{
Colormap		cm;
XColor			cell_def[256];
double 			cur_dist;
int 			cur_red_dist_index=0;
int 			cur_green_dist_index=0;
int 			cur_blue_dist_index=0;
double 			cur_red_dist;
double 			cur_green_dist;
double 			cur_blue_dist;
double 			dist;
double 			red_dist;
double 			green_dist;
double 			blue_dist;
int    			i;
XColor 			tmp = {0,0,0,0,0,0};
int    			screen;
int    			ncells;
Display 		*d = XtDisplay(w);
XWindowAttributes 	win_att;
int			depth;
Pixel			pix;
unsigned long		red_mask=0;
unsigned long		green_mask=0;
unsigned long		blue_mask=0;
unsigned long		red_mult=0;
unsigned long		green_mult=0;
unsigned long		blue_mult=0;

    screen = XScreenNumberOfScreen(XtScreen(w));
    XtVaGetValues(w, XmNdepth, &depth, NULL);
    if(depth == 8) 
	    ncells = 255;
    else if(depth == 12) 
	    ncells = 16; 
    else
	    ncells = 256;
    if(XtIsRealized(w))
    {
	    XGetWindowAttributes(d, XtWindow(w), &win_att);
	    if(win_att.colormap != 
	        DefaultColormap(XtDisplay(w), screen))
	    {
	        cm = win_att.colormap;
	        red_mask = win_att.visual->red_mask;
	        green_mask = win_att.visual->green_mask;
	        blue_mask = win_att.visual->blue_mask;

	        red_mult = red_mask & (~red_mask+1);
	        green_mult = green_mask & (~green_mask+1);
	        blue_mult = blue_mask & (~blue_mask+1);
	    }
	    else
	    {
	        cm = DefaultColormap(XtDisplay(w), screen);
	    }
    }
    else
    {
	    cm = DefaultColormap(XtDisplay(w), screen);
    }

    for (i = 0; i < ncells; i++)
	{
	    switch(depth)
	        {
	        case 8:
		    cell_def[i].pixel = i;
		    break;
	        case 12:
		    pix = i;
		    cell_def[i].pixel = 
		        (pix * red_mult) + 
		        (pix * green_mult) + 
		        (pix * blue_mult);
		    break;
	        default:
		    pix = i;
		    cell_def[i].pixel = 
		        (pix * red_mult) | 
		        (pix * green_mult) | 
		        (pix * blue_mult);
		    break;
	        }
	    cell_def[i].flags = DoRed | DoGreen | DoBlue;
	}

    XQueryColors(d, cm, cell_def, ncells);

    cur_dist = sqrt((double)((double)65535*(double)65535*(double)65535));
    cur_red_dist = cur_green_dist = cur_blue_dist = 65535.0*65535.0;
    for (i = 0; i < ncells; i++)
	{
	red_dist = (double)(target->red - cell_def[i].red) * 
	      	   (double)(target->red - cell_def[i].red);
	green_dist = (double)(target->green - cell_def[i].green) * 
	      	     (double)(target->green - cell_def[i].green);
	blue_dist = (double)(target->blue - cell_def[i].blue) * 
	      	    (double)(target->blue - cell_def[i].blue);
		    
	switch(depth)
	    {
	    Boolean ok2use;
	    case 8:
		dist = sqrt(red_dist + green_dist + blue_dist);
		if (!dontuse) ok2use = True;
		else ok2use = (dontuse[cell_def[i].pixel] == 0);
		if ((dist < cur_dist) && (ok2use))
		    {
		    cur_dist = dist;
		    tmp.red = cell_def[i].red;
		    tmp.green = cell_def[i].green;
		    tmp.blue = cell_def[i].blue;
		    tmp.pixel = cell_def[i].pixel;
		    }
		break;
	    default:
		if(red_dist < cur_red_dist)
		    {
		    cur_red_dist = red_dist;
		    cur_red_dist_index = i;
		    }
		if(green_dist < cur_green_dist)
		    {
		    cur_green_dist = green_dist;
		    cur_green_dist_index = i;
		    }
		if(blue_dist < cur_blue_dist)
		    {
		    cur_blue_dist = blue_dist;
		    cur_blue_dist_index = i;
		    }
		break;
	    }
	}
    switch(depth)
	{
	case 8:
	    target->red = tmp.red;
        target->green = tmp.green;
        target->blue = tmp.blue;
        target->pixel = tmp.pixel;
        target->flags = tmp.flags;
        target->pad = tmp.pad;
	    break;
	default:
	    pix = (cell_def[cur_red_dist_index].pixel & red_mask) |
		  (cell_def[cur_green_dist_index].pixel & green_mask) |
		  (cell_def[cur_blue_dist_index].pixel & blue_mask);
	    target->pixel = pix;
	    target->red = cell_def[cur_red_dist_index].red; 
	    target->green = cell_def[cur_green_dist_index].green; 
	    target->blue = cell_def[cur_blue_dist_index].blue; 
	    break;
	}
}


void find_color(Widget w, XColor *target) { _find_color(w, target, NULL); }

void find_color_ronly (Widget w, XColor *target, char *dontuse) 
{
    _find_color(w, target, dontuse);
}

