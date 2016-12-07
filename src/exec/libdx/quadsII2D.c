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
#include "quadsII2DClass.h"

#define WALK_LIMIT		32

#define BIN_SEARCH_LIMIT	1

#define FALSE 0
#define TRUE 1

typedef struct 
{
    ArrayHandle	  pHandle;
    int		  *neighbors;
    ArrayHandle   qHandle;
    int		  *visited;
    int		  vCount;
    float	  *point;
    float	  *quadPoints[4];
    int		  quad[4];
    QuadCoord	  *quadCoord;
    float	  fuzz;
    int		  fuzzFlag;
    int 	  limit;
    int		  gridCounts[2];
} QuadArgs;

static Error _dxfInitializeTask(Pointer);
static Error _dxfInitialize(QuadsII2DInterpolator);
static int   _dxfCleanup(QuadsII2DInterpolator);

static int   quad_coords(QuadArgs *);
static int   quadExit(QuadArgs *, int, int);
static int   QuadsWalk(QuadArgs *, int);

#define INSIDE           0
#define POS_OUT          1
#define NEG_OUT         -1
#define POS_FUZZ         2
#define NEG_FUZZ        -2
#define ZERO_AREA        3

#define WithinFuzz(s)   ((s) == POS_FUZZ || (s) == NEG_FUZZ)
#define NegSide(s)      ((s) == NEG_FUZZ || (s) == NEG_OUT)
#define PosSide(s)      ((s) == POS_FUZZ || (s) == POS_OUT)

#define DIAGNOSTIC(str) \
	DXMessage("quads interpolator failure: %s", (str))

int
_dxfRecognizeQuadsII2D(Field field)
{
    Array    array;
    Type t;
    Category c;
    int r, s[32];

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "quads");

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
	return 0;
    
    DXGetArrayInfo(array, NULL, &t, &c, &r, s);

    if (t != TYPE_FLOAT || c != CATEGORY_REAL || r != 1)
    {
	DXSetError(ERROR_DATA_INVALID, "#11450");
	return 0;
    }

    if (s[0] != 2)
    {
	DXSetError(ERROR_DATA_INVALID, "#11363", "quads", 2);
	return 0;
    }

    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
	return 0;
    
    DXGetArrayInfo(array, NULL, &t, &c, &r, s);

    if (t != TYPE_INT || c != CATEGORY_REAL || r != 1)
    {
	DXSetError(ERROR_DATA_INVALID, "#11450");
	return 0;
    }

    if (s[0] != 4)
    {
	DXSetError(ERROR_DATA_INVALID, "#11004", "quads", 2);
	return 0;
    }
	
    return 1;
}

QuadsII2DInterpolator
_dxfNewQuadsII2DInterpolator(Field field, 
			enum interp_init initType, double fuzz, Matrix *m) 
{
    return (QuadsII2DInterpolator)_dxf_NewQuadsII2DInterpolator(field, 
			initType, fuzz, m, &_dxdquadsii2dinterpolator_class);
}

QuadsII2DInterpolator
_dxf_NewQuadsII2DInterpolator(Field field,
			enum interp_init initType, float fuzz, Matrix *m,
		    struct quadsii2dinterpolator_class *class)
{
    QuadsII2DInterpolator 	qi;
    float	*mm, *MM;

    qi = (QuadsII2DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
	    (struct fieldinterpolator_class *)class);

    if (! qi)
	return NULL;

    mm = ((Interpolator)qi)->min;
    MM = ((Interpolator)qi)->max;

    if (((MM[0] - mm[0]) * (MM[1] - mm[1])) == 0.0)
    {
	DXDelete((Object)qi);
	return NULL;
    }

    qi->pArray	  = NULL;
    qi->pHandle   = NULL;
    qi->dArray	  = NULL;
    qi->dHandle   = NULL;
    qi->qArray	  = NULL;
    qi->qHandle   = NULL;
    qi->nArray	  = NULL;
    qi->neighbors = NULL;
    qi->visited	  = NULL;
    qi->grid	  = NULL;

    qi->vCount = 1;

    qi->hint = -1;

    if (initType == INTERP_INIT_PARALLEL)
    {
	if (! DXAddTask(_dxfInitializeTask, (Pointer)&qi, sizeof(qi), 1.0))
	{
	    DXDelete((Object)qi);
	    return NULL;
	}

    }
    else if (initType == INTERP_INIT_IMMEDIATE)
    {
	if (! _dxfInitialize(qi))
	{
	    DXDelete((Object)qi);
	    return NULL;
	}
    }

    return qi;
}

static Error
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(QuadsII2DInterpolator *)p);
}

static Error
_dxfInitialize(QuadsII2DInterpolator qi)
{
    Field	field;
    Type	dataType;
    Category	dataCategory;
    int		i;
    float	len, area;

    qi->fieldInterpolator.initialized = 1;

    field = (Field)((Interpolator)qi)->dataObject;

    /*
     * De-reference data
     */
    qi->dArray   = (Array)DXGetComponentValue(field, "data");
    if (!qi->dArray) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return ERROR;
    }
    DXReference((Object)qi->dArray);

    DXGetArrayInfo(qi->dArray, NULL, &dataType, &dataCategory, NULL, NULL);

    qi->dHandle = DXCreateArrayHandle(qi->dArray);
    if (! qi->dHandle)
	return ERROR;

    DXGetArrayInfo(qi->dArray, NULL, &((Interpolator)qi)->type,
		&((Interpolator)qi)->category,
		&((Interpolator)qi)->rank, ((Interpolator)qi)->shape);

    /*
     * Get the grid. 
     */
    qi->pArray = (Array)DXGetComponentValue(field, "positions");
    if (!qi->pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return ERROR;
    }
    DXReference((Object)qi->pArray);

    qi->pHandle = DXCreateArrayHandle(qi->pArray);
    if (! qi->pHandle)
	return ERROR;

    /*
     * Get the quads. 
     */
    qi->qArray = (Array)DXGetComponentValue(field, "connections");
    if (!qi->qArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return ERROR;
    }
    DXReference((Object)qi->qArray);

    DXGetArrayInfo(qi->qArray, &qi->nQuads, NULL, NULL, NULL, NULL);

    qi->qHandle = DXCreateArrayHandle(qi->qArray);
    if (! qi->qHandle)
	return ERROR;

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    qi->nElements = DXGetItemSize(qi->dArray) 
			/ DXTypeSize(((Interpolator)qi)->type);

    /*
     * Get the neighbors
     */
    if (DXQueryGridConnections(qi->qArray, NULL, qi->gridCounts))
    {
	qi->neighbors = NULL;
	qi->nArray = NULL;
    }
    else
    {	
	qi->nArray = DXNeighbors(((Field)((Interpolator)qi)->dataObject));
	if (! qi->nArray)
	    return ERROR;
	
	DXReference((Object)qi->nArray);

	if (qi->fieldInterpolator.localized)
	    qi->neighbors = (int *)DXGetArrayDataLocal(qi->nArray);
	else
	    qi->neighbors = (int *)DXGetArrayData(qi->nArray);

	if (! qi->neighbors)
	    return ERROR;
    }

    if (qi->nQuads)
    {
	area = (((Interpolator)qi)->max[0] - ((Interpolator)qi)->min[0]) *
		    (((Interpolator)qi)->max[1] - ((Interpolator)qi)->min[1]);
    
	area /= qi->nQuads;

	len = sqrt(area);

	qi->fieldInterpolator.fuzz *= len;
    }

    qi->grid = _dxfMakeSearchGrid(((Interpolator)qi)->max,
				((Interpolator)qi)->min, qi->nQuads, 2);
    if (! qi->grid)
	return ERROR;

    
    for (i = 0; i < qi->nQuads; i++)
    {
	int qbuf[4], *q;
	float pbuf[8];
	float *points[4];
	register int j;

	q = (int *)DXGetArrayEntry(qi->qHandle, i, (Pointer)qbuf);

	for (j = 0; j < 4; j++)
	    points[j] = (float *)DXGetArrayEntry(qi->pHandle,
						*q++, (Pointer)(pbuf+(j<<1)));


	if (! _dxfAddItemToSearchGrid(qi->grid, points, 4, i))
	    return ERROR;
    }
	
    qi->gridFlag = 1;

    qi->visited = (int *)DXAllocateZero(qi->nQuads*sizeof(int));
    if (! qi->visited)
	return ERROR;

    return OK;
}

Error
_dxfQuadsII2DInterpolator_Delete(QuadsII2DInterpolator qi)
{
    _dxfCleanup(qi);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) qi);
}

int
_dxfQuadsII2DInterpolator_PrimitiveInterpolate(QuadsII2DInterpolator qi,
		    int *n, float **points, Pointer *values, int fuzzFlag)
{
    int primNum = -1;
    QuadCoord quadCoord;
    int i;
    int found;
    Pointer v;
    float *p;
    QuadArgs args;
    int	dep;
    char *dbuf = NULL;
    int  itemSize;
    InvalidComponentHandle icH = ((FieldInterpolator)qi)->invCon;
    Matrix *xform;

    if (! qi->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(qi))
	{
	    _dxfCleanup(qi);
	    return ERROR;
	}

	qi->fieldInterpolator.initialized = 1;
    }

    dep = qi->fieldInterpolator.data_dependency;
    v = (Pointer)*values;
    p = (float *)*points;

    args.fuzz      = qi->fieldInterpolator.fuzz;
    args.quadCoord = &quadCoord;
    args.pHandle   = qi->pHandle;
    args.neighbors = qi->neighbors;
    args.qHandle   = qi->qHandle;
    args.visited   = qi->visited;
    args.limit	   = 10;

    args.gridCounts[0] = qi->gridCounts[0] - 1;
    args.gridCounts[1] = qi->gridCounts[1] - 1;

    itemSize = DXGetItemSize(qi->dArray);
    dbuf = DXAllocate(4*itemSize);
    if (! dbuf)
	return ERROR;
    
    if (((FieldInterpolator)qi)->xflag)
	xform = &(((FieldInterpolator)qi)->xform);
    else
	xform = NULL;


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

	args.vCount   = qi->vCount++;
	args.fuzzFlag = fuzzFlag;
	args.point    = pPtr;

	/*
	 * Check the hint first, if one exists...
	 */
	if (qi->hint != -1)
	    found = QuadsWalk(&args, qi->hint);

	/*
	 * If it wasn't found,  try the search grid
	 */
	if (found == -1)
	{
	    if (qi->gridFlag)
	    {
		_dxfInitGetSearchGrid(qi->grid, (float *)pPtr);
		while((primNum = _dxfGetNextSearchGrid(qi->grid)) != -1
							&& found == -1)
		    if (args.visited[primNum] != qi->vCount)
			found = QuadsWalk(&args, primNum);
	    }
	    else
	    {
		for (i = 0; i < qi->nQuads && found == -1; i++)
		    if (args.visited[primNum] != qi->vCount)
			found = QuadsWalk(&args, primNum);
	    }
	}

	if (found == -1 || (icH && DXIsElementInvalid(icH, found)))
	    break;

	qi->hint = found;

#define INTERPOLATE(type, round)				\
{								\
    type *d0, *d1, *d2, *d3, *r;				\
								\
    r = (type *)v;						\
								\
    d0 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		    args.quad[0], (Pointer)dbuf);		\
								\
    d1 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		    args.quad[1], (Pointer)(dbuf+itemSize));	\
								\
    d2 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		    args.quad[2], (Pointer)(dbuf+2*itemSize));	\
								\
    d3 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		    args.quad[3], (Pointer)(dbuf+3*itemSize));	\
								\
    for (i = 0; i < qi->nElements; i++)				\
	*r++ = (*d0++ * quadCoord.p) + 				\
		    (*d1++ * quadCoord.q) + 			\
			    (*d2++ * quadCoord.r) +		\
				(*d3++ * quadCoord.s) +		\
				    round;			\
    								\
    v = (Pointer)r;						\
}

	if (((FieldInterpolator)qi)->cstData)
	{
	    memcpy(v, ((FieldInterpolator)qi)->cstData, itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
	else if (dep == DATA_POSITIONS_DEPENDENT)
	{
            Type dataType;

            if ((dataType = ((Interpolator)qi)->type) == TYPE_FLOAT)
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
	    memcpy(v, DXGetArrayEntry(qi->dHandle,
					found, (Pointer)dbuf), itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}


	/*
	 * Only use fuzz on first point
	 */
	fuzzFlag = FUZZ_OFF;

	p += 2;
	*n -= 1;
    }

    if (! dbuf)
	DXFree((Pointer)dbuf);

    *values = (Pointer)v;
    *points = (float *)p;

    return OK;
}

static int
_dxfCleanup(QuadsII2DInterpolator qi)
{								
    if (qi->fieldInterpolator.localized)
    {
	if (qi->neighbors)
	    DXFreeArrayDataLocal(qi->nArray, (Pointer)qi->neighbors);
    }

    if (qi->pHandle)
    {
	DXFreeArrayHandle(qi->pHandle);
	qi->pHandle = NULL;
    }

    if (qi->qHandle)
    {
	DXFreeArrayHandle(qi->qHandle);
	qi->qHandle = NULL;
    }

    if (qi->dHandle)
    {
	DXFreeArrayHandle(qi->dHandle);
	qi->dHandle = NULL;
    }

    qi->dHandle   = NULL;
    qi->neighbors = NULL;
    qi->pHandle   = NULL;
    qi->qHandle   = NULL;


    if (qi->visited)
    {
	DXFree((Pointer)qi->visited);
	qi->visited = NULL;
    }

    if (qi->dArray)
    {
	DXDelete((Object)qi->dArray);
	qi->dArray = NULL;
    }

    if (qi->pArray)
    {
	DXDelete((Object)qi->pArray);
	qi->pArray = NULL;
    }

    if (qi->nArray)
    {
	DXDelete((Object)qi->nArray);
	qi->nArray = NULL;
    }

    if (qi->qArray)
    {
	DXDelete((Object)qi->qArray);
	qi->qArray = NULL;
    }

    if (qi->grid)
    {
	_dxfFreeSearchGrid(qi->grid);
	qi->grid = NULL;
    }
    return OK;
}

static int quad_coords (QuadArgs *args)
{
    double 	a, a01, a12, a20;
    double 	b, b13, b32, b21;
    double	x, x0, x1, x2, x3;
    double	y, y0, y1, y2, y3;
    QuadCoord   *q;
    float       fuzz, fa, fb;

    x  = args->point[0];    	    y  = args->point[1];
    x0 = args->quadPoints[0][0];    y0 = args->quadPoints[0][1];
    x1 = args->quadPoints[1][0];    y1 = args->quadPoints[1][1];
    x2 = args->quadPoints[2][0];    y2 = args->quadPoints[2][1];
    x3 = args->quadPoints[3][0];    y3 = args->quadPoints[3][1];

    fuzz = args->fuzz;
    q    = args->quadCoord;

    a  = a01 = (x *y0 + x0*y1 + x1*y  - y *x0 - y0*x1 - y1*x );
    a += a12 = (x *y1 + x1*y2 + x2*y  - y *x1 - y1*x2 - y2*x );
    a += a20 = (x *y2 + x2*y0 + x0*y  - y *x2 - y2*x0 - y0*x );

    if (a == 0.0)
	return ZERO_AREA;
    
    if ((a01 >= 0.0 && a12 >= 0.0 && a20 >= 0.0) ||
        (a01 <= 0.0 && a12 <= 0.0 && a20 <= 0.0) ||
        (a01 == 0.0 && ((a12 <= 0.0 && a20 <= 0.0) ||
                        (a12 <= 0.0 && a20 <= 0.0)    )) ||
        (a12 == 0.0 && ((a01 <= 0.0 && a20 <= 0.0) ||
                        (a01 <= 0.0 && a20 <= 0.0)    )) ||
        (a20 == 0.0 && ((a12 <= 0.0 && a01 <= 0.0) ||
                        (a12 <= 0.0 && a01 <= 0.0)    ))
       )
    {
	a = 1.0 / a;
	q->p = a12 * a;
	q->q = a20 * a;
	q->r = a01 * a;
	q->s = 0.0;

	return INSIDE;
    }
    else
    {
	fa = fuzz * a;
	if (fa < 0.0)
	    fa = -fa;

	if ((a01 >= -fa && a12 >= -fa && a20 >= -fa) ||
	     (a01 <= fa && a12 <= fa && a20 <= fa))
	{
	    a = 1.0 / a;
	    q->p = a12 * a;
	    q->q = a20 * a;
	    q->r = a01 * a;
	    q->s = 0.0;

	    if (a > 0)
		return POS_FUZZ;
	    else
		return NEG_FUZZ;
	}
    }

    b  = b13 = (x *y1 + x1*y3 + x3*y  - y *x1 - y1*x3 - y3*x );
    b += b32 = (x *y3 + x3*y2 + x2*y  - y *x3 - y3*x2 - y2*x );
    b += b21 = (x *y2 + x2*y1 + x1*y  - y *x2 - y2*x1 - y1*x );
	
	if(b == 0.0)
	return ZERO_AREA;

    if ((b13 >= 0.0 && b32 >= 0.0 && b21 >= 0.0) ||
	(b13 <= 0.0 && b32 <= 0.0 && b21 <= 0.0) ||
        (b13 == 0.0 && ((b32 <= 0.0 && b21 <= 0.0) ||
                        (b32 <= 0.0 && b21 <= 0.0)    )) ||
        (b32 == 0.0 && ((b13 <= 0.0 && b21 <= 0.0) ||
                        (b13 <= 0.0 && b21 <= 0.0)    )) ||
        (b21 == 0.0 && ((b32 <= 0.0 && b13 <= 0.0) ||
                        (b32 <= 0.0 && b13 <= 0.0)    ))
       )
    {
	b = 1.0 / b;
	q->p = 0.0;
	q->q = b32 * b;
	q->r = b13 * b;
	q->s = b21 * b;

	return INSIDE;
    }
    else
    {
	fb = fuzz*b;
	if (fb < 0.0)
	    fb = -fb;

	if ((b13 >= -fb && b32 >= -fb && b21 >= -fb) ||
	     (b13 <= fb && b32 <= fb && b21 <= fb))
	{
	    b = 1.0 / b;
	    q->p = 0.0;
	    q->q = b32 * b;
	    q->r = b13 * b;
	    q->s = b21 * b;

	    if (b > 0)
		return POS_FUZZ;
	    else
		return NEG_FUZZ;
	}
    }

    q->p = a01;
    q->q = b32;
    q->r = a20;
    q->s = b13;
    
    if (a + b > 0)
	return POS_OUT;
    else
	return NEG_OUT;
}

static int
quadExit(QuadArgs *args, int side, int index)
{
    QuadCoord      *be;
    int		   *nbrs;
    int		   nBuf[4];
    float          best;		/* best choice so far */
    int            f;

    f = -1;

    be   = args->quadCoord;

    if (args->neighbors)
	nbrs = args->neighbors + (index<<2);
    else
    {
	int x, y;
	int xSkip = args->gridCounts[1];

	nbrs = nBuf;

	y = index % xSkip;
	x = index / xSkip;

	if (x == 0) 			nbrs[0] = -1;
	else        			nbrs[0] = index - xSkip;
	if (x >= args->gridCounts[0]-1) nbrs[1] = -1;
	else 				nbrs[1] = index + xSkip;
	if (y == 0) 			nbrs[2] = -1;
	else        			nbrs[2] = index - 1;
	if (y >= args->gridCounts[1]-1) nbrs[3] = -1;
	else 				nbrs[3] = index + 1;
    }

    if (NegSide(side))
    {
	best = DXD_MAX_FLOAT;

	if (be->p > 0.0 && be->p < best && nbrs[0] >= 0)
	{
	    best = be->p;
	    f    = nbrs[0];
	}
	if (be->q > 0.0 && be->q < best && nbrs[1] >= 0)
	{
	    best = be->q;
	    f    = nbrs[1];
	}
	if (be->r > 0.0 && be->r < best && nbrs[2] >= 0)
	{
	    best = be->r;
	    f    = nbrs[2];
	}
	if (be->s > 0.0 && be->s < best && nbrs[3] >= 0)
	{
	    best = be->s;
	    f    = nbrs[3];
	}
    }
    else
    {
	best = -DXD_MAX_FLOAT;

	if (be->p < 0.0 && be->p > best && nbrs[0] >= 0)
	{
	    best = be->p;
	    f    = nbrs[0];
	}
	if (be->q < 0.0 && be->q > best && nbrs[1] >= 0)
	{
	    best = be->q;
	    f    = nbrs[1];
	}
	if (be->r < 0.0 && be->r > best && nbrs[2] >= 0)
	{
	    best = be->r;
	    f    = nbrs[2];
	}
	if (be->s < 0.0 && be->s > best && nbrs[3] >= 0)
	{
	    best = be->s;
	    f    = nbrs[3];
	}
    }

    return f;
}

static int
QuadsWalk(QuadArgs *args, int quadIndex)
{
    int  	  side;
    int           *quad;
    int		  knt;
    int		  nbr;
    ArrayHandle	  pHandle;
    ArrayHandle   qHandle;
    int		  *visited, vCount;
    int		  limit;
    int		  fuzzFlag;
    int		  *vPtr;

    pHandle  = args->pHandle;
    qHandle  = args->qHandle;
    visited  = args->visited;
    vCount   = args->vCount;
    limit    = args->limit;
    fuzzFlag = args->fuzzFlag;

    for (knt = 0; quadIndex != -1 && knt < limit; knt++, quadIndex = nbr)
    {
	float pbuf[2], qbuf[2], rbuf[2], sbuf[2];

	if (visited)
	{ 
	    vPtr = visited + quadIndex;

	    if (*vPtr == vCount)
		break;
	    else
		*vPtr = vCount;
	}

	quad = (int *)DXGetArrayEntry(qHandle, quadIndex,
						    (Pointer)(args->quad));

	args->quadPoints[0]  =
	    (float *)DXGetArrayEntry(pHandle, quad[0], (Pointer)pbuf);

	args->quadPoints[1]  =
	    (float *)DXGetArrayEntry(pHandle, quad[1], (Pointer)qbuf);

	args->quadPoints[2]  =
	    (float *)DXGetArrayEntry(pHandle, quad[2], (Pointer)rbuf);

	args->quadPoints[3]  =
	    (float *)DXGetArrayEntry(pHandle, quad[3], (Pointer)sbuf);

	side = quad_coords(args);

	if (side == ZERO_AREA)
	    break;

	if (side == INSIDE || (fuzzFlag == FUZZ_ON && WithinFuzz(side)))
	{
	    if (quad != args->quad)
		memcpy(args->quad, quad, 4*sizeof(int));

	    return quadIndex;
	}
	
	nbr = quadExit(args, side, quadIndex);
	if (nbr == -1)
	    return -1;
    }

    return -1;
}

Object
_dxfQuadsII2DInterpolator_Copy(QuadsII2DInterpolator old, enum _dxd_copy copy)
{
    QuadsII2DInterpolator new;

    new = (QuadsII2DInterpolator)
	    _dxf_NewObject((struct object_class *)&_dxdquadsii2dinterpolator_class);

    if (!(_dxf_CopyQuadsII2DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

QuadsII2DInterpolator
_dxf_CopyQuadsII2DInterpolator(QuadsII2DInterpolator new, 
				QuadsII2DInterpolator old, enum _dxd_copy copy)
{

    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nPoints       = old->nPoints;
    new->nQuads        = old->nQuads;
    new->nElements     = old->nElements;
    new->hint          = old->hint;
    new->gridCounts[0] = old->gridCounts[0];
    new->gridCounts[1] = old->gridCounts[1];

    if (! new->fieldInterpolator.initialized)
    {
	new->pArray    = NULL;
	new->qArray    = NULL;
	new->dArray    = NULL;
	new->nArray    = NULL;
	new->pHandle   = NULL;
	new->qHandle   = NULL;
	new->dHandle   = NULL;
	new->neighbors = NULL;
	new->visited   = NULL;
    }
    else
    {
	new->pArray = (Array)DXReference((Object)old->pArray);
	new->qArray = (Array)DXReference((Object)old->qArray);
	new->dArray = (Array)DXReference((Object)old->dArray);
	new->nArray = (Array)DXReference((Object)old->nArray);

	new->pHandle = DXCreateArrayHandle(new->pArray);
	new->qHandle = DXCreateArrayHandle(new->qArray);
	new->dHandle = DXCreateArrayHandle(new->dArray);
	if (! new->pHandle || ! new->qHandle || ! new->dHandle)
	    return NULL;

	if (new->fieldInterpolator.localized)
	{
	    if (new->nArray)
		new->neighbors= (int *)DXGetArrayDataLocal(new->nArray);
	}
	else
	{
	    if (new->nArray)
		new->neighbors= (int *)DXGetArrayData(new->nArray);
	}

	if (old->gridFlag)
	{
	    new->gridFlag = 1;
	    new->grid = _dxfCopySearchGrid(old->grid);
	    new->visited = NULL;
	}
	else
	{
	    new->gridFlag = 0;
	}
    }


    new->vCount = 1;

    new->visited = (int *)DXAllocateZero(old->nQuads*sizeof(int));
    if (! new->visited)
	return NULL;

    if (DXGetError())
	return NULL;

    return new;
}

Interpolator
_dxfQuadsII2DInterpolator_LocalizeInterpolator(QuadsII2DInterpolator qi)
{
    if (qi->fieldInterpolator.localized)
	return (Interpolator)qi;

    qi->fieldInterpolator.localized = 1;

    if (qi->fieldInterpolator.initialized)
    {
	if (qi->nArray)
	    qi->neighbors = (int *)DXGetArrayDataLocal(qi->nArray);
	
	qi->pHandle = DXCreateArrayHandle(qi->pArray);
	if (! qi->pHandle)
	    return NULL;
	
	qi->qHandle = DXCreateArrayHandle(qi->qArray);
	if (! qi->qHandle)
	    return NULL;
	
	qi->dHandle = DXCreateArrayHandle(qi->dArray);
	if (! qi->dHandle)
	    return NULL;
    }

    if (DXGetError())
        return NULL;
    else
        return (Interpolator)qi;
}
