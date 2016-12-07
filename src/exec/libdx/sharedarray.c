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

#if defined(HAVE_SHMAT)

#if defined(HAVE_SYS_SHM_H)
#include <sys/shm.h>
#endif

struct _shmtab {
    int     id;	  /* Segment id		     			*/ 
    Pointer base; /* Base address of segment 			*/
    int	    count; /* count of objects referencing the segment	*/

    /*
     * If desired, the user can register a callback to occur when 
     * the segment is released.  The data is his.
     */
    void    (*onRelease)(int id, Pointer base, Pointer data);
    Pointer data;
};

static HashTable shmtab = NULL;

Error
_newSharedSegment(int id, void (*sor)(int, Pointer, Pointer), Pointer d)
{
    struct _shmtab *sptr, s;

    if (! shmtab)
    {
        shmtab = DXCreateHash(sizeof(struct _shmtab), NULL, NULL);
	if (! shmtab)
	    return ERROR;
    }

    sptr = DXQueryHashElement(shmtab, (Key)&id);
    if (sptr)
    {
        DXSetError(ERROR_INTERNAL, "_newSharedSegment called on known segment");
	return ERROR;
    }

    s.id    	= id;
    s.base  	= (Pointer)shmat(id, 0, 0);
    s.count 	= 0;
    s.onRelease = sor;
    s.data      = d;
    if (! DXInsertHashElement(shmtab, (Element)&s))
	return ERROR;
    return OK;
}

/*
 * This will register the segment (if not already registered) and
 * return its base.
 */
static Pointer
_getBase(int id)
{
    struct _shmtab *sptr;

    if (! shmtab || NULL == (sptr = DXQueryHashElement(shmtab, (Key)&id)))
        if (! _newSharedSegment(id, NULL, NULL))
	    return NULL;

    sptr = DXQueryHashElement(shmtab, (Key)&id);
    if (! sptr)
    {
        DXSetError(ERROR_INTERNAL, "unable to access known shared memory segment");
	return ERROR;
    }

    return sptr->base;
}

/*
 * Can be called to explicity map a new shared memory segment to install
 * a callback to be called when the last SharedArray referencing memory
 * in that segment is deleted.
 */
Error
DXRegisterSharedSegment(int id, void (*sor)(int, Pointer, Pointer), Pointer d)
{
    /*  DXQueryHashElement returns a "struct _shmtab *"  */
    if (! shmtab || (NULL == DXQueryHashElement(shmtab, (Key)&id)))
        if (! _newSharedSegment(id, sor, d))
	    return ERROR;

    return OK;
}

/*
 * Called on NewSharedArrayV.  Make sure its registered, then
 * bump its object count. 
 */
static Pointer
_referenceSegment(int id)
{
    struct _shmtab *sptr;

    if (! shmtab || (NULL == (sptr = DXQueryHashElement(shmtab, (Key)&id))))
        if (! _newSharedSegment(id, NULL, NULL))
	    return NULL;

    sptr = DXQueryHashElement(shmtab, (Key)&id);
    if (! sptr)
    {
        DXSetError(ERROR_INTERNAL, "unable to access known shared memory segment");
	return ERROR;
    }

    sptr->count++;
    return sptr->base;
}

/*
 * Called on DeleteSharedArray.  Decrement segment object count and
 * if it goes to 0, disattach.
 */
static Error
_unreferenceSegment(int id)
{
    struct _shmtab *sptr;

    if (! shmtab || (NULL == (sptr = DXQueryHashElement(shmtab, (Key)&id))))
    {
        DXSetError(ERROR_INTERNAL,
		"unreferenceing an unknown shared memory segment");
	return ERROR;
    }

    sptr->count --;
    if (sptr->count < 0 || (sptr->count == 0 && sptr->base == 0))
    {
        DXSetError(ERROR_INTERNAL,
		"unreferenceing an unmapped shared memory segment");
	return ERROR;
    }

    if (sptr->count == 0)
    {
        shmdt(sptr->base);
	if (sptr->onRelease)
	   (*sptr->onRelease)(id, sptr->base, sptr->data);
	sptr->base 	= 0;
	sptr->onRelease = NULL;
	sptr->data 	= NULL;
    }

    return OK;
}

SharedArray
_dxf_NewSharedArrayV(int id, Pointer d, int knt, Type t, Category c, int r, int *s,
				struct sharedarray_class *class)
{
    Pointer base = _referenceSegment(id);

    if (! base)
        return NULL;
    else
    {
	SharedArray a = (SharedArray) _dxf_NewArrayV(t, CATEGORY_REAL, r, s,
				    (struct array_class *)class);
	if (!a)
	    return NULL;

	a->array.items = knt;
	a->id = id;
	a->base = base;
	a->offset = ((long)d) - ((long)base);

	return a;
    }
}

SharedArray
DXNewSharedArrayV(int id, Pointer d, int k, Type t, Category c, int r, int *s)
{
    return _dxf_NewSharedArrayV(id, d, k, t, c, r, s, &_dxdsharedarray_class);
}

SharedArray
DXNewSharedArray(int id, Pointer d, int k, Type t, Category c, int r, ...)
{
    int shape[100];
    int i;
    va_list arg;

    ASSERT(r<100);

    /* collect args */
    va_start(arg,r);
    for (i=0; i<r; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    /* call V version */
    return DXNewSharedArrayV(id, d, k, t, c, r, shape);
}

Error
_dxfSharedArray_Delete(SharedArray a)
{
    a->array.data = NULL;
    _unreferenceSegment(a->id);
    return _dxfArray_Delete((Array)a);
}

Pointer
_dxfSharedArray_GetArrayData(SharedArray a)
{
    return (Pointer)(((char *)a->base) + a->offset);
}

Error
_dxfGetSharedArrayInfo(SharedArray a, int *id, long *offset)
{
    if (DXGetObjectClass((Object)a) != CLASS_ARRAY ||
        DXGetArrayClass((Array)a) != CLASS_SHAREDARRAY)
    {
        DXSetError(ERROR_INTERNAL,
	    "_dxfGetSharedArrayInfo on object that is not a CLASS_SHAREDARRAY");
	return ERROR;
    }

    if (id) 	*id     = a->id;
    if (offset) *offset = a->offset;
    return OK;
}

SharedArray
DXNewSharedArrayFromOffsetV(int id, long offset, int k, Type t, Category c, int r, int *s)
{
    Pointer data, base = _getBase(id);
    if (!base)
        return NULL;

    data = (Pointer)(((char *)base) + offset);
    
    return DXNewSharedArrayV(id, data, k, t, c, r, s);
}

SharedArray
DXNewSharedArrayFromOffset(int id, long offset, int k, Type t, Category c, int r, ...)
{
    int shape[100];
    int i;
    va_list arg;

    ASSERT(r<100);

    /* collect args */
    va_start(arg,r);
    for (i=0; i<r; i++)
	shape[i] = va_arg(arg, int);
    va_end(arg);

    /* call V version */
    return DXNewSharedArrayFromOffsetV(id, offset, k, t, c, r, shape);
}

#else

Error
_newSharedSegment(int id, void (*sor)(int, Pointer, Pointer), Pointer d)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

Error
DXRegisterSharedSegment(int id, void (*sor)(int, Pointer, Pointer), Pointer d)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

SharedArray
_dxf_NewSharedArrayV(int id, Pointer d, int knt, Type t, Category c, int r, int *s,
				struct sharedarray_class *class)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

SharedArray
DXNewSharedArrayV(int id, Pointer d, int knt, Type t, Category c, int r, int *s)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

SharedArray
DXNewSharedArray(int id, Pointer d, int knt, Type t, Category c, int r, ...)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

Error
_dxfSharedArray_Delete(SharedArray a)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

Pointer
_dxfSharedArray_GetArrayData(SharedArray a)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return NULL;
}

Error
_dxfGetSharedArrayInfo(SharedArray a, int *id, long *offset)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return ERROR;
}

SharedArray
DXNewSharedArrayFromOffsetV(int id, long offset, int knt, Type t, Category c, int r, int *s)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return NULL;
}

SharedArray
DXNewSharedArrayFromOffset(int id, long offset, int knt, Type t, Category c, int r, ...)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "shared array support requires shmat");
    return NULL;
}

#endif
