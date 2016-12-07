/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/dxmods/RCS/grid.m,v 5.0 92/11/12 09:15:18 svs Exp Locker: gresh 
 * 
 */

/***
MODULE:		
    Grid
SHORTDESCRIPTION:
    Produces a set of points on a grid
CATEGORY:
    Realization
INPUTS:
    point;     vector;         NULL;    point around which to build the grid
    structure; string;         brick;   shape of the grid
    shape;     vector list;    structure dependent; size and shape of structure
    density;   integer list;     3;      density of grid
OUTPUTS:
    grid;      geometry field; NULL;    the resulting grid
FLAGS:
BUGS:
AUTHOR:
    Brian P. O'Toole, Charles R. Clark
END:
***/
#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_grid.h"

static Error grid_point (float *, Object, int *);
static Error grid_structure (char **, Object, int);
static Error grid_shape (float **, char *, Object, int, int *);
static Error grid_density (int **, char *, Object);

int m_Grid (in, out)
    Object		*in;
    Object		*out;
{
    Object		ino0, ino1, ino2, ino3;
    float		point[3];
    char		*structure;
    float		*shape;
    int			*density;
    int			RETURN_CODE = ERROR;
    int                 dim_point, dim_structure;

    out[0] = NULL;
    structure = NULL;
    shape = NULL;
    density = NULL;

    ino0 = in[0];
    ino1 = in[1];
    ino2 = in[2];
    ino3 = in[3];

    if (ino0 == NULL) {
	DXSetError (ERROR_BAD_PARAMETER, "#10000", "point");
        return ERROR;
    }

    if (! grid_point (point, ino0, &dim_point))
	goto cleanup;

    if (! grid_structure (&structure, ino1, dim_point))
	goto cleanup;

    if (! grid_shape (&shape, structure, ino2, dim_point, &dim_structure))
	goto cleanup;

    if (! grid_density (&density, structure, ino3))
	goto cleanup;

    if (dim_point != dim_structure) {
        DXSetError (ERROR_BAD_PARAMETER, "#11826","dimensionality of point",
                    "dimensionality of shape");
        goto cleanup;
    }
    if ((dim_point != 2)&&(dim_point != 3)) {
       DXSetError(ERROR_BAD_PARAMETER,"#11362","point and shape", 2, 3);
       goto cleanup;
    }
    if (! _dxfGrid (point, structure, shape, density, dim_point, &out[0])) {
	goto cleanup;
    }

    RETURN_CODE = OK;
    DXFree((Pointer) structure);
    DXFree((Pointer) shape);
    DXFree((Pointer) density);
    return RETURN_CODE;

cleanup:
    DXFree((Pointer) structure);
    DXFree((Pointer) shape);
    DXFree((Pointer) density);
    out[0]=NULL;
    return RETURN_CODE;

}

static Error grid_point (float *point, Object ino, int *dim)
{
    float tmp_point[3];

    if (! point) 
	DXErrorReturn (ERROR_UNEXPECTED, "point NULL");
    if (! ino)
	DXErrorReturn (ERROR_UNEXPECTED, "ino NULL");

    if (DXExtractParameter (ino, TYPE_FLOAT, 2, 1, (Pointer) tmp_point)) {
	point[0] = tmp_point[0];
	point[1] = tmp_point[1];
	point[2] = 0.0;
        *dim = 2;
    } else if (DXExtractParameter (ino, TYPE_FLOAT, 3, 1, (Pointer) tmp_point)) {
	point[0] = tmp_point[0];
	point[1] = tmp_point[1];
	point[2] = tmp_point[2];
        *dim = 3;
    } else {
	DXSetError(ERROR_BAD_PARAMETER, "#10231","point", 2, 3);
	return (ERROR);
    }

    return (OK);
}

static Error grid_structure (char **structure, Object ino, int dim)
{
    char		*str;

    if (! structure)
	DXErrorReturn (ERROR_UNEXPECTED, "structure NULL");

    if (ino == NULL) {
	str = "brick";
        if (dim==2) {
         DXSetError(ERROR_BAD_PARAMETER,"#11842","brick structure",2,"point");
         return ERROR;
        }
    }

    else
    {
	if (! DXExtractString (ino, &str)) {
	    DXSetError (ERROR_BAD_PARAMETER, "#10200","structure");
            return ERROR;
        }
    }

    if (! (*structure = (char *) DXAllocate (strlen (str) + 1)))
	return ERROR;
    strcpy (*structure, str);

    if ((strcmp (*structure, "line")) &&
	(strcmp (*structure, "rectangle")) &&
	(strcmp (*structure, "crosshair")) &&
	(strcmp (*structure, "ellipse")) &&
	(strcmp (*structure, "point")) &&
	(strcmp (*structure, "brick")))
    {
	DXSetError (ERROR_BAD_PARAMETER, "#10203","structure",
                    "line, rectangle, crosshair, ellipse, brick");
	return (ERROR);
    }
    if (!strcmp(*structure,"crosshair")) {
        if (dim==2) {
         DXSetError(ERROR_BAD_PARAMETER,"#11842","crosshair",2,"point");
         return ERROR;
        }
    }
    if (!strcmp(*structure,"brick")) {
        if (dim==2) {
           DXSetError(ERROR_BAD_PARAMETER,"#11842","brick",2,"point");
           return ERROR;
         }
    }

    return (OK);
}

static Error grid_shape (float **shape, char *structure, Object ino, 
                         int dim_p, int *dim_s)
{
    float tmp_line_shape[3], tmp_line_shape_2D[2];
    float tmp_rectangle_shape[6], tmp_rectangle_shape_2D[4];
    float tmp_crosshair_shape[9];
    float tmp_ellipse_shape[6], tmp_ellipse_shape_2D[4];
    float tmp_brick_shape[9];

    if (! shape)
	DXErrorReturn (ERROR_UNEXPECTED, "shape NULL");
    if (! structure)
	DXErrorReturn (ERROR_UNEXPECTED, "structure NULL");

    if (! strcmp (structure, "line"))
    {
	if (! (*shape = (float *) DXAllocate (3 * sizeof (float))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*shape)[0] = 1.0;
	    (*shape)[1] = 0.0;
	    (*shape)[2] = 0.0;
            *dim_s = dim_p;
	} else {
	    if (DXExtractParameter (ino, TYPE_FLOAT, 2, 1, 
		(Pointer) tmp_line_shape_2D)) {
	        (*shape)[0] = tmp_line_shape_2D[0];
	        (*shape)[1] = tmp_line_shape_2D[1];
	        (*shape)[2] = 0.0;
                *dim_s = 2;
	    } else if (DXExtractParameter(ino, TYPE_FLOAT, 3, 1, 
		(Pointer) tmp_line_shape)) {
	        (*shape)[0] = tmp_line_shape[0];
	        (*shape)[1] = tmp_line_shape[1];
	        (*shape)[2] = tmp_line_shape[2];
                *dim_s = 3;
	    } else {
		DXSetError (ERROR_BAD_PARAMETER, "#10231",
                           "for line structure, shape", 2, 3);
		return (ERROR);
	    }
            if (((*shape)[0]==0)&&((*shape)[1]==0)&&((*shape)[2]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831","shape");
               return ERROR;
            }
	}
    }

    else if (! strcmp (structure, "rectangle"))
    {
	if (! (*shape = (float *) DXAllocate (6 * sizeof (float))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*shape)[0] = 1.0;
	    (*shape)[1] = 0.0;
	    (*shape)[2] = 0.0;
	    (*shape)[3] = 0.0;
	    (*shape)[4] = 1.0;
	    (*shape)[5] = 0.0;
            *dim_s = dim_p;
	} else {
	    if (DXExtractParameter (ino, TYPE_FLOAT, 2, 2, 
		(Pointer) tmp_rectangle_shape_2D)) {
	        (*shape)[0] = tmp_rectangle_shape_2D[0];
	        (*shape)[1] = tmp_rectangle_shape_2D[1];
	        (*shape)[2] = 0.0;
	        (*shape)[3] = tmp_rectangle_shape_2D[2];
	        (*shape)[4] = tmp_rectangle_shape_2D[3];
	        (*shape)[5] = 0.0;
                *dim_s = 2;
	    } else if (DXExtractParameter (ino, TYPE_FLOAT, 3, 2, 
		(Pointer) tmp_rectangle_shape)) {
	        (*shape)[0] = tmp_rectangle_shape[0];
	        (*shape)[1] = tmp_rectangle_shape[1];
	        (*shape)[2] = tmp_rectangle_shape[2];
	        (*shape)[3] = tmp_rectangle_shape[3];
	        (*shape)[4] = tmp_rectangle_shape[4];
	        (*shape)[5] = tmp_rectangle_shape[5];
                *dim_s = 3;
	    } else {
		DXSetError (ERROR_BAD_PARAMETER, "#10581",
                            "for rectangle structure, shape", 2, 2, 2, 3);
		return (ERROR);
	    }
            if (((*shape)[0]==0)&&((*shape)[1]==0)&&((*shape)[2]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
            if (((*shape)[3]==0)&&((*shape)[4]==0)&&((*shape)[5]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831","shape");
               return ERROR;
            }
	}
    }

    else if (! strcmp (structure, "crosshair"))
    {
	if (! (*shape = (float *) DXAllocate (9 * sizeof (float))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*shape)[0] = 1.0;
	    (*shape)[1] = 0.0;
	    (*shape)[2] = 0.0;
	    (*shape)[3] = 0.0;
	    (*shape)[4] = 1.0;
	    (*shape)[5] = 0.0;
	    (*shape)[6] = 0.0;
	    (*shape)[7] = 0.0;
	    (*shape)[8] = 1.0;
            *dim_s = dim_p; 
	} else {
	    if (DXExtractParameter (ino, TYPE_FLOAT, 3, 3, 
		(Pointer) tmp_crosshair_shape)) {
	        (*shape)[0] = tmp_crosshair_shape[0];
	        (*shape)[1] = tmp_crosshair_shape[1];
	        (*shape)[2] = tmp_crosshair_shape[2];
	        (*shape)[3] = tmp_crosshair_shape[3];
	        (*shape)[4] = tmp_crosshair_shape[4];
	        (*shape)[5] = tmp_crosshair_shape[5];
	        (*shape)[6] = tmp_crosshair_shape[6];
	        (*shape)[7] = tmp_crosshair_shape[7];
	        (*shape)[8] = tmp_crosshair_shape[8];
                *dim_s = 3;
	    } else {
		DXSetError (ERROR_BAD_PARAMETER, "#10580",
                            "for crosshair structure, shape", 3, 3);
		return (ERROR);
	    }
            if (((*shape)[0]==0)&&((*shape)[1]==0)&&((*shape)[2]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
            if (((*shape)[3]==0)&&((*shape)[4]==0)&&((*shape)[5]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
            if (((*shape)[6]==0)&&((*shape)[7]==0)&&((*shape)[8]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
	}
    }
    else if (! strcmp (structure, "ellipse"))
    {
	if (! (*shape = (float *) DXAllocate (6 * sizeof (float))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*shape)[0] = 1.0;
	    (*shape)[1] = 0.0;
	    (*shape)[2] = 0.0;
	    (*shape)[3] = 0.0;
	    (*shape)[4] = 1.0;
	    (*shape)[5] = 0.0;
            *dim_s = dim_p;
	} else {
	    if (DXExtractParameter (ino, TYPE_FLOAT, 2, 2, 
		(Pointer) tmp_ellipse_shape_2D)) {
	        (*shape)[0] = tmp_ellipse_shape_2D[0];
	        (*shape)[1] = tmp_ellipse_shape_2D[1];
	        (*shape)[2] = 0.0;
	        (*shape)[3] = tmp_ellipse_shape_2D[2];
	        (*shape)[4] = tmp_ellipse_shape_2D[3];
	        (*shape)[5] = 0.0;
                *dim_s = 2;
	    } else if (DXExtractParameter (ino, TYPE_FLOAT, 3, 2, 
		(Pointer) tmp_ellipse_shape)) {
	        (*shape)[0] = tmp_ellipse_shape[0];
	        (*shape)[1] = tmp_ellipse_shape[1];
	        (*shape)[2] = tmp_ellipse_shape[2];
	        (*shape)[3] = tmp_ellipse_shape[3];
	        (*shape)[4] = tmp_ellipse_shape[4];
	        (*shape)[5] = tmp_ellipse_shape[5];
                *dim_s = 3;
	    } else {
		DXSetError (ERROR_BAD_PARAMETER, "#10581",
                            "for ellipse structure, shape", 2, 2, 2, 3);
		return (ERROR);
	    }
            if (((*shape)[0]==0)&&((*shape)[1]==0)&&((*shape)[2]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
            if (((*shape)[3]==0)&&((*shape)[4]==0)&&((*shape)[5]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
	}
    }

    else		/* brick */
    {
	if (! (*shape = (float *) DXAllocate (9 * sizeof (float))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*shape)[0] = 1.0;
	    (*shape)[1] = 0.0;
	    (*shape)[2] = 0.0;
	    (*shape)[3] = 0.0;
	    (*shape)[4] = 1.0;
	    (*shape)[5] = 0.0;
	    (*shape)[6] = 0.0;
	    (*shape)[7] = 0.0;
	    (*shape)[8] = 1.0;
            *dim_s = dim_p;
	} else {
	    if (DXExtractParameter (ino, TYPE_FLOAT, 3, 3, 
		(Pointer) tmp_brick_shape)) {
	        (*shape)[0] = tmp_brick_shape[0];
	        (*shape)[1] = tmp_brick_shape[1];
	        (*shape)[2] = tmp_brick_shape[2];
	        (*shape)[3] = tmp_brick_shape[3];
	        (*shape)[4] = tmp_brick_shape[4];
	        (*shape)[5] = tmp_brick_shape[5];
	        (*shape)[6] = tmp_brick_shape[6];
	        (*shape)[7] = tmp_brick_shape[7];
	        (*shape)[8] = tmp_brick_shape[8];
                *dim_s = 3;
	    } else {
		DXSetError (ERROR_BAD_PARAMETER, "#10580",
                            "for brick structure, shape", 3, 3);
		return (ERROR);
	    }
            if (((*shape)[0]==0)&&((*shape)[1]==0)&&((*shape)[2]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
            if (((*shape)[3]==0)&&((*shape)[4]==0)&&((*shape)[5]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }
            if (((*shape)[6]==0)&&((*shape)[7]==0)&&((*shape)[8]==0)) {
               DXSetError (ERROR_BAD_PARAMETER,"#11831", "shape");
               return ERROR;
            }

	}
    }

    return (OK);
}

static Error grid_density (int **density, char *structure, Object ino)
{
    int i, len_of_list;
	
    if (! density)
	DXErrorReturn (ERROR_UNEXPECTED, "density NULL");
    if (! structure)
	DXErrorReturn (ERROR_UNEXPECTED, "structure NULL");

    if (! strcmp (structure, "line"))
    {
	len_of_list = 1;
	if (! (*density = (int *) DXAllocate (1 * sizeof (int))))
	     return ERROR;

	if (ino == NULL)
	{
	    (*density)[0] = 3;
	}

	else
	{
	    if (! DXExtractParameter (ino, TYPE_INT, 1, 1, (Pointer) *density))
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#11832", "line structure",
                            "density");
		return (ERROR);
	    }
	}
    }

    else if (! strcmp (structure, "rectangle"))
    {
	len_of_list = 2;
	if (! (*density = (int *) DXAllocate (2 * sizeof (int))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*density)[0] = 3;
	    (*density)[1] = 3;
	}

	else
	{
	    if (! DXExtractParameter (ino, TYPE_INT, 1, 2, (Pointer) *density))
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#11833","rectangle structure",
                            "density", 2);
		return (ERROR);
	    }
	}
    }

    else if (! strcmp (structure, "crosshair"))
    {
	len_of_list = 3;
	if (! (*density = (int *) DXAllocate (3 * sizeof (int))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*density)[0] = 3;
	    (*density)[1] = 3;
	    (*density)[2] = 3;
	}

	else
	{
	    if (! DXExtractParameter (ino, TYPE_INT, 1, 3, (Pointer) *density))
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#11833","crosshair structure",
                            "density", 3);
		return (ERROR);
	    }
	}
    }

    else if (! strcmp (structure, "ellipse"))
    {
	len_of_list = 1;
	if (! (*density = (int *) DXAllocate (1 * sizeof (int))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*density)[0] = 3;
	}

	else
	{
	    if (! DXExtractParameter (ino, TYPE_INT, 1, 1, (Pointer) *density))
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#11832", "ellipse structure",
                            "density");
		return (ERROR);
	    }
	}
    }

    else		/* brick */
    {
	len_of_list = 3;
	if (! (*density = (int *) DXAllocate (3 * sizeof (int))))
	    return ERROR;

	if (ino == NULL)
	{
	    (*density)[0] = 3;
	    (*density)[1] = 3;
	    (*density)[2] = 3;
	}

	else
	{
	    if (! DXExtractParameter (ino, TYPE_INT, 1, 3, (Pointer) *density))
	    {
		DXSetError (ERROR_BAD_PARAMETER, "#11833", "brick structure",
                            "density", 3);
		return (ERROR);
	    }
	}
    }
/*
 * warn if any densities are one (1) since a dimension will be lost
 */
    for (i = 0; i < len_of_list; i++) {
	if((*density)[i] == 1) {
	    DXWarning("A density of one means a dimension will be lost.");
	    break;
	}
    }

/*
 * error out if any densities are zero.
 */
    for (i = 0; i < len_of_list; i++) {
	if((*density)[i] == 0) {
	    DXSetError (ERROR_BAD_PARAMETER, "#10531", "density");
	    return (ERROR);
	}
    }

    return (OK);
}
