/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwRotateInteractor.c,v $
  Author: Mark Hood

  This file contains the implementation of the TDM rotation interactor.  

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>
#include "hwDeclarations.h"
#include "hwRotateInteractor.h"
#include "hwInteractorEcho.h"
#include "hwMatrix.h"
 
#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

/*
 *  Forward references
 */

//static void DoubleClick (tdmInteractor, int, int, tdmInteractorReturn *) ;
static void StartStroke (tdmInteractor, int, int, int, int) ;
static void StartViewStroke (tdmInteractor, int, int, int, int) ;
static void StartRotateStroke (tdmInteractor, int, int, int, int) ;
static void EndStroke (tdmInteractor, tdmInteractorReturnP) ;
static void ResumeEcho (tdmInteractor, tdmInteractorRedrawMode) ;
static void Destroy (tdmInteractor) ;

static void InitTwirlStroke (tdmInteractor, float, float) ;
static void Twirl (tdmInteractor, int, int, int, int) ;
static void InitRollStroke (tdmInteractor, int, int) ;
static void ViewRoll (tdmInteractor, int, int, int, int) ;
static void PlaneRollWithEcho (tdmInteractor, int, int, int, int) ;
static int PlaneRoll (tdmInteractor I, int x, int y, float rot[4][4]) ;

static void globeConfig (tdmInteractor,
			 int *, int *, tdmInteractorRedrawMode) ;
static void gnomonConfig (tdmInteractor,
			  int *, int *, tdmInteractorRedrawMode) ;
static void viewerConfig (tdmInteractor,
			  int *, int *, tdmInteractorRedrawMode) ;
static void RestoreConfig (tdmInteractor,
			   int, int, tdmInteractorRedrawMode) ;

static void extractViewRot (float M[4][4]) ;
static void orbitFrom (float R[4][4], float current[4][4],
		       float to[3], float vdist) ;

/*
 *  Null functions
 */

static void NullFunction()
{
}

/*
static void
NullStartStroke (tdmInteractor I, int x, int y, int btn, int s)
{
}
*/

static void
NullDoubleClick (tdmInteractor I, int x, int y, tdmInteractorReturn *R)
{
  R->change = 0 ;
}

/*
static void
NullEndStroke (tdmInteractor I, tdmInteractorReturn *R)
{
  R->change = 0 ;
}
*/

static void
NullResumeEcho (tdmInteractor I, tdmInteractorRedrawMode redrawmode)
{
}

static void
NullEchoFunc (tdmInteractor I, void *u, float m[4][4], int f)
{
}

/*
 *  Creation functions
 */

tdmInteractor
_dxfCreateRotationInteractor(tdmInteractorWin W,
			     tdmEchoType E, tdmRotateModel M)
{
  register tdmInteractor I ;

  ENTRY(("_dxfCreateRotationInteractor(0x%x, 0x%x, 0x%x)",
	 W, E, M));

  if (W && (I = _dxfAllocateInteractor (W, sizeof(tdmRotateData)))) {
    DEFDATA(I,tdmRotateData) ;
    DEFPORT(I_PORT_HANDLE) ;
    int echoRadius ;
    
    /* instance initial interactor methods */
    FUNC(I, DoubleClick) = NullDoubleClick ;
    FUNC(I, StartStroke) = StartRotateStroke ;
    FUNC(I, EndStroke) = EndStroke ;
    FUNC(I, ResumeEcho) = ResumeEcho ;
    FUNC(I, Destroy) = Destroy ;
    FUNC(I, restoreConfig) = RestoreConfig ;
    
    switch (M)
      {
      case tdmZTwirl:
	FUNC(I, StrokePoint) = Twirl ;
	break ;
      case tdmXYPlaneRoll:
      default:
	FUNC(I, StrokePoint) = PlaneRollWithEcho ;
	break ;
      }
    
    switch (E)
      {
      case tdmGlobeEcho:
	FUNC(I, EchoFunc) = EFUNCS(DrawGlobe) ;
	FUNC(I, Config) = globeConfig ;
	echoRadius = GLOBERADIUS ;
	
	/* globe is LL corner: window has no effect on size or placement */
	PDATA(vllx) = PDATA(vlly) = GLOBEOFFSET ;
	PDATA(vurx) = PDATA(vury) = GLOBEOFFSET + 2*GLOBERADIUS ;
	
	PDATA(foreground) = (void *) 1 ;
	PDATA(background) = (void *) 1 ;
	break ;
	
      case tdmGnomonEcho:
	FUNC(I, EchoFunc) = EFUNCS(DrawGnomon) ;
	FUNC(I, Config) = gnomonConfig ;
	echoRadius = GNOMONRADIUS ;
	
	PDATA(foreground) = (void *) 1 ;
	PDATA(background) = (void *) 1 ;
	break ;
	
      default:
      case tdmNullEcho:
	FUNC(I, EchoFunc) = NullEchoFunc ;
	FUNC(I, Config) = gnomonConfig ;
	echoRadius = 0 ;
	
	PDATA(foreground) = (void *) 0 ;
	PDATA(background) = (void *) 0 ;
      }

    FUNC(I, KeyStruck) = _dxfNullKeyStruck;

    
    /* allocate background save-under */
    if (PDATA(background))
      PDATA(background) =
	_dxf_ALLOCATE_PIXEL_ARRAY
	  (PORT_CTX, (2*echoRadius +1), (2*echoRadius +1)) ;

    /* allocate foreground + background image for quick restoration */
    if (PDATA(foreground))
      PDATA(foreground) =
	_dxf_ALLOCATE_PIXEL_ARRAY
	  (PORT_CTX, (2*echoRadius +1), (2*echoRadius +1)) ;

    /* initial draw is into both buffers */
    PDATA(redrawmode) = tdmBothBufferDraw ;
    
    /* initially invisible, no font defined */
    PDATA(visible) = 0 ;
    PDATA(font) = -1 ;
    
    EXIT(("I = 0x%x",I));
    return I ;
  }

  EXIT(("ERROR"));
  return 0 ;
}


tdmInteractor
_dxfCreateViewRotationInteractor (tdmInteractorWin W,
				  tdmViewEchoFunc E, tdmRotateModel M,
				  void *udata)
{
  /*
   *  Initialize and return a handle to a view rotation interactor.
   *  The echo function is supplied by the application.
   */

  register tdmInteractor I ;

  ENTRY(("_dxfCreateViewRotationInteractor(0x%x, 0x%x, 0x%x, 0x%x)",
	 W, E, M, udata));

  if (W && (I = _dxfAllocateInteractor (W, sizeof(tdmRotateData))))
    {
      DEFDATA(I,tdmRotateData) ;

      /* instance initial interactor methods */
      FUNC(I, StartStroke) = StartViewStroke ;
      FUNC(I, DoubleClick) = NullDoubleClick ;
      FUNC(I, EndStroke) = EndStroke ;
      FUNC(I, ResumeEcho) = NullResumeEcho ;
      FUNC(I, EchoFunc) = E ;
      FUNC(I, Destroy) = _dxfDeallocateInteractor ;
      FUNC(I, Config) = viewerConfig ;
      FUNC(I, restoreConfig) = NullFunction ;
      
      switch (M)
	{
	case tdmXYPlaneRoll:
	  FUNC(I, StrokePoint) = ViewRoll ;
	  break ;
	case tdmZTwirl:
	default:
	  FUNC(I, StrokePoint) = Twirl ;
	  break ;
	}

      /* copy pointer to user data */
      UDATA(I) = udata ;

      EXIT((""));
      return I ;
    }
  else
      EXIT(("ERROR"));
      return 0 ;
}

void
_dxfRotateInteractorVisible (tdmInteractor I)
{
  ENTRY(("_dxfRotateInteractorVisible(0x%x)", I));

  if (I)
    {
      DEFDATA(I,tdmRotateData) ;
      PDATA(visible) = 1 ;
    }

  EXIT((""));
}

void 
_dxfRotateInteractorInvisible (tdmInteractor I)
{
  ENTRY(("_dxfRotateInteractorInvisible(0x%x)", I));

  if (I)
    {
      DEFDATA(I,tdmRotateData) ;
      PDATA(visible) = 0 ;
    }

  EXIT((""));
}

/*
 *  Method implementations
 */

static void 
StartRotateStroke (tdmInteractor I, int x, int y, int btn, int s)
{
  DEFDATA(I,tdmRotateData) ;

  ENTRY(("StartRotateStroke(0x%x, %d, %d, %d)", I, x, y, btn));

  /* set up hardware, stay in that configuration until EndStroke */
  CALLFUNC(I,Config) (I, &PDATA(displaymode), &PDATA(buffermode),
		      tdmBackBufferDraw) ;

  /* initialize start matrix to current view rotation */
  COPYMATRIX (PDATA(strXfm), CDATA(viewXfm)) ;
  extractViewRot(PDATA(strXfm)) ;
  StartStroke (I, x, y, btn, s) ;

  EXIT((""));
}


static void 
StartViewStroke (tdmInteractor I, int x, int y, int btn, int s)
{
  DEFDATA(I,tdmRotateData) ;

  ENTRY(("StartViewStroke(0x%x, %d, %d, %d)", I, x, y, btn));

  /* set up hardware, stay in that configuration until EndStroke */
  CALLFUNC(I,Config) (I, &PDATA(displaymode), &PDATA(buffermode),
		      tdmBackBufferDraw) ;

  /* initialize start matrix to current view matrix */
  COPYMATRIX (PDATA(strXfm), CDATA(viewXfm)) ;
  StartStroke (I, x, y, btn, s) ;

  EXIT((""));
}


static void 
StartStroke (tdmInteractor I, int x, int y, int btn, int s)
{
  DEFDATA(I,tdmRotateData) ;
  register float dx, dy ;

  ENTRY(("StartStroke(0x%x, %d, %d, %d)", I, x, y, btn));

  /* transform (x,y) to echo coordinate system */
  y = YFLIP(y) ;

  CDATA(xlast) = x ;
  CDATA(ylast) = y ;

  dx = x - PDATA(gx) ;
  dy = y - PDATA(gy) ;

  /* initialize incremental matrix */
  COPYMATRIX (PDATA(incXfm), identity) ;

  /* initialize appropriate stroke processor */
  if (FUNC(I, StrokePoint) == Twirl)
      InitTwirlStroke (I, dx, dy) ;
  else
      InitRollStroke (I, x, y) ;

  EXIT((""));
}


static void 
InitTwirlStroke (tdmInteractor I, float dx, float dy)
{
  /*
   *  Initialize twirling (rotation about Z axis).
   */
  double a ;
  DEFDATA(I,tdmRotateData) ;

  ENTRY(("InitTwirlStroke(0x%x, %f, %f)", I, dx, dy));

  /* compute basis vectors of rotated coordinate system */
  a = sqrt ((double)dx * (double)dx + (double)dy * (double)dy) ;
  dx /= a ;
  dy /= a ;
  
  PDATA(xbasis)[0] =  dx ;
  PDATA(xbasis)[1] =  dy ;
  PDATA(ybasis)[0] = -dy ;
  PDATA(ybasis)[1] =  dx ;

  /* initialize current matrix */
  COPYMATRIX (PDATA(newXfm), identity) ;

  EXIT((""));
}


static void 
InitRollStroke (tdmInteractor I, int x, int y)
{
  /*
   *  Initialize rolling (XY rotation) mode.
   */
  DEFDATA(I,tdmRotateData) ;

  ENTRY(("InitRollStroke(0x%x, %d, %d)", I, x, y));

  /* initialize current matrix */
  COPYMATRIX (PDATA(newXfm), identity) ;

  EXIT((""));
}


static void 
EndStroke (tdmInteractor I, tdmInteractorReturnP R)
{
  /*
   *  End stroke and return appropriate info.
   */
  DEFDATA(I,tdmRotateData) ;

  ENTRY(("EndStroke(0x%x, 0x%x)", I, R));

  if (PDATA(rotation_update))
    {
      /* return relative rotation transform */
      MULTMATRIX (R->matrix, PDATA(incXfm), PDATA(newXfm)) ;

      if (CDATA(view_coords_set))
	{
	  /* return `to', `from', `up', and view matrix expressing them */
	  PRINT(("RotEndStroke: current viewXfm:"));
	  MPRINT(CDATA(viewXfm)) ;
	  
	  _dxfRenormalizeView(CDATA(viewXfm)) ;
	  PRINT(("RotEndStroke: renormalized viewXfm:"));
	  MPRINT(CDATA(viewXfm)) ;
	  
	  R->from[0] = CDATA(to[0]) + CDATA(vdist)*CDATA(viewXfm[0][2]) ;
	  R->from[1] = CDATA(to[1]) + CDATA(vdist)*CDATA(viewXfm[1][2]) ;
	  R->from[2] = CDATA(to[2]) + CDATA(vdist)*CDATA(viewXfm[2][2]) ;

	  R->to[0] = CDATA(to[0]) ;
	  R->to[1] = CDATA(to[1]) ;
	  R->to[2] = CDATA(to[2]) ;
	  
	  R->up[0] = CDATA(viewXfm[0][1]) ;
	  R->up[1] = CDATA(viewXfm[1][1]) ;
	  R->up[2] = CDATA(viewXfm[2][1]) ;

	  R->dist = CDATA(vdist) ;

	  if (FUNC(I, StrokePoint) == ViewRoll)
	    {
	      /* return view matrix directly */
	      COPYMATRIX(R->view, CDATA(viewXfm)) ;
	    }
	  else
	    {
	      /* compute matrix that orbits `from' point about `to' point */
	      orbitFrom (R->matrix, PDATA(strXfm), CDATA(to), CDATA(vdist)) ;
	      MULTMATRIX(R->view, PDATA(strXfm), R->matrix) ;
	    }
	}
      R->reason = tdmROTATION_UPDATE ;
      R->change = 1 ;
    }
  else
      /* no change */
      R->change = 0 ;

  PDATA(rotation_update) = 0 ;
  CALLFUNC(I,restoreConfig) (I, PDATA(displaymode), PDATA(buffermode),
			     tdmBackBufferDraw) ;

  EXIT((""));
}


static void 
Destroy (tdmInteractor I)
{
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("Destroy(0x%x)", I));

  if (PDATA(background))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(background)) ;

  if (PDATA(foreground))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(foreground)) ;

  _dxfDeallocateInteractor(I) ;

  EXIT((""));
}


/*
 *  Z rotation model
 */

static void
Twirl (tdmInteractor I, int x, int y, int flag, int state)
{
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  double a ;
  float c, s, dx, dy, tmp[4][4] ;

  ENTRY(("Twirl(0x%x, %d, %d)", I, x, y));

  y = YFLIP(y) ;

  if (CDATA(xlast) == x && CDATA(ylast) == y)
    {
      EXIT(("xlast and ylast the same"));
      return ;
    }
  else
    {
      CDATA(xlast) = x ;
      CDATA(ylast) = y ;
    }

  dx = x - PDATA(gx) ;
  dy = y - PDATA(gy) ;

  a = sqrt ((double)dx * (double)dx + (double)dy * (double)dy) ;
  if (a < 1.0) {
    EXIT(("a < 1.0"));
    return ;
  }

  /*
   *  Compute sine and cosine of rotation matrix relative to the first point
   *  of this stroke.
   */

  s = (PDATA(ybasis)[0]*dx + PDATA(ybasis)[1]*dy) / a ;
  c = (PDATA(xbasis)[0]*dx + PDATA(xbasis)[1]*dy) / a ;

  PDATA(newXfm)[0][0] =  c ;
  PDATA(newXfm)[0][1] =  s ;
  PDATA(newXfm)[0][2] =  0 ;
  PDATA(newXfm)[0][3] =  0 ;

  PDATA(newXfm)[1][0] = -s ;
  PDATA(newXfm)[1][1] =  c ;
  PDATA(newXfm)[1][2] =  0 ;
  PDATA(newXfm)[1][3] =  0 ;

  PDATA(newXfm)[2][0] =  0 ;
  PDATA(newXfm)[2][1] =  0 ;
  PDATA(newXfm)[2][2] =  1 ;
  PDATA(newXfm)[2][3] =  0 ;

  PDATA(newXfm)[3][0] =  0 ;
  PDATA(newXfm)[3][1] =  0 ;
  PDATA(newXfm)[3][2] =  0 ;
  PDATA(newXfm)[3][3] =  1 ;
  
  MULTMATRIX (tmp, PDATA(incXfm), PDATA(newXfm)) ;
  MULTMATRIX (CDATA(viewXfm), PDATA(strXfm), tmp) ;
  _dxf_LOAD_MATRIX (PORT_CTX, CDATA(viewXfm)) ;
  CDATA(view_state)++ ;

  /* update software matrix stack shadow */
  _dxfLoadViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  
  /* call echo function */
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 0) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1) ;

  /* redraw any auxiliary echos */
  tdmResumeEcho (AUX(I), tdmAuxEchoMode) ;

  PDATA(rotation_update) = 1 ;

  EXIT((""));
}


/*
 *  XY rotation models
 */

static int
PlaneRoll (tdmInteractor I, int x, int y, float rot[4][4])
{
  /*
   *  Pointer is attached to an infinite plane parallel to XY, which rolls
   *  over a virtual sphere.  If we bail out, return 0.
   */

  DEFDATA(I,tdmRotateData) ;
  register float dx, dy, t, s, c ;
  float tmp[4][4] ;
  double a ;

  ENTRY(("PlaneRoll(0x%x, %d, %d, 0x%x)", I, x, y, rot));

  y = YFLIP(y) ;

  dx = CDATA(xlast) - x ;
  dy = CDATA(ylast) - y ;

  if (dx == 0 && dy == 0)
    {
      EXIT(("no change"));
      return 0 ;
    }
  else
    {
      CDATA(xlast) = x ;
      CDATA(ylast) = y ;
    }

  /* rotate delta vector by pi/2, use as axis of rotation */
  t = dy ;
  dy = -dx ;
  dx = t ;

  /* normalize */
  if ((a = sqrt ((double)dx * (double)dx + (double)dy * (double)dy)) < 1.0) {
    EXIT(("a < 1.0"));
    return 0 ;
  }

  dx /= a ;
  dy /= a ;

  /* convert pixels to radians */
  a *= 1.0/(float)PDATA(gradius) ;

  /* compute the matrix which rotates about the vector [dx dy 0] */
  s = (float) sin(a) ;
  c = (float) cos(a) ;
  t = 1.0 - c ;
  ROTXY (rot, dx, dy, s, c, t) ;

  /* accumulate new relative and incremental rotation matrices */
  MULTMATRIX (tmp, PDATA(newXfm), rot) ;
  COPYMATRIX (PDATA(newXfm), tmp) ;
  MULTMATRIX (rot, PDATA(incXfm), PDATA(newXfm)) ;

  EXIT(("1"));
  return 1 ;
}


static void
PlaneRollWithEcho (tdmInteractor I, int x, int y, int flag, int s)
{
  float rot[4][4] ;
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("PlaneRollWithEcho(0x%x, %d, %d)", I, x, y));
  
  /* apply plane rolling model */
  if (!PlaneRoll (I, x, y, rot))
    {
      EXIT(("PlaneRoll returns 0"));
      return ;
    }

  /* accumulate start transform */
  MULTMATRIX (CDATA(viewXfm), PDATA(strXfm), rot) ;

  /* load new view matrix, erase old echo, draw new echo */
  _dxf_LOAD_MATRIX (PORT_CTX, CDATA(viewXfm)) ;
  _dxfLoadViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  CDATA(view_state)++ ;
  
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 0) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1) ;

  /* redraw any auxiliary echo */
  tdmResumeEcho (AUX(I), tdmAuxEchoMode) ;

  PDATA(rotation_update) = 1 ;

  EXIT((""));
}


static void
ViewRoll (tdmInteractor I, int x, int y, int flag, int s)
{
  float rot[4][4], from[3], zaxis[3], Near, Far ;
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("ViewRoll(0x%x, %d, %d)", I, x, y));

  /* apply plane rotation model */
  if (!PlaneRoll (I, x, y, rot))
    {
      EXIT(("PlaneRoll returns 0"));
      return ;
    }

  if (CDATA(view_coords_set))
      /* orbit the `from' point about the `to' point */
      orbitFrom(rot, PDATA(strXfm), CDATA(to), CDATA(vdist)) ;

  /* rotate the view */
  MULTMATRIX (CDATA(viewXfm), PDATA(strXfm), rot) ;

  /* get the new clip planes */
  GET_VC_ORIGIN(CDATA(viewXfm), from) ;
  GET_VIEW_DIRECTION(CDATA(viewXfm), zaxis) ;
  _dxfGetNearFar(CDATA(projection), CDATA(w),
		CDATA(projection)? CDATA(fov): CDATA(width),
		from, zaxis, CDATA(box), &Near, &Far);

  /* setting new clip planes requires creating a new projection matrix */
  if (CDATA(projection))
    {
      _dxfSetProjectionInfo(CDATA(stack), 1, CDATA(fov), CDATA(aspect),
			   Near, Far);
      _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, CDATA(fov), CDATA(aspect),
				     Near, Far);
    }
  else
    {
      _dxfSetProjectionInfo(CDATA(stack), 0, CDATA(width), CDATA(aspect),
			   Near, Far);
      _dxf_SET_ORTHO_PROJECTION(PORT_CTX, CDATA(width), CDATA(aspect),
			       Near, Far); 
    }

  /* load the new view matrix */
  _dxf_LOAD_MATRIX (PORT_CTX, CDATA(viewXfm)) ;
  _dxfLoadViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  CDATA(view_state)++ ;

  /* erase and draw the echo */
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 0) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1) ;

  PDATA(rotation_update) = 1 ;

  EXIT((""));
}


static void
orbitFrom (float R[4][4], float current[4][4], float to[3], float vdist)
{
  /*
   *  To rotate the view about the `to' point, we in effect rotate the
   *  `from' point, the origin of the current view coordinate system.
   *  This keeps the `to' point centered in the view.
   *
   *  The columns of `R' are the basis vectors of the new view rotation
   *  coordinate system, expressed in terms of the current view rotation
   *  coordinate system (ie, `R' is a relative as opposed to an absolute
   *  rotation).  `vdist' is the distance between the `from' and `to'
   *  points, so the new `from-to' vector (expressed in current view
   *  rotation coordinates) is the 3rd column of `R' scaled by `vdist'.
   *
   *  The `to' point is then rotated by the current view rotation matrix
   *  and then added to the `from-to' vector.  The negative of the result
   *  replaces the 4th (translation) row of the current view matrix.  This
   *  is the same matrix that would be obtained by rotating the `from-to'
   *  vector into world coordinates, adding the `to' point in world
   *  coordinates, expressing the result as a translation matrix, and
   *  pre-concatenating it with current view rotation.
   */

  float fromVectVC[3], toVC[3] ;
  
  ENTRY(("orbitFrom(0x%x, 0x%x, 0x%x, %f)",
	 R, current, to, vdist));
  
  fromVectVC[0] = R[0][2] * vdist ;
  fromVectVC[1] = R[1][2] * vdist ;
  fromVectVC[2] = R[2][2] * vdist ;
  
  toVC[0] = to[0] * current[0][0] +
            to[1] * current[1][0] +
	    to[2] * current[2][0] ;
  toVC[1] = to[0] * current[0][1] +
            to[1] * current[1][1] +
	    to[2] * current[2][1] ;
  toVC[2] = to[0] * current[0][2] +
            to[1] * current[1][2] +
	    to[2] * current[2][2] ;
  
  current[3][0] = -(fromVectVC[0] + toVC[0]) ;
  current[3][1] = -(fromVectVC[1] + toVC[1]) ;
  current[3][2] = -(fromVectVC[2] + toVC[2]) ;

  EXIT((""));
}

  
/*
 *  Configuration and drawing routines 
 */

static void
extractViewRot (float M[4][4])
{
  /*
   *  DXExtract view rotation from M.
   */
  ENTRY(("extractViewRot(0x%x)", M));
  
  M[0][3] =  0 ; M[1][3] =  0 ; M[2][3] =  0 ; 
  M[3][0] =  0 ; M[3][1] =  0 ; M[3][2] =  0 ; M[3][3] =  1 ;

  EXIT((""));
}


static void
ResumeEcho (tdmInteractor I, tdmInteractorRedrawMode redrawmode)
{
  /*
   *  Redraw interactor I
   */
  
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("ResumeEcho(0x%x, 0x%x)", I, redrawmode));

  if (!PDATA(visible))
    {
      /* invisible: draw nothing */
      EXIT(("invisible"));
      return ;
    }

  if (FUNC(I, EchoFunc) == NullEchoFunc)
    {
      PRINT(("null echo function, drawing aux echos"));
      tdmResumeEcho (AUX(I), redrawmode) ;
      EXIT((""));
      return ;
    }
  
  if (PDATA(foreground) &&
      redrawmode == tdmAuxEchoMode &&
      PDATA(foregroundView) == CDATA(view_state))
    {
      /*
       *  Quick restoration.  This isn't just an optimization: the cursor
       *  interactor uses the gnomon as an auxiliary echo, and the GL
       *  context must not be altered during cursor interaction mode.
       */
      _dxf_WRITE_BUFFER (PORT_CTX,
			PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
			PDATA(foreground)) ;
      PRINT(("restoration via blit, drawing aux echos"));
      tdmResumeEcho (AUX(I), tdmAuxEchoMode) ;
      EXIT((""));
      return ;
    }
      
  /* configure */
  CALLFUNC(I,Config) (I, &PDATA(displaymode), &PDATA(buffermode), redrawmode) ;

  /* get view rotation */
  COPYMATRIX (PDATA(strXfm), CDATA(viewXfm)) ;
  extractViewRot(PDATA(strXfm)) ;

  if (PDATA(background) &&
      (redrawmode == tdmBothBufferDraw || redrawmode == tdmFrontBufferDraw))
      /* get new image background */
      _dxf_READ_BUFFER (PORT_CTX,
			PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
			PDATA(background)) ;

  /* draw echo on top of background image */
  _dxf_LOAD_MATRIX (PORT_CTX, PDATA(strXfm)) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), PDATA(strXfm), 0) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), PDATA(strXfm), 1) ;

  if (PDATA(foreground) && 
      (redrawmode == tdmBothBufferDraw || redrawmode == tdmFrontBufferDraw))
    {
      /* get new image background + foreground for quick restoration */
      _dxf_READ_BUFFER (PORT_CTX,
		       PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
		       PDATA(foreground)) ;
      PDATA(foregroundView) = CDATA(view_state) ;
    }
  
  /* restore configuration */
  CALLFUNC(I,restoreConfig) (I, PDATA(displaymode), PDATA(buffermode),
			     redrawmode) ;

  PRINT(("echo redrawn, drawing aux echos"));
  if (redrawmode == tdmBothBufferDraw)
    {
      PRINT(("tdmBothBufferDraw -> tdmFrontBufferDraw"));
      tdmResumeEcho (AUX(I), tdmFrontBufferDraw) ;
    }
  else
      tdmResumeEcho (AUX(I), redrawmode) ;

  EXIT((""));
}


static void 
globeConfig (tdmInteractor I,
	     int *CurrentDisplayMode, int *CurrentBufferMode,
	     tdmInteractorRedrawMode redrawMode) 
{
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("globeConfig(0x%x, 0x%x, 0x%x, 0x%x)",
	 I, CurrentDisplayMode, CurrentBufferMode, redrawMode));

  PDATA(redrawmode) = redrawMode ;

  if (redrawMode == tdmAuxEchoMode)
    {
      /* frame buffer configuration tasks already done */
      PRINT(("redraw mode tdmAuxEchoMode, skipping buffer configuration"));
      _dxfPushViewport (CDATA(stack)) ;
      _dxfSetViewport (CDATA(stack), (int) PDATA(vllx), (int) PDATA(vurx),
		      (int) PDATA(vlly), (int) PDATA(vury)) ;
      _dxf_SET_VIEWPORT (PORT_CTX, (int) PDATA(vllx), (int) PDATA(vurx),
			(int) PDATA(vlly), (int) PDATA(vury)) ;

      _dxfPushViewMatrix(CDATA(stack)) ;
      _dxf_SET_ORTHO_PROJECTION (PORT_CTX, 2, 1, 1, -1) ;

      EXIT((""));
      return ;
    }

  /* turn off Z buffer */
  CDATA(zbuffer) = _dxf_GET_ZBUFFER_STATUS(PORT_CTX) ;
  if (CDATA(zbuffer))
      _dxf_SET_ZBUFFER_STATUS (PORT_CTX, 0) ;

  /* configure the frame buffer */
  _dxf_BUFFER_CONFIG (PORT_CTX, CDATA(image),
		     0, CDATA(h)-CDATA(ih), CDATA(iw)-1, CDATA(h)-1, 
		     CurrentDisplayMode, CurrentBufferMode, redrawMode) ;

  /* get origin of window and dimensions of screen */
  _dxf_GET_WINDOW_ORIGIN (PORT_CTX, &CDATA(ox), &CDATA(oy)) ;
  _dxf_GET_MAXSCREEN (PORT_CTX, &CDATA(xmaxscreen), &CDATA(ymaxscreen)) ;

  /* virtual sphere radius is half of smallest window dimension */
  if (CDATA(h) > CDATA(w))
    {
      PDATA(gradius) = CDATA(w)/2 ;
      PDATA(gx) = PDATA(gradius) ;
      PDATA(gy) = PDATA(gradius) + (CDATA(h) - CDATA(w))/2 ;
    }
  else
    {
      PDATA(gradius) = CDATA(h)/2 ;
      PDATA(gy) = PDATA(gradius) ;
      PDATA(gx) = PDATA(gradius) + (CDATA(w) - CDATA(h))/2 ;
    }

  PDATA(g2) = PDATA(gradius)*PDATA(gradius) ;

  /* push and load new viewport */
  _dxfPushViewport (CDATA(stack)) ;
  _dxfSetViewport (CDATA(stack), (int) PDATA(vllx), (int) PDATA(vurx),
		  (int) PDATA(vlly), (int) PDATA(vury)) ;
  _dxf_SET_VIEWPORT (PORT_CTX, (int) PDATA(vllx), (int) PDATA(vurx),
		    (int) PDATA(vlly), (int) PDATA(vury)) ;

  /* get current viewing matrix and push it */
  _dxfGetViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  _dxfPushViewMatrix(CDATA(stack)) ;

  /* load new projection */
  _dxf_SET_ORTHO_PROJECTION (PORT_CTX, 2, 1, 1, -1) ;

  /* globe image background coordinates for lrectread() and lrectwrite() */
  PDATA(illx) = XSCREENCLIP(PDATA(vllx)) ;
  PDATA(illy) = YSCREENCLIP(PDATA(vlly)) ;
  PDATA(iurx) = XSCREENCLIP(PDATA(vurx)) ;
  PDATA(iury) = YSCREENCLIP(PDATA(vury)) ;

  /* set solid area pattern */
  _dxf_SET_SOLID_FILL_PATTERN (PORT_CTX) ;

  EXIT((""));
}


static void
gnomonConfig (tdmInteractor I, int *c, int *d,
	      tdmInteractorRedrawMode redrawMode)
{
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("gnomonConfig(0x%x, 0x%x, 0x%x, 0x%x)",
	 I, c, d, redrawMode));

  /* gnomon position depends upon window size */
  PDATA(gradius) = GNOMONRADIUS ;
  PDATA(gx) = CDATA(w) - (GNOMONOFFSET+GNOMONRADIUS) ;
  PDATA(gy) = GNOMONOFFSET+GNOMONRADIUS ;
  PDATA(g2) = GNOMONRADIUS*GNOMONRADIUS ;
  PDATA(vllx) = CDATA(w) - (GNOMONOFFSET + 2*GNOMONRADIUS) ;
  PDATA(vlly) = GNOMONOFFSET ;
  PDATA(vurx) = CDATA(w) - GNOMONOFFSET ;
  PDATA(vury) = GNOMONOFFSET + 2*GNOMONRADIUS ;
      
  /* gnomon image background coordinates for lrectread() and lrectwrite() */
  PDATA(illx) = XSCREENCLIP(PDATA(vllx)) ;
  PDATA(illy) = YSCREENCLIP(PDATA(vlly)) ;
  PDATA(iurx) = XSCREENCLIP(PDATA(vurx)) ;
  PDATA(iury) = YSCREENCLIP(PDATA(vury)) ;

  PDATA(redrawmode) = redrawMode ;

  if (redrawMode == tdmAuxEchoMode)
    {
      PRINT(("redraw mode is tdmAuxEchoMode, skipping configuration"));
      _dxfPushViewport (CDATA(stack)) ;
      _dxfSetViewport (CDATA(stack), (int) PDATA(vllx), (int) PDATA(vurx),
		      (int) PDATA(vlly), (int) PDATA(vury)) ;
      _dxf_SET_VIEWPORT (PORT_CTX, (int) PDATA(vllx), (int) PDATA(vurx),
			(int) PDATA(vlly), (int) PDATA(vury)) ;

      _dxfPushViewMatrix(CDATA(stack)) ;
      _dxf_SET_ORTHO_PROJECTION (PORT_CTX, 2, 1, 1, -1) ;
    }
  else
      globeConfig (I, c, d, redrawMode) ;

  EXIT((""));
}


static void
viewerConfig (tdmInteractor I, int *c, int *d,
	      tdmInteractorRedrawMode redrawMode)
{
  DEFDATA(I,tdmRotateData) ;

  ENTRY(("viewerConfig(0x%x, 0x%x, 0x%x, 0x%x)",
	 I, c, d, redrawMode));

  /* get current view matrix */
  _dxfGetViewMatrix (CDATA(stack), CDATA(viewXfm)) ;

  /* virtual sphere radius is half of smallest window dimension */
  if (CDATA(h) > CDATA(w))
    {
      PDATA(gradius) = CDATA(w)/2 ;
      PDATA(gx) = PDATA(gradius) ;
      PDATA(gy) = PDATA(gradius) + (CDATA(h) - CDATA(w))/2 ;
    }
  else
    {
      PDATA(gradius) = CDATA(h)/2 ;
      PDATA(gy) = PDATA(gradius) ;
      PDATA(gx) = PDATA(gradius) + (CDATA(w) - CDATA(h))/2 ;
    }

  PDATA(g2) = PDATA(gradius)*PDATA(gradius) ;

  EXIT((""));
}


static void
RestoreConfig (tdmInteractor I,
	       int OrigDisplayMode, int OrigBufferMode,
	       tdmInteractorRedrawMode redrawMode)
{
  float M[4][4] ;
  int llx, lly, urx, ury ;
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("RestoreConfig(0x%x, %d, %d, 0x%x)",
	 I, OrigDisplayMode, OrigBufferMode, redrawMode));

  /* restore viewport */
  _dxfPopViewport (CDATA(stack)) ;
  _dxfGetViewport (CDATA(stack), &llx, &urx, &lly, &ury) ;
  _dxf_SET_VIEWPORT (PORT_CTX, llx, urx, lly, ury) ;

  if (PDATA(redrawmode) != tdmAuxEchoMode)
    {
      /* restore hardware projection matrix */
      if (CDATA(projection))
	  _dxf_SET_PERSPECTIVE_PROJECTION
	      (PORT_CTX, CDATA(fov), CDATA(aspect),
	       CDATA(Near), CDATA(Far)) ;
      else
	  _dxf_SET_ORTHO_PROJECTION
	      (PORT_CTX, CDATA(width), CDATA(aspect),
	       CDATA(Near), CDATA(Far)) ;

      /* restore frame buffer configuration */
      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, OrigDisplayMode, OrigBufferMode, redrawMode) ;

      /* restore zbuffer usage */
      _dxf_SET_ZBUFFER_STATUS (PORT_CTX, CDATA(zbuffer)) ;
    }

  /* restore viewing/modeling hardware matrix */
  _dxfPopViewMatrix(CDATA(stack)) ;
  _dxfGetViewMatrix (CDATA(stack), M) ;
  _dxf_LOAD_MATRIX (PORT_CTX, M) ;

  EXIT((""));
}

#if 0
/***** CAN THIS DEAD CODE BE REMOVED??????? ********/
static void
PointRoll (tdmInteractor I, int x, int y)
{
  /*
   *  DXApply vertical Z projection of pointer onto surface of virtual
   *  sphere and "attach" pointer to it, until pointer moves off echo.
   */
  
  DEFDATA(I,tdmRotateData) ;
  dxMatrix rot ;
  double a, u, v, bx, by, bz ;
  register float dx, dy, t, s, c, cx, sx, cy, sy ;
  float rx, ry, rz ;

  ENTRY(("PointRoll(0x%x, %d, %d)", I, x, y));
  
  y = YFLIP(y) ;

  if (CDATA(xlast) == x && CDATA(ylast) == y) {
    EXIT(("xlast && ylast unchanged"));
    return ;
  } else {
    CDATA(xlast) = x ;
    CDATA(ylast) = y ;
  }

  dx = x - PDATA(gx) ;
  dy = y - PDATA(gy) ;

  if (dx*dx + dy*dy > PDATA(g2))
    {
      /*
       *  Fell off the edge of the world:  concatenate newest matrix onto
       *  incremental accumulation and start twirling.
       */
      loadmatrix(PDATA(newXfm)) ;
      multmatrix(PDATA(incXfm)) ;
      getmatrix(PDATA(incXfm)) ;

      InitTwirlStroke (I, dx, dy) ;
      EXIT(("fell off world"));
      return ;
    }

  bx = (double)dx/(double)PDATA(gradius) ;
  by = (double)dy/(double)PDATA(gradius) ;

  /*
   *  Compute matrix to rotate vector [ax,ay,az] to [bx,by,bz].
   *  These vectors extend from the center of the sphere to the radius.
   */

  if (bx == PDATA(ax) && by == PDATA(ay)) {
    EXIT(("bx and by unchanged"));
    return ;
  }
  
  bz = sqrt (1.0 - (bx*bx + by*by)) ;
  
  /* cross product is the axis of rotation */
  rx = PDATA(ay)*bz - PDATA(az)*by ;
  ry = PDATA(az)*bx - PDATA(ax)*bz ;
  rz = PDATA(ax)*by - PDATA(ay)*bx ;

  /* length of axis is the sine of the angle to rotate */
  s = (float) sqrt ((double)rx*rx + (double)ry*ry + (double)rz*rz) ;

  /* cosine of angle is dot product of vectors */
  c = PDATA(ax)*bx + PDATA(ay)*by + PDATA(az)*bz ;

  /* normalize rotation axis */
  rx /= s ; ry /= s ; rz /= s ;
  
  /* compute matrix which rotates about [rx,ry,rz] */
  t = 1.0 - c ;

  ROTXYZ (rot, rx, ry, rz, s, c, t) ;
  loadmatrix (rot) ;

#ifdef RELATIVEROT
  /* accumulate relative rotation matrix onto transform */
  multmatrix(PDATA(newXfm)) ;
  getmatrix(PDATA(newXfm)) ;

  /* update previous vector */
  PDATA(ax) = bx ;
  PDATA(ay) = by ;
  PDATA(az) = bz ;
#else
  COPYMATRIX (PDATA(newXfm), rot) ;
#endif

  /* accumulate new and incremental transforms onto start transform */
  multmatrix(PDATA(incXfm)) ;
  multmatrix(PDATA(strXfm)) ;

  /* get new view, erase old echo, draw new echo */
  getmatrix(CDATA(viewXfm)) ;
  CDATA(view_state)++ ;
  
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 0) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1) ;

  /* redraw any auxiliary echo */
  tdmResumeEcho (AUX(I), tdmAuxEchoMode) ;
  swapbuffers() ;

  PDATA(rotation_update) = 1 ;

  EXIT((""));
}
#endif
