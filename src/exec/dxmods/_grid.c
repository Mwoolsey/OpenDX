/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "_grid.h"

#define TRUE 			1
#define FALSE 			0


typedef struct
{
    int			npos;
    int			ncons;
    Array		pos;
    Array		cons;
} GridOutputs;

static Error grid_point ();
static Error grid_line ();
static Error grid_rectangle ();
static Error grid_crosshair ();
static Error grid_ellipse ();
static Error grid_brick ();
static Error generate_crosshair_pos ();
static Error generate_ellipse_pos ();
static Error generate_crosshair_cons ();
static Error generate_ellipse_cons ();
static Error generate_field ();
/* CLEANUP FOR apos AND acons FOR ALL FUNCTIONS OCCURS IN generate_field() */

int _dxfGrid (float *point, char *structure, float *shape, int *density, 
          int dim, Object *outo)
{

    if (! strcmp (structure, "point"))
    {
	if (! grid_point (point, dim, outo))
	{
	    return (ERROR);
	}
    }
    else if (! strcmp (structure, "line"))
    {
	if (! grid_line (point, shape, density, dim, outo))
	{
	    return (ERROR);
	}
    }

    else if (! strcmp (structure, "rectangle"))
    {
	if (! grid_rectangle (point, shape, density, dim, outo))
	{
	    return (ERROR);
	}
    }

    else if (! strcmp (structure, "crosshair"))
    {
	if (! grid_crosshair (point, shape, density, dim, outo))
	{
	    return (ERROR);
	}
    }

    else if (! strcmp (structure, "ellipse"))
    {
	if (! grid_ellipse (point, shape, density, dim, outo))
	{
	    return (ERROR);
	}
    }

    else		/* brick */
    {
	if (! grid_brick (point, shape, density, dim, outo))
	{
	    return (ERROR);
	}
    }

    return (OK);
}

static Error grid_point (point, dim, outo)
    float		*point;
    Object		*outo;
    int                 dim;
{
    Array		apos;
    Field		f = NULL;
    int                 RETURN_CODE = ERROR;

    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (outo == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input outo");

    /*
     * First, generate the positions.
     */

    apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, dim);

    if (! DXAddArrayData (apos, 0, 1, (Pointer) point)) {
	DXDebug("G","DXAddArrayData for point.");
	goto cleanup;
    }



    if (! (f = DXNewField ())) {
	DXDebug("G","DXNewField for point.");
	goto cleanup;
    }

    if (! (DXSetComponentValue (f, "positions", (Object) apos))) {
	DXDebug("G","DXSetComponentValue for point.");
	goto cleanup;
    }

    apos = NULL;

    if (! DXChangedComponentValues (f, "positions")) {
	DXDebug("G","ChangedComponentValue for point.");
	goto cleanup;
    }

    if (! DXEndField (f)) {
	DXDebug("G","DXEndField for point.");
	goto cleanup;
    }

    *outo = (Object) f;

    f = NULL;
    RETURN_CODE = OK;

cleanup:
    DXDelete((Object) apos);
    DXDelete((Object) f);
    return RETURN_CODE;

}

static Error grid_line (point, shape, density, dim, outo)
    float		*point;
    float		*shape;
    int			*density, dim;
    Object		*outo;
{
    int			count[1];
    float		origin[3];
    float		delta[3];
    Array		apos;
    Array		acons;
    float		tmp;

    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (shape == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input shape");
    if (density == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input density");
    if (outo == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input outo");

    /*
     * Determine the number of positions and connections to generate.
     */

    count[0] = density[0];

    if (count[0] < 0)
    {
	DXErrorReturn (ERROR_BAD_PARAMETER, "density must be positive integers");
    }

    else if (count[0] == 0)
    {
	/*
	 * Empty grid.
	 */

	apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, dim);
	acons = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 2);
	if (! generate_field ("lines", apos, acons, outo))
	    return ERROR;
    }

    else if (count[0] == 1)
    {
	/*
	 * Single point.
	 */

	if (! grid_point (point, dim, outo))
	    return ERROR;
    }

    else 		/* (count[0] > 1) */
    {
	/*
	 * Now generate the origin point.
	 */

	origin[0] = point[0] - shape[0];
	origin[1] = point[1] - shape[1];
	if (dim==3) origin[2] = point[2] - shape[2];

	/*
	 * Now generate the delta.
	 */

	tmp =  2.0 / (count[0] - 1);		
	delta[0] = tmp * shape[0];
	delta[1] = tmp * shape[1];
	if (dim==3) delta[2] = tmp * shape[2];

	/*
	 * First, generate the positions.
	 */

	apos = (Array) DXNewRegularArray (TYPE_FLOAT, dim, count[0], 
				 (Pointer) origin, (Pointer) (delta + 0));

	/* 
	 * Next, generate the connections.
	 */

	acons = (Array) DXNewPathArray (count[0]);

	if (! generate_field ("lines", apos, acons, outo))
	    return ERROR;
    }

    return (OK);
} 

static Error grid_rectangle (point, shape, density, dim, outo)
    float		*point;
    float		*shape;
    int			*density;
    Object		*outo;
    int                 dim;
{
    int			count[2];
    float		origin[3], zero_origin[3];
    float		delta[6];
    Array		apos = NULL, apos0 = NULL, apos1 = NULL;
    Array		acons = NULL, acons0 = NULL, acons1 = NULL;
    float		tmp;
    float		t_shape[3];
    int			t_density[1];

    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (shape == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input shape");
    if (density == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input density");
    if (outo == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input outo");

    /*
     * Set up the zero origin for use in product arrays.
     */

    zero_origin[0] = zero_origin[1] = zero_origin[2] = 0.0;

    /*
     * Determine the number of positions and connections to generate.
     */

    count[0] = density[0];
    count[1] = density[1];

    if ((count[0] < 0) || (count[1] < 0))
    {
	DXErrorReturn (ERROR_BAD_PARAMETER, "density must be positive integers");
    }

    else if ((count[0] == 0) || (count[1] == 0))
    {
	/*
	 * Empty grid.
	 */

	apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, dim);
	acons = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 4);
	if (! generate_field ("quads", apos, acons, outo))
	    return ERROR;
    }

    else if (count[0] == 1)
    {
	/*
	 * Single line.
	 */

	t_shape[0] = shape[3];
	t_shape[1] = shape[4];
	t_shape[2] = shape[5];
	t_density[0] = density[1];

	if (! grid_line (point, t_shape, t_density, dim, outo))
	    return ERROR;
    }

    else if (count[1] == 1)
    {
	/*
	 * Single line.
	 */

	t_shape[0] = shape[0];
	t_shape[1] = shape[1];
	t_shape[2] = shape[2];
	t_density[0] = density[0];

	if (! grid_line (point, t_shape, t_density, dim, outo))
	    return ERROR;
    }

    else		/* both counts > 1 */
    {
	/*
	 * Now generate the origin point.
	 */

	origin[0] = point[0] - shape[0] - shape[3];
	origin[1] = point[1] - shape[1] - shape[4];
	origin[2] = point[2] - shape[2] - shape[5];

	/*
	 * Now generate the delta.
	 */

	tmp =  2.0 / (count[0] - 1);		
	delta[0] = tmp * shape[0];
	delta[1] = tmp * shape[1];
	delta[2] = tmp * shape[2];

	tmp =  2.0 / (count[1] - 1);		
	delta[3] = tmp * shape[3];
	delta[4] = tmp * shape[4];
	delta[5] = tmp * shape[5];

	/*
	 * First, generate the positions.
	 */

	apos0 = (Array) DXNewRegularArray (TYPE_FLOAT, dim, count[0], 
				 (Pointer) origin, (Pointer) (delta + 0));
	if (!apos0)
	    return ERROR;

	apos1 = (Array) DXNewRegularArray (TYPE_FLOAT, dim, count[1], 
				 (Pointer) zero_origin, (Pointer) (delta + 3));
	if (!apos1) {
	    DXDelete((Object) apos0);
	    return ERROR;
	}

	apos  = (Array) DXNewProductArray (2, apos0, apos1);

	/* 
	 * Next, generate the connections.
	 */

	acons0 = (Array) DXNewPathArray (count[0]);
	if (!acons0)
	    return ERROR;

	acons1 = (Array) DXNewPathArray (count[1]);
	if (!acons1) {
	    DXDelete((Object) acons0);
	    return ERROR;
	}

	acons  = (Array) DXNewMeshArray (2, acons0, acons1);

	/* create rectangle field for outo, if error DXDelete() apos and acons */
	if (! generate_field ("quads", apos, acons, outo))
	    return ERROR;
    }

    return (OK);
}

static Error grid_crosshair (point, shape, density, dim, outo)
    float		*point;
    float		*shape;
    int			*density;
    Object		*outo;
{
    int			count[3];
    float		origin[9];
    float		delta[9];
    Array		apos;
    Array		acons;
    float		*pos;
    int			*cons;
    float		tmp;
    float		line_shape[3];
    int			line_density[1];

    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (shape == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input shape");
    if (density == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input density");
    if (outo == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input outo");

    /*
     * Determine the number of positions and connections to generate.
     */

    count[0] = density[0];
    count[1] = density[1];
    count[2] = density[2];

    if ((count[0] <= 0) || (count[1] <= 0) || (count[2] <= 0))
    {
	DXErrorReturn (ERROR_BAD_PARAMETER, "density must be positive integers");
    } else if ((count[0] == 1) && (count[1] == 1) && (count[2] == 1)) {
	/*
	 *  Single Point - call grid_line and you are done
	 */
	 line_shape[0] = 1;
	 line_shape[1] = 1;
	 line_shape[2] = 1;
	 line_density[0] = 1;
	 if (!grid_line(point, line_shape, line_density, dim, outo))
	    return ERROR;

    }

    else		/* some count > 1 */
    {
	/*
	 * Ensure that all counts are odd numbers so that the crosshair
	 * may be centered about the origin.
	 */

	if (count[0] % 2 == 0)
	    (count[0])++;
	if (count[1] % 2 == 0)
	    (count[1])++;
	if (count[2] % 2 == 0)
	    (count[2])++;

	/*
	 * First, generate the positions.
	 */

	apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, 3);

	if (! DXAddArrayData (apos, 0, count[0] + count[1] + count[2] - 2, 
			    (Pointer) NULL))
            return ERROR;			       

	/*
	 * Set up the positions array.
	 */

	if (! (pos = (float *) DXGetArrayData (apos)))
	    return ERROR;

	/*
	 * Now generate the origin points.
	 */

	origin[0] = point[0] - shape[0];
	origin[1] = point[1] - shape[1];
	origin[2] = point[2] - shape[2];

	origin[3] = point[0] - shape[3];
	origin[4] = point[1] - shape[4];
	origin[5] = point[2] - shape[5];

	origin[6] = point[0] - shape[6];
	origin[7] = point[1] - shape[7];
	origin[8] = point[2] - shape[8];

	/*
	 * Now generate the delta.
	 */

	if (count[0] == 1)
	    tmp = 0;
	else
	    tmp =  2.0 / (count[0] - 1);		
	delta[0] = tmp * shape[0];
	delta[1] = tmp * shape[1];
	delta[2] = tmp * shape[2];

	if (count[1] == 1)
	    tmp = 0;
	else
	    tmp =  2.0 / (count[1] - 1);		
	delta[3] = tmp * shape[3];
	delta[4] = tmp * shape[4];
	delta[5] = tmp * shape[5];

	if (count[2] == 1)
	    tmp = 0;
	else
	    tmp =  2.0 / (count[2] - 1);		
	delta[6] = tmp * shape[6];
	delta[7] = tmp * shape[7];
	delta[8] = tmp * shape[8];

	if (! generate_crosshair_pos (pos, point, count, origin, delta))
	    return ERROR;
	
	/* 
	 * Next, generate the connections.
	 */

	acons = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 2);

	if (! DXAddArrayData (acons, 0, count[0] + count[1] + count[2] - 3,
	    (Pointer) NULL))
	    return ERROR;				   

	/*
	 * Set up the connections array.
	 */

	if (! (cons = (int *) DXGetArrayData (acons)))
	    return ERROR;

	if (! generate_crosshair_cons (cons, count))
	    return ERROR;

	if (! generate_field ("lines", apos, acons, outo))
	    return ERROR;
    }

    return (OK);
}

static Error grid_ellipse (point, shape, density, dim, outo)
    float		*point;
    float		*shape;
    int			*density;
    Object		*outo;
    int                 dim;
{
    int			count[1];
    Array		apos;
    Array		acons;
    float		*pos;
    int			*cons;

    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (shape == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input shape");
    if (density == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input density");
    if (outo == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input outo");

    /*
     * Determine the number of positions and connections to generate.
     */

    count[0] = density[0];

    if (count[0] < 0)
    {
	DXErrorReturn (ERROR_BAD_PARAMETER, "density must be positive integers");
    }

    else if (count[0] == 0)
    {
	/*
	 * Empty grid.
	 */

	apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, dim);
	acons = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 2);
	if (! generate_field ("lines", apos, acons, outo))
	    return ERROR;
    }

    else 		/* (count[0] > 0) */
    {
	/*
	 * First, generate the positions.
	 */

	apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, dim);

	if (! DXAddArrayData (apos, 0, count[0], (Pointer) NULL))
	    return ERROR;

	/*
	 * Set up the positions array.
	 */

	if (! (pos = (float *) DXGetArrayData (apos)))
	    return ERROR;

	if (! generate_ellipse_pos (pos, point, shape, count, dim))
	    return ERROR;
	
	/* 
	 * Next, generate the connections.
	 */

	acons = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 2);

	if (count[0] == 1)
	{
	    if (! grid_point (point, dim, outo))
		 return ERROR;
	}

	else
	{
	    if (! DXAddArrayData (acons, 0, count[0], (Pointer) NULL))
		 return ERROR;

	    /*
	     * Set up the connections array.
	     */

	    if (! (cons = (int *) DXGetArrayData (acons)))
		 return ERROR;

	    if (! generate_ellipse_cons (cons, count))
		 return ERROR;

	    if (! generate_field ("lines", apos, acons, outo))
		 return ERROR;
	}
    }

    return (OK);
} 

static Error grid_brick (point, shape, density, dim, outo)
    float		*point;
    float		*shape;
    int			*density;
    Object		*outo;
{
    int			count[3];
    float		origin[3], zero_origin[3];
    float		delta[9];
    Array		apos = NULL, apos0 = NULL, apos1 = NULL, apos2 = NULL;
    Array		acons = NULL,acons0 = NULL,acons1 = NULL,acons2 = NULL;
    float		tmp;
    float		t_shape[9];
    int			t_density[3];

    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (shape == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input shape");
    if (density == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input density");
    if (outo == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input outo");

    /*
     * Set up the zero origin for use in product arrays.
     */

    zero_origin[0] = zero_origin[1] = zero_origin[2] = 0.0;

    /*
     * Determine the number of positions and connections to generate.
     */

    count[0] = density[0];
    count[1] = density[1];
    count[2] = density[2];

    if ((count[0] < 0) || (count[1] < 0) || (count[2] < 0))
    {
	DXErrorReturn (ERROR_BAD_PARAMETER, "density must be positive integers");
    }

    else if ((count[0] == 0) || (count[1] == 0) || (count[2] == 0))
    {
	/*
	 * Empty grid.
	 */

	apos = DXNewArray (TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	acons = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 8);
	if (! generate_field ("cubes", apos, acons, outo))
	    return ERROR;
    }

    else if (count[0] == 1)
    {
	/*
	 * Single rectangle.
	 */

	t_shape[0] = shape[3];
	t_shape[1] = shape[4];
	t_shape[2] = shape[5];
	t_shape[3] = shape[6];
	t_shape[4] = shape[7];
	t_shape[5] = shape[8];
	t_density[0] = density[1];
	t_density[1] = density[2];

	if (! grid_rectangle (point, t_shape, t_density, dim, outo))
	     return ERROR;
    }

    else if (count[1] == 1)
    {
	/*
	 * Single rectangle.
	 */

	t_shape[0] = shape[0];
	t_shape[1] = shape[1];
	t_shape[2] = shape[2];
	t_shape[3] = shape[6];
	t_shape[4] = shape[7];
	t_shape[5] = shape[8];
	t_density[0] = density[0];
	t_density[1] = density[2];

	if (! grid_rectangle (point, t_shape, t_density, dim, outo))
	    return ERROR;
    }

    else if (count[2] == 1)
    {
	/*
	 * Single rectangle.
	 */

	t_shape[0] = shape[0];
	t_shape[1] = shape[1];
	t_shape[2] = shape[2];
	t_shape[3] = shape[3];
	t_shape[4] = shape[4];
	t_shape[5] = shape[5];
	t_density[0] = density[0];
	t_density[1] = density[1];

	if (! grid_rectangle (point, t_shape, t_density, dim, outo))
	    return ERROR;
    }

    else		/* all counts > 1 */
    {
	/*
	 * Now generate the origin point.
	 */

	origin[0] = point[0] - shape[0] - shape[3] - shape[6];
	origin[1] = point[1] - shape[1] - shape[4] - shape[7];
	origin[2] = point[2] - shape[2] - shape[5] - shape[8];

	/*
	 * Now generate the delta.
	 */

	tmp =  2.0 / (count[0] - 1);		
	delta[0] = tmp * shape[0];
	delta[1] = tmp * shape[1];
	delta[2] = tmp * shape[2];

	tmp =  2.0 / (count[1] - 1);		
	delta[3] = tmp * shape[3];
	delta[4] = tmp * shape[4];
	delta[5] = tmp * shape[5];

	tmp =  2.0 / (count[2] - 1);		
	delta[6] = tmp * shape[6];
	delta[7] = tmp * shape[7];
	delta[8] = tmp * shape[8];

	/*
	 * First, generate the positions.
	 */

	apos0 = (Array) DXNewRegularArray (TYPE_FLOAT, 3, count[0], 
				 (Pointer) origin, (Pointer) (delta + 0));
	if (!apos0) 
	    return ERROR;

	apos1 = (Array) DXNewRegularArray (TYPE_FLOAT, 3, count[1], 
				 (Pointer) zero_origin, (Pointer) (delta + 3));
	if (!apos1) {
	    DXDelete((Object) apos0);
	    return ERROR;
	}

	apos2 = (Array) DXNewRegularArray (TYPE_FLOAT, 3, count[2], 
				 (Pointer) zero_origin, (Pointer) (delta + 6));
	if (!apos2) {
	    DXDelete((Object) apos0);
	    DXDelete((Object) apos1);
	    return ERROR;
	}

	apos  = (Array) DXNewProductArray (3, apos0, apos1, apos2);

	/* 
	 * Next, generate the connections.
	 */

	acons0 = (Array) DXNewPathArray (count[0]);
	if (!acons0) 
	    return ERROR;

	acons1 = (Array) DXNewPathArray (count[1]);
	if (!acons1) {
	    DXDelete((Object) acons0);
	    return ERROR;
	}

	acons2 = (Array) DXNewPathArray (count[2]);
	if (!acons2) {
	    DXDelete((Object) acons0);
	    DXDelete((Object) acons1);
	    return ERROR;
	}

	acons  = (Array) DXNewMeshArray (3, acons0, acons1, acons2);

	/* create brick field for outo, if error, DXDelete() apos and acons */
	if (! generate_field ("cubes", apos, acons, outo))
	    return ERROR;
    }

    return (OK);
}


static Error generate_crosshair_pos (pos, point, count, origin, delta)
    float		*pos;
    float		*point;
    int			*count;
    float		*origin;
    float		*delta;
{
    int			i;
    int			index;
    int			midcount;

    if (pos == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input pos");
    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (count == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input count");
    if (origin == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input origin");
    if (delta == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input delta");

    /*
     * Assumes that count[0] >= 1, count[1] >= 1, count[2] >= 1.
     */

    if (count[0] % 2 == 0)
	(count[0])++;
    if (count[1] % 2 == 0)
	(count[1])++;
    if (count[2] % 2 == 0)
	(count[2])++;

    pos[0] = point[0];
    pos[1] = point[1];
    pos[2] = point[2];

    index = 3;

    midcount = (count[0] - 1) / 2;

    for (i = 0; i < count[0]; i++)
    {
	if (i != midcount)
	{
	    pos[index]     = origin[0] + i * delta[0];
	    pos[index + 1] = origin[1] + i * delta[1];
	    pos[index + 2] = origin[2] + i * delta[2];

	    index += 3;
	}
    }

    midcount = (count[1] - 1) / 2;

    for (i = 0; i < count[1]; i++)
    {
	if (i != midcount)
	{
	    pos[index]     = origin[3] + i * delta[3];
	    pos[index + 1] = origin[4] + i * delta[4];
	    pos[index + 2] = origin[5] + i * delta[5];

	    index += 3;
	}
    }

    midcount = (count[2] - 1) / 2;

    for (i = 0; i < count[2]; i++)
    {
	if (i != midcount)
	{
	    pos[index]     = origin[6] + i * delta[6];
	    pos[index + 1] = origin[7] + i * delta[7];
	    pos[index + 2] = origin[8] + i * delta[8];

	    index += 3;
	}
    }

    return (OK);
}

static Error generate_ellipse_pos (pos, point, shape, count, dim)
    float		*pos;
    float		*point;
    float		*shape;
    int			*count;
{
    double		delta;
    int			i;
    int			index;
    float		cosval;
    float		sinval;

    if (pos == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input pos");
    if (point == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input point");
    if (shape == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input shape");
    if (count == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input count");

    /*
     * Assumes that the two vectors which form "shape" are orthogonal.
     */

    delta = (double) (2.0 * M_PI / count[0]);

    for (i = 0; i < count[0]; i++)
    {
	index = dim * i;
	cosval = (float) cos (i * delta);
	sinval = (float) sin (i * delta);

	pos[index    ] = point[0] + shape[0] * cosval + shape[3] * sinval;
	pos[index + 1] = point[1] + shape[1] * cosval + shape[4] * sinval;
        if (dim==3)
	 pos[index + 2] = point[2] + shape[2] * cosval + shape[5] * sinval;
    }

    return (OK);
}

static Error generate_crosshair_cons (cons, count)
    int			*cons;
    int			*count;
{
    int			i;
    int			index;
    int			midcount;
    int			pos;
    int			inloop;

    if (cons == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input cons");
    if (count == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input count");

    /* 
     * This assumes that count[0] >= 1, count[1] >= 1, and count[2] >= 1.
     */

    if (count[0] % 2 == 0)
	(count[0])++;
    if (count[1] % 2 == 0)
	(count[1])++;
    if (count[2] % 2 == 0)
	(count[2])++;

    index = 0;
    pos = 1;
    inloop = FALSE;

    midcount = (count[0] - 1) / 2;

    for (i = 0; i < count[0] - 1; i++)
    {
	inloop = TRUE;

	if (i == midcount - 1)
	{
	    cons[index    ] = pos;
	    cons[index + 1] = 0;
	}

	else if (i == midcount)
	{
	    cons[index    ] = 0;
	    cons[index + 1] = pos + 1;
	    pos++;
	}

	else
	{
	    cons[index    ] = pos;
	    cons[index + 1] = pos + 1;
	    pos++;
	}

	index += 2;
    }

    if (inloop)
	pos++;

    inloop = FALSE;

    midcount = (count[1] - 1) / 2;

    for (i = 0; i < count[1] - 1; i++)
    {
	inloop = TRUE;

	if (i == midcount - 1)
	{
	    cons[index    ] = pos;
	    cons[index + 1] = 0;
	}

	else if (i == midcount)
	{
	    cons[index    ] = 0;
	    cons[index + 1] = pos + 1;
	    pos++;
	}

	else
	{
	    cons[index    ] = pos;
	    cons[index + 1] = pos + 1;
	    pos++;
	}

	index += 2;
    }

    if (inloop)
	pos++;

    midcount = (count[2] - 1) / 2;

    for (i = 0; i < count[2] - 1; i++)
    {
	if (i == midcount - 1)
	{
	    cons[index    ] = pos;
	    cons[index + 1] = 0;
	}

	else if (i == midcount)
	{
	    cons[index    ] = 0;
	    cons[index + 1] = pos + 1;
	    pos++;
	}

	else
	{
	    cons[index    ] = pos;
	    cons[index + 1] = pos + 1;
	    pos++;
	}

	index += 2;
    }

    return (OK);
}

static Error generate_ellipse_cons (cons, count)
    int			*cons;
    int			*count;
{
    int			i;
    int			index;

    if (cons == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input cons");
    if (count == NULL)
	DXErrorReturn (ERROR_BAD_PARAMETER, "missing input count");

    /* 
     * This assumes that count[0] > 1.
     */

    for (i = 0; i < count[0]; i++)
    {
	index = 2 * i;

	cons[index    ] = i;
	cons[index + 1] = ((i + 1 == count[0]) ? 0 : i + 1);
    }

    return (OK);
}

static Error generate_field (element_type, apos, acons, outo)
    char		*element_type;
    Array		apos;
    Array		acons;
    Object		*outo;
{
    Field		f = NULL;
    int                 RETURN_CODE = ERROR;

    if (element_type == NULL) {
	DXSetError(ERROR_BAD_PARAMETER, "missing input element_type");
	goto cleanup;
    }
    if (apos == NULL) {
	DXDebug("G","apos is null in generate_field");
	goto cleanup;
    }
    if (acons == NULL) {
	DXDebug("G","acons is null in generate_field");
	goto cleanup;
    }
    if (outo == NULL) {
	DXSetError(ERROR_BAD_PARAMETER, "missing input outo");
	goto cleanup;
    }

    if (! (f = DXNewField ())) {
	DXDebug("G","DXNewField in generate_field");
	goto cleanup;
    }

    if (! (DXSetComponentValue (f, "positions", (Object) apos))) {
	DXDebug("G","DXSetComponentValue for apos in generate_field");
	goto cleanup;
    }
    apos = NULL;

    if (! (DXSetComponentValue (f, "connections", (Object) acons))) {
	DXDebug("G","DXSetComponentValue for acons in generate_field");
	goto cleanup;
    }
    acons = NULL;

    if (! (DXSetComponentAttribute (f, "connections", "element type",
			          (Object) DXNewString (element_type)))) {
	DXDebug("G","DXSetComponentAttribute for acons in generate_field");
	goto cleanup;				   
    }

    if (! DXChangedComponentValues (f, "positions")) {
	DXDebug("G","DXChangedComponentValues for positions in generate_field");
	goto cleanup;
    }
    if (! DXChangedComponentValues (f, "connections")) {
	DXDebug("G","DXChangedComponentValues for connections in generate_field");
	goto cleanup;
    }

    if (! DXEndField (f)) {
	DXDebug("G","DXEndField in generate_field");
	goto cleanup;
    }

    *outo = (Object) f;

    f = NULL;
    RETURN_CODE = OK;

cleanup:
    DXDelete((Object) apos);
    DXDelete((Object) acons);
    DXDelete((Object) f);
    return RETURN_CODE;

}
