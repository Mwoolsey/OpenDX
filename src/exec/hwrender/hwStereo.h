/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
#ifndef _STEREO_H_
#define _STEREO_H_

#include <dxconfig.h>

#define DX_STEREO_LEFT  1
#define DX_STEREO_RIGHT 2

#include <dxstereo.h>

Error _dxfInitializeStereoSystemMode(void *, dxObject);
Error _dxfExitStereoSystemMode(void *);

Error _dxfInitializeStereoCameraMode(void *, dxObject);
Error _dxfExitStereoCameraMode(void *);
Error _dxfCreateStereoCameras(void *,
				 int, float, float,
				 float *, float *, float *, float, float,
				 float *, float *, float *, float **,
				 float *, float *, float *, float **);
#if defined(DX_NATIVE_WINDOWS)
Error _dxfMapStereoXY(void *, HWND, HRGN, WindowInfo, int, int, int*, int*);
#else
Error _dxfMapStereoXY(void *, Window, Window, WindowInfo, int, int, int*, int*);
#endif

#endif
