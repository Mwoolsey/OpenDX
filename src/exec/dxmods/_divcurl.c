/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/_divcurl.c,v 1.6 2006/01/04 22:00:50 davidt Exp $
 */


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "_divcurl.h"

static Object _dxfDivCurlObject(Object, int);
static Error  _dxfDivCurlField(Field, int);
static Error  _dxfDivCurlTask(Pointer ptr);

static Error  _dxfDivCurlRegular(Field, int *, float *, int);
static Error  _dxfDivCurlCubesRegular(Field, int *, float *);

static Error  _dxfDivCurlIrregular(Field);

static Error  _dxfRecursiveSetGroupTypeV(Object, Type, Category, int, int *);

static void  _dxfTetrasDivCurl(ArrayHandle, Pointer, Type, float *,
					float *, byte *, int *);
static void  _dxfCubesDivCurl(ArrayHandle, Pointer, Type, float *,
					float *, byte *, int *);

typedef struct {
    Field field;
    int   flags;
} Task;

#define DO_DIV	0x01
#define DO_CURL 0x02

#define TRUE	1
#define FALSE	0

Object
_dxfDivCurl(Object object, Object *div, Object *curl)
{
    int flags = 0;
    Object copy = NULL;

    if (div)
	flags |= DO_DIV;

    if (curl)
	flags |= DO_CURL;

    copy = DXCopy(object, COPY_STRUCTURE);
    if (! copy)
	goto error;

    if (! _dxfDivCurlObject(copy, flags))
	goto error;

    if (div)
    {
	*div = DXCopy(copy, COPY_STRUCTURE);
	if (*div == NULL)
	    goto error;
	
	if (! DXRename(*div, "div", "data"))
	    goto error;
	
	if (curl)
	    if (! DXRemove(*div, "curl"))
		goto error;
    }
	
    if (curl)
    {
	*curl = DXCopy(copy, COPY_STRUCTURE);
	if (*curl == NULL)
	    goto error;
	
	if (! DXRename(*curl, "curl", "data"))
	    goto error;
	
	if (div)
	    if (! DXRemove(*curl, "div"))
		goto error;
    }
    
    if (copy != object)
	DXDelete((Object)copy);

    return object;

error:
    if (div && *div)
    {
	DXDelete(*div);
	*div = NULL;
    }

    if (curl && *curl)
    {
	DXDelete(*curl);
	*curl = NULL;
    }

    if (copy != object)
	DXDelete(copy);
    return NULL;
}

static Object
_dxfDivCurlObject(Object object, int flags)
{
    int i;
    Field field;
    Object child;
    Class class;
    Task task;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);
    
    switch (class)
    {
	case CLASS_FIELD:
	    if (! _dxfDivCurlField((Field)object, flags))
		return NULL;
	    else
		return object;
	
	case CLASS_COMPOSITEFIELD:

	    if (! DXGrow(object, 1, NULL, "data", NULL))
		return NULL;

	    DXCreateTaskGroup();

	    task.flags = flags;

	    i = 0;
	    while(NULL != (field = DXGetPart(object, i++)))
	    {
		task.field = field;
		if (! DXAddTask(_dxfDivCurlTask, (Pointer)&task, sizeof(Task), 1.0))
		{
		    DXAbortTaskGroup();
		    return NULL;
		}
	    }
	    
	    if (!DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		return NULL;
	    
	    if (! DXShrink(object))
		return NULL;
    
	    return object;
	
	case CLASS_MULTIGRID:
	case CLASS_SERIES:
	case CLASS_GROUP:

	    i = 0; 
	    while (NULL != (child=DXGetEnumeratedMember((Group)object,i++,NULL)))
		if (! _dxfDivCurlObject(child, flags))
		    return NULL;
	    
	    return object;
	
	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)object, &child, 0))
		return NULL;
	    
	    if (! _dxfDivCurlObject(child, flags))
		return NULL;
	    else
		return object;
    
	case CLASS_CLIPPED:

	    if (! DXGetClippedInfo((Clipped)object, &child, 0))
		return NULL;
	    
	    if (! _dxfDivCurlObject(child, flags))
		return NULL;
	    else
		return object;
    
	default:
	    DXSetError(ERROR_DATA_INVALID, "#11381");
	    return NULL;
    }
}

static Error
_dxfDivCurlTask(Pointer ptr)
{
    Task *task = (Task *)ptr;

    return _dxfDivCurlField(task->field, task->flags);
}

static Error
_dxfDivCurlField(Field field, int flags)
{
    Type		type;
    Category		cat;
    int			dim, rank, shape[32];
    Array		positions;
    Array		connections;
    Array		data;
    float		del[9];
    int  		counts[3];
    int			grid;
    int			nPoints, pDim;
    Object		attr;

    if (DXEmptyField(field))
	return OK;

    data = (Array)DXGetComponentValue(field, "data");
    if (! data)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	return ERROR;
    }

    DXGetArrayInfo(data, NULL, &type, &cat, &rank, shape);
    if (cat != CATEGORY_REAL)
    {
	DXSetError (ERROR_DATA_INVALID, "#11150", "data");
	return ERROR;
    }

    if (rank != 1 || shape[0] != 3)
    {
	DXSetError (ERROR_DATA_INVALID, "#10230", "data", 3);
	return ERROR;
    }

    positions   = (Array)DXGetComponentValue(field, "positions");
    connections = (Array)DXGetComponentValue(field, "connections");

    DXGetArrayInfo(positions, &nPoints, NULL, NULL, NULL, &pDim);
    if (rank != 1 && shape[0] == pDim)
    {
	DXSetError (ERROR_DATA_INVALID, "#10230", "data", pDim);
	return ERROR;
    }

    attr = DXGetComponentAttribute(field, "data", "dep");
    if (! attr)
    {
	DXSetError(ERROR_DATA_INVALID, "#10255", "data", "dep");
	return ERROR;
    }
    else if (strcmp(DXGetString((String)attr), "positions"))
    {
	DXSetError(ERROR_DATA_INVALID, "#11251", "data");
	return ERROR;
    }
    
    DXReference(attr);

    /*
     * Set up div and curl arrays
     */
    if (flags & DO_DIV)
    {
	Array array;
	
	array = (Array)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	if (! array)
	    return ERROR;
	
	if (! DXAddArrayData(array, 0, nPoints, NULL))
	{
	    DXDelete((Object)array);
	    return ERROR;
	}

	if (! DXSetComponentValue(field, "div", (Object)array))
	{
	    DXDelete((Object)array);
	    return ERROR;
	}

	if (! DXSetComponentAttribute(field, "div", "dep", attr))
	    return ERROR;
    }

    if (flags & DO_CURL)
    {
	Array array;
	
	array = (Array)DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (! array)
	    return ERROR;
	
	if (! DXAddArrayData(array, 0, nPoints, NULL))
	{
	    DXDelete((Object)array);
	    return ERROR;
	}

	if (! DXSetComponentValue(field, "curl", (Object)array))
	{
	    DXDelete((Object)array);
	    return ERROR;
	}

	if (! DXSetComponentAttribute(field, "curl", "dep", attr))
	    return ERROR;
    }

    DXDelete(attr);

    /*
     * We need to ensure that there are no invalid positions referenced by	     * a valid connection element.
     */
    if (! DXInvalidateUnreferencedPositions((Object)field))
	return ERROR;
    
    /*
     * If the grid is regular and axis-aligned, we can use a faster method.
     * Unless there are any invalid positions.
     */
    if (DXQueryGridConnections(connections, NULL, NULL) &&
	DXQueryGridPositions(positions, &dim, counts, NULL, del) &&
	! DXGetComponentValue(field, "invalid positions"))
    {
	switch(dim)
	{
	    case 1:
		grid = 1;
		break;
	    
	    case 2:
		grid = del[0] != 0.0 && del[1] == 0.0 &&
		       del[2] == 0.0 && del[3] != 0.0;
		break;
	    
	    case 3:
		grid = del[0] != 0.0 && del[1] == 0.0 && del[2] == 0.0 &&
		       del[3] == 0.0 && del[4] != 0.0 && del[5] == 0.0 &&
		       del[6] == 0.0 && del[7] == 0.0 && del[8] != 0.0;
		break;
	    
	    default:
		grid = 0;
	}
    }
    else
	grid = 0;

    if (grid)
    {
	if (! _dxfDivCurlRegular(field, counts, del, dim))
	    return ERROR;
    }
    else
    {
	if (!  _dxfDivCurlIrregular(field))
	    return ERROR;
    }

    if (DXGetComponentValue(field, "original data"))
	DXDeleteComponent(field, "original data");

    return OK;
}

static Error
_dxfRecursiveSetGroupTypeV (Object parent, Type type, 
			    Category cat, int rank, int *shape)
{
    Object		child;
    int			i;

    if (DXGetObjectClass(parent) == CLASS_GROUP)
    {
	for (i = 0; 
	    NULL != (child = DXGetEnumeratedMember((Group) parent, i, NULL)); i++)
	{
	    if (! _dxfRecursiveSetGroupTypeV (child, type, cat, rank, shape))
		return ERROR;
	}

	if (! DXSetGroupTypeV ((Group) parent, type, cat, rank, shape))
	    return ERROR;
    }

    return (OK);
}

static Error
_dxfDivCurlRegular(Field field, int *counts, float *deltas, int nDim)
{
    Object attr;

    /*
     * Make sure data is dep on positions
     */
    attr = DXGetComponentAttribute(field, "data", "dep");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "#10255", "data", "dependency");
	return ERROR;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "#10200", "dependency attribute");
	return ERROR;
    }

    if (strcmp(DXGetString((String)attr), "positions"))
    {
	DXSetError(ERROR_DATA_INVALID, "#11251", "data");
	return ERROR;
    }

    switch(nDim)
    {
	case 3:
	    return _dxfDivCurlCubesRegular(field, counts, deltas);
	
	default:
	    DXSetError(ERROR_DATA_INVALID, "#10230", "positions", 3);
	    return ERROR;
    }
}

#define REG_CURL(dx, dy, dz, curl)				\
{								\
    curl[0] = (dy[2] - dz[1]);					\
    curl[1] = (dz[0] - dx[2]);					\
    curl[2] = (dx[1] - dy[0]);					\
}

#define REG_DIV(dx, dy, dz, div)				\
{								\
    *(div) = (dx[0] + dy[1] + dz[2]);				\
}

static Error
_dxfDivCurlCubesRegular(Field field, int *counts, float *deltas)
{
    float   		   dxInv = 1.0 / deltas[0];
    float   		   dyInv = 1.0 / deltas[4];
    float   		   dzInv = 1.0 / deltas[8];
    int     		   xKnt  = counts[0];
    int     		   yKnt  = counts[1];
    int     		   zKnt  = counts[2];
    float   		   *div, *curl;
    Array   		   inArray, dArray, cArray;
    Type    		   type;
    int     		   i, j, k;
    int     		   lx, ly, lz, nx, ny, nz;
    int	    		   nItems;

    inArray = (Array)DXGetComponentValue(field, "data");

    DXGetArrayInfo(inArray, &nItems, &type, NULL, NULL, NULL);

    dArray = (Array)DXGetComponentValue(field, "div");
    cArray = (Array)DXGetComponentValue(field, "curl");

    if (dArray)
    {
	div = (float *)DXGetArrayData(dArray);
	if (!div || DXGetError())
	    goto error;
    }
    else
	div = NULL;

    
    if (cArray)
    {
	curl = (float *)DXGetArrayData(cArray);
	if (!curl || DXGetError())
	    goto error;
    }
    else
	curl = NULL;
	

#define REG_CUBES_DIVCURL(type)						\
{									\
    type  *data, *last, *next;						\
    int   xSkip, ySkip, zSkip;						\
    float dx[3], dy[3], dz[3];						\
    float xStep, yStep, zStep;						\
									\
    data    = (type  *)DXGetArrayData(inArray);				\
    if (! data || DXGetError())						\
	goto error;							\
									\
    zSkip = 3;								\
    ySkip = zSkip * counts[2];						\
    xSkip = ySkip * counts[1];						\
									\
    for (i = 0; i < xKnt; i++)						\
    {									\
	xStep = dxInv;							\
	if (i == 0)							\
	{								\
	    lx = 0;							\
	    nx = xSkip;							\
	    xStep = dxInv;						\
	}								\
	else if (i == (xKnt - 1))					\
	{								\
	    lx = xSkip;							\
	    nx = 0;							\
	    xStep = dxInv;						\
	}								\
	else								\
	{								\
	    lx = xSkip;							\
	    nx = xSkip;							\
	    xStep = 0.5  * dxInv;					\
	}								\
									\
	for (j = 0; j < yKnt; j++)					\
	{								\
	    yStep = dyInv;						\
	    if (j == 0)							\
	    {								\
		ly = 0;							\
		ny = ySkip;						\
		yStep = dyInv;						\
	    }								\
	    else if (j == (yKnt - 1))					\
	    {								\
		ly = ySkip;						\
		ny = 0;							\
		yStep = dyInv;						\
	    }								\
	    else							\
	    {								\
		ly = ySkip;						\
		ny = ySkip;						\
		yStep = 0.5  * dyInv;					\
	    }								\
									\
	    for (k = 0; k < zKnt; k++)					\
	    {								\
		zStep = dzInv;						\
		if (k == 0)						\
		{							\
		    lz = 0;						\
		    nz = zSkip;						\
		    zStep = dzInv;					\
		}							\
		else if (k == (zKnt - 1))				\
		{							\
		    lz = zSkip;						\
		    nz = 0;						\
		    zStep = dzInv;					\
		}							\
		else							\
		{							\
		    lz = zSkip;						\
		    nz = zSkip;						\
		    zStep = 0.5  * dzInv;				\
		}							\
									\
		last = data - lx;					\
		next = data + nx;					\
									\
		dx[0] = ((float)*next++ - (float)*last++) * xStep;	\
		dx[1] = ((float)*next++ - (float)*last++) * xStep;	\
		dx[2] = ((float)*next   - (float)*last  ) * xStep;	\
									\
		last = data - ly;					\
		next = data + ny;					\
									\
		dy[0] = ((float)*next++ - (float)*last++) * yStep;	\
		dy[1] = ((float)*next++ - (float)*last++) * yStep;	\
		dy[2] = ((float)*next   - (float)*last  ) * yStep;	\
									\
		last = data - lz;					\
		next = data + nz;					\
									\
		dz[0] = ((float)*next++ - (float)*last++) * zStep;	\
		dz[1] = ((float)*next++ - (float)*last++) * zStep;	\
		dz[2] = ((float)*next   - (float)*last  ) * zStep;	\
									\
		if (div)						\
		{							\
		    REG_DIV(dx, dy, dz, div);				\
		    div ++;						\
		}							\
									\
		if (curl)						\
		{							\
		    REG_CURL(dx, dy, dz, curl);				\
		    curl += 3;						\
		}							\
									\
		data += 3;						\
	    }								\
	}								\
    }									\
}	
    
    switch(type)
    {
	case TYPE_DOUBLE:
	    REG_CUBES_DIVCURL(double);
	    break;
		
	case TYPE_FLOAT:
	    REG_CUBES_DIVCURL(float);
	    break;
		
	case TYPE_INT:
	    REG_CUBES_DIVCURL(int);
	    break;
		
	case TYPE_UINT:
	    REG_CUBES_DIVCURL(uint);
	    break;
		
	case TYPE_SHORT:
	    REG_CUBES_DIVCURL(short);
	    break;
		
	case TYPE_USHORT:
	    REG_CUBES_DIVCURL(ushort);
	    break;
		
	case TYPE_BYTE:
	    REG_CUBES_DIVCURL(byte);
	    break;

	case TYPE_UBYTE:
	    REG_CUBES_DIVCURL(ubyte);
	    break;

	default:
	    DXSetError(ERROR_DATA_INVALID, "#10320", "data");
	    goto error;
    }

    return OK;

error:
    return ERROR;
}

static Error
_dxfDivCurlIrregular(Field field)
{
    Array       pA = NULL, cA = NULL, dA = NULL, curlA = NULL, divA = NULL;
    void        (*elementMethod)() = NULL;
    Object      attr;
    int         pDim, nVperE, nPoints, nElements, eDim = 0;
    Type        dataType;
    char        *eltType;
    byte        *counts = NULL;
    float       *divs = NULL;
    float       *curls = NULL;
    Pointer     data = NULL;
    int	        *e, i, j;
    float       *dPtr, *cPtr;
    byte        *c;
    int         curlsInLocal=0, divsInLocal=0;
    ArrayHandle pH = NULL, cH = NULL;
    InvalidComponentHandle icH = NULL;

    if (DXEmptyField(field))
	goto done;
    
    /*
     * Make sure invalid connections are up to data
     */
    if (! DXInvalidateConnections((Object)field))
	goto error;

    /*
     * Make sure data is dep on positions
     */
    attr = DXGetComponentAttribute(field, "data", "dep");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "#10255", "data", "dependency");
	goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "#10200", "dependency attribute");
	goto error;
    }

    if (strcmp(DXGetString((String)attr), "positions"))
    {
	DXSetError(ERROR_DATA_INVALID, "#11251", "data");
	goto error;
    }

    /*
     * Get the element type 
     */
    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_DATA_INVALID, "#10255", "connections", "element type");
	goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "#10200", "element type attribute");
	goto error;
    }

    eltType = DXGetString((String)attr);

    /*
     * Get various arrays, pointers to their contents and other
     * related information
     */
    pA = (Array)DXGetComponentValue(field, "positions");
    if (! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data", "positions");
	goto error;
    }

    pH = DXCreateArrayHandle(pA);
    if (! pH)
	goto error;

    DXGetArrayInfo(pA, &nPoints, NULL, NULL, NULL, &pDim);

    if (DXGetError() != ERROR_NONE)
	goto error;

    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data", "connections");
	goto error;
    }

    cH = DXCreateArrayHandle(cA);
    if (! cH)
	goto error;

    DXGetArrayInfo(cA, &nElements, NULL, NULL, NULL, &nVperE);

    if (DXGetError() != ERROR_NONE)
	goto error;

    dA = (Array)DXGetComponentValue(field, "data");
    data = DXGetArrayDataLocal(dA);

    DXGetArrayInfo(dA, NULL, &dataType, NULL, NULL, NULL);

    /*
     * Get element-type dependent info: the dimensionality of the elements
     * and a pointer to a routine which will determine element div and curl
     */
    if (! strcmp(eltType, "tetrahedra"))
    {
	eDim = 3; elementMethod = _dxfTetrasDivCurl;
    }
    else if (! strcmp(eltType, "cubes"))
    {
	eDim = 3; elementMethod = _dxfCubesDivCurl;
    }
    else
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11380", eltType);
	goto error;
    }

    /*
     * Dimensionality of elements and the space in which they are embedded
     * had better agree.
     */
    if (eDim != pDim)
    {
	DXSetError(ERROR_DATA_INVALID, "#11360", pDim, eltType, eDim);
	goto error;
    }

    /*
     * Allocation a counts array, recalling the number of element
     * samples that have been accumulated for each position.
     */
    counts = (byte *)DXAllocateLocalZero(nPoints);
    if (! counts)
    {
       DXResetError();
       counts = (byte *)DXAllocateZero(nPoints);
    }
    if (! counts)
	goto error;
    
    /*
     * Ideally, use a local memory buffer for the output buffers
     * If insufficient memory is available, use the output array directly
     * to avoid an unnecessary copy.
     */
    curlA = (Array)DXGetComponentValue(field, "curl");
    if (curlA)
    {
	curls = (float *)DXAllocateLocalZero(nPoints * pDim * sizeof(float));
	if (! curls)
	{
	    DXResetError();
	    curls = (float *)DXGetArrayData(curlA);
	    if (! curls || DXGetError())
		goto error;

	    curlsInLocal = FALSE;
	}
	else
	    curlsInLocal = TRUE;
    }
    else
	curls = NULL;

    divA = (Array)DXGetComponentValue(field, "div");
    if (divA)
    {
	divs = (float *)DXAllocateLocalZero(nPoints * sizeof(float));
	if (! divs)
	{
	   DXResetError();
	   divs = (float *)DXGetArrayData(divA);
	    if (! divs || DXGetError())
		goto error;

	   divsInLocal = FALSE;
	}
	else
	    divsInLocal = TRUE;
    }
    else
	divs = NULL;
    
    /*
     * Get the invalid component handle for connections
     */
    icH = DXCreateInvalidComponentHandle((Object)field, NULL, "connections");
    if (! icH)
	goto error;

    /*
     * For each element, compute the local redults and add the results
     * to the slots of the buffers that correspond to the vertices
     * of the element.  Bump a counter for the affected vertices.
     */
    for (i = 0; i < nElements; i++)
    {
	if (DXIsElementValid(icH, i))
	{
	    int ebuf[16];

	    e = DXGetArrayEntry(cH, i, (Pointer)ebuf);

	    (*elementMethod)(pH, data, dataType, divs, curls, counts, e);
	}
    }

    DXFreeInvalidComponentHandle(icH);
    icH = NULL;

    /*
     * For each output datum, divide through by the number of elements
     * that contributed to it.
     */
    dPtr = divs; cPtr = curls; c = counts;
    for (i = 0; i < nPoints; i++, c++)
    {
	if (*c)
	{
	    float d = 1.0 / counts[i];

	    if (dPtr)
		*dPtr++ *= d;

	    if (cPtr)
		for (j = 0; j < pDim; j++)
		    *cPtr++ *= d;
	}
	else
	{
	    if (dPtr)
		dPtr ++;
	    if (cPtr)
		cPtr ++;
	}
    }

    DXFree((Pointer)counts);
    counts = NULL;

    /*
     * If we were able to create local-memory output buffers, we
     * now have to copy out the local buffers.
     * Otherwise, the arrays already exist and the  data's already there.
     */
    if (divA && divsInLocal)
    {
	if (! DXAddArrayData(divA, 0, nPoints, (Pointer)divs))
	    goto error;

	DXFree((Pointer)divs);
	divs = NULL;
    }

    if (curlA && curlsInLocal)
    {
	if (! DXAddArrayData(curlA, 0, nPoints, (Pointer)curls))
	    goto error;
	
	DXFree((Pointer)curls);
	curls = NULL;
    }

    DXFreeArrayHandle(pH);
    DXFreeArrayHandle(cH);

    if (dA && data)
	DXFreeArrayDataLocal(dA, (Pointer)data);

done:
    return OK;

error:

    DXFree((Pointer)counts);

    if (icH)
	DXFreeInvalidComponentHandle(icH);
    DXFreeArrayHandle(pH);
    DXFreeArrayHandle(cH);

    if (dA && data)
	DXFreeArrayDataLocal(dA, (Pointer)data);

    if (divA && divsInLocal)
	DXFreeArrayDataLocal(divA, (Pointer)divs);
    
    if (curlA && curlsInLocal)
	DXFreeArrayDataLocal(curlA, (Pointer)curls);
    
    return ERROR;
}

#define TETRA_WEIGHTS(x, y, z, w0, w1, w2, w3)			\
{								\
    float x0t, y0t, z0t;					\
    float xt1, yt1, zt1;					\
    float xt2, yt2, zt2;					\
    float xt3, yt3, zt3;					\
    float yzt12, yzt13, yzt23, yz0t2, yz0t3, yz01t, yz02t;	\
								\
    x0t = (x) -  x0;     y0t = (y) -  y0;     z0t = (z) -  z0;	\
    xt1 =  x1 - (x);     yt1 =  y1 - (y);     zt1 =  z1 - (z);	\
    xt2 =  x2 - (x);     yt2 =  y2 - (y);     zt2 =  z2 - (z);	\
    xt3 =  x3 - (x);     yt3 =  y3 - (y);     zt3 =  z3 - (z);	\
   								\
    yzt12 = yt1 * zt2 - yt2 * zt1;				\
    yzt13 = yt1 * zt3 - yt3 * zt1;				\
    yzt23 = yt2 * zt3 - yt3 * zt2;				\
    yz0t2 = y0t * z02 - y02 * z0t;				\
    yz0t3 = y0t * z03 - y03 * z0t;				\
    yz01t = y01 * z0t - y0t * z01;				\
    yz02t = y02 * z0t - y0t * z02;				\
								\
    w0 = (xt1 * yzt23 - xt2 * yzt13 + xt3 * yzt12) * invVolume;	\
    w1 = (x0t * yz023 - x02 * yz0t3 + x03 * yz0t2) * invVolume;	\
    w2 = (x01 * yz0t3 - x0t * yz013 + x03 * yz01t) * invVolume;	\
    w3 = (x01 * yz02t - x02 * yz01t + x0t * yz012) * invVolume;	\
}

#define TETRA_VECTOR(type, p, q, r, s, wp, wq, wr, ws, vector)	\
{								\
    type  *dp, *dq, *dr, *ds;					\
								\
    /*								\
     * Get the data values at the vertices			\
     */								\
    dp = ((type *)data) + 3*p;					\
    dq = ((type *)data) + 3*q;					\
    dr = ((type *)data) + 3*r;					\
    ds = ((type *)data) + 3*s;					\
								\
    /*								\
     * DXInterpolate the data values for the			\
     * center point and the points a step away			\
     * along the three axes					\
     */								\
    vector[0] = wp*(*dp++)+wq*(*dq++)+wr*(*dr++)+ws*(*ds++);	\
    vector[1] = wp*(*dp++)+wq*(*dq++)+wr*(*dr++)+ws*(*ds++);	\
    vector[2] = wp*(*dp)  +wq*(*dq)  +wr*(*dr)  +ws*(*ds)  ;	\
}

#define DELTA_VECTORS(vc, vx, vy, vz, dx, dy, dz)		\
{								\
    dx[0]=vx[0]-vc[0]; dx[1]=vx[1]-vc[1]; dx[2]=vx[2]-vc[2];	\
    dy[0]=vy[0]-vc[0]; dy[1]=vy[1]-vc[1]; dy[2]=vy[2]-vc[2];	\
    dz[0]=vz[0]-vc[0]; dz[1]=vz[1]-vc[1]; dz[2]=vz[2]-vc[2];	\
}

#define IRREG_CURL(dx, dy, dz, curl)				\
{								\
    curl[0] = (dy[2] - dz[1]) * invStep;			\
    curl[1] = (dz[0] - dx[2]) * invStep;			\
    curl[2] = (dx[1] - dy[0]) * invStep;			\
}

#define IRREG_DIV(dx, dy, dz, div)				\
{								\
    div = (dx[0] + dy[1] + dz[2]) * invStep;			\
}

static void
_dxfTetrasDivCurl(ArrayHandle pH, Pointer data,
		Type type, float *divs, float *curls, byte *counts, int *tetra)
{
    float   *pt;
    int     p, q, r, s;
    float   x0, x1, x2, x3;
    float   y0, y1, y2, y3;
    float   z0, z1, z2, z3;
    float   x01, x02, x03;
    float   y01, y02, y03;
    float   z01, z02, z03;
    float   wc0, wc1, wc2, wc3; /* weights for 4 vertices for centroid */
    float   wx0, wx1, wx2, wx3; /* weights for 4 vertices for x step   */
    float   wy0, wy1, wy2, wy3; /* weights for 4 vertices for y step   */
    float   wz0, wz1, wz2, wz3; /* weights for 4 vertices for z step   */
    float   volume, invVolume;
    float   step, invStep;
    float   yz012, yz013, yz023;
    float   cx, cy, cz;		/* centroid			       */
    float   curl[3]= {0,0,0}, div;
    float   vc[3], vx[3], vy[3], vz[3];
    float   dx[3], dy[3], dz[3];
    float   pbuf[3];

    p = *tetra++;
    q = *tetra++;
    r = *tetra++;
    s = *tetra;

    pt = DXGetArrayEntry(pH, p, (Pointer)pbuf);
    x0 = *pt++; y0 = *pt++; z0 = *pt;

    pt = DXGetArrayEntry(pH, q, (Pointer)pbuf);
    x1 = *pt++; y1 = *pt++; z1 = *pt;

    pt = DXGetArrayEntry(pH, r, (Pointer)pbuf);
    x2 = *pt++; y2 = *pt++; z2 = *pt;

    pt = DXGetArrayEntry(pH, s, (Pointer)pbuf);
    x3 = *pt++; y3 = *pt++; z3 = *pt;

    x01 = x1 - x0; y01 = y1 - y0; z01 = z1 - z0;
    x02 = x2 - x0; y02 = y2 - y0; z02 = z2 - z0;
    x03 = x3 - x0; y03 = y3 - y0; z03 = z3 - z0;

    /*
     * Need to get a handle on the size of the tetrahedra.  Use manhattan
     * length to estimate edge length; average three edges incident on p0
     * to get an average edge length; take 0.01 of that.
     */
    step = ((x01 + y01 + z01) + (x02 + y02 + z02) + (x03 + y03 + z03)) * 0.0333;
    if (step == 0.0)
	return;
    
    /*
     * Need to divide data delta by step length
     */
    invStep = 1.0 / step;

    /*
     * A few intermediates we will be re-using
     */
    yz012 = y01 * z02 - y02 * z01;
    yz013 = y01 * z03 - y03 * z01;
    yz023 = y02 * z03 - y03 * z02;

    /*
     * volume of tetrahedra
     */
    volume = x01 * yz023 - x02 * yz013 + x03 * yz012;
    if (volume == 0.0)
	return;

    /*
     * Need to normalize weights later
     */
    invVolume = 1.0 / volume;

    /*
     * Get a point inside the tetrahedra 
     */
    cx = (x0 + x1 + x2 + x3) * 0.25;
    cy = (y0 + y1 + y2 + y3) * 0.25;
    cz = (z0 + z1 + z2 + z3) * 0.25;

    /*
     * evalate weights for center point and for
     * points one step up along each axis
     */
    TETRA_WEIGHTS(cx,      cy,      cz,      wc0, wc1, wc2, wc3);
    TETRA_WEIGHTS(cx+step, cy,      cz,      wx0, wx1, wx2, wx3);
    TETRA_WEIGHTS(cx,      cy+step, cz,      wy0, wy1, wy2, wy3);
    TETRA_WEIGHTS(cx,      cy,      cz+step, wz0, wz1, wz2, wz3);

    switch(type)
    {
	case TYPE_DOUBLE:
	    TETRA_VECTOR(double, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(double, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(double, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(double, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;

	case TYPE_FLOAT:
	    TETRA_VECTOR(float, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(float, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(float, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(float, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;
	
	case TYPE_INT:
	    TETRA_VECTOR(int, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(int, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(int, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(int, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;

	case TYPE_UINT:
	    TETRA_VECTOR(uint, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(uint, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(uint, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(uint, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;

	case TYPE_SHORT:
	    TETRA_VECTOR(short, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(short, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(short, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(short, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;
	
	case TYPE_USHORT:
	    TETRA_VECTOR(ushort, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(ushort, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(ushort, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(ushort, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;
	
	case TYPE_BYTE:
	    TETRA_VECTOR(byte, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(byte, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(byte, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(byte, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;
	
	case TYPE_UBYTE:
	    TETRA_VECTOR(ubyte, p, q, r, s, wc0, wc1, wc2, wc3, vc);
	    TETRA_VECTOR(ubyte, p, q, r, s, wx0, wx1, wx2, wx3, vx);
	    TETRA_VECTOR(ubyte, p, q, r, s, wy0, wy1, wy2, wy3, vy);
	    TETRA_VECTOR(ubyte, p, q, r, s, wz0, wz1, wz2, wz3, vz);

	    break;
	
	default:
	    return;
    }

    DELTA_VECTORS(vc, vx, vy, vz, dx, dy, dz);

    if (curls)
	IRREG_CURL(dx, dy, dz, curl);
    
    if (divs)
	IRREG_DIV(dx, dy, dz, div);

    if (divs)
    {
	divs[p] += div;
	divs[q] += div;
	divs[r] += div;
	divs[s] += div;
    }

    if (curls)
    {
	float *cPtr;

	cPtr = curls + 3*p;
	*cPtr++ += curl[0]; 
	*cPtr++ += curl[1]; 
	*cPtr   += curl[2];

	cPtr = curls + 3*q;
	*cPtr++ += curl[0]; 
	*cPtr++ += curl[1]; 
	*cPtr   += curl[2];

	cPtr = curls + 3*r;
	*cPtr++ += curl[0]; 
	*cPtr++ += curl[1]; 
	*cPtr   += curl[2];

	cPtr = curls + 3*s;
	*cPtr++ += curl[0]; 
	*cPtr++ += curl[1]; 
	*cPtr   += curl[2];
    }

    counts[p] ++;
    counts[q] ++;
    counts[r] ++;
    counts[s] ++;

    return;
}

static void
_dxfCubesDivCurl(ArrayHandle pH, Pointer data,
		Type type, float *div, float *curl, byte *counts, int *cube)
{
    int tetra[4];

    tetra[0] = cube[0];
    tetra[1] = cube[6];
    tetra[2] = cube[5];
    tetra[3] = cube[3];

    _dxfTetrasDivCurl(pH, data, type, div, curl, counts, tetra);
    
    tetra[0] = cube[1];
    tetra[1] = cube[2];
    tetra[2] = cube[4];
    tetra[3] = cube[7];

    _dxfTetrasDivCurl(pH, data, type, div, curl, counts, tetra);
}
