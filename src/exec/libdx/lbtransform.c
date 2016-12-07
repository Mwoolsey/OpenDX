/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <math.h>
#include "cameraClass.h"
#include "internals.h"
#include "render.h"

static void doboxpts(Point *pts, int ortho,
		     float minx, float miny, float minz,
		     float max, float maxy, float maxz,
		     int *behind);
Array
_dxf_TransformArray(Matrix *, Camera, Array, Array *, Field, int *, float);


Array
_dxf_TransformBox(Matrix *, Camera, Array, int *, float);

/*
 * DXTransform a regular array.  This consists of applying the full
 * transformation to the origin and all but the translation part to the
 * deltas: if the transform is xA + b$ and each point is of the form o
 * + id$, then each transformed point is of the form (oA+b)+(idA).
 * This is only applicable if there is no perspective transformation
 * involved.
 */

static Array
TransformRegularArray(Matrix m, Camera c, Array points, Array *box)
{
    int n;
    Point origin, delta;
    Array xpoints;

    /* is regular array transformation applicable? */
    if ((c && !c->ortho) || DXGetArrayClass(points)!=CLASS_REGULARARRAY)
	return NULL;

    /* better to transform existing box in regular case */
    if (box)
	*box = NULL;

    /* type check */
    if (!DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 3) &&
        !DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 2) &&
        !DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 1))
	DXErrorReturn(ERROR_BAD_TYPE,
	    "positions data must be 1, 2, or 3-D float vectors");

    /* set z to 0 in case this is 2-d or 1-d */
    origin = delta = DXPt(0,0,0);

    /* get regular array info */
    if (!DXGetRegularArrayInfo((RegularArray)points, &n,
			     (Pointer)&origin, (Pointer)&delta))
	return NULL;

    /* apply full transformation to the origin */
    origin = DXApply(origin, m);

    /* apply transformation less translation part to the delta */
    m.b[0] = m.b[1] = m.b[2] = 0;
    delta = DXApply(delta, m);

    /* construct the regular array result */
    xpoints = (Array) DXNewRegularArray(TYPE_FLOAT, 3, n,
				      (Pointer)&origin, (Pointer)&delta);

    return xpoints;
}


/*
 * DXTransform a product geometry array.  This consists of applying the
 * full transformation to the first term and all but the translation part
 * to the remaining terms: if the transformation is xA+b$ and each point
 * is of the form t_0+t_1+t_2+\cdots$, where each t_i$ is a point from
 * a different term, then the transformed points are of the form
 * (t_0A+b)+(t_1A)+(t_2A)+\cdots$.  This is only applicable if there is
 * no perspective transformation involved.
 *
 * Note the hack involving FUDGE: for regular partitioned grids that
 * exactly coincide with the pixel grid, this method of computing the
 * transform can introduce roundoff errors causing origin+n*deltax
 * of one partition to overlap the origin of the next partition, causing
 * pixels to appear to be in more than one partition, resulting in
 * volume rendering artifacts.  We "solve" this by moving the grid a
 * tiny bit, thus avoiding exact pixel alignment of the grid.  Note that
 * all this does is make the artifact much less likely; it doesn't
 * eliminate this possibility.
 */

#define FUDGE (1.0/93.0)

static Array
TransformProductArray(Matrix m, Camera c, Array points, Array *box,
		      Field f, int *behind)
{
    int i, n;
    Array terms[100];

    /* is product array transformation applicable? */
    if ((c && !c->ortho) || DXGetArrayClass(points)!=CLASS_PRODUCTARRAY)
	return NULL;

    /* type check */
    if (!DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 3) &&
        !DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 2) &&
        !DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 1))
	DXErrorReturn(ERROR_BAD_TYPE,
	    "positions data must be 1, 2, or 3-D float vectors");
    /* get terms */
    if (!DXGetProductArrayInfo((ProductArray)points, &n, terms))
	return NULL;
    ASSERT(n<100);

    /* apply whole transformation to first term */
    if (c) {				/* fudge only final camera transform */
	m.b[0] += FUDGE;		/* fudge the x - see comment above */
	m.b[1] += FUDGE;		/* fudge the y - see comment above */
    }
    terms[0] = _dxf_TransformArray(&m, NULL, terms[0], NULL, f, NULL, 0.0);
    if (!terms[0])
	return NULL;

    /* transform the box with the fudged matrix */
    if (box) {
	Array b = (Array) DXGetComponentValue(f, BOX);
	*box = _dxf_TransformBox(&m, NULL, b, behind, 0.0);
	if (!*box)
	    return NULL;
    }

    /* now zero the translation and apply rotation part to rest of terms */
    m.b[0] = m.b[1] = m.b[2] = 0;
    for (i=1; i<n; i++) {
	terms[i] = _dxf_TransformArray(&m, NULL, terms[i], NULL, f, NULL, 0.0);
	if (!terms[i])
	    return NULL;
    }

    /* construct product array result */
    return (Array) DXNewProductArrayV(n, terms);
}


/*
 * Transforms the input array of points or normals by transformation
 * {\tt t} followed by the camera transformation specified by {\tt c}.
 * The transformation is a point or normal transformation according to the
 * setting of the {\tt normals} flag.
 * XXX - vectorize this
 * XXX - handle different data types
*/

#define XFORM {							\
    ox = mp->A[0][0]*ix+mp->A[1][0]*iy+mp->A[2][0]*iz+mp->b[0];	\
    oy = mp->A[0][1]*ix+mp->A[1][1]*iy+mp->A[2][1]*iz+mp->b[1];	\
    oz = mp->A[0][2]*ix+mp->A[1][2]*iy+mp->A[2][2]*iz+mp->b[2];	\
    out[i].x = ox, out[i].y = oy, out[i].z = oz;		\
}

#define BBOX {							\
    if (ox > maxx) maxx = ox; if (ox < minx) minx = ox;		\
    if (oy > maxy) maxy = oy; if (oy < miny) miny = oy;		\
}

#define PBOX {							\
    if (ox > maxx) maxx = ox; if (ox < minx) minx = ox;		\
    if (oy > maxy) maxy = oy; if (oy < miny) miny = oy;		\
    if (oz > maxz) maxz = oz; if (oz < minz) minz = oz;		\
}

static Array
TransformIrregular(Matrix *mp, Camera c, Array points, Point *box, int *behind)
{
    int i, n, rank, shape[100], dim;
    Array xpoints;
    Point *in, *out;
    float ix, iy, iz, ox, oy, oz, 
        minx=DXD_MAX_FLOAT, miny=DXD_MAX_FLOAT, minz=DXD_MAX_FLOAT, 
        maxx=-DXD_MAX_FLOAT, maxy=-DXD_MAX_FLOAT, maxz=-DXD_MAX_FLOAT;

    /* get input data to local memory */
    in = (Point *) DXGetArrayData(points);
    if (!DXGetArrayInfo(points, &n, NULL, NULL, &rank, shape))
	DXErrorReturn(ERROR_INTERNAL, "DXGetArrayInfo failed!");
    if (rank!=1) {
	DXSetError(ERROR_BAD_PARAMETER,
	    "positions data must be 1, 2, or 3-D float vectors (not scalar)");
	return NULL;
    }
    dim = shape[0];

    /* create output array, allocate storage for it */
    xpoints = DXAllocateArray(DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3), n);
    out = (Point *) DXGetArrayData(DXAddArrayData(xpoints, 0, n, NULL));
    if (!out)
	return NULL;

    if (box) {
	minx = miny = minz = DXD_MAX_FLOAT;
	maxx = maxy = maxz = -DXD_MAX_FLOAT;
    }

    /* transform points */
    if (dim==3) {
	if (box) {
	    if (!c || c->ortho) {
		for (i=0; i<n; i++) {
		    ix=in[i].x, iy=in[i].y, iz=in[i].z;
		    XFORM;
		    BBOX;
		}
	    } else {
		for (i=0; i<n; i++) {
		    ix=in[i].x, iy=in[i].y, iz=in[i].z;
		    XFORM;
		    PBOX;
		}
	    }
	} else {
	    for (i=0; i<n; i++) {
		ix=in[i].x, iy=in[i].y, iz=in[i].z;
		XFORM;
	    }
	}
    } else if (dim==2) {
	struct in2 {float x, y;} *in2 = (struct in2 *)in;
	for (i=0; i<n; i++) {
	    ix=in2[i].x, iy=in2[i].y, iz=0;
	    XFORM;
	    if (box) {
		PBOX;
	    }
	}
    } else if (dim==1) {
	struct in1 {float x;} *in1 = (struct in1 *)in;
	for (i=0; i<n; i++) {
	    ix=in1[i].x, iy=0, iz=0;
	    XFORM;
	    if (box) {
		PBOX;
	    }
	}
    } else {
	DXSetError(ERROR_BAD_PARAMETER,
		 "array is %d-d, expecting 1-d, 2-d, or 3-d", dim);
	return NULL;
    }

    /* expand to eight points */
    if (box)
	doboxpts(box, !c || c->ortho,
		 minx, miny, minz, maxx, maxy, maxz, behind);

    return xpoints;
}

Array
_dxf_TransformNormals(Matrix *mp, Array points)
{
    int i, n, dim, bad = 0;
    Array xpoints;
    Point *in, *out;
    Matrix m;
    Point cstIn, cstOut;

    /* normals unchanged? */
    if (!mp)
	return points;

    /* type check */
    if (!DXTypeCheck(points, TYPE_FLOAT, CATEGORY_REAL, 1, 3))
	DXErrorReturn(ERROR_BAD_PARAMETER, "normals not 3-d");

    /* matrix is different for normals */
    m = DXAdjointTranspose(*mp);
    
    if (DXQueryConstantArray(points, &n, &cstIn))
    {
	float x, y, z, d;
	x=m.A[0][0]*cstIn.x+m.A[1][0]*cstIn.y+m.A[2][0]*cstIn.z+m.b[0];
	y=m.A[0][1]*cstIn.x+m.A[1][1]*cstIn.y+m.A[2][1]*cstIn.z+m.b[1];
	z=m.A[0][2]*cstIn.x+m.A[1][2]*cstIn.y+m.A[2][2]*cstIn.z+m.b[2];
	d = sqrt(x*x+y*y+z*z);
	if (d) d = (float)1.0 / d;
	else bad = n;
	cstOut.x = x*d;
	cstOut.y = y*d;
	cstOut.z = z*d;

	xpoints = (Array)DXNewConstantArray(n, (Pointer)&cstOut,
				TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (! xpoints)
	    return NULL;
    }
    else 
    {
	/* get input data */
	in = (Point *) DXGetArrayData(points);
	if (!DXGetArrayInfo(points, &n, NULL, NULL, NULL, &dim))
	    DXErrorReturn(ERROR_INTERNAL, "DXGetArrayInfo failed!");

	/* create output array, allocate storage for it */
	xpoints = DXAllocateArray(DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3), n);
	out = (Point *) DXGetArrayData(DXAddArrayData(xpoints, 0, n, NULL));
	if (!out)
	    return NULL;


	/* transform points */
	for (i=0; i<n; i++) {
	    float x, y, z, d;
	    x=m.A[0][0]*in[i].x+m.A[1][0]*in[i].y+m.A[2][0]*in[i].z+m.b[0];
	    y=m.A[0][1]*in[i].x+m.A[1][1]*in[i].y+m.A[2][1]*in[i].z+m.b[1];
	    z=m.A[0][2]*in[i].x+m.A[1][2]*in[i].y+m.A[2][2]*in[i].z+m.b[2];
	    d = sqrt(x*x+y*y+z*z);
	    if (d) d = (float)1.0 / d;
	    else bad++;
	    out[i].x = x*d;
	    out[i].y = y*d;
	    out[i].z = z*d;
	}
    }

    if (bad)
	DXWarning("%d zero-length %s detected",
		bad, bad>1? "normals were" : "normal was");

    return xpoints;
}

/*
 * Transforms the input array of points or normals by transformation
 * {\tt t} followed by the camera transformation specified by {\tt c}.
 * The transformation is a point or normal transformation according to the
 * setting of the {\tt normals} flag.
 * Returns a pointer to a new array holding the transformed points or null to
 * indicate an error.
 */

Array
_dxf_TransformArray(Matrix *mp, Camera c, Array points,
		Array *box, Field f, int *behind, float fuzz)
{
    Array xpoints;
    Matrix m;
    Point *boxpts;

    /*
     * if both mp and c are null, the transform requested
     * is identity, so return without doing anything.
     * Because of this, you can call DXTransform(o, NULL, NULL)
     * to make sure everything is in world coordinates as often as
     * you want without penalty.
     */
    if (!mp && !c && DXTypeCheck(points,TYPE_FLOAT,CATEGORY_REAL,1,3))
	return points;

    /* get the transform */
    if (mp)
	m = DXConcatenate(*mp, DXGetCameraMatrixWithFuzz(c, fuzz));
    else
	m = DXGetCameraMatrixWithFuzz(c, fuzz);

    /* regular array? */
    if ((xpoints=TransformRegularArray(m, c, points, box))!=NULL)
	return xpoints;

    /* product array? */
    if ((xpoints=TransformProductArray(m, c, points, box, f, behind))!=NULL)
	return xpoints;

    /* irregular array */
    else {
	if (box) {
	    *box = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	    if (!*box) return NULL;
	    boxpts = (Point *) DXGetArrayData(DXAddArrayData(*box, 0, 8, NULL));
	    if (!boxpts) return NULL;
	} else
	    boxpts = NULL;
	return TransformIrregular(&m, c, points, boxpts, behind);
    }
}

/*
 * DXTransform a box, rectifying to the new coordinate system,
 * and expanding to 8 points if necessary
 */


Array
_dxf_TransformBox(Matrix *mp, Camera c, Array points, int *behind, float fuzz)
{
    float minx, miny, minz, maxx, maxy, maxz;
    Point *pts;
    Array box;
    int i, n;

    /* transform */
    box = _dxf_TransformArray(mp, c, points, NULL, NULL, NULL, fuzz);
    if (!box) return NULL;
    DXGetArrayInfo(box, &n, NULL, NULL, NULL, NULL);
    pts = (Point *) DXGetArrayData(box);
    if (!pts) return NULL;

    /* compute min/max */
    minx = miny = minz = DXD_MAX_FLOAT;
    maxx = maxy = maxz = -DXD_MAX_FLOAT;
    for (i=0; i<n; i++) {
	if (pts[i].x < minx) minx = pts[i].x;
	if (pts[i].y < miny) miny = pts[i].y;
	if (pts[i].z < minz) minz = pts[i].z;
	if (pts[i].x > maxx) maxx = pts[i].x;
	if (pts[i].y > maxy) maxy = pts[i].y;
	if (pts[i].z > maxz) maxz = pts[i].z;
    }

    /* expand to 8 points if necessary */
    if (n!=8) {
	Array a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!DXAddArrayData(a, 0, 8, NULL)) return NULL;
	DXDelete((Object)box);
	box = a;
    }	

    /* rectify */
    pts = (Point *) DXGetArrayData(box);
    if (!pts) return NULL;
    doboxpts(pts, !c || c->ortho, minx, miny, minz, maxx, maxy, maxz, behind);

    return box;
}


/*
 * While the perspective divide for positions is deferred until
 * the object is tiled to allow clipping, the perspective
 * divide for the bounding box is done during the transformation
 * stage.  This routine computes the screen-aligned bounding box,
 * taking into account perspective.
 *
 * The result that we leave in the "bounding box" is rather strange,
 * in that 1) the min/max in x have had the perspective divide done
 * while the min/max in z have not, and 2) we clip the bounding box
 * at the nearPlane plane to be only the portion BEHIND the viewer; this
 * means box[0].z is near, and box[7].z can be used to determine whether
 * a clipping pass is needed.
 *
 * MAX_XY is the maximum x,y that we will allow (by choice of near)
 * after the perspective divide.  This must be large enough to
 * allow nearby triangles that are partly in front of and partly
 * behind the camera to remain on screen as long as possible, but
 * not so large as to allow overflow (in float->int conversion) or
 * insufficient precision in the triangle routines.
 *
 * We also limit near by SMALL, because even if x,y are uniformly
 * 0, we can't allow 1/near to overflow.
 */

#define MAX_XY 100000.0
#define SMALL 1e-35

static void
doboxpts(Point *pts, int ortho,
	 float minx, float miny, float minz,
	 float maxx, float maxy, float maxz,
	 int *behind
) {
    float nearPlane;

    if (!ortho) {

	float xy, max = 0;
	float mxz, mnz;

	/* extreme x,y */
	xy = minx;  if (xy<0) xy = -xy;  if (xy>max) max = xy;
	xy = maxx;  if (xy<0) xy = -xy;  if (xy>max) max = xy;
	xy = miny;  if (xy<0) xy = -xy;  if (xy>max) max = xy;
	xy = maxy;  if (xy<0) xy = -xy;  if (xy>max) max = xy;

	/* compute nearPlane to avoid overflow and loss of precision */
	nearPlane = max / MAX_XY;
	if (nearPlane<SMALL) nearPlane = SMALL;
	nearPlane = -nearPlane;

	/* x,y bounds after perspective divide */
	mxz = maxz>nearPlane? nearPlane : maxz;
	mnz = minz>nearPlane? nearPlane : minz;
	minx /= (minx<0? -mxz : -mnz);
	maxx /= (maxx>0? -mxz : -mnz);
	miny /= (miny<0? -mxz : -mnz);
	maxy /= (maxy>0? -mxz : -mnz);

	/* is object behind camera? */
	if (behind && minz>nearPlane)
	    *behind = 1;

    } else
	nearPlane = 0;

    /* generate the bounding box */
    if (maxz<nearPlane) maxz = nearPlane;
    pts[0].x = minx,  pts[0].y = miny,  pts[0].z = nearPlane;
    pts[1].x = minx,  pts[1].y = miny,  pts[1].z = maxz;
    pts[2].x = minx,  pts[2].y = maxy,  pts[2].z = nearPlane;
    pts[3].x = minx,  pts[3].y = maxy,  pts[3].z = maxz;
    pts[4].x = maxx,  pts[4].y = miny,  pts[4].z = nearPlane;
    pts[5].x = maxx,  pts[5].y = miny,  pts[5].z = maxz;
    pts[6].x = maxx,  pts[6].y = maxy,  pts[6].z = nearPlane;
    pts[7].x = maxx,  pts[7].y = maxy,  pts[7].z = maxz;

}

static Array ApplyTransform_GenericArray(Array, Matrix *, int);
static Array ApplyTransform_RegularArray(Array, Matrix *, int);
static Array ApplyTransform_ProductArray(Array, Matrix *, int);
static Array ApplyTransform_ConstantArray(Array, Matrix *, int);

static Field ApplyTransform_Field(Field, Matrix *);
static Array ApplyTransform_Array(Array, Matrix *, int);

static Object AT_traverse(Object, Matrix *);
static Error  AT_field_task(Pointer);

static Array SetDimensionality(Array, int);

Object
DXApplyTransform(Object object, Matrix *stack)
{
    if (! DXCreateTaskGroup())
	goto error;
    
    object = AT_traverse(object, stack);
    if (! object)
    {
	DXAbortTaskGroup();
	goto error;
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    return object;

error:
    return NULL;
}

static Object
AT_traverse(Object object, Matrix *stack)
{
    switch (DXGetObjectClass(object))
    {
	case CLASS_ARRAY:
	{
	    Object attr = DXGetAttribute(object, "geometric");
	    if (attr)
	    {
		if (! strcmp(DXGetString((String)attr), "vector"))
		{
		    Matrix ax;
		    ax = DXAdjointTranspose(*stack);
		    return (Object)ApplyTransform_Array((Array)object, &ax, 0);
		}
		else
		    return (Object)ApplyTransform_Array((Array)object,stack,0);
	    }
	    else
		return object;
	}

	case CLASS_FIELD:
	    return (Object)ApplyTransform_Field((Field)object, stack);
	
	case CLASS_XFORM:
	{
	    Matrix newmatrix, product;
	    Object child;

	    if (! DXGetXformInfo((Xform)object, &child, &newmatrix))
		return NULL;
	    
	    if (stack)
	    {
		product = DXConcatenate(newmatrix, *stack);
		return AT_traverse(child, &product);
	    }
	    else
		return AT_traverse(child, &newmatrix);
	}

	case CLASS_CLIPPED:
	{
	    Object clipper, clipped;

	    if (! DXGetClippedInfo((Clipped)object, &clipped, &clipper))
		return NULL;
	    
	    if (clipped && NULL == (clipped = AT_traverse(clipped, stack)))
		return NULL;

	    if (clipper && NULL == (clipper = AT_traverse(clipper, stack)))
	    {
		DXDelete(clipped);
		return NULL;
	    }

	    return (Object)DXNewClipped(clipped, clipper);
	}

	case CLASS_GROUP:
	{
	    int i;
	    Object child;
	    Group  result;

	    result = (Group)DXCopy(object, COPY_HEADER);

	    i = 0;
	    while (NULL !=
		(child = DXGetEnumeratedMember((Group)object, i, NULL)))
	    {
		child = AT_traverse(child, stack);
		if (! child)
		{
		    DXDelete((Object)result);
		    return NULL;
		}
		if (! DXSetEnumeratedMember(result, i++, child))
		{
		    DXDelete((Object)result);
		    return NULL;
		}
	    }

	    return (Object)result;
	}

	default:
	    return DXCopy(object, COPY_STRUCTURE);
    }
}

#define NOT_GEOMETRIC		0
#define GEOMETRIC_VECTORS	1
#define GEOMETRIC_POSITIONS	2

typedef struct {
    Field field;
    Matrix matrix;
} AT_field_args; 

static Field
ApplyTransform_Field(Field field, Matrix *matrix)
{
    AT_field_args args;

    field = (Field)DXCopy((Object)field, COPY_STRUCTURE);
    if (! field) 
	return NULL;

    if (! matrix)
	return field;

    args.field = field;
    args.matrix = *matrix;

    if (! DXAddTask(AT_field_task, (Pointer)&args, sizeof(args), 1.0))
	goto error;
    
    return field;

error:
    DXDelete((Object)field);
    return NULL;
}

static Error
AT_field_task(Pointer ptr)
{
    Array a;
    int   i;
    char  *name;
    Matrix adjxpose;
    int    adjxpose_done = 0;
    Field  field;
    Matrix *matrix;

    field  = ((AT_field_args *)ptr)->field;
    matrix = &((AT_field_args *)ptr)->matrix;

    i = 0;
    while (NULL != (a = (Array)DXGetEnumeratedComponentValue(field, i++, &name)))
    {
	/*doit = NOT_GEOMETRIC;*/

	if (! strcmp(name, "positions"))
	{
	    a = (Array)ApplyTransform_Array(a, matrix, 0);
	}
	else if (! strcmp(name, "normals")   ||
	         ! strcmp(name, "gradient")  ||
	         ! strcmp(name, "binormals") ||
	         ! strcmp(name, "tangents"))
	{
	    if (! adjxpose_done)
	    {
		adjxpose = DXAdjointTranspose(*matrix);
		adjxpose_done = 1;
	    }

	    a = (Array)ApplyTransform_Array(a, &adjxpose, 1);
	}
	else
	{
	    Object attr = DXGetComponentAttribute(field, name, "geometric");
	    if (attr)
	    {
		if (! strcmp(DXGetString((String)attr), "positional"))
		    a = ApplyTransform_Array(a, matrix, 0);
		else if (! strcmp(DXGetString((String)attr), "vector"))
		{
		    if (! adjxpose_done)
		    {
			adjxpose = DXAdjointTranspose(*matrix);
			adjxpose_done = 1;
		    }

		    a = ApplyTransform_Array(a, &adjxpose, 0);
		}
	    }
	}

	if (! a)
	    goto error;

	if (! DXSetComponentValue(field, name, (Object)a))
	    goto error;
    }

    DXDeleteComponent(field, "box");
    if (! DXEndField(field))
	goto error;

    return OK;

error:
    return ERROR;
}

static Array
ApplyTransform_Array(Array array, Matrix *m, int normalize)
{
    Array a;
    switch (DXGetArrayClass(array))
    {
	case CLASS_REGULARARRAY:
	    if (DXQueryConstantArray(array, NULL, NULL))
		a = ApplyTransform_ConstantArray(array, m, normalize);
	    else
		a = ApplyTransform_RegularArray(array, m, normalize);
	    break;
	case CLASS_PRODUCTARRAY:
	    a = ApplyTransform_ProductArray(array, m, normalize);
	    break;
	case CLASS_CONSTANTARRAY:
	    a = ApplyTransform_ConstantArray(array, m, normalize);
	    break;
	case CLASS_ARRAY:
	    a = ApplyTransform_GenericArray(array, m, normalize);
	    break;
	default:
	    DXSetError(ERROR_DATA_INVALID, 
			"Inappropriate array class for transformation");
	    return NULL;
    }

    return a;
}
	    
static Array
ApplyTransform_ProductArray(Array array, Matrix *m, int norm)
{
    ProductArray result = NULL;
    int          i, n;
    Array        terms[100];
    Matrix	 m0;
    int		 dim;

    if (! DXGetProductArrayInfo((ProductArray)array, &n, terms))
	goto error;
    
    if (NULL == 
	(terms[0] = (Array)ApplyTransform_Array((Array)terms[0], m, norm)))
	goto error;

    if (n > 1)
    {
	memcpy((char *)&m0, (char *)m, sizeof(Matrix));
	m0.b[0] = m0.b[1] = m0.b[2] = 0.0;

	for (i = 1; i < n; i++)
	{
	    terms[i] = (Array)ApplyTransform_Array((Array)terms[i], &m0, norm);
	    if (! terms[i])
	    {
		for (i--; i >= 0; i--)
		    DXDelete((Object)terms[i]);
		goto error;
	    }
	}
    }

    /*
     * Find max. dimensionality term and pad out remainder to match
     */
    dim = 0;
    for (i = 0; i < n; i++)
    {   
	int d;

	DXGetArrayInfo(terms[i], NULL, NULL, NULL, NULL, &d);

	if (d > dim)
	    dim = d;
    }

    for (i = 0; i < n; i++)
    {  
	Array new = SetDimensionality(terms[i], dim);

	if (! new)
	    goto error;

	if (new != terms[i])
	{
	    DXDelete((Object)terms[i]);
	    terms[i] = new;
	}
    }

    result = DXNewProductArrayV(n, terms);
    if (! result)
    {
	for (i--; i >= 0; i--)
	    DXDelete((Object)terms[i]);
	goto error;
    }

    return (Array)result;

error:
    DXDelete((Object)result);
    return NULL;
}

#define TRANSFORM_REGULARARRAY(type, round) 		\
{							\
    type *io = (type *)inOrigin;			\
    type *id = (type *)inDeltas;			\
    type *oo = (type *)outOrigin;			\
    type *od = (type *)outDeltas;			\
    int  i, j;						\
							\
    for (i = 0; i < nDout; i++)				\
    {							\
	double fo, fd;					\
							\
	fd = 0.0;					\
	fo = matrix->b[i];				\
							\
	for (j = 0; j < nDin; j++)			\
	{						\
	    fo += io[j]*matrix->A[j][i];		\
	    fd += id[j]*matrix->A[j][i];		\
	}						\
							\
	oo[i] = (type)(fo + round);			\
	od[i] = (type)(fd + round);			\
    }							\
}

static Array
ApplyTransform_RegularArray(Array array, Matrix *matrix, int norm)
{
    Type     type;
    Category cat;
    Pointer  inOrigin = NULL, outOrigin = NULL;
    Pointer  inDeltas = NULL, outDeltas = NULL;
    int      rank, shape[32], n, count, nDin, nDout, i;
    Array    result = NULL;

    if (! DXGetArrayInfo(array, &count, &type, &cat, &rank, shape))
	return NULL;
    
    if (rank != 1 || shape[0] > 3)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "data to be transformed must be vectors of dimensionality <= 3");
	return NULL;
    }

    if (norm)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "cannot normalize transformed non-constant regular vectors");
	return NULL;
    }

    nDin = shape[0];

    for (nDout = 3; nDout >= 1; nDout --)
    {
	int zero = (matrix->b[nDout-1] == 0.0);
	
	for (i = 0; i < nDin && zero; i++)
	    zero = (matrix->A[i][nDout-1] == 0.0);

	if (! zero)
	    break;
    }

    inOrigin  = DXAllocate(nDin*DXTypeSize(type));
    inDeltas  = DXAllocate(nDin*DXTypeSize(type));
    outOrigin = DXAllocate(nDout*DXTypeSize(type));
    outDeltas = DXAllocate(nDout*DXTypeSize(type));

    if (! inOrigin || ! inDeltas || ! outOrigin || ! outDeltas)
    {
	DXFree(inOrigin);
	DXFree(inDeltas);
	DXFree(outOrigin);
	DXFree(outDeltas);
	return NULL;
    }

    DXGetRegularArrayInfo((RegularArray)array, &n, inOrigin, inDeltas);

    switch(type)
    {
	case TYPE_DOUBLE:
		TRANSFORM_REGULARARRAY(double, 0.0);
		break;

	case TYPE_FLOAT:
		TRANSFORM_REGULARARRAY(float, 0.0);
		break;

	case TYPE_INT:
		TRANSFORM_REGULARARRAY(int, 0.5);
		break;

	case TYPE_UINT:
		TRANSFORM_REGULARARRAY(uint, 0.5);
		break;

	case TYPE_SHORT:
		TRANSFORM_REGULARARRAY(short, 0.5);
		break;

	case TYPE_USHORT:
		TRANSFORM_REGULARARRAY(ushort, 0.5);
		break;

	case TYPE_BYTE:
		TRANSFORM_REGULARARRAY(byte, 0.5);
		break;

	case TYPE_UBYTE:
		TRANSFORM_REGULARARRAY(ubyte, 0.5);
		break;
    }

    result=(Array)DXNewRegularArray(type, nDout, n, outOrigin, outDeltas);

    DXFree(outOrigin);
    DXFree(outDeltas);
    DXFree(inOrigin);
    DXFree(inDeltas);

    return result;
}

#define TRANSFORM_CONSTANTARRAY(type, round) 		\
{							\
    type *io = (type *)inVector;			\
    type *oo = (type *)outVector;			\
    int  i, j;						\
							\
    for (i = 0; i < nDout; i++)				\
    {							\
	double fo;					\
							\
	fo = matrix->b[i];				\
							\
	for (j = 0; j < nDin; j++)			\
	    fo += io[j]*matrix->A[j][i];		\
							\
	oo[i] = (type)(fo + round);			\
    }							\
}

#define NORMALIZE_CONSTANTARRAY(type)			\
{							\
    type *oo = (type *)outVector;			\
    double foo = 0.0;					\
    for (i = 0; i < nDout; i++) foo += oo[i]*oo[i];     \
    if (foo != 0.0)					\
	for (i = 0; i < nDout; i++) oo[i] /= foo;	\
}

static Array
ApplyTransform_ConstantArray(Array array, Matrix *matrix, int norm)
{
    Type     type;
    Category cat;
    int      rank, shape[32], count, nDin, nDout, i;
    Array    result = NULL;
    Pointer  inVector;
    Pointer  outVector;

    if (! DXGetArrayInfo(array, &count, &type, &cat, &rank, shape))
	return NULL;

    if (norm && type != TYPE_FLOAT && type != TYPE_DOUBLE)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"cannot normalize non-floating point vectors");
	return NULL;
    }
    
    if (rank != 1 || shape[0] > 3)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "data to be transformed must be vectors of dimensionality <= 3");
	return NULL;
    }

    nDin = shape[0];

    for (nDout = 3; nDout >= 1; nDout --)
    {
	int zero = (matrix->b[nDout-1] == 0.0);
	
	for (i = 0; i < nDin && zero; i++)
	    zero = (matrix->A[i][nDout-1] == 0.0);

	if (! zero)
	    break;
    }

    if (norm && type != TYPE_FLOAT && type != TYPE_DOUBLE)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"cannot normalize non-floating point vectors");
	return NULL;
    }

    inVector  = DXGetConstantArrayData(array);
    outVector = DXAllocate(nDin*DXTypeSize(type));
    if (! outVector)
	return NULL;
    
    switch(type)
    {
	case TYPE_DOUBLE:
		TRANSFORM_CONSTANTARRAY(double, 0.0);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(double);
		break;

	case TYPE_FLOAT:
		TRANSFORM_CONSTANTARRAY(float, 0.0);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(float);
		break;

	case TYPE_INT:
		TRANSFORM_CONSTANTARRAY(int, 0.5);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(int);
		break;

	case TYPE_UINT:
		TRANSFORM_CONSTANTARRAY(uint, 0.5);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(uint);
		break;

	case TYPE_SHORT:
		TRANSFORM_CONSTANTARRAY(short, 0.5);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(short);
		break;

	case TYPE_USHORT:
		TRANSFORM_CONSTANTARRAY(ushort, 0.5);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(ushort);
		break;

	case TYPE_BYTE:
		TRANSFORM_CONSTANTARRAY(byte, 0.5);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(byte);
		break;

	case TYPE_UBYTE:
		TRANSFORM_CONSTANTARRAY(ubyte, 0.5);
		if (norm)
		    NORMALIZE_CONSTANTARRAY(ubyte);
		break;
    }

    result=(Array)DXNewConstantArray(count, outVector, type, cat, rank, nDout);
    DXFree(outVector);

    return result;
}

#define TRANSFORM_GENERICARRAY(type, round)				   \
{									   \
    type *ip = (type *)inPointer;					   \
    type *op = (type *)outPointer;					   \
									   \
    for (i = 0; i < count; i++, ip += nDin, op += nDout) 		   \
    {									   \
	for (j = 0; j < nDout; j++)					   \
	{								   \
	    double f = m->b[j];						   \
									   \
	    for (k = 0; k < nDin; k++)					   \
		f += ip[k] * m->A[k][j];				   \
	    								   \
	    op[j] = (type)(f + round);					   \
	}								   \
    }									   \
}


#define NORMALIZE_GENERICARRAY(type)					   \
{									   \
    float foo;								   \
    type *op = (type *)outPointer;		   			   \
    for (j = 0; j < count; j++, op += nDout)				   \
    {									   \
	for (i = 0, foo = 0.0; i < nDout; i++) foo += op[i]*op[i];	   \
	if (foo != 0.0)							   \
	{								   \
	    foo = 1.0 / sqrt(foo);					   \
	    for (i = 0; i < nDout; i++) op[i] *= foo;			   \
	}								   \
    }									   \
}

static Array
ApplyTransform_GenericArray(Array array, Matrix *m, int norm)
{
    Category          cat;
    int               rank, shape[32];
    Array             result = NULL;
    int               i, j, k;
    int		      count;
    int		      nDin, nDout;
    Type	      type;
    Pointer  	      inPointer, outPointer;

    if (! DXGetArrayInfo(array, &count, &type, &cat, &rank, shape))
	goto error;
    
    if (rank != 1 || shape[0] > 3)
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "data to be transformed must be vectors of dimensionality <= 3");
	goto error;
    }

    nDin = shape[0];

    for (nDout = 3; nDout >= 1; nDout --)
    {
	int zero = (m->b[nDout-1] == 0.0);
	
	for (i = 0; i < nDin && zero; i++)
	    zero = (m->A[i][nDout-1] == 0.0);

	if (! zero)
	    break;
    }

    result = DXNewArray(type, cat, 1, nDout);
    if (! result)
	goto error;

    if (! DXAddArrayData(result, 0, count, NULL))
	goto error;
    
    inPointer  = DXGetArrayData(array);
    outPointer = DXGetArrayData(result);

    switch(type)
    {
	case TYPE_DOUBLE:
		TRANSFORM_GENERICARRAY(double, 0.0);
		if (norm)
		    NORMALIZE_GENERICARRAY(double);
		break;

	case TYPE_FLOAT:
		TRANSFORM_GENERICARRAY(float, 0.0);
		if (norm)
		    NORMALIZE_GENERICARRAY(float);
		break;

	case TYPE_INT:
		TRANSFORM_GENERICARRAY(int, 0.5);
		if (norm)
		    NORMALIZE_GENERICARRAY(int);
		break;

	case TYPE_UINT:
		TRANSFORM_GENERICARRAY(uint, 0.5);
		if (norm)
		    NORMALIZE_GENERICARRAY(uint);
		break;

	case TYPE_SHORT:
		TRANSFORM_GENERICARRAY(short, 0.5);
		if (norm)
		    NORMALIZE_GENERICARRAY(short);
		break;

	case TYPE_USHORT:
		TRANSFORM_GENERICARRAY(ushort, 0.5);
		if (norm)
		    NORMALIZE_GENERICARRAY(ushort);
		break;

	case TYPE_BYTE:
		TRANSFORM_GENERICARRAY(byte, 0.5);
		if (norm)
		    NORMALIZE_GENERICARRAY(byte);
		break;

	case TYPE_UBYTE:
		TRANSFORM_GENERICARRAY(ubyte, 0.5);
		if (norm)
		    NORMALIZE_GENERICARRAY(ubyte);
		break;
    }

    return result;

error:
    DXDelete((Object)result);
    return NULL;
}

	    
static Array
SetDimensionality_ProductArray(Array array, int n)
{
    ProductArray result = NULL;
    int          i;
    Array        terms[100];

    if (! DXGetProductArrayInfo((ProductArray)array, &n, terms))
	goto error;
    
    for (i = 0; i < n; i++)
    {
	terms[i] = (Array)SetDimensionality((Array)terms[i], n);
	if (! terms[i])
	{
	    for (i--; i >= 0; i--)
		DXDelete((Object)terms[i]);
	    goto error;
	}
    }

    result = DXNewProductArrayV(n, terms);
    if (! result)
    {
	for (i--; i >= 0; i--)
	    DXDelete((Object)terms[i]);
	goto error;
    }

    return (Array)result;

error:
    DXDelete((Object)result);
    return NULL;
}

static Array
SetDimensionality_RegularArray(Array array, int n)
{
    Type     	   type;
    Category 	   cat;
    unsigned char  *origin = NULL;
    unsigned char  *delta = NULL;
    int      	   rank, shape[32],  count, nD;
    Array    	   result = NULL;

    if (! DXGetArrayInfo(array, &count, &type, &cat, &rank, shape))
	return NULL;
    
    if (rank != 1)
    { 
	DXSetError(ERROR_INTERNAL, "can only set dimensionality of vectors");
	return NULL;
    }
    
    if (shape[0] == n)
	return array;

    if (shape[0] > n)
	nD = shape[0];
    else
	nD = n;

    origin  = DXAllocate(nD*DXTypeSize(type));
    delta   = DXAllocate(nD*DXTypeSize(type));

    if (! origin || ! delta)
    {
	DXFree(origin);
	DXFree(delta);
	return NULL;
    }

    DXGetRegularArrayInfo((RegularArray)array, NULL, origin, delta);

    if (n > shape[0])
    {
	int inSize  = shape[0]*DXTypeSize(type)*DXCategorySize(cat);
	int outSize = n*DXTypeSize(type)*DXCategorySize(cat);

	memset(origin+inSize, 0, outSize-inSize);
	memset(delta+inSize, 0, outSize-inSize);
    }

    result  = (Array)DXNewRegularArray(type, n, count, origin, delta);

    DXFree(origin);
    DXFree(delta);

    return result;
}

static Array
SetDimensionality_ConstantArray(Array array, int n)
{
    int      rank, shape[32], count;
    Type     type;
    Category cat;
    Array    result = NULL;
    Pointer  buf = NULL;
    int	     size;

    if (! DXGetArrayInfo(array, &count, &type, &cat, &rank, shape))
	return NULL;
    
    if (rank != 1)
    { 
	DXSetError(ERROR_INTERNAL, "can only set dimensionality of vectors");
	return NULL;
    }

    if (shape[0] == n)
	return array;
    
    size = n*DXTypeSize(type)*DXCategorySize(cat);

    buf = DXAllocateZero(size);
    if (! buf)
	return NULL;
    
    if (shape[0] < n)
	size = shape[0]*DXTypeSize(type)*DXCategorySize(cat);
    
    memcpy(buf, DXGetConstantArrayData(array), size);

    result = (Array)DXNewConstantArray(count, buf, type, cat, 1, n);

    DXFree(buf);
    
    return result;
}

static Array
SetDimensionality_GenericArray(Array array, int n)
{
    Category          cat;
    Type	      type;
    int               rank, shape[32];
    Array             result = NULL;
    int		      count;
    unsigned char     *inPointer, *outPointer;
    int		      inSize, outSize, i;

    if (! DXGetArrayInfo(array, &count, &type, &cat, &rank, shape))
	goto error;
    
    if (rank != 1)
    { 
	DXSetError(ERROR_INTERNAL, "can only set dimensionality of vectors");
	return NULL;
    }

    if (shape[0] == n)
	return array;

    result = DXNewArray(type, cat, 1, n);
    if (! result)
	goto error;

    if (! DXAddArrayData(result, 0, count, NULL))
    {
	DXDelete((Object)result);
	goto error;
    }
    
    inPointer  = (unsigned char *)DXGetArrayData(array);
    outPointer = (unsigned char *)DXGetArrayData(result);

    inSize  = DXGetItemSize(array);
    outSize = DXGetItemSize(result);

    if (shape[0] < n)
    {
	int size1 = inSize;
	int size2 = outSize - inSize;

	for (i = 0; i < count; i++)
	{
	    memcpy(outPointer, inPointer, size1);
	    memset(outPointer+size1, 0, size2);
	    inPointer  += inSize;
	    outPointer += outSize;
	}
    }
    else
    {
	for (i = 0; i < count; i++)
	{
	    memcpy(outPointer, inPointer, inSize);
	    inPointer  += inSize;
	    outPointer += outSize;
	}
    }

    return result;

error:
    DXDelete((Object)result);
    return NULL;
}

static Array
SetDimensionality(Array array, int n)
{
    Array a;

    if (DXGetObjectClass((Object)array) != CLASS_ARRAY)
    {
	DXSetError(ERROR_INTERNAL, "SetDimensionality called on non-Array");
	return NULL;
    }

    switch (DXGetArrayClass(array))
    {
	case CLASS_REGULARARRAY:
	    if (DXQueryConstantArray(array, NULL, NULL))
		a = SetDimensionality_ConstantArray(array, n);
	    else
		a = SetDimensionality_RegularArray(array, n);
	    break;
	case CLASS_PRODUCTARRAY:
	    a = SetDimensionality_ProductArray(array, n);
	    break;
	case CLASS_CONSTANTARRAY:
	    a = SetDimensionality_ConstantArray(array, n);
	    break;
	case CLASS_ARRAY:
	    a = SetDimensionality_GenericArray(array, n);
	    break;
	default:
	    DXSetError(ERROR_DATA_INVALID, 
			"Inappropriate array class for transformation");
	    return NULL;
    }

    return a;
}
