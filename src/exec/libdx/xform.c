/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "xformClass.h"

/*
 * New..., DXDelete...
 */

static Xform
_NewXform(Object o, Matrix m, struct xform_class *class)
{
    Xform t = (Xform) _dxf_NewObject((struct object_class *)class);
    if (!t)
	return NULL;

    t->o = DXReference(o);
    t->m = m;

    return t;
}

Xform
DXNewXform(Object o, Matrix m)
{
    return _NewXform(o, m, &_dxdxform_class);
}

Xform
DXGetXformInfo(Xform t, Object *o, Matrix *m)
{
    CHECK(t, CLASS_XFORM);
    if (o)
	*o = t->o;
    if (m)
	*m = t->m;
    return t;
}


Xform
DXSetXformObject(Xform t, Object o)
{
    DXReference(o);
    DXDelete(t->o);
    t->o = o;
    return t;
}

Xform
DXSetXformMatrix(Xform t, Matrix m)
{
    t->m = m;
    return t;
}


Error
_dxfXform_Delete(Xform t)
{
    DXDelete(t->o);
    return OK;
}


Object
_dxfXform_Copy(Xform old, enum _dxd_copy copy)
{
    Xform new;
    Object o;

    /*
     * Do recursive copy?
     * NB - no difference between COPY_HEADER and COPY_ATTRIBUTES
     * here, because we can't create an xform object w/o o
     */
    if (copy==COPY_HEADER || copy==COPY_ATTRIBUTES)
	o = old->o;
    else {
	o = DXCopy(old->o, copy);
	if (!o)
	    return NULL;
    }

    /* make new object */
    new = DXNewXform(o, old->m);
    if (!new) {
	DXDelete(o);
	return NULL;
    }

    /* copy superclass parts */
    return _dxf_CopyObject((Object)new, (Object)old, copy);
}    

Object
_dxfXform_GetType(Xform t, Type *ty, Category *c, int *rank, int *shape)
{
    if (!t)
	return NULL;
    return DXGetType(t->o, ty, c, rank, shape);
}

