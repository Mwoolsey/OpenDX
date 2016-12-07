#include <dxconfig.h>
#include <string.h>
#include <dx/dx.h>
#include "render.h"
#include "internals.h"
#include "displayutil.h"
Error
_dxf_InitBuffer(struct buffer *b, enum pix_type pix_type,
     int width, int height, int ox, int oy, RGBColor bkgnd)
{
    register int i, n;
    int init;
    float br, bg, bb;
    init = b->init;
    n = width*height;
    br = bkgnd.r;
    bg = bkgnd.g;
    bb = bkgnd.b;
    if (pix_type==pix_fast) {
 b->buffer = DXAllocateLocal(n*sizeof(struct fast) + 16);
 if (!b->buffer) {
     DXResetError();
     b->buffer = DXAllocate(n*sizeof(struct fast));
     if (!b->buffer)
  return ERROR;
 }
 b->u.fast = (struct fast *) ((((long)b->buffer)+16 -1) & ~(16 -1));
    } else if (pix_type==pix_big) {
 b->buffer = DXAllocateLocal(n*sizeof(struct big));
 if (!b->buffer) {
     DXResetError();
     b->buffer = DXAllocate(n*sizeof(struct big));
     if (!b->buffer)
  return ERROR;
 }
 b->u.big = (struct big *) b->buffer;
    } else if (pix_type==pix_zbuf) {
 b->buffer = DXAllocateLocal(n*sizeof(struct zbuf));
 if (!b->buffer) {
     DXResetError();
     b->buffer = DXAllocate(n*sizeof(struct zbuf));
     if (!b->buffer)
  return ERROR;
 }
 b->u.zbuf = (struct zbuf *) b->buffer;
    }
    b->pix_type = pix_type;
    b->width = width;
    b->height = height;
    b->ox = ox;
    b->oy = oy;
    b->min = DXPt(-ox, -oy, 0);
    b->max = DXPt(width-ox, height-oy, 0);
    b->omin = DXPt(DXD_MAX_FLOAT, DXD_MAX_FLOAT, 0);
    b->omax = DXPt(DXD_MIN_FLOAT, DXD_MIN_FLOAT, 0);
    if (pix_type==pix_fast) {
 struct fast *f;
 for (i=0, f=b->u.fast; i<n; i++, f++) {
     f->c.r = br;
     f->c.g = bg;
     f->c.b = bb;
     f->z = -DXD_MAX_FLOAT;
 }
    } else if (pix_type==pix_zbuf) {
 struct zbuf *p;
 for (i=0, p=b->u.zbuf; i<n; i++, p++)
     p->z = -DXD_MAX_FLOAT;
    } else if (init==0) {
 struct big *p;
 for (i=0, p=b->u.big; i<n; i++, p++) {
     p->c.r = br;
     p->c.g = bg;
     p->c.b = bb;
     p->co.r = 0;
     p->co.g = 0;
     p->co.b = 0;
     p->co.o = 0;
     p->z = -DXD_MAX_FLOAT;
     p->front = DXD_MAX_FLOAT;
     p->back = -DXD_MAX_FLOAT;
     p->in = 0;
 }
    } else if (init==1) {
 static struct big init, *p;
 static RGBColor c = {0, 0, 0};
 static struct rgbo co = {0, 0, 0, 0};
 init.c = c;
 init.co = co;
 init.z = -DXD_MAX_FLOAT;
 init.front = DXD_MAX_FLOAT;
 init.back = -DXD_MAX_FLOAT;
 init.in = 0;
 for (i=0, p=b->u.big; i<n; i++, p++)
     *p = init;
    } else if (init==2) {
 struct big *p;
 memset(b->u.big, 0, n*sizeof(struct big));
 for (i=0, p=b->u.big; i<n; i++, p++) {
     p->z = -DXD_MAX_FLOAT;
     p->front = DXD_MAX_FLOAT;
     p->back = -DXD_MAX_FLOAT;
 }
    }
    return OK;
}
extern int XImageByteOrder(
);
Error
_dxf_BeginClipping(struct buffer *b)
{
    int n, i;
    n = b->width*b->height;
    for (i=0; i<n; i++) {
 b->u.big[i].front = -DXD_MAX_FLOAT;
 b->u.big[i].back = DXD_MAX_FLOAT;
    }
    return OK;
}
Error
_dxfCapClipping(struct buffer *b, float nearPlane)
{
    int n = b->height*b->width;
    struct big *pix= b->u.big;
    while (--n >= 0)
    {
 if (pix->front == pix->back)
     pix->front = nearPlane;
 pix ++;
    }
    return OK;
}
Error
_dxf_MergeBackIntoZ(struct buffer *b)
{
    int n, i;
    struct big *pix;
    if (b->merged)
 return OK;
    n = b->width*b->height;
    for (i=0, pix=b->u.big; i<n; i++, pix++)
 if (pix->back > pix->z)
     pix->z = pix->back;
 else
     pix->back = pix->z;
    b->merged = 1;
    return OK;
}
Error
_dxf_EndClipping(struct buffer *b)
{
    int n, i;
    n = b->width*b->height;
    for (i=0; i<n; i++) {
 b->u.big[i].front = DXD_MAX_FLOAT;
 b->u.big[i].back = -DXD_MAX_FLOAT;
    }
    return OK;
}
Error
_dxf_CopyBufferToImage(struct buffer *b, Field i, int xoff, int yoff)
{
    int iwidth, bheight=b->height, y;
    register int x, bwidth=b->width;
    Object o;
    Type type;
    Array a;
    o = DXGetComponentAttribute(i, IMAGE, IMAGE_TYPE);
    if (o==O_FB_IMAGE)
 return _dxf_CopyBufferToFBImage(b, i, xoff, yoff);
    else if (o==O_X_IMAGE)
 return _dxf_CopyBufferToPImage(b, i, xoff, yoff);
    else if (o)
 DXErrorReturn(ERROR_BAD_PARAMETER, "#11600");
    if (!DXGetImageSize(i, &iwidth, NULL))
 return ERROR;
    a = (Array)DXGetComponentValue(i, COLORS);
    if (! a)
 return ERROR;
    DXGetArrayInfo(a, NULL, &type, NULL, NULL, NULL);
    if (type == TYPE_FLOAT)
    {
 if (b->pix_type == pix_fast)
 {
     { RGBColor *pixels = (RGBColor *)DXGetArrayData(a); RGBColor *t; register RGBColor *tt; struct fast *ff; register struct fast *f; ff = b->u.fast; tt = pixels + yoff*iwidth + xoff; for (y=0; y<bheight; y++) { for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)
     t->r = f->c.r, t->g = f->c.g, t->b = f->c.b;
     ff += bwidth; tt += iwidth; } }
 }
 else if (b->pix_type == pix_big)
 {
     { RGBColor *pixels = (RGBColor *)DXGetArrayData(a); RGBColor *t; register RGBColor *tt; struct big *ff; register struct big *f; ff = b->u.big; tt = pixels + yoff*iwidth + xoff; for (y=0; y<bheight; y++) { for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)
     t->r = f->c.r, t->g = f->c.g, t->b = f->c.b;
     ff += bwidth; tt += iwidth; } }
 }
    }
    else if (type == TYPE_UBYTE)
    {
 if (b->pix_type == pix_fast)
 {
     { RGBByteColor *pixels = (RGBByteColor *)DXGetArrayData(a); RGBByteColor *t; register RGBByteColor *tt; struct fast *ff; register struct fast *f; ff = b->u.fast; tt = pixels + yoff*iwidth + xoff; for (y=0; y<bheight; y++) { for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)
     { t->r = (f->c.r >= 1.0) ? 255 : (f->c.r <= 0.0) ? 0 : 255 * f->c.r; t->g = (f->c.g >= 1.0) ? 255 : (f->c.g <= 0.0) ? 0 : 255 * f->c.g; t->b = (f->c.b >= 1.0) ? 255 : (f->c.b <= 0.0) ? 0 : 255 * f->c.b; }
     ff += bwidth; tt += iwidth; } }
 }
 else if (b->pix_type == pix_big)
 {
     { RGBByteColor *pixels = (RGBByteColor *)DXGetArrayData(a); RGBByteColor *t; register RGBByteColor *tt; struct big *ff; register struct big *f; ff = b->u.big; tt = pixels + yoff*iwidth + xoff; for (y=0; y<bheight; y++) { for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)
     { t->r = (f->c.r >= 1.0) ? 255 : (f->c.r <= 0.0) ? 0 : 255 * f->c.r; t->g = (f->c.g >= 1.0) ? 255 : (f->c.g <= 0.0) ? 0 : 255 * f->c.g; t->b = (f->c.b >= 1.0) ? 255 : (f->c.b <= 0.0) ? 0 : 255 * f->c.b; }
     ff += bwidth; tt += iwidth; } }
 }
    }
    a = (Array)DXGetComponentValue(i, "zbuffer");
    if (a)
    {
 if (b->pix_type == pix_fast)
 {
     { float *z_pixels = (float *)DXGetArrayData(a); float *t; register float *tt; struct fast *ff; register struct fast *f; ff = b->u.fast; tt = z_pixels + yoff*iwidth + xoff; for (y=0; y<bheight; y++) { for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)
     *t = f->z;
     ff += bwidth; tt += iwidth; } }
 }
 else if (b->pix_type == pix_big)
 {
     { float *z_pixels = (float *)DXGetArrayData(a); float *t; register float *tt; struct big *ff; register struct big *f; ff = b->u.big; tt = z_pixels + yoff*iwidth + xoff; for (y=0; y<bheight; y++) { for (x=0, f=ff, t=tt; x<bwidth; x++, f++, t++)
     *t = f->z;
     ff += bwidth; tt += iwidth; } }
 }
    }
    return OK;
}
Error
_dxf_CopyBufferToFBImage(struct buffer *buf, Field i, int xoff, int yoff)
{
    return OK;
}
Error
_dxf_CopyFace(struct buffer *b, int if_object)
{
    int i, j, x, y, nx, ny;
    struct big *pix, *p;
    if (b->fmax.x<b->fmin.x || b->fmax.y<b->fmin.y)
 return OK;
    x = (int)b->fmin.x;
    y = (int)b->fmin.y;
    nx = (int)b->fmax.x - x + 1;
    ny = (int)b->fmax.y - y + 1;
    if (x < 0) nx+=x, x=0;
    if (x+nx >= b->width) nx = b->width-x;
    if (y < 0) ny+=y, y=0;
    if (y+ny >= b->height) ny = b->height-y;
    if (if_object) if_object = IF_OBJECT;
    pix = b->u.big + y*b->width;
    for (i=0; i<ny; i++, pix+=b->width) {
 for (j=0, p=pix+x; j<nx; j++, p++) {
     int in = p->in;
     if (in & IN_FACE) {
  if (p->co.o>p->z && p->co.o<p->front && p->co.o>p->back) {
      p->c.r=p->co.r;
      p->c.g=p->co.g;
      p->c.b=p->co.b;
      p->z=p->co.o;
  }
  in &= ~IN_FACE;
  in ^= if_object;
  p->in = in;
     }
 }
    }
    return OK;
}
Error
_dxf_InterferenceObject(struct buffer *buf)
{
    int xmin, xmax, ymin, ymax, x, y;
    int width = buf->width, height = buf->height;
    struct big *pp, *p;
    xmin = ((int)((buf->omin.x)-30000.0)+30000);
    ymin = ((int)((buf->omin.y)-30000.0)+30000);
    xmax = ((int)((buf->omax.x)-30000.0)+30000);
    ymax = ((int)((buf->omax.y)-30000.0)+30000);
    if (xmin<0) xmin = 0; else if (xmin>=width) xmin = width;
    if (xmax<0) xmax = 0; else if (xmax>=width) xmax = width;
    if (ymin<0) ymin = 0; else if (ymin>=height) ymin = height;
    if (ymax<0) ymax = 0; else if (ymax>=height) ymax = height;
    for (y=ymin, pp=buf->u.big+y*width; y<ymax; y++, pp+=width) {
 for (x=xmin, p=pp+xmin; x<xmax; x++, p++) {
     int in = p->in;
     if (in & IF_OBJECT) {
  if (in & IF_UNION)
      in |= IF_INTERSECTION;
  in |= IF_UNION;
  in &= ~IF_OBJECT;
  p->in = in;
     }
 }
    }
    return OK;
}
static Field decode_8bit(Array encodedImage) 
{ 
 Field pixField = NULL; 
 Array a = NULL; 
 int i, j, image_size, flag, knt; 
 ubyte *ptr = (ubyte *)DXGetArrayData(encodedImage); 
 int *res, x, y; 
 ubyte *pix; 
 
 a = (Array)DXGetAttribute((Object)encodedImage, "resolution"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, 
 "cached image missing resolution attrbute"); 
 goto error; 
 } 
 
 res = DXGetArrayData(a); 
 x = res[0]; y = res[1]; 
 
 pixField = DXNewField(); 
 if (! pixField) 
 goto error; 
 
 a = DXMakeGridPositions(2, x, y, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0); 
 if (! a) 
 goto error; 
 
 if (! DXSetComponentValue(pixField, POSITIONS, (Object)a)) 
 goto error; 
 
 a = DXMakeGridConnections(2, x, y); 
 if (! a) 
 goto error; 
 if (! DXSetConnections(pixField, "quads", a)) 
 goto error; 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0, 1); 
 if (! a) 
 goto error; 
 if (! DXAddArrayData(a, 0, x*y, NULL)) 
 goto error; 
 if (! DXSetAttribute((Object)a, "8bit", O_X_IMAGE)) 
 goto error; 
 
 pix = (ubyte *)DXGetArrayData(a); 
 
 image_size = x*y; 
 
 for (i = 0; i != image_size; i += knt) 
 { 
 flag = *ptr & 0x80; 
 knt = *ptr & 0x7f; 
 
 ptr ++; 
 
 if (flag) 
 { 
 memcpy((char *)pix, (char *)ptr, sizeof(ubyte)*knt); 
 (*((ubyte *)(pix)) = *((ubyte *)(ptr))); 
 pix += knt*sizeof(ubyte); 
 ptr += knt*sizeof(ubyte); 
 } else { 
 ubyte proto; 
 (*((ubyte *)(&proto)) = *((ubyte *)(ptr))); 
 for (j = 0; j < knt; j++) 
 { 
 *(ubyte *)pix = proto; 
 pix += sizeof(ubyte); 
 } 
 
 ptr += sizeof(ubyte); 
 } 
 } 
 
 if (! DXSetComponentValue(pixField, IMAGE, (Object)a)) 
 goto error; 
 
 return DXEndField(pixField); 
 
error: 
 DXFree((Object)pixField); 
 DXFree((Object)a); 
 
 return NULL; 
}
static Array encode_8bit(Field image, translationP t) 
{ 
 ubyte *start_p, *p; 
 int start_i, i, knt, size; 
 int x, y; 
 Array a = NULL; 
 Object attr = NULL; 
 int image_size, res[2], nd; 
 int segknt, totknt; 
 SegListSegment *seg; 
 SegList *seglist = NULL; 
 ubyte *segptr=NULL; 
 int mask = 0; 
 
 a = (Array)DXGetComponentValue(image, "connections"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "image missing connections"); 
 return NULL; 
 } 
 
 if (!DXQueryGridConnections(a, &nd, res) || nd != 2) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 x = res[0]; 
 y = res[1]; 
 image_size = x*y; 
 
 seglist = DXNewSegList(sizeof(ubyte), 0, 0); 
 if (! seglist) 
 goto error; 
 
 segknt = 0; 
 totknt = 0; 
 
 a = (Array)DXGetComponentValue(image, IMAGE); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 start_i = 0; 
 start_p = (ubyte *)DXGetArrayData(a); 
 a = NULL; 
 
 while (start_i < image_size) 
 { 
 
 i = start_i + 1; 
 p = start_p + sizeof(ubyte); 
 
 while (i < image_size) 
 { 
 if (! (*((ubyte *)(start_p)) == *((ubyte *)(p)))) 
 break; 
 
 i++; 
 p += sizeof(ubyte); 
 } 
 
 knt = i - start_i; 
 
 
 if (knt == 1) 
 { 
 
 while ((!(*((ubyte *)(p+0)) == *((ubyte *)(p+sizeof(ubyte)))) || 
 !(*((ubyte *)(p+0)) == *((ubyte *)(p+2*sizeof(ubyte))))) && i < image_size - 2) 
 i++, p += sizeof(ubyte); 
 
 
 if (i == image_size-2) 
 i = image_size; 
 
 knt = i - start_i; 
 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 int size = length * sizeof(ubyte); 
 
 
 knt -= length; 
 
 
 totknt += (size + 1); 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 *segptr++ = 0x80 | length; 
 segknt --; 
 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 } 
 } 
 else 
 { 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 
 
 knt -= length; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 *segptr++ = length; 
 segknt--; 
 
 size = sizeof(ubyte); 
 totknt += size + 1; 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 
 } 
 } 
 
 start_i = i; 
 start_p = p; 
 } 
 
 DXDebug("Y", "compaction: %d bytes in %d bytes out", 
 image_size*sizeof(ubyte), totknt); 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, sizeof(ubyte)); 
 if (! a) 
 goto error; 
 
 if (! DXAddArrayData(a, 0, totknt, NULL)) 
 goto error; 
 
 start_p = (ubyte *)DXGetArrayData(a); 
 
 DXInitGetNextSegListSegment(seglist); 
 while (totknt > 0) 
 { 
 int s, l; 
 seg = DXGetNextSegListSegment(seglist); 
 if (! seg) 
 { 
 DXSetError(ERROR_INTERNAL, "run_length encoding error"); 
 goto error; 
 } 
 
 s = DXGetSegListSegmentItemCount(seg) * sizeof(ubyte); 
 if (totknt > s) 
 l = s; 
 else 
 l = totknt; 
 
 memcpy(start_p, DXGetSegListSegmentPointer(seg), l); 
 
 start_p += l; 
 totknt -= l; 
 } 
 
 res[0] = x; 
 res[1] = y; 
 
 attr = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0); 
 if (! attr) 
 goto error; 
 
 if (! DXAddArrayData((Array)attr, 0, 2, (Pointer)res)) 
 goto error; 
 
 if (! DXSetAttribute((Object)a, "resolution", attr)) 
 goto error; 
 attr = NULL; 
 
 if (! DXSetAttribute((Object)a, "encoding type", 
 (Object)DXNewString("8bit")))
 goto error; 
 
 DXDeleteSegList(seglist); 
 return a; 
 
error: 
 DXDeleteSegList(seglist); 
 DXDelete((Object)a); 
 
 return ERROR; 
}
static Field decode_12bit(Array encodedImage) 
{ 
 Field pixField = NULL; 
 Array a = NULL; 
 int i, j, image_size, flag, knt; 
 ubyte *ptr = (ubyte *)DXGetArrayData(encodedImage); 
 int *res, x, y; 
 ubyte *pix; 
 
 a = (Array)DXGetAttribute((Object)encodedImage, "resolution"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, 
 "cached image missing resolution attrbute"); 
 goto error; 
 } 
 
 res = DXGetArrayData(a); 
 x = res[0]; y = res[1]; 
 
 pixField = DXNewField(); 
 if (! pixField) 
 goto error; 
 
 a = DXMakeGridPositions(2, x, y, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0); 
 if (! a) 
 goto error; 
 
 if (! DXSetComponentValue(pixField, POSITIONS, (Object)a)) 
 goto error; 
 
 a = DXMakeGridConnections(2, x, y); 
 if (! a) 
 goto error; 
 if (! DXSetConnections(pixField, "quads", a)) 
 goto error; 
 
 a = DXNewArray(TYPE_USHORT, CATEGORY_REAL, 0, 1); 
 if (! a) 
 goto error; 
 if (! DXAddArrayData(a, 0, x*y, NULL)) 
 goto error; 
 if (! DXSetAttribute((Object)a, "12bit", O_X_IMAGE)) 
 goto error; 
 
 pix = (ubyte *)DXGetArrayData(a); 
 
 image_size = x*y; 
 
 for (i = 0; i != image_size; i += knt) 
 { 
 flag = *ptr & 0x80; 
 knt = *ptr & 0x7f; 
 
 ptr ++; 
 
 if (flag) 
 { 
 memcpy((char *)pix, (char *)ptr, sizeof(ushort)*knt); 
 (((char *)(pix))[0] = ((char *)(ptr))[0], ((char *)(pix))[1] = ((char *)(ptr))[1]); 
 pix += knt*sizeof(ushort); 
 ptr += knt*sizeof(ushort); 
 } else { 
 ushort proto; 
 (((char *)(&proto))[0] = ((char *)(ptr))[0], ((char *)(&proto))[1] = ((char *)(ptr))[1]); 
 for (j = 0; j < knt; j++) 
 { 
 *(ushort *)pix = proto; 
 pix += sizeof(ushort); 
 } 
 
 ptr += sizeof(ushort); 
 } 
 } 
 
 if (! DXSetComponentValue(pixField, IMAGE, (Object)a)) 
 goto error; 
 
 return DXEndField(pixField); 
 
error: 
 DXFree((Object)pixField); 
 DXFree((Object)a); 
 
 return NULL; 
}
static Array encode_12bit(Field image, translationP t) 
{ 
 ubyte *start_p, *p; 
 int start_i, i, knt, size; 
 int x, y; 
 Array a = NULL; 
 Object attr = NULL; 
 int image_size, res[2], nd; 
 int segknt, totknt; 
 SegListSegment *seg; 
 SegList *seglist = NULL; 
 ubyte *segptr=NULL; 
 int mask = COLORMASK12; 
 
 a = (Array)DXGetComponentValue(image, "connections"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "image missing connections"); 
 return NULL; 
 } 
 
 if (!DXQueryGridConnections(a, &nd, res) || nd != 2) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 x = res[0]; 
 y = res[1]; 
 image_size = x*y; 
 
 seglist = DXNewSegList(sizeof(ubyte), 0, 0); 
 if (! seglist) 
 goto error; 
 
 segknt = 0; 
 totknt = 0; 
 
 a = (Array)DXGetComponentValue(image, IMAGE); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 start_i = 0; 
 start_p = (ubyte *)DXGetArrayData(a); 
 a = NULL; 
 
 while (start_i < image_size) 
 { 
 
 i = start_i + 1; 
 p = start_p + sizeof(ushort); 
 
 while (i < image_size) 
 { 
 if (! (!((*((ushort *)(start_p)) ^ *((ushort *)(p))) & COLORMASK12))) 
 break; 
 
 i++; 
 p += sizeof(ushort); 
 } 
 
 knt = i - start_i; 
 
 
 if (knt == 1) 
 { 
 
 while ((!(!((*((ushort *)(p+0)) ^ *((ushort *)(p+sizeof(ushort)))) & COLORMASK12)) || 
 !(!((*((ushort *)(p+0)) ^ *((ushort *)(p+2*sizeof(ushort)))) & COLORMASK12))) && i < image_size - 2) 
 i++, p += sizeof(ushort); 
 
 
 if (i == image_size-2) 
 i = image_size; 
 
 knt = i - start_i; 
 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 int size = length * sizeof(ushort); 
 
 
 knt -= length; 
 
 
 totknt += (size + 1); 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 *segptr++ = 0x80 | length; 
 segknt --; 
 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 } 
 } 
 else 
 { 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 
 
 knt -= length; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 *segptr++ = length; 
 segknt--; 
 
 size = sizeof(ushort); 
 totknt += size + 1; 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 
 } 
 } 
 
 start_i = i; 
 start_p = p; 
 } 
 
 DXDebug("Y", "compaction: %d bytes in %d bytes out", 
 image_size*sizeof(ushort), totknt); 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, sizeof(ubyte)); 
 if (! a) 
 goto error; 
 
 if (! DXAddArrayData(a, 0, totknt, NULL)) 
 goto error; 
 
 start_p = (ubyte *)DXGetArrayData(a); 
 
 DXInitGetNextSegListSegment(seglist); 
 while (totknt > 0) 
 { 
 int s, l; 
 seg = DXGetNextSegListSegment(seglist); 
 if (! seg) 
 { 
 DXSetError(ERROR_INTERNAL, "run_length encoding error"); 
 goto error; 
 } 
 
 s = DXGetSegListSegmentItemCount(seg) * sizeof(ubyte); 
 if (totknt > s) 
 l = s; 
 else 
 l = totknt; 
 
 memcpy(start_p, DXGetSegListSegmentPointer(seg), l); 
 
 start_p += l; 
 totknt -= l; 
 } 
 
 res[0] = x; 
 res[1] = y; 
 
 attr = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0); 
 if (! attr) 
 goto error; 
 
 if (! DXAddArrayData((Array)attr, 0, 2, (Pointer)res)) 
 goto error; 
 
 if (! DXSetAttribute((Object)a, "resolution", attr)) 
 goto error; 
 attr = NULL; 
 
 if (! DXSetAttribute((Object)a, "encoding type", 
 (Object)DXNewString("12bit")))
 goto error; 
 
 DXDeleteSegList(seglist); 
 return a; 
 
error: 
 DXDeleteSegList(seglist); 
 DXDelete((Object)a); 
 
 return ERROR; 
}
static Field decode_24bit(Array encodedImage) 
{ 
 Field pixField = NULL; 
 Array a = NULL; 
 int i, j, image_size, flag, knt; 
 ubyte *ptr = (ubyte *)DXGetArrayData(encodedImage); 
 int *res, x, y; 
 ubyte *pix; 
 
 a = (Array)DXGetAttribute((Object)encodedImage, "resolution"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, 
 "cached image missing resolution attrbute"); 
 goto error; 
 } 
 
 res = DXGetArrayData(a); 
 x = res[0]; y = res[1]; 
 
 pixField = DXNewField(); 
 if (! pixField) 
 goto error; 
 
 a = DXMakeGridPositions(2, x, y, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0); 
 if (! a) 
 goto error; 
 
 if (! DXSetComponentValue(pixField, POSITIONS, (Object)a)) 
 goto error; 
 
 a = DXMakeGridConnections(2, x, y); 
 if (! a) 
 goto error; 
 if (! DXSetConnections(pixField, "quads", a)) 
 goto error; 
 
 a = DXNewArray(TYPE_UINT, CATEGORY_REAL, 0, 1); 
 if (! a) 
 goto error; 
 if (! DXAddArrayData(a, 0, x*y, NULL)) 
 goto error; 
 if (! DXSetAttribute((Object)a, "24bit", O_X_IMAGE)) 
 goto error; 
 
 pix = (ubyte *)DXGetArrayData(a); 
 
 image_size = x*y; 
 
 for (i = 0; i != image_size; i += knt) 
 { 
 flag = *ptr & 0x80; 
 knt = *ptr & 0x7f; 
 
 ptr ++; 
 
 if (flag) 
 { 
 memcpy((char *)pix, (char *)ptr, sizeof(int32)*knt); 
 (((char *)(pix))[0] = ((char *)(ptr))[0], ((char *)(pix))[1] = ((char *)(ptr))[1], ((char *)(pix))[2] = ((char *)(ptr))[2], ((char *)(pix))[3] = ((char *)(ptr))[3]); 
 pix += knt*sizeof(int32); 
 ptr += knt*sizeof(int32); 
 } else { 
 uint proto; 
 (((char *)(&proto))[0] = ((char *)(ptr))[0], ((char *)(&proto))[1] = ((char *)(ptr))[1], ((char *)(&proto))[2] = ((char *)(ptr))[2], ((char *)(&proto))[3] = ((char *)(ptr))[3]); 
 for (j = 0; j < knt; j++) 
 { 
 *(uint *)pix = proto; 
 pix += sizeof(int32); 
 } 
 
 ptr += sizeof(int32); 
 } 
 } 
 
 if (! DXSetComponentValue(pixField, IMAGE, (Object)a)) 
 goto error; 
 
 return DXEndField(pixField); 
 
error: 
 DXFree((Object)pixField); 
 DXFree((Object)a); 
 
 return NULL; 
}
static Array encode_24bit(Field image, translationP t) 
{ 
 ubyte *start_p, *p; 
 int start_i, i, knt, size; 
 int x, y; 
 Array a = NULL; 
 Object attr = NULL; 
 int image_size, res[2], nd; 
 int segknt, totknt; 
 SegListSegment *seg; 
 SegList *seglist = NULL; 
 ubyte *segptr=NULL; 
 int mask = COLORMASK24; 
 
 a = (Array)DXGetComponentValue(image, "connections"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "image missing connections"); 
 return NULL; 
 } 
 
 if (!DXQueryGridConnections(a, &nd, res) || nd != 2) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 x = res[0]; 
 y = res[1]; 
 image_size = x*y; 
 
 seglist = DXNewSegList(sizeof(ubyte), 0, 0); 
 if (! seglist) 
 goto error; 
 
 segknt = 0; 
 totknt = 0; 
 
 a = (Array)DXGetComponentValue(image, IMAGE); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 start_i = 0; 
 start_p = (ubyte *)DXGetArrayData(a); 
 a = NULL; 
 
 while (start_i < image_size) 
 { 
 
 i = start_i + 1; 
 p = start_p + sizeof(int32); 
 
 while (i < image_size) 
 { 
 if (! (!((*((uint *)(start_p)) ^ *((uint *)(p))) & mask))) 
 break; 
 
 i++; 
 p += sizeof(int32); 
 } 
 
 knt = i - start_i; 
 
 
 if (knt == 1) 
 { 
 
 while ((!(!((*((uint *)(p+0)) ^ *((uint *)(p+sizeof(int32)))) & mask)) || 
 !(!((*((uint *)(p+0)) ^ *((uint *)(p+2*sizeof(int32)))) & mask))) && i < image_size - 2) 
 i++, p += sizeof(int32); 
 
 
 if (i == image_size-2) 
 i = image_size; 
 
 knt = i - start_i; 
 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 int size = length * sizeof(int32); 
 
 
 knt -= length; 
 
 
 totknt += (size + 1); 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 *segptr++ = 0x80 | length; 
 segknt --; 
 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 } 
 } 
 else 
 { 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 
 
 knt -= length; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 *segptr++ = length; 
 segknt--; 
 
 size = sizeof(int32); 
 totknt += size + 1; 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 
 } 
 } 
 
 start_i = i; 
 start_p = p; 
 } 
 
 DXDebug("Y", "compaction: %d bytes in %d bytes out", 
 image_size*sizeof(int32), totknt); 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, sizeof(ubyte)); 
 if (! a) 
 goto error; 
 
 if (! DXAddArrayData(a, 0, totknt, NULL)) 
 goto error; 
 
 start_p = (ubyte *)DXGetArrayData(a); 
 
 DXInitGetNextSegListSegment(seglist); 
 while (totknt > 0) 
 { 
 int s, l; 
 seg = DXGetNextSegListSegment(seglist); 
 if (! seg) 
 { 
 DXSetError(ERROR_INTERNAL, "run_length encoding error"); 
 goto error; 
 } 
 
 s = DXGetSegListSegmentItemCount(seg) * sizeof(ubyte); 
 if (totknt > s) 
 l = s; 
 else 
 l = totknt; 
 
 memcpy(start_p, DXGetSegListSegmentPointer(seg), l); 
 
 start_p += l; 
 totknt -= l; 
 } 
 
 res[0] = x; 
 res[1] = y; 
 
 attr = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0); 
 if (! attr) 
 goto error; 
 
 if (! DXAddArrayData((Array)attr, 0, 2, (Pointer)res)) 
 goto error; 
 
 if (! DXSetAttribute((Object)a, "resolution", attr)) 
 goto error; 
 attr = NULL; 
 
 if (! DXSetAttribute((Object)a, "encoding type", 
 (Object)DXNewString("24bit")))
 goto error; 
 
 DXDeleteSegList(seglist); 
 return a; 
 
error: 
 DXDeleteSegList(seglist); 
 DXDelete((Object)a); 
 
 return ERROR; 
}
static Field decode_32bit(Array encodedImage) 
{ 
 Field pixField = NULL; 
 Array a = NULL; 
 int i, j, image_size, flag, knt; 
 ubyte *ptr = (ubyte *)DXGetArrayData(encodedImage); 
 int *res, x, y; 
 ubyte *pix; 
 
 a = (Array)DXGetAttribute((Object)encodedImage, "resolution"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, 
 "cached image missing resolution attrbute"); 
 goto error; 
 } 
 
 res = DXGetArrayData(a); 
 x = res[0]; y = res[1]; 
 
 pixField = DXNewField(); 
 if (! pixField) 
 goto error; 
 
 a = DXMakeGridPositions(2, x, y, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0); 
 if (! a) 
 goto error; 
 
 if (! DXSetComponentValue(pixField, POSITIONS, (Object)a)) 
 goto error; 
 
 a = DXMakeGridConnections(2, x, y); 
 if (! a) 
 goto error; 
 if (! DXSetConnections(pixField, "quads", a)) 
 goto error; 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, 4); 
 if (! a) 
 goto error; 
 if (! DXAddArrayData(a, 0, x*y, NULL)) 
 goto error; 
 if (! DXSetAttribute((Object)a, "32bit", O_X_IMAGE)) 
 goto error; 
 
 pix = (ubyte *)DXGetArrayData(a); 
 
 image_size = x*y; 
 
 for (i = 0; i != image_size; i += knt) 
 { 
 flag = *ptr & 0x80; 
 knt = *ptr & 0x7f; 
 
 ptr ++; 
 
 if (flag) 
 { 
 memcpy((char *)pix, (char *)ptr, sizeof(int32)*knt); 
 (((char *)(pix))[0] = ((char *)(ptr))[0], ((char *)(pix))[1] = ((char *)(ptr))[1], ((char *)(pix))[2] = ((char *)(ptr))[2], ((char *)(pix))[3] = ((char *)(ptr))[3]); 
 pix += knt*sizeof(int32); 
 ptr += knt*sizeof(int32); 
 } else { 
 uint proto; 
 (((char *)(&proto))[0] = ((char *)(ptr))[0], ((char *)(&proto))[1] = ((char *)(ptr))[1], ((char *)(&proto))[2] = ((char *)(ptr))[2], ((char *)(&proto))[3] = ((char *)(ptr))[3]); 
 for (j = 0; j < knt; j++) 
 { 
 *(uint *)pix = proto; 
 pix += sizeof(int32); 
 } 
 
 ptr += sizeof(int32); 
 } 
 } 
 
 if (! DXSetComponentValue(pixField, IMAGE, (Object)a)) 
 goto error; 
 
 return DXEndField(pixField); 
 
error: 
 DXFree((Object)pixField); 
 DXFree((Object)a); 
 
 return NULL; 
}
static Array encode_32bit(Field image, translationP t) 
{ 
 ubyte *start_p, *p; 
 int start_i, i, knt, size; 
 int x, y; 
 Array a = NULL; 
 Object attr = NULL; 
 int image_size, res[2], nd; 
 int segknt, totknt; 
 SegListSegment *seg; 
 SegList *seglist = NULL; 
 ubyte *segptr=NULL; 
 int mask = 0; 
 
 a = (Array)DXGetComponentValue(image, "connections"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "image missing connections"); 
 return NULL; 
 } 
 
 if (!DXQueryGridConnections(a, &nd, res) || nd != 2) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 x = res[0]; 
 y = res[1]; 
 image_size = x*y; 
 
 seglist = DXNewSegList(sizeof(ubyte), 0, 0); 
 if (! seglist) 
 goto error; 
 
 segknt = 0; 
 totknt = 0; 
 
 a = (Array)DXGetComponentValue(image, IMAGE); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 start_i = 0; 
 start_p = (ubyte *)DXGetArrayData(a); 
 a = NULL; 
 
 while (start_i < image_size) 
 { 
 
 i = start_i + 1; 
 p = start_p + sizeof(int32); 
 
 while (i < image_size) 
 { 
 if (! (*((uint *)(start_p)) == *((uint *)(p)))) 
 break; 
 
 i++; 
 p += sizeof(int32); 
 } 
 
 knt = i - start_i; 
 
 
 if (knt == 1) 
 { 
 
 while ((!(*((uint *)(p+0)) == *((uint *)(p+sizeof(int32)))) || 
 !(*((uint *)(p+0)) == *((uint *)(p+2*sizeof(int32))))) && i < image_size - 2) 
 i++, p += sizeof(int32); 
 
 
 if (i == image_size-2) 
 i = image_size; 
 
 knt = i - start_i; 
 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 int size = length * sizeof(int32); 
 
 
 knt -= length; 
 
 
 totknt += (size + 1); 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 *segptr++ = 0x80 | length; 
 segknt --; 
 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 } 
 } 
 else 
 { 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 
 
 knt -= length; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 *segptr++ = length; 
 segknt--; 
 
 size = sizeof(int32); 
 totknt += size + 1; 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 
 } 
 } 
 
 start_i = i; 
 start_p = p; 
 } 
 
 DXDebug("Y", "compaction: %d bytes in %d bytes out", 
 image_size*sizeof(int32), totknt); 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, sizeof(ubyte)); 
 if (! a) 
 goto error; 
 
 if (! DXAddArrayData(a, 0, totknt, NULL)) 
 goto error; 
 
 start_p = (ubyte *)DXGetArrayData(a); 
 
 DXInitGetNextSegListSegment(seglist); 
 while (totknt > 0) 
 { 
 int s, l; 
 seg = DXGetNextSegListSegment(seglist); 
 if (! seg) 
 { 
 DXSetError(ERROR_INTERNAL, "run_length encoding error"); 
 goto error; 
 } 
 
 s = DXGetSegListSegmentItemCount(seg) * sizeof(ubyte); 
 if (totknt > s) 
 l = s; 
 else 
 l = totknt; 
 
 memcpy(start_p, DXGetSegListSegmentPointer(seg), l); 
 
 start_p += l; 
 totknt -= l; 
 } 
 
 res[0] = x; 
 res[1] = y; 
 
 attr = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0); 
 if (! attr) 
 goto error; 
 
 if (! DXAddArrayData((Array)attr, 0, 2, (Pointer)res)) 
 goto error; 
 
 if (! DXSetAttribute((Object)a, "resolution", attr)) 
 goto error; 
 attr = NULL; 
 
 if (! DXSetAttribute((Object)a, "encoding type", 
 (Object)DXNewString("32bit")))
 goto error; 
 
 DXDeleteSegList(seglist); 
 return a; 
 
error: 
 DXDeleteSegList(seglist); 
 DXDelete((Object)a); 
 
 return ERROR; 
}
static Field decode_RGBColor(Array encodedImage) 
{ 
 Field pixField = NULL; 
 Array a = NULL; 
 int i, j, image_size, flag, knt; 
 ubyte *ptr = (ubyte *)DXGetArrayData(encodedImage); 
 int *res, x, y; 
 ubyte *pix; 
 
 a = (Array)DXGetAttribute((Object)encodedImage, "resolution"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, 
 "cached image missing resolution attrbute"); 
 goto error; 
 } 
 
 res = DXGetArrayData(a); 
 x = res[0]; y = res[1]; 
 
 pixField = DXNewField(); 
 if (! pixField) 
 goto error; 
 
 a = DXMakeGridPositions(2, x, y, 0.0, 0.0, 0.0, 1.0, 1.0, 0.0); 
 if (! a) 
 goto error; 
 
 if (! DXSetComponentValue(pixField, POSITIONS, (Object)a)) 
 goto error; 
 
 a = DXMakeGridConnections(2, x, y); 
 if (! a) 
 goto error; 
 if (! DXSetConnections(pixField, "quads", a)) 
 goto error; 
 
 a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0, 3); 
 if (! a) 
 goto error; 
 if (! DXAddArrayData(a, 0, x*y, NULL)) 
 goto error; 
 if (! DXSetAttribute((Object)a, "RGBColor", O_X_IMAGE)) 
 goto error; 
 
 pix = (ubyte *)DXGetArrayData(a); 
 
 image_size = x*y; 
 
 for (i = 0; i != image_size; i += knt) 
 { 
 flag = *ptr & 0x80; 
 knt = *ptr & 0x7f; 
 
 ptr ++; 
 
 if (flag) 
 { 
 memcpy((char *)pix, (char *)ptr, sizeof(RGBColor)*knt); 
 memcpy((char *)(pix), (char *)(ptr), sizeof(RGBColor)); 
 pix += knt*sizeof(RGBColor); 
 ptr += knt*sizeof(RGBColor); 
 } else { 
 RGBColor proto; 
 memcpy((char *)(&proto), (char *)(ptr), sizeof(RGBColor)); 
 for (j = 0; j < knt; j++) 
 { 
 *(RGBColor *)pix = proto; 
 pix += sizeof(RGBColor); 
 } 
 
 ptr += sizeof(RGBColor); 
 } 
 } 
 
 if (! DXSetComponentValue(pixField, IMAGE, (Object)a)) 
 goto error; 
 
 return DXEndField(pixField); 
 
error: 
 DXFree((Object)pixField); 
 DXFree((Object)a); 
 
 return NULL; 
}
static Array encode_RGBColor(Field image, translationP t) 
{ 
 ubyte *start_p, *p; 
 int start_i, i, knt, size; 
 int x, y; 
 Array a = NULL; 
 Object attr = NULL; 
 int image_size, res[2], nd; 
 int segknt, totknt; 
 SegListSegment *seg; 
 SegList *seglist = NULL; 
 ubyte *segptr=NULL; 
 int mask = 0; 
 
 a = (Array)DXGetComponentValue(image, "connections"); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "image missing connections"); 
 return NULL; 
 } 
 
 if (!DXQueryGridConnections(a, &nd, res) || nd != 2) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 x = res[0]; 
 y = res[1]; 
 image_size = x*y; 
 
 seglist = DXNewSegList(sizeof(ubyte), 0, 0); 
 if (! seglist) 
 goto error; 
 
 segknt = 0; 
 totknt = 0; 
 
 a = (Array)DXGetComponentValue(image, IMAGE); 
 if (! a) 
 { 
 DXSetError(ERROR_INTERNAL, "invalid image"); 
 return NULL; 
 } 
 
 start_i = 0; 
 start_p = (ubyte *)DXGetArrayData(a); 
 a = NULL; 
 
 while (start_i < image_size) 
 { 
 
 i = start_i + 1; 
 p = start_p + sizeof(RGBColor); 
 
 while (i < image_size) 
 { 
 if (! ((((RGBColor *)(start_p))->r == ((RGBColor *)(p))->r) && (((RGBColor *)(start_p))->g == ((RGBColor *)(p))->g) && (((RGBColor *)(start_p))->b == ((RGBColor *)(p))->b))) 
 break; 
 
 i++; 
 p += sizeof(RGBColor); 
 } 
 
 knt = i - start_i; 
 
 
 if (knt == 1) 
 { 
 
 while ((!((((RGBColor *)(p+0))->r == ((RGBColor *)(p+sizeof(RGBColor)))->r) && (((RGBColor *)(p+0))->g == ((RGBColor *)(p+sizeof(RGBColor)))->g) && (((RGBColor *)(p+0))->b == ((RGBColor *)(p+sizeof(RGBColor)))->b)) || 
 !((((RGBColor *)(p+0))->r == ((RGBColor *)(p+2*sizeof(RGBColor)))->r) && (((RGBColor *)(p+0))->g == ((RGBColor *)(p+2*sizeof(RGBColor)))->g) && (((RGBColor *)(p+0))->b == ((RGBColor *)(p+2*sizeof(RGBColor)))->b))) && i < image_size - 2) 
 i++, p += sizeof(RGBColor); 
 
 
 if (i == image_size-2) 
 i = image_size; 
 
 knt = i - start_i; 
 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 int size = length * sizeof(RGBColor); 
 
 
 knt -= length; 
 
 
 totknt += (size + 1); 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 *segptr++ = 0x80 | length; 
 segknt --; 
 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 } 
 } 
 else 
 { 
 
 while (knt > 0) 
 { 
 
 int length = (knt > 127) ? 127 : knt; 
 
 
 knt -= length; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 *segptr++ = length; 
 segknt--; 
 
 size = sizeof(RGBColor); 
 totknt += size + 1; 
 
 while (size > 0) 
 { 
 int l; 
 
 if (segknt == 0) { seg = DXNewSegListSegment(seglist,1024); if (! seg) goto error; segptr = DXGetSegListSegmentPointer(seg); if (! segptr) goto error; segknt = 1024; }; 
 
 l = (size > segknt) ? segknt : size; 
 
 memcpy(segptr, start_p, l); 
 segptr += l; 
 size -= l; 
 start_p += l; 
 segknt -= l; 
 } 
 
 } 
 } 
 
 start_i = i; 
 start_p = p; 
 } 
 
 DXDebug("Y", "compaction: %d bytes in %d bytes out", 
 image_size*sizeof(RGBColor), totknt); 
 
 a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, sizeof(ubyte)); 
 if (! a) 
 goto error; 
 
 if (! DXAddArrayData(a, 0, totknt, NULL)) 
 goto error; 
 
 start_p = (ubyte *)DXGetArrayData(a); 
 
 DXInitGetNextSegListSegment(seglist); 
 while (totknt > 0) 
 { 
 int s, l; 
 seg = DXGetNextSegListSegment(seglist); 
 if (! seg) 
 { 
 DXSetError(ERROR_INTERNAL, "run_length encoding error"); 
 goto error; 
 } 
 
 s = DXGetSegListSegmentItemCount(seg) * sizeof(ubyte); 
 if (totknt > s) 
 l = s; 
 else 
 l = totknt; 
 
 memcpy(start_p, DXGetSegListSegmentPointer(seg), l); 
 
 start_p += l; 
 totknt -= l; 
 } 
 
 res[0] = x; 
 res[1] = y; 
 
 attr = (Object)DXNewArray(TYPE_INT, CATEGORY_REAL, 0); 
 if (! attr) 
 goto error; 
 
 if (! DXAddArrayData((Array)attr, 0, 2, (Pointer)res)) 
 goto error; 
 
 if (! DXSetAttribute((Object)a, "resolution", attr)) 
 goto error; 
 attr = NULL; 
 
 if (! DXSetAttribute((Object)a, "encoding type", 
 (Object)DXNewString("RGBColor")))
 goto error; 
 
 DXDeleteSegList(seglist); 
 return a; 
 
error: 
 DXDeleteSegList(seglist); 
 DXDelete((Object)a); 
 
 return ERROR; 
}
Object
_dxf_EncodeImage(Field image)
{
    Type t; Category c;
    int r, s;
    Object attr;
    Object encodedimage = NULL;
    Object o = DXGetComponentAttribute(image, IMAGE, X_SERVER);
    translationP translation = NULL;
    Array a = (Array)DXGetComponentValue(image, IMAGE);
    if (! a)
    {
 DXSetError(ERROR_INTERNAL, "invalid image field");
 return NULL;
    }
    if (o)
 translation = _dxf_GetXTranslation(o);
    DXMarkTime("encode start");
    if (! DXGetArrayInfo(a, NULL, &t, &c, &r, &s))
 return NULL;
    if (t == TYPE_UBYTE && (r == 0 || (r == 1 && s == 1)))
 encodedimage = (Object)encode_8bit(image, translation);
    else if (t == TYPE_USHORT && (r == 0 || (r == 1 && s == 1)))
 encodedimage = (Object)encode_12bit(image, translation);
    else if (t == TYPE_UINT && (r == 0 || (r == 1 && s == 1)))
 encodedimage = (Object)encode_24bit(image, translation);
    else if (t == TYPE_UBYTE && r == 1 && s == 4)
 encodedimage = (Object)encode_32bit(image, translation);
    else if (t == TYPE_FLOAT && (r == 1 && s == 3))
 encodedimage = (Object)encode_RGBColor(image, translation);
    else
 encodedimage = DXCopy((Object)image, COPY_STRUCTURE);
    if (!encodedimage)
 return NULL;
    if (! DXSetIntegerAttribute(encodedimage, "image tag",
     (int)DXGetObjectTag((Object)image)))
    {
 DXDelete(encodedimage);
 return NULL;
    }
    attr = DXGetAttribute((Object)image, "camera");
    if (attr)
 if (! DXSetAttribute(encodedimage, "camera", attr))
 {
     DXDelete(encodedimage);
     return NULL;
 }
    attr = DXGetAttribute((Object)image, "object box");
    if (attr)
 if (! DXSetAttribute(encodedimage, "object box", attr))
 {
     DXDelete(encodedimage);
     return NULL;
 }
    attr = DXGetComponentAttribute(image, IMAGE, IMAGE_TYPE);
    if (attr)
 if (! DXSetAttribute(encodedimage, IMAGE_TYPE, attr))
 {
     DXDelete(encodedimage);
     return NULL;
 }
    attr = DXGetAttribute((Object)image, X_SERVER);
    if (attr)
 if (! DXSetAttribute(encodedimage, X_SERVER, attr))
 {
     DXDelete(encodedimage);
     return NULL;
 }
    DXMarkTime("encode end");
    return encodedimage;
}
Field
_dxf_DecodeImage(Object encodedimage)
{
    Field image;
    char *str;
    Object attr;
    attr = DXGetAttribute((Object)encodedimage, "encoding type");
    if (! attr)
    {
 if (DXGetObjectClass(encodedimage) != CLASS_FIELD)
 {
     DXSetError(ERROR_DATA_INVALID, "invalid image");
     return NULL;
 }
 image = (Field)DXCopy(encodedimage, COPY_STRUCTURE);
    }
    else
    {
 if (DXGetObjectClass(encodedimage) != CLASS_ARRAY)
 {
     DXSetError(ERROR_DATA_INVALID, "invalid encoded image");
     return NULL;
 }
 if (DXGetObjectClass(attr) != CLASS_STRING)
 {
     DXSetError(ERROR_INTERNAL, "invalid encoded image");
     return NULL;
 }
 str = DXGetString((String)attr);
 DXMarkTime("decode start");
 if (! strcmp(str, "8bit"))
     image = decode_8bit((Array)encodedimage);
 else if (! strcmp(str, "12bit"))
     image = decode_12bit((Array)encodedimage);
 else if (! strcmp(str, "24bit"))
     image = decode_24bit((Array)encodedimage);
 else if (! strcmp(str, "32bit"))
     image = decode_32bit((Array)encodedimage);
 else if (! strcmp(str, "RGBColor"))
     image = decode_RGBColor((Array)encodedimage);
 else
 {
     DXSetError(ERROR_INTERNAL, "invalid encoded image");
     return NULL;
 }
 attr = DXGetAttribute(encodedimage, "image tag");
 if (! attr)
 {
     DXSetError(ERROR_INTERNAL, "missing image tag");
     DXDelete((Object)image);
     return NULL;
 }
 if (! DXSetAttribute((Object)image, "image tag", attr))
 {
     DXDelete((Object)image);
     return NULL;
 }
 attr = DXGetAttribute(encodedimage, "camera");
 if (attr)
     if (! DXSetAttribute((Object)image, "camera", attr))
     {
  DXDelete((Object)image);
  return NULL;
     }
 attr = DXGetAttribute(encodedimage, "object box");
 if (attr)
     if (! DXSetAttribute((Object)image, "object box", attr))
     {
  DXDelete((Object)image);
  return NULL;
     }
 if (NULL != (attr = DXGetAttribute(encodedimage, IMAGE_TYPE)))
     if (! DXSetComponentAttribute(image, IMAGE, IMAGE_TYPE, attr))
     {
  DXDelete((Object)image);
  return NULL;
     }
 if (NULL != (attr = DXGetAttribute(encodedimage, X_SERVER)))
     if (! DXSetAttribute((Object)image, X_SERVER, attr))
     {
  DXDelete((Object)image);
  return NULL;
     }
    }
    DXMarkTime("decode end");
    return image;
}
