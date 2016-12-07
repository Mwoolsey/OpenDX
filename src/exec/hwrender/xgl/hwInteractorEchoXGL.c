/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwInteractorEchoXGL.c,v $

  These are the XGL implementations of the TDM direct interactor echos,
  along with miscellaneous utility routines.  Each function needs to be
  ported to the native graphics API of the target system in order to provide
  support for direct interactors.

  Applications such as DX can be ported as a first step without direct
  interactor facilities by stubbing out everything here.

  TDM's idea of the origin of DC space is the lower-left corner of the
  graphics window, while XGL uses the upper-left convention.  The only
  primitives in XGL which use device coordinates are the pixel level
  read/write primitives.  Therefore, in order to draw in pixel space we set
  up model and world coordinates to map to TDM's notion of DC; XGL then
  flips the Y coordinate implicitly.  However, when we use the XGL pixel
  read/write primitives we have to flip the Y coordinate ourselves.

 $Log: hwInteractorEchoXGL.c,v $
 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:42  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:36  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:01:47  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:33:11  svs
 Copy of release 3.1.4 code

 * Revision 8.1  1996/05/03  17:54:54  gda
 * fix(?) zoom box when NO_BACKING_STORE set
 *
 * Revision 8.0  1995/10/03  22:14:56  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.8  1995/08/30  21:09:53  c1orrang
 * Re-doublebuffered solaris build w/ DX_SINGLE_BUFFER_SUN flag
 *
 * Revision 7.4  1994/06/08  20:07:53  jrush
 * Change made in common layer to interface to SET_LINE_ATTRIBUTES
 * from long to int32.  This has no effect other than to generate
 * a compile time warning, thus this change.
 * ,.
 *
 * Revision 7.3  1994/05/06  14:26:42  mjh
 * Cursor interactor maintenance:  add SetLineAttributes to support dashed
 * lines.
 *
 * Revision 7.2  94/03/31  15:55:11  tjm
 * added versioning
 * 
 * Revision 7.1  94/01/18  18:59:57  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:26:03  svs
 * ship level code, release 2.0
 * 
 * Revision 1.4  93/10/05  18:11:43  ellen
 * Fixed Zoombox...it wasn't redrawing correctly, and
 * was leaving the white zoom box trail.
 * 
 * Revision 1.3  93/09/28  08:39:00  tjm
 * fixed <DMCZAP54> garbage on screen when moving roam point
 * 
 * Revision 1.2  93/07/14  13:28:51  tjm
 * added solaris ifdefs
 * 
 * Revision 1.1  93/06/29  10:01:25  tjm
 * Initial revision
 * 
 * Revision 5.3  93/05/11  18:04:30  mjh
 * Implement screen door transparency for the GT.
 * 
 * Revision 5.2  93/04/09  18:34:39  mjh
 * replace calls to TDM_DRAW_ARROWHEAD_XGL with _dxf_DRAW_ARROWHEAD_XGL
 * 
 * Revision 5.1  93/03/30  14:40:57  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.9  93/03/29  15:37:30  ellen
 * Cleaned up _dxf_CREATE_CURSOR_PIXMAPS.
 * 
 * Revision 5.0.2.8  93/03/22  18:41:56  ellen
 * Set error in _dxf_CREATE_CURSOR_PIXMAPS if there isn't enough memory.
 * 
 * Revision 5.0.2.7  93/02/11  20:39:55  mjh
 * In _dxf_DRAW_GNOMON() and _dxf_DRAW_GLOBE(), use PDATA(buffermode) 
 * instead of PDATA(redrawmode) restoring the frame buffer configuration
 * after blitting the echo from the draw buffer to the display buffer.
 * 
 * Revision 5.0.2.6  93/02/04  19:25:36  mjh
 * Implement cursor interactor echo.  
 * 
 * Revision 5.0.2.5  93/02/02  18:39:19  mjh
 * Implement globe echo.  DXRemove shadow rendering code since this is
 * very slow on the Sun GS.
 * 
 * Revision 5.0.2.4  93/02/01  22:22:11  mjh
 * Implement pixel read/write, gnomon and zoom box echos.
 *
 * The gnomon echo is no longer double-buffered as this incurs the expense
 * of copying the display buffer to the draw buffer as part of the
 * interactor setup, and this is quite slow with the Sun GS card.  The echo
 * is now rendered into the draw buffer and then blitted into the display
 * buffer.
 * 
 * Revision 5.0.2.3  93/01/06  17:00:55  mjh
 * window creation and destruction
 * 
 * Revision 5.0.2.2  93/01/06  04:42:00  mjh
 * initial revision
 * 
\*---------------------------------------------------------------------------*/

#include <stdio.h>
#define STRUCTURES_ONLY
#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwMatrix.h"
#include "hwInteractor.h"
#include "hwInteractorEcho.h"
#include "hwCursorInteractor.h"
#include "hwRotateInteractor.h"
#include "hwZoomInteractor.h"
#include "hwGlobeEchoDef.h"

#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

void *
_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height) ;

void 
_dxf_BUFFER_CONFIG (void *ctx, void *image, int llx, int lly, int urx, int ury,
		   int *CurrentDisplayMode, int *CurrentBufferMode,
		   tdmInteractorRedrawMode redrawMode) 
{ 
  /*
   *  Direct interactor echos use this and following routine to manage
   *  double buffering.
   */
  
  DEFCONTEXT(ctx) ;
  DPRINT("\n(_dxf_BUFFER_CONFIG") ;

  if(!DOUBLE_BUFFER_MODE)
     return;

  *CurrentBufferMode = BUFFER_CONFIG_MODE ;

  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw)
    {
      DPRINT1 ("\n%s", redrawMode == tdmBothBufferDraw ?
	       "tdmBothBufferDraw" : "tdmFrontBufferDraw") ;
      /*
       *  Enable the front buffer for writing.  This translates into
       *  setting the XGL draw and display buffers to the same buffer so
       *  that draw updates are immediately visible.
       *
       *  We won't be double buffering the globe and gnomon echos for the
       *  XGL port, so we needn't be concerned with keeping the back
       *  buffer contents identical to the front.
       */

      if (DISPLAY_BUFFER != DRAW_BUFFER)
	  if (DISPLAY_BUFFER == 1)
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 1,
			      XGL_WIN_RAS_BUF_DISPLAY, 1,
			      NULL) ;
	      DRAW_BUFFER = 1 ;
	    }
	  else
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 0,
			      XGL_WIN_RAS_BUF_DISPLAY, 0,
			      NULL) ;
	      DRAW_BUFFER = 0 ;
	    }

      BUFFER_CONFIG_MODE = FRNTBUFFER ;
    }
  else if (redrawMode == tdmBackBufferDraw ||
	   redrawMode == tdmViewEchoMode)
    {
      DPRINT1 ("\n%s", redrawMode == tdmViewEchoMode ?
	       "tdmViewEchoMode" : "tdmBackBufferDraw") ;
      /*
       *  Enable the back buffer for writing.  In XGL this just means
       *  setting the draw and display buffers not equal to each other.
       */

      if (DISPLAY_BUFFER == DRAW_BUFFER)
	  if (DISPLAY_BUFFER == 1)
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 0,
			      XGL_WIN_RAS_BUF_DISPLAY, 1,
			      NULL) ;
	      DRAW_BUFFER = 0 ;
	    }
	  else
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 1,
			      XGL_WIN_RAS_BUF_DISPLAY, 0,
			      NULL) ;
	      DRAW_BUFFER = 1 ;
	    }

      BUFFER_CONFIG_MODE = BCKBUFFER ;
    }
  else
      DPRINT("\nignoring redraw mode") ;

  DPRINT(")") ;
}


void
_dxf_BUFFER_RESTORE_CONFIG (void *ctx, int OrigDisplayMode, int OrigBufferMode,
			   tdmInteractorRedrawMode redrawMode)
{
  /*
   *  Restore frame buffer configuration.
   */

  DEFCONTEXT(ctx) ;
  DPRINT("\n(_dxf_BUFFER_RESTORE_CONFIG") ;

  if(!DOUBLE_BUFFER_MODE)
    return;

  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw)
    {
      DPRINT1("\ncurrent interactor redraw mode is %s",
	      redrawMode == tdmFrontBufferDraw ?
	      "tdmFrontBufferDraw" : "tdmBothBufferDraw") ;

      if (OrigBufferMode == BCKBUFFER)
        {
          DPRINT("\nrestoring to original back buffer draw") ;
	  if (DISPLAY_BUFFER == 1)
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 0,
			      XGL_WIN_RAS_BUF_DISPLAY, 1,
			      NULL) ;
	      DRAW_BUFFER = 0 ;
	    }
	  else
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 1,
			      XGL_WIN_RAS_BUF_DISPLAY, 0,
			      NULL) ;
	      DRAW_BUFFER = 1 ;
	    }
	  
	  BUFFER_CONFIG_MODE = BCKBUFFER ;
	}
      else
	  DPRINT("\nremaining in original front buffer draw") ;
    }
  else if (redrawMode == tdmBackBufferDraw ||
	   redrawMode == tdmViewEchoMode)
    {
      DPRINT1 ("\ncurrent interactor redraw mode is %s",
	       redrawMode == tdmViewEchoMode ?
	       "tdmViewEchoMode" : "tdmBackBufferDraw") ;

      if (OrigBufferMode == FRNTBUFFER)
        {
          DPRINT("\nrestoring to original front buffer draw") ;
	  if (DISPLAY_BUFFER == 1)
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 1,
			      XGL_WIN_RAS_BUF_DISPLAY, 1,
			      NULL) ;
	      DRAW_BUFFER = 1 ;
	    }
	  else
	    {
	      xgl_object_set (XGLRAS,
			      XGL_WIN_RAS_BUF_DRAW, 0,
			      XGL_WIN_RAS_BUF_DISPLAY, 0,
			      NULL) ;
	      DRAW_BUFFER = 0 ;
	    }
	  
	  BUFFER_CONFIG_MODE = FRNTBUFFER ;
	}
      else
          DPRINT("\nremaining in original back buffer draw") ;
    }
  else
      DPRINT("\nignoring redraw mode") ;

  DPRINT(")") ;
}

void
_dxf_READ_BUFFER(void *ctx, int llx, int lly, int urx, int ury, void *buff)
{ 
  /*
   *  DXRead an array of pixels from display buffer into buff.
   */

  DEFCONTEXT(ctx) ;
  Xgl_bounds_i2d rect ;

  DPRINT("\n(_dxf_READ_BUFFER") ;
  DPRINT1("\nllx %d", llx) ; DPRINT1("  lly %d", lly) ;
  DPRINT1("  urx %d", urx) ; DPRINT1("  ury %d", ury) ;

  rect.xmin = llx ;         rect.xmax = urx ;
  rect.ymin = VP_YMAX-ury ; rect.ymax = VP_YMAX-lly ;

  /* set buff as rendering surface for temporary XGL context */
  xgl_object_set (XGLTMPCTX, XGL_CTX_DEVICE, buff, NULL) ;

  if (DISPLAY_BUFFER == DRAW_BUFFER)
      /* doesn't matter which buffer we read from */
#ifdef solaris
      xgl_context_copy_buffer (XGLTMPCTX, &rect, NULL, XGLRAS) ;
#else
      xgl_context_copy_raster (XGLTMPCTX, &rect, NULL, XGLRAS) ;
#endif
  else
      if (DISPLAY_BUFFER == 1)
	{
	  /* set draw buffer to 1 so we can read it, but don't swap display */
	  xgl_object_set (XGLRAS,
			  XGL_WIN_RAS_BUF_DRAW, 1,
			  XGL_WIN_RAS_BUF_DISPLAY, 1,
			  NULL) ;
	  
	  /* read pixels and reset original buffer assignments */
#ifdef solaris
      xgl_context_copy_buffer (XGLTMPCTX, &rect, NULL, XGLRAS) ;
#else
      xgl_context_copy_raster (XGLTMPCTX, &rect, NULL, XGLRAS) ;
#endif
	  xgl_object_set (XGLRAS,
			  XGL_WIN_RAS_BUF_DRAW, 0,
			  XGL_WIN_RAS_BUF_DISPLAY, 1,
			  NULL) ;
	}
      else
	{
	  /* set draw buffer to 0 so we can read it, but don't swap display */
	  xgl_object_set (XGLRAS,
			  XGL_WIN_RAS_BUF_DRAW, 0,
			  XGL_WIN_RAS_BUF_DISPLAY, 0,
			  NULL) ;
	  
	  /* read pixels and reset original buffer assignments */
#ifdef solaris
      xgl_context_copy_buffer (XGLTMPCTX, &rect, NULL, XGLRAS) ;
#else
      xgl_context_copy_raster (XGLTMPCTX, &rect, NULL, XGLRAS) ;
#endif
	  xgl_object_set (XGLRAS,
			  XGL_WIN_RAS_BUF_DRAW, 1,
			  XGL_WIN_RAS_BUF_DISPLAY, 0,
			  NULL) ;
	}

  DPRINT(")") ;
}
 
void
_dxf_WRITE_BUFFER (void *ctx, int llx, int lly, int urx, int ury, void *buff)
{ 
  /*
   *  DXWrite pixel array from buff.  The entire contents of buff are written.
   */

  DEFCONTEXT(ctx) ;
  Xgl_pt_i2d position ;

  DPRINT("\n(_dxf_WRITE_BUFFER") ;
  DPRINT1("\nllx %d", llx) ; DPRINT1("  lly %d", lly) ;
  DPRINT1("  urx %d", urx) ; DPRINT1("  ury %d", ury) ;

  position.x = llx ; position.y = VP_YMAX-ury ;
#ifdef solaris
  xgl_context_copy_buffer (XGLCTX, NULL, &position, buff) ;
#else
  xgl_context_copy_raster (XGLCTX, NULL, &position, buff) ;
#endif

  DPRINT(")") ;
}

void 
_dxf_DRAW_GLOBE (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  /*
   *  draw == 1 to draw globe, draw == 0 to undraw.  This is done with two
   *  separate calls in order to support explicit erasure of edges for
   *  some implementations.  A draw is always preceded by an undraw and
   *  the pair of invocations is assumed to be atomic.
   */

  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_color color ;
  Xgl_pt_list line ;
  Xgl_pt_f3d pos[LATS+LONGS+1] ;
  register Xgl_pt_f3d *p ;
  int u, v, on, dummy = 0 ; 

  /* globe edge visibility flags.  all globe instance share this data. */
  static struct {
    int latvis, longvis ;
  } edges[LATS][LONGS] ;

  /* globe and globeface defined in tdmGlobeEchoDef.h */
  register float (*Globe)[LONGS][3] = (float (*)[LONGS][3])globe ;
  register struct Face (*Globeface)[LONGS] = (struct Face(*)[LONGS])globeface ;

  /* view normal */
  register float z0, z1, z2 ;
  z0 = rot[0][2] ; z1 = rot[1][2] ; z2 = rot[2][2] ;

#define FACEVISIBLE(u,v,z0,z1,z2) \
  (Globeface[u][v].norm[0] * z0 + \
   Globeface[u][v].norm[1] * z1 + \
   Globeface[u][v].norm[2] * z2 > 0.0)

#define ENDLINE { \
  if (p != pos) xgl_multipolyline (XGLCTX, NULL, 1, &line) ; \
  p = pos ; }

#define MOVE(vertex) { \
  ENDLINE ; \
  *p = *(Xgl_pt_f3d *)vertex ; \
  line.num_pts = 1 ; }
      
#define DRAW(vertex) { \
  *++p = *(Xgl_pt_f3d *)vertex ; \
  line.num_pts++ ; }

  DPRINT("\n(DrawGlobe") ;
  if (draw)
    {
      color.rgb.r = 1.0 ;
      color.rgb.g = 1.0 ;
      color.rgb.b = 1.0 ;
      xgl_object_set (XGLCTX,
		      XGL_CTX_LINE_COLOR, &color,
		      NULL) ;
    }
  else
    {
      if (PDATA(redrawmode) != tdmViewEchoMode)
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
	  
	  /* force graphics output into back (draw) buffer */
	  _dxf_BUFFER_CONFIG (PORT_CTX, &dummy, dummy, dummy, dummy, dummy,
			     &dummy, &dummy, tdmBackBufferDraw) ;
	  
	  /* erase globe background */
	  _dxf_WRITE_BUFFER (PORT_CTX, PDATA(illx), PDATA(illy),
			    PDATA(iurx), PDATA(iury), PDATA(background)) ;
	}

      DPRINT(")") ;
      return ;
    }

  /* mark visible edges */
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

  /* set up XGL point list */
  line.pt_type = XGL_PT_F3D ;
  line.bbox = NULL ;
  line.pts.f3d = p = pos ;
  
  /* draw each visible edge exactly once */
  for (u=0 ; u<LATS ; u++)
    {
      /* latitude lines */
      for (v=0, on=0 ; v<LONGS-1 ; v++)
          if (edges[u][v].latvis)
            {
              if (!on)
                {
                  on = 1 ;
		  MOVE(Globe[u][v]) ;
                }
	      DRAW(Globe[u][v+1]) ;
              edges[u][v].latvis = 0 ;
            }
          else on = 0 ;

      /* close latitude line if necessary */
      if (edges[u][LONGS-1].latvis)
        {
          if (!on) MOVE(Globe[u][LONGS-1]) ;

	  DRAW(Globe[u][0]) ;
          edges[u][LONGS-1].latvis = 0 ;
        }
    }

  for (v=0 ; v<LONGS ; v++)
      /* longitude lines */
      for (u=0, on=0 ; u<LATS-1 ; u++)
          if (edges[u][v].longvis)
            {
              if (!on)
                {
                  on = 1 ;
		  MOVE(Globe[u][v]) ;
                }
	      DRAW(Globe[u+1][v]) ;
              edges[u][v].longvis = 0 ;
            }
          else on = 0 ;

  ENDLINE ;

  if (PDATA(redrawmode) != tdmViewEchoMode)
    {
      /* copy rendered globe from draw buffer to display buffer */
      Xgl_bounds_i2d rect ;
      Xgl_pt_i2d position ;

      position.x = PDATA(illx) ; position.y = VP_YMAX-PDATA(iury) ;
      rect.xmin =  PDATA(illx) ;  rect.ymin = VP_YMAX-PDATA(iury) ;
      rect.xmax =  PDATA(iurx) ;  rect.ymax = VP_YMAX-PDATA(illy) ;

      /* ensure that draw buffer rendering is completed */
      xgl_context_post (XGLCTX, 0) ;

      /* copy the combined globe and background image to display buffer */
#ifdef solaris
      xgl_object_set (XGLRAS,
		      XGL_RAS_SOURCE_BUFFER, DRAW_BUFFER,
		      NULL) ;

      /* Set XGLCTX output dest to XGL_RENDER_DISPLAY_BUFFER
	 ( == DISPLAY_BUFFER) */
      xgl_object_set (XGLCTX,
		      XGL_CTX_RENDER_BUFFER, XGL_RENDER_DISPLAY_BUFFER,
		      NULL) ;
      xgl_context_copy_buffer
	  (XGLCTX, &rect, &position, XGLRAS) ;

      /* Reset XGLCTX output dest to default */
      xgl_object_set (XGLCTX,
		      XGL_CTX_RENDER_BUFFER, XGL_RENDER_DRAW_BUFFER,
		      NULL) ;
#else
      xgl_context_copy_buffer
	  (XGLCTX, &rect, &position, DRAW_BUFFER, DISPLAY_BUFFER) ; 
#endif

      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmBackBufferDraw) ;
    }
  DPRINT(")") ;
}

void
_dxf_DRAW_ARROWHEAD (void *ctx, float ax, float ay)
{
  DEFCONTEXT(ctx) ;
  Xgl_pt_list line ;
  Xgl_pt_f3d verts[3] ;
  double angle, hyp, length ;
  float x1, y1 ;

  /* set up XGL point list */
  line.pt_type = XGL_PT_F3D ;
  line.bbox = NULL ;
  line.num_pts = 3 ;
  line.pts.f3d = verts ;
  verts[1].x = ax ;
  verts[1].y = ay ;
  verts[0].z = 0 ;
  verts[1].z = 0 ;
  verts[2].z = 0 ;
  
  hyp = sqrt(ax*ax + ay*ay) ;

  if (hyp == 0.0)
      return ;

  if (hyp < 0.2)
      length = 0.06 ;
  else
      length = 0.12 ;

  /* To avoid acos DOMAIN errors due to floating point inaccuracies */
  if (-ax/hyp > 1.0)
    hyp = -ax;
  if (-ax/hyp < -1.0)
    hyp = ax;

  angle = acos((double)(-ax/hyp)) ;
  if (ay > 0)
      angle = 2*M_PI - angle ;

  angle = angle + M_PI/6 ;
  if (angle > 2*M_PI)
      angle = angle - 2*M_PI ;

  x1 = cos(angle) * length ;
  y1 = sin(angle) * length ;

  verts[0].x = ax + x1 ;
  verts[0].y = ay + y1 ;

  angle = angle - M_PI/3 ;
  if (angle < 0)
      angle = angle + 2*M_PI ;

  x1 = cos(angle) * length ;
  y1 = sin(angle) * length ;

  verts[2].x = ax + x1 ;
  verts[2].y = ay + y1 ;

  xgl_multipolyline (XGLCTX, NULL, 1, &line) ;
}

void 
_dxf_DRAW_GNOMON (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  /*
   *  draw == 1 to draw gnomon, draw == 0 to undraw.  This is done with
   *  two separate calls in order to support explicit erasure of edges for
   *  some implementations.  A draw is always preceded by an undraw and
   *  the pair of invocations is assumed to be atomic.
   *
   *  Computations are done in normalized screen coordinates in order to
   *  render arrow heads correctly.
   */

  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_color color ;
  Xgl_pt_list line ;
  Xgl_pt_f3d pos[2] ;
  int dummy = 0 ; 

  float origin[2] ;
  float xaxis[2],  yaxis[2],  zaxis[2] ;
  float xlabel[2], ylabel[2], zlabel[2] ;

  DPRINT("\n(DrawGnomon") ;

  if (draw)
    {
      color.rgb.r = 1.0 ;
      color.rgb.g = 1.0 ;
      color.rgb.b = 1.0 ;
      xgl_object_set (XGLCTX,
		      XGL_CTX_LINE_COLOR, &color,
#ifdef solaris
		      XGL_CTX_STEXT_COLOR, &color,
#else
		      XGL_CTX_SFONT_TEXT_COLOR, &color,
#endif
		      NULL) ;
    }
  else
    {
      if (PDATA(redrawmode) != tdmViewEchoMode)
	{
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
	  
	  /* force graphics output into back (draw) buffer */
	  _dxf_BUFFER_CONFIG (PORT_CTX, &dummy, dummy, dummy, dummy, dummy,
			     &dummy, &dummy, tdmBackBufferDraw) ;
	  
	  /* erase gnomon background */
	  _dxf_WRITE_BUFFER (PORT_CTX, PDATA(illx), PDATA(illy),
			    PDATA(iurx), PDATA(iury), PDATA(background)) ;
	}

      DPRINT(")") ;
      return ;
    }

  origin[0] = 0.0 ;
  origin[1] = 0.0 ;

  xaxis[0] = 0.7 * rot[0][0] ; xaxis[1] = 0.7 * rot[0][1] ;
  yaxis[0] = 0.7 * rot[1][0] ; yaxis[1] = 0.7 * rot[1][1] ;
  zaxis[0] = 0.7 * rot[2][0] ; zaxis[1] = 0.7 * rot[2][1] ;

  /* set up normalized coordinates */
  _dxf_PUSH_REPLACE_MATRIX (PORT_CTX, (float (*)[4])identity) ;

  /* set up XGL point list */
  line.pt_type = XGL_PT_F3D ;
  line.bbox = NULL ;
  line.num_pts = 2 ;
  line.pts.f3d = pos ;
  pos[0].x = origin[0] ;
  pos[0].y = origin[1] ;
  pos[0].z = 0 ;
  pos[1].z = 0 ;
  
  /* draw x axis */
  pos[1].x = xaxis[0] ;
  pos[1].y = xaxis[1] ;
  xgl_multipolyline (XGLCTX, NULL, 1, &line) ;
  _dxf_DRAW_ARROWHEAD (PORT_CTX, xaxis[0], xaxis[1]) ;

  /* draw y axis */
  pos[1].x = yaxis[0] ;
  pos[1].y = yaxis[1] ;
  xgl_multipolyline (XGLCTX, NULL, 1, &line) ;
  _dxf_DRAW_ARROWHEAD (PORT_CTX, yaxis[0], yaxis[1]) ;

  /* draw z axis */
  pos[1].x = zaxis[0] ;
  pos[1].y = zaxis[1] ;
  xgl_multipolyline (XGLCTX, NULL, 1, &line) ;
  _dxf_DRAW_ARROWHEAD (PORT_CTX, zaxis[0], zaxis[1]) ;

  /* compute axes label positions */
  xlabel[0] = 0.8 * rot[0][0] ; xlabel[1] = 0.8 * rot[0][1] ;
  ylabel[0] = 0.8 * rot[1][0] ; ylabel[1] = 0.8 * rot[1][1] ;
  zlabel[0] = 0.8 * rot[2][0] ; zlabel[1] = 0.8 * rot[2][1] ;

  /* move labels left and/or down if they're going to hit arrowheads */
  if (xlabel[0] <= 0) xlabel[0] -= 0.15 ;
  if (xlabel[1] <= 0) xlabel[1] -= 0.15 ;

  if (ylabel[0] <= 0) ylabel[0] -= 0.15 ;
  if (ylabel[1] <= 0) ylabel[1] -= 0.15 ;

  if (zlabel[0] <= 0) zlabel[0] -= 0.15 ;
  if (zlabel[1] <= 0) zlabel[1] -= 0.15 ;

  /* render the axes labels */
  pos[1].x = 0 ; pos[1].y = 0 ;
#ifdef solaris
  xgl_object_set (XGLCTX, XGL_CTX_ATEXT_CHAR_HEIGHT, 0.15, 0) ;
#else
  xgl_object_set (XGLCTX, XGL_CTX_ANNOT_CHAR_HEIGHT, 0.15, 0) ;
#endif

  pos[0].x = xlabel[0] ; pos[0].y = xlabel[1] ;
  xgl_annotation_text (XGLCTX, "X", &pos[0], &pos[1]) ;

  pos[0].x = ylabel[0] ; pos[0].y = ylabel[1] ;
  xgl_annotation_text (XGLCTX, "Y", &pos[0], &pos[1]) ;

  pos[0].x = zlabel[0] ; pos[0].y = zlabel[1] ;
  xgl_annotation_text (XGLCTX, "Z", &pos[0], &pos[1]) ;

  /* return to pushed coordinate system */
  _dxf_POP_MATRIX(PORT_CTX) ;

  if (PDATA(redrawmode) != tdmViewEchoMode)
    {
      /* copy draw buffer to display buffer */
      Xgl_bounds_i2d rect ;
      Xgl_pt_i2d position ;

      position.x = PDATA(illx) ; position.y = VP_YMAX-PDATA(iury) ;
      rect.xmin =  PDATA(illx) ;  rect.ymin = VP_YMAX-PDATA(iury) ; 
      rect.xmax =  PDATA(iurx) ;  rect.ymax = VP_YMAX-PDATA(illy) ;
      
      /* ensure that draw buffer rendering is completed */
      xgl_context_post (XGLCTX, 0) ; 

      /* copy the combined gnomon and background image to display buffer */
#ifdef solaris
      xgl_object_set (XGLRAS,
		      XGL_RAS_SOURCE_BUFFER, DRAW_BUFFER,
		      NULL) ;
      xgl_object_set (XGLCTX,
		      XGL_CTX_RENDER_BUFFER, XGL_RENDER_DISPLAY_BUFFER,
		      NULL) ;
      xgl_context_copy_buffer
	  (XGLCTX, &rect, &position, XGLRAS) ;
      xgl_object_set (XGLCTX,
		      XGL_CTX_RENDER_BUFFER, XGL_RENDER_DRAW_BUFFER,
		      NULL) ;
#else
      xgl_context_copy_buffer
	  (XGLCTX, &rect, &position, DRAW_BUFFER, DISPLAY_BUFFER) ; 
#endif

      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmBackBufferDraw) ;
    }

  DPRINT(")") ;
} 


static void
_dxf_captureZoomBox (tdmInteractor I, void *udata, float rot[4][4])
{
  DEFDATA(I,tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_bounds_i2d rect ;
  Xgl_pt_i2d position ;
  int x1, y1, x2, y2 ;

  xgl_object_set(XGLTMPCTX, XGL_CTX_DEVICE, CDATA(image), NULL);

  /* left */
  rect.xmin =  MAX(0,x1-1) ;       rect.xmax =  rect.xmin + 1 ;
  rect.ymin =  MAX(0,VP_YMAX-y2) ; rect.ymax =  MAX(0,VP_YMAX-y1) ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLTMPCTX, &rect, &position, XGLRAS);
#else
  xgl_context_copy_raster (XGLTMPCTX, &rect, &position, XGLRAS);
#endif

      /* right */
  rect.xmin =  MAX(0,x2-1) ;       rect.xmax =  rect.xmin + 1 ;
  rect.ymin =  MAX(0,VP_YMAX-y2) ; rect.ymax =  MAX(0,VP_YMAX-y1) ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLTMPCTX, &rect, &position, XGLRAS);
#else
  xgl_context_copy_raster (XGLTMPCTX, &rect, &position, XGLRAS);
#endif

  /* bottom */
  rect.xmin =  MAX(0,x1) ;         rect.xmax =  MAX(0,x2) ;
  rect.ymin =  MAX(0,VP_YMAX-y1-1) ; rect.ymax =  rect.ymin + 1 ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLTMPCTX, &rect, &position, XGLRAS);
#else
  xgl_context_copy_raster (XGLTMPCTX, &rect, &position, XGLRAS);
#endif

  /* top */
  rect.xmin =  MAX(0,x1) ;         rect.xmax =  MAX(0,x2) ;
  rect.ymin =  MAX(0,VP_YMAX-y2-1) ; rect.ymax =  rect.ymin + 1 ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLTMPCTX, &rect, &position, XGLRAS);
#else
  xgl_context_copy_raster (XGLTMPCTX, &rect, &position, XGLRAS);
#endif
}

static void
_dxf_restoreZoomBox (tdmInteractor I, void *udata, float rot[4][4])
{
  DEFDATA(I,tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_bounds_i2d rect ;
  Xgl_pt_i2d position ;
  int x1, y1, x2, y2 ;

  /*
   *  Repair the image by blitting pixels from backing store.  The
   *  blits are expressed in XGL's notion of device coordinates, so we
   *  must flip the Y coordinates.
   */

  /* left */
  rect.xmin =  MAX(0,x1-1) ;       rect.xmax =  rect.xmin + 1 ;
  rect.ymin =  MAX(0,VP_YMAX-y2) ; rect.ymax =  MAX(0,VP_YMAX-y1) ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLCTX, &rect, &position, CDATA(image)) ;
#else
  xgl_context_copy_raster (XGLCTX, &rect, &position, CDATA(image)) ;
#endif

      /* right */
  rect.xmin =  MAX(0,x2-1) ;       rect.xmax =  rect.xmin + 1 ;
  rect.ymin =  MAX(0,VP_YMAX-y2) ; rect.ymax =  MAX(0,VP_YMAX-y1) ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLCTX, &rect, &position, CDATA(image)) ;
#else
  xgl_context_copy_raster (XGLCTX, &rect, &position, CDATA(image)) ;
#endif

  /* bottom */
  rect.xmin =  MAX(0,x1) ;         rect.xmax =  MAX(0,x2) ;
  rect.ymin =  MAX(0,VP_YMAX-y1-1) ; rect.ymax =  rect.ymin + 1 ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLCTX, &rect, &position, CDATA(image)) ;
#else
  xgl_context_copy_raster (XGLCTX, &rect, &position, CDATA(image)) ;
#endif

  /* top */
  rect.xmin =  MAX(0,x1) ;         rect.xmax =  MAX(0,x2) ;
  rect.ymin =  MAX(0,VP_YMAX-y2-1) ; rect.ymax =  rect.ymin + 1 ;
  position.x = rect.xmin ;         position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLCTX, &rect, &position, CDATA(image)) ;
#else
  xgl_context_copy_raster (XGLCTX, &rect, &position, CDATA(image)) ;
#endif
}

#undef _dxf_SERVICES_FLAGS
extern hwFlags _dxf_SERVICES_FLAGS();

void 
_dxf_DRAW_ZOOMBOX (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  /*
   *  draw == 1 to draw zoombox, draw == 0 to undraw.  This is done with
   *  two separate calls in order to support explicit erasure.
   */

  DEFDATA(I,tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int x1, y1, x2, y2 ;
  DPRINT("\n(DrawZoomBox") ;

  x1 = PDATA(x1) ; x2 = PDATA(x2) ;
  y1 = PDATA(y1) ; y2 = PDATA(y2) ;

  if (draw)
    {
      /*
       *  Coordinates are expressed in TDM's notion of device coordinates,
       *  so _dxf_SET_WORLD_SCREEN() needs to be called before this routine
       *  is invoked.
       */

      Xgl_color color ;
      Xgl_pt_list box ;
      Xgl_pt_f3d pos[5] ;

      if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE))
	      _dxf_captureZoomBox(I, udata, rot);

      color.rgb.r = 1.0 ; color.rgb.g = 1.0 ; color.rgb.b = 1.0 ;
      xgl_object_set (XGLCTX,
		      XGL_CTX_LINE_COLOR, &color,
		      NULL) ;
      
      box.pt_type = XGL_PT_F3D ;
      box.bbox = NULL ;
      box.num_pts = 5 ;
      box.pts.f3d = pos ;

      pos[0].x = x1 ; pos[0].y = y1 ; pos[0].z = 0 ;
      pos[1].x = x2 ; pos[1].y = y1 ; pos[1].z = 0 ;
      pos[2].x = x2 ; pos[2].y = y2 ; pos[2].z = 0 ;
      pos[3].x = x1 ; pos[3].y = y2 ; pos[3].z = 0 ;
      pos[4].x = x1 ; pos[4].y = y1 ; pos[4].z = 0 ;
      
      xgl_multipolyline (XGLCTX, NULL, 1, &box) ;
      xgl_context_post (XGLCTX, 0) ;
    }
  else if (CDATA(image))
    {
	_dxf_restoreZoomBox(I, udata, rot);
    }
  else
    {
      DPRINT("\nno backing store to erase zoombox") ;
    }

  DPRINT(")") ;
}


int
_dxf_GET_ZBUFFER_STATUS (void *ctx)
{
  /*
   *  return whether zbuffer is active or not
   */
  
  DEFCONTEXT(ctx) ;
  Xgl_hlhsr_mode zmode ;
  int status ;
  DPRINT("\n(_dxf_GET_ZBUFFER_STATUS:") ;

  xgl_object_get (XGLCTX, XGL_3D_CTX_HLHSR_MODE, &zmode) ;
  if (zmode == XGL_HLHSR_NONE)
      status = 0 ;
  else
      status = 1 ;

  DPRINT1 ("%d)", status) ;
  return status ;
}

void
_dxf_SET_ZBUFFER_STATUS (void *ctx, int status)
{
  /*
   *  set zbuffer off or on
   */

  DEFCONTEXT(ctx) ;
  DPRINT1 ("\n(_dxf_SET_ZBUFFER_STATUS:%d", status) ;

  if (status)
#ifdef solaris
      xgl_object_set (XGLCTX, XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_Z_BUFFER, 0) ;
#else
      xgl_object_set (XGLCTX, XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_ZBUFFER, 0) ;
#endif
  else
      xgl_object_set (XGLCTX, XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_NONE, 0) ;

  DPRINT(")") ;
}

void
_dxf_SET_SOLID_FILL_PATTERN (void *ctx)
{
  /*
   *  reset screen door half transparency to solid if used
   */

  DEFCONTEXT(ctx) ;
  DPRINT("\n(_dxf_SET_SOLID_FILL_PATTERN") ;

  xgl_object_set
      (XGLCTX,
       XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
       NULL) ;
  
  DPRINT(")") ;
}

void
_dxf_GET_WINDOW_ORIGIN (void *ctx, int *x, int *y)
{
  /*
   *  return LL corner of window relative to LL corner of screen in pixels
   */
  
  DEFCONTEXT(ctx) ;
  Xgl_pt_i2d origin ;
  DPRINT("\n(_dxf_GET_WINDOW_ORIGIN") ;

  xgl_object_get (XGLRAS, XGL_WIN_RAS_POSITION, &origin) ;

  *y = SCREEN_YMAX - (origin.y + VP_YMAX) ;
  *x = origin.x ;

  DPRINT1 ("\n%d,", *x) ; DPRINT1 (" %d)", *y) ;
}

void
_dxf_GET_MAXSCREEN (void *ctx, int *x, int *y)
{
  /*
   *  return maximum coordinates of entire screen in pixels
   */

  DEFCONTEXT(ctx) ;
  DPRINT("\n(_dxf_GET_MAXSCREEN") ;

  *x = SCREEN_XMAX ;
  *y = SCREEN_YMAX ;
  DPRINT1 ("\n%d,", *x) ; DPRINT1 (" %d)", *y) ;
}

void
_dxf_SET_WORLD_SCREEN (void *ctx, int w, int h)
{
  /*
   *  This routine can be ignored if API can write directly to DC,
   *  otherwise, must set up transforms for screen mode.
   */
  
  DEFCONTEXT(ctx) ;
  float S[4][4], xs, ys ;
  DPRINT("\n(_dxf_SET_WORLD_SCREEN") ;

  xs = 2.0 / ((w-1) + 0.5) ;
  ys = 2.0 / ((h-1) + 0.5) ;

  S[0][0] = xs ; S[0][1] =  0 ; S[0][2] = 0 ; S[0][3] = 0 ;
  S[1][0] =  0 ; S[1][1] = ys ; S[1][2] = 0 ; S[1][3] = 0 ;
  S[2][0] =  0 ; S[2][1] =  0 ; S[2][2] = 1 ; S[2][3] = 0 ;
  S[3][0] = -1 ; S[3][1] = -1 ; S[3][2] = 0 ; S[3][3] = 1 ;

  _dxf_SET_VIEWPORT (ctx, 0, w-1, 0, h-1) ;
  xgl_transform_write (PROJECTION_TRANSFORM, S) ;
  xgl_transform_write (MODEL_VIEW_TRANSFORM, identity) ;

  DPRINT(")") ;
}

void
_dxf_CREATE_CURSOR_PIXMAPS (tdmInteractor I)
{
  /*
   *  Cursors are pixmaps used for representing probe/roam positions.
   */
  
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_pt_i2d pos ;
  Xgl_color black, gray, green, white ;
  DPRINT("\n(xglTDM_CREATE_CURSOR_PIXMAPS") ;

  /* set up XGL rgb colors */
  green.rgb.r = 0.0 ;  green.rgb.g = 1.0 ; green.rgb.b = 0.0 ;
  gray.rgb.r  = 0.5 ;   gray.rgb.g = 0.5 ;  gray.rgb.b = 0.5 ;
  black.rgb.r = 0.0 ;  black.rgb.g = 0.0 ; black.rgb.b = 0.0 ;
  white.rgb.r = 1.0 ;  white.rgb.g = 1.0 ; white.rgb.b = 1.0 ;

  /* Initialize the Interactors */
  PDATA(cursor_saveunder) = PDATA(ActiveSquareCursor) =
        PDATA(PassiveSquareCursor) = PDATA(ProjectionMark) = NULL ;

  /* allocate a cursor save-under for the roam/probe interactor */
  if ( !(PDATA(cursor_saveunder) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
		PDATA(cursor_size), PDATA(cursor_size)) ))  goto error ;

  /* allocate and init active cursor pixmap: green outline, black interior */
  if ( !(PDATA(ActiveSquareCursor) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
		PDATA(cursor_size), PDATA(cursor_size)) ))  goto error ;

  /* set pixel array as rendering surface for temporary XGL context */
  xgl_object_set (XGLTMPCTX, XGL_CTX_DEVICE, PDATA(ActiveSquareCursor), NULL) ;

  for (pos.x=0 ; pos.x<PDATA(cursor_size) ; pos.x++)
      for (pos.y=0 ; pos.y<PDATA(cursor_size) ; pos.y++)
	  if (pos.x==0 || pos.x==PDATA(cursor_size)-1 ||
	      pos.y==0 || pos.y==PDATA(cursor_size)-1 )

	      xgl_context_set_pixel (XGLTMPCTX, &pos, &green) ;
	  else
	      xgl_context_set_pixel (XGLTMPCTX, &pos, &black) ;
	      
  /* allocate and init passive cursor pixmap: gray outline, black interior */
  if ( !(PDATA(PassiveSquareCursor) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
		PDATA(cursor_size), PDATA(cursor_size)) )) goto error ;

  /* set pixel array as rendering surface for temporary XGL context */
  xgl_object_set (XGLTMPCTX,
		  XGL_CTX_DEVICE, PDATA(PassiveSquareCursor),
		  NULL) ;

  for (pos.x=0 ; pos.x<PDATA(cursor_size) ; pos.x++)
      for (pos.y=0 ; pos.y<PDATA(cursor_size) ; pos.y++)
	  if (pos.x==0 || pos.x==PDATA(cursor_size)-1 ||
	      pos.y==0 || pos.y==PDATA(cursor_size)-1 )

	      xgl_context_set_pixel (XGLTMPCTX, &pos, &gray) ;
	  else
	      xgl_context_set_pixel (XGLTMPCTX, &pos, &black) ;
	      
  /* allocate and init projection mark pixmap */
  if ( !(PDATA(ProjectionMark) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, 3, 3)) )
	goto error ;

  /* set pixel array as rendering surface for temporary XGL context */
  xgl_object_set (XGLTMPCTX,
		  XGL_CTX_DEVICE, PDATA(ProjectionMark),
		  NULL) ;

  for (pos.x=0 ; pos.x<3 ; pos.x++)
      for (pos.y=0 ; pos.y<3 ; pos.y++)
	  xgl_context_set_pixel (XGLTMPCTX, &pos, &white) ;
	      
  DPRINT(")") ;
  return ;

error:
  DPRINT("\nerror)" ) ;
  PDATA(cursor_saveunder) = PDATA(ActiveSquareCursor) =
        PDATA(PassiveSquareCursor) = PDATA(ProjectionMark) = NULL ;
  return ;

}

void
_dxf_DRAW_MARKER (tdmInteractor I)
{
  /*
   *  Draw projection marks for the probe/roam interactor.  Marks are the
   *  projections of a probe/roam interactor's position onto the bounding
   *  box.
   */
  
  DEFDATA(I,CursorData) ;	
  DEFPORT(I_PORT_HANDLE) ;
  register int i, mx, my ;
  Xgl_rect_af3d rect ;
  Xgl_rect_list rect_list ;
  DPRINT("\n(_dxf_DRAW_MARKER") ;

  for (i = 0 ; i < PDATA(iMark) ; i++) 
    {
      PDATA(pXmark)[i] = PDATA(Xmark)[i] ;
      PDATA(pYmark)[i] = PDATA(Ymark)[i] ;
    }

  PDATA(piMark) = PDATA(iMark) ;
  
  for (i = 0 ; i < PDATA(iMark) ; i++)
    {
      /* round marker to int */
      mx = (int)(PDATA(Xmark[i])+0.5) ;
      my = (int)(PDATA(Ymark[i])+0.5) ;
      
      /* blit it */
      if (!CLIPPED(mx,my)) 
	  _dxf_WRITE_BUFFER (PORT_CTX, mx-1, my-1, mx+1, my+1,
			    PDATA(ProjectionMark)) ;
    }
  DPRINT(")") ;
}

void
_dxf_ERASE_PREVIOUS_MARKS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_pt_i2d position ;
  Xgl_bounds_i2d rect ;
  register int i, j, mx, my ;

  /* do nothing if interactor is invisible or backing store not available */
  if (!PDATA(visible) || !CDATA(image)) return ;

  /* restore the image over the previous projection marks */
  for (i = 0 ; i < PDATA(piMark) ; i++)
    {
      /* round mark coordinates to int */
      mx = (long)(PDATA(pXmark[i])+0.5) ;
      my = (long)(PDATA(pYmark[i])+0.5) ;
      
      if (!CLIPPED(mx,my))
	{
	  rect.xmin = mx-1 ; rect.ymin = VP_YMAX - (my+1) ;
	  rect.xmax = mx+1 ; rect.ymax = VP_YMAX - (my-1) ;
	  position.x = rect.xmin ; position.y = rect.ymin ;
#ifdef solaris
	  xgl_context_copy_buffer (XGLCTX, &rect, &position, CDATA(image)) ;
#else
	  xgl_context_copy_raster (XGLCTX, &rect, &position, CDATA(image)) ;
#endif
	}
    }
}

void
_dxf_ERASE_CURSOR (tdmInteractor I, int x, int y)
{
  /*
   *  Erase previous probe/roam cursor position by blitting stored image.
   */
  
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_pt_i2d position ;
  Xgl_bounds_i2d rect ;
  int s2 ;
  
  /* if not visible, nor backing store available, nor within window, return */
  if (!PDATA(visible) || !CDATA(image) || CLIPPED(x,y)) return ;

  s2= PDATA(cursor_size) / 2 ;
  rect.xmin = x-s2 ; rect.ymin = VP_YMAX - (y+s2) ;
  rect.xmax = x+s2 ; rect.ymax = VP_YMAX - (y-s2) ;
  position.x = rect.xmin ; position.y = rect.ymin ;
#ifdef solaris
  xgl_context_copy_buffer (XGLCTX, &rect, &position, CDATA(image)) ;
#else
  xgl_context_copy_raster (XGLCTX, &rect, &position, CDATA(image)) ;
#endif
}


void
_dxf_POINTER_INVISIBLE (void *ctx)
{
  /*
   *  Substitute an invisible mouse pointer.
   */
  
  DEFCONTEXT(ctx) ;
  int i ;
  Pixmap p1, p2 ;
  XColor fc, bc ;
  char tmp_bits[8] ;
  Cursor cursorId ;
  DPRINT("\n(_dxf_POINTER_INVISIBLE)") ;

  for (i = 0 ; i < 8 ; i++)
    tmp_bits[i] = 0 ;

  p1 = XCreatePixmapFromBitmapData
     (XGLDISPLAY, RootWindow(XGLDISPLAY, XGLSCREEN), tmp_bits, 4, 4, 0, 0, 1) ;
  p2 = XCreatePixmapFromBitmapData
     (XGLDISPLAY, RootWindow(XGLDISPLAY, XGLSCREEN), tmp_bits, 4, 4, 0, 0, 1) ;

  cursorId = XCreatePixmapCursor (XGLDISPLAY, p1, p2, &fc, &bc, 1, 1 ) ;

  XFreePixmap (XGLDISPLAY, p1) ;
  XFreePixmap (XGLDISPLAY, p2) ;

  XDefineCursor (XGLDISPLAY, XGLXWINID, cursorId) ; 
}

void
_dxf_POINTER_VISIBLE (void *ctx)
{ 
  /*
   *  Use the default visible mouse pointer.
   */
  
  DEFCONTEXT(ctx) ;
  DPRINT("\n(_dxf_POINTER_VISIBLE)") ;

  XUndefineCursor (XGLDISPLAY, XGLXWINID) ;
}

void
_dxf_WARP_POINTER (void *ctx, int x, int y)
{
  /*
   *  Warp mouse pointer to specified position.
   */
  
  DEFCONTEXT(ctx) ;
  DPRINT("\n(_dxf_WARP_POINTER)") ;

  XWarpPointer (XGLDISPLAY, None, DefaultRootWindow(XGLDISPLAY), 0, 0, 0, 0,
		x, SCREEN_YMAX-y) ;
}

static void
_dxf_SET_LINE_ATTRIBUTES (void *ctx, int32 color, int style, float width)
{
  DEFCONTEXT(ctx) ;
  Xgl_color xgl_color ;

  /*
   *  color is packed RGB.
   *  style 0 is solid.  style 1 is dashed.
   *  width is screen width of line, may be subpixel.  not currently used.
   */

  switch (color)
    {
    case 0x8f8f8f8f:
      xgl_color.rgb.r = 0.5 ; xgl_color.rgb.g = 0.5 ; xgl_color.rgb.b = 0.5 ;
      break ;
    case 0xffffffff:
    default:
      xgl_color.rgb.r = 1.0 ; xgl_color.rgb.g = 1.0 ; xgl_color.rgb.b = 1.0 ;
      break ;
    }

  xgl_object_set (XGLCTX, XGL_CTX_LINE_COLOR, &xgl_color, NULL) ;

  switch (style)
    {
    case 1:
      xgl_object_set (XGLCTX, XGL_CTX_LINE_STYLE, XGL_LINE_PATTERNED, 0) ;
      xgl_object_set (XGLCTX, XGL_CTX_LINE_PATTERN, xgl_lpat_dashed, 0);
      break ;
    case 0:
    default:
      xgl_object_set (XGLCTX, XGL_CTX_LINE_STYLE, XGL_LINE_SOLID, 0) ;
      break ;
    }
}


void
_dxf_SET_LINE_COLOR_WHITE (void *ctx)
{
  /* obsolete entry:  do not use in new code */ 
  DEFCONTEXT(ctx) ;
  Xgl_color color  ;

  color.rgb.r = 1.0 ; color.rgb.g = 1.0 ; color.rgb.b = 1.0 ;
  xgl_object_set (XGLCTX, XGL_CTX_LINE_COLOR, &color, NULL) ;
}
  
void
_dxf_SET_LINE_COLOR_GRAY (void *ctx)
{
  /* obsolete entry:  do not use in new code */
  DEFCONTEXT(ctx) ;
  Xgl_color color ;

  color.rgb.r = 0.5 ; color.rgb.g = 0.5 ; color.rgb.b = 0.5 ;
  xgl_object_set (XGLCTX, XGL_CTX_LINE_COLOR, &color, NULL) ;
}
  
void
_dxf_DRAW_LINE(void *ctx, int x1, int y1, int x2, int y2)
{
  /*
   *  Used for cursor box.
   */

  DEFCONTEXT(ctx) ;
  Xgl_pt_list line  ;
  static Xgl_pt_f3d verts[2] = {0, 0, 0, 0, 0, 0} ;

  DPRINT("\n(_dxf_DRAW_LINE") ;
  DPRINT1("\n%d", x1) ; DPRINT1(" %d", y1) ;
  DPRINT1("\n%d", x2) ; DPRINT1(" %d", y2) ;

  line.pt_type = XGL_PT_F3D ;
  line.bbox = NULL ;
  line.num_pts = 2 ;
  line.pts.f3d = verts ;

  verts[0].x = x1 ;
  verts[0].y = y1 ;
  verts[1].x = x2 ;
  verts[1].y = y2 ;

  xgl_multipolyline (XGLCTX, NULL, 1, &line) ;
  DPRINT(")") ;
}

void
_dxf_DRAW_CURSOR_COORDS (tdmInteractor I, char *text)
{
  /*
   *  Draw cursor coordinates at specified position.
   */
  
  DEFDATA(I,CursorData) ;	
  DEFPORT(I_PORT_HANDLE) ;
  Xgl_pt_f3d pos, offs ;
  Xgl_color white, black ;

  white.rgb.r = 1.0 ;
  white.rgb.g = 1.0 ;
  white.rgb.b = 1.0 ;

  black.rgb.r = 0.0 ;
  black.rgb.g = 0.0 ;
  black.rgb.b = 0.0 ;

  offs.x = 0 ; offs.y = 0 ; offs.z = 0 ; pos.z = 0 ;

#ifdef solaris
  xgl_object_set (XGLCTX,
		  XGL_CTX_ATEXT_CHAR_HEIGHT, 0.036,
		  XGL_CTX_STEXT_CHAR_EXPANSION_FACTOR, 0.7,
		  XGL_CTX_STEXT_COLOR, &black,
		  NULL) ;
#else
  xgl_object_set (XGLCTX,
		  XGL_CTX_ANNOT_CHAR_HEIGHT, 0.036,
		  XGL_CTX_SFONT_CHAR_EXPANSION_FACTOR, 0.7,
		  XGL_CTX_SFONT_TEXT_COLOR, &black,
		  NULL) ;
#endif

  pos.x = PDATA(coord_llx)+5 ; pos.y = PDATA(coord_lly)+3 ; 
  xgl_annotation_text (XGLCTX, text, &pos, &offs) ;

  xgl_object_set (XGLCTX,
#ifdef solaris
		  XGL_CTX_STEXT_COLOR, &white,
#else
		  XGL_CTX_SFONT_TEXT_COLOR, &white,
#endif
		  NULL) ;

  pos.x = PDATA(coord_llx)+4 ; pos.y = PDATA(coord_lly)+4 ;
  xgl_annotation_text (XGLCTX, text, &pos, &offs) ;

}

tdmInteractorEchoT	_dxd_hwInteractorEchoPort = 
{
  /* BufferConfig */		_dxf_BUFFER_CONFIG,
  /* BufferRestoreConfig */	_dxf_BUFFER_RESTORE_CONFIG,
  /* CreateCurXFsorPixmaps */	_dxf_CREATE_CURSOR_PIXMAPS,
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
