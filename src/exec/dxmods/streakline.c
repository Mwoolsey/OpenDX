/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dxmods/streakline.c,v 1.9 2006/06/10 16:33:58 davidt Exp $:
 */
 
#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>
#include <math.h>

#include "stream.h"
#include "vectors.h"

#define NAME_ARG	0
#define DATA_ARG	1
#define POINTS_ARG	2
#define TIMES_ARG	3
#define HEAD_ARG	4
#define FRAME_ARG	5
#define CURL_ARG	6
#define FLAG_ARG	7
#define SCALE_ARG	8
#define HOLD_ARG	9

typedef enum
{
    STREAK_OK,
    STREAK_FULL,
    STREAK_ERROR
} StreakFlags;

#ifdef STREAM_MAX
#undef STREAM_MAX
#endif

#define STREAK_MAX	25000
#define TOT_MAX		50000
#define ZERO_MAX	100

/*
 * Streak status flags: streaks awaiting their starting time, 	
 * Streaks currently alive (eg. as of the ending time of the last
 * interval) and streaks that have either exited the vector field.
 */
#define STREAK_NEW		1
#define STREAK_ALIVE		2
#define STREAK_DEAD		3
#define STREAK_PENDING		4

typedef struct cacheStreak *CacheStreak;
typedef struct cacheObject *CacheObject;

struct cacheStreak
{
    Series streak;		/* If the streak is alive, this is it.   */
    double  time;		/* If not, this is the time to start it. */
    POINT_TYPE  point[3];	/* And this is the place to start it.    */
    VECTOR_TYPE  norm[3];	/* And this is the propagated normal	 */
    VECTOR_TYPE  binorm[3];	/* And this is the propagated binormal	 */
    VECTOR_TYPE  tan[3];	/* And this is the propagated tangent	 */
    int    status;		/* status: waiting, alive or dead	 */
    int	   start;		/* offset of newly added vertices	 */
};

struct cacheObject
{
    int          frame;
    int		 nDim;
    float        t0, t1;
    Object	 v0, v1;
    Interpolator c0, c1;
    VectorField	 vf0, vf1;
    int		 nStreaks;
    CacheStreak  streaks;
    float	 *frameTimes;
};

typedef struct streakVars
{
    VectorField  vf0;
    VectorField  vf1;
    InstanceVars I0;
    InstanceVars I1;
    float        t0, t1, t;
    int	         inside0, inside1;
    int	         nDim;
} StreakVars;

static Array       Starts(Object, Object, int);
static Array       Times(Object, Object);
static Error       StartsRecurse(Object, Array *);
static Error       StartsAddPoints(Array *, Array);
static Error       AlignStartPtsAndTimes(Array, Array);
static Error       Streaklines(CacheObject, float, int);
static Error       Streakline(CacheObject, int, float);
static Error       StreakTask(Pointer);
static Error       GetFrameData(Object, Object, CacheObject);
static Error       TraceFrame(Pointer);
static Error       InitFrame(Vector *, Vector *, Vector *);
static Error       UpdateFrame(float *, float *, float *,
				float *, float *, float *,
				Vector *, Vector *, Vector *);
static void        RotateAroundVector(Vector *, float, float, float *);
static int 	   ZeroVector(VECTOR_TYPE *, int nDim);
static int         IsRegular(Object);
static Error       GetElementType(Object, char **);
static Error       GetCacheObject(Object *, CacheObject *, Object *);
static Object      SetCacheObject(Object *, CacheObject);
static void        GetCacheKey(Object *, Object *, int *);
static Error       FreeCacheObject(Pointer);
static Error       NewCacheObject(Array, Array, CacheObject *);
static Stream      NewStreakBuf(int);
static void        DestroyStreakBuf(Stream);
static Field       StreakToField(Stream);
static Error       SetupStreakVars(StreakVars *, CacheObject);
static Error       FreeStreakVars(StreakVars *);
static int         Streak_FindElement(StreakVars *, POINT_TYPE *);
static int         Streak_FindMultiGridContinuation(StreakVars *, POINT_TYPE *);
static Error	   Streak_Interpolate(StreakVars *, POINT_TYPE *,
						double, VECTOR_TYPE *);
static Error	   Streak_StepTime(StreakVars *, double, VECTOR_TYPE *, double *);
static int         Streak_FaceWeights(StreakVars *, POINT_TYPE *);
static int         Streak_InsideWeights(StreakVars *, POINT_TYPE *);
static Error	   Streak_FindBoundary(StreakVars *, POINT_TYPE *,
						POINT_TYPE *, double *);
static int	   Streak_Neighbor(StreakVars *, VECTOR_TYPE *);
static StreakFlags AddPointToStreak(Stream, POINT_TYPE *, VECTOR_TYPE *, double);
static Error       GetTail(Object, char *, Pointer);
static int 	   IsSeries(Object o);
static Error       ResetVectorField(VectorField);
static Error       GeometryCheck(Object, int);

static VectorField InitVectorField(Object);
static Error       DestroyVectorField(VectorField);

Error
m_Streakline(Object *in, Object *out)
{
    Object 	  vectors, curls = NULL;
    int		  curlFlag = 0;
    float  	  c;
    int		  head;
    int		  frame;
    Type   	  type;
    Category 	  cat;
    int		  rank;
    int	   	  nD, n;
    CacheObject   cstreaks;
    int		  thisframe;
    int		  i, f;
    Group	  group  = NULL;
    CacheStreak	  streak;
    Array	  starts = NULL;
    Array	  times  = NULL;
    Class	  vClass;
    Object cachedObject = NULL;
    float termTime, seriesEnd=0;

    out[0] = NULL;

    if (! in[DATA_ARG])
    {
	DXSetError(ERROR_MISSING_DATA, "#10000", "data");
	goto error;
    }

    vectors = in[DATA_ARG];

    if ((vClass = DXGetObjectClass(vectors)) == CLASS_GROUP)
	vClass = DXGetGroupClass((Group)vectors);
    
    if (vClass != CLASS_FIELD && 
	vClass != CLASS_COMPOSITEFIELD && 
	vClass != CLASS_MULTIGRID && 
	vClass != CLASS_SERIES)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"data must be field, multigrid, composite field or series");
	goto error;
    }

    if (! DXGetType(vectors, &type, &cat, &rank, &nD))
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "data");
	goto error;
    }

    if (type != TYPE_FLOAT || cat != CATEGORY_REAL || (nD != 2 && nD != 3))
    {
	DXSetError(ERROR_BAD_PARAMETER, "#10302", "data");
	goto error;
    }

    if (! GeometryCheck(vectors, nD))
	goto error;

    if (IsSeries(vectors))
    {
	int i;
	float f;

	for (i = 0; DXGetSeriesMember((Series)vectors, i, &f); i++)
	    seriesEnd = f;

	head = i - 1;
    }

    /*
     * If head is given, then OK.  
     */
    if (in[HEAD_ARG])
    {
	if (! DXExtractInteger(in[HEAD_ARG], &head))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "head");
	    goto error;
	}
    }
    
    /*
     * Regarding frame: if its not given, then we run to head time.
     */
    if (in[FRAME_ARG])
    {
	if (! DXExtractInteger(in[FRAME_ARG], &frame))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "frame");
	    goto error;
	}
    }
    else
	frame = head;

    /*
     * If the input is a series, then the termination time should
     * be the series position of the frame'th member of the
     * series.  If the input is NOT a series, then its found
     * in the series position attribute.
     */
    if (IsSeries(vectors))
    {
	if (! DXGetSeriesMember((Series)vectors, frame, &termTime))
	    termTime = seriesEnd;
    }
    else
    {
	Object attr = DXGetAttribute((Object)vectors, "series position");
	if (! attr)
	{
	    DXSetError(ERROR_BAD_PARAMETER,
		"vector series member must have series position attribute");
	    goto error;
	}

	if (! DXExtractFloat(attr, &termTime))
	{
	    DXSetError(ERROR_DATA_INVALID, "series position attribute");
	    goto error;
	}
    }
	
    if (in[CURL_ARG])
    {
	int nDC;

	curls = in[CURL_ARG];
	if (! DXGetType(curls, &type, &cat, &rank, &nDC))
	    goto error;
	
	if (IsSeries(vectors) && !IsSeries(curls))
	{
	    DXSetError(ERROR_BAD_PARAMETER, 
		"since data is series curl must be series");
	    goto error;
	}
	
	if (type != TYPE_FLOAT || cat != CATEGORY_REAL || rank != 1)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "curls must be float/real vectors");
	    goto error;
	}

	if (nDC != nD)
	{
	    DXSetError(ERROR_BAD_PARAMETER, 
		"curl vectors must match data in dimensionality");
	    goto error;
	}

	curlFlag = 1;
    }

    if (in[FLAG_ARG])
    {
	if (!DXExtractInteger(in[FLAG_ARG], &curlFlag) ||
			(curlFlag < 0 || curlFlag > 1))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "curl flag must be 0 or 1");
	    goto error;
	}
    }

    if (in[SCALE_ARG])
    {
	if (! DXExtractFloat(in[SCALE_ARG], &c))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "c");
	    goto error;
	}

	if (c <= 0.0)
	    c = DEFAULT_C;
    }
    else
	c = DEFAULT_C;
    
    /*
     * Look in the cache to see if there is an object representing earlier 
     * calls to streakline.  
     */
    if (! GetCacheObject(in, &cstreaks, &cachedObject))
	goto error;

    if (cstreaks)
    {
	/* 
	 * if the current termination time is earlier than the 
	 * termination time of the streaks already computed, determine
	 * the frame number of the termination.
	 */
	if (cstreaks->frame > 0)
	{
	    if (termTime <= cstreaks->frameTimes[cstreaks->frame])
	    {
		for (frame = 0; frame <= cstreaks->frame; frame++)
		    if (termTime == cstreaks->frameTimes[frame])
			break;
	    
		if (frame == cstreaks->frame+1)
		{
		    DXSetError(ERROR_BAD_PARAMETER,
			"out-of-sequence series member");
		    goto error;
		}
	    }
	}
    }
    else
    {
	/*
	 * If there isn't a streak object, then we create a new one.
	 *
	 * Starting points are gathered from argument or centerpoint of
	 * first member of vector series
	 */
	starts = Starts(in[POINTS_ARG], in[DATA_ARG], nD);
	if (starts)
	    DXGetArrayInfo(starts, &n, NULL, NULL, NULL, NULL);
	if (! starts || n == 0)
	    goto no_streaks;
	
	if (! DXTypeCheck(starts, TYPE_FLOAT, CATEGORY_REAL, 1, nD))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "starts");
	    goto error;
	}

	/*
	 * Starting points are gathered from argument, or are extracted
	 * from the series position of the first member of the vector series.
	 */
	times = Times(in[TIMES_ARG], in[DATA_ARG]);
	if (! times)
	    goto error;
    
	/*
	 * Match up the starting points and times
	 */
	if (! AlignStartPtsAndTimes(starts, times))
	    goto error;
    
	if (! NewCacheObject(starts, times, &cstreaks))
	    goto error;
	
	DXDelete((Object)starts); starts = NULL;
	DXDelete((Object)times);  times  = NULL;
    }

    if (frame <= cstreaks->frame)
	goto past_head;

    thisframe = cstreaks->frame + 1;

    /* 
     * If a frame argument was given, then make sure its in sequence.
     * If frame argument was NOT given, then if the input is a series, frame
     * is the same as head, which is either given or the number of series 
     * members.  If the input is NOT a series, then frame is the current
     * frame only.
     */
    if (in[FRAME_ARG])
    {
	if (!IsSeries(vectors) && frame != thisframe)
	{
	    DXSetError(ERROR_BAD_PARAMETER, 
		"frame parameter given is out of sequence");
	    goto error;
	}

	if (frame > head)
	    frame = head;
    }
    else if (IsSeries(vectors))
	frame = head;
    else
	frame = thisframe;

    /*
     * The following loop updates the frame and generates streaks.
     */
    for (f = thisframe; f <= frame; f++)
    {
	if (! GetFrameData(vectors, curls, cstreaks))
	    goto error;
	
	if (! Streaklines(cstreaks, c, curlFlag))
	    goto error;
    }
    
    if (!cachedObject)
	if (NULL == (cachedObject = SetCacheObject(in, cstreaks)))
	    goto error;

past_head:

    streak = cstreaks->streaks;
    for (i = 0; i < cstreaks->nStreaks; i++, streak++)
    {
	Series s = NULL;
	Field  f;
	int    j, k;
	float  t;

	k = 0;
	for (j = 0; j <= frame; j++)
	{
	    f = (Field)DXGetSeriesMember(streak->streak, j, &t);
	    if (!f)
		break;

	    if (! DXEmptyField(f))
	    {
	        if (s == NULL)
	        {
		   s = DXNewSeries();
		   if (! s)
		       goto error;
		}

		if (! DXSetSeriesMember(s, k++, t, (Object)f))
		{
		    DXDelete((Object)s);
		    goto error;
		}
	    }
	}

	if (s)
	{
	    if (group == NULL)
	    {
		group = DXNewGroup();
		if (! group)
		{
		    DXDelete((Object)s);
		    goto error;
		}
	    }

	    if (! DXSetMember(group, NULL, (Object)s))
	    {
		DXDelete((Object)s);
		goto error;
	    }
	}
    }
    
    if (! group)
	out[0] = (Object)DXEndField(DXNewField());
    else
    {
	DXSetFloatAttribute((Object)group, "fuzz", 4);
	out[0] = (Object)group;
    }
    
    DXDelete(cachedObject);
    
    return OK;

no_streaks:
    DXDelete(cachedObject);
    DXDelete((Object)group);
    DXDelete((Object)starts);
    DXDelete((Object)times);
    out[0] = NULL;
    return OK;

error:
    DXDelete(cachedObject);
    DXDelete((Object)starts);
    DXDelete((Object)times);
    DXDelete((Object)group);
    out[0] = NULL;
    return ERROR;
}

typedef struct streakargs
{
    CacheObject cstreak;
    int		i;
    float       c;
} StreakArgs;

typedef struct frameargs
{
    int		 curlFlag;
    Interpolator i0, i1;
    float        t0, t1;
    CacheStreak  streak;
} FrameArgs;

static Error
Streaklines(CacheObject cstreak, float c, int curlFlag)
{
    int           i, n;
    StreakArgs    stask;
    FrameArgs     ftask;
    int		  nP;
    Interpolator  curlMap0 = NULL;
    Interpolator  curlMap1 = NULL;
    MultiGrid	  cObject0 = NULL;
    MultiGrid	  cObject1 = NULL;


    /*
     * If this is the 0-th frame, put an empty field into the streaks
     * Otherwise, put any necessary streaks.
     */
    if (cstreak->frame == 0)
    {
	CacheStreak streak = cstreak->streaks;
	Field       eField = DXEndField(DXNewField());

	for (i = 0; i < cstreak->nStreaks; i++, streak++)
	{
	    if (! DXSetSeriesMember(streak->streak, 0, cstreak->t1,
							(Object)eField))
		goto error;
	}
    }
    else
    {
	if (cstreak->t1 <- cstreak->t0)
	    cstreak->t1 = cstreak->t0;

	nP = DXProcessors(0);

	n = cstreak->nStreaks;
	if (n < 1)
	    return OK;

	if (n == 1 || nP == 1)
	{
	    for (i = 0; i < n; i++)
	    {
		if (! Streakline(cstreak, i, c))
		    goto error;
	    }
	}
	else
	{
	    stask.cstreak	= cstreak;
	    stask.c      	= c;

	    if (! DXCreateTaskGroup())
		goto error;
	    
	    for (i = 0; i < n; i++)
	    {
		stask.i = i;
	    
		if (! DXAddTask(StreakTask, (Pointer)&stask, sizeof(stask), 1.0))
		{
		    DXAbortTaskGroup();
			goto error;
		}
	    }

	    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		goto error;
	}

	if (cstreak->nDim == 3)
	{
	    if (curlFlag)
	    {
		if (cstreak->c0)
		    curlMap0 = cstreak->c0;
		else
		{
		    int i;

		    cObject0 = DXNewMultiGrid();
		    if (! cObject0)
			goto error;

		    for (i = 0; i < cstreak->vf0->nmembers; i++)
		    {
			VectorGrp vg = cstreak->vf0->members[i];

			if (! (*(vg->CurlMap))(vg, cObject0))
			    goto error;
		    }

		    curlMap0 = DXNewInterpolator((Object)cObject0, 	
					INTERP_INIT_IMMEDIATE, -1.0);
		    
		    if (! curlMap0)
			goto error;
		}
		
		DXReference((Object)curlMap0);

		if (cstreak->c1)
		    curlMap1 = cstreak->c1;
		else
		{
		    int i;

		    cObject1 = DXNewMultiGrid();
		    if (! cObject1)
			goto error;

		    for (i = 0; i < cstreak->vf1->nmembers; i++)
		    {
			VectorGrp vg = cstreak->vf1->members[i];

			if (! (*(vg->CurlMap))(vg, cObject1))
			    goto error;
		    }

		    curlMap1 = DXNewInterpolator((Object)cObject1, 	
					INTERP_INIT_IMMEDIATE, 0.0);
		    
		    if (! curlMap1)
			goto error;
		}
		
		DXReference((Object)curlMap1);

		if (! curlMap0 || ! curlMap1)
		{
		    DXSetError(ERROR_INTERNAL, "missing curl map");
		    goto error;
		}

		ftask.curlFlag = 1;
		ftask.i0 = curlMap0;
		ftask.i1 = curlMap1;
	    }
	    else
	    {
		ftask.curlFlag = 0;
		ftask.i0 = NULL;
		ftask.i1 = NULL;
	    }

	    ftask.t0 = cstreak->t0;
	    ftask.t1 = cstreak->t1;

	    if (n == 1 || nP == 1)
	    {
		for (i = 0; i < n; i++)
		{
		    ftask.streak = cstreak->streaks + i;
		    if (! TraceFrame((Pointer)&ftask))
			goto error;
		}
	    }
	    else
	    {
		if (! DXCreateTaskGroup())
		    goto error;
		
		for (i = 0; i < n; i++)
		{
		    ftask.streak = cstreak->streaks + i;
		    if (! DXAddTask(TraceFrame, (Pointer)&ftask,
							sizeof(ftask), 1.0))
		    {
			DXAbortTaskGroup();
			goto error;
		    }
		}
		
		if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
		    goto error;
	    }

	    DXDelete((Object)curlMap0);
	    DXDelete((Object)curlMap1);
	    curlMap0 = curlMap1 = NULL;
	}
    }

    return OK;

error:
    if (cObject0)
	DXDelete((Object)cObject0);
    if (cObject1)
	DXDelete((Object)cObject1);
    if (curlMap0)
	DXDelete((Object)curlMap0);
    if (curlMap1)
	DXDelete((Object)curlMap1);
    return ERROR;
}

static Error
StreakTask(Pointer p)
{
    StreakArgs *t = (StreakArgs *)p;

    return Streakline(t->cstreak, t->i, t->c);
}

static Error
Streakline(CacheObject cstreak,	/* accumulated streak info		*/
	   int	       which,	/* which to operate on			*/
	   float       c)	/* step criterion			*/
{
    double	 elapsedtime = 0;
    float	 head	     = cstreak->t1;
    CacheStreak  streak	     = cstreak->streaks + which;
    int		 nDim 	     = cstreak->nDim;
    Stream 	 streakBuf   = NULL;
    POINT_TYPE	 pb0[3], pb1[3];
    VECTOR_TYPE	 vb0[3], vb1[3];
    POINT_TYPE 	 *p0, *p1, *ptmp;
    VECTOR_TYPE	 *v0, *v1, *vtmp;
    double     	 t, t2;
    int		 done, keepPoint;
    Field 	 field = NULL;
    int 	 i, totKnt;
    StreakFlags  sFlag;
    StreakVars   svars;
    int		 zeroVector;

    memset(&svars, 0, sizeof(svars));

    /*
     * Double buffer pointers
     */
    p0 = pb0; p1 = pb1;
    v0 = vb0; v1 = vb1;

    /*
     * If streak has already died, well, OK.
     */
    if (streak->status == STREAK_DEAD)
	return OK;

    /*
     * If streak is awaiting birth and its start time is after the current
     * interval, well, OK.  But got to put an empty field marker in.
     */
    if (streak->status == STREAK_PENDING && streak->time > head)
	goto streak_done;

    /*
     * If streak is alive, then we need to get its current termination point.
     * If the streak is newborn, then the point is already in the streak
     * header.  Otherwise, we need to get it from the fragment already
     * computed.
     */
    if (streak->status == STREAK_ALIVE)
    {
	float fbuf[3];
	GetTail((Object)streak->streak,"positions",(Pointer)&fbuf[0]);
	for (i = 0; i < nDim; i++)
	    streak->point[i] = fbuf[i];
    }
    else
	streak->status = STREAK_ALIVE;

    for (i = 0; i < nDim; i++)
	p0[i] = streak->point[i];

    if (streak->time > cstreak->t0)
	elapsedtime = streak->time;
    else 
	elapsedtime = cstreak->t0;

    if (! SetupStreakVars(&svars, cstreak))
	goto error;

    nDim = cstreak->nDim;

    /*
     * If we can't find the start, we can't extend the streak.
     */
    if (! Streak_FindElement(&svars, p0))
    {   
	streak->status = STREAK_DEAD;
	goto streak_done;
    }
	    
    if (! Streak_Interpolate(&svars, p0, elapsedtime, v0))
	goto error;
    
    streakBuf = NewStreakBuf(nDim);
    if (! streakBuf)
	goto error;

    sFlag = AddPointToStreak(streakBuf, p0, v0, elapsedtime);
    if (sFlag == STREAK_ERROR)
	goto error;

    /*
     * While streak is passing from partition to partition...
     */
    done = 0; totKnt = 1;
    while (!done)
    {
	zeroVector = ZeroVector(v0, nDim);

	if (zeroVector)
	{
	    /*
	     * Interpolate this point at t1.  If its zero there too,
	     * then it'll be zero everywhere in between (due to linear
	     * interpolation in time.
	     */

	    if (! Streak_Interpolate(&svars, p0, svars.t1, v1))
		goto error;
	    
	    if (ZeroVector(v1, nDim))
	    {
		sFlag = AddPointToStreak(streakBuf, p0, v0, svars.t1);
		if (sFlag == STREAK_ERROR)
		    goto error;
		
		streak->status = STREAK_ALIVE;
		goto streak_done;
	    }
	    else
	    {
		/*
		 * If it isn't zero at t1, then take a small step
		 * in time to get off zero.
		 */
		while (zeroVector)
		{
		    elapsedtime += 0.025 * (svars.t1 - elapsedtime);

		    if (! Streak_Interpolate(&svars, p0, elapsedtime, v0))
			goto error;

		    zeroVector = ZeroVector(v0, nDim);
		}
	    }
	}
	
	if (! Streak_StepTime(&svars, c, v0, &t))
	    goto error;
	
	/*
	 * If the time step exceeds the head, truncate it and mark
	 * the streak complete.
	 */
	if (elapsedtime + t > svars.t1)
	{
	    t = svars.t1 - elapsedtime;
	    streak->status = STREAK_ALIVE;
	    done = 1;
	}

	/*
	 * Get the point a time step away in the direction of the
	 * vector
	 */
	if (! zeroVector)
	    for (i = 0; i < nDim; i++)
	    {
		p1[i] = p0[i] + t*v0[i];
		if (p1[i] == p0[i])
		    v0[i] = 0.0;
	    }

	/*
	 * If the new point is not in the current element, find the point
	 * at which it exits the element and iterponlate the point there.  
	 * Then move on to the neighbor, if one exists.
	 *
	 * Otherwise, just interpolate the endpoint.
	 */
	if (! Streak_InsideWeights(&svars, p1))
	{
	    int   found;

	    /*
	     * If the new sample isn't in the previous element, find the
	     * exit point of the streak and interpolate in the current 
	     * element there.
	     */
	    if (! Streak_FindBoundary(&svars, p0, v0, &t2))
		goto error;
	    
	    elapsedtime += t2;

	    for (i = 0; i < nDim; i++)
	    {
		double d = ((double)p0[i]) + ((double)t2)*((double)v0[i]);
		p1[i] = d;
	    }

	    if (! Streak_FaceWeights(&svars, p1))
		goto error;

	    if (! Streak_Interpolate(&svars, p1, elapsedtime,  v1))
		goto error;

	    found = Streak_Neighbor(&svars, v1);
	    if (found == -1)
		goto error;
	    if (! found)
		found = Streak_FindMultiGridContinuation(&svars, p0);
	    if (! found)
	    {
		streak->status = STREAK_DEAD;
		done = 1;
	    }
	    else
		done = 0;

	    if (done || t2 > 0)
		keepPoint = 1;
	    else
		keepPoint = 0;

	    t = t2;
	}
	else
	{
	    keepPoint = 1;
	    elapsedtime += t;

	    if (! Streak_Interpolate(&svars, p1, elapsedtime, v1))
		goto error;
	}

	if (keepPoint && t > 0.0)
	{
	    sFlag = AddPointToStreak(streakBuf, p1, v1, elapsedtime);
	    if (sFlag == STREAK_ERROR)
		goto error;
	    else if (sFlag == STREAK_FULL)
		done = 1;
	}

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

streak_done:

    if (streakBuf)
    {
	field = StreakToField(streakBuf);
	if (! field)
	    goto error;

	DestroyStreakBuf(streakBuf);
	streakBuf = NULL;
    }
    else
	field = DXEndField(DXNewField());
    
    if (! DXSetSeriesMember(streak->streak, cstreak->frame,
						svars.t0, (Object)field))
	goto error;
    
    FreeStreakVars(&svars);
    
    return OK;

error:
    FreeStreakVars(&svars);
    DestroyStreakBuf(streakBuf);
    DXDelete((Object)field);

    return ERROR;
}

static Error
TraceFrame(Pointer ptr)
{
    Interpolator i0      = ((FrameArgs *)ptr)->i0;
    Interpolator i1      = ((FrameArgs *)ptr)->i1;
    float        t0      = ((FrameArgs *)ptr)->t0;
    float        t1      = ((FrameArgs *)ptr)->t1;
    CacheStreak  cs      = ((FrameArgs *)ptr)->streak;
    int          flag    = ((FrameArgs *)ptr)->curlFlag;
    Series	 s       = cs->streak;
    Array        cArray0 = NULL;
    Array        cArray1 = NULL;
    Field        field   = NULL;
    Object	 next;
    Field	 prev;
    Array        nArray = NULL, bArray = NULL;
    Array        vArray, tArray, pArray;
    float        *points, *vecarr, *curls0=NULL, *curls1=NULL,
    		 *normals, *binormals, *time;
    int          i, nVectors, nDim, seg;
    Object       dattr = NULL;
    Vector       fn, fb, ft, vectors;

    field = NULL; prev = NULL;
    for (seg = 0; NULL != (next = DXGetSeriesMember(s, seg, NULL)); seg++)
    {
	if (field && ! DXEmptyField(field))
	    prev = field;
	field = (Field)next;
    }

    if (DXEmptyField(field))
	return OK;

    pArray = (Array)DXGetComponentValue(field, "positions");
    if (! pArray)
	goto error;
    
    points = (float *)DXGetArrayData(pArray);

    if (flag)
    {
        i0 = (Interpolator)DXCopy((Object)i0, COPY_STRUCTURE);
	if (! i0)
	    goto error;

	cArray0 = DXMapArray(pArray, i0, NULL);
	if (! cArray0)
	    goto error;
	
	DXDelete((Object)i0);

        i1 = (Interpolator)DXCopy((Object)i1, COPY_STRUCTURE);
	if (! i1)
	    goto error;

	cArray1 = DXMapArray(pArray, i1, NULL);
	if (! cArray1)
	    goto error;

	DXDelete((Object)i1);
    
	curls0 = (float *)DXGetArrayData(cArray0);
	curls1 = (float *)DXGetArrayData(cArray1);
	if (! curls0 || ! curls1)
	    goto error;
    }

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

    tArray = (Array)DXGetComponentValue(field, "time");
    if (! tArray)
	goto error;

    time = (float *)DXGetArrayData(tArray);
    if (! time)
	goto error;

    nArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! nArray)
	goto error;
    
    if (! DXAddArrayData(nArray, 0, nVectors, NULL))
	goto error;
    
    normals = (float *)DXGetArrayData(nArray);
    if (! normals)
	goto error;
    
    bArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! bArray)
	goto error;
    
    if (! DXAddArrayData(bArray, 0, nVectors, NULL))
	goto error;
    
    binormals = (float *)DXGetArrayData(bArray);
    if (! binormals)
	goto error;
    
    /*
     * If this is not the first segment of the stream, we need to get
     * the propagated frame of reference and initial vertex normal and
     * binormal from the cached stuff.  Otherwise, we initialize the 
     * process.
     */
    if (prev)
    {
	fn.x = cs->norm[0];   fn.y = cs->norm[1];   fn.z = cs->norm[2];
	fb.x = cs->binorm[0]; fb.y = cs->binorm[1]; fb.z = cs->binorm[2];
	ft.x = cs->tan[0];    ft.y = cs->tan[1];    ft.z = cs->tan[2];

	GetTail((Object)prev, "normals", (Pointer)normals);
	GetTail((Object)prev, "binormals", (Pointer)binormals);
    }
    else
    {
	if (! InitFrame(&vectors, &fn, &fb))
	    goto error;

	vecarr[0] = vectors.x; vecarr[1] = vectors.y; vecarr[2] = vectors.z;
	
	ft.x = vectors.x; ft.y = vectors.y; ft.z = vectors.z;

	_dxfvector_normalize_3D(&fn, &fn);
	_dxfvector_normalize_3D(&fb, &fb);
	_dxfvector_normalize_3D(&ft, &ft);

	normals[0]   = fn.x; normals[1]   = fn.y; normals[2]   = fn.z;
	binormals[0] = fb.x; binormals[1] = fb.y; binormals[2] = fb.z;
    }
    
    for (i = 1; i < nVectors; i++)
    {
	points += 3; time += 1; vecarr += 3; normals += 3; binormals += 3;

	if (flag)
	{
	    float avcurl[3], pcurl[3], ncurl[3], d;

	    d = (*time - t0) / (t1 - t0);

	    pcurl[0] = curls0[0] + d*(curls1[0] - curls0[0]);
	    pcurl[1] = curls0[1] + d*(curls1[1] - curls0[1]);
	    pcurl[2] = curls0[2] + d*(curls1[2] - curls0[2]);

	    ncurl[0] = curls0[3] + d*(curls1[3] - curls0[3]);
	    ncurl[1] = curls0[4] + d*(curls1[4] - curls0[4]);
	    ncurl[2] = curls0[5] + d*(curls1[5] - curls0[5]);

	    avcurl[0] = 0.5 * (pcurl[0] + ncurl[0]);
	    avcurl[1] = 0.5 * (pcurl[1] + ncurl[1]);
	    avcurl[2] = 0.5 * (pcurl[2] + ncurl[2]);
	    
	    curls0 += 3;
	    curls1 += 3;

	    if (! UpdateFrame(points, vecarr, avcurl, time, normals,
				binormals, &fn, &fb, &ft))
		goto error;
	}
	else
	{
	    if (! UpdateFrame(points, vecarr, NULL, time, normals,
				binormals, &fn, &fb, &ft))
		goto error;
	}
    }

    cs->norm[0]   = fn.x; cs->norm[1]   = fn.y; cs->norm[2]   = fn.z;
    cs->binorm[0] = fb.x; cs->binorm[1] = fb.y; cs->binorm[2] = fb.z;
    cs->tan[0]    = ft.x; cs->tan[1]    = ft.y; cs->tan[2]    = ft.z;

    dattr = (Object)DXNewString("positions");
    if (! dattr)
	goto error;
    DXReference(dattr);

    if (! DXSetComponentValue(field, "normals", (Object)nArray))
	goto error;
    nArray = NULL;

    if (! DXSetComponentAttribute(field, "normals", "dep", dattr))
	goto error;

    if (! DXSetComponentValue(field, "binormals", (Object)bArray))
	goto error;
    bArray = NULL;

    if (! DXSetComponentAttribute(field, "binormals", "dep", dattr))
	goto error;

    DXDelete(dattr);
    DXDelete((Object)cArray0);
    DXDelete((Object)cArray1);
    
done:
    return OK;

error:
    DXDelete((Object)nArray);
    DXDelete((Object)bArray);
    DXDelete((Object)cArray0);
    DXDelete((Object)cArray1);

    return ERROR;
}

static Array
Starts(Object starts, Object vectors, int nD)
{
    Array array = NULL;

    if (starts)
    {
	Object xstarts = DXApplyTransform(starts, NULL);
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

static Array
Times(Object times, Object vectors)
{
    Array array = NULL;

    if (times)
    {
	if (! StartsRecurse(times, &array))
	    goto error;
    }
    else if (vectors)
    {
	float time;

	if (IsSeries(vectors))
	    DXGetSeriesMember((Series)vectors, 0, &time);
	else
	{
	    Object attr = DXGetAttribute(vectors, "series position");

	    if (attr == NULL)
		DXSetError(ERROR_MISSING_DATA,
			"series position attribute on data");
	    
	    if (! DXExtractFloat(attr, &time))
		DXSetError(ERROR_DATA_INVALID,
			"series position attribute on data");
	}

	array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	if (! array)
	    goto error;
	
	if (! DXAddArrayData(array, 0, 1, (Pointer)&time))
	    goto error;

    }
    else
    {
	float *ptr;

	array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1);
	if (! array)
	    goto error;
	
	if (! DXAddArrayData(array, 0, 1, NULL))
	    goto error;
	
	ptr = (float *)DXGetArrayData(array);
	if (! ptr)
	    goto error;
	
	*ptr = 0.0;
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
			DXSetError(ERROR_DATA_INVALID, "vectors");
			goto error;
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
		    DXSetError(ERROR_DATA_INVALID, "vectors");
		    goto error;
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
	    return OK;

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
NewStreakBuf(int nDim)
{
    Stream s = NULL;

    s = (Stream)DXAllocateZero(sizeof(struct stream));
    if (! s)
	goto error;
    
    s->pArray  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nDim);
    s->tArray  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
    s->vArray  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, nDim);

    if (s->pArray == NULL || s->vArray == NULL || s->tArray == NULL)
	goto error;

    s->arrayKnt = 0;

    s->points  = (float *)DXAllocate(nDim*STREAM_BUF_POINTS*sizeof(float));
    s->time    = (float *)DXAllocate(STREAM_BUF_POINTS*sizeof(float));
    s->vectors = (float *)DXAllocate(nDim*STREAM_BUF_POINTS*sizeof(float));
    if (s->points  == NULL || s->time == NULL || s->vectors == NULL)
	goto error;

    s->nDim     = nDim;
    s->bufKnt   = 0;

    return s;

error:
    DestroyStreakBuf(s);

    return NULL;
}

static void
DestroyStreakBuf(Stream s)
{
    if (s)
    {
	DXDelete((Object)s->pArray);
	DXDelete((Object)s->vArray);
	DXDelete((Object)s->tArray);
	DXFree((Pointer)s->points);
	DXFree((Pointer)s->time);
	DXFree((Pointer)s->vectors);
	DXFree((Pointer)s);
    }
}

static Error
FlushStreak(Stream s)
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

static StreakFlags
AddPointToStreak(Stream s, POINT_TYPE *p, VECTOR_TYPE *v, double t)
{
    if (s->bufKnt == STREAM_BUF_POINTS)
        if (! FlushStreak(s))
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

    if (s->bufKnt + s->arrayKnt >= STREAK_MAX)
	return STREAK_FULL;

    return STREAK_OK;

error:
    return STREAK_ERROR;
}

static Field
StreakToField(Stream buf)
{
    float c[3];
    float d[3];
    Array a = NULL;
    Object dattr = NULL;
    Field field;

    if (! FlushStreak(buf))
	goto error;

    c[0] = 0.7; c[1] = 0.7; c[2] = 0.0;
    d[0] = 0.0; d[1] = 0.0; d[2] = 0.0;

    dattr = (Object)DXNewString("positions");
    if (! dattr)
	goto error;
    
    DXReference(dattr);

    field = DXNewField();
    if (! field)
	goto error;

    if (! DXSetComponentValue(field, "positions", (Object)buf->pArray))
	goto error;
    buf->pArray = NULL;

    if (! DXSetComponentAttribute(field, "positions", "dep", dattr))
	goto error;

    if (! DXSetComponentValue(field, "data", (Object)buf->vArray))
	goto error;
    buf->vArray = NULL;

    if (! DXSetComponentAttribute(field, "data", "dep", dattr))
	goto error;

    if (! DXSetComponentValue(field, "time", (Object)buf->tArray))
	goto error;
    buf->tArray = NULL;

    if (! DXSetComponentAttribute(field, "time", "dep", dattr))
	goto error;

    DXDelete(dattr);

    a = DXMakeGridConnections(1, buf->arrayKnt);
    if (! a)
	goto error;

    if (! DXSetComponentValue(field, "connections", (Object)a))
	goto error;
    a = NULL;

    if (! DXSetComponentAttribute(field, "connections", "element type",
						(Object)DXNewString("lines")))
	goto error;
    
    a = (Array)DXNewRegularArray(TYPE_FLOAT, 3,
			buf->arrayKnt, (Pointer)c, (Pointer)d);
    if (! a)
	goto error;

    if (! DXSetComponentValue(field, "colors", (Object)a))
	goto error;
    a = NULL;

    if (! DXSetIntegerAttribute((Object)field, "shade", 0))
	goto error;

    if (! DXEndField(field))
	goto error;

    return field;

error:
    DXDelete(dattr);
    return ERROR;
}

static int
ZeroVector(VECTOR_TYPE *v, int n)
{
    int i;

    for (i = 0; i < n; i++)
	if (v[i] != 0.0) return 0;
    
    return 1;
}

static Error
InitFrame(Vector *v, Vector *n, Vector *b)
{
    n->x = n->y = n->y = 0.0;

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

    _dxfvector_cross_3D(v, n, b);
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
UpdateFrame(float *p, float *v, float *c, float *t, float *n, float *b, 
					Vector *fn, Vector *fb, Vector *ft)
{
    float  twist;
    float  mTwist[9], mBend[9];
    float  len, cA;
    int	   bend = 0;
    Vector v1, v2, vv, cross, nt;


    /*
     * If theres a curl, rotate by the curl around the prior segment vector.
     */
    if (c != NULL)
    {
	Vector step;
	Vector vc;
	
	vc.x = c[0]; vc.y = c[1]; vc.z = c[2];

	v1.x = p[0]; v1.y = p[1]; v1.z = p[2];
	v2.x = (p-3)[0]; v2.y = (p-3)[1]; v2.z = (p-3)[2];
	_dxfvector_subtract_3D(&v1, &v2, &step);

	twist = 0.5 * _dxfvector_dot_3D(&step, &vc);
	if (twist != 0.0)
	{
		float fna[3], fba[3];
		fna[0] = fn->x; fna[1] = fn->y; fna[2] = fn->z;
		fba[0] = fb->x; fba[1] = fb->y; fba[2] = fb->z;
		
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
    vv.x = v[0]; vv.y = v[1]; vv.z = v[2];
    len = _dxfvector_length_3D(&vv);
    if (len == 0.0)
    {
	n[0] = fn->x; n[1] = fn->y; n[2] = fn->z;
	b[0] = fb->x; b[1] = fb->y; b[2] = fb->z;
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
	n[0] = fn->x; n[1] = fn->y; n[2] = fn->z;
	b[0] = fb->x; b[1] = fb->y; b[2] = fb->z;

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
	n[0] = fn->x; n[1] = fn->y; n[2] = fn->z;
	b[0] = fb->x; b[1] = fb->y; b[2] = fb->z;
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
NewCacheObject(Array pArray, Array tArray, CacheObject *co)
{
    CacheStreak cs = NULL;
    float       *points, *times;
    int		nDim, nStreaks, i, j;

    *co = NULL;

    points = (float *)DXGetArrayData(pArray);
    if (! points)
	goto error;
    
    times = (float *)DXGetArrayData(tArray);
    if (! times)
	goto error;

    if (! DXGetArrayInfo(pArray, &nStreaks, NULL, NULL, NULL, &nDim))
	goto error;

    *co = (CacheObject)DXAllocateZero(sizeof(struct cacheObject));
    if (! *co)
	goto error;
    
    cs = (CacheStreak)DXAllocateZero(nStreaks*sizeof(struct cacheStreak));
    if (!cs)
	goto error;

    (*co)->streaks  = cs;
    (*co)->nDim     = nDim;

    for (i = 0; i < nStreaks; i++, cs++)
    {
	for (j = 0; j < nDim; j++)
	    cs->point[j] = *points++;
	cs->time   = *times++;
	cs->status = STREAK_PENDING;
	cs->start  = 0;
	cs->streak = DXNewSeries();
       if (cs->streak == NULL)
	   goto error;
    }

    /*
     * Note: no frame data is currently contained in the following
     * structure
     */
    (*co)->frame      = -1;
    (*co)->nStreaks   = nStreaks;
    (*co)->frameTimes = NULL;


    return OK;

error:
    FreeCacheObject(*co);
    return ERROR;
}

static Error
FreeCacheObject(Pointer ptr)
{
    int i;
    CacheObject co = ptr;

    if (co)
    {
	if (co->streaks)
	{
	    for (i = 0; i < co->nStreaks; i++)
		if (co->streaks[i].streak)
		    DXDelete((Object)co->streaks[i].streak);
	    DXFree((Pointer)co->streaks);
	}

	if (co->frameTimes)
	    DXFree(co->frameTimes);

	if (co->v0)
	    DXDelete(co->v0);
	if (co->c0)
	    DXDelete((Object)co->c0);
	if (co->vf0)
	    DestroyVectorField(co->vf0);

	if (co->v1)
	    DXDelete(co->v1);
	if (co->c1)
	    DXDelete((Object)co->c1);
	if (co->vf1)
	    DestroyVectorField(co->vf1);

	DXFree((Pointer)co);
    }

    return OK;
}

static void
GetCacheKey(Object *in, Object *keys, int *nk)
{
    int n = 0;

    if (in[DATA_ARG] && IsSeries(in[DATA_ARG]))
	keys[n++] = in[DATA_ARG];

    if (in[NAME_ARG])   keys[n++] = in[NAME_ARG];
    if (in[POINTS_ARG]) keys[n++] = in[POINTS_ARG];
    if (in[TIMES_ARG])  keys[n++] = in[TIMES_ARG];
    if (in[HEAD_ARG])   keys[n++] = in[HEAD_ARG];
    if (in[FLAG_ARG])   keys[n++] = in[FLAG_ARG];
    if (in[SCALE_ARG])  keys[n++] = in[SCALE_ARG];
    
    *nk = n;
}

static Error
GetCacheObject(Object *in, CacheObject *co, Object *obj)
{
    int	        nk;
    Object      keys[8];
    Private	pco;
    int		hold = 0;


    if (in[HOLD_ARG])
	if (!DXExtractInteger(in[HOLD_ARG], &hold) || hold < 0 || hold > 1)
	{
	    DXSetError(ERROR_BAD_PARAMETER, "hold must be integer flag");
	    return ERROR;
	}
    
    if (hold)
    {
	int frame;

	if (! in[FRAME_ARG])
	{
	    DXSetError(ERROR_BAD_PARAMETER, "if hold is set, frame is required");
	    return ERROR;
	}
	  
	if (!DXExtractInteger(in[FRAME_ARG], &frame))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "frame must be integer");
	    return ERROR;
	}

	if (frame != 0)
	{
	    pco = (Private)DXGetCacheEntry("Streakline", 0, 1, in[NAME_ARG]);
	    if (! pco)
	    {
		DXSetError(ERROR_BAD_PARAMETER, 
		    "hold set and frame not 0, but no cached state is present");
		return ERROR;
	    }
	}
	else
	{
	    DXSetCacheEntry(NULL, CACHE_PERMANENT,
			    "Streakline", 0, 1, in[NAME_ARG]);
	    pco = NULL;
	}
    }
    else
    {
	GetCacheKey(in, keys, &nk);
	pco = (Private)DXGetCacheEntryV("Streakline", 0, nk, keys);
    }

    if (! pco)
    {
	*co = NULL;
	*obj = NULL;
    }
    else
    {
	
	*co = (CacheObject)DXGetPrivateData(pco);
	*obj =  DXReference((Object)pco);
    }

    return OK;
}

static Object
SetCacheObject(Object *in, CacheObject co)
{
    Private	pco = NULL;
    Object 	keys[8];
    int		nk;

    pco = DXNewPrivate((Pointer)co, FreeCacheObject);
    if (! pco)
	return ERROR;
    
    DXReference((Object)pco);

    if (in[HOLD_ARG])
    {
	if (! DXSetCacheEntry((Object)pco, CACHE_PERMANENT,
						"Streakline", 0, 1, in[NAME_ARG]))
	    return ERROR;
    }
    else
    {
	GetCacheKey(in, keys, &nk);

	if (! DXSetCacheEntryV((Object)pco, CACHE_PERMANENT,
					"Streakline", 0, nk, keys))
	    goto error;
    }

    return (Object)pco;

error:
    DXDelete((Object)pco);
    return NULL;
}

static Error
GetTail(Object o, char *name, Pointer l)
{
    Field f;
    Object next;
    int n, i;
    byte *ptr;
    int  size;
    Array a;

    if (DXGetObjectClass(o) == CLASS_FIELD)
    {
	f = (Field)o;
    }
    else /* object is a series */
    {
	f = NULL;
	for (i = 0; NULL != (next = DXGetSeriesMember((Series)o, i, NULL)); i++)
	    f = (Field)next;
    }

    if (! f || DXEmptyField(f))
    {
	DXSetError(ERROR_INTERNAL, "GetTail called with bad field");
	return ERROR;
    }

    a = (Array)DXGetComponentValue(f, name);
    if (! a)
    {
	DXSetError(ERROR_INTERNAL, "GetTail: no component %s in field", name);
	return ERROR;
    }

    size = DXGetItemSize(a);
    DXGetArrayInfo(a, &n, NULL, NULL, NULL, NULL);

    ptr = ((byte *)DXGetArrayData(a)) + (n-1)*size;
    memcpy(l, ptr, size);

    return OK;
}
	
static Error
SetupStreakVars(StreakVars *svars, CacheObject cstreak)
{
    svars->I0 = svars->I1 = NULL;

    svars->vf0 = cstreak->vf0;
    svars->vf1 = cstreak->vf1;

    svars->t0 = cstreak->t0;
    svars->t1 = cstreak->t1;

    svars->inside0 = svars->inside1 = 0;

    svars->nDim = cstreak->nDim;

    return OK;
}

static Error
FreeStreakVars(StreakVars *svars)
{
    if (svars->I0)
	(*(svars->I0->currentVectorGrp->FreeInstanceVars))(svars->I0);
    if (svars->I1)
	(*(svars->I1->currentVectorGrp->FreeInstanceVars))(svars->I1);
    svars->I0 = svars->I1 = NULL;
    return OK;
}

static Error
GetFrameData(Object vectors, Object curls, CacheObject co)
{
    Object curl = NULL;

    co->frame ++;

    if (co->v0)
	DXDelete((Object)co->v0);
    co->v0  = co->v1;

    if (co->c0)
	DXDelete((Object)co->c0);
    co->c0 = co->c1;

    if (co->vf0)
    {
	DestroyVectorField(co->vf0);
	co->vf0 = NULL;
    }

    if (co->vf1)
    {
	co->vf0 = co->vf1;
	if (! ResetVectorField(co->vf0))
	    goto error;
    }

    co->t0  = co->t1;

    if (IsSeries(vectors))
    {
	co->v1 = DXGetSeriesMember((Series)vectors, co->frame, &co->t1);
	if (! co->v1)
	{
	    DXSetError(ERROR_MISSING_DATA, "data: empty series group");
	    goto error;
	}

	co->v1 = DXReference(DXCopy(co->v1, COPY_STRUCTURE));
	if (! co->v1)
	    goto error;
	
	if (curls)
	{
	    float t;

	    curl = DXGetSeriesMember((Series)curls, co->frame, &t);
	    if (! curl)
	    {
		DXSetError(ERROR_MISSING_DATA, "curl: empty series group");
		goto error;
	    }

	    if (t != co->t1)
	    {
		DXSetError(ERROR_DATA_INVALID, 
		    "curl and data series positions do not match");
		goto error;
	    }
	}
    }
    else
    {
	Object attr;

	attr = DXGetAttribute(vectors, "series position");
	if (! attr)
	{
	    DXSetError(ERROR_MISSING_DATA, "series position");
	    goto error;
	}

	if (! DXExtractFloat(attr, &co->t1))
	{
	    DXSetError(ERROR_DATA_INVALID, "series position");
	    goto error;
	}

	co->v1 = DXReference(DXCopy(vectors, COPY_STRUCTURE));
	if (! co->v1)
	    goto error;
	
	curl = curls;
	
    }

    if (curl)
    {
	co->c1 = DXNewInterpolator(curl, INTERP_INIT_PARALLEL, 0.0);
	if (! co->c1)
	    goto error;
	
	DXReference((Object)co->c1);
    }

    co->vf1 = InitVectorField(co->v1);
    if (! co->vf1)
	goto error;

    if (co->frameTimes)
	co->frameTimes =
		(float *)DXReAllocate(co->frameTimes,
					(co->frame+1)*sizeof(float));
    else
	co->frameTimes =
		(float *)DXAllocate((co->frame+1)*sizeof(float));

    if (! co->frameTimes)
	goto error;
	
    co->frameTimes[co->frame] = co->t1;

    return OK;

error:
    return ERROR;
}

static InstanceVars
Streak_FindElement_1(VectorField vf, POINT_TYPE *point, VectorGrp last)
{
    InstanceVars I;
    int i;

    for (i = 0; i < vf->nmembers; i++)
	if (vf->members[i] != last)
	{
	    I = (*(vf->members[i]->NewInstanceVars))(vf->members[i]);

	    if ((*(I->currentVectorGrp->FindElement))(I, point))
	    {
		vf->current = vf->members[i];
		return I;
	    }

	    (*(I->currentVectorGrp->FreeInstanceVars))(I);
	}

    return 0;
}

static InstanceVars
Streak_FindMultiGridContinuation_1(VectorField vf, POINT_TYPE *point, VectorGrp last)
{
    InstanceVars I;
    int i;

    for (i = 0; i < vf->nmembers; i++)
	if (vf->members[i] != last)
	{
	    I = (*(vf->members[i]->NewInstanceVars))(vf->members[i]);

	    if ((*(I->currentVectorGrp->FindMultiGridContinuation))(I, point))
	    {
		vf->current = vf->members[i];
		return I;
	    }

	    (*(I->currentVectorGrp->FreeInstanceVars))(I);
	}

    return 0;
}

static int
Streak_FindElement(StreakVars *svars, POINT_TYPE *p)
{
    if (! svars->inside0)
    {
	VectorGrp last;

	if (svars->I0)
	{
	    last = svars->I0->currentVectorGrp;
	    (*(last->FreeInstanceVars))(svars->I0);
	}
	else
	    last = NULL;

	svars->I0 = Streak_FindElement_1(svars->vf0, p, last);
	if (! svars->I0)
	    return 0;
    }

    if (! svars->inside1)
    {
	VectorGrp last;

	if (svars->I1)
	{
	    last = svars->I1->currentVectorGrp;
	    (*(last->FreeInstanceVars))(svars->I1);
	}
	else
	    last = NULL;

	svars->I1 = Streak_FindElement_1(svars->vf1, p, last);
	if (! svars->I1)
	    return 0;
    }
    return 1;
}


static int
Streak_FindMultiGridContinuation(StreakVars *svars, POINT_TYPE *p)
{
    if (! svars->inside0)
    {
	VectorGrp last;

	if (svars->I0)
	{
	    last = svars->I0->currentVectorGrp;
	    (*(last->FreeInstanceVars))(svars->I0);
	}
	else
	    last = NULL;

	svars->I0 = Streak_FindMultiGridContinuation_1(svars->vf0, p, last);
	if (! svars->I0)
	    return 0;
    }

    if (! svars->inside1)
    {
	VectorGrp last;

	if (svars->I1)
	{
	    last = svars->I1->currentVectorGrp;
	    (*(last->FreeInstanceVars))(svars->I1);
	}
	else
	    last = NULL;

	svars->I1 = Streak_FindMultiGridContinuation_1(svars->vf1, p, last);
	if (! svars->I1)
	    return 0;
    }
    return 1;
}


static Error
Streak_Interpolate(StreakVars *svars, POINT_TYPE *p, double t, VECTOR_TYPE *v)
{
    double d;
    int i;
    VECTOR_TYPE v0[3], v1[3];
    
    if (! (*(svars->I0->currentVectorGrp->Interpolate))(svars->I0, p, v0))
	return ERROR;

    if (! (*(svars->I1->currentVectorGrp->Interpolate))(svars->I1, p, v1))
	return ERROR;
    
    d = (t - svars->t0) / (svars->t1 - svars->t0);

    for (i = 0; i < svars->nDim; i++)
	v[i] = v0[i] + d*(v1[i] - v0[i]);
    
    return OK;
}

static Error
Streak_StepTime(StreakVars *svars, double c, VECTOR_TYPE *v, double *t)
{
    double t0, t1;
    
    if (! (*(svars->I0->currentVectorGrp->StepTime))(svars->I0, c, v, &t0))
	return ERROR;

    if (! (*(svars->I1->currentVectorGrp->StepTime))(svars->I1, c, v, &t1))
	return ERROR;
    
    if (t0 < t1)
	*t = t0;
    else 
        *t = t1;

    return OK;
}

static int
Streak_InsideWeights(StreakVars *svars, POINT_TYPE *p)
{
    
    svars->inside0 = (*(svars->I0->currentVectorGrp->Weights))(svars->I0, p);
    svars->inside1 = (*(svars->I1->currentVectorGrp->Weights))(svars->I1, p);
    
    if (svars->inside0 && svars->inside1)
	return 1;
    else
	return 0;
}

#if 0
static int
Streak_Weights(StreakVars *svars, POINT_TYPE *p)
{
    
    (*(svars->I0->currentVectorGrp->Weights))(svars->I0, p);
    (*(svars->I1->currentVectorGrp->Weights))(svars->I1, p);
    
    return 1;
}
#endif

static int
Streak_FaceWeights(StreakVars *svars, POINT_TYPE *p)
{
    
    if (! svars->inside0)
	(*(svars->I0->currentVectorGrp->FaceWeights))(svars->I0, p);

    if (! svars->inside1)
	(*(svars->I0->currentVectorGrp->FaceWeights))(svars->I1, p);
    
    return 1;
}

static Error
Streak_FindBoundary(StreakVars *svars, POINT_TYPE *p, VECTOR_TYPE *v, double *t)
{
    double t0, t1;

    if (svars->inside0)
	t0 = DXD_MAX_FLOAT;
    else
	if (! (*(svars->I0->currentVectorGrp->FindBoundary))
					(svars->I0, p, v, &t0))
	    return ERROR;

    if (svars->inside1)
	t1 = DXD_MAX_FLOAT;
    else
	if (! (*(svars->I1->currentVectorGrp->FindBoundary))
					(svars->I1, p, v, &t1))
	    return ERROR;
	
    if (t1 < t0)
	*t = t1;
    else
	*t = t0;
    
    if (*t == DXD_MAX_FLOAT)
    {
	DXSetError(ERROR_INTERNAL, "error in Streak_FindBoundary");
	return ERROR;
    }

    return OK;
}

static int
Streak_Neighbor(StreakVars *svars, VECTOR_TYPE *v)
{
    int found;

    if (! svars->inside0)
    {
	found = (*(svars->I0->currentVectorGrp->Neighbor))(svars->I0, v);
	if (found != 1)
	    return found;
    }

    if (! svars->inside1)
    {
	found = (*(svars->I1->currentVectorGrp->Neighbor))(svars->I1, v);
	if (found != 1)
	    return found;
    }

    return 1;
}

static int
IsSeries(Object o)
{
    if (DXGetObjectClass(o) != CLASS_GROUP)
	return 0;
    if (DXGetGroupClass((Group)o) != CLASS_SERIES)
	return 0;
    return 1;
}

static Error
ResetVectorField(VectorField vf)
{
    int i;

    for (i = 0; i < vf->nmembers; i++)
    {
	VectorGrp vg = vf->members[i];

	if (vg->Reset)
	    if (! (*(vg->Reset))(vg))
		return ERROR;
    }

    return OK;
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
	    char *str;
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
	    break;
	}
	
	case CLASS_SERIES:
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
