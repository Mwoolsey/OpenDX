/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>
#include "render.h"
#include "internals.h"
#include "zclip.h"


#define EXTRA 30

int
_dxf_zclip_triangles(struct xfield *xf, int *elements, int i, int *indices,
	int nelt, struct xfield *xx, inv_stat invalid_status)
{
#if defined(__PURIFY__) 
    /*  Purify on the HP can't handle the #defines for this function */
    return nelt;
#else
    static Triangle xtriangles[EXTRA];
    static RGBColor xfcolors[3*EXTRA];
    static RGBColor xbcolors[3*EXTRA];
    static Point    xpositions[3*EXTRA];
    static float    xopacities[3*EXTRA];
    static Vector   xnormals[3*EXTRA];
    static int      xindices[EXTRA];
    InvalidComponentHandle ich =
	(invalid_status == INV_UNKNOWN) ? xf->iElts : NULL;
    Triangle *xtri;
    RGBColor *xfc, *xbc;
    Vector   *xnrm;
    Point *xpos, *positions, *p1, *p2, *p3;
    float *xop;
    RGBColor *fcolors   = xf->fcolors;
    RGBColor *bcolors   = xf->bcolors;
    float    *opacities = xf->opacities;
    Vector   *normals   = xf->normals;
    float    nearPlane       = xf->nearPlane;

    int      fcst = xf->fcst;
    int      bcst = xf->bcst;
    int      ncst = xf->ncst;
    int      ocst = xf->ocst;

    int      nxtri, nxpt, v1, v2, v3, interp_colors, interp_normals;
    float    z1, z2, z3;

    RGBColor *cmap = xf->cmap;
    float    *omap = xf->omap;

    /* no clipping to do? */
    if (xf->box[7].z <= nearPlane) {
	xx->nconnections = 0;
	DXDebug("R", "%d triangles accepted", nelt);
	return nelt;
    }

    positions = xf->positions;

    xfc   = xfcolors;
    xbc   = xbcolors;
    xpos  = xpositions;
    xop   = xopacities;
    xnrm  = xnormals;

    xpos  = xpositions;
    xtri  = xtriangles;
    nxtri = 0;
    nxpt  = 0;

    interp_colors  = xf->colors_dep == dep_positions;
    interp_normals = xf->normals && xf->normals_dep==dep_positions;

    for (elements += 3*i; i<nelt && nxtri<EXTRA-2; i++, elements += 3) {
	if (ich && DXIsElementInvalid(ich, i))
	    continue;
	v1 = elements[0];
	v2 = elements[1];
	v3 = elements[2];
	p1 = &positions[v1];
	p2 = &positions[v2];
	p3 = &positions[v3];
	z1 = p1->z;
	z2 = p2->z;
	z3 = p3->z;
	TRYCLIP(z1,z2,z3, p1,p2,p3, v1,v2,v3) else
	TRYCLIP(z2,z3,z1, p2,p3,p1, v2,v3,v1) else
	TRYCLIP(z3,z1,z2, p3,p1,p2, v3,v1,v2)
    }

    if (nxtri) {
	*xx = *xf;
	xx->cmap = NULL;
	xx->omap = NULL;
	xx->fcolors = fcolors? (Pointer)xfcolors : NULL;
	xx->bcolors = bcolors? (Pointer)xbcolors : NULL;
	xx->opacities = opacities? (Pointer)xopacities : NULL;
	xx->positions = positions? xpositions : NULL;
	xx->normals = normals? xnormals : NULL;
	xx->indices = indices? xindices : NULL;
	xx->c.triangles = xtriangles;
	xx->lights = xf->lights;
    }
    DXDebug("R", "%d triangles in, %d clipped triangles out",  nelt, nxtri);
    xx->nconnections = nxtri;

    return i;
#endif
}
