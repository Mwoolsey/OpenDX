/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_gradient.c,v 1.5 2000/08/24 20:04:12 davidt Exp $
 */

#include <dxconfig.h>


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "vectors.h"
#include "_gradient.h"

static Object _dxfGradientObject(Object);
static Error  _dxfGradientField(Pointer);

static Error  _dxfGradientRegular(Field, int *, float *, int, int *);
static Error  _dxfGradientCubesRegular(Field, int *, float *, int *);
static Error  _dxfGradientQuadsRegular(Field, int *, float *, int *);
static Error  _dxfGradientLinesRegular(Field, int *, float *, int *);

static Error  _dxfGradientIrregular(Field);

static Error  _dxfRecursiveSetGroupTypeV(Object, Type, Category, int, int *);

static void   _dxfTrisGradient(ArrayHandle, Pointer, Type,
				float *, byte *, int *, int);
static void   _dxfQuadsGradient(ArrayHandle, Pointer, Type,
				float *, byte *, int *, int);
static void   _dxfTetrasGradient(ArrayHandle, Pointer, Type,
				float *, byte *, int *, int);
static void   _dxfCubesGradient(ArrayHandle, Pointer, Type,
				float *, byte *, int *, int);

Object
_dxfGradient(Object object)
{
    Object copy = NULL;

    copy = DXCopy(object, COPY_STRUCTURE);
    if (! copy)
	goto error;

    if (! _dxfGradientObject(copy))
	goto error;
    
    return copy;

error:
    if (copy != object)
	DXDelete(copy);

    return NULL;
}

static Object
_dxfGradientObject(Object object)
{
    int i, typed;
    Field field;
    Object child;
    Class class;
    Type type;
    Category cat;
    int rank, shape[32];


    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);
    
    switch (class)
    {
	case CLASS_FIELD:
	    if (! _dxfGradientField((Pointer)&object))
		return NULL;
	    else
		return object;
	
	case CLASS_COMPOSITEFIELD:

	    if (! DXGrow(object, 1, NULL, "data", NULL))
		return NULL;

	    DXCreateTaskGroup();

	    i = 0;
	    while(NULL != (field = DXGetPart(object, i++)))
		if (! DXAddTask(_dxfGradientField, 
				(Pointer)&field, sizeof(Pointer), 1.0))
		{
		    DXAbortTaskGroup();
		    return NULL;
		}
	    
	    if (!DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		return NULL;
	    
	    field = DXGetPart(object, 0);
	    if (! field)
	    {
		/*
		 * Group with no parts.  Fairly boring case.  Set the Type
		 * of the Group to something reasonable.
		 */

		type = TYPE_FLOAT;
		cat = CATEGORY_REAL;
		rank = 1;
		shape[0] = 3;

	    }
	    else
	    {
		/*
		 * Set the Type of the Group to the type of the part we have
		 * found.
		 */
		DXGetType((Object)field, &type, &cat, &rank, shape);
	    }

	    if (! DXShrink(object))
		return NULL;

	    if (! _dxfRecursiveSetGroupTypeV(object, type, cat, rank, shape))
		return NULL;
    
	    return object;
	
	case CLASS_SERIES:
	case CLASS_MULTIGRID:

	    typed = (NULL != DXGetType(object, NULL, NULL, NULL, NULL));

	    i = 0; 
	    for (i = 0; ;i++)
	    {
	        child = DXGetEnumeratedMember((Group)object, i, NULL);
		if (! child)
		    break;

		if (! _dxfGradientObject(child))
		    return NULL;
	    }

	    if (typed)
	    {
		DXGetType(DXGetEnumeratedMember((Group)object, 0, NULL),
						&type, &cat, &rank, shape);
		if (! DXSetGroupTypeV((Group)object, type, cat, rank, shape))
		    return NULL;
	    }
	    
	    return object;
	
	case CLASS_GROUP:

	    i = 0; 
	    for (i = 0; ;i++)
	    {
	        child = DXGetEnumeratedMember((Group)object, i, NULL);
		if (! child)
		    break;

		if (! _dxfGradientObject(child))
		    return NULL;
	    }
	    
	    return object;
	
	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)object, &child, 0))
		return NULL;
	    
	    if (! _dxfGradientObject(child))
		return NULL;
	    else
		return object;
    
	case CLASS_CLIPPED:

	    if (! DXGetClippedInfo((Clipped)object, &child, 0))
		return NULL;
	    
	    if (! _dxfGradientObject(child))
		return NULL;
	    else
		return object;
    
	default:

	    DXSetError(ERROR_DATA_INVALID, "#11381");
	    return NULL;
    }
}

static Error
_dxfGradientField(Pointer ptr)
{
    Field		field;
    Type		type;
    Category		cat;
    int			nItems, dim, rank, shape[32];
    Array		positions;
    Array		connections;
    Array		data;
    float		del[9];
    int  		counts[3];
    int			grid;
    Object		attr;
    int			permute[32];
    int			axes[32];
    int			pFlag=0;

    field = *(Field *)ptr;

    if (DXEmptyField(field))
	return OK;
    
    if (! DXGetComponentValue(field, "connections"))
    { 
	DXSetError(ERROR_MISSING_DATA, "#10240", "connections");
	goto error;
    }

    data = (Array)DXGetComponentValue(field, "data");
    if (! data)
    { 
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	goto error;
    }

    DXGetArrayInfo(data, &nItems, &type, &cat, &rank, shape);
    if (cat != CATEGORY_REAL)
    {
	DXSetError (ERROR_DATA_INVALID, "#11150", "data");
	goto error;
    }

    if (! (rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, "#10081", "data");
	goto error;
    }

    /*
     * Make sure data is dep on positions
     */
    attr = DXGetComponentAttribute(field, "data", "dep");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "#10255", "data", "dep");

	goto error;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "#11450");
	goto error;
    }

    if (strcmp(DXGetString((String)attr), "positions"))
    {
	DXSetError(ERROR_DATA_INVALID, "#11251", "data");
	goto error;
    }

    /*
     * Quick out for constant data
     */
    if (DXQueryConstantArray(data, NULL, NULL))
    {
	float zero[3] = {0.0, 0.0, 0.0};
	int   nDim;
	Array grad;

	positions = (Array)DXGetComponentValue(field, "positions");
	DXGetArrayInfo(positions, NULL, NULL, NULL, NULL, &nDim);

	grad = (Array)DXNewConstantArray(nItems, (Pointer)&zero, 	
					TYPE_FLOAT, CATEGORY_REAL, 1, nDim);
	if (! grad)
	    goto error;
	
	DXSetComponentValue(field, "data", (Object)grad);
    }
    else
    {
	positions   = (Array)DXGetComponentValue(field, "positions");
	connections = (Array)DXGetComponentValue(field, "connections");

	if (! DXInvalidateUnreferencedPositions((Object)field))
	    goto error;
	
	if (! DXInvalidateConnections((Object)field))
	    goto error;

	/*
	 * If the grid is regular and axis-aligned, we can use a faster method.
	 * UNLESS there's an invalid connections element
	 */
	if (DXQueryGridConnections(connections, NULL, NULL) &&
	    DXQueryGridPositions(positions, &dim, counts, NULL, del) &&
	    ! DXGetComponentValue(field, "invalid connections"))
	{
	    int i, j, k;

	    for (i = 0; i < dim; i++)
	    {
		permute[i] = -1;
		axes[i]    = -1;
	    }
	    
	    grid = 1;
	    for (i = 0; i < dim && grid; i++)
	    {
		k = -1;
		for (j = 0; j < dim && grid; j++)
		{
		    /*
		     * Check that only one axis delta coefficient is non-zero
		     */
		    if (del[i*dim+j] != 0.0) {
			if (k != -1)
			    grid = 0;
			else
			    k = j;
		    }
		}

		if (! grid)
		    break;
		    
		/*
		 * Verify that this geometrical axis is not currently covered
		 * by some other data axis
		 */
		if (axes[k] != -1)
		    grid = 0;
		else
		    axes[k] = i;
		
		permute[i] = k;
	    }

	    if (grid)
	    {
		pFlag = 1;
		for (i = 0; i < dim && pFlag; i++)
		    pFlag = i == (permute[i]);
	    }
	}
	else
	    grid = 0;

	if (grid)
	{
	    if (! _dxfGradientRegular(field, counts, del, dim,
							pFlag?NULL:permute))
		goto error;
	}
	else
	{
	    if (!  _dxfGradientIrregular(field))
		goto error;
	}
    }

    if (DXGetComponentValue(field, "original data"))
	DXDeleteComponent(field, "original data");
    
    DXChangedComponentValues(field, "data");
    if (! DXEndField(field))
	goto error;

    return OK;

error:
    return ERROR;
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
_dxfGradientRegular(Field field, int *counts, float *deltas, int nDim, int *permute)
{
    switch(nDim)
    {
	case 1:
	    return _dxfGradientLinesRegular(field, counts, deltas, permute);

	case 2:
	    return _dxfGradientQuadsRegular(field, counts, deltas, permute);

	case 3:
	    return _dxfGradientCubesRegular(field, counts, deltas, permute);
	
	default:
	    DXSetError(ERROR_DATA_INVALID, "#10300", "data");
	    return ERROR;
    }
}

static Error
_dxfGradientCubesRegular(Field field, int *counts, float *deltas, int *permute)
{
    float   dxInv, dyInv, dzInv;
    int     xKnt  = counts[0];
    int     yKnt  = counts[1];
    int     zKnt  = counts[2];
    Vector  *vectors;
    Array   inArray, outArray;
    Type    type;
    int     i, j, k;
    int     lx, ly, lz, nx, ny, nz;
    float   dx, dy, dz;
    int	    nItems;

    if (permute)
    {
	dxInv = 1.0 / deltas[permute[0]];
	dyInv = 1.0 / deltas[permute[1]+3];
	dzInv = 1.0 / deltas[permute[2]+6];
    }
    else
    {
	dxInv = 1.0 / deltas[0];
	dyInv = 1.0 / deltas[4];
	dzInv = 1.0 / deltas[8];
    }

    outArray = NULL;

    inArray = (Array)DXGetComponentValue(field, "data");

    DXGetArrayInfo(inArray, &nItems, &type, NULL, NULL, NULL);

    outArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! outArray)
	goto error;
    
    if (! DXAddArrayData(outArray, 0, nItems, NULL))
	goto error;
    
#define REG_CUBES_GRADIENT(type)					\
{									\
    type  *data, *last, *next;						\
    int   xSkip, ySkip, zSkip;						\
									\
    vectors = (Vector *)DXGetArrayData(outArray);				\
    data    = (type  *)DXGetArrayData(inArray);				\
									\
    zSkip = 1;								\
    ySkip = zSkip * counts[2];						\
    xSkip = ySkip * counts[1];						\
									\
    for (i = 0; i < xKnt; i++)						\
    {									\
	dx = dxInv;							\
	if (i == 0)							\
	{								\
	    lx = 0;							\
	    nx = xSkip;							\
	    dx = dxInv;							\
	}								\
	else if (i == (xKnt - 1))					\
	{								\
	    lx = xSkip;							\
	    nx = 0;							\
	    dx = dxInv;							\
	}								\
	else								\
	{								\
	    lx = xSkip;							\
	    nx = xSkip;							\
	    dx = 0.5  * dxInv;						\
	}								\
									\
	for (j = 0; j < yKnt; j++)					\
	{								\
	    dy = dyInv;							\
	    if (j == 0)							\
	    {								\
		ly = 0;							\
		ny = ySkip;						\
		dy = dyInv;						\
	    }								\
	    else if (j == (yKnt - 1))					\
	    {								\
		ly = ySkip;						\
		ny = 0;							\
		dy = dyInv;						\
	    }								\
	    else							\
	    {								\
		ly = ySkip;						\
		ny = ySkip;						\
		dy = 0.5  * dyInv;					\
	    }								\
									\
	    for (k = 0; k < zKnt; k++)					\
	    {								\
		dz = dzInv;						\
		if (k == 0)						\
		{							\
		    lz = 0;						\
		    nz = zSkip;						\
		    dz = dzInv;						\
		}							\
		else if (k == (zKnt - 1))				\
		{							\
		    lz = zSkip;						\
		    nz = 0;						\
		    dz = dzInv;						\
		}							\
		else							\
		{							\
		    lz = zSkip;						\
		    nz = zSkip;						\
		    dz = 0.5  * dzInv;					\
		}							\
									\
		last = data - lx;					\
		next = data + nx;					\
		vectors->x = (((float)(*next)) - *last) * dx;		\
									\
		last = data - ly;					\
		next = data + ny;					\
		vectors->y = (((float)(*next)) - *last) * dy;		\
									\
		last = data - lz;					\
		next = data + nz;					\
		vectors->z = (((float)(*next)) - *last) * dz;		\
									\
		data ++;						\
		vectors ++;						\
	    }								\
	}								\
    }									\
									\
    if (permute)							\
    {									\
	int ix, iy, iz;							\
	float *data = (float  *)DXGetArrayData(outArray);		\
	ix = permute[0]; iy = permute[1]; iz = permute[2];		\
	for (i = 0; i < nItems; i++)					\
	{								\
	    float x = data[0];						\
	    float y = data[1];						\
	    float z = data[2];						\
	    data[ix] = x;						\
	    data[iy] = y;						\
	    data[iz] = z;						\
	    data += 3;							\
	}								\
    }									\
}	
    
    switch(type)
    {
	case TYPE_DOUBLE:
	    REG_CUBES_GRADIENT(double);
	    break;
		
	case TYPE_FLOAT:
	    REG_CUBES_GRADIENT(float);
	    break;
		
	case TYPE_INT:
	    REG_CUBES_GRADIENT(int);
	    break;
		
	case TYPE_UINT:
	    REG_CUBES_GRADIENT(uint);
	    break;
		
	case TYPE_SHORT:
	    REG_CUBES_GRADIENT(short);
	    break;
		
	case TYPE_USHORT:
	    REG_CUBES_GRADIENT(ushort);
	    break;
		
	case TYPE_BYTE:
	    REG_CUBES_GRADIENT(byte);
	    break;
		
	case TYPE_UBYTE:
	    REG_CUBES_GRADIENT(ubyte);
	    break;

	default:
	    DXSetError(ERROR_DATA_INVALID, "#10320", "data");
	    goto error;
    }

    if (! DXSetComponentValue(field, "data", (Object)outArray))
	goto error;
    
    return OK;

error:
    DXDelete((Object)outArray);
    return ERROR;
}

static Error
_dxfGradientQuadsRegular(Field field, int *counts, float *deltas, int *permute)
{
    float   dxInv;
    float   dyInv;
    int     xKnt  = counts[0];
    int     yKnt  = counts[1];
    float   *vectors;
    Array   inArray, outArray;
    Type    type = TYPE_FLOAT;
    int     i, j;
    int     lx, ly, nx, ny;
    float   dx, dy;
    int	    nItems = 0;

    if (permute)
    {
	dxInv = 1.0 / deltas[1];
	dyInv = 1.0 / deltas[2];
    }
    else
    {
	dxInv = 1.0 / deltas[0];
	dyInv = 1.0 / deltas[3];
    }

    outArray = NULL;

    inArray = (Array)DXGetComponentValue(field, "data");

    DXGetArrayInfo(inArray, &nItems, &type, NULL, NULL, NULL);

    outArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 2);
    if (! outArray)
	goto error;
    
    if (! DXAddArrayData(outArray, 0, nItems, NULL))
	goto error;
    
#define REG_QUADS_GRADIENT(type)					\
{									\
    type  *data, *last, *next;						\
    int   xSkip, ySkip;							\
									\
    vectors = (float *)DXGetArrayData(outArray);				\
    data    = (type  *)DXGetArrayData(inArray);				\
									\
    ySkip = 1;								\
    xSkip = ySkip * counts[1];						\
									\
    for (i = 0; i < xKnt; i++)						\
    {									\
	dx = dxInv;							\
	if (i == 0)							\
	{								\
	    lx = 0;							\
	    nx = xSkip;							\
	    dx = dxInv;							\
	}								\
	else if (i == (xKnt - 1))					\
	{								\
	    lx = xSkip;							\
	    nx = 0;							\
	    dx = dxInv;							\
	}								\
	else								\
	{								\
	    lx = xSkip;							\
	    nx = xSkip;							\
	    dx = 0.5  * dxInv;						\
	}								\
									\
	for (j = 0; j < yKnt; j++)					\
	{								\
	    dy = dyInv;							\
	    if (j == 0)							\
	    {								\
		ly = 0;							\
		ny = ySkip;						\
		dy = dyInv;						\
	    }								\
	    else if (j == (yKnt - 1))					\
	    {								\
		ly = ySkip;						\
		ny = 0;							\
		dy = dyInv;						\
	    }								\
	    else							\
	    {								\
		ly = ySkip;						\
		ny = ySkip;						\
		dy = 0.5  * dyInv;					\
	    }								\
									\
									\
	    last = data - lx;						\
	    next = data + nx;						\
	    *vectors++ = (((float)(*next)) - *last) * dx;		\
									\
	    last = data - ly;						\
	    next = data + ny;						\
	    *vectors++ = (((float)(*next)) - *last) * dy;		\
									\
	    data ++;							\
	}								\
    }									\
    									\
    if (permute)							\
    {									\
	float *data = (float  *)DXGetArrayData(outArray);	        \
	for (i = 0; i < nItems; i++)					\
	{								\
	    float t = data[0];						\
	    data[0] = data[1];						\
	    data[1] = t;						\
	    data += 2;							\
	}								\
    }									\
}	
    
    switch(type)
    {
	case TYPE_DOUBLE:
	    REG_QUADS_GRADIENT(double);
	    break;
		
	case TYPE_FLOAT:
	    REG_QUADS_GRADIENT(float);
	    break;
		
	case TYPE_INT:
	    REG_QUADS_GRADIENT(int);
	    break;
		
	case TYPE_UINT:
	    REG_QUADS_GRADIENT(uint);
	    break;
		
	case TYPE_SHORT:
	    REG_QUADS_GRADIENT(short);
	    break;
		
	case TYPE_USHORT:
	    REG_QUADS_GRADIENT(ushort);
	    break;
		
	case TYPE_BYTE:
	    REG_QUADS_GRADIENT(byte);
	    break;
		
	case TYPE_UBYTE:
	    REG_QUADS_GRADIENT(ubyte);
	    break;

	default:
	    DXSetError(ERROR_DATA_INVALID, "#10320", "data");
	    goto error;
    }

    if (! DXSetComponentValue(field, "data", (Object)outArray))
	goto error;
    
    return OK;

error:
    DXDelete((Object)outArray);
    return ERROR;
}

static Error
_dxfGradientLinesRegular(Field field, int *counts, float *deltas, int *permute)
{
    float   dxInv = 1.0 / deltas[0];
    int     xKnt  = counts[0];
    float   *vectors;
    Array   inArray, outArray;
    Type    type = TYPE_FLOAT;
    int     i;
    int     lx, nx;
    float   dx;
    int	    nItems = 0;

    outArray = NULL;

    inArray = (Array)DXGetComponentValue(field, "data");

    DXGetArrayInfo(inArray, &nItems, &type, NULL, NULL, NULL);

    outArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1);
    if (! outArray)
	goto error;
    
    if (! DXAddArrayData(outArray, 0, nItems, NULL))
	goto error;
    
#define REG_LINES_GRADIENT(type)					\
{									\
    type  *data, *last, *next;						\
									\
    vectors = (float *)DXGetArrayData(outArray);				\
    data    = (type  *)DXGetArrayData(inArray);				\
									\
    for (i = 0; i < xKnt; i++)						\
    {									\
	dx = dxInv;							\
	if (i == 0)							\
	{								\
	    lx = 0;							\
	    nx = 1;							\
	    dx = dxInv;							\
	}								\
	else if (i == (xKnt - 1))					\
	{								\
	    lx = 1;							\
	    nx = 0;							\
	    dx = dxInv;							\
	}								\
	else								\
	{								\
	    lx = 1;							\
	    nx = 1;							\
	    dx = 0.5  * dxInv;						\
	}								\
									\
	last = data - lx;						\
	next = data + nx;						\
	*vectors++ = (((float)(*next)) - *last) * dx;			\
									\
	data ++;							\
    }									\
}	
    
    switch(type)
    {
	case TYPE_DOUBLE:
	    REG_LINES_GRADIENT(double);
	    break;
		
	case TYPE_FLOAT:
	    REG_LINES_GRADIENT(float);
	    break;
		
	case TYPE_INT:
	    REG_LINES_GRADIENT(int);
	    break;
		
	case TYPE_UINT:
	    REG_LINES_GRADIENT(uint);
	    break;
		
	case TYPE_SHORT:
	    REG_LINES_GRADIENT(short);
	    break;
		
	case TYPE_USHORT:
	    REG_LINES_GRADIENT(ushort);
	    break;
		
	case TYPE_BYTE:
	    REG_LINES_GRADIENT(byte);
	    break;
		
	case TYPE_UBYTE:
	    REG_LINES_GRADIENT(ubyte);
	    break;

	default:
	    DXSetError(ERROR_DATA_INVALID, "#10320", "data");
	    goto error;
    }

    if (! DXSetComponentValue(field, "data", (Object)outArray))
	goto error;
    
    return OK;

error:
    DXDelete((Object)outArray);
    return ERROR;
}

static Error
_dxfGradientIrregular(Field field)
{
    Array       pA = NULL, cA = NULL, dA = NULL, gA = NULL;
    void        (*elementMethod)() = NULL;
    Object      attr;
    int         pDim, nVperE, nPoints, nElements;
    Type        dataType;
    char        *eltType;
    byte        *counts = NULL;
    float       *gradients = NULL;
    Pointer     data = NULL;
    int	        i, j, nInv;
    float       *v;
    byte        *c;
    ArrayHandle pHandle = NULL, cHandle = NULL;
    InvalidComponentHandle icH = NULL;
    InvalidComponentHandle ipH = NULL;

    if (DXEmptyField(field))
	goto done;

    /*
     * Get the element type 
     */
    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	DXSetError(ERROR_MISSING_DATA, "#10255", "connections", "element type");
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
    DXGetArrayInfo(pA, &nPoints, NULL, NULL, NULL, &pDim);

    if (DXGetError() != ERROR_NONE)
	goto error;

    cA = (Array)DXGetComponentValue(field, "connections");
    DXGetArrayInfo(cA, &nElements, NULL, NULL, NULL, &nVperE);

    if (DXGetError() != ERROR_NONE)
	goto error;


    dA = (Array)DXGetComponentValue(field, "data");
    DXGetArrayInfo(dA, NULL, &dataType, NULL, NULL, NULL);

    data = DXGetArrayDataLocal(dA);
    if (DXGetError() != ERROR_NONE)
	goto error;
    
    if (dataType != TYPE_DOUBLE && dataType != TYPE_FLOAT  &&
	dataType != TYPE_INT    && dataType != TYPE_UINT   &&
	dataType != TYPE_SHORT  && dataType != TYPE_USHORT &&
	dataType != TYPE_BYTE   && dataType != TYPE_UBYTE)
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#10320", "data");
	goto error;
    }

    /*
     * Get element-type dependent info: the dimensionality of the elements
     * and a pointer to a routine which will determine an element gradient
     */
    if (! strcmp(eltType, "tetrahedra"))
    {
	if (pDim != 3)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11001", "tetrahedra");
	    goto error;
	}
	elementMethod = _dxfTetrasGradient;
    }
    else if (! strcmp(eltType, "cubes"))
    {
	if (pDim != 3)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11001", "cubes");
	    goto error;
	}
	elementMethod = _dxfCubesGradient;
    }
    else if (! strcmp(eltType, "triangles"))
    {
	if (pDim != 3 && pDim != 2)
	{
	    DXSetError(ERROR_DATA_INVALID,  "#11002", "triangles");
	    goto error;
	}
	elementMethod = _dxfTrisGradient;
    }
    else if (! strcmp(eltType, "quads"))
    {
	if (pDim != 3 && pDim != 2)
	{
	    DXSetError(ERROR_DATA_INVALID,  "#11002", "quads");
	    goto error;
	}
	elementMethod = _dxfQuadsGradient;
    }
    else
    {
	DXSetError(ERROR_NOT_IMPLEMENTED, "#11380", eltType);
	goto error;
    }

    /*
     * Allocation a counts array, recalling the number of element
     * gradients that have been accumulated for each position.
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
     * Ideally, use a local memory buffer for the gradient accumulator. 
     * If insufficient memory is available, use the output array directly
     * to avoid an unnecessary copy.
     */
    gradients = (float *)DXAllocateLocalZero(nPoints * pDim * sizeof(float));
    if (! gradients)
    {
       DXResetError();

	gA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, pDim);
	if (! gA)
	    goto error;
    
       if (! DXAddArrayData(gA, 0, nPoints, NULL))
	   goto error;
	
	gradients = (float *)DXGetArrayData(gA);

	memset(gradients, 0, nPoints * pDim * sizeof(float));
    }
    if (! gradients)
	goto error;

    pHandle = DXCreateArrayHandle(pA);
    if (! pHandle)
	goto error;
    
    cHandle = DXCreateArrayHandle(cA);
    if (! cHandle)
	goto error;
    
    icH = DXCreateInvalidComponentHandle((Object)field, NULL, "connections");
    if (! icH)
	goto error;

    /*
     * For each element, compute the local gradient and add the results
     * to the slots of the gradient buffer that correspond to the vertices
     * of the element.  Bump a counter for the affected vertices.
     */
    for (i = 0; i < nElements; i++)
    {
	if (DXIsElementValid(icH, i))
	{
	    int cBuffer[8], *e;

	    e = (int *)DXGetArrayEntry(cHandle, i, (Pointer)cBuffer);

	    (*elementMethod)(pHandle, data, dataType,
				gradients, counts, e, pDim);

	}
    }

    DXFreeInvalidComponentHandle(icH);
    icH = NULL;

    ipH = DXCreateInvalidComponentHandle((Object)field, NULL, "positions");
    if (! ipH)
	goto error;

    /*
     * For each gradient vector, divide through by the number of element
     * gradients that contributed to it.
     */
    v = gradients; c = counts;
    for (i = nInv = 0; i < nPoints; i++, c++)
    {
	if (*c)
	{
	    float d = 1.0 / counts[i];

	    for (j = 0; j < pDim; j++)
		*v++ *= d;
	}
	else
	{
	    DXSetElementInvalid(ipH, i);
	    v += pDim;
	    nInv ++;
	}
    }

    DXFree((Pointer)counts);
    counts = NULL;

    if (nInv != 0)
    {
	if (! DXSaveInvalidComponent(field, ipH))
	    goto error;
    }

    DXFreeInvalidComponentHandle(ipH);
    ipH = NULL;

    /*
     * If we were able to create a local-memory gradient buffer, we
     * now have to create the output array and copy out the gradient buffer.
     * Otherwise, the array already exists and the  data's already there.
     */
    if (! gA)
    {
	gA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, pDim);
	if (! gA)
	    goto error;
	
	if (! DXAddArrayData(gA, 0, nPoints, (Pointer)gradients))
	    goto error;
	
	DXFree((Pointer)gradients);
	gradients = NULL;
    }

    if (dA && data)
	DXFreeArrayDataLocal(dA, (Pointer)data);

    /*
     * DXInsert the gradient array into the field
     */
    if (! DXSetComponentValue(field, "data", (Object)gA))
	goto error;
    gA = NULL;


    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    
done:
    return OK;

error:

    if (icH)
	DXFreeInvalidComponentHandle(icH);

    if (ipH)
	DXFreeInvalidComponentHandle(ipH);

    DXFree((Pointer)counts);

    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (dA && data)
	DXFreeArrayDataLocal(dA, (Pointer)data);

    if (gA)
	DXDelete((Object)gA);
    else if (gradients)
	DXFree((Pointer)gradients);
    
    return ERROR;
}

#define INVERT(m, s, r, ok)						\
{									\
    double i;								\
    float  M[9];							\
									\
    M[0] = m[4]*m[8] - m[5]*m[7];					\
    M[3] = m[5]*m[6] - m[3]*m[8];					\
    M[6] = m[3]*m[7] - m[4]*m[6];					\
									\
    i = m[0]*M[0] + m[1]*M[3] + m[2]*M[6];				\
    if (i == 0.0)							\
    {									\
	ok = 0;								\
    }									\
    else								\
    {									\
	M[1] = m[7]*m[2] - m[8]*m[1];					\
	M[4] = m[8]*m[0] - m[6]*m[2];					\
	M[7] = m[6]*m[1] - m[7]*m[0];					\
									\
	M[2] = m[1]*m[5] - m[2]*m[4];					\
	M[5] = m[2]*m[3] - m[0]*m[5];					\
	M[8] = m[0]*m[4] - m[1]*m[3];					\
									\
	i = 1.0 / i;							\
									\
	r[0] = -i * (M[0]*s[0]+M[1]*s[1]+M[2]*s[2]);			\
	r[1] = -i * (M[3]*s[0]+M[4]*s[1]+M[5]*s[2]);			\
	r[2] = -i * (M[6]*s[0]+M[7]*s[1]+M[8]*s[2]);			\
									\
	ok = 1;								\
    }									\
}

#define _VERTEX(m, moff, s, soff, q, element)					\
    {										\
	int _q = q;								\
	int i1 = element[_q];							\
	Vector *p1 = pPtrs[_q];							\
	(m)[moff+0] = p1->x - p0->x;						\
	(m)[moff+1] = p1->y - p0->y;						\
	(m)[moff+2] = p1->z - p0->z;						\
	s[soff]   = -(((float)dPtr[i1]) - d0);					\
    }										\


#define LOOP(ctype, n, table, element)						\
{										\
    Vector *p0;									\
    float m[9], s[3], r[3], *gPtr;						\
    int     i0; 								\
    float   d0; 								\
    ctype *dPtr = (ctype *)data;						\
    int *t = table, ok;								\
    for (i = 0; i < n; i++)							\
    {										\
	p0 = pPtrs[i];								\
	i0 = element[i];							\
	d0 = dPtr[i0];								\
	_VERTEX(m, 0, s, 0, *t++, element);					\
	_VERTEX(m, 3, s, 1, *t++, element);					\
	_VERTEX(m, 6, s, 2, *t++, element);					\
	INVERT(m, s, r, ok);							\
	if (ok)									\
	{									\
	    gPtr = gradients + 3*element[i];					\
	    gPtr[0] += r[0];							\
	    gPtr[1] += r[1];							\
	    gPtr[2] += r[2];							\
	    counts[element[i]] ++;						\
	}									\
    }										\
}

static int CubeEdgeTable[] =
{
    1, 2, 4,
    0, 3, 5,
    0, 3, 6,
    1, 2, 7,
    0, 5, 6,
    1, 4, 7,
    2, 4, 7,
    3, 5, 6
};

static void
_dxfCubesGradient(ArrayHandle points, Pointer data,
	    Type type, float *gradients, byte *counts, int *cube, int pDim)
{
    Vector  pBuf[8];
    Vector  *pPtrs[8];
    int     i;

    for (i = 0; i < 8; i++)
	pPtrs[i] = (Vector *)DXGetArrayEntry(points, cube[i], pBuf+i);
    
    switch(type)
    {
	case TYPE_FLOAT:  LOOP(float,  8, CubeEdgeTable, cube); break;
	case TYPE_DOUBLE: LOOP(double, 8, CubeEdgeTable, cube); break;
	case TYPE_INT:    LOOP(int,    8, CubeEdgeTable, cube); break;
	case TYPE_UINT:   LOOP(uint,   8, CubeEdgeTable, cube); break;
	case TYPE_SHORT:  LOOP(short,  8, CubeEdgeTable, cube); break;
	case TYPE_USHORT: LOOP(ushort, 8, CubeEdgeTable, cube); break;
	case TYPE_BYTE:   LOOP(byte,   8, CubeEdgeTable, cube); break;
	case TYPE_UBYTE:  LOOP(ubyte,  8, CubeEdgeTable, cube); break;
        default: break;
    }

    return;
}

static int TetraEdgeTable[] =
{
    1,  2, 3,
    0,  2, 3,
    0,  1, 3,
    0,  1, 2,
};

static void
_dxfTetrasGradient(ArrayHandle points, Pointer data,
	    Type type, float *gradients, byte *counts, int *tet, int pDim)
{
    Vector  pBuf[4];
    Vector  *pPtrs[4];
    int     i;

    for (i = 0; i < 4; i++)
	pPtrs[i] = (Vector *)DXGetArrayEntry(points, tet[i], pBuf+i);

    switch(type)
    {
	case TYPE_DOUBLE: LOOP(double, 4, TetraEdgeTable, tet); break;
	case TYPE_FLOAT:  LOOP(float,  4, TetraEdgeTable, tet); break;
	case TYPE_INT:    LOOP(int,    4, TetraEdgeTable, tet); break;
	case TYPE_UINT:   LOOP(uint,   4, TetraEdgeTable, tet); break;
	case TYPE_SHORT:  LOOP(short,  4, TetraEdgeTable, tet); break;
	case TYPE_USHORT: LOOP(ushort, 4, TetraEdgeTable, tet); break;
	case TYPE_BYTE:   LOOP(byte,   4, TetraEdgeTable, tet); break;
	case TYPE_UBYTE:  LOOP(ubyte,  4, TetraEdgeTable, tet); break;
        default: break;
    }

    return;
}

static void
_dxfTrisGradient(ArrayHandle points, Pointer data,
		Type type, float *gradients, byte *counts, int *tri, int pDim)
{

    float   scale;
    Vector  v0, v1, cross;
    float   d0=0, d1=0, d2=0;
    float   *gPtr;
    int     p, q, r;
    float   pBuf[3], qBuf[3], rBuf[3];
    Vector  *p0, *p1, *p2;

    p = *tri++;
    q = *tri++;
    r = *tri;

    p0 = (Vector *)DXGetArrayEntry(points, p, (Pointer)pBuf);
    p1 = (Vector *)DXGetArrayEntry(points, q, (Pointer)qBuf);
    p2 = (Vector *)DXGetArrayEntry(points, r, (Pointer)rBuf);

#define GET_Zs(type)			\
{					\
    d0 = (float)((type *)data)[p];	\
    d1 = (float)((type *)data)[q];	\
    d2 = (float)((type *)data)[r];	\
}

    switch(type)
    {
	case TYPE_DOUBLE: GET_Zs(double); break;
	case TYPE_FLOAT:  GET_Zs(float);  break;
	case TYPE_INT:    GET_Zs(int);    break;
	case TYPE_UINT:   GET_Zs(uint);   break;
	case TYPE_SHORT:  GET_Zs(short);  break;
	case TYPE_USHORT: GET_Zs(ushort); break;
	case TYPE_BYTE:   GET_Zs(byte);   break;
	case TYPE_UBYTE:  GET_Zs(ubyte);  break;
        default: break;
    }

#undef GET_Zs

    if (pDim == 2)
    {
	v0.x = p1->x - p0->x; v0.y = p1->y - p0->y; v0.z = 0.0;
	v1.x = p2->x - p0->x; v1.y = p2->y - p0->y; v1.z = 0.0;

	vector_cross(&v1, &v0, &cross);
	scale = vector_length(&cross);
	if (scale == 0.0)
	    return;
	
	if (cross.z >= 0)
	    scale = -scale;
	
	v0.z = d1 - d0;
	v1.z = d2 - d0;

	vector_cross(&v1, &v0, &cross);
	vector_scale(&cross, 1.0/scale, &cross);

	gPtr = gradients + (p<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 

	gPtr = gradients + (q<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 

	gPtr = gradients + (r<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 
    }
    else if (pDim == 3)
    {
	Vector  dp0, dp1, dp2, normal, pvec, gvec, grad;

	vector_subtract(p1, p0, &v0);
	vector_subtract(p2, p0, &v1);

	vector_cross(&v1, &v0, &cross);
	scale = vector_length(&cross);
	if (scale == 0.0)
	    return;
	
	vector_scale(&cross, 1.0/scale, &normal);

	vector_scale(&normal, d0, &dp0);
	vector_scale(&normal, d1, &dp1);
	vector_scale(&normal, d2, &dp2);

	vector_add(&dp0, p0, &dp0);
	vector_add(&dp1, p1, &dp1);
	vector_add(&dp2, p2, &dp2);

	vector_subtract(&dp1, &dp0, &v0);
	vector_subtract(&dp2, &dp0, &v1);

	vector_cross(&v0, &v1, &cross);
	vector_scale(&cross, 1.0/scale, &gvec);

	vector_scale(&normal, vector_dot(&normal, &gvec), &pvec);
	vector_subtract(&gvec, &pvec, &grad);

	vector_add(((Vector *)gradients)+p, &grad, ((Vector *)gradients)+p);
	vector_add(((Vector *)gradients)+q, &grad, ((Vector *)gradients)+q);
	vector_add(((Vector *)gradients)+r, &grad, ((Vector *)gradients)+r);
    }

    counts[p] ++;
    counts[q] ++;
    counts[r] ++;

    return;
}


static void
_dxfQuadsGradient(ArrayHandle points, Pointer data,
	    Type type, float *gradients, byte *counts, int *quad, int pDim)
{
    float   scale;
    Vector  v0, v1, cross;
    float   d0=0, d1=0, d2=0, d3=0;
    float   *gPtr;
    int     p, q, r, s;
    float   pBuf[3], qBuf[3], rBuf[3], sBuf[3];

    p = *quad++;
    q = *quad++;
    r = *quad++;
    s = *quad++;

#define GET_Zs(type)			\
{					\
    d0 = (float)((type *)data)[p];	\
    d1 = (float)((type *)data)[q];	\
    d2 = (float)((type *)data)[r];	\
    d3 = (float)((type *)data)[s];	\
}

    switch(type)
    {
	case TYPE_DOUBLE: GET_Zs(double); break;
	case TYPE_FLOAT:  GET_Zs(float);  break;
	case TYPE_INT:    GET_Zs(int);    break;
	case TYPE_UINT:   GET_Zs(uint);   break;
	case TYPE_SHORT:  GET_Zs(short);  break;
	case TYPE_USHORT: GET_Zs(ushort); break;
	case TYPE_BYTE:   GET_Zs(byte);   break;
	case TYPE_UBYTE:  GET_Zs(ubyte);  break;
        default: break;
    }

#undef GET_Zs

    if (pDim == 2)
    {
	float *pPtr, *qPtr, *rPtr, *sPtr;

	pPtr = (float *)DXGetArrayEntry(points, p, (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(points, q, (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(points, r, (Pointer)rBuf);
	sPtr = (float *)DXGetArrayEntry(points, s, (Pointer)sBuf);

	v0.x = sPtr[0] - pPtr[0]; v0.y = sPtr[1] - pPtr[1]; v0.z = 0.0;
	v1.x = qPtr[0] - rPtr[0]; v1.y = qPtr[1] - rPtr[1]; v1.z = 0.0;

	vector_cross(&v1, &v0, &cross);
	scale = vector_length(&cross);
	if (scale == 0.0)
	    return;
	
	if (cross.z >=  0)
	    scale = -scale;
	
	v0.z = d3 - d0;
	v1.z = d1 - d2;

	vector_cross(&v1, &v0, &cross);
	vector_scale(&cross, 1.0/scale, &cross);

	gPtr = gradients + (p<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 

	gPtr = gradients + (q<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 

	gPtr = gradients + (r<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 

	gPtr = gradients + (s<<1);
	*gPtr++ += cross.x; 
	*gPtr++ += cross.y; 
    }
    else if (pDim == 3)
    {
	Vector  *p0, *p1, *p2, *p3;
	Vector  dp0, dp1, dp2, dp3, normal, pvec, gvec, grad;

	p0 = (Vector *)DXGetArrayEntry(points, p, (Pointer)pBuf);
	p1 = (Vector *)DXGetArrayEntry(points, q, (Pointer)qBuf);
	p2 = (Vector *)DXGetArrayEntry(points, r, (Pointer)rBuf);
	p3 = (Vector *)DXGetArrayEntry(points, s, (Pointer)sBuf);

	vector_subtract(p3, p0, &v0);
	vector_subtract(p1, p2, &v1);

	vector_cross(&v0, &v1, &cross);
	scale = vector_length(&cross);
	if (scale == 0.0)
	    return;
	
	vector_scale(&cross, 1.0/scale, &normal);

	vector_scale(&normal, d0, &dp0);
	vector_scale(&normal, d1, &dp1);
	vector_scale(&normal, d2, &dp2);
	vector_scale(&normal, d3, &dp3);

	vector_add(&dp0, p0, &dp0);
	vector_add(&dp1, p1, &dp1);
	vector_add(&dp2, p2, &dp2);
	vector_add(&dp3, p3, &dp3);

	vector_subtract(&dp3, &dp0, &v0);
	vector_subtract(&dp1, &dp2, &v1);

	vector_cross(&v1, &v0, &cross);
	vector_scale(&cross, 1.0/scale, &gvec);

	vector_scale(&normal, vector_dot(&normal, &gvec), &pvec);
	vector_subtract(&gvec, &pvec, &grad);

	vector_add(((Vector *)gradients)+p, &grad, ((Vector *)gradients)+p);
	vector_add(((Vector *)gradients)+q, &grad, ((Vector *)gradients)+q);
	vector_add(((Vector *)gradients)+r, &grad, ((Vector *)gradients)+r);
	vector_add(((Vector *)gradients)+s, &grad, ((Vector *)gradients)+s);
    }

    counts[p] ++;
    counts[q] ++;
    counts[r] ++;
    counts[s] ++;

    return;
}

#if 0
static Error
Invert(float *m, float *s, float *r)
{
    double i;
    float  M[9];

    M[0] = m[4]*m[8] - m[5]*m[7];
    M[3] = m[5]*m[6] - m[3]*m[8];
    M[6] = m[3]*m[7] - m[4]*m[6];

    i = m[0]*M[0] + m[1]*M[3] + m[2]*M[6];
    if (i == 0.0)
	return ERROR;

    M[1] = m[7]*m[2] - m[8]*m[1];
    M[4] = m[8]*m[0] - m[6]*m[2];
    M[7] = m[6]*m[1] - m[7]*m[0];

    M[2] = m[1]*m[5] - m[2]*m[4];
    M[5] = m[2]*m[3] - m[0]*m[5];
    M[8] = m[0]*m[4] - m[1]*m[3];

    i = 1.0 / i;

    r[0] = -i * (M[0]*s[0]+M[1]*s[1]+M[2]*s[2]);
    r[1] = -i * (M[3]*s[0]+M[4]*s[1]+M[5]*s[2]);
    r[2] = -i * (M[6]*s[0]+M[7]*s[1]+M[8]*s[2]);
	
    return OK;
}
#endif

