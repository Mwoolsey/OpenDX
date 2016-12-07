/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "screenClass.h"

/*
 * New..., Delete...
 */

static Screen
_NewScreen(Object o, int position, int z, struct screen_class *class)
{
    Screen s = (Screen) _dxf_NewObject((struct object_class *)class);
    if (!s)
	return NULL;

    s->o = DXReference(o);
    s->position = position;
    s->z = z;

    return s;
}

Screen
DXNewScreen(Object o, int position, int z)
{
    return _NewScreen(o, position, z, &_dxdscreen_class);
}

Screen
DXGetScreenInfo(Screen s, Object *o, int *position, int *z)
{
    CHECK(s, CLASS_SCREEN);
    if (o)
	*o = s->o;
    if (position)
	*position = s->position;
    if (z)
	*z = s->z;
    return s;
}


Screen
DXSetScreenObject(Screen s, Object o)
{
    DXReference(o);
    DXDelete(s->o);
    s->o = o;
    return s;
}


int
_dxfScreen_Delete(Screen s)
{
    DXDelete(s->o);
    return OK;
}


Object
_dxfScreen_Copy(Screen old, enum _dxd_copy copy)
{
    Screen new;
    Object o;

    /*
     * Do recursive copy?
     * NB - no difference between COPY_HEADER and COPY_ATTRIBUTES
     * here, because we can't create a screen object w/o o
     */
    if (copy==COPY_HEADER || copy==COPY_ATTRIBUTES)
	o = old->o;
    else {
	o = DXCopy(old->o, copy);
	if (!o)
	    return NULL;
    }

    /* make new object */
    new = DXNewScreen(o, old->position, old->z);
    if (!new) {
	DXDelete(o);
	return NULL;
    }

    /* copy superclass parts */
    return _dxf_CopyObject((Object)new, (Object)old, copy);
}    
