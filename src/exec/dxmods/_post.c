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
#include "_post.h"

#define TO_POSITIONS	1
#define TO_CONNECTIONS	2

typedef struct
{
    Field field;
    int   dir;
    Array comp;
} TaskArgs;

static Object _dxfPostToConnections(Field, char **);
static Object _dxfPostToPositions(Field, char **);
static Object _dxfPostObject(Object, int, Array);
static Error  _dxfPostFieldTask(Pointer);
static Object _dxfPostField(Field, int, Array);
static Error  _dxfGetComponentList(Object, Array, int, char **);
static Error  _dxfRemoveOriginal(Field, char *);


Object 
_dxfPost(Object obj, char *str, Array comp)
{
     int dir;

     if (!strcmp(str, "positions"))
	dir = TO_POSITIONS;
    else if (!strcmp(str, "connections"))
	dir = TO_CONNECTIONS;
    else
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10203", "dependency", 
					"`positions' or `connections'");
	return NULL;
    }

    return _dxfPostObject(obj, dir, comp);
}

Array
_dxfPostArray(Field field, char *component, char *destination)
{
    Object attr;
    char *source;
    Array src, dst = NULL;
    Field tmpField = NULL;
    Array tmpArray;
    char  *compNames[2];

    if (strcmp(destination, "positions") && strcmp(destination, "connections"))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10203", "dependency", 
					"`positions' or `connections'");
	goto error;
    }

    if (DXEmptyField(field))
	goto error;

    src = (Array)DXGetComponentValue(field, component);
    if (! src)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", component);
	goto error;
    }

    attr = DXGetComponentAttribute(field, component, "dep");
    if (!attr || DXGetObjectClass(attr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "#10241", component, "dep");
	goto error;
    }

    source = DXGetString((String)attr);

    if (! strcmp(source, destination))
    {
	Type t;
	Category c;
	int n, r, s[32];
	Pointer p;

	DXGetArrayInfo(src, &n, &t, &c, &r, s);

	if (DXQueryConstantArray(src, NULL, NULL))
	{
	    dst = (Array)DXNewConstantArrayV(n, DXGetConstantArrayData(src),
								    t, c, r, s);
	    if (! dst)
		goto error;
	}
	else
	{
	    dst = DXNewArrayV(t, c, r, s);
	    if (! dst)
		goto error;
	    
	    p = DXGetArrayData(src);
	    if (! p)
		goto error;

	    if (! DXAddArrayData(dst, 0, n, p))
		goto error;
	}
	
	goto ok;
    }

    tmpField = (Field)DXCopy((Object)field, COPY_ATTRIBUTES);
    if (! tmpField)
	goto error;

    tmpArray = (Array)DXGetComponentValue(field, "positions");
    if (! tmpArray)
	goto error;
    
    if (! DXSetComponentValue(tmpField, "positions", (Object)tmpArray))
	 goto error;
	
    tmpArray = (Array)DXGetComponentValue(field, "connections");
    if (! tmpArray)
	goto error;
    
    if (! DXSetComponentValue(tmpField, "connections", (Object)tmpArray))
	goto error;
    
    if (! DXSetComponentValue(tmpField, "data", (Object)src))
	goto error;
    
    compNames[0] = "data";
    compNames[1] = NULL;

    if (! strcmp(destination, "positions"))
    {
	if (! _dxfPostToPositions(tmpField, compNames))
	    goto error;
    }
    else
    {
	if (! _dxfPostToConnections(tmpField, compNames))
	    goto error;
    }

    dst = (Array)DXGetComponentValue(tmpField, "data");
    if (! dst)
	goto error;
    
    /*
     * A little magic to get the resulting component unreferenced:
     * As a member of tmpField it has a ref count of 1.  DXReference it
     * and the ref count goes to 2.  DXDelete tmpField and the ref count
     * returns to 1.  Unreference it to get the 0.
     */
    DXReference((Object)dst);
    DXDelete((Object)tmpField);
    tmpField = NULL;
    DXUnreference((Object)dst);
    
ok:
    DXDelete((Object)tmpField);
    return dst;

error:
    DXDelete((Object)tmpField);
    DXDelete((Object)dst);
    return NULL;
}
	    
static Object
_dxfPostObject(Object obj, int dir, Array comp)
{
    Class class;

    class = DXGetObjectClass(obj);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)obj);
    
    switch(class)
    {
	case CLASS_FIELD:
	    if (! _dxfPostField((Field)obj, dir, comp))
		goto error;

	    break;
	
	case CLASS_GROUP:
	case CLASS_SERIES:
	case CLASS_MULTIGRID:
	{
	    Object child;
	    Group  group;
	    int    i;

	    group = (Group)obj;

	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember(group, i++, NULL)))
		if (! _dxfPostObject(child, dir, comp))
		    goto error;
	    
	    break;
	}
	
	case CLASS_XFORM:
	{
	    Object child;

	    if (! DXGetXformInfo((Xform)obj, &child, 0))
		goto error;
	    
	    if (! _dxfPostObject(child, dir, comp))
		goto error;
	    
	    break;
	}

	case CLASS_CLIPPED:
	{
	    Object child;

	    if (! DXGetClippedInfo((Clipped)obj, &child, NULL))
		goto error;
	    
	    if (! _dxfPostObject(child, dir, comp))
		goto error;
	    
	    break;
	}

	case CLASS_COMPOSITEFIELD:
	{
	    int		i;
	    TaskArgs	args;
	    Group	group;
	    char	*compStrings[32];

	    if (! _dxfGetComponentList(obj, comp, dir, compStrings))
	    {
		if (DXGetError() != ERROR_NONE)
		    goto error;
	    }
	    else
	    {
		if (! DXGrowV(obj, 1, NULL, compStrings))
		    goto error;
	    
		args.dir  = dir;
		args.comp = comp;
	    
		if (! DXCreateTaskGroup())
		    goto error;
	    
		group = (Group)obj;
		
		i = 0;
		while (NULL != 
		    (args.field = (Field)DXGetEnumeratedMember(group, i++, NULL)))
		    if (! DXAddTask(_dxfPostFieldTask,
					(Pointer)&args, sizeof(args), 1.0))
		    {
			DXAbortTaskGroup();
			goto error;
		    }
		
		if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		    goto error;
		
		if (! DXShrink(obj))
		    goto error;
	    }
	    
	    break;
	}

	default:
	    DXSetError(ERROR_DATA_INVALID, "#11381");
	    goto error;
    }

    return obj;

error:

    return NULL;
}

static Error
_dxfPostFieldTask(Pointer ptr)
{
    int      dir;
    Array    comp;
    Field    field;
    TaskArgs *args;

    args = (TaskArgs *)ptr;

    dir   = args->dir;
    comp  = args->comp;
    field = args->field;

    if (! _dxfPostField(field, dir, comp))
	return ERROR;
    
    return OK;
}

static Object
_dxfPostField(Field field, int dir, Array comp)
{
    char *compStrings[32], **c;

    if (! DXInvalidateConnections((Object)field))
	goto error;

    if (! _dxfGetComponentList((Object)field, comp, dir, compStrings)) {
	if (DXGetError() != ERROR_NONE)
	    goto error;
	else
	    return (Object)field;
    }
	
    for (c = compStrings; *c; c++)
	DXChangedComponentValues(field, *c);


    if (dir == TO_POSITIONS)
	field = (Field)_dxfPostToPositions(field, compStrings);
    else
	field = (Field)_dxfPostToConnections(field, compStrings);
    
    if (! field)
	return NULL;
    
    return (Object)DXEndField(field);

error:
    return NULL;
}

static Object
_dxfPostToPositions(Field field, char **comp)
{
    Array 	array, nArray;
    int   	i, j, regConnections, vPerE;
    int   	cCounts[32], cDim;
    byte 	*knts, *dstData, *srcData;
    Type  	type; Category c; int r, s[32];
    int   	nPositions, nConnections;
    char 	*name;
    byte  	*accumulator = NULL;
    char  	*originalDeleteList[100];
    int   	nDelete = 0;
    ArrayHandle cHandle = NULL;
    Pointer	ebuf = NULL;
    InvalidComponentHandle ich = NULL;

    knts = NULL;
    dstData = NULL;
    nArray = NULL;
    
    if (DXEmptyField(field))
	return (Object)field;

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
    {
	DXSetError(ERROR_DATA_INVALID, "#10240", "positions");
	goto error;
    }

    DXGetArrayInfo(array, &nPositions, NULL, NULL, NULL, NULL);
	
    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
	return (Object)field;
    
    DXGetArrayInfo(array, &nConnections, &type, &c, &r, &vPerE);

    if (DXQueryGridConnections(array, &cDim, cCounts))
    {
	regConnections = 1;
    }
    else
    {
	regConnections = 0;
	cHandle = DXCreateArrayHandle(array);
	if (! cHandle)
	    goto error;
	ebuf = DXAllocate(vPerE*sizeof(int));
	if (! ebuf)
	    goto error;
    }

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object)field, 
							NULL, "connections");
	if (! ich)
	    goto error;
    }

    knts = (byte *)DXAllocateLocal(nPositions * sizeof(byte));
    if (! knts)
    {
	DXResetError();
	knts = (byte *)DXAllocate(nPositions * sizeof(byte));
    }
    if (! knts)
	goto error;

    i = 0;
    while (NULL != (array=(Array)DXGetEnumeratedComponentValue(field, i, &name)))
    {
	Object attr;
	int    doit, n, itemSize, typeSize, vPerI;

	if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
	    goto component_done;

	/*
	 * Do we operate on this component
	 */
	if (! strcmp(name, "positions")   		|| 
	    ! strcmp(name, "connections") 		||
	    ! strcmp(name, "invalid positions")		||
	    ! strcmp(name, "invalid connections")	||
	    ! strncmp(name, "original", 8))
	    goto component_done;

	
	/*
	 * If the component refs anything, leave it alone.
	 */
	if (DXGetComponentAttribute(field, name, "ref"))
	    goto component_done;
	
	attr = DXGetComponentAttribute(field, name, "dep");
	if (! attr)
	    goto component_done;

	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10200", "dependency attribute");
	    goto error;
	}

	if (strcmp(DXGetString((String)attr), "connections"))
	    goto component_done;

	if (! comp)
	    doit = 1;
	else
	{
	    char **c;

	    doit = 0;
	    for (c = comp; *c && !doit; c++)
		if (! strcmp(name,  *c))
		    doit = 1;
	}

	if (! doit)
	    goto component_done;
	

	originalDeleteList[nDelete++] = name;

	if (! DXChangedComponentValues(field, name))
	    goto error;
	
	if (DXGetComponentAttribute(field, name, "ref"))
	{
	    DXSetError(ERROR_DATA_INVALID, "#12110");
	    goto error;
	}
	    
	/*
	 * OK.  Now extract the relevant info from the array
	 */
	DXGetArrayInfo(array, &n, &type, &c, &r, s);

	if (n != nConnections)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10400", name, "connections");
	    goto error;
	}

	if (DXQueryConstantArray(array, NULL, NULL))
	{
	    nArray = (Array)DXNewConstantArrayV(nPositions,
				DXGetConstantArrayData(array), type, c, r, s);
	    if (! nArray)
		goto error;
	}
	else
	{
	    srcData = (byte *)DXGetArrayData(array);
	    if (! srcData)
		goto error;

	    itemSize = DXGetItemSize(array);
	    typeSize = DXTypeSize(type);
	    vPerI    = itemSize / typeSize;

	    /*
	     * Now create a local array buffer
	     */
	    accumulator = (byte *)DXAllocateLocal(nPositions*vPerI*sizeof(float));
	    if (! accumulator)
	    {
		DXResetError();
		accumulator = (byte *)DXAllocate(nPositions*vPerI*sizeof(float));
	    }
	    if (! accumulator)
		goto error;
	    
	    memset(accumulator, 0, nPositions*vPerI*sizeof(float));
	    memset(knts, 0, nPositions * sizeof(byte));

	    nArray = DXNewArrayV(type, c, r, s);

	    if (! nArray)
		goto error;
	    
	    if (! DXAddArrayData(nArray, 0, nPositions, NULL))
		goto error;

	    if (regConnections)
	    {
		int pSkip[32], cSkip[32], adj[32];
		typedef struct {
		    int	inc, limit;
		    byte   *cPtr, *pPtr, *kPtr;
		    int 	kSkip, pSkip, cSkip;
		} Loop;
		Loop loop[32];

		pSkip[0] = cSkip[0] = 1;
		for (j = 1; j < cDim; j++)
		{
		    pSkip[j] = pSkip[j-1] * cCounts[(cDim-1)-(j-1)];
		    cSkip[j] = cSkip[j-1] * (cCounts[(cDim-1)-(j-1)] - 1);
		}
		
		for (j = 0; j < (1 << cDim); j++)
		{
		    int k;

		    adj[j] = 0;
		    for (k = 0; k < cDim; k++)
			if (j & (1 << k))
			    adj[j] += pSkip[k];
		}
		
		for (j = 0; j < cDim; j++)
		{
		    loop[j].inc   = 0;
		    loop[j].limit = cCounts[(cDim-1)-j];
		    loop[j].cPtr  = srcData;
		    loop[j].pPtr  = accumulator;
		    loop[j].kPtr  = knts;
		    loop[j].kSkip = pSkip[j];
		    loop[j].pSkip = pSkip[j]*vPerI*sizeof(float);
		    loop[j].cSkip = cSkip[j]*itemSize;
		}


		switch (type)
		{
		    case TYPE_DOUBLE:
			TO_POSITIONS_REGULAR(double);
			break;

		    case TYPE_FLOAT:
			TO_POSITIONS_REGULAR(float);
			break;

		    case TYPE_INT:
			TO_POSITIONS_REGULAR(int);
			break;

		    case TYPE_UINT:
			TO_POSITIONS_REGULAR(uint);
			break;

		    case TYPE_SHORT:
			TO_POSITIONS_REGULAR(short);
			break;

		    case TYPE_USHORT:
			TO_POSITIONS_REGULAR(ushort);
			break;

		    case TYPE_BYTE:
			TO_POSITIONS_REGULAR(byte);
			break;

		    case TYPE_UBYTE:
			TO_POSITIONS_REGULAR(ubyte);
			break;

		    default:
			DXSetError(ERROR_DATA_INVALID, "#10320", name);
			goto error;
		}
	    }
	    else /* irregular connections */
	    {
		int   l;

		switch (type)
		{
		    case TYPE_DOUBLE:
			    TO_POSITIONS_IRREGULAR(double);
			    break;

		    case TYPE_FLOAT:
			    TO_POSITIONS_IRREGULAR(float);
			    break;

		    case TYPE_INT:
			    TO_POSITIONS_IRREGULAR(int);
			    break;

		    case TYPE_UINT:
			    TO_POSITIONS_IRREGULAR(uint);
			    break;

		    case TYPE_SHORT:
			    TO_POSITIONS_IRREGULAR(short);
			    break;

		    case TYPE_USHORT:
			    TO_POSITIONS_IRREGULAR(ushort);
			    break;

		    case TYPE_BYTE:
			    TO_POSITIONS_IRREGULAR(byte);
			    break;

		    case TYPE_UBYTE:
			    TO_POSITIONS_IRREGULAR(ubyte);
			    break;

		    default:
			DXSetError(ERROR_DATA_INVALID, "#10320", name);
			goto error;
		}
	    }
	}
	
	if (! DXSetComponentValue(field, name, (Object)nArray))
	    goto error;
	nArray = NULL;

	if (! DXSetComponentAttribute(field, name, "dep", 
					(Object)DXNewString("positions")))
	    goto error;
	
	DXFree((Pointer)accumulator);
	accumulator = NULL;
	
	DXFree((Pointer)dstData);
	dstData = NULL;

component_done:
	
	i++;
    }
    
    for (i = 0; i < nDelete; i++)
	if (! _dxfRemoveOriginal(field, originalDeleteList[i]))
	    goto error;
    
    if (! DXInvalidateUnreferencedPositions((Object)field))
	goto error;

    DXFree((Pointer)knts);
    knts = NULL;

    if (cHandle)
	DXFreeArrayHandle(cHandle);
    DXFree(ebuf);

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    ich = NULL;

    return (Object)field;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFree(ebuf);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    DXFree((Pointer)dstData);
    DXFree((Pointer)knts);
    DXFree((Pointer)accumulator);
    DXDelete((Object)nArray);

    return NULL;
}

static Object
_dxfPostToConnections(Field field, char **comp)
{
    Array array, nArray;
    int   i, j, regConnections, vPerE;
    int   cCounts[32], cDim, *elements;
    byte *knts, *dstData, *srcData;
    Type  type; Category c; int r, s[32];
    int   nPositions, nConnections;
    char  *name;
    byte *accumulator = NULL;
    ArrayHandle cHandle = NULL;
    Pointer	ebuf = NULL;
    InvalidComponentHandle ich = NULL;

    knts = NULL;
    dstData = NULL;
    nArray = NULL;
    
    if (DXEmptyField(field))
	return (Object)field;

    array = (Array)DXGetComponentValue(field, "positions");
    if (! array)
	return (Object)field;


    DXGetArrayInfo(array, &nPositions, NULL, NULL, NULL, NULL);
	
    array = (Array)DXGetComponentValue(field, "connections");
    if (! array)
    {
	DXSetError(ERROR_DATA_INVALID, "#10240", "connections");
	goto error;
    }
    
    DXGetArrayInfo(array, &nConnections, &type, &c, &r, &vPerE);

    if (DXQueryGridConnections(array, &cDim, cCounts))
    {
	regConnections = 1;
    }
    else
    {
	regConnections = 0;
	cHandle = DXCreateArrayHandle(array);
	if (! cHandle)
	    goto error;
	ebuf = DXAllocate(vPerE*sizeof(int));
	if (! ebuf)
	    goto error;
    }

    if (! regConnections)
    {
	elements = (int *)DXGetArrayData(array);
	if (! elements)
	    goto error;
    }

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object)field, 
							NULL, "connections");
	if (! ich)
	    goto error;
    }

    knts = (byte *)DXAllocateLocal(nConnections * sizeof(byte));
    if (! knts)
    {
	DXResetError();
	knts = (byte *)DXAllocate(nConnections * sizeof(byte));
    }
    if (! knts)
	goto error;

    i = 0;
    while (NULL != (array=(Array)DXGetEnumeratedComponentValue(field, i, &name)))
    {
	Object attr;
	int    doit, n, itemSize, typeSize, vPerI;

	/*
	 * Do we operate on this component
	 */
	if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
	    goto component_done;

	if (! strcmp(name, "positions")   		|| 
	    ! strcmp(name, "connections") 		||
	    ! strcmp(name, "invalid positions") 	||
	    ! strcmp(name, "invalid connections") 	||
	    ! strncmp(name, "original", 8))
	    goto component_done;
	
	/*
	 * If the component refs anything, leave it alone.
	 */
	if (DXGetComponentAttribute(field, name, "ref"))
	    goto component_done;
	
	attr = DXGetComponentAttribute(field, name, "dep");
	if (! attr)
	    goto component_done;

	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10200", "dependency attribute");
	    goto error;
	}

	if (strcmp(DXGetString((String)attr), "positions"))
	    goto component_done;

	if (! comp)
	    doit = 1;
	else
	{
	    char **c;

	    doit = 0;
	    for (c = comp; *c && !doit; c++)
		if (! strcmp(name, *c))
		    doit = 1;
	}

	if (! doit)
	    goto component_done;
	
	if (! _dxfRemoveOriginal(field, name))
	    goto error;
	
	if (! DXChangedComponentValues(field, name))
	    goto error;
	
	if (DXGetComponentAttribute(field, name, "ref"))
	{
	    DXSetError(ERROR_DATA_INVALID, "#12110");
	    goto error;
	}

	/*
	 * OK.  Now extract the relevant info from the array
	 */
	DXGetArrayInfo(array, &n, &type, &c, &r, s);

	if (n != nPositions)
	{
	    DXSetError(ERROR_DATA_INVALID, "#10400", name, "positions");
	    goto error;
	}

	if (DXQueryConstantArray(array, NULL, NULL))
	{
	    nArray = (Array)DXNewConstantArrayV(nConnections,
				DXGetConstantArrayData(array), type, c, r, s);
	    if (! nArray)
		goto error;
	}
	else
	{
	    srcData = (byte *)DXGetArrayData(array);
	    if (! srcData)
		goto error;

	    itemSize = DXGetItemSize(array);
	    typeSize = DXTypeSize(type);
	    vPerI    = itemSize / typeSize;


	    /*
	     * Now create a local array buffer
	     */
	    accumulator = (byte *)DXAllocateLocal(nConnections*vPerI*sizeof(float));
	    if (! accumulator)
	    {
		DXResetError();
		accumulator = (byte *)DXAllocate(nConnections*vPerI*sizeof(float));
	    }
	    if (! accumulator)
		goto error;
	    
	    memset(accumulator, 0, nConnections*vPerI*sizeof(float));
	    memset(knts, 0, nConnections * sizeof(byte));

	    nArray = DXNewArrayV(type, c, r, s);

	    if (! nArray)
		goto error;
	    
	    if (! DXAddArrayData(nArray, 0, nConnections, NULL))
		goto error;
	    
	    if (regConnections)
	    {
		int pSkip[32], cSkip[32], adj[32];
		typedef struct {
		    int	inc, limit;
		    byte   *cPtr, *pPtr, *kPtr;
		    int 	kSkip, pSkip, cSkip;
		} Loop;
		Loop loop[32];

		pSkip[0] = cSkip[0] = 1;
		for (j = 1; j < cDim; j++)
		{
		    pSkip[j] = pSkip[j-1] * cCounts[(cDim-1)-(j-1)];
		    cSkip[j] = cSkip[j-1] * (cCounts[(cDim-1)-(j-1)] - 1);
		}
		
		for (j = 0; j < (1 << cDim); j++)
		{
		    int k;

		    adj[j] = 0;
		    for (k = 0; k < cDim; k++)
			if (j & (1 << k))
			    adj[j] += pSkip[k];
		}
		
		for (j = 0; j < cDim; j++)
		{
		    loop[j].inc   = 0;
		    loop[j].limit = cCounts[(cDim-1)-j];
		    loop[j].cPtr  = accumulator;
		    loop[j].pPtr  = srcData;
		    loop[j].kPtr  = knts;
		    loop[j].kSkip = cSkip[j];
		    loop[j].pSkip = pSkip[j]*itemSize;
		    loop[j].cSkip = cSkip[j]*vPerI*sizeof(float);
		}


		switch (type)
		{
		    case TYPE_DOUBLE:
			TO_CONNECTIONS_REGULAR(double);
			break;

		    case TYPE_FLOAT:
			TO_CONNECTIONS_REGULAR(float);
			break;

		    case TYPE_INT:
			TO_CONNECTIONS_REGULAR(int);
			break;

		    case TYPE_UINT:
			TO_CONNECTIONS_REGULAR(uint);
			break;

		    case TYPE_SHORT:
			TO_CONNECTIONS_REGULAR(short);
			break;

		    case TYPE_USHORT:
			TO_CONNECTIONS_REGULAR(ushort);
			break;

		    case TYPE_BYTE:
			TO_CONNECTIONS_REGULAR(byte);
			break;

		    case TYPE_UBYTE:
			TO_CONNECTIONS_REGULAR(ubyte);
			break;

		    default:

			DXSetError(ERROR_DATA_INVALID, "#10320", name);
			goto error;
		}
	    }
	    else /* irregular connections */
	    {
		int   l;

		switch (type)
		{
		    case TYPE_DOUBLE:
			TO_CONNECTIONS_IRREGULAR(double);
			break;

		    case TYPE_FLOAT:
			TO_CONNECTIONS_IRREGULAR(float);
			break;

		    case TYPE_UINT:
			TO_CONNECTIONS_IRREGULAR(uint);
			break;

		    case TYPE_INT:
			TO_CONNECTIONS_IRREGULAR(int);
			break;

		    case TYPE_USHORT:
			TO_CONNECTIONS_IRREGULAR(ushort);
			break;

		    case TYPE_SHORT:
			TO_CONNECTIONS_IRREGULAR(short);
			break;

		    case TYPE_BYTE:
			TO_CONNECTIONS_IRREGULAR(byte);
			break;

		    case TYPE_UBYTE:
			TO_CONNECTIONS_IRREGULAR(ubyte);
			break;

		    default:

			DXSetError(ERROR_DATA_INVALID, "#10320", name);
			goto error;
		}
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)nArray))
	    goto error;
	nArray = NULL;

	if (! DXSetComponentAttribute(field, name, "dep", 
					(Object)DXNewString("connections")))
	    goto error;
	
	DXFree((Pointer)accumulator);
	accumulator = NULL;

	DXFree((Pointer)dstData);
	dstData = NULL;

component_done:
	
	i++;
    }

    DXFree(ebuf);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    DXFree((Pointer)knts);
    knts = NULL;

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    ich = NULL;

    return (Object)field;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFree(ebuf);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    DXFree((Pointer)dstData);
    DXFree((Pointer)knts);
    DXFree((Pointer)accumulator);
    DXDelete((Object)nArray);

    return NULL;
}


static Error
_dxfGetComponentList(Object o, Array a, int dir, char **compStrings)
{
    int i;

    if (a)
    {
	i = 0;
	while (DXExtractNthString((Object)a, i, &compStrings[i]))
	    i ++;
	
	compStrings[i] = NULL;

	return OK;
    }
    else
    {
	if (DXGetObjectClass(o) == CLASS_GROUP)
	{
	    Object c;

	    i = 0;
	    while (NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
		if (_dxfGetComponentList(c, NULL, dir, compStrings))
		    break;
	    
	    if (!c || DXGetError() != ERROR_NONE)
		return ERROR;
	}
	else
	{
	    Field f = (Field)o;
	    int j;
	    char *n;
	    Object a;
	    char *dep, *str;

	    if (DXEmptyField(f))
		return ERROR;

	    if (dir == TO_CONNECTIONS)
		str = "connections";
	    else
		str = "positions";

	    i = j = 0;
	    while (NULL != DXGetEnumeratedComponentValue(f, i++, &n))
	    {
		if (!strcmp(n, "positions") || !strcmp(n, "connections"))
		    continue;

		a = DXGetComponentAttribute(f, n, "dep");
		if (! a)
		    continue;

		if (DXGetObjectClass(a) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10200",
					"dependency attribute");
		    return ERROR;
		}

		dep = DXGetString((String)a);

		if (strcmp(dep, str))
		     compStrings[j++] = n;
	    }

	    compStrings[j] = NULL;
	}
    }

    return OK;
}


static Error
_dxfRemoveOriginal(Field f, char *name)
{
    char *origName;

    origName = (char *)DXAllocate(12+strlen(name));
    if (! origName)
	return ERROR;

    sprintf(origName, "original %s", name);

    if (DXGetComponentValue(f, name))
	DXDeleteComponent(f, origName);

    DXFree((Pointer)origName);

    return OK;
}
