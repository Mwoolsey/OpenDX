/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


 
#include <dx/dx.h>
#include <string.h>
#include <math.h>
#include "verify.h"
#include "../libdx/edf.h"

static Error Check(Object o);
static Error GroupCheck(Group g);
static Error FieldCheck(Field f);

static Error PositionsCheck(Array a);
static char *ConnectionsCheck(Array a);
static Error AreTetrasOK(Array pos, Array conn);
static Error AreTrisOK(Array pos, Array conn);
static Error AreLinesOK(Array pos, Array conn);

static int TetraVol(Point *p, Point *q, Point *r, Point *s);

#if 0
static int TriArea(Point *p, Point *q, Point *r);
#endif

 
/* look for malformed objects and print messages about them.
 */
int
m_Verify(Object *in, Object *out)
{
    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }

    if (_dxfIsCircular(in[0]) == ERROR)
	return ERROR;

    if (Check(in[0]) == ERROR)
	return ERROR;
    
    out[0] = in[0];
    return OK;
}
 


static Error Check(Object o)
{
    Object subo, subo2;
    Matrix m;
    int fixed, z;
    int i;

    switch (DXGetObjectClass(o)) {

      case CLASS_GROUP:
	/* for each member */
	for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, NULL)); i++)
	    if (!Check(subo))
		return ERROR;

	if (!GroupCheck((Group)o))
	    return ERROR;

	break;
	    
      case CLASS_FIELD:

	if (!_dxfValidate((Field)o))
	    return ERROR;

	if (!FieldCheck((Field)o))
	    return ERROR;

	break;
	
      case CLASS_ARRAY:
	break;

      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &subo, &fixed, &z))
	    return ERROR;

	if (!Check(subo))
	    return ERROR;
	
	break;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &subo, &m))
	    return ERROR;
	
	if (!Check(subo))
	    return ERROR;
	
	break;
	
      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &subo, &subo2))
	    return ERROR;
	
	if (!Check(subo))
	    return ERROR;
	
	if (!Check(subo2))
	    return ERROR;
	
	break;

      default:

	break;
    }

    return OK;
}

struct mustmatch {
    Field f;
    int member;
    Array p;
    int posshape;
    Array c;
    char *contype;
    int isreg;
    int meshoff[MAXRANK];
};

static Error stuffstruct(struct mustmatch *m, Object o, int memnumber)
{
    int r, s[MAXRANK];

    /* is this too draconian? */
    if (DXGetObjectClass(o) != CLASS_FIELD) {
	DXSetError(ERROR_DATA_INVALID, 
		   "composite field members must be fields");
	return ERROR;
    }
    m->f = (Field)o;
    m->member = memnumber;

    m->p = (Array)DXGetComponentValue(m->f, "positions");
    if (m->p) {
	if (!DXGetArrayInfo(m->p, NULL, NULL, NULL, &r, s))
	    return ERROR;
	m->posshape = s[0];
    } else
	m->posshape = 0;

    m->c = (Array)DXGetComponentValue(m->f, "connections");
    if (m->c) {
	if (!DXGetStringAttribute((Object)m->c, "element type", &m->contype)) {
	    DXSetError(ERROR_DATA_INVALID, 
   "'connections' component is required to have an 'element type' attribute");
	    return ERROR;
	}
	if (DXGetArrayClass(m->c) == CLASS_MESHARRAY) {
	    m->isreg++;
	    if (!DXGetMeshOffsets((MeshArray)m->c, m->meshoff))
		return ERROR;
	} else
	    m->isreg = 0;
    } else
	m->contype = NULL;

    return OK;
}

static Error cmpstruct(struct mustmatch *m1, struct mustmatch *m2)
{
    int i;

    /* if they both have positions, make sure the dimensionality matches.
     */
    if (m1->p && m2->p) {
	if (m1->posshape != m2->posshape) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "all members of a composite field must have the same positions dimensionality.  member %d is %dD, member %d is %dD", 
		       m1->member, m1->posshape, m2->member, m2->posshape);
	    return ERROR;
	}
    }
    
    /* if neither has connections (both are scattered points fields)
     *  that's fine and we are done.
     */
    if (!m1->c && !m2->c)
	return OK;
    
    /* do they both have a connections component?
     */
    if (!m1->c || !m2->c) {
	DXSetError(ERROR_DATA_INVALID,
		   "all members of a composite field must have the same type of connections.  member %d has %s connection component, member %d has %s connection component",
		   m1->member, m1->c ? "a" : "no", 
		   m2->member, m2->c ? "a" : "no");
	return ERROR;
    }
    /* does the element type match?
     */
    if (!m1->contype || !m2->contype || strcmp(m1->contype, m2->contype)) {
	DXSetError(ERROR_DATA_INVALID,
		   "all members of a composite field must have the same type of connections.  member %d has %s 'element type' attribute, member %d has %s 'element type' attribute",
		   m1->member, m1->contype ? m1->contype : "no", 
		   m2->member, m2->contype ? m2->contype : "no");
	return ERROR;
    }
    /* do the meshoffsets make sense if they are part of a reg grid? 
     */
    /* this isn't enough.  we need to compare all the meshoffsets with
     * all the other components and make sure they don't overlap.  the
     * cheap test here will catch if any are identical to the first non-
     * empty group member.  since none should be the same (and 0,0,0 is
     * the most likely value), this might help some.
     */
    if (m1->isreg && m2->isreg) {
	for (i=0; i<m1->posshape; i++) {
	    if (m1->meshoff[i] != m2->meshoff[i])
		break;
	}
	if (i == m1->posshape) {
	    DXSetError(ERROR_DATA_INVALID, 
		       "composite field members %d and %d should not have identical connection mesh offset values",
		       m1->member, m2->member);
	    return ERROR;
	}
    }

    return OK;
}

static Error GroupCheck(Group g)
{
    Object subo;
    struct mustmatch first, next;
    int needone = 1;
    int i;

    switch (DXGetGroupClass(g)) {
      case CLASS_COMPOSITEFIELD:

	for (i=0; (subo=DXGetEnumeratedMember(g, i, NULL)); i++) {
	    /* get info from first nonempty member */
	    if (needone) {
		if (!stuffstruct(&first, subo, i))
		    return ERROR;
		if (first.p != NULL) {
		    needone = 0;
		    continue;
		}
	    }
	
	    /* get info from subo member */
	    if (!stuffstruct(&next, subo, i))
		return ERROR;

	    if (next.p == NULL)
		continue;

	    /* make sure the positions are all the same shape, and that
	     *  all the connections are the same type, and that if the
	     *  positions are regular, that the mesh offsets make sense.
	     */
	    if (!cmpstruct(&first, &next))
		return ERROR;
	}

	break;

      case CLASS_MULTIGRID:
	/* ? */
	break;

      case CLASS_SERIES:
	/* warn if series positions are not monotonic? */
	break;

      default:  /* generic group, or new group type */
	/* no restrictions */
	break;
    }

    return OK;
}


static Error 
FieldCheck(Field f)
{
    Array subo, pos;
    char *name;
    char *elemtype;
    int haspositions = 0;
    int i;

    pos = (Array)DXGetComponentValue(f, "positions");
    if (pos)
	haspositions++;
    
    for (i=0; (subo=(Array)DXGetEnumeratedComponentValue(f, i, &name)); i++) {
	
	if (DXGetObjectClass((Object)subo) != CLASS_ARRAY) {
	    DXSetError(ERROR_DATA_INVALID, 
		     "field component `%s' not an array object", name);
	    return ERROR;
	}
	
	if (!strcmp(name, "positions")) {
	    if (!PositionsCheck(subo))
		return ERROR;
	    continue;
	}
	
	if (!strcmp(name, "connections")) {
	    if (!haspositions) {
		DXSetError(ERROR_DATA_INVALID, 
			   "field has 'connections' but no 'positions'");
		return ERROR;
	    }

	    if (!(elemtype = ConnectionsCheck(subo)))
		return ERROR;
	    
	    if (!strcmp(elemtype, "lines")) {
		if (!AreLinesOK(pos, subo))
		    return ERROR;
		continue;
	    }
	    if (!strcmp(elemtype, "quads")) {
		continue;
	    }
	    if (!strcmp(elemtype, "cubes")) {
		continue;
	    }
	    if (!strcmp(elemtype, "triangles")) {
		if (!AreTrisOK(pos, subo))
		    return ERROR;
		continue;
	    }
	    if (!strcmp(elemtype, "tetrahedra")) {
		if (!AreTetrasOK(pos, subo))
		    return ERROR;
		continue;
	    }
	}
	
	
	if (!strcmp(name, "faces")) {
	    continue;
	}
	if (!strcmp(name, "loops")) {
	    continue;
	}
	if (!strcmp(name, "edges")) {
	    continue;
	}
    }
    
    return OK;
}

static Error
PositionsCheck(Array a)
{
    int items;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];

    if (!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
	return ERROR;

    if (type != TYPE_FLOAT) {
	DXSetError(ERROR_DATA_INVALID, "positions must be type float");
	return ERROR;
    }

    if (cat != CATEGORY_REAL) {
	DXSetError(ERROR_DATA_INVALID, "positions must be category real");
	return ERROR;
    }
    
    if (rank != 1) {
	DXSetError(ERROR_DATA_INVALID, "positions must be a vector");
	return ERROR;
    }

    return OK;
	
}


static char *
ConnectionsCheck(Array a)
{
    int items;
    Type type;
    Category cat;
    int rank, shape[MAXRANK];
    char *elemtype;

    if (!DXGetArrayInfo(a, &items, &type, &cat, &rank, shape))
	return ERROR;

    if (type != TYPE_INT) {
	DXSetError(ERROR_DATA_INVALID, "connections must be type integer");
	return ERROR;
    }

    if (cat != CATEGORY_REAL) {
	DXSetError(ERROR_DATA_INVALID, "connections must be category real");
	return ERROR;
    }
    
    if (rank != 1) {
	DXSetError(ERROR_DATA_INVALID, "connections must be a vector");
	return ERROR;
    }

    if (!DXGetStringAttribute((Object)a, "element type", &elemtype)) {
	DXSetError(ERROR_DATA_INVALID, 
		 "`connections' component has no `element type' attribute");
	return ERROR;
    }

    return elemtype;
}

static Error
AreTetrasOK(Array pos, Array conn)
{
    int pitems, citems;
    int prank, pshape[MAXRANK];
    int crank, cshape[MAXRANK];
    int i, j, *ip;
    int handed = 0, this;
    Point *pp;

    if (!DXGetArrayInfo(pos, &pitems, NULL, NULL, &prank, pshape))
	return ERROR;

    if (prank != 1 || pshape[0] != 3) {
	DXSetError(ERROR_DATA_INVALID, 
		   "tetrahedra positions must be 3-vectors");
	return ERROR;
    }

    if (pitems == 0) {
	DXWarning("positions array contains no items");
	return OK;
    }

    pp = (Point *)DXGetArrayData(pos);
    if (!pp)
	return ERROR;

    if (!DXGetArrayInfo(conn, &citems, NULL, NULL, &crank, cshape))
	return ERROR;

    if (crank != 1 || cshape[0] != 4) {
	DXSetError(ERROR_DATA_INVALID, 
		   "tetrahedra connections must be 4-vectors");
	return ERROR;
    }
	
    if (citems == 0) {
	DXWarning("connections array contains no items");
	return OK;
    }

    ip = (int *)DXGetArrayData(conn);
    if (!ip)
	return ERROR;

    for (i=0; i<citems; i++, ip+=4) {
	for (j=0; j<4; j++) {
	    if (ip[j] < 0 || ip[j] >= pitems) {
		DXSetError(ERROR_DATA_INVALID, 
		    "connection index %d out of range (%d); must be 0 to %d",
			   i, ip[j], pitems-1);
		return ERROR;
	    }
	}

	/* find the first tetra which is has either positive or negative
	 *  volume, and make sure all others agree.
	 */
	this = TetraVol(pp+ip[0], pp+ip[1], pp+ip[2], pp+ip[3]);
	if (this && !handed) {
	    handed = this;
	    continue;
	}

	if (this && this != handed) {
	    DXSetError(ERROR_DATA_INVALID, 
		"the positions in tetrahedra %d are ordered inconsistently compared to other tetrahedra in this field",
		       i);
	    return ERROR;
	}
    }

    return OK;
}

static Error
AreTrisOK(Array pos, Array conn)
{
    int pitems, citems;
    int prank, pshape[MAXRANK];
    int crank, cshape[MAXRANK];
    int i, j, *ip;
    Point *pp;

    if (!DXGetArrayInfo(pos, &pitems, NULL, NULL, &prank, pshape))
	return ERROR;

    if (prank != 1 || (pshape[0] != 2 && pshape[0] != 3)) {
	DXSetError(ERROR_DATA_INVALID, 
		   "triangle positions must be 2- or 3-vectors");
	return ERROR;
    }

    if (pitems == 0) {
	DXWarning("positions array contains no items");
	return OK;
    }

    pp = (Point *)DXGetArrayData(pos);
    if (!pp)
	return ERROR;

    if (!DXGetArrayInfo(conn, &citems, NULL, NULL, &crank, cshape))
	return ERROR;

    if (crank != 1 || cshape[0] != 3) {
	DXSetError(ERROR_DATA_INVALID, 
		   "triangle connections must be 3-vectors");
	return ERROR;
    }
	
    if (citems == 0) {
	DXWarning("connections array contains no items");
	return OK;
    }

    ip = (int *)DXGetArrayData(conn);
    if (!ip)
	return ERROR;

    for (i=0; i<citems; i++, ip+=3) {
	for (j=0; j<3; j++) {
	    if (ip[j] < 0 || ip[j] >= pitems) {
		DXSetError(ERROR_DATA_INVALID, 
		    "connection index %d out of range (%d); must be 0 to %d",
			   i, ip[j], pitems-1);
		return ERROR;
	    }
	}

#if 0
	/* this doesn't mean anything */
	if (!TriArea(pp+ip[0], pp+ip[1], pp+ip[2])) {
	    DXSetError(ERROR_DATA_INVALID, 
		 "the positions in triangle %d are ordered inconsistently",
		       i);
	    return ERROR;
	}
#endif

    }

    return OK;
}


static Error
AreLinesOK(Array pos, Array conn)
{
    int pitems, citems;
    int prank, pshape[MAXRANK];
    int crank, cshape[MAXRANK];
    int i, j, *ip;
    float *fp;

    if (!DXGetArrayInfo(pos, &pitems, NULL, NULL, &prank, pshape))
	return ERROR;

    if (prank != 1 || pshape[0] < 1 || pshape[0] > 3) {
	DXSetError(ERROR_DATA_INVALID, 
		   "line positions must be 1-, 2- or 3-vectors");
	return ERROR;
    }

    if (pitems == 0) {
	DXWarning("positions array contains no items");
	return OK;
    }

    fp = (float *)DXGetArrayData(pos);
    if (!fp)
	return ERROR;

    if (!DXGetArrayInfo(conn, &citems, NULL, NULL, &crank, cshape))
	return ERROR;

    if (crank != 1 || cshape[0] != 2) {
	DXSetError(ERROR_DATA_INVALID, 
		   "line connections must be 2-vectors");
	return ERROR;
    }
	
    if (citems == 0) {
	DXWarning("connections array contains no items");
	return OK;
    }

    ip = (int *)DXGetArrayData(conn);
    if (!ip)
	return ERROR;

    for (i=0; i<citems; i++, ip+=2) {
	for (j=0; j<2; j++) {
	    if (ip[j] < 0 || ip[j] >= pitems) {
		DXSetError(ERROR_DATA_INVALID, 
		    "connection index %d out of range (%d); must be 0 to %d",
			   i, ip[j], pitems-1);
		return ERROR;
	    }
	}

    }

    return OK;
}

/* compute the volume of the tetra and see if it is a positive
 *  or negative number.  this returns -1, 0 or 1 for negative, too small
 *  to tell, and positive, respectively.  
 *  if the volume is negative, switching the order of any two of
 *  the point indicies will fix the problem.
 */
static int TetraVol(Point *p, Point *q, Point *r, Point *s)
{
    double xqp, yqp, zqp;
    double xrp, yrp, zrp;
    double xsp, ysp, zsp;
    double  vx,  vy,  vz;
    double vol;

    xqp = q->x - p->x; yqp = q->y - p->y; zqp = q->z - p->z;
    xrp = r->x - p->x; yrp = r->y - p->y; zrp = r->z - p->z;
    xsp = s->x - p->x; ysp = s->y - p->y; zsp = s->z - p->z;

    vx = (yrp * zqp) - (yqp * zrp);
    vy = (zrp * xqp) - (zqp * xrp);
    vz = (xrp * yqp) - (xqp * yrp);

    vol = vx*xsp + vy*ysp + vz*zsp;

    if (fabs(vol) < DXD_FLOAT_EPSILON)
	return 0;

    return (vol < 0.0) ? -1 : 1;
}

#if 0
/* 
 */
static int TriArea(Point *p, Point *q, Point *r)
{
    double xpq, ypq;
    double xrq, yrq;
    double area;

    xpq = p->x - q->x; ypq = p->y - q->y;
    xrq = r->x - q->x; yrq = r->y - q->y;

    area = (xpq * yrq) - (xrq * ypq);
    
    if (fabs(area) < DXD_FLOAT_EPSILON)
	return 0;

    return (area < 0.0) ? -1 : 1;
}
#endif


/*
 * try to detect circular references.  attrs can be references to things
 *  further up the hierarchy, but direct descendents (like components or
 *  group members) can't refer back to something in their parent chain.
 */
struct plist {
    Object this;
    struct plist *next;
};

static Error cpath(struct plist *p, Object o);
static Error pushchild(struct plist *p, Object newo);
static void popchild(struct plist *p);

Error _dxfIsCircular(Object o)
{
    struct plist pl;
    
    pl.this = o;
    pl.next = NULL;

    return cpath(&pl, o);
}

static Error cpath(struct plist *p, Object o)
{
    Object child = NULL;
    char *name = NULL;
    int i;

    switch (DXGetObjectClass(o)) {

      case CLASS_GROUP:
	for (i=0; (child=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
	    if (pushchild(p, child) == ERROR) {
		DXAddMessage("member %d", i);
		return ERROR;
	    }
	    popchild(p);
	}
	return OK;

      case CLASS_FIELD:
	for (i=0; (child=DXGetEnumeratedComponentValue((Field)o, i, &name)); i++) {
	    if (pushchild(p, child) == ERROR) {
		DXAddMessage("member %d", i);
		return ERROR;
	    }
	    popchild(p);
	}
	return OK;
	
      case CLASS_SCREEN:
	if (!DXGetScreenInfo((Screen)o, &child, NULL, NULL))
	    return ERROR;
	
	if (pushchild(p, child) == ERROR) {
	    DXAddMessage("screen object");
	    return ERROR;
	}
	popchild(p);

	return OK;

      case CLASS_CLIPPED:
	if (!DXGetClippedInfo((Clipped)o, &child, NULL))
	    return ERROR;

	if (pushchild(p, child) == ERROR) {
	    DXAddMessage("clipped object");
	    return ERROR;
	}
	popchild(p);

	if (!DXGetClippedInfo((Clipped)o, NULL, &child))
	    return ERROR;

	if (pushchild(p, child) == ERROR) {
	    DXAddMessage("clipping object");
	    return ERROR;
	}
	popchild(p);

	return OK;

      case CLASS_XFORM:
	if (!DXGetXformInfo((Xform)o, &child, NULL))
	    return ERROR;

	if (pushchild(p, child) == ERROR) {
	    DXAddMessage("transform object");
	    return ERROR;
	}
	popchild(p);

	return OK;

      /* all other objects can't contain others */
      default:
	return OK;

    }

    /* not reached */
}

#if 0
/* private for debugging 
 */
static void printplist(struct plist *p)
{
    struct plist *pp;

    pp = p;
    do {
	DXMessage("p = 0x%08x: object = 0x%08x, next = 0x%08x", 
		  (uint)pp, (uint)pp->this, (uint)pp->next);
	pp = pp->next;
    } while (pp);
}
#endif

/* check list on the way down.  if you find a duplicate, return error.
 *  otherwise alloc space for a new entry and add this object to the
 *  end of the list.
 */
static Error pushchild(struct plist *p, Object newo)
{
    struct plist *pl, *pp;

    pp = p;
    do {
	if (newo == pp->this) {
	    DXSetError(ERROR_DATA_INVALID, "circular reference in object");
	    return ERROR;
	}
	if (pp->next)
	    pp = pp->next;
    } while (pp->next);

    pl = (struct plist *)DXAllocateLocal(sizeof(struct plist));
    if (!pl) 
	return ERROR;

    pl->this = newo;
    pl->next = NULL;

    pp->next = pl;

    return cpath(p, newo);
}

/* remove the last entry from the list.
 */
static void popchild(struct plist *p)
{
    struct plist *pp;

    pp = p;
    while (pp->next) {
	if (!pp->next->next) {
	    DXFree((Pointer)pp->next);
	    pp->next = NULL;
	    return;
	}
	pp = pp->next;
    }
    /* not reached */
}

