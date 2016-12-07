/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include <math.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#define ZOFFSET         1.0
#define SENSITIVITY	4.0

typedef struct
{
    float x, y;
} Point2D;

typedef struct 
{
    int		depth;
    int		pickPath[10240];
    Array	picks;
    Array	paths;
    Array       weights;
} PickBuf;

typedef struct
{
    Point	xyz;
    int		index;
    int		pathlen;
    int		poke;
} Pick;

typedef struct 
{
    int 	segment;
    int		nweights;
} PickWts;

#define BOX_HIT    1
#define BOX_MISS   0
#define BOX_ERROR -1

static Error  GetObjectAndCameraFromCache(char *, Object *, Camera *);
static Field  GetPicksFromCache(char *, Object *);
static Error  PutPicksIntoCache(char *, Object *, Field);

static Error  PickObject(Object object, PickBuf *pick, int, Point2D *,
					Matrix *stack, int, float, float, 
					float, float, Camera);

static Error  PickField(Field f, PickBuf *p, int, Point2D,
					Matrix *s, int, float, float, 
					float, float);
static int    PickBox(Field f, Point2D, Matrix *s, int, float);
static Point *TransformArray(Array src, Matrix *m);
static int    TriangleHit(Point **, Point2D *, Point *, int, float *);

static Error  PickTriangles(Field, PickBuf *, int, Point2D,
					Matrix *, int, float, float,
					float, float);
static Error  PickQuads(Field, PickBuf *, int, Point2D,
					Matrix *, int, float, float,
					float, float);
static Error  Elt2D_Perspective(PickBuf *, int, Point2D, Point *,
					int *, int, int, int,
					float, int *, float,
					float, float);
static Error  Elt2D_NoPerspective(PickBuf *, int, Point2D, Point *,
					int *, int, int, int,
					float, int *, float,
					float, float);

static Error  PickLines(Field, PickBuf *, int, Point2D,
					Matrix *, int, float, float,
					float, float);
static Error  Elt1D_Perspective(PickBuf *, int, Point2D, Point *,
					int *, int, int, float, float,
					float, float);
static Error  Elt1D_NoPerspective(PickBuf *, int, Point2D, Point *,
					int *, int, int, float, float,
					float, float);

static Error  PickPoints(Field, PickBuf *, int, Point2D,
					Matrix *, int, float, float,
					float, float);
static Error  Elt0D_Perspective(PickBuf *, int, Point2D, Point *,
					int, float, float,
					float, float);
static Error  Elt0D_NoPerspective(PickBuf *, int, Point2D, Point *,
					int, float, float,
					float, float);

static Error  PickFLE(Field, PickBuf *, int, Point2D, 
					Matrix *, int, float, float,
					float, float);
static Error  FLE_NoPerspective(PickBuf *, int, Point2D, Point *, int,
					int *, int, int *, int, int *, int,
					float, float,
					float, float);
static Error  FLE_Perspective(PickBuf *, int, Point2D, Point *, int,
					int *, int, int *, int, int *, int,
					float, float,
					float, float);

static Error  PickPolylines(Field, PickBuf *, int, Point2D,
					Matrix *, int, float, float,
					float, float);

static Error  Polyline_Perspective(PickBuf *, int, Point2D, Point *,
					int *, int *, int, int, int, float,
					float, float, float);

static Error  Polyline_NoPerspective(PickBuf *, int, Point2D, Point *,
					int *, int *, int, int, int, float,
					float, float, float);

static Error  AddPick(PickBuf *, int, Point, int, int, float *);
static Field  PickBufToField(PickBuf *, Matrix);
static Field  GetFirstHits(Field);
static Error  PickInterpolate(Object, Field, int);

extern Error _dxfLoadStereoModes();
extern Error _dxfInitializeStereoCameraMode(void *, Object);

#define CACHE_PICKS	1
#define NO_CACHE_PICKS	0

#define FIRST_HIT_ONLY  1
#define ALL_HITS	0

#define PICK_TAG_ARG    0
#define IMAGE_TAG_ARG   1
#define LIST_ARG 	2
#define STEP_ARG 	3
#define FIRST_ARG 	4
#define PERSISTENCE_ARG	5
#define INTERPOLATE_ARG 6
#define OBJECT_ARG      7
#define CAMERA_ARG	8

#define NO_INTERP	0
#define CLOSEST_VERTEX  1
#define INTERPOLATE     2

/*
 * Used only by StereoPick
 */
#define STEREO_ARG      9
#define WHERE_ARG       10

static Array getXY(Array);

Error
m_Pick(Object *in, Object *out)
{
    Object   cacheObject = NULL, object = NULL;
    Array    list = NULL;
    Camera   camera = NULL;
    PickBuf  pick;
    int      nPicks, nDim, i;
    Matrix   initialMatrix, cameraMatrix, invCameraMatrix;
    Point2D  *xy, *xyPicks = NULL;
    int	     persp;
    float    nearPlane;
    float    width;
    float    xCenter, yCenter;
    int      xRes, yRes;
    Field    result = NULL, field = NULL;
    float    sensitivity;
    char     *ptag, *itag;
    int      persistence;
    int	     first;
    int      interpolate = NO_INTERP;

    pick.picks     = NULL;
    pick.paths     = NULL;
    pick.weights   = NULL;
    pick.depth     = 0;

    out[0] = NULL;

    if (in[CAMERA_ARG])
    {
	if (in[OBJECT_ARG] == NULL)
	{
	    DXSetError(ERROR_MISSING_DATA, "#10000", "object");
	    goto error;
	}
	object = in[OBJECT_ARG];

	if (in[CAMERA_ARG] == NULL)
	{
	    DXSetError(ERROR_MISSING_DATA, "#10000", "camera");
	    goto error;
	}
	camera = (Camera)in[CAMERA_ARG];

	if (in[FIRST_ARG])
	{
	    if (! DXExtractInteger(in[FIRST_ARG], &first))
	    {
		DXSetError(ERROR_BAD_PARAMETER, "first must be integer");
		goto error;
	    }

	    if (first != FIRST_HIT_ONLY && first != ALL_HITS)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "first must be 0 or 1");
		goto error;
	    }
	}
	else 
	    first = FIRST_HIT_ONLY;
	
	if (in[INTERPOLATE_ARG])
	{
	    if (! DXExtractInteger(in[INTERPOLATE_ARG], &interpolate))
	    {
		DXSetError(ERROR_BAD_PARAMETER, "interpolate must be integer");
		goto error;
	    }

	    if (interpolate != NO_INTERP &&
		interpolate != CLOSEST_VERTEX &&
		interpolate != INTERPOLATE)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "interpolate must be %d %d or %d",
			NO_INTERP, CLOSEST_VERTEX, INTERPOLATE);
		goto error;
	    }
	}

	list = (Array)in[LIST_ARG];
	if (list)
	{
	    list = getXY(list);
	    if (!list && DXGetError() != ERROR_NONE)
		goto error;
	}
	
	if (object && camera && list)
	{
	    DXGetArrayInfo(list, &nPicks, NULL, NULL, NULL, &nDim);
	    if (nDim != 2)
	    {
		DXSetError(ERROR_DATA_INVALID, "pick list must be 2-D");
		goto error;
	    }

	    xyPicks = (Point2D *)DXAllocate(nPicks*sizeof(Point2D));
	    if (! xyPicks)
		goto error;
	    
	    if (! DXExtractParameter((Object)list, TYPE_FLOAT,
					    2, nPicks, (Pointer)xyPicks))
		goto error;

	    cameraMatrix = DXGetCameraMatrix(camera);
	    cameraMatrix = DXConcatenate(cameraMatrix,
				DXTranslate(DXPt(0.0,0.0,ZOFFSET)));
	    invCameraMatrix = DXInvert(cameraMatrix);

	    DXGetCameraResolution(camera, &xRes, &yRes);
	
	    persp = 0;
	    if (! DXGetOrthographic(camera, &width, NULL))
	    {
		DXGetPerspective(camera, &width, NULL);
		persp = 1;
		nearPlane = -0.001;
	    }
	    else
	    {
		nearPlane = DXD_MAX_FLOAT;
	    }
	    
	    xCenter = xRes / 2.0;
	    yCenter = yRes / 2.0;

	    sensitivity = SENSITIVITY;

	    initialMatrix = cameraMatrix;
	

	    xy = xyPicks;
	    for (i = 0; i < nPicks; i++, xy ++)
	    {
		xy->x = xy->x - xCenter;
		xy->y = (yRes-xy->y) - yCenter;

		if (! PickObject(object, &pick, i, xy, &initialMatrix, persp,
			nearPlane, sensitivity, -DXD_MAX_FLOAT, DXD_MAX_FLOAT,
			camera))
		     goto error;
	    }

	    field = PickBufToField(&pick, invCameraMatrix);
	    if (! field)
		goto error;
	    

	    if (! DXSetAttribute((Object)field, "pick object", object))
		goto error;

	}

	if (field)
	{
	    object = DXGetAttribute((Object)field, "pick object");
	    if (! object)
	    {     
		DXSetError(ERROR_MISSING_DATA,
		     "unable to access pick object attribute");
		goto error;
	    }

	    if (first == FIRST_HIT_ONLY)
		result = GetFirstHits(field);
	    else
	    {
		result = (Field)DXCopy((Object)field, COPY_STRUCTURE);
		DXSetAttribute((Object)result, "pick object", NULL);
	    }

	    if (! result)
		goto error;

	    if (interpolate && !DXEmptyField(result))
	    {
		Array array;
		int   n, nn, i;

		if (! PickInterpolate(object, result, interpolate))
		    goto error;
		
		/*
		 * error check to verify that all picks got all data
		 */
	    
		array = (Array)DXGetComponentValue(result, "positions");
		DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);

		for (i = 0;
		     NULL != (array =
			    (Array)DXGetEnumeratedComponentValue(result, i, NULL));
		     i++)
		{
		    Object attr = DXGetAttribute((Object)array, "dep");
		    if (! attr)
			continue;
		    
		    if (! strcmp("positions", DXGetString((String)attr)))
		    {
			DXGetArrayInfo(array, &nn, NULL, NULL, NULL, NULL);
			if (nn != n)
			{
			    DXSetError(ERROR_DATA_INVALID,
			       "component mismatch in pick object");
			    goto error;
			}
		    }
		}
	    }

	    DXDeleteComponent(result, "pick weights");

	    DXDelete(DXReference((Object)field));
	}
	else
	{
	    result = DXNewField();
	    if (! result)
		goto error;
	}
    }
    else
    {
	if (in[PICK_TAG_ARG] == NULL)
	{
	    DXSetError(ERROR_MISSING_DATA, "#10000", "pickTag");
	    goto error;
	}

	if (! DXExtractString(in[PICK_TAG_ARG], &ptag))
	{
	    DXSetError(ERROR_DATA_INVALID, "pickTag must be STRING");
	    goto error;
	}

	if (in[IMAGE_TAG_ARG] == NULL)
	{
	    if (in[LIST_ARG] != NULL)
	    {
		DXSetError(ERROR_MISSING_DATA, "#10000", "imageTag");
		goto error;
	    }
	}
	else
	{
	    if (! DXExtractString(in[IMAGE_TAG_ARG], &itag))
	    {
		DXSetError(ERROR_DATA_INVALID, "imageTag must be STRING");
		goto error;
	    }

	    if (! GetObjectAndCameraFromCache(itag, &cacheObject, &camera))
		goto error;
	}

	if (in[PERSISTENCE_ARG])
	{
	    if (! DXExtractInteger(in[PERSISTENCE_ARG], &persistence))
	    {
		DXSetError(ERROR_BAD_PARAMETER, "persistence must be integer");
		goto error;
	    }

	    if (persistence != CACHE_PICKS && persistence != NO_CACHE_PICKS)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "persistence must be 0 or 1");
		goto error;
	    }
	}
	else 
	    persistence = CACHE_PICKS;

	if (in[FIRST_ARG])
	{
	    if (! DXExtractInteger(in[FIRST_ARG], &first))
	    {
		DXSetError(ERROR_BAD_PARAMETER, "first must be integer");
		goto error;
	    }

	    if (first != FIRST_HIT_ONLY && first != ALL_HITS)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "first must be 0 or 1");
		goto error;
	    }
	}
	else 
	    first = FIRST_HIT_ONLY;
	
	if (in[INTERPOLATE_ARG])
	{
	    if (! DXExtractInteger(in[INTERPOLATE_ARG], &interpolate))
	    {
		DXSetError(ERROR_BAD_PARAMETER, "interpolate must be integer");
		goto error;
	    }

	    if (interpolate != NO_INTERP &&
		interpolate != CLOSEST_VERTEX &&
		interpolate != INTERPOLATE)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "interpolate must be %d %d or %d",
			NO_INTERP, CLOSEST_VERTEX, INTERPOLATE);
		goto error;
	    }
	}

	object = in[OBJECT_ARG];
	if (! object)
	    object = cacheObject;

	list = (Array)in[LIST_ARG];
	
	if (object && camera && list)
	{
	    if (DXGetObjectClass((Object)list) != CLASS_ARRAY)
	    {
		DXSetError(ERROR_DATA_INVALID, "pick list must be class ARRAY");
		goto error;
	    }

	    DXGetArrayInfo(list, &nPicks, NULL, NULL, NULL, &nDim);
	    if (nDim != 2)
	    {
		DXSetError(ERROR_DATA_INVALID, "pick list must be 2-D");
		goto error;
	    }

	    xyPicks = (Point2D *)DXAllocate(nPicks*sizeof(Point2D));
	    if (! xyPicks)
		goto error;
	    
	    if (! DXExtractParameter((Object)list, TYPE_FLOAT,
					    2, nPicks, (Pointer)xyPicks))
		goto error;

	    cameraMatrix = DXGetCameraMatrix(camera);
	    cameraMatrix = DXConcatenate(cameraMatrix,
				DXTranslate(DXPt(0.0,0.0,ZOFFSET)));
	    invCameraMatrix = DXInvert(cameraMatrix);

	    DXGetCameraResolution(camera, &xRes, &yRes);
	
	    persp = 0;
	    if (! DXGetOrthographic(camera, &width, NULL))
	    {
		DXGetPerspective(camera, &width, NULL);
		persp = 1;
		nearPlane = -0.001;
	    }
	    else
	    {
		nearPlane = DXD_MAX_FLOAT;
	    }
	    
	    xCenter = xRes / 2.0;
	    yCenter = yRes / 2.0;

	    sensitivity = SENSITIVITY;

	    initialMatrix = cameraMatrix;

	    xy = xyPicks;
	    for (i = 0; i < nPicks; i++, xy ++)
	    {
		xy->x = xy->x - xCenter;
		xy->y = xy->y - yCenter;

		if (! PickObject(object, &pick, i, xy, &initialMatrix, persp,
			nearPlane, sensitivity, -DXD_MAX_FLOAT, DXD_MAX_FLOAT, 
			camera))
		     goto error;
	    }

	    field = PickBufToField(&pick, invCameraMatrix);
	    if (! field)
		goto error;
	    

	    if (! DXSetAttribute((Object)field, "pick object", object))
		goto error;

	    if (! PutPicksIntoCache(ptag, in, field))
		goto error;

	}
	else if (persistence == CACHE_PICKS)
	{
	    field = GetPicksFromCache(ptag, in);
	}

	if (field)
	{
	    object = DXGetAttribute((Object)field, "pick object");
	    if (! object)
	    {     
		DXSetError(ERROR_MISSING_DATA,
		     "unable to access pick object attribute");
		goto error;
	    }

	    if (first == FIRST_HIT_ONLY)
		result = GetFirstHits(field);
	    else
	    {
		result = (Field)DXCopy((Object)field, COPY_STRUCTURE);
		DXSetAttribute((Object)result, "pick object", NULL);
	    }

	    if (! result)
		goto error;

	    if (interpolate && !DXEmptyField(result))
	    {
		Array array;
		int   n, nn, i;

		if (! PickInterpolate(object, result, interpolate))
		    goto error;
		
		/*
		 * error check to verify that all picks got all data
		 */
	    
		array = (Array)DXGetComponentValue(result, "positions");
		DXGetArrayInfo(array, &n, NULL, NULL, NULL, NULL);

		for (i = 0;
		     NULL != (array =
			    (Array)DXGetEnumeratedComponentValue(result, i, NULL));
		     i++)
		{
		    Object attr = DXGetAttribute((Object)array, "dep");
		    if (! attr)
			continue;
		    
		    if (! strcmp("positions", DXGetString((String)attr)))
		    {
			DXGetArrayInfo(array, &nn, NULL, NULL, NULL, NULL);
			if (nn != n)
			{
			    DXSetError(ERROR_DATA_INVALID,
			       "component mismatch in pick object");
			    goto error;
			}
		    }
		}
	    }

	    DXDeleteComponent(result, "pick weights");

	    DXDelete(DXReference((Object)field));
	}
	else
	{
	    result = DXNewField();
	    if (! result)
		goto error;
	}

    }

    if (camera != (Camera)in[CAMERA_ARG])
        DXDelete((Object)camera);

    if (list && (Object)list != in[LIST_ARG])
	DXDelete((Object)list);

    if (! result)
	out[0] = (Object)DXNewField();
    else
	out[0] = (Object)result;

    DXDelete((Object)cacheObject);
    DXDelete((Object)pick.picks);
    DXDelete((Object)pick.paths);
    DXDelete((Object)pick.weights);

    DXFree((Pointer)xyPicks);

    return OK;

error:
    if (list && (Object)list != in[LIST_ARG])
	DXDelete((Object)list);

    if (camera != (Camera)in[CAMERA_ARG])
        DXDelete((Object)camera);

    DXDelete((Object)cacheObject);
    DXDelete((Object)camera);

    if (field)
	DXDelete(DXReference((Object)field));

    DXDelete((Object)result);
    DXDelete((Object)pick.picks);
    DXDelete((Object)pick.paths);
    DXDelete((Object)pick.weights);
    DXFree((Pointer)xyPicks);

    out[0] = NULL;

    return ERROR;
}

extern Error _dxfGetStereoCameras(void *, Camera, Camera *, Camera *);
extern void *_dxfGetStereoWindowInfo(char *, void *, void *);

Error m_StereoPick(Object *in, Object *out)
{
    Camera camera, lcamera, rcamera;
    char *where;
    void *globals;

    out[0] = NULL;

    if (! in[LIST_ARG])
        return OK;

    in[LIST_ARG] = (Object)getXY((Array)in[LIST_ARG]);
    if (! in[LIST_ARG])
        return OK;

    if (! in[STEREO_ARG])
    {
        DXSetError(ERROR_BAD_PARAMETER, "stereo arg must be given");
        return ERROR;
    }

    if (! in[WHERE_ARG] || ! DXExtractString(in[WHERE_ARG], &where))
    {
        DXSetError(ERROR_BAD_PARAMETER, "where must be given");
        return ERROR;
    }

    if (! _dxfLoadStereoModes())
    {
        DXSetError(ERROR_BAD_PARAMETER, "error loading stereo code");
        return ERROR;
    }

    globals = _dxfGetStereoWindowInfo(where, NULL, NULL);
    if (! globals)
    {
        DXSetError(ERROR_BAD_PARAMETER, "error accessing stereo window info");
        return ERROR;
    }

    if (! in[CAMERA_ARG])
    {
        DXSetError(ERROR_BAD_PARAMETER, "camera arg must be given");
        return ERROR;
    }

    camera = (Camera)in[CAMERA_ARG];

    _dxfLoadStereoModes();

    if (! _dxfInitializeStereoCameraMode(globals, in[STEREO_ARG]))
    {
        DXSetError(ERROR_BAD_PARAMETER, "error initting stereo mode");
        return ERROR;
    }

    if (! _dxfGetStereoCameras(globals, camera, &lcamera, &rcamera))
    {
        DXSetError(ERROR_BAD_PARAMETER, "error getting stereo cameras");
        return ERROR;
    }

    return m_Pick(in, out);
}

static Field
GetPicksFromCache(char *tag, Object *in)
{
    Object obj;
    char *buf = (char *)DXAllocate(strlen(tag) + strlen(".picks") + 1);
    if (! buf)
	return ERROR;
    
    sprintf(buf, "%s.picks", tag);

    obj = DXGetCacheEntry(buf, 0, 1, in[OBJECT_ARG]);

    DXFree((Pointer)buf);
    return (Field)obj;
}

static Error
PutPicksIntoCache(char *tag, Object *in, Field f)
{
    Error e;
    char *buf = (char *)DXAllocate(strlen(tag) + strlen(".picks") + 1);
    if (! buf)
	return ERROR;
    
    sprintf(buf, "%s.picks", tag);

    e = DXSetCacheEntry((Object)f, CACHE_PERMANENT, buf, 0, 1,
						    in[OBJECT_ARG]);

    DXFree((Pointer)buf);
    return e;
}

static Error
AddPick(PickBuf *p, int poke, Point xyz, int nwts, int seg, float *wts)
{
    int nPicks, nPathElts, depth = p->depth + 2, wtLnth;
    Pick pick;
    PickWts pickWts;

    if (p->picks == NULL)
    {
	p->picks   = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, sizeof(Pick));
	p->paths   = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
	p->weights = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
	if (! p->picks || ! p->paths || ! p->weights)
	    return ERROR;
	
	DXReference((Object)p->picks);
	DXReference((Object)p->paths);
	DXReference((Object)p->weights);
    }

    DXGetArrayInfo(p->picks, &nPicks, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(p->paths, &nPathElts, NULL, NULL, NULL, NULL);

    pick.xyz     = xyz;
    pick.index   = nPathElts;
    pick.poke    = poke;
    pick.pathlen = depth;

    if (! DXAddArrayData(p->picks, nPicks, 1, (Pointer)&pick))
	return ERROR;
    
    if (! DXAddArrayData(p->paths, nPathElts, depth, (Pointer)p->pickPath))
	return ERROR;
    
    pickWts.nweights = nwts;
    pickWts.segment  = seg;

    DXGetArrayInfo(p->weights, &wtLnth, NULL, NULL, NULL, NULL);
    if (! DXAddArrayData(p->weights, wtLnth, sizeof(pickWts), (Pointer)&pickWts))
	return ERROR;
    
    if (nwts)
    {
	wtLnth += sizeof(pickWts);
	if (! DXAddArrayData(p->weights, wtLnth, nwts*sizeof(float), (Pointer)wts))
	    return ERROR;
    }

    return OK;
}

#define TYPE      Pick
#define LT(a,b)   ((a)->poke < (b)->poke || \
			(((a)->poke == (b)->poke) && (a)->xyz.z > (b)->xyz.z))
#define GT(a,b)   ((a)->poke > (b)->poke || \
			(((a)->poke == (b)->poke) && (a)->xyz.z < (b)->xyz.z))
#define QUICKSORT picksort

#include "../libdx/qsort.c"

static Field
PickBufToField(PickBuf *p, Matrix m)
{
    Array positions = NULL;
    Array pokes     = NULL;
    Array indices   = NULL;
    Array paths     = NULL;
    Field field     = NULL;
    int   nPicks, nPokes, nPathElts;
    Pick  *picks;
    Point *posPtr;
    int   *pokPtr;
    int   *idxPtr;
    int   *pathSrc, *pathDst;
    int   i, j, lastPoke;

    if (p->picks == NULL)
        return DXNewField();
    
    DXGetArrayInfo(p->picks, &nPicks,    NULL, NULL, NULL, NULL);
    DXGetArrayInfo(p->paths, &nPathElts, NULL, NULL, NULL, NULL);

    picks = (Pick *)DXGetArrayData(p->picks);

    picksort(picks, nPicks);

    /*
     * Count the pokes
     */
    nPokes = 1;
    i = picks[0].poke;
    for (j = 1; j < nPicks; j++)
	if (i != picks[j].poke)
	{
	    nPokes ++;
	    i = picks[j].poke;
	}

    positions = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    pokes     = DXNewArray(TYPE_INT,   CATEGORY_REAL, 0);
    indices   = DXNewArray(TYPE_INT,   CATEGORY_REAL, 0);
    paths     = DXNewArray(TYPE_INT,   CATEGORY_REAL, 0);

    if (! positions || ! pokes || ! indices || ! paths)
	return NULL;
    
    if (! DXAddArrayData(positions, 0, nPicks,    NULL) ||
	! DXAddArrayData(pokes,     0, nPokes,    NULL) ||
	! DXAddArrayData(indices,   0, nPicks,    NULL) ||
	! DXAddArrayData(paths,     0, nPathElts, NULL))
	return NULL;
    
    posPtr = (Point *)DXGetArrayData(positions);
    pokPtr = (int   *)DXGetArrayData(pokes);
    idxPtr = (int   *)DXGetArrayData(indices);

    pathSrc = (int   *)DXGetArrayData(p->paths);
    pathDst = (int   *)DXGetArrayData(paths);

    lastPoke = -1;
    for (i = j = 0; i < nPicks; i++, picks ++)
    {
	*posPtr++ = DXApply(picks->xyz, m);
	*idxPtr++ = j;

	memcpy(pathDst, pathSrc+picks->index, picks->pathlen*sizeof(int));
	pathDst += picks->pathlen;
	j += picks->pathlen;

	if (picks->poke != lastPoke)
	{
	    *pokPtr++ = i;
	    lastPoke = picks->poke;
	}
    }

    field = DXNewField();
    if (! field)
	goto error;
    
    if ((! DXSetStringAttribute((Object)positions, "dep", "positions"))   ||
        (! DXSetStringAttribute((Object)pokes,     "ref", "picks"))       ||
        (! DXSetStringAttribute((Object)indices,   "dep", "positions"))   ||
        (! DXSetStringAttribute((Object)indices,   "ref", "pick paths")))
	goto error;
    
    if (! DXSetComponentValue(field, "positions", (Object)positions))
	goto error;
    positions = NULL;

    if (! DXSetComponentValue(field, "pokes", (Object)pokes))
	goto error;
    pokes = NULL;

    if (! DXSetComponentValue(field, "picks", (Object)indices))
	goto error;
    indices = NULL;

    if (! DXSetComponentValue(field, "pick paths", (Object)paths))
	goto error;
    paths = NULL;

    if (! DXSetComponentValue(field, "pick weights", (Object)p->weights))
	goto error;

    return field;

error:
    DXDelete((Object)positions);
    DXDelete((Object)pokes);
    DXDelete((Object)indices);
    DXDelete((Object)paths);
    DXDelete((Object)field);
    return NULL;
}

static Error
GetObjectAndCameraFromCache(char *tag, Object *object, Camera *camera)
{
    char *buf;

    if (object)
    {
	buf = (char *)DXAllocate(strlen(tag) + strlen(".object") + 1);
	if (! buf)
	    goto error;
    
	sprintf(buf, "%s.object", tag);

	*object = DXGetCacheEntry(buf, 0, 0);
	DXFree((Pointer)buf);
    }

    if (camera)
    {
	buf = (char *)DXAllocate(strlen(tag) + strlen(".camera") + 1);
	if (! buf)
	    goto error;
    
	sprintf(buf, "%s.camera", tag);

	*camera = (Camera)DXGetCacheEntry(buf, 0, 0);
	DXFree((Pointer)buf);
    }

    return OK;

error:
    return ERROR;
}

static Error
PickObject(Object object, PickBuf *pick, int poke, Point2D *xy,
	Matrix *stack, int persp, float nearPlane, float sens,
	float min, float max, Camera camera)
{
    Object attr = DXGetAttribute(object, "pickable");
    int    pickable;

    if (attr)
    {
	if (! DXExtractInteger(attr, &pickable))
	{
	    DXSetError(ERROR_DATA_INVALID,
		"pickable attribute must be an integer");
	    return ERROR;
	}

	if (! pickable)
	    return OK;
    }

    switch(DXGetObjectClass(object))
    {
	case CLASS_XFORM:
	{
	    Object child;
	    Matrix matrix, product;

	    if (! DXGetXformInfo((Xform)object, &child, &matrix))
		goto error;
	    
	    product = DXConcatenate(matrix, *stack);

	    pick->pickPath[pick->depth++] = 0;

	    if (! PickObject(child, pick, poke, xy,
			&product, persp, nearPlane,
			sens, min, max, camera))
		return ERROR;

	    pick->depth --;
	    return OK;
	}

	case CLASS_CLIPPED:
	{
	    Object child;
	    Object clipper;
	    PickBuf clipPick;

	    clipPick.picks     = NULL;
	    clipPick.paths     = NULL;
	    clipPick.depth     = pick->depth;

	    if (! DXGetClippedInfo((Clipped)object, &child, &clipper))
		goto error;

	    pick->pickPath[pick->depth++] = 0;

	    if (! PickObject(clipper, &clipPick, poke, xy, stack, 
				persp, nearPlane, sens, min, max, camera))
		return ERROR;
	    
	    if (clipPick.picks)
	    {
		int nPicks;
		Pick *picks = (Pick *)DXGetArrayData(clipPick.picks);

		DXGetArrayInfo(clipPick.picks, &nPicks, NULL, NULL, NULL, NULL);

		if (nPicks == 1)
		{
		    max = picks->xyz.z;
		}
		else
		{
		    picksort(picks, nPicks);
		    max = picks[0].xyz.z;
		    min = picks[nPicks-1].xyz.z;
		}

		DXDelete((Object)clipPick.picks);
		DXDelete((Object)clipPick.paths);
	    }
	    
	    if (! PickObject(child, pick, poke, xy, stack, 
				persp, nearPlane, sens, min, max, camera))
		return ERROR;
	    
	    pick->depth --;

	    return OK;
	}

	case CLASS_GROUP:
	{
	    Object child;
	    int i;

	    pick->depth ++;

	    for (i = 0;;i++)
	    {
		child = DXGetEnumeratedMember((Group)object, i, NULL);
		if (! child)
		    break;
		
		pick->pickPath[pick->depth-1] = i;

		if (! PickObject(child, pick, poke, xy, 
			stack, persp, nearPlane, sens, min, max, camera))
		    return ERROR;
	    }

	    pick->depth --;

	    return OK;
	}

	case CLASS_FIELD:
	{
	    return PickField((Field)object, pick, poke, *xy,
		    stack, persp, nearPlane, sens, min, max);
	}

	case CLASS_SCREEN:
	{
	    Object child;
	    int fixed, z, width, height;
	    Matrix new = *stack;

	    if (! DXGetScreenInfo((Screen)object, &child, &fixed, &z))
		goto error;

	    DXGetCameraResolution(camera, &width, &height);
	    
	    if (fixed == SCREEN_VIEWPORT)
	    {
		new.b[0] = new.b[0]*width - width/2;
		new.b[1] = new.b[1]*height - height/2;
	    }
	    else if (fixed == SCREEN_PIXEL)
	    {
		new.b[0] -= width/2;
		new.b[1] -= height/2;
	    }
	    else if (fixed == SCREEN_WORLD)
	    {
		Matrix cm;

		cm = DXGetCameraMatrix(camera);
		new = DXConcatenate(*stack, cm);

		if (DXGetPerspective(camera, NULL, NULL))
		{
		    new.b[0] = - new.b[0] / new.b[2];
		    new.b[1] = - new.b[1] / new.b[2];
		}
	    }
	    else if (fixed == SCREEN_STATIONARY)
		new.b[0] = new.b[1] = new.b[2] = 0.0;
	       
	    new.A[0][0] = 1.0; new.A[0][1] = 0.0; new.A[0][2] = 0.0;
	    new.A[1][0] = 0.0; new.A[1][1] = 1.0; new.A[1][2] = 0.0;
	    new.A[2][0] = 0.0; new.A[2][1] = 0.0; new.A[2][2] = 1.0;

	    pick->pickPath[pick->depth++] = 0;

	    if (! PickObject(child, pick, poke, xy,
			&new, persp, nearPlane, sens, min, max, camera))
		return ERROR;

	    pick->depth --;
	    return OK;
	}

	default:
	{
	    return OK;
	}
    }

error:
    return ERROR;
}

static Error
PickField(Field f, PickBuf *p, int poke, Point2D xy, Matrix *s,
		    int persp, float nearPlane, float sens, float min, float max)
{
    Object c;
    char *str;
    int  pb;

    if (DXEmptyField(f))
	return OK;

    pb = PickBox(f, xy, s, persp, nearPlane);

    if (pb == BOX_ERROR)
	return ERROR;
    else if (pb == BOX_HIT)
    {
    
	if (NULL != (c = DXGetComponentValue(f, "connections")))
	{
	    if (! DXGetStringAttribute(c, "element type", &str))
	    {
		DXSetError(ERROR_MISSING_DATA, "element type attribute");
		return ERROR;
	    }

	    if (! strcmp(str, "triangles"))
		return PickTriangles(f, p, poke, xy, s,
				persp, nearPlane, sens, min, max);
	    else if (! strcmp(str, "quads"))
		return PickQuads(f, p, poke, xy, s,
				persp, nearPlane, sens, min, max);
	    else if (! strcmp(str, "lines"))
		return PickLines(f, p, poke, xy, s,
				persp, nearPlane, sens, min, max);
	    else
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"unable to pick when element type is %s", str);
		return ERROR;
	    }
	}
	else if (DXGetComponentValue(f, "faces"))
	{
	    return PickFLE(f, p, poke, xy, s, persp, nearPlane, sens, min, max);
	}
	else if (DXGetComponentValue(f, "polylines"))
	{
	    return PickPolylines(f, p, poke, xy, s,
				persp, nearPlane, sens, min, max);
	}
	else
	    return PickPoints(f, p, poke, xy, s, persp, nearPlane, sens, min, max);
    }
    
    return OK;
}

static Point *
TransformArray(Array src, Matrix *m)
{
    int		 i;
    Point        *dst = NULL;
    ArrayHandle  handle = NULL;
    int          nPts, nDim;
    Pointer      buf = NULL;

    DXGetArrayInfo(src, &nPts, NULL, NULL, NULL, &nDim);

    dst = DXAllocate(nPts*sizeof(Point));
    if (! dst)
	goto error;
    
    handle = DXCreateArrayHandle(src);
    if (! handle)
	goto error;

    buf = DXAllocate(DXGetItemSize(src));
    if (! buf)
	goto error;
    
    for (i = 0; i < nPts; i++)
    {
	Point d;
	float *s;

	s = (float *)DXGetArrayEntry(handle, i, buf);

	if (nDim == 1)
	    d.x = s[0], d.y = d.z = 0.0;
	if (nDim == 2)
	    d.x = s[0], d.y = s[1], d.z = 0.0;
	else
	    d.x = s[0], d.y = s[1], d.z = s[2];
	
	dst[i] = DXApply(d, *m);
    }

    DXFreeArrayHandle(handle);
    DXFree(buf);

    return dst;

error:
    DXFreeArrayHandle(handle);
    DXFree(buf);
    DXFree((Pointer)dst);

    return NULL;
}


static int box_triangles[] =
{
    0, 1, 3,
    0, 3, 2,
    1, 5, 7,
    1, 7, 3,
    5, 4, 6,
    5, 6, 7,
    4, 0, 2,
    4, 2, 6,
    3, 7, 6,
    3, 6, 2,
    0, 1, 5,
    0, 5, 4
};

static int
PickBox(Field field, Point2D xy, Matrix *s, int persp, float nearPlane)
{
    Point points[8];
    int   i, f;

    if (! DXBoundingBox((Object)field, points))
	return ERROR;
    
    if (s)
	for (i = 0; i < 8; i++)
	    points[i] = DXApply(points[i], *s);
	
    if (persp)
    {
	if (!Elt2D_Perspective(NULL, 0, xy, points, box_triangles, 
		8, 12, 3, nearPlane, &f, 0.0, -DXD_MAX_FLOAT, DXD_MAX_FLOAT))
	    goto error;
    }
    else
    {
	if (!Elt2D_NoPerspective(NULL, 0, xy, points, box_triangles, 
		8, 12, 3, nearPlane, &f, 0.0, -DXD_MAX_FLOAT, DXD_MAX_FLOAT))
	    goto error;
    }

    return f;

error:
    return -1;
}

static int
TriangleHit(Point **tri, Point2D *xy, Point *xyz, int persp, float *wts)
{
    int i, side = 0;
    float a[3];

    for (i = 0; i < 3; i++)
    {
	int j = (i == 2) ? 0 : i+1;
	float x0 = tri[j]->x - tri[i]->x;
	float y0 = tri[j]->y - tri[i]->y;
	float x1 = xy->x - tri[i]->x;
	float y1 = xy->y - tri[i]->y;
	float cross = x0*y1 - x1*y0;

	if ((cross < 0.0 && side == 1) || (cross > 0 && side == -1))
	    return 0;
	
	if (side == 0) {
	    if (cross < 0)
		side = -1;
	    else if (cross > 0)
		side =  1;
	}

	a[i] = cross;
    }

    if (side == 0)
	return 0;

    if (xyz || wts)
    {
	float A = 1.0 / (a[0] + a[1] + a[2]);
	float a0 = A*a[0];
	float a1 = A*a[1];
	float a2 = A*a[2];

	if (xyz)
	{
	    xyz->x = xy->x;
	    xyz->y = xy->y;
	    xyz->z = a0*tri[2]->z + a1*tri[0]->z + a2*tri[1]->z;

	    if (persp)
	    {
		float z = xyz->z = (ZOFFSET * xyz->z) / (1.0 + xyz->z);
		float depth = ZOFFSET - z;
		xyz->x *= depth;
		xyz->y *= depth;
	    }
	}

	if (wts)
	{
	    wts[0] = a1;
	    wts[1] = a2;
	    wts[2] = a0;
	}
    }

    return 1;
}

static Error
PickTriangles(Field f, PickBuf *p, int poke, Point2D xy,
    Matrix *m, int perspective, float nearPlane, float sens, float min, float max)
{
    Array   pA;
    Array   tA;
    Point   *xPoints = NULL;
    int     nPts, nTris;
    int	    *tris;

    pA = (Array)DXGetComponentValue(f, "positions");
    tA = (Array)DXGetComponentValue(f, "connections");
    if (! tA || ! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "positions or connections");
	goto error;
    }

    DXGetArrayInfo(pA, &nPts,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo(tA, &nTris, NULL, NULL, NULL, NULL);

    /*
     * Transform points
     */
    xPoints = TransformArray(pA, m);
    if (! xPoints)
	return ERROR;

    tris = (int *)DXGetArrayData(tA);
    
    if (perspective)
    {
	if (! Elt2D_Perspective(p, poke, xy, xPoints, tris,
			    nPts, nTris, 3, nearPlane, NULL, sens, min, max))
	    goto error;
    }
    else
    {
	if (! Elt2D_NoPerspective(p, poke, xy, xPoints, tris,
			    nPts, nTris, 3, nearPlane, NULL, sens, min, max))
	    goto error;
    }

    DXFree((Pointer)xPoints);
    return OK;

error:
    DXFree((Pointer)xPoints);
    return ERROR;
}

static Error
PickQuads(Field f, PickBuf *p, int poke, Point2D xy,
    Matrix *m, int perspective, float nearPlane, float sens, float min, float max)
{
    Array   pA;
    Array   qA;
    Point   *xPoints = NULL;
    int     nPts, nQuads;
    int	    *quads;

    pA = (Array)DXGetComponentValue(f, "positions");
    qA = (Array)DXGetComponentValue(f, "connections");
    if (! qA || ! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "positions or connections");
	goto error;
    }

    DXGetArrayInfo(pA, &nPts,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo(qA, &nQuads, NULL, NULL, NULL, NULL);

    /*
     * Transform points
     */
    xPoints = TransformArray(pA, m);
    if (! xPoints)
	return ERROR;

    quads = (int *)DXGetArrayData(qA);
    
    if (perspective)
    {
	if (! Elt2D_Perspective(p, poke, xy, xPoints, quads, 
			    nPts, nQuads, 4, nearPlane, NULL, sens, min, max))
	    goto error;
    }
    else
    {
	if (! Elt2D_NoPerspective(p, poke, xy, xPoints, quads, 
			    nPts, nQuads, 4, nearPlane, NULL, sens, min, max))
	    goto error;
    }

    DXFree((Pointer)xPoints);
    return OK;

error:
    DXFree((Pointer)xPoints);
    return ERROR;
}

static int triTab[]  = {0, 1, 2};
static int quadTab[] = {0, 1, 3, 2};

static Error
Elt2D_Perspective(PickBuf *p, int poke, Point2D xy, Point *xPoints, int *elts,
	    int nPts, int nElts, int vPerE, float nearPlane, int *found, float sens, 
	    float min, float max)
{
    int i, j;
    float dNear;
    Point xyz;
    int *table = (vPerE == 3) ? triTab : (vPerE == 4) ? quadTab : NULL;
    Point *sPoints = (Point *)DXAllocate(nPts*sizeof(Point));
    float *weights = (float *)DXAllocate(vPerE*sizeof(float));
    if (! sPoints || ! weights)
	goto error;
    
    nearPlane += ZOFFSET;
    dNear = -1.0 / nearPlane;
    
    /*
     * perform perspective on all unclipped transformed points
     */
    for (i = 0; i < nPts; i++)
    {
	float z = xPoints[i].z;
	if (z < nearPlane)
	{
	    float invDepth = 1.0 / (-z + ZOFFSET);
	    sPoints[i].x = xPoints[i].x * invDepth;
	    sPoints[i].y = xPoints[i].y * invDepth;
	    sPoints[i].z = xPoints[i].z * invDepth;
	}
    }

    if (found)
	*found = 0;

    for (i = 0; i < nElts; i++, elts += vPerE)
    {
	int     last, next, ngood, nclip;
	Point   *sclipped[12];
	Point   sclippt[12];
	int     orig[12];
	int     lastClipped, nextClipped;

	if (table)
	    last = elts[table[vPerE-1]];
	else
	    last = elts[vPerE-1];

	lastClipped = xPoints[last].z >= nearPlane;

	ngood = 0;
	nclip = 0;
	for (j = 0; j < vPerE; j++)
	{
	    if (table)
		next = elts[table[j]];
	    else
		next = elts[j];

	    nextClipped = xPoints[next].z >= nearPlane;

	    if (! lastClipped)
	    {
		sclipped[ngood] = sPoints + last;
		orig[ngood] = last;
		ngood ++;
	    }

	    if (lastClipped != nextClipped)
	    {
		Point   *lxp = xPoints + last;
		Point   *nxp = xPoints + next;
		float   x, y, d = (nearPlane - lxp->z) / (nxp->z - lxp->z);

		x = lxp->x + d*(nxp->x - lxp->x);
		y = lxp->y + d*(nxp->y - lxp->y);
		/* z = nearPlane; */
		
		sclippt[nclip].x = dNear * x;
		sclippt[nclip].y = dNear * y;
		sclippt[nclip].y = dNear * y;

		sclipped[ngood] = sclippt + nclip;
		orig[ngood] = -1;

		nclip++;
		ngood++;
	    }

	    last = next;
	    lastClipped = nextClipped;
	}

	/*
	 * If a fragment survived clipping...
	 */
	
	for (j = 2; j < ngood; j++)
	{
	    Point   *sc[3];
	    int     si[3];

	    sc[0] = sclipped[si[0] =   0];
	    sc[1] = sclipped[si[1] = j-1];
	    sc[2] = sclipped[si[2] =   j];

	    if (TriangleHit(sc, &xy, &xyz, 1, NULL))
	    {
		int ni=0, k;
		float nearPlane = DXD_MAX_FLOAT;

		if (found)
		    *found = 1;

		if (! p)
		    goto done;

		if (xyz.z > min && xyz.z < max)
		{
		    Point  *tpts[3];

		    for (k = 0; k < 3; k++)
			if (orig[si[k]] != -1)
			{
			    float dx = sPoints[orig[si[k]]].x - xy.x;
			    float dy = sPoints[orig[si[k]]].y - xy.y;
			    float n = sqrt(dx*dx + dy*dy);
			    if (n < nearPlane)
			    {
				ni = orig[si[k]];
				nearPlane = n;
			    }
			}

		    /* 
		     * find an *original* triangle containing the pick
		     * point.
		     */

		    if (table)
		    {
			tpts[0] = sPoints + elts[table[0]];
			tpts[2] = sPoints + elts[table[1]];
		    }
		    else
		    {
			tpts[0] = sPoints + elts[0];
			tpts[2] = sPoints + elts[1];
		    }
		       
		    for (k = 2; k < vPerE; k++)
		    {
			float wts[3];

			tpts[1] = tpts[2];
			if (table)
			    tpts[2] = sPoints + elts[table[k]];
			else
			    tpts[2] = sPoints + elts[k];
			
			if (TriangleHit(tpts, &xy, NULL, 1, wts))
			{
			    int l;

			    for (l = 0; l < vPerE; l++)
				weights[l] = 0;
			    
			    if (table)
			    {
				weights[table[0]]   = wts[0];
				weights[table[k-1]] = wts[1];
				weights[table[k]]   = wts[2];
			    }
			    else
			    {
				weights[0]   = wts[0];
				weights[k-1] = wts[1];
				weights[k]   = wts[2];
			    }

			    break;
			}
		    }

		    if (k == vPerE)
		    {
			DXSetError(ERROR_INTERNAL,
				"found in clipped but not in original");
			goto error;
		    }

		    p->pickPath[p->depth+0] = i;
		    p->pickPath[p->depth+1] = ni;
		    if (! AddPick(p, poke, xyz, vPerE, -1, weights))
			goto error;
		}
		
		break;
	    }
	}
    }

done:
    DXFree((Pointer)sPoints);
    DXFree((Pointer)weights);
    return OK;

error:
    DXFree((Pointer)sPoints);
    DXFree((Pointer)weights);
    return ERROR;
}

static Error
Elt2D_NoPerspective(PickBuf *p, int poke, Point2D xy, Point *xPoints, int *elts,
	    int nPts, int nElts, int vPerE, float nearPlane, int *found, float sens,
	    float min, float max)
{
    Point xyz;
    int i, j, k, ni=0;
    int *table = (vPerE == 3) ? triTab : (vPerE == 4) ? quadTab : NULL;
    float nearest;
    int indices[3];
    Point *pts[3];
    float wts[3], *weights = (float *)DXAllocate(vPerE*sizeof(float));

    if (! weights) goto error;
    
    if (found)
	*found = 0;

    for (i = 0; i < nElts; i++, elts += vPerE)
    {
	if (table)
	{
	    indices[0] = elts[table[0]];
	    indices[2] = elts[table[1]];
	}
	else
	{
	    indices[0] = elts[0];
	    indices[2] = elts[1];
	}

	pts[0] = xPoints + indices[0];
	pts[2] = xPoints + indices[2];

	for (j = 2; j < vPerE; j++)
	{
	    indices[1] = indices[2];
	    pts[1]     = pts[2];

	    if (table)
		indices[2] = elts[table[j]];
	    else
		indices[2] = elts[j];
	    
	    pts[2] = xPoints + indices[2];
	    
	    if (TriangleHit(pts, &xy, &xyz, 0, wts))
	    {
		if (!p)
		{
		    if (found)
			*found = 1;
		    goto done;
		}
		
		if (xyz.z >= nearPlane)
		    continue;

		if (found)
		    *found = 1;

		for (k = 0; k < vPerE; k++)
		    weights[k] = 0;
			
		if (table)
		{
		    weights[table[0]]   = wts[0];
		    weights[table[j-1]] = wts[1];
		    weights[table[j]]   = wts[2];
		}
		else
		{
		    weights[0]   = wts[0];
		    weights[j-1] = wts[1];
		    weights[j]   = wts[2];
		}

		for (k = 0, nearest = DXD_MAX_FLOAT; k < 3; k++)
		{
		    if (pts[k]->z < nearPlane)
		    {
			float dx = pts[k]->x - xy.x;
			float dy = pts[k]->y - xy.y;
			float n = sqrt(dx*dx + dy*dy);

			if (n < nearest)
			{
			    ni = indices[k];
			    nearest = n;
			}
		    }
		}

		p->pickPath[p->depth+0] =  i;
		p->pickPath[p->depth+1] = ni;
		if (! AddPick(p, poke, xyz, vPerE, -1, weights))
		    goto error;
		
		break;
	    }
	}
    }

done:
    DXFree((Pointer)weights);
    return OK;

error:
    DXFree((Pointer)weights);
    return ERROR;
}

static Error
PickLines(Field f, PickBuf *p, int poke, Point2D xy, Matrix *m,
	int perspective, float nearPlane, float sens, float min, float max)
{
    Array   pA;
    Array   lA;
    Point   *xPoints = NULL;
    int     nPts, nLines;
    int	    *lines;

    pA = (Array)DXGetComponentValue(f, "positions");
    lA = (Array)DXGetComponentValue(f, "connections");
    if (! lA || ! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "positions or connections");
	goto error;
    }

    DXGetArrayInfo(pA, &nPts,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo(lA, &nLines, NULL, NULL, NULL, NULL);

    /*
     * Transform points
     */
    xPoints = TransformArray(pA, m);
    if (! xPoints)
	return ERROR;

    lines = (int *)DXGetArrayData(lA);
    
    if (perspective)
    {
	if (! Elt1D_Perspective(p, poke, xy, xPoints, lines, nPts,
			    nLines, nearPlane, sens, min, max))
	    goto error;
    }
    else
    {
	if (! Elt1D_NoPerspective(p, poke, xy, xPoints, lines, nPts,
			    nLines, nearPlane, sens, min, max))
	    goto error;
    }

    DXFree((Pointer)xPoints);
    return OK;

error:
    DXFree((Pointer)xPoints);
    return ERROR;
}


static Error
Elt1D_Perspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints,
    int *elts, int nPts, int nElts, float nearPlane, float sens,
    float min, float max)
{
    int i;
    Point *sPoints;
    
    sPoints = (Point *)DXAllocate(nPts*sizeof(Point));
    if (! sPoints)
	goto error;

    nearPlane += ZOFFSET;
    
    /*
     * perform perspective on all unclipped transformed points
     */
    for (i = 0; i < nPts; i++)
    {
	float z = xPoints[i].z;
	if (z < nearPlane)
	{
	    float invDepth = 1.0 / (-z + ZOFFSET);
	    sPoints[i].x = xPoints[i].x * invDepth;
	    sPoints[i].y = xPoints[i].y * invDepth;
	    sPoints[i].z = xPoints[i].z * invDepth;
	}
    }

    for (i = 0; i < nElts; i++, elts += 2)
    {
	int     p, q;
	Point   *px, *qx;
	Point   *ps, *qs, *sclipped[2];
	Point   sclippt;
	float   x, y, z, a, b, c, D, dx, dy, depth;
	Point   xyz;
	int	orig[2];

	p = elts[0];
	q = elts[1];

	px = xPoints + p;
	qx = xPoints + q;

	ps = sPoints + p;
	qs = sPoints + q;

	if (px->z > nearPlane && qx->z > nearPlane)
	    continue;
	
	if (px->z <= nearPlane && qx->z <= nearPlane)
	{
	    /*xclipped[0] = px;*/
	    /*xclipped[1] = qx;*/

	    sclipped[0] = ps;
	    sclipped[1] = qs;

	    orig[0] = p;
	    orig[1] = q;
	}
	else if (px->z > nearPlane && qx->z <= nearPlane)
	{
	    float d = (nearPlane - qx->z) / (px->z - qx->z);

	    /*xclippt.x = qx->x + d*(px->x - qx->x);*/
	    /*xclippt.y = qx->y + d*(px->y - qx->y);*/
	    /*xclippt.z = nearPlane;*/
	    /*xclipped[0] = &xclippt;*/
	    /*xclipped[1] = qx;*/

	    sclippt.x = qs->x + d*(ps->x - qs->x);
	    sclippt.y = qs->y + d*(ps->y - qs->y);
	    sclippt.z = nearPlane;
	    sclipped[0] = &sclippt;
	    sclipped[1] = qs;

	    orig[0] = -1;
	    orig[1] = q;
	}
	else
	{
	    float d = (nearPlane - qx->z) / (px->z - qx->z);

	    /*xclippt.x = qx->x + d*(px->x - qx->x);*/
	    /*xclippt.y = qx->y + d*(px->y - qx->y);*/
	    /*xclippt.z = nearPlane;*/
	    /*xclipped[0] = &xclippt;*/
	    /*xclipped[1] = px;*/

	    sclippt.x = qs->x + d*(ps->x - qs->x);
	    sclippt.y = qs->y + d*(ps->y - qs->y);
	    sclippt.z = nearPlane;
	    sclipped[0] = &sclippt;
	    sclipped[1] = ps;

	    orig[0] = -1;
	    orig[1] = p;
	}

	dx = sclipped[1]->x - sclipped[0]->x;
	dy = sclipped[1]->y - sclipped[0]->y;

	if (dx == 0 && dy == 0)
	    continue;
	
	D = 1.0 / sqrt(dx*dx + dy*dy);

	a = dy * D;
	b = -dx * D;
	c = -(a*sclipped[0]->x + b*sclipped[0]->y);

	D = a*xy.x + b*xy.y + c;

	if (-sens < D && D < sens)
	{
	    /*
	     * step along line normal by -D to get closest point on line
	     */
	    x = xy.x - D*a;
	    y = xy.y - D*b;

	    if ((((sclipped[0]->x-sens) <= x && x <= (sclipped[1]->x+sens)) ||
		 ((sclipped[0]->x+sens) >= x && x >= (sclipped[1]->x-sens))) &&
		(((sclipped[0]->y-sens) <= y && y <= (sclipped[1]->y+sens)) ||
		 ((sclipped[0]->y+sens) >= y && y >= (sclipped[1]->y-sens))))
	    {
		float nearPlane = DXD_MAX_FLOAT;
		int   k, ni=0;

		if (fabs(dx) > fabs(dy))
		    D = (x - sclipped[0]->x)/(sclipped[1]->x - sclipped[0]->x);
		else
		    D = (y - sclipped[0]->y)/(sclipped[1]->y - sclipped[0]->y);
		
		/*
		 * interpolate on screen
		 */
		x = sclipped[0]->x + D*(sclipped[1]->x - sclipped[0]->x);
		y = sclipped[0]->y + D*(sclipped[1]->y - sclipped[0]->y);
		z = sclipped[0]->z + D*(sclipped[1]->z - sclipped[0]->z);

		/*
		 * Perspective transformation is zp = z / depth, where
		 * depth = -z + d.  To regain original z, z = d(zp) / 1 + zp.
		 */
		xyz.z = (ZOFFSET * z) / (1.0 + z);

		if (xyz.z > min && xyz.z < max)
		{
		    float weights[2];

		    /*
		     * now undo perspective on xy by multiplying by depth
		     */
		    depth = -xyz.z + ZOFFSET;
		    xyz.x = x * depth;
		    xyz.y = y * depth;

		    for (k = 0; k < 2; k++)
			if (orig[k] != -1)
			{
			    float dx = sPoints[orig[k]].x - xy.x;
			    float dy = sPoints[orig[k]].y - xy.y;
			    float n = sqrt(dx*dx + dy*dy);
			    if (n < nearPlane)
			    {
				ni = orig[k];
				nearPlane = n;
			    }
			}

		    if (fabs(dx) > fabs(dy))
			D = (x - sPoints[p].x)/(sPoints[q].x - sPoints[p].x);
		    else
			D = (y - sPoints[p].y)/(sPoints[q].y - sPoints[p].y);
		    
		    weights[0] = 1.0 - D;
		    weights[1] = D;
		    
		    pick->pickPath[pick->depth+0] = i;
		    pick->pickPath[pick->depth+1] = ni;
		    if (! AddPick(pick, poke, xyz, 2, -1, weights))
			goto error;
		}
	    }

	}
    }

    DXFree((Pointer)sPoints);
    return OK;

error:
    DXFree((Pointer)sPoints);
    return ERROR;
}

static Error
Elt1D_NoPerspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints,
		    int *elts, int nPts, int nElts, float nearPlane, float sens,
		    float min, float max)
{
    int i;
    int orig[2];

    nearPlane += ZOFFSET;
    
    for (i = 0; i < nElts; i++, elts += 2)
    {
	int     p, q;
	Point   *px, *qx, *xclipped[2];
	Point   xclippt;
	float   x, y, a, b, c, D, dx, dy;
	Point   xyz;

	p = elts[0];
	q = elts[1];

	px = xPoints + p;
	qx = xPoints + q;

	if (px->z > nearPlane && qx->z > nearPlane)
	    continue;
	
	if (px->z <= nearPlane && qx->z <= nearPlane)
	{
	    xclipped[0] = px;
	    xclipped[1] = qx;

	    orig[0] = p;
	    orig[1] = q;
	}
	else if (px->z > nearPlane && qx->z <= nearPlane)
	{
	    float d = (nearPlane - qx->z) / (px->z - qx->z);

	    xclippt.x = qx->x + d*(px->x - qx->x);
	    xclippt.y = qx->y + d*(px->y - qx->y);
	    xclippt.z = nearPlane;
	    xclipped[0] = &xclippt;
	    xclipped[1] = qx;

	    orig[0] = -1;
	    orig[1] = q;
	}
	else
	{
	    float d = (nearPlane - qx->z) / (px->z - qx->z);

	    xclippt.x = qx->x + d*(px->x - qx->x);
	    xclippt.y = qx->y + d*(px->y - qx->y);
	    xclippt.z = nearPlane;
	    xclipped[0] = &xclippt;
	    xclipped[1] = px;

	    orig[0] = -1;
	    orig[1] = p;
	}

	dx = xclipped[1]->x - xclipped[0]->x;
	dy = xclipped[1]->y - xclipped[0]->y;

	if (dx == 0 && dy == 0)
	    continue;
	
	D = 1.0 / sqrt(dx*dx + dy*dy);

	a = dy * D;
	b = -dx * D;
	c = -(a*xclipped[0]->x + b*xclipped[0]->y);

	D = a*xy.x + b*xy.y + c;

	if (-sens < D && D < sens)
	{

	    /*
	     * step along line normal by -D to get closest point on line
	     */
	    x = xy.x - D*a;
	    y = xy.y - D*b;

	    if ((((xclipped[0]->x-sens) <= x && x <= (xclipped[1]->x+sens)) ||
		 ((xclipped[0]->x+sens) >= x && x >= (xclipped[1]->x-sens))) &&
		(((xclipped[0]->y-sens) <= y && y <= (xclipped[1]->y+sens)) ||
		 ((xclipped[0]->y+sens) >= y && y >= (xclipped[1]->y-sens))))
	    {
		float nearPlane = DXD_MAX_FLOAT;
		int   k, ni=0;

		if (fabs(dx) > fabs(dy))
		    D = (x - xclipped[0]->x)/(xclipped[1]->x - xclipped[0]->x);
		else
		    D = (y - xclipped[0]->y)/(xclipped[1]->y - xclipped[0]->y);
		
		/*
		 * interpolate on screen
		 */
		xyz.x = x;
		xyz.y = y;
		xyz.z = xclipped[0]->z + D*(xclipped[1]->z - xclipped[0]->z);

		if (xyz.z > min && xyz.z < max)
		{
		    float weights[2];

		    for (k = 0; k < 2; k++)
			if (orig[k] != -1)
			{
			    float dx = xPoints[orig[k]].x - xy.x;
			    float dy = xPoints[orig[k]].y - xy.y;
			    float n = sqrt(dx*dx + dy*dy);
			    if (n < nearPlane)
			    {
				ni = orig[k];
				nearPlane = n;
			    }
			}
		    
		    if (fabs(dx) > fabs(dy))
			D = (x - xPoints[p].x)/(xPoints[q].x - xPoints[p].x);
		    else
			D = (y - xPoints[p].y)/(xPoints[q].y - xPoints[p].y);
			
		    weights[0] = 1.0 - D;
		    weights[1] = D;
			
		    pick->pickPath[pick->depth+0] = i;
		    pick->pickPath[pick->depth+1] = ni;
		    if (! AddPick(pick, poke, xyz, 2, -1, weights))
			goto error;
		}
	    }
	}

    }

    return OK;

error:
    return ERROR;
}

static Error
PickPolylines(Field f, PickBuf *p, int poke, Point2D xy, Matrix *m,
	int perspective, float nearPlane, float sens, float min, float max)
{
    Array   pA;
    Array   plA, eA;
    Point   *xPoints = NULL;
    int     nPts, nPolylines, nEdges;
    int	    *polylines, *edges;

    pA  = (Array)DXGetComponentValue(f, "positions");
    plA = (Array)DXGetComponentValue(f, "polylines");
    eA  = (Array)DXGetComponentValue(f, "edges");
    if (! plA || ! pA || ! eA)
    {
	DXSetError(ERROR_MISSING_DATA, "positions, polylines or edges");
	goto error;
    }

    DXGetArrayInfo(pA,  &nPts,      NULL, NULL, NULL, NULL);
    DXGetArrayInfo(plA, &nPolylines, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eA,  &nEdges,     NULL, NULL, NULL, NULL);

    /*
     * Transform points
     */
    xPoints = TransformArray(pA, m);
    if (! xPoints)
	return ERROR;

    polylines = (int *)DXGetArrayData(plA);
    edges     = (int *)DXGetArrayData(eA);
    
    if (perspective)
    {
	if (! Polyline_Perspective(p, poke, xy, xPoints, polylines, edges,
			nPts, nPolylines, nEdges, nearPlane, sens, min, max))
	    goto error;
    }
    else
    {
	if (! Polyline_NoPerspective(p, poke, xy, xPoints, polylines, edges,
			nPts, nPolylines, nEdges, nearPlane, sens, min, max))
	    goto error;
    }

    DXFree((Pointer)xPoints);
    return OK;

error:
    DXFree((Pointer)xPoints);
    return ERROR;
}

static Error
Polyline_Perspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints,
    int *plines, int *edges, int nPts, int nPl, int nE, float nearPlane,
    float sens, float min, float max)
{
    int pline;
    int i;
    Point *sPoints;
    
    sPoints = (Point *)DXAllocate(nPts*sizeof(Point));
    if (! sPoints)
	goto error;

    nearPlane += ZOFFSET;
    
    /*
     * perform perspective on all unclipped transformed points
     */
    for (i = 0; i < nPts; i++)
    {
	float z = xPoints[i].z;
	if (z < nearPlane)
	{
	    float invDepth = 1.0 / (-z + ZOFFSET);
	    sPoints[i].x = xPoints[i].x * invDepth;
	    sPoints[i].y = xPoints[i].y * invDepth;
	    sPoints[i].z = xPoints[i].z * invDepth;
	}
    }

    for (pline = 0; pline < nPl; pline++)
    {
	int estart = plines[pline];
	int eend   = (pline == nPl-1) ? nE : plines[pline+1];
	int e, eIndex;

	for (e = estart, eIndex = 0; e < eend; e++, eIndex++)
	{
	    int     p, q;
	    Point   *px, *qx;
	    Point   *ps, *qs, *sclipped[2];
	    Point   sclippt;
	    float   x, y, z, a, b, c, D, dx, dy, depth;
	    Point   xyz;
	    int     orig[2];

	    p = edges[e];
	    q = edges[(e == eend-1) ? estart : e+1];

	    px = xPoints + p;
	    qx = xPoints + q;

	    ps = sPoints + p;
	    qs = sPoints + q;

	    if (px->z > nearPlane && qx->z > nearPlane)
		continue;
	
	    if (px->z <= nearPlane && qx->z <= nearPlane)
	    {
		/*xclipped[0] = px;*/
		/*xclipped[1] = qx;*/

		sclipped[0] = ps;
		sclipped[1] = qs;

		orig[0] = p;
		orig[1] = q;
	    }
	    else if (px->z > nearPlane && qx->z <= nearPlane)
	    {
		float d = (nearPlane - qx->z) / (px->z - qx->z);

		/*xclippt.x = qx->x + d*(px->x - qx->x);*/
		/*xclippt.y = qx->y + d*(px->y - qx->y);*/
		/*xclippt.z = nearPlane;*/
		/*xclipped[0] = &xclippt;*/
		/*xclipped[1] = qx;*/

		sclippt.x = qs->x + d*(ps->x - qs->x);
		sclippt.y = qs->y + d*(ps->y - qs->y);
		sclippt.z = nearPlane;
		sclipped[0] = &sclippt;
		sclipped[1] = qs;

		orig[0] = -1;
		orig[1] = q;
	    }
	    else
	    {
		float d = (nearPlane - qx->z) / (px->z - qx->z);

		/*xclippt.x = qx->x + d*(px->x - qx->x);*/
		/*xclippt.y = qx->y + d*(px->y - qx->y);*/
		/*xclippt.z = nearPlane;*/
		/*xclipped[0] = &xclippt;*/
		/*xclipped[1] = px;*/

		sclippt.x = qs->x + d*(ps->x - qs->x);
		sclippt.y = qs->y + d*(ps->y - qs->y);
		sclippt.z = nearPlane;
		sclipped[0] = &sclippt;
		sclipped[1] = ps;

		orig[0] = -1;
		orig[1] = p;
	    }

	    dx = sclipped[1]->x - sclipped[0]->x;
	    dy = sclipped[1]->y - sclipped[0]->y;

	    if (dx == 0 && dy == 0)
		continue;
	    
	    D = 1.0 / sqrt(dx*dx + dy*dy);

	    a = dy * D;
	    b = -dx * D;
	    c = -(a*sclipped[0]->x + b*sclipped[0]->y);

	    D = a*xy.x + b*xy.y + c;

	    if (-sens < D && D < sens)
	    {
		/*
		 * step along line normal by -D to get closest point on line
		 */
		x = xy.x - D*a;
		y = xy.y - D*b;

		if ((((sclipped[0]->x-sens) <= x && x <= (sclipped[1]->x+sens)) ||
		     ((sclipped[0]->x+sens) >= x && x >= (sclipped[1]->x-sens))) &&
		    (((sclipped[0]->y-sens) <= y && y <= (sclipped[1]->y+sens)) ||
		     ((sclipped[0]->y+sens) >= y && y >= (sclipped[1]->y-sens))))
		{
		    float nearPlane = DXD_MAX_FLOAT;
		    int   k, ni=0;

		    if (fabs(dx) > fabs(dy))
			D = (x - sclipped[0]->x)/(sclipped[1]->x - sclipped[0]->x);
		    else
			D = (y - sclipped[0]->y)/(sclipped[1]->y - sclipped[0]->y);
		    
		    /*
		     * interpolate on screen
		     */
		    x = sclipped[0]->x + D*(sclipped[1]->x - sclipped[0]->x);
		    y = sclipped[0]->y + D*(sclipped[1]->y - sclipped[0]->y);
		    z = sclipped[0]->z + D*(sclipped[1]->z - sclipped[0]->z);

		    /*
		     * Perspective transformation is zp = z / depth, where
		     * depth = -z + d.  To regain original z, z = d(zp) / 1 + zp.
		     */
		    xyz.z = (ZOFFSET * z) / (1.0 + z);

		    if (xyz.z > min && xyz.z < max)
		    {
			float weights[2];

			/*
			 * now undo perspective on xy by multiplying by depth
			 */
			depth = -xyz.z + ZOFFSET;
			xyz.x = x * depth;
			xyz.y = y * depth;

			for (k = 0; k < 2; k++)
			    if (orig[k] != -1)
			    {
				float dx = sPoints[orig[k]].x - xy.x;
				float dy = sPoints[orig[k]].y - xy.y;
				float n = sqrt(dx*dx + dy*dy);
				if (n < nearPlane)
				{
				    ni = orig[k];
				    nearPlane = n;
				}
			    }

			if (fabs(dx) > fabs(dy))
			    D = (x - sPoints[p].x)/(sPoints[q].x - sPoints[p].x);
			else
			    D = (y - sPoints[p].y)/(sPoints[q].y - sPoints[p].y);
			
			weights[0] = 1.0 - D;
			weights[1] = D;
			
			pick->pickPath[pick->depth+0] = pline;
			pick->pickPath[pick->depth+1] = ni;
			if (! AddPick(pick, poke, xyz, 2, eIndex, weights))
			    goto error;
		    }
		}
	    }
	}
    }

    DXFree((Pointer)sPoints);
    return OK;

error:
    DXFree((Pointer)sPoints);
    return ERROR;
}

static Error
Polyline_NoPerspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints,
    int *plines, int *edges, int nPts, int nPl, int nE, float nearPlane,
    float sens, float min, float max)
{
    int pline;
    int orig[2];

    nearPlane += ZOFFSET;
    
    for (pline = 0; pline < nPl; pline++)
    {
	int estart = plines[pline];
	int eend   = (pline == nPl-1) ? nE : plines[pline+1];
	int e, eIndex;

	for (e = estart, eIndex = 0; e < eend; e++, eIndex++)
	{
	    int     p, q;
	    Point   *px, *qx, *xclipped[2];
	    Point   xclippt;
	    float   x, y, a, b, c, D, dx, dy;
	    Point   xyz;

	    p = edges[e];
	    q = edges[(e == eend-1) ? estart : e+1];

	    px = xPoints + p;
	    qx = xPoints + q;

	    if (px->z > nearPlane && qx->z > nearPlane)
		continue;
	
	    if (px->z <= nearPlane && qx->z <= nearPlane)
	    {
		xclipped[0] = px;
		xclipped[1] = qx;

		orig[0] = p;
		orig[1] = q;
	    }
	    else if (px->z > nearPlane && qx->z <= nearPlane)
	    {
		float d = (nearPlane - qx->z) / (px->z - qx->z);

		xclippt.x = qx->x + d*(px->x - qx->x);
		xclippt.y = qx->y + d*(px->y - qx->y);
		xclippt.z = nearPlane;
		xclipped[0] = &xclippt;
		xclipped[1] = qx;

		orig[0] = -1;
		orig[1] = q;
	    }
	    else
	    {
		float d = (nearPlane - qx->z) / (px->z - qx->z);

		xclippt.x = qx->x + d*(px->x - qx->x);
		xclippt.y = qx->y + d*(px->y - qx->y);
		xclippt.z = nearPlane;
		xclipped[0] = &xclippt;
		xclipped[1] = px;

		orig[0] = -1;
		orig[1] = p;
	    }

	    dx = xclipped[1]->x - xclipped[0]->x;
	    dy = xclipped[1]->y - xclipped[0]->y;

	    if (dx == 0 && dy == 0)
		continue;
	    
	    D = 1.0 / sqrt(dx*dx + dy*dy);

	    a = dy * D;
	    b = -dx * D;
	    c = -(a*xclipped[0]->x + b*xclipped[0]->y);

	    D = a*xy.x + b*xy.y + c;

	    if (-sens < D && D < sens)
	    {

		/*
		 * step along line normal by -D to get closest point on line
		 */
		x = xy.x - D*a;
		y = xy.y - D*b;

		if ((((xclipped[0]->x-sens) <= x && x <= (xclipped[1]->x+sens)) ||
		     ((xclipped[0]->x+sens) >= x && x >= (xclipped[1]->x-sens))) &&
		    (((xclipped[0]->y-sens) <= y && y <= (xclipped[1]->y+sens)) ||
		     ((xclipped[0]->y+sens) >= y && y >= (xclipped[1]->y-sens))))
		{
		    float nearPlane = DXD_MAX_FLOAT;
		    int   k, ni=0;

		    if (fabs(dx) > fabs(dy))
			D = (x - xclipped[0]->x)/(xclipped[1]->x - xclipped[0]->x);
		    else
			D = (y - xclipped[0]->y)/(xclipped[1]->y - xclipped[0]->y);
		    
		    /*
		     * interpolate on screen
		     */
		    xyz.x = x;
		    xyz.y = y;
		    xyz.z = xclipped[0]->z + D*(xclipped[1]->z - xclipped[0]->z);

		    if (xyz.z > min && xyz.z < max)
		    {
			float weights[2];

			for (k = 0; k < 2; k++)
			    if (orig[k] != -1)
			    {
				float dx = xPoints[orig[k]].x - xy.x;
				float dy = xPoints[orig[k]].y - xy.y;
				float n = sqrt(dx*dx + dy*dy);
				if (n < nearPlane)
				{
				    ni = orig[k];
				    nearPlane = n;
				}
			    }
			
			if (fabs(dx) > fabs(dy))
			    D = (x - xPoints[p].x)/(xPoints[q].x - xPoints[p].x);
			else
			    D = (y - xPoints[p].y)/(xPoints[q].y - xPoints[p].y);
			    
			weights[0] = 1.0 - D;
			weights[1] = D;
			    
			pick->pickPath[pick->depth+0] = pline;
			pick->pickPath[pick->depth+1] = ni;
			if (! AddPick(pick, poke, xyz, 2, eIndex, weights))
			    goto error;
		    }
		}
	    }
	}

    }

    return OK;

error:
    return ERROR;
}


static Error
PickPoints(Field f, PickBuf *p, int poke, Point2D xy, Matrix *m,
	    int perspective, float nearPlane, float sens, float min, float max)
{
    Array   pA;
    Point   *xPoints = NULL;
    int     nPts;

    pA = (Array)DXGetComponentValue(f, "positions");
    if (! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "positions");
	goto error;
    }

    DXGetArrayInfo(pA, &nPts,  NULL, NULL, NULL, NULL);

    /*
     * Transform points
     */
    xPoints = TransformArray(pA, m);
    if (! xPoints)
	return ERROR;

    if (perspective)
    {
	if (! Elt0D_Perspective(p, poke, xy, xPoints,
				nPts, nearPlane, sens, min, max))
	    goto error;
    }
    else
    {
	if (! Elt0D_NoPerspective(p, poke, xy, xPoints,
				nPts, nearPlane, sens, min, max))
	    goto error;
    }

    DXFree((Pointer)xPoints);
    return OK;

error:
    DXFree((Pointer)xPoints);
    return ERROR;
}

static Error
Elt0D_Perspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints, 
			int nPts, float nearPlane, float sens, float min, float max)
{
    int i;
    Point2D *sPoints;
    
    sPoints = (Point2D *)DXAllocate(nPts*sizeof(Point2D));
    if (! sPoints)
	goto error;

    nearPlane += ZOFFSET;
    
    /*
     * perform perspective on all unclipped transformed points
     */
    for (i = 0; i < nPts; i++)
    {
	float z = xPoints[i].z;
	if (z < nearPlane)
	{
	    float invDepth = 1.0 / (-z + ZOFFSET);
	    sPoints[i].x = xPoints[i].x * invDepth;
	    sPoints[i].y = xPoints[i].y * invDepth;
	}
    }

    for (i = 0; i < nPts; i++)
    {
	if (xPoints[i].z < nearPlane &&
	    fabs(sPoints[i].x - xy.x) < sens &&
	    fabs(sPoints[i].y - xy.y) < sens)
	{
	    if (xPoints[i].z > min && xPoints[i].z < max)
	    {
		pick->pickPath[pick->depth+0] = i;
		pick->pickPath[pick->depth+1] = i;
		if (! AddPick(pick, poke, xPoints[i], 0, -1, NULL))
		    goto error;
	    }
	}
    }

    DXFree((Pointer)sPoints);
    return OK;

error:
    DXFree((Pointer)sPoints);
    return ERROR;
}

static Error
Elt0D_NoPerspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints, 
			int nPts, float nearPlane, float sens, float min, float max)
{
    int i;
    
    nearPlane += ZOFFSET;
    
    for (i = 0; i < nPts; i++)
    {
	if (xPoints[i].z < nearPlane &&
	    fabs(xPoints[i].x - xy.x) < sens &&
	    fabs(xPoints[i].y - xy.y) < sens)
	{
	    if (xPoints[i].z > min && xPoints[i].z < max)
	    {
		pick->pickPath[pick->depth+0] = i;
		pick->pickPath[pick->depth+1] = i;
		if (! AddPick(pick, poke, xPoints[i], 0, -1, NULL))
		    goto error;
	    }
	}
    }

    return OK;

error:
    return ERROR;
}

static Error
PickFLE(Field f, PickBuf *p, int poke, Point2D xy, Matrix *m,
	int perspective, float nearPlane, float sens, float min, float max)
{
    Array   pA;
    Array   fA, lA, eA;
    Point   *xPoints = NULL;
    int     nPts, nFaces, nLoops, nEdges;
    int     *faces, *loops, *edges;

    pA = (Array)DXGetComponentValue(f, "positions");
    fA = (Array)DXGetComponentValue(f, "faces");
    lA = (Array)DXGetComponentValue(f, "loops");
    eA = (Array)DXGetComponentValue(f, "edges");
    if (! lA || ! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "positions, faces, loops or edges");
	goto error;
    }

    DXGetArrayInfo(pA, &nPts,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo(fA, &nFaces, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(lA, &nLoops, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(eA, &nEdges, NULL, NULL, NULL, NULL);

    /*
     * Transform points
     */
    xPoints = TransformArray(pA, m);
    if (! xPoints)
	return ERROR;

    faces = (int *)DXGetArrayData(fA);
    loops = (int *)DXGetArrayData(lA);
    edges = (int *)DXGetArrayData(eA);
    
    if (perspective)
    {
	if (! FLE_Perspective(p, poke, xy, xPoints, nPts,
		faces, nFaces, loops, nLoops, edges, nEdges, nearPlane, sens,
		min, max))
	    goto error;
    }
    else
    {
	if (! FLE_NoPerspective(p, poke, xy, xPoints, nPts,
		faces, nFaces, loops, nLoops, edges, nEdges, nearPlane, sens,
		min, max))
	    goto error;
    }

    DXFree((Pointer)xPoints);
    return OK;

error:
    DXFree((Pointer)xPoints);
    return ERROR;
}

static Error
FLE_NoPerspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints,
       int nPts, int *faces, int nFaces, int *loops, int nLoops, int *edges,
       int nEdges, float nearPlane, float sens, float min, float max)
{
    int f, found;
    int l, fl, ll;
    int e, fe, le;

    for (f = 0; f < nFaces; f++)
    {
	int count = 0;

	fl = faces[f];
	ll = (f == nFaces-1) ? nLoops - 1 : faces[f+1] - 1;

	for (l = fl, found = 0; l <= ll && !found; l++)
	{
	    int start, end;

	    fe = loops[l];
	    le = (l == nLoops-1) ? nEdges - 1 : loops[l+1] - 1;
	    
	    end = edges[le];
	    for (e = fe; e <= le && !found; e++)
	    {
		float x0, x1;
		float y0, y1;

		start = end;
		end = edges[e];

		x0 = xPoints[start].x;
		y0 = xPoints[start].y;

		x1 = xPoints[end].x;
		y1 = xPoints[end].y;

		if (y0 == y1)
		    continue;

		if (y0 > y1)
		{ 
		    float tt;
		    tt = x0;
		    x0 = x1;
		    x1 = tt;
		    tt = y0;
		    y0 = y1;
		    y1 = tt;
		}

		/*
		 * If the edge's y-range (closed at the top) spans
		 * the xy y value, we may have a hit.
		 */
		if (xy.y > y0 && xy.y <= y1)
		{
		    /*
		     * If both edge endpoints are to the right of
		     * the xy x point, we have a hit.  Otherwise, if
		     * the edge x-range spans the xy point, we get
		     * the intersection of the edge with the y = xy.y
		     * line.  If this intersection is to the right of
		     * the xy point, we have a hit.
		     */
		    if (x0 > xy.x && x1 >= xy.x)
		    {
			count ++;
		    }
		    else if ((x0 < xy.x && x1 >= xy.x) ||
			     (x0 > xy.x && x1 <= xy.x))
		    {
			float x, d = (xy.y - y0) / (y1 - y0);
			x = x0 + d*(x1 - x0);
			if (x == xy.x)
			    found = 1;

			else if (x > xy.x)
			    count ++;
		    }
		}
	    }
	}

	/*
	 * If count is odd, then xy is inside the FLE.  Determine the xyz point
	 * and add it to the pick buf.  Find the point by computing a planar
	 * equation and evaluating it for Z at the xy point
	 */
	if (found || count & 0x1)
	{
	    Vector v0, v1, vn, normal;
	    float l0, l0i=0;
	    float l1=0, l1i=0;
	    float l, a, b, c, d;
	    int p, q, r, k;
	    Point xyz;
	    float nearPlane = DXD_MAX_FLOAT;
	    int ni=0;

	    /*
	     * Compute a planar equation from the first loop in the face
	     */

	    fe = loops[fl];
	    le = (fl == nLoops-1) ? nEdges - 1 : loops[fl+1] - 1;

	    normal = DXPt(0.0,0.0,0.0);

	    q = edges[le];
	    r = edges[fe];
	    for (e = fe; e <= le; e++)
	    {
		p = q;
		q = r;
		r = (e == le) ? edges[fe] : edges[e+1];
	    
		if (e == fe)
		{
		    v0 = DXSub(xPoints[q], xPoints[p]);
		    l0 = DXLength(v0);
		    if (l0 != 0.0)
			l0i = 1.0 / l0;
		}
		else
		{ 
		    v0  = v1;
		    l0  = l1;
		    l0i = l1i;
		}

		v1 = DXSub(xPoints[r], xPoints[q]);
		l1 = DXLength(v1);

		if (l1 == 0.0 || l0 == 0.0)
		    continue;
		
		l1i = 1.0 / l1;

		vn = DXMul(DXCross(v0, v1), l0i*l1i);
		
		l = DXLength(vn);
		if (l > 0.0001)
		    normal = DXAdd(DXMul(vn, 1.0/l), normal);
	    }

	    l = DXLength(normal);
	    if (l == 0.0)
	    {
	        DXSetError(ERROR_INTERNAL, "hit a zero area FLE?");
		return ERROR;
	    }

	    l = 1.0 / l;
	    a = normal.x * l;
	    b = normal.y * l;
	    c = normal.z * l;

	    if (c == 0.0)
	    {
		DXSetError(ERROR_INTERNAL, "hit an edge-on FLE?");
		return ERROR;
	    }

	    d = -(a*xPoints[q].x + b*xPoints[q].y + c*xPoints[q].z);

	    xyz.x = xy.x;
	    xyz.y = xy.y;
	    xyz.z = -(a*xy.x + b*xy.y + d) / c;

	    for (k = fl; k <= ll; k++)
	    {
		fe = loops[k];
		le = (k == nLoops-1) ? nEdges - 1 : loops[k+1] - 1;
		
		for (e = fe; e <= le && !found; e++)
		{
		    float dx = xPoints[edges[e]].x - xy.x;
		    float dy = xPoints[edges[e]].y - xy.y;
		    float n = sqrt(dx*dx + dy*dy);
		    if (n < nearPlane)
		    {
			ni = edges[e];
			nearPlane = n;
		    }
		}
	    }
		
	    if (xyz.z > min && xyz.z < max)
	    {
		pick->pickPath[pick->depth+0] = f;
		pick->pickPath[pick->depth+1] = ni;
		if (! AddPick(pick, poke, xyz, 0, -1, NULL))
		    goto error;
	    }
	}
    }

    return OK;

error:
    return ERROR;
}

#define MAX_CLIPPTS 100

static Error
FLE_Perspective(PickBuf *pick, int poke, Point2D xy, Point *xPoints,
       int nPts, int *faces, int nFaces, int *loops, int nLoops, int *edges,
       int nEdges, float nearPlane, float sens, float min, float max)
{
    int i, f, found;
    int l, fl, ll;
    int e, fe, le;
    float dNear, z, depth;
    int vKnt=0, cKnt, vLen = 100;
    Point cPoints[MAX_CLIPPTS];
    Point **cLoop   = (Point **)DXAllocate(vLen*sizeof(Pointer));
    Point *sPoints  = (Point *)DXAllocate(nPts*sizeof(Point));
    if (! sPoints || ! cLoop)
	goto error;

    nearPlane += ZOFFSET;
    dNear = -1.0 / nearPlane;
    
    /*
     * perform perspective on all unclipped transformed points
     */
    for (i = 0; i < nPts; i++)
    {
	float z = xPoints[i].z;
	if (z < nearPlane)
	{
	    float invDepth = 1.0 / (-z + ZOFFSET);
	    sPoints[i].x = xPoints[i].x * invDepth;
	    sPoints[i].y = xPoints[i].y * invDepth;
	    sPoints[i].z = xPoints[i].z * invDepth;
	}
    }

    for (f = 0; f < nFaces; f++)
    {
	int count = 0;

	fl = faces[f];
	ll = (f == nFaces-1) ? nLoops - 1 : faces[f+1] - 1;

	for (l = fl, found = 0; l <= ll && !found; l++)
	{
	    Point *start, *end;

	    fe = loops[l];
	    le = (l == nLoops-1) ? nEdges - 1 : loops[l+1] - 1;

	    vKnt = cKnt = 0;
	    for (e = fe; e <= le; e++)
	    {
		int tv, nv;
		int ne = (e == le) ? fe : e + 1;
		tv = edges[e];
		nv = edges[ne];

		if (xPoints[tv].z <= nearPlane)
		{
		    if (vKnt == vLen)
		    {
			cLoop = (Point **)DXReAllocate(cLoop,
						2*vLen*sizeof(Pointer));
			if (cLoop == NULL)
			    goto error;
			vLen *= 2;
		    }

		    cLoop[vKnt++] = sPoints + tv;
		}

		if ((xPoints[tv].z < nearPlane && xPoints[nv].z > nearPlane) ||
		    (xPoints[tv].z > nearPlane && xPoints[nv].z < nearPlane))
		{
		    float d = (nearPlane - sPoints[tv].z) / 
				    (sPoints[nv].z - sPoints[tv].z);

		    if (cKnt == MAX_CLIPPTS)
		    {
			DXSetError(ERROR_DATA_INVALID, "too many clip points");
			goto error;
		    }

		    cPoints[cKnt].x = dNear*(xPoints[tv].x + d*(xPoints[nv].x-xPoints[tv].x));
		    cPoints[cKnt].y = dNear*(xPoints[tv].y + d*(xPoints[nv].y-xPoints[tv].y));
		    cPoints[cKnt].z = dNear*(xPoints[tv].z + d*(xPoints[nv].z-xPoints[tv].z));

		    cLoop[vKnt++] = cPoints + cKnt++;
		}
	    }

	    if (vKnt < 3)
		continue;
	    
	    end = cLoop[vKnt-1];
	    for (e = 0; e < vKnt && !found; e++)
	    {
		float x0, x1;
		float y0, y1;

		start = end;
		end   = cLoop[e];

		x0 = start->x;
		y0 = start->y;

		x1 = end->x;
		y1 = end->y;

		if (y0 == y1)
		    continue;

		if (y0 > y1)
		{ 
		    float tt;
		    tt = x0;
		    x0 = x1;
		    x1 = tt;
		    tt = y0;
		    y0 = y1;
		    y1 = tt;
		}

		/*
		 * If the edge's y-range (closed at the top) spans
		 * the xy y value, we may have a hit.
		 */
		if (xy.y > y0 && xy.y <= y1)
		{
		    /*
		     * If both edge endpoints are to the right of
		     * the xy x point, we have a hit.  Otherwise, if
		     * the edge x-range spans the xy point, we get
		     * the intersection of the edge with the y = xy.y
		     * line.  If this intersection is to the right of
		     * the xy point, we have a hit.
		     */
		    if (x0 > xy.x && x1 >= xy.x)
		    {
			count ++;
		    }
		    else if ((x0 < xy.x && x1 >= xy.x) ||
			     (x0 > xy.x && x1 <= xy.x))
		    {
			float x, d = (xy.y - y0) / (y1 - y0);
			x = x0 + d*(x1 - x0);
			if (x == xy.x)
			    found = 1;

			else if (x > xy.x)
			    count ++;
		    }
		}
	    }
	}

	/*
	 * If count is odd, then xy is inside the FLE.  Determine the xyz point
	 * and add it to the pick buf.  Find the point by computing a planar
	 * equation and evaluating it for Z at the xy point
	 */
	if (found || count & 0x1)
	{
	    Vector v0, v1, vn, normal;
	    float l0, l0i=0;
	    float l1=0, l1i=0;
	    float l, a, b, c, d;
	    Point *p, *q, *r;
	    Point xyz;
	    int k, ni=0;

	    /*
	     * Compute a planar equation
	     */

	    normal = DXPt(0.0,0.0,0.0);

	    q = cLoop[vKnt-1];
	    r = cLoop[0];
	    for (e = 0; e < vKnt; e++)
	    {
		p = q;
		q = r;
		r = (e == vKnt-1) ? cLoop[0] : cLoop[e+1];
	    
		if (e == 0)
		{
		    v0 = DXSub(*q, *p);
		    l0 = DXLength(v0);
		    if (l0 != 0.0)
			l0i = 1.0 / l0;
		}
		else
		{ 
		    v0  = v1;
		    l0  = l1;
		    l0i = l1i;
		}

		v1 = DXSub(*r, *q);
		l1 = DXLength(v1);

		if (l1 == 0.0 || l0 == 0.0)
		    continue;
		
		l1i = 1.0 / l1;

		vn = DXMul(DXCross(v0, v1), l0i*l1i);
		
		l = DXLength(vn);
		if (l > 0.0001)
		    normal = DXAdd(DXMul(vn, 1.0/l), normal);
	    }

	    l = DXLength(normal);
	    if (l == 0.0)
	    {
	        DXSetError(ERROR_INTERNAL, "hit a zero area FLE?");
		return ERROR;
	    }

	    l = 1.0 / l;
	    a = normal.x * l;
	    b = normal.y * l;
	    c = normal.z * l;

	    if (c == 0.0)
	    {
		DXSetError(ERROR_INTERNAL, "hit an edge-on FLE?");
		return ERROR;
	    }

	    d = -(a*q->x + b*q->y + c*q->z);

	    xyz.x = xy.x;
	    xyz.y = xy.y;
	    xyz.z = -(a*xy.x + b*xy.y + d) / c;
	
	    z = xyz.z = (ZOFFSET * xyz.z) / (1.0 + xyz.z);
	    depth = ZOFFSET - z;
	    xyz.x *= depth;
	    xyz.y *= depth;


	    for (k = fl; k <= ll; k++)
	    {
		fe = loops[k];
		le = (k == nLoops-1) ? nEdges - 1 : loops[k+1] - 1;
		
		for (e = fe; e <= le && !found; e++)
		    if (xPoints[edges[e]].z < nearPlane)
		    {
			float dx = sPoints[edges[e]].x - xy.x;
			float dy = sPoints[edges[e]].y - xy.y;
			float n = sqrt(dx*dx + dy*dy);
			if (n < nearPlane)
			{
			    ni = edges[e];
			    nearPlane = n;
			}
		    }
	    }
		
	    if (xyz.z > min && xyz.z < max)
	    {
		pick->pickPath[pick->depth+0] = f;
		pick->pickPath[pick->depth+1] = ni;
		if (! AddPick(pick, poke, xyz, 0, -1, NULL))
		    goto error;
	    }
	}
    }

    DXFree((Pointer)cLoop);
    DXFree((Pointer)sPoints);
    return OK;

error:
    DXFree((Pointer)cLoop);
    DXFree((Pointer)sPoints);
    return ERROR;
}

static Field
GetFirstHits(Field field)
{
    int     i;
    Field   newField = NULL;
    Array   inPointsA, inPicksA, inPokesA, inPathsA, inWeightsA;
    Array   outPointsA = NULL, outPicksA = NULL,
	    outPokesA = NULL, outPathsA = NULL,
	    outWeightsA = NULL;
    float   *inPoints, *outPoints;
    int     *inPicks, *outPicks;
    int     *inPokes, *outPokes;
    int     *inPaths, *outPaths;
    ubyte   *inWeights, *outWeights;
    PickWts *wts;
    int     wtSz;
    int     pathSz;
    int     nPokes, nPicks, nPaths;

    if (DXEmptyField(field))
	return DXEndField(DXNewField());
    
    inPointsA  = (Array)DXGetComponentValue(field, "positions");
    inPicksA   = (Array)DXGetComponentValue(field, "picks");
    inPokesA   = (Array)DXGetComponentValue(field, "pokes");
    inPathsA   = (Array)DXGetComponentValue(field, "pick paths");
    inWeightsA = (Array)DXGetComponentValue(field, "pick weights");

    if (! inPointsA || ! inPicksA || ! inPokesA || ! inPathsA)
    {
	DXSetError(ERROR_MISSING_DATA, "pick result");
	goto error;
    }

    DXGetArrayInfo(inPokesA,  &nPokes, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(inPathsA,  &nPaths, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(inPicksA,  &nPicks, NULL, NULL, NULL, NULL);

    inPicks   = (int   *)DXGetArrayData(inPicksA);
    inPokes   = (int   *)DXGetArrayData(inPokesA);
    inPaths   = (int   *)DXGetArrayData(inPathsA);
    inPoints  = (float *)DXGetArrayData(inPointsA);
    inWeights = (ubyte *)DXGetArrayData(inWeightsA);

    pathSz = 0; wtSz = 0; wts = (PickWts *)inWeights;
    for (i = 0; i < nPokes; i++)
    {
	int poke = inPokes[i];
	int pick = inPicks[poke];
	int np  = ((i == nPokes-1) ? nPicks  : inPokes[i+1]) - poke;
	int psz = ((poke == nPicks-1) ? nPaths : inPicks[poke+1]) - pick;
	int k;

	pathSz += psz;

	for (k = 0; k < np; k++)
	{
	    int n = wts->nweights;
	    int wsz = sizeof(PickWts) + n*sizeof(float);

	    if (k == 0)
		wtSz += wsz;
	    
	    wts = (PickWts *)(((ubyte *)wts) + wsz);
	}
    }

    outPointsA  = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    outPicksA   = DXNewArray(TYPE_INT,   CATEGORY_REAL, 0);
    outPokesA   = DXNewArray(TYPE_INT,   CATEGORY_REAL, 0);
    outPathsA   = DXNewArray(TYPE_INT,   CATEGORY_REAL, 0);
    outWeightsA = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);

    if (!outPointsA || !outPicksA || !outPokesA || !outPathsA || !outWeightsA)
	goto error;
    
    if (! DXAddArrayData(outPointsA,  0, nPokes, NULL) ||
        ! DXAddArrayData(outPokesA,   0, nPokes, NULL) ||
        ! DXAddArrayData(outPicksA,   0, nPokes, NULL) ||
        ! DXAddArrayData(outPathsA,   0, pathSz, NULL) ||
        ! DXAddArrayData(outWeightsA, 0,   wtSz, NULL))
	goto error;

    outPicks    = (int   *)DXGetArrayData(outPicksA);
    outPokes    = (int   *)DXGetArrayData(outPokesA);
    outPaths    = (int   *)DXGetArrayData(outPathsA);
    outPoints   = (float *)DXGetArrayData(outPointsA);
    outWeights  = (ubyte *)DXGetArrayData(outWeightsA);

    pathSz = 0; wtSz = 0; wts = (PickWts *)inWeights;
    for (i = 0; i < nPokes; i++)
    {
	int poke = inPokes[i];
	int pick = inPicks[poke];
	int np  = ((i == nPokes-1) ? nPicks  : inPokes[i+1]) - poke;
	int psz = ((poke == nPicks-1) ? nPaths : inPicks[poke+1]) - pick;
	int k;

	memcpy((char *)outPoints, (char *)(inPoints+3*poke), 3*sizeof(float));
	outPoints += 3;

	outPokes[i] = i;
	outPicks[i] = pathSz;

	memcpy((char *)outPaths, (char *)(inPaths+pick), psz*sizeof(int));
	outPaths += psz;
	pathSz   += psz;

	for (k = 0; k < np; k++)
	{
	    int n = wts->nweights;
	    int wsz = sizeof(PickWts) + n*sizeof(float);

	    if (k == 0)
	    {
		memcpy((char *)outWeights, (char *)wts, wsz);
		outWeights += wsz;
	    }
	    
	    wts = (PickWts *)(((ubyte *)wts) + wsz);
	}
    }

    newField = DXNewField();
    if (! newField)
	goto error;
    
    if ((! DXSetStringAttribute((Object)outPointsA,  "dep", "positions"))   ||
        (! DXSetStringAttribute((Object)outPokesA,   "ref", "picks"))       ||
        (! DXSetStringAttribute((Object)outPicksA,   "dep", "positions"))   ||
        (! DXSetStringAttribute((Object)outPicksA,   "ref", "pick paths")))
	goto error;
    
    if (! DXSetComponentValue(newField, "positions", (Object)outPointsA))
	goto error;
    outPointsA = NULL;

    if (! DXSetComponentValue(newField, "pokes", (Object)outPokesA))
	goto error;
    outPokesA = NULL;

    if (! DXSetComponentValue(newField, "picks", (Object)outPicksA))
	goto error;
    outPicksA = NULL;

    if (! DXSetComponentValue(newField, "pick paths", (Object)outPathsA))
	goto error;
    outPathsA = NULL;

    if (! DXSetComponentValue(newField, "pick weights", (Object)outWeightsA))
	goto error;
    outPathsA = NULL;

    return newField;

error:
    DXDelete((Object)outWeightsA);
    DXDelete((Object)outPointsA);
    DXDelete((Object)outPicksA);
    DXDelete((Object)outPokesA);
    DXDelete((Object)outPathsA);
    DXDelete((Object)newField);
    return NULL;
}

static Error PickTraverse(Object, Matrix *, Point,
			int *, int, int, int, Field, int, int, float *);

static Error
PickInterpolate(Object object, Field picks, int flag)
{
    Array array, wArray = NULL;
    PickWts *wPtr;
    Point *point;
    int npokes, npicks;
    int i, j;

    if (DXEmptyField(picks))
	return OK;

    array = (Array)DXGetComponentValue(picks, "positions");
    point = (Point *)DXGetArrayData(array);

    wArray = (Array)DXGetComponentValue(picks, "pick weights");
    if (! wArray)
    {
	DXSetError(ERROR_MISSING_DATA, "pick weights");
	goto error;
    }

    DXReference((Object)wArray);
    DXDeleteComponent(picks, "pick weights");

    wPtr = (PickWts *)DXGetArrayData(wArray);

    if (! DXQueryPokeCount(picks, &npokes))
	goto error;
    
    for (i = 0; i < npokes; i++)
    {

	if (! DXQueryPickCount(picks, i, &npicks))
	    goto error;
	
	for (j = 0; j < npicks; j++, point++)
	{
	    int *path, elt, vert, length;
	    float *wts = (float *)(wPtr + 1);

	    if (! DXQueryPickPath(picks, i, j, &length, &path, &elt, &vert))
		goto error;
	    
	    if (! PickTraverse(object, NULL, *point, path, length, elt,
				vert, picks, flag, wPtr->segment, wts))
		goto error;

	    wPtr = (PickWts *)(((char *)wPtr)+sizeof(PickWts)+wPtr->nweights*sizeof(float));
	}
    }

    DXDelete((Object)wArray);
    return OK;

error:
    DXDelete((Object)wArray);
    return ERROR;
}

#define INTERPOLATE_PICK(type, rnd)					  \
{									  \
    int i, j;								  \
    type *_dst = (type *)dstD;						  \
									  \
    for (i = 0; i < nItems; i++)					  \
	buf[i] = 0.0;							  \
    									  \
    for (i = 0; i < vPerE; i++)						  \
    {									  \
	type *_src = ((type *)DXGetArrayData(srcA)) + element[i]*nItems;  \
	for (j = 0; j < nItems; j++)					  \
	    buf[j] += wts[i]*_src[j];					  \
    }									  \
    									  \
    for (i = 0; i < nItems; i++)					  \
	_dst[i] = (type)(buf[i] + rnd);					  \
}									  

static Error
PickTraverse(Object object, Matrix *stack, Point xyz,
    int *path, int length, int eid, int vid,
    Field picks, int flag, int seg, float *wts)
{
    switch(DXGetObjectClass(object))
    {
	case CLASS_FIELD:
	{
	    Field    f = (Field)object;
	    Type     t;
	    Category c;
	    int      r, s[32];
	    Array    srcA, dstA;
	    Pointer  *srcD, *dstD;
	    int      itemSize;
	    int      dstL;
	    int	     srcI;
	    char     *name;
	    int      index;
	    char     *dstName;
	    Object   attr;
	    Array    eltsA, edgesA;
	    int      vPerE;
	    int	     *element=NULL;
	    int      lineseg[2];

	    if (length != 0)
	    {
		DXSetError(ERROR_INTERNAL, 
			"path continues past a field object");
		goto error;
	    }

	    if (flag == INTERPOLATE)
	    {
		eltsA = (Array)DXGetComponentValue(f, "connections");
		if (eltsA)
		{
		    DXGetArrayInfo(eltsA, NULL, NULL, NULL, NULL, &vPerE);
		    element = ((int *)DXGetArrayData(eltsA)) + eid*vPerE;
		}
		else 
		{
		    eltsA = (Array)DXGetComponentValue(f, "polylines");
		    if (eltsA)
		    {
			int estart, eend, *polylines, *edges;
			int nP, nE;

			edgesA = (Array)DXGetComponentValue(f, "edges");

			polylines = (int *)DXGetArrayData(eltsA);
			edges     = (int *)DXGetArrayData(edgesA);

			DXGetArrayInfo(eltsA,  &nP, NULL, NULL, NULL, NULL);
			DXGetArrayInfo(edgesA, &nE, NULL, NULL, NULL, NULL);

			estart = polylines[eid];
			eend   = (eid == nP-1) ? nE : polylines[eid+1];

			lineseg[0] = edges[estart + seg];
			if (estart + seg + 1 == eend)
			    lineseg[1] = edges[estart];
			else
			    lineseg[1] = edges[estart + seg + 1];
			
			element = lineseg;
			vPerE = 2;
		    }
		    else
		    {
			flag = CLOSEST_VERTEX;
		    }
		}
	    }

	    for (index = 0;
	         NULL != (srcA = 
			(Array)DXGetEnumeratedComponentValue(f, index, &name));
		 index ++)
	    {
		char *dependency;

		attr = DXGetAttribute((Object)srcA, "dep");
		if (! attr)
		    continue;
		
		dependency = DXGetString((String)attr);
		
		if (! strcmp(name, "positions"))
		    dstName = "closest vertex";
		else
		    dstName = name;

		DXGetArrayInfo(srcA, NULL, &t, &c, &r, s);

		dstA = (Array)DXGetComponentValue(picks, dstName);
		if (! dstA)
		{

		    dstA = DXNewArrayV(t, c, r, s);
		    if (! dstA)
			goto error;
			
		    if (! DXSetAttribute((Object)dstA, "dep",
					(Object)DXNewString("positions")))
			goto error;

		    if (! DXSetComponentValue(picks, dstName, (Object)dstA))
			goto error;
		}
		else
		    if (! DXTypeCheckV(dstA, t, c, r, s))
		    {
			DXSetError(ERROR_DATA_INVALID,
			    "mismatched data components");
			goto error;
		    }
	    
		DXGetArrayInfo(dstA, &dstL, NULL, NULL, NULL, NULL);

		if (! DXAddArrayData(dstA, dstL, 1, NULL))
		    goto error;
		
		itemSize = DXGetItemSize(dstA);

		dstD = (Pointer)((char *)DXGetArrayData(dstA)
							+ dstL*itemSize);

		if (flag == CLOSEST_VERTEX ||
		    !strcmp(dependency, "connections") ||
		    !strcmp(dependency, "faces") || 
		    !strcmp(dependency, "polylines") || 
		    !strcmp(dstName, "closest vertex"))
		{
		    if (! strcmp(dependency, "connections") ||
			! strcmp(dependency, "polylines") ||
			! strcmp(dependency, "faces"))
			srcI = eid;
		    else
			srcI = vid;

		    srcD = (Pointer)((char *)DXGetArrayData(srcA)
						    + srcI*itemSize);

		    memcpy(dstD, srcD, itemSize);
		}
		else if (flag == INTERPOLATE)
		{
		    int nItems = DXGetItemSize(dstA) / DXTypeSize(t);
		    float *buf = (float *)DXAllocate(nItems*sizeof(float));
		    if (! buf)
			goto error;

		    switch(t)
		    {
			case TYPE_DOUBLE: INTERPOLATE_PICK(double,0.0); break;
			case TYPE_FLOAT:  INTERPOLATE_PICK(float,0.0);  break;
			case TYPE_INT:    INTERPOLATE_PICK(int,0.5);    break;
			case TYPE_UINT:   INTERPOLATE_PICK(uint,0.5);   break;
			case TYPE_SHORT:  INTERPOLATE_PICK(short,0.5);  break;
			case TYPE_USHORT: INTERPOLATE_PICK(ushort,0.5); break;
			case TYPE_BYTE:   INTERPOLATE_PICK(byte,0.5);   break;
			case TYPE_UBYTE:  INTERPOLATE_PICK(ubyte,0.5);  break;
		    	default: break;
		    }

		    DXFree((Pointer)buf);
		}
	    }


	    return OK;
	}

	case CLASS_GROUP:
	{
	    Group g = (Group)object;
	    
	    object = DXGetEnumeratedMember(g, *path, NULL);
	    if (! object)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"pick path references non-existent group member");
		goto error;
	    }

	    return PickTraverse(object, stack, xyz, path+1, length-1,
				eid, vid, picks, flag, seg, wts);
	}

	case CLASS_XFORM:
	{
	    Matrix m;
	    Xform x = (Xform)object;

	    if (*path != 0)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"pick path references invalid xform child");
		goto error;
	    }

	    DXGetXformInfo(x, &object, &m);
	    if (stack)
	    {
		Matrix p = DXConcatenate(m, *stack);
		return PickTraverse(object, &p, xyz, path+1, length-1,
				    eid, vid, picks, flag, seg, wts);
	    }
	    else
		return PickTraverse(object, &m, xyz, path+1, length-1,
				    eid, vid, picks, flag, seg, wts);
	}

	case CLASS_CLIPPED:
	{
	    Clipped c = (Clipped)object;

	    if (*path != 0)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"pick path references invalid Clipped child");
		goto error;
	    }

	    DXGetClippedInfo(c, &object, NULL);

	    return PickTraverse(object, stack, xyz, path+1, length-1,
					eid, vid, picks, flag, seg, wts);
	}
		
	case CLASS_SCREEN:
	{
	    Screen s = (Screen)object;

	    if (*path != 0)
	    {
		DXSetError(ERROR_DATA_INVALID, 
			"pick path references invalid Screen child");
		goto error;
	    }

	    DXGetScreenInfo(s, &object, NULL, NULL);

	    return PickTraverse(object, stack, xyz, path+1, length-1,
					eid, vid, picks, flag, seg, wts);
	}

	default:
	{
	    DXSetError(ERROR_BAD_CLASS, 
		"invalid object encountered in traversal");
	    goto error;
	}
    }

error:
    return ERROR;
}


#if 0
static Error
GetAAMatrix(Object o, Matrix *r)
{
    Matrix p, m;

    if (DXGetObjectClass(o) != CLASS_XFORM)
	goto error;
    
    DXGetXformInfo((Xform)o, &o, &p);

    if (DXGetObjectClass(o) != CLASS_XFORM)
	goto error;
    
    DXGetXformInfo((Xform)o, &o, &m);
    
    p = DXConcatenate(m, p);

    if (DXGetObjectClass(o) != CLASS_GROUP ||
	DXGetGroupClass((Group)o) != CLASS_GROUP)
	goto error;
    
    o = DXGetEnumeratedMember((Group)o, 1, NULL);

    if (DXGetObjectClass(o) != CLASS_XFORM)
	goto error;
    
    DXGetXformInfo((Xform)o, &o, &m);
    
    *r = DXConcatenate(m, p);

    return OK;

error:
    DXSetError(ERROR_DATA_INVALID,
	"unrecognized object in AutoAxes object header");
    return ERROR;
}
#endif

static Array
getXY(Array in)
{
    int i, nIn, nOut, *dataOut;
    DXEvents dataIn;
    Type t;
    Category c;
    int rank, nDim;
    Array out = NULL;

    if (DXGetObjectClass((Object)in) != CLASS_ARRAY)
    {
	DXSetError(ERROR_DATA_INVALID, "pick list must be class ARRAY");
	return NULL;
    }

    DXGetArrayInfo(in, &nIn, &t, &c, &rank, &nDim);

    if (t != TYPE_INT || c != CATEGORY_REAL || rank != 1)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"pick list array of real integer vectors");
	return NULL;
    }

    if (nDim == 2)
	return in;

    dataIn = (DXEvents)DXGetArrayData(in);
    for (i = nOut = 0; i < nIn; i++)
    {
	int b = WHICH_BUTTON(&dataIn[i]);
	if (b != NO_BUTTON && dataIn[i].mouse.state == BUTTON_DOWN)
	    nOut++;
    }
    
    if (nOut == 0)
	return NULL;
    
    out = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    if (! out)
	return NULL;

    if (! DXAddArrayData(out, 0, nOut, NULL))
    {
	DXDelete((Object)out);
	return NULL;
    }

    dataIn = (DXEvents)DXGetArrayData(in);
    dataOut = (int *)DXGetArrayData(out);
    for (i = 0; i < nIn; i++)
    {
	int b = WHICH_BUTTON(&dataIn[i]);
	if (b != NO_BUTTON && dataIn[i].mouse.state == BUTTON_DOWN) 
	{
	    dataOut[0] = dataIn[i].mouse.x;
	    dataOut[1] = dataIn[i].mouse.y;
	    dataOut += 2;
	}
    }
    
    return out;
}

