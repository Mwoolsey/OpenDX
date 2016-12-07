/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/point.c,v 5.0 92/11/12 09:08:34 svs Exp Locker: gresh 
 * 
 */

#include <dx/dx.h>
#include "render.h"

#define PERSPECTIVE(action) {					\
    if (perspective) {					\
	if (p.z>nearPlane)					\
	    action;						\
	p.z = (float)-1.0/p.z;  p.x *= p.z;  p.y *= p.z;	\
    }								\
}

#define COLOR(i) (cmap? \
    cmap[((unsigned char *)colors)[i]] : \
    ((RGBColor *)colors)[i])

#define COLORP(i) (cmap? \
    &(cmap[((unsigned char *)colors)[i]]) : \
    &(((RGBColor *)colors)[i]))

#define OPACITY(i) (omap? \
    omap[((unsigned char *)opacities)[i]] : \
    ((float *)opacities)[i])

Error
_dxf_Points(struct buffer *b, struct xfield *xf)
{
    Point p;
    int i, x, y;
    int width = b->width, height = b->height;
    int ox = b->ox, oy = b->oy;
    int perspective = xf->tile.perspective;
    RGBColor *cmap = xf->cmap;
    Pointer colors = xf->fcolors;
    float nearPlane = xf->nearPlane;
    Point *positions = xf->positions;
    int n = xf->npositions;
    InvalidComponentHandle ich = xf->iPts;
    char ccst = xf->fcst;

    if (!colors)
	DXErrorReturn(ERROR_BAD_PARAMETER, "points must have front colors");

    ASSERT(xf->colors_dep == dep_positions);

    if (b->pix_type==pix_fast) {

	struct fast *buf = b->u.fast;
	struct fast *pix;

	for (i=0; i<n; i++) {

	    if (ich && DXIsElementInvalid(ich, i))
		continue;

	    p = positions[i];
	    PERSPECTIVE(continue);
	    y = ((int)(p.y+30000.0)-30000 + oy);
	    x = ((int)(p.x+30000.0)-30000 + ox);
	    pix = buf + y*width + x;
	    if (0<=x && x<width && 0<=y && y<height && p.z>=pix->z) {
		pix->c = ccst ? COLOR(0) : COLOR(i);
		pix->z = p.z;
	    }
	}

    } else if (b->pix_type==pix_big) {

	struct big *buf = b->u.big;
	struct big *pix;

	for (i=0; i<n; i++) {

	    if (ich && DXIsElementInvalid(ich, i))
		continue;

	    p = positions[i];
	    PERSPECTIVE(continue);
	    y = ((int)(p.y+30000.0)-30000 + oy);
	    x = ((int)(p.x+30000.0)-30000 + ox);
	    pix = buf + y*width + x;
	    if (0<=x && x<width && 0<=y && y<height
	      && p.z>=pix->z && p.z<pix->front && p.z>=pix->back) {
		pix->c = ccst ? COLOR(0) : COLOR(i);
		pix->z = p.z;
	    }
	}
    }

    return OK;
}

/*
 * Note the comment in volume.c about NOT doing the comparison
 * with ->back for translucent objects.
 */

Error
_dxf_Point(struct buffer *b, struct xfield *xf, int i)
{
    Point p;
    int x, y;
    RGBColor *c;
    float o, obar;
    int width = b->width, height = b->height;
    int ox = b->ox, oy = b->oy;
    int perspective = xf->tile.perspective;
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    Pointer colors = xf->fcolors;
    Pointer opacities = xf->opacities;
    float nearPlane = xf->nearPlane;
    Point *positions = xf->positions;
    char ccst = xf->fcst;
    char ocst = xf->ocst;

    ASSERT(xf->colors_dep == dep_positions);

    if (b->pix_type==pix_fast) {

	struct fast *buf = b->u.fast;
	struct fast *pix;

	c = ccst ? COLORP(0) : COLORP(i);
	o = ocst ? OPACITY(0) : OPACITY(i), obar = 1.0-o;
	p = positions[i];
	PERSPECTIVE(return OK);
	y = ((int)(p.y+30000.0)-30000 + oy);
	x = ((int)(p.x+30000.0)-30000 + ox);
	pix = buf + y*width + x;
	if (0<=x && x<width && 0<=y && y<height && p.z>=pix->z) {
	    pix->c.r = o*c->r + obar*pix->c.r;
	    pix->c.g = o*c->g + obar*pix->c.g;
	    pix->c.b = o*c->b + obar*pix->c.b;
	}

    } else if (b->pix_type==pix_big) {

	struct big *buf = b->u.big;
	struct big *pix;

	c = ccst ? COLORP(0) : COLORP(i);
	o = ocst ? OPACITY(0) : OPACITY(i), obar = 1.0-o;
	p = positions[i];
	PERSPECTIVE(return OK);
	y = ((int)(p.y+30000.0)-30000 + oy);
	x = ((int)(p.x+30000.0)-30000 + ox);
	pix = buf + y*width + x;
	if (0<=x && x<width && 0<=y && y<height
	  && p.z>=pix->z && p.z<pix->front) {
	    pix->c.r = o*c->r + obar*pix->c.r;
	    pix->c.g = o*c->g + obar*pix->c.g;
	    pix->c.b = o*c->b + obar*pix->c.b;
	}
    }

    return OK;
}

