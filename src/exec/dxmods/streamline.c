/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/streamline.c,v 1.11 2006/06/10 16:33:58 davidt Exp $:
 */
#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <math.h>
#include "stream.h"
#include "vectors.h"

typedef enum
{
    STREAM_OK,
    STREAM_FULL,
    STREAM_ERROR
} StreamFlags;

#ifdef STREAM_MAX
#undef STREAM_MAX
#endif

#define STREAM_MAX	25000
#define TOT_MAX		50000
#define ZERO_MAX	100

static Stream 	   NewStream(int);
static void   	   DestroyStream(Stream);
static Error   	   FlushStream(Stream);
static Field  	   StreamToField(Stream);
static StreamFlags AddPointToStream(Stream, POINT_TYPE *, VECTOR_TYPE *, double);

static VectorField  InitVectorField(Object);
static Error      DestroyVectorField(VectorField);
static Error      GetElementType(Object, char **);

static Array      Starts(Object, Object, int);
static Error      StartsRecurse(Object, Array *);
static Error      StartsAddPoints(Array *, Array);
static Error      AlignStartPtsAndTimes(Array, Array);

static Error      Streamlines(Object, Array, Array, float, float, Group,
							    Object, int);
static Error      Streamline(VectorField, float *, float, float, float, Group);
static Error      StreamTask(Pointer);

static Error      TraceFrame(Pointer);
static Error      InitFrame(Vector *, Vector *, Vector *);
static Error      UpdateFrame(float *, float *, float *, float *, 
				float *, Vector *, Vector *, Vector *);
static void       RotateAroundVector(Vector *, float, float, float *);

static int 	  ZeroVector(VECTOR_TYPE *, int nDim);
static double 	  VectorDot(VECTOR_TYPE *, VECTOR_TYPE *, int nDim);

static int        IsRegular(Object);
static Error	  GeometryCheck(Object, int);

static InstanceVars FindElement(VectorField, POINT_TYPE *, VectorGrp);
static InstanceVars FindMultiGridContinuation(VectorField, POINT_TYPE *, VectorGrp);

Error
m_Streamline(Object *in, Object *out)
{
    Array  	starts = NULL, times = NULL;
    Object 	vectors, curls = NULL;
    int		curlFlag = 0;
    float  	c;
    float  	duration;
    Group  	group = NULL;
    Type   	type;
    Category 	cat;
    int		rank;
    int	   	nD, n;
    Class	vClass;

    out[0] = NULL;

    if (! in[0])
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "data");
	goto error;
    }

    vectors = in[0];
    if ((vClass = DXGetObjectClass(vectors)) == CLASS_GROUP)
	vClass = DXGetGroupClass((Group)vectors);
    
    if (vClass != CLASS_FIELD          &&
	vClass != CLASS_COMPOSITEFIELD &&
	vClass != CLASS_MULTIGRID)
    {
	DXSetError(ERROR_BAD_PARAMETER, 
		"data must be a field, multigrid, or composite field");
	goto error;
    }

    if (! DXGetType(vectors, &type, &cat, &rank, &nD))
    {
	DXSetError(ERROR_MISSING_DATA, "#10250", "data", "data");
	goto error;
    }

    if (type != TYPE_FLOAT || cat != CATEGORY_REAL || (nD != 2 && nD != 3))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10302", "data");
	goto error;
    }

    if (vectors)
	if (! GeometryCheck(vectors, nD))
	    goto error;

    /*
     * Starting points are gathered from arg 1 or centerpoint of vector
     * field.
     */
    starts = Starts(in[1], in[0], nD);
    if (starts)
        DXGetArrayInfo(starts, &n, NULL, NULL, NULL, NULL);
    if (! starts || n == 0)
	goto no_streams;
    
    if (! DXTypeCheck(starts, TYPE_FLOAT, CATEGORY_REAL, 1, nD))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#11823", "start", "data");
	goto error;
    }

    /*
     * Starting times are gathered from arg 2 or are a single 0.0
     */
    times = Starts(in[2], NULL, 1);
    
    /*
     * Match up the starting points and times
     */
    if (! AlignStartPtsAndTimes(starts, times))
	goto error;
    
    if (in[3])
    {
	if (! DXExtractFloat(in[3], &duration))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10080", "head");
	    goto error;
	}
    }
    else
	duration = DXD_MAX_FLOAT;
    
    if (in[4])
    {
	int nDC;

	curls = in[4];
	if (! DXGetType(curls, &type, &cat, &rank, &nDC))
	    goto error;
	
	if (type != TYPE_FLOAT || cat != CATEGORY_REAL || rank != 1)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10282", "curl");
	    goto error;
	}

	if (nDC != nD)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#11400", "curl", "data");
	    goto error;
	}

	curlFlag = 1;
    }

    if (in[5])
    {
	if (! DXExtractInteger(in[5], &curlFlag) ||
				(curlFlag < 0 || curlFlag > 1))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10070", "curlflag");
	    goto error;
	}
    }

    if (in[6])
    {
	if (! DXExtractFloat(in[6], &c))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "#10090", "stepscale");
	    goto error;
	}

	if (c <= 0.0)
	    c = DEFAULT_C;
    }
    else
	c = DEFAULT_C;

    group = DXNewGroup();
    if (! group)
	goto error;
    
    if (! Streamlines(vectors, starts, times, c, duration,
					group, curls, curlFlag))
	goto error;
    
    if (! DXGetEnumeratedMember(group, 0, 0))
    {
	DXDelete((Object)group);
	out[0] = (Object)DXEndField(DXNewField());
    }
    else
    {
	DXSetFloatAttribute((Object)group, "fuzz", 4);
	out[0] = (Object)group;
    }
    
    DXDelete((Object)starts);
    DXDelete((Object)times);
    return OK;

no_streams:
    out[0] = NULL;
    DXDelete((Object)starts);
    DXDelete((Object)times);
    return OK;

error:
    DXDelete((Object)starts);
    DXDelete((Object)times);
    DXDelete((Object)group);

    return ERROR;
}

typedef struct task
{
    VectorField vf;
    float       point[3];
    float       time;
    float       c, duration;
    Group       g;
} Task;

static Error
Streamlines(Object iVectors, Array starts, Array times, float c,
		float d, Group g, Object curl, int curlFlag)
{
    Object	  vectors = NULL;
    VectorField   vf = NULL;
    int           i, j, n;
    Task          task;
    float         *point;
    float         *time;
    int           nDim;
    Field 	  f;
    int		  nP;

    nP = DXProcessors(0);

    DXGetArrayInfo(starts, &n, NULL, NULL, NULL, &nDim);

    if (n < 1)
	return OK;

    vectors = DXReference(DXCopy(iVectors, COPY_STRUCTURE));
    if (! vectors)
	return ERROR;

    vf = InitVectorField(vectors);
    if (! vf)
	goto error;

    point = (float *)DXGetArrayData(starts);
    time  = (float *)DXGetArrayData(times);
    if (!point || !time)
	 goto error;

    if (n == 1 || nP == 1)
    {
	for (i = 0; i < n; i++)
	{
	    if (*time <= d)
		if (! Streamline(vf, point, *time, c, d, g))
		    goto error;
	    
	    point += nDim;
	    time  += 1;
	}
    }
    else
    {
	task.vf       = vf;
	task.c        = c;
	task.duration = d;
	task.g        = g;

	if (! DXCreateTaskGroup())
	    goto error;
	
	for (i = 0; i < n; i++)
	{
	    for (j = 0; j < nDim; j++)
		task.point[j] = *point++;
	    
	    task.time = *time++;
	    
	    if (task.time <= task.duration)
		if (! DXAddTask(StreamTask, (Pointer)&task, sizeof(task), 1.0))
		{
		    DXAbortTaskGroup();
			goto error;
		}
	}

	if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	    goto error;
    }

    /*
     * Did we generate any streamlines?  Count them.
     */
    for (n = 0; DXGetEnumeratedMember(g, n, 0); n++);

    if (n <= 0)
	goto no_streams;
    
    if (vf->nDim == 3)
    {
	if (curlFlag)
	{
	    Interpolator  curlMap;

	    if (curl)
		curlMap = DXNewInterpolator(curl, INTERP_INIT_PARALLEL, 0.0);
	    else
	    {
		MultiGrid mg = DXNewMultiGrid();
		if (! mg)
		    goto error;
	    
		for (i = 0; i < vf->nmembers; i++)
		    if (! (*(vf->members[i]->CurlMap))(vf->members[i], mg))
		    {
			DXDelete((Object)mg);
			goto error;
		    }
		
		curlMap = DXNewInterpolator((Object)mg,
					INTERP_INIT_PARALLEL, -1.0);
	    }

	    if (! curlMap && DXGetError() != ERROR_NONE)
		goto error;

	    if (curlMap)
	    {
		if (! DXMap((Object)g, (Object)curlMap, "positions", "curl"))
		{
		    DXDelete((Object)curlMap);
		    goto error;
		}

		DXDelete((Object)curlMap);
	    }
	}


	if (n == 1 || nP == 1)
	{
	    for (i = 0; i < n; i++)
	    {
		f = (Field)DXGetEnumeratedMember(g, i, NULL);
		if (! TraceFrame((Pointer)f))
		    goto error;
	    }
	}
	else
	{
	    if (! DXCreateTaskGroup())
		goto error;
	    
	    for (i = 0; i < n; i++)
	    {
		f = (Field)DXGetEnumeratedMember(g, i, NULL);
		if (! DXAddTask(TraceFrame, (Pointer)f, 0, 1.0))
		{
		    DXAbortTaskGroup();
		    goto error;
		}
	    }
	    
	    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		goto error;
	}
    }

no_streams:
    DestroyVectorField(vf);
    vf = NULL;
    DXDelete((Object)vectors);
    return OK;

error:
    DXDelete((Object)vectors);
    DestroyVectorField(vf);
    return ERROR;
}

static Error
StreamTask(Pointer p)
{
    Task *t = (Task *)p;

    return Streamline(t->vf, t->point, t->time, t->c, t->duration, t->g);
}


static Error
Streamline(VectorField vf, 	/* Vector field to trace streamline in	*/
	   float       *point,	/* streamline origination position	*/
	   float       time,	/* streamline origination time		*/
	   float       c,	/* step criterion			*/
	   float       duration,/* max. lifetime of stream		*/
	   Group       g)	/* group in which to put the result     */
{
    Stream 	 stream = NULL;
    VectorGrp    vg=NULL;
    POINT_TYPE	 pb0[3], pb1[3], *ptmp;
    VECTOR_TYPE	 vb0[3], vb1[3], *vtmp;
    POINT_TYPE 	 *p0, *p1;
    VECTOR_TYPE  *v0, *v1;
    double     	 t, t2, elapsedtime;
    int		 nDim;
    int		 done, keepPoint;
    Error	 (*Interpolate)();
    Error	 (*StepTime)();
    Error	 (*FindBoundary)();
    int 	 (*Neighbor)();
    int 	 (*Weights)();
    int 	 (*FaceWeights)();
    Field 	 field = NULL;
    int 	 i, totKnt, zeroKnt, streamKnt;
    InstanceVars I = NULL;
    StreamFlags  sFlag;
    int		 zero;

    stream      = NULL;
    elapsedtime = time;
    
    /*
     * Double buffer pointers
     */
    p0 = pb0; p1 = pb1;
    v0 = vb0; v1 = vb1;

    /*
     * Initial conditions
     */
    for (i = 0; i < vf->nDim; i++)
	p0[i] = point[i];

    I = FindElement(vf, p0, NULL);
    if (! I)
	goto stream_done;

    vg 		 = I->currentVectorGrp;

    Interpolate  = vg->Interpolate;
    StepTime     = vg->StepTime;
    FindBoundary = vg->FindBoundary;
    Neighbor     = vg->Neighbor;
    Weights      = vg->Weights;
    FaceWeights  = vg->FaceWeights;
    nDim	 = vg->nDim;

    if (! (*Interpolate)(I, p0, v0))
	goto error;
    
    stream = NewStream(nDim);
    if (! stream)
	goto error;

    sFlag = AddPointToStream(stream, p0, v0, elapsedtime);
    if (sFlag == STREAM_ERROR)
	goto error;

    if (ZeroVector(v0, nDim))
	goto stream_done;
    
    /*
     * While stream is passing from partition to partition...
     */
    done = 0; totKnt = 1; zeroKnt = 0; streamKnt = 1;
    while (!done)
    {
	/*
	 * Get the point and the appropriate time step
	 */
	if (! (*StepTime)(I, c, v0, &t))
	    goto error;
	
	/*
	 * If the time step exceeds the duration, truncate it and mark
	 * the stream complete.
	 */
	if (elapsedtime + t > duration)
	{
	    t = duration - elapsedtime;
	    done = 1;
	}

	/*
	 * Get the point a time step away in the direction of the
	 * vector
	 */
	for (i = 0; i < nDim; i++)
	    p1[i] = p0[i] + t*v0[i];

	if (! (*Weights)(I, p1))
	{
	    int found;
		
	    /*
	     * Segment proceeds from a point inside an element (or on
	     * the boundary of the element) to one outside the element.
	     * Move to the boundary.
	     */

	    if (! (*FindBoundary)(I, p0, v0, &t2))
		goto stream_done;

	    for (i = 0; i < nDim; i++)
		p1[i] = p0[i] + t2*v0[i];

	    /*
	     * P1 is the point in the boundary.  Interpolate a vector for it
	     * based only on the face.
	     */
	    if (! (*FaceWeights)(I, p1))
		goto stream_done;

	    if (! (*Interpolate)(I, p1, v1))
		goto error;
	    
	    found = (*Neighbor)(I, v1);
            if (found == -1)
                goto error;
            else if (! found) {
                if (vf->nmembers != 1)
		{
		    VectorGrp lastGrp = I->currentVectorGrp;

		    (*vg->FreeInstanceVars)(I);
		    I = FindMultiGridContinuation(vf, p1, lastGrp);
		    if (! I)
		    {
			done = 1;
		    }
		    else
		    {
			vg = I->currentVectorGrp;

			Interpolate  = vg->Interpolate;
			StepTime     = vg->StepTime;
			FindBoundary = vg->FindBoundary;
			Neighbor     = vg->Neighbor;
			Weights      = vg->Weights;
			FaceWeights  = vg->FaceWeights;
		    }
		}
		else 
		    done = 1;
	    }
		
	    if (done || t2 > 0.05*t)
		keepPoint = 1;
	    else
		keepPoint = 0;

	    t = t2;
	}
	else
	{
	    /*
	     * Point is inside the current element.  Regardless of whether 
	     * p0 is on the boundary, the step is fine and since p1 is not
	     * on the boundary, we clear the boundary flag.
	     */
	    if (! (*Interpolate)(I, p1, v1))
		goto error;

	    keepPoint = 1;
	}

	if (ZeroVector(v1, nDim) || VectorDot(v0, v1, nDim) < -0.0)
	    done = 1;
	
	elapsedtime += t;

	if (keepPoint && t != 0.0)
	{
	    sFlag = AddPointToStream(stream, p1, v1, elapsedtime);
	    if (sFlag == STREAM_ERROR)
		goto error;
	    else if (sFlag == STREAM_FULL)
	    {
		DXWarning("streamline point limit exceeded");
		done = 1;
	    }

	    streamKnt ++;
	}

	zero = 1;
	for (i = 0; zero && i < nDim; i++)
	    zero = (p0[i] == p1[i]);
	
	if (zero)
	{
	    if (++zeroKnt > ZERO_MAX)
	    {
		DXWarning("possible infinite loop detected");
		done = 1;
	    }
	}
	else
	    zeroKnt = 0;

	if (++totKnt > TOT_MAX)
	{
	    DXWarning("possible infinite loop detected");
	    done = 1;
	}

	ptmp = p0;
	p0 = p1;
	p1 = ptmp;

	vtmp = v0;
	v0 = v1;
	v1 = vtmp;

    }

stream_done:

    if (stream)
    {
	field = StreamToField(stream);
	if (! field)
	    goto error;

	DestroyStream(stream);
	stream = NULL;

	if (! DXSetMember(g, NULL, (Object)field))
	    goto error;
    }

    if (I)
	(*vg->FreeInstanceVars)(I);
    
    return OK;

error:
    if (I)
	(*vg->FreeInstanceVars)(I);
    
    DestroyStream(stream);
    DXDelete((Object)field);

    return ERROR;
}

static Error
TraceFrame(Pointer ptr)
{
    Field  field;
    Array  nArray = NULL, bArray = NULL;
    Array  cArray, vArray,  pArray;
    float  *points, *vecarr, *curls, *binarr, *normarr;
    int    i, nVectors, nDim;
    Object depattr = NULL;
    Vector  fn, fb, ft, normals, vectors, binormals;

    field = (Field)ptr;
    if (DXEmptyField(field))
	return OK;

    vArray = (Array)DXGetComponentValue(field, "data");
    if (! vArray)
	goto error;

    DXGetArrayInfo(vArray, &nVectors, NULL, NULL, NULL, &nDim);
    if (nDim < 3)
	goto done;

    vecarr = (float *)DXGetArrayData(vArray);
    if (! vecarr)
	goto error;
	vectors.x = vecarr[0];
	vectors.y = vecarr[1];
	vectors.z = vecarr[2];

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
	goto error;

    DXGetArrayInfo(pArray, &nVectors, NULL, NULL, NULL, &nDim);
    if (nDim < 3)
	goto done;

    points = (float *)DXGetArrayData(pArray);
    if (! points)
	goto error;

    cArray = (Array)DXGetComponentValue(field, "curl");
    if (cArray)
    {
	curls = (float *)DXGetArrayData(cArray);
	if (! curls)
	    goto error;
    }
    else
	curls = NULL;
    
    nArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! nArray)
	goto error;
    
    if (! DXAddArrayData(nArray, 0, nVectors, NULL))
	goto error;
    
    normarr = (float *)DXGetArrayData(nArray);
    if (! normarr)
	goto error;
	normals.x = normarr[0];
	normals.y = normarr[1];
	normals.z = normarr[2];
    
    bArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! bArray)
	goto error;
    
    if (! DXAddArrayData(bArray, 0, nVectors, NULL))
	goto error;
    
    binarr = (float *)DXGetArrayData(bArray);
    if (! binarr)
	goto error;
	binormals.x = binarr[0];
	binormals.y = binarr[1];
	binormals.z = binarr[2];
    
    if (! InitFrame(&vectors, &normals, &binormals))
	goto error;
	
	vecarr[0] = vectors.x;
	vecarr[1] = vectors.y;
	vecarr[2] = vectors.z;
	normarr[0] = normals.x;
	normarr[1] = normals.y;
	normarr[2] = normals.z;
	binarr[0] = binormals.x;
	binarr[1] = binormals.y;
	binarr[2] = binormals.z;
    
    fn.x = normals.x;
    fn.y = normals.y;
    fn.z = normals.z;
    fb.x = binormals.x;
    fb.y = binormals.y;
    fb.z = binormals.z;
    ft.x = vectors.x;
    ft.y = vectors.y;
    ft.z = vectors.z;

    _dxfvector_normalize_3D(&fn, &fn);
    _dxfvector_normalize_3D(&fb, &fb);
    _dxfvector_normalize_3D(&ft, &ft);

    for (i = 1; i < nVectors; i++)
    {
	float avcurl[3];

	points += 3; vecarr += 3; normarr += 3; binarr += 3;
	if (curls)
	{
	    avcurl[0] = 0.5 * (curls[0] + curls[3]);
	    avcurl[1] = 0.5 * (curls[1] + curls[4]);
	    avcurl[2] = 0.5 * (curls[2] + curls[5]);
	    curls += 3;

	    if (! UpdateFrame(points, vecarr, avcurl, normarr,
						binarr, &fn, &fb, &ft))
		goto error;
	}
	else
	{
	    if (! UpdateFrame(points, vecarr, NULL, normarr,
						binarr, &fn, &fb, &ft))
		goto error;
	}
    }

    depattr = (Object)DXNewString("positions");
    if (! depattr)
	goto error;
    DXReference(depattr);

    if (! DXSetComponentValue(field, "normals", (Object)nArray))
	goto error;
    nArray = NULL;

    if (! DXSetComponentAttribute(field, "normals", "dep", depattr))
	goto error;

    if (! DXSetComponentValue(field, "binormals", (Object)bArray))
	goto error;
    bArray = NULL;

    if (! DXSetComponentAttribute(field, "binormals", "dep", depattr))
	goto error;

    if (cArray)
	DXDeleteComponent(field, "curl");
    
    DXDelete(depattr);
    
done:
    return OK;

error:
    DXDelete((Object)nArray);
    DXDelete((Object)bArray);

    return ERROR;
}

static Array
Starts(Object starts, Object vectors, int nD)
{
    Array array = NULL;

    if (starts)
    {
	Object xstarts;

        xstarts = DXApplyTransform(starts, NULL);
	if (! xstarts)
	    goto error;
	
	if (! StartsRecurse(xstarts, &array))
	{
	    if (starts != xstarts)
		DXDelete((Object)xstarts);
	    goto error;
	}

	if (starts != xstarts)
	    DXDelete((Object)xstarts);
    }
    else if (vectors)
    {
	Point box[8];
	float pt[3];

	if (! DXBoundingBox(vectors, box))
	    return NULL;
	
	pt[0] = (box[0].x + box[7].x) / 2.0;
	pt[1] = (box[0].y + box[7].y) / 2.0;
	pt[2] = (box[0].z + box[7].z) / 2.0;
    
	array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nD);
	if (! array)
	    goto error;
	
	if (! DXAddArrayData(array, 0, 1, (Pointer)pt))
	    goto error;

    }
    else
    {
	float *ptr;
	int i;

	array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nD);
	if (! array)
	    goto error;
	
	if (! DXAddArrayData(array, 0, 1, NULL))
	    goto error;
	
	ptr = (float *)DXGetArrayData(array);
	if (! ptr)
	    goto error;
	
	for (i = 0; i < nD; i++)
	    *ptr++ = 0.0;
    }

    return array;

error:
    DXDelete((Object)array);
    return NULL;
}

static Error
AlignStartPtsAndTimes(Array starts, Array times)
{
    int nStarts, nTimes;

    DXGetArrayInfo(starts, &nStarts, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(times,  &nTimes, NULL, NULL, NULL, NULL);

    if (nStarts == nTimes)
	return OK;
    else if (nStarts == 1)
    {
	int itemSize = DXGetItemSize(starts);
	int i;
	unsigned char *src, *dst;

	if (! DXAddArrayData(starts, 1, nTimes-1, NULL))
	    goto error;

	src = (unsigned char *)DXGetArrayData(starts);
	dst = src + itemSize;
	for (i = 0; i < nTimes; i++, dst += itemSize)
	     memcpy(dst, src, itemSize);
    }
    else if (nTimes == 1)
    {
	int itemSize = DXGetItemSize(times);
	int i;
	unsigned char *src, *dst;

	if (! DXAddArrayData(times, 1, nStarts-1, NULL))
	    goto error;

	src = (unsigned char *)DXGetArrayData(times);
	dst = src + itemSize;
	for (i = 0; i < nStarts; i++, dst += itemSize)
	     memcpy(dst, src, itemSize);
    }
    else
    {
	DXSetError(ERROR_BAD_PARAMETER, "#11160",
		"start positions", nStarts, "start times", nTimes);
	goto error;
    }

    return OK;

error:
    return ERROR;
}

static Error
StartsRecurse(Object o, Array *a)
{
    Object c;
    int    i;

    if (DXGetObjectClass(o) == CLASS_ARRAY)
        return StartsAddPoints(a, (Array)o);
    else if (DXGetObjectClass(o) == CLASS_FIELD)
    {
	if (! DXEmptyField((Field)o))
	    return StartsAddPoints(a, 
		(Array)DXGetComponentValue((Field)o, "positions"));
	else
	    return OK;
    }
    else if (DXGetObjectClass(o) == CLASS_GROUP)
    {
	i = 0;
	while(NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
	{
	    if (! StartsRecurse(c, a))
		goto error;
	}
	return OK;
    }
    else
    {
	DXSetError(ERROR_BAD_PARAMETER, "starts");
	return ERROR;
    }

error:
    DXDelete((Object)*a);
    *a = NULL;
    return ERROR;
}

static Error
StartsAddPoints(Array *dst, Array src)
{
    int nSrc, nDst, rank, shape[32];

    if (! src || DXGetObjectClass((Object)src) != CLASS_ARRAY)
	return ERROR;
    
    if (*dst == NULL)
    {
	DXGetArrayInfo(src, &nSrc, NULL, NULL, &rank, shape);
	if (nSrc == 0)
	    return OK;

	*dst = DXNewArrayV(TYPE_FLOAT, CATEGORY_REAL, rank, shape);
	if (! *dst)
	     return ERROR;
	
	if (rank == 0)
	    shape[0] = 1;

	if (! DXAddArrayData(*dst, 0, nSrc, NULL))
	    return ERROR;

	if (! DXExtractParameter((Object)src, TYPE_FLOAT, shape[0], nSrc, 
							DXGetArrayData(*dst)))
	    return ERROR;
    }
    else
    {
	DXGetArrayInfo(*dst, &nDst, NULL, NULL, &rank, shape);
	DXGetArrayInfo(src, &nSrc, NULL, NULL, NULL, NULL);
	if (nSrc == 0)
	    return OK;

	if (! DXAddArrayData(*dst, nDst, nSrc, NULL))
	    return ERROR;

	if (rank == 0)
	    shape[0] = 1;

	if (! DXExtractParameter((Object)src, TYPE_FLOAT, shape[0], nSrc, 
			(Pointer)(((float *)DXGetArrayData(*dst))+nDst*shape[0])))
	    return ERROR;
    }

    return OK;
}

static Error
DestroyVectorField(VectorField vf)
{
    if (vf)
    {
	int i;
	for (i = 0; i < vf->nmembers; i++)
	    if (vf->members[i])
		(*(vf->members[i]->Delete))(vf->members[i]);
	
	DXFree((Pointer)vf->members);
	DXFree((Pointer)vf);
    }
    return OK;
}

static VectorField
InitVectorField(Object vfo)
{
    char *str;
    VectorField vf = NULL;
    Class class;

    vf = (struct vectorField *)DXAllocateZero(sizeof(struct vectorField));
    if (! vf)
	goto error;
    
    class = DXGetObjectClass(vfo);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)vfo);
    
    switch(class)
    {
	case CLASS_MULTIGRID:
	{
	    int i, n;
	    char *str;

	    if (! DXGetMemberCount((Group)vfo, &n))
		goto error;
	    
	    vf->nmembers = n;
	    
	    vf->members = (VectorGrp *)DXAllocate(n*sizeof(VectorGrp));
	    if (! vf->members)
		goto error;
	    
	    for (i = 0; i < n; i++)
	    {
		Object mo = DXGetEnumeratedMember((Group)vfo, i, NULL);
		if (!mo)
		    goto error;
		
		class = DXGetObjectClass(mo);
		if (class == CLASS_GROUP)
		    class = DXGetGroupClass((Group)mo);
		
		if (class != CLASS_FIELD && class != CLASS_COMPOSITEFIELD)
		{
		    DXSetError(ERROR_DATA_INVALID, "invalid multigrid member");
		    goto error;
		}

		if (! GetElementType(mo, &str))
		{
		    DXSetError(ERROR_DATA_INVALID, "vectors");
		    goto error;
		}

		switch (IsRegular(mo))
		{
		    case 1:
			vf->members[i] = _dxfReg_InitVectorGrp(mo, str);
			break;
		
		    case 0:
			vf->members[i] = _dxfIrreg_InitVectorGrp(mo, str);
			break;
		    
		    default:
		    {
			DXSetError(ERROR_DATA_INVALID, "vectors");
			goto error;
		    }
		}
		
		if (! vf->members[i])
		    goto error;
	    }
	}
	break;

	case CLASS_FIELD:
	case CLASS_COMPOSITEFIELD:
	{
	    vf->nmembers = 1;

	    vf->members = (VectorGrp *)DXAllocate(sizeof(VectorGrp));
	    if (! vf->members)
		goto error;
	    
	    if (! GetElementType(vfo, &str))
	    {
		DXSetError(ERROR_DATA_INVALID, "vectors");
		goto error;
	    }

	    switch (IsRegular(vfo))
	    {
		case 1:
		    vf->members[0] = _dxfReg_InitVectorGrp(vfo, str);
		    break;
	    
		case 0:
		    vf->members[0] = _dxfIrreg_InitVectorGrp(vfo, str);
		    break;
		
		default:
		{
		    DXSetError(ERROR_DATA_INVALID, "vectors");
		    goto error;
		}
	    }
	    
	    if (! vf->members[0])
		goto error;
	}
	break;
    
	default:
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"invalid object encountered in vector field");
	    goto error;
	}
    }

    vf->nDim = vf->members[0]->nDim;

    return vf;

error:
    DestroyVectorField(vf);
    return NULL;
}

static Error
GetElementType(Object o, char **str)
{
    Object c;

    *str = NULL;

    if (DXGetObjectClass(o) == CLASS_FIELD)
    {
	if (DXEmptyField((Field)o))
	    return ERROR;
	
	if (! DXGetComponentValue((Field)o, "connections"))
	{
	    DXSetError(ERROR_DATA_INVALID, "data has no connections");
	    return ERROR;
	}

	c = DXGetComponentAttribute((Field)o, "connections", "element type");
	if (! c)
	{
	    DXSetError(ERROR_MISSING_DATA, "element type attribute");
	    return ERROR;
	}

	if (DXGetObjectClass(c) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, "element type attribute");
	    return ERROR;
	}

	*str = DXGetString((String)c);
	return OK;
    }
    else if (DXGetObjectClass(o) == CLASS_GROUP)
    {
	int i;
	Error j = ERROR;

	i = 0;
	while(NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
	{
	    j = GetElementType(c, str);
	    if (j != ERROR)
		return OK;
	}

	return ERROR;
    }
    else
    {
	DXSetError(ERROR_BAD_CLASS, "vector field");
	return ERROR;
    }
}

static Stream
NewStream(nDim)
{
    Stream s = NULL;

    s = (Stream)DXAllocateZero(sizeof(struct stream));
    if (! s)
	goto error;
    
    s->points  = (float *)DXAllocate(nDim*STREAM_BUF_POINTS*sizeof(float));
    s->pArray  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nDim);
    s->time    = (float *)DXAllocate(STREAM_BUF_POINTS*sizeof(float));
    s->tArray  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    s->vectors = (float *)DXAllocate(nDim*STREAM_BUF_POINTS*sizeof(float));
    s->vArray  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nDim);
    if (s->points  == NULL || s->time   == NULL ||
        s->pArray  == NULL || s->tArray == NULL ||
	s->vectors == NULL || s->vArray == NULL)
	goto error;

    s->nDim     = nDim;
    s->bufKnt   = 0;
    s->arrayKnt = 0;

    return s;

error:
    DestroyStream(s);

    return NULL;
}

static void
DestroyStream(Stream s)
{
    if (s)
    {
	DXFree((Pointer)s->points);
	DXFree((Pointer)s->time);
	DXFree((Pointer)s->vectors);
	DXDelete((Object)s->pArray);
	DXDelete((Object)s->vArray);
	DXDelete((Object)s->tArray);
	DXFree((Pointer)s);
    }
}

static Error
FlushStream(Stream s)
{
    if (! DXAddArrayData(s->pArray, s->arrayKnt, s->bufKnt, (Pointer)s->points))
	goto error;

    if (! DXAddArrayData(s->vArray, s->arrayKnt, s->bufKnt, (Pointer)s->vectors))
	goto error;

    if (! DXAddArrayData(s->tArray, s->arrayKnt, s->bufKnt, (Pointer)s->time))
	goto error;
    
    s->arrayKnt += s->bufKnt;
    s->bufKnt = 0;
    
    return OK;

error:
    return ERROR;
}

static StreamFlags
AddPointToStream(Stream s, POINT_TYPE *p, VECTOR_TYPE *v, double t)
{
    if (s->bufKnt == STREAM_BUF_POINTS)
        if (! FlushStream(s))
	    goto error;
    
    if (s->nDim == 2)
    {
	float *ptr;

	ptr = s->points + s->bufKnt*s->nDim;
	*ptr++ = *p++;
	*ptr++ = *p++;

	ptr = s->vectors + s->bufKnt*s->nDim;
	*ptr++ = *v++;
	*ptr++ = *v++;
    }
    else
    {
	float *ptr;

	ptr = s->points + s->bufKnt*s->nDim;
	*ptr++ = *p++;
	*ptr++ = *p++;
	*ptr++ = *p++;

	ptr = s->vectors + s->bufKnt*s->nDim;
	*ptr++ = *v++;
	*ptr++ = *v++;
	*ptr++ = *v++;
    }

    s->time[s->bufKnt] = t;

    s->bufKnt ++;

    if (s->arrayKnt + s->bufKnt > STREAM_MAX)
	return STREAM_FULL;

    return STREAM_OK;

error:
    return STREAM_ERROR;
}

static Field
StreamToField(Stream s)
{
    float c[3];
    float d[3];
    Field f = NULL;
    Array a = NULL;
    Object depattr = NULL;

    depattr = (Object)DXNewString("positions");
    if (! depattr)
	goto error;
    
    DXReference(depattr);

    c[0] = 0.7; c[1] = 0.7; c[2] = 0.0;
    d[0] = 0.0; d[1] = 0.0; d[2] = 0.0;

    if (! FlushStream(s))
	goto error;

    f = DXNewField();
    if (! f)
	goto error;
    
    if (! DXSetComponentValue(f, "positions", (Object)s->pArray))
	goto error;
    s->pArray = NULL;

    if (! DXSetComponentAttribute(f, "positions", "dep", depattr))
	goto error;
    
    if (! DXSetComponentValue(f, "data", (Object)s->vArray))
	goto error;
    s->vArray = NULL;

    if (! DXSetComponentAttribute(f, "data", "dep", depattr))
	goto error;
    
    if (! DXSetComponentValue(f, "time", (Object)s->tArray))
	goto error;
    s->tArray = NULL;

    if (! DXSetComponentAttribute(f, "time", "dep", depattr))
	goto error;
    
    a = DXMakeGridConnections(1, s->arrayKnt);
    if (! a)
	goto error;
    
    if (! DXSetComponentValue(f, "connections", (Object)a))
	goto error;
    a = NULL;

    a = (Array)DXNewRegularArray(TYPE_FLOAT, 3,
			s->arrayKnt, (Pointer)c, (Pointer)d);
    if (! a)
	goto error;

    if (! DXSetComponentValue(f, "colors", (Object)a))
	goto error;
    a = NULL;

    if (! DXSetComponentAttribute(f, "connections", "element type", 						(Object)DXNewString("lines")))
	goto error;
    
    if (! DXSetIntegerAttribute((Object)f, "shade", 0))
	goto error;

    if (! DXEndField(f))
	goto error;

    DXDelete(depattr);

    return f;

error:
    DXDelete(depattr);
    DXDelete((Object)f);
    return ERROR;
}

Error
_dxfMinMaxBox(Field f, float *min, float *max)
{
    float box[24], *b;
    int   i, j;

    if (! DXBoundingBox((Object)f, (Point *)box))
	return ERROR;

    min[0] = min[1] = min[2] =  DXD_MAX_FLOAT;
    max[0] = max[1] = max[2] = -DXD_MAX_FLOAT;

    b = box;
    for (i = 0; i < 8; i++)
	for (j = 0; j < 3; j++, b++)
	{
	    if (*b < min[j]) min[j] = *b;
	    if (*b > max[j]) max[j] = *b;
	}
    
    return OK;
}

Error
_dxfInitVectorPart(VectorPart p, Field f)
{
    Object a;

    p->field = f;

    p->gArray = (Array)DXGetComponentValue(f, "ghostzones");
    if (p->gArray)
        p->ghosts = (unsigned short *)DXGetArrayData(p->gArray);
    else
        p->ghosts = NULL;

    a = DXGetComponentAttribute(f, "data", "dep");
    if (! a)
    {
	fprintf(stderr, "data dep error\n");
	DXSetError(ERROR_MISSING_DATA, "data dependency");
        return ERROR;
    }
    else
    {
        char *s = DXGetString((String)a);
        if (! strcmp(s, "positions"))
            p->dependency = DEP_ON_POSITIONS;
        else if (! strcmp(s, "connections"))
            p->dependency = DEP_ON_POSITIONS;
	else
	{
	    fprintf(stderr, "data dep error 2\n");
	    DXSetError(ERROR_DATA_INVALID, "data dependency");
	    return ERROR;
	}
    }

    if (! _dxfMinMaxBox(f, p->min, p->max))
    {
	fprintf(stderr, "minmaxbox error\n");
        return ERROR;
    }
    
    return OK;
}

int
_dxfIsInBox(VectorPart p, POINT_TYPE *pt, int n, int fuzz)
{
    int i;

    if (fuzz)
    {
	for (i = 0; i < n; i++)
	{
	    float f = 0.0001 * (p->max[i] - p->min[i]);
	    if (pt[i] < (p->min[i]-f) ||  pt[i] > (p->max[i]+f)) return 0;
	}
    }
    else
    {
	for (i = 0; i < n; i++)
	    if (pt[i] < p->min[i] ||  pt[i] > p->max[i]) return 0;
    }
    
    return 1;
}

static int
ZeroVector(VECTOR_TYPE *v, int n)
{
    int i;

    for (i = 0; i < n; i++)
	if (v[i] != 0.0) return 0;
    
    return 1;
}

static double
VectorDot(VECTOR_TYPE *v0, VECTOR_TYPE *v1, int n)
{
    int i;
    double d = 0;

    for (i = 0; i < n; i++)
	d += *v0++ * *v1++;
    
    return d;
}

static Error
InitFrame(Vector *v, Vector *n, Vector *b)
{
    n->x = n->y = n->z = 0.0;

    if (v->x == 0.0 && v->y == 0.0 && v->z == 0.0)
    {
	n->x = 0.0; n->y = 0.0; n->z = 0.0;
	b->x = 0.0; b->y = 0.0; b->z = 0.0;
	return OK;
    }
    
    if (v->x == 0.0 && v->y == 0.0 && v->z > 0.0)
	n->x = 1.0;
    else if (v->x == 0.0 && v->y == 0.0 && v->z < 0.0)
	n->x = -1.0;
    else if (v->x == 0.0 && v->y > 0.0 && v->z == 0.0)
	n->z = 1.0;
    else if (v->x == 0.0 && v->y < 0.0 && v->z == 0.0)
	n->z = -1.0;
    else if (v->x > 0.0 && v->y == 0.0 && v->z == 0.0)
	n->y = 1.0;
    else if (v->x < 0.0 && v->y == 0.0 && v->z == 0.0)
	n->y = -1.0;
    else
    {
	float d;

	if (v->z != 0.0)
	{
	    n->x = n->y = 1.0;
	    n->z = -(v->x + v->y) / v->z;
	}
	else if (v->y != 0.0)
	{
	    n->x = n->z = 1.0;
	    n->y = -(v->x + v->z) / v->y;
	}
    
	d = 1.0 / sqrt(n->x*n->x + n->y*n->y + n->z*n->z);
	
	n->x *= d;
	n->y *= d;
	n->z *= d;
    }

    _dxfvector_cross_3D(n, v, b);
    _dxfvector_normalize_3D(b, b);

    return OK;
}

/*
 * update frame of reference from prior vertex to current vertex based
 * on incuming and outgoing edge vectors and twist.
 */

#define VecMat(a, b, c)				\
{						\
    float x = a->x;				\
    float y = a->y;				\
    float z = a->z;				\
						\
    c->x = x*(b)[0] + y*(b)[3] + z*(b)[6];    \
    c->y = x*(b)[1] + y*(b)[4] + z*(b)[7];    \
    c->z = x*(b)[2] + y*(b)[5] + z*(b)[8];    \
}

#define MatMat(a, b, c)				\
{						\
    VecMat((a+0), (b), (c+0));			\
    VecMat((a+3), (b), (c+3));			\
    VecMat((a+6), (b), (c+6));			\
}

static Error
UpdateFrame(float *pp, float *vp, float *cp, float *np, float *bp, 
					Vector *fn, Vector *fb, Vector *ft)
{
    float  twist;
    float  mTwist[9], mBend[9];
    float  len, cA;
    int	   bend = 0;
    Vector vc, vv, nt, cross;

	vv.x = vp[0]; vv.y = vp[1]; vv.z = vp[2];

    /*
     * If theres a curl, rotate by the curl around the prior segment vector.
     */
    if (cp != NULL)
    {
	Vector step;
	Vector v1, v2;
	
	vc.x = cp[0]; vc.y = cp[1]; vc.z = cp[2];
	v1.x = pp[0]; v1.y = pp[1]; v1.z = pp[2];
	v2.x = (pp-3)[0]; v2.y = (pp-3)[1]; v2.z = (pp-3)[2];
	_dxfvector_subtract_3D(&v1, &v2, &step);
	/*
	_dxfvector_scale_3D(step, (*t - *(t-1)), step);
	*/

	twist = 0.5 * _dxfvector_dot_3D(&step, &vc);
	if (twist != 0.0)
	{
	    RotateAroundVector(ft, cos(twist), sin(twist), mTwist);

	    VecMat(fn, mTwist, fn);
	    VecMat(fb, mTwist, fb);
	}
    }

    /*
     * Look at the length of the step proceeding from the current
     * vertex.  If its zero length, just leave the frame of reference
     * alone.
     */
    len = _dxfvector_length_3D(&vv);
    if (len == 0.0)
    {
	np[0] = fn->x; np[1] = fn->y; np[2] = fn->z;
	bp[0] = fb->x; bp[1] = fb->y; bp[2] = fb->z;
	return OK;
    }
    
    /*
     * DXDot the incoming and outgoing tangents to determine
     * whether a bend occurs.
     */
    _dxfvector_scale_3D(&vv, 1.0/len, &nt);
    cA = _dxfvector_dot_3D(&nt, ft);

    /*
     * If there's a bend angle...
     */
    if (cA < 0.999999)
    {
	float sA, angle;

	bend = 1;

	/*
	 * DXNormalize the bend axis
	 */
	_dxfvector_cross_3D(&nt, ft, &cross);
	_dxfvector_normalize_3D(&cross, &cross);

	/*
	 * Rotate the incoming frame of reference by half the angle to
	 * get a vertex FOR
	 */
	angle = acos(cA)/2;
	cA = cos(angle);
	sA = sin(angle);

	RotateAroundVector(&cross, cA, sA, mBend);

	VecMat(fn, mBend, fn);
	VecMat(fb, mBend, fb);
	VecMat(ft, mBend, ft);

	/*
	 * Now we have the vertex frame of reference
	 */
	np[0] = fn->x; np[1] = fn->y; np[2] = fn->z;
	bp[0] = fb->x; bp[1] = fb->y; bp[2] = fb->z;

	/*
	 * If there was a bend, perform the second half of the rotation
	 */
	if (bend)
	{
	    VecMat(fn, mBend, fn);
	    VecMat(fb, mBend, fb);
	}

	/*
	 * Outgoing tangent is normalized exiting segment vector.  Also
	 * normalize the frame normal and binormal.
	 */
	_dxfvector_normalize_3D(fn, fn);
	_dxfvector_normalize_3D(fb, fb);
	ft->x = nt.x; ft->y = nt.y; ft->z = nt.z;
    }
    else
    {
	np[0] = fn->x; np[1] = fn->y; np[2] = fn->z;
	bp[0] = fb->x; bp[1] = fb->y; bp[2] = fb->z;
    }

    return OK;

}

/*
 * Create a 3x3 matrix defined by the axis v and the sin and cosine 
 * of an angle Theta.
 */
static void
RotateAroundVector(Vector *v, float c, float s, float *M)
{
    float x, y, z;
    float sx, sy, sz;

    x = v->x; y = v->y; z = v->z;
    sx = x*x; sy = y*y; sz = z*z;

    M[0] =  sx*(1.0-c) + c; M[1] = x*y*(1.0-c)-z*s; M[2] = z*x*(1.0-c)+y*s;
    M[3] = x*y*(1.0-c)+z*s; M[4] =  sy*(1.0-c) + c; M[5] = y*z*(1.0-c)-x*s;
    M[6] = z*x*(1.0-c)-y*s; M[7] = y*z*(1.0-c)+x*s; M[8] =  sz*(1.0-c) + c;
}

static int
IsRegular(Object o)
{
    Object c;
    Array a, b;

    if (DXGetObjectClass(o) == CLASS_FIELD)
    {
	if (DXEmptyField((Field)o))
	    return -1;

	a = (Array)DXGetComponentValue((Field)o, "connections");
	b = (Array)DXGetComponentValue((Field)o, "positions");

	if (! a || ! b)
	    return -1;

	if (DXQueryGridConnections(a, NULL, NULL) &&
	    DXQueryGridPositions(b, NULL, NULL, NULL, NULL))
	    return 1;
	else
	    return 0;
    }
    else if (DXGetObjectClass(o) == CLASS_GROUP)
    {
	int i, j, k;

	i = 0; k = -1;
	while(NULL != (c = DXGetEnumeratedMember((Group)o, i++, NULL)))
	{
	    j = IsRegular(c);
	    if (j == 0)
		return 0;
	    if (j == 1)
		k = 1;
	}

	return k;
    }
    else
    {
	DXSetError(ERROR_BAD_CLASS, "vector field");
	return -1;
    }
}

static Error
GeometryCheck(Object vectors, int nD)
{
    Class class = DXGetObjectClass(vectors);
    if (class == CLASS_GROUP)
	class = DXGetGroupClass((Group)vectors);
    
    switch(class)
    {
	case CLASS_FIELD:
	{
	    Field f;
	    Array a;
	    Object o;
	    int n;

	    f = (Field)vectors;
	    if (DXEmptyField(f))
		return OK;
	    
	    a = (Array)DXGetComponentValue(f, "positions");
	    if (! DXGetArrayInfo(a, NULL, NULL, NULL, NULL, &n))
		goto error;
	    
	    if (n != nD)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"vectors are %d-D, vector space is %d-D", nD, n);
		goto error;
	    }

	    if (! DXGetComponentValue(f, "connections"))
	    {
		DXSetError(ERROR_MISSING_DATA, "connections");
		goto error;
	    }

	    o = DXGetComponentAttribute(f, "connections", "element type");
	    if (! o)
	    {
		DXSetError(ERROR_MISSING_DATA, "connections element type");
		goto error;
	    }

	    if (DXGetObjectClass(o) != CLASS_STRING)
	    {
		DXSetError(ERROR_BAD_CLASS, "connections element type attribute");
		goto error;
	    }

	    /*
	    str = DXGetString((String)o);

	    switch(n)
	    {
		case 2:
		    if (strcmp(str, "quads") && strcmp(str, "triangles"))
		    {
			DXSetError(ERROR_DATA_INVALID, 
				"%d-D vector space requires %d-D elements", n, n);
			goto error;
		    }
		    break;
		
		case 3:
		    if (strcmp(str, "cubes") && strcmp(str, "tetrahedra"))
		    {
			DXSetError(ERROR_DATA_INVALID, 
				"%d-D vector space requires %d-D elements", n, n);
			goto error;
		    }
		    break;
		
		default:
		    DXSetError(ERROR_DATA_INVALID, "2- or 3-D vectors required");
		    goto error;
	    }
	    */
	    break;
	}
	
	case CLASS_MULTIGRID:
	case CLASS_COMPOSITEFIELD:
	{
	    Object child;
	    int i;

	    i = 0; 
	    while (NULL != (child = DXGetEnumeratedMember((Group)vectors, i++, NULL)))
		if (! GeometryCheck(child, nD))
		    return ERROR;
	    
	    break;
	}

	default:
	    DXSetError(ERROR_DATA_INVALID,
		"vectors must be field or composite field");
	    goto error;
    }

    return OK;

error:
    return ERROR;
}

static InstanceVars
FindElement(VectorField vField, POINT_TYPE *point, VectorGrp last)
{
    int i;
    InstanceVars I;

    for (i = 0; i < vField->nmembers; i++)
    {
	if (vField->members[i] != last)
	{
	    I = (*(vField->members[i]->NewInstanceVars))(vField->members[i]);

	    if ((*(I->currentVectorGrp->FindElement))(I, point))
	    {
		vField->current = vField->members[i];
		return I;
	    }

	    (*(I->currentVectorGrp->FreeInstanceVars))(I);
	}
    }

    return 0;
}

static InstanceVars
FindMultiGridContinuation(VectorField vField, POINT_TYPE *point, VectorGrp last)
{
    int i;
    InstanceVars I;

    for (i = 0; i < vField->nmembers; i++)
    {
	if (vField->members[i] != last)
	{
	    I = (*(vField->members[i]->NewInstanceVars))(vField->members[i]);

	    if ((*(I->currentVectorGrp->FindMultiGridContinuation))(I, point))
	    {
		vField->current = vField->members[i];
		return I;
	    }

	    (*(I->currentVectorGrp->FreeInstanceVars))(I);
	}
    }

    return 0;
}
