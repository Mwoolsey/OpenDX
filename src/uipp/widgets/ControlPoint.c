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

#include <math.h>
#include "ColorMapEditor.h"
#include "ColorMapEditorP.h"

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif



#define HALF_LEVEL (1/(NUM_LEVELS*2))
/*
 *  Declare templates for locally defined and used subroutines
 */
void PrintAllControlPoints(XmColorMapEditorWidget cmew);
void PrintControlPoint(ControlMap *map, int point_num);
void AddControlPoint(ControlMap* map, double level, double value, 
		     Boolean above);
void CallActivateCallback(XmColorMapEditorWidget cmew, int reason);
void RemoveSelectedControlPoints(XmColorMapEditorWidget cmew);
static void DeselectAllPoints(ControlMap *map);
static Boolean PointExists(ControlMap *map, double level, double value);
static void InsertControlPoint( ControlMap* map, int i,
			        double level, double value );
static void SetControlBoxXY( ControlMap* map, XRectangle* box,
			     double level, double value );
static void GetControlValues( ControlMap* map, int x, int y,
			      double* level, double* value );
static int RemoveControlPoint( ControlMap* map, int index );
static int MoveControlPoint( ControlMap* map, int index, int x, int y );
static int RepositionControlPoint( ControlMap* map, int index,
				   double level, double value );
static int GetControlPoint( ControlMap* map, int x, int y, int grab_mode );
static void DrawRubberband(XmDrawingAreaWidget w, ControlMap* map);
static void StretchRubberband(XmDrawingAreaWidget w, ControlMap* map);
static void DrawMultiPointGrabBoxes( Widget w, ControlMap* map);
static void ClearMultiPointGrabs( ControlMap* map);
void LoadColormapFromFile(char *filename, XmColorMapEditorWidget cmew,
	Boolean init);
void RecalculateControlPointLevels(XmColorMapEditorWidget cmew);


/*  Subroutine:	CreateControlMap
 *  Purpose:	Create a ControlMap structure and partially initialize it.
 *  Note:	Some things, such as the GC, must be initialized at expose time
 */
ControlMap* CreateControlMap( ControlField* field, ControlLine* line,
			      Pixel color, Boolean clip_value )
{
    ControlMap* map;

    map = (ControlMap *)XtMalloc(sizeof(struct ControlMapRec));
    map->rubber_banding = False;
    map->points = NULL;
    map->undo_top = -1;
    map->num_undoable = 0;
    map->boxes = NULL;
    map->num_points = 0;
    map->size_buf = 0;
    /* map->grabbed is the 'current' control point.  The grabbed point is black,
       the rest are hollow.  Button 1 need not be kept depressed for a control
       point to be grabbed */
    map->grabbed = -1;
    map->clip_value = clip_value;
    map->double_click_interval = 350;
    map->field = field;
    map->line = line;
    map->gc = NULL;
    map->erase_gc = NULL;
    map->color = color;
    map->end_policy = FILL_LEVEL;
    map->multi_point_grab = False;
    map->toggle_on = False;
    return map;
}


/*  Subroutine:	CMEAddControlPoint
 *  Purpose:	External interface to add a control point
 */
void CMEAddControlPoint(Widget w, double level, double value, Boolean above )
{
XmColorMapEditorWidget cmew = (XmColorMapEditorWidget)w;
ControlField *field;
int i;

    PushUndoPoints(cmew);
    /*
     * Normalize the level to 0.0 - 1.0
     */
    level = (level - cmew->color_map_editor.value_minimum)/
		(cmew->color_map_editor.value_maximum - 
		cmew->color_map_editor.value_minimum);
    for(i = 0; i < 4; i++)
	{
	field = cmew->color_map_editor.g.field[i];
	if(!field->map[0]->toggle_on) continue;

	DrawControlBoxes(field->map[0], field->map[0]->erase_gc);
	DrawLine(field->map[0]->line, field->map[0]->line->erase_gc);
	AddControlPoint(field->map[0], level, value, True);
	SetArrayValues(field->map[0], field->line[0]);
	SetLineCoords(field->line[0]);
	DeselectAllPoints(field->map[0]);
	DrawControlBoxes(field->map[0], field->map[0]->gc);
	DrawLine(field->map[0]->line, field->map[0]->line->gc);
	field->user_callback(field);
	}
    PrintAllControlPoints(cmew);
    if(cmew->color_map_editor.g.field[0]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[1]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[2]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[3]->map[0]->toggle_on)
	CallActivateCallback(cmew, XmCR_MODIFIED);
}
/*  Subroutine:	AddControlPoint
 *  Purpose:	Put a new control point in the control point list
 */
void AddControlPoint(ControlMap* map, double level, double value, 
		     Boolean above)
{
    register int i;

    if(PointExists(map, level, value)) return;
    if (level > 1.0 )
	{
	level = 1.0;
	}
    if (level < 0.0)
	{
	level = 0.0;
	}
    /*  Position i at the index we want to use  */
    if(above)
	for(i = 0; 
	    (i < map->num_points) && (level >= map->points[i].level); 
	    i++ );
    else
	for(i = 0; 
	    (i < map->num_points) && (level > map->points[i].level); 
	    i++ );
    InsertControlPoint(map, i, level, value);
}


/*  Subroutine:	InsertControlPoint
 *  Purpose:	Knowing where the new point is to go, insert it
 */
static void InsertControlPoint( ControlMap* map, int index,
			        double level, double value )
{
    register int i;

    if( map->num_points == map->size_buf )
    {
	map->size_buf += ADDITION;
	if( map->points )
	    map->points =
		(ControlPoint *)XtRealloc((char*)map->points, 
				map->size_buf * sizeof(ControlPoint));
	else
	    map->points =
		(ControlPoint *)XtMalloc(map->size_buf * sizeof(ControlPoint));
	if( map->boxes )
	    map->boxes =
		(XRectangle *)XtRealloc((char*)map->boxes, 
				map->size_buf * sizeof(XRectangle));
	else
	    map->boxes =
		(XRectangle *)XtMalloc(map->size_buf * sizeof(XRectangle));
	/*  Set general stuff that is unchanged by other activity  */
	for( i=map->num_points; i<map->size_buf; i++ )
	{
	    map->boxes[i].width = HASH_SIZE;
	    map->boxes[i].height = HASH_SIZE;
	}
    }
    /*  Move everything above that index up  */
    for( i=map->num_points; i>index; i-- )
    {
	(void)memcpy((char *)&map->points[i], (char *)&map->points[i-1], 
			sizeof(ControlPoint));
	(void)memcpy((char *)&map->boxes[i], (char *)&map->boxes[i-1], 
			sizeof(XRectangle));
    }
    ++map->num_points;
    map->points[index].level = level;
    map->points[index].value = value;
    map->points[index].wrap_around = 0;
    map->points[index].multi_point_grab = False;
    SetControlBoxXY(map, &map->boxes[index], level, value);
    map->points[index].x = map->boxes[index].x + HASH_OFFSET;
    map->points[index].y = map->boxes[index].y + HASH_OFFSET;
    map->points[index].old_print_x = -999;
}


/*  Subroutine:	SetControlBoxXY
 *  Purpose:	Given level and value, set the drawing x and y for the hash box
 */
static void SetControlBoxXY( ControlMap* map, XRectangle* box,
			     double level, double value )
{
    /*  Roll value over into the range of 0.0 to 1.0  */
    if( value > 1.0 )
	value -= (double)((int)value);
    else if( value < 0.0 )
	value += 1.0 - (double)((int)value);
    if( map->field->vertical )
    {
	/*  Level is on vertical axis from bottom, value on horizontal  */
	box->x = (value * (map->field->width - 1))
	  + map->field->left_margin - HASH_OFFSET;
	box->y = ((1.0 - level) * (map->field->height - 1))
	  + map->field->top_margin - HASH_OFFSET; 
    }
    else
    {
	box->x = (level * (map->field->width - 1))
	  + map->field->left_margin - HASH_OFFSET;
	box->y = (value * (map->field->height - 1))
	  + map->field->top_margin - HASH_OFFSET;
    }
}


/*  Subroutine:	GetControlValues
 *  Purpose:	Given window x and y, set correspondong level and value.
 *		Clip value to range of 0-1.
 */
static void GetControlValues( ControlMap* map, int x, int y,
			      double* level, double* value )
{
double max_level;


    if( map->field->vertical )
    {
	/*  Level is on vertical axis (from bottom), value on horizontal  */
	*level = (double)(1 - ((double)(y - map->field->top_margin)/
			  (double)(map->field->height - 1)));
	*value = (double)((double)(x - map->field->left_margin)/
			  (double)(map->field->width - 1));
    }
    else
    {
	/*  Level is on horizontal axis (from left), value on vertical  */
	*level = (double)(x - map->field->left_margin)
	  / (double)(map->field->width - 1);
	*value = (double)(y - map->field->top_margin)
	  / (double)(map->field->height - 1);
    }
    /*  If value is a min-max between 0 and 1, restrict it  */
    if( map->clip_value )
    {
	/*  Clip value to range of 0-1  */
	if( *value > 1.0 )
	    *value = 1.0;
	else if( *value < 0.0 )
	    *value = 0.0;
    }
    max_level = 1.0;
    if( *level > max_level )
        *level = max_level;
    else if( *level < 0.0 )
        *level = 0.0;
}


/*  Subroutine: RemoveSelectedControlPoints
 *  Purpose:	Take all selected control points out of the field
 */
void RemoveSelectedControlPoints(XmColorMapEditorWidget cmew)
{
int i,j;
ControlField *field;
ControlMap *map;

    for(i = 0; i < 4; i++)
    {
	field = cmew->color_map_editor.g.field[i];
	map = field->map[0];
	if(!map->toggle_on) continue;
	
	DrawControlBoxes(field->map[0], field->map[0]->erase_gc);
	DrawLine(field->map[0]->line, field->map[0]->line->erase_gc);
	if (map->grabbed >= 0)
	    {
	    RemoveControlPoint(map,map->grabbed);
	    map->grabbed = -1;
	    }
	for (j = map->num_points - 1; j >= 0; j--)
	    {
	    if (map->points[j].multi_point_grab)
		{
		RemoveControlPoint(map,j);
		}
	    }
	SetArrayValues(field->map[0], field->line[0]);
	SetLineCoords(field->line[0]);
	DrawControlBoxes(field->map[0], field->map[0]->gc);
	DrawLine(field->line[0], field->map[0]->line->gc);
    }
    field->user_callback(field);
    PrintAllControlPoints(cmew);
}
/*  Subroutine: RemoveAllControlPoints
 *  Purpose:	Take all control points out of the field
 */
void RemoveAllControlPoints( ControlField *field)
{
int i, num_pts;

    num_pts = field->map[0]->num_points;
    for (i=0; i < num_pts; i++)
	{
	RemoveControlPoint(field->map[0],0);
	}
    PrintAllControlPoints(field->cmew);
}
/*  Subroutine: RemoveControlPoint
 *  Purpose:	Take a control point out of the list
 */
static int RemoveControlPoint( ControlMap* map, int index )
{
    int i;

    if( (index < 0) || (index >= map->num_points) )
	return 0;
    /*  Move everything between there and index down  */
    for( i=index+1; i<map->num_points; i++ )
    {
	(void)memcpy((char *)&map->points[i-1], (char *)&map->points[i], 
			sizeof(ControlPoint));
	(void)memcpy((char *)&map->boxes[i-1], (char *)&map->boxes[i], 
			sizeof(XRectangle));
    }
    --map->num_points;
    return 0;
}


/*  Subroutine:	MoveControlPoint
 *  Purpose:	Given window x and y, Reposition the Control Point
 */
static int MoveControlPoint( ControlMap* map, int index, int x, int y )
{
    double level, value;

    x = MAX(map->field->left_margin ,x);
    y = MAX(map->field->top_margin  ,y);
    x = MIN(map->field->width + map->field->left_margin - 1, x);
    y = MIN(map->field->height+ map->field->top_margin - 1, y);
    GetControlValues(map, x, y, &level, &value);
    map->points[index].x = x;
    map->points[index].y = y;
    /*  Include wrap-around offset if point has a prior wrap-around  */
    if( map->points[index].wrap_around != 0 )
	value += (double)map->points[index].wrap_around;
    return RepositionControlPoint(map, index, level, value);
}


/*  Subroutine:	RepositionControlPoint
 *  Purpose:	Shift a control point to a new location (maintain good order)
 *  Args:	Index of point being moved and its new level and value
 */
static int RepositionControlPoint( ControlMap* map, int index,
				   double level, double value )
{
Boolean get_new_vals;
Boolean use_adjacent_level;
double adjacent_level=0;
int min_y, max_y;

    /*
     * use_adjacent_level and adjacent_level were implemented for the case
     * where a control point "bumps" into another.  Since a use can place
     * a control point at an arbitrary (non-pixel aligned) level, when another
     * point "bumps" into it, it should get the level of the non-pixel aligned
     * point.
     */
    get_new_vals = False;
    use_adjacent_level = False;
    max_y = (map->field->height - 1)
	  + map->field->top_margin; 
    min_y = map->field->top_margin;
    if (index == 0)
	{
	if (map->points[index].y >= max_y)
	    {
	    map->points[index].y = max_y;
	    get_new_vals = True;
	    }
	if (map->num_points > 1)
	    {
	    if (map->points[index].y <= map->points[index+1].y )
		{
		map->points[index].y = map->points[index+1].y;
	    	get_new_vals = True;
		use_adjacent_level = True;
		adjacent_level = map->points[index+1].level;
		}
	    }
	}
    if (index == map->num_points - 1)
	{
	if (map->points[index].y <= min_y)
	    {
	    map->points[index].y = min_y;
	    get_new_vals = True;
	    }
	if (map->num_points > 1)
	    {
	    if (map->points[index].y >= map->points[index-1].y )
		{
		map->points[index].y = map->points[index-1].y;
	    	get_new_vals = True;
		use_adjacent_level = True;
		adjacent_level = map->points[index-1].level;
		}
	    }
	}
    if ( (index > 0) && (index < map->num_points-1) )
	{
	if ( map->points[index].y <= map->points[index+1].y )
	    {
	    map->points[index].y = map->points[index+1].y;
	    get_new_vals = True;
	    use_adjacent_level = True;
	    adjacent_level = map->points[index+1].level;
	    }
	if ( map->points[index].y >= map->points[index-1].y )
	    {
	    map->points[index].y = map->points[index-1].y;
	    get_new_vals = True;
	    use_adjacent_level = True;
	    adjacent_level = map->points[index-1].level;
	    }
	}
    if (get_new_vals)
	{	
	GetControlValues(map, map->points[index].x, map->points[index].y, 
			&level, &value);
	}
    if(use_adjacent_level)
	map->points[index].level = adjacent_level;
    else
	map->points[index].level = level;
    map->points[index].value = value;
    SetControlBoxXY(map, &map->boxes[index], level, value);
    return index;
}


/*  Subroutine:	GetControlPoint
 *  Purpose:	Return the index of the point indicated (if the radial
 *		distance is within GRAB_RANGE) or a new point
 *  Note:	grab_mode: <0:new | ==0:grab,else new | >0:grab, else nothing
 */
static int GetControlPoint( ControlMap* map, int x, int y, int grab_mode )
{
    double value, level;
    int i;
    int save_error;
    int save_i;
    register int error_i, error_y;

    GetControlValues(map, x, y, &level, &value);
    /*  Position i at the index we want to use  */
    for( i = 0; i < map->num_points; i++ )
	{
	if (level < (map->points[i].level - 
			(double)HASH_OFFSET/(double)NUM_LEVELS)) break;
	}
    /* If there are two points on the same level, select the one whose x value
       comes the closest to the x of the button press. Note that GRAB_RANGE
       is normally compared with the square of the error, so I simply
       check that the y values are w/in 3 pixels of an existing
       control point, rather than do the square root computation. */
    if (i < (map->num_points - 1))
	{
	    if ( (map->points[i].level == map->points[i+1].level) &&
	     (abs(y - map->points[i].y) < 3) )
	    {
	    if (abs(x - map->points[i+1].x) < abs(x - map->points[i].x) )
		    {
		    i++;
		    }
	    }
	}
    if( grab_mode >= 0 )
    {
	save_error = GRAB_RANGE + GRAB_RANGE;
	save_i = -1;
	for(i = 0; i < map->num_points; i++)
	{
	    error_i = x - (map->points[i].x);
	    error_i *= error_i;
	    if( error_i <= GRAB_RANGE )
	    {
		error_y = y - (map->points[i].y);
		error_y *= error_y;
		error_i += error_y;
		if (error_i < save_error)
		{
		    save_error = error_i;
		    save_i = i;
		}
	    }
	}
	if (save_i >= 0)
	{
	    return save_i;
	}
	else
	{
	    return -1;
	}
    }
    InsertControlPoint(map, i, level, value);
    return i;
}


/*  Subroutine:	RedrawControlBoxes
 *  Purpose:	Clear the window and draw the control points anew
 */
void RedrawControlBoxes( ControlMap* map )
{
    XClearWindow(XtDisplay(map->field->w), XtWindow(map->field->w));
    DrawControlBoxes(map, map->gc);
}


/*  Subroutine:	DrawControlBoxes
 *  Purpose:	Draw the visible control point hash boxes in the window
 */
void DrawControlBoxes( ControlMap* map, GC gc )
{
    int start, finish, index;
    int low, high;

    if (gc == NULL) return;

    if( map->field->vertical )
    {
	low = map->field->top_margin - HASH_SIZE;
	high = map->field->top_margin + map->field->height - 1;
	/*  Points are ordered highest y to lowest  */
	for( start=0;
	     (start<map->num_points) && (map->boxes[start].y>=high);
	     start++ );
	for( finish=start;
	     (finish<map->num_points) && (map->boxes[finish].y>=low);
	     finish++ );
    }
    else
    {
	low = map->field->left_margin - HASH_SIZE;
	high = map->field->left_margin + map->field->width - 1;
	/*  Points are ordered lowest x to highest  */
	for( start=0;
	     (start<map->num_points) && (map->boxes[start].x<low);
	     start++ );
	for( finish=start;
	     (finish<map->num_points) && (map->boxes[finish].x<high);
	     finish++ );
    }
    if( finish > start )
    {
	XDrawRectangles(XtDisplay(map->field->w), XtWindow(map->field->w),
			gc, &map->boxes[start], finish - start);
	if(   (map->grabbed >= 0)
	   && (map->grabbed >= start)
	   && (map->grabbed < finish) )
	{
	    XRectangle* box = &map->boxes[map->grabbed];
	    /*  Fill region inside box (fill size is 1 smaller than box)  */
	    ++box->x;
	    ++box->y;
	    --box->width;
	    --box->height;
	    XFillRectangles(XtDisplay(map->field->w), XtWindow(map->field->w),
			    gc, box, 1);
	    --box->x;
	    --box->y;
	    ++box->width;
	    ++box->height;
	}
	for(index=0; index < map->num_points; index++)
	    {
	    if (map->points[index].multi_point_grab)
		{
	        XRectangle* box = &map->boxes[index];
	        /*  Fill region inside box (fill size is 1 smaller than box)  */
	        ++box->x;
	        ++box->y;
	        --box->width;
	        --box->height;
	        XFillRectangles(XtDisplay(map->field->w), 
				XtWindow(map->field->w), gc, box, 1);
	        --box->x;
	        --box->y;
	        ++box->width;
	        ++box->height;
		}
	    }
    }
    XSync(XtDisplay(map->field->w), FALSE);
}


/*  Subroutine:	ResizeControlBoxes
 *  Purpose:	Update all box drawing params for the current sizes of things
 */
void ResizeControlBoxes( ControlMap* map )
{
    int i;
    for( i = 0; i < map->num_points; i++ )
	{
	SetControlBoxXY(map, &map->boxes[i],
			map->points[i].level, map->points[i].value);
	map->points[i].x = map->boxes[i].x + HASH_OFFSET;
	map->points[i].y = map->boxes[i].y + HASH_OFFSET;
	}
}


/*  Subroutine:	MapInput
 *  Purpose:	Handle input of button events
 *  Returns:	True if values have been changed
 */
Boolean MapInput( Widget w, ControlMap* map,
		  XmDrawingAreaCallbackStruct* call_data )
{
ControlField    *field;
static Time time = 0;	/* Time of last button1 press event */
int index, dummy_index;
int delta_y, delta_x;
int old_x=0;
int xmax;
int xmin;
int point;
Boolean constrain_horizontal;
Boolean constrain_vertical;
int i;
Boolean set;

    if ( ((call_data->event->type == ButtonPress) || 
	  (call_data->event->type == ButtonRelease)) &&
         (call_data->event->xbutton.button != Button1) )
	{
	return FALSE;
	}

    if ( (call_data->event->type == MotionNotify) &&
	!(call_data->event->xmotion.state & Button1Mask) )
	{
	return FALSE;
	}
    constrain_vertical = map->field->cmew->color_map_editor.constrain_vert &&
			 map->toggle_on;
    constrain_horizontal = map->field->cmew->color_map_editor.constrain_hor &&
			 map->toggle_on;
    if( call_data->event->type == ButtonPress )
        {
	if( call_data->event->xbutton.button == Button1 )
	    {
	    XtVaGetValues( map->field->toggle_b,
		XmNset, &set, NULL);
	    if(!set)
		{
		for(i = 0; i < 4; i++)
		    {
		    field = map->field->cmew->color_map_editor.g.field[i];
		    XtVaGetValues(field->toggle_b,
			XmNset, &set, NULL);
		    if((XmDrawingAreaWidget)w == field->w)
			{
			XtVaSetValues( field->toggle_b,
			    XmNset, True, NULL);
			map->toggle_on = True;
			}
		    else
			{
			if(set)
			    {
			    XtVaSetValues( field->toggle_b,
				XmNset, False, NULL);
			    field->map[0]->toggle_on = False;
			    }
			}
		    }
		}

	    if(ShiftMask & call_data->event->xbutton.state)
		{
		point = GetControlPoint(map, call_data->event->xbutton.x,
					call_data->event->xbutton.y, 1);
		if(point >= 0)
		    {
		    DrawControlBoxes(map, map->erase_gc);
		    if(map->grabbed == point)
			{
			map->grabbed = -1;
			}
		    else
			{
			map->points[point].multi_point_grab = 
				!map->points[point].multi_point_grab;
			if(map->grabbed != -1)
			    {
			    map->points[map->grabbed].multi_point_grab = True;
			    map->grabbed = -1;
			    }
			}
		    DrawControlBoxes(map, map->gc);
		    PrintAllControlPoints(map->field->cmew);
		    }
		return False;
		}
	    DrawControlBoxes(map, map->erase_gc);
	    DrawLine(map->line, map->line->erase_gc);
	    if(  (call_data->event->xbutton.time - time)
	       < map->double_click_interval )
	        {
		PushUndoPoints(map->field->cmew);
		ClearMultiPointGrabs(map);
		if( map->grabbed >= 0 )
		    {
		    RemoveControlPoint(map, map->grabbed);
		    map->grabbed = -1;
		    SetArrayValues(map, map->line);
		    SetLineCoords(map->line);
		    CallActivateCallback(map->field->cmew, XmCR_MODIFIED);
		    }
		else
		    {
		    map->grabbed =
		      GetControlPoint(map, call_data->event->xbutton.x,
				      call_data->event->xbutton.y, -1);
		    map->points[map->grabbed].wrap_around = 0;
		    }
		time = 0;
	        }
	    else	/* Not a double click */
	        {
		map->grabbed =
		    GetControlPoint(map, call_data->event->xbutton.x,
				    call_data->event->xbutton.y, 1);
		time = call_data->event->xbutton.time;
		/* Btn not pressed on a control point */
		if (map->grabbed == -1)
		    {
		    ClearMultiPointGrabs(map);
		    map->multi_point_grab = True;
		    map->rubber_banding = True;
		    map->first_rubber_band = True;
		    map->rubber_band_base_y = call_data->event->xbutton.y;
		    map->rubber_band_current_y = call_data->event->xbutton.y;
		    }
		else
		    {
		    /* Btn pressed on a control point that is part of a multi
			point grab */
		    PushUndoPoints(map->field->cmew);
		    if (map->points[map->grabbed].multi_point_grab)
			{
			/* set grabbed to -1 so we can distinguish this mode
			   while doing the motion processing */
			map->grabbed = -1;
		        map->multi_move_base_y = call_data->event->xbutton.y;
		        map->multi_move_base_x = call_data->event->xbutton.x;
			map->multi_point_move = True;
			}
		    else
			{
			ClearMultiPointGrabs(map);
			}
		    }
	        }
	    DrawControlBoxes(map, map->gc);
	    DrawLine(map->line, map->line->gc);
	    PrintAllControlPoints(map->field->cmew);
	    if( time == 0 )
		return TRUE;
	    }
        }
    else if( call_data->event->type == MotionNotify )
        {
	/*
	 * Shift/Button events are processed in the button press.
	 */
	if(ShiftMask & call_data->event->xmotion.state) return FALSE;

	if( map->grabbed >= 0 )
	    {
	    DrawControlBoxes(map, map->erase_gc);/* Erase the old boxes */
	    DrawLine(map->line, map->line->erase_gc);/* Erase the old lines */
	    if (constrain_vertical)
		{
		call_data->event->xmotion.x = map->points[map->grabbed].x;
		old_x = call_data->event->xmotion.x;
		}
	    if (constrain_horizontal)
		{
		call_data->event->xmotion.y = map->points[map->grabbed].y;
		}
	    map->grabbed =
	      MoveControlPoint(map, map->grabbed, call_data->event->xmotion.x,
			       call_data->event->xmotion.y);
	    if (constrain_vertical)
		{
		map->points[map->grabbed].x = old_x;
		}
	    SetArrayValues(map, map->line);
	    SetLineCoords(map->line);
	    DrawControlBoxes(map, map->gc);
	    DrawLine(map->line, map->line->gc);
	    PrintAllControlPoints(map->field->cmew);
	    return TRUE;
	    }
	else if(map->rubber_banding)
	    {
	    map->rubber_band_current_y = call_data->event->xmotion.y;
	    StretchRubberband((XmDrawingAreaWidget)w, map);
	    DrawMultiPointGrabBoxes(w, map);
	    }
	else if( map->multi_point_move )
	    {
	    DrawControlBoxes(map, map->erase_gc);     /* Erase the old boxes */
	    DrawLine(map->line, map->line->erase_gc); /* Erase the old lines */
	    delta_y = call_data->event->xmotion.y - map->multi_move_base_y;
	    delta_x = call_data->event->xmotion.x - map->multi_move_base_x;
	    if (constrain_vertical)
		{
		delta_x = 0;
		}
	    if (constrain_horizontal)
		{
		delta_y = 0;
		}
	    /* Calculate the y limits of movement for the group */
	    xmax = (map->field->width-1) +
		 map->field->right_margin - HASH_OFFSET;
	    xmin = map->field->left_margin;
	    if (delta_x != 0)
		{
		for(index = 0; index < map->num_points; index++)
		    {
		    if (map->points[index].multi_point_grab)
			{
			if (delta_x + map->points[index].x < xmin)
			    {
			    delta_x = -(map->points[index].x - xmin);
			    }
			if ((delta_x + map->points[index].x) > xmax)
			    {
			    delta_x = ((map->field->width-1) +
				map->field->left_margin) - 
				map->points[index].x;
			    }
			}
		    }
		}
	    if (delta_y > 0)
		{
	        for(index = 0; index < map->num_points; index++)
	            {
	            if (map->points[index].multi_point_grab)
		        {
		        if (index != 0)
			    {
		    	    if (delta_y > (map->points[index-1].y - 
						map->points[index].y) )
			        {
			        delta_y = map->points[index-1].y - 
						map->points[index].y;
			        }
			    }
		        else
			    {
			    delta_y = MIN(delta_y, (((map->field->height - 1) +
					map->field->top_margin - HASH_OFFSET) -
					map->points[0].y));
			    }
		        break;
		        }
	            }    
		}
	    if (delta_y < 0)
		{
	        for(index = map->num_points - 1; index >= 0;  index--)
	            {
	            if (map->points[index].multi_point_grab)
		        {
		        if ((index + 1) != map->num_points)
			    {
		    	    if (abs(delta_y) > abs(map->points[index+1].y - 
						map->points[index].y))
			        {
			        delta_y = map->points[index+1].y - 
						map->points[index].y;
			        }
			    }
		        else
			    {
		    	    if  ( (map->points[index].y + delta_y) <
				 (HASH_SIZE - 1)  ) 
				{
			        delta_y = -(map->points[index].y - HASH_SIZE+1);
			        }
			    }
		        break;
		        }
	            }
		}
	    if (delta_y >= 0)
		{
	        for (index = 0; index < map->num_points; index++)
		    {
		    if (map->points[index].multi_point_grab)
		        {	
	    	        dummy_index = MoveControlPoint(map, index, 
			    	map->points[index].x + delta_x,
				map->points[index].y + delta_y);
		        }
		    }
		}
	    else
		{
	        for (index = map->num_points - 1; index >= 0; index--)
		    {
		    if (map->points[index].multi_point_grab)
		        {	
	    	        dummy_index = MoveControlPoint(map, index, 
			    	map->points[index].x + delta_x, 
				map->points[index].y + delta_y);
			if (index != dummy_index)
			    {
			    map->points[index].multi_point_grab = True;
			    map->points[index+1].multi_point_grab = False;
			    }
		        }
		    }
		}
	    map->multi_move_base_y = call_data->event->xmotion.y;
	    map->multi_move_base_x = call_data->event->xmotion.x;
	    SetArrayValues(map, map->line);
	    SetLineCoords(map->line);
	    DrawControlBoxes(map, map->gc);
	    DrawLine(map->line, map->line->gc);
	    PrintAllControlPoints(map->field->cmew);
	    return TRUE;
	    }
        }
    else if(call_data->event->type == ButtonRelease)
        {
	/*
	 * Shift/Button events are processed in the button press.
	 */
	if(ShiftMask & call_data->event->xbutton.state)
	    {
	    CallActivateCallback(map->field->cmew, XmCR_SELECT);
	    return FALSE;
	    }

	if (map->rubber_banding)
	    {
	    map->rubber_banding = False;
	    map->multi_point_grab = False;
	    map->rubber_band_current_y = call_data->event->xbutton.y;
	    if(!map->first_rubber_band)
		{
		DrawRubberband((XmDrawingAreaWidget)w, map);
		}
	    DrawMultiPointGrabBoxes(w, map);
	    CallActivateCallback(map->field->cmew, XmCR_SELECT);
	    }
	else if(map->grabbed >= 0)
	    {
	    DrawControlBoxes(map, map->erase_gc);
	    DrawLine(map->line, map->line->erase_gc);
	    if (constrain_vertical)
		{
		call_data->event->xbutton.x = map->points[map->grabbed].x;
		}
	    if (constrain_horizontal)
		{
		call_data->event->xbutton.y = map->points[map->grabbed].y;
		}
	    map->grabbed =
	      MoveControlPoint(map, map->grabbed, call_data->event->xbutton.x,
			       call_data->event->xbutton.y);
	    if( map->clip_value == FALSE )
		/*  Record the degree of wrap-around  */
		map->points[map->grabbed].wrap_around =
		  (short)floor(map->points[map->grabbed].value);
	    SetArrayValues(map, map->line);
	    SetLineCoords(map->line);
	    CallActivateCallback(map->field->cmew, XmCR_MODIFIED);
	    DrawControlBoxes(map, map->gc);
	    DrawLine(map->line, map->line->gc);
	    return TRUE;
	    }
	else if (map->multi_point_move)
	    {
	    map->multi_point_move = False;
	    CallActivateCallback(map->field->cmew, XmCR_MODIFIED);
	    }
	else
	    {
	    CallActivateCallback(map->field->cmew, XmCR_SELECT);
	    }
	PrintAllControlPoints(map->field->cmew);
        }
    return FALSE;
}
static void DrawRubberband(XmDrawingAreaWidget w, ControlMap* map)
{
	XDrawRectangle(XtDisplay(w), XtWindow(w), map->field->rubber_band_gc,
		w->manager.shadow_thickness,
		MIN(map->rubber_band_base_y, map->rubber_band_current_y),
		w->core.width-2*(w->manager.shadow_thickness)-1,
		abs(map->rubber_band_base_y - map->rubber_band_current_y));

	map->rubber_band_old_y = map->rubber_band_current_y;
}
static void StretchRubberband(XmDrawingAreaWidget w, ControlMap* map)
{
    if (!map->first_rubber_band)
    XDrawRectangle(XtDisplay(w), XtWindow(w), map->field->rubber_band_gc,
	w->manager.shadow_thickness,
	MIN(map->rubber_band_base_y, map->rubber_band_old_y),
	w->core.width-2*(w->manager.shadow_thickness)-1,
	abs(map->rubber_band_base_y - map->rubber_band_old_y));

    XDrawRectangle(XtDisplay(w), XtWindow(w), map->field->rubber_band_gc,
	w->manager.shadow_thickness,
	MIN(map->rubber_band_base_y, map->rubber_band_current_y),
	w->core.width-2*(w->manager.shadow_thickness)-1,
	abs(map->rubber_band_base_y - map->rubber_band_current_y));

    map->first_rubber_band = False;
    map->rubber_band_old_y = map->rubber_band_current_y;
}
/*  Subroutine:	DrawControlBoxes
 *  Purpose:	Draw the visible control point hash boxes in the window
 */
static void DrawMultiPointGrabBoxes( Widget w, ControlMap* map)
{
int start, finish;
int low, high;
int yupper, ylower, index;

    if( map->field->vertical )
        {
	low = map->field->top_margin - HASH_SIZE;
	high = map->field->top_margin + map->field->height - 1;
	/*  Points are ordered highest y to lowest  */
	for( start=0;
	     (start<map->num_points) && (map->boxes[start].y>high);
	     start++ );
	for( finish=start;
	     (finish<map->num_points) && (map->boxes[finish].y>low);
	     finish++ );
        }
    else
        {
	low = map->field->left_margin - HASH_SIZE;
	high = map->field->left_margin + map->field->width - 1;
	/*  Points are ordered lowest x to highest  */
	for( start=0;
	     (start<map->num_points) && (map->boxes[start].x<low);
	     start++ );
	for( finish=start;
	     (finish<map->num_points) && (map->boxes[finish].x<high);
	     finish++ );
        }
    if( finish > start )
        {
	ylower = MIN(map->rubber_band_current_y, 
			map->rubber_band_base_y) - 2*HASH_OFFSET;
	yupper = MAX(map->rubber_band_current_y, 
			map->rubber_band_base_y) + HASH_OFFSET;
        for( index=0; (index < map->num_points); index++ )
	    {
	    /* Is the point between the grab range ? */
	    if ( (map->boxes[index].y > ylower) && 
		 (map->boxes[index].y < yupper) )
	        {
		/* If the point is currently not grabbed, fill it in */
		if (!map->points[index].multi_point_grab)
		    {
	            XRectangle* box = &map->boxes[index];
		    map->points[index].multi_point_grab = True;
	        /*  Fill region inside box (fill size is 1 smaller than box)  */
	            ++box->x;
	            ++box->y;
	            --box->width;
	            --box->height;
	            XFillRectangles(XtDisplay(map->field->w), 
				XtWindow(map->field->w), map->gc, box, 1);
	            --box->x;
	            --box->y;
	            ++box->width;
	            ++box->height;
		    }
	        }
	    else
	        {
	        /* The point is out of the grab range. If it was grabbed,
		   unfill the rectangle  */
		if (map->points[index].multi_point_grab)
		    {
	            XRectangle* box = &map->boxes[index];
		    map->points[index].multi_point_grab = False;
	        /*  Fill region inside box (fill size is 1 smaller than box)  */
	            ++box->x;
	            ++box->y;
	            --box->width;
	            --box->height;
	            XFillRectangles(XtDisplay(map->field->w), 
				XtWindow(map->field->w), map->erase_gc, box, 1);
	            --box->x;
	            --box->y;
	            ++box->width;
	            ++box->height;
		    }
	        }
	    }
        }
    XSync(XtDisplay(map->field->w), FALSE);
}
/*  Subroutine:	DrawControlBoxes
 *  Purpose:	Draw the visible control point hash boxes in the window
 */
static void ClearMultiPointGrabs( ControlMap* map)
{
int index;

    for (index = 0; index < map->num_points; index ++)
	{
	map->points[index].multi_point_grab = False;
	}
}
/*  Subroutine:	CallActivateCallback
 *  Purpose:	Call registered callbacks to report value change 
 */
void CallActivateCallback( XmColorMapEditorWidget cmew, int reason)
{
XmColorMapEditorCallbackStruct call_value;
int 		i;
ControlField 	*field;
ControlMap 	*map;
int		index;

    call_value.reason = reason;
    call_value.op_values = NULL;
    call_value.val_values = NULL;
    call_value.sat_values = NULL;
    call_value.hue_values = NULL;

    for(i = 0; i < 4; i++)
	{
	if (cmew->color_map_editor.g.field[i]->map[0]->toggle_on)
	    {
	    /*
	     * The value MUST match the #defines in ColorMapEditor.h
	     */
	    call_value.selected_area = i;
	    break;
	    }
	}
    call_value.min_value = cmew->color_map_editor.value_minimum;
    call_value.max_value = cmew->color_map_editor.value_maximum;
    field = cmew->color_map_editor.g.field[0];

    /*
     * Note: If the field toggle is not on, set the number of selected control
     * points to zero, so the application can set menu sensitivity correctly.
     */
    call_value.hue_selected = 0;
    call_value.sat_selected = 0;
    call_value.val_selected = 0;
    call_value.op_selected = 0;
    if(!field->map[0]->toggle_on)
	{
	call_value.hue_selected = 0;
	}
    else if (field->map[0]->grabbed >=0 )
	{
	call_value.hue_selected = 1;
	}
    else
	{
	for(i = 0; i < field->map[0]->num_points;
						i++)
	    {
	    if (field->map[0]->points[i].multi_point_grab)
		{
		call_value.hue_selected++;
		}
	    }
	}
    field = cmew->color_map_editor.g.field[1];
    if(!field->map[0]->toggle_on)
	{
	call_value.sat_selected = 0;
	}
    else if (field->map[0]->grabbed >=0 )
	{
	call_value.sat_selected = 1;
	}
    else
	{
	for(i = 0; i < field->map[0]->num_points; i++)
	    {
	    if (field->map[0]->points[i].multi_point_grab)
		{
		call_value.sat_selected++;
		}
	    }
	}
    field = cmew->color_map_editor.g.field[2];
    if(!field->map[0]->toggle_on)
	{
	call_value.val_selected = 0;
	}
    else if (field->map[0]->grabbed >=0 )
	{
	call_value.val_selected = 1;
	}
    else
	{
	for(i = 0; i < field->map[0]->num_points;
						i++)
	    {
	    if (field->map[0]->points[i].multi_point_grab)
		{
		call_value.val_selected++;
		}
	    }
	}
    field = cmew->color_map_editor.g.field[3];
    if(!field->map[0]->toggle_on)
	{
	call_value.op_selected = 0;
	}
    else if (field->map[0]->grabbed >=0 )
	{
	call_value.op_selected = 1;
	}
    else
	{
	for(i = 0; i < field->map[0]->num_points;
						i++)
	    {
	    if (field->map[0]->points[i].multi_point_grab)
		{
		call_value.op_selected++;
		}
	    }
	}
#ifdef Comment
{
    struct 
    {
    Boolean	occupied;
    int		num_hues;
    int		num_sats;
    int		num_vals;
    int		hue_index[2];
    int		sat_index[2];
    int		val_index[2];
    int		wrap_around;
    double	level;
    } flags[NUM_LEVELS];

    int		num_alloced_pts;

    /* Initialize the flags array */
    for (i = 0; i < NUM_LEVELS; i++)
	{
	flags[i].occupied = False;
	flags[i].num_hues = 0;
	flags[i].num_sats = 0;
	flags[i].num_vals = 0;
	flags[i].wrap_around = 0;
	}
	/* Find all the control points and set the corresponding flag in the 
	   flags array. */
	for (i = 0; i < 3; i ++)
	    {
	    field = cmew->color_map_editor.g.field[i];
	    for (j = 0; j < field->map[0]->num_points; j++)
		{
		level = field->map[0]->points[j].level;
		index = level*(NUM_LEVELS-1) + 0.5;
		flags[index].occupied = True;
		flags[index].level = field->map[0]->points[j].level;
		switch(i)
		    {
		    case 0:
			if(flags[index].num_hues == 0)
			    {
			    flags[index].num_hues = 1;
			    flags[index].hue_index[0] = j;
			    }
			else
			    {
			    flags[index].num_hues = 2;
			    flags[index].hue_index[1] = j;
			    }
			break;
		    case 1:
			if(flags[index].num_sats == 0)
			    {
			    flags[index].num_sats = 1;
			    flags[index].sat_index[0] = j;
			    }
			else
			    {
			    flags[index].num_sats = 2;
			    flags[index].sat_index[1] = j;
			    }
			break;
		    case 2:
			if(flags[index].num_vals == 0)
			    {
			    flags[index].num_vals = 1;
			    flags[index].val_index[0] = j;
			    }
			else
			    {
			    flags[index].num_vals = 2;
			    flags[index].val_index[1] = j;
			    }
			break;
		    }
		/* Record hue wrap arounds */
		if (i == 0)
		    {
		    flags[index].wrap_around = 
				field->map[0]->points[j].wrap_around;
		    }
		}
	    }
	if (!flags[0].occupied)
	    {
	    flags[0].occupied = True;
	    flags[0].level = 0.0;
	    }
	if (!flags[NUM_LEVELS-1].occupied)
	    {
	    flags[NUM_LEVELS-1].occupied = True;
	    flags[NUM_LEVELS-1].level = 1.0;
	    }
	/* Count the number of control points we have to send */
	call_value.num_points = 0;
	for (i = 0; i < NUM_LEVELS; i++)
	    {
	    if ( flags[i].occupied )
		{
	        call_value.num_points++;
		if ( (flags[i].num_hues == 2) ||
		     (flags[i].num_sats == 2) ||
		     (flags[i].num_vals == 2) )
		    {
	            call_value.num_points++;
		    }
		}
	    }
	call_value.reason = reason;
	call_value.data = 
		(double *)XtMalloc(call_value.num_points*sizeof(double));
	call_value.hue_values = 
		(double *)XtMalloc(call_value.num_points*sizeof(double));
	call_value.sat_values = 
		(double *)XtMalloc(call_value.num_points*sizeof(double));
	call_value.val_values = 
		(double *)XtMalloc(call_value.num_points*sizeof(double));

	j = 0;
	for (i = 0; i < NUM_LEVELS; i++)
	    {
	    if ( flags[i].occupied )
		{
		if ( (flags[i].num_hues < 2) &&
		     (flags[i].num_sats < 2) &&
		     (flags[i].num_vals < 2) )
		    {
		    call_value.data[j] = cmew->color_map_editor.value_minimum + 
			flags[i].level*(cmew->color_map_editor.value_maximum - 
			cmew->color_map_editor.value_minimum);
	     	    call_value.hue_values[j] = flags[i].wrap_around +
			cmew->color_map_editor.g.field[0]->line[0]->values[i];
	     	    call_value.sat_values[j] =
			cmew->color_map_editor.g.field[1]->line[0]->values[i];
	     	    call_value.val_values[j] =
			cmew->color_map_editor.g.field[2]->line[0]->values[i];
			j++;
		    }
		else
		    {
		    call_value.data[j] = cmew->color_map_editor.value_minimum + 
			flags[i].level*(cmew->color_map_editor.value_maximum - 
			cmew->color_map_editor.value_minimum);
		    field = cmew->color_map_editor.g.field[0];
		    if(flags[i].num_hues == 2)
			{
			call_value.hue_values[j] = 
			    field->map[0]->points[flags[i].hue_index[0]].value;
			}
		    else
			{
	     	        call_value.hue_values[j] = flags[i].wrap_around +
			field->line[0]->values[i];
			}
			    
		    field = cmew->color_map_editor.g.field[1];
		    if(flags[i].num_sats == 2)
			{
			call_value.sat_values[j] = 
			    field->map[0]->points[flags[i].sat_index[0]].value;
			}
		    else
			{
	     	        call_value.sat_values[j] = field->line[0]->values[i];
			}
			    
		    field = cmew->color_map_editor.g.field[2];
		    if(flags[i].num_vals == 2)
			{
			call_value.val_values[j] = 
			    field->map[0]->points[flags[i].val_index[0]].value;
			}
		    else
			{
	     	        call_value.val_values[j] = field->line[0]->values[i];
			}
		    j++;

		    call_value.data[j] = cmew->color_map_editor.value_minimum + 
			flags[i].level*(cmew->color_map_editor.value_maximum - 
			cmew->color_map_editor.value_minimum);
		    field = cmew->color_map_editor.g.field[0];
		    if(flags[i].num_hues == 2)
			{
			call_value.hue_values[j] = 
			    field->map[0]->points[flags[i].hue_index[1]].value;
			}
		    else
			{
	     	        call_value.hue_values[j] = flags[i].wrap_around +
			field->line[0]->values[i];
			}
			    
		    field = cmew->color_map_editor.g.field[1];
		    if(flags[i].num_sats == 2)
			{
			call_value.sat_values[j] = 
			    field->map[0]->points[flags[i].sat_index[1]].value;
			}
		    else
			{
	     	        call_value.sat_values[j] = field->line[0]->values[i];
			}
			    
		    field = cmew->color_map_editor.g.field[2];
		    if(flags[i].num_vals == 2)
			{
			call_value.val_values[j] = 
			    field->map[0]->points[flags[i].val_index[1]].value;
			}
		    else
			{
	     	        call_value.val_values[j] = field->line[0]->values[i];
			}
		    j++;
		    }
		}
	    }

	/* Opacity field */
	field = cmew->color_map_editor.g.field[3];
	call_value.num_op_points = field->map[0]->num_points;
	num_alloced_pts = call_value.num_op_points;
	if (field->map[0]->points[0].level != 0.0)
	    {
	    num_alloced_pts++;
	    }
	if (field->map[0]->points[call_value.num_op_points-1].level < 1.0)
	     {
	    num_alloced_pts++;
	    }
	call_value.op_data = 
		(double *)XtMalloc(num_alloced_pts*sizeof(double));
	call_value.op_values = 
		(double *)XtMalloc(num_alloced_pts*sizeof(double));
	if (field->map[0]->points[0].level != 0.0)
	    {
	    call_value.op_data[0] = cmew->color_map_editor.value_minimum;
	    call_value.op_values[0] = field->map[0]->line[0].values[0];
	    index = 1;
	    }
	else
	    {
	    index = 0;
	    }
	for (i = 0; i < call_value.num_op_points; i++)
	    {
	    call_value.op_data[index] = cmew->color_map_editor.value_minimum + 
		field->map[0]->points[i].level*
		(cmew->color_map_editor.value_maximum - 
		 cmew->color_map_editor.value_minimum);
	    call_value.op_values[index] =
		 cmew->color_map_editor.g.field[3]->map[0]->points[i].value;
	    index++;
	    }
	if (field->map[0]->points[call_value.num_op_points-1].level < 1.0)
	    {
	    call_value.op_data[index] = cmew->color_map_editor.value_maximum;
	    call_value.op_values[index] =
		 field->map[0]->line[0].values[(int)(NUM_LEVELS-1)];
	    }
	call_value.num_op_points = num_alloced_pts;
 }
#endif
	/*minimum = cmew->color_map_editor.value_minimum;*/
	/*range = (cmew->color_map_editor.value_maximum -*/
        /*	   cmew->color_map_editor.value_minimum);*/

	/* Hue field */
	map = cmew->color_map_editor.g.field[HUE]->map[0];
	index = map->num_points;
	call_value.num_hue_points = index;
	call_value.hue_values = (double *)XtMalloc(2*index*sizeof(double));

	for (i = 0; i < index ; i++)
	{
#ifdef Comment
	    call_value.hue_values[i] =  minimum + map->points[i].level*range;
#else
	    call_value.hue_values[i] =  map->points[i].level;
#endif
	    call_value.hue_values[i + index] = map->points[i].value;
	}

	/* Saturation field */
	map = cmew->color_map_editor.g.field[SATURATION]->map[0];
	index = map->num_points;
	call_value.num_sat_points = index; 
	call_value.sat_values = (double *)XtMalloc(2*index*sizeof(double));
	for (i = 0; i < index ; i++)
	{
#ifdef Comment
	    call_value.sat_values[i] =  minimum + map->points[i].level*range;
#else
	    call_value.sat_values[i] =  map->points[i].level;
#endif
	    call_value.sat_values[index +i] = map->points[i].value;
	}

	/* Value field */
	map = cmew->color_map_editor.g.field[VALUE]->map[0];
	index = map->num_points;
	call_value.num_val_points = index; 
	call_value.val_values = (double *)XtMalloc(2*index*sizeof(double));
	for (i = 0; i < index ; i++)
	{
#ifdef Comment
	    call_value.val_values[i] =  minimum + map->points[i].level*range;
#else
	    call_value.val_values[i] =  map->points[i].level;
#endif
	    call_value.val_values[index +i] = map->points[i].value;
	}

	/* Opacity field */
	map = cmew->color_map_editor.g.field[OPACITY]->map[0];
	index = map->num_points;
	call_value.num_op_points = index; 
	call_value.op_values = (double *)XtMalloc(2*index*sizeof(double));
	for (i = 0; i < index ; i++)
	{
#ifdef Comment
	    call_value.op_values[i] = minimum + map->points[i].level*range;
#else
	    call_value.op_values[i] =  map->points[i].level;
#endif
	    call_value.op_values[i + index ] = map->points[i].value;
	}

	XtCallCallbacks((Widget)cmew, XmNactivateCallback, &call_value);

	if (call_value.op_values)
		XtFree((char*)call_value.op_values);
	if (call_value.val_values)
		XtFree((char*)call_value.val_values);
	if (call_value.sat_values)
		XtFree((char*)call_value.sat_values);
	if (call_value.hue_values)
		XtFree((char*)call_value.hue_values);
}
void LoadHardCodeDefaults(XmColorMapEditorWidget cmew, Boolean init)
{
int i;
ControlPoint cp_ptr;
int n;
Arg wargs[12];

  if (!init)
    {
    for (i=0; i<4; i++)
	{
	DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->erase_gc);
	DrawLine(cmew->color_map_editor.g.field[i]->map[0]->line,
		cmew->color_map_editor.g.field[i]->map[0]->line->erase_gc);
	RemoveAllControlPoints(cmew->color_map_editor.g.field[i]);
	}
    } 
  cmew->color_map_editor.value_minimum = 0.0;
  cmew->color_map_editor.value_maximum = 100.0;
  if (!init)
    {
    n=0;
    DoubleSetArg(wargs[n], XmNdValue, cmew->color_map_editor.value_minimum);n++;
    XtSetValues((Widget)cmew->color_map_editor.min_num, wargs, n);
    n=0;
    DoubleSetArg(wargs[n], XmNdValue, cmew->color_map_editor.value_maximum);n++;
    XtSetValues((Widget)cmew->color_map_editor.max_num, wargs, n);
    }

  /* Add the 2 Hue Control Points */
  cp_ptr.level = 0.0;
  cp_ptr.value = 0.6666667;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[0]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  cp_ptr.level = 1.0;
  cp_ptr.value = 0.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[0]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  SetArrayValues(cmew->color_map_editor.g.field[0]->map[0],
   cmew->color_map_editor.g.field[0]->line[0]);
  SetLineCoords(cmew->color_map_editor.g.field[0]->line[0]);

  /* Add the Saturation ControlPoints */
  cp_ptr.level = 0.0;
  cp_ptr.value = 1.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[1]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  cp_ptr.level = 1.0;
  cp_ptr.value = 1.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[1]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  SetArrayValues(cmew->color_map_editor.g.field[1]->map[0],
	    cmew->color_map_editor.g.field[1]->line[0]);
  SetLineCoords(cmew->color_map_editor.g.field[1]->line[0]);

  /* Add the Value ControlPoints */
  cp_ptr.level = 0.0;
  cp_ptr.value = 1.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[2]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  cp_ptr.level = 1.0;
  cp_ptr.value = 1.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[2]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  SetArrayValues(cmew->color_map_editor.g.field[2]->map[0],
	    cmew->color_map_editor.g.field[2]->line[0]);
  SetLineCoords(cmew->color_map_editor.g.field[2]->line[0]);


  /* Add the Opacity ControlPoints */
  cp_ptr.level = 1.0;
  cp_ptr.value = 1.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[3]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  cp_ptr.level = 0.0;
  cp_ptr.value = 1.0;
  cp_ptr.wrap_around = 0;
  cp_ptr.x = 0;
  cp_ptr.y = 0;
  AddControlPoint( cmew->color_map_editor.g.field[3]->map[0], 
            cp_ptr.level, cp_ptr.value, True );

  SetArrayValues(cmew->color_map_editor.g.field[3]->map[0],
	    cmew->color_map_editor.g.field[3]->line[0]);
  SetLineCoords(cmew->color_map_editor.g.field[3]->line[0]);

  for (i = 0; i < 4; i++)
    {
    if (!init)
      {
      DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->gc);
      DrawLine(cmew->color_map_editor.g.field[i]->line[0],
		cmew->color_map_editor.g.field[i]->map[0]->line->gc);
      cmew->color_map_editor.g.field[i]->user_callback(cmew->color_map_editor.g.field[i]);
      }
    }
}

void LoadColormapFromFile(char *filename, XmColorMapEditorWidget cmew,
	Boolean init)
{
int i,j;
ControlPoint cp_ptr;
FILE *fp;
int num_points, n;
char *message;
Arg wargs[12];
double min, max;

  if ( (fp = fopen(filename, "r")) == NULL)
    {
    message = XtMalloc(strlen(filename) + 64);
    strcpy(message, "Cannot open color map file \"");
    strcat(message, filename);
    strcat(message, "\".");
    XtWarning(message);
    XtFree(message);

    LoadHardCodeDefaults(cmew, init);
    return;
    }

  if (!init)
    {
    for (i=0; i<4; i++)
      {
      DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->erase_gc);
      DrawLine(cmew->color_map_editor.g.field[i]->map[0]->line,
		cmew->color_map_editor.g.field[i]->map[0]->line->erase_gc);
      RemoveAllControlPoints(cmew->color_map_editor.g.field[i]);
      SetArrayValues(cmew->color_map_editor.g.field[i]->map[0],
          cmew->color_map_editor.g.field[i]->line[0]);
      SetLineCoords(cmew->color_map_editor.g.field[i]->line[0]);
      DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->gc);
      DrawLine(cmew->color_map_editor.g.field[i]->line[0],
		cmew->color_map_editor.g.field[i]->map[0]->line->gc);
      }
    } 
  if ( fscanf(fp,"%lg",&min) < 0 )
    {
    message = XtMalloc(strlen(filename) + 64);
    strcpy(message, "Error occurred while reading color map file \"");
    strcat(message, filename);
    strcat(message, "\".");
    XtWarning(message);
    XtFree(message);

    LoadHardCodeDefaults(cmew, init);
    return;
    }
  if ( fscanf(fp,"%lg",&max) < 0 )
    {
    message = XtMalloc(strlen(filename) + 64);
    strcpy(message, "Error occurred while reading color map file \"");
    strcat(message, filename);
    strcat(message, "\".");
    XtWarning(message);
    XtFree(message);

    LoadHardCodeDefaults(cmew, init);
    return;
    }
  if (!init)
    {
    if(cmew->color_map_editor.min_editable)
	{
	cmew->color_map_editor.value_minimum = min;
	n=0;
	DoubleSetArg(wargs[n], XmNdValue, min);n++;
	XtSetValues((Widget)cmew->color_map_editor.min_num, wargs, n);
	}
    if(cmew->color_map_editor.max_editable)
	{
	cmew->color_map_editor.value_maximum = max;
	n=0;
	DoubleSetArg(wargs[n], XmNdValue, max);n++;
	XtSetValues((Widget)cmew->color_map_editor.max_num, wargs, n);
	}
    }

  for (i=0; i<3; i++)				/* for all three fields */
    {
    if(!init)
      {
          DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->gc);
          DrawLine(cmew->color_map_editor.g.field[i]->map[0]->line,
		cmew->color_map_editor.g.field[i]->map[0]->line->gc);
      }

    if ( fscanf(fp,"%d",&num_points) < 0 )
	{
	message = XtMalloc(strlen(filename) + 64);
	strcpy(message, "Error occurred while reading color map file \"");
	strcat(message, filename);
	strcat(message, "\".");
	XtWarning(message);
	XtFree(message);

	LoadHardCodeDefaults(cmew, init);
	return;
	}
    for (j=0;j < num_points; j++)
  	{
	if ( fscanf(fp,"%lg",&(cp_ptr.level)) < 0 )
	    {
	    message = XtMalloc(strlen(filename) + 64);
	    strcpy(message, "Error occurred while reading color map file \"");
	    strcat(message, filename);
	    strcat(message, "\".");
	    XtWarning(message);
	    XtFree(message);

	    LoadHardCodeDefaults(cmew, init);
	    return;
	    }
	if ( fscanf(fp,"%lg",&(cp_ptr.value)) < 0 )
	    {
	    message = XtMalloc(strlen(filename) + 64);
	    strcpy(message, "Error occurred while reading color map file \"");
	    strcat(message, filename);
	    strcat(message, "\".");
	    XtWarning(message);
	    XtFree(message);

	    LoadHardCodeDefaults(cmew, init);
	    return;
	    }
	if ( fscanf(fp,"%d",&(cp_ptr.wrap_around)) < 0 )
	    {
	    message = XtMalloc(strlen(filename) + 64);
	    strcpy(message, "Error occurred while reading color map file \"");
	    strcat(message, filename);
	    strcat(message, "\".");
	    XtWarning(message);
	    XtFree(message);

	    LoadHardCodeDefaults(cmew, init);
	    return;
	    }
	if ( fscanf(fp,"%d",&(cp_ptr.x)) < 0 )
	    {
	    message = XtMalloc(strlen(filename) + 64);
	    strcpy(message, "Error occurred while reading color map file \"");
	    strcat(message, filename);
	    strcat(message, "\".");
	    XtWarning(message);
	    XtFree(message);

	    LoadHardCodeDefaults(cmew, init);
	    return;
	    }
	if ( fscanf(fp,"%d",&(cp_ptr.y)) < 0 )
	    {
	    message = XtMalloc(strlen(filename) + 64);
	    strcpy(message, "Error occurred while reading color map file \"");
	    strcat(message, filename);
	    strcat(message, "\".");
	    XtWarning(message);
	    XtFree(message);

	    LoadHardCodeDefaults(cmew, init);
	    return;
	    }
        AddControlPoint( cmew->color_map_editor.g.field[i]->map[0], 
            cp_ptr.level, cp_ptr.value, True );
	SetArrayValues(cmew->color_map_editor.g.field[i]->map[0],
	    cmew->color_map_editor.g.field[i]->line[0]);
	SetLineCoords(cmew->color_map_editor.g.field[i]->line[0]);
        }
      if (!init)
	{
        DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->gc);
        DrawLine(cmew->color_map_editor.g.field[i]->line[0],
		cmew->color_map_editor.g.field[i]->map[0]->line->gc);
        cmew->color_map_editor.g.field[i]->user_callback(cmew->color_map_editor.g.field[i]);
	}
    }

    /* Handle Opacity separately for backwards comaptabilty with old
       colormap files */
    i = 3;
    if(!init)
      {
          DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->gc);
          DrawLine(cmew->color_map_editor.g.field[i]->map[0]->line,
		cmew->color_map_editor.g.field[i]->map[0]->line->gc);
      }

    if ( fscanf(fp,"%d",&num_points) < 0 )
	{
 	num_points = 1;
	cp_ptr.level = 1.0;
	cp_ptr.value = 1.0;
	AddControlPoint( cmew->color_map_editor.g.field[i]->map[0], 
            	cp_ptr.level, cp_ptr.value, True );
	SetArrayValues(cmew->color_map_editor.g.field[i]->map[0],
	   	cmew->color_map_editor.g.field[i]->line[0]);
	SetLineCoords(cmew->color_map_editor.g.field[i]->line[0]);
	}
    else
	{
	for (j=0;j < num_points; j++)
  	    {
	    if ( fscanf(fp,"%lg",&(cp_ptr.level)) < 0 )
		{
		message = XtMalloc(strlen(filename) + 64);
		strcpy(message,
		       "Error occurred while reading color map file \"");
		strcat(message, filename);
		strcat(message, "\".");
		XtWarning(message);
		XtFree(message);

		LoadHardCodeDefaults(cmew, init);
		return;
		}
	    if ( fscanf(fp,"%lg",&(cp_ptr.value)) < 0 )
		{
		message = XtMalloc(strlen(filename) + 64);
		strcpy(message,
		       "Error occurred while reading color map file \"");
		strcat(message, filename);
		strcat(message, "\".");
		XtWarning(message);
		XtFree(message);

		LoadHardCodeDefaults(cmew, init);
		return;
		}
	    if ( fscanf(fp,"%d",&(cp_ptr.wrap_around)) < 0 )
		{
		message = XtMalloc(strlen(filename) + 64);
		strcpy(message,
		       "Error occurred while reading color map file \"");
		strcat(message, filename);
		strcat(message, "\".");
		XtWarning(message);
		XtFree(message);

		LoadHardCodeDefaults(cmew, init);
		return;
		}
	    if ( fscanf(fp,"%d",&(cp_ptr.x)) < 0 )
		{
		message = XtMalloc(strlen(filename) + 64);
		strcpy(message,
		       "Error occurred while reading color map file \"");
		strcat(message, filename);
		strcat(message, "\".");
		XtWarning(message);
		XtFree(message);

		LoadHardCodeDefaults(cmew, init);
		return;
		}
	    if ( fscanf(fp,"%d",&(cp_ptr.y)) < 0 )
		{
		message = XtMalloc(strlen(filename) + 64);
		strcpy(message,
		       "Error occurred while reading color map file \"");
		strcat(message, filename);
		strcat(message, "\".");
		XtWarning(message);
		XtFree(message);

		LoadHardCodeDefaults(cmew, init);
		return;
		}
	    AddControlPoint( cmew->color_map_editor.g.field[i]->map[0], 
            	cp_ptr.level, cp_ptr.value, True );
	    SetArrayValues(cmew->color_map_editor.g.field[i]->map[0],
	   	cmew->color_map_editor.g.field[i]->line[0]);
	    SetLineCoords(cmew->color_map_editor.g.field[i]->line[0]);
            }
	}
      if (!init)
	{
        DrawControlBoxes(cmew->color_map_editor.g.field[i]->map[0],
		cmew->color_map_editor.g.field[i]->map[0]->gc);
        DrawLine(cmew->color_map_editor.g.field[i]->line[0],
		cmew->color_map_editor.g.field[i]->map[0]->line->gc);
        cmew->color_map_editor.g.field[i]->user_callback(cmew->color_map_editor.g.field[i]);
	}
  if (fclose(fp) != 0)
      {
      message = XtMalloc(strlen(filename) + 64);
      strcpy(message,
	     "Cannot close color map file \"");
      strcat(message, filename);
      strcat(message, "\".");
      XtWarning(message);
      XtFree(message);

      LoadHardCodeDefaults(cmew, init);
      return;
      }
}
void XmColorMapEditorLoad(Widget cmew, double min, double max, 
			  int nhues, double *hues,
			  int nsats, double *sats,
			  int nvals, double *vals,
			  int nopat, double *opat)
{
int i;
double norm, range=0;
Boolean skip_normalize;
#ifdef PASSDOUBLEVALUE
XtArgVal dx_l;
#endif

    XmColorMapEditorWidget w = (XmColorMapEditorWidget)cmew;

    /* Erase the old Control points */
    if (XtIsRealized((Widget)w))
    {
	for (i=0; i<4; i++)
	{
	    DrawControlBoxes(w->color_map_editor.g.field[i]->map[0],
		    w->color_map_editor.g.field[i]->map[0]->erase_gc);
	    DrawLine(w->color_map_editor.g.field[i]->map[0]->line,
		    w->color_map_editor.g.field[i]->map[0]->line->erase_gc);
	    RemoveAllControlPoints(w->color_map_editor.g.field[i]);
	    SetArrayValues(w->color_map_editor.g.field[i]->map[0],
	      w->color_map_editor.g.field[i]->line[0]);
	    SetLineCoords(w->color_map_editor.g.field[i]->line[0]);
	    DrawControlBoxes(w->color_map_editor.g.field[i]->map[0],
		    w->color_map_editor.g.field[i]->map[0]->gc);
	    DrawLine(w->color_map_editor.g.field[i]->line[0],
		    w->color_map_editor.g.field[i]->map[0]->line->gc);
	}
    }

    if(w->color_map_editor.min_editable)
    {
	w->color_map_editor.value_minimum = min;
	XtVaSetValues((Widget)w->color_map_editor.min_num, 
		XmNdValue, DoubleVal(min, dx_l), NULL);
    }
    if(w->color_map_editor.max_editable)
    {
	w->color_map_editor.value_maximum = max;
	XtVaSetValues((Widget)w->color_map_editor.max_num, 
		XmNdValue, DoubleVal(max, dx_l), NULL);
    }

    if(XtIsRealized((Widget)w))
	for(i = 0; i < 4; i++)
	    RemoveAllControlPoints(w->color_map_editor.g.field[i]);

#if Comment
    if (min == max) {
	skip_normalize = True;
    } else {
	skip_normalize = False;
        range = max - min;
    }
#else
    skip_normalize = True;
#endif

    for(i = 0; i < nhues; i++)
    {
	norm = (skip_normalize ? *hues : (*hues - min) / range) ;
	hues++;
	AddControlPoint(w->color_map_editor.g.field[HUE]->map[0], 
			norm,
			*(hues++), 
			True );
    }
    for(i = 0; i < nsats; i++)
    {
	norm = (skip_normalize ? *sats : (*sats - min) / range) ;
	sats++;
	AddControlPoint(w->color_map_editor.g.field[SATURATION]->map[0], 
			norm,	
			*(sats++), 
			True );
    }
    for(i = 0; i < nvals; i++)
    {
	norm = (skip_normalize ? *vals : (*vals - min) / range) ;
	vals++;
	AddControlPoint(w->color_map_editor.g.field[VALUE]->map[0], 
			norm,	
			*(vals++), 
			True );
    }
    for(i = 0; i < nopat; i++)
    {
	norm = (skip_normalize ? *opat : (*opat - min) / range) ;
	opat++;
	AddControlPoint(w->color_map_editor.g.field[OPACITY]->map[0], 
			norm,	
			*(opat++), 
			True );
    }

    if(XtIsRealized((Widget)w))
    {
	for(i = 0; i < 4; i++)
	{
	    SetArrayValues(w->color_map_editor.g.field[i]->map[0],
		w->color_map_editor.g.field[i]->line[0]);
	    SetLineCoords(w->color_map_editor.g.field[i]->line[0]);

	    DrawControlBoxes(w->color_map_editor.g.field[i]->map[0],
		    w->color_map_editor.g.field[i]->map[0]->gc);
	    DrawLine(w->color_map_editor.g.field[i]->line[0],
		    w->color_map_editor.g.field[i]->map[0]->line->gc);
	}
	w->color_map_editor.g.field[0]->user_callback(w->color_map_editor.g.field[0]);
    }
}

void RecalculateControlPointLevels(XmColorMapEditorWidget cmew)
{
int i, j;

    for(i = 0; i < 4; i++)
	{
	for (j = 0; j < cmew->color_map_editor.g.field[i]->map[0]->num_points;
	     			j++)
	    {
	    MoveControlPoint( 
			cmew->color_map_editor.g.field[i]->map[0], 
			j, 
			cmew->color_map_editor.g.field[i]->map[0]->points[j].x,
			cmew->color_map_editor.g.field[i]->map[0]->points[j].y);
	    }
	}
}
void CMESawToothWave(Widget w, int nsteps, Boolean start_on_left,
		     Boolean use_selected)
{
int 			i;
double 			level;
double 			value;
double 			tmp_value;
double 			level_inc;
XmColorMapEditorWidget 	cmew = (XmColorMapEditorWidget)w;
ControlMap 		*map;
double 			real_min_level;
double 			real_max_level;
int			num_selected;
int			pt_index1=0;
int			pt_index2=0;
int			field;
double			min_value;
double			max_value;
double			min_level;
double			max_level;

    PushUndoPoints(cmew);
    for(field = 0; field < 4; field++)
    {
	if(!cmew->color_map_editor.g.field[field]->map[0]->toggle_on) continue;
	map = cmew->color_map_editor.g.field[field]->map[0];
	num_selected = 0;
	for (i = 0; i < map->num_points; i++)
	    if(map->points[i].multi_point_grab)
	    {
		num_selected++;
		if(num_selected == 1)
		    pt_index1 = i;
		else if(num_selected > 1)
		    pt_index2 = i;
	    }
	if(use_selected && (num_selected < 2) )
	{
	    XtWarning("There must be at least 2 selected Control Points");
	    return;
	}

	/*
	 * Erase the control points and lines
	 */
	DrawControlBoxes(cmew->color_map_editor.g.field[field]->map[0],
		cmew->color_map_editor.g.field[field]->map[0]->erase_gc);
	DrawLine(cmew->color_map_editor.g.field[field]->map[0]->line,
		cmew->color_map_editor.g.field[field]->map[0]->line->erase_gc);

	if(use_selected)
	{
	    min_level = map->points[pt_index1].level;
	    min_value = map->points[pt_index1].value;
	    max_level = map->points[pt_index2].level;
	    max_value = map->points[pt_index2].value;
	}
	else
	{
	    min_level = 0.0;
	    max_level = 1.0;
	    min_value = 0.0;
	    if(field == 0)
		max_value = 0.66666;
	    else
		max_value = 1.0;
	}
	real_min_level = MIN(min_level, max_level);
	real_max_level = MAX(min_level, max_level);
	if(!use_selected)
	{
	    RemoveAllControlPoints(map->field);
	}
	else
	{
	    for (i = map->num_points - 1; i >= 0 ; i--)
	    {
		if ((map->points[i].level <= real_max_level) &&
		    (map->points[i].level >= real_min_level) )
		{
		    if( (i >= pt_index1) && (i <= pt_index2) ) 
			RemoveControlPoint(map, i);
		}
	    }
	}

	level_inc = (max_level - min_level)/(nsteps-1);

	/*
	 * Insure that the min_value < max_value
	 */
	if(min_value > max_value)
	{
	    tmp_value = min_value;
	    min_value = max_value;
	    max_value = tmp_value;
	}

	for(i = 0; i < nsteps; i++)
	    {
	    level = min_level + (i * level_inc);
	    if(start_on_left)
		value = min_value;
	    else
		value = max_value;
	    AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
		level, value, True );
	    if(start_on_left)
		value = max_value;
	    else
		value = min_value;
	    AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
		level, value, True );
	    }
	SetArrayValues(cmew->color_map_editor.g.field[field]->map[0],
	    cmew->color_map_editor.g.field[field]->line[0]);
	SetLineCoords(cmew->color_map_editor.g.field[field]->line[0]);
	DeselectAllPoints(cmew->color_map_editor.g.field[field]->map[0]);
	DrawControlBoxes(cmew->color_map_editor.g.field[field]->map[0],
		cmew->color_map_editor.g.field[field]->map[0]->gc);
	DrawLine(cmew->color_map_editor.g.field[field]->line[0],
		cmew->color_map_editor.g.field[field]->map[0]->line->gc);
    cmew->color_map_editor.g.field[field]->user_callback(cmew->color_map_editor.g.field[field]);
    }
    if(cmew->color_map_editor.g.field[0]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[1]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[2]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[3]->map[0]->toggle_on)
	CallActivateCallback(cmew, XmCR_MODIFIED);
    PrintAllControlPoints(cmew);
}

void CMESquareWave(Widget w, int nsteps, Boolean start_on_left,
		   Boolean use_selected)
{
int 			i;
double 			level;
double 			value;
double 			tmp_value;
double 			level_inc;
XmColorMapEditorWidget	cmew = (XmColorMapEditorWidget)w;
ControlMap 		*map=NULL;
double 			real_min_level=0;
double 			real_max_level=0;
int			num_selected;
int			pt_index1=0;
int			pt_index2=0;
int			field;
double			min_value=0;
double			max_value=0;
double			min_level=0;
double			max_level=0;

    PushUndoPoints(cmew);
    for(field = 0; field < 4; field++)
    {
	if(!cmew->color_map_editor.g.field[field]->map[0]->toggle_on) continue;
	map = cmew->color_map_editor.g.field[field]->map[0];
	num_selected = 0;
	for (i = 0; i < map->num_points; i++)
	    if(map->points[i].multi_point_grab)
	    {
		num_selected++;
		if(num_selected == 1)
		    pt_index1 = i;
		else if(num_selected > 1)
		    pt_index2 = i;
	    }
	if(use_selected && (num_selected < 2) )
	{
	    XtWarning("There must be at least 2 selected Control Points");
	    return;
	}

	if(use_selected)
	{
	    min_level = map->points[pt_index1].level;
	    min_value = map->points[pt_index1].value;
	    max_level = map->points[pt_index2].level;
	    max_value = map->points[pt_index2].value;
	}
	else
	{
	    min_level = 0.0;
	    max_level = 1.0;
	    min_value = 0.0;
	    if(field == 0)
		max_value = 0.66666;
	    else
		max_value = 1.0;
	}
	real_min_level = MIN(min_level, max_level);
	real_max_level = MAX(min_level, max_level);
	break;
    }
    /*
     * Erase the control points and lines
     */
    DrawControlBoxes(cmew->color_map_editor.g.field[field]->map[0],
	    cmew->color_map_editor.g.field[field]->map[0]->erase_gc);
    DrawLine(cmew->color_map_editor.g.field[field]->map[0]->line,
	    cmew->color_map_editor.g.field[field]->map[0]->line->erase_gc);

    if(!use_selected)
    {
	RemoveAllControlPoints(map->field);
    }
    else
    {
	for (i = map->num_points - 1; i >= 0 ; i--)
	{
	    if ((map->points[i].level <= real_max_level) &&
		(map->points[i].level >= real_min_level) )
	    {
		if( (i >= pt_index1) && (i <= pt_index2) ) 
		    RemoveControlPoint(map, i);
	    }
	}
    }
    level_inc = (max_level - min_level)/(nsteps-1);

    /*
     * Insure that the min_value < max_value
     */
    if(min_value > max_value)
    {
	tmp_value = min_value;
	min_value = max_value;
	max_value = tmp_value;
    }
    if(start_on_left)
	value = min_value;
    else
	value = max_value;
    for(i = 0; i < nsteps; i++)
	{
	level = min_level + (i * level_inc);
	AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
	    level, value, True );
	if(value == min_value)
	    {
		value = max_value;
	    }
	else
	    {
		value = min_value;
	    }
	AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
	    level, value, True );
	}
    SetArrayValues(cmew->color_map_editor.g.field[field]->map[0],
	cmew->color_map_editor.g.field[field]->line[0]);
    SetLineCoords(cmew->color_map_editor.g.field[field]->line[0]);
    DeselectAllPoints(cmew->color_map_editor.g.field[field]->map[0]);
    DrawControlBoxes(cmew->color_map_editor.g.field[field]->map[0],
	    cmew->color_map_editor.g.field[field]->map[0]->gc);
    DrawLine(cmew->color_map_editor.g.field[field]->line[0],
	    cmew->color_map_editor.g.field[field]->map[0]->line->gc);
    cmew->color_map_editor.g.field[field]->user_callback(cmew->color_map_editor.g.field[field]);
    if(cmew->color_map_editor.g.field[0]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[1]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[2]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[3]->map[0]->toggle_on)
	CallActivateCallback(cmew, XmCR_MODIFIED);
    PrintAllControlPoints(cmew);
}

void CMEStep(Widget w, int nsteps, Boolean use_selected)
{
int 			i;
double 			level;
double 			value;
double 			inc_level;
double 			inc_value;
XmColorMapEditorWidget	cmew = (XmColorMapEditorWidget)w;
ControlMap 		*map;
double 			real_min_level;
double 			real_max_level;
double 			real_min_value;
double 			real_max_value;
int			num_selected;
int			pt_index1=0;
int			pt_index2=0;
int			field;
double			min_value;
double			max_value;
double			min_level;
double			max_level;

    PushUndoPoints(cmew);
    for(field = 0; field < 4; field++)
    {
	if(!cmew->color_map_editor.g.field[field]->map[0]->toggle_on) continue;
	map = cmew->color_map_editor.g.field[field]->map[0];
	num_selected = 0;
	for (i = 0; i < map->num_points; i++)
	    if(map->points[i].multi_point_grab)
	    {
		num_selected++;
		if(num_selected == 1)
		    pt_index1 = i;
		else if(num_selected > 1)
		    pt_index2 = i;
	    }

	if( use_selected && (num_selected < 2) )
	{
	    XtWarning("There must be at least 2 selected Control Points");
	    return;
	}

	/*
	 * Erase the control points and lines
	 */
	DrawControlBoxes(cmew->color_map_editor.g.field[field]->map[0],
		cmew->color_map_editor.g.field[field]->map[0]->erase_gc);
	DrawLine(cmew->color_map_editor.g.field[field]->map[0]->line,
		cmew->color_map_editor.g.field[field]->map[0]->line->erase_gc);

	if(use_selected)
	{
	    min_level = map->points[pt_index1].level;
	    min_value = map->points[pt_index1].value;
	    max_level = map->points[pt_index2].level;
	    max_value = map->points[pt_index2].value;
	}
	else
	{
	    min_level = 0.0;
	    max_level = 1.0;
	    if(field == 0)
	    {
		min_value = 0.66666;
		max_value = 0.0;
	    }
	    else
	    {
		min_value = 0.0;
		max_value = 1.0;
	    }
	}
	real_min_level = MIN(min_level, max_level);
	real_max_level = MAX(min_level, max_level);
	real_min_value = MIN(min_value, max_value);
	real_max_value = MAX(min_value, max_value);
	if(!use_selected)
	{
	    RemoveAllControlPoints(map->field);
	}
	else
	{
	    for (i = map->num_points - 1; i >= 0 ; i--)
	    {
		if ((map->points[i].level <= real_max_level) &&
		    (map->points[i].level >= real_min_level) )
		{
		    if( (i >= pt_index1) && (i <= pt_index2) ) 
			RemoveControlPoint(map, i);
		}
	    }
	}
	
	level = min_level;
	value = min_value;
	inc_level = (max_level - min_level)/(nsteps+1);
	inc_value = (max_value - min_value)/nsteps;

	/*
	 * Add the first point
	 */
	AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
	    level, value, True );
	level += inc_level;

	for(i = 0; i < nsteps; i++)
	{
	    /*
	     * Deal with rounding errors.
	     */
	    if(level > real_max_level) level = real_max_level;
	    if(level < real_min_level) level = real_min_level;
	    /*
	     * If max_level < min_level, we have to reverse the order we add
	     * the points in at each level, since a point is considered to be
	     * "below" when added at the same level.
	     */
	    if(max_level < min_level)
		value += inc_value;
	    AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
		level, value, True );
	    if(max_level < min_level)
		value -= inc_value;
	    else
		value += inc_value;
	    if(value > real_max_value) value = real_max_value;
	    if(value < real_min_value) value = real_min_value;
	    AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
		level, value, True );
	    if(max_level < min_level)
		value += inc_value;
	    level += inc_level;
	}
	/*
	 * Add the last point
	 */
	AddControlPoint( cmew->color_map_editor.g.field[field]->map[0], 
	    max_level, value, False );

	SetArrayValues(cmew->color_map_editor.g.field[field]->map[0],
	    cmew->color_map_editor.g.field[field]->line[0]);
	SetLineCoords(cmew->color_map_editor.g.field[field]->line[0]);
	DeselectAllPoints(cmew->color_map_editor.g.field[field]->map[0]);
	DrawControlBoxes(cmew->color_map_editor.g.field[field]->map[0],
		cmew->color_map_editor.g.field[field]->map[0]->gc);
	DrawLine(cmew->color_map_editor.g.field[field]->line[0],
		cmew->color_map_editor.g.field[field]->map[0]->line->gc);
	cmew->color_map_editor.g.field[field]->user_callback(cmew->color_map_editor.g.field[field]);
    }
    if(cmew->color_map_editor.g.field[0]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[1]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[2]->map[0]->toggle_on ||
       cmew->color_map_editor.g.field[3]->map[0]->toggle_on)
	CallActivateCallback(cmew, XmCR_MODIFIED);
    PrintAllControlPoints(cmew);
}
void PrintAllControlPoints(XmColorMapEditorWidget cmew)
{
ControlMap *map;
int        i;
int        j;
Dimension  width;
Dimension  height;


    XtVaGetValues(cmew->color_map_editor.grid_w,
	XmNwidth, &width,
	XmNheight, &height,
	NULL);

    XClearArea(XtDisplay(cmew),
	       XtWindow(cmew->color_map_editor.grid_w),
	       0, 0, width, height, False);

    if(cmew->color_map_editor.print_points == XmPRINT_NONE) return;

    for(i = 0; i < 4; i++)
    {
	map = cmew->color_map_editor.g.field[i]->map[0];
	if(!map->toggle_on) continue;
	for(j = 0; j < map->num_points; j++)
	{
	    if(cmew->color_map_editor.print_points == XmPRINT_SELECTED)
	    {
		if( (map->points[j].multi_point_grab) ||
		    (j == map->grabbed) )
		{
		    PrintControlPoint(map, j);
		}
	    }
	    else if(cmew->color_map_editor.print_points == XmPRINT_ALL)
	    {
		PrintControlPoint(map, j);
	    }
	}
    }
}

void PrintControlPoint(ControlMap *map, int point_num)
{
int         direction;
int         ascent;
int         descent;
XCharStruct overall;
char	    string[256];
XmColorMapEditorWidget cmew;
Dimension   width;
Position    x;
Position    y;

    cmew = map->field->cmew;
    XtVaGetValues(cmew->color_map_editor.grid_w, 
	XmNwidth, &width, 
	NULL);

    sprintf(string, "%g", cmew->color_map_editor.value_minimum +
	(cmew->color_map_editor.value_maximum - 
	 cmew->color_map_editor.value_minimum) *
	 map->points[point_num].level);

    XTextExtents(cmew->color_map_editor.font, string, strlen(string),
		&direction, &ascent, &descent, &overall);
    x = (width - overall.width) - 5;
    y = map->points[point_num].y;
    if( (y - (ascent + descent)) < 0)
    {
	y = ascent + descent;
    }
    XDrawString(XtDisplay(cmew), 
		XtWindow(cmew->color_map_editor.grid_w), 
		cmew->color_map_editor.fontgc,
		x, 
		y, 
		string, strlen(string));

}

static Boolean PointExists(ControlMap *map, double level, double value)
{
int i;

    for(i = 0; i < map->num_points; i++)
    {
	if( (map->points[i].level == level) &&
	    (map->points[i].value == value) )
	    return True;
    }
    return False;
}
static void DeselectAllPoints(ControlMap *map)
{
int i;
    for(i = 0; i < map->num_points; i++)
	{
	map->points[i].multi_point_grab = False;
	}
    map->grabbed = -1;
}
