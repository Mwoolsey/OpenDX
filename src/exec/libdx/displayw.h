/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 2002                                        */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#ifndef _DISPLAYW_H_
#define _DISPLAYW_H_

#if defined(DX_NATIVE_WINDOWS)

#include <dx/dx.h>
#include <windows.h>
#include <sys/types.h>
#include "internals.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void 		_dxfResizeSWWindow(SWWindow *);
int 		DXInitializeWindows(HINSTANCE, DWORD);
DWORD 		_dxfGetContainerThread();
Error 		DXGetWindowsInstance(HINSTANCE *);
void *		_dxf_GetTranslation(Object);
Error 		_dxfCreateSWWindow (SWWindow *);
void 		_dxfResizeSWWindow(SWWindow *);
unsigned char * _dxf_GetWindowsPixels(Field, int *);
Error		_dxf_SetSoftwareWindowsWindowActive(char *, int);
Error		DXSetSoftwareWindowsWindowActive(char *, int);
Object		DXDisplayWindows(Object, int, char *, char *);
Field 		_dxf_CaptureWindowsSoftwareImage(int, char *, char *);
Field 		_dxf_MakeWindowsImage(int, int, int, char *);
Field		_dxf_ZeroWindowsPixels(Field, int, int, int, int, RGBColor);
Error		_dxf_translateWindowsImage(Object,  int, int, int, int, int, int,
				int, int, int, int, translationP, unsigned char *, int, unsigned char *);
Error		_dxf_CopyBufferToWindowsImage(struct buffer *, Field, int, int);
HWND		DXGetWindowsHandle(char *where);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* DX_NATIVE_WINDOWS */

#endif /* _DISPLAYW_H_ */

