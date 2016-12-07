/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include "../base/defines.h"
#include "ControlField.h"
#include "Color.h"

void SetColorCoords( XColor *cells, ControlField* field,
			    short* load, short load_limit );

void SetRGBLines( XColor* cells, ControlField* field,
		  ControlLine* h, ControlLine* s, ControlLine* v )
{
    int i;

    short* load = field->line[0]->load;
    for( i=0; i<field->line[0]->last_level_index; i++ )
	load[i] = h->load[i] + s->load[i] + v->load[i];
    DrawLine(field->line[0], field->line[0]->erase_gc);
    DrawLine(field->line[1], field->line[1]->erase_gc);
    DrawLine(field->line[2], field->line[2]->erase_gc);
    SetColorCoords(cells, field, load, 3);
    DrawLine(field->line[0], field->line[0]->gc);	/* Draw new lines */
    DrawLine(field->line[1], field->line[1]->gc);
    DrawLine(field->line[2], field->line[2]->gc);
}

/*  Subroutine:	SetColorCoords
 *  Purpose:	Set the proper coordinates to draw the RGB lines as per the 
 *		Color Cells
 */
void SetColorCoords( XColor *cells, ControlField* field,
			    short* load, short load_limit )
{
    LineCoord* coord;
    struct ControlLineRec *red, *green, *blue;
    int i;
    float factor;
    Boolean prior_overload, current_overload, next_overload;
    register XPoint *red_point, *green_point, *blue_point;

    red = field->line[0];
    green = field->line[1];
    blue = field->line[2];
    red_point = red->points;
    green_point = green->points;
    blue_point = blue->points;

    coord = field->coords;
    if( field->vertical )
    {
	short margin = field->left_margin;

	factor = 255.0 / (float)(field->width - 1);
	prior_overload = FALSE;
	current_overload = (load[0] > load_limit);
	for( i=0; i<red->last_level_index; i++ )
	{
	    register short red_coord, green_coord, blue_coord;

	    next_overload = (load[i+1] > load_limit);
	    red_coord = ((float)cells[i].red / factor) + margin;
	    green_coord = ((float)cells[i].green / factor) + margin;
	    blue_coord = ((float)cells[i].blue / factor) + margin;
	    red_point->x = red_coord;
	    green_point->x = green_coord;
	    blue_point->x = blue_coord;
	    if( prior_overload || current_overload )
	    {
		red_point->y =
		  green_point->y =
		    blue_point->y = coord[i].low;
	    }
	    else
	    {
		red_point->y =
		  green_point->y =
		    blue_point->y = coord[i].mid;
	    }
	    ++red_point;
	    ++green_point;
	    ++blue_point;
	    if( current_overload || next_overload )
	    {
		red_point->y =
		  green_point->y =
		    blue_point->y = coord[i].high;
		red_point->x = red_coord;
		++red_point;
		green_point->x = green_coord;
		++green_point;
		blue_point->x = blue_coord;
		++blue_point;
	    }
	    prior_overload = current_overload;
	    current_overload = next_overload;
	}
    }
    else
    {
	short margin = field->top_margin;

	factor = 256 / (field->height - 1);
	prior_overload = FALSE;
	current_overload = (load[0] > load_limit);
	for( i=0; i<red->last_level_index; i++ )
	{
	    register short red_coord, green_coord, blue_coord;

	    next_overload = (load[i+1] > load_limit);
	    red_coord = ((65535 - (int)cells[i].red) / factor) + margin;
	    green_coord = ((65535 - (int)cells[i].green) / factor) + margin;
	    blue_coord = ((65535 - (int)cells[i].blue) / factor) + margin;
	    red_point->y = red_coord;
	    blue_point->y = blue_coord;
	    green_point->y = green_coord;
	    if( prior_overload || current_overload )
	    {
		red_point->x =
		  green_point->x =
		    blue_point->x = coord[i].low;
	    }
	    else
	    {
		red_point->x =
		  green_point->x =
		    blue_point->x = coord[i].mid;
	    }
	    ++red_point;
	    ++green_point;
	    ++blue_point;
	    if( current_overload || next_overload )
	    {
		red_point->x =
		  green_point->x =
		    blue_point->x = coord[i].high;
		red_point->y = red_coord;
		++red_point;
		green_point->y = green_coord;
		++green_point;
		blue_point->y = blue_coord;
		++blue_point;
	    }
	    prior_overload = current_overload;
	    current_overload = next_overload;
	}
    }
    red->num_points =
      green->num_points = 
        blue->num_points = red_point - red->points;
}
