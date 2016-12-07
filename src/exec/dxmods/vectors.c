/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/vectors.c,v 1.3 1999/05/10 15:45:33 gda Exp $
 */

#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "vectors.h"

float
_dxfvector_length_3D(Vector *v)
{
    float	length_squared;

    length_squared = v->x * v->x + v->y * v->y + v->z * v->z;
    return((float) sqrt((double) length_squared));
}

float
_dxfvector_length_squared_3D(Vector *v)
{
    return(v->x * v->x + v->y * v->y + v->z * v->z);
}

Error 
_dxfvector_normalize_3D(Vector *vi, Vector *vo)
{
    float	length;

    length = _dxfvector_length_3D(vi);

    if (length <= 0.0)
        return ERROR;

    vo->x = vi->x / length;
    vo->y = vi->y / length;
    vo->z = vi->z / length;

    return OK;
}

Error
_dxfvector_add_3D(Vector *v0, Vector *v1, Vector *v)
{
    v->x = v0->x + v1->x;
    v->y = v0->y + v1->y;
    v->z = v0->z + v1->z;

    return OK;
}

Error
_dxfvector_subtract_3D(Vector *v0, Vector *v1, Vector *v)
{
    v->x = v0->x - v1->x;
    v->y = v0->y - v1->y;
    v->z = v0->z - v1->z;

    return OK;
}

Error
_dxfvector_scale_3D(Vector *vi, float scale, Vector *vo)
{
    vo->x = scale * vi->x;
    vo->y = scale * vi->y;
    vo->z = scale * vi->z;

    return OK;
}

float _dxfvector_dot_3D(Vector *v0, Vector *v1)
{
    float dot;

    dot = v0->x * v1->x + v0->y * v1->y + v0->z * v1->z;

    return(dot);
}

Error 
_dxfvector_cross_3D(Vector *v0, Vector *v1, Vector *v)
{
    v->x = v0->y * v1->z - v0->z * v1->y;
    v->y = v0->z * v1->x - v0->x * v1->z;
    v->z = v0->x * v1->y - v0->y * v1->x;

    return OK;
}

float
_dxfvector_angle_3D(Vector *v0, Vector *v1)
{
    float 	f, l0, l1;

    l0 = _dxfvector_length_3D(v0);
    l1 = _dxfvector_length_3D(v1);

    if (l0 == 0.0 || l1 == 0.0)
	return -1.0;

    f = _dxfvector_dot_3D(v0, v1) /(l0 * l1);

    if (f >= 1.0)
	return 0.0;
    else if (f <= -1.0)
	return M_PI;
    else
	return((float) acos((double) f));
}

int
_dxfvector_zero_3D(Vector *v)
{
    if ((v->x == 0.0) && (v->y == 0.0) && (v->z == 0.0))
	return(1);
    else
	return(0);
}

/* 
 * And the 2-D versions
 */

float
_dxfvector_length_2D(Vector *v)
{
    float	length_squared;

    length_squared = v->x * v->x + v->y * v->y;
    return((float) sqrt((double) length_squared));
}

float
_dxfvector_length_squared_2D(Vector *v)
{
    return(v->x * v->x + v->y * v->y);
}

Error 
_dxfvector_normalize_2D(Vector *vi, Vector *vo)
{
    float	length;

    length = _dxfvector_length_2D(vi);

    if (length <= 0.0)
        return ERROR;

    vo->x = vi->x / length;
    vo->y = vi->y / length;

    return OK;
}

Error
_dxfvector_add_2D(Vector *v0, Vector *v1, Vector *v)
{
    v->x = v0->x + v1->x;
    v->y = v0->y + v1->y;

    return OK;
}

Error
_dxfvector_subtract_2D(Vector *v0, Vector *v1, Vector *v)
{
    v->x = v0->x - v1->x;
    v->y = v0->y - v1->y;

    return OK;
}

Error
_dxfvector_scale_2D(Vector *vi, float scale, Vector *vo)
{
    vo->x = scale * vi->x;
    vo->y = scale * vi->y;

    return OK;
}

float _dxfvector_dot_2D(Vector *v0, Vector *v1)
{
    float dot;

    dot = v0->x * v1->x + v0->y * v1->y;

    return(dot);
}

Error 
_dxfvector_cross_2D(Vector *v0, Vector *v1, Vector *v)
{
    v->x = v0->y * v1->z - v0->z * v1->y;
    v->y = v0->z * v1->x - v0->x * v1->z;

    return OK;
}

float
_dxfvector_angle_2D(Vector *v0, Vector *v1)
{
    float 	f, l0, l1;

    l0 = _dxfvector_length_2D(v0);
    l1 = _dxfvector_length_2D(v1);

    if (l0 == 0.0 || l1 == 0.0)
	return -1.0;

    f = _dxfvector_dot_2D(v0, v1) / (l0 * l1);

    if (f >= 1.0)
	return 0.0;
    else if (f <= -1.0)
	return M_PI;
    else
	return((float) acos((double) f));
}

int
_dxfvector_zero_2D(Vector *v)
{
    if ((v->x == 0.0) && (v->y == 0.0))
	return(1);
    else
	return(0);
}
