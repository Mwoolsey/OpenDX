/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_rubbersheet.c,v 1.5 2000/08/24 20:04:20 davidt Exp $:
 */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "_rubbersheet.h"
#include "vectors.h"
#include "_normals.h"

#include <dxconfig.h>


typedef struct
{
    Field	     field;
    float	     scale, offset;
} Params;

enum PrimType
    { PRIM_LINES, PRIM_TRIANGLES, PRIM_QUADS, PRIM_INVALID};

#define TRIANGLE(i,j,k)		  \
    *outConnections++ = base + i; \
    *outConnections++ = base + j; \
    *outConnections++ = base + k;

#define QUAD(i,j,k,l)		  \
    *outConnections++ = base + i; \
    *outConnections++ = base + j; \
    *outConnections++ = base + k; \
    *outConnections++ = base + l;

static enum  PrimType GetPrimType(Field);
static Error GetQuadNormal(ArrayHandle, ArrayHandle, int, float *);
static Error GetQuadNormals(ArrayHandle, ArrayHandle, int, float **);
static Error GetTriNormal(ArrayHandle, ArrayHandle, int, float *);
static Error GetTriNormals(ArrayHandle, ArrayHandle, int, float **);
static Error _GetTriNormal(float *, float *, float *, float *);
static Error _GetQuadNormal(float *, float *, float *, float *, float *);
static Error RS_Object(Object, Object, Object, Object);
static Error RS_Field(Object, float, float);
static Error RS_Field_task(Pointer);
static Error RS_Field_PDep_task(Field, float, float);
static Error RS_Field_CDep_task(Field, int, int, float, float);
static Error SetDefaultColor(Field, int);
static Error ValidForRubberSheet(Object);
static Error GetScaleAndOffset(Object, Object, Object, Object,
						float *, float *);

Error
_dxfRubberSheet(Object ino0, Object scale, Object min, Object max, Object *outo)
{
    Object copy = NULL;

    *outo = NULL;

    if (NULL == (copy = DXCopy(ino0, COPY_STRUCTURE)))
	goto error;

    if (! DXCreateTaskGroup())
	goto error;

    if (! RS_Object(copy, scale, min, max))
    {
	DXAbortTaskGroup();
	goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    *outo = copy;
    return OK;

error:
    DXDelete((Object)copy);
    return ERROR;
}

static Error
RS_Object(Object object, Object scale_o, Object min_o, Object max_o)
{
    Class class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);

    switch(class)
    {
	case CLASS_FIELD:
	{
	    float s, o;

	    if (DXEmptyField((Field)object))
		return OK;

	    if (! GetScaleAndOffset(object, scale_o, min_o, max_o, &s, &o))
	        return ERROR;
	    
	    if (! RS_Field(object, s, o))
		return ERROR;

	   if (! DXSetFloatAttribute(object, "RubberSheet scale", s))
	       return ERROR;
	    
	    return OK;
	}

	case CLASS_SERIES:
	case CLASS_COMPOSITEFIELD:
	case CLASS_MULTIGRID:
	{
	    float s, o;

	    if (! GetScaleAndOffset(object, scale_o, min_o, max_o, &s, &o))
	        return ERROR;
	    
	    if (! RS_Field(object, s, o))
		return ERROR;

	   if (! DXSetFloatAttribute(object, "RubberSheet scale", s))
	       return ERROR;
	    
	    return OK;
	}

	case CLASS_GROUP:
	{
	    Object c;
	    int    i;
	    Group  g = (Group)object;

	    for (i = 0; NULL != (c = DXGetEnumeratedMember(g, i, NULL)); i++)
		if (! RS_Object(c, scale_o, min_o, max_o))
		    return ERROR;

	    return OK;
	}

	case CLASS_XFORM:
	{
	    Object c;

	    if (! DXGetXformInfo((Xform)object, &c, NULL))
		return ERROR;

	    if (! RS_Object(c, scale_o, min_o, max_o))
		return ERROR;
	    
	    return OK;
	}

	case CLASS_CLIPPED:
	{
	    Object c;

	    if (! DXGetClippedInfo((Clipped)object, &c, NULL))
		return ERROR;

	    if (! RS_Object(c, scale_o, min_o, max_o))
		return ERROR;
	    
	    return OK;
	}

	case CLASS_SCREEN:
	{
	    Object c;

	    if (! DXGetScreenInfo((Screen)object, &c, NULL, NULL))
		return ERROR;

	    if (! RS_Object(c, scale_o, min_o, max_o))
		return ERROR;
	    
	    return OK;
	}

	default:
	    DXSetError(ERROR_BAD_CLASS,
		"unknown object encountered in traversal");
	    return ERROR;
    }

}
	    
static Error
RS_Field(Object object, float scale, float offset)
{
    Class class = DXGetObjectClass(object);
    
    switch(class)
    {
	case CLASS_FIELD:
	{
	    Params params;

	    if (DXEmptyField((Field)object))
		return OK;

	    if (! ValidForRubberSheet(object))
		return ERROR;

	    params.field  = (Field)object;
	    params.scale  = scale;
	    params.offset = offset;

	    if (! DXAddTask(RS_Field_task,
			(Pointer)&params, sizeof(params), 1.0))
		return ERROR;

	    return OK;
	}

	case CLASS_GROUP:
	{
	    Group g = (Group)object;
	    int i;
	    Object c;

	    for (i = 0; NULL != (c = DXGetEnumeratedMember(g, i, NULL)); i++)
		if (! RS_Field(c, scale, offset))
		    return ERROR;
	    
	    return OK;
	}

	case CLASS_XFORM:
	{
	    Object c;

	    if (! DXGetXformInfo((Xform)object, &c, NULL))
		return ERROR;

	    if (! RS_Field(c, scale, offset))
		return ERROR;
	    
	    return OK;
	}

	case CLASS_CLIPPED:
	{
	    Object c;

	    if (! DXGetClippedInfo((Clipped)object, &c, NULL))
		return ERROR;

	    if (! RS_Field(c, scale, offset))
		return ERROR;
	    
	    return OK;
	}

	case CLASS_SCREEN:
	{
	    Object c;

	    if (! DXGetScreenInfo((Screen)object, &c, NULL, NULL))
		return ERROR;

	    if (! RS_Field(c, scale, offset))
		return ERROR;
	    
	    return OK;
	}

	default:
	    DXSetError(ERROR_BAD_CLASS,
		"unknown object encountered in traversal");
	    return ERROR;
    }
}

static Error
RS_Field_task(Pointer ptr)
{
    Params *params;
    float  scale, offset;
    Field  field;
    Object attr;
    int	   dDepP, nDepP;

    params = (Params *)ptr;

    field  = params->field;
    scale  = params->scale;
    offset = params->offset;

    if (DXEmptyField(field))
	return OK;

    attr = DXGetComponentAttribute(field, "data", "dep");
    if (! attr || DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "invalid or nonexistant data dependency attribute");
	return ERROR;
    }

    if (!strcmp(DXGetString((String)attr), "positions"))
	dDepP = 1;
    else if (!strcmp(DXGetString((String)attr), "connections"))
	dDepP = 0;
    else
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "invalid data dependency: %s", DXGetString((String)attr));
	return ERROR;
    }
    
    /*
     * If there are no normals input, then by default, normals
     * dependency follows data.  If there is a normals component,
     * then its dependency overrides.
     */
    nDepP = dDepP;
    if (DXGetComponentValue(field, "normals"))
    {
	attr = DXGetComponentAttribute(field, "normals", "dep");
	if (! attr || DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"invalid or nonexistant normals dependency attribute");
	    return ERROR;
	}

	if (!strcmp(DXGetString((String)attr), "positions"))
	    nDepP = 1;
	else if (!strcmp(DXGetString((String)attr), "connections"))
	    nDepP = 0;
	else
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"invalid normals dependency: %s",
					DXGetString((String)attr));
	    return ERROR;
	}
    }

    if (dDepP && nDepP)
    {
	if (! RS_Field_PDep_task(field, scale, offset))
	    return ERROR;
    }
    else
    {
	if (! RS_Field_CDep_task(field, nDepP, dDepP, scale, offset))
	    return ERROR;
    }

    if (! _dxfNormalsObject((Object)field, "data"))
	return ERROR;

    DXDeleteComponent(field, "box");
    
    if (! DXEndField(field))
	return ERROR;
    
    return OK;
}

#define STRETCH_POSITIONS_VARYING_NORMAL(type)				\
{									\
    type *ptr = (type *)data;						\
									\
    for (i = 0; i < nPositions; i++)					\
    {									\
	float *pt = (float *)DXGetArrayEntry(pHandle, i, pbuf);	\
									\
	vector_scale((Vector *)v_normal,				\
	    scale * (*ptr + offset), (Vector *)s_normal);	        \
	vector_add((Vector *)pt, (Vector *)s_normal,			\
	(Vector *)outPositions);    		  			\
									\
	ptr	     += 1;						\
	v_normal     += 3;						\
	outPositions += 3;						\
    }									\
}

#define STRETCH_POSITIONS_CONSTANT_NORMAL(type)				\
{									\
    type *ptr = (type *)data;						\
    for (i = 0; i < nPositions; i++)					\
    {									\
	float *pt = (float *)DXGetArrayEntry(pHandle, i, pbuf);	\
									\
	vector_scale((Vector *)c_normal,				\
	    scale * (*ptr + offset), (Vector *)s_normal);	        \
	vector_add((Vector *)pt, (Vector *)s_normal,			\
	(Vector *)outPositions);    		 			\
									\
	ptr	     += 1;						\
	outPositions += 3;						\
    }									\
}

#define STRETCH_POSITIONS_2D(type)					\
{									\
    type *ptr = (type *)data;						\
    for (i = 0; i < nPositions; i++)					\
    {									\
	float *pt = (float *)DXGetArrayEntry(pHandle, i, pbuf);	\
	for (j = 0; j < nDim; j++)					\
	    *outPositions++ = *pt++;					\
									\
	*outPositions++ = (*ptr++ + offset) * scale;			\
    }									\
}

static Error
RS_Field_PDep_task(Field field, float scale, float offset)
{
    Array	  cArray, dArray;
    Array	  pArrayIn, pArrayOut;
    float	  *outPositions;
    float	  *data;
    enum PrimType primType;
    int		  nPositions, nDim;
    Type	  type, dataType;
    Category	  cat;
    int		  rank, shape[32];
    ArrayHandle   pHandle = NULL;
    ArrayHandle   cHandle = NULL;
    Pointer	  pbuf = NULL;

    pArrayOut  = NULL;

    if ((primType = GetPrimType(field)) == PRIM_INVALID)
    goto error;

    cArray = (Array)DXGetComponentValue(field, "connections");
    if (! cArray)
    {
	DXSetError(ERROR_UNEXPECTED, "missing connections");
	goto error;
    }

    dArray = (Array)DXGetComponentValue(field, "data");
    if (! dArray)
    {
	DXSetError(ERROR_UNEXPECTED, "missing data");
	goto error;
    }

    DXGetArrayInfo(dArray, NULL, &dataType, NULL, NULL, NULL);

    pArrayIn = (Array)DXGetComponentValue(field, "positions");
    DXGetArrayInfo(pArrayIn, &nPositions, &type, &cat, &rank, shape);
    if (type != TYPE_FLOAT || cat != CATEGORY_REAL || rank != 1)
    {
	DXSetError(ERROR_UNEXPECTED, "type/category/rank error");
	goto error;
    }

    nDim = shape[0];

    pHandle = DXCreateArrayHandle(pArrayIn);
    pbuf    = DXAllocate(DXGetItemSize(pArrayIn));
    if (! pHandle || ! pbuf)
	goto error;

    data = (float *)DXGetArrayData(dArray);

    /*
    * If there are already 3 dimensions, we move the point in the normal
    * direction.  Otherwise, we add the next dimension.
    */
    if (nDim == 3)
    {
	Array nArray;

	pArrayOut = DXNewArrayV(type, cat, rank, shape);
	if (! pArrayOut)
	    goto error;

	if (! DXAddArrayData(pArrayOut, 0, nPositions, NULL))
	    goto error;

	outPositions = (float *)DXGetArrayData(pArrayOut);

	/*
	 * We need a direction in which to stretch the point.  If there are
	 * position normals, use them.  Otherwise, assume the data is planar
	 * and get the normal to the plane.
	 */
	nArray = (Array)DXGetComponentValue(field, "normals");

	if (nArray && !DXQueryConstantArray(nArray, NULL, NULL))
	{
	    Object att;
	    float *v_normal, s_normal[3];
	    int   i;

	    att = DXGetComponentAttribute(field, "normals", "dep");
	    if (! att || DXGetObjectClass(att) != CLASS_STRING)
	    {
		DXSetError(ERROR_DATA_INVALID,
			"bad normals dependency attribute");
		goto error;
	    }

	    if (strcmp(DXGetString((String)att), "positions"))
	    {
		DXSetError(ERROR_DATA_INVALID,
			"normals dependency must match data dependency");
		goto error;
	    }
	       
	    v_normal = (float *)DXGetArrayData(nArray);

	    switch(dataType)
	    {
		case TYPE_DOUBLE:
		    STRETCH_POSITIONS_VARYING_NORMAL(double);
		    break;

		case TYPE_FLOAT:
		    STRETCH_POSITIONS_VARYING_NORMAL(float);
		    break;

		case TYPE_UBYTE:
		    STRETCH_POSITIONS_VARYING_NORMAL(ubyte);
		    break;

		case TYPE_BYTE:
		    STRETCH_POSITIONS_VARYING_NORMAL(byte);
		    break;

		case TYPE_USHORT:
		    STRETCH_POSITIONS_VARYING_NORMAL(ushort);
		    break;

		case TYPE_SHORT:
		    STRETCH_POSITIONS_VARYING_NORMAL(short);
		    break;

		case TYPE_UINT:
		    STRETCH_POSITIONS_VARYING_NORMAL(uint);
		    break;

		case TYPE_INT:
		    STRETCH_POSITIONS_VARYING_NORMAL(int);
		    break;
	        default:
		    break;
	    }
	}
	else 
	{
	    float *c_normal, c_normal_buf[3], s_normal[3];
	    int   nConnections;
	    int   i;

	    DXGetArrayInfo(cArray, &nConnections, NULL, NULL, NULL, NULL);

	    /*
	     * Now if there is a normals array, it must be constant
	     */
	    if (nArray)
	    {
		c_normal = (float *)DXGetConstantArrayData(nArray);
	    }
	    else
	    {
		cHandle = DXCreateArrayHandle(cArray);
		if (! cHandle)
		    goto error;

		if (primType == PRIM_TRIANGLES)
		{
		    if (! GetTriNormal(pHandle, cHandle,
						    nConnections, c_normal_buf))
		    {
			DXSetError(ERROR_DATA_INVALID,
			    "unable to derive a surface normal");
			goto error;
		    }
		}
		else if (primType == PRIM_QUADS)
		{
		    if (! GetQuadNormal(pHandle, cHandle,
						nConnections, c_normal_buf))
		    {
			DXSetError(ERROR_DATA_INVALID,
			    "unable to derive a surface normal");
			goto error;
		    }
		}
		else if (primType == PRIM_LINES)
		{
		    DXSetError(ERROR_DATA_INVALID,
			    "require position normals for 3D lines");
		    goto error;
		}

		c_normal = c_normal_buf;
		DXFreeArrayHandle(cHandle);
		cHandle = NULL;
	    }

	    switch(dataType)
	    {
		case TYPE_DOUBLE:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(double);
		    break;

		case TYPE_FLOAT:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(float);
		    break;

		case TYPE_UBYTE:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(ubyte);
		    break;

		case TYPE_BYTE:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(byte);
		    break;

		case TYPE_USHORT:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(ushort);
		    break;

		case TYPE_SHORT:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(short);
		    break;

		case TYPE_UINT:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(uint);
		    break;

		case TYPE_INT:
		    STRETCH_POSITIONS_CONSTANT_NORMAL(int);
		    break;
	        default:
		    break;
	    }

	}
    }
    else if (nDim == 2 || nDim == 1)
    {
	int i, j;

	shape[0] ++;

	pArrayOut = DXNewArrayV(type, cat, rank, shape);
	if (! pArrayOut)
	    goto error;

	if (! DXAddArrayData(pArrayOut, 0, nPositions, NULL))
	    goto error;

	outPositions = (float *)DXGetArrayData(pArrayOut);

	switch(dataType)
	{
	    case TYPE_DOUBLE:
		STRETCH_POSITIONS_2D(double);
		break;

	    case TYPE_FLOAT:
		STRETCH_POSITIONS_2D(float);
		break;

	    case TYPE_UBYTE:
		STRETCH_POSITIONS_2D(ubyte);
		break;

	    case TYPE_BYTE:
		STRETCH_POSITIONS_2D(byte);
		break;

	    case TYPE_USHORT:
		STRETCH_POSITIONS_2D(ushort);
		break;

	    case TYPE_SHORT:
		STRETCH_POSITIONS_2D(short);
		break;

	    case TYPE_UINT:
		STRETCH_POSITIONS_2D(uint);
		break;

	    case TYPE_INT:
		STRETCH_POSITIONS_2D(int);
		break;
	    default:
	        break;
	}
    }

    DXSetComponentValue(field, "positions", (Object)pArrayOut);
    pArrayOut = NULL;

    /*
     * If there's a colors component, use it.  Otherwise, create one.
     */
    if (!DXGetComponentValue(field, "colors") && 
		    !DXGetComponentValue(field, "front colors"))
	if (! SetDefaultColor(field, nPositions))
	    goto error;

    DXDeleteComponent(field, "normals");

    DXFreeArrayHandle(pHandle);
    DXFreeArrayHandle(cHandle);
    DXFree(pbuf);

    DXEndField(field);

    return OK;

error:
    DXFreeArrayHandle(pHandle);
    DXFreeArrayHandle(cHandle);
    DXFree(pbuf);
    DXDelete((Object)pArrayOut);
    return ERROR;
}

#define STRETCH_CONNECTIONS_3D(type)					\
{									\
    type *data;								\
									\
    data = (type *)inData;						\
    for (i = 0; i < nConnections; i++)					\
    {									\
	int *elt;							\
									\
	elt = (int *)DXGetArrayEntry(cHandle, i, (Pointer)ebuf);  \
									\
	if (! dDepP)							\
	{								\
	    scaledData = scale * (data[i] + offset);			\
									\
	    if (! nDepP || cst_normal) {				\
	        if (cst_normal)						\
	        {							\
		    vector_scale((Vector *)cst_normal,			\
		        scaledData, (Vector *)s_normal);		\
	        }							\
	        else							\
	        {							\
		    vector_scale(((Vector *)normals)+i,			\
		        scaledData, (Vector *)s_normal);		\
	        }							\
	    }								\
	}								\
									\
	for (j = 0; j < nVerts; j++)					\
	{								\
	    float *pt, pbuf[3];						\
	    int   pNum;							\
									\
	    pNum = *elt++;						\
									\
	    pt = (float *)DXGetArrayEntry(pHandle,		\
					pNum, (Pointer)pbuf);		\
									\
	    *outPositions++ = pt[0];					\
	    *outPositions++ = pt[1];					\
	    *outPositions++ = pt[2];					\
									\
	    if (dDepP)							\
	    {								\
		scaledData = scale * (data[pNum] + offset);		\
		if (! nDepP) {						\
		    if (cst_normal)					\
			vector_scale(((Vector *)cst_normal),		\
				scaledData, (Vector *)s_normal);	\
		    else						\
			vector_scale(((Vector *)normals)+i,		\
				scaledData, (Vector *)s_normal);	\
		}							\
	    }								\
	    else							\
	    {								\
		if (nDepP && !cst_normal)				\
		    vector_scale(((Vector *)normals)+pNum,		\
				scaledData, (Vector *)s_normal);	\
	    }								\
									\
	    vector_add((Vector *)pt,					\
		(Vector *)s_normal, (Vector *)outPositions);		\
									\
	    outPositions += 3;						\
	}								\
									\
    }									\
}

#define STRETCH_CONNECTIONS_2D(type)					\
{									\
    type *data = (type *)inData;					\
									\
    for (i = 0; i < nConnections; i++)					\
    {									\
	int *elt;							\
									\
	elt = (int *)DXGetArrayEntry(cHandle, i, (Pointer)ebuf); 	\
									\
	if (!dDepP)							\
	    scaledData = scale * (data[i] + offset);			\
									\
	for (j = 0; j < nVerts; j++)					\
	{								\
	    float *pt, pbuf[2];						\
	    int   pNum;							\
									\
	    pNum = *elt++;						\
									\
	    if (dDepP)							\
		scaledData = scale * (data[pNum] + offset);		\
									\
	    pt = (float *)DXGetArrayEntry(pHandle,		\
					pNum, (Pointer)pbuf);		\
									\
	    for (k = 0; k < nDim; k++)					\
		*outPositions++ = pt[k];				\
	    *outPositions++ = 0.0;					\
									\
	    for (k = 0; k < nDim; k++)					\
		*outPositions++ = pt[k];				\
	    *outPositions++ = scaledData;				\
	}								\
    }									\
}

static Error
RS_Field_CDep_task(Field field, int nDepP, int dDepP, float scale, float offset)
{
    Array	  dArrayIn;
    Array	  pArrayIn, pArrayOut;
    Array	  cArrayIn, cArrayOut;
    float	  *outPositions;
    int		  *outConnections;
    Pointer	  inData;
    int		  nConnections;
    enum PrimType primType;
    int		  nPositions, nDim, nVerts, outPerIn=0;
    Type	  type, dataType;
    Category	  cat;
    int		  rank, shape[32], i, j, k;
    float 	  scaledData=0;
    float	  s_normal[3], *c_normals = NULL;
    Object	  attr;
    char	  *name;
    Array 	  inA, outA = NULL;
    ArrayHandle   pHandle = NULL;
    ArrayHandle   cHandle = NULL;
    int 	  *ebuf = NULL;
    char	  *toDelete[100];
    int   	  nDelete;
    InvalidComponentHandle iInv = NULL, oInv = NULL;

    cArrayOut   = NULL;
    pArrayOut   = NULL;

    if (DXEmptyField(field))
	return OK;
    
    if (! DXInvalidateConnections((Object)field))
	goto error;

    if ((primType = GetPrimType(field)) == PRIM_INVALID)
	goto error;
    
    cArrayIn = (Array)DXGetComponentValue (field, "connections");
    if (! cArrayIn)
    {
	DXSetError(ERROR_UNEXPECTED, "missing connections");
	goto error;
    }

    DXGetArrayInfo(cArrayIn, &nConnections, NULL, NULL, NULL, &nVerts);

    dArrayIn = (Array)DXGetComponentValue (field, "data");
    if (! dArrayIn)
    {
	DXSetError(ERROR_UNEXPECTED, "missing data");
	goto error;
    }
    DXGetArrayInfo(dArrayIn, NULL, &dataType, NULL, NULL, NULL);

    pArrayIn = (Array)DXGetComponentValue(field, "positions");
    if (! pArrayIn)
    {
	DXSetError(ERROR_UNEXPECTED, "missing positions");
	goto error;
    }

    DXGetArrayInfo(pArrayIn, &nPositions, &type, &cat, &rank, shape);

    nDim = shape[0];

    if (nDim > 3)
    {
	DXSetError(ERROR_DATA_INVALID, ">3D positions not supported");
	goto error;
    }

    cHandle = DXCreateArrayHandle(cArrayIn);
    pHandle = DXCreateArrayHandle(pArrayIn);
    if (! cHandle || ! pHandle)
	goto error;
    
    ebuf = (int *)DXAllocate(nVerts*sizeof(int));
    if (! ebuf)
	goto error;

    inData = DXGetArrayData(dArrayIn);

    if (DXGetComponentValue(field, "invalid connections"))
    {
	iInv = DXCreateInvalidComponentHandle((Object)field, 
			NULL, "connections");
	if (! iInv)
	    goto error;
    }

    if (nDim == 3)
    {
	float *normals    = NULL;
	float *cst_normal = NULL;
	Array nArray;

	/*
	 * Get normals.  Options are: position dependent normals,
	 * connections dependent normals and none at all.  In the
	 * last case, generate one dep on connections.
	 */
	nArray = (Array)DXGetComponentValue(field, "normals");
	if (nArray && DXQueryConstantArray(nArray, NULL, NULL))
	{
	    cst_normal = (float *)DXGetConstantArrayData(nArray);
	}
	else if (nArray)
	{
	    normals = (float *)DXGetArrayData(nArray);
	}
	else if (primType == PRIM_TRIANGLES)
	{
	    if (! GetTriNormals(pHandle, cHandle, nConnections, &normals))
		goto error;
	}
	else if (primType == PRIM_QUADS)
	{
	    if (! GetQuadNormals(pHandle, cHandle, nConnections, &normals))
		goto error;
	}
	else if (primType == PRIM_LINES)
	{
	    DXSetError(ERROR_MISSING_DATA,
		"normals component required for lines in 3-space");
	    goto error;
	}

	pArrayOut = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (! DXAddArrayData(pArrayOut, 0, 2*nVerts*nConnections, NULL))
	    goto error;

	outPositions = (float *)DXGetArrayData(pArrayOut);

	switch(dataType)
	{
	    case TYPE_DOUBLE:
		STRETCH_CONNECTIONS_3D(double);
		break;

	    case TYPE_FLOAT:
		STRETCH_CONNECTIONS_3D(float);
		break;

	    case TYPE_INT:
		STRETCH_CONNECTIONS_3D(int);
		break;

	    case TYPE_UINT:
		STRETCH_CONNECTIONS_3D(uint);
		break;

	    case TYPE_SHORT:
		STRETCH_CONNECTIONS_3D(short);
		break;

	    case TYPE_USHORT:
		STRETCH_CONNECTIONS_3D(ushort);
		break;

	    case TYPE_UBYTE:
		STRETCH_CONNECTIONS_3D(ubyte);
		break;

	    case TYPE_BYTE:
		STRETCH_CONNECTIONS_3D(byte);
		break;
	    default:
	        break;
	}

	if (! nArray && normals)
	    DXFree((Pointer)normals);
    }
    else
    {
	pArrayOut = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nDim+1);
	if (! DXAddArrayData(pArrayOut, 0, 2*nVerts*nConnections, NULL))
	    goto error;

	outPositions = (float *)DXGetArrayData(pArrayOut);

	switch(dataType)
	{
	    case TYPE_DOUBLE:
		STRETCH_CONNECTIONS_2D(double);
		break;

	    case TYPE_FLOAT:
		STRETCH_CONNECTIONS_2D(float);
		break;

	    case TYPE_UINT:
		STRETCH_CONNECTIONS_2D(uint);
		break;

	    case TYPE_INT:
		STRETCH_CONNECTIONS_2D(int);
		break;

	    case TYPE_USHORT:
		STRETCH_CONNECTIONS_2D(ushort);
		break;

	    case TYPE_SHORT:
		STRETCH_CONNECTIONS_2D(short);
		break;

	    case TYPE_BYTE:
		STRETCH_CONNECTIONS_2D(byte);
		break;

	    case TYPE_UBYTE:
		STRETCH_CONNECTIONS_2D(ubyte);
		break;
	    default:
	        break;
	}
    }

    
    switch (primType)
    {
	case PRIM_TRIANGLES:
	{
	    cArrayOut = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	    if (! cArrayOut)
		goto error;

	    if (! DXAddArrayData(cArrayOut, 0, 8*nConnections, NULL))
		goto error;

	    outConnections = (int *)DXGetArrayData(cArrayOut);

	    for (i = 0; i < nConnections; i++)
	    {
		int base;

		/*
		 * base is the index of the first of the 6 points
		 * corresponding to input triangle i (and hence output prism
		 * i).
		 */
		base = i*6;

		TRIANGLE(1,3,5);
		TRIANGLE(0,4,2);
		TRIANGLE(5,3,2);
		TRIANGLE(5,2,4);
		TRIANGLE(3,1,0);
		TRIANGLE(3,0,2);
		TRIANGLE(1,5,4);
		TRIANGLE(1,4,0);
	    }
	
	    outPerIn = 8;

	}
	break;

	case PRIM_QUADS:
	{
	    cArrayOut = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
	    if (! cArrayOut)
		goto error;

	    if (! DXAddArrayData(cArrayOut, 0, 6*nConnections, NULL))
		goto error;

	    outConnections = (int   *)DXGetArrayData(cArrayOut);

	    for (i = 0; i < nConnections; i++)
	    {
		int base;

		/*
		 * base is the index of the first of the 8 points
		 * corresponding to input quad i (and hence output box i).
		 */
		base = i<<3;

		QUAD(0,1,2,3);
		QUAD(4,5,6,7);
		QUAD(1,5,0,4);
		QUAD(0,4,2,6);
		QUAD(2,6,3,7);
		QUAD(3,7,1,5);
	    }

	    outPerIn = 6;
	}
	break;

	case PRIM_LINES:
	{
	    cArrayOut = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
	    if (! cArrayOut)
		goto error;

	    if (! DXAddArrayData(cArrayOut, 0, nConnections, NULL))
		goto error;

	    outConnections = (int   *)DXGetArrayData(cArrayOut);

	    for (i = 0; i < nConnections; i++)
	    {
		int base;

		/*
		 * base is the index of the first of the 8 points
		 * corresponding to input quad i (and hence output box i).
		 */
		base = i<<2;

		QUAD(0,1,2,3);
	    }

	    outPerIn = 1;
	}
	break;
        default:
	  break;
    }

    /*
     * DXDelete components that are no longer appropriate.
     */
    DXDeleteComponent(field, "normals");
    DXDeleteComponent(field, "box");
    DXDeleteComponent(field, "data statistics");
    DXDeleteComponent(field, "front colors");
    DXDeleteComponent(field, "back colors");
    DXDeleteComponent(field, "invalid positions");

    /*
     * Need to get rid of everything that deps refs or ders positions.
     * Before doing so, delete the connections component to break the recursion
     * chain - we do not want to delete data, for example, and because it is
     * dep on connections, it would get deleted by the recursive call when
     * DXChangedComponentStructure notices that connections refs positions and
     * calls itself recursively on it.
     */
    DXDeleteComponent(field, "connections");

    /*
     * Set new positions and connections arrays.
     */
    if (! DXSetComponentValue(field, "positions",   (Object)pArrayOut))
	goto error;
    pArrayOut = NULL;

    if (! DXSetComponentValue(field, "connections", (Object)cArrayOut))
	goto error;
    cArrayOut = NULL;

    switch(primType)
    {
	case PRIM_LINES:
	case PRIM_QUADS:
	    if (! DXSetComponentAttribute(field, "connections", "element type",
					    (Object)DXNewString("quads")))
		goto error;
	    break;
	case PRIM_TRIANGLES:
	    if (! DXSetComponentAttribute(field, "connections", "element type",
					    (Object)DXNewString("triangles")))
		goto error;
	    break;
        default:
	    break;
    }

    i = 0; nDelete = 0;
    while ((inA=(Array)DXGetEnumeratedComponentValue(field, i++, &name)) != NULL)
    {
	char     *src;
	char     *dst;
	int      itemSize;
	int      j, k;
	Type     t;
	Category c;
	int      n, r, s[32];

	if (! strcmp(name, "connections") || ! strcmp(name, "positions"))
	    continue;

	if ((attr = DXGetComponentAttribute(field, name, "dep")) == NULL)
	    continue;
	
	if (DXGetComponentAttribute(field, name, "ref") ||
	    DXGetComponentAttribute(field, name, "der"))
	{
	    toDelete[nDelete++] = name;
	    continue;
	}
	
	if (! strcmp(DXGetString((String)attr), "positions"))
	{
	    DXGetArrayInfo(inA, &n, &t, &c, &r, s);

	    if (DXQueryConstantArray(inA, NULL, NULL))
	    {
		outA = (Array)DXNewConstantArrayV(2*nVerts*nConnections,
				    DXGetConstantArrayData(inA), t, c, r, s);
	    }
	    else
	    {
		outA = DXNewArrayV(t, c, r, s);
		if (! outA)
		    goto error;

		if (! DXAddArrayData(outA, 0, 2*nVerts*nConnections, NULL))
		    goto error;
		
		itemSize = DXGetItemSize(inA);

		src = (char *)DXGetArrayData(inA);
		dst = (char *)DXGetArrayData(outA);
		for (j = 0; j < nConnections; j++)
		{
		    int *e = (int *)DXGetArrayEntry(cHandle, j, ebuf);
		    
		    for (k = 0; k < nVerts; k++)
		    {
			memcpy(dst, src+e[k]*itemSize, itemSize);
			dst += itemSize;
			memcpy(dst, src+e[k]*itemSize, itemSize);
			dst += itemSize;
		    }
		}

	    }
	}
	else if (! strcmp(DXGetString((String)attr), "connections"))
	{
	    DXGetArrayInfo(inA, &n, &t, &c, &r, s);

	    if (DXQueryConstantArray(inA, NULL, NULL))
	    {
		outA = (Array)DXNewConstantArrayV(outPerIn*n,
				    DXGetConstantArrayData(inA), t, c, r, s);
	    }
	    else
	    {
		outA = DXNewArrayV(t, c, r, s);
		if (! outA)
		    goto error;

		if (! DXAddArrayData(outA, 0, outPerIn*n, NULL))
		    goto error;
		
		itemSize = DXGetItemSize(inA);

		src = (char *)DXGetArrayData(inA);
		dst = (char *)DXGetArrayData(outA);
		for (j = 0; j < n; j++)
		{
		    for (k = 0; k < outPerIn; k++)
		    {
			memcpy(dst, src, itemSize);
			dst += itemSize;
		    }
		    src += itemSize;
		}

	    }
	}
	else
	{
	    DXSetError(ERROR_DATA_INVALID, "invalid dependency: %s",
						DXGetString((String)attr));
	    goto error;
	}

	if (! DXSetComponentValue(field, name, (Object)outA))
	    goto error;
	outA = NULL;
    }


    if (! DXGetComponentValue(field, "colors"))
    {
	if (! SetDefaultColor(field, outPerIn*nConnections))
	    goto error;
	
	if (! DXSetComponentAttribute(field, "colors", "dep", 
					(Object)DXNewString("connections")))
	    goto error;
    }


    if (iInv)
    {
	int k = 0;

	oInv = DXCreateInvalidComponentHandle((Object)cArrayOut, 
				NULL, "connections");
	if (! oInv)
	    goto error;
	    
	for (i = 0; i < nConnections; i++)
	    if (DXIsElementInvalid(iInv, i))
	    {
		for (j = 0; j < outPerIn; j++)
		    if (! DXSetElementInvalid(oInv, k++))
			goto error;
	    }
	    else k += outPerIn;

	if (! DXSaveInvalidComponent(field, oInv))
	    goto error;
    }

    for (i = 0; i < nDelete; i++)
	DXDeleteComponent(field, toDelete[i]);

    if (iInv)
	DXFreeInvalidComponentHandle(iInv);
    if (oInv)
	DXFreeInvalidComponentHandle(oInv);

    DXFree((Pointer)ebuf);
    DXFreeArrayHandle(pHandle);
    DXFreeArrayHandle(cHandle);

    DXEndField(field);

    return OK;

error:
    if (iInv)
	DXFreeInvalidComponentHandle(iInv);
    if (oInv)
	DXFreeInvalidComponentHandle(oInv);
    DXFree((Pointer)ebuf);
    DXFreeArrayHandle(pHandle);
    DXFreeArrayHandle(cHandle);
    DXFree((Pointer)c_normals);
    DXDelete((Object)pArrayOut);
    DXDelete((Object)cArrayOut);
    DXDelete((Object)outA);
    return ERROR;
}

static Error
GetQuadNormal(ArrayHandle pHandle, ArrayHandle cHandle, int n, float *normal)
{
    int   j;

    for (j = 0; j < n; j++, normal += 3)
    {
	int   *quad, quadbuf[4];
	float *p, *q, *r, *s;
	float pbuf[3], qbuf[3], rbuf[3], sbuf[3];

	quad = (int *)DXGetArrayEntry(cHandle, j, (Pointer)quadbuf);
	p = (float *)DXGetArrayEntry(pHandle, quad[0], (Pointer)pbuf);
	q = (float *)DXGetArrayEntry(pHandle, quad[1], (Pointer)qbuf);
	r = (float *)DXGetArrayEntry(pHandle, quad[2], (Pointer)rbuf);
	s = (float *)DXGetArrayEntry(pHandle, quad[3], (Pointer)sbuf);

	if (! _GetQuadNormal(p, q, r, s, normal))
	    continue;

	return OK;

    }

    return ERROR;
}

static Error
GetQuadNormals(ArrayHandle pHandle, ArrayHandle cHandle, int n, float **normals)
{
    int   j;
    float *ptr;

    *normals = (float *)DXAllocateLocal(n * 3 * sizeof(float));
    if (! *normals)
    {
	DXResetError();
	*normals = (float *)DXAllocate(n * 3 * sizeof(float));
    }
    if (! *normals)
	return ERROR;

    ptr = *normals;
    for (j = 0; j < n; j++, ptr += 3)
    {
	int   *quad, quadbuf[4];
	float *p, *q, *r, *s;
	float pbuf[3], qbuf[3], rbuf[3], sbuf[3];

	quad = (int *)DXGetArrayEntry(cHandle, j, (Pointer)quadbuf);
	p = (float *)DXGetArrayEntry(pHandle, quad[0], (Pointer)pbuf);
	q = (float *)DXGetArrayEntry(pHandle, quad[1], (Pointer)qbuf);
	r = (float *)DXGetArrayEntry(pHandle, quad[2], (Pointer)rbuf);
	s = (float *)DXGetArrayEntry(pHandle, quad[3], (Pointer)sbuf);

	if (! _GetQuadNormal(p, q, r, s, ptr))
	    ptr[0] = ptr[1] = ptr[2] = 0;
    }

    return OK;
}

static Error
GetTriNormal(ArrayHandle pHandle, ArrayHandle cHandle, int n, float *normal)
{
    int   j;

    for (j = 0; j < n; j++)
    {
	int   *tri, tbuf[3];
	float *p, *q, *r;
	float pbuf[3], qbuf[3], rbuf[3];

	tri = (int *)DXGetArrayEntry(cHandle, j, (Pointer)tbuf);
	p = (float *)DXGetArrayEntry(pHandle, tri[0], (Pointer)pbuf);
	q = (float *)DXGetArrayEntry(pHandle, tri[1], (Pointer)qbuf);
	r = (float *)DXGetArrayEntry(pHandle, tri[2], (Pointer)rbuf);

	if (_GetTriNormal(p, q, r, normal))
	    return OK;
    }

    return ERROR;
}

static Error
GetTriNormals(ArrayHandle pHandle, ArrayHandle cHandle, int n, float **normals)
{
    int   j;
    float *ptr;

    *normals = (float *)DXAllocateLocal(n * 3 * sizeof(float));
    if (! *normals)
    {
	DXResetError();
	*normals = (float *)DXAllocate(n * 3 * sizeof(float));
    }
    if (! *normals)
	return ERROR;

    ptr = *normals;
    for (j = 0; j < n; j++, ptr+=3)
    {
	int   *tri, tbuf[3];
	float *p, *q, *r, pbuf[3], qbuf[3], rbuf[3];

	tri = (int *)DXGetArrayEntry(cHandle, j, (Pointer)tbuf);
	p = (float *)DXGetArrayEntry(pHandle, tri[0], (Pointer)pbuf);
	q = (float *)DXGetArrayEntry(pHandle, tri[1], (Pointer)qbuf);
	r = (float *)DXGetArrayEntry(pHandle, tri[2], (Pointer)rbuf);

	if (! _GetTriNormal(p, q, r, ptr))
	    ptr[0] = ptr[1] = ptr[2] = 0.0;
    }

    return OK;
}

static Error
_GetTriNormal(float *p, float *q, float *r, float *normal)
{
    float v0[3], v1[3], d;

    vector_subtract((Vector *)q, (Vector *)p, (Vector *)v0);
    vector_subtract((Vector *)r, (Vector *)p, (Vector *)v1);
    vector_cross((Vector *)v0, (Vector *)v1, (Vector *)normal);
    d = vector_length((Vector *)normal);
    if (d == 0.0)
	return ERROR;
    else
	vector_scale((Vector *)normal, 1.0/d, (Vector *)normal);

    return OK;
}

static Error
_GetQuadNormal(float *p, float *q, float *r, float *s, float *normal)
{
    float v0[3], v1[3], d, cross[3];

    vector_subtract((Vector *)q, (Vector *)p, (Vector *)v0);
    vector_subtract((Vector *)r, (Vector *)p, (Vector *)v1);
    vector_cross((Vector *)v1, (Vector *)v0, (Vector *)normal);

    vector_subtract((Vector *)s, (Vector *)q, (Vector *)v0);
    vector_subtract((Vector *)p, (Vector *)q, (Vector *)v1);
    vector_cross((Vector *)v1, (Vector *)v0, (Vector *)cross);
    vector_add((Vector *)normal, (Vector *)cross, (Vector *)normal);

    vector_subtract((Vector *)r, (Vector *)s, (Vector *)v0);
    vector_subtract((Vector *)q, (Vector *)s, (Vector *)v1);
    vector_cross((Vector *)v1, (Vector *)v0, (Vector *)cross);
    vector_add((Vector *)normal, (Vector *)cross, (Vector *)normal);

    vector_subtract((Vector *)p, (Vector *)r, (Vector *)v0);
    vector_subtract((Vector *)s, (Vector *)r, (Vector *)v1);
    vector_cross((Vector *)v1, (Vector *)v0, (Vector *)cross);
    vector_add((Vector *)normal, (Vector *)cross, (Vector *)normal);

    d = vector_length((Vector *)normal);
    if (d == 0.0)
	return ERROR;
    else
	vector_scale((Vector *)normal, 1.0/d, (Vector *)normal);

    return OK;
}

static enum PrimType
GetPrimType(Field field)
{
    Object attr;
    char   *str;

    attr = DXGetComponentAttribute(field, "connections", "element type");
    if (! attr)
    {
	if (! DXGetComponentValue(field, "connections"))
	    DXSetError(ERROR_MISSING_DATA, "connections");
	else
	    DXSetError(ERROR_MISSING_DATA, "element type attribute");
	return PRIM_INVALID;
    }

    if (DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_INTERNAL, "element type attribute not STRING");
	return PRIM_INVALID;
    }

    str = DXGetString((String)attr);

    if (! strcmp(str, "lines"))
	return PRIM_LINES;
    else if (! strcmp(str, "quads"))
	return PRIM_QUADS;
    else if (! strcmp(str, "triangles"))
	return PRIM_TRIANGLES;

    DXSetError(ERROR_DATA_INVALID, "element type %s not supported", str);
    return PRIM_INVALID;
}

static Error
SetDefaultColor(Field field, int n)
{
    RegularArray array;
    float 	 color[3];
    float 	 delta[3];

    color[0] = 0.5; color[1] = 0.7; color[2] = 1.0;
    delta[0] = 0.0; delta[1] = 0.0; delta[2] = 0.0;

    array = DXNewRegularArray(TYPE_FLOAT, 3, n, (Pointer)color, (Pointer)delta);
    if (! array)
	return ERROR;

    DXSetComponentValue(field, "colors", (Object)array);

    return OK;
}

static Error
ValidForRubberSheet(Object object)
{
    Category 	cat;
    int 	rank, shape[32];

    if (! DXGetType(object, NULL, &cat, &rank, shape) ||
	(cat != CATEGORY_REAL) ||
	(! (rank == 0 || (rank == 1 && shape[0] == 1))))
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "invalid data: scalar or 1-vector real data required");
	return ERROR;
    }

    return OK;
}

static Error
GetScaleAndOffset(Object obj, Object scale_o, Object min_o,
			Object max_o, float *scale, float *offset)
{
    float		thickness;
    float		max, min;
    Class		class;

    if (! DXStatistics(obj, "data", &min, &max, NULL, NULL))
    {
        DXAddMessage("#10520","data");
	return ERROR;
    }

    if (min_o)
    {
	if (! DXExtractParameter(min_o, TYPE_FLOAT, 1, 1, (Pointer)&min))
	{
	   class = DXGetObjectClass(min_o);

	   if ((class != CLASS_GROUP) && (class != CLASS_FIELD))
	   {
              /*
	       * min must be a field or a scalar value
	       */
	      DXSetError(ERROR_BAD_PARAMETER, "#10520", "min");
              return ERROR;
           }

	   if (! DXStatistics(min_o, "data", &min, NULL, NULL, NULL))
	   {
              /*
	       * min must be a field or a scalar value
	       */
	      DXAddMessage("#10520","min");
	      return ERROR;
           }
        }

	*offset = -min;
    }
    else
	*offset = 0.0;

    if (max_o)
    {
	if (! DXExtractParameter(max_o, TYPE_FLOAT, 1, 1, (Pointer)&max))
	{
	     class = DXGetObjectClass(max_o);

	     if ((class != CLASS_GROUP) && (class != CLASS_FIELD))
	     {
		/*
		 * max must be a field or a scalar value
		 */
		DXSetError(ERROR_BAD_PARAMETER, "#10520", "max");
		return ERROR;
	     }

	     if (! DXStatistics(max_o, "data", NULL, &max, NULL, NULL))
	     {
		/*
		 * max must be a field or a scalar value
		 */
		DXAddMessage("#10520","max");
		return ERROR;
	     }
       }
    }

    if (scale_o)
    {
        if (! DXExtractParameter (scale_o, TYPE_FLOAT, 1, 1, (Pointer)scale))
	{
            /*
	     * scale must be a scalar value
	     */
            DXSetError (ERROR_BAD_PARAMETER, "#10080", "scale");
            return ERROR;
        }
    }
    else
    {
	Point boxPoints[8];

	if (! DXBoundingBox(obj, boxPoints))
	    return (ERROR);
	
	thickness = DXLength(DXSub(boxPoints[7], boxPoints[0]));
	if (thickness == 0.0)
	{ 
	    *scale = 0;
	}
	else
	{
	    if (min == max)
	    {
		*scale = 1.0;
	    }
	    else 
	    {
		*scale = thickness / (10.0 * (max - min));
	    }
	}
    }

    return  OK;
}
