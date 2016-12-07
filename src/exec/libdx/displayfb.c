/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <math.h>
#include <dx/dx.h>
#include "internals.h"


#if DXD_HAS_LIBIOP


#include <iop/afb.h>

struct arg {
    Object o;				/* the object to copy */
    struct fbpixel *fb;			/* the fb */
    int ox, oy;				/* overall image origin */
    int width, height;			/* total size of fb */
    int i, n;				/* we are part i of n */
};

#define SGN(x) ((x)>0? 1 : (x)<0? -1 : 0)

static Error
traverse(Object o, struct arg *arg)
{
    union {
	Pointer ptr;
	RGBColor *rgb;
	unsigned char *byte;
    } pixels, f, ff;
    RGBColor *fmap = NULL;
    struct { unsigned char r, g, b; } imap[256];
    Object oo;
    Point box[8];
    int tx, ty, p, px, py, ox, oy, width, height, x, y, i;
    struct fbpixel *t, *tt;
    struct { float x, y; } deltas[2];
    int d0, d1, dims[2], offsets[2];
    Array a;

    switch (DXGetObjectClass((Object)o)) {
    case CLASS_GROUP:
	if (DXGetGroupClass((Group)o)!=CLASS_COMPOSITEFIELD)
	    DXErrorReturn(ERROR_BAD_PARAMETER,
			"generic groups and series cannot be displayed");
	for (i=0; oo=DXGetEnumeratedMember((Group)o, i, NULL); i++)
	    if (!traverse(oo, arg))
		return ERROR;
	break;

    case CLASS_FIELD:

	/* check */
	if (DXGetComponentValue((Field)o,IMAGE))
	    DXErrorReturn(ERROR_BAD_PARAMETER,
			"arranged special-format images not supported");

	/* mapped? */
	a = (Array) DXGetComponentValue((Field)o, "color map");
	if (a) {
	    char *conv = (char *)_dxd_convert + UNSIGN(0);
	    fmap = (RGBColor *) DXGetArrayData(a);
	    if (!fmap) goto error;
	    for (i=0; i<256; i++) {
		int r = ((union hl *)&fmap[i].r)->hl.hi;
		int g = ((union hl *)&fmap[i].g)->hl.hi;
		int b = ((union hl *)&fmap[i].b)->hl.hi;
		imap[i].r = conv[r];
		imap[i].g = conv[g];
		imap[i].b = conv[b];
	    }
	    a = (Array) DXGetComponentValue((Field)o, "colors");
	    pixels.ptr = DXGetArrayData(a);
	} else {
	    pixels.rgb = DXGetPixels((Field)o);
	}
	if (!pixels.ptr)
	    return ERROR;

	/* XXX - checking? */
	a = (Array) DXGetComponentValue((Field)o, POSITIONS);
	DXQueryGridPositions(a, NULL, dims, NULL, (float *)deltas);
	deltas[0].x = SGN(deltas[0].x);
	deltas[0].y = SGN(deltas[0].y);
	deltas[1].x = SGN(deltas[1].x);
	deltas[1].y = SGN(deltas[1].y);
	height = dims[0];
	width = dims[1];

	/* mesh offsets for position */
	a = (Array) DXGetComponentValue((Field)o, CONNECTIONS);
	DXGetMeshOffsets((MeshArray)a, offsets);

	/* deltas for writing */
	d0 = deltas[0].x - arg->width*deltas[0].y;
	d1 = deltas[1].x - arg->width*deltas[1].y;

	/* origin */
	ox = offsets[0]*deltas[0].x + offsets[1]*deltas[1].x;
	oy = offsets[0]*deltas[0].y + offsets[1]*deltas[1].y;

	/* offset for implicit partitioning */
	p = arg->i*height/arg->n;
	height = (arg->i+1)*height/arg->n - p;

	/* effect of that offset on placement within image */
	px = deltas[0].x * p;
	py = deltas[0].y * p;

	/* where to start writing */
	tx = ox - arg->ox + px;			/* origin of this part */
	ty = oy - arg->oy + py;			/* origin of this part */
	ty = arg->height - ty - 1;		/* flip upside down */
	t = arg->fb + ty*arg->width + tx;	/* to */

	/* XXX - performance critical code follows. */
	if (fmap) {

	    int r, g, b, zero = FBPIXEL(0,0,0);
	    f.byte = pixels.byte + p*width;

	    for (y=0; y<height; y++) {
		ff.byte = f.byte;
		tt = t;
		for (x=0; x<width; x++) {
		    int i = *ff.byte;
		    r = imap[i].r;
		    g = imap[i].g;
		    b = imap[i].b;
		    if (r || g || b) {
			tt->b = b;	/* order is important here */
			tt->g = g;	/* to keep the writes in the buffer */
			tt->r = r;	/* for as long as possible */
			tt->a = 0xff;
		    } else
			*(int *)tt = zero;
		    ff.byte++;
		    tt += d1;
		}
		f.byte += width;
		t += d0;
	    }

	} else {

	    char *conv = (char *)_dxd_convert + UNSIGN(0);
	    int r, g, b, zero = FBPIXEL(0,0,0);
	    f.rgb = pixels.rgb + p*width;

	    for (y=0; y<height; y++) {
		ff.rgb = f.rgb;
		tt = t;
		for (x=0; x<width; x++) {
		    r = ((union hl *)&(ff.rgb->r))->hl.hi;
		    g = ((union hl *)&(ff.rgb->g))->hl.hi;
		    b = ((union hl *)&(ff.rgb->b))->hl.hi;
		    if (r || g || b) {
			b = conv[b];
			g = conv[g];
			r = conv[r];
			tt->b = b;	/* order is important here */
			tt->g = g;	/* to keep the writes in the buffer */
			tt->r = r;	/* for as long as possible */
			tt->a = 0xff;
		    } else
			*(int *)tt = zero;
		    ff.rgb++;
		    tt += d1;
		}
		f.rgb += width;
		t += d0;
	    }
	}
	break;
    }
    return OK;

error:
    return ERROR;
}

static Error
task(struct arg *arg)
{
    return traverse(arg->o, arg);
}

/*
 *
 */

struct arg2 {
    int i, n;
    struct fbpixel *fb;
    int width, height;
};

static Error
task2(struct arg2 *arg2)
{
    int start = (arg2->i*arg2->height/arg2->n) * arg2->width;
    int end = (((arg2->i+1)*arg2->height/arg2->n)) * arg2->width;
    memset(arg2->fb+start, 0, (end-start)*sizeof(*(arg2->fb)));
    return OK;
}



/*
 *
 */

#define ONE_KB 1024

struct fb {
    int nb;			/* whether use non-blocking send */
    afb_id afb_id;		/* id for non-blocking i/o */
    char *buffer;		/* buffer address if pending, NULL if free */
    Object object;		/* object that owns buffer for fbimage */
} *fb;				/* frame buffer state */

 
Error
_dxf_initfb()
{
    fb = (struct fb *) DXAllocateZero(sizeof(struct fb));
    if (!fb)
	return ERROR;
    fb->nb = getenv("NOFBNB")? 0 : 1;
    _dxfinit_convert(1.5);	/* gamma=1.5 is suitable for frame buffer */
    return OK;
}

static Error
send(char *chan, struct fb *fb, void *image,
     int width, int height, int x, int y)
{
    static int mask = 0xffffffff;
    int flags = AFB_SWITCH;

    if (!*chan)
	chan = NULL;

    if (x < 0) {
	x = -(1 + x);
	flags = 0;
    }

    x &= ~3;
    y &= ~3;

    DXMarkTime("start send");
    if (fb->nb) {
	fb->afb_id = afb_send_nb(chan,mask,flags, image, width, height, x, y);
	if (!fb->afb_id) {
	    DXSetError(ERROR_INTERNAL, "fb send: %s", afb_errmsg(afb_errno));
	    return ERROR;
	}
    } else {
	int rc = afb_send(chan, mask, flags, image, width, height, x, y);
	if (rc < 0) {
	    DXSetError(ERROR_INTERNAL, "fb send: %s", afb_errmsg(rc));
	    return ERROR;
	}
    }
    DXMarkTime("end send");

    return OK;
}


Object
DXDisplayFB(Object o, char *name, int x, int y)
{
    int i, j, n, rc, fbimage;
    Point box[8];
    struct arg arg;
    struct fbpixel *pp, *p;
    char copy[100], *s;

    DXMarkTime("start output fb");

    /* strip :0 off end of name */
    strcpy(copy, name? name : "");
    for (s=copy; *s; s++)
	if (*s==':')
	    *s = '\0';
    DXDebug("X", "sending to FB '%s'", copy);

    /* composite image size */
    if (!DXGetImageBounds(o, &arg.ox, &arg.oy, &arg.width, &arg.height))
	return NULL;

    /* is it a simple FB image? */
    fbimage = (DXGetObjectClass(o)==CLASS_FIELD
	       && DXGetComponentValue((Field)o,IMAGE));

    /* wait for pending send */
    if (fb->buffer || fb->object) {
	rc = afb_send_wait(fb->afb_id);
	if (rc<0) {
	    DXSetError(ERROR_INTERNAL, "fb wait: %s", afb_errmsg(rc));
	    goto error;
	}
	DXFree(fb->buffer);
	DXDelete(fb->object);
	fb->buffer = NULL;
	fb->object = NULL;
	DXMarkTime("end wait");
    }

    if (fbimage) {

	struct fbpixel *pixels = _dxf_GetFBPixels((Field)o);

	/* send it */
	if (!pixels)
	    goto error;
	if (!send(copy, fb, (void*)pixels, arg.width, arg.height, x, y))
	    goto error;
	if (fb->nb)
	    fb->object = DXReference(o);

    } else {

	/* allocate a buffer large enough for the entire image */
	int oldw, oldh, neww, newh;

	/* extra margins for 4-pixel alignment */
	oldw = arg.width;
	oldh = arg.height;
	neww = arg.width  = (oldw & 0x3) ? (oldw & (~3)) + 4 : oldw;
	newh = arg.height = (oldh & 0x3) ? (oldh & (~3)) + 4 : oldh;

	/* allocate buffer, zero extra "edges", or whole thing if composite */
	n = neww * newh * sizeof(struct fbpixel);
	fb->buffer = DXAllocate(n + ONE_KB);
	if (!fb->buffer)
	    return NULL;
	arg.fb = (struct fbpixel *)(((int)fb->buffer+ONE_KB-1) & ~(ONE_KB-1));
	if (DXGetObjectClass(o)==CLASS_FIELD) {
	    for (i=0, pp=arg.fb;  i<newh-oldh;  i++, pp+=neww)
		for (j=0, p=pp;  j<neww;  j++, p++)
		    *(int*)p = FBPIXEL(0,0,0);
	    for (i=i, pp=pp;  i<newh;  i++, pp+=neww)
		for (j=oldw, p=pp+oldw;  j<neww;  j++, p++)
		    *(int*)p = FBPIXEL(0,0,0);
	} else {
	    /* zero whole thing if composite, to avoid holes */
	    struct arg2 arg2;
	    arg2.fb = arg.fb;
	    arg2.width = arg.width;
	    arg2.height = arg.height;
	    arg2.n = DXProcessors(0);
	    DXCreateTaskGroup();
	    for (arg2.i=0; arg2.i<arg2.n; arg2.i++)
		DXAddTask((Error(*)(Pointer))task2, (Pointer)&arg2,
			sizeof(arg), 1.0);
	    if (!DXExecuteTaskGroup())
		goto error;
	}
	DXMarkTime("init buffer");

	/* convert in parallel */
	DXDebug("X", "convert after render");
	arg.o = o;
	arg.n = DXProcessors(0);
	DXCreateTaskGroup();
	for (arg.i=0; arg.i<arg.n; arg.i++)
	    DXAddTask((Error(*)(Pointer))task, (Pointer)&arg, sizeof(arg), 1.0);
	if (!DXExecuteTaskGroup())
	    goto error;

	/* send the image */
	if (!send(copy, fb, (void*)arg.fb, neww, newh, x, y))
	    goto error;
	if (!fb->nb) {
	    DXFree(fb->buffer);
	    fb->buffer = NULL;
	}
    }

    /* free buffer and return */
    DXMarkTime("end output fb");
    return o;

error:
    DXFree(fb->buffer);
    DXDelete(fb->object);
    fb->buffer = NULL;
    fb->object = NULL;
    return NULL;
}


/*
 * FB images have type char, each item is four chars
 */

Field
_dxf_MakeFBImage(int width, int height)
{
    Field i;
    Array a;

    /* sanity check */
    if ((double)width*height*3*sizeof(float) > (double)(unsigned int)(-1)) {
	DXSetError(ERROR_BAD_PARAMETER,
		 "image size %dx%d exceeds address space", width, height);
	return NULL;
    }

    /* an image is a field */
    i = DXNewField();
    if (!i)
	return NULL;

    /* positions */
    a = DXMakeGridPositions(2, height,width,
			  /* origin: */ 0.0,0.0,
			  /* deltas: */ 0.0,1.0, 1.0,0.0);
    if (!a)
	return NULL;
    DXSetComponentValue(i, POSITIONS, (Object)a);

    /* connections */
    a = DXMakeGridConnections(2, height,width);
    if (!a)
	return NULL;
    DXSetConnections(i, "quads", a);

    /* colors */
    a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, 4);
    if (!a)
	return NULL;
    if (!DXAddArrayData(a, 0, width*height, NULL)) {
	DXAddMessage("can't make %dx%d image", width, height);
	return NULL;
    }
    DXSetComponentValue(i, IMAGE, (Object)a);
    DXSetAttribute((Object)a, IMAGE_TYPE, O_FB_IMAGE);

    /* set default ref and dep */
    return DXEndField(i);
}


struct fbpixel *
_dxf_GetFBPixels(Field i)
{
    Array a;

    /* return the colors component */
    a = (Array) DXGetComponentValue(i, IMAGE);
    if (!a || DXGetAttribute((Object)a,IMAGE_TYPE) != O_FB_IMAGE)
	DXErrorReturn(ERROR_BAD_PARAMETER, "not an FB image field");
    if (!DXTypeCheck(a, TYPE_UBYTE, CATEGORY_REAL, 1, 4))
	DXErrorReturn(ERROR_BAD_PARAMETER, "FB image has wrong type");
    return (struct fbpixel *) DXGetArrayData(a);
}


Field
_dxf_ZeroFBPixels(Field image, int left, int right, int top, int bot, RGBColor c)
{
    int bwidth, n, bheight, iwidth, iheight, x, y;
    int *tt, *t, *pixels;
    int pix;
    int r, g, b;

    bwidth = right-left;
    n = bwidth * sizeof(int);
    bheight = top-bot;
    pixels = (int *) _dxf_GetFBPixels(image);

    DXGetImageSize(image, &iwidth, &iheight);
    tt = pixels + (iheight-bot-1)*iwidth + left;

    r = (int)255*c.r;
    g = (int)255*c.g;
    b = (int)255*c.b;
    pix = FBPIXEL(r,g,b);

    for (y=0; y<bheight; y++, tt-=iwidth)
	for (x=0, t=tt; x<bwidth; x++, t++)
	    *t = pix;

    return image;
}


#else /* !no libiop (not ibmpvs) */


Object
DXDisplayFB(Object o, char *name, int x, int y)
{
    DXErrorReturn(ERROR_BAD_PARAMETER,
		"frame buffer output not supported on this machine");
}

struct fbpixel
*_dxf_GetFBPixels(Field i)
{
    DXErrorReturn(ERROR_BAD_PARAMETER,
		"frame buffer output not supported on this machine");
}

Field
_dxf_MakeFBImage(int width, int height)
{
    DXErrorReturn(ERROR_BAD_PARAMETER,
		"frame buffer output not supported on this machine");
}

Field
_dxf_ZeroFBPixels(Field image, int left, int right, int top, int bot, RGBColor c)
{
    DXErrorReturn(ERROR_BAD_PARAMETER,
		"frame buffer output not supported on this machine");
}


#endif
