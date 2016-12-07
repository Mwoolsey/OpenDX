/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/map.c,v 1.5 2006/01/03 17:02:23 davidt Exp $:
 */

#include <dxconfig.h>


/***
MODULE:
 DXMap
SHORTDESCRIPTION:
    applies a function defined by a map onto a field
CATEGORY:
 Data Transformation
INPUTS:
 input;           field or vector list;        none; field to map
 map;          scalar, vector or field;    identity; map to use
 source;                        string; "positions"; component to use as index into the map
 destination;                   string;      "data"; component to put result in
OUTPUTS:
 output;          field or vector list;        NULL; input field mapped according to map
FLAGS:
BUGS:
AUTHOR:
 Greg Abram
END:
***/

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <stdio.h>
#include <dx/dx.h>

#define DEFAULT_DESTINATION	"data"
#define DEFAULT_SOURCE		"positions"

#define COMPONENT_LENGTH	64

typedef struct
{
    Object	target;
    Object	map;
    char	srcComponent[COMPONENT_LENGTH];
    char	dstComponent[COMPONENT_LENGTH];
} MapTask;

static Object Map_Object(Object, Object, char *, char *);
static Error  Map_Task(Pointer);
       Object DXMap(Object, Object, char *, char *);
static Error  SetGroupTypeVRecursive(Object, Type, Category, int, int *);


int
m_Map(Object *in, Object *out)
{
    char     *dstComponent;
    char     *srcComponent;
    Object   map;
    Type     type;
    Category cat;
    int	     rank, shape[32];
    Object   input;

    out[0] = NULL;
    input  = NULL;
    map    = NULL;

    if(! in[0])
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	goto error;
    }

    if (! in[1])
    {
	out[0] = in[0];
	return OK;
    }
	
    srcComponent = DEFAULT_SOURCE;
    if (in[2] && ! DXExtractString(in[2], &srcComponent))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "source");
	goto error;
    }

    dstComponent = DEFAULT_DESTINATION;
    if (in[3] && ! DXExtractString(in[3], &dstComponent))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10200", "destination");
	goto error;
    }

    /*
     * If input is an array, make sure its type float
     */
    if (DXGetObjectClass(in[0]) == CLASS_ARRAY)
    {
	Type type;
	Category cat;
	int  nItems, rank, shape[32];

	DXGetArrayInfo((Array)in[0], &nItems, &type, &cat, &rank, shape);
	if (cat != CATEGORY_REAL)
	{
	    DXSetError(ERROR_DATA_INVALID, "#11150", "input");
	    goto error;
	}

#define CONVERT_TYPE(ctype)					\
{								\
    int i;							\
    ctype *iPtr;						\
    float *fPtr;						\
								\
    input = (Object)DXNewArrayV(TYPE_FLOAT, cat, rank, shape);	\
    if (! input)						\
	goto error;						\
	    							\
    if (! DXAddArrayData((Array)input, 0, nItems, NULL))	\
	goto error;						\
								\
    iPtr = (ctype *)DXGetArrayData((Array)in[0]);		\
    fPtr = (float *)DXGetArrayData((Array)input);		\
								\
    nItems *= (DXGetItemSize((Array)in[0])/sizeof(ctype));	\
								\
    for (i = 0; i < nItems; i++)				\
	*fPtr++ = (float)*iPtr++;				\
	    							\
    type = TYPE_FLOAT;						\
}
	switch(type)
	{
	    case TYPE_DOUBLE: CONVERT_TYPE(double); break;
	    case TYPE_FLOAT:  input = in[0];        break;
	    case TYPE_INT:    CONVERT_TYPE(int);    break;
	    case TYPE_UINT:   CONVERT_TYPE(uint);   break;
	    case TYPE_SHORT:  CONVERT_TYPE(short);  break;
	    case TYPE_USHORT: CONVERT_TYPE(ushort); break;
	    case TYPE_BYTE:   CONVERT_TYPE(byte);   break;
	    case TYPE_UBYTE:  CONVERT_TYPE(ubyte);  break;
	    default:
		DXSetError(ERROR_DATA_INVALID, "#10320", "input");
	}

#undef CONVERT_TYPE

    }
    else
	input = in[0];

    DXReference(input);
	    
    if (! DXMapCheck(input, in[1], srcComponent, &type, &cat, &rank, shape))
	goto error;
    
    /*
     * If the map is to be interpolated, create an interpolator.
     */
    if (DXGetObjectClass(in[1]) == CLASS_FIELD ||
	(DXGetObjectClass(in[1]) == CLASS_GROUP &&
	    (DXGetGroupClass((Group)in[1]) == CLASS_COMPOSITEFIELD ||
	    (DXGetGroupClass((Group)in[1]) == CLASS_MULTIGRID))))
    {
	map = (Object)DXNewInterpolator(in[1], INTERP_INIT_PARALLEL, -1.0);
	if (! map)
	    goto error;
    }
    else
	map = in[1];
    
    DXReference(map);
    
    if (DXGetObjectClass(input) == CLASS_ARRAY)
    {
	out[0] = DXMap(input, map, NULL, NULL);

	if (! out[0] || DXGetError())
	    goto error;
    }
    else
    {
	out[0] = DXApplyTransform(input, NULL);
	if (! out[0])
	    goto error;
    
	DXCreateTaskGroup();

	if (! Map_Object(out[0], map, srcComponent, dstComponent)
			|| DXGetError() != ERROR_NONE)
	{
	    DXAbortTaskGroup();
	    goto error;
	}

	if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	    goto error;

	if ( !strcmp(dstComponent, "data"))
	{
	    if (! SetGroupTypeVRecursive(out[0], type, cat, rank, shape))
		goto error;
	}
    }

    DXDelete(map);
    DXDelete(input);

    return OK;

error:
    DXDelete(map);
    if (input != in[0]) DXDelete(input);
    DXDelete(out[0]);
    out[0] = NULL;

    return ERROR;
}

static Object
Map_Object(Object target, Object map, char *srcComponent, char *dstComponent)
{
    int	    i;
    Object  child;
    MapTask task;

    switch(DXGetObjectClass(target))
    {
	case CLASS_ARRAY:
	case CLASS_FIELD:
	    task.target = target;
	    task.map    = map;
	    strncpy(task.srcComponent, srcComponent, COMPONENT_LENGTH);
	    strncpy(task.dstComponent, dstComponent, COMPONENT_LENGTH);

	    if (! DXAddTask(Map_Task, (Pointer)&task, sizeof(task), 1.0))
		return NULL;
	    break;
	
	case CLASS_GROUP:
	    i = 0;
	    while ((child=DXGetEnumeratedMember((Group)target, i, NULL)) != NULL)
	    {
		if (! Map_Object(child, map, srcComponent, dstComponent))
		    return NULL;

		i++;
	    }

	    break;

	case CLASS_XFORM:
	    if (! DXGetXformInfo((Xform)target, &child, NULL))
		return NULL;
	    
	    return Map_Object(child, map, srcComponent, dstComponent);

	case CLASS_CLIPPED:
	    if (! DXGetClippedInfo((Clipped)target, &child, NULL))
		return NULL;
	    
	    return Map_Object(child, map, srcComponent, dstComponent);

	default:
	    break;
    }

    return target;
}

static Error
Map_Task(Pointer taskPtr)
{
    MapTask *task;
    Object   map;

    task = (MapTask *)taskPtr;

    if (DXGetObjectClass(task->map) == CLASS_INTERPOLATOR)
    {
	if (! (map = DXCopy(task->map, COPY_STRUCTURE)))
	    return ERROR;
    }
    else
        map = task->map;

    DXReference(map);
	
    if (ERROR == DXMap(task->target, map, task->srcComponent, task->dstComponent))
    {
	DXDelete(map);
	return ERROR;
    }

    DXDelete(map);
    return OK;
}

static Error
SetGroupTypeVRecursive(Object object,
			Type type, Category cat, int rank, int *shape)
{
    int    i;
    Object child;
    Class class;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);

    switch(class)
    {
	case CLASS_CLIPPED:
	   if (! DXGetClippedInfo((Clipped)object, &child, NULL))
	       return ERROR;
	    
	   return SetGroupTypeVRecursive(child, type, cat, rank, shape);
	
	case CLASS_XFORM:
	   if (! DXGetXformInfo((Xform)object, &child, NULL))
	       return ERROR;
	    
	   return SetGroupTypeVRecursive(child, type, cat, rank, shape);
	
	case CLASS_GROUP:

	    i = 0;
	    while ((child=DXGetEnumeratedMember((Group)object, i, NULL)) != NULL)
	    {
		if (! SetGroupTypeVRecursive(child, type, cat, rank, shape))
		    return ERROR;
		
		i ++;
	    }

	    return OK;

        case CLASS_MULTIGRID:
        case CLASS_COMPOSITEFIELD:
	case CLASS_SERIES:

	    i = 0;
	    while ((child=DXGetEnumeratedMember((Group)object, i, NULL)) != NULL)
	    {
		if (! SetGroupTypeVRecursive(child, type, cat, rank, shape))
		    return ERROR;
		
		i ++;
	    }

	    if (! DXSetGroupTypeV((Group)object, type, cat, rank, shape))
		return ERROR;

	    return OK;
	
	default:
	    return OK;
    }

}
