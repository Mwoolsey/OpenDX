/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

#define SORT_ASCENDING	0
#define SORT_DESCENDING	1

static Object SortObject(Object o, int flag);

Error
m_Sort(Object *in, Object *out)
{
    int   sortFlag = SORT_ASCENDING;

    out[0] = NULL;

    if (in[1])
    {
	if (! DXExtractInteger(in[1], &sortFlag))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "sort flag");
	    goto error;
	}

	if (sortFlag < 0 || sortFlag > 1)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "sort flag must be 0 or 1");
	    goto error;
	}
    }

    out[0] = SortObject(in[0], sortFlag);

    if (! out[0])
	goto error;

    return OK;

error:
    if (in[0] != out[0])
	DXFree(out[0]);
    out[0] = NULL;
    return ERROR;
}

static int   *SortArray(Array, int);
static int   *MakeMap(int *, int);
static Array  ReMap(Array, int *);
static Array  ShuffleArray(Array, int *);

static Object
SortObject(Object o, int flag)
{
    int *indices = NULL, *map = NULL;
    Object new = NULL;

    switch(DXGetObjectClass(o))
    {
	case CLASS_GROUP: 
	{
	    int    i;
	    Object child;
	    Group  g = (Group)o;
	    Class  c = DXGetGroupClass(g);

	    if (c == CLASS_COMPOSITEFIELD)
	    {
		DXSetError(ERROR_DATA_INVALID,
			"cannot sort objects of class %s",
			(c == CLASS_COMPOSITEFIELD) ?
			    "composite field" : "multigrid");
		goto error;
	    }

	    new = DXCopy(o, COPY_HEADER);
	    if (! new)
		goto error;

	    if (c == CLASS_GROUP || c == CLASS_MULTIGRID)
	    {
		char *name;

		i = 0;
		while (NULL != (child = DXGetEnumeratedMember(g, i++, &name)))
		{
		    child = SortObject(child, flag);
		    if (! child)
			goto error;
		    
		    if (! DXSetMember((Group)new, name, child))
			goto error;
		}
	    }
	    else
	    {
		float f;

		i = 0;
		while (NULL != (child = DXGetSeriesMember((Series)g, i, &f)))
		{
		    child = SortObject(child, flag);
		    if (! child)
			goto error;
		    
		    if (! DXSetSeriesMember((Series)new, i++, f, child))
			goto error;
		}
	    }

	    break;
	}

	case CLASS_ARRAY:
	{
	    int *indices = SortArray((Array)o, flag);
	    if (! indices)
		goto error;

	    new = (Object)ShuffleArray((Array)o, indices);
	    DXFree((Pointer)indices);

	    if (! new)
		goto error;
	    
	    break;
	}

	case CLASS_FIELD:
	{
	    char *name, *sort_dep, *str;
	    Field f = (Field)o;
	    Array dArray = NULL;
	    Array src, dst = NULL;
	    int i, n;

	    if (DXEmptyField(f))
		break;

	    dArray = (Array)DXGetComponentValue(f, "data");
	    if (! dArray)
	    {
		DXSetError(ERROR_MISSING_DATA, "no data component");
		goto error;
	    }

	    if (! DXGetStringAttribute((Object)dArray, "dep", &sort_dep))
	    {
		DXSetError(ERROR_MISSING_DATA, "no data dependency");
		goto error;
	    }

	    DXGetArrayInfo(dArray, &n, NULL, NULL, NULL, NULL);

	    indices = SortArray(dArray, flag);
	    if (! indices)
		goto error;
	    
	    map = MakeMap(indices, n);
	    if (! map)
		goto error;

	    new = DXCopy(o, COPY_HEADER);
	    if (! new)
		goto error;

	    i = 0;
	    while (NULL != 
		(src = (Array)DXGetEnumeratedComponentValue(f, i++, &name)))
	    {
		if (! strcmp(name, "connections"))
		    str = "connections";
		else if (! DXGetStringAttribute((Object)src, "dep", &str))
		    str = NULL;

		if (str && ! strcmp(str, sort_dep))
		{
		    dst = ShuffleArray(src, indices);
		    if (! dst)
			goto error;

		    if (!DXSetComponentValue((Field)new, name, (Object)dst))
			goto error;
		    
		    src = (Array)DXGetComponentValue((Field)new, name);
		}

		if (DXGetStringAttribute((Object)src, "ref", &str) &&
					    !strcmp(str, sort_dep))
		{
		    dst = ReMap(src, map);
		    if (! dst)
			goto error;

		    if (!DXSetComponentValue((Field)new, name, (Object)dst))
			goto error;
		}
	    }

	    DXFree((Pointer)indices);
	    DXFree((Pointer)map);

	    break;
	}

	case CLASS_XFORM:
	{
	    Object child;
	    Matrix matrix;

	    if (! DXGetXformInfo((Xform)o, &child, &matrix))
		goto error;
	    
	    child = SortObject(child, flag);
	    if (! child)
		goto error;
	    
	    new = (Object)DXNewXform(child, matrix);
	    if (! new)
	    {
		DXDelete(child);
		goto error;
	    }

	    break;
	}

	case CLASS_CLIPPED:
	{
	    Object child;
	    Object clipper;

	    if (! DXGetClippedInfo((Clipped)o, &child, &clipper))
		goto error;
	    
	    child = SortObject(child, flag);
	    if (! child)
		goto error;
	    
	    new = (Object)DXNewClipped(child, clipper);
	    if (! new)
	    {
		DXDelete(child);
		goto error;
	    }

	    break;
	}

	case CLASS_SCREEN:
	{
	    Object child;
	    int z, position;

	    if (! DXGetScreenInfo((Screen)o, &child, &position, &z))
		goto error;
	    
	    child = SortObject(child, flag);
	    if (! child)
		goto error;
	    
	    new = (Object)DXNewScreen(child, position, z);
	    if (! new)
	    {
		DXDelete(child);
		goto error;
	    }

	    break;
	}
        default:
	    DXSetError(ERROR_NOT_IMPLEMENTED, "Unimplemented Class");
	    goto error;
	    break;
    }

    return new;

error:
    DXFree((Pointer)indices);
    DXFree((Pointer)map);
    if (new && new != o)
	DXDelete((Object)new);
    return NULL;
}

#define LT(a,b) 	((a)->value < (b)->value)
#define GT(a,b) 	((a)->value > (b)->value)


typedef struct {int index; double value;} dval;
#define TYPE		dval
#define QUICKSORT 	SortDouble
#define QUICKSORT_LOCAL	SortDoublelocal
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; float value;} fval;
#define TYPE		fval
#define QUICKSORT 	SortFloat
#define QUICKSORT_LOCAL	SortFloat_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; int value;} ival;
#define TYPE		ival
#define QUICKSORT 	SortInt
#define QUICKSORT_LOCAL	SortInt_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; unsigned int value;} uival;
#define TYPE		uival
#define QUICKSORT 	SortUInt
#define QUICKSORT_LOCAL	SortUInt_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; short value;} sval;
#define TYPE		sval
#define QUICKSORT 	SortShort
#define QUICKSORT_LOCAL	SortShort_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; unsigned short value;} usval;
#define TYPE 		usval
#define QUICKSORT 	SortUShort
#define QUICKSORT_LOCAL	SortUShort_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; char value;} bval;
#define TYPE		bval
#define QUICKSORT 	SortByte
#define QUICKSORT_LOCAL	SortByte_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

typedef struct {int index; unsigned char value;} ubval;
#define TYPE 		ubval
#define QUICKSORT 	SortUByte
#define QUICKSORT_LOCAL	SortUByte_local
#include "../libdx/qsort.c"
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

#define SORTARRAY(type, stype,  sort)				\
{								\
    stype *list;						\
    int  i;							\
    type *data;							\
								\
    list = (stype *)DXAllocate(n*sizeof(stype));		\
    if (! list)							\
	goto error;						\
    								\
    data = (type *)DXGetArrayData(a);				\
    if (! data)							\
    {								\
	DXFree((Pointer)list);					\
	goto error;						\
    }								\
								\
    for (i = 0; i < n; i++)					\
    {								\
	list[i].index = i;					\
	list[i].value = data[i];				\
    }								\
								\
    sort(list, n);						\
								\
    indices = (int *)DXAllocate(n*sizeof(int));			\
    if (! indices)						\
    {								\
	DXFree((Pointer)list);					\
	goto error;						\
    }								\
     								\
    for (i = 0; i < n; i++)					\
	indices[i] = list[i].index;				\
    								\
    DXFree((Pointer)list);					\
}



static int *
SortArray(Array a, int flag)
{
    int      *indices = NULL;
    int      i, n, r, s[32];
    Type     t;
    Category c;

    if (DXGetObjectClass((Object)a) != CLASS_ARRAY)
	goto error;
    
    DXGetArrayInfo(a, &n, &t, &c, &r, s);

    if (r != 0 && (r != 1 || s[0] != 1))
    {
	DXSetError(ERROR_DATA_INVALID,
		"sort object must be scalar or 1-vector");
	goto error;
    }

    switch(t)
    {
	case TYPE_DOUBLE: SORTARRAY(double,         dval,  SortDouble); break;
	case TYPE_FLOAT:  SORTARRAY(float,          fval,  SortFloat);  break;
#if 0
	case TYPE_INT:    SORTARRAY(int,            ival,  SortInt);    break;
#else
	case TYPE_INT:
{								
    ival *list;						
    int  i;							
    int *data;							
								
    list = (ival *)DXAllocate(n*sizeof(ival));		
    if (! list)							
	goto error;						
    								
    data = (int *)DXGetArrayData(a);				
    if (! data)							
    {								
	DXFree((Pointer)list);					
	goto error;						
    }								
								
    for (i = 0; i < n; i++)					
    {								
	list[i].index = i;					
	list[i].value = data[i];				
    }								
								
    SortInt(list, n);						
								
    indices = (int *)DXAllocate(n*sizeof(int));			
    if (! indices)						
    {								
	DXFree((Pointer)list);					
	goto error;						
    }								
     								
    for (i = 0; i < n; i++)					
	indices[i] = list[i].index;				
    								
    DXFree((Pointer)list);					
}
break;
#endif
	case TYPE_UINT:   SORTARRAY(unsigned int,   uival, SortUInt);   break;
	case TYPE_SHORT:  SORTARRAY(short,          sval,  SortShort);  break;
	case TYPE_USHORT: SORTARRAY(unsigned short, usval, SortUShort); break;
	case TYPE_BYTE:   SORTARRAY(char,           bval,  SortByte);   break;
	case TYPE_UBYTE:  SORTARRAY(unsigned char,  ubval, SortUByte);  break;
	default: 
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"invalid data type in sort object");
	    goto error;
	}
    }

    if (flag == SORT_DESCENDING)
    {
	int m = n >> 1;
	n -= 1;

	for (i = 0; i < m; i++)
	{
	    int t = indices[i];
	    indices[i] = indices[n-i];
	    indices[n-i] = t;
	}
    }

    return indices;

error:
    DXFree((Pointer)indices);
    return NULL;
}

static Array
ShuffleArray(Array src, int *indices)
{
    Type t;
    Category c;
    Array dst = NULL;
    int n, r, s[32], size, i;
    unsigned char *sPtr, *dPtr;

    if (DXGetObjectClass((Object)src) != CLASS_ARRAY)
    {
	DXSetError(ERROR_BAD_CLASS, 
	     "component encountered that is not class ARRAY");
	goto error;
    }
	
    DXGetArrayInfo(src, &n, &t, &c, &r, s);

    size = DXGetItemSize(src);

    dst = DXNewArrayV(t, c, r, s);
    if (! dst)
	goto error;

    if (! DXAddArrayData(dst, 0, n, NULL))
	goto error;

    sPtr = (unsigned char *)DXGetArrayData(src);
    dPtr = (unsigned char *)DXGetArrayData(dst);

    if (! sPtr || ! dPtr)
	goto error;
    
    for (i = 0; i < n; i++)
    {
	memcpy(dPtr, sPtr+(indices[i]*size), size);
	dPtr += size;
    }

    return dst;

error:
    DXDelete((Object)dst);
    return NULL;
}

static int *
MakeMap(int *indices, int n)
{
    int i, *map = DXAllocate(n*sizeof(int));
    if (! map)
	goto error;
    
    for (i = 0; i < n; i++)
	map[indices[i]] = i;
    
    return map;

error:
    return NULL;
}
    
static Array
ReMap(Array src, int *map)
{
    Type t;
    Category c;
    Array dst = NULL;
    int n, nr, r, s[32], size, i;
    int *sPtr, *dPtr;

    if (DXGetObjectClass((Object)src) != CLASS_ARRAY)
    {
	DXSetError(ERROR_BAD_CLASS, 
	     "component encountered that is not class ARRAY");
	goto error;
    }
	
    DXGetArrayInfo(src, &n, &t, &c, &r, s);

    if (t != TYPE_INT)
    {
	DXSetError(ERROR_DATA_INVALID, "ref component must be type INTEGER");
	goto error;
    }

    size = DXGetItemSize(src);

    dst = DXNewArrayV(t, c, r, s);
    if (! dst)
	goto error;

    if (! DXAddArrayData(dst, 0, n, NULL))
	goto error;

    sPtr = (int *)DXGetArrayData(src);
    dPtr = (int *)DXGetArrayData(dst);

    if (! sPtr || ! dPtr)
	goto error;
    
    nr = n * size/sizeof(int);
    for (i = 0; i < nr; i++)
	*dPtr++ = map[*sPtr++];

    return dst;

error:
    DXDelete((Object)dst);
    return NULL;
}
