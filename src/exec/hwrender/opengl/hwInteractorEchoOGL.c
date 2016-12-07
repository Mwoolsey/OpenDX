/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/opengl/hwInteractorEchoOGL.c,v $

  These are the Open GL implementations of the direct interactor echos,
  along with miscellaneous utility routines.  Each function needs to be
  ported to the native graphics API of the target system in order to provide
  support for direct interactors.

  Applications such as DX can be ported as a first step without direct
  interactor facilities by stubbing out everything here.

\*---------------------------------------------------------------------------*/

#include "../hwDeclarations.h"
#include "../hwMatrix.h"
#include "../hwInteractor.h"
#include "../hwCursorInteractor.h"
#include "../hwRotateInteractor.h"
#include "../hwZoomInteractor.h"
#include "../hwGlobeEchoDef.h"
#include "../hwPortLayer.h"
#include "../hwWindow.h"
#include "../hwDebug.h"
#include "hwPortOGL.h"

static float WS[4][4] = {
  { 1,  0,  0,  0},
  { 0,  1,  0,  0},
  { 0,  0,  1,  0},
  {-1, -1,  0,  1}
};

static void _dxf_SET_WORLD_SCREEN (void *ctx, int w, int h)
{
  /*
   *  This routine can be ignored if the API can write directly to DC,
   *  otherwise, it must set up transforms for screen mode.
   *
   *  The caller of this routine is expected to restore the current
   *  model/view/projection/viewport transforms if necessary.
   */
  ENTRY(("_dxf_SET_WORLD_SCREEN(0x%x, %d, %d)",ctx, w, h));

  SET_WORLD_SCREEN(w, h) ;

  EXIT((""));
}


static void _dxf_READ_BUFFER (void *ctx, int llx, int lly,
			      int urx, int ury, void *buff)
{
  /*
   *  DX hardware renderer expects pixels to be packed into 32-bit integers,
   *  so we use GL_RGBA format with a type of GL_UNSIGNED_BYTE.  Default
   *  read buffer is GL_BACK, so we must explicitly set GL_FRONT.
   */
  ENTRY(("_dxf_READ_BUFFER (0x%x, %d, %d, %d, %d, 0x%x)",
	 ctx, llx, lly, urx, ury, buff));
#if !defined(alphax)
  /* 
   * On DEC alpha this call causes the HW window 
   * not to move -- *AND* it's removal seems to
   * have no effect.
   */
  glReadBuffer(GL_FRONT) ;
#endif

  glReadPixels (llx, lly, (urx-llx)+1, (ury-lly)+1,
		GL_RGBA, GL_UNSIGNED_BYTE, buff) ;

  EXIT((""));
}


static void _dxf_WRITE_BUFFER (void *ctx, int llx, int lly,
			       int urx, int ury, void *buff)
{
  DEFCONTEXT(ctx) ;

  /*
   *  DX hardware renderer expects pixels to be packed into 32-bit integers,
   *  so we use GL_RGBA format with a type of GL_UNSIGNED_BYTE.  Bitmaps
   *  and images are always placed at the current raster position, which
   *  must be expressed in model/view coordinates.
   */
  ENTRY(("_dxf_WRITE_BUFFER(0x%x, %d, %d, %d, %d, 0x%x)",
	 ctx, llx, lly, urx, ury, buff));

  SET_RASTER_SCREEN (llx, lly) ;
  glDrawPixels ((urx-llx)+1, (ury-lly)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;

  EXIT((""));
}

static void _dxf_BUFFER_CONFIG (void *ctx, void *image,
				int llx, int lly, int urx, int ury,
				int *CurrentDisplayMode,
				int *CurrentBufferMode,
				tdmInteractorRedrawMode redrawMode)
{
  /*
   *  Direct interactor echos use this and following routine to manage
   *  double buffering.
   */

  ENTRY(("_dxf_BUFFER_CONFIG(0x%x, 0x%x, %d, %d, %d, %d, 0x%x, 0x%x, %d)",
	 ctx, image, llx, lly, urx, ury,
	 CurrentDisplayMode, CurrentBufferMode, redrawMode));

  /* record current GL buffer mode */
  glGetIntegerv (GL_DRAW_BUFFER, CurrentBufferMode) ;
  PRINT(("value glGetIntegerv(GL_DRAW_BUFFER) = %d",CurrentBufferMode));
  
  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw) {
    PRINT(("%s", (redrawMode == tdmBothBufferDraw) ?
	   "tdmBothBufferDraw" : "tdmFrontBufferDraw"));
    /*
     *  tdmBothBufferDraw is supposed to draw into both the front and
     *  back buffers.  This is no longer necessary since we don't swap
     *  buffers when animating the gnomon and globe now.
     */
    glDrawBuffer(GL_FRONT);
  } else if (redrawMode == tdmBackBufferDraw ||
	     redrawMode == tdmViewEchoMode) {
    PRINT(("%s", (redrawMode == tdmViewEchoMode) ?
	   "tdmViewEchoMode" : "tdmBackBufferDraw"));
    
    /* tdmBackBufferDraw is no longer different from tdmViewEchoMode */
    glDrawBuffer(GL_BACK) ;
  } else
    PRINT(("ignoring redraw mode")) ;

  EXIT((""));
}


static void _dxf_BUFFER_RESTORE_CONFIG (void *ctx,
					int OrigDisplayMode,
					int OrigBufferMode,
					tdmInteractorRedrawMode redrawMode)
{
  /*
   *  Restore frame buffer configuration.
   */

  ENTRY(("_dxf_BUFFER_RESTORE_CONFIG (0x%x, %d, %d, %d)",
	 ctx, OrigDisplayMode, OrigBufferMode, redrawMode));

  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw) {

    PRINT(("current interactor redraw mode is \"%s\"",
	   (redrawMode == tdmFrontBufferDraw) ?
	   "tdmFrontBufferDraw" : "tdmBothBufferDraw"));
    /*
     *  Call glFlush() so everything written to the front buffer becomes
     *  visible.  There's no need to do this if we're currently writing
     *  to the back buffer, as an implicit flush occurs when swapping
     *  the back buffer to the front.
     */
    glFlush() ;
    
    if (OrigBufferMode == GL_BACK ||
	OrigBufferMode == GL_BACK_LEFT) {
      PRINT(("restoring to original back buffer draw"));
      glDrawBuffer(GL_BACK) ;
    } else {
      PRINT(("remaining in original front buffer draw"));
    }
  } else if (redrawMode == tdmBackBufferDraw ||
	     redrawMode == tdmViewEchoMode) {

    PRINT(("current interactor redraw mode is \"%s\"",
	   (redrawMode == tdmViewEchoMode) ?
	   "tdmViewEchoMode" : "tdmBackBufferDraw"));

    if (OrigBufferMode == GL_FRONT ||
	OrigBufferMode == GL_FRONT_LEFT) {
      PRINT(("restoring to original front buffer draw"));
      glDrawBuffer(GL_FRONT) ;
    } else {
      PRINT(("remaining in original back buffer draw"));
    }
  } else {
    PRINT(("ignoring redraw mode"));
  }

  EXIT((""));
}


/* sine of arrowhead join angle of 150 degrees */
#define SIN 0.5

/* cosine of arrowhead join angle of 150 degrees */
#define COS -0.8660254

/* length of arrowhead lines relative to axis length */
#define ALENGTH 0.20

#define DRAW_ARROW(axis)                                                    \
{									    \
  glBegin(GL_LINES) ;                                                       \
    glVertex2f (0, 0) ;                                                     \
    glVertex2fv(axis) ;                                                     \
  glEnd() ;                                                                 \
                                                                            \
  axc = axis[0] * COS ;                                                     \
  axs = axis[0] * SIN ;                                                     \
  ayc = axis[1] * COS ;                                                     \
  ays = axis[1] * SIN ;                                                     \
									    \
  glBegin(GL_LINE_STRIP) ;						    \
    glVertex2f (axis[0] + ALENGTH*(axc-ays), axis[1] + ALENGTH*(axs+ayc)) ; \
    glVertex2fv(axis) ;							    \
    glVertex2f (axis[0] + ALENGTH*(axc+ays), axis[1] + ALENGTH*(ayc-axs)) ; \
  glEnd() ;								    \
}


static void _dxf_DRAW_GNOMON (tdmInteractor I, void *udata,
			      float rot[4][4], int draw)
{
  /*
   *  draw == 1 to draw gnomon, draw == 0 to undraw.  This is done with
   *  two separate calls in order to support explicit erasure of edges for
   *  some implementations.  A draw is always preceded by an undraw and
   *  the pair of invocations is atomic.
   *
   *  Computations are done in normalized screen coordinates in order to
   *  render arrow heads conveniently.
   */
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int dummy = 0 ;
  float xaxis[2],  yaxis[2],  zaxis[2] ;
  float xlabel[2], ylabel[2], zlabel[2] ;
  float axc, ays, axs, ayc ;

  ENTRY(("_dxf_DRAW_GNOMON (0x%x, 0x%x, 0x%x, %d)",
	 I, udata, rot, draw));

  if (PDATA(font) == -1) {
    PDATA(font) = 0 ;

    /* 1 pixel in normalized coordinates */
    PDATA(nudge) = 1.0/(float)GNOMONRADIUS ;
    
    /* approx. font offset for axes labels in normalized coordinates */
    PDATA(swidth) = 10 * PDATA(nudge) ;
  }

  if (draw) {
    glColor3f(1.0, 1.0, 1.0) ;
    glLineWidth(1.0) ;
  } else {
    if (PDATA(redrawmode) != tdmViewEchoMode) {
      /*
       *  In tdmViewEchoMode (DX's Execute On Change), we are drawing
       *  the gnomon echo on top of a background image that is redrawn
       *  with every frame of a direct interaction.
       *
       *  If we're not in that mode, the background image is static
       *  while the gnomon echo rotates in front of it, so erasing the
       *  gnomon means we have to repair damage to the background.  We
       *  do this by blitting a portion of the static image to the
       *  back buffer, drawing the gnomon over that, then blitting the
       *  combined results back to the front buffer.
       */

      /* force graphics output into back buffer */
      glDrawBuffer(GL_BACK) ;

      /* erase gnomon background */
      _dxf_WRITE_BUFFER (PORT_CTX, PDATA(illx), PDATA(illy),
			 PDATA(iurx), PDATA(iury), PDATA(background)) ;
    }

    /* draw wide black lines to ensure visibility against background */
    glColor3f(0.0, 0.0, 0.0) ;
    glLineWidth(3.0) ;
  }

  xaxis[0] = 0.7 * rot[0][0] ; xaxis[1] = 0.7 * rot[0][1] ;
  yaxis[0] = 0.7 * rot[1][0] ; yaxis[1] = 0.7 * rot[1][1] ;
  zaxis[0] = 0.7 * rot[2][0] ; zaxis[1] = 0.7 * rot[2][1] ;

  xlabel[0] = 0.8 * rot[0][0] ; xlabel[1] = 0.8 * rot[0][1] ;
  ylabel[0] = 0.8 * rot[1][0] ; ylabel[1] = 0.8 * rot[1][1] ;
  zlabel[0] = 0.8 * rot[2][0] ; zlabel[1] = 0.8 * rot[2][1] ;

  glPushMatrix() ;
  glLoadIdentity() ;

  DRAW_ARROW(xaxis) ;
  DRAW_ARROW(yaxis) ;
  DRAW_ARROW(zaxis) ;

  /* move labels if they interfere with axes rendering */
  if (xlabel[0] <= 0) xlabel[0] -= PDATA(swidth) ;
  if (xlabel[1] <= 0) xlabel[1] -= PDATA(swidth) ;

  if (ylabel[0] <= 0) ylabel[0] -= PDATA(swidth) ;
  if (ylabel[1] <= 0) ylabel[1] -= PDATA(swidth) ;

  if (zlabel[0] <= 0) zlabel[0] -= PDATA(swidth) ;
  if (zlabel[1] <= 0) zlabel[1] -= PDATA(swidth) ;

  if (!draw) {
    /* offset text slightly for shadow */
    xlabel[0] += PDATA(nudge) ; xlabel[1] -= PDATA(nudge) ;
    ylabel[0] += PDATA(nudge) ; ylabel[1] -= PDATA(nudge) ;
    zlabel[0] += PDATA(nudge) ; zlabel[1] -= PDATA(nudge) ;
  }

  glRasterPos2f (xlabel[0], xlabel[1]) ;
  glCallList('X'+FONTLISTBASE) ;

  glRasterPos2f (ylabel[0], ylabel[1]) ;
  glCallList('Y'+FONTLISTBASE) ;

  glRasterPos2f (zlabel[0], zlabel[1]) ;
  glCallList('Z'+FONTLISTBASE) ;

  glPopMatrix() ;

  if (draw && PDATA(redrawmode) != tdmViewEchoMode) {
    /* copy rendered gnomon from back buffer to front buffer */
    glReadBuffer(GL_BACK) ;
    glDrawBuffer(GL_FRONT) ;
    
    SET_RASTER_SCREEN (PDATA(illx), PDATA(illy)) ;
    glCopyPixels (PDATA(illx), PDATA(illy),
		  (PDATA(iurx)-PDATA(illx))+1, (PDATA(iury)-PDATA(illy))+1,
		  GL_COLOR) ;
    /*
     *  Restore original buffer config (PDATA(buffermode)) from
     *  tdmFrontBufferDraw (which is equivalent to GL_FRONT).  This also
     *  causes all output so far to be flushed.
     */
    _dxf_BUFFER_RESTORE_CONFIG
      (PORT_CTX, dummy, PDATA(buffermode), tdmFrontBufferDraw) ;
  }

  EXIT((""));
}


static void _dxf_DRAW_GLOBE (tdmInteractor I, void *udata,
			     float rot[4][4], int draw)
{
  /*
   *  draw == 1 to draw globe, draw == 0 to undraw.  This is done with two
   *  separate calls in order to support explicit erasure of edges for
   *  some implementations.  A draw is always preceded by an undraw and
   *  the pair of invocations is atomic.
   */

  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int u, v, on, dummy = 0 ;

  /* globe edge visibility flags.  all globe instance share this data. */
  static struct {
    int latvis, longvis ;
  } edges[LATS][LONGS] ;

  /* globe and globeface defined in tdmGlobeEchoDef.h */
  register const float (*Globe)[LONGS][3] = &globe[0] ;
  register const struct Face (*Globeface)[LONGS] = &globeface[0] ;

  /* view normal */
  register float z0, z1, z2 ;

  ENTRY(("_dxf_DRAW_GLOBE (0x%x, 0x%x, 0x%x, %d)",
	 I, udata, rot, draw));

  z0 = rot[0][2] ; z1 = rot[1][2] ; z2 = rot[2][2] ;

#define FACEVISIBLE(u,v,z0,z1,z2) \
  (Globeface[u][v].norm[0] * z0 + \
   Globeface[u][v].norm[1] * z1 + \
   Globeface[u][v].norm[2] * z2 > 0.0)

  if (draw)
    {
      glColor3f(1.0, 1.0, 1.0) ;
      glLineWidth(1.0) ;
    }
  else
    {
      if(PDATA(redrawmode) != tdmViewEchoMode)
	{
	  /*
	   *  In tdmViewEchoMode (DX's Execute On Change), we are drawing
	   *  the globe echo on top of a background image that is redrawn
	   *  with every frame of a direct interaction.
	   *
	   *  If we're not in that mode, the background image is static
	   *  while the globe echo rotates in front of it, so erasing the
	   *  globe means we have to repair damage to the background.  We
	   *  do this by blitting a portion of the static image to the
	   *  back buffer, drawing the globe over that, then blitting the
	   *  combined results back to the front buffer.
	   */

	  /* force graphics output into back buffer */
	  glDrawBuffer(GL_BACK) ;

	  /* erase globe background */
	  _dxf_WRITE_BUFFER (PORT_CTX, PDATA(illx), PDATA(illy),
			     PDATA(iurx), PDATA(iury), PDATA(background)) ;
	}
      /* draw wide black lines to ensure visibility against background */
      glColor3f(0.0, 0.0, 0.0) ;
      glLineWidth(3.0) ;
    }

  /*
   *  Compute face visibility and flag edges.  Flags are used for both the
   *  undraw and draw invocations.
   */

  if (!draw)
    {
      for (u=0 ; u<LATS-1 ; u++)
	{
	  if (FACEVISIBLE(u, 0, z0, z1, z2))
	    {
	      edges[u][LONGS-1].latvis++ ;
	      edges[u+1][LONGS-1].latvis++ ;
	      edges[u][0].longvis++ ;
	      edges[u][LONGS-1].longvis++ ;
	    }
	  for (v=1 ; v<LONGS ; v++)
	      if (FACEVISIBLE(u, v, z0, z1, z2))
		{
		  edges[u][v-1].latvis++ ;
		  edges[u+1][v-1].latvis++ ;
		  edges[u][v].longvis++ ;
		  edges[u][v-1].longvis++ ;
		}
	}

      /* north pole */
      if (z1 > 0.0)
	  for (v=0 ; v<LONGS ; v++)
	      edges[LATS-1][v].latvis++ ;

      /* south pole */
      if (z1 < 0.0)
	  for (v=0 ; v<LONGS ; v++)
	      edges[0][v].latvis++ ;
    }

  /*
   *  Draw each visible edge exactly once.  This allows XOR implementations.
   */

  for (u=0 ; u<LATS ; u++)
    {
      for (v=0, on=0 ; v<LONGS-1 ; v++)
	  if (edges[u][v].latvis)
	    {
	      if (!on)
		{
		  on = 1 ;
		  glBegin(GL_LINE_STRIP) ;
		  glVertex3fv(Globe[u][v]) ;
		}
	      glVertex3fv(Globe[u][v+1]) ;

	      /* reset edge flag */
	      if (draw) edges[u][v].latvis = 0 ;
	    }
	  else if (on)
	    {
	      on = 0 ;
	      glEnd() ;
	    }

      /* close latitude line if necessary */
      if (edges[u][LONGS-1].latvis)
	{
	  if (!on)
	    {
	      glBegin(GL_LINE_STRIP) ;
	      glVertex3fv(Globe[u][LONGS-1]) ;
	    }
	  glVertex3fv(Globe[u][0]) ;
	  glEnd() ;

	  /* reset edge flag */
	  if (draw) edges[u][LONGS-1].latvis = 0 ;
	}
      else if (on) glEnd() ;
    }

  /* longitude lines */
  for (v=0 ; v<LONGS ; v++)
    {
      for (u=0, on=0 ; u<LATS-1 ; u++)
	  if (edges[u][v].longvis)
	    {
	      if (!on)
		{
		  on = 1 ;
		  glBegin(GL_LINE_STRIP) ;
		  glVertex3fv(Globe[u][v]) ;
		}
	      glVertex3fv(Globe[u+1][v]) ;

	      /* reset edge flag */
	      if (draw) edges[u][v].longvis = 0 ;
	    }
	  else if (on)
	    {
	      on = 0 ;
	      glEnd() ;
	    }
      if (on) glEnd() ;
    }

  if (draw && PDATA(redrawmode) != tdmViewEchoMode)
    {
      /* copy rendered globe from back buffer to front buffer */
      glReadBuffer(GL_BACK) ;
      glDrawBuffer(GL_FRONT) ;

      SET_RASTER_SCREEN (PDATA(illx), PDATA(illy)) ;
      glCopyPixels (PDATA(illx), PDATA(illy),
		    (PDATA(iurx)-PDATA(illx))+1, (PDATA(iury)-PDATA(illy))+1,
		    GL_COLOR) ;
      /*
       *  Restore original buffer config (PDATA(buffermode)) from
       *  tdmFrontBufferDraw (which is equivalent to GL_FRONT).  This also
       *  causes all output so far to be flushed.
       */
      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmFrontBufferDraw) ;
    }

  EXIT((""));
}


static int _dxf_GET_ZBUFFER_STATUS (void *ctx)
{
  int status ;

  ENTRY(("_dxf_GET_ZBUFFER_STATUS(0x%x)",ctx));

  status = glIsEnabled(GL_DEPTH_TEST) ;

  EXIT(("status = %d",status));
  return status ;
}


static void _dxf_SET_ZBUFFER_STATUS (void *ctx, int status)
{
  ENTRY(("_dxf_SET_ZBUFFER_STATUS (0x%x, %d)",ctx, status));

  if (status)
    glEnable(GL_DEPTH_TEST) ;
  else
    glDisable(GL_DEPTH_TEST) ;

  EXIT((""));
}


static void _dxf_GET_WINDOW_ORIGIN (void *ctx, int *x, int *y)
{
#if defined(DX_NATIVE_WINDOWS)
	DEFCONTEXT(ctx) ;

	WINDOWPLACEMENT plc;
	if (! GetWindowPlacement(OGLXWIN, &plc))
		return;

	*x = plc.ptMinPosition.x;
	*y = plc.ptMinPosition.y;
#else
  Window child ;
  DEFCONTEXT(ctx) ;

  /*
   *  return lower left corner of window relative to screen in pixels
   */

  ENTRY(("_dxf_GET_WINDOW_ORIGIN (0x%x, 0x%x, 0x%x)",ctx, x, y));

  XTranslateCoordinates (OGLDPY, OGLXWIN, RootWindow (OGLDPY, OGLSCREEN),
			 0, 0, x, y, &child) ;

  *y = YMAXSCREEN - (*y + WINHEIGHT) + 1 ;

#endif
  EXIT(("x = %d, y = %d", *x, *y));
}


static void _dxf_GET_MAXSCREEN (void *ctx, int *x, int *y)
{
  DEFCONTEXT(ctx) ;

  /*
   *  return width of entire screen in pixels
   */

  ENTRY(("_dxf_GET_MAXSCREEN (0x%x, 0x%x, 0x%x)",ctx, x, y));

  *x = XMAXSCREEN ;
  *y = YMAXSCREEN ;

  EXIT(("x = %d, y = %d", *x, *y));
}


static void _dxf_SET_LINE_ATTRIBUTES (void *ctx,
				      int32 color, int style, float width)
{
  /*
   *  color is packed RGB.
   *  style 0 is solid.  style 1 is dashed.
   *  width is screen width of line, may be subpixel.  not currently used.
   */

  ENTRY(("_dxf_SET_LINE_ATTRIBUTES (0x%x, 0x%x, %d, %d)",
	 ctx, color, style, width));

  switch (color) {
  case 0x8f8f8f8f:
    glColor3f (0.5, 0.5, 0.5) ;
    break ;
  case 0xffffffff:
  default:
    glColor3f (1.0, 1.0, 1.0) ;
    break ;
  }

  switch (style) {
  case 1:
    glEnable(GL_LINE_STIPPLE);
    glLineStipple (1, 0x00ff) ;
    break ;
  case 0:
  default:
    glDisable(GL_LINE_STIPPLE);
    break ;
  }

  EXIT((""));
}


static void _dxf_DRAW_LINE(void *ctx, int x1, int y1, int x2, int y2)
{
  ENTRY(("_dxf_DRAW_LINE(0x%x, %d, %d, %d, %d)",ctx, x1, y1, x2, y2));

  glLineWidth(1.0) ;
  glBegin(GL_LINES) ;
    glVertex2i(x1, y1) ;
    glVertex2i(x2, y2) ;
  glEnd() ;

  EXIT((""));
}

static void _dxf_CREATE_CURSOR_PIXMAPS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int i, j, k, csize ;

  ENTRY(("_dxf_CREATE_CURSOR_PIXMAPS(0x%x)",I));
  
  csize = PDATA(cursor_size)*PDATA(cursor_size) ;

  /* allocate and init projection mark pixmap to white pixels */
  PDATA(ProjectionMark) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, 3, 3) ;

  for (i=0 ; i<9 ; i++)
    ((int32 *) PDATA(ProjectionMark))[i] =
#             if defined(alphax)
                                                        0x00ffffff
#             else
                                                        0x00ff0000
#             endif
                                                        ;

  /* allocate a cursor save-under and init to zero */
  PDATA(cursor_saveunder) =
      _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
				 PDATA(cursor_size), PDATA(cursor_size)) ;

  for (i=0 ; i<csize ; i++)
      ((int32 *)PDATA(cursor_saveunder))[i] = 0 ;

  /* allocate and init the cursor pixmaps */
  PDATA(ActiveSquareCursor) =
      _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
				 PDATA(cursor_size), PDATA(cursor_size)) ;

  PDATA(PassiveSquareCursor) =
      _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
				 PDATA(cursor_size), PDATA(cursor_size)) ;

  k = 0 ;
  for (i=0 ; i<PDATA(cursor_size) ; i++)
      for (j=0 ; j<PDATA(cursor_size) ; j++)
	  if (i==0 || i==PDATA(cursor_size)-1 ||
	      j==0 || j==PDATA(cursor_size)-1 )
	    {
	      /* active cursor is green, passive is gray */
	      ((int32 *)PDATA(ActiveSquareCursor))[k] =
#             if defined(alphax)
                                                        0x0000ff00
#             else
                                                        0x00ff0000
#             endif
                                                        ;

	      ((int32 *)PDATA(PassiveSquareCursor))[k++] =
#             if defined(alphax)
	                                                    0x00afafaf
#             else
	                                                    0xafafaf00
#             endif
                                                            ;
	    }
	  else
	    {
	      /* interiors of both are black */
	      ((int32 *)PDATA(ActiveSquareCursor))[k] = 0 ;
	      ((int32 *)PDATA(PassiveSquareCursor))[k++] =  0 ;
	    }

  EXIT((""));
}


#define IMAGE(type,x,y) \
(CLIPPED(x,y) ? 0 : ((type *)(CDATA(image))) [(x)+(y)*CDATA(iw)])


static void _dxf_DRAW_MARKER (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  int32 i, mx, my ;

  ENTRY(("_dxf_DRAW_MARKER(0x%x)",I));
    
  for (i = 0 ; i < PDATA(iMark) ; i++) 
    {
      PDATA(pXmark)[i] = PDATA(Xmark)[i] ;
      PDATA(pYmark)[i] = PDATA(Ymark)[i] ;
    }

  PDATA(piMark) = PDATA(iMark) ;
  
  for (i = 0 ; i < PDATA(iMark) ; i++)
    {
      /* round marker to long for future erasure */
      mx = (int32)(PDATA(Xmark[i])+0.5) ;
      my = (int32)(PDATA(Ymark[i])+0.5) ;
      
      /* draw it */
      if (!CLIPPED(mx,my))
	{
	  glRasterPos2i(mx-1, my-1) ;
	  CHECK_RASTER_POS();
	  glDrawPixels(3, 3, GL_RGBA, GL_UNSIGNED_BYTE, PDATA(ProjectionMark)) ;
	}
    }

  EXIT((""));
}


static void _dxf_ERASE_PREVIOUS_MARKS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  int32 i, mx, my, m[9] ;

  ENTRY(("_dxf_ERASE_PREVIOUS_MARKS (0x%x)", I));
  
  if (!PDATA(visible)) {
    EXIT(("not visible"));
    return ;
  }

  m[0]=m[1]=m[2]=m[3]=m[4]=m[5]=m[6]=m[7]=m[8] = 0 ;

  /* Restore the image over the previous marks */
  for ( i = 0 ; i < PDATA(piMark) ; i++ )
    {
      /* round mark coordinates */
      mx = (int32)(PDATA(pXmark[i])+0.5) ;
      my = (int32)(PDATA(pYmark[i])+0.5) ;

      if (!CLIPPED(mx,my))
	{
	  /* copy image to array m */
	  m[0] = IMAGE(int32, mx-1, my-1) ;
	  m[1] = IMAGE(int32, mx,   my-1) ;
	  m[2] = IMAGE(int32, mx+1, my-1) ;
	  m[3] = IMAGE(int32, mx-1, my  ) ;
	  m[4] = IMAGE(int32, mx,   my  ) ;
	  m[5] = IMAGE(int32, mx+1, my  ) ;
	  m[6] = IMAGE(int32, mx-1, my+1) ;
	  m[7] = IMAGE(int32, mx,   my+1) ;
	  m[8] = IMAGE(int32, mx+1, my+1) ;

	  /* blit m to the screen */
	  glRasterPos2i (mx-1, my-1) ;
	  CHECK_RASTER_POS();
	  glDrawPixels (3, 3, GL_RGBA, GL_UNSIGNED_BYTE, m) ;
	}
    }

  EXIT((""));
}


static void _dxf_ERASE_CURSOR (tdmInteractor I, int x, int y)
{
  DEFDATA(I,CursorData) ;
  int32 i, j, k, s2, llx, lly ;

  ENTRY(("_dxf_ERASE_CURSOR (0x%x, %d, %d)",I, x, y));
  
  if (!PDATA(visible) || CLIPPED(x,y)) {
    EXIT(("not visible or clipped"));
    return ;
  }

  k=0 ;
  s2= PDATA(cursor_size) / 2 ;

  llx = x-s2 ;
  lly = y-s2 ;

  /* copy image to save-under array */
  for (i=0 ; i<PDATA(cursor_size) ; i++)
      for (j=0 ; j<PDATA(cursor_size) ; j++)
	  ((int32 *)PDATA(cursor_saveunder))[k++] = IMAGE(int32, llx+j, lly+i) ;

  /* blit it to the screen */
  glRasterPos2i (llx, lly) ;
  CHECK_RASTER_POS();
  glDrawPixels (PDATA(cursor_size), PDATA(cursor_size),
		GL_RGBA, GL_UNSIGNED_BYTE, PDATA(cursor_saveunder)) ;

  EXIT((""));
}


static void _dxf_DRAW_CURSOR_COORDS (tdmInteractor I, char *text)
{
  int len ;
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("_dxf_DRAW_CURSOR_COORDS(0x%x, \"%s\")",I, text));
  
  len = strlen(text) ;
  glListBase(FONTLISTBASE) ;

  glColor3f (0.0, 0.0, 0.0) ;
  glRasterPos2i (PDATA(coord_llx)+3, PDATA(coord_lly)+2) ;
  glCallLists (len, GL_BYTE, text) ;
  
  glColor3f (1.0, 1.0, 1.0) ;
  glRasterPos2i (PDATA(coord_llx)+2, PDATA(coord_lly)+3) ;
  glCallLists (len, GL_BYTE, text) ;

  /* make all cursor motion updates visible now */
  glFlush() ;

  EXIT((""));
}


static void _dxf_POINTER_INVISIBLE (void *ctx)
{
#if !defined(DX_NATIVE_WINDOWS)
  /* make the mouse pointer invisible */
  int i ;
  Pixmap p1, p2 ;
  XColor fc, bc ;
  char tmp_bits[8] ;
  Cursor cursorId ;
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_POINTER_INVISIBLE(0x%x)",ctx));

  for (i = 0 ; i < 8 ; i++)
    tmp_bits[i] = 0 ;

  p1 = XCreatePixmapFromBitmapData
     (OGLDPY, RootWindow(OGLDPY, OGLSCREEN), tmp_bits, 4, 4, 0, 0, 1) ;
  p2 = XCreatePixmapFromBitmapData
     (OGLDPY, RootWindow(OGLDPY, OGLSCREEN), tmp_bits, 4, 4, 0, 0, 1) ;

  cursorId = XCreatePixmapCursor (OGLDPY, p1, p2, &fc, &bc, 1, 1 ) ;

  XFreePixmap (OGLDPY, p1) ;
  XFreePixmap (OGLDPY, p2) ;

  XDefineCursor (OGLDPY, OGLXWIN, cursorId) ;
#endif

  EXIT((""));
}


static void
_dxf_POINTER_VISIBLE (void *ctx)
{
#if !defined(DX_NATIVE_WINDOWS)
  DEFCONTEXT(ctx) ;

  /* make the mouse pointer visible */
  ENTRY(("_dxf_POINTER_VISIBLE(0x%x)",ctx));

  XUndefineCursor (OGLDPY, OGLXWIN) ;

#endif
  EXIT((""));
}


static void _dxf_WARP_POINTER (void *ctx, int x, int y)
{
#if !defined(DX_NATIVE_WINDOWS)
  DEFCONTEXT(ctx) ;

  /* move mouse pointer to position [x,y] relative to screen origin */
  ENTRY(("_dxf_WARP_POINTER (0x%x, %d, %d)",ctx, x, y));

  XWarpPointer (OGLDPY, None, DefaultRootWindow(OGLDPY), 0, 0, 0, 0,
		x, YMAXSCREEN-y) ;

#endif
  EXIT((""));
}

static void _dxf_restoreZoomBox (tdmInteractor I, void *udata,
			       float rot[4][4])
{
  static int32 buff[4096] ;
  register int32 *bp, *image ;
  register int i, iw, n, y2 ;
  int x1, x2, y1, ih ;
  DEFDATA(I, tdmZoomData) ;
    
  iw = CDATA(iw) ;
  ih = CDATA(ih) ;
  image = (int32 *)CDATA(image) ;
  
  /* clip coords to image to avoid invalid raster positions */
  x1 = (PDATA(x1)<0 ? 0 : (PDATA(x1)>(iw-1) ? iw-1 : PDATA(x1))) ;
  x2 = (PDATA(x2)<0 ? 0 : (PDATA(x2)>(iw-1) ? iw-1 : PDATA(x2))) ;
  y1 = (PDATA(y1)<0 ? 0 : (PDATA(y1)>(ih-1) ? ih-1 : PDATA(y1))) ;
  y2 = (PDATA(y2)<0 ? 0 : (PDATA(y2)>(ih-1) ? ih-1 : PDATA(y2))) ;
  
  /* Top Row */
  glRasterPos2i (x1, y1) ;
  CHECK_RASTER_POS();
  glDrawPixels ((x2-x1)+1, 1, GL_RGBA, GL_UNSIGNED_BYTE, image+(x1+y1*iw)) ;
  if(y1<ih-1)
  {
     glRasterPos2i (x1, y1+1) ;
     glDrawPixels((x2-x1)+1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
		  image+(x1+(y1+1)*iw));
  }

  /* Bottom Row */
  glRasterPos2i (x1, y2) ;
  CHECK_RASTER_POS();
  glDrawPixels ((x2-x1)+1, 1, GL_RGBA, GL_UNSIGNED_BYTE, image+(x1+y2*iw)) ;
  if(y2<ih-1)
  {
     glRasterPos2i(x1,y2+1);
     glDrawPixels ((x2-x1)+1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
		   image+(x1+(y2+1)*iw)) ;
  }
  
  for (bp=buff, n=x1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
    *bp++ = image[n] ;
  
  glRasterPos2i (x1, y1) ;
  CHECK_RASTER_POS();
  glDrawPixels (1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
  if(x1<iw-1)
  {
     for (bp=buff, n=x1+1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
     *bp++ = image[n] ;
     glRasterPos2i (x1+1, y1) ;
     glDrawPixels (1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
  }
  
  for (bp=buff, n=x2+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
    *bp++ = image[n] ;
  
  glRasterPos2i (x2, y1) ;
  CHECK_RASTER_POS();
  glDrawPixels (1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;

  if(x2<iw-1)
  {
     for (bp=buff, n=x2+1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
       *bp++ = image[n] ;
     glRasterPos2i (x2+1, y1) ;
     glDrawPixels (1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
  }
  
  EXIT((""));
}

static void _dxf_captureZoomBox (tdmInteractor I, void *udata,
			       float rot[4][4])
{
  static int32 buff[4096] ;
  register int32 *bp, *image ;
  register int i, iw, n, y2 ;
  int x1, x2, y1, ih ;
  DEFDATA(I, tdmZoomData) ;
    
  iw = CDATA(iw) ;
  ih = CDATA(ih) ;
  image = (int32 *)CDATA(image) ;
  
  /* clip coords to image to avoid invalid raster positions */
  x1 = (PDATA(x1)<0 ? 0 : (PDATA(x1)>(iw-1) ? iw-1 : PDATA(x1))) ;
  x2 = (PDATA(x2)<0 ? 0 : (PDATA(x2)>(iw-1) ? iw-1 : PDATA(x2))) ;
  y1 = (PDATA(y1)<0 ? 0 : (PDATA(y1)>(ih-1) ? ih-1 : PDATA(y1))) ;
  y2 = (PDATA(y2)<0 ? 0 : (PDATA(y2)>(ih-1) ? ih-1 : PDATA(y2))) ;
  
  /* Top Row */
  glReadPixels (x1, y1, (x2-x1)+1, 1, GL_RGBA,
			GL_UNSIGNED_BYTE, image+(x1+y1*iw)) ;

  if(y1<ih-1)
     glReadPixels(x1, y1+1, (x2-x1)+1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
		  image+(x1+(y1+1)*iw));

  /* Bottom Row */
  glReadPixels (x1, y2, (x2-x1)+1, 1, GL_RGBA,
			GL_UNSIGNED_BYTE, image+(x1+y2*iw)) ;
  if(y2<ih-1)
     glReadPixels (x1,y2+1,(x2-x1)+1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
		   image+(x1+(y2+1)*iw)) ;
  
  glReadPixels (x1, y1, 1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
  for (bp=buff, n=x1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
    image[n] = *bp++;

  if(x1<iw-1)
  {
     glReadPixels (x1+1, y1, 1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
     for (bp=buff, n=x1+1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
       image[n] = *bp++;
  }
  
  glReadPixels (x2, y1, 1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
  for (bp=buff, n=x2+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
    image[n] = *bp++;

  if(x2<iw-1)
  {
     glReadPixels (x2+1, y1, 1, (y2-y1)+1, GL_RGBA, GL_UNSIGNED_BYTE, buff) ;
     for (bp=buff, n=x2+1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
       image[n] = *bp++;
  }
  
  EXIT((""));
}
#undef _dxf_SERVICES_FLAGS
extern hwFlags _dxf_SERVICES_FLAGS();

static void _dxf_DRAW_ZOOMBOX (tdmInteractor I, void *udata,
			       float rot[4][4], int draw)
{
  /* zoom box height better not be any longer than this... */
  DEFDATA(I, tdmZoomData) ;
  
  ENTRY(("_dxf_DRAW_ZOOMBOX (0x%x, 0x%x, 0x%x, %d)",
	 I, udata, rot, draw));

  if (draw) {

    if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE))
	_dxf_captureZoomBox(I, udata, rot);

    /* draw the box */
    glColor3f (1.0, 1.0, 1.0) ;
    glLineWidth(1.0) ;
    glBegin(GL_LINE_STRIP) ;

    /* The additional 0.5 fixes bug <JKAROL370> */
    /* (J) (:Q= The Additional 0.5 Does Not FIX, but Rather
       Simplifies <JKAROL370> */
    glVertex2f ((float)PDATA(x1)+0.5, (float)PDATA(y1)+0.5) ;
    glVertex2f ((float)PDATA(x2)+0.5, (float)PDATA(y1)+0.5) ;
    glVertex2f ((float)PDATA(x2)+0.5, (float)PDATA(y2)+0.5) ;
    glVertex2f ((float)PDATA(x1)+0.5, (float)PDATA(y2)+0.5) ;
    glVertex2f ((float)PDATA(x1)+0.5, (float)PDATA(y1)+0.5) ;

    glEnd() ;
    glFlush() ;
  }
  else
      _dxf_restoreZoomBox(I, udata, rot);
  
  EXIT((""));
}


/*
 *  STUB SECTION
 */
static void _dxf_SET_LINE_COLOR_WHITE (void *ctx)
{
  /* obsolete entry:  do not use in new code */
  ENTRY(("_dxf_SET_LINE_COLOR_WHITE(0x%x)",ctx));
  
  glColor3f(1.0, 1.0, 1.0) ;

  EXIT(("ERROR: stub"));
}


static void
_dxf_SET_LINE_COLOR_GRAY (void *ctx)
{
  /* obsolete entry:  do not use in new code */
  ENTRY(("_dxf_SET_LINE_COLOR_GRAY(0x%x)",ctx));

  glColor3f(0.5, 0.5, 0.5) ;

  EXIT(("ERROR: stub"));
}


static void _dxf_SET_SOLID_FILL_PATTERN (void *ctx)
{
  /* obsolete for OpenGL port */
  ENTRY(("_dxf_SET_SOLID_FILL_PATTERN (0x%x)",ctx));
  EXIT(("Error: stub"));
}


static void _dxf_DRAW_ARROWHEAD (void* ctx, float ax, float ay)
{
  /* obsolete for OpenGL port */
  ENTRY(("_dxf_DRAW_ARROWHEAD (0x%x, %f, %f)",ctx,ax,ay));
  EXIT(("Error: stub"));
}


/*
 *  Port handle section.
 */
#define STRUCTURES_ONLY
#include "../hwInteractorEcho.h"

tdmInteractorEchoT	_dxd_hwInteractorEchoPortOGL =
{
  /* BufferConfig */		_dxf_BUFFER_CONFIG,
  /* BufferRestoreConfig */	_dxf_BUFFER_RESTORE_CONFIG,
  /* CreateCursorPixmaps */	_dxf_CREATE_CURSOR_PIXMAPS,
  /* DrawArrowhead */		_dxf_DRAW_ARROWHEAD,
  /* DrawCursorCoords */	_dxf_DRAW_CURSOR_COORDS,
  /* DrawGlobe */		_dxf_DRAW_GLOBE,
  /* DrawGnomon */ 		_dxf_DRAW_GNOMON,
  /* DrawLine */		_dxf_DRAW_LINE,
  /* DrawMarker */		_dxf_DRAW_MARKER,
  /* DrawZoombox */		_dxf_DRAW_ZOOMBOX,
  /* EraseCursor */		_dxf_ERASE_CURSOR,
  /* ErasePreviousMarks */	_dxf_ERASE_PREVIOUS_MARKS,
  /* GetMaxscreen */		_dxf_GET_MAXSCREEN,
  /* GetWindowOrigin */		_dxf_GET_WINDOW_ORIGIN,
  /* GetZbufferStatus */	_dxf_GET_ZBUFFER_STATUS,
  /* PointerInvisible */	_dxf_POINTER_INVISIBLE,
  /* PointerVisible */		_dxf_POINTER_VISIBLE,
  /* ReadBuffer */		_dxf_READ_BUFFER,
  /* SetLineColorGray */	_dxf_SET_LINE_COLOR_GRAY,
  /* SetLineColorWhite */	_dxf_SET_LINE_COLOR_WHITE,
  /* SetSolidFillPattern */	_dxf_SET_SOLID_FILL_PATTERN,
  /* SetWorldScreen */		_dxf_SET_WORLD_SCREEN,
  /* SetZbufferStatus */	_dxf_SET_ZBUFFER_STATUS,
  /* WarpPointer */		_dxf_WARP_POINTER,
  /* WriteBuffer */		_dxf_WRITE_BUFFER,
  /* SetLineAttributes */       _dxf_SET_LINE_ATTRIBUTES
};
