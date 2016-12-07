/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999,2002                              */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * Header: /usr/people/gresh/code/svs/src/libdx/RCS/displayx.c,v 5.2 93/01/25 17:06:43 gda Exp Locker: gresh 
 * 
 */

#include <dxconfig.h>
#include <dx/dx.h>

#define FORCE_VISUAL 

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define Screen XScreen
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef Screen

#include "internals.h"
#include "render.h"
#include "displayx.h"
#include "displayutil.h"

typedef Error (*Handler)(int, Pointer);
typedef int   (*Checker)(int, Pointer);
extern Error DXRegisterWindowHandlerWithCheckProc(Handler proc, Checker check, Display *d, Pointer arg);

static int xerror = 0;

static int error_handler(Display *dpy, XErrorEvent *event)
{
    char buffer[100];
    DXDebug("X", "error handler");
    XGetErrorText(dpy, event->error_code, buffer, sizeof(buffer));
    DXSetError(ERROR_INTERNAL, "#11690", buffer);
    xerror = 1;
    return xerror;
}

Colormap
getWindowColormap(Display *dpy, Window w)
{
    XWindowAttributes xwa;

    XGetWindowAttributes(dpy, w, &xwa);

    return xwa.colormap;
}

/*
 * The following code is used to debug the color allocation
 * algorithm in getOneMapTranslation and is generally replaced
 * by the following #defines
 */
#if 0

static int allocated[256];
static int alloc_first = 1;

static Error
XALLOCCOLOR(Display *d, Colormap map, XColor *c)
{
    if (alloc_first)
    {
	memset(allocated, 0, sizeof(allocated));
	alloc_first = 0;
    }

    if (! XAllocColor(d, map, c))
	return ERROR;

    allocated[c->pixel] ++;

    return OK;
}

static Error
XFREECOLOR(Display *d, Colormap map, long c)
{
    if (alloc_first)
    {
	DXSetError(ERROR_INTERNAL, "freed color before allocation");
	return ERROR;
    }

    if (allocated[c] <= 0)
    {
	DXSetError(ERROR_INTERNAL, "freed unallocated color");
	return ERROR;
    }

    XFreeColors(d, map, (unsigned long *)&c, 1, 0);
    allocated[c] --;

    return OK;
}

#else

#define XALLOCCOLOR(d, m, c) XAllocColor(d, m, c)
#define XFREECOLOR(d, m, c)  XFreeColors(d, m, &c, 1, 0)

#endif



/*
 * Dither options: dither in color space or no dither
 */
#define DITHER								\
{									\
    d = m[(x+left) &(DX-1)]; 						\
    ir = DITH(rr,ir,d);							\
    ig = DITH(gg,ig,d);							\
    ib = DITH(bb,ib,d);							\
}

#define NO_DITHER

/*
 * translation: three types: translate RGB separately in three
 * tables prior to creating a combined pixel, translate the combined
 * pixel, and no translation.
 */
#define THREE_XLATE 							\
{									\
    ir = rtable[ir];							\
    ig = gtable[ig];							\
    ib = btable[ib];							\
}

#define ONE_XLATE							\
{									\
    i = table[i];							\
}

#define NO_XLATE

/*
 * color mapping: create color from index
 */
#define COLORMAP							\
{									\
    ir = colormap[i].r;							\
    ig = colormap[i].g;							\
    ib = colormap[i].b;							\
}

#define NO_COLORMAP
	
/*
 * Extract data: float or byte, index or vector, pixel from renderer
 */
#define FLOAT_VECTOR_TO_INDEX_VECTOR 					\
{									\
    float *_ptr = (float *)_src;					\
    ir = (int)(255 * _ptr[0]);						\
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;				\
    ig = (int)(255 * _ptr[1]);						\
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;				\
    ib = (int)(255 * _ptr[2]);						\
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;				\
}

#define BYTE_VECTOR_TO_INDEX_VECTOR	 				\
{									\
    ir = ((unsigned char *)_src)[0];					\
    ig = ((unsigned char *)_src)[1];					\
    ib = ((unsigned char *)_src)[2];					\
}

#define BYTE_SCALAR_TO_INDEX_VECTOR					\
    ir = ig = ib = *(unsigned char *)_src;				

#define BYTE_SCALAR_TO_INDEX     					\
    i = *(unsigned char *)_src;	

#define BIG_PIX_TO_INDEX_VECTOR						\
{									\
    struct big *_ptr = (struct big *)_src;				\
    ir = (int)(255 * _ptr->c.r);					\
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;				\
    ig = (int)(255 * _ptr->c.g);					\
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;				\
    ib = (int)(255 * _ptr->c.b);					\
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;				\
}

#define FAST_PIX_TO_INDEX_VECTOR					\
{									\
    struct fast *_ptr = (struct fast *)_src;				\
    ir = (int)(255 * _ptr->c.r);					\
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;				\
    ig = (int)(255 * _ptr->c.g);					\
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;				\
    ib = (int)(255 * _ptr->c.b);					\
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;				\
}

/*
 * form pixel from rgb: either 5/9/5 for 8-bit shared, 
 * 12/12/12 for 12-bit shared, 256/256/256 for true or
 * direct color, or no pixel formation for 8 or 12 bit
 * private.
 */

#define PIXEL_SUM							\
    i = (ir) + (ig) + (ib)

#define PIXEL_1								\
    i = (((((ir)*gg)+(ig))*bb)+(ib))

#define PIXEL_3								\
    i = ir | ig | ib

#define DIRECT_PIXEL_1

#define DIRECT_PIXEL_3							\
    i = (i << 16) | (i << 8) | i

/* 
 * Gamma: yes or no, applied in color space
 */
#define NO_GAMMA

#define GAMMA								\
{									\
    ir = gamma[ir];							\
    ig = gamma[ig];							\
    ib = gamma[ib];							\
}

/*
 * store the result.  Type dependent on output depth
 */
#define STORE_INT8							\
    *(unsigned char *)t_dst = i;					

#define STORE_INT16							\
    *(short *)t_dst = i;					

#define STORE_INT24							\
    ((unsigned char *)t_dst)[0] = i & 0xff;				\
    ((unsigned char *)t_dst)[1] = (i >> 8) & 0xff;			\
    ((unsigned char *)t_dst)[2] = (i >> 16) & 0xff;					
#define STORE_INT32							\
    *(int32 *)t_dst = i;					


/*
 * Options on loop:
 *     extract: 
 *	 Extraction methods produce pixel values in the 0-255 range
 *		BIG_PIX_TO_INDEX_VECTOR
 *		FAST_PIX_TO_INDEX_VECTOR
 *		FLOAT_VECTOR_TO_INDEX_VECTOR
 *		BYTE_VECTOR_TO_INDEX_VECTOR
 *		BYTE_SCALAR_TO_INDEX_VECTOR
 *		BYTE_SCALAR_TO_INDEX
 *     gamma:
 *	 Gamma lookups are in 0-255 range
 *		NO_GAMMA
 * 		GAMMA
 *
 *     dither: 
 *	 Dither methods inputs and outputs are both in the 0-255 range
 *		NO_DITHER
 *		THREE_DITHER
 * 		ONE_DITHER
 *
 *     colormap:
 *	 Colormap method inputs and outputs are both in the 0-255 range
 *		NO_COLORMAP
 *		COLORMAP
 *
 *     translate:
 *       Translation inputs and outputs are in 0-255 range
 *		NO_XLATE
 *		THREE_XLATE	(used before forming pixel)
 *		ONE_XLATE	(used after forming pixel)
 *
 *     pixel:
 *	 pixel inputs are 0-255, outputs are display dependent
 *		PIXEL_1
 *		PIXEL_3
 *		DIRECT_PIXEL_1
 *		DIRECT_PIXEL_3
 *
 *     result:
 *       input is a pixel, output is display dependent
 *              STORE_INT8
 *              STORE_INT16
 *              STORE_INT24
 *              STORE_INT32
 */

#define LOOP(isz, osz, D, ext, cmap, gam, dith, x1, pix, x2, res)	\
{ 									\
    unsigned char *_dst = dst + (dst_imp_offset + dst_exp_offset)*osz; 	\
    unsigned char *_src = src + (src_imp_offset + src_exp_offset)*isz;	\
    unsigned char *t_dst; 						\
    int _d0 = d0*osz, _d1 = d1*osz;					\
    int i;								\
									\
    for (y=0;  y<height;  y++)						\
    { 									\
	if (D)								\
	    m = &(_dxd_dither_matrix[(oy-(p+y))&(DY-1)][0]); 		\
									\
	t_dst = _dst;							\
	for (x = 0; x < width; x++)					\
	{								\
	    ext;							\
	    cmap;							\
	    gam;							\
	    dith;							\
	    x1;								\
	    pix;							\
	    x2;								\
	    res;							\
									\
	    t_dst += _d1;						\
	    _src += isz;						\
	}								\
									\
	_dst += _d0;							\
    }									\
}

#define SGN(x) ((x)>0? 1 : (x)<0? -1 : 0)

#define OUT_INT8	1
#define OUT_INT16	2
#define OUT_INT24	4
#define OUT_INT32	3

int
_dxf_GetXBytesPerPixel(translationP translation)
{
  return translation->bytesPerPixel;
}

Error
_dxf_translateXImage(Object image, /* object defining overall pixel grid          */
	int iwidth, int iheight,  /* composite width and height                  */
	int offx, int offy,	  /* origin of composite image in pixel grid     */
	int px, int py,		  /* explicit partition origin                   */
	int cx, int cy,		  /* explicit partition width and height         */
	int slab, int nslabs,	  /* implicit partitioning slab number and count */
	translationP translation, 
	unsigned char *src, int inType, unsigned char *dst)
{
    unsigned long *table;
    int width, height;
    int p, x, y, d, i;
    short *m;
    struct { float x, y; } deltas[2];
    int d0, d1, dims[2], ox, oy;
    Array a, cmap;
    /*Visual			*vis;*/
    unsigned long		*rtable,*gtable,*btable;
    int				rr,gg,bb;
    int				left=0;
    int				outType;
    int				*gamma;
    int				src_imp_offset, src_exp_offset;
    int				dst_imp_offset, dst_exp_offset;
    int				ir, ig, ib;

    if (!translation)
	DXErrorReturn(ERROR_INTERNAL, "#11610");

    /*vis = (Visual *)translation->visual;*/

    rr = translation->redRange;
    gg = translation->greenRange;
    bb = translation->blueRange;

    gamma = translation->gammaTable;

    /*
     * determine orientation of image
     */
    a = (Array) DXGetComponentValue((Field)image, POSITIONS);
    DXQueryGridPositions(a, NULL, dims, NULL, (float *)deltas);

    deltas[0].x = SGN(deltas[0].x);
    deltas[0].y = SGN(deltas[0].y);
    deltas[1].x = SGN(deltas[1].x);
    deltas[1].y = SGN(deltas[1].y);

    height = cx;
    width  = cy;

    /*
     * The origin of this partition in output is the offset of
     * the partition in the composite image minus the origin of
     * the composite image in the output.
     */
    ox = (px*deltas[0].x + py*deltas[1].x) - offx;
    oy = (px*deltas[0].y + py*deltas[1].y) - offy;

    /*
     * Get the strides in the output.  
     */
    d0 = 1*deltas[0].x + iwidth*deltas[0].y;
    d1 = 1*deltas[1].x + iwidth*deltas[1].y;

    /*
     * if we need to invert the image, we need to reposition the
     * origin in the output image and negate the per-row skip.
     */
    if (translation->invertY)
    {
	oy = (iheight-1) - oy;
	if (d0 == 1 || d0 == -1) d1 = -d1;
	else                     d0 = -d0;
    }

    /*
     * offsets for implicit partitioning
     */
    if (nslabs > 1)
    {
	p = slab*height/nslabs;

	height = (slab+1)*height/nslabs - p;

	src_imp_offset = p*width;
	dst_imp_offset = p*d0;
    }
    else
    {
	src_imp_offset = dst_imp_offset = 0;
	p = 0;
    }

    /*
     * offsets for explicit partitioning
     */
    src_exp_offset = 0;
    dst_exp_offset = oy*iwidth + ox;

    left = ox;

    if (_dxf_GetXBytesPerPixel(translation) == 4)
	outType = OUT_INT32;
    else if (_dxf_GetXBytesPerPixel(translation) == 3)
	outType = OUT_INT24;
    else if (_dxf_GetXBytesPerPixel(translation) == 2)
	outType = OUT_INT16;
    else
	outType = OUT_INT8;

	
    if (inType == IN_BYTE_SCALAR)
    {
        /*
         * if NoTranslation, colormap will be installed.  
         */
        if (translation->translationType == NoTranslation)
        {
	    if (outType == OUT_INT8)
		LOOP(sizeof(byte), 1, 0, BYTE_SCALAR_TO_INDEX, 
		    NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE,
		    DIRECT_PIXEL_1, NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(byte), 2, 0, BYTE_SCALAR_TO_INDEX,
		    NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE,
		    DIRECT_PIXEL_1, NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(byte), 3, 0, BYTE_SCALAR_TO_INDEX,
		    NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE,
		    DIRECT_PIXEL_1, NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(byte), 4, 0, BYTE_SCALAR_TO_INDEX,
		    NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE,
		    DIRECT_PIXEL_1, NO_XLATE, STORE_INT32);
        }
        else
        {
	    /*
	     * Otherwise, it must be applied on a pixel by pixel basis
	     */
	    int n, gray;
	    float *fmap;
	    struct { int r, g, b; } colormap[256];
    
	    cmap = (Array) DXGetComponentValue((Field)image, "color map");
	    if (! cmap)
	    {
	        DXSetError(ERROR_INTERNAL, "colormap required for byte images");
	        goto error;
	    }
    
	    fmap = (float *)DXGetArrayData(cmap);
	    if (!fmap)
	        goto error;
    
	    DXGetArrayInfo(cmap, &n, NULL, NULL, NULL, &gray);
    
	    if (gray == 1)
	        for (i=0; i<256; i++)
	        {
		    colormap[i].r = *fmap++ * 255.0;
		    colormap[i].g = colormap[i].b = colormap[i].r;
	        }
	    else
	        for (i=0; i<256; i++)
	        {
		    colormap[i].r = *fmap++ * 255.0;
		    colormap[i].g = *fmap++ * 255.0;
		    colormap[i].b = *fmap++ * 255.0;
	        }
    
	    if (translation->translationType == OneMap) 
	    {
	        table = translation->table;
	        rr    = translation->redRange;
	        gg    = translation->greenRange;
	        bb    = translation->blueRange;
      
		if (outType == OUT_INT8)
		    LOOP(sizeof(byte), 1, 1, BYTE_SCALAR_TO_INDEX,
		      COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		      ONE_XLATE, STORE_INT8)
		else if (1 == OUT_INT16)
		    LOOP(sizeof(byte), 2, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		      ONE_XLATE, STORE_INT16)
		else if (1 == OUT_INT24)
		    LOOP(sizeof(byte), 3, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		      ONE_XLATE, STORE_INT24)
		else
		    LOOP(sizeof(byte), 4, 1, BYTE_SCALAR_TO_INDEX,
		      COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		      ONE_XLATE, STORE_INT32);
	    }
	    else if (translation->translationType == GrayStatic)
	    {
		rtable = translation->rtable;
		gtable = translation->gtable;
		btable = translation->btable;

		if (outType == OUT_INT8)
		    LOOP(sizeof(byte), 1, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT8)
		else if (outType == OUT_INT16)
		    LOOP(sizeof(byte), 2, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT16)
		else if (outType == OUT_INT24)
		    LOOP(sizeof(byte), 3, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT24)
		else
		    LOOP(sizeof(byte), 4, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT32);
	    }
	    else if (translation->translationType == GrayMapped)
	    {
		rtable = translation->rtable;
		gtable = translation->gtable;
		btable = translation->btable;

		table = translation->table;
		

		if (outType == OUT_INT8)
		    LOOP(sizeof(byte), 1, 0, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, ONE_XLATE, STORE_INT8)
		else if (outType == OUT_INT16)
		    LOOP(sizeof(byte), 2, 0, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, ONE_XLATE, STORE_INT16)
		else if (outType == OUT_INT24)
		    LOOP(sizeof(byte), 3, 0, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, ONE_XLATE, STORE_INT24)
		else
		    LOOP(sizeof(byte), 4, 0, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, 
		      PIXEL_SUM, ONE_XLATE, STORE_INT32);
	    }
	    else if (translation->translationType == ColorStatic)
	    {
		rtable = translation->rtable;
		gtable = translation->gtable;
		btable = translation->btable;
		rr     = translation->redRange;
		gg     = translation->greenRange;
		bb     = translation->blueRange;

		if (outType == OUT_INT8)
		    LOOP(sizeof(byte), 1, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT8)
		else if (outType == OUT_INT16)
		    LOOP(sizeof(byte), 2, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT16)
		else if (outType == OUT_INT24)
		    LOOP(sizeof(byte), 3, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT24)
		else
		    LOOP(sizeof(byte), 4, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_SUM, NO_XLATE, STORE_INT32);
	    }
	    else if (translation->translationType == ThreeMap) 
	    {
	        rtable = translation->rtable;
	        gtable = translation->gtable;
	        btable = translation->btable;
    
		if (outType == OUT_INT8)
		    LOOP(sizeof(byte), 1, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_3, NO_XLATE, STORE_INT8)
		else if (outType == OUT_INT16)
		    LOOP(sizeof(byte), 2, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_3, NO_XLATE, STORE_INT16)
		else if (outType == OUT_INT24)
		    LOOP(sizeof(byte), 3, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_3, NO_XLATE, STORE_INT24)
		else
		    LOOP(sizeof(byte), 4, 1, BYTE_SCALAR_TO_INDEX, 
		      COLORMAP, GAMMA, DITHER, THREE_XLATE, 
		      PIXEL_3, NO_XLATE, STORE_INT32);
	    }
        }
    }
    else if (inType == IN_FLOAT_VECTOR)
    {
	if (translation->translationType == NoTranslation)
	{
	    if (outType == OUT_INT8)
		LOOP(3*sizeof(float), 1, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(float), 2, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(float), 3, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(float), 4, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;
	    
	    if (outType == OUT_INT8)
		LOOP(3*sizeof(float), 1, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(float), 2, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(float), 3, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(float), 4, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayMapped)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    table = translation->table;
	    
	    if (outType == OUT_INT8)
		LOOP(3*sizeof(float), 1, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(float), 2, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(float), 3, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(float), 4, 0, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT32);
	}
	else if (translation->translationType == ColorStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;
	    rr     = translation->redRange;
	    gg     = translation->greenRange;
	    bb     = translation->blueRange;

	    if (outType == OUT_INT8)
		LOOP(3*sizeof(float), 1, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(float), 2, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(float), 3, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(float), 4, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == OneMap)
	{
	    table = translation->table;
	    rr    = translation->redRange;
	    gg    = translation->greenRange;
	    bb    = translation->blueRange;

	    if (outType == OUT_INT8)
		LOOP(3*sizeof(float), 1, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(float), 2, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(float), 3, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(float), 4, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT32);
	}
	else
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(3*sizeof(float), 1, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(float), 2, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(float), 3, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(float), 4, 1, FLOAT_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT32);
	}
    }
    else if (inType == IN_BIG_PIX)
    {
	if (translation->translationType == NoTranslation)
	{
	    if (outType == OUT_INT8)
		LOOP(sizeof(struct big), 1, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct big), 2, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct big), 3, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct big), 4, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(sizeof(struct big), 1, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct big), 2, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct big), 3, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct big), 4, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayMapped)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    table = translation->table;
	    
	    if (outType == OUT_INT8)
		LOOP(sizeof(struct big), 1, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct big), 2, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct big), 3, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct big), 4, 0, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT32);
	}
	else if (translation->translationType == ColorStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;
	    rr     = translation->redRange;
	    gg     = translation->greenRange;
	    bb     = translation->blueRange;
	    
	    if (outType == OUT_INT8)
		LOOP(sizeof(struct big), 1, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct big), 2, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct big), 3, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct big), 4, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == OneMap)
	{
	    table = translation->table;
	    rr    = translation->redRange;
	    gg    = translation->greenRange;
	    bb    = translation->blueRange;

	    if (outType == OUT_INT8)
		LOOP(sizeof(struct big), 1, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct big), 2, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct big), 3, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct big), 4, 1, BIG_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		  ONE_XLATE, STORE_INT32);
	}
	else
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(sizeof(struct big), 1, 1, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct big), 2, 1, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct big), 3, 1, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct big), 4, 1, BIG_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT32);
	}
    }
    else if (inType == IN_FAST_PIX)
    {
	if (translation->translationType == NoTranslation)
	{
	    if (outType == OUT_INT8)
		LOOP(sizeof(struct fast), 1, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct fast), 2, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct fast), 3, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct fast), 4, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(sizeof(struct fast), 1, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct fast), 2, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct fast), 3, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct fast), 4, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayMapped)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    table = translation->table;
	    
	    if (outType == OUT_INT8)
		LOOP(sizeof(struct fast), 1, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct fast), 2, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct fast), 3, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct fast), 4, 0, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT32);
	}
	else if (translation->translationType == ColorStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;
	    rr     = translation->redRange;
	    gg     = translation->greenRange;
	    bb     = translation->blueRange;
	    
	    if (outType == OUT_INT8)
		LOOP(sizeof(struct fast), 1, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct fast), 2, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct fast), 3, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct fast), 4, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == OneMap)
	{
	    table = translation->table;
	    rr    = translation->redRange;
	    gg    = translation->greenRange;
	    bb    = translation->blueRange;

	    if (outType == OUT_INT8)
		LOOP(sizeof(struct fast), 1, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct fast), 2, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct fast), 3, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct fast), 4, 1, FAST_PIX_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		  ONE_XLATE, STORE_INT32);
	}
	else
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(sizeof(struct fast), 1, 1, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(sizeof(struct fast), 2, 1, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(sizeof(struct fast), 3, 1, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(sizeof(struct fast), 4, 1, FAST_PIX_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT32);
	}
    }
    else if (inType == IN_BYTE_VECTOR)
    {
	if (translation->translationType == NoTranslation)
	{
	    if (outType == OUT_INT8)
		LOOP(3*sizeof(byte), 1, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(byte), 2, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(byte), 3, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(byte), 4, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, NO_GAMMA, NO_DITHER, NO_XLATE, PIXEL_3,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(3*sizeof(byte), 1, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(byte), 2, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(byte), 3, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(byte), 4, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == GrayMapped)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    table = translation->table;
	    
	    if (outType == OUT_INT8)
		LOOP(3*sizeof(byte), 1, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(byte), 2, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(byte), 3, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(byte), 4, 0, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, NO_DITHER, THREE_XLATE, PIXEL_SUM,
		  ONE_XLATE, STORE_INT32);
	}
	else if (translation->translationType == ColorStatic)
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;
	    rr     = translation->redRange;
	    gg     = translation->greenRange;
	    bb     = translation->blueRange;
	    
	    if (outType == OUT_INT8)
		LOOP(3*sizeof(byte), 1, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(byte), 2, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(byte), 3, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(byte), 4, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_SUM, 
		  NO_XLATE, STORE_INT32);
	}
	else if (translation->translationType == OneMap)
	{
	    table = translation->table;
	    rr    = translation->redRange;
	    gg    = translation->greenRange;
	    bb    = translation->blueRange;

	    if (outType == OUT_INT8)
		LOOP(3*sizeof(byte), 1, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(byte), 2, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(byte), 3, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1, 
		  ONE_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(byte), 4, 1, BYTE_VECTOR_TO_INDEX_VECTOR, 
		  NO_COLORMAP, NO_GAMMA, DITHER, NO_XLATE, PIXEL_1,
		  ONE_XLATE, STORE_INT32);
	}
	else
	{
	    rtable = translation->rtable;
	    gtable = translation->gtable;
	    btable = translation->btable;

	    if (outType == OUT_INT8)
		LOOP(3*sizeof(byte), 1, 1, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT8)
	    else if (outType == OUT_INT16)
		LOOP(3*sizeof(byte), 2, 1, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT16)
	    else if (outType == OUT_INT24)
		LOOP(3*sizeof(byte), 3, 1, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT24)
	    else
		LOOP(3*sizeof(byte), 4, 1, BYTE_VECTOR_TO_INDEX_VECTOR,
		  NO_COLORMAP, GAMMA, DITHER, THREE_XLATE, PIXEL_3, 
		  NO_XLATE, STORE_INT32);
	}
    }

    return OK;

error:
    return ERROR;
}


/*
 * This is the cache entry for a window.  It is maintained by
 * the routines that follow.
 */

/* #X added */
#define WINDOW_UI       1
#define WINDOW_EXTERNAL 2
#define WINDOW_LOCAL    3
/* #X end */

struct window {
    Display *dpy;		/* our own private display connection */
    int fd;			/* fd for that connections */
    GC gc;			/* graphics context */
    unsigned char *pixels;	/* the converted pixels */
    Object pixels_owner;	/* owner of the pixels, if not us */
    XImage *ximage;		/* X image to hold converted pixels */
    int winFlag;		/* is a X window associated with the struct? */
    Window wid;			/* window id */
    /* #X 'ui' became 'wndowMode' */
    int windowMode;             /* is window owned by UI, another external source, or local? */
    int refresh;		/* set to cause refresh of window */
    int wwidth, wheight, wdepth;/* current window size */
    int pwidth, pheight;	/* current image size */
    Atom pixmap_atom;		/* pixmap for ui */
    Atom flag_atom;		/* flag for ui */
    int waitPixmap;		/* whether to wait for pixmap */
    int waitEvent;		/* whether to wait for particular event */
    int pixmapFull;		/* whether the pixmap is full or available for new image */
    int error;			/* whether X error occurred */
    Pixmap pixmap;		/* pixmap for ui */
    char *title;		/* window title */
    translationP translation;	/* color table translation info */
    Object translation_owner;	/* object that keeps the translation */
    int colormapTag;		/* tag of user-defined cmap (if so) */
    char *cacheid;		/* cache id */
    Private dpy_object;		/* object handle for display */
    int	objectTag;		/* tag of object currently in pixmap */
    int win_invalid;		/* has the window been destroyed? */
    Handler handler;		/* which input handler to use */
    int active;			/* whether the window is currently active */
    int cmap_installed;		/* is the colormap installed? */
};

struct display {
    char *cacheid;
    Display *dpy;
};


static Object createTranslation (Private dpy, char *where,
			int desiredDepth, int *directMap, struct window *w);

static void
free_ximage(struct window *w)
{
    /* ximage data structure */
    if (w->ximage) {
	w->ximage->data = NULL;
	XDestroyImage(w->ximage);
	w->ximage = NULL;
    }

    /* pixels */
    if (w->pixels_owner) {
	DXDelete(w->pixels_owner);
	w->pixels_owner = NULL;
	w->pixels = NULL;
    } else {
	DXFree((Pointer)w->pixels);
	w->pixels = NULL;
	w->pwidth = w->pheight = 0;
    }
}

static Error
reset_window(struct window *w)
{
    if (w->pixmap)
    {
	XFreePixmap(w->dpy, w->pixmap);
	w->pixmap = None;
    }

    if (w->ximage)
	free_ximage(w);
    
    if (w->cmap_installed)
    {
	XUninstallColormap(w->dpy, (Colormap)w->translation->cmap);
	w->cmap_installed = 0;
    }

    if (w->winFlag && w->windowMode == WINDOW_LOCAL)
    {
	XDestroyWindow(w->dpy, w->wid);
	w->wid = 0;
	w->winFlag = 0;
    }
	
    if (w->gc)
    {
	XFreeGC(w->dpy, w->gc);
	w->gc = 0;
    }

    if (w->translation_owner)
	DXDelete(w->translation_owner);
    w->translation_owner = NULL;
    w->translation = NULL;

    if (w->dpy_object)
	XSync(w->dpy, 0);

    if (xerror) goto error;
    return OK;

error:    
    xerror = 0;
    return ERROR;
}

static Error
delete_window(Pointer p)
{
    struct window *w = (struct window *)p;

    DXDebug("X", "closing window connection, fd %d", w->fd);
    DXRegisterWindowHandlerWithCheckProc(NULL, NULL, w->dpy, NULL);

    if (! reset_window(w))
	goto error;

    if (w->dpy_object)
	XSync(w->dpy, 0);

    DXFree((Pointer)w->title);
    DXFree((Pointer)w->cacheid);
    DXDelete((Object)w->dpy_object);

    if (w->translation_owner)
	DXDelete((Object)w->translation_owner);
    w->translation_owner = NULL;
    w->translation = NULL;

    DXFree(p);

    if (xerror) goto error;
    return OK;

error:    
    xerror = 0;
    return ERROR;
}

int
XChecker(int fd, Pointer p)
{
    struct window *w = p;
    return XPending(w->dpy);
}

static Error
_DXSetSoftwareWindowActive(struct window *w, int active)
{
    if (active)
    {
	if (! DXRegisterWindowHandlerWithCheckProc(w->handler, XChecker, w->dpy, (Pointer)w))
	    return ERROR;
	
	w->active = 1;
    }
    else
    {
	if (w->cmap_installed)
	{
	    XUninstallColormap(w->dpy, (Colormap)w->translation->cmap);
	    w->cmap_installed = 0;
	}

	if (! DXRegisterWindowHandlerWithCheckProc(NULL, NULL, w->dpy, (Pointer)w))
	    return ERROR;
	
	w->active = 0;
    }

    return OK;
}

static Error
display_either(Object image, int width, int height, int ox, int oy,
	       struct window *w)
{
    Display *dpy = w->dpy;

    if (w->active == 0)
    {
	if (! _DXSetSoftwareWindowActive(w, 1))
	    goto error;
    
	w->active = 1;
    }

    w->objectTag = DXGetObjectTag(image);

    /* is it a simple X image? */
    if (DXGetObjectClass(image)==CLASS_FIELD
	      && DXGetComponentValue((Field)image,IMAGE))
    {
	int image_tag;

        if (! DXGetIntegerAttribute(image, "image tag", &image_tag))
		  image_tag = (int)DXGetObjectTag(image);

	if ((int)DXGetObjectTag(w->pixels_owner) != image_tag) 
	{
	    free_ximage(w);
	    w->pixels_owner = DXReference(image);
	    w->pixels = _dxf_GetXPixels((Field)w->pixels_owner);
	    if (!w->pixels)
		goto error;
	    w->ximage = XCreateImage(dpy,
				     (Visual *)w->translation->visual,
				     w->translation->depth,
				     ZPixmap, 0, 
				     (char *) w->pixels,
				     width, height, 8, 0);
	}

    }

    /*
     * no; (re)-allocate ximage if necessary
     */
    else if (width != w->pwidth || height != w->pheight)
    {

	free_ximage(w);
	  w->pixels  = (unsigned char *) DXAllocate(width * height * 
				       _dxf_GetXBytesPerPixel(w->translation));
        if (!w->pixels)
	    goto error;
        w->pwidth = width;
        w->pheight = height;
	w->ximage = XCreateImage(dpy,
				 (Visual *)w->translation->visual,
				 w->translation->depth,
				 ZPixmap, 0, 
				 (char *) w->pixels,
				 width, height, 8, 0);
    }

    /*
     * dither the image
     */
    if (!w->pixels_owner)
    {
	/*
	 * zero whole thing if composite, to avoid holes
	 */
	if (DXGetObjectClass(image)!=CLASS_FIELD)
	{
	    uint32	pixel;
	    int		i;

	    if (w->translation->translationType == ThreeMap) 
	    {
	        pixel = RGBMIX(w->translation->rtable[0],
			       w->translation->gtable[0],
			       w->translation->btable[0]);
	    }
	    else if (w->translation->translationType == OneMap)
	    {
	        pixel = w->translation->table[0];
	    }
	    else
	        pixel = 0;

	    if (_dxf_GetXBytesPerPixel(w->translation) == 4)
	    {
	        uint32 *pixels = (uint32 *)w->pixels;
	    
		for(i=0;i<(width*height);i++)
		    *pixels++ = pixel;
	    }
	    else if (_dxf_GetXBytesPerPixel(w->translation) == 3)
	    {
		unsigned char *pixels = (unsigned char *)w->pixels;

		for(i=0;i<(width*height);i++) 
		{
		  *pixels++ = (pixel >>  0) & 0xff;
		  *pixels++ = (pixel >>  8) & 0xff;
		  *pixels++ = (pixel >> 16) & 0xff;
		}
	    }
	    else if (_dxf_GetXBytesPerPixel(w->translation) == 2)
	    {
		unsigned short *pixels = (unsigned short *)w->pixels;

		for(i=0;i<(width*height);i++) 
		  *pixels++ = (unsigned short) pixel;
	    }
	    else 
		memset(w->pixels,(unsigned char)pixel, width*height);

	}

	if (!_dxf_dither(image, width, height, ox, oy, w->translation,w->pixels))
	    goto error;
    }

    if (xerror) goto error;
    return OK;

error:
    xerror = 0;
    return ERROR;
}


static char *names[] = {
    "nothing",
    "event 1",
    "KeyPress",
    "KeyRelease",
    "ButtonPress",
    "ButtonRelease",
    "MotionNotify",
    "EnterNotify",
    "LeaveNotify",
    "FocusIn",
    "FocusOut",
    "KeymapNotify",
    "Expose",
    "GraphicsExpose",
    "NoExpose",
    "VisibilityNotify",
    "CreateNotify",
    "DestroyNotify",
    "UnmapNotify",
    "MapNotify",
    "MapRequest",
    "ReparentNotify",
    "ConfigureNotify",
    "ConfigureRequest",
    "GravityNotify",
    "ResizeRequest",
    "CirculateNotify",
    "CirculateRequest",
    "PropertyNotify",
    "SelectionClear",
    "SelectionRequest",
    "SelectionNotify",
    "ColormapNotify",
    "ClientMessage",
    "MappingNotify"
};
    

/*
 * Two possibilites: displaying a window in script mode, in which 
 * case we create the window and we refresh it; or displaying the
 * window in ui mode, in which case the UI creates the window and
 * refreshes it.  These are distinguished by whether the wid
 * parameter is a number or a string.
 */

#define BORDER 1

static Atom XA_WM_PROTOCOLS;
static Atom XA_WM_DELETE_WINDOW;

#if 0
void
DumpColors(Display *dpy, Colormap cmap)
{
    XColor colors[4096];
    int i, l;
    int flag = 0;

    l = 256;

    for (i = 0; i < l; i++)
	if (flag)
	    colors[i].pixel = (i << 16) | (i << 8) | i;
	else
	    colors[i].pixel = i;

    XQueryColors(dpy, cmap, colors, l);

    for (i = 0; i < l; i++)
	DXMessage("%d %d %d", 
	    colors[i].red >> 8,
	    colors[i].green >> 8,
	    colors[i].blue >> 8);
}

void
iDumpColors(long dpy, long cmap)
{
    DumpColors((Display *)dpy, (Colormap)cmap);
}

#endif

static Error
handler_script(int fd, struct window *w)
{
    XEvent event;
    extern char *names[];

    /* XXX - check for broken connection */
    XSync(w->dpy, 0);
    if (xerror) goto error;

    while (w->waitPixmap || w->waitEvent || XPending(w->dpy)) {

	XNextEvent(w->dpy, &event);

	if (event.xany.window != w->wid)
	    continue;

	DXDebug("X", "event %s", names[event.type]);
	if (xerror) goto error;

	if (event.type==w->waitEvent)
	{
	    w->waitEvent = 0;
	}
	if (event.type==Expose)
	{
	    w->refresh = 1;
	} 
	else if (event.type==EnterNotify)
	{
	    XInstallColormap(w->dpy, (Colormap)w->translation->cmap);
	    w->cmap_installed = 1;
	}
	else if (event.type==LeaveNotify)
	{
	    XUninstallColormap(w->dpy, (Colormap)w->translation->cmap);
	    w->cmap_installed = 0;
	}
	else if (event.type==ConfigureNotify)
	{
	    w->wwidth = event.xconfigure.width;
	    w->wheight = event.xconfigure.height;
	}
	else if (event.type==DestroyNotify)
	{
	    DXSetCacheEntry(NULL, CACHE_PERMANENT, w->cacheid, 0, 0);
	    if (xerror) goto error;
	    return OK;
	}
	else if (event.type==ClientMessage
	    && event.xclient.message_type==XA_WM_PROTOCOLS)
	{
	    DXSetCacheEntry(NULL, CACHE_PERMANENT, w->cacheid, 0, 0);
	    if (xerror) goto error;
	    return OK;
	}
    }

    if (w->refresh)
    {
	XInstallColormap(w->dpy, (Colormap)w->translation->cmap);
	w->cmap_installed = 1;
	XPutImage(w->dpy, w->wid, w->gc, w->ximage,
		  0, 0, 0, 0, w->wwidth, w->wheight);
    }

    w->refresh = 0;

    XSync(w->dpy, 0);

    if (xerror) goto error;
    return OK;

error:
    xerror = 0;
    return ERROR;
}

static Error  setColors(struct window *w, Colormap xcmap, Array icmap);

static Error
display_script(Object image, int width, int height,
		int ox, int oy, struct window *w)
{
    Display 		 *dpy;
    Window 		 root;
    /*int 		 screen;*/
    translationP	 trans = NULL;
    XSetWindowAttributes xswa;
    XColor 		 cdef, hdef;
    /*Visual  		 *vis;*/
    /*Colormap		 xcmap;*/
    int			 border = BORDER;

    ASSERT(DXGetError()==ERROR_NONE);
    dpy = w->dpy;
    root = DefaultRootWindow(dpy);
    /*screen = DefaultScreen(dpy);*/
    /*vis = w->translation->visual;*/
    trans = w->translation;
    /*xcmap = trans->cmap;*/

    /* create the window if necessary */
    if (!w->gc) {

	XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", 0);
	XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);

	/* get the color map and visual window */

	if(! trans)
	   DXErrorReturn(ERROR_INTERNAL, "#11610");
	
	XLookupColor(dpy, trans->cmap, "black", &cdef, &hdef);

	xswa.background_pixel = hdef.pixel;
	xswa.border_pixel     = hdef.pixel;
	xswa.colormap         = trans->cmap;

	w->wid  = XCreateWindow(dpy, root, 0, 0, width, height,
			border,
			trans->depth,
			InputOutput, trans->visual,
			CWColormap | CWBackPixel | CWBorderPixel, &xswa);
	
	w->winFlag = 1;

        XSetStandardProperties(dpy, w->wid, w->title, w->title,
			       None, NULL, 0, NULL);

	XChangeProperty(dpy, w->wid, XA_WM_PROTOCOLS, XA_ATOM, 32,
			PropModeReplace,
			(unsigned char *)&XA_WM_DELETE_WINDOW,	1);

        XSelectInput(dpy, w->wid,
	    EnterWindowMask 		| 
	    LeaveWindowMask 		| 
	    ExposureMask    		| 
	    StructureNotifyMask);

	XMapRaised(dpy, w->wid);
	w->waitEvent = Expose;
        w->gc = XCreateGC(dpy, w->wid, 0, 0);
        w->wwidth = width;
        w->wheight = height;
	DXMarkTime("create window");
    }

    if (!display_either(image, width, height, ox, oy, w))
	goto error;

    /* resize the window if necessary */
    if (width!=w->wwidth || height!=w->wheight) {
	XResizeWindow(dpy, w->wid, width, height);
	w->waitEvent = ConfigureNotify;
        w->wwidth = width;
        w->wheight = height;
	DXMarkTime("new size");
    }

    /* handle events (i.e. actually draw image) */
    w->refresh = 1;
    if (!handler_script(w->fd, w))
	goto error;

    if (xerror) goto error;
    return OK;

error:
    xerror = 0;
    return ERROR;
}

/* #X added */
static Error
display_external(Object image, int width, int height,
		int ox, int oy, struct window *w)
{
    Display 		 *dpy;
    Window 		 root;
    /*int 		 screen;*/
    translationP	 trans = NULL;
    XSetWindowAttributes xswa;
    XColor 		 cdef, hdef;
    /*Visual  		 *vis;*/
    /*Colormap		 xcmap;*/
    int			 border = BORDER;

    ASSERT(DXGetError()==ERROR_NONE);
    dpy = w->dpy;
    root = DefaultRootWindow(dpy);
    /*screen = DefaultScreen(dpy);*/
    /*vis = w->translation->visual;*/
    trans = w->translation;
    /*xcmap = trans->cmap;*/

    /* create the window if necessary */
    if (!w->gc) {

	XA_WM_PROTOCOLS = XInternAtom(dpy, "WM_PROTOCOLS", 0);
	XA_WM_DELETE_WINDOW = XInternAtom(dpy, "WM_DELETE_WINDOW", 0);

	/* get the color map and visual window */

	if(! trans)
	   DXErrorReturn(ERROR_INTERNAL, "#11610");
	
	if (w->wid == -1)
	{
	    XLookupColor(dpy, trans->cmap, "black", &cdef, &hdef);

	    xswa.background_pixel = hdef.pixel;
	    xswa.border_pixel     = hdef.pixel;
	    xswa.colormap         = trans->cmap;

	    w->wid  = XCreateWindow(dpy, root, 0, 0, width, height,
			    border,
			    trans->depth,
			    InputOutput, trans->visual,
			    CWColormap | CWBackPixel | CWBorderPixel, &xswa);
	
	    w->winFlag = 1;

	}

	XChangeProperty(dpy, w->wid, XA_WM_PROTOCOLS, XA_ATOM, 32,
			PropModeReplace,
			(unsigned char *)&XA_WM_DELETE_WINDOW,	1);

        XSelectInput(dpy, w->wid,
	    EnterWindowMask 		| 
	    LeaveWindowMask 		| 
	    ExposureMask    		| 
	    StructureNotifyMask);

	XMapRaised(dpy, w->wid);
        w->gc = XCreateGC(dpy, w->wid, 0, 0);
        w->wwidth = width;
        w->wheight = height;
	DXMarkTime("create window");
    }

    if (!display_either(image, width, height, ox, oy, w))
	goto error;

    w->wwidth = width;
    w->wheight = height;

    /* handle events (i.e. actually draw image) */
    w->refresh = 1;
    if (!handler_script(w->fd, w))
	goto error;

    if (xerror) goto error;
    return OK;

error:
    xerror = 0;
    return ERROR;
}
/* #X end */

/*
 * UI mode input handler and display routine.
 */

static Error
handler_ui(int fd, struct window *w)
{
    XEvent event;
    extern char *names[];

    while (w->waitEvent || w->waitPixmap || XPending(w->dpy)) {

	XNextEvent(w->dpy, &event);

	if (event.xany.window != w->wid)
	    continue;

	DXDebug("X", "event %s", names[event.type]);
	if (xerror) goto error;

	if (event.type==DestroyNotify)
	{
	    w->win_invalid = 1;
	    DXSetCacheEntry(NULL, CACHE_PERMANENT, w->cacheid, 0, 0);
	    w->wid = None;
	    w->winFlag = 0;
	    if (xerror) goto error;
	    return OK;
	}
	else if (event.type==EnterNotify)
	{
	    XInstallColormap(w->dpy, (Colormap)w->translation->cmap);
	    w->cmap_installed = 1;
	}
	else if (event.type==LeaveNotify)
	{
	    XUninstallColormap(w->dpy, (Colormap)w->translation->cmap);
	    w->cmap_installed = 0;
	}
	else if (event.type==PropertyNotify)
	{
	    DXDebug("X", "atom 0x%x, state %d",
		  event.xproperty.atom, event.xproperty.state);
	    if (event.xproperty.atom==w->flag_atom
	      && event.xproperty.state==PropertyDelete) {
		w->pixmapFull = 0;
		w->waitPixmap = 0;
	    }
	}
    }

    return OK;

error:
    xerror = 0;
    return ERROR;
}

static Error
display_ui(Object image, int width, int height,
			int ox, int oy, struct window *w)
{

    XEvent event;
    Display *dpy;
    Pixmap new = None;
    translationP trans;
    Visual *vis;
    Colormap xcmap;

    dpy = w->dpy;

    if ((trans = w->translation) == NULL)
    {
	DXSetError(ERROR_INTERNAL, "no translation in display_ui");
	goto error;
    }

    vis = trans->visual;

    xcmap = trans->cmap;
    if (! xcmap)
    {
      DXSetError(ERROR_INTERNAL, "Translation had no colormap");
      goto error;
    }	

    /* initialization if necessary */
    if (!w->gc)
    {
        w->pixmap_atom = XInternAtom(dpy, "DX_PIXMAP_ID", 0);
        w->flag_atom = XInternAtom(dpy, "DX_FLAG", 0);
        XSelectInput(dpy, w->wid, 
	    PropertyChangeMask		|
	    StructureNotifyMask		|
	    EnterWindowMask 		| 
	    LeaveWindowMask);
        w->gc = XCreateGC(dpy, w->wid, 0, 0);

    }

    if (w->winFlag &&
		(vis->class == PseudoColor || vis->class == DirectColor))
	XSetWindowColormap(dpy, w->wid, xcmap);


    if (!display_either(image, width, height, ox, oy, w))
	goto error;

#define ASYNC 1

#if ASYNC
    /* wait for last image to complete */
    if (w->pixmapFull > 0)
    {
	w->waitPixmap = 1;
	if (!handler_ui(w->fd, w))
	    goto error;
    }
#endif

    /* put image in pixmap */
    DXDebug("X", "putting image and changing property");
    if (width!=w->wwidth || height!=w->wheight || 
	w->translation->depth != w->wdepth) {

	/* new or new-size pixmap */
	if (width==0 || height==0)
	    DXErrorGoto(ERROR_BAD_PARAMETER, "#11750");
        new = XCreatePixmap(dpy, w->wid, width, height, 
			    w->translation->depth);
	if (!new)
	    goto error;
	XPutImage(w->dpy, new, w->gc, w->ximage, 0, 0, 0, 0, width, height);
        XChangeProperty(dpy, w->wid, w->pixmap_atom, XA_PIXMAP,
			32, PropModeReplace, (unsigned char*)&(new), 1);

	/* now free the old one */
        if (w->pixmap)
            XFreePixmap(dpy, w->pixmap);
	w->pixmap = new;
        w->wwidth = width;
        w->wheight = height;
	w->wdepth = w->translation->depth;

    } else {

	/* just use the old pixmap */
	XPutImage(w->dpy, w->pixmap, w->gc, w->ximage,
		  0, 0, 0, 0, width, height);

    }

    XInstallColormap(w->dpy, (Colormap)w->translation->cmap);
    w->cmap_installed = 1;

    /* flag ready */
    XChangeProperty(dpy, w->wid, w->flag_atom, XA_INTEGER, 32,
		    PropModeReplace, (unsigned char*)&(w->pixmap), 1);

    XSync(dpy, 0);
    DXMarkTime("put image");
    w->pixmapFull = 1;

    /* generate exposure event */
    event.xclient.type = ClientMessage;
    event.xclient.window = w->wid;
    event.xclient.message_type = XA_INTEGER;
    event.xclient.format = 16;
    event.xclient.data.s[0]  = width;
    event.xclient.data.s[1] = height;
    XSendEvent(dpy, w->wid, 1, NoEventMask, &event);

#if !ASYNC
    if (!handler_ui(w->fd, c))
	goto error;
#endif

    if (xerror) goto error;
    return OK;

error:
    xerror = 0;
    w->waitPixmap = 0;
    w->waitEvent = 0;
    return ERROR;
}



/*
 *XXX These are all separate calls because DXDisplayX is a documented interface
 * that does not support depth.
 */

Object
DXDisplayX(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,0,host,window);
}
Object
DXDisplayX4(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,4,host,window);
}
Object
DXDisplayX8(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,8,host,window);
}
Object
DXDisplayX12(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,12,host,window);
}
Object
DXDisplayX15(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,15,host,window);
}
Object
DXDisplayX16(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,16,host,window);
}
Object
DXDisplayX24(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,24,host,window);
}
Object
DXDisplayX32(Object image, char *host, char *window)
{
  return DXDisplayXAny(image,32,host,window);
}

static Error  CheckColormappedImage(Object image, Array colormap);

static Error
delete_display(Pointer p)
{
    if (p)
    {
	struct display *dpy_ptr = (struct display *)p;
	Display *dpy = dpy_ptr->dpy;

	if (! dpy)
	{
	    DXSetError(ERROR_INTERNAL, "error closing display");
	    return ERROR;
	}

	XSync(dpy,0);

	XCloseDisplay(dpy);

	DXFree(dpy_ptr->cacheid);

	DXFree(p);
    }

    return OK;
}

static Private
create_display(char *name, char *title, int depth, Display **dpy)
{
    Private p  = NULL;
    char *cacheid = NULL;
    struct display *dpy_ptr = NULL;
    char buf[1024];
    Display *d;

    sprintf(buf, "XDisplay %s %s %d", name, title, depth);
    
    p = (Private)DXGetCacheEntry(buf, 0, 0);
    if (p)
    {
	dpy_ptr = (struct display *)DXGetPrivateData(p);
    }
    else
    {
	int len = strlen(buf) + 1;

	cacheid = (char *)DXAllocate(len);
	if (! cacheid)
	    goto error;
	
	memcpy(cacheid, buf, len);

#if defined(intelnt) || defined(WIN32)
{
	int i;
	i = _dxfHostIsLocal(name);
	if(i)
	    d = XOpenDisplay(NULL);
	else
	    d = XOpenDisplay(name);
}
#else
	d = XOpenDisplay(name);
#endif
	if (! d)
	    goto error;

	dpy_ptr = (struct display *)DXAllocate(sizeof(struct display));
	if (! dpy_ptr)
	    goto error;
	
	p = DXNewPrivate((Pointer)dpy_ptr, delete_display);
	if (! p)
	    goto error;
	
	DXReference((Object)p);

	dpy_ptr->dpy     = d;
	dpy_ptr->cacheid = cacheid;

	cacheid = NULL;

/*
 * DO NOT CACHE DISPLAY UNTIL ALL WINDOWS CAN SHARE IT!
 */
#if 0
	if (! DXSetCacheEntry((Object)p, CACHE_PERMANENT,
						dpy_ptr->cacheid, 0, 0))
	    goto error;
#endif
    }

    *dpy = dpy_ptr->dpy;
    
    return p;

error:
    if (p)
	DXDelete((Object)p);
    else if (dpy_ptr)
	DXFree((Pointer)dpy_ptr);
    
    DXFree((Pointer)cacheid);

    return ERROR;
}

static Display *
getDisplay(Private p)
{
    struct display *dpy_ptr;

    if (! p)
	return NULL;
    
    dpy_ptr = (struct display *)DXGetPrivateData(p);
    return dpy_ptr->dpy;
}

static Error
free_private_string(Pointer p)
{
    DXFree(p);
    return OK;
}


#define WINDOW_TEMPLATE "Display Window Software X%s,%s"

static char * 
getWindowCacheTag(char *host, char *title)
{
    char *cacheid = (char *)DXAllocate(strlen(WINDOW_TEMPLATE) +
				       strlen(host) +
				       strlen(title) + 1);
    if (! cacheid)
	return NULL;
    
    sprintf(cacheid, WINDOW_TEMPLATE, host, title);

    return cacheid;
}

#define WINDOW_ID_TEMPLATE "Display Window ID %s"

static char * 
getWindowIDCacheTag()
{
    char *modid;
    char *cacheid;

    modid = (char *)DXGetModuleId();

    cacheid = (char *)DXAllocate(strlen(WINDOW_ID_TEMPLATE) +
				 strlen(modid) + 1);

    sprintf(cacheid, WINDOW_ID_TEMPLATE, modid);

    DXFreeModuleId((Pointer)modid);

    if (! cacheid)
	return NULL;

    return cacheid;
}

static Error 
setWindowIDInCache(char *tag, char *title)
{
    Private p = NULL;
    char *tmp = NULL;
    
    tmp = (char *)DXAllocate(strlen(title) + 1);
    if (! tmp)
	goto error;
    
    strcpy(tmp, title);

    p = DXNewPrivate((Pointer)tmp, free_private_string);
    if (! p)
	goto error;
    tmp = NULL;
    
    if (! DXSetCacheEntry((Object)p, CACHE_PERMANENT, tag, 0, 0))
	goto error;
    p = NULL;

    return OK;

error:
    DXFree((Pointer)tmp);
    DXDelete((Object)p);
    return ERROR;
}

static char *
getWindowIDFromCache(char *tag)
{
    Private p = NULL;
    char *tmp, *p_data;

    p = (Private)DXGetCacheEntry(tag, 0, 0);
    
    if (! p)
	return NULL;

    p_data = (char *)DXGetPrivateData(p);

    tmp = (char *)DXAllocate(strlen(p_data) + 1);
    if (! tmp)
	goto error;
    
    strcpy(tmp, p_data);
    DXDelete((Object)p);

    return tmp;

error:
    DXDelete((Object)p);
    return NULL;
}

Error
_dxf_SetSoftwareXWindowActive(char *where, int active)
{
    char *cachetag = NULL;
    Private p = NULL;
    char *host = NULL, *title = NULL;

    if (! DXParseWhere(where, NULL, NULL, &host, &title, NULL, NULL))
	 goto error;

    cachetag = getWindowCacheTag(host, title);

    p = (Private)DXGetCacheEntry(cachetag, 0, 0);
    DXFree((Pointer)cachetag);

    if (p)
    {
	struct window *w = (struct window *)DXGetPrivateData(p);

	if (! _DXSetSoftwareWindowActive(w, active))
	    goto error;

	DXDelete((Object)p);
    }

    DXFree((Pointer)host);
    DXFree((Pointer)title);

    return OK;

error:
    DXDelete((Object)p);
    DXFree((Pointer)host);
    DXFree((Pointer)title);

    return ERROR;
}
	


static Error
getWindowStructure(char *host, int depth, char *title, int *directMap,
	struct window **window, Private *window_object, int required)
{
    Private p = NULL;
    char *cachetag = NULL;
    struct window *w = NULL;
    Visual *vis;

    *window = NULL;
    *window_object = NULL;

    if (!title || !*title)
	title = "Image";
	 
    if (!host || !*host)
	host = getenv("DISPLAY");
    if (! host)
#if defined(intelnt) || defined(WIN32)
       host = "";
#else
       host = "unix:0";
#endif

    cachetag = getWindowCacheTag(host, title);

    /*
     * If this is a UI window, then check the "title" (which is in
     * fact ##windowid) against the previous title to see if we have
     * a new window Id.
     */
    if (title[0] == '#' && title[1] == '#')
    {
	char *old_title = NULL, *title_tag = getWindowIDCacheTag();
	if (! title_tag)
	    goto error;

	old_title = getWindowIDFromCache(title_tag);
	if (old_title)
	{
	    /*
	     * If the two differ, throw away the cached window object
	     * and save new window id
	     */
	    if (strcmp(title, old_title))
	    {
		char *old_tag = getWindowCacheTag(host, old_title);

		if (! DXSetCacheEntry(NULL, 0, old_tag, 0, 0))
		{
		    DXFree((Pointer)old_title);
		    DXFree((Pointer)title_tag);
		    goto error;
		}

		DXFree((Pointer)old_tag);

		/*
		 * and save new window id
		 */
		setWindowIDInCache(title_tag, title);
	    }

	    DXFree((Pointer)old_title);

	}
	else
	    setWindowIDInCache(title_tag, title);

	DXFree((Pointer)title_tag);
    }
		
    p = (Private)DXGetCacheEntry(cachetag, 0, 0);
    
    if (!p && required)
	return ERROR;

    if (p)
	w = (struct window *)DXGetPrivateData(p);
    
    /*
     * If there is a window, we need to determine whether the associated
     * translation is OK.
     */
    if (w && w->translation && (depth == 0 || w->translation->depth == depth))
    {
	/*
	 * If we received a direct colormap, then we need to update the
	 * translation colormap if the previous translation was of type
	 * NoTranslation and the new colormap is different from the
	 * previous colormap.  We need to try to create a *new* translation
	 * if the previous translation was not type NoTranslation.
         */
	if (*directMap)
	{
	    if (w->translation->translationType != NoTranslation)
	    {
		DXDelete(w->translation_owner);
		w->translation = NULL;
		w->translation_owner = NULL;

		w->colormapTag = 0;

		w->translation_owner = createTranslation(w->dpy_object,
						host, depth, directMap, w);
		
		w->translation = _dxf_GetXTranslation(w->translation_owner);
	    
		if (w->winFlag)
		    XSetWindowColormap(w->dpy, w->wid,
				(Colormap)w->translation->cmap);
	    }
	}
	else if (w->translation->translationType == NoTranslation ||
			      (depth != 0 && w->translation->depth != depth))
	{
	    DXDelete(w->translation_owner);
	    w->translation = NULL;
	    w->translation_owner = NULL;

	    w->colormapTag = 0;

	    w->translation_owner =
		createTranslation(w->dpy_object, host, depth, directMap, w);
		    
	    w->translation =
		_dxf_GetXTranslation(w->translation_owner);
	    
	    if (w->winFlag)
		XSetWindowColormap(w->dpy, w->wid,
			(Colormap)w->translation->cmap);
	}
    }
    else
    {
	char *num;

	if (w)
	{
	    /*
	     * then we need to reset the window since we
	     * will be changing visuals.
	     */
	    reset_window(w);
	}
	else
	{
	    /*
	     * window structure wasn't in cache
	     */
	    w = (struct window *)DXAllocateZero(sizeof(struct window));
	    if (! w)
		goto error;

	    p = DXNewPrivate((Pointer)w, delete_window);
	    if (! p)
		goto error;
	    
	    if (! DXSetCacheEntry((Object)p, CACHE_PERMANENT, cachetag, 0, 0))
		goto error;
	    
	    DXReference((Object)p);

	    w->dpy_object = create_display(host, title, depth, &w->dpy);
	    if (! w->dpy_object)
	    {
		DXSetError(ERROR_BAD_PARAMETER, "#11700", host);
		goto error;
	    }

	    XSetErrorHandler(error_handler);
	
	    /*
	     * #X section was
	     * w->ui = 1;
	     * num += 2;
	     *
	     * for (s=num; *s; s++)
	     *     if (*s<'0' || '9'<*s)
	     *           w->ui = 0;
	     *
	     * if (w->ui)
	     * {
	     *	   w->handler = (Handler)handler_ui;
	     *	   w->wid = atoi(num);
	     *	   w->winFlag = 1;
	     * }
             * else
             *  {
	     *	   w->handler = (Handler)handler_script;
     	     *	   w->wid = -1;
	     *     w->winFlag = 0;
             * }
	     *
	     * And is now:
	     */
	    num = title;
	    if (num[0]=='#' && num[1]=='#')
	    {
		w->windowMode = WINDOW_UI;
		w->handler = (Handler)handler_ui;
		num += 2;
		w->wid = atoi(num);
		w->winFlag = 1;
	    }
	    else if (num[0]=='#' && num[1]=='X')
	    {
		w->windowMode = WINDOW_EXTERNAL;
		w->handler = (Handler)handler_script;
		num += 2;
		w->wid = atoi(num);
		w->winFlag = 1;
	    }
	    else
	    {
		w->windowMode = WINDOW_LOCAL;
		w->handler = (Handler)handler_script;
		w->wid = -1;
		w->winFlag = 0;
	    }
	    /* #X end */

	    w->fd = ConnectionNumber(w->dpy);

	    w->title = (char *)DXAllocate(strlen(title) + 1);
	    strcpy(w->title, title);

	    w->cacheid = (char *)DXAllocate(strlen(cachetag) + 1);
	    strcpy(w->cacheid, cachetag);
	}

	w->objectTag = 0;

	w->translation_owner =
		createTranslation(w->dpy_object, host, depth, directMap, w);

	if (! w->translation_owner)
	    goto error;

	w->translation = _dxf_GetXTranslation(w->translation_owner);

	vis = w->translation->visual;

	if (w->winFlag &&
	    vis != XDefaultVisual(w->dpy, XDefaultScreen(w->dpy)) &&
	    (vis->class == PseudoColor || vis->class == DirectColor))
	    XSetWindowColormap(w->dpy, w->wid, (Colormap)w->translation->cmap);
    }

    *window = w;
    *window_object = p;

    DXFree((Pointer)cachetag);
    cachetag = NULL;

    return OK;

error:
    if (p)
	DXDelete((Object)p);
    else
	DXFree((Pointer)w);

    if (! DXSetCacheEntry(NULL, CACHE_PERMANENT, cachetag, 0, 0))
	goto error;
	
    if (cachetag)
	DXFree((Pointer)cachetag);

    return ERROR;
}

static Array
getColormap(Object o)
{
    Array iArray = NULL;
    Array map;

    if (DXGetObjectClass(o) == CLASS_ARRAY)
    {
	map = (Array)o;
    }
    else
    {
	int i;
	float *indices;

	iArray = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 1);
	if (! iArray)
	    goto error;
	
	if (! DXAddArrayData(iArray, 0, 255, NULL))
	    goto error;

	indices = (float *)DXGetArrayData(iArray);

	for (i = 0; i < 256; i++)
	    indices[i] = i;
	
	map = (Array)DXMap((Object)iArray, o, NULL, NULL);
	if (! map)
	    goto error;
	
	DXDelete((Object)iArray);
    }

    return (Array)DXReference((Object)map);

error:
    DXDelete((Object)iArray);
    return NULL;
}

Object
DXDisplayXAny(Object image, int depth, char *host, char *window)
{
    struct window *w = NULL;
    int directMap = 0;
    Object attr;
    int  doPixels = 1;
    Private w_obj = NULL;;

    attr = DXGetAttribute(image, "direct color map");
    if (attr)
        if (! DXExtractInteger(attr, &directMap))
        {
	    DXSetError(ERROR_BAD_PARAMETER, 
		  "direct color map attribute must be an integer");
	    goto error;
        }

    /*
     * Get the structure that describes the window.  The following
     * routine either gets it from the cache or creates it and
     * places it there.  If direct colormapping is requested, make
     * an extra effort to get an appropriate visual.  Note that
     * if we *can't* do direct mapping, then getWindowStructure will
     * reset the directMap flag.
     */
    if (! getWindowStructure(host, depth, window, &directMap, &w, &w_obj, 0))
        return NULL;
    
    /*
     * we can only direct map if the visual was PseudoColor or GrayScale
     */
    if (directMap && w->translation->translationType != NoTranslation)
	directMap = 0;
  
    /*
     * We don't need to re-translate if its the same image and the
     * translation is either NoTranslation or the color map is the 
     * same.
     */
    if (DXGetObjectTag(image) == w->objectTag)
    {
	Object o;

	doPixels = 0;

	if (! directMap)
	{
	    o = DXGetAttribute(image, "color map");
	    if (!o && DXGetObjectClass(image) == CLASS_FIELD)
		o = DXGetComponentValue((Field)image, "color map");
		
	    if (o && DXGetObjectTag(o) != w->colormapTag)
		    doPixels = 1;
	}
    }

    if (doPixels)
    {
	int width, height, ox, oy;

	/*
	 * composite image size
	 */
	if (! DXGetImageBounds(image, &ox, &oy, &width, &height))
	    return NULL;
  
	if (! w->translation)
	    goto error;
  
	/*
	 * call appropriate display routine
	 */
	/* #X was
	 * if (w->ui)
	 */
	if (w->windowMode == WINDOW_UI)
	{
	    if (!display_ui(image, width, height, ox, oy, w))
		goto error;
	}
	/* #X added */
	else if (w->windowMode == WINDOW_EXTERNAL)
	{
	    if (!display_external(image, width, height, ox, oy, w))
		goto error;
	}
	/* #X end */
	else
	{
	    if (!display_script(image, width, height, ox, oy, w))
		goto error;
	}

	w->objectTag = DXGetObjectTag(image);
    }
      
    if (directMap)
    {
	Object iCmap;

	if (w->translation->translationType == NoTranslation)
	{
	
	    iCmap = DXGetAttribute(image, "color map");
	    if (!iCmap && DXGetObjectClass(image) == CLASS_FIELD)
		iCmap = DXGetComponentValue((Field)image, "color map");

	    if (iCmap)
	    {
		Array colormap = getColormap(iCmap);

		if (! colormap)
		     goto error;
	    
		if (! CheckColormappedImage(NULL, colormap))
		{
		    if ((Object)colormap != iCmap)
			DXDelete((Object)colormap);
		    goto error;
		}
      
		if (! setColors(w, (Colormap)w->translation->cmap, colormap))
		{
		    if ((Object)colormap != iCmap)
			DXDelete((Object)colormap);
		    goto error;
		}
		
		if ((Object)colormap != iCmap)
		    DXDelete((Object)colormap);
	    }
	}
    }

    XFlush(w->dpy);

    DXDelete((Object)w_obj);

    return image;
  
error:
    DXDelete((Object)w_obj);
    return NULL;
}

#if 0
Error
DXSetWindowColormap(Object iCmap, char *host, char *window)
{
    struct window *w = NULL;
    Private w_obj = NULL;
    Array colormap = getColormap(iCmap);
    int directMap = 1;

    if (! colormap)
	goto error;

    if (! CheckColormappedImage(NULL, colormap))
        goto error;

    /*
     * Get the structure that describes the window.  The following
     * routine either gets it from the cache or creates it and
     * places it there.
     */
    if (! getWindowStructure(host, 0, window, &directMap, &w, &w_obj, 0))
	return NULL;
    
    if (! directMap)
    {
	DXSetError(ERROR_INTERNAL,
		"Unable to do direct colormapping on this display");
	goto error;
    }

    if (! w->translation)
    {
	DXSetError(ERROR_INTERNAL, "window has no translation");
        goto error;
    }

    if (w->translation->translationType != NoTranslation)
    {
	DXSetError(ERROR_INTERNAL, "window not set up for direct colormapping");
	goto error;
    }
      
    if (! setColors(w, (Colormap)w->translation->cmap, colormap))
	goto error;

    XInstallColormap(w->dpy, (Colormap)w->translation->cmap);
    w->cmap_installed = 1;

    XFlush(w->dpy);
   
    if ((Object)colormap != iCmap)
	DXDelete((Object)colormap);

    DXDelete((Object)w_obj);

    return OK;

error:
    DXDelete((Object)w_obj);

    if ((Object)colormap != iCmap)
	DXDelete((Object)colormap);

    return ERROR;
}
#endif


Error
_dxf_CopyBufferToXImage(struct buffer *buf, Field image, int xoff, int yoff)
{
    int iwidth, iheight;
    unsigned char *dst, *src;
    translationP translation;
    int inType;
    Object o;
    o = DXGetComponentAttribute(image, IMAGE, X_SERVER);
    translation = _dxf_GetXTranslation(o);
    if (! translation)
    {
        DXSetError(ERROR_INTERNAL,
            "missing translation in _dxf_CopyBufferToXImage");
        return ERROR;
    }
    dst = _dxf_GetXPixels(image);
    if(!dst)
      return ERROR;
    if (buf->pix_type == pix_fast)
    {
        src = (unsigned char *)buf->u.fast;
        inType = IN_FAST_PIX;
    }
    else if (buf->pix_type == pix_big)
    {
        src = (unsigned char *)buf->u.big;
        inType = IN_BIG_PIX;
    }
    else
    {
        DXSetError(ERROR_INTERNAL, "unrecognized pix type");
        return ERROR;
    }
    if (!DXGetImageSize(image, &iwidth, &iheight))
        return ERROR;
    return _dxf_translateImage((Object)image, iwidth, iheight,
                0, 0, yoff, xoff, buf->height, buf->width,
                0, 1, translation,
                src, inType, dst);
}


/*
 * Special images: dither X images.  They are relative to
 * the color map on a particular X server, so they contain
 * a pointer to the X translation table returned by createTranslation
 */

Field _dxf_MakeXImage(int width, int height, int depth, char *where)
{
    Field i = NULL;
    Array a = NULL;
    translationP	translation;
    Private w_obj = NULL;
    struct window *w;
    char *s, copy[201], *host = NULL, *window = NULL;
    int directMap = 0;

    strcpy(copy, where);
    for (s = copy; *s && !window; s++)
	if (*s == ',')
	{
	    if (host == NULL) 
		host = s+1;
	    else
	        window = s+1;
	    
	    *s = '\0';
	}

    /* an image is a field */
    i = DXNewField();
    if (!i) goto error;

    /* positions */
    a = DXMakeGridPositions(2, height,width,
			  /* origin: */ 0.0,0.0,
			  /* deltas: */ 0.0,1.0, 1.0,0.0);
    if (!a) goto error;
    DXSetComponentValue(i, POSITIONS, (Object)a);
    a = NULL;

    /* connections */
    a = DXMakeGridConnections(2, height,width);
    if (!a) goto error;
    DXSetConnections(i, "quads", a);
    a = NULL;

    XSetErrorHandler(error_handler);

    if (! getWindowStructure(host, depth, window, &directMap, &w, &w_obj, 0))
	goto error;

    translation = w->translation;
    if (!translation)
	goto error;

    if (_dxf_GetXBytesPerPixel(translation) == 4)
    {
      a = DXNewArray(TYPE_UINT, CATEGORY_REAL, 0);
    }
    else if (_dxf_GetXBytesPerPixel(translation) == 3)
    {
      a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, 3);
    }
    else if (_dxf_GetXBytesPerPixel(translation) == 2)
    {
      a = DXNewArray(TYPE_USHORT, CATEGORY_REAL, 0);
    }
    else
    {
      a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
    }

    if (!a)
	goto error;

    if (!DXAddArrayData(a, 0, width*height, NULL))
    {
	DXAddMessage("can't make %dx%d image", width, height);
	goto error;
    }
    DXSetAttribute((Object)a, IMAGE_TYPE, O_X_IMAGE);
    DXSetAttribute((Object)a, X_SERVER, w->translation_owner);
    DXSetComponentValue(i, IMAGE, (Object)a);
    a = NULL;

    DXDelete((Object)w_obj);

    /* set default ref and dep */
    return DXEndField(i);

error:
    DXDelete((Object)w_obj);
    DXDelete((Object)a);
    DXDelete((Object)i);
    return NULL;
}


unsigned char *
_dxf_GetXPixels(Field i)
{
    Array a;

    /* return the colors component */
    a = (Array) DXGetComponentValue(i, IMAGE);
    if (!a || DXGetAttribute((Object)a,IMAGE_TYPE) != O_X_IMAGE) {
      DXSetError(ERROR_BAD_PARAMETER, "#11610");
	return NULL;
      }
    if (!DXTypeCheck(a, TYPE_UBYTE, CATEGORY_REAL, 0) &&
        !DXTypeCheck(a, TYPE_UBYTE, CATEGORY_REAL, 1, 3) &&
        !DXTypeCheck(a, TYPE_USHORT, CATEGORY_REAL, 0) &&
	!DXTypeCheck(a, TYPE_UINT, CATEGORY_REAL, 0)) {
	DXSetError(ERROR_BAD_PARAMETER, "#11610");
	return NULL;
      }
    return (unsigned char *) DXGetArrayData(a);
}

#define ZERO_LOOP(osz, d, dith, x1, pix, x2, res)			\
{ 									\
    unsigned char *_dst = dst + ((iheight-top)*iwidth + left)*osz; 	\
    unsigned char *t_dst; 						\
    int _ir = ir, _ig = ig, _ib = ib, _y = (iheight-top);		\
									\
    for (y=0;  y<bheight;  y++, _y++)					\
    { 									\
	if (d)								\
	    m = &(_dxd_dither_matrix[_y&(DY-1)][0]); 			\
									\
	t_dst = _dst;							\
	for (x = 0; x < bwidth; x++)					\
	{								\
	    ir = _ir;							\
	    ig = _ig;							\
	    ib = _ib;							\
									\
	    dith;							\
	    x1;								\
	    pix;							\
	    x2;								\
	    res;							\
									\
	    t_dst += osz;						\
	}								\
									\
	_dst += osz*iwidth;						\
    }									\
}

Field
_dxf_ZeroXPixels(Field image, int left, int right, int top, int bot, RGBColor c)
{
    int                         x, y, d, i;
    Object 			oo;
    int				rr, gg, bb;
    int				outType;
    int				*gamma;
    translationP		translation = NULL;
    int				ir, ig, ib;
    int				bheight, bwidth;
    int				iheight, iwidth;
    unsigned char 		*dst = (unsigned char *)_dxf_GetXPixels(image);
    short			*m;

    if (! dst)
    {
	DXSetError(ERROR_INTERNAL, "unable to access pixels in _dxf_ZeroXPixels");
	goto error;
    }

    oo = DXGetComponentAttribute(image, IMAGE, X_SERVER);
    if (oo)
	translation = _dxf_GetXTranslation(oo);

    if (!translation)
	DXErrorReturn(ERROR_INTERNAL, "#11610");

    rr = translation->redRange;
    gg = translation->greenRange;
    bb = translation->blueRange;

    gamma = translation->gammaTable;

    bwidth = right - left;
    bheight = top - bot;
    DXGetImageSize(image, &iwidth, &iheight);

    ir = (int)(255 * c.r);
    ir = (ir > 255) ? 255 : (ir < 0) ? 0 : ir;
    ig = (int)(255 * c.g);
    ig = (ig > 255) ? 255 : (ig < 0) ? 0 : ig;
    ib = (int)(255 * c.b);
    ib = (ib > 255) ? 255 : (ib < 0) ? 0 : ib;

    if (_dxf_GetXBytesPerPixel(translation) == 4)
	outType = OUT_INT32;
    else if (_dxf_GetXBytesPerPixel(translation) == 3)
	outType = OUT_INT24;
    else if (_dxf_GetXBytesPerPixel(translation) == 2)
	outType = OUT_INT16;
    else
	outType = OUT_INT8;

    if (translation->translationType == OneMap)
    {
	unsigned long *table = translation->table;

	if (outType == OUT_INT8)
	    ZERO_LOOP(1, 1, DITHER, NO_XLATE, PIXEL_1, ONE_XLATE, STORE_INT8)
	else if (outType == OUT_INT16)
	    ZERO_LOOP(2, 1, DITHER, NO_XLATE, PIXEL_1, ONE_XLATE, STORE_INT16)
	else if (outType == OUT_INT24)
	    ZERO_LOOP(3, 1, DITHER, NO_XLATE, PIXEL_1, ONE_XLATE, STORE_INT24)
	else 
	    ZERO_LOOP(4, 1, DITHER, NO_XLATE, PIXEL_1, ONE_XLATE, STORE_INT32);
    }
    else if (translation->translationType == GrayStatic)
    {
	unsigned long *rtable = translation->rtable;
	unsigned long *gtable = translation->gtable;
	unsigned long *btable = translation->btable;

	GAMMA;
	THREE_XLATE;
	PIXEL_SUM;

	if (outType == OUT_INT8)
	    ZERO_LOOP(1, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT8)
	else if (outType == OUT_INT16)
	    ZERO_LOOP(2, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT16)
	else if (outType == OUT_INT24)
	    ZERO_LOOP(3, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT24)
	else 
	    ZERO_LOOP(4, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT32);
    }
    else if (translation->translationType == GrayMapped)
    {
	unsigned long *rtable = translation->rtable;
	unsigned long *gtable = translation->gtable;
	unsigned long *btable = translation->btable;
	unsigned long *table  = translation->table;

	GAMMA;
	THREE_XLATE;
	PIXEL_SUM;
	ONE_XLATE;

	if (outType == OUT_INT8)
	    ZERO_LOOP(1, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT8)
	else if (outType == OUT_INT24)
	    ZERO_LOOP(3, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT24)
	else if (outType == OUT_INT16)
	    ZERO_LOOP(2, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT16)
	else 
	    ZERO_LOOP(4, 0, NO_DITHER, NO_XLATE,
		DIRECT_PIXEL_1, NO_XLATE, STORE_INT32);
    }
    else if (translation->translationType == ColorStatic)
    {
	unsigned long *rtable = translation->rtable;
	unsigned long *gtable = translation->gtable;
	unsigned long *btable = translation->btable;

	GAMMA;

	if (outType == OUT_INT8)
	    ZERO_LOOP(1, 1, DITHER, THREE_XLATE,
		PIXEL_SUM, NO_XLATE, STORE_INT8)
	else if (outType == OUT_INT16)
	    ZERO_LOOP(2, 1, DITHER, THREE_XLATE,
		PIXEL_SUM, NO_XLATE, STORE_INT16)
	else if (outType == OUT_INT24)
	    ZERO_LOOP(3, 1, DITHER, THREE_XLATE,
		PIXEL_SUM, NO_XLATE, STORE_INT24)
	else 
	    ZERO_LOOP(4, 1, DITHER, THREE_XLATE,
		PIXEL_SUM, NO_XLATE, STORE_INT32);
    }
    else if (translation->translationType == ThreeMap)
    {
	unsigned long *rtable = translation->rtable;
	unsigned long *gtable = translation->gtable;
	unsigned long *btable = translation->btable;


	if (outType == OUT_INT8)
	{
	    GAMMA;
	    ZERO_LOOP(1, 1, DITHER, THREE_XLATE,
				    PIXEL_3, NO_XLATE, STORE_INT8)
	}
	else if (outType == OUT_INT16)
	{
	    GAMMA;
	    ZERO_LOOP(2, 1, DITHER, THREE_XLATE,
				    PIXEL_3, NO_XLATE, STORE_INT16)
	}
	else if (outType == OUT_INT24)
	{
	    GAMMA;
	    ZERO_LOOP(3, 1, DITHER, THREE_XLATE,
				    PIXEL_3, NO_XLATE, STORE_INT24)
	}
	else if (outType == OUT_INT32)
	{
	    GAMMA;
	    THREE_XLATE;
	    PIXEL_3;
	    ZERO_LOOP(4, 0, NO_DITHER, NO_XLATE,
				    DIRECT_PIXEL_1, NO_XLATE, STORE_INT32)
	}
	else
	{
	    DXSetError(ERROR_INTERNAL, 
		"outType is invalid for OneMap translation");
	}
    }

    return image;

error:
    return NULL;
}

#define BITSPERXCOLOR 16

#define PIXELTOREDINDEX(pixel)   (((pixel) & rMask) >> rShift)
#define PIXELTOGREENINDEX(pixel) (((pixel) & gMask) >> gShift)
#define PIXELTOBLUEINDEX(pixel)  (((pixel) & bMask) >> bShift)

#define REDINDEXTOPIXEL(index)   (((index) << rShift) & rMask)
#define GREENINDEXTOPIXEL(index) (((index) << gShift) & gMask)
#define BLUEINDEXTOPIXEL(index)  (((index) << bShift) & bMask)

#define	COLORTOINDEX(color)      ((color) >> (BITSPERXCOLOR - usefulBits))
#define	INDEXTOCOLOR(index,color) 					      \
{									      \
    int shift;								      \
									      \
    for (shift=(BITSPERXCOLOR-usefulBits),color=0;shift>=0;shift-=usefulBits) \
        color |= ((index) << shift); 					      \
									      \
    /*									      \
     * if usefulBits is not a power if two, get the last color bits   	      \
     */									      \
    if (shift)								      \
      color |= ((index) >> -shift);				      	      \
}

static Error getThreeMapTranslation(Display *dpy, translationT *trans);
static Error getStaticGrayTranslation(Display *dpy, translationT *trans);
static Error getStaticColorTranslation(Display *dpy, translationT *trans);
static Error getGrayMappedTranslation(Display *dpy, translationT *trans, int force);
static Error getOneMapTranslation(Display *dpy, translationT *trans, int force);
static Error getDirectTranslation(Display *dpy, translationT *trans);

static Error getPrivateColormap(Display *d, translationT *t, Colormap c);

static Error
delete_translation(Pointer p)
{
    translationT *d = (translationT *)p;
    DXDebug("X", "closing service connection");

    if (! d->dpy_object)
    {
	DXSetError(ERROR_INTERNAL,
		"null display object in delete_translation");
	return ERROR;
    }

    if (d->cmap && d->ownsCmap &&
	(Colormap)d->cmap != XDefaultColormap(d->dpy, XDefaultScreen(d->dpy)))
      XFreeColormap((Display *)d->dpy,(Colormap)d->cmap);

    DXDelete((Object)d->dpy_object);

    DXFree(p);
    return OK;
}

static int
getShift(unsigned long mask)
{
  int	inc;
  for(inc=0;inc< 32 && (mask|(1<<inc))!=mask;inc++){};

  return inc;
}

static int
getUsefulBits(unsigned long mask)
{
  int	start,end;
  for(start=0;start< 32 && (mask|(1<<start))!=mask;start++){};
  for(end=start;end< 32 && (mask|(1<<end))==mask;end++){};

  return end-start;
}

/* 
 * the following defines the list of acceptable visuals
 */
static  struct visualLookupS  { 
  int depth; 
  int class; 
#ifdef FORCE_VISUAL
}  aVis[32], _aVis[] = {
#else
}  aVis[] = {
#endif
  {  4, PseudoColor },
  {  4, StaticColor },
  {  4, GrayScale   },
  {  4, StaticGray  },
  {  8, PseudoColor },
  {  8, StaticColor },
  {  8, TrueColor },
  {  8, DirectColor },
  {  8, GrayScale   },
  {  8, StaticGray  },
  { 12, PseudoColor },
  { 12, TrueColor   },
  { 12, DirectColor },
  { 15, TrueColor   },
  { 16, TrueColor   },
  { 24, DirectColor },
  { 24, TrueColor   },
  { 32, DirectColor },
  { 32, TrueColor   },
  { -1, -1          }
};

#ifdef FORCE_VISUAL
static struct classes
{
  int   class;
  char  *name;
} Classes[] = {
  { PseudoColor, "PseudoColor" },
  { StaticColor, "StaticColor" },
  { GrayScale,   "GrayScale"   },
  { StaticGray,  "StaticGray"  },
  { TrueColor,   "TrueColor"   },
  { DirectColor, "DirectColor" },
  { -1, NULL}
};
#endif
 


static Visual *
getBestVisual (Display *dpy, int *depth, int *directMap)
{
    int	  	  count,i,j;
    XVisualInfo	  *visualInfo=NULL,template;
    Visual	  *visual,*ret=NULL;
    int	          screen = XDefaultScreen(dpy);

#ifdef FORCE_VISUAL
    {
	char *str;
	int  depth;

	str = (char *)getenv("DXVISUAL");
	if (str)
	{
	    if (1 != sscanf(str, "%d", &depth))
	    { 
		DXSetError(ERROR_INTERNAL, "bad visual specification: %s", str);
		return NULL;
	    }

	    for (i = 0; str[i] && str[i] != ','; i++);

	    if (! str[i])
	    { 
		DXSetError(ERROR_INTERNAL, "bad visual specification: %s", str);
		return NULL;
	    }

	    for (j = 0; Classes[j].class != -1; j++)
		if (! strcmp(Classes[j].name, str+i+1))
		    break;

	    if (Classes[j].class == -1)
	    { 
		DXSetError(ERROR_INTERNAL, "bad visual specification: %s", str);
		return NULL;
	    }

	    aVis[0].depth = depth;
	    aVis[0].class = Classes[j].class;
	    aVis[1].depth = -1;
	    aVis[1].class = -1;
	}
	else
	{
	    for (i = 0; _aVis[i].depth != -1; i++)
		aVis[i] = _aVis[i];
	    aVis[i].depth = -1;
	    aVis[i].class = -1;
	}
    }
#endif

    if (*directMap)
    {
        int mask;
  
        template.colormap_size = 256;
        mask = VisualColormapSizeMask;
  
        visualInfo = XGetVisualInfo(dpy, mask, &template, &count);
  
        if (count)
        {
	    for (i = 0; !ret && i < count; i++)
	    {
		/*
		 * In order of preference... first
		 * look for a PseudoColor visual, then a
		 * DirectColor visual, then a GrayScale visual,
		 * finally a StaticGray.
		 */
	        if (visualInfo[i].class == PseudoColor &&
		    (*depth == 0 || visualInfo[i].depth == *depth))
	        {
		    *depth = visualInfo[i].depth;
		    ret = visualInfo[i].visual;
		}

	        if (!ret && visualInfo[i].class == GrayScale &&
		    (*depth == 0 || visualInfo[i].depth == *depth))
	        {
		    *depth = visualInfo[i].depth;
		    ret = visualInfo[i].visual;
		}
	    }

	    if(visualInfo)
	        XFree ((void *)visualInfo);
	}
	    
	if (ret)
	    return ret;
    }

    /*
     * If we couldn't find a PseudoColor or GrayScale visual, then we won't
     * be direct mapping
     */
    *directMap = 0;


    /*
     * If a depth was requested
     *	see if the default depth matches the requested one,
     * 	then try any visual of that depth.  Only use the default
     *  if its a color type.
     */
    if(*depth)
    {
        if(*depth == XDefaultDepth(dpy, screen))
	{
	    visual = XDefaultVisual(dpy,screen);
	    if (visual->class == PseudoColor ||
		visual->class == DirectColor ||
		visual->class == TrueColor   ||
		visual->class == StaticColor)
	    {

		for (i = 0; aVis[i].depth != -1; i++)
		    if ((*depth == aVis[i].depth) &&
			(visual->class == aVis[i].class))
		{
		    return visual;
		}
	    }
	}

	/*
	 * Get all the available visuals of the requested depth
	 */
	template.depth = *depth;
	visualInfo = XGetVisualInfo(dpy,VisualDepthMask,&template,&count);

	/*
	 * Look through them for a color visual 
	 */
	for (i = 0; !ret && aVis[i].depth != -1; i++)
	    if (aVis[i].class == PseudoColor ||
		aVis[i].class == DirectColor ||
		aVis[i].class == TrueColor   ||
		aVis[i].class == StaticColor) 
	    {
		for (j = 0; !ret && j < count; j++)
		    if ((visualInfo[j].depth == aVis[i].depth) &&
			(visualInfo[j].class == aVis[i].class))
		    {
			ret = visualInfo[j].visual;
		    }
	    }

	/*
	 * If no color visual of the requested depth was found
	 * look for *any* visual of that depth
	 */
	for (i = 0; !ret && aVis[i].depth != -1; i++)
	    for (j = 0; !ret && j < count; j++)
		if ((visualInfo[j].depth == aVis[i].depth) &&
		    (visualInfo[j].class == aVis[i].class))
		{
		    ret = visualInfo[j].visual;
		}

	if (visualInfo)
	    XFree ((void *)visualInfo);

	visualInfo = NULL;
	if (ret)
	    return ret;
	else 
	    DXWarning("#11620",*depth);
    }

      
    /* If no depth given, or depth was invalid,
     *	then try the default (if its color)
     *	then 8 bit color,
     *	then any color visual
     *	then any acceptable visual
     *
     * See if the default visual is color and is present on the
     * list of acceptable visuals
     */
    *depth = XDefaultDepth(dpy,XDefaultScreen(dpy));
    visual =  XDefaultVisual(dpy,XDefaultScreen(dpy));

    if (visual->class == PseudoColor ||
	visual->class == DirectColor ||
	visual->class == TrueColor   ||
	visual->class == StaticColor)
    {
	for (i = 0; aVis[i].depth != -1; i++)
	    if ((*depth == aVis[i].depth) &&
		(visual->class == aVis[i].class))
	    {
		DXMessage("#2000",*depth);
		return visual;
	    }
    }

    /*
     * Get all the available 8-bit visuals
     */
    template.depth = *depth = 8;
    visualInfo = XGetVisualInfo(dpy,VisualDepthMask,&template,&count);

    /*
     * Look for an acceptable color 8-bit visual
     */
    for (i = 0; !ret && aVis[i].depth != -1; i++)
	if (aVis[i].class == PseudoColor ||
	    aVis[i].class == DirectColor ||
	    aVis[i].class == TrueColor   ||
	    aVis[i].class == StaticColor) 
	{
	    for (j = 0; !ret && j < count; j++)
		if ((visualInfo[j].depth == aVis[i].depth) &&
		    (visualInfo[j].class == aVis[i].class))
		{
		    ret = visualInfo[j].visual;
		}
	}

    if (visualInfo)
	XFree ((void *)visualInfo);

    visualInfo = NULL;

    if (ret) 
    {
	DXMessage("#2000",*depth);
	return ret;
    }

    /*
     * get *all* visuals
     */
    visualInfo = XGetVisualInfo(dpy,0,&template,&count);

    /*
     * look for any acceptable color visual
     */
    for (i = 0; !ret && aVis[i].depth != -1; i++)
	if (aVis[i].class == PseudoColor ||
	    aVis[i].class == DirectColor ||
	    aVis[i].class == TrueColor   ||
	    aVis[i].class == StaticColor) 
	{
	    for (j = 0; !ret && j < count; j++)
		if ((visualInfo[j].depth == aVis[i].depth) &&
		    (visualInfo[j].class == aVis[i].class))
		{
		    *depth = visualInfo[j].depth; 
		    ret = visualInfo[j].visual;
		}
	}

    /*
     * look for any acceptable visual (even grayscale or staticgray)
     */
    for (i = 0; !ret && aVis[i].depth != -1; i++)
	for (j = 0; !ret && j < count; j++)
	    if ((visualInfo[j].depth == aVis[i].depth) &&
		(visualInfo[j].class == aVis[i].class))
	    {
		*depth = visualInfo[j].depth; 
		ret = visualInfo[j].visual;
	    }

    if (visualInfo)
	XFree ((void *)visualInfo);

    visualInfo = NULL;

    if (ret)
    {
	DXMessage("#2000",*depth);
	return ret;
    }
    else 
	DXSetError(ERROR_BAD_PARAMETER,"#11720");
  
    return NULL;
}

static Object
createTranslation(Private dpy_object, char *where,
		int desiredDepth, int *directMap, struct window *w)
{
    char 		cacheid[100];
    Private 		p = NULL;
    translationT 	*trans;
    Colormap 		xcmap = BadColor; /* Correct Initialization */
    Visual		*visual = NULL;
    int   		dscrn;
    Window		droot;
    int			cached = 0;
    XWindowAttributes   xwa;
    Display 		*dpy;
    XImage		*dummyXImage;

    if (! dpy_object)
    {
        DXSetError(ERROR_INTERNAL,
	    "no display object passed into createTranslation");
        goto error;
    }

    dpy = getDisplay(dpy_object);

    if (!where || !*where)
        where = (char *)getenv("DISPLAY");
    if (!where)
#if defined(intelnt) || defined(WIN32)
        where = "";
#else
        where = "unix:0";
#endif

    sprintf(cacheid, "X%s%d", where, desiredDepth);

    /*
     * If we need to create a translation...
     */
    if (! p)
    {
        trans = (translationT *)DXAllocateZero(sizeof(translationT));
        if (! trans)
            goto error;
    
	trans->dpy_object = (Private)DXReference((Object)dpy_object);
	trans->dpy = dpy;

	p = DXNewPrivate((Pointer)trans, delete_translation);
	if (! p)
	{
	    DXFree((Pointer)trans);
	    goto error;
	}

	DXReference((Object)p);

	dscrn = XDefaultScreen(dpy);
	droot = XRootWindow(dpy, dscrn);

	trans->depth = desiredDepth;
	trans->invertY = 1;		/* all SW images are inverted */

	/*
	 * If its not a UI window and its depth is wrong ro a direct
	 * mapping is requested and this window can't handle it, delete the
	 * window.
	 */
	/* #X was
	 * if (w && w->winFlag && !w->ui &&
	 */
	if (w && w->winFlag && w->windowMode == WINDOW_LOCAL &&
	    ((!w->translation) ||
	     (w->translation->depth != desiredDepth && desiredDepth != 0) ||
	     ((*directMap == 1) && w->translation->translationType != NoTranslation) ||
	     ((*directMap == 0) && w->translation->translationType == NoTranslation)))
	{
	    if (w->gc)
	    {
		XFreeGC(dpy, w->gc);
		w->gc = NULL;
	    }

	    if (w->winFlag)
	    {
		XDestroyWindow(dpy, w->wid);
		w->wid = 0;
		w->winFlag = 0;
	    }
	}

	if (w && w->winFlag)
	{
	    XGetWindowAttributes(dpy, w->wid, &xwa);
	    visual = xwa.visual;
	    trans->depth = xwa.depth;

	    if (xwa.depth != 8 || xwa.visual->class != PseudoColor)
		*directMap = 0;
	}
	else
	{
	    visual = getBestVisual(dpy, &desiredDepth, directMap); 
	    if (!visual)
		goto error;
	    trans->depth  = desiredDepth;
	}

	trans->visual = visual;

	/*
	 * This is the only way I can think of to get the
	 * bytes per pixel...
	 */
	dummyXImage = XCreateImage(dpy, visual, trans->depth,
				ZPixmap, 0, NULL, 1, 1, 8, 0);
	trans->bytesPerPixel = dummyXImage->bits_per_pixel >> 3;
	XDestroyImage(dummyXImage);

	/*
	 * if there's a custom colormap or this isn't the default visual,
	 * then we need to create a colormap.  Otherwise, we use the
	 * default colormap (either a pseudo/static or a direct/true)
	 * and create appropriate translation tables.
	 */
	if (visual != XDefaultVisual(dpy, dscrn) || (*directMap &&
		(visual->class == PseudoColor || visual->class == GrayScale) &&
		trans->depth >= 8))
	{
	    if (w && w->winFlag && xwa.colormap && !(*directMap))
	    {
		xcmap = xwa.colormap;
		trans->ownsCmap = 0;
	    }
	    else
	    {
		xcmap = XCreateColormap(dpy, droot, visual, AllocNone);
		trans->ownsCmap = 1;
	    }
	}
	else
	{
	    xcmap = XDefaultColormap(dpy, dscrn);
	    trans->ownsCmap = 1;
	}

	if (! xcmap)
	    goto error;

	trans->cmap = xcmap;

	if (*directMap && xcmap != XDefaultColormap(dpy, dscrn)
		&& ((visual->class == PseudoColor  || visual->class == GrayScale)
		&& trans->depth >= 8))
	{
	    if (! getDirectTranslation(dpy, trans))
		goto error;
	}
	else if (visual->class == DirectColor)
	{
	    if (! getThreeMapTranslation(dpy, trans))
		goto error;
	}
	else if (visual->class == PseudoColor)
	{
	    if (!getOneMapTranslation(dpy, trans, visual->class == StaticColor))
	    {
		if (DXGetError() != ERROR_NONE)
		    goto error;

		if (w && w->winFlag && xwa.colormap)
		    xcmap = XCreateColormap(dpy, w->wid, visual, AllocNone);
		else
		    xcmap = XCreateColormap(dpy, droot, visual, AllocNone);

		if (! xcmap)
		{
		    DXSetError(ERROR_INTERNAL, 
			"unable to create private colormap");
		    goto error;
		}
		 
		if (! getPrivateColormap(dpy, trans, xcmap))
		{
		    if (DXGetError() == ERROR_NONE)
			DXSetError(ERROR_INTERNAL, 
			    "unable to create private cmap and translation");
		    goto error;
		}

		if (trans->cmap != XDefaultColormap(dpy, dscrn))
		  XFreeColormap(dpy, trans->cmap);
		
		trans->cmap = xcmap;
		trans->ownsCmap = 1;
	    }
	}
	else if (visual->class == GrayScale)
	{
	    if (! getGrayMappedTranslation(dpy, trans, 1))
		goto error;
	}
	else if (visual->class == StaticGray)
	{
	    if (! getStaticGrayTranslation(dpy, trans))
		goto error;
	}
	else if (visual->class == StaticColor || visual->class == TrueColor)
	{
	    if (! getStaticColorTranslation(dpy, trans))
		goto error;
	}
	else 
	{
	    DXSetError(ERROR_INTERNAL, "unknown visual class");
		goto error;
	}
    }

    return (Object)p;

error:    
  
    if (! cached)
        DXDelete((Object)p);
    else
        DXSetCacheEntry(NULL, CACHE_PERMANENT, cacheid, 0, 0);

    return NULL;
}

static Error getGamma(int depth, float *gamma)
{
    char		*str = NULL;

    switch(depth)
    {
	case 8:  str = getenv("DXGAMMA_8BIT");  break;
	case 12: str = getenv("DXGAMMA_12BIT"); break;
	case 15: str = getenv("DXGAMMA_15BIT"); break;
	case 16: str = getenv("DXGAMMA_16BIT"); break;
	case 24: str = getenv("DXGAMMA_24BIT"); break;
	case 32: str = getenv("DXGAMMA_32BIT"); break;
    };

    if (str == NULL) str = getenv("DXGAMMA");

    if (str)
    {
	if (sscanf(str, "%f", gamma) != 1)
            *gamma = 0; 
    }

    if (!str || *gamma == 0) {
        switch(depth)
        {
	    case 8:  *gamma = DXD_GAMMA_8BIT;  break;
	    case 12: *gamma = DXD_GAMMA_12BIT; break;
	    case 15: *gamma = DXD_GAMMA_15BIT; break;
	    case 16: *gamma = DXD_GAMMA_16BIT; break;
	    case 24: *gamma = DXD_GAMMA_24BIT; break;
	    case 32: *gamma = DXD_GAMMA_32BIT; break;
        };
    }

    return OK;
}

#define COFFSET	1	/* offset to first color in colors[], this allows
			 * us to avoid having a 0 colors index
			 */
static Error
getDirectTranslation(Display *dpy, translationT *t)
{ 
    float		invgamma;
    int			i;
    unsigned long 	pixels[256];

    if (! getGamma(t->depth, &(t->gamma)))
        return ERROR;

    invgamma = 1.0 / t->gamma;

    t->translationType = NoTranslation;
    t->table  = NULL;
    t->rtable = NULL;
    t->gtable = NULL;
    t->btable = NULL;

    for (i=0; i<256; i++)
    {
	float f = i / 255.0;
        t->gammaTable[i] = 255*pow(f, invgamma);
    }

    if (! XAllocColorCells(dpy, (Colormap)t->cmap, True, NULL, 0, pixels, 256))
	return ERROR;

    return OK;
}

static Error
getThreeMapTranslation(Display *dpy, translationT *d)
{
  Visual 		*visual = (Visual *)d->visual;
  Colormap		cmap    = (Colormap)d->cmap;
  int			i,n,saveI;
  int	 		cmapsize;
  unsigned long		planes;
  int			nn;
  int			usefulBits;
  unsigned char 	colorIndex;
  unsigned char 	pixelIndex;
  XColor		colors[MAXRGBCMAPSIZE+COFFSET];
  unsigned long		pixels[MAXRGBCMAPSIZE];
  unsigned char		save[MAXRGBCMAPSIZE];
  unsigned long		rwriteable[MAXRGBCMAPSIZE];
  unsigned long		gwriteable[MAXRGBCMAPSIZE];
  unsigned long		bwriteable[MAXRGBCMAPSIZE];
  unsigned long		rMask,gMask,bMask;
  int			rShift,gShift,bShift;
  float 		invgamma;

  if (! getGamma(d->depth, &(d->gamma)))
    goto error;

  invgamma = 1.0 / d->gamma;
    
  for (i=0; i<256; i++)
  {
    float f = i / 255.0;
    d->gammaTable[i] = 255*pow(f, invgamma);
  }

  /*
   * Get info on each channel of the r,g,b color map
   */
  rMask  = visual->red_mask;
  rShift = getShift(rMask);
  gMask  = visual->green_mask;
  gShift = getShift(gMask);
  bMask  = visual->blue_mask;
  bShift = getShift(bMask);

  /*
   * XXX we should be able to get the next two from visual->bits_per_rgb
   * and visual->map_entries respectively, but the first is wrong on the
   * indigo and the other is wrong on the HP. So we'll calculate
   * intelligent values here.
   */
  usefulBits =  getUsefulBits(rMask);
  cmapsize   =  1 << usefulBits;
  cmapsize   = cmapsize > MAXRGBCMAPSIZE ? MAXRGBCMAPSIZE : cmapsize;

  /*
   * Get info on each channel of the r,g,b color map
   */
  rMask  = visual->red_mask;
  rShift = getShift(rMask);
  gMask  = visual->green_mask;
  gShift = getShift(gMask);
  bMask  = visual->blue_mask;
  bShift = getShift(bMask);

  /*
   * XXX we should be able to get the next two from visual->bits_per_rgb
   * and visual->map_entries respectively, but the first is wrong on the
   * indigo and the other is wrong on the HP. So we'll calculate
   * intelligent values here.
   */
  usefulBits =  getUsefulBits(rMask);
  cmapsize   =  1 << usefulBits;
  cmapsize   = cmapsize > MAXRGBCMAPSIZE ? MAXRGBCMAPSIZE : cmapsize;

  d->usefulBits = usefulBits;
  d->redRange = d->blueRange = d->greenRange = cmapsize;
  d->translationType = ThreeMap;
  d->rtable = &d->data[0];
  d->gtable = &d->data[MAXRGBCMAPSIZE];
  d->btable = &d->data[MAXRGBCMAPSIZE*2];

  d->table = NULL;
    
  /*
   * Allocate as many color cells are we can
   */

  if (XAllocColorCells(dpy, cmap, 0, NULL, 0, pixels, cmapsize))
  {
      nn = cmapsize;
  }
  else
  {
      for (n=cmapsize, nn=0; n>0; n/=2) 
	while (XAllocColorPlanes(dpy, cmap, False, pixels+nn,n,
				 0,0,0,&planes,&planes,&planes)) 
        {
	    DXDebug("X","Allocated %d",n);
	    nn += n;
	}
  }

  if (nn == cmapsize)
  {
      unsigned char redI,greenI,blueI;

      for (n = 0; n < nn; n++)
      {
	  redI   = PIXELTOREDINDEX(pixels[n]);
	  greenI = PIXELTOGREENINDEX(pixels[n]);
	  blueI  = PIXELTOBLUEINDEX(pixels[n]);

	  INDEXTOCOLOR(redI,   colors[n].red);
	  INDEXTOCOLOR(greenI, colors[n].green);
	  INDEXTOCOLOR(blueI,  colors[n].blue);

	  colors[n].flags = DoRed | DoGreen | DoBlue;
	  colors[n].pixel = pixels[n];
	  
	  d->rtable[n] = n;
	  d->gtable[n] = n;
	  d->btable[n] = n;
      }

      XStoreColors(dpy, cmap, colors, nn);
  }
  else
  {

      /* 
       * set defaults for the allocated colors
       * .. and set defaults for the translation tables.
       */
      bzero(rwriteable,MAXRGBCMAPSIZE * sizeof (unsigned long));
      bzero(gwriteable,MAXRGBCMAPSIZE * sizeof (unsigned long));
      bzero(bwriteable,MAXRGBCMAPSIZE * sizeof (unsigned long));
      for(n=0;n<nn;n++)
	{
	  unsigned char redI,greenI,blueI;

	  redI = PIXELTOREDINDEX(pixels[n]);
	  greenI = PIXELTOGREENINDEX(pixels[n]);
	  blueI = PIXELTOBLUEINDEX(pixels[n]);
	  colors[n].pixel = pixels[n];
	  INDEXTOCOLOR(redI,colors[n].red);
	  INDEXTOCOLOR(greenI,colors[n].green);
	  INDEXTOCOLOR(blueI,colors[n].blue);
	  colors[n].flags = DoRed | DoGreen | DoBlue;
	  rwriteable[redI]   = colors[n].pixel;
	  gwriteable[greenI] = colors[n].pixel;
	  bwriteable[blueI]  = colors[n].pixel;
	}
      /*
       * Store the defaults
       */
      if(nn)
	XStoreColors(dpy, cmap, colors, nn);

      /* query the colors */
      for (n=COFFSET; n<cmapsize+COFFSET; n++) {
	colors[n].pixel = REDINDEXTOPIXEL(n-COFFSET) |
			  GREENINDEXTOPIXEL(n-COFFSET) |
			  BLUEINDEXTOPIXEL(n-COFFSET);
      }
      XQueryColors(dpy, (Colormap)cmap, &colors[COFFSET] , cmapsize);

      /*
       * Set up translation table, save any duplicate color values
       */
      /* red */
      bzero(d->rtable,MAXRGBCMAPSIZE * sizeof(unsigned long));
      saveI = 0;
      for(n=COFFSET;n<cmapsize+COFFSET;n++)
	{
	  colorIndex = COLORTOINDEX(colors[n].red);
	  pixelIndex = PIXELTOREDINDEX(colors[n].pixel);

	  /*
	   * if we have no tranlation for this color,
	   * set the translation...
	   */
	  if(!d->rtable[colorIndex]) {
	    d->rtable[colorIndex] = n;
	  } else {
	    /* if this translation is already assigned, we have a duplicate
	     * color in the X color table. If one of them is writeable, save it,
	     * we'll use it for a color that is missing from the X color table
	     */
	    if(rwriteable[pixelIndex])
	      save[saveI++] = n;
	    else 
	      if(rwriteable[PIXELTOREDINDEX(colors[d->rtable[colorIndex]].pixel)]) {
		save[saveI++] = d->rtable[colorIndex];
		d->rtable[colorIndex] = n;
	      }
	  }
	}
      /*
       * find any unassigned rtable entries and assign them using saved pixels
       */
      for(n=0;n<cmapsize;n++) {
	if(!d->rtable[n]) {
	  if(saveI) {
	    unsigned long tmpPixel;

	    saveI--;
	    pixelIndex = save[saveI];
	    d->rtable[n] = pixelIndex;
	    /* Use one of the allocated pixel value to set the new color */
	    tmpPixel = colors[pixelIndex].pixel;
	    colors[pixelIndex].pixel = 
	      rwriteable[PIXELTOREDINDEX(colors[pixelIndex].pixel)];
	    colors[pixelIndex].flags = DoRed;
	    INDEXTOCOLOR(n,colors[pixelIndex].red);
	    XStoreColor(dpy,cmap,&colors[pixelIndex]);
	    colors[pixelIndex].pixel = tmpPixel;
	  } else {
	    /* if no more available saved colors, approximate the color
	     * by using a close neighbor. (This sould only happen if we
	     * have duplicate r/o entries in the X color table).
	     */
	    if(n) 
	      d->rtable[n] = d->rtable[n-1];
	    else {
	      for(i=0;i<cmapsize;i++)
		if(d->rtable[i]) {
		  d->rtable[n] = d->rtable[i];
		  break;
		}
	    DXWarning("#11640");
	    }
	  }
	}
      }
      /* green */
      bzero(d->gtable,MAXRGBCMAPSIZE * sizeof(unsigned long));
      saveI = 0;
      for(n=COFFSET;n<cmapsize+COFFSET;n++)
	{
	  colorIndex = COLORTOINDEX(colors[n].green);
	  pixelIndex = PIXELTOGREENINDEX(colors[n].pixel);

	  /* if we have no tranlation for this color, set the translation... */
	  if(!d->gtable[colorIndex]) {
	    d->gtable[colorIndex] = n;
	  } else {
	    /* if this translation is already assigned, we have a duplicate
	     *  color in the X color table. If one of them is writeable, save it,
	     *  we'll use it for a color that is missing from the X color table
	     */
	    if(gwriteable[pixelIndex])
	      save[saveI++] = n;
	    else 
	      if(gwriteable[PIXELTOGREENINDEX(colors[d->gtable[colorIndex]].pixel)]) {
		save[saveI++] = d->gtable[colorIndex];
		d->gtable[colorIndex] = n;
	      }
	  }
	}
      /*
       * find any unassigned gtable entries and assign them using saved pixels
       */
      for(n=0;n<cmapsize;n++) {
	if(!d->gtable[n]) {
	  if(saveI) {
	    unsigned long tmpPixel;

	    saveI--;
	    pixelIndex = save[saveI];
	    d->gtable[n] = pixelIndex;
	    /* Use one of the allocated pixel value to set the new color */
	    tmpPixel = colors[pixelIndex].pixel;
	    colors[pixelIndex].pixel = 
	      gwriteable[PIXELTOGREENINDEX(colors[pixelIndex].pixel)];
	    colors[pixelIndex].flags = DoGreen;
	    INDEXTOCOLOR(n,colors[pixelIndex].green);
	    XStoreColor(dpy,cmap,&colors[pixelIndex]);
	    colors[pixelIndex].pixel = tmpPixel;
	  } else {
	    /* if no more available saved colors, approximate the color
	     * by using a close neighbor. (This sould only happen if we
	     * have duplicate r/o entries in the X color table).
	     */
	    if(n) 
	      d->gtable[n] = d->gtable[n-1];
	    else {
	      for(i=0;i<cmapsize;i++)
		if(d->gtable[i]) {
		  d->gtable[n] = d->gtable[i];
		  break;
		}
	    DXWarning("#11640");
	    }
	  }
	}
      }
      /* blue */
      bzero(d->btable,MAXRGBCMAPSIZE * sizeof(unsigned long));
      saveI = 0;
      for(n=COFFSET;n<cmapsize+COFFSET;n++)
	{
	  colorIndex = COLORTOINDEX(colors[n].blue);
	  pixelIndex = PIXELTOBLUEINDEX(colors[n].pixel);

	  /* if we have no tranlation for this color, set the translation... */
	  if(!d->btable[colorIndex]) {
	    d->btable[colorIndex] = n;
	  } else {
	    /* if this translation is already assigned, we have a duplicate
	     *  color in the X color table. If one of them is writeable, save it,
	     *  we'll use it for a color that is missing from the X color table
	     */
	    if(bwriteable[pixelIndex])
	      save[saveI++] = n;
	    else 
	      if(bwriteable[PIXELTOBLUEINDEX(colors[d->btable[colorIndex]].pixel)]) {
		save[saveI++] = d->btable[colorIndex];
		d->btable[colorIndex] = n;
	      }
	  }
	}
      /*
       * find any unassigned btable entries and assign them using saved pixels
       */
      for(n=0;n<cmapsize;n++) {
	if(!d->btable[n]) {
	  if(saveI) {
	    unsigned long tmpPixel;

	    saveI--;
	    pixelIndex = save[saveI];
	    d->btable[n] = pixelIndex;
	    /* Use one of the allocated pixel value to set the new color */
	    tmpPixel = colors[pixelIndex].pixel;
	    colors[pixelIndex].pixel = 
	      bwriteable[PIXELTOBLUEINDEX(colors[pixelIndex].pixel)];
	    colors[pixelIndex].flags = DoBlue;
	    INDEXTOCOLOR(n,colors[pixelIndex].blue);
	    XStoreColor(dpy,cmap,&colors[pixelIndex]);
	    colors[pixelIndex].pixel = tmpPixel;
	  } else {
	    /* if no more available saved colors, approximate the color
	     * by using a close neighbor. (This sould only happen if we
	     * have duplicate r/o entries in the X color table).
	     */
	    if(n) 
	      d->btable[n] = d->btable[n-1];
	    else {
	      for(i=0;i<cmapsize;i++)
		if(d->btable[i]) {
		  d->btable[n] = d->btable[i];
		  break;
		}
	    DXWarning("#11640");
	    }
	  }
	}
      }
      for (n=COFFSET; n<cmapsize+COFFSET; n++) {
	  colors[n].pixel = REDINDEXTOPIXEL(n-COFFSET) |
			    GREENINDEXTOPIXEL(n-COFFSET) |
			    BLUEINDEXTOPIXEL(n-COFFSET);
      }
  }

  /*
   * Change the r,g,btable values from 'n's to the pixels values.
   */
#define SWAPSHORT(s1,s2) \
  { \
      unsigned short tmp1; \
      unsigned short tmp2 = (unsigned short) s2; \
      char *ps1=(char *)&tmp1,*ps2=(char*)&(tmp2); \
      ps1[0] = ps2[1]; \
      ps1[1] = ps2[0]; \
      s1 = (unsigned long) tmp1; \
  }

#define SWAPLONG(s1,s2) \
  { \
      unsigned long tmp1; \
      unsigned long tmp2 = (unsigned long) s2; \
      char *ps1=(char *)&tmp1,*ps2=(char*)&(tmp2); \
      ps1[0] = ps2[3]; \
      ps1[1] = ps2[2]; \
      ps1[2] = ps2[1]; \
      ps1[3] = ps2[0]; \
      s1 = (unsigned long) tmp1; \
  }
	
#if defined(WORDS_BIGENDIAN)
  if(XImageByteOrder(dpy) == MSBFirst)  
#else
  if(XImageByteOrder(dpy) == LSBFirst)
#endif
  {
    for(i=0;i<cmapsize;i++) {
       d->rtable[i] = colors[d->rtable[i]].pixel & rMask;
       d->gtable[i] = colors[d->gtable[i]].pixel & gMask;
       d->btable[i] = colors[d->btable[i]].pixel & bMask;
     }
  } else {
    if(_dxf_GetXBytesPerPixel(d) == 4) {
      for(i=0;i<cmapsize;i++) {
	SWAPLONG(d->rtable[i],(colors[d->rtable[i]].pixel & rMask));
	SWAPLONG(d->gtable[i],(colors[d->gtable[i]].pixel & gMask));
	SWAPLONG(d->btable[i],(colors[d->btable[i]].pixel & bMask));
      }
    } else if(_dxf_GetXBytesPerPixel(d) == 2) {
      for(i=0;i<cmapsize;i++) {
	SWAPSHORT(d->rtable[i],(colors[d->rtable[i]].pixel & rMask));
	SWAPSHORT(d->gtable[i],(colors[d->gtable[i]].pixel & gMask));
	SWAPSHORT(d->btable[i],(colors[d->btable[i]].pixel & bMask));
      }
    } else {
      for(i=0;i<cmapsize;i++) {
	d->rtable[i] = (colors[d->rtable[i]].pixel & rMask);
	d->gtable[i] = (colors[d->gtable[i]].pixel & gMask);
	d->btable[i] = (colors[d->btable[i]].pixel & bMask);
      }
    }
   }

#ifdef DEBUGGED
  /* query the colors for DEBUG only*/
  for (n=0; n<cmapsize; n++) {
    colors[n].pixel = REDINDEXTOPIXEL(n) |
                      GREENINDEXTOPIXEL(n) |
	              BLUEINDEXTOPIXEL(n);
  }
  XQueryColors(dpy, (Colormap)cmap, colors , cmapsize);

  DXDebug("X","Colors:");
  for(i=0;i<cmapsize;i++)
    {
      DXDebug("X","%d: %d %d %d",i, colors[i].red>>8,
	      colors[i].green>>8,
	      colors[i].blue>>8);
    }
  DXDebug("X","Tables:");
  for(i=0;i<cmapsize;i++)
    {
      DXDebug("X","%d: %d %d %d",i, 
	      PIXELTOREDINDEX(d->rtable[i]),
	      PIXELTOGREENINDEX(d->gtable[i]),
	      PIXELTOBLUEINDEX(d->btable[i]));
    }
  DXDebug("X","Looked Up:");
  for(i=0;i<cmapsize;i++)
    {
      if(COLORTOINDEX(colors[PIXELTOREDINDEX(d->rtable[i])].red) != i ||
	 COLORTOINDEX(colors[PIXELTOGREENINDEX(d->gtable[i])].green) != i ||
	 COLORTOINDEX(colors[PIXELTOBLUEINDEX(d->btable[i])].blue) != i)
	DXDebug("X","%d: %d %d %d",i,
		colors[PIXELTOREDINDEX(d->rtable[i])].red>>8,
		colors[PIXELTOGREENINDEX(d->gtable[i])].green>>8,
		colors[PIXELTOBLUEINDEX(d->btable[i])].blue>>8);
    }
#endif
    if (xerror) goto error;

#if 0
    /*
     * expand table out to 256 slots
     */
    if (cmapsize < 256)
    {
	int i, j, k;
	int dupCount = 1 << (8 - usefulBits);

        /*
         * dupCount is the number of input 0 -> 255 colors that map
         * into the available color range
         */

	for (i = cmapsize-1; i >= 0; i--)
	{
	    
	    for (j = 0, k = i*dupCount; j < dupCount; j++, k++)
	    {
		d->rtable[k] = d->rtable[i];
		d->gtable[k] = d->gtable[i];
		d->btable[k] = d->btable[i];
	    }
	}
    }
#endif
		
    return OK;

error:    
    xerror = 0;
    return ERROR;
}

struct color_sort {
    int i;		/* index in R*G*B color map */
    int j;		/* nearest index in current X color map */
    int diff;		/* difference */
};

#if DXD_OS2_SYSCALL
static int _Optlink
#else
static int
#endif
compare(const void * p1, const void * p2)
{
    const struct color_sort *a = p1;
    const struct color_sort *b = p2;
    if (a->diff<b->diff)
	return 1;
    else
	return -1;
}

/*
 * These define the number of colors in each of R,G,B that we will support
 * in the X lookup table. This means we use a total of RR*GG*BB (225) color
 * table entires.
 */
#define MAX_RR 16
#define MAX_GG 16
#define MAX_BB 16

#define COLOR_EXACT	1
#define COLOR_ALLOC	2
#define COLOR_APPROX 	3
#define COLOR_NONE	4

#define APPROX_TOLERANCE 0.1

#define HI(x) (((x)>>8)&255)

static Error
getOneMapTranslation(Display *dpy, translationT *d, int force)
{
    Visual 		*visual = (Visual *)d->visual;
    Colormap		cmap    = (Colormap)d->cmap;
    XColor 		colors[MAXCMAPSIZE];
    int 		i, j, best = 0, r, g, b, n,nn, max_nbrhd; 
    int 		comp[MAXCMAPSIZE],
			uncomp[MAXCMAPSIZE];
    unsigned char       readOnly[MAXCMAPSIZE];
    struct {unsigned char r, g, b;} map[MAX_RR*MAX_GG*MAX_BB];
    struct color_sort 	sort[MAX_RR*MAX_GG*MAX_BB];
    unsigned char       colorAssigned[MAX_RR*MAX_GG*MAX_BB];
    unsigned long	pixels[MAXCMAPSIZE];
    int 		cmapsize, xlatesize = 0;
    int			rr,gg,bb;
    float		invgamma, gamma;
    char		*str;
    int			nExact = 0, nAlloc = 0, nApprox = 0;
    int			maxdiff = 0;
    float		approximationTolerance = APPROX_TOLERANCE;
    int 		nPrivate, nShared;

    str = getenv("DX8BITCMAP");
    if (str)
	sscanf(str, "%f", &approximationTolerance);
    
    if (approximationTolerance < 0)
	return ERROR;

    approximationTolerance *= sqrt(3.0);

    if (! getGamma(d->depth, &(d->gamma)))
        goto error;

    gamma = d->gamma;
    invgamma = 1.0 / d->gamma;

    /* find out how big the cmap is */
    cmapsize = visual->map_entries;
    cmapsize = cmapsize > MAXCMAPSIZE? MAXCMAPSIZE: cmapsize;

    if (cmapsize <= 16)
    {
	rr = 2;
	gg = 3;
	bb = 2;
    }
    else if (cmapsize <= 256)
    {
	rr = 5;
	gg = 9;
	bb = 5;
    }
    else
    {
	rr = 16;
	gg = 16;
	bb = 16;
    }

    /*
     * setup the translation table mapping straight (595) color index
     * into available colormap
     */
    d->translationType = OneMap;
    d->redRange        = rr;
    d->greenRange      = gg;
    d->blueRange       = bb;
    d->table           = (unsigned long *)d->data;

    d->rtable = NULL;
    d->gtable = NULL;
    d->gtable = NULL;

    /*
     * gamma compensation; corresponds to xpic, so we can coexist w/ xpic
     */
    for (i=0; i<256; i++) {
	float f = i / 255.0;
        comp[i] = 255*pow(f, invgamma);
	uncomp[i] = 255*pow(f, gamma);
    }

    /*
     * go for it: try to allocate the entire chunk required.
     */
    xlatesize = rr * gg * bb;
    if (XAllocColorCells(dpy, cmap, 0, NULL, 0, pixels, xlatesize))
    {
	XFreeColors(dpy, cmap, pixels, xlatesize, 0);

	for (r=0; r<rr; r++)
	    for (g=0; g<gg; g++)
		for (b=0; b<bb; b++)
		{
		    unsigned short sr, sg, sb;
		    XColor c;

		    int p = MIX(r, g, b);

		    sr = comp[r*255/(rr-1)];
		    sg = comp[g*255/(gg-1)];
		    sb = comp[b*255/(bb-1)];

		    c.red   = sr << 8;
		    c.green = sg << 8;
		    c.blue  = sb << 8;

		    if (! XAllocColor(dpy, cmap, &c))
		    {
			DXDebug("X", "XAllocColor failed");
			return ERROR;
		    }

		    d->table[p] = c.pixel;
		}
	
	return OK;
    }
	    
    /*
     * the set of colors we need
     */
    for (r=0; r<rr; r++) {
        for (g=0; g<gg; g++) {
            for (b=0; b<bb; b++) {
                i = MIX(r,g,b);
                map[i].r = comp[r*255/(rr-1)];
                map[i].g = comp[g*255/(gg-1)];
                map[i].b = comp[b*255/(bb-1)];
		sort[i].i = i;
		sort[i].j = -1;
		sort[i].diff = 1000000;

		d->table[i] = -1;
            }
        }
    }

    /*
     * Find out how many unallocated memory cells there are
     */
    for (n = cmapsize, nn = 0; n > 0; n /= 2)
    {
	while (XAllocColorCells(dpy, cmap, False, NULL, 0, pixels+nn, n))
	    nn += n;
    }

    if (nn)
	XFreeColors(dpy, cmap, pixels, nn, 0);

    /*
     * create a flag array indicating the unallocated cells
     */
    memset(readOnly, 1, cmapsize*sizeof(unsigned char));

    for (j = 0; j < nn; j++) {
	readOnly[pixels[j]] = 0;
    }

    /*
     * query the colors
     */
    for (j=0; j<cmapsize; j++)
    {
	colors[j].pixel = j;
	colors[j].flags = DoRed | DoGreen | DoBlue;
    }
    XQueryColors(dpy, cmap, colors, cmapsize);
    /*
     * Now determine which of the already-allocated cells are sharable.
     * We do this by trying to re-allocate them... if we get the same
     * pixel index, then the cell was sharable.
     */

    nPrivate = nShared = 0;
    for (i = 0; i < cmapsize; i++)
    {
	if (readOnly[colors[i].pixel])
	{
    	    XColor c;

	    c.red   = colors[i].red;
	    c.green = colors[i].green;
	    c.blue  = colors[i].blue;

	    if (XAllocColor(dpy, cmap, &c))
	    {
		if (c.pixel != colors[i].pixel)
		{
		    readOnly[colors[i].pixel] = 0;
		    nPrivate ++;
		}
		else
		    nShared ++;

	     XFreeColors(dpy, cmap, &c.pixel, 0, 0);
	    }
	    else
	    {
		nPrivate ++;
		readOnly[colors[i].pixel] = 0;
	    }
	} 
    }

    DXDebug("X", "%d private %d shared cells in colormap", nPrivate, nShared);

    /*
     * The color color allocation algorithm is based on a subdivision
     * of the color space into a rrxggxbb grid of partitions.
     * If we have enough writable colors available, each partition
     * will be assigned the color corresponding to the center of the
     * partition.  This color will be placed into an available slot
     * in the color map and the index of the slot will be retained in
     * the translation.  Then, to translate a color, the partition
     * containing the color is found, and the output pixel gets the 
     * index of the color map slot containing the color for that partition.
     *
     * If there are not enough writable slots in the color map, some 
     * partitions will have to make due with a color that was either
     * allocated read-only by another application, or was already
     * allocated for another partition.  So we will maintain a list
     * of the colors we need, sorted by the distance from the centerpoint
     * of the corresponding partition to the nearest color already in
     * the table.
     *
     * First, for each pre-allocated color, look at the 3x3x3 neighborhood
     * of partitions around the partition containing the color.  If the
     * distance from the color to the center of the nighborhood partition
     * is closer than any prior color, update the distance and index 
     * associated with the neighbor partition.
     */
    for (j=0; j<cmapsize; j++)
    {
	/*
	 * if it is pre-allocated...
	 */
	if(readOnly[j])
	{

	    int ri, gi, bi;

	    /*
	     * find the indices of the color-space partition containing
	     * the pre-allocated color.
	     */
	    r = (uncomp[HI(colors[j].red)]/255.0) * (rr-1) + 0.5;
	    g = (uncomp[HI(colors[j].green)]/255.0) * (gg-1) + 0.5;
	    b = (uncomp[HI(colors[j].blue)]/255.0) * (bb-1) + 0.5;

	    /*
	     * for each of the 3x3x3 partitions in the neighborhood...
	     */
	    for (ri = r-1; ri <= r+1; ri++)
	    {
		if (ri < 0 || ri >= rr)
		    continue;

		for (gi = g-1; gi <= g+1; gi++)
		{
		    if (gi < 0 || gi >= gg)
			continue;

		    for (bi = b-1; bi <= b+1; bi++)
		    {
			int i, dr, dg, db, diff;

			if (bi < 0 || bi >= bb)
			    continue;

			/*
			 * determine the index of the (ri, gi, bi) partition
			 */
			i = MIX(ri,gi,bi);

			/*
			 * get the euclidean distance between the 
			 * pre-allocated color and the ideal color for 
			 * this partition.
			 */
			dr = map[i].r-HI(colors[j].red);
			dg = map[i].g-HI(colors[j].green);
			db = map[i].b-HI(colors[j].blue);
			diff = dr*dr + dg*dg + db*db;

			/*
			 * if its the closest found so far...
			 */
			if (diff < sort[i].diff )
			{
			    sort[i].diff = diff;
			    sort[i].j = j;
			}

		    }
		}
	    }
	}
    }

    /*
     * sort by distance
     */
    qsort((char *)sort, xlatesize, sizeof(sort[0]), compare);

    /*
     * Initialize a flag array indicating which cells have received
     * a color.
     */
    memset(colorAssigned, COLOR_NONE, rr*gg*bb);

/*
 * The following reallocates a color we know is already in the color
 * map.  This is done in this manner so that the reference count
 * of the color cell is correct.
 */
#define REALLOCATE_COLOR(index, dst)				 \
{				 				 \
    XColor c, *src = colors + index;				 \
				 				 \
    c.red   = src->red;				 		 \
    c.green = src->green;				 	 \
    c.blue  = src->blue;				 	 \
				 				 \
    if (! XALLOCCOLOR(dpy, cmap, &c) || c.red != src->red ||	 \
	c.green != src->green || c.blue != src->blue ||		 \
	c.pixel != src->pixel)					 \
    {								 \
	DXDebug("X", "color reallocation failure");		 \
	goto error;				 		 \
    }				 				 \
				 				 \
    (dst) = src->pixel;				 		 \
}

    /*
     * allocate colors, worst match first
     */
    for (i = n = 0; i < xlatesize; i++)
    {
	int ii = sort[i].i;

	/*
	 * If diff is 0, then we saw an exact-matching pre-allocated 
	 * color.  Otherwise, if there's an available writeable colormap
	 * slot, set up an exact-matching color.  In this case, the new
	 * diff will be zero and we will place the newly allocated color
	 * into out color table.  Finally, if no writable colormap slots
	 * are available, we break out of this loop and find an approximation
	 * below.
	 */
	if (sort[i].diff == 0)
	{
	    int j = sort[i].j;
	    REALLOCATE_COLOR(j, d->table[ii]);
	    nExact ++;
	    colorAssigned[ii] = COLOR_EXACT;
	}
	else if (n < nn)
	{
	    unsigned short sr, sg, sb;
	    XColor c;

	    sr = map[ii].r;
	    sg = map[ii].g;
	    sb = map[ii].b;

	    c.red   = sr<<8;
	    c.green = sg<<8;
	    c.blue  = sb<<8;

	    if (! XALLOCCOLOR(dpy, cmap, &c))
		goto error;

	    if (HI(c.red) != sr || HI(c.green) != sg || HI(c.blue) != sb)
	    {
		DXDebug("X", "XAllocColor allocation mismatch: %x :: %x",
		    (HI(c.red)<<16) | (HI(c.green)<<8) | HI(c.blue),
		    (sr<<16) | (sg<<8) | sb);
		
		/*
		 * If we actually allocated a new cell, increment our 
		 * counter.
		 */
		if (! readOnly[c.pixel])
		{
		    readOnly[c.pixel] ++;
		    n ++;
		    nAlloc++;
		}

		colorAssigned[ii] = COLOR_APPROX;
		d->table[ii] = c.pixel;
		colors[c.pixel] = c;
	    }
	    else
	    {
		d->table[ii] = c.pixel;
		n++;
		nAlloc++;
		colorAssigned[ii] = COLOR_ALLOC;
		colors[c.pixel] = c;
	    }
	}
	else if (sort[i].diff != -1 && sort[i].diff != 1000000)
	{
	    int j = sort[i].j;
	    REALLOCATE_COLOR(j, d->table[ii]);
	    colorAssigned[ii] = COLOR_APPROX;
	}
	else
	    colorAssigned[ii] = COLOR_NONE;
    }

    /*
     * Now for the remaining required colors, we need to find the 
     * best approximation available.
     */
    if (rr < gg)
	max_nbrhd = gg;
    else
	max_nbrhd = gg;

    if (max_nbrhd < bb)
	max_nbrhd = bb;

    for (r = i = 0; r < rr; r++)
    {
	for (g = 0; g < gg; g++)
	{
	    for (b = 0; b < bb; b++, i++)
	    {
		int done = 0;
		int mindiff = DXD_MAX_INT;

		/*
		 * If the cell already has a color, OK, fine, skip it.
		 */
		if (colorAssigned[i] == COLOR_EXACT ||
		    colorAssigned[i] == COLOR_ALLOC)
		     continue;

		while (! done)
		{
		    for (n = 1; !done && n < max_nbrhd; n++)
		    {
			int ri, gi, bi;

			for (ri = r - n; ri <= r + n; ri ++)
			{
			    if (ri < 0 || ri >= rr)
				continue;
			
			    for (gi = g - n; gi <= g + n; gi++)
			    {
				if (gi < 0 || gi >= gg)
				    continue;
				
				for (bi = b - n; bi <= b + n; bi++)
				{
				    if (bi < 0 || bi >= bb)
					continue;
				    
				    /*
				     * get the index of the neighbor
				     */
				    j = MIX(ri, gi, bi);

				    if (colorAssigned[j] != COLOR_NONE)
				    {
					int dr, dg, db, diff;

					j = d->table[j];
					dr = map[i].r-HI(colors[j].red);
					dg = map[i].g-HI(colors[j].green);
					db = map[i].b-HI(colors[j].blue);
					diff = dr*dr + dg*dg + db*db;

					/*
					 * if its the closest found
					 * so far...
					 */
					if (diff < mindiff)
					{
					    mindiff = diff;
					    best = j;
					}

					/*
					 * continue to look thorugh
					 * the neighboring partitions
					 * at this distance, but no
					 * farther out.
					 */
					done = 1;
				    }
				}
			    }
			}
		    }
		}

		if (colorAssigned[i] == COLOR_APPROX)
		    XFREECOLOR(dpy, cmap, d->table[i]);
		
		REALLOCATE_COLOR(best, d->table[i]);

		/*
		 * minimum over candidate approximations is the actual
		 * difference between the chosen approximation and the 
		 * desired color.
		 */
		if (mindiff > maxdiff)
		    maxdiff = mindiff;

		nApprox ++;

		if (! done && n == max_nbrhd)
		    DXErrorReturn(ERROR_UNEXPECTED, "#11680");
	    }
	}
    }

    DXDebug("X", "%d exact matches with pre-allocated color cells", nExact);
    DXDebug("X", "%d color cells allocated for exact colors", nAlloc);
    if (nApprox)
	DXDebug("X", "%d colors approximated (max diff %f)", 
		nApprox, sqrt(maxdiff/(3.0*256.0*256.0)));
    else
	DXDebug("X", "no approximations necessary");
    
    if (! force && sqrt(maxdiff/(256.0*256.0)) > approximationTolerance)
	goto error;
    
    if (xerror) goto error;
    return OK;

error:    
    for (i = 0; i < xlatesize; i++)
        if (d->table[i] != -1)
	    XFREECOLOR(dpy, cmap, d->table[i]);

    xerror = 0;
    return ERROR;
}

struct gap
{
    int         gap;
    int         start;
    struct gap *next;
};

int
grayCmp(const void * p1, const void * p2)
{
    const struct gap **a = (const struct gap **) p1;
    const struct gap **b = (const struct gap **) p2;
    if ((*a)->gap < (*b)->gap)
	return 1;
    else
	return -1;
}

static Error
getGrayMappedTranslation(Display *dpy, translationT *d, int force)
{
    Visual 		*visual = (Visual *)d->visual;
    Colormap		cmap    = (Colormap)d->cmap;
    struct gap          *freeList, gaps[257], *sort[256], *gapList;
    int                 i, j, nextGap, nAvailable, nAllocated;
    int			nPrivate, nShared;
    unsigned long	pixels[MAXCMAPSIZE];
    XColor		*color, colors[MAXCMAPSIZE];
    int			cmapsize;
    unsigned char	allocated[256];
    unsigned char	readOnly[256];
    float		invgamma;

    memset(allocated, 0, 256*sizeof(unsigned char));

    /*
     * find out how big the cmap is
     */
    cmapsize = visual->map_entries;
    cmapsize = cmapsize > MAXCMAPSIZE? MAXCMAPSIZE : cmapsize;

    d->translationType = GrayMapped;

    d->rtable = &d->data[0];
    d->gtable = &d->data[MAXRGBCMAPSIZE];
    d->btable = &d->data[MAXRGBCMAPSIZE*2];

    d->table  = &d->data[MAXRGBCMAPSIZE*3];

    if (! getGamma(d->depth, &(d->gamma)))
        goto error;

    invgamma = 1.0 / d->gamma;
    for (i=0; i<256; i++)
    {
	float f = i / 255.0;
        d->gammaTable[i] = 255*pow(f, invgamma);
    }

    for (i = 0; i < 256; i++)
    {
	d->rtable[i] = (int)((0.30*i) + 0.5);
	d->gtable[i] = (int)((0.59*i) + 0.5);
	d->btable[i] = (int)((0.11*i) + 0.5);
	d->table[i]  = 0xffffffff;
    }

    /*
     * go for it: try to allocate the entire chunk required.
     */
    if (XAllocColorCells(dpy, cmap, 0, NULL, 0, pixels, 256))
    {
	XFreeColors(dpy, cmap, pixels, 256, 0);

	for (i = 0; i < 256; i++)
	{
	    XColor c;

	    c.red   = c.green = c.blue = (i << 8) | i;

	    if (! XAllocColor(dpy, cmap, &c))
	    {
		DXDebug("X", "XAllocColor failed");
		return ERROR;
	    }

	    d->table[i] = c.pixel;
	}
	
	return OK;
    }
	    
    /*
     * Find out how many unallocated memory cells there are
     */
    for (i = cmapsize, nAvailable = 0; i > 0; i /= 2)
    {
	while (XAllocColorCells(dpy, cmap, False,
				NULL, 0, pixels+nAvailable, i))
	    nAvailable += i;
    }

    if (nAvailable)
	XFreeColors(dpy, cmap, pixels, nAvailable, 0);

    /*
     * create a flag array indicating the unallocated cells
     */
    memset(readOnly, 1, cmapsize*sizeof(unsigned char));
    for (j = 0; j < nAvailable; j++)
	readOnly[pixels[j]] = 0;

    /*
     * query the colors
     */
    for (color = colors, j=0; j<cmapsize; j++, color++)
    {
	color->pixel = j;
	color->flags = DoRed | DoGreen | DoBlue;
    }
    XQueryColors(dpy, (Colormap)cmap, colors, cmapsize);

    /*
     * Now determine which of the already-allocated cells are sharable.
     * We do this by trying to re-allocate them... if we get the same
     * pixel index, then the cell was sharable.
     */
    nPrivate = nShared = 0;
    for (i = 0, color = colors; i < cmapsize; i++, color++)
    {
	if (readOnly[color->pixel])
	{
	    XColor c;
	    int    grayLevel = color->red >> 8;

	    if (d->table[grayLevel] == 0xffffffff)
	    {
		c.red   = color->red;
		c.green = color->green;
		c.blue  = color->blue;

		if (XALLOCCOLOR(dpy, cmap, &c))
		{
		    if (c.pixel != color->pixel)
		    {
			nPrivate ++;
			readOnly[color->pixel] = 0;
			XFREECOLOR(dpy, cmap, c.pixel);
		    }
		    else
		    {
			nShared ++;
			d->table[grayLevel] = c.pixel;
			allocated[c.pixel]++;
		    }
		}
		else
		{
		    nPrivate ++;
		    readOnly[color->pixel] = 0;
		}
	    }
	}
    }

    /*
     * Make a list of the gaps between pre-allocated sharable
     * colors
     */
    gapList = NULL;
    for (i = j = nextGap = 0; i < cmapsize; i++)
    {
	if (d->table[i] != 0xffffffff)
	{
	    if ((i - j) > 0)
	    {
		gaps[nextGap].gap   = i - j;
		gaps[nextGap].start = j;
		nextGap ++;
	    }

	    j = i + 1;
	}
    }

    if (j < cmapsize-1)
    {
	gaps[nextGap].gap  = cmapsize - j;
	gaps[nextGap].start = j;
	nextGap++;
    }

    /*
     * Sort them from largest gap to smallest gap
     */

    for (i = 0; i < nextGap; i++)
	sort[i] = gaps+i;

    qsort((char *)sort, nextGap, sizeof(sort[0]), grayCmp);

    /*
     * Make a linked list of the sorted gaps
     */
    
    gapList = sort[0];
    for (i = 0; i < nextGap-1; i++)
	sort[i]->next = sort[i+1];
    sort[nextGap-1]->next = NULL;

    /*
     * link up the gap blocks into a free list
     */
    freeList = gaps + nextGap;
    for (i = nextGap; i < 256; i++)
	gaps[i].next = gaps + (i+1);
    gaps[256].next = NULL;

    /*
     * Now allocate our available writable colors to the middle of
     * the gaps in sorted order
     */
    for (nAllocated = 0; nAllocated < nAvailable && gapList; nAllocated++)
    {
	/*
	 * the new gray level is the midpoint of the gap
	 */
	int grayLevel = gapList->start + (gapList->gap >> 1);
	int gap0 = grayLevel - gapList->start;
	int gap1 = gapList->gap - (gap0 + 1);
	XColor c0, c1;

	c0.red   = c1.red   = grayLevel | (grayLevel << 8);
	c0.green = c1.green = c0.red;
	c0.blue  = c1.blue  = c0.red;

	if (! XALLOCCOLOR(dpy, cmap, &c0))
	    goto error;
	
	if (((c0.red   ^ c1.red)   & 0xff00) &&
	    ((c0.green ^ c1.green) & 0xff00) &&
	    ((c0.blue  ^ c1.blue)  & 0xff00))
	    nAllocated++;

	d->table[grayLevel] = c0.pixel;

	/*
	 * If there are gaps on either side of the inserted value,
	 * then create gap structures for them and insert into
	 * the sorted list.
	 */
	if (gap0)
	{
	    struct gap *last, *new;

	    new = freeList;
	    freeList = freeList->next;

	    new->gap   = grayLevel - gapList->start;
	    new->start = gapList->start;

	    for (last = gapList; last; last = last->next)
		if (last->next == NULL || last->next->gap < new->gap)
		    break;
	    
	    new->next = last->next;
	    last->next = new;
	}

	if (gap1)
	{
	    struct gap *last, *new;

	    new = freeList;
	    freeList = freeList->next;

	    new->gap   = gapList->gap - (gap0 + 1);
	    new->start = grayLevel + 1;

	    for (last = gapList; last; last = last->next)
		if (last->next == NULL || last->next->gap < new->gap)
		    break;
	    
	    new->next = last->next;
	    last->next = new;
	}

	{
	    struct gap *tmp = gapList->next;
	    gapList->next = freeList;
	    freeList = gapList;
	    gapList = tmp;
	}
    }

    while (gapList)
    {
	if (gapList->start == 0) 
	{
	    for (i = 0; i < gapList->gap; i++)
		d->table[i] = d->table[gapList->gap];
	}
	else if (gapList->start + gapList->gap >= 256)
	{
	    for (i = gapList->start; i < 256; i++)
		d->table[i] = d->table[gapList->start - 1];
	}
	else
	{
	    for (i = 0, j = gapList->start; i < (gapList->gap >> 1); i++, j++)
		d->table[j] = d->table[gapList->start-1];
	    
	    for (; i < gapList->gap; i++, j++)
		d->table[j] = d->table[gapList->start + gapList->gap];
	}

	gapList = gapList->next;
    }
    
    if (xerror) goto error;
    return OK;

error:    
    for (i = 0; i < 256; i++)
        if (d->table[i] != 0xffffffff)
	    XFREECOLOR(dpy, cmap, d->table[i]);

    xerror = 0;
    return ERROR;
}

static Error
getStaticGrayTranslation(Display *dpy, translationT *d)
{
    int			i;
    Visual 		*visual = (Visual *)d->visual;
    float		invgamma;

    d->translationType = GrayStatic;

    if (! getGamma(d->depth, &(d->gamma)))
        goto error;

    invgamma = 1.0 / d->gamma;
    for (i=0; i<256; i++)
    {
	float f = i / 255.0;
        d->gammaTable[i] = 255*pow(f, invgamma);
    }

    d->rtable = &d->data[0];
    d->gtable = &d->data[MAXRGBCMAPSIZE];
    d->btable = &d->data[MAXRGBCMAPSIZE*2];

    for (i = 0; i < 256; i++)
    {
	d->rtable[i] = (int)((0.30*i) + 0.5);
	d->gtable[i] = (int)((0.59*i) + 0.5);
	d->btable[i] = (int)((0.11*i) + 0.5);
    }

    if (visual->map_entries != 256)
    {
	float scale = visual->map_entries / 256.0;

	for (i = 0; i < 256; i++)
	{
	    d->rtable[i] *= scale;
	    d->gtable[i] *= scale;
	    d->btable[i] *= scale;
	}
    }

    return OK;

error:    
    return ERROR;
}

static Error
getStaticColorTranslation(Display *dpy, translationT *d)
{
    int 		i;
    float		invgamma;
    Visual 		*vis = (Visual *)(d->visual);
    int			redWidth, redShift, redBits, maxRed;
    int			greenWidth, greenShift, greenBits, maxGreen;
    int			blueWidth, blueShift, blueBits, maxBlue;

    d->translationType = ColorStatic;

    d->redRange   = 256;
    d->greenRange = 256;
    d->blueRange  = 256;

    if (! getGamma(d->depth, &(d->gamma)))
        goto error;

    invgamma = 1.0 / d->gamma;
    for (i=0; i<256; i++)
    {
	float f = i / 255.0;
        d->gammaTable[i] = 255*pow(f, invgamma);
    }

    redShift = 0;
    redBits = vis->red_mask;
    while ((0x1 & redBits) == 0)
	redShift++, redBits >>= 1;
    for (maxRed = 1, redWidth = 0; redBits & maxRed; maxRed <<= 1, redWidth ++);

    greenShift = 0;
    greenBits = vis->green_mask;
    while ((0x1 & greenBits) == 0)
	greenShift++, greenBits >>= 1;
    for (maxGreen = 1, greenWidth = 0; greenBits & maxGreen; maxGreen <<= 1, greenWidth ++);

    blueShift = 0;
    blueBits = vis->blue_mask;
    while ((0x1 & blueBits) == 0)
	blueShift++, blueBits >>= 1;
    for (maxBlue = 1, blueWidth = 0; blueBits & maxBlue; maxBlue <<= 1, blueWidth ++);

    d->rtable = &d->data[0];
    d->gtable = &d->data[MAXRGBCMAPSIZE];
    d->btable = &d->data[MAXRGBCMAPSIZE*2];

    d->redRange   = maxRed;
    d->greenRange = maxGreen;
    d->blueRange  = maxBlue;

#if defined(WORDS_BIGENDIAN)
    if(XImageByteOrder(dpy) == LSBFirst)
#else
    if(XImageByteOrder(dpy) == MSBFirst)
#endif
    {
	redShift   = 24 - redShift;
	greenShift = 24 - greenShift;
	blueShift  = 24 - blueShift;
    }

    for (i = 0; i < maxRed; i++)
	d->rtable[i] = i << redShift;

    for (i = 0; i < maxGreen; i++)
	d->gtable[i] = i << greenShift;

    for (i = 0; i < maxBlue; i++)
	d->btable[i] = i << blueShift;
    
    return OK;

error:    
    return ERROR;
}

static Error
setColors(struct window *w, Colormap xCmap, Array iCmap)
{
    XColor colors[MAXCMAPSIZE];
    float *cIn = (float *)DXGetArrayData(iCmap);
    int   i, j, n, grayIn, grayOut;
    translationT *trans = w->translation;
    Visual *vis = (Visual *)(trans->visual);
    int oneMap, r, g, b;

    if (! trans)
    {
        DXSetError(ERROR_INTERNAL, "cannot setColors without translation");
        return ERROR;
    }

    if (vis->class == PseudoColor)
    {
	grayOut = 0;
	oneMap = 1;
    }
    else if (vis->class == DirectColor)
    {
	grayOut = 0;
	oneMap = 0;
    }
    else if (vis->class == GrayScale)
    {
	grayOut = 1;
	oneMap = 1;
    }
    else
    {
	DXSetError(ERROR_INTERNAL, "Invalid visual type");
	return ERROR;
    }

    DXGetArrayInfo(iCmap, &n, NULL, NULL, NULL, &grayIn);
    if (grayIn == 3) grayIn = 0;

    if (grayIn)
    {
        for (i = 0; i < n; i++)
        {
	    colors[i].flags = DoRed | DoGreen | DoBlue;

	    if (oneMap)
		colors[i].pixel = i;
	    else
		colors[i].pixel = (i << 16) | (i << 8) | i;

	    j = (int)(*cIn++ * 255);

	    colors[i].red   = 0x000000ff * trans->gammaTable[j];
            colors[i].green = colors[i].blue = colors[i].red;
        }
    }
    else
    {
        for (i = 0; i < n; i++)
        {
	    colors[i].flags = DoRed | DoGreen | DoBlue;

	    if (oneMap)
		colors[i].pixel = i;
	    else
		colors[i].pixel = (i << 16) | (i << 8) | i;

	    r = (int)(*cIn++ * 255);
	    g = (int)(*cIn++ * 255);
	    b = (int)(*cIn++ * 255);

	    if (grayOut)
	    {
		r = 0x000000ff * trans->gammaTable[r];
		g = 0x000000ff * trans->gammaTable[g];
		b = 0x000000ff * trans->gammaTable[b];

		colors[i].red   = 0.30*r + 0.59*g + 0.11*b;
		colors[i].green = colors[i].blue = colors[i].red;
	    }
	    else
	    {
		colors[i].red   = 0x000000ff * trans->gammaTable[r];
		colors[i].green = 0x000000ff * trans->gammaTable[g];
		colors[i].blue  = 0x000000ff * trans->gammaTable[b];
	    }
        }
    }

    XStoreColors(w->dpy, xCmap, colors, n);

    return OK;
}

static Error
CheckColormappedImage(Object image, Array colormap)
{
    Array a;
    int n;
    Type t;
    Category c;
    int r, s[32];

    if (image)
    {
	if (DXGetObjectClass(image) != CLASS_FIELD)
	{
	    DXSetError(ERROR_DATA_INVALID, "color-mapped images must be fields");
	    goto error;
	}

	a = (Array)DXGetComponentValue((Field)image, "colors");
	if (! a)
	{
	    DXSetError(ERROR_DATA_INVALID, "missing colors component");
	    goto error;
	}

	if (! DXGetArrayInfo(a, NULL, &t, &c, &r, s))
	    return ERROR;

	if ((t != TYPE_BYTE && t != TYPE_UBYTE) ||
	    (c != CATEGORY_REAL) ||
	    (r != 0 && (r != 1 || s[0] != 1)))
	{
	    DXSetError(ERROR_DATA_INVALID,
	      "colormapped images must be byte or ubyte, scalar or 1-vector");
	    return ERROR;
	}
    }

    if (colormap)
    {
	if (DXGetObjectClass((Object)colormap) != CLASS_ARRAY)
	{
	    DXSetError(ERROR_DATA_INVALID,
	      "image colormap images must be an Array");
	    goto error;
	}

	DXGetArrayInfo(colormap, &n, &t, &c, &r, s);

	if ((n > 256) ||
	    (t != TYPE_FLOAT) ||
	    (c != CATEGORY_REAL) ||
	    (r != 1) ||
	    (s[0] != 1 && s[0] != 3))
	{
	    DXSetError(ERROR_DATA_INVALID,
		    "image colormap images must be >=256 float 3-vectors");
	    goto error;
	}
    }

    return OK;

error:
    return ERROR;
}

#if DXD_OS2_SYSCALL
static int _Optlink
#else
static int
#endif
icmp(const void * p1, const void * p2)
{
    const long *a = p1;
    const long *b = p2;
    if (*a < *b)
	return -1;
    else
	return 1;
}

static Error
getPrivateColormap(Display *dpy, translationT *trans, Colormap xcmap)
{
    int mymapsize, cmapsize;
    XColor colors[MAXCMAPSIZE];
    XColor ncolors[MAXCMAPSIZE];
    float invgamma;
    unsigned int comp[256];
    int start, n, nn, i, r, g, b, rr, gg, bb;
    unsigned long pixels[MAXCMAPSIZE];


    if (! getGamma(trans->depth, &(trans->gamma)))
        goto error;

    invgamma = 1.0 / trans->gamma;

    cmapsize = ((Visual *)trans->visual)->map_entries;

    if (cmapsize <= 16)
    {
	rr = 2;
	gg = 3;
	bb = 2;
    }
    else if (cmapsize <= 256)
    {
	rr = 5;
	gg = 9;
	bb = 5;
    }
    else
    {
	rr = 16;
	gg = 16;
	bb = 16;
    }

    trans->translationType = OneMap;
    trans->redRange   = rr;
    trans->greenRange = gg;
    trans->blueRange  = bb;

    trans->table = (unsigned long *)trans->data;
    trans->rtable = NULL;
    trans->gtable = NULL;
    trans->btable = NULL;

    mymapsize = rr * gg * bb;

    for (n = cmapsize, nn = 0; n > 0; n /= 2)
	while(XAllocColorCells(dpy, xcmap, False, NULL, 0, pixels+nn, n))
	    nn += n;
    
    if (nn < mymapsize)
    {
	DXSetError(ERROR_INTERNAL, 
		"too few free color cells available in private colormap");
	return ERROR;
    }

    DXDebug("X", "able to allocate %d cells in private colormap", nn);

    for (i = 0; i < 256; i++)
    {
	float f = i / 255.0;
	comp[i] = 255*pow(f, invgamma);
    }

    for (i = 0; i < cmapsize; i++)
    {
	colors[i].pixel = i;
	colors[i].flags = DoRed | DoGreen | DoBlue;
    }

    /*
     * If we allocated more cells than we need, we'll copy the input
     * colormap's contents to the available slots to minimize the
     * technicolor vomit.  We assume that if some of the cells are
     * unwritable, then they contain the same colors as the input
     * colormap.
     */
    start = nn - mymapsize;

    qsort((char *)pixels, nn, sizeof(long), icmp);

#if DEBUG
    DXMessage("nn: %d start: %d, mymapsize: %d", start, nn, mymapsize);
#endif

    XQueryColors(dpy, trans->cmap, colors, cmapsize);

    for (i = 0; i < start; i++)
    {
	int entry = pixels[i];

	ncolors[i].red   = colors[entry].red;
	ncolors[i].green = colors[entry].green;
	ncolors[i].blue  = colors[entry].blue;
	ncolors[i].pixel = entry;
	ncolors[i].flags = DoRed | DoGreen | DoBlue;

#if DEBUG
	DXMessage("copy: %d) %d %d %d", ncolors[i].pixel,
	    ncolors[i].red, ncolors[i].green, ncolors[i].blue);
#endif
    }

    for (r = 0, i = start; r < rr; r++)
	for (g = 0; g < gg; g++)
	    for (b = 0; b < bb; b++, i++)
	    {
		int p = MIX(r, g, b);
		int entry = pixels[i];

		ncolors[i].red   = (comp[r*255/(rr-1)]) << 8;
		ncolors[i].green = (comp[g*255/(gg-1)]) << 8;
		ncolors[i].blue  = (comp[b*255/(bb-1)]) << 8;
		ncolors[i].pixel = entry;
		ncolors[i].flags = DoRed | DoGreen | DoBlue;

		trans->table[p] = entry;

#if DEBUG
		DXMessage("set: %d) %d %d %d", ncolors[i].pixel,
		    ncolors[i].red, ncolors[i].green, ncolors[i].blue);
#endif
	    }

    XStoreColors(dpy, xcmap, ncolors, nn);

    XFreeColors(dpy, xcmap, pixels, start, 0);

    for (i = 0; i < start; i++)
    {
#if DEBUG
	int foo = ncolors[i].pixel;
#endif

	if (! XAllocColor(dpy, xcmap, ncolors+i))
	    return ERROR;
	
#if DEBUG
	DXMessage("realloc: %d -> %d", foo, ncolors[i].pixel);
#endif
    }
	

    return OK;

error:
    return ERROR;
}


static Field
_dxfCaptureDereferencedOneMapImage(struct window *w, translationT *t, Visual *v)
{
    Field image = DXMakeImageFormat(w->wwidth, w->wheight, "BYTE");
    ubyte  *dxmap = NULL;

    if (image)
    {
	int    y, dst_rowsize = w->wwidth*3;
	int    i, j, pixsize = _dxf_GetXBytesPerPixel(t);
	XColor xmap[256];
	Array  c = (Array)DXGetComponentValue(image, "colors");
	if (! c)
	    goto error;

	for (i = 0; i < v->map_entries; i++)
	    xmap[i].pixel = i;

	XQueryColors(w->dpy, (Colormap)t->cmap, xmap, v->map_entries);

	dxmap = (ubyte *)DXAllocate(3*v->map_entries*sizeof(ubyte));
	if (! dxmap)
	    goto error;

	for (i = j = 0; i < v->map_entries; i++, j += 3)
	{
	    dxmap[j+0] = xmap[i].red   >> 8;
	    dxmap[j+1] = xmap[i].green >> 8;
	    dxmap[j+2] = xmap[i].blue  >> 8;
	}

	if (pixsize == 1)
	{
	    ubyte *src = (ubyte *)w->pixels;
	    ubyte *dst = ((ubyte *)DXGetArrayData(c))
				+ (w->wheight-1)*dst_rowsize;

	    for (y = 0; y < w->wheight; y++)
	    {
		ubyte *d = dst;
		ubyte *e = d + dst_rowsize;

		while (d < e)
		{
		    ubyte *c = dxmap + 3 * (*src);
		    *d++ = *c++;
		    *d++ = *c++;
		    *d++ = *c++;
		    src += pixsize;
		}

		dst -= dst_rowsize;
	    }
	}
	else if (pixsize == 2)
	{
	    ushort *src = (ushort *)w->pixels;
	    ubyte *dst = ((ubyte *)DXGetArrayData(c))
				+ (w->wheight-1)*dst_rowsize;

	    for (y = 0; y < w->wheight; y++)
	    {
		ubyte *d = dst;
		ubyte *e = d + dst_rowsize;

		while (d < e)
		{
		    ubyte *c = dxmap + 3 * (*src);
		    *d++ = *c++;
		    *d++ = *c++;
		    *d++ = *c++;
		    src += pixsize;
		}

		dst -= dst_rowsize;
	    }
	}
	else if (pixsize == 4)
	{
	    ulong *src = (ulong *)w->pixels;
	    ubyte *dst = ((ubyte *)DXGetArrayData(c))
				+ (w->wheight-1)*dst_rowsize;

	    for (y = 0; y < w->wheight; y++)
	    {
		ubyte *d = dst;
		ubyte *e = d + dst_rowsize;

		while (d < e)
		{
		    ubyte *c = dxmap + 3 * (*src);
		    *d++ = *c++;
		    *d++ = *c++;
		    *d++ = *c++;
		    src += pixsize;
		}

		dst -= dst_rowsize;
	    }
	}
    }

    DXFree((Pointer)dxmap);

    return image;

error:
    DXDelete((Object)image);
    return ERROR;
}

static Field
_dxfCaptureDelayedColorImage(struct window *w, translationT *t, Visual *v)
{
    Field image = DXMakeImageFormat(w->wwidth, w->wheight, "DELAYEDCOLOR");
    if (image)
    {
	int    i, pixsize = _dxf_GetXBytesPerPixel(t);
	XColor xmap[256];
	float  *dxmap;
	Array  c = (Array)DXGetComponentValue(image, "colors");
	Array  m = (Array)DXGetComponentValue(image, "color map");
	if (! c || ! m)
	    goto error;

	if (pixsize == 1)
	{
	    int    y, dst_rowsize = w->wwidth, src_rowsize = pixsize*w->wwidth;
	    ubyte  *src = (ubyte *)w->pixels;
	    ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
				(w->wheight-1)*dst_rowsize;

	    for (y = 0; y < w->wheight; y++)
	    {
		memcpy(dst, src, w->wwidth);
		dst -= dst_rowsize;
		src += src_rowsize;
	    }
	}
	else
	{
	    int    y, dst_rowsize = w->wwidth;
	    ubyte  *src = (ubyte *)w->pixels;
	    ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
				(w->wheight-1)*dst_rowsize;

	    for (y = 0; y < w->wheight; y++)
	    {
		ubyte *d = dst;
		ubyte *e = d + dst_rowsize;

		while (d < e)
		{
		    *d++ = *src;
		    src += pixsize;
		}

		dst -= dst_rowsize;
	    }
	}

	for (i = 0; i < v->map_entries; i++)
	{
	    xmap[i].pixel = i;
	    xmap[i].flags = DoRed | DoGreen | DoBlue;
	}

	XQueryColors(w->dpy, (Colormap)t->cmap, xmap, v->map_entries);
	dxmap = (float *)DXGetArrayData(m);

	for (i = 0; i < v->map_entries; i++)
	{
	    *dxmap++ = xmap[i].red / 65536.0;
	    *dxmap++ = xmap[i].green / 65536.0;
	    *dxmap++ = xmap[i].blue / 65536.0;
	}

	for ( ; i < 256; i++)
	{
	    *dxmap++ = 0.0;
	    *dxmap++ = 0.0;
	    *dxmap++ = 0.0;
	}
    }

    return image;

error:
    DXDelete((Object)image);
    return ERROR;
}

static Field
_dxfCaptureThreeMapImage(struct window *w, translationT *t, Visual *v)
{
    Field image = DXMakeImageFormat(w->wwidth, w->wheight, "BYTE");
    if (image)
    {
	int    rshift, gshift, bshift;
	int    rmask, gmask, bmask;
	int    i, notSimple;
	XColor xmap[256];
	Array  c = (Array)DXGetComponentValue(image, "colors");
	if (! c)
	    goto error;

	for (rshift = 0; rshift < 32; rshift++)
	    if (v->red_mask & (1 << rshift))
		break;

	for (gshift = 0; gshift < 32; gshift++)
	    if (v->green_mask & (1 << gshift))
		break;

	for (bshift = 0; bshift < 32; bshift++)
	    if (v->blue_mask & (1 << bshift))
		break;

	rmask = (v->red_mask)   >> rshift;
	gmask = (v->green_mask) >> gshift;
	bmask = (v->blue_mask)  >> bshift;
	    
	for (i = 0; i < v->map_entries; i++)
	    xmap[i].pixel = (i << rshift) | (i << gshift) | (i << bshift);

	XQueryColors(w->dpy, (Colormap)t->cmap, xmap, v->map_entries);

	for (notSimple = 0, i = 0; i < v->map_entries; i++)
	{
	    xmap[i].red >>= 8, xmap[i].green >>= 8, xmap[i].blue >>= 8;
	    if ((xmap[i].red   != i) ||
	        (xmap[i].green != i) ||
	        (xmap[i].blue  != i))
		    notSimple = 1;
	}

	if (notSimple)
	{
	    /*
	     * then the map isn't 1-1, so we need to map the pixels
	     */
	    if ((rshift == 0 || rshift == 8 || rshift == 16 || rshift == 24) &&
		(gshift == 0 || gshift == 8 || gshift == 16 || gshift == 24) &&
	        (bshift == 0 || bshift == 8 || bshift == 16 || bshift == 24))
	    {
#if !defined(WORDS_BIGENDIAN)
		int rbyte = (rshift == 0)  ? 0 :
			    (rshift == 8)  ? 1 :
			    (rshift == 16) ? 2 : 3;
		int gbyte = (gshift == 0)  ? 0 :
			    (gshift == 8)  ? 1 :
			    (gshift == 16) ? 2 : 3;
		int bbyte = (bshift == 0)  ? 0 :
			    (bshift == 8)  ? 1 :
			    (bshift == 16) ? 2 : 3;
#else
		int rbyte = (rshift == 0)  ? 3 :
			    (rshift == 8)  ? 2 :
			    (rshift == 16) ? 1 : 0;
		int gbyte = (gshift == 0)  ? 3 :
			    (gshift == 8)  ? 2 :
			    (gshift == 16) ? 1 : 0;
		int bbyte = (bshift == 0)  ? 3 :
			    (bshift == 8)  ? 2 :
			    (bshift == 16) ? 1 : 0;
#endif
	    
		if (rmask == 255 && gmask == 255 && bmask == 255)
		{
		    /*
		     * then the pixels contain 3 bytes
		     * so we can decode by extracting bytes
		     */
		    int    y, dst_rowsize = 3*w->wwidth;
		    ubyte  *src = (ubyte *)w->pixels;
		    ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
					(w->wheight-1)*dst_rowsize;

		    for (y = 0; y < w->wheight; y++)
		    {
			ubyte *d = dst;
			ubyte *e = d + dst_rowsize;

			while (d < e)
			{
			    *d++ = xmap[src[rbyte]].red;
			    *d++ = xmap[src[gbyte]].green;
			    *d++ = xmap[src[bbyte]].blue;
			    src += 4;
			}

			dst -= dst_rowsize;
		    }
		}
		else
		{
		    /*
		     * then the pixels contain 3 bytes
		     * so we can decode by extracting bytes
		     * but we have to mask them
		     */
		    int    y, dst_rowsize = 3*w->wwidth;
		    ubyte  *src = (ubyte *)w->pixels;
		    ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
					(w->wheight-1)*dst_rowsize;

		    for (y = 0; y < w->wheight; y++)
		    {
			ubyte *d = dst;
			ubyte *e = d + dst_rowsize;

			while (d < e)
			{
			    *d++ = xmap[src[rbyte]&rmask].red;
			    *d++ = xmap[src[gbyte]&gmask].green;
			    *d++ = xmap[src[bbyte]&bmask].blue;
			    src += 4;
			}

			dst -= dst_rowsize;
		    }
		}
	    }
	    else
	    {
		/*
		 * then the pixels contain 3 bytes
		 * so we can decode by extracting bytes
		 * but we have to mask them
		 */
		int    y, dst_rowsize = 3*w->wwidth;
		long   *src = (long *)w->pixels;
		ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
				    (w->wheight-1)*dst_rowsize;

		for (y = 0; y < w->wheight; y++)
		{
		    ubyte *d = dst;
		    ubyte *e = d + dst_rowsize;

		    while (d < e)
		    {
			*d++ = xmap[(*src>>rshift)&rmask].red;
			*d++ = xmap[(*src>>gshift)&gmask].green;
			*d++ = xmap[(*src>>bshift)&bmask].blue;
			src ++;
		    }

		    dst -= dst_rowsize;
		}
	    }
	}
	else
	{
	    /*
	     * then the map IS 1-1, so we DON'T need to map the pixels
	     */
	    if ((rshift == 0 || rshift == 8 || rshift == 16 || rshift == 24) &&
		(gshift == 0 || gshift == 8 || gshift == 16 || gshift == 24) &&
	        (bshift == 0 || bshift == 8 || bshift == 16 || bshift == 24))
	    {
#if defined(WORDS_BIGENDIAN)
		int rbyte = (rshift == 0)  ? 3 :
			    (rshift == 8)  ? 2 :
			    (rshift == 16) ? 1 : 0;
		int gbyte = (gshift == 0)  ? 3 :
			    (gshift == 8)  ? 2 :
			    (gshift == 16) ? 1 : 0;
		int bbyte = (bshift == 0)  ? 3 :
			    (bshift == 8)  ? 2 :
			    (bshift == 16) ? 1 : 0;
#else
		int rbyte = (rshift == 0)  ? 0 :
			    (rshift == 8)  ? 1 :
			    (rshift == 16) ? 2 : 3;
		int gbyte = (gshift == 0)  ? 0 :
			    (gshift == 8)  ? 1 :
			    (gshift == 16) ? 2 : 3;
		int bbyte = (bshift == 0)  ? 0 :
			    (bshift == 8)  ? 1 :
			    (bshift == 16) ? 2 : 3;
#endif
	    
		if (rmask == 255 && gmask == 255 && bmask == 255)
		{
		    /*
		     * then the pixels contain 3 bytes
		     * so we can decode by extracting bytes
		     */
		    int    y, dst_rowsize = 3*w->wwidth;
		    ubyte  *src = (ubyte *)w->pixels;
		    ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
					(w->wheight-1)*dst_rowsize;

		    for (y = 0; y < w->wheight; y++)
		    {
			ubyte *d = dst;
			ubyte *e = d + dst_rowsize;

			while (d < e)
			{
			    *d++ = src[rbyte];
			    *d++ = src[gbyte];
			    *d++ = src[bbyte];
			    src += 4;
			}

			dst -= dst_rowsize;
		    }
		}
		else
		{
		    /*
		     * then the pixels contain 3 bytes
		     * so we can decode by extracting bytes
		     * but we have to mask them
		     */
		    int    y, dst_rowsize = 3*w->wwidth;
		    ubyte  *src = (ubyte *)w->pixels;
		    ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
					(w->wheight-1)*dst_rowsize;

		    for (y = 0; y < w->wheight; y++)
		    {
			ubyte *d = dst;
			ubyte *e = d + dst_rowsize;

			while (d < e)
			{
			    *d++ = src[rbyte]&rmask;
			    *d++ = src[gbyte]&gmask;
			    *d++ = src[bbyte]&bmask;
			    src += 4;
			}

			dst -= dst_rowsize;
		    }
		}
	    }
	    else
	    {
		/*
		 * then the pixels contain 3 bytes
		 * so we can decode by extracting bytes
		 * but we have to mask them
		 */
		int    y, dst_rowsize = 3*w->wwidth;
		long   *src = (long *)w->pixels;
		ubyte  *dst = ((ubyte *)DXGetArrayData(c)) + 
				    (w->wheight-1)*dst_rowsize;

		for (y = 0; y < w->wheight; y++)
		{
		    ubyte *d = dst;
		    ubyte *e = d + dst_rowsize;

		    while (d < e)
		    {
			*d++ = (*src>>rshift)&rmask;
			*d++ = (*src>>gshift)&gmask;
			*d++ = (*src>>bshift)&bmask;
			src ++;
		    }

		    dst -= dst_rowsize;
		}
	    }
	}
    }

    return image;

error:
    DXDelete((Object)image);
    return ERROR;
}


Field 
_dxf_CaptureXSoftwareImage(int depth, char *host, char *window)
{
    struct window *w = NULL;
    Private w_obj = NULL;
    int directMap = 0;
    Field image = NULL;
    translationP t;
    Visual *v;

    if (! getWindowStructure(host, depth, window, &directMap, &w, &w_obj, 1))
	goto error;
    
    if (w->active == 0)
	return NULL;
    
    t = w->translation;
    v = t->visual;
    
    switch(v->class)
    {
	case PseudoColor:
	case StaticColor:
	case GrayScale:
	    {
		int mapSize = (v->map_entries > MAXCMAPSIZE)
			    ? MAXCMAPSIZE : v->map_entries;
		
		if (mapSize > 256)
		    image = _dxfCaptureDereferencedOneMapImage(w, t, v);
		else
		    image = _dxfCaptureDelayedColorImage(w, t, v);

		break;
	    }

	case TrueColor:
	case DirectColor:
	    {
		image = _dxfCaptureThreeMapImage(w, t, v);
		break;
	    }
	
	default:
	    DXSetError(ERROR_INTERNAL, "invalid visual class");
	    goto error;
    }

    if (! image)
	goto error;
    
    DXDelete((Object)w_obj);
    return image;

error:
    DXDelete((Object)image);
    DXDelete((Object)w_obj);
    return NULL;
}


Error
DXGetSoftwareWindow(char *where, Window *window)
{
    Private p = NULL;
    char *cachetag = NULL;
    struct window *w;
    char *host = NULL, *title = NULL;

    if (! DXParseWhere(where, NULL, NULL, &host, &title, NULL, NULL))
    {
	DXSetError(ERROR_UNEXPECTED, "unable to parse where parameter");
	goto error;
    }

    if (!title || !*title)
	title = "Image";

    if (!host || !*host)
	host = getenv("DISPLAY");
    if (! host)
#if defined(intelnt) || defined(WIN32)
       host = "";
#else
       host = "unix:0";
#endif


    cachetag = getWindowCacheTag(host, title);
    p = (Private)DXGetCacheEntry(cachetag, 0, 0);
    
    if (!p)
	goto error;

    w = (struct window *)DXGetPrivateData(p);
    *window = w->wid;

    DXFree((Pointer)title);
    DXFree((Pointer)host);
    DXDelete((Object)p);
    DXFree((Pointer)cachetag);

    return OK;

error:
    DXFree((Pointer)title);
    DXFree((Pointer)host);
    if (p)
	DXDelete((Object)p);
    if (cachetag)
	DXFree((Pointer)cachetag);

    return ERROR;
}

