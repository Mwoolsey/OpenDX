/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <string.h>
#include <dx/dx.h>
#include "linesRR1DClass.h"

static void  _dxfCleanup(LinesRR1DInterpolator);
static Error _dxfInitializeTask(Pointer);
static Error _dxfInitialize(LinesRR1DInterpolator);

#define DIAGNOSTIC(str) \
	DXMessage("regular lines interpolator failure: %s", (str))

int
_dxfRecognizeLinesRR1D(Field field)
{
    Array array;
    int	  nDim;
    int   status;

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "lines");

    status = OK;

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
	return ERROR;
    else
    {
	if (!DXQueryGridPositions(array, &nDim, NULL, NULL, NULL))
	    return ERROR;

	if (nDim != 1)
	    return ERROR;
    }

    array = (Array)DXGetComponentValue(field, "connections");
    if (!array)
	return ERROR;
    else
    {
	if (!DXQueryGridConnections(array, &nDim, NULL))
	    return ERROR;
	
	if (nDim != 1)
	    return ERROR;
    }

    return status;
}

LinesRR1DInterpolator
_dxfNewLinesRR1DInterpolator(Field field, 
		enum interp_init initType, double fuzz, Matrix *m)
{
    return (LinesRR1DInterpolator)_dxf_NewLinesRR1DInterpolator(field, 
			initType, fuzz, m, &_dxdlinesrr1dinterpolator_class);
}

LinesRR1DInterpolator
_dxf_NewLinesRR1DInterpolator(Field field,
			enum interp_init initType, float fuzz, Matrix *m,
			struct linesrr1dinterpolator_class *class)
{
    LinesRR1DInterpolator	li;
    float	*mm, *MM;

    li = (LinesRR1DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
			(struct fieldinterpolator_class *)class);
    if (! li)
	return NULL;

    mm = ((Interpolator)li)->min;
    MM = ((Interpolator)li)->max;

    if ((MM[0] - mm[0]) == 0.0)
    {
	DXDelete((Object)li);
	return NULL;
    }

    li->dataArray   = NULL;
    li->pointsArray = NULL;
    li->data	    = NULL;

    if (initType == INTERP_INIT_PARALLEL)
    {
	if (! DXAddTask(_dxfInitializeTask, (Pointer)&li, sizeof(li), 1.0))
	{
	    DXDelete((Object)li);
	    return NULL;
	}
    }
    else if (initType == INTERP_INIT_IMMEDIATE)
    {
	if (! _dxfInitialize(li))
	{
	    DXDelete((Object)li);
	    return NULL;
	}
    }

    return li;
}

static Error
_dxfInitialize(LinesRR1DInterpolator li)
{
    Field			field;
    int				nDim;
    float			origin;
    float			delta;
    Type			dataType;
    Category			dataCategory;

    field = (Field)((Interpolator)li)->dataObject;

    /*
     * De-reference data
     */
    li->dataArray   = (Array)DXGetComponentValue(field, "data");
    if (!li->dataArray) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return ERROR;
    }
    DXReference((Object)li->dataArray);

    DXGetArrayInfo(li->dataArray, NULL, &((Interpolator)li)->type,
	&((Interpolator)li)->category, &((Interpolator)li)->rank, 
	((Interpolator)li)->shape);
    
    li->data = DXCreateArrayHandle(li->dataArray);
    if (! li->data)
	return ERROR;

    /*
     * Get the grid. 
     */
    li->pointsArray = (Array)DXGetComponentValue(field, "positions");
    if (!li->pointsArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return ERROR;
    }
    DXReference((Object)li->pointsArray);

    /*
     * get info about data values
     */
    DXGetArrayInfo(li->dataArray, NULL, &dataType, &dataCategory, NULL, NULL);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    li->nElements = DXGetItemSize(li->dataArray) /
				DXTypeSize(((Interpolator)li)->type);

    /*
     * The grid should be regular
     */
    DXQueryGridPositions(li->pointsArray, &nDim, &li->count, &origin, &delta);

    li->invDelta = 1.0 / delta;
    li->origin   = origin;

    /*
     * Figure fuzz as a proportion of primitive lenth
     */
    li->fieldInterpolator.fuzz *= delta;

    li->fieldInterpolator.initialized = 1;

    return OK;
}

Error
_dxfLinesRR1DInterpolator_Delete(LinesRR1DInterpolator li)
{
    _dxfCleanup(li);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) li);
}

int
_dxfLinesRR1DInterpolator_PrimitiveInterpolate(LinesRR1DInterpolator li, int *n,
				float **points, Pointer *values, int fuzzFlag)
{
    float    x;
    int      ix;
    float    dx;
    float    xMax, xMin;
    int	     ixMax;
    float    org;
    float    iD;
    float    w0, w1;
    int	     i;
    float    *p;
    Pointer  v;
    float    fuzz;
    int	     dep;
    int	     itemSize, typeSize;
    InvalidComponentHandle icH = ((FieldInterpolator)li)->invCon;
    ubyte    *dbuf;
    Matrix   *xform;

    if (! li->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(li))
	{
	    _dxfCleanup(li);
	    return 0;
	}

	li->fieldInterpolator.initialized = 1;
    }

    dep = li->fieldInterpolator.data_dependency;
    typeSize = DXTypeSize(((Interpolator)li)->type);
    itemSize = li->nElements * typeSize;

    /*
     * De-reference indexing info from interpolator
     */
    org = li->origin;
    iD  = li->invDelta;

    /*
     * De-reference bounding box
     */
    xMax = ((Interpolator)li)->max[0];
    xMin = ((Interpolator)li)->min[0];

    ixMax = li->count - 1;

    fuzz = li->fuzz;

    if (((FieldInterpolator)li)->xflag)
        xform = &(((FieldInterpolator)li)->xform);
    else
        xform = NULL;

    p = *points;
    v = *values;

    dbuf = (ubyte *)DXAllocate(2*itemSize);
    if (! dbuf)
	return 0;

    while (*n > 0)
    {
	float xpt;

	if (xform)
	    xpt = (*p)*xform->A[0][0] + xform->b[0];
	else
	    xpt = *p;

	if (fuzzFlag == FUZZ_ON)
	{
	    if (xpt < xMin-fuzz || xpt > xMax+fuzz) break;
	}
	else
	{
	    if (xpt < xMin || xpt > xMax) break;
	}

	x = (xpt - org) * iD;

#define INTERPOLATE(type, round)					\
{									\
    type *v0, *v1, *r;							\
									\
    r = (type *)v;							\
									\
    v0 = (type *)DXGetArrayEntry(li->data, ix, (Pointer)dbuf);		\
    v1 = (type *)DXGetArrayEntry(li->data,				\
				ix+1, (Pointer)(dbuf+itemSize));	\
									\
    for (i = 0; i < li->nElements; i++)					\
	*r++ = w1*(*v1++) + w0*(*v0++) + round;				\
									\
    v = (Pointer)r;							\
}

	if (((FieldInterpolator)li)->cstData)
	{
	    memcpy(v, ((FieldInterpolator)li)->cstData, itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
	else if (dep == DATA_POSITIONS_DEPENDENT)
	{
            Type dataType;

	    ix = x;
	    dx = x - ix;
	    if (ix >= ixMax)
	    {
		ix = ixMax - 1;
		dx = 1.0;
	    }
	    else if (ix < 0 || dx < 0.0)
	    {
		ix = 0;
		dx = 0.0;
	    }

	    w0 = 1.0 - dx;
	    w1 = dx;

	    if (icH && DXIsElementInvalid(icH, ix))
		break;

            if ((dataType = ((Interpolator)li)->type) == TYPE_FLOAT)
            {
                INTERPOLATE(float, 0.0);
            }
            else if (dataType == TYPE_DOUBLE)
            {
                INTERPOLATE(double, 0.0);
            }
            else if (dataType == TYPE_INT)
            {
                INTERPOLATE(int, 0.5);
            }
            else if (dataType == TYPE_SHORT)
            {
                INTERPOLATE(short, 0.5);
            }
            else if (dataType == TYPE_USHORT)
            {
                INTERPOLATE(ushort, 0.5);
            }
            else if (dataType == TYPE_UINT)
            {
                INTERPOLATE(uint, 0.5);
            }
            else if (dataType == TYPE_BYTE)
            {
                INTERPOLATE(byte, 0.5);
            }
            else if (dataType == TYPE_UBYTE)
            {
                INTERPOLATE(ubyte, 0.5);
            }
            else
            {
                INTERPOLATE(unsigned char, 0.5);
            }
	}
	else
	{
	    ix = x;
	    if (ix >= ixMax)
		ix = ixMax - 1;
	    else if (ix < 0)
		ix = 0;

	    if (icH && DXIsElementInvalid(icH, ix))
		break;

	    memcpy(v, 
		DXGetArrayEntry(li->data, ix, (Pointer)dbuf), itemSize);
	    v = ((char *)v) + itemSize;
	}

	/*
	 * only use fuzz on first point
	 */
	fuzzFlag = FUZZ_OFF;

	p += 1;
	(*n)--;
    }

    DXFree((Pointer)dbuf);

    *points = (float *)p;
    *values = v;

    return OK;
}

static void
_dxfCleanup(LinesRR1DInterpolator li)
{								
    if (li->data)
    {
	DXFreeArrayHandle(li->data);
	li->data = NULL;
    }

    if (li->pointsArray)
    {
	DXDelete((Object)li->pointsArray);
	li->pointsArray = NULL;
    }

    if (li->dataArray)
    {
	DXDelete((Object)li->dataArray);
	li->dataArray = NULL;
    }
}

Object
_dxfLinesRR1DInterpolator_Copy(LinesRR1DInterpolator old, enum _dxd_copy copy)
{
    LinesRR1DInterpolator new;

    new = (LinesRR1DInterpolator)
	_dxf_NewObject((struct object_class *)&_dxdlinesrr1dinterpolator_class);
    
    if (!(_dxf_CopyLinesRR1DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

LinesRR1DInterpolator
_dxf_CopyLinesRR1DInterpolator(LinesRR1DInterpolator new,
				    LinesRR1DInterpolator old, enum _dxd_copy copy)
{
    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->origin    = old->origin;
    new->invDelta  = old->invDelta;
    new->nElements = old->nElements;
    new->count     = old->count;
    new->fuzz      = old->fuzz;

    if (old->fieldInterpolator.initialized)
    {
	new->pointsArray = (Array)DXReference((Object)old->pointsArray);
	new->dataArray   = (Array)DXReference((Object)old->dataArray);
	new->data        = DXCreateArrayHandle(new->dataArray);
    }

    if (DXGetError())
 	return NULL;
    else
	return new;
}

static Error
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(LinesRR1DInterpolator *)p);
}

Interpolator
_dxfLinesRR1DInterpolator_LocalizeInterpolator(LinesRR1DInterpolator li)
{
    if (li->fieldInterpolator.localized)
	return (Interpolator)li;

    li->fieldInterpolator.localized = 1;

    return (Interpolator)li;
}
