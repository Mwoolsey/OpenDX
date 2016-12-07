/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/dxmods/reduce.c,v 1.8 2006/01/03 17:02:24 davidt Exp $:
 */

/***
SECTION: Rendering
MODULE:
    Reduce
SHORTDESCRIPTION:
    creates a set of reduced-resolution versions of an input field
CATEGORY:
    Import and Export
INPUTS:
    input;                            field;              none; field to reduce
    factor;      scalar list or vector list; all powers of two; set of reduction factors
OUTPUTS:
    reduced;                 field or group;              NULL; set of reduced-resolution versions
FLAGS:
BUGS:
AUTHOR:
 Greg Abram
END:
***/

#include <stdio.h>
#include <math.h>
#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

#define MAX_DIMENSION	10
#define MAX_ELEMENTS	16
#define MAX_STACKBUF	15	/* 5x filter, 3-vector */

#define MAX_REDUCTION   32

#define TRUE  1
#define FALSE 0

#define NULL_DEPENDENCY		0
#define POSITIONS_DEPENDENT	1
#define CONNECTIONS_DEPENDENT	2

#define INVALID_POSITIONS	1
#define INVALID_CONNECTIONS	2

typedef struct  oMapElt
{
    int 	inOffset;
    int 	outOffset;
} OMapElt;

typedef struct originMap
{
    int	     	nDim;
    OMapElt 	**map;
} *OriginMap;

typedef struct
{
    int 	left, right;
} Shoulder;

       Error   m_Reduce(Object *, Object *);
static Object  Reduce(Object, int, float *, int);
static Object  ReduceObject(Object, Private, int);
static Object  ReduceGroup(Group, Private, int);
static Object  ReduceCField(CompositeField, Private, int);
static Object  ReduceSeries(Series, Private, int);
static Object  ReduceField(Field, Private, int, Private);
static Error   ReduceFieldTask(Pointer);
static Array   ReduceArrayReg(Array, int, Shoulder *, int *);
static Array   ReduceArrayIrreg(Array, int, int *, Shoulder *,
			    int *, int *, int *, int);
static Error   ReduceSumAxis(Pointer, Pointer, int, int, int,
			    int *, int *, int *, int *, int, float *, Type);
static Error   InvalidateData(Field, Field, int *, int);
static void    AxesOrder(int *, int *, int);
static Object  GrowObject(Object, float);

static Private ReducedMeshOffsets(CompositeField, Private);
static Error   DeleteOriginMapObject(Pointer);
static Error   DeleteFactorsObject(Pointer);

#define GetEM(g,i,n)	DXGetEnumeratedMember((g),(i),(n))
#define GetECV(g,i,n)	DXGetEnumeratedComponentValue((g),(i),(n))
#define GetECA(g,i,n,a)	DXGetEnumeratedComponentAttribute((g),(i),(n),(a))

Error
m_Reduce(Object *in, Object *out)
{
    Object   data;
    int      nFactors;
    float    *factors, maxFactor;
    int	     i;
    int	     nDim;
    Type     type;
    Category cat;
    int	     rank, shape[32];

    if ((data = in[0]) == NULL)
	DXErrorReturn(ERROR_MISSING_DATA, "data argument missing");
    
    
    if (in[1] == NULL)
    {
	nFactors = 1;
	nDim     = 1;
	factors  = NULL;
    }
    else
    {
	if (DXGetObjectClass(in[1]) != CLASS_ARRAY)
	    DXErrorReturn(ERROR_BAD_PARAMETER, "bad reduction specification");

	if (DXGetType(in[1], &type, &cat, &rank, shape))
	{
	    if ((type != TYPE_INT && type != TYPE_FLOAT)
		|| cat != CATEGORY_REAL 
		|| (rank != 0 && rank != 1))
		DXErrorReturn(ERROR_BAD_PARAMETER, 
			"illegal reduction specification");
	    
	    if (rank == 1)
		nDim = shape[0];
	    else
		nDim = 1;
	}
	else 
	    DXErrorReturn(ERROR_BAD_PARAMETER, "bad reduction specification");

	if (nDim > MAX_DIMENSION)
	    DXErrorReturn(ERROR_DATA_INVALID, "dimension too high");
	
	if (! DXGetArrayInfo((Array)in[1], &nFactors, NULL, NULL, NULL, NULL))
	    DXErrorReturn(ERROR_BAD_PARAMETER, 
		    "illegal reduction specification");

	if ((factors = (float *)DXAllocate(nDim*nFactors*sizeof(float))) == NULL)
	    DXErrorReturn(ERROR_NO_MEMORY, "error allocating factor array");

	if (!DXExtractParameter(in[1], TYPE_FLOAT, nDim, 
				    nFactors, (Pointer)factors))
	{
	    DXFree((Pointer)factors);
	    DXErrorReturn(ERROR_INTERNAL, "error extracting factor parameter");
	}
	
	maxFactor = 1.0;
	for (i = 0; i < nDim*nFactors; i++)
	    if (factors[i] < 1.0)
	    {
		DXSetError(ERROR_BAD_PARAMETER, 
			"reduction factors must be >= 1.0");
		DXFree((Pointer)factors);
		out[0] = NULL;
		return ERROR;
	    }
	    else if (factors[i] > maxFactor)
		maxFactor = factors[i];
    }
	
    out[0] = Reduce(data, nFactors, factors, nDim);

    DXFree((Pointer)factors);

    if (! out[0])
	return ERROR;
    else
	return OK;
}

static Object
Reduce(Object input, int nFactors, float *factorList, int nDim)
{
    int     i, j;
    float   *flist[MAX_DIMENSION], *factors, maxFactor;
    Object  template, result, reduced;
    Private plist[MAX_DIMENSION];
    int	    reduceFlags[MAX_DIMENSION];

    template = NULL;
    reduced  = NULL;
    result   = NULL;
    factors  = NULL;

    for (i = 0; i < nFactors; i++)
    {
	flist[i] = (float *)DXAllocate(MAX_DIMENSION * sizeof(float));
	if (! flist[i])
	{
	    for (--i; i >= 0; i--)
		DXDelete((Object)plist[i]);

	    return NULL;
	}

	plist[i] = DXNewPrivate((Pointer)flist[i], DeleteFactorsObject);
	if (! plist[i])
	{
	    for (--i; i >= 0; i--)
		DXDelete((Object)plist[i]);
	    
	    return NULL;
	}

	DXReference((Object)plist[i]);
    }

    if (factorList)
    {
	maxFactor = 1;

	if (nDim == 1)
	    for (i = 0; i < nFactors; i++)
	    {
		factors = flist[i];
		reduceFlags[i] = FALSE;

		if (factorList[i] <= 0) factorList[i] = 1.0;

		for (j = 0; j < MAX_DIMENSION; j++)
		    factors[j] = factorList[i];
	    
		if (maxFactor < factorList[i])
		    maxFactor = factorList[i];
		
		if (factorList[i] > 1.0)
		    reduceFlags[i] = TRUE;
	    }
	else
	    for (i = 0; i < nFactors; i++)
	    {
		factors = flist[i];
		reduceFlags[i] = FALSE;

		for (j = 0; j < nDim; j++)
		{
		    if (factorList[i*nDim+j] <= 0) factorList[i*nDim+j] = 1.0;

		    if ((factors[j] = factorList[i*nDim+j]) > 1.0)
			reduceFlags[i] = TRUE;
	    
		    if (maxFactor < factors[j])
			maxFactor = factors[j];
		}
	    }
    }
    else
    {
	for (i = 0; i < nFactors; i++)
	{
	    factors = flist[i];

	    for (j = 0; j < MAX_DIMENSION; j++)
		factors[j] = 2;
	    
	    /* 
	     * always reduce...
	     */
	    reduceFlags[i] = TRUE;
	}

	maxFactor = 1 << nFactors;
    }

    if (nFactors == 1 && maxFactor == 1.0)
    {
        for(i=0; i < nFactors; i++)
		DXDelete((Object)plist[i]);
	return input;
    }

    template = DXCopy(input, COPY_STRUCTURE);
    if (! template)
	goto error;

    if (!GrowObject(template, maxFactor))
	goto error;

    if (!template || DXGetError())
	goto error;

    DXCreateTaskGroup();

    if (nFactors == 1)
    {
	result = DXCopy(template, COPY_STRUCTURE);
	if (! result)
	{
	    DXAbortTaskGroup();
	    goto error;
	}

	if (!ReduceObject(result, plist[0], nDim))
	{
	    DXAbortTaskGroup();
	    goto error;
	}

    }
    else
    {
	result = (Object)DXNewGroup();
	if (! result)
	    goto error;

	for (i = 0; i < nFactors; i++)
	{
	    if (reduceFlags[i])
	    {
		reduced = DXCopy(template, COPY_STRUCTURE);
		if (! reduced)
		{
		    DXAbortTaskGroup();
		    goto error;
		}

		if (!ReduceObject(reduced, plist[i], nDim))
		{
		    DXAbortTaskGroup();
		    goto error;
		}
	    }
	    else
		reduced = input;

	    if (! DXSetMember((Group)result, NULL, (Object)reduced)) 
	    {
		DXDelete((Object)result);
		DXDelete((Object)template);
		return NULL;
	    }

	    reduced = NULL;
	}

	if (i == 0)
	    goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError())
	goto error;

    if (template != input)
	DXDelete((Object)template);

    for (i = 0; i < nFactors; i++)
	DXDelete((Object)plist[i]);

    return result;

error:
    for (i = 0; i < nFactors; i++)
	DXDelete((Object)plist[i]);

    if (template != input)
	DXDelete(template);

    DXDelete(result);
    DXDelete(reduced);

    return NULL;
}

static Object
GrowObject(Object object, float maxFactor)
{
    Class  class;
    Object child;
    int    i, j;
    char   *name, *components[64];
    Field  field, field0;
    int    overlap;
    Array  array;
    float  pos;

    overlap = (((int)ceil(maxFactor)) | 0x1) >> 1;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);
    
    switch(class)
    {
	case CLASS_XFORM:

	    if (! DXGetXformInfo((Xform)object, &child, 0))
		return NULL;
	    
	    if (! GrowObject(child, maxFactor))
		return NULL;

	    break;

	case CLASS_SERIES:
	    i = 0;
	    while (NULL != (child=DXGetSeriesMember((Series)object, i, &pos)))
	    {
		child = GrowObject(child, maxFactor);
		if (! child || DXGetError())
		    return NULL;

		if (! DXSetSeriesMember((Series)object, i, pos, child))
		    return NULL;
		
		i++;
	    }

	    break;

	case CLASS_MULTIGRID:
	case CLASS_GROUP:
	     
	    i = 0;
	    while (NULL != (child = GetEM((Group)object, i, &name)))
	    {
		child = GrowObject(child, maxFactor);
		if (! child || DXGetError())
		    return NULL;

		if (! DXSetEnumeratedMember((Group)object, i, child))
		    return NULL;
		
		i++;
	    }

	    break;
	
	case CLASS_COMPOSITEFIELD:

	    field0 = NULL;
	    for (i = 0; NULL != (field = DXGetPart(object, i)); i++)
	    {
		if (DXGetObjectClass((Object)field) != CLASS_FIELD)
		{
		    DXSetError(ERROR_DATA_INVALID,
			"invalid member of composite field");
		    return NULL;
		}

		if (DXEmptyField(field) ||
			!DXGetComponentValue(field, "connections"))
		    continue;

		array = (Array)DXGetComponentValue(field, "connections");
		if (! array)
		{
		    DXSetError(ERROR_MISSING_DATA, "missing connections");
		    return NULL;
		}

		if (! DXQueryGridConnections(array, NULL, NULL))
		{
		    DXSetError(ERROR_DATA_INVALID, "irregular connections");
		    return NULL;
		}

		field0 = field;
	    }

	    if (field0 == NULL)
		return object;

	    i = j = 0;
	    while (NULL != GetECV(field0, i++, &components[j]))
		if (strcmp(components[j], "statistics") &&
		    strcmp(components[j], "neighbors") &&
		    strcmp(components[j], "positions") &&
		    strcmp(components[j], "connections") &&
		    strcmp(components[j], "box"))
		    j++;
	    
	    components[j] = NULL;
	    
	    if (! DXGrowV(object, overlap, NULL, components))
		return NULL;
	    
	    break;

	case CLASS_FIELD:
	    
	    if (DXEmptyField((Field)object) ||
			!DXGetComponentValue((Field)object, "connections"))
		break;

	    i = j = 0;
	    while (NULL != GetECV((Field)object, i++, &components[j]))
		if (strcmp(components[j], "statistics") &&
		    strcmp(components[j], "neighbors") &&
		    strcmp(components[j], "positions") &&
		    strcmp(components[j], "connections") &&
		    strcmp(components[j], "box"))
		    j++;
	    
	    components[j] = NULL;
	    
	    if (! DXGrowV(object, overlap, NULL, components))
		return NULL;
	    
	    break;
	
	default:

	    DXSetError(ERROR_DATA_INVALID, 
	    	"unsupported object class encountered");
	    return NULL;
    }

    return object;
}

static Object
ReduceObject(Object object, Private factors, int nDim)
{
    float  *f;
    Class  class;
    Object result;
    Object child;
    int    i;

    f = (float *)DXGetPrivateData(factors);

    for (i = 0; i < nDim; i++)
	if (f[i] > 1.0)
	    goto reduce;
    
    return object;

reduce:

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);

    switch (class)
    {
	case CLASS_XFORM:
	    if (! DXGetXformInfo((Xform)object, &child, 0))
		return NULL;
	    
	    result = ReduceObject(child, factors, nDim);
	    break;

	case CLASS_FIELD: 
	    result = ReduceField((Field)object, factors, nDim, NULL);
	    break;
	
	case CLASS_COMPOSITEFIELD:
	    result = ReduceCField((CompositeField)object, factors, nDim);
	    break;
	
	case CLASS_SERIES:
	    result = ReduceSeries((Series)object, factors, nDim);
	    break;
	
	case CLASS_MULTIGRID:
	case CLASS_GROUP:
	    result = ReduceGroup((Group)object, factors, nDim);
	    break;
	
	default:
	    DXSetError(ERROR_DATA_INVALID, "invalid data object");
	    result = NULL;
	    break;
    }

    return result;
}

static Object
ReduceGroup(Group group, Private factors, int nDim)
{
    int i;
    Object child;

    i = 0;
    while ((child = DXGetEnumeratedMember(group, i++, NULL)) != NULL)
	if (! ReduceObject(child, factors, nDim))
	    return NULL;

    return (Object)group;
}

static Object
ReduceCField(CompositeField cfield, Private factors, int nDim)
{
    int i;
    Object child;
    Private omap;

    omap = ReducedMeshOffsets(cfield, factors);
    if (! omap)
	return NULL;
    
    i = 0;
    while ((child = DXGetEnumeratedMember((Group)cfield, i++, NULL)) != NULL)
    {
	if (DXGetObjectClass((Object)child) != CLASS_FIELD)
	{
	    DXSetError(ERROR_DATA_INVALID, "invalid member of composite field");
	    return NULL;
	}

	if (! ReduceField((Field)child, factors, nDim, omap))
	    return NULL;
    }
    
    return (Object)cfield;
}

static Object
ReduceSeries(Series series, Private factors, int nDim)
{
    int i;
    Object child;

    i = 0;
    while ((child = DXGetSeriesMember((Series)series, i++, NULL)) != NULL)
	if (! ReduceObject(child, factors, nDim))
	    return NULL;

    return (Object)series;
}

typedef struct 
{
    Field   field;
    int	    nDim;
    Private omap;
    Private factors;
} FieldTask;
    
static Object
ReduceField(Field field, Private factors, int nDim, Private omap)
{
    FieldTask task;

    if (DXEmptyField(field) || !DXGetComponentValue(field, "connections"))
	return (Object)field;

    task.field    = field;
    task.nDim     = nDim;
    task.factors  = (Private)DXReference((Object)factors);

    if (omap)
	task.omap = (Private)DXReference((Object)omap);
    else
	task.omap = NULL;

    if (! DXAddTask(ReduceFieldTask, (Pointer)&task, sizeof(task), 1.0))
	return NULL;
    else
	return (Object)field;
}

static Error
ReduceFieldTask(Pointer ptr)
{
    FieldTask	*task;
    Field       oField, rField;
    float	*inFactors;
    int		nDim;
    OriginMap   omap;
    OMapElt     **maps;
    Array	inCons, outCons, comp;
    int		nDimCons;
    int		inCCounts[MAX_DIMENSION], origCCounts[MAX_DIMENSION];
    int		outCCounts[MAX_DIMENSION];
    int		origMeshOffsets[MAX_DIMENSION];
    int		reducedMeshOffsets[MAX_DIMENSION];
    int		meshOffsets[MAX_DIMENSION];
    int		i, j;
    float	factors[MAX_DIMENSION];
    int		axes[MAX_DIMENSION];
    int		filter[MAX_DIMENSION];
    char 	*name, *str;
    Object	attr;
    Shoulder	shoulder[MAX_DIMENSION];
    int		dep;

    task = (FieldTask *)ptr;

    rField 	= task->field;
    nDim  	= task->nDim;
    inFactors 	= (float *)DXGetPrivateData(task->factors);

    if (task->omap)
    {
	omap = (OriginMap)DXGetPrivateData(task->omap);
	maps = omap->map;
    }
    else 
    {
	omap = NULL;
	maps = NULL;
    }
    
    DXChangedComponentValues(rField, "data");

    if (NULL == (inCons = (Array)DXGetComponentValue(rField, "connections")))
    {
	DXSetError(ERROR_MISSING_DATA, "connections missing");
	return ERROR;
    }

    if (NULL == DXQueryGridConnections(inCons, &nDimCons, inCCounts))
    {
	DXSetError(ERROR_DATA_INVALID, "field connections irregular");
	return ERROR;
    }
	
    if (nDim != 1 && nDim != nDimCons)
    {
	DXSetError(ERROR_DATA_INVALID, "dimension disagreement");
	return ERROR;
    }

    if (! DXGetMeshOffsets((MeshArray)inCons, meshOffsets))
    {
	DXSetError(ERROR_DATA_INVALID, "field connections irregular");
	return ERROR;
    }

    if (! DXQueryOriginalMeshExtents(rField, origMeshOffsets, origCCounts))
    {
	for (i = 0; i < nDimCons; i++)
	{
	    shoulder[i].left = shoulder[i].right = 0;
	}
    }
    else
    {
	for (i = 0; i < nDimCons; i++)
	{
	    meshOffsets[i] += origMeshOffsets[i];

	    shoulder[i].left  = origMeshOffsets[i];
	    shoulder[i].right = (inCCounts[i]-origCCounts[i])-shoulder[i].left;

	    inCCounts[i] = origCCounts[i];
	}
    }

    if (nDim != 1)
	for (i = 0; i < nDimCons; i++)
	    if (inFactors[i] != 0)
		factors[i] = inFactors[i];
	    else
		factors[i] = 1.0;
    else
	for (i = 0; i < nDimCons; i++)
	    if (inFactors[0] != 0)
		factors[i] = inFactors[0];
	    else
		factors[i] = 1.0;

    for (i = 0; i < nDimCons; i++)
    {
	if (inCCounts[i] < 1)
	{
	    DXSetError(ERROR_DATA_INVALID, "<1 poins along %d axis", i);
	    return ERROR;
	}

	if (factors[i] == 1 || inCCounts[i] == 1)
	{
	    outCCounts[i] = inCCounts[i];
	    filter[i] = 1;
	}
	else
	{
	    outCCounts[i] = (inCCounts[i] + 1) / factors[i];

	    if (outCCounts[i] < 2)
		outCCounts[i] = 2;

	    filter[i] = ((int)ceil(factors[i])) | 0x01;
	}
    }

    /*
     * DXCopy input field for use as a template.  Original field gets 
     * reduced in place.
     */
    oField = (Field)DXCopy((Object)rField, COPY_STRUCTURE);
    if (! oField)
	return ERROR;

    /*
     * Make regular output connections
     */
    outCons = DXMakeGridConnectionsV(nDimCons, outCCounts);
    if (! outCons)
	goto error;

    if (! DXSetComponentValue(rField, "connections", (Object)outCons))
	goto error;
    
    if (maps)
    {
	for (i = 0; i < nDimCons; i++)
	{
	    for (j = 0; maps[i][j].inOffset != -1; j++)
		if (maps[i][j].inOffset == meshOffsets[i])
		{
		    reducedMeshOffsets[i] =  maps[i][j].outOffset;
		    break;
		}

	    if (maps[i][j].inOffset == -1)
	    {
		DXSetError(ERROR_INTERNAL, "mesh offset error");
		goto error;
	    }
	}

	if (! DXSetMeshOffsets((MeshArray)outCons, reducedMeshOffsets))
	{
	    DXSetError(ERROR_INTERNAL, "mesh offset error");
	    goto error;
	}

    }

    /*
     * Re-order axes for optimal reduction in irregular case
     */
    AxesOrder(filter, axes, nDimCons);

    /*
     * Now reduce each other component that deps on positions
     */
    i = -1;
    while ((comp=(Array)GetECV(oField, ++i, &name)) != NULL)
    {
	if (! strncmp(name, "original", 8))
	{
	    DXDeleteComponent(rField, name);
	    continue;
	}

	if (! strncmp(name, "invalid ", 8) ||
	    ! strcmp(name, "connections"))
	    continue;

	attr = GetECA(oField, i, NULL, "dep");
	if (! attr)
	    continue;

	str = DXGetString((String)attr);
	if (! str)
	    goto error;

	if (! strcmp(str, "positions"))
	    dep = POSITIONS_DEPENDENT;
	else if (! strcmp(str, "connections"))
	    dep = CONNECTIONS_DEPENDENT;
	else
	    continue;

	if (DXQueryGridPositions(comp, NULL, NULL, NULL, NULL))
	    comp = ReduceArrayReg(comp, nDimCons, shoulder, outCCounts);
	else
	{
	    if (! strcmp(name, "positions"))
		comp = ReduceArrayIrreg(comp, nDimCons, NULL,
				    shoulder, inCCounts, outCCounts, axes, dep);
	    else
		comp = ReduceArrayIrreg(comp, nDimCons, filter,
				    shoulder, inCCounts, outCCounts, axes, dep);
	}

	if (! comp)
	    goto error;

	if (! DXSetComponentValue(rField, name, (Object)comp))
	    goto error;
    }

    if (DXGetComponentValue(oField, "invalid positions"))
	if (! InvalidateData(oField, rField, filter, INVALID_POSITIONS))
	    goto error;
	
    if (DXGetComponentValue(oField, "invalid connections"))
	if (! InvalidateData(oField, rField, filter, INVALID_CONNECTIONS))
	    goto error;
	

    DXDelete((Object)oField);

    DXDeleteComponent(rField, "box");
    DXEndField(rField);

    DXDelete((Object)task->factors);
    DXDelete((Object)task->omap);

    return OK;

error:

    DXDelete((Object)task->factors);
    DXDelete((Object)task->omap);
    DXDelete((Object)oField);
    return ERROR;
}

static Array
ReduceArrayReg(Array inArray, int nDimGrid,
				    Shoulder *shoulder, int *outCountsGrid)
{
    int		i, j, nDimData, nextAxis;
    int		inCountsData[MAX_DIMENSION], outCountsData[MAX_DIMENSION];
    float	origins[MAX_DIMENSION];
    float	inDeltas[MAX_DIMENSION], outDeltas[MAX_DIMENSION];
    float	inMax, outMax;
    Array	outArray;
    float	*odPtr, *idPtr;
    int		*ocPtr, *icPtr;

    DXQueryGridPositions(inArray, &nDimData, inCountsData, origins, inDeltas);

    nextAxis = 0;
    for (i = 0; i < nDimData; i++)
    {
	if (inCountsData[i] != 1)
	{
	    outCountsData[i] = outCountsGrid[nextAxis];

	    for (j = 0; j < nDimData; j++)
		origins[j] += shoulder[nextAxis].left * inDeltas[i*nDimData+j];

	    inCountsData[i] -= 
		(shoulder[nextAxis].left + shoulder[nextAxis].right);

	    nextAxis ++;
	}
	else
	    outCountsData[i] = 1;
    }

    if (nextAxis != nDimGrid)
    {
	DXSetError(ERROR_DATA_INVALID, "regular data dimensions mismatch grid");
	return NULL;
    }

    odPtr = outDeltas;
    idPtr = inDeltas;
    ocPtr = outCountsData;
    icPtr = inCountsData;
    for (i = 0; i < nDimData; i++)
    {
	/* 
	 * Make sure we do not reduce to less than two samples along 
	 * each axis.  
	 */

	for (j = 0; j < nDimData; j++)
	{
	    if (*ocPtr > 1)
	    {
		*odPtr = ((double)((*icPtr) - 1) * (double)(*idPtr))
				/ (double)((*ocPtr) - 1);
	
		/*
		 * A kludge to ensure that the output grid lies entirely inside 
		 * the input grid.
		 */
		inMax = (*idPtr) * ((*icPtr) - 1);
		if (inMax < 0.0) inMax = -inMax;

		outMax = (*odPtr) * ((*ocPtr) - 1);
		if (outMax < 0.0) outMax = -outMax;
		
		while(outMax > inMax)
		{
		    *odPtr *= 0.9999999;
		    outMax = (*odPtr) * ((*ocPtr) - 1);
		    if (outMax < 0.0) outMax = -outMax;
		}
	    }
	    else
		*odPtr = 0.0;

	    idPtr++;
	    odPtr++;
	}

	icPtr++;
	ocPtr++;
    }

    outArray = DXMakeGridPositionsV(nDimData, outCountsData, origins, outDeltas);
    return outArray;
}

typedef struct
{
    byte  		*inStart;
    byte  		*outStart;
    int    		inSkip;
    int    		outSkip;
    int    		increment;
    int    		limit;
} FilterLoopCntl;

static Array
ReduceArrayIrreg(Array inDataArray, int nDim, int *filter, Shoulder *shoulders,
			    int *inCounts, int *outCounts, int *axes, int dep)
{
    Array	outDataArray;
    int 	axis, a;
    int		i, nElts;
    int		tmpOutCounts[MAX_DIMENSION], tmpInCounts[MAX_DIMENSION];
    int		tmpLeft[MAX_DIMENSION], tmpRight[MAX_DIMENSION];
    Pointer	buf0, buf1, inBuf;
    Type	type;
    Category	cat;
    int		rank, shape[MAX_DIMENSION];
    int		nItems, itemSize, bufSize;
    int		status;
    int		outItems;
    float	strides[MAX_DIMENSION];
    int		filterLen;
    int		outDataCounts[MAX_DIMENSION], inDataCounts[MAX_DIMENSION];

    if (dep == POSITIONS_DEPENDENT)
	for (i = 0; i < nDim; i++)
	{
	    outDataCounts[i] = outCounts[i];
	    inDataCounts[i]  = inCounts[i];
	}
    else
	for (i = 0; i < nDim; i++)
	{
	    outDataCounts[i] = outCounts[i] - 1;
	    inDataCounts[i]  = inCounts[i] - 1;
	}

    outItems = 1;
    for (i = 0; i < nDim; i++)
	outItems *= outDataCounts[i];
    
    if (!DXGetArrayInfo(inDataArray, &nItems, &type, &cat, &rank, shape))
	return NULL;

    if (cat != CATEGORY_REAL || nItems <= 0)
    {
	DXSetError(ERROR_DATA_INVALID, "bad data component");
	return NULL;
    }

    if (outItems == nItems)
	return inDataArray;

    itemSize = DXGetItemSize(inDataArray);
    nElts = itemSize / DXTypeSize(type);
    bufSize = itemSize * nItems;

    inBuf = DXGetArrayData(inDataArray);

    buf0 = inBuf;
    buf1 = NULL;

    for (i = 0; i < nDim; i++)
    {
	tmpInCounts[i] = tmpOutCounts[i] = 
		inDataCounts[i] + shoulders[i].left + shoulders[i].right;

	if (outDataCounts[i] > 1)
	    strides[i] = (inDataCounts[i]-1.0)/(outDataCounts[i]-1.0);
	else
	    strides[i] = -1;

	tmpLeft[i] = shoulders[i].left;
	tmpRight[i] = shoulders[i].right;
    }

    for (a = 0; a < nDim; a++)
    {
	axis = axes[a];

	tmpOutCounts[axis] = outDataCounts[axis];

	if (tmpInCounts[axis] != tmpOutCounts[axis])
	{
	    if (! filter || filter[axis] <= 1)
		filterLen = 1;
	    else
		filterLen = filter[axis];

	    bufSize = nElts * itemSize;
	    for (i = 0; i < nDim; i++)
		bufSize *= tmpOutCounts[i];
	    
	    buf1 = DXAllocate(bufSize);
	    if (! buf1)
	    {
		if (buf0 != inBuf)
		    DXFree(buf0);
		return NULL;
	    }

	    status = ReduceSumAxis(buf0, buf1, filterLen, nDim, nElts,
				tmpInCounts, tmpOutCounts, tmpLeft,
				tmpRight, axis, strides, type);

	    if (status != OK)
	    {
		if (buf0 != inBuf) DXFree(buf0);
		DXFree(buf1);
		return NULL;
	    }

	    if (buf0 != inBuf) DXFree(buf0);
	    buf0 = buf1;
	    buf1 = NULL;
	}

	tmpInCounts[axis] = outDataCounts[axis];
	tmpLeft[axis] = tmpRight[axis] = 0;
    }
    
    if (! (outDataArray = DXNewArrayV(type, cat, rank, shape)))
	return NULL;

    if (! DXAddArrayData(outDataArray, 0, outItems, buf0))
    {
	DXDelete((Object)outDataArray);
	return NULL;
    }

    DXFree(buf0);
    
    return outDataArray;
}

#define AXISLOOP(type)							\
{									\
    while(1)								\
    {									\
	byte    *outPtr, *inPtr;					\
	float   *sumElt;						\
	float   *lastSumElt;						\
	type    *outElt;						\
	type    *inElt;							\
	float   *bufElt, *bufPtr=NULL;					\
									\
	outPtr = loop[0].outStart;					\
	inPtr  = loop[0].inStart + startSkip;				\
							    		\
	/*								\
	 * Initialize accumulator and buffer				\
	 */								\
	memset(sumPtr, 0, nElts*sizeof(float));				\
	memset(buf, 0, filter*nElts*sizeof(float));			\
									\
	bufI = filter;							\
	stride = filter;						\
									\
	for (i = 0; i < loopLength; i++)				\
	{								\
	    /*								\
	     * Handle wrapping buf					\
	     */								\
	    if (++bufI >= filter)					\
	    {								\
		bufPtr = (float *)buf;					\
		bufI = 0;						\
	    }								\
									\
	    sumElt     = (float *)sumPtr;				\
	    lastSumElt = (float *)lastSumPtr;				\
	    inElt      = (type *)inPtr;					\
	    bufElt     = bufPtr;					\
									\
	    for (j = 0; j < nElts; j++)					\
	    {								\
		*lastSumElt  = *sumElt;					\
		*sumElt     += (((float)*inElt) - *bufElt);		\
		*bufElt      = *inElt;					\
		lastSumElt++;						\
		sumElt++;						\
		inElt++;						\
		bufElt++;						\
	    }								\
									\
	    /*								\
	     * We may have to interpolate an output sample		\
	     */								\
	    if (stride <= 1.0 && i != (loopLength-1))			\
	    {								\
		/*							\
		 * Pointers to interpolate				\
		 */							\
		outElt     = (type *)outPtr;				\
		sumElt     = (float *)sumPtr;				\
		lastSumElt = (float *)lastSumPtr;			\
									\
		if (stride == 1.0)					\
		    for (j = 0; j < nElts; j++)				\
			*outElt++ = (type)				\
			  (divisor * (*sumElt++) + round);		\
		else if (stride == 0.0)					\
		    for (j = 0; j < nElts; j++)				\
			*outElt++ = (type)				\
			  (divisor * (*lastSumElt++) + round);  	\
		else							\
		    for (j = 0; j < nElts; j++)				\
		    {							\
			*outElt++ = (type)				\
(divisor * (*lastSumElt + stride*(*sumElt-*lastSumElt)) + round);	\
			lastSumElt ++;					\
			sumElt ++;					\
		    }							\
									\
		stride += strideLength;					\
		outPtr += outAxisSkip;					\
	    }								\
									\
	    if (strideLength > 0)					\
		stride -= 1.0;						\
									\
	    if (i >= incStart && i < incEnd)				\
		inPtr += inAxisSkip;					\
									\
	    bufPtr += nElts;						\
	}								\
									\
	/* 								\
	 * For the last value along the axis. sum up the buffer		\
	 */								\
	if (++bufI >= filter)						\
	    bufI = 0;							\
									\
	bufPtr = buf + nElts*bufI;					\
									\
	sumElt = sumPtr;						\
	bufElt = bufPtr;						\
	for (i = 0; i < nElts; i++)					\
	    *sumElt++ = *bufElt++;					\
									\
	for (i = 1; i < filter; i++)					\
	{								\
	    if (++bufI >= filter)					\
		bufI = 0;						\
									\
	    bufPtr = buf + nElts*bufI;					\
									\
	    bufElt = bufPtr;						\
	    sumElt = sumPtr;						\
	    for (j = 0; j < nElts; j++)					\
		*sumElt++ += *bufElt++;					\
	}								\
									\
	sumElt = sumPtr;						\
	outElt = (type *)outPtr;					\
	for (i = 0; i < nElts; i++)					\
	    *outElt++ = (type)(divisor * *sumElt++ + round);		\
	    								\
	if (nDim == 1)							\
	    break;							\
									\
	/*								\
	 * Bump loops inner-to-outer.  Terminate			\
	 * either when out of loops or find a un-finished loop.		\
	 */								\
	l = loop + 1;							\
	for (i = 1; i < nDim; i++)					\
	{								\
	    if (l->increment < l->limit)				\
	    {								\
		l->increment++;						\
		break;							\
	    }								\
									\
	    l++;							\
	}								\
									\
	/*								\
	 * If all loops complete, continue loop that iterates		\
	 * the axis currently being filtered.  Break loop that		\
	 * handles outer loop.						\
	 */								\
									\
	if (i == nDim)							\
	    break;							\
									\
	/*								\
	 * Otherwise, bump the incompleted loop.			\
	 */								\
	l->inStart  += l->inSkip;					\
	l->outStart += l->outSkip;					\
									\
	/*								\
	 * Now re-initialize lower axis loops.				\
	 */								\
	for (j = 0; j < i; j++)						\
	{								\
	    loop[j].increment = 0;					\
	    loop[j].inStart   = l->inStart;				\
	    loop[j].outStart  = l->outStart;				\
	}								\
									\
	/*								\
	 * And continue the outer loop.					\
	 */								\
    }									\
}
	
static Error
ReduceSumAxis(Pointer inBuf, Pointer outBuf, int filter, int nDim, int nElts,
    int *inCounts, int *outCounts, int *leftAxis, int *rightAxis, int axis,
    float *strides, Type type)
{
    int			inSkip[MAX_DIMENSION];
    int			outSkip[MAX_DIMENSION];
    int			overhang;
    int			i, j;
    FilterLoopCntl	loop[MAX_DIMENSION], *l;
    int		        inAxisSkip, outAxisSkip;
    float		*buf, *sumPtr, *lastSumPtr;
    int		   	bufI;
    float		strideLength, stride;
    int			left, right;
    int			incStart, incEnd, startSkip, loopLength;
    int			typeSize;
    float		divisor = 1.0 / filter;
    float		round;

    buf = sumPtr = lastSumPtr = NULL;

    typeSize = DXTypeSize(type);

    strideLength = strides[axis];
    left  = leftAxis[axis];
    right = rightAxis[axis];

    outSkip[nDim-1] = inSkip[nDim-1] = nElts * typeSize;
    for (i = (nDim-2); i >= 0; i--)
    {
	inSkip[i]  = inSkip[i+1]  *  inCounts[i+1];
	outSkip[i] = outSkip[i+1] * outCounts[i+1];
    }

    /*
     * Make sure that the filter is odd in length
     */
    filter |= 0x01;

    /*
     * Here's the setup for looping in each non-filter direction:
     */
    l = loop;

    l->limit     = inCounts[axis] - 1;
    l->inSkip    = inSkip[axis];
    l->outSkip   = outSkip[axis];
    l->inStart   = (byte *)inBuf;
    l->outStart  = (byte *)outBuf;
    l->increment = 0;

    l++;

    for (i = 0; i < nDim; i++)
	if (i != axis)
	{
	    l->limit     = inCounts[i] - 1;
	    l->inSkip    = inSkip[i];
	    l->outSkip   = outSkip[i];
	    l->inStart   = (byte *)inBuf;
	    l->outStart  = (byte *)outBuf;
	    l->increment = 0;
	    l++;
	}
    
    /*
     * Get buffer and accumulator that will span the
     * box filter in the current filter axis.
     */
    buf = (float *)DXAllocateLocal(filter*nElts*sizeof(float));
    if (! buf)
    {
	DXResetError();
	buf = (float *)DXAllocate(filter*nElts*sizeof(float));
    }
    if (! buf)
	goto error;
    
    sumPtr = (float *)DXAllocateLocal(nElts*sizeof(float));
    if (! sumPtr)
    {
	DXResetError();
	sumPtr = (float *)DXAllocate(nElts*sizeof(float));
    }
    if (! sumPtr)
	goto error;

    lastSumPtr = (float *)DXAllocateLocal(nElts*sizeof(float));
    if (! lastSumPtr)
    {
	DXResetError();
	lastSumPtr = (float *)DXAllocate(nElts*sizeof(float));
    }
    if (! lastSumPtr)
	goto error;

    /*limit        = loop[0].limit;*/
    inAxisSkip   = loop[0].inSkip;
    outAxisSkip  = loop[0].outSkip;

    overhang    = (filter - 1) >> 1;

    /*
     * Basic loop operates on the total number of samples accumulated
     * to produce the output line.  This is the number of input samples
     * minus extra added for shoulders plus the two overhangs.
     */
    loopLength  = (inCounts[axis] - (left + right)) + (overhang + overhang);

    /*
     * If the left shoulder is wider than the overhang, we will need to skip
     * unused samples.  Since there is at least overhang shoulder samples,
     * we start bumping the input pointer immediately.  Otherwise, we start 
     * operating on the first sample.  In this case, we begin bumping the
     * pointer when we have replicated (overhang - left) samples.
     */
    if (left > overhang)
    {
	startSkip = (left - overhang) * inAxisSkip;
	incStart  = 0;
    }
    else
    {
	startSkip = 0;
	incStart  = overhang - left;
    }

    /*
     * If the right shoulder is shorter than the overhang, we cease
     * bumping the input pointer when we have exhausted the shoulder.
     */
    if (right < overhang)
    {
	incEnd = (loopLength - 1) - (overhang - right);
    }
    else
    {
	incEnd = loopLength - 1;
    }

    switch(type)
    {
	case TYPE_BYTE:
	    round = 0.5;
	    AXISLOOP(byte);
	    break;

	case TYPE_UBYTE:
	    round = 0.5;
	    AXISLOOP(ubyte);
	    break;

	case TYPE_SHORT:
	    round = 0.5;
	    AXISLOOP(short);
	    break;

	case TYPE_USHORT:
	    round = 0.5;
	    AXISLOOP(ushort);
	    break;

	case TYPE_INT:
	    round = 0.5;
	    AXISLOOP(int);
	    break;

	case TYPE_UINT:
	    round = 0.5;
	    AXISLOOP(uint);
	    break;

	case TYPE_FLOAT:
	    round = 0.0;
	    AXISLOOP(float);
	    break;

	case TYPE_DOUBLE:
	    round = 0.0;
	    AXISLOOP(double);
	    break;
        default:
	    break;
    }

    DXFree((Pointer)buf);
    DXFree((Pointer)sumPtr);
    DXFree((Pointer)lastSumPtr);

    return OK;

error:
    DXFree((Pointer)sumPtr);
    DXFree((Pointer)lastSumPtr);
    DXFree((Pointer)buf);

    return ERROR;
}

static void
AxesOrder(int *filter, int *axes, int nDim)
{
    int flags[MAX_DIMENSION];
    int max, i, j;

    memset(flags, 1, nDim*sizeof(int));

    for (i = 0; i < nDim; i++)
    {
	max = -1;

	for (j = 0; j < nDim; j++)
	    if (flags[j] && max < filter[j])
	    {
		axes[i] = j;
		max     = filter[j];
	    }
	
	flags[axes[i]] = 0;
    }
}

typedef struct node
{
    struct node *next;
    int	         offset;
} Node;

#define MAX_NODES	2048

static Private
ReducedMeshOffsets(CompositeField cfield, Private pfactors)
{
    Field	part;
    Node	lists[MAX_DIMENSION];
    int		axisCounts[MAX_DIMENSION];
    Node	nodeStash[MAX_NODES];
    int		growthOffsets[MAX_DIMENSION];
    int		meshOffsets[MAX_DIMENSION];
    int		nextn, nDim;
    int		i, j, k, l;
    Node	*n, *last, *next;
    Array	conn;
    int		first, offset;
    OriginMap	omap;
    OMapElt     **maps;
    Private	omapObject;
    float	*factors;

    omap = NULL;

    if (! pfactors)
	return NULL;
    
    factors = (float *)DXGetPrivateData(pfactors);
    if (! factors)
	return NULL;


    first = 1;
    nextn = 0;

    /*
     * First, look through the partitions looking for those that lie
     * along the axes.
     */
    i = 0;
    while(NULL != (part = (Field)DXGetPart((Object)cfield, i++)))
    {
	conn = (Array)DXGetComponentValue(part, "connections");
	if (! conn)
	{
	    DXSetError(ERROR_MISSING_DATA, "partition with no connections found");
	    return NULL;
	}
		
	if (first)
	{
	    if (! DXQueryGridConnections(conn, &nDim, NULL))
	    {
		DXSetError(ERROR_DATA_INVALID, "irregular connections found");
		return NULL;
	    }

	    /*
	     * Set up empty linked lists for the insertion of partition 
	     * origins.
	     */
	    for (j = 0; j < nDim; j++)
	    {
		lists[j].next = NULL;
		lists[j].offset = 0;
		axisCounts[j] = 1;	/* for 0 slot */
	    }

	    first = 0;
	}

	if (! DXGetMeshOffsets((MeshArray)conn, meshOffsets))
	    return NULL;
	
	if (DXQueryOriginalMeshExtents(part, growthOffsets, NULL))
	    for (j = 0; j < nDim; j++)
		meshOffsets[j] += growthOffsets[j];

	for (j = 0; j < nDim; j++)
	{
	    offset = meshOffsets[j];
	    if (offset == 0)
		continue;

	    last = &lists[j];
	    next = last->next;
	    while (next != NULL)
	    {
		if (next->offset >= offset)
		    break;
		last = next;
		next = next->next;
	    }

	    if (next && next->offset == offset)
		continue;
	
	    n = nodeStash + nextn++;
	    if (nextn >= MAX_NODES)
	    {
		DXSetError(ERROR_INTERNAL, "exhausted supply of list nodes");
		return NULL;
	    }

	    n->offset = offset;
	    n->next = next;
	    last->next = n;

	    axisCounts[j] ++;
	}

    }

    omap = (OriginMap)DXAllocate(sizeof(struct originMap));
    if (! omap)
	goto error;
    
    omap->map = (OMapElt **)DXAllocateZero(nDim*sizeof(struct oMapElt *));
    if (! omap->map)
	goto error;
    
    omap->nDim = nDim;
    
    maps = omap->map;

    for (i = 0; i < nDim; i++)
    {
	maps[i]=(OMapElt *)DXAllocate((axisCounts[i]+1)*sizeof(struct originMap));
	if (! maps[i])
	    goto error;

	j = k = 0;
	next = &lists[i];
	while (next)
	{
	    maps[i][j].inOffset  = next->offset;
	    maps[i][j].outOffset = k;

	    last = next;
	    next = next->next;

	    if (! next)
		break;

	    if (factors[i] > 1.0)
	    {
		/*
		 * The input count in the last partition is
		 * ((nextOffset - lastOffset) + 1).  The output
		 * counts for that (see ReduceFieldTask) is 
		 * (counts + 1) / factor.  The offset of the next
		 * partition is last + (outCounts-1)
		 */
		l = (((next->offset - last->offset) + 2)/factors[i]);
		if (l < 2) l = 2;
		l -= 1;
	    }
	    else
		l = next->offset - last->offset;

	    k += l;
	    j++;
	}

	maps[i][j+1].inOffset  = -1;
	maps[i][j+1].outOffset = -1;

    }

    omapObject = DXNewPrivate((Pointer)omap, DeleteOriginMapObject);

    if (! omapObject)
	goto error;

    return omapObject;

error:
    if (omap)
	DeleteOriginMapObject((Pointer) omap);
    
    return NULL;
}

static Error 
DeleteOriginMapObject(Pointer p)
{
    int       i;
    OriginMap o;

    if (! p)
    {
	DXSetError(ERROR_INTERNAL,"DeleteOriginMapObject called on NULL pointer");
	return ERROR;
    }

    o = (OriginMap)p;

    for (i = 0; i < o->nDim; i++)
	DXFree((Pointer)o->map[i]);
    
    DXFree((Pointer)o->map);
    
    DXFree(p);
    
    return OK;
}

static Error
DeleteFactorsObject(Pointer p)
{   
    DXFree(p);
    return OK;
}

#define INDICES(ref, indices, strides, ndim)		\
{							\
    int i, j = (ref);					\
    for (i = 0; i < (ndim)-1; i++) {			\
	(indices)[i] = j / (strides)[i];		\
	j = j % (strides)[i];				\
    }							\
    (indices)[i] = j;					\
}

#define REFERENCE(indices, ref, strides, ndim) 		\
{							\
    int i, j = 0;					\
    for (i = 0; i < (ndim)-1; i++)			\
	j += (indices)[i]*(strides)[i];			\
    (ref) = j + (indices)[(ndim)-1];			\
}


static Error
InvalidateData(Field original, Field reduced, int *filter, int invalid)
{
    InvalidComponentHandle inInvalids = NULL, outInvalids = NULL;
    Array inArray, outArray;
    int i, nDim, index;
    int inCounts[10], inStrides[10];
    int outCounts[10], outStrides[10];
    float axisScale[10];
    int indices[10], min[10], max[10];

    inArray = (Array)DXGetComponentValue(original, "connections");
    if (! inArray)
    {
	DXSetError(ERROR_INTERNAL, "original has no connections");
	goto error;
    }

    if (! DXQueryGridConnections(inArray, &nDim, inCounts))
    {
	DXSetError(ERROR_INTERNAL, "original positions are not grid");
	goto error;
    }

    outArray = (Array)DXGetComponentValue(reduced, "connections");
    if (! outArray)
    {
	DXSetError(ERROR_INTERNAL, "reduced has no positions");
	goto error;
    }

    if (! DXQueryGridConnections(outArray, NULL, outCounts))
    {
	DXSetError(ERROR_INTERNAL, "reduced connections are not grid");
	goto error;
    }

    if (invalid == INVALID_POSITIONS)
    {
	inInvalids = DXCreateInvalidComponentHandle((Object)original,
						NULL, "positions");
	outInvalids = DXCreateInvalidComponentHandle((Object)reduced, 
						NULL, "positions");
    }
    else
    {
	for (i = 0; i < nDim; i++)
	{
	    inCounts[i] -= 1;
	    outCounts[i] -= 1;
	}

	inInvalids = DXCreateInvalidComponentHandle((Object)original,
						NULL, "connections");
	outInvalids = DXCreateInvalidComponentHandle((Object)reduced, 
						NULL, "connections");
    }

    inStrides[nDim-1] = outStrides[nDim-1] = 1;
    for (i = nDim-2; i >= 0; i--)
    {
	inStrides[i] = inStrides[i+1] * inCounts[i+1];
	outStrides[i] = outStrides[i+1] * outCounts[i+1];
    }

    for (i = 0; i < nDim; i++)
	if (inCounts[i] == 0)
	    axisScale[i] = 1.0;
	else
	    axisScale[i] = ((float)outCounts[i]) / ((float)inCounts[i]);

    if (! inInvalids || ! outInvalids)
	goto error;
    
    DXSetAllValid(outInvalids);

    DXInitGetNextInvalidElementIndex(inInvalids);
    while (-1 != (index = DXGetNextInvalidElementIndex(inInvalids)))
    {

	INDICES(index, indices, inStrides, nDim);

	for (i = 0; i < nDim; i++)
	{
	    int f = filter[i] >> 1;

	    /*
	     * Get the range of input samples along the axis that the
	     * invalid input entry affects.  This is the range of 
	     * input points that the invalid point affects in the
	     * filtering process.
	     */
	    min[i] = indices[i] - f;
	    if (min[i] < 0) min[i] = 0;
	    max[i] = indices[i] + f;
	    if (max[i] >= inCounts[i]) max[i] = inCounts[i] - 1;

	    /*
	     * Now transform these input indices to output indices
	     * by scaling.  This gives us the range of output points
	     * that are affected by this input point.
	     */
	    min[i] = floor(min[i] * axisScale[i]);
	    max[i] = floor(max[i] * axisScale[i]);

	    /*
	     * initialize the loop
	     */
	    indices[i] = min[i];
	}

	/*
	 * Now loop to set the appropriate output points invalid
	 */
	for ( ;; )
	{
	    int r;

	    REFERENCE(indices, r, outStrides, nDim);

	    if (! DXSetElementInvalid(outInvalids, r))
		goto error;
	    
	    for (i = 0; i < nDim; i++)
	    {
		indices[i] ++;
		if (indices[i] <= max[i])
		    break;

		indices[i] = min[i];
	    }

	    if (i == nDim)
		break;
	}
    }

    if (! DXSaveInvalidComponent(reduced, outInvalids))
	goto error;
    
    DXFreeInvalidComponentHandle(outInvalids);
    DXFreeInvalidComponentHandle(inInvalids);
    outInvalids = inInvalids = NULL;

    return OK;

error:
    DXFreeInvalidComponentHandle(outInvalids);
    DXFreeInvalidComponentHandle(inInvalids);

    return ERROR;
}
