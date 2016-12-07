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
#include "math.h"
#include "cubesRRClass.h"

static Error  _dxfInitialize(CubesRRInterpolator);
static Error  _dxfInitializeTask(Pointer);
static void   _dxfCleanup(CubesRRInterpolator);

#define DIAGNOSTIC(str) \
	DXMessage("regular cubes interpolator failure: %s", (str))

int
_dxfRecognizeCubesRR(Field field)
{
    Array	array;
    int		nDim;
    Type	type;
    Category	cat;
    int		rank, shape[32];

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "cubes");

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
	return ERROR;
    else
    {
	if (!DXQueryGridPositions(array, &nDim, NULL, NULL, NULL))
	    return ERROR;
	
	if (! DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape))
	    return ERROR;

	if (cat != CATEGORY_REAL)
	    return ERROR;

	if (nDim != 3)
	    return ERROR;
    }

    array = (Array)DXGetComponentValue(field, "connections");
    if (!array)
	return ERROR;
    else
    {
	if (!DXQueryGridConnections(array, &nDim, NULL))
	    return ERROR;
	
	if (nDim != 3)
	    return ERROR;
    }

    return OK;
}

CubesRRInterpolator
_dxfNewCubesRRInterpolator(Field field,
		enum interp_init initType, double fuzz, Matrix *m)
{
    return (CubesRRInterpolator)_dxf_NewCubesRRInterpolator(field,
			initType, fuzz, m, &_dxdcubesrrinterpolator_class);
}


CubesRRInterpolator 
_dxf_NewCubesRRInterpolator(Field field,
			enum interp_init initType, float fuzz, Matrix *m,
			struct cubesrrinterpolator_class *class)
{
    CubesRRInterpolator ci;

    ci = (CubesRRInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
			(struct fieldinterpolator_class *)class);
    if (! ci)
	return NULL;

    ci->pointsArray = NULL;
    ci->dataArray   = NULL;
    ci->data  = NULL;

    /*
     * The grid should be regular
     */
    {
	float localOrigin[3], displacement[3];
	Array connections, positions;
	Matrix mIn;

	positions = (Array)DXGetComponentValue(field, "positions");
	connections = (Array)DXGetComponentValue(field, "connections");

	if (!positions || !connections)
	{
	    DXDelete((Object)(ci));
	    return NULL;
	}

	DXQueryGridPositions(positions, NULL, NULL, localOrigin, (float *)mIn.A);
	DXGetMeshOffsets((MeshArray)connections, ci->meshOffsets);

	displacement[0] = ci->meshOffsets[0]*mIn.A[0][0] +
			  ci->meshOffsets[1]*mIn.A[1][0] +
			  ci->meshOffsets[2]*mIn.A[2][0];
	displacement[1] = ci->meshOffsets[0]*mIn.A[0][1] +
			  ci->meshOffsets[1]*mIn.A[1][1] +
			  ci->meshOffsets[2]*mIn.A[2][1];
	displacement[2] = ci->meshOffsets[0]*mIn.A[0][2] +
			  ci->meshOffsets[1]*mIn.A[1][2] +
			  ci->meshOffsets[2]*mIn.A[2][2];
	
	mIn.b[0] = localOrigin[0] - displacement[0];
	mIn.b[1] = localOrigin[1] - displacement[1];
	mIn.b[2] = localOrigin[2] - displacement[2];

	((Interpolator)ci)->matrix = DXInvert(mIn);
    }

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

    return ci;
}

static Error
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(CubesRRInterpolator *) p);
}

static Error
_dxfInitialize(CubesRRInterpolator ci)
{
    Field		field;
    int			nDim;
    int			i;
    Type		dataType;
    Category		dataCategory;

    field = (Field)((Interpolator)ci)->dataObject;

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
    DXReference((Object)ci->dataArray);

    DXGetArrayInfo(ci->dataArray, NULL, &((Interpolator)ci)->type,
	&((Interpolator)ci)->category, &((Interpolator)ci)->rank,
	((Interpolator)ci)->shape);
    
    ci->data = DXCreateArrayHandle(ci->dataArray);
    if (! ci->data)
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
    DXReference((Object)ci->pointsArray);

    /*
     * get info about data values
     */
    DXGetArrayInfo(ci->dataArray, NULL, &dataType, &dataCategory, NULL, NULL);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    ci->nElements = DXGetItemSize(ci->dataArray) / DXTypeSize(dataType);

    DXQueryGridPositions(ci->pointsArray, &nDim, ci->counts, NULL, NULL);

    /*
     * Figure out how big to increment pointers along each dimension.
     * Use either number of positions or number of primitives along the
     * axis, depending on whether the data is positions or connections
     * dependant.
     */

    if (ci->fieldInterpolator.data_dependency == DATA_POSITIONS_DEPENDENT)
    {
	ci->size[2] = 1;
	for (i = 1; i >= 0; i--)
	    ci->size[i] = ci->counts[i+1] * ci->size[i+1];
    }
    else
    {
	ci->size[2] = 1;
	for (i = 1; i >= 0; i--)
	    ci->size[i] = (ci->counts[i+1] - 1) * ci->size[i+1];
    }

    ci->eltStrides[2] = 1;
    ci->eltStrides[1] = ci->counts[2] - 1;
    ci->eltStrides[0] = (ci->counts[1] - 1) * ci->eltStrides[1];

    return OK;
}

Error
_dxfCubesRRInterpolator_Delete(CubesRRInterpolator ci)
{
    _dxfCleanup(ci);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) ci);
}

static void
_dxfCleanup(CubesRRInterpolator ci)
{
    if (ci->data)
    {
	DXFreeArrayHandle(ci->data);
	ci->data = NULL;
    }

    if (ci->pointsArray)
    {
	DXDelete((Object)ci->pointsArray);
	ci->pointsArray = NULL;
    }
    
    if (ci->dataArray)
    {
	DXDelete((Object)ci->dataArray);
	ci->dataArray = NULL;
    }
}

#define INTERPOLATE(type, round)					   \
{									   \
    type    *ptr000, *ptr001, *ptr010, *ptr011;			   	   \
    type    *ptr100, *ptr101, *ptr110, *ptr111;			   	   \
    type    *r = (type *)v;						   \
									   \
    ptr000 = (type *)DXGetArrayEntry(ci->data, baseIndex,		   \
				(Pointer)(dbuf));			   \
									   \
    ptr100 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz0,		   \
				(Pointer)(dbuf+itemSize));		   \
									   \
    ptr010 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz1,		   \
				(Pointer)(dbuf+2*itemSize));		   \
									   \
    ptr110 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz1+sz0,	   \
				(Pointer)(dbuf+3*itemSize));		   \
									   \
    ptr001 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz2,		   \
				(Pointer)(dbuf+4*itemSize));		   \
									   \
    ptr101 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz2+sz0,	   \
				(Pointer)(dbuf+5*itemSize));		   \
									   \
    ptr011 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz2+sz1,	   \
				(Pointer)(dbuf+6*itemSize));		   \
									   \
    ptr111 = (type *)DXGetArrayEntry(ci->data, baseIndex+sz2+sz1+sz0,	   \
				(Pointer)(dbuf+7*itemSize));		   \
									   \
    for (i = 0; i < ci->nElements; i++)					   \
	*r++ = weight000*(*ptr000++) + weight001*(*ptr001++) +		   \
	       weight010*(*ptr010++) + weight011*(*ptr011++) +		   \
	       weight100*(*ptr100++) + weight101*(*ptr101++) +		   \
	       weight110*(*ptr110++) + weight111*(*ptr111++) + round;	   \
									   \
    v = (Pointer)r;							   \
}

int
_dxfCubesRRInterpolator_PrimitiveInterpolate(CubesRRInterpolator ci,
		int *n, float **points, Pointer *values, int fuzzFlag)
{
    Point	           pt;
    int 	           ix, iy, iz;
    float	           dx, dy, dz;
    int		           sz0, sz1, sz2;
    int		           ixMax, iyMax, izMax;
    float	           weight000, weight001, weight010, weight011;
    float	           weight100, weight101, weight110, weight111;
    float	           A, B, C, D;
    int		           baseIndex, i;
    Point	           *p;
    Pointer	           v;
    float	           *mA, *mb;
    float	           fuzz;
    int		           dep;
    int 	           itemSize;
    int 	           typeSize;
    int		           *mo;
    int		           esz0, esz1;
    InvalidComponentHandle icH = ((FieldInterpolator)ci)->invCon;
    ubyte		   *dbuf = NULL;
    Matrix 		   *xform = NULL;

    if (! ci->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(ci))
	{
	    _dxfCleanup(ci);
	    return 0;
	}

	ci->fieldInterpolator.initialized = 1;
    }

    /*
     * De-reference indexing info from interpolator
     */
    sz0  = ci->size[0];
    sz1  = ci->size[1];
    sz2  = ci->size[2];
    mA = (float *)((Interpolator)ci)->matrix.A;
    mb = (float *)((Interpolator)ci)->matrix.b;
    mo = ci->meshOffsets;
    esz0 = ci->eltStrides[0];
    esz1 = ci->eltStrides[1];

    dep = ci->fieldInterpolator.data_dependency;
    typeSize = DXTypeSize(((Interpolator)ci)->type);
    itemSize = ci->nElements * typeSize;

    ixMax = ci->counts[0] - 1;
    iyMax = ci->counts[1] - 1;
    izMax = ci->counts[2] - 1;

    /*
     * De-reference bounding box
     */
    /* xMax = ((Interpolator)ci)->max[0]; */
    /* xMin = ((Interpolator)ci)->min[0]; */
    /* yMax = ((Interpolator)ci)->max[1]; */
    /* yMin = ((Interpolator)ci)->min[1]; */
    /* zMax = ((Interpolator)ci)->max[2]; */
    /* zMin = ((Interpolator)ci)->min[2]; */

    p = (Point *)*points;
    v = (Pointer)*values;

    fuzz = ((FieldInterpolator)ci)->fuzz;

    dbuf = (ubyte *)DXAllocate(8*itemSize);
    if (! dbuf)
	return 0;
    
    if (((FieldInterpolator)ci)->xflag)
	xform = &(((FieldInterpolator)ci)->xform);

    while (*n > 0)
    {

	if (((FieldInterpolator)ci)->xflag)
	{
	    float x, y, z;

	    x = p->x*xform->A[0][0] + 
		p->y*xform->A[1][0] +
		p->z*xform->A[2][0] +
		xform->b[0];

	    y = p->x*xform->A[0][1] + 
		p->y*xform->A[1][1] +
		p->z*xform->A[2][1] +
		xform->b[1];

	    z = p->x*xform->A[0][2] + 
		p->y*xform->A[1][2] +
		p->z*xform->A[2][2] +
		xform->b[2];

	/*
	 * This matrix transforms the point in (XYZ) space into a point
	 * in index space.
	 */
	    pt.x = (x*mA[0] + y*mA[3] + z*mA[6] + mb[0]);
	    pt.y = (x*mA[1] + y*mA[4] + z*mA[7] + mb[1]);
	    pt.z = (x*mA[2] + y*mA[5] + z*mA[8] + mb[2]);
	}
	else
	{
	/*
	 * This matrix transforms the point in (XYZ) space into a point
	 * in index space.
	 */
	    pt.x = (p->x*mA[0] + p->y*mA[3] + p->z*mA[6] + mb[0]);
	    pt.y = (p->x*mA[1] + p->y*mA[4] + p->z*mA[7] + mb[1]);
	    pt.z = (p->x*mA[2] + p->y*mA[5] + p->z*mA[8] + mb[2]);
	}

	pt.x -= mo[0];
	pt.y -= mo[1];
	pt.z -= mo[2];

	if (fuzzFlag == FUZZ_ON)
	{
	    if ((pt.x < -fuzz       || 
		 pt.x > ixMax+fuzz) ||
		(pt.y < -fuzz       || 
		 pt.y > iyMax+fuzz) ||
		(pt.z < -fuzz       || 
		 pt.z > izMax+fuzz)) break;
	}
	else
	{
	    if ((pt.x < 0      ||
		 pt.x > ixMax) ||
		(pt.y < 0      ||
		 pt.y > iyMax) ||
		(pt.z < 0      ||
		 pt.z > izMax)) break;
	}

	if (((FieldInterpolator)ci)->cstData)
	{
	    memcpy(v, ((FieldInterpolator)ci)->cstData, itemSize);
	    v = (Pointer)(((char *)v) + itemSize);
	}
	else if (dep == DATA_POSITIONS_DEPENDENT)
	{
            Type dataType;

	    ix = pt.x;
	    dx = pt.x - ix;

	    if (fuzzFlag != FUZZ_ON && (ix>ixMax || (ix==ixMax && dx>0.0) || ix<0))
		goto not_found;

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

	    iy = pt.y;
	    dy = pt.y - iy;

	    if (fuzzFlag != FUZZ_ON && (iy>iyMax || (iy==iyMax && dy>0.0) || iy<0))
		goto not_found;

	    if (iy >= iyMax)
	    {
		iy = iyMax - 1;
		dy = 1.0;
	    }
	    else if (iy < 0 || dy < 0.0)
	    {
		iy = 0;
		dy = 0.0;
	    }

	    iz = pt.z;
	    dz = pt.z - iz;

	    if (fuzzFlag != FUZZ_ON && (iz>izMax || (iz==izMax && dz>0.0) || iz<0))
		goto not_found;

	    if (iz >= izMax)
	    {
		iz = izMax - 1;
		dz = 1.0;
	    }
	    else if (iz < 0 || dz < 0.0)
	    {
		iz = 0;
		dz = 0.0;
	    }

	    if (icH)
	    {
		baseIndex = ix*esz0 + iy*esz1 + iz;
		if (DXIsElementInvalid(icH, baseIndex))
		    goto not_found;
	    }

	    baseIndex = (ix*sz0 + iy*sz1 + iz*sz2);

	    A = dx * dy;
	    B = dx * dz;
	    C = dy * dz;
	    D = dz * A;

	    weight111 = D;
	    weight011 = C - D;
	    weight101 = B - D;
	    weight001 = dz - B - C + D;
	    weight110 = A - D;
	    weight010 = dy - C - A + D;
	    weight100 = dx - B - A + D;
	    weight000 = 1.0 - dz - dy - dx + A + B + C - D;
	
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
	    ix = pt.x;
	    if (ix > ixMax || ix < 0)
		goto not_found;

	    if (ix >= ixMax)
		ix = ixMax - 1;
	    else if (ix < 0)
		ix = 0;

	    iy = pt.y;
	    if (iy > iyMax || iy < 0)
		goto not_found;

	    if (iy >= iyMax)
		iy = iyMax - 1;
	    else if (iy < 0)
		iy = 0;

	    iz = pt.z;
	    if (iz > izMax || iz < 0)
		goto not_found;

	    if (iz >= izMax)
		iz = izMax - 1;
	    else if (iz < 0)
		iz = 0;

	    if (icH)
	    {
		baseIndex = ix*esz0 + iy*esz1 + iz;
		if (DXIsElementInvalid(icH, baseIndex))
		    goto not_found;
	    }

	    baseIndex = (ix*sz0 + iy*sz1 + iz*sz2);

	    memcpy(v, DXGetArrayEntry(ci->data,
					baseIndex, (Pointer)dbuf), itemSize);
	    v = (Pointer)((char *)v + itemSize);
	}

	/*
	 * Only use fuzz on first primitive
	 */
	fuzzFlag = FUZZ_OFF;

	p ++;
	(*n)--;
    }

not_found:
    
    DXFree((Pointer)dbuf);

    *points = (float *)p;
    *values = (Pointer)v;

    return OK;
}

Object
_dxfCubesRRInterpolator_Copy(CubesRRInterpolator old, enum _dxd_copy copy)
{
    CubesRRInterpolator new;

    new = (CubesRRInterpolator)
	    _dxf_NewObject((struct object_class *)&_dxdcubesrrinterpolator_class);
    
    if (!(_dxf_CopyCubesRRInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

CubesRRInterpolator
_dxf_CopyCubesRRInterpolator(CubesRRInterpolator new, 
				CubesRRInterpolator old, enum _dxd_copy copy)
{

    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nElements = old->nElements;

    memcpy((char *)new->size,        (char *)old->size,        sizeof(old->size));
    memcpy((char *)new->counts,      (char *)old->counts,      sizeof(old->counts));
    memcpy((char *)new->eltStrides,  (char *)old->eltStrides,  sizeof(old->eltStrides));
    memcpy((char *)new->meshOffsets, (char *)old->meshOffsets, sizeof(old->meshOffsets));

    new->fuzz = old->fuzz;

    if (new->fieldInterpolator.initialized)
    {
	new->pointsArray = (Array)DXReference((Object)old->pointsArray);
	new->dataArray   = (Array)DXReference((Object)old->dataArray);
	new->data        = DXCreateArrayHandle(old->dataArray);
    }
    else
    {
	new->pointsArray = NULL;
	new->dataArray   = NULL;

	new->data  = NULL;
    }

    if (DXGetError())
	return NULL;

    return new;
}

Interpolator
_dxfCubesRRInterpolator_LocalizeInterpolator(CubesRRInterpolator ci)
{
    ci->fieldInterpolator.localized = 1;
    return (Interpolator)ci;
}
