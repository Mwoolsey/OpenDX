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
#include "interpClass.h"

#define MAX_SHAPE		32

/*
 * Parallelism will be applied by the calling procedure
 */
#undef PARALLEL

typedef struct
{
    Field		dataField;
    Object		map;
    char		*dst;
    char		*src;
} MapTask;

#define NT_MISSING	0x1
#define NT_MISMATCH	0x2
#define NT_OK		0x4
#define NT_EMPTY	0x8
#define NT_ERROR	(NT_MISSING | NT_MISMATCH)

static Error  Map_Object(Object, Object, char *, char *);
static Error  Map_Field(Field, Object, char *, char *);
static Error  Map_Field_Task(Pointer);
static Error  Map_Group(Group, Object, char *, char *);
static Array  Map_Field_Constant(Array, Array);
static Error  SetGroupTypeVRecursive(Object, Type, Category, int, int *);
static int    GetNamedType(Object, char *, Type *, Category *, int *, int *);
static int    CheckNamedType(Object, char *, Type, Category, int, int *);
static Array  MapArrayIrreg(Array, Interpolator, Array *);
static Array  MapArrayConstantReg(RegularArray, Interpolator, Array *);
static Object DXGetMapType(Object, Type *, Category *, int *, int *);

Object
DXMap(Object g_in, Object map, char *srcComponent, char *dstComponent)
{
    Object   result;
    Type     type;
    Category cat;
    int	     rank, shape[32];

    /* 
     * map had better be an array, a field, an interpolator, or a
     * composite field.
     */
    switch(DXGetObjectClass(map))
    {
	case CLASS_ARRAY:
	case CLASS_FIELD:
	case CLASS_XFORM:
	case CLASS_INTERPOLATOR:
	    break;
	
	case CLASS_GROUP:
	    if (DXGetGroupClass((Group)map) == CLASS_COMPOSITEFIELD ||
		DXGetGroupClass((Group)map) == CLASS_MULTIGRID)
		break;
	
	default:
	    DXSetError(ERROR_DATA_INVALID, "map type invalid");
	    return NULL;
    }

    if (DXGetObjectClass(g_in) == CLASS_ARRAY)
    {

	if (DXGetObjectClass(map) == CLASS_ARRAY)
	{
	    int	     nItems;

	    if (! DXGetArrayInfo((Array)map, &nItems, &type, &cat, &rank, shape))
		return NULL;

	    if (nItems != 1)
	    {
		DXSetError(ERROR_DATA_INVALID, "constant array size != 1");
		return NULL;
	    }

	    if (! DXGetArrayInfo((Array)g_in, &nItems, NULL, NULL, NULL, NULL))
		return NULL;

	    if (nItems <= 0)
	    {
		DXSetError(ERROR_DATA_INVALID, "index array length < 1");
		return NULL;
	    }

	    result = (Object)DXNewConstantArrayV(nItems, 
			DXGetArrayData((Array)map), type, cat, rank, shape);
	    
	    return result;
	}
	else
	{
	    Interpolator mapInterp;

	    if (DXGetObjectClass(map) != CLASS_INTERPOLATOR)
	    {
		mapInterp = DXNewInterpolator(map, INTERP_INIT_DELAY, -1.0);
		if (! mapInterp)
		{
		    DXAddMessage("unable to interpolate in map");
		    return NULL;
		}

		DXReference((Object)mapInterp);

		result = (Object)DXMapArray((Array)g_in, mapInterp, NULL);

		DXDelete((Object)mapInterp);
	    }
	    else
		result = (Object)DXMapArray((Array)g_in, (Interpolator)map, NULL);

	    return result;
	}
    }
    else
    {
	result = g_in;

#ifdef PARALLEL
	DXCreateTaskGroup();
#endif

	if (! Map_Object(result, map, srcComponent, dstComponent))
	{
	    DXAddMessage("mapping function failed");
#ifdef PARALLEL
	    DXAbortTaskGroup();
#endif
	    return NULL;
	}

#ifdef PARALLEL
	DXExecuteTaskGroup();
#endif

	/* 
	 * If we created the data component, then we may need to 
	 * change the type of g_out
	 */
	if (! strcmp(dstComponent, "data"))
	{
	    Type	type;
	    Category	cat;
	    int		rank, shape[MAX_SHAPE];

	    if (! DXGetMapType(map, &type, &cat, &rank, shape))
		return NULL;

	    if (! SetGroupTypeVRecursive(result, type, cat, rank, shape))
		return NULL;
	}
    }
	
    return result;
}

static Object
DXGetMapType(Object map, Type *t, Category *c, int *r, int *s)
{
    switch(DXGetObjectClass(map))
    {
	case CLASS_INTERPOLATOR:
	    return DXGetMapType(((Interpolator)map)->rootObject, t, c, r, s);
	case CLASS_XFORM:
	{
	    Object chile;

	    if (! DXGetXformInfo((Xform)map, &chile, NULL))
		return ERROR;
	    
	    return DXGetMapType(chile, t, c, r, s);
	}
	default:
	    return DXGetType(map, t, c, r, s);
    }
}

static Error
Map_Object(Object inObject, Object map, char *src, char *dst)
{
    Object child;

    switch(DXGetObjectClass(inObject))
    {
	case CLASS_GROUP:
	    return  Map_Group((Group)inObject, map, src, dst);
	
	case CLASS_FIELD:
	    return  Map_Field((Field)inObject, map, src, dst);
	
	case CLASS_XFORM:
	    if (! DXGetXformInfo((Xform)inObject, &child, NULL))
		return ERROR;
	    
	    return Map_Object(child, map, src, dst);
	
	case CLASS_CLIPPED:
	    if (! DXGetClippedInfo((Clipped)inObject, &child, NULL))
		return ERROR;
	    
	    return Map_Object(child, map, src, dst);
	
	default:
	    DXSetError(ERROR_DATA_INVALID, "Unknown object class");
	    return ERROR;
    }
}

static Error
Map_Group(Group in, Object map, char *src, char *dst)
{
    int      i;
    Object   child;

    for (i = 0; (child=DXGetEnumeratedMember(in, i, NULL)); i++)
	if (! Map_Object(child, map, src, dst))
	    return ERROR;
    
    return OK;
}

static Error
Map_Field(Field in, Object map, char *src, char *dst)
{
    MapTask task;

    if (! DXEmptyField(in))
    {
	task.dataField	   = in;
	task.map	   = map;
	task.src	   = src;
	task.dst	   = dst;

#ifdef PARALLEL
	DXAddTask(Map_Field_Task, (Pointer)&task, sizeof(task), 1.0);
#else
	if (! Map_Field_Task((Pointer)&task))
	    return ERROR;
#endif
    }
    else
	DXDeleteComponent(in, "data");

    return OK;
}

static Error
Map_Field_Task(Pointer task)
{
    Field        in;
    Object	 map;
    char	 *src;
    char	 *dst;
    Interpolator mapInterp;
    Array	 indexArray;
    Array	 newArray;
    Array	 invalidArray;
    Object	 attr;
    char	 *invalidName = NULL;
    char 	 *invalidAssociation = NULL;
    Array	 origInvalid;

    in 		= ((MapTask *)task)->dataField;
    map		= ((MapTask *)task)->map;
    src         = ((MapTask *)task)->src;
    dst         = ((MapTask *)task)->dst;

    /*
     * Get the index array and make sure its appropriate for
     * indexing the specified map
     */
    indexArray = (Array)DXGetComponentValue(in, src);
    if (! indexArray)
    {
	DXSetError(ERROR_MISSING_DATA, 
			"Error accessing source component");
	return ERROR;
    }

    attr = DXGetComponentAttribute(in, src, "dep");
    if (!strcmp(src, "positions") ||
		   (attr && !strcmp(DXGetString((String)attr), "positions")))
    {
	invalidName        = "invalid positions";
	invalidAssociation = "positions";
	invalidArray       = (Array)DXGetComponentValue(in, invalidName);
    }
    else if (!strcmp(src, "connections") ||
		 (attr && !strcmp(DXGetString((String)attr), "connections")))
    {
	invalidName        = "invalid connections";
	invalidAssociation = "connections";
	invalidArray       = (Array)DXGetComponentValue(in, invalidName);
    }
    else if (!strcmp(src, "polylines") ||
		 (attr && !strcmp(DXGetString((String)attr), "polylines")))
    {
	invalidName        = "invalid polylines";
	invalidAssociation = "polylines";
	invalidArray       = (Array)DXGetComponentValue(in, invalidName);
    }
    else if (!strcmp(src, "faces") ||
		 (attr && !strcmp(DXGetString((String)attr), "faces")))
    {
	invalidName        = "invalid faces";
	invalidAssociation = "faces";
	invalidArray       = (Array)DXGetComponentValue(in, invalidName);
    }
    else 
    {
	invalidName        = NULL;
	invalidArray       = NULL;
    }

    origInvalid = invalidArray;

    switch(DXGetObjectClass(map))
    {
        case CLASS_ARRAY:
	    if (! (newArray = Map_Field_Constant(indexArray, (Array)map)))
		return ERROR;
	    break;
	
	/*
	 * Groups: make sure composite field, then handle as field.
	 */
	case CLASS_GROUP:
	    if (DXGetGroupClass((Group)map) != CLASS_COMPOSITEFIELD ||
		DXGetGroupClass((Group)map) != CLASS_MULTIGRID)
	    {
		DXSetError(ERROR_DATA_INVALID, "invalid map");
		return ERROR;
	    }
	case CLASS_XFORM:
	case CLASS_FIELD:
	    mapInterp = DXNewInterpolator(map, INTERP_INIT_DELAY, -1.0);
	    if (! mapInterp)
	    {
		DXAddMessage("unable to interpolate in map");
		return ERROR;
	    }
	    
	    DXReference((Object)mapInterp);

	    newArray = DXMapArray(indexArray, mapInterp, &invalidArray);
	    if (! newArray)
	    {
		DXDelete((Object)mapInterp);
		return ERROR;
	    }

	    DXDelete((Object)mapInterp);
	    break;
	
	case CLASS_INTERPOLATOR:
	    newArray = DXMapArray(indexArray, (Interpolator)map, &invalidArray);
	    if (! newArray)
		return ERROR;
	    
	    break;
	
	default:
	    DXSetError(ERROR_DATA_INVALID, "invalid map");
	    return ERROR;
    }

    if (invalidArray && invalidArray != origInvalid)
    {
	if (invalidName)
	{
	    Type type;

	    if (DXGetComponentValue(in, invalidName))
		DXDeleteComponent(in, invalidName);

	    DXSetComponentValue(in, invalidName, (Object)invalidArray);
	    
	    DXGetArrayInfo(invalidArray, NULL, &type, NULL, NULL, NULL);

	    if (type == TYPE_INT || type == TYPE_UINT)
		DXSetComponentAttribute(in, invalidName, 
			    "ref", (Object)DXNewString(invalidAssociation));
	    else
		DXSetComponentAttribute(in, invalidName, 
			    "dep", (Object)DXNewString(invalidAssociation));
	}
	else
	{
	    if (attr)
	    {
		DXWarning("%s%s%s", "Unable to create \"invalid ",
		    DXGetString((String)attr),
		    "\" component.  Unmapped values will not be invalidated.");
	    }
	    else
	    {
		DXWarning("%s%s%s", 
		    "Unable to create invalid data component when dependency",
		    " of the input data is not given.",
		    " Unmapped values will not be invalidated.");
	    }
	    DXDelete((Object)invalidArray);
	}
    }

    DXSetComponentValue(in, dst, (Object)newArray);

    if (! strcmp(src, "connections"))
	attr = (Object)DXNewString("connections");
    else if (! strcmp(src, "faces"))
	attr = (Object)DXNewString("faces");
    else if (! strcmp(src, "polylines"))
	attr = (Object)DXNewString("polylines");
    else
	attr = DXGetComponentAttribute(in, src, "dep");
    if (attr)
	DXSetComponentAttribute(in, dst, "dep", attr);

    DXChangedComponentValues(in, dst);

    if (! DXEndField(in))
	return ERROR;

    return OK;
}

Array
DXMapArray(Array indexArray, Interpolator mapInterp, Array *iaPtr)
{
    /*
     * If the array is regular and constant then create
     * a constant regular result.  Otherwise, create irregular result.  
     */
    if (DXQueryConstantArray(indexArray, NULL, NULL))
	return MapArrayConstantReg((RegularArray)indexArray, mapInterp, iaPtr);
    else
	return MapArrayIrreg(indexArray, mapInterp, iaPtr);
}

static Array
MapArrayConstantReg(RegularArray indexArray, 
			Interpolator mapInterp, Array *iaPtr)
{
    Array        newArray = NULL;
    Pointer	 data = NULL;
    Pointer      point = NULL;
    int		 i, dataSize;
    Type 	 type;
    Category	 cat;
    int		 rank, shape[100];
    int		 count;
    InvalidComponentHandle ich = NULL;

    newArray  = NULL;

    DXGetArrayInfo((Array)indexArray, &count, NULL, NULL, NULL, NULL);

    if (! DXGetMapType((Object)mapInterp, &type, &cat, &rank, shape))
    {
	DXSetError(ERROR_DATA_INVALID, "unable to access data type of map");
	return NULL;
    }

    dataSize = 1;
    for (i = 0; i < rank; i++)
	dataSize *= shape[i];
    dataSize *= (DXTypeSize(type) * DXCategorySize(cat));

    data  = DXAllocate(dataSize);
    if (!data)
	goto error;
    
    point = DXGetConstantArrayData((Array)indexArray);
    if (! point)
	goto error;
    
    i = 1;
    if (! DXInterpolate(mapInterp, &i, (float *)point, data))
	goto error;
    
    if (i)
    {
	memset(data, 0, dataSize);
	
	if (iaPtr)
	{
	    ich = DXCreateInvalidComponentHandle((Object)indexArray,
							*iaPtr, NULL);
		
	    if (! ich)
		goto error;
		
	    DXSetAllInvalid(ich);

	    *iaPtr = DXGetInvalidComponentArray(ich);

	    DXFreeInvalidComponentHandle(ich);
	    ich = NULL;
	}
    }

    newArray = (Array)DXNewConstantArrayV(count, data, type, cat, rank, shape);
    
error:

    DXFree(data);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    return (Array)newArray;
}

static Array
MapArrayIrreg(Array indexArray, Interpolator mapInterp, Array *iaPtr)
{
    Array	newArray;
    char	*points;
    char	*data;
    int		nItems, next;
    Type	type;
    Category	cat;
    int		rank;
    int		shape[32];
    int		indexItemSize;
    int		dataItemSize;
    int		nItemsRemaining, nInterpolated, nNotInterpolated;
    int 	newInvalid;
    InvalidComponentHandle ich = NULL;

    /*
     * Get info about data array of the map and make the destination
     * array
     */
    if (! DXGetMapType((Object)mapInterp, &type, &cat, &rank, shape))
    {
	DXSetError(ERROR_DATA_INVALID, "unable to access data type of map");
	return NULL;
    }


    newArray = DXNewArrayV(type, cat, rank, shape);
    if (! newArray)
       return ERROR;

    if (! DXGetArrayInfo(indexArray, &nItems, NULL, NULL, NULL, NULL))
	goto error;

    if (nItems == 0)
    {
	DXWarning("Empty index array");
	return newArray;
    }

    if (! DXAddArrayData(newArray, 0, nItems, NULL))
	goto error;

    indexItemSize = DXGetItemSize(indexArray);
    dataItemSize = DXGetItemSize(newArray);

    /*
     * Get pointer to index data and destination buffer
     */
    points = (char *)DXGetArrayData(indexArray);
    data   = (char *)DXGetArrayData(newArray);

    if (DXGetError() != ERROR_NONE)
	goto error;

    /*
     * DXInterpolate the data
     */
    nItemsRemaining = nItems;
    nNotInterpolated = 0;
    if (! DXInterpolate(mapInterp, &nItemsRemaining, (float *)points, data))
	goto error;

    next = 0; newInvalid = 0;
    while (nItemsRemaining != 0)
    {
	nInterpolated = nItems - nItemsRemaining;
	next += nInterpolated;

	if (iaPtr)
	{
	    if (! ich)
	    {
		ich = DXCreateInvalidComponentHandle((Object)indexArray,
								*iaPtr, NULL);
		if (! ich)
		    goto error;
	    }
	    
	    if (! DXIsElementInvalid(ich, next))
	    {
		newInvalid ++;

		if (! DXSetElementInvalid(ich, next))
		    goto error;
	    }
	}

	data   += nInterpolated * dataItemSize;
	memset(data, (char)0, dataItemSize);
	data   += dataItemSize;
	nItemsRemaining --;
	nNotInterpolated ++;
	next ++;

	if (nItemsRemaining == 0)
	    break;

	points += (nInterpolated + 1) * indexItemSize;

	nItems = nItemsRemaining;
	if (! DXInterpolate(mapInterp, &nItemsRemaining, (float *)points, data))
	    goto error;
    }

    if (ich)
    {
	if (newInvalid)
	{
	    *iaPtr = DXGetInvalidComponentArray(ich);
	    if (! *iaPtr)
		goto error;
	}

	DXFreeInvalidComponentHandle(ich);
    }

    return newArray;

error:
    if (newArray)
	DXDelete((Object)newArray);
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    return ERROR;
}

static Array
Map_Field_Constant(Array indexArray, Array map)
{
    Category 	cat;
    Type     	type;
    int		nItems, rank, shape[MAX_SHAPE];
    Array	newArray;
    char 	*oldData;

    DXGetArrayInfo(map, &nItems, &type, &cat, &rank, shape);
    if (nItems != 1)
    {
	DXSetError(ERROR_DATA_INVALID, "constant array nItems != 1");
	return NULL;
    }

    DXGetArrayInfo(indexArray, &nItems, NULL, NULL, NULL, NULL);

    oldData = (char *)DXGetArrayDataLocal(map);
    if (! oldData)
	return NULL;

    newArray = (Array)DXNewConstantArrayV(nItems, (Pointer)oldData,
				type, cat, rank, shape);

    DXFreeArrayDataLocal(map, (Pointer)oldData);

    return newArray;
}

static Error
SetGroupTypeVRecursive(Object object,
			Type type, Category cat, int rank, int *shape)
{
    int    i;
    Object child;
    Class class;

    if (DXGetObjectClass(object) != CLASS_GROUP)
	return OK;
    
    class = DXGetGroupClass((Group)object);

    for (i = 0; (child=DXGetEnumeratedMember((Group)object, i, NULL)); i++)
    {
	if (! SetGroupTypeVRecursive(child, type, cat, rank, shape))
	    return ERROR;
    }

    if (class == CLASS_COMPOSITEFIELD || class == CLASS_MULTIGRID)
	if (! DXSetGroupTypeV((Group)object, type, cat, rank, shape))
	{
	    DXSetError(ERROR_INTERNAL, "unable to set type of mapped group");
	    return ERROR;
	}

    return OK;
}

/*
 * return type of named component of an object.
 * If not guaranteed to be uniform, verify that it is.
 */
static int
GetNamedType(Object obj, char *name, Type *t, Category *c, int *r, int *s)
{
    Type	myType;
    Category	myCat;
    int		myRank, myShape[32];
    Class	class;
    int    	i;
    Group  	gp;
    Object	child;
    int		ret;

    if (t == NULL)
	t = &myType;

    if (c == NULL)
	c = &myCat;

    if (r == NULL)
	r = &myRank;

    if (s == NULL)
	s = myShape;

    class = DXGetObjectClass(obj);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)obj);

    switch (DXGetObjectClass(obj))
    {
	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)obj, &child, 0))
		return NT_ERROR;

	    return GetNamedType(child, name, t, c, r, s);
	
	case CLASS_CLIPPED:
	    
	    if (! DXGetClippedInfo((Clipped)obj, &child, NULL))
		return NT_ERROR;

	    return GetNamedType(child, name, t, c, r, s);

	case CLASS_MULTIGRID:
	case CLASS_COMPOSITEFIELD:
	case CLASS_SERIES:
	case CLASS_GROUP:

	    gp = (Group)obj;

	    /*
	     * Look for a non-empty child.  If child returns non-empty and
	     * error, then return an error.  If it returnd non_empty and
	     * OK, then quit looking.
	     */
	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember(gp, i++, NULL)))
	    {
		ret = GetNamedType(child, name, t, c, r, s);

		if (ret == NT_EMPTY)
		    continue;

		if (ret & NT_ERROR)
		    return ret;

		if (ret == NT_OK)
		    break;
	    }
	
	    /* 
	     * If we never found a non-empty NT_OK one, return NT_EMPTY
	     */
	    if (child == NULL)
		return NT_EMPTY;
	
	    /* 
	     * If its a generic group, check remainder of the children
	     */
	    if (class == CLASS_GROUP)
	    while (NULL != (child = DXGetEnumeratedMember(gp, i++, NULL)))
	    {
		ret = CheckNamedType(child, name, *t, *c, *r, s);

		if (ret & NT_ERROR)
		    return ret;
	    }

	    return NT_OK;

	case CLASS_FIELD:

	    /*
	     * If its a field, then return NT_EMPTY if its empty, NT_ERROR
	     * if the named component is missing from a non-empty field, and
	     * NT_OK  otherwise.
	     */
	    if (DXEmptyField((Field)obj))
		return NT_EMPTY;

	    child = DXGetComponentValue((Field)obj, name);
	    if (! child)
		return NT_MISSING;

	    if (! DXGetType(child, t, c, r, s))
		return NT_ERROR;
		
	    return NT_OK;
	
	case CLASS_ARRAY:

	    if (! DXGetArrayInfo((Array)obj, NULL, t, c, r, s))
		return NT_ERROR;

	    return NT_OK;
	
	default:

	    return NT_OK;
    }
}

/*
 * Verify that the elements of an object match the given type. 
 */
static int
CheckNamedType(Object obj, char *name, Type t, Category c, int r, int *s)
{
    Type 	ot;
    Category 	oc;
    int 	or, os[32];
    int		i;
    Class	class;
    Object	child;

    class = DXGetObjectClass(obj);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)obj);

    switch (class)
    {
	case CLASS_XFORM:
	    if (! DXGetXformInfo((Xform)obj, &child, NULL))
		return NT_ERROR;
	    
	    return CheckNamedType(child, name, t, c, r, s);
	       
	case CLASS_CLIPPED:
	    if (! DXGetClippedInfo((Clipped)obj, &child, NULL))
		return NT_ERROR;
	    
	    return CheckNamedType(child, name, t, c, r, s);
	       
	case CLASS_GROUP:
	{
	    Group  gp;
	    Object child;
	    int    res, ss;

	    gp = (Group)obj;

	    i = 0; ss = NT_EMPTY;
	    while (NULL != (child = DXGetEnumeratedMember(gp, i++, NULL)))
	    {
		res = CheckNamedType(child, name, t, c, r, s);
	    
		if (res & NT_ERROR)
		    return res;
		
		if (res != NT_EMPTY)
		    ss = NT_OK;
	    }

	    return ss;
	}

	case CLASS_SERIES:
	case CLASS_MULTIGRID:
	case CLASS_COMPOSITEFIELD:
	{
	    Group  gp;
	    Object child;
	    int	   res;

	    gp = (Group)obj;

	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember(gp, i++, NULL)))
	    {
		res = CheckNamedType(child, name, t, c, r, s);

		if (res & NT_ERROR)
		    return res;
		
		if (res == NT_OK)
		    return NT_OK;
	    }
	    
	    return NT_EMPTY;
	}
	
	case CLASS_FIELD:
	{
	    Field field;
	    Object child;

	    field = (Field)obj;

	    if (DXEmptyField(field))
		return NT_EMPTY;

	    child = DXGetComponentValue(field, name);
	    if (! child)
		return NT_MISSING;
		
	    if (! DXGetType(child, &ot, &oc, &or, os))
		return NT_ERROR;
		
	    if (t != ot || c != oc ||
		((or != r) && 
		    !((or == 1 && os[0] == 1 && r == 0) ||
		     (or == 0 &&  s[0] == 1 && r == 1))))
		return NT_MISMATCH;

	    /*
	     * Check shape iff we did not have the rank 0 vs rank 1, shape 1
	     * case looked for above (in which case the ranks will mismatch)
	     */
	    if (r == or)
		for (i = 0; i < r; i++)
		    if (s[i] != os[i]) 
		return NT_MISMATCH;
	    
	    return NT_OK;
	}
	
	default:

	    DXSetError(ERROR_DATA_INVALID, "invalid map object");
	    return NT_ERROR;
    }
}


Object
DXMapCheck(Object in, Object map, char *srcComponent,
	Type *type, Category *cat, int *rank, int *shape)
{
    Class    mapClass;
    Type     iT,     mT;
    Category iC,     mC;
    int	     iR,     mR;
    int      iS[32], mS[32];
    int	     i;

    if ((mapClass = DXGetObjectClass(map)) == CLASS_GROUP)
	 mapClass = DXGetGroupClass((Group)map);

    switch(mapClass)
    {
	case CLASS_XFORM:
	{
	    Object c;

	    if (! DXGetXformInfo((Xform)map, &c, NULL))
		return NULL;
	    
	    if (c == NULL)
		return NULL;

	    return DXMapCheck(in, c, srcComponent, type, cat, rank, shape);
	}

	case CLASS_ARRAY:
	{
	    /*
	     * Create a constant array.  If the input is an array, an array 
	     * of the same length is produced, each item of which is the same
	     * as the single item in the map array.  If input is a field or 
	     * composite field, arrays are created as above for each component
	     * specified by srcComponent and added to each field of the input.
	     * Only restriction is that the map array contains only a single
	     * item.
	     */
	    int n;
	    Type t;
	    Category c;

	    if (! DXGetArrayInfo((Array)map, &n, &t, &c, &iR, iS))
		return NULL;
	    else if (n != 1)
	    {
		DXSetError(ERROR_DATA_INVALID, "map is an array of length != 1");
		return NULL;
	    }

	    if (type)
		*type = t;
	    
	    if (cat)
		*cat = c;

	    if (rank)
		*rank = iR;
	
	    if (shape)
		for (i = 0; i < iR; i++)
		    shape[i] = iS[i];
	    
	    return  in;
	}

	case CLASS_FIELD:
	case CLASS_MULTIGRID:
	case CLASS_COMPOSITEFIELD:
	{
	    /*
	     * If the map is either a field or a composite field, we must verify
	     * that the specified index component of the input matches the type
	     * of the positions component of the map.
	     */
	    int r;
	
	    r = GetNamedType(map, "positions", &mT, &mC, &mR, mS);
	    if (r & NT_ERROR)
	    {
		if (r == NT_ERROR)
		    DXAddMessage("invalid positions component in map object");
		else if (r == NT_MISSING)
		    DXSetError(ERROR_MISSING_DATA, "#10258", "map", "positions");
		else if (r == NT_MISMATCH)
		    DXSetError(ERROR_MISSING_DATA, 
				"type mismatch within map object");    
		return NULL;
	    }

	    r = GetNamedType(map, "data", type, cat, rank, shape);
	    if (r & NT_ERROR)
	    {
		if (r == NT_ERROR)
		    DXAddMessage("invalid data component in map object");
		else if (r == NT_MISSING)
		    DXSetError(ERROR_MISSING_DATA, "#10258", "map", "data");
		else if (r == NT_MISMATCH)
		    DXSetError(ERROR_MISSING_DATA, 
				"type mismatch within map object");    
		return NULL;
	    }

	    r = GetNamedType(in, srcComponent, &iT, &iC, &iR, iS);
	    if (r & NT_ERROR)
	    {
		if (r == NT_ERROR)
		    DXAddMessage("invalid %s component in input object",
				 srcComponent);
		else if (r == NT_MISSING)
		    DXSetError(ERROR_MISSING_DATA, 
			       "#10258", "input", srcComponent);
		else if (r == NT_MISMATCH)
		    DXSetError(ERROR_MISSING_DATA, 
				"type mismatch within input object");    

		return NULL;
	    }
	    else if (r == NT_EMPTY)
		return in;

	    if ((iT   != mT) ||
		(iC   != mC) ||
		((iR  != mR) &&
		    !((mR == 1 && mS[0] == 1 && iR == 0) ||
		      (iR == 1 && iS[0] == 1 && mR == 0))))
	    {
		DXSetError(ERROR_DATA_INVALID,
		    "rank/shape mismatch between input and map");
		return NULL;
	    }
	
	    if (mR == iR)
		for (i = 0; i < mR; i++)
		    if (mS[i] != iS[i])
		    {
			DXSetError(ERROR_DATA_INVALID, 
			    "rank/shape mismatch between input and map");
			return NULL;
		    }


	    return in;
	}

	default:
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"Map must be field, composite field or array");
	    return NULL;
	}
    }
}
