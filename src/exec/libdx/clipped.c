/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <string.h>
#include "clippedClass.h"
#include "internals.h"

/*
 * New..., DXDelete...
 */

Clipped
_NewClipped(Object render, Object clipping, struct clipped_class *class)
{
    Clipped c = (Clipped) _dxf_NewObject((struct object_class *)class);
    if (!c)
	return NULL;

    c->render = DXReference(render);
    c->clipping = DXReference(clipping);

    return c;
}

Clipped
DXNewClipped(Object render, Object clipping)
{
    return _NewClipped(render, clipping, &_dxdclipped_class);
}

Clipped
DXGetClippedInfo(Clipped c, Object *render, Object *clipping)
{
    CHECK(c, CLASS_CLIPPED);
    if (render)
	*render = c->render;
    if (clipping)
	*clipping = c->clipping;
    return c;
}


Clipped
DXSetClippedObjects(Clipped c, Object render, Object clipping)
{
    CHECK(c, CLASS_CLIPPED);
    if (render) {
	DXReference(render);
	DXDelete(c->render);
	c->render = render;
    }
    if (clipping) {
	DXReference(clipping);
	DXDelete(c->clipping);
	c->clipping = clipping;
    }
    return c;
}


int
_dxfClipped_Delete(Clipped c)
{
    DXDelete(c->render);
    DXDelete(c->clipping);
    return OK;
}


/*
 * DXCopy
 */

Object
_dxfClipped_Copy(Clipped old, enum _dxd_copy copy)
{
    Clipped new;
    Object render, clipping;

    /*
     * Do recursive copy?
     * NB - no difference between COPY_HEADER and COPY_ATTRIBUTES
     * here, because we can't create a clipped object w/o subparts
     */
    if (copy==COPY_HEADER || copy==COPY_ATTRIBUTES) {
	render = old->render;
	clipping = old->clipping;
    } else {
	render = DXCopy(old->render, copy);
	if (!render)
	    return NULL;
	clipping = DXCopy(old->clipping, copy);
	if (!clipping) {
	    DXDelete(render);
	    return NULL;
	}
    }

    /* make new object */
    new = DXNewClipped(render, clipping);
    if (!new) {
	DXDelete(render);
	DXDelete(clipping);
	return NULL;
    }

    /* copy superclass parts */
    return _dxf_CopyObject((Object)new, (Object)old, copy);
}



/*
 *  I seem to see that dx never uses the functions facenormals or outline.
 *  Thus they are if'd out.
 */

#if 0
static
void
facenormals(Field f)
{
    Array a;
    Point *points;
    RGBColor *colors;
    Vector *normals;
    Triangle *tris;
    int i, n;

    /* triangles */
    a = (Array) DXGetComponentValue(f, "connections");
    DXGetArrayInfo(a, &n, NULL, NULL, NULL, NULL);
    tris = (Triangle *) DXGetArrayData(a);

    /* points */
    a = (Array) DXGetComponentValue(f, "positions");
    points = (Point *) DXGetArrayData(a);

    /* colors */
    a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    DXAddArrayData(a, 0, n, NULL);
    colors = (RGBColor *) DXGetArrayData(a);
    for (i=0; i<n; i++)
	colors[i] = DXRGB(.5,.5,1);
    DXSetComponentValue(f, COLORS, (Object) a);
    DXSetAttribute((Object)a, DEP, O_CONNECTIONS);
    
    /* normals */
    a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    DXAddArrayData(a, 0, n, NULL);
    normals = (Vector *) DXGetArrayData(a);
    for (i=0; i<n; i++) {
	Triangle t;
	Vector a, b, c;
	t = tris[i];
	a = DXSub(points[t.q],points[t.p]);
	b = DXSub(points[t.r],points[t.p]);
	c = DXCross(a,b);
	normals[i] = DXNormalize(c);
    }
    DXSetComponentValue(f, NORMALS, (Object)a);
    DXSetAttribute((Object)a, DEP, O_CONNECTIONS);
}
#endif

#if 0

static
Field
outline(Object o)
{
    Field f;
    Point box[8];
    int i;

    f = DXNewField();
    DXBoundingBox(o, box);
    DXAddPoints(f, 0, 8, box);
    for (i=0; i<8; i++)
	DXAddColor(f, i, DXRGB(0,1,1));
    i = 0;
    DXAddLine(f, i++, DXLn(0,1));
    DXAddLine(f, i++, DXLn(0,2));
    DXAddLine(f, i++, DXLn(0,4));
    DXAddLine(f, i++, DXLn(1,3));
    DXAddLine(f, i++, DXLn(1,5));
    DXAddLine(f, i++, DXLn(2,3));
    DXAddLine(f, i++, DXLn(2,6));
    DXAddLine(f, i++, DXLn(3,7));
    DXAddLine(f, i++, DXLn(4,5));
    DXAddLine(f, i++, DXLn(4,6));
    DXAddLine(f, i++, DXLn(5,7));
    DXAddLine(f, i++, DXLn(6,7));

    return DXEndField(f);
}
#endif


/*
 * front faces     back faces
 *   Y                     -Z
 *    2---3            2---3
 *   / \ a \          / b / \
 *  6 c 0---1 X   -X 6---7 f 1
 *   \ / e /          \ d \ /
 *    4---5            4---5
 *   Z                     -Y
 *
 * edge #   endpoints   faces
 *    0        01       a       e  
 *    1        13       a         f
 *    2        73         b       f
 *    3        32       a b              
 *    4        02       a   c           
 *    5        26         b c          
 *    6        76         b   d                 
 *    7        64           c d   
 *    8        04           c   e
 *    9        45             d e
 *   10        75             d   f
 *   11        51               e f
 *
 */

#define X 1
#define Y 2
#define Z 4

/*
 * The following macros let us specify a face by naming a
 * series of point indices.
 */

#define BEGIN {							\
    first = last = -1;						\
    DXDebug("C", "FACE");						\
}

#define POINT(n) {						\
    if (last>=0) {						\
	if (debug)						\
	    DXAddLine(clip, elements++, DXLn(last,n));		\
	else if (first>=0)					\
	    DXAddTriangle(clip, elements++, DXTri(first,last,n));	\
    } else							\
	first = n;						\
    last = n;							\
    DXDebug("C", "    POINT %d",n);				\
}

#define END {							\
    if (debug && first>=0)					\
	DXAddLine(clip, elements++, DXLn(last,first));		\
}

/*
 * This macro checks edge #e, going from point a to point b,
 * to see if it crosses the plane; if so, it adds a new point
 * and records in the crossings array the point number for
 * that edge crossing.  Note that the point number are specified
 * assuming a nearPlane point of 0, thus we must xor the specified point
 * numbers a and b with nearPlane.
 */

#define CROSSING(e,a,b) {	/* edge e, endpoints a and b */		\
    Point pa, pb;							\
    float da, db, t;							\
    pa = points[nearPlane^(a)];	/* point a, relative to nearPlane */	\
    pb = points[nearPlane^(b)];	/* point b, relative to nearPlane */	\
    da = DXDot(pa, n);		/* projection of a on normal */		\
    db = DXDot(pb, n);		/* projection of b on normal */		\
    if (db!=da) {		/* XXX ABS((d-da)/(db-da))<DXD_MAX_FLOAT */	\
	t = (d-da) / (db-da);	/* compare w/ projection of p */	\
	if (0<=t && t<=1) {	/* is there a crossing? */		\
	    points[npoints] = DXAdd(DXMul(pb,t), DXMul(pa,(1-t)));	\
	    crossings[(e)] = npoints;					\
	    POINT(npoints);						\
	    npoints++;							\
        }								\
    }									\
}

/*
 * The following macros check alternately in order the vertices and
 * edges of a face; the vertices are included if they are on the correct
 * side of the plane; if the edges cross the plane, the corresponding
 * point is included.  The face is specified by points nearPlane^start,
 * nearPlane^start^x, nearPlane^start^x^y, nearPlane^start^y; and by edges e, e+1,
 * e+3, and e+4, mod 12.
 */

#define CORNER(a) 							    \
    if (DXDot(points[a],n)>d)						    \
	POINT(a)							    \
    else								    \
        DXDebug("C", "    (skipping %d: %.2g<=%.2g)",a,DXDot(points[a],n),d);   \

#define EDGE(e) if (crossings[e]>=0) POINT(crossings[e])

#define FACE(start,x,y,e) {	\
    BEGIN;			\
    CORNER(nearPlane^start);		\
    EDGE((e)%12);		\
    CORNER(nearPlane^start^x);	\
    EDGE((e+1)%12);		\
    CORNER(nearPlane^start^x^y);	\
    EDGE((e+3)%12);		\
    CORNER(nearPlane^start^y);	\
    EDGE((e+4)%12);		\
    END;			\
}

static Vector
perp(Vector *d, Vector *a, Vector *b)
{
    Vector c;
    c.x = d->x;
    c.y = d->y;
    c.z = d->z;
    if (c.x!=0) 
	c.x*=.02,d->x=c.x, a->x=0,a->y=c.x,a->z=0, b->x=0,b->y=0,b->z=c.x;
    else if (c.y!=0) 
	c.y*=.02,d->y=c.y, a->x=0,a->y=0,a->z=c.y, b->x=c.y,b->y=0,b->z=0;
    else if (c.z!=0) 
	c.z*=.02,d->z=c.z, a->x=c.z,a->y=0,a->z=0, b->x=0,b->y=c.z,b->z=0;

    return c;
}

Object
DXClipPlane(Object o, Point p, Vector n)
{
    Field clip;			/* resulting clipping solid */
    Point points[20];		/* up to 14: 8 original + 6 crossings */
    int npoints;		/* actual number of points */
    int crossings[12];		/* point num for each of 12 edge crossings */
    int first, last;		/* first, last point num on current face */
    int elements = 0;		/* number of triangles so far */
    int nearPlane = 0;			/* maximal box corner in direction of n */
    Vector v1, v2, v4;
    float max, d, l1, l2, l4;
    int i, debug;
    ConstantArray array;

    /* change the sense of the normal */
    n = DXNeg(n);

    /* check the debug flag */
    debug = DXGetAttribute(o, "debug clipping")? 1 : 0;

    /* get 8 points of box */
    if (!DXBoundingBox(o, points)) {
	if (DXGetError()==ERROR_NONE)
	    return o;
	else {
	    DXAddMessage("bad input object");
	    return NULL;
	}
    }
    npoints = 8;


    /*
     * Expand box slightly to avoid problems w/ planes on face of box,
     * taking account of the possibility of a zero-length side.
     * XXX - there must be a better way
     */
    v1 = DXSub(points[1],points[0]);
    v2 = DXSub(points[2],points[0]);
    v4 = DXSub(points[4],points[0]);
    d = DXLength(DXSub(points[7], points[0])) * .02;
    l1 = DXLength(v1);
    l2 = DXLength(v2);
    l4 = DXLength(v4);
    if (l2==0 && l4==0)
	perp(&v1, &v2, &v4);
    else if (l1==0 && l4==0)
	perp(&v2, &v4, &v1);
    else if (l1==0 && l2==0)
	perp(&v4, &v1, &v2);
    else {
	v1 = DXMul(DXNormalize(l1>0 ? v1 : DXCross(v2,v4)), d);
	v2 = DXMul(DXNormalize(l2>0 ? v2 : DXCross(v4,v1)), d);
	v4 = DXMul(DXNormalize(l4>0 ? v4 : DXCross(v1,v2)), d);
    }
    for (i=0; i<8; i++) {
	points[i] = i&1? DXAdd(points[i], v1) : DXSub(points[i], v1);
	points[i] = i&2? DXAdd(points[i], v2) : DXSub(points[i], v2);
	points[i] = i&4? DXAdd(points[i], v4) : DXSub(points[i], v4);
    }

#if 0
    /* expand box slightly to avoid problems w/ planes on face of box */
    for (i=0; i<8; i++) {
	Point a, b;
	a = points[i];
	b = points[i^7];
	points[i] = DXAdd(a, DXMul(DXSub(a,b),.05));
    }
#endif

    /* find maximal point in direction of normal to plane */
    for (i=0, max= -DXD_MAX_FLOAT; i<8; i++) {
	float l = DXDot(n, points[i]);
	if (l>max) {
	    max = l;
	    nearPlane = i;
	}
    }
    DXDebug("C", "nearPlane %d", nearPlane);

    /* field to hold result */
    clip = DXNewField();
    if (!clip)
	return NULL;

    /* start out with no crossings */
    for (i=0; i<12; i++)
	crossings[i] = -1;

    /* used by CROSSING macro */
    d = DXDot(p, n);

    /* check 12 edges for crossings, constructing face as we go */
    BEGIN;
    CROSSING( 0, 0,1);
    CROSSING( 1, 1,3);
    CROSSING( 2, 7,3);
    CROSSING( 3, 3,2);
    CROSSING( 4, 0,2);
    CROSSING( 5, 2,6);
    CROSSING( 6, 7,6);
    CROSSING( 7, 6,4);
    CROSSING( 8, 0,4);
    CROSSING( 9, 4,5);
    CROSSING(10, 7,5);
    CROSSING(11, 5,1);
    END;

    /* six faces */
    FACE(0,X,Y, 0);
    FACE(0,Y,Z, 4);
    FACE(0,Z,X, 8);
    FACE(7,Z,X, 2);
    FACE(7,X,Y, 6);
    FACE(7,Y,Z, 10);

    /* we're done now */
    DXAddPoints(clip, 0, npoints, points);
    if (debug)
	for (i=0; i<npoints; i++)
	    DXAddColor(clip, i, DXRGB(0,1,1));


    array = DXNewConstantArray(1,(Pointer)&p,TYPE_FLOAT,CATEGORY_REAL,1,3);
    DXSetAttribute((Object)clip,"clipplane point",(Object)array);

    /* record the normal with its original sign */
    n = DXNeg(n);

    array = DXNewConstantArray(1,(Pointer)&n,TYPE_FLOAT,CATEGORY_REAL,1,3);
    DXSetAttribute((Object)clip,"clipplane normal",(Object)array);

    DXEndField(clip);

    /* what to return */
    if (debug) {
	Group g = DXNewGroup();
	DXSetMember(g, NULL, o);
	DXSetMember(g, NULL, (Object)clip);
	return (Object) g;
    } else
	return (Object)DXNewClipped(o, (Object)clip);
}


#define QUAD(p,x,y)	\
    BEGIN;		\
    POINT(p);		\
    POINT(p^x);		\
    POINT(p^x^y);	\
    POINT(p^y);		\
    END;


Object
DXClipBox(Object o, Point p1, Point p2)
{
    int first, last, elements = 0, debug = 0, i;
    Point points[8];
    Field clip;
    ConstantArray array;

    /* check the debug flag */
    debug = DXGetAttribute(o, "debug clipping")? 1 : 0;

    /* a field to hold the box */
    clip = DXNewField();
    if (!clip)
	return NULL;

    /* add eight points */
    points[0] = DXPt(p1.x,p1.y,p1.z);
    points[1] = DXPt(p1.x,p1.y,p2.z);
    points[2] = DXPt(p1.x,p2.y,p1.z);
    points[3] = DXPt(p1.x,p2.y,p2.z);
    points[4] = DXPt(p2.x,p1.y,p1.z);
    points[5] = DXPt(p2.x,p1.y,p2.z);
    points[6] = DXPt(p2.x,p2.y,p1.z);
    points[7] = DXPt(p2.x,p2.y,p2.z);
    DXAddPoints(clip, 0, 8, points);

    /* colors for debug */
    if (debug)
	for (i=0; i<8; i++)
	    DXAddColor(clip, i, DXRGB(0,1,1));

    /* and six quads */
    QUAD(0,X,Y);
    QUAD(0,Y,Z);
    QUAD(0,Z,X);
    QUAD(7,Z,X);
    QUAD(7,X,Y);
    QUAD(7,Y,Z);

    array = DXNewConstantArray(1,(Pointer)&p1,TYPE_FLOAT,CATEGORY_REAL,1,3);
    DXSetAttribute((Object)clip,"clipbox min",(Object)array);

    array = DXNewConstantArray(1,(Pointer)&p2,TYPE_FLOAT,CATEGORY_REAL,1,3);
    DXSetAttribute((Object)clip,"clipbox max",(Object)array);

    /* done defining field */
    DXEndField(clip);

    /* what to return */
    if (debug) {
	Group g = DXNewGroup();
	DXSetMember(g, NULL, o);
	DXSetMember(g, NULL, (Object)clip);
	return (Object) g;
    } else
	return (Object)DXNewClipped(o, (Object)clip);

}

