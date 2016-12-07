/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#include <stdarg.h>
#include <string.h>
#include "arrayClass.h"

/*
 * size multipliers for type, category
 */

/* XXX */
int
DXTypeSize(Type t)
{
    switch (t) {
        case TYPE_BYTE:   return sizeof(byte);
        case TYPE_UBYTE:  return sizeof(ubyte);
        case TYPE_SHORT:  return sizeof(short);
        case TYPE_USHORT: return sizeof(ushort);
        case TYPE_INT:    return sizeof(int);
        case TYPE_UINT:   return sizeof(uint);
        case TYPE_HYPER:  return 8;
        case TYPE_FLOAT:  return sizeof(float);
        case TYPE_DOUBLE: return sizeof(double);
        case TYPE_STRING: return sizeof(char);
        default:
	    return DXSetError(ERROR_BAD_PARAMETER, "unknown Type %d", t);
    }
}

int
DXCategorySize(Category category)
{
    switch(category) {
        case CATEGORY_REAL:       return 1;
        case CATEGORY_COMPLEX:    return 2;
        case CATEGORY_QUATERNION: return 4;
        default:
	    return DXSetError(ERROR_BAD_PARAMETER,
			    "unknown Category %d", category);
    }
}


/*
 * new array, shape specified as a vector
 */

Array
_dxf_NewArrayV(Type type, Category category, int rank, int *shape,
	   struct array_class *class)
{
    int i;
    Array a = (Array) _dxf_NewObject((struct object_class *)class);
    if (!a)
	return NULL;

    /* specified info */
    a->type = type;
    a->category = category;
    a->rank = rank;
    if (shape && rank!=0) {
	a->shape = rank>NLSHAPE?
	    (int*)DXAllocate(rank*sizeof(*shape)) : a->lshape;
	if (!a->shape)
	    return NULL;
	for (i=0; i<rank; i++)
	    a->shape[i] = shape[i];
    } else if (rank != 0) {
	DXSetError(ERROR_BAD_PARAMETER,
		 "rank=%d but shape was not specified in DXNewArray", rank);
	return NULL;
    } else
	a->shape = NULL;

    /* size is derived */
    a->size = DXTypeSize(type) * DXCategorySize(category);
    if (!a->size)
	return NULL;
    for (i=0; i<rank; i++)
	a->size *= shape[i];
    if (!a->size)
	DXErrorReturn(ERROR_BAD_PARAMETER, "extent specified as 0 in DXNewArray");
    a->items = 0;
    a->data = NULL;
    DXcreate_lock(&a->inprogress, "in progress");

    return a;
}    


Array
DXNewArrayV(Type type, Category category, int rank, int *shape)
{
    return _dxf_NewArrayV(type, category, rank, shape, &_dxdarray_class);
}


Array
DXNewArray(Type type, Category category, int rank, ...)
{
    int shape[100];
    int i;
    va_list arg;

    ASSERT(rank<100);

    /* collect args */
    va_start(arg,rank);
    for (i=0; i<rank; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    /* call V version */
    return DXNewArrayV(type, category, rank, shape);
}


Array
DXGetArrayInfo(Array a, int *items, Type *type, Category *category,
	     int *rank, int *shape)
{
    int i;

    CHECK(a, CLASS_ARRAY);

    if (items)
	*items = a->items;
    if (type)
	*type = a->type;
    if (category)
	*category = a->category;
    if (rank)
	*rank = a->rank;
    if (shape)
	for (i=0; i<a->rank; i++)
	    shape[i] = a->shape[i];

    return a;
}


Object
_dxfArray_GetType(Array a, Type *t, Category *c, int *rank, int *shape)
{
    int i;

    if (t)
	*t = a->type;
    if (c)
	*c = a->category;
    if (rank)
	*rank = a->rank;
    if (shape)
	for (i=0; i<a->rank; i++)
	    shape[i] = a->shape[i];

    return (Object) a;
}


int DXGetItemSize(Array a)
{
    CHECK(a, CLASS_ARRAY);
    return a->size;
}


Array
DXTypeCheckV(Array a, Type type, Category category, int rank, int *shape)
{
    Type t;
    Category c;
    int i, r, s[100];

    ASSERT(rank<100);

    if (!DXGetArrayInfo(a, NULL, &t, &c, &r, NULL))
	return NULL;
    if (t!=type || c!=category || r!=rank)
	return NULL;

    if (!DXGetArrayInfo(a, NULL, NULL, NULL, NULL, s))
	return NULL;
    if (shape)
	for (i=0; i<rank; i++)
	    if (s[i]!=shape[i])
		return NULL;

    return a;
}


Array
DXTypeCheck(Array a, Type type, Category category, int rank, ...)
{
    int shape[100];
    int i;
    va_list arg;

    ASSERT(rank<100);

    /* collect args */
    va_start(arg,rank);
    for (i=0; i<rank; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    /* call V version */
    return DXTypeCheckV(a, type, category, rank, shape);
}


Pointer
_dxfArray_GetArrayData(Array a)
{
    CHECK(a, CLASS_ARRAY);

    /* always return something */
    if (!a->data)
	DXAllocateArray(a, 0);

    return a->data;
}



Pointer
DXGetArrayDataLocal(Array a)
{
	Pointer data;

	CHECK(a, CLASS_ARRAY);
	data = DXGetArrayData(a);
#if DXD_HAS_LOCAL_MEMORY
	{
		int n;
		Pointer local;
		if (!data)
			return NULL;
		/* XXX - copy, record local reference */
		n = a->size * a->items;
		local = DXAllocateLocal(n);
		if (!local) {
			DXResetError();
			DXWarning("no room for array of %d bytes in local memory", n);
			return data;
		}
		memcpy(local, data, n);
		return local;
	}
#else
	return data;
#endif
}



Array
DXFreeArrayDataLocal(Array a, Pointer data)
{
    CHECK(a, CLASS_ARRAY);
#if DXD_HAS_LOCAL_MEMORY
    if (data!=a->data)
	DXFree(data);
#endif
    return a;
}


Error
_dxfArray_Delete(Array a)
{
    if (a->shape != a->lshape)
	DXFree((Pointer)(a->shape));
    if (a->data != (Pointer)a->ldata)
	DXFree((Pointer)(a->data));
    return OK;
}


Object
_dxfArray_Copy(Array old, enum _dxd_copy copy)
{
    if (copy==COPY_DATA)
	DXErrorReturn(ERROR_BAD_PARAMETER, "copying data is not implemented");
    return (Object)old;
}


Array
DXAddArrayData(Array a, int start, int n, Pointer data)
{
    CHECKARRAY(a, CLASS_ARRAY);   /* only valid for irregular arrays */

    /* expand if necessary */
    /* second test guarantees that we allocate a buffer */
    if (start+n > a->allocated || a->allocated==0) {
	/* allocate 25% extra iff this is a realloc */	
        int alloc = (a->allocated? (start+n)*5/4 : start+n);
	if (!DXAllocateArray(a, alloc))
	    return NULL;
    }

    /* copy the data */
    if (data)
	memcpy((char *)(a->data)+start*a->size, data, n*a->size);

    /* record the new number of items */
    if (start+n > a->items)
	a->items = start+n;

    return a;
}


Array
DXAllocateArray(Array a, int n)
{
    CHECKARRAY(a, CLASS_ARRAY);    /* only valid for irregular arrays */

    /* second test guarantees something will be allocated even if 0 length */
    if (n > a->allocated || a->allocated==0) {
	int bytes = n * a->size;
	Pointer d;
	if (bytes > sizeof(a->ldata)) {
	    if (a->data == (Pointer)a->ldata) {
		d = DXAllocate(bytes);
		if (!d)
		    return NULL;
		memcpy(d, a->ldata, sizeof(a->ldata));
	    } else {
		d = DXReAllocate(a->data, bytes);
		if (!d)
		    return NULL;
	    }
	    a->data = d;
	} else
	    a->data = (Pointer)a->ldata;
	a->allocated = n;
    }
    return a;
}


Array
DXTrim(Array a)
{
#if 0  /* EndField() calls this on all arrays regardless of type */
    CHECKARRAY(a, CLASS_ARRAY);   /* only valid for irregular arrays */
#else
    CHECK(a, CLASS_ARRAY);
#endif

    if (a->allocated > a->items) {
	if (a->data != (Pointer)a->ldata) {
	    Pointer d = DXReAllocate(a->data, a->items*a->size);
	    if (!d)   /* realloc shouldn't fail, but just in case ... */
		DXErrorReturn(ERROR_UNEXPECTED, "re-alloc failed!");
	    a->data = d;
	}
	a->allocated = a->items;
    }				   
    return a;
}

/* this might be a useful function to add sometime */
Array
DXTrimItems(Array a, int nitems)
{
    CHECKARRAY(a, CLASS_ARRAY);   /* only valid for irregular arrays */

    /* reset the item count only if smaller, 
     * and release any extra space in either case.
     */
    if (a->items > nitems)
	a->items = nitems;

    return DXTrim(a);
}

Pointer
DXGetArrayData(Array a)
{
    return _dxfGetArrayData(a);
}

ArrayHandle
DXCreateArrayHandle(Array array)
{
    ArrayHandle handle = NULL;
    Array *terms = NULL;

    handle = (ArrayHandle)DXAllocateZero(sizeof(struct arrayHandle));
    if (! handle)
	goto error;
    
    handle->class    = DXGetArrayClass(array);
    handle->itemSize = DXGetItemSize(array);

    DXGetArrayInfo(array, &handle->nElements,
		&handle->type, &handle->cat, NULL, NULL);

    handle->nValues = handle->itemSize / DXTypeSize(handle->type);

    if (array->data)
    {
	handle->data = array->data;
    }
    else if (DXQueryConstantArray(array, NULL, NULL))
    {
	handle->cdata = DXAllocate(handle->itemSize);
	if (! handle->cdata)
	    goto error;

	DXQueryConstantArray(array, NULL, handle->cdata);
    }
    else
	switch(handle->class)
	{
	    case CLASS_REGULARARRAY:
		handle->origin = DXAllocate(handle->itemSize);
		handle->delta  = DXAllocate(handle->itemSize);
		if (! handle->origin || ! handle->delta)
		    goto error;
		DXGetRegularArrayInfo((RegularArray)array, NULL,
					handle->origin, handle->delta);
	    break;
	    
	    case CLASS_PATHARRAY:
	    break;

	    case CLASS_PRODUCTARRAY:
	    {
		int i;

		DXGetProductArrayInfo((ProductArray)array,
						&handle->nTerms, NULL);

		terms = (Array *)DXAllocate(handle->nTerms * sizeof(Array));

		handle->scratch1 = (int *)DXAllocate(handle->itemSize);
		handle->scratch2 = (int *)DXAllocate(handle->itemSize);
		
		handle->strides = (int *)
			DXAllocateZero(handle->nTerms * sizeof(int));
		handle->counts = (int *)
			DXAllocateZero(handle->nTerms * sizeof(int));
		handle->scratch = (Pointer *)
			DXAllocateZero(handle->nTerms * sizeof(Pointer));
		handle->handles = (ArrayHandle *)
			DXAllocateZero(handle->nTerms * sizeof(ArrayHandle));
		handle->indices = (int *)
			DXAllocateZero(handle->nTerms * sizeof(int));
		
		if (! handle->strides || ! handle->counts  ||
		    ! terms           || ! handle->indices ||
		    ! handle->scratch1    || ! handle->scratch2    ||
		    ! handle->scratch || ! handle->handles) goto error;

		DXGetProductArrayInfo((ProductArray)array, NULL, terms);

		for (i = 0; i < handle->nTerms; i++)
		{
		    handle->handles[i] = DXCreateArrayHandle(terms[i]);
		    if (NULL == handle->handles[i])
			goto error;
		    
		    handle->scratch[i]=DXAllocate(DXGetItemSize(terms[i]));
		    if (NULL == handle->scratch)
			goto error;
		    
		    DXGetArrayInfo(terms[i], &handle->counts[i],
						NULL, NULL, NULL, NULL);
		}

		DXFree((Pointer)terms);

		handle->strides[handle->nTerms-1] = 1;
		for (i = handle->nTerms-2; i >= 0; i--)
		    handle->strides[i] = 
			    handle->strides[i+1]*handle->counts[i+1];
		
	    }
	    break;

	    case CLASS_MESHARRAY:
	    {
		int i;

		DXGetMeshArrayInfo((MeshArray)array, &handle->nTerms, NULL);

		terms = (Array *)DXAllocate(handle->nTerms * sizeof(Array));

		handle->scratch1 = (int *)DXAllocate(handle->itemSize);
		handle->scratch2 = (int *)DXAllocate(handle->itemSize);
		
		handle->strides = (int *)
			DXAllocateZero(handle->nTerms * sizeof(int));
		handle->counts = (int *)
			DXAllocateZero(handle->nTerms * sizeof(int));
		handle->scratch = (Pointer *)
			DXAllocateZero(handle->nTerms * sizeof(Pointer));
		handle->handles = (ArrayHandle *)
			DXAllocateZero(handle->nTerms * sizeof(ArrayHandle));
		handle->indices = (int *)
			DXAllocateZero(handle->nTerms * sizeof(int));
		
		if (! handle->strides || ! handle->counts || ! terms ||
		    ! handle->scratch1    || ! handle->scratch2   ||
		    ! handle->indices ||
		    ! handle->scratch || ! handle->handles) goto error;

		DXGetMeshArrayInfo((MeshArray)array, NULL, terms);
		
		for (i = 0; i < handle->nTerms; i++)
		{
		    handle->handles[i] = DXCreateArrayHandle(terms[i]);
		    if (NULL == handle->handles[i])
			goto error;
		    
		    handle->scratch[i]=DXAllocate(DXGetItemSize(terms[i]));
		    if (NULL == handle->scratch)
			goto error;
		    
		    DXGetArrayInfo(terms[i], &handle->counts[i],
						NULL, NULL, NULL, NULL);
		    handle->counts[i] += 1;
		}

		DXFree((Pointer)terms);

		handle->strides[handle->nTerms-1] = 1;
		for (i = handle->nTerms-2; i >= 0; i--)
		    handle->strides[i] = 
			    handle->strides[i+1]*(handle->counts[i+1]-1);
		
	    }
	    break;

	    default:
		handle->data = DXGetArrayData(array);
		break;
	}
    
    return handle;

error:
    DXFreeArrayHandle(handle);
    DXFree((Pointer)terms);
    return NULL;
}

Error
DXFreeArrayHandle(ArrayHandle handle)
{
    int i;

    if (handle)
    {
	for (i = 0; i < handle->nTerms; i++)
	{
	    DXFreeArrayHandle(handle->handles[i]);
	    DXFree(handle->scratch[i]);
	}

	DXFree((Pointer)handle->strides);
	DXFree((Pointer)handle->counts);
	DXFree((Pointer)handle->handles);
	DXFree((Pointer)handle->indices);
	DXFree((Pointer)handle->scratch);
	DXFree((Pointer)handle->scratch1);
	DXFree((Pointer)handle->scratch2);
	DXFree((Pointer)handle->cdata);
	DXFree((Pointer)handle->origin);
	DXFree((Pointer)handle->delta);
	DXFree((Pointer)handle);
    }

    return OK;
}

#define REGULAR_GETELEMENT(type)					\
{									\
    type *dst = (type *)scratch;					\
    type *org = (type *)handle->origin;					\
    type *del = (type *)handle->delta;					\
    int i;								\
    for (i = 0; i < handle->nValues; i++)				\
	*dst++ = *org++ + offset*(*del++);				\
}	

#define MESHARRAY_ADDTERM(type)						\
{									\
    type *dst = (type *)scratch;					\
    type *right;							\
    int i, j;								\
									\
    for (i = 1; i < handle->nTerms; i++)				\
    {									\
	right = (type *)DXGetArrayEntry(handle->handles[i],		\
		    handle->indices[i], handle->scratch[i]);		\
	for (j = 0; j < handle->nValues; j++)				\
	    dst[j] += right[j];						\
    }									\
}

static void
_dxfTermIndices(ArrayHandle handle, int offset, int *indices)
{
    int i;
    for (i = 0; i < handle->nTerms-1; i++)
    {
	indices[i] = offset / handle->strides[i];
	offset = offset % handle->strides[i];
    }

    indices[handle->nTerms-1] = offset;
}

Pointer
DXCalculateArrayEntry(ArrayHandle handle, int offset, Pointer scratch)
{
    if (handle->data)
        return (Pointer)(((char *)handle->data) + offset*handle->itemSize);
    else if (handle->cdata)
	return handle->cdata;
    else
    {
	switch (handle->class)
	{
	    case CLASS_REGULARARRAY:
		switch(handle->type)
		{
		    case TYPE_DOUBLE: REGULAR_GETELEMENT(double);  break;
		    case TYPE_FLOAT:  REGULAR_GETELEMENT(float);   break;
		    case TYPE_INT:    REGULAR_GETELEMENT(int);     break;
		    case TYPE_UINT:   REGULAR_GETELEMENT(uint);    break;
		    case TYPE_SHORT:  REGULAR_GETELEMENT(short);   break;
		    case TYPE_USHORT: REGULAR_GETELEMENT(ushort);  break;
		    case TYPE_BYTE:   REGULAR_GETELEMENT(byte);    break;
		    case TYPE_UBYTE:  REGULAR_GETELEMENT(ubyte);   break;
		    case TYPE_HYPER: case TYPE_STRING: break;
		}
	    break;
	    
	    case CLASS_PATHARRAY:
		((int *)scratch)[0] = offset;
		((int *)scratch)[1] = offset+1;
	    break;

	    case CLASS_MESHARRAY:
	    {
		_dxfTermIndices(handle, offset, handle->indices);

		if (handle->nTerms == 1)
		{
		    memcpy(scratch, DXGetArrayEntry(handle->handles[0],
			handle->indices[0], handle->scratch[0]),
			handle->handles[0]->itemSize);
		}
		else
		{
		    int *left, *right, *dst, *ptr, i, j, k, nItems;

		    memcpy(handle->scratch1, DXGetArrayEntry(handle->handles[0],
			handle->indices[0], handle->scratch[0]),
			handle->handles[0]->itemSize);

		    nItems = handle->handles[0]->nValues;
		    left = handle->scratch1; dst = handle->scratch2;
		    for (i = 1; i < handle->nTerms; i++)
		    {
			if (i == handle->nTerms-1)
			    dst = (int *)scratch;

			right = (int *)DXGetArrayEntry(handle->handles[i],
					handle->indices[i], handle->scratch[i]);

			ptr = dst;
			for (j = 0; j < nItems; j++)
			{
			    int tmp = left[j] * handle->counts[i];

			    for (k = 0; k < handle->handles[i]->nValues; k++)
				*ptr++ = tmp + right[k];
			}

			nItems *= handle->handles[i]->nValues;

			ptr = dst;
			dst = left;
			left = ptr;
		    }
		}
	    }
	    break;
		    
	    case CLASS_PRODUCTARRAY:
	    {
		_dxfTermIndices(handle, offset, handle->indices);

		if (handle->nTerms == 1)
		{
		    memcpy(scratch, DXGetArrayEntry(handle->handles[0],
			handle->indices[0], handle->scratch[0]),
			handle->handles[0]->itemSize);
		}
		else
		{
		    memcpy(scratch, DXGetArrayEntry(handle->handles[0],
			handle->indices[0], handle->scratch[0]),
			handle->handles[0]->itemSize);
		    
		    switch(handle->type)
		    {
			case TYPE_DOUBLE: MESHARRAY_ADDTERM(double);  break;
			case TYPE_FLOAT:  MESHARRAY_ADDTERM(float);   break;
			case TYPE_INT:    MESHARRAY_ADDTERM(int);     break;
			case TYPE_UINT:   MESHARRAY_ADDTERM(uint);    break;
			case TYPE_SHORT:  MESHARRAY_ADDTERM(short);   break;
			case TYPE_USHORT: MESHARRAY_ADDTERM(ushort);  break;
			case TYPE_BYTE:   MESHARRAY_ADDTERM(byte);    break;
			case TYPE_UBYTE:  MESHARRAY_ADDTERM(ubyte);   break;
		    case TYPE_HYPER: case TYPE_STRING: break;
		    }
		}
	    }
	    break;
	    default: /* All other TYPEs and CLASSes  */
	    break;
	}

    }
    return scratch;

}


/* make string arrays */
Array
DXMakeStringList(int n, char *s, ...)
{
    int i;
    char **c;
    va_list arg;
    Array a;

    if (n == 0)
	return NULL;

    if (n < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "#10020", "stringlist count");
	return NULL;
    }

    c = (char **)DXAllocate(n * sizeof(char *));
    if (!c)
	return NULL;

    c[0] = s;

    /* collect args */
    va_start(arg,s);
    for (i=1; i<n; i++)
	c[i] = va_arg(arg, char *);
    va_end(arg);

    /* call V version */
    a = DXMakeStringListV(n, c);

    DXFree(c);
    return a;
}

/* count plus string list */
Array
DXMakeStringListV(int n, char **s)
{
    Array a;
    int i;
    int count, l, maxlen = 0;
    char *p;

    if (n == 0 || !s)
	return NULL;

    if (n < 0) {
	DXSetError(ERROR_BAD_PARAMETER, "#10020", "stringlist count");
	return NULL;
    }

    for (count=0; count < n; count++) {
	if (!s[count])
	    continue;

	if ((l=strlen(s[count])) > maxlen)
	    maxlen = l;
    }
    
    a = DXNewArray(TYPE_STRING, CATEGORY_REAL, 1, maxlen+1);
    if (!a)
	return NULL;

    if (!DXAddArrayData(a, 0, n, NULL))
	goto error;

    p = (char *)DXGetArrayData(a);
    if (!p)
	goto error;

    for (i=0; i<n; i++, p += maxlen+1)
	s[i] ? strncpy(p, s[i], maxlen+1) : memset(p, '\0', maxlen+1);

    return a;

  error:
    DXDelete((Object)a);
    return NULL;
}

static Array 
packit(int n, Type t, Category c, int rank, int *shape, Pointer p)
{
    Array a = NULL;

    a = DXNewArrayV(t, c, rank, shape);
    if (!a)
	return NULL;

    if (!DXAddArrayData(a, 0, n, p)) {
	DXDelete((Object)a);
	return NULL;
    }

    return a;
}

Array DXMakeInteger(int n)
{
    return packit(1, TYPE_INT, CATEGORY_REAL, 0, NULL, (Pointer)&n);
}

Array DXMakeFloat(float f)
{
    return packit(1, TYPE_FLOAT, CATEGORY_REAL, 0, NULL, (Pointer)&f);
}
