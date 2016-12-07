/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* 
 * $Header: /src/master/dx/src/exec/hwrender/hwDraw.c,v 1.12 2006/01/03 17:02:26 davidt Exp $
 */

#include <dxconfig.h>

#define render_c

#include <stdio.h>
#include <math.h>
#include "hwDeclarations.h"
#include "hwPortLayer.h"
#include "hwWindow.h"
#include "hwObject.h"
#include "hwScreen.h"
#include "hwView.h"
#include "hwGather.h"
#include "hwMatrix.h"
#include "hwDebug.h"

#if !defined(DX_NATIVE_WINDOWS)
#include <X11/Xlib.h>
#endif

#if !defined(hp700)

#if !defined(DX_NATIVE_WINDOWS)
#include <GL/glx.h>
#endif

#include <GL/gl.h>
#endif

Error _dxfDrawMono (gatherO, void*, dxObject, Camera, int);
Error _dxfDrawStereo (gatherO, void*, dxObject, Camera, int);
Error _dxfDrawEither (gatherO, void*, dxObject, Camera, int);
typedef float MATRIX[4][4];

extern void _dxf_computeMatrix(float *, float *, float *, float, MATRIX *);
extern void _dxfSetProjectionMatrix(void *, void *, MATRIX);

extern dxObject _dxf_getCachedObject(void *, dxObject);
extern Error _dxf_gather(dxObject object, gatherO gather, void *globals);
extern Error _dxf_updateView(viewO view, void *globals, screenO camScreen);
extern Error _dxf_paint(void* globals, viewO view, int buttonUp, screenO camScreen);

#if defined(DX_NATIVE_WINDOWS)
static HANDLE OGLLock = NULL;
static int firstRender = 1;
#endif


Error
_dxfDrawMonoInCurrentContext (void* globals, dxObject o, Camera c, int buttonUp)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  gatherO gather = NULL;
  int r;


#if defined(DX_NATIVE_WINDOWS)
  if (firstRender)
  {
	  firstRender = 0;
	  OGLLock = CreateMutex(NULL, TRUE, NULL);
  }
  else
  {
	  if (DXWaitForSignal(1, &OGLLock) == WAIT_TIMEOUT)
		  return ERROR;
  }
#endif

#if !defined(DX_NATIVE_WINDOWS)
  if (DXUI)
    DXUIMessage("HW","begin");
#endif

  if (GATHER_TAG == DXGetObjectTag(o))
  {
    gather = (gatherO)GATHER;
  }
  else
  {
    if (GATHER)
    {
      DXDelete(GATHER);
      GATHER = NULL;
    }

    GATHER_TAG = -1;
    gather = _dxf_newHwGather();
    if (! gather)
      goto error;

    if (! _dxf_gather(o, gather, globals))
      goto error;

    GATHER     = DXReference((dxObject)gather);
    GATHER_TAG = DXGetObjectTag(o);
    OBJECT_TAG = GATHER_TAG;
  }

  /*
   * It seems like there's an unnecessary reference here... 
   * gather is owned by globals about 6 lines up.  Shouldn't need
   * this extra ref - which needed to be matched by a DXDelete 
   * below.
   */
  DXReference((dxObject)gather);

  _dxf_STARTFRAME(LWIN);

    _dxf_CLEARBUFFER(LWIN);
    _dxfDrawEither(gather, globals, o, c, buttonUp);

#if defined(DX_NATIVE_WINDOWS)
    _dxf_SWAP_BUFFERS(PORT_CTX);
#else
    _dxf_SWAP_BUFFERS(PORT_CTX, XWINID);
#endif

  if (DXUI)
    DXUIMessage("HW","end");

  _dxf_ENDFRAME(LWIN);

  DXDelete((dxObject)gather);

  return r;

error:
  if (gather)
    DXDelete((dxObject)gather);
  return ERROR;
}


Error
_dxfDraw (void* globals, dxObject o, Camera c, int buttonUp)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  gatherO gather = NULL;
  int r;


#if defined(DX_NATIVE_WINDOWS)
  if (firstRender)
  {
	  firstRender = 0;
	  OGLLock = CreateMutex(NULL, TRUE, NULL);
  }
  else
  {
	  if (DXWaitForSignal(1, &OGLLock) == WAIT_TIMEOUT)
		  return ERROR;
  }
#endif

#if !defined(DX_NATIVE_WINDOWS)
  if (DXUI)
    DXUIMessage("HW","begin");
#endif

  if (GATHER_TAG == DXGetObjectTag(o))
  {
    gather = (gatherO)GATHER;
  }
  else
  {
    if (GATHER)
    {
      DXDelete(GATHER);
      GATHER = NULL;
    }

    GATHER_TAG = -1;
    gather = _dxf_newHwGather();
    if (! gather)
      goto error;

    if (! _dxf_gather(o, gather, globals))
      goto error;

    GATHER     = DXReference((dxObject)gather);
    GATHER_TAG = DXGetObjectTag(o);
    OBJECT_TAG = GATHER_TAG;
  }

  /*
   * It seems like there's an unnecessary reference here... 
   * gather is owned by globals about 6 lines up.  Shouldn't need
   * this extra ref - which needed to be matched by a DXDelete 
   * below.
   */
  DXReference((dxObject)gather);

  _dxf_STARTFRAME(LWIN);

  if (STEREOSYSTEMMODE >= 0)
      r = _dxfDrawStereo(gather, globals, o, c, buttonUp);
  else
      r = _dxfDrawMono(gather, globals, o, c, buttonUp);

  if (DXUI)
    DXUIMessage("HW","end");

  _dxf_ENDFRAME(LWIN);

  DXDelete((dxObject)gather);

  return r;

error:
  if (gather)
    DXDelete((dxObject)gather);
  return ERROR;
}


Error
_dxfDrawMono (gatherO gather, void* globals, dxObject o, Camera c, int buttonUp)
{
    int pixw, pixh;
    DEFGLOBALDATA(globals);
    DEFPORT(PORT_HANDLE);

#if defined(DX_NATIVE_WINDOWS)
  {
    RECT rect;
    _dxf_SET_OUTPUT_WINDOW(LWIN, LEFTWINDOW);
    GetClientRect(XWINID, &rect);
    pixw = rect.right - rect.left;
    pixh = rect.bottom - rect.top;
  }
#else
  {
    XWindowAttributes xwa;

    _dxf_SET_OUTPUT_WINDOW(LWIN, XWINID);
    XGetWindowAttributes(DPY, XWINID, &xwa);
    pixw = xwa.width - 1;
    pixh = xwa.height - 1;
  }
#endif

    _dxf_SET_VIEWPORT (PORT_CTX, 0, pixw, 0, pixh) ;
    _dxfSetViewport (MATRIX_STACK, 0, pixw, 0, pixh) ;
    _dxf_CLEARBUFFER(LWIN);
    _dxfDrawEither(gather, globals, o, c, buttonUp);

#if defined(DX_NATIVE_WINDOWS)
    _dxf_SWAP_BUFFERS(PORT_CTX);
#else
    _dxf_SWAP_BUFFERS(PORT_CTX, XWINID);
#endif

    return OK;
}

#define DEFCAMERA(interactorData)       \
    register tdmInteractorCam _cdata = (interactorData)->Cam

extern Camera _dxfSetCameraProjection(Camera c, float *p);

Error
_dxfGetStereoCameras (void *globals, Camera c, Camera *lcp, Camera *rcp)
{
    DEFGLOBALDATA(globals);
    DEFPORT(PORT_HANDLE);
    DEFCAMERA(INTERACTOR_DATA);
    float Near, Far;
    float lto[3], lfrom[3], lup[3], *lProjection;
    float rto[3], rfrom[3], rup[3], *rProjection;
    float zaxis[3], x, y, z, d;
    Camera sc;

    x = CURRENT_TO[0] - CURRENT_FROM[0];
    y = CURRENT_TO[1] - CURRENT_FROM[1];
    z = CURRENT_TO[2] - CURRENT_FROM[2];
    d = sqrt(x*x + y*y + z*z);
    zaxis[0] = x/d;
    zaxis[1] = y/d;
    zaxis[2] = z/d;

    _dxfGetNearFar(CDATA(projection), CDATA(w),
                   CDATA(projection) ? CDATA(fov) : CDATA(width),
                   CURRENT_FROM, zaxis, CDATA(box), &Near, &Far);

    _dxfCreateStereoCameras(globals,
                            CDATA(projection), CURRENT_FOV, CURRENT_WIDTH,
                            CURRENT_TO, CURRENT_FROM, CURRENT_UP, Near, Far,
                            lto, lfrom, lup, &lProjection,
                            rto, rfrom, rup, &rProjection);

    sc = (Camera)DXCopy((dxObject)c, COPY_STRUCTURE);
    sc = DXSetView(sc,
                DXPt(lfrom[0], lfrom[1], lfrom[2]),
                DXPt(lto[0],   lto[1],   lto[2]),
                DXVec(lup[0],  lup[1],   lup[2]));
    sc = _dxfSetCameraProjection(sc, lProjection);
    *lcp = sc;

    sc = (Camera)DXCopy((dxObject)c, COPY_STRUCTURE);
    sc = DXSetView(sc,
                DXPt(rfrom[0], rfrom[1], rfrom[2]),
                DXPt(rto[0],   rto[1],   rto[2]),
                DXVec(rup[0],  rup[1],   rup[2]));
    sc = _dxfSetCameraProjection(sc, rProjection);
    *rcp = sc;

    return OK;
}

Error
_dxfDrawStereo (gatherO gather, void* globals, dxObject o, Camera c, int buttonUp)
{
    DEFGLOBALDATA(globals);
    DEFPORT(PORT_HANDLE);
    DEFCAMERA(INTERACTOR_DATA);
    float Near, Far;
    float lto[3], lfrom[3], lup[3], *lProjection;
    float rto[3], rfrom[3], rup[3], *rProjection;
    float zaxis[3], x, y, z, d;

    x = CURRENT_TO[0] - CURRENT_FROM[0];
    y = CURRENT_TO[1] - CURRENT_FROM[1];
    z = CURRENT_TO[2] - CURRENT_FROM[2];
    d = sqrt(x*x + y*y + z*z);
    zaxis[0] = x/d;
    zaxis[1] = y/d;
    zaxis[2] = z/d;

    _dxfGetNearFar(CDATA(projection), CDATA(w),
                   CDATA(projection) ? CDATA(fov) : CDATA(width),
                   CURRENT_FROM, zaxis, CDATA(box), &Near, &Far);

    _dxfCreateStereoCameras(globals, 
		            CDATA(projection), CURRENT_FOV, CURRENT_WIDTH,
			    CURRENT_TO, CURRENT_FROM, CURRENT_UP, Near, Far,
                            lto, lfrom, lup, &lProjection,
                            rto, rfrom, rup, &rProjection);

    _dxf_SET_OUTPUT_WINDOW(LWIN, LEFTWINDOW);

#if defined(DX_NATIVE_WINDOWS)

	if (USEGLSTEREO)
	{
		glDrawBuffer(GL_BACK_LEFT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
#endif

    _dxf_CLEARBUFFER(LWIN);

    _dxf_SET_VIEWPORT (PORT_CTX,
                LEFTWINDOWINFO.xOffset,
                LEFTWINDOWINFO.xOffset + LEFTWINDOWINFO.xSize,
                LEFTWINDOWINFO.yOffset,
                LEFTWINDOWINFO.yOffset + LEFTWINDOWINFO.ySize);
    _dxfSetViewport (MATRIX_STACK,
                LEFTWINDOWINFO.xOffset,
                LEFTWINDOWINFO.xOffset + LEFTWINDOWINFO.xSize,
                LEFTWINDOWINFO.yOffset,
                LEFTWINDOWINFO.yOffset + LEFTWINDOWINFO.ySize);

    _dxf_computeMatrix(lto, lfrom, lup, CDATA(width), (MATRIX *)&CDATA(viewXfm));

    _dxfLoadViewMatrix(CDATA(stack), CDATA(viewXfm));

    if (lProjection)
        _dxfSetProjectionMatrix(PORT_CTX,  MATRIX_STACK, *(MATRIX *)lProjection);
    else if (CDATA(projection))
        _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, CURRENT_FOV, LEFTWINDOWINFO.aspect, Near, Far);
    else
        _dxf_SET_ORTHO_PROJECTION(PORT_CTX, CURRENT_WIDTH, LEFTWINDOWINFO.aspect, Near, Far);

    if (! _dxfDrawEither(gather, globals, o, c, buttonUp))
        return ERROR;

#if defined(DX_NATIVE_WINDOWS)
	if (USEGLSTEREO)
	{
		glDrawBuffer(GL_BACK_RIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
#endif

    if (LEFTWINDOW != RIGHTWINDOW)
    {
        _dxf_SET_OUTPUT_WINDOW(LWIN, RIGHTWINDOW);
        _dxf_CLEARBUFFER(LWIN);
    }

    _dxf_SET_VIEWPORT (PORT_CTX,
                RIGHTWINDOWINFO.xOffset,
                RIGHTWINDOWINFO.xOffset + RIGHTWINDOWINFO.xSize,
                RIGHTWINDOWINFO.yOffset,
                RIGHTWINDOWINFO.yOffset + RIGHTWINDOWINFO.ySize);
    _dxfSetViewport (MATRIX_STACK,
                RIGHTWINDOWINFO.xOffset,
                RIGHTWINDOWINFO.xOffset + RIGHTWINDOWINFO.xSize,
                RIGHTWINDOWINFO.yOffset,
                RIGHTWINDOWINFO.yOffset + RIGHTWINDOWINFO.ySize);

    _dxf_computeMatrix(rto, rfrom, rup, CDATA(width), (MATRIX *)&CDATA(viewXfm));
    _dxfLoadViewMatrix(CDATA(stack), CDATA(viewXfm));

    if (rProjection)
        _dxfSetProjectionMatrix(PORT_CTX,  MATRIX_STACK, *(MATRIX *)rProjection);
    else if (CDATA(projection))
        _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, CURRENT_FOV, RIGHTWINDOWINFO.aspect, Near, Far);
    else
        _dxf_SET_ORTHO_PROJECTION(PORT_CTX, CURRENT_WIDTH, RIGHTWINDOWINFO.aspect, Near, Far);

    if (! _dxfDrawEither(gather, globals, o, c, buttonUp))
        return ERROR;

    _dxf_SET_OUTPUT_WINDOW(LWIN, LEFTWINDOW);

#if defined(DX_NATIVE_WINDOWS)
	if (USEGLSTEREO)
	{
		glDrawBuffer(GL_BACK_LEFT);
	}
#endif

#if defined(DX_NATIVE_WINDOWS)
    _dxf_SWAP_BUFFERS(PORT_CTX);
#else
    _dxf_SWAP_BUFFERS(PORT_CTX, LEFTWINDOW);
#endif

    if (LEFTWINDOW != RIGHTWINDOW)
    {
        _dxf_SET_OUTPUT_WINDOW(LWIN, RIGHTWINDOW);
#if defined(DX_NATIVE_WINDOWS)
		_dxf_SWAP_BUFFERS(PORT_CTX);
#else
        _dxf_SWAP_BUFFERS(PORT_CTX, RIGHTWINDOW);
#endif
    }
    return OK;

}

Error
_dxfDrawEither (gatherO gather, void* globals, dxObject o, Camera c, int buttonUp)
{
  DEFGLOBALDATA(globals);
  DEFPORT(PORT_HANDLE);
  viewO view = NULL;
  RGBColor background;
  float pm[4][4];
  float vm[4][4];
  int left,right,bottom,top,width, height;
  int flags;
  screenO camScreen = NULL;

  DXReference((dxObject)gather);

  /*
   * Create a new view structure each time through.  We used to
   * cache the view structure, but that didn't work (see NSC85)
   */
  PRINT(("creating new view structure"));

  TIMER("> View");
  _dxfGetViewMatrix(MATRIX_STACK, vm);   /* XXX this misses the fuzz */
  _dxfGetProjectionMatrix(PORT_HANDLE,MATRIX_STACK, pm);
  _dxfGetViewport(MATRIX_STACK,&left,&right,&bottom,&top);
  DXGetBackgroundColor(c,&background);
  
  view = _dxf_newHwView();
  if (! view)
      goto error;

  if (! _dxf_setHwViewGather(view, gather))
      goto error;

  if (! _dxf_setHwViewProjectionMatrix(view, pm) ||
      ! _dxf_setHwViewViewMatrix(view, vm)       ||
      ! _dxf_setHwViewBackground(view, background))
      goto error;
    
  if (! _dxf_getHwGatherFlags(gather, &flags))
      goto error;
    
  if (! _dxf_setHwViewFlags(view, flags))
      goto error;

  width = right - left;
  height = top - bottom;

  camScreen = _dxf_newHwScreen(NULL,NULL,vm,pm,width,height);
  if (! camScreen)
      goto error;

  if (! _dxf_updateView(view,globals,camScreen))
    goto error;

  TIMER("< View");

  /*
   * We now have a gather structure that has the correct object
   * and view so let's render it.
   */
  TIMER("> Paint");
  if (!(_dxf_paint(globals, view, buttonUp, camScreen)))
    goto error;
  TIMER("< Paint");
  
  /*
   * I don't think you can get into this code anymore if there is no
   * camera but I won't change this because it's working. pka 20Nov96
   */
  if (c) {
    DXDelete((dxObject)view);
    DXDelete((dxObject)gather);
  }

  if (camScreen)
      DXDelete(camScreen);

#if defined(DX_NATIVE_WINDOWS)
  ReleaseMutex(OGLLock);
#endif

  EXIT(("OK"));
  return OK;
  
 error:

#if defined(DX_NATIVE_WINDOWS)
  ReleaseMutex(OGLLock);
#endif

  if (c) {
    if (view)
	DXDelete((dxObject)view);
    if (gather)
	DXDelete((dxObject)gather);
  }

  if (camScreen)
      DXDelete(camScreen);

  EXIT(("ERROR"));
  return ERROR;
}

#undef render_c
