/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/volume.c,v 5.0 92/11/12 09:09:08 svs Exp Locker: gresh 
 */


#include <string.h>
#include <stdlib.h>
#include <dx/dx.h>
#include "render.h"
#include <stdio.h>

/*
 * Make a sort routine called facesort()
 */

#define TYPE struct sortindex
#define LT(a,b) ((a)->z < (b)->z)
#define GT(a,b) ((a)->z > (b)->z)
#define QUICKSORT facesort
#include "qsort.c"


/*
 * Irregular.  At this point, _dxf_Gather (called from Tile) has gathered
 * into gather->sort all the volume and translucent faces.  We must
 * now sort them and call _dxf_Triangle to render them.
 *
 * XXX - a better sort algorithm would be desirable here
 *
 * The size of the elements that go into the sort array depends on such
 * factors as the element type (triangle, quad etc.); for faces derived
 * from connection-dependent volume elements, there is an additional word
 * naming the element that the face came from to identify the color.  This
 * size appears in SIZE macro in shade.c, the INFO macro in gather.c,
 * and the ADVANCE macro in volume.c, and these must be kept in sync.
 */

#define GET(s,n) (((int *)((long)(s) + sizeof(struct sort)))[n])

#define ADVANCE(n) \
    (s = (struct sort *)((long)s + sizeof(struct sort) + n * sizeof(int)))

#define IMMEDIATE(s,type) ((type *)((long)(s) + sizeof(struct sort)))

Error
_dxf_VolumeIrregular(struct buffer *b, struct gather *gather, int clip)
{
    struct sortindex *sortindex = NULL, *si;
    struct sort *s;
    struct xfield *xf;
    Quadrilateral quad;
    Triangle tri;
    Line line;
    Point *p;
    float z;
    int i, j, g, n;

    /* anything to do? */
    if (!gather->nthings)
	return OK;

    /* allocate buffer for sort index array */
    sortindex = (struct sortindex *)
	DXAllocateLocal(gather->nthings * sizeof(struct sortindex));
    if (!sortindex) {
	DXResetError();
	sortindex = (struct sortindex *)
	    DXAllocate(gather->nthings * sizeof(struct sortindex));
	if (!sortindex)
	    goto error;
	DXWarning("sort index array (%d bytes) "
		"is too big for local memory",
		gather->nthings * sizeof(struct sortindex));
    }

    /* compute z values, sort the faces */
    for (i=0, s=gather->sort; s<gather->current; i++) {
	xf = &gather->fields[s->field];
	p = xf->positions;
	sortindex[i].sort = s;
	switch (s->type) {
	case SORT_POINT:
	    z = p[GET(s,0)].z;
	    ADVANCE(1);
	    break;
	case SORT_LINE:
	    line = xf->c.lines[GET(s,0)];
	    z = (((double)p[line.p].z) + ((double)p[line.q].z)) / 2.0;
	    ADVANCE(1);
	    break;
	case SORT_POLYLINE:
	    line.p = xf->edges[GET(s,1)];
	    line.q = xf->edges[GET(s,1)+1];
	    z = (((double)p[line.p].z) + ((double)p[line.q].z)) / 2.0;
	    ADVANCE(2);
	    break;
	case SORT_TRI:
	    tri = xf->c.triangles[GET(s,0)];
	    z = (((double)p[tri.p].z) + ((double)p[tri.q].z) + 
		 ((double)p[tri.r].z)) / 3.0;
	    ADVANCE(1);
	    break;
	case SORT_QUAD:
	    quad = xf->c.quads[GET(s,0)];
	    z = (((double)p[quad.p].z) + ((double)p[quad.q].z) +
		 ((double)p[quad.r].z) + ((double)p[quad.s].z)) / 4.0;
	    ADVANCE(1);
	    break;
	case SORT_INV_TRI_PTS:
	case SORT_TRI_PTS:
	    z = (((double)p[GET(s,0)].z) + ((double)p[GET(s,1)].z) +
	         ((double)p[GET(s,2)].z)) / 3.0;
	    ADVANCE(3);
	    break;
	case SORT_INV_TRI_FLAT:
	case SORT_TRI_FLAT:
	    z = (((double)p[GET(s,0)].z) + ((double)p[GET(s,1)].z) +
	   	 ((double)p[GET(s,2)].z)) / 3.0;
	    ADVANCE(4);
	    break;
	case SORT_INV_QUAD_PTS:
	case SORT_QUAD_PTS:
	    z = (((double)p[GET(s,0)].z) + ((double)p[GET(s,1)].z) +
		 ((double)p[GET(s,2)].z) + ((double)p[GET(s,3)].z)) / 4.0;
	    ADVANCE(4);
	    break;
	case SORT_INV_QUAD_FLAT:
	case SORT_QUAD_FLAT:
	    z = (((double)p[GET(s,0)].z) + ((double)p[GET(s,1)].z) +
		 ((double)p[GET(s,2)].z) + ((double)p[GET(s,3)].z)) / 4.0;
	    ADVANCE(5);
	    break;
	default:
	    DXErrorReturn(ERROR_INTERNAL, "unknown type in sort array");
	}
	sortindex[i].z = z;
    }
    DXMarkTimeLocal("start sort");
    facesort(sortindex, gather->nthings);
    DXMarkTimeLocal("end sort");

    /* render them in sorted order */
    for (i=0, si=sortindex, n=gather->nthings; i<n; i++, si++) {

	s = si->sort;
	xf = &gather->fields[s->field];

	ASSERT(xf->opacities || xf->volume);

	/*
	 * Merge identical surface faces from different fields into one
	 * non-surface face.  This avoids artifacts that occur when surface
	 * faces, e.g. in partitions, are mis-sorted.  (Identical is taken
	 * to mean identical z sort values, face type, x sum).
	 */


#define SUM(s,xf,x) (							\
    p = xf->positions,							\
    type==SORT_QUAD_PTS?						\
	p[GET(s,0)].x + p[GET(s,1)].x + p[GET(s,2)].x + p[GET(s,3)].x :	\
	p[GET(s,0)].x + p[GET(s,1)].x + p[GET(s,2)].x			\
)

#define IGNORE (2)

	if (s->surface==IGNORE)
	    continue;
	if (s->surface) {
	    int type = s->type;
	    if (type==SORT_QUAD_PTS || type==SORT_TRI_PTS) {
		float x = SUM(s,xf,x);
		float y = SUM(s,xf,y);
		float z = si->z;
		struct sortindex *si1;
		Point *p;
		for (j=i+1, si1=si+1;  j<n && si1->z==z;  j++, si1++) {
		    struct sort *s1 = si1->sort;
		    struct xfield *xf1 = &gather->fields[s1->field];
		    if (s1->surface && xf1!=xf && s1->type==type) {
			float x1 = SUM(s1,xf1,x);
			float y1 = SUM(s1,xf1,y);
			if (x==x1 && y==y1) {
			    s->surface = 0;
			    s1->surface = IGNORE;
			}
		    }
		}
	    }
	}

#define FCOLORP(i) (Pointer)(xf->cmap? \
    (Pointer)&(((unsigned char *)xf->fcolors)[xf->fcst ? 0 : i]) : \
    (Pointer)&(((RGBColor *)xf->fcolors)[xf->fcst ? 0 : i]))

#define BCOLORP(i) (Pointer)(xf->cmap? \
    (Pointer)&(((unsigned char *)xf->bcolors)[xf->fcst ? 0 : i]) : \
    (Pointer)&(((RGBColor *)xf->bcolors)[xf->fcst ? 0 : i]))

#define OPACITYP(i) (Pointer)(xf->omap? \
    (Pointer)&(((unsigned char *)xf->opacities)[xf->ocst ? 0 : i]) : \
    xf->opacities? (Pointer)&(((float *)xf->opacities)[xf->ocst ? 0 : i]) : NULL)


	switch (s->type) {
	case SORT_POINT:
	    if (!_dxf_Point(b, xf, GET(s,0)))
		goto error;
	    break;
	case SORT_LINE:
	    g = GET(s,0);
	    if (xf->colors_dep == dep_positions) {
		if (!_dxf_Line(b, xf, 1, xf->c.lines, &g, clip, INV_VALID))
		    goto error;
	    } else if (xf->colors_dep == dep_connections) {
		if (!_dxf_LineFlat(b, xf, 1, xf->c.lines, &g,
				xf->fcolors, xf->opacities,
				clip, INV_VALID))
		    goto error;
	    }
	    break;
	case SORT_POLYLINE:
            line.p = xf->edges[GET(s,1)];
	    line.q = xf->edges[GET(s,1)+1];
	    if (xf->colors_dep == dep_positions) {
		if (!_dxf_Line(b, xf, 1, &line, NULL, clip, INV_VALID))
		    goto error;
	    } else if (xf->colors_dep == dep_polylines) {
		if (!_dxf_LineFlat(b, xf, 1, &line, NULL,
			       xf->fcolors, xf->opacities,
			       clip, INV_VALID))
		    goto error;
	    }
	    break;
	case SORT_TRI:
	    g = GET(s,0);		    
	    if (xf->colors_dep == dep_positions || xf->lights) {
		if (!_dxf_Triangle(b, xf, 1, xf->c.triangles+g, &g,
					s->surface, clip, INV_VALID))
		    goto error;
	    } else if (xf->colors_dep == dep_connections) {
		if (!_dxf_TriangleFlat(b, xf, 1, xf->c.triangles+g, NULL,
				   FCOLORP(g), BCOLORP(g),
				   OPACITYP(g), s->surface, clip, INV_VALID))
		    goto error;
	    }
	    break;
	case SORT_QUAD:
	    g = GET(s,0);
	    if (xf->colors_dep == dep_positions || xf->lights) {
		if (!_dxf_Quad(b, xf, 1, xf->c.quads+g, &g,
						s->surface, clip, INV_VALID))
		    goto error;
	    } else if (xf->colors_dep == dep_connections) {
		if (!_dxf_QuadFlat(b, xf, 1, xf->c.quads+g, NULL,
			       FCOLORP(g), BCOLORP(g),
			       OPACITYP(g), s->surface, clip, INV_VALID))
		    goto error;
	    }
	    break;
	case SORT_TRI_PTS:
	    if (!_dxf_Triangle(b, xf, 1, IMMEDIATE(s,Triangle), NULL,
						s->surface, clip, INV_VALID))
		goto error;
	    break;
	case SORT_INV_TRI_PTS:
	    if (!_dxf_Triangle(b, xf, 1, IMMEDIATE(s,Triangle), NULL,
						s->surface, clip, INV_INVALID))
		goto error;
	    break;
	case SORT_TRI_FLAT:
	    g = GET(s,3);
	    if (!_dxf_TriangleFlat(b, xf, 1, IMMEDIATE(s,Triangle), NULL,
			       FCOLORP(g), BCOLORP(g),
			       OPACITYP(g), s->surface, clip, INV_VALID))
		goto error;
	    break;
	case SORT_INV_TRI_FLAT:
	    g = GET(s,3);
	    if (!_dxf_TriangleFlat(b, xf, 1, IMMEDIATE(s,Triangle), NULL,
			       FCOLORP(g), BCOLORP(g),
			       OPACITYP(g), s->surface, clip, INV_INVALID))
		goto error;
	    break;
	case SORT_QUAD_PTS:
	    if (!_dxf_Quad(b, xf, 1, IMMEDIATE(s,Quadrilateral), NULL,
			    s->surface, clip, INV_VALID))
		goto error;
	    break;
	case SORT_INV_QUAD_PTS:
	    if (!_dxf_Quad(b, xf, 1, IMMEDIATE(s,Quadrilateral), NULL,
			    s->surface, clip, INV_INVALID))
		goto error;
	    break;
	case SORT_QUAD_FLAT:
	    g = GET(s,4);
	    if (!_dxf_QuadFlat(b, xf, 1, IMMEDIATE(s,Quadrilateral), NULL,
			   FCOLORP(g), BCOLORP(g),
			   OPACITYP(g), s->surface, clip, INV_VALID))
		goto error;
	    break;
	case SORT_INV_QUAD_FLAT:
	    g = GET(s,4);
	    if (!_dxf_QuadFlat(b, xf, 1, IMMEDIATE(s,Quadrilateral), NULL,
			   FCOLORP(g), BCOLORP(g),
			   OPACITYP(g), s->surface, clip, INV_INVALID))
		goto error;
	    break;
	default:
	    DXErrorReturn(ERROR_INTERNAL, "unknown type in sort array");
	}
    }

    DXFree((Pointer)sortindex);
    return OK;

error:
    DXFree((Pointer)sortindex);
    return ERROR;
}


/*
 * Some macros to help us access counts (Ki), strides (Si), and
 * delta vectors (Di), and also the same for the indices into
 * connection-dependent colors.
 */

#define D0 xf->d[0]
#define D1 xf->d[1]
#define D2 xf->d[2]

#define K0 xf->k[0]
#define K1 xf->k[1]
#define K2 xf->k[2]

#define S0 xf->s[0]
#define S1 xf->s[1]
#define S2 xf->s[2]


#define CK0 xf->ck[0]
#define CK1 xf->ck[1]
#define CK2 xf->ck[2]

#define CS0 xf->cs[0]
#define CS1 xf->cs[1]
#define CS2 xf->cs[2]


/*
 *
 */

#define FLOOR(a) (((int)((a)+100000.0)-100000))
#define CEIL(a) (((int)((a)-100000.0)+100000))
#define ROUND(a) FLOOR((a)+0.5)
#define ABS(a) ((a)<0? -(a) : (a))
#define DOTXY(u,v) (u.x*v.x+u.y*v.y)

static
void
reduce(Vector u, Vector v, Vector *up, Vector *vp)
{
    int i, j;
    float d;
    do {
	d = DOTXY(u,u);
	if (!d) break;
	d = DOTXY(u,v) / d;
	i = d>0? (int)(d+0.4999) : (int)(d-0.4999);
	v = DXSub(v, DXMul(u,i));
	d = DOTXY(v,v);
	if (!d) break;
	d = DOTXY(v,u) / d;
	j = d>0? (int)(d+0.4999) : (int)(d-0.4999);
	u = DXSub(u, DXMul(v,j));
    } while (i!=0 || j!=0);
    *up = u, *vp = v;
}

static
float
side(Vector u, Vector v)
{
    float a, b, minx, maxx, miny, maxy;
#   define mm(MAX,x) (a = MAX(0,u.x), b = MAX(v.x,u.x+v.x), MAX(a,b))
    maxx = mm(MAX,x);
    minx = mm(MIN,x);
    maxy = mm(MAX,y);
    miny = mm(MIN,y);
    return MAX(maxx-minx, maxy-miny);
}

static
float
area(Vector u, Vector v)
{
    float a = u.x*v.y - u.y*v.x;
    return ABS(a);
}


/*
 * A number of the algorithms depend on enumerating planes of voxels
 * or voxel faces that are the most parallel to the image plane.  This
 * is judged by comparing the delta z's between the planes, as measured
 * in the direction of the camera view (i.e. perpendicular to the
 * image plane).  The orient() routine accomplishes this by reordering
 * the axes so that the number 2 axis has the smallest delta z between planes,
 * and returns that delta z.  After calling orient(), the rendering algorithm
 * can then just be written to iterate through axis 2 in the outermost loop.
 */

static void
swap(struct xfield *xf, float *den, int a, int b) {
    if (ABS(den[a]) > ABS(den[b])) {
	{float t; t=den[a]; den[a]=den[b]; den[b]=t;}
	{Vector t; t=xf->d[a]; xf->d[a]=xf->d[b]; xf->d[b]=t;}
	{int t; t=xf->k[a]; xf->k[a]=xf->k[b]; xf->k[b]=t;}
	{int t; t=xf->s[a]; xf->s[a]=xf->s[b]; xf->s[b]=t;}
	{int t; t=xf->ck[a]; xf->ck[a]=xf->ck[b]; xf->ck[b]=t;}
	{int t; t=xf->cs[a]; xf->cs[a]=xf->cs[b]; xf->cs[b]=t;}
    }
}

static
float
orient(struct xfield *xf, int print)
{
    float num, den[3];

    num = D0.x*D1.y*D2.z + D1.x*D2.y*D0.z + D2.x*D0.y*D1.z
	- D2.x*D1.y*D0.z - D0.x*D2.y*D1.z - D1.x*D0.y*D2.z;
    den[0] = D1.x*D2.y - D2.x*D1.y;
    den[1] = D2.x*D0.y - D0.x*D2.y;
    den[2] = D0.x*D1.y - D1.x*D0.y;

    if (print)
	DXDebug("R", "%.3f / %.3f,%.3f,%.3f", num, den[0], den[1], den[2]);

    /* bubble largest den[i] (smallest dz) to number 2 position */
    swap(xf, den, 0, 1);
    swap(xf, den, 1, 2);

    if (print)
	DXDebug("R", "%.3f / %.3f,%.3f,%.3f", num, den[0], den[1], den[2]);

    return num / den[2];
}


/*
 * Regular volume rendering.  In this case, _dxf_Gather only gathered the
 * fields in gather->fields, because Tile set gather->sort to NULL.
 * For each field, we determine the backmost corner and deltas and
 * strides relative to that corner, and then sort the fields wrt the
 * z value of the backmost corner.  Then we enumerate the fields in
 * order of the z value of the backmost corner.  This is guaranteed
 * to be the correct order for partitioned regular fields.  For each
 * field we then paint the field according to one of a number of
 * algorithms:
 *
 * 0 - automatically choose one
 * 1 - all faces using irregular element algorithm
 * 2 - one plane direction (plus edges) using irregular element algorithm
 * 3 - composite planes, using smooth shaded quads
 * 4 - composite planes, using flat shaded quads w/ average of four corners
 * 5 - composite planes, using flat shaded quads w/ corner value
 * 6 - patch
 * 7 - pixel average
 */

static
int
#ifdef OS2
_Optlink
#endif
compare(const void *p1, const void * p2)
{
    const struct xfield **a = (const struct xfield **) p1;
    const struct xfield **b = (const struct xfield **) p2;
    if ((*a)->o.z < (*b)->o.z)
	return -1;
    else if ((*a)->o.z > (*b)->o.z)
	return 1;
    else return 0;
}

static
void
step(struct xfield *xf, int i)		    /* step along axis i: */
{
    Vector d, m;
    d = xf->d[i];
    if (d.z < 0) {			    /* if it takes us towards back */
	m = DXMul(d, xf->k[i]-1);		    /* delta to move to new origin */
	xf->o = DXAdd(xf->o, m);		    /* move origin */
	xf->d[i] = DXNeg(d);		    /* adjust delta vector */
	xf->n += (xf->k[i]-1) * xf->s[i];   /* adjust origin index */
	xf->s[i] = -xf->s[i];		    /* adjust stride */
	xf->cn += (xf->ck[i]-1) * xf->cs[i];/* same thing for */
	xf->cs[i] = -xf->cs[i];		    /* connection-dep indices */
    }
}


/*
 *
 */

/* --CHECKME-- the following was prototyped but never defined 
static Error VolumeRegularQuad(struct buffer *, struct xfield *, int);
 */

static Error VolumeRegularFace(struct buffer *, struct xfield *, int);
static Error VolumeRegularFace3(struct buffer *, struct xfield *, int);
static Error VolumeRegularPlane(struct buffer *, struct xfield *, int);
Error
_dxf_VolumeRegular(struct buffer *b, struct gather *gather, int clip)
{
    int i /*, patch_size = 0*/;
    struct xfield **xfields, *xf = NULL;
    Vector u, v;
    float a, s;

    /* anything to do? */
    if (!gather->nfields)
	return OK;

    /* allocate an array of pointers to xfields */
    xfields = (struct xfield **)
	DXAllocateLocal(gather->nfields * sizeof(struct xfield *));
    if (!xfields)
	goto error;

    /*
     * Collect counts (->k[i]) for each axis; compute strides (->s[i])
     * from counts; get origin (->o), get delta vectors (->d[i]) for each
     * axis; step along edges of cube to get to back corner (new origin
     * ->o and point index of this origin ->n), adjusting strides and
     * deltas as necessary.
     */
    for (i=0; i<gather->nfields; i++) {

	int n;

	/* xf */
	xf = &gather->fields[i];
	xfields[i] = xf;

	/* perspective? */
	if (xf->tile.perspective)
	    DXErrorReturn(ERROR_BAD_PARAMETER,
			"perspective volume rendering is not supported");

	/* counts and strides */
	DXQueryGridConnections(xf->connections_array, &n, xf->k);
	ASSERT(n==3);
	S2 = 1;				    /* stride for axis 2 */
	S1 = K2;			    /* stride for axis 1 */
	S0 = K2 * K1;			    /* stride for axis 0 */


	/* for conn-dep colors */
        CK0 = K0 - 1;			    /* one fewer in ea dim */
        CK1 = K1 - 1;			    /* for conn-dep colors */
        CK2 = K2 - 1;
	CS2 = 1;			    /* stride for axis 2 */
	CS1 = CK2;			    /* stride for axis 1 */
	CS0 = CK2 * CK1;		    /* stride for axis 0 */

	/* origin and delta vectors */
	DXQueryGridPositions(xf->positions_array, &n, NULL,
			   (float *)(&xf->o), (float *)(xf->d));
	ASSERT(n==3);

	/* step along axes to get to back corner */
	step(xf, 0);
	step(xf, 1);
	step(xf, 2);
    }

    /* sort the fields by z value of back corner */
    qsort((char *)xfields, gather->nfields, sizeof(*xfields), compare);

    /* compute parameters */
    if (xf->volAlg == 4)
    {
	xf = xfields[0];
	orient(xf, 1);
	a = area(D0, D1);
	s = side(D0, D1);
	DXDebug("R", "(%.2f,%.2f) x (%.2f,%.2f): area %.2f, side %.2f",
		    D0.x, D0.y, D1.x, D1.y, a, s);
	reduce(D0, D1, &u, &v);
	a = area(u, v);
	s = side(u, v);
	DXDebug("R", "(%.2f,%.2f) x (%.2f,%.2f): area %.2f, side %.2f",
	      u.x, u.y, v.x, v.y, a, s);
	/*patch_size = CEIL(s);*/
	/* XXX - warp doesn't do z yet, or color multipliers */
	if (s<=1)
	    b->vol = /*"warp"*/ "plane";
	else if (a<50)
	    b->vol = "plane";
	else if (a < 75)
	    b->vol = "face";
	else
	    b->vol = "face3";
    }
    else if (xf->volAlg == 1)
	b->vol = "face";
    else if (xf->volAlg == 2)
	b->vol = "face3";
    else
	b->vol = "plane";

    /*
     * If there are invalid connections, cannot use plane algorithm
     */
    if (xf->iElts && !strcmp(b->vol, "plane"))
       b->vol = "face";

    /* for each field, in sorted order, call appropriate rendering routine */
    for (i=0; i<gather->nfields; i++) {
	struct xfield *xf = xfields[i];
	Error rc;
	if (strcmp(b->vol,"face")==0)
	    rc = VolumeRegularFace(b, xf, clip);
	else if (strcmp(b->vol,"face3")==0)
	    rc = VolumeRegularFace3(b, xf, clip);
	else if (strcmp(b->vol,"plane")==0)
	    rc = VolumeRegularPlane(b, xf, clip);
	else
	    DXErrorGoto(ERROR_BAD_PARAMETER, "unknown volume rendering method");
	if (xf->positions_local) {
	    DXFreeArrayDataLocal(xf->positions_array, (Pointer)xf->positions);
    	    xf->positions = NULL;
	    xf->positions_local = 0;
	}
	if (!rc) goto error;
    }

    DXFree((Pointer)xfields);
    return OK;

error:
    DXFree((Pointer)xfields);
    return ERROR;
}


/*
 * Looping macros
 */

#define FOR2(k) for (i2=0,n2=xf->n; i2<k;  i2++,n2+=S2)
#define FOR1(k) for (i1=0,n1=n2;    i1<k;  i1++,n1+=S1)
#define FOR0(k) for (i0=0,n =n1;    i0<k;  i0++,n +=S0)


/*
 * DXRender all faces using the same face-based algorithm as for
 * irregular grids
 */

/* XXX - for now */
#define EXPAND_POSITIONS						    	\
    xf->positions = (Point *) DXGetArrayDataLocal(xf->positions_array);	    	\
    xf->positions_local = 1;						        \
    if (!xf->positions)								\
	return ERROR;

#define Q(o, i, j, surface) {							\
	Quadrilateral quad;							\
        quad.p = o, quad.q = o+i, quad.r = o+j, quad.s = o+i+j;			\
        if (!_dxf_Quad(b, xf, 1, &quad, NULL, surface, clip, INV_VALID))	\
	    return ERROR;							\
    }

#define Q_INVALID(o, i, j, surface) {						\
	Quadrilateral quad;							\
        quad.p = o, quad.q = o+i, quad.r = o+j, quad.s = o+i+j;			\
        if (!_dxf_Quad(b, xf, 1, &quad, NULL, surface, clip, INV_INVALID))	\
	    return ERROR;							\
    }

#define QF_INVALID(o, i, j, col, surface) {					\
	Quadrilateral quad;							\
        quad.p = o, quad.q = o+i, quad.r = o+j, quad.s = o+i+j;			\
        if (!_dxf_QuadFlat(b, xf, 1, &quad, NULL, 				\
		       FCOLORP(col), BCOLORP(col), OPACITYP(col),		\
		       surface, clip, INV_INVALID))				\
	    return ERROR;							\
    }

#define QF(o, i, j, col, surface) {						\
	Quadrilateral quad;							\
        quad.p = o, quad.q = o+i, quad.r = o+j, quad.s = o+i+j;			\
        if (!_dxf_QuadFlat(b, xf, 1, &quad, NULL,				\
		       FCOLORP(col), BCOLORP(col), OPACITYP(col),		\
		       surface, clip, INV_VALID))				\
	    return ERROR;							\
    }

#define FORWARDS  1
#define BACKWARDS 2


static Error
VolumeRegularFace3(struct buffer *b, struct xfield *xf, int clip)
{
    int i0, i1, i2, n0, n1, n2;
    int col0, col1, col2;
    InvalidComponentHandle ich = xf->iElts;

    EXPAND_POSITIONS;
    orient(xf, 0);

    if (xf->colors_dep==dep_positions) {

	if (ich)
	{
	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    Q(n0, S1, S2, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    Q(n0, S0, S2, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }

	    /*
	     * Now the front faces of all the cubes
	     */
	    n2 = xf->n, col2 = xf->cn;
	    for (i2 = 0; i2 <= CK2; i2++)
	    {
	        int surface = i2 == 0 || i2 == CK2;

	        col1 = col2;
	        n1   = n2;

	        for (i1 = 0; i1 < CK1; i1++)
	        {
		    col0 = col1;
		    n0   = n1;

		    for (i0 = 0; i0 < CK0; i0++)
		    {
			if (i2 != 0 && DXIsElementInvalid(ich, col0-CS2))
			{
			    Q_INVALID(n0, S0, S1, surface);
			    Q_INVALID(n0-S2+S1, S0, S2, i1==(CK1-1));
			    Q_INVALID(n0+S0-S2, S2, S1, i0==(CK0-1));
			}
			else
			{
			    Q(n0, S0, S1, surface);
			    if (i2 != 0)
			    {
				Q(n0-S2+S1, S0, S2, i1==(CK1-1));
				Q(n0+S0-S2, S2, S1, i0==(CK0-1));
			    }
			}
     
			col0 += CS0;
			n0   +=  S0;
		    }

		    col1 += CS1;
		    n1   +=  S1;
		 }

		 col2 += CS2, n2 += S2;
	    }
	}
	else
	{
	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    Q(n0, S1, S2, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    Q(n0, S0, S2, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }

	    /*
	     * Now the front faces of all the cubes
	     */
	    n2 = xf->n, col2 = xf->cn;
	    for (i2 = 0; i2 <= CK2; i2++)
	    {
	        int surface = i2 == 0 || i2 == CK2;

	        col1 = col2;
	        n1   = n2;

	        for (i1 = 0; i1 < CK1; i1++)
	        {
		    col0 = col1;
		    n0   = n1;

		    for (i0 = 0; i0 < CK0; i0++)
		    {
			Q(n0, S0, S1, surface);
			if (i2 != 0)
			{
			    Q(n0-S2+S1, S0, S2, i1==(CK1-1));
			    Q(n0+S0-S2, S2, S1, i0==(CK0-1));
			}
     
			col0 += CS0;
			n0   +=  S0;
		    }

		    col1 += CS1;
		    n1   +=  S1;
		 }

		 col2 += CS2, n2 += S2;
	    }
	}

    } else if (xf->colors_dep==dep_connections) {

	if (ich)
	{
	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    QF(n0, S1, S2, col0, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    QF(n0, S0, S2, col0, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }

	    /*
	     * Now the front faces of all the cubes
	     */
	    n2 = xf->n, col2 = xf->cn;
	    for (i2 = 0; i2 <= CK2; i2++)
	    {
	        int surface = i2 == 0 || i2 == CK2;

	        col1 = col2;
	        n1   = n2;

	        for (i1 = 0; i1 < CK1; i1++)
	        {
		    col0 = col1;
		    n0   = n1;

		    for (i0 = 0; i0 < CK0; i0++)
		    {
			if (i2 != 0 && DXIsElementInvalid(ich, col0-CS2))
			{
			    QF_INVALID(n0, S0, S1, col0-CS2, surface);
			    QF_INVALID(n0-S2+S1, S0, S2, col0-CS2, i1==(CK1-1));
			    QF_INVALID(n0+S0-S2, S2, S1, col0-CS2, i0==(CK0-1));
			}
			else if (i2 == 0)
			{
			    QF(n0, S0, S1, col0, surface)
			}
			else
			{
			    QF(n0, S0, S1, col0-CS2, surface);
			    QF(n0-S2+S1, S0, S2, col0-CS2, i1==(CK1-1));
			    QF(n0+S0-S2, S2, S1, col0-CS2, i0==(CK0-1));
			}
     
			col0 += CS0;
			n0   +=  S0;
		    }

		    col1 += CS1;
		    n1   +=  S1;
		 }

		 col2 += CS2, n2 += S2;
	    }

	}
	else
	{
	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    QF(n0, S1, S2, col0, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    QF(n0, S0, S2, col0, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }

	    /*
	     * Now the front faces of all the cubes
	     */
	    n2 = xf->n, col2 = xf->cn;
	    for (i2 = 0; i2 <= CK2; i2++)
	    {
	        int surface = i2 == 0 || i2 == CK2;

	        col1 = col2;
	        n1   = n2;

	        for (i1 = 0; i1 < CK1; i1++)
	        {
		    col0 = col1;
		    n0   = n1;

		    for (i0 = 0; i0 < CK0; i0++)
		    {
			if (i2 == 0)
			{
			    QF(n0, S0, S1, col0, surface)
			}
			else
			{
			    QF(n0, S0, S1, col0-CS2, surface);
			    QF(n0-S2+S1, S0, S2, col0-CS2, i1==(CK1-1));
			    QF(n0+S0-S2, S2, S1, col0-CS2, i0==(CK0-1));
			}
     
			col0 += CS0;
			n0   +=  S0;
		    }

		    col1 += CS1;
		    n1   +=  S1;
		 }

		 col2 += CS2, n2 += S2;
	    }

	}

    }

    return OK;
}


/*
 * Find out which plane has smallest delta z spacing along view
 * axis between planes, and render only faces in that plane (plus
 * the edge faces that form the boundary).
 */

static Error
VolumeRegularFace(struct buffer *b, struct xfield *xf, int clip)
{
    int i0, i1, i2, n, n0, n1, n2, col0, col1, col2;
    InvalidComponentHandle ich = xf->iElts;

    EXPAND_POSITIONS;
    orient(xf, 0);

    if (xf->colors_dep==dep_positions) {

	if (ich)
	{
	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    Q(n0, S1, S2, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    Q(n0, S0, S2, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }

	    /*
	     * Now all the faces perpendicular to axis 2 in back-to-front
	     * order
	     */
	    n2 = xf->n, col2 = xf->cn;
	    for (i2 = 0; i2 <= CK2; i2++)
	    {
	        int surface = i2 == 0 || i2 == CK2;

	        col1 = col2;
	        n1   = n2;

	        for (i1 = 0; i1 < CK1; i1++)
	        {
		    col0 = col1;
		    n0   = n1;

		    for (i0 = 0; i0 < CK0; i0++)
		    {
			if (i2 != 0 && DXIsElementInvalid(ich, col0-CS2))
			{
			    if (i0 != 0)
				Q(n0-S2, S2, S1, 0);
			    
			    if (i1 != 0)
				Q(n0-S2, S0, S2, 0);

			    Q_INVALID(n0, S0, S1, surface);
			    
			    if (i1 != (CK1-1))
				Q_INVALID(n0-S2+S1, S0, S2, 0);

			    if (i0 != (CK0-1))
				Q_INVALID(n0+S0-S2, S2, S1, 0);
			}
			else
			    Q(n0, S0, S1, surface);
     
			col0 += CS0;
			n0   +=  S0;
		    }

		    col1 += CS1;
		    n1   +=  S1;
		 }

		 col2 += CS2, n2 += S2;
	    }

	    /*
	     * front face perpendicular to axis 0
	     */
	    n1 = xf->n+CK0*S0, col1 = xf->cn+(CK0-1)*CS0;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    if (DXIsElementInvalid(ich, col0))
			Q_INVALID(n0, S1, S2, 1)
		    else
			Q(n0, S1, S2, 1);
 
		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    /*
	     * front face perpendicular to axis 1
	     */
	    n1 = xf->n+CK1*S1, col1 = xf->cn+(CK1-1)*CS1;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    if (DXIsElementInvalid(ich, col0))
			Q_INVALID(n0, S0, S2, 1)
		    else
			Q(n0, S0, S2, 1);
 
		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }
	}
	else
	{
	    FOR2 (K2) {
		FOR1 (K1-1)
		    FOR0 (K0-1)
			Q(n, S0, S1, i2==0 || i2==K2-1)
		if (i2 < K2-1) {
		    FOR1 (K1) {
			if (i1==0 || i1==K1-1)
			    FOR0 (K0-1)
				Q(n, S2, S0, 1)
			if (i1 < K1-1) {
			    Q(n1, S1, S2, 1)
			    Q(n1+(K0-1)*S0, S1, S2, 1)
			}
		    }
		}
	    }
	}

    } else if (xf->colors_dep==dep_connections) {

	if (ich)
	{
	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    QF(n0, S1, S2, col0, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    n1 = xf->n, col1 = xf->cn;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    QF(n0, S0, S2, col0, 1);

		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }

	    /*
	     * Now all the faces perpendicular to axis 2 in back-to-front
	     * order
	     */
	    n2 = xf->n, col2 = xf->cn;
	    for (i2 = 0; i2 <= CK2; i2++)
	    {
	        int surface = i2 == 0 || i2 == CK2;

	        col1 = col2;
	        n1   = n2;

	        for (i1 = 0; i1 < CK1; i1++)
	        {
		    col0 = col1;
		    n0   = n1;

		    for (i0 = 0; i0 < CK0; i0++)
		    {
			if (i2 != 0 && DXIsElementInvalid(ich, col0-CS2))
			{
			    if (i0 != 0)
				QF(n0-S2, S2, S1, col0-CS0, 0);
			    
			    if (i1 != 0)
				QF(n0-S2, S0, S2, col0-CS1, 0);

			    QF_INVALID(n0, S0, S1, col0-CS2, surface);
			    
			    if (i1 != (CK1-1))
				QF_INVALID(n0-S2+S1, S0, S2, col0-CS2, 0);

			    if (i0 != (CK0-1))
				QF_INVALID(n0+S0-S2, S2, S1, col0-CS2, 0);
			}
			else if (i2 == 0)
			    QF(n0, S0, S1, col0, surface)
			else
			    QF(n0, S0, S1, col0-CS2, surface);
     
			col0 += CS0;
			n0   +=  S0;
		    }

		    col1 += CS1;
		    n1   +=  S1;
		 }

		 col2 += CS2, n2 += S2;
	    }

	    /*
	     * front face perpendicular to axis 0
	     */
	    n1 = xf->n+CK0*S0, col1 = xf->cn+(CK0-1)*CS0;
	    for (i1 = 0; i1 < CK1; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    if (DXIsElementInvalid(ich, col0))
			QF_INVALID(n0, S1, S2, col0, 1)
		    else
			QF(n0, S1, S2, col0, 1);
 
		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS1;
		n1   +=  S1;
	    }

	    /*
	     * front face perpendicular to axis 1
	     */
	    n1 = xf->n+CK1*S1, col1 = xf->cn+(CK1-1)*CS1;
	    for (i1 = 0; i1 < CK0; i1++)
	    {
	        col0 = col1;
	        n0   = n1;

	        for (i0 = 0; i0 < CK2; i0++)
	        {
		    if (DXIsElementInvalid(ich, col0))
			QF_INVALID(n0, S0, S2, col0, 1)
		    else
			QF(n0, S0, S2, col0, 1);
 
		    col0 += CS2;
		    n0   +=  S2;
		}
		
		col1 += CS0;
		n1   +=  S0;
	    }
	}
	else
	{
	    col2 = xf->cn;
	    FOR2 (K2) {
		col1 = col2;
		FOR1 (K1-1) {
		    col0 = col1;
		    FOR0 (K0-1) {
			QF(n, S0, S1, col0, i2==0 || i2==K2-1)
			col0 += CS0;
		    }
		    col1 += CS1;
		}
		if (i2 < K2-1) {
		    col1 = col2;
		    FOR1 (K1) {
			if (i1==0 || i1==K1-1) {
			    col0 = col1;
			    FOR0 (K0-1) {
				QF(n, S2, S0, col0, 1)
				col0 += CS0;
			    }
			}
			if (i1 < K1-1) {
			    QF(n1, S1, S2, col1, 1)
			    QF(n1+(K0-1)*S0, S1, S2, col1+(CK0-1)*CS0, 1)
			}
			if (i1 < K1-2)
			    col1 += CS1;
		    }
		}
		if (i2 < K2-2)
		    col2 += CS2;
	    }
	}

    }

    return OK;
}


static Error
VolumeRegularPlane(struct buffer *buf, struct xfield *xf, int clip)
{
    int i2, n2, i;
    float dz, omul, cmul;
    Pointer op = xf->opacities;


#if 0
    if (xf->colors_dep==dep_connections)
	DXErrorReturn(ERROR_BAD_PARAMETER,
		    "hi-res volumes must have position-dependent colors");
#endif

    dz = orient(xf, 0);
    cmul = dz * xf->tile.color_multiplier;
    omul = dz * xf->tile.opacity_multiplier;

    /*
     * If connections-dep, copy conn-dep indices ->cn, ->ck, and ->cs
     * into ->n, ->k, and ->s and then do the same loop.
     * Couldn't do it earlier because we didn't know if we
     * would end up here or in say VolumeRegularFace which needs
     * both sets of numbers.  Also expand grid to fill the space.
     */
    if (xf->colors_dep==dep_connections) {
	D0 = DXMul(D0, (double)K0/(double)CK0);
	D1 = DXMul(D1, (double)K1/(double)CK1);
	D2 = DXMul(D2, (double)K2/(double)CK2);
	xf->n = xf->cn;
	for (i=0; i<3; i++) {
	    xf->k[i] = xf->ck[i];
	    xf->s[i] = xf->cs[i];
	}
    }

    FOR2 (K2) {
	if (i2==0) continue;
	_dxf_CompositePlane(buf,
			buf->ox + xf->o.x + i2*D2.x,
			buf->oy + xf->o.y + i2*D2.y,
			xf->o.z + i2*D2.z,
			clip,
			D0.x, D0.y, D0.z, D1.x, D1.y, D1.z,
			xf->cmap ?
			    xf->fcst ? 
				(Pointer)((unsigned char *)(xf->fcolors) + 0) :
				(Pointer)((unsigned char *)(xf->fcolors) + n2) :
			    xf->fcst ? 
				(Pointer)((RGBColor *)(xf->fcolors) + 0) :
				(Pointer)((RGBColor *)(xf->fcolors) + n2),
			xf->fcst, xf->cmap,
			xf->omap ? 
			    xf->ocst ? 
				(Pointer)(op? ((unsigned char *)op)+0 : NULL) : 
				(Pointer)(op? ((unsigned char *)op)+n2 : NULL) : 
			    xf->ocst ?
				(Pointer)(op? ((float *)op)+0 : NULL) :
				(Pointer)(op? ((float *)op)+n2 : NULL),
			xf->ocst, xf->omap,
			cmul, omul,
			S0, S1, K0, K1);
    }

    return OK;
}
