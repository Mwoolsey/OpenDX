/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "fle2DClass.h"

#define FALSE 0
#define TRUE 1

static Error _dxfInitializeTask(Pointer);
static Error _dxfInitialize(Fle2DInterpolator);
static int   _dxfCleanup(Fle2DInterpolator);

static int CountIntersections(Fle2DInterpolator, float *, int);
static int InsideFace(Fle2DInterpolator, float *, int);
static int FindFace(Fle2DInterpolator, float *);

int
_dxfRecognizeFLE2D(Field field)
{
    Array    array;
    Type     t;
    Category c;
    int      r, s[32];
    Object   depAttr;

    CHECK(field, CLASS_FIELD);

    array = (Array)DXGetComponentValue(field, "faces");
    if (! array)
	return 0;

    DXGetArrayInfo(array, NULL, &t, &c, &r, s);

    if (t != TYPE_INT || c != CATEGORY_REAL ||
			!(r == 0 || (r == 1 && s[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, "invalid faces component");
	return 0;
    }

    array = (Array)DXGetComponentValue(field, "loops");
    if (! array)
	return 0;

    DXGetArrayInfo(array, NULL, &t, &c, &r, s);

    if (t != TYPE_INT || c != CATEGORY_REAL ||
			!(r == 0 || (r == 1 && s[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, "invalid loops component");
	return 0;
    }

    array = (Array)DXGetComponentValue(field, "edges");
    if (! array)
	return 0;

    DXGetArrayInfo(array, NULL, &t, &c, &r, s);

    if (t != TYPE_INT || c != CATEGORY_REAL ||
			!(r == 0 || (r == 1 && s[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, "invalid edges component");
	return 0;
    }

    array = (Array)DXGetComponentValue(field, "positions");
    if (!array)
    {
	DXSetError(ERROR_MISSING_DATA, "#10240", "positions");
	return 0;
    }

    DXGetArrayInfo(array, NULL, &t, &c, &r, s);
	
    if (t != TYPE_FLOAT || c != CATEGORY_REAL || r != 1 || s[0] != 2)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"fle positions must be float and 2-D");
	return 0;
    }

    depAttr = DXGetComponentAttribute(field, "data", "dep");
    if (! depAttr)
    {
	DXSetError(ERROR_DATA_INVALID, "missing data dependency");
	return 0;
    }

    if (DXGetObjectClass(depAttr) != CLASS_STRING)
    {
	DXSetError(ERROR_DATA_INVALID, "invalid data dependency attribute");
	return 0;
    }

    if (strcmp(DXGetString((String)depAttr), "faces"))
    {
	DXSetError(ERROR_DATA_INVALID, 
		"invalid data dependence: fle data must be dep faces");
	return 0;
    }
    
    return 1;
}

Fle2DInterpolator
_dxfNewFle2DInterpolator(Field field,
		enum interp_init initType, double fuzz, Matrix *m) 
{
    return (Fle2DInterpolator)_dxf_NewFle2DInterpolator(field, 
			    initType, fuzz, m, &_dxdfle2dinterpolator_class);
}

Fle2DInterpolator
_dxf_NewFle2DInterpolator(Field field,
		enum interp_init initType, float fuzz, Matrix *m,
		struct fle2dinterpolator_class *class)
{
    Fle2DInterpolator 	fle;
    float	        *mm, *MM;

    fle = (Fle2DInterpolator)_dxf_NewFieldInterpolator(field, fuzz, m,
	    (struct fieldinterpolator_class *)class);

    if (! fle)
	return NULL;

    fle->fArray = NULL;
    fle->lArray = NULL;
    fle->eArray = NULL;
    fle->pArray = NULL;
    fle->dArray = NULL;
    fle->mmfArray = NULL;
    fle->mmlArray = NULL;

    mm = ((Interpolator)fle)->min;
    MM = ((Interpolator)fle)->max;

    if (((MM[0] - mm[0]) * (MM[1] - mm[1])) == 0.0)
    {
	DXDelete((Object)fle);
	return NULL;
    }

    fle->hint = -1;

    if (initType == INTERP_INIT_PARALLEL)
    {
	if (! DXAddTask(_dxfInitializeTask, (Pointer)&fle, sizeof(fle), 1.0))
	{
	    DXDelete((Object)fle);
	    return NULL;
	}

    }
    else if (initType == INTERP_INIT_IMMEDIATE)
    {
	if (! _dxfInitialize(fle))
	{
	    DXDelete((Object)fle);
	    return NULL;
	}
    }

    return fle;
}

static Error
_dxfInitializeTask(Pointer p)
{
    return _dxfInitialize(*(Fle2DInterpolator *)p);
}

static Error
_dxfInitialize(Fle2DInterpolator fle)
{
    Field	field;
    int		i, j, k;
    float	lx, ly, lX, lY;
    float	fx, fy, fX, fY;
    MinMax	*lmm, *fmm;

    fle->fieldInterpolator.initialized = 1;

    field = (Field)((Interpolator)fle)->dataObject;

    fle->fArray = (Array)DXGetComponentValue(field, "faces");
    fle->lArray = (Array)DXGetComponentValue(field, "loops");
    fle->eArray = (Array)DXGetComponentValue(field, "edges");
    fle->pArray = (Array)DXGetComponentValue(field, "positions");
    fle->dArray = (Array)DXGetComponentValue(field, "data");

    DXReference((Object)fle->fArray);
    DXReference((Object)fle->lArray);
    DXReference((Object)fle->eArray);
    DXReference((Object)fle->pArray);
    DXReference((Object)fle->dArray);

    fle->faces     = (int   *)DXGetArrayData(fle->fArray);
    fle->loops     = (int   *)DXGetArrayData(fle->lArray);
    fle->edges     = (int   *)DXGetArrayData(fle->eArray);
    fle->positions = (float *)DXGetArrayData(fle->pArray);

    DXGetArrayInfo(fle->fArray, &fle->nFaces, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(fle->lArray, &fle->nLoops, NULL, NULL, NULL, NULL);
    DXGetArrayInfo(fle->eArray, &fle->nEdges, NULL, NULL, NULL, NULL);

    fle->mmfArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 4);
    fle->mmlArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 4);
    if (! fle->mmfArray || ! fle->mmlArray)
	goto error;

    DXReference((Object)fle->mmfArray);
    DXReference((Object)fle->mmlArray);

    if (! DXAddArrayData(fle->mmfArray, 0, fle->nFaces, NULL))
	goto error;

    if (! DXAddArrayData(fle->mmlArray, 0, fle->nLoops, NULL))
	goto error;

    fmm = fle->minmax_faces = (MinMax *)DXGetArrayData(fle->mmfArray);
    if (! fmm)
	goto error;

    lmm = fle->minmax_loops = (MinMax *)DXGetArrayData(fle->mmlArray);
    if (! lmm)
	goto error;

    for (i = 0; i < fle->nFaces; i++, fmm++)
    {
	int sl = fle->faces[i];
	int el = (i == fle->nFaces-1) ? fle->nLoops : fle->faces[i+1];

	fx = fy =  DXD_MAX_FLOAT;
	fX = fY = -DXD_MAX_FLOAT;

	for (j = sl; j < el; j++, lmm++)
	{
	    int se = fle->loops[j];
	    int ee = (j == fle->nLoops-1) ? fle->nEdges : fle->loops[j+1];

	    lx = ly =  DXD_MAX_FLOAT;
	    lX = lY = -DXD_MAX_FLOAT;
	
	    for (k = se; k < ee; k++)
	    {
		float *xy = fle->positions+(2*fle->edges[k]);

		if (*xy < lx) lx = *xy;
		if (*xy > lX) lX = *xy;
		xy++;
		if (*xy < ly) ly = *xy;
		if (*xy > lY) lY = *xy;
	    }

            if (lx < fx) fx = lx;
	    if (lX > fX) fX = lX;
	    if (ly < fy) fy = ly;
	    if (lY > fY) fY = lY;

	    lmm->x = lx;
	    lmm->y = ly;
	    lmm->X = lX;
	    lmm->Y = lY;
	}

	fmm->x = fx;
	fmm->y = fy;
	fmm->X = fX;
	fmm->Y = fY;
    }

    DXGetArrayInfo(fle->dArray, NULL, &((Interpolator)fle)->type,
		&((Interpolator)fle)->category,
		&((Interpolator)fle)->rank, ((Interpolator)fle)->shape);

    fle->dHandle = DXCreateArrayHandle(fle->dArray);
    if (! fle->dHandle)
	return ERROR;


    return OK;

error:

    DXDelete((Pointer)fle->mmfArray);
    fle->mmfArray = NULL;

    DXDelete((Pointer)fle->mmlArray);
    fle->mmlArray = NULL;

    return ERROR;
}

Error
_dxfFle2DInterpolator_Delete(Fle2DInterpolator fle)
{
    _dxfCleanup(fle);
    return _dxfFieldInterpolator_Delete((FieldInterpolator) fle);
}

static int
CountIntersections(Fle2DInterpolator fle, float *xy, int loop)
{
    int   se = fle->loops[loop];
    int   le = (loop == fle->nLoops-1) ? fle->nEdges : fle->loops[loop+1];
    int   e, p, q, knt;
    float t, x, *ppos, *qpos, *top, *bottom;

    p    = fle->edges[se];
    ppos = fle->positions + (p<<1);
    for (e = se, knt = 0; e < le; e++, p = q, ppos = qpos)
    {
	if (e == le-1)
	    q = fle->edges[se];
	else
	    q = fle->edges[e+1];
    
	qpos = fle->positions + (q<<1);

	if (qpos[1] > ppos[1])
	{
	    top = qpos; bottom = ppos;
	}
	else if (qpos[1] < ppos[1])
	{
	    top = ppos; bottom = qpos;
	}
	else
	    continue;
	
	if (top[1] < xy[1])
	    continue;
	
	if (bottom[1] >= xy[1])
	    continue;

	if (top[0] < xy[0] && bottom[0] < xy[0])
	    continue;
	
	if (top[0] > xy[0] && bottom[0] > xy[0])
	{
	    knt ++;
	    continue;
	}
	
	t = (xy[1] - top[1]) / (bottom[1] - top[1]);
	x = top[0] + t*(bottom[0] - top[0]);

	if (x >= xy[0])
	{
	    knt ++;
	    continue;
	}
    }

    return knt;
}

static int 
InsideFace(Fle2DInterpolator fle, float *xy, int face)
{
    int    sl = fle->faces[face];
    int    el = (face == fle->nFaces-1) ? fle->nLoops : fle->faces[face+1];
    int    knt;
    int    l;
    MinMax *mm = fle->minmax_loops + sl;

    for (l = sl, knt = 0; l < el; l++, mm++)
    {
	if (mm->x < xy[0] && mm->X > xy[0] &&
	    mm->y < xy[1] && mm->Y > xy[1])
	    knt += CountIntersections(fle, xy, l);
    }
	
    if (knt & 0x1)
	return face;

    return -1;
}


static int 
FindFace(Fle2DInterpolator fle, float *xy)
{
    int i;

    if (fle->hint >= 0)
    {
	MinMax *mm = fle->minmax_faces + fle->hint;

	if (mm->x < xy[0] && mm->X > xy[0] &&
			    mm->y < xy[1] && mm->Y > xy[1])
	{
	    if (-1 != InsideFace(fle, xy, fle->hint))
		return fle->hint;
	}
    }

    for (i = 0; i < fle->nFaces; i++)
    {
	MinMax *mm = fle->minmax_faces + i;

	if (mm->x < xy[0] && mm->X > xy[0] &&
			    mm->y < xy[1] && mm->Y > xy[1])
	{
	    if (-1 != InsideFace(fle, xy, i))
	    {
		fle->hint = i;
		return i;
	    }
	}
    }

    fle->hint = -1;
    return -1;
}

int
_dxfFle2DInterpolator_PrimitiveInterpolate(Fle2DInterpolator fle,
		    int *n, float **points, Pointer *values, int fuzzFlag)
{
    int found;
    Pointer v;
    float *p;
    int itemSize;
    Matrix *xform;
    char *dbuf = NULL;

    if (! fle->fieldInterpolator.initialized)
    {
	if (! _dxfInitialize(fle))
	{
	    _dxfCleanup(fle);
	    return ERROR;
	}

	fle->fieldInterpolator.initialized = 1;
    }

    itemSize = DXGetItemSize(fle->dArray);

    dbuf = (char *)DXAllocate(itemSize);
    if (! dbuf)
	return ERROR;
    
    if (((FieldInterpolator)fle)->xflag)
	xform = &(((FieldInterpolator)fle)->xform);
    else
	xform = NULL;

    v = *values;
    p = (float *)*points;

    /*
     * For each point in the input, attempt to interpolate the point.
     * When a point cannot be interpolated, quit.
     */
    while(*n != 0)
    {
	float xpt[2];
	float *pPtr;

	if (xform)
	{
	    xpt[0] = p[0]*xform->A[0][0] +
		     p[1]*xform->A[1][0] +
		          xform->b[0];

	    xpt[1] = p[0]*xform->A[0][1] +
		     p[1]*xform->A[1][1] +
		          xform->b[1];
	    pPtr = xpt;
	}
	else
	    pPtr = p;


	found = FindFace(fle, pPtr);

	if (found == -1)
	    break;
	
	memcpy(v, DXGetArrayEntry(fle->dHandle, found, dbuf), itemSize);

	v = (Pointer)(((unsigned char *)v) + itemSize);

	/*
	 * Only use fuzz on first point
	 */
	fuzzFlag = FUZZ_OFF;

	p += 2;
	*n -= 1;
    }

    DXFree((Pointer)dbuf);

    *values = v;
    *points = (float *)p;

    return OK;
/*
error:
    DXFree((Pointer)dbuf);
    return ERROR;
*/
}

static int
_dxfCleanup(Fle2DInterpolator fle)
{								
    if (fle->dHandle)
    {
	DXFreeArrayHandle(fle->dHandle);
	fle->dHandle = NULL;
    }

    DXDelete((Object)fle->fArray);
    DXDelete((Object)fle->lArray);
    DXDelete((Object)fle->eArray);
    DXDelete((Object)fle->pArray);
    DXDelete((Object)fle->dArray);
    DXDelete((Object)fle->mmfArray);
    DXDelete((Object)fle->mmlArray);

    fle->fArray   = NULL;
    fle->lArray   = NULL;
    fle->eArray   = NULL;
    fle->pArray   = NULL;
    fle->dArray   = NULL;
    fle->mmfArray = NULL;
    fle->mmlArray = NULL;

    return OK;
}

Object
_dxfFle2DInterpolator_Copy(Fle2DInterpolator old, enum _dxd_copy copy)
{
    Fle2DInterpolator new;

    new = (Fle2DInterpolator)
	    _dxf_NewObject((struct object_class *)&_dxdfle2dinterpolator_class);

    if (!(_dxf_CopyFle2DInterpolator(new, old, copy)))
    {
	DXDelete((Object)new);
	return NULL;
    }
    else
	return (Object)new;
}

Fle2DInterpolator
_dxf_CopyFle2DInterpolator(Fle2DInterpolator new, 
				Fle2DInterpolator old, enum _dxd_copy copy)
{

    if (! _dxf_CopyFieldInterpolator((FieldInterpolator)new,
					(FieldInterpolator)old, copy))
	return NULL;
    
    new->nEdges    = old->nEdges;
    new->nFaces    = old->nFaces;
    new->nLoops    = old->nLoops;
    new->hint      = old->hint;

    if (new->fieldInterpolator.initialized)
    {
	new->fArray    = (Array)DXReference((Object)old->fArray);
	new->lArray    = (Array)DXReference((Object)old->lArray);
	new->eArray    = (Array)DXReference((Object)old->eArray);
	new->pArray    = (Array)DXReference((Object)old->pArray);
	new->dArray    = (Array)DXReference((Object)old->dArray);
	new->mmfArray  = (Array)DXReference((Object)old->mmfArray);
	new->mmlArray  = (Array)DXReference((Object)old->mmlArray);

	if (new->fieldInterpolator.localized)
	{
	    new->faces        = (int    *)DXGetArrayDataLocal(new->fArray);
	    new->loops        = (int    *)DXGetArrayDataLocal(new->lArray);
	    new->edges        = (int    *)DXGetArrayDataLocal(new->eArray);
	    new->positions    = (float  *)DXGetArrayDataLocal(new->pArray);
	    new->minmax_faces = (MinMax *)DXGetArrayDataLocal(new->mmfArray);
	    new->minmax_loops = (MinMax *)DXGetArrayDataLocal(new->mmlArray);
	}
	else
	{
	    new->faces        = (int    *)DXGetArrayData(new->fArray);
	    new->loops        = (int    *)DXGetArrayData(new->lArray);
	    new->edges        = (int    *)DXGetArrayData(new->eArray);
	    new->positions    = (float  *)DXGetArrayData(new->pArray);
	    new->minmax_faces = (MinMax *)DXGetArrayData(new->mmfArray);
	    new->minmax_loops = (MinMax *)DXGetArrayData(new->mmlArray);
	}

	new->dHandle = DXCreateArrayHandle(new->dArray);
	if (! new->dHandle)
	    return NULL;
    }
    else
    {
	new->fArray    = NULL;
	new->lArray    = NULL;
	new->eArray    = NULL;
	new->pArray    = NULL;
	new->dHandle   = NULL;
    }

    if (DXGetError())
	return NULL;

    return new;
}

Interpolator
_dxfFle2DInterpolator_LocalizeInterpolator(Fle2DInterpolator fle)
{
    if (fle->fieldInterpolator.localized)
	return (Interpolator)fle;

    fle->fieldInterpolator.localized = 1;

    if (fle->fieldInterpolator.initialized)
    {
        fle->faces        = (int    *)DXGetArrayDataLocal(fle->fArray);
        fle->loops        = (int    *)DXGetArrayDataLocal(fle->lArray);
        fle->edges        = (int    *)DXGetArrayDataLocal(fle->eArray);
        fle->positions    = (float  *)DXGetArrayDataLocal(fle->pArray);
        fle->minmax_faces = (MinMax *)DXGetArrayDataLocal(fle->mmfArray);
        fle->minmax_faces = (MinMax *)DXGetArrayDataLocal(fle->mmlArray);
    }

    if (DXGetError())
        return NULL;
    else
        return (Interpolator)fle;
}
