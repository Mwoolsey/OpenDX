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
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "ColorMapEditorP.h"
#include <Xm/DrawingA.h>
#include <Xm/DrawingAP.h>
#include "Grid.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

/* External Functions */

void PrintAllControlPoints(XmColorMapEditorWidget cmew); /* From ControlPoint.c */

#define NUM_TICKS 11

static void ResizeCallback( Widget w, int color,
			    XmDrawingAreaCallbackStruct* cb );
extern GC InitGC(Widget w, Pixel color );

Widget CreateGrid( Widget parent, Pixel color, Arg* args, int num_args )
{
    Arg wargs[40];
    Widget w;
    long                 mask;
    XWindowAttributes    getatt;
    XSetWindowAttributes setatt;

    (void)memcpy((char *)wargs, (char *)args, num_args * sizeof(Arg));
    XtSetArg(wargs[num_args], XmNshadowThickness, 0); num_args++;
    w = XmCreateDrawingArea(parent, "graph", wargs, num_args);
    XtAddCallback(w, XmNexposeCallback, (XtCallbackProc)ResizeCallback, (XtPointer)color);
    XtAddCallback(w, XmNresizeCallback, (XtCallbackProc)ResizeCallback, (XtPointer)color);
    XtManageChild(w);

    /*
     * Make sure button and motion events get passed through.
     */
    XGetWindowAttributes(XtDisplay(w), XtWindow(w), &getatt);

    mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

    setatt.event_mask            = getatt.your_event_mask & ~mask;
    setatt.do_not_propagate_mask = getatt.do_not_propagate_mask & ~mask;

    XChangeWindowAttributes(XtDisplay(w), XtWindow(w),
                            CWEventMask | CWDontPropagate,
                            &setatt);
    return w;
}

/*  Subroutine:	ResizeCallback
 *  Purpose:	Redraw the grid when window newly exposed
 */
static void ResizeCallback( Widget w, int color,
			    XmDrawingAreaCallbackStruct* cb )
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)XtParent(w);

    if( !XtIsRealized(w) )
        {
        return;
        }
    else
	{
	if(cb->reason == XmCR_EXPOSE)
	{
	    /*
	     * We need the first expose to get things drawn the first time.
	     */
	    PrintAllControlPoints(cmew);
	    if(!cmew->color_map_editor.first_grid_expose)
		return;
	    cmew->color_map_editor.first_grid_expose = False;
	}
	if(cmew->color_map_editor.draw_mode == CME_GRID)
	    DrawGrid(w, color);
	if((cmew->color_map_editor.draw_mode == CME_HISTOGRAM) ||
	   (cmew->color_map_editor.draw_mode == CME_LOGHISTOGRAM))
	    DrawHistogram(w, color);
	}
}
/*  Subroutine:	DrawGrid
 *  Purpose: Do the actual grid drawing
 */

void DrawGrid(Widget w, Pixel color)
{
XmColorMapEditorWidget cmew;
int height, width, length, ypos, long_length, short_length;
GC gc;
int i;
Pixmap pixmap;
Pixel bg;

    if ((w == NULL)  || !XtIsRealized(w))
	return;

    cmew = (XmColorMapEditorWidget)XtParent(w);

    XtVaGetValues(w, XmNbackground, &bg, NULL);
    height = w->core.height;
    width = w->core.width;

    if(cmew->color_map_editor.grid_pixmap) 
	XFreePixmap(XtDisplay(cmew), cmew->color_map_editor.grid_pixmap);

    pixmap = XCreatePixmap(XtDisplay(w),
			   XtWindow(w),
			   width,
			   height, 
			   DefaultDepth(XtDisplay(w), 
					XScreenNumberOfScreen(XtScreen(cmew))));
    cmew->color_map_editor.grid_pixmap = pixmap;
    gc = InitGC( w, color);
    XSetForeground(XtDisplay(w), gc, bg);
    XFillRectangle(XtDisplay(w), pixmap, gc, 0, 0, width, height);
    XSetForeground(XtDisplay(w), gc, color);

    short_length = (int) ( (float)width/5.0);
    long_length = short_length*2.0;

    /* Draw the vertical line */
    XDrawLine(XtDisplay(w), pixmap, gc, 0, 0, 0, height);
    /* Draw the ticks */
    for (i = 0; i<NUM_TICKS; i++)
	{
	ypos =  (int) (float)((height-1)*i)/(NUM_TICKS-1);
	if ( (i == 0) || (i == NUM_TICKS-1) || (i == NUM_TICKS/2) )
	    {
	    length = long_length;
	    }
	else
	    {
	    length = short_length;
	    }
	XDrawLine(XtDisplay(w), pixmap, gc, 1, ypos, length, ypos);
	}
    XtReleaseGC(w, gc);
    XSetWindowBackgroundPixmap(XtDisplay(w), XtWindow(w), pixmap);
    XClearWindow(XtDisplay(w), XtWindow(w));
    PrintAllControlPoints(cmew);
}
void DrawHistogram(Widget w, Pixel color)
{
int height, width, length, ypos1, ypos2;
GC gc;
int i;
XmColorMapEditorWidget cmew;
int *bins=NULL;
int num_bins;
int max_bin;
Pixmap pixmap;
Pixel bg;

    if ((w == NULL)  || !XtIsRealized(w))
	return;

    cmew = (XmColorMapEditorWidget)XtParent(w);

    if(cmew->color_map_editor.draw_mode == CME_HISTOGRAM)
	bins = cmew->color_map_editor.bins;
    else if(cmew->color_map_editor.draw_mode == CME_LOGHISTOGRAM)
	bins = cmew->color_map_editor.log_bins;

    num_bins = cmew->color_map_editor.num_bins;

    XtVaGetValues(w, XmNbackground, &bg, NULL);
    height = w->core.height;
    width = w->core.width;

    if(cmew->color_map_editor.grid_pixmap) 
	XFreePixmap(XtDisplay(cmew), cmew->color_map_editor.grid_pixmap);

    pixmap = XCreatePixmap(XtDisplay(w),
			   XtWindow(w),
			   width,
			   height,
			   DefaultDepth(XtDisplay(w), 
				XScreenNumberOfScreen(XtScreen(w))));
    cmew->color_map_editor.grid_pixmap = pixmap;
    gc = InitGC( w, color);
    XSetForeground(XtDisplay(w), gc, bg);
    XFillRectangle(XtDisplay(w), pixmap, gc, 0, 0, width, height);
    XSetForeground(XtDisplay(w), gc, color);
    if(!bins) 
    {
	DrawGrid(w, color);
	return;
    }
    max_bin = 0;
    for(i = 0; i < num_bins; i++)
    {
	max_bin = MAX(max_bin, bins[i]);
    }

    /* Draw the bins. Note: Invert y for X windows */
    for(i = 0; i < num_bins; i++)
    {
	length = ((float)bins[i]/(float)max_bin)*(float)(width-3);
	ypos1 =  (int) (float)((height-1)*(num_bins - i - 1))/(num_bins);
	XDrawLine(XtDisplay(w), pixmap, gc, 1, ypos1, length, ypos1);
	ypos2 =  (int) (float)((height-1)*(num_bins - i))/(num_bins);
	XDrawLine(XtDisplay(w), pixmap, gc, 1, ypos2, length, ypos2);
	/* Draw the vertical line */
	XDrawLine(XtDisplay(w), pixmap, gc, length, ypos1, length, ypos2);
    }
    XDrawLine(XtDisplay(w), pixmap, gc, 0, 0, 0, height);
    XtReleaseGC(w, gc);
    XSetWindowBackgroundPixmap(XtDisplay(w), XtWindow(w), pixmap);
    XClearWindow(XtDisplay(w), XtWindow(w));
    PrintAllControlPoints(cmew);
}
