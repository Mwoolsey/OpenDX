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
#include "ControlField.h"
#include "ColorMapEditorP.h"


/*
 *  Declare templates for locally defined and used subroutines
 */
static int GetLineRange( ControlLine* line, double level_1, double level_2,
			 int* first_index );
static void InterpolateAlongArray( ControlLine* line, int start_index,
				   int count,
				   double level_1, double level_2,
				   double value_1, double value_2 );
static void FillAlongArray( ControlLine* line, int start_index,
			    int count, double value );


/*  Subroutine:	SetArrayValues
 *  Purpose:	Set the proper coordinates to draw the line as per the control
 *		points
 */
void SetArrayValues( ControlMap* map, ControlLine* line )
{
    double value;
    int index, count;
    int control_point;

    /*  Set mark count to 0 (count used to mark redundantly defined levels)  */
    for( index=0; index<line->field->num_levels; index++ )
	line->load[index] = 0;
    if( map->num_points == 0 )
    {
	FillAlongArray(line, 0, line->field->num_levels, 1.0);
	return;
    }
    /*  Points are always ordered lowest level to highest  */
    for( control_point = 0;
	    (control_point < map->num_points)
	 && (map->points[control_point].level < 0.0);
	 control_point++ );

    /*  Fill between beginning of graph and first control point (if needed)  */
    if( (control_point == 0) && (map->points[control_point].level > 0.0) )
    {
	count = GetLineRange(line, 0.0, map->points[0].level, &index);
	if( count > 0 )
	{
	    if( map->end_policy == FILL_ZERO )
		FillAlongArray(line, index, count, 0.0);
	    else if( map->end_policy == FILL_ONE )
		FillAlongArray(line, index, count, 1.0);
	    else
	    {
		if( (value = map->points[control_point].value) > 1.0 )
		    value -= (double)((int)value);
		else if( value < 0.0 )
		    value -= ((double)((int)value) - 1);
		FillAlongArray(line, index, count, value);
	    }
	    for( count += index; index < count; index++ )
		++line->load[index];
	    if( map->end_policy != FILL_LEVEL )
		++line->load[index - 1];
	}
	else if( count == 0 )
	    line->load[index] += 2;
    }
    /*  Fill between control points  */
    while(  (++control_point < map->num_points)
	 && (map->points[control_point].level <= 1.0) )
    {
	count = GetLineRange(line, map->points[control_point-1].level,
			     map->points[control_point].level, &index);
				 
	if( count > 0 )
	{
	    InterpolateAlongArray(line, index, count,
				 map->points[control_point-1].level,
				 map->points[control_point].level,
				 map->points[control_point-1].value,
				 map->points[control_point].value);
	    for( count += index; index < count; index++ )
		++line->load[index];
	}
	else if( count == 0 )
	    ++line->load[index];
    }
    /*  Fill between end of graph and last control point (if needed)  */
    if(   (control_point >= map->num_points)
       && (map->points[--control_point].level <= 1.0) )
    {
	count = GetLineRange(line, map->points[control_point].level,
			     1.0, &index);
	if( count > 0 )
	{
	    if( map->end_policy == FILL_ZERO )
	    {
		FillAlongArray(line, index, count, 0.0);
		++line->load[index];
	    }
	    else if( map->end_policy == FILL_ONE )
	    {
		FillAlongArray(line, index, count, 0.0);
		++line->load[index];
	    }
	    else
	    {
		if( (value = map->points[control_point].value) > 1.0 )
		    value -= (double)((int)value);
		else if( value < 0.0 )
		    value -= ((double)((int)value) - 1);
		FillAlongArray(line, index, count, value);
	    }
	    for( count += index; index < count; index++ )
		++line->load[index];
	}
	else if( count == 0 )
	    line->load[index] += 2;
    }

    for(control_point = 0; control_point < map->num_points; control_point++)
    {
	index = map->points[control_point].level*(NUM_LEVELS-1) + 0.5;
	line->values[index] = map->points[control_point].value;
    }
}


/*  Subroutine:	GetLineRange
 *  Purpose:	Determine which line points fall between the two levels
 *  Returns:	The number of points between the levels (inclusive)
 */
static int GetLineRange( ControlLine* line, double level_1, double level_2,
			 int* first_index )
{
    double d_first, d_last;
    int last;

    /*  Choose the index of the first point (above-inclusive)  */
    if( level_1 > 0.0 )
    {
	d_first = level_1 * (double)line->last_level_index;
	*first_index = (int)ceil(d_first);
    }
    else
	*first_index = 0;
    /*  If segment would be beyond graph range, return  */
    if( *first_index > line->last_level_index )
	return -1;
    /*  Choose the index of the last point (below-inclusive)  */
    if( level_2 <= 1.0 )
    {
	d_last = level_2 * (double)line->last_level_index;
	last = (int)floor(d_last);
    }
    else
	last = line->last_level_index;
    if( last < 0 )
	/*  Before the graph range  */
	return -1;
    else if( last < *first_index )
	/*  Does not span any level centers  */
	return 0;
    else
	return (1 + last - *first_index);
}


/*  Subroutine:	InterpolateAlongArray
 *  Purpose:	Set the "value" coordinates of a list of line points by ...
 *		interpolating between the two control points
 *  Note:	Values need not be in the range of 0-1, but levels must
 */
static void InterpolateAlongArray( ControlLine* line, int start_index,
				   int count,
				   double level_1, double level_2,
				   double value_1, double value_2 )
{
    register double current_value;
    register double* value_entry;
    double level_at_start, level_at_end;
    double value_at_end;
    double value_increment;
    double* end_value_entry;

    /*  Set pointer to before first point on line in this range  */
    value_entry = &line->values[start_index - 1];

    /*  Determine level at first point and interpolate value at that level  */
    level_at_start = (double)start_index / (double)line->last_level_index;
    if (level_2 != level_1)
    {
	current_value = value_1 +
	  (((level_at_start - level_1) * (value_2 - value_1))
	   / (level_2 - level_1));
    }
    else
    {
	current_value = value_1;
    }
    /*  If there is only one point, avoid excess work  */
    if( count == 1 )
	{
	value_entry++;
	*value_entry = current_value;
	}
    else
    {
	/*  Determine level and interpolate value for the last point  */
	level_at_end =
	  (double)(start_index + count - 1) / (double)line->last_level_index;
	value_at_end = value_1 +
	  (((level_at_end - level_1) * (value_2 - value_1))
	   / (level_2 - level_1));
	value_increment = (value_at_end - current_value) / (double)count;
	/*  Determine pointer at end for use as loop counter		     *
	 *  Remember that value_entry starts out one less than actual start  */
	end_value_entry = value_entry + count;
	while( ++value_entry <= end_value_entry )
	{
	    if( current_value > 1.0 )
	    {
		++line->load[value_entry - line->values];
		current_value -= (double)((int)current_value);
	    }
	    else if( current_value < 0.0 )
	    {
		++line->load[value_entry - line->values];
		current_value -= ((double)((int)current_value) - 1);
	    }
	    *value_entry = current_value;
	    current_value += value_increment;
	}
    }
}


/*  Subroutine:	FillAlongArray
 *  Purpose:	Set all of the line values in the given range to the given value
 */
static void FillAlongArray( ControlLine* line, int start_index,
			    int count, double value )
{
    double* end_value_entry;
    register double* value_entry;

    /*  Set pointer to first point on line in this range  */
    value_entry = &line->values[start_index - 1];
    /*  Don't forget that pointer starts back one from where it should be  */
    end_value_entry = value_entry + count + 1;
    while( ++value_entry < end_value_entry )
	*value_entry = value;
}
