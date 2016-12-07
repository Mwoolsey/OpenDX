/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>


#include <stdio.h>
#include <math.h>
#include <string.h>
#include "trisRI2DClass.h"

#define WALK_LENGTH	5

#define FALSE 0
#define TRUE 1

static Error _dxfInitializeTask(Pointer);
static Error _dxfInitialize(TrisRI2DInterpolator);
static int   _dxftriangular_coords(float *, float *, 
		float *, float *, TriCoord *, float);
static int   _dxfCleanup(TrisRI2DInterpolator);
static int   _dxftriExit(int, TriCoord *, int *);
static int   _dxfTrisWalk(TrisRI2DInterpolator, float *, int, TriCoord *);
static int   _dxfTrisSearch(TrisRI2DInterpolator, float *, int, TriCoord *, int);

int
_dxfRecognizeTrisRI2D(Field field)
{
    Array    array;
    Type     t;
    Category c;

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "triangles");

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return 0;
    }

    DXGetArrayInfo(array, NULL, &t, &c, NULL, NULL);
	
    if (c != CATEGORY_REAL)
    {
	DXSetError(ERROR_DATA_INVALID, "#11150", "connections");
	return 0;
    }
	
    array = (Array)DXGetComponentValue(field, "connections");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return 0;
    }

    DXGetArrayInfo(array, NULL, &t, &c, NULL, NULL);
    
    if (t != TYPE_INT || c != CATEGORY_REAL)
    {
	DXSetError(ERROR_DATA_INVALID, "#11450");
	return 0;
    }
    
    return 1;
}

TrisRI2DInterpolator
_dxfNewTrisRI2DInterpolator(Field field,
		enum interp_init initType, double fuzz, Matrix *m) 
{
    return (TrisRI2DInterpolator)_dxf_NewTrisRI2DInterpolator(field, 
			    initType, fuzz, m, &_dxdtrisri2dinterpolator_class);
}

TrisRI2DInterpolator
_dxf_NewTrisRI2DInterpolator(Field field,
		enum interp_init initType, float fuzz, Matrix *m,
		struct trisri2dinterpolator_class *class)
{
    TrisRI2DInterpolator 	ti;
    float	*mm, *MM;

    ti = (TrisRI2DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
	    (struct fieldinterpolator_class *)class);

    if (! ti)
	return NULL;

    mm = ((Interpolator)ti)->min;
    MM = ((Interpolator)ti)->max;

    if (((MM[0] - mm[0]) * (MM[1] - mm[1])) == 0.0)
    {
	DXDelete((Object)ti);
	return NULL;
    }


    ti->pArray	= NULL;
    ti->pHandle	= NULL;
    ti->dArray	= NULL;
    ti->dHandle	= NULL;
    ti->tArray	= NULL;
    ti->nArray	= NULL;
    ti->grid	= NULL;

    ti->hint = -1;

    if (initType == INTERP_INIT_PARALLEL)
    {
	if (! DXAddTask(_dxfInitializeTask, (Pointer)&ti, sizeof(ti), 1.0))
	{
	    DXDelete((Object)ti);
	    return NULL;
	}

    }
    else if (initType == INTERP_INIT_IMMEDIATE)
    {
	if (! _dxfInitialize(ti))
	{
	    DXDelete((Object)ti);
	    return NULL;
	}
    }

    return ti;
}

static Error
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(TrisRI2DInterpolator *)p);
}

static Error
_dxfInitialize(TrisRI2DInterpolator ti)
{
    Field	field;
    Type	dataType;
    Category	dataCategory;
    int		i;
    int		*tri;
    float	len, area;

    ti->fieldInterpolator.initialized = 1;

    field = (Field)((Interpolator)ti)->dataObject;

    /*
     * De-reference data
     */
    ti->dArray   = (Array)DXGetComponentValue(field, "data");
    if (!ti->dArray) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return ERROR;
    }
    DXReference((Object)ti->dArray);

    DXGetArrayInfo(ti->dArray, NULL, &((Interpolator)ti)->type,
		&((Interpolator)ti)->category,
		&((Interpolator)ti)->rank, ((Interpolator)ti)->shape);

    ti->dHandle = DXCreateArrayHandle(ti->dArray);
    if (! ti->dHandle)
	return ERROR;


    /*
     * Get the grid. 
     */
    ti->pArray = (Array)DXGetComponentValue(field, "positions");
    if (!ti->pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return ERROR;
    }
    DXReference((Object)ti->pArray);

    ti->pHandle = DXCreateArrayHandle(ti->pArray);
    if (! ti->pHandle)
	return ERROR;

    /*
     * Get the triangles. 
     */
    ti->tArray = (Array)DXGetComponentValue(field, "connections");
    if (!ti->tArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return ERROR;
    }
    DXReference((Object)ti->tArray);

    DXGetArrayInfo(ti->tArray, &ti->nTriangles, NULL, NULL, NULL, NULL);

    /*
     * get info about data values
     */
    DXGetArrayInfo(ti->dArray, NULL, &dataType, &dataCategory, NULL, NULL);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    ti->nElements = DXGetItemSize(ti->dArray) / DXTypeSize(dataType);

    if (ti->fieldInterpolator.localized)
	ti->triangles = (Triangle *)DXGetArrayDataLocal(ti->tArray);
    else
	ti->triangles = (Triangle *)DXGetArrayData(ti->tArray);

    if (! ti->triangles)
	return ERROR;

    /*
     * Get the neighbors
     */
    ti->nArray = 
	DXNeighbors((Field)(ti->fieldInterpolator.interpolator.dataObject));
    if (! ti->nArray)
	return ERROR;

    DXReference((Object)ti->nArray);

    if (ti->fieldInterpolator.localized)
	ti->neighbors = (Triangle *)DXGetArrayDataLocal(ti->nArray);
    else
	ti->neighbors = (Triangle *)DXGetArrayData(ti->nArray);

    if (! ti->neighbors)
	return ERROR;

    if (ti->nTriangles)
    {
	area = (((Interpolator)ti)->max[0] - ((Interpolator)ti)->min[0]) *
		    (((Interpolator)ti)->max[1] - ((Interpolator)ti)->min[1]);
    
	area /= ti->nTriangles;

	len = sqrt(area);

	ti->fieldInterpolator.fuzz *= len;
    }

    /* 
     * Create the search grid
     */
    ti->grid = _dxfMakeSearchGrid(((Interpolator)ti)->max, 
			((Interpolator)ti)->min, ti->nTriangles, 2);
    if (! ti->grid)
	return ERROR;

    tri = (int *)ti->triangles;

    for (i = 0; i < ti->nTriangles; i++)
    {
	float *points[3], pbuf[6];
	register int j;

	for (j = 0; j < 3; j++)
	    points[j] = (float *)
		DXGetArrayEntry(ti->pHandle, *tri++, (Pointer)(pbuf+2*j));

	_dxfAddItemToSearchGrid(ti->grid, points, 3, i);
    }

    return OK;
}

Error
_dxfTrisRI2DInterpolator_Delete(TrisRI2DInterpolator ti)
{
    _dxfCleanup(ti);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) ti);
}

int
_dxfTrisRI2DInterpolator_PrimitiveInterpolate(TrisRI2DInterpolator ti,
		    int *n, float **points, Pointer *values, int fuzzFlag)
{
    int primNum;
    TriCoord triCoord;
    Triangle *tri;
    int i;
    int found;
    Pointer v;
    float *p;
    int dep;
    int itemSize;
    InvalidComponentHandle icH = ((FieldInterpolator)ti)->invCon;
    char *dbuf = NULL;
    Matrix *xform;

    if (! ti->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(ti))
	{
	    _dxfCleanup(ti);
	    return ERROR;
	}

	ti->fieldInterpolator.initialized = 1;
    }

    dep = ti->fieldInterpolator.data_dependency;
    itemSize = DXGetItemSize(ti->dArray);

    dbuf = (char *)DXAllocate(3*itemSize);
    if (! dbuf)
	return ERROR;
    
    if (((FieldInterpolator)ti)->xflag)
	xform = &(((FieldInterpolator)ti)->xform);
    else
	xform = NULL;

    v = *values;
    p = (float *)*points;

    /*
     * For each point in the input, attempt to interpolate the point.
     * When a point cannot be interpolated, quit.
     */
    while(*n != 0)
    {
	float xpt[2];
	float *pPtr;

	if (xform)
	{
	    xpt[0] = p[0]*xform->A[0][0] +
		     p[1]*xform->A[1][0] +
		          xform->b[0];

	    xpt[1] = p[0]*xform->A[0][1] +
		     p[1]*xform->A[1][1] +
		          xform->b[1];
	    pPtr = xpt;
	}
	else
	    pPtr = p;


	found = -1;

	/*
	 * Check the hint first, if one exists...
	 */
	if (ti->hint != -1)
	    found = _dxfTrisWalk(ti, pPtr, ti->hint, &triCoord);

	/*
	 * If it wasn't found,  try the search grid
	 */
	if (found == -1)
	{
	    _dxfInitGetSearchGrid(ti->grid, (float *)pPtr);
	    while((primNum = _dxfGetNextSearchGrid(ti->grid)) != -1 && found == -1)
                found = _dxfTrisSearch(ti,
				pPtr, primNum, &triCoord, fuzzFlag);
	}

	if (found == -1 || (icH && DXIsElementInvalid(icH, found)))
	    break;

	ti->hint = found;

#define INTERPOLATE(type, round)				\
{								\
    type *d0, *d1, *d2, *r;					\
								\
    tri = ti->triangles + found;				\
    r = (type *)v;						\
								\
    d0 = (type *)DXGetArrayEntry(ti->dHandle, tri->p, 		\
				(Pointer)dbuf);			\
    d1 = (type *)DXGetArrayEntry(ti->dHandle, tri->q, 		\
				(Pointer)(dbuf+itemSize));	\
    d2 = (type *)DXGetArrayEntry(ti->dHandle, tri->r, 		\
				(Pointer)(dbuf+2*itemSize));	\
    for (i = 0; i < ti->nElements; i++)				\
	*r++ = (*d0++ * triCoord.p) + 				\
		    (*d1++ * triCoord.q) + 			\
			    (*d2++ * triCoord.r) + round;	\
								\
    v = (Pointer)r;						\
}

	if (((FieldInterpolator)ti)->cstData)
	{
	    memcpy(v, ((FieldInterpolator)ti)->cstData, itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
	else if (dep == DATA_POSITIONS_DEPENDENT)
	{
            Type dataType;

            if ((dataType = ((Interpolator)ti)->type) == TYPE_FLOAT)
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
	    memcpy(v, DXGetArrayEntry(ti->dHandle,
						found, dbuf), itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}

	
	/*
	 * Only use fuzz on first point
	 */
	fuzzFlag = FUZZ_OFF;

	p += 2;
	*n -= 1;
    }

    *values = v;
    *points = (float *)p;

    return OK;
}

static int
_dxfCleanup(TrisRI2DInterpolator ti)
{								
    if (ti->fieldInterpolator.localized)
    {
	if (ti->neighbors)
	    DXFreeArrayDataLocal(ti->nArray, (Pointer)ti->neighbors);
	if (ti->triangles)
	    DXFreeArrayDataLocal(ti->tArray, (Pointer)ti->triangles);
    }

    ti->neighbors = NULL;
    ti->triangles = NULL;

    if (ti->pHandle)
    {
	DXFreeArrayHandle(ti->pHandle);
	ti->pHandle = NULL;
    }

    if (ti->dHandle)
    {
	DXFreeArrayHandle(ti->dHandle);
	ti->dHandle = NULL;
    }

    if (ti->dArray)
    {
	DXDelete((Object)ti->dArray);
	ti->dArray = NULL;
    }

    if (ti->pArray)
    {
	DXDelete((Object)ti->pArray);
	ti->pArray = NULL;
    }

    if (ti->nArray)
    {
	DXDelete((Object)ti->nArray);
	ti->nArray = NULL;
    }

    if (ti->tArray)
    {
	DXDelete((Object)ti->tArray);
	ti->tArray = NULL;
    }

    if (ti->grid)
    {
	_dxfFreeSearchGrid(ti->grid);
	ti->grid = NULL;
    }

    return OK;
}

#define ON_EDGE(q, r, s) \
    (((q) == 0.0) && ((((r) <= 0.0) && ((s) <= 0.0)) || (((r) >= 0.0) && ((s) >= 0.0))))

static int _dxftriangular_coords (float *pt, float *p0, 
			float *p1, float *p2, TriCoord *b, float fuzz)
{
    double 	a, a0, a1, a2;
    double	x, x0, x1, x2;
    double	y, y0, y1, y2;

    x  = pt[0];    y  = pt[1];
    x0 = p0[0];    y0 = p0[1];
    x1 = p1[0];    y1 = p1[1];
    x2 = p2[0];    y2 = p2[1];

    a  = (x0*y1 + x1*y2 + x2*y0 - y0*x1 - y1*x2 - y2*x0) / 2.0;

    a0 = (x *y1 + x1*y2 + x2*y  - y *x1 - y1*x2 - y2*x ) / 2.0;
    a1 = (x0*y  + x *y2 + x2*y0 - y0*x  - y *x2 - y2*x0) / 2.0;
    a2 = (x0*y1 + x1*y  + x *y0 - y0*x1 - y1*x  - y *x0) / 2.0;

    b->p = a0 / a;
    b->q = a1 / a;
    b->r = a2 / a;

    if (ON_EDGE(a0, a1, a2) || ON_EDGE(a1, a2, a0) || ON_EDGE(a2, a0, a1))
        return 0;

    if ((b->p >=  0.0 && b->q >=  0.0 && b->r >=  0.0) ||
	(b->p <=  0.0 && b->q <=  0.0 && b->r <=  0.0))
    {
	return 0;
    }
    else if ((b->p >= -fuzz && b->q >= -fuzz && b->r >= -fuzz) ||
	     (b->p <=  fuzz && b->q <=  fuzz && b->r <=  fuzz))
    {
	if (a > 0)
	    return 2;
	else
	    return -2;
    }
    else if (a > 0)
    {
	return 1;
    }
    else
    {
	return -1;
    }
}

static int
_dxftriExit(int side, TriCoord *be, int *face)
{
    float best;		/* best choice so far */
    int f;

    best = 0;
    f = -1;

    if (side < 0)
    {
	if (be->p > best)
	{
	    best = be->p;
	    f = 0;
	}
	if (be->q > best)
	{
	    best = be->q;
	    f = 1;
	}
	if (be->r > best)
	{
	    best = be->r;
	    f = 2;
	}
    }
    else
    {
	if (be->p < best)
	{
	    best = be->p;
	    f = 0;
	}

	if (be->q < best)
	{
	    best = be->q;
	    f = 1;
	}
	if (be->r < best)
	{
	    best = be->r;
	    f = 2;
	}
    }

    if (f == -1)
	return 0;

    *face = f;
    return 1;
}

static int
_dxfTrisSearch(TrisRI2DInterpolator ti, float *point, 
			int triIndex, TriCoord *triCoord, int fuzzFlag)
{
    int  	face;
    float	*p0, *p1, *p2;
    int  	side;
    Triangle	*tri;
    float	pbuf[6];

    while (triIndex != -1)
    {
	tri = ti->triangles + triIndex;

	p0 = (float *)DXGetArrayEntry(ti->pHandle,
					tri->p, (Pointer)(pbuf));
	p1 = (float *)DXGetArrayEntry(ti->pHandle,
					tri->q, (Pointer)(pbuf+2));
	p2 = (float *)DXGetArrayEntry(ti->pHandle,
					tri->r, (Pointer)(pbuf+4));

	side = _dxftriangular_coords(point, p0, p1, p2, triCoord, 
						ti->fieldInterpolator.fuzz);

	if (side == 0)
	{
	    return triIndex;
	}

	else if (fuzzFlag == FUZZ_ON && (side == 2 || side == -2))
	{
	    if (!_dxftriExit(side, triCoord, &face))
	    {
		return triIndex;
	    }
	}

	if (!_dxftriExit(side, triCoord, &face))
	{
	    return -1;
	}

	triIndex = ((int *)(ti->neighbors))[(triIndex*3) + face];
    };


    return -1;
}

static int
_dxfTrisWalk(TrisRI2DInterpolator ti, float *point,
				int triIndex, TriCoord *triCoord)
{
    int  	face;
    float	*p0, *p1, *p2;
    int  	side;
    Triangle	*tri;
    float	pbuf[6];

    while (triIndex != -1)
    {
	tri = ti->triangles + triIndex;

	p0 = (float *)DXGetArrayEntry(ti->pHandle,
					tri->p, (Pointer)(pbuf));
	p1 = (float *)DXGetArrayEntry(ti->pHandle,
					tri->q, (Pointer)(pbuf+2));
	p2 = (float *)DXGetArrayEntry(ti->pHandle,
					tri->r, (Pointer)(pbuf+4));

	side = _dxftriangular_coords(point, p0, p1, p2, triCoord,
						ti->fieldInterpolator.fuzz);

	if (side == 0)
	    return triIndex;
	
	if (!_dxftriExit(side, triCoord, &face))
	    return -1;

	triIndex = ((int *)(ti->neighbors))[(triIndex*3) + face];
    };

    return -1;
}

Object
_dxfTrisRI2DInterpolator_Copy(TrisRI2DInterpolator old, enum _dxd_copy copy)
{
    TrisRI2DInterpolator new;

    new = (TrisRI2DInterpolator)
	    _dxf_NewObject((struct object_class *)&_dxdtrisri2dinterpolator_class);

    if (!(_dxf_CopyTrisRI2DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

TrisRI2DInterpolator
_dxf_CopyTrisRI2DInterpolator(TrisRI2DInterpolator new, 
				TrisRI2DInterpolator old, enum _dxd_copy copy)
{

    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nPoints    = old->nPoints;
    new->nTriangles = old->nTriangles;
    new->nElements  = old->nElements;
    new->hint       = old->hint;

    if (new->fieldInterpolator.initialized)
    {
	new->pArray    = (Array)DXReference((Object)old->pArray);
	new->tArray = (Array)DXReference((Object)old->tArray);
	new->dArray      = (Array)DXReference((Object)old->dArray);
	new->nArray = (Array)DXReference((Object)old->nArray);

	if (new->fieldInterpolator.localized)
	{
	    new->triangles = (Triangle *)DXGetArrayDataLocal(new->tArray);
	    new->neighbors = (Triangle *)DXGetArrayDataLocal(new->nArray);
	}
	else
	{
	    new->triangles = (Triangle *)DXGetArrayData(new->tArray);
	    new->neighbors = (Triangle *)DXGetArrayData(new->nArray);
	}

	new->pHandle = DXCreateArrayHandle(new->pArray);
	new->dHandle = DXCreateArrayHandle(new->dArray);
	if (! new->pHandle || ! new->dHandle)
	    return NULL;

	new->grid = _dxfCopySearchGrid(old->grid);
	if (! new->grid)
	    return NULL;
    }
    else
    {
	new->pArray    = NULL;
	new->tArray    = NULL;
	new->dArray    = NULL;
	new->nArray    = NULL;
	new->pHandle   = NULL;
	new->triangles = NULL;
	new->dHandle   = NULL;
	new->grid      = NULL;
    }

    if (DXGetError())
	return NULL;

    return new;
}

Interpolator
_dxfTrisRI2DInterpolator_LocalizeInterpolator(TrisRI2DInterpolator ti)
{
    if (ti->fieldInterpolator.localized)
	return (Interpolator)ti;

    ti->fieldInterpolator.localized = 1;

    if (ti->fieldInterpolator.initialized)
    {
        ti->triangles = (Triangle *)DXGetArrayDataLocal(ti->tArray);
        ti->neighbors = (Triangle *)DXGetArrayDataLocal(ti->nArray);
    }

    if (DXGetError())
        return NULL;
    else
        return (Interpolator)ti;
}
