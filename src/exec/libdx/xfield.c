/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>



#include <string.h>
#include <dx/dx.h>
#include "render.h"
#include "internals.h"

#define CAT(x,y) x##y


/*
 * get a component array
 */

#define array(array, name, required) {					    \
    xf->array = (Array) DXGetComponentValue(f, name);			    \
    if (!xf->array) {							    \
	if (required) {							    \
	    DXSetError(ERROR_MISSING_DATA, "%s component is missing", name);  \
	    return ERROR;						    \
	}								    \
	return OK;							    \
    }									    \
}


/*
 * check the type of a component
 */

#define check(comp, name, t, dim) {					     \
    if (dim==0 || dim==1) {						     \
	if (!DXTypeCheck(xf->CAT(comp,_array), t, CATEGORY_REAL, 0) &&	     \
	  !DXTypeCheck(xf->CAT(comp,_array), t, CATEGORY_REAL, 1, 1)) {	     \
	    DXSetError(ERROR_DATA_INVALID, "%s component has bad type", name); \
	    return ERROR;						     \
	}								     \
    } else {								     \
	if (!DXTypeCheck(xf->CAT(comp,_array), t, CATEGORY_REAL, 1, dim))	{    \
	    DXSetError(ERROR_DATA_INVALID, "%s component has bad type", name); \
	    return ERROR;						     \
	}								     \
    }									     \
}


/*
 * get the component data and number of items
 */

#define get(comp, xd) {							    \
    if (xd==XD_GLOBAL) {						    \
	*(Pointer*)&xf->comp = DXGetArrayData(xf->CAT(comp,_array));	    \
	if (!xf->comp)							    \
	    return ERROR;						    \
    } else if (xd==XD_LOCAL) {						    \
	*(Pointer*)&xf->comp = DXGetArrayDataLocal(xf->CAT(comp,_array));	    \
	xf->CAT(comp,_local) = 1;					    \
	if (!xf->comp)							    \
	    return ERROR;						    \
    }									    \
    DXGetArrayInfo(xf->CAT(comp,_array), &xf->CAT(n,comp), NULL,NULL,NULL,NULL);\
}


void
_dxf_XZero(struct xfield *xf)
{
    memset(xf, 0, sizeof(struct xfield));
}


/*
 * Get the component data, and check its number of items
 * against another component.  Note - we only consider it a mismatch
 * if xf->count is non-zero; this for example allows _dxf_XOpacities to
 * work even if we haven't done _dxf_XColors (as for example in _Survey).
 */

#define compare(comp, name, count, xd) {				      \
    int n;								      \
    if (xd==XD_GLOBAL) {						      \
	*(Pointer*)&xf->comp = DXGetArrayData(xf->CAT(comp,_array));	      \
	if (!xf->comp)							      \
	    return ERROR;						      \
    } else if (xd==XD_LOCAL) {						      \
	*(Pointer*)&xf->comp = DXGetArrayDataLocal(xf->CAT(comp,_array));     \
	xf->CAT(comp,_local) = 1;					      \
	if (!xf->comp)							      \
	    return ERROR;						      \
    }									      \
    DXGetArrayInfo(xf->CAT(comp,_array), &n, NULL,NULL,NULL,NULL);	      \
    if (n!=count && count!=0) {						      \
	DXSetError(ERROR_DATA_INVALID,					      \
		 "%s component has %d items, expecting %d",		      \
		 name, n, count);					      \
	return ERROR;							      \
    }									      \
}


/*
 * invalid data
 */

Error
_dxf_XInvalidPositions(Field f, struct xfield *xf)
{
    if (! xf->iPts)
    {
	if (DXGetComponentValue(f, "invalid positions")) {
	    xf->iPts = DXCreateInvalidComponentHandle((Object)f,
							NULL, "positions");
	    if (! xf->iPts)
		return ERROR;
	}
	else
	    xf->iPts = NULL;
    }
    
    return OK;
}
	

Error
_dxf_XInvalidConnections(Field f, struct xfield *xf)
{
    if (! xf->iElts)
    {
	if (DXGetComponentValue(f, "invalid connections")) {
	    xf->iElts = DXCreateInvalidComponentHandle((Object)f,
						NULL, "connections");
	    if (! xf->iElts)
		return ERROR;
	}
	else
	    xf->iElts = NULL;
    }
    
    return OK;
}
	
Error
_dxf_XInvalidPolylines(Field f, struct xfield *xf)
{
    if (! xf->iElts)
    {
	if (DXGetComponentValue(f, "invalid polylines")) {
	    xf->iElts = DXCreateInvalidComponentHandle((Object)f,
						NULL, "polylines");
	    if (! xf->iElts)
		return ERROR;
	}
	else
	    xf->iElts = NULL;
    }
    
    return OK;
}
	
Error
_dxf_XLighting(Field f, struct xfield *xf)
{
    Object fattr, battr;
    Object pl = DXGetAttribute((Object)f, "lights");
    if (! pl)
	return OK;

    xf->lights = (LightList)DXGetPrivateData((Private)pl);

    if (! _dxf_XNormals(f, xf, XR_REQUIRED, XD_GLOBAL))
	return ERROR;

    fattr = DXGetAttribute((Object)f, "ambient");
    if (! fattr)
	fattr = DXGetAttribute((Object)f, "front ambient");
    if (fattr) {
	if (! DXExtractFloat(fattr, &(xf->kaf)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "ambient must be float");
    } else xf->kaf = 1.0;

    battr = DXGetAttribute((Object)f, "back ambient");
    if (battr) {
	if (! DXExtractFloat(fattr, &(xf->kab)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "ambient must be float");
    } else xf->kab = xf->kaf;
       
    fattr = DXGetAttribute((Object)f, "diffuse");
    if (! fattr)
	fattr = DXGetAttribute((Object)f, "front diffuse");
    if (fattr) {
	if (! DXExtractFloat(fattr, &(xf->kdf)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "diffuse must be float");
    } else xf->kdf = 0.7;

    battr = DXGetAttribute((Object)f, "back diffuse");
    if (battr) {
	if (! DXExtractFloat(fattr, &(xf->kdb)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "diffuse must be float");
    } else xf->kdb = xf->kdf;
       
    fattr = DXGetAttribute((Object)f, "specular");
    if (! fattr)
	fattr = DXGetAttribute((Object)f, "front specular");
    if (fattr) {
	if (! DXExtractFloat(fattr, &(xf->ksf)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "specular must be float");
    } else xf->ksf = 0.5;

    battr = DXGetAttribute((Object)f, "back specular");
    if (battr) {
	if (! DXExtractFloat(fattr, &(xf->ksb)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "specular must be float");
    } else xf->ksb = xf->ksf;
       
    fattr = DXGetAttribute((Object)f, "shininess");
    if (! fattr)
	fattr = DXGetAttribute((Object)f, "front shininess");
    if (fattr) {
	if (! DXExtractInteger(fattr, &(xf->kspf)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "shininess must be float");
    } else xf->kspf = 10;

    battr = DXGetAttribute((Object)f, "back shininess");
    if (battr) {
	if (! DXExtractInteger(fattr, &(xf->kspb)))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "shininess must be float");
    } else xf->kspb = xf->kspf;
       
    return OK;
}

/*
 * box, points, connections
 */

Error
_dxf_XBox(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    array(box_array, BOX, required);
    /* type checking! */
    get(box, xd);
    return OK;
}


Error
_dxf_XPositions(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    array(positions_array, POSITIONS, required);
    get(positions, xd);
    return OK;
}


Error
_dxf_XConnections(Field f, struct xfield *xf, enum xr required)
{
    static struct info {
	char *name;	/* element type attribute */
	enum ct ct;	/* corresponding ct */
	int n;		/* ints per item */
	int expand;	/* whether to expand data */
	int volume;	/* whether these are volume connections */
    } info[] = {
	{ "lines",	ct_lines,	2,	1,	0 },
	{ "triangles",	ct_triangles,	3,	1,	0 },
	{ "quads",	ct_quads,	4,	0,	0 },
	{ "tetrahedra",	ct_tetrahedra,	4,	1,	1 },
	{ "cubes",	ct_cubes,	8,	0,	1 },
	{ NULL }
    };
    struct info *i;
    Object ct;
    char *s;
    Object attr;

    if (NULL != (attr = DXGetAttribute((Object)f, "volume algorithm")))
    {
	if (! strcmp(DXGetString((String)attr),"face"))
	    xf->volAlg = 1;
	else if (! strcmp(DXGetString((String)attr),"face3"))
	    xf->volAlg = 2;
	else if (! strcmp(DXGetString((String)attr),"plane"))
	    xf->volAlg = 3;
	else
	    xf->volAlg = 4;
    }
    else
	xf->volAlg = 4;


    xf->ct = ct_none;
    array(connections_array, CONNECTIONS, required);
    ct = DXGetAttribute((Object)xf->connections_array, ELEMENT_TYPE);
    s = DXGetString((String)ct);
    if (!s)
	DXErrorReturn(ERROR_BAD_PARAMETER, "bad or missing element type");

    /* try to find it */
    for (i=info; i->name; i++) {
	if (strcmp(s, i->name)==0) {
	    check(connections, "connections", TYPE_INT, i->n);
	    if (i->expand) {
		*(Pointer*)&xf->c = DXGetArrayData(xf->connections_array);
		if (!*(Pointer*)&xf->c)
		    return ERROR;
	    }
	    DXGetArrayInfo(xf->connections_array, &xf->nconnections,
			 NULL,NULL,NULL,NULL);
	    xf->ct = i->ct;
	    xf->volume = i->volume;
	    return OK;
	}
    }

    DXSetError(ERROR_BAD_PARAMETER, "Unrecognized element type %s", ct);
    return ERROR;
}
		

Error
_dxf_XNeighbors(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    int n;

    if (xf->ct==ct_tetrahedra) {
	array(neighbors_array, NEIGHBORS, required);
	check(neighbors, "neighbors", TYPE_INT, 4);
	compare(neighbors, "neighbors", xf->nconnections, xd);
    } else if (xf->ct==ct_cubes) {
	if (DXQueryGridConnections(xf->connections_array, &n, xf->k)) {
	    if (n!=3)
		DXErrorReturn(ERROR_DATA_INVALID,
			    "cubes connections must have dimensionality 3");
	} else {
	    xf->neighbors_array = DXNeighbors(f);
	    if (!xf->neighbors_array)
		return ERROR;
	    check(neighbors, "neighbors", TYPE_INT, 6);
	    compare(neighbors, "neighbors", xf->nconnections, xd);
	}
    }
	
    return OK;
}


/*
 *
 */

Error
_dxf_XColors(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    Array colors_array;
    char *fs, *bs, *s = NULL;
    Object o;
    Type t;
    int r, shape[32];

    /*
     * Try for both colors array first, for performance.  Since this
     * is internally generated, we assume it isn't mapped colors.
     */
    colors_array = (Array) DXGetComponentValue(f, BOTH_COLORS);
    if (colors_array) {
	if (!DXGetArrayInfo(colors_array, &xf->ncolors, &t, NULL, &r, shape))
	    return ERROR;

	if (r != 1 || shape[0] != 3)
	{
	    DXSetError(ERROR_INTERNAL, "both_colors must be rgb vector");
	    return ERROR;
	}

	if (t == TYPE_UBYTE)
	    xf->fbyte = xf->bbyte = 1;
	else if (t == TYPE_FLOAT)
	    xf->fbyte = xf->bbyte = 0;
	else
	{
	    DXSetError(ERROR_INTERNAL, "both_colors must be ubyte or float");
	    return ERROR;
	}

	xf->ncolors /= 2;
	xf->fcolors_array = xf->bcolors_array = colors_array;

	if (xd != XD_NONE)
	{
	    if (DXQueryConstantArray(colors_array, NULL, (Pointer)&xf->fbuf))
	    {
		xf->fcst = xf->bcst = 1;
		xf->fcolors = xf->bcolors = (Pointer)&xf->fbuf;
	    }
	    else if (xd == XD_LOCAL)
	    {
	      xf->fcolors = DXGetArrayDataLocal(xf->fcolors_array);
	      if (!xf->fcolors)
		 return ERROR;
	      xf->bcolors = (Pointer)
		(((RGBColor *)(xf->fcolors)) + xf->ncolors);
	      xf->fcolors_local = 1;
	      xf->fcst = xf->bcst = 0;
	    }
	    else if (xd == XD_GLOBAL)
	    {
	      xf->fcolors = DXGetArrayData(xf->fcolors_array);
	      if (!xf->fcolors)
		 return ERROR;
	      xf->bcolors = (Pointer)
		(((RGBColor *)(xf->fcolors)) + xf->ncolors);
	      xf->fcst = xf->bcst = 0;
	    }
	}
	else
	{
	    xf->fcolors = NULL;
	    xf->bcolors = NULL;
	}
	if (!xf->fcolors)
	    return ERROR;
	o = DXGetAttribute((Object)colors_array, DEP);
	if (o==O_POSITIONS) xf->colors_dep = dep_positions;
	else if (o==O_CONNECTIONS) xf->colors_dep = dep_connections;
	else if (o==O_POLYLINES) xf->colors_dep = dep_polylines;
	else DXErrorReturn(ERROR_MISSING_DATA, "invalid color dependency");
	return OK;
    }

    /* get front/back colors arrays */
    colors_array = (Array) DXGetComponentValue(f, COLORS);
    xf->fcolors_array = (Array) DXGetComponentValue(f, FRONT_COLORS);
    xf->bcolors_array = (Array) DXGetComponentValue(f, BACK_COLORS);
    if (!xf->fcolors_array) xf->fcolors_array = colors_array;
    if (!xf->bcolors_array) xf->bcolors_array = colors_array;
    if (!xf->fcolors_array && !xf->bcolors_array) {
	if (required==XR_REQUIRED) {
	    DXErrorReturn(ERROR_MISSING_DATA, "colors component is missing");
	} else
	    return OK;
    }
    
    
    /* find dependency */
    fs = DXGetString((String)DXGetAttribute((Object)xf->fcolors_array, DEP));
    bs = DXGetString((String)DXGetAttribute((Object)xf->bcolors_array, DEP));
    if (!fs || !bs)
	s = DXGetString((String)DXGetAttribute((Object)colors_array, DEP));
    if (!fs) fs = s;
    if (!bs) bs = s;
    if (fs && bs && strcmp(fs,bs)!=0)
	DXErrorReturn(ERROR_BAD_PARAMETER, "inconsistent colors dep attributes");
    s = fs? fs : bs;
    if (!s) DXErrorReturn(ERROR_MISSING_DATA, "missing color dependency");
    if (strcmp(s,"positions")==0) xf->colors_dep = dep_positions;
    else if (strcmp(s,"connections")==0) xf->colors_dep = dep_connections;
    else if (strcmp(s,"polylines")==0) xf->colors_dep = dep_polylines;
    else DXErrorReturn(ERROR_MISSING_DATA, "invalid color dependency");

    /* number of colors */
    DXGetArrayInfo(xf->fcolors_array? xf->fcolors_array : xf->bcolors_array,
		 &xf->ncolors, NULL, NULL, NULL, NULL);

    if (xf->fcolors_array) {
	Type type;
	int n;
	DXGetArrayInfo(xf->fcolors_array, NULL, &type, NULL, &r, shape);
	if (type==TYPE_UBYTE)
	{
	    if (r == 0 || (r == 1 && shape[0] == 1))
	    {
		check(fcolors, "colors", TYPE_UBYTE, 1);
		array(cmap_array, "color map", 1);
		compare(cmap, "color map", 256, XD_LOCAL);
	    }
	    else
	    {
		check(fcolors, "colors", TYPE_UBYTE, 3);
		xf->fbyte = 1;
	    }
	}
	else
	{
	    check(fcolors, "colors", TYPE_FLOAT, 3);
	    xf->fbyte = 0;
	}
	if (xd != XD_NONE)
	{
	    if (DXQueryConstantArray(xf->fcolors_array, NULL, (Pointer)&xf->fbuf))
	    {
		xf->fcst = 1;
		xf->fcolors = (Pointer)&xf->fbuf;
	    }
	    else if (xd == XD_LOCAL)
	    {
	      xf->fcolors = DXGetArrayDataLocal(xf->fcolors_array);
	      if (!xf->fcolors)
		 return ERROR;
	      xf->fcolors_local = 1;
	      xf->fcst = 0;
	    }
	    else if (xd == XD_GLOBAL)
	    {
	      xf->fcolors = DXGetArrayData(xf->fcolors_array);
	      if (!xf->fcolors)
		 return ERROR;
	      xf->fcst = 0;
	    }
	}
	else
	    xf->fcolors = NULL;

	DXGetArrayInfo(xf->fcolors_array, &n, NULL, NULL, NULL, NULL);
	if (n != xf->ncolors && xf->ncolors != 0)
	    DXSetError(ERROR_DATA_INVALID,		      
		 "front colors component has %d items, expecting %d", n, xf->ncolors);					      
    }
    if (xf->bcolors_array) {
	Type type;
	int n;
	DXGetArrayInfo(xf->bcolors_array, NULL, &type, NULL, &r, shape);
	if (type==TYPE_UBYTE) {
	    if (r == 0 || (r == 1 && shape[0] == 1))
	    {
		check(fcolors, "colors", TYPE_UBYTE, 1);
		array(cmap_array, "color map", 1);
		compare(cmap, "color map", 256, XD_LOCAL);
	    }
	    else
	    {
		check(bcolors, "colors", TYPE_UBYTE, 3);
		xf->bbyte = 1;
	    }
	}
	else
	{
	    if (xf->cmap_array) {
		DXErrorReturn(ERROR_BAD_PARAMETER,
			    "mixed delayed and direct colors "
			    "are not supported");
	    }
	    check(bcolors, "colors", TYPE_FLOAT, 3);
	    xf->bbyte = 0;
	}
	if (xd != XD_NONE)
	{
	    if (DXQueryConstantArray(xf->bcolors_array, NULL, (Pointer)&xf->bbuf))
	    {
		xf->bcst = 1;
		xf->bcolors = (Pointer)&xf->bbuf;
	    }
	    else if (xd == XD_LOCAL)
	    {
	      xf->bcolors = DXGetArrayDataLocal(xf->bcolors_array);
	      if (!xf->bcolors)
		 return ERROR;
	      xf->bcolors_local = 1;
	      xf->bcst = 0;
	    }
	    else if (xd == XD_GLOBAL)
	    {
	      xf->bcolors = DXGetArrayData(xf->bcolors_array);
	      if (!xf->bcolors)
		 return ERROR;
	      xf->bcst = 0;
	    }
	}
	else
	    xf->bcolors = NULL;

	DXGetArrayInfo(xf->bcolors_array, &n, NULL, NULL, NULL, NULL);
	if (n != xf->ncolors && xf->ncolors != 0)
	DXSetError(ERROR_DATA_INVALID,		      
		 "back colors component has %d items, expecting %d", n, xf->ncolors);					      
    }

    return OK;
}


/*
 * Normals
 */

#define CHECK_NORMAL(p) { \
    float d = DXDot(xf->normals[t.p], c); \
    if (d < 0) \
	DXWarning("inconsistent normals: tri %d, pt p, d=%g", i, d); \
}

#if 0   /* was !ibmpvs && !OPTIMIZED, but isn't called anymore */
static void
check_normals(struct xfield *xf)
{
    int i, every;

    if (!xf->normals)
	xf->normals = (Point *) DXGetArrayData(xf->normals_array);
    if (!xf->normals)
	return;
    if (xf->ct!=ct_triangles)
	return;
    if (xf->colors_dep!=dep_positions)
	return;
    if (xf->normals_dep!=dep_positions)
	return;
    if (!xf->positions)
	xf->positions = (Point *) DXGetArrayData(xf->positions_array);
    if (!xf->positions)
	return;

    every = xf->nconnections/5;
    if (every==0)
	every = 1;

    for (i=0; i<xf->nconnections; i+=every) {
	Triangle t;
	Point p , q, r;
	Vector a, b, c;
	t = xf->c.triangles[i];
	p = xf->positions[t.p];
	q = xf->positions[t.q];
	r = xf->positions[t.r];
	a = DXSub(q, p);
	b = DXSub(r, p);
	c = DXCross(a, b);
	CHECK_NORMAL(p);
	CHECK_NORMAL(q);
	CHECK_NORMAL(r);
    }
}
#endif


Error
_dxf_XNormals(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    char *s;
    Object o;
    int n, m;

    if (NULL != (o = DXGetAttribute((Object)f, "shade")))
    {
	int flag;

	if (DXGetObjectClass(o) != CLASS_ARRAY)
	    DXErrorReturn(ERROR_BAD_CLASS, "shade attribute must be class ARRAY");

	if (! DXExtractInteger(o, &flag))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "shade attribute must be integer");

	if (flag == 0)
	{
	    xf->normals_array = NULL;
	    return OK;
	}
	else if (flag != 1)
	    DXErrorReturn(ERROR_DATA_INVALID, "shade attribute value must be 0 or 1");
    }

    array(normals_array, NORMALS, required);
    check(normals, "normals", TYPE_FLOAT, 3);
    s = DXGetString((String)DXGetAttribute((Object)xf->normals_array, DEP));
    if (strcmp(s,"positions")==0) xf->normals_dep = dep_positions;
    else if (strcmp(s,"connections")==0) xf->normals_dep = dep_connections;
    else if (strcmp(s,"polylines")==0) xf->normals_dep = dep_polylines;
    else DXErrorReturn(ERROR_MISSING_DATA, "invalid normals dependency");

    if (xd != XD_NONE)
    {
	if (DXQueryConstantArray(xf->normals_array, NULL, (Pointer)&xf->nbuf))
	{
	    xf->ncst = 1;
	    xf->normals = (Pointer)&xf->nbuf;
	}
	else if (xd == XD_LOCAL)
	{
	  xf->normals = DXGetArrayDataLocal(xf->normals_array);
	  if (!xf->normals)
	     return ERROR;
	  xf->normals_local = 1;
	  xf->ncst = 0;
	}
	else if (xd == XD_GLOBAL)
	{
	  xf->normals = DXGetArrayData(xf->normals_array);
	  if (!xf->normals)
	     return ERROR;
	  xf->ncst = 0;
	}
    }
    else
    {
	xf->normals = NULL;
    }

    DXGetArrayInfo(xf->normals_array, &n, NULL, NULL, NULL, NULL);
    

    if (xf->normals_dep==dep_positions) {
	m = xf->npositions;
    } else if (xf->normals_dep==dep_connections) {
	m = xf->nconnections;
    } else if (xf->normals_dep==dep_polylines) {
	m = xf->npolylines;
    } else
	DXErrorReturn(ERROR_DATA_INVALID, "bad normals dependency");

    if (m && m != n)
    {
	DXSetError(ERROR_DATA_INVALID,		
		 "normals component has %d items, expecting %d", n, m);	
	return ERROR;
    }

    return OK;
}

Error
_dxf_XOpacities(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    Type type;
    int n;

    array(opacities_array, OPACITIES, required);
    if (xf->fcolors_array)
    {
	Object o;
	char *s;

	o = DXGetComponentAttribute(f, OPACITIES, DEP);
	if (! o)
	    DXErrorReturn(ERROR_DATA_INVALID, "missing opacities dependency");
	
	s = DXGetString((String)o);
	if (! s)
	    DXErrorReturn(ERROR_DATA_INVALID, "invalid opacities dependency");
	
	if ((!strcmp(s, "positions")   && xf->colors_dep != dep_positions)   ||
	    (!strcmp(s, "polylines")   && xf->colors_dep != dep_polylines)   ||
	    (!strcmp(s, "connections") && xf->colors_dep != dep_connections))
	    DXErrorReturn(ERROR_DATA_INVALID, 
		"mismatch between opacities and colors dependencies");
    }

    if (xd != XD_NONE)
    {
	if (DXQueryConstantArray(xf->opacities_array, NULL, (Pointer)&xf->obuf))
	{
	    xf->ocst = 1;
	    xf->opacities = (Pointer)&xf->obuf;
	}
	else if (xd == XD_LOCAL)
	{
	  xf->opacities = DXGetArrayDataLocal(xf->opacities_array);
	  if (!xf->opacities)
	     return ERROR;
	  xf->opacities_local = 1;
	  xf->ocst = 0;
	}
	else if (xd == XD_GLOBAL)
	{
	  xf->opacities = DXGetArrayData(xf->opacities_array);
	  if (!xf->opacities)
	     return ERROR;
	  xf->ocst = 0;
	}
    }
    else
    {
	xf->opacities = NULL;
    }

    DXGetArrayInfo(xf->opacities_array, &n, NULL, NULL, NULL, NULL);

    if (n != xf->ncolors)
    {
	DXSetError(ERROR_DATA_INVALID,		
		 "normals component has %d items, expecting %d", n, xf->ncolors);
	return ERROR;
    }

    DXGetArrayInfo(xf->opacities_array, NULL, &type, NULL, NULL, NULL);
    if (type==TYPE_UBYTE) {

	check(opacities, "opacities", TYPE_UBYTE, 1);

	/*
	 * A scalar (or 1-vector) opacity array implies either a 
	 * opacity map or honest-to-god scaled opacities
	 */
	array(omap_array, "opacity map", 0);
	if (xf->omap_array)
	{
	    compare(omap, "opacity map", 256, XD_LOCAL);
	    xf->obyte = 0;
	}
	else
	    xf->obyte = 1;
    } else {
	check(opacities, "opacities", TYPE_FLOAT, 1);
	xf->obyte = 0;
    }

    return OK;
}


Error
_dxf_XPolylines(Field f, struct xfield *xf, enum xr required, enum xd xd)
{
    array(polylines_array, POLYLINES, required);
    check(polylines, "polylines", TYPE_INT, 0);
    get(polylines, xd);

    /* edges required if polylines present */
    array(edges_array, "edges", required);
    check(edges, "edges", TYPE_INT, 0);
    get(edges, xd);

    xf->ct = ct_polylines;
    xf->nconnections = xf->npolylines;

    return OK;
}

#if 0

#define FREE(comp) \
    if (xf->CAT(comp,_local)) \
        DXFreeArrayDataLocal(xf->CAT(comp,_array), (Pointer)xf->comp); \
    if (xf->CAT(comp,_array)) \
        DXFreeExpandedArrayData(xf->CAT(comp,_array));
#else

#define FREE(comp) \
    if (xf->CAT(comp,_local)) \
        DXFreeArrayDataLocal(xf->CAT(comp,_array), (Pointer)xf->comp); 
#endif

Error
_dxf_XFreeLocal(struct xfield *xf)
{
    FREE(box);
    FREE(positions);
    FREE(neighbors);
    FREE(fcolors);
    FREE(bcolors);
    FREE(normals);
    FREE(opacities);
    FREE(polylines);
    FREE(edges);
    FREE(cmap);
    FREE(omap);
    DXFreeInvalidComponentHandle(xf->iPts);
    DXFreeInvalidComponentHandle(xf->iElts);
    return OK;
}


