/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



#ifdef OS2
#include <stdlib.h>
#include <types.h>
#endif
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/cursorfont.h>
#include <X11/IntrinsicP.h>
#include <X11/Intrinsic.h>
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/Composite.h>
#include <X11/Constraint.h>
#include <Xm/XmP.h>
#include <Xm/Xm.h>
#include "WorkspaceW.h"
#include "WorkspaceP.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif



#define DEPTH 5
#define LOOKLEFT 0
#define LOOKRIGHT 1
#define LOOKUP 2
#define LOOKDOWN 3
#define WIDGET_TYPE 0
#define LINE_TYPE 1
#define SLOT_LEN 40

#define Abs(A) ((A) < 0 ? -(A) : (A))

typedef struct _LineList {
    XmWorkspaceLine line;
    struct _LineList *next;
}               LineList;

/* External Functions */

void ReallocCollideLists(XmWorkspaceWidget ww); /* From WorkspaceW.c */
void  AugmentExposureAreaForLine (XmWorkspaceWidget ww,
                                         XmWorkspaceLine line);

/* Internal Functions */

static LineElement *
ManhattanWorker(XmWorkspaceWidget ww, int srcx, int srcy,
	  int dstx, int dsty,
	  int level, int *cost,
	  Widget source, Widget destination,
	  XmWorkspaceLine new, int *failnum, LineList *ignore_lines);
static void 
CollapseList(XmWorkspaceWidget ww, LineElement * list, LineList *ignored);
void 
CopyPointsToLine(XmWorkspaceWidget ww, LineElement * list,
		 XmWorkspaceLine line);
void
AddWidgetToCollideList(Widget child);
void 
HideWidgetInCollideList(XmWorkspaceWidget ww, Widget child);
void 
DeleteWidgetFromCollideList(Widget child);
static void 
InsertXCollideList(XmWorkspaceWidget ww, int x, int ymin,
		   int ymax, Widget child, XmWorkspaceLine line, int type);
static void 
InsertYCollideList(XmWorkspaceWidget ww, int y, int ymin,
		   int ymax, Widget child, XmWorkspaceLine line, int type);
static void 
RemoveXCollideList(XmWorkspaceWidget ww, int x, int min, int max);
static void 
RemoveYCollideList(XmWorkspaceWidget ww, int y, int min, int max);
static void 
AddSegmentToCollideList(XmWorkspaceWidget ww,
			XmWorkspaceLine line, int seg_num);
static void 
RemoveSegmentFromCollideList(XmWorkspaceWidget ww,
			     XmWorkspaceLine line, int seg_num);
void 
RemoveLineFromCollideList(XmWorkspaceWidget ww, XmWorkspaceLine line);

static Boolean 
DetectYCollision(XmWorkspaceWidget ww, int y, int x1, int x2,
		 XmWorkspaceLine * line, Widget * widget);
static Boolean 
DetectXCollision(XmWorkspaceWidget ww, int x, int y1, int y2,
		 XmWorkspaceLine * line, Widget * widget);
static LineElement *
CreateDestinationElement(int dstx, int dsty,
			 int *mycost);
static int 
FindHorizontalSlot(XmWorkspaceWidget ww, int x, int y,
		   int horizontal_dir, int vertical_dir, int slot_len);
static int 
FindVerticalSlot(XmWorkspaceWidget ww, int x, int y,
		 int horizontal_dir, int vertical_dir, int slot_len);
static int 
FindVerticalPlumb(XmWorkspaceWidget ww, int srcx, int srcy,
		  int direction, Widget child, Boolean do_lines);
void 
AddLineToCollideList(XmWorkspaceWidget ww, XmWorkspaceLine line);
void 
WidgetCollide(XmWorkspaceWidget ww, Widget child);
static int 
FindHorizontalPlumbIgnoringLines(XmWorkspaceWidget ww, int srcx,
			  int srcy, int direction, LineList * ignore_lines);
void 
MarkCommonLines(XmWorkspaceWidget ww);
static void 
SwapEntries(XmWorkspaceWidget ww, XmWorkspaceLineRec * line1,
	    XmWorkspaceLineRec * line2, XmWorkspaceLineRec * prev_line1,
	    XmWorkspaceLineRec * prev_line2);

static void 
RemoveYWidgetCollideList(XmWorkspaceWidget ww, int y,
			 int min, int max, Widget widget);
static void 
RemoveXWidgetCollideList(XmWorkspaceWidget ww, int x,
			 int min, int max, Widget widget);
static LineList *
FindIgnoredLines(XmWorkspaceLine new,
		 int srcx, int srcy, int dstx, int dsty,
		 Widget source, Widget destination);
static void
FreeIgnoredLines(LineList *ignore_lines);

static LineElement *
MakeLineElement(int x, int y);

static LineElement *
ObstructedFind(XmWorkspaceWidget ww,
	       LineElement *element, 
	       int ymax, 
	       int horizontal_dir,
	       int srcx, int srcy, int dstx, int dsty,
	       Widget source, Widget destination,
	       int level,
	       int *cost,
	       XmWorkspaceLine new, int *failnum, LineList *ignore_lines);
static int
MeasureLine(LineElement *line);
static Boolean 
DetectXCollisionIgnoringLines(XmWorkspaceWidget ww, int x, int y1, int y2,
			      XmWorkspaceLine * line, Widget * widget,
			      LineList *ignored);
static Boolean 
DetectYCollisionIgnoringLines(XmWorkspaceWidget ww, int y, int x1, int x2,
			      XmWorkspaceLine * line, Widget * widget,
			      LineList *ignored);
static int
FindDogleg(XmWorkspaceWidget ww, 
	   LineElement *line,
	   LineElement *dogleg,
	   int ymax,
	   int *cost,
	   int horizontal_dir,
	   int vertical_dir);
static LineElement*
ReverseLine(LineElement* line);

static int      stub_length;
static Boolean  horizontal_path_clear;

/*
 * Routine Manhattan Purpose: Main Manhattan line routing Program.
 * This routine starts at src{x,y}, and proceeds to dst{x,y} after
 * allowing for "stub_length" down from source, or up from dest.
 * This routine ALWAYS starts out moving vertically, never horizontally.
 * Also, src always heads down and dst always heads up.
 *
 * SUMMARY:
 *   Case 1: Check to see if this is a vertical line.
 *   If first level,
 *	Init
 *	Check for common lines
 *	Case 2: we are plunging to the same box as the shared line, pitchfork
 *	Case 3: use as much of the line as possible, recurse
 *   Case 4: we are going down, but not far enough for stubs, etc.
 *   Case 5: we have to go back up
 *   Check to see if the plumb down from src overlaps plumb up from dest
 *   Case 6: vertical line (same as case 1?)
 *   Case 7: two vertical lines and a cross bar.
 *   Plumbs don't overlap, or there is no clear horizontal line.
 *   Case 8: 
 *   Case 8A: 
 *   default
 */
LineElement *
Manhattan(XmWorkspaceWidget ww, int srcx, int srcy,
	  int dstx, int dsty,
	  int level, int *cost,
	  Widget source, Widget destination,
	  XmWorkspaceLine new, int *failnum)
{
    register int    ypos, xmax, ymax;
    int             xmin;
    int             ymin;
    int		    mycost, lower_cost;
    LineElement     *min_line;
    LineElement	    *src_element=NULL, *dst_element;
    LineElement	    *previous_element=NULL, *current_element=NULL;
    LineElement	    *remainder_line, *line;
    int             horizontal_dir;
    int             newx;
    int             slotx;
    XmWorkspaceLine ret_line;
    Widget          ret_widget;
    LineList       *ignore_lines, *current_ignore;
    XmWorkspaceLine copy_line;
    int             delta_x, delta_y, dist, min_dist, min_cost, min_point, i;

    level++;
    remainder_line = NULL;

    /*
     * CASE 1:
     * If the line is vertical, and the source is above the dest, and
     * we can connect the source and dest directly, do connect them.
     */
    if ((srcx == dstx) && (srcy < dsty))
    {
#ifdef TRACE
	printf("Lev = %d, srcx = dstx\n", level);
#endif
	ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, NULL, TRUE);
	ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP, NULL, TRUE);
	/* If there is clear sailing between the source & dest... */
	if ((ymax > ymin) && (ymax != -1) && (ymin != -1))
	{
	    src_element = MakeLineElement(srcx, srcy);
	    mycost = 1;
	    dst_element = CreateDestinationElement(dstx, dsty, &mycost);
	    src_element->next = dst_element;
	    *cost = mycost;
	    return src_element;
	}
    }
    /*
     * At the first level, see which of the existing lines share this source
     * or destination.  Pick a line to "copy" i.e. use as much of the
     * exisiting "copy_line" as makes sense.
     */
#ifdef TRACE
    printf("Lev = %d, Initialization\n", level);
#endif
    *failnum = 0;
    stub_length = ww->workspace.wtol + ww->workspace.ltol +
	ww->workspace.line_thickness / 2 + 1;
    new->is_to_be_collapsed = TRUE;
    new->reverse_on_copy = FALSE;
    /*
     * See if we have at least one clear horizontal
     * path between the source and destination 
     */
    if (srcy < dsty)
    {
	ymin = srcy;
	ymax = dsty;
    }
    else
    {
	ymin = dsty;
	ymax = srcy;
    }
    if (srcx < dstx)
    {
	xmin = source->core.x + source->core.width + ww->workspace.wtol + 1;
	xmax = (destination->core.x - 1) - ww->workspace.wtol;
    }
    else
    {
	xmin = destination->core.x + destination->core.width +
	    ww->workspace.wtol + 1;
	xmax = (source->core.x - 1) - ww->workspace.wtol;
    }
    horizontal_path_clear = False;
    for (ypos = ymin; ypos <= ymax; ypos++)
    {
	if (!DetectYCollision(ww, ypos, xmin, xmax, 
			      &ret_line, &ret_widget))
	{
	    horizontal_path_clear = True;
	    break;
	}
    }
#ifdef TRACE
    printf("hor path clear=%d\n", horizontal_path_clear);
#endif

    /* Determine the set of lines that share the same initial and
     * terminal point with 'new'.  These lines are ignored in colision
     * detection, and candidates for the "copy_line".  Put these
     * into ignore_lines.
     */
    ignore_lines = FindIgnoredLines(new, srcx, srcy, dstx, dsty,
				    source, destination);
#ifdef TRACE
    printf("Ignore lines = %d\n", ignore_lines);
#endif
    if (ignore_lines)
    {
	/*
	 * We have a list of lines to ignore.  First, determine which of
	 * these paths is the best one to copy.  The criterium for this
	 * is which line ends up closest to our final destination. 
	 * XXX Should we use Manhattan distance or real distance?
	 */
	copy_line = NULL;
	min_dist = 10000000;
	/* Traverse the ignore list */
	for (current_ignore = ignore_lines; current_ignore != NULL;
	     current_ignore = current_ignore->next)
	{
	    /* Do not follow the path of a line that will be moved! */
	    if (current_ignore->line->is_to_be_moved)
		continue;
	    delta_x = Abs(dstx -
			  current_ignore->line->point[
				current_ignore->line->num_points - 1].x);
	    delta_y = Abs(dsty -
			  current_ignore->line->point[
				current_ignore->line->num_points - 1].y);
	    dist = delta_x * delta_x + delta_y * delta_y;
	    if (dist < min_dist)
	    {
		copy_line = current_ignore->line;
		min_dist = dist;
	    }
	}
	/*
	 * We have found a line to copy.  First, determine which portion
	 * of the path we want to use.  This will be done by determining
	 * which point on the existing path is closest to our actual
	 * dest.  The first cut at this is to use this point and the
	 * final destination while calling this routine recursively 
	 *
	 * Do not use the first or last points of the line. 
	 */
	if (copy_line != NULL)
	{
#ifdef TRACE
	    printf("Lev = %d, Copy a line\n", level);
#endif
	    /*
	     * CASE 2:
	     * If we are copying the entire line except for the last
	     * point, i.e. if we share the same destination widget,  then
	     * we will "PitchFork", or go almost all the way, and then
	     * branch "stub_length" away from the end to get to the correct
	     * dest point.
	     */
	    if (copy_line->destination == destination &&
		(dsty - copy_line->point[copy_line->num_points - 2].y >
		    4 * stub_length ||
		dsty - copy_line->point[copy_line->num_points - 2].y == 
		    stub_length))
	    {
		mycost = 1;
		for (i = 0; i < copy_line->num_points; i++)
		{
		    current_element = MakeLineElement(
			copy_line->point[i].x,
			copy_line->point[i].y);
		    mycost++;
		    if (i == 0)
		    {
			src_element = current_element;
		    }
		    else
		    {
			previous_element->next = current_element;
		    }
		    previous_element = current_element;
		}
		current_element->y = dsty - stub_length;
		current_element->next = MakeLineElement(
		    dstx, dsty - stub_length);
		mycost++;
		current_element = current_element->next;
		current_element->next =
		    CreateDestinationElement(dstx, dsty, &mycost);
		*cost = mycost;
		new->is_to_be_collapsed = FALSE;
		FreeIgnoredLines(ignore_lines);
		return src_element;
	    }
	    /*
	     * CASE 3:
	     * Otherwise, we have a common line.  Find the closest point 
	     * (except the end points).  If we find one, recurse with
	     * that point as the source.
	     * XXX Should we use Manhattan distance or real distance?
	     */
	    min_dist = 1000000;
	    min_cost = 1000;
	    min_line = NULL;
	    min_point = 0;
	    for (horizontal_dir = LOOKLEFT, i = copy_line->num_points - 2;
		 i >= 1;
		 horizontal_dir == LOOKLEFT? horizontal_dir = LOOKRIGHT:
					     (horizontal_dir = LOOKLEFT, i--))
	    {
		/*
		 * Create a horizontal segment, and call routine
		 * recursively.
		 */
		newx = FindHorizontalPlumbIgnoringLines(ww,
					  copy_line->point[i].x,
					  copy_line->point[i].y,
					  horizontal_dir, ignore_lines);
		/*
		 * Adjust the plumb in to the destination.
		 * If we encountered an obstruction before the
		 * destination x position, use a point mid way in
		 * between the source and the obstruction.  It looks
		 * cleaner and it avoids potentially complex (at this
		 * point, unroutable) situations.
		 */
		if (horizontal_dir == LOOKLEFT)
		{
		    if (dstx > copy_line->point[i].x)
		    {
			slotx = FindVerticalSlot(ww, copy_line->point[i].x,
						 copy_line->point[i].y,
						 LOOKLEFT, LOOKDOWN, 1);
			if (slotx < newx)
			    continue;
			newx = slotx;
		    }
		    else
		    {
			if (newx < dstx)
			    newx = dstx;
			else
			    newx = copy_line->point[i].x -
				(copy_line->point[i].x - newx) / 2;
			newx = FindVerticalSlot(ww, newx, copy_line->point[i].y,
						LOOKRIGHT, LOOKDOWN, 1);
		    }
		}
		else
		{
		    if (dstx < copy_line->point[i].x)
		    {
			slotx = FindVerticalSlot(ww, copy_line->point[i].x,
						 copy_line->point[i].y,
						 LOOKRIGHT, LOOKDOWN, 1);
			if (slotx > newx)
			    continue;
			newx = slotx;
		    }
		    else
		    {
			if (newx > dstx)
			    newx = dstx;
			else
			    newx = copy_line->point[i].x +
				(newx - copy_line->point[i].x) / 2;
			newx = FindVerticalSlot(ww, newx, copy_line->point[i].y,
						LOOKLEFT, LOOKDOWN, 1);
		    }
		}
		/*
		 * Create a stub up from the destination, and 
		 * see if it clears.  If so, call worker, else bomb.
		 */
		ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP, NULL, TRUE);
		if (dsty - stub_length < ymin)
		{
		    lower_cost = 1000;
		}
		else
		{
		    lower_cost = 0;
		    remainder_line = ManhattanWorker(ww, newx,
			  copy_line->point[i].y, dstx, dsty-stub_length, level,
			  &lower_cost, source, destination, new, failnum,
			  ignore_lines);
		    if (new->reverse_on_copy)
		    {
			remainder_line = ReverseLine(remainder_line);
			new->reverse_on_copy = FALSE;
		    }
		    /* Tack the new stub on the end */
		    if (remainder_line)
		    {
			for (line = remainder_line; 
			     line->next;
			     line = line->next)
			;
			if (line->x == dstx)
			{
			    line->y = dsty;
			}
			else
			{
			    line->next =
				CreateDestinationElement(dstx, dsty+stub_length,
							 &lower_cost);
			}
		    }
		}
		if (lower_cost < min_cost)
		{
		    min_cost = lower_cost;
		    min_dist = MeasureLine(remainder_line) + 
			       Abs(copy_line->point[i].x - newx);
		    if (min_line)
		    {
			FreeLineElementList(min_line);
		    }
		    min_line = remainder_line;
		    min_point = i;
		}
		else if (lower_cost == min_cost &&
		    (dist = MeasureLine(remainder_line) + 
			    Abs(copy_line->point[i].x - newx)) < min_dist)
		{
		    min_dist = dist;
		    if (min_line)
		    {
			FreeLineElementList(min_line);
		    }
		    min_line = remainder_line;
		    min_point = i;
		}
		else
		{
		    FreeLineElementList(remainder_line); 
		    remainder_line = NULL;
		}
	    }
	    if (min_cost < 999)
	    {
		/*
		 * We now have the entire line, we just have to allocate
		 * things and return them in the usual format 
		 */

		mycost = min_cost;
		for (i = 0; i <= min_point; i++)
		{
		    current_element = MakeLineElement(
			copy_line->point[i].x,
			copy_line->point[i].y);

		    mycost++;
		    if (i == 0)
		    {
			src_element = current_element;
		    }
		    else
		    {
			previous_element->next = current_element;
		    }
		    previous_element = current_element;
		}
		current_element->next = min_line;

		CollapseList(ww, current_element, ignore_lines);
		FreeIgnoredLines(ignore_lines);
		new->is_to_be_collapsed = FALSE;

		*cost = mycost;
		return src_element;
	    }
	    else
	    {
		FreeLineElementList(min_line);
	    }
	}				/* if (copy_lines != NULL) */
	/* Free the ignore list */
	FreeIgnoredLines(ignore_lines);
    }				/* if (ignore_lines) */


    if ((srcy+stub_length == dsty-stub_length) && horizontal_path_clear)
    {
	    src_element = MakeLineElement(srcx, srcy);
	    src_element->next = MakeLineElement(srcx, srcy+stub_length);
	    src_element->next->next = MakeLineElement(dstx, dsty-stub_length);
	    mycost = 3;
	    src_element->next->next->next = 
		CreateDestinationElement(dstx, dsty, &mycost);
	    *cost = mycost;
	    return src_element;
    }
	
    /*
     * Put in the stubs
     */
    ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP,   NULL, TRUE);
    ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, NULL, TRUE);
    if (dsty - stub_length < ymin || srcy + stub_length > ymax)
    {
	*cost = 1000;
    }
    else
    {
	*cost = 0;
	remainder_line = ManhattanWorker(ww,
	      srcx, srcy+stub_length, dstx, dsty-stub_length,
	      level, cost, source, destination, new, failnum, NULL);
	if (new->reverse_on_copy)
	{
	    remainder_line = ReverseLine(remainder_line);
	    new->reverse_on_copy = FALSE;
	}
	/* Tack the new stub on the end */
	if (remainder_line)
	{
	    for (line = remainder_line; 
		 line->next;
		 line = line->next)
	    ;
	    if (line->x == dstx)
	    {
		line->y = dsty;
	    }
	    else
	    {
		line->next =
		    CreateDestinationElement(dstx, dsty, cost);
		++(*cost);
	    }
	    if (remainder_line->x == srcx)
	    {
		remainder_line->y = srcy;
	    }
	    else
	    {
		line = MakeLineElement(srcx, srcy);
		line->next = remainder_line;
		remainder_line = line;
		++(*cost);
	    }
	}
    }
    /*
     * If the line was unroutable, force a simple route, ignoring any collisions
     */
    if ((*cost >= 999) && ww->workspace.force_route)
    {
	if (srcy <= dsty)
	{
	    src_element = MakeLineElement(srcx, srcy);
	    mycost = 1;
	    current_element = src_element;

	    current_element->next = 
		MakeLineElement(srcx, srcy + (dsty-srcy)/2);
	    mycost++;
	    current_element = current_element->next;

	    current_element->next = 
		MakeLineElement(dstx, srcy + (dsty-srcy)/2);
	    mycost++;
	    current_element = current_element->next;

	    current_element->next = 
		CreateDestinationElement(dstx, dsty, &mycost);
	    *cost = mycost;
	    FreeLineElementList(remainder_line);
	    return src_element;
	}
	else
	{
	    src_element = MakeLineElement(srcx, srcy);
	    mycost = 1;
	    current_element = src_element;

	    current_element->next = 
		MakeLineElement(srcx, srcy + stub_length);
	    mycost++;
	    current_element = current_element->next;

	    current_element->next = 
		MakeLineElement(srcx + (dstx-srcx)/2, srcy + stub_length);
	    mycost++;
	    current_element = current_element->next;

	    current_element->next = 
		MakeLineElement(srcx + (dstx-srcx)/2, dsty - stub_length);
	    mycost++;
	    current_element = current_element->next;

	    current_element->next = 
		MakeLineElement(dstx, dsty - stub_length);
	    mycost++;
	    current_element = current_element->next;

	    current_element->next = 
		CreateDestinationElement(dstx, dsty, &mycost);
	    *cost = mycost;
	    FreeLineElementList(remainder_line);
	    return src_element;
	}

    }
    return remainder_line;
}

static LineElement *
ManhattanWorker(XmWorkspaceWidget ww, int srcx, int srcy,
	  int dstx, int dsty,
	  int level, int *cost,
	  Widget source, Widget destination,
	  XmWorkspaceLine new, int *failnum,
	  LineList       *ignore_lines)
{
    register int    xpos, ypos, xmax=0, ymax;
    int             xmin=0, xmax_src, xmax_dst, xmin_src, xmin_dst;
    int             ymin, ycenter;
    int             mycost, right_cost, left_cost,
		    src_left_cost, src_right_cost,
		    dst_left_cost, dst_right_cost;
    int		    lower_level_cost[5];
    LineElement     *line_center[5];
    LineElement     src_dogleg1;
    LineElement     src_dogleg2;
    LineElement     src_dogleg3;
    LineElement     src_dogleg4;
    LineElement     *src_element, *dst_element,
		    *element1 = NULL, *element2 = NULL, *element3 = NULL,
		    *remainder_line, *line, *right_line=NULL, *left_line=NULL,
		    *src_line=NULL, *dst_line=NULL;
    int             vertical_dir, horizontal_dir;
    int             newx;
    LineElement     new_src, new_dst,
		    src_dogleg_left, src_dogleg_right,
		    dst_dogleg_left, dst_dogleg_right,
		    src_line_left, src_line_right,
		    dst_line_left, dst_line_right;
    Boolean         do_lines;
    XmWorkspaceLine ret_line;
    Widget          ret_widget;
    int             inc;
    int		    left_failnum;
    int		    right_failnum;

    level++;

    /*
     * CASE 1:
     * If the line is vertical, and the source is above the dest, and
     * we can connect the source and dest directly, do connect them.
     */
    if ((srcx == dstx) && (srcy < dsty))
    {
#ifdef TRACE
	printf("Lev = %d, srcx = dstx\n", level);
#endif
	ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, NULL, TRUE);
	ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP, NULL, TRUE);
	/* If there is clear sailing between the source & dest... */
	if ((ymax > ymin) && (ymax != -1) && (ymin != -1))
	{
	    src_element = MakeLineElement(srcx, srcy);
	    mycost = 1;
	    dst_element = CreateDestinationElement(dstx, dsty, &mycost);
	    src_element->next = dst_element;
	    *cost = mycost;
	    return src_element;
	}
    }

    /* Initialization is complete, and we didn't find a simple case 
     * during it.
     * Allocate src_element - we will always need it.
     */
#ifdef TRACE
    printf("Lev = %d, Allocating source element\n", level);
#endif
    src_element = MakeLineElement(srcx, srcy);
    mycost = 1;
    if (level > 15)
    {
	*cost = 999;
	return src_element;
    }

    /*
     * CASE 4:
     * If the source above the destination, but close (w/in 2*stub_length), 
     * we create the stub segments, reverse the source and destination
     * points, and call the routine recursively.
     */
    if ((srcy <= dsty) && 
	((srcy + stub_length) >= (dsty - stub_length)) &&
	(level == 1) && 
	horizontal_path_clear && xmin < xmax)
    {
#ifdef TRACE
	printf("Lev = %d, src & dst are w/in 4 stub_length\n", level);
#endif
	ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, NULL, TRUE);
	ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP, NULL, TRUE);

	/*
	 * if ymax = -1, there was no vertical plumb available and hence,
	 * there is NO route available from this point! 
	 */
	if (ymax == -1)
	{
	    *failnum = 1;
#ifdef Printit
	    printf("Routing Failure1! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    return src_element;
	}

	/* if ymin = -1, there was no vertical plumb available */
	/* This is an unroutable case! */
	if (ymin == -1)
	{
	    *failnum = 2;
#ifdef Printit
	    printf("Routing Failure2! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    return src_element;
	}

	/* First, calculate the lines for the source */
	if (srcx < dstx)
	{
	    horizontal_dir = LOOKRIGHT;
	}
	else
	{
	    horizontal_dir = LOOKLEFT;
	}
	vertical_dir = LOOKUP;

	newx = FindVerticalSlot(ww, srcx, srcy, horizontal_dir, vertical_dir,
				SLOT_LEN);
	/* If newx = -1, there was no open slot! */
	if (newx == -1)
	{
	    /* Try finding a vertical slot in the other direction */
	    if (srcx < dstx)
	    {
		horizontal_dir = LOOKRIGHT;
	    }
	    else
	    {
		horizontal_dir = LOOKLEFT;
	    }
	    newx = FindVerticalSlot(ww, srcx, srcy, horizontal_dir, vertical_dir,
				    SLOT_LEN);
	    if (newx == -1)
	    {
		*failnum = 3;
#ifdef Printit
		printf("Routing Failure3! level = %d \n", level);
#endif
		*cost = 999;
		/* Bundle up the allocated elements so they will be returned */
		src_element->next = NULL;
		return src_element;
	    }
	}
	for (ypos = srcy + stub_length; ypos <= ymax; ypos++)
	{
	    if (!DetectYCollision(ww, ypos, srcx, newx, &ret_line, &ret_widget))
	    {
		src_line = MakeLineElement(srcx, ypos);
		mycost += 1;
		new_dst.x = FindVerticalSlot(ww, srcx, ypos,
			    horizontal_dir, vertical_dir, Abs(ypos - srcy));
		new_dst.y = ypos;
		break;
	    }
	}
	if (ypos >= ymax)
	{
	    *failnum = 4;
#ifdef Printit
	    printf("Routing Failure4! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    return src_element;
	}
	/* Now calculate the lines for the destination */
	if (srcx < dstx)
	{
	    horizontal_dir = LOOKLEFT;
	}
	else
	{
	    horizontal_dir = LOOKRIGHT;
	}
	vertical_dir = LOOKDOWN;
	newx = FindVerticalSlot(ww, dstx, dsty, horizontal_dir, vertical_dir,
				SLOT_LEN);
	/* If newx = -1, there was no open slot! */
	if (newx == -1)
	{
	    *failnum = 5;
#ifdef Printit
	    printf("Routing Failure5! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    return src_element;
	}
	for (ypos = dsty - stub_length; ypos >= ymin; ypos--)
	{
	    if (!DetectYCollision(ww, ypos, dstx, newx, &ret_line, &ret_widget))
	    {
		dst_line = MakeLineElement(dstx, ypos);
		mycost += 1;
		/*
		 * We have to adjust the x position because of the new y
		 * position 
		 */
		new_src.x = FindVerticalSlot(ww, dstx, ypos,
			    horizontal_dir, vertical_dir, Abs(ypos - dsty));
		new_src.y = ypos;
		break;
	    }
	}
	if (ypos <= ymin)
	{
	    *failnum = 6;
#ifdef Printit
	    printf("Routing Failure6! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    return src_element;
	}
	dst_element = CreateDestinationElement(dstx, dsty, &mycost);
	/* Try to find a simple solution, else recurse */
	/* Straight line case */
	if ((new_src.y == new_dst.y) && (!DetectYCollision(ww, new_src.y,
			     new_src.x, new_dst.x, &ret_line, &ret_widget)))
	{
	    src_element->next = src_line;
	    src_line->next = dst_line;
	    dst_line->next = dst_element;
	    *cost = mycost;
	    return src_element;
	}
	/*
	 * Look for overlap between two horizontal plumbs, and try to find a
	 * single segment to bridge the gap in the overlap region. 
	 */
	if (new_src.x < new_dst.x)
	{
	    xmax = FindHorizontalPlumbIgnoringLines(ww, new_src.x, new_src.y,
						    LOOKRIGHT, ignore_lines);
	    xmin = FindHorizontalPlumbIgnoringLines(ww, new_dst.x, new_dst.y,
						    LOOKLEFT, ignore_lines);
	    xmax = MIN(xmax, new_dst.x);
	    xmin = MAX(xmin, new_src.x);
	}
	else
	{
	    xmax = FindHorizontalPlumbIgnoringLines(ww, new_dst.x, new_dst.y,
						    LOOKRIGHT, ignore_lines);
	    xmin = FindHorizontalPlumbIgnoringLines(ww, new_src.x, new_src.y,
						    LOOKLEFT, ignore_lines);
	    xmax = MIN(xmax, new_src.x);
	    xmin = MAX(xmin, new_dst.x);
	}
	if (xmin < xmax)	/* If there is overlap */
	{
#ifdef TRACE
	    printf("Lev = %d, There is overlap\n", level);
#endif
	    for (xpos = xmin; xpos <= xmax; xpos++)
	    {
		if (!DetectXCollision(ww, xpos, new_dst.y,
				      new_src.y, &ret_line, &ret_widget))
		{
#ifdef TRACE
		    printf("Lev = %d, Simple Solution\n", level);
#endif
		    element1 = MakeLineElement(xpos, src_line->y);
		    element2 = MakeLineElement(xpos, dst_line->y);
		    mycost += 2;
		    src_element->next = src_line;
		    src_line->next = element1;
		    element1->next = element2;
		    element2->next = dst_line;
		    dst_line->next = dst_element;
		    *cost = mycost;
		    return src_element;
		}
	    }
	}
	/*
	 * No Simple Solution, so recurse. Note the reversal of source and
	 * destination. 
	 */
#ifdef TRACE
	printf("Lev = %d, Recurse\n", level);
#endif
	line_center[1] = ManhattanWorker(ww, new_src.x, new_src.y,
			    new_dst.x, new_dst.y, level, &lower_level_cost[1],
			    source, destination, new, failnum, ignore_lines);
	if (new->reverse_on_copy)
	{
	    line_center[1] = ReverseLine(line_center[1]);
	    new->reverse_on_copy = FALSE;
	}
	dst_element->next = dst_line;
	dst_line->next = line_center[1];
	line = line_center[1];
	while (line)
	{
	    if (line->next == NULL)
	    {
		line->next = src_line;
		break;
	    }
	    line = line->next;
	}
	src_line->next = src_element;
	*cost = mycost + lower_level_cost[1];
	new->reverse_on_copy = TRUE;
	return dst_element;
    }
    /*
     * CASE 5:
     * If we're going to have to go back up (src below dst):
     * Add doglegs to go around the source & dest widgets. We will try left
     * and right doglegs for both the source and destination.  We then try
     * each of the combinations (src left,right to dst left, right), finding 
     * either a simple solution (if I extend both doglegs far enough, they can
     * be connected by a vertical line) or a complicated one derived by 
     * recursion.  The path of least cost will be chosen, solving ties by 
     * line length (solving lenght ties by going left). 
     */
    else if ((srcy > dsty) && (horizontal_path_clear || xmin > xmax))
    {
#ifdef TRACE
	printf("Lev = %d, src below dst\n", level);
#endif
	dst_element = CreateDestinationElement(dstx, dsty, &mycost);
	mycost += 1;

	/*
	 * For both src & dst, create two horizontal lines, a dogleg left,
	 * and a dogleg right.
	 * Src_line and dst_line are points that define the end of a stub
	 * coming off a widget, and the horizontal position of the dogleg 
	 */
	src_line_left.x = srcx;
	src_line_left.y = srcy;
	dst_line_left.x = dstx;
	dst_line_left.y = dsty;
	src_line_right.x = srcx;
	src_line_right.y = srcy;
	dst_line_right.x = dstx;
	dst_line_right.y = dsty;

	src_left_cost  = 0;
	src_right_cost = 0;
	dst_left_cost  = 0;
	dst_right_cost = 0;

	/*
	 * First, let's do the source dogleg to the left
	 */
	horizontal_dir = LOOKLEFT;
	vertical_dir = LOOKUP;

#ifdef TRACE
	printf("Lev = %d, Create 4 doglegs\n", level);
#endif
	ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, NULL, TRUE);

	/*
	 * if ymax = -1, there was no vertical plumb available and hence,
	 * there is NO route available from this point! 
	 */
	if (ymax == -1)
	{
	    *failnum = 7;
#ifdef Printit
	    printf("Routing Failure7! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    FreeLineElementList(dst_element);
	    return src_element;
	}

	FindDogleg(ww, &src_line_left, &src_dogleg_left, ymax, 
		   &src_left_cost, horizontal_dir, vertical_dir);

	/* Try looking the other way */
	horizontal_dir = LOOKRIGHT;
	FindDogleg(ww, &src_line_right, &src_dogleg_right, ymax, 
		   &src_right_cost, horizontal_dir, vertical_dir);

	/* Now, create the destination dogleg */
	horizontal_dir = LOOKLEFT;
	vertical_dir = LOOKDOWN;

	ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP, NULL, TRUE);

	/* if ymin = -1, there was no vertical plumb available */
	/* This is an unroutable case! */
	if (ymin == -1)
	{
	    *failnum = 8;
#ifdef Printit
	    printf("Routing Failure8! level = %d \n", level);
#endif
	    *cost = 999;
	    /* Bundle up the allocated elements so they will be returned */
	    src_element->next = NULL;
	    FreeLineElementList(dst_element);
	    return src_element;
	}

	/* find horizontal endpoint for the new line */
	FindDogleg(ww, &dst_line_left, &dst_dogleg_left, ymin, 
		   &dst_left_cost, horizontal_dir, vertical_dir);

	/* Try looking the other way */
	horizontal_dir = LOOKRIGHT;
	FindDogleg(ww, &dst_line_right, &dst_dogleg_right, ymin, 
		   &dst_right_cost, horizontal_dir, vertical_dir);

	/*
	 * We are done creating the two doglegs, so we now try to find a
	 * simple path between the two dogleg endpoints.  If we can't, we
	 * will call Manhattan with the new source and destination points
	 * Note - exchange the source and destination points 
	 */
#ifdef TRACE
	printf("Lev = %d, Look for simple solution\n", level);
#endif
	lower_level_cost[1] = 
	    lower_level_cost[2] = 
	    lower_level_cost[3] = 
	    lower_level_cost[4] = 0;
	line_center[1] = NULL;
	line_center[2] = NULL;
	line_center[3] = NULL;
	line_center[4] = NULL;
	if (src_left_cost < 999 && dst_right_cost < 999)
	{
	    if (src_dogleg_left.x > dst_dogleg_right.x)
	    {
		/* Insure the is no overlap by default */
		xmin = 10000;
		xmax = -1;
		xmax = FindHorizontalPlumbIgnoringLines(ww, dst_dogleg_right.x,
					   dst_dogleg_right.y, LOOKRIGHT,
					   ignore_lines);
		if (xmax > src_dogleg_left.x)
		{
		    xmax = src_dogleg_left.x;
		}
		xmin = FindHorizontalPlumbIgnoringLines(ww, src_dogleg_left.x,
					   src_dogleg_left.y, LOOKLEFT,
					   ignore_lines);
		if (xmin < dst_dogleg_right.x)
		{
		    xmin = dst_dogleg_right.x;
		}
		if (xmin < xmax)    /* If there is overlap */
		{
		    for (xpos = xmin; xpos <= xmax; xpos++)
		    {
			if (!DetectXCollision(ww, xpos, dst_dogleg_right.y,
				     src_dogleg_left.y, &ret_line, &ret_widget))
			{
			    line_center[1] = 
				MakeLineElement(
				    dst_line_right.x, dst_line_right.y);
			    line_center[1]->next =
				MakeLineElement (xpos, dst_line_right.y);
			    line_center[1]->next->next =
				MakeLineElement (xpos, src_line_left.y);
			    line_center[1]->next->next->next =
				MakeLineElement (
				    src_line_left.x, src_line_left.y);
			    lower_level_cost[1] = 4;
			    break;
			}
		    }
		}
	    }
	    if (line_center[1] == NULL)
	    {
		line_center[1] = 
		    MakeLineElement(
			dst_line_right.x, dst_line_right.y);
		line_center[1]->next = ManhattanWorker(ww, dst_dogleg_right.x,
				    dst_dogleg_right.y, src_dogleg_left.x,
				    src_dogleg_left.y, level,
				    &lower_level_cost[1], 
				    source, destination, new, failnum,
				    ignore_lines);
		if (new->reverse_on_copy)
		{
		    line_center[1]->next = ReverseLine(line_center[1]->next);
		    new->reverse_on_copy = FALSE;
		}
		for (line = line_center[1]->next; line; line = line->next)
		{
		    if (!line->next)
		    {
			line->next = MakeLineElement (
			    src_line_left.x, src_line_left.y);
			break;
		    }
		}
		lower_level_cost[1] += 2;
	    }
	}
	else
	{
	    lower_level_cost[1] = 999;
	}

	if (src_right_cost < 999 && dst_left_cost < 999)
	{
	    line_center[2] = NULL;
	    if (src_dogleg_right.x < dst_dogleg_left.x)
	    {
		/* Insure there is no overlap by default */
		xmin = 10000;
		xmax = -1;
		xmax = FindHorizontalPlumbIgnoringLines(ww, src_dogleg_right.x,
					   src_dogleg_right.y, LOOKRIGHT,
					   ignore_lines);
		if (xmax > dst_dogleg_left.x)
		{
		    xmax = dst_dogleg_left.x;
		}
		xmin = FindHorizontalPlumbIgnoringLines(ww, dst_dogleg_left.x,
					   dst_dogleg_left.y, LOOKLEFT,
					   ignore_lines);
		if (xmin < src_dogleg_right.x)
		{
		    xmin = src_dogleg_right.x;
		}
		if (xmin < xmax)	/* If there is overlap */
		{
		    for (xpos = xmin; xpos <= xmax; xpos++)
		    {
			if (!DetectXCollision(ww, xpos, dst_dogleg_left.y,
				    src_dogleg_right.y, &ret_line, &ret_widget))
			{
			    line_center[2] =
				MakeLineElement(
				    dst_line_left.x, dst_line_left.y);
			    line_center[2]->next =
				MakeLineElement(xpos, dst_line_left.y);
			    line_center[2]->next->next =
				MakeLineElement(xpos, src_line_right.y);
			    line_center[2]->next->next->next =
				MakeLineElement(
				    src_line_right.x, src_line_right.y);
			    lower_level_cost[2] = 4;
			    break;
			}
		    }
		}
	    }
	    if (line_center[2] == NULL)
	    {
		line_center[2] = 
		    MakeLineElement(
			dst_line_left.x, dst_line_left.y);
		line_center[2]->next = ManhattanWorker(ww, dst_dogleg_left.x,
				    dst_dogleg_left.y, src_dogleg_right.x,
				    src_dogleg_right.y, level, 
				    &lower_level_cost[2],
				    source, destination, new, failnum,
				    ignore_lines);
		if (new->reverse_on_copy)
		{
		    line_center[2]->next = ReverseLine(line_center[2]->next);
		    new->reverse_on_copy = FALSE;
		}
		for (line = line_center[2]->next; line; line = line->next)
		{
		    if (!line->next)
		    {
			line->next = MakeLineElement (
			    src_line_right.x, src_line_right.y);
			break;
		    }
		}
		lower_level_cost[2] += 2;
	    }
	}
	else
	{
	    lower_level_cost[2] = 999;
	}
	if (src_left_cost < 999 && dst_left_cost < 999)
	{
	    line_center[3] = NULL;
	    /* Insure there is no overlap by default */
	    xmin = 10000;
	    xmax = -1;
	    xmin_src = FindHorizontalPlumbIgnoringLines(ww, src_dogleg_left.x,
					   src_dogleg_left.y, LOOKLEFT,
					   ignore_lines);
	    xmin_dst = FindHorizontalPlumbIgnoringLines(ww, dst_dogleg_left.x,
					   dst_dogleg_left.y, LOOKLEFT,
					   ignore_lines);
	    xmin = MAX(xmin_src, xmin_dst);
	    xmax = MIN(src_dogleg_left.x, dst_dogleg_left.x);
	    /*
	     * Limit how far into never never land we will go to get around
	     * things 
	     */
	    xmin = MAX(xmin, (xmax - 300));
	    if (xmin < xmax)	/* If there is overlap */
	    {
		for (xpos = xmax; xpos >= xmin; xpos--)
		{
		    if (!DetectXCollision(ww, xpos, dst_dogleg_left.y,
				 src_dogleg_left.y, &ret_line, &ret_widget))
		    {
			line_center[3] =
			    MakeLineElement (dst_line_left.x, dst_line_left.y);
			line_center[3]->next =
			    MakeLineElement (xpos, dst_line_left.y);
			line_center[3]->next->next =
			    MakeLineElement (xpos, src_line_left.y);
			line_center[3]->next->next->next =
			    MakeLineElement (src_line_left.x, src_line_left.y);
			lower_level_cost[3] = 4;
			break;
		    }
		}
	    }
	    if (line_center[3] == NULL)
	    {
		line_center[3] = 
		    MakeLineElement(
			dst_line_left.x, dst_line_left.y);
		line_center[3]->next = ManhattanWorker(ww, dst_dogleg_left.x,
				    dst_dogleg_left.y, src_dogleg_left.x,
				    src_dogleg_left.y, level,
				    &lower_level_cost[3],
				    source, destination, new, failnum,
				    ignore_lines);
		if (new->reverse_on_copy)
		{
		    line_center[3]->next = ReverseLine(line_center[3]->next);
		    new->reverse_on_copy = FALSE;
		}
		for (line = line_center[3]->next; line; line = line->next)
		{
		    if (!line->next)
		    {
			line->next = MakeLineElement (
			    src_line_left.x, src_line_left.y);
			break;
		    }
		}
		lower_level_cost[3] += 2;
	    }
	}
	else
	{
	    lower_level_cost[3] = 999;
	}
	if (src_right_cost < 999 && dst_right_cost < 999)
	{
	    line_center[4] = NULL;
	    /* Insure there is no overlap by default */
	    xmin = 10000;
	    xmax = -1;
	    xmax_src = FindHorizontalPlumbIgnoringLines(ww, src_dogleg_right.x,
					   src_dogleg_right.y, LOOKRIGHT,
					   ignore_lines);
	    xmax_dst = FindHorizontalPlumbIgnoringLines(ww, dst_dogleg_right.x,
					   dst_dogleg_right.y, LOOKRIGHT,
					   ignore_lines);
	    xmax = MIN(xmax_src, xmax_dst);
	    xmin = MAX(src_dogleg_right.x, dst_dogleg_right.x);
	    xmax = MIN(xmax, (xmin + 300));
	    if (xmin < xmax)	/* If there is overlap */
	    {
		for (xpos = xmin; xpos <= xmax; xpos++)
		{
		    if (!DetectXCollision(ww, xpos, dst_dogleg_right.y,
				src_dogleg_right.y, &ret_line, &ret_widget))
		    {
			line_center[4] =
			    MakeLineElement(dst_line_right.x, dst_line_right.y);
			line_center[4]->next =
			    MakeLineElement (xpos, dst_line_right.y);
			line_center[4]->next->next =
			    MakeLineElement (xpos, src_line_right.y);
			line_center[4]->next->next->next =
			    MakeLineElement(src_line_right.x, src_line_right.y);
			lower_level_cost[4] = 4;
			break;
		    }
		}
	    }
	    if (line_center[4] == NULL)
	    {
		line_center[4] = 
		    MakeLineElement(
			dst_line_right.x, dst_line_right.y);
		line_center[4]->next = ManhattanWorker(ww, dst_dogleg_right.x,
				    dst_dogleg_right.y, src_dogleg_right.x,
				    src_dogleg_right.y, level,
				    &lower_level_cost[4],
				    source, destination, new, failnum,
				    ignore_lines);
		if (new->reverse_on_copy)
		{
		    line_center[4]->next = ReverseLine(line_center[4]->next);
		    new->reverse_on_copy = FALSE;
		}
		for (line = line_center[4]->next; line; line = line->next)
		{
		    if (!line->next)
		    {
			line->next = MakeLineElement (
			    src_line_right.x, src_line_right.y);
			break;
		    }
		}
		lower_level_cost[4] += 2;
	    }
	}
	else
	{
	    lower_level_cost[4] = 999;
	}

#ifdef TRACE
	printf("Lev = %d, Choose lowest cost\n", level);
#endif
	if ((lower_level_cost[1] >= 999) &&
	    (lower_level_cost[2] >= 999) &&
	    (lower_level_cost[3] >= 999) &&
	    (lower_level_cost[4] >= 999))
	{
	    *failnum = 9;
#ifdef Printit
	    printf("Routing Failure9! level = %d \n", level);
#endif
	    src_element->next = dst_element;
	    *cost = 999;
	    FreeLineElementList(line_center[1]);
	    FreeLineElementList(line_center[2]);
	    FreeLineElementList(line_center[3]);
	    FreeLineElementList(line_center[4]);
	    return src_element;
	}

	mycost += 2;
	if (lower_level_cost[1] < 999 &&
	    (lower_level_cost[1] < lower_level_cost[2] ||
		(lower_level_cost[1] == lower_level_cost[2] &&
		MeasureLine(line_center[1]) <= MeasureLine(line_center[2]))) &&
	    (lower_level_cost[1] < lower_level_cost[3] ||
		(lower_level_cost[1] == lower_level_cost[3] &&
		MeasureLine(line_center[1]) <= MeasureLine(line_center[3]))) &&
	    (lower_level_cost[1] < lower_level_cost[4] ||
		(lower_level_cost[1] == lower_level_cost[4] &&
		MeasureLine(line_center[1]) <= MeasureLine(line_center[4]))))
	{
	    FreeLineElementList(line_center[2]);
	    FreeLineElementList(line_center[3]);
	    FreeLineElementList(line_center[4]);
	    dst_element->next = line_center[1];
	    line = line_center[1];
	    while (line)
	    {
		if (line->next == NULL)
		{
		    line->next = src_element;
		    break;
		}
		line = line->next;
	    }
	    *cost = mycost + lower_level_cost[1];
	}
	else if (lower_level_cost[2] < 999 &&
	    (lower_level_cost[2] < lower_level_cost[1] ||
		(lower_level_cost[2] == lower_level_cost[1] &&
		MeasureLine(line_center[2]) <= MeasureLine(line_center[1]))) &&
	    (lower_level_cost[2] < lower_level_cost[3] ||
		(lower_level_cost[2] == lower_level_cost[3] &&
		MeasureLine(line_center[2]) <= MeasureLine(line_center[3]))) &&
	    (lower_level_cost[2] < lower_level_cost[4] ||
		(lower_level_cost[2] == lower_level_cost[4] &&
		MeasureLine(line_center[2]) <= MeasureLine(line_center[4]))))
	{
	    FreeLineElementList(line_center[1]);
	    FreeLineElementList(line_center[3]);
	    FreeLineElementList(line_center[4]);
	    dst_element->next = line_center[2];
	    line = line_center[2];
	    while (line)
	    {
		if (line->next == NULL)
		{
		    line->next = src_element;
		    break;
		}
		line = line->next;
	    }
	    *cost = mycost + lower_level_cost[2];
	}
	else if (lower_level_cost[3] < 999 &&
	    (lower_level_cost[3] < lower_level_cost[1] ||
		(lower_level_cost[3] == lower_level_cost[1] &&
		MeasureLine(line_center[3]) <= MeasureLine(line_center[1]))) &&
	    (lower_level_cost[3] < lower_level_cost[2] ||
		(lower_level_cost[3] == lower_level_cost[2] &&
		MeasureLine(line_center[3]) <= MeasureLine(line_center[2]))) &&
	    (lower_level_cost[3] < lower_level_cost[4] ||
		(lower_level_cost[3] == lower_level_cost[4] &&
		MeasureLine(line_center[3]) <= MeasureLine(line_center[4]))))
	{
	    FreeLineElementList(line_center[1]);
	    FreeLineElementList(line_center[2]);
	    FreeLineElementList(line_center[4]);
	    dst_element->next = line_center[3];
	    line = line_center[3];
	    while (line)
	    {
		if (line->next == NULL)
		{
		    line->next = src_element;
		    break;
		}
		line = line->next;
	    }
	    *cost = mycost + lower_level_cost[3];
	}
	else if (lower_level_cost[4] < 999)
	{
	    FreeLineElementList(line_center[1]);
	    FreeLineElementList(line_center[2]);
	    FreeLineElementList(line_center[3]);
	    dst_element->next = line_center[4];
	    line = line_center[4];
	    while (line)
	    {
		if (line->next == NULL)
		{
		    line->next = src_element;
		    break;
		}
		line = line->next;
	    }
	    *cost = mycost + lower_level_cost[4];
	}
	else
	{
	    *cost = 999;
	    FreeLineElementList(line_center[1]);
	    FreeLineElementList(line_center[2]);
	    FreeLineElementList(line_center[3]);
	    FreeLineElementList(line_center[4]);
	    dst_element->next = NULL;
	}

	new->reverse_on_copy = TRUE;
	return dst_element;
    }				/* End if (srcy > dsty) */

    /*
     * Else either srcy <= dsty && they aren't within 4*stub_length &&
     * this isn't the first time through, or !horizontal_path_clear.
     * This boils down to: we're going from top down (w/o worrying about
     * stubs), or there's no clear horizontal path.
     */
    /* create plumbs */
    do_lines = level != 1;
    ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, 
			     level == 1? source: NULL, do_lines);
    ymin = FindVerticalPlumb(ww, dstx, dsty, LOOKUP,
			     level == 1? destination: NULL, do_lines);
    if (ymax == -1 || ymin == -1)
    {
	*failnum = ymax == -1? 10: 11;
#ifdef Printit
	printf("Routing Failure%d! level = %d \n", *failnum, level);
#endif
	*cost = 999;
	return src_element;
    }

#ifdef TRACE
    printf("Lev = %d, Simple route attempt\n", level);
#endif
    /*
     * Restrict ymax and ymin to be in between the source and destination.
     * These limits include the stub-lengths.
     */
    if (ymin < srcy)
	ymin = srcy;
    if (ymax > dsty)
	ymax = dsty;

    /*
     * If there is overlap, i.e. if the plumb up from the destination
     * passes the plumb down from the source:
     */
    if (ymin < ymax)
    {
	/* CASE 6: (XXX isn't this just case 1?) */
	if (srcx == dstx)	/* Vertical Line case */
	{
	    dst_element = CreateDestinationElement(dstx, dsty, &mycost);
	    src_element->next = dst_element;
	    *cost = mycost;
	    return src_element;
	}
#ifdef TRACE
	printf("Lev = %d, Overlap in the y direction\n", level);
#endif
	/*
	 * CASE 7: two vertical lines with a cross bar.
	 *
	 * Search the overlap region for a clear horizontal line segement.
	 * Start in the center and search down first, then up.
	 * XXX Switch these two because if we search down, then over,
	 * we stand a reasonable chance of crossing a horizontal line?
	 */
	ycenter = (srcy - dsty) / 2 + dsty;
	ycenter = MAX(MIN(ycenter,ymax), ymin);
	xpos = FindHorizontalPlumbIgnoringLines(ww, srcx, ycenter, 
		dstx < srcx? LOOKLEFT: LOOKRIGHT,
		ignore_lines);
	if (xpos == -1 || xpos == srcx) 
	    inc = -1;
	else
	    inc =  1;
	for (ypos = ycenter; ypos <= ymax && ypos >= ymin; ypos += inc)
	{
	    if (!DetectYCollision(ww, ypos, srcx, dstx, &ret_line, &ret_widget))
	    {
		dst_element = CreateDestinationElement(dstx, dsty, &mycost);
		element1 = MakeLineElement(srcx, ypos);
		element2 = MakeLineElement(dstx, ypos);
		mycost += 2;
		src_element->next = element1;
		element1->next = element2;
		element2->next = dst_element;
		*cost = mycost;
		return src_element;	/* Done! return plumbs and horizontal
					 * lines */
	    }
	}
	for (ypos = ycenter; ypos <= ymax && ypos >= ymin; ypos -= inc)
	{
	    if (!DetectYCollision(ww, ypos, srcx, dstx, &ret_line, &ret_widget))
	    {
		dst_element = CreateDestinationElement(dstx, dsty, &mycost);
		element1 = MakeLineElement(srcx, ypos);
		element2 = MakeLineElement(dstx, ypos);
		mycost += 2;
		src_element->next = element1;
		element1->next = element2;
		element2->next = dst_element;
		*cost = mycost;
		return src_element;	/* Done! return plumbs and horizontal
					 * lines */
	    }
	}
    }
#ifdef TRACE
    printf("Lev = %d, No overlap\n", level);
#endif
    /*
     * The plumb up from dst and down from src do not overlap, or we could 
     * not find a horizontal line in the overlapping region.  Here, we will 
     * take two routes, and compare the relative cost, if the level 
     * is shallow enough.
     *
     * The method for choosing the two routes is to look at all of the
     * possible y positions between the source and the obstruction.  Look
     * left and right from the y position.  Save the result of each search.
     * Choose the best y postion for looking left and for looking right. When
     * looking towards the destination, the x position closest to the
     * destination x should be choosen.  When looking away from the
     * destination, the x postions just beyond the obstruction will be
     * choosen. 
     */
    element1 = MakeLineElement(srcx, 0);
    mycost += 1;
    src_element->next = element1;
    /*
     * CASE 8: two vertical lines that can't be joined by a horizontal 
     * segment.
     *
     * For the source, find a reasonable dogleg.  We start by checking to see
     * that we haven't already checked the entire range (i.e. ymax != dsty...)
     * and that we aren't too expensive already.
     */
    if ((level <= DEPTH) && (ymax != dsty))
    {
#ifdef TRACE
	printf("Lev = %d, No obstruction\n", level);
#endif
	/* Find open slots to the left and right of the current obstruction.
	 * Remember that ymax is the farthest down that the source
	 * can see at srcx (element1->x).
	 * This finds a column of unobstructed pixels starting here, down.
	 * XXX Doesn't this all assume that we can get from x*slot to srcx
	 * without a Collision?
	 */

	element2 = MakeLineElement(srcx, 0);
	left_cost = right_cost = 0;

	left_failnum = 11;
	left_line =
	    ObstructedFind(ww,
			   element1, 
			   ymax, 
			   LOOKLEFT,
			   srcx, srcy, dstx, dsty,
			   source, destination,
			   level,
			   &left_cost,
			   new, &left_failnum,
			   ignore_lines);
	right_failnum = 13;
	right_line =
	    ObstructedFind(ww,
			   element2, 
			   ymax, 
			   LOOKRIGHT,
			   srcx, srcy, dstx, dsty,
			   source, destination,
			   level,
			   &right_cost,
			   new, &right_failnum,
			   ignore_lines);

	if ((left_cost >= 999) && (right_cost >= 999))
	{
	    *failnum = 16;
#ifdef Printit
	    printf("Routing Failure16! level = %d \n", level);
#endif
	    *cost = 999;
            if (right_line) FreeLineElementList(right_line);
            if (left_line) FreeLineElementList(left_line);
	    FreeLineElementList(element2);
	    return src_element;
	}
	/*
	 * XXX Perhaps we should measure the lengths of the lines?
	 */
	if (left_cost == right_cost)
	{
	    if (MeasureLine(left_line) > MeasureLine(right_line))
	    {
		if (left_line) FreeLineElementList(left_line);
		XtFree((char*)element1);
		src_element->next = element2;
		element2->next = right_line;
		*cost = mycost + right_cost;
		return src_element;
	    }
	    else
	    {
		if (right_line) FreeLineElementList(right_line);
		XtFree((char*)element2);
		src_element->next = element1;
		element1->next = left_line;
		*cost = mycost + left_cost;
		return src_element;
	    }
	}
	else if (right_cost < left_cost)
	{
  	    if (left_line) FreeLineElementList(left_line);
	    XtFree((char*)element1);
	    src_element->next = element2;
	    element2->next = right_line;
	    *cost = mycost + right_cost;
	    return src_element;
	}
	else
	{
  	    if (right_line) FreeLineElementList(right_line);
	    XtFree((char*)element2);
	    src_element->next = element1;
	    element1->next = left_line;
	    *cost = mycost + left_cost;
	    return src_element;
	}
    }
    /*
     * CASE 8A:  Same as case 8 except that we don't try both ways.
     *
     * level>DEPTH , so if first attemp is OK, don't try the other way
     * XXX how often does this happen, and is it sufficient to not
     * even have this case?
     */
    else if (ymax != dsty)
    {
#ifdef TRACE
	printf("Lev = %d, Only look one way\n", level);
#endif
	if (srcx < dstx)
	{
	    horizontal_dir = LOOKRIGHT;
	}
	else
	{
	    horizontal_dir = LOOKLEFT;
	}
	vertical_dir = LOOKDOWN;
	if (srcx < dstx)
	{
	    right_cost = 0;
	    right_failnum = 16;
	    right_line =
		ObstructedFind(ww,
			       element1, 
			       ymax, 
			       horizontal_dir,
			       srcx, srcy, dstx, dsty,
			       source, destination,
			       level,
			       &right_cost,
			       new, &right_failnum,
			       ignore_lines);
	    element1->next = right_line;
	    *cost = mycost + right_cost;
	    return src_element;
	}
	else
	{
	    left_cost = 0;
	    left_failnum = 19;
	    left_line =
		ObstructedFind(ww,
			       element1, 
			       ymax, 
			       horizontal_dir,
			       srcx, srcy, dstx, dsty,
			       source, destination,
			       level,
			       &left_cost,
			       new, &right_failnum,
			       ignore_lines);
	    element1->next = left_line;
	    *cost = mycost + left_cost;
	    return src_element;
	}
    }

    /*
     * If we got here, it could be because there simply was no room in the
     * space to lay down a horizontal line.  We will now check if that is the
     * case, and if so, look outside the y bounds of the source and dest to
     * find room. 
     */
    if (1)
    {
#ifdef TRACE
	printf("Lev = %d, Extended dogleg\n", level);
#endif
	/*
	 * If we get here, it is because there is no horizontal path between
	 * the source and the destination.  In this case, try to construct an
	 * "extended dogleg" around the source, and then call the routine
	 * recursively to complete the route. 
	 */
	if (srcx < dstx)
	{
	    src_dogleg1.x = srcx;
	    src_dogleg1.y = srcy;
	    horizontal_dir = LOOKRIGHT;
	    vertical_dir = LOOKUP;
	    src_dogleg2.x = FindVerticalSlot(ww, srcx, srcy, horizontal_dir,
					     vertical_dir, SLOT_LEN);
	    if ((src_dogleg2.x == -1) ||
		(DetectYCollision(ww, src_dogleg1.y, src_dogleg1.x,
				  src_dogleg2.x, &ret_line, &ret_widget)))
	    {
		horizontal_dir = LOOKLEFT;
		src_dogleg2.x = FindVerticalSlot(ww, srcx, srcy,
				    horizontal_dir, vertical_dir, SLOT_LEN);
		if ((src_dogleg2.x == -1) ||
		    (DetectYCollision(ww, src_dogleg1.y, src_dogleg1.x,
				    src_dogleg2.x, &ret_line, &ret_widget)))
		{
		    *cost = 999;
		    return src_element;
		}
	    }
	    src_dogleg2.y = src_dogleg1.y;
	    if (dstx < src_dogleg2.x)
	    {
		horizontal_dir = LOOKLEFT;
	    }
	    else
	    {
		horizontal_dir = LOOKRIGHT;
	    }
	    /*
	     * Use src_dogleg3.y as a temp variable to hold the starting y
	     * value for the search for a Horizontal slot.  We will subtract
	     * off the height of the widget so we start the search at a
	     * reasonable y position.  This also insures that we do not
	     * "double back" on the horizontal segment we just create with
	     * the first two dogleg points. 
	     */
	    src_dogleg3.y = src_dogleg2.y - source->core.height;
	    src_dogleg3.y = FindHorizontalSlot(ww, src_dogleg2.x, src_dogleg3.y,
					       horizontal_dir, vertical_dir,
					       Abs(dstx - src_dogleg2.x));
	    if ((src_dogleg3.y == -1) ||
		(DetectXCollision(ww, src_dogleg2.x, src_dogleg2.y,
				  src_dogleg3.y, &ret_line, &ret_widget)))

	    {
		if (src_dogleg2.x > srcx)	/* We have only looked right,
						 * so try */
		{		/* looking left */
		    horizontal_dir = LOOKLEFT;
		    src_dogleg2.x = FindVerticalSlot(ww, srcx, srcy,
				    horizontal_dir, vertical_dir, SLOT_LEN);
		    if ((src_dogleg2.x == -1) ||
			(DetectYCollision(ww, src_dogleg1.y, src_dogleg1.x,
				    src_dogleg2.x, &ret_line, &ret_widget)))
		    {
			*cost = 999;
			return src_element;
		    }
		}
		src_dogleg2.y = src_dogleg1.y;
		if (dstx < src_dogleg2.x)
		{
		    horizontal_dir = LOOKLEFT;
		}
		else
		{
		    horizontal_dir = LOOKRIGHT;
		}
		/*
		 * Use src_dogleg3.y as a temp variable to hold the starting
		 * y value for the search for a Horizontal slot.  We will
		 * subtract off the height of the widget so we start the
		 * search at a reasonable y position.  This also insures that
		 * we do not "double back" on the horizontal segment we just
		 * create with the first two dogleg points. 
		 */
		src_dogleg3.y = src_dogleg2.y - source->core.height;
		src_dogleg3.y = FindHorizontalSlot(ww, src_dogleg2.x, src_dogleg3.y,
					       horizontal_dir, vertical_dir,
						 Abs(dstx - src_dogleg2.x));
		if ((src_dogleg3.y == -1) ||
		    (DetectXCollision(ww, src_dogleg2.x, src_dogleg2.y,
				    src_dogleg3.y, &ret_line, &ret_widget)))

		{
		    *cost = 999;
		    return src_element;
		}
	    }
	}
	else
	{
	    src_dogleg1.x = srcx;
	    src_dogleg1.y = srcy;
	    horizontal_dir = LOOKLEFT;
	    vertical_dir = LOOKUP;
	    src_dogleg2.x = FindVerticalSlot(ww, srcx, srcy, horizontal_dir,
					     vertical_dir, SLOT_LEN);
	    if ((src_dogleg2.x == -1) ||
		(DetectYCollision(ww, src_dogleg1.y, src_dogleg1.x,
				  src_dogleg2.x, &ret_line, &ret_widget)))
	    {
		horizontal_dir = LOOKRIGHT;
		src_dogleg2.x = FindVerticalSlot(ww, srcx, srcy,
				    horizontal_dir, vertical_dir, SLOT_LEN);
		if ((src_dogleg2.x == -1) ||
		    (DetectYCollision(ww, src_dogleg1.y, src_dogleg1.x,
				    src_dogleg2.x, &ret_line, &ret_widget)))
		{
		    *cost = 999;
		    return src_element;
		}
	    }
	    src_dogleg2.y = src_dogleg1.y;
	    if (dstx < src_dogleg2.x)
	    {
		horizontal_dir = LOOKLEFT;
	    }
	    else
	    {
		horizontal_dir = LOOKRIGHT;
	    }
	    /*
	     * Use src_dogleg3.y as a temp variable to hold the starting y
	     * value for the search for a Horizontal slot.  We will subtract
	     * off the height of the widget so we start the search at a
	     * reasonable y position.  This also insures that we do not
	     * "double back" on the horizontal segment we just create with
	     * the first two dogleg points. 
	     */
	    src_dogleg3.y = src_dogleg2.y - source->core.height;
	    src_dogleg3.y = FindHorizontalSlot(ww, src_dogleg2.x, src_dogleg3.y,
					       horizontal_dir, vertical_dir,
					       Abs(src_dogleg2.x - dstx));
	    if ((src_dogleg3.y == -1) ||
		(DetectXCollision(ww, src_dogleg2.x, src_dogleg2.y,
				  src_dogleg3.y, &ret_line, &ret_widget)))

	    {
		if (src_dogleg2.x < dstx)	/* We have only looked left,
						 * so try */
		{		/* looking right */
		    horizontal_dir = LOOKRIGHT;
		    src_dogleg2.x = FindVerticalSlot(ww, srcx, srcy,
				    horizontal_dir, vertical_dir, SLOT_LEN);
		    if ((src_dogleg2.x == -1) ||
			(DetectYCollision(ww, src_dogleg1.y, src_dogleg1.x,
				    src_dogleg2.x, &ret_line, &ret_widget)))
		    {
			*cost = 999;
			return src_element;
		    }
		}
		src_dogleg2.y = src_dogleg1.y;
		if (dstx < src_dogleg2.x)
		{
		    horizontal_dir = LOOKLEFT;
		}
		else
		{
		    horizontal_dir = LOOKRIGHT;
		}
		/*
		 * Use src_dogleg3.y as a temp variable to hold the starting
		 * y value for the search for a Horizontal slot.  We will
		 * subtract off the height of the widget so we start the
		 * search at a reasonable y position.  This also insures that
		 * we do not "double back" on the horizontal segment we just
		 * create with the first two dogleg points. 
		 */
		src_dogleg3.y = src_dogleg2.y - source->core.height;
		src_dogleg3.y = FindHorizontalSlot(ww, src_dogleg2.x, src_dogleg3.y,
					       horizontal_dir, vertical_dir,
						 Abs(src_dogleg2.x - dstx));
		if ((src_dogleg3.y == -1) ||
		    (DetectXCollision(ww, src_dogleg2.x, src_dogleg2.y,
				    src_dogleg3.y, &ret_line, &ret_widget)))

		{
		    *cost = 999;
		    return src_element;
		}
	    }
	}
	src_dogleg3.x = src_dogleg2.x;
	vertical_dir = LOOKDOWN;
	src_dogleg4.x = dstx;
	src_dogleg4.y = src_dogleg3.y;
	remainder_line = ManhattanWorker(ww, src_dogleg4.x, src_dogleg4.y, 
				dstx, dsty, level, &mycost, 
				source, destination, new, failnum,
				ignore_lines);
	if (new->reverse_on_copy)
	{
	    remainder_line = ReverseLine(remainder_line);
	    new->reverse_on_copy = FALSE;
	}

	if (element1) FreeLineElementList(element1);
	element1 = MakeLineElement(src_dogleg1.x, src_dogleg1.y);
	element2 = MakeLineElement(src_dogleg2.x, src_dogleg2.y);
	element3 = MakeLineElement(src_dogleg3.x, src_dogleg3.y);
	mycost += 3;
	src_element->next = element1;
	element1->next = element2;
	element2->next = element3;
	element3->next = remainder_line;
	*cost = mycost;
	return src_element;
    }

#ifdef Comment
    ymin = MIN(srcy, dsty);
    ymax = MAX(srcy, dsty);
    for (ypos = ymin; ypos <= ymax; ypos++)
    {
	if (!DetectYCollision(ww, ypos, srcx, dstx, &ret_line, &ret_widget))
	{
	    break;
	}
	else
	{
	    continue;
	}
    }
    if (ypos == ymax)
    {
	ymax = FindVerticalPlumb(ww, srcx, srcy, LOOKDOWN, NULL, TRUE);
	if (ymax > ypos)
	{
	    for (ypos = ypos; ypos <= ymax; ypos++)
	    {
		if (!DetectYCollision(ww, ypos, srcx, dstx, &ret_line, &ret_widget))
		{
		    /* Go 1/2 way to the dest */
		    xmin = MIN(srcx, dstx);
		    xpos = xmin + Abs(srcx - dstx) / 2;
		    line_center[1] = ManhattanWorker(ww, xpos, ypos, dstx,
			       dsty, level, &left_cost, source, destination,
				new, failnum, ignore_lines);
		    if (new->reverse_on_copy)
		    {
			line_center[1] = ReverseLine(line_center[1]);
			new->reverse_on_copy = FALSE;
		    }
		    if (element1) FreeLineElementList(element1);
		    element1 = MakeLineElement(srcx, ypos);
		    mycost += 1;
		    src_element->next = element1;
		    element1->next = line_center[1];
		    *cost = mycost;
		    return src_element;
		}
	    }
	}
    }
#endif
    *failnum = 23;
#ifdef Printit
    printf("Routing Failure23! level = %d \n", level);
#endif
    *cost = 999;
    return src_element;
}


/* Determine the set of lines that share the same initial and
 * terminal point with 'new'.  These lines are ignored in colision
 * detection, and candidates for the "copy_line".  Put these
 * into ignore_lines.
 */
static LineList *
FindIgnoredLines(XmWorkspaceLine new,
		 int srcx, int srcy, int dstx, int dsty,
		 Widget source, Widget destination)
{
    XmWorkspaceLine	    w_line;
    XmWorkspaceConstraints  constraints;
    LineList		    *ignore_lines, *current_ignore;

    ignore_lines = NULL;
    constraints = WORKSPACE_CONSTRAINT(destination);
    for (w_line = constraints->workspace.destination_lines;
	 w_line; w_line = w_line->dst_next)
    {
	if ((w_line->num_points > 0) &&
	    (w_line->point[w_line->num_points - 1].x == dstx) &&
	    (w_line->point[w_line->num_points - 1].y == dsty) &&
	    (w_line != new))
	{
	    /*
	     * Add this line to the list of lines to ignore in collision
	     * detection 
	     */
	    current_ignore = (LineList *) XtMalloc(sizeof(LineList));
	    if (ignore_lines)
	    {
		current_ignore->next = ignore_lines->next;
		ignore_lines->next = current_ignore;
	    }
	    else
	    {
		ignore_lines = current_ignore;
		ignore_lines->next = NULL;
	    }
	    current_ignore->line = w_line;
	}
    }
    constraints = WORKSPACE_CONSTRAINT(source);
    for (w_line = constraints->workspace.source_lines;
	 w_line; w_line = w_line->src_next)
    {
	if ((w_line->num_points > 0) &&
	    (w_line->point[0].x == srcx) &&
	    (w_line->point[0].y == srcy) &&
	    (w_line != new))
	{
	    /*
	     * Add this line to the list of lines to ignore in collision
	     * detection 
	     */
	    current_ignore = (LineList *) XtMalloc(sizeof(LineList));
	    if (ignore_lines)
	    {
		current_ignore->next = ignore_lines->next;
		ignore_lines->next = current_ignore;
	    }
	    else
	    {
		ignore_lines = current_ignore;
		ignore_lines->next = NULL;
	    }
	    current_ignore->line = w_line;
	}
    }

    return ignore_lines;
}

static void
FreeIgnoredLines(LineList *ignore_lines)
{
    LineList *tmp;
    while(ignore_lines)
    {
	tmp = ignore_lines;
	ignore_lines = ignore_lines->next;
	XtFree((char*)tmp);
    }
}
void 
FreeLineElementList(LineElement *list)
{
    while (list != NULL)
    {
      LineElement *tmp = list->next;
      XtFree((char*)list);
      list = tmp;
    }
}


static LineElement *
MakeLineElement(int x, int y)
{
    LineElement *r;
    r = (LineElement *) XtMalloc(sizeof(LineElement));
    r->next = NULL;
    r->x = x;
    r->y = y;

    return r;
}

/*
 * Routine CreateDestinationElement Purpose: Allocate Memory for the
 * Desination List Element and inc the cost 
 */
static LineElement *
CreateDestinationElement(int dstx, int dsty, int *mycost)
{
    (*mycost)++;
    return MakeLineElement(dstx, dsty);
}

/*
 * Routine FindVerticalSlot Purpose: Given a point, find the first free
 * vertical slot of length slot_len.  Return the x position of this slot. 
 * A slot is a space where there is room to draw a line.  The width of a slot
 * will be determined by the line width and some tolerance parameter that
 * control how closely lines may be placed to one another. 
 */
static int 
FindVerticalSlot(XmWorkspaceWidget ww, int x, int y,
		 int horizontal_dir, int vertical_dir, int slot_len)
{
    register int    i;
    int             yend;
    XmWorkspaceLine ret_line;
    Widget          ret_widget;

    if (vertical_dir == LOOKUP)
    {
	yend = y - slot_len;
    }
    else
    {
	yend = y + slot_len;
    }

    if (horizontal_dir == LOOKLEFT)
    {
	for (i = x; i >= 0; i--)
	{
	    if (!DetectXCollision(ww, i, y, yend, &ret_line, &ret_widget))
	    {
		return i;
	    }
	}
	return -1;		/* No Slots available in this direction */
    }
    else
    {
	for (i = x; i < ww->core.width; i++)
	{
	    if (!DetectXCollision(ww, i, y, yend, &ret_line, &ret_widget))
	    {
		return i;
	    }
	}
	return -1;		/* No Slots available in this direction */
    }
}

static int 
FindHorizontalSlot(XmWorkspaceWidget ww, int x, int y,
		   int horizontal_dir, int vertical_dir, int slot_len)
{
    register int    i;
    register int    xend;
    XmWorkspaceLine ret_line;
    Widget          ret_widget;

    if (horizontal_dir == LOOKRIGHT)
    {
	xend = x + slot_len;
    }
    else
    {
	xend = x - slot_len;
    }

    if (vertical_dir == LOOKUP)
    {
	for (i = y; i >= 0; i--)
	{
	    if (!DetectYCollision(ww, i, x, xend, &ret_line, &ret_widget))
	    {
		return i;
	    }
	}
	return -1;		/* No Slots available in this direction */
    }
    else
    {
	for (i = y; i < ww->core.height; i++)
	{
	    if (!DetectYCollision(ww, i, x, xend, &ret_line, &ret_widget))
	    {
		return i;
	    }
	}
	return -1;		/* No Slots available in this direction */
    }
}

/*
 * Subroutine: DetectYCollision Purpose:    See if this line segment collides
 * with anything 
 */
static Boolean 
DetectYCollision(XmWorkspaceWidget ww, int y, int x1, int x2,
		 XmWorkspaceLine * line, Widget * widget)
{
    register CollideList *cl_ptr;
    int             xmin, xmax;
    register int    ystart, yend, ypos;
    int             offset;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    ystart = y - offset;
    yend = y + offset;
    ystart = MAX(ystart, 0);
    yend = MIN(yend, ww->core.height - 1);

    xmin = MIN(x1, x2);
    xmax = MAX(x1, x2);
    for (ypos = ystart; ypos <= yend; ypos++)
    {
	if (ypos >= ww->core.height)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_y + ypos);
	     cl_ptr != NULL; cl_ptr = cl_ptr->next)
	{
	    if (!((cl_ptr->min > xmax) || (cl_ptr->max < xmin)))
	    {
		*line = cl_ptr->line;
		*widget = cl_ptr->child;
		return TRUE;
	    }
	}
    }
    return FALSE;
}

/*
 * Subroutine: DetectYLineCollision Purpose:    See if this line segment
 * collides with another line 
 */
static Boolean 
DetectYLineCollision(XmWorkspaceWidget ww, int y, int x1, int x2,
		     XmWorkspaceLine * line, Widget * widget)
{
    register CollideList *cl_ptr;
    int             xmin, xmax;
    int             offset;
    register int    ypos, ystart, yend;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    ystart = y - offset;
    yend = y + offset;
    ystart = MAX(ystart, 0);
    yend = MIN(yend, ww->core.height - 1);

    xmin = MIN(x1, x2);
    xmax = MAX(x1, x2);
    for (ypos = ystart; ypos <= yend; ypos++)
    {
	if (ypos >= ww->core.height)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_y + ypos);
	     cl_ptr != NULL; cl_ptr = cl_ptr->next)
	{
	    if (!((cl_ptr->min > xmax) || (cl_ptr->max < xmin)))
	    {
		if (cl_ptr->type == LINE_TYPE)
		{
		    *line = cl_ptr->line;
		    *widget = cl_ptr->child;
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*
 * Subroutine: DetectYLineCollision Purpose:    See if this line segment
 * collides with another line 
 */
static Boolean 
DetectYCollisionIgnoringLines(XmWorkspaceWidget ww, int y, int x1, int x2,
			      XmWorkspaceLine * line, Widget * widget,
			      LineList *ignored)
{
    register CollideList *cl_ptr;
    int             xmin, xmax;
    int             offset;
    register int    ypos, ystart, yend;
    int             found;
    LineList        *l;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    ystart = y - offset;
    yend = y + offset;
    ystart = MAX(ystart, 0);
    yend = MIN(yend, ww->core.height - 1);

    xmin = MIN(x1, x2);
    xmax = MAX(x1, x2);
    for (ypos = ystart; ypos <= yend; ypos++)
    {
	if (ypos >= ww->core.height)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_y + ypos);
	     cl_ptr != NULL; cl_ptr = cl_ptr->next)
	{
	    if (!((cl_ptr->min > xmax) || (cl_ptr->max < xmin)))
	    {
		found = FALSE;
		if (cl_ptr->type == LINE_TYPE)
		{
		    for (l = ignored; l; l = l->next)
		    {
			if (l->line == cl_ptr->line)
			{
			    found = TRUE;
			    break;
			}
		    }
		}
		if (!found)
		{
		    *line = cl_ptr->line;
		    *widget = cl_ptr->child;
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*
 * Subroutine: DetectXCollision Purpose:    See if this line segment collides
 * with anything 
 */
static Boolean 
DetectXCollision(XmWorkspaceWidget ww, int x, int y1, int y2,
		 XmWorkspaceLine * line, Widget * widget)
{
    register CollideList *cl_ptr;
    int             ymin, ymax;
    int             offset;
    register int    xpos, xstart, xend;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    xstart = x - offset;
    xend = x + offset;
    xstart = MAX(xstart, 0);
    xend = MIN(xend, ww->core.width - 1);

    ymin = MIN(y1, y2);
    ymax = MAX(y1, y2);

    for (xpos = xstart; xpos <= xend; xpos++)
    {
	if (xpos >= ww->core.width)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_x + xpos);
	     cl_ptr != NULL; cl_ptr = cl_ptr->next)
	{
	    if (!((cl_ptr->min > ymax) || (cl_ptr->max < ymin)))
	    {
		*line = cl_ptr->line;
		*widget = cl_ptr->child;
		return TRUE;
	    }
	}
    }
    return FALSE;
}

/*
 * Subroutine: DetectXLineCollision Purpose:    See if this line segment
 * collides with another line 
 */
static Boolean 
DetectXLineCollision(XmWorkspaceWidget ww, int x, int y1, int y2,
		     XmWorkspaceLine * line, Widget * widget)
{
    register CollideList *cl_ptr;
    int             ymin, ymax;
    int             offset;
    register int    xpos, xstart, xend;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    xstart = x - offset;
    xend = x + offset;
    xstart = MAX(xstart, 0);
    xend = MIN(xend, ww->core.width - 1);

    ymin = MIN(y1, y2);
    ymax = MAX(y1, y2);

    for (xpos = xstart; xpos <= xend; xpos++)
    {
	if (xpos >= ww->core.width)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_x + xpos);
	     cl_ptr != NULL; cl_ptr = cl_ptr->next)
	{
	    if (!((cl_ptr->min > ymax) || (cl_ptr->max < ymin)))
	    {
		if (cl_ptr->type == LINE_TYPE)
		{
		    *line = cl_ptr->line;
		    *widget = cl_ptr->child;
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*
 * Subroutine: DetectXLineCollision Purpose:    See if this line segment
 * collides with another line 
 */
static Boolean 
DetectXCollisionIgnoringLines(XmWorkspaceWidget ww, int x, int y1, int y2,
			      XmWorkspaceLine * line, Widget * widget,
			      LineList *ignored)
{
    register CollideList *cl_ptr;
    int             ymin, ymax;
    int             offset;
    register int    xpos, xstart, xend;
    int		    found;
    LineList        *l;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    xstart = x - offset;
    xend = x + offset;
    xstart = MAX(xstart, 0);
    xend = MIN(xend, ww->core.width - 1);

    ymin = MIN(y1, y2);
    ymax = MAX(y1, y2);

    for (xpos = xstart; xpos <= xend; xpos++)
    {
	if (xpos >= ww->core.width)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_x + xpos);
	     cl_ptr != NULL; cl_ptr = cl_ptr->next)
	{
	    if (!((cl_ptr->min > ymax) || (cl_ptr->max < ymin)))
	    {
		found = FALSE;
		if (cl_ptr->type == LINE_TYPE)
		{
		    for (l = ignored; l; l = l->next)
		    {
			if (l->line == cl_ptr->line)
			{
			    found = TRUE;
			    break;
			}
		    }
		}
		if (!found)
		{
		    *line = cl_ptr->line;
		    *widget = cl_ptr->child;
		    return TRUE;
		}
	    }
	}
    }
    return FALSE;
}

/*
 * Subroutine: FindVerticalPlumb Purpose:    Take a plumb from the given
 * point (srcx, srcy) in the indicated direction (Up or Down) and return the
 * y position of the last free point.  do_lines indicates whether to ignore
 * lines that share the same srcx & srcy. 
 */

static int 
FindVerticalPlumb(XmWorkspaceWidget ww, int srcx, int srcy,
		  int direction, Widget widget, Boolean do_lines)
{
    register CollideList *cl_ptr;
    int             y;
    register int    xstart, xend, xpos;
    int             offset;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    xstart = srcx - offset;
    xend = srcx + offset;
    xstart = MAX(xstart, 0);
    xend = MIN(xend, ww->core.width - 1);

    if (direction == LOOKUP)
    {
	y = 0;
    }
    else
    {
	y = ww->core.height - 1;
    }
    for (xpos = xstart; xpos <= xend; xpos++)
    {
	if (xpos >= ww->core.width)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_x + xpos); cl_ptr != NULL;
	     cl_ptr = cl_ptr->next)
	{
	    if (direction == LOOKUP)
	    {
		if ((do_lines && (cl_ptr->type == LINE_TYPE)) ||
		    (cl_ptr->type == WIDGET_TYPE) ||
		    (!do_lines && ((cl_ptr->line->src_x != srcx) ||
				   (cl_ptr->line->src_y != srcy))))
		{
		    if ((y < cl_ptr->max) && (srcy > cl_ptr->max) &&
			(   cl_ptr->type == LINE_TYPE ||
			    (widget != cl_ptr->child)))
		    {
			y = cl_ptr->max + 1;
		    }
		}
		/*
		 * If the source point is in a widget or line, there is no
		 * plumb 
		 */
		/* Also, check to see if the widget that it is in is O.K. */
		if ((srcy > cl_ptr->min) && (srcy < cl_ptr->max) &&
		    (widget != NULL) && (widget != cl_ptr->child))
		{
		    return -1;
		}
	    }
	    else
	    {
		if ((do_lines && (cl_ptr->type == LINE_TYPE)) ||
		    (cl_ptr->type == WIDGET_TYPE) ||
		    (!do_lines && ((cl_ptr->line->src_x != srcx) ||
				   (cl_ptr->line->src_y != srcy))))
		{
		    if ((y > cl_ptr->min) && (srcy < cl_ptr->min) &&
			(   cl_ptr->type == LINE_TYPE ||
			    (widget != cl_ptr->child)))
		    {
			y = cl_ptr->min - 1;
		    }
		}
		/*
		 * If the source point is in a widget or line, there is no
		 * plumb 
		 */
		/* Also, check to see if the widget that it is in is O.K. */
		if ((srcy > cl_ptr->min) && (srcy < cl_ptr->max) &&
		    (widget != NULL) && (widget != cl_ptr->child))
		{
		    return -1;
		}
	    }
	}
    }
    return y;
}

/*
 * Subroutine: FindHorizontalPlumbIgnoringLines Purpose:    Find a horizontal
 * Plumb (clear channel), but ignore any collisions with the line that has
 * been passed in.  
 */
static int 
FindHorizontalPlumbIgnoringLines(XmWorkspaceWidget ww, int srcx,
			   int srcy, int direction, LineList *ignore_lines)
{
    register CollideList *cl_ptr;
    int             x, ypos;
    int             offset, ystart, yend;
    register LineList *current_ignore;
    Boolean         ignore;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    ystart = srcy - offset;
    yend = srcy + offset;
    ystart = MAX(ystart, 0);
    yend = MIN(yend, ww->core.height - 1);

    /*
     * Initialize x, the value to be returned.  We assume an unobstructed
     * plumb, and trim it down as we encounter obstructions. 
     */
    if (direction == LOOKLEFT)
    {
	x = 0;
    }
    else
    {
	x = ww->core.width - 1;
    }
    for (ypos = ystart; ypos <= yend; ypos++)
    {
	if (ypos >= ww->core.height)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_y + ypos); cl_ptr != NULL;
	     cl_ptr = cl_ptr->next)
	{
	    /*
	     * Compare this line in the collision list with the list of lines
	     * that we are to ignore.  If it matches, break out of loop 
	     */
	    ignore = FALSE;
	    for (current_ignore = ignore_lines; current_ignore != NULL;
		 current_ignore = current_ignore->next)
	    {
		if (current_ignore->line == cl_ptr->line)
		{
		    ignore = TRUE;
		    break;
		}
	    }
	    if (!ignore)
	    {
		if (direction == LOOKLEFT)
		{
		    if ((x < cl_ptr->max) && (srcx > cl_ptr->max))
		    {
			x = cl_ptr->max + 1;
		    }
		    /*
		     * If the source point is in a widget or line, there's no
		     * plumb 
		     */
		    if ((srcx >= cl_ptr->min) && (srcx <= cl_ptr->max))
		    {
			return -1;
		    }
		}
		else
		{
		    if ((x > cl_ptr->min) && (srcx < cl_ptr->min))
		    {
			x = cl_ptr->min - 1;
		    }
		    /*
		     * If the source point is in a widget or line, there's no
		     * plumb 
		     */
		    if ((srcx >= cl_ptr->min) && (srcx <= cl_ptr->max))
		    {
			return -1;
		    }
		}
	    }
	}
    }
    return x;
}

#if 0

static int 
FindHorizontalPlumb(XmWorkspaceWidget ww, int srcx, int srcy,
		    int direction)
{
    register CollideList *cl_ptr;
    register int    x, ypos;
    XmWorkspaceLine ret_line;
    Widget          ret_widget;
    int             offset, ystart, yend;

    offset = ww->workspace.ltol + ww->workspace.line_thickness - 1;
    ystart = srcy - offset;
    yend = srcy + offset;
    ystart = MAX(ystart, 0);
    yend = MIN(yend, ww->core.height - 1);

    /*
     * Initialize x, the value to be returned.  We assume an unobstructed
     * plumb, and trim it down as we encounter obstructions. 
     */
    if (direction == LOOKLEFT)
    {
	x = 0;
    }
    else
    {
	x = ww->core.width - 1;
    }
    for (ypos = ystart; ypos <= yend; ypos++)
    {
	if (ypos >= ww->core.height)
	    continue;
	for (cl_ptr = *(ww->workspace.collide_list_y + ypos); cl_ptr != NULL;
	     cl_ptr = cl_ptr->next)
	{
	    if (direction == LOOKLEFT)
	    {
		if ((x < cl_ptr->max) && (srcx > cl_ptr->max))
		{
		    x = cl_ptr->max + 1;
		}
		/*
		 * If the source point is in a widget or line, there is no
		 * plumb 
		 */
		if ((srcx >= cl_ptr->min) && (srcx <= cl_ptr->max))
		{
		    return -1;
		}
	    }
	    else
	    {
		if ((x > cl_ptr->min) && (srcx < cl_ptr->min))
		{
		    x = cl_ptr->min - 1;
		}
		/*
		 * If the source point is in a widget or line, there is no
		 * plumb 
		 */
		if ((srcx >= cl_ptr->min) && (srcx <= cl_ptr->max))
		{
		    return -1;
		}
	    }
	}
    }
    if (DetectYCollision(ww, srcy, x, srcx, &ret_line, &ret_widget))
    {
	return -1;
    }
    return x;
}
#endif

/*
 * Subroutine: AddWidgetToCollideList Purpose:    Add the effects of this
 * widget to the collide list See if it hits any existing lines, and set them
 * to be redrawn 
 */
void 
AddWidgetToCollideList(Widget child)
{
    XmWorkspaceWidget ww;
    int             x, y;
    int             ymin, ymax, xmin, xmax;
    register MyWidgetList *wl_ptr;
    int             wtol;

    ww = (XmWorkspaceWidget) XtParent(child);
    wtol = ww->workspace.wtol;

    /*
     * Calculate the extents of the widget, adding in wtol to bound lines
     * from getting too close.  Clip the values to be within the workspace 
     */
    ymin = child->core.y - wtol;
    ymin = MAX(ymin, 0);
    ymax = ymin + child->core.height + wtol;
    ymax = MIN(ymax, ww->core.height);
    xmin = child->core.x - wtol;
    xmin = MAX(xmin, 0);
    xmax = xmin + child->core.width + wtol;
    xmax = MIN(xmax, ww->core.width);
    /* Add this child widget to the workspaces private widget list */
    wl_ptr = (MyWidgetList *) XtMalloc(sizeof(MyWidgetList));
    wl_ptr->xmin = xmin;
    wl_ptr->xmax = xmax;
    wl_ptr->ymin = ymin;
    wl_ptr->ymax = ymax;
    wl_ptr->w = child;
    wl_ptr->next = ww->workspace.widget_list;
    ww->workspace.widget_list = wl_ptr;

    /* Add the entries to the collision lists */
    for (x = xmin; x <= xmax; x++)
    {
	InsertXCollideList(ww, x, ymin, ymax, child, NULL, WIDGET_TYPE);
    }
    for (y = ymin; y <= ymax; y++)
    {
	InsertYCollideList(ww, y, xmin, xmax, child, NULL, WIDGET_TYPE);
    }
    WidgetCollide(ww, child);
}

/*
 * Subroutine: DeleteWidgetFromCollideList Purpose:    Remove the effects of
 * this widget to the collide list 
 */
void 
DeleteWidgetFromCollideList(Widget child)
{
    XmWorkspaceWidget ww;
    register int    x, y;
    register MyWidgetList *wl_ptr, *back_ptr;

    ww = (XmWorkspaceWidget) XtParent(child);
    /*
     * Nothing to remove
     */
    if (!ww->workspace.widget_list) return;
    wl_ptr = ww->workspace.widget_list;
    back_ptr = NULL;
    while (child != wl_ptr->w)/* Find the widget in the list */
    {
	back_ptr = wl_ptr;
	wl_ptr = wl_ptr->next;
	/*
	 * If we did not find it, so return w/out doing anything. 
	 */
	if (wl_ptr == NULL)
	    return;
    }
    /* Remove the X's */
    for (x = wl_ptr->xmin; x <= wl_ptr->xmax; x++)
    {
	RemoveXWidgetCollideList(ww, x, wl_ptr->ymin, wl_ptr->ymax,
				 wl_ptr->w);
    }
    /* Remove the Y's */
    for (y = wl_ptr->ymin; y <= wl_ptr->ymax; y++)
    {
	RemoveYWidgetCollideList(ww, y, wl_ptr->xmin, wl_ptr->xmax,
				 wl_ptr->w);
    }
    if (wl_ptr == ww->workspace.widget_list)
    {
	ww->workspace.widget_list = wl_ptr->next;
    }
    else
    {
	back_ptr->next = wl_ptr->next;
    }
    XtFree((char*)wl_ptr);
}

/*
 * Subroutine: ChangeWidgetInCollideList Purpose:    Change the effects of
 * this widget to the collide list See if it hits any existing lines, and set
 * them to be redrawn 
 */
void 
ChangeWidgetInCollideList(XmWorkspaceWidget ww, Widget child)
{
    register int    x, y;
    register MyWidgetList *wl_ptr;
    int             wtol;

    wl_ptr = ww->workspace.widget_list;
    if(!wl_ptr) return;
    while (child != wl_ptr->w)	/* Find the widget in the list */
    {
	wl_ptr = wl_ptr->next;
	if(!wl_ptr) return;
    }
    for (x = wl_ptr->xmin; x <= wl_ptr->xmax; x++)	/* Remove the X's */
    {
	RemoveXCollideList(ww, x, wl_ptr->ymin, wl_ptr->ymax);
    }
    for (y = wl_ptr->ymin; y <= wl_ptr->ymax; y++)	/* Remove the Y's */
    {
	RemoveYCollideList(ww, y, wl_ptr->xmin, wl_ptr->xmax);
    }
    /* Add this child widget to the workspaces private widget list */
    wtol = ww->workspace.wtol;
    wl_ptr->xmin = child->core.x - wtol;
    wl_ptr->xmin = MAX(wl_ptr->xmin, 0);
    wl_ptr->xmax = wl_ptr->xmin + child->core.width + wtol;
    wl_ptr->xmax = MIN(wl_ptr->xmax, ww->core.width);
    wl_ptr->ymin = child->core.y - wtol;
    wl_ptr->ymin = MAX(wl_ptr->ymin, 0);
    wl_ptr->ymax = wl_ptr->ymin + child->core.height + wtol;
    wl_ptr->ymax = MIN(wl_ptr->ymax, ww->core.height);

    /* Add the entries to the collision lists */
    for (x = wl_ptr->xmin; x <= wl_ptr->xmax; x++)
    {
	InsertXCollideList(ww, x, wl_ptr->ymin, wl_ptr->ymax, child,
			   NULL, WIDGET_TYPE);
    }
    for (y = wl_ptr->ymin; y <= wl_ptr->ymax; y++)
    {
	InsertYCollideList(ww, y, wl_ptr->xmin, wl_ptr->xmax, child,
			   NULL, WIDGET_TYPE);
    }
    WidgetCollide(ww, child);
}

/*
 * Subroutine: HideWidgetInCollideList Purpose:    Change the effects of this
 * widget to the collide list See if it hits any existing lines, and set them
 * to be redrawn 
 */
void 
HideWidgetInCollideList(XmWorkspaceWidget ww, Widget child)
{
    register int    x, y;
    register MyWidgetList *wl_ptr;

    wl_ptr = ww->workspace.widget_list;
    if(!wl_ptr) return;
    while (child != wl_ptr->w)	/* Find the widget in the list */
    {
	wl_ptr = wl_ptr->next;
	if(!wl_ptr) return;
    }
    for (x = wl_ptr->xmin; x <= wl_ptr->xmax; x++)	/* Remove the X's */
    {
	RemoveXCollideList(ww, x, wl_ptr->ymin, wl_ptr->ymax);
    }
    for (y = wl_ptr->ymin; y <= wl_ptr->ymax; y++)	/* Remove the Y's */
    {
	RemoveYCollideList(ww, y, wl_ptr->xmin, wl_ptr->xmax);
    }
    /* Add this child widget to the workspaces private widget list */
    /*wtol = ww->workspace.wtol;*/
    wl_ptr->xmin = 0;
    wl_ptr->xmax = 0;
    wl_ptr->ymin = 0;
    wl_ptr->ymax = 0;

    /* Add the entries to the collision lists */
    for (x = wl_ptr->xmin; x <= wl_ptr->xmax; x++)
    {
	InsertXCollideList(ww, x, wl_ptr->ymin, wl_ptr->ymax, child,
			   NULL, WIDGET_TYPE);
    }
    for (y = wl_ptr->ymin; y <= wl_ptr->ymax; y++)
    {
	InsertYCollideList(ww, y, wl_ptr->xmin, wl_ptr->xmax, child,
			   NULL, WIDGET_TYPE);
    }
}

/*
 * Subroutine: WidgetCollide Purpose:    See if this widget hits any existing
 * lines, and redraw them if necessary 
 */
void 
WidgetCollide(XmWorkspaceWidget ww, Widget child)
{
    int             wtol, xmin, xmax, ymin, ymax;
    XmWorkspaceLine ret_line;
    Widget          ret_widget;
    register int    x, y;

    if (ww->workspace.lines == NULL)
    {
	return;
    }
    wtol = ww->workspace.wtol;
    xmin = child->core.x - wtol;
    xmin = MAX(xmin, 0);
    xmax = xmin + child->core.width + wtol;
    xmax = MIN(xmax, ww->core.width);
    ymin = child->core.y - wtol;
    ymin = MAX(ymin, 0);
    ymax = ymin + child->core.height + wtol;
    ymax = MIN(ymax, ww->core.height);

    for (x = xmin; x <= xmax; x++)
    {
	if (DetectXLineCollision(ww, x, ymin, ymax, &ret_line, &ret_widget))
	{
	    if (ret_line)
	    {
		ret_line->is_to_be_moved = TRUE;
		AugmentExposureAreaForLine(ww, ret_line);
	    }
	}
    }
    for (y = ymin; y <= ymax; y++)
    {
	if (DetectYLineCollision(ww, y, xmin, xmax, &ret_line, &ret_widget))
	{
	    if (ret_line)
	    {
		ret_line->is_to_be_moved = TRUE;
		AugmentExposureAreaForLine(ww, ret_line);
	    }
	}
    }
}
/*
 * Subroutine: AddSegmentToCollideList Purpose:    Add a line segment to the
 * collide list Note: Segment numbers range from 1 to line-num_points - 1 
 */
static void 
AddSegmentToCollideList(XmWorkspaceWidget ww,
			XmWorkspaceLine line,
			int seg_num)
{
    register int    ymin, ymax, xmin, xmax;

    if (line->point[seg_num - 1].x == line->point[seg_num].x)
    {
	ymin = MIN(line->point[seg_num - 1].y, line->point[seg_num].y);
	ymin = MAX(ymin, 0);
	ymax = MAX(line->point[seg_num - 1].y, line->point[seg_num].y);
	ymax = MIN(ymax, ww->core.height);
	if ((line->point[seg_num].x >= 0) &&
	    (line->point[seg_num].x < ww->core.width))
	{
	    InsertXCollideList(ww, line->point[seg_num].x, ymin, ymax, NULL,
			       line, LINE_TYPE);
	}
    }
    else			/* This line is horizontal */
    {
	xmin = MIN(line->point[seg_num - 1].x, line->point[seg_num].x);
	xmin = MAX(xmin, 0);
	xmax = MAX(line->point[seg_num - 1].x, line->point[seg_num].x);
	xmax = MIN(xmax, ww->core.width);
	if ((line->point[seg_num].y >= 0) &&
	    (line->point[seg_num].y < ww->core.height))
	{
	    InsertYCollideList(ww, line->point[seg_num].y, xmin, xmax, NULL,
			       line, LINE_TYPE);
	}
    }
}
/*
 * Subroutine: RemoveLineFromCollideList Purpose:    Remove the effects of
 * the entire line on the collide list 
 */
void 
RemoveLineFromCollideList(XmWorkspaceWidget ww, XmWorkspaceLine line)
{
    register int    i;

    for (i = 0; i < line->num_points - 1; i++)
    {
	RemoveSegmentFromCollideList(ww, line, i + 1);
    }
}

/*
 * Subroutine: AddLineToCollideList Purpose:    Add a line to the collide
 * list 
 */
void 
AddLineToCollideList(XmWorkspaceWidget ww, XmWorkspaceLine line)
{
    register int    i;

    for (i = 0; i < line->num_points - 1; i++)
    {
	AddSegmentToCollideList(ww, line, i + 1);
    }
}
/*
 * Subroutine: RemoveSegmentFromCollideList Purpose:    Remove a line segment
 * from the collide list Note: Segment numbers range from 1 to
 * line-num_points - 1 
 */
static void 
RemoveSegmentFromCollideList(XmWorkspaceWidget ww,
			     XmWorkspaceLine line,
			     int seg_num)
{
    register int    ymin, ymax, xmin, xmax;

    if (line->point[seg_num - 1].x == line->point[seg_num].x)
    {
	ymin = MIN(line->point[seg_num - 1].y, line->point[seg_num].y);
	ymin = MAX(ymin, 0);
	ymax = MAX(line->point[seg_num - 1].y, line->point[seg_num].y);
	ymax = MIN(ymax, ww->core.height);
	if ((line->point[seg_num].x >= 0) &&
	    (line->point[seg_num].x < ww->core.width))
	{
	    RemoveXCollideList(ww, line->point[seg_num].x, ymin, ymax);
	}
    }
    else			/* This line is horizontal */
    {
	xmin = MIN(line->point[seg_num - 1].x, line->point[seg_num].x);
	xmin = MAX(xmin, 0);
	xmax = MAX(line->point[seg_num - 1].x, line->point[seg_num].x);
	xmax = MIN(xmax, ww->core.width);
	if ((line->point[seg_num].y >= 0) &&
	    (line->point[seg_num].y < ww->core.height))
	{
	    RemoveYCollideList(ww, line->point[seg_num].y, xmin, xmax);
	}
    }
}

/*
 * Subroutine: InsertXCollideList Purpose:    Add an entry to the collide
 * list 
 */
static void 
InsertXCollideList(XmWorkspaceWidget ww, int x, int ymin,
		   int ymax, Widget child, XmWorkspaceLine line, int type)
{
    register CollideList *cl_ptr;

    if (x >= ww->core.width)
	return;
    /*
     * added this test 4/13/95.  I am getting core dump at the start
     * of the for loop, because collide_width < core.width.  Solution?
     * (x>=ww->core.width) implies (ww->core.width>=collide_width)
     * which means that calling ReallocCollideLists will do something.
     */
    if (x >= ww->workspace.collide_width)
        ReallocCollideLists (ww);

    cl_ptr = (CollideList *) XtMalloc(sizeof(CollideList));
    cl_ptr->min = ymin;
    cl_ptr->max = ymax;
    cl_ptr->child = child;
    cl_ptr->line = line;
    cl_ptr->type = type;
    cl_ptr->next = *(ww->workspace.collide_list_x + x);
    *(ww->workspace.collide_list_x + x) = cl_ptr;
}

/*
 * Subroutine: InsertYCollideList Purpose:    Add an entry to the collide
 * list 
 */
static void 
InsertYCollideList(XmWorkspaceWidget ww, int y, int xmin,
		   int xmax, Widget child, XmWorkspaceLine line, int type)
{
    register CollideList *cl_ptr;

    if (y >= ww->core.height)
	return;
    /*
     * added this test 4/13/95.  I am getting core dump at the start
     * of the for loop, because collide_width < core.width.  Solution?
     * (x>=ww->core.width) implies (ww->core.width>=collide_width)
     * which means that calling ReallocCollideLists will do something.
     */
    if (y >= ww->workspace.collide_height)
        ReallocCollideLists (ww);

    cl_ptr = (CollideList *) XtMalloc(sizeof(CollideList));
    cl_ptr->min = xmin;
    cl_ptr->max = xmax;
    cl_ptr->child = child;
    cl_ptr->line = line;
    cl_ptr->type = type;
    cl_ptr->next = *(ww->workspace.collide_list_y + y);
    *(ww->workspace.collide_list_y + y) = cl_ptr;
}

/*
 * Subroutine: RemoveXCollideList Purpose:    Remove an entry to the collide
 * list 
 */
static void 
RemoveXCollideList(XmWorkspaceWidget ww, int x, int min, int max)
{
    register CollideList *back_ptr=NULL, *tmp_ptr;

    if (x >= ww->core.width)
	return;

    /*
     * added this test 4/13/95.  I am getting core dump at the start
     * of the for loop, because collide_width < core.width.  Solution?
     * (x>=ww->core.width) implies (ww->core.width>=collide_width)
     * which means that calling ReallocCollideLists will do something.
     */
    if (x >= ww->workspace.collide_width) 
	ReallocCollideLists (ww);

    for (tmp_ptr = *(ww->workspace.collide_list_x + x); tmp_ptr != NULL;
	 tmp_ptr = tmp_ptr->next)
    {
	if ((min == tmp_ptr->min) && (max == tmp_ptr->max))
	{
	    if (tmp_ptr == *(ww->workspace.collide_list_x + x))
	    {
		*(ww->workspace.collide_list_x + x) = tmp_ptr->next;
	    }
	    else
	    {
		back_ptr->next = tmp_ptr->next;
	    }
	    XtFree((char*)tmp_ptr);
	    break;
	}
	back_ptr = tmp_ptr;
    }
}

/*
 * Subroutine: RemoveXWidgetCollideList Purpose:    Remove an entry to the
 * collide list 
 */
static void 
RemoveXWidgetCollideList(XmWorkspaceWidget ww, int x,
			 int min, int max, Widget widget)
{
    register CollideList *back_ptr=NULL, *tmp_ptr;

    if (x >= ww->core.width)
	return;
    /*
     * added this test 4/13/95.  I am getting core dump at the start
     * of the for loop, because collide_width < core.width.  Solution?
     * (x>=ww->core.width) implies (ww->core.width>=collide_width)
     * which means that calling ReallocCollideLists will do something.
     */
    if (x >= ww->workspace.collide_width) 
	ReallocCollideLists (ww);

    for (tmp_ptr = *(ww->workspace.collide_list_x + x); tmp_ptr != NULL;
	 tmp_ptr = tmp_ptr->next)
    {
	if ((min == tmp_ptr->min) && (max == tmp_ptr->max) &&
	    (widget == tmp_ptr->child))
	{
	    if (tmp_ptr == *(ww->workspace.collide_list_x + x))
	    {
		*(ww->workspace.collide_list_x + x) = tmp_ptr->next;
	    }
	    else
	    {
		back_ptr->next = tmp_ptr->next;
	    }
	    XtFree((char*)tmp_ptr);
	    break;
	}
	back_ptr = tmp_ptr;
    }
}

/*
 * Subroutine: RemoveYCollideList Purpose:    Remove an entry to the collide
 * list 
 */
static void 
RemoveYCollideList(XmWorkspaceWidget ww, int y, int min, int max)
{
    register CollideList *back_ptr=NULL, *tmp_ptr;

    if (y >= ww->core.height)
	return;
    /*
     * added this test 4/13/95.  I am getting core dump at the start
     * of the for loop, because collide_width < core.width.  Solution?
     * (x>=ww->core.width) implies (ww->core.width>=collide_width)
     * which means that calling ReallocCollideLists will do something.
     */
    if (y >= ww->workspace.collide_height) 
	ReallocCollideLists (ww);

    for (tmp_ptr = *(ww->workspace.collide_list_y + y); tmp_ptr != NULL;
	 tmp_ptr = tmp_ptr->next)
    {
	if ((min == tmp_ptr->min) && (max == tmp_ptr->max))
	{
	    if (tmp_ptr == *(ww->workspace.collide_list_y + y))
	    {
		*(ww->workspace.collide_list_y + y) = tmp_ptr->next;
	    }
	    else
	    {
		back_ptr->next = tmp_ptr->next;
	    }
	    XtFree((char*)tmp_ptr);
	    break;
	}
	back_ptr = tmp_ptr;
    }
}

/*
 * Subroutine: RemoveYWidgetCollideList Purpose:    Remove an entry to the
 * collide list 
 */
static void 
RemoveYWidgetCollideList(XmWorkspaceWidget ww, int y,
			 int min, int max, Widget widget)
{
    register CollideList *back_ptr=NULL, *tmp_ptr;

    if (y >= ww->core.height)
	return;
    /*
     * added this text 4/13/95.  I am getting core dump at the start
     * of the for loop, because collide_width < core.width.  Solution?
     * (x>=ww->core.width) implies (ww->core.width>=collide_width)
     * which means that calling ReallocCollideLists will do something.
     */
    if (y >= ww->workspace.collide_height) 
	ReallocCollideLists (ww);

    for (tmp_ptr = *(ww->workspace.collide_list_y + y); tmp_ptr != NULL;
	 tmp_ptr = tmp_ptr->next)
    {
	if ((min == tmp_ptr->min) && (max == tmp_ptr->max) &&
	    (widget == tmp_ptr->child))
	{
	    if (tmp_ptr == *(ww->workspace.collide_list_y + y))
	    {
		*(ww->workspace.collide_list_y + y) = tmp_ptr->next;
	    }
	    else
	    {
		back_ptr->next = tmp_ptr->next;
	    }
	    XtFree((char*)tmp_ptr);
	    break;
	}
	back_ptr = tmp_ptr;
    }
}

void 
CopyPointsToLine(XmWorkspaceWidget ww, LineElement * list,
		 XmWorkspaceLine line)
{
    register LineElement *tmp;
    static Boolean  collapse = TRUE;
    int             count;

    if (collapse && line->is_to_be_collapsed)
	CollapseList(ww, list, NULL);
    if (list == NULL)
	return;

    if (line->reverse_on_copy)
    {
#define FIX_REVERSE 1 
#if FIX_REVERSE 
	int npts;
#endif
	/* First, see how many items are in the list */
	line->num_points = 0;
	tmp = list;
	while (tmp->next != NULL)
	{
	    line->num_points++;
	    tmp = tmp->next;
	}
	line->num_points++;
#if FIX_REVERSE
	npts = (line->num_points > 50 ? 50 : line->num_points); 
#endif
	/* line->num_pts is now correct */
	count = 1;
	while (list->next != NULL)
	{
#if FIX_REVERSE
	    if (count < npts)
#else
	    if (line->num_points < 50)	
#endif
	    {
#if FIX_REVERSE
		line->point[npts - count].x = list->x;
		line->point[npts - count].y = list->y;
#else
		line->point[line->num_points - count].x = list->x;
		line->point[line->num_points - count].y = list->y;
#endif
		count++;
	    }
	    list = list->next;
	}
#if FIX_REVERSE
	/* npts should == count at this point */
	line->point[npts - count].x = list->x;
	line->point[npts - count].y = list->y;
#else
	line->point[line->num_points - count].x = list->x;
	line->point[line->num_points - count].y = list->y;
#endif
    }
    else
    {
	line->num_points = 0;
	while (list->next != NULL)
	{
	    if (line->num_points < (50 - 1))
	    {
		line->point[line->num_points].x = list->x;
		line->point[line->num_points].y = list->y;
		line->num_points++;
	    }
	    list = list->next;
	}
	line->point[line->num_points].x = list->x;
	line->point[line->num_points].y = list->y;
	line->num_points++;
    }
}

/*
 * Subroutine: CollapseList Purpose:    Remove unneeded points from the list 
 */
static void 
CollapseList(XmWorkspaceWidget ww, LineElement * list, LineList *ignored)
{
    register LineElement *cur_ptr, *element1, *element2, *element3;
    LineElement     ret_line;
    Widget          ret_widget;

    cur_ptr = list;
    while (cur_ptr != NULL)
    {
	if ((cur_ptr->next != NULL) && (cur_ptr->next->next != NULL))
	{
	    element1 = cur_ptr;
	    element2 = cur_ptr->next;
	    element3 = cur_ptr->next->next;
	    if ((element1->x == element2->x) && (element1->x == element3->x))
	    {
		element1->next = element3;
		XtFree((char*)element2);
		continue;
	    }
	    if ((element1->y == element2->y) && (element1->y == element3->y))
	    {
		element1->next = element3;
		XtFree((char*)element2);
		continue;
	    }
	    if (element1 != list && element1->x == element2->x)
	    {
		if (!DetectYCollisionIgnoringLines(ww, 
				   element1->y, element1->x, element3->x,
				   (XmWorkspaceLine *)&ret_line, 
				   (Widget *)&ret_widget, ignored) &&
		    !DetectXCollisionIgnoringLines(ww, 
				   element3->x, element1->y, element3->y,
				   (XmWorkspaceLine *)&ret_line, 
				   (Widget *)&ret_widget, ignored))
		{
		    element1->next = element3;
		    element1->x = element3->x;
		    XtFree((char*)element2);
		    continue;
		}
	    }
	    else if (element1 != list)
	    {
		if (!DetectXCollisionIgnoringLines(ww,
				   element1->x, element1->y, element3->y,
				   (XmWorkspaceLine *)&ret_line, 
				   (Widget *)&ret_widget, ignored) &&
		    !DetectYCollisionIgnoringLines(ww,
				   element3->y, element1->x, element3->x,
				   (XmWorkspaceLine *)&ret_line, 
				   (Widget *)&ret_widget, ignored))
		{
		    element1->next = element3;
		    element1->y = element3->y;
		    XtFree((char*)element2);
		    continue;
		}
	    }
	}
	cur_ptr = cur_ptr->next;
    }
}

/*
 * Subroutine: MarkCommonLines Purpose:    Look through the entire list of
 * workspace lines.  If a line is to be moved (re-routed) go through the
 * remainder of the list, find all lines that share its source or
 * destination and mark them to be re-routed. 
 */

void 
MarkCommonLines(XmWorkspaceWidget ww)
{
    XmWorkspaceLineRec *original_line=NULL, *line1, *line2, *prev_line1, *prev_line2, *tmp_line;
    int             min_disty, cur_disty;
    int             min_distx, cur_distx;

    if ((original_line = ww->workspace.lines) != NULL)
    {
	line1 = original_line;
	prev_line1 = NULL;
	while (line1)
	{
	    /*
	     * If line 1 is to be moved, search the rest of the list for
	     * common lines, and mark them to be moved 
	     */
	    min_disty = Abs(line1->dst_y - line1->src_y);
	    min_distx = Abs(line1->dst_x - line1->src_x);
	    line2 = line1->next;
	    prev_line2 = line1;
	    if (line1->is_to_be_moved)
	    {
		while (line2)
		{
		    if (((line1->src_x == line2->src_x) &&
			 (line1->src_y == line2->src_y)) ||
			((line1->dst_x == line2->dst_x) &&
			 (line1->dst_y == line2->dst_y)))
		    {

			line2->is_to_be_moved = TRUE;
			AugmentExposureAreaForLine(ww, line2);
			cur_disty = Abs(line2->src_y - line2->dst_y);
			cur_distx = Abs(line2->dst_x - line2->src_x);
			if (cur_disty < min_disty)
			{
			    min_disty = cur_disty;
			    min_distx = cur_distx;
			    SwapEntries(ww, line1, line2, prev_line1, prev_line2);
			    tmp_line = line1;
			    line1 = line2;
			    line2 = tmp_line;
			}
			if (cur_disty == min_disty)
			{
			    if (cur_distx < min_distx)
			    {
				min_disty = cur_disty;
				min_distx = cur_distx;
				SwapEntries(ww, line1, line2, prev_line1, prev_line2);
				tmp_line = line1;
				line1 = line2;
				line2 = tmp_line;
			    }
			}
		    }
		    prev_line2 = line2;
		    line2 = line2->next;
		}
	    }
	    else
		/*
		 * line 1 is NOT to be moved, so look through the rest of the
		 * list for a common line, and if one is found, mark line 1
		 * to be moved 
		 */
	    {
		while (line2)
		{
		    if (((line1->src_x == line2->src_x) &&
			 (line1->src_y == line2->src_y)) ||
			((line1->dst_x == line2->dst_x) &&
			 (line1->dst_y == line2->dst_y)))
		    {
			if (line2->is_to_be_moved)
			{
			    line1->is_to_be_moved = TRUE;
			    AugmentExposureAreaForLine(ww, line1);
			    cur_disty = Abs(line2->src_y - line2->dst_y);
			    cur_distx = Abs(line2->src_x - line2->dst_x);
			    if (cur_disty < min_disty)
			    {
				min_disty = cur_disty;
				min_distx = cur_distx;
				SwapEntries(ww, line1, line2, prev_line1, prev_line2);
				tmp_line = line1;
				line1 = line2;
				line2 = tmp_line;
				/* Expire rest of list */
				/*
				 * while (line2->next) line2 = line2->next; 
				 */
			    }
			    if (cur_disty == min_disty)
			    {
				if (cur_distx < min_distx)
				{
				    min_disty = cur_disty;
				    min_distx = cur_distx;
				    SwapEntries(ww, line1, line2, prev_line1, prev_line2);
				    tmp_line = line1;
				    line1 = line2;
				    line2 = tmp_line;
				    /* Expire rest of list */
				    /*
				     * while (line2->next) line2 =
				     * line2->next; 
				     */
				}
			    }
			}
		    }
		    prev_line2 = line2;
		    line2 = line2->next;
		}
	    }
	    prev_line1 = line1;
	    line1 = line1->next;
	}
    }
}

/* Swap the entries in the list */
static void 
SwapEntries(XmWorkspaceWidget ww, XmWorkspaceLineRec * line1,
	    XmWorkspaceLineRec * line2, XmWorkspaceLineRec * prev_line1,
	    XmWorkspaceLineRec * prev_line2)
{

    /* Handle this as a special case */
    if (line1->next == line2)
    {
	if (prev_line1 == NULL)
	{
	    ww->workspace.lines = line2;
	    line1->next = line2->next;
	    line2->next = line1;
	}
	else
	{
	    prev_line1->next = line2;
	    line1->next = line2->next;
	    line2->next = line1;
	}
	return;
    }
    /* Remove line1 */
    if (prev_line1 == NULL)
	ww->workspace.lines = line1->next;
    else
	prev_line1->next = line1->next;
    /* Remove line2 */
    prev_line2->next = line2->next;
    /* Put line2 back where line1 was */
    if (prev_line1 == NULL)
    {
	line2->next = ww->workspace.lines;
	ww->workspace.lines = line2;
    }
    else
    {
	line2->next = prev_line1->next;
	prev_line1->next = line2;
    }
    /* Put line1 back where line2 was */
    line1->next = prev_line2->next;
    prev_line2->next = line1;
}

static LineElement *
ObstructedFind(XmWorkspaceWidget ww,
	       LineElement *element, 
	       int ymax, 
	       int horizontal_dir,
	       int srcx, int srcy, int dstx, int dsty,
	       Widget source, Widget destination,
	       int level,
	       int *cost,
	       XmWorkspaceLine new, int *failnum, LineList *ignore_lines)
{
    LineElement *line;
    LineElement new_src;
    int         xslot;
    int         *xsave;
    int         ycenter;
    int         ysave;
    int         ypos;

    /* Find open slots to the left and right of the current obstruction.
     * Remember that ymax is the farthest down that the source
     * can see at srcx (element->x).
     * This finds a column of unobstructed pixels starting here, down.
     * XXX Doesn't this all assume that we can get from x*slot to srcx
     * without a Collision?
     */
    xslot = FindVerticalSlot(ww, element->x, ymax,
				  horizontal_dir, LOOKDOWN, SLOT_LEN);

    if (xslot == -1)
    {
	*cost = 999;
	*failnum += 1;
#ifdef Printit
	printf("Routing Failure%d! level = %d \n", *failnum, level);
#endif
	return NULL;
    }

    /*
     * Look to the left and to the right for all y positions between the
     * source and the obstruction.  Record the results.
     * Step through all the y positions between the source and the 
     * obstruction looking for horizontal plumbs.  If we don't even
     * reach the slot, then just say we reached the slot (unless the slot
     * is passed dstx, in which case, say we reached dstx).
     */
    xsave  = (int *) XtMalloc(sizeof(int) * (ww->core.height));
#ifdef TRACE
    printf("Lev = %d, Look %s for all ypos\n", level,
	horizontal_dir == LOOKLEFT? "left": "right");
#endif
    for (ypos = srcy; ypos <= ymax; ypos++)
    {
	xsave[ypos] = FindHorizontalPlumbIgnoringLines(ww, srcx, ypos,
						       horizontal_dir,
						       ignore_lines);
	if (horizontal_dir == LOOKLEFT)
	{
	    if ((dstx > xslot) && (xsave[ypos] < xslot) &&
		(xsave[ypos] != -1))
	    {
		xsave[ypos] = xslot;
	    }
	    if ((dstx < xslot) && (xsave[ypos] < dstx) &&
		(xsave[ypos] != -1))
	    {
		xsave[ypos] = dstx;
	    }
	}
	else
	{
	    if ((dstx < xslot) && (xsave[ypos] > xslot) &&
		(xsave[ypos] != -1))
	    {
		xsave[ypos] = xslot;
	    }
	    if ((dstx > xslot) && (xsave[ypos] > dstx) &&
		(xsave[ypos] != -1))
	    {
		xsave[ypos] = dstx;
	    }
	}
    }

    ycenter = srcy + (ymax - srcy) / 2;
    /* Init it to something reasonable */
    new_src.x = -10000;
    new_src.y = ycenter;
    ysave = ycenter;

    /* Find the resulting x position that is closest to the dest */
    for (ypos = ycenter; ypos <= ymax; ypos++)
    {
	if ((Abs(xsave[ypos] - xslot) <
	     Abs(new_src.x - xslot)) &&
	    (xsave[ypos] != -1))
	{
	    ysave     = ypos;
	    new_src.y = ypos;
	    new_src.x = xsave[ypos];
	}
    }
    for (ypos = ycenter - 1; ypos >= srcy; ypos--)
    {
	if ((Abs(xsave[ypos] - xslot) <
	     Abs(new_src.x - xslot)) &&
	    (xsave[ypos] != -1))
	{
	    ysave     = ypos;
	    new_src.y = ypos;
	    new_src.x = xsave[ypos];
	}
    }
    element->y = ysave;

    XtFree((char*)xsave);

    /*
     * If we were unable to find a new point, failure.
     * XXX What kind of failure?
     */
    if (new_src.x == -10000)
    {
	*failnum += 2;
#ifdef Printit
	printf("Routing Failure%d! level = %d \n", *failnum, level);
#endif
	*cost = 999;
	return NULL;
    }
    else if (new_src.x == -1)
    {
	*failnum += 3;
#ifdef Printit
	printf("Routing Failure%d! level = %d \n", *failnum, level);
#endif
	*cost = 999;
	return NULL;
    }

    /* Adjust the x position so there is room to go down from here */
    else
    {
	new_src.x = FindVerticalSlot(ww, new_src.x, new_src.y,
				     (horizontal_dir == LOOKLEFT? 
					 LOOKRIGHT: LOOKLEFT),
				     LOOKDOWN, SLOT_LEN);
	if (new_src.x == -1)
	{
	    *cost = 999;
	    return NULL;
	}
	else
	{
	    line = ManhattanWorker(ww, new_src.x, new_src.y,
			     dstx, dsty, level, cost, source,
			     destination, new, failnum, ignore_lines);
	    if (new->reverse_on_copy)
	    {
		line = ReverseLine(line);
		new->reverse_on_copy = FALSE;
	    }
	}
    }
    return (line);
}

static int
MeasureLine(LineElement *line)
{
    int length = 0;
    int lastx;
    int lasty;

    if (!line)
	return 0;

    lastx = line->x;
    lasty = line->y;
    line = line->next;
    while (line)
    {
	length += Abs(lastx - line->x) + Abs(lasty - line->y);
	lastx = line->x;
	lasty = line->y;
	line = line->next;
    }

    return length;
}

static int
FindDogleg(XmWorkspaceWidget ww, 
	   LineElement *line,
	   LineElement *dogleg,
	   int ymax,
	   int *cost,
	   int horizontal_dir,
	   int vertical_dir)
{
    int		newx;
    LineElement *ret_line;
    Widget      ret_widget;

    /* find horizontal endpoint for the new line */
    newx = FindVerticalSlot(ww, line->x, line->y,
			    horizontal_dir, vertical_dir, SLOT_LEN);

    /* If newx = -1, there was no open slot! */
    if (newx == -1)
    {
	*cost = 999;
    }
    else
    {
	dogleg->x = newx;
	dogleg->y = line->y;

	/*
	 * If there is a collision with the horizontal segment of the
	 * dogleg, keep looking at successively higher y positions, until
	 * we hit ymax 
	 */
	while (((vertical_dir == LOOKUP   && line->y < ymax) ||
		(vertical_dir == LOOKDOWN && line->y > ymax)) &&
	       DetectYCollision(ww, line->y, line->x,
			    dogleg->x, (XmWorkspaceLine *)&ret_line, 
			    (Widget *)&ret_widget))
	{
	    line->y += (vertical_dir == LOOKUP? 1: -1) *
			ww->workspace.line_thickness;
	}
	dogleg->y = line->y;
	if ((vertical_dir == LOOKUP   && line->y >= ymax) ||
	    (vertical_dir == LOOKDOWN && line->y <= ymax))
	{
	    *cost = 999;
	}
    }
    return 0;
}

static LineElement*
ReverseLine(LineElement* line)
{
    LineElement *next;
    LineElement *last;

    if (!line)
	return line;

    last = line;
    line = line->next;
    last->next = NULL;
    while(line)
    {
	next = line->next;
	line->next = last;
	last = line;
	line = next;
    }
    return last;
}
