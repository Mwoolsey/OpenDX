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
#include "cubesIIClass.h"

#define FALSE 0
#define TRUE 1

#define SEARCH_LIMIT	10

typedef struct
{
    ArrayHandle	pHandle;
    int		*nbrs;
    ArrayHandle	cHandle;
    int		*visited;
    int		vCount;
    Point	*point;
    Point	*p0, *p1, *p2, *p3;
    int		tetra[4];
    float	*bary;
    float	fuzz;
    int		fuzzFlag;
    int		limit;
    int		*gridCounts;
    int		Cyz, Cz;
    CubesIIInterpolator ci;
} CubesIIArgs;

#define NBR_0	0x01
#define NBR_1	0x02
#define NBR_2	0x04
#define NBR_3	0x08
#define NBR_4	0x10
#define NBR_5	0x20

static Error _dxfInitializeTask(Pointer);
static Error _dxfInitialize(CubesIIInterpolator);
static void  _dxfCleanup(CubesIIInterpolator);
static int   CubesIISearch(CubesIIArgs *, int);
static int   IsInCube(CubesIIArgs *, int);
static int   IsInTetra(CubesIIArgs *);
static int   GetRegularNeighbor(CubesIIArgs *, int, int);

#define DIAGNOSTIC(str)  \
	DXMessage("cubes interpolator failure: %s", (str))

/*
 * Return values from IsInTetra, IsInCube (plus 0-5)
 */
#define INSIDE		-2
#define OUTSIDE		-1

int
_dxfRecognizeCubesII(Field field)
{
    Array 	array;
    Type	type;
    Category 	cat;
    int		rank, shape[32];
    int		status;

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "cubes");
    
    status = OK;

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
    {
	DIAGNOSTIC("missing positions component");
	status = ERROR;
    }
    else
    {
	if (! DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape))
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
    if (!array)
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

CubesIIInterpolator
_dxfNewCubesIIInterpolator(Field field, 
	enum interp_init initType, double fuzz, Matrix *m) 
{
    return (CubesIIInterpolator)_dxf_NewCubesIIInterpolator(field,
		initType, fuzz, m, &_dxdcubesiiinterpolator_class);
}

CubesIIInterpolator
_dxf_NewCubesIIInterpolator(Field field,
		enum interp_init initType, float fuzz, Matrix *m,
		struct cubesiiinterpolator_class *class)
{
    CubesIIInterpolator	ci;

    if (DXEmptyField(field))
	return NULL;
    
    ci = (CubesIIInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
				(struct fieldinterpolator_class *)class);
    if (! ci)
	return NULL;

    ci->pHandle       = NULL;
    ci->pointsArray   = NULL;
    ci->dHandle	      = NULL;
    ci->dataArray     = NULL;
    ci->cHandle       = NULL;
    ci->cubesArray    = NULL;
    ci->visited       = NULL;
    ci->grid	      = NULL;
    ci->vCount	      = 1;

    if (initType == INTERP_INIT_PARALLEL)
    {
	if (! DXAddTask(_dxfInitializeTask, (Pointer)&ci, sizeof(ci), 1.0))
	{
	    DXDelete((Object)ci);
	    return NULL;
	}
    }
    else if (initType == INTERP_INIT_IMMEDIATE)
    {
	if (! _dxfInitialize(ci))
	{
	    DXDelete((Object)ci);
	    return NULL;
	}
    }

    ci->hint = -1;
    
    return ci;
}

static Error
_dxfInitializeTask(Pointer p)
{
    int status;

    status = _dxfInitialize(*(CubesIIInterpolator *)p);
    return status;
}

#define ISOBUF	32

static Error
_dxfInitialize(CubesIIInterpolator ci)
{
    Field	   field;
    Type	   dataType;
    Category	   dataCategory;
    register int   i;

    field = (Field) ((Interpolator)ci)->dataObject;

    ci->fieldInterpolator.initialized = 1;

    /*
     * De-reference data
     */
    ci->dataArray   = (Array)DXGetComponentValue(field, "data");
    if (!ci->dataArray) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return ERROR;
    }

    DXGetArrayInfo(ci->dataArray, NULL, &((Interpolator)ci)->type,
		&((Interpolator)ci)->category, &((Interpolator)ci)->rank,
		((Interpolator)ci)->shape);
	
    ci->dHandle = DXCreateArrayHandle(ci->dataArray);
    if (! ci->dHandle)
	return ERROR;

    /*
     * Get the grid. 
     */
    ci->pointsArray = (Array)DXGetComponentValue(field, "positions");
    if (!ci->pointsArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return ERROR;
    }

    ci->pHandle = DXCreateArrayHandle(ci->pointsArray);
    if (! ci->pHandle)
	return ERROR;

    /*
     * Get the cubes. 
     */
    ci->cubesArray = (Array)DXGetComponentValue(field, "connections");
    if (!ci->cubesArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	return ERROR;
    }

    if (! DXGetArrayInfo(ci->cubesArray, &ci->nCubes, NULL, NULL, NULL, NULL))
	return ERROR;
    
    if (NULL == (ci->cHandle = DXCreateArrayHandle(ci->cubesArray)))
	return ERROR;
    
    /*
     * If cubes are a mesh array (eg. regular connections), get the mesh info.
     * Otherwise, get the neighbors, generating one if one exists.  If NULL
     * is returned, well, OK.
     */
    if (DXQueryGridConnections(ci->cubesArray, NULL, ci->gridCounts))
    {
	ci->nbrsArray = NULL;
	ci->gridCounts[0] -= 1;
	ci->gridCounts[1] -= 1;
	ci->gridCounts[2] -= 1;
	ci->Cyz = ci->gridCounts[1]*ci->gridCounts[2];
	ci->Cz  = ci->gridCounts[2];
    }
    else
	ci->nbrsArray = (Array)DXNeighbors(field);

    /*
     * get info about data values
     */
    DXGetArrayInfo(ci->dataArray, NULL, &dataType, &dataCategory, NULL, NULL);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    ci->nElements = DXGetItemSize(ci->dataArray) / DXTypeSize(dataType);

    if (ci->fieldInterpolator.localized)
    {
	if (ci->nbrsArray)
	    ci->nbrs = (int   *)DXGetArrayDataLocal(ci->nbrsArray);
	else
	    ci->nbrs = NULL;
    }
    else
    {
	if (ci->nbrsArray)
	    ci->nbrs = (int   *)DXGetArrayData(ci->nbrsArray);
	else
	    ci->nbrs = NULL;
    }

    if (DXGetError())
	return ERROR;

    /* 
     * Create the search grid
     */
    ci->grid = _dxfMakeSearchGrid(((Interpolator)ci)->max,
			((Interpolator)ci)->min, ci->nCubes, 3);
		    
    if (! ci->grid)
	return ERROR;

    for (i = 0; i < ci->nCubes; i++)
    {
	float *points[8], pbuf[24];
	register int j;
	int   cBuf[8], *c;

	c = (int *)DXGetArrayEntry(ci->cHandle, i, (Pointer)cBuf);

	for (j = 0; j < 8; j++)
	    points[j] = (float *)
		DXGetArrayEntry(ci->pHandle, c[j], (Pointer)(pbuf+j*3));

	if (! _dxfAddItemToSearchGrid(ci->grid, points, 8, i))
	    break;
    }

    ci->gridFlag = 1;
    
    ci->visited = (int *)DXAllocate(ci->nCubes * sizeof(int));
    if (! ci->visited)
	return ERROR;

    memset(ci->visited, 0, ci->nCubes*sizeof(int));

    return OK;
}

Error
_dxfCubesIIInterpolator_Delete(CubesIIInterpolator ci)
{
    _dxfCleanup(ci);
    _dxfFieldInterpolator_Delete((FieldInterpolator) ci);

    return OK;
}

int
_dxfCubesIIInterpolator_PrimitiveInterpolate(CubesIIInterpolator ci,
		int *n, float **points, Pointer *values, int fuzzFlag)
{
    int                    i;
    int                    found;
    int	                   cubeI;
    Point                  *p;
    Pointer                v;
    float                  fuzz;
    float                  bary[4];
    CubesIIArgs            args;
    int		           *visited;
    int		           dep;
    float                  xMin, xMax;
    float                  yMin, yMax;
    float                  zMin, zMax;
    int		           itemSize;
    ubyte		   *dbuf = NULL;
    InvalidComponentHandle icH = ((FieldInterpolator)ci)->invCon;
    Matrix		   *xform;


    if (! ci->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(ci))
	{
	    _dxfCleanup(ci);
	    return 0;
	}

	ci->fieldInterpolator.initialized = 1;
    }

    xMin = ((Interpolator)ci)->min[0];
    yMin = ((Interpolator)ci)->min[1];
    zMin = ((Interpolator)ci)->min[2];

    xMax = ((Interpolator)ci)->max[0];
    yMax = ((Interpolator)ci)->max[1];
    zMax = ((Interpolator)ci)->max[2];

    dep = ci->fieldInterpolator.data_dependency;

    itemSize = ci->nElements*DXTypeSize(((Interpolator)ci)->type);

    args.pHandle      = ci->pHandle;
    args.cHandle      = ci->cHandle;
    args.nbrs         = ci->nbrs;
    args.fuzz         = fuzz = ci->fieldInterpolator.fuzz;
    args.visited      = ci->visited;
    args.bary         = bary;
    args.limit        = SEARCH_LIMIT;

    args.gridCounts = ci->gridCounts;
    args.Cyz        = ci->Cyz;
    args.Cz         = ci->Cz;

    args.ci = ci;

    visited = ci->visited;
    
    dbuf = (ubyte *)DXAllocate(4*itemSize);
    if (! dbuf)
	return 0;

    if (((FieldInterpolator)ci)->xflag)
	xform = &((FieldInterpolator)ci)->xform;
    else 
	xform = NULL;

    p = (Point *)*points;
    v = (Pointer)*values;


    while (*n)
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

	found = OUTSIDE;

	args.fuzzFlag = fuzzFlag;
	args.point    = pPtr;
	args.vCount   = ci->vCount++;

	/*
	 * Check the hint first, if one exists...
	 */

	cubeI = ci->hint;

	/*
	fuzz = fuzzFlag * fuzz;
	*/

	if (cubeI >= 0)
	{
	    found = CubesIISearch(&args, cubeI);
	}

	if (found == OUTSIDE)
	{
	    /* 
	     * If the sample was not found in the hint, look in the bin
	     *
	     * For each cube in the bin...
	     */
	    if (ci->gridFlag)
	    {
		_dxfInitGetSearchGrid(ci->grid, (float *)p);
		while((cubeI = _dxfGetNextSearchGrid(ci->grid)) != -1
							&& found==OUTSIDE)
		{
		    if (visited[cubeI] != args.vCount)
		    {
			found = CubesIISearch(&args, cubeI);
		    }
		}
	    }
	    else
	    {
		for (cubeI = 0; cubeI < ci->nCubes && found==OUTSIDE; cubeI++)
		    if (visited[cubeI] != args.vCount)
		    {
			found = CubesIISearch(&args, cubeI);
		    }
	    }

	}

	if (found == OUTSIDE || (icH && DXIsElementInvalid(icH, found)))
	    break;
	
	ci->hint = found;

#define INTERPOLATE(type, round)					    \
{									    \
    type *d0, *d1, *d2, *d3, *r;					    \
									    \
    r = (type *)v;							    \
									    \
    /*									    \
     * Get pointers to first data elements for each corner		    \
     */									    \
    d0 = (type *)DXGetArrayEntry(ci->dHandle,			    	    \
			args.tetra[0], (Pointer)(dbuf));    		    \
									    \
    d1 = (type *)DXGetArrayEntry(ci->dHandle,			    	    \
			args.tetra[1], (Pointer)(dbuf+itemSize));    	    \
									    \
    d2 = (type *)DXGetArrayEntry(ci->dHandle,			    	    \
			args.tetra[2], (Pointer)(dbuf+2*itemSize));    	    \
									    \
    d3 = (type *)DXGetArrayEntry(ci->dHandle,			    	    \
			args.tetra[3], (Pointer)(dbuf+3*itemSize));    	    \
									    \
    for (i = 0; i < ci->nElements; i++)					    \
    {									    \
	*r++ = round + (bary[0] * (*d0++)) + (bary[1] * (*d1++)) +	    \
		   (bary[2] * (*d2++)) + (bary[3] * (*d3++));		    \
    }									    \
									    \
    v = (Pointer)r;							    \
}

	if (((FieldInterpolator)ci)->cstData)
	{
	    memcpy(v, ((FieldInterpolator)ci)->cstData, itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
	else if (dep == DATA_POSITIONS_DEPENDENT)
	{
            Type dataType;

            if ((dataType = ((Interpolator)ci)->type) == TYPE_FLOAT)
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
	    memcpy(v, DXGetArrayEntry(ci->dHandle,
				    found, (Pointer)dbuf), itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
    
	(*n) --;
	p++;

	/*
	 * Only use fuzz the first time
	 */
	fuzzFlag = FUZZ_OFF;
    }

    if (dbuf)
	DXFree((Pointer)dbuf);

    *points = (float *)p;
    *values = (Pointer)v;

    return OK;
}

static void
_dxfCleanup(CubesIIInterpolator ci)
{
    if (ci->fieldInterpolator.localized)
    {
	if (ci->nbrs)
	    DXFreeArrayDataLocal(ci->nbrsArray, (Pointer)ci->nbrs);
    }

    if (ci->dHandle)
    {
	DXFreeArrayHandle(ci->dHandle);
	ci->dHandle = NULL;
    }

    if (ci->pHandle)
    {
	DXFreeArrayHandle(ci->pHandle);
	ci->pHandle = NULL;
    }

    if (ci->cHandle)
    {
	DXFreeArrayHandle(ci->cHandle);
	ci->cHandle = NULL;
    }

    DXFree((Pointer)ci->visited);

    _dxfFreeSearchGrid(ci->grid);
}

Object
_dxfCubesIIInterpolator_Copy(CubesIIInterpolator old, enum _dxd_copy copy)
{
    CubesIIInterpolator new;

    new = (CubesIIInterpolator)
	    _dxf_NewObject((struct object_class *)&_dxdcubesiiinterpolator_class);

    if (!(_dxf_CopyCubesIIInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

CubesIIInterpolator
_dxf_CopyCubesIIInterpolator(CubesIIInterpolator new, 
				CubesIIInterpolator old, enum _dxd_copy copy)
{

    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nPoints   = old->nPoints;
    new->nCubes    = old->nCubes;
    new->nElements = old->nElements;
    new->hint      = old->hint;
    new->visited   = NULL;

    if (new->fieldInterpolator.initialized)
    {
	new->pointsArray   = old->pointsArray;
	new->cubesArray    = old->cubesArray;
	new->dataArray     = old->dataArray;
	new->nbrsArray     = old->nbrsArray;

	new->cHandle = DXCreateArrayHandle(old->cubesArray);
	new->pHandle = DXCreateArrayHandle(old->pointsArray);
	new->dHandle = DXCreateArrayHandle(old->dataArray);
	if (! new->cHandle || ! new->pHandle || ! new->dHandle)
	    return NULL;

	if (new->fieldInterpolator.localized)
	{
	    if (old->nbrsArray)
		new->nbrs    = (int *)DXGetArrayDataLocal(new->nbrsArray);
	}
	else
	{
	    if (old->nbrsArray)
		new->nbrs    = (int   *)DXGetArrayData(new->nbrsArray);
	}

	if (old->gridFlag)
	{
	    new->gridFlag = 1;
	    new->grid = _dxfCopySearchGrid(old->grid);
	    if (! new->grid)
		return NULL;
	}
	else
	    new->gridFlag = 0;
    }
    else
    {
	new->pointsArray  = NULL;
	new->dataArray    = NULL;
	new->cubesArray   = NULL;
	new->nbrsArray    = NULL;

	new->cHandle  = NULL;
	new->dHandle  = NULL;
	new->pHandle  = NULL;

	new->nbrs    = NULL;
	new->visited = NULL;

	new->gridFlag = 0;
    }

    if (! old->nbrsArray)
    {
	memcpy(new->gridCounts, old->gridCounts, sizeof(old->gridCounts));
	new->Cyz = old->Cyz;
	new->Cz  = old->Cz;
    }

    new->vCount      = 1;

    new->visited = (int *)DXAllocate(new->nCubes*sizeof(int));
    if (! new->visited)
	return NULL;

    memset(new->visited, 0, new->nCubes*sizeof(int));

    if (DXGetError())
	return NULL;

    return new;
}

Interpolator
_dxfCubesIIInterpolator_LocalizeInterpolator(CubesIIInterpolator ci)
{
    ci->fieldInterpolator.localized = 1;

    if (ci->fieldInterpolator.initialized)
    {
	if (ci->nbrs)
	    ci->nbrs = (int *)DXGetArrayDataLocal(ci->nbrsArray);
    }

    if (DXGetError())
        return NULL;
    else
        return (Interpolator)ci;
}

#define TET0	0x01
#define TET1	0x02
#define TET2	0x04
#define TET3	0x08
#define TET4	0x10
#define TET5	0x20

static int
IsInCube(CubesIIArgs *args, int index)
{
    int  *cube, cubeBuf[8];
    char  been_here;
    int   *tet;

#ifdef TEST
    nCubeSearches ++;
#endif

    cube = DXGetArrayEntry(args->cHandle, index, (Pointer)cubeBuf);
    tet   = args->tetra;

    been_here = 0;

    goto tetra1;

tetra0:
    if (been_here & TET0) goto done;

    been_here |= TET0;

    tet[0] = cube[0]; tet[1] = cube[3]; tet[2] = cube[1]; tet[3] = cube[5];
    switch(IsInTetra(args)) {
	case INSIDE:	return INSIDE;
	case 0:		return 5;
	case 1:		return 2;
	case 2:		goto tetra3;
	case 3:		return 0;
    }
    
tetra1:
    if (been_here & TET1) goto done;

    been_here |= TET1;

    tet[0] = cube[5]; tet[1] = cube[2]; tet[2] = cube[3]; tet[3] = cube[7];
    switch(IsInTetra(args)) {
	case INSIDE:	return INSIDE;
	case 0:		return 3;
	case 1:		return 5;
	case 2:		goto tetra2;
	case 3:		goto tetra3;
    }
    
tetra2:
    if (been_here & TET2) goto done;

    been_here |= TET2;

    tet[0] = cube[5]; tet[1] = cube[4]; tet[2] = cube[2]; tet[3] = cube[7];
    switch(IsInTetra(args)) {
	case INSIDE:	return INSIDE;
	case 0:		goto tetra5;
	case 1:		goto tetra1;
	case 2:		return 1;
	case 3:		goto tetra4;
    }
    
tetra3:
    if (been_here & TET3) goto done;

    been_here |= TET3;

    tet[0] = cube[5]; tet[1] = cube[3]; tet[2] = cube[2]; tet[3] = cube[0];
    switch(IsInTetra(args)) {
	case INSIDE:	return INSIDE;
	case 0:		return 0;
	case 1:		goto tetra4;
	case 2:		goto tetra0;
	case 3:		goto tetra1;
    }
    
tetra4:
    if (been_here & TET4) goto done;

    been_here |= TET4;

    tet[0] = cube[5]; tet[1] = cube[2]; tet[2] = cube[4]; tet[3] = cube[0];
    switch(IsInTetra(args)) {
	case INSIDE:	return INSIDE;
	case 0:		return 4;
	case 1:		return 2;
	case 2:		goto tetra3;
	case 3:		goto tetra2;
    }
    
tetra5:
    if (been_here & TET5) goto done;

    been_here |= TET5;

    tet[0] = cube[4]; tet[1] = cube[2]; tet[2] = cube[7]; tet[3] = cube[6];
    switch(IsInTetra(args)) {
	case INSIDE:	return INSIDE;
	case 0:		return 3;
	case 1:		return 1;
	case 2:		return 4;
	case 3:		goto tetra2;
    }
    
    return OUTSIDE;

done:
   return OUTSIDE;
}

static int
IsInTetra(CubesIIArgs *args)
{
    Point	*pt;
    Vector      *vert, vbuf;
    float	*bary;
    float 	v0, v1, v2, v3, v;
    float	xt, x0, x1, x2, x3;
    float	yt, y0, y1, y2, y3;
    float	zt, z0, z1, z2, z3;
    float 	x01, x02, x03, x0t, xt1, xt2, xt3;
    float 	y01, y02, y03, y0t, yt1, yt2, yt3;
    float 	z01, z02, z03, z0t, zt1, zt2, zt3;
    float	yz012, yz013, yz023, yzt12, yzt13;
    float	yzt23, yz0t2, yz0t3, yz01t, yz02t;
    int		p, q, r, s;
    float	opp, fuzz;
    int		k;

    pt    = args->point;
    bary  = args->bary;
    fuzz  = args->fuzz;

    p = args->tetra[0];
    q = args->tetra[1];
    r = args->tetra[2];
    s = args->tetra[3];

    xt = pt->x;           yt = pt->y;           zt = pt->z;

    vert = (Vector *)DXGetArrayEntry(args->pHandle, p, (Pointer)&vbuf);
    x0 = vert->x; y0 = vert->y; z0 = vert->z;

    vert = (Vector *)DXGetArrayEntry(args->pHandle, q, (Pointer)&vbuf);
    x1 = vert->x; y1 = vert->y; z1 = vert->z;

    vert = (Vector *)DXGetArrayEntry(args->pHandle, r, (Pointer)&vbuf);
    x2 = vert->x; y2 = vert->y; z2 = vert->z;

    vert = (Vector *)DXGetArrayEntry(args->pHandle, s, (Pointer)&vbuf);
    x3 = vert->x; y3 = vert->y; z3 = vert->z;

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

    v0 = xt1 * yzt23 - xt2 * yzt13 + xt3 * yzt12;
    v1 = x0t * yz023 - x02 * yz0t3 + x03 * yz0t2;
    v2 = x01 * yz0t3 - x0t * yz013 + x03 * yz01t;
    v3 = x01 * yz02t - x02 * yz01t + x0t * yz012;

    v  = x01 * yz023 - x02 * yz013 + x03 * yz012;

    fuzz *= v;
    if (fuzz < 0.0) fuzz = -fuzz;

    if (v == 0.0)
    {
	bary[0] = 0.0;
	bary[1] = 0.0;
	bary[2] = 0.0;
	bary[3] = 0.0;
	return OUTSIDE;
    }

    if (((v0 >= -fuzz) && (v1 >= -fuzz) && (v2 >= -fuzz) && (v3 >= -fuzz))
     || ((v0 <=  fuzz) && (v1 <=  fuzz) && (v2 <=  fuzz) && (v3 <=  fuzz)))
    {
	v = 1.0 / v;
	bary[0] = v0 * v;
	bary[1] = v1 * v;
	bary[2] = v2 * v;
	bary[3] = 1.0 - (bary[0] + bary[1] + bary[2]);
	return INSIDE;
    }

    if (v > 0)
    {
        k = 0; opp = v0;
        if (v1 < opp) {opp = v1; k = 1;}
        if (v2 < opp) {opp = v2; k = 2;}
        if (v3 < opp) {          k = 3;}
    }
    else
    {
        k = 0; opp = v0;
        if (v1 > opp) {opp = v1; k = 1;}
        if (v2 > opp) {opp = v2; k = 2;}
        if (v3 > opp) {          k = 3;}
    }

    return k;
}

static int 
CubesIISearch(CubesIIArgs *args, int index)
{
    int		*nbrs;
    int         *visited, vCount;
    int         limit;
    int		k, new;
    int		*vPtr;

    visited = args->visited;
    vCount  = args->vCount;
    limit   = args->limit;
    nbrs    = args->nbrs;
    
    for (k = 0; index != OUTSIDE && k < limit; k++)
    {
	if (visited)
	{
	    vPtr = visited + index;

	    if (*vPtr == vCount)
		break;
	    else
		*vPtr = vCount;
	}

	new = IsInCube(args, index);

	/*
	 * Results of the IsInCube routine are either INSIDE, OUTSIDE,
	 * or the exit side for the neighbors walk.
	 */
	switch(new)
	{
	    case INSIDE:  return index;
	    case OUTSIDE: index = OUTSIDE; break;
	    default:	  if (nbrs)
			      index = nbrs[(index*6) + new]; 
			  else
			      index = GetRegularNeighbor(args, index, new);
	}
    }

    return OUTSIDE;
}

static int
GetRegularNeighbor(CubesIIArgs *args, int index, int exit)
{
    int i;

    switch(exit) {
	case 0:
	    i = index / args->Cyz;
	    if (i > 0) 
		return index - args->Cyz;
	    else       
		return -1;
	
	case 1:
	    i = index / args->Cyz;
	    if (i < (args->gridCounts[0] - 1)) 
		return index + args->Cyz;
	    else       
		return -1;
	    
	case 2:
	    i = (index % args->Cyz) / args->Cz;
	    if (i > 0)
		return index - args->Cz;
	    else
		return -1;
	
	case 3:
	    i = (index % args->Cyz) / args->Cz;
	    if (i < (args->gridCounts[1] - 1))
		return index + args->Cz;
	    else
		return -1;
	    
	case 4:
	    i = index % args->gridCounts[2];
	    if (i > 0)
		return index - 1;
	    else
		return -1;
	    
	case 5:
	    i = index % args->gridCounts[2];
	    if (i < (args->gridCounts[2] - 1))
		return index + 1;
	    else
		return -1;

	default:
	    DXMessage("illegal exit");
	    return -1;
    }
}
