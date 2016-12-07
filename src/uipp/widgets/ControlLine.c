/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"



/*
 *  The graph is drawn within the framework of the top of a bar graph
 *  with a line along each edge (with length of the value difference
 *  with that neighbor) and accross the top.  Thus there are two points
 *  for each bar, at either end of the top of its bar.
 *
 *  The range of the entire graph is 0.0 at one edge and 1.0 at the
 *  other.  (Of course the 0-1 can be remapped to anything in the
 *  application).  Since the values at 0.0 and 1.0 are drawn and the
 *  edges are treated as the center of the bars for 0 and 1 in a bar
 *  graph, the first and last "bars" are half the width of the rest.
 *								     1.0 - 0
 *  The following is a crude example of a range of six bars across 	 -
 *  the graph.		0.0  | |  |  |  |  | |  1.0			   1
 *            		     0   1  2  3  4  5				 -
 *  0=(0.0-0.1), 1=(0.1-0.3), 2=(0.3-0.5),				   2
 *  3=(0.5-0.7), 4=(0.7-0.9), 5=(0.9-1.0)				 -
 *									   3
 *  The bar width is [(bar_count - 1) / width] (except the 2 half bars)	 -
 *									   4
 *      index = (int)(level * (float)(bar_count - 1))			 -
 *  To go from index to level at the bar's center:			   5
 *	level = (float)index / (float)(bar_count - 1)			 -
 *								     0.0 - 6
 *  The graph is adaptively created, following the bar edges only in
 *  cases where the level or its neighbor is used by more than 1 control
 *  point.  The reason for this is that a "connect-the-centers" line
 *  graph scheme creates a smoother graph, but an "outline-the-edges" bar
 *  graph scheme is needed to show discontinuities (as occurs between
 *  redundant control points at a given level: called overload in the code).
 *
 *  Bar_count - 1 is called last_level_index in the ControlLine record.
 */


#include <math.h>
#include "ControlField.h"


/*
 *  Declare templates for locally defined and used subroutines
 */
static void InitializeLine( ControlLine* line, int num_levels );


/*  Subroutine:	CreateControlLine
 *  Purpose:	Create and initialize a ControlLine structure (leave setting up
 *		GC for expose time)
 */
ControlLine* CreateControlLine( ControlField* field,
			        int load_limit, Pixel color )
{
    ControlLine* line;

    line = (ControlLine *)XtMalloc(sizeof(struct ControlLineRec));
    line->points = NULL;
    line->values = NULL;
    line->load = NULL;
    line->load_limit = load_limit;
    line->num_points = 0;
    line->num_levels = 0;
    line->field = field;
    line->gc = NULL;
    line->erase_gc = NULL;
    line->color = color;
    InitializeLine(line, field->num_levels);
    SetLineCoords(line);
    return line;
}


/*  Subroutine:	InitializeLine
 *  Purpose:	Set the fixed parameters of the drawn line for the current
 *		dimensions.  Malloc the space for values, points & loads
 */
static void InitializeLine( ControlLine* line, int num_levels )
{
    if( num_levels > line->num_levels )
    {
	if( line->values )
	    XtFree((char*)line->values);
	if( line->points )
	    XtFree((char*)line->points);
	if( line->load )
	    XtFree((char*)line->load);
	line->values = (double *)XtCalloc(num_levels, sizeof(double));
	line->points = (XPoint *)XtMalloc(2 * num_levels * sizeof(XPoint));
	/*  Need extra load to fake next which tests FALSE for last level  */
	line->load = (short *)XtCalloc(num_levels + 1, sizeof(short));
	line->load[num_levels] = 0;
    }
    line->num_points = 2 * num_levels;
    line->num_levels = num_levels;
    line->last_level_index = num_levels - 1;
}


/*  Subroutine:	SetLineCoords
 *  Purpose:	Set the proper coordinates to draw the line as per the control
 *		points
 *  Note:	Load limit is a measure to compare against the number of control
 *		point ranges which apply at a given level.  The test switches
 *		between simple graph (centered) and bar graph (two edges)
 */
void SetLineCoords( ControlLine* line )
{
    LineCoord* coord;
    int i;
    Boolean prior_overload, current_overload, next_overload;
    register XPoint* line_point = line->points;

    coord = line->field->coords;

    /*
     * RGB field.
     */
    if(line->field->num_lines == 3)
    {
	if( line->field->vertical )
	{
	    double value_range = (double)(line->field->width - 1);
	    short margin = line->field->left_margin;

	    prior_overload = FALSE;
	    current_overload = (line->load[0] > line->load_limit);
	    for( i=0; i<line->last_level_index; i++ )
	    {
		register short val_coord;

		val_coord = 
			(int)((line->values[i] * value_range) + 0.5) + margin;
		line_point->x = val_coord;
		if( prior_overload || current_overload )
		    line_point->y = coord[i].low;
		else
		    line_point->y = coord[i].mid;
		++line_point;
		next_overload = (line->load[i+1] > line->load_limit);
		if( current_overload || next_overload )
		{
		    line_point->x = val_coord;
		    line_point->y = coord[i].high;
		    ++line_point;
		}
		prior_overload = current_overload;
		current_overload = next_overload;
	    }
	}
	else
	{
	    double value_range = (double)(line->field->height - 1);
	    short margin = line->field->top_margin;

	    prior_overload = FALSE;
	    current_overload = (line->load[0] > line->load_limit);
	    for( i=0; i<line->last_level_index; i++ )
	    {
		register short val_coord;

		val_coord =
		  (int)(((1.0 - line->values[i]) * value_range) + 0.5) + margin;
		line_point->y = val_coord;
		if( prior_overload || current_overload )
		    line_point->x = coord[i].low;
		else
		    line_point->x = coord[i].mid;
		++line_point;
		next_overload = (line->load[i+1] > line->load_limit);
		if( current_overload || next_overload )
		{
		    line_point->y = val_coord;
		    line_point->x = coord[i].high;
		    ++line_point;
		}
		prior_overload = current_overload;
		current_overload = next_overload;
	    }
	}
    }
    else
    {
	if(line->field->num_maps > 0)
	{
	    for( i=0; i < line->field->map[0]->num_points; i++)
	    {
		if(i==0)
		{
		    line_point->x = line->field->map[0]->points[i].x;
		    line_point->y = line->field->height +
					line->field->top_margin - 1;
		    line_point++;
		}
		line_point->x = line->field->map[0]->points[i].x;
		line_point->y = line->field->map[0]->points[i].y;
		line_point++;
		if(i==(line->field->map[0]->num_points - 1))
		{
		    line_point->x = line->field->map[0]->points[i].x;
		    line_point->y = line->field->top_margin;
		    line_point++;
		}
	    }
	}
    }
    line->num_points = line_point - line->points;
}


/*  Subroutine:	DrawLine
 *  Purpose:	Draw the line on the screen and record a toggle in case it is
 *		an Xor operation
 */
void DrawLine( ControlLine* line, GC gc )
{
    if(gc == NULL) return;
    if (!line->field->w) return ;
    if (!XtIsRealized((Widget)(line->field->w))) return ;
    XDrawLines(XtDisplay(line->field->w), XtWindow(line->field->w),
	       gc, line->points, line->num_points,
	       CoordModeOrigin);
    XSync(XtDisplay(line->field->w), FALSE);
}
