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
#include "tetrasClass.h"


#define SEARCH_LIMIT	4

#undef DIAG	

#define FALSE 0
#define TRUE 1

typedef struct
{
    ArrayHandle	       points;
    TetNeighbors       *neighbors;
    Tetrahedron	       *tetras;
    int		       *visited;
    int		       vCount;
    Point              *point;
    Point              *p0, *p1, *p2, *p3;
    BaryCoord          *bary;
    float              fuzz;
    int		       fuzzFlag;
    int		       limit;
    Point	       *mm;
} TetraArgs;

static int	_dxfbarycentric_coords(TetraArgs *);
static void	_dxfCleanup(TetrasInterpolator);
static int	_dxfInitializeTask(Pointer);
static int	_dxfInitialize(TetrasInterpolator);
static int	_dxfbaryExit(int, int, TetraArgs *);
static int	_dxfTetraSearch(TetraArgs *, int);
static int	_dxfBBoxTest(TetraArgs *, int);

#define DIAGNOSTIC(str) \
	DXMessage("tetrahedra interpolator failure: %s", (str))
    
/*
 * The following are the returned by _dxfbarycentric_coords indicating
 * whether the sample point lay inside the tetrahedra, within FUZZ of
 * the tetrahedra, well outside the tetrahedra, or that the tetrahedra
 * was of zero volume.
 */

#define	INSIDE		 0
#define POS_OUT		 1
#define NEG_OUT		-1
#define POS_FUZZ	 2
#define NEG_FUZZ	-2
#define ZERO_VOLUME	 3

#define WithinFuzz(s)	((s) == POS_FUZZ || (s) == NEG_FUZZ)
#define NegSide(s)	((s) == NEG_FUZZ || (s) == NEG_OUT)
#define PosSide(s)	((s) == POS_FUZZ || (s) == POS_OUT)

int
_dxfRecognizeTetras(Field field)
{
    Array	array;
    Type	type;
    Category	cat;
    int		rank, shape[32];
    int		status;

    CHECK(field, CLASS_FIELD);

    status = OK;

    ELT_TYPECHECK(field, "tetrahedra");
    
    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
    {
	DIAGNOSTIC("missing positions component");
	status = ERROR;
    }
    else
    {
	if (! DXGetArrayInfo(array, NULL, NULL, &cat, &rank, shape))
	{
	    DIAGNOSTIC("error accessing positions info");
	    status = ERROR;
	}

	if (cat != CATEGORY_REAL)
	{
	    DIAGNOSTIC("only CATEGORY_REAL positions supported");
	    status = ERROR;
	}
    }

    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
    {
	DIAGNOSTIC("missing connections component");
	status = ERROR;
    }
    else
    {
	if (! DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape))
	{
	    DIAGNOSTIC("error accessing connections info");
	    status = ERROR;
	}

	if (type != TYPE_INT)
	{
	    DIAGNOSTIC("only TYPE_INT connections supported");
	    status = ERROR;
	}

	if (cat != CATEGORY_REAL)
	{
	    DIAGNOSTIC("only CATEGORY_REAL connections supported");
	    status = ERROR;
	}
    }

    return status;
}

TetrasInterpolator
_dxfNewTetrasInterpolator(Field field,
		enum interp_init initType, double fuzz, Matrix *m)
{
    return (TetrasInterpolator)_dxf_NewTetrasInterpolator(field,
			initType, fuzz, m, &_dxdtetrasinterpolator_class);
}

TetrasInterpolator
_dxf_NewTetrasInterpolator(Field field,
		enum interp_init initType, float fuzz, Matrix *m,
				    struct tetrasinterpolator_class *class)
{
    TetrasInterpolator 	ti;
    float	*mm, *MM;

    ti = (TetrasInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
			    (struct fieldinterpolator_class *)class);

    if (! ti)
	return NULL;

    mm = ((Interpolator)ti)->min;
    MM = ((Interpolator)ti)->max;

    if (((MM[0] - mm[0]) * (MM[1] - mm[1]) * (MM[2] - mm[2])) == 0.0)
    {
	DXDelete((Object)ti);
	return NULL;
    }

    ti->points      	= NULL;
    ti->pointsArray   	= NULL;
    ti->data		= NULL;
    ti->dataArray     	= NULL;
    ti->mmArray     	= NULL;
    ti->tetras 		= NULL;
    ti->tetrasArray   	= NULL;
    ti->neighbors	= NULL;
    ti->neighborsArray 	= NULL;
    ti->visited 	= NULL;

    ti->vCount = 1;

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

Object
_dxfTetrasInterpolator_Copy(TetrasInterpolator old, enum _dxd_copy copy)
{
    TetrasInterpolator new;

    new = (TetrasInterpolator)
	_dxf_NewObject((struct object_class *)&_dxdtetrasinterpolator_class);

    if (!(_dxf_CopyTetrasInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

TetrasInterpolator
_dxf_CopyTetrasInterpolator(TetrasInterpolator new, 
				TetrasInterpolator old, enum _dxd_copy copy)
{
    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new, 
						(FieldInterpolator)old, copy))
	return NULL;
    
    if (! old->fieldInterpolator.initialized)
    {
	new->points         = NULL;
	new->tetras         = NULL;
	new->data           = NULL;
	new->neighbors 	    = NULL;
	new->pointsArray    = NULL;
	new->neighborsArray = NULL;
	new->tetrasArray    = NULL;
	new->dataArray      = NULL;
	new->mmArray        = NULL;
	new->nPoints   	    = old->nPoints;
	new->nTetras  	    = old->nTetras;
	new->nElements	    = old->nElements;
	new->visited	    = NULL;
    }
    else
    {
	new->pointsArray    = (Array)DXReference((Object)old->pointsArray);
	new->neighborsArray = (Array)DXReference((Object)old->neighborsArray);
	new->tetrasArray    = (Array)DXReference((Object)old->tetrasArray);
	new->dataArray      = (Array)DXReference((Object)old->dataArray);
	new->mmArray        = (Array)DXReference((Object)old->mmArray);

	new->nPoints   = old->nPoints; 
	new->nTetras   = old->nTetras;
	new->nElements = old->nElements;

	new->hint = -1;

	new->points = DXCreateArrayHandle(new->pointsArray);
	if (! new->points)
	    return NULL;

	new->data = DXCreateArrayHandle(new->dataArray);
	if (! new->data)
	    return NULL;

	if (new->fieldInterpolator.localized)
	{
	    new->tetras    =
		    (Tetrahedron *)DXGetArrayDataLocal(new->tetrasArray);
	    new->neighbors =
		    (TetNeighbors *)DXGetArrayDataLocal(new->neighborsArray);
	    new->mm        =(Point *)DXGetArrayDataLocal(new->mmArray);
	}
	else
	{
	    new->tetras    =(Tetrahedron *)DXGetArrayData(new->tetrasArray);
	    new->neighbors =
			(TetNeighbors *)DXGetArrayData(new->neighborsArray);
	    new->mm        =(Point *)DXGetArrayData(new->mmArray);
	}

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

    new->vCount	     = 1;
    new->searchLimit = old->searchLimit;

    new->visited = (int *)DXAllocate(new->nTetras*sizeof(int));
    if (! new->visited)
	return NULL;

    memset(new->visited, 0, new->nTetras*sizeof(int));

    if (DXGetError())
	return NULL;

    return (TetrasInterpolator)new;
}

static int
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(TetrasInterpolator *)p);
}

static int
_dxfInitialize(TetrasInterpolator ti)
{
    int			i, j;
    Field               field;
    float		len, vol;
    int			*t;
    Point 		*mm;
    float		fuzz = 0;

    ti->hint = -1;

    field = (Field)((Interpolator)ti)->dataObject;

    /*
     * De-reference data
     */
    ti->dataArray   = (Array)DXGetComponentValue(field, "data");
    if (!ti->dataArray) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return 0;
    }
    DXReference((Object)ti->dataArray);

    ti->data = DXCreateArrayHandle(ti->dataArray);
    if (! ti->data)
	return 0;

    DXGetArrayInfo(ti->dataArray, NULL, &((Interpolator)ti)->type,
		&((Interpolator)ti)->category,
		&((Interpolator)ti)->rank, ((Interpolator)ti)->shape);

    /*
     * Get the grid. 
     */
    ti->pointsArray = (Array)DXGetComponentValue(field, "positions");
    if (!ti->pointsArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return 0;
    }
    DXReference((Object)ti->pointsArray);

    ti->points = DXCreateArrayHandle(ti->pointsArray);
    if (! ti->points)
	return ERROR;

    /*
     * Get the tetrahedra. 
     */
    ti->tetrasArray = (Array)DXGetComponentValue(field, "connections");
    if (!ti->tetrasArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return 0;
    }
    DXReference((Object)ti->tetrasArray);

    DXGetArrayInfo(ti->tetrasArray, &ti->nTetras, NULL, NULL, NULL, NULL);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    ti->nElements = DXGetItemSize(ti->dataArray) / 
			DXTypeSize(((Interpolator)ti)->type);

    /*
     * Get the neighbors array
     */
    ti->neighborsArray = DXNeighbors((Field)((Interpolator)ti)->dataObject);
    if (!ti->neighborsArray)
	return 0;

    DXReference((Object)ti->neighborsArray);

    if (ti->fieldInterpolator.localized)
	ti->neighbors = (TetNeighbors *)DXGetArrayDataLocal(ti->neighborsArray);
    else
	ti->neighbors = (TetNeighbors *)DXGetArrayData(ti->neighborsArray);

    if (ti->fieldInterpolator.localized)
	ti->tetras = (Tetrahedron *)DXGetArrayDataLocal(ti->tetrasArray);
    else
	ti->tetras = (Tetrahedron *)DXGetArrayData(ti->tetrasArray);

    ti->mmArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! ti->mmArray)
	return 0;
    
    DXReference((Object)ti->mmArray);
    
    if (! DXAddArrayData(ti->mmArray, 0, 2*ti->nTetras, NULL))
	return 0;
    
    ti->mm = (Point *)DXGetArrayData(ti->mmArray);

    ti->grid = _dxfMakeSearchGrid(((Interpolator)ti)->max, 
			((Interpolator)ti)->min, ti->nTetras, 3);
    if (! ti->grid)
	return 0;

    if (ti->nTetras)
    {
	vol = 1.0;
	for (i = 0; i < 3; i++)
	    vol *= ((Interpolator)ti)->max[i] - ((Interpolator)ti)->min[i];
	
	vol /= ti->nTetras;

	len = pow((double)vol, 0.3333333);
	
	fuzz = ti->fieldInterpolator.fuzz * len;
    }
    
    t  = (int *)ti->tetras;

    mm = ti->mm;
    for (i = 0; i < ti->nTetras; i++, t += 4)
    {
	float *v;
	register float x,X,y,Y,z,Z;
	float *pt[4];
	Point pBuf[4];
	
	pt[0] = v = (float *)DXGetArrayEntry(ti->points, t[0], 
							    (Pointer)(pBuf));

	x = X = *v++;
	y = Y = *v++;
	z = Z = *v++;

	for (j = 1; j < 4; j++)
	{
	    register float tt;

	    pt[j] = v = (float *)DXGetArrayEntry(ti->points, t[j], 
							(Pointer)(pBuf + j));

	    tt = *v++;
	    if (x > tt) x = tt;
	    if (X < tt) X = tt;

	    tt = *v++;
	    if (y > tt) y = tt;
	    if (Y < tt) Y = tt;

	    tt = *v;
	    if (z > tt) z = tt;
	    if (Z < tt) Z = tt;
	}

	mm->x = x - fuzz;
	mm->y = y - fuzz;
	mm->z = z - fuzz;
	mm++;

	mm->x = X + fuzz;
	mm->y = Y + fuzz;
	mm->z = Z + fuzz;
	mm++;

	if (! _dxfAddItemToSearchGrid(ti->grid, pt, 4, i))
	    break;
    }

    if (i != ti->nTetras)
	return 0;

    ti->gridFlag = 1;

    ti->visited = (int *)DXAllocate(ti->nTetras*sizeof(int));
    if (! ti->visited)
	return ERROR;

    memset(ti->visited, 0, ti->nTetras*sizeof(int));

    ti->fieldInterpolator.initialized = 1;

    return OK;
}

Error
_dxfTetrasInterpolator_Delete(TetrasInterpolator ti)
{
    _dxfCleanup(ti);
    _dxfFieldInterpolator_Delete((FieldInterpolator) ti);

    return OK;
}

#ifdef DIAG
static int nT;
static int nB;
#endif

static int
_dxfBBoxTest(TetraArgs *args, int i)
{
    float *mm, *p;

    mm = (float *)(args->mm + 2*i);
    p  = (float *) args->point;

    if (args->fuzzFlag)
    {
	float f;

	f = args->fuzz * (mm[3] - mm[0]);
	if (*p < mm[0]-f || *p > mm[3]+f) goto out;
	p++; mm++;

	f = args->fuzz * (mm[3] - mm[0]);
	if (*p < mm[0]-f || *p > mm[3]+f) goto out;
	p++; mm++;

	f = args->fuzz * (mm[3] - mm[0]);
	if (*p < mm[0]-f || *p > mm[3]+f) goto out;
    }
    else
    {
	if (*p < mm[0] || *p > mm[3]) goto out;
	p++; mm++;

	if (*p < mm[0] || *p > mm[3]) goto out;
	p++; mm++;

	if (*p < mm[0] || *p > mm[3]) goto out;
    }

    return 1;

out:

#ifdef DIAG
    nB ++;
#endif

    return 0;
}

int
_dxfTetrasInterpolator_PrimitiveInterpolate(TetrasInterpolator ti, int *n,
			    float **points, Pointer *values, int fuzzFlag)
{
    BaryCoord   	   bary;
    int         	   i;
    int         	   found;
    Pointer     	   v;
    Point       	   *p;
    float       	   xMin, xMax;
    float       	   yMin, yMax;
    float       	   zMin, zMax;
    float       	   fuzz;
    TetraArgs   	   args;
    int			   dep;
    int			   itemSize;
    ubyte 		   *dbuf;
    InvalidComponentHandle icH = ((FieldInterpolator)ti)->invCon;
    Matrix		   *xform;

    if (!((FieldInterpolator)ti)->initialized)
    {
	if (! _dxfInitialize(ti))
	{
	    _dxfCleanup(ti);
	    return 0;
	}

	((FieldInterpolator)ti)->initialized = 1;
    }

    xMin = ((Interpolator)ti)->min[0];
    yMin = ((Interpolator)ti)->min[1];
    zMin = ((Interpolator)ti)->min[2];
     
    xMax = ((Interpolator)ti)->max[0];
    yMax = ((Interpolator)ti)->max[1];
    zMax = ((Interpolator)ti)->max[2];
     
    v = (Pointer)*values;
    p = (Point *)*points;

    fuzz = ((FieldInterpolator)ti)->fuzz;

    args.fuzz      = ti->fieldInterpolator.fuzz;
    args.bary      = &bary;
    args.points	   = ti->points;
    args.tetras    = ti->tetras;
    args.neighbors = ti->neighbors;
    args.visited   = ti->visited;
    args.mm	   = ti->mm;

    dep = ti->fieldInterpolator.data_dependency;

    itemSize = ti->nElements*DXTypeSize(((Interpolator)ti)->type);

    dbuf = (ubyte *)DXAllocate(itemSize*4);
    if (! dbuf)
	return 0;
    
    if (((FieldInterpolator)ti)->xflag)
	xform = &(((FieldInterpolator)ti)->xform);
    else
	xform = NULL;

    /*
     * For each point in the input, attempt to interpolate the point.
     * When a point cannot be interpolated, quit.
     */
    while(*n != 0)
    {
	Point xpt;
	Point *pPtr;

	if (xform)
	{
	    xpt.x = p->x*xform->A[0][0] +
		    p->y*xform->A[1][0] +
		    p->z*xform->A[2][0] +
		         xform->b[0];

	    xpt.y = p->x*xform->A[0][1] +
		    p->y*xform->A[1][1] +
		    p->z*xform->A[2][1] +
		         xform->b[1];

	    xpt.z = p->x*xform->A[0][2] +
		    p->y*xform->A[1][2] +
		    p->z*xform->A[2][2] +
		         xform->b[2];
	    
	    pPtr = &xpt;
	}
	else
	    pPtr = p;


	if (fuzzFlag == FUZZ_ON)
	{
	    if ((pPtr->x > xMax+fuzz  ||
		 pPtr->x < xMin-fuzz) ||
		(pPtr->y > yMax+fuzz  ||
		 pPtr->y < yMin-fuzz) ||
		(pPtr->z > zMax+fuzz  ||
		 pPtr->z < zMin-fuzz))
		    break;
	}
	else
	{
	    if ((pPtr->x > xMax  ||
		 pPtr->x < xMin) ||
		(pPtr->y > yMax  ||
		 pPtr->y < yMin) ||
		(pPtr->z > zMax  ||
		 pPtr->z < zMin))
		    break;
	}

#ifdef DIAG
	nT = 0;
	nB = 0;
#endif
	found = -1;

	args.fuzzFlag = fuzzFlag;
	args.point    = pPtr;
	args.vCount   = ti->vCount ++;
	args.limit = SEARCH_LIMIT;

	/*
	 * Check the hint first, if one exists...
	 */
	if (ti->hint != -1 && _dxfBBoxTest(&args, ti->hint))
	{
	    found = _dxfTetraSearch(&args, ti->hint);
	}

	/* 
	 * If it wasn't found, try the search grid. 
	 */
	if (found == -1)
	{
	    if (ti->gridFlag)
	    {
		int primNum;

		_dxfInitGetSearchGrid(ti->grid, (float *)pPtr);
		while((primNum = _dxfGetNextSearchGrid(ti->grid)) != -1
							&& found == -1)
		    if (ti->visited[primNum] != ti->vCount &&
					_dxfBBoxTest(&args, primNum))
			found = _dxfTetraSearch(&args, primNum);
	    }
	}

	if (found == -1 || (icH && DXIsElementInvalid(icH, found)))
	    break;

	ti->hint = found;

#define INTERPOLATE(type, round)				\
    {								\
	type *d0, *d1, *d2, *d3, *r;				\
	Tetrahedron *tet;					\
								\
	tet = ti->tetras + found;				\
								\
	r = (type *)v;						\
								\
	d0 = (type *)DXGetArrayEntry(ti->data, 			\
			tet->p, (Pointer)(dbuf));		\
								\
	d1 = (type *)DXGetArrayEntry(ti->data, 			\
			tet->q, (Pointer)(dbuf+itemSize));	\
								\
	d2 = (type *)DXGetArrayEntry(ti->data, 			\
			tet->r, (Pointer)(dbuf+2*itemSize));	\
								\
	d3 = (type *)DXGetArrayEntry(ti->data, 			\
			tet->s, (Pointer)(dbuf+3*itemSize));	\
								\
	for (i = 0; i < ti->nElements; i++)			\
	    *r++ = (*d0++ * bary.p) + (*d1++ * bary.q) + 	\
		    (*d2++ * bary.r) + (*d3++ * bary.s) +	\
			round;					\
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
	    memcpy(v, DXGetArrayEntry(ti->data,
				    found, (Pointer)dbuf), itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}


	/*
	 * Only use fuzz on first point
	 */
	fuzzFlag = FUZZ_OFF;

	p ++;
	*n -= 1;
    }

    DXFree((Pointer)dbuf);

    *values = (Pointer)v;
    *points = (float *)p;

    return OK;
}

static void
_dxfCleanup(TetrasInterpolator ti)
{								
    /*
     * Note: only free up the array data if its allocated
     * in local memory
     */

    if (((FieldInterpolator)ti)->localized)
    {
	if (ti->tetras)
	{
	    DXFreeArrayDataLocal(ti->tetrasArray, (Pointer)ti->tetras);
	    ti->tetras = NULL;
	}

	if (ti->neighbors)
	{
	    DXFreeArrayDataLocal(ti->neighborsArray, (Pointer)ti->neighbors);
	    ti->neighbors = NULL;
	}
    }

    if (ti->points)
    {
	DXFreeArrayHandle(ti->points);
	ti->points = NULL;
    }

    if (ti->data)
    {
	DXFreeArrayHandle(ti->data);
	ti->data = NULL;
    }

    if (ti->pointsArray)
    {
	DXDelete((Object)ti->pointsArray);
	ti->pointsArray = NULL;
    }

    if (ti->dataArray)
    {
	DXDelete((Object)ti->dataArray);
	ti->dataArray = NULL;
    }

    if (ti->tetrasArray)
    {
	DXDelete((Object)ti->tetrasArray);
	ti->tetrasArray = NULL;
    }

    if (ti->neighborsArray)
    {
	DXDelete((Object)ti->neighborsArray);
	ti->neighborsArray = NULL;
    }
    
    if (ti->gridFlag)
	_dxfFreeSearchGrid(ti->grid);
    
    if (ti->visited)
    {
	DXFree((Pointer)ti->visited);
	ti->visited = NULL;
    }

    if (ti->mmArray)
    {
	DXDelete((Object)ti->mmArray);
	ti->mm = NULL;
    }

}

/*
 * This procedure determines the barycentric coordinates of the point pt with
 * respect to the points p0, p1, p2, and p3.  It does so by finding five 
 * volumes: vol, vol0, vol1, vol2, vol3.  The code to determine these 
 * volumes could be replaced by the following 5 lines of code:
 *   	vol0 = tetra_volume (pt, p1, p2, p3);
 *   	vol1 = tetra_volume (p0, pt, p2, p3);
 *   	vol2 = tetra_volume (p0, p1, pt, p3);
 *   	vol3 = tetra_volume (p0, p1, p2, pt);
 *      vol  = vol0 + vol1 + vol2 + vol3;
 * I.e., vol is the scaled oriented volume of the original tetra, and voli
 * is the scaled oriented volume of the tetra formed by substituting the point
 * pt for the point pi in the ordering p0, p1, p2, p3.  If any of the latter
 * four volumes have orientation opposite to the orientation of the original
 * tetra, then the points pt and pi are on opposite sides of a face of the
 * tetrahedron, and therefore pt is outside of the tetrahedron.  Since the
 * above volumes are calculated with VERY similar determinants, this 
 * procedure takes all the common multiplies and subtracts from all those
 * determinants, and calculates them each only once.  Using 5 separate volume
 * procedure calls, this would take 36 multiplies and 56 add/subtracts.  Using
 * this procedure, this takes 32 multiplies and 39 add/subtracts, which should
 * be a better than 10% savings on one of the most costly procedures (because
 * of the number of times it gets called) in the whole streamline algorithm.
 */

static int _dxfbarycentric_coords (TetraArgs *args)
{
    float 	vol0, vol1, vol2, vol3, vol;
    float	xt, x0, x1, x2, x3;
    float	yt, y0, y1, y2, y3;
    float	zt, z0, z1, z2, z3;
    float 	x01, x02, x03, x0t, xt1, xt2, xt3;
    float 	y01, y02, y03, y0t, yt1, yt2, yt3;
    float 	z01, z02, z03, z0t, zt1, zt2, zt3;
    float	yz012, yz013, yz023, yzt12, yzt13;
    float	yzt23, yz0t2, yz0t3, yz01t, yz02t;
    Point       *pt, *p0, *p1, *p2, *p3;
    BaryCoord	*b;
    float	fuzz;

    pt   = args->point;
    p0   = args->p0;
    p1   = args->p1;
    p2   = args->p2;
    p3   = args->p3;
    b    = args->bary;

    xt = pt->x;    yt = pt->y;    zt = pt->z;
    x0 = p0->x;    y0 = p0->y;    z0 = p0->z;
    x1 = p1->x;    y1 = p1->y;    z1 = p1->z;
    x2 = p2->x;    y2 = p2->y;    z2 = p2->z;
    x3 = p3->x;    y3 = p3->y;    z3 = p3->z;

    x01 = x1 - x0;     y01 = y1 - y0;     z01 = z1 - z0;
    x02 = x2 - x0;     y02 = y2 - y0;     z02 = z2 - z0;
    x03 = x3 - x0;     y03 = y3 - y0;     z03 = z3 - z0;
    x0t = xt - x0;     y0t = yt - y0;     z0t = zt - z0;
    xt1 = x1 - xt;     yt1 = y1 - yt;     zt1 = z1 - zt;
    xt2 = x2 - xt;     yt2 = y2 - yt;     zt2 = z2 - zt;
    xt3 = x3 - xt;     yt3 = y3 - yt;     zt3 = z3 - zt;

    yz012 = y01 * z02 - y02 * z01;
    yz013 = y01 * z03 - y03 * z01;
    yz023 = y02 * z03 - y03 * z02;
    yzt12 = yt1 * zt2 - yt2 * zt1;
    yzt13 = yt1 * zt3 - yt3 * zt1;
    yzt23 = yt2 * zt3 - yt3 * zt2;
    yz0t2 = y0t * z02 - y02 * z0t;
    yz0t3 = y0t * z03 - y03 * z0t;
    yz01t = y01 * z0t - y0t * z01;
    yz02t = y02 * z0t - y0t * z02;

    vol0 = xt1 * yzt23 - xt2 * yzt13 + xt3 * yzt12;
    vol1 = x0t * yz023 - x02 * yz0t3 + x03 * yz0t2;
    vol2 = x01 * yz0t3 - x0t * yz013 + x03 * yz01t;
    vol3 = x01 * yz02t - x02 * yz01t + x0t * yz012;

    vol  = vol0 + vol1 + vol2 + vol3;

    if (vol == 0.0)
	return ZERO_VOLUME;

    b->p = vol0;
    b->q = vol1;
    b->r = vol2;
    b->s = vol3;

#if 1
    if (((b->p >=  0.0)&&(b->q >=  0.0)&&(b->r >=  0.0)&&(b->s >=  0.0)) ||
	((b->p <=  0.0)&&(b->q <=  0.0)&&(b->r <=  0.0)&&(b->s <=  0.0)))
#else
    if (((b->p >=  0.0)&&(b->q >=  0.0)&&(b->r >=  0.0)&&(b->s >=  0.0)) ||
	((b->p <=  0.0)&&(b->q <=  0.0)&&(b->r <=  0.0)&&(b->s <=  0.0)) ||
	((b->p ==  0.0)||(b->q ==  0.0)||(b->r ==  0.0)||(b->s ==  0.0)))
#endif
    {
	vol = 1.0 / vol;
	b->p *= vol;
	b->q *= vol;
	b->r *= vol;
	b->s *= vol;
	return INSIDE;
    }

    fuzz = args->fuzz * vol;
    
    if (((b->p >= -fuzz)&&(b->q >= -fuzz)&&(b->r >= -fuzz)&&(b->s >= -fuzz)) ||
	((b->p <=  fuzz)&&(b->q <=  fuzz)&&(b->r <=  fuzz)&&(b->s <=  fuzz)))
    {
	vol = 1.0 / vol;
	b->p *= vol;
	b->q *= vol;
	b->r *= vol;
	b->s *= vol;
	return POS_FUZZ;
    }

    if (vol > 0)
	return POS_OUT;
    else
	return NEG_OUT;
}


static int
_dxfbaryExit(int side, int index, TetraArgs *args)
{
    float     best;		/* best choice so far */
    int       f;
    int       *nbrs;
    float     *be;
    int	      *v, vc;
    int       n;
    float     d;

    nbrs = (int *)(args->neighbors + index);
    be   = (float *)args->bary;
    v    = args->visited;
    vc   = args->vCount;

    f = -1;

    if (NegSide(side))
    {
	best = -DXD_MAX_FLOAT;

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d > 0.0 && d > best)
	{
	    best = d; f = n;
	}

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d > 0.0 && d > best)
	{
	    best = d; f = n;
	}

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d > 0.0 && d > best)
	{
	    best = d; f = n;
	}

	n = *nbrs; d = *be;
	if (n >= 0 && v[n] != vc && d > 0.0 && d > best)
	{
	    best = d; f = n;
	}
    }
    else
    {
	best = DXD_MAX_FLOAT;

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d < 0.0 && d < best)
	{
	    best = d; f = n;
	}

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d < 0.0 && d < best)
	{
	    best = d; f = n;
	}

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d < 0.0 && d < best)
	{
	    best = d; f = n;
	}

	n = *nbrs++; d = *be++;
	if (n >= 0 && v[n] != vc && d < 0.0 && d < best)
	{
	    best = d; f = n;
	}
    }

    return f;
}

static int
_dxfTetraSearch(TetraArgs *args, int index)
{
    int  	nbr;
    int  	side;
    Tetrahedron	*tet;
    int		knt;
    int		*vPtr;
    ArrayHandle	points;
    Tetrahedron	*tetras;
    int		*visited, vCount;
    int		limit;
    int		fuzzFlag;
    Point	ptBuf[4];

    points    = args->points;
    tetras    = args->tetras;
    visited   = args->visited;
    vCount    = args->vCount;
    limit     = args->limit;
    fuzzFlag  = args->fuzzFlag;

    nbr = 0;

    for (knt = 0; index != -1 && knt < limit; knt++, index = nbr)
    {
	if (visited)
	{
	    vPtr = visited + index;

	    if (*vPtr == vCount)
		break;
	    else
		*vPtr = vCount;
	}

	tet = tetras + index;

	args->p0  = (Point *)
		    DXGetArrayEntry(points, tet->p, (Pointer)(ptBuf+0));
	args->p1  = (Point *)
		    DXGetArrayEntry(points, tet->q, (Pointer)(ptBuf+1));
	args->p2  = (Point *)
		    DXGetArrayEntry(points, tet->r, (Pointer)(ptBuf+2));
	args->p3  = (Point *)
		    DXGetArrayEntry(points, tet->s, (Pointer)(ptBuf+3));

#ifdef DIAG
	nT ++;
#endif

	side = _dxfbarycentric_coords(args);

	/*
	 * If the point was contained in the tetrahedra, return its index.
	 * Otherwise, if its within fuzz of the tetrahedra and the tetrahedra
	 * has a neighbor in that direction, continue.  If its within fuzz of
	 * the primitive but the tetra has no neighbor in that direction,
	 * return the primitive's index.  If the point is not within fuzz of
	 * the primitive, continue the neighbor search.
	 *
	 * But first, if the tetra is of zero volume, neighbor walking will
	 * not work.
	 */
	if (side == ZERO_VOLUME)
	     break;

	if (side == INSIDE)
	    return index;

	/*
	 * If the fuzz flag is on and we are within fuzz and there's
	 * no neighbor, this one will do.
	 */
	if (fuzzFlag == FUZZ_ON && WithinFuzz(side))
	    return index;

	/*
	 * Otherwise, if there's no neighbor, we failed.  If there is,
	 * we keep going.
	 */

	if (knt < (limit-1))
	{
	    nbr = _dxfbaryExit(side, index, args);
	    if (nbr < 0)
		return -1;
	}

	if (! _dxfBBoxTest(args, nbr))
	    return -1;
    }

    return -1;
}

Interpolator
_dxfTetrasInterpolator_LocalizeInterpolator(TetrasInterpolator ti)
{
    if (ti->fieldInterpolator.localized)
	return (Interpolator)ti;

    ti->fieldInterpolator.localized = 1;

    if (ti->fieldInterpolator.initialized)
    {
	ti->data      = DXGetArrayDataLocal(ti->dataArray);
	ti->tetras    = (Tetrahedron *)DXGetArrayDataLocal(ti->tetrasArray);
	ti->neighbors = (TetNeighbors *)DXGetArrayDataLocal(ti->neighborsArray);
    }

    if (DXGetError())
	return NULL;
    else
	return (Interpolator)ti;
}
