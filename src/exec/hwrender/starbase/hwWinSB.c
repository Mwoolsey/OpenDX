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
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwWinSB.c,v $
  Authors: Ellen Ball
	   Mark Hood

  Creates a Starbase/X11 window without mapping to the screen
  
	Performs the following calls:
		
		GetXVisualInfo
		FindImagePlanesVisual
		CreateImagePlanesWindow
		XStoreName
		XSync
		make_X11_gopen_string
		gopen
	then set
		IMAGE_CTX aka (fildes) = gopen(stuff)
		XID = window 

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <starbase.c.h>

#define STRUCTURES_ONLY
#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwPortSB.h"
#include "hwInteractor.h"
#include "wsutils.h"

#include <X11/Xutil.h>

#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwPortLayer.h"

#include "hwDebug.h"

Window
_dxf_CREATE_WINDOW (void *globals, char *winName, int width, int height)
{
  int 	      argc;
  char 	      *argv[];
  char 	      *outdevice;
  Window      window;
  int 	      fildes;
  int 	      screen;
  int	      x=0, y=0;
  int         dx_height ;
  int         cmap, banks, PlanesPerBank, OverlayPlanes;
  int	      dbuffer_mode, bplanes, current_buffer ;
  int	      number_of_planes ;
  
  int         transparentOverlays;    /* Non-zero if there's at least one
				       * overlay visual and if at least one
				       * of those supports a transparent
				       * pixel. */
  int         numVisuals;             /* Number of XVisualInfo struct's
				       * pointed to to by pVisuals. */
  XVisualInfo *pVisuals;              /* All of the device's visuals. */
  int         numOverlayVisuals;      /* Number of OverlayVisualInfo's
				       * pointed to by pOverlayVisuals.
				       * If this number is zero, the device
				       * does not have overlay planes. */
  
  OverlayInfo *pOverlayVisuals;       /* The device's overlay plane visual
				       * information. */
  int         numImageVisuals;        /* Number of XVisualInfo's pointed
				       * to by pImageVisuals. */
  XVisualInfo **pImageVisuals;        /* The device's image visuals. */
  int         cmapHint;
  int         desiredDepth,flexibility;
  Visual      *pImageVisualToUse;     /* The device's visual used for the
				       * image planes window. */
  int         imageDepthToUse;        /* Depth of the image planes window. */
  Colormap    imageColormap;          /* Image planes window's colormap. */
  int         mustFreeImageColormap;  /* Does this program need to call
				       * XFreeColormap() for imageColormap. */
  int xmaxscreen, ymaxscreen ;
  
  DEFGLOBALDATA(globals) ;

  ENTRY(("_dxf_CREATE_WINDOW(0x%x, \"%s\", %d, %d)",
	 globals, winName, width, height));
  
  screen = DefaultScreen(DPY);
  xmaxscreen = DisplayWidth(DPY, screen) - 1 ;
  ymaxscreen = DisplayHeight(DPY, screen) - 1 ;
  
  /* Get all of the visual information about the screen: */
  if (GetXVisualInfo(DPY, screen, &transparentOverlays,
		     &numVisuals, &pVisuals,
		     &numOverlayVisuals, &pOverlayVisuals,
		     &numImageVisuals, &pImageVisuals))
    {
      /* Unable to find acceptable X visual */
      DXErrorGoto(ERROR_INTERNAL,"#11720");
    }
  
  /* Find an appropriate image planes visual: */
  if (FindImagePlanesVisual(DPY, screen, numImageVisuals, pImageVisuals,
			    SB_CMAP_TYPE_FULL, 24, FLEXIBLE,
			    &pImageVisualToUse, &imageDepthToUse))
    {
      /* Unable to find acceptable X visual */
      DXErrorGoto(ERROR_INTERNAL,"#11720");
    }
  
  if (pVisuals)
      XFree((char *)pVisuals) ;
  
  if (pImageVisuals)
      tdmFree(pImageVisuals) ;
  
  /*
   *  We need to record the DX notion of the window height in order to
   *  deal with flipping the Y coordinate.  See _dxf_SET_VIEWPORT() and
   *  _dxf_SET_WINDOW_SIZE() in tdmPortSB.c.
   */
  
  dx_height = height ;

  if (PARENT_WINDOW)
    {
      /*
       *  A Starbase window cannot render beyond the physical limits
       *  with which it was created.  If there's a UI, we can support
       *  resize scaling by setting the window size to the maximum
       *  screen; when the window is re-parented, it will be clipped to
       *  the parent window bounds.  We can't do this if we're running
       *  in script mode since there is no parent window to do the
       *  clipping.
       */

      width = xmaxscreen +1 ;
      height = ymaxscreen +1 ;
    }
  
  if (CreateImagePlanesWindow(DPY, screen,
			      RootWindow(DPY, screen),
			      x, y, width, height,
			      imageDepthToUse, pImageVisualToUse,
			      NULL, NULL, winName, winName,
			      &window,
			      &imageColormap, &mustFreeImageColormap))
    {
      PRINT(("Could not create X window"));
      DXErrorGoto (ERROR_INTERNAL, "#13770") ;
    }
  
  PIXDEPTH = imageDepthToUse;

  XStoreName(DPY, window, isdigit(winName[0]) ? "DX" : winName) ; 
  XSync(DPY,False);
  outdevice = make_X11_gopen_string(DPY, window) ;
  
  PRINT(("Output device (via \"make_X11_gopen_string\"): \"%s\"", outdevice));
  
  /*
   *  Allocate a context for starbase-specific data.  Deallocate in
   *  _dxf_DESTROY_WINDOW().
   */
  
  if (PORT_HANDLE->private = (void *) tdmAllocateLocalZero(sizeof(tdmSBctxT)))
    {
      DEFPORT(PORT_HANDLE) ;
      SBTRANSLATION = (hwTranslationP) _dxf_CREATE_HW_TRANSLATION(LWIN) ;
      
      /* open with OUTDEV instead of OUTINDEV in order to use X11 input */
      if ((FILDES = gopen (outdevice, OUTDEV, NULL,
			   INIT|THREE_D|MODEL_XFORM)) == -1)
        {
	  PRINT(("gopen() failed"));
	  tdmFree(PORT_HANDLE->private) ;
	  goto error ;
        }
      else
	{

	  free(outdevice);

	  /* Set up for CMAP_FULL and double buffering */
          inquire_fb_configuration(FILDES,&banks,&PLANES,&PlanesPerBank, 
				   &OverlayPlanes) ;
  	  COLOR_MAP = CMAP_FULL ;
	  shade_mode(FILDES,COLOR_MAP|INIT,TRUE) ;
	  
          PRINT(("Set double_buffer on"));
	  inquire_display_mode(FILDES, &cmap, &dbuffer_mode, &bplanes, 
			       &current_buffer) ;
	  if (dbuffer_mode==0) 
	    {
	      number_of_planes =
		  double_buffer(FILDES,TRUE|INIT|SUPPRESS_CLEAR,PLANES/2) ;
	      PRINT(("number of planes per buffer: %d.", number_of_planes));
	    }

	  BUFFER_MODE = DoubleBufferMode ;
	  BUFFER_STATE = 0 ;
	  
	  /* Select clearing type for subsequent clear_view_surface
	     This performs better than zbuffer_switch.
	     For multiple bank devices, only the current bank (set by
	     bank_switch) is cleared. 
	     */
	  clear_control(FILDES,CLEAR_VIEWPORT|CLEAR_ZBUFFER) ; 
	  
	  DPY_SB = DPY ;
	  XWINDOW_ID = window ;
	  SCREEN_SB = screen ;
	  LIGHTSTATE = 0 ;
	  AMBIENT_ID = -1 ;
	  WIN_HEIGHT = dx_height ;
	  XMAXSCREEN = xmaxscreen ;
	  YMAXSCREEN = ymaxscreen ;
	  GNOBE_BUF = (void *) NULL ;
	}
    }
  else
    {
      PRINT(("out of memory allocating private context"));
      DXErrorGoto (ERROR_INTERNAL, "#13000") ;
    }
  
  EXIT(("window = 0x%x", window));
  return window ;
  
 error:
  if(outdevice) free(outdevice) ;
  DXSetError (ERROR_BAD_PARAMETER, "#13670") ; 
  EXIT(("ERROR"));
  return 0 ;
}

extern _dxf_CREATE_HW_TRANSLATION (void *win) ;
