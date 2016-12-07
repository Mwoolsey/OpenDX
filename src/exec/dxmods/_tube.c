/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "vectors.h"
#include "_post.h"

#define RIBBON 2   /* ribbon is 2-gon (sort of) */

#define TOLERANCE_ANGLE	45.0

typedef struct
{
    int head, seg;
} PathElt;

static Field TubeArrays_Reg(Field, int, int);
static Error TracePath(Vector *, PathElt *, int, Vector **, Vector **);

#define O_CONNECTIONS _o_connections;

static Field TubeIrregular(Field, double, int, float *, float *, float);
static Field TubePolyline(Field, double, int, float *, float *, float);

static Field RemoveFieldComponents(Field);
static Array CvtPositions(Field);
static Field TubeField(Field, double, int, float *, float *, float);

static
Field
TubeRegular(Field f, double diameter, int ngon, float *cs, float *sn)
{
    int i, j, k, l, npoints, nnewpoints, nnewconn, max;
    Array a = NULL, oPts = NULL, nPts = NULL, nQuads = NULL, nNrms = NULL;
    Point *points, *newpoints;
    Vector *normals=NULL, *newnormals, *binormals=NULL;
    Quadrilateral *quads, *t;
    int  own_binormals = 0, own_normals = 0;

    /* get points */
    oPts = (Array) DXGetComponentValue(f, "positions");
    if (!oPts)
	return f;

    /* if it's not an empty field make sure the connections are ok before
     * going any further.
     */
    a = DXGetConnections(f, "lines");
    if (!a)
    {
	if (!DXGetComponentValue(f, "connections"))
	    DXSetError(ERROR_MISSING_DATA, "connections component");
	else
	    DXSetError(ERROR_DATA_INVALID, "element type must be lines");
	return NULL;
    }

    /* now back to the points */
    if (! DXGetArrayInfo(oPts, &npoints, NULL, NULL, NULL, NULL))
	return NULL;
    
    if (npoints<2)
	return f;
    
    if (! DXTypeCheck(oPts, TYPE_FLOAT, CATEGORY_REAL, 1, 3))
    {
	oPts = CvtPositions(f);
	if (! oPts)
	    goto error;
    }

    DXReference((Object)oPts);

    points = (Point *)DXGetArrayData(oPts);
    if (! points)
	goto error;

    for (i = 0; i < npoints-1; i++)
    {
	Vector *p = (Vector *)(points+i);
	Vector *q = (Vector *)(p+1);
	Vector d;

	vector_subtract(p, q, &d);
	if (! vector_zero(&d))
	    break;
    }

    if (i == npoints-1)
        return RemoveFieldComponents(f);

    nnewpoints = ngon * npoints;
    /*nconn = npoints-1;*/
    nnewconn = (ngon==RIBBON? 1 : ngon) * (npoints-1);

    /*
     * get binormals if there are any
     */
    a = (Array) DXGetComponentValue(f, "binormals");
    if (a)
    {
	int k;

	if (! DXTypeCheck(a, TYPE_FLOAT, CATEGORY_REAL, 1, 3))
	{
	    DXSetError(ERROR_DATA_INVALID, "bad binormals");
	    goto error;
	}

	if (! DXGetArrayInfo(a, &k, NULL, NULL, NULL, NULL))
	{
	    DXSetError(ERROR_DATA_INVALID, "binormals count != positions count");
	    goto error;
	}

	binormals = (Vector *)DXGetArrayData(a);
	if (! binormals)
	    goto error;
    }
	    
    /*
     * get normals if there are any
     */
    a = (Array) DXGetComponentValue(f, "normals");
    if (a)
    {
	int k;

	if (! DXTypeCheck(a, TYPE_FLOAT, CATEGORY_REAL, 1, 3))
	{
	    DXSetError(ERROR_DATA_INVALID, "bad normals");
	    goto error;
	}

	if (! DXGetArrayInfo(a, &k, NULL, NULL, NULL, NULL))
	{
	    DXSetError(ERROR_DATA_INVALID, "normals count != positions count");
	    goto error;
	}

	normals = (Vector *)DXGetArrayData(a);
	if (! normals)
	    goto error;
    }
	    
    /*
     * If the data is < 3D, use the additional dimension as the normal
     */
    if (! normals && ! binormals)
    {
	Point box[8], min, max;
	int i, doit;
	float normal[3];

	normal[0] = normal[1] = normal[2] = 0.0;

	if (! DXBoundingBox((Object)f, box))
	    return NULL;
	
	min.x = min.y = min.z =  DXD_MAX_FLOAT;
	max.x = max.y = max.z = -DXD_MAX_FLOAT;
	for (i = 0; i < 8; i++)
	{
	    if (box[i].x < min.x) min.x = box[i].x;
	    if (box[i].x > max.x) max.x = box[i].x;
	    if (box[i].y < min.y) min.y = box[i].y;
	    if (box[i].y > max.y) max.y = box[i].y;
	    if (box[i].z < min.z) min.z = box[i].z;
	    if (box[i].z > max.z) max.z = box[i].z;
	}

	if (min.z == max.z)
	{
	    normal[2] = 1.0;
	    doit = 1;
	}
	else if (min.y == max.y)
	{
	    normal[1] = 1.0;
	    doit = 1;
	}
	else if (min.x == max.x)
	{
	    normal[0] = 1.0;
	    doit = 1;
	}
	else
	    doit = 0;
	
	if (doit)
	{
	    own_normals = 1;
	    normals = (Vector *)DXAllocate(npoints * sizeof(Vector));
	    if (! normals)
		goto error;
	    
	    for (i = 0; i < npoints; i++)
		memcpy((char *)(normals + i), (char *)normal, sizeof(normal));
	}
    }
    
    if (normals && binormals)
    {
	/*
	 * then check the handed-ness of the coordinate system
	 */
	Vector tan;
	Vector cross;

	vector_subtract(points+1, points, &tan);
	vector_cross(normals, &tan, &cross);
	
	if (vector_dot(&cross, binormals) < 0)
	    diameter = -diameter;
    }
    else if (normals && ! binormals)
    {
	Vector tan;
	own_binormals = 1;
	binormals = (Vector *)DXAllocate(npoints * sizeof(Vector));
	if (! binormals)
	    goto error;
	for (i = 0; i < npoints; i++)
	{
	    if (i == 0)
		vector_subtract(points+1, points, &tan);
	    else if (i == (npoints-1))
		vector_subtract(points+i, points+(i-1), &tan);
	    else
		vector_subtract(points+i, points+(i-1), &tan);
	    vector_cross(normals+i, &tan, binormals+i);
	    vector_normalize(binormals+i, binormals+i);
	}
    }
    else if (! normals && binormals)
    {
	Vector tan;
	own_normals = 1;
	normals = (Vector *)DXAllocate(npoints * sizeof(Vector));
	if (! normals)
	    goto error;
	for (i = 0; i < npoints; i++)
	{
	    if (i == 0)
		vector_subtract(points+1, points, &tan);
	    else if (i == (npoints-1))
		vector_subtract(points+i, points+(i-1), &tan);
	    else
		vector_subtract(points+i, points+(i-1), &tan);
	    vector_cross(&tan, binormals+i, normals+i);
	    vector_normalize(normals+i, normals+i);
	}
    }
    else if (! normals && ! binormals)
    {
	own_normals = own_binormals = 1;

	if (! TracePath(points, NULL, npoints, &normals, &binormals))
	    return NULL;
    }

    /* new points array */
    nPts = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if ( ! nPts)
	goto error;

    if (! DXAddArrayData(nPts, 0, nnewpoints, NULL))
	goto error;

    newpoints = (Point *)DXGetArrayData(nPts);
    if (! newpoints)
	goto error;

    /* new normals array */
    nNrms = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! nNrms)
	goto error;

    if (! DXAddArrayData(nNrms, 0, nnewpoints, NULL))
	goto error;

    newnormals = (Vector *) DXGetArrayData(nNrms);
    if (! newnormals)
	goto error;

    /* new quads array */
    nQuads = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
    if (! nQuads)
	goto error;

    if (! DXAddArrayData(nQuads, 0, nnewconn, NULL))
	goto error;
	
    quads = (Quadrilateral*)DXGetArrayData(nQuads);
    if (! quads)
	goto error;
    
    /* new points and normals */
    if (ngon==RIBBON) {				/* ribbon */
	for (i=0; i<npoints; i++) {
	    Vector w;
	    w = DXMul(binormals[i], diameter/2);
	    newpoints[i] = DXSub(points[i], w);
	    newpoints[i+npoints] = DXAdd(points[i], w);
	    newnormals[i] = normals[i];
	    newnormals[i+npoints] = normals[i];
	}
    } else {					/* tube */
	for (i=0; i<npoints; i++) {
	    for (j=i, k=0; k<ngon; j+=npoints, k++) {
		Vector normal;
		normal = DXAdd(DXMul(normals[i],cs[2*k]),
			     DXMul(binormals[i],sn[2*k]));
		newnormals[j] =  normal;
		newpoints[j] = DXAdd(points[i], DXMul(normal,diameter/2));
	    }
	}
    }

    /* new quads - order is important for dep connections */
    max = (ngon==RIBBON? 1 : ngon) * npoints;    
    for (t=quads, j=0; j<max; j+=npoints) {
	for (i=0; i<npoints-1; i++) {
	    k = i + j;
	    t->p = k;
	    t->q = (l=k+npoints) < nnewpoints ? l : l-nnewpoints;
	    t->r = k+1;
	    t->s = (l=k+npoints+1) < nnewpoints ? l : l-nnewpoints;
	    t++;  
	}
    }

    if (! TubeArrays_Reg(f, ngon, npoints))
	goto error;

    DXSetComponentValue(f, "positions", (Object)nPts); nPts = NULL;
    DXSetComponentValue(f, "normals", (Object)nNrms); nNrms = NULL;
    if (!DXSetConnections(f, "quads", nQuads))
	return NULL;
    nQuads = NULL;

    /*
     * add default colors if necessary
     */
    if (!DXGetComponentValue(f, "colors") &&
        !DXGetComponentValue(f, "front colors") &&
	!DXGetComponentValue(f, "back colors")
    ) {
	static RGBColor col = {.7, .7, 0};
	a = (Array)DXNewConstantArray(nnewpoints, (Pointer)&col,
					TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	DXSetComponentValue(f, "colors", (Object)a); /* ok because f is a copy */
    }

    if (DXGetAttribute((Object)f, "shade"))
	if (! DXSetAttribute((Object)f, "shade", NULL))
	    goto error;

    if (! DXEndField(f))
	goto error;

    if (own_binormals)
	DXFree((Pointer)binormals);
    if (own_normals)
	DXFree((Pointer)normals);

    DXDelete((Object)oPts);

    return f;

error:
    DXDelete((Object)oPts);
    DXDelete((Object)nPts);
    DXDelete((Object)nNrms);
    DXDelete((Object)nQuads);
    if (own_normals)
	DXFree((Pointer)normals);
    if (own_binormals)
	DXFree((Pointer)binormals);
    
    return NULL;
}

static
Object
_Tube(Object o, double diameter, int ngon, float *cs, float *sn, float t)
{
    int i;
    Object sub;

    switch (DXGetObjectClass(o)) {
    case CLASS_GROUP:
	for (i=0; (sub=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
	    if (!_Tube(sub, diameter, ngon, cs, sn, t))
		return NULL;
	return o;
	break;
    case CLASS_FIELD:
	return (Object)TubeField((Field)o, diameter, ngon, cs, sn, t);
	break;
    case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &sub, NULL))
	    return NULL;
	if (!_Tube(sub, diameter, ngon, cs, sn, t))
	    return NULL;
	return o;
	break;
    case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &sub, NULL, NULL))
	    return NULL;
	if (!_Tube(sub, diameter, ngon, cs, sn, t))
	    return NULL;
	return o;
	break;
    default:
	DXErrorReturn(ERROR_BAD_CLASS, "object has an unsupported class");
    }
}

static Field
TubeField(Field f, double d, int n, float *cs, float *sn, float t)
{
    Object attr;
    Array array = NULL;
    Field tube = NULL;

    if (DXGetComponentValue(f, "normals"))
    {
	attr = DXGetComponentAttribute(f, "normals", "dep");
	if (! attr || DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"missing or invalid normals dependency attribute");
	    return NULL;
	}

	if (!strcmp(DXGetString((String)attr), "connections"))
	{
	    array = _dxfPostArray(f, "normals", "positions");
	    if (! array)
		goto error;
	    
	    if (! DXSetComponentValue(f, "normals", (Object)array))
		goto error;
	    array = NULL;
	
	    if (! DXSetComponentAttribute(f, "normals", "dep",
			DXGetComponentAttribute(f, "positions", "dep")))
		goto error;
	}
    }

    if (DXGetComponentValue(f, "binormals"))
    {
	attr = DXGetComponentAttribute(f, "binormals", "dep");
	if (! attr || DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"missing or invalid binormals dependency attribute");
	    return NULL;
	}

	if (!strcmp(DXGetString((String)attr), "connections"))
	{
	    array = _dxfPostArray(f, "binormals", "positions");
	    if (! array)
		goto error;
	    
	    if (! DXSetComponentValue(f, "binormals", (Object)array))
		goto error;
	    array = NULL;
	
	    if (! DXSetComponentAttribute(f, "binormals", "dep",
			DXGetComponentAttribute(f, "positions", "dep")))
		goto error;
	}
    }

    if (DXGetComponentValue((Field)f, "polylines"))
    {
	    tube = TubePolyline(f, d, n, cs, sn, t);
    }
    else if (DXQueryGridConnections((Array)DXGetComponentValue((Field)f,
	  "connections"), NULL, NULL) &&
	  !DXGetComponentValue((Field)f, "invalid connections") &&
	  !DXGetComponentValue((Field)f, "invalid positions"))
	    tube = TubeRegular(f, d, n, cs, sn);
	else
	    tube = TubeIrregular(f, d, n, cs, sn, t);
    
    if (! tube)
	goto error;
    
    return f;

error:
    DXDelete((Object)array);
    return NULL;
}

Object
DXTube(Object input, double diameter, int ngon)
{
    Object o = NULL;
    float *cs = NULL, *sn = NULL;
    int i;
    float t = cos(TOLERANCE_ANGLE);

    /* check */
    if (ngon <= 1)
	DXErrorReturn(ERROR_BAD_PARAMETER, "1 or fewer faces in Tube");

    /* copy object */
    o = DXCopy(input, COPY_STRUCTURE);
    if (!o)
	return NULL;
    
    if (diameter < 0)
    {
	Point       box[8];

	if (! DXInvalidateUnreferencedPositions(o))
	    DXErrorReturn(ERROR_INTERNAL, 
		"invalidating positions in DXTube");
	
	if (!DXValidPositionsBoundingBox((Object)o, box))
		    DXErrorReturn(ERROR_BAD_PARAMETER,
				    "object has no bounding box");

	diameter = DXLength(DXSub(box[7],box[0])) / 50.0;
    }

    /* change this to change the shape */
    cs = (float *) DXAllocateLocal(2*ngon*sizeof(float));
    sn = (float *) DXAllocateLocal(2*ngon*sizeof(float));
    if (!cs || !sn)
	goto error;
    for (i=0; i<2*ngon; i++) {
	cs[i] = cos(2*M_PI*i/ngon/2);
	sn[i] = sin(2*M_PI*i/ngon/2);
    }

    /* compute tube */
    if (! _Tube(o, diameter, ngon, cs, sn, t))
	goto error;
    
    if (ngon == 2)
	DXSetFloatAttribute(o, "Ribbon width", diameter);
    else
	DXSetFloatAttribute(o, "Tube diameter", diameter);

    /* free and return */
    DXFree((Pointer)cs);
    DXFree((Pointer)sn);
    return o;

error:

    if (o != input)
	DXDelete(o);
    DXFree((Pointer)cs);
    DXFree((Pointer)sn);
    return NULL;
}


Object
DXRibbon(Object o, double width)
{
    return DXTube(o, width, RIBBON);
}

static Field
TubeArrays_Reg(Field field, int ngon, int nPoints)
{
    Array s, d = NULL;
    Object att;
    char *name, *toDelete[100];
    int i, nDelete;

    /*nQuads = (ngon == 2) ? 1 : ngon;*/

    i = nDelete = 0;
    while ((s = (Array)DXGetEnumeratedComponentValue(field, i++, &name)) != NULL)
    {
	if (!strcmp(name, "positions")   ||
	    !strcmp(name, "connections") ||
	    !strcmp(name, "normals")     ||
	    !strcmp(name, "opacity map") ||
	    !strcmp(name, "color map"))
		continue;
	
	if (!strcmp(name, "binormals") ||
	    !strcmp(name, "tangents"))
	{
	    toDelete[nDelete++] = name;
	    continue;
	}


	if (DXGetComponentAttribute(field, name, "ref") ||
	    DXGetComponentAttribute(field, name, "der"))
	{
		toDelete[nDelete++] = name;
		continue;
	}

	att = DXGetComponentAttribute(field, name, "dep");
	if (! att) 
	{
		toDelete[nDelete++] = name;
		continue;
	}

	if (DXGetObjectClass(att) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, "dependency attribute on %s", name);
	    goto error;
	}

	if (! strcmp(DXGetString((String)att), "positions"))
	{
	    Type t;
	    Category c;
	    int j, r, shape[32], size;
	    char *dPtr, *sPtr = (char *)DXGetArrayData(s);

	    DXGetArrayInfo(s, NULL, &t, &c, &r, shape);

	    d = DXNewArrayV(t, c, r, shape);
	    if (! d)
		goto error;
	    
	    if (! DXAddArrayData(d, 0, ngon*nPoints, NULL))
		goto error;
	    
	    size = DXGetItemSize(s);
	    dPtr = (char *)DXGetArrayData(d);

	    for (j = 0; j < ngon; j++)
	    {
		memcpy(dPtr, sPtr, nPoints*size);
		dPtr += nPoints*size;
	    }
	}
	else if (! strcmp(DXGetString((String)att), "connections"))
	{
	    Type t;
	    Category c;
	    int j, r, shape[32], size;
	    char *dPtr, *sPtr = (char *)DXGetArrayData(s);
	    int nTri = (ngon == 2) ? 1 : ngon;

	    DXGetArrayInfo(s, NULL, &t, &c, &r, shape);

	    d = DXNewArrayV(t, c, r, shape);
	    if (! d)
		goto error;
	    
	    if (! DXAddArrayData(d, 0, nTri*(nPoints-1), NULL))
		goto error;
	    
	    size = DXGetItemSize(s);
	    dPtr = (char *)DXGetArrayData(d);

	    for (j = 0; j < nTri; j++)
	    {
		memcpy(dPtr, sPtr, (nPoints-1)*size);
		dPtr += (nPoints-1)*size;
	    }
	}

	if (! DXSetComponentValue(field, name, (Object)d))
	    goto error;
	
	d = NULL;
    }

    for (i = 0; i < nDelete; i++)
	DXDeleteComponent(field, toDelete[i]);
    
    return field;

error:
    DXDelete((Object)d);
    return NULL;
}

static Error InitFrame(Vector *, PathElt *, int, Vector *,
				Vector *, Vector *, Vector *, Vector *);
static Error UpdateFrame(Vector *, Vector *, Vector *, Vector *, Vector *, 
						    Vector *, Vector *);
static void RotateAroundVector(Vector *, float, float, float *);

static Error
TracePath(Vector *points, PathElt *path, int npoints, 
				Vector **nPtr, Vector **bPtr)
{
    Vector *normals = NULL;
    Vector *binormals = NULL;
    int    i;
    Vector fn, fb, tangent;
    int loop, nUniquePoints;

    if (path && (path[0].head == path[npoints-1].head))
	loop = 1;
    else  
	loop = 0;

    nUniquePoints = loop ? npoints-1 : npoints;

    *nPtr = normals = (Vector *)DXAllocate(nUniquePoints*sizeof(Vector));
    if (! normals)
	goto error;
    
    *bPtr = binormals = (Vector *)DXAllocate(nUniquePoints*sizeof(Vector));
    if (! binormals)
	goto error;
    
    if (! InitFrame(points, path, npoints, &tangent, 
				normals, binormals, &fn, &fb))
    {
	DXFree((Pointer)normals);   *nPtr = NULL;
	DXFree((Pointer)binormals); *bPtr = NULL;
	return ERROR;
    }
    
    for (i = 1; i < nUniquePoints-1; i++)
    {
	Vector *p, *q;

	if (path)
	{
	    p = points + path[i].head;
	    q = points + path[i+1].head;
	}
	else
	{
	    p = points + i;
	    q = points + i + 1;
	}

	normals ++; binormals ++;

	if (! UpdateFrame(p, q, &tangent, normals, binormals, &fn, &fb))
	    goto error;
    }

    if (loop) /* can oly be set if path or indices is non-null */
    {
	Vector *p=NULL, *q=NULL;

	if (path)
	{
	    p = points + path[nUniquePoints-1].head;
	    q = points + path[0].head;
	}
	
	normals ++; binormals ++;

	if (! UpdateFrame(p, q, &tangent, normals, binormals, &fn, &fb))
	    goto error;
    }
    else
    {
	normals[1]   = fn;
	binormals[1] = fb;
    }

    return OK;

error:

    DXFree((Pointer)normals);
    DXFree((Pointer)binormals);

    *nPtr = NULL;
    *bPtr = NULL;

    return ERROR;
}

static Error
InitFrame(Vector *pts, PathElt *path, int np, Vector *t,
				Vector *n, Vector *b, Vector *fn, Vector *fb)
{
    int i;
    int loop;

    loop = (path && (path[0].head == path[np-1].head));

    for (i = 0; i < np-1; i++)
    {
	Vector *p, *q;

	if (path)
	{
	    if (i == 0)
		if (loop)
		    p = pts + path[np-2].head;
		else
		    p = pts + path[0].head;
	    else
		p = pts + path[i-1].head;

	    if (i == np-1)
		if (loop)
		    q = pts + path[1].head;
		else
		    q = pts + path[i].head;
	    else
		q = pts + path[i+1].head;
	}
	else
	{
	    p = pts + i;
	    q = pts + i + 1;
	}

	_dxfvector_subtract_3D(q, p, t);

	if (t->x != 0.0 || t->y != 0.0 || t->z != 0.0)
	    break;
    }

    if (i == np-1)
	return ERROR;

    _dxfvector_normalize_3D(t, t);
    
    n->x = n->y = n->z = 0.0;
    if (t->x == 0.0 && t->y == 0.0 && t->z > 0.0)
	n->x = 1.0;
    else if (t->x == 0.0 && t->y == 0.0 && t->z < 0.0)
	n->x = -1.0;
    else if (t->x == 0.0 && t->y > 0.0 && t->z == 0.0)
	n->z = 1.0;
    else if (t->x == 0.0 && t->y < 0.0 && t->z == 0.0)
	n->z = -1.0;
    else if (t->x > 0.0 && t->y == 0.0 && t->z == 0.0)
	n->y = 1.0;
    else if (t->x < 0.0 && t->y == 0.0 && t->z == 0.0)
	n->y = -1.0;
    else
    {
	float d;

	if (t->z != 0.0)
	{
	    n->x = n->y = 1.0;
	    n->z = -(t->x + t->y) / t->z;
	}
	else if (t->y != 0.0)
	{
	    n->x = n->z = 1.0;
	    n->y = -(t->x + t->z) / t->y;
	}
    
	d = 1.0 / sqrt(n->x*n->x + n->y*n->y + n->z*n->z);
	
	n->x *= d;
	n->y *= d;
	n->z *= d;
    }

    _dxfvector_cross_3D(n, t, b);
    _dxfvector_normalize_3D(b, b);

    for (i = 0; i < np-1; i++)
    {
	if (path)
	{
	    _dxfvector_subtract_3D(pts + path[i+1].head, pts + path[i].head, t);
	}
	else
	{
	    _dxfvector_subtract_3D(pts + i + 1, pts + i, t);
	}
	if (_dxfvector_normalize_3D(t, t) != ERROR)
	    break;
    } 

    *fb = *b;
    _dxfvector_cross_3D(t, b, fn);
    _dxfvector_normalize_3D(fn, fn);

    return OK;
}

/*
 * update frame of reference from prior vertex to current vertex based
 * on incuming and outgoing edge vectors and twist.
 */

#define VecMat(a, b, c)					    \
{							    \
    Vector p;						    \
    p.x = (a)->x*(b)[0] + (a)->y*(b)[3] + (a)->z*(b)[6];    \
    p.y = (a)->x*(b)[1] + (a)->y*(b)[4] + (a)->z*(b)[7];    \
    p.z = (a)->x*(b)[2] + (a)->y*(b)[5] + (a)->z*(b)[8];    \
    *(c) = p;						    \
}

static Error
UpdateFrame(Vector *p0, Vector *p1, Vector *lt, Vector *n, Vector *b, 
						    Vector *fn, Vector *fb)
{
    float  mBend[9];
    Vector cross;
    float  len, cA;
    Vector nt;

    _dxfvector_subtract_3D(p1, p0, &nt);

    /*
     * Look at the length of the step proceeding from the current
     * vertex.  If its zero length, just leave the frame of reference
     * alone.
     */
    len = _dxfvector_length_3D(&nt);
    if (len == 0.0)
    {
	n->x = fn->x; n->y = fn->y; n->z = fn->z;
	b->x = fb->x; b->y = fb->y; b->z = fb->z;
	return OK;
    }
    
    /*
     * DXDot the incoming and outgoing tangents to determine
     * whether a bend occurs.
     */
    _dxfvector_scale_3D(&nt, 1.0/len, &nt);
    cA = _dxfvector_dot_3D(lt, &nt);

    /*
     * If there's a bend angle...
     */
    if (cA < 0.999999)
    {
	float sA, angle;

	/*
	 * DXNormalize the bend axis
	 */
	_dxfvector_cross_3D(&nt, lt, &cross);
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

	/*
	 * Now we have the vertex frame of reference
	 */
	n->x = fn->x; n->y = fn->y; n->z = fn->z;
	b->x = fb->x; b->y = fb->y; b->z = fb->z;

	VecMat(fn, mBend, fn);
	VecMat(fb, mBend, fb);

	/*
	 * Outgoing tangent is normalized exiting segment vector.  Also
	 * normalize the frame normal and binormal.
	 */
	_dxfvector_normalize_3D(fn, fn);
	_dxfvector_normalize_3D(fb, fb);
	*lt = nt;
    }
    else
    {
	n->x = fn->x; n->y = fn->y; n->z = fn->z;
	b->x = fb->x; b->y = fb->y; b->z = fb->z;
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

typedef struct
{
    SegList *list;
    char    *name;
    int     size;
    int     dep;
} CList;

typedef struct hashElt
{
    struct hashElt *link;
    int tail, seg;
} HashElt;

static CList *NewCList(Field);
static Error DestroyCList(CList *);
static Error CopyArraysPartialPath(Field, int, PathElt *, int, CList *);
static Field CListToField(Field, CList *);
static Error TubePartialPath(Vector *, double, int, float *, float *,
		PathElt *, int, CList *, Vector *, Vector *);
static Error PathDot(Vector *, PathElt *, int, Vector *, Vector **, int);

static Field
TubeIrregular(Field f, double d, int ngon, float *cs, float *sn, float t)
{
    Array cA, pA, nA, bA;
    PathElt *forward = NULL, *backward = NULL, *combined = NULL, *list;
    char *flags = NULL;
    Line *segs;
    int  nSegs, nPts;
    HashElt  **index = NULL, *links = NULL, *link, *found;
    int  nf, nb;
    Vector *points, *normals = NULL, *binormals = NULL;
    int own_normals = 0; 
    CList  *clist = NULL;
    int i, j, done, nonZeroSegs;
    Vector *segVectors = NULL;
    InvalidComponentHandle ich = NULL;

    if (DXEmptyField(f))
	return f;
    
    pA = (Array)DXGetComponentValue(f, "positions");
    if (! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "missing positions component");
	goto error;
    }

    if (!DXComponentOpt(pA, (Pointer*)&points, &nPts, 0, TYPE_FLOAT, 3))
    {
	DXResetError();
	if (NULL == (pA = CvtPositions(f)) ||
	    !DXComponentOpt(pA, (Pointer*)&points, &nPts, 0, TYPE_FLOAT, 3))
		return NULL;
	DXSetComponentValue(f, "positions", (Object)pA);
    }

    cA = DXGetConnections(f, "lines");
    if (! cA)
    {
	if (!DXGetComponentValue(f, "connections"))
	    DXSetError(ERROR_MISSING_DATA, "connections component");
	else
	    DXSetError(ERROR_DATA_INVALID, "element type must be lines");
	goto error;
    }
    
    DXGetArrayInfo(cA, &nSegs, NULL, NULL, NULL, NULL);
    if (nSegs == 0)
	return RemoveFieldComponents(f);

    segs = (Line *)DXGetArrayData(cA);

    segVectors = (Vector *)DXAllocate(nSegs*sizeof(Vector));
    if (! segVectors)
	goto error;
    
    if (DXGetComponentValue(f, "invalid connections"))
    {
	ich = DXCreateInvalidComponentHandle((Object)f, NULL, "connections");
	if (! ich)
	    goto error;
    }

    for (i = nonZeroSegs = 0; i < nSegs; i++)
    {
	if (!ich || DXIsElementValid(ich, i))
	{
	    Vector *p = (Vector *)(points+segs[i].p);
	    Vector *q = (Vector *)(points+segs[i].q);
	    float l;

	    vector_subtract(p, q, segVectors+i);
	    l = _dxfvector_length_3D(segVectors + i);
	    if (l != 0)
	    {
		nonZeroSegs = 1;
		l = 1.0 / l;
		_dxfvector_scale_3D(segVectors + i, l, segVectors + i);
	    }
	}
    }

    if (! nonZeroSegs)
    {
	DXFree((Pointer)segVectors);
        return RemoveFieldComponents(f);
    }

    nA = (Array)DXGetComponentValue(f, "normals");
    if (nA)
	normals = (Vector *)DXGetArrayData(nA);
    
    bA = (Array)DXGetComponentValue(f, "binormals");
    if (bA)
	binormals = (Vector *)DXGetArrayData(bA);

    /*
     * If the data is < 3D, use the additional dimension as the normal
     */
    if (! normals && ! binormals)
    {
	Point box[8], min, max;
	int i, doit;
	float normal[3];

	normal[0] = normal[1] = normal[2] = 0.0;

	if (! DXBoundingBox((Object)f, box))
	    return NULL;
	
	min.x = min.y = min.z =  DXD_MAX_FLOAT;
	max.x = max.y = max.z = -DXD_MAX_FLOAT;
	for (i = 0; i < 8; i++)
	{
	    if (box[i].x < min.x) min.x = box[i].x;
	    if (box[i].x > max.x) max.x = box[i].x;
	    if (box[i].y < min.y) min.y = box[i].y;
	    if (box[i].y > max.y) max.y = box[i].y;
	    if (box[i].z < min.z) min.z = box[i].z;
	    if (box[i].z > max.z) max.z = box[i].z;
	}

	if (min.z == max.z)
	{
	    normal[2] = 1.0;
	    doit = 1;
	}
	else if (min.y == max.y)
	{
	    normal[1] = 1.0;
	    doit = 1;
	}
	else if (min.x == max.x)
	{
	    normal[0] = 1.0;
	    doit = 1;
	}
	else
	    doit = 0;
	
	if (doit)
	{
	    own_normals = 1;
	    normals = (Vector *)DXAllocate(nPts * sizeof(Vector));
	    if (! normals)
		goto error;
	    
	    for (i = 0; i < nPts; i++)
		memcpy((char *)(normals + i), (char *)normal, sizeof(normal));
	}
    }

    forward  = (PathElt *)DXAllocate((nSegs+2)*sizeof(PathElt));
    backward = (PathElt *)DXAllocate((nSegs+2)*sizeof(PathElt));
    combined = (PathElt *)DXAllocate((nSegs+2)*sizeof(PathElt));
    flags    = (char *)DXAllocateZero((nSegs+2)*sizeof(char));

    if (! forward || ! backward || ! combined || ! flags)
	goto error;
    
    index = (HashElt **)DXAllocateZero(nPts*sizeof(HashElt *));
    links = (HashElt *) DXAllocate(2*nSegs*sizeof(HashElt));
    if (! index || ! links)
	goto error;

    clist = NewCList(f);
    if (! clist)
	goto error;


    for (i = 0; i < nSegs; i++)
    {
	if (!ich || DXIsElementValid(ich, i))
	{
	    int p = segs[i].p;
	    int q = segs[i].q;

	    if (! vector_zero(segVectors+i))
	    {
		links[(i<<1)].link = index[p];
		links[(i<<1)].tail = q;
		links[(i<<1)].seg  = i;
		index[p] = links + (i<<1);

		links[(i<<1)+1].link = index[q];
		links[(i<<1)+1].tail = p;
		links[(i<<1)+1].seg  = i;
		index[q] = links + (i<<1) + 1;
	    }
	}
    }

    for (i = 0; i < nPts; )
    {
	nf = nb = 0;

	/*
	 * See if there's an unused segment incident on this 
	 * vertex.
	 */
	for (link = index[i]; link; link = link->link)
	    if (flags[link->seg] == 0)
		break;

	/*
	 * If not, go on to next vertex.
	 */
	if (! link)
	{
	    i++;
	    continue;
	}
	
	/*
	 * Start a forward chain here.
	 */
	forward[nf].head  = i;
	forward[nf++].seg = link->seg;

	/* 
	 * Trace the forward chain.
	 */
	done = 0;
	while (! done)
	{
	    int lastseg = link->seg;

	    flags[link->seg] = 1;
	    forward[nf].head = link->tail;

	    found = NULL;
	    for (link = index[link->tail]; link && !found; link = link->link)
		if (flags[link->seg] == 0)
		{
		    float d = vector_dot(segVectors+lastseg,
					    segVectors+link->seg);
		    Line *s0 = segs + lastseg;
		    Line *s1 = segs + link->seg;

		    if (((s0->p == s1->p || s0->q == s1->q) && d < -t) ||
		        ((s0->p == s1->q || s0->q == s1->p) && d >  t))
			found = link;
		}

	    if (found)
		forward[nf++].seg = found->seg;
	    else
	    { 
		done = 1;
		forward[nf++].seg = -1;
	    }

	    link = found;
	}

	/*
	 * If there's another unused segment incident on this vertex,
	 * trace a backward chain.
	 */
	for (link = index[i], found = NULL; link && !found; link = link->link)
	    if (flags[link->seg] == 0)
	    {
		float d = vector_dot(segVectors+forward[0].seg,
					segVectors+link->seg);
		Line *s0 = segs + forward[0].seg;
		Line *s1 = segs + link->seg;

		if (((s0->p == s1->p || s0->q == s1->q) && d < -t) ||
		    ((s0->p == s1->q || s0->q == s1->p) && d >  t))
		    found = link;
	    }
	
	if (found)
	{
	    int lastseg = found->seg;

	    link = found;
	    while (found)
	    {
		backward[nb].head = link->tail;
		backward[nb++].seg  = link->seg;
		flags[link->seg]  = 1;

		found = NULL;
		for (link=index[link->tail]; link && !found; link=link->link)
		    if (flags[link->seg] == 0)
		    {
			float d = vector_dot(segVectors+lastseg,
						segVectors+link->seg);
			Line *s0 = segs + lastseg;
			Line *s1 = segs + link->seg;

			if (((s0->p == s1->p || s0->q == s1->q) && d < -t) ||
			    ((s0->p == s1->q || s0->q == s1->p) && d >  t))
			    found = link;
		    }
		
		if (found)
		{
		    lastseg = found->seg;
		    link = found;
		}
	    }

	    for (j = 0; j < nb; j++)
	    {
		combined[j].head = backward[(nb-1)-j].head;
		combined[j].seg  = backward[(nb-1)-j].seg;
	    }
	
	    for (j = 0; j < nf; j++)
	    {
		combined[nb+j].head = forward[j].head;
		combined[nb+j].seg  = forward[j].seg;
	    }

	    list = combined;
	}
	else
	{
	    list = forward;
	}

	if (! TubePartialPath(points, d, ngon, cs, sn, list, nf+nb, clist,
						normals, binormals)) {
	    if (DXGetError() == ERROR_NONE)
		continue;
	    else
		goto error;
	}

	if (! CopyArraysPartialPath(f, ngon, list, nf+nb, clist))
	    goto error;
    }

    if (! CListToField(f, clist))
	goto error;
    
    if (own_normals)
	DXFree((Pointer)normals);

    DXFree((Pointer)segVectors);
    DXFree((Pointer)forward);
    DXFree((Pointer)backward);
    DXFree((Pointer)combined);
    DXFree((Pointer)flags);
    DXFree((Pointer)index);
    DXFree((Pointer)links);
    DestroyCList(clist);
    if (ich) DXFreeInvalidComponentHandle(ich);

    return f;

error:
    DXFree((Pointer)segVectors);
    DXFree((Pointer)forward);
    DXFree((Pointer)backward);
    DXFree((Pointer)combined);
    DXFree((Pointer)flags);
    DXFree((Pointer)index);
    DXFree((Pointer)links);
    DestroyCList(clist);
    if (ich) DXFreeInvalidComponentHandle(ich);

    return NULL;
}


#define NewCListEntry(n, s, i, d)     		 \
    clist[i].list = DXNewSegList(s, -1, -1);	 \
    if (! clist[i].list) goto error;		 \
    clist[i].name = n;				 \
    clist[i].size = s;				 \
    clist[i].dep = d				 \

#define DEP_ON_POSITIONS    1
#define DEP_ON_CONNECTIONS  2
#define NO_DEP		    3

#define DEP_ON_POSITIONS    1
#define CLIST_POSITIONS     0
#define CLIST_CONNECTIONS   1
#define CLIST_NORMALS       2


static CList *
NewCList(Field f)
{
    CList *clist = NULL;
    int i, knt;
    char *name;
    Array a;

    /*
     * Count them.  positions, connections, normals, nothing that
     * refs or ders, no binormals or tangents.
     */
    knt = 3;
    for (i = 0; DXGetEnumeratedComponentValue(f, i, &name); i++)
    {
	if (! strcmp(name, "positions") ||
	    ! strcmp(name, "connections") ||
	    ! strcmp(name, "polylines") ||
	    ! strcmp(name, "edges") ||
	    ! strcmp(name, "normals") ||
	    ! strcmp(name, "binormals") ||
	    ! strcmp(name, "tangents") ||
	    ! strncmp(name, "invalid", 7) ||
	    DXGetComponentAttribute(f, name, "der") ||
	    DXGetComponentAttribute(f, name, "ref")) continue;
	
	knt ++;
    }

    clist = (CList *)DXAllocateZero((knt+1)*sizeof(CList));
    if (! clist)
	goto error;
    
    NewCListEntry("positions",   3*sizeof(float), CLIST_POSITIONS,   NO_DEP);
    NewCListEntry("connections", 4*sizeof(int),   CLIST_CONNECTIONS, NO_DEP);
    NewCListEntry("normals",     3*sizeof(float), CLIST_NORMALS,     NO_DEP);

    i = 0; knt = 3;
    while ((a = (Array)DXGetEnumeratedComponentValue(f, i++, &name)) != NULL)
    {
	Object attr;
	int    dep;

	if (! strcmp(name, "positions") ||
	    ! strcmp(name, "connections") ||
	    ! strcmp(name, "normals") ||
	    ! strcmp(name, "binormals") ||
	    ! strcmp(name, "tangents") ||
	    ! strcmp(name, "polylines") ||
	    ! strcmp(name, "edges") ||
	    ! strncmp(name, "invalid", 7) ||
	    DXGetComponentAttribute(f, name, "der") ||
	    DXGetComponentAttribute(f, name, "ref")) continue;
	
	attr = DXGetComponentAttribute(f, name, "dep");
	if (! attr)
	    continue;
	
	if (DXGetObjectClass(attr) != CLASS_STRING)
	{
	    DXSetError(ERROR_BAD_CLASS, "component dependency attribute");
	    goto error;
	}

	if (! strcmp(DXGetString((String)attr), "positions")) 
	    dep = DEP_ON_POSITIONS;
	else if (!strcmp(DXGetString((String)attr), "connections") ||
		 !strcmp(DXGetString((String)attr), "polylines")) 
	    dep = DEP_ON_CONNECTIONS;
	else
	{
	    DXSetError(ERROR_DATA_INVALID, "invalid component dependency");
	    goto error;
	}

	NewCListEntry(name, DXGetItemSize(a), knt, dep);
	knt ++;
    }

    return clist;

error:
    if (clist)
        DestroyCList(clist);
    return NULL;
}

static Error
DestroyCList(CList *clist)
{
    if (clist)
    {
	CList *c = clist;

	while (c->list)
	{
	    DXDeleteSegList(c->list);
	    c++;
	}

	DXFree((Pointer)clist);
    }
    return OK;
}

static Error
CopyArraysPartialPath(Field f, int ng, PathElt *path, int np, CList *clist)
{
    int i, j;
    int loop = (path[0].head == path[np-1].head);
    int nu;

    nu = loop ? np - 1 : np;

    for (i = 3; clist[i].list; i++)
    {
	Array sA = (Array)DXGetComponentValue(f, clist[i].name);
	if (! sA)
	{
	    DXSetError(ERROR_INTERNAL, "no src array for clist component %s",
					clist[i].name);
	    return ERROR;
	}

	if (clist[i].dep == DEP_ON_POSITIONS)
	{
	    char *src, *dst;
	    char *sPtr, *dPtr;
	    SegListSegment *slist;

	    slist = DXNewSegListSegment(clist[i].list, nu*ng);
	    if (! slist)
		goto error;
	    
	    src = (char *)DXGetArrayData(sA);
	    dst = (char *)DXGetSegListSegmentPointer(slist);

	    for (dPtr = dst, j = 0; j < nu; j++)
	    {
		sPtr = src + clist[i].size*path[j].head;
		memcpy(dPtr, sPtr, clist[i].size);
		dPtr += clist[i].size;
	    }

	    for (j = 1; j < ng; j++)
	    {
		memcpy(dPtr, dst, nu*clist[i].size);
		dPtr += nu*clist[i].size;
	    }
	}
	else
	{
	    char *src, *dst;
	    char *sPtr, *dPtr;
	    SegListSegment *slist;
	    int nt;

	    if (ng == RIBBON)
		nt = 1;
	    else
		nt = ng;

	    slist = DXNewSegListSegment(clist[i].list, (np-1)*nt);
	    if (! slist)
		goto error;
	    
	    src = (char *)DXGetArrayData(sA);
	    dst = (char *)DXGetSegListSegmentPointer(slist);

	    for (dPtr = dst, j = 0; j < np-1; j++)
	    {
		sPtr = src + clist[i].size*path[j].seg;
		memcpy(dPtr, sPtr, clist[i].size);
		dPtr += clist[i].size;
	    }

	    for (j = 1; j < nt; j++)
	    {
		memcpy(dPtr, dst, (np-1)*clist[i].size);
		dPtr += (np-1)*clist[i].size;
	    }
	}
    }

    return OK;

error:
    return ERROR;
}

static Field
CListToField(Field f, CList *clist)
{
    Type t;
    Category c;
    int r, s[32];
    int i, j, k;
    char *name, *toDelete[100];
    Array sA, dA = NULL;

    for (i = j = 0; DXGetEnumeratedComponentValue(f, i, &name); i++)
    {
	if (! strcmp(name, "color map") ||
	    ! strcmp(name, "opacity map"))
	    continue;

	for (k = 0; clist[k].list; k++)
	    if (! strcmp(clist[k].name, name))
	        break;
	
	if (! clist[k].list)
	    toDelete[j++] = name;
    }

    for (i = 0; i < j; i++)
	DXDeleteComponent(f, toDelete[i]);
    
    for (i = 0; clist[i].list; i++)
    {
	char *sPtr, *dPtr;
	int  size;
	SegListSegment *slist;

	if (i == CLIST_CONNECTIONS)
	{
	    dA = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4);
	    if (! dA)
		goto error;
	    
	    if (! DXAddArrayData(dA, 0, DXGetSegListItemCount(clist[i].list), NULL))
		goto error;

	    dPtr = (char *)DXGetArrayData(dA);

	    if (! DXSetComponentValue(f, clist[i].name, (Object)dA))
		goto error;
	    dA = NULL;

	    if (! DXSetComponentAttribute(f, "connections", "element type",
				(Object)DXNewString("quads")))
		goto error;
	}
	else if (i ==  CLIST_NORMALS)
	{
	    dA = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	    if (! dA)
		goto error;

	    if (! DXAddArrayData(dA, 0, DXGetSegListItemCount(clist[i].list), NULL))
		goto error;

	    dPtr = (char *)DXGetArrayData(dA);

	    if (! DXSetComponentValue(f, clist[i].name, (Object)dA))
		goto error;
	    dA = NULL;
	}
	else
	{
	    sA = (Array)DXGetComponentValue(f, clist[i].name);
	    if (! sA)
	    {
		DXSetError(ERROR_INTERNAL, "no src array for clist component %s",
					clist[i].name);
		goto error;
	    }

	    DXGetArrayInfo(sA, NULL, &t, &c, &r, s);

	    dA = DXNewArrayV(t, c, r, s);
	    if (! dA)
		goto error;

	    if (! DXAddArrayData(dA, 0, DXGetSegListItemCount(clist[i].list), NULL))
		goto error;

	    dPtr = (char *)DXGetArrayData(dA);

	    if (! DXSetComponentValue(f, clist[i].name, (Object)dA))
		goto error;
	    dA = NULL;
	}
	
	

	DXInitGetNextSegListSegment(clist[i].list);
	while ((slist = DXGetNextSegListSegment(clist[i].list)) != NULL)
	{
	    sPtr = DXGetSegListSegmentPointer(slist);
	    size = DXGetSegListSegmentItemCount(slist) * clist[i].size;

	    memcpy(dPtr, sPtr, size);
	    dPtr += size;
	}

    }

    /*
     * add default colors if necessary
     */
    if (!DXGetComponentValue(f, "colors") &&
        !DXGetComponentValue(f, "front colors") &&
	!DXGetComponentValue(f, "back colors"))
    {
	int n = DXGetSegListItemCount(clist[CLIST_POSITIONS].list);
	static RGBColor col = {.7, .7, 0};
	static RGBColor zero = {0, 0, 0};
	Array a = (Array) DXNewRegularArray(TYPE_FLOAT, 3, n,
				    (Pointer)&col, (Pointer)&zero);
	if (! a)
	    goto error;

	DXSetComponentValue(f, "colors", (Object)a); /* ok because f is a copy */
    }

    if (DXGetAttribute((Object)f, "shade"))
	if (! DXSetAttribute((Object)f, "shade", NULL))
	    goto error;

    if (! DXEndField(f))
	goto error;

    return f;

error:
    DXDelete((Object)dA);
    return NULL;
}

#define REVERSE	1

static Error
TubePartialPath(Vector *points, double diameter, int ngon, 
	    float *cs, float *sn, PathElt *path, int npoints, CList *clist,
	    Vector *normals, Vector *binormals)
{
    int i, j, k, nnewpoints, nnewconn, max;
    Vector *newpoints, *newnormals;
    Quadrilateral *quads, *t;
    int  pOffset, loop, nUniquePoints;
    SegListSegment *slist;
    int  myNormals = 0, myBinormals = 0;

    loop = (path[0].head == path[npoints-1].head);
    if (loop)
	nUniquePoints = npoints - 1;
    else
	nUniquePoints = npoints;

    nnewpoints = ngon * nUniquePoints;

    /*nconn = npoints-1;*/
    nnewconn = (ngon==RIBBON? 1 : ngon) * (npoints-1);

    if (normals && !binormals)
    {
	if (! PathDot(points, path, npoints, normals, &binormals, !REVERSE))
	    goto error;
	myBinormals = 1;
    }
    else if (! normals && binormals)
    {
	if (! PathDot(points, path, npoints, binormals, &normals, REVERSE))
	    goto error;
	myNormals = 1;
    }
    else if (! normals && ! binormals)
    {
	if (! TracePath(points, path, npoints, &normals, &binormals))
	    return ERROR;

	if (! normals && ! binormals)
	    return OK;

	myNormals = 1;
	myBinormals = 1;
    }

    /*
     * When we add quads to the clist, we need to take
     * the positions that arose from earlier partial paths into
     * account.
     */
    pOffset = DXGetSegListItemCount(clist[CLIST_POSITIONS].list);

    /*
     * new points buffer
     */
    slist = DXNewSegListSegment(clist[CLIST_POSITIONS].list, nnewpoints);
    if (! slist)
	goto error;
    newpoints = (Vector *)DXGetSegListSegmentPointer(slist);

    /*
     * new normals buffer
     */
    slist = DXNewSegListSegment(clist[CLIST_NORMALS].list, nnewpoints);
    if (! slist)
	goto error;
    newnormals = (Vector *)DXGetSegListSegmentPointer(slist);

    /*
     * new connections buffer
     */
    slist = DXNewSegListSegment(clist[CLIST_CONNECTIONS].list, nnewconn);
    if (! slist)
	goto error;
    quads = (Quadrilateral *)DXGetSegListSegmentPointer(slist);

    /* new points and normals */
    if (ngon==RIBBON)
    {				/* ribbon */
	for (i=0; i < nUniquePoints; i++)
	{
	    Vector w;
	    int    pi = path[i].head;
	    Vector *binorm, *norm;

	    if (myBinormals)
		binorm = binormals + i;
	    else
		binorm = binormals + path[i].head;

	    if (myNormals)
		norm = normals + i;
	    else
		norm = normals + path[i].head;

	    w = DXMul(*binorm, diameter/2);
	    newpoints[i] = DXSub(points[pi], w);
	    newpoints[i+nUniquePoints] = DXAdd(points[pi], w);
	    newnormals[i] = *norm;
	    newnormals[i+nUniquePoints] = *norm;
	}
    }
    else
    {					/* tube */
	for (i=0; i<nUniquePoints; i++)
	{
	    int pi = path[i].head;

	    for (j=i, k=0; k<ngon; j+=nUniquePoints, k++) {
		Vector normal;
		Vector *binorm, *norm;

		if (myBinormals)
		    binorm = binormals + i;
		else
		    binorm = binormals + path[i].head;

		if (myNormals)
		    norm = normals + i;
		else
		    norm = normals + path[i].head;

		normal = DXAdd(DXMul(*norm,cs[2*k]), DXMul(*binorm,sn[2*k]));
		newnormals[j] =  normal;
		newpoints[j] = DXAdd(points[pi], DXMul(normal,diameter/2));
	    }
	}
    }

    /*
     * new quads.  j iterates around the tube and i along the tube.
     */
    t = quads;
    max = (ngon==RIBBON? 1 : ngon);
    for (j = 0; j < max; j ++)
    {
	int p, q, r, s, ps, qs;

	p = pOffset + (j * nUniquePoints);

	if (j == ngon-1)
	    q = pOffset;
	else
	    q = pOffset + ((j+1) * nUniquePoints);
	
	ps = p;
	qs = q;

	for (i=0; i<npoints-1; i++)
	{
	    if (i == npoints-2 && loop)
	    {
		r = ps;
		s = qs;
	    }
	    else
	    {
		r = p + 1;
		s = q + 1;
	    }

	    t->p = p;
	    t->q = q;
	    t->r = r;
	    t->s = s;

	    p++;
	    q++;
	    t++;
	}
    }

    if (myNormals)
	DXFree((Pointer)normals);
    if (myBinormals)
	DXFree((Pointer)binormals);

    return OK;

error:
    
    if (myNormals)
	DXFree((Pointer)normals);
    if (myBinormals)
	DXFree((Pointer)binormals);

    return ERROR;
}

static Error
PathDot(Vector *points, PathElt *path, int npoints, 
	Vector *vec0, Vector **vec1, int reverse)
{
    int i, loop;

    loop = (path[0].head == path[npoints-1].head);

    if (loop)
	npoints --;

    *vec1 = (Vector *)DXAllocate(npoints * sizeof(Vector));
    if (! *vec1)
	return ERROR;
    
    for (i = 0; i < npoints; i++)
    {
	int p, q, r;
	Vector v0, v1, tan;
	float l;

	if (i == 0)
	    if (loop)
		p = path[npoints-1].head;
	    else
		p = path[0].head;
	else
	    p = path[i-1].head;
    
	if (i == (npoints-1))
	    if (loop)
		q = path[0].head;
	    else
		q = path[npoints-1].head;
	else
	    q = path[i+1].head;

	r = path[i].head;
	
	_dxfvector_subtract_3D(points+r, points+p, &v0);
	_dxfvector_subtract_3D(points+q, points+r, &v1);

	if ((l = _dxfvector_length_3D(&v0)) != 0.0)
	    l = 1.0 / l;
	_dxfvector_scale_3D(&v0, l, &v0);

	if ((l = _dxfvector_length_3D(&v1)) != 0.0)
	    l = 1.0 / l;
	_dxfvector_scale_3D(&v1, l, &v1);

	_dxfvector_add_3D(&v0, &v1, &tan);
	if ((l = _dxfvector_length_3D(&tan)) != 0.0)
	    l = 1.0 / l;
	_dxfvector_scale_3D(&tan, l, &tan);

	if (reverse == 0)
	    _dxfvector_cross_3D(vec0+path[i].head, &tan, (*vec1)+i);
	else
	    _dxfvector_cross_3D(&tan, vec0+path[i].head, (*vec1)+i);
    }

    return OK;
}

static Field
RemoveFieldComponents(Field f)
{
    int i;
    char *names[100];

    for (i = 0; DXGetEnumeratedComponentValue(f, i, &names[i]); i++);
    for (--i; i >= 0; i--)
	DXDeleteComponent(f, names[i]);
    return DXEndField(f);
}

static Array
CvtPositions(Field f)
{
    Array in;
    Array out = NULL;
    Array nA = NULL;
    int nPts, nDim;
    int i, j, k;
    float *iPtr, *oPtr;
    float normal[3];

    normal[0] = normal[1] = 0.0;
    normal[2] = 1.0;

    in = (Array)DXGetComponentValue(f, "positions");
    DXGetArrayInfo(in, &nPts, NULL, NULL, NULL, &nDim);
    if (nDim > 3)
    {
	DXSetError(ERROR_DATA_INVALID, 
		"cannot tube lines of greater than 3 dimensions");
	goto error;
    }
    
    out = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! out)
	goto error;

    if (! DXAddArrayData(out, 0, nPts, NULL))
	goto error;
    
    iPtr = (float *)DXGetArrayData(in);
    oPtr = (float *)DXGetArrayData(out);
    if (! iPtr || ! oPtr)
	goto error;
    
    k = (nDim > 3) ? 3 : nDim;

    for (i = 0; i < nPts; i++, iPtr += nDim)
    {
       for (j = 0; j < k; j++)
	   *oPtr++ = iPtr[j];
	
	for (j = k; j < 3; j++)
	    *oPtr++ = 0;
    }

    if (! DXSetComponentValue(f, "positions", (Object)out))
	goto error;
    
    nA = (Array)DXNewConstantArray(nPts, (Pointer)normal,
		TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    if (! nA)
	goto error;
    
    if (! DXSetComponentValue(f, "normals", (Object)nA))
	goto error;
    nA = NULL;
    
    if (! DXSetComponentAttribute(f, "normals", "dep", 
				(Object)DXNewString("positions")))
	goto error;

    return out;

error:
    DXDelete((Object)out);
    DXDelete((Object)nA);
    return NULL;
}

static Field
TubePolyline(Field f, double d, int ngon, float *cs, float *sn, float t)
{
    Array pA, nA, bA, plA, eA;
    int nPolylines, *polylines, nEdges, *edges;
    int pl, nPts;
    int nPath = 0;
    PathElt *path = NULL;
    CList  *clist = NULL;
    Vector *points, *normals = NULL, *binormals = NULL;
    int own_normals = 0;
    Type type;
    Category cat;
    int rank, shape[32];
    InvalidComponentHandle ich = NULL;

    if (DXEmptyField(f))
	return f;
    
    plA = (Array)DXGetComponentValue(f, "polylines");
    if (! plA)
    {
	DXSetError(ERROR_MISSING_DATA, "missing polylines component");
	goto error;
    }


    DXGetArrayInfo(plA, &nPolylines, &type, &cat, &rank, shape);
    
    if (type != TYPE_INT || cat != CATEGORY_REAL ||
       !(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "polylines must be scalar (or 1-vector) integers");
	goto error;
    }

    polylines = (int *)DXGetArrayData(plA);

    eA = (Array)DXGetComponentValue(f, "edges");
    if (! eA)
    {
	DXSetError(ERROR_MISSING_DATA, "missing edges component");
	goto error;
    }

    DXGetArrayInfo(eA, &nEdges, &type, &cat, &rank, shape);

    if (type != TYPE_INT || cat != CATEGORY_REAL ||
       !(rank == 0 || (rank == 1 && shape[0] == 1)))
    {
	DXSetError(ERROR_DATA_INVALID, 
	    "edges must be scalar (or 1-vector) integers");
	goto error;
    }

    edges = (int *)DXGetArrayData(eA);
    
    pA = (Array)DXGetComponentValue(f, "positions");
    if (! pA)
    {
	DXSetError(ERROR_MISSING_DATA, "missing positions component");
	goto error;
    }

    if (!DXComponentOpt(pA, (Pointer*)&points, &nPts, 0, TYPE_FLOAT, 3))
    {
	DXResetError();
	if (NULL == (pA = CvtPositions(f)) ||
	    !DXComponentOpt(pA, (Pointer*)&points, &nPts, 0, TYPE_FLOAT, 3))
		return NULL;
	DXSetComponentValue(f, "positions", (Object)pA);
    }

    nA = (Array)DXGetComponentValue(f, "normals");
    if (nA)
	normals = (Vector *)DXGetArrayData(nA);
    
    bA = (Array)DXGetComponentValue(f, "binormals");
    if (bA)
	binormals = (Vector *)DXGetArrayData(bA);

    /*
     * If the data is < 3D, use the additional dimension as the normal
     */
    if (! normals && ! binormals)
    {
	Point box[8], min, max;
	int i, doit;
	float normal[3];

	normal[0] = normal[1] = normal[2] = 0.0;

	if (! DXBoundingBox((Object)f, box))
	    return NULL;
	
	min.x = min.y = min.z =  DXD_MAX_FLOAT;
	max.x = max.y = max.z = -DXD_MAX_FLOAT;
	for (i = 0; i < 8; i++)
	{
	    if (box[i].x < min.x) min.x = box[i].x;
	    if (box[i].x > max.x) max.x = box[i].x;
	    if (box[i].y < min.y) min.y = box[i].y;
	    if (box[i].y > max.y) max.y = box[i].y;
	    if (box[i].z < min.z) min.z = box[i].z;
	    if (box[i].z > max.z) max.z = box[i].z;
	}

	if (min.z == max.z)
	{
	    normal[2] = 1.0;
	    doit = 1;
	}
	else if (min.y == max.y)
	{
	    normal[1] = 1.0;
	    doit = 1;
	}
	else if (min.x == max.x)
	{
	    normal[0] = 1.0;
	    doit = 1;
	}
	else
	    doit = 0;
	
	if (doit)
	{
	    own_normals = 1;
	    normals = (Vector *)DXAllocate(nPts * sizeof(Vector));
	    if (! normals)
		goto error;
	    
	    for (i = 0; i < nPts; i++)
		memcpy((char *)(normals + i), (char *)normal, sizeof(normal));
	}
    }

    clist = NewCList(f);
    if (! clist)
	goto error;

    if (DXGetComponentValue(f, "invalid polylines"))
    {
	ich = DXCreateInvalidComponentHandle((Object)f, NULL, "polylines");
	if (! ich)
	    goto error;
    }


    for (pl = 0; pl < nPolylines; pl++)
    {
	if (!ich || DXIsElementValid(ich, pl))
	{
	    int e0 = polylines[pl];
	    int e1 = (pl == nPolylines-1) ? nEdges : polylines[pl+1];
	    int i, knt = e1 - e0;

	    if (knt > nPath)
	    {
		path = (PathElt *)DXReAllocate(path, knt*sizeof(PathElt));
		if (! path)
		    goto error;
		nPath = knt;
	    }

	    for (i = 0; i < knt; i++)
	    {
		path[i].seg  = pl;
		path[i].head = edges[e0+i];
	    }

	    if (! TubePartialPath(points, d, ngon, cs, sn, path, knt, clist,
						    normals, binormals)) {
		if (DXGetError() == ERROR_NONE)
		    continue;
		else
		    goto error;
	    }

	    if (! CopyArraysPartialPath(f, ngon, path, knt, clist))
		goto error;
	}
    }

    if (! CListToField(f, clist))
	goto error;
    
    if (own_normals)
	DXFree((Pointer)normals);

    DXFree((Pointer)path);
    DestroyCList(clist);
    if (ich) DXFreeInvalidComponentHandle(ich);

    return f;

    error:
    DXFree((Pointer)path);
    DestroyCList(clist);
    if (ich) DXFreeInvalidComponentHandle(ich);

    return NULL;
}
