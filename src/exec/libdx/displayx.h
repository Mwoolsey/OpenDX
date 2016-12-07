/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999,2002                              */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/libdx/displayx.h,v 1.2 2003/07/11 05:50:41 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _DISPLAYX_H_
#define _DISPLAYX_H_ 

#if !defined(DX_NATIVE_WINDOWS)
#include <dx/dx.h>
#include <sys/types.h>
#include "internals.h"

Object		DXDisplayXAny(Object, int, char*, char*);
Object 		DXDisplayX(Object, char *, char *);
Object 		DXDisplayX4(Object, char *, char *);
Object 		DXDisplayX8(Object, char *, char *);
Object 		DXDisplayX12(Object, char *, char *);
Object 		DXDisplayX15(Object, char *, char *);
Object 		DXDisplayX16(Object, char *, char *);
Object 		DXDisplayX24(Object, char *, char *);
Object 		DXDisplayX32(Object, char *, char *);
int    		_dxf_getXDepth(char *);
unsigned char * _dxf_GetXPixels(Field);
Field 		_dxf_MakeXImage(int, int, int, char *);
Field		_dxf_ZeroXPixels(Field, int, int, int, int, RGBColor);
Field 		_dxf_CaptureXSoftwareImage(int, char *, char *);
Error		_dxf_translateXImage(Object, int, int, int, int,
				int, int, int, int, int, int, translationP, 
				unsigned char *, int, unsigned char *);
Error		_dxf_SetSoftwareXWindowActive(char *, int);


#endif /* !DX_NATIVE_WINDOWS */

#endif /* _DISPLAYX_H_ */

