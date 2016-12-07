/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwInteractorEchoSB.c,v $
  Authors: Ellen Ball, Mark Hood

  These are the Starbase implementations of the TDM direct interactor echos,
  along with miscellaneous utility routines.  Each function needs to be
  ported to the native graphics API of the target system in order to provide
  support for direct interactors.

  Applications such as DX can be ported as a first step without direct
  interactor facilities by stubbing out everything here.

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#define _HP_FAST_MACROS 1
#include  <starbase.c.h>

#define STRUCTURES_ONLY
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwMatrix.h"
#include "hwInteractor.h"
#include "hwInteractorEcho.h"
#include "hwCursorInteractor.h"
#include "hwRotateInteractor.h"
#include "hwZoomInteractor.h"
#include "hwGlobeEchoDef.h"

#include <X11/Xutil.h>

#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

#define YFLIPSB(y) ((WIN_HEIGHT-(y)) -1)
#define IMAGE_INDEX(x,y) ((x)+YFLIPSB(y)*CDATA(iw))

void
_dxf_DRAW_ARROWHEAD (void*, float, float) ;

void *
_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height);

void 
_dxf_BUFFER_CONFIG (void *ctx, void *image, int llx, int lly, int urx, int ury,
		    int *CurrentDisplayMode, int *CurrentBufferMode,
		    tdmInteractorRedrawMode redrawMode) 
{ 
  DEFCONTEXT(ctx) ;
  int size, i, x_len, y_len ;

  ENTRY(("_dxf_BUFFER_CONFIG(0x%x, 0x%x, %d, %d, %d, %d, 0x%x, 0x%x, %d)",
	 ctx, image, llx, lly, urx, ury,
	 CurrentDisplayMode, CurrentBufferMode, redrawMode));

  /* record SB context information */
  *CurrentBufferMode = BUFFER_CONFIG_MODE ;
  PRINT(("CurrentBufferMode = %d", *CurrentBufferMode));

  if (redrawMode == tdmBothBufferDraw ||
      redrawMode == tdmFrontBufferDraw)
    {
      PRINT(("%s",
	     redrawMode == tdmBothBufferDraw ?
	     "tdmBothBufferDraw" : "tdmFrontBufferDraw"));
      /*
       *  Enable the front buffer for writing.  Treat request for both
       *  buffer draw the same since we don't double buffer interactor
       *  echos any longer.
       */

      double_buffer (FILDES, TRUE|DFRONT, PLANES/2) ;
      BUFFER_CONFIG_MODE = FRNTBUFFER ;
    }
  else if (redrawMode == tdmBackBufferDraw ||
	   redrawMode == tdmViewEchoMode)
    {
      PRINT(("%s", redrawMode == tdmViewEchoMode ?
	       "tdmViewEchoMode" : "tdmBackBufferDraw"));
      /*
       *  Enable the back buffer for writing.
       */

      double_buffer (FILDES, TRUE, PLANES/2) ;
      BUFFER_CONFIG_MODE = BCKBUFFER ;
    }
  else
    PRINT(("ignoring redraw mode"));

  EXIT((""));
}


void
_dxf_BUFFER_RESTORE_CONFIG (void *ctx, int OrigDisplayMode, int OrigBufferMode,
			    tdmInteractorRedrawMode redrawMode)
{
  /*
   *  Restore frame buffer configuration.
   */

  DEFCONTEXT(ctx) ;

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
          double_buffer (FILDES, TRUE, PLANES/2) ;
          BUFFER_CONFIG_MODE = BCKBUFFER ;
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
          double_buffer (FILDES, TRUE|DFRONT, PLANES/2) ;
          BUFFER_CONFIG_MODE = FRNTBUFFER ;
	}
      else
	PRINT(("remaining in original back buffer draw"));
    }
  else
    PRINT(("ignoring redraw mode"));

  EXIT((""));
}

void
_dxf_READ_BUFFER(void *ctx, int llx, int lly, int urx, int ury, void *buff)
{ 
  DEFCONTEXT(ctx) ;
  register int i, size ;
  register unsigned char *red, *green, *blue ;
  tdmImageSBP tdmImages ;
  int raw = 1, bank, bank_selection ; 
  int x_len, y_len ;

  ENTRY(("_dxf_READ_BUFFER(0x%x, %d, %d, %d, %d, 0x%x)",
	 ctx, llx, lly, urx, ury, buff));
  
  /*
   *  READ buffers :
   *
   *  For CRX24/CRX24Z...read, bank_switch, read, bank_switch, read
   *  all three banks.  Bank 2 is red; Bank 1 is green; Bank 0 is blue.
   *
   *  For CRX...read from active bank?  Active bank is front.
   *  Might need to read both bank 0 and 1.
   *
   *  The final block_read parameter is raw.  "1" means that data
   *  is organized in a device dependent format
   *
   *  Pixels are always read from the front (display) buffer.
   */
  
  tdmImages = (tdmImageSBP) buff ;
  red = tdmImages->saveBufRed ;
  green = tdmImages->saveBufGreen ;
  blue = tdmImages->saveBufBlue ;
  
  /* Set up Starbase array */
  x_len = urx - llx + 1 ;
  y_len = ury - lly + 1 ;
  size = x_len * y_len ;
  
  PRINT(("Block read"));
  if (PLANES == 48)  {
    bank_selection = 3 ;
    bank_switch (FILDES, bank_selection+2, 0) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, red, raw) ;
    bank_switch (FILDES, bank_selection+1, 0) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, green, raw) ;
    bank_switch (FILDES, bank_selection+0, 0) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, blue, raw) ;
  }

  if (PLANES == 24) {
    bank_switch (FILDES, 2, 0) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, red, raw) ;
    bank_switch (FILDES, 1, 0) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, green, raw) ;
    bank_switch (FILDES, 0, 0) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, blue, raw) ;
    
    PRINT(("Copy and shift nibble"));
    if (BUFFER_STATE) {
      PRINT(("BUFFER_STATE is 1"));
      for (i = 0; i < size ; i++, red++, green++, blue++) {
	*red = (*red & 0x0f) | (*red << 4) ;
	*green = (*green & 0x0f) | (*green << 4) ;
	*blue = (*blue & 0x0f) | (*blue << 4) ;
      }
    }
    else {
      PRINT(("BUFFER_STATE is 0"));
      for (i = 0; i < size ; i++, red++, green++, blue++) {
	*red = (*red & 0xf0) | (*red >> 4) ;
	*green = (*green & 0xf0) | (*green >> 4) ;
	*blue = (*blue & 0xf0) | (*blue >> 4) ;
      }
    }
  }
  
  if (PLANES == 16) {
    /* Still need to fix for flashing on CRX.  */
    /* And need to check CRX for GNOMON and GLOBE draw 5/13/93 */
    bank_switch(FILDES, !BUFFER_STATE, 1) ;
    dcblock_read(FILDES, llx, YFLIPSB(ury), x_len, y_len, blue, raw) ;
    bank_switch(FILDES, BUFFER_STATE, 1) ;
  }
  
  EXIT((""));
}
 
void
_dxf_WRITE_BUFFER (void *ctx, int llx, int lly, int urx, int ury, void *buff)
{ 
  DEFCONTEXT(ctx) ;

  int x_len, y_len ;
  int raw = 1 ;
  int bank ;
  tdmImageSBP tdmImages ;

  /* write buffers :
   *
   * For CRX24/CRX24Z...write, bank_switch, write, bank_switch, write
   * all three banks.  Bank 2 is red; Bank 1 is green; Bank 0 is blue.
   *
   * For CRX...write to buffer, but which?
   * Might need to read both bank 0 and 1.
   */

  ENTRY(("_dxf_WRITE_BUFFER(0x%x, %d, %d, %d, %d, 0x%x)",
	 ctx, llx, lly, urx, ury, buff));

  tdmImages = (tdmImageSBP) buff ;

  /* Set up Starbase array */
  x_len = urx - llx + 1 ;
  y_len = ury - lly + 1 ;

  if ((PLANES == 24) || (PLANES == 48))  {
	/* Bank switch to each bank and write */
	bank_switch (FILDES, 2, 0) ;
	dcblock_write(FILDES, llx, YFLIPSB(ury), x_len, y_len,
       		(unsigned char *) tdmImages->saveBufRed, raw) ;
	bank_switch (FILDES, 1, 0) ;
	dcblock_write(FILDES, llx, YFLIPSB(ury), x_len, y_len,
          	(unsigned char *) tdmImages->saveBufGreen, raw) ;
	bank_switch (FILDES, 0, 0) ;
	dcblock_write(FILDES, llx, YFLIPSB(ury), x_len, y_len,
        	(unsigned char *) tdmImages->saveBufBlue, raw) ;
  }

  if (PLANES == 16) {
    /* Should remove flashing bug, but might need to change to BUFFER_STATE */
    if (BUFFER_CONFIG_MODE == BCKBUFFER) bank = BUFFER_STATE ;
    if (BUFFER_CONFIG_MODE == FRNTBUFFER) bank = !BUFFER_STATE ; /* 8th */
    if (BUFFER_CONFIG_MODE == BOTHBUFFERS) bank = BUFFER_STATE ;
    bank_switch (FILDES, bank, 1) ;
    dcblock_write(FILDES, llx, YFLIPSB(ury), x_len, y_len,
         (unsigned char *) tdmImages->saveBufBlue, raw) ;
  }

  EXIT((""));
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
  int u, v, on, dummy = 0 ;

  /* globe edge visibility flags.  all globe instance share this data. */
  static struct {
    int latvis, longvis ;
  } edges[LATS][LONGS] ;

  /* globe and globeface defined in hwGlobeEchoDef.h */
  register float (*Globe)[LONGS][3] = (float (*)[LONGS][3])globe ;
  register struct Face (*Globeface)[LONGS] = (struct Face (*)[LONGS])globeface ;

  /* view normal */
  register float z0, z1, z2 ;
  z0 = rot[0][2] ; z1 = rot[1][2] ; z2 = rot[2][2] ;

  ENTRY(("_dxf_DRAW_GLOBE(0x%x, 0x%x, 0x%x, %d)",
	 I, udata, rot, draw));
  
#define FACEVISIBLE(u,v,z0,z1,z2) \
  (Globeface[u][v].norm[0] * z0 + \
   Globeface[u][v].norm[1] * z1 + \
   Globeface[u][v].norm[2] * z2 > 0.0)

  if (draw)
    {
      line_color(FILDES, 1.0, 1.0, 1.0);
      line_width(FILDES, 0.0, VDC_UNITS);
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

      EXIT((""));
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

  /* draw each visible edge exactly once */
  for (u=0 ; u<LATS ; u++)
    {
      for (v=0, on=0 ; v<LONGS-1 ; v++)
          if (edges[u][v].latvis)
            {
              if (!on)
                {
                  on = 1 ;
                  move3d(FILDES, Globe[u][v][0],
                                 Globe[u][v][1],
                                 Globe[u][v][2]);
                }
              draw3d(FILDES, Globe[u][v+1][0],
                             Globe[u][v+1][1],
                             Globe[u][v+1][2]);
              edges[u][v].latvis = 0 ;
            }
          else if (on)
            {
              on = 0 ;
            }

      /* close latitude line if necessary */
      if (edges[u][LONGS-1].latvis)
        {
          if (!on)
            {
              move3d(FILDES, Globe[u][LONGS-1][0],
                             Globe[u][LONGS-1][1],
                             Globe[u][LONGS-1][2]);
            }
          draw3d(FILDES, Globe[u][0][0],
                         Globe[u][0][1],
                         Globe[u][0][2]);
          edges[u][LONGS-1].latvis = 0 ;
        }
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
                  move3d(FILDES, Globe[u][v][0],
                                 Globe[u][v][1],
                                 Globe[u][v][2]);
                }
              draw3d(FILDES, Globe[u+1][v][0],
                             Globe[u+1][v][1],
                             Globe[u+1][v][2]);
              edges[u][v].longvis = 0 ;
            }
          else if (on)
            {
              on = 0 ;
            }
    }

  if (PDATA(redrawmode) != tdmViewEchoMode)
    {
      /* copy rendered globe from draw buffer to display buffer */
      BUFFER_STATE = !BUFFER_STATE ;
      _dxf_READ_BUFFER (PORT_CTX,
                        PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
                        GNOBE_BUF) ;
      BUFFER_STATE = !BUFFER_STATE ;

      /* force graphics output into front (draw) buffer */
      double_buffer (FILDES, TRUE|DFRONT, PLANES/2) ;

      _dxf_WRITE_BUFFER (PORT_CTX,
                        PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
                        GNOBE_BUF) ;

      /* switch back to normal */
      double_buffer (FILDES, TRUE, PLANES/2) ;

      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmBackBufferDraw) ;
    }

  EXIT((""));
}

void
_dxf_DRAW_ARROWHEAD (void* ctx, float ax, float ay)
{
  DEFCONTEXT(ctx);
  double angle, hyp, length ;
  float x1, y1 ;

  ENTRY(("_dxf_DRAW_ARROWHEAD(0x%x, %f, %f)", ctx, ax, ay));
  
  hyp = sqrt(ax*ax + ay*ay) ;

  if (hyp == 0.0) {
    EXIT(("hyp == 0.0"));
    return ;
  }

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

  move2d(FILDES, ax, ay);
  draw2d(FILDES, ax + x1, ay + y1);

  angle = angle - M_PI/3 ;
  if (angle < 0)
      angle = angle + 2*M_PI ;

  x1 = cos(angle) * length ;
  y1 = sin(angle) * length ;

  move2d(FILDES, ax, ay);
  draw2d(FILDES, ax + x1, ay + y1);

  EXIT((""));
}

void 
_dxf_DRAW_GNOMON (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  DEFDATA(I,tdmRotateData) ;
  DEFPORT(I_PORT_HANDLE) ;

  /*
   *  draw == 1 to draw gnomon, draw == 0 to undraw.  This is done with
   *  two separate calls in order to support explicit erasure of edges for
   *  some implementations.  A draw is always preceded by an undraw and
   *  the pair of invocations is assumed to be atomic.
   *
   *  Computations are done in normalized screen coordinates in order to
   *  render arrow heads correctly.
   */

  float origin[2] ;
  float xaxis[2],  yaxis[2],  zaxis[2] ;
  float xlabel[2], ylabel[2], zlabel[2] ;
  int worldx, worldy, vdcx, vdcy, dcx, dcy, dummy = 0 ;

  ENTRY(("_dxf_DRAW_GNOMON(0x%x, 0x%x, 0x%x, %d)",
	 I, udata, rot, draw));

  /* 1 pixel in normalized coordinates */
  PDATA(nudge) = 1.0/(float)GNOMONRADIUS ;

  if (draw)
    {
      line_color(FILDES, 1.0, 1.0, 1.0);
      text_color(FILDES, 1.0, 1.0, 1.0);
      line_width(FILDES, 0.0, VDC_UNITS);
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
      EXIT((""));
      return ;
    }

  origin[0] = 0.0 ;
  origin[1] = 0.0 ;

  xaxis[0] = 0.7 * rot[0][0] ; xaxis[1] = 0.7 * rot[0][1] ;
  yaxis[0] = 0.7 * rot[1][0] ; yaxis[1] = 0.7 * rot[1][1] ;
  zaxis[0] = 0.7 * rot[2][0] ; zaxis[1] = 0.7 * rot[2][1] ;

  xlabel[0] = 0.8 * rot[0][0] ; xlabel[1] = 0.8 * rot[0][1] ;
  ylabel[0] = 0.8 * rot[1][0] ; ylabel[1] = 0.8 * rot[1][1] ;
  zlabel[0] = 0.8 * rot[2][0] ; zlabel[1] = 0.8 * rot[2][1] ;

  push_matrix3d (FILDES, (float (*)[4])identity) ;
  move2d(FILDES, origin[0], origin[1]);
  draw2d(FILDES, xaxis[0], xaxis[1]);
  _dxf_DRAW_ARROWHEAD (PORT_CTX, xaxis[0], xaxis[1]) ;

  move2d(FILDES, origin[0], origin[1]);
  draw2d(FILDES, yaxis[0], yaxis[1]);
  _dxf_DRAW_ARROWHEAD (PORT_CTX, yaxis[0], yaxis[1]) ;

  move2d(FILDES, origin[0], origin[1]);
  draw2d(FILDES, zaxis[0], zaxis[1]);
  _dxf_DRAW_ARROWHEAD (PORT_CTX, zaxis[0], zaxis[1]) ;

  if (xlabel[0] <= 0) xlabel[0] -= 0.15 ;
  if (xlabel[1] <= 0) xlabel[1] -= 0.15 ;

  if (ylabel[0] <= 0) ylabel[0] -= 0.15 ;
  if (ylabel[1] <= 0) ylabel[1] -= 0.15 ;

  if (zlabel[0] <= 0) zlabel[0] -= 0.15 ;
  if (zlabel[1] <= 0) zlabel[1] -= 0.15 ;

  character_height(FILDES, 0.3);
  text2d(FILDES, xlabel[0], xlabel[1], "X", VDC_TEXT, FALSE);
  text2d(FILDES, ylabel[0], ylabel[1], "Y", VDC_TEXT, FALSE);
  text2d(FILDES, zlabel[0], zlabel[1], "Z", VDC_TEXT, FALSE);
  pop_matrix(FILDES) ;

  if (PDATA(redrawmode) != tdmViewEchoMode)
    {
      PRINT(("Copy back buffer gnomon to front"));
      /* copy draw buffer to display buffer */
      /* read gnomon part of image from back buffer */
      /* Need to allocate gnomonBuffer once, and not each time */

#if 1            /* this could be put in TDM_CREATE_WINDOW */
      if (!GNOBE_BUF) {
         /* allocate a buffer */
         if (!(GNOBE_BUF = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, 
		2*GNOMONRADIUS + 1, 2*GNOMONRADIUS + 1))) {
	   EXIT(("ERROR: out of memory"));
	   return ;
          }
      }
#endif
      /* BUFFER_STATE is used in READ_BUFFER
       * Normal operation is read front/write back
       * For gnomon draw, we do the opposite:  draw gnomon to back
       * copy back to front for gnomon area
       */
      BUFFER_STATE = !BUFFER_STATE ;
      _dxf_READ_BUFFER (PORT_CTX,
                        PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
			GNOBE_BUF) ;
      BUFFER_STATE = !BUFFER_STATE ;

      /* force graphics output into front (draw) buffer */
      double_buffer (FILDES, TRUE|DFRONT, PLANES/2) ;

      _dxf_WRITE_BUFFER (PORT_CTX,
                        PDATA(illx), PDATA(illy), PDATA(iurx), PDATA(iury),
                        GNOBE_BUF) ;

      /* switch back to normal */
      double_buffer (FILDES, TRUE, PLANES/2) ;

      _dxf_BUFFER_RESTORE_CONFIG
	  (PORT_CTX, dummy, PDATA(buffermode), tdmBackBufferDraw) ;
    }

  EXIT((""));
} 


void 
_dxf_DRAW_ZOOMBOX (tdmInteractor I, void *udata, float rot[4][4], int draw)
{
  DEFDATA(I,tdmZoomData) ;
  DEFPORT(I_PORT_HANDLE) ;

  int x1, y1, x2, y2, yflipper ;
  int zoombox [5][2] ;
  int i, j ;
  register unsigned char *r0, *g0, *b0 ;
  static unsigned char R[4096], G[4096], B[4096] ;
  tdmImageSBT tmp ;

  ENTRY(("_dxf_DRAW_ZOOMBOX(0x%x, 0x%x, 0x%x, %d)",
	 I, udata, rot, draw));

  /*
   *  draw == 1 to draw zoombox, draw == 0 to undraw.  This is done with two
   *  separate calls in order to support explicit erasure.
   */

  /* Make certain the window is within the boundaries of the screen */
  x1 = XSCREENCLIP(PDATA(x1)) ;
  x2 = XSCREENCLIP(PDATA(x2)) ;
  y1 = YSCREENCLIP(PDATA(y1)) ;
  y2 = YSCREENCLIP(PDATA(y2)) ;

  if (draw)
    {
      /* draw the box */
      PRINT(("Draw the zoom box"));
      zoombox[0][0] = x1 ; zoombox[0][1] = YFLIPSB(y1) ;  /* llx and lly */
      zoombox[1][0] = x2 ; zoombox[1][1] = YFLIPSB(y1) ;
      zoombox[2][0] = x2 ; zoombox[2][1] = YFLIPSB(y2) ;  /* urx and ury */
      zoombox[3][0] = x1 ; zoombox[3][1] = YFLIPSB(y2) ;
      zoombox[4][0] = x1 ; zoombox[4][1] = YFLIPSB(y1) ;
      line_color(FILDES, 1.0, 1.0, 1.0);
      dcpolyline(FILDES,(int *)zoombox,5,FALSE) ;
    }
  else
    {
      /* repair the image */
      if ( (PLANES == 24) || (PLANES == 48))  /* CRX48Z */
    	{
      	r0 = ((tdmImageSBP)CDATA(image))->saveBufRed ;
      	g0 = ((tdmImageSBP)CDATA(image))->saveBufGreen ;
      	b0 = ((tdmImageSBP)CDATA(image))->saveBufBlue ;
    	}
      else
      	r0 = g0 = b0 = ((tdmImageSBP)CDATA(image))->saveBufBlue ;

  tmp.saveBufRed = R ;
  tmp.saveBufGreen = G ;
  tmp.saveBufBlue = B ;

  /* restore the image over the previous zoom box */
 yflipper = y2 - y1 ;

 /* Draw the Right side of image previously covered by zoom box */
 for (i = 0 ; i <= yflipper ; i++)
    {
        j = IMAGE_INDEX (x2-1, y1+i) ;
        R[(yflipper - i) * 2] = r0[j] ;
        G[(yflipper - i) * 2] = g0[j] ;
        B[(yflipper - i) * 2] = b0[j] ; 

        j = IMAGE_INDEX (x2, y1+i) ;
        R[((yflipper - i) * 2) + 1] = r0[j] ;
        G[((yflipper - i) * 2) + 1] = g0[j] ;
        B[((yflipper - i) * 2) + 1] = b0[j] ;
    }
 _dxf_WRITE_BUFFER (PORT_CTX, x2-1, y1, x2, y2, (void *) &tmp ) ;

 /* Draw the Left side of image previously covered by zoom box */
 for (i = 0 ; i <= yflipper ; i++)
    {
        j = IMAGE_INDEX (x1-1, y1+i) ;
        R[(yflipper - i) * 2] = r0[j] ;
        G[(yflipper - i) * 2] = g0[j] ;
        B[(yflipper - i) * 2] = b0[j] ;

        j = IMAGE_INDEX (x1, y1+i) ;
        R[((yflipper - i) * 2) + 1] = r0[j] ;
        G[((yflipper - i) * 2) + 1] = g0[j] ;
        B[((yflipper - i) * 2) + 1] = b0[j] ;

    }
 _dxf_WRITE_BUFFER (PORT_CTX, x1-1, y1, x1, y2, (void *) &tmp ) ;

  /* Draw the bottom part of the image */
  for (i = 0; i <= (x2 - x1) ; i++)
    {
          j = IMAGE_INDEX (x1 + i, y1) ;
          R[i] = r0[j] ;
          G[i] = g0[j] ;
          B[i] = b0[j] ;
    }

  _dxf_WRITE_BUFFER (PORT_CTX, x1, y1, x2, y1, (void *) &tmp ) ;

  /* Draw the top part of the image */
  for (i = 0 ; i <= (x2 - x1) ; i++) 
    {
          j = IMAGE_INDEX (x1 + i, y2) ;
          R[i] = r0[j] ;
          G[i] = g0[j] ;
          B[i] = b0[j] ;
    }

  _dxf_WRITE_BUFFER (PORT_CTX, x1, y2, x2, y2, (void *) &tmp ) ;
 }

  EXIT((""));
}

int
_dxf_GET_ZBUFFER_STATUS (void *ctx)
{
  int status ;

  ENTRY(("_dxf_GET_ZBUFFER_STATUS(0x%x)", ctx));

  /* we will never be in zbuffer mode when drawing interactors on Starbase */
  status = 0 ;

  EXIT((""));
  return status ;
}

void
_dxf_SET_ZBUFFER_STATUS (void *ctx, int status)
{
  ENTRY(("_dxf_SET_ZBUFFER_STATUS(0x%x, %d)", ctx, status));
  
  /* we will never be in zbuffer mode when drawing interactors on Starbase */

  EXIT((""));
}

void
_dxf_SET_SOLID_FILL_PATTERN (void *ctx)
{
  ENTRY(("_dxf_SET_SOLID_FILL_PATTERN(0x%x)", ctx));

  /* used just to reset screen door half transparency, not used in Starbase */

  EXIT((""));
}

void
_dxf_GET_WINDOW_ORIGIN (void *ctx, int *x, int *y)
{
  /*
   *  return LL corner of window relative to LL corner of screen in pixels
   */

  Window child ;
  DEFCONTEXT(ctx) ;
  ENTRY(("_dxf_GET_WINDOW_ORIGIN(0x%x, 0x%x, 0x%x)", ctx, x, y));

  XTranslateCoordinates (DPY_SB, XWINDOW_ID, RootWindow(DPY_SB, SCREEN_SB),
			 0, 0, x, y, &child) ;

  *y = YMAXSCREEN - (*y + WIN_HEIGHT) + 1 ;

  EXIT(("x = %d y = %d", *x, *y));
}

void
_dxf_GET_MAXSCREEN (void *ctx, int *x, int *y)
{
  /*
   *  return maximum coordinates of entire screen in pixels
   */

  DEFCONTEXT(ctx) ;
  ENTRY(("_dxf_GET_MAXSCREEN(0x%x, 0x%x, 0x%x)", ctx, x, y));

  *x = XMAXSCREEN ;
  *y = YMAXSCREEN ;

  EXIT(("x = %d y = %d", *x, *y));
}

void
_dxf_SET_WORLD_SCREEN (void *ctx, int w, int h)
{
  ENTRY(("_dxf_SET_WORLD_SCREEN(0x%x, %d, %d)", ctx, w, h));
  
  /* Starbase provides DC versions of required primitives */

  EXIT(("stubbed"));
}

void
_dxf_CREATE_CURSOR_PIXMAPS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  register unsigned char *red, *green, *blue ;
  register int i, j, csize ;
  tdmImageSBP tdmImage ;

  ENTRY(("_dxf_CREATE_CURSOR_PIXMAPS(0x%x)", I));
  
  /* Allocate pixmap for projection mark.  Not used, but expected elsewhere */
  PDATA(ProjectionMark) = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, 3, 3) ;
  
  /* allocate a cursor save-under */
  csize = PDATA(cursor_size)*PDATA(cursor_size) ;
  PDATA(cursor_saveunder) =
      _dxf_ALLOCATE_PIXEL_ARRAY
	  (PORT_CTX, PDATA(cursor_size), PDATA(cursor_size)) ;

  red =   ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufRed ;
  green = ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufGreen ;
  blue =  ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufBlue ;

  /* allocate and init active cursor pixmap */
  PDATA(ActiveSquareCursor) = 
      _dxf_ALLOCATE_PIXEL_ARRAY
	  (PORT_CTX, PDATA(cursor_size), PDATA(cursor_size)) ;

  red =   ((tdmImageSBP)PDATA(ActiveSquareCursor))->saveBufRed ;
  green = ((tdmImageSBP)PDATA(ActiveSquareCursor))->saveBufGreen ;
  blue =  ((tdmImageSBP)PDATA(ActiveSquareCursor))->saveBufBlue ;

  if ( (PLANES == 24) || (PLANES == 48))  /* CRX48Z */
      for (i=0 ; i<PDATA(cursor_size) ; i++)
	  for (j=0 ; j<PDATA(cursor_size) ; j++)
	      if (i==0 || i==PDATA(cursor_size)-1 ||
		  j==0 || j==PDATA(cursor_size)-1 )
		{
		  /* active cursor outline is green */
		  *red++ = 0 ; *green++ = 0xff ; *blue++ = 0 ;
		}
	      else
		{
		  /* interior is black */
		  *red++ = 0 ; *green++ = 0 ; *blue++ = 0 ;
		}
  else
      for (i=0 ; i<PDATA(cursor_size) ; i++)
	  for (j=0 ; j<PDATA(cursor_size) ; j++)
	      if (i==0 || i==PDATA(cursor_size)-1 ||
		  j==0 || j==PDATA(cursor_size)-1 )
		  /* active cursor outline is green */
		  *blue++ = 0xff/* XXX wrong */ ;
	      else
		  /* interior is black */
		  *blue++ = 0 ;

  /* allocate and init passive cursor pixmap */
  PDATA(PassiveSquareCursor) = 
      _dxf_ALLOCATE_PIXEL_ARRAY
	  (PORT_CTX, PDATA(cursor_size), PDATA(cursor_size)) ;

  red =   ((tdmImageSBP)PDATA(PassiveSquareCursor))->saveBufRed ;
  green = ((tdmImageSBP)PDATA(PassiveSquareCursor))->saveBufGreen ;
  blue =  ((tdmImageSBP)PDATA(PassiveSquareCursor))->saveBufBlue ;

  if ( (PLANES == 24) || (PLANES == 48))  /* CRX48Z */
      for (i=0 ; i<PDATA(cursor_size) ; i++)
	  for (j=0 ; j<PDATA(cursor_size) ; j++)
	      if (i==0 || i==PDATA(cursor_size)-1 ||
		  j==0 || j==PDATA(cursor_size)-1 )
		{
		  /* passive cursor outline is gray */
		  *red++ = 0x88 ; *green++ = 0x88 ; *blue++ = 0x88 ;
		}
	      else
		{
		  /* interior is black */
		  *red++ = 0 ; *green++ = 0 ; *blue++ = 0 ;
		}
  else
      for (i=0 ; i<PDATA(cursor_size) ; i++)
	  for (j=0 ; j<PDATA(cursor_size) ; j++)
	      if (i==0 || i==PDATA(cursor_size)-1 ||
		  j==0 || j==PDATA(cursor_size)-1 )
		  /* passive cursor outline is gray */
		  *blue++ = 0x80 ;
	      else
		  /* interior is black */
		  *blue++ = 0 ;

  EXIT((""));
}

void
_dxf_ERASE_PREVIOUS_MARKS (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  register unsigned char *rp, *gp, *bp ;
  register int i, j, mx, my ;
  unsigned char R[9], G[9], B[9] ;
  tdmImageSBT tmp ;

  ENTRY(("_dxf_ERASE_PREVIOUS_MARKS(0x%x)", I));
  
  if (!PDATA(visible)) {
    EXIT(("not visible"));
    return ;
  }

  if ( (PLANES == 24) || (PLANES == 48))  /* CRX48Z */
    {
      rp = ((tdmImageSBP)CDATA(image))->saveBufRed ;
      gp = ((tdmImageSBP)CDATA(image))->saveBufGreen ;
      bp = ((tdmImageSBP)CDATA(image))->saveBufBlue ;
    }
  else
      rp = gp = bp = ((tdmImageSBP)CDATA(image))->saveBufBlue ;

  tmp.saveBufRed = R ;
  tmp.saveBufGreen = G ;
  tmp.saveBufBlue = B ;

  /* restore the image over the previous marks */
  for (i = 0 ; i < PDATA(piMark) ; i++)
    {
      /* round mark coordinates */
      mx = (long)(PDATA(pXmark[i])+0.5) ;
      my = (long)(PDATA(pYmark[i])+0.5) ;
      
      if (!CLIPPED(mx,my))
	{
	  /* copy image */
	  j = IMAGE_INDEX (mx-1, my+1) ;
	  R[0] = rp[j] ;
	  G[0] = gp[j] ;
	  B[0] = bp[j] ;

	  j = IMAGE_INDEX (mx,   my+1) ;
	  R[1] = rp[j] ;
	  G[1] = gp[j] ;
	  B[1] = bp[j] ;

	  j = IMAGE_INDEX (mx+1, my+1) ;
	  R[2] = rp[j] ;
	  G[2] = gp[j] ;
	  B[2] = bp[j] ;

	  j = IMAGE_INDEX (mx-1, my  ) ;
	  R[3] = rp[j] ;
	  G[3] = gp[j] ;
	  B[3] = bp[j] ;

	  j = IMAGE_INDEX (mx,   my  ) ;
	  R[4] = rp[j] ;
	  G[4] = gp[j] ;
	  B[4] = bp[j] ;

	  j = IMAGE_INDEX (mx+1, my  ) ;
	  R[5] = rp[j] ;
	  G[5] = gp[j] ;
	  B[5] = bp[j] ;

	  j = IMAGE_INDEX (mx-1, my-1) ;
	  R[6] = rp[j] ;
	  G[6] = gp[j] ;
	  B[6] = bp[j] ;

	  j = IMAGE_INDEX (mx,   my-1) ;
	  R[7] = rp[j] ;
	  G[7] = gp[j] ;
	  B[7] = bp[j] ;

	  j = IMAGE_INDEX (mx+1, my-1) ;
	  R[8] = rp[j] ;
	  G[8] = gp[j] ;
	  B[8] = bp[j] ;

	  /* write it to the screen */
	  _dxf_WRITE_BUFFER (PORT_CTX, mx-1, my-1, mx+1, my+1, (void *)&tmp) ;
	}
    }

  EXIT((""));
}

void
_dxf_ERASE_CURSOR (tdmInteractor I, int x, int y)
{
  DEFDATA(I,CursorData) ;
  DEFPORT(I_PORT_HANDLE) ;
  register unsigned char *r0, *g0, *b0, *r1, *g1, *b1 ;
  register int i, j, k ;
  int s2, llx, lly, ury ;
  
  ENTRY(("_dxf_ERASE_CURSOR(0x%x, %d, %d)", I, x, y));
  
  if (!PDATA(visible) || CLIPPED(x,y)) {
    EXIT(("clipped or not visible"));
    return ;
  }

  s2= PDATA(cursor_size) / 2 ;
  llx = x-s2 ;
  lly = y-s2 ;
  ury = y+s2 ;

  if ( (PLANES == 24) || (PLANES == 48))  /* CRX48Z */
    {
      r0 = ((tdmImageSBP)CDATA(image))->saveBufRed ;
      g0 = ((tdmImageSBP)CDATA(image))->saveBufGreen ;
      b0 = ((tdmImageSBP)CDATA(image))->saveBufBlue ;
      
      r1 = ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufRed ;
      g1 = ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufGreen ;
      b1 = ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufBlue ;
    }
  else
    {
      r0 = g0 = b0 = ((tdmImageSBP)CDATA(image))->saveBufBlue ;
      r1 = g1 = b1 = ((tdmImageSBP)PDATA(cursor_saveunder))->saveBufBlue ;
    }

  /* copy image to save-under array */
  for (i=0 ; i<PDATA(cursor_size) ; i++)
      for (j=0 ; j<PDATA(cursor_size) ; j++)
	{
	  k = IMAGE_INDEX (llx+j, ury-i) ;
	  *r1++ = r0[k] ;
	  *g1++ = g0[k] ;
	  *b1++ = b0[k] ;
	}

  /* write it to the screen */
  _dxf_WRITE_BUFFER (PORT_CTX, llx, lly,
		    x+s2, y+s2, (void *)PDATA(cursor_saveunder)) ;

  EXIT((""));
}


void
_dxf_POINTER_INVISIBLE (void *ctx)
{
  DEFCONTEXT(ctx) ;

  int		i, screen;
  Pixmap  	p1, p2;
  XColor  	fc, bc;
  char          tmp_bits[8];
  Cursor 	cursorId ;

  ENTRY(("_dxf_POINTER_INVISIBLE(0x%x)", ctx));

  for (i = 0; i < 8; i++)
    tmp_bits[i] = 0 ;

  screen = DefaultScreen(DPY_SB) ;
  p1 = XCreatePixmapFromBitmapData (DPY_SB, RootWindow(DPY_SB, screen),
           tmp_bits, 4, 4, 0, 0, 1) ;
  p2 = XCreatePixmapFromBitmapData (DPY_SB, RootWindow(DPY_SB, screen),
           tmp_bits, 4, 4, 0, 0, 1) ;
  cursorId = XCreatePixmapCursor (DPY_SB, p1, p2, &fc, &bc, 1, 1 ) ;
  XFreePixmap ( DPY_SB, p1 ) ;
  XFreePixmap ( DPY_SB, p2 ) ;

  XDefineCursor (DPY_SB, XWINDOW_ID, cursorId) ;

  EXIT((""));
}

void
_dxf_POINTER_VISIBLE (void *ctx)
{ 
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_POINTER_VISIBLE(0x%x)", ctx));
  
  XUndefineCursor (DPY_SB, XWINDOW_ID) ;

  EXIT((""));
}

void
_dxf_WARP_POINTER (void *ctx, int x, int y)
{
  /*
   *  warp pointer to specified position relative to root window
   */
  
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_WARP_POINTER(0x%x, %d, %d)", ctx, x, y));

  XWarpPointer (DPY_SB, None, RootWindow(DPY_SB, SCREEN_SB),
		0, 0, 0, 0, x, YMAXSCREEN-y) ;

  EXIT((""));
}

static void
_dxf_SET_LINE_ATTRIBUTES (void *ctx, int32 color, int style, float width)
{
  DEFCONTEXT(ctx) ;

  /*
   *  color is packed RGB.
   *  style 0 is solid.  style 1 is dashed.
   *  width is screen width of line, may be subpixel.  not currently used.
   */
  ENTRY(("_dxf_SET_LINE_ATTRIBUTES(0x%x, %d, %d, %f)",
	 ctx, color, style, width));

  switch (color)
    {
    case 0x8f8f8f8f:
      line_color (FILDES, 0.5, 0.5, 0.5) ;
      break ;
    case 0xffffffff:
    default:
      line_color (FILDES, 1.0, 1.0, 1.0) ;
      break ;
    }

  switch (style)
    {
    case 1:
      line_type (FILDES, DASH) ;
      break ;
    case 0:
    default:
      line_type (FILDES, SOLID) ;
      break ;
    }

  EXIT((""));
}

void
_dxf_SET_LINE_COLOR_WHITE (void *ctx)
{
  /*
   *  entry is obsolete; do not use in new code
   */
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_LINE_COLOR_WHITE(0x%x)", ctx));

  line_color (FILDES, 1.0, 1.0, 1.0) ;

  EXIT((""));
}
  
void
_dxf_SET_LINE_COLOR_GRAY (void *ctx)
{
  /*
   *  entry is obsolete; do not use in new code
   */
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_LINE_COLOR_GRAY(0x%x)", ctx));
  
  line_color (FILDES, 0.5, 0.5, 0.5) ;

  EXIT((""));
}
  
void
_dxf_DRAW_LINE(void *ctx, int x1, int y1, int x2, int y2)
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_DRAW_LINE(0x%x, %d, %d, %d, %d)", ctx, x1, y1, x2, y2));

  dcmove (FILDES, x1, YFLIPSB(y1)) ;
  dcdraw (FILDES, x2, YFLIPSB(y2)) ;

  EXIT((""));
}

void
_dxf_DRAW_MARKER (tdmInteractor I)
{
  DEFDATA(I,CursorData) ;	
  DEFPORT(I_PORT_HANDLE) ;
  register int i, mx, my ;

  ENTRY(("_dxf_DRAW_MARKER(0x%x)", I));
  
  for (i = 0 ; i < PDATA(iMark) ; i++) 
    {
      PDATA(pXmark)[i] = PDATA(Xmark)[i] ;
      PDATA(pYmark)[i] = PDATA(Ymark)[i] ;
    }

  PDATA(piMark) = PDATA(iMark) ;
  
  fill_color (FILDES, 1.0, 1.0, 1.0) ;
  for (i = 0 ; i < PDATA(iMark) ; i++)
    {
      /* round marker to int */
      mx = (int)(PDATA(Xmark[i])+0.5) ;
      my = (int)(PDATA(Ymark[i])+0.5) ;
      
      /* draw it */
      if (!CLIPPED(mx,my))
	  dcrectangle (FILDES, mx-1, YFLIPSB(my-1), mx+1, YFLIPSB(my+1)) ;
    }

  EXIT((""));
}

void
_dxf_DRAW_CURSOR_COORDS (tdmInteractor I, char *text)
{
  DEFDATA(I,CursorData) ;	
  DEFPORT(I_PORT_HANDLE) ;

  ENTRY(("_dxf_DRAW_CURSOR_COORDS(0x%x, \"%s\")", I, text));

  dccharacter_height (FILDES, 20) ;
  text_color (FILDES, 0.0, 0.0, 0.0) ;
  dctext (FILDES, PDATA(coord_llx)+4, YFLIPSB(PDATA(coord_lly)+2), text) ;

  text_color(FILDES, 1.0, 1.0, 1.0) ;
  dctext (FILDES, PDATA(coord_llx)+3, YFLIPSB(PDATA(coord_lly)+3), text) ;

  EXIT((""));
}


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
