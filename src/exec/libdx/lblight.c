/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "lightClass.h"

/*
 * XXX - make the various kinds of light subclasses?
 */

static Light
_NewLight(struct light_class *class)
{
    Light l = (Light) _dxf_NewObject((struct object_class *)class);
    if (!l)
	return NULL;

    l->kind = invalid_kind;
    l->relative = invalid_relative;

    return l;
}

Light
DXNewPointLight(Point position, RGBColor color)
{
    Light l = _NewLight(&_dxdlight_class);
    if (!l)
	return NULL;

    l->kind = point;
    l->relative = world;
    l->position = position;
    l->color = color;

    return l;
}


Light
DXNewDistantLight(Vector direction, RGBColor color)
{
    float d;
    Light l;

    d = DXLength(direction);
    if (d)
	direction = DXDiv(direction, d);
    else
	DXErrorReturn(ERROR_BAD_PARAMETER, "zero-length light direction vector");

    l = _NewLight(&_dxdlight_class);
    if (!l)
	return NULL;
    l->kind = distant;
    l->relative = world;
    l->direction = direction;
    l->color = color;

    return l;
}

Light
DXNewCameraPointLight(Point position, RGBColor color)
{
    Light l = _NewLight(&_dxdlight_class);
    if (!l)
	return NULL;

    l->kind = point;
    l->relative = camera;
    l->position = position;
    l->color = color;

    return l;
}

Light
DXNewCameraDistantLight(Vector direction, RGBColor color)
{
    float d;
    Light l;

    d = DXLength(direction);
    if (d)
	direction = DXDiv(direction, d);
    else
	DXErrorReturn(ERROR_BAD_PARAMETER, "zero-length light direction vector");

    l = _NewLight(&_dxdlight_class);
    if (!l)
	return NULL;
    l->kind = distant;
    l->relative = camera;
    l->direction = direction;
    l->color = color;

    return l;
}

Light
DXQueryDistantLight(Light l, Vector *direction, RGBColor *color)
{
    CHECK(l, CLASS_LIGHT);
    if (l->kind!=distant || l->relative!=world)
	return NULL;
    if (direction)
	*direction = l->direction;
    if (color)
	*color = l->color;
    return l;
}


Light
DXQueryCameraDistantLight(Light l, Vector *direction, RGBColor *color)
{
    CHECK(l, CLASS_LIGHT);
    if (l->kind!=distant || l->relative!=camera)
	return NULL;
    if (direction)
	*direction = l->direction;
    if (color)
	*color = l->color;
    return l;
}



Light
DXNewAmbientLight(RGBColor color)
{
    Light l = _NewLight(&_dxdlight_class);
    if (!l)
	return NULL;

    l->kind = ambient;
    l->relative = invalid_relative;
    l->color = color;

    return l;
}

Light
DXQueryAmbientLight(Light l, RGBColor *color)
{
    CHECK(l, CLASS_LIGHT);
    if (l->kind != ambient)
	return NULL;
    if (color)
	*color = l->color;
    return l;
}


int
_dxfLight_Delete(Light l)
{
    return OK;
}


/*
 * XXX - is this ok?
 */

Object
_dxfLight_Copy(Light old, enum _dxd_copy copy)
{
    return (Object) old;
}
