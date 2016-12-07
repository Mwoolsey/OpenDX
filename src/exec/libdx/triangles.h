/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#define CEIL(x) ((int)((x)-30000.0)+30000)

#ifdef PASS4
#define CAT(a,b) a##b
#endif

/* to avoid overflow on fp->int */
#define DXD_MAX_INT_2 (DXD_MAX_INT/2)

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
 * Note that this appears in line.c also.
 */
#define OPACITY_THRESHOLD 0.5

#ifdef PASS2

#define TRIANGLEX1(pst, pptr, shrgb, sho, col, face, TEST, PIXEL, LABEL, VER) {$\
									$\
    pst *pp, *p;							$\
    Pointer colors;							$\
    int cstcolors;							$\
    RGBColor cbuf1, cbuf2, cbuf3;					$\
    char cbyte;								$\
    char obyte = xf->obyte;						$\
									$\
    /* offsets */							$\
    x1+=ox, y1+=oy;							$\
    x2+=ox, y2+=oy;							$\
    x3+=ox, y3+=oy;							$\
									$\
    /* avoid overflow - XXX check performance impact */			$\
    if (x1<-DXD_MAX_INT_2 || x1>DXD_MAX_INT_2 || y1<-DXD_MAX_INT_2 || y1>DXD_MAX_INT_2 ||			$\
      x2<-DXD_MAX_INT_2 || x2>DXD_MAX_INT_2 || y2<-DXD_MAX_INT_2 || y2>DXD_MAX_INT_2 ||				$\
      x3<-DXD_MAX_INT_2 || x3>DXD_MAX_INT_2 || y3<-DXD_MAX_INT_2 || y3>DXD_MAX_INT_2)				$\
	DXErrorReturn(ERROR_BAD_PARAMETER,				$\
		"camera causes numerical overflow");			$\
									$\
    /* x delta setup; sign of d also indicates parity */		$\
    d1 = y3-y2,  d2 = y1-y3,  d3 = y2-y1;				$\
    d = x1*d1 + x2*d2 + x3*d3;						$\
    Qx = d? (float)1.0 / (float)d : 0.0;				$\
    d1 *= Qx,  d2 *= Qx,  d3 *= Qx;					$\
									$\
    /* colors */							$\
    if (col) {								$\
	if (d < 0)							$\
	    cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;	$\
	else								$\
	    cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;	$\
	if (!colors) goto CAT(LABEL,VER);				$\
    }									$\
    if (shrgb) {							$\
	if (shademe) {							$\
	    c1 = ((RGBColor *)colors) + i1;				$\
	    c2 = ((RGBColor *)colors) + i2;				$\
	    c3 = ((RGBColor *)colors) + i3;				$\
	} else {							$\
	    if (cmap)							$\
		c1= &MAPCOLS(v1), c2= &MAPCOLS(v2), c3= &MAPCOLS(v3);	$\
	    else {							$\
		COLS(v1, cbuf1); COLS(v2, cbuf2); COLS(v3, cbuf3);	$\
		c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;			$\
	    }								$\
	}								$\
	r1=c1->r, g1=c1->g, b1=c1->b;					$\
	r2=c2->r, g2=c2->g, b2=c2->b;					$\
	r3=c3->r, g3=c3->g, b3=c3->b;					$\
    } else if (col) {							$\
	if (cmap){							$\
	   r=MAPCOLS(index).r; g=MAPCOLS(index).g; b=MAPCOLS(index).b;	$\
	} else {							$\
	   COLS(index, cbuf1);						$\
	   r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;			$\
	}								$\
    }									$\
    if (sho) {								$\
	if (omap) o1=MAPOPS(v1), o2=MAPOPS(v2), o3=MAPOPS(v3);		$\
	else {								$\
	    OPS(v1, o1); OPS(v2, o2); OPS(v3, o3);			$\
	}								$\
    } else if (opacities||omap) {					$\
	if (omap) o=MAPOPS(index);				    	$\
	else      OPS(index, o);					$\
	obar = 1.0-o;							$\
    } else								$\
	o = 1.0, obar = 0.0;						$\
									$\
    /* x deltas */							$\
    if (shrgb) {							$\
	dxr = (r1 == r2 && r1 == r3) ? 0.0 : d1*r1 + d2*r2 + d3*r3;	$\
	dxg = (g1 == g2 && g1 == g3) ? 0.0 : d1*g1 + d2*g2 + d3*g3;	$\
	dxb = (b1 == b2 && b1 == b3) ? 0.0 : d1*b1 + d2*b2 + d3*b3;	$\
    }									$\
    if (sho) dxo = (o1 == o2 && o1 == o3) ? 0.0 : d1*o1 + d2*o2 + d3*o3;$\
    dxz = (z1 == z2 && z1 == z3) ? 0.0 : d1*z1 + d2*z2 + d3*z3;		$\
									$\
    /* sort pts so 1 <= 2 <= 3, so 1 to 3 is the long edge */		$\
    XCHNG(y1,y2,x1,x2,r1,r2,g1,g2,b1,b2,o1,o2,z1,z2,shrgb,sho)		$\
    XCHNG(y1,y3,x1,x3,r1,r3,g1,g3,b1,b3,o1,o3,z1,z3,shrgb,sho)		$\
    XCHNG(y2,y3,x2,x3,r2,r3,g2,g3,b2,b3,o2,o3,z2,z3,shrgb,sho)		$\
									$\
    /* object bounds for faces */					$\
    /* XXX - phooey, reusing A and B here to avoid compiler problems */	$\
    if (face) {								$\
	A = B = x1;							$\
	if (x2<A) A = x2; else B = x2;					$\
        if (x3<A) A = x3; else if (x3>B) B = x3;			$\
	if (A<buf->fmin.x) buf->fmin.x = A;				$\
	if (B>buf->fmax.x) buf->fmax.x = B;				$\
	if (y1<buf->fmin.y) buf->fmin.y = y1;				$\
	if (y3>buf->fmax.y) buf->fmax.y = y3;				$\
    }									$\
									$\
    /* round up to get iy1, iy2, iy3*/					$\
    iy1 = CEIL(y1), iy2 = CEIL(y2), iy3 = CEIL(y3);			$\
    if (iy1>iy2 || iy2>iy3) {						$\
	DXSetError(ERROR_DATA_INVALID,					$\
	    "position number %d, %d, or %d is invalid", v1, v2, v3);	$\
	return ERROR;							$\
    }									$\
    if (iy1==iy3)					/* TIME */	$\
	goto CAT(LABEL,VER);



#define TRIANGLEX2(pst, pptr, shrgb, sho, col, face, TEST, PIXEL, LABEL, VER)$\
    /* y deltas */							$\
    d = y3 - y1;							$\
    Qy = d? (float)1.0 / (float)d : 0.0;				$\
    if (shrgb) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;		$\
    if (sho) dyo = (float)(o3-o1) * Qy;					$\
    dyz = (z3-z1) * Qy;							$\
    dyA = (float)(x3-x1) * Qy; 						$\
									$\
    /* initial values */						$\
    A = x1;								$\
    pp = buf->pptr + iy1*width;					   	$\
									$\
    /* y fractional pixel adjustment and/or y clip */			$\
    d = iy1 - y1;							$\
    if (iy1<0)								$\
	d-=iy1, pp=buf->pptr;					   	$\
    A += dyA*d;								$\
    if (shrgb) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;				$\
    if (sho) o1+=dyo*d;							$\
    z1 += dyz*d;							$\
									$\
    /* do the two halves of the triangle */				$\
    nn = 0;								$\
    HALF(iy1, iy2, x1, x2, y1, y2, shrgb, sho, TEST, PIXEL)		$\
    HALF(iy2, iy3, x2, x3, y2, y3, shrgb, sho, TEST, PIXEL)		$\
    buf->pixels += nn;							$\
}

#define TRIANGLEV1(pst, pptr, shrgb, sho, col, face, TEST, PIXEL, LABEL, VER) {$\
									$\
    pst *pp, *p;							$\
    Pointer colors;							$\
    int cstcolors;							$\
    RGBColor cbuf1, cbuf2, cbuf3;					$\
    char cbyte;								$\
    char obyte = xf->obyte;						$\
									$\
    /* offsets */							$\
    x1+=ox, y1+=oy;							$\
    x2+=ox, y2+=oy;							$\
    x3+=ox, y3+=oy;							$\
									$\
    /* avoid overflow - XXX check performance impact */			$\
    if (x1<-DXD_MAX_INT_2 || x1>DXD_MAX_INT_2 || y1<-DXD_MAX_INT_2 || y1>DXD_MAX_INT_2 ||			$\
      x2<-DXD_MAX_INT_2 || x2>DXD_MAX_INT_2 || y2<-DXD_MAX_INT_2 || y2>DXD_MAX_INT_2 ||				$\
      x3<-DXD_MAX_INT_2 || x3>DXD_MAX_INT_2 || y3<-DXD_MAX_INT_2 || y3>DXD_MAX_INT_2)				$\
	DXErrorReturn(ERROR_BAD_PARAMETER,				$\
		"camera causes numerical overflow");			$\
									$\
    /* x delta setup; sign of d also indicates parity */		$\
    d1 = y3-y2,  d2 = y1-y3,  d3 = y2-y1;				$\
    d = x1*d1 + x2*d2 + x3*d3;						$\
    Qx = d? (float)1.0 / (float)d : 0.0;				$\
    d1 *= Qx,  d2 *= Qx,  d3 *= Qx;					$\
									$\
    /* colors ONLY if valid */						$\
    if (valid) {							$\
	if (col) {							$\
	    if (d < 0)							$\
		cbyte = fbyte, colors = fcolors, cstcolors = xf->fcst;	$\
	    else							$\
		cbyte = bbyte, colors = bcolors, cstcolors = xf->bcst;	$\
	    if (!colors) goto CAT(LABEL,VER);				$\
	}								$\
	if (shrgb) {							$\
	    if (cmap) c1= &MAPCOLS(v1), c2= &MAPCOLS(v2), c3= &MAPCOLS(v3);$\
	    else {							$\
		COLS(v1, cbuf1); COLS(v2, cbuf2); COLS(v3, cbuf3);	$\
		c1 = &cbuf1; c2 = &cbuf2; c3 = &cbuf3;			$\
	    }								$\
	    r1=c1->r, g1=c1->g, b1=c1->b;				$\
	    r2=c2->r, g2=c2->g, b2=c2->b;				$\
	    r3=c3->r, g3=c3->g, b3=c3->b;				$\
	} else if (col) {						$\
	    if (cmap) r=MAPCOLS(i).r, g=MAPCOLS(i).g, b=MAPCOLS(i).b;	$\
	    else {						 	$\
		COLS(i, cbuf1);						$\
		r = cbuf1.r; g = cbuf1.g; b = cbuf1.b;			$\
	    }								$\
	}								$\
	if (sho) {							$\
	    if (omap) o1=MAPOPS(v1), o2=MAPOPS(v2), o3=MAPOPS(v3);	$\
	    else {							$\
		OPS(v1, o1); OPS(v2, o2); OPS(v3, o3);			$\
	    }								$\
	} else if (opacities||omap) {					$\
	    if (omap) o=MAPOPS(i);					$\
	    else      OPS(i, o);					$\
	    obar = 1.0-o;						$\
	} else								$\
	    o = 1.0, obar = 0.0;					$\
									$\
	if (shrgb) dxr = d1*r1 + d2*r2 + d3*r3;				$\
	if (shrgb) dxg = d1*g1 + d2*g2 + d3*g3;				$\
	if (shrgb) dxb = d1*b1 + d2*b2 + d3*b3;				$\
	if (sho)   dxo = d1*o1 + d2*o2 + d3*o3;				$\
    } else								$\
	r1 = r2 = r3 = g1 = g2 = g3 = b1 = b2 = b3 = o1 = o2 = o3 = 0.0;$\
									$\
    /* x deltas */							$\
    dxz = d1*z1 + d2*z2 + d3*z3;					$\
									$\
    /* sort pts so 1 <= 2 <= 3, so 1 to 3 is the long edge */		$\
    XCHNG(y1,y2,x1,x2,r1,r2,g1,g2,b1,b2,o1,o2,z1,z2,shrgb,sho)		$\
    XCHNG(y1,y3,x1,x3,r1,r3,g1,g3,b1,b3,o1,o3,z1,z3,shrgb,sho)		$\
    XCHNG(y2,y3,x2,x3,r2,r3,g2,g3,b2,b3,o2,o3,z2,z3,shrgb,sho)		$\
									$\
    /* object bounds for faces */					$\
    /* XXX - phooey, reusing A and B here to avoid compiler problems */	$\
    if (face) {								$\
	A = B = x1;							$\
	if (x2<A) A = x2; else B = x2;					$\
        if (x3<A) A = x3; else if (x3>B) B = x3;			$\
	if (A<buf->fmin.x) buf->fmin.x = A;				$\
	if (B>buf->fmax.x) buf->fmax.x = B;				$\
	if (y1<buf->fmin.y) buf->fmin.y = y1;				$\
	if (y3>buf->fmax.y) buf->fmax.y = y3;				$\
    }									$\
									$\
    /* round up to get iy1, iy2, iy3*/					$\
    iy1 = CEIL(y1), iy2 = CEIL(y2), iy3 = CEIL(y3);			$\
    if (iy1>iy2 || iy2>iy3) {						$\
	DXSetError(ERROR_DATA_INVALID,					$\
	    "position number %d, %d, or %d is invalid", v1, v2, v3);	$\
	return ERROR;							$\
    }									$\
    if (iy1==iy3)				/* TIME */		$\
	goto CAT(LABEL,VER);						$\
									$\
    /* y deltas */							$\
    d = y3 - y1;							$\
    Qy = d? (float)1.0 / (float)d : 0.0;				$\
    if (valid) {							$\
	if (shrgb) dyr=(r3-r1)*Qy, dyg=(g3-g1)*Qy, dyb=(b3-b1)*Qy;	$\
	if (sho) dyo = (float)(o3-o1) * Qy;				$\
    }									$\
    dyz = (z3-z1) * Qy;							$\
    dyA = (float)(x3-x1) * Qy; 						$\
									$\
    /* initial values */						$\
    A = x1;								$\
    pp = buf->pptr + iy1*width;					    	$\
									$\
    /* y fractional pixel adjustment and/or y clip */			$\
    d = iy1 - y1;							$\
    if (iy1<0)								$\
	d-=iy1, pp=buf->pptr;					    	$\
    A += dyA*d;								    

#define TRIANGLEV2(pst, pptr, shrgb, sho, col, face, TEST, PIXEL, LABEL, VER)$\
									$\
    if (valid) {							$\
	if (shrgb) r1+=dyr*d, g1+=dyg*d, b1+=dyb*d;			$\
	if (sho) o1+=dyo*d;						$\
    }									$\
    z1 += dyz*d;							$\
									$\
    /* do the two halves of the triangle */				$\
    nn = 0;								$\
    if (valid) {							$\
	HALF(iy1, iy2, x1, x2, y1, y2, shrgb, sho, TEST, PIXEL) 	$\
	HALF(iy2, iy3, x2, x3, y2, y3, shrgb, sho, TEST, PIXEL)	    	$\
    } else {								$\
	HALF(iy1, iy2, x1, x2, y1, y2, 0, 0, TEST, PIXEL)		$\
	HALF(iy2, iy3, x2, x3, y2, y3, 0, 0, TEST, PIXEL)		$\
    }									$\
    buf->pixels += nn;							$\
}

#endif

#ifdef PASS3

#define X(a,b) (d=a, a=b, b=d)

#define XCHNG(y1,y2,x1,x2,r1,r2,g1,g2,b1,b2,o1,o2,z1,z2,shrgb,sho) {	$\
    if (y2 < y1) {							$\
	X(y1,y2), X(x1,x2);						$\
	if (shrgb) X(r1,r2), X(g1,g2), X(b1,b2);			$\
	if (sho) X(o1,o2);						$\
	X(z1,z2);							$\
    }									$\
}

/* direct or mapped colors */
#define COLS(i, dst)							$\
{									$\
    if (cbyte)								$\
    {									$\
	RGBByteColor *ptr=((RGBByteColor *)colors)+(cstcolors ? 0 : i);	$\
	dst.r = _dxd_ubyteToFloat[ptr->r];				$\
	dst.g = _dxd_ubyteToFloat[ptr->g];				$\
	dst.b = _dxd_ubyteToFloat[ptr->b];				$\
    } else								$\
	dst = (((RGBColor *)colors)[cstcolors ? 0 : i]);		$\
}

#define OPS(i, dst) 							$\
{									$\
    if (obyte)								$\
      dst = _dxd_ubyteToFloat[((ubyte *)opacities)[xf->ocst ? 0 : i]];  $\
    else								$\
      dst = (((float *)opacities)[xf->ocst ? 0 : i]);			$\
}

#define MAPCOLS(i) (cmap[((unsigned char *)colors)[cstcolors ? 0 : i]])
#define MAPOPS(i) (omap[((unsigned char *)opacities)[xf->ocst ? 0 : i]])


/*
 * The various per-pixel routines.  We start with the opaque
 * cases: opaque triangles, and triangles that are part of
 * the faces/loops/edges stuff.  We define two comparisons,
 * for case of render-time CSG (CLIPZ) or no render-time CSG
 * (NOCLIPZ).  We also define a comparison for interference checking
 * (IFZ) that also flips the IF_OBJECT bit as appropriate.
 */

/*
 * The IFZ macro here is a conditional containing an outer condition
 * controlling an XOR assignment, followed by a continuation of the condition.
 * To get around a preprocessor limitation (commas in macro arguments) we
 * (Nancy and Greg) changed the inner XOR assignment to an additional	
 * term in the condition that always evaluates TRUE.
 */

#define CLIPZ   (z>p->z && z<p->front && z>p->back)
#define NOCLIPZ (z>p->z)
#define IFZ     ((z<p->front)             &&	\
		 (1 | (p->in^=IF_OBJECT)) &&	\
		 (z>p->z)                 &&	\
		 (z>p->back))

#define OPAQUE {				\
	p->c.r=r; p->c.g=g; p->c.b=b; p->z=z;   \
}

#define FACE {					\
    /* reuse co.o for z here; XXX-use union? */	\
    p->co.r=r; p->co.g=g; p->co.b=b; p->co.o=z;	\
    p->in ^= IN_FACE;				\
}


/*
 * Clipping triangles just affect the front/back buffers
 */

#define CLIPPINGPIXEL {							\
    if (z > p->front)	/* find frontmost surface */			\
	p->front = z;	/* XXX - could use orientation of face */	\
    if (z < p->back)	/* find backmost surface */			\
	p->back = z;							\
}


/*
 * For translucent triangles, we can assume that ->back has
 * already been merged into ->z; furthermore, we reuse ->back
 * in volume rendering, so it is incorrect to compare against
 * ->back here because we may be mixed in with volume faces
 * This also occurs in line.c.
 */

#define TRANSCLIPZ (z>p->z && z<p->front)

#define TRANSLUCENT {							\
	p->c.r=o*r+obar*p->c.r;						\
	p->c.g=o*g+obar*p->c.g;						\
	p->c.b=o*b+obar*p->c.b;						\
	if (o > OPACITY_THRESHOLD)					\
	    p->z=z;							\
}

#define COMPOSITE {							\
	p->c.r=r+obar*p->c.r;						\
	p->c.g=g+obar*p->c.g;						\
	p->c.b=b+obar*p->c.b;						\
}


/*
 * We use ->back instead of ->z to keep track of the distance to
 * the last volume face rendered; this allows us to leave ->z alone,
 * which makes it possible to mix translucent surfaces clipped by
 * ->z with volumes.  In order for this to work, we have to have
 * merged ->back into ->z before doing translucent surfaces and
 * volumes.
 */

#define VOLUME {							\
    if (p->in<=0) {							\
	if (z < p->front) {						\
	    if (surface) {						\
		p->in++;						\
		if (z > p->back)					\
		    p->back = z;					\
		if (valid && !xf->tile.flat_z)				\
		    { p->co.r=r; p->co.g=g; p->co.b=b; }		\
	    }								\
	}								\
    } else {								\
	if (surface)							\
	    p->in = 0;							\
	if (z >= p->front) {						\
	    p->in = -DXD_MAX_INT;					\
	    z = p->front;						\
	}								\
	if (z > p->back) {						\
	    float ar; float ag; float ab; float ao;			\
	    if (valid) { /* don't affect color if not valid, just Z */	\
		if (/*xf->tile.flat_z*/ 1) {				\
		    d = (z - p->back) * cmul;				\
		    ar = d * r;						\
		    ag = d * g;						\
		    ab = d * b;						\
		    ao = d * o * omul;					\
		} else {						\
		    d = 0.5 * (z - p->back) * cmul;			\
		    ar = d * (p->co.r+r);				\
		    ag = d * (p->co.g+g);				\
		    ab = d * (p->co.b+b);				\
		    ao = d * (p->co.o+r) * omul;			\
		    p->co.r = r;					\
		    p->co.g = g;					\
		    p->co.b = b;					\
		    p->co.o = o;					\
		}							\
		if (/*xf->tile.fast_exp*/ 1) {				\
		    if ((obar=1-ao) < 0.0)				\
			obar = 0.0;					\
		} else {						\
		    if ((obar=1-0.125*ao) < 0.0)			\
			obar = 0.0;					\
		    obar = obar*obar; obar = obar*obar; obar = obar*obar;\
		}							\
		p->c.r = p->c.r * obar + ar;				\
		p->c.g = p->c.g * obar + ag;				\
		p->c.b = p->c.b * obar + ab;				\
	    }								\
	    p->back = z;						\
	}								\
    }									\
}


/*
 * shade/don't shade colors/opacities
 * colored/uncolored surfaces
 */

#define NOSHRGB 0
#define SHRGB 1

#define NOSHO 0
#define SHO 1

#define UNCOL 0
#define COL 1


#define CEIL(x) ((int)((x)-30000.0)+30000)

#define HALF(iy1, iy2, x1, x2, y1, y2, shrgb, sho, TEST, PIXEL) {	$\
									$\
    if (iy2>height)							$\
	iy2 = height;							$\
    if (iy1<iy2 && iy2>0 && iy1<=height) {				$\
	B = x1;								$\
	d = y2 - y1;							$\
	d = d? (float)1.0 / (float)d : 0.0;				$\
	dyB = (float)(x2-x1) * d;					$\
	if (iy1<0) iy1=0;						$\
	d = iy1 - y1;							$\
	B += d*dyB;							$\
	for (iy=iy1; iy<iy2; iy++) {					$\
	    iA = CEIL(A);						$\
	    iB = CEIL(B);						$\
	    if (iB>iA) left=iA, right=iB;				$\
	    else left=iB, right=iA;					$\
	    if (left<0) left=0;						$\
	    if (right>width) right=width;				$\
	    n = right - left;						$\
	    if (n>0) {							$\
		nn += n;						$\
		p = pp+left;						$\
		d = left - A;						$\
		if (shrgb) r=r1+d*dxr, g=g1+d*dxg, b=b1+d*dxb;		$\
		if (sho) o=o1+d*dxo, obar=1.0-o;			$\
		z=z1+d*dxz;						$\
		while (--n>=0) {					$\
		    if (TEST) {						$\
			PIXEL;						$\
			if (shrgb) r+=dxr, g+=dxg, b+=dxb;		$\
			if (sho) o+=dxo, obar=1.0-o;			$\
			z+=dxz;						$\
			p++;						$\
		    } else {						$\
			if (shrgb) r+=dxr, g+=dxg, b+=dxb;		$\
			if (sho) o+=dxo, obar=1.0-o;			$\
			z+=dxz;						$\
			p++;						$\
		    }							$\
		}							$\
	    }								$\
	    if (shrgb) r1+=dyr, g1+=dyg, b1+=dyb;			$\
	    if (sho) o1+=dyo;						$\
	    z1 += dyz;							$\
	    A+=dyA, B+=dyB;						$\
	    pp+=width;							$\
	}								$\
    }									$\
}

#endif
