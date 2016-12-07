/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/* 
 * $Header: /src/master/dx/src/exec/hwrender/hwInitScreen.c,v 1.8 2006/01/03 17:02:27 davidt Exp $
 */

#include <dxconfig.h>


#define tdmInitScreen_c

/*
 * Environment:		Risc System 6000
 * Current Source:	$Source: /src/master/dx/src/exec/hwrender/hwInitScreen.c,v $
 * Author:		Dave Owens
 *
 */

#include <stdio.h>
#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#elif defined(HAVE_SYS_SIGNAL_H)
#include <sys/signal.h>
#endif
#include <math.h>

#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwPortLayer.h"
#include "hwClientMessage.h"
#include "hwMemory.h"

#include "hwDebug.h"

typedef Error (*Handler)(int, Pointer);

extern void _dxfCacheState(char *, dxObject, dxObject, int);
extern void _dxfFreeStack (void *stack);
extern void _dxfDeletePortHandle(tdmPortHandleP);
extern void _dxf_DeleteObjectHash(HashTable);

Error
_dxfInitRenderObject(WinP win)
{
  /*
   *  This function is called each time we are starting to render a new object.
   *  This means either the camera or the object has changed.
   */
  
  Vector up ;
  int projection, pixwidth, pixheight ;
  Point to, box[8], from, zaxis;
  float width, aspect, fov ;
  float mat[4][4], dist, Near, Far ;
  dxMatrix rot ;
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxfInitRenderObject(0x%x)",win));

  /* mark the software image and approximation buffers invalid */
  SW_BUF_CURRENT = FALSE ;
  SAVE_BUF_VALID = FALSE ;


  /* get view coordinate system */
  if (! DXGetView(CAMERA, &from, &to, &up)) {
    EXIT(("ERROR: unable to get camara view"));
    DXErrorReturn (ERROR_DATA_INVALID, "unable to get camera view") ;
  }

  /* get camera resolution */
  DXGetCameraResolution(CAMERA, &pixwidth, &pixheight) ;

  /* get view orientation matrix */
  rot = DXGetCameraRotation(CAMERA) ;

  /* get projection parameters */
  if (DXGetOrthographic(CAMERA, &width, &aspect))
  {
      fov = 0.0;
      projection = 0 ;
  }
  else if (DXGetPerspective(CAMERA, &fov, &aspect))
  {
      width = 0.0;
      projection = 1 ;
  }
  else {
    EXIT(("ERROR: unable to get projection data"));
    DXErrorReturn (ERROR_DATA_INVALID, "unable to obtain projection data") ;
  }

  _dxfSetCurrentView(win, (float *)&to, (float *)&from, (float *)&up, fov, width);

  /* compute view zaxis */
  zaxis = DXSub(to, from) ;
  dist = (float)DXLength(zaxis) ;
  zaxis.x /= dist ; zaxis.y /= dist ; zaxis.z /= dist ;

  if (DXBoundingBox(OBJECT, box))
    {
      /* find suitable near and far clip plane distances */
      _dxfGetNearFar(projection, pixwidth, projection? fov: width,
		    (float *)&from, (float *)&zaxis, (float (*)[3])box,
		    &Near, &Far);

      /* set bounding box for interactors that need it */
      _dxfSetInteractorBox(INTERACTOR_DATA, (float (*)[3])box) ;
    }
  else
    {
      PRINT(("Could not obtain bounding box!")) ;
      DXResetError() ;
      Far =   10000.0 ;
      Near = -10000.0 ;
    }

  /* set up view projection */
  if (projection)
    {
      _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, fov, aspect, Near, Far) ;
      _dxfSetInteractorViewInfo(INTERACTOR_DATA,
			       (float *)&from, (float *)&to, (float *)&up,
			       dist, fov, 0, aspect, 1, Near, Far) ;

      PRINT(("set perspective fov = %f Near = %d Far = %f",
	     RAD2DEG(2 * atan(fov/2)), Near, Far));
    }
  else
    {
      _dxf_SET_ORTHO_PROJECTION(PORT_CTX, width, aspect, Near, Far) ;
      _dxfSetInteractorViewInfo(INTERACTOR_DATA,
			       (float *)&from, (float *)&to, (float *)&up,
			       dist, 0, width, aspect, 0, Near, Far) ;

      PRINT(("set ortho view width = %f height = %f Near = %f Far = %f",
	     width, width*aspect, Near, Far));
    }

  /* set up hardware viewing matrix and software shadow */
  mat[0][0] = rot.A[0][0] ; mat[0][1] = rot.A[0][1] ; mat[0][2] = rot.A[0][2] ;
  mat[1][0] = rot.A[1][0] ; mat[1][1] = rot.A[1][1] ; mat[1][2] = rot.A[1][2] ;
  mat[2][0] = rot.A[2][0] ; mat[2][1] = rot.A[2][1] ; mat[2][2] = rot.A[2][2] ;
  mat[3][0] =    rot.b[0] ; mat[3][1] =    rot.b[1] ; mat[3][2] =    rot.b[2] ;

  mat[0][3] = 0.0 ; mat[1][3] = 0.0 ; mat[2][3] = 0.0 ; mat[3][3] = 1.0 ;

  _dxf_LOAD_MATRIX (PORT_CTX, mat) ;
  if(projection)
    _dxfInitStack (MATRIX_STACK, pixwidth, mat, projection,
		   fov, aspect, Near, Far) ;
  else
    _dxfInitStack (MATRIX_STACK, pixwidth, mat, projection,
		   width, aspect, Near, Far) ;

  _dxfCacheState(ORIGINALWHERE, OBJECT, CAMERA, 1);

  PRINT(("View matrix:"));
  MPRINT(mat);
  EXIT(("OK"));
  return OK ;
}

int
_dxfEndRenderPass (WinP win)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxfEndRenderPass(0x%x)",win));

  if ( _dxfTrySaveBuffer(win))
  {
      SAVEBUFCAMERATAG = CAMERA_TAG;
      SAVEBUFOBJECTTAG = OBJECT_TAG;
  }


  /*
   *  Redraw any defined interactor echos.  The echos can't be saved by
   *  _dxfTrySaveBuffer() since they are updated more frequently than once
   *  per rendering pass.
   */
  
  _dxfRedrawInteractorEchos(INTERACTOR_DATA) ;

  EXIT((""));
  return 0;
}

int
_dxfEndSWRenderPass (WinP win, Field image)
{
  DEFWINDATA(win) ;

  ENTRY(("_dxfEndSWRenderPass(0x%x, 0x%x)",win,image));

  if (image)
    DXDelete((Pointer)image);

  _dxfRedrawInteractorEchos(INTERACTOR_DATA) ;

  EXIT((""));
  return 0;
}

#if !defined(DX_NATIVE_WINDOWS)

extern void _dxfSetXError (Display *dpy, XErrorEvent *event);

static int
_HandleExitErrors (Display *dpy, XErrorEvent *rep)
{
  ENTRY(("_HandleExitErrors (0x%x, 0x%x)",dpy, rep));

  if (rep->error_code == BadWindow)
    {
      EXIT(("ERROR: BadWindow error ignored"));
      return 1 ;
    }

  if((rep->error_code == 8) && (rep->request_code == 2))
  {
     EXIT(("ERROR: BadMatch error ignored\n"));
     return(1);
  }

  _dxfSetXError (dpy, rep) ;

  EXIT((""));
  return 1;
}
#endif

Error
_dxfEndRenderModule (tdmChildGlobalP globals)
{
  DEFGLOBALDATA(globals) ;
  DEFPORT(PORT_HANDLE) ;

  /*
   *  This function is called whenever the global variable cache entry is
   *  deleted.
   */

  DEBUG_MARKER("_dxfEndRenderModule ENTRY");
  ENTRY(("_dxfEndRenderModule(0x%x)",globals));

#if !defined(DX_NATIVE_WINDOWS)
  /* the window is sometimes already destroyed before calling gexit() */
  XSetErrorHandler(_HandleExitErrors) ;
#endif

  if (globals->cacheId)
    {
      tdmFree(globals->cacheId) ;
      globals->cacheId = NULL ;
    }
      
  _dxfDestroyAllInteractors(INTERACTOR_DATA) ;
  
  if (OBJECT)
    {
      DXDelete(OBJECT) ;
      OBJECT = NULL ;
    }

  if (GATHER)
    {
      DXDelete(GATHER);
      GATHER = NULL;
    }

  if (CAMERA)
    {
      DXDelete(CAMERA) ;
      CAMERA = NULL ;
    }

  if (SW_BUF)
    {
      tdmFree(SW_BUF);
      SW_BUF = NULL;
    }
  if (MATRIX_STACK)
    {
      _dxfFreeStack(MATRIX_STACK) ;
      MATRIX_STACK = NULL ;
    }

  /* delete graphics API's data structures */
   _dxf_DESTROY_WINDOW(LWIN) ;

#if defined(DX_NATIVE_WINDOWS)
  _dxfSendClientMessage (PARENT_WINDOW, GLShutdown, 0) ;
#else
  _dxfSendClientMessage (DPY, PARENT_WINDOW, GLShutdown, 0) ;

  /* delete callback for display's connection */
  PRINT(("deleting callback for display connection %d",
	 ConnectionNumber(DPY)));

  DXRegisterInputHandler((Handler) NULL, ConnectionNumber(DPY), NULL) ;
#endif
    
  if (PORT_HANDLE)
    {
      _dxfDeletePortHandle(PORT_HANDLE);
    }

  if (MESHHASH)
    _dxf_DeleteObjectHash(MESHHASH);

  if (TEXTUREHASH)
    _dxf_DeleteObjectHash(TEXTUREHASH);

  /* goodbye render instance */
  tdmFree(globals) ; 

  tdmCheckAllocTable(3);

  EXIT(("OK"));
  DEBUG_MARKER("_dxfEndRenderModule EXIT");
  return OK ;
}

#undef tdmInitScreen_c


