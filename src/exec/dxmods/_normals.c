/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/_normals.c,v 1.7 2006/01/04 22:00:50 davidt Exp $
 */

#include <dxconfig.h>


#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "vectors.h"
#include "_normals.h"

typedef struct
{
    Object obj;
    float  rad;
} AreaTask;

typedef struct
{
    Field field;
    float radius;
} AreaArgs;

#define NO_DEPENDENCY	   1
#define DEP_ON_POSITIONS   2
#define DEP_ON_CONNECTIONS 3
#define DEP_ON_FACES       4

static Error DetermineFunction(Object, char *, int *, float *);
static Error _dxfNormalsTraverse(Object, char *);
static Error _dxfPositionNormals(Pointer);
static Error _dxfConnectionNormals(Pointer);
static Error _dxfFaceNormals(Pointer);
static Error _dxfAreaNormals(Pointer);
static int   CompareDependency(Object, char *);
static CompositeField GridNormalsCompositeField(CompositeField, int);
static Field GridNormalsField(Field, int);
static Error _DetermineFunction(Object, char *, int *);
static Error _dxfConnectionNormals_Field(Field);
static Array  _dxf_CDep_triangles(Field);
static Array  _dxf_CDep_quads(Field);
static Error _dxfPositionNormals_Field(Field);
static Array  _dxf_PDep_triangles(Field);
static Array  _dxf_PDep_quads(Field);
static Error _dxfAreaNormals_Field(Field, float);
static float  _dxfTriangleArea(float *, float *, float *);
static int    _dxfInside(float *, float, float *);
static float  _dxfDistance(float *, float *);
static Error  _dxfCheckTriangle(int, float, int, int *,
		    int *, int *, float *, float *, float *);
static float  _dxfIntersect(float *, float, float *, float *);
static float  _dxfTriangleArea(float *, float *, float *);


Object
_dxfNormals(Object object, char *method)
{
    Object copy = NULL;

    if (method == NULL)
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "target dependency");
	goto error;
    }

    copy = DXCopy(object, COPY_STRUCTURE);
    if (! copy)
	goto error;

    if (! _dxfNormalsObject(copy, method))
	goto error;
    
    return copy;

error:
    DXDelete(copy);
    return NULL;
}

Object
_dxfNormalsObject(Object o, char *m)
{
    int n;
    char *gm = NULL;

    /*
     * make sure method string is in global memory
     */
    n = strlen(m)+1;
    gm = (char *)DXAllocate(n);
    if (! gm)
	goto error;
    strcpy(gm, m);
    gm[n-1] = '\0';

    if (! DXCreateTaskGroup())
	goto error;
    
    if (! _dxfNormalsTraverse(o, gm))
    {
	DXAbortTaskGroup();
	goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    DXFree((Pointer)gm);

    return o;

error:
    DXFree((Pointer)gm);
    return NULL;
}

static Error
_dxfNormalsTraverse(Object object, char *method)
{
    Class    class;
    int      i;
    Object   child;
    int      dep;
    float    rad;
    Object   comp;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);

    switch(class)
    {
	case CLASS_FIELD:
	    if (DXEmptyField((Field)object))
		return OK;
	    
	    if (NULL != (comp = DXGetComponentValue((Field)object, "connections")))
	    {
		char *str;
		Object eType = DXGetAttribute(comp, "element type");

		if (! eType || DXGetObjectClass(eType) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID,
			"invalid or missing element type attribute");
		    return ERROR;
		}

		str = DXGetString((String)eType);

		if (strcmp(str, "quads") && strcmp(str, "triangles"))
		    return OK;
	    }
	    else if (! DXGetComponentValue((Field)object, "faces"))
		return OK;

	    i = CompareDependency(object, method);
	    if (i == -1)
		goto error;
	    else if (i == 1)
		return OK;

	    if (! DetermineFunction(object, method, &dep, &rad))
		goto error;
	    
	    if (dep == NO_DEPENDENCY)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10450", method);
		goto error;
	    }

	    if (! GridNormalsField((Field)object, dep))
	    {
		if (dep == DEP_ON_POSITIONS && rad <= 0.0)
		{
		    return DXAddTask(_dxfPositionNormals,
			(Pointer)object, 0, 1.0);
		}
		else if (dep == DEP_ON_POSITIONS && rad > 0.0)
		{
		    AreaTask task;

		    task.obj = object;
		    task.rad = rad;

		    return DXAddTask(_dxfAreaNormals,
				    (Pointer)&task, sizeof(task), 1.0);
		}
		else if (dep == DEP_ON_CONNECTIONS)
		{
		    return DXAddTask(_dxfConnectionNormals,(Pointer)object,0,1.0);
		}
		else 
		{
		    return DXAddTask(_dxfFaceNormals,(Pointer)object,0,1.0);
		}
	    }
	    
	    break;
	
	case CLASS_COMPOSITEFIELD:
	    
	    i = CompareDependency(object, method);
	    if (i == -1)
		goto error;
	    else if (i == 1)
		return OK;

	    if (! DetermineFunction(object, method, &dep, &rad))
		goto error;
	    
	    if (GridNormalsCompositeField((CompositeField)object, dep))
		return OK;
	    
	    if (dep == NO_DEPENDENCY)
	    {
		DXSetError(ERROR_DATA_INVALID, "#10450", "method");
		goto error;
	    }

	    /*
	     * positions and area require  special stuff (eg. Grow) at
	     * the CompositeField level
	     */

	    if (dep == DEP_ON_POSITIONS && rad <= 0.0)
	    {
		return DXAddTask(_dxfPositionNormals,(Pointer)object,0,1.0);
	    }
	    else if (dep == DEP_ON_POSITIONS && rad > 0.0)
	    {
		AreaTask task;

		task.obj = object;
		task.rad = rad;

		return DXAddTask(_dxfAreaNormals,
				(Pointer)&task, sizeof(task), 1.0);
	    }

	    /*
	     * else connections or faces, and since we don't do anything
	     * special at the composite field level, so fall through to
	     * the generic group case.
	     */
	
	case CLASS_MULTIGRID:
	case CLASS_SERIES:
	case CLASS_GROUP:

	    i = 0;
	    for (;;)
	    {
	    	child = DXGetEnumeratedMember((Group)object, i++, NULL);
		if (! child)
		    break;

		if (! _dxfNormalsTraverse(child, method))
		    goto error;
	    }
	    
	    break;

	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)object, &child, 0))
		goto error;
	    
	    if (! _dxfNormalsObject(child, method))
		goto error;
	    
	    break;

	case CLASS_CLIPPED:

	    if (! DXGetClippedInfo((Clipped)object, &child, 0))
		goto error;
	    
	    if (! _dxfNormalsObject(child, method))
		goto error;
	    
	    break;
	
	default:
	    break;
    }

    return OK;

error:
    return ERROR;
}

static Error
DetermineFunction(Object o, char *method, int *dep, float *radius)
{
    int i;

    if (method == NULL || !strcmp(method, "manhattan"))
    {
	*radius = -1.0;
	*dep = DEP_ON_POSITIONS;
	return OK;
    }

    for (i=0; method[i]; i++)
	if ((method[i] < '0' || method[i] > '9') && method[i] != '.')
	    break;
    
    if (method[i])
    {
	*radius = 0;
	return _DetermineFunction(o, method, dep);
    }
    else
    {
	if (sscanf(method, "%f", radius) != 1)
	    return ERROR;
	*dep = DEP_ON_POSITIONS;
	return OK;
    }
}

static Error
_DetermineFunction(Object o, char *name, int *dep)
{
    Class class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);

    switch(class)
    {
	case CLASS_FIELD:
	{
	    char *str; Array a;

	    if (DXEmptyField((Field)o))
	    { 
		*dep = NO_DEPENDENCY;
		return OK;
	    }

	    /*
	     * make sure the component is there
	     */
	    a = (Array)DXGetComponentValue((Field)o, name);
	    if (a == NULL)
	    {
		DXSetError(ERROR_MISSING_DATA, "#10240", name);
		*dep = NO_DEPENDENCY;
		return ERROR;
	    }

	    if (! strcmp(name, "connections"))
	    {
		*dep = DEP_ON_CONNECTIONS;
		return OK;
	    }

	    if (! strcmp(name, "faces"))
	    {
		*dep = DEP_ON_FACES;
		return OK;
	    }

	    if (! DXGetStringAttribute((Object)a, "dep", &str))
	    {
		DXSetError(ERROR_DATA_INVALID, "#10255", name, "dep");
		*dep = NO_DEPENDENCY;
		return ERROR;
	    }

	    if (! strcmp(str, "positions"))
	    {
		*dep = DEP_ON_POSITIONS;
		return OK;
	    }
	    else if (! strcmp(str, "connections"))
	    {
		*dep = DEP_ON_CONNECTIONS;
		return OK;
	    }
	    else if (! strcmp(str, "faces"))
	    {
		*dep = DEP_ON_CONNECTIONS;
		return OK;
	    }
	    else
	    {
		DXSetError(ERROR_DATA_INVALID, "#11250", name);
		*dep = NO_DEPENDENCY;
		return ERROR;
	    }
	}

	case CLASS_COMPOSITEFIELD:
	{
	    Object c;
	    int i;

	    i = 0;
	    for (*dep = NO_DEPENDENCY; *dep == NO_DEPENDENCY; )
	    {
		c = DXGetEnumeratedMember((Group)o, i++, NULL);
		if (c == NULL)
		    return OK;
		
		if (! _DetermineFunction(c, name, dep))
		    return ERROR;
	    }

	    return OK;
	}

	default:
	{
	    DXSetError(ERROR_DATA_INVALID, "#11381");
	    return ERROR;
	}
    }
}

static Error
_dxfConnectionNormals(Pointer ptr)
{
     return _dxfConnectionNormals_Field((Field)ptr);
}


#define NORMAL(nv, nd, norm)						    \
{									    \
    float length;							    \
    int i;								    \
									    \
    norm->x = 0.0;							    \
    norm->y = 0.0;							    \
    norm->z = 0.0;							    \
									    \
    for (i = 0; i < nv; i++)						    \
    {									    \
	int j = (i == nv-1) ? 0 : i + 1;				    \
	norm->x += (ptrs[i][1] - ptrs[j][1]) *				    \
				(ptrs[i][2] + ptrs[j][2]);		    \
	norm->y += (ptrs[i][2] - ptrs[j][2]) *				    \
				    (ptrs[i][0] + ptrs[j][0]);		    \
	norm->z += (ptrs[i][0] - ptrs[j][0]) *				    \
				(ptrs[i][1] + ptrs[j][1]);		    \
    }									    \
									    \
    length = vector_length(norm);					    \
    if (length == 0.0)							    \
    {									    \
	norm->x = 0.0;							    \
	norm->y = 0.0;							    \
	norm->z = 1.0;							    \
	goto degenerate;						    \
    }									    \
    else								    \
    {									    \
	float d = 1.0 / length;						    \
	vector_scale(norm, d, norm);					    \
    }									    \
}

static Error
_dxfConnectionNormals_Field(Field field)
{
    Array		array;
    int			rank, shape[10];
    Type		type;
    Category		cat;
    int			n;
    char		*primitive;
    Object 		attr;

    /*
     * Check first whether the input field is empty.
     */

    if (DXEmptyField(field))
	return OK;

    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
	return OK;
    
    DXGetArrayInfo(array, &n, NULL, NULL, NULL,NULL);
    
    DXDeleteComponent(field, "normals");
    
    array = (Array)DXGetComponentValue(field, "positions");
    DXGetArrayInfo(array, NULL, &type, &cat, &rank, shape);

    if (rank != 1 || shape[0] > 3 || shape[0] < 2)
    {
	DXSetError(ERROR_DATA_INVALID, "positions must be 2 or 3-D");
	return ERROR;
    }

    if (shape[0] == 2)
    {
        float normal[3] = {0.0, 0.0, 1.0};
	array = (Array)DXNewConstantArray(n, (Pointer)normal,
		TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    }
    else
    {
	attr = DXGetComponentAttribute(field, "connections", "element type");
	if (!attr)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10255", 
			    "connections", "element type");
	    return ERROR;
	}

	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10200", "element type attribute");
	    return ERROR;
	}

	/*
	 * Check the topological primitive.
	 */
	    primitive = DXGetString((String)attr);

	if (! strcmp(primitive, "triangles"))
	    array = _dxf_CDep_triangles(field);
	else if (! strcmp(primitive, "quads"))
	    array = _dxf_CDep_quads(field);
	else
	    array = NULL;
    }

    if (array)
    {
	/*
	 * Set the components of the output fields.
	 */
	if (! DXSetComponentValue(field, "normals",(Object) array))
	    return ERROR;

	if (! DXSetComponentAttribute(field, "normals", "dep",
				    (Object) DXNewString("connections")))
	    return ERROR;

	if (! DXEndField(field))
	    return ERROR;
    }

    return OK;
}

static Array
_dxf_CDep_triangles(Field field)
{
    Array		a_connections, a_positions;
    Array		a_normals = NULL;
    int			n_connections;
    Vector		*normals;
    int			i, j;
    int			degenerate_count;
    ArrayHandle		cHandle = NULL, pHandle = NULL;
    int			nd;

    /*
     * Do quick type checks to ensure that the components are correct.
     */
    a_connections = (Array)DXGetComponentValue(field, "connections");
    if (!a_connections)
        goto error;

    a_positions = (Array)DXGetComponentValue(field, "positions");
    if (!a_positions)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	goto error;
    }
    
    if (! DXGetArrayInfo(a_positions, &n_connections, NULL, NULL, NULL, &nd))
	goto error;

    if (! DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL))
	goto error;

    a_normals = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (!a_normals)
	goto error;

    /*
     * We now assume that there is at least one connection.
     */

    if (! DXTypeCheck(a_connections, TYPE_INT, CATEGORY_REAL, 1, 3))
	goto error;

    /*
     * Setup the final array information.
     */

    if (! DXAddArrayData(a_normals, 0, n_connections,(Pointer) NULL))
        goto error;

    /*
     * Setup and initialize the components.
     */

    if (NULL ==(normals =(Vector *)DXGetArrayData(a_normals)))
	goto error;

    if (NULL ==(cHandle = DXCreateArrayHandle(a_connections)))
	goto error;
    
    if (NULL ==(pHandle = DXCreateArrayHandle(a_positions)))
	goto error;


    /*
     * For each connection...
     */
    degenerate_count = 0;
    for(i = 0; i < n_connections; i++, normals++)
    {
	int     *element, ebuf[3];
	float	*ptrs[3], buf[9];

	element =(int *)DXCalculateArrayEntry(cHandle, i,(Pointer)ebuf);

	for(j = 0; j < 3; j++)						   
	    ptrs[j] = (float *)DXGetArrayEntry(pHandle, element[j], buf+j*nd); 

	NORMAL(3, nd, normals);

	continue;

degenerate:
	degenerate_count ++;
    }

    /*
     * Set the components of the output fields.
     */
    if (! DXSetComponentValue(field, "normals",(Object) a_normals))
	goto error;

    if (! DXSetComponentAttribute(field, "normals", "dep",
				(Object) DXNewString("connections")))
        goto error;

    if (! DXEndField(field))
	goto error;

    if (degenerate_count > 0)
	DXMessage("Number of degenerate triangles: %d", degenerate_count);

    DXFreeArrayHandle(cHandle);
    DXFreeArrayHandle(pHandle);

    return a_normals;

error:
    if (cHandle)
	DXFreeArrayHandle(cHandle);

    if (pHandle)
	DXFreeArrayHandle(pHandle);

    if (a_normals)
	DXDelete((Object)a_normals);

    return NULL;
	
}

static Array
_dxf_CDep_quads(Field field)
{
    Array		a_connections, a_positions;
    Array		a_normals = NULL;
    int			n_connections;
    Vector		*normals;
    int			i, nd;
    int			degenerate_count;
    ArrayHandle		cHandle = NULL, pHandle = NULL;

    /*
     * Do quick type checks to ensure that the components are correct.
     */
    a_connections = (Array)DXGetComponentValue(field, "connections");
    if (!a_connections)
        goto error;

    a_positions = (Array)DXGetComponentValue(field, "positions");
    if (!a_positions)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	goto error;
    }
    
    if (! DXGetArrayInfo(a_positions, NULL, NULL, NULL, NULL, &nd))
	goto error;

    if (! DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL))
	goto error;

    a_normals = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (!a_normals)
	goto error;

    /*
     * We now assume that there is at least one connection.
     */

    if (! DXTypeCheck(a_connections, TYPE_INT, CATEGORY_REAL, 1, 4))
	goto error;

    /*
     * Setup the final array information.
     */

    if (! DXAddArrayData(a_normals, 0, n_connections,(Pointer) NULL))
        goto error;

    /*
     * Setup and initialize the components.
     */

    if (NULL ==(normals =(Vector *)DXGetArrayData(a_normals)))
	goto error;

    if (NULL ==(cHandle = DXCreateArrayHandle(a_connections)))
	goto error;
    
    if (NULL ==(pHandle = DXCreateArrayHandle(a_positions)))
	goto error;

    /*
     * For each connection...
     */
    degenerate_count = 0;
    for(i = 0; i < n_connections; i++, normals++)
    {
	int *element, ebuf[4];
	float *ptrs[4], buf[12];

	element =(int *)DXCalculateArrayEntry(cHandle, i,(Pointer)ebuf);

	ptrs[0] = (float *)DXGetArrayEntry(pHandle, element[0], buf+0*nd); 
	ptrs[1] = (float *)DXGetArrayEntry(pHandle, element[2], buf+1*nd); 
	ptrs[2] = (float *)DXGetArrayEntry(pHandle, element[3], buf+2*nd); 
	ptrs[3] = (float *)DXGetArrayEntry(pHandle, element[1], buf+3*nd); 

	NORMAL(4, nd, normals);

	continue;

degenerate:
	degenerate_count ++;
    }

    if (degenerate_count > 0)
	DXMessage("Number of degenerate triangles: %d", degenerate_count);

    DXFreeArrayHandle(cHandle);
    DXFreeArrayHandle(pHandle);

    return a_normals;

error:
    if (cHandle)
	DXFreeArrayHandle(cHandle);

    if (pHandle)
	DXFreeArrayHandle(pHandle);

    if (a_normals)
	DXDelete((Object)a_normals);

    return NULL;
	
}

static Error
positionsNormalsTask(Pointer ptr)
{
    Field field = (Field)ptr;

    return _dxfPositionNormals_Field(field);
}

static Error
_dxfPositionNormals(Pointer ptr)
{
    Object object = (Object)ptr;
    Object child;
    int i;
    Class class = DXGetObjectClass(object);

    if (class == CLASS_GROUP)
    {
	if (! DXGrow(object, 1, NULL, NULL))
	    return ERROR;

	if (! DXCreateTaskGroup())
	    return ERROR;

	for (i = 0; ;i++)
	{
	    child = DXGetEnumeratedMember((Group)object, i, NULL);
	    if (! child)
		break;
	    
	    if (! DXAddTask(positionsNormalsTask, (Pointer)child, 0, 1.0))
	    {
		DXAbortTaskGroup();
		return ERROR;
	    }
	}

	if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	    return ERROR;

	if (! DXShrink(object))
	    return ERROR;
	
	return OK;
    }
    else 
	 return _dxfPositionNormals_Field((Field)object);
}

static Error
_dxfPositionNormals_Field(Field field)
{
    Array		array;
    int			rank, shape[10];
    Type		type;
    Category		cat;
    int			n;
    char		*primitive;
    Object 		attr;

    /*
     * Check first whether the input field is empty.
     */

    if (DXEmptyField(field))
	return OK;

    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
	return OK;
    
    DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);
    if (n <= 0)
	return OK;
    
    DXDeleteComponent(field, "normals");
    
    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
    {
	DXSetError(ERROR_DATA_INVALID, "#10200", "positions component");
	return ERROR;
    }

    DXGetArrayInfo(array, &n, &type, &cat, &rank, shape);

    if (rank != 1 || shape[0] < 2 || shape[0] > 3)
    {
	DXSetError(ERROR_DATA_INVALID, "positions must be 2 or 3-D");
	return ERROR;
    }

    if (shape[0] == 2)
    {
        float normal[3] = {0.0, 0.0, 1.0};
	array = (Array)DXNewConstantArray(n, (Pointer)normal,
		TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    }
    else
    {
	attr = DXGetComponentAttribute(field, "connections", "element type");
	if (!attr)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10255",
				    "connections", "element type");
	    return ERROR;
	}

	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10200", "element type attribute");
	    return ERROR;
	}

	/*
	 * Check the topological primitive.
	 */

	primitive = DXGetString((String)attr);

	if (! strcmp(primitive, "triangles"))
	    array =_dxf_PDep_triangles(field);
	else if (! strcmp(primitive, "quads"))
	    array =_dxf_PDep_quads(field);
	else
	    array = NULL;
    }
    
    if (array)
    {
	/*
	 * Set the components of the output fields.
	 */

	if (DXGetComponentValue(field, "original normals"))
	    DXDeleteComponent(field, "original normals");

	if (! (DXSetComponentValue(field, "normals",(Object)array) &&
	       DXChangedComponentValues(field, "normals")	       &&
	       DXEndField(field)))
	    return ERROR;
    }

    return OK;
}

static Array
_dxf_PDep_triangles(Field field)
{
    Array		nArray;
    Array		a_connections, a_positions;
    int			n_connections, n_positions;
    float		centroid[3];
    float		dist;
    float		invdist;
    Vector		*normals, *n;
    int			i, nd;
    int			degenerate_count;
    ArrayHandle		pHandle = NULL;
    ArrayHandle		cHandle = NULL;

    degenerate_count = 0;

    /* 
     * Initialize outputs.
     */

    nArray = NULL;
    if (NULL == (nArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;

    a_connections = (Array)DXGetComponentValue(field, "connections");
    a_positions = (Array)DXGetComponentValue(field, "positions");

    DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(a_positions, &n_positions, NULL, NULL, NULL, &nd);

    if (! DXTypeCheck(a_connections, TYPE_INT, CATEGORY_REAL, 1, 3))
	goto error;

    /*
     * Setup the final array information.
     */
    if (! DXAddArrayData(nArray, 0, n_positions, (Pointer)NULL))
        goto error;

    /*
     * Setup and initialize the components.
     */

    normals = (Vector *)DXGetArrayData(nArray);

    for(i = 0, n = normals; i < n_positions; i++, n++)
	n->x = n->y = n->z = 0.0;

    /*
     * For each connection...
     */

    pHandle = DXCreateArrayHandle(a_positions);
    cHandle = DXCreateArrayHandle(a_connections);
    if (! pHandle || ! cHandle)
	goto error;

    for(i = 0; i < n_connections; i++)
    {
	Vector enorm;
	int    j, *element, ebuf[3];
	float  *ptrs[3], buf[9];

	element =(int *)DXCalculateArrayEntry(cHandle, i,(Pointer)ebuf);

	for(j = 0; j < 3; j++)						   
	    ptrs[j] = (float *)DXGetArrayEntry(pHandle, element[j], buf+j*nd); 

	NORMAL(3, nd, (&enorm));

        /*
	 * Determine the coordinates of the centroid of the triangle.
	 */

	if (nd == 3)
	{
	    centroid[0] = (ptrs[0][0]+ptrs[1][0]+ptrs[2][0])*ONE_THIRD;
	    centroid[1] = (ptrs[0][1]+ptrs[1][1]+ptrs[2][1])*ONE_THIRD;
	    centroid[2] = (ptrs[0][2]+ptrs[1][2]+ptrs[2][2])*ONE_THIRD;

	    for (j = 0; j < 3; j++)
	    {
		float x, y, z;
		x = ptrs[j][0] - centroid[0];
		y = ptrs[j][1] - centroid[1];
		z = ptrs[j][2] - centroid[2];
		dist = ABS(x)+ ABS(y)+ ABS(z);
		if (dist > 0.0)
		{
		    invdist = 1.0 / dist;
		    n = normals + element[j];
		    n->x += enorm.x * invdist;
		    n->y += enorm.y * invdist;
		    n->z += enorm.z * invdist;
		}
	    }
	}
	else
	{
	    centroid[0] = (ptrs[0][0]+ptrs[1][0]+ptrs[2][0])*ONE_THIRD;
	    centroid[1] = (ptrs[0][1]+ptrs[1][1]+ptrs[2][1])*ONE_THIRD;

	    for (j = 0; j < 3; j++)
	    {
		float x, y;
		x = ptrs[j][0] - centroid[0];
		y = ptrs[j][1] - centroid[1];
		dist = ABS(x)+ ABS(y);
		if (dist > 0.0)
		{
		    invdist = 1.0 / dist;
		    n = normals + element[j];
		    n->z += enorm.z * invdist;
		}
	    }
	}

	continue;

degenerate:
	degenerate_count ++;						    
    }

    DXFreeArrayHandle(pHandle);
    DXFreeArrayHandle(cHandle);

    /*
     * Now normalize all the normals
     */

    for(i = 0; i < n_positions; i++)
    {
	if (vector_zero((Vector *)normals))
	{
	    normals->x = (float)0.0;
	    normals->y = (float)0.0;
	    normals->z = (float)1.0;
	}

	if (! vector_normalize((Vector *)normals, (Vector *)normals))
	    goto error;
	
	normals ++;
    }

    if (degenerate_count > 0)
	DXMessage("Number of degenerate triangles: %d", degenerate_count);

    return nArray;

error:
    DXDelete((Object)nArray);
    return NULL;
}

static Array
_dxf_PDep_quads(Field field)
{
    Array		nArray;
    Array		a_connections, a_positions;
    int			n_connections, n_positions;
    float		centroid[3];
    float		dist;
    float		invdist;
    Vector		*normals, *n;
    int			i, nd;
    int			degenerate_count;
    ArrayHandle		pHandle = NULL;
    ArrayHandle		cHandle = NULL;

    degenerate_count = 0;

    /* 
     * Initialize outputs.
     */

    nArray = NULL;

    if (NULL == (nArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;

    a_connections = (Array)DXGetComponentValue(field, "connections");
    a_positions = (Array)DXGetComponentValue(field, "positions");

    DXGetArrayInfo(a_connections, &n_connections, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(a_positions, &n_positions, NULL, NULL, NULL, &nd);

    if (! DXTypeCheck(a_connections, TYPE_INT, CATEGORY_REAL, 1, 4))
	goto error;

    /*
     * Setup the final array information.
     */
    if (! DXAddArrayData(nArray, 0, n_positions, (Pointer)NULL))
        goto error;

    /*
     * Setup and initialize the components.
     */

    normals = (Vector *)DXGetArrayData(nArray);

    for(i = 0, n = normals; i < n_positions; i++, n++)
	n->x = n->y = n->z = 0.0;

    pHandle = DXCreateArrayHandle(a_positions);
    cHandle = DXCreateArrayHandle(a_connections);
    if (! pHandle || ! cHandle)
	goto error;

    for(i = 0; i < n_connections; i++)
    {
	Vector enorm;
	int    j, *element, ebuf[4];
	float  *ptrs[4], buf[12];

	element = (int *)DXCalculateArrayEntry(cHandle, i,(Pointer)ebuf);

	ptrs[0] = (float *)DXGetArrayEntry(pHandle, element[0], buf+0*nd); 
	ptrs[1] = (float *)DXGetArrayEntry(pHandle, element[2], buf+1*nd); 
	ptrs[2] = (float *)DXGetArrayEntry(pHandle, element[3], buf+2*nd); 
	ptrs[3] = (float *)DXGetArrayEntry(pHandle, element[1], buf+3*nd); 

	NORMAL(4, nd, (&enorm));

	/*
	 * Determine the addresses of the normals at the 
	 * three points.
	 */
	if (nd == 3)
	{
	    centroid[0] = (ptrs[0][0]+ptrs[1][0]+ptrs[2][0]+ptrs[3][0])*ONE_FOURTH;
	    centroid[1] = (ptrs[0][1]+ptrs[1][1]+ptrs[2][1]+ptrs[3][1])*ONE_FOURTH;
	    centroid[2] = (ptrs[0][2]+ptrs[1][2]+ptrs[2][2]+ptrs[3][2])*ONE_FOURTH;

	    for (j = 0; j < 4; j++)
	    {
		float x, y, z;
		x = ptrs[j][0] - centroid[0];
		y = ptrs[j][1] - centroid[1];
		z = ptrs[j][2] - centroid[2];
		dist = ABS(x)+ ABS(y)+ ABS(z);
		if (dist > 0.0)
		{
		    invdist = 1.0 / dist;
		    n = normals + element[j];
		    n->x += enorm.x * invdist;
		    n->y += enorm.y * invdist;
		    n->z += enorm.z * invdist;
		}
	    }
	}
	else
	{
	    centroid[0] = (ptrs[0][0]+ptrs[1][0]+ptrs[2][0]+ptrs[3][0])*ONE_FOURTH;
	    centroid[1] = (ptrs[0][1]+ptrs[1][1]+ptrs[2][1]+ptrs[3][1])*ONE_FOURTH;

	    n->x = 0.0;
	    n->y = 0.0;

	    for (j = 0; j < 4; j++)
	    {
		float x, y;
		x = ptrs[j][0] - centroid[0];
		y = ptrs[j][1] - centroid[1];
		dist = ABS(x)+ ABS(y);
		if (dist > 0.0)
		{
		    invdist = 1.0 / dist;
		    n = normals + element[j];
		    n->z += enorm.z * invdist;
		}
	    }
	}

	continue;

degenerate:
	degenerate_count ++;						   
    }

    DXFreeArrayHandle(pHandle);
    DXFreeArrayHandle(cHandle);

    /*
     * Now normalize all the normals
     */

    for(i = 0; i < n_positions; i++)
    {
	if (vector_zero((Vector *)normals))
	{
	    normals->x = (float)0.0;
	    normals->y = (float)0.0;
	    normals->z = (float)1.0;
	}

	if (! vector_normalize((Vector *)normals, (Vector *)normals))
	    goto error;
	
	normals ++;
    }

    if (degenerate_count > 0)
	DXMessage("Number of degenerate triangles: %d", degenerate_count);

    return nArray;

error:
    DXDelete((Object)nArray);
    return NULL;
}

static Error
areaNormalsTask(Pointer ptr)
{
    AreaArgs *args = (AreaArgs *)ptr;

    return _dxfAreaNormals_Field(args->field, args->radius);
}

static Error
_dxfAreaNormals(Pointer ptr)
{
    Object object = ((AreaTask *)ptr)->obj;
    float  radius = ((AreaTask *)ptr)->rad;
    int i;
    Object child;
    
    if (DXGetObjectClass(object) == CLASS_COMPOSITEFIELD)
    {
	if (! DXGrow(object, 1, NULL, NULL))
	    return ERROR;

	if (! DXCreateTaskGroup())
	    return ERROR;

	for (i = 0; ;i++)
	{
	    AreaArgs a;

	    child = DXGetEnumeratedMember((Group)object, i, NULL);
	    if (! child)
		break;

	    a.radius = radius;
	    a.field  = (Field)child;
	    
	    if (! DXAddTask(areaNormalsTask, (Pointer)&a, sizeof(a), 1.0))
	    {
		DXAbortTaskGroup();
		return ERROR;
	    }
	}

	if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	    return ERROR;

	if (! DXShrink(object))
	    return ERROR;
	
	return OK;
    }
    else 
	 return _dxfAreaNormals_Field((Field)object, radius);
}

static Error
_dxfAreaNormals_Field(Field field, float radius)
{
    Array	triArray, nbrArray, ptArray, nrmArray;
    int		nTriangles;
    int		nPoints;
    float	*pts, *normals, *facets;
    int		*tris, *nbrs;
    int		*pointTris=NULL;
    int 	*pointFlags=NULL;
    float	*f;
    int		*t;
    int		p, q, r, i, j;
    float	v0[3], v1[3];
    float	totEdgeLen;

    facets = NULL;
    nrmArray = NULL;

    triArray = (Array)DXGetComponentValue(field, "connections");
    ptArray = (Array)DXGetComponentValue(field, "positions");

    DXMarkTimeLocal("nbrs start");

    nbrArray = DXNeighbors(field);
    if (! nbrArray)
	goto error;
    
    nbrs = (int *)DXGetArrayData(nbrArray);
    pts = (float *)DXGetArrayData(ptArray);
    
    if (! DXQueryOriginalSizes(field, &nPoints, NULL))
	DXGetArrayInfo(ptArray, &nPoints, NULL, NULL, NULL, NULL);

    DXGetArrayInfo(triArray, &nTriangles, NULL, NULL, NULL, NULL);

    pointTris = (int *)DXAllocate(nPoints * sizeof(int));
    if (! pointTris)
	goto error;
    
    memset((int *)pointTris, -1, nPoints * sizeof(int));

    tris = (int *)DXGetArrayData(triArray);

    facets = (float *)DXAllocate(nTriangles*3*sizeof(float));
    if (! facets)
	goto error;

    memset((int *)facets, 0, nTriangles*3*sizeof(float));

    f = facets;
    t = tris;
    totEdgeLen = 0.0;
    for(i = 0; i < nTriangles; i++)
    {
	p = t[0];
	q = t[1];
	r = t[2];

	totEdgeLen += _dxfDistance(pts+(3*p), pts+(3*q));
	totEdgeLen += _dxfDistance(pts+(3*q), pts+(3*r));
	totEdgeLen += _dxfDistance(pts+(3*r), pts+(3*p));

	v0[0] = pts[3*p + 0] - pts[3*q + 0];
	v0[1] = pts[3*p + 1] - pts[3*q + 1];
	v0[2] = pts[3*p + 2] - pts[3*q + 2];

	v1[0] = pts[3*p + 0] - pts[3*r + 0];
	v1[1] = pts[3*p + 1] - pts[3*r + 1];
	v1[2] = pts[3*p + 2] - pts[3*r + 2];

	f[0] = v0[1] * v1[2] - v1[1] * v0[2];
	f[1] = v0[2] * v1[0] - v1[2] * v0[0];
	f[2] = v0[0] * v1[1] - v1[0] * v0[1];

	vector_normalize((Vector *)f, (Vector *)f);

	f += 3;

	for (j = 0; j < 3; j++)
	{
	    if (*t < nPoints)
		pointTris[*t] = i;
	    t++;
	}
    }

    radius *= (totEdgeLen /(3*nTriangles));

    nrmArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! nrmArray)
	goto error;

    if (! DXAddArrayData(nrmArray, 0, nPoints, NULL))
	goto error;

    normals = (float *)DXGetArrayData(nrmArray);

    memset((int *)normals, 0, nPoints*3*sizeof(float));

    pointFlags = (int *)DXAllocate(nTriangles * sizeof(int));
    if (! pointFlags)
	goto error;
    
    memset((int *)pointFlags, -1, nTriangles * sizeof(int));

    for(i = 0; i < nPoints; i++)
    {
	if (pointTris[i] >= 0)
	{
	    _dxfCheckTriangle(i, radius, pointTris[i], pointFlags,
		    tris, nbrs, pts, facets, normals);
	    if (! vector_normalize((Vector *)(normals+3*i), 
					(Vector *)(normals+3*i)))
	    {
		normals[3*i+0] = 0.0;
		normals[3*i+1] = 1.0;
		normals[3*i+2] = 0.0;
	    }
	}
	else
	{
	    normals[3*i+0] = 0.0;
	    normals[3*i+1] = 1.0;
	    normals[3*i+2] = 0.0;
	}
    }

    DXFree((Pointer)facets);
    DXFree((Pointer)pointTris);
    DXFree((Pointer)pointFlags);

    /*
     * Set the components of the output fields.
     */

    if (DXGetComponentValue(field, "original normals"))
	DXDeleteComponent(field, "original normals");

    if (! (DXSetComponentValue(field, "normals",(Object)nrmArray) &&
	   DXSetComponentAttribute(field, "normals", "dep", 	
					(Object)DXNewString("positions")) &&
	   DXChangedComponentValues(field, "normals")		        &&
	   DXEndField(field)))
    {
	goto error;
    }

    return OK;

error:
    DXFree((Pointer)facets);
    DXFree((Pointer)pointTris);
    DXFree((Pointer)pointFlags);
    DXDelete((Object)nrmArray);

    return ERROR;
}

static Error
_dxfCheckTriangle(int centerIndex, float radius, int triIndex, int *flags,
		    int *triangles, int *neighbors, float *positions, 
		    float *facets, float *normals)
{
    float *p, *q, *r;
    int   *triangle;
    int	  *nbrs;
    float *normal;
    int   in[3];
    float trap[12];
    float *center;
    float *facet;
    int   i, j, next;
    int   allOut, allIn;
    float area, t;

    if (flags[triIndex] == centerIndex)
	return OK;
    
    flags[triIndex] = centerIndex;

    center   = positions +(3*centerIndex);
    normal   = normals   +(3*centerIndex);
    facet    = facets    +(3*triIndex);
    triangle = triangles +(3*triIndex);
    nbrs     = neighbors +(3*triIndex);

    allIn  = 1;
    allOut = 1;
    for(i = 0; i < 3; i++)
    {
	p = positions +(3*triangle[i]);
	in[i] = _dxfInside(center, radius, p);

	if (in[i])
	    allOut = 0;
	else
	    allIn = 0;
    }

    if (allOut)
	return OK;
    
    if (allIn)
    {
	p = positions + 3*triangle[0];
	q = positions + 3*triangle[1];
	r = positions + 3*triangle[2];

	area  = _dxfTriangleArea(p, q, r);
	facet = facets + 3*(triIndex);

	normal[0] += area*facet[0];
	normal[1] += area*facet[1];
	normal[2] += area*facet[2];
    }
    else
    {
	j = 0;
	for(i = 0; i < 3; i++)
	{
	    if (i == 2)
		next = 0;
	    else
		next = i + 1;
	    
	    p = positions +(3*triangle[i]);
	    q = positions +(3*triangle[next]);

	    if (in[i])
	    {
		trap[j++] = p[0];
		trap[j++] = p[1];
		trap[j++] = p[2];

		if (! in[next])
		{
		    t = _dxfIntersect(center, radius, p, q);
		    if (t == -1.0)
		    {
			DXMessage("Intersection error");
			t = 0.0;
		    }

		    trap[j++] = p[0] + t*(q[0]-p[0]);
		    trap[j++] = p[1] + t*(q[1]-p[1]);
		    trap[j++] = p[2] + t*(q[2]-p[2]);
		}
	    }
	    else
	    {
		if (in[next])
		{
		    t = _dxfIntersect(center, radius, p, q);
		    if (-0.001 > t || 1.0001 < t)
		    {
			DXMessage("Intersection error: %f", t);
			if (t < 0.5)t = 0.0;
			else t = 1.0;
		    }

		    trap[j++] = p[0] + t*(q[0]-p[0]);
		    trap[j++] = p[1] + t*(q[1]-p[1]);
		    trap[j++] = p[2] + t*(q[2]-p[2]);
		}
	    }
	}

	if (j > 12)
	    DXMessage("clip error");
		
	facet = facets + 3*(triIndex);

	if (j == 12)
	{
	    area  = _dxfTriangleArea(trap+0, trap+3, trap+6);
	    normal[0] += area*facet[0];
	    normal[1] += area*facet[1];
	    normal[2] += area*facet[2];

	    area  = _dxfTriangleArea(trap+0, trap+6, trap+9);
	    normal[0] += area*facet[0];
	    normal[1] += area*facet[1];
	    normal[2] += area*facet[2];
	}
	else
	{
	    area  = _dxfTriangleArea(trap+0, trap+3, trap+6);
	    normal[0] += area*facet[0];
	    normal[1] += area*facet[1];
	    normal[2] += area*facet[2];
	}
    }

    for(i = 0; i < 3; i++)
    {
	if (i == 2)
	    next = 0;
	else
	    next = i + 1;
	
	if (nbrs[i] >= 0 &&(in[i] || in[next]))
	    _dxfCheckTriangle(centerIndex, radius, nbrs[i], flags,
		    triangles, neighbors, positions, facets, normals);
    }

    return OK;
}

static int
_dxfInside(float *center, float radius, float *pt)
{
    return(_dxfDistance(center, pt)< radius);
}

static float
_dxfDistance(float *p, float *q)
{
    float a, b, c;

    a = *p++ - *q++;
    b = *p++ - *q++;
    c = *p++ - *q++;

    return sqrt(a*a + b*b + c*c);
}

static float
_dxfTriangleArea(float *p, float *q, float *r)
{
    float a, b, c, S;

    a = _dxfDistance(p, q);
    b = _dxfDistance(q, r);
    c = _dxfDistance(r, p);

    S = (a + b + c)/ 2;

    return sqrt(S*(S-a)*(S-b)*(S-c));
}
	
static float 
_dxfIntersect(float *center, float radius, float *p0, float *p1)
{
    float x10, y10, z10;
    float x0c, y0c, z0c;
    float A, B, C, D;
    float t;

    x10 = p1[0] - p0[0];
    y10 = p1[1] - p0[1];
    z10 = p1[2] - p0[2];

    x0c = p0[0] - center[0];
    y0c = p0[1] - center[1];
    z0c = p0[2] - center[2];

    A = x10*x10 + y10*y10 + z10*z10;
    B = 2 *(x10*x0c + y10*y0c + z10*z0c);
    C = x0c*x0c + y0c*y0c + z0c*z0c - radius*radius;

    D = B*B - 4*A*C;

    if (D >= 0.0)
    {
	D = sqrt(D);

	t = (-B + D)/(2 * A);
	if (t >= -0.0001 && t <= 1.0001)
	    return t;

	t = (-B - D)/(2 * A);
	if (t >= -0.0001 && t <= 1.0001)
	    return t;
    }
    
    return -1.0;
}

static int 
CompareDependency(Object o, char *target)
{
    Class class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);

    if (class == CLASS_FIELD)
    {
	Object attr;

	if (! DXGetComponentValue((Field)o, "normals"))
	    return 0;

	attr = DXGetComponentAttribute((Field)o, "normals", "dep");
	if (! attr)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"normals component is missing the dep attribute");
	    return -1;
	}

	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, 
		"dep attribute must be class STRING");
	    return -1;
	}

	return !strcmp(DXGetString((String)attr), target);
    }
    else if (class == CLASS_COMPOSITEFIELD)
    {
	Object child;
	int i, j;

	i = 0;
	while(NULL != (child = DXGetEnumeratedMember((Group)o, i++, NULL)))
	{
	    j = CompareDependency(child, target);
	    if (j != 1)
		return j;
	}

	return 1;
    }
    else
    {
	DXSetError(ERROR_BAD_CLASS, "field or composite field");
	return -1;
    }
}

static Error
_dxfFaceNormals(Pointer ptr)
{
    Field field = (Field)ptr;
    Array fa, la, ea, pa, na;
    int *f, *l, *e;
    float *p;
    int nf, nl, ne, ndim, rank;

    na = (Array)DXGetComponentValue(field, "normals");
    if (na)
    {
	char *str;

	if (! DXGetStringAttribute((Object)na, "dep", &str))
	{
	    DXSetError(ERROR_DATA_INVALID, "normals dependency attribute");
	    goto error;
	}

	if (! strcmp(str, "faces"))
	    return OK;
	
	na = NULL;
    }

    fa   = (Array)DXGetComponentValue(field, "faces");
    la   = (Array)DXGetComponentValue(field, "loops");
    ea   = (Array)DXGetComponentValue(field, "edges");
    pa   = (Array)DXGetComponentValue(field, "positions");
    if (! fa || ! la || ! ea || ! pa)
    {
	DXSetError(ERROR_MISSING_DATA, "one of faces/loops/edges/positions");
	goto error;
    }

    f = (int *)DXGetArrayData(fa);
    DXGetArrayInfo(fa, &nf, NULL, NULL, NULL, NULL);

    l = (int *)DXGetArrayData(la);
    DXGetArrayInfo(la, &nl, NULL, NULL, NULL, NULL);

    e = (int *)DXGetArrayData(ea);
    DXGetArrayInfo(ea, &ne, NULL, NULL, NULL, NULL);

    p = (float *)DXGetArrayData(pa);
    DXGetArrayInfo(pa, NULL, NULL, NULL, &rank, &ndim);

    if (rank != 1 || ndim < 2 || ndim > 3)
    {
	DXSetError(ERROR_DATA_INVALID, "data must be 2- or 3-D");
	goto error;
    }
    else
    {
	na = _dxf_FLE_Normals(f, nf, l, nl, e, ne, p, ndim);
    }

    if (! na)
	goto error;

    if (! DXSetStringAttribute((Object)na, "dep", "faces"))
	goto error;

    if (! DXSetComponentValue(field, "normals", (Object)na))
	goto error;
    
    return OK;

error:
    DXDelete((Object)na);
    return ERROR;
}

/*
 * This is the interface called in triangulation code
 */
Array
_dxf_FLE_Normals(int *f, int nf, int *l, int nl, int *e, int ne, float *v, int nd)
{
    int   i, face, maxloop;
    Point *normal;
    float **ptrs = NULL;
    Array nA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! nA)
	goto error;
    
    if (! DXAddArrayData(nA, 0, nf, NULL))
	goto error;
    
    normal = (Point *)DXGetArrayData(nA);
    
    /*
     * For each face, find the longest outer loop
     */
    maxloop = 0;
    for (face = 0; face < nf; face ++)
    {
	int loop   = f[face];
	int start  = l[loop];
	int end    = (loop == nl-1) ? ne : l[loop+1];
	int knt    = end - start;

	if (knt > maxloop)
	    maxloop = knt;
    }

    ptrs = (float **)DXAllocate(maxloop*sizeof(float *));
    if (! ptrs)
	goto error;

    /*
     * For each face...
     */
    for (face = 0; face < nf; face++, normal++)
    {
	int   loop   = f[face];
	int   start  = l[loop];
	int   end    = (loop == nl-1) ? ne : l[loop+1];
	int   knt    = end - start;
	float l;

	for (i = 0; i < knt; i++)
	    ptrs[i] = v + e[i+start]*nd;
	
	NORMAL(knt, nd, normal);

	l = DXLength(*normal);
	if (l != 0.0)
	    *normal = DXMul(*normal, l);

	continue;

degenerate:
	normal->x = 0.0;
	normal->y = 0.0;
	normal->z = 1.0;
	continue;
    }

    DXFree((Pointer)ptrs);

    return nA;

error:
    DXDelete((Object)nA);
    DXFree((Pointer)ptrs);
    return NULL;
}

static Field
GridNormalsField(Field field, int dep)
{
    Array pArray, cArray, nArray;
    int pDim, cDim, n, counts[3];
    float x, y, z, d, *vectors[2], deltas[9], norm[3];
    int i, j;
    Object attr;

    pArray = (Array)DXGetComponentValue(field, "positions");
    cArray = (Array)DXGetComponentValue(field, "connections");

    if (! pArray || ! cArray)
	return NULL;
    
    if (! DXQueryGridConnections(cArray, &cDim, NULL) ||
	! DXQueryGridPositions(pArray, &pDim, counts, NULL, deltas))
	return NULL;
    
    if (cDim != 2 || pDim < 2 || pDim > 3)
	return NULL;
    
    for (i = j = 0; i < pDim; i++)
	if (counts[i] > 1)
	    vectors[j++] = deltas + i*pDim;
    
    if (j > 2)
	return NULL;
    
    if (pDim == 3)
    {
	x = vectors[0][1]*vectors[1][2] - vectors[0][2]*vectors[1][1];
	y = vectors[0][2]*vectors[1][0] - vectors[0][0]*vectors[1][2];
	z = vectors[0][0]*vectors[1][1] - vectors[0][1]*vectors[1][0];
    }
    else
    {
	x = 0.0;
	y = 0.0;
	z = vectors[0][0]*vectors[1][1] - vectors[0][1]*vectors[1][0];
    }

    d = x*x + y*y + z*z;
    if (d != 0.0)
    {
	d = 1.0 / sqrt(d);

	norm[0] = x * d;
	norm[1] = y * d;
	norm[2] = z * d;
    }
    else norm[0] = norm[1] = norm[2] = 0.0;

    if (dep == DEP_ON_POSITIONS)
    {
	DXGetArrayInfo(pArray, &n, NULL, NULL, NULL, NULL);
	attr = (Object)DXNewString("positions");
    }
    else if (dep == DEP_ON_CONNECTIONS)
    {
	DXGetArrayInfo(cArray, &n, NULL, NULL, NULL, NULL);
	attr = (Object)DXNewString("connections");
    }
    else
	return NULL;

    if (! attr)
	return NULL;

    nArray = (Array)DXNewConstantArray(n, (Pointer)&norm, TYPE_FLOAT,
			CATEGORY_REAL, 1, 3);
    if (! nArray)
    {
	DXDelete((Object)attr);
	return NULL;
    }
    
    if (DXGetComponentValue(field, "normals"))
	DXDeleteComponent(field, "normals");
    
    if (! DXSetComponentValue(field, "normals", (Object)nArray))
    {
	DXDelete((Object)nArray);
	DXDelete((Object)attr);
	return NULL;
    }

    if (! DXSetComponentAttribute(field, "normals", "dep", attr))
    {
	DXDelete((Object)nArray);
	DXDelete((Object)attr);
	return NULL;
    }

    return field;
}

static CompositeField
GridNormalsCompositeField(CompositeField o, int dep)
{
    int i;
    Object c;
    
    i = 0;
    while(NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
	if (! GridNormalsField((Field)c, dep))
	    return NULL;
    
    return o;
}
	 
