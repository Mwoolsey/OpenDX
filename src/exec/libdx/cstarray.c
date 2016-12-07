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

static int IsConstantArray(Array a);

static ConstantArray
_NewConstantArray(int n, Pointer d, Type t, Category c, int r, int *s,
		 struct constantarray_class *class)
{
    ConstantArray a = (ConstantArray) _dxf_NewArrayV(t, c, r, s,
				      (struct array_class *)class);
    if (!a)
	return NULL;

    /* copy info */
    a->array.items = n;
    if (a->array.size <= sizeof(a->array.ldata)) {
	a->data = (Pointer)a->array.ldata;
    } else {
	a->data = DXAllocate(a->array.size);
	if (!a->data)
	    return NULL;
    }
    memcpy(a->data, d, a->array.size);

    return a;
}

ConstantArray
DXNewConstantArray(int n, Pointer d, Type t, Category c, int r, ...)
{
    int shape[100];
    int i;
    va_list arg;

    ASSERT(r < 100);

    /* collect args */
    va_start(arg,r);
    for (i=0; i<r; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    return _NewConstantArray(n, d, t, c, r, shape, &_dxdconstantarray_class);
}

ConstantArray
DXNewConstantArrayV(int n, Pointer d, Type t, Category c, int r, int *s)
{
    return _NewConstantArray(n, d, t, c, r, s, &_dxdconstantarray_class);
}

Array
DXQueryConstantArray(Array a, int *n, Pointer d)
{
    CHECK(a, CLASS_ARRAY);   /* can be either REGULAR or CONSTANT */

    if (! IsConstantArray(a))
        return NULL;

    if (d)
    {
	if (DXGetArrayClass((Array)a) == CLASS_CONSTANTARRAY)
	    memcpy(d, ((ConstantArray)a)->data, a->size);
	else 
	    memcpy(d, ((RegularArray)a)->origin, a->size);
    }

    if (n)
	*n = a->items;

    return a;
}

Pointer
DXGetConstantArrayData(Array a)
{
    CHECK(a, CLASS_ARRAY);   /* can be either REGULAR or CONSTANT */

    if (! IsConstantArray(a))
    {
	DXSetError(ERROR_BAD_PARAMETER, "object not constant array");
        return NULL;
    }

    if (DXGetArrayClass(a) == CLASS_CONSTANTARRAY)
	return ((ConstantArray)a)->data;
    else 
	return ((RegularArray)a)->origin;
}


Pointer
_dxfConstantArray_GetArrayData(ConstantArray a)
{
    int size = a->array.size;
    int items = a->array.items;
    int i;
    byte *src = NULL, *dst = NULL, *d;

    /* DXlock array */
    EXPAND_LOCK(a);

    dst = (byte *)DXAllocate(size*items);
    src = (byte *)DXAllocate(size);
    if (! dst || ! src)
	goto error;

    memcpy(src, a->data, size);

    for (i = 0, d = dst; i < items; i++, d += size)
	memcpy(d, src, size);
    
    DXFree((Pointer)src);

    EXPAND_RETURN(a, dst);

error:
    DXFree((Pointer)dst);
    DXFree((Pointer)src);
    EXPAND_ERROR(a);
}


#define CONSTANT_REGULAR(t, cst)				\
{								\
    t *ptr = (t *)((RegularArray)a)->delta;			\
    int  i, knt = a->size / DXTypeSize(a->type);			\
    for (i = 0, cst = 1; i < knt && cst; i++, ptr++)		\
	if (*ptr != 0) cst = 0;					\
}

static int 
IsConstantArray(Array a)
{
    if (DXGetArrayClass(a) == CLASS_CONSTANTARRAY)
	return 1;
    else if (DXGetArrayClass(a) == CLASS_REGULARARRAY)
    {
	int cst=0;
	switch(a->type)
	{
	    case TYPE_DOUBLE: CONSTANT_REGULAR(double, cst); break;
	    case TYPE_FLOAT:  CONSTANT_REGULAR(float,  cst); break;
	    case TYPE_INT:    CONSTANT_REGULAR(int,    cst); break;
	    case TYPE_UINT:   CONSTANT_REGULAR(uint,   cst); break;
	    case TYPE_SHORT:  CONSTANT_REGULAR(short,  cst); break;
	    case TYPE_USHORT: CONSTANT_REGULAR(ushort, cst); break;
	    case TYPE_BYTE:   CONSTANT_REGULAR(byte,   cst); break;
	    case TYPE_UBYTE:  CONSTANT_REGULAR(ubyte,  cst); break;
		case TYPE_HYPER: case TYPE_STRING: break;
	}
	return cst;
    }
    else
	return 0;
}
