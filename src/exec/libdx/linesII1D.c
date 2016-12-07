/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "linesII1DClass.h"

#define FALSE 0
#define TRUE 1

static Error _dxfInitialize(LinesII1DInterpolator);
static Error _dxfInitializeTask(Pointer);
static int   linear_coords(float, float, float, LinearCoord *, float);
static int   _dxfCleanup(LinesII1DInterpolator);

int
_dxfRecognizeLinesII1D(Field field)
{
    Array    array;
    int      rank, shape[32];
    Type     type;
    Category cat;

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "lines");

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return 0;
    }

    DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape);

    if (rank != 0 && (rank != 1 || shape[0] != 1))
    {
	DXSetError(ERROR_DATA_INVALID, "#11003", "lines", 1);
	return 0;
    }

    array = (Array)DXGetComponentValue(field, "connections");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return 0;
    }
    else
    {
	DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape);
    
	if (type != TYPE_INT)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11450");
	    return 0;
	}
	
	if (cat != CATEGORY_REAL)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11450");
	    return 0;
	}
    }
	
    return 1;
}

LinesII1DInterpolator
_dxfNewLinesII1DInterpolator(Field field,
			enum interp_init initType, double fuzz, Matrix *m) 
{
    return (LinesII1DInterpolator)_dxf_NewLinesII1DInterpolator(field, 
			initType, fuzz, m, &_dxdlinesii1dinterpolator_class);
}

LinesII1DInterpolator
_dxf_NewLinesII1DInterpolator(Field field,
			enum interp_init initType, float fuzz, Matrix *m,
			struct linesii1dinterpolator_class *class)
{
    LinesII1DInterpolator 	li;

    li = (LinesII1DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
	    (struct fieldinterpolator_class *)class);

    if (! li)
	return NULL;

    li->pointsArray	= NULL;
    li->dataArray	= NULL;
    li->dHandle		= NULL;
    li->linesArray	= NULL;

    li->hint = 0;

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
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(LinesII1DInterpolator *)p);
}

static Error
_dxfInitialize(LinesII1DInterpolator li)
{
    Field	field;
    float	len;
    int 	i;

    li->fieldInterpolator.initialized = 1;

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
		&((Interpolator)li)->category,
		&((Interpolator)li)->rank, ((Interpolator)li)->shape);
    
    li->dHandle = DXCreateArrayHandle(li->dataArray);
    if (! li->dHandle)
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

    li->pHandle = DXCreateArrayHandle(li->pointsArray);
    if (! li->pHandle)
	return ERROR;
    
    /*
     * Get the lines. 
     */
    li->linesArray = (Array)DXGetComponentValue(field, "connections");
    if (!li->linesArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return ERROR;
    }

    li->lHandle = DXCreateArrayHandle(li->linesArray);
    if (! li->lHandle)
	return ERROR;

    if (DXQueryGridConnections(li->linesArray, NULL, &li->nLines))
    {
	/*
	 * The above call returns the number of points in the sequence, not
	 * the number of segments.
	 */
	li->nLines -= 1;
    }
    else
    {
	Type     type;
	Category cat;
	int	 rank, shape[32];

	if (! DXGetArrayInfo(li->linesArray, &li->nLines,
					&type, &cat, &rank, shape))
	    return ERROR;
	
	if (type != TYPE_INT || cat != CATEGORY_REAL ||
					rank != 1 || shape[0] != 2)
	    return ERROR;

    }

    DXReference((Object)li->linesArray);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    li->nElements = DXGetItemSize(li->dataArray) /
			DXTypeSize(((Interpolator)li)->type);

    /*
     * Figure fuzz as a proportion of average primitive length
     */
    len = (((Interpolator)li)->max[0] - ((Interpolator)li)->min[0]);
    if (li->nLines)
	len /= li->nLines;
    else
	len = 0.0;

    li->fieldInterpolator.fuzz *= len;

    li->grid = _dxfMakeSearchGrid(((Interpolator)li)->max,
			((Interpolator)li)->min, li->nLines, 1);
    if (! li->grid)
	return 0;
    
    li->gridFlag = 1;

    for (i = 0;  i < li->nLines; i++)
    {
	int   *line, lbuf[2];
	float *pts[2], pBuf[2];

	line = (int *)DXGetArrayEntry(li->lHandle, i, (Pointer)lbuf);

	pts[0] = (float *)DXGetArrayEntry(li->pHandle, line[0], (Pointer)(pBuf+0));
	pts[1] = (float *)DXGetArrayEntry(li->pHandle, line[1], (Pointer)(pBuf+1));

	if (! _dxfAddItemToSearchGrid(li->grid, pts, 2, i))
	    return 0;
    }

    li->fieldInterpolator.initialized = 1;
    li->hint = -1;

    return OK;
}

Error
_dxfLinesII1DInterpolator_Delete(LinesII1DInterpolator li)
{
    _dxfCleanup(li);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) li);
}

int
_dxfLinesII1DInterpolator_PrimitiveInterpolate(LinesII1DInterpolator li,
		    int *n, float **points, Pointer *values, int fuzzFlag)
{
    float p0, p1;
    LinearCoord linesCoord;
    int seg;
    int i;
    int found;
    Pointer v;
    float *p;
    int i0 = 0, i1 = 0;
    float fuzz;
    int	dep;
    int itemSize;
    ubyte *dbuf;
    InvalidComponentHandle icH = li->fieldInterpolator.invCon;
    Matrix *xform;


    if (! li->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(li))
	{
	    _dxfCleanup(li);
	    return 0;
	}

	li->fieldInterpolator.initialized = 1;
    }

    v = *values;
    p = (float *)*points;

    fuzz = li->fieldInterpolator.fuzz;
    dep = li->fieldInterpolator.data_dependency;
    itemSize = li->nElements*DXTypeSize(((Interpolator)li)->type);

    dbuf = (ubyte *)DXAllocate(2*itemSize);
    if (! dbuf)
	return 0;

    if (((FieldInterpolator)li)->xflag)
	xform = &((FieldInterpolator)li)->xform;
    else
	xform = NULL;


    /*
     * For each point in the input, attempt to interpolate the point.
     * When a point cannot be interpolated, quit.
     */
    while(*n != 0)
    {
        float xpt;
        float *pPtr;

        if (xform)
        {
            xpt = (*p)*xform->A[0][0] + xform->b[0];

            pPtr = &xpt;
        }
        else
            pPtr = p;

	found = 0;
	seg = li->hint;

	/*
	 * If a hint, try it
	 */
	if (seg >= 0)
	{
	    int l, *line, lbuf[2];
	
	    line = (int *)DXGetArrayEntry(li->lHandle, seg, (Pointer)lbuf);

	    i0 = line[0];
	    i1 = line[1];

	    p0 = *(float *)DXGetArrayEntry(li->pHandle, i0, (Pointer)&p0);
	    p1 = *(float *)DXGetArrayEntry(li->pHandle, i1, (Pointer)&p1);

	    /*
	     * linear_coords will return 0 for inside (inclusive),
	     * +-1 for outside the fuzz zone, +-2 for inside the fuzz
	     * zone.
	     */
	    l = linear_coords(*pPtr, p0, p1, &linesCoord, fuzz);

	    if (l == 0 || ((l == 2 || l == -2) && fuzzFlag))
		found = 1;
	}
	
	if (! found)
	{
	    int l;

	    _dxfInitGetSearchGrid(li->grid, pPtr);
	    while ((seg = _dxfGetNextSearchGrid(li->grid)) != -1 && !found)
	    {
		int *line, lbuf[2];

		if ((icH && DXIsElementInvalid(icH, seg)))
		    continue;

		line = (int *)DXGetArrayEntry(li->lHandle, seg, (Pointer)lbuf);

		i0 = line[0];
		i1 = line[1];

		p0 = *(float *)DXGetArrayEntry(li->pHandle, i0, (Pointer)&p0);
		p1 = *(float *)DXGetArrayEntry(li->pHandle, i1, (Pointer)&p1);

		/*
		 * linear_coords will return 0 for inside (inclusive),
		 * +-1 for outside the fuzz zone, +-2 for inside the fuzz
		 * zone.
		 */
		l = linear_coords(*pPtr, p0, p1, &linesCoord, fuzz);

		if (l == 0 || ((l == 2 || l == -2) && fuzzFlag))
		    found = 1;
	    }
	}

	if (!found)
	    break;

	li->hint = seg;

#define INTERPOLATE(type, round)			 		       \
{									       \
    type *d0, *d1, *r;							       \
							   		       \
    r = (type *)v;							       \
							   		       \
    d0 = (type *)DXGetArrayEntry(li->dHandle, i0, (Pointer)dbuf);	       \
    d1 = (type *)DXGetArrayEntry(li->dHandle, i1,			       \
					(Pointer)(dbuf+itemSize));	       \
    for (i = 0; i < li->nElements; i++)					       \
	*r++ = (*d0++ * linesCoord.p) + (*d1++ * linesCoord.q) + round;	       \
									       \
    v = (Pointer)r;							       \
}

	if (((FieldInterpolator)li)->cstData)
	{
	    memcpy(v, ((FieldInterpolator)li)->cstData, itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
	else if (dep == DATA_POSITIONS_DEPENDENT)
	{
            Type dataType;

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
	    memcpy(v, DXGetArrayEntry(li->dHandle, seg, (Pointer)dbuf),
		itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}

	/*
	 * Only use fuzz first time around
	 */
	fuzzFlag = FUZZ_OFF;

	p ++;
	*n -= 1;
    }

    DXFree(dbuf);

    *values = (Pointer)v;
    *points = (float *)p;

    return OK;
}

static int
_dxfCleanup(LinesII1DInterpolator li)
{								
    if (li->lHandle)
    {
	DXFreeArrayHandle(li->lHandle);
	li->lHandle = NULL;
    }

    if (li->pHandle)
    {
	DXFreeArrayHandle(li->pHandle);
	li->pHandle = NULL;
    }

    if (li->dHandle)
    {
	DXFreeArrayHandle(li->dHandle);
	li->dHandle = NULL;
    }

    if (li->dataArray)
    {
	DXDelete((Object)li->dataArray);
	li->dataArray = NULL;
    }

    if (li->pointsArray)
    {
	DXDelete((Object)li->pointsArray);
	li->pointsArray = NULL;
    }

    if (li->linesArray)
    {
	DXDelete((Object)li->linesArray);
	li->linesArray = NULL;
    }

    if (li->gridFlag)
    {
	_dxfFreeSearchGrid(li->grid);
	li->gridFlag = 0;
    }

    return OK;
}

static int
linear_coords (float pt, float p0, float p1, LinearCoord *b, float fuzz)
{
    int code;
    float min, max;

    b->p = 0.0;
    b->q = 0.0;

    if (p0 > p1)
	min = p1, max = p0;
    else
	min = p0, max = p1;

    if (min-fuzz > pt)
	code = -1;
    else if (max+fuzz < pt)
	code =  1;
    else
    {
	if (p0 == p1)
	{
	    b->p = 1.0;
	    b->q = 0.0;
	}
	else
	{
	    b->q = (pt - p0) / (p1 - p0);
	    b->p = 1.0 - b->q;
	}

	if (p0 > pt)
	    code = -2;
	else if (p1 <= pt)
	    code = 2;
	else
	    code = 0;
    }

    return code;
}


Interpolator
_dxfLinesII1DInterpolator_LocalizeInterpolator(LinesII1DInterpolator li)
{
    if (li->fieldInterpolator.localized)
	return (Interpolator)li;

    li->fieldInterpolator.localized = 1;
    return (Interpolator)li;
}

Object
_dxfLinesII1DInterpolator_Copy(LinesII1DInterpolator old, enum _dxd_copy copy)
{
    LinesII1DInterpolator new;

    new = (LinesII1DInterpolator)
	_dxf_NewObject((struct object_class *)&_dxdlinesii1dinterpolator_class);
    
    if (!(_dxf_CopyLinesII1DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

LinesII1DInterpolator
_dxf_CopyLinesII1DInterpolator(LinesII1DInterpolator new,
				    LinesII1DInterpolator old, enum _dxd_copy copy)
{
    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nPoints    = old->nPoints;
    new->nLines     = old->nLines;
    new->nElements  = old->nElements;
    new->hint       = old->hint;

    if (old->fieldInterpolator.initialized)
    {
	new->pointsArray = (Array)DXReference((Object)old->pointsArray);
	new->linesArray  = (Array)DXReference((Object)old->linesArray);
	new->dataArray   = (Array)DXReference((Object)old->dataArray);

	new->lHandle = DXCreateArrayHandle(new->linesArray);
	if (! new->lHandle)
	    return NULL;

	new->pHandle = DXCreateArrayHandle(new->pointsArray);
	if (! new->pHandle)
	    return NULL;

	new->dHandle   = DXCreateArrayHandle(old->dataArray);
	if (! new->dHandle)
	    return NULL;

	if (old->gridFlag)
	{
	    new->gridFlag = 1;
	    new->grid = _dxfCopySearchGrid(old->grid);
	    if (! new->grid)
		 return NULL;
	}
	else
	{
	    new->gridFlag = 0;
	}
    }
    else
    {
	new->pointsArray = NULL;
	new->linesArray  = NULL;
	new->dataArray   = NULL;
	new->dHandle     = NULL;
	new->pHandle     = NULL;
    }

    if (DXGetError())
	return NULL;
    else
	return new;
}

