/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <dx/dx.h>

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#include "_normals.h"

/*
 * 
 * data structures, constants and prototypes
 *
 */

#define MAXDEPTH 200
#define MAXRANK  100

/* info about where and how to draw each object
 */
typedef struct construct {
    Class class;                /* object class determines shape */
    int isattr;                 /* this object is an attribute of another */
    int isshared;               /* this object is used more than once */
    Point center;		/* center of this objects representation */
    float di, dj, dk; 		/* size of the node box */
    Point in, out;              /* location where attachment lines connect */
} Construct;

/* info about how to do the general layout
 */
typedef struct policy {
    int vertical;               /* vs. horizontal */
    int version;                /* test code */
} Policy;

/* your basic linked list
 */
typedef struct nlist {
    struct node *this;
    struct nlist *next;
} Nlist;

/* one of these nodes per object in the input, storing the
 *  type and relationship between the objs.
 */
typedef struct node {
    int level;			/* depth from the top object */
    int levelnumber;		/* which member of my level */
    int isattr;			/* am i an attribute of my parent? */
    int attrnumber;		/* which attribute of my parent */
    int descendents;            /* number of child objects (ex. attrs) */
    int predescend;             /* accum width preceeding my first child */
    int lastdescend;            /* accum width at end of my last child */
    struct node *attr;		/* my first attribute */
    struct node *child;		/* my first child */
    struct node *sibling;	/* the next object at my level */
    struct node *parent;	/* my parent object */
    Nlist *godparent;		/* if shared, more parent objects */
    char *ptag;                 /* text for this obj from parent */
    char *ctag;                 /* text for this obj describing child */
    Construct where;            /* where and how big to draw this object */
} Node;

/* overall stats which are needed to do layout
 */
typedef struct layout {
    int level;                  /* current depth from top obj */
    int maxlevel;               /* deepest depth from top obj to bottom */
    int membercount;            /* total number of objects */
    int uniquemembers;          /* number of unique objects */
    int levelmembers[MAXDEPTH]; /* number of members per level */
    int maxlevelmembers;        /* largest number of members at any level */
    int widestlevel;            /* which level contains the most members */
} Layout;

/* pointers directly to the next available location in each array.
 */
typedef struct quick {
    Point *pos;			/* pointer into positions array */
    int *conn;			/* pointer into connections array */
    RGBColor *color;		/* pointer into colors array */
} Quick;

/* output object information
 */
typedef struct outobjinfo {
    Group g;                    /* output object */
    Object font;		/* font for labels */

    Field fquads;               /* field of quads (is this needed?) */
    int totalquads;             /* total number of quads in output */
    int nquads;                 /* current number of quads */
    Quick quaddata;             /* pointers to speed access */

    Field flines;               /* field of lines */
    int totallines;             /* total number of lines in output */
    int nlines;                 /* current number of lines */
    Quick linedata;             /* pointers to speed access */

    Field ftris;                /* field of triangles */
    int totaltris;              /* total number of triangles in output */
    int ntris;                  /* current number of triangles */
    Quick tridata;              /* pointers to speed access */

    Field fpoints;              /* field of scattered points */
    int totalpoints;		/* total number of scattered pts in output */
    int npoints;                /* current number of scattered points */
    Quick pointdata;		/* pointers to speed access */

} OutObjInfo;

/* input object information
 */
typedef struct inobjinfo {
    Layout layout;              /* overall stats on object */
    HashTable ht;               /* used to determine if objects are shared */
    Policy policy;              /* overall layout policy */
    Node *root;                 /* root of node tree */
} InObjInfo;

/*  single handle to make it easy to pass data structs around
 */
typedef struct visinfo {
    InObjInfo oi;		/* the input object we are trying to see */
    OutObjInfo oo;		/* the output object we are creating */
} VisInfo;


/* the three basic processing passes:
 *
 *  build a structure representing each object and it's relationship
 *   with other objects, and accumuate stats on the overall number of
 *   nodes, their size, the amount of geometry it is going to take to
 *   represent this visually.
 *
 *  decide the layout of the nodes
 *
 *  convert that struct into geometry, with the option of each object
 *   type being a different shape, size and color.
 *
 */  
static Error traverse (Object o, VisInfo *vi, Node *parent, 
		       int isattr, int num, char *ptag);
static Error layout (VisInfo *vi);
static Object makevisual (VisInfo *vi);

static Error initVisInfo (VisInfo *vi);         /* create & init new */
static Error initInObjInfo (InObjInfo *oi);
static Error initOutObjInfo (OutObjInfo *oo);
static void  endVisInfo (VisInfo *vi);          /* normal exit */
static void  endInObjInfo (InObjInfo *oi);
static void  endOutObjInfo (OutObjInfo *oo);
static void  errdelVisInfo (VisInfo *vi);       /* error exit */
static void  errdelInObjInfo (InObjInfo *oi);
static void  errdelOutObjInfo (OutObjInfo *oo);


static Node *newnode (int level, int isattr, int number, int dup,
		      Class class, Node *parent, char *ptag);
static Error freenodetree (Node *np);
static int nodeplace (Node *np, Policy *p, Layout *l, OutObjInfo *oo);
static void attrplace (Node *np, Policy *p, Layout *l, OutObjInfo *oo);
static void nodecenter (Node *np, Policy *p, Layout *l, int isShared);
static Error drawObject (OutObjInfo *oo, Node *np, Policy *p);
static Error setptag(Node *np, char *ptag);
static Error setctag(Node *np, char *ctag);

static Error setpolicyversion (VisInfo *vi, int version);
static Error setpolicyvertical (VisInfo *vi, int vertical);

/* needed anymore? */
#if 0
static Error addparent (Node *me, Node *parent);
static int numnodes (Node *np);
#endif

static void nodecount (Node *np, int *count);

/* these need overhaul now */
static Error initgroup (OutObjInfo *oo);

static Error allocoutobj (OutObjInfo *oo);
static Error nextoutobj (OutObjInfo *oo, Node *np, Policy *p);
static Error addCube (OutObjInfo *oo, Node *np, RGBColor *c);
static Error addLabel (Group g, Object font, Construct *c, Policy *p);
static Error addStringTag (char *s, int type, Group g, Object font, 
			   Construct *c, Policy *p);

static void geomCount (OutObjInfo *oo, Class class, int isAttr, int isShared);

/*
 * 
 * subroutines to determine if an object is shared
 *
 */


/* duplicate object section
 */

/* (should this be moved to libdx at some point?) */

struct hashElement
{
    Pointer   key;	/* either an object or an id */
    Pointer   data;	/* the other thing */
};

typedef struct hashElement *HashElement;

/* looks for entry keyed by object memory address.  if found, sets 
 *  the id associated with that object.  if not found, sets id to 0.
 *  after determining the object isn't already in the list, AddID should be 
 *  called to put the new object and ID in the list.
 */
static HashTable FindIDbyObject(HashTable ht, Object object, ulong *id)
{
    HashElement eltPtr;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&object);
    *id = eltPtr ? (ulong)eltPtr->data : 0;

    return ht;
}


static HashTable AddIDbyObject(HashTable ht, Object object, ulong id)
{
    struct hashElement elt;
    HashElement        eltPtr;

    elt.key  = (Pointer)object;
    elt.data = (Pointer)id;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&object);
    if (eltPtr)
	return NULL;   /* shouldn't already be there */

    if (! DXInsertHashElement(ht, (Element)&elt))
	return NULL;
    
    return ht;
}

/* looks for entry keyed by ID and sets object to the object memory address
 *  associated with it, if present.  if not found, sets object to NULL.
 *  the AddObject function, below, is used to add an object to the
 *  table by ID.
 */
#if 0
static HashTable FindObjectbyID(HashTable ht, ulong id, Object *object)
{
    HashElement eltPtr;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&id);
    *object = eltPtr ? (Object)eltPtr->data : NULL;

    return ht;
}

static HashTable AddObjectbyID(HashTable ht, ulong id, Object object)
{
    struct hashElement elt;
    HashElement        eltPtr;

    elt.key  = (Pointer)id;
    elt.data = (Pointer)object;

    eltPtr = (HashElement)DXQueryHashElement(ht, (Key)&id);
    if (eltPtr)
	return NULL;  /* shouldn't already be there */

    if (! DXInsertHashElement(ht, (Element)&elt))
	return NULL;
    
    return ht;
}
#endif


/* 
 * data struct manipulation routines
 */

static Error initVisInfo(VisInfo *vi)
{
    if (initInObjInfo(&vi->oi) == ERROR)
	return ERROR;

    if (initOutObjInfo(&vi->oo) == ERROR)
	return ERROR;

    return OK;
}

static Error initInObjInfo(InObjInfo *oi)
{
    memset((char *)oi, '\0', sizeof(*oi));
    
    if (!(oi->ht = DXCreateHash(sizeof(struct hashElement), NULL, NULL)))
	return ERROR;
    
    oi->layout.level = -1;
    oi->policy.vertical = 1;

    return OK;
}

static Error initOutObjInfo(OutObjInfo *oo)
{
    memset((char *)oo, '\0', sizeof(*oo));

    /* make the groups, fields and arrays, but don't allocate
     *  space in the arrays until you know how many items they will
     *  eventually hold.
     */
    if (!initgroup(oo))
	return ERROR;

    if (!(oo->font = DXGetFont("variable", NULL, NULL)))
	return ERROR;

    return OK;
}

static void endVisInfo(VisInfo *vi)
{
    endInObjInfo(&vi->oi);
    endOutObjInfo(&vi->oo);
}

static void endInObjInfo(InObjInfo *oi)
{
    DXDestroyHash(oi->ht);
    freenodetree(oi->root);
}

static void endOutObjInfo(OutObjInfo *oo)
{
    if (!oo)
	return;

    if (oo->g)
	DXEndObject ((Object) oo->g);

    DXDelete (oo->font);
}

static void errdelVisInfo(VisInfo *vi)
{
    errdelInObjInfo (&vi->oi);
    errdelOutObjInfo (&vi->oo);
}

static void errdelInObjInfo(InObjInfo *oi)
{
    DXDestroyHash (oi->ht);
    freenodetree (oi->root);
}

static void errdelOutObjInfo(OutObjInfo *oo)
{
    if (!oo)
	return;

    DXDelete ((Object)oo->g);
    DXDelete ((Object)oo->font);
}

static Error initgroup(OutObjInfo *oo)
{
    Field f;
    Array a=NULL;

    /* top level group */
    if (!(oo->g = DXNewGroup()))
	return ERROR;
    
    /* quads */
    if (!(f = DXNewField()))
	goto error;

    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetComponentValue(f, "positions", (Object)a))
	goto error;
    a = NULL;

    if (!(a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 4)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "element type", "quads"))
	goto error;
    if (!DXSetStringAttribute((Object)a, "ref", "positions"))
	goto error;
    if (!DXSetComponentValue(f, "connections", (Object)a))
	goto error;
    a = NULL;
    
    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "dep", "connections"))
	goto error;
    if (!DXSetComponentValue(f, "colors", (Object)a))
	goto error;
    a = NULL;

    if (!DXSetMember(oo->g, "quads", (Object)f))
	goto error;
    oo->fquads = f;
    f = NULL;


    /* lines */
    if (!(f = DXNewField()))
	return ERROR;

    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetComponentValue(f, "positions", (Object)a))
	goto error;
    a = NULL;

    if (!(a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "element type", "lines"))
	goto error;
    if (!DXSetStringAttribute((Object)a, "ref", "positions"))
	goto error;
    if (!DXSetComponentValue(f, "connections", (Object)a))
	goto error;
    a = NULL;
    
    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "dep", "connections"))
	goto error;
    if (!DXSetComponentValue(f, "colors", (Object)a))
	goto error;
    a = NULL;
    
    if (!DXSetMember(oo->g, "lines", (Object)f))
	goto error;
    oo->flines = f;
    f = NULL;
    
    
    /* tris */
    if (!(f = DXNewField()))
	return ERROR;

    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetComponentValue(f, "positions", (Object)a))
	goto error;
    a = NULL;

    if (!(a = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "element type", "triangles"))
	goto error;
    if (!DXSetStringAttribute((Object)a, "ref", "positions"))
	goto error;
    if (!DXSetComponentValue(f, "connections", (Object)a))
	goto error;
    a = NULL;
    
    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "dep", "connections"))
	goto error;
    if (!DXSetComponentValue(f, "colors", (Object)a))
	goto error;
    a = NULL;

    if (!DXSetMember(oo->g, "triangles", (Object)f))
	goto error;
    oo->ftris = f;
    f = NULL;

    
    /* points */
    if (!(f = DXNewField()))
	return ERROR;

    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetComponentValue(f, "positions", (Object)a))
	goto error;
    a = NULL;

    if (!(a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3)))
	goto error;
    if (!DXSetStringAttribute((Object)a, "dep", "positions"))
	goto error;
    if (!DXSetComponentValue(f, "colors", (Object)a))
	goto error;
    a = NULL;
    
    if (!DXSetMember(oo->g, "points", (Object)f))
	goto error;
    oo->fpoints = f;
    f = NULL;
    

    return OK;
    
  error:
    DXDelete((Object)oo->g);
    DXDelete((Object)f);
    DXDelete((Object)a);
    return ERROR;
}

static Error addCube(OutObjInfo *oo, Node *n, RGBColor *colorptr)
{
    Point *t;     /* center of this cube */
    Point *cp;    /* corners of cube for child, centers of parent bottom (0)
		     and child top (1) for line */
    Construct *l; /* the 'where' info */
    int i, *ip;
    int index;    /* next cube number */
    int point;    /* point index numbers */
    RGBColor *c;  /* quad and line colors */


    l = &n->where;
    t = &l->center;

    /* cube positions - each cube is 6 quads */
    cp = oo->quaddata.pos;
    index = oo->nquads;

    cp->x = t->x - l->di;  cp->y = t->y - l->dj;  cp->z = t->z - l->dk;  cp++;
    cp->x = t->x - l->di;  cp->y = t->y - l->dj;  cp->z = t->z + l->dk;  cp++;
    cp->x = t->x - l->di;  cp->y = t->y + l->dj;  cp->z = t->z - l->dk;  cp++;
    cp->x = t->x - l->di;  cp->y = t->y + l->dj;  cp->z = t->z + l->dk;  cp++;

    cp->x = t->x + l->di;  cp->y = t->y - l->dj;  cp->z = t->z - l->dk;  cp++;
    cp->x = t->x + l->di;  cp->y = t->y - l->dj;  cp->z = t->z + l->dk;  cp++;
    cp->x = t->x + l->di;  cp->y = t->y + l->dj;  cp->z = t->z - l->dk;  cp++;
    cp->x = t->x + l->di;  cp->y = t->y + l->dj;  cp->z = t->z + l->dk;  cp++;

    oo->quaddata.pos += 8;


    /* quad connections */
    ip = oo->quaddata.conn;
    point = 8 * index/6;

    *ip++ = point+0; *ip++ = point+1; *ip++ = point+4; *ip++ = point+5; 
    *ip++ = point+0; *ip++ = point+2; *ip++ = point+1; *ip++ = point+3; 
    *ip++ = point+0; *ip++ = point+4; *ip++ = point+2; *ip++ = point+6; 
    *ip++ = point+7; *ip++ = point+3; *ip++ = point+6; *ip++ = point+2; 
    *ip++ = point+7; *ip++ = point+6; *ip++ = point+5; *ip++ = point+4; 
    *ip++ = point+7; *ip++ = point+5; *ip++ = point+3; *ip++ = point+1; 

    oo->quaddata.conn += 6 * 4;


    /* cube color */
    c = oo->quaddata.color;

    for (i=0; i<6; i++) {
	*c = *colorptr; c++;
    }

    oo->quaddata.color += 6;

    oo->nquads += 6;


    /* line between child and parent */
    if (n->parent) {
	index = oo->nlines;

	/* line positions */
	cp = oo->linedata.pos;

	cp[0] = n->parent->where.out;
	cp[1] = l->in;

	oo->linedata.pos += 2;

	/* line connections */
	ip = oo->linedata.conn;
	point = 2 * index;

	for (i=0; i<2; i++)
	    ip[i] = point++;

	oo->linedata.conn += 2;

	/* line color */
	c = oo->linedata.color;

	c->r = 0.9; c->g = 0.8; c->b = 0.1;

	oo->linedata.color++;

	oo->nlines++;
    }

    return OK;
}

static Pointer initarray(Group g, char *member, char *component, 
    int count, int mult)
{
    Field f;
    Array a;
    Pointer p;

    if (!(f = (Field) DXGetMember(g, member)))
	return NULL;
    
    if (!(a = (Array)DXGetComponentValue(f, component)))
	return NULL;
    
    if (!DXAddArrayData(a, 0, count * mult, NULL))
	return NULL;
    
    if (!(p = DXGetArrayData(a)))
	return NULL;

    return p;
}

static Error allocoutobj(OutObjInfo *oo)
{
    Quick *q;

    if (oo->totalquads > 0) {
	q = &oo->quaddata;
	if (!(q->pos = (Point *)initarray (oo->g, "quads", "positions", 
					   oo->totalquads/6, 8)))
	    return ERROR;
	
	if (!(q->conn = (int *)initarray (oo->g, "quads", "connections", 
					  oo->totalquads, 1)))
	    return ERROR;

	if (!(q->color = (RGBColor *)initarray (oo->g, "quads", "colors", 
						oo->totalquads, 1)))
	    return ERROR;
    }

    if (oo->totallines > 0) {
	q = &oo->linedata;
	if (!(q->pos = (Point *)initarray (oo->g, "lines", "positions", 
					   oo->totallines, 2)))
	    return ERROR;
	
	if (!(q->conn = (int *)initarray (oo->g, "lines", "connections", 
					  oo->totallines, 1)))
	    return ERROR;
	
	if (!(q->color = (RGBColor *)initarray (oo->g, "lines", "colors", 
						oo->totallines, 1)))
	    return ERROR;
    }

    if (oo->totaltris > 0) {
	q = &oo->tridata;
	if (!(q->pos = (Point *)initarray (oo->g, "tris", "positions", 
					   oo->totaltris, 3)))
	    return ERROR;
	
	if (!(q->conn = (int *)initarray (oo->g, "tris", "connections", 
					  oo->totaltris, 1)))
	    return ERROR;
	
	if (!(q->color = (RGBColor *)initarray (oo->g, "tris", "colors", 
						oo->totaltris, 1)))
	    return ERROR;
    }

    if (oo->totalpoints > 0) {
	q = &oo->pointdata;
	if (!(q->pos = (Point *)initarray (oo->g, "points", "positions", 
					   oo->totalpoints, 1)))
	    return ERROR;
	
	q->conn = NULL;
	
	if (!(q->color = (RGBColor *)initarray (oo->g, "points", "colors", 
						oo->totalpoints, 1)))
	    return ERROR;
    }

    return OK;
}

static Error nextoutobj(OutObjInfo *oo, Node *np, Policy *p)
{
    Node *thisnp;
    
    for (thisnp = np; thisnp; thisnp = thisnp->sibling) {

	if (thisnp->attr)
	    nextoutobj(oo, thisnp->attr, p);
	
	if (thisnp->child)
	    nextoutobj(oo, thisnp->child, p);
	
	drawObject(oo, thisnp, p);
    }
    
    return OK;
}


/* initialize a node and make the links to parents, siblings
 */
static Node *newnode(int level, int isattr, int number, int dup, Class class, 
		     Node *parent, char *ptag)
{
    Node *np = NULL;

    if (!(np = DXAllocateLocalZero(sizeof(Node))))
	return NULL;

#if 1  /* debug */
    if (!isattr) {
	DXDebug("V", "crNode: level %d, member %2d", level, number);
    } else {
	DXDebug("V", "crAttr: level %d, member %2d", level, number);
    }
#endif

    np->level = level;
    np->isattr = isattr;
    if (!isattr)
	np->levelnumber = number;
    else
	np->attrnumber = number;
    np->where.class = class;
    np->where.isattr = isattr;
    np->where.isshared = dup;
    np->parent = parent;
    setptag(np, ptag);

    if (parent) {
	if (isattr) {
	    if (parent->attr == NULL)
		parent->attr = np;
	    else {
		Node *lnext;
		
		lnext = parent->attr;
		while (lnext->sibling)
		    lnext = lnext->sibling;
		lnext->sibling = np;
	    }
	} else {
	    if (parent->child == NULL)
		parent->child = np;
	    else {
		Node *lnext;
		
		lnext = parent->child;
		while (lnext->sibling)
		    lnext = lnext->sibling;
		lnext->sibling = np;
	    }
	    parent->descendents++;
	}
    }

    return np;
}

/* inserts at the front of the list since that's cheap and insert
 * order doesn't matter.
 */
#if 0
static Error addparent(Node *np, Node *parent)
{
    Nlist *lp;
    
    if (!(lp = DXAllocateLocal(sizeof(Nlist))))
	return ERROR;
		
    lp->this = parent;
    lp->next = NULL;

    if (np->godparent)
	lp->next = np->godparent;

    np->godparent = lp;
    
    return OK;
}
#endif

/* change the parent tag info for a node
 */
static Error setptag(Node *np, char *ptag)
{
    int len;

    if (!ptag || !ptag[0])
	return OK;

    len = strlen(ptag) + 1;
    np->ptag = DXAllocateLocal(len);
    if (!np->ptag)
	return ERROR;

    strcpy(np->ptag, ptag);
    return OK;
}

/* change the child tag info for a node
 */
static Error setctag(Node *np, char *ctag)
{
    int len;

    if (!ctag || !ctag[0])
	return OK;

    len = strlen(ctag) + 1;
    np->ctag = DXAllocateLocal(len);
    if (!np->ctag)
	return ERROR;

    strcpy(np->ctag, ctag);
    return OK;
}

/* recurse thru the data structure determining the placement of
 *  each object node and the number of geometric objects it will
 *  generate.
 */
static int nodeplace(Node *np, Policy *p, Layout *l, OutObjInfo *oo)
{
    Node *thisnp;
    Construct *c;
    int prev_width = 0;


    /* if you have a parent, start out at the trailing edge of it */
    if (np->parent)
	prev_width = np->parent->predescend;


    for (thisnp = np; thisnp; thisnp=thisnp->sibling) {

	/* figure out the width your node is centered over */
	thisnp->predescend = prev_width;

	if (thisnp->child)
	    prev_width = nodeplace(thisnp->child, p, l, oo);
	else
	    prev_width++;

	thisnp->lastdescend = prev_width;


	/* use policy and layout to determine location and size */
	c = &thisnp->where;
	nodecenter(thisnp, p, l, c->isshared);
	geomCount(oo, c->class, c->isattr, c->isshared);


	/* place the attributes after the owning node is placed */
	if (thisnp->attr)
	    attrplace(thisnp->attr, p, l, oo);

    }

    /* return the value for lastdescend for the parent to use */
    return prev_width;
}

/* determine placement for attribute nodes, and add up the number of
 *  geometric objects which will be generated.  attributes don't 
 *  contribute to the overall placement of the object nodes so this
 *  routine is much simpler than the nodeplace() routine.
 */
static void attrplace(Node *np, Policy *p, Layout *l, OutObjInfo *oo)
{
    Node *thisnp;
    Construct *c;

    for (thisnp = np; thisnp; thisnp=thisnp->sibling) {

	/* use policy and layout to determine location and size */
	c = &thisnp->where;
	nodecenter(thisnp, p, l, c->isshared);
	geomCount(oo, c->class, c->isattr, c->isshared);

    }
}

#define LEVELDEPTH 2.0
#define LEVELWIDTH 1.2
#define ATTRWIDTH  2.0

/* use Policy, class, isShared and isAttr to determine location. 
*/
static void nodecenter(Node *np, Policy *p, Layout *l, int isShared)
{
    Construct *c = &np->where;
    Node *base;
    int width = 0;

    base = (c->isattr) ? np->parent : np;

#if 1  /* debug */
    if (!c->isattr) {
	DXDebug("V", 
		"ptNode: level %d, member %2d: pre=%2d, desc=%2d, last=%2d",
		np->level, np->levelnumber, 
		np->predescend, np->descendents, np->lastdescend);
    } else {
	DXDebug("V", 
		"ptAttr: level %d, member %2d", np->level, np->attrnumber);
    }
#endif

    switch (p->version) {
      case 1:
	/* center the members of each level, independent of other levels */
	c->center.x = base->level * LEVELDEPTH;
	c->center.y = base->levelnumber - (l->levelmembers[base->level] / 2.0);
	c->center.z = 0.0;
	break;

      case 2:
      default:
	/* center the parent over its children */
	c->center.x = base->level * LEVELDEPTH;
	c->center.y = (base->predescend + 
		    (base->lastdescend - base->predescend)/2.0) * LEVELWIDTH;
	c->center.z = 0;
	break;

      case 3:
	/* same as 2 with each level in alternating between Y and Z 
	 * NOT WORKING
	 */
	c->center.x = base->level * LEVELDEPTH;
	if (base->level % 2) {
	    c->center.y = (base->predescend + 
		    (base->lastdescend - base->predescend)/2.0) * LEVELWIDTH;
	    c->center.z = 0;
	} else {
	    c->center.y = 0;
	    c->center.z = (base->predescend + 
		    (base->lastdescend - base->predescend)/2.0) * LEVELWIDTH;
	}
	break;

      case 4:
	/* put parent over first child */
	c->center.x = base->level * LEVELDEPTH;
	c->center.y = base->predescend;
	c->center.z = 0.0;
	break;

      case 5:
	/* use 3rd dim, stack objects in depth at each level */
	if (base->parent)
	    width = base->parent->predescend;
	
	c->center.x = base->level * LEVELDEPTH;
	c->center.y = width + base->descendents/2;
	c->center.z = base->levelnumber - (l->levelmembers[base->level] / 2.0);
	break;

    }
    
    c->di = 0.15;
    c->dj = 0.15;
    c->dk = 0.15;
    c->in  = DXPt(c->center.x - c->di, c->center.y, c->center.z);
    c->out = DXPt(c->center.x + c->di, c->center.y, c->center.z);

    if (c->isattr) {
	c->center.x += 0.25; 
	if (p->vertical)
	    c->center.y += (c->dj * np->attrnumber) * ATTRWIDTH;
	else
	    c->center.y -= (c->dj * np->attrnumber) * ATTRWIDTH;
	c->center.z = 0.0;
	
	c->di *= 0.3;
	c->dj *= 0.3;
	c->dk *= 0.3;
	c->in  = DXPt(c->center.x - c->di, c->center.y, c->center.z);
	c->out = DXPt(c->center.x + c->di, c->center.y, c->center.z); 
    }

    if (p->vertical) {
	float tmp;
	
	tmp = -c->center.x; c->center.x = c->center.y; c->center.y = tmp;
	tmp = -c->in.x;     c->in.x = c->in.y;         c->in.y = tmp;
	tmp = -c->out.x;    c->out.x = c->out.y;       c->out.y = tmp;
    }
}
	    

/* count up the number of nodes in the tree.  whether this is unique
 *  objects or not depends on whether shared objects are given new nodes
 *  or not.  check the code that calls newnode to check.
 */
#if 0
static int numnodes(Node *np)
{
    int count = 0;

    nodecount(np, &count);
    
    return count;
}
#endif

/* recursive routine for numnodes
 */
static void nodecount(Node *np, int *count)
{
    Node *nextnp;

    for (nextnp = np; nextnp; ) {

	if (nextnp->child)
	    nodecount(nextnp->child, count);

	nextnp = nextnp->sibling;

	(*count)++;
    }
    
    return;

}

/* delete all members of the tree, including the shared parent linked lists,
 *  and return all the storage.
 */
static Error freenodetree(Node *np)
{
    Node *this_np, *next_np;
    Nlist *this_lp, *next_lp;

    for (next_np = np; next_np; ) {

	if (next_np->attr)
	    freenodetree(next_np->attr);

	if (next_np->child)
	    freenodetree(next_np->child);

	this_np = next_np;
	next_np = next_np->sibling;

	/* free multiple parent linked list if it exists */
	next_lp = this_np->godparent;
	while (next_lp) {
	    DXFree((Pointer)next_lp->this);  /* the node */
	    this_lp = next_lp;
	    next_lp = next_lp->next;
	    DXFree((Pointer)this_lp);        /* the link */
	}
	
	DXFree((Pointer)this_np->ptag);
	DXFree((Pointer)this_np->ctag);

	DXFree((Pointer)this_np);
    }
    
    return OK;
}

/* initialize policy struct */
static Error setpolicyversion(VisInfo *vi, int version)
{
    vi->oi.policy.version = version;
    return OK;
}

static Error setpolicyvertical(VisInfo *vi, int vertical)
{
    vi->oi.policy.vertical = vertical;
    return OK;
}


#if 0
/* UNUSED */
/* calls the per object routines to add geometry to output */
static Error addConstruct(Node np, OutObjInfo *oo)
{

}

/* UNUSED */
/* add the geometry and connect to parent
 */
static Error addGeometry(OutObjInfo *oo, Construct child, Construct parent)
{
}
#endif


/* 
 * per object routines: names, geometry
 */

static char *ClassName (Class c)
{
    switch(c) {    
      case CLASS_MIN:            return "min";
      case CLASS_OBJECT:         return "object";
      case CLASS_PRIVATE:        return "private";
      case CLASS_STRING:	 return "string";	
      case CLASS_FIELD:	       	 return "field";	
      case CLASS_GROUP:	       	 return "group";	
      case CLASS_SERIES:         return "series";
      case CLASS_COMPOSITEFIELD: return "compositefield";
      case CLASS_ARRAY:	       	 return "array";		
      case CLASS_REGULARARRAY:   return "regulararray";
      case CLASS_PATHARRAY:      return "patharray";    
      case CLASS_PRODUCTARRAY:   return "productarray";
      case CLASS_MESHARRAY:      return "mesharray";
      case CLASS_XFORM:	       	 return "xform";		
      case CLASS_SCREEN:	 return "screen";		
      case CLASS_CLIPPED:	 return "clipped";	
      case CLASS_CAMERA:	 return "camera";	
      case CLASS_LIGHT:	       	 return "light";	
      case CLASS_MAX:            return "max";
      case CLASS_DELETED:        return "deleted";        
      case CLASS_CONSTANTARRAY:  return "constantarray";        
      case CLASS_MULTIGRID:	 return "multigrid";        
      default: 			 return "unknown";
    }
    /* notreached */
}

#if 0
static char *TypeName (Type t)
{
    switch(t) {    
      case TYPE_UBYTE:		return "ubyte";
      case TYPE_BYTE:		return "byte";
      case TYPE_USHORT:		return "ushort";
      case TYPE_SHORT:		return "short";
      case TYPE_UINT:		return "uint";
      case TYPE_INT:		return "int";
   /* case TYPE_UHYPER:		return "uhyper"; */
      case TYPE_HYPER:		return "hyper";
      case TYPE_FLOAT:		return "float";
      case TYPE_DOUBLE:		return "double";
      case TYPE_STRING:		return "string";
      default:			return "unknown";
    }
    /* notreached */
}

static char *CatName (Category c)
{
    switch(c) {    
      case CATEGORY_REAL:	return "real";
      case CATEGORY_COMPLEX:	return "complex";
      case CATEGORY_QUATERNION: return "quaternion";
      default:			return "unknown";
    }
    /* notreached */
}
#endif 

static void geomCount (OutObjInfo *oo, Class class, int isAttr, int isShared)
{
    switch (class) {
      default:
	oo->totalquads += 6;
	oo->totallines ++;
	break;
    }

    return;
}

enum what { cube, tetra, hex };

/*
 */
static Error drawObject (OutObjInfo *oo, Node *np, Policy *p)
{
    Construct *c;
    RGBColor color;
    enum what w; 

    c = &np->where;
    switch (c->class) {
      case CLASS_GROUP:
      case CLASS_SERIES:
      case CLASS_COMPOSITEFIELD:
      case CLASS_MULTIGRID:
	color = DXRGB(0.1, 0.9, 0.1);
	w = cube;
	break;

      case CLASS_FIELD:
	color = DXRGB(0.9, 0.5, 0.1);
	w = cube;
	break;

      case CLASS_ARRAY:
      case CLASS_CONSTANTARRAY:
      case CLASS_REGULARARRAY:
      case CLASS_PATHARRAY:
      case CLASS_PRODUCTARRAY:
      case CLASS_MESHARRAY:
	color = DXRGB(0.9, 0.1, 0.1);
	w = cube;
	break;

      case CLASS_STRING:
	color = DXRGB(0.5, 0.5, 0.1);
	w = cube;
	break;

      case CLASS_XFORM:
	color = DXRGB(0.1, 0.5, 0.5);
	w = cube;
	break;

      case CLASS_SCREEN:
	color = DXRGB(0.5, 0.1, 0.5);
	w = cube;
	break;

      case CLASS_LIGHT:
	color = DXRGB(0.9, 0.9, 0.9);
	w = cube;
	break;

      case CLASS_CAMERA:
	color = DXRGB(0.2, 0.2, 0.9);
	w = cube;
	break;

      default:
	color = DXRGB(0.9, 0.1, 0.7);
	w = cube;
	break;
    }

    if (c->isshared)
	color = DXRGB(color.r * 0.1, color.g * 0.1, color.b * 0.1);

    switch (w) {
      case cube:
	if (!addCube(oo, np, &color))
	    return ERROR;
      case tetra:
      case hex:
      default:
	break;
    }

    if (!addLabel(oo->g, oo->font, c, p))
	return ERROR;

    if (np->ptag && !addStringTag (np->ptag, 1, oo->g, oo->font, c, p))
	return ERROR;

    if (np->ctag && !addStringTag (np->ctag, 2, oo->g, oo->font, c, p))
	return ERROR;

    return OK;
}

#define LABELSIZE  0.7
#define TAGSIZE    0.2

/* object label
 */
static Error addLabel (Group g, Object font, Construct *c, Policy *p)
{
    Xform x;
    Matrix m;
    Object o;
    Vector v;

    v.z = c->center.z;
    if (p->vertical) {
	v.x = c->center.x - c->dj;
	v.y = c->center.y + (c->di * 1.10);
	m = DXConcatenate(DXScale(c->dj, c->di, LABELSIZE), DXTranslate(v));
    } else {
	v.x = c->center.x - c->di;
	v.y = c->center.y + (c->dj * 1.10);
	m = DXConcatenate(DXScale(c->di, c->dj, LABELSIZE), DXTranslate(v));
    }

    o = DXGeometricText(ClassName(c->class), font, NULL);
    if (!o)
	return ERROR;

    if (!(x = DXNewXform(o, m)))
	return ERROR;

    if (!DXSetMember(g, NULL, (Object)x))
	return ERROR;

    return OK;
}


/* additional string info.  type 1 is from parent, type 2 is from child.
 */
static Error addStringTag (char *s, int type, Group g, Object font, 
			    Construct *c, Policy *p)
{
    Xform x;
    Matrix m;
    Object o;
    Vector v;

    v.z = c->center.z + (c->dk * 1.05);
    if (type == 2) {
	if (p->vertical) {
	    v.x = c->center.x - c->dj;
	    v.y = c->center.y - c->di;
	    m = DXConcatenate(DXScale(c->dj, c->di, TAGSIZE), DXTranslate(v));
	} else {
	    v.x = c->center.x - c->di;
	    v.y = c->center.y - c->dj;
	    m = DXConcatenate(DXScale(c->di, c->dj, TAGSIZE), DXTranslate(v));
	}
    } else if (type == 1) {
	if (p->vertical) {
	    v.x = c->center.x - c->dj;
	    v.y = c->center.y;
	    m = DXConcatenate(DXScale(c->dj, c->di, TAGSIZE), DXTranslate(v));
	} else {
	    v.x = c->center.x - c->di;
	    v.y = c->center.y;
	    m = DXConcatenate(DXScale(c->di, c->dj, TAGSIZE), DXTranslate(v));
	}
    } else {
	DXSetError(ERROR_INTERNAL, "illegal type in addStringTag");
	return ERROR;
    }


    o = DXGeometricText(s, font, NULL);
    if (!o)
	return ERROR;

    if (!(x = DXNewXform(o, m)))
	return ERROR;

#if 0
    if (!DXSetFloatAttribute((Object)x, "fuzz", 1.0))
	return ERROR;
#endif

    if (!DXSetMember(g, NULL, (Object)x))
	return ERROR;

    return OK;
}


/* 
 *
 * routines which do the major functions.
 *
 */



/* depth, width and counts of members of object.
 */
static
Error traverse(Object o, VisInfo *vi, Node *parent, int isattr, int num, 
	       char *ptag)
{
    InObjInfo *oi;
    Layout *y;
    int count, i;
    ulong obj_id;
    float pos;
    Error rc;
    Array terms[MAXRANK];
    Class class, subclass;
    Object subo, subo1, subo2;
    char *name;
    uint dup;
    Node *l = NULL;
    Matrix m;
    char cbuf[256];

    oi = &vi->oi;
    y = &oi->layout;

    /* if not an attribute, go down a level and start counting members */
    if (!isattr) {
	y->level++;
	if (y->level > y->maxlevel)
	    y->maxlevel = y->level;

	num = y->levelmembers[y->level];

	y->levelmembers[y->level]++;
	if (y->levelmembers[y->level] > y->maxlevelmembers) {
	    y->maxlevelmembers = y->levelmembers[y->level];
	    y->widestlevel = y->level;
	}
    }

    /* check for shared objects
     */
    if (!FindIDbyObject(oi->ht, o, &obj_id)) {
	rc = ERROR;
	goto done;
    }
    
    /* if obj_id == 0, this is a new object.  if obj_id == anything else, this
     *  is a duplicate.
     */
    dup = (obj_id != 0);
    
    y->membercount++;
    if (!dup)
	y->uniquemembers++;

    class = DXGetObjectClass(o);
    switch (class) {
      case CLASS_ARRAY:
	subclass = DXGetArrayClass((Array)o);
	break;
      case CLASS_GROUP:
	subclass = DXGetGroupClass((Group)o);
	break;
      default:
	subclass = class;
    }

    l = newnode(y->level, isattr, num, dup, subclass, parent, ptag);
    if (!l) {
	rc = ERROR;
	goto done;
    }
    
    if (!oi->root)
	oi->root = l;

    if (!isattr) {
	/* if we aren't already processing the attributes of an object,
	 *  check for attributes.
	 */
	for (i=0; (subo=DXGetEnumeratedAttribute(o, i, &name)); i++)
	    traverse(subo, vi, l, 1, i, name);
    }

    switch (class) {
      case CLASS_STRING:
	setctag(l, DXGetString((String)o));
	break;

      case CLASS_ARRAY:
	switch (subclass) {
	  case CLASS_ARRAY:
	  case CLASS_PATHARRAY:
	  case CLASS_CONSTANTARRAY:
	  case CLASS_REGULARARRAY:
	    break;

	  case CLASS_PRODUCTARRAY:
	    DXGetProductArrayInfo((ProductArray)o, &count, terms);
	    for (i=0; i<count; i++) {
		sprintf(cbuf, "term %d", i);
		traverse((Object)terms[i], vi, l, 0, 0, cbuf);
	    }
	    break;

	  case CLASS_MESHARRAY:
	    DXGetMeshArrayInfo((MeshArray)o, &count, terms);
	    for (i=0; i<count; i++) {
		sprintf(cbuf, "term %d", i);
		traverse((Object)terms[i], vi, l, 0, 0, cbuf);
	    }

	    break;

	  default:
	    DXSetError(ERROR_INTERNAL, "unrecognized array subclass");
	    break;
	}
	break;

      case CLASS_FIELD:
	for (i=0; 
	     (subo=DXGetEnumeratedComponentValue((Field)o, i, &name)); i++) {
	    traverse(subo, vi, l, 0, 0, name);
	}

	break;
		
      case CLASS_LIGHT:
	break;

      case CLASS_CLIPPED:
	DXGetClippedInfo((Clipped)o, &subo1, &subo2);
	traverse(subo1, vi, l, 0, 0, "being clipped");
	traverse(subo2, vi, l, 0, 0, "clipped by");
	break;

      case CLASS_SCREEN:
	DXGetScreenInfo((Screen)o, &subo, NULL, NULL);
	traverse(subo, vi, l, 0, 0, NULL);
	break;

      case CLASS_XFORM:
	DXGetXformInfo((Xform)o, &subo1, &m);
	sprintf(cbuf, "[ [%g %g %g] [%g %g %g] [%g %g %g] ] + [%g %g %g]", 
		(double)m.A[0][0], (double)m.A[0][1], (double)m.A[0][2],
		(double)m.A[1][0], (double)m.A[1][1], (double)m.A[1][2],
		(double)m.A[2][0], (double)m.A[2][1], (double)m.A[2][2],
		(double)m.b[0], (double)m.b[1], (double)m.b[2]);

	setctag(l, cbuf);
	traverse(subo1, vi, l, 0, 0, NULL);
	break;

      case CLASS_CAMERA:
	/* ortho, perspective? */
	break;

      case CLASS_GROUP:
	switch (subclass) {
	  case CLASS_GROUP:
	  case CLASS_MULTIGRID:
	  case CLASS_COMPOSITEFIELD:
	    for (i=0; (subo=DXGetEnumeratedMember((Group)o, i, &name)); i++) {
		if (name)
		    traverse(subo, vi, l, 0, 0, name);
		else {
		    sprintf(cbuf, "member %d", i);
		    traverse(subo, vi, l, 0, 0, cbuf);
		}
	    }

	    break;
		
	  case CLASS_SERIES:
	    for (i=0; (subo=DXGetSeriesMember((Series)o, i, &pos)); i++) {
		sprintf(cbuf, "position %g", pos);
		traverse(subo, vi, l, 0, 0, cbuf);
	    }

	    break;
		
	  default:
	    DXSetError(ERROR_INTERNAL, "unrecognized group subclass");
	    break;
	}
	break;

      default:
	DXSetError(ERROR_INTERNAL, "unrecognized object class");
	rc = ERROR;
	goto done;
    }

    if (!dup && !AddIDbyObject(oi->ht, o, y->membercount+1)) {
	rc = ERROR;
	goto done;
    }

    rc = OK;

  done:
    if (!isattr)
	y->level--;
    return rc;
}

/* decide the screen asthetics.
 */
static Error layout(VisInfo *vi)
{
    nodeplace(vi->oi.root, &vi->oi.policy, &vi->oi.layout, &vi->oo);

    /* patch this here until i decide how to fix this better.
     * there is one fewer line than number of nodes.
     */
    vi->oo.totallines--;
    return OK;
}


/* turn the node tree into a renderable thing.
 */
static Object makevisual(VisInfo *vi)
{
    if (!allocoutobj(&vi->oo))
	return NULL;

    if (!nextoutobj(&vi->oo, vi->oi.root, &vi->oi.policy))
	return NULL;

    if (!_dxfNormalsObject((Object)vi->oo.g, "connections"))
	return NULL;

    return (Object) vi->oo.g;
}



/* external entry point
 */
Error m_VisualObject(Object *in, Object *out)
{
    VisInfo vi;
    int olddebug=0;
    char *options, *cp;

    out[0] = NULL;

    if (!in[0]) {
	DXSetError(ERROR_BAD_PARAMETER, "#10000", "input");
	return ERROR;
    }


    if (!initVisInfo(&vi))
	goto error;
    
    olddebug = DXQueryDebug("V");

    if (in[1] && !DXExtractString(in[1], &options)) {
	DXSetError(ERROR_BAD_PARAMETER, "bad options string");
	return ERROR;
    }

    if (in[1] && options) {
	for (cp = options; *cp; cp++) {
	    switch (*cp) {
	      case 'd':
		DXEnableDebug("V", 1);
		break;
	      case '0': case '1': case '2': case '3': case '4':
	      case '5': case '6': case '7': case '8': case '9':
		setpolicyversion(&vi, *cp - '0');
		break;
	      case 'h':
		setpolicyvertical(&vi, 0);
		break;
	      case 'v':
		setpolicyvertical(&vi, 1);
		break;
	      case 'm':
		/* every 1000 calls to the memmgr, check to be sure
		 *  things haven't been corrupted.
		 */
		DXTraceAlloc(1000);
		break;
	    }
	}
    }

    if (traverse(in[0], &vi, NULL, 0, 0, NULL) == ERROR)
	goto error;

    if (layout(&vi) == ERROR)
	goto error;

    if (!(out[0] = makevisual(&vi)))
	goto error;

    
    endVisInfo(&vi);
    DXEnableDebug("V", olddebug);
    DXTraceAlloc(0);
    return OK;

  error:
    errdelVisInfo(&vi);
    DXEnableDebug("V", olddebug);
    DXTraceAlloc(0);
    return ERROR;

}
