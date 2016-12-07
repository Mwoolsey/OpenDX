/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwCursorInteractor.c,v $
  Adapted for tdm from src/ui/uix/Picture*.* 10/29/91 by Mark Hood

  This file contains the GL implementation of the tdm cursor and roam
  interactors.

  Most of the code is taken from the dx Picture widget implementation with
  changes for GL output and tdm data structure differences.  Basic
  computations are the same; some differences include handedness of
  coordinate systems and the location of the origin in Xt and GL.

  Known limitations:

  - The cursor interactors operate in world coordinates only; local
    coordinate systems are not supported.
  - Application must provide copy of background image for lrectwrite().
  - Assumes image size is the same as the window size and that they have the
    same origin.

\*---------------------------------------------------------------------------*/

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif


#include <stdio.h>
#include <math.h>

#include "hwDeclarations.h"

#ifdef STANDALONE
#include <X11/Xlib.h>
#else
#include "hwMemory.h"
#include "hwPortLayer.h"
#if !defined(DX_NATIVE_WINDOWS)
#include <X11/Xlib.h>
#endif
#endif

#include "hwMatrix.h"
#include "hwInteractorEcho.h"
#include "hwCursorInteractor.h"

#include "hwDebug.h"

/*
 *  Forward references
 */

#ifdef sun4
/* HUGE_VAL is defined as (__infinity()) on sun4, but not typed as double! */
extern double __infinity() ;
#endif

static void Select (tdmInteractor, int, int, int, int) ;
static void BtnMotion (tdmInteractor, int, int, int, int) ;
static void Deselect (tdmInteractor I, tdmInteractorReturnP R) ;
static void CreateDelete (tdmInteractor, int, int, tdmInteractorReturnP R) ;
static void Config(tdmInteractor, tdmInteractorRedrawMode) ;
static void RestoreConfig(tdmInteractor, tdmInteractorRedrawMode) ;
static void Redisplay(tdmInteractor, tdmInteractorRedrawMode) ;
static void DestroyCursorInteractor(tdmInteractor) ;

static void PickStart (tdmInteractor, int, int, int, int) ;
static void PickEnd (tdmInteractor, tdmInteractorReturnP) ;

static void restore_image (tdmInteractor I) ;
static void draw_box (tdmInteractor I) ;
static void setup_bounding_box (tdmInteractor I) ;
static void get_transforms_from_basis (tdmInteractor I) ;
static void save_cursors_in_canonical_form (tdmInteractor I, int i) ;
static void restore_cursors_from_canonical_form (tdmInteractor I) ;
static void calc_projected_axis(tdmInteractor I) ;
static void draw_cursors (tdmInteractor I) ;
static int create_cursor (tdmInteractor I) ;
static int delete_cursor (tdmInteractor I, int id) ;
static int find_closest (tdmInteractor I, int x, int y) ;
static int select_cursor (tdmInteractor I, int id) ;
static int deselect_cursor (tdmInteractor I) ;
static int add2historybuffer (tdmInteractor I, int x, int y) ;

static void
constrain (tdmInteractor I, double *s0, double *s1, double *s2,
	   double dx, double dy, double dz,
	   double WItrans[4][4], double Wtrans[4][4], int sdx, int sdy) ;

static void
project (tdmInteractor I, double s0, double s1, double s2,
	 double Wtrans[4][4], double WItrans[4][4]) ;

static double
line_of_sight (tdmInteractor I, double s0, double s1, int *newbox) ;

static void
CallCursorCallbacks(tdmInteractor I, int reason, int cursor_num,
		    double screen_x, double screen_y, double screen_z,
		    tdmInteractorReturn *R) ;

/*
 *  Creation and management routines.
 */

tdmInteractor
_dxfCreateCursorInteractor (tdmInteractorWin W, tdmCursorType C)
{
  /*
   *  Initialize and return a handle to a cursor interactor.
   */

  register tdmInteractor I ;

  ENTRY(("_dxfCreateCursorInteractor(0x%x, 0x%x)", W, C));

  if (W && (I = _dxfAllocateInteractor (W, sizeof(CursorData))))
    {
      DEFDATA(I,CursorData) ;
      DEFPORT(I_PORT_HANDLE);

      PDATA(box_state) = -1 ;
      PDATA(box_change) = 1 ;
      PDATA(view_state) = -1 ;
      PDATA(visible) = 0 ;
      PDATA(reset) = 0 ;

      PDATA(cursor_size) = 5 ;
      PDATA(cursor_shape) = XmSQUARE ;
      PDATA(font) = -1 ;

      PDATA(selected) = NULL;
      PDATA(n_cursors) = 0;
      PDATA(xbuff) = NULL;
      PDATA(ybuff) = NULL;
      PDATA(zbuff) = NULL;
      PDATA(cxbuff) = NULL;
      PDATA(cybuff) = NULL;
      PDATA(czbuff) = NULL;

      PDATA(CursorNotBlank) = TRUE;
      PDATA(FirstTime) = TRUE;
      PDATA(FirstTimeMotion) = TRUE;

      PDATA(iMark) = 0 ;
      PDATA(piMark) = 0 ;

      /* save under for coordinate feedback area */
      PDATA(coord_saveunder) = NULL;
      if(!(PDATA(coord_saveunder) =
	  _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, COORD_WIDTH, COORD_HEIGHT)))
	goto error ;

      /* allocate a buffer to store bounding box */
      if(!(PDATA(basis) = (XmBasis *) tdmAllocateLocal (sizeof(XmBasis))))
	goto error ;

      /* initialize cursor pixmaps */
      _dxf_CREATE_CURSOR_PIXMAPS(I) ;

      if (PDATA(cursor_saveunder) ==  NULL ||
	  PDATA(ActiveSquareCursor) == NULL ||
	  PDATA(PassiveSquareCursor) == NULL ||
	  PDATA(ProjectionMark) == NULL)
	  goto error ;

      switch (C)
	{
	case tdmRoamCursor:
	  FUNC(I, DoubleClick) = CreateDelete ;
	  FUNC(I, StartStroke) = Select ;
	  FUNC(I, StrokePoint) = BtnMotion ;
	  FUNC(I, EndStroke) = Deselect ;
	  FUNC(I, ResumeEcho) = Redisplay ;
	  FUNC(I, Destroy) = DestroyCursorInteractor ;
	  PDATA(mode) = XmROAM_MODE ;
	  PDATA(roam_valid) = 0 ;
	  break ;
	case tdmPickCursor:
	  FUNC(I, DoubleClick) = _dxfNullDoubleClick ;
	  FUNC(I, StartStroke) = PickStart ;
	  FUNC(I, StrokePoint) = _dxfNullStrokePoint ;
	  FUNC(I, EndStroke) = PickEnd ;
	  FUNC(I, ResumeEcho) = _dxfNullResumeEcho ;
	  FUNC(I, Destroy) = DestroyCursorInteractor ;
	  PDATA(mode) = XmPICK_MODE ;
	  break ;
	case tdmProbeCursor:
	default:
	  FUNC(I, DoubleClick) = CreateDelete ;
	  FUNC(I, StartStroke) = Select ;
	  FUNC(I, StrokePoint) = BtnMotion ;
	  FUNC(I, EndStroke) = Deselect ;
	  FUNC(I, ResumeEcho) = Redisplay ;
	  FUNC(I, Destroy) = DestroyCursorInteractor ;
	  PDATA(mode) = XmCURSOR_MODE ;
	  break ;
	}
    
      FUNC(I, KeyStruck) = _dxfNullKeyStruck;

      EXIT(("I = 0x%x",I));
      return I ;
    }
  else
    {
      EXIT(("ERROR: could not allocate interactor I = NULL"));
      return 0 ;
    }
 error:
  DestroyCursorInteractor(I) ;
  EXIT(("ERROR: I = NULL"));
  return 0;
}


static void
DestroyCursorInteractor (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE);

  ENTRY(("DestroyCursorInteractor(0x%x)", I));

  if (PDATA(ActiveSquareCursor))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(ActiveSquareCursor)) ;

  if (PDATA(PassiveSquareCursor))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(PassiveSquareCursor)) ;

  if (PDATA(ProjectionMark))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(ProjectionMark)) ;

  if (PDATA(coord_saveunder))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(coord_saveunder)) ;

  if (PDATA(cursor_saveunder))
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, PDATA(cursor_saveunder)) ;

  if (PDATA(basis))
      tdmFree(PDATA(basis)) ;

  if (PDATA(selected))
      tdmFree(PDATA(selected)) ;

  if (PDATA(xbuff))
      tdmFree(PDATA(xbuff)) ;
  if (PDATA(ybuff))
      tdmFree(PDATA(ybuff)) ;
  if (PDATA(zbuff))
      tdmFree(PDATA(zbuff)) ;

  if (PDATA(cxbuff))
      tdmFree(PDATA(cxbuff)) ;
  if (PDATA(cybuff))
      tdmFree(PDATA(cybuff)) ;
  if (PDATA(czbuff))
      tdmFree(PDATA(czbuff)) ;

  _dxfDeallocateInteractor(I) ;
  EXIT((""));
}

void
_dxfResetCursorInteractor (tdmInteractor I)
{
  /* set reset state so that it can effected at Redisplay() time */
  ENTRY(("_dxfResetCursorInteractor(0x%x)", I));

  if (I)
    {
      DEFDATA(I,CursorData) ;
      PDATA(reset) = 1 ;
    }

  EXIT((""));
}


static void
ResetRoamPoint (tdmInteractor I)
{
  double x, y, z, nx, ny ;
  DEFDATA(I,CursorData) ;

  ENTRY(("ResetRoamPoint(0x%x)", I));

  if (CDATA(view_coords_set))
    {
      /* canonical world has bounding box origin at center */
      x = CDATA(to[0]) - PDATA(Ox) ;
      y = CDATA(to[1]) - PDATA(Oy) ;
      z = CDATA(to[2]) - PDATA(Oz) ;
    }
  else
    {
      /* canonical world has bounding box origin at center */
      x = PDATA(FacetXmax)/2 ;
      y = PDATA(FacetYmax)/2 ;
      z = PDATA(FacetZmax)/2 ;
    }

  /* canonical world -> pure screen */
  XFORM_COORDS(PDATA(Wtrans), x, y, z, nx, ny, PDATA(roam_zbuff)) ;
  PDATA(roam_xbuff) = (int) nx ;
  PDATA(roam_ybuff) = (int) ny ;

  /* pure screen -> pure world */
  save_cursors_in_canonical_form (I, 0) ;

  PDATA(roam_valid) = 1 ;

  EXIT((""));
}


void
_dxfCursorInteractorVisible (tdmInteractor I)
{

  ENTRY(("_dxfCursorInteractorVisible(0x%x)", I));

  if (I)
    {
      DEFDATA(I,CursorData) ;
      PDATA(visible) = 1 ;
      if (PDATA(mode) == XmROAM_MODE)
	{
	  /*
	   *  Get roam point: it's not loaded by application as cursors
	   *  are.  Must call Config in order to set up transforms and
	   *  bounding box properly.
	   */

	  Config (I, tdmFrontBufferDraw) ;
	  ResetRoamPoint(I) ;
	  RestoreConfig (I, tdmFrontBufferDraw) ;
	}
    }

  EXIT((""));
}


void
_dxfCursorInteractorInvisible (tdmInteractor I)
{

  ENTRY(("_dxfCursorInteractorInvisible(0x%x)", I));

  if (I)
    {
      DEFDATA(I,CursorData) ;
      PDATA(visible) = 0 ;
    }

  EXIT((""));
}


void
_dxfCursorInteractorChange (tdmInteractor I, int id, int reason,
			   float x, float y, float z)
{
  /*
   *  This routine is called by DX to load and delete cursors.
   */

  ENTRY(("_dxfCursorInteractorChange(0x%x, %d, %d, %f, %f, %f)",
	 I, id, reason, x, y, z));

  if (I)
    {
      DEFDATA(I,CursorData) ;

      PRINT(("%s mode",((PDATA(mode) == XmCURSOR_MODE) ? "cursor" : "roam")));

      if (reason == tdmCURSOR_CREATE)
	{
	  PRINT(("CURSOR_CREATE"));

	  deselect_cursor(I) ;
	  id = create_cursor(I) ;

	  PDATA(cxbuff)[id] = x ;
	  PDATA(cybuff)[id] = y ;
	  PDATA(czbuff)[id] = z ;

	  select_cursor (I, id) ;

	  /* pure world -> pure screen */
	  restore_cursors_from_canonical_form(I) ;

	  if (PDATA(visible))
	    {
	      PRINT(("interactor is visible"));
	      Config (I, tdmBothBufferDraw) ;
	      if (x < PDATA(Ox) || x > PDATA(FacetXmax)+PDATA(Ox) ||
		  y < PDATA(Oy) || y > PDATA(FacetYmax)+PDATA(Oy) ||
		  z < PDATA(Oz) || z > PDATA(FacetZmax)+PDATA(Oz))
		{
		  /*
		   *  Grow the bounding box.  This is a lot of work; what
		   *  we need here is a protocol element that says "expect
		   *  n cursors" so that we can do the work only after the
		   *  last cursor has been created.
		   */
		  setup_bounding_box(I) ;
		  restore_image(I) ;
		  RestoreConfig (I, tdmBothBufferDraw) ;
		  _dxfRedrawInteractorEchos(WINDOW(I)) ;
		}
	      else
		{
		  draw_cursors(I) ;
		  draw_box(I) ;
		  RestoreConfig (I, tdmBothBufferDraw) ;
		}
	    }
	  else
	    {
	      /* flag possible bounding box expansion */
	      PRINT(("interactor is not visible"));
	      PDATA(box_change) = 1 ;
	    }
	}
      else if (reason == tdmCURSOR_DELETE)
	{
	  PRINT(("CURSOR_DELETE"));

	  if (PDATA(visible))
	    {
	      /* establish graphics context for delete_cursor() */
	      PRINT(("interactor is visible"));
	      Config (I, tdmBothBufferDraw) ;
	    }
	  else
	    {
	      PRINT(("interactor is invisible"));
	    }

	  if (id < 0)
	    {
	      int n, i ;
	      PRINT(("delete all cursors"));

	      n = PDATA(n_cursors) ;
	      for (i = 0; i<n; i++)
		  /* N.B. delete_cursor() decrements PDATA(n_cursors) */
		  delete_cursor (I, 0) ;
	    }
	  else if (id < PDATA(n_cursors))
	    {
	      PRINT(("delete cursor %d", id));
	      delete_cursor (I, id) ;
	    }
	  else
	    {
	      /* should generate error here */
	    }

	  if (PDATA(visible))
	    {
	      /* possibly shrink the bounding box */
	      setup_bounding_box(I) ;
	      restore_image(I) ;
	      RestoreConfig (I, tdmBothBufferDraw) ;
	      _dxfRedrawInteractorEchos(WINDOW(I)) ;
	    }
	  else
	      /* flag possible bounding box contraction */
	      PDATA(box_change) = 1 ;
	}
    }

  EXIT((""));
}


static void
Redisplay (tdmInteractor I, tdmInteractorRedrawMode redrawMode)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("Redisplay(0x%x, %d)", I, redrawMode));

  if (PDATA(visible))
    {
      PRINT(("interactor is visible"));
      Config(I, redrawMode) ;

      if (PDATA(mode) == XmROAM_MODE && PDATA(reset) == 1)
	{
	  /* reset the roam point based on newly reset camera view */
	  ResetRoamPoint(I) ;
	  PDATA(reset) = 0 ;

	  /* deal with possible bounding box contraction */
	  setup_bounding_box(I) ;
	}

      /* read new save-under for digital feedback */
      _dxf_READ_BUFFER (PORT_CTX, PDATA(coord_llx), PDATA(coord_lly),
			PDATA(coord_urx), PDATA(coord_ury),
			PDATA(coord_saveunder)) ;

      /* draw cursors and bounding box */
      draw_cursors(I) ;
      draw_box(I) ;
      RestoreConfig (I, redrawMode) ;

      /* redraw auxiliary echos */
      PRINT(("echo redrawn, drawing aux echos"));
      if (redrawMode == tdmBothBufferDraw)
	{
	  PRINT(("tdmBothBufferDraw -> tdmFrontBufferDraw"));
	  tdmResumeEcho (AUX(I), tdmFrontBufferDraw) ;
	}
      else
	  tdmResumeEcho (AUX(I), redrawMode) ;
    }
  else
      PRINT(("invisible"));

  EXIT((""));
}


static void
Config (tdmInteractor I, tdmInteractorRedrawMode redrawMode)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("Config(0x%x, %d)", I, redrawMode));

  /* get origin of window and dimensions of screen */
  _dxf_GET_WINDOW_ORIGIN (PORT_CTX, &CDATA(ox), &CDATA(oy)) ;
  _dxf_GET_MAXSCREEN (PORT_CTX, &CDATA(xmaxscreen), &CDATA(ymaxscreen)) ;

  /* save viewport and viewing matrix */
  _dxfPushViewport(CDATA(stack)) ;
  _dxfGetViewMatrix (CDATA(stack), CDATA(viewXfm)) ;
  _dxfPushViewMatrix(CDATA(stack)) ;

  /* set up one-to-one mapping of screen space to world space */
  /* N.B. assumes image size and offset is same as window */
  _dxf_SET_WORLD_SCREEN (PORT_CTX, CDATA(w), CDATA(h)) ;

  /* disable Z buffer comparisons */
  if ((CDATA(zbuffer) = _dxf_GET_ZBUFFER_STATUS(PORT_CTX)))
      _dxf_SET_ZBUFFER_STATUS (PORT_CTX, 0) ;

  /* configure the frame buffer */
  _dxf_BUFFER_CONFIG (PORT_CTX, CDATA(image),
		     0, CDATA(h)-CDATA(ih), CDATA(iw)-1, CDATA(h)-1,
		     &PDATA(displaymode), &PDATA(buffermode), redrawMode) ;

  if (redrawMode == tdmBothBufferDraw || redrawMode == tdmFrontBufferDraw)
    {
      /* adjust save-under coordinates based on window size */
      PDATA(coord_urx) = CDATA(w)-1 ;
      PDATA(coord_ury) = CDATA(h)-1 ;
      PDATA(coord_llx) = MAX(0, (CDATA(w) - COORD_WIDTH)) ;
      PDATA(coord_lly) = MAX(0, (CDATA(h) - COORD_HEIGHT)) ;

      /* ensure that image area is on the physical screen */
      PDATA(coord_llx) = XSCREENCLIP(PDATA(coord_llx)) ;
      PDATA(coord_lly) = YSCREENCLIP(PDATA(coord_lly)) ;
      PDATA(coord_urx) = XSCREENCLIP(PDATA(coord_urx)) ;
      PDATA(coord_ury) = YSCREENCLIP(PDATA(coord_ury)) ;
    }

  /* update from changes */
  if (PDATA(box_change) ||
      PDATA(box_state)  != CDATA(box_state) ||
      PDATA(view_state) != CDATA(view_state))
    {
      get_transforms_from_basis(I);
      setup_bounding_box(I) ;
      calc_projected_axis(I) ;
      restore_cursors_from_canonical_form(I) ;
      PDATA(box_change) = 0 ;
      PDATA(box_state) = CDATA(box_state) ;
      PDATA(view_state) = CDATA(view_state) ;
    }

  EXIT((""));
}


static void
RestoreConfig (tdmInteractor I, tdmInteractorRedrawMode redrawMode)
{
  float M[4][4] ;
  int left, right, bottom, top ;
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("RestoreConfig(0x%x, %d)", I, redrawMode));

  /* restore viewport */
  _dxfPopViewport (CDATA(stack)) ;
  _dxfGetViewport (CDATA(stack), &left, &right, &bottom, &top) ;
  _dxf_SET_VIEWPORT (PORT_CTX, left, right, bottom, top) ;

  /* restore projection matrix */
  if (CDATA(projection))
      _dxf_SET_PERSPECTIVE_PROJECTION (PORT_CTX, CDATA(fov), CDATA(aspect),
				      CDATA(Near), CDATA(Far)) ;
  else
      _dxf_SET_ORTHO_PROJECTION (PORT_CTX, CDATA(width), CDATA(aspect),
				CDATA(Near), CDATA(Far)) ;

  /* restore view matrix */
  _dxfPopViewMatrix(CDATA(stack)) ;
  _dxfGetViewMatrix (CDATA(stack), M) ;
  _dxf_LOAD_MATRIX (PORT_CTX, M) ;

  /* restore zbuffer usage */
  if (CDATA(zbuffer))
      _dxf_SET_ZBUFFER_STATUS (PORT_CTX, 1) ;

  /* restore frame buffer configuration */
  _dxf_BUFFER_RESTORE_CONFIG
      (PORT_CTX, PDATA(displaymode), PDATA(buffermode), redrawMode) ;

  EXIT((""));
}


static void
restore_image (tdmInteractor I)
{
  /*
   *  Restore image supplied by application, erasing all interactor echos.
   */

  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("restore_image(0x%x)",I));

  if (CDATA(image))
    {
      /* N.B. following assumes image and window dimensions match */
      _dxf_WRITE_BUFFER (PORT_CTX,
			 0, 0, CDATA(iw)-1, CDATA(ih)-1, CDATA(image)) ;
    }

  EXIT((""));
}


/*
 *  Pick implementation.
 */

static void
PickStart (tdmInteractor I, int x, int y, int btn, int s)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("PickStart(0x%x, %d, %d, %d)", I, x, y, btn));

  y = YFLIP(y) ;

  if (!CLIPPED(x,y))
    {
      Config (I, tdmBothBufferDraw) ;
      _dxf_WRITE_BUFFER (PORT_CTX,
			 x - PDATA(cursor_size)/2,
			 y - PDATA(cursor_size)/2,
			 x + PDATA(cursor_size)/2,
			 y + PDATA(cursor_size)/2,
			 (void *)PDATA(PassiveSquareCursor)) ;

      RestoreConfig (I, tdmBothBufferDraw) ;
    }

  PDATA(pick_xbuff) = x ;
  PDATA(pick_ybuff) = y ;

  EXIT((""));
}

static void
PickEnd (tdmInteractor I, tdmInteractorReturnP R)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("PickEnd(0x%x, 0x%x)", I, R));

  R->change = 1 ;
  R->x = PDATA(pick_xbuff) ;
  R->y = PDATA(pick_ybuff) ;

  PRINT(("x: %f, y: %f", R->x, R->y));

  EXIT((""));
}

/*
 *  Code adapted from Picture.c follows.
 */

static void
Select (tdmInteractor I, int x, int y, int btn, int s)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  tdmInteractorReturn R ;
  int  	id;

  /*
   *  This routine is called in response to a button press.  Set up
   *  drawing context, perform selection, and stay in that context until
   *  Deselect() is called.
   */

  ENTRY(("Select(0x%x, %d, %d, %d)", I, x, y, btn));

  y = YFLIP(y) ;
  Config (I, tdmBothBufferDraw) ;

  PDATA(FirstTimeMotion) = TRUE ;
  PDATA(FirstTime) = TRUE ;

  /* reset pointer history buffer */
  PDATA(K) = 0 ;

  id = find_closest (I, x, y) ;

  if (id == -1)
    {
      /* no cursor selected, do nothing */
      deselect_cursor(I) ;
      EXIT(("id == -1"));
      return ;
    }

  if ((PDATA(CursorNotBlank) == TRUE))
    {
      _dxf_POINTER_INVISIBLE(PORT_CTX) ;
      PDATA(CursorNotBlank) = FALSE ;
    }

  if (PDATA(mode) == XmCURSOR_MODE)
    {
      PDATA(Z) = PDATA(zbuff)[id];
      PDATA(pcx) = x = PDATA(X) = PDATA(xbuff)[id];
      PDATA(pcy) = y = PDATA(Y) = PDATA(ybuff)[id];
    }
  else if (PDATA(mode) == XmROAM_MODE)
    {
      PDATA(Z) = PDATA(roam_zbuff);
      PDATA(pcx) = x = PDATA(X) = PDATA(roam_xbuff);
      PDATA(pcy) = y = PDATA(Y) = PDATA(roam_ybuff);
    }

  deselect_cursor(I) ;
  select_cursor (I, id) ;

  /* draw cursors and projection marks for selected */
  draw_cursors(I) ;
  project (I, (double)x, (double)y, (double)PDATA(Z),
	   PDATA(Wtrans), PDATA(WItrans)) ;

  _dxf_DRAW_MARKER(I) ;
  CallCursorCallbacks (I, tdmCURSOR_SELECT, id,
		       PDATA(X), PDATA(Y), PDATA(Z), &R) ;

  EXIT((""));
}


static void
BtnMotion (tdmInteractor I, int x, int y, int t, int s)
{
  /*
   *  This routine is called in response to pointer movement.  The
   *  drawing context should already be established.
   */

  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  tdmInteractorReturn R ;
  double dx, dy, dz;
  double major_dx, major_dy ;
  double angle, angle_tol ;
  int id, warp ;

  /*
   *  If we have no selected cursors, return.
   */

  ENTRY(("BtnMotion(0x%x, %d, %d)", I, x, y));

  if(PDATA(mode) == XmCURSOR_MODE)
    {
      for(id = 0; id < PDATA(n_cursors); id++)
	{
	  if (PDATA(selected[id])) break;
	}
      if (id == PDATA(n_cursors)) {
	EXIT((""));
	return;
      }
    }
  else if(PDATA(mode) == XmROAM_MODE)
    {
      if (!PDATA(roam_selected)) {
	EXIT((""));
	return;
      }
    }

  y = YFLIP(y) ;

  /*
   *  Pointer is invisible at this point.  If it moves to the edge of the
   *  screen, cursors won't move and the user won't have any indication why.
   *  Deal with this by warping the pointer back to the middle of the window
   *  whenever it goes outside.  Contents of pointer history buffer must be
   *  updated as well.
   */

  warp = FALSE ;
  if (x <= 0)
    {
      x = CDATA(w)/2 ;
      PDATA(px) += x ;
      PDATA(ppx) += x ;
      PDATA(pppx) += x ;
      PDATA(ppppx) += x ;
      PDATA(pppppx) += x ;
      PDATA(ppppppx) += x ;
      PDATA(pppppppx) += x ;
      warp = TRUE ;
    }
  if (x >= CDATA(w)-1)
    {
      x = CDATA(w)/2 ;
      PDATA(px) -= x ;
      PDATA(ppx) -= x ;
      PDATA(pppx) -= x ;
      PDATA(ppppx) -= x ;
      PDATA(pppppx) -= x ;
      PDATA(ppppppx) -= x ;
      PDATA(pppppppx) -= x ;
      warp = TRUE ;
    }
  if (y <= 0)
    {
      y = CDATA(h)/2 ;
      PDATA(py) += y ;
      PDATA(ppy) += y ;
      PDATA(pppy) += y ;
      PDATA(ppppy) += y ;
      PDATA(pppppy) += y ;
      PDATA(ppppppy) += y ;
      PDATA(pppppppy) += y ;
      warp = TRUE ;
    }
  if (y >= CDATA(h)-1)
    {
      y = CDATA(h)/2 ;
      PDATA(py) -= y ;
      PDATA(ppy) -= y ;
      PDATA(pppy) -= y ;
      PDATA(ppppy) -= y ;
      PDATA(pppppy) -= y ;
      PDATA(ppppppy) -= y ;
      PDATA(pppppppy) -= y ;
      warp = TRUE ;
    }

  if (warp)
    {
      /* move the invisible pointer back inside the window */
      _dxf_WARP_POINTER(PORT_CTX, CDATA(ox)+x, CDATA(oy)+y) ;
    }

  if (PDATA(K) < 8)
    {
      /* haven't filled the history buffer yet */
      add2historybuffer(I, x, y) ;
      EXIT((""));
      return ;
    }

  /*
   *  Use the history buffer as a low-pass filter over pointer movements.
   *  Angle of movement is determined from first and last points in the
   *  history buffer.
   */

  major_dx = x - PDATA(pppppppx) ;
  major_dy = y - PDATA(pppppppy) ;

  if ((major_dx == 0.0) && (major_dy == 0.0)) {
    EXIT((""));
    return ;
  }

  angle = acos(major_dx/sqrt(major_dx*major_dx + major_dy*major_dy)) ;
  if (major_dy > 0)
    {
      angle = 2*M_PI - angle ;
    }

  angle_tol = 0.25 ;

  dz = 0.0 ;
  if ((fabs(PDATA(angle_posz) - angle) < angle_tol) ||
      (2*M_PI - fabs(PDATA(angle_posz) - angle) < angle_tol))
    {
      dz = (y - PDATA(py))*(y - PDATA(py)) +
	   (x - PDATA(px))*(x - PDATA(px)) ;
      dz = sqrt(dz) ;
    }
  if ((fabs(PDATA(angle_negz) - angle) < angle_tol) ||
      (2*M_PI - fabs(PDATA(angle_negz) - angle) < angle_tol))
    {
      dz = (y - PDATA(py))*(y - PDATA(py)) +
 	   (x - PDATA(px))*(x - PDATA(px)) ;
      dz = sqrt(dz) ;
      dz = -dz ;
    }

  dx = 0.0 ;
  if ((fabs(PDATA(angle_posx) - angle) < angle_tol) ||
      (2*M_PI - fabs(PDATA(angle_posx) - angle) < angle_tol))
    {
      dx = (y - PDATA(py))*(y - PDATA(py)) +
	   (x - PDATA(px))*(x - PDATA(px)) ;
      dx = sqrt(dx) ;
    }
  if ((fabs(PDATA(angle_negx) - angle) < angle_tol) ||
      (2*M_PI - fabs(PDATA(angle_negx) - angle) < angle_tol))
    {
      dx = (y - PDATA(py))*(y - PDATA(py)) +
	   (x - PDATA(px))*(x - PDATA(px)) ;
      dx = sqrt(dx) ;
      dx = -dx ;
    }

  dy = 0.0 ;
  if ((fabs(PDATA(angle_posy) - angle) < angle_tol) ||
      (2*M_PI - fabs(PDATA(angle_posy) - angle) < angle_tol))
    {
      dy = (y - PDATA(py))*(y - PDATA(py)) +
	   (x - PDATA(px))*(x - PDATA(px)) ;
      dy = sqrt(dy) ;
    }
  if ((fabs(PDATA(angle_negy) - angle) < angle_tol) ||
      (2*M_PI - fabs(PDATA(angle_negy) - angle) < angle_tol))
    {
      dy = (y - PDATA(py))*(y - PDATA(py)) +
	   (x - PDATA(px))*(x - PDATA(px)) ;
      dy = sqrt(dy) ;
      dy = -dy ;
    }

  /*
   * Constrain motion along a single axis.
   */

  switch (CDATA(cursor_constraint))
    {
    case tdmCONSTRAIN_NONE:
      break;
    case tdmCONSTRAIN_X:
      dy = dz = 0.0;
      break;
    case tdmCONSTRAIN_Y:
      dx = dz = 0.0;
      break;
    case tdmCONSTRAIN_Z:
      dx = dy = 0.0;
      break;
    }

  if (!PDATA(x_movement_allowed)) dx = 0.0;
  if (!PDATA(y_movement_allowed)) dy = 0.0;
  if (!PDATA(z_movement_allowed)) dz = 0.0;

  /* bail out if appropriate */
  if (dz == 0.0 && dy == 0.0 && dx == 0.0)
    {
      add2historybuffer (I, x, y) ;
      EXIT((""));
      return ;
    }

  /* constrain motion to within the bounding box */
  /*X = PDATA(X) ; Y = PDATA(Y) ; Z = PDATA(Z) ;*/

  constrain (I, &PDATA(X), &PDATA(Y), &PDATA(Z), dx, dy, dz,
	     PDATA(WItrans), PDATA(Wtrans),
	     x - PDATA(px), y - PDATA(py)) ;

  /* update cursor position */
  if (PDATA(mode) == XmCURSOR_MODE)
    {
      PDATA(xbuff)[id] = (int)PDATA(X) ;
      PDATA(ybuff)[id] = (int)PDATA(Y) ;
      PDATA(zbuff)[id] = PDATA(Z) ;

      save_cursors_in_canonical_form (I, id) ;
    }
  else if (PDATA(mode) == XmROAM_MODE)
    {
      PDATA(roam_xbuff) = (int)PDATA(X) ;
      PDATA(roam_ybuff) = (int)PDATA(Y) ;
      PDATA(roam_zbuff) = PDATA(Z) ;

      save_cursors_in_canonical_form(I, 0) ;
    }

  /* erase previous projection marks and cursor */
  _dxf_ERASE_PREVIOUS_MARKS(I) ;
  _dxf_ERASE_CURSOR (I, PDATA(pcx), PDATA(pcy)) ;

  /* repair damage to aux echo (gnomon) not stored in image */
  tdmResumeEcho (AUX(I), tdmAuxEchoMode) ;

  /* draw new selected cursor position and current position of others */
  if (PDATA(FirstTimeMotion) != TRUE)
      draw_cursors(I) ;

  PDATA(FirstTimeMotion) = FALSE ;

  /* draw new projection marks */
  project (I, (double)PDATA(X), (double)PDATA(Y), (double)PDATA(Z),
	   PDATA(Wtrans), PDATA(WItrans)) ;
  _dxf_DRAW_MARKER(I) ;

  /* repair damage to bounding box */
  draw_box(I) ;

  /* record prev cursor x,y so we can erase it next time it is moved */
  PDATA(pcx) = (int)PDATA(X);
  PDATA(pcy) = (int)PDATA(Y);

  /* digital feedback */
  CallCursorCallbacks (I, tdmCURSOR_DRAG, id,
		       PDATA(X), PDATA(Y), PDATA(Z), &R) ;
  add2historybuffer (I, x, y) ;

  EXIT((""));
}


static void
Deselect (tdmInteractor I, tdmInteractorReturn *R)
{
  /*
   *  This routine is called in response to a pointer button release.  Drawing
   *  should already be configured and RestoreConfig() must be called
   *  before exiting this routine.
   */

  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int i;

  ENTRY(("Deselect(0x%x, 0x%x)", I, R));
  
  if (PDATA(mode) == XmCURSOR_MODE)
    {
      PDATA(cursor_num) = -1 ;
      for (i = 0 ; i < PDATA(n_cursors) ; i++)
	{
	  if (PDATA(selected)[i] == TRUE)
	    {
	      if (PDATA(CursorNotBlank) != TRUE)
		{
		  /* warp pointer back */
		  _dxf_WARP_POINTER(PORT_CTX,
				   CDATA(ox)+PDATA(xbuff[i]),
				   CDATA(oy)+PDATA(ybuff[i])) ;
		  _dxf_POINTER_VISIBLE(PORT_CTX) ;
		  PDATA(CursorNotBlank) = TRUE;
		}
	      PDATA(cursor_num) = i;
	    }
	}
      if (PDATA(cursor_num) == -1)
	  R->change = 0 ;
      else
	{
	  CallCursorCallbacks (I, tdmCURSOR_MOVE, PDATA(cursor_num),
			       (double) PDATA(xbuff[PDATA(cursor_num)]),
			       (double) PDATA(ybuff[PDATA(cursor_num)]),
			       PDATA(zbuff[PDATA(cursor_num)]),
			       R) ;
	}
    }
  else if (PDATA(mode) == XmROAM_MODE)
    {
      if (PDATA(roam_selected) && PDATA(CursorNotBlank) != TRUE )
	{
	  /* warp pointer back */
	  _dxf_WARP_POINTER(PORT_CTX,
			   CDATA(ox)+PDATA(roam_xbuff),
			   CDATA(oy)+PDATA(roam_ybuff)) ;
	  _dxf_POINTER_VISIBLE(PORT_CTX) ;
	  PDATA(CursorNotBlank) = TRUE;
	}

      PDATA(cursor_num) = 0 ;

      if (! PDATA(roam_selected))
	  R->change = 0 ;
      else
	{
	  CallCursorCallbacks (I, tdmCURSOR_MOVE, 0,
			       (double) PDATA(roam_xbuff),
			       (double) PDATA(roam_ybuff),
			       PDATA(roam_zbuff), R) ;
	}
    }

  if ( PDATA(CursorNotBlank) != TRUE )
    {
      _dxf_POINTER_INVISIBLE(PORT_CTX) ;
      PDATA(CursorNotBlank) = TRUE;
    }

  PDATA(FirstTimeMotion) = TRUE ;
  PDATA(FirstTime) = TRUE ;
  PDATA(K) = 0 ;

  _dxf_ERASE_PREVIOUS_MARKS(I) ;
  draw_box(I) ;
  draw_cursors(I) ;
  RestoreConfig (I, tdmBothBufferDraw) ;

  EXIT((""));
}


static void
CreateDelete (tdmInteractor I, int x, int y, tdmInteractorReturn *R)
{
  /*
   *  Create and delete cursors, or move a roam point, in response to a
   *  double click.  Config() must be called here to set up the drawing
   *  context, and RestoreConfig() must be called before returning.
   */
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int id, new_box ;

  ENTRY(("CreateDelete(0x%x, %d, %d, 0x%x)", I, x, y, R));

  Config (I, tdmBothBufferDraw) ;
  y = YFLIP(y) ;

  if ((id = find_closest (I, x, y)) != -1)
    {
      /* delete cursor if not roaming */
      if (PDATA(mode) != XmROAM_MODE)
	{
	  delete_cursor (I, id) ;
	  CallCursorCallbacks (I, tdmCURSOR_DELETE, id, 0, 0, 0, R) ;
	  /* repair box, prevent flashing when manipulating aux echos */
	  draw_box(I) ;
	}
      /* no-op if roaming */
      RestoreConfig (I, tdmBothBufferDraw) ;
      EXIT((""));
      return ;
    }

  /* project point onto bounding box */
  PDATA(Z) = line_of_sight (I, (double)(x), (double)(y), &new_box) ;

  if (PDATA(mode) == XmCURSOR_MODE)
    {
      /* create new cursor if in cursor mode */
      PDATA(X) = x ;
      PDATA(Y) = y ;

      deselect_cursor(I) ;
      id = create_cursor(I) ;

      PDATA(xbuff)[id] = x ;
      PDATA(ybuff)[id] = y ;
      PDATA(zbuff)[id] = PDATA(Z) ;

      select_cursor (I, id) ;
      save_cursors_in_canonical_form (I, id) ;

      CallCursorCallbacks (I, tdmCURSOR_CREATE, id,
			   PDATA(X), PDATA(Y), PDATA(Z), R) ;
    }
  else if (PDATA(mode) == XmROAM_MODE)
    {
      /* move the roam point */
      _dxf_ERASE_CURSOR (I, PDATA(roam_xbuff), PDATA(roam_ybuff)) ;

      PDATA(X) = x ;
      PDATA(Y) = y ;

      PDATA(roam_xbuff) = x ;
      PDATA(roam_ybuff) = y ;
      PDATA(roam_zbuff) = PDATA(Z) ;

      select_cursor (I, 0) ;
      save_cursors_in_canonical_form (I, 0) ;

      CallCursorCallbacks (I, tdmCURSOR_MOVE, id,
			   PDATA(X), PDATA(Y), PDATA(Z), R);
    }

  /* reset the pointer history buffer */
  PDATA(K) = 0 ;
  add2historybuffer (I, x, y) ;

  /* record prev cursor position for future erasure */
  PDATA(pcx) = (int)PDATA(X) ;
  PDATA(pcy) = (int)PDATA(Y) ;

  if (new_box)
    {
      /* recompute bounding box, restore image, redraw interactors */
      setup_bounding_box(I) ;
      restore_image(I) ;
      RestoreConfig (I, tdmBothBufferDraw) ;
      _dxfRedrawInteractorEchos(WINDOW(I)) ;
      /*
       *  N.B. recursions such as above can only be done outside
       *  Config()/RestoreConfig() pairs, as not all graphics context is
       *  stacked.
       */
    }
  else
    {
      /* draw newly created or moved cursor */
      _dxf_ERASE_PREVIOUS_MARKS(I) ;
      draw_cursors(I);
      /* repair box, prevent flashing when manipulating aux echos */
      draw_box(I) ;
      RestoreConfig (I, tdmBothBufferDraw) ;
    }

  EXIT((""));
}


static void
draw_cursors (tdmInteractor I)
{
  /*
   *  drawing context should already be established if visible.
   */

  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int i;

  ENTRY(("draw_cursors(0x%x)", I));

  if (!PDATA(visible))
    {
      PRINT(("invisible"));
      EXIT((""));
      return ;
    }

  if (PDATA(mode) == XmCURSOR_MODE)
    {
      for (i = 0; i < PDATA(n_cursors); i++)
	{
	  if (CLIPPED(PDATA(xbuff[i]),PDATA(ybuff[i])))
	      continue ;

	  /* depth cueing ignored for now */
	  /*depth = Z_LEVELS*(PDATA(zbuff)[i] - PDATA(Zmin))/
	            (PDATA(Zmax) - PDATA(Zmin));*/

	  if( PDATA(selected)[i] == TRUE )
	      _dxf_WRITE_BUFFER (PORT_CTX,
				PDATA(xbuff[i]) - PDATA(cursor_size)/2,
				PDATA(ybuff[i]) - PDATA(cursor_size)/2,
				PDATA(xbuff[i]) + PDATA(cursor_size)/2,
				PDATA(ybuff[i]) + PDATA(cursor_size)/2,
				(void *)PDATA(ActiveSquareCursor)) ;
	  else
	      _dxf_WRITE_BUFFER (PORT_CTX,
				PDATA(xbuff[i]) - PDATA(cursor_size)/2,
				PDATA(ybuff[i]) - PDATA(cursor_size)/2,
				PDATA(xbuff[i]) + PDATA(cursor_size)/2,
				PDATA(ybuff[i]) + PDATA(cursor_size)/2,
				(void *)PDATA(PassiveSquareCursor)) ;
	}
    }
  else if (PDATA(mode) == XmROAM_MODE &&
	   !CLIPPED(PDATA(roam_xbuff), PDATA(roam_ybuff)))
    {
      /* depth cueing ignored for now */
      /*depth = Z_LEVELS*(PDATA(roam_zbuff) - PDATA(Zmin))/
	               (PDATA(Zmax) - PDATA(Zmin)) ;*/

      if(PDATA(roam_selected))
	  _dxf_WRITE_BUFFER (PORT_CTX,
			    PDATA(roam_xbuff) - PDATA(cursor_size)/2,
			    PDATA(roam_ybuff) - PDATA(cursor_size)/2,
			    PDATA(roam_xbuff) + PDATA(cursor_size)/2,
			    PDATA(roam_ybuff) + PDATA(cursor_size)/2,
			    (void *)PDATA(ActiveSquareCursor)) ;
      else
	  _dxf_WRITE_BUFFER (PORT_CTX,
			    PDATA(roam_xbuff) - PDATA(cursor_size)/2,
			    PDATA(roam_ybuff) - PDATA(cursor_size)/2,
			    PDATA(roam_xbuff) + PDATA(cursor_size)/2,
			    PDATA(roam_ybuff) + PDATA(cursor_size)/2,
			    (void *)PDATA(PassiveSquareCursor)) ;
    }

  EXIT((""));
}


/*
 * !!!! Using macros with return statements in them is very difficult to
 * !!!! maintain.  This should be fixed.  (Jay Rush)
 */

#define CLIP2D(x1, y1, x2, y2)						 \
{                                                                        \
  /* clip (x2,y2) of line [(x1,y1) (x2,y2)] to image width and height */ \
  dv[0] = x2 - x1 ;							 \
  dv[1] = y2 - y1 ;							 \
									 \
  /* (x1,y1) + t*(dv[0],dv[1]) = (x2,y2) */                              \
  if (x2 > (CDATA(w)-1))						 \
    {									 \
      x2 = CDATA(w)-1 ;							 \
      if (dv[0] == 0.0) {						 \
        EXIT(("dv[0] == 0.0"));						 \
        return ;                                              		 \
      }									 \
      t = ((CDATA(w)-1) - x1) / dv[0] ;					 \
      y2 = y1 + t*dv[1] ;						 \
    }									 \
  else if (x2 < 0)							 \
    {									 \
      x2 = 0 ;								 \
      if (dv[0] == 0.0) {						 \
        EXIT(("dv[0] == 0.0"));						 \
        return ;                                              		 \
      }									 \
      t = -x1/dv[0] ;							 \
      y2 = y1 + t*dv[1] ;						 \
    }									 \
									 \
  if (y2 > (CDATA(h)-1))						 \
    {									 \
      y2 = CDATA(h)-1 ;							 \
      if (dv[1] == 0.0) {						 \
        EXIT(("dv[1] == 0.0"));						 \
        return ;                                              		 \
      }									 \
      t = ((CDATA(h)-1) - y1) / dv[1] ;					 \
      x2 = x1 + t*dv[0] ;						 \
    }									 \
  else if (y2 < 0)							 \
    {									 \
      y2 = 0 ;								 \
      if (dv[1] == 0.0) {						 \
        EXIT(("dv[1] == 0.0"));						 \
        return ;                                              		 \
      }									 \
      t = -y1/dv[1] ;							 \
      x2 = x1 + t*dv[0] ;						 \
    }									 \
                                                                         \
  if (x2>(CDATA(w)-1) || x2<0 || y2>(CDATA(h)-1) || y2<0) {		 \
    EXIT(("x2 or y2 outside window"));					 \
    return ;       							 \
  }								         \
}


static void
draw_box_line(tdmInteractor I, struct pnt p0, struct pnt p1,
	      int moveOK, int constraint_axis)
{
  /* set line attributes, clip it, draw it */
  DEFDATA(I, CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int style ;
  long color ;
  double t, dv[4] ;

  ENTRY(("draw_box_line(0x%x, 0x%x, 0x%x, %d, %d)",
	 I, &p0, &p1, moveOK, constraint_axis));
  
  if (moveOK && (CDATA(cursor_constraint) == tdmCONSTRAIN_NONE ||
		 CDATA(cursor_constraint) == constraint_axis))
      color = 0xffffffff ;
  else
      color = 0x8f8f8f8f ;

  if (!p0.visible || !p1.visible)
      /* dashed */
      style = 1 ;
  else
      /* solid */
      style = 0 ;

  _dxf_SET_LINE_ATTRIBUTES (PORT_CTX, color, style, 1.0) ;

  if (p0.pwcoor > 0.0 || p1.pwcoor > 0.0)
    {
      /* at least one endpoint visible in front of eye */
      if (p0.pwcoor < 0.0 || p1.pwcoor < 0.0)
        {
	  /* clip the negative endpoint to the w=0 hyperplane */
	  struct pnt *start, *end ;

	  if (p0.pwcoor > 0.0)
	    { start = &p0 ; end = &p1 ; }
	  else
	    { start = &p1 ; end = &p0 ; }

	  dv[0] = end->pxcoor - start->pxcoor ;
	  dv[1] = end->pycoor - start->pycoor ;
       /* dv[2] = end->pzcoor - start->pzcoor ; */
	  dv[3] = end->pwcoor - start->pwcoor ;

	  /* start->pwcoor + t*dv[3] = clipend->pwcoor = w = 0 */
	  t = (-start->pwcoor) / dv[3] ; 

	  /* can't divide by w=0, but multiplying by 1e6 is good enuf */
	  end->sxcoor = (start->pxcoor + t*dv[0]) * 1000000 ;
	  end->sycoor = (start->pycoor + t*dv[1]) * 1000000 ;
	}

      /* clip both endpoints of screen coord line to image bounds */
      /* Beware! -- this macro may return without drawing line!! */
      CLIP2D (p0.sxcoor, p0.sycoor, p1.sxcoor, p1.sycoor) ;
      CLIP2D (p1.sxcoor, p1.sycoor, p0.sxcoor, p0.sycoor) ;

      _dxf_DRAW_LINE (PORT_CTX,
		      (int)p0.sxcoor, (int)p0.sycoor,
		      (int)p1.sxcoor, (int)p1.sycoor) ;
    }

  EXIT((""));
}


static void
draw_box (tdmInteractor I)
{
  /*
   *  renderer context should already be set up for cursor mode.
   */

  DEFDATA(I, CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("draw_box(0x%x)", I));
  
  draw_box_line (I, PDATA(Face1)[0], PDATA(Face1)[1],
		 PDATA(z_movement_allowed), tdmCONSTRAIN_Z) ;

  draw_box_line (I, PDATA(Face1)[2], PDATA(Face1)[1],
		 PDATA(x_movement_allowed), tdmCONSTRAIN_X) ;

  draw_box_line (I, PDATA(Face1)[3], PDATA(Face1)[2],
		 PDATA(z_movement_allowed), tdmCONSTRAIN_Z) ;

  draw_box_line (I, PDATA(Face1)[0], PDATA(Face1)[3],
		 PDATA(x_movement_allowed), tdmCONSTRAIN_X) ;

  draw_box_line (I, PDATA(Face2)[0], PDATA(Face2)[1],
		 PDATA(z_movement_allowed), tdmCONSTRAIN_Z) ;

  draw_box_line (I, PDATA(Face2)[2], PDATA(Face2)[1],
		 PDATA(x_movement_allowed), tdmCONSTRAIN_X) ;

  draw_box_line (I, PDATA(Face2)[2], PDATA(Face2)[3],
		 PDATA(z_movement_allowed), tdmCONSTRAIN_Z) ;

  draw_box_line (I, PDATA(Face2)[0], PDATA(Face2)[3],
		 PDATA(x_movement_allowed), tdmCONSTRAIN_X) ;

  draw_box_line (I, PDATA(Face1)[0], PDATA(Face2)[0],
		 PDATA(y_movement_allowed), tdmCONSTRAIN_Y) ;

  draw_box_line (I, PDATA(Face1)[1], PDATA(Face2)[1],
		 PDATA(y_movement_allowed), tdmCONSTRAIN_Y) ;

  draw_box_line (I, PDATA(Face1)[2], PDATA(Face2)[2],
		 PDATA(y_movement_allowed), tdmCONSTRAIN_Y) ;

  draw_box_line (I, PDATA(Face1)[3], PDATA(Face2)[3],
		 PDATA(y_movement_allowed), tdmCONSTRAIN_Y) ;

  /* reset line attributes to white solid single width */
  _dxf_SET_LINE_ATTRIBUTES (PORT_CTX, 0xffffffff, 0, 1.0) ;

  EXIT((""));
}


static void
CallCursorCallbacks(tdmInteractor I, int reason, int cursor_num,
		    double screen_x, double screen_y, double screen_z,
		    tdmInteractorReturn *R)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  char text[256] ;

  /*
   *  Nothing is actually called back in the TDM version.  `R' is
   *  returned instead.
   */

  ENTRY(("CallCursorCallbacks(0x%x, %d, %d, %f, %f, %f, 0x%x)",
	 I, reason, cursor_num, screen_x, screen_y, screen_z, R));

  R->change = 1 ;
  R->reason = reason ;
  R->id = cursor_num ;

  if (reason == tdmCURSOR_DELETE) {
    EXIT((""));
    return ;
  }

  /* transform to world coordinates */
  XFORM_COORDS(PDATA(PureWItrans),
	       screen_x, screen_y, screen_z, R->x, R->y, R->z) ;

  if (reason == tdmCURSOR_CREATE) {
    EXIT((""));
    return ;
  }

  /* erase the coordinate feedback area */
  _dxf_WRITE_BUFFER (PORT_CTX,
		    PDATA(coord_llx), PDATA(coord_lly),
		    PDATA(coord_urx), PDATA(coord_ury),
		    (void *)PDATA(coord_saveunder)) ;

  if (reason == tdmCURSOR_MOVE) {
    EXIT((""));
    return ;
  }

  sprintf (text, "( %8g, %8g, %8g )", R->x, R->y, R->z) ;
  _dxf_DRAW_CURSOR_COORDS(I, text) ;

  EXIT((""));
}


static void
get_transforms_from_basis (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  float projmat[4][4] ;
  int i, j;

  ENTRY(("get_transforms_from_basis(0x%x)", I));
  
  if (PDATA(box_change) ||
      PDATA(box_state) != CDATA(box_state))
    {
      /*
       *  In this implementation Bw is only used to define the bounding
       *  box, and does not define the world->screen transform.  The view
       *  transform is taken directly from the matrix stack in order to
       *  interact properly with the view rotator.
       */
      PDATA(basis)->Bw[0][3] = 0 ;
      PDATA(basis)->Bw[1][3] = 0 ;
      PDATA(basis)->Bw[2][3] = 0 ;
      PDATA(basis)->Bw[3][3] = 1 ;
      
      PDATA(Ox) = PDATA(basis)->Bw[3][0] = CDATA(box[0][0]) ;
      PDATA(Oy) = PDATA(basis)->Bw[3][1] = CDATA(box[0][1]) ;
      PDATA(Oz) = PDATA(basis)->Bw[3][2] = CDATA(box[0][2]) ;
      
      PDATA(basis)->Bw[0][0] = CDATA(box[4][0]) ;
      PDATA(basis)->Bw[0][1] = CDATA(box[4][1]) ;
      PDATA(basis)->Bw[0][2] = CDATA(box[4][2]) ;
      
      PDATA(basis)->Bw[1][0] = CDATA(box[2][0]) ;
      PDATA(basis)->Bw[1][1] = CDATA(box[2][1]) ;
      PDATA(basis)->Bw[1][2] = CDATA(box[2][2]) ;
      
      PDATA(basis)->Bw[2][0] = CDATA(box[1][0]) ;
      PDATA(basis)->Bw[2][1] = CDATA(box[1][1]) ;
      PDATA(basis)->Bw[2][2] = CDATA(box[1][2]) ;
    }	

  if (PDATA(view_state) != CDATA(view_state))
    {
      /*
       *  Compute PureWtrans and its inverse by accumulating the view,
       *  projection, and screen transforms.  PureWtrans and its inverse
       *  are expressed as double float matrices.
       *
       *  TDM projection space spans -1..1 in X and Y. If the viewport
       *  depth is provided, use it; otherwise assume a 24 bit Z buffer
       *  range.
       *
       *  The original Picture widget code uses a left-handed coordinate
       *  system visible face computation, so Sz is negated below to
       *  compensate for API's with right-handed device coordinates.
       */
      float Sx, Sy ;

      /* copy view transform to double array */
      for (i=0 ; i<4 ; i++)
	for (j=0 ; j<4 ; j++)
	  PDATA(PureWtrans[i][j]) = (double)CDATA(viewXfm[i][j]) ;

      /* get projection matrix and concatenate with view */
      _dxfGetProjectionMatrix (I_PORT_HANDLE,CDATA(stack), projmat) ;
      _dxfMult44f (PDATA(PureWtrans), projmat) ;

      /* compute screen matrix; assume image size and offset same as window */
      COPYMATRIX(CDATA(scrnXfm), identity) ;

      Sx = ((float)CDATA(w)-1.)/2. ;
      Sy = ((float)CDATA(h)-1.)/2. ;
      /*Sz = CDATA(d) ? ((float)CDATA(d)-1.)/2. : (float)0xffffff/2. ;*/

      CDATA(scrnXfm[0][0]) =  Sx ;
      CDATA(scrnXfm[1][1]) =  Sy ;

#if 0
  {
  float fov, aspect;
  int proj ;
  static int FirstPass = 1;
  static int Sign = 0;

#if defined(DXD_HW_DC_RIGHT_HANDED)
#if defined(sgi) /* a hack to get around differences in sgi DX z handedness */
      {
	char version[20];
	gversion(version);
	if(!strncmp(version,"GL4DXG",6)) {
	  CDATA(scrnXfm[2][2]) = -Sz ;
	} else {
	  CDATA(scrnXfm[2][2]) = Sz ;
	}
      }
#else
      CDATA(scrnXfm[2][2]) = -Sz ;
#endif
#else
      CDATA(scrnXfm[2][2]) =  Sz ;
#endif

#if defined(sgi)
   if(FirstPass)
   {
      char version[20];

      gversion(version);
      if(!strncmp(version,"GL4DXG",6))
         Sign = 0;
      else
         Sign = 1;

      if(getenv("DX_FORCE_Z_PLUS"))
      {
         Sign = 1;
	 printf("Right-Handed.\n");
      }
      else
      if(getenv("DX_FORCE_Z_MINUS"))
      {
         Sign = 0;
	 printf("Left-Handed.\n");
      }
      FirstPass = 0;
   }
#endif
  }
#endif

      CDATA(scrnXfm[3][0]) =  Sx + 0.5 ;
      CDATA(scrnXfm[3][1]) =  Sy + 0.5 ;
      CDATA(scrnXfm[3][2]) =  0.5 ;

      /* concatenate screen matrix and find inverse */
      _dxfMult44f (PDATA(PureWtrans), CDATA(scrnXfm)) ;
      _dxfInverse (PDATA(PureWItrans), PDATA(PureWtrans));
      
      PRINT(("view transform:"));
      MPRINT(CDATA(viewXfm)) ;
      PRINT(("projection transform:"));
      MPRINT(projmat) ;
      PRINT(("screen transform:"));
      MPRINT(CDATA(scrnXfm)) ;
      PRINT(("PureWtrans:"));
      MPRINT(PDATA(PureWtrans)) ;
      PRINT(("PureWItrans:"));
      MPRINT(PDATA(PureWItrans)) ;
    }

  EXIT((""));
}


static int
find_closest ( tdmInteractor I, int x, int y )
{
  int i;
  DEFDATA(I,CursorData) ;

  ENTRY(("find_closest(0x%x, %d, %d)", I, x, y));

  if (PDATA(mode) == XmCURSOR_MODE) {
    for ( i = 0 ; i < PDATA(n_cursors) ; i++ )
      {
	if ( (abs (PDATA(xbuff)[i] - x) < PDATA(cursor_size)/2+1) &&
	    (abs (PDATA(ybuff)[i] - y) < PDATA(cursor_size)/2+1) ) {
	  EXIT(("%d",i));
	  return i;
	}
      }
  } else if (PDATA(mode) == XmROAM_MODE) {
    if ( (abs (PDATA(roam_xbuff) - x) < PDATA(cursor_size)/2+1) &&
	(abs (PDATA(roam_ybuff) - y) < PDATA(cursor_size)/2+1) ) {
      EXIT(("0"));
      return 0;
    }
  }

  EXIT(("-1"));
  return -1;
}


static int
select_cursor ( tdmInteractor I, int id)
{
  DEFDATA(I,CursorData) ;

  ENTRY(("select_cursor(0x%x, %d)", I, id));

  if (PDATA(mode) == XmCURSOR_MODE)
    {
      PDATA(selected)[id] = TRUE;
    }
  else
    {
      PDATA(roam_selected) = TRUE;
    }

  EXIT((""));
  return OK;
}



static int
deselect_cursor (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  int i;

  ENTRY(("deselect_cursor(0x%x)", I));
  
  if (PDATA(mode) == XmCURSOR_MODE)
    {
      for ( i = 0 ; i < PDATA(n_cursors) ; i++ )
	{
	  PDATA(selected)[i] = FALSE;
	}
    }
  else
    {
      PDATA(roam_selected) = FALSE;
    }

  EXIT((""));
  return OK;
}

static int
create_cursor (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  int i, *itmp ;
  double *dtmp ;

  ENTRY(("create_cursor(0x%x)", I));

  /*
   * Realloc the appropriate arrays
   */

  itmp = (int *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(int));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      itmp[i] = PDATA(xbuff)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(xbuff));
    }
  PDATA(xbuff) = itmp;

  itmp = (int *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(int));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      itmp[i] = PDATA(ybuff)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(ybuff));
    }
  PDATA(ybuff) = itmp;

  dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(double));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      dtmp[i] = PDATA(zbuff)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(zbuff));
    }
  PDATA(zbuff) = dtmp;

  dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(double));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      dtmp[i] = PDATA(cxbuff)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(cxbuff));
    }
  PDATA(cxbuff) = dtmp;

  dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(double));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      dtmp[i] = PDATA(cybuff)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(cybuff));
    }
  PDATA(cybuff) = dtmp;

  dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(double));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      dtmp[i] = PDATA(czbuff)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(czbuff));
    }
  PDATA(czbuff) = dtmp;

  itmp = (int *)tdmAllocateLocal((PDATA(n_cursors) + 1)*sizeof(int));
  for (i = 0; i < PDATA(n_cursors); i++)
    {
      itmp[i] = PDATA(selected)[i];
    }
  if (PDATA(n_cursors) > 0)
    {
      tdmFree(PDATA(selected));
    }

  PDATA(selected) = itmp;

  PDATA(selected)[PDATA(n_cursors)] = FALSE;
  PDATA(n_cursors)++;


  EXIT(("id = %d)", PDATA(n_cursors) - 1));
  return (PDATA(n_cursors) - 1);
}

static int
delete_cursor ( tdmInteractor I, int id )
{
/*
 *  GL context must already be configured for cursor mode if visible.
 */

  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int		*itmp;
  double	*dtmp;
  int		i, k ;

  ENTRY(("delete_cursor(0x%x, %d)", I, id ));

    PDATA(selected)[id] = FALSE;
    _dxf_ERASE_CURSOR (I, PDATA(xbuff[id]), PDATA(ybuff[id])) ;

    /*
     * Realloc the arrays
     */

    /*
     * xbuff
     */
    itmp = (int *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(int));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
	if (id != i) itmp[k++] = PDATA(xbuff)[i];
	}
    tdmFree(PDATA(xbuff));
    PDATA(xbuff) = itmp;

    /*
     * ybuff
     */
    itmp = (int *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(int));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
	if (id != i) itmp[k++] = PDATA(ybuff)[i];
	}
    tdmFree(PDATA(ybuff));
    PDATA(ybuff) = itmp;

    /*
     * zbuff
     */
    dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
	if (id != i) dtmp[k++] = PDATA(zbuff)[i];
	}
    tdmFree(PDATA(zbuff));
    PDATA(zbuff) = dtmp;

    /*
     * cxbuff
     */
    dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
        if (id != i) dtmp[k++] = PDATA(cxbuff)[i];
	}
    tdmFree(PDATA(cxbuff));
    PDATA(cxbuff) = dtmp;

    /*
     * cybuff
     */
    dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
        if (id != i) dtmp[k++] = PDATA(cybuff)[i];
	}
    tdmFree(PDATA(cybuff));
    PDATA(cybuff) = dtmp;

    /*
     * czbuff
     */
    dtmp = (double *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(double));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
        if (id != i) dtmp[k++] = PDATA(czbuff)[i];
	}
    tdmFree(PDATA(czbuff));
    PDATA(czbuff) = dtmp;

    /*
     * selected
     */
    itmp = (int *)tdmAllocateLocal((PDATA(n_cursors) - 1)*sizeof(int));
    k = 0;
    for (i = 0; i < PDATA(n_cursors); i++)
	{
        if (id != i) itmp[k++] = PDATA(selected)[i];
	}
    tdmFree(PDATA(selected));
    PDATA(selected) = itmp;

    PDATA(n_cursors)--;
    draw_cursors(I);

  EXIT((""));

  return OK;
}

static void
constrain (tdmInteractor I,
	   double *s0, double *s1, double *s2,
	   double dx, double dy, double dz,
	   double WItrans[4][4], double Wtrans[4][4],
	   int sdx, int sdy)
{
  DEFDATA(I,CursorData) ;
  double	sxcoor;
  double	sycoor;
  double	szcoor;
  double	original_dist;
  double	new_dist;
  double	normalize;
  double 	x, y, z;
  double 	sav_x, sav_y, sav_z;
  double  old_x, old_y, old_z;

  ENTRY(("constrain(0x%x, 0x%x, 0x%x, 0x%x, %f, %f, %f, 0x%x, 0x%x, %d, %d)",
	 I, s0, s1, s2, dx, dy, dz, WItrans, Wtrans, sdx, sdy));

    /*
     * Remember the old positions
     */
    old_x = *s0;
    old_y = *s1;
    old_z = *s2;
    original_dist = sqrt(sdx*sdx + sdy*sdy);

    /* xform point back to canonical world coords */
    XFORM_COORDS(PDATA(WItrans), *s0, *s1, *s2, x, y, z) ;

    sav_x = x;
    sav_y = y;
    sav_z = z;

    /*
     * Start out by moving 1/100 of the distance in world coords.
     */
    x += dx*(PDATA(FacetXmax) - PDATA(FacetXmin))/100;
    y += dy*(PDATA(FacetYmax) - PDATA(FacetYmin))/100;
    z += dz*(PDATA(FacetZmax) - PDATA(FacetZmin))/100;

    /*
     * Now, xform back to screen coords to see how far we have moved.
     */
    XFORM_COORDS(PDATA(Wtrans), x, y, z, sxcoor, sycoor, szcoor) ;

    /*
     * See how far we have moved.
     */
    new_dist = sqrt((sxcoor - PDATA(pcx))*(sxcoor - PDATA(pcx)) +
		    (sycoor - PDATA(pcy))*(sycoor - PDATA(pcy)));

    if (new_dist != 0.0)
	{
	normalize = original_dist/new_dist;
	dx = dx * normalize;
	dy = dy * normalize;
	dz = dz * normalize;
	}
    /*
     * Modify the deltas as appropriate.
     */
    if (CDATA(cursor_speed) == tdmSLOW_CURSOR)
	{
	dx = dx*(PDATA(FacetXmax) - PDATA(FacetXmin))/300;
	dy = dy*(PDATA(FacetYmax) - PDATA(FacetYmin))/300;
	dz = dz*(PDATA(FacetZmax) - PDATA(FacetZmin))/300;
	}
    if (CDATA(cursor_speed) == tdmMEDIUM_CURSOR)
	{
	dx = dx*(PDATA(FacetXmax) - PDATA(FacetXmin))/100;
	dy = dy*(PDATA(FacetYmax) - PDATA(FacetYmin))/100;
	dz = dz*(PDATA(FacetZmax) - PDATA(FacetZmin))/100;
	}
    if (CDATA(cursor_speed) == tdmFAST_CURSOR)
	{
	dx = dx*(PDATA(FacetXmax) - PDATA(FacetXmin))/33;
	dy = dy*(PDATA(FacetYmax) - PDATA(FacetYmin))/33;
	dz = dz*(PDATA(FacetZmax) - PDATA(FacetZmin))/33;
	}

    /*
     * DXApply the new deltas.
     */
    x = dx + sav_x;
    y = dy + sav_y;
    z = dz + sav_z;

    /* adjust the point so it lies w/in the bounding box */
    if (x < 0.0)
	{
	x = 0.0;
	}
    if (y < 0.0)
	{
	y = 0.0;
	}
    if (z < 0.0)
	{
	z = 0.0;
	}
    if (x > PDATA(FacetXmax))
	{
	x = PDATA(FacetXmax);
	}
    if (y > PDATA(FacetYmax))
	{
	y = PDATA(FacetYmax);
	}
    if (z > PDATA(FacetZmax))
	{
	z = PDATA(FacetZmax);
	}

    /* xform the point back to image coord system */
    XFORM_COORDS(PDATA(Wtrans), x, y, z, *s0, *s1, *s2) ;

    /*
     * If we have attempted to move out of the widow, throw away the
     * change by restoring the original position.
     */
    if ( (*s0 < 0.0) || (*s0 > CDATA(w) - PDATA(cursor_size)) ||
	 (*s1 < 0.0) || (*s1 > CDATA(h) - PDATA(cursor_size)) )
	{
	*s0 = old_x;
	*s1 = old_y;
	*s2 = old_z;
	}

  EXIT((""));
}


static void
save_cursors_in_canonical_form (tdmInteractor I, int i)
{
  DEFDATA(I,CursorData) ;

  ENTRY(("save_cursors_in_canonical_form(0x%x, %d)", I, i));

  if (PDATA(mode) == XmCURSOR_MODE)
    {
      XFORM_COORDS(PDATA(PureWItrans),
		   PDATA( xbuff[i]), PDATA( ybuff[i]), PDATA( zbuff[i]),
		   PDATA(cxbuff[i]), PDATA(cybuff[i]), PDATA(czbuff[i])) ;
    }
  else if (PDATA(mode) == XmROAM_MODE)
    {
      XFORM_COORDS(PDATA(PureWItrans),
		   PDATA(roam_xbuff),
		   PDATA(roam_ybuff),
		   PDATA(roam_zbuff),
		   PDATA(roam_cxbuff),
		   PDATA(roam_cybuff),
		   PDATA(roam_czbuff)) ;
    }

  EXIT((""));
}

static void
restore_cursors_from_canonical_form (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  double x, y ;
  int i ;

  ENTRY(("restore_cursors_from_canonical_form(0x%x)", I));

  /* xform point back from canonical coords */
  for (i = 0 ; i < PDATA(n_cursors) ; i++)
    {
      XFORM_COORDS(PDATA(PureWtrans),
		   PDATA(cxbuff[i]), PDATA(cybuff[i]), PDATA(czbuff[i]),
		   x, y, PDATA(zbuff[i])) ;

      PDATA(xbuff)[i] = (int) x ;
      PDATA(ybuff)[i] = (int) y ;
    }

  XFORM_COORDS(PDATA(PureWtrans),
	       PDATA(roam_cxbuff), PDATA(roam_cybuff), PDATA(roam_czbuff),
	       x, y, PDATA(roam_zbuff)) ;

  PDATA(roam_xbuff) = (int) x ;
  PDATA(roam_ybuff) = (int) y ;

  EXIT((""));
}

static double
line_of_sight (tdmInteractor I, double s0, double s1, int *newbox)
{
  DEFDATA(I,CursorData) ;
  double x, y, z, xmin, ymin, zmin ;
  double lx, ly, lz, t, xt, yt, zt, tmax, tmin, tmark[6] ;
  int i, j ;

  ENTRY(("line_of_sight(0x%x, %f, %f, 0x%x)", I, s0, s1, newbox));

#ifdef TDM_READ_ZBUFFER /* prototype code: works on GL/Sabine platform */
#include <gl.h>
  long zbuff[25] = {0}, ibuff[25] = {0} ;
  double ztemp ;

  /* read z buffer under cursor */
  readsource(SRC_ZBUFFER) ;
  lrectread ((int)s0-2, (int)s1-2, (int)s0+2, (int)s1+2, zbuff) ;

  /* read image under cursor */
  readsource(SRC_AUTO) ;
  lrectread ((int)s0-2, (int)s1-2, (int)s0+2, (int)s1+2, ibuff) ;

  for (ztemp = 0.0, j = 0, i = 0 ; i < 25 ; i++)
      /*
       *  Only count depth values associated with non-zero pixels, since
       *  the Sabine clears the Z buffer with a dirty bit.  Pixels that
       *  aren't touched have undefined depth values.
       */
      if (ibuff[i])
	{
	  /* interpret depth values as 24-bit signed integers and accumulate */
	  if (zbuff[i] > 0x7fffff)
	      ztemp += zbuff[i]-0x1000000 ;
	  else
	      ztemp += zbuff[i] ;
	  j++ ;
	}

  /* take mean and negate to compensate for -Sz factor in screen transform */
  if (j)
    {
      *newbox = 0 ;
      z = -ztemp / j ;
      EXIT(("z = %f",z));
      return z ;
    }
#endif

  /* use pixel Zmax and Zmin as depth, convert to canonical world */
  XFORM_COORDS(PDATA(WItrans), s0, s1, PDATA(Zmax), x, y, z) ;
  XFORM_COORDS(PDATA(WItrans), s0, s1, PDATA(Zmin), xmin, ymin, zmin) ;

  /*
   *  The line of sight equation is
   *
   *  X = x + t*lx ;
   *  Y = y + t*ly ;
   *  Z = z + t*ly ;
   *
   *  [lx,ly,lz] is the line of sight.  Substitute bounding box faces for
   *  X, Y, Z and solve for t.  Record t if resulting point is within face
   *  edges.
   */

  i = 0 ;
  lx = xmin - x ;
  ly = ymin - y ;
  lz = zmin - z ;

  if (lx != 0.0)
    {
      t  = (PDATA(Face2)[0].xcoor - x) / lx ;
      yt = y + t*ly ; zt = z + t*lz ;

      if ((yt >= 0.0) && (yt <= PDATA(FacetYmax)) &&
	  (zt >= 0.0) && (zt <= PDATA(FacetZmax)))

	  tmark[i++] = t ;

      t  = (PDATA(Face2)[3].xcoor - x) / lx ;
      yt = y + t*ly ; zt = z + t*lz ;

      if ((yt >= 0.0) && (yt <= PDATA(FacetYmax)) &&
	  (zt >= 0.0) && (zt <= PDATA(FacetZmax)))

	  tmark[i++] = t ;
    }

  if (ly != 0.0)
    {
      t  = (PDATA(Face1)[0].ycoor - y) / ly ;
      xt = x + t*lx ; zt = z + t*lz ;

      if ((xt >= 0.0) && (xt <= PDATA(FacetXmax)) &&
	  (zt >= 0.0) && (zt <= PDATA(FacetZmax)))

	  tmark[i++] = t ;

      t  = (PDATA(Face2)[0].ycoor - y) / ly ;
      xt = x + t*lx ; zt = z + t*lz ;

      if ((xt >= 0.0) && (xt <= PDATA(FacetXmax)) &&
	  (zt >= 0.0) && (zt <= PDATA(FacetZmax)))

	  tmark[i++] = t ;
    }

  if (lz != 0.0)
    {
      t  = (PDATA(Face1)[1].zcoor - z) / lz ;
      xt = x + t*lx ; yt = y + t*ly ;

      if ((xt >= 0.0) && (xt <= PDATA(FacetXmax)) &&
	  (yt >= 0.0) && (yt <= PDATA(FacetYmax)))

	  tmark[i++] = t ;

      t  = (PDATA(Face1)[0].zcoor - z) / lz ;
      xt = x + t*lx ; yt = y + t*ly ;

      if ((xt >= 0.0) && (xt <= PDATA(FacetXmax)) &&
	  (yt >= 0.0) && (yt <= PDATA(FacetYmax)))

	  tmark[i++] = t ;
    }

  if (i == 0)
    {
      /* didn't hit any faces */
      *newbox = 1 ;
      t = 0.5 ;
    }
  else
    {
      /* got a hit */
      *newbox = 0 ;
      tmax = 0 ; tmin = 1 ;

      /* find low and high marks, take midpoint */
      for (j = 0 ; j < i ; j++)
	{
	  if (tmark[j] > tmax)
	      tmax = tmark[j] ;

	  if (tmark[j] < tmin)
	      tmin = tmark[j] ;
	}
      t = (tmin + tmax) / 2 ;
    }

  /* convert to screen */
  xt = x + t*lx ; yt = y + t*ly ; zt = z + t*lz ;
  XFORM_COORDS(PDATA(Wtrans), xt, yt, zt, x, y, z) ;
  
  EXIT(("z = %f", z));
  return z ;
}

static void
project (tdmInteractor I, double s0, double s1, double s2,
	 double Wtrans[4][4], double WItrans[4][4])
{
  DEFDATA(I,CursorData) ;
  int  i = -1;
  double x, y, z;

  ENTRY(("project(0x%x, %f, %f, %f, 0x%x, 0x%x)",
	 I, s0, s1, s2,  Wtrans, WItrans));

    XFORM_COORDS(PDATA(WItrans), s0, s1, s2, x, y, z) ;

    if ( (PDATA(Face1)[0].visible == TRUE) &&
	 (PDATA(Face1)[1].visible == TRUE) &&
	 (PDATA(Face1)[2].visible == TRUE) &&
	 (PDATA(Face1)[3].visible == TRUE) )
	{
	i++;
	XFORM_COORDS(PDATA(Wtrans), x, PDATA(Face1[0].ycoor), z,
		     PDATA(Xmark[i]), PDATA(Ymark[i]), PDATA(Zmark[i])) ;
	}

    if ( (PDATA(Face2)[0].visible == TRUE) &&
	 (PDATA(Face2)[1].visible == TRUE) &&
	 (PDATA(Face2)[2].visible == TRUE) &&
	 (PDATA(Face2)[3].visible == TRUE) )
	{
	i++;
	XFORM_COORDS(PDATA(Wtrans), x, 0, z,
		     PDATA(Xmark[i]), PDATA(Ymark[i]), PDATA(Zmark[i])) ;
	}


    if ( (PDATA(Face1)[0].visible == TRUE) &&
	 (PDATA(Face1)[1].visible == TRUE) &&
	 (PDATA(Face2)[1].visible == TRUE) &&
	 (PDATA(Face2)[0].visible == TRUE) )
	{
	i++;
	XFORM_COORDS(PDATA(Wtrans), 0, y, z,
		     PDATA(Xmark[i]), PDATA(Ymark[i]), PDATA(Zmark[i])) ;
	}

    if ( (PDATA(Face1)[2].visible == TRUE) &&
	 (PDATA(Face1)[3].visible == TRUE) &&
	 (PDATA(Face2)[2].visible == TRUE) &&
	 (PDATA(Face2)[3].visible == TRUE) )
	{
	i++;
	XFORM_COORDS(PDATA(Wtrans), PDATA(Face2[3].xcoor), y, z,
		     PDATA(Xmark[i]), PDATA(Ymark[i]), PDATA(Zmark[i])) ;
	}

    if ( (PDATA(Face1)[1].visible == TRUE) &&
	 (PDATA(Face1)[2].visible == TRUE) &&
	 (PDATA(Face2)[1].visible == TRUE) &&
	 (PDATA(Face2)[2].visible == TRUE) )
	{
	i++;
	XFORM_COORDS(PDATA(Wtrans), x, y, PDATA(Face2[1].zcoor),
		     PDATA(Xmark[i]), PDATA(Ymark[i]), PDATA(Zmark[i])) ;
	}

    if ( (PDATA(Face1)[0].visible == TRUE) &&
	 (PDATA(Face1)[3].visible == TRUE) &&
	 (PDATA(Face2)[0].visible == TRUE) &&
	 (PDATA(Face2)[3].visible == TRUE) )
	{
	i++;
	XFORM_COORDS(PDATA(Wtrans), x, y, 0,
		     PDATA(Xmark[i]), PDATA(Ymark[i]), PDATA(Zmark[i])) ;
	}
    PDATA(iMark) = i + 1;

  EXIT((""));
}

static int add2historybuffer(tdmInteractor I, int x, int y)
{
  DEFDATA(I,CursorData) ;

  ENTRY(("add2historybuffer(0x%x, %d, %d)", I, x, y));
  
    /* Shuffle the points back */
    PDATA(pppppppx) = PDATA(ppppppx);
    PDATA(pppppppy) = PDATA(ppppppy);

    PDATA(ppppppx) = PDATA(pppppx);
    PDATA(ppppppy) = PDATA(pppppy);

    PDATA(pppppx) = PDATA(ppppx);
    PDATA(pppppy) = PDATA(ppppy);

    PDATA(ppppx) = PDATA(pppx);
    PDATA(ppppy) = PDATA(pppy);

    PDATA(pppx) = PDATA(ppx);
    PDATA(pppy) = PDATA(ppy);

    PDATA(ppx) = PDATA(px);
    PDATA(ppy) = PDATA(py);

    PDATA(px) = x;
    PDATA(py) = y;
    if (PDATA(K) < 8) PDATA(K)++;

  EXIT((""));
  return OK;
}


/*  Subroutine:	calc_projected_axis
 *  Purpose:    For Cursor Mode 2 - calculate the angles of the projected
 * 		x, y, and x axes.  The calculated angles represent the
 *		angle off the screen X axis that is create while moving
 *		along the associated axis in the positive direction
 */
static void
calc_projected_axis(tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  double	x1;
  double  y1;
  double  x2;
  double  y2;
  double  delta_x;
  double  delta_y;
  double  hyp_x;
  double  hyp_y;
  double  hyp_z;
  ENTRY(("calc_projected_axis(0x%x)", I));

    /* X axis */
    x1 = PDATA(Face2)[0].sxcoor;
    y1 = PDATA(Face2)[0].sycoor;
    x2 = PDATA(Face2)[3].sxcoor;
    y2 = PDATA(Face2)[3].sycoor;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    hyp_x = sqrt(delta_x*delta_x + delta_y*delta_y);
    if (hyp_x != 0)
	{
	PDATA(angle_posx) = acos( delta_x/hyp_x);
	}
    else
	{
	PDATA(angle_posx) = -100.0;
	}
    if( delta_y  > 0 )
	{
	PDATA(angle_posx) = 2*M_PI - PDATA(angle_posx);
	}
    if (PDATA(angle_posx) < M_PI)
	{
	PDATA(angle_negx) = PDATA(angle_posx) + M_PI;
	}
    else
	{
	PDATA(angle_negx) = PDATA(angle_posx) - M_PI;
	}

    /* Y axis */
    x1 = PDATA(Face2)[0].sxcoor;
    y1 = PDATA(Face2)[0].sycoor;
    x2 = PDATA(Face1)[0].sxcoor;
    y2 = PDATA(Face1)[0].sycoor;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    hyp_y = sqrt(delta_x*delta_x + delta_y*delta_y);
    if (hyp_y != 0)
	{
	PDATA(angle_posy) = acos( delta_x/hyp_y);
	}
    else
	{
	PDATA(angle_posy) = -100.0;
	}
    if( delta_y  > 0 )
	{
	PDATA(angle_posy) = 2*M_PI - PDATA(angle_posy);
	}
    if (PDATA(angle_posy) < M_PI)
	{
	PDATA(angle_negy) = PDATA(angle_posy) + M_PI;
	}
    else
	{
	PDATA(angle_negy) = PDATA(angle_posy) - M_PI;
	}

    /* Z axis */
    x1 = PDATA(Face2)[0].sxcoor;
    y1 = PDATA(Face2)[0].sycoor;
    x2 = PDATA(Face2)[1].sxcoor;
    y2 = PDATA(Face2)[1].sycoor;
    delta_x = x2 - x1;
    delta_y = y2 - y1;
    hyp_z = sqrt(delta_x*delta_x + delta_y*delta_y);
    if (hyp_z != 0)
	{
	PDATA(angle_posz) = acos( delta_x/hyp_z);
	}
    else
	{
	PDATA(angle_posz) = -100.0;
	}
    if( delta_y  > 0 )
	{
	PDATA(angle_posz) = 2*M_PI - PDATA(angle_posz);
	}
    if (PDATA(angle_posz) < M_PI)
	{
	PDATA(angle_negz) = PDATA(angle_posz) + M_PI;
	}
    else
	{
	PDATA(angle_negz) = PDATA(angle_posz) - M_PI;
	}

    /* Check to see if the projected axis have similar angles.  If they are
     * very close, disable the one with the shortest projected length. Also,
     * disable movement along an axis if the length of the projected axis
     * is less than 10 pixels.
     */
    PDATA(x_movement_allowed) = TRUE;
    PDATA(y_movement_allowed) = TRUE;
    PDATA(z_movement_allowed) = TRUE;

    if ( (fabs(PDATA(angle_posx) - PDATA(angle_posy)) < 0.2) ||
	 (fabs(PDATA(angle_posx) - PDATA(angle_negy)) < 0.2) )
	{
	if ( hyp_x < hyp_y)
	    {
	    PDATA(x_movement_allowed) = FALSE;
	    }
	else
	    {
	    PDATA(y_movement_allowed) = FALSE;
	    }
	}

    if ( (fabs(PDATA(angle_posx) - PDATA(angle_posz)) < 0.2) ||
	 (fabs(PDATA(angle_posx) - PDATA(angle_negz)) < 0.2) )
	{
	if ( hyp_x < hyp_z)
	    {
	    PDATA(x_movement_allowed) = FALSE;
	    }
	else
	    {
	    PDATA(z_movement_allowed) = FALSE;
	    }
	}

    if ( (fabs(PDATA(angle_posy) - PDATA(angle_posz)) < 0.2) ||
	 (fabs(PDATA(angle_posy) - PDATA(angle_negz)) < 0.2) )
	{
	if ( hyp_y < hyp_z)
	    {
	    PDATA(y_movement_allowed) = FALSE;
	    }
	else
	    {
	    PDATA(z_movement_allowed) = FALSE;
	    }
	}
    if (hyp_x < 10)
	{
	PDATA(x_movement_allowed) = FALSE;
	}
    if (hyp_y < 10)
	{
	PDATA(y_movement_allowed) = FALSE;
	}
    if (hyp_z < 10)
	{
	PDATA(z_movement_allowed) = FALSE;
	}

  EXIT((""));
}

#define TRANSFORM4D(M, x, y, z, px, py, pz, pw, tx, ty, tz, sx, sy)	\
{									\
  px = x*M[0][0] + y*M[1][0] + z*M[2][0] + M[3][0] ;			\
  py = x*M[0][1] + y*M[1][1] + z*M[2][1] + M[3][1] ;			\
  pz = x*M[0][2] + y*M[1][2] + z*M[2][2] + M[3][2] ;			\
  pw = x*M[0][3] + y*M[1][3] + z*M[2][3] + M[3][3] ;			\
  if (pw != 0.0) {tx = px/pw ; ty = py/pw ; tz = pz/pw ;}		\
  sx = tx ; sy = ty ;							\
									\
  if (tz > PDATA(Zmax)) PDATA(Zmax) = tz ;				\
  if (tz < PDATA(Zmin)) PDATA(Zmin) = tz ;				\
									\
  PRINT(("%15f %15f %15f --> %15f %15f %15f %15f",			\
	 x, y, z, tx, ty, tx, pw));					\
}									  

/*  Subroutine:	setup_bounding_box
 *  Purpose:	Get the bounding box coordinates in canonical form
 *              Note that Face1 and Face2 are parallel to the XY plane
 *
 *	The matrix Bw is assumed to be in the following format:
 *
 *		Ux   Uy   Uz   0
 *		Vx   Vy   Vz   0
 *		Wx   Wy   Wz   0
 *		Ox   Oy   Oz   1
 *
 * 	Where U, V and W are the endpoints of the axis of the bounding box
 *	and O is the origin of the bounding box.  To get the bounding box
 *	in canonical form, it must be translated to the origin of the world
 *	coordinate system.  This is done by subtracting Ox, Oy and Oz from
 *	the associated components of the other points.
 */
static void
setup_bounding_box(tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  double x, y, z, nz ;
  double vdelta_x, vdelta_y, vdelta_z ;
  double wdelta_x, wdelta_y, wdelta_z ;
  double xmax, xmin, ymax, ymin, zmax, zmin ;
  int i ;
  double xlen, ylen, zlen, maxlen ;

  ENTRY(("setup_bounding_box(0x%x)", I));

  if (PDATA(basis) == NULL)
    {
      EXIT(("PDATA(basis) is NULL"));
      return ;
    }

  PDATA(Zmax) = -DXD_MAX_DOUBLE ;
  PDATA(Zmin) =  DXD_MAX_DOUBLE ;

  xmax = ymax = zmax = -DXD_MAX_DOUBLE ;
  xmin = ymin = zmin =  DXD_MAX_DOUBLE ;

  xmin = MIN(xmin, PDATA(basis)->Bw[0][0]) ;
  xmin = MIN(xmin, PDATA(basis)->Bw[1][0]) ;
  xmin = MIN(xmin, PDATA(basis)->Bw[2][0]) ;
  xmin = MIN(xmin, PDATA(basis)->Bw[3][0]) ;

  ymin = MIN(ymin, PDATA(basis)->Bw[0][1]) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[1][1]) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[2][1]) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[3][1]) ;

  zmin = MIN(zmin, PDATA(basis)->Bw[0][2]) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[1][2]) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[2][2]) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[3][2]) ;

  xmax = MAX(xmax, PDATA(basis)->Bw[0][0]) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[1][0]) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[2][0]) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[3][0]) ;

  ymax = MAX(ymax, PDATA(basis)->Bw[0][1]) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[1][1]) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[2][1]) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[3][1]) ;

  zmax = MAX(zmax, PDATA(basis)->Bw[0][2]) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[1][2]) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[2][2]) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[3][2]) ;

  /*udelta_x = PDATA(basis)->Bw[0][0] - PDATA(basis)->Bw[3][0] ;*/
  /*udelta_y = PDATA(basis)->Bw[0][1] - PDATA(basis)->Bw[3][1] ;*/
  /*udelta_z = PDATA(basis)->Bw[0][2] - PDATA(basis)->Bw[3][2] ;*/

  vdelta_x = PDATA(basis)->Bw[1][0] - PDATA(basis)->Bw[3][0] ;
  vdelta_y = PDATA(basis)->Bw[1][1] - PDATA(basis)->Bw[3][1] ;
  vdelta_z = PDATA(basis)->Bw[1][2] - PDATA(basis)->Bw[3][2] ;

  wdelta_x = PDATA(basis)->Bw[2][0] - PDATA(basis)->Bw[3][0] ;
  wdelta_y = PDATA(basis)->Bw[2][1] - PDATA(basis)->Bw[3][1] ;
  wdelta_z = PDATA(basis)->Bw[2][2] - PDATA(basis)->Bw[3][2] ;

  /*
   * U + V
   */
  xmin = MIN(xmin, PDATA(basis)->Bw[0][0] + vdelta_x) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[0][0] + vdelta_x) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[0][1] + vdelta_y) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[0][1] + vdelta_y) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[0][2] + vdelta_z) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[0][2] + vdelta_z) ;

  /*
   * U + W
   */
  xmin = MIN(xmin, PDATA(basis)->Bw[0][0] + wdelta_x) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[0][0] + wdelta_x) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[0][1] + wdelta_y) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[0][1] + wdelta_y) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[0][2] + wdelta_z) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[0][2] + wdelta_z) ;

  /*
   * V + W
   */
  xmin = MIN(xmin, PDATA(basis)->Bw[1][0] + wdelta_x) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[1][0] + wdelta_x) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[1][1] + wdelta_y) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[1][1] + wdelta_y) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[1][2] + wdelta_z) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[1][2] + wdelta_z) ;

  /*
   * U + V + W
   */
  xmin = MIN(xmin, PDATA(basis)->Bw[0][0] + vdelta_x + wdelta_x) ;
  xmax = MAX(xmax, PDATA(basis)->Bw[0][0] + vdelta_x + wdelta_x) ;
  ymin = MIN(ymin, PDATA(basis)->Bw[0][1] + vdelta_y + wdelta_y) ;
  ymax = MAX(ymax, PDATA(basis)->Bw[0][1] + vdelta_y + wdelta_y) ;
  zmin = MIN(zmin, PDATA(basis)->Bw[0][2] + vdelta_z + wdelta_z) ;
  zmax = MAX(zmax, PDATA(basis)->Bw[0][2] + vdelta_z + wdelta_z) ;

  /*
   * Grow the bounding box to include the current set of cursors.
   */
  if (PDATA(mode) == XmCURSOR_MODE)
    {
      for (i = 0 ; i < PDATA(n_cursors) ; i++)
	{
	  xmin = MIN(xmin, PDATA(cxbuff)[i]) ;
	  xmax = MAX(xmax, PDATA(cxbuff)[i]) ;
	  ymin = MIN(ymin, PDATA(cybuff)[i]) ;
	  ymax = MAX(ymax, PDATA(cybuff)[i]) ;
	  zmin = MIN(zmin, PDATA(czbuff)[i]) ;
	  zmax = MAX(zmax, PDATA(czbuff)[i]) ;
	}
    }
  else if (PDATA(mode) == XmROAM_MODE && PDATA(roam_valid))
    {
      xmin = MIN(xmin, PDATA(roam_cxbuff)) ;
      xmax = MAX(xmax, PDATA(roam_cxbuff)) ;
      ymin = MIN(ymin, PDATA(roam_cybuff)) ;
      ymax = MAX(ymax, PDATA(roam_cybuff)) ;
      zmin = MIN(zmin, PDATA(roam_czbuff)) ;
      zmax = MAX(zmax, PDATA(roam_czbuff)) ;
    }

  /*
   * Handle 2D images by forcing a minimum value for each dimension
   */
  xlen = xmax - xmin ;
  ylen = ymax - ymin ;
  zlen = zmax - zmin ;
  maxlen = MAX(xlen, ylen) ;
  maxlen = MAX(maxlen, zlen) ;

  if (xmin == xmax)
    {
      xmax = xmin + maxlen/2000 ;
    }
  if (ymin == ymax)
    {
      ymax = ymin + maxlen/2000 ;
    }
  if (zmin == zmax)
    {
      zmax = zmin + maxlen/2000 ;
    }

  /*
   * Save the bounding box coords in cannonical form.
   */
  PDATA(FacetXmax) = xmax - xmin ;
  PDATA(FacetYmax) = ymax - ymin ;
  PDATA(FacetZmax) = zmax - zmin ;

  PDATA(FacetXmin) =  0.0 ;
  PDATA(FacetYmin) =  0.0 ;
  PDATA(FacetZmin) =  0.0 ;

  PDATA(Face1)[0].xcoor = 0.0 ;
  PDATA(Face1)[0].ycoor = PDATA(FacetYmax) ;
  PDATA(Face1)[0].zcoor = 0.0 ;

  PDATA(Face1)[1].xcoor = 0.0 ;
  PDATA(Face1)[1].ycoor = PDATA(FacetYmax) ;
  PDATA(Face1)[1].zcoor = PDATA(FacetZmax) ;

  PDATA(Face1)[2].xcoor = PDATA(FacetXmax) ;
  PDATA(Face1)[2].ycoor = PDATA(FacetYmax) ;
  PDATA(Face1)[2].zcoor = PDATA(FacetZmax) ;

  PDATA(Face1)[3].xcoor = PDATA(FacetXmax) ;
  PDATA(Face1)[3].ycoor = PDATA(FacetYmax) ;
  PDATA(Face1)[3].zcoor = 0.0 ;

  PDATA(Face2)[0].xcoor = 0.0 ;
  PDATA(Face2)[0].ycoor = 0.0 ;
  PDATA(Face2)[0].zcoor = 0.0 ;

  PDATA(Face2)[1].xcoor = 0.0 ;
  PDATA(Face2)[1].ycoor = 0.0 ;
  PDATA(Face2)[1].zcoor = PDATA(FacetZmax) ;

  PDATA(Face2)[2].xcoor = PDATA(FacetXmax) ;
  PDATA(Face2)[2].ycoor = 0.0 ;
  PDATA(Face2)[2].zcoor = PDATA(FacetZmax) ;

  PDATA(Face2)[3].xcoor = PDATA(FacetXmax) ;
  PDATA(Face2)[3].ycoor = 0.0 ;
  PDATA(Face2)[3].zcoor = 0.0 ;

  /*
   * Save the origin of the bounding box for CallCursorCallbacks.
   */
  PDATA(Ox) = xmin ;
  PDATA(Oy) = ymin ;
  PDATA(Oz) = zmin ;

  /*
   * Build matrix for cannonical form transformations.
   */
  DMCOPY(PDATA(Wtrans), dIdentity) ;

  /* Pre-concatenate the translation to canonical form */
  PDATA(Wtrans[3][0]) = xmin ;
  PDATA(Wtrans[3][1]) = ymin ;
  PDATA(Wtrans[3][2]) = zmin ;

  _dxfMult44 (PDATA(Wtrans), PDATA(PureWtrans)) ;
  _dxfInverse (PDATA(WItrans), PDATA(Wtrans)) ;

  PRINT(("Face1 canonical WC -> DC: "));
  /* transform the canonical form into projection and screen coordinates */
  for ( i = 0 ; i < 4 ; i++ )
    {
      TRANSFORM4D(PDATA(Wtrans),
		  PDATA(Face1[i].xcoor),
		  PDATA(Face1[i].ycoor),
		  PDATA(Face1[i].zcoor),
		  PDATA(Face1[i].pxcoor),
		  PDATA(Face1[i].pycoor),
		  PDATA(Face1[i].pzcoor),
		  PDATA(Face1[i].pwcoor),
		  PDATA(Face1[i].txcoor),
		  PDATA(Face1[i].tycoor),
		  PDATA(Face1[i].tzcoor),
		  PDATA(Face1[i].sxcoor),
		  PDATA(Face1[i].sycoor)) ;
    }

  PRINT(("Face2 canonical WC -> DC: "));

  for ( i = 0 ; i < 4 ; i++ )
    {
      TRANSFORM4D(PDATA(Wtrans),
		  PDATA(Face2[i].xcoor),
		  PDATA(Face2[i].ycoor),
		  PDATA(Face2[i].zcoor),
		  PDATA(Face2[i].pxcoor),
		  PDATA(Face2[i].pycoor),
		  PDATA(Face2[i].pzcoor),
		  PDATA(Face2[i].pwcoor),
		  PDATA(Face2[i].txcoor),
		  PDATA(Face2[i].tycoor),
		  PDATA(Face2[i].tzcoor),
		  PDATA(Face2[i].sxcoor),
		  PDATA(Face2[i].sycoor)) ;
    }

  PRINT(("Zmin: %f Zmax: %f", PDATA(Zmin), PDATA(Zmax)));

  /* remove the hidden lines of the bounding box */
  /* Shoot a ray from the eye to a vertex. Use a z value that
     is slightly closer to the eye than the vertex.  Then, xform the point
     back into cannonical form.  If the point is outside the box,
     then the vertex is visible */
  for ( i = 0 ; i < 4 ; i++ )
    {

      /* see which vertex is visible */
      nz = (double)(PDATA(Face1)[i].tzcoor +
		    ((PDATA(Zmax) - PDATA(Zmin)) / 5000.0)) ;

      XFORM_COORDS(PDATA(WItrans),
		   PDATA(Face1[i].txcoor), PDATA(Face1[i].tycoor), nz,
		   x, y, z) ;

      /* the x, y, z are now in the original coordinate system
	 see if they are inside the box */
      if ( (x >= PDATA(FacetXmin)) && (x <= PDATA(FacetXmax)) &&
	  (y >= PDATA(FacetYmin)) && (y <= PDATA(FacetYmax)) &&
	  (z >= PDATA(FacetZmin)) && (z <= PDATA(FacetZmax)) )
	  PDATA(Face1)[i].visible = FALSE ;
      else
	  PDATA(Face1)[i].visible = TRUE ;

      nz = (double)(PDATA(Face2)[i].tzcoor +
		    ((PDATA(Zmax) - PDATA(Zmin)) / 5000.0)) ;

      XFORM_COORDS(PDATA(WItrans),
		   PDATA(Face2[i].txcoor), PDATA(Face2[i].tycoor), nz,
		   x, y, z) ;

      /* the x, y, z are now in the original coordinate system
	 see if they are inside the box */
      if ( (x >= PDATA(FacetXmin)) && (x <= PDATA(FacetXmax)) &&
	  (y >= PDATA(FacetYmin)) && (y <= PDATA(FacetYmax)) &&
	  (z >= PDATA(FacetZmin)) && (z <= PDATA(FacetZmax)) )
	  PDATA(Face2)[i].visible = FALSE ;
      else
	  PDATA(Face2)[i].visible = TRUE ;
    }

  EXIT((""));
}
