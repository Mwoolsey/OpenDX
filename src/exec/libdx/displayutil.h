/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _DISPLAYW_UTIL_
#define _DISPLAYW_UTIL_

#include "internals.h"

struct arg {
    Object			image;			/* input image */
    int				width, height;	/* image size */
    int				ox, oy;			/* image origin */
    int				i;				/* which band */
    int				n;				/* number of bands */
    translationP	translation; 	/* color map translation */
    unsigned char	*out;			/* output pixel array */
};

struct buffer;

Error		_dxf_dither(Object, int, int, int, int, void *, unsigned char *);
int			_dxf_getXDepth(char *);
translationP  _dxf_GetXTranslation(Object);
Error		DXParseWhere(char *, int *, char **, char **, char **, char **, char **);
Field		_dxfSaveSoftwareWindow(char *);
Object		DXDisplay(Object, int, char *, char *);
unsigned char * _dxf_GetPixels(Field);
Field		_dxf_MakeImage(int, int, int, char *);
Field		_dxf_CaptureSoftwareImage(int, char *, char *);
Error		DXSetSoftwareWindowActive(char *, int);
Error		_dxf_CopyBufferToPImage(struct buffer*, Field, int, int);

#if defined(WORDS_BIGENDIAN)
#define COLORMASK12 (XImageByteOrder(t->dpy) == MSBFirst) ? 0x0fff : 0xfff0
#else
#if !defined(DX_NATIVE_WINDOWS)
#define COLORMASK12 (XImageByteOrder(t->dpy) == MSBFirst) ? 0xfff0 : 0x0fff
#else
#define COLORMASK12 0xfff0
#endif /* DX_NATIVE_WINDOWS */
#endif /* WORDS_BIGENDIAN */

#if defined(WORDS_BIGENDIAN)
#define COLORMASK24 (XImageByteOrder(t->dpy) == MSBFirst) ? 0x00ffffff : 0xffffff00
#else
#if !defined(DX_NATIVE_WINDOWS)
#define COLORMASK24 (XImageByteOrder(t->dpy) == MSBFirst) ? 0xffffff00 : 0x00ffffff
#else
#define COLORMASK24 0xffffff00
#endif /* DX_NATIVE_WINDOWS */
#endif /* WORDS_BIGENDIAN */

#endif /* _DISPLAYUTIL_H_ */

