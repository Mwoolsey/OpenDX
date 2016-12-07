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
#include "objectClass.h"
#include "internals.h"

/*
 * DXBoundingBox returns a bounding box of an object.  The box is not
 * necessarily aligned to world coordinates if a transform has been applied.
 *
 * The top level bounding box routine just calls _dxf_BoundingBox with
 * a null transform; the transform argument is for propagating the
 * transforms downward.
 */


#define VALID_POSITIONS_ONLY 1
#define ALL_POSITIONS        0

Object
DXBoundingBox(Object o, Point *box)
{
    if (DXGetError()!=ERROR_NONE)
	DXErrorReturn(ERROR_INTERNAL,
		    "DXBoundingBox called with error code set");
    return _dxfBoundingBox(o, box, NULL, ALL_POSITIONS);
}

Object
DXValidPositionsBoundingBox(Object o, Point *box)
{
    if (DXGetError()!=ERROR_NONE)
	DXErrorReturn(ERROR_INTERNAL,
		    "DXBoundingBox called with error code set");
    return _dxfBoundingBox(o, box, NULL, VALID_POSITIONS_ONLY);
}

/*
 * compute a bounding box from a set of points
 */

static
void expand(float *out, int dim, int nboxpts, float *min, float *max)
{
    int i, j, k, m, n;
    n = dim * nboxpts;
    for (i=0, m=n/2; i<dim; i++, m/=2) {
	    for (j=0; j<n; j+=2*m) {
	    for (k=0; k<m; k+=dim) {
		out[i+j+k] = min[i];
		out[m+i+j+k] = max[i];
	    }
	}
    }
}


static Error
bbox(float *out, float *in, int dim, int nboxpts, int npoints,
	InvalidComponentHandle ich)
{
    float min[100], max[100];
    int i, j, k, n;

    /* callers should have checked this */
    ASSERT(npoints!=0);
    ASSERT(dim<100);

    /* initialize */
    for (i=0; i<dim; i++) {
        min[i] = DXD_MAX_FLOAT;
        max[i] = -DXD_MAX_FLOAT;
    }

    /* compute min/max */
    if (ich)
    {
	n = dim * npoints;
	for (i=k=0; i<n; k++, i+=dim) {
	    if (! DXIsElementInvalid(ich, k))
	    {
		for (j=0; j<dim; j++) {
		    if (in[i+j] < min[j])
			min[j] = in[i+j];
		    if (in[i+j] > max[j])
			max[j] = in[i+j];
		}
	    }
	}
    }
    else
    {
	n = dim * npoints;
	for (i=0; i<n; i+=dim) {
	    for (j=0; j<dim; j++) {
		if (in[i+j] < min[j])
		    min[j] = in[i+j];
		if (in[i+j] > max[j])
		    max[j] = in[i+j];
	    }
	}
    }

    /* expand min/max out to nboxpts corners */
    expand(out, dim, nboxpts, min, max);
    return OK;
}


/*
 * compute a bounding box from a group
 */

Object
_dxfGroup_BoundingBox(Group g, Point *box, Matrix *m, int validity)
{
    Object o;
    int i, j, n=0, align=0;
    Point min, max, p;
    Matrix mat, inv;

    /* compute the collective min/max of components */
    for (i=0; (o=DXGetEnumeratedMember(g, i, NULL)); i++) {

	/* get the component bounding box */
	if (!_dxfBoundingBox(o, box, m, validity)) {
	    if (DXGetError()!=ERROR_NONE)
		return NULL;		/* a real error occured */
	    continue;			/* or, the field had no bounding box */
	}

	/*
	 * Align group box to first box.  We do this by considering
	 * box[0] to be the origin of a coordinate system and box[1]-box[0],
	 * box[2]-box[0] and box[4]-box[0] to be the axes.  We transform
	 * all the bounding boxes into this coordinate system and form
	 * a bounding box there, and then transform that bounding box back
	 * to the original coordinate system when we're done.
	 *
	 * Modification: only do this if all dimensions are non-zero.
	 */
	if (box) {
	    if (n==0) {
		Vector v1, v2, v4;
		float l1, l2, l4;
		v1 = DXSub(box[1], box[0]), l1 = fabs(DXLength(v1));
		v2 = DXSub(box[2], box[0]), l2 = fabs(DXLength(v2));
		v4 = DXSub(box[4], box[0]), l4 = fabs(DXLength(v4));
		/* XXX - use an epsilon here */
		if (l1!=0.0 && l2!=0.0 && l4!=0.0) {
#if defined(ibm6000)
		    mat.A[0][0] = v1.x;
		    mat.A[0][1] = v1.y;
		    mat.A[0][2] = v1.z;

		    mat.A[1][0] = v2.x;
		    mat.A[1][1] = v2.y;
		    mat.A[1][2] = v2.z;

		    mat.A[2][0] = v4.x;
		    mat.A[2][1] = v4.y;
		    mat.A[2][2] = v4.z;

		    mat.b[0] = box[0].x;
		    mat.b[1] = box[0].y;
		    mat.b[2] = box[0].z;
#else
		    *(Point*) mat.A[0] = v1;
		    *(Point*) mat.A[1] = v2;
		    *(Point*) mat.A[2] = v4;
		    *(Point*) mat.b    = box[0];
#endif
		    inv = DXInvert(mat);
		    align = 1;
		}
		min.x = min.y = min.z = DXD_MAX_FLOAT;
		max.x = max.y = max.z = -DXD_MAX_FLOAT;
	    }
	    for (j=0; j<8; j++) {
		p = align? DXApply(box[j],inv) : box[j];
		if (p.x<min.x) min.x = p.x;  if (p.x>max.x) max.x = p.x;
		if (p.y<min.y) min.y = p.y;  if (p.y>max.y) max.y = p.y;
		if (p.z<min.z) min.z = p.z;  if (p.z>max.z) max.z = p.z;
	    }
	    n++;
	}
    }

    /*
     * DXTransform the bounding box back to the original
     * coordinate system (see above).
     */
    if (box) {
	if (n==0)
	    return NULL;
	for (i=0; i<8; i++) {
	    p = DXPt(i&1?max.x:min.x, i&2?max.y:min.y, i&4?max.z:min.z);
	    box[i] = align? DXApply(p, mat) : p;
	}
    }

    return (Object) g;
}

Object
_dxfCompositeField_BoundingBox(Group g, Point *box, Matrix *m, int validity)
{
    Object o;
    int i, j, n=0;
    Point min, max, *p;

    min.x = min.y = min.z = DXD_MAX_FLOAT;
    max.x = max.y = max.z = -DXD_MAX_FLOAT;

    /* compute the collective min/max of components */
    for (i=0; (o=DXGetEnumeratedMember(g, i, NULL)); i++) {

	/* get the component bounding box */
	if (!_dxfBoundingBox(o, box, NULL, validity)) {
	    if (DXGetError()!=ERROR_NONE)
		return NULL;		/* a real error occured */
	    continue;			/* or, the field had no bounding box */
	}

	/*
	 * Align group box to first box.  We do this by considering
	 * box[0] to be the origin of a coordinate system and box[1]-box[0],
	 * box[2]-box[0] and box[4]-box[0] to be the axes.  We transform
	 * all the bounding boxes into this coordinate system and form
	 * a bounding box there, and then transform that bounding box back
	 * to the original coordinate system when we're done.
	 *
	 * Modification: only do this if all dimensions are non-zero.
	 */
	if (box)
	{
	    for (j=0, p=box; j<8; j++, p++)
	    {
		if (p->x<min.x) min.x = p->x;  if (p->x>max.x) max.x = p->x;
		if (p->y<min.y) min.y = p->y;  if (p->y>max.y) max.y = p->y;
		if (p->z<min.z) min.z = p->z;  if (p->z>max.z) max.z = p->z;
	    }
	    n++;
	}
    }

    /*
     * DXTransform the bounding box back to the original
     * coordinate system (see above).
     */
    if (box) {
	if (n==0)
	    return NULL;
	for (i=0; i<8; i++)
	{
	    if (m)
	    {
		box[i] = DXApply(DXPt(i&1?max.x:min.x,
				      i&2?max.y:min.y,
				      i&4?max.z:min.z), *m);
	    }
	    else
	    {
		box[i] = DXPt(i&1?max.x:min.x,
			      i&2?max.y:min.y,
			      i&4?max.z:min.z);
	    }
	}
    }

    return (Object) g;
}


/*
 * compute a bounding box from a field
 */

static
Error
_copyout(Point *out, Matrix *m, Array a)
{
    int i, rank, dim;
    float *in;

    if (!DXGetArrayInfo(a, NULL, NULL, NULL, &rank, &dim))
	return ERROR;
    if (rank==0)
	dim = 1;
    in = (float *) DXGetArrayData(a);
    if (!in)
	return ERROR;

    if (dim==3) {
	if (m)
	    for (i=0; i<8; i++)
		out[i] = DXApply(((Point*)in)[i], *m);
	else
	    for (i=0; i<8; i++)
		out[i] = ((Point*)in)[i];
    } else if (dim==2) {
	if (m)
	    for (i=0; i<4; i++)
		out[2*i] = out[2*i+1] = DXApply(DXPt(in[2*i], in[2*i+1], 0), *m);
	else
	    for (i=0; i<4; i++)
		out[2*i] = out[2*i+1] = DXPt(in[2*i], in[2*i+1], 0);
    } else if (dim==1) {
	if (m)
	    for (i=0; i<2; i++)
		out[4*i] = out[4*i+1] = out[4*i+2] = out[4*i+3]
		    = DXApply(DXPt(in[i],0,0), *m);
	else
	    for (i=0; i<2; i++)
		out[4*i] = out[4*i+1] = out[4*i+2] = out[4*i+3]
		    = DXPt(in[i],0,0);
    } else {
	DXSetError(ERROR_BAD_PARAMETER, "positions must be 3-dimensional");
	return ERROR;
    }

    return OK;
}


/*
 * Check or figure out dimensionality
 */

static
Error
check(Array a, int *dim, int *nboxpts)
{
    Type type;
    Category category;
    int rank, i;

    /* type check */
    if (!DXGetArrayInfo(a, NULL, &type, &category, &rank, NULL))
	return ERROR;
    if (type!=TYPE_FLOAT || category!=CATEGORY_REAL || (rank!=0 && rank!=1))
	DXErrorReturn(ERROR_BAD_TYPE, "bad positions/box type in bounding box");

    /* get info */
    if (rank==0)
	*dim = 1;
    else if (!DXGetArrayInfo(a, NULL, NULL, NULL, NULL, dim))
	return ERROR;
    *nboxpts = 1;
    for (i=0; i<*dim; i++)
	*nboxpts *= 2;

    return OK;
}

/* XXX */
#define MAX_DIMS 25

Object
_dxfField_BoundingBox(Field f, Point *box, Matrix *m, int validity)
{
    Array points_array, box_array;
    float *in, *out;
    int i, j, dim, nboxpts, npoints, counts[MAX_DIMS];
    float origins[MAX_DIMS], deltas[MAX_DIMS*MAX_DIMS];
    float min[MAX_DIMS], max[MAX_DIMS];
    InvalidComponentHandle ich = NULL;
    int do_invalids = 0;

    /*
     * If its an empty field, then return NULL but no error.
     */
    if (DXEmptyField(f))
	goto null_return;

    /* points_array XXX return NULL if no points, but not an error */
    points_array = (Array) DXGetComponentValue(f, POSITIONS);
    if (!points_array)
	goto null_return;

    if (validity == VALID_POSITIONS_ONLY)
    {
	int invalids_present = ( DXGetComponentValue(f, "invalid positions") !=
				 NULL );

	/*
	 * if theres a valid_box component, or there are no invalids and
	 * there is a box component, then just return it, applying
	 * transform if any
	 */
	box_array = (Array) DXGetComponentValue(f, VBOX);
	if (! box_array && ! invalids_present)
	    box_array = (Array)DXGetComponentValue(f, BOX);

	if (box_array && box)
	    goto done;
    
	if (invalids_present)
	{
	    ich = DXCreateInvalidComponentHandle((Object)f, NULL, "positions");
	    if (! ich)
		goto null_return;

	    do_invalids = 1;
	}
	else
	{
	    validity = ALL_POSITIONS;
	    do_invalids = 0;
	}
    }
    else if (NULL != (box_array = (Array)DXGetComponentValue(f, BOX)))
	goto done;

    /* get dim, nboxpts */
    if (!check(points_array, &dim, &nboxpts))
	goto null_return;

    if (dim>MAX_DIMS)
    {
	DXSetError(ERROR_BAD_PARAMETER, "dimensionality of object is too high");
	goto null_return;
    }

    DXGetArrayInfo(points_array, &npoints, NULL, NULL, NULL, NULL);
    if (npoints==0 || (do_invalids && npoints == DXGetInvalidCount(ich)))
	goto null_return;

    /*
     * allocate array
     */
    box_array = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, dim);
    if (!DXAddArrayData(box_array, 0, nboxpts, NULL))
	goto null_return;

    if (do_invalids)
    {
	DXSetComponentValue(f, VBOX, (Object)box_array);
	DXSetAttribute((Object)box_array, DER, 
		  (Object)DXMakeStringList(2, POSITIONS, INVALID_POSITIONS));
    }
    else
    {
	DXSetComponentValue(f, BOX, (Object)box_array);
	DXSetAttribute((Object)box_array, DER, O_POSITIONS);
    }

    out = (float *) DXGetArrayData(box_array);
    if (!out)
	return ERROR;

    /* is it regular or irregular? */
    if (DXQueryGridPositions(points_array, NULL, counts, origins, deltas))
    {
	/*
	 * Check to see if the corners of the box are
	 * valid.  If so, we can just use the fast mechanism
	 */
	if (do_invalids)
	{
	    /* 
	     * quick out if the corners are valid
	     */
	    int index, i, j, strides[MAX_DIMS];

	    for (i = 0; i < dim; i++)
	    {
		strides[i] = counts[i] - 1;

		for (j = i+1; j < dim; j++)
		   strides[i] *= counts[j];
	    }
	    
	    do_invalids = 0;
	    for (i = 0; !do_invalids && i < 1<<dim; i++)
	    {
		for (j = index = 0; j < dim; j++)
		    if (i & (1<<j)) index += strides[j];
		
		do_invalids = DXIsElementInvalid(ich, index);
	    }
	}

	/*
	 * If all the corners were valid, just look at them
	 */
	if (! do_invalids)
	{
	    /*
	     * compute min/max
	     */
	    for (i=0; i<dim; i++) {
		float mn=0, mx=0, o;
		for (j=0; j<dim; j++) {
		    float m = deltas[j*dim+i] * (counts[j]-1);
		    if (m>0) mx += m;
		    else     mn += m;
		}
		o = origins[i];
		min[i] = o + mn;
		max[i] = o + mx;
	    }

	    /*
	     * expand min/max out to nboxpts corners
	     */
	    expand(out, dim, nboxpts, min, max);

	    goto done;
	}

    }
    
    if (DXGetArrayClass(points_array) == CLASS_ARRAY)
    {
	in = (float *) DXGetArrayData(points_array);
	if (!in)
	    return NULL;

	DXGetArrayInfo(points_array, &npoints, NULL, NULL, NULL, NULL);
	if (npoints==0)
	    return NULL;

	/* compute bounding box from points */
	bbox(out, in, dim, nboxpts, npoints, ich);
    }
    else
    {
	int	    i, j;
	float       min[MAX_DIMS], max[MAX_DIMS], *pt=NULL;
	ArrayHandle handle = DXCreateArrayHandle(points_array);
	Pointer     scratch = DXAllocate(DXGetItemSize(points_array));
	if (! handle || ! scratch)
	{
	    DXFreeArrayHandle(handle);
	    DXFree(scratch);
	    return ERROR;
	}

	DXGetArrayInfo(points_array, &npoints, NULL, NULL, NULL, NULL);
	if (npoints==0)
	{
	    DXFreeArrayHandle(handle);
	    DXFree(scratch);
	    return NULL;
	}

	for (i = 0; i < dim; i++)
	{
	    min[i] =  DXD_MAX_FLOAT;
	    max[i] = -DXD_MAX_FLOAT;
	}

	if (ich)
	    for (i = 0; i < npoints; i++)
	    {
		if (! DXIsElementInvalid(ich, i))
		{
		    pt = (float *)DXCalculateArrayEntry(handle, i, scratch);
		    for (j = 0; j < dim; j++)
		    {
			if (pt[j] < min[j]) min[j] = pt[j];
			if (pt[j] > max[j]) max[j] = pt[j];
		    }
		}
	    }
	else
	    for (i = 0; i < npoints; i++)
	    {
		pt = (float *)DXIterateArray(handle, i, (Pointer)pt, scratch);
		for (j = 0; j < dim; j++)
		{
		    if (pt[j] < min[j]) min[j] = pt[j];
		    if (pt[j] > max[j]) max[j] = pt[j];
		}
	    }
	

	DXFreeArrayHandle(handle);
	DXFree(scratch);

	/* expand min/max out to nboxpts corners */
	expand(out, dim, nboxpts, min, max);
    }


done:

    if (ich) {
        DXFreeInvalidComponentHandle(ich);
        ich = NULL;
	}

    if (box && !_copyout(box, m, box_array))
	goto null_return;

    return (Object) f;

null_return:

    if (ich)
        DXFreeInvalidComponentHandle(ich);
    
    return NULL;
}

/*
 * bounding box of xform node
 */

Object
_dxfXform_BoundingBox(Xform x, Point *box, Matrix *mp, int validity)
{
    Matrix m;
    Object o;

    /* get this node's matrix and object */
    if (!DXGetXformInfo(x, &o, &m))
	return NULL;

    /* concatenate this node's matrix m with incoming  matrix *mp */
    if (mp)
	m = DXConcatenate(m, *mp);

    /* compute o's bounding box */
    if (!_dxfBoundingBox(o, box, &m, validity))
	return NULL;

    return (Object) x;
}


/*
 * Bounding box of clipped node.  Note: ignore bounding box of
 * clipping object;  because it is generally based on the bounding
 * box of the transformed clipped object and then has its bbox
 * recomputed by DXEndField, it will have an unnecessarily large
 * bounding box; but this is ok, because its bounding box is of no
 * interest anyway.
 */

Object
_dxfClipped_BoundingBox(Clipped c, Point *box, Matrix *mp, int validity)
{
    Object render;

    /* use bounding box of render object, ignoring clipping object */
    if (!DXGetClippedInfo(c, &render, NULL))
	return NULL;
    if (!_dxfBoundingBox(render, box, mp, validity))
	return NULL;

    return (Object) c;
}


/*
 * Screen objects are in completely different coordinates, so
 * we don't include them in DXBoundingBox.
 */

Object
_dxfScreen_BoundingBox(Screen s, Point *box, Matrix *mp, int validity)
{
    Object o;
    int fixed, i;

    if (!DXGetScreenInfo(s, &o, &fixed, NULL))
	return NULL;
    if (fixed==SCREEN_WORLD) {
	static Point zero = {0, 0, 0}, p;
	p = mp? DXApply(zero, *mp) : zero;
	if (box)
	    for (i=0; i<8; i++)
		box[i] = p;
	return (Object) s;
    } else
	return NULL;
}

