/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwNavigateInteractor.c,v $
  Author: Mark Hood

  The navigation interactor is a means for moving in 3D space.  The user is
  able to move back and forth along the current navigation vector (initially
  the view vector) by using the left and right mouse buttons.  At the same
  time, the navigation vector is incrementally updated to rotate (pivot)
  toward the mouse pointer position in the image window.  The incremental
  update speed may be adjusted by two parameters, rotate_speed_factor and
  translate_speed_factor.

  A separate parameter, look_at_direction, determines in what direction
  relative to the navigation vector the user is looking.  If the
  look_at_direction is not LOOK_FORWARD, then pivoting is disabled, leaving
  translation along the navigation vector the only possible movement.

\*---------------------------------------------------------------------------*/

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <stdio.h>
#include <math.h>
#include "hwDeclarations.h"
#include "hwMatrix.h"
#include "hwInteractorEcho.h"

#include "hwPortLayer.h"

#include "hwDebug.h"

/*
 *  Navigator private data structure.
 */

typedef struct {
  /* which mouse button pressed */
  int btn ;

  /* direction of movement */
  float nav_dir[3] ;
  float nav_up[3] ;
  int first_time ;

  /* view direction relative to navigation vector */
  int look_at_direction ;  /* tdmLOOK_FORWARD, tdmLOOK_LEFT, tdmLOOK_UP, etc */

  /* throttle factors for pivot and translation */
  float rotate_speed_factor ;	  /* 0.0 .. 100.0 */
  float translate_speed_factor ;  /* 0.0 .. 100.0 */

  /* radius of virtual sphere, center, and square of radius */
  float gradius, gx, gy, g2 ;

  /* update flag */
  int nav_update ;
} NavigateData ;

/*
 *  Forward references
 */

// static void DoubleClick (tdmInteractor, int, int, tdmInteractorReturn *) ;
static void StartStroke (tdmInteractor, int, int, int, int) ;
static void StrokePoint (tdmInteractor I, int x, int y, int flag, int state) ;
static void EndStroke (tdmInteractor, tdmInteractorReturnP) ;

/*
 *  Null functions
 */

static void NullFunction (tdmInteractor I, tdmInteractorRedrawMode M)
{
}

static void
NullDoubleClick (tdmInteractor I, int x, int y, tdmInteractorReturn *R)
{
  R->change = 0 ;
}


/*
 *  Creation function
 */

tdmInteractor
_dxfCreateNavigator (tdmInteractorWin W, tdmViewEchoFunc EchoFunc, void *udata)
{
  /*
   *  Initialize and return a handle to a navigation interactor.
   *  The echo function is supplied by the application.
   */

  register tdmInteractor I ;

  ENTRY(("_dxfCreateNavigator(0x%x, 0x%x, 0x%x)", W, EchoFunc, udata));

  if (W && (I = _dxfAllocateInteractor(W, sizeof(NavigateData))))
    {
      DEFDATA(I,NavigateData) ;

      /* instance initial interactor methods */
      FUNC(I, DoubleClick) = NullDoubleClick ;
      FUNC(I, StartStroke) = StartStroke ;
      FUNC(I, StrokePoint) = StrokePoint ;
      FUNC(I, EndStroke) = EndStroke ;
      FUNC(I, EchoFunc) = EchoFunc ;
      FUNC(I, ResumeEcho) = NullFunction ;
      FUNC(I, Destroy) = _dxfDeallocateInteractor ;
      FUNC(I, KeyStruck) = _dxfNullKeyStruck;

      
      /* init data */
      PDATA(look_at_direction) = tdmLOOK_FORWARD ;
      PDATA(rotate_speed_factor) = 21 ;
      PDATA(translate_speed_factor) = 11 ;
      PDATA(nav_update) = 0 ;
      PDATA(first_time) = 1 ;

      /* copy pointer to user data */
      UDATA(I) = udata ;

      EXIT(("OK"));
      return I ;
    }
  else
    {
      EXIT(("ERROR"));
      return 0 ;
    }
}


/*
 *  Utils
 */

void
_dxfResetNavigateInteractor (tdmInteractor I)
{
  ENTRY(("_dxfResetNavigateInteractor(0x%x)", I));

  if (I)
    {
      DEFDATA(I, NavigateData) ;
      PDATA(look_at_direction) = tdmLOOK_FORWARD ;
    }

  EXIT((""));
}

void
_dxfSetNavigateLookAt (tdmInteractor I,
		       int direction, float angle, float current_view[3][3], 
		       float viewDirReturn[3], float viewUpReturn[3])
{
  /*
   *  Set view vectors relative to navigation vectors.
   */
  
  ENTRY(("_dxfSetNavigateLookAt(0x%x, %d, %f, 0x%x, 0x%x, 0x%x)",
	 I, direction, angle, current_view, viewDirReturn, viewUpReturn));

  if (I)
    {
      DEFDATA(I, NavigateData) ;
      float mat[4][4], r[3], c, s, t ;

      PDATA(look_at_direction) = direction ;
      if (PDATA(first_time) || direction == tdmLOOK_ALIGN)
	{
	  /* set navigation vectors to current view basis vectors */
	  PDATA(nav_dir[0]) = -current_view[0][2] ;
	  PDATA(nav_dir[1]) = -current_view[1][2] ;
          PDATA(nav_dir[2]) = -current_view[2][2] ;

	  PDATA(nav_up[0]) = current_view[0][1] ;
	  PDATA(nav_up[1]) = current_view[1][1] ;
          PDATA(nav_up[2]) = current_view[2][1] ;

	  if (PDATA(first_time))
	    {
	      PRINT(("first time"));
	      PDATA(first_time) = 0 ;
	    }
	  if (direction == tdmLOOK_ALIGN)
	    {
	      PRINT(("LOOK_ALIGN"));
	      PDATA(look_at_direction) = tdmLOOK_FORWARD ;
	    }
	}

      /* derive matrix to rotate `angle' radians around axis `r' */
      switch (direction)
	{
	case tdmLOOK_ALIGN:
	case tdmLOOK_FORWARD:
	  PRINT(("%s", direction == tdmLOOK_ALIGN ? "" : "LOOK_FORWARD"));
	  VCOPY (r, PDATA(nav_up)) ;
	  angle = 0 ;
	  break ;
	case tdmLOOK_BACKWARD:
	  PRINT(("LOOK_BACKWARD"));
	  VCOPY (r, PDATA(nav_up)) ;
	  angle = M_PI ;
	  break ;
	case tdmLOOK_LEFT:
	  PRINT(("LOOK_LEFT %f", RAD2DEG(angle)));
	  VCOPY (r, PDATA(nav_up)) ;
	  break ;
	case tdmLOOK_RIGHT:
	  PRINT(("LOOK_RIGHT %f", RAD2DEG(angle)));
	  r[0] = -PDATA(nav_up[0]) ;
	  r[1] = -PDATA(nav_up[1]) ;
	  r[2] = -PDATA(nav_up[2]) ;
	  break ;
	case tdmLOOK_UP:
	  PRINT(("LOOK_UP %f", RAD2DEG(angle)));
	  CROSS (r, PDATA(nav_dir), PDATA(nav_up)) ;
	  break ;
	case tdmLOOK_DOWN:
	  PRINT(("LOOK_DOWN %f", RAD2DEG(angle)));
	  CROSS (r, PDATA(nav_up), PDATA(nav_dir)) ;
	  break ;
	default:
	  PRINT(("unknown look-at direction"));
	  break ;
	}
	  
      s = (float) sin((double)angle) ;
      c = (float) cos((double)angle) ;
      t = 1 - c ;
	  
      ROTXYZ (mat, r[0], r[1], r[2], s, c, t) ;
      XFORM_VECTOR (mat, PDATA(nav_dir), viewDirReturn) ;
      XFORM_VECTOR (mat, PDATA(nav_up), viewUpReturn) ;

      PRINT(("viewDirReturn:"));
      VPRINT(viewDirReturn) ;
      PRINT(("viewUpReturn:"));
      VPRINT(viewUpReturn) ;
    }

  EXIT((""));
}

void
_dxfSetNavigateTranslateSpeed (tdmInteractor I, float speed)
{
  ENTRY(("_dxfSetNavigateTranslateSpeed(0x%x, %f)", I, speed));

  if (I)
    {
      DEFDATA(I, NavigateData) ;
      PDATA(translate_speed_factor) = speed ;
    }

  EXIT((""));
}

void
_dxfSetNavigateRotateSpeed (tdmInteractor I, float speed)
{
  ENTRY(("_dxfSetNavigateRotateSpeed(0x%x, %f)", I, speed));

  if (I)
    {
      DEFDATA(I, NavigateData) ;
      PDATA(rotate_speed_factor) = speed ;
    }

  EXIT((""));
}


/*
 *  Method implementations
 */

static void 
StartStroke (tdmInteractor I, int x, int y, int btn, int s)
{
  float Near, Far, from[3], zaxis[3];
  DEFDATA(I,NavigateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("StartStroke(0x%x, %d, %d, %d)", I, x, y, btn));

  /* get current view matrix */
  _dxfGetViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  PRINT(("viewXfm:"));
  MPRINT(CDATA(viewXfm)) ;

  /* define a virtual sphere with radius half of smallest window dimension */
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

  /* record square of radius */
  PDATA(g2) = PDATA(gradius)*PDATA(gradius) ;

  /* record button in use */
  PDATA(btn) = btn ;

  if (! CDATA(projection))
    {
      /* force perspective projection */
      PRINT(("forcing perspective..."));

      CDATA(projection) = 1 ;
      CDATA(fov) = CDATA(width)/CDATA(vdist) ;
      GET_VC_ORIGIN(CDATA(viewXfm), from) ;
      GET_VIEW_DIRECTION(CDATA(viewXfm), zaxis) ;

      /* compute appropriate clip planes */
      _dxfGetNearFar(1, CDATA(w), CDATA(fov),
		     from, zaxis, CDATA(box), &Near, &Far);

      /* set up projection */
      _dxfSetProjectionInfo(CDATA(stack), 1, CDATA(fov),
			    CDATA(aspect), Near, Far);
      _dxf_SET_PERSPECTIVE_PROJECTION (PORT_CTX, CDATA(fov),
				       CDATA(aspect), Near, Far) ;
      CDATA(view_state)++ ;
    }

  EXIT((""));
}


static void
StrokePoint (tdmInteractor I, int x, int y, int flag, int state)
{
  DEFDATA(I,NavigateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  float rot[4][4], trn[4][4], mat[4][4] ;
  float Near, Far, from[3], zaxis[3];
  double a, len, square ;
  register float dx, dy, rx, ry, t, s, c ;

  ENTRY(("StrokePoint(0x%x, %d, %d)", I, x, y));

  if (PDATA(look_at_direction) == tdmLOOK_FORWARD)
    {
      /*
       *  Compute pivot axis and rotation angle.
       *
       *  Project mouse pointer onto a virtual sphere.  The pivot axis
       *  [rx,ry,rz] is the cross product of the projection [x,y,z] with
       *  the view vector [0,0,-1].  Since these are unit vectors the
       *  length of the cross product is also the sine of the angle
       *  between them.
       *
       *  rz is always zero so it is ignored here.
       */

      y = YFLIP(y) ;
      dx = x - PDATA(gx) ;
      dy = y - PDATA(gy) ;
      
      if ((square = dx*dx + dy*dy) > PDATA(g2))
	{
	  /* off the edge of the virtual sphere */
	  len = sqrt(square) ;
	  rx = -dy/len ;
	  ry =  dx/len ;
	  a = M_PI/2 ;
	}
      else if (square > 0)
	{
	  /* project point onto virtual sphere */
	  rx = -(double)dy / (double)PDATA(gradius) ;
	  ry =  (double)dx / (double)PDATA(gradius) ;
	  s = (float)sqrt ((double)rx*rx + (double)ry*ry) ;
	  rx /= s ;
	  ry /= s ;
	  a = asin(s) ;
	}
      else
	{
	  /* don't divide by zero */
	  rx = 0 ;
	  ry = 0 ;
	  a = 0 ;
	}
      
      /* multiply angle `a' by pivot speed factor */
      a *= PDATA(rotate_speed_factor) * 0.001 ;
      
      /* construct pivot matrix */
      s = sin(a) ; c = cos(a) ; t = 1 - c ;
      ROTXY (rot, rx, ry, s, c, t) ;
      
      /* compose with current view */
      MULTMATRIX (mat, CDATA(viewXfm), rot) ;

      /* update navigation vectors */
      PDATA(nav_dir[0]) = -mat[0][2] ;
      PDATA(nav_dir[1]) = -mat[1][2] ;
      PDATA(nav_dir[2]) = -mat[2][2] ;

      PDATA(nav_up[0]) = mat[0][1] ;
      PDATA(nav_up[1]) = mat[1][1] ;
      PDATA(nav_up[2]) = mat[2][1] ;
    }
  else
    {
      COPYMATRIX (mat, CDATA(viewXfm)) ;
    }

  if (PDATA(btn) == 1)
    {
      /* move view coordinate origin along navigate vector */
      COPYMATRIX (trn, identity) ;

      trn[3][0] = -PDATA(translate_speed_factor) * 0.0005 *
	           CDATA(vdist) * PDATA(nav_dir[0]) ;
      trn[3][1] = -PDATA(translate_speed_factor) * 0.0005 *
	           CDATA(vdist) * PDATA(nav_dir[1]) ;
      trn[3][2] = -PDATA(translate_speed_factor) * 0.0005 *
	           CDATA(vdist) * PDATA(nav_dir[2]) ;

      MULTMATRIX (CDATA(viewXfm), trn, mat) ;
    }
  else if (PDATA(btn) == 3)
    {
      /* move view coordinate origin backwards along navigate vector */
      COPYMATRIX (trn, identity) ;

      trn[3][0] =  PDATA(translate_speed_factor) * 0.0005 *
	           CDATA(vdist) * PDATA(nav_dir[0]) ;
      trn[3][1] =  PDATA(translate_speed_factor) * 0.0005 *
	           CDATA(vdist) * PDATA(nav_dir[1]) ;
      trn[3][2] =  PDATA(translate_speed_factor) * 0.0005 *
	           CDATA(vdist) * PDATA(nav_dir[2]) ;

      MULTMATRIX (CDATA(viewXfm), trn, mat) ;
    }
  else
    {
      COPYMATRIX (CDATA(viewXfm), mat) ;
    }

  /* get the new clip planes */
  GET_VC_ORIGIN(CDATA(viewXfm), from) ;
  GET_VIEW_DIRECTION(CDATA(viewXfm), zaxis) ;
  _dxfGetNearFar(1, CDATA(w), CDATA(fov), from, zaxis, CDATA(box), &Near, &Far);

  /* setting new clip planes requires creating a new projection matrix */
  _dxfSetProjectionInfo(CDATA(stack), 1, CDATA(fov), CDATA(aspect), Near, Far);
  _dxf_SET_PERSPECTIVE_PROJECTION(PORT_CTX, CDATA(fov), CDATA(aspect),
				  Near, Far);

  /* load the new view matrix */
  _dxf_LOAD_MATRIX (PORT_CTX, CDATA(viewXfm)) ;
  _dxfLoadViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  CDATA(view_state)++ ;

  /* draw the echo */
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 0) ;
  CALLFUNC(I,EchoFunc) (I, UDATA(I), CDATA(viewXfm), 1) ;

  PDATA(nav_update) = 1 ;

  EXIT((""));
}


static void 
EndStroke (tdmInteractor I, tdmInteractorReturnP R)
{
  /*
   *  End stroke and return appropriate info.
   */

  DEFDATA(I,NavigateData) ;

  ENTRY(("EndStroke(0x%x, 0x%x)", I, R));

  if (PDATA(nav_update))
    {
      PRINT(("current viewXfm:"));
      MPRINT(CDATA(viewXfm)) ;

      _dxfRenormalizeView(CDATA(viewXfm)) ;
      PRINT(("renormalized viewXfm:"));
      MPRINT(CDATA(viewXfm)) ;

      /*
       *  Convert VC origin to world coordinates to get `from' point.  Use
       *  transpose of orthogonal view transform (W->V) to get V->W.
       */
      R->from[0] = -(CDATA(viewXfm[3][0])*CDATA(viewXfm[0][0]) +
	             CDATA(viewXfm[3][1])*CDATA(viewXfm[0][1]) +
		     CDATA(viewXfm[3][2])*CDATA(viewXfm[0][2])) ;
      R->from[1] = -(CDATA(viewXfm[3][0])*CDATA(viewXfm[1][0]) +
	             CDATA(viewXfm[3][1])*CDATA(viewXfm[1][1]) +
		     CDATA(viewXfm[3][2])*CDATA(viewXfm[1][2])) ;
      R->from[2] = -(CDATA(viewXfm[3][0])*CDATA(viewXfm[2][0]) +
	             CDATA(viewXfm[3][1])*CDATA(viewXfm[2][1]) +
		     CDATA(viewXfm[3][2])*CDATA(viewXfm[2][2])) ;

      /* `to' point is `vdist' from `from' along view vector */
      R->to[0] = R->from[0] - CDATA(vdist)*CDATA(viewXfm[0][2]) ;
      R->to[1] = R->from[1] - CDATA(vdist)*CDATA(viewXfm[1][2]) ;
      R->to[2] = R->from[2] - CDATA(vdist)*CDATA(viewXfm[2][2]) ;
      
      /* `up' vector is 2nd column of orthogonal view matrix */
      R->up[0] = CDATA(viewXfm[0][1]) ;
      R->up[1] = CDATA(viewXfm[1][1]) ;
      R->up[2] = CDATA(viewXfm[2][1]) ;
      
      R->dist = CDATA(vdist) ;
      
      MCOPY(R->view, CDATA(viewXfm)) ;
      R->change = 1 ;
    }
  else
      /* no change */
      R->change = 0 ;

  PDATA(nav_update) = 0 ;

  EXIT((""));
}
