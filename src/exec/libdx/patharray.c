/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include "arrayClass.h"

/*
 * Note that we don't store count, which is the number of points,
 * because the number of elements is stored in items, which
 * is one less than the number of points.
 */

static PathArray
_NewPathArray(int count, struct patharray_class *class)
{
    static int two[] = {2};
    PathArray a = (PathArray) _dxf_NewArrayV(TYPE_INT, CATEGORY_REAL, 1, two,
        (struct array_class *)class);
    if (!a)
	return NULL;
    if (count<=0)
	DXErrorReturn(ERROR_BAD_PARAMETER,
		    "path array must have at least one point");
    a->array.items = count - 1;
    a->offset = 0;
    return a;
}


PathArray
DXNewPathArray(int count)
{
    return _NewPathArray(count, &_dxdpatharray_class);
}


PathArray
DXGetPathArrayInfo(PathArray a, int *count)
{
    CHECKARRAY(a, CLASS_PATHARRAY);
    if (count)
	*count = a->array.items + 1;
    return a;
}


PathArray
DXSetPathOffset(PathArray a, int offset)
{
    CHECKARRAY(a, CLASS_PATHARRAY);
    a->offset = offset;
    return a;
}

PathArray
DXGetPathOffset(PathArray a, int *offset)
{
    CHECKARRAY(a, CLASS_PATHARRAY); 
    if (offset)
	*offset = a->offset;
    return a;
}

Pointer
_dxfPathArray_GetArrayData(PathArray a)
{
    int i, *data, *d, n;

    /* DXlock array */
    EXPAND_LOCK(a);

    /* allocate result */
    data = (int *) DXAllocate(a->array.size * a->array.items);
    if (!data)
	goto error;
    
    /* fill it in */
    for (i=0, d=data, n=a->array.items; i<n; i++) {
	*d++ = i;
        *d++ = i+1;
    }	    
    
    /* DXunlock and return */
    EXPAND_RETURN(a, data);

error:
    EXPAND_ERROR(a);
}


