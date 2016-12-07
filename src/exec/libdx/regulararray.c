/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include "arrayClass.h"

RegularArray
_dxf_NewRegularArray(Type t, int dim, int n, Pointer origin, Pointer delta,
		 struct regulararray_class *class)
{
    RegularArray a = (RegularArray) _dxf_NewArrayV(t, CATEGORY_REAL, 1, &dim,
				      (struct array_class *)class);
    if (!a)
	return NULL;

    /* copy info */
    a->array.items = n;
    if (2*a->array.size <= sizeof(a->array.ldata)) {
	a->origin = (Pointer)a->array.ldata;
	a->delta = ((char *)(a->array.ldata)) + a->array.size;
    } else {
	a->origin = DXAllocate(a->array.size);
	if (!a->origin)
	    return NULL;
	a->delta = DXAllocate(a->array.size);
	if (!a->delta)
	    return NULL;
    }
    memcpy(a->origin, origin, a->array.size);
    memcpy(a->delta, delta, a->array.size);

    return a;
}

RegularArray
DXNewRegularArray(Type t, int dim, int n, Pointer origin, Pointer delta)
{
    return _dxf_NewRegularArray(t, dim, n, origin, delta, &_dxdregulararray_class);
}

RegularArray
DXGetRegularArrayInfo(RegularArray a, int *count, Pointer origin, Pointer delta)
{
    CHECKARRAY(a, CLASS_REGULARARRAY);

    if (count)
	*count = a->array.items;
    if (origin)
	memcpy(origin, a->origin, a->array.size);
    if (delta)
	memcpy(delta, a->delta, a->array.size);

    return a;
}

#define EXPAND(type) {							\
									\
    type *data = (type *) DXAllocate(size * items);			\
    type *origin = (type *) DXAllocateLocal(2 * size);			\
    type *delta = origin + n;						\
    type *d;								\
									\
    /* local copy of origin and delta for performance */		\
    if (!data || !origin) {						\
	DXFree((Pointer)data);						\
	DXFree((Pointer)origin);						\
	goto error;							\
    }									\
    memcpy(origin, a->origin, size);					\
    memcpy(delta, a->delta, size);					\
    									\
    /* fill in the new items */						\
    for (i=0, d=data; i<items; i++)					\
	for (j=0; j<n; j++)						\
	    *d++ = origin[j] + (type)(i*delta[j]);			\
									\
    /* free local copy */						\
    DXFree((Pointer)origin);						\
									\
    /* DXunlock and return */						\
    EXPAND_RETURN(a, data);						\
									\
}

Pointer
_dxfRegularArray_GetArrayData(RegularArray a)
{
    int size = a->array.size;
    int items = a->array.items;
    int n = a->array.shape[0];
    int i, j;

    /* DXlock array */
    EXPAND_LOCK(a);

    if (a->array.type==TYPE_SHORT) {
	EXPAND(short);
    } else if (a->array.type==TYPE_BYTE) {
	EXPAND(byte);
    } else if (a->array.type==TYPE_UBYTE) {
	EXPAND(ubyte);
    } else if (a->array.type==TYPE_USHORT) {
	EXPAND(ushort);
    } else if (a->array.type==TYPE_UINT) {
	EXPAND(uint);
    } else if (a->array.type==TYPE_INT) {
	EXPAND(int);
    } else if (a->array.type==TYPE_FLOAT) {
	EXPAND(float);
    } else if (a->array.type==TYPE_DOUBLE) {
	EXPAND(double);
    } else
	DXErrorGoto(ERROR_BAD_PARAMETER, "unknown array type");

error:
    EXPAND_ERROR(a);
}

Error
_dxfRegularArray_Delete(RegularArray a)
{
    if (a->origin != (Pointer)a->array.ldata) {
	DXFree(a->origin);
	DXFree(a->delta);
    }
    return _dxfArray_Delete((Array)a);
}
