/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/maptoplane.c,v 1.7 2003/07/11 05:50:35 davidt Exp $:
 */

#include <dxconfig.h>
#include "_maptoplane.h"

/***
MODULE:		
    _dxfMapToPlane
SHORTDESCRIPTION:
    Maps a field onto a plane.
CATEGORY:
    Realization
INPUTS:
    data;    data field;              none; data to be mapped
    point;       vector;  center of object; a point on the map plane
    normal;      vector;           [0 0 1]; normal to the map plane
OUTPUTS:
    plane;   data field;              NULL; mapped plane
FLAGS:
BUGS:
AUTHOR:
    Brian P. O'Toole
END:
***/

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "bounds.h"
#include "vectors.h"

static Error map_normal ();

int m_MapToPlane (in, out)
    Object		*in;
    Object		*out;
{
    Object		ino0, ino1, ino2;
    Point               p;
    float		point[3];
    float		normal[3];

    out[0] = NULL;

    ino0 = in[0];
    ino1 = in[1];
    ino2 = in[2];

    if (ino0 == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	goto error;
    }

    /* class = DXGetObjectClass (ino0); */

    if (ino1 == NULL) 
    {
	/*
         * Choose the center of the bounding box as the point.
	 */

	if (! _dxf_BBoxPoint (in[0], &p, BB_CENTER))
	{
	    DXSetError(ERROR_DATA_INVALID,
		"unable to determine bounding box for data");
	    goto error;
	}

	point[0] = (float) p.x;
	point[1] = (float) p.y;
	point[2] = (float) p.z;
    }

    else 
    {
	if (! DXExtractParameter (ino1, TYPE_FLOAT, 3, 1, (Pointer) point))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10370", "point", "3-D vector");
	    return ERROR;
	}
    }

    if (! map_normal (normal, ino2))
	return ERROR;

    out[0] = _dxfMapToPlane (ino0, (Vector*)point, (Vector*)normal);

    if (DXGetError() != ERROR_NONE)
        return ERROR;

    if (! out[0])
    {
        out[0] = (Object)DXEndField(DXNewField());
	if (! out[0])
	    return ERROR;
	else
	    return OK;
    }

    if (! DXGetPart(out[0], 0))
    {
	DXDelete(out[0]);
	out[0] = (Object)DXEndField(DXNewField());
    }

    return OK;

error:
    return ERROR;
}


static Error map_normal (normal, ino)
    float		*normal;
    Object		ino;
{
    if (ino == NULL)
    {
	normal[0] = (float) 0.0;
	normal[1] = (float) 0.0;
	normal[2] = (float) 1.0;
    }
    else
    {
	if (! DXExtractParameter (ino, TYPE_FLOAT, 3, 1, (Pointer) normal))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10370", "normal", "3-D vector");
	    return ERROR;
	}
	
	if (vector_zero((Vector *)normal))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#11822", "normal");
	    return ERROR;
	}

    }

    return (OK);
}
