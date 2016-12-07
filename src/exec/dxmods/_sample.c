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
#include <stdlib.h>
#include <dx/dx.h>
#include "_sample.h"
#include "measure.h"
#include "vectors.h"

#define TRUE 			1
#define FALSE 			0

#define MAX_V_PER_E		8

static Object  _Sample(Object, int);

Object
Sample(Object o, int n)
{
    Object copy = NULL;
    
    copy = DXCopy(o, COPY_STRUCTURE);
    if (! copy)
	goto error;

    if (! DXInvalidateConnections(copy))
	goto error;

    if (! DXCreateTaskGroup())
	return NULL;

    if (! _Sample(copy, n))
    {
	DXAbortTaskGroup();
	goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
       return NULL;
    
    return copy;

error:
    if (copy != o)
	DXDelete((Object)copy);

    return NULL;
}
    
typedef float			(*PFF)();
typedef Error			(*PFE)();

typedef struct
{
    Field field;
    int   share;
    Array grid;
} SampleArgs;

static Object _Sample_CompositeField(CompositeField, int);
static Object _Sample_MultiGrid(MultiGrid, int);
static Object _Sample_Field(Field, int, Array);
static Field  _Sample_Field_Regular(Field, int, Array);
static Field  _Sample_Field_Irregular(Field, int, Array);
static Error  _Sample_Field_NoConnections(Field, int);

static Error  SpawnVolumeTasks(Object);
static Error  SF_Task(Pointer);
static int    Field_Count(Object, int *);
static int    FieldVolumeTask(Pointer);
static float  MemberVolume(Object);

static Array  _Sample_Grid(Object, int, int);
static Array  _Sample_Grid_Regular(Object, int, int);
static Array  _Sample_Grid_Irregular(Object, int, int);
static Array  _Solve_Grid(int, int, float *, float *, int *, int);

static Error  Cleanup_Sample_Field(Field);
static void   _Check_Sample_Count(Object o, int n);

static float  _dxfFieldVolume(Field);
static float  _dxfBoxVolume(Object, int *);

static PFE    _dxfGetSampleMethod(Field);
static PFF    _dxfGetVolumeMethod(Field);
static Field  CreateOutputReg(Field, Array);

static int   invert33(float *, float *);
static Error invert(int, float *, float *);

static Object
_Sample_Field(Field field, int n, Array grid)
{
    Object		pos, con;
    int			reg;

    if (DXEmptyField(field))
	return (Object)DXEndField(DXNewField());
    
    con = DXGetComponentValue(field, "connections");
    if (! con)
        con = DXGetComponentValue(field, "faces");
    if (! con)
        con = DXGetComponentValue(field, "polylines");
    
    if (! con)
    {
	if (! _Sample_Field_NoConnections(field, n))
	    return NULL;
    }
    else
    {
	pos = DXGetComponentValue(field, "positions");

	reg = DXQueryGridPositions((Array)pos, NULL, NULL, NULL, NULL) &&
			      DXQueryGridConnections((Array)con, NULL, NULL);

	if (reg && ! DXGetComponentValue(field, "invalid connections"))
	{
	    if (! _Sample_Field_Regular(field, n, grid))
		return NULL;
	}
	else
	{
	    if (! _Sample_Field_Irregular(field, n, grid))
		return NULL;
	}

    }

    if (! Cleanup_Sample_Field(field))
	goto error;
    
    return (Object)field;

error:
    return NULL;
}

static Field
_Sample_Field_Regular(Field field, int n, Array grid)
{
    Array array;
    int	  i, j, k, nd, ed;
    int	  oCounts[3],  pCounts[3];
    int   oOffsets[3];
    float gDeltas[9],  pDeltas[9];
    float gOrigin[3],  oOrigin[3];
    float p0[3], p1[3], g0[3], g1[3];
    int   deleteGrid;
    char  *name;

    array = NULL;

    if (DXEmptyField(field))
	return field;

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
	goto error;

    if (! DXQueryGridPositions(array, &nd, pCounts, p0, pDeltas))
	goto irreg;

    ed = 0;
    for (i = 0; i < nd; i++)
	if (pCounts[i] != 1)
	    ed ++;

    if (! grid)
    {
	deleteGrid = 1;
	grid = _Sample_Grid((Object)field, n, ed);
    }
    else
	deleteGrid = 0;

    if (! grid)
	goto error;

    if (! DXQueryGridPositions(grid, &nd, NULL, gOrigin, gDeltas))
	goto irreg;
    
    /*
     * Move the non-trivial deltas to the lower slots
     */
    k = 0;
    for (i = 0; i < nd; i++)
	if (pCounts[i] > 1)
	{
	    for (j = 0; j < nd; j++)
		pDeltas[k*nd+j] = pDeltas[i*nd+j];
	    
	    pCounts[k] = pCounts[i];

	    k++;
	}
    
    for (i = k; i < nd; i++)
    {
	for (j = 0; j < nd; j++)
	    pDeltas[i*nd+j] = 0.0;
	    
	pCounts[k] = 1;
    }
    
    /*
     * Get the corners of the original grid
     */
    for (i = 0; i < nd; i++)
    {
	p1[i] = p0[i];

	for (j = 0; j < nd; j++)
	     p1[i] += (pCounts[j]-1)*pDeltas[j*nd+i];
    }

    /*
     * Invert the delta matrix of the grid to get a mapping from (xyz)
     * space to grid (ijk) space.  Subtract the grid origin from the
     * corners of the original grid and back-translate to get the corners
     * of the original grid in grid (ijk) coordinates.
     */
    switch(nd)
    {
	case 3:
	    {
		float matrix[9];

		if (! invert33(gDeltas, matrix))
		    goto error;
		
		p0[0] -= gOrigin[0];
		p0[1] -= gOrigin[1];
		p0[2] -= gOrigin[2];
		g0[0] = p0[0]*matrix[0]+p0[1]*matrix[3]+p0[2]*matrix[6];
		g0[1] = p0[0]*matrix[1]+p0[1]*matrix[4]+p0[2]*matrix[7];
		g0[2] = p0[0]*matrix[2]+p0[1]*matrix[5]+p0[2]*matrix[8];
		
		p1[0] -= gOrigin[0];
		p1[1] -= gOrigin[1];
		p1[2] -= gOrigin[2];
		g1[0] = p1[0]*matrix[0]+p1[1]*matrix[3]+p1[2]*matrix[6];
		g1[1] = p1[0]*matrix[1]+p1[1]*matrix[4]+p1[2]*matrix[7];
		g1[2] = p1[0]*matrix[2]+p1[1]*matrix[5]+p1[2]*matrix[8];
	    }
	    break;

	case 2:
	    {
		float matrix[4], m;

		m = gDeltas[0]*gDeltas[3] - gDeltas[1]*gDeltas[2];
		if (m == 0)
		{
		    DXSetError(ERROR_DATA_INVALID, "bad delta vectors");
		    goto error;
		}

		m = 1.0 / m;
		matrix[0] =  gDeltas[3]*m;
		matrix[1] = -gDeltas[1]*m;
		matrix[2] = -gDeltas[2]*m;
		matrix[3] =  gDeltas[0]*m;
		
		p0[0] -= gOrigin[0];
		p0[1] -= gOrigin[1];
		g0[0] = p0[0]*matrix[0] + p0[1]*matrix[2];
		g0[1] = p0[0]*matrix[1] + p0[1]*matrix[3];
		
		p1[0] -= gOrigin[0];
		p1[1] -= gOrigin[1];
		g1[0] = p1[0]*matrix[0] + p1[1]*matrix[2];
		g1[1] = p1[0]*matrix[1] + p1[1]*matrix[3];

	    }
	    break;

	case 1:
	    {
		float m;

		m = gDeltas[0];
		if (m == 0)
		{
		    DXSetError(ERROR_DATA_INVALID, "bad delta vectors");
		    goto error;
		}

		m = 1.0 / m;
		
		g0[0] = (p0[0] - gOrigin[0])*m;
		g0[1] = (p0[0] - gOrigin[0])*m;
		
		g1[0] = (p1[0] - gOrigin[0])*m;
		g1[1] = (p1[0] - gOrigin[0])*m;
	    }
	    break;
    }

    for (i = ed; i < nd; i++)
	g0[i] = g1[i] = 0.0;

    for (j = 0; j < nd; j++)
    {
	if (ceil(g0[j]) > floor(g1[j]))
	    goto empty;

	oOffsets[j] = ceil(g0[j]);
	oCounts[j]  = (floor(g1[j]) - oOffsets[j]) + 1;
	if (oCounts[j] == 0)
	    oCounts[j] = 1;
    }

    for (j = 0; j < nd; j++)
    {
	oOrigin[j] = gOrigin[j];

	for (k = 0; k < nd; k++)
	    oOrigin[j] += oOffsets[k]*gDeltas[k*nd + j];
    }

    array = DXMakeGridPositionsV(nd, oCounts, oOrigin, gDeltas);
    if (! array)
	goto error;
    
    if (! CreateOutputReg(field, array))
	goto error;

    if (deleteGrid)
	DXDelete((Object)grid);
	
    return field;
	
empty:

    if (deleteGrid)
	DXDelete((Object)grid);

    while (DXGetEnumeratedComponentValue(field, 0, &name))
	DXDeleteComponent(field, name);

    return field;

irreg:
    return _Sample_Field_Irregular(field, n, grid);

error:

    DXDelete((Object)array);
    return NULL;
}

static Field
_Sample_Field_Irregular(Field field, int nSamples, Array grid)
{
    PFE			worker;
    Array               pos;
    int 		r, s[16];
    Category 		c;
    Type 		t;

    /*
     * Check first whether the input field is empty.
     */

    if (DXEmptyField (field))
	goto error;

    pos = (Array) DXGetComponentValue (field, "positions");
    if (pos == NULL)
    {
	DXSetError (ERROR_MISSING_DATA, "missing positions");
	goto error;
    }

    /*
     * Ascertain whether two dimensional or irregular data.
     * If two dimensional, find the bounding square and generate the
     * corresponding regular grid.
     * Otherwise, leave the work to specialized algorithm (worker routines).
     */

    DXGetArrayInfo (pos, NULL, &t, &c, &r, s);

    if (t != TYPE_FLOAT)
    { 
	DXSetError (ERROR_DATA_INVALID, "Positions must be of type float"); 
	goto error;
    } 

    if (c != CATEGORY_REAL) 
    { 
	DXSetError (ERROR_DATA_INVALID, "Positions must be of category real");
	goto error;
    }

    worker = _dxfGetSampleMethod(field);
    if (! worker)
	goto error;

    if (((* worker) (field, nSamples, grid)) != OK)
	goto error;

    return field;

error:
    return NULL;
}

static Error
_Sample_Field_NoConnections(Field field, int n)
{
    Array old, new = NULL;
    int	  nPositions, i, j;
    InvalidComponentHandle ich = NULL;
    ArrayHandle handle = NULL;
    char *name, *oBuf = NULL;

    if (DXEmptyField(field))
	return OK;

    old = (Array)DXGetComponentValue(field, "positions");
    DXGetArrayInfo(old, &nPositions, NULL, NULL, NULL, NULL);

    ich = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! ich)
	goto error;

    if (DXGetValidCount(ich) == 0)
    {
	char *names[256];
	for (i = 0; !DXGetEnumeratedComponentValue(field, i, names+i); i++);
	for (i = i-1; i >= 0; i--)
	    DXDeleteComponent(field, names[i]);
	goto done;
    }
    else
    {
	int knt;
	float f, fskip;
	char *toDelete[256];
	int nDelete;
	Type t;
	Category c;
	int rank, shape[32], size;

	n *= (float)nPositions / DXGetValidCount(ich);

	fskip = (float)nPositions / n;
	if (fskip <= 1.0)
	    fskip = 1.0;

	for (knt = 0, f = 0.0; f < nPositions; f += fskip)
	    if (DXIsElementValid(ich, (int)f))
		knt ++;

	i = 0; nDelete = 0;
	while ((old = (Array)DXGetEnumeratedComponentValue(field, i++, &name))
									!= NULL)
	{
	    Object attr;

	    if (DXGetComponentAttribute(field, name, "der") ||
		DXGetComponentAttribute(field, name, "ref") ||
		! (attr = DXGetComponentAttribute(field, name, "dep")))
	    {
		toDelete[nDelete++] = name;
		continue;
	    }
	    else
	    {
		char *nPtr, *oPtr;

		if (DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_BAD_CLASS,
			"%s dependency attribute", name);
		    goto error;
		}

		if (strcmp(DXGetString((String)attr), "positions"))
		{
		    toDelete[nDelete++] = name;
		    continue;
		}

		DXGetArrayInfo(old, NULL, &t, &c, &rank, shape);
		size = DXGetItemSize(old);

		new = DXNewArrayV(t, c, rank, shape);
		if (! new)
		    goto error;
		
		if (! DXAddArrayData(new, 0, knt, NULL))
		    goto error;
		
		nPtr = DXGetArrayData(new);

		handle = DXCreateArrayHandle(old);
		if (! handle)
		    goto error;
		
		oBuf = (char *)DXAllocate(size);

		for (f = 0.0, j = 0; f < nPositions; f += fskip, j = (int)f)
		    if (DXIsElementValid(ich, j))
		    {
			oPtr = DXGetArrayEntry(handle, j, oBuf);
			memcpy(nPtr, oPtr, size);
			nPtr += size;
		    }
		
		DXFreeArrayHandle(handle);
		handle = NULL;

		DXFree(oBuf);
		oBuf = NULL;
		
		if (! DXSetComponentValue(field, name, (Object)new))
		    goto error;
	    }
	}

	for (i = 0; i < nDelete; i++)
	    DXDeleteComponent(field, toDelete[i]);
    }

done:
    DXFreeInvalidComponentHandle(ich);

    return OK;

error:
    DXFreeArrayHandle(handle);
    DXFreeInvalidComponentHandle(ich);
    DXFree(oBuf);
    DXDelete((Object)new);
    return ERROR;
}

static Array
_Sample_Grid(Object o, int nSamples, int eltDim)
{
    Field field=NULL;
    Array pA;
    Class class;

    if ((class = DXGetObjectClass(o)) == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);
    
    if (class == CLASS_FIELD)
    {
	field = (Field)o;
    }
    else if (class == CLASS_COMPOSITEFIELD)
    {
	int    i;

	for (i = 0; ;i++)
	{
	    field = (Field)DXGetEnumeratedMember((Group)o, i, NULL);
	    if (! field)
		break;
	    
	    if (DXGetObjectClass((Object)field) != CLASS_FIELD)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"invalid composite field member");
		return NULL;
	    }

	    if (! DXEmptyField(field))
		break;
	}

	if (field == NULL)
	    return NULL;
    }

    pA = (Array)DXGetComponentValue(field, "positions");
    if (! pA)
	return NULL;
	
    if (DXQueryGridPositions(pA, NULL, NULL, NULL, NULL))
	return _Sample_Grid_Regular(o, nSamples, eltDim);
    else
	return _Sample_Grid_Irregular(o, nSamples, eltDim);
}

static Array
_Sample_Grid_Irregular(Object object, int nSamples, int eltDim)
{
    int   i, counts[3];
    Point box[8], min, max;
    float origin[3], deltas[9];

    if (! DXBoundingBox(object, box))
    {
	DXSetError(ERROR_DATA_INVALID, "unable to compute bounding box");
	return NULL;
    }

    min.x = min.y = min.z =  DXD_MAX_FLOAT;
    max.x = max.y = max.z = -DXD_MAX_FLOAT;

    for (i = 0; i < 8; i++)
    {
	if (min.x > box[i].x) min.x = box[i].x;
	if (max.x < box[i].x) max.x = box[i].x;
	if (min.y > box[i].y) min.y = box[i].y;
	if (max.y < box[i].y) max.y = box[i].y;
	if (min.z > box[i].z) min.z = box[i].z;
	if (max.z < box[i].z) max.z = box[i].z;
    }

    origin[0] = min.x; origin[1] = min.y; origin[2] = min.z;

    deltas[0] = max.x-min.x; deltas[1] = 0;           deltas[2] = 0;
    deltas[3] = 0;           deltas[4] = max.y-min.y; deltas[5] = 0;
    deltas[6] = 0;           deltas[7] = 0;           deltas[8] = max.z-min.z;

    counts[0] = counts[1] = counts[2] = 2;

    if (deltas[0] == 0.0) { deltas[0] = 1.0; counts[0] = 1; };
    if (deltas[4] == 0.0) { deltas[4] = 1.0; counts[1] = 1; };
    if (deltas[8] == 0.0) { deltas[8] = 1.0; counts[2] = 1; };

    return _Solve_Grid(3, nSamples, origin, deltas, counts, eltDim);
}

static Array
_Solve_Grid(int nd, int nSamples, 
	float *iOrigin, float *iDeltas, int *iCounts, int eltDim)
{
    int tCounts[3], oCounts[3], i, j, k;
    float A, B, a, l[3], d, oOrigin[3], oDeltas[9], tDeltas[9];

    /*
     * Compress the n-D grid so the first k are non-trivial
     */
    k = 0;
    for (i = 0; i < nd; i++)
	if (iCounts[i] > 1)
	{
	    for (j = 0; j < nd; j++)
		tDeltas[k*nd+j] = iDeltas[i*nd+j];
	    
	    tCounts[k] = iCounts[i];

	    k++;
	}

    if (k == 0)
    {
	DXSetError(ERROR_DATA_INVALID, "degenerate grid");
	goto error;
    }
    
    /*
     * Remaining dimensions are trivial.
     */
    for (i = k; i < nd; i++)
    {
	for (j = 0; j < nd; j++)
	    tDeltas[i*nd+j] = 0.0;
	    
	tCounts[i] = 1;
    }
    
    /*
     * Get edge lengths of grid
     */
    for (i = 0; i < k; i++)
    {
	l[i] = 0.0;
	for (j = 0; j < nd; j++)
	{
	    d = tDeltas[i*nd + j];
	    l[i] += d * d;
	}

	l[i] = sqrt(l[i]) * (tCounts[i] - 1);
    }

    if (eltDim == -1)
	eltDim = k;
    
    switch(k)
    {
	case 1:
	    oCounts[0] = nSamples;
	    oCounts[1] = 1;
	    oCounts[2] = 1;
	    break;
	
	case 2:
	    switch(eltDim)
	    {
		case 1:
		    oCounts[0] = nSamples * (l[0] / (l[1] + l[1]));
		    oCounts[1] = nSamples * (l[1] / (l[1] + l[1]));
		    oCounts[2] = 1;
		    break;
		
		case 2:
		    A = l[1] / l[0];
		    a = sqrt(nSamples / A);
		    oCounts[0] = (int)(a + 0.5);
		    if (oCounts[0] == 0)
			oCounts[0] = 1;
		    oCounts[1] = (int)((nSamples / oCounts[0]) + 0.5);
		    oCounts[2] = 1;
		    break;

		default:
		    DXSetError(ERROR_INTERNAL, 
			"%d-D elements embedded in 2-D space", eltDim);
		    goto error;
	    }
	    break;
	
	case 3:
	    switch(eltDim)
	    {
		case 1:
		    oCounts[0] = nSamples * (l[0] / (l[1] + l[1] + l[2]));
		    oCounts[1] = nSamples * (l[1] / (l[1] + l[1] + l[2]));
		    oCounts[2] = nSamples * (l[2] / (l[1] + l[1] + l[2]));
		    break;
		
		case 2:
		    A = l[1] / l[0];
		    B = l[2] / l[0];
		    a = sqrt(3*nSamples / (A + B + A*B));
		    oCounts[0] = (int)(a + 0.5);
		    oCounts[1] = (int)((a*A) + 0.5);
		    oCounts[2] = (int)((a*B) + 0.5);
		    break;

		case 3:
		    A = l[0] / l[2];
		    B = l[1] / l[2];
		    a = pow(nSamples / (A*B), 0.333333333);
		    oCounts[2] = (int)(a + 0.5);
		    if (oCounts[2] < 1) 
			oCounts[2] = 1;
		    nSamples = ((float)nSamples / (float)oCounts[2]) + 0.5;
		    A = l[1] / l[0];
		    a = sqrt(nSamples / A);
		    oCounts[0] = (int)(a + 0.5);
		    if (oCounts[0] == 0)
			oCounts[0] = 1;
		    oCounts[1] = (int)((nSamples / oCounts[0]) + 0.5);
		    break;

		default:
		    DXSetError(ERROR_INTERNAL, 
			"%d-D elements embedded in 2-D space", eltDim);
		    goto error;
	    }
	    break;
    }

    for (i = 0; i < k; i++)
	if (oCounts[i] < 1) oCounts[i] = 1;

    for (i = 0; i < nd; i++)
    {
	if (oCounts[i] > 0)
	{
	    d = ((float)tCounts[i] - 1) / oCounts[i];

	    for (j = 0; j < nd; j++)
		oDeltas[i*nd + j] = d * tDeltas[i*nd + j];
	    
	    k++;
	}
	else
	    for (j = 0; j < nd; j++)
		oDeltas[i*nd + j] = 0.0;

    }

    for (i = 0; i < nd; i++)
    {
	oOrigin[i] = iOrigin[i];

	for (j = 0; j < nd; j++)
	    oOrigin[i] += oDeltas[j*nd+i] / 2.0;
    }

    return DXMakeGridPositionsV(nd, oCounts, oOrigin, oDeltas);

error:
    return NULL;
}

#define INTERPOLATE_3D(type, round)					\
{									\
    Vector *p   = (Vector *)pList;					\
    char *sData = (char *)DXGetArrayData(sA);                           \
    type *dPtr  = (type *)DXGetArrayData(dA);                           \
    int s;								\
									\
    for (s = 0; s < nSamples; s++, p++)					\
    {									\
	float x, y, z, w[8], A, B, C, D;				\
	int   ix, iy, iz, base, j, k;					\
	type *sPtr;							\
									\
	if (p->x == oCounts[0]-1) {ix = oCounts[0]-2; x = 1.0;}		\
	else                      {ix = (int)p->x; x = p->x - ix;}	\
	if (p->y == oCounts[1]-1) {iy = oCounts[1]-2; y = 1.0;}		\
	else                      {iy = (int)p->y; y = p->y - iy;}	\
	if (p->z == oCounts[2]-1) {iz = oCounts[2]-2; z = 1.0;}		\
	else                      {iz = (int)p->z; z = p->z - iz;}	\
									\
	A = x * y; B = x * z; C = y * z; D = A * z;			\
									\
	w[7] = D;							\
	w[6] = C - D;							\
	w[5] = B - D;							\
	w[4] = z - B - C + D;						\
	w[3] = A - D;							\
	w[2] = y - C - A + D;						\
	w[1] = x - B - A + D;						\
	w[0] = 1.0 - z - y - x + A + B + C - D;				\
									\
	base = ix*skip[0] + iy*skip[1] + iz*skip[2];			\
									\
	sPtr = (type *)(sData + base*itemSize);				\
									\
	for (j = 0; j < itemValues; j++)				\
	    fBuf[j] = w[0] * sPtr[j];					\
									\
	for (j = 1; j < 8; j++)						\
	{								\
	    int v = base + (j & 0x1 ? skip[0] : 0) 			\
			 + (j & 0x2 ? skip[1] : 0) 			\
			 + (j & 0x4 ? skip[2] : 0);			\
	    type *sPtr = (type *)(sData + v*itemSize);			\
	    for (k = 0; k < itemValues; k++)				\
		fBuf[k] += w[j] * sPtr[k];				\
	}								\
									\
	for (j = 0; j < itemValues; j++)				\
	    dPtr[j] = fBuf[j] + round;					\
									\
	dPtr += itemValues;						\
    }									\
}

#define INTERPOLATE_2D(type, round)					\
{									\
    float *p    = (float *)pList;					\
    char *sData = (char *)DXGetArrayData(sA);                           \
    type *dPtr  = (type *)DXGetArrayData(dA);                           \
    int s;								\
									\
    for (s = 0; s < nSamples; s++, p += 2)				\
    {									\
	float x, y, w[4];						\
	int   ix, iy, base, j, k;					\
	type *sPtr;							\
									\
	if (p[0] == oCounts[0]-1) {ix = oCounts[0]-2; x = 1.0;}		\
	else                      {ix = (int)p[0]; x = p[0] - ix;}	\
	if (p[1] == oCounts[1]-1) {iy = oCounts[1]-2; y = 1.0;}		\
	else                      {iy = (int)p[1]; y = p[1] - iy;}	\
									\
	w[0] = (1 - x)*(1 - y);						\
	w[1] = x*(1 - y);						\
	w[2] = (1 - x)*y;						\
	w[3] = x*y;							\
									\
	base = ix*skip[0] + iy*skip[1];					\
									\
	sPtr = (type *)(sData + base*itemSize);				\
									\
	for (j = 0; j < itemValues; j++)				\
	    fBuf[j] = w[0] * sPtr[j];					\
									\
	for (j = 1; j < 4; j++)						\
	{								\
	    int v = base + (j & 0x1 ? skip[0] : 0) 			\
			 + (j & 0x2 ? skip[1] : 0);			\
	    type *sPtr = (type *)(sData + v*itemSize);			\
	    for (k = 0; k < itemValues; k++)				\
		fBuf[k] += w[j] * sPtr[k];				\
	}								\
									\
	for (j = 0; j < itemValues; j++)				\
	    dPtr[j] = fBuf[j] + round;					\
									\
	dPtr += itemValues;						\
    }									\
}

#define INTERPOLATE_1D(type, round)					\
{									\
    float *p    = (float *)pList;					\
    char *sData = (char *)DXGetArrayData(sA);                           \
    type *dPtr  = (type *)DXGetArrayData(dA);                           \
    int s;								\
									\
    for (s = 0; s < nSamples; s++, p ++)				\
    {									\
	float x;							\
	int   ix, j;							\
	type  *src0, *src1;						\
									\
	if (*p == oCounts[0]-1) {ix = oCounts[0]-2; x = 1.0;}		\
	else                    {ix = (int)*p; x = p[0] - ix;}		\
									\
	src0 = (type *)sData + (ix * itemSize);				\
	src1 = src0 + itemSize;						\
									\
	for (j = 0; j < itemValues; j++)				\
	    dPtr[j] = (x*src1[j] + (1 - x)*src0[j]) + round;		\
									\
	dPtr += itemValues;						\
    }									\
}

static Error InvertRegular(int, float *, float *, float *, float *);
static void MatMult(int,  float *, float *, float *,
					float *, float *, float *);
static Field
CreateOutputReg(Field f, Array np)
{
    Array   op;
    int     size[3], skip[3], oCounts[3], nCounts[3], nd, ndInterp, i, j, k;
    float   oOrigin[3], oDeltas[9];
    float   nOrigin[3], nDeltas[9];
    float   mOrigin[3], mDeltas[9];
    float   oOriginInv[3], oDeltasInv[9];
    int     *cList=NULL, *cc;
    float   *pList=NULL, *pp;
    int     indices[3];
    int     nSamples, nDelete=0;
    Array   sA, dA = NULL;
    char    *name, *toDelete[100];
    float   *fBuf = NULL;
    InvalidComponentHandle inInvalids = NULL, outInvalids = NULL;

    indices[0] = 0; indices[1] = 0; indices[2] = 0;

    op = (Array)DXGetComponentValue(f, "positions");
    if (! op)
	goto error;
    
    if (! DXQueryGridPositions(op, &nd, oCounts, oOrigin, oDeltas))
        goto error;

    for (i = j = 0; i < nd; i++)
    {
	if (oCounts[i] != 1)
	{
	    if (i != j)
	    {
		oCounts[j] = oCounts[i];
		for (k = 0; k < nd; k++)
		    oDeltas[(j*nd)+k] = oDeltas[(i*nd)+k];
	    }
	    j++;
	}
    }

    for (i = j; i < nd; i++)
    {
	oCounts[i] = 1;
	for (k = 0; k < nd; k++)
	    oDeltas[(i*nd)+k] = 0.0;
    }
    
    ndInterp = j;

    if (! DXQueryGridPositions(np, NULL, nCounts, nOrigin, nDeltas))
        goto error;

    if (! InvertRegular(nd, oOrigin, oDeltas, oOriginInv, oDeltasInv))
	goto error;

    MatMult(nd, nOrigin, nDeltas, oOriginInv, oDeltasInv, mOrigin, mDeltas);

    DXGetArrayInfo(np, &nSamples, NULL, NULL, NULL, NULL);

    cList = (int *)DXAllocate(nSamples*sizeof(int));
    pList = (float *)DXAllocate(nSamples*ndInterp*sizeof(float));
    if (! pList || ! cList)
	goto error;

    size[nd-1] = 1;
    skip[nd-1] = 1;
    for (i = nd - 2; i >= 0; i --)
    {
	size[i] = (oCounts[i+1] == 1 ? 1 : (oCounts[i+1] - 1)) * size[i+1];
	skip[i] = (i == nd - 2) ? oCounts[i+1] : skip[i+1] * oCounts[i+1];
    }

    /*
     * For each output sample, get the index of the connection it
     * lies in (the c array) and the point in the original position 
     * index space that the sample corresponds to.
     */
    pp = pList;
    cc = cList;
    for (i = 0; i < nSamples; i++)
    {
	*cc = 0;
	for (j = 0; j < ndInterp; j++)
	{
	    pp[j] = mOrigin[j];
	    for (k = 0; k < nd; k++)
		pp[j] += indices[k]*mDeltas[(k*nd) + j];

	    *cc += ((int)pp[j])*size[j];
	}

	cc++;
	pp += ndInterp;

	if (i != nSamples-1)
	{
	    indices[nd-1]++;
	    for (j = nd-1; j >= 0; j--)
		if (indices[j] >= nCounts[j])
		{
		    indices[j] = 0;
		    indices[j-1]++;
		}
		else
		    break;
	}
    }

    if (DXGetComponentValue(f, "invalid positions") ||
	DXGetComponentValue(f, "invalid connections"))
    {
	inInvalids  = DXCreateInvalidComponentHandle((Object)f, 
						NULL, "connections");
	outInvalids = DXCreateInvalidComponentHandle((Object)np,
						NULL, "positions");
	
	if (! inInvalids || ! outInvalids)
	    goto error;
					
	for (i = 0; i < nSamples; i++)
	    if (DXIsElementInvalid(inInvalids, cList[i]))
		DXSetElementInvalid(outInvalids, i);
	
	if (! DXSaveInvalidComponent(f, outInvalids))
	    goto error;
	
	DXFreeInvalidComponentHandle(inInvalids);  inInvalids = NULL;
	DXFreeInvalidComponentHandle(outInvalids); outInvalids = NULL;
    }

    DXDeleteComponent(f, "connections");

    i = 0;
    while(NULL != (sA = (Array)DXGetEnumeratedComponentValue(f, i++, &name)))
    {
	Object at;
	Type t; 
	Category c;
	int r, shape[32];
	int  itemSize, itemValues;

	if (!strcmp(name, "positions") || !strcmp(name, "invalid positions"))
	    continue;
	    
	if (DXGetComponentAttribute(f, name, "ref") 		   ||
	    DXGetComponentAttribute(f, name, "der") 		   || 
	    NULL == (at = DXGetComponentAttribute(f, name, "dep")) ||
	    !strcmp(name, "invalid connections"))
	{
	    toDelete[nDelete++] = name;
	    continue;
	}

	if (DXGetObjectClass(at) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
	    goto error;
	}

	DXGetArrayInfo(sA, NULL, &t, &c, &r, shape);
	
	itemSize   = DXGetItemSize(sA);
	itemValues = itemSize / DXTypeSize(t);

	if (DXQueryConstantArray(sA, NULL, NULL))
	{
	    dA = (Array)DXNewConstantArrayV(nSamples, DXGetConstantArrayData(sA),
					t, c, r, shape);
	    if (! dA)
		goto error;
	}
	else
	{
	    dA = DXNewArrayV(t, c, r, shape);
	    if (! dA)
		goto error;
	    
	    if (! DXAddArrayData(dA, 0, nSamples, NULL))
		goto error;

	    if (! strcmp(DXGetString((String)at), "positions"))
	    {
		int nItems = DXGetItemSize(dA) / DXTypeSize(t);

		fBuf = (float *)DXAllocate(nItems*sizeof(float));
		if (! fBuf)
		    goto error;

		switch(ndInterp)
		{
		    case 3:
			switch(t) 
			{
			  case TYPE_FLOAT:  INTERPOLATE_3D(float,  0.0); break;
			  case TYPE_DOUBLE: INTERPOLATE_3D(double, 0.0); break;
			  case TYPE_SHORT:  INTERPOLATE_3D(short,  0.5); break;
			  case TYPE_USHORT: INTERPOLATE_3D(ushort, 0.5); break;
			  case TYPE_INT:    INTERPOLATE_3D(int,    0.5); break;
			  case TYPE_UINT:   INTERPOLATE_3D(uint,   0.5); break;
			  case TYPE_UBYTE:  INTERPOLATE_3D(ubyte,  0.5); break;
			  case TYPE_BYTE:   INTERPOLATE_3D(byte,   0.5); break;
			  default: break;
			}
			break;
		    case 2:
			switch(t) 
			{
			  case TYPE_FLOAT:  INTERPOLATE_2D(float,  0.0); break;
			  case TYPE_DOUBLE: INTERPOLATE_2D(double, 0.0); break;
			  case TYPE_SHORT:  INTERPOLATE_2D(short,  0.5); break;
			  case TYPE_USHORT: INTERPOLATE_2D(ushort, 0.5); break;
			  case TYPE_INT:    INTERPOLATE_2D(int,    0.5); break;
			  case TYPE_UINT:   INTERPOLATE_2D(uint,   0.5); break;
			  case TYPE_UBYTE:  INTERPOLATE_2D(ubyte,  0.5); break;
			  case TYPE_BYTE:   INTERPOLATE_2D(byte,   0.5); break;
			  default: break;
			}
			break;
		    case 1:
			switch(t) 
			{
			  case TYPE_FLOAT:  INTERPOLATE_1D(float,  0.0); break;
			  case TYPE_DOUBLE: INTERPOLATE_1D(double, 0.0); break;
			  case TYPE_SHORT:  INTERPOLATE_1D(short,  0.5); break;
			  case TYPE_USHORT: INTERPOLATE_1D(ushort, 0.5); break;
			  case TYPE_INT:    INTERPOLATE_1D(int,    0.5); break;
			  case TYPE_UINT:   INTERPOLATE_1D(uint,   0.5); break;
			  case TYPE_UBYTE:  INTERPOLATE_1D(ubyte,  0.5); break;
			  case TYPE_BYTE:   INTERPOLATE_1D(byte,   0.5); break;
			  default: break;
			}
			break;
		}

		DXFree((Pointer)fBuf);
		fBuf = 0;
	    }
	    else if (! strcmp(DXGetString((String)at), "connections"))
	    {
		char *src, *dst;
		int s;

		src = (char *)DXGetArrayData(sA);
		dst = (char *)DXGetArrayData(dA);

		for (s = 0; s < nSamples; s++)
		{
		    memcpy(dst, src+(cList[s]*itemSize), itemSize);
		    dst += itemSize;
		}
	    }
	    else
	    {
		DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
		goto error;
	    }
	}

	if (! DXSetComponentValue(f, name, (Object)dA))
	    goto error;
	dA = NULL;
	
	if (! DXSetComponentAttribute(f, name, "dep", 
				(Object)DXNewString("positions")))
	    goto error;
    }

    for (i = 0; i < nDelete; i++)
	DXDeleteComponent(f, toDelete[i]);

    if (! DXSetComponentValue(f, "positions", (Object)np))
	goto error;
    
    if (outInvalids)
	DXFreeInvalidComponentHandle(outInvalids);
    if (inInvalids)
	DXFreeInvalidComponentHandle(inInvalids);
    DXFree((Pointer)pList);
    DXFree((Pointer)cList);

    return f;

error:
    if (outInvalids)
	DXFreeInvalidComponentHandle(outInvalids);
    if (inInvalids)
	DXFreeInvalidComponentHandle(inInvalids);
    DXFree((Pointer)dA);
    DXFree((Pointer)fBuf);
    DXFree((Pointer)pList);
    DXFree((Pointer)cList);

    return NULL;
}

static Error
InvertRegular(int nd, float *sO, float *sD, float *dO, float *dD)
{
    float m;

    switch(nd)
    {
	case 1:
	    if (sD[0] == 0.0)
		goto error;
	    dD[0] = 1.0 / sD[0];
	    dO[0] = -sO[0]*dD[0];
	    break;

	case 2:

	    m = sD[0]*sD[3] - sD[1]*sD[2];
	    if (m == 0)
		goto error;

	    m = 1.0 / m;
	    dD[0] =  sD[3]*m;
	    dD[1] = -sD[1]*m;
	    dD[2] = -sD[2]*m;
	    dD[3] =  sD[0]*m;
	    dO[0] = -(sO[0]*dD[0] + sO[1]*dD[2]);
	    dO[1] = -(sO[0]*dD[1] + sO[1]*dD[3]);
	    break;
	
	case 3:
	    if (! invert33(sD, dD))
		goto error;

	    dO[0] = -(sO[0]*dD[0] + sO[1]*dD[3] + sO[2]*dD[6]);
	    dO[1] = -(sO[0]*dD[1] + sO[1]*dD[4] + sO[2]*dD[7]);
	    dO[2] = -(sO[0]*dD[2] + sO[1]*dD[5] + sO[2]*dD[8]);
	    break;
	
	default:
	    DXSetError(ERROR_NOT_IMPLEMENTED, ">3 dimensions");
	    return ERROR;
    }

    return OK;

error:
    DXSetError(ERROR_DATA_INVALID, "non-invertible data");
    return ERROR;
}

static void
MatMult(int nd, float *o0, float *d0, 
		float *o1, float *d1,
		float *or, float *dr)
{
    int i, j, k;

    for (i = 0; i < nd; i++)
    {
	for (j = 0; j < nd; j++)
	{
	    dr[i*nd+j] = 0.0;
	    for (k = 0; k < nd; k++)
		dr[i*nd+j] += d0[i*nd+k]*d1[k*nd+j];
	}

	or[i] = o1[i];
	for (j = 0; j < nd; j++)
	    or[i] += o0[j]*d1[j*nd+i];
    }
}


static Object
_Sample(Object o, int n)
{
    Class      class;
    Object     c, new;
    int        i;
    SampleArgs args;

    class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);

    switch(class)
    {
	case CLASS_SERIES:
	{
   	    float position;

	    i = 0;
	    while (NULL != (c = DXGetSeriesMember((Series)o, i, &position)))
	    {
		new = _Sample(c, n);
		if (! new)
		    goto error;
		if (new != c)
		    DXSetSeriesMember((Series)o, i, position, (Object)new);
		i++;
	    }
	    
	    break;
	}

	case CLASS_GROUP:
	{
   	    char *name;

	    i = 0;
	    while (NULL != (c = DXGetEnumeratedMember((Group)o, i, &name)))
	    {
		new = _Sample(c, n);
		if (! new)
		    goto error;
		if (new != c) {
		    if (name)
		        DXSetMember((Group)o, name, (Object)new);
		    else
		        DXSetEnumeratedMember((Group)o, i, (Object)new);
		}
		i++;
	    }
	    
	    break;
	}
	
	case CLASS_COMPOSITEFIELD:

	    _Check_Sample_Count(o, n);

	    if (NULL == (o = _Sample_CompositeField((CompositeField)o, n)))
		goto error;

	    break;

	case CLASS_MULTIGRID:

	    _Check_Sample_Count(o, n);

	    if (NULL == (o = _Sample_MultiGrid((MultiGrid)o, n)))
		goto error;

	    break;

	case CLASS_FIELD:

	    _Check_Sample_Count(o, n);

	    args.field = (Field)o;
	    args.share = n;
	    args.grid = NULL;

	    if (! DXAddTask(SF_Task, (Pointer)&args, sizeof(args), 1.0))
		goto error;

	    break;
	
	case CLASS_XFORM:
	{
	    Object new;

	    DXGetXformInfo((Xform)o, &c, NULL);
	    if (NULL == (new = _Sample(c, n)))
		goto error;
	
	    if (new != c)
		 DXSetXformObject((Xform)o, new);

	    break;
	}

	case CLASS_CLIPPED:
	{
	    Object new;

	    DXGetClippedInfo((Clipped)o, &c, NULL);
	    if (NULL == (new = _Sample(c, n)))
		goto error;
	
	    if (new != c)
		 DXSetClippedObjects((Clipped)o, new, NULL);
	    
	    break;
	}
	
	default:
	    break;
    }

    return o;

error:
    return NULL;
}
    
static int  
Field_Count(Object o, int *n)
{
    int i, tot, t;
    Object c;

    switch(DXGetObjectClass(o))
    {
	case CLASS_GROUP:
	    i = 0; tot = 0;
	    while (NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
	    {
		t = Field_Count(c, n);
		if (t < 0)
		    return -1;
		tot += t;
	    }
	    
	    return tot;
	
	case CLASS_FIELD:
	    if (! DXEmptyField((Field)o))
	    {
		char *str;
		Object attr;
		int m;

		if (DXGetComponentValue((Field)o, "connections"))
		{
		    attr = DXGetComponentAttribute((Field)o,
					"connections", "element type");
		    if (! attr)
		    {
			DXSetError(ERROR_MISSING_DATA, "#10255",
					"connections", "element type");
			return -1;
		    }

		    str = DXGetString((String)attr);

		    if      (!strcmp(str, "triangles"))  m = 2;
		    else if (!strcmp(str, "quads"))      m = 2;
		    else if (!strcmp(str, "lines"))      m = 1;
		    else if (!strcmp(str, "cubes"))      m = 3;
		    else if (!strcmp(str, "tetrahedra")) m = 3;
		    else
		    {
			DXSetError(ERROR_DATA_INVALID, "#10410", str);
			return -1;
		    }

		    if (*n == -1)
			*n = m;
		    else if (*n != m)
		    {
			DXSetError(ERROR_DATA_INVALID, "#10420");
			return -1;
		    }
		}
		else
		{
		    if (*n == -1)
			*n = 0;
		    else if (*n > 0)
		    {
			DXSetError(ERROR_DATA_INVALID, "#10420");
			return -1;
		    }
		}
	    }
		    
	    return 1;
	
	case CLASS_XFORM:
	    DXGetXformInfo((Xform)o, &c, NULL);
	    return Field_Count(c, n);

	case CLASS_CLIPPED:
	    DXGetClippedInfo((Clipped)o, &c, NULL);
	    return Field_Count(c, n);
	
	default:
	    return 0;
    }
}

static Error
SpawnVolumeTasks(Object o)
{
    int i;
    Object c;

    switch(DXGetObjectClass(o))
    {
	case CLASS_GROUP:
	    i = 0;
	    while (NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
		if (! SpawnVolumeTasks(c))
		    return ERROR;
	    
	    return OK;
	
	case CLASS_FIELD:
	    if (NULL == DXGetAttribute(o, "volume"))
		if (! DXAddTask(FieldVolumeTask, (Pointer)o, 0, 1.0))
		    return ERROR;
	    return OK;
	
	case CLASS_XFORM:
	    DXGetXformInfo((Xform)o, &c, NULL);
	    return SpawnVolumeTasks(c);

	case CLASS_CLIPPED:
	    DXGetClippedInfo((Clipped)o, &c, NULL);
	    return SpawnVolumeTasks(c);
	
	default:
	    return OK;
    }
}

static float
MemberVolume(Object o)
{
    int i;
    float tot;
    Object c;
    Object attr;

    switch(DXGetObjectClass(o))
    {
	case CLASS_GROUP:
	    i = 0; tot = 0.0;
	    while (NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
		tot += MemberVolume(c);
	    
	    return tot;
	
	case CLASS_FIELD:
	    if (NULL == (attr = DXGetAttribute(o, "volume")))
	    {
		DXSetError(ERROR_INTERNAL, "missing field volume");
		return -1;
	    }
	    return *(float *)DXGetConstantArrayData((Array)attr);
	
	case CLASS_XFORM:
	    DXGetXformInfo((Xform)o, &c, NULL);
	    return MemberVolume(c);

	case CLASS_CLIPPED:
	    DXGetClippedInfo((Clipped)o, &c, NULL);
	    return MemberVolume(c);
	
	default:
	    return 0.0;
    }
}

static Error
SF_Task(Pointer ptr)
{
    SampleArgs *args = (SampleArgs *)ptr;

    if (! _Sample_Field(args->field, args->share, args->grid))
    {
	DXDelete((Object)args->grid);
	return ERROR;
    }
    
    DXDelete((Object)args->grid);
    return OK;
}

static Object
_Sample_CompositeField(CompositeField cf, int n)
{
    float tv, bv, mv, v;
    int   nd, i, nElts;
    SampleArgs  args;
    Array grid=NULL;
    int eltDim = -1;
    Object c;

    nElts = Field_Count((Object)cf, &eltDim);
    if (nElts < 0)
	return NULL;

    if (! DXCreateTaskGroup())
	goto error;
    
    nElts = 0;
    if (! SpawnVolumeTasks((Object)cf))
    {
	DXAbortTaskGroup();
	goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    tv = 0;
    for (i = 0; NULL != (c = DXGetEnumeratedMember((Group)cf, i, NULL)); i++)
    {
	if (DXGetObjectClass(c) != CLASS_FIELD)
	{
	    DXSetError(ERROR_DATA_INVALID,
			"composite members must be fields");
	    goto error;
	}

	v = MemberVolume(c);
	if (v < 0)
	    goto error;
	
	tv += v;
    }

    if (tv == 0.0)
	goto empty;
	
    if (eltDim != 0)
    {
	if (-1 == (bv = _dxfBoxVolume((Object)cf, &nd)))
	    return NULL;

	if (nd == eltDim)
	    n *= bv / tv;

	grid = _Sample_Grid((Object)cf, n, eltDim);
	if (! grid)
	    goto error;
    }
    
    for (i = 0; NULL != (c = DXGetEnumeratedMember((Group)cf, i, NULL)); i++)
    {
	args.grid  = (Array)DXReference((Object)grid);
	mv = MemberVolume(c);
	if (mv > 0.0)
	{
	    args.share = 1 + n*(mv/tv);
	    args.field = (Field)c;
	    if (! DXAddTask(SF_Task, (Pointer)&args, sizeof(args), 1.0))
		goto error;
	}
    }

    return (Object)cf;

empty:
    return (Object)DXEndField(DXNewField());

error:
    return NULL;
}


static Object
_Sample_MultiGrid(MultiGrid mg, int n)
{
    float tv, v;
    int   nd, i, nElts;
    int eltDim = -1;
    Object c;

    if (-1 == _dxfBoxVolume((Object)mg, &nd))
	return NULL;

    nElts = Field_Count((Object)mg, &eltDim);
    if (nElts < 0)
	return NULL;

    if (! DXCreateTaskGroup())
	goto error;
    
    nElts = 0;
    if (! SpawnVolumeTasks((Object)mg))
    {
	DXAbortTaskGroup();
	goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    tv = 0;
    for (i = 0; NULL != (c = DXGetEnumeratedMember((Group)mg, i, NULL)); i++)
    {
	v = MemberVolume(c);
	if (v < 0)
	    goto error;
	
	tv += v;
    }

    if (tv <= 0.0)
    {
	DXSetError(ERROR_INTERNAL, "multigrid volume is <= 0.0");
	goto error;
    }

    for (i = 0; NULL != (c = DXGetEnumeratedMember((Group)mg, i, NULL)); i++)
    {
	if (! _Sample(c, 1+n*MemberVolume(c)/tv))
	    goto error;
    }

    return (Object)mg;

error:
    return NULL;
}


static Error  PolylinesSample(Field, int, Array);
static Error  FacesSample(Field, int, Array);

static float  LineLength(float **, int);
static Error  LineSample(Field, int, Array);

static float  QuadArea(float **, int);
static Error  QuadSample(Field, int, Array);
static int    coords_quad(float *, float **, float *);

static float  TriangleArea(float **, int);
static Error  TriangleSample(Field, int, Array);
static int    coords_triangle(float *, float **, float *);

static float  CubeVolume(float **, int);
static Error  CubeSample(Field, int, Array);
static int    coords_cube(float *, float **, float *);

static float  TetrahedraVolume(float **, int);
static Error  TetrahedraSample(Field, int, Array);
static int    coords_tetrahedra (float *, float **, float *);
static int    _invert33(float *, float *, float *, float *);

static Field  CreateOutputIrreg(Field, SegList *, SegList *);
static Field  CreateOutputPolylines(Field, SegList *, SegList *);
static Field  CreateOutputFaces(Field, SegList *, SegList *);

static Error  PlanarSurface(Field, int, int (*inside)(), int, Array);
static Error  CurvedSurface(Field, int, int (*inside)(), int, Array);
static Error  Volume(Field, int, int (*inside)(), int, Array);

static PFE
_dxfGetSampleMethod(Field field)
{
    Object		obj;
    char		*primitive;

    obj = DXGetComponentAttribute (field, "connections", "element type");
    if (obj == NULL)
    {
	if (DXGetComponentValue(field, "polylines"))
	    primitive = "polylines";
	else if (DXGetComponentValue(field, "faces"))
	    primitive = "faces";
	else
	{
	    DXSetError(ERROR_MISSING_DATA, "no element type attribute");
	    goto error;
	}
    }
    else
    {
	primitive = DXGetString ((String) obj);
	if (primitive == NULL)
	{
	    DXSetError(ERROR_MISSING_DATA,"unable to access element type attribute");
	    goto error;
	}
    }

    if (! strcmp (primitive, "tetrahedra")) 
    {
	return TetrahedraSample;
    }
    else if (! strcmp (primitive, "cubes")) 
    {
	return CubeSample;
    }
    else if (! strcmp (primitive, "triangles")) 
    {
	return TriangleSample;
    }
    else if (! strcmp (primitive, "quads")) 
    {
	return QuadSample;
    }
    else if (! strcmp (primitive, "lines")) 
    {
	return LineSample;
    }
    else if (! strcmp (primitive, "polylines")) 
    {
	return PolylinesSample;
    }
    else if (! strcmp (primitive, "faces")) 
    {
	return FacesSample;
    }
    else
    {
	DXSetError (ERROR_DATA_INVALID, "unknown primitive: %s", primitive);
	goto error;
    }

error:
    return NULL;
}

static Error
TriangleSample(Field field, int nSamples, Array grid)
{
    int i, n, nd;
    int counts[3];
    Array pA;

    pA = (Array)DXGetComponentValue(field, "positions");
    if (! DXQueryGridPositions(pA, &n, counts, NULL, NULL))
    {
	DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nd);
    }
    else
    {
	for (i = nd = 0; i < n; i++)
	    if (counts[i] > 1)
		nd ++;
    }
    
    if (nd == 2)
	return PlanarSurface(field, 3, coords_triangle, nSamples, grid);
    else if (nd == 3)
	return CurvedSurface(field, 3, coords_triangle, nSamples, grid);
    else
    {
	DXSetError(ERROR_DATA_INVALID, "triangles require 2 or 3D positions");
	goto error;
    }

error:
    return ERROR;
}


static Error
QuadSample(Field field, int nSamples, Array grid)
{
    int i, n, nd;
    int counts[3];
    Array pA;

    pA = (Array)DXGetComponentValue(field, "positions");
    if (! DXQueryGridPositions(pA, &n, counts, NULL, NULL))
    {
	DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nd);
    }
    else
    {
	for (i = nd = 0; i < n; i++)
	    if (counts[i] > 1)
		nd ++;
    }
    
    if (nd == 2)
	return PlanarSurface(field, 4, coords_quad, nSamples, grid);
    else if (nd == 3)
	return CurvedSurface(field, 4, coords_quad, nSamples, grid);
    else
    {
	DXSetError(ERROR_DATA_INVALID, "triangles require 2 or 3D positions");
	goto error;
    }

error:
    return ERROR;
}

static Error
TetrahedraSample(Field field, int nSamples, Array grid)
{
    return Volume(field, 4, coords_tetrahedra, nSamples, grid);
}

static Error
CubeSample(Field field, int nSamples, Array grid)
{
    return Volume(field, 8, coords_cube, nSamples, grid);
}

static PFF
_dxfGetVolumeMethod(Field field)
{
    Object		obj;
    char		*primitive;

    obj = DXGetComponentAttribute (field, "connections", "element type");
    if (obj == NULL)
    {
	DXSetError(ERROR_MISSING_DATA, "no element type attribute");
	goto error;
    }

    primitive = DXGetString ((String) obj);
    if (primitive == NULL)
    {
	DXSetError(ERROR_MISSING_DATA,"unable to access element type attribute");
	goto error;
    }

    if (! strcmp (primitive, "tetrahedra")) 
    {
	return TetrahedraVolume;
    }
    else if (! strcmp (primitive, "cubes")) 
    {
	return CubeVolume;
    }
    else if (! strcmp (primitive, "triangles")) 
    {
	return TriangleArea;
    }
    else if (! strcmp (primitive, "quads")) 
    {
	return QuadArea;
    }
    else if (! strcmp (primitive, "lines")) 
    {
	return LineLength;
    }
    else
    {
	DXSetError (ERROR_DATA_INVALID, "invalid primitive: %s", primitive);
	goto error;
    }

error:
    return NULL;
}

typedef struct
{
    int   index;
    float weights[1];
} Interp;

static Error
PlanarSurface(Field field, int vPerE,
	int (*inside)(),
	int nSamples, Array grid)
{
    Array		   a_connections, a_positions;
    int			   n_connections, n_positions;
    float		   *eltPoints[MAX_V_PER_E];
    int			   basex, pDim, gDim;
    float		   gridy[2];
    char		   *grid_status = NULL;
    int			   grid_pt;
    float		   mint[2], maxt[2];
    float		   mintrel[2], maxtrel[2];
    int			   mintgrid[2], maxtgrid[2];
    int			   i, j, k;
    int			   gCounts[3];
    int			   deleteGrid=0;
    SegList 		   *pList = NULL, *iList = NULL;
    int			   size;
    float                  pBuf[MAX_V_PER_E][3];
    float                  tBuf[MAX_V_PER_E][2];
    ArrayHandle            pHandle = NULL;
    ArrayHandle            cHandle = NULL;
    int			   cBuf[32], *elt;
    InvalidComponentHandle iHandle = NULL;
    float		   mA[9], mB[3];
    float		   mAinv[9], mBinv[3];
    char		   *name;

    if (! grid)
    {
	float bVol, tVol, rat;
	float deltas[9];
	int   nd, counts[3];

	Array pA = (Array)DXGetComponentValue(field, "positions");

	if (DXQueryGridPositions(pA, &nd, counts, NULL, deltas))
	{
	    int i, j;
	    float l, *d;
	    for (bVol = 1.0, d = deltas, i = 0; i < nd; i++)
	    {
		if (counts[i] <= 1)
		{
		    d += nd;
		}
		else
		{
		    for (l = 0.0, j = 0; j < nd; j++, d++)
			l += *d * *d;
		    bVol *= (counts[i]-1)*sqrt(l);
		}
	    }
	}
	else
	{
	    bVol = _dxfBoxVolume((Object)field, NULL);
	    if (bVol < 0.0)
		goto error;
	}

	tVol = _dxfFieldVolume(field);
	if (tVol < 0.0)
	    goto error;
	
	if (tVol == 0.0)
	    goto empty;
	
	rat = bVol / tVol;
	if (rat > 0.99) rat = 1.0;
	nSamples *= rat;
	deleteGrid = 1;
	grid = _Sample_Grid((Object)field, nSamples, 2);
	if (! grid)
	    goto error;
    }
    else
	deleteGrid = 0;
    
    a_connections = (Array)DXGetComponentValue(field, "connections");
    a_positions   = (Array)DXGetComponentValue(field, "positions"  );

    DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(a_positions,   &n_positions,   NULL, NULL, NULL, &pDim);

    size = nSamples < 100 ? nSamples : nSamples / 10;

    pList = DXNewSegList(pDim*sizeof(float), size, size);
    iList = DXNewSegList(sizeof(int)+vPerE*sizeof(float), size, size);
    if (! pList || ! iList)
	goto error;

    if (! DXQueryGridPositions(grid, &gDim, gCounts, mB, mA))
	goto error;

    if (!  InvertRegular(gDim, mB, mA, mBinv, mAinv))
	goto error;

    /*
     * DXAllocate and initialize the grid-status array.
     */
    grid_status = (char *)DXAllocateZero(gCounts[0]*gCounts[1]*sizeof(char));
    if (grid_status == NULL)
	goto error;

    pHandle = DXCreateArrayHandle(a_positions);
    if (! pHandle)
        goto error;

    if (DXGetComponentValue(field, "invalid connections"))
    {
        iHandle = DXCreateInvalidComponentHandle((Object)field,
                                                NULL, "connections");
        if (iHandle == NULL)
            goto error;
    }
    
    cHandle = DXCreateArrayHandle(a_connections);
    if (! cHandle)
	goto error;

    for (i = 0; i < n_connections; i++)
    {
	elt = (int *)DXGetArrayEntry(cHandle, i, cBuf);

        if (iHandle && DXIsElementInvalid(iHandle, i))
            continue;

	/*
	 * Determine the extents of the bounding box of the element
	 */
	mint[0] = mint[1] =  DXD_MAX_FLOAT;
	maxt[0] = maxt[1] = -DXD_MAX_FLOAT;
	for (j = 0; j < vPerE; j++)
	{
	    float *p;
	    int ii, jj;
	
	    p = (float *)DXGetArrayEntry(pHandle, *elt++, pBuf+j*gDim);
	    
	    for (ii = 0; ii < 2; ii++)
	    {
		tBuf[j][ii] = mBinv[ii];
		for (jj = 0; jj < pDim; jj++)
		    tBuf[j][ii] += mAinv[jj*gDim+ii]*p[jj];
	    }
		    
	    eltPoints[j] = &tBuf[j][0];

	    p = eltPoints[j];

	    if (p[0] < mint[0]) mint[0] = p[0];
	    if (p[0] > maxt[0]) maxt[0] = p[0];
	    if (p[1] < mint[1]) mint[1] = p[1];
	    if (p[1] > maxt[1]) maxt[1] = p[1];
	}

	mintrel[0] = mint[0];
	mintrel[1] = mint[1];
 
	maxtrel[0] = maxt[0];
	maxtrel[1] = maxt[1];

	for (j = 0; j < 2; j++)
	{
	    mintgrid[j] = (int)(mintrel[j] - 0.1);
	    if (mintgrid[j] < 0) mintgrid[j] = 0;
	    else if (mintgrid[j] >= gCounts[j]) mintgrid[j] = gCounts[j]-1;

	    maxtgrid[j] = ceil(maxtrel[j] + 0.1);
	    if (maxtgrid[j] < 0) maxtgrid[j] = 0;
	    else if (maxtgrid[j] >= gCounts[j]) maxtgrid[j] = gCounts[j]-1;
	}

	basex = mintgrid[0] * gCounts[1];

	for (j  = mintgrid[0]; j <= maxtgrid[0]; j++)
	{
	    grid_pt = basex + mintgrid[1];

	    for (k  = mintgrid[1]; k <= maxtgrid[1]; k++)
	    {
		if (! grid_status[grid_pt])
		{
		    float weights[10];

		    gridy[0] = j;
		    gridy[1] = k;

		    if ((*inside)(gridy, (float **)eltPoints, weights))
		    {
			float  *pptr = (float *)DXNewSegListItem(pList);
			Interp *iptr = (Interp *)DXNewSegListItem(iList);
			int    ii;

			if (! pptr || ! iptr)
			    goto error;

			grid_status[grid_pt] = 1;
			
			if (pDim == 3)
			{
			    pptr[0] = mA[0*gDim+0]*gridy[0] +
				      mA[1*gDim+0]*gridy[1] +
				      mB[0];
			    pptr[1] = mA[0*gDim+1]*gridy[0] +
				      mA[1*gDim+1]*gridy[1] +
				      mB[1];
			    pptr[2] = mA[0*gDim+2]*gridy[0] +
				      mA[1*gDim+2]*gridy[1] +
				      mB[2];
			}
			else
			{
			    pptr[0] = mA[0*gDim+0]*gridy[0] +
				      mA[1*gDim+0]*gridy[1] +
				      mB[0];
			    pptr[1] = mA[0*gDim+1]*gridy[0] +
				      mA[1*gDim+1]*gridy[1] +
				      mB[1];
			}

			iptr->index = i;
			for (ii=0; ii<vPerE; ii++)
			    iptr->weights[ii] = weights[ii];
		    }
		}

		grid_pt  ++;
	    }

	    basex += gCounts[1];
	}
    }

    DXFree((Pointer) grid_status);
    grid_status = NULL;

    if (DXGetSegListItemCount(pList) != 0)
    {
        if (! CreateOutputIrreg(field, pList, iList))
	    goto error;
	
	goto done;
    }
    
empty:

    while (DXGetEnumeratedComponentValue(field, 0, &name))
	DXDeleteComponent(field, name);

done:

    if (pHandle)
        DXFreeArrayHandle(pHandle);

    if (cHandle)
        DXFreeArrayHandle(cHandle);

    if (iHandle)
        DXFreeInvalidComponentHandle(iHandle);

    if (iList)
	DXDeleteSegList(iList);

    if (pList)
	DXDeleteSegList(pList);
    
    if (deleteGrid)
	DXDelete((Object)grid);

    return OK;

error:
    if (pHandle)
        DXFreeArrayHandle(pHandle);

    if (cHandle)
        DXFreeArrayHandle(cHandle);

    if (iHandle)
        DXFreeInvalidComponentHandle(iHandle);

    if (iList)
	DXDeleteSegList(iList);

    if (pList)
	DXDeleteSegList(pList);

    if (deleteGrid)
	DXDelete((Object)grid);

    DXFree ((Pointer)grid_status);
    return ERROR;
}

static Error
CurvedSurface(Field field,
		int vPerE, int (*inside)(),
		int nSamples, Array grid)
{
    Array		   a_connections, a_positions;
    int			   n_connections, n_positions;
    float		   element[MAX_V_PER_E][3];
    float		   element2D[MAX_V_PER_E][2], *elt2D[MAX_V_PER_E];
    int			   *connections, cBuf[32];
    float		   gridx[2], gridy[2];
    float		   mint[2], maxt[2];
    int			   mintgrid[2], maxtgrid[2];
    int			   i, j, k;
    float		   gDeltas[9], gOrigin[3];
    int			   gCounts[3], gSizes[3];
    int			   fCounts[2];
    int			   deleteGrid=0;
    float		   matrix[9];
    int			   p_axis, axis0, axis1;
    float		   v0[3], v1[3], normal[3], a, b, c, d, *p;
    SegList 		   *pList = NULL, *iList = NULL;
    int			   size;
    ArrayHandle            pHandle = NULL;
    ArrayHandle            cHandle = NULL;
    InvalidComponentHandle iHandle = NULL;
    char		   *name;

    size = nSamples < 100 ? nSamples : nSamples / 10;

    pList = DXNewSegList(3*sizeof(float), size, size);
    iList = DXNewSegList(sizeof(int)+vPerE*sizeof(float), size, size);
    if (! pList || ! iList)
	goto error;

    for (i = 0; i < vPerE; i++)
	elt2D[i] = &element2D[i][0];

    if (! grid)
    {
	deleteGrid = 1;
	grid = _Sample_Grid((Object)field, nSamples, 2);
	if (! grid)
	    goto error;
    }
    else
	deleteGrid = 0;

    a_connections = (Array)DXGetComponentValue(field, "connections");
    a_positions   = (Array)DXGetComponentValue(field, "positions"  );

    DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(a_positions,   &n_positions,   NULL, NULL, NULL, NULL);

    if (! DXQueryGridPositions(grid, NULL, gCounts, gOrigin, gDeltas))
	goto error;

    gSizes[2] = 1;
    gSizes[1] = gCounts[2];
    gSizes[0] = gCounts[0]*gSizes[1];

    if (! invert33(gDeltas, matrix))
	goto error;

    pHandle = DXCreateArrayHandle(a_positions);
    if (! pHandle)
        goto error;

    if (DXGetComponentValue(field, "invalid connections"))
    {
        iHandle = DXCreateInvalidComponentHandle((Object)field,
                                                NULL, "connections");
        if (iHandle == NULL)
            goto error;
    }

    cHandle = DXCreateArrayHandle(a_connections);
    if (! cHandle)
	goto error;

    for (i = 0; i < n_connections; i++)
    {
        if (iHandle && DXIsElementInvalid(iHandle, i))
	{
	    connections += vPerE;
            continue;
	}

	connections = (int *)DXGetArrayEntry(cHandle, i, cBuf);

	/*
	 * DXTransform element to canonical coordinates: unit steps,
         * origin at grid origin.
	 */
	for (j = 0; j < vPerE; j++)
	{
	    float *p, p0[3], pBuf[MAX_V_PER_E];
	
	    p = (float *)DXGetArrayEntry(pHandle, *connections++,
							(Pointer)pBuf);

	    p0[0] = p[0] - gOrigin[0];
	    p0[1] = p[1] - gOrigin[1];
	    p0[2] = p[2] - gOrigin[2];

	    element[j][0] = p0[0]*matrix[0]+p0[1]*matrix[3]+p0[2]*matrix[6];
	    element[j][1] = p0[0]*matrix[1]+p0[1]*matrix[4]+p0[2]*matrix[7];
	    element[j][2] = p0[0]*matrix[2]+p0[1]*matrix[5]+p0[2]*matrix[8];
	}

	/*
	 * Determine which face element is most perpendicular to.  To
	 * do so, compute planar equation.
	 */
	v0[0] = element[1][0] - element[0][0];
	v0[1] = element[1][1] - element[0][1];
	v0[2] = element[1][2] - element[0][2];

	v1[0] = element[2][0] - element[0][0];
	v1[1] = element[2][1] - element[0][1];
	v1[2] = element[2][2] - element[0][2];

	if (vector_zero((Vector *)v0) || vector_zero((Vector *)v1))
	    continue;
	
	normal[0] = v0[1]*v1[2] - v1[1]*v0[2];
	normal[1] = v0[2]*v1[0] - v1[2]*v0[0];
	normal[2] = v0[0]*v1[1] - v1[0]*v0[1];
    
	if (vector_zero((Vector *)normal))
	    continue;
	
	if ((a = normal[0]) < 0.0)
	    a = -a;

	if ((b = normal[1]) < 0.0)
	    b = -b;

	if ((c = normal[2]) < 0.0)
	    c = -c;
    
	if (a >= b && a >= c)
	{
	    /*
	     * use the yz plane
	     */
	    p_axis = 0;
	    axis0  = 1;
	    axis1  = 2;
	}
	else if (b >= c)
	{
	    /*
	     * use the xz plane
	     */
	    axis0  = 0;
	    p_axis = 1;
	    axis1  = 2;
	}
	else
	{
	    /*
	     * use the xy plane
	     */
	    axis0  = 0;
	    axis1  = 1;
	    p_axis = 2;
	}

	a = normal[0];
	b = normal[1];
	c = normal[2];
	d = -(a*element[0][0] + b*element[0][1] + c*element[0][2]);

	fCounts[0] = gCounts[axis0];
	fCounts[1] = gCounts[axis1];

	/*fOrigin[0] = gOrigin[axis0];*/
	/*fOrigin[1] = gOrigin[axis1];*/

	mint[0] = mint[1] =  DXD_MAX_FLOAT;
	maxt[0] = maxt[1] = -DXD_MAX_FLOAT;

	/*
	 * Project onto selected axis and subtract off the face origin
	 * This leaves us in a face-space in which the (0,0) grid point
	 * is at (0.0, 0.0).
	 */
	for (j = 0; j < vPerE; j++)
	{
	    element2D[j][0] = element[j][axis0];
	    element2D[j][1] = element[j][axis1];

	    p = &element2D[j][0];

	    if (p[0] < mint[0]) mint[0] = p[0];
	    if (p[0] > maxt[0]) maxt[0] = p[0];

	    if (p[1] < mint[1]) mint[1] = p[1];
	    if (p[1] > maxt[1]) maxt[1] = p[1];
	}

	for (j = 0; j < 2; j++)
	{
	    mintgrid[j] = (int)mint[j];
	    if (mintgrid[j] < 0)
		mintgrid[j] = 0;
	    else if (mintgrid[j] >= fCounts[j])
		mintgrid[j] = fCounts[j] - 1;

	    maxtgrid[j] = (int)maxt[j];
	    if (maxtgrid[j] < 0)
		maxtgrid[j] = 0;
	    else if (maxtgrid[j] >= fCounts[j])
		maxtgrid[j] = fCounts[j] - 1;
	}

	gridx[0] = mintgrid[0];
	gridx[1] = 0.0;

	for (j  = mintgrid[0]; j <= maxtgrid[0]; j++)
	{
	    gridy[0] = gridx[0];
	    gridy[1] = gridx[1] + mintgrid[1];

	    for (k  = mintgrid[1]; k <= maxtgrid[1]; k++)
	    {
		float g_pt[MAX_V_PER_E];
		float weights[MAX_V_PER_E];

		if ((*inside)(gridy, elt2D, weights))
		{
		    float     *pptr = (float *)DXNewSegListItem(pList);
		    Interp *iptr = (Interp *)DXNewSegListItem(iList);
		    int	      ii;

		    if (! pptr || ! iptr)
			goto error;

		    switch(p_axis)
		    {
			case 0:
			    g_pt[1] = gridy[0];
			    g_pt[2] = gridy[1];
			    g_pt[0] = (b*g_pt[1] + c*g_pt[2] + d) / -a;
			    break;
			case 1:
			    g_pt[0] = gridy[0];
			    g_pt[2] = gridy[1];
			    g_pt[1] = (a*g_pt[0] + c*g_pt[2] + d) / -b;
			    break;
			case 2:
			    g_pt[0] = gridy[0];
			    g_pt[1] = gridy[1];
			    g_pt[2] = (a*g_pt[0] + b*g_pt[1] + d) / -c;
			    break;
		    }

		    pptr[0] = g_pt[0]*gDeltas[0] +
			      g_pt[1]*gDeltas[3] +
			      g_pt[2]*gDeltas[6] + gOrigin[0];
		    pptr[1] = g_pt[0]*gDeltas[1] +
			      g_pt[1]*gDeltas[4] +
			      g_pt[2]*gDeltas[7] + gOrigin[1];
		    pptr[2] = g_pt[0]*gDeltas[2] +
			      g_pt[1]*gDeltas[5] +
			      g_pt[2]*gDeltas[8] + gOrigin[2];

		    iptr->index = i;
		    for (ii = 0; ii < vPerE; ii++)
			iptr->weights[ii] = weights[ii];
		}

		gridy[1] += 1.0;
	    }

	    gridx[0] += 1.0;
	}
    }

    if (DXGetSegListItemCount(pList) != 0)
    {
        if (! CreateOutputIrreg(field, pList, iList))
	    goto error;
	
	goto done;
    }
    
    while(DXGetEnumeratedComponentValue(field, 0, &name))
	DXDeleteComponent(field, name);

done:

    if (pHandle)
        DXFreeArrayHandle(pHandle);

    if (cHandle)
        DXFreeArrayHandle(cHandle);

    if (iHandle)
        DXFreeInvalidComponentHandle(iHandle);

    if (pList)
	DXDeleteSegList(pList);

    if (iList)
	DXDeleteSegList(iList);
    
    if (deleteGrid)
	DXDelete((Object)grid);
    
    return OK;

error:
    if (pHandle)
        DXFreeArrayHandle(pHandle);

    if (cHandle)
        DXFreeArrayHandle(cHandle);

    if (iHandle)
        DXFreeInvalidComponentHandle(iHandle);

    if (pList)
	DXDeleteSegList(pList);

    if (iList)
	DXDeleteSegList(iList);

    if (deleteGrid)
	DXDelete((Object)grid);

    return ERROR;
}

static Error
Volume(Field field,
		int vPerE, int (*inside)(),
		int nSamples, Array grid)
{
    Array		   a_connections, a_positions;
    int			   n_connections, n_positions;
    float		   *element[MAX_V_PER_E];
    int			   yz, xyz;
    int			   basex, basey;
    float		   gridx[3], gridy[3], gridz[3];
    char		   *grid_status = NULL;
    int			   grid_pt;
    float		   mint[3], maxt[3];
    float		   mintrel[3], maxtrel[3];
    int			   mintgrid[3], maxtgrid[3];
    int			   i, j, k, l;
    float		   gDeltas[9], gOrigin[3];
    int			   gCounts[3];
    int			   deleteGrid=0;
    float		   matrix[9];
    SegList 		   *pList = NULL, *iList = NULL;
    int			   size;
    ArrayHandle		   pHandle = NULL, cHandle = NULL;
    float 		   pBuf[MAX_V_PER_E][3];
    int			   eBuf[MAX_V_PER_E], *elt;
    InvalidComponentHandle iHandle = NULL;
    char		   *name;

    size = nSamples < 100 ? nSamples : nSamples / 10;

    pList = DXNewSegList(3*sizeof(float), size, size);
    iList = DXNewSegList(sizeof(int)+vPerE*sizeof(float), size, size);
    if (! pList || ! iList)
	goto error;

    if (! grid)
    {
	float bVol, tVol, rat;

	deleteGrid = 1;

	bVol = _dxfBoxVolume((Object)field, NULL);
	if (bVol < 0.0)
	    goto error;
	
	tVol = _dxfFieldVolume(field);
	if (tVol < 0.0)
	    goto error;
	
	if (tVol == 0.0)
	    goto empty;
	
	rat = bVol / tVol;
	if (rat > 0.99) rat = 1.0;
	nSamples *= rat;
	grid = _Sample_Grid((Object)field, nSamples, 3);
	if (! grid)
	    goto error;
    }
    else
	deleteGrid = 0;

    a_connections = (Array)DXGetComponentValue(field, "connections");
    a_positions   = (Array)DXGetComponentValue(field, "positions"  );

    DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(a_positions,   &n_positions,   NULL, NULL, NULL, NULL);

    if (! DXQueryGridPositions(grid, NULL, gCounts, gOrigin, gDeltas))
	goto error;

    yz  = gCounts[1] * gCounts[2];
    xyz = gCounts[0] * yz;

    if (! invert33(gDeltas, matrix))
	goto error;

    /*
     * DXAllocate and initialize the grid-status array.
     */

    grid_status = (char *)DXAllocateZero(xyz * sizeof(char));
    if (grid_status == NULL)
	goto error;

    pHandle = DXCreateArrayHandle(a_positions);
    cHandle = DXCreateArrayHandle(a_connections);
    if (! pHandle || ! cHandle)
	goto error;
	
    if (DXGetComponentValue(field, "invalid connections"))
    {
	iHandle = DXCreateInvalidComponentHandle((Object)field,
						NULL, "connections");
	if (! iHandle)
	    goto error;
    }

    for (i = 0; i < n_connections; i++)
    {
	if (iHandle && DXIsElementInvalid(iHandle, i))
	    continue;

	elt = (int *)DXGetArrayEntry(cHandle, i, (Pointer)eBuf);

	/*
	 * Determine the extents of the bounding box of the element.
	 */
	mint[0] = mint[1] = mint[2] =  DXD_MAX_FLOAT;
	maxt[0] = maxt[1] = maxt[2] = -DXD_MAX_FLOAT;
	for (j = 0; j < vPerE; j++)
	{
	    float *p;
	
	    element[j] = (float *)DXGetArrayEntry(pHandle,
					elt[j], (Pointer)&pBuf[j][0]);

	    p = element[j];

	    if (*p < mint[0]) mint[0] = *p;
	    if (*p > maxt[0]) maxt[0] = *p;
	    p++;

	    if (*p < mint[1]) mint[1] = *p;
	    if (*p > maxt[1]) maxt[1] = *p;
	    p++;

	    if (*p < mint[2]) mint[2] = *p;
	    if (*p > maxt[2]) maxt[2] = *p;
	}

	for (j = 0; j < 3; j++)
	{
	    mint[j]  = mint[j] - gOrigin[j];
	    maxt[j]  = maxt[j] - gOrigin[j];
	}

	mintrel[0] = mint[0]*matrix[0]+mint[1]*matrix[3]+mint[2]*matrix[6];
	mintrel[1] = mint[0]*matrix[1]+mint[1]*matrix[4]+mint[2]*matrix[7];
	mintrel[2] = mint[0]*matrix[2]+mint[1]*matrix[5]+mint[2]*matrix[8];

	maxtrel[0] = maxt[0]*matrix[0]+maxt[1]*matrix[3]+maxt[2]*matrix[6];
	maxtrel[1] = maxt[0]*matrix[1]+maxt[1]*matrix[4]+maxt[2]*matrix[7];
	maxtrel[2] = maxt[0]*matrix[2]+maxt[1]*matrix[5]+maxt[2]*matrix[8];

	for (j = 0; j < 3; j++)
	    if (mintrel[j] > maxtrel[j])
	    {
		float t = maxtrel[j];
		maxtrel[j] = mintrel[j];
		mintrel[j] = t;
	    }

	for (j = 0; j < 3; j++)
	{
	    mintgrid[j] = floor(mintrel[j]-0.1);
	    if (mintgrid[j] < 0) mintgrid[j] = 0;
	    else if (mintgrid[j] > gCounts[j] - 1)
		mintgrid[j] = gCounts[j] - 1;

	    maxtgrid[j] = ceil(maxtrel[j]+0.1);
	    if (maxtgrid[j] < 0) maxtgrid[j] = 0;
	    else if (maxtgrid[j] > gCounts[j] - 1)
		maxtgrid[j] = gCounts[j] - 1;
	}

	basex = mintgrid[0] * yz;

	gridx[0] = gOrigin[0] + mintgrid[0]*gDeltas[0];
	gridx[1] = gOrigin[1] + mintgrid[0]*gDeltas[1];
	gridx[2] = gOrigin[2] + mintgrid[0]*gDeltas[2];

	for (j  = mintgrid[0]; j <= maxtgrid[0]; j++)
	{
	    basey = basex + mintgrid[1] * gCounts[2];

	    gridy[0] = gridx[0] + mintgrid[1]*gDeltas[3];
	    gridy[1] = gridx[1] + mintgrid[1]*gDeltas[4];
	    gridy[2] = gridx[2] + mintgrid[1]*gDeltas[5];

	    for (k = mintgrid[1]; k <= maxtgrid[1]; k++)
	    {
		grid_pt = basey + mintgrid[2];

		gridz[0] = gridy[0] + mintgrid[2]*gDeltas[6];
		gridz[1] = gridy[1] + mintgrid[2]*gDeltas[7];
		gridz[2] = gridy[2] + mintgrid[2]*gDeltas[8];

		for (l = mintgrid[2]; l <= maxtgrid[2]; l++)
		{
		    /*
		     * If we have already determined the element which
		     * contains the grid point, continue.
		     */
		    if (! grid_status[grid_pt])
		    {
			float weights[MAX_V_PER_E];
			if ((*inside)(gridz, element, weights))
			{
			    float  *pptr = (float *)DXNewSegListItem(pList);
			    Interp *iptr = (Interp *)DXNewSegListItem(iList);

			    grid_status[grid_pt] = 1;

			    if (! pptr || ! iptr)
			        goto error;

			    pptr[0] = gridz[0];
			    pptr[1] = gridz[1];
			    pptr[2] = gridz[2];

			    iptr->index = i;
			    memcpy(&iptr->weights[0], weights,
							vPerE*sizeof(float));
			}
		    }

		    grid_pt  ++;
		    gridz[0] += gDeltas[6];
		    gridz[1] += gDeltas[7];
		    gridz[2] += gDeltas[8];
		}

		basey += gCounts[2];
		gridy[0] += gDeltas[3];
		gridy[1] += gDeltas[4];
		gridy[2] += gDeltas[5];
	    }

	    basex += yz;
	    gridx[0] += gDeltas[0];
	    gridx[1] += gDeltas[1];
	    gridx[2] += gDeltas[2];
	}
    }

    DXFree((Pointer) grid_status);
    grid_status = NULL;

    if (DXGetSegListItemCount(pList) != 0)
    {
        if (! CreateOutputIrreg(field, pList, iList))
	    goto error;
	
	goto done;
    }
    
empty:

    while (DXGetEnumeratedComponentValue(field, 0, &name))
	DXDeleteComponent(field, name);

done:

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    
    if (iHandle)
	DXFreeInvalidComponentHandle(iHandle);

    if (iList)
	DXDeleteSegList(iList);

    if (pList)
	DXDeleteSegList(pList);

    if (deleteGrid)
	DXDelete((Object)grid);
    
    return OK;

error:
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    
    if (iHandle)
	DXFreeInvalidComponentHandle(iHandle);

    if (iList)
	DXDeleteSegList(iList);

    if (pList)
	DXDeleteSegList(pList);

    if (deleteGrid)
	DXDelete((Object)grid);

    DXFree ((Pointer)grid_status);
    return ERROR;
}

typedef struct
{
    int   index;
    int   i0, i1;
    float weights[2];
} PolylineInterp;

static Error
PolylinesSample(Field field, int nSamples, Array grid)
{
    int            	   i, j, k, nDim, nPts, nPolylines, nEdges;
    ArrayHandle    	   points   = NULL;
    InvalidComponentHandle ich      = NULL;
    Array   	           pA, plA, eA;
    float		   totLength;
    float		   spacing, next;
    char		   *name;
    int			   *polylines, *edges;
    SegList                *pList   = NULL;
    SegList                *iList   = NULL;

    if (nSamples == 0)
	goto empty;

    if (DXEmptyField(field))
	goto error;

    pA = (Array)DXGetComponentValue(field, "positions");
    if (! pA)
	goto error;
    
    if (! DXGetArrayInfo(pA, &nPts, NULL, NULL, NULL, &nDim))
	goto error;
    
    points = DXCreateArrayHandle(pA);
    if (! points)
	goto error;
    
    plA = (Array)DXGetComponentValue(field, "polylines");
    if (! plA)
	goto error;
    
    if (! DXGetArrayInfo(plA, &nPolylines, NULL, NULL, NULL, NULL))
	goto error;
    
    eA = (Array)DXGetComponentValue(field, "edges");
    if (! plA)
	goto error;
    
    if (! DXGetArrayInfo(eA, &nEdges, NULL, NULL, NULL, NULL))
	goto error;
    
    polylines = (int *)DXGetArrayData(plA);
    edges     = (int *)DXGetArrayData(eA);

    if (DXGetComponentValue(field, "invalid polylines"))
    {
	ich = DXCreateInvalidComponentHandle((Object)field, NULL, "polylines");
	if (! ich)
	    goto error;
    }

    totLength = 0.0;
    for (i = 0; i < nPolylines; i++)
    {
	float *p0, p0Buf[10];
	float *p1, p1Buf[10];
	int   start, end, knt, *seg;

	if (ich && DXIsElementInvalid(ich, i))
	    continue;

	start = polylines[i];
	end   = (i == nPolylines-1) ? nEdges-1 : polylines[i+1]-1;
	knt   = end - start;

	seg = edges + start;

	p1 = (float *)DXGetArrayEntry(points, *seg, p0Buf);

	for (j = 0, seg++; j < knt; j++, seg++)
	{
	    float l;

	    p0 = p1;
	    p1 = (float *)DXGetArrayEntry(points, *seg,
				((j & 0x1) ? p0Buf : p1Buf));

	    for (l = 0.0, k = 0; k < nDim; k++)
	    {
		float d = p1[k] - p0[k];
		l += d*d;
	    }

	    totLength += sqrt(l);
	}
    }

    if (totLength == 0) 
	goto empty;

    pList = DXNewSegList(nDim*sizeof(float), nSamples, 10);
    iList = DXNewSegList(sizeof(PolylineInterp), nSamples, 10);
    if (! pList || ! iList)
	goto error;

    spacing = totLength / nSamples;
    next    = spacing / 2.0;

    totLength = 0.0;
    for (i = 0; i < nPolylines; i++)
    {
	float *p0, p0Buf[10];
	float *p1, p1Buf[10];
	int   start, end, knt, *seg;
	float l0, l1, l;
	int   p, q;

	if (ich && DXIsElementInvalid(ich, i))
	    continue;

	start = polylines[i];
	end   = (i == nPolylines-1) ? nEdges-1 : polylines[i+1]-1;
	knt   = end - start;

	seg = edges + start;
	q = *seg;

	p1 = (float *)DXGetArrayEntry(points, q, p0Buf);

	for (j = 0; j < knt; j++, totLength += l)
	{

	    p = q;
	    q = *(++seg);

	    p0 = p1;
	    p1 = (float *)DXGetArrayEntry(points, q, ((j & 0x1) ? p0Buf : p1Buf));

	    for (l = 0.0, k = 0; k < nDim; k++)
	    {
		float d = p1[k] - p0[k];
		l += d*d;
	    }

	    if (l == 0.0)
		continue;

	    l = sqrt(l);

	    l0 = totLength;
	    l1 = totLength + l;

	    while (next < l1)
	    {
		float          w1 = (next - l0) / (l1 - l0);
		float          w0 = 1.0 - w1;
		float          *pptr=(float *)DXNewSegListItem(pList);
		PolylineInterp *iptr=(PolylineInterp *)DXNewSegListItem(iList);
								
		iptr->index = i;

		iptr->i0 = p;
		iptr->i1 = q;
		iptr->weights[0] = w0;
		iptr->weights[1] = w1;

		for (k = 0; k < nDim; k++)
		    *pptr++ = w0*p0[k] + w1*p1[k];
		
		next += spacing;

	    }
	}
    }

    if (DXGetSegListItemCount(pList) != 0)
    {
        if (! CreateOutputPolylines(field, pList, iList))
	    goto error;
	
	goto done;
    }
    
empty:
    while (DXGetEnumeratedComponentValue(field, 0, &name))
	DXDeleteComponent(field, name);

done:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(points);
    DXDeleteSegList(pList);
    DXDeleteSegList(iList);

    return OK;

error:

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(points);
    DXDeleteSegList(pList);
    DXDeleteSegList(iList);

    return ERROR;
}

typedef struct
{
    int index;
    float weights[2];
} LineInterp;

struct segment
{
    struct segment *p0, *p1;
    byte           visited, invalid, reverse;
    int            seg;
    float	   length;
};


static Error
LineSample(Field field, int nSamples, Array grid)
{
    int            	   i, j, nDim, nPts, nSegs;
    struct segment 	   *segment = NULL;
    struct segment 	   **hash   = NULL;
    SegList 	   	   *pList   = NULL;
    SegList 	   	   *iList   = NULL;
    ArrayHandle    	   points   = NULL;
    ArrayHandle    	   elements = NULL;
    InvalidComponentHandle ich      = NULL;
    Array   	           pA, cA;
    float		   length, totLength, invLength;
    float		   spacing, next;
    char		   *name;

    if (DXEmptyField(field))
	goto error;

    pA = (Array)DXGetComponentValue(field, "positions");
    if (! pA)
	goto error;
    
    if (! DXGetArrayInfo(pA, &nPts, NULL, NULL, NULL, &nDim))
	goto error;
    
    points = DXCreateArrayHandle(pA);
    if (! points)
	goto error;
    
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	goto error;
    
    if (! DXGetArrayInfo(cA, &nSegs, NULL, NULL, NULL, NULL))
	goto error;
    
    elements = DXCreateArrayHandle(cA);
    if (! elements)
	goto error;
    
    segment = (struct segment *)DXAllocate(nSegs * sizeof(struct segment));
    if (! segment)
	goto error;

    hash = (struct segment **)DXAllocateZero(nPts * sizeof(struct segment *));
    if (! hash)
	goto error;

    pList = DXNewSegList(nDim*sizeof(float), nSamples, 10);
    iList = DXNewSegList(sizeof(int)+2*sizeof(float), nSamples, 10);
    if (! pList || ! iList)
	goto error;

    ich = DXCreateInvalidComponentHandle((Object)field, NULL, "connections");
    if (! ich)
	goto error;

    totLength = invLength = 0.0;
    for (i = 0; i < nSegs; i++)
    {
	struct segment *s;
	int   *elt, eBuf[2];
	float *p0, p0Buf[10];
	float *p1, p1Buf[10];
	float l;

	segment[i].visited = 0;
	segment[i].seg     = i;

	elt = (int *)DXGetArrayEntry(elements, i, eBuf);

	if (elt[0] == elt[1])
	{
	    segment[i].p0 = NULL;
	    segment[i].p1 = NULL;
	    segment[i].invalid = 1;
	    continue;
	}

	p0 = (float *)DXGetArrayEntry(points, elt[0], p0Buf);
	p1 = (float *)DXGetArrayEntry(points, elt[1], p1Buf);

	segment[i].invalid = DXIsElementInvalid(ich, i);

	for (l = 0.0, j = 0; j < nDim; j++)
	{
	    float d = p1[j] - p0[j];
	    l += d*d;
	}

	segment[i].length  = sqrt(l);

	if ((s = hash[elt[0]]) == NULL)
	{
	    hash[elt[0]]  = segment + i;
	    segment[i].p0 = NULL;
	}
	else
	{
	    int *nbr, nBuf[2];

	    nbr = DXGetArrayEntry(elements, s->seg, nBuf);

	    if (nbr[0] == elt[0])
		s->p0 = segment + i;
	    else
		s->p1 = segment + i;
		

	    segment[i].p0 = s;

	    hash[elt[0]]  = NULL;
	}

	if ((s = hash[elt[1]]) == NULL)
	{
	    hash[elt[1]]  = segment + i;
	    segment[i].p1 = NULL;
	}
	else
	{
	    int *nbr, nBuf[2];

	    nbr = DXGetArrayEntry(elements, s->seg, nBuf);

	    if (nbr[0] == elt[1])
		s->p0 = segment + i;
	    else
		s->p1 = segment + i;
		
	    segment[i].p1 = s;

	    hash[elt[1]]  = NULL;
	}

	totLength += segment[i].length;

	if (segment[i].invalid)
	    invLength += segment[i].length;
    }

    if (invLength == totLength || nSamples == 0)
	goto empty;

    nSamples = nSamples * totLength / (totLength - invLength);
    spacing = (totLength - invLength) / nSamples;
    length  = 0;
    next    = spacing / 2.0;

    for (i = 0; i < nSegs; i++)
	if (segment[i].visited == 0 && !segment[i].invalid)
	{
	    int first;
	    struct segment *p, *start, *l;

	    /*
	     * Trace back as far as possible
	     */
	    l = NULL;
	    start = p = &segment[i];
	    do
	    {
		if (l == NULL) /* then go either way */
		{
		    l = p;
		    p = p->p0;
		}
		else if (l == p->p0)
		{
		   l = p;
		   p = p->p1;
		}
		else
		{
		   l = p;
		   p = p->p0;
		}
	    } while (p && (p != start));
	    
	    /*
	     * Now forward setting reverse flag
	     */
	    start = p = l;
	    l = NULL;
	    do
	    {
		if (l == NULL) /* then go either way */
		{
		    l = p;
		    if (p->p0)
		    {
			p->reverse = 1;
			p = p->p0;
		    }
		    else
		    {
			p->reverse = 0;
			p = p->p1;
		    }
		}
		else if (l == p->p0)
		{
		    l = p;
		    p->reverse = 0;
		    p = p->p1;
		}
		else
		{
		    l = p;
		    p->reverse = 1;
		    p = p->p0;
		}
	    } while (p && (p != start));
	    
	    p = start;
	    do
	    {
	    float *p0=NULL, *p1=NULL;
		float l0 = length, l1 = l0 + p->length;

		p->visited = 1;
		length += p->length;

		first = 1;
		while (next < l1)
		{
		    int   *elt, eBuf[10];
		    float p0Buf[10];
		    float p1Buf[10];

		    if (! p->invalid)
		    {
			float      w1 = (next - l0) / (l1 - l0);
			float      w0 = 1.0 - w1;
			float      *pptr=(float *)DXNewSegListItem(pList);
			LineInterp *iptr=(LineInterp *)DXNewSegListItem(iList);
			if (first)
			{
			    elt=(int *)DXGetArrayEntry(elements, p->seg, eBuf);
			    p0=(float *)DXGetArrayEntry(points, elt[0], p0Buf);
			    p1=(float *)DXGetArrayEntry(points, elt[1], p1Buf);
			    first = 0;
			}

			if (! pptr || ! iptr)
			    goto error;
		    
			
			iptr->index = p->seg;
			if (p->reverse)
			{
			    iptr->weights[1] = w0;
			    iptr->weights[0] = w1;
			    for (j = 0; j < nDim; j++)
				*pptr++ = w1*p0[j] + w0*p1[j];
			}
			else
			{
			    iptr->weights[0] = w0;
			    iptr->weights[1] = w1;
			    for (j = 0; j < nDim; j++)
				*pptr++ = w0*p0[j] + w1*p1[j];
			}

		    }
		
		    next += spacing;
		}

		if (p->reverse)
		    p = p->p0;
		else
		    p = p->p1;

	    } while (p && (p != start));
	}

    if (DXGetSegListItemCount(pList) != 0)
    {
        if (! CreateOutputIrreg(field, pList, iList))
	    goto error;
	
	goto done;
    }
    
empty:
    while (DXGetEnumeratedComponentValue(field, 0, &name))
	DXDeleteComponent(field, name);

done:
		    
    DXFree((Pointer)segment);
    DXFree((Pointer)hash);
    DXFreeArrayHandle(points);
    DXFreeArrayHandle(elements);
    DXDeleteSegList(pList);
    DXDeleteSegList(iList);
    DXFreeInvalidComponentHandle(ich);

    return OK;

error:

    DXFree((Pointer)segment);
    DXFree((Pointer)hash);
    DXFreeArrayHandle(points);
    DXFreeArrayHandle(elements);
    DXDeleteSegList(pList);
    DXDeleteSegList(iList);
    DXFreeInvalidComponentHandle(ich);

    return ERROR;
}

static int
_invert33(float *a, float *b, float *c, float *dst)
{
    float m00, m01, m02;
    float m10, m11, m12;
    float m20, m21, m22;
    float c00, c01, c02;
    float c10, c11, c12;
    float c20, c21, c22;
    float det;

    m00 = *a++;
    m01 = *a++;
    m02 = *a++;
    m10 = *b++;
    m11 = *b++;
    m12 = *b++;
    m20 = *c++;
    m21 = *c++;
    m22 = *c++;

    c00 = m11*m22 - m12*m21;
    c01 = m10*m22 - m20*m12;
    c02 = m10*m21 - m20*m11;
    c10 = m01*m22 - m21*m02;
    c11 = m00*m22 - m02*m20;
    c12 = m00*m21 - m20*m01;
    c20 = m01*m12 - m11*m02;
    c21 = m00*m12 - m10*m02;
    c22 = m00*m11 - m10*m01;

    det = m00*c00 - m10*c10 + m20*c20;

    if (det == 0.0)
	return 0;
    
    det = 1.0 / det;

    *dst++ =  det * c00;
    *dst++ = -det * c10;
    *dst++ =  det * c20;
    *dst++ = -det * c01;
    *dst++ =  det * c11;
    *dst++ = -det * c21;
    *dst++ =  det * c02;
    *dst++ = -det * c12;
    *dst++ =  det * c22;

    return 1;
}

static int
invert33(float *src, float *dst)
{
    int i, j;
    int badVector[3], nBad;

    if (_invert33(src, src+3, src+6, dst))
	return 1;

    /*
     * OK.  Matrix was singular, so we can replace dependant vectors (or 
     * zero vectors) with a arbitrary independent vectors.  First, find bad
     * vectors.
     *
     * First possibility is zero vectors.
     */
    nBad = 0;
    for (i = 0; i < 3; i++)
    {
	int zeroVector = 1;

	for (j = 0; j < 3 && zeroVector; j++)
	    zeroVector = (src[i*3+j] == 0.0);

	if ((badVector[i] = zeroVector) == 1)
	    nBad ++;
    }

    if (nBad == 3)
	goto error;

    /* 
     * Now for each non-zero vector, cross it against the remaining
     * non-zero vectors and look at the length of the result to see if they
     * are parallel.
     */
    for (i = 0; i < 2; i++)
    {
	if (badVector[i])
	    continue;
	else
	{
	    float *v = src + 3*i;

	    for (j = i+1; j < 3 && !badVector[i]; j++)
	    {
		if (badVector[j])
		    continue;
		else
		{
		    float *w = src + 3*j;

		    if ((v[2]*w[1] == v[1]*w[2]) &&
			(v[0]*w[2] == v[2]*w[0]) &&
			(v[1]*w[0] == v[0]*w[1]))
		    {
			badVector[i] = 1;
			nBad ++;
		    }
		}
	    }
	}
    }

    /*
     * There are either 0, 1 or two vectors identified as bad.
     */
    switch(nBad)
    {
	/*
	 * If there are zero bad vectors, then any one is a combination
	 * of the other two.  So we can replace it by the cross.
	 */
	case 0:
	{
	    float *v = src;
	    float *w = src+3;
	    float c[3];

	    c[0] = v[2]*w[1] - v[1]*w[2];
	    c[1] = v[0]*w[2] - v[2]*w[0];
	    c[2] = v[1]*w[0] - v[0]*w[1];
	    if (! _invert33(v, w, c, dst))
		goto error;

	}
	break;

	/*
	 * If there is one bad vector, again, we replace it by the cross
	 * product of the others.
	 */
#define CROSS(a, b, c)					\
{							\
    c[0] = a[2]*b[1] - a[1]*b[2];			\
    c[1] = a[0]*b[2] - a[2]*b[0]; 			\
    c[2] = a[1]*b[0] - a[0]*b[1];			\
}

	case 1:
	{
	    float *v, *w, c[3];

	    if (badVector[0])
	    {
		v = src+3;
		w = src+6;
		CROSS(v, w, c);
		if (! _invert33(v, c, w, dst))
		    goto error;
	    }
	    else if (badVector[1])
	    {
		v = src;
		w = src+6;
		CROSS(v, w, c);
		if (! _invert33(v, c, w, dst))
		    goto error;
	    }
	    else
	    {
		v = src;
		w = src+3;
		CROSS(v, w, c);
		if (! _invert33(v, w, c, dst))
		    goto error;
	    }

	}
	break;

	/*
	 * If two are bad, then we find the good one, any non-parallel
	 * to it and the cross of the two.
	 */

#define NON_PARALLEL(a, b)				\
{							\
    if (a[0] == 0.0 && a[1] == 0.0)			\
    {							\
	b[0] = 1.0; b[1] = 0.0; b[2] = 0.0;		\
    }							\
    else						\
    {							\
	b[0] = 0.0; b[1] = 0.0; b[2] = 1.0;		\
    }							\
}

	case 2:
	{
	    float *v, c0[3], c1[3];

	    if (! badVector[0])
	    {
		v = src;
		NON_PARALLEL(v, c0);
		CROSS(v, c0, c1);
		if (! _invert33(v, c0, c1, dst))
		    goto error;
	    }
	    else if (! badVector[1])
	    {
		v = src+3;
		NON_PARALLEL(v, c0);
		CROSS(v, c0, c1);
		if (! _invert33(c0, v, c1, dst))
		    goto error;
	    }
	    else
	    {
		v = src+6;
		NON_PARALLEL(v, c0);
		CROSS(v, c0, c1);
		if (! _invert33(c0, c1, v, dst))
		    goto error;
	    }
	    
	}
	break;
    }

   return 1;

error:
    DXSetError(ERROR_DATA_INVALID, "degenerate regular positions");
    return 0;
}

static float
TetrahedraVolume(float **points, int pDim)
{
    float *p, *q, *r, *s, t;
    float pq[3], pr[3], ps[3];
    int i;

    p = points[0];
    q = points[1];
    r = points[2];
    s = points[3];

    for (i = 0; i < 3; i++)
    {
	pq[i] = q[i] - p[i];
	pr[i] = r[i] - p[i];
	ps[i] = s[i] - p[i];
    }

    if (vector_zero((Vector *)pq) ||
	vector_zero((Vector *)pr) ||
	vector_zero((Vector *)ps)) return 0.0;

    t = 0.16666667 *
		(pq[0] * (pr[1]*ps[2] - pr[2]*ps[1]) -
		 pr[0] * (pq[1]*ps[2] - pq[2]*ps[1]) +
		 ps[0] * (pq[1]*pr[2] - pq[2]*pr[1]));
    
    if (t >= 0.0) return t;
    else return -t;
}

static int
coords_tetrahedra(float *pt, float **tetra, float *weights)
{
    float 	vol0, vol1, vol2, vol3;
    float	xt, x0, x1, x2, x3;
    float	yt, y0, y1, y2, y3;
    float	zt, z0, z1, z2, z3;
    float 	x01, x02, x03, x0t, xt1, xt2, xt3;
    float 	y01, y02, y03, y0t, yt1, yt2, yt3;
    float 	z01, z02, z03, z0t, zt1, zt2, zt3;
    float	yz012, yz013, yz023, yzt12, yzt13;
    float	yzt23, yz0t2, yz0t3, yz01t, yz02t;


    xt = pt[0];    yt = pt[1];    zt = pt[2];
    x0 = tetra[0][0];    y0 = tetra[0][1];    z0 = tetra[0][2];
    x1 = tetra[1][0];    y1 = tetra[1][1];    z1 = tetra[1][2];
    x2 = tetra[2][0];    y2 = tetra[2][1];    z2 = tetra[2][2];
    x3 = tetra[3][0];    y3 = tetra[3][1];    z3 = tetra[3][2];

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

    if ((vol0 >= 0 && vol1 >= 0 && vol2 >= 0 && vol3 >= 0) ||
	(vol0 <= 0 && vol1 <= 0 && vol2 <= 0 && vol3 <= 0))
    {
	if (weights)
	{
	    float d, v = vol0 + vol1 + vol2 + vol3;
	    if (v != 0.0)
		d = 1.0 / v;
	    else
		d = 0.0;
	    
	    weights[0] = vol0 * d;
	    weights[1] = vol1 * d;
	    weights[2] = vol2 * d;
	    weights[3] = vol3 * d;
	}

	return 1;
    }

    return 0;
}

static float
CubeVolume(float **points, int pDim)
{
    float *tetra[4];
    float vol;

    tetra[0] = points[2];
    tetra[1] = points[3];
    tetra[2] = points[7];
    tetra[3] = points[1];
    vol  = TetrahedraVolume(tetra, pDim);

    tetra[0] = points[7];
    tetra[1] = points[1];
    tetra[2] = points[5];
    tetra[3] = points[4];
    vol += TetrahedraVolume(tetra, pDim);

    tetra[0] = points[7];
    tetra[1] = points[2];
    tetra[2] = points[6];
    tetra[3] = points[4];
    vol += TetrahedraVolume(tetra, pDim);

    tetra[0] = points[0];
    tetra[1] = points[1];
    tetra[2] = points[4];
    tetra[3] = points[2];
    vol += TetrahedraVolume(tetra, pDim);

    tetra[0] = points[2];
    tetra[1] = points[7];
    tetra[2] = points[1];
    tetra[3] = points[4];
    vol += TetrahedraVolume(tetra, pDim);

    return vol;
}

static int
coords_cube(float *pt, float **cube, float *weights)
{
    float *tet[4];
    float tetweights[4];

    if (weights)
	memset(weights, 0, 8*sizeof(float));

    tet[0] = cube[2]; tet[1] = cube[3]; tet[2] = cube[7]; tet[3] = cube[1];
    if (coords_tetrahedra(pt, tet, tetweights))
    {
	if (weights)
	{
	    weights[2] = tetweights[0];
	    weights[3] = tetweights[1];
	    weights[7] = tetweights[2];
	    weights[1] = tetweights[3];
	}
	return 1;
    }

    tet[0] = cube[7]; tet[1] = cube[1]; tet[2] = cube[5]; tet[3] = cube[4];
    if (coords_tetrahedra(pt, tet, tetweights))
    {
	if (weights)
	{
	    weights[7] = tetweights[0];
	    weights[1] = tetweights[1];
	    weights[5] = tetweights[2];
	    weights[4] = tetweights[3];
	}
	return 1;
    }

    tet[0] = cube[7]; tet[1] = cube[2]; tet[2] = cube[6]; tet[3] = cube[4];
    if (coords_tetrahedra(pt, tet, tetweights))
    {
	if (weights)
	{
	    weights[7] = tetweights[0];
	    weights[2] = tetweights[1];
	    weights[6] = tetweights[2];
	    weights[4] = tetweights[3];
	}
	return 1;
    }

    tet[0] = cube[0]; tet[1] = cube[1]; tet[2] = cube[4]; tet[3] = cube[2];
    if (coords_tetrahedra(pt, tet, tetweights))
    {
	if (weights)
	{
	    weights[0] = tetweights[0];
	    weights[1] = tetweights[1];
	    weights[4] = tetweights[2];
	    weights[2] = tetweights[3];
	}
	return 1;
    }

    tet[0] = cube[2]; tet[1] = cube[7]; tet[2] = cube[1]; tet[3] = cube[4];
    if (coords_tetrahedra(pt, tet, tetweights))
    {
	if (weights)
	{
	    weights[2] = tetweights[0];
	    weights[7] = tetweights[1];
	    weights[1] = tetweights[2];
	    weights[4] = tetweights[3];
	}
	return 1;
    }

    return 0;
}

static float
TriangleArea(float **points, int pDim)
{
    float *p0, *p1, *p2;
    float v0x, v0y;
    float v1x, v1y;
    float a;

    p0 = points[0];
    p1 = points[1];
    p2 = points[2];

    if (pDim == 2)
    {
	v0x = p1[0] - p0[0];
	v0y = p1[1] - p0[1];

	v1x = p2[0] - p0[0];
	v1y = p2[1] - p0[1];

	a = 0.5 * (v0x*v1y - v0y*v1x);
	if (a >= 0) return a;
	else return -a;
    }
    else
    {
	Vector v0, v1, cross;

	vector_subtract((Vector *)p0, (Vector *)p1, &v0);
	vector_subtract((Vector *)p0, (Vector *)p2, &v1);
	vector_cross(&v0, &v1, &cross);
	a = 0.5 * vector_length(&cross);
    }

    if (a >= 0) return a;
    else return -a;
}

static int
coords_triangle(float *pt, float **triangle, float *weights)
{
    float *p0, *p1, *p2;
    double v0x, v0y;
    double v1x, v1y;
    double a0, a1, a2;
    float d, a;

    p0 = triangle[0];
    p1 = triangle[1];
    p2 = triangle[2];

    v0x = p1[0] - pt[0];
    v0y = p1[1] - pt[1];
    v1x = p2[0] - pt[0];
    v1y = p2[1] - pt[1];
    a0 = v0x*v1y - v0y*v1x;

    v0x = p2[0] - pt[0];
    v0y = p2[1] - pt[1];
    v1x = p0[0] - pt[0];
    v1y = p0[1] - pt[1];
    a1 = v0x*v1y - v0y*v1x;

    v0x = p0[0] - pt[0];
    v0y = p0[1] - pt[1];
    v1x = p1[0] - pt[0];
    v1y = p1[1] - pt[1];
    a2 = v0x*v1y - v0y*v1x;

    a = a0 + a1 + a2;

    if (a == 0.0)
	return 0;
    else
	d = 1.0 / a;
	
    a0 = a0 * d;
    a1 = a1 * d;
    a2 = a2 * d;


    if (a0 < -0.0001 || a1 < -0.0001 || a2 < -0.0001)
        return 0;
	
    if (weights)
    {
	weights[0] = a0;
	weights[1] = a1;
	weights[2] = a2;
    }

    return 1;
}

static float
QuadArea(float **points, int pDim)
{
    float *tri0[3], *tri1[3];

    tri0[0] = points[0];
    tri0[1] = points[1];
    tri0[2] = points[2];

    tri1[0] = points[0];
    tri1[1] = points[2];
    tri1[2] = points[3];

    return TriangleArea(tri0, pDim) + TriangleArea(tri1, pDim);
}

static int
coords_quad(float *pt, float **quad, float *weights)
{
    float *tri[3];
    float tweights[3];

    tri[0] = quad[0];
    tri[1] = quad[1];
    tri[2] = quad[2];

    if (coords_triangle(pt, tri, tweights))
    {
	if (weights)
	{
	    weights[0] = tweights[0];
	    weights[1] = tweights[1];
	    weights[2] = tweights[2];
	    weights[3] = 0.0;
	}
	return 1;
    }

    tri[0] = quad[1];
    tri[1] = quad[3];
    tri[2] = quad[2];

    if (coords_triangle(pt, tri, tweights))
    {
	if (weights)
	{
	    weights[0] = 0.0;
	    weights[1] = tweights[0];
	    weights[2] = tweights[2];
	    weights[3] = tweights[1];
	}
	return 1;
    }
    
    return 0;
}

static float
LineLength(float **pts, int pDim)
{
    int i;
    float d, l;
    float *p0 = pts[0];
    float *p1 = pts[1];

    l = 0.0;
    for (i = 0; i < pDim; i++)
    {
       d = p0[i] - p1[i];
       l += d*d;
    }

    return sqrt(l);
}

#define INTERPOLATE(type, round)					\
{									\
    Interp *iPtr;							\
    char *sData = (char *)DXGetArrayData(sA);				\
    type *dPtr  = (type *)DXGetArrayData(dA);				\
									\
    DXInitGetNextSegListItem(iList);					\
    while (NULL != (iPtr = (Interp *)DXGetNextSegListItem(iList)))	\
    {									\
	int j, k, *elt;							\
	type *sPtr;							\
									\
	elt = (int *)DXGetArrayEntry(elements, iPtr->index, eBuf);	\
									\
	sPtr = (type *)(sData + elt[0]*itemSize);			\
									\
	for (j = 0; j < itemValues; j++)				\
	    fBuf[j] = iPtr->weights[0]*sPtr[j];				\
									\
	for (j = 1; j < nV; j++)					\
	{								\
	    sPtr = (type *)(sData + elt[j]*itemSize);			\
									\
	    for (k = 0; k < itemValues; k++)				\
		fBuf[k] += iPtr->weights[j]*sPtr[k];			\
	}								\
									\
	for (j = 0; j < itemValues; j++)				\
	    dPtr[j] = fBuf[j] + round;					\
									\
	dPtr += itemValues;						\
    }									\
}

static Field
CreateOutputIrreg(Field f, SegList *pList, SegList *iList)
{
    Array          cA = NULL, sA, dA = NULL;
    char           *dptr;
    SegListSegment *slist;
    char           *name, *toDelete[100];
    int            nV, nDelete=0;
    int            i, nSamples = DXGetSegListItemCount(pList);
    float          *fBuf = NULL;
    ArrayHandle    elements = NULL;
    int		   eBuf[32];

    cA = (Array)DXGetComponentValue(f, "connections");
    if (! cA)
    {
	DXSetError(ERROR_INTERNAL, "missing connections");
	goto error;
    }
    DXReference((Object)cA);

    DXDeleteComponent(f, "connections");

    elements = DXCreateArrayHandle(cA);
    if (! elements)
	goto error;

    DXGetArrayInfo(cA, NULL, NULL, NULL, NULL, &nV);

    i = 0;
    while(NULL != (sA = (Array)DXGetEnumeratedComponentValue(f, i++, &name)))
    {
	Object   at;
	Type     t; 
	Category c;
	int      r, shape[32];
	int      itemSize, itemValues;
	int      nItems;

	if (! strcmp(name, "positions"))
	    continue;
	    
	if (DXGetComponentAttribute(f, name, "ref") ||
	    DXGetComponentAttribute(f, name, "der") || 
	    !strncmp(name, "invalid", 8) 	    ||
	    NULL == (at = DXGetComponentAttribute(f, name, "dep")))
	{
	    toDelete[nDelete++] = name;
	    continue;
	}

	if (DXGetObjectClass(at) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
	    goto error;
	}

	DXGetArrayInfo(sA, NULL, &t, &c, &r, shape);
	
	itemSize   = DXGetItemSize(sA);
	itemValues = itemSize / DXTypeSize(t);

	if (DXQueryConstantArray(sA, NULL, NULL))
	{
	    dA = (Array)DXNewConstantArrayV(DXGetSegListItemCount(pList),
				DXGetConstantArrayData(sA), t, c, r, shape);
	    if (! dA)
		goto error;
	}
	else
	{
	    nItems = DXGetItemSize(sA) / DXTypeSize(t);

	    dA = DXNewArrayV(t, c, r, shape);
	    if (! dA)
		goto error;
	
	    if (! DXAddArrayData(dA, 0, nSamples, NULL))
		goto error;

	    if (! strcmp(DXGetString((String)at), "positions"))
	    {
		fBuf = (float *)DXAllocate(nItems * sizeof(float));
		if (fBuf == NULL)
		    goto error;

		switch(t) 
		{
		    case TYPE_FLOAT:  INTERPOLATE(float,  0.0); break;
		    case TYPE_DOUBLE: INTERPOLATE(double, 0.0); break;
		    case TYPE_SHORT:  INTERPOLATE(short,  0.5); break;
		    case TYPE_USHORT: INTERPOLATE(ushort, 0.5); break;
		    case TYPE_INT:    INTERPOLATE(int,    0.5); break;
		    case TYPE_UINT:   INTERPOLATE(uint,   0.5); break;
		    case TYPE_UBYTE:  INTERPOLATE(ubyte,  0.5); break;
		    case TYPE_BYTE:   INTERPOLATE(byte,   0.5); break;
		    default: break;
		}

		DXFree((Pointer)fBuf);
		fBuf = NULL;
	    }
	    else if (! strcmp(DXGetString((String)at), "connections"))
	    {
		char *src, *dst;
		Interp *iPtr;

		src = (char *)DXGetArrayData(sA);
		dst = (char *)DXGetArrayData(dA);

		DXInitGetNextSegListItem(iList);
		while (NULL != 
			(iPtr = (Interp *)DXGetNextSegListItem(iList)))
		{
		    memcpy(dst, src+(iPtr->index*itemSize), itemSize);
		    dst += itemSize;
		}
	    }
	    else
	    {
		DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
		goto error;
	    }
	}

	if (! DXSetComponentValue(f, name, (Object)dA))
	    goto error;
	
	if (! DXSetComponentAttribute(f, name, "dep", 
				(Object)DXNewString("positions")))
	    goto error;
    }

    for (i = 0; i < nDelete; i++)
	DXDeleteComponent(f, toDelete[i]);

    sA = (Array)DXGetComponentValue(f, "positions");
    if (! sA)
	goto error;
    
    if (! DXGetArrayInfo(sA, NULL, NULL, NULL, NULL, &nV))
	goto error;
    
    dA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nV);
    if (! dA)
	goto error;
    
    if (! DXAddArrayData(dA, 0, DXGetSegListItemCount(pList), NULL))
	goto error;
    
    dptr = (char *)DXGetArrayData(dA);
    DXInitGetNextSegListSegment(pList);
    while (NULL != (slist = DXGetNextSegListSegment(pList)))
    {
	int size = DXGetSegListSegmentItemCount(slist) * nV * sizeof(float);

	memcpy(dptr, DXGetSegListSegmentPointer(slist), size);
	dptr += size;
    }

    if (! DXSetComponentValue(f, "positions", (Object)dA))
	goto error;
    
    DXFreeArrayHandle(elements);
    DXDelete((Object)cA);

    return f;

error:
    DXFreeArrayHandle(elements);
    DXDelete((Object)cA);
    DXFree((Pointer)fBuf);

    return NULL;
}

#define INTERPOLATE_POLYLINE(type, round)					   \
{										   \
    PolylineInterp *iPtr;							   \
    type *dPtr  = (type *)DXGetArrayData(dA);					   \
										   \
    DXInitGetNextSegListItem(iList);						   \
    while (NULL != (iPtr = (PolylineInterp *)DXGetNextSegListItem(iList)))	   \
    {										   \
	int j;									   \
	type *s0;								   \
	type *s1;								   \
										   \
	s0 = (type *)DXGetArrayEntry(ah, iPtr->i0, buf0);			   \
	s1 = (type *)DXGetArrayEntry(ah, iPtr->i1, buf1);			   \
										   \
	for (j = 0; j < itemValues; j++)					   \
	    *dPtr++ = (type)(iPtr->weights[0]*s0[j]+iPtr->weights[1]*s1[j]+round); \
    }										   \
}
static Field
CreateOutputPolylines(Field f, SegList *pList, SegList *iList)
{
    Array          sA, dA = NULL;
    char           *dptr;
    SegListSegment *slist;
    char           *name, *toDelete[100];
    int            nV = 2, nDelete=0;
    int            i, nSamples = DXGetSegListItemCount(pList);
    ArrayHandle    ah = NULL;
    Pointer	   buf0 = NULL, buf1 = NULL;

    DXDeleteComponent(f, "polylines");
    DXDeleteComponent(f, "edges");

    i = 0;
    while(NULL != (sA = (Array)DXGetEnumeratedComponentValue(f, i++, &name)))
    {
	Object   at;
	Type     t; 
	Category c;
	int      r, shape[32];
	int      itemSize, itemValues;

	if (! strcmp(name, "positions"))
	    continue;

	if (DXGetComponentAttribute(f, name, "ref") ||
	    DXGetComponentAttribute(f, name, "der") || 
	    !strncmp(name, "invalid", 8) 	    ||
	    NULL == (at = DXGetComponentAttribute(f, name, "dep")))
	{
	    toDelete[nDelete++] = name;
	    continue;
	}

	if (DXGetObjectClass(at) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
	    goto error;
	}

	DXGetArrayInfo(sA, NULL, &t, &c, &r, shape);
	
	itemSize   = DXGetItemSize(sA);
	itemValues = itemSize / DXTypeSize(t);

	if (DXQueryConstantArray(sA, NULL, NULL))
	{
	    dA = (Array)DXNewConstantArrayV(DXGetSegListItemCount(pList),
				DXGetConstantArrayData(sA), t, c, r, shape);
	    if (! dA)
		goto error;
	}
	else
	{
	    /*nItems = DXGetItemSize(sA) / DXTypeSize(t);*/

	    dA = DXNewArrayV(t, c, r, shape);
	    if (! dA)
		goto error;
	
	    if (! DXAddArrayData(dA, 0, nSamples, NULL))
		goto error;

	    ah = DXCreateArrayHandle(sA);
	    if (! ah)
		goto error;
	    
	    buf0 = DXAllocate(DXGetItemSize(sA));
	    buf1 = DXAllocate(DXGetItemSize(sA));
	    if (! buf0 || ! buf1)
		goto error;

	    if (! strcmp(DXGetString((String)at), "positions"))
	    {
		switch(t) 
		{
		    case TYPE_FLOAT:  INTERPOLATE_POLYLINE(float,  0.0); break;
		    case TYPE_DOUBLE: INTERPOLATE_POLYLINE(double, 0.0); break;
		    case TYPE_SHORT:  INTERPOLATE_POLYLINE(short,  0.5); break;
		    case TYPE_USHORT: INTERPOLATE_POLYLINE(ushort, 0.5); break;
		    case TYPE_INT:    INTERPOLATE_POLYLINE(int,    0.5); break;
		    case TYPE_UINT:   INTERPOLATE_POLYLINE(uint,   0.5); break;
		    case TYPE_UBYTE:  INTERPOLATE_POLYLINE(ubyte,  0.5); break;
		    case TYPE_BYTE:   INTERPOLATE_POLYLINE(byte,   0.5); break;
		    default: break;
		}

	    }
	    else if (! strcmp(DXGetString((String)at), "polylines"))
	    {
		char *dst;
		PolylineInterp *iPtr;

		dst = (char *)DXGetArrayData(dA);

		DXInitGetNextSegListItem(iList);
		while (NULL != 
			(iPtr = (PolylineInterp *)DXGetNextSegListItem(iList)))
		{
		    memcpy(dst, DXGetArrayEntry(ah, iPtr->index, buf0), itemSize);
		    dst += itemSize;
		}
	    }
	    else
	    {
		DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
		goto error;
	    }
	}

	DXFree(buf0); buf0 = NULL;
	DXFree(buf1); buf1 = NULL;
	DXFreeArrayHandle(ah); ah = NULL;

	if (! DXSetComponentValue(f, name, (Object)dA))
	    goto error;
	
	if (! DXSetComponentAttribute(f, name, "dep", 
				(Object)DXNewString("positions")))
	    goto error;
    }

    for (i = 0; i < nDelete; i++)
	DXDeleteComponent(f, toDelete[i]);

    sA = (Array)DXGetComponentValue(f, "positions");
    if (! sA)
	goto error;
    
    if (! DXGetArrayInfo(sA, NULL, NULL, NULL, NULL, &nV))
	goto error;
    
    dA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nV);
    if (! dA)
	goto error;
    
    if (! DXAddArrayData(dA, 0, DXGetSegListItemCount(pList), NULL))
	goto error;
    
    dptr = (char *)DXGetArrayData(dA);
    DXInitGetNextSegListSegment(pList);
    while (NULL != (slist = DXGetNextSegListSegment(pList)))
    {
	int size = DXGetSegListSegmentItemCount(slist) * nV * sizeof(float);

	memcpy(dptr, DXGetSegListSegmentPointer(slist), size);
	dptr += size;
    }

    if (! DXSetComponentValue(f, "positions", (Object)dA))
	goto error;
    
    return f;

error:
    DXFree(buf0);
    DXFree(buf1);
    DXFree(ah);
    return NULL;
}

#define SQUEEZE(knts, del, pDim, eDim)				\
{								\
    int i, j, k;						\
								\
    /*								\
     * Squeeze out unnecessary axes				\
     */								\
    for (i = j = 0; i < pDim; i++)				\
    {								\
	if (knts[i] != 1)					\
	{							\
	    if (i != j)						\
	    {							\
		knts[j] = knts[i];				\
		for (k = 0; k < pDim; k++)			\
		    del[j*pDim+k] = del[i*pDim+k];		\
	    }							\
	    j++;						\
	}							\
    }								\
    if (j != eDim)						\
    {								\
	DXSetError(ERROR_DATA_INVALID, "dimension mismatch");	\
	goto error;						\
    }								\
    for (j = j; j < pDim; j++)					\
    {								\
	knts[j] = 1;						\
	for (k = 0; k < pDim; k++)				\
	    del[j*sd+k] = 0.0;					\
    }								\
}


static void
_Get_Counts(int ndim, int nSamples, float *lengths, int *samples)
{
    switch(ndim)
    {
	float A, B, a;

	case 1:
	    samples[0] = nSamples;
	    break;
	
	case 2:
	    A = lengths[1] / lengths[0];
	    a = sqrt(nSamples / A);
	    samples[0] = (int)(a + 0.5);
	    samples[1] = (int)((a * A) + 0.5);
	    if (! samples[0])
	    {
		samples[0] = nSamples;
		samples[1] = 1;
	    }
	    else if (! samples[1])
	    {
		samples[0] = 1;
		samples[1] = nSamples;
	    }
	    break;
	
	case 3:
	    A = lengths[1] / lengths[0];
	    B = lengths[2] / lengths[0];
	    a = pow(nSamples / (A*B), 0.333333333);
	    samples[0] = (int)(a + 0.5);
	    samples[1] = (int)((a * A) + 0.5);
	    samples[2] = (int)((a * B) + 0.5);
	    if (! samples[0] || ! samples[1] || !samples[2])
	    {
		int i, j, tmp_samples[3];
		float tmp_lengths[3];

		for (i = j = 0; i < 3; i++)
		    if (samples[i])
			tmp_lengths[j++] = lengths[i];
		
		if (j > 0)
		    _Get_Counts(j, nSamples, tmp_lengths, tmp_samples);

		for (i = j = 0; i < 3; i++)
		    if (samples[i]) samples[i] = tmp_samples[j++];
		else
		    samples[i] = 1;
	    }

	    break;
    }
}

static Array
_Sample_Grid_Regular(Object o, int nSamples, int eltDim)
{
    Field field;
    int   i, j, k, sd, ed, gridConnections=0;
    float deltas[9], *d;
    int   counts[3], samples[3];
    float origin[3];
    float lengths[3];
    Class class;
    Array grid;

    class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);
    
    ed = eltDim;

    if (class == CLASS_COMPOSITEFIELD)
    {
	Array cA, pA;
	int maxGrid[3], minGrid[3], pc[3], part;
	float m[9];

	/*
	 * We need the overall composite field origin and counts.
	 */
	for (part = 0, field = NULL; ;part++)
	{

	    Field f = (Field)DXGetEnumeratedMember((Group)o, part, NULL);
	    if (! f)
		break;
	    
	    if (DXEmptyField(f))
		continue;

	    if (field == NULL)
	    {
		int pc[3];
		int cc[3];

		field = f;

		cA = (Array)DXGetComponentValue(field, "connections");
		if (! cA)
		    goto error;

		pA = (Array)DXGetComponentValue(field, "positions");
		if (! pA)
		    goto error;

		if (! DXQueryGridPositions(pA, &sd, pc, origin, deltas))
		    goto error;
		
		SQUEEZE(pc, deltas, sd, ed);

		/*
		 * If the connections are grid, we can get this info from
		 * the mesh offset info.  Initialize for a search for the
		 * maximum count, and determine the overall origin.
		 */
		gridConnections = (NULL != DXQueryGridConnections(cA, &ed, cc));
		if (gridConnections)
		{
		    int mo[3];

		    for (i = 0; i < ed; i++)
			minGrid[i] = 0;

		    if (! DXGetMeshOffsets((MeshArray)cA, mo))
			goto error;
		    
		    /*
		     * determine overall offset by subtracting off the
		     * offset to the mesh origin.  Also initialize the max
		     * grid search.
		     */
		    for (i = 0; i < ed; i++)
		    {
			for (k = 0; k < sd; k++)
			    origin[k] -= mo[i]*deltas[i*sd+k];
			
			maxGrid[i] = cc[i] + mo[i] - 1;
		    }
		}
		else
		{
		    /*
		     * if not, then we use the origin of the first field
		     * as the temporary origin.  For each field, we get the 
		     * grid coordinates of its origin relative to the 
		     * (0,0,0) origin of the first field and use that
		     * as mesh offset.  This requires determining a back
		     * transformation from (xyz) space to the (ijk) space
		     * of the first field.
		     *
		     * Determine the inverse deltas matrix.  The result
		     * will map points in (xyz) space to (ijk) coordinates
		     * in the first field's grid.
		     */
		    if (! invert(sd, deltas, m))
			goto error;
		    
		    /*
		     * For each member of the composite field, we will
		     * determine the pseudo-mesh offsets relative to the 
		     * grid of this field.  
		     */
		    for (i = 0; i < sd; i++)
		    {
			minGrid[i] = 0;
			maxGrid[i] = pc[i] - 1;
		    }
		}
	    }
	    else
	    {
		if (gridConnections)
		{
		    int mo[3], cnts[3];

		    cA = (Array)DXGetComponentValue(f, "connections");
		    if (! cA)
			goto error;

		    if (! DXQueryGridConnections(cA, NULL, cnts))
			goto error;

		    if (! DXGetMeshOffsets((MeshArray)cA, mo))
			goto error;
		    
		    for (i = 0; i < sd; i++)
		    {
			j = mo[i] + cnts[i] - 1;
			if (j > maxGrid[i])
			    maxGrid[i] = j;
		    }
		}
		else
		{
		    int mo[3];
		    int pc[3];
		    float po[3];
		    float pdel[9];

		    pA = (Array)DXGetComponentValue(f, "positions");
		    if (! pA)
			goto error;

		    if (! DXQueryGridPositions(pA, NULL, pc, po, pdel))
			goto error;

		    SQUEEZE(pc, pdel, sd, ed);
		    
		    /*
		     * back-transform to get the origin in the first field's 
		     * grid coordinates
		     */
		    for (j = 0; j < sd; j++)
			po[j] -= origin[j];

		    for (i = 0; i < sd; i++)
		    {
			float f;

			f = 0;
			for (j = 0; j < sd; j++)
			    f += po[j]*m[j*sd+i];
			
			mo[i] = (int)(f + 0.5);
		    }

		    for (i = 0; i < sd; i++)
		    {
			if (mo[i] < minGrid[i])
			    minGrid[i] = mo[i];

			j = mo[i] + pc[i] - 1;
			if (j > maxGrid[i])
			    maxGrid[i] = j;
		    }
		}
	    }
	}

	if (gridConnections)
	{
	    for (i = 0; i < ed; i++)
		counts[i] = maxGrid[i] + 1;
	}
	else
	{
	    Array pA;
	    float po[3];
	    
	    pA = (Array)DXGetComponentValue(field, "positions");
	    if (! pA)
		goto error;
    
	    if (! DXQueryGridPositions(pA, &sd, pc, po, deltas))
		goto error;
	    
	    SQUEEZE(pc, deltas, sd, ed);

	    /*
	     * max and min grids are relative to this grid's origin.  Overall
	     * origin is offset from here by minGrid offsets.  
	     */
	    for (i = 0; i < sd; i++)
	    {
		origin[i] = po[i];
		for (j = 0; j < ed; j++)
		    origin[i] += minGrid[j]*deltas[j*sd+i];
	    }

	    /*
	     * For the meaningful axes, counts are max - min, else 1.
	     */
	    for (i = 0; i < ed; i++)
		counts[i] = (maxGrid[i] - minGrid[i]) + 1;
	}
    }
    else if (class == CLASS_FIELD)
    {
	Array pA;

	field = (Field)o;

	pA = (Array)DXGetComponentValue(field, "positions");
	if (! pA)
	    goto error;
    
	if (! DXQueryGridPositions(pA, &sd, counts, origin, deltas))
	    goto error;

	/*
	 * Squeeze out unnecessary axes
	 */
	for (i = j = 0; i < sd; i++)
	{
	    if (counts[i] != 1)
	    {
		if (i != j)
		{
		    counts[j] = counts[i];
		    for (k = 0; k < sd; k++)
			deltas[j*sd+k] = deltas[i*sd+k];
		}

		j++;
	    }
	}

	for (j = j; j < sd; j++)
	{
	    counts[j] = 1;
	    for (k = 0; k < sd; k++)
		deltas[j*sd+k] = 0.0;
	}
    }

    /*
     * Determine lengths of meaningful deltas
     */
    for (i = 0, d = deltas; i < ed; i++)
    {
	lengths[i] = 0.0;
	for (j = 0; j < sd; j++, d++)
	    lengths[i] += (*d)*(*d);
	lengths[i] = (counts[i]-1)*sqrt(lengths[i]);
    }

    if (nSamples > 1)
    {
	/*
	 * Determine allocation of samples along meaningful edges
	 */
	_Get_Counts(ed, nSamples, lengths, samples);
	    
	/* i is already == ed but lack of initialization makes some nervous */
	for (i=ed; i < sd; i++)
	    samples[i] = 1;
    }
    else
	for (i=0; i < sd; i++)
	    samples[i] = 1;

    /*
     * Scale deltas by ratio of number of original steps to number of 
     * grid steps.
     */
    for (i = 0; i < ed; i++)
    {
	float s = ((float)(counts[i] - 1)) / samples[i];
	for (j = 0; j < sd; j++)
	    deltas[i*sd+j] *= s;
    }

    /*
     * Add half a delta to the origin
     */
    for (i = 0; i < ed; i++)
    {
	for (j = 0; j < sd; j++)
	    origin[j] += (deltas[i*sd+j] / 2.0);
    }

    /*
     * flesh the deltas out so that the deltas matrix is square
     */
    for (i = ed; i < sd; i++)
	for (j = 0; j < sd; j++)
	    deltas[i*sd+j] = 0.0;

    grid = DXMakeGridPositionsV(sd, samples, origin, deltas);

    return grid;

error:
    return NULL;
}

static Error
invert(int n, float *in, float *out)
{

    switch(n)
    {
	case 3:
	{
	    if (! invert33(in, out))
		return ERROR;
		    
	    break;
	}


	case 2:
	{
	    float m;

	    m = in[0]*in[3] - in[1]*in[2];
	    if (m == 0)
		return ERROR;

	    m = 1.0 / m;
	    out[0] =  in[3]*m;
	    out[1] = -in[1]*m;
	    out[2] = -in[2]*m;
	    out[3] =  in[0]*m;

	    break;
	}

	case 1:
	{
	    if (in[0] == 0)
	    {
		DXSetError(ERROR_DATA_INVALID, "bad delta vectors");
		return ERROR;
	    }

	    out[0] = 1.0 / in[0];

	    break;
	}
    }

    return OK;
}

#if 0
static float
rand1()
{
    return ((float)(rand()&0x7fff)) / 0x7fff;
}
#endif

#define FIELD_TRIES	1024

static int 
FieldVolumeTask(Pointer ptr)
{
    Field field = (Field)ptr;

    if (-1.0 == _dxfFieldVolume(field))
	return ERROR;
    
    return OK;
    
}

static float StandardFieldVolume(Field);
static float PolylineFieldVolume(Field);
static float FaceFieldVolume(Field);

static float 
_dxfFieldVolume(Field field)
{
    Array         	   pArray, cArray, vArray = NULL;
    float 		   fvol = 0.0;

    if (DXEmptyField(field))
	goto done;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "missing positions");
	goto error;
    }

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (cArray)
    {
	if (DXQueryGridPositions(pArray, NULL, NULL, NULL, NULL) &&
	    DXQueryGridConnections(cArray, NULL, NULL) &&
	    ! DXGetComponentValue(field, "invalid connections"))
	{
	    fvol = _dxfBoxVolume((Object)field, NULL);
	}
	else 
	{
	    fvol = StandardFieldVolume(field);
	}
    }
    else if (DXGetComponentValue(field, "polylines"))
    {
	fvol = PolylineFieldVolume(field);
    }
    else if (DXGetComponentValue(field, "faces"))
    {
	fvol = PolylineFieldVolume(field);
    }
    else
    {
	InvalidComponentHandle invalids;

	invalids = DXCreateInvalidComponentHandle((Object)field,
					NULL, "positions");
	if (invalids == NULL)
	    goto error;
	
	fvol = DXGetValidCount(invalids);

	DXFreeInvalidComponentHandle(invalids);	invalids = NULL;
	goto done;
    }

done:

    vArray = (Array)DXNewConstantArray(1, (Pointer)&fvol,
				TYPE_FLOAT, CATEGORY_REAL, 0);
    if (! vArray)
	goto error;
	
    if (! DXSetAttribute((Object)field, "volume", (Object)vArray))
	goto error;
	
    return fvol;

error:
    DXDelete((Object)vArray);

    return -1;
}

static float
StandardFieldVolume(Field field)
{
    Array         	   pArray, cArray, vArray = NULL;
    InvalidComponentHandle invalids = NULL;
    ArrayHandle   	   pHandle = NULL, cHandle = NULL;
    float 		   fvol = 0.0;
    int           	   pDim, nCon;
    int           	   i, j, n;
    static float  	   (*metric)(float **, int);
    int           	   eltSize;

    if (DXEmptyField(field))
	goto done;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "missing positions");
	goto error;
    }

    cArray = (Array)DXGetComponentValue(field, "connections");

    DXGetArrayInfo(pArray, NULL, NULL, NULL, NULL, &pDim);
    DXGetArrayInfo(cArray, &nCon, NULL, NULL, NULL, &eltSize);

    pHandle = DXCreateArrayHandle(pArray);
    cHandle = DXCreateArrayHandle(cArray);
    if (! pHandle || !cHandle)
	goto error;
    
    if (DXGetComponentValue(field, "invalid connections"))
    {
	invalids = DXCreateInvalidComponentHandle((Object)field,
							    NULL, "connections");
	if (! invalids)
	    goto error;
    }

    metric = _dxfGetVolumeMethod(field);
    if (! metric)
	goto error;

    fvol = 0.0; n = 0;
    for (i = 0; i < nCon; i++)
	if (!invalids || DXIsElementValid(invalids, i))
	{
	    int   *elt, eBuf[32];
	    float *pts[32], pBuf[32][3];

	    n ++;

	    elt = (int *)DXGetArrayEntry(cHandle, i, (Pointer)eBuf);

	    for (j = 0; j < eltSize; j++)
		pts[j] = (float *)DXGetArrayEntry(pHandle, 
					    elt[j], (Pointer)&pBuf[j][0]);
		
	    fvol += (*metric)(pts, pDim);
	}

    DXFreeArrayHandle(pHandle);	        pHandle = NULL;
    DXFreeArrayHandle(cHandle);	        cHandle = NULL;
    DXFreeInvalidComponentHandle(invalids);	invalids = NULL;

done:
    return fvol;

error:
    if (! pHandle)  DXFreeArrayHandle(pHandle);
    if (! cHandle)  DXFreeArrayHandle(cHandle);
    if (! invalids) DXFreeInvalidComponentHandle(invalids);
    DXDelete((Object)vArray);

    return -1;
}

static float 
PolylineFieldVolume(Field field)
{
    Array pArray, plArray, eArray;
    int   i, j, pDim;
    float buf0[32], buf1[32];
    float *ptrs[2];
    int   nPolylines, nEdges, *polylines, *edges;
    ArrayHandle pHandle;
    float fvol;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
    {
	DXSetError(ERROR_MISSING_DATA, "missing positions");
	goto error;
    }

    plArray = (Array)DXGetComponentValue(field, "polylines");
    if (! plArray)
    {
	DXSetError(ERROR_MISSING_DATA, "missing polylines");
	goto error;
    }

    eArray = (Array)DXGetComponentValue(field, "edges");
    if (! eArray)
    {
	DXSetError(ERROR_MISSING_DATA, "polylines but no edges");
	goto error;
    }

    DXGetArrayInfo(plArray, &nPolylines, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eArray, &nEdges, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(pArray, NULL, NULL, NULL, NULL, &pDim);

    polylines = (int *)DXGetArrayData(plArray);
    edges     = (int *)DXGetArrayData(eArray);

    pHandle = DXCreateArrayHandle(pArray);

    for (i = 0, fvol = 0.0; i < nPolylines; i++)
    {
	int start = polylines[i];
	int end   = (i == nPolylines-1) ? nEdges : polylines[i+1];
	int knt   = (end - start) - 1;
	int *seg  = edges + start;

	ptrs[1] = (float *)DXGetArrayEntry(pHandle, *seg, buf1);
	
	for (j = 0, seg++; j < knt; j++, seg++)
	{
	    ptrs[0] = ptrs[1];
	    ptrs[1] = (float *)DXGetArrayEntry(pHandle, *seg, 
			    (j & 0x1) ? buf1 : buf0);
	    
	    fvol += LineLength(ptrs, pDim);
	}
    }

    pHandle = NULL;

    return fvol;

error:
    return -1;
}

static float
FaceFieldVolume(Field field)
{
    Array faces, loops, edges, positions;
    int   *faceData, *loopData, *edgeData;
    ArrayHandle phandle = NULL;
    int nFaces, nLoops, nEdges, nDim;
    float area = 0;
    Type t;
    Category c;
    int rank, shape[32];
    int face;
    InvalidComponentHandle ich = NULL;
    Array   normals = NULL, areas = NULL;
    Vector *normData, *fn;
    float  *areaData, *fa;

    if (DXEmptyField(field))
	goto done;
    
    faces = (Array)DXGetComponentValue(field, "faces");
    if (! faces)
    {
	DXSetError(ERROR_MISSING_DATA, "faces");
	goto error;
    }

    DXGetArrayInfo(faces, &nFaces, &t, &c, &rank, shape);

    if (nFaces == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"faces must be real integer scalars");
	goto error;
    }

    loops = (Array)DXGetComponentValue(field, "loops");
    if (! loops)
    {
	DXSetError(ERROR_MISSING_DATA, "loops");
	goto error;
    }

    DXGetArrayInfo(loops, &nLoops, &t, &c, &rank, shape);

    if (nLoops == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"loops must be real integer scalars");
	goto error;
    }

    edges = (Array)DXGetComponentValue(field, "edges");
    if (! edges)
    {
	DXSetError(ERROR_MISSING_DATA, "edges");
	goto error;
    }

    DXGetArrayInfo(edges, &nEdges, &t, &c, &rank, shape);

    if (nEdges == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"edges must be real integer scalars");
	goto error;
    }

    positions = (Array)DXGetComponentValue(field, "positions");
    if (! positions)
    {
	DXSetError(ERROR_MISSING_DATA, "positions");
	goto error;
    }

    DXGetArrayInfo(positions, NULL, &t, &c, &rank, &nDim);

    if (t != TYPE_FLOAT || c != CATEGORY_REAL
	|| rank != 1 || (nDim != 2 && nDim != 3))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"positions must be real float 2- or 3-vectors");
	goto error;
    }

    phandle = DXCreateArrayHandle(positions);
    if (! phandle)
	goto error;

    normals = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! normals)
	goto error;
    
    if (! DXAddArrayData(normals, 0, nFaces, NULL))
	goto error;
    
    areas = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (! areas)
	goto error;
    
    if (! DXAddArrayData(areas, 0, nFaces, NULL))
	goto error;
    
    faceData = (int    *)DXGetArrayData(faces);
    loopData = (int    *)DXGetArrayData(loops);
    edgeData = (int    *)DXGetArrayData(edges);
    normData = (Vector *)DXGetArrayData(normals);
    areaData = (float  *)DXGetArrayData(areas);

    if (! DXSetComponentValue(field, "face normals", (Object)normals))
	goto error;
    normals = NULL;

    if (! DXSetComponentValue(field, "face areas", (Object)areas))
	goto error;
    areas = NULL;

    if (DXGetComponentValue(field, "invalid faces"))
    {
	ich = DXCreateInvalidComponentHandle((Object)field, NULL, "faces");
	if (ich == NULL)
	    goto error;
    }

    fn = normData;
    fa = areaData;
    for (face = 0; face < nFaces; face++, fn++, fa++)
    {
	int fLoop = faceData[face];
	int lLoop = (face == nFaces-1) ? nLoops : faceData[face+1];
	int loop;
	float l=0;

	if (ich && DXIsElementInvalid(ich, face))
	    continue;

	for (loop = fLoop; loop < lLoop; loop++)
	{
	    int fEdge = loopData[loop];
	    int lEdge = (loop == nLoops-1) ? nEdges : loopData[loop+1];

	    if (loop == fLoop)
	    {
		if (! _dxfLoopNormal(edgeData+fEdge, phandle,
					NULL, (lEdge-fEdge), nDim, fn))
		    goto error;

		l = *fa = vector_length(fn);
	    }
	    else
	    {
		Vector ln;

		if (! _dxfLoopNormal(edgeData+fEdge, phandle,
					NULL, (lEdge-fEdge), nDim, &ln))
		    goto error;

		if ((fn->x*ln.x + fn->y*ln.y + fn->z*ln.z) > 0)
		    *fa += vector_length(&ln);
		else
		    *fa -= vector_length(&ln);
	    }
	}

	*fa = fabs(*fa / 2);
	area += *fa;

	if (l > 0)
	    vector_scale((Vector *)fn, 1.0/l, (Vector *)fn);

    }

done:

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);

    return  area;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    DXFree((Object)normals);
    DXFree((Object)areas);
    return -1;
}


static Error
Cleanup_Sample_Field(Field field)
{
    DXDeleteComponent(field, "connections");
    DXSetAttribute((Object)field, "volume", NULL);
    return OK;
}

static float
_dxfBoxVolume(Object object, int *nd)
{
    Point boxPts[8], *b;
    int   i;
    Point min, max;
    float ext, vol;

    if (! DXBoundingBox(object, boxPts))
    {
	DXSetError(ERROR_DATA_INVALID, "unable to compute bounding box");
	goto error;
    }
    
    min.x = min.y = min.z =  DXD_MAX_FLOAT;
    max.x = max.y = max.z = -DXD_MAX_FLOAT;
    for (b = boxPts, i = 0; i < 8; i++, b++)
    {
	if (min.x > b->x) min.x = b->x;
	if (max.x < b->x) max.x = b->x;
	if (min.y > b->y) min.y = b->y;
	if (max.y < b->y) max.y = b->y;
	if (min.z > b->z) min.z = b->z;
	if (max.z < b->z) max.z = b->z;
    }

    vol = 1.0;
    i = 0;

    ext = max.x - min.x;
    if (ext > 0) { vol *= ext; i++; }

    ext = max.y - min.y;
    if (ext > 0) { vol *= ext; i++; }

    ext = max.z - min.z;
    if (ext > 0) { vol *= ext; i++; }

    if (nd) *nd = i;

    return vol;

error:
    return -1.0;
}


typedef struct
{
    float x, X, y, Y;
} MinMax;

static int 
InsideFace(float  x,	  float y,
	   int    face,   float *positions,
	   int    *faces, int   nF,
	   int    *loops, int   nL,
	   int    *edges, int   nE,
	   MinMax *loopMM)
{
    int     l;
    int     ls = faces[face];
    int     le = (face == nF-1) ? nL : faces[face+1];
    MinMax *lmm;
    int     knt = 0;

    for (l = ls, lmm = loopMM + ls; l < le; l++, lmm++)
    {
	int     e;
	int     es = loops[l];
	int     ee = (l == nL-1) ? nE : loops[l+1];

	if (x >= lmm->x && x <= lmm->X && 
	    y >= lmm->y && y <= lmm->Y)
	{
	    int    q = edges[es];
	    float *ppos, *qpos = positions+(q<<1);
	    float *t, *b, intercept, d;

	    for (e = es; e < ee; e++)
	    {
		/*p    = q;*/
		ppos = qpos;

		q    = (e == ee-1) ? edges[es] : edges[e+1];
		qpos = positions+(q<<1);

		if      (qpos[1] > ppos[1]) t = qpos, b = ppos;
		else if (qpos[1] < ppos[1]) t = ppos, b = qpos;
		else continue;

		if (t[1] <  y) continue;
		if (b[1] >= y) continue;
		if (t[0] < x && b[0] < x) continue;
		if (t[0] > x && b[0] > x)
		{
		    knt++;
		    continue;
		}

		d = (y - t[1]) / (b[1] - t[1]);
		intercept = t[0] + d*(b[0] - t[0]);

		if (intercept > x) knt++;
	    }
	}
    }

    return knt & 0x1;
}

static Error
FacesSample(Field field, int n, Array grid)
{
    Array   pArray, fArray, lArray, eArray, nArray, aArray;
    int     nP, nF, nL, nE, planar;
    float   *positions2D = NULL, *positions3D;
    float   *positions, *fareas;
    int     *faces, *loops, *edges;
    Vector  *fnorms;
    int     i, f, l, e, nDim;
    float   A=0, B=0, C=0, D=0;
    SegList *pList = NULL, *iList = NULL;
    int     axis=0;
    InvalidComponentHandle ich = NULL;
    float   totArea=0;
    MinMax  *loopMM = NULL;
    MinMax  *faceMM = NULL;
    MinMax  minmax;

    if (DXEmptyField(field)) 
	goto empty;

    if (! DXGetComponentValue(field, "face normals"))
	if ((totArea = FaceFieldVolume(field)) < 0.0)
	   goto error;

    pArray = (Array)DXGetComponentValue(field, "positions");
    fArray = (Array)DXGetComponentValue(field, "faces");
    lArray = (Array)DXGetComponentValue(field, "loops");
    eArray = (Array)DXGetComponentValue(field, "edges");
    nArray = (Array)DXGetComponentValue(field, "face normals");
    aArray = (Array)DXGetComponentValue(field, "face areas");
    if (!pArray || !fArray || !lArray || !eArray || !nArray || !aArray)
	goto error;

    DXGetArrayInfo(pArray, &nP, NULL, NULL, NULL, &nDim);
    DXGetArrayInfo(fArray, &nF, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(lArray, &nL, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eArray, &nE, NULL, NULL, NULL, NULL);

    positions = (float  *)DXGetArrayData(pArray);
    faces     = (int    *)DXGetArrayData(fArray);
    loops     = (int    *)DXGetArrayData(lArray);
    edges     = (int    *)DXGetArrayData(eArray);
    fnorms    = (Vector *)DXGetArrayData(nArray);
    fareas    = (float  *)DXGetArrayData(aArray);

    loopMM = (MinMax *)DXAllocate(nL*sizeof(MinMax));
    faceMM = (MinMax *)DXAllocate(nF*sizeof(MinMax));
    if (! loopMM || ! faceMM)
	goto error;

    if (DXGetComponentValue(field, "invalid faces"))
    {
	ich = DXCreateInvalidComponentHandle((Object)field, NULL, "faces");
	if (! ich)
	    goto error;
    }

    /*
     * If the dimensionality is three, we look to see if the
     * faces all lie in the sample plane, since if using a 
     * single grid of prospective samples will result in a nice
     * even distribution of samples.
     */
    if (nDim == 3)
    {
	Vector *f0 = NULL;
	Vector *fn = fnorms;
        int firstValid=0;

	/*
	 * They are all planar if the face normals are very close
	 * to collinear.
	 */
	for (f = 0, planar = 1; f < nF && planar; f++, fn++)
	{
	    if (ich && DXIsElementInvalid(ich, f))
		continue;

	    if (f0)
	    {
		if ((f0->x*fn->x + f0->y*fn->y + f0->z*fn->z) < 0.95)
		    planar = 0;
	    }
	    else
	    {
		f0 = fn;
		firstValid = f;
	    }
	}

	if (! f0)
	    goto empty;
	
	/*
	 * If they are coplanar, then project all the positions into the
	 * most perpendicular 2-D plane.
	 */
	if (planar)
	{
	    float *p2d, *p3d;

	    positions2D = (float *)DXAllocate(2 * nP * sizeof(float));
	    if (! positions2D)
		goto error;
	    
	    p2d = positions2D;
	    p3d = (float *)positions;
	    
	    if (fabs(f0->x) > fabs(f0->y) && fabs(f0->x) > fabs(f0->z))
	    {
		axis = 0;
		for (i = 0; i < nP; i++)
		{
		    ++p3d;
		    *p2d++ = *p3d++;
		    *p2d++ = *p3d++;
		}
	    }
	    else if (fabs(f0->y) > fabs(f0->x) && fabs(f0->y) > fabs(f0->z))
	    {
		axis = 1;
		for (i = 0; i < nP; i++)
		{
		    *p2d++ = *p3d++;
		    ++p3d;
		    *p2d++ = *p3d++;
		}
	    }
	    else
	    {
		axis = 2;
		for (i = 0; i < nP; i++)
		{
		    *p2d++ = *p3d++;
		    *p2d++ = *p3d++;
		    ++p3d;
		}
	    }
	    
	    positions3D = positions;
	    positions   = positions2D;

	    
	    /*
	     * Determine the planar equation of the 3-D plane
	     * using the normal to the first valid plane and
	     * one point on that plane.
	     */
	    p3d = positions3D + 3*edges[loops[faces[firstValid]]];
	    f0  = fnorms + firstValid;
	    A = f0->x;
	    B = f0->y;
	    C = f0->z;
	    D = -(A*p3d[0] + B*p3d[1] + C*p3d[2]);

	}
    }
    else if (nDim == 2)
    {
	planar = 1;
    }
    else
    {
	DXSetError(ERROR_NOT_IMPLEMENTED,
	    "Sample on faces/loops/edges data that is %d-D", nDim);
	goto error;
    }

    pList = DXNewSegList(nDim*sizeof(float), n, n);
    iList = DXNewSegList(sizeof(int), n, n);
    if (! pList || ! iList)
	goto error;

    /*
     * If the faces are coplanar, we use a single grid of prospective
     * sample points for all the faces
     */
    if (planar)
    {
	float xSize, ySize, dx, dy, ox, oy, x, y;
	int   nx, ny, ix, iy, xstart, ystart, xend, yend;
	MinMax *fmm, *lmm;

	minmax.x =  DXD_MAX_FLOAT, minmax.X = -DXD_MAX_FLOAT;
	minmax.y =  DXD_MAX_FLOAT, minmax.Y = -DXD_MAX_FLOAT;

	/*
	 * Determine the minmax of the set of faces, each face,
	 * and each loop.
	 */
	for (f = 0, fmm = faceMM; f < nF; f++, fmm++)
	{
	    int lstart = faces[f];
	    int lend   = (f == nF-1) ? nL : faces[f+1];

	    fmm->x = DXD_MAX_FLOAT, fmm->X = -DXD_MAX_FLOAT;
	    fmm->y = DXD_MAX_FLOAT, fmm->Y = -DXD_MAX_FLOAT;

	    if (ich && DXIsElementInvalid(ich, f))
		continue;

	    for (l = lstart, lmm = loopMM + lstart; l < lend; l++, lmm++)
	    {
		int estart = loops[l];
		int eend   = (l == nL-1) ? nE: loops[l+1];

		lmm->x = DXD_MAX_FLOAT, lmm->X = -DXD_MAX_FLOAT;
		lmm->y = DXD_MAX_FLOAT, lmm->Y = -DXD_MAX_FLOAT;

		for (e = estart; e < eend; e++)
		{
		    float *p = positions + 2*edges[e];
		    if (*p < lmm->x) lmm->x = *p;
		    if (*p > lmm->X) lmm->X = *p;
		    p++;
		    if (*p < lmm->y) lmm->y = *p;
		    if (*p > lmm->Y) lmm->Y = *p;
		}

		if (lmm->x < fmm->x) fmm->x = lmm->x;
		if (lmm->X > fmm->X) fmm->X = lmm->X;
		if (lmm->y < fmm->y) fmm->y = lmm->y;
		if (lmm->Y > fmm->Y) fmm->Y = lmm->Y;
	    }

	    if (fmm->x < minmax.x) minmax.x = fmm->x;
	    if (fmm->X > minmax.X) minmax.X = fmm->X;
	    if (fmm->y < minmax.y) minmax.y = fmm->y;
	    if (fmm->Y > minmax.Y) minmax.Y = fmm->Y;
	}

	/*
	 * Determine the x and y extents of the bounding box
	 */
	xSize = minmax.X - minmax.x;
	ySize = minmax.Y - minmax.y;

	/*
	 * scale the number of samples by the ratio of the area of
	 * the bounding box to that covered by the faces
	 */
	n *= ((xSize * ySize) / totArea);

	/*
	 * allocate samples in rows and columns based on the aspect 
	 * ratio of the bounding box.
	 */
	nx = sqrt(n*xSize/ySize);
	ny = n / nx;

	/*
	 * determine the grid origin and steps
	 */
	if (nx <= 0) dx = xSize, nx = 1;
	else         dx = xSize / nx;

	if (ny <= 0) dy = ySize, ny = 1;
	else         dy = ySize / ny;

	ox = minmax.x + dx/2;
	oy = minmax.y + dx/2;

	/*
	 * for each face, determine which samples it covers
	 */
	for (f = 0, fmm = faceMM; f < nF; f++, fmm++)
	{
	    float   fx, fy;

	    if (ich && DXIsElementInvalid(ich, f))
		continue;

	    /*
	     * Get the range of prospective sample points that
	     * are covered by the bounding box of the face
	     */
	    xstart = (fmm->x - ox) / dx;
	    if (ox+xstart*dx < fmm->x) xstart ++;
	    xend   = (fmm->X - ox) / dx;

	    ystart = (fmm->y - oy) / dy;
	    if (oy+ystart*dy < fmm->y) ystart ++;
	    yend   = (fmm->Y - oy) / dy;

	    if (xstart < 0) xstart = 0;
	    if (ystart < 0) ystart = 0;
	    if (xend >= nx) xend = nx-1;
	    if (yend >= ny) yend = ny-1;

	    fx = ox + xstart*dx;
	    fy = oy + ystart*dy;

	    /*
	     * For each prospective point covered by the bounding box 
	     * of the face, see if its inside the face.
	     */
	    for (x = fx, ix = xstart; ix <= xend; ix++, x += dx)
		for (y = fy, iy = ystart; iy <= yend; iy++, y += dy)
		    if (InsideFace(x, y, f, positions, faces, nF,
				loops, nL, edges, nE, loopMM))
		    {
			float  *pptr = (float *)DXNewSegListItem(pList);
			Interp *iptr = (Interp *)DXNewSegListItem(iList);

			if (nDim == 2)
			{
			    *pptr++ = x;
			    *pptr++ = y;
			}
			else
			{
                            float x3d=0, y3d=0, z3d=0;

			    switch(axis)
			    {
				case 0:
				    y3d = x;
				    z3d = y;
				    x3d = -(B*y3d + C*z3d + D) / A;
				    break;
				case 1: 
				    x3d = x;
				    z3d = y;
				    y3d = -(A*x3d + C*z3d + D) / B;
				    break;
				case 2:
				    x3d = x;
				    y3d = y;
				    z3d = -(A*x3d + B*y3d + D) / C;
				    break;
			    }
			    *pptr++ = x3d;
			    *pptr++ = y3d;
			    *pptr++ = z3d;
			}

			iptr->index = f;
		    }
	}
    }
    else
    {
	/*
	 * Non-planar case.  Determine a grid for each face
	 * independently.
	 */
	float xSize, ySize, dx, dy, ox, oy, x, y;
	int   nx, ny, ix, iy;
	MinMax *fmm, *lmm;

	positions3D = positions;

	positions2D = (float *)DXAllocate(2 * nP * sizeof(float));
	if (! positions2D)
	    goto error;

	for (f = 0, fmm = faceMM; f < nF; f++, fmm++)
	{
	    int     lstart = faces[f];
	    int     lend   = (f == nF-1) ? nL : faces[f+1];
	    int	    nface;
	    Vector  *fn = fnorms + f;
	    float   *p0;

	    if (ich && DXIsElementInvalid(ich, f))
		continue;

	    /*
	     * Determine the best axis of projection for the face
	     */
	    if (fabs(fn->x) > fabs(fn->y) && fabs(fn->x) > fabs(fn->z))
		axis = 0;
	    else if (fabs(fn->y) > fabs(fn->x) && fabs(fn->y) > fabs(fn->z))
		axis = 1;
	    else
		axis = 2;

	    /* 
	     * project the points used by this face into 2-D.  As
	     * this is being done, acquire the loop and face 2-D
	     * minmax.
	     */
	    fmm->x = DXD_MAX_FLOAT, fmm->X = -DXD_MAX_FLOAT;
	    fmm->y = DXD_MAX_FLOAT, fmm->Y = -DXD_MAX_FLOAT;
	    for (l = lstart, lmm = loopMM + lstart; l < lend; l++, lmm++)
	    {
		int estart = loops[l];
		int eend   = (l == nL-1) ? nE : loops[l+1];
		int e;
		float *p2d, *p3d;

		lmm->x = DXD_MAX_FLOAT, lmm->X = -DXD_MAX_FLOAT;
		lmm->y = DXD_MAX_FLOAT, lmm->Y = -DXD_MAX_FLOAT;

		for (e = estart; e < eend; e++)
		{
		    p2d = positions2D + 2*edges[e];
		    p3d = positions3D + 3*edges[e];

		    switch (axis)
		    {
			case 0: p2d[0] = p3d[1]; p2d[1] = p3d[2]; break;
			case 1: p2d[0] = p3d[0]; p2d[1] = p3d[2]; break;
			case 2: p2d[0] = p3d[0]; p2d[1] = p3d[1]; break;
		    }

		    if (p2d[0] < lmm->x) lmm->x = p2d[0];
		    if (p2d[0] > lmm->X) lmm->X = p2d[0];
		    if (p3d[1] < lmm->y) lmm->y = p3d[1];
		    if (p3d[1] > lmm->Y) lmm->Y = p3d[1];
		}

		if (lmm->x < fmm->x) fmm->x = lmm->x;
		if (lmm->X > fmm->X) fmm->X = lmm->X;
		if (lmm->y < fmm->y) fmm->y = lmm->y;
		if (lmm->Y > fmm->Y) fmm->Y = lmm->Y;
	    }

	    /*
	     * Resolve the planar equation based on the face normal
	     * and one vertex of the face.
	     */
	    p0 = positions3D + 3*edges[loops[faces[f]]];
	    A = fn->x;
	    B = fn->y;
	    C = fn->z;
	    D = -(A*p0[0] + B*p0[1] + C*p0[2]);

	    /*
	     * Determine the proportion of the samples that
	     * should lie in this face.
	     */
	    nface = n * (fareas[f] / totArea);
	    if (nface <= 0)
		continue;
	    
	    /*
	     * determine the 2-D extents of the bounding box of the
	     * 2-D projected face.
	     */
	    xSize = fmm->X - fmm->x;
	    ySize = fmm->Y - fmm->y;

	    /*
	     * scale the sample count by the ratio of the total area
	     * of the bounding box to the area of the face.
	     */
	    nface *= ((xSize * ySize) / fareas[f]);
	    if (nface == 0)
		continue;

	    /*
	     * allocate the prospective samples in rows and columns.
	     */
	    nx = sqrt(nface*xSize/ySize);
	    ny = nface / nx;

	    /* 
	     * get the origin and deltas of the prospective sample grid
	     */
	    if (nx <= 0) dx = xSize, nx = 1;
	    else         dx = xSize / nx;

	    if (ny <= 0) dy = ySize, ny = 1;
	    else         dy = ySize / ny;

	    ox = fmm->x + dx/2;
	    oy = fmm->y + dx/2;

	    for (x = ox, ix = 0; ix < nx; ix++, x += dx)
		for (y = oy, iy = 0; iy < ny; iy++, y += dy)
		    if (InsideFace(x, y, f, positions2D, faces, nF,
				loops, nL, edges, nE, loopMM))
		    {
			float  *pptr = (float *)DXNewSegListItem(pList);
			Interp *iptr = (Interp *)DXNewSegListItem(iList);

			if (nDim == 2)
			{
			    *pptr++ = x;
			    *pptr++ = y;
			}
			else
			{
			    float x3d=0, y3d=0, z3d=0;

			    switch(axis)
			    {
				case 0:
				    y3d = x;
				    z3d = y;
				    x3d = -(B*y3d + C*z3d + D) / A;
				    break;
				case 1: 
				    x3d = x;
				    z3d = y;
				    y3d = -(A*x3d + C*z3d + D) / B;
				    break;
				case 2:
				    x3d = x;
				    y3d = y;
				    z3d = -(A*x3d + B*y3d + D) / C;
				    break;
			    }
			    *pptr++ = x3d;
			    *pptr++ = y3d;
			    *pptr++ = z3d;
			}

			iptr->index = f;
		    }
	}
    }

    if (pList && DXGetSegListItemCount(pList) != 0)
    {
        if (! CreateOutputFaces(field, pList, iList))
	    goto error;
	
	goto done;
    }

empty:

    {
	char *name;

	while (DXGetEnumeratedComponentValue(field, 0, &name))
	    DXDeleteComponent(field, name);
    }

done:

    if (ich)         DXFreeInvalidComponentHandle(ich);
    if (iList)       DXDeleteSegList(iList);
    if (pList)       DXDeleteSegList(pList);
    if (loopMM)      DXFree((Pointer)loopMM);
    if (faceMM)      DXFree((Pointer)faceMM);
    if (positions2D) DXFree((Pointer)positions2D);

    return OK;

error:

    if (ich)         DXFreeInvalidComponentHandle(ich);
    if (iList)       DXDeleteSegList(iList);
    if (pList)       DXDeleteSegList(pList);
    if (loopMM)      DXFree((Pointer)loopMM);
    if (faceMM)      DXFree((Pointer)faceMM);
    if (positions2D) DXFree((Pointer)positions2D);
    
    return ERROR;

}

static Field
CreateOutputFaces(Field f, SegList *pList, SegList *iList)
{
    Array          sA, dA = NULL;
    char           *dptr;
    SegListSegment *slist;
    char           *name, *toDelete[100];
    int            nV = 2, nDelete = 0;
    int            i, nSamples = DXGetSegListItemCount(pList);
    ArrayHandle    ah = NULL;
    Pointer	   buf0 = NULL, buf1 = NULL;

    DXDeleteComponent(f, "faces");
    DXDeleteComponent(f, "loops");
    DXDeleteComponent(f, "edges");
    DXDeleteComponent(f, "face normals");
    DXDeleteComponent(f, "face areas");

    i = 0;
    while(NULL != (sA = (Array)DXGetEnumeratedComponentValue(f, i++, &name)))
    {
	Object   at;
	Type     t; 
	Category c;
	int      r, shape[32];
	int      itemSize;

	if (! strcmp(name, "positions"))
	    continue;

	if (DXGetComponentAttribute(f, name, "ref") ||
	    DXGetComponentAttribute(f, name, "der") || 
	    !strncmp(name, "invalid", 8) 	    ||
	    NULL == (at = DXGetComponentAttribute(f, name, "dep")))
	{
	    toDelete[nDelete++] = name;
	    continue;
	}

	if (DXGetObjectClass(at) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
	    goto error;
	}

	DXGetArrayInfo(sA, NULL, &t, &c, &r, shape);
	
	itemSize   = DXGetItemSize(sA);
	/*itemValues = itemSize / DXTypeSize(t);*/

	if (DXQueryConstantArray(sA, NULL, NULL))
	{
	    dA = (Array)DXNewConstantArrayV(DXGetSegListItemCount(pList),
				DXGetConstantArrayData(sA), t, c, r, shape);
	    if (! dA)
		goto error;
	}
	else
	{
	    /*nItems = DXGetItemSize(sA) / DXTypeSize(t);*/

	    dA = DXNewArrayV(t, c, r, shape);
	    if (! dA)
		goto error;
	
	    if (! DXAddArrayData(dA, 0, nSamples, NULL))
		goto error;

	    ah = DXCreateArrayHandle(sA);
	    if (! ah)
		goto error;
	    
	    buf0 = DXAllocate(DXGetItemSize(sA));
	    buf1 = DXAllocate(DXGetItemSize(sA));
	    if (! buf0 || ! buf1)
		goto error;

	    if (! strcmp(DXGetString((String)at), "faces"))
	    {
		char *dst;
		PolylineInterp *iPtr;

		dst = (char *)DXGetArrayData(dA);

		DXInitGetNextSegListItem(iList);
		while (NULL != 
			(iPtr = (PolylineInterp *)DXGetNextSegListItem(iList)))
		{
		    memcpy(dst, DXGetArrayEntry(ah, iPtr->index, buf0), itemSize);
		    dst += itemSize;
		}
	    }
	    else
	    {
		DXSetError(ERROR_BAD_CLASS, "%s dependency attribute", name);
		goto error;
	    }
	}

	DXFree(buf0); buf0 = NULL;
	DXFree(buf1); buf1 = NULL;
	DXFreeArrayHandle(ah); ah = NULL;

	if (! DXSetComponentValue(f, name, (Object)dA))
	    goto error;
	
	if (! DXSetComponentAttribute(f, name, "dep", 
				(Object)DXNewString("positions")))
	    goto error;
    }

    for (i = 0; i < nDelete; i++)
	DXDeleteComponent(f, toDelete[i]);

    sA = (Array)DXGetComponentValue(f, "positions");
    if (! sA)
	goto error;
    
    if (! DXGetArrayInfo(sA, NULL, NULL, NULL, NULL, &nV))
	goto error;
    
    dA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nV);
    if (! dA)
	goto error;
    
    if (! DXAddArrayData(dA, 0, DXGetSegListItemCount(pList), NULL))
	goto error;
    
    dptr = (char *)DXGetArrayData(dA);
    DXInitGetNextSegListSegment(pList);
    while (NULL != (slist = DXGetNextSegListSegment(pList)))
    {
	int size = DXGetSegListSegmentItemCount(slist) * nV * sizeof(float);

	memcpy(dptr, DXGetSegListSegmentPointer(slist), size);
	dptr += size;
    }

    if (! DXSetComponentValue(f, "positions", (Object)dA))
	goto error;
    
    return f;

error:
    DXFree(buf0);
    DXFree(buf1);
    DXFree(ah);
    return NULL;
}


/* for a field or multigrid/composite field, if there are no 
 * connections then Sample returns a subset of the existing
 * points.  generate a warning if the number of samples requested
 * exceeds the number of positions in the input.
 */
static void
_Check_Sample_Count(Object o, int n)
{
    Field               field;
    Object		con;
    int			vCount = 0;
    int			i;
    Class 		objclass;
    InvalidComponentHandle ich = NULL;


    objclass = DXGetObjectClass(o);
    if (objclass == CLASS_GROUP)
	objclass = DXGetGroupClass((Group)o);


    switch (objclass) {
      case CLASS_FIELD:
	field = (Field)o;
	if (DXEmptyField(field))
	    return;

	/* if there are connections, with interpolation we can
         * generate as many points as they want.
	 */
	con = DXGetComponentValue(field, "connections");
	if (! con)
	    con = DXGetComponentValue(field, "faces");
	if (! con)
	    con = DXGetComponentValue(field, "polylines");
	if (con)
	    return;
	
	ich = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
	if (! ich)
	    return;
	
	vCount = DXGetValidCount(ich);
	if (n > vCount)
	    DXWarning("cannot generate %d sample points; input contains %d valid positions",
		      n, vCount);
	
	return;
	    
      case CLASS_MULTIGRID:
      case CLASS_COMPOSITEFIELD:
	for (i=0; (field=(Field)DXGetEnumeratedMember((Group)o, i, NULL)); i++)
        {
	    if (DXEmptyField(field))
		continue;

	    /* if there are connections, with interpolation we can
	     * generate as many points as they want.
	     */
	    con = DXGetComponentValue(field, "connections");
	    if (! con)
		con = DXGetComponentValue(field, "faces");
	    if (! con)
		con = DXGetComponentValue(field, "polylines");
	    if (con)
		return;
	
	    ich = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
	    if (! ich)
		return;
	    
	    vCount += DXGetValidCount(ich);
	}
	
	if (n > vCount)
	    DXWarning("cannot generate %d sample points; input contains %d valid positions",
		      n, vCount);
	
	return;

      default:
        return;
	
    }
}

