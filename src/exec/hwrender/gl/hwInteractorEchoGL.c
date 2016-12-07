/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/gl/hwInteractorEchoGL.c,v $

  These are the IBM GL implementations of the TDM direct interactor echos,
  along with miscellaneous utility routines.  Each function needs to be
  ported to the native graphics API of the target system in order to provide
  support for direct interactors.

  Applications such as DX can be ported as a first step without direct
  interactor facilities by stubbing out everything here.

\*---------------------------------------------------------------------------*/

#include "hwDeclarations.h"

#if defined(sgi)
#include <get.h>
#endif

#define X_H
#define Cursor glCursor
#define String glString
#define Boolean glBoolean
#define Object glObject
#include <gl/gl.h>
#undef Object
#undef Boolean
#undef String
#undef Cursor
#undef X_H

#include <gl/device.h>
#include "hwMatrix.h"
#include "hwInteractor.h"
#include "hwCursorInteractor.h"
#include "hwRotateInteractor.h"
#include "hwZoomInteractor.h"
#include "hwGlobeEchoDef.h"

#include "hwDebug.h"

#ifndef STANDALONE
/*#include "hwMemory.h"*/
#endif

#include "hwPortLayer.h"

#define IMAGE(type,x,y) \
(CLIPPED(x,y) ? 0 : ((type *)(CDATA(image))) [(x)+(y)*CDATA(iw)])

static void 
_dxf_BUFFER_CONFIG (void *ctx, void *image, int llx, int lly, int urx, int ury,
		    int *CurrentDisplayMode, int *CurrentBufferMode,
		    tdmInteractorRedrawMode redrawMode) 
{ 
  /*
   *  Direct interactor echos use this and following routine to manage
   *  double buffering.
   */

  ENTRY(("_dxf_BUFFER_CONFIG (0x%x, 0x%x, %d, %d, %d, %d, 0x%x, 0x%x, %d)",
	 ctx, image, llx, lly, urx, ury,
	 CurrentDisplayMode, CurrentBufferMode, redrawMode));

  /* record current GL buffer mode */
  *CurrentBufferMode = getbuffer() ;
  
  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw)
    {
      PRINT(("%s", redrawMode == tdmBothBufferDraw ?
	     "tdmBothBufferDraw" : "tdmFrontBufferDraw"));

      /*
       *  Interactor echos no longer double buffered, so don't bother
       *  keeping front and back buffer contents identical.
       */
      
      frontbuffer(TRUE) ;
      backbuffer(FALSE) ;
    }
  else if (redrawMode == tdmBackBufferDraw ||
	   redrawMode == tdmViewEchoMode)
    {
      PRINT(("%s", redrawMode == tdmViewEchoMode ?
	     "tdmViewEchoMode" : "tdmBackBufferDraw"));

      frontbuffer(FALSE) ;
      backbuffer(TRUE) ;
    }
  else
    PRINT(("ignoring redraw mode"));

  EXIT((""));
}


static void
_dxf_BUFFER_RESTORE_CONFIG (void *ctx, int OrigDisplayMode, int OrigBufferMode,
			    tdmInteractorRedrawMode redrawMode)
{
  /*
   *  Restore frame buffer configuration.
   */

  ENTRY(("_dxf_BUFFER_RESTORE_CONFIG(0x%x, %d, %d, %d)",
	 ctx, OrigDisplayMode, OrigBufferMode, redrawMode));

  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw)
    {
      PRINT(("current interactor redraw mode is %s",
	     redrawMode == tdmFrontBufferDraw ?
	     "tdmFrontBufferDraw" : "tdmBothBufferDraw"));

      if (OrigBufferMode == BCKBUFFER)
        {
          PRINT(("restoring to original back buffer draw"));
	  frontbuffer(FALSE) ;
	  backbuffer(TRUE) ;
	}
      else
	PRINT(("remaining in original front buffer draw"));
    }
  else if (redrawMode == tdmBackBufferDraw ||
	   redrawMode == tdmViewEchoMode)
    {
      PRINT(("current interactor redraw mode is %s",
	     redrawMode == tdmViewEchoMode ?
	     "tdmViewEchoMode" : "tdmBackBufferDraw"));

      if (OrigBufferMode == FRNTBUFFER)
        {
          PRINT(("restoring to original front buffer draw"));
	  frontbuffer(TRUE) ;
	  backbuffer(FALSE) ;
	}
      else
	PRINT(("remaining in original back buffer draw"));
    }
  else
    PRINT(("\nignoring redraw mode"));

  EXIT((""));
}

static void
_dxf_READ_BUFFER (void *ctx, int llx, int lly, int urx, int ury, void *buff)
{
  ENTRY(("_dxf_READ_BUFFER (0x%x, %d, %d, %d, %d, 0x%x)",
	 ctx, llx, lly, urx, ury, buff));

  readsource(SRC_FRONT) ;
  lrectread (llx, lly, urx, ury, (unsigned long *)buff) ;

  EXIT((""));
}

static void
_dxf_WRITE_BUFFER (void *ctx, int llx, int lly, int urx, int ury, void *buff)
{
  ENTRY(("_dxf_WRITE_BUFFER(0x%x, %d, %d, %d, %d, 0x%x)",
	 ctx, llx, lly, urx, ury, buff));

  lrectwrite (llx, lly, urx, ury, (const unsigned long *)buff) ;

  EXIT((""));
}

static void 
_dxf_DRAW_GLOBE (tdmInteractor I, void *udata, float rot[4][4], int draw)
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
  register const float (*Globe)[LONGS][3] = globe ;
  register const struct Face (*Globeface)[LONGS] = globeface ;

  /* view normal */
  register float z0, z1, z2 ;
  z0 = rot[0][2] ; z1 = rot[1][2] ; z2 = rot[2][2] ;

#define FACEVISIBLE(u,v,z0,z1,z2) \
  (Globeface[u][v].norm[0] * z0 + \
   Globeface[u][v].norm[1] * z1 + \
   Globeface[u][v].norm[2] * z2 > 0.0)

  ENTRY(("_dxf_DRAW_GLOBE (0x%x, 0x%x, 0x%x, %d)",I, udata, rot, draw));

  if (draw)
    {
      lmcolor(LMC_COLOR) ;
      cpack(0xffffffff) ;
      linewidth(1) ;
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
	  frontbuffer(FALSE) ;
	  backbuffer(TRUE) ;
	  
	  /* erase globe background */
	  lrectwrite (PDATA(illx), PDATA(illy),
		      PDATA(iurx), PDATA(iury), PDATA(background)) ;
	}

#ifndef NOSHADOW      
      /* draw wide black lines to ensure visibility against background */
      lmcolor(LMC_COLOR) ;
      cpack(0x0) ;
      linewidth(2) ;
#else
      EXIT(("No shadow"));
      return ;
#endif
    }

#ifndef FACEVIS
  /*
   *  Compute visible edges explicitly.  This method might be faster and
   *  works in XOR mode but as implemented here is applicable only for a
   *  globe-type object rendered with latitude and longitude lines.
   */
#ifndef NOSHADOW
  if (!draw)
#endif
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

  /*
   *  Draw each visible edge exactly once.
   */
  for (u=0 ; u<LATS ; u++)
    {
      for (v=0, on=0 ; v<LONGS-1 ; v++)
	  if (edges[u][v].latvis)
	    {
	      if (!on)
		{
		  on = 1 ;
		  bgnline() ;
		  v3f(Globe[u][v]) ;
		}
	      v3f (Globe[u][v+1]) ;
#ifndef NOSHADOW
	      if (draw)
#endif
		  edges[u][v].latvis = 0 ;
	    }
	  else if (on)
	    {
	      on = 0 ;
	      endline() ;
	    }

      /* close latitude line if necessary */
      if (edges[u][LONGS-1].latvis)
	{
	  if (!on)
	    {
	      bgnline() ;
	      v3f(Globe[u][LONGS-1]) ;
	    }
	  v3f (Globe[u][0]) ;
	  endline() ;
#ifndef NOSHADOW
	  if (draw)
#endif
	      edges[u][LONGS-1].latvis = 0 ;
	}
      else if (on)
	  endline() ;
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
		  bgnline() ;
		  v3f(Globe[u][v]) ;
		}
	      v3f(Globe[u+1][v]) ;
#ifndef NOSHADOW
	      if (draw)
#endif
		  edges[u][v].longvis = 0 ;
	    }
	  else if (on)
	    {
	      on = 0 ;
	      endline() ;
	    }
      if (on)
	  endline() ;
    }
#else  
  /*
   *  Do it the easy way:  draw all visible faces regardless of shared
   *  edges.  Most edges are drawn twice, so this is slower and not
   *  compatible with XOR rendering.
   */
  for (u=0 ; u<LATS-1 ; u++)
      for (v=0 ; v<LONGS ; v++)
	  if (FACEVISIBLE(u, v, z0, z1, z2))
	      poly (4, Globeface[u][v].face) ;

  /* north pole */
  if (z1 > 0.0)
      poly (LONGS, Globe[LATS-1]) ;

  /* south pole */
  if (z1 < 0.0)
      poly (LONGS, Globe[0]) ;

#endif /* ifndef FACEVIS */

  if (draw && PDATA(redrawmode) != tdmViewEchoMode)
    {
      /* copy rendered globe from back buffer to front buffer */
      readsource(SRC_BACK) ;
      frontbuffer(TRUE) ;
      backbuffer(FALSE) ;
      rectcopy (PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
		PDATA(illx), PDATA(illy)) ;

      /* restore original buffer config from current tdmFrontBufferDraw */
      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmFrontBufferDraw) ;
    }
  EXIT((""));
}

static void
_dxf_DRAW_ARROWHEAD (void* ctx, float ax, float ay)
{
  double angle, hyp, length ;
  float x1, y1 ;
  float p[2] ;
  
  ENTRY(("_dxf_DRAW_ARROWHEAD(0x%x, %f, %f)",ctx, ax, ay));

  hyp = sqrt(ax*ax + ay*ay) ;

  if (hyp == 0.0) {
    EXIT(("hyp == 0.0"));
    return ;
  }
  
  if (hyp < 0.2)
      length = 0.06 ;
  else
      length = 0.12 ;
  
  angle = acos((double)(-ax/hyp)) ;
  if (ay > 0)
      angle = 2*M_PI - angle ;
  
  angle = angle + M_PI/6 ;
  if (angle > 2*M_PI)
      angle = angle - 2*M_PI ;

  x1 = cos(angle) * length ;
  y1 = sin(angle) * length ;
  
  bgnline() ;
  p[0] = ax ;      p[1] = ay ;      v2f(p) ;
  p[0] = ax + x1 ; p[1] = ay + y1 ; v2f(p) ;
  endline() ;
  
  angle = angle - M_PI/3 ;
  if (angle < 0)
      angle = angle + 2*M_PI ;

  x1 = cos(angle) * length ;
  y1 = sin(angle) * length ;
  
  bgnline() ;
  p[0] = ax ;      p[1] = ay ;      v2f(p) ;
  p[0] = ax + x1 ; p[1] = ay + y1 ; v2f(p) ;
  endline() ;

  EXIT((""));
}

static void 
_dxf_DRAW_GNOMON (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  /*
   *  draw == 1 to draw gnomon, draw == 0 to undraw.  This is done with
   *  two separate calls in order to support explicit erasure of edges for
   *  some implementations.  A draw is always preceded by an undraw and
   *  the pair of invocations is atomic.
   *
   *  Computations are done in normalized screen coordinates in order to
   *  render arrow heads correctly.
   */

  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int dummy = 0 ;
  float origin[2] ;
  float xaxis[2],  yaxis[2],  zaxis[2] ;
  float xlabel[2], ylabel[2], zlabel[2] ;

  ENTRY(("_dxf_DRAW_GNOMON (0x%x, 0x%x, 0x%x, %d)",I, udata, rot, draw));

  if (PDATA(font) == -1)
    {
      /* font width for axes labels in normalized coordinates */
      font(0) ;
      PDATA(font) = 0 ;
      PDATA(swidth) = (float)strwidth("Z")/(float)GNOMONRADIUS ;

      /* 1 pixel in normalized coordinates */
      PDATA(nudge) = 1.0/(float)GNOMONRADIUS ;
    }
  else
      font(PDATA(font)) ;
  
  if (draw)
    {
      lmcolor(LMC_COLOR) ;
      cpack(0xffffffff) ;
      linewidth(1) ;
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
	  
	  /* force graphics output into back buffer */
	  frontbuffer(FALSE) ;
	  backbuffer(TRUE) ;
	  
	  /* erase gnomon background */
	  lrectwrite (PDATA(illx), PDATA(illy),
		      PDATA(iurx), PDATA(iury), PDATA(background)) ;
	}

#ifndef NOSHADOW      
      /* draw wide black lines to ensure visibility against background */
      lmcolor(LMC_COLOR) ;
      cpack(0x0) ;
      linewidth(2) ;
#else
      EXIT(("No shadow"));
      return ;
#endif
    }

  origin[0] = 0 ;
  origin[1] = 0 ;

  xaxis[0] = 0.7 * rot[0][0] ; xaxis[1] = 0.7 * rot[0][1] ;
  yaxis[0] = 0.7 * rot[1][0] ; yaxis[1] = 0.7 * rot[1][1] ;
  zaxis[0] = 0.7 * rot[2][0] ; zaxis[1] = 0.7 * rot[2][1] ;

  xlabel[0] = 0.8 * rot[0][0] ; xlabel[1] = 0.8 * rot[0][1] ;
  ylabel[0] = 0.8 * rot[1][0] ; ylabel[1] = 0.8 * rot[1][1] ;
  zlabel[0] = 0.8 * rot[2][0] ; zlabel[1] = 0.8 * rot[2][1] ;

  pushmatrix() ;
  loadmatrix(identity) ;
  bgnline() ; v2f(origin) ; v2f(xaxis) ; endline() ;
  _dxf_DRAW_ARROWHEAD(PORT_CTX, xaxis[0], xaxis[1]) ;

  bgnline() ; v2f(origin) ; v2f(yaxis) ; endline() ;
  _dxf_DRAW_ARROWHEAD(PORT_CTX, yaxis[0], yaxis[1]) ;

  bgnline() ; v2f(origin) ; v2f(zaxis) ; endline() ;
  _dxf_DRAW_ARROWHEAD(PORT_CTX, zaxis[0], zaxis[1]) ;
  
  if (xlabel[0] <= 0) xlabel[0] -= PDATA(swidth) ;
  if (xlabel[1] <= 0) xlabel[1] -= PDATA(swidth) ;
  
  if (ylabel[0] <= 0) ylabel[0] -= PDATA(swidth) ;
  if (ylabel[1] <= 0) ylabel[1] -= PDATA(swidth) ;
  
  if (zlabel[0] <= 0) zlabel[0] -= PDATA(swidth) ;
  if (zlabel[1] <= 0) zlabel[1] -= PDATA(swidth) ;

#ifndef NOSHADOW  
  if (!draw)
    {
      /* offset text slightly for shadow */
      xlabel[0] += PDATA(nudge) ; xlabel[1] -= PDATA(nudge) ;
      ylabel[0] += PDATA(nudge) ; ylabel[1] -= PDATA(nudge) ;
      zlabel[0] += PDATA(nudge) ; zlabel[1] -= PDATA(nudge) ;
    }
#endif

  font(0) ;
  cmov2 (xlabel[0], xlabel[1]) ;
  charstr ("X") ;
  cmov2 (ylabel[0], ylabel[1]) ;
  charstr ("Y") ;
  cmov2 (zlabel[0], zlabel[1]) ;
  charstr ("Z") ;

  popmatrix() ;

  if (draw && PDATA(redrawmode) != tdmViewEchoMode)
    {
      /* copy rendered gnomon from back buffer to front buffer */
      readsource(SRC_BACK) ;
      frontbuffer(TRUE) ;
      backbuffer(FALSE) ;
      rectcopy (PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
		PDATA(illx), PDATA(illy)) ;

      /* restore original buffer config from current tdmFrontBufferDraw */
      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmFrontBufferDraw) ;
    }

  EXIT((""));
} 

static void
_dxf_restoreZoomBox(tdmInteractor I, void *udata, float rot[4][4])
{
  register long *bp, *image ;
  register int i, iw, ih, n, y2 ;
  int x1, x2, y1 ;
  static long buff[4096] ;
  DEFDATA(I, tdmZoomData) ;

  iw = CDATA(iw) ;
  ih = CDATA(ih) ;
  image = (long *)CDATA(image) ;

  /* clip coords to image to avoid invalid raster positions */
  x1 = (PDATA(x1)<0 ? 0 : (PDATA(x1)>(iw-1) ? iw-1 : PDATA(x1))) ;
  x2 = (PDATA(x2)<0 ? 0 : (PDATA(x2)>(iw-1) ? iw-1 : PDATA(x2))) ;
  y1 = (PDATA(y1)<0 ? 0 : (PDATA(y1)>(ih-1) ? ih-1 : PDATA(y1))) ;
  y2 = (PDATA(y2)<0 ? 0 : (PDATA(y2)>(ih-1) ? ih-1 : PDATA(y2))) ;
  
  lrectwrite (x1, y1, x2, y1, (const unsigned long *)image+(x1 + y1*iw)) ;
  if (y1 < (ih+1))
      lrectwrite (x1, y1+1, x2, y1+1,
		(const unsigned long *)image+(x1 + (y1+1)*iw)) ;

  lrectwrite (x1, y2, x2, y2, (const unsigned long *)image+(x1 + y2*iw)) ;
  if (y2 < (ih+1))
      lrectwrite (x1, y1+1, x2, y1+1,
		(const unsigned long *)image+(x1 + (y1+1)*iw)) ;

  for (bp=buff, n=x1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
      *bp++ = image[n] ;
  lrectwrite (x1, y1, x1, y2, (const unsigned long *)buff) ;

  if (x1 < iw-1)
  {
      for (bp=buff, n=x1+1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
	  *bp++ = image[n] ;
      lrectwrite (x1+1, y1, x1+1, y2, (const unsigned long *)buff) ;
  }

  for (bp=buff, n=x2+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
      *bp++ = image[n] ;
  lrectwrite (x2, y1, x2, y2, (const unsigned long *)buff) ;

  if (x2 < iw-1)
  {
      for (bp=buff, n=x2+1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
	  *bp++ = image[n] ;
      lrectwrite (x2+1, y1, x2+1, y2, (const unsigned long *)buff) ;
  }

}

static void
_dxf_captureZoomBox(tdmInteractor I, void *udata, float rot[4][4])
{
  register long *bp, *image ;
  register int i, iw, ih, n, y2 ;
  int x1, x2, y1 ;
  static long buff[4096] ;
  DEFDATA(I, tdmZoomData) ;

  readsource(SRC_FRONT);

  iw = CDATA(iw) ;
  ih = CDATA(ih) ;
  image = (long *)CDATA(image) ;

  /* clip coords to image to avoid invalid raster positions */
  x1 = (PDATA(x1)<0 ? 0 : (PDATA(x1)>(iw-1) ? iw-1 : PDATA(x1))) ;
  x2 = (PDATA(x2)<0 ? 0 : (PDATA(x2)>(iw-1) ? iw-1 : PDATA(x2))) ;
  y1 = (PDATA(y1)<0 ? 0 : (PDATA(y1)>(ih-1) ? ih-1 : PDATA(y1))) ;
  y2 = (PDATA(y2)<0 ? 0 : (PDATA(y2)>(ih-1) ? ih-1 : PDATA(y2))) ;

  lrectread (x1, y1, x2, y1, (const unsigned long *)image+(x1 + y1*iw)) ;
  if (y1 < (ih+1))
      lrectread (x1, y1+1, x2, y1+1,
			(const unsigned long *)image+(x1 + (y1+1)*iw)) ;

  lrectread (x1, y2, x2, y2, (const unsigned long *)image+(x1 + y2*iw)) ;
  if (y2 < (ih+1))
      lrectread (x1, y2+1, x2, y2+1, 
			(const unsigned long *)image+(x1 + (y2+1)*iw)) ;

  lrectread (x1, y1, x1, y2, (const unsigned long *)buff) ;
  for (bp=buff, n=x1+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
      image[n] = *bp++;
  if (x1 < iw-1)
  {
      lrectread (x1+1, y1, x1+1, y2, (const unsigned long *)buff) ;
      for (bp=buff, n=(x1+1)+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
	  image[n] = *bp++;
  }

  lrectread (x2, y1, x2, y2, (const unsigned long *)buff) ;
  for (bp=buff, n=x2+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
      image[n] = *bp++;

  if (x2 < iw-1)
  {
      lrectread (x2+1, y1, x2+1, y2, (const unsigned long *)buff) ;
      for (bp=buff, n=(x2+1)+(y1*iw), i=y1 ; i<=y2 ; i++, n+=iw)
	  image[n] = *bp++;
  }
}

#undef _dxf_SERVICES_FLAGS
extern hwFlags _dxf_SERVICES_FLAGS();

static void 
_dxf_DRAW_ZOOMBOX (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  DEFDATA(I, tdmZoomData) ;

  ENTRY(("_dxf_DRAW_ZOOMBOX (0x%x, 0x%x, 0x%x, %d)",I, udata, rot, draw));

  if (draw)
  {
      if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE))
	  _dxf_captureZoomBox(I, udata, rot);

      /* draw the box */
      lmcolor(LMC_COLOR) ;
      cpack(0xffffffff) ;
      linewidth(1) ;
      recti(PDATA(x1), PDATA(y1), PDATA(x2), PDATA(y2)) ;
  }
  else
      _dxf_restoreZoomBox(I, udata, rot);

  EXIT((""));
}

static int
_dxf_GET_ZBUFFER_STATUS (void *ctx)
{
  int status ;

  ENTRY(("_dxf_GET_ZBUFFER_STATUS(0x%x)",ctx));

  status = getzbuffer() ;

  EXIT(("status = %d", status));
  return status ;
}

static void
_dxf_SET_ZBUFFER_STATUS (void *ctx, int status)
{
  ENTRY(("_dxf_SET_ZBUFFER_STATUS(0x%x, %d)",ctx,status));

  zbuffer(status) ;

  EXIT((""));
}

static void
_dxf_SET_SOLID_FILL_PATTERN (void *ctx)
{
  ENTRY(("_dxf_SET_SOLID_FILL_PATTERN(0x%x)", ctx));

  setpattern(0) ;

  EXIT((""));
}

static void
_dxf_GET_WINDOW_ORIGIN (void *ctx, int *x, int *y)
{
  /*
   *  return lower left corner of window relative to screen in pixels
   */

  ENTRY(("_dxf_GET_WINDOW_ORIGIN(0x%x, 0x%x, 0x%x)",ctx, x, y));

  getorigin ((long *)x, (long *)y) ;

  EXIT(("x = %d y = %d", *x, *y));
}

static void
_dxf_GET_MAXSCREEN (void *ctx, int *x, int *y)
{
  /*
   *  return width of entire screen in pixels
   */
  ENTRY(("_dxf_GET_MAXSCREEN(0x%x, 0x%x, 0x%x)",ctx, x, y));

  *x = XMAXSCREEN ;
  *y = YMAXSCREEN ;

  EXIT((""));
}

static void
_dxf_SET_WORLD_SCREEN (void *ctx, int w, int h)
{
  /*
   *  set up a one-to-one mapping of world to screen
   */

  ENTRY(("_dxf_SET_WORLD_SCREEN(0x%x, %d, %d)",ctx, w, h));

  viewport (0, w-1, 0, h-1) ;
  ortho2 (-0.5, (float)w-0.5, -0.5, (float)h-0.5) ;
  loadmatrix(identity) ;

  EXIT((""));
}

static void
_dxf_CREATE_CURSOR_PIXMAPS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  int i, j, k, csize ;

  ENTRY(("_dxf_CREATE_CURSOR_PIXMAPS(0x%x)",I));

  csize = PDATA(cursor_size)*PDATA(cursor_size) ;

  /* allocate and init projection mark pixmap */
  PDATA(ProjectionMark) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, 3, 3) ;

  /* allocate a cursor save-under and init to zero */
  PDATA(cursor_saveunder) =
      _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX,
				 PDATA(cursor_size), PDATA(cursor_size)) ;

  for (i=0 ; i<csize ; i++)
      ((long *)PDATA(cursor_saveunder))[i] = 0 ;
  
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
	      ((long *)PDATA(ActiveSquareCursor))[k] = 0x0000ff00 ;
	      ((long *)PDATA(PassiveSquareCursor))[k++] = 0xafafafaf ;
	    }
	  else
	    {
	      /* interiors of both are black */
	      ((long *)PDATA(ActiveSquareCursor))[k] = 0 ;
	      ((long *)PDATA(PassiveSquareCursor))[k++] =  0 ;
	    }

  EXIT((""));
}

static void
_dxf_DRAW_MARKER (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;	
  long i, mx, my ;

  ENTRY(("_dxf_DRAW_MARKER(0x%x)", I));

  for (i = 0 ; i < PDATA(iMark) ; i++) 
    {
      PDATA(pXmark)[i] = PDATA(Xmark)[i] ;
      PDATA(pYmark)[i] = PDATA(Ymark)[i] ;
    }

  PDATA(piMark) = PDATA(iMark) ;
  
  cpack(0xffffffff) ;
  for (i = 0 ; i < PDATA(iMark) ; i++)
    {
      /* round marker to long for future lrectwrite() erasure */
      mx = (long)(PDATA(Xmark[i])+0.5) ;
      my = (long)(PDATA(Ymark[i])+0.5) ;
      
      /* draw it */
      if (!CLIPPED(mx,my))
	  recti (mx-1, my-1, mx+1, my+1) ;
    }

  EXIT((""));
}

static void
_dxf_ERASE_PREVIOUS_MARKS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  long i, mx, my, m[9] ;
  
  ENTRY(("_dxf_ERASE_PREVIOUS_MARKS(0x%x)",I));

  if (!PDATA(visible)) {
    EXIT(("not visible"));
    return ;
  }

  m[0]=m[1]=m[2]=m[3]=m[4]=m[5]=m[6]=m[7]=m[8] = 0 ;

  /* Restore the image over the previous marks */
  for ( i = 0 ; i < PDATA(piMark) ; i++ )
    {
      /* round mark coordinates */
      mx = (long)(PDATA(pXmark[i])+0.5) ;
      my = (long)(PDATA(pYmark[i])+0.5) ;
      
      if (!CLIPPED(mx,my))
	{
	  /* copy image to array m */
	  m[0] = IMAGE(long, mx-1, my-1) ;
	  m[1] = IMAGE(long, mx,   my-1) ;
	  m[2] = IMAGE(long, mx+1, my-1) ;
	  m[3] = IMAGE(long, mx-1, my  ) ;
	  m[4] = IMAGE(long, mx,   my  ) ;
	  m[5] = IMAGE(long, mx+1, my  ) ;
	  m[6] = IMAGE(long, mx-1, my+1) ;
	  m[7] = IMAGE(long, mx,   my+1) ;
	  m[8] = IMAGE(long, mx+1, my+1) ;
      
	  /* blit m to the screen */
	  lrectwrite (mx-1, my-1, mx+1, my+1, (const unsigned long *)m) ;
	}
    }

  EXIT((""));
}

static void
_dxf_ERASE_CURSOR (tdmInteractor I, int x, int y)
{
  DEFDATA(I,CursorData) ;
  long i, j, k, s2, llx, lly ;
  
  ENTRY(("_dxf_ERASE_CURSOR(0x%x, %d, %d)", I, x, y));

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
	  ((long *)PDATA(cursor_saveunder))[k++] = IMAGE(long, llx+j, lly+i) ;

  /* blit it to the screen */
  lrectwrite (llx, lly, x+s2, y+s2, (const unsigned long *)PDATA(cursor_saveunder)) ;

  EXIT((""));
}


static void
_dxf_POINTER_INVISIBLE (void *ctx)
{
  /* make the mouse pointer invisible */
  ENTRY(("_dxf_POINTER_INVISIBLE(0x%x)",ctx));

  cursoff() ;

  EXIT((""));
}

static void
_dxf_POINTER_VISIBLE (void *ctx)
{
  /* make the mouse pointer visible */

  ENTRY(("_dxf_POINTER_VISIBLE(0x%x)",ctx));

  curson() ;

  EXIT((""));
}

static void
_dxf_WARP_POINTER (void *ctx, int x, int y)
{
  /* move mouse pointer to position [x,y] relative to screen origin */

  ENTRY(("_dxf_WARP_POINTER(0x%x, %d, %d)",ctx, x, y));

  setvaluator (MOUSEX, (Int16)x, 0, XMAXSCREEN) ;
  setvaluator (MOUSEY, (Int16)y, 0, YMAXSCREEN) ;

  EXIT((""));
}

static void
_dxf_SET_LINE_ATTRIBUTES (void *ctx, int32 color, int style, float width)
{
  /*
   *  color is packed RGB.
   *  style 0 is solid.  style 1 is dashed.
   *  width is screen width of line, may be subpixel.  not currently used.
   */
  ENTRY(("_dxf_SET_LINE_ATTRIBUTES(0x%x, 0x%x, 0x%x, %f)",
	 ctx,color,style,width));

  lmcolor(LMC_COLOR) ;
  cpack(color) ;
  setlinestyle(style) ;

  EXIT((""));
}

static void
_dxf_SET_LINE_COLOR_WHITE (void *ctx)
{
  /*
   *  entry is obsolete; do not use in new code
   */

  ENTRY(("_dxf_SET_LINE_COLOR_WHITE(0x%x)",ctx));

  lmcolor(LMC_COLOR) ;
  cpack(0xffffffff) ;

  EXIT((""));
}
  
static void
_dxf_SET_LINE_COLOR_GRAY (void *ctx)
{
  /*
   *  entry is obsolete; do not use in new code
   */
  ENTRY(("_dxf_SET_LINE_COLOR_GRAY(0x%x)",ctx));

  lmcolor(LMC_COLOR) ;
  cpack(0x8f8f8f8f) ;

  EXIT((""));
}
  
static void
_dxf_DRAW_LINE(void *ctx, int x1, int y1, int x2, int y2)
{
  Int32 p[2] ;

  ENTRY(("_dxf_DRAW_LINE(0x%x, %d, %d, %d, %d)",ctx, x1, y1, x2, y2));

  bgnline() ;
  p[0] = x1 ; p[1] = y1 ; v2i(p) ;
  p[0] = x2 ; p[1] = y2 ; v2i(p) ;
  endline() ;

  EXIT((""));
}

static void
_dxf_DRAW_CURSOR_COORDS (tdmInteractor I, char *text)
{
  DEFDATA(I,CursorData) ;	

  ENTRY(("_dxf_DRAW_CURSOR_COORDS(0x%x, \"%s\")",I, text));

  if (PDATA(font) == -1)
    {
#if ibm6000
      loadXfont (1, "*helvetica*bold-r*14*100-100*") ;
      PDATA(font) = 1 ;
#else
      PDATA(font) = 0 ;
#endif
    }

  font(PDATA(font)) ;

  cpack(0) ;          /* black bottom shadow */
  cmov2 (PDATA(coord_llx)+3, PDATA(coord_lly)+getdescender()) ;
  charstr(text) ;
  
  cpack(0xffffffff) ; /* white text */
  cmov2 (PDATA(coord_llx)+2, PDATA(coord_lly)+getdescender()+1) ;
  charstr(text) ;

  EXIT((""));
}

#define STRUCTURES_ONLY
#include "hwInteractorEcho.h"

tdmInteractorEchoT	_dxd_hwInteractorEchoPort = 
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

