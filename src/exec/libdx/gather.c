/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>
#include "render.h"

/*
 * Gather the translucent triangles from a field
 */

#define SKIP (xf->tile.skip && (rand()&((xf->tile.skip-1)<<8)))


/*
 * Parameters
 */

#define PARAMETER(parm, name, type, get) {				    \
    type p;								    \
    Object a;								    \
    a = DXGetAttribute(o, name);					    \
    if (a) {								    \
        if (!get(a, &p))						    \
	    DXErrorReturn(ERROR_BAD_PARAMETER, "invalid " name " parameter"); \
        new->parm = p;							    \
    }									    \
}

#define MULTIPLIER(parm, name, type, get) {				    \
    type p;								    \
    Object a;								    \
    a = DXGetAttribute(o, name);					    \
    if (a) {								    \
        if (!get(a, &p))						    \
	    DXErrorReturn(ERROR_BAD_PARAMETER, "invalid " name " parameter"); \
        new->parm *= p;							    \
    }									    \
}

static Error
parameters(Object o, struct tile *new, struct tile *old)
{
    *new = *old;
    if (!old->ignore) {
	PARAMETER(fast_exp,     "fast exp",     int,  DXExtractInteger);
	PARAMETER(flat_x,       "flat x",       int,  DXExtractInteger);
	PARAMETER(flat_y,       "flat y",       int,  DXExtractInteger);
	PARAMETER(flat_z,       "flat z",       int,  DXExtractInteger);
	PARAMETER(skip,         "skip",         int,  DXExtractInteger);
	PARAMETER(patch_size,   "patch size",   int,  DXExtractInteger);
	MULTIPLIER(color_multiplier,"color multiplier",float,DXExtractFloat);
	MULTIPLIER(opacity_multiplier,"opacity multiplier",float,DXExtractFloat);
    }
    if (!_dxf_approx(o, &new->approx))
	return ERROR;
    return OK;
}


/*
 * macros to add things to the sort array
 *
 * The size of the elements that go into the sort array depends on such
 * factors as the element type (triangle, quad etc.); for faces derived
 * from connection-dependent volume elements, there is an additional word
 * naming the element that the face came from to identify the color.  This
 * size appears in SIZE macro in shade.c, the INFO macro in gather.c,
 * and the ADVANCE macro in volume.c, and these must be kept in sync.
 */

#define ADD(p,n) \
    ((int *)((long)s + sizeof(struct sort)))[n] = p

#define INFO(t, n, surf) {						      \
    s->field = field;							      \
    s->type = t;							      \
    s->surface = surf;							      \
    s = (struct sort *) ((long)s + sizeof(struct sort) + n * sizeof(int));     \
    gather->nthings++;							      \
}

/*
 *
 */

#define CULLPOINT(pt, p, min, max) (cull && ( \
    pt[p].x<min.x || pt[p].x>max.x || pt[p].y<min.y || pt[p].y>max.y))

#define CULLLINE(pt, p, q, min, max) (cull && ( \
       (pt[p].x<min.x && pt[q].x<min.x) \
    || (pt[p].x>max.x && pt[q].x>max.x) \
    || (pt[p].y<min.y && pt[q].y<min.y) \
    || (pt[p].y>max.y && pt[q].y>max.y)))

#define CULLTRIP(pt, p, q, r, min, max) (cull && ( \
       (pt[p].x<min.x && pt[q].x<min.x && pt[r].x<min.x) \
    || (pt[p].x>max.x && pt[q].x>max.x && pt[r].x>max.x) \
    || (pt[p].y<min.y && pt[q].y<min.y && pt[r].y<min.y) \
    || (pt[p].y>max.y && pt[q].y>max.y && pt[r].y>max.y)))

#define CULLTRI(pt, tri, min, max) \
    CULLTRIP(pt, tri.p, tri.q, tri.r, min, max)

#define CULLQUADP(pt, p, q, r, s, min, max) (cull && ( \
       (pt[p].x<min.x && pt[q].x<min.x && pt[r].x<min.x && pt[s].x<min.x) \
    || (pt[p].x>max.x && pt[q].x>max.x && pt[r].x>max.x && pt[s].x>max.x) \
    || (pt[p].y<min.y && pt[q].y<min.y && pt[r].y<min.y && pt[s].y<min.y) \
    || (pt[p].y>max.y && pt[q].y>max.y && pt[r].y>max.y && pt[s].y>max.y)))

#define CULLQUAD(pt, quad, min, max) \
    CULLQUADP(pt, quad.p, quad.q, quad.r, quad.s, min, max)


/*
 *
 */

#define POINT(i) {							      \
    if (!CULLPOINT(xf->positions, i, min, max)) {			      \
	ADD(i,0);							      \
	INFO(SORT_POINT, 1, 0);						      \
    }									      \
}

#define LINE(i) {							      \
    line = xf->c.lines[i];						      \
    if (!CULLLINE(xf->positions, line.p, line.q, min, max)) {		      \
	ADD(i,0);							      \
	INFO(SORT_LINE, 1, 0);					     	      \
    }									      \
}									      

#define POLYLINE(i)							      \
{									      \
    int j, start, end;							      \
    start = xf->polylines[i];						      \
    if (i == xf->nconnections-1)					      \
	end = xf->nedges - 1;						      \
    else								      \
	end = xf->polylines[i+1] - 1;					      \
    for (j = start; j < end; j++) {					      \
	int p = xf->edges[j], q = xf->edges[j+1];			      \
	if (!CULLLINE(xf->positions, p, q, min, max)) {		      	      \
	    ADD(i,0);							      \
	    ADD(j,1);							      \
	    INFO(SORT_POLYLINE, 2, 0);					      \
	}								      \
    }									      \
}

#define TRI(i, surf) {							      \
    tri = xf->c.triangles[i];						      \
    if (!CULLTRI(xf->positions, tri, min, max)) {			      \
	ADD(i,0);							      \
	INFO(SORT_TRI, 1, surf);					      \
    }									      \
}

#define QUAD(i, surf) {							      \
    quad = xf->c.quads[i];						      \
    if (!CULLQUAD(xf->positions, quad, min, max)) {			      \
	ADD(i,0);							      \
	INFO(SORT_QUAD, 1, surf);					      \
    }									      \
}

#define TRI_PTS(p, q, r, surf) {					      \
    if (!CULLTRIP(xf->positions, p, q, r, min, max))	{		      \
	ADD(p,0), ADD(q,1), ADD(r,2);					      \
	INFO(SORT_TRI_PTS, 3, surf);					      \
    }									      \
}

#define TRI_FLAT(p, q, r, col, surf) {					      \
    if (!CULLTRIP(xf->positions, p, q, r, min, max))	{		      \
	ADD(p,0), ADD(q,1), ADD(r,2), ADD(col,3);			      \
	INFO(SORT_TRI_FLAT, 4, surf);					      \
    }									      \
}

#define QUAD_PTS(p, q, r, s, surf) {					      \
    if (!CULLQUADP(xf->positions, p, q, r, s, min, max)) {		      \
	ADD(p,0), ADD(q,1), ADD(r,2), ADD(s,3);				      \
	INFO(SORT_QUAD_PTS, 4, surf);					      \
    }									      \
}

#define QUAD_FLAT(p, q, r, s, col, surf) {				      \
    if (!CULLQUADP(xf->positions, p, q, r, s, min, max)) {		      \
	ADD(p,0), ADD(q,1), ADD(r,2), ADD(s,3), ADD(col,4);		      \
	INFO(SORT_QUAD_FLAT, 5, surf);					      \
    }									      \
}

#define QF(a,b,c, d,e,f, g,h,i, j,k,l, sf,SF) {			 	      \
    quad.p = ((a)*Y+(b))*Z+(c);						      \
    quad.q = ((d)*Y+(e))*Z+(f);						      \
    quad.r = ((g)*Y+(h))*Z+(i);						      \
    quad.s = ((j)*Y+(k))*Z+(l);						      \
    QUAD_FLAT(quad.p, quad.q, quad.r, quad.s, col, (sf==0||sf==SF));	      \
}

#define Q(a,b,c, d,e,f, g,h,i, j,k,l, sf,SF) {				      \
    quad.p = ((a)*Y+(b))*Z+(c);						      \
    quad.q = ((d)*Y+(e))*Z+(f);						      \
    quad.r = ((g)*Y+(h))*Z+(i);						      \
    quad.s = ((j)*Y+(k))*Z+(l);						      \
    QUAD_PTS(quad.p, quad.q, quad.r, quad.s, (sf==0||sf==SF));		      \
}

#define TRI_PTS_INVALID(p, q, r, surf) {				      \
    if (!CULLTRIP(xf->positions, p, q, r, min, max))	{		      \
	ADD(p,0), ADD(q,1), ADD(r,2);					      \
	INFO(SORT_INV_TRI_PTS, 3, surf);				      \
    }									      \
}

#define TRI_FLAT_INVALID(p, q, r, col, surf) {				      \
    if (!CULLTRIP(xf->positions, p, q, r, min, max))	{		      \
	ADD(p,0), ADD(q,1), ADD(r,2), ADD(col,3);			      \
	INFO(SORT_INV_TRI_FLAT, 4, surf);				      \
    }									      \
}

#define QUAD_PTS_INVALID(p, q, r, s, surf) {				      \
    if (!CULLQUADP(xf->positions, p, q, r, s, min, max)) {		      \
	ADD(p,0), ADD(q,1), ADD(r,2), ADD(s,3);				      \
	INFO(SORT_INV_QUAD_PTS, 4, surf);				      \
    }									      \
}

#define QUAD_FLAT_INVALID(p, q, r, s, col, surf) {			      \
    if (!CULLQUADP(xf->positions, p, q, r, s, min, max)) {		      \
	ADD(p,0), ADD(q,1), ADD(r,2), ADD(s,3), ADD(col,4);		      \
	INFO(SORT_INV_QUAD_FLAT, 5, surf);				      \
    }									      \
}

#define Q_INVALID(a,b,c, d,e,f, g,h,i, j,k,l, sf,SF) {			      \
    quad.p = ((a)*Y+(b))*Z+(c);						      \
    quad.q = ((d)*Y+(e))*Z+(f);						      \
    quad.r = ((g)*Y+(h))*Z+(i);						      \
    quad.s = ((j)*Y+(k))*Z+(l);						      \
    QUAD_PTS_INVALID(quad.p, quad.q, quad.r, quad.s, (sf==0||sf==SF));	      \
}

#define QF_INVALID(a,b,c, d,e,f, g,h,i, j,k,l, sf,SF) {			      \
    quad.p = ((a)*Y+(b))*Z+(c);						      \
    quad.q = ((d)*Y+(e))*Z+(f);						      \
    quad.r = ((g)*Y+(h))*Z+(i);						      \
    quad.s = ((j)*Y+(k))*Z+(l);						      \
    QUAD_FLAT_INVALID(quad.p, quad.q, quad.r, quad.s, col, (sf==0||sf==SF));  \
}

Object
_dxfField_Gather(Field f, struct gather *gather, struct tile *tile)
{
    Point min, max;
    Line line;
    Triangle tri;
    Quadrilateral quad;
    int col, i, j, k, field, cull;
    struct sort *s;
    struct xfield *xf;
    InvalidComponentHandle ich = NULL;


    /* get the info we need */
    field = gather->nfields;
    xf = gather->fields + field;
    cull = gather->cull;

    if (DXEmptyField(f))
	return (Object) f;
    _dxf_XZero(xf);

    if (!_dxf_XPositions(f, xf, XR_REQUIRED, XD_NONE)) return NULL;
    if (!_dxf_XBox(f, xf, XR_REQUIRED, XD_GLOBAL)) return NULL;
    min.x = gather->min.x - .5  /* for lines */;
    min.y = gather->min.y - .5  /* for lines */;
    max.x = gather->max.x + .5  /* for lines */;
    max.y = gather->max.y + .5  /* for lines */;
    if (CULLBOX(xf->box, min, max))
	return (Object) f;

    /* store parameters and get new ones */
    if (!parameters((Object)f, &xf->tile, tile)) return NULL;

    /* get box for near plane for perspective clipping */
    /* XXX - should do culling against near plane */
    /* NB - this has to come after parameters() call */
    if (xf->tile.perspective)
	xf->nearPlane = xf->box[0].z;

    if (!_dxf_XColors(f, xf, XR_REQUIRED, XD_GLOBAL)) return NULL;
    if (xf->tile.approx!=approx_dots) {
	if (!_dxf_XOpacities(f, xf, XR_OPTIONAL, XD_GLOBAL)) return NULL;
	if (!_dxf_XConnections(f, xf, XR_OPTIONAL)) return NULL;
	if (!_dxf_XPolylines(f, xf, XR_OPTIONAL, XD_GLOBAL)) return NULL;
    }
    if (!xf->opacities_array && !xf->volume)
	return (Object) f;
    if (xf->connections_array && xf->nconnections==0) /* to match _Count */
	return (Object) f;
    if (!_dxf_XNeighbors(f, xf, XR_REQUIRED, XD_GLOBAL)) return NULL;

    /* sanity check for fcolors = bcolors for volumes */
    /* and for position-dependent */
    /* XXX - xf->volume? */
    if (xf->ct==ct_cubes || xf->ct==ct_tetrahedra)

	if (xf->fcolors_array != xf->bcolors_array)
	    DXErrorReturn(ERROR_DATA_INVALID,
			"use colors and not front/back colors for volumes");
    
    /* Get lighting parameters if they are present */
    if (! _dxf_XLighting(f, xf))
	return NULL;

    /* 
     * In regular case, sort is null, but we still gather the field
     * NB - Gather__Clipped depends on this to determine whether a
     * translucent or volume object was clipped
     */
    gather->nfields += 1;
    DXDebug("G", "    is included (lt %g rt %g bt %g tp %g)",
	  xf->box[0].x, xf->box[7].x, xf->box[0].y, xf->box[7].y);

    if (xf->ct == ct_none)
    {
	if (! _dxf_XInvalidPositions(f, xf))
	    return NULL;
    }
    else
    {
	if (! _dxf_XInvalidConnections(f, xf))
	    return NULL;
	if (! _dxf_XInvalidPolylines(f, xf))
	    return NULL;
    }
    
    /*
     * Everything from here down is assumed to require an expanded
     * positions list because a sort array is involved.  If a sort
     * array is not involved, i.e. one of the regular cases, we would
     * not have expanded the positions (XD_NONE above), and it is up
     * to the regular volume rendering code to handle the compact array.
     */
    if (!gather->sort)
	return (Object) f;
    xf->positions = (Point *) DXGetArrayData(xf->positions_array);
    if (!xf->positions)
	return NULL;

    /* where to start */
    s = gather->current;

    /* translucent points */
    if (xf->ct==ct_none) {
	if (!_dxf_XInvalidPositions(f, xf))
	    return NULL;
	ich = xf->iPts;
	for (i=0; i<xf->npositions; i++)
	    if (!ich || DXIsElementValid(ich, i))
		POINT(i);
    }

    /* translucent lines */
    else if (xf->ct==ct_lines) {
	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;
	for (i=0; i<xf->nconnections; i++)
	    if (!ich || DXIsElementValid(ich, i))
		LINE(i);
    }

    else if (xf->ct == ct_polylines) {
	if (!_dxf_XInvalidPolylines(f, xf))
	    return NULL;
	for (i=0; i < xf->nconnections; i++) {
	    POLYLINE(i);
	}
    }

    /* translucent triangles */
    else if (xf->ct==ct_triangles) {
	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;
	for (i=0; i<xf->nconnections; i++)
	    if (!ich || DXIsElementValid(ich, i))
		TRI(i, 0);
    }
    
    /* translucent quads */
    else if (xf->ct==ct_quads) {
	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;
	/* XXX - local? */
	xf->c.quads = (Quadrilateral *)
	    DXGetArrayData/*Local*/(xf->connections_array);
	if (!xf->c.quads)
	    return NULL;
	for (i=0; i<xf->nconnections; i++)
	    if (!ich || DXIsElementValid(ich, i))
		QUAD(i, 0);
	DXFreeArrayDataLocal(xf->connections_array, (Pointer)xf->c.quads);
    }

	
    /* tetrahedra dep positions */
    else if (xf->ct==ct_tetrahedra && xf->colors_dep==dep_positions) {
	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;
	for (i=0; i<xf->nconnections; i++) {
	    int *neighbors = ((int(*)[4])(xf->neighbors))[i];
	    int valid = !ich || DXIsElementValid(ich, i);
	    /* for each face of tetra */
	    for (j=0; j<4; j++) {
		/* include triangles on boundary */
		/* and triangles we haven't done before */
		if (neighbors[j]<0 || neighbors[j]<i) {
		    /* construct triangle */
		    for (k=0; k<3; k++)
			((int*)(&tri))[k]
			    = ((int*)(&(xf->c.tetrahedra[i])))[(k+j+1)%4];

		    if (valid)
			TRI_PTS(tri.p, tri.q, tri.r, neighbors[j]<0)
		    else
			TRI_PTS_INVALID(tri.p, tri.q, tri.r, neighbors[j]<0)
		}
	    }
	}
    }

    /* tetrahedra dep connections */
    else if (xf->ct==ct_tetrahedra && xf->colors_dep==dep_connections) {
	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;
	for (i=0; i<xf->nconnections; i++) {
	    int *neighbors = ((int(*)[4])(xf->neighbors))[i];
	    int valid = !ich || DXIsElementValid(ich, i);
	    /* for each face of tetra */
	    for (j=0; j<4; j++) {
		/* include triangles on boundary */
		/* and triangles we haven't done before */
		if (neighbors[j]<0 || neighbors[j]<i) {
		    /* construct triangle */
		    for (k=0; k<3; k++)
			((int*)(&tri))[k]
			    = ((int*)(&(xf->c.tetrahedra[i])))[(k+j+1)%4];

		    if (valid)
			TRI_FLAT(tri.p, tri.q, tri.r, i, neighbors[j]<0)
		    else
			TRI_FLAT_INVALID(tri.p, tri.q, tri.r, i, neighbors[j]<0)
		}
	    }
	}
    }

    /* three-dimensional regular connections cube faces dep positions */
    else if (xf->ct==ct_cubes && xf->colors_dep==dep_positions) {
	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;

	if (!xf->neighbors_array) {

	    if (! ich)
	    {
		int X = xf->k[0], Y = xf->k[1], Z = xf->k[2], x, y, z;

		for (x=0; x<X; x++) {
		    for (y=0; y<Y; y++) {
			for (z=0; z<Z; z++) {
			    if (y<Y-1 && z<Z-1)
				Q(x,y,z, x,y+1,z, x,y,z+1, x,y+1,z+1, x,X-1);
			    if (z<Z-1 && x<X-1)
				Q(x,y,z, x+1,y,z, x,y,z+1, x+1,y,z+1, y,Y-1);
			    if (x<X-1 && y<Y-1)
				Q(x,y,z, x+1,y,z, x,y+1,z, x+1,y+1,z, z,Z-1);
			}
		    }
		}
	    }
	    else
	    {
		int X = xf->k[0], Y = xf->k[1], Z = xf->k[2];
		int x, y, z, colx, coly, colz;

		for (x=0; x<X; x++) {
		    colx = x<X-1? x : X-2;
		    for (y=0; y<Y; y++) {
			coly = y<Y-1? y : Y-2;
			for (z=0; z<Z; z++) {
			    int valid;
			    colz = z<Z-1? z : Z-2;
			    col = (colx*(Y-1)+coly)*(Z-1)+colz;
			    valid = !ich || DXIsElementValid(ich, col);
			    if (valid)
			    {
				if (y<Y-1 && z<Z-1)
				    Q(x,y,z,x,y+1,z,x,y,z+1,x,y+1,z+1,x,X-1);
				if (z<Z-1 && x<X-1)
				    Q(x,y,z,x+1,y,z,x,y,z+1,x+1,y,z+1,y,Y-1);
				if (x<X-1 && y<Y-1)
				    Q(x,y,z,x+1,y,z,x,y+1,z,x+1,y+1,z,z,Z-1);
			    }
			    else
			    {
				if (y<Y-1 && z<Z-1)
				    Q_INVALID(x,y,z,x,y+1,z,x,y,z+1,
							    x,y+1,z+1,x,X-1);

				if (z<Z-1 && x<X-1)
				    Q_INVALID(x,y,z,x+1,y,z,x,y,z+1,
							    x+1,y,z+1, y,Y-1);
				if (x<X-1 && y<Y-1)
				    Q_INVALID(x,y,z,x+1,y,z,x,y+1,z,
							    x+1,y+1,z, z,Z-1);
			    }
			}
		    }
		}
	    }

	} else {

	    xf->c.cubes = (Cube *)DXGetArrayData(xf->connections_array);
	    if (!xf->c.cubes)
		return ERROR;

	    /* cubes dep positions */
	    for (i=0; i<xf->nconnections; i++) {
		int *nbrs = ((int(*)[6])(xf->neighbors))[i];
		int *cube = (int *)&(xf->c.cubes[i]);
		int valid = !ich || DXIsElementValid(ich, i);
		if (valid)
		{
		    if (nbrs[0]<0 || nbrs[0]<i)
			QUAD_PTS(cube[0],cube[1],cube[2],cube[3], nbrs[0]<0);
		    if (nbrs[1]<0 || nbrs[1]<i)
			QUAD_PTS(cube[4],cube[5],cube[6],cube[7], nbrs[1]<0);
		    if (nbrs[2]<0 || nbrs[2]<i)
			QUAD_PTS(cube[0],cube[1],cube[4],cube[5], nbrs[2]<0);
		    if (nbrs[3]<0 || nbrs[3]<i)
			QUAD_PTS(cube[2],cube[3],cube[6],cube[7], nbrs[3]<0);
		    if (nbrs[4]<0 || nbrs[4]<i)
			QUAD_PTS(cube[0],cube[2],cube[4],cube[6], nbrs[4]<0);
		    if (nbrs[5]<0 || nbrs[5]<i)
			QUAD_PTS(cube[1],cube[3],cube[5],cube[7], nbrs[5]<0);
		}
		else
		{
		    if (nbrs[0]<0 || nbrs[0]<i)
			QUAD_PTS_INVALID(cube[0],cube[1],cube[2],
							cube[3], nbrs[0]<0);
		    if (nbrs[1]<0 || nbrs[1]<i)
			QUAD_PTS_INVALID(cube[4],cube[5],cube[6],
							cube[7], nbrs[1]<0);
		    if (nbrs[2]<0 || nbrs[2]<i)
			QUAD_PTS_INVALID(cube[0],cube[1],cube[4],
							cube[5], nbrs[2]<0);
		    if (nbrs[3]<0 || nbrs[3]<i)
			QUAD_PTS_INVALID(cube[2],cube[3],cube[6],
							cube[7], nbrs[3]<0);
		    if (nbrs[4]<0 || nbrs[4]<i)
			QUAD_PTS_INVALID(cube[0],cube[2],cube[4],
							cube[6], nbrs[4]<0);
		    if (nbrs[5]<0 || nbrs[5]<i)
			QUAD_PTS_INVALID(cube[1],cube[3],cube[5],
							cube[7], nbrs[5]<0);
		}
	    }
	}

    }


    /* three-dimensional regular connections cube faces dep connections */
    else if (xf->ct==ct_cubes && xf->colors_dep==dep_connections) {

	if (!_dxf_XInvalidConnections(f, xf))
	    return NULL;
	ich = xf->iElts;

	if (!xf->neighbors_array) {

	    int X = xf->k[0], Y = xf->k[1], Z = xf->k[2];
	    int x, y, z, colx, coly, colz;

	    for (x=0; x<X; x++) {
		colx = x<X-1? x : X-2;
		for (y=0; y<Y; y++) {
		    coly = y<Y-1? y : Y-2;
		    for (z=0; z<Z; z++) {
			int valid;
			colz = z<Z-1? z : Z-2;
			col = (colx*(Y-1)+coly)*(Z-1)+colz;
			valid = !ich || DXIsElementValid(ich, col);
			if (valid)
			{
			    if (y<Y-1 && z<Z-1)
				QF(x,y,z, x,y+1,z, x,y,z+1, x,y+1,z+1, x,X-1);
			    if (z<Z-1 && x<X-1)
				QF(x,y,z, x+1,y,z, x,y,z+1, x+1,y,z+1, y,Y-1);
			    if (x<X-1 && y<Y-1)
				QF(x,y,z, x+1,y,z, x,y+1,z, x+1,y+1,z, z,Z-1);
			}
			else
			{
			    if (y<Y-1 && z<Z-1)
				QF_INVALID(x,y,z, x,y+1,z,
						x,y,z+1, x,y+1,z+1, x,X-1);
			    if (z<Z-1 && x<X-1)
				QF_INVALID(x,y,z, x+1,y,z,
						x,y,z+1, x+1,y,z+1, y,Y-1);
			    if (x<X-1 && y<Y-1)
				QF_INVALID(x,y,z, x+1,y,z,
						x,y+1,z, x+1,y+1,z, z,Z-1);
			}
		    }
		}
	    }

	} else {

	    xf->c.cubes = (Cube *)DXGetArrayData(xf->connections_array);
	    if (!xf->c.cubes)
		return ERROR;

	    /* cubes dep positions */
	    for (i=0; i<xf->nconnections; i++) {
		int *nbrs = ((int(*)[6])(xf->neighbors))[i];
		int *cube = (int *)&(xf->c.cubes[i]);
		int valid = !ich || DXIsElementValid(ich, i);
		if (valid)
		{
		    if (nbrs[0]<0 || nbrs[0]<i)
			QUAD_FLAT(cube[0],cube[1],cube[2],
					cube[3], i, nbrs[0]<0);
		    if (nbrs[1]<0 || nbrs[1]<i)
			QUAD_FLAT(cube[4],cube[5],cube[6],
					cube[7], i, nbrs[1]<0);
		    if (nbrs[2]<0 || nbrs[2]<i)
			QUAD_FLAT(cube[0],cube[1],cube[4],
					cube[5], i, nbrs[2]<0);
		    if (nbrs[3]<0 || nbrs[3]<i)
			QUAD_FLAT(cube[2],cube[3],cube[6],
					cube[7], i, nbrs[3]<0);
		    if (nbrs[4]<0 || nbrs[4]<i)
			QUAD_FLAT(cube[0],cube[2],cube[4],
					cube[6], i, nbrs[4]<0);
		    if (nbrs[5]<0 || nbrs[5]<i)
			QUAD_FLAT(cube[1],cube[3],cube[5],
					cube[7], i, nbrs[5]<0);
		}
		else
		{
		    if (nbrs[0]<0 || nbrs[0]<i)
			QUAD_FLAT_INVALID(cube[0],cube[1],cube[2],
					cube[3], i, nbrs[0]<0);
		    if (nbrs[1]<0 || nbrs[1]<i)
			QUAD_FLAT_INVALID(cube[4],cube[5],cube[6],
					cube[7], i, nbrs[1]<0);
		    if (nbrs[2]<0 || nbrs[2]<i)
			QUAD_FLAT_INVALID(cube[0],cube[1],cube[4],
					cube[5], i, nbrs[2]<0);
		    if (nbrs[3]<0 || nbrs[3]<i)
			QUAD_FLAT_INVALID(cube[2],cube[3],cube[6],
					cube[7], i, nbrs[3]<0);
		    if (nbrs[4]<0 || nbrs[4]<i)
			QUAD_FLAT_INVALID(cube[0],cube[2],cube[4],
					cube[6], i, nbrs[4]<0);
		    if (nbrs[5]<0 || nbrs[5]<i)
			QUAD_FLAT_INVALID(cube[1],cube[3],cube[5],
					cube[7], i, nbrs[5]<0);
		}
	    }
	}

    }

    /* update sort pointer */
    gather->current = s;
    return (Object) f;
}


/*
 * Gather the translucent "triangles" from a group
 */

Object
_dxfGroup_Gather(Group g, struct gather *gather, struct tile *tile)
{
    Object o;
    int i;
    struct tile new;

    if (!parameters((Object)g, &new, tile)) return NULL;
    if (DXGetObjectClass((Object)g)==CLASS_COMPOSITEFIELD)
	new.ignore = 1;

    for (i=0; (o=DXGetEnumeratedMember(g, i, NULL)); i++) {
	DXDebug("G", "member %d", i);
	if (!_dxfGather(o, gather, &new))
	    return NULL;
    }

    return (Object) g;
}


/*
 * No faces in a Light
 * XXX - if Shade() removed lights, this would not be necessary
 */

Object
_dxfLight_Gather(Light l, struct gather *gather, struct tile *tile)
{
    return (Object) l;
}


Object
_dxfXform_Gather(Xform x, struct gather *gather, struct tile *tile)
{
    Object o;
    struct tile new;
    if (!parameters((Object)x, &new, tile)) return NULL;
    if (!DXGetXformInfo(x, &o, NULL))
	return NULL;
    return _dxfGather(o, gather, &new);
}


Object
_dxfScreen_Gather(Screen s, struct gather *gather, struct tile *tile)
{
    Object o;
    struct tile new;
    if (!parameters((Object)s, &new, tile)) return NULL;
    new.perspective = 0;
    if (!DXGetScreenInfo(s, &o, NULL, NULL))
	return NULL;
    return _dxfGather(o, gather, &new);
}


Object
_dxfClipped_Gather(Clipped clipped, struct gather *gather, struct tile *tile)
{
    Object render, clipping;
    int n = gather->nfields;
    struct tile new;

    DXGetClippedInfo(clipped, &render, &clipping);
    if (!parameters((Object)clipped, &new, tile)) return NULL;

    if (!_dxfGather(render, gather, &new))
	return NULL;
    /* translucent faces clipped by this surface? */
    if (n!=gather->nfields) {
	if (gather->clipping && gather->clipping!=clipping)
	    DXErrorReturn(ERROR_BAD_PARAMETER,
	       "only one clipping object allowed for all translucent objects");
	gather->clipping = clipping;
    }
    return (Object) clipped;
}

