/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* 
 * (C) COPYRIGHT International Business Machines Corp. 1997
 * All Rights Reserved
 * Licensed Materials - Property of IBM
 * 
 * US Government Users Restricted Rights - Use, duplication or
 * disclosure restricted by GSA ADP Schedule Contract with IBM Corp.
 */

#include <dxconfig.h>



#define String dxString
#define Object dxObject
#define Angle dxAngle
#define Matrix dxMatrix
#define Screen dxScreen
#define Boolean dxBoolean

#include <dx/dx.h>

#undef String
#undef Object
#undef Angle
#undef Matrix
#undef Screen
#undef Boolean

#if !defined(DX_NATIVE_WINDOWS)
#include <X11/Xlib.h>
#endif

#include <math.h>
#include <stdlib.h>     /* for getenv prototype */

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwStereo.h"

/************** Default Stereo System Modes ********************
 * Default intialization is no system initialization. Default exit
 * is to delete the left and right windows if they differ from the
 * frame.  Two default window modes, both viewports inside the frame.
 * The first is side-by-size, dividing the frame in half.  The second
 * is top/bottom, with the top in the topmost 492 scanlines of the 
 * window and the bottom in the lowest 492 scan lines.
 ****************************************************************/

#if defined(DX_NATIVE_WINDOWS)
static int  defInitializeStereoSystemMode0(HDC, HWND);
static int  defExitStereoSystem0(HDC, HWND, HRGN *, HRGN *);
static int  defCreateStereoWindows0(HDC, HWND, HRGN *, WindowInfo *, HRGN *, WindowInfo *);

static int  defInitializeStereoSystemMode1(HDC, HWND);
static int  defExitStereoSystem1(HDC, HWND, HRGN *, HRGN *);
static int  defCreateStereoWindows1(HDC, HWND, HRGN *, WindowInfo *, HRGN *, WindowInfo *);

static int  defInitializeStereoSystemMode2(HDC, HWND);
static int  defExitStereoSystem2(HDC, HWND, HRGN *, HRGN *);
static int  defCreateStereoWindows2(HDC, HWND, HRGN *, WindowInfo *, HRGN *, WindowInfo *);

static int  defInitializeStereoSystemMode3(HDC, HWND);
static int  defExitStereoSystem3(HDC, HWND, HRGN *, HRGN *);
static int  defCreateStereoWindows3(HDC, HWND, HRGN *, WindowInfo *, HRGN *, WindowInfo *);

#else
static int  defInitializeStereoSystemMode0(Display *, Window);
static int  defExitStereoSystem0(Display *, Window, Window, Window);
static int  defCreateStereoWindows0(Display *, Window,
					Window *, WindowInfo *,
					Window *, WindowInfo *);

static int  defInitializeStereoSystemMode1(Display *, Window);
static int  defExitStereoSystem1(Display *, Window, Window, Window);
static int  defCreateStereoWindows1(Display *, Window,
					Window *, WindowInfo *,
					Window *, WindowInfo *);

static int  defInitializeStereoSystemMode2(Display *, Window);
static int  defExitStereoSystem2(Display *, Window, Window, Window);
static int  defCreateStereoWindows2(Display *, Window,
					Window *, WindowInfo *,
					Window *, WindowInfo *);

static int  defInitializeStereoSystemMode3(Display *, Window);
static int  defExitStereoSystem3(Display *, Window, Window, Window);
static int  defCreateStereoWindows3(Display *, Window,
					Window *, WindowInfo *,
					Window *, WindowInfo *);
#endif

static StereoSystemMode _defaultStereoSystemModes[4];

int 
DXDefaultStereoSystemModes(int *n, StereoSystemMode **ssms)
{
    *ssms = _defaultStereoSystemModes;

    (*ssms)[0].initializeStereoSystemMode = defInitializeStereoSystemMode0;
    (*ssms)[0].exitStereo 		  = defExitStereoSystem0;
    (*ssms)[0].createStereoWindows 	  = defCreateStereoWindows0;

    (*ssms)[1].initializeStereoSystemMode = defInitializeStereoSystemMode1;
    (*ssms)[1].exitStereo 		  = defExitStereoSystem1;
    (*ssms)[1].createStereoWindows 	  = defCreateStereoWindows1;

    (*ssms)[2].initializeStereoSystemMode = defInitializeStereoSystemMode2;
    (*ssms)[2].exitStereo 		  = defExitStereoSystem2;
    (*ssms)[2].createStereoWindows 	  = defCreateStereoWindows2;

	(*ssms)[3].exitStereo                 = defExitStereoSystem3;
	(*ssms)[3].createStereoWindows        = defCreateStereoWindows3;
	(*ssms)[3].initializeStereoSystemMode = defInitializeStereoSystemMode3;

    *n = 4;
    return OK;
}

#if defined(DX_NATIVE_WINDOWS)
static int
defInitializeStereoSystemMode0(HDC hdc, HWND w)
#else
static int
defInitializeStereoSystemMode0(Display *dpy, Window w)
#endif
{
    return OK;
}

#if defined(DX_NATIVE_WINDOWS)
static int
defExitStereoSystem0(HDC hdc, HWND w, HRGN *left, HRGN *right)
#else
static int
defExitStereoSystem0(Display *dpy, Window frame, Window left, Window right)
#endif
{
#if defined(DX_NATIVE_WINDOWS)
	return ERROR;
#else
    if (left != frame)
	XDestroyWindow(dpy, left);
    if (right != frame)
	XDestroyWindow(dpy, right);
    return OK;
#endif
}

#if defined(DX_NATIVE_WINDOWS)
Error
defCreateStereoWindows0(HDC hdc, HWND w,
						 HRGN *left, WindowInfo *leftWI,
						 HRGN *right, WindowInfo *rightWI)
#else
Error
defCreateStereoWindows0(Display *dpy, Window frame,
		Window *left, WindowInfo *leftWI,
		Window *right, WindowInfo *rightWI)
#endif
{
#if defined(DX_NATIVE_WINDOWS)
	return ERROR;
#else
    XWindowAttributes xwattr;
    XGetWindowAttributes(dpy, frame, &xwattr);

    *left = frame;
    leftWI->xOffset = 0;
    leftWI->yOffset = 0;
    leftWI->xSize = xwattr.width/2;
    leftWI->ySize = xwattr.height;

    leftWI->aspect = xwattr.height / xwattr.width;

    *right = frame;
    rightWI->xOffset = xwattr.width/2;
    rightWI->yOffset = 0;
    rightWI->xSize = xwattr.width/2;
    rightWI->ySize = xwattr.height;

    rightWI->aspect = xwattr.height / xwattr.width;

    return OK;
#endif
}

#if defined(DX_NATIVE_WINDOWS)
static int
defInitializeStereoSystemMode1(HDC hdc, HWND w)
#else
static int
defInitializeStereoSystemMode1(Display *dpy, Window w)
#endif
{
#ifdef sgi
    system("/usr/gfx/setmon -n STR_RECT");
#endif
    return OK;
}

#if defined(DX_NATIVE_WINDOWS)
static int
defExitStereoSystem1(HDC hdc, HWND w, HRGN *left, HRGN *right)
#else
static int
defExitStereoSystem1(Display *dpy, Window frame, Window left, Window right)
#endif
{
#if defined(DX_NATIVE_WINDOWS)
	return ERROR;
#else
#ifdef sgi
    system("/usr/gfx/setmon -n 60HZ");
#endif
    if (left != frame)
	XDestroyWindow(dpy, left);
    if (right != frame)
	XDestroyWindow(dpy, right);
    return OK;
#endif
}

#if defined(DX_NATIVE_WINDOWS)
Error
defCreateStereoWindows1(HDC hdc, HWND w,
						 HRGN *left, WindowInfo *leftWI,
						 HRGN *right, WindowInfo *rightWI)
#else
Error
defCreateStereoWindows1(Display *dpy, Window frame,
		Window *left, WindowInfo *leftWI,
		Window *right, WindowInfo *rightWI)
#endif
{
#if defined(DX_NATIVE_WINDOWS)
	return ERROR;
#else
    XWindowAttributes xwattr;
    XGetWindowAttributes(dpy, frame, &xwattr);

    *left = frame;
    leftWI->xOffset = 0;
    leftWI->yOffset = 0;
    leftWI->xSize = xwattr.width;
    leftWI->ySize = 492;

    leftWI->aspect = 492.0 / xwattr.width;

    *right = frame;
    rightWI->xOffset = 0;
    rightWI->yOffset = xwattr.height-492;
    rightWI->xSize = xwattr.width;
    rightWI->ySize = 492;

    rightWI->aspect = 492.0 / xwattr.width;

    return OK;
#endif
}

#if defined(DX_NATIVE_WINDOWS)
static int
defInitializeStereoSystemMode2(HDC hdc, HWND w)
#else
static int
defInitializeStereoSystemMode2(Display *dpy, Window w)
#endif
{
    char *cmd = getenv("DX_INIT_STEREO_COMMAND");
    if (cmd)
	system(cmd);

    return OK;
}

#if defined(DX_NATIVE_WINDOWS)
static int
defExitStereoSystem2(HDC hdc, HWND w, HRGN *left, HRGN *right)
#else
static int
defExitStereoSystem2(Display *dpy, Window frame, Window left, Window right)
#endif
{
    char *cmd = getenv("DX_EXIT_STEREO_COMMAND");
    if (cmd)
	system(cmd);

#if defined(DX_NATIVE_WINDOWS)
	return OK;
#else
    if (left != frame)
	XDestroyWindow(dpy, left);
    if (right != frame)
	XDestroyWindow(dpy, right);
    return OK;
#endif
}

#if defined(DX_NATIVE_WINDOWS)
Error
defCreateStereoWindows2(HDC hdc, HWND w,
						 HRGN *left, WindowInfo *leftWI,
						 HRGN *right, WindowInfo *rightWI)
#else
Error
defCreateStereoWindows2(Display *dpy, Window frame,
		Window *left, WindowInfo *leftWI,
		Window *right, WindowInfo *rightWI)
#endif
{
#if defined(DX_NATIVE_WINDOWS)
	return ERROR;
#else

    XWindowAttributes xwattr;
    XGetWindowAttributes(dpy, frame, &xwattr);

    *left = frame;
    leftWI->xOffset = 0;
    leftWI->yOffset = 0;
    leftWI->xSize = xwattr.width;
    leftWI->ySize = xwattr.height/2;

    rightWI->aspect = ((float)rightWI->ySize) / rightWI->xSize;

    *right = frame;
    rightWI->xOffset = 0;
    rightWI->yOffset = xwattr.height/2;
    rightWI->xSize = xwattr.width;
    rightWI->ySize = xwattr.height/2;

    rightWI->aspect = ((float)rightWI->ySize) / rightWI->xSize;

    return OK;
#endif
}

/********************************************************************/
/* GL stereo mode */

#if !defined(DX_NATIVE_WINDOWS)
Error
defCreateStereoWindows3(Display *dpy, Window frame,
		Window *left, WindowInfo *leftWI,
		Window *right, WindowInfo *rightWI) {
	return ERROR;

#else

int defCreateStereoWindows3(HDC hdc, HWND w,
						 HRGN *left, WindowInfo *leftWI,
						 HRGN *right, WindowInfo *rightWI) {
	RECT rect;

	if (GetWindowRect(w, &rect)) {
		leftWI->xOffset = rightWI->xOffset = 0;
		leftWI->yOffset = rightWI->yOffset = 0;
		leftWI->xSize   = rightWI->xSize   = rect.right - rect.left;
		leftWI->ySize   = rightWI->ySize   = rect.bottom - rect.top;
		leftWI->aspect  = rightWI->aspect  =
			((float) leftWI->ySize) / leftWI->xSize;
		return OK;
	}
	return ERROR;
#endif
}

#if !defined(DX_NATIVE_WINDOWS)
static int defInitializeStereoSystemMode3(Display *dpy, Window w) {
	return ERROR;

#else

static int defInitializeStereoSystemMode3(HDC hdc, HWND w) {
	char *cmd;
	int checkID;
	PIXELFORMATDESCRIPTOR checkPfd;

	/* reuse 'cmd' here */
	cmd = getenv("DX_USE_GL_STEREO");
	if (!atoi(cmd)) {
		DXMessage("InitStereoSysMode: GL stereo not requested, aborting.");
		return ERROR;
	}
	checkID  = GetPixelFormat(hdc);
	if (!DescribePixelFormat(hdc, checkID,
		sizeof(PIXELFORMATDESCRIPTOR), &checkPfd)) {
		return ERROR;
	}
	if ((checkPfd.dwFlags & PFD_STEREO) == 0) {
		DXMessage("InitStereoSysMode: init failed: PFD_STEREO not set");
		return ERROR;
	}

	return OK;
#endif
}

#if !defined(DX_NATIVE_WINDOWS)
static int defExitStereoSystem3(Display *dpy, Window frame, Window left, Window right) {
	return ERROR;

#else

static int defExitStereoSystem3(HDC hdc, HWND w, HRGN *left, HRGN *right) {
//	char *cmd;

/* needed?
	cmd = getenv("DX_EXIT_STEREO_COMMAND");
	if (cmd)			system(cmd);
*/
	return OK;
#endif
}
