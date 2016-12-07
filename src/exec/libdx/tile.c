/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <dx/dx.h>
#include "internals.h"
#include "render.h"

#ifdef os2
#include <float.h>
#endif

Field DXMakeImageFormat(int, int, char *);


/*
 * default tiling parameters
 */

static struct tile default_tile = {
    0,			/* ignore */
    1,			/* fast_exp */
    0,			/* flat_x */
    0,			/* flat_y */
    1,			/* flat_z */
    0,			/* skip */
    0,			/* volume patch size */
    1,			/* color multiplier */
    1,			/* opacity multiplier */
    0			/* perspective */
};


/*
 * struct patch - Argument block to _Patch function
 */

struct patch {				/* per-patch information */
    int      x, y;			/* indices of patch for debugging */
    int      bot, left, right, top;	/* image coordinates of patch */
    int      tfields;			/* number of translucent fields */
    int      nconnections;		/* number of connections */
    int      nbytes;			/* number of bytes for sort array */
    RGBColor background;		/* background color (from camera) */
    struct common {			/* shared info */
	Object o;			/* what to render */
	Field image;			/* output image */
	int perspective;		/* perspective camera being used? */
	enum img_type img_type;		/* type of output image */
	int regular;			/* whether to use regular or irreg */
	int volume;			/* number of volume fields */
	int fast;			/* whether to use small fb */
	int width, height;		/* size of output image */
					/* info for cleanup() follows */
	lock_type DXlock;			/* DXlock for the count */
	volatile int count;		/* count of tasks still using o */
	lock_type done;			/* unlocked when all done */
    } *common;
};


/*
 * patch - Renders an image patch according to the arguments
 * supplied by the struct patch argument block.
 * Be sure always to call finished(common) before returning
 * from patch, otherwise the cleanup task will wait forever
 */

static void
finished(struct common *common)
{
    DXlock(&common->DXlock, 0);
    if (--(common->count)==0)
	DXunlock(&common->done, 0);
    DXunlock(&common->DXlock, 0);
}


static Error
patch(Pointer ptr)
{
    struct buffer b;
    int i, nbytes;
    int clip_status;
    struct gather gather;
    struct patch *p = (struct patch *)ptr;
    struct common *c = p->common;
    enum pix_type pix_type;


    /* startup, debugging */
    DXDebug("R", "patch %d,%d (%d to %d, %d to %d) (%d elements):",
	  p->x, p->y, p->left, p->right, p->bot, p->top, p->nconnections);
    memset(&b, 0, sizeof(b));
    memset(&gather, 0, sizeof(gather));
    DXExtractInteger(DXGetAttribute(c->o, "init"), &b.init);
    DXExtractInteger(DXGetAttribute(c->o, "showpatches"), &b.showpatches);
    DXExtractInteger(DXGetAttribute(c->o, "notriangles"), &b.notriangles);
    DXExtractInteger(DXGetAttribute(c->o, "setuponly"), &b.setuponly);
    DXExtractInteger(DXGetAttribute(c->o, "tri"), &b.tri);
    DXExtractInteger(DXGetAttribute(c->o, "irregular"), &b.irregular);
    b.vol = NULL;
    DXExtractString(DXGetAttribute(c->o, "vol"), &b.vol);
    DXMarkTimeLocal("startup");

    /* short circuit empty patches */
    if (!p->nconnections) {
	_dxf_ZeroPixels(c->image, p->left, p->right, p->top, p->bot, p->background);
	finished(c);
	DXMarkTimeLocal("empty");
	return OK;
    }

    /* perspective flag XXX - reentrancy? */
    default_tile.perspective = c->perspective;

    /* gather translucent fields and/or faces */
    if (p->tfields) {
	gather.cull = !c->perspective;
	gather.min = DXPt(p->left-c->width/2, p->bot-c->height/2, 0);
	gather.max = DXPt(p->right-c->width/2, p->top-c->height/2, 0);
	gather.nfields = 0;
	gather.fields = (struct xfield *)
	    DXAllocateLocal((p->tfields+1) * sizeof(struct xfield));
	if (!gather.fields)
	    goto error;
	gather.clipping = NULL;
	if (c->regular) {
	    if (!_dxfGather(c->o, &gather, &default_tile))
		goto error;
	    DXASSERTGOTO(gather.nfields == p->tfields);
	} else {
	    gather.sort = (struct sort *) DXAllocateLocal(p->nbytes);
	    if (!gather.sort) {
		DXResetError();
		gather.sort = (struct sort *) DXAllocate(p->nbytes);
		if (!gather.sort)
		    goto error;
		DXWarning("#4880",p->nbytes);
	    }
	    gather.current = gather.sort;
	    if (!_dxfGather(c->o, &gather, &default_tile))
		goto error;
	    nbytes = (long)gather.current - (long)gather.sort;
	    DXDebug("R", "    IRREGULAR: estimated %d bytes, actual %d bytes",
		  p->nbytes, nbytes);
	    DXASSERTGOTO(nbytes <= p->nbytes);
	    DXASSERTGOTO(gather.nfields == p->tfields);
	    /* XXX - sometimes sgi system realloc moves this array! */
	    if (nbytes != p->nbytes)
		gather.sort = (struct sort *)
		    DXReAllocate((Pointer)gather.sort, nbytes);
	    gather.current = (struct sort *) ((long)gather.sort + nbytes);
	    DXMarkTimeLocal("gather");
	}
    }

    /* what pixel type? */
    if (!c->fast)
	pix_type = pix_big;
    else
	pix_type = pix_fast;

    /* rendering buffer (after sort array is reallocated) */
    if (!_dxf_InitBuffer(&b, pix_type,
		     p->right-p->left - b.showpatches,
		     p->top-p->bot - b.showpatches,
		     c->width/2-p->left, c->height/2-p->bot, p->background))
	goto error;
    b.iwidth = c->width;
    if (c->img_type==img_fb) {
	b.img.fb = _dxf_GetFBPixels(c->image);
	b.img.fb += c->width*(c->height-1-p->bot) + p->left;
    }
    DXMarkTimeLocal("init buffer");

    /* opaque surfaces */
    if (!_dxfPaint(c->o, &b, UNCLIPPED, &default_tile))
	goto error;
    DXMarkTimeLocal("paint");

    /* translucent face/volume clipping surface, if any */
    clip_status = UNCLIPPED;
    if (gather.clipping) {
	_dxf_BeginClipping(&b);
	if (!_dxfPaint(gather.clipping, &b, CLIPPING, &default_tile))
	    goto error;
	_dxfCapClipping(&b, DXD_MAX_FLOAT);
	_dxf_MergeBackIntoZ(&b);
	clip_status = NEST(clip_status);
	DXMarkTimeLocal("merge");
    }

    /* sort and paint the translucent faces and/or volumes */
    if (c->regular) {
	if (!_dxf_VolumeRegular(&b, &gather, clip_status))
	    goto error;
    } else {
	if (!_dxf_VolumeIrregular(&b, &gather, clip_status))
	    goto error;
    }
    DXMarkTimeLocal("volume");

    /* delete local copies, then we're done with c->o */
    if (gather.fields)
	for (i=0; i<gather.nfields; i++)
	    _dxf_XFreeLocal(&gather.fields[i]);
    DXFree((Pointer)gather.sort);
    finished(c);
    DXMarkTimeLocal("free local");

    /* transfer from rendering buffer to image */
    if (!_dxf_CopyBufferToImage(&b, c->image, p->left, p->bot))
	goto error;
    DXMarkTimeLocal("copy");

    /* clean up and return */
    DXDebug("R", "    %d triangles, %d pixels", b.triangles, b.pixels);
    DXFree((Pointer)gather.fields);
    DXFree((Pointer)b.buffer);
    DXMarkTimeLocal("free");
    return OK;

error:
    finished(c);
    DXFree((Pointer)gather.sort);
    DXFree((Pointer)gather.fields);
    DXFree((Pointer)b.buffer);
    return ERROR;
}


static Error
cleanup(Pointer ptr)
{
    struct common *common = (struct common *)ptr;

    /* if everyone's done with common->o, delete it */
    if (common->count) {
	DXlock(&common->done, 0);
	DXunlock(&common->done, 0);
    }
    DXMarkTimeLocal("idle (delete)");
    DXDelete(common->o);
    DXMarkTimeLocal("delete");
    return OK;
}




Field
DXRender(Object o, Camera camera, char *format)
{
    struct count *count;
    struct pcount *pc;
    struct patch *p, *patches;
    struct common c, *cp;
    int ci, irregular, n, i, j;
    int nosplit;
    int x, y, left, bot, lt, rt=0, tp=0, bt, split;
    Point box[8];
    float width;
    int	depth;
    RGBColor background;
    int subdiv;
    
#ifdef os2
           /* disable underflow errors. This may happen during the
            * calculation of the specular shininess
            */
            _control87(EM_UNDERFLOW, EM_UNDERFLOW);
#endif

    /* initial values */
    DXMarkTimeLocal("start render");
    c.image = NULL;
    c.o = NULL;
    count = NULL;
    c.count = 0;
    cp = NULL;
    patches = NULL;

    /* ci^2 patches */
    n = DXProcessors(0);
    ci = n>1? (int)(2.3 * sqrt((float)n)) : 1;
    DXExtractInteger(DXGetAttribute(o, "ci"), &ci);
    DXDebug("R", "%dx%d patches", ci, ci);

    /* allocate image */
    if (DXGetObjectClass((Object)camera) != CLASS_CAMERA)
	DXErrorReturn(ERROR_BAD_PARAMETER, "#13590");
    DXGetCameraResolution(camera, &c.width, &c.height);

    /* fuzz units and perspective flag */
    if (DXGetOrthographic(camera, &width, NULL))
	c.perspective = 0;
    else if (DXGetPerspective(camera, &width, NULL))
	c.perspective = 1;
    else
	DXErrorGoto(ERROR_INTERNAL, "#13230");
    
    if (! DXGetBackgroundColor(camera, &background))
	DXErrorGoto(ERROR_INTERNAL, "#13600");

#if defined(DX_NATIVE_WINDOWS)
	if (c.width & 0x1) 
		c.width += 1;
#endif
	       
    if (format) {
#if DXD_HAS_LIBIOP
	if (strncmp(format,"FB",2)==0) {
	    /* round up to multiple of 4 for frame buffer plus 4 */
	    /* (note extra 4 are assumed in image.c) */
            c.height = (c.height & 0x3) ? (c.height & (~3)) + 4 : c.height;
            c.width  = (c.width & 0x3) ? (c.width & (~3)) + 4 : c.width;
	    c.image = _dxf_MakeFBImage(c.width, c.height);
	    c.img_type = img_fb;
	    DXDebug("R", "FB format");
	} else
#endif
	/* XXX - extract host name */
	if (format[0]=='X') {
	    char depth_str[201];
	    char *s;

	    strcpy(depth_str, format);

	    for (s = depth_str, i = 0; *s && *s != ','; s++) continue;
	    if (*s == ',') *s++ = '\0';

	    if((depth = _dxf_getXDepth(depth_str)) < 0) return NULL;

	    c.image = _dxf_MakeImage(c.width, 
				      c.height,
				      depth,
				      format);
	    c.img_type = img_x;
	    DXDebug("R", "X format");
	} else if(format[0]=='W') { 
		// Make c.image as compatible with a Windows DIB bitmap as possible.
		c.image = _dxf_MakeImage(c.width, 
			c.height, 
			24,
			format);
		c.img_type = img_rgb;
	}
	else {
	    c.image = DXMakeImageFormat(c.width, c.height, format);
	    c.img_type = img_rgb;
	}
    } else {
	c.image = DXMakeImageFormat(c.width, c.height, NULL);
	c.img_type = img_rgb;
    }

    if (!c.image) goto error;
    DXMarkTime("make image");

    /* associate camera and object box with image */
    /* XXX - DXBoundingBox requires extra traversal; is there a better way? */
    if (DXValidPositionsBoundingBox(o, box)) {
	Array a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	DXSetAttribute((Object)c.image, "object box", (Object)a);
	if (!(DXAddArrayData(a, 0, 8, (Pointer)box)))
	    goto error;
    }
    DXSetAttribute((Object)c.image, "camera", (Object)camera);

    /* shade/count */
    count = (struct count *) DXAllocate(sizeof(struct count));
    if (!count)
	goto error;

#define MAX_SUBDIV 3

    for (subdiv = 0; subdiv < MAX_SUBDIV; subdiv++, ci *= 2)
    {
	DXcreate_lock(&count->DXlock, 0);

	count->nx = count->ny = ci;
	count->width = c.width;
	count->height = c.height;
	count->irregular = 0;
	count->volume = 0;
	count->regular = 0;
	count->fast = 1;
	count->pcount = 
		(struct pcount *)DXAllocateZero(ci*ci*sizeof(struct pcount));
	if (!count->pcount) goto error;
	c.o = (Object)_dxfXShade(o, camera, count, box);
	if (!c.o)
	{
	    if (DXGetError() == ERROR_NO_MEMORY)
		goto out_of_memory;
	    else
		goto error;
	}

	irregular = (DXGetAttribute(c.o, "irregular") != NULL);
	if (count->irregular==0 && count->regular<=1 && !irregular) {
	    DXDebug("R", "REGULAR");
	    c.regular = 1;
	} else
	    c.regular = 0;

	c.volume = count->volume;
	DXDebug("R", count->fast? "FAST" : "BIG");
	c.fast = count->fast;
	DXMarkTime("end shade");

	/* put common in global memory, allocate array of patch structs */
	cp = (struct common *) DXAllocate(sizeof(struct common));
	if (!cp)
	    goto out_of_memory;

	*cp = c;

	DXcreate_lock(&cp->DXlock, "cleanup DXlock");
	DXcreate_lock(&cp->done,   "done DXlock");    

	DXlock(&cp->done, 0);

	patches = (struct patch *) DXAllocate(3*sizeof(struct patch)*ci*ci);
	if (!patches)
	    goto out_of_memory;

	/* splitting threshhold */
	for (n=ci*ci, pc=count->pcount, i=0, split=0; i<n; i++, pc++)
	    split += pc->nconnections;

	split /= ci*ci;			/* average size */
	split *= 6;			/* split things 6x average size */

	/* do the screen patches in parallel */
	nosplit = getenv("SPLIT")? 0 : 1;
	DXCreateTaskGroup();

	for (y=0, bot=0, pc=count->pcount, p=patches;  y<ci;  y++, bot=tp) {
	    for (x=0, left=0;  x<ci;  x++, left=rt, pc++) {
		int n = pc->nconnections;
		int tfields = pc->tfields;
		int nbytes = pc->nbytes;
		int s, sx, sy, sci;
		if (n>split && !nosplit) {
		    s = 2;
		    DXDebug("R", "splitting %d,%d (%d>%d)", x, y, n, split);
		    s=2, sy=2*y, sx=2*x, sci=2*ci;
		} else
		    s=1, sy=1*y, sx=1*x, sci=1*ci;
		for (j=0, bt=bot; j<s; j++, bt=tp) {
		    tp = (sy+j+1) * c.height / sci;
		    for (i=0, lt=left; i<s; i++, lt=rt, p++) {
			rt = (sx+i+1) * c.width / sci;
			p->x = x;
			p->y = y;
			p->bot = bt;
			p->left = lt;
			p->right = rt;
			p->top = tp;
			p->tfields = tfields;
			p->nconnections = n;
			p->nbytes = nbytes;
			p->common = cp;
			memcpy((char *)&(p->background), (char *)&background,
							    sizeof(RGBColor));
			if (!DXAddTask(patch, (Pointer)p, 0, n))
			{
			    DXAbortTaskGroup();
			    if (DXGetError() == ERROR_NO_MEMORY)
				goto out_of_memory;
			    else
				return NULL;
			}
			cp->count++;
		    }
		}
	    }
	}
	if (!DXAddTask(cleanup, (Pointer)cp, 0, -1))
	{
	    DXAbortTaskGroup();
	    if (DXGetError() == ERROR_NO_MEMORY)
		goto out_of_memory;
	    else
		return NULL;
	}

	if (!DXExecuteTaskGroup())
	{
	    if (DXGetError() == ERROR_NO_MEMORY)
		goto out_of_memory;
	    else
		goto error;
	}
	else 
	    break;

out_of_memory:
	DXResetError();
	if (cp) {
	    DXtry_lock(&cp->DXlock, 0);
	    DXunlock(&cp->DXlock, 0);
	    DXdestroy_lock(&cp->DXlock);

	    DXtry_lock(&cp->done, 0);
	    DXunlock(&cp->done, 0);
	    DXdestroy_lock(&cp->done);

	    DXtry_lock(&count->DXlock, 0);
	    DXunlock(&count->DXlock, 0);
	    DXdestroy_lock(&count->DXlock);

	    DXFree((Pointer)cp);
	    cp = NULL;
	}
	DXFree((Pointer)patches);
	patches = NULL;
	DXFree((Pointer)count->pcount);
	count->pcount = NULL;
    }

    if (subdiv == MAX_SUBDIV)
    {
	DXSetError(ERROR_NO_MEMORY, "screen subdivision failed");
	goto error;
    }


    /* clean up and go home */
    DXdestroy_lock(&cp->DXlock);
    DXdestroy_lock(&cp->done);
    DXFree((Pointer)cp);
    DXFree((Pointer)patches);
    DXFree((Pointer)count->pcount);
    DXFree((Pointer)count);

#ifdef os2
            _control87(0, EM_UNDERFLOW);
#endif

    return c.image;

error:
    if (cp) {
	DXdestroy_lock(&cp->DXlock);
	DXdestroy_lock(&cp->done);
	DXFree((Pointer)cp);
    }
    DXFree((Pointer)patches);
    if (count) {
	DXFree((Pointer)count->pcount);
	DXFree((Pointer)count);
    }
    DXDelete((Object)c.image);

#ifdef os2
            _control87(0, EM_UNDERFLOW);
#endif

    return NULL;
}


