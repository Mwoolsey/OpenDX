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
#include "quadsRR2DClass.h"

static void  _dxfCleanup(QuadsRR2DInterpolator);
static Error _dxfInitializeTask(Pointer);
static Error _dxfInitialize(QuadsRR2DInterpolator qi);

#define DIAGNOSTIC(str) \
	DXMessage("quads interpolator: %s\n", (str));

int
_dxfRecognizeQuadsRR2D(Field field)
{
    Array 	array;
    int	  	nDim;
    int	  	status;
    Type	type;
    Category	cat;
    int		rank, shape[32];

    CHECK(field, CLASS_FIELD);

    ELT_TYPECHECK(field, "quads");

    status = OK;

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
	return 0;
    
    if (!DXQueryGridPositions(array, &nDim, NULL, NULL, NULL))
	return 0;

    if (nDim != 2)
	return 0;

    if (! DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape))
	return 0;

    if (cat != CATEGORY_REAL)
	return 0;

    if (rank != 1 || shape[0] != 2)
	return 0;

    array = (Array)DXGetComponentValue(field, "connections");
    if (!array)
	return 0;
    
    if (!DXQueryGridConnections(array, &nDim, NULL))
	return 0;
    
    if (nDim != 2)
	return 0;

    return status;
}

QuadsRR2DInterpolator 
_dxfNewQuadsRR2DInterpolator(Field field, 
		enum interp_init initType, double fuzz, Matrix *m)
{
    return (QuadsRR2DInterpolator)_dxf_NewQuadsRR2DInterpolator(field, 
			initType, fuzz, m, &_dxdquadsrr2dinterpolator_class);
}


QuadsRR2DInterpolator 
_dxf_NewQuadsRR2DInterpolator(Field field, 
		enum interp_init initType, float fuzz, Matrix *m,
		struct quadsrr2dinterpolator_class *class)
{
    QuadsRR2DInterpolator qi;

    qi = (QuadsRR2DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
			(struct fieldinterpolator_class *)class);
    if (! qi)
	return NULL;

    qi->pointsArray = NULL;
    qi->dataArray   = NULL;

    /*
     * The grid should be regular
     */
    {
	float localOrigin[2];
	Array connections, positions;
	float deltas[4], d;

	positions = (Array)DXGetComponentValue(field, "positions");
	connections = (Array)DXGetComponentValue(field, "connections");

	if (!positions || !connections)
	{
	    DXDelete((Object)(qi));
	    return NULL;
	}

	DXQueryGridPositions(positions, NULL, NULL, localOrigin, deltas);

	d = 1.0 / (deltas[0]*deltas[3] - deltas[1]*deltas[2]);

	((float *)((Interpolator)qi)->matrix.A)[0] =  deltas[3] * d;
	((float *)((Interpolator)qi)->matrix.A)[1] = -deltas[1] * d;
	((float *)((Interpolator)qi)->matrix.A)[2] = -deltas[2] * d;
	((float *)((Interpolator)qi)->matrix.A)[3] =  deltas[0] * d;

	DXGetMeshOffsets((MeshArray)connections, qi->meshOffsets);

	((Interpolator)qi)->matrix.b[0] = localOrigin[0]
			- (qi->meshOffsets[0]*deltas[0] + qi->meshOffsets[1]*deltas[1]);
	((Interpolator)qi)->matrix.b[1] = localOrigin[1]
			- (qi->meshOffsets[0]*deltas[2] + qi->meshOffsets[1]*deltas[3]);
    }

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
    return _dxfInitialize(*(QuadsRR2DInterpolator *)p);
}


static Error
_dxfInitialize(QuadsRR2DInterpolator qi)
{
    Field		field;
    int			nDim;
    int			i;
    float		deltas[4];
    Type		dataType;
    Category		dataCategory;

    field = (Field)((Interpolator)qi)->dataObject;

    qi->fieldInterpolator.initialized = 1;

    /*
     * De-reference data
     */
    qi->dataArray   = (Array)DXGetComponentValue(field, "data");
    if (!qi->dataArray) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return ERROR;
    }
    DXReference((Object)qi->dataArray);

    DXGetArrayInfo(qi->dataArray, NULL, &((Interpolator)qi)->type,
	&((Interpolator)qi)->category, &((Interpolator)qi)->rank, 
	((Interpolator)qi)->shape);

    /*
     * Get the grid. 
     */
    qi->pointsArray = (Array)DXGetComponentValue(field, "positions");
    if (!qi->pointsArray)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return ERROR;
    }
    DXReference((Object)qi->pointsArray);

    /*
     * get info about data values
     */
    DXGetArrayInfo(qi->dataArray, NULL, &dataType, &dataCategory, NULL, NULL);

    /*
     * Don't worry about maintaining shape of input; just determine how 
     * many values to interpolate.
     */
    qi->nElements = DXGetItemSize(qi->dataArray) / 
				DXTypeSize(((Interpolator)qi)->type);

    /*
     * The grid should be regular
     */
    DXQueryGridPositions(qi->pointsArray, &nDim, qi->counts, NULL, deltas);

    /*
     * Figure out how big to increment pointers along each dimension
     * Use either number of positions or number of primitive elements along
     * each axis depending on data dependency.
     */
    if (qi->fieldInterpolator.data_dependency == DATA_POSITIONS_DEPENDENT)
    {
	qi->size[1] = 1;
	for (i = 0; i >= 0; i--)
	    qi->size[i] = qi->counts[i+1] * qi->size[i+1];
    }
    else
    {
	qi->size[1] = 1;
	for (i = 0; i >= 0; i--)
	    qi->size[i] = (qi->counts[i+1] - 1) * qi->size[i+1];
    }

    qi->dHandle = DXCreateArrayHandle(qi->dataArray);
    if (! qi->dHandle)
	return ERROR;

    return OK;
}

Error
_dxfQuadsRR2DInterpolator_Delete(QuadsRR2DInterpolator qi)
{
    _dxfCleanup(qi);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) qi);
}

int
_dxfQuadsRR2DInterpolator_PrimitiveInterpolate(QuadsRR2DInterpolator qi,
			int *n, float **points, Pointer *values, int fuzzFlag)
{
    float	    x, y;
    int 	    ix, iy;
    float	    dx = 0, dy = 0;
    int		    ixMax;
    int		    iyMax;
    int		    sz0, sz1;
    float    	    weight00, weight01, weight10, weight11;
    float	    dxdy;
    int		    baseIndex, i;
    float	    *p;
    Pointer	    v;
    float	    *A, *b;
    float	    fuzz;
    int		    dep;
    int		    itemSize;
    int		    typeSize;
    int 	    *mo;
    float	    ptx, pty;
    InvalidComponentHandle icH = ((FieldInterpolator)qi)->invCon;
    char	    *dbuf = NULL;
    Matrix	    *xform;

    if (! qi->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(qi))
	{
	    _dxfCleanup(qi);
	    return 0;
	}

	qi->fieldInterpolator.initialized = 1;
    }

    /*
     * De-reference indexing info from interpolator
     */
    A  = (float *)((Interpolator)qi)->matrix.A;
    b  = (float *)((Interpolator)qi)->matrix.b;
    mo = qi->meshOffsets;

    sz0 = qi->size[0];
    sz1 = qi->size[1];

    dep = qi->fieldInterpolator.data_dependency;
    typeSize = DXTypeSize(((Interpolator)qi)->type);
    itemSize = qi->nElements * typeSize;

    fuzz = qi->fuzz;

    ixMax = qi->counts[0] - 1;
    iyMax = qi->counts[1] - 1;

    /*
     * De-reference bounding box
     */
    /* xMax = ((Interpolator)qi)->max[0] + fuzz; */
    /* xMin = ((Interpolator)qi)->min[0] - fuzz; */
    /* yMax = ((Interpolator)qi)->max[1] + fuzz; */
    /* yMin = ((Interpolator)qi)->min[1] - fuzz; */

    if (((FieldInterpolator)qi)->xflag)
	xform = &(((FieldInterpolator)qi)->xform);
    else
	xform = NULL;


    p = *points;
    v = *values;

    dbuf = (char *)DXAllocate(4*itemSize);
    if (! dbuf)
	return ERROR;

    while (*n > 0)
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


	ptx = pPtr[0] - b[0];
	pty = pPtr[1] - b[1];

	x = ptx*A[0] + pty*A[2] - mo[0];
	y = ptx*A[1] + pty*A[3] - mo[1];

	if (fuzzFlag == FUZZ_ON)
	{
	    if ((x < fuzz        || 
		 x > ixMax+fuzz) ||
		(y < -fuzz       || 
		 y > iyMax+fuzz)) break;
	}
	else
	{
	    if ((x < 0      || 
		 x > ixMax) ||
		(y < 0      || 
		 y > iyMax)) break;
	}

	/*
	 * Note: assuming we passed the above test, we really will
	 * be interpolating.
	 */

#define INTERPOLATE(type, round)				\
{								\
    type *v00, *v10, *v01, *v11, *r;				\
								\
    baseIndex = ix*sz0 + iy*sz1;				\
								\
    v00 = (type *)DXGetArrayEntry(qi->dHandle, 			\
			baseIndex, (Pointer)dbuf);		\
								\
    v10 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		baseIndex+sz0, (Pointer)(dbuf+itemSize));	\
								\
    v01 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		baseIndex+sz1, (Pointer)(dbuf+2*itemSize));	\
								\
    v11 = (type *)DXGetArrayEntry(qi->dHandle, 			\
		baseIndex+sz1+sz0, (Pointer)(dbuf+3*itemSize));	\
								\
    r = (type *)v;						\
								\
    for (i = 0; i < qi->nElements; i++)				\
	*r++ = weight00*(*v00++) + weight01*(*v01++) +		\
	       weight10*(*v10++) + weight11*(*v11++) +		\
	       round;						\
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

	    ix = x;
	    dx = x - ix;
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

	    iy = y;
	    dy = y - iy;
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

	    if (icH)
	    {
		baseIndex = ix*(qi->counts[1] - 1) + iy;
		if (DXIsElementInvalid(icH, baseIndex))
		    goto not_found;
	    }

	    dxdy = dx * dy;

	    weight11 = dxdy;
	    weight10 = dx - dxdy;
	    weight01 = dy - dxdy;
	    weight00 = 1 - dx - dy + dxdy;

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
	    ix = x;
	    if (fuzzFlag != FUZZ_ON &&
		(ix>ixMax || (ix==ixMax && dx>0.0) || ix<0))
		goto not_found;

	    if (ix >= ixMax)
		ix = ixMax - 1;
	    else if (ix < 0)
		ix = 0;

	    iy = y;
	    if (fuzzFlag != FUZZ_ON &&
		(iy>iyMax || (iy==iyMax && dy>0.0) || iy<0))
		goto not_found;

	    if (iy >= iyMax)
		iy = iyMax - 1;
	    else if (iy < 0)
		iy = 0;

	    if (icH)
	    {
		baseIndex = ix*(qi->counts[1] - 1) + iy;
		if (DXIsElementInvalid(icH, baseIndex))
		    goto not_found;
	    }

	    baseIndex = ix*sz0 + iy*sz1;

	    memcpy(v, DXGetArrayEntry(qi->dHandle,
		baseIndex, (Pointer)dbuf), itemSize);

	    v = (Pointer)(((char *)v) + itemSize);
	}

	/*
	 * Only use fuzz on first sample
	 */
	fuzzFlag = FUZZ_OFF;
	fuzz = 0.0;

	/*
	 * Only use fuzz on first sample
	 */
	p += 2;
	(*n)--;
    }

not_found:

    *points = (float *)p;
    *values = (Pointer)v;

    if (dbuf)
	DXFree((Pointer)dbuf);

    return OK;
}

static void
_dxfCleanup(QuadsRR2DInterpolator qi)
{								
    if (qi->dHandle)
    {
	DXFreeArrayHandle(qi->dHandle);
	qi->dHandle = NULL;
    }

    if (qi->pointsArray)
    {
	DXDelete((Object)qi->pointsArray);
	qi->pointsArray = NULL;
    }

    if (qi->dataArray)
    {
	DXDelete((Object)qi->dataArray);
	qi->dataArray = NULL;
    }
}

Object
_dxfQuadsRR2DInterpolator_Copy(QuadsRR2DInterpolator old, enum _dxd_copy copy)
{
    QuadsRR2DInterpolator new;

    new = (QuadsRR2DInterpolator)
	    _dxf_NewObject((struct object_class *)&_dxdquadsrr2dinterpolator_class);

    if (!(_dxf_CopyQuadsRR2DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

QuadsRR2DInterpolator
_dxf_CopyQuadsRR2DInterpolator(QuadsRR2DInterpolator new, 
				QuadsRR2DInterpolator old, enum _dxd_copy copy)
{

    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    memcpy((char *)new->size,   (char *)old->size,   2*sizeof(int));
    memcpy((char *)new->counts, (char *)old->counts, 2*sizeof(float));
    memcpy((char *)new->meshOffsets, (char *)old->meshOffsets, sizeof(old->meshOffsets));

    new->fuzz       = old->fuzz;
    new->nElements  = old->nElements;

    if (new->fieldInterpolator.initialized)
    {
	new->pointsArray    = (Array)DXReference((Object)old->pointsArray);
	new->dataArray      = (Array)DXReference((Object)old->dataArray);

	new->dHandle = DXCreateArrayHandle(new->dataArray);
	if (! new->dHandle)
	    return ERROR;
    }
    else
    {
	new->pointsArray    = NULL;
	new->dataArray      = NULL;
	new->dHandle	    = NULL;
    }

    if (DXGetError())
	return NULL;

    return new;
}

Interpolator
_dxfQuadsRR2DInterpolator_LocalizeInterpolator(QuadsRR2DInterpolator qi)
{
    if (qi->fieldInterpolator.localized)
	return (Interpolator)qi;

    qi->fieldInterpolator.localized = 1;

    if (qi->fieldInterpolator.initialized)
	qi->dHandle = DXCreateArrayHandle(qi->dataArray);

    if (DXGetError())
        return NULL;
    else
        return (Interpolator)qi;
}
