/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <dx/dx.h>
#include "internals.h"
#include "displayx.h"
#include "displayw.h"

unsigned char _dxd_convert[NC] = { 0 };	   /* see internals.h for more info */

typedef  struct {char r, g, b;} rgb;
typedef  struct {double r, g, b;} drgb;

static Field _dxf_outputrgb_yuv(Field i, int fd, int yuv);
static int rgb_to_yuv(drgb *buf, char *tmp, double *yuv_buf[3], int n_line);
#define YUV_BUF 5

Field DXMakeImage(int width, int height)
{
    return DXMakeImageFormat(width, height, NULL);
}

Field DXMakeImageFormat(int width, int height, char *format)
{
    Field i = NULL;
    Array a;

    if (! format)
	format = getenv("DXPIXELTYPE");
    if (! format)
	format = "FLOAT";

    /* sanity check */
    if ((double)width*height*3*sizeof(float) > (double)(unsigned int)(-1)) {
	/* image size %dx%d exceeds address space */
	DXSetError(ERROR_BAD_PARAMETER, "#11710", width, height);
	return NULL;
    }
    
    /*
     * an image is a field
     */
    i = DXNewField();
    if (!i)
	return NULL;

    /*
     * positions
     */
    a = DXMakeGridPositions(2, height,width,
			  /* origin: */ 0.0,0.0,
			  /* deltas: */ 0.0,1.0, 1.0,0.0);
    if (!a)
	goto error;

    if (! DXSetComponentValue(i, POSITIONS, (Object)a))
	goto error;

    /*
     * connections
     */
    a = DXMakeGridConnections(2, height,width);
    if (!a)
	goto error;

    if (! DXSetConnections(i, "quads", a))
	goto error;

    /*
     * colors
     */
    if (!format 				      ||
	!strncmp(format, "FLOAT",   strlen("FLOAT"))  ||
	!strncmp(format, "DXFloat", strlen("DXFloat"))||
	!strncmp(format, "DXFLOAT", strlen("DXFLOAT")))
    {
	a = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
    }
    else if (!strncmp(format, "BYTE",   strlen("BYTE"))   ||
	     !strncmp(format, "DXByte", strlen("DXByte")) ||
	     !strncmp(format, "DXBYTE", strlen("DXBYTE")))
    {
	a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 1, 3);
    }
    else if (!strncmp(format, "DELAYEDCOLOR", strlen("DELAYEDCOLOR"))     ||
	     !strncmp(format, "DXDELAYEDCOLOR", strlen("DXDELAYEDCOLOR")) ||
	     !strncmp(format, "DXDelayedColor", strlen("DXDelayedColor")))
    {
	Array onearray = NULL;
	int   one = 1;
	Array map = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 1, 3);
	if (! map)
	    goto error;

	if (! DXSetComponentValue(i, "color map", (Object)map))
	{
	    DXDelete((Object)map);
	    goto error;
	}

	if (! DXAddArrayData(map, 0, 256, NULL))
	    goto error;
	
	a = DXNewArray(TYPE_UBYTE, CATEGORY_REAL, 0);
	if (! a)
	    goto error;

	if (! DXSetAttribute((Object)a, "ref",
				(Object)DXNewString("color map")))
	    goto error;

	/* for delayed colors, add the direct color map attribute */
        onearray = DXNewArray(TYPE_INT,CATEGORY_REAL, 0);
	if (!DXAddArrayData(onearray, 0, 1, &one))
	    goto error;
	if (!DXSetAttribute((Object)i, "direct color map", (Object)onearray))
	    goto error;
	onearray = NULL;
    }
    else
    {
	DXSetError(ERROR_BAD_PARAMETER, "invalid image format: %s", format);
	goto error;
    }

    if (!a)
	goto error;

    if (!DXAddArrayData(a, 0, width*height, NULL))
    {
	DXAddMessage("can't make %dx%d image", width, height);
	goto error;
    }

    if (! DXSetComponentValue(i, COLORS, (Object)a))
	goto error;

    if (format[strlen(format)-1] == 'Z')
    {
	Array z = DXNewArray(TYPE_FLOAT, CATEGORY_REAL, 0);
	if (! z)
	    goto error;
	
	if (!DXAddArrayData(z, 0, width*height, NULL))
	{
	    DXAddMessage("can't make %dx%d zbuffer", width, height);
	    goto error;
	}

	if (! DXSetComponentValue(i, "zbuffer", (Object)z))
	    goto error;
    }
	
    /*
     * set default ref and dep
     */
    if (!  DXEndField(i))
	goto error;
    
    return i;

error:
    DXDelete((Object)i);
    return NULL;
}


RGBColor
*DXGetPixels(Field i)
{
    int n;
    Array a;
    char *s;

    /* check positions */
    a = (Array) DXGetComponentValue(i, POSITIONS);
    if (!DXQueryGridPositions(a, &n, NULL, NULL, NULL) || n!=2) {
	/* Image field does not have 2-d regular positions */
	DXSetError(ERROR_BAD_PARAMETER, "#11714");
	return NULL;
    }

    /* check connections */
    a = (Array) DXGetComponentValue(i, CONNECTIONS);
    if (!DXQueryGridConnections(a, &n, NULL) || n!=2) {
	/* Image field does not have 2-d regular connections */	
	DXSetError(ERROR_BAD_PARAMETER, "#11716");
	return NULL;
    }

    /* return the colors component */
    a = (Array) DXGetComponentValue(i, COLORS);
    if (!a || !DXTypeCheck(a, TYPE_FLOAT, CATEGORY_REAL, 1, 3)) {
	/* Image field does not have 3-d float colors */
	DXSetError(ERROR_BAD_PARAMETER, "#11718");
	return NULL;
    }

    /* check dep */
    s = DXGetString((String)DXGetAttribute((Object)a, DEP));
    if (s && strcmp(s, POSITIONS)) {
	/* %s must be dependent on positions */
	DXSetError(ERROR_BAD_PARAMETER, "#11251", "Image colors");
	return NULL;
    }

    return (RGBColor *) DXGetArrayData(a);
}

RGBByteColor
*DXGetBytePixels(Field i)
{
    int n;
    Array a;
    char *s;

    /* check positions */
    a = (Array) DXGetComponentValue(i, POSITIONS);
    if (!DXQueryGridPositions(a, &n, NULL, NULL, NULL) || n!=2) {
	/* Image field does not have 2-d regular positions */
	DXSetError(ERROR_BAD_PARAMETER, "#11714");
	return NULL;
    }

    /* check connections */
    a = (Array) DXGetComponentValue(i, CONNECTIONS);
    if (!DXQueryGridConnections(a, &n, NULL) || n!=2) {
	/* Image field does not have 2-d regular connections */	
	DXSetError(ERROR_BAD_PARAMETER, "#11716");
	return NULL;
    }

    /* return the colors component */
    a = (Array) DXGetComponentValue(i, COLORS);
    if (!a || !DXTypeCheck(a, TYPE_UBYTE, CATEGORY_REAL, 1, 3)) {
	/* Image field does not have 3-d byte colors */
	DXSetError(ERROR_BAD_PARAMETER, "#11718");
	return NULL;
    }

    /* check dep */
    s = DXGetString((String)DXGetAttribute((Object)a, DEP));
    if (s && strcmp(s, POSITIONS)) {
	/* %s must be dependent on positions */
	DXSetError(ERROR_BAD_PARAMETER, "#11251", "Image colors");
	return NULL;
    }

    return (RGBByteColor *) DXGetArrayData(a);
}



static Object
bounds(Object o, int *left, int *right, int *bot, int *top)
{
    Array a;
    int i, n, dims[2], offsets[2], lt, rt, bt, tp;
    Object oo;
    struct { float x, y; } deltas[2];

    switch (DXGetObjectClass(o)) {
    case CLASS_GROUP:
	for (i=0; (oo=DXGetEnumeratedMember((Group)o,i,NULL)); i++)
	    if (!bounds(oo, left, right, bot, top))
		return NULL;
	break;

    case CLASS_FIELD:

	/* positions (for size) and sense */
	a = (Array) DXGetComponentValue((Field)o, POSITIONS);
	if (!a||
	    !DXQueryGridPositions(a, &n, NULL, NULL, NULL)||
	    (n!=2)||
	    !DXQueryGridPositions(a, NULL, dims, NULL, (float *)deltas))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "#11714");
                         /* Image field does not have 2-d regular positions */


	/* connections (for offsets) */
	a = (Array) DXGetComponentValue((Field)o, CONNECTIONS);
	if (!a||
	    !DXGetMeshOffsets((MeshArray)a, offsets))
	    DXErrorReturn(ERROR_BAD_PARAMETER, "#11716");
                         /* Image field does not have 2-d regular connections */

	/* extents */
	deltas[0].x = deltas[0].x>0? 1 : deltas[0].x<0? -1 : 0;
	deltas[0].y = deltas[0].y>0? 1 : deltas[0].y<0? -1 : 0;
	deltas[1].x = deltas[1].x>0? 1 : deltas[1].x<0? -1 : 0;
	deltas[1].y = deltas[1].y>0? 1 : deltas[1].y<0? -1 : 0;
	lt = offsets[0]*deltas[0].x + offsets[1]*deltas[1].x;
	bt = offsets[0]*deltas[0].y + offsets[1]*deltas[1].y;
	rt = lt + (dims[0]-1)*deltas[0].x + (dims[1]-1)*deltas[1].x;
	tp = bt + (dims[0]-1)*deltas[0].y + (dims[1]-1)*deltas[1].y;
	if (lt < *left) *left = lt;
	if (rt < *left) *left = rt;
	if (rt > *right) *right = rt;
	if (lt > *right) *right = lt;
	if (bt < *bot) *bot = bt;
	if (tp < *bot) *bot = tp;
	if (tp > *top) *top = tp;
	if (bt > *top) *top = bt;
	break;

    default:
	DXSetError(ERROR_BAD_PARAMETER, "#10190", "image");
	return NULL;
    }

    return o;
}

Object
DXGetImageBounds(Object o, int *x, int *y, int *width, int *height)
{
    int left = DXD_MAX_INT, right = -DXD_MAX_INT;
    int bottom = DXD_MAX_INT, top = -DXD_MAX_INT;

    if (!bounds(o, &left, &right, &bottom, &top))
	return NULL;    
    if (x)
	*x = left;
    if (y)
	*y = bottom;
    if (width)
	*width = right - left + 1;
    if (height)
	*height = top - bottom + 1;
    return o;
}

Field
DXGetImageSize(Field i, int *width, int *height)
{
    return (Field)DXGetImageBounds((Object)i, NULL, NULL, width, height);
}

/*
 * work around an sgi compiler bug
 */
#define CLAMP(p) (p=(p<0?0:p), p=(p>255?255:p))
#define CLAMPF(p) (p=(p<0?0:p), p=(p>1.0?1.0:p))

#define RGBRGB_OUTPUT_DELAYED_FLOAT(indexType) \
{ \
    indexType *pixels = (indexType *)DXGetArrayData(colors); \
    indexType *from; \
    RGBColor  *mapData = (RGBColor *)DXGetArrayData(map); \
 \
    if (!yuv) \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*width; \
	for (x=0; x<width; x++) { \
	    RGBColor *mptr = &mapData[from[x]];\
	    rgb *bptr = &buf[x];	\
	    r = 255*mptr->r; \
	    g = 255*mptr->g; \
	    b = 255*mptr->b; \
	    bptr->r = CLAMP(r); \
	    bptr->g = CLAMP(g); \
	    bptr->b = CLAMP(b); \
	} \
 \
	if (n!=write (fd, buf, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
    else \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*width; \
	for (x=0; x<width; x++) { \
	    RGBColor *mptr = &mapData[from[x]];  \
	    drgb *dbptr = &dbuf[x];		\
	    dr = mptr->r; \
	    dg = mptr->g; \
	    db = mptr->b; \
	    dbptr->r = CLAMPF(dr); \
	    dbptr->g = CLAMPF(dg); \
	    dbptr->b = CLAMPF(db); \
	} \
	rgb_to_yuv(dbuf, tmp, yuv_buf, width); \
 \
	if (2*width!=write (fd, tmp, 2*width)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RGBRGB_OUTPUT_DELAYED_INT(indexType, mapType) \
{ \
    indexType *pixels = (indexType *)DXGetArrayData(colors); \
    indexType *from; \
    mapType  *mapData = (mapType *)DXGetArrayData(map); \
 \
    if (!yuv) \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*width; \
	for (x=0; x<width; x++) { \
	    mapType *m = mapData + 3*(*from++); \
	    rgb *bptr = &buf[x];	\
	    r = *m++; \
	    g = *m++; \
	    b = *m++; \
	    bptr->r = CLAMP(r); \
	    bptr->g = CLAMP(g); \
	    bptr->b = CLAMP(b); \
	} \
 \
	if (n!=write (fd, buf, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
    else \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*width; \
	for (x=0; x<width; x++) { \
	    mapType *m = mapData + 3*(*from++); \
	    drgb *dbptr = &dbuf[x];	\
	    dr = (*m++)/255.0; \
	    dg = (*m++)/255.0; \
	    db = (*m++)/255.0; \
	    dbptr->r = CLAMPF(dr); \
	    dbptr->g = CLAMPF(dg); \
	    dbptr->b = CLAMPF(db); \
	} \
	rgb_to_yuv(dbuf, tmp, yuv_buf, width); \
 \
	if (2*width!=write (fd, tmp, 2*width)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RGBRGB_OUTPUT_DELAYED_INT_ALL(pixtype) \
switch(map_type) \
{ \
    case TYPE_UBYTE:  RGBRGB_OUTPUT_DELAYED_INT(pixtype, ubyte);  break; \
    case TYPE_USHORT: RGBRGB_OUTPUT_DELAYED_INT(pixtype, ushort); break; \
    case TYPE_SHORT:  RGBRGB_OUTPUT_DELAYED_INT(pixtype, short);  break; \
    case TYPE_INT:    RGBRGB_OUTPUT_DELAYED_INT(pixtype, int);    break; \
    case TYPE_UINT:   RGBRGB_OUTPUT_DELAYED_INT(pixtype, uint);   break; \
    case TYPE_FLOAT:  RGBRGB_OUTPUT_DELAYED_FLOAT(pixtype);   	  break; \
    default: \
    {  \
	DXSetError(ERROR_DATA_INVALID, \
	    "delayed color maps must be float, ubyte, ushort, short, int or uint"); \
	goto error; \
    } \
}

#define RGBRGB_OUTPUT_FLOAT \
{ \
    float *pixels = (float *)DXGetArrayData(colors); \
    float *from; \
    int wid = width * 3;  \
 \
    if (!yuv) \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*wid; \
	for (x=0; x<width; x++) { \
	    r = 255*(*from++); \
	    g = 255*(*from++); \
	    b = 255*(*from++); \
	    buf[x].r = CLAMP(r); \
	    buf[x].g = CLAMP(g); \
	    buf[x].b = CLAMP(b); \
	} \
 \
	if (n!=write (fd, buf, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
    else \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*wid; \
	for (x=0; x<width; x++) { \
	    dr = (*from++); \
	    dg = (*from++); \
	    db = (*from++); \
	    dbuf[x].r = CLAMPF(dr); \
	    dbuf[x].g = CLAMPF(dg); \
	    dbuf[x].b = CLAMPF(db); \
	} \
	rgb_to_yuv(dbuf, tmp, yuv_buf, width); \
 \
	if (2*width!=write (fd, tmp, 2*width)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RGBRGB_OUTPUT_INTEGER(int_type) \
{ \
    int_type *pixels = (int_type *)DXGetArrayData(colors); \
    int_type *from; \
    int wid = width * 3;  \
 \
    if (!yuv) \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*wid; \
	for (x=0; x<width; x++) { \
	    r = (*from++); \
	    g = (*from++); \
	    b = (*from++); \
	    buf[x].r = CLAMP(r); \
	    buf[x].g = CLAMP(g); \
	    buf[x].b = CLAMP(b); \
	} \
\
	if (n!=write (fd, buf, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
    else \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*wid; \
	for (x=0; x<width; x++) { \
	    dr = (*from++)/255.0; \
	    dg = (*from++)/255.0; \
	    db = (*from++)/255.0; \
	    dbuf[x].r = CLAMPF(dr); \
	    dbuf[x].g = CLAMPF(dg); \
	    dbuf[x].b = CLAMPF(db); \
	} \
	rgb_to_yuv(dbuf, tmp, yuv_buf, width); \
\
	if (2*width!=write (fd, tmp, 2*width)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RGBRGB_OUTPUT_UBYTE \
{ \
    ubyte *pixels = (ubyte *)DXGetArrayData(colors); \
    ubyte *from; \
    int wid = width * 3;  \
 \
    if (!yuv) { \
	for (y=height-1; y>=0; y--) { \
	    from = pixels + y*wid; \
	    if (n!=write (fd, from, n)) \
		DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	} \
    } else { \
	for (y=height-1; y>=0; y--) { \
	    from = pixels + y*wid; \
	    for (x=0; x<width; x++) { \
		dr = (*from++)/255.0; \
		dg = (*from++)/255.0; \
		db = (*from++)/255.0; \
		dbuf[x].r = CLAMPF(dr); \
		dbuf[x].g = CLAMPF(dg); \
		dbuf[x].b = CLAMPF(db); \
	    } \
	    rgb_to_yuv(dbuf, tmp, yuv_buf, width); \
	    if (2*width!=write (fd, tmp, 2*width)) \
		DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	} \
    } \
}

Field
DXOutputRGB(Field i, int fd)
{
    return (_dxf_outputrgb_yuv(i, fd, 0));
}

Field DXOutputYUV(Field i, int fd)
{
    return (_dxf_outputrgb_yuv(i, fd, 1));
}

static
Field
_dxf_outputrgb_yuv(Field i, int fd, int yuv)
{
    int x, y, n, r, g, b;
    double dr, dg, db;
    rgb *buf = NULL;
    int width, height;
    Array colors, map;
    Type pixels_type;
    int  rank, shape[32];
    char *tmp = NULL;
    double *yuv_buf[3] = {NULL, NULL, NULL};
    double *yuv_big_buf = NULL;
    drgb *dbuf = NULL;

    /* don't handle fb images for now */
    if (DXGetComponentValue(i, IMAGE))
	return i;

    if (!DXGetImageSize(i, &width, &height))
	goto error;

    n = width * sizeof(rgb);

    buf = (rgb *) DXAllocate(n); 
    if (!buf)
	goto error;

    if (yuv)
    {
	dbuf = (drgb *) DXAllocate(width*sizeof(drgb));
	if (!dbuf)
	    goto error;
	tmp = (char *) DXAllocate(width*2);
	if (!tmp)
	    goto error;
	if (!(yuv_big_buf = (double *) DXAllocate(3*(width+YUV_BUF)*sizeof(double))))
	    goto error;
	yuv_buf[0] = &yuv_big_buf[0];
	yuv_buf[1] = &yuv_big_buf[width+YUV_BUF];
	yuv_buf[2] = &yuv_big_buf[2*(width+YUV_BUF)];
    }

    colors = (Array)DXGetComponentValue(i, "colors");
    if (! colors)
	goto error;
    
    DXGetArrayInfo(colors, NULL, &pixels_type, NULL, &rank, shape);

    if (rank == 0 || (rank == 1 && shape[0] == 1))
    {
	Type map_type;
	int  mr, ms[32];

	map = (Array)DXGetComponentValue(i, "color map");
	if (! map)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"delayed-color image with no colormap");
	    goto error;
	}

	DXGetArrayInfo(map, NULL, &map_type, NULL, &mr, ms);
	if (mr != 1 || ms[0] != 3)
	{
	    DXSetError(ERROR_DATA_INVALID,
		"delayed color map must be 3-vector");
	    goto error;
	}

	switch(pixels_type)
	{
	    case TYPE_INT:    RGBRGB_OUTPUT_DELAYED_INT_ALL(int);    break;
	    case TYPE_UINT:   RGBRGB_OUTPUT_DELAYED_INT_ALL(uint);   break;
	    case TYPE_SHORT:  RGBRGB_OUTPUT_DELAYED_INT_ALL(short);  break;
	    case TYPE_USHORT: RGBRGB_OUTPUT_DELAYED_INT_ALL(ushort); break;
	    case TYPE_UBYTE:  RGBRGB_OUTPUT_DELAYED_INT_ALL(ubyte);  break;
	    default:
		DXSetError(ERROR_DATA_INVALID,
	    "delayed-color pixels must be ubyte, ushort, short, int or uint");
		goto error;
	}
    }
    else if (rank == 1 && shape[0] == 3)
    {
	switch(pixels_type)
	{
	    case TYPE_FLOAT:  RGBRGB_OUTPUT_FLOAT;           break;
	    case TYPE_INT:    RGBRGB_OUTPUT_INTEGER(int);    break;
	    case TYPE_UINT:   RGBRGB_OUTPUT_INTEGER(uint);   break;
	    case TYPE_SHORT:  RGBRGB_OUTPUT_INTEGER(short);  break;
	    case TYPE_USHORT: RGBRGB_OUTPUT_INTEGER(ushort); break;
	    case TYPE_UBYTE:  RGBRGB_OUTPUT_UBYTE;           break;
	    default:
		DXSetError(ERROR_DATA_INVALID,
		    "pixels must be float, ubyte, ushort, short, int or uint");
		goto error;
	}
    }	
    else
    {
	DXSetError(ERROR_DATA_INVALID, "%s %s",
	    "image pixels must either be single valued (for delayed colors)",
	    "or 3-vectors");
	goto error;
    }

    DXFree((Pointer)buf);
    if (yuv)
    {
	DXFree((Pointer)dbuf);
	DXFree((Pointer)tmp);
	DXFree((Pointer)yuv_big_buf);
    }

    return i;

error:
    DXFree((Pointer)buf);
    if (yuv)
    {
	DXFree((Pointer)dbuf);
	DXFree((Pointer)tmp);
	DXFree((Pointer)yuv_big_buf);
    }
    return NULL;
}

#define RRGGBB_OUTPUT_DELAYED_FLOAT(indexType) \
{ \
    indexType *pixels = (indexType *)DXGetArrayData(colors); \
    indexType *from; \
    RGBColor  *mapData = (RGBColor *)DXGetArrayData(map); \
 \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*width; \
	for (x=0; x<width; x++) { \
	    r = 255*mapData[from[x]].r; \
	    g = 255*mapData[from[x]].g; \
	    b = 255*mapData[from[x]].b; \
	    bufR[x] = CLAMP(r); \
	    bufG[x] = CLAMP(g); \
	    bufB[x] = CLAMP(b); \
	} \
 \
	if (n!=write (fh[0], bufR, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[1], bufG, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[2], bufB, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RRGGBB_OUTPUT_DELAYED_INT(indexType, mapType) \
{ \
    indexType *pixels = (indexType *)DXGetArrayData(colors); \
    indexType *from; \
    mapType  *mapData = (mapType *)DXGetArrayData(map); \
 \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*width; \
	for (x=0; x<width; x++) { \
	    mapType *m = mapData + 3*(*from++); \
	    r = *m++; \
	    g = *m++; \
	    b = *m++; \
	    bufR[x] = CLAMP(r); \
	    bufG[x] = CLAMP(g); \
	    bufB[x] = CLAMP(b); \
	} \
 \
	if (n!=write (fh[0], bufR, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[1], bufG, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[2], bufB, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RRGGBB_OUTPUT_DELAYED_INT_ALL(pixtype) \
switch(map_type) \
{ \
    case TYPE_UBYTE:  RRGGBB_OUTPUT_DELAYED_INT(pixtype, ubyte);  break; \
    case TYPE_USHORT: RRGGBB_OUTPUT_DELAYED_INT(pixtype, ushort); break; \
    case TYPE_SHORT:  RRGGBB_OUTPUT_DELAYED_INT(pixtype, short);  break; \
    case TYPE_INT:    RRGGBB_OUTPUT_DELAYED_INT(pixtype, int);    break; \
    case TYPE_UINT:   RRGGBB_OUTPUT_DELAYED_INT(pixtype, uint);   break; \
    case TYPE_FLOAT:  RRGGBB_OUTPUT_DELAYED_FLOAT(pixtype);   break; \
    default: \
    {  \
	DXSetError(ERROR_DATA_INVALID, \
	    "delayed color maps must be float, ubyte, ushort, short, int or uint"); \
	goto error; \
    } \
}

#define RRGGBB_OUTPUT_FLOAT \
{ \
    float *pixels = (float *)DXGetArrayData(colors); \
    float *from; \
    int wid = width * 3;  \
 \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*wid; \
	for (x=0; x<width; x++) { \
	    r = 255*(*from++); \
	    g = 255*(*from++); \
	    b = 255*(*from++); \
	    bufR[x] = CLAMP(r); \
	    bufG[x] = CLAMP(g); \
	    bufB[x] = CLAMP(b); \
	} \
 \
	if (n!=write (fh[0], bufR, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[1], bufG, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[2], bufB, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

#define RRGGBB_OUTPUT_INTEGER(int_type) \
{ \
    int_type *pixels = (int_type *)DXGetArrayData(colors); \
    int_type *from; \
    int wid = width * 3;  \
 \
    for (y=height-1; y>=0; y--) { \
	from = pixels + y*wid; \
	for (x=0; x<width; x++) { \
	    r = (*from++); \
	    g = (*from++); \
	    b = (*from++); \
	    bufR[x] = CLAMP(r); \
	    bufG[x] = CLAMP(g); \
	    bufB[x] = CLAMP(b); \
	} \
 \
	if (n!=write (fh[0], bufR, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[1], bufG, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
	if (n!=write (fh[2], bufB, n)) \
            DXErrorReturn ( ERROR_UNEXPECTED, "Error writing image file" ); \
    } \
}

Field
DXOutputRGBSeparate(Field i, int *fh)
{
    int x, y, n, r, g, b;
    ubyte *bufR = NULL, *bufG = NULL, *bufB = NULL;
    int width, height;
    Array colors, map;
    Type pixels_type;
    int  rank, shape[32];

    /* don't handle fb images for now */
    if (DXGetComponentValue(i, IMAGE))
	return i;

    if (!DXGetImageSize(i, &width, &height))
	goto error;

    n = width * sizeof(ubyte);

    bufR = (ubyte *) DXAllocate(n); 
    if (!bufR)
	goto error;

    bufG = (ubyte *) DXAllocate(n); 
    if (!bufG)
	goto error;

    bufB = (ubyte *) DXAllocate(n); 
    if (!bufB)
	goto error;

    colors = (Array)DXGetComponentValue(i, "colors");
    if (! colors)
	goto error;
    
    DXGetArrayInfo(colors, NULL, &pixels_type, NULL, &rank, shape);

    if (rank == 0 || (rank == 1 && shape[0] == 1))
    {
	Type map_type;
	int  mr, ms[32];

	map = (Array)DXGetComponentValue(i, "color map");
	if (! map)
	{
	    DXSetError(ERROR_DATA_INVALID, 
		"delayed-color image with no colormap");
	    goto error;
	}

	DXGetArrayInfo(map, NULL, &map_type, NULL, &mr, ms);
	if (mr != 1 || ms[0] != 3)
	{
	    DXSetError(ERROR_DATA_INVALID,
		"delayed color map must be 3-vector");
	    goto error;
	}

	switch(pixels_type)
	{
	    case TYPE_INT:    RRGGBB_OUTPUT_DELAYED_INT_ALL(int);    break;
	    case TYPE_UINT:   RRGGBB_OUTPUT_DELAYED_INT_ALL(uint);   break;
	    case TYPE_SHORT:  RRGGBB_OUTPUT_DELAYED_INT_ALL(short);  break;
	    case TYPE_USHORT: RRGGBB_OUTPUT_DELAYED_INT_ALL(ushort); break;
	    case TYPE_UBYTE:  RRGGBB_OUTPUT_DELAYED_INT_ALL(ubyte);  break;
	    default:
		DXSetError(ERROR_DATA_INVALID,
	    "delayed-color pixels must be ubyte, ushort, short, int or uint");
		goto error;
	}
    }
    else if (rank == 1 && shape[0] == 3)
    {
	switch(pixels_type)
	{
	    case TYPE_FLOAT:  RRGGBB_OUTPUT_FLOAT;           break;
	    case TYPE_INT:    RRGGBB_OUTPUT_INTEGER(int);    break;
	    case TYPE_UINT:   RRGGBB_OUTPUT_INTEGER(uint);   break;
	    case TYPE_SHORT:  RRGGBB_OUTPUT_INTEGER(short);  break;
	    case TYPE_USHORT: RRGGBB_OUTPUT_INTEGER(ushort); break;
	    case TYPE_UBYTE:  RRGGBB_OUTPUT_INTEGER(ubyte);  break;
	    default:
		DXSetError(ERROR_DATA_INVALID,
		    "pixels must be float, ubyte, ushort, short, int or uint");
		goto error;
	}
    }	
    else
    {
	DXSetError(ERROR_DATA_INVALID, "%s %s",
	    "image pixels must either be single valued (for delayed colors)",
	    "or 3-vectors");
	goto error;
    }

    DXFree((Pointer)bufR);
    DXFree((Pointer)bufG);
    DXFree((Pointer)bufB);

    return i;

error:
    DXFree((Pointer)bufR);
    DXFree((Pointer)bufG);
    DXFree((Pointer)bufB);
    return NULL;
}


/*
 * Conversion from floating point to 8-bit pixel
 * We do this by using the high order 16 bits of the ieee
 * float (giving us eight bits of precision) as an index into
 * a 64K byte table.  This combines fp to integer conversion,
 * range limiting, and gamma correction.  Even though the table
 * is 64K bytes, most of these are exceptional cases, so the
 * values we need mostly stay in the cache.
 */

void _dxfinit_convert(double gamma)
{
    static double convert_gamma = 0.0;	/* current gamma loaded into table */
    union hl f;
    int c, i;

    if (gamma==convert_gamma)		/* already computed? */
	return;
    f.f = 1.0;				/* start at pixel value 1.0 */
    i = UNSIGN(f.hl.hi);		/* unsigned index */
    memset(_dxd_convert+i, 255, NC-i);	/* set high values to 255 */
    do {				/* loop through lower values */
	f.hl.hi--;			/* go to next smaller number */
	c = pow(f.f, 1.0/gamma)*255.0;	/* f.f converts to c */
	_dxd_convert[UNSIGN(f.hl.hi)] = c;	/* put conversion in table */
    } while (c!=0);			/* stop when we get to 0 */
    convert_gamma = gamma;		/* record the current gamma value */
    DXMarkTimeLocal("init convert");
}


/*
 *
 */

Field
_dxf_ZeroPixels(Field image, int left, int right, int top, int bot, RGBColor color)
{
    Object o;
    RGBColor *fpixels;
    RGBByteColor *bpixels;
    
    o = DXGetComponentAttribute(image, IMAGE, IMAGE_TYPE);

    if (o)
    {
	if (o==O_FB_IMAGE)
	    return _dxf_ZeroFBPixels(image, left, right, top, bot, color);
	else if (o==O_X_IMAGE)
#if defined(DX_NATIVE_WINDOWS)
		return _dxf_ZeroWindowsPixels(image, left, right, top, bot, color);
#else
	    return _dxf_ZeroXPixels(image, left, right, top, bot, color);
#endif
	else
	    DXErrorReturn(ERROR_BAD_PARAMETER, "unknown image type");

    }
    else if ((fpixels = DXGetPixels(image)) != NULL)
    {
	if (color.r == 0.0 && color.g == 0.0 && color.b == 0.0)
	{
	    int bwidth = right-left, n = bwidth * sizeof(RGBColor);
	    int bheight = top-bot, iwidth, y;
	    RGBColor *tt;

	    DXGetImageSize(image, &iwidth, NULL);
	    tt = fpixels + bot*iwidth + left;

	    for (y=0; y<bheight; y++, tt+=iwidth)
		memset(tt, 0, n);
	}
	else
	{
	    int bwidth = right-left; 
	    int bheight = top-bot, iwidth, x, y;
	    RGBColor *tt;
	    float *t, r, g, b;

	    DXGetImageSize(image, &iwidth, NULL);
	    tt = fpixels + bot*iwidth + left;

	    r = color.r;
	    g = color.g;
	    b = color.b;

	    for (y=0; y<bheight; y++, tt+=iwidth)
	    {
		t = (float *)tt;
		for (x = 0; x < bwidth; x++)
		{
		    *t++ = r;
		    *t++ = g;
		    *t++ = b;
		}
	    }
	}
    }
    else
    {
	DXResetError();

	if ((bpixels = DXGetBytePixels(image)) == NULL)
	    DXErrorReturn(ERROR_BAD_PARAMETER, "unknown image type");
    
	if (color.r == 0.0 && color.g == 0.0 && color.b == 0.0)
	{
	    int bwidth = right-left, n = bwidth * sizeof(RGBByteColor);
	    int bheight = top-bot, iwidth, y;
	    RGBByteColor *tt;

	    DXGetImageSize(image, &iwidth, NULL);
	    tt = bpixels + bot*iwidth + left;

	    for (y=0; y<bheight; y++, tt+=iwidth)
		memset(tt, 0, n);
	}
	else
	{
	    int bwidth = right-left; 
	    int bheight = top-bot, iwidth, x, y;
	    RGBByteColor *tt;
	    ubyte *t, r, g, b;

	    DXGetImageSize(image, &iwidth, NULL);
	    tt = bpixels + bot*iwidth + left;

	    r = color.r > 1.0 ? 255 : color.r < 0.0 ? 0 : (ubyte)(color.r*255);
	    g = color.g > 1.0 ? 255 : color.g < 0.0 ? 0 : (ubyte)(color.g*255);
	    b = color.b > 1.0 ? 255 : color.b < 0.0 ? 0 : (ubyte)(color.b*255);

	    for (y=0; y<bheight; y++, tt+=iwidth)
	    {
		t = (ubyte *)tt;
		for (x = 0; x < bwidth; x++)
		{
		    *t++ = r;
		    *t++ = g;
		    *t++ = b;
		}
	    }
	}
    }

    return image;
}

/* new version of rgb to yuv converter */
static
int
rgb_to_yuv(drgb *buf, char *tmp, double *yuv_buf[3], int n_line)

{
     int i, other;
     double  Er1, Eg1, Eb1, Er2, Eg2, Eb2, Cr1, Cr2, Cb1, Cb2;
     double  Y, Cr, Cb;

/*   From Graphics File Formats by Brown and Shepherd 1995 p.124 */
/*   with corrections for the bad math in there                  */
/*   also, abekas file format is written out uyvy...             */
/*   Cr == U and Cb == V                                         */


	for(i = 0; i < n_line; i++) {

		if (i/2)
		    other = i-1;
		else
		    other = i+1;

		if (other >= n_line)
		    other = n_line-1;

		Er1 = buf[i].r; 
		Eg1 = buf[i].g;
		Eb1 = buf[i].b;
		Er2 = buf[other].r; 
		Eg2 = buf[other].g;
		Eb2 = buf[other].b;

                Y = 65.48*Er1 + 128.55*Eg1 + 24.97*Eb1 + 16;
                if (Y > 235) Y = 235;
                if (Y < 16) Y = 16;

                /* I believe this is writing out VYUY instead of
                   UYVY, but it seems to give the right pictures */
                if (i&0x01) {
                   Cr1 = 111.96*Er1 - 93.75*Eg1 -18.21*Eb1 + 128;
                   Cr2 = 111.96*Er2 - 93.75*Eg2 -18.21*Eb2 + 128;
		   Cr = (Cr1 + Cr2)/2.0;
                   if (Cr > 240) Cr = 240;
                   if (Cr < 16) Cr = 16;
                   tmp[2*i] = (ubyte)(Cr);
                   tmp[2*i+1] = (ubyte)(Y);
                }
                else { 
                   Cb1 = -37.77*Er1 - 74.16*Eg1 + 111.93*Eb1 + 128; 
                   Cb2 = -37.77*Er2 - 74.16*Eg2 + 111.93*Eb2 + 128; 
		   Cb = (Cb1 + Cb2)/2.0;
                   if (Cb > 240) Cb = 240;
                   if (Cb < 16) Cb = 16;
                   tmp[2*i] = (ubyte)(Cb);
                   tmp[2*i+1] = (ubyte)(Y);
                }
        }
	return OK;
}

