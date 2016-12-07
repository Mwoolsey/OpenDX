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

/*
 * The following is a kludge relating to to whether or not the
 * z-buffer is updated when a translucent surface is rendered.
 * When we render two transparent objects, we want the closer to
 * overlay the farther, which itself overlays still farther surfaces.
 * A centroid sort is used to approximately prioritize the transparent
 * surfaces.  If it is incorrect at a given pixel, then (in the absense
 * of any other information) they are overlaid in the incorrect order.
 * Which, if the transparency of the front-most (and rendered first)
 * transparent surface is low, is an OK approximation.  If, however, the
 * frontmost surface is relatively opaque, then this approximation
 * becomes less accurate.  In this case it is better to forget about
 * the second transparent surface (which would erroneously be laid over
 * it).  So when the surface is relatively opaque, we update the z-buffer
 * to eliminate subsequent farther surfaces.
 *
 * Note that this appears in triangles.h also.
 */
#define OPACITY_THRESHOLD 0.5


#define ABS(x) ((x)<0? -(x) : (x))
#define FLOOR(x) ((int)((x)+10000.0)-10000)
#define ROUND(x) ((int)((x)+10000.5)-10000)

/*
 * For translucent lines, we can assume that ->back has
 * already been merged into ->z; furthermore, we reuse ->back
 * in volume rendering, so it is incorrect to compare against
 * ->back here because we may be mixed in with volume faces
 * This also occurs in triangle.c.
 */

#define SCALE (1<<10)	/* integer scaling */
#define MAXC (1<<20)	/* max coordinates before overflow */

#define CLIPZ      (z>pix->z && z<pix->front && z>pix->back)
#define NOCLIPZ    (z>pix->z)
#define TRANSCLIPZ (z>pix->z && z<pix->front)

#define LOOP(flat,op,ztest) {						    \
									    \
    for (i=0; i<=nmaj; i++) {						    \
	if (op) {							    \
	    if (0<=x && x<width && 0<=y && y<height && ztest) {		    \
		obar = 1.0 - o;						    \
		pix->c.r = o*r + obar*pix->c.r;				    \
		pix->c.g = o*g + obar*pix->c.g;				    \
		pix->c.b = o*b + obar*pix->c.b;				    \
		if (o > OPACITY_THRESHOLD)                                  \
		    pix->z = z;						    \
	    }								    \
	} else {							    \
	    if (0<=x && x<width && 0<=y && y<height && ztest) {		    \
		pix->c.r = r;						    \
		pix->c.g = g;						    \
		pix->c.b = b;						    \
		pix->z = z;						    \
	    }								    \
	}								    \
	pix += dpix_dmaj;						    \
	x += dx_dmaj;							    \
	y += dy_dmaj;							    \
	if ((frac-=min)<0) {						    \
	    frac += maj;						    \
	    pix += dpix_dmin;						    \
	    x += dx_dmin;						    \
	    y += dy_dmin;						    \
	}								    \
	z += dz;							    \
	if (!flat) {							    \
	    r += dr;							    \
	    g += dg;							    \
	    b += db;							    \
	    if (op) o += dop;						    \
	}								    \
    }									    \
}

#define CLIP(flat,op,clipz) {	\
    if (clip)			\
	LOOP(flat,op,clipz)	\
    else			\
	LOOP(flat,op,NOCLIPZ)	\
}

#define ZCLIP(p,p1, c,c1, o,o1) {			\
    float a = (nearPlane-p.z) / (p1.z-p.z); /* XXX */	\
    float abar = 1.0-a;					\
    p.x = p.x*abar + p1.x*a;				\
    p.y = p.y*abar + p1.y*a;				\
    p.z = nearPlane;					\
    c.r = c.r*abar + c1.r*a;				\
    c.g = c.g*abar + c1.g*a;				\
    c.b = c.b*abar + c1.b*a;				\
    o = o*abar + o1*a;					\
    p.z = nearPlane;					\
}

#define XYCLIP(p,p1, c,c1, o,o1, x,y, dx,dy, op, mm) {		\
    float m = mm;						\
    if (p.x op m) {						\
        float d = (m - p.x) / dx;				\
	if (d<.5) {						\
	    p.y += d * dy;					\
	    p.z += d * (p1.z - p.z);				\
	    c.r += d * (c1.r - c.r);				\
	    c.g += d * (c1.g - c.g);				\
	    c.b += d * (c1.b - c.b);				\
	    o += d * (o1 - o);					\
	} else {						\
	    d = (m - p1.x) / dx;				\
	    p.y = p1.y + d * dy;				\
	    p.z = p1.z + d * (p1.z - p.z);			\
	    c.r = c1.r + d * (c1.r - c.r);			\
	    c.g = c1.g + d * (c1.g - c.g);			\
	    c.b = c1.b + d * (c1.b - c.b);			\
	    o = o1 + d * (o1 - o);				\
	}							\
	p.x = m;						\
    }								\
}



static Error
line(struct buffer *buf, struct xfield *xf, Line l,
     int flat, RGBColor c, RGBColor c1, float o, float o1, int clip)
{
    Point p, p1;
    float length, d, dr = 0, dg = 0, db = 0, r, g, b, z;
    float dx, dy, dz, dop = 0, obar, a;
    int maj, dpix_dmaj, dy_dmaj, dx_dmaj;
    int min, dpix_dmin, dy_dmin, dx_dmin;
    int i, nmaj, x, y, frac;
    int width = buf->width, height = buf->height;

    /* positions */
    p = xf->positions[l.p];
    p1 = xf->positions[l.q];

    /* perspective */
    if (xf->tile.perspective) {
	float nearPlane = xf->nearPlane;
	if (p.z>nearPlane && p1.z>nearPlane)
	    return OK;
	if (p.z>nearPlane) {
	    ZCLIP(p,p1, c,c1, o,o1);
	} else if (p1.z>nearPlane) {
	    ZCLIP(p1,p, c1,c, o1,o);
	}
	p.z = (float)-1.0/p.z;  p.x *= p.z;  p.y *= p.z;
	p1.z = (float)-1.0/p1.z;  p1.x *= p1.z;  p1.y *= p1.z;

    }

    /* quick check */
    if ((p.x<buf->min.x-.5 && p1.x<buf->min.x-.5) ||
	(p.y<buf->min.y-.5 && p1.y<buf->min.y-.5) ||
	(p.x>buf->max.x+.5 && p1.x>buf->max.x+.5) ||
	(p.y>buf->max.y+.5 && p1.y>buf->max.y+.5))
	return OK;

#if 0
    /* overflow check - XXX check performance impact */
    if (p.x>=MAXC || p.x<=-MAXC || p.y>=MAXC || p.y<=-MAXC
      || p1.x>=MAXC || p1.x<=-MAXC || p1.y>=MAXC || p1.y<=-MAXC)
	DXErrorReturn(ERROR_BAD_PARAMETER, "camera causes numerical overflow");
#endif

    /* deltas */
    dx = p1.x - p.x;
    dy = p1.y - p.y;

    /* octant-dependent setup */
    if (ABS(dy) < ABS(dx)) {

	XYCLIP(p,p1, c,c1, o,o1, x,y,  dx, dy, <, buf->min.x-.5);
	XYCLIP(p1,p, c1,c, o1,o, x,y, -dx,-dy, <, buf->min.x-.5);
	XYCLIP(p,p1, c,c1, o,o1, x,y,  dx, dy, >, buf->max.x+.5);
	XYCLIP(p1,p, c1,c, o1,o, x,y, -dx,-dy, >, buf->max.x+.5);
	dx = p1.x - p.x;
	dy = p1.y - p.y;

	x = ROUND(p.x);
	d = x - p.x;
	a = p.y + (dx? dy/dx*d : 0);
	y = ROUND(a);

	if (dx>0) dx_dmaj =  1,  length =  dx,  nmaj = ROUND(p1.x) - x;
	else      dx_dmaj = -1,  length = -dx,  nmaj = x - ROUND(p1.x);
	maj = length * SCALE;
	frac = (a - y) * maj;
	if (dy>0) dy_dmin =  1,  min =  dy*SCALE,  frac = maj/2 - frac;
	else      dy_dmin = -1,  min = -dy*SCALE,  frac = maj/2 + frac;
	dx_dmin = 0;
	dy_dmaj = 0;

    } else {

	XYCLIP(p,p1, c,c1, o,o1, y,x,  dy, dx, <, buf->min.y-.5);
	XYCLIP(p1,p, c1,c, o1,o, y,x, -dy,-dx, <, buf->min.y-.5);
	XYCLIP(p,p1, c,c1, o,o1, y,x,  dy, dx, >, buf->max.y+.5);
	XYCLIP(p1,p, c1,c, o1,o, y,x, -dy,-dx, >, buf->max.y+.5);
	dx = p1.x - p.x;
	dy = p1.y - p.y;

	y = ROUND(p.y);
	d = y - p.y;
	a = p.x + (dy? dx/dy*d : 0);
	x = ROUND(a);

	if (dy>0) dy_dmaj =  1,  length =  dy,  nmaj = ROUND(p1.y) - y;
	else      dy_dmaj = -1,  length = -dy,  nmaj = y - ROUND(p1.y);
	maj = length * SCALE;
	frac = (a - x) * maj;
	if (dx>0) dx_dmin =  1,  min =  dx*SCALE,  frac = maj/2 - frac;
	else      dx_dmin = -1,  min = -dx*SCALE,  frac = maj/2 + frac;
	dy_dmin = 0;
	dx_dmaj = 0;

	length = ABS(dy);
    }

    /* delta z, color, taking care not to divide by 0 */
    dz  = length? (p1.z - p.z) / length : 0.0;

    if (! flat)
    {
	dop = length? (o1   - o  ) / length : 0.0;
	dr  = length? (c1.r - c.r) / length : 0.0;
	dg  = length? (c1.g - c.g) / length : 0.0;
	db  = length? (c1.b - c.b) / length : 0.0;
	r = c.r + dr * d;
	g = c.g + dg * d;
	b = c.b + db * d;
	o += dop * d;
    }
    else
    {
	r = c.r;
	g = c.g;
	b = c.b;
    }

    z = p.z + dz * d;

    /* delta pix */
    dpix_dmaj = dy_dmaj*width + dx_dmaj;
    dpix_dmin = dy_dmin*width + dx_dmin;

    /* offset */
    x += buf->ox;
    y += buf->oy;

    if (buf->pix_type==pix_fast) {
	struct fast *pix = buf->u.fast + y*width + x;
	ASSERT(!clip);
	if (flat)
	    if (xf->opacities)
		LOOP(1,1, NOCLIPZ)
	    else
		LOOP(1,0, NOCLIPZ)
	else
	    if (xf->opacities)
		LOOP(0,1, NOCLIPZ)
	    else
		LOOP(0,0, NOCLIPZ)
    } else if (buf->pix_type==pix_big) {
	struct big *pix = buf->u.big + y*width + x;
	if (flat)
	    if (xf->opacities)
		CLIP(1,1, TRANSCLIPZ)
	    else
		CLIP(1,0, CLIPZ)
	else
	    if (xf->opacities)
		CLIP(0,1, TRANSCLIPZ)
	    else
		CLIP(0,0, CLIPZ)
    }

    return OK;
}


#define COLOR(i, dst) \
{ \
    if (cmap) \
	dst = cmap[((unsigned char *)colors)[i]]; \
    else if (fbyte) \
    { \
	ubyte *ptr = ((ubyte *)colors + 3*i); \
	dst.r = _dxd_ubyteToFloat[*ptr++]; \
	dst.g = _dxd_ubyteToFloat[*ptr++]; \
	dst.b = _dxd_ubyteToFloat[*ptr++]; \
    } \
    else \
	dst = ((RGBColor *)colors)[i]; \
}

#define OPACITY(i, dst) \
{ \
    if (opacities) { \
	if (omap) \
	    dst = omap[((unsigned char *)opacities)[i]]; \
	else if (obyte) \
	    dst = _dxd_ubyteToFloat[((unsigned char *)opacities)[i]]; \
	else \
	    dst = ((float *)opacities)[i]; \
    } else \
	dst = 1.0; \
}

Error
_dxf_Line(struct buffer *b, struct xfield *xf,
	int n, Line *lines, int *indices, int clip, inv_stat inv)
{
    unsigned char *colors = (unsigned char *)xf->fcolors;
    unsigned char *opacities = (unsigned char *)xf->opacities;
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    int i;
    InvalidComponentHandle ich = xf->iElts;
    RGBColor cbuf[2];
    char fbyte = xf->fbyte;
    char obyte = xf->obyte;
    char fcst = xf->fcst;
    char ocst = xf->ocst;
    float pop = 0, qop = 0;
    int odep = xf->colors_dep == dep_connections;

    if (ocst) {
	OPACITY(0, pop);
	OPACITY(0, qop);
    }

    if (fbyte || obyte)
	_dxf_initUbyteToFloat();

    if (xf->lights)
    {

	_dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
			    xf->kab, xf->kdb, xf->ksb, xf->kspb,
			    xf->fcolors, xf->bcolors, xf->cmap, 
			    xf->normals, xf->lights, 
			    xf->colors_dep, xf->normals_dep,
			    cbuf, NULL, 2, xf->fcst, xf->bcst, xf->ncst, 
			    fbyte, 0);
	
	for (i=0; i<n; i++)
	{
	    int index = (indices) ? indices[i] : i;
	    Line *l = &lines[index];
	    
	    if (ich && DXIsElementInvalid(ich, index))
		continue;
	    
	    if (! _dxfApplyLights((int *)l, &index, 1))
		return ERROR;

	    if (!ocst) {
		if (odep) {
		    OPACITY(index, pop);
		    qop = pop;
		} else {
		    OPACITY(l->p, pop);
		    OPACITY(l->q, qop);
		}
	    }

	    if (!line(b, xf, *l, 0, cbuf[0], cbuf[1], pop, qop, clip))
		return ERROR;
	}
    }
    else
    {
	RGBColor pColor, qColor;

	if (!colors)
	    DXErrorReturn(ERROR_BAD_PARAMETER, "lines must have front colors");
	
	if (fcst)
	{
	    COLOR(0, qColor);
	    pColor = qColor;
	}
	
	for (i=0; i<n; i++) {
	    int index = (indices) ? indices[i] : i;
	    Line *l = &lines[index];
	    Error rc;
	    
	    if (ich && DXIsElementInvalid(ich, index))
		continue;
	    
	    if (!ocst) {
		if (odep) {
		    OPACITY(index, pop);
		    qop = pop;
		} else {
		    OPACITY(l->p, pop);
		    OPACITY(l->q, qop);
		}
	    }

	    if (! fcst) {
		if (odep) {
		    COLOR(index, pColor);
		    qColor = pColor;
		} else {
		    COLOR(l->p, pColor);
		    COLOR(l->q, qColor);
		}
	    }

	    rc = line(b, xf, *l, 0, pColor, qColor, pop, qop, clip);

	    if (!rc) return ERROR;
	}
    }

    return OK;
}


Error
_dxf_LineFlat(struct buffer *b, struct xfield *xf, int n, Line *lines,
	  int *indices,
	  Pointer colors, Pointer opacities, int clip, inv_stat inv)
{
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    char obyte = xf->obyte;
    int i;
    InvalidComponentHandle ich = xf->iElts;
    RGBColor pColor, qColor;
    char fcst = xf->fcst;
    char fbyte = xf->fbyte;
    char ocst = xf->ocst;
    float pop = 0, qop = 0;
    int odep = xf->colors_dep == dep_connections;

    if (ocst) {
	OPACITY(0, pop);
	OPACITY(0, qop);
    }

    if (!xf->fcolors)
	DXErrorReturn(ERROR_BAD_PARAMETER, "lines must have front colors");

    if (fcst)
    {
	COLOR(0, pColor);
	qColor = pColor;
    }

    for (i=0; i<n; i++) {
	int index = (indices) ? indices[i] : i;
	Line *l = &lines[index];
	Error rc;

	if (ich && DXIsElementInvalid(ich, index))
	    continue;

	if (!ocst) {
	    if (odep) {
		OPACITY(index, pop);
		qop = pop;
	    } else {
		OPACITY(l->p, pop);
		OPACITY(l->q, qop);
	    }
	}

	if (! fcst) {
	    if (odep) {
		COLOR(index, pColor);
		qColor = pColor;
	    } else {
		COLOR(l->p, pColor);
		COLOR(l->q, qColor);
	    }
	}

	rc = line(b, xf, *l, 1, pColor, qColor, pop, qop, clip);

	if (!rc) return ERROR;
    }

    return OK;
}

Error
_dxf_Polyline(struct buffer *b, struct xfield *xf,
	int n, int *polylines, int *indices, int clip, inv_stat inv)
{
    unsigned char *colors = (unsigned char *)xf->fcolors;
    unsigned char *opacities = (unsigned char *)xf->opacities;
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    int i;
    InvalidComponentHandle ich = xf->iElts;
    char fbyte = xf->fbyte;
    char obyte = xf->obyte;
    char fcst = xf->fcst;
    char ocst = xf->ocst;
    float pop = 0, qop = 0;
    dependency cdep = xf->colors_dep;
    int start, last, knt, j;
    Line *l;
    RGBColor *cbuf = NULL;
    int cbufknt = 0;

    if (indices)
    {
	DXSetError(ERROR_INTERNAL, "non-null indices in PolyLine");
	return ERROR;
    }

    if (ocst) {
	OPACITY(0, pop);
	OPACITY(0, qop);
    }

    if (fbyte || obyte)
	_dxf_initUbyteToFloat();

    if (xf->lights)
    {
	for (i=0; i<n; i++)
	{
	    start = polylines[i];
	    last  = (i == n-1) ? xf->nedges-1 : polylines[i+1]-1;
	    knt   = last - start;

	    if (ich && DXIsElementInvalid(ich, i))
		continue;

	    if (knt+1 > cbufknt)
	    {
		cbuf = (RGBColor *)DXReAllocate(cbuf, (knt+1)*sizeof(RGBColor));
		if (! cbuf)
		    goto error;
		
		cbufknt = (knt+1);
	    }

	    l = (Line *)(xf->edges + start);

	    /*
	     * As far as appliyong lights goes, we pretend that polylines
	     * are effectively connections elements of varying numbers
	     * of vertices
	     */

	    _dxfInitApplyLights(xf->kaf, xf->kdf, xf->ksf, xf->kspf,
				xf->kab, xf->kdb, xf->ksb, xf->kspb,
				xf->fcolors, xf->bcolors, xf->cmap, 
				xf->normals, xf->lights, 
				xf->colors_dep, xf->normals_dep, cbuf,
				NULL, (knt+1), xf->fcst, xf->bcst, xf->ncst, 
				fbyte, 0);
	

	    if (! _dxfApplyLights((int *)l, &i, 1))
		return ERROR;

	    if (!ocst && cdep == dep_positions)
		OPACITY(l->q, qop);

	    for (j = 0; j < knt; j++, l = (Line *)(((int *)l) + 1))
	    {
		if (!ocst && cdep == dep_positions)
		{
		    pop = qop;
		    OPACITY(l->q, qop);
		}

		if (!line(b, xf, *l, 0, cbuf[j], cbuf[j+1], pop, qop, clip))
		    return ERROR;
	    }
	}
    }
    else
    {
	RGBColor pColor, qColor;

	if (!colors)
	    DXErrorReturn(ERROR_BAD_PARAMETER, "lines must have front colors");
	
	if (fcst)
	{
	    COLOR(0, qColor);
	    pColor = qColor;
	}
	
	for (i=0; i<n; i++)
	{
	    start = polylines[i];
	    last  = (i == n-1) ? xf->nedges-1 : polylines[i+1]-1;
	    knt   = last - start;
	    
	    if (ich && DXIsElementInvalid(ich, i))
		continue;

	    l = (Line *)(xf->edges + start);

	    if (!ocst)
	    {
		if (cdep == dep_polylines)
		{
		    OPACITY(i, pop);
		    qop = pop;
		}
		else if (cdep == dep_positions)
		{
		    OPACITY(l->p, qop);
		}
	    }

	    if (!fcst)
	    {
		if (cdep == dep_polylines)
		{
		    COLOR(i, pColor);
		    qColor = pColor;
		}
		else if (cdep == dep_positions)
		{
		    COLOR(l->p, qColor);
		}
	    }

	    for (j = 0; j < knt; j++, l = (Line *)(((int *)l) + 1))
	    {
	    
		if (!ocst && cdep == dep_positions)
		{
		    pop = qop;
		    OPACITY(l->q, qop);
		}

		if (!fcst && cdep == dep_positions)
		{
		    pColor = qColor;
		    COLOR(l->q, qColor);
		}

		if (!line(b, xf, *l, 0, pColor, qColor, pop, qop, clip))
		    return ERROR;
	    }
	}
    }

    if (cbuf)
	DXFree((Pointer)cbuf);

    return OK;

error:
    if (cbuf)
	DXFree((Pointer)cbuf);

    return ERROR;
}


Error
_dxf_PolylineFlat(struct buffer *b, struct xfield *xf, int n, int *polylines,
	  int *indices,
	  Pointer colors, Pointer opacities, int clip, inv_stat inv)
{
    RGBColor *cmap = xf->cmap;
    float *omap = xf->omap;
    char obyte = xf->obyte;
    int i;
    InvalidComponentHandle ich = xf->iElts;
    RGBColor pColor, qColor;
    char fcst = xf->fcst;
    char fbyte = xf->fbyte;
    char ocst = xf->ocst;
    float pop = 0, qop = 0;
    dependency cdep = xf->colors_dep;
    int start, last, knt, j;
    Line *l;

    if (ocst) {
	OPACITY(0, pop);
	OPACITY(0, qop);
    }

    if (!xf->fcolors)
	DXErrorReturn(ERROR_BAD_PARAMETER, "polylines must have front colors");

    if (fcst)
    {
	COLOR(0, pColor);
	qColor = pColor;
    }

    for (i=0; i<n; i++) {

	start = polylines[i];
	last  = (i == n-1) ? xf->nedges-1 : polylines[i+1]-1;
	knt   = last - start;

	if (ich && DXIsElementInvalid(ich, i))
	    continue;

	l = (Line *)(xf->edges + start);

	if (!ocst)
	{
	    if (cdep == dep_polylines)
	    {
		OPACITY(i, pop);
		qop = pop;
	    }
	    else if (cdep == dep_positions)
	    {
		OPACITY(l->p, qop);
	    }
	}

	if (!fcst)
	{
	    if (cdep == dep_polylines)
	    {
		COLOR(i, pColor);
		qColor = pColor;
	    }
	    else if (cdep == dep_positions)
	    {
		COLOR(l->p, qColor);
	    }
	}

	for (j = 0; j < knt; j++, l = (Line *)(((int *)l) + 1))
	{
	    if (!ocst && cdep == dep_positions)
	    {
		pop = qop;
		OPACITY(l->q, qop);
	    }

	    if (!fcst && cdep == dep_positions)
	    {
		pColor = qColor;
		COLOR(l->q, qColor);
	    }

	    if (! line(b, xf, *l, 1, pColor, qColor, pop, qop, clip))
		return ERROR;
	}

    }

    return OK;
}
