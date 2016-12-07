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
#include "linesRI1DClass.h"

#define FALSE 0
#define TRUE 1

static Error _dxfInitialize(LinesRI1DInterpolator);
static Error _dxfInitializeTask(Pointer);
static int   linear_coords(float, float, float, LinearCoord *, float);
static int   _dxfCleanup(LinesRI1DInterpolator);

int
_dxfRecognizeLinesRI1D(Field field)
{
    Array    array;
    int      nDim;
    int      rank, shape[32];
    Type     type;
    Category cat;
    int      nPts;

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "lines");

    if (DXGetComponentValue(field, "invalid connections") ||
		DXGetComponentValue(field, "invalid positions"))
	return 0;

    array = (Array)DXGetComponentValue(field, "connections");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return 0;
    }
    
    if (! DXQueryGridConnections(array, &nDim, &nPts))
	return 0;
    
    if (nDim != 1)
    {
	DXSetError(ERROR_DATA_INVALID, "#11004", "lines", 2);
	return 0;
    }

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return 0;
    }

    if (DXQueryGridPositions(array, &rank, NULL, NULL, NULL))
    {
	if (rank != 1)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11003", 
					    "lines", 1);
	    return 0;
	}
    }
    else
    {
	float *pts = (float *)DXGetArrayData(array);
	int  i, n;

	DXGetArrayInfo(array, &n, &type, &cat, &rank, shape);

	if (rank != 0 && (rank != 1 || shape[0] != 1))
	{
	    DXSetError(ERROR_DATA_INVALID, "#11003", "lines", 1);
	    return 0;
	}

	if (nPts > n)
	{
	    DXSetError(ERROR_DATA_INVALID, "not enough positions");
	    return 0;
	}

	if (nPts > 2)
	{
	    for (i = 1; i < nPts; i++)
		if (pts[i] != pts[i-1])
		    break;

	    if (i != nPts)
	    {
		if (pts[i-1] > pts[i])
		{
		    for (i = 2; i < nPts; i++)
			if (pts[i-1] < pts[i])
			{
			    DXSetError(ERROR_DATA_INVALID, "non-monotonic map");
			    return 0;
			}
		}
		else
		{
		    for (i = 2; i < nPts; i++)
			if (pts[i-1] > pts[i])
			{
			    DXSetError(ERROR_DATA_INVALID, "non-monotonic map");
			    return 0;
			}
		}
	    }
	}
    }
	
    return 1;
}

LinesRI1DInterpolator
_dxfNewLinesRI1DInterpolator(Field field,
			enum interp_init initType, double fuzz, Matrix *m) 
{
    return (LinesRI1DInterpolator)_dxf_NewLinesRI1DInterpolator(field, 
			initType, fuzz, m, &_dxdlinesri1dinterpolator_class);
}

LinesRI1DInterpolator
_dxf_NewLinesRI1DInterpolator(Field field,
			enum interp_init initType, float fuzz, Matrix *m,
			struct linesri1dinterpolator_class *class)
{
    LinesRI1DInterpolator 	li;

    li = (LinesRI1DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
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
    return _dxfInitialize(*(LinesRI1DInterpolator *)p);
}

static Error
_dxfInitialize(LinesRI1DInterpolator li)
{
    Field	field;
    int		seg, i, i0, i1, dir;
    float	p0, p1;
    float	len;

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

    if (DXQueryGridConnections(li->linesArray, NULL, &li->nLines))
    {
	li->linesArray = NULL;

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

	DXReference((Object)li->linesArray);
    }

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    li->nElements = DXGetItemSize(li->dataArray) /
			DXTypeSize(((Interpolator)li)->type);

    if (li->fieldInterpolator.localized)
    {
	if (li->linesArray)
	    li->lines  = (int *)DXGetArrayDataLocal(li->linesArray);
	else
	    li->lines  = NULL;
    }
    else
    {
	if (li->linesArray)
	    li->lines  = (int *)DXGetArrayDataLocal(li->linesArray);
	else
	    li->lines  = NULL;
    }

    if (DXGetError())
	return ERROR;

    /*
     * Check to verify that the positions are monotonic.  If so,
     * remember which way they go.
     */
    dir = 0; 
    for (seg = 0; seg < li->nLines; seg++)
    {
	i = seg << 1;

	if (li->lines)
	{
	    i0 = li->lines[i];
	    i1 = li->lines[i+1];
	}
	else
	{
	    i0 = seg;
	    i1 = seg + 1;
	}

	p0 = *(float *)DXGetArrayEntry(li->pHandle, i0, (Pointer)&p0);
	p1 = *(float *)DXGetArrayEntry(li->pHandle, i1, (Pointer)&p1);

	if (p0 < p1)
	{
	    if (dir == -1)
	    {
		DXSetError(ERROR_DATA_INVALID, "#11851");
		return ERROR;
	    }
	    else 
		dir     = 1;
	}
	else if (p0 > p1)
	{
	    if (dir == 1)
	    {
		DXSetError(ERROR_DATA_INVALID, "#11851");
		return ERROR;
	    }
	    else
		dir = -1;
	}
    }

    li->direction = dir;

    /*
     * Figure fuzz as a proportion of average primitive length
     */
    len = (((Interpolator)li)->max[0] - ((Interpolator)li)->min[0]);
    if (li->nLines)
	len /= li->nLines;
    else
	len = 0.0;

    li->fieldInterpolator.fuzz *= len;

    li->fieldInterpolator.initialized = 1;

    return OK;
}

Error
_dxfLinesRI1DInterpolator_Delete(LinesRI1DInterpolator li)
{
    _dxfCleanup(li);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) li);
}

int
_dxfLinesRI1DInterpolator_PrimitiveInterpolate(LinesRI1DInterpolator li,
		    int *n, float **points, Pointer *values, int fuzzFlag)
{
    float p0, p1;
    LinearCoord linesCoord;
    int seg;
    int i;
    int found;
    int outside;
    Pointer v;
    float *p;
    int dir;
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

	seg = li->hint;

	/*
	 * If no hint, start in the middle.
	 */
	if (seg < 0)
	    seg = li->nLines >> 1;

	found   = FALSE;
	outside = FALSE;
	dir     = 1;

	while (!found && !outside)
	{
	    i = seg << 1;

	    if (li->lines)
	    {
		i0 = li->lines[i];
		i1 = li->lines[i+1];
	    }
	    else
	    {
		i0 = seg;
		i1 = seg + 1;
	    }

	    p0 = *(float *)DXGetArrayEntry(li->pHandle, i0, (Pointer)&p0);
	    p1 = *(float *)DXGetArrayEntry(li->pHandle, i1, (Pointer)&p1);

	    if (li->direction == -1)
	    {
		float t = p0;
		p0 = p1;
		p1 = t;
	    }

	    if (p0 == p1)
	    {
		if (p0 == *pPtr)
		{
		    linesCoord.p = 1.0;
		    linesCoord.q = 0.0;
		    found = 1;
		    break;
		}
		else if (*pPtr > p0)
		    dir = li->direction;
		else
		    dir = -li->direction;
	    }
	    else 
	    {
		/*
		 * linear_coords will return 0 for inside (inclusive),
		 * +-1 for outside the fuzz zone, +-2 for inside the fuzz
		 * zone.
		 */
		if (! (dir=linear_coords(*pPtr, p0, p1, &linesCoord, fuzz)))
		{
		    found = 1;
		    break;
		}

		if (li->direction == -1)
		    dir = -dir;
		
		/* 
		 * If we are inside the fuzz zone and in the corresponding
		 * extreme of the set of segments, we are done.
		 */
		if (fuzzFlag == FUZZ_ON &&
		    ((dir == -2 && seg == 0) || 
		     (dir == 2 && seg == (li->nLines-1))))
		{
		    found = 1;
		    break;
		}
	    }

	    if (dir < 0)
		seg --;
	    else
		seg ++;
		    
	    if (seg < 0 || seg >= li->nLines) {
		if (dir == 2 || dir == -2)
		{
		    seg -= dir;
		    outside = FALSE;
		    found = 1;
		}
		else
		    outside = TRUE;
	    }
	}

	if (outside == TRUE || (icH && DXIsElementInvalid(icH, seg)))
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
_dxfCleanup(LinesRI1DInterpolator li)
{								
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

    if (li->lines)
    {
	DXFreeArrayDataLocal(li->linesArray, (Pointer)li->lines);
	li->lines = NULL;
    }

    if (li->linesArray)
    {
	DXDelete((Object)li->linesArray);
	li->linesArray = NULL;
    }

    return OK;
}

static int
linear_coords (float pt, float p0, float p1, LinearCoord *b, float fuzz)
{
    int code;

    b->p = 0.0;
    b->q = 0.0;

    if (p0-fuzz > pt)
	code = -1;
    else if (p1+fuzz < pt)
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
_dxfLinesRI1DInterpolator_LocalizeInterpolator(LinesRI1DInterpolator li)
{
    if (li->fieldInterpolator.localized)
	return (Interpolator)li;

    li->fieldInterpolator.localized = 1;

    if (li->fieldInterpolator.initialized)
    {
        li->lines  = (int   *)DXGetArrayDataLocal(li->linesArray);
    }
    return (Interpolator)li;
}

Object
_dxfLinesRI1DInterpolator_Copy(LinesRI1DInterpolator old, enum _dxd_copy copy)
{
    LinesRI1DInterpolator new;

    new = (LinesRI1DInterpolator)
	_dxf_NewObject((struct object_class *)&_dxdlinesri1dinterpolator_class);
    
    if (!(_dxf_CopyLinesRI1DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

LinesRI1DInterpolator
_dxf_CopyLinesRI1DInterpolator(LinesRI1DInterpolator new,
				    LinesRI1DInterpolator old, enum _dxd_copy copy)
{
    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nPoints    = old->nPoints;
    new->nLines     = old->nLines;
    new->nElements  = old->nElements;
    new->hint       = old->hint;
    new->direction  = old->direction;

    if (new->fieldInterpolator.initialized)
    {
	new->pointsArray = (Array)DXReference((Object)old->pointsArray);
	new->linesArray  = (Array)DXReference((Object)old->linesArray);
	new->dataArray   = (Array)DXReference((Object)old->dataArray);

	if (new->fieldInterpolator.localized)
	{
	    new->lines  = (int   *)DXGetArrayDataLocal(new->linesArray);
	}
	else
	{
	    new->lines  = (int   *)DXGetArrayData(new->linesArray);
	}

	new->pHandle = DXCreateArrayHandle(new->pointsArray);
	if (! new->pHandle)
	    return NULL;

	new->dHandle   = DXCreateArrayHandle(old->dataArray);
	if (! new->dHandle)
	    return NULL;
    }
    else
    {
	new->pointsArray = NULL;
	new->linesArray  = NULL;
	new->dataArray   = NULL;
	new->lines       = NULL;
	new->dHandle     = NULL;
	new->pHandle     = NULL;
    }

    if (DXGetError())
	return NULL;
    else
	return new;
}

