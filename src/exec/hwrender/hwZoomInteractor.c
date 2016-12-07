/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwZoomInteractor.c,v $
  Author: Mark Hood
    
  This file contains the implementation of the tdm Zoom interactor.

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>

#include "hwDeclarations.h"
#include "hwMatrix.h"
#include "hwInteractorEcho.h"
#include "hwZoomInteractor.h"

#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

/*
 *  Forward references
 */

static void StartZoomStroke (tdmInteractor, int, int, int, int) ;
static void StartPanZoomStroke (tdmInteractor, int, int, int, int) ;
static void StartStroke (tdmInteractor, int, int, int, int) ;
static void StrokePoint (tdmInteractor, int, int, int, int) ;
static void EndZoomStroke (tdmInteractor, tdmInteractorReturnP) ;
static void EndPanZoomStroke (tdmInteractor, tdmInteractorReturnP) ;

static void zoomConfig (tdmInteractor I) ;
static void restoreConfig (tdmInteractor I) ;

/*
 *  null functions
 */

static void
NullDoubleClick (tdmInteractor I, int x, int y, tdmInteractorReturn *R)
{
  R->change = 0 ;
}

static void
NullResumeEcho (tdmInteractor I, tdmInteractorRedrawMode redraw_mode)
{
}

/*
 *  Creation functions
 */

tdmInteractor
_dxfCreateZoomInteractor (tdmInteractorWin W)
{
  /*
   *  Initialize and return a handle to a zoom interactor.
   */

  register tdmInteractor I ;

  ENTRY(("_dxfCreateZoomInteractor(0x%x)", W));
  
  if (W && (I = _dxfAllocateInteractor(W, sizeof(tdmZoomData))))
    {
      DEFDATA(I,tdmZoomData) ;

      /* instance interactor object methods */
      FUNC(I, StartStroke) = StartZoomStroke ;
      FUNC(I, StrokePoint) = StrokePoint ;
      FUNC(I, DoubleClick) = NullDoubleClick ;
      FUNC(I, EndStroke) = EndZoomStroke ;
      FUNC(I, ResumeEcho) = NullResumeEcho ;
      FUNC(I, Destroy) = _dxfDeallocateInteractor ;
      
      EXIT(("I = 0x%x",I));
      return I ;
    }
  else
      EXIT(("ERROR"));
      return 0 ;
}

tdmInteractor
_dxfCreatePanZoomInteractor (tdmInteractorWin W)
{
  /*
   *  Initialize and return a handle to a pan zoom interactor.
   */

  register tdmInteractor I ;

  ENTRY(("_dxfCreatePanZoomInteractor(0x%x)", W));

  if (W && (I = _dxfAllocateInteractor(W, sizeof(tdmZoomData))))
    {
      DEFDATA(I, tdmZoomData) ;

      /* instance interactor object methods */
      FUNC(I, StartStroke) = StartPanZoomStroke ;
      FUNC(I, StrokePoint) = StrokePoint ;
      FUNC(I, DoubleClick) = NullDoubleClick ;
      FUNC(I, EndStroke) = EndPanZoomStroke ;
      FUNC(I, ResumeEcho) = NullResumeEcho ;
      FUNC(I, Destroy) = _dxfDeallocateInteractor ;
      FUNC(I, KeyStruck) = _dxfNullKeyStruck;

      
      EXIT(("I = 0x%x",I));
      return I ;
    }
  else
      EXIT(("ERROR"));
      return 0 ;
}


/*
 *  method implementations
 */

static void
StartZoomStroke (tdmInteractor I, int x, int y, int btn, int s) 
{
  DEFDATA(I, tdmZoomData) ;

  ENTRY(("StartZoomStroke(0x%x, %d, %d, %d)", I, x, y, btn));

  PDATA(btn) = btn ;
  if (btn == 2) {
    EXIT(("btn == 2"));
    return ;
  }

  zoomConfig(I) ;

  /* zoom center is middle of the window */
  PDATA(cx) = CDATA(w) / 2 ;
  PDATA(cy) = CDATA(h) / 2 ;

  StartStroke (I, x, y, btn, s) ;

  EXIT((""));
}


static void
StartPanZoomStroke (tdmInteractor I, int x, int y, int btn, int s) 
{
  DEFDATA(I, tdmZoomData) ;

  ENTRY(("StartPanZoomStroke(0x%x, %d, %d ,%d)", I, x, y, btn));
  
  PDATA(btn) = btn ;
  if (btn == 2) {
    EXIT(("btn == 2"));
    return ;
  }

  zoomConfig(I) ;

  /* zoom center is initial mouse position */
  PDATA(cx) = x ;
  PDATA(cy) = YFLIP(y) ;

  StartStroke (I, x, y, btn, s) ;

  EXIT((""));
}


static void
StartStroke (tdmInteractor I, int x, int y, int btn, int s)
{
  DEFDATA(I, tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int dx, dy, ax, ay ;

  ENTRY(("StartStroke(0x%x, %d, %d, %d)", I, x, y, btn));
  
  PDATA(px) = x ;
  PDATA(py) = y ;

  y = YFLIP(y) ;
  x = x<0 ? 0 : (x>CDATA(w) ? CDATA(w) : x) ;
  y = y<0 ? 0 : (y>CDATA(h) ? CDATA(h) : y) ;

  /* displacement from zoom center */
  dx = x - PDATA(cx) ;
  dy = y - PDATA(cy) ;
  ax = abs(dx) ;
  ay = abs(dy) ;

  /* force zoom box aspect to match window aspect */
  if ((dy >=  dx * CDATA(aspect) && dy >= -dx * CDATA(aspect)) ||
      (dy <= -dx * CDATA(aspect) && dy <=  dx * CDATA(aspect)))
    {
      PDATA(y1) = PDATA(cy) - ay ;
      PDATA(x1) = PDATA(cx) - ay / CDATA(aspect) ;

      PDATA(y2) = PDATA(y1) + 2 * ay ;
      PDATA(x2) = PDATA(x1) + 2 * ay / CDATA(aspect) ;
    }
  else
    {
      PDATA(x1) = PDATA(cx) - ax ;
      PDATA(y1) = PDATA(cy) - ax * CDATA(aspect) ;

      PDATA(x2) = PDATA(x1) + 2 * ax ;
      PDATA(y2) = PDATA(y1) + 2 * ax * CDATA(aspect) ;
    }

  _dxf_DRAW_ZOOMBOX(I, (void *)0, (float (*)[4])identity, 1) ;

  EXIT((""));
}


static void
StrokePoint (tdmInteractor I, int x, int y, int flag, int s)
{
  DEFDATA(I, tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int dx, dy, ax, ay ;

  ENTRY(("StrokePoint(0x%x, %d, %d)", I, x, y));
  
  if ((PDATA(btn) == 2) || (x == PDATA(px) && y == PDATA(py))) {
    EXIT(("btn == 2 or no movement"));
    return ;
  }
      
  PDATA(px) = x ;
  PDATA(py) = y ;
  y = YFLIP(y) ;

  x = x<0 ? 0 : (x>CDATA(w) ? CDATA(w) : x) ;
  y = y<0 ? 0 : (y>CDATA(h) ? CDATA(h) : y) ;

  /* displacement from zoom box center */
  dx = x - PDATA(cx) ;
  dy = y - PDATA(cy) ;
  ax = abs(dx) ;
  ay = abs(dy) ;

  _dxf_DRAW_ZOOMBOX(I, (void *)0, (float (*)[4])identity, 0) ;

  /* force zoom box aspect to equal window aspect */
  if ((dy >=  dx * CDATA(aspect) && dy >= -dx * CDATA(aspect)) ||
      (dy <= -dx * CDATA(aspect) && dy <=  dx * CDATA(aspect)))
    {
      PDATA(y1) = PDATA(cy) - ay ;
      PDATA(x1) = PDATA(cx) - ay / CDATA(aspect) ;

      PDATA(y2) = PDATA(y1) + 2 * ay ;
      PDATA(x2) = PDATA(x1) + 2 * ay / CDATA(aspect) ;
    }
  else
    {
      PDATA(x1) = PDATA(cx) - ax ;
      PDATA(y1) = PDATA(cy) - ax * CDATA(aspect) ;

      PDATA(x2) = PDATA(x1) + 2 * ax ;
      PDATA(y2) = PDATA(y1) + 2 * ax * CDATA(aspect) ;
    }

  _dxf_DRAW_ZOOMBOX(I, (void *)0, (float (*)[4])identity, 1) ;

  EXIT((""));
}


static void
EndZoomStroke (tdmInteractor I, tdmInteractorReturnP M)
{
  DEFDATA(I, tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int boxwidth ;

  ENTRY(("EndZoomStroke(0x%x, 0x%x)", I, M));
  
  /*
   *  Return zoom factor and a rotation/translation matrix.  The matrix is
   *  identity for the Zoom interactor.
   */

  COPYMATRIX(M->matrix, identity) ;
  if (PDATA(btn) == 2)
    {
      M->change = 0 ;
      EXIT(("btn == 2"));
      return ;
    }

  boxwidth = PDATA(x2) - PDATA(x1) ;
  if (boxwidth == 0 || boxwidth == CDATA(w))
    {
      M->change = 0 ;
      PRINT(("no change"));
    }
  else
    {
      if (PDATA(btn) == 1)
	  M->zoom_factor = (float)boxwidth / (float)CDATA(w) ;
      else
	  M->zoom_factor = (float)CDATA(w) / (float)boxwidth ;

      M->to[0] = CDATA(to[0]) ;
      M->to[1] = CDATA(to[1]) ;
      M->to[2] = CDATA(to[2]) ;

      M->from[0] = CDATA(from[0]) ;
      M->from[1] = CDATA(from[1]) ;
      M->from[2] = CDATA(from[2]) ;

      M->up[0] = CDATA(up[0]) ;
      M->up[1] = CDATA(up[1]) ;
      M->up[2] = CDATA(up[2]) ;

      M->change = 1 ;
    }

  /* undraw the zoom box, restore frame buffer configuration */
  _dxf_DRAW_ZOOMBOX(I, (void *)0, (float (*)[4])identity, 0) ;
  restoreConfig(I) ;

  EXIT((""));
}


static void
EndPanZoomStroke (tdmInteractor I, tdmInteractorReturnP M)
{
  DEFDATA(I, tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int boxwidth ;


  /*
   *  Return zoom factor and a matrix.  The matrix is a rotation or
   *  translation in view coordinates relative to the view origin (`from'
   *  point), *not* the final view matrix; therefore, it is returned in
   *  the `matrix' field and not the `view' field.
   */
  ENTRY(("EndPanZoomStroke(0x%x, 0x%x)", I, M));
  
  COPYMATRIX(M->matrix, identity) ;
  if (PDATA(btn) == 2)
    {
      M->change = 0 ;
      EXIT(("btn == 2"));
      return ;
    }

  _dxf_DRAW_ZOOMBOX(I, (void *)0, (float (*)[4])identity, 0) ;

  boxwidth = PDATA(x2) - PDATA(x1) ;
  if (boxwidth == 0 || boxwidth == CDATA(w))
    {
      M->change = 0 ;
      PRINT(("no change"));
    }
  else
    {
      float vcx, vcy, vcz, wcx, wcy, wcz, screenToView ;

      if (PDATA(btn) == 1)
	  M->zoom_factor = (float)boxwidth / (float)CDATA(w) ;
      else
	  M->zoom_factor = (float)CDATA(w) / (float)boxwidth ;

      /* compute scale conversion for screen -> view */
      screenToView = (CDATA(projection)? CDATA(fov): CDATA(width)) / CDATA(w) ;

      /* convert zoom center to view coordinates */
      vcx = (PDATA(cx) - CDATA(w)/2) * screenToView  ;
      vcy = (PDATA(cy) - CDATA(h)/2) * screenToView  ;
      vcz = -1 ;
      
      if (CDATA(projection))
	{
	  /* perspective:  pan pivots view vector to look at zoom center */
	  float rx, ry, c, t ;
	  double s ;

	  /* [vcx,vcy,vcz] = new view vector expressed in VC: normalize it */
	  s = sqrt((double)(vcx*vcx + vcy*vcy + vcz*vcz)) ;
	  vcx /= s ; vcy /= s ; vcz /= s ;

	  /* axis of rotation in view coordinates = [vcx,vcy,vcz] X [0,0,-1] */
	  rx = -vcy ; ry = vcx ; /*rz = 0 ;*/
	  
	  /* length of axis is the sine of the angle to rotate */
	  s = (float) sqrt((double)(rx*rx + ry*ry)) ;
	  
	  /* cosine of angle is [vcx,vcy,vcz].[0,0,-1] */
	  c = -vcz ;
	  
	  /* normalize rotation axis */
	  rx /= s ; ry /= s ;
	  
	  /* construct relative rotation matrix expressing oldVC -> newVC */
	  t = 1.0 - c ;
	  ROTXY (M->matrix, rx, ry, s, c, t) ;

	  /* rotate new view vector into current world coordinates */
	  wcx = vcx * CDATA(viewXfm[0][0]) +
	        vcy * CDATA(viewXfm[0][1]) +
		vcz * CDATA(viewXfm[0][2]) ;
	  wcy = vcx * CDATA(viewXfm[1][0]) +
	        vcy * CDATA(viewXfm[1][1]) +
		vcz * CDATA(viewXfm[1][2]) ;
	  wcz = vcx * CDATA(viewXfm[2][0]) +
	        vcy * CDATA(viewXfm[2][1]) +
		vcz * CDATA(viewXfm[2][2]) ;

	  /* construct new `to' point */
	  M->to[0] = CDATA(from[0]) + CDATA(vdist)*wcx ;
	  M->to[1] = CDATA(from[1]) + CDATA(vdist)*wcy ;
	  M->to[2] = CDATA(from[2]) + CDATA(vdist)*wcz ;

	  /* `from' point is unchanged */
	  M->from[0] = CDATA(from[0]) ;
	  M->from[1] = CDATA(from[1]) ;
	  M->from[2] = CDATA(from[2]) ;

	  /*
	   *  Consider the columns of M->matrix as the basis of the newly
	   *  rotated view coordinate system, expressed in terms of the
	   *  old view coordinate system.  Then rotating the Y column of
	   *  M->matrix into current world coordinates gives the new `up'
	   *  vector.
	   */
	  M->up[0] = M->matrix[0][1] * CDATA(viewXfm[0][0]) +
	             M->matrix[1][1] * CDATA(viewXfm[0][1]) +
		     M->matrix[2][1] * CDATA(viewXfm[0][2]) ;
	  M->up[1] = M->matrix[0][1] * CDATA(viewXfm[1][0]) +
	             M->matrix[1][1] * CDATA(viewXfm[1][1]) +
		     M->matrix[2][1] * CDATA(viewXfm[1][2]) ;
	  M->up[2] = M->matrix[0][1] * CDATA(viewXfm[2][0]) +
	             M->matrix[1][1] * CDATA(viewXfm[2][1]) +
		     M->matrix[2][1] * CDATA(viewXfm[2][2]) ;
	}
      else
	{
	  /* ortho view: pan matrix is a translation in view X and Y only */
	  M->matrix[3][0] = -vcx ;
	  M->matrix[3][1] = -vcy ;
	  
	  /* convert translation into world coordinates */
	  wcx = vcx*CDATA(viewXfm[0][0]) + vcy*CDATA(viewXfm[0][1]) ;
	  wcy = vcx*CDATA(viewXfm[1][0]) + vcy*CDATA(viewXfm[1][1]) ;
	  wcz = vcx*CDATA(viewXfm[2][0]) + vcy*CDATA(viewXfm[2][1]) ;

	  /* translate `to' and `from' points */
	  M->to[0] = CDATA(to[0]) + wcx ;
	  M->to[1] = CDATA(to[1]) + wcy ;
	  M->to[2] = CDATA(to[2]) + wcz ;

	  M->from[0] = CDATA(from[0]) + wcx ;
	  M->from[1] = CDATA(from[1]) + wcy ;
	  M->from[2] = CDATA(from[2]) + wcz ;

	  /* `up' remains the same */
	  M->up[0] = CDATA(up[0]) ;
	  M->up[1] = CDATA(up[1]) ;
	  M->up[2] = CDATA(up[2]) ;
	}
      PRINT(("zoom factor %f", M->zoom_factor));
      PRINT(("look-to point"));
      VPRINT(M->to) ;
      PRINT(("up vector"));
      VPRINT(M->up) ;
      PRINT(("look-from point"));
      VPRINT(M->from) ;
      PRINT(("matrix"));
      MPRINT(M->matrix) ;
      M->change = 1 ;
    }

  restoreConfig(I) ;
  EXIT((""));
}

/*
 *  Configuration functions
 */

static void 
zoomConfig (tdmInteractor I)
{
  DEFDATA(I, tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("zoomConfig(0x%x)", I));

  /* get origin of window and dimensions of screen */
  _dxf_GET_WINDOW_ORIGIN (PORT_CTX, &CDATA(ox), &CDATA(oy)) ;
  _dxf_GET_MAXSCREEN (PORT_CTX, &CDATA(xmaxscreen), &CDATA(ymaxscreen)) ;

  /* save viewport and get viewing matrix */
  _dxfPushViewport(CDATA(stack)) ;
  _dxfGetViewMatrix(CDATA(stack), CDATA(viewXfm)) ;
  
  /* push the SW modeling matrix and HW modeling/viewing matrix */
  _dxfPushMatrix(CDATA(stack)) ;
  _dxf_PUSH_REPLACE_MATRIX (PORT_CTX, (float (*)[4])identity) ;
  
  /* set up one-to-one mapping of world to screen */
  _dxf_SET_WORLD_SCREEN (PORT_CTX, CDATA(w), CDATA(h)) ;

  /* disable Z buffer comparisons */
  if ((CDATA(zbuffer) = _dxf_GET_ZBUFFER_STATUS(PORT_CTX)))
      _dxf_SET_ZBUFFER_STATUS (PORT_CTX, 0) ;

  /* set up frame buffer */
  _dxf_BUFFER_CONFIG (PORT_CTX, CDATA(image),
		     0, CDATA(h)-CDATA(ih), CDATA(iw)-1, CDATA(h)-1,
		     &PDATA(displaymode), &PDATA(buffermode),
		     tdmFrontBufferDraw) ;

  EXIT((""));
}

static void
restoreConfig (tdmInteractor I)
{
  int left, right, bottom, top ;
  DEFDATA(I, tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("restoreConfig(0x%x)", I));

  /* restore viewport */
  _dxfPopViewport(CDATA(stack)) ;
  _dxfGetViewport(CDATA(stack), &left, &right, &bottom, &top) ;
  _dxf_SET_VIEWPORT(PORT_CTX, left, right, bottom, top) ;

  /* restore projection matrix */
  if (CDATA(projection))
      _dxf_SET_PERSPECTIVE_PROJECTION (PORT_CTX, CDATA(fov), CDATA(aspect),
				      CDATA(Near), CDATA(Far)) ;
  else
      _dxf_SET_ORTHO_PROJECTION (PORT_CTX, CDATA(width), CDATA(aspect),
				CDATA(Near), CDATA(Far)) ;
  
  /* restore model/view matrix */
  _dxfPopMatrix(CDATA(stack)) ;
  _dxf_POP_MATRIX(PORT_CTX) ;

  /* restore zbuffer usage */
  if (CDATA(zbuffer))
      _dxf_SET_ZBUFFER_STATUS (PORT_CTX, 1) ;

  /* restore frame buffer configuration */
  _dxf_BUFFER_RESTORE_CONFIG
      (PORT_CTX, PDATA(displaymode), PDATA(buffermode), tdmFrontBufferDraw) ;

  EXIT((""));
}
