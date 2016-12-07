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

#include <math.h>
#include <dx/dx.h>
#include "measure.h"
#include "vectors.h"

#define WHAT_NONE	0
#define WHAT_VOLUME	1
#define WHAT_AREA	2
#define WHAT_LENGTH     3
#define WHAT_ELEMENT    4

Object DXMeasure(Object, char *);

static Error  MeasureTask(Pointer);
static Object MeasureObject(Object, int);
static Object MeasureElements(Object);
static Field  MeasureElements_Field(Field);

typedef struct
{
    int   nSegments;
    int   *segments;
    Array positions;
    Array connections;
} Segments;

static Error Cubes_Volume(Field,      Segments *, float *);
static Error Tetrahedra_Volume(Field, Segments *, float *);
static Error Triangles_Area(Field,    Segments *, float *);
static Error Triangles_Volume(Field,  Segments *, float *);
static Error Quads_Area(Field,        Segments *, float *);
static Error Quads_Volume(Field,      Segments *, float *);
static Error Lines_Length(Field,      Segments *, float *);
static Error Lines_Area(Field,        Segments *, float *);

static Error FLE_Area(Field, Segments *, float *);
static Error FLE_Area_Elements(Field, float *);

static Error Polyline_Length(Field, Segments *, float *);
static Error Polyline_Length_Elements(Field, float *);

static Error Volume_CField(int, Segments *, float *);
static Error Loop_Volume(int, int, Line *, Vector *, Array, float *);
static Error Loop_Area(int, Segments *, float *);

Error
m_Measure(Object *in, Object *out)
{
    char *what;

    out[0] = NULL;

    if (! in[0])
    {
	DXSetError(ERROR_MISSING_DATA, "#10000", "input");
	goto error;
    }

    if (! in[1])
	what = NULL;
    else if (! DXExtractString(in[1], &what))
    {
	DXSetError(ERROR_BAD_PARAMETER, "\"what\" must be a string");
	goto error;
    }
    
    out[0] = DXMeasure(in[0], what);
    if (! out[0])
	goto error;

    return OK;


error:
    return ERROR;
}

Object
DXMeasure(Object object, char *what)
{
    if (what && !strcmp(what, "element"))
    {
	Object copy = DXCopy(object, COPY_STRUCTURE);
	if (! copy)
	    return NULL;

	if (! MeasureElements(copy))
	{
	    if (copy != object)
		DXDelete(copy);
	    return NULL;
	}

	return copy;
    }
    else
    {
	int iwhat;

	if (! what)
	    iwhat = WHAT_NONE;
	else if (! strcmp(what, "volume"))
	    iwhat = WHAT_VOLUME;
	else if (! strcmp(what, "area"))
	    iwhat = WHAT_AREA;
	else if (! strcmp(what, "length"))
	    iwhat = WHAT_LENGTH;
	else if (! strcmp(what, "element"))
	    iwhat = WHAT_ELEMENT;
	else
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10430", what);
	    return NULL;
	}

	return MeasureObject(object, iwhat);

    }

}

typedef float (*PFF)();
typedef Error (*PFE)();

typedef struct
{
    Object   child;
    int      what;
    float    *measure;
    Segments *segs;
    PFE      task;
} MeasureTaskArgs;

static Error
MeasureTask(Pointer ptr)
{
    Object   object;
    float    *measure;
    Segments *segs;
    PFE      task;

    object  = ((MeasureTaskArgs *)ptr)->child;
    /* what    = ((MeasureTaskArgs *)ptr)->what; */
    measure = ((MeasureTaskArgs *)ptr)->measure;
    segs    = ((MeasureTaskArgs *)ptr)->segs;
    task    = ((MeasureTaskArgs *)ptr)->task;

    if (! (*task)(object, segs, measure) || DXGetError() != ERROR_NONE)
	return ERROR;
    else
	return OK;
}

typedef struct 
{
    char *etype;
    int  what;
    PFE  fMethod;
    PFE  cfMethod;
    int  growFlag;
} MethodTableEntry;

static MethodTableEntry methodTable[] =
{
    {"triangles",  WHAT_AREA,   Triangles_Area,    NULL,	  0},
    {"triangles",  WHAT_NONE,   Triangles_Area,    NULL,	  0},
    {"triangles",  WHAT_VOLUME, Triangles_Volume,  Volume_CField, 1},
    {"cubes",      WHAT_NONE,   Cubes_Volume,      NULL,	  0},
    {"cubes",      WHAT_VOLUME, Cubes_Volume,      NULL,	  0},
    {"tetrahedra", WHAT_NONE,   Tetrahedra_Volume, NULL,	  0},
    {"tetrahedra", WHAT_VOLUME, Tetrahedra_Volume, NULL,	  0},
    {"quads",      WHAT_NONE,   Quads_Area,        NULL,	  0},
    {"quads",      WHAT_AREA,   Quads_Area,        NULL,	  0},
    {"quads",      WHAT_VOLUME, Quads_Volume,      Volume_CField, 1},
    {"lines",      WHAT_NONE,   Lines_Length,      NULL,	  0},
    {"lines",      WHAT_LENGTH, Lines_Length,      NULL,	  0},
    {"lines",      WHAT_AREA,   Lines_Area,        Loop_Area,	  0},
    {"faces",      WHAT_AREA,   FLE_Area,          NULL,    	  0},
    {"faces",      WHAT_NONE,   FLE_Area,          NULL,    	  0},
    {"polylines",  WHAT_LENGTH, Polyline_Length,   NULL,    	  0},
    {"polylines",  WHAT_NONE,   Polyline_Length,   NULL,    	  0},
    {"",-1,NULL,NULL}
};

static Error
GetMethods(Object o, int what, PFE *fieldMethod, PFE *cfieldMethod, int *gFlag)
{
    Class class = DXGetObjectClass(o);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)o);
    
    switch(class)
    {
	case CLASS_FIELD:
	{
	    Object attr;
	    char *str;
	    MethodTableEntry *m;

	    if (DXEmptyField((Field)o))
		return ERROR;

	    if (DXGetComponentValue((Field)o, "connections"))
	    {
		attr = DXGetComponentAttribute((Field)o, "connections",
						"element type");

		if (attr == NULL)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10255",
					    "connections", "element type");
		    return ERROR;
		}

		if (attr == NULL || DXGetObjectClass(attr) != CLASS_STRING)
		{
		    DXSetError(ERROR_DATA_INVALID, "#10200", 
			    "element type attribute");
		    return ERROR;
		}

		str = DXGetString((String)attr);
	    }
	    else if (DXGetComponentValue((Field)o, "faces"))
	    {
		str = "faces";
	    }
	    else if (DXGetComponentValue((Field)o, "polylines"))
	    {
		str = "polylines";
	    }
	    else
	    {
		DXSetError(ERROR_DATA_INVALID,
			"Measure requires connections, faces or polylines");
		return ERROR;
	    }

	    for (m = methodTable; m->what != -1; m++)
	    {
		if (! strcmp(m->etype, str) && m->what == what)
		    break;
	    }

	    if (m->what == -1)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#10440",
			what == WHAT_AREA ? "area" :
			    what == WHAT_LENGTH ? "length" :
				what == WHAT_VOLUME ? "volume" : 
				   "(unknown measurement type)", str);
		return ERROR;
	    }
	
	    if (fieldMethod)
		*fieldMethod  = m->fMethod;

	    if (cfieldMethod)
		*cfieldMethod = m->cfMethod;
	    
	    if (gFlag)
		*gFlag = m->growFlag;

	    return OK;
	}

	case CLASS_COMPOSITEFIELD:
	{
	    int i = 0; Object c; Group g = (Group)o;
	    while (NULL != (c = DXGetEnumeratedMember(g, i++, NULL)))
		if (GetMethods(c, what, fieldMethod, cfieldMethod, gFlag))
		    return OK;
		else if (DXGetError() != ERROR_NONE)
		    return ERROR;
	    
	    return ERROR;
	}
	default:
	    break;
    }

    DXSetError(ERROR_BAD_PARAMETER, "Invalid Class type.");
    return ERROR;
}

static Object
MeasureObject(Object object, int what)
{
    float     *measureTable = NULL;
    Segments  *segments = NULL;
    Class     class;
    Object    result = NULL;
    Object    workCopy = NULL;
    float     measure;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);

    workCopy = DXCopy(object, COPY_STRUCTURE);
    if (! workCopy)
	goto error;
		
    if (! DXInvalidateConnections(workCopy))
	goto error;

    if (! DXInvalidateUnreferencedPositions(workCopy))
	goto error;

    switch (class)
    {
	case CLASS_FIELD:
	{
	    PFE task;

	    if (DXEmptyField((Field)workCopy))
	    {
		measure = 0.0;
	    }
	    else
	    {
		if (! GetMethods(workCopy, what, &task, NULL, NULL))
		{
		    if (DXGetError() == ERROR_NONE)
			DXSetError(ERROR_INTERNAL, 
		    "unknown problem getting methods for non-empty field");
		    goto error;
		}

		if (! (*task)((Field)workCopy, NULL, &measure))
		    goto error;
	    }
	    
	    measure = fabs(measure);

	    result = (Object)DXNewConstantArray(1, &measure, 
				    TYPE_FLOAT, CATEGORY_REAL, 0);
	    
	}
	break;
	
	case CLASS_MULTIGRID:
	{
	    int i;
	    Object child;

	    measure = 0.0;

	    for (i = 0;
		 NULL != (child=DXGetEnumeratedMember((Group)workCopy,i,NULL));
		 i++)
	    {
		Object c_obj = MeasureObject(child, what);
		float  c_meas;
		
		if (c_obj == NULL)
		    goto error;


		c_meas = *(float *)DXGetConstantArrayData((Array)c_obj);

		if (c_meas == -1)
		    goto error;
		
		measure += c_meas;
	    }
	    
	    result = (Object)DXNewConstantArray(1, &measure, 
				    TYPE_FLOAT, CATEGORY_REAL, 0);
	    
	}
	break;

	case CLASS_COMPOSITEFIELD:
	{
	    MeasureTaskArgs args;
	    Object child;
	    int i;
	    PFE ftask, ctask;
	    int growFlag;

	    if (! GetMethods(workCopy, what, &ftask, &ctask, &growFlag))
	    {
		if (DXGetError() == ERROR_NONE)
		    measure = 0;
		else
		    goto error;
	    }
	    else
	    {
		if (growFlag)
		{
		    if (! DXCull(workCopy))
			goto error;

		    if (! DXGrow(workCopy, 1, NULL, NULL))
			goto error;
		}

		for (i = 0; DXGetEnumeratedMember((Group)workCopy, i, NULL); i++);

		measureTable = (float *)DXAllocate(i*sizeof(float));
		if (! measureTable)
		    goto error;
		
		segments = (Segments *)DXAllocateZero(i*sizeof(Segments));
		if (! segments)
		    goto error;
		
		if (! DXCreateTaskGroup())
		    goto error;
		
		for (i = 0;
		     NULL != (child=DXGetEnumeratedMember((Group)workCopy,i,NULL));
		     i++)
		{
		    args.child    = child;
		    args.what     = what;
		    args.measure  = measureTable + i;
		    args.segs     = segments + i;
		    args.task     = ftask;
		
		    if (! DXAddTask(MeasureTask,(Pointer)&args, sizeof(args), 1.0))
		    {
			DXAbortTaskGroup();
			goto error;
		    }
		}

		if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		    goto error;
		
		if (ctask)
		{
		    if (! (*ctask)(i, segments, &measure))
			goto error;
		}
		else
		    measure = 0.0;

		while (i > 0)
		    measure += measureTable[--i];
		
		measure = fabs(measure);
		
		if (DXGetError() != ERROR_NONE)
		    goto error;

		if (segments)
		    DXFree((Pointer)segments);
		    
		DXFree((Pointer)measureTable);
		measureTable = NULL;
	    }

	    result = (Object)DXNewConstantArray(1, &measure, 
				    TYPE_FLOAT, CATEGORY_REAL, 0);
	    
	}
	break;

	case CLASS_GROUP:
	{
	    Object child;
	    char *name;
	    int i;

	    result = DXCopy(workCopy, COPY_ATTRIBUTES);

	    measure = 0.0;
	    i = 0;
	    for ( ;; )
	    {
		child = DXGetEnumeratedMember((Group)workCopy, i, &name);
		if (! child)
		    break;

		child = MeasureObject(child, what);
		if (! child)
		    goto error;

		if (! DXSetMember((Group)result, name, child))
		    goto error;

		i++;
	    }
	}
	break;

	case CLASS_SERIES:
	{
	    Object child;
	    float pos;
	    int i;

	    result = DXCopy(workCopy, COPY_ATTRIBUTES);

	    measure = 0.0;
	    i = 0;
	    for ( ;; )
	    {
		child = DXGetSeriesMember((Series)workCopy, i, &pos);
		if (! child)
		    break;

		child = MeasureObject(child, what);
		if (! child)
		    goto error;

		if (! DXSetSeriesMember((Series)result, i, pos, child))
		    goto error;
		
		i++;
	    }
	}
	break;

	case CLASS_XFORM:
	{

	    workCopy = DXApplyTransform(workCopy, NULL);
	    if (! workCopy)
		goto error;

	    result = MeasureObject(workCopy, what);
	}
	break;
	   
	case CLASS_CLIPPED:
	{
	    Object child;

	    if (! DXGetClippedInfo((Clipped)workCopy, &child, NULL))
		goto error;
	    
	    result = MeasureObject(child, what);
	}
	break;

	default:
	    DXSetError(ERROR_BAD_PARAMETER, "input must be field or group");
	    goto error;
    }

    if (workCopy != object)
	DXDelete(workCopy);
    
    return result;

error:
    if (workCopy != object)
	DXDelete(workCopy);

    DXFree((Pointer)measureTable);
    DXFree((Pointer)segments);

    return NULL;
}

static Error
Lines_Length(Field field, Segments *segs, float *measure)
{
    Array       pA, cA;
    ArrayHandle pHandle = NULL, cHandle = NULL;
    float       delta[9];
    int         nDim, nSegments;
    float       length;
    int 	counts[3];
    int	 	regP;
    InvalidComponentHandle ich = NULL;

    *measure = 0.0;

    if (DXEmptyField(field))
	return OK;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	return OK;
    
    DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nDim);

    if (nDim > 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#10341", "positions");
	return OK;
    }

    if (DXQueryGridPositions(pA, NULL, counts, NULL, delta))
    {
	int i;

	regP = 1;

	for (i = 0; i < nDim; i++)
	    if (counts[i] != 1)
	    {
		memcpy(delta, delta+(i*nDim), nDim*sizeof(float));
		break;
	    }
    }
    else if (DXGetArrayClass(pA) == CLASS_REGULARARRAY)
    {
	DXGetRegularArrayInfo((RegularArray)pA, NULL, NULL, (Pointer)delta);
	regP = 1;
    }
    else
	regP = 0;

    DXGetArrayInfo(cA, &nSegments, NULL, NULL, NULL, NULL);

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "connections");
	if (ich == NULL)
	    goto error;
    }

    if (!ich && (DXQueryGridConnections(cA, NULL, NULL) ||
	DXGetArrayClass(cA) == CLASS_PATHARRAY) && regP)
    {
	int i;

	length = 0;
	for (i = 0; i < nDim; i++)
	    length += delta[i] * delta[i];
	length = nSegments*sqrt(length);
    }
    else
    {
	int seg;

	pHandle = DXCreateArrayHandle(pA);
	if (! pHandle)
	    goto error;
    
	cHandle = DXCreateArrayHandle(cA);
	if (! cHandle)
	    goto error;

	length = 0;
	for (seg = 0; seg < nSegments; seg++)
	{
	    int i;
	    float  seglen, a;
	    Line    *line, lBuf;
	    float  *fp, *fq, fpBuf[3], fqBuf[3];

	    if (ich && DXIsElementInvalid(ich, seg))
		continue;

	    line = (Line *)DXCalculateArrayEntry(cHandle, seg, (Pointer)&lBuf);

	    fp = (float *)DXCalculateArrayEntry(pHandle,
					line->p, (Pointer)fpBuf);
	    fq = (float *)DXCalculateArrayEntry(pHandle,
					line->q, (Pointer)fqBuf);

	    seglen = 0;
	    for (i = 0; i < nDim; i++)
	    {
		a = *fp++ - *fq++;
		seglen += a*a;
	    }

	    length += sqrt(seglen);
	}
    }

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    *measure = length;
    return OK;

error:
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    
    return ERROR;
}

static Error
Triangles_Area(Field field, Segments *segs, float *measure)
{
    Array       pA, cA;
    ArrayHandle pHandle = NULL;
    int         *elements, nDim, nTris;
    float       area;
    int         tri;
    InvalidComponentHandle ich = NULL;

    *measure = 0.0;

    if (DXEmptyField(field))
	return OK;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	return OK;

    pHandle = DXCreateArrayHandle(pA);
    if (! pHandle)
	goto error;
    
    DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nDim);
    DXGetArrayInfo(cA, &nTris, NULL, NULL, NULL, NULL);

    elements = (int *)DXGetArrayData(cA);

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "connections");
	if (ich == NULL)
	    goto error;
    }

    area = 0;
    for (tri = 0; tri < nTris; tri++)
    {
	int    ip, iq, ir;
	float  fpBuf[3], fqBuf[3], frBuf[3];
	float  *fp = fpBuf, *fq = fqBuf, *fr = frBuf;
	int    i;
	float  v0[3], v1[3];

	if (ich && DXIsElementInvalid(ich, tri))
	{
	    elements += 3;
	    continue;
	}

	ip = *elements++;
	iq = *elements++;
	ir = *elements++;

	fp = (float *)DXCalculateArrayEntry(pHandle, ip, (Pointer)fpBuf);
	fq = (float *)DXCalculateArrayEntry(pHandle, iq, (Pointer)fqBuf);
	fr = (float *)DXCalculateArrayEntry(pHandle, ir, (Pointer)frBuf);

	for (i = 0; i < nDim; i++)
	{
	    v0[i] = fq[i] - fp[i];
	    v1[i] = fr[i] - fp[i];
	}

	if (nDim == 2)
	    area += v0[0]*v1[1] - v0[1]*v1[0];
	else
	{
	    Vector cross;
	    vector_cross((Vector *)v0, (Vector *)v1, &cross);
	    area += vector_length(&cross);
	}
    }

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    *measure = 0.5*area;
    return OK;

error:
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    
    return ERROR;
}

static Error
Quads_Area(Field field, Segments *segs, float *measure)
{
    Array       pA, cA;
    ArrayHandle pHandle = NULL, cHandle = NULL;
    int         nDim, nQuads, counts[3];
    float       area, deltas[9];
    int         i;
    InvalidComponentHandle ich = NULL;

    *measure = 0.0;

    if (DXEmptyField(field))
	return OK;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	return OK;
    
    DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nDim);
    if (nDim != 2 && nDim != 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#10340", "positions");
	return ERROR;
    }

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "connections");
	if (ich == NULL)
	    goto error;
    }

    if (!ich && DXQueryGridConnections(cA, NULL, NULL) &&
        DXQueryGridPositions(pA, NULL, counts, NULL, deltas))
    {
	if (nDim == 2)
	{
	    area = deltas[0]*deltas[3] - deltas[2]*deltas[1];
	    area *= (counts[0]-1) * (counts[1]-1);
	}
	else if (nDim == 3)
	{
	    Vector cross;
	    Vector *v[3];
	    int i, j, k = 1;

	    for (i = j = 0; i < 3; i++)
		if (counts[i] > 1)
		{
		    v[j++] = (Vector *)(deltas + i*3);
		    k *= (counts[i]-1);
		}
	    
	    if (j != 2)
	    {
		DXSetError(ERROR_DATA_INVALID,
			"quad positions must be planar");
		goto error;
	    }

	    vector_cross(v[0], v[1], &cross);
	    area = k*vector_length(&cross);
	}
	else
	{
	    DXSetError(ERROR_DATA_INVALID, "quads must be in 2-D or 3-D");
	    goto error;
	}


	*measure = area;
	return OK;
    }

    DXGetArrayInfo(cA, &nQuads, NULL, NULL, NULL, NULL);

    pHandle = DXCreateArrayHandle(pA);
    if (! pHandle)
	goto error;
    
    cHandle = DXCreateArrayHandle(cA);
    if (! cHandle)
	goto error;

    area = 0;
    for (i = 0; i < nQuads; i++)
    {
	int    ip, iq, ir, is;
	float  fpBuf[3], fqBuf[3], frBuf[3], fsBuf[3];
	float  *fp = fpBuf, *fq = fqBuf, *fr = frBuf, *fs = fsBuf;
	int    j;
	float  v0[3], v1[3];
	int    *quad, quadBuf[4];

	if (ich && DXIsElementInvalid(ich, i))
	    continue;

	quad = (int *)DXCalculateArrayEntry(cHandle, i, (Pointer)quadBuf);

	ip = quad[0];
	iq = quad[1];
	ir = quad[2];
	is = quad[3];

	fp = (float *)DXCalculateArrayEntry(pHandle, ip, (Pointer)fpBuf);
	fq = (float *)DXCalculateArrayEntry(pHandle, iq, (Pointer)fqBuf);
	fr = (float *)DXCalculateArrayEntry(pHandle, ir, (Pointer)frBuf);
	fs = (float *)DXCalculateArrayEntry(pHandle, is, (Pointer)fsBuf);

	for (j = 0; j < nDim; j++)
	{
	    v0[j] = fq[j] - fp[j];
	    v1[j] = fr[j] - fp[j];
	}

	if (nDim == 2)
	    area += v0[0]*v1[1] - v0[1]*v1[0];
	else
	{
	    Vector cross;
	    vector_cross((Vector *)v0, (Vector *)v1, &cross);
	    area += vector_length(&cross);
	}

	for (j = 0; j < nDim; j++)
	{
	    v0[j] = fr[j] - fs[j];
	    v1[j] = fq[j] - fs[j];
	}

	if (nDim == 2)
	    area += v0[0]*v1[1] - v0[1]*v1[0];
	else
	{
	    Vector cross;
	    vector_cross((Vector *)v0, (Vector *)v1, &cross);
	    area += vector_length(&cross);
	}
    }

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    *measure = 0.5*area;
    return OK;

error:
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    return ERROR;
}

static Error
Tetrahedra_Volume(Field field, Segments *segs, float *measure)
{
    Array       pA, cA;
    ArrayHandle pHandle = NULL;
    int         *elements, nDim, nTets;
    float       area;
    int         i;
    InvalidComponentHandle ich = NULL;

    *measure = 0.0;

    if (DXEmptyField(field))
	return OK;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	return OK;
    
    DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nDim);

    pHandle = DXCreateArrayHandle(pA);
    if (! pHandle)
	 goto error;

    DXGetArrayInfo(cA, &nTets, NULL, NULL, NULL, NULL);
    elements = (int *)DXGetArrayData(cA);

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "connections");
	if (ich == NULL)
	    goto error;
    }

    area = 0;
    for (i = 0; i < nTets; i++)
    {
	int    ip, iq, ir, is;
	float  fpBuf[3], fqBuf[3], frBuf[3], fsBuf[3];
	float  *fp = fpBuf, *fq = fqBuf, *fr = frBuf, *fs = fsBuf;
	int    j;
	float  v0[3], v1[3], v2[3];
	Vector cross;

	if (ich && DXIsElementInvalid(ich, i))
	{ 
	    elements += 4;
	    continue;
	}

	ip = *elements++;
	iq = *elements++;
	ir = *elements++;
	is = *elements++;

	fp = (float *)DXCalculateArrayEntry(pHandle, ip, (Pointer)fpBuf);
	fq = (float *)DXCalculateArrayEntry(pHandle, iq, (Pointer)fqBuf);
	fr = (float *)DXCalculateArrayEntry(pHandle, ir, (Pointer)frBuf);
	fs = (float *)DXCalculateArrayEntry(pHandle, is, (Pointer)fsBuf);

	for (j = 0; j < nDim; j++)
	{
	    v0[j] = fq[j] - fp[j];
	    v1[j] = fr[j] - fp[j];
	    v2[j] = fs[j] - fp[j];
	}

	vector_cross((Vector *)v1, (Vector *)v0, &cross);
	area += vector_dot(&cross, (Vector *)v2);
    }

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    *measure = area/6.0;
    return OK;

error:
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    return ERROR;
}


static int cubeToTets[] = 
{
    1, 2, 7, 3,
    7, 2, 4, 6,
    1, 7, 4, 5,
    1, 4, 2, 0,
    1, 2, 4, 7
};

static Error
Cubes_Volume(Field field, Segments *segs, float *measure)
{
    Array       pA, cA;
    ArrayHandle pHandle = NULL, cHandle = NULL;
    int         counts[3];
    float       deltas[9];
    int         nDim, nCubes;
    float       volume;
    int         i, *cube, cubeBuf[8];
    InvalidComponentHandle ich = NULL;

    *measure = 0;

    if (DXEmptyField(field))
	return OK;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	return OK;
    
    DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &nDim);
    if (nDim != 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#10342", "positions");
	return ERROR;
    }

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "connections");
	if (ich == NULL)
	    goto error;
    }

    if (!ich && DXQueryGridConnections(cA, NULL, NULL) &&
	DXQueryGridPositions(pA, NULL, counts, NULL, deltas))
    {
	Vector *v0 = (Vector *)deltas;
	Vector *v1 = (Vector *)(deltas + nDim);
	Vector *v2 = (Vector *)(deltas + 2*nDim);
	Vector cross;

	vector_cross((Vector *)v0, (Vector *)v1, &cross);
	volume = (counts[0]-1)*(counts[1]-1)*(counts[2]-1)
			    *vector_dot((Vector *)v2, &cross);

	*measure = volume;
	return OK;
    }
    
    DXGetArrayInfo(cA, &nCubes, NULL, NULL, NULL, NULL);

    cHandle = DXCreateArrayHandle(cA);
    if (! cHandle)
	goto error;

    pHandle = DXCreateArrayHandle(pA);
    if (! pHandle)
	goto error;

    if (DXGetComponentValue(field, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "connections");
	if (ich == NULL)
	    goto error;
    }

    volume = 0;
    for (i = 0; i < nCubes; i++)
    {
	int    j;
	Vector pBuf[8];
	Vector *pts[8];

	if (ich && DXIsElementInvalid(ich, i))
	    continue;

	cube = (int *)DXCalculateArrayEntry(cHandle, i, (Pointer)cubeBuf);

	for (j = 0; j < 8; j++)
	    pts[j] = (Vector *)DXCalculateArrayEntry(pHandle,
						cube[j], (Pointer)(pBuf + j));

	for (j = 0; j < 5; j++)
	{
	    Vector *fp = pts[cubeToTets[j*4 + 0]];
	    Vector *fq = pts[cubeToTets[j*4 + 1]];
	    Vector *fr = pts[cubeToTets[j*4 + 2]];
	    Vector *fs = pts[cubeToTets[j*4 + 3]];
	    Vector v0, v1, v2;
	    Vector cross;

	    vector_subtract(fq, fp, &v0);
	    vector_subtract(fr, fp, &v1);
	    vector_subtract(fs, fp, &v2);

	    vector_cross((Vector *)&v0, (Vector *)&v1, &cross);
	    volume += vector_dot((Vector *)&v2, &cross);
	}
    }

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);

    *measure = volume/6.0;
    return OK;

error:
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    return ERROR;
}

typedef struct
{
    int    cindex;
    Vector point;
} HashElement3;

typedef struct
{
    int    cindex;
    float  point[2];
} HashElement2;

static Error
Triangles_Volume(Field field, Segments *segments, float *measure)
{
    Vector origin;
    int  i;
    Array ocA, cA, pA, nA;
    Vector *points;
    int *t, *tris, *n, *nbrs;
    int nPts, nTris, nEdges, nDim;
    Vector *p, *q, *r, vqp, vrp, vsp, cross;
    float area = 0.0;
    Line *segs = NULL;

    *measure = 0;

    if (DXEmptyField(field))
	return OK;

    origin.x = origin.y = origin.z = 0.0;

    pA = (Array)DXGetComponentValue(field, "positions");
    cA = (Array)DXGetComponentValue(field, "connections");
    if (! cA)
	return OK;
    
    nA = (Array)DXNeighbors(field);
    if (! nA)
	goto error;
    
    points = (Vector *)DXGetArrayData(pA);
    DXGetArrayInfo(pA, &nPts, NULL, NULL, NULL, &nDim);
    if (nDim != 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#10342", "positions");
	goto error;
    }

    /*
     * If the field was the result of Grow, then we only add in the
     * original triangles
     */
    ocA = (Array)DXGetComponentValue(field, "original connections");
    if (ocA)
	DXGetArrayInfo(ocA, &nTris, NULL, NULL, NULL, NULL);
    else
	DXGetArrayInfo(cA, &nTris, NULL, NULL, NULL, NULL);
    
    t = tris = (int *)DXGetArrayData(cA);
    n = nbrs = (int *)DXGetArrayData(nA);

    /*
     * For each triangle, compute the volume.  Look at the neighbors to
     * see if there are any unshared edges.
     */
    area = 0; nEdges = 0;
    for (i = 0; i < nTris; i++)
    {
	p = points + *t++;
	q = points + *t++;
	r = points + *t++;
	vector_subtract(q, p, &vqp);
	vector_subtract(r, p, &vrp);
	vector_subtract(&origin, p, &vsp);
	vector_cross(&vqp, &vrp, &cross);
	area += vector_dot(&cross, &vsp);

	if (*n++ < 0) nEdges++;
	if (*n++ < 0) nEdges++;
	if (*n++ < 0) nEdges++;
    }

    area = area / 6.0;
    
    /*
     * If there are unshared edges, get list of them.
     */
    if (nEdges)
    {
	Line *s;

	segs = (Line *)DXAllocate(nEdges*sizeof(Line));
	if (! segs)
	    goto error;
	
	s = segs;
	for (n = nbrs, t = tris, i = 0; i < nTris; n += 3, t += 3, i++)
	{
	    if (n[0] < 0) {s->p = t[1]; s->q = t[2]; s++;};
	    if (n[1] < 0) {s->p = t[0]; s->q = t[2]; s++;};
	    if (n[2] < 0) {s->p = t[0]; s->q = t[1]; s++;};
	}
    
	/*
	 * If this was an element of a composite field, we will need to
	 * accumulate the loops between all the composite field members.
	 * Otherwise, we can go ahead and add in the loop areas here.
	 */
	if (segments)
	{
	    segments->nSegments = nEdges;
	    segments->segments = (int *)segs;
	    segments->positions = pA;
	}
	else
	{
	    float tmp;

	    if (! Loop_Volume(nEdges, nPts, segs, points, NULL, &tmp))
		goto error;

	    area += tmp;

	    DXFree((Pointer)segs);
	    segs = NULL;
	}
    }
    else
    {
	if (segments)
	{
	    segments->nSegments = 0;
	    segments->segments = NULL;
	    segments->positions = NULL;
	}
    }

    if (DXGetError() != ERROR_NONE)
	return ERROR;

    *measure = area;
    return OK;

error:
    DXFree((Pointer)segs);
    if (segments)
	segments->segments = NULL;
    
    return ERROR;
}

static PseudoKey pHash3(Key);
static int       pCmp3(Key, Key);
static PseudoKey pHash2(Key);
static int       pCmp2(Key, Key);

#define HASHPOINT3(pPtr, offset)					       \
{									       \
    HashElement3 *hPtr, hElt;						       \
									       \
    memcpy(&hElt.point, pPtr, sizeof(Vector));				       \
    if (NULL == (hPtr=(HashElement3 *)DXQueryHashElement(hash, (Key)(&hElt)))) \
    {									       \
	offset = hElt.cindex = nPts++;					       \
	if (! DXInsertHashElement(hash, (Element)&hElt))		       \
	    goto error;							       \
    }									       \
    else								       \
	offset = hPtr->cindex;						       \
}

#define HASHPOINT2(pPtr, offset)					       \
{									       \
    HashElement2 *hPtr, hElt;						       \
									       \
    memcpy(&hElt.point[0], pPtr, 2*sizeof(float));			       \
    if (NULL == (hPtr=(HashElement2 *)DXQueryHashElement(hash, (Key)(&hElt))))\
    {									       \
	offset = hElt.cindex = nPts++;					       \
	if (! DXInsertHashElement(hash, (Element)&hElt))		       \
	    goto error;							       \
    }									       \
    else								       \
	offset = hPtr->cindex;						       \
}

static Error
Volume_CField(int nParts, Segments *edges, float *measure)
{
    int i, j, nEdges, nPts;
    Line *segs = NULL, *sPtr;
    HashTable hash = NULL;
    Vector *points = NULL;
    float vol;
    HashElement3 *hPtr;

    *measure = 0.0;

    nEdges = 0;
    for (i = 0; i < nParts; i++)
	nEdges += edges[i].nSegments;
    
    if (nEdges == 0)
	return OK;

    /* 
     * We need to associate positions from different partitions so that 
     * we can determine how loops match up.  This hash table will give a
     * index to each unique point referred to by some edge of the edge list.
     */
    hash = DXCreateHash(sizeof(HashElement3), pHash3, pCmp3);
    if (! hash)
	goto error;
       
    /* 
     * Segs will be a list of segments from all the partitions that refer
     * to the unified positions indices.
     */
    segs = (Line *)DXAllocate(nEdges * sizeof(Line));
    if (! segs)
	goto error;
    
    /*
     * For each partition.  For each segment in the partition, if the
     * endpoint is not already in the hash table, add it.  Then add the
     * hashed index to the accumulated segment list.
     */
    nPts = 0; sPtr = segs;
    for (i = 0; i < nParts; i++)
    {
	Array       pA    = edges[i].positions;
	Line        *pSeg = (Line *)edges[i].segments;
	ArrayHandle handle;

	if (edges[i].nSegments == 0)
	    continue;

	handle = DXCreateArrayHandle(pA);
	if (! handle)
	    goto error;

	/*
	 * For each endpoint of the segment, get the index of the point
	 * in the accumulated positions.  If not there already, add the 
	 * point to the hash table
	 */
	for (j = 0; j < edges[i].nSegments; j++, sPtr ++, pSeg ++)
	{
	    float *p, pBuf[3];
	    float *q, qBuf[3];

	    p = (float *)DXCalculateArrayEntry(handle, pSeg->p, (Pointer)pBuf);
	    q = (float *)DXCalculateArrayEntry(handle, pSeg->q, (Pointer)qBuf);

	    HASHPOINT3(p, sPtr->p);
	    HASHPOINT3(q, sPtr->q);
	}

	DXFreeArrayHandle(handle);
	handle = NULL;

	/*
	 * Got the info we needed from the partition.  Delete the
	 * segment list.  The point list was a pointer to the
	 * partitions positions array data, so we don't delete it.
	 */
	DXFree((Pointer)edges[i].segments);
    }

    /*
     * Create a table indexed by the mapped vertex pointers that
     * contains pointers to points.
     */
    points = (Vector *)DXAllocate(nPts * 3 * sizeof(float));
    if (! points)
	goto error;
    
    DXInitGetNextHashElement(hash);
    while (NULL != (hPtr = (HashElement3 *)DXGetNextHashElement(hash)))
	memcpy(points+hPtr->cindex, &hPtr->point, sizeof(Vector));
    
    DXDestroyHash(hash);
    hash = NULL;

    /*
     * Now determine the volume that these loops comprise
     */
    if (! Loop_Volume(nEdges, nPts, segs, points, NULL, &vol))
	goto error;

    DXFree((Pointer)segs);
    DXFree((Pointer)points);

    *measure = vol;
    return OK;

error:
    DXDestroyHash(hash);
    DXFree((Pointer)segs);
    DXFree((Pointer)points);

    return ERROR;
    
}

typedef struct link
{
    struct link *link;
    int  tail, seg;
} Link;

static Error
Loop_Volume(int nEdges, int nPts, Line *segs, Vector *points, Array pA, float *measure)
{
    int i, j;
    int *forward = NULL, *backward = NULL, *combined = NULL, *list;
    Link  **index = NULL, *links = NULL, *link = NULL;
    ubyte *flags = NULL;
    int nexte, nf, nb, done;
    Vector *p, *r, vrp, vsp, vqp, cross;
    Line *sPtr;
    float vol;
    Vector origin;
    ArrayHandle pHandle = NULL;
    Vector pBuf, qBuf, rBuf;
    int reverse;

    *measure = 0.0;

    if (nEdges == 0.0)
	return OK;

    /*
     * Since these will be passed around later, regardless of
     * whether they have real stuff in them, better init them now.
     */
    memset(&pBuf, 0, sizeof(pBuf));
    memset(&qBuf, 0, sizeof(qBuf));
    memset(&rBuf, 0, sizeof(rBuf));

    origin.x = origin.y = origin.z = 0.0;

    if (points == NULL)
	DXGetArrayInfo(pA, &nPts, NULL, NULL, NULL, NULL);

    forward  = (int      *)DXAllocate((nEdges+2)*sizeof(int));
    backward = (int      *)DXAllocate((nEdges+2)*sizeof(int));
    combined = (int      *)DXAllocate((nEdges+2)*sizeof(int));
    flags    = (ubyte    *)DXAllocateZero((nEdges+2)*sizeof(ubyte));
    index    = (Link    **)DXAllocateZero(nPts*sizeof(Link *));
    links    = (Link     *)DXAllocateZero(2*nEdges*sizeof(Link));
    if (! forward || ! backward || ! combined ||
	! flags   || ! index    || ! links) goto error;
    

    if (pA)
    {
       pHandle = DXCreateArrayHandle(pA);
       if (! pHandle)
	   goto error;
    }

    sPtr = segs; nexte = 0;
    for (i = 0; i < nEdges; i++, sPtr ++)
    {
	int ip = sPtr->p;
	int iq = sPtr->q;

	links[nexte].link   = index[ip];
	links[nexte].tail   = iq;
	links[nexte].seg    = i;
	index[ip]           = links + nexte;
	nexte ++;

	links[nexte].link = index[iq];
	links[nexte].tail = ip;
	links[nexte].seg  = i;
	index[iq]         = links + nexte;

	nexte ++;
    }

    vol = 0;
    for (i = 0; i < nPts; )
    {
	 nf = nb = 0;

	 for (link = index[i]; link; link = link->link)
	     if (flags[link->seg] == 0)
		 break;
	
	if (! link)
	{
	    i++;
	    continue;
	}

	if (segs[link->seg].p == i)
	    reverse = 1;
	else 
	    reverse = 0;

	/*
	 * start forward chain
	 */
	for (done = 0; ! done; done = (link == 0))
	{
	    flags[link->seg] = 1;
	    forward[nf++] = link->tail;

	    for (link = index[link->tail]; link; link = link->link)
		if (flags[link->seg] == 0)
		    break;
	}

	/*
	 * If there's another unused segment incident on this vertex,
	 * trace a backward chain.
	 */
	for (link = index[i]; link; link = link->link)
	    if (flags[link->seg] == 0)
		break;
	
	if (link)
	{
	    for (done = 0; ! done; done = (link == 0))
	    {
		backward[nb++] = link->tail;
		flags[link->seg]  = 1;

		for (link = index[link->tail]; link; link = link->link)
		    if (flags[link->seg] == 0)
			break;
	    }

	    for (j = 0; j < nb; j++)
		combined[j] = backward[(nb-1)-j];

	    for (j = 0; j < nf; j++)
		combined[nb+j] = forward[j];

	    list = combined;
	    nf += nb;
	}
	else
	{
	    i++;
	    list = forward;
	}

	if (list[0] == list[nf-1])
	    nf -= 1;

	if (points)
	{
	    p = points + list[0];
	    r = points + list[1];
	}
	else
	{
	    p = (Vector *)DXCalculateArrayEntry(pHandle,list[0],(Pointer)&pBuf);
	    r = (Vector *)DXCalculateArrayEntry(pHandle,list[1],(Pointer)&rBuf); 
	}

	vector_subtract(r, p, &vrp);
	vector_subtract(&origin, p, &vsp);
	for (j = 1; j < nf - 1; j++)
	{
	    vqp = vrp;
	    if (points)
	    {
		/* q = r; */
		r = points + list[j+1];
	    }
	    else
	    {
		/* q    = r; */
		qBuf = rBuf;
		r = (Vector *)DXCalculateArrayEntry(pHandle,
						list[j+1],(Pointer)&rBuf); 
	    }

	    vector_subtract(r, p, &vrp);
	    vector_cross(&vqp, &vrp, &cross);

	    if (reverse)
		vol -= vector_dot(&cross, &vsp);
	    else
		vol += vector_dot(&cross, &vsp);
	}
    }

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    DXFree((Pointer)forward);
    DXFree((Pointer)backward);
    DXFree((Pointer)combined);
    DXFree((Pointer)flags);
    DXFree((Pointer)index);
    DXFree((Pointer)links);
	
    *measure = vol / 6.0;
    return OK;

error:

    if (pHandle)
	DXFreeArrayHandle(pHandle);
    DXFree((Pointer)forward);
    DXFree((Pointer)backward);
    DXFree((Pointer)combined);
    DXFree((Pointer)flags);
    DXFree((Pointer)index);
    DXFree((Pointer)links);
    return ERROR;
}

static float primes[] =
    { 143669821.0, 45497241659.0, 1455799321.0};

static PseudoKey
pHash2(Key key)
{
    float *point = (float *)(((HashElement2 *)key)->point);
    long l, k;
    int i;
    float j;

    l = 0;
    for (i = 0; i < 2; i++)
    {
	j = primes[i] * point[i];
	k = *(int *)&j;
	k = (((k ^ (k >> 8))) ^ (k >> 16)) ^ (k >> 24);

	l += k;
    }

    return (PseudoKey)l;
}

static int
pCmp2(Key k0, Key k1)
{
    float *p0 = (float *)(((HashElement2 *)k0)->point);
    float *p1 = (float *)(((HashElement2 *)k1)->point);

    if (*p0++ != *p1++) return 1;
    if (*p0++ != *p1++) return 1;
    return 0;
}

static PseudoKey
pHash3(Key key)
{
    float *point = (float *)&(((HashElement3 *)key)->point);
    long l, k;
    int i;
    float j;

    l = 0;
    for (i = 0; i < 3; i++)
    {
	j = primes[i] * point[i];
	k = *(int *)&j;
	k = (((k ^ (k >> 8))) ^ (k >> 16)) ^ (k >> 24);

	l += k;
    }

    return (PseudoKey)l;
}

static int
pCmp3(Key k0, Key k1)
{
    float *p0 = (float *)&(((HashElement3 *)k0)->point);
    float *p1 = (float *)&(((HashElement3 *)k1)->point);

    if (*p0++ != *p1++) return 1;
    if (*p0++ != *p1++) return 1;
    if (*p0++ != *p1++) return 1;
    return 0;
}

static Error
Lines_Area(Field f, Segments *s, float *measure)
{
    Segments seg;

    if (DXEmptyField(f))
    {
	seg.nSegments   = 0;
	seg.connections = NULL;
	seg.positions   = NULL;
	*measure = 0.0;
	return OK;
    }
    else
    {
	seg.positions = (Array)DXGetComponentValue(f, "positions");
	seg.connections = (Array)DXGetComponentValue(f, "connections");
	DXGetArrayInfo(seg.connections,
			&(seg.nSegments), NULL, NULL, NULL, NULL);

	if (s)
	{
	    *s = seg;
	    *measure = 0.0;
	    return OK;
	}
	else 
	{
	    return Loop_Area(1, &seg, measure);
	}
    }
}

static Error
Loop_Area(int nParts, Segments *segs, float *measure)
{
    int i, j;
    int *forward = NULL, *backward = NULL, *combined = NULL, *list;
    Link  **index = NULL, *links = NULL, *link = NULL;
    ubyte *flags = NULL;
    int nexte, nf, nb, done;
    float area;
    int nEdges, nPts, nDim;
    float *points = NULL;
    ArrayHandle pH = NULL, cH = NULL;
    HashTable hash = NULL;
    HashElement3 *hPtr;

    *measure = 0.0;

    if (nParts == 0)
	return OK;
    
    nEdges = 0;
    for (i = 0; i < nParts; i++)
	nEdges += segs[i].nSegments;
    
    if (nEdges == 0)
	return OK;

    DXGetArrayInfo(segs[0].positions, NULL, NULL, NULL, NULL, &nDim);

    if (nDim == 3)
	hash = DXCreateHash(sizeof(HashElement3), pHash3, pCmp3);
    else if (nDim == 2)
	hash = DXCreateHash(sizeof(HashElement2), pHash2, pCmp2);
    else
    {
	DXSetError(ERROR_DATA_INVALID, 
		"calculating area requires 2 or 3-D positions");
	goto error;
    }

    if (! hash)
	goto error;

    /*
     * No more than twice as many points as segments
     */
    index    = (Link    **)DXAllocateZero(2*nEdges*sizeof(Link *));
    links    = (Link     *)DXAllocateZero(2*nEdges*sizeof(Link));
    if (! index || ! links)
	goto error;
    
    /*
     * For each partition.  For each line segment within the 
     * partition, hash the vertices and create a new line segment
     * using the hashed vertex indices.  Then add this segment to
     * the index and links tables 
     */
    nPts = 0; nexte = 0; 
    for (i = 0; i < nParts; i++)
    {
	pH = DXCreateArrayHandle(segs[i].positions);
	if (! pH)
	    goto error;
	
	cH = DXCreateArrayHandle(segs[i].connections);
	if (! cH)
	    goto error;
	
	for (j = 0; j < segs[i].nSegments; j++)
	{
	    Line lBuf, *l=NULL;
	    float *p, *q;
	    float pBuf[3], qBuf[3];
	    int ip, iq;

	    l = (Line *) DXIterateArray(cH, j, (Pointer)l, (Pointer)&lBuf);

	    p = (float *)DXCalculateArrayEntry(pH, l->p, (Pointer)pBuf);
	    q = (float *)DXCalculateArrayEntry(pH, l->q, (Pointer)qBuf);

	    if (nDim == 3)
	    {
		HASHPOINT3(p, ip);
		HASHPOINT3(q, iq);
	    }
	    else
	    {
		HASHPOINT2(p, ip);
		HASHPOINT2(q, iq);
	    }

	    links[nexte].link   = index[ip];
	    links[nexte].tail   = iq;
	    links[nexte].seg    = nexte>>1;
	    index[ip]           = links + nexte;
	    nexte ++;

	    links[nexte].link = index[iq];
	    links[nexte].tail = ip;
	    links[nexte].seg  = nexte>>1;
	    index[iq]         = links + nexte;
	    nexte ++;
	}

	DXFreeArrayHandle(cH); cH = NULL;
	DXFreeArrayHandle(pH); pH = NULL;
    }

    /*
     * Create a directly-addressable points table
     */
    points = (float *)DXAllocate(nPts*nDim*sizeof(float));
    if (! points)
	goto error;
    
    DXInitGetNextHashElement(hash);
    while (NULL != (hPtr = (HashElement3 *)DXGetNextHashElement(hash)))
	memcpy(points+(hPtr->cindex*nDim), &hPtr->point, nDim*sizeof(float));
    
    DXDestroyHash(hash);
    hash = NULL;
	
    /*
     * Now find the connected loops. For each, compute the area,
     * triangulating from the first vertex of the chain.  If the 
     * loop is closed, back off the end.  Thus the last link is
     * missing whether or not the loop is closed.
     */
    forward  = (int      *)DXAllocate((nEdges+2)*sizeof(int));
    backward = (int      *)DXAllocate((nEdges+2)*sizeof(int));
    combined = (int      *)DXAllocate((nEdges+2)*sizeof(int));
    flags    = (ubyte    *)DXAllocateZero((nEdges+2)*sizeof(ubyte));
    if (! forward || ! backward || ! combined || ! flags)
	goto error;

    area = 0;
    for (i = 0; i < nPts; )
    {
	 nf = nb = 0;

	 for (link = index[i]; link; link = link->link)
	     if (flags[link->seg] == 0)
		 break;
	
	if (! link)
	{
	    i++;
	    continue;
	}

	/*
	 * start forward chain
	 */
	for (done = 0; ! done; done = (link == 0))
	{
	    flags[link->seg] = 1;
	    forward[nf++] = link->tail;

	    for (link = index[link->tail]; link; link = link->link)
		if (flags[link->seg] == 0)
		    break;
	}

	/*
	 * If there's another unused segment incident on this vertex,
	 * trace a backward chain.
	 */
	for (link = index[i]; link; link = link->link)
	    if (flags[link->seg] == 0)
		break;
	
	if (link)
	{
	    for (done = 0; ! done; done = (link == 0))
	    {
		backward[nb++] = link->tail;
		flags[link->seg]  = 1;

		for (link = index[link->tail]; link; link = link->link)
		    if (flags[link->seg] == 0)
			break;
	    }

	    for (j = 0; j < nb; j++)
		combined[j] = backward[(nb-1)-j];

	    for (j = 0; j < nf; j++)
		combined[nb+j] = forward[j];

	    list = combined;
	    nf += nb;
	}
	else
	{
	    i++;
	    list = forward;
	}

	if (list[0] == list[nf-1])
	    nf -= 1;

	if (nDim == 2)
	{
	    float *p, *q;
	    float v0[2], v1[2];

	    p = points + 2*list[0];
	    q = points + 2*list[1];

	    v1[0] = q[0] - p[0];
	    v1[1] = q[1] - p[1];

	    for (j = 1; j < nf-1; j++)
	    {
		v0[0] = v1[0]; v0[1] = v1[1];
		q = points + 2*list[j+1];
		v1[0] = q[0] - p[0];
		v1[1] = q[1] - p[1];
		area += v0[0]*v1[1] - v0[1]*v1[0];
	    }
	}
	else
	{
	    Vector *p, *q;
	    Vector v0, v1, cross;

	    p = ((Vector *)points) + list[0];
	    q = ((Vector *)points) + list[1];

	    vector_subtract(q, p, &v1);

	    for (j = 1; j < nf-1; j++)
	    {
		v0 = v1;
		q = ((Vector *)points)+list[j+1];
		vector_subtract(q, p, &v1);
		vector_cross(&v0, &v1, &cross);
		area += vector_length(&cross);
	    }
	}
    }

    DXFree((Pointer)points);
    DXFree((Pointer)forward);
    DXFree((Pointer)backward);
    DXFree((Pointer)combined);
    DXFree((Pointer)flags);
    DXFree((Pointer)index);
    DXFree((Pointer)links);
	
    *measure = area / 2.0;
    return OK;

error:

    if (hash)
	DXDestroyHash(hash);
    if (pH)
	DXFreeArrayHandle(pH);
    if (cH)
	DXFreeArrayHandle(cH);
    DXFree((Pointer)points);
    DXFree((Pointer)forward);
    DXFree((Pointer)backward);
    DXFree((Pointer)combined);
    DXFree((Pointer)flags);
    DXFree((Pointer)index);
    DXFree((Pointer)links);
    return ERROR;
}

static Error
Quads_Volume(Field field, Segments *segments, float *measure)
{
    Vector origin;
    int  i;
    Array gcA = NULL, cA, pA, nA = NULL;
    int *n, *nbrs;
    int nPts, nQuads, nEdges, nDim;
    Vector *p, *q, *r, *s, vqp, vrp, vop, cross;
    int quadBuf[4], *quad = NULL;
    float area = 0.0;
    Line *segs = NULL;
    ArrayHandle cHandle = NULL, pHandle = NULL;
    int grown, regGrid;
    int oSize[2], gSize[2];
    int oMO[2], gMO[2];

    *measure = 0.0;

    if (DXEmptyField(field))
	return OK;

    origin.x = origin.y = origin.z = 0.0;

    cA = (Array)DXGetComponentValue(field, "original connections");
    if (cA)
    {
	grown = 1;
	pA = (Array)DXGetComponentValue(field, "original positions");
	if (! pA)
	    goto error;

	gcA = (Array)DXGetComponentValue(field, "connections");
    }
    else
    {
	/*
	 * Then not the result of Grow
	 */
	grown = 0;

	cA = (Array)DXGetComponentValue(field, "connections");
	if (! cA)
	    goto error;

	pA = (Array)DXGetComponentValue(field, "positions");
	if (! pA)
	    goto error;
    }

    DXGetArrayInfo(cA, &nQuads, NULL, NULL, NULL, NULL);
    
    DXGetArrayInfo(pA, &nPts, NULL, NULL, NULL, &nDim);
    if (nDim != 3)
    {
	DXSetError(ERROR_DATA_INVALID, "#10342", "positions");
	goto error;
    }

    /*
     * If the grid was irregular, we need to compute neighbors to find 
     * unshares edges.
     */
    if (! DXQueryGridConnections(cA, NULL, oSize))
    {
	nA = DXNeighbors(field);
	if (! nA)
	    goto error;
	n = nbrs = (int *)DXGetArrayData(nA);
	regGrid = 0;
    }
    else
    {
	regGrid = 1;
	n = nbrs = NULL;
    }

    cHandle = DXCreateArrayHandle(cA);
    if (! cHandle)
       goto error;

    pHandle = DXCreateArrayHandle(pA);
    if (! pHandle)
	goto error;

    DXGetArrayInfo(cA, &nQuads, NULL, NULL, NULL, NULL);

    /*
     * For each triangle, divide into two triangles and compute volumes.
     */
    area = 0; nEdges = 0;
    for (i = 0; i < nQuads; i++)
    {
	Vector pBuf, qBuf, rBuf, sBuf;

	quad = (int *)DXIterateArray(cHandle, i, quad, (Pointer)quadBuf);

	p = (Vector *)DXCalculateArrayEntry(pHandle, quad[0], (Pointer)&pBuf);
	q = (Vector *)DXCalculateArrayEntry(pHandle, quad[1], (Pointer)&qBuf);
	r = (Vector *)DXCalculateArrayEntry(pHandle, quad[2], (Pointer)&rBuf);
	s = (Vector *)DXCalculateArrayEntry(pHandle, quad[3], (Pointer)&sBuf);

	vector_subtract(q, p, &vqp);
	vector_subtract(r, p, &vrp);
	vector_subtract(&origin, p, &vop);
	vector_cross(&vrp, &vqp, &cross);
	area += vector_dot(&cross, &vop);

	vector_subtract(r, s, &vrp);
	vector_subtract(q, s, &vqp);
	vector_subtract(&origin, s, &vop);
	vector_cross(&vqp, &vrp, &cross);
	area += vector_dot(&cross, &vop);

	if (!regGrid)
	{
	    if (*n++ < 0) nEdges++;
	    if (*n++ < 0) nEdges++;
	    if (*n++ < 0) nEdges++;
	    if (*n++ < 0) nEdges++;
	}
    }
    area = area / 6.0;
    
    /*
     * If the grid was regular, then we need to determine the number
     * of unshared edges from the mesh info.  If it was regular, then
     * we counted in the above loop.
     */
    if (regGrid)
    {
	if (grown)
	{
	    if (! DXGetMeshOffsets((MeshArray)gcA, gMO))
		goto error;

	    if (! DXQueryGridConnections(gcA, NULL, gSize))
		goto error;

	    if (! DXGetMeshOffsets((MeshArray)cA, oMO))
		goto error;
	    
	    if (gMO[0] == oMO[0])
		nEdges += (oSize[1] - 1);
	    if (gMO[1] == oMO[1])
		nEdges += (oSize[0] - 1);
	    if (gMO[0]+gSize[0] == oMO[0]+oSize[0])
		nEdges += (oSize[1] - 1);
	    if (gMO[1]+gSize[1] == oMO[1]+oSize[1])
		nEdges += (oSize[0] - 1);
	}
	else
	    nEdges += 2*(oSize[0] - 1) + 2*(oSize[1] - 1);
    }
	
    /*
     * If there were external edges, then we need to create a segment list.
     * If the grid is regular and grown, we determine the external edges by
     * comparing the grid boundary against the grown grid boundary.
     * If the grid is regular and *NOT* grown, then all outer edges are
     * external.  Finally, if the grid is irregular, we have to go
     * through the neighbors component.
     */
    if (nEdges)
    {
	Line *s;

	segs = (Line *)DXAllocate(nEdges*sizeof(Line));
	if (! segs)
	    goto error;
	
	s = segs;

	if (regGrid)
	{
	    if (grown)
	    {
		if (gMO[1] == oMO[1])
		    for (i = 0; i < oSize[0]-1; i++, s++)
		    {
			s->p = i*oSize[1];
			s->q = s->p + oSize[1];
		    }

		if (gMO[1]+gSize[1] == oMO[1]+oSize[1])
		    for (i = 0; i < oSize[0]-1; i++, s++)
		    {
			s->q = i*oSize[1] + (oSize[1] - 1);
			s->p = s->q + oSize[1];
		    }

		if (gMO[0] == oMO[0])
		    for (i = 0; i < oSize[1]-1; i++, s++)
		    {
			s->q = i;
			s->p = s->q + 1;
		    }

		if (gMO[0]+gSize[0] == oMO[0]+oSize[0])
		    for (i = 0; i < oSize[1]-1; i++, s++)
		    {
			s->p = (oSize[0]-1)*oSize[1] + i;
			s->q = s->p + 1;
		    }
	    }
	    else
	    {
		for (i = 0; i < oSize[0]-1; i++)
		{
		    s->p = i*oSize[1];
		    s->q = s->p + oSize[1];
		    s++;
		    s->q = i*oSize[1] + (oSize[1]-1);
		    s->p = s->q + oSize[1];
		    s++;
		}

		for (i = 0; i < oSize[1]-1; i++)
		{
		    s->q = i;
		    s->p = i+1;
		    s++;
		    s->p = (oSize[0]-1)*oSize[1] + i;
		    s->q = s->p+1;
		    s++;
		}
	    }
	}
	else
	{
	    n = nbrs;
	    for (i = 0; i < nQuads; i++)
	    {
		quad = (int *)DXIterateArray(cHandle, i, quad, (Pointer)quadBuf);

		if (*n++ < 0) {s->q = quad[0]; s->p = quad[1]; s++;};
		if (*n++ < 0) {s->q = quad[3]; s->p = quad[2]; s++;};
		if (*n++ < 0) {s->q = quad[2]; s->p = quad[0]; s++;};
		if (*n++ < 0) {s->q = quad[1]; s->p = quad[3]; s++;};
	    }
	}
    
	/*
	 * If this was an element of a composite field, we will need to
	 * accumulate the loops between all the composite field members.
	 * Otherwise, we can go ahead and add in the loop areas here.
	 */
	if (segments)
	{
	    segments->nSegments = nEdges;
	    segments->segments = (int *)segs;
	    segments->positions = pA;
	}
	else
	{
	    float tmp;

	    if (! Loop_Volume(nEdges, nPts, segs, NULL, pA, &tmp))
		goto error;

	    area += tmp;

	    DXFree((Pointer)segs);
	    segs = NULL;
	}
    }
    else
    {
	if (segments)
	{
	    segments->nSegments = 0;
	    segments->segments = NULL;
	    segments->positions = NULL;
	}
    }

    if (DXGetError() != ERROR_NONE)
	return ERROR;

    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (pHandle)
	DXFreeArrayHandle(pHandle);

    *measure = area;
    return OK;

error:
    if (cHandle)
	DXFreeArrayHandle(cHandle);
    if (pHandle)
	DXFreeArrayHandle(pHandle);
    DXFree((Pointer)segs);
    if (segments)
	segments->segments = NULL;
    
    return ERROR;
}

static Object
MeasureElements(Object object)
{
    Class class;

    class = DXGetObjectClass(object);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)object);

    switch(class)
    {
	case CLASS_FIELD:
	{
	    if (DXEmptyField((Field)object) ||
		(!DXGetComponentValue((Field)object, "faces") &&
		 !DXGetComponentValue((Field)object, "polylines") &&
		 !DXGetComponentValue((Field)object, "connections")))
	    {
		if (DXGetComponentValue((Field)object, "data"))
		    DXDeleteComponent((Field)object, "data");

		return object;
	    }
	    else
	    {
		return (Object)MeasureElements_Field((Field)object);
	    }
	}
	break;
	
	case CLASS_GROUP:
	case CLASS_COMPOSITEFIELD:
	case CLASS_MULTIGRID:
	case CLASS_SERIES:
	{
	    int    i;
	    Group  g = (Group)object;
	    Object child;
	    int    typed = (NULL != DXGetType(object, NULL, NULL, NULL, NULL));

	    for (i = 0; NULL != (child = DXGetEnumeratedMember(g, i, NULL)); i++)
		if (! MeasureElements(child))
		    return ERROR;
	    
	    if (typed)
		if (! DXSetGroupType(g, TYPE_FLOAT, CATEGORY_REAL, 0))
		    return ERROR;
	    
	    return object;
	}

	case CLASS_XFORM:
	{
	    Object child;

	    if (! DXGetXformInfo((Xform)object, &child, NULL))
		goto error;

	    return MeasureElements(child);
	}
	   
	case CLASS_CLIPPED:
	{
	    Object child;

	    if (! DXGetClippedInfo((Clipped)object, &child, NULL))
		goto error;
	    
	    return MeasureElements(child);
	}

	default:
	    DXSetError(ERROR_BAD_PARAMETER, "input must be field or group");
	    return NULL;
    }
    
error:
    return NULL;
}

static Error  line_1d(ArrayHandle, ArrayHandle, float *, int);
static Error  line_2d(ArrayHandle, ArrayHandle, float *, int);
static Error  line_3d(ArrayHandle, ArrayHandle, float *, int);
static Error   tri_2d(ArrayHandle, ArrayHandle, float *, int);
static Error   tri_3d(ArrayHandle, ArrayHandle, float *, int);
static Error  quad_2d(ArrayHandle, ArrayHandle, float *, int);
static Error  quad_3d(ArrayHandle, ArrayHandle, float *, int);
static Error tetra_3d(ArrayHandle, ArrayHandle, float *, int);
static Error  cube_3d(ArrayHandle, ArrayHandle, float *, int);

typedef enum {
    dep_connections,
    dep_faces,
    dep_polylines
} dependency;

static Field
MeasureElements_Field(Field f)
{
    char        *etype;
    Object      attr;
    Array       pA, cA, dA = NULL;
    int         pd, nm;
    ArrayHandle pH = NULL, cH = NULL;
    PFE		mth;
    dependency  dep;

    pA = (Array)DXGetComponentValue(f, "positions");
    DXGetArrayInfo(pA, NULL, NULL, NULL, NULL, &pd);

    cA = (Array)DXGetComponentValue(f, "connections");
    if (cA) dep = dep_connections;
    else
    {
	cA = (Array)DXGetComponentValue(f, "faces");
	if (cA) dep = dep_faces;
	else
	{
	    cA = (Array)DXGetComponentValue(f, "polylines");
	    if (cA) dep = dep_polylines;
	    else
	    {
		DXSetError(ERROR_MISSING_DATA,
			"field contains no connections, faces or polylines");
		goto error;
	    }
	}
    }
		
    DXGetArrayInfo(cA, &nm, NULL, NULL, NULL, NULL);

    dA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    if (! dA)
	goto error;
    
    if (! DXAddArrayData(dA, 0, nm, NULL))
	goto error;

    if (dep == dep_connections)
    {
	pH = DXCreateArrayHandle(pA);
	cH = DXCreateArrayHandle(cA);
	if (!pH || !cH)
	    goto error;
    
	attr = DXGetComponentAttribute(f, "connections", "element type");
	if (! attr || DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "element type attribute");
	    return ERROR;
	}

	etype = (char *)DXGetString((String)attr);

	if (! strcmp(etype, "lines"))
	{
	    switch(pd)
	    {
		case 1: mth = line_1d; break;
		case 2: mth = line_2d; break;
		case 3: mth = line_3d; break;
		default:
		{
		    DXSetError(ERROR_DATA_INVALID,
			    "measuring lines requires 1, 2, or 3-D positions");
		    goto error;
		}
	    }
	}
	else if (! strcmp(etype, "triangles"))
	{
	    switch(pd)
	    {
		case 2: mth = tri_2d; break;
		case 3: mth = tri_3d; break;
		default:
		{
		    DXSetError(ERROR_DATA_INVALID,
			    "measuring triangles requires 2 or 3-D positions");
		    goto error;
		}
	    }
	}
	else if (! strcmp(etype, "quads"))
	{
	    switch(pd)
	    {
		case 2: mth = quad_2d; break;
		case 3: mth = quad_3d; break;
		default:
		{
		    DXSetError(ERROR_DATA_INVALID,
			    "measuring quads requires 2 or 3-D positions");
		    goto error;
		}
	    }
	}
	else if (! strcmp(etype, "tetrahedra"))
	{
	    switch(pd)
	    {
		case 3: mth = tetra_3d; break;
		default:
		{
		    DXSetError(ERROR_DATA_INVALID,
			    "measuring tetras requires 3-D positions");
		    goto error;
		}
	    }
	}
	else if (! strcmp(etype, "cubes"))
	{
	    switch(pd)
	    {
		case 3: mth = cube_3d; break;
		default:
		{
		    DXSetError(ERROR_DATA_INVALID,
			    "measuring cubes requires 3-D positions");
		    goto error;
		}
	    }
	}
	else
	{
	    DXSetError(ERROR_DATA_INVALID,
		    "unsupported element type: %s", etype);
	    goto error;
	}

	if (! (*mth)(pH, cH, (float *)DXGetArrayData(dA), nm))
	    goto error;
    
	DXFreeArrayHandle(pH);
	pH = NULL;

	DXFreeArrayHandle(cH);
	cH = NULL;
    
	if (! DXSetStringAttribute((Object)dA, "dep", "connections"))
	    goto error;
    }
    else if (dep == dep_faces)
    {
	if (! FLE_Area_Elements(f, (float *)DXGetArrayData(dA)))
	    goto error;
    
	if (! DXSetStringAttribute((Object)dA, "dep", "faces"))
	    goto error;
    }
    else 
    {
	if (! Polyline_Length_Elements(f, (float *)DXGetArrayData(dA)))
	    goto error;
    
	if (! DXSetStringAttribute((Object)dA, "dep", "polylines"))
	    goto error;
    }
    
    if (DXGetComponentValue(f, "data"))
	DXDeleteComponent(f, "data");
    
    if (DXGetComponentValue(f, "data statistics"))
	DXDeleteComponent(f, "data statistics");
    
    if (! DXSetComponentValue(f, "data", (Object)dA))
	goto error;
    dA = NULL;
    
    if (! DXEndField(f))
	goto error;
    
    return f;

error:
    if (pH)
	DXFreeArrayHandle(pH);
    if (cH)
	DXFreeArrayHandle(cH);
    if (dA)
	DXDelete((Object)dA);
    
    return NULL;
}

static Error 
line_1d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[2];
    float *pPtr, pBuf[1];
    float *qPtr, qBuf[1];
    float l;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);

	l = *pPtr - *qPtr;

	*m++ = (l < 0) ? -l : l;
    }

    return OK;
}

static Error
line_2d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[2];
    float *pPtr, pBuf[2];
    float *qPtr, qBuf[2];
    float xdiff, ydiff;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);

	xdiff = pPtr[0] - qPtr[0];
	ydiff = pPtr[1] - qPtr[1];

	*m++ = sqrt(xdiff*xdiff + ydiff*ydiff);
    }

    return OK;
}

static Error
line_3d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[2];
    float *pPtr, pBuf[3];
    float *qPtr, qBuf[3];
    float xdiff, ydiff, zdiff;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);

	xdiff = pPtr[0] - qPtr[0];
	ydiff = pPtr[1] - qPtr[1];
	zdiff = pPtr[2] - qPtr[2];

	*m++ = sqrt(xdiff*xdiff + ydiff*ydiff + zdiff*zdiff);
    }

    return OK;
}

static Error
tri_2d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[3];
    float *pPtr, pBuf[2];
    float *qPtr, qBuf[2];
    float *rPtr, rBuf[2];
    float v0[2], v1[2];
    float a;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(pH, cPtr[2], (Pointer)rBuf);

	v0[0] = qPtr[0] - pPtr[0];
	v0[1] = qPtr[1] - pPtr[1];

	v1[0] = rPtr[0] - pPtr[0];
	v1[1] = rPtr[1] - pPtr[1];

	a = (v0[0]*v1[1] - v0[1]*v1[0]) * 0.5;

	*m++ = (a < 0) ? -a : a;
    }

    return OK;
}

static Error
tri_3d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[3];
    float *pPtr, pBuf[3];
    float *qPtr, qBuf[3];
    float *rPtr, rBuf[3];
    Vector v0, v1, cross;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(pH, cPtr[2], (Pointer)rBuf);

	v0.x = qPtr[0] - pPtr[0];
	v0.y = qPtr[1] - pPtr[1];
	v0.z = qPtr[2] - pPtr[2];

	v1.x = rPtr[0] - pPtr[0];
	v1.y = rPtr[1] - pPtr[1];
	v1.z = rPtr[2] - pPtr[2];

	vector_cross(&v0, &v1, &cross);
	*m++ = vector_length(&cross) * 0.5;
    }

    return OK;
}

static Error
quad_2d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[4];
    float *pPtr, pBuf[2];
    float *qPtr, qBuf[2];
    float *rPtr, rBuf[2];
    float *sPtr, sBuf[2];
    float v0[2], v1[2];
    float a, b;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(pH, cPtr[2], (Pointer)rBuf);
	sPtr = (float *)DXGetArrayEntry(pH, cPtr[3], (Pointer)sBuf);

	v0[0] = qPtr[0] - pPtr[0];
	v0[1] = qPtr[1] - pPtr[1];

	v1[0] = rPtr[0] - pPtr[0];
	v1[1] = rPtr[1] - pPtr[1];

	a = v0[0]*v1[1] - v0[1]*v1[0];
	a = (a < 0) ? -a : a;

	v0[0] = qPtr[0] - sPtr[0];
	v0[1] = qPtr[1] - sPtr[1];

	v1[0] = rPtr[0] - sPtr[0];
	v1[1] = rPtr[1] - sPtr[1];

	b = v0[0]*v1[1] - v0[1]*v1[0];
	b = (b < 0) ? -b : b;

	*m++ = (a+b) * 0.5;
    }

    return OK;
}

static Error
quad_3d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[4];
    float *pPtr, pBuf[3];
    float *qPtr, qBuf[3];
    float *rPtr, rBuf[3];
    float *sPtr, sBuf[3];
    Vector v0, v1, cross;
    float  a, b;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(pH, cPtr[2], (Pointer)rBuf);
	sPtr = (float *)DXGetArrayEntry(pH, cPtr[3], (Pointer)sBuf);

	v0.x = qPtr[0] - pPtr[0];
	v0.y = qPtr[1] - pPtr[1];
	v0.z = qPtr[2] - pPtr[2];

	v1.x = rPtr[0] - pPtr[0];
	v1.y = rPtr[1] - pPtr[1];
	v1.z = rPtr[2] - pPtr[2];

	vector_cross(&v0, &v1, &cross);
	a = vector_length(&cross);

	v0.x = qPtr[0] - sPtr[0];
	v0.y = qPtr[1] - sPtr[1];
	v0.z = qPtr[2] - sPtr[2];

	v1.x = rPtr[0] - sPtr[0];
	v1.y = rPtr[1] - sPtr[1];
	v1.z = rPtr[2] - sPtr[2];

	vector_cross(&v0, &v1, &cross);
	b = vector_length(&cross);

	*m++ = (a+b) * 0.5;
    }

    return OK;
}

#define TETRA(a, b, c, d, r)					\
{								\
    Vector v0, v1, v2, cross;					\
								\
    vector_subtract((Vector *)(a), (Vector *)(d), &v0);		\
    vector_subtract((Vector *)(b), (Vector *)(d), &v1);		\
    vector_subtract((Vector *)(c), (Vector *)(d), &v2);		\
    vector_cross(&v0, &v1, &cross);				\
    (r) = vector_dot(&cross, &v2);				\
    (r) = (r) > 0 ? (r) : -(r);					\
}

static Error
tetra_3d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[4];
    float *pPtr, pBuf[3];
    float *qPtr, qBuf[3];
    float *rPtr, rBuf[3];
    float *sPtr, sBuf[3];
    float  a;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(pH, cPtr[2], (Pointer)rBuf);
	sPtr = (float *)DXGetArrayEntry(pH, cPtr[3], (Pointer)sBuf);

	TETRA(pPtr, qPtr, rPtr, sPtr, a);
	*m++ = a/6.0;
    }

    return OK;
}

static Error
cube_3d(ArrayHandle pH, ArrayHandle cH, float *m, int n)
{
    int   *cPtr, cBuf[8];
    float *pPtr, pBuf[3];
    float *qPtr, qBuf[3];
    float *rPtr, rBuf[3];
    float *sPtr, sBuf[3];
    float *tPtr, tBuf[3];
    float *uPtr, uBuf[3];
    float *vPtr, vBuf[3];
    float *wPtr, wBuf[3];
    float  a, r;
    int   i;

    for (i = 0; i < n; i++)
    {
	cPtr = (int   *)DXGetArrayEntry(cH, i, (Pointer)cBuf);
	pPtr = (float *)DXGetArrayEntry(pH, cPtr[0], (Pointer)pBuf);
	qPtr = (float *)DXGetArrayEntry(pH, cPtr[1], (Pointer)qBuf);
	rPtr = (float *)DXGetArrayEntry(pH, cPtr[2], (Pointer)rBuf);
	sPtr = (float *)DXGetArrayEntry(pH, cPtr[3], (Pointer)sBuf);
	tPtr = (float *)DXGetArrayEntry(pH, cPtr[4], (Pointer)tBuf);
	uPtr = (float *)DXGetArrayEntry(pH, cPtr[5], (Pointer)uBuf);
	vPtr = (float *)DXGetArrayEntry(pH, cPtr[6], (Pointer)vBuf);
	wPtr = (float *)DXGetArrayEntry(pH, cPtr[7], (Pointer)wBuf);

	TETRA(qPtr, rPtr, wPtr, sPtr, r);

	TETRA(wPtr, rPtr, tPtr, vPtr, a);
	r += a;
	TETRA(qPtr, wPtr, tPtr, uPtr, a);
	r += a;
	TETRA(qPtr, tPtr, rPtr, pPtr, a);
	r += a;
	TETRA(qPtr, rPtr, tPtr, wPtr, a);
	r += a;
	*m++ = r/6.0;
    }

    return OK;
}

Error
_dxfLoopNormal(int *indices, ArrayHandle pH, float *pts,
			int nV, int nD, Vector *normal)
{
    int i;
    float *n = (float *)normal;
    float *p, *q;
    float buf0[3], buf1[3];

    if (!pH && !pts)
    {
	DXSetError(ERROR_INTERNAL, 
		"_dxfLoopNormal called with neither a points array or handle");
	return ERROR;
    }

    n[0] = n[1] = n[2] = 0.0;

    if (nD == 2)
    {
	if (pH)
	    p = (float *)DXGetArrayEntry(pH, indices[(nV-1)], (Pointer)buf1);
	else
	    p = pts + (nV-1)*nD;

	for (i = 0; i < nV; i++)
	{
	    if (pH)
		q = (float *)DXGetArrayEntry(pH, indices[i],
				(i & 0x1) ? (Pointer)buf1 : (Pointer)buf0);
	    else
		q = pts + i*nD;


	    n[2] += (p[0] - q[0])*(p[1] + q[1]);
	    p = q;
	}
    }
    else
    {
	if (pH)
	    p = (float *)DXGetArrayEntry(pH, indices[(nV-1)], (Pointer)buf1);
	else
	    p = pts + (nV-1)*nD;

	for (i = 0; i < nV; i++)
	{
	    if (pH)
		q = (float *)DXGetArrayEntry(pH, indices[i],
				(i & 0x1) ? (Pointer)buf1 : (Pointer)buf0);
	    else
		q = pts + i*nD;

	    n[0] += (p[1] - q[1])*(p[2] + q[2]);
	    n[1] += (p[2] - q[2])*(p[0] + q[0]);
	    n[2] += (p[0] - q[0])*(p[1] + q[1]);
	    p = q;
	}
    }

    return OK;
}

static Error
FLE_Area(Field field, Segments *segs, float *measure)
{
    Array faces, loops, edges, positions;
    int   *faceData, *loopData, *edgeData;
    ArrayHandle phandle = NULL;
    int nFaces, nLoops, nEdges, nDim;
    float area = 0;
    Type t;
    Category c;
    int rank, shape[32];
    int face;
    Vector fN, lN;
    InvalidComponentHandle ich = NULL;

    if (DXEmptyField(field))
	goto done;
    
    faces = (Array)DXGetComponentValue(field, "faces");
    if (! faces)
    {
	DXSetError(ERROR_MISSING_DATA, "faces");
	goto error;
    }

    DXGetArrayInfo(faces, &nFaces, &t, &c, &rank, shape);

    if (nFaces == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"faces must be real integer scalars");
	goto error;
    }

    loops = (Array)DXGetComponentValue(field, "loops");
    if (! loops)
    {
	DXSetError(ERROR_MISSING_DATA, "loops");
	goto error;
    }

    DXGetArrayInfo(loops, &nLoops, &t, &c, &rank, shape);

    if (nLoops == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"loops must be real integer scalars");
	goto error;
    }

    edges = (Array)DXGetComponentValue(field, "edges");
    if (! edges)
    {
	DXSetError(ERROR_MISSING_DATA, "edges");
	goto error;
    }

    DXGetArrayInfo(edges, &nEdges, &t, &c, &rank, shape);

    if (nEdges == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"edges must be real integer scalars");
	goto error;
    }

    positions = (Array)DXGetComponentValue(field, "positions");
    if (! positions)
    {
	DXSetError(ERROR_MISSING_DATA, "positions");
	goto error;
    }

    DXGetArrayInfo(positions, NULL, &t, &c, &rank, &nDim);

    if (t != TYPE_FLOAT || c != CATEGORY_REAL
	|| rank != 1 || (nDim != 2 && nDim != 3))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"positions must be real float 2- or 3-vectors");
	goto error;
    }

    phandle = DXCreateArrayHandle(positions);
    if (! phandle)
	goto error;

    faceData = (int *)DXGetArrayData(faces);
    loopData = (int *)DXGetArrayData(loops);
    edgeData = (int *)DXGetArrayData(edges);

    if (DXGetComponentValue(field, "invalid faces"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "faces");
	if (ich == NULL)
	    goto error;
    }

    for (face = 0; face < nFaces; face++)
    {
	int fLoop = faceData[face];
	int lLoop = (face == nFaces-1) ? nLoops : faceData[face+1];
	int loop;

	if (ich && DXIsElementInvalid(ich, face))
	    continue;

	for (loop = fLoop; loop < lLoop; loop++)
	{
	    int fEdge = loopData[loop];
	    int lEdge = (loop == nLoops-1) ? nEdges : loopData[loop+1];

	    if (loop == fLoop)
	    {
		if (! _dxfLoopNormal(edgeData+fEdge, phandle,
					NULL, (lEdge-fEdge), nDim, &fN))
		    goto error;

		area += vector_length(&fN);
	    }
	    else
	    {
		if (! _dxfLoopNormal(edgeData+fEdge, phandle,
					NULL, (lEdge-fEdge), nDim, &lN))
		    goto error;

		if ((fN.x*lN.x + fN.y*lN.y + fN.z*lN.z) > 0)
		    area += vector_length(&lN);
		else
		    area -= vector_length(&lN);
	    }
	}
    }

done:

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    *measure = fabs(0.5*area);

    return OK;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    return ERROR;
}


static Error
FLE_Area_Elements(Field field, float *measurements)
{
    Array faces, loops, edges, positions;
    int   *faceData, *loopData, *edgeData;
    ArrayHandle phandle = NULL;
    int nFaces, nLoops, nEdges, nDim;
    Type t;
    Category c;
    int rank, shape[32];
    int face;
    Vector fN, lN;
    InvalidComponentHandle ich = NULL;

    if (DXEmptyField(field))
	goto done;
    
    faces = (Array)DXGetComponentValue(field, "faces");
    if (! faces)
    {
	DXSetError(ERROR_MISSING_DATA, "faces");
	goto error;
    }

    DXGetArrayInfo(faces, &nFaces, &t, &c, &rank, shape);

    if (nFaces == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"faces must be real integer scalars");
	goto error;
    }

    loops = (Array)DXGetComponentValue(field, "loops");
    if (! loops)
    {
	DXSetError(ERROR_MISSING_DATA, "loops");
	goto error;
    }

    DXGetArrayInfo(loops, &nLoops, &t, &c, &rank, shape);

    if (nLoops == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"loops must be real integer scalars");
	goto error;
    }

    edges = (Array)DXGetComponentValue(field, "edges");
    if (! edges)
    {
	DXSetError(ERROR_MISSING_DATA, "edges");
	goto error;
    }

    DXGetArrayInfo(edges, &nEdges, &t, &c, &rank, shape);

    if (nEdges == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"edges must be real integer scalars");
	goto error;
    }

    positions = (Array)DXGetComponentValue(field, "positions");
    if (! positions)
    {
	DXSetError(ERROR_MISSING_DATA, "positions");
	goto error;
    }

    DXGetArrayInfo(positions, NULL, &t, &c, &rank, &nDim);

    if (t != TYPE_FLOAT || c != CATEGORY_REAL
	|| rank != 1 || (nDim != 2 && nDim != 3))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"positions must be real float 2- or 3-vectors");
	goto error;
    }

    phandle = DXCreateArrayHandle(positions);
    if (! phandle)
	goto error;

    faceData = (int *)DXGetArrayData(faces);
    loopData = (int *)DXGetArrayData(loops);
    edgeData = (int *)DXGetArrayData(edges);

    if (DXGetComponentValue(field, "invalid faces"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "faces");
	if (ich == NULL)
	    goto error;
    }

    for (face = 0; face < nFaces; face++)
    {
	int fLoop = faceData[face];
	int lLoop = (face == nFaces-1) ? nLoops : faceData[face+1];
	int loop;
	float area = 0.0;

	if (ich && DXIsElementInvalid(ich, face))
	{
	    measurements[face] = 0.0;
	    continue;
	}

	for (loop = fLoop; loop < lLoop; loop++)
	{
	    int fEdge = loopData[loop];
	    int lEdge = (loop == nLoops-1) ? nEdges : loopData[loop+1];

	    if (loop == fLoop)
	    {
		if (! _dxfLoopNormal(edgeData+fEdge, phandle,
					NULL, (lEdge-fEdge), nDim, &fN))
		    goto error;

		area += vector_length(&fN);
	    }
	    else
	    {
		if (! _dxfLoopNormal(edgeData+fEdge, phandle,
					NULL, (lEdge-fEdge), nDim, &lN))
		    goto error;

		if ((fN.x*lN.x + fN.y*lN.y + fN.z*lN.z) > 0)
		    area += vector_length(&lN);
		else
		    area -= vector_length(&lN);
	    }
	}

	measurements[face] = fabs(0.5*area);
    }

done:

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);

    return OK;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    return ERROR;
}

static Error
Polyline_Length(Field field, Segments *segs, float *measure)
{
    Array polylines, edges, positions;
    int   *polylineData, *edgeData;
    ArrayHandle phandle = NULL;
    int nPolylines, nEdges, nDim;
    float length = 0;
    Type t;
    Category c;
    int rank, shape[32];
    int i, p, e;
    Pointer pbuf0=NULL, pbuf1=NULL;
    float *p0 = NULL, *p1 = NULL;
    InvalidComponentHandle ich = NULL;

    if (DXEmptyField(field))
	goto done;
    
    polylines = (Array)DXGetComponentValue(field, "polylines");
    if (! polylines)
    {
	DXSetError(ERROR_MISSING_DATA, "polylines");
	goto error;
    }

    DXGetArrayInfo(polylines, &nPolylines, &t, &c, &rank, shape);

    if (nPolylines == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"polylines must be real integer scalars");
	goto error;
    }

    edges = (Array)DXGetComponentValue(field, "edges");
    if (! edges)
    {
	DXSetError(ERROR_MISSING_DATA, "edges");
	goto error;
    }

    DXGetArrayInfo(edges, &nEdges, &t, &c, &rank, shape);

    if (nEdges == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"edges must be real integer scalars");
	goto error;
    }

    positions = (Array)DXGetComponentValue(field, "positions");
    if (! positions)
    {
	DXSetError(ERROR_MISSING_DATA, "positions");
	goto error;
    }

    DXGetArrayInfo(positions, NULL, &t, &c, &rank, &nDim);

    if (t != TYPE_FLOAT || c != CATEGORY_REAL
	|| rank != 1 || (nDim != 2 && nDim != 3))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"positions must be real float 2- or 3-vectors");
	goto error;
    }

    phandle = DXCreateArrayHandle(positions);
    if (! phandle)
	goto error;

    pbuf0 = DXAllocate(DXGetItemSize(positions));
    pbuf1 = DXAllocate(DXGetItemSize(positions));
    if (!pbuf0 || !pbuf1)
	goto error;

    polylineData = (int *)DXGetArrayData(polylines);
    edgeData = (int *)DXGetArrayData(edges);

    if (DXGetComponentValue(field, "invalid polylines"))
    {
	ich = DXCreateInvalidComponentHandle((Object)field, NULL, "polylines");
	if (ich == NULL)
	    goto error;
    }

    for (p = 0; p < nPolylines; p++)
    {
	int fEdge = polylineData[p];
	int lEdge = (p == nPolylines-1) ? nEdges : polylineData[p+1];

	if (ich && DXIsElementInvalid(ich, p))
	    continue;

	e = fEdge;
	p1 = (float *)DXCalculateArrayEntry(phandle, edgeData[e], 
					(e & 0x1) ? pbuf1 : pbuf0);
	for (e++; e < lEdge; e++)
	{
	    float l;

	    p0 = p1;
	    p1 = (float *)DXCalculateArrayEntry(phandle, edgeData[e], 
					    (e & 0x1) ? pbuf1 : pbuf0);
	    
	    for (l = 0.0, i = 0; i < nDim; i++)
	    {
		float d = p1[i] - p0[i];
		l += d*d;
	    }

	    length += sqrt(l);

	}
    }

done:

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    DXFree(pbuf0);
    DXFree(pbuf1);
    *measure = fabs(length);

    return OK;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    DXFree(pbuf0);
    DXFree(pbuf1);
    return ERROR;
}

static Error
Polyline_Length_Elements(Field field, float *measurements)
{
    Array polylines, edges, positions;
    int   *polylineData, *edgeData;
    ArrayHandle phandle = NULL;
    int nPolylines, nEdges, nDim;
    Type t;
    Category c;
    int rank, shape[32];
    int i, p, e;
    Pointer pbuf0=NULL, pbuf1=NULL;
    float *p0 = NULL, *p1 = NULL;
    InvalidComponentHandle ich = NULL;

    if (DXEmptyField(field))
	goto done;
    
    polylines = (Array)DXGetComponentValue(field, "polylines");
    if (! polylines)
    {
	DXSetError(ERROR_MISSING_DATA, "polylines");
	goto error;
    }

    DXGetArrayInfo(polylines, &nPolylines, &t, &c, &rank, shape);

    if (nPolylines == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"polylines must be real integer scalars");
	goto error;
    }

    edges = (Array)DXGetComponentValue(field, "edges");
    if (! edges)
    {
	DXSetError(ERROR_MISSING_DATA, "edges");
	goto error;
    }

    DXGetArrayInfo(edges, &nEdges, &t, &c, &rank, shape);

    if (nEdges == 0)
	goto done;
    
    if (t != TYPE_INT ||
	c != CATEGORY_REAL ||
	!(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"edges must be real integer scalars");
	goto error;
    }

    positions = (Array)DXGetComponentValue(field, "positions");
    if (! positions)
    {
	DXSetError(ERROR_MISSING_DATA, "positions");
	goto error;
    }

    DXGetArrayInfo(positions, NULL, &t, &c, &rank, &nDim);

    if (t != TYPE_FLOAT || c != CATEGORY_REAL
	|| rank != 1 || (nDim != 2 && nDim != 3))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"positions must be real float 2- or 3-vectors");
	goto error;
    }

    phandle = DXCreateArrayHandle(positions);
    if (! phandle)
	goto error;

    pbuf0 = DXAllocate(DXGetItemSize(positions));
    pbuf1 = DXAllocate(DXGetItemSize(positions));
    if (!pbuf0 || !pbuf1)
	goto error;

    polylineData = (int *)DXGetArrayData(polylines);
    edgeData = (int *)DXGetArrayData(edges);

    if (DXGetComponentValue(field, "invalid polylines"))
    {
	ich = DXCreateInvalidComponentHandle((Object) field, NULL, "polylines");
	if (ich == NULL)
	    goto error;
    }

    for (p = 0; p < nPolylines; p++)
    {
	int fEdge = polylineData[p];
	int lEdge = (p == nPolylines-1) ? nEdges : polylineData[p+1];
	float length = 0;

	if (ich && DXIsElementInvalid(ich, p))
	{
	    measurements[p] = 0.0;
	    continue;
	}

	e = fEdge;
	p1 = (float *)DXCalculateArrayEntry(phandle, edgeData[e], 
					(e & 0x1) ? pbuf1 : pbuf0);
	for (e++; e < lEdge; e++)
	{
	    float l;

	    p0 = p1;
	    p1 = (float *)DXCalculateArrayEntry(phandle, edgeData[e], 
					    (e & 0x1) ? pbuf1 : pbuf0);
	    
	    for (l = 0.0, i = 0; i < nDim; i++)
	    {
		float d = p1[i] - p0[i];
		l += d*d;
	    }

	    length += sqrt(l);

	}

	measurements[p] = fabs(length);
    }

done:

    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    DXFree(pbuf0);
    DXFree(pbuf1);

    return OK;

error:
    if (ich)
	DXFreeInvalidComponentHandle(ich);
    DXFreeArrayHandle(phandle);
    DXFree(pbuf0);
    DXFree(pbuf1);
    return ERROR;
}
