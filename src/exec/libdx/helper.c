/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include "fieldClass.h"
#include "internals.h"

static
Array
add(Field f, int start, int n, Pointer c, Object ct, int per)
{
    Array a = (Array) DXGetComponentValue(f, CONNECTIONS);
    if (!a) {
	a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, per);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, CONNECTIONS, (Object) a);
	DXSetAttribute((Object)a, ELEMENT_TYPE, ct);
    } else {
	Object t;
	t = DXGetComponentAttribute(f, CONNECTIONS, ELEMENT_TYPE);
	if (!t)
	    DXErrorReturn(ERROR_BAD_PARAMETER,
		"Connections component is missing element type attribute");
	if (t!=ct) {
	    char *s, *cs;
	    s = DXGetString((String)t);
	    cs = DXGetString((String)ct);
	    if (!s || !cs)
		return NULL;
	    if (strcmp(cs,s) != 0)
		DXErrorReturn(ERROR_BAD_PARAMETER, "Connections type conflict");
	}
    }
    if (!DXAddArrayData(a, start, n, c))
	return NULL;
    return a;
}

/*
 * begin/end cell
 */

Field _dxfBeginCell(Field f)
{
    CHECK(f, CLASS_FIELD);
    f->cell = 1;
    return f;
}

Field _dxfEndCell(Field f)
{
    CHECK(f, CLASS_FIELD);
    f->cell = 0;
    return f;
}


/*
 * beginface, next point, end face
 */

Field
_dxfBeginFace(Field f)
{
    CHECK(f, CLASS_FIELD);
    f->npts = 0;
    return f;
}

Field
_dxfNextPoint(Field f, PointId id)
{
    CHECK(f, CLASS_FIELD);
    if (f->npts==f->pts_alloc) {
	int n = f->pts_alloc? f->pts_alloc*3/2 : 100;
	PointId *p = (PointId *) DXReAllocate((Pointer)f->pts, n*sizeof(int));
	if (!p)
	    return NULL;
	f->pts = p;
	f->pts_alloc = n;
    }
    f->pts[f->npts++] = id;
    return f;
}


/*
 * For volume faces, canonically decompose the face
 * into triangles and put into hash table, distinguishing
 * between surface and inner faces.
 *
 * XXX - what if hash table fills up?
 *       doesn't preserve sense of surface faces
 */

static
Field
_EndFaceVolume(Field f)
{
    int npoints, min, i, dir, h;
    Triangle t;

    if (f->npts<3)
	return NULL;

    /* create hash table */
    if (!f->hash) {
	DXMarkTime("start faces");
	if (!DXGetArrayInfo((Array)DXGetComponentValue(f, POSITIONS), &npoints,
	  NULL, NULL, NULL, NULL))
	    DXErrorReturn(ERROR_MISSING_DATA,
			"points component missing in EndFace");
	for (f->hash_alloc=1; f->hash_alloc<10*npoints; f->hash_alloc*=2)
	    continue;
	f->hash = (struct hash *)
	    DXAllocate(f->hash_alloc*sizeof(struct hash));
	if (!f->hash)
	    return NULL;
	memset(f->hash, 0, f->hash_alloc*sizeof(struct hash));
	/* free old surface and inner faces */
	DXSetComponentValue(f, SURFACE, NULL);
	DXSetComponentValue(f, INNER, NULL);
    }

    /* canonical starting point is min vertex number */
    for (min=0, i=1; i<f->npts; i++)
	if (f->pts[i] < f->pts[min])
	    min = i;

#define PT(i) (f->pts[(i)%(f->npts)])

    /* canonical direction: pick smallest neighbor of min */
    dir = PT(min+1)<PT(min-1)? 0 : 1;

    /* f->npts-2 triangles */
    for (i=1; i<f->npts-1; i++) {

	/* here's the triangle */
	t.p = PT(min);
	t.q = PT(min+i+(dir));
	t.r = PT(min+i+(1-dir));

	/* put in hash table */
	h = (t.p + 17*t.q + 513*t.r) & (f->hash_alloc-1);
	while (f->hash[h].count && (f->hash[h].tri.p!=t.p
	       || f->hash[h].tri.q!=t.q || f->hash[h].tri.r!=t.r))
	    h = (h+1) & (f->hash_alloc-1);
	f->hash[h].tri = t;
	f->hash[h].count += 1;
	if (f->hash[h].count==1)	/* new face: */
	    f->nsurface += 1;		/*     assume surface */
	else if (f->hash[h].count&1) {	/* count now odd, was even: */
	    f->nsurface += 1;		/*     now surface */
	    f->ninner -= 1;		/*     was inner */
	} else {			/* count now even, was odd: */
	    f->ninner += 1;		/*     now inner */
	    f->nsurface -= 1;		/*     was surface */
	}
    }

    f->npts = 0;

    return f;
}


/*
 * For surface faces, just put the faces into "triangle" component.
 */

static
Field
_EndFaceSurface(Field f)
{
    int n, i;
    Triangle t;
    Array a;

    /* allocate "triangle" component */
    a = add(f, 0, 0, NULL, O_TRIANGLES, 3);
    if (!a)
	return NULL;

    /* starting face number */
    if (!DXGetArrayInfo(a, &n, NULL, NULL, NULL, NULL))
	DXErrorReturn(ERROR_INTERNAL, "DXGetArrayInfo failed in EndFace!");

    /* add f->npts-2 triangles starting at face n */
    for (i=1; i<f->npts-1; i++, n++) {
	t.p = f->pts[0];
	t.q = f->pts[i];
	t.r = f->pts[i+1];
	if (!DXAddArrayData(a, n, 1, (Pointer)&t))
	    return NULL;
    }

    f->npts = 0;

    return f;
}

Field
_dxfEndFace(Field f)
{
    CHECK(f, CLASS_FIELD);

    if (f->cell)
	return _EndFaceVolume(f);
    else
	return _EndFaceSurface(f);
}


/*
 * end field
 */

static int trace = 0;

void
_dxfTraceField(int t)
{
    trace = t;
}

/*
 * These variables allow for sharing of strings
 */

Field
DXEndField(Field f)
{
    Triangle *surface, *inner;
    int i; 
    int ns, ni, np, nc, n;
    Array a, ap, ac;
    Object o;
    char *name;
    static struct {
	char **component;			/* the component */
	char **attribute;			/* its attribute */
	Object *value;				/* the attribute's value */
	char **name;				/* dependent's name */
	int n;					/* number of items in value */
    } *s, standard[] = {			/* standard comp attributes */
	{ &POSITIONS,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &CONNECTIONS,	&DEP,	&O_CONNECTIONS, &CONNECTIONS, -1 },
	{ &CONNECTIONS,	&REF,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &DATA,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &COLORS,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &IMAGE,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &FRONT_COLORS,  &DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &BACK_COLORS,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &OPACITIES,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &TANGENTS,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &NORMALS,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ &BINORMALS,	&DEP,	&O_POSITIONS,   &POSITIONS,   -1 },
	{ NULL }
    };

    CHECK(f, CLASS_FIELD);


    /* trim component arrays */
    for (i=0; (o=DXGetEnumeratedComponentValue(f, i, NULL)); i++)
	if (DXGetObjectClass(o)==CLASS_ARRAY)
	    if (!DXTrim((Array)o))
		return NULL;

    /* free ->pts array */
    if (f->pts) {
	DXFree((Pointer)f->pts);
	f->pts_alloc = 0;
    }

    /* copy hash table of faces */
    if (f->hash) {

	DXMarkTime("end faces");
	DXMarkTime("start copy");

	/* surface */
	a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, SURFACE, (Object)a);
	DXSetAttribute((Object)a, REF, O_POSITIONS);
	DXAddArrayData(a, 0, f->nsurface, NULL);
	surface = (Triangle *) DXGetArrayData(a);

	/* inner */
	a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, INNER, (Object)a);
	DXSetAttribute((Object)a, REF, O_POSITIONS);
	DXAddArrayData(a, 0, f->ninner, NULL);
	inner = (Triangle *) DXGetArrayData(a);

	/* copy surface and inner faces in */
	for (i=0, ns=0, ni=0; i<f->hash_alloc; i++) {
	    if (f->hash[i].count&1)		/* odd count => surface face */
		surface[ns++] = f->hash[i].tri;
	    else if (f->hash[i].count!=0)	/* even count => inner face */
		inner[ni++] = f->hash[i].tri;
	}
	ASSERT(ns==f->nsurface && ni==f->ninner);

	DXFree((Pointer)f->hash);
	f->hash = NULL;
	f->hash_alloc = 0;

	DXMarkTime("end copy");
    }

    /* pre-compute bounding box */
    if (!DXBoundingBox((Object) f, NULL) && (DXGetError()!=ERROR_NONE))
	return NULL;

    /*
     * Pre-compute volume element neighbors.
     * These are done ahead of time to prevent the race condition
     * that arises if done on-the-fly in rendering.  In other
     * modules, which parallelize by data, it is ok to do them on
     * the fly.
     */
    if (!DXGetComponentValue(f, NEIGHBORS)) {
	if (!_dxf_TetraNeighbors(f) && DXGetError()!=ERROR_NONE)
	    return NULL;
	if (!_dxf_CubeNeighbors(f) && DXGetError()!=ERROR_NONE)
	    return NULL;
    }

    /* if we decide at some point to precompute statistics, here is one
     * place where it makes sense to put the call.  the other is in the
     * DXEndObject() routine, since stats can go parallel and needs to
     * compute stats on compositefields differently than normal fields.
     *
     * we currently do not precompute statistics because it can be expensive
     * in time for large arrays, and if there are invalid points the
     * stats have to be computed twice, once with and without considering
     * the invalids.  if the stats aren't needed, this is unnecessary work.
     * the argument for precomputing them is to avoid the stats call modifying
     * the input to a module.  the statistics() call adds a statistics array to
     * the input field so subsequent calls for stats do not have to recompute
     * the same values.  normally modifying the input to a module is a very bad
     * thing because if two tasks in one module modify the same input, there is
     * no locking at the field level and the result can be garbage.  in fact i
     * think this is the only routine we knowingly allow to modify inputs.
     * we allow it to avoid precomputing stats on every field every time, and
     * don't have problems because first we don't allow multiple modules
     * to execute at the same time so two different modules can't be computing
     * statistics on the same field at the same time, and second most modules
     * go parallel by giving different fields to each task, so even if the
     * tasks are executing at the same time, they are generally working on
     * different fields.  also, many modules need the min/max before going
     * to the task level, so the stats are computed before going parallel.
     * it is a potential source of problems however.
     */

    /* i moved the following code down to here.  it used to be the first code
     * in the module, but it makes more sense to be here next to the code
     * which uses these values.   nsc 06jun96 
     */

    /* number of items in value */
    np = nc = -1;
    ap = (Array)DXGetComponentValue(f, POSITIONS);
    if (ap && !DXGetArrayInfo(ap, &np, NULL, NULL, NULL, NULL))
	return NULL;
    ac = (Array)DXGetComponentValue(f, CONNECTIONS);
    if (ac && !DXGetArrayInfo(ac, &nc, NULL, NULL, NULL, NULL))
	return NULL;
    for (s=standard; s->component; s++) {
	if (*s->name == POSITIONS)
	    s->n = np;
	else if (*s->name == CONNECTIONS)
	    s->n = nc;
	else
	    s->n = -1;
    }

    /*
     * set standard component attributes
     * XXX - this depends on knowing that fields always store the
     * name as a pointer to one of the standard strings
     */
    for (i=0; (o=DXGetEnumeratedComponentValue(f, i, &name)); i++) {
	for (s=standard; s->component; s++) {
	    if (name==*s->component && s->n>=0) {
		if (!DXGetAttribute(o, *s->attribute)) {
		    if (*s->attribute==DEP) {
			if (!DXGetArrayInfo((Array)o,&n,NULL,NULL,NULL,NULL))
			    return NULL;
			if (n!=s->n) {
			    DXSetError(ERROR_DATA_INVALID,
				     "The \"%s\" component has %d elements.  "
				     "By default, it depends on the \"%s\" "
				     "component, which has %d elements.  "
				     "A valid \"dep\" attribute should "
				     "be specified for the \"%s\" component "
				     "to override the default.",
				     *s->component, n, *s->name, s->n,
				     *s->component);
			    return NULL;
			}
		    }
		    DXSetAttribute(o, *s->attribute, *s->value);
		}
		/* break; */
	    }
	}
    }

    if (trace) {
	int npoints=0, nsurface=0, ninner=0;
	DXGetArrayInfo((Array)DXGetComponentValue(f, POSITIONS), &npoints,
		     NULL, NULL, NULL, NULL);
	DXGetArrayInfo((Array)DXGetComponentValue(f, SURFACE), &nsurface,
		     NULL, NULL, NULL, NULL);
	DXGetArrayInfo((Array)DXGetComponentValue(f, INNER), &ninner,
		     NULL, NULL, NULL, NULL);
	DXDebug("F", "field 0x%x: %d points, %d surface, %d inner",
		f, npoints, nsurface, ninner);
    }

    return f;
}


/*
 * ChangedXXX routines
 */

/* true if there is an attr named `a', the contents are a single string,
 *  and the contents match the target component name.
 */
#define ATTR(a) (DXGetStringAttribute(c, a, &cp) && !strcmp(cp, component))

/* true if there is an attr named `a', the contents are either a single
 *  string or a string list, and if any member of the stringlist matches
 *  the target component name.
 */
static int attrlist(Object c, char *a, char *component)
{
    Object o;
    int l;
    char *cp;
    
    if (!(o = DXGetAttribute(c, a)))
	return 0;
    
    for (l=0; DXExtractNthString(o, l, &cp); l++)
	if (!strcmp(cp, component))
	    return 1;

    return 0;
}

Field
DXChangedComponentStructure(Field f, char *component)
{
    int i, n;
    Object c;
    char *names[100], *cp;

    for (i=0, n=0; (c=DXGetEnumeratedComponentValue(f, i, &names[n])); i++)
	if (ATTR(DEP) || ATTR(REF) || attrlist(c, DER, component))
	    if (strcmp(names[n],component) != 0)
			n++;
    ASSERT(n<100);
    for (i=0; i<n; i++) {
	DXDeleteComponent(f, names[i]);
	DXChangedComponentStructure(f, names[i]);
    }
    return f;
}    

Field
DXChangedComponentValues(Field f, char *component)
{
    int i, n;
    Object c;
    char *names[100];

	if (! strcmp(component, "invalid connections"))
	{
		DXInvalidateUnreferencedPositions((Object)f);
		DXChangedComponentValues(f, "connections");
		DXChangedComponentValues(f, "positions");
	}
	else if (! strcmp(component, "invalid positions"))
	{
		if (DXGetComponentValue(f, "connections"))
		{
		    DXInvalidateConnections((Object)f);
		    DXInvalidateUnreferencedPositions((Object)f);
		    DXChangedComponentValues(f, "connections");
		}
		DXChangedComponentValues(f, "positions");
	}
	else
	{
		for (i=0, n=0; (c=DXGetEnumeratedComponentValue(f, i, &names[n])); i++)
			if (attrlist(c, DER, component))
				if (strcmp(names[n],component) != 0)
					n++;
		ASSERT(n<100);
		for (i=0; i<n; i++) {
			DXDeleteComponent(f, names[i]);
			DXChangedComponentStructure(f, names[i]);
		}
    }
    return f;
}    


/*
 * DXAdd routines
 */

Field
DXAddPoints(Field f, int start, int n, Point *p)
{
    Array a = (Array) DXGetComponentValue(f, POSITIONS);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, POSITIONS, (Object)a);
	DXSetAttribute((Object)a, DEP, O_POSITIONS);
    }
    if (!DXAddArrayData(a, start, n, (Pointer)p))
	return NULL;
    return f;
}

Field
DXAddPoint(Field f, int id, Point p)
{
    return DXAddPoints(f, id, 1, &p);
}

Field
DXAddColors(Field f, int start, int n, RGBColor *c)
{
    Array a = (Array) DXGetComponentValue(f, COLORS);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, COLORS, (Object)a);
	DXSetAttribute((Object)a, DEP, O_POSITIONS);
    }
    if (!DXAddArrayData(a, start, n, (Pointer)c))
	return NULL;
    return f;
}

Field
DXAddColor(Field f, PointId id, RGBColor c)
{
    return DXAddColors(f, id, 1, &c);
}

Field
DXAddFrontColors(Field f, int start, int n, RGBColor *c)
{
    Array a = (Array) DXGetComponentValue(f, FRONT_COLORS);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, FRONT_COLORS, (Object)a);
	DXSetAttribute((Object)a, DEP, O_POSITIONS);
    }
    if (!DXAddArrayData(a, start, n, (Pointer)c))
	return NULL;
    return f;
}

Field
DXAddFrontColor(Field f, PointId id, RGBColor c)
{
    return DXAddFrontColors(f, id, 1, &c);
}

Field
DXAddBackColors(Field f, int start, int n, RGBColor *c)
{
    Array a = (Array) DXGetComponentValue(f, BACK_COLORS);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, BACK_COLORS, (Object)a);
	DXSetAttribute((Object)a, DEP, O_POSITIONS);
    }
    if (!DXAddArrayData(a, start, n, (Pointer)c))
	return NULL;
    return f;
}

Field
DXAddBackColor(Field f, PointId id, RGBColor c)
{
    return DXAddBackColors(f, id, 1, &c);
}

Field
DXAddOpacities(Field f, int start, int n, float *o)
{
    Array a = (Array) DXGetComponentValue(f, OPACITIES);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, OPACITIES, (Object)a);
	DXSetAttribute((Object)a, DEP, O_POSITIONS);
    }
    if (!DXAddArrayData(a, start, n, (Pointer)o))
	return NULL;
    return f;
}

Field
DXAddOpacity(Field f, int id, double o)
{
    float fo = o;
    return DXAddOpacities(f, id, 1, &fo);
}

Field
DXAddNormals(Field f, int start, int n, Vector *v)
{
    Array a = (Array) DXGetComponentValue(f, NORMALS);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, NORMALS, (Object) a);
	DXSetAttribute((Object)a, DEP, O_POSITIONS);
    }
    /* XXX - check dep of exsiting component */
    if (!DXAddArrayData(a, start, n, (Pointer) v))
	return NULL;
    return f;
}

Field
DXAddNormal(Field f, PointId id, Vector v)
{
    return DXAddNormals(f, id, 1, &v);
}

Field
DXAddFaceNormals(Field f, int start, int n, Vector *v)
{
    Array a = (Array) DXGetComponentValue(f, NORMALS);
    if (!a) {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (!a)
	    return NULL;
	DXSetComponentValue(f, NORMALS, (Object) a);
	DXSetAttribute((Object)a, DEP, O_CONNECTIONS);
    }
    /* XXX - check dep of exsiting component */
    if (!DXAddArrayData(a, start, n, (Pointer) v))
	return NULL;
    return f;
}

Field
DXAddFaceNormal(Field f, PointId id, Vector v)
{
    return DXAddFaceNormals(f, id, 1, &v);
}

Field
DXAddLines(Field f, int start, int n, Line *l)
{
    return add(f, start, n, (Pointer)l, O_LINES, 2)? f : NULL;
}

Field
DXAddLine(Field f, PointId id, Line l)
{
    return DXAddLines(f, id, 1, &l);
}

Field
DXAddTriangles(Field f, int start, int n, Triangle *t)
{
    return add(f, start, n, (Pointer)t, O_TRIANGLES, 3)? f : NULL;
}

Field
DXAddTriangle(Field f, PointId id, Triangle t)
{
    return DXAddTriangles(f, id, 1, &t);
}

Field
DXAddQuads(Field f, int start, int n, Quadrilateral *q)
{
    return add(f, start, n, (Pointer)q, O_QUADS, 4)? f : NULL;
}

Field
DXAddQuad(Field f, PointId id, Quadrilateral q)
{
    return DXAddQuads(f, id, 1, &q);
}

Field
DXAddTetrahedra(Field f, int start, int n, Tetrahedron *t)
{
    return add(f, start, n, (Pointer)t, O_TETRAHEDRA, 4)? f : NULL;
}

Field
DXAddTetrahedron(Field f, PointId id, Tetrahedron t)
{
    return DXAddTetrahedra(f, id, 1, &t);
}

Array
DXGetConnections(Field f, char *type)
{
    Array a;
    char *ct;
    a = (Array) DXGetComponentValue(f, CONNECTIONS);
    if (!a)
	return NULL;
    ct = DXGetString((String)DXGetAttribute((Object)a, ELEMENT_TYPE));
    if (!ct || strcmp(ct, type)!=0)
	return NULL;
    return a;
}

Field
DXSetConnections(Field f, char *type, Array a)
{
    if (!DXSetComponentValue(f, CONNECTIONS, (Object)a))
	return NULL;
    if (!DXSetAttribute((Object)a, ELEMENT_TYPE, _dxfstringobject(type, 0)))
	return NULL;
    return f;
}



/*
 * Object-level interface for sharing neighbors and box components
 * among objects that share positions and connections
 */

#define POSITIONS_ENTRY		0x01
#define CONNECTIONS_ENTRY	0x02

typedef struct 
{
    Object a; 	 /* array			    */
    int    t;    /* positions or connections entry  */
    Field  f; 	 /* prototypical field		    */
    Array  r; 	 /* result array (box or neighbors) */
} HElement;

#define PASS1	1
#define PASS2	2

/*
 * Traverse an object to the field level.  For each field we do one of two
 * things depending on the pass.  For pass1 we ensure that there's a hash 
 * element for each unique positions or connections component.  For pass2
 * we access the hash table based on the positions connections to get a 
 * bounding box component, and based on the connections component to get a
 * neighbors component (if one was created: recall that we don't create 
 * neighbors for regular connections.  In pass2 we also call DXEndField.
 */
static Error
_Recur(Object o, HashTable hTable, int pass)
{
    if (! o)
	return OK;

    switch(DXGetObjectClass(o))
    {
	case CLASS_GROUP:
	{
	    int i;
	    Object c;

	    i = 0;
	    while ((c = DXGetEnumeratedMember((Group)o, i++, NULL)) != NULL)
		if (! _Recur(c, hTable, pass))
		    goto error;
	    
	    break;
	}

	case CLASS_XFORM:
	{
	    Object c;

	    if (! DXGetXformInfo((Xform)o, &c, NULL))
		goto error;

	    if (! _Recur(c, hTable, pass))
		goto error;
		
	    break;
	}
	
	case CLASS_SCREEN:
	{
	    Object c;

	    if (! DXGetScreenInfo((Screen)o, &c, NULL, NULL))
		goto error;

	    if (! _Recur(c, hTable, pass))
		goto error;
	    
	    break;
	}
	
	case CLASS_CLIPPED:
	{
	    Object c;

	    if (! DXGetClippedInfo((Clipped)o, &c, NULL))
		goto error;

	    if (! _Recur(c, hTable, pass))
		goto error;

	    break;
	}

	case CLASS_FIELD:
	{
	    HElement helt, *hptr;
	    Field f = (Field)o;

	    if (DXEmptyField(f))
		break;
	    
	    helt.f = f;
	    helt.r = NULL;

	    if (pass == PASS1)
	    {
		helt.a = DXGetComponentValue(f, "positions");
		if (helt.a)
		{
		    helt.t = POSITIONS_ENTRY;

		    if (! DXQueryHashElement(hTable, (Key)&helt))
			if (! DXInsertHashElement(hTable, (Element)&helt))
			    goto error;
		}

		helt.a = DXGetComponentValue(f, "connections");
		if (helt.a)
		{
		    helt.t = CONNECTIONS_ENTRY;

		    if (! DXQueryHashElement(hTable, (Key)&helt))
			if (! DXInsertHashElement(hTable, (Element)&helt))
			    goto error;
		}
	    }
	    else
	    {
		Object str;

		helt.a = DXGetComponentValue(f, "positions");
		if (helt.a)
		{
		    hptr = (HElement *)DXQueryHashElement(hTable, (Key)&helt);
		    if (! hptr)
		    {
			DXSetError(ERROR_INTERNAL, 
			    "DXEndObject: positions entry missing in pass2");
			goto error;
		    }

		    if (hptr->t != POSITIONS_ENTRY)
		    {
			DXSetError(ERROR_INTERNAL, "DXEndObject: entry type error");
			goto error;
		    }

		    if (hptr->r)
		    {
			if (! DXSetComponentValue(f, "box", (Object)hptr->r))
			    goto error;
		    
			str = (Object)DXNewString("positions");
			if (! str)
			    goto error;

			if (! DXSetComponentAttribute(f, "box", "der", str))
			{
			    DXDelete(str);
			    goto error;
			}
		    }
		}

		helt.a = DXGetComponentValue(f, "connections");
		if (helt.a)
		{
		    hptr = (HElement *)DXQueryHashElement(hTable, (Key)&helt);
		    if (! hptr)
		    {
			DXSetError(ERROR_INTERNAL, 
			    "DXEndObject: connections entry missing in pass2");
			goto error;
		    }

		    if (hptr->t != CONNECTIONS_ENTRY)
		    {
			DXSetError(ERROR_INTERNAL, "DXEndObject: entry type error");
			goto error;
		    }

		    if (hptr->r)
		    {
			if (!DXSetComponentValue(f, "neighbors", (Object)hptr->r))
			    goto error;
		
			str = (Object)DXNewString("connections");
			if (! str)
			    goto error;

			if (! DXSetComponentAttribute(f, "neighbors", "dep", str))
			{
			    DXDelete(str);
			    goto error;
			}
					    
			if (! DXSetComponentAttribute(f, "neighbors", "ref", str))
			{
			    DXDelete(str);
			    goto error;
			}
		    }
		}

#ifndef DO_ENDFIELD
		if (! DXEndField(f))
		    goto error;
#endif

	    }

	    break;
	}

	default:
	    break;
    }

    return OK;

error:
    return ERROR;
}

/* 
 * Task executed for each hash table entry.  If the entry is of type
 * POSITIONS_ENTRY, then compute the bounding box and stick it into the
 * result slot of the hash element.  Otherwise, it ought to be of type
 * CONNECTIONS_ENTRY, so we compute a neighbors component and put it into
 * the hash element.
 */
static Error
EndObjectTask(Pointer ptr)
{
    HElement *hPtr = *(HElement **)ptr;

    if (hPtr->t == POSITIONS_ENTRY)
    {
	if (DXBoundingBox((Object)(hPtr->f), NULL))
	{
	    hPtr->r = (Array)DXGetComponentValue(hPtr->f, "box");
	    if (! hPtr->r)
	    {
		if (DXGetError() == ERROR_NONE)
		    DXSetError(ERROR_INTERNAL, 
		      "DXBoundingBox succeeded but no box component was found");
		goto error;
	    }
	}
	else
	{
	    if (DXGetError() != ERROR_NONE)
		goto error;
	    
	    hPtr->r = NULL;
	}
    }
    else if (hPtr->t == CONNECTIONS_ENTRY)
    {
	hPtr->r = DXNeighbors(hPtr->f);

	if (DXGetError() != ERROR_NONE)
	    goto error;
    }
    else
    {
	DXSetError(ERROR_INTERNAL, "unknown entry type");
	goto error;
    }

    return OK;

error:
    return ERROR;
}

Object
DXEndObject(Object o)
{
    HashTable hTable = NULL;
    HElement *elt;

    hTable = DXCreateHash(sizeof(HElement), NULL, NULL);
    if (! hTable)
	goto error;
    
    /*
     * Traverse the input, creating a hash entry of type POSITIONS_ENTRY
     * for each unique positions array and a entry of type CONNECTIONS_ENTRY
     * for each unique connections array.
     */
    if (! _Recur(o, hTable, PASS1))
	goto error;
    
    /*
     * Traverse the hash table.  For each element of type CONNECTIONS_ENTRY,
     * create a task that will create a neighbors array using the template
     * field.
     */
    if (! DXCreateTaskGroup())
	goto error;

    DXInitGetNextHashElement(hTable);
    while ((elt = (HElement *)DXGetNextHashElement(hTable)) != NULL)
    {
	if (elt->t == CONNECTIONS_ENTRY)
	    if (! DXAddTask(EndObjectTask, (Pointer)&elt, sizeof(Pointer), 1))
	    {
		DXAbortTaskGroup();
		goto error;
	    }
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    /*
     * Traverse the hash table again.  For each element of type
     * CONNECTIONS_ENTRY, * create a task that will create a box array
     * using the template field.
     */
    if (! DXCreateTaskGroup())
	goto error;

    DXInitGetNextHashElement(hTable);
    while ((elt = (HElement *)DXGetNextHashElement(hTable)) != NULL)
    {
	if (elt->t == POSITIONS_ENTRY)
	    if (! DXAddTask(EndObjectTask, (Pointer)&elt, sizeof(Pointer), 1))
	    {
		DXAbortTaskGroup();
		goto error;
	    }
    }

    if (! DXExecuteTaskGroup() || DXGetError() != ERROR_NONE)
	goto error;
    
    /*
     * Traverse the input again.  For each field, access the hash table keyed 
     * by the positions component address to get a box component and by the
     * connections component address to get a neighbors component.  Then call
     * DXEndField on each.
     */
    if (! _Recur(o, hTable, PASS2))
	goto error;
    
    DXDestroyHash(hTable);
    
    return o;

error:
    DXDestroyHash(hTable);
    return NULL;
}



