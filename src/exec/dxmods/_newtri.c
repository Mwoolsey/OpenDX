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

#define	TRIANGULATE_ALWAYS		1
#define	TRIANGULATE_DEBUG		1
#define	TRIANGULATE_MAIN		0
#define	FIX_TRIANGLE_ORIENTATION	1


#include <stdio.h>
#include <math.h>
#include <dx/dx.h>
#include "_normals.h"
#include "_newtri.h"


#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#define	LOCAL_LOOPS	256

/*
 * To make this more efficient declare a register float _det_ in any
 * routine that uses this macro.
 */

static double	_det_;
#define DetSign(p0,p1,p2)\
(\
    _det_ = (double)(p0.u * ((double)p1.v - p2.v)) +\
	    (double)(p1.u * ((double)p2.v - p0.v)) +\
	    (double)(p2.u * ((double)p0.v - p1.v)),\
    _det_ < (float) 0.0 ? -1 : _det_ > (float) 0.0 ? 1 : 0\
)

#define Determinant(p0,p1,p2)\
    ( p0.u * (p1.v - p2.v) + p1.u * (p2.v - p0.v) + p2.u * (p0.v - p1.v))


static void	FlipLoop	 (Loop *loops);
static int	IntersectsEdge	 (Loop *loops, BVertex *p, BVertex *q);
static void	LoopSign	 (Loop *lp);
static void	LoopBBox	 (Loop *lp);
static int	PointInTri	 (Point2 p, Point2 plast, Point2 pnext,
				  Point2 v0, Point2 v1, Point2 v2, int tsign);
static void	ProjectPoints	 (Point *normal, float *vertices,
			          int nloops, Loop *loops, int nDim);
static int	AddLinkNested	 (Loop *loops, BVertex *p, BVertex *spares,
				  int *nvert, BVertex **first, BVertex **end);
static int	AnyInsideNested (TreeNode holes, BVertex *p0, BVertex *p1,
				  BVertex *p2, int psign);
static Error	TriangulateNestedLoops (int nloops, Loop *loops,
				  float *vertices,
				  Point *normal, int np,
				  Triangle *tptr, int *rntri, int nDim);
static Error    InitLoopsNested (int *, Loop **, int);
static void	LoopSignNested	 (Loop *lp);
static int      AddLink          (Loop *loops, BVertex *p, BVertex **spares,
				  int *nvert, BVertex **first, BVertex **end);
static int      AnyInside        (Loop *loops, BVertex *p0, BVertex *p1,
                                  BVertex *p2, int psign);
static Error    InitLoops        (int *nloops, Loop **loops, BVertex *spares,
				  int np);
static Error	TriangulateLoops (int nloops, Loop *loops, float *vertices,
				  Point *normal, int np,
				  Triangle *tptr, int *rntri, int nDim);
static Error    AssignTreeLevel(TreeNode, int);
static Error    TriangulateLoopWithHoles(TreeNode, int *, Triangle *);



#if 0
void ShowLoop(BVertex *);
#endif

static int srprint = 0;
#if TRIANGULATE_DEBUG
#define srprintf	if (srprint) printf
#else
#define	srprintf
#endif


int
_dxf_TriangleCount (int *f, int nf, int *l, int nl, int *e, int ne)
{
    FLEP	in;

    in.faces	= f;
    in.loops	= l;
    in.edges	= e;
    in.nfaces	= nf;
    in.nloops	= nl;
    in.nedges	= ne;

    return (_dxf__TriangleCount (&in));
}


int
_dxf__TriangleCount (FLEP *in)
{
    FLEP	lin;
    int		f;
    int		l, ls, le;
    int		es, ee;
    int		ntri	= 0;
    int		nvert;

    lin = *in;

    for (f = 0; f < lin.nfaces; f++)
    {
	nvert = 0;

	ls = lin.faces[f];
	le = f < lin.nfaces - 1 ? lin.faces[f + 1] : lin.nloops;

	for (l = ls; l < le; l++)
	{
	    es = lin.loops[l];
	    ee = l < lin.nloops - 1 ? lin.loops[l + 1] : lin.nedges;
	    nvert += ee - es;
	}

	nvert += 2 * (le - ls);
	ntri  += nvert - 2;
    }

    return (ntri);
}



Error
_dxfTriangulateField(Field field)
{
    Error	ret	= ERROR;
    Array	af, al, ae, ap, an;
    int		*f, *l, *e;
    int		nf, nl, ne, np;
    float 	*p;
    Vector	*n;
    Triangle	*tri	= NULL;
    int		ntri;
    int		newtri	= 0;
    Array	arr;
    int		size;
    int		i, *map = NULL;
    Array       sA=NULL, dA=NULL;
    char	*name;
    int		nDim;

    af = (Array) DXGetComponentValue (field, "faces");
    al = (Array) DXGetComponentValue (field, "loops");
    ae = (Array) DXGetComponentValue (field, "edges");
    ap = (Array) DXGetComponentValue (field, "positions");

    if (af == NULL || al == NULL || ae == NULL || ap == NULL)
    {
	DXSetError (ERROR_BAD_PARAMETER,
	      "missing at least one of faces/loops/edges/positions");
	goto cleanup;
    }

    f = (int    *) DXGetArrayData (af);
    l = (int    *) DXGetArrayData (al);
    e = (int    *) DXGetArrayData (ae);
    p = (float  *) DXGetArrayData (ap);

    DXGetArrayInfo (af, &nf,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo (al, &nl,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo (ae, &ne,  NULL, NULL, NULL, NULL);
    DXGetArrayInfo (ap, &np,  NULL, NULL, NULL, &nDim);

    if (nDim == 2 || nDim == 3)
    {
	an = (Array) DXGetComponentValue(field, "normals");
	if (! an)
	{
	    an = _dxf_FLE_Normals(f, nf, l, nl, e, ne, p, nDim);
	    if (! an)
		goto cleanup;
	    
	    if (! DXSetComponentValue(field, "normals", (Object)an))
	    {
		DXDelete((Object)an);
		goto cleanup;
	    }

	    if (! DXSetStringAttribute((Object)an, "dep", "faces"))
	       goto cleanup;
	}

	n = (Vector *) DXGetArrayData(an);
    }
    else
    {
	DXSetError(ERROR_DATA_INVALID, 
		"Faces/Loops/Edges vertices must be 2 or 3D");
	goto cleanup;
    }

    ntri = _dxf_TriangleCount (f, nf, l, nl, e, ne);
    size = ntri * sizeof (Triangle);
    tri = (Triangle *) DXAllocateLocal (size);
    if (tri == NULL)
	goto cleanup;

    map = (int *)DXAllocate(nf*sizeof(int));
    if (! map)
	goto cleanup;

    if (! _dxf_TriangulateFLEP (f, nf, l, nl, e, ne, 
				p, np, n, tri, &newtri, map, nDim))
	goto cleanup;
    
    if (newtri == 0)
    {
	ret = OK;
	goto cleanup;
    }

#define	STRING(_x)	((Object) DXNewString (_x))
    arr = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 3);
    if (arr == NULL)
	goto cleanup;
    if (! DXAddArrayData (arr, 0, newtri, (Pointer) tri))
	goto cleanup;
    if (! DXSetComponentValue (field, "connections", (Object) arr))
	goto cleanup;
    arr = NULL;
    if (! DXSetComponentAttribute (field, "connections", "element type",
				 STRING ("triangles")))
	goto cleanup;
    
    i = 0;
    while (NULL != (sA = (Array)DXGetEnumeratedComponentValue(field, i++, &name)))
    {
	if (! strcmp(name, "invalid faces"))
	{
	    int i, j, k;
	    InvalidComponentHandle ifaces, itris;

	    ifaces = DXCreateInvalidComponentHandle((Object)field,
							NULL, "faces");
	    itris  = DXCreateInvalidComponentHandle((Object)field,
							NULL, "connections");
	    
	    if (! ifaces || ! itris)
		goto cleanup;
	    
	    DXSetAllValid(itris);

	    for (i = k = 0; i < nf; i++)
	    {
		if (DXIsElementInvalid(ifaces, i))
		{
		    for (j = 0; j < map[i]; j++)
			DXSetElementInvalid(itris, k++);
		}
		else
		    k += map[i];
	    }

	    if (! DXSaveInvalidComponent(field, itris))
	    {
		DXFreeInvalidComponentHandle(itris);
		DXFreeInvalidComponentHandle(ifaces);
		goto cleanup;
	    }

	    DXFreeInvalidComponentHandle(itris);
	    DXFreeInvalidComponentHandle(ifaces);
	}
	else
	{
	    int itemSize;
	    Type type;
	    Category cat;
	    int rank, shape[30];
	    Object attr = DXGetComponentAttribute(field, name, "dep");

	    dA = NULL;

	    if (! attr)
		continue;
	    
	    if (strcmp(DXGetString((String)attr), "faces"))
		continue;
	    
	    DXGetArrayInfo(sA, NULL, &type, &cat, &rank, shape);
	    itemSize = DXGetItemSize(sA);

	    if (DXQueryConstantArray(sA, NULL, NULL))
	    {
		dA = (Array)DXNewConstantArrayV(newtri, 
			    DXGetConstantArrayData(sA), type, cat, rank, shape);
		if (! dA)
		    goto cleanup;
	    }
	    else
	    {
		int i, j;
		char *srcPtr;
		char *dstPtr;

		dA = DXNewArrayV(type, cat, rank, shape);
		if (! dA)
		    goto cleanup;
		
		if (! DXAddArrayData(dA, 0, newtri, NULL))
		    goto cleanup;
		
		dstPtr = (char *)DXGetArrayData(dA);
		srcPtr = (char *)DXGetArrayData(sA);

		for (i = 0; i < nf; i++)
		{
		    for (j = 0; j < map[i]; j++)
		    {
			memcpy(dstPtr, srcPtr, itemSize);
			dstPtr += itemSize;
		    }
		    srcPtr += itemSize;
		}
	    }

	    if (! DXSetComponentValue(field, name, (Object)dA))
		goto cleanup;
	    dA = NULL;

	    if (! DXSetComponentAttribute(field, name, "dep", 
					    (Object)DXNewString("connections")))
		goto cleanup;

	}
    }

    /*
     * Get rid of the old stuff
     */

    DXDeleteComponent (field, "invalid faces");
    DXDeleteComponent (field, "faces");
    DXDeleteComponent (field, "loops");
    DXDeleteComponent (field, "edges");
    if (! DXEndField (field))
	goto cleanup;

    ret = OK;

cleanup:
    if (newtri == 0 && ret == OK)
    {
	char *name;

	while (DXGetEnumeratedComponentValue (field, 0, &name))
	    DXDeleteComponent (field, name);
    }

    DXDelete((Object) dA);
    DXFree ((Pointer) tri);
    DXFree ((Pointer) map);
    return (ret);
}

Error
_dxf_TriangulateFLEP (int *f, int nf, int *l, int nl, int *e, int ne,
		float *p, int np, Vector *normal, Triangle *tris, int *ntri,
		int *map, int nDim)
{
    FLEP	in;

    in.faces	= f;
    in.loops	= l;
    in.edges	= e;
    in.points	= p;
    in.map	= map;
    in.nfaces	= nf;
    in.nloops	= nl;
    in.nedges	= ne;
    in.npoints	= np;
    in.nDim     = nDim;

    return (_dxf__TriangulateFLEP (&in, normal, tris, ntri, NULL));
}


Error
_dxf__TriangulateFLEP (FLEP *in, Vector *normal, Triangle *tris, int *ntri,
		  FLEP *valid)
{
    Vector      tri_direction;
    int         temp, k;
    float       dir;
    FLEP	lin;
    FLEP	lvalid= {NULL, NULL};
    Error	ret	= ERROR;
    Loop	*loops	= NULL;
    int		size;
    Triangle	*ltris, *tmp_tris;
    int		lntri=0;
    int		ftri;
    int		f;
    int		l, ls, le;
    int		es, ee;
    int		skips;

    lin = *in;
    if (valid)
	lvalid = *valid;

    size  = lin.nloops * sizeof (Loop);
    loops = (Loop *) DXAllocateLocal (size);
    if (loops == NULL)
	goto cleanup;
    
    ltris = tris;
    lntri = 0;

    for (f = 0; f < lin.nfaces; f++)
    {
	if (valid && ! lvalid.faces[f])
	    continue;

	skips = 0;
	ls = lin.faces[f];
	le = f < lin.nfaces - 1 ? lin.faces[f + 1] : lin.nloops;

	memset (loops, 0, (le - ls) * sizeof (Loop));

	for (l = ls; l < le; l++)
	{
	    if (valid && ! lvalid.loops[l])
	    {
		skips++;
		continue;
	    }

	    es = lin.loops[l];
	    ee = l < lin.nloops - 1 ? lin.loops[l + 1] : lin.nedges;

	    loops[l - ls - skips].nvert = ee - es;
	    loops[l - ls - skips].vids  = lin.edges + es;
	}

	if (getenv("DX_SIMPLE_LOOPS"))
	{
	    if (! TriangulateLoops (le - ls - skips, loops, lin.points,
		    (normal) ? normal + f : NULL, lin.npoints, ltris, &ftri, lin.nDim))
	    {
#if TRIANGULATE_ALWAYS
		if (DXGetError () == ERROR_INTERNAL)
		    DXResetError ();
		else
#endif
		    goto cleanup;
	    }
	}
	else
	{
	    if (! TriangulateNestedLoops (le - ls - skips, loops, lin.points,
		    (normal) ? normal + f : NULL, lin.npoints, ltris, &ftri, lin.nDim))
	    {
#if TRIANGULATE_ALWAYS
		if (DXGetError () == ERROR_INTERNAL)
		    DXResetError ();
		else
#endif
		    goto cleanup;
	    }
	}




#if FIX_TRIANGLE_ORIENTATION
        for (k=0, tmp_tris=ltris; k<ftri; k++) {
	  if (lin.nDim == 2)
	  {
	      float *p = lin.points + 2*tmp_tris->p;
	      float *q = lin.points + 2*tmp_tris->q;
	      float *r = lin.points + 2*tmp_tris->r;
	      float x0 = q[0] - p[0];
	      float y0 = q[1] - p[1];
	      float x1 = r[0] - q[0];
	      float y1 = r[1] - q[1];

	      tri_direction.x = 0.0;
	      tri_direction.y = 0.0;
	      tri_direction.z = x0*y1 - x1*y0;
	  }
	  else
	  {
	      Point *pts = (Point *)lin.points;

	      tri_direction = DXCross(DXSub(pts[tmp_tris->q], pts[tmp_tris->p]),
                                  DXSub(pts[tmp_tris->r], pts[tmp_tris->q]));

	  }

	  dir = DXDot(tri_direction, normal[f]);

          if (dir < 0) {
            temp = tmp_tris->r;
            tmp_tris->r = tmp_tris->q;
            tmp_tris->q = temp;
          }
          tmp_tris++;
        }

#endif

	
	lntri += ftri;
	ltris += ftri;
	in->map[f] = ftri;
    }

    ret = OK;

cleanup:
    if (ret == OK && ntri)
	*ntri = lntri;

    DXFree ((Pointer) loops);
    return (ret);
}


#define Shift \
{ \
    p0 = p1; \
    p1 = p2; \
    p2 = p2->next; \
    continue; \
}

#define EmitTri012 \
{ \
    i0 = p0->id; \
    i1 = p1->id; \
    i2 = p2->id; \
    if (i0 != i1 && i0 != i2 && i1 != i2) \
    { \
	tptr->p = p0->id; \
	tptr->q = p1->id; \
	tptr->r = p2->id; \
	tptr++; \
	ntri++; \
    } \
    p0->next = p2; \
    p2->prev = p0; \
}


/*
 * $$$$$ NOTE:  This code, as did the previous version from Jarek et al,
 * $$$$$        assumes that the first loop is the outer loop and that
 * $$$$$        all subsequent loops are holes _dxf_inside of it.
 */

#if 0
static int dumpLoop = 0;
static int loopLimit = 99999;

static void
DumpLoop(BVertex *p0)
{
    FILE *fd;
    int n = 0;
    BVertex *b0, *b1;
    
    fd = fopen("dump.out", "w");

    b0 = b1 = p0;
    do
    {
	n++;
	b0 = b0->next;
    } while (b0 != b1);
    fprintf(fd, "%d\n", n);
    b0 = b1 = p0;
    do
    {
	fprintf(fd, "    %d\n", b0->id);
	b0 = b0->next;
    } while (b0 != b1);

    fclose(fd);
}
#endif


static Error
TraverseTree(TreeNode node, int *rntri, Triangle *tbase)
{
    TreeNode child;

    if (! node)
	return OK;

    if ((node->level & 0x1) != 0)
    {
	/* 
	 * Then its an outer contour
	 */
	if (! TriangulateLoopWithHoles(node, rntri, tbase))
	    goto error;
    }

    for (child = node->children; child; child = child->sibling)
	if (! TraverseTree(child, rntri, tbase))
	    goto error;

    return OK;

error:
    return ERROR;
}

static Error
TriangulateLoopWithHoles(TreeNode node, int *rntri, Triangle *tbase)
{
    Loop *loop = node->loop;
    int nvert = loop->nvert, not_done;
    BVertex *p0 = loop->base, *p1=NULL, *p2=NULL;
    BVertex *end = NULL;
    BVertex *first = NULL;
    int	i0, i1, i2;
    int nholes;
    TreeNode holes;
    BVertex *spares;
    int n;
    int tsign, psign = loop->sign;
    Triangle *tptr = tbase + *rntri;
    int ntri = *rntri;
    int pass = 0;
    Loop *l;

    if (! psign)
	return OK;

    /*
     * Get spares required for linking.  While you're at it, make sure
     * the holes are oriented opposite to the outer contour.  Also - create
     * linked list of loops
     */
    node->loop->next = node->loop->prev = node->loop;
    for (holes = node->children, nholes = 0; holes; holes = holes->sibling)
    {
	nholes ++;
	holes->loop->next = node->loop->next;
	node->loop->next = holes->loop;
	if (holes->loop->sign == psign)
	    FlipLoop(holes->loop);
    }
    
    /*
     * and doubly link the list 
     */
    l = node->loop;
    do {
	l->next->prev = l;
    } while ((l = l->next) != node->loop);

    spares = (BVertex *)DXAllocateZero(nholes * 2 * sizeof(BVertex));
    if (! spares)
	goto error;

    n = 0;
    while (1)
    {
	do
	{
	    not_done = 0;
	    pass ++;

	    if (first != NULL)
		p0 = first;
	    
	    if (first == end)
		end = NULL;

	    while (p0->pass < pass && p0 != end)
	    {
		p0->pass = pass;

		p1 = p0->next;
		p2 = p1->next;

		if (!p0->tried)
		{
		    srprintf ("DXTri: %2d %2d %2d (%3d)",
			   p0->id, p1->id, p2->id, nvert);

		    if (p0->id == p1->id || p1->id == p2->id ||
			(tsign = DetSign(p0->pos, p1->pos, p2->pos)) == 0)
		    {
			/*
			 * degenerate triangle.  eliminate p1 from the loop.
			 */
			p0->next = p2;
			p2->prev = p0;
			not_done++;
			nvert--;
		    }
		    else if (tsign != psign ||
			AnyInsideNested (node->children, p0, p1, p2, psign))
		    {
			p0->tried = 1;
		    }
		    else
		    {
			not_done++;
			p0->prev->tried = 0;
			if (p0 == first)
			    first = first->prev;
			if (p2 == end) 
			{
			    if (end == first)
				end = NULL;
			    else
				end = end->next;
			}

			if (p1 == first)
			    first = p0;

			EmitTri012;
			nvert--;

			while (nvert > 2)
			{
			    p1 = p0->next;
			    p2 = p1->next;

			    if (DetSign(p0->pos, p1->pos, p2->pos) == 0)
			    {
				p0->next = p2;
				p2->prev = p0;
				if (p1 == first)
				    first = p0;
				nvert --;
			    }
			    else
				break;
			}
		    }
		}

		p0 = p0->next;
	    }

	} while (not_done && nvert >= 3);

	if (! AddLinkNested(node->loop, p0, spares+2*n, &nvert, &first, &end))
	    break;
	
	n ++;
    }

    if (nvert == 3)
	EmitTri012;

    *rntri = ntri;

    DXFree((Pointer)spares);
    return OK;

error:
    DXFree((Pointer)spares);
    return ERROR;
}

static TreeNode CreateNestingTree(int nLoops, Loop *loops);

static Error TriangulateNestedLoops (int nloops, Loop *loops, float *vertices,
			       Point *normal, int np,
			       Triangle *tptr, int *rntri, int nDim)
{
    Error	ret	= ERROR;	/* return status                    */
    BVertex	*vbase	= NULL;		/* base of list blocks              */
    int		nvert;			/* total vertex count               */
    int		size;			/* size temporary                   */
    int		i, j;
    BVertex	*bptr;
    TreeNode    tree = NULL;

    if (rntri)
	*rntri = 0;

    for (nvert = 0, i = 0; i < nloops; i++)
	nvert += loops[i].nvert;

    size = nvert * sizeof (BVertex);
    vbase = (BVertex *) DXAllocateLocal (size);
    if (vbase == NULL)
	goto error;

    /*
     * For each loop:
     *   Initialize its boundary vertex list as a circularly linked list.
     *   For now we'll just put in the vertex id.  We'll fill in the 
     *   vertex positions later.  We separate this for two reasons.  First,
     *   it involves random accessing and we don't want to guarantee that
     *   we destroy any savings we get from the PVS's line cache, and 
     *   second we want to use tight loops for the projection from 3-space
     *   to 2-space.
     */
    bptr = vbase;
    for (j = 0; j < nloops; j++)
    {
	loops[j].base = bptr;

	for (i = 0, bptr = loops[j].base; i < loops[j].nvert; i++, bptr++)
	{
	    bptr->next   = bptr + 1;
	    bptr->prev   = bptr - 1;
	    bptr->lp     = loops + j;
	    bptr->id     = *loops[j].vids++;
	}

	/*
	 * Circularly link the list
	 */

	loops[j].base[loops[j].nvert - 1].next = loops[j].base;
	loops[j].base->prev = loops[j].base + (loops[j].nvert - 1);
    }

    ProjectPoints (normal, vertices, nloops, loops, nDim);

    /*
     * Get rid of colinear loops and loops with fewer than 3 points and
     * link all of the others together.
     */

    if (! InitLoopsNested (&nloops, &loops, np))
	goto error;

    tree = CreateNestingTree(nloops, loops);
    if (! tree)
	goto error;
    
    AssignTreeLevel(tree, 0);

    if (! TraverseTree(tree, rntri, tptr))
	goto error;

    ret = OK;

error:
/* cleanup */
    DXFree((Pointer)tree);
    DXFree ((Pointer) vbase);
    return (ret);
}


#if 1
static int nShow = 0;

void 
ShowLoop(BVertex *first)
{
    int i, n;
    BVertex *p0;
    char buf[32];
    FILE *foo;

    sprintf(buf, "bad%d.dx", nShow++);

    foo = fopen(buf, "w");
    if ( foo )
    {
	n = 0;
	p0 = first;
	do
	{
	    n++;
	} while ((p0 = p0->next) != first);

	fprintf(foo,
    "object \"faces\" class array type int rank 0 items 1 data follows\n0\nattribute \"ref\" string \"loops\"\n");

	fprintf(foo,
    "object \"loops\" class array type int rank 0 items 1 data follows\n0\nattribute \"ref\" string \"edges\"\n");

	fprintf(foo,
    "object \"edges\" class array type int rank 0 items %d data follows\n", n);
	for (i = 0; i < n; i++)
	    fprintf(foo, "%d\n", i);
	fprintf(foo, "attribute \"ref\" string \"positions\"\n");

	fprintf(foo,
    "object \"positions\" class array type float rank 1 shape 3 items %d data follows\n", n);
	for (i = 0, p0 = first; i < n; i++, p0 = p0->next)
	    fprintf(foo, "%f %f 0\n", p0->pos.u, p0->pos.v);
	
	fprintf(foo,
    "object \"data\" class array type int rank 0 items %d data follows\n", n);
	for (i = 0, p0 = first; i < n; i++, p0 = p0->next)
	    fprintf(foo, "%d\n", p0->id);
	fprintf(foo, "attribute \"dep\" string \"positions\"\n");
	
	fprintf(foo, "object \"fle\" class field\n");
	fprintf(foo, "    component \"faces\" \"faces\"\n");
	fprintf(foo, "    component \"loops\" \"loops\"\n");
	fprintf(foo, "    component \"edges\" \"edges\"\n");
	fprintf(foo, "    component \"positions\" \"positions\"\n");
	fprintf(foo, "    component \"data\" \"data\"\n");
	fprintf(foo, "end\n");

	fclose(foo);
    }
    
}
#endif

/*
 * Figure out what the smallest dimension of the polygon is, e.g. the
 * dimension that contains the portion of the normal vector with the
 * largest magnitude.  Now copy in the vertices, projecting them from
 * 3D space to the appropriate 2D space.
 */

static void ProjectPoints (Point *normal, float *vertices,
			   int nloops, Loop *loops, int nDim)
{
    Point	lnormal;
    int		elim;
    int		i, j;
    BVertex	*bptr;

    if (nDim == 2)
    {
	float	*pptr;
	Loop	*loop;

	for (j = 0, loop = loops; j < nloops; j++, loop++)
	    for (i = 0, bptr = loop->base; i < loop->nvert; i++, bptr++)
	    {
		pptr = vertices + 2*bptr->id;
		bptr->pos.u = pptr[0];
		bptr->pos.v = pptr[1];
	    }
    }
    else
    {
	Point   *vptr = (Point *)vertices;
	Point	*pptr;
	Loop	loop;

	lnormal = *normal;
	if (lnormal.x < (float) 0.0) lnormal.x = -lnormal.x;
	if (lnormal.y < (float) 0.0) lnormal.y = -lnormal.y;
	if (lnormal.z < (float) 0.0) lnormal.z = -lnormal.z;

	elim = 0;
	if (lnormal.y > lnormal.x)
	    elim = 1;
	if (lnormal.z > (elim == 0 ? lnormal.x : lnormal.y))
	    elim = 2;

	for (j = 0; j < nloops; j++)
	{
	    loop = loops[j];

	    switch (elim)
	    {
		case 0:	/* get rid of X */
		    for (i = 0, bptr = loop.base; i < loop.nvert; i++, bptr++)
		    {
			pptr = vptr + bptr->id;
			bptr->pos.u = pptr->y;
			bptr->pos.v = pptr->z;
		    }
		    break;

		case 1:	/* get rid of Y */
		    for (i = 0, bptr = loop.base; i < loop.nvert; i++, bptr++)
		    {
			pptr = vptr + bptr->id;
			bptr->pos.u = pptr->x;
			bptr->pos.v = pptr->z;
		    }
		    break;

		case 2:	/* get rid of Z */
		    for (i = 0, bptr = loop.base; i < loop.nvert; i++, bptr++)
		    {
			pptr = vptr + bptr->id;
			bptr->pos.u = pptr->x;
			bptr->pos.v = pptr->y;
		    }
		    break;
	    }
	}
    }
}


static PseudoKey
HashPoint(Key k)
{
    PseudoKey pk;

    Point2 *p = &((*(BVertex **)k)->pos);
    pk = (PseudoKey)(*(int *)((&p->u))*17 + *((int *)(&p->v))*53);

    return pk;
}

static int
ComparePoint(Key k0, Key k1)
{
    Point2 *p0 = &((*(BVertex **)k0)->pos);
    Point2 *p1 = &((*(BVertex **)k1)->pos);

    return ! (p0->u == p1->u && p0->v == p1->v);
}


#define A_INSIDE_B  	 1
#define B_INSIDE_A  	 2
#define A_OUTSIDE_B 	 3
#define AB_CROSS    	 4
#define AB_DISJOINT 	 5
#define AB_INDETERMINATE 6


static Error
_CheckNesting(Loop *A, Loop *B, int *result)
{
  BVertex *i, *o;
  int j, knt, inside = 0, outside = 0;
  float ui, uo0, uo1;
  float vi, vo0, vo1;
  float u, t;
  float mU, MU, mV, MV;

  /*
   * Determine the overlap region
   */
  if (A->box.ll.u > B->box.ll.u) mU = A->box.ll.u;
  else				 mU = B->box.ll.u;
  if (A->box.ll.v > B->box.ll.v) mV = A->box.ll.v;
  else				 mV = B->box.ll.v;
  if (A->box.ur.v < B->box.ur.v) MU = A->box.ur.u;
  else				 MU = B->box.ur.u;
  if (A->box.ur.v < B->box.ur.v) MV = A->box.ur.v;
  else				 MV = B->box.ur.v;


  for (i = A->base, j = 0; j < A->nvert; j++, i = i->next)
  {
    ui = i->pos.u;
    vi = i->pos.v;

    if (ui < mU || ui > MU || vi < mV || vi > MV)
    {
      outside ++;
      continue;
    }

    o = B->base;
    knt = 0;
    do
    {
      uo1 = o->pos.u;
      uo0 = o->next->pos.u;
      vo1 = o->pos.v;
      vo0 = o->next->pos.v;

      /*
       * Orient the edge endpoints so that vo0 <= vo1
       */
      if (vo0 > vo1)
      {
	t = vo0; vo0 = vo1; vo1 = t;
	t = uo0; uo0 = uo1; uo1 = t;
      }

      /*
       * if the edge is horizontal, forget it.
       */
      if (vo0 == vo1)
	continue;
	  
      /* 
       * if both endpoints of the edge lie to the left of
       * the point, forget it.
       */
      if (ui > uo0 && ui > uo1)
	continue;

      /*
       * If the edge lies entirely above or below the point,
       * forget it.
       */
      if (vi > vo1 || vi < vo0)
	continue;
      
      /*
       * If the v exactly coincides with the upper endpoint, count
       * it a cross.  If it exactly coincides with the lower, skip it.
       */
      if (vi == vo1 && ui < uo1)
      {
	knt ++;
	continue;
      }
      else if (vi == vo0)
	  continue;
      
      /*
       * Get the edge value of u at v = vi. If thats to the right
       * of the test point, count it a cross.
       */
      t = (vi - vo0) / (vo1 - vo0);
      u = uo0 + t*(uo1 - uo0);
      if (u > ui)
      {
	knt ++;
	continue;
      }
    }
    while ((o = o->next) != B->base);

    if (knt & 0x1)
      inside ++;
    else
      outside ++;
  }

  if (! inside && ! outside)
  {
      DXSetError(ERROR_INTERNAL, "empty loop?");
      return ERROR;
  }

  if (inside && !outside)
    *result = A_INSIDE_B;
  else if (!inside && outside)
    *result = A_OUTSIDE_B;
  else if (!inside && !outside)
    *result = AB_CROSS;

  return OK;
}

static Error
_CheckNestingBoxes(Loop *A, Loop *B, int *result)
{
    if (A->box.ll.u > B->box.ur.u) *result = AB_DISJOINT;
    else if (A->box.ll.v > B->box.ur.v) *result = AB_DISJOINT;
    else if (A->box.ur.u < B->box.ll.u) *result = AB_DISJOINT;
    else if (A->box.ur.v < B->box.ll.v) *result = AB_DISJOINT;
    else *result = AB_INDETERMINATE;

    return OK;
}


static Error
CheckNesting(Loop *A, Loop *B, int *result)
{
  /*
   * If boxes don't overlap, loops are disjoint
   */
  if (! _CheckNestingBoxes(A, B, result))
      return ERROR;
  else if (*result == AB_DISJOINT)
      return OK;

  /*
   * Check points of A against B loop.  If all are inside,
   * then A is inside B and we're done.  Also - if points
   * of A are both inside and outside B, then we're also done.
   */
  if (!_CheckNesting(A, B, result))
    return ERROR;

  if (*result == A_INSIDE_B || *result == AB_CROSS)
    return OK;
 
  /*
   * All points of A are outside B.  Check to see if A is nested
   * inside B or whether they are disjoint.
   */
  if (! _CheckNesting(B, A, result))
    return ERROR;

  if (*result == A_INSIDE_B)
  {
    *result = B_INSIDE_A;
    return OK;
  }
  else if (*result == AB_CROSS)
  {
    DXSetError(ERROR_INTERNAL, "B x A but A !x B");
    return ERROR;
  }
  else
  {
    *result = AB_DISJOINT;
    return OK;
  }
}

static Error
InsertTreeNode(TreeNode current, TreeNode new)
{
    TreeNode last = NULL, next;

    last = NULL;
    next = current->children;
    while (next)
    {
	int ab;
	
	if (! CheckNesting(new->loop, next->loop, &ab))
	    return ERROR;

	if (ab == A_INSIDE_B)
	{
	    if (new->children)
	    { 
		DXSetError(ERROR_INTERNAL, "topology error");
		return ERROR;
	    }
	    else
		return InsertTreeNode(next, new);
	}
	else if (ab == B_INSIDE_A)
	{
	    TreeNode tmp = next;

	    if (last)
		last->sibling = next->sibling;
	    else
		current->children = next->sibling;

	    next = next->sibling;

	    tmp->sibling = new->children;
	    new->children = tmp;
	}
	else if (ab == AB_CROSS)
	{ 
	      DXSetError(ERROR_INTERNAL, "topology error");
	      return ERROR;
	}
	else
	{
	    last = next;
	    next = next->sibling;
	}
    }

    new->sibling = current->children;
    current->children = new;

    return OK;
}

static Error
AssignTreeLevel(TreeNode node, int level)
{
    TreeNode next;

    if (! node)
	return OK;
      
    node->level = level;
    
    for (next = node->children; next; next = next->sibling)
    {
	if (! AssignTreeLevel(next, level+1))
	    return ERROR;
    }
    
    return OK;
}

static TreeNode
CreateNestingTree(int nLoops, Loop *loops)
{
    int i;
    struct treenode *root;
    struct treenode *nodes;

    nodes = (struct treenode *)DXAllocate((nLoops+1) * sizeof(struct treenode));
    if (! nodes)
	goto error;
    
    root = nodes + 0;

    root->sibling = NULL;
    root->children = NULL;
    root->nChildren = 0;
    root->loop = NULL;

    for (i = 0; i < nLoops; i++)
    {
	/* 
	 * remember that forst node is the root
	 */
	TreeNode node = nodes + (i+1);

	node->loop = loops + i;
	node->sibling = NULL;
	node->children = NULL;
	node->nChildren = 0;

	if (! InsertTreeNode(root, node))
	    goto error;
    }

    return root;
  
error:
    DXFree((Pointer)nodes);
    return NULL;
}

static Error
InitLoopsNested (int *nloopsPtr, Loop **loopsPtr, int np)
{
    int		nloops = *nloopsPtr;
    Loop 	*loops = *loopsPtr;
    int		i, j;
    BVertex	*p0, *p1, *p2;
    int		maxLoops = *nloopsPtr;
    HashTable   ptHash =
	DXCreateHash(sizeof(BVertex *), HashPoint, ComparePoint);
    if (! ptHash)
	goto error;

    for (j = 0; j < nloops; j++)
    {
	p0 = loops[j].base;
	do
	{
	    BVertex **q;
	    q = (BVertex **)DXQueryHashElement(ptHash, (Key)&p0);
	    if (q)
		p0->id = (*q)->id;
	    else
		if (! DXInsertHashElement(ptHash, (Element)&p0))
		    goto error;
	    p0 = p0->next;
	}
	while (p0 != loops[j].base); 
    }

    DXDestroyHash(ptHash);
    ptHash = NULL;

    for (j = 0; j < nloops; j++)
    {
	p0 = loops[j].base;
	do 
	{
	    for (p1 = loops[j].base; p1 != p0; p1 = p1->next)
		if (p1->id == p0->id)
		{
		    if (p0->next == p1)
		    {
			p0->next = p1->next;
			p0->next->prev = p0;
			loops[j].nvert -= 1;
		    }
		    else
		    {
			int p1loop, p0loop;
			
			for (p1loop = 0, p2 = p1; p2 != p0; p2 = p2->next, p1loop++);
			p0loop = loops[j].nvert - p1loop;

			p2 = p0->prev;

			p0->prev = p1->prev;
			p0->prev->next = p0;

			p1->prev = p2;
			p1->prev->next = p1;

			if (p0loop > 2 && p1loop <= 2)
			{
			    loops[j].nvert = p0loop;
			    loops[j].base  = p0;
			}
			else if (p0loop <= 2 && p1loop > 2)
			{
			    loops[j].nvert = p1loop;
			    loops[j].base  = p1;
			}
			else if (p0loop > 2 && p1loop > 2)
			{
			    loops[j].nvert = p0loop;
			    loops[j].base  = p0;

			    if (nloops == maxLoops)
			    {
				Loop *newloops = (Loop *)
					DXAllocate(2*maxLoops*sizeof(Loop));
				if (! newloops)
				    goto error;
				
				memcpy(newloops, loops, nloops*sizeof(Loop));

				if (loops != *loopsPtr)
				    DXFree((Pointer)loops);
				
				loops = newloops;
			    
				maxLoops *= 2;
			    }
		    
			    loops[nloops] = loops[j];

			    loops[nloops].base = p1;
			    loops[nloops].nvert = p1loop;
			    nloops ++;

			    /*
			     * If its the outer loop, then one may lie inside
			     * the other. If so, its a hole.  Otherwise, 
			     * it should be handled as a single outer loop.
			     * So check.
			     */
			}
			else
			    loops[j].nvert = 0;
		    }

		    p1 = p0 = loops[j].base;
		    break;
		}
	    
	    p0 = p0->next;

	} while (p0 != loops[j].base);
    }

    for (j = 0; j < nloops; j++)
    {
	p0 = loops[j].base;
	if (p0)
	{
	    int nv = loops[j].nvert;

	    if (nv)
	    {
		do 
		{
		    if (nv > 3 && p0->id == p0->next->next->id)
		    {
			nv -= 2;
			loops[j].base = p0;
			p0->next = p0->next->next->next;
			p0->next->prev = p0;
		    }
		    p0 = p0->next;
		} while (p0 != loops[j].base);

		loops[j].nvert = nv;
	    }
	}
    }

    for (i = j = 0; j < nloops; j++)
	if (loops[j].nvert != 0)
	{
	    if (j != 0)
		loops[i] = loops[j];
	    i++;
	}

    nloops = i;

    for (i = 0; i < nloops; i++)
    {
	p0 = loops[i].base;
	do
	{
	    p0->tried = 0;
	    p0->pass = 0;
	    p0 = p0->next;
	} while (p0 != loops[i].base);
    }

    *nloopsPtr = nloops;
    *loopsPtr = loops;

    for (j = 0; j < nloops; j++)
    {
	LoopBBox (loops + j);
	LoopSignNested (loops + j);
    }

    for (j = 0; j < nloops; j++)
	if (loops[j].sign == 0 || loops[j].nvert < 3)
	    loops[j].merged = TRUE;

    return OK;

error:
    if (ptHash)
	DXDestroyHash(ptHash);
    return ERROR;
}


static void FlipLoop (Loop *loops)
{
    Loop	loop;
    int		i;
    BVertex	*curr;
    BVertex	*next;

    loop = *loops;

    for (i = 0, curr = loop.base; i < loop.nvert; i++, curr = next)
    {
	next = curr->next;
	curr->next = curr->prev;
	curr->prev = next;
    }

    loops->sign *= -1;
}


static void
LoopBBox (Loop *lp)
{
    BVertex	*p;
    Point2	pos;
    Point2	ll;		
    Point2	ur;

    if (! lp->base)
    {
        lp->box.ll.u = lp->box.ll.v = 0.0;
        lp->box.ur.u = lp->box.ur.v = 0.0;
	return;
    }

    ur = ll = lp->base->pos;
    for (p = lp->base->next; p != lp->base; p = p->next)
    {
	pos = p->pos;

	if (pos.u < ll.u)      ll.u = pos.u;
	if (pos.u > ur.u) ur.u = pos.u;
	
	if (pos.v < ll.v)      ll.v = pos.v;
	if (pos.v > ur.v) ur.v = pos.v;
    }

    lp->box.ll = ll;
    lp->box.ur = ur;

    return;
}

static void LoopSignNested (Loop *lp)
{
    BVertex	*p;
    BVertex	*p0, *p1, *p2;
    BVertex	*tmp;
    Point2	pos;
    Point2	ll;			/* the lower left */
    int		psign;

    /*
     * Find the "leftmost bottom-most" vertex for use as our starting midpoint.
     * While were at it, find the rest of the bounding box too.
     */

    p = lp->base;
    p1 = lp->base;
    ll = p1->pos;
    for (p = lp->base->next; p != lp->base; p = p->next)
    {
	pos = p->pos;

	if (pos.u < ll.u)
	{
	    ll.u = pos.u;
	    p1   = p;
	}
	else if (pos.u == ll.u && pos.v < ll.v)
	{
	    ll.v = pos.v;
	    p1   = p;
	}
    }

    /*
     * Walk around the polygon until we either get a non-zero sign for the
     * determinant or until we've come all the way around.  If we do get
     * a sign then that is the characteristic direction for the polygon
     * and we can return it immediately.  Otherwise all of the points in
     * the polygon are colinear.
     */

    tmp = p1;
    p0  = p1->prev;
    p2  = p1->next;

    do
    {
	psign = DetSign (p0->pos, p1->pos, p2->pos);
	if (psign)
	{
	    lp->sign = psign;
	    return;
	}

	p0 = p1;
	p1 = p2;
	p2 = p2->next;

    } while (p2 != tmp);

    lp->sign = 0;
}


#define CheckPoint()\
{\
    ip = p->id;\
    if (ip == i0 || ip == i1 || ip == i2)\
	continue;\
    ppos = p->pos;\
    if (ppos.u < tribox.ll.u || ppos.v < tribox.ll.v ||\
        ppos.u > tribox.ur.u || ppos.v > tribox.ur.v)\
	continue;\
    if (PointInTri (ppos, p->prev->pos, p->next->pos, v0, v1, v2, psign))\
	return (TRUE);\
}

static int AnyInsideNested (TreeNode holes, BVertex *p0,
			BVertex *p1, BVertex *p2, int psign)
{
    BVertex	*p;
    int		i0, i1, i2;
    Point2	v0, v1, v2;
    Point2	ppos;
    BBox	tribox, loopbox;
    int		ip;
    Loop	*lp;
    int		i;
    int		nvert;
    TreeNode    hole;

    i0 = p0->id;  i1 = p1->id;  i2 = p2->id;
    v0 = p0->pos; v1 = p1->pos; v2 = p2->pos;

    tribox.ll.u = v0.u<v1.u ? v0.u<v2.u ? v0.u : v2.u : v1.u<v2.u ? v1.u : v2.u;
    tribox.ll.v = v0.v<v1.v ? v0.v<v2.v ? v0.v : v2.v : v1.v<v2.v ? v1.v : v2.v;
    tribox.ur.u = v0.u>v1.u ? v0.u>v2.u ? v0.u : v2.u : v1.u>v2.u ? v1.u : v2.u;
    tribox.ur.v = v0.v>v1.v ? v0.v>v2.v ? v0.v : v2.v : v1.v>v2.v ? v1.v : v2.v;

    /*
     * Check the boundary
     */

    for (p = p2->next; p != p0; p = p->next)
	CheckPoint ();

    /*
     * Check the other loops.
     */

    for (hole = holes; hole; hole = hole->sibling)
    {
	lp = hole->loop;
	loopbox = lp->box;
	if (loopbox.ur.u < tribox.ll.u || loopbox.ll.u > tribox.ur.u ||
	    loopbox.ur.v < tribox.ll.v || loopbox.ll.v > tribox.ur.v)
	    continue;

	for (i = 0, nvert = lp->nvert, p = lp->base;
	     i < nvert;
	     i++, p = p->next)
	    CheckPoint ();
    }

    return (FALSE);
}


#if 0
    foreach point P on the boundary
      foreach unconnected loop L
	foreach point Q on an unconnected loop
	    if P or Q has been split then continue
	    make the edge PQ
		foreach vertex V on the boundary and all loops
		    W = V->next;
		    if (PQ intersects VW)
			continue on to next Q or P
		no intersections so connect PQ
    could not form a link return FALSE

    NOTE: the above has been changed slightly.  It now finds
    the first inside loop visible from some point on the outside
    loop.  Then of all the possible pairs between these two,
    it chooses the closest.

#endif

static int
AddLinkNested(Loop *loops, BVertex *p0, BVertex *spares, int *nvert,
				BVertex **first, BVertex **end)
{
    BVertex	*sp;
    BVertex	*p, *q;
    BVertex	*np, *nq;
    Loop	*lp;
    int		i, maxi;
    int		j, maxj;
    int		k;

    sp = spares;

    for (j = 0, maxj = *nvert, p = p0; j < maxj; j++, p = p->next)
    {
	for (k = 1, lp = loops->next; lp != loops; lp = lp->next, k++)
	{
	    if (lp->merged)
		continue;

	    for (i = 0, maxi = lp->nvert, q = lp->base;
		 i < maxi;
		 i++, q = q->next)
	    {
		if (p->id == q->id)
		    continue;

		if (IntersectsEdge (loops, p, q))
		    continue;

		srprintf ("LINKING at %2d - %2d\n", p->id, q->id);
		np = sp++;
		nq = sp++;
		*np = *p;
		*nq = *q;

		np->pass = 0;
		nq->pass = 0;

		np->next = q;
		nq->next = p;
		np->prev = p->prev;
		nq->prev = q->prev;
		p->prev->next = np;
		q->prev->next = nq;
		p->prev  = nq;
		q->prev  = np;

		lp->merged     = TRUE;
		lp->next->prev = lp->prev;
		lp->prev->next = lp->next;
		lp->next       = NULL;
		lp->prev       = NULL;

		*nvert += lp->nvert + 2;
		loops->nvert = *nvert;
		loops->base  = p0;

		np->prev->tried = 0;
		np->tried = 0;

		*first = np->prev;
		*end   = nq->next;

		return (TRUE);
	    
	    }
	}
    }

    return FALSE;
}

/*
 * If the edge we are looking at contains one of the link points then
 * just move on.  If we find that a vertex has been replicated then we
 * assume an intersection since if we don't we run the risk of introducing
 * a loop that runs counter to the real direction of the boundary.
 * Next we check the rotations of the triangles formed by connecting the
 * endpoints of the segments.  Basically, if we find that both triangles
 * formed using one segment as the base rotate the same way then there
 * isn't an intersection.
 */

#define CheckEdge(_V)\
{\
    v1 = v0->next;\
    srprintf ("   %2d - %2d:  ", v0->id, v1->id);\
    if (v0 == _V || v1 == _V)\
    {\
	srprintf ("point match\n");\
	continue;\
    }\
    v0i  = v0->id;\
    v1i  = v1->id;\
    if (pid == v0i || pid == v1i || qid == v0i || qid == v1i)\
    {\
	srprintf ("ID MATCH\n");\
	return (TRUE);\
    }\
    v0os = v0->pos;\
    v1os = v1->pos;\
    if ((v0os.u < pqbox.ll.u && v1os.u < pqbox.ll.u) ||\
        (v0os.v < pqbox.ll.v && v1os.v < pqbox.ll.v) ||\
        (v0os.u > pqbox.ur.u && v1os.u > pqbox.ur.u) ||\
        (v0os.v > pqbox.ur.v && v1os.v > pqbox.ur.v))\
	{\
	    srprintf ("no bbox overlap\n");\
	    continue;\
	}\
    abc = DetSign (ppos, qpos, v0os);\
    abd = DetSign (ppos, qpos, v1os);\
    cda = DetSign (v0os, v1os, ppos);\
    cdb = DetSign (v0os, v1os, qpos);\
    srprintf ("%2d %2d %2d %2d  ", abc, abd, cda, cdb);\
    if ((abc * abd) == 1 || (cda * cdb) == 1)\
    {\
	srprintf ("no intersection\n");\
	continue;\
    }\
    srprintf ("INTERSECTION\n");\
    return (TRUE);\
}

static int IntersectsEdge (Loop *loops, BVertex *p, BVertex *q)
{
    int		pid, qid;
    Point2	ppos, qpos;
    BVertex	*v0, *v1;
    int		v0i, v1i;
    Point2	v0os, v1os;
    Loop	*lp;
    int		i;
    int		nvert;
    int		abc, abd, cda, cdb;
    BBox	pqbox;
    BBox	loopbox;

    pid  = p->id;
    qid  = q->id;
    ppos = p->pos;
    qpos = q->pos;

    if (ppos.u < qpos.u)
    {
	pqbox.ll.u = ppos.u;
	pqbox.ur.u = qpos.u;
    }
    else
    {
	pqbox.ll.u = qpos.u;
	pqbox.ur.u = ppos.u;
    }

    if (ppos.v < qpos.v)
    {
	pqbox.ll.v = ppos.v;
	pqbox.ur.v = qpos.v;
    }
    else
    {
	pqbox.ll.v = qpos.v;
	pqbox.ur.v = ppos.v;
    }

    /*
     * Check around the boundary
     */

    for (v0 = p->next; v0 != p; v0 = v0->next)
	CheckEdge (p);

    /*
     * Check around each loop
     */

    for (lp = loops->next; lp != loops; lp = lp->next)
    {
	loopbox = lp->box;
	if (loopbox.ur.u < pqbox.ll.u || loopbox.ll.u > pqbox.ur.u ||
	    loopbox.ur.v < pqbox.ll.v || loopbox.ll.v > pqbox.ur.v)
	    continue;

	for (i = 0, nvert = lp->nvert, v0 = lp->base;
	     i < nvert;
	     i++, v0 = v0->next)
	{
	    CheckEdge (q);
	}
    }

    return (FALSE);
}


/*
 * Determines whether a point is _dxf_inside a triangle by examining the
 * rotations in the triangles formed by each pair of vertices in the
 * test triangle and the point.  If all of the rotations are the same
 * then the point is _dxf_inside of the triangle, if one is different then
 * the point is outside, if one doesn't rotate then the point is either
 * on or coincident with one of the triangle's edges.
 */

static int PointInTri (Point2 p, Point2 plast, Point2 pnext,
				Point2 v0, Point2 v1, Point2 v2, int psign)
{
    int		det01p, det12p, det20p;
    int		np = 0, nn = 0, nz = 0;
    Point2	e0, e1;

    det01p = DetSign (v0, v1, p);
    if (det01p > 0)
        np ++;
    else if (det01p == 0)
    {
        memcpy(&e0, &v0, sizeof(Point2));
        memcpy(&e1, &v1, sizeof(Point2));
        nz ++;
    }
    else 
	nn ++;

    det12p = DetSign (v1, v2, p);
    if (det12p > 0)
        np ++;
    else if (det12p == 0)
    {
        memcpy(&e0, &v1, sizeof(Point2));
        memcpy(&e1, &v2, sizeof(Point2));
        nz ++;
    }
    else 
	nn ++;

    det20p = DetSign (v2, v0, p);
    if (det20p > 0)
        np ++;
    else if (det20p == 0)
    {
       	memcpy(&e0, &v2, sizeof(Point2));
        memcpy(&e1, &v0, sizeof(Point2));
        nz ++;
    }
    else 
	nn ++;

    if (nn == 3 || np == 3)
	return TRUE;
    else if ((nn == 0 || np == 0) && nz == 1)
    {
	int p0, p1;

	p0 = DetSign(e0, e1, plast);
	p1 = DetSign(e0, e1, pnext);
	if (p0 == psign || p1 == psign)
	    return TRUE;
	else
	    return FALSE;
    }
    else
	return FALSE;
}


#if TRIANGULATE_MAIN
main ()
{
    Point	pvals [100];
    int		ids[1000];
    Loop	loops[100];
    Triangle	tris[1000];
    int		ntri;
    int		nloops	= 0;
    int		npts	= 0;
    int		i, j, k;
    Point	*pptr;
    Point	p;
    Point	normal;

    p.x = p.y = p.z = 0.0;
    for (i = 0, pptr = pvals; i < 10; i++)
    {
	p.x = (float) i;
	for (j = 0; j < 10; j++)
	{
	    p.y = (float) j;
	    *pptr++ = p;
	}
    }

    normal.x = 0.0;
    normal.y = 0.0;
    normal.z = 1.0;

    for (;;)
    {
	printf ("enter number of loops:  ");
	fflush (stdout);
	scanf ("%d", &nloops);
	if (nloops == 0)
	    exit (0);
	if (nloops < 0)
	{
	    nloops = -nloops;
	    srprint = TRUE;
	}
	for (j = 0; j < nloops; j++)
	{
	    loops[j].vids = ids + npts;
	    printf ("  loop %2d: enter cnt & indices:  ", j);
	    fflush (stdout);
	    scanf ("%d", &loops[j].nvert);
	    for (i = 0; i < loops[j].nvert; i++)
		scanf ("%d", &ids[npts++]);
	}
	printf ("\n");

	TriangulateLoops (nloops, loops, pvals, &normal, 100, tris, &ntri);
	printf ("  %3d triangles in buffer %x\n", ntri, tris);
	for (i = 0; i < ntri; i++)
	    printf ("%3d:  %3d %3d %3d\n", i, tris[i].p, tris[i].q, tris[i].r);
    }
}
#endif


static Error TriangulateLoops (int nloops, Loop *loops, float *vertices,
			       Point *normal, int np,
			       Triangle *tptr, int *rntri, int nDim)
{
    Error	ret	= ERROR;	 /* return status                    */
    int		nvert;			 /* total vertex count               */
    BVertex	*vbase	= NULL;		 /* base of list blocks              */
    BVertex	*spares;		 /* spare nodes for splitting        */
    int		size;			 /* size temporary                   */
    Loop	loop;			 /* local copy of current loop       */
    int		psign;			 /* sign of the polygon's  rotation  */
    int		tsign;			 /* sign of the triangle's rotation  */
    BVertex	*p0, *p1=NULL, *p2=NULL; /* vertices of tri being considered */
    int		i0, i1, i2;
    int		pass;
    int		ntri	= 0;		 /* # of triangles generated         */
    int		i, j;
    BVertex	*bptr;
    Loop	*orig_loops = loops;
    BVertex	*first, *end;
    int		not_done;
#if 0
    int		loopknt = 0;
#endif

    if (rntri)
	*rntri = 0;

    /*
     * Since each loop requires a link and each link requires splitting 2
     * vertices we end up with the following for the maximum number of
     * vertices.
     */

    for (nvert = 0, i = 0; i < nloops; i++)
	nvert += loops[i].nvert;
    nvert += 2 * (nloops - 1);

    size = nvert * sizeof (BVertex);
    vbase = (BVertex *) DXAllocateLocal (size);
    if (vbase == NULL)
	goto error;
    spares = vbase;
    memset (vbase, 0, size);

    /*
     * For each loop:
     *   * Initialize its boundary vertex list as a circularly linked list.
     *   * We first have to chunk off the necessary number of BVertex nodes
     *     for use with this list and set the rest aside as the spares.
     *   * For now we'll just put in the vertex id.  We'll fill in the 
     *     vertex positions later.  We separate this for two reasons.  First,
     *     it involves random accessing and we don't want to guarantee that
     *     we destroy any savings we get from the PVS's line cache, and 
     *     second we want to use tight loops for the projection from 3-space
     *     to 2-space.
     */

    for (j = 0; j < nloops; j++)
    {
	loops[j].base = spares;
	spares += loops[j].nvert;
	loop = loops[j];

	for (i = 0, bptr = loop.base; i < loop.nvert; i++, bptr++)
	{
	    bptr->next   = bptr + 1;
	    bptr->prev   = bptr - 1;
	    bptr->lp     = loops + j;
	    bptr->id     = *loop.vids++;
	}

	/*
	 * Circularly link the list
	 */

	bptr = loop.base + (loop.nvert - 1);
	bptr->next = loop.base;
	loop.base->prev = bptr;
    }

    ProjectPoints (normal, vertices, nloops, loops, nDim);

    /*
     * Get rid of colinear loops and loops with fewer than 3 points and
     * link all of the others together.
     */

    if (! InitLoops (&nloops, &loops, spares, np))
	goto error;

    psign = loops->sign;
    if (psign == 0)
    {
	ret = OK;
	goto error;
    }


    first = loops->base;
    end   = NULL;
    pass  = 0;
    nvert = loops->nvert;
    p0    = loops->base;
    first = NULL;
    while (nloops > 0)
    {
	do
	{
	    if (first != NULL)
		p0 = first;

	    not_done = 0;
	    pass++;
	    
	    if (first == end)
		end = NULL;

	    while (p0->pass < pass && p0 != end)
	    {
		p0->pass = pass;

		p1 = p0->next;
		p2 = p1->next;

		if (!p0->tried)
		{
		    srprintf ("DXTri: %2d %2d %2d (%3d/%3d)",
			   p0->id, p1->id, p2->id, pass, nvert);

		    if (p0->id == p1->id || p1->id == p2->id ||
			(tsign = DetSign(p0->pos, p1->pos, p2->pos)) == 0)
		    {
			/*
			 * degenerate triangle.  eliminate p1 from the loop.
			 */
			p0->next = p2;
			p2->prev = p0;
			not_done++;
			nvert--;
		    }
		    else if (tsign != psign ||
			AnyInside (loops, p0, p1, p2, psign))
		    {
			p0->tried = 1;
		    }
		    else
		    {
			not_done++;
			p0->prev->tried = 0;
			if (p0 == first)
			    first = first->prev;
			if (p2 == end) 
			{
			    if (end == first)
				end = NULL;
			    else
				end = end->next;
			}

#if 0
			if (pass >= which_pass)
			{
			    fprintf(stderr, "output: %d %d %d\n", p0->id, p1->id, p2->id);
			    ShowLoop(p2);
			}
#endif

			EmitTri012;
			nvert--;

			while (nvert > 2)
			{
			    p1 = p0->next;
			    p2 = p1->next;

			    if (DetSign(p0->pos, p1->pos, p2->pos) == 0)
			    {
				p0->next = p2;
				p2->prev = p0;
				nvert --;
			    }
			    else
				break;
			}
		    }
		}

		p0 = p0->next;
	    }

	} while (not_done && nvert >= 3);

	if (nloops > 1)
	    if (! AddLink (loops, p0, &spares, &nvert, &first, &end))
	    {
		DXSetError (ERROR_INTERNAL, "Couldn't add link");
		goto error;
	    }

	nloops--;
    }

    if (nvert > 3)
    {
	DXSetError(ERROR_DATA_INVALID, "topology error");
#if 0
        {
	FILE *foo = fopen("bad.dx", "w");
	if ( foo )
	{
	    if (! first)
		first = p0;

	    for (i = 0, p0 = first; p0->next != first; p0 = p0->next)
		i++;

	    fprintf(foo,
	"object \"faces\" class array type int rank 0 items 1 data follows\n0\nattribute \"ref\" string \"loops\"\n");

	    fprintf(foo,
	"object \"loops\" class array type int rank 0 items 1 data follows\n0\nattribute \"ref\" string \"edges\"\n");

	    fprintf(foo,
	"object \"edges\" class array type int rank 0 items %d data follows\n", nvert);
	    for (i = 0; i < nvert; i++)
		fprintf(foo, "%d\n", i);
	    fprintf(foo,
	"attribute \"ref\" string \"positions\"\n ", nvert);

	    fprintf(foo,
	"object \"positions\" class array type float rank 1 shape 3 items %d data follows\n", nvert);
	    for (i = 0, p0 = first; i < nvert; i++, p0 = p0->next)
		fprintf(foo, "%f %f 0\n", p0->pos.u, p0->pos.v);
	    
	    fprintf(foo, "object \"fle\" class field\n");
	    fprintf(foo, "    component \"faces\" \"faces\"\n");
	    fprintf(foo, "    component \"loops\" \"loops\"\n");
	    fprintf(foo, "    component \"edges\" \"edges\"\n");
	    fprintf(foo, "    component \"positions\" \"positions\"\n");
	    fprintf(foo, "end\n");

	    fclose(foo);
	}
	}
#endif
	goto error;
    }

    if (nvert == 3)
	EmitTri012;

#if 0
    if (dumpLoop || loopknt == loopLimit) 
	DumpLoop(p0);
#endif

    ret = OK;
    goto cleanup;

error:
#if ! TRIANGULATE_ALWAYS
    ntri = 0;
#endif

cleanup:
    if (orig_loops != loops)
       DXFree((Pointer)loops);
    DXFree ((Pointer) vbase);
    if (rntri)
	*rntri = ntri;
    return (ret);
}

static int
AnyInside (Loop *loops, BVertex *p0,
			BVertex *p1, BVertex *p2, int psign)
{
    BVertex	*p;
    int		i0, i1, i2;
    Point2	v0, v1, v2;
    Point2	ppos;
    BBox	tribox, loopbox;
    int		ip;
    Loop	*lp;
    int		i;
    int		nvert;

    i0 = p0->id;  i1 = p1->id;  i2 = p2->id;
    v0 = p0->pos; v1 = p1->pos; v2 = p2->pos;

    tribox.ll.u = v0.u<v1.u?v0.u<v2.u?v0.u : v2.u : v1.u<v2.u?v1.u : v2.u;
    tribox.ll.v = v0.v<v1.v?v0.v<v2.v?v0.v : v2.v : v1.v<v2.v?v1.v : v2.v;
    tribox.ur.u = v0.u>v1.u?v0.u>v2.u?v0.u : v2.u : v1.u>v2.u?v1.u : v2.u;
    tribox.ur.v = v0.v>v1.v?v0.v>v2.v?v0.v : v2.v : v1.v>v2.v?v1.v : v2.v;

    /*
     * Check the boundary
     */

    for (p = p2->next; p != p0; p = p->next)
	CheckPoint ();

    /*
     * Check the other loops.
     */

    for (lp = loops->next; lp != loops; lp = lp->next)
    {
	loopbox = lp->box;
	if (loopbox.ur.u < tribox.ll.u || loopbox.ll.u > tribox.ur.u ||
	    loopbox.ur.v < tribox.ll.v || loopbox.ll.v > tribox.ur.v)
	    continue;

	for (i = 0, nvert = lp->nvert, p = lp->base;
	     i < nvert;
	     i++, p = p->next)
	    CheckPoint ();
    }

    return (FALSE);
}

static Error
InitLoops (int *nloopsPtr, Loop **loopsPtr, BVertex *spares,
		       int np)
{
    int		nloops = *nloopsPtr;
    Loop 	*loops = *loopsPtr;
    int		i, j, k;
    int		flip;
    BVertex	*p0, *p1, *p2;
    int		maxLoops = *nloopsPtr;
    HashTable   ptHash =
	DXCreateHash(sizeof(BVertex *), HashPoint, ComparePoint);
    if (! ptHash)
	goto error;

    for (j = 0; j < nloops; j++)
    {
	p0 = loops[j].base;
	do
	{
	    BVertex **q;
	    q = (BVertex **)DXQueryHashElement(ptHash, (Key)&p0);
	    if (q)
		p0->id = (*q)->id;
	    else
		if (! DXInsertHashElement(ptHash, (Element)&p0))
		    goto error;
	    p0 = p0->next;
	}
	while (p0 != loops[j].base); 
    }

    DXDestroyHash(ptHash);
    ptHash = NULL;

    for (j = 0; j < nloops; j++)
    {
	p0 = loops[j].base;
	do 
	{
	    for (p1 = loops[j].base; p1 != p0; p1 = p1->next)
		if (p1->id == p0->id)
		{
		    if (p0->next == p1)
		    {
			p0->next = p1->next;
			p0->next->prev = p0;
			loops[j].nvert -= 1;
		    }
		    else
		    {
			int p1loop, p0loop;
			
			for (p1loop = 0, p2 = p1; p2 != p0; p2 = p2->next, p1loop++);
			p0loop = loops[j].nvert - p1loop;

			p2 = p0->prev;

			p0->prev = p1->prev;
			p0->prev->next = p0;

			p1->prev = p2;
			p1->prev->next = p1;

			if (p0loop > 2 && p1loop <= 2)
			{
			    loops[j].nvert = p0loop;
			    loops[j].base  = p0;
			}
			else if (p0loop <= 2 && p1loop > 2)
			{
			    loops[j].nvert = p1loop;
			    loops[j].base  = p1;
			}
			else if (p0loop > 2 && p1loop > 2)
			{
			    loops[j].nvert = p0loop;
			    loops[j].base  = p0;

			    if (nloops == maxLoops)
			    {
				Loop *newloops = (Loop *)
					DXAllocate(2*maxLoops*sizeof(Loop));
				if (! newloops)
				    goto error;
				
				memcpy(newloops, loops, nloops*sizeof(Loop));

				if (loops != *loopsPtr)
				    DXFree((Pointer)loops);
				
				loops = newloops;
			    
				maxLoops *= 2;
			    }
		    
			    loops[nloops] = loops[j];

			    loops[nloops].base = p1;
			    loops[nloops].nvert = p1loop;
			    nloops ++;
			}
			else
			    loops[j].nvert = 0;
		    }

		    p1 = p0 = loops[j].base;
		    break;
		}
	    
	    p0 = p0->next;

	} while (p0 != loops[j].base);
    }

    for (j = 0; j < nloops; j++)
	loops[j].merged = FALSE;

    for (j = 0; j < nloops; j++)
	LoopSign (loops + j);

    loops->next = loops->prev = loops;
    if (nloops == 1)
	return OK;

    /*
     * If one of the inner loops (a hole) rotates the same way as the
     * outer loop then we need to flip its sense.  We do this by
     * interchanging its next and its prev pointers.  But first, we
     * want to make sure that all of our outer loops always rotate
     * the same way, this way thin faces will have their tops & bottoms
     * (or fronts & backs) triangulated the same way rather than oppositely.
     * This is particularly important for reducing the triangle count when
     * a thin solid collapses to a face during simplification.
     */

    if (loops->sign == 1)
	FlipLoop (loops);
    flip = loops->sign;
    for (j = 1; j < nloops; j++)
	if (loops[j].sign == flip)
	    FlipLoop (loops + j);

    for (j = 0; j < nloops; j++)
	if (loops[j].sign == 0 || loops[j].nvert < 3)
	    loops[j].merged = TRUE;

    for (j = 0; j < nloops; j++)
    {
	p0 = loops[j].base;
	if (p0)
	{
	    do 
	    {
		for (k = j+1; k < nloops; k++)
		    if ((p1 = loops[k].base) != NULL)
			do
			{
			    if (p1->id == p0->id)
			    {
				p2 = p0->next;
				p0->next = p1->next;
				p0->next->prev = p0;
				p1->next = p2;
				p2->prev = p1;
				loops[j].nvert += loops[k].nvert;
				loops[k].nvert = 0;
				loops[k].base = NULL;
				if (loops[j].box.ll.u > loops[k].box.ll.u)
				    loops[j].box.ll.u = loops[k].box.ll.u;
				if (loops[j].box.ll.v > loops[k].box.ll.v)
				    loops[j].box.ll.v = loops[k].box.ll.v;
				if (loops[j].box.ur.u < loops[k].box.ur.u)
				    loops[j].box.ur.u = loops[k].box.ur.u;
				if (loops[j].box.ur.v < loops[k].box.ur.v)
				    loops[j].box.ur.v = loops[k].box.ur.v;
				p1 = NULL;
			    }
			    else
				p1 = p1->next;
			} while (p1 != loops[k].base);
		p0 = p0->next;
	    } while (p0 != loops[j].base);
	}

	p0 = loops[j].base;
	if (p0)
	{
	    int nv = loops[j].nvert;

	    if (nv)
	    {
		do 
		{
		    if (nv > 3 && p0->id == p0->next->next->id)
		    {
			nv -= 2;
			loops[j].base = p0;
			p0->next = p0->next->next->next;
			p0->next->prev = p0;
		    }
		    p0 = p0->next;
		} while (p0 != loops[j].base);

		loops[j].nvert = nv;
	    }
	}
    }

    for (i = j = 0; j < nloops; j++)
	if (loops[j].nvert != 0)
	{
	    if (j != 0)
		loops[i] = loops[j];
	    i++;
	}

    nloops = i;

    for (i = 0; i < nloops; i++)
    {
	p0 = loops[i].base;
	do
	{
	    p0->tried = 0;
	    p0->pass = 0;
	    p0 = p0->next;
	} while (p0 != loops[i].base);
    }

    /*
     * Set up the loop linked lists
     */

    for (j = 0; j < nloops; j++)
    {
	loops[j].next = loops + j + 1;
	loops[j].prev = loops + j - 1;
    }
    loops[nloops - 1].next = loops;
    loops[0].prev = loops + nloops - 1;

    for (j = 0; j < nloops; j++)
    {
	if (loops[j].merged)
	{
	    loops[j].prev->next = loops[j].next;
	    loops[j].next->prev = loops[j].prev;
	}
    }

    *nloopsPtr = nloops;
    *loopsPtr = loops;
    return OK;

error:
    if (ptHash)
	DXDestroyHash(ptHash);
    return ERROR;
}

static int
AddLink(Loop *loops, BVertex *p0, BVertex **spares, int *nvert,
				BVertex **first, BVertex **end)
{
    BVertex	*sp;
    BVertex	*p, *q;
    BVertex	*np, *nq;
    Loop	*lp;
    int		i, maxi;
    int		j, maxj;
    int         k;

    sp = *spares;

    for (j = 0, maxj = *nvert, p = p0; j < maxj; j++, p = p->next)
    {
	for (k = 1, lp = loops->next; lp != loops; lp = lp->next, k++)
	{
	    for (i = 0, maxi = lp->nvert, q = lp->base;
		 i < maxi;
		 i++, q = q->next)
	    {
		if (p->id == q->id)
		    continue;

		if (IntersectsEdge (loops, p, q))
		    continue;

		srprintf ("LINKING at %2d - %2d\n", p->id, q->id);
		np = sp++;
		nq = sp++;
		*np = *p;
		*nq = *q;

		np->pass = 0;
		nq->pass = 0;

		np->next = q;
		nq->next = p;
		np->prev = p->prev;
		nq->prev = q->prev;
		p->prev->next = np;
		q->prev->next = nq;
		p->prev  = nq;
		q->prev  = np;

		lp->merged     = TRUE;
		lp->next->prev = lp->prev;
		lp->prev->next = lp->next;
		lp->next       = NULL;
		lp->prev       = NULL;

		*nvert += lp->nvert + 2;
		*spares = sp;
		loops->nvert = *nvert;
		loops->base  = p0;

		np->prev->tried = 0;
		np->tried = 0;

		if (srprint)
		{
		    printf("sequence after adding nested loop:");
		    for (j = 0, maxj = *nvert, p = p0; j < maxj; j++, p = p->next)
			 printf("%d\n", p->id);
		}

		*first = np->prev;
		*end   = nq->next;

		return (TRUE);
	    
	    }
	}
    }

    return FALSE;
}

static void LoopSign (Loop *lp)
{
    BVertex	*p;
    BVertex	*p0, *p1, *p2;
    BVertex	*tmp;
    Point2	pos;
    Point2	ll;			/* the lower left */
    Point2	ur;
    int		psign;

    /*
     * Find the "leftmost bottom-most" vertex for use as our starting midpoint.
     * While were at it, find the rest of the bounding box too.
     */

    p = lp->base;

    for (p1 = p2 = p, ll = ur = p->pos, tmp = p->next;
	 tmp != p;
	 tmp = tmp->next)
    {
	pos = tmp->pos;

	if (pos.u < ll.u)
	{
	    ll.u = pos.u;
	    p1   = tmp;
	}
	else if (pos.u == ll.u)
	{
	    if (pos.v < p1->pos.v)
		p1 = tmp;
	}
	else if (pos.u > ur.u)
	    ur.u = pos.u;
	
	if (pos.v < ll.v)
	    ll.v = pos.v;
	else if (pos.v > ur.v)
	    ur.v = pos.v;
    }

    /*
     * Walk around the polygon until we either get a non-zero sign for the
     * determinant or until we've come all the way around.  If we do get
     * a sign then that is the characteristic direction for the polygon
     * and we can return it immediately.  Otherwise all of the points in
     * the polygon are colinear.
     */

    tmp = p1;
    p0  = p1->prev;
    p2  = p1->next;

    do
    {
	psign = DetSign (p0->pos, p1->pos, p2->pos);
	if (psign)
	{
	    lp->sign     = psign;
	    lp->box.ll   = ll;
	    lp->box.ur   = ur;
	    return;
	}

	p0 = p1;
	p1 = p2;
	p2 = p2->next;
    } while (p2 != tmp);

    lp->sign = 0;
    ll.u = ll.v = 0.0;
    lp->box.ll = ll;
    lp->box.ur = ll;
}


