/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/dxmods/RCS/_grow.c,v 5.0 92/11/12 09:12:57 svs Exp Locker: gresh 
 *
 * 
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <dx/dx.h>

static int  IsRegular(Object);
static Object GrowObject(Object, int, Pointer, Array);

extern Object _dxfRegGrow(Object, int, Pointer, Array);
extern Object _dxfIrregGrow(Object, int, Array);

extern Object _dxfRegInvalidateDupBoundary(Object);
extern Object _dxfIrregInvalidateDupBoundary(Object);

extern Object _dxfRegShrink(Object);
extern Object _dxfIrregShrink(Object);

       Array  _dxfReRef(Array, int);

#define IS_REGULAR		0
#define IS_NOT_REGULAR		1
#define IS_REGULAR_ERROR	2

Object
DXGrowV(Object object, int rings, Pointer fill, char **componentNames)
{
    int i;
    Array components = NULL;

    if (rings < 1)
	return object;

    if (componentNames && componentNames[0] != NULL)
    {
	for (i = 0; componentNames[i]; i++);
	components = DXMakeStringListV(i, componentNames);
	if (! components)
	    return NULL;
    }
    else
	componentNames = NULL;

    if (! GrowObject(object, rings, fill, components))
	goto error;
    
    if (components)
	DXFree((Object)components);

    return object;

error:
    if (components)
	DXFree((Object)components);

    return NULL;
}

Object
DXGrow(Object object, int rings, Pointer fill, ...)
{
    char *components[100];
    int i;
    va_list arg;

    va_start(arg,fill);
    for (i = 0; i < 100; i++)
	if (NULL == (components[i] = va_arg(arg, char *)))
	    break;
    va_end(arg);

    return DXGrowV(object, rings, fill, components);
}

static Object
GrowObject(Object object, int rings, Pointer fill, Array components)
{

    switch(DXGetObjectClass(object))
    {
	case CLASS_FIELD:
	{
	    break;
	}
	
	case CLASS_GROUP:
	{
	    if (DXGetGroupClass((Group)object) == CLASS_COMPOSITEFIELD)
	    {
		int regularity = IsRegular(object);

	        if (regularity == IS_REGULAR)
		{
		    if (!_dxfRegGrow(object, rings, fill, components))
			goto error;
		}
		else if (regularity == IS_NOT_REGULAR)
		{
		    if (! _dxfIrregGrow(object, rings, components))
			goto error;
		}
	    }
	    else
	    {
		Object c;
		Group g = (Group)object;
		int i;
	    
		for (i=0; NULL != (c = DXGetEnumeratedMember(g, i, NULL)); i++)
		    if (! GrowObject(c, rings, fill, components))
			return NULL;
	    }
	    break;
	}

	case CLASS_XFORM:
	{
	    Object x;
		
	    if (! DXGetXformInfo((Xform)object, &x, NULL))
		goto error;
	    
	    if (! GrowObject(x, rings, fill, components))
		goto error;
	}
	    
	case CLASS_CLIPPED:
	{
	    Object x;
		
	    if (! DXGetClippedInfo((Clipped)object, &x, NULL))
		goto error;
	    
	    if (! GrowObject(x, rings, fill, components))
		goto error;
	}

	default:
	    return NULL;
    }

    return object;

error:
    return NULL;
}

Object
DXShrink(Object object)
{
    switch(DXGetObjectClass(object))
    {
	case CLASS_FIELD:
	{
	    break;
	}
	
	case CLASS_GROUP:
	{
	    if (DXGetGroupClass((Group)object) == CLASS_COMPOSITEFIELD)
	    {
		int regularity = IsRegular(object);

	        if (regularity == IS_REGULAR)
		{
		    if (!_dxfRegShrink(object))
			goto error;
		}
		else if (regularity == IS_NOT_REGULAR)
		{
		    if (! _dxfIrregShrink(object))
			goto error;
		}
	    }
	    else
	    {
		Object c;
		Group g = (Group)object;
		int i;
	    
		for (i=0; NULL != (c = DXGetEnumeratedMember(g, i, NULL)); i++)
		    if (! DXShrink(c))
			return NULL;
	    }
	    break;
	}

	case CLASS_XFORM:
	{
	    Object x;
		
	    if (! DXGetXformInfo((Xform)object, &x, NULL))
		goto error;
	    
	    if (! DXShrink(x))
		goto error;
	}
	    
	case CLASS_CLIPPED:
	{
	    Object x;
		
	    if (! DXGetClippedInfo((Clipped)object, &x, NULL))
		goto error;
	    
	    if (! DXShrink(x))
		goto error;
	}

	default:
	    return NULL;
    }

    return object;

error:
    return NULL;
}

Object
DXInvalidateDupBoundary(Object object)
{
    switch(DXGetObjectClass(object))
    {
	case CLASS_FIELD:
	{
	    Object a = DXGetComponentValue((Field)object, "invalid positions");
	    if (a)
		if (! DXSetComponentValue((Field)object, "saved invalid positions", a))
		    goto error;
	    
	    break;
	}
	
	case CLASS_GROUP:
	{
	    if (DXGetGroupClass((Group)object) == CLASS_COMPOSITEFIELD)
	    {
		int regularity = IsRegular(object);

	        if (regularity == IS_REGULAR)
		{
		    if (!_dxfRegInvalidateDupBoundary(object))
			goto error;
		}
		else if (regularity == IS_NOT_REGULAR)
		{
		    if (! _dxfIrregInvalidateDupBoundary(object))
			goto error;
		}
	    }
	    else
	    {
		Object c;
		Group g = (Group)object;
		int i;
	    
		for (i=0; NULL != (c = DXGetEnumeratedMember(g, i, NULL)); i++)
		    if (! DXInvalidateDupBoundary(c))
			return NULL;
	    }
	    break;
	}

	case CLASS_XFORM:
	{
	    Object x;
		
	    if (! DXGetXformInfo((Xform)object, &x, NULL))
		goto error;
	    
	    if (! DXInvalidateDupBoundary(x))
		goto error;
	    
	    break;
	}
	    
	case CLASS_CLIPPED:
	{
	    Object x;
		
	    if (! DXGetClippedInfo((Clipped)object, &x, NULL))
		goto error;
	    
	    if (! DXInvalidateDupBoundary(x))
		goto error;
	    
	    break;
	}

	default:
	    return object;
    }

    return object;

error:
    return NULL;
}

Object
DXRestoreDupBoundary(Object object)
{
    switch(DXGetObjectClass(object))
    {
	case CLASS_FIELD:
	{
	    Array a = (Array)DXGetComponentValue((Field)object, "saved invalid positions");
	    if (a)
	    {
		if (DXGetComponentValue((Field)object, "invalid positions"))
		    DXDeleteComponent((Field)object, "invalid positions");

		if (! DXSetComponentValue((Field)object, 
				    "invalid positions", (Object)a))
		    goto error;

		DXDeleteComponent((Field)object, "saved invalid positions");
	    }
	    else
		DXDeleteComponent((Field)object, "invalid positions");

	    break;
	}
	
	case CLASS_GROUP:
	{
	    Object c;
	    Group g = (Group)object;
	    int i;
	
	    for (i=0; NULL != (c = DXGetEnumeratedMember(g, i, NULL)); i++)
		if (! DXRestoreDupBoundary(c))
		    return NULL;

	    break;
	}

	case CLASS_XFORM:
	{
	    Object x;
		
	    if (! DXGetXformInfo((Xform)object, &x, NULL))
		goto error;
	    
	    if (! DXRestoreDupBoundary(x))
		goto error;
	    
	    break;
	}
	    
	case CLASS_CLIPPED:
	{
	    Object x;
		
	    if (! DXGetClippedInfo((Clipped)object, &x, NULL))
		goto error;
	    
	    if (! DXRestoreDupBoundary(x))
		goto error;
	    
	    break;
	}

	default:
	    return object;
    }

    return object;

error:
    return NULL;
}

static int
IsRegular(Object in)
{
    int    i;
    Array  array;
    Object child;
    int    result = IS_REGULAR_ERROR;

    switch(DXGetObjectClass(in))
    {
	case CLASS_GROUP:

	    i = 0;
	    while (NULL != (child = DXGetEnumeratedMember((Group)in, i++, NULL)))
	    {
		result = IsRegular(child);
		if (result == IS_NOT_REGULAR)
		    return result;
	    }

	    return result;

        case CLASS_FIELD:

	    if (DXEmptyField((Field)in))
		return IS_REGULAR_ERROR;
		
	    array = (Array)DXGetComponentValue((Field)in, "connections");
	    if (! array)
		return IS_REGULAR_ERROR;

	    if (DXQueryGridConnections(array, NULL, NULL))
		return IS_REGULAR;
	    else
		return IS_NOT_REGULAR;

        default:

	    DXSetError(ERROR_DATA_INVALID, "IsRegular: data not group or field");
	    return IS_REGULAR_ERROR;
    }
}

/* --FIXME
 * The following two functions are not used anywhere, determined by
 * the compiler. Commented out by DT

static char **
makeGlobalStrings(char **locPtrs)
{
    int i, j, k;
    char **l, **g, **gblPtrs;

    if (! locPtrs)
	return NULL;
    
    if (! *locPtrs)
	return NULL;

    for (i = 0, l = locPtrs; *l; l++, i++);

    gblPtrs = (char **)DXAllocate((i+1) * sizeof(char *));

    for (j = 0, l = locPtrs, g = gblPtrs; j <= i; j++, l++, g++)
    {
	if (! *l)
	    *g = NULL;
	else
	{
	    k = strlen(*l) + 1;
	    *g = DXAllocate(k * sizeof(char));
	    if (! *g)
	    {
		while (--j >= 0)
		    DXFree((Pointer)gblPtrs[j]);
		DXFree((Pointer)gblPtrs);
		return NULL;
	    }

	    strcpy(*g, *l);
	}
    }

    return gblPtrs;
}

static void
freeGlobalStrings(char **gblPtrs)
{
    char **g;

    if (gblPtrs)
    {
	for (g = gblPtrs; *g; g++)
	    DXFree((Pointer)*g);
	DXFree((Pointer)gblPtrs);
    }
}

****/

Field
DXQueryOriginalSizes(Field partition, int *positions, int *connections)
{
    Array cArray, pArray;

    pArray = (Array)DXGetComponentValue(partition, "original positions");
    cArray = (Array)DXGetComponentValue(partition, "original connections");

    if (! pArray || ! cArray)
	return NULL;

    if (positions)
	DXGetArrayInfo(pArray, positions, NULL, NULL, NULL, NULL);

    if (connections)
	DXGetArrayInfo(cArray, connections, NULL, NULL, NULL, NULL);

    return partition;
}

Array
_dxfReRef(Array array, int n)
{
    Type     type;
    Category cat;
    int      rank, shape[32];
    int      nItems, nReferences;
    int      *ptr, i;

    DXGetArrayInfo(array, &nItems, &type, &cat, &rank, shape);

    if (type != TYPE_INT || cat != CATEGORY_REAL)
    {
	DXSetError(ERROR_DATA_INVALID, "referencing array must be INT/REAL");
	return NULL;
    }

    nReferences = nItems;
    for (i = 0; i < rank; i++)
	nReferences *= shape[i];
    
    ptr = (int *)DXGetArrayData(array);
    for (i = 0; i < nReferences; i++, ptr++)
	if (*ptr >= n)
	    *ptr = -1;

    return array;
}

#define TYPE int
#define LT(a,b) ((*(a))<(*(b)))
#define GT(a,b) ((*(a))>(*(b)))
#define QUICKSORT refsort

#include "qsort.c"

#define SetOrigName(buf, name)  sprintf((buf), "original %s", (name))

Error
_dxf_RemoveDupReferences(Field field)
{
    Array    iA, oA = NULL;
    int      i, n, nIn, nOut, r, s[64], nref;
    char     *name, origName[256];
    int      *sPtr, *dPtr, *p0, *p1;
    Type     t;
    Category c;

    n = 0;
    while (NULL != (iA=(Array)DXGetEnumeratedComponentValue(field, n++, &name)))
    {
	SetOrigName(origName, name);

	if (! DXGetComponentValue(field, origName))
	    continue;

	if (DXGetComponentAttribute(field, name, "dep"))
	    continue;
	
	if (NULL == DXGetComponentAttribute(field, name, "ref"))
	    continue;
	
	if (! strcmp(name, "connections"))
	    continue;
	
	/*
	 * Remember: only single references are allowed
	 */
	DXGetArrayInfo(iA, &nIn, &t, &c, &r, s);

	if (nIn == 0)
	    continue;

	if (t != TYPE_INT ||
	    c != CATEGORY_REAL)
	{
	    DXSetError(ERROR_DATA_INVALID, 
			"invalid referential component: %s", name);
	    goto error;
	}

	nref = DXGetItemSize(iA) / sizeof(int);
	if (nref != 1)
	    continue;

	sPtr = (int *)DXGetArrayData(iA);

	refsort(sPtr, nIn);

	p0 = sPtr;
	p1 = sPtr+1;
	nOut = 1;
	for (i = 1; i < nIn; i++, p1++)
	    if (*p0 != *p1)
	    {
		p0 = p1;
		nOut++;
	    }
	
	if (nIn != nOut)
	{
	    oA = DXNewArrayV(t, c, r, s);
	    if (! oA)
		goto error;
	    
	    if (! DXAddArrayData(oA, 0, nOut, NULL))
		goto error;
	    
	    dPtr = (int *)DXGetArrayData(oA);

	    *dPtr = *sPtr++;
	    for (i = 1; i < nIn; i++, sPtr++)
		if (*dPtr != *sPtr)
		{
		    dPtr++;
		    *dPtr = *sPtr;
		}
	    
	    if (! DXSetComponentValue(field, name, (Object)oA))
		goto error;
	    oA = NULL;
	}
    }

    return OK;

error:
    DXDelete((Object)oA);
    return ERROR;
}
	
