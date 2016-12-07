/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                            */
/*********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwPortXGL.c,v $

 This file contains the implementation of the basic DX porting layer for the
 Sun XGL graphics API.

 XGL splits its transform pipeline into

     local model(LM) -> global model(GM) -> world(WC) -> virtual DC -> DC

 Lighting occurs in world coordinates and the viewing matrix is assumed to
 go from WC directly to VDC.  In the DX port we stuff only the projection
 matrix into the view transform and composite the local modeling, global
 modeling, and viewing matrices into a model/view matrix.
 	
 Directional light sources in XGL point in the direction of the propagation
 of the light, not toward the light sources themselves.

 Specular reflections have to be explicitly enabled. XGL seems to perform a
 local viewer computation for specularity.

 XGL provides an implicit window resize policy, but we must be able to
 provide viewports smaller than the entire DC range for some renderings.
 Therefore we set XGL_CTX_VDC_MAP to XGL_VDC_MAP_OFF and explicitly set the
 viewport whenever the window size changes.

 If the graphics window is to be reparented (owned by the UI process), then
 it must be created from a window covering the whole screen; otherwise, the
 device viewport cannot be made larger than the initial window size.  This
 does not seem to be a restriction for children of the root window.

 According the "SPARCstation 2GS / "SPARCstation 2GT Technical White Paper"
 from Sun, the GT will accelerate screen door transparency.  There isn't any
 mention of this in the XGL manuals.  Screen door is implemented here by
 using a surface fill stipple; however, it's really slow in the GS, so it's
 only enabled for the GT.

 $Log: hwPortXGL.c,v $
 Revision 1.5  2000/05/16 18:48:37  gda
 Changes to compile using MS compilers under Cygwin

 Revision 1.4  1999/05/10 15:45:40  gda
 Copyright message

 Revision 1.4  1999/05/10 15:45:40  gda
 Copyright message

 Revision 1.3  1999/05/03 14:06:43  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.2  1999/04/21 18:40:10  gda
 COLORMAP -> CLRMAP due to conflict on cygwin

 Revision 1.1.1.1  1999/03/24 15:18:36  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:02:42  gda
 OpenDX Baseline

 Revision 9.10  1998/01/12 14:09:03  svs
 added necessary arg to call to SWAP

 * Revision 9.9  1998/01/09  14:02:41  gda
 * stereo compatibility
 *
 Revision 9.8  1997/08/25 20:13:53  paula
 all routines in this file must be static.

 * Revision 9.7  1997/07/31  15:00:12  svs
 * use _dxf_DrawOpaque, which exists, rather than _drawOpaque, which doesn't
 *
 * Revision 9.6  1997/07/29  17:14:55  svs
 * fixed args to DrawTranslucent
 *
 * Revision 9.5  1997/07/29  14:55:56  gda
 * fixed _dxf_DrawTrans to reflect correct arg list.  Looks
 * like xgl doesn't handle transparency.
 *
 * Revision 9.4  1997/06/27  13:35:25  gda
 * StartFrame and EndFrame, usefule (elsewhere) for
 * display list hashing
 *
 * Revision 9.3  1997/06/17  01:36:26  gda
 * *** empty log message ***
 *
 * Revision 9.1.1.2  1997/06/10  21:15:12  gda
 * near, far
 *
 * Revision 9.1.1.1  1997/05/22  22:34:00  svs
 * wintel build AND 3.1.4 demand fixes
 *
 * Revision 9.1  1997/05/22  22:33:56  svs
 * Copy of release 3.1.4 code
 *
 * Revision 8.5  1996/12/20  13:46:26  svs
 * moved SERVICES_FLAGS to hw independent part
 *
 * Revision 8.4  1996/05/07  13:46:41  gda
 * added ReadImage call
 *
 * Revision 8.3  1996/05/03  17:54:20  gda
 * removed questionable 'static' from def. of dxf_SERVICES_FLAGS
 *
 * Revision 8.2  1996/01/18  17:08:39  c1orrang
 * added stubs for 8.0 transparency
 *
 * Revision 8.1  1995/10/16  12:55:05  c1orrang
 * solaris gamma correction <JKAROL307>
 *
 * Revision 8.0  1995/10/03  22:15:36  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.16  1995/08/30  21:09:08  c1orrang
 * Re-doublebuffered solaris build w/ DX_SINGLE_BUFFER_SUN flag
 *
 * Revision 7.15  1994/08/04  08:26:23  nsc
 * ifdef out solaris changes which don't compile under old version of xgl
 *
 * Revision 7.14  94/06/30  16:57:47  nsc
 * changes to make hardware rendering work under solaris.
 * we can't read pixels back from the adaptor, so we are:
 * running single buffered and using X to read back the bytes
 * under the gnomon, and we are allocating our own pixel buffers
 * instead of using the ones created by the system
 * 
 * Revision 7.13  94/06/24  17:05:11  nsc
 * add code for solaris only which goes into single
 * buffer mode until we can work around the double
 * buffer mode bug.
 * 
 * Revision 7.0.2.6  1994/06/20  17:57:01  ellen
 * oops...made xgl_context_post/xgl_context_flush change specific for
 * solaris (and not for sun4)
 *
 * Revision 7.0.2.5  94/06/20  17:35:21  ellen
 * Changed all xgl_context_post to xgl_context_flush (ctx, XGL_FLUSH_BUFFERS) for
 * solaris.
 * 
 * Revision 7.0.2.4  94/06/08  20:04:36  jrush
 * Interface to WRITE_PIXEL_RECT changed from unsigned long *
 * to uint32 * in comman layer because of DEC port.  This
 *  would have caused a compile time warning in this
 * code thus the change.
 * 
 * Revision 7.12  1994/06/08  20:03:37  jrush
 * Interface to WRITE_PIXEL_RECT changed from unsigned long *
 * to uint32 * in comman layer because of DEC port.  This
 * would have caused a compile time warning in this
 * code thus the change.
 *
 * Revision 7.11  1994/06/06  17:58:31  mjh
 * redraw interactor echos in _dxf_WRITE_APPROX_BACKSTORE(), since it's no
 * longer done by _dxfProcessEvents().  Fixes <MJH13>.
 *
 * Revision 7.10  94/04/21  15:35:27  tjm
 * added polylines to sun and hp, fixed mem leak in xf->origConnections and
 * added performance timing marks to sun and hp
 * 
 * Revision 7.9  94/04/12  08:12:27  tjm
 * Fixed CREATE_WIN to use messages file and return error value
 * 
 * Revision 7.8  94/03/31  15:55:19  tjm
 * added versioning
 * 
 * Revision 7.7  94/03/29  09:13:30  tjm
 * added versioning
 * 
 * Revision 7.6  94/03/23  16:34:20  tjm
 * added model clipping 
 * 
 * Revision 7.5  94/03/09  17:16:27  tjm
 * restored DrawOpaque interface, use TRANSLATION->gamma for gamma correction
 * 
 * Revision 7.4  94/02/24  18:31:53  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.3  94/02/07  16:01:06  tjm
 * added services flags to sgi, hp and sun
 * 
 * Revision 7.2  94/01/20  20:10:32  ellen
 * Added mask to pixels in _dxf_DRAW_IMAGE, and added some error
 * checking for WRITE_PIXEL_RECT, and cleaned up some #if 0
 * 
 * Revision 7.1  94/01/18  19:00:07  svs
 * changes since release 2.0.1
 * 
 * Revision 6.5  94/01/18  12:32:43  ellen
 * Cleaned up DRAW_IMAGE to use _dxf_dither, so that the xgl
 * code is more like the 6000 and sgi and software for
 * 2-d images without camera.   Still need to do gamma correction
 * after the RCS branch.
 * 
 * Revision 6.4  94/01/11  17:52:46  ellen
 * Added code to validImage to handle multiple sized arranged
 * 2-d images.  It initializes the space around a small
 * image and handles the bug where the image would wrap
 * with 3 images of multiple sizes.
 * 
 * Revision 6.3  93/12/20  18:19:18  jdurko
 * set numplanes to PIXDEPTH
 * 
 * Revision 6.2  93/12/13  18:02:06  tjm
 * removed tdmAdapter struct
 * 
 * Revision 6.1  93/11/16  10:26:11  svs
 * ship level code, release 2.0
 * 
 * Revision 1.19  93/10/21  11:00:59  tjm
 * removed XCloseDisplay calls. This is now done in _dxfDeletePortHandle
 * 
 * 
 * Revision 1.18  93/09/29  17:56:16  mjh
 * apply hideous screen object hack
 * 
 * Revision 1.17  93/09/29  14:49:20  tjm
 * fixed <PITTS21> wrong background color on sun. 2 problems negative colors
 * in image (now clamp to 0.0) and const colors ignored for mesh primitives
 * (now set const colors when no normals)
 * 
 * Revision 1.16  93/09/23  08:01:01  tjm
 * fixed WRITE_APPROX_BACKING store. Previously we were redrawing every timee
 * we needed to refresh.
 * 
 * Revision 1.15  93/09/22  15:18:57  tjm
 * XGL drawing now returns error code, script windows resize to image, and
 * you can now open a 2nd hw window
 * 
 * Revision 1.14  93/09/14  17:40:29  tjm
 * made changes to support used of LMC_AD for gl, also glObjects
 * 
 * Revision 1.13  93/09/13  18:40:39  ellen
 * Added code for single and multiple 2-d images.  Still	
 * need to add code to Clamp color between 0.0 and 1.0 in
 * validImage.  This code should be in hwrender for HP and
 * Sun.
 * 
 * Revision 1.12  93/08/25  17:20:27  jdurko
 * don't free portHandle->private anymore
 * 
 * Revision 1.11  93/08/25  17:06:04  jdurko
 * set PORT_CTX to NULL after freeing just in case.
 * 
 * Revision 1.10  93/08/24  10:56:37  jdurko
 * put back xgl_open() call
 * 
 * Revision 1.9  93/08/23  18:29:46  tjm
 * changed demand load interface
 * 
 * Revision 1.8  93/08/23  11:42:55  tjm
 * added demand loading
 * 
 * Revision 1.7  93/07/28  16:29:09  tjm
 * removed XCloseDisplay, this caused problems with code that reuses the 
 * XDisplay
 * 
 * Revision 1.6  93/07/24  19:08:06  tjm
 * fixed bug with REPLACE_VIEW_MATRIX, z is now 0,+1 not -1,+1
 * 
 * Revision 1.5  93/07/23  17:29:16  ellen
 * Set available in hwIsAvailable
 * 
 * Revision 1.4  93/07/14  13:29:01  tjm
 * added solaris ifdefs
 * 
 * Revision 1.3  93/07/14  13:16:53  tjm
 * added solaris stuff
 * 
 * Revision 1.2  93/06/29  11:29:26  tjm
 * removed debug statement allowing single buffer.
 * 
 * Revision 1.1  93/06/29  10:01:37  tjm
 * Initial revision
 * 
 * Revision 5.5  93/06/03  15:39:17  ellen
 * Added guts to _DXF_SET_BACKGROUND_COLOR.
 * 
 * Revision 5.4  93/05/18  14:56:23  mjh
 * Disable screen door even on GT.  Hardware acceleration is only 
 * available through XGL 3.0.
 * 
 * Revision 5.3  93/05/11  18:03:51  mjh
 * Implement screen door transparency for the GT.
 * 
 * Revision 5.2  93/05/06  18:40:23  ellen
 * Changed DXWarning, DXError*, and DXSetError with code numbers.
 * 
 * Revision 5.1  93/03/30  14:41:44  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.21  93/03/29  15:38:47  ellen
 * Cleaned up lots of memory leakiness.  Still need to turn off
 * error notification for xgl during tmeshes and drawing big chunks.
 * 
 * Revision 5.0.2.20  93/03/22  18:39:26  ellen
 * There is a bug on xgl (BUG ID#1123335).  No NULL is
 * returned on failed xgl_object_create.  Need to use xgl_object_set to
 * set error notification on _dxf_ALLOCATE_PIXEL_ARRAY.
 * 
 * Revision 5.0.2.19  93/03/12  18:18:23  ellen
 * *** empty log message ***
 * 
 * Revision 5.0.2.18  93/03/10  16:25:38  mjh
 * check light number < MAX_LIGHTS-1
 * 
 * Revision 5.0.2.17  93/02/25  17:45:46  ellen
 * Added code to _dxf_GRAPHICS_NOT_AVAILABLE
 * to check for hostname, display, xgl, etc.
 * 
 * Revision 5.0.2.16  93/02/10  16:15:29  mjh
 * flush XGL output queue before swapping buffers, not after.
 * 
 * Revision 5.0.2.15  93/02/04  19:21:58  mjh
 * Create scratch XGL context in _dxf_CREATE_WINDOW(), not
 * _dxf_INIT_RENDER_MODULE().  The cursor interactor is created before the
 * latter routine is called, and it requires a scratch context.
 * Take sqrt() of specular exponent to match software renderer more 
 * closely.
 * Create all raster objects with a color type of RGB.
 * Miscellaneous changes to commentary.
 * 
 * Revision 5.0.2.14  93/02/02  15:34:14  mjh
 * Fix resize when graphics window is reparented by UI.  Reparenting 
 * seems to restrict the size of the graphics window to a maximum of
 * its initial creation size.  
 * 
 * Revision 5.0.2.13  93/02/01  22:19:40  mjh
 * Implement pixel read/write, gnomon and zoom box echos.
 * 
 * Revision 5.0.2.12  93/01/25  15:06:34  owens
 * added _dxf_SET_BACKGROUND_COLOR
 * 
 * Revision 5.0.2.11  93/01/13  16:57:44  mjh
 * go into double buffer mode in _dxf_INIT_RENDER_MODULE().  Apparently DX
 * no longer calls _dxf_DOUBLE_BUFFER_MODE() explicitly upon window creation.
 * 
 * Revision 5.0.2.10  93/01/12  23:00:17  owens
 * reversed sense of _dxf_GRAPHICS_NOT_AVAILABLE to a rather silly and
 * unobvious interpretation
 * 
 * Revision 5.0.2.9  93/01/12  19:06:35  mjh
 * fix double buffering
 * 
 * Revision 5.0.2.8  93/01/10  22:17:30  mjh
 * add lighting
 * 
 * Revision 5.0.2.7  93/01/09  00:26:49  mjh
 * frame buffer stuff
 * 
 * Revision 5.0.2.6  93/01/08  16:55:21  mjh
 * fix visual selection
 * 
 * Revision 5.0.2.5  93/01/08  13:33:41  mjh
 * implement transformation pipeline
 * 
 * Revision 5.0.2.4  93/01/07  03:52:24  mjh
 * peruse available visuals.  keep track of number of windows open
 * so that xgl_open() and xgl_close() can be called appropriately.
 * 
 * Revision 5.0.2.3  93/01/06  17:00:25  mjh
 * window creation and destruction
 * 
 * Revision 5.0.2.2  93/01/06  04:41:31  mjh
 * initial revision
 * 
\*---------------------------------------------------------------------------*/

/*
 * try to work around solaris bug by not using double buffering.
 * nsc 21jun94
 */
int SunSingleBug = 0;

/*
 *  following define marks workaround code for Sun XGL bug 
 *  Ref ID #1239569   Bug ID #1123335  3/18/93
 */
#define XGL_ALLOC_BUG 1

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <dlfcn.h>

#define STRUCTURES_ONLY
#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwMatrix.h"
#include "hwPortXGL.h"
#include "hwInteractor.h"
#include "hwSort.h"

#ifndef STANDALONE
#include "hwMemory.h"
#endif

#include "hwDebug.h"

#include "hwPortLayer.h"

/* for translationT structure */
#define Object dxObject
#define Matrix dxMatrix
#include "internals.h"
#undef Object
#undef Matrix

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#if 1
#define ___ansi_fflush(n)		fflush(n)
#endif

#define DXHW_DD_VERSION 0x080000

void _dxf_SET_VIEWPORT (void *ctx, int left, int right, int bottom, int top) ;
void _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (void *ctx, int on) ;
void *_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height) ;
void _dxf_FREE_PIXEL_ARRAY (void *ctx, void *pixels) ;
void _dxf_SINGLE_BUFFER_MODE (void *ctx) ;
void _dxf_DOUBLE_BUFFER_MODE (void *ctx) ;
static void _dxf_WRITE_PIXEL_RECT(void *win, uint32 *foo, int x, int y,
		      int camw, int camh);
static hwTranslationP   _dxf_CREATE_HW_TRANSLATION(void *win) ;


/* The XGL system state object, one per process (XGL session) */
#ifdef solaris
static Xgl_sys_state sys_st ;
#else
static Xgl_sys_st sys_st ;
#endif
static int _num_windows_open = 0 ;
static int stackDepth = -5000;

/* Used by _dxf_ALLOCATE_PIXEL_ARRAY because of bug */
static int error_flag = 0 ;

/* 50% screen door transparency pattern */
static unsigned long screen_50[32] = {
  0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
  0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
  0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
  0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
  0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa, 0x55555555, 0xaaaaaaaa,
  0x55555555, 0xaaaaaaaa} ;

static Visual *
_tdmGetVisual (Display *dpy, int scr, int depth, int visualClass)
{
  int n, i ;
  Visual *vis ;
  XVisualInfo desc, *info, *v ;

  desc.screen = scr ;
  desc.depth = depth ;
  info = XGetVisualInfo (dpy, VisualScreenMask | VisualDepthMask, &desc, &n) ;
  
  for (vis=0, v=info, i=0 ; i<n ; i++, v++)
      if (v->class == visualClass)
	{
	  vis = v->visual ;
	  break ;
	}

  XFree((char *)info) ;
  return vis ;
}

static void
_tdmGetHardwareVisual (Display *dpy, int scr, int *depth, Visual **vis)
{
  /*
   *  Try to find a visual which will give the best results on this
   *  particular frame buffer.
   */
  
  DPRINT("\n(_tdmGetHardwareVisual") ;
  if ((*vis = _tdmGetVisual (dpy, scr, 24, TrueColor)))
    {
      DPRINT("\nfound a TrueColor visual") ;
      *depth = 24 ;
    }
  else if ((*vis = _tdmGetVisual (dpy, scr, 24, DirectColor)))
    {
      DPRINT("\nfound a DirectColor visual") ;
      *depth = 24 ;
    }
  else if ((*vis = _tdmGetVisual (dpy, scr, 8, PseudoColor)))
    {
      DPRINT("\nfound a PseudoColor visual") ;
      *depth = 8 ;
    }
  else
      *depth = 0 ;

  DPRINT(")") ;
}
#ifdef solaris
_tdmEnufMemory(Xgl_sys_state sys_st)
{
  Xgl_error_info errInfo;

  xgl_object_get(sys_st,XGL_SYS_ST_ERROR_INFO,&errInfo) ;
  error_flag = (int)errInfo.category ;
}
#else
static void 
_tdmEnufMemory(Xgl_error_type type, int num, char *msg, char *op_name,
        char *obj_name, char *operand)
{
        DPRINT ("\n(_tdmEnufMemory)") ;
        msg = NULL ;
        error_flag = num ;
}
#endif

static int
_dxf_SET_BACKGROUND_COLOR (void *ctx, RGBColor background_color)
{
  Xgl_color 	color ;
  DEFCONTEXT(ctx) ;
  DPRINT ("\n(_dxf_SET_BACKGROUND_COLOR)") ;

  color.rgb.r = pow(background_color.r,1./XGLTRANSLATION->gamma) ;
  color.rgb.g = pow(background_color.g,1./XGLTRANSLATION->gamma) ;
  color.rgb.b = pow(background_color.b,1./XGLTRANSLATION->gamma) ;

  xgl_object_set (XGLCTX, XGL_CTX_BACKGROUND_COLOR, &color, 0) ;
}


void*
_dxf_CREATE_WINDOW (void *globals, char *winName, int w, int h)
{
  /*
   *  Create a new hardware graphics window with specified name and
   *  dimensions.  Return the X identifier associated with the window.
   */

  DEFGLOBALDATA(globals) ;
  Window xid ;
  Visual *vis ;
  XSetWindowAttributes attrs ;
  int screen, depth, buffers ;
  tdmXGLctx tdmCtx = 0 ;
  DPRINT("\n(xglTDM_CREATE_WINDOW") ;

  /* 
   * we can't seem to read pixels back from the adaptor correctly. 
   * since we can't figure out if it's an xgl bug or some bug in our code
   * and we can't display the gnomon correctly without getting the pixels
   * back, the workaround for now is to run single buffer (so X knows which
   * buffer is visible since there is only 1) and to use X to read the
   * pixels back instead of asking the xgl lib for them.  nsc 30jun94 
   */

#if 0
#if solaris
  SunSingleBug = 1;
  _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE);
#endif
#else
   if(getenv("DX_SINGLE_BUFFER_SUN"))
   {
      SunSingleBug = 1;
      _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE);
   }
#endif

  /*
   *  Allocate context for XGL-specific data.
   *  It's deallocated in _dxf_DESTROY_WINDOW().
   */
  
  if (! (tdmCtx = 
	 PORT_HANDLE->private = (void *) tdmAllocateZero(sizeof(tdmXGLctxT))))
    {
      DPRINT("\nout of memory allocating private context)") ;
      DXErrorGoto (ERROR_INTERNAL, "#13000") ;
    }
  else
#if 0
      memset ((void *)tdmCtx, 0, sizeof(tdmXGLctxT)) ;
#else
  {
  DEFPORT(PORT_HANDLE) ;
  XGLTRANSLATION = _dxf_CREATE_HW_TRANSLATION(LWIN) ;
  }
#endif

  /*
   *  Open an X window with an appropriate visual type.
   */
  
  screen = DefaultScreen(DPY) ;
  tdmCtx->screen_xmax = DisplayWidth(DPY, screen) - 1 ;
  tdmCtx->screen_ymax = DisplayHeight(DPY, screen) - 1 ;

  _tdmGetHardwareVisual (DPY, screen, &depth, &vis) ;
  DPRINT1 ("\nvisual depth: %d", depth) ;
  if (depth == 0) {
    /* Unable to find acceptable X visual */
    DXErrorGoto(ERROR_INTERNAL,"#11720");
  }

  attrs.border_pixel = None ;
  attrs.background_pixel = None ;
  attrs.colormap = XCreateColormap (DPY, DefaultRootWindow(DPY),
				    vis, AllocNone) ;

  if (PARENT_WINDOW)
    {
      /* UI will reparent and clip this window */
      w = tdmCtx->screen_xmax +1 ;
      h = tdmCtx->screen_ymax +1 ;
    }
  
  if (! (xid = XCreateWindow (DPY, DefaultRootWindow(DPY),
			      0, 0, w, h, 0, depth, InputOutput, vis,
			      CWBackPixel | CWBorderPixel | CWColormap,
			      &attrs)))
    {
      DPRINT("\nCould not create X window)") ;
      DXErrorGoto (ERROR_INTERNAL, "#13770") ;
    }
  else
      XStoreName (DPY, xid, isdigit(winName[0]) ? "DX" : winName) ; 

  tdmCtx->num_planes = PIXDEPTH = depth;
      
  /*
   *  initialize XGL window objects
   */
  
  tdmCtx->xgl_x_win.X_display = DPY ;
  tdmCtx->xgl_x_win.X_window = xid ;
  tdmCtx->xgl_x_win.X_screen = screen ;
  
  tdmCtx->obj_desc.win_ras.type = XGL_WIN_X ;
  tdmCtx->obj_desc.win_ras.desc = &tdmCtx->xgl_x_win ;

  /* create raster from X window, request double buffers and DXRGB colors */
  /*  Called Sun Software about bug.  xgl_object_create fails, but
   *  doesn't return a NULL (as documented).
   *  Ref ID #1239569   Bug ID #1123335  3/18/93
   */
#ifdef XGL_ALLOC_BUG
  error_flag = 0 ;
  xgl_object_set (sys_st,
        XGL_SYS_ST_ERROR_NOTIFICATION_FUNCTION, _tdmEnufMemory, NULL) ;
#endif

  tdmCtx->ras = xgl_object_create
      (sys_st, XGL_WIN_RAS, &tdmCtx->obj_desc,
       XGL_DEV_COLOR_TYPE, XGL_COLOR_RGB,
       XGL_WIN_RAS_BUFFERS_REQUESTED, (SunSingleBug ? 1 : 2),
       NULL) ;

#ifdef XGL_ALLOC_BUG
  if (error_flag != 0 || !tdmCtx->ras)
#else
  if (!tdmCtx->ras) 
#endif
    {
      DXErrorGoto(ERROR_INTERNAL,"could not create xgl raster") ;
    }

  /* check result of numbuffer request */
  xgl_object_get
      (tdmCtx->ras, XGL_WIN_RAS_BUFFERS_ALLOCATED, &tdmCtx->num_buffers) ;

  if (SunSingleBug) {
    DPRINT1 ("\nrequested 1 buffers, got %d", tdmCtx->num_buffers) ;
    if (tdmCtx->num_buffers != 1)
      DPRINT("\ndidn't get expected number of buffers") ;
  } else {
    DPRINT1 ("\nrequested 2 buffers, got %d", tdmCtx->num_buffers) ;
    if (tdmCtx->num_buffers < 2)
      DPRINT("\nhardware double buffering not available") ;
  }

  /* get raster current maximum coordinates */
  xgl_object_get (tdmCtx->ras,
		  XGL_DEV_MAXIMUM_COORDINATES, &tdmCtx->dev_max_coords) ;
  
  DPRINT1 ("\nmaximum X value: %f", tdmCtx->dev_max_coords.x) ;
  DPRINT1 ("\nmaximum Y value: %f", tdmCtx->dev_max_coords.y) ;
  DPRINT1 ("\nmaximum Z value: %f", tdmCtx->dev_max_coords.z) ;

  /* create XGL graphics context */
  tdmCtx->xglctx = xgl_object_create
      (sys_st, XGL_3D_CTX, NULL,
       XGL_CTX_DEVICE, tdmCtx->ras,
       XGL_CTX_MODEL_TRANS_STACK_SIZE, 64,
       XGL_CTX_VDC_ORIENTATION,  XGL_Y_UP_Z_TOWARD,
       XGL_CTX_VDC_MAP,          XGL_VDC_MAP_OFF,
#ifdef solaris
       XGL_3D_CTX_HLHSR_MODE,    XGL_HLHSR_Z_BUFFER,
#else
       XGL_3D_CTX_HLHSR_MODE,    XGL_HLHSR_ZBUFFER,
#endif
       XGL_CTX_NEW_FRAME_ACTION, XGL_CTX_NEW_FRAME_CLEAR |
                                 XGL_CTX_NEW_FRAME_HLHSR_ACTION,
       XGL_CTX_CLIP_PLANES,      XGL_CLIP_XMIN | XGL_CLIP_XMAX |
                                 XGL_CLIP_YMIN | XGL_CLIP_YMAX |
                                 XGL_CLIP_ZMIN | XGL_CLIP_ZMAX,
       XGL_CTX_DEFERRAL_MODE,    XGL_DEFER_ASTI,
       XGL_3D_CTX_LIGHT_NUM,     MAX_LIGHTS,
       NULL) ;
  
#ifdef XGL_ALLOC_BUG
  if (error_flag != 0 || !tdmCtx->xglctx)
#else
  if (!tdmCtx->xglctx)
#endif
    {
      DXErrorGoto(ERROR_INTERNAL,"could not create xgl graphics context") ;
    }

  stackDepth = 0;
  DPRINT1("\nStack Depth = %d", stackDepth);

  /* create a scratch XGL context */
  tdmCtx->xgl_tmp_ctx = xgl_object_create (sys_st, XGL_3D_CTX, NULL, 
#ifdef solaris
       XGL_3D_CTX_HLHSR_MODE,    XGL_HLHSR_Z_BUFFER,
#endif
	NULL);

#ifdef XGL_ALLOC_BUG
  if (error_flag != 0 || !tdmCtx->xgl_tmp_ctx)
#else
  if (!tdmCtx->xgl_tmp_ctx)
#endif
    {
      DXErrorGoto(ERROR_INTERNAL,"could not create scratch xgl graphics context") ;
    }

  /* get some additional info about the hardware */
#ifdef solaris
  tdmCtx->hw_info = (Xgl_inquire *) xgl_inquire (sys_st, &tdmCtx->obj_desc) ;
#else
  tdmCtx->hw_info = (Xgl_inquire *) xgl_inquire (&tdmCtx->obj_desc) ;
#endif
  DPRINT1 ("\nframe buffer name: %s", tdmCtx->hw_info->name) ;

#if 0
  if (strcmp (tdmCtx->hw_info->name, "Sun:GT"))
      tdmCtx->use_screen_door = 0 ;
  else
      tdmCtx->use_screen_door = 1 ;
#else
  /*
   *  Turns out that hardware acceleration for screen door transparency is
   *  only available through XGL 3.0 or SunPhigs 2.0.
   */
  tdmCtx->use_screen_door = 0 ;
#endif
      
  /* ship all buffered XGL commands */
#ifndef solaris
  xgl_context_post (tdmCtx->xglctx, 0) ;
#else
  xgl_context_flush (tdmCtx->xglctx, XGL_FLUSH_BUFFERS) ;
#endif

  /* OK, we have at least one window open now */
  _num_windows_open++ ;

  DPRINT(")") ;
  return (void*)xid ;

 error:
  if (tdmCtx)
    {
      if (tdmCtx->ras) xgl_object_destroy(tdmCtx->ras) ;
      if (tdmCtx->xglctx) xgl_object_destroy(tdmCtx->xglctx) ;
      tdmFree(tdmCtx) ;
    }

  DPRINT("\nerror)") ;
  return 0 ;
}

void
_dxf_DESTROY_WINDOW (void *win)
{
  /*
   *  Destroy specified window and its resources.
   */

  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  DPRINT ("\n(xglTDM_DESTROY_WINDOW") ;

  if (SAVE_BUF)
    {
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
      SAVE_BUF = (void *) NULL ;
    }

  if (XGLTRANSLATION)
    {
      tdmFree(XGLTRANSLATION);
      XGLTRANSLATION = NULL;
    }

  if (SW_BUF) {
    tdmFree(SW_BUF) ;
    SW_BUF = NULL ;
    SW_BUF_SIZE = 0 ;
    }

  if (PORT_CTX)
    {

      if (XGLCTX)
	{
#ifndef solaris
	  xgl_context_post (XGLCTX, 0) ;
#else
          xgl_context_flush (XGLCTX, XGL_FLUSH_BUFFERS) ;
#endif
	  xgl_object_destroy(XGLCTX) ;
	}

      if (XGLRAS) xgl_object_destroy(XGLRAS) ;
      if (XGLTMPCTX) xgl_object_destroy(XGLTMPCTX) ;
      if (TMP_TRANSFORM) xgl_object_destroy(TMP_TRANSFORM) ;
      if (SCREEN_DOOR_50) xgl_object_destroy(SCREEN_DOOR_50) ;
      if (HW_INFO) DXFree(HW_INFO) ; /* xgl allocated this data... */
      tdmFree(PORT_CTX) ;           /* ...we allocated this */
      PORT_CTX = (void *) NULL ;        /* in case we try to free again */
    }

  _num_windows_open-- ;

  DPRINT(")") ;
}

void
_dxf_SET_OUTPUT_WINDOW (void *win, Window w)
{
  /*
   *  Direct all graphics to specified window.  Not needed for XGL.
   */
}

void
_dxf_SET_WINDOW_SIZE (void *win, int w, int h)
{
  /*
   *  Set graphics window to the specified size.
   */

  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  DPRINT("\n(xglTDM_SET_WINDOW_SIZE") ;
  DPRINT1("\n%dx", w) ; DPRINT1("%d", h) ;

  VP_XMAX = w - 1;
  VP_YMAX = h - 1;
  XResizeWindow (DPY, XWINID, w-1, h-1) ;
  XSync (DPY, False) ;
  _dxf_SET_VIEWPORT (PORT_CTX, 0, w-1, 0, h-1) ;
  DPRINT(")") ;
}

 Error
_dxf_INIT_RENDER_MODULE (void *globals)
{
  /*
   *  This routine is called once, after the graphics window has been
   *  created.  It's function is loosely defined to encompass all the
   *  `first time' initialization of graphics window.
   */

  DEFGLOBALDATA(globals) ;
  DEFPORT(PORT_HANDLE) ;
  DPRINT("\n(xglTDM_INIT_RENDER_MODULE") ;

#ifdef XGL_ALLOC_BUG
  error_flag = 0 ;
#endif

  /* create a scratch transform object */
  TMP_TRANSFORM = xgl_object_create (sys_st, XGL_TRANS, NULL, 0) ;

#ifdef XGL_ALLOC_BUG
  if (error_flag != 0 || !TMP_TRANSFORM)  
#else
  if (!TMP_TRANSFORM)
#endif
      DXErrorGoto (ERROR_NO_MEMORY, "#13000") ;

  /* treat global modeling transform as composite of model and view */
  xgl_object_get (XGLCTX, XGL_CTX_GLOBAL_MODEL_TRANS, &MODEL_VIEW_TRANSFORM) ;

  /* treat viewing transform as a projection matrix */
  xgl_object_get (XGLCTX, XGL_CTX_VIEW_TRANS, &PROJECTION_TRANSFORM) ;
  
  /* create screen door transparency pattern */
  SCREEN_DOOR_50 = xgl_object_create
      (sys_st, XGL_MEM_RAS, 0,
       XGL_RAS_DEPTH, 1, XGL_RAS_WIDTH, 32, XGL_RAS_HEIGHT, 32,
       NULL) ;

#ifdef XGL_ALLOC_BUG
  if (error_flag != 0 || !SCREEN_DOOR_50)  
#else
  if (!SCREEN_DOOR_50)
#endif
      DXErrorGoto (ERROR_NO_MEMORY, "#13000") ;

#ifdef solaris
  xgl_object_set (SCREEN_DOOR_50, XGL_MEM_RAS_IMAGE_BUFFER_ADDR, &screen_50, 0);
#else
  xgl_object_set (SCREEN_DOOR_50, XGL_MEM_RAS_MEMORY_ADDRESS, &screen_50, 0) ;
#endif
	  
  if (SunSingleBug) {
    /* go into single buffer mode */
    _dxf_SINGLE_BUFFER_MODE(PORT_CTX) ; 
    BUFFER_MODE = SingleBufferMode ;
  } else {
    /* go into double buffer mode */
    _dxf_DOUBLE_BUFFER_MODE(PORT_CTX) ; 
    BUFFER_MODE = DoubleBufferMode ;
  }

  /* initialize all lights OFF.  they have active default values. */
  _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (PORT_CTX, 0) ;

  /* mark the image hardware approximation save buffer unallocated */
  SAVE_BUF = 0 ;
  SAVE_BUF_SIZE = 0 ;

#if 0
  /*
   *  Try to load the rendering options file.  We need to do this to set
   *  up rendering approximation options this even though the options file
   *  itself is not used.  Crufty.
   */
  if (!_dxfLoadRenderOptionsFile(globals))
      DXErrorGoto(ERROR_DATA_INVALID, "#13760") ;
#endif

  DPRINT(")") ;
  return 1 ;

 error:
  DPRINT("\nerror)") ;
  return 0 ;
}

void
_dxf_SINGLE_BUFFER_MODE (void *ctx)
{
  /*
   *  Go into single buffer mode
   */

  DEFCONTEXT(ctx) ;
  int num_buffers ;
  DPRINT("\n(xgl_TDM_SINGLE_BUFFER_MODE") ;

  xgl_object_set (XGLRAS,
		  XGL_WIN_RAS_BUF_DRAW,          0,
		  XGL_WIN_RAS_BUF_DISPLAY,       0,
		  XGL_WIN_RAS_BUFFERS_REQUESTED, 1,
		  0) ;

  /* check result of single buffer request */
  xgl_object_get
      (XGLRAS, XGL_WIN_RAS_BUFFERS_ALLOCATED, &num_buffers) ;

  DPRINT1 ("\nrequested 1 buffers, got %d", num_buffers) ;

  DRAW_BUFFER = 0 ;
  DISPLAY_BUFFER = 0 ;
  DOUBLE_BUFFER_MODE = 0 ;
  BUFFER_CONFIG_MODE = BOTHBUFFERS ;
  
#ifndef solaris
  xgl_context_post (XGLCTX, 0) ;
#else
  xgl_context_flush (XGLCTX, XGL_FLUSH_BUFFERS) ;
#endif
  DPRINT(")") ;
}

void
_dxf_DOUBLE_BUFFER_MODE (void *ctx)
{
  /*
   *  Go into double buffer mode.
   */

  DEFCONTEXT(ctx) ;
  DPRINT("\n(xglTDM_DOUBLE_BUFFER_MODE") ;

  if (NUM_BUFFERS > 1)
    {
      xgl_object_set (XGLRAS,
		      XGL_WIN_RAS_BUFFERS_REQUESTED, 2,
		      0) ;

      xgl_object_set (XGLRAS,
		      XGL_WIN_RAS_BUF_DRAW, 1,
		      XGL_WIN_RAS_BUF_DISPLAY, 0,
		      0) ;

      DRAW_BUFFER = 1 ;
      DISPLAY_BUFFER = 0 ;
      DOUBLE_BUFFER_MODE = 1 ;
      BUFFER_CONFIG_MODE = BCKBUFFER ;

#ifndef solaris
      xgl_context_post (XGLCTX, 0) ;
#else
      xgl_context_flush (XGLCTX, XGL_FLUSH_BUFFERS) ;
#endif
    }
  DPRINT(")") ;
}

void
_dxf_SWAP_BUFFERS (void *ctx, Window w)
{
  /*
   *  Switch front and back buffers.  This is always called at the end of
   *  a rendering pass regardless of double/single buffer mode.  Always
   *  flush XGL output queue before swapping buffers.
   */

  DEFCONTEXT(ctx) ;
  DPRINT("\n(xglTDM_SWAP_BUFFERS") ;

#ifndef solaris
  xgl_context_post (XGLCTX, 0) ;
#else
  xgl_context_flush (XGLCTX, XGL_FLUSH_BUFFERS) ;
#endif

  if (DOUBLE_BUFFER_MODE)
      if (DISPLAY_BUFFER == 1)
	{
	  xgl_object_set (XGLRAS,
			  XGL_WIN_RAS_BUF_DRAW, 1,
			  XGL_WIN_RAS_BUF_DISPLAY, 0,
			  NULL) ;
	  DRAW_BUFFER = 1 ;
	  DISPLAY_BUFFER = 0 ;
	}
      else
	{
	  xgl_object_set (XGLRAS,
			  XGL_WIN_RAS_BUF_DRAW, 0,
			  XGL_WIN_RAS_BUF_DISPLAY, 1,
			  NULL) ;
	  DRAW_BUFFER = 0 ;
	  DISPLAY_BUFFER = 1 ;
	}
  DPRINT(")") ;
}

void
_dxf_CLEAR_AREA (void *ctx, int left, int right, int bottom, int top)
{
  /*
   *  Clear image and zbuffer planes in specified pixel bounds.
   */

  DEFCONTEXT(ctx) ;
  int vp_left, vp_right, vp_bottom, vp_top ;

  DPRINT ("\n(xglTDM_CLEAR_AREA") ;
  DPRINT1 ("\nleft right bottom top %d, ", left) ;
  DPRINT1 ("%d, ", right) ; DPRINT1 ("%d, ", bottom) ; DPRINT1 ("%d ", top) ;
 
  /* save original viewport */
  vp_left = VP_LEFT ; vp_right = VP_RIGHT ;
  vp_bottom = VP_BOTTOM ; vp_top = VP_TOP;

  /* set new viewport */
  _dxf_SET_VIEWPORT (ctx, left, right, bottom, top) ;

  /* clear rendering surface */
  xgl_context_new_frame(XGLCTX) ;

  /* reset the original viewport */
  _dxf_SET_VIEWPORT (ctx, vp_left, vp_right, vp_bottom, vp_top) ;
  DPRINT(")") ;
}

void
_dxf_WRITE_PRECISE_RENDERING (void *win, int camw, int camh)
{
  /*
   *  DXWrite pixels from software rendering buffer (SW_BUF) into the main
   *  graphics window.  camw and camh are the DX camera resolutions in width
   *  and height; these may differ from the graphics window dimensions.
   */

  DEFWINDATA(win) ;
}


static int
_dxf_READ_APPROX_BACKSTORE (void *win, int camw, int camh)
{
  /*
   *  DXRead pixels from the main graphics window into the hardware saveunder
   *  buffer (SAVE_BUF). camw and camh are the DX camera resolutions in width
   *  and height; these may differ from the graphics window dimensions.
   *
   *  This is required in the XGL port for interactor echos, even though
   *  we don't use _dxf_WRITE_APPROX_BACKSTORE().
   */

  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  DPRINT ("\n(xglTDM_READ_APPROX_BACKSTORE") ;
  DPRINT1 ("\nSAVE_BUF = 0x%x", SAVE_BUF) ;

  if (SAVE_BUF_SIZE !=  camw * camh)
    {
      if (SAVE_BUF) 
	{
	  _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
	  SAVE_BUF = (void *) NULL ;
	}
      
      SAVE_BUF_SIZE = camw * camh ;
      DPRINT1 ("\nSAVE_BUF_SIZE = %d", SAVE_BUF_SIZE) ;
      
      /* allocate a buffer */
      if (!(SAVE_BUF = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, camw, camh)))
	{
	  DPRINT("\nout of memory)") ;
	  DXSetError (ERROR_NO_MEMORY, "#13000") ;
	  return 0 ;
	}
      /*
       * If we are running against an adapter that does not do
       * readpixels well (i.e. E&S) then simply fill
       * the buffer with black pixels.  Return OK so
       * common layer uses the buffer for the interactors.
       */
      if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
			  SF_INVALIDATE_BACKSTORE)) {

#if defined(PRIVATE_PIXELS)
	  Xgl_usgn32 *pixarray;

	  /* get address of buffer associated with portctx and zero it */
	  xgl_object_get(SAVE_BUF, XGL_MEM_RAS_IMAGE_BUFFER_ADDR, &pixarray);
	  /* the +1 is because the buffer needs to have an even width */
	  bzero(pixarray, (camw+1) * camh * ((NUM_PLANES < 24) ? 1 : 4));
#endif
	  DPRINT("done)") ;
	  return OK ;
      }
    }
  
  if (!_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
		       SF_INVALIDATE_BACKSTORE)) {
    /* read entire extent of image from frame buffer */
    _dxf_READ_BUFFER (PORT_CTX, 0, 0, camw-1, camh-1, SAVE_BUF) ;
  }

  DPRINT(")") ;
  return 1 ;
}

void
_dxf_WRITE_APPROX_BACKSTORE (void *win, int camw, int camh)
{
  /*
   *  DXWrite pixels from hardware saveunder buffer (SAVE_BUF) into the main
   *  graphics window.  camw and camh are the DX camera resolutions in width
   *  and height; these may differ from the graphics window dimensions.
   *
   */

  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  DPRINT ("\n(xglTDM_WRITE_APPROX_BACKSTORE") ;

  _dxf_WRITE_BUFFER(PORT_CTX, 0, 0, camw-1, camh-1, SAVE_BUF);

  /* redraw interactor echos; can't save them in SAVE_BUF */
  _dxfRedrawInteractorEchos(INTERACTOR_DATA) ; 

  /* ensure that draw buffer rendering is completed */
#ifndef solaris
  xgl_context_post (XGLTMPCTX, 0) ;
#else
  xgl_context_flush (XGLTMPCTX, XGL_FLUSH_BUFFERS) ;
#endif

  _dxf_SWAP_BUFFERS(PORT_CTX, (Window)win);

  DPRINT("\ndone)") ;
}


void
_dxf_INIT_RENDER_PASS (void *win, int bufferMode, int zBufferMode)
{
  /*
   *  Clear the window and prepare for a rendering pass.
   */
  
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  DPRINT("\n(xglTDM_INIT_RENDER_PASS") ;

  /* next call made to convince DI layer we're really in double buffer mode */
  _dxfChangeBufferMode (win, bufferMode) ;

  SAVE_BUF_VALID = FALSE ;

  DPRINT(")") ;
}


void
_dxf_INIT_SW_RENDER_PASS (void *win)
{
  /*
   *  Clear the screen and prepare for writing a software-rendered image.
   */
  
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  DPRINT("\n(xglTDM_INIT_SW_RENDER_PASS") ;

  xgl_context_new_frame(XGLCTX) ;

  DPRINT(")") ;
}

void
_dxf_MAKE_PROJECTION_MATRIX (int projection, float width, float aspect,
			    float Near, float Far, register float M[4][4])
{
  /*
   *  Return a projection matrix for a simple orthographic or perspective
   *  camera model (no oblique projections).  This matrix projects view
   *  coordinates into a normalized projection coordinate system spanning
   *  -1..1 in X and Y.
   *
   *  The projection parameter is 1 for perspective views and 0 for
   *  orthogonal views.  Width is in world units for orthogonal views.
   *  For perspective views, the width parameter is the field of view
   *  expressed as the ratio of field width to eye distance, or twice the
   *  tangent of half the angle of view.  Aspect is the height of the
   *  window expressed as a fraction of the window width.
   *
   *  The Near and Far parameters are distances from the view coordinate
   *  origin along the line of sight.  The Z clipping planes are at -Near
   *  and -Far since we are using a right-handed coordinate system.  For
   *  perspective, Near and Far must be positive in order to avoid drawing
   *  objects at or behind the eyepoint depth.
   *
   *  XGL provides default view clip bounds and a default VDC window of
   *  [-1,1] x [-1,1] x [0,1], so we don't need to alter it.  We use
   *  right-handed coordinate systems in DX; for perspective projections,
   *  the derivation of the projection matrix is easiest if we transform
   *  visible primitives into the -w half-space.  The Sun hardware cannot
   *  accelerate clipping in -w, so we negate a few matrix elements to
   *  force w positive.
   */

  DPRINT("\n(xglTDM_MAKE_PROJECTION_MATRIX") ;

  M[0][0]=1/(width/2) ; M[0][1]=0 ;              M[0][2]=0 ; M[0][3]=0 ;
  M[1][0]=0 ;           M[1][1]=M[0][0]/aspect ; M[1][2]=0 ; M[1][3]=0 ;
  M[2][0]=0 ;           M[2][1]=0 ;              M[2][2]=1 ; M[2][3]=0 ;
  M[3][0]=0 ;           M[3][1]=0 ;              M[3][2]=0 ; M[3][3]=1 ;

  Far = -Far ;
  Near = -Near ;

  if (projection)
    {
      /* perspective: see Newman & Sproull equations 23-2 and 23-4 */
      float alpha =    1 / (1 - Far/Near) ;
      float beta  = -Far / (1 - Far/Near) ;

      /* negate matrix elements to force w positive */
      M[2][2] = -alpha ;
      M[3][2] = -beta ;
      M[2][3] = -1 ;
      M[3][3] =  0 ;
    }
  else
    {
      /* orthographic */
      M[2][2] = -1 / (Far-Near) ;
      M[3][2] =  1 + (Near / (Far-Near)) ;
    }

  DPRINT(")") ;
}

void
_dxf_SET_ORTHO_PROJECTION
    (void *ctx, float width, float aspect, float Near, float Far)
{
  /*
   *  Set up an orthographic view projection and load it into the hardware.
   */

  DEFCONTEXT(ctx) ;
  float M[4][4] ;

  DPRINT ("\n(xglTDM_SET_ORTHO_PROJECTION") ;
  DPRINT1 ("\nwidth aspect %f, ", width) ;  DPRINT1 ("%f", aspect) ; 
  DPRINT1 ("\nNear  Far    %f, ", Near) ;   DPRINT1 ("%f", Far) ;

  _dxf_MAKE_PROJECTION_MATRIX (0, width, aspect, Near, Far, M) ;

  /*
   *  Make XGL's model/world transformation pipeline act like GL's
   *  view/projection pipeline split by loading only the projection
   *  component into the viewing matrix.
   *
   *  Note: this maps the GL (and TDM) notion of view coordinates into
   *  XGL's world coordinates.
   */
  
  xgl_transform_write (PROJECTION_TRANSFORM, (void *)M) ;

  DPRINT("\ncalled xgl_transform_write() with matrix\n") ; MPRINT(M) ;
  DPRINT (")") ;
}

void
_dxf_SET_PERSPECTIVE_PROJECTION
    (void *ctx, float fov, float aspect, float Near, float Far)
{
  /*
   *  Set up a perspective projection and load it into the hardware.
   */

  DEFCONTEXT(ctx) ;
  float M[4][4] ;

  DPRINT ("\n(xglTDM_SET_PERSPECTIVE_PROJECTION") ;
  DPRINT1 ("\nfov  aspect %f, ", fov) ;  DPRINT1 ("%f", aspect) ; 
  DPRINT1 ("\nNear Far    %f, ", Near) ; DPRINT1 ("%f", Far) ;

  _dxf_MAKE_PROJECTION_MATRIX (1, fov, aspect, Near, Far, M) ;

  /*
   *  Make XGL's model/world transformation pipeline act like GL's
   *  view/projection pipeline split by loading only the projection
   *  component into the viewing matrix.
   *
   *  Note: this maps the GL (and tdm) notion of view coordinates into
   *  XGL's world coordinates.
   */
  
  xgl_transform_write (PROJECTION_TRANSFORM, (void *)M) ;

  DPRINT("\ncalled xgl_transform_write() with matrix\n") ; MPRINT(M) ;
  DPRINT (")") ;
}


void
_dxf_REPLACE_VIEW_MATRIX(void *ctx, float M[4][4])
{
  float M2[4][4] ;
  DEFCONTEXT(ctx) ;
  DPRINT ("\n(_dxf_REPLACE_VIEW_MATRIX\n") ; MPRINT(M) ; 

  if (M[0][0]==1.0 && M[0][1]==0.0 && M[0][2]==0.0 && M[0][3]==0.0 &&
      M[1][0]==0.0 && M[1][1]==1.0 && M[1][2]==0.0 && M[1][3]==0.0 &&
      M[2][0]==0.0 && M[2][1]==0.0 && M[2][2]==1.0 && M[2][3]==0.0 &&
      M[3][0]==0.0 && M[3][1]==0.0 && M[3][2]==0.0 && M[3][3]==1.0)
    {
      /*
       *  Here is a hideous hack for screen objects...
       *  Default Z for M is -1,+1: we need 0,+1 here, so scale and translate.
       */
      
      DPRINT("\nreceived screen object identity matrix...") ;

      COPYMATRIX (M2, M) ;
      M2[2][2] *= -0.5 ;
      M2[3][2] = M2[3][2] * (-0.5) + 0.5 ;
      
      xgl_transform_write (PROJECTION_TRANSFORM, (void *)M2) ;
      DPRINT("\ncalled xgl_transform_write() with:\n") ; MPRINT(M2) ;
    }
  else
      xgl_transform_write (PROJECTION_TRANSFORM, (void *)M) ;

  DPRINT(")") ;
}


void
_dxf_SET_VIEWPORT (void *ctx, int left, int right, int bottom, int top)
{
  /*
   *  Set up viewport in pixels.
   */

  DEFCONTEXT(ctx) ;
#ifdef solaris
  Xgl_bounds_d3d vp ;
#else
  Xgl_bounds_f3d vp ;
#endif

  DPRINT ("\n(xglTDM_SET_VIEWPORT") ;
  DPRINT1 ("\nleft right bottom top: %d, ", left) ;
  DPRINT1 ("%d, ", right) ; DPRINT1 ("%d, ", bottom) ; DPRINT1 ("%d", top) ;

  vp.xmin = left ;           vp.xmax = right ;
  vp.ymin = VP_YMAX - top ;  vp.ymax = VP_YMAX - bottom ;
  vp.zmin = 0 ;              vp.zmax = VP_ZMAX ;

  DPRINT("\nsetting XGL_CTX_DC_VIEWPORT") ;
  DPRINT1("\nvp.xmin %f", vp.xmin) ; DPRINT1(" vp.xmax %f", vp.xmax) ;
  DPRINT1("\nvp.ymin %f", vp.ymin) ; DPRINT1(" vp.ymax %f", vp.ymax) ;
  DPRINT1("\nvp.zmin %f", vp.zmin) ; DPRINT1(" vp.zmax %f", vp.zmax) ;

  xgl_object_set (XGLCTX, XGL_CTX_DC_VIEWPORT, &vp, 0) ;
#ifndef solaris
  xgl_context_post (XGLCTX, 0) ;
#else
  xgl_context_flush (XGLCTX, XGL_FLUSH_BUFFERS) ;
#endif

  /* save viewport dimensions */
  VP_LEFT   = left ;   VP_RIGHT  = right ;
  VP_BOTTOM = bottom ; VP_TOP    = top ; 
  DPRINT(")") ;
}

void
_dxf_LOAD_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  Load (replace) M onto the hardware matrix stack.
   */

  DEFCONTEXT(ctx) ;
  DPRINT ("\n(xglTDM_LOAD_MATRIX\n") ; MPRINT(M) ; 

  xgl_transform_write (MODEL_VIEW_TRANSFORM, (void *)M) ;

  DPRINT(")") ;
}

void
_dxf_PUSH_MULTIPLY_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  Pushing and multiplying are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */
  
  DEFCONTEXT(ctx) ;
  DPRINT ("\n(xglTDM_PUSH_MULTIPLY_MATRIX\n") ; MPRINT(M) ;

  stackDepth++;
  DPRINT1("\nStack Depth = %d", stackDepth);

  xgl_context_update_model_trans (XGLCTX, XGL_MTR_PUSH|XGL_MTR_GLOBAL_TRANS) ;
  xgl_transform_write (TMP_TRANSFORM, (void *)M) ;
  xgl_transform_multiply (MODEL_VIEW_TRANSFORM,
			  TMP_TRANSFORM, MODEL_VIEW_TRANSFORM) ;

  DPRINT(")") ;
}

void
_dxf_PUSH_REPLACE_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  Pushing and loading are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */

  DEFCONTEXT(ctx) ;
  DPRINT ("\n(xglTDM_PUSH_REPLACE_MATRIX\n") ; MPRINT(M) ; 

  stackDepth++;
  DPRINT1("\nStack Depth = %d", stackDepth);

  xgl_context_update_model_trans (XGLCTX, XGL_MTR_PUSH|XGL_MTR_GLOBAL_TRANS) ;
  xgl_transform_write (MODEL_VIEW_TRANSFORM, (void *)M) ;

  DPRINT(")") ;
}

void
_dxf_POP_MATRIX (void *ctx)
{
  /*
   *  Pop the hardware matrix stack.
   */

  DEFCONTEXT(ctx) ;
  DPRINT ("\n(xglTDM_POP_MATRIX") ;

  stackDepth--;
  DPRINT1("\nStack Depth = %d", stackDepth);

  xgl_context_update_model_trans (XGLCTX, XGL_MTR_POP|XGL_MTR_GLOBAL_TRANS) ;

  DPRINT(")") ;
}

void
_dxf_SET_MATERIAL_SPECULAR (void *ctx, float r, float g, float b, 
			    float exponent)
{
  /*
   *  Set specular reflection coefficients for red, green, and blue; set
   *  specular exponent (shininess) from exponent.
   */

  Xgl_color spec ;
  DEFCONTEXT(ctx) ;
  DPRINT("\n(xglTDM_SET_MATERIAL_SPECULAR") ;

  spec.rgb.r = r ;
  spec.rgb.g = g ;
  spec.rgb.b = b ;

  /* fudge the specular exponent to match software renderer better */
  exponent = pow(exponent,1./XGLTRANSLATION->gamma) ;

  xgl_object_set (XGLCTX, XGL_3D_CTX_SURF_FRONT_SPECULAR_COLOR, &spec, 0) ;
  xgl_object_set (XGLCTX, XGL_3D_CTX_SURF_FRONT_SPECULAR_POWER, exponent, 0) ;
  DPRINT(")") ;
}

void
_dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (void *ctx, int on)
{
  /*
   *  Set global lighting attributes.  Some API's, like IBM GL, can only
   *  set certain lighting attributes such as attenuation globally across
   *  all defined lights; these API's should do the appropriate
   *  initialization through this call.  If `on' is FALSE, turn lighting
   *  completely off.
   */

  DEFCONTEXT(ctx) ;
  DPRINT("\n(xglTDM_SET_GLOBAL_LIGHT_ATTRIBUTES") ;

  xgl_object_set
      (XGLCTX,
       XGL_CTX_SURF_FRONT_COLOR_SELECTOR, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
       XGL_3D_CTX_SURF_FRONT_LIGHT_COMPONENT, XGL_LIGHT_ENABLE_COMP_AMBIENT |
		                              XGL_LIGHT_ENABLE_COMP_DIFFUSE |
		                              XGL_LIGHT_ENABLE_COMP_SPECULAR,
       NULL) ;

  if (!on)
    {
      int i ;
      Xgl_boolean light_switches[MAX_LIGHTS] ;

      xgl_object_get (XGLCTX, XGL_3D_CTX_LIGHT_SWITCHES, light_switches) ;
      
      for (i=0 ; i<MAX_LIGHTS ; i++)
	  light_switches[i] = 0 ;
      
      xgl_object_set
	  (XGLCTX,
	   XGL_3D_CTX_LIGHT_SWITCHES, light_switches,
	   NULL) ;
    }

  DPRINT(")") ;
}


void *
_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height)
{
  /*
   *  DXAllocate storage for a pixel array.
   */

  DEFCONTEXT(ctx) ;
  void *foo ;
  int nbytes;
#if defined(PRIVATE_PIXELS)
  Xgl_usgn32 *private_pix = NULL;
#endif
  DPRINT("\n(xglTDM_ALLOCATE_PIXEL_ARRAY") ;


#if defined(PRIVATE_PIXELS)
  nbytes = (width+1) * (height) * (NUM_PLANES < 24 ? 1 : 4);
  private_pix = (Xgl_usgn32 *)DXAllocate(nbytes);
  if (!private_pix) {
      DPRINT ( ")" ) ;
      return NULL;
  }
#endif

  /*  Called Sun Software about bug.  xgl_object_create fails, but
   *  doesn't return a NULL (as documented).
   *  Ref ID #1239569   Bug ID #1123335  3/18/93
   */
#ifdef XGL_ALLOC_BUG
  error_flag = 0 ;
  xgl_object_set (sys_st,
        XGL_SYS_ST_ERROR_NOTIFICATION_FUNCTION, _tdmEnufMemory, NULL) ;

  foo = (void *) xgl_object_create (sys_st, 
			  XGL_MEM_RAS, 0,
                          XGL_DEV_COLOR_TYPE, XGL_COLOR_RGB,
#if defined(PRIVATE_PIXELS)
			  XGL_MEM_RAS_IMAGE_BUFFER_ADDR, private_pix,
			  XGL_RAS_SOURCE_BUFFER, XGL_BUFFER_SEL_DRAW,
#endif
                          XGL_RAS_DEPTH, (NUM_PLANES < 24? 8: 32),
                          XGL_RAS_WIDTH, width,
                          XGL_RAS_HEIGHT, height,
                          NULL) ;

  if (error_flag == 0 && foo) { 
    DPRINT ( ")" ) ;
    return (void *) foo ;
    }
  else {
    DPRINT ( "error)" ) ;
    DXSetError (ERROR_NO_MEMORY, "#13000") ;
    return NULL ;
    }
#else 	
   return (void *) xgl_object_create (sys_st, XGL_MEM_RAS, 0,
				     XGL_DEV_COLOR_TYPE, XGL_COLOR_RGB,
				     XGL_RAS_DEPTH, (NUM_PLANES < 24? 8: 32),
				     XGL_RAS_WIDTH, width,
				     XGL_RAS_HEIGHT, height,
				     NULL) ;
#endif
}

void 
_dxf_FREE_PIXEL_ARRAY (void *ctx, void *pixels)
{
    Xgl_usgn32 *pixarray;

    /*
     *  Free pixel array storage.
     */
    
    DPRINT("\n(xglTDM_FREE_PIXEL_ARRAY") ;
    if (pixels) {

#if defined(PRIVATE_PIXELS)
	/* get address of buffer associated with portctx and zero it */
	xgl_object_get((Xgl_ras)pixels, 
		       XGL_MEM_RAS_IMAGE_BUFFER_ADDR, &pixarray);
	DXFree((Pointer)pixarray);

	xgl_object_set((Xgl_ras)pixels, 
			XGL_MEM_RAS_IMAGE_BUFFER_ADDR, NULL, 
		        NULL);
#endif

	xgl_object_destroy((Xgl_ras)pixels) ;
    }
    DPRINT(")") ;
}

typedef struct argbS
{
   float a, b, g, r;
} argbT,*argbP;

Error
_dxf_DRAW_IMAGE(void* win, dxObject image, hwTranslationP dummy)
{
 DEFWINDATA(win) ;
 DEFPORT(PORT_HANDLE) ;
 int           x,y,camw,camh ;
 int 		i, n ;
 unsigned long *pixels = NULL;
 void * translation = XGLTRANSLATION;
 DPRINT ("\n(_dxf_DRAW_IMAGE") ;

 if(!DXGetImageBounds(image,&x,&y,&camw,&camh))
    goto error;

  if(!(pixels = tdmAllocateZero(sizeof(unsigned long) * camw * camh)))
    goto error;

  if(!_dxf_dither(image,camw,camh,x,y,translation,pixels))
    goto error;

  n = camw * camh * sizeof(argbT) ;
  if (n != SW_BUF_SIZE) {
      if (SW_BUF)
        tdmFree(SW_BUF) ;
      SW_BUF = (void *) NULL;
      SW_BUF = (void *) tdmAllocateZero(n);
      if (!SW_BUF) {
        SW_BUF_SIZE = 0;
        goto error ;
        }
      else
        SW_BUF_SIZE = n;
      }

  for(i=0;i<camw*camh;i++) {
      ((argbP) SW_BUF)[i].r = (float) ((pixels[i]&0xFF)/255.0) ;
      ((argbP) SW_BUF)[i].g = (float)(((pixels[i] >> 8)&0xFF)/255.0) ;
      ((argbP) SW_BUF)[i].b = (float)(((pixels[i] >> 16)&0xFF)/255.0) ;
  }

  SW_BUF_CURRENT = TRUE ;

  /* Check for error */
  _dxf_WRITE_PIXEL_RECT(win,NULL,0,0,camw,camh);

  if (pixels) {
	tdmFree(pixels) ;
  	pixels = NULL ;
	}
  DPRINT (")") ;
  return OK ;

error:
  if(pixels) {
    tdmFree(pixels);
    pixels = NULL;
  }

  if (SW_BUF) {
    tdmFree(SW_BUF) ;
    SW_BUF = NULL ;
    SW_BUF_SIZE = 0 ;
    }

  DPRINT (")") ;
  return ERROR ;
}

static hwTranslationP
_dxf_CREATE_HW_TRANSLATION(void *win)
{
  DEFWINDATA(win) ;
  hwTranslationP        ret;
  int           i;

  if (! (ret = (hwTranslationP) tdmAllocate(sizeof(translationT))))
    return NULL;

  ret->dpy = DPY;
  ret->visual = NULL;
  ret->cmap = (void *)CLRMAP;
  ret->depth = 24;
  ret->invertY = TRUE;

# if defined(sun4)
     ret->gamma = 2.0;
# else
    ret->gamma = 1.0;
# endif

  ret->translationType = ThreeMap;
  ret->redRange = 256;
  ret->greenRange = 256;
  ret->blueRange = 256;
  ret->usefulBits = 8;
  ret->rtable = &ret->data[0];
  ret->gtable = &ret->data[256];
  ret->btable = &ret->data[256*2];
  ret->table = NULL;

  for(i=0;i<256;i++) {
    ret->rtable[i] = i;
    ret->gtable[i] = i << 8;
    ret->btable[i] = i << 16;
  }
  return ret;
}


static void
_dxf_WRITE_PIXEL_RECT(void *win, uint32 *foo, int x, int y,
        int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  int i ;
  void *image ;
  Xgl_pt_i2d pos ;
  Xgl_color color ;
  DPRINT("\n(_dxf_WRITE_PIXEL_RECT") ;

  DPRINT1("\n camera width: %d,", camw) ;
  DPRINT1(" camera height: %d", camh) ;

  if(!(image = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, camw, camh)))
     goto error ;

  /* set pixel array as rendering surface for XGL context */
  xgl_object_set (XGLTMPCTX, XGL_CTX_DEVICE, image, NULL) ;
 
  for (pos.y=0 ; pos.y<(camh) ; pos.y++) {
      for (pos.x=0 ; pos.x<camw ; pos.x++) {
        i = (pos.y*camw) + pos.x ;
        color.rgb.r = (float) ((argbP)SW_BUF)[i].r ;
        color.rgb.g = (float) ((argbP)SW_BUF)[i].g ;
        color.rgb.b = (float) ((argbP)SW_BUF)[i].b ;
        xgl_context_set_pixel (XGLTMPCTX, &pos, &color) ;
        }
   }

   _dxf_WRITE_BUFFER (PORT_CTX, 0, 0, camw-1, camh-1, image) ;

  /* ensure that draw buffer rendering is completed */
#ifndef solaris
  xgl_context_post (XGLTMPCTX, 0) ;
#else
  xgl_context_flush (XGLTMPCTX, XGL_FLUSH_BUFFERS) ;
#endif

  if (image)
        _dxf_FREE_PIXEL_ARRAY (PORT_CTX, image) ;

  DPRINT(")") ;
  return /* OK */;

error:
  if (image)
        _dxf_FREE_PIXEL_ARRAY (PORT_CTX, image) ;
  DPRINT(")") ;
  return /* 0 */;
}


static hwFlagsT servicesFlags = (SF_VOLUME_BOUNDARY | SF_TMESH | SF_QMESH |
				 SF_POLYLINES);


int
_dxf_DEFINE_LIGHT (void *win, int n, Light light)

{
  /*
   *  Define and switch on light `id' using the attributes specified by
   *  the DX `light' parameter.  If `light' is NULL, turn off light `id'.
   *  If more lights are defined than supported by the API, the extra
   *  lights are ignored.
   *
   *  Note that XGL does not transform lights by the current
   *  transformation matrix as GL does, and requires that the lights be
   *  expressed in world coordinates.  Since we have loaded only the
   *  projection component of the view into the context's view matrix, we
   *  must convert the DX world coordinates into view coordinates
   *  ourselves.
   */
  
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  float view[4][4], x, y, z ;
  Xgl_light xgl_lights[MAX_LIGHTS] ;
  Xgl_boolean xgl_light_switches[MAX_LIGHTS] ;
  Xgl_pt_f3d xgl_direction, xgl_position ;
  Xgl_color xgl_light_color ;
  RGBColor	color;
  Vector	direction;
  float		iGamma = 1.0;
  DPRINT1 ("\n(xglTDM_DEFINE_LIGHT:%d", n) ; 
  
  if (n > MAX_LIGHTS-1)
    {
      /* Only first %d lights are used */
      DXWarning("#5190", MAX_LIGHTS-1);
      goto error ;
    }
  
  xgl_object_get (XGLCTX, XGL_3D_CTX_LIGHTS, xgl_lights) ;
  xgl_object_get (XGLCTX, XGL_3D_CTX_LIGHT_SWITCHES, xgl_light_switches) ;
  
  if (! light)
      /* turn off light n */
      xgl_light_switches[n] = 0 ;
  else if (DXQueryDistantLight(light,&direction,&color))
    {
      DPRINT("\ndirectional\n") ; SPRINT(direction) ;
      /* NB: XGL direction is direction of light propagation! */

      DPRINT("\nworld-relative") ;

      if (n == 0)
	{
	  /* 0 is the ambient light */
	  DXErrorGoto(ERROR_INTERNAL, "#13740");
	}
      
      xgl_light_switches[n] = 1 ;

      _dxfGetCompositeMatrix (MATRIX_STACK, view) ;
      xgl_direction.x = -(direction.x*view[0][0] +
			  direction.y*view[1][0] +
			  direction.z*view[2][0]) ;
      xgl_direction.y = -(direction.x*view[0][1] +
			  direction.y*view[1][1] +
			  direction.z*view[2][1]) ;
      xgl_direction.z = -(direction.x*view[0][2] +
			  direction.y*view[1][2] +
			  direction.z*view[2][2]) ;
	  
      xgl_light_color.rgb.r = color.r ;
      xgl_light_color.rgb.g = color.g ;
      xgl_light_color.rgb.b = color.b ;

      DPRINT1 ("\nXGL light propagation direction: %f", xgl_direction.x) ;
      DPRINT1 (" %f", xgl_direction.y) ; DPRINT1 (" %f", xgl_direction.z) ;
      DPRINT("\ncolor\n") ; CPRINT(color) ;
	  
      xgl_object_set (xgl_lights[n],
		      XGL_LIGHT_TYPE, XGL_LIGHT_DIRECTIONAL,
		      XGL_LIGHT_COLOR, &xgl_light_color,
		      XGL_LIGHT_DIRECTION, &xgl_direction,
		      NULL) ;
    }  else if (DXQueryCameraDistantLight(light,&direction,&color)) {

      DPRINT("\ncamera-relative") ;


      if (n == 0)
	{
	  /* 0 is the ambient light */
	  DXErrorGoto(ERROR_INTERNAL, "#13740");
	}
      
      xgl_light_switches[n] = 1 ;

      xgl_direction.x = -direction.x ;
      xgl_direction.y = -direction.y ;
      xgl_direction.z = -direction.z ;

      xgl_light_color.rgb.r = color.r ;
      xgl_light_color.rgb.g = color.g ;
      xgl_light_color.rgb.b = color.b ;
	  
      DPRINT1 ("\nXGL light propagation direction: %f", xgl_direction.x) ;
      DPRINT1 (" %f", xgl_direction.y) ; DPRINT1 (" %f", xgl_direction.z) ;
      DPRINT("\ncolor\n") ; CPRINT(color) ;
	  
      xgl_object_set (xgl_lights[n],
		      XGL_LIGHT_TYPE, XGL_LIGHT_DIRECTIONAL,
		      XGL_LIGHT_COLOR, &xgl_light_color,
		      XGL_LIGHT_DIRECTION, &xgl_direction,
		      NULL) ;
    } else if (DXQueryAmbientLight(light,&color)) {
	  
      DPRINT("\nambient") ; 

      xgl_light_switches[n] = 1 ;

      iGamma = 1./XGLTRANSLATION->gamma;
      xgl_light_color.rgb.r = (float)pow(color.r,iGamma) ;
      xgl_light_color.rgb.g = (float)pow(color.g,iGamma) ;
      xgl_light_color.rgb.b = (float)pow(color.b,iGamma) ;
      DPRINT("\ncolor\n") ; CPRINT(color) ;
	  
      xgl_object_set (xgl_lights[n],
		      XGL_LIGHT_TYPE, XGL_LIGHT_AMBIENT,
		      XGL_LIGHT_COLOR, &xgl_light_color,
		      NULL) ;
    } else {
      DPRINT("\nunknown light type") ;
      DXErrorGoto(ERROR_DATA_INVALID, "#13751") ;
    }

  xgl_object_set (XGLCTX,
		  XGL_3D_CTX_LIGHT_SWITCHES, xgl_light_switches,
		  NULL) ;

  DPRINT(")") ;
  return TRUE ;

 error:
  DPRINT("\nerror)") ;
  return FALSE ;
}

static int
_dxf_ADD_CLIP_PLANES (void* win, Point *pt, Vector *normal, int count)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE) ;
  Xgl_usgn32	planeCount;
  Xgl_plane	planes[MAX_CLIP_PLANES];
  float		view[4][4];
  double	dview[4][4],datview[4][4];
  int		i;

  planeCount = CLIP_PLANE_CNT;
  CLIP_PLANE_CNT += count;

  if (CLIP_PLANE_CNT > MAX_CLIP_PLANES)
    {
      /* too many clip planes specified, clip object ignored */
      DXWarning("#13890");
      return OK;
    }
  
  xgl_object_get(XGLCTX, XGL_3D_CTX_MODEL_CLIP_PLANES, planes);

  /*
   * pt and normal are in world coordinates. Because the view matrix is
   * in the PROJECTION_MATRIX for xgl, this implies that we have view
   * coordinates for clip planes (not world as the document says).
   *
   * So we need to apply the inverse of the clipping transform to the 
   * pt and normal to correct for this.
   */
  _dxfGetViewMatrix(MATRIX_STACK,view);
  COPYMATRIX(dview,view);
  _dxfAdjointTranspose(datview,dview);

  for(i=0;i < count; i++) {
    planes[planeCount].pt.x     = pt[i].x;
    planes[planeCount].pt.y     = pt[i].y;
    planes[planeCount].pt.z     = pt[i].z;
    
    planes[planeCount].normal.x = normal[i].x;
    planes[planeCount].normal.y = normal[i].y;
    planes[planeCount].normal.z = normal[i].z;
    
    APPLY3((float *)&planes[planeCount].pt,dview);
    APPLY3((float *)&planes[planeCount].normal,datview);

    planeCount++;
  }
  
  xgl_object_set(XGLCTX, XGL_3D_CTX_MODEL_CLIP_PLANE_NUM, planeCount, 0);
  xgl_object_set(XGLCTX, XGL_3D_CTX_MODEL_CLIP_PLANES, planes, 0);

  return OK;

 error:

  return ERROR;

}

static int
_dxf_REMOVE_CLIP_PLANES (void* win, int count)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE) ;
  Xgl_usgn32	planeCount;

  
  if (CLIP_PLANE_CNT > MAX_CLIP_PLANES) {
    CLIP_PLANE_CNT -= count;
    return OK;
  }

  planeCount = CLIP_PLANE_CNT;
  CLIP_PLANE_CNT -= count;

  if (planeCount < count)
    {
      /* Too many clip planes deleted */
      DXErrorGoto(ERROR_INTERNAL,"#13900");
    }
  
  planeCount -= count;
  xgl_object_set(XGLCTX, XGL_3D_CTX_MODEL_CLIP_PLANE_NUM, planeCount, 0);

  return OK;

 error:

  return ERROR;

}

static int _dxf_GET_VERSION(char ** dsoStr)
{
   static char * DXHW_LIB_FLAVOR = "XGL";

   ENTRY(("_dxf_GET_VERSION(void)"));
   if(dsoStr)
      if(*dsoStr)
         *dsoStr = DXHW_LIB_FLAVOR;
   return(DXHW_DD_VERSION);
   EXIT(("OK"));
}

Error _dxf_DrawTrans(void *globals, RGBColor ambientColor, int buttonUp)
{
   DEFGLOBALDATA(globals);
   DEFPORT(PORT_HANDLE);
   SortListP     sl = (SortListP)SORTLIST;
   SortD         sorted = sl->sortList;
   int           nSorted = sl->itemCount;
   int 		 i, j;

   for (i = 0; i < nSorted; i = j)
       for (j = i; j < nSorted && sorted[j].xfp == sorted[i].xfp; j++)
	   _dxf_DrawOpaque(PORT_HANDLE, (xfieldP)sorted[i].xfp, (j-i), 
					ambientColor, buttonUp);
    
   return OK;
}

extern Error _dxf_DrawOpaque (tdmPortHandleP portHandle, xfieldP xf,
			      RGBColor ambientColor, int buttonUp);

static Error _dxf_StartFrame(void *win)
{
    return OK;
}

static Error _dxf_EndFrame(void *win)
{
    return OK;
}


static void
_dxf_ClearBuffer(void *win)
{
    DEFWINDATA(win) ;
    DEFPORT(PORT_HANDLE) ;
    xgl_context_new_frame(XGLCTX) ;
}

static tdmDrawPortT _DrawPort = 
{
  /* AllocatePixelArray */       _dxf_ALLOCATE_PIXEL_ARRAY,          
  /* AddClipPlane */             _dxf_ADD_CLIP_PLANES,
  /* ClearArea */ 	       	 _dxf_CLEAR_AREA,  
  /* CreateHwTranslation */	 _dxf_CREATE_HW_TRANSLATION,                  
  /* CreateWindow */	       	 _dxf_CREATE_WINDOW,                 
  /* DefineLight */ 	       	 _dxf_DEFINE_LIGHT,                  
  /* DestroyWindow */ 	       	 _dxf_DESTROY_WINDOW,                
  /* DrawClip */		 NULL,        
  /* DrawImage */		 _dxf_DRAW_IMAGE,			
  /* Drawopaque */		 _dxf_DrawOpaque,                           
  /* RemoveClipPlane */          _dxf_REMOVE_CLIP_PLANES,
  /* FreePixelArray */ 	       	 _dxf_FREE_PIXEL_ARRAY,              
  /* InitRenderModule */         _dxf_INIT_RENDER_MODULE,            
  /* InitRenderPass */ 	       	 _dxf_INIT_RENDER_PASS,              
  /* LoadMatrix */ 	       	 _dxf_LOAD_MATRIX,                   
  /* MakeProjectionMatrix */     _dxf_MAKE_PROJECTION_MATRIX,        
  /* PopMatrix */ 	       	 _dxf_POP_MATRIX,                    
  /* PushMultiplyMatrix */       _dxf_PUSH_MULTIPLY_MATRIX,          
  /* PushReplaceMatrix */        _dxf_PUSH_REPLACE_MATRIX,           
  /* ReadApproxBackstore */      _dxf_READ_APPROX_BACKSTORE,         
  /* ReplaceViewMatrix */        _dxf_REPLACE_VIEW_MATRIX,           
  /* SetBackgroundColor */       _dxf_SET_BACKGROUND_COLOR,          
  /* SetDoubleBufferMode */    	 _dxf_DOUBLE_BUFFER_MODE,        
  /* SetGlobalLightAttributes */ _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES,  
  /* SetMaterialSpecular */      _dxf_SET_MATERIAL_SPECULAR,         
  /* SetOrthoProjection */       _dxf_SET_ORTHO_PROJECTION,          
  /* SetOutputWindow */ 	 _dxf_SET_OUTPUT_WINDOW,                    
  /* SetPerspectiveProjection */ _dxf_SET_PERSPECTIVE_PROJECTION,    
  /* SetSingleBufferMode */    	 _dxf_SINGLE_BUFFER_MODE,        
  /* SetViewport */ 	       	 _dxf_SET_VIEWPORT,                  
  /* SetWindowSize */ 	       	 _dxf_SET_WINDOW_SIZE,               
  /* SwapBuffers */ 	       	 _dxf_SWAP_BUFFERS,                  
  /* WriteApproxBackstore */     _dxf_WRITE_APPROX_BACKSTORE, 
  /* WritePixelRect */		 _dxf_WRITE_PIXEL_RECT,			 

  /* End of functions available to 2.0 and 2.0.1 DX release */
 
  /* DrawTransparent */          _dxf_DrawTrans,
  /* GetMatrix */                NULL,
  /* GetProjection */            NULL,
  /* GetVersion */              _dxf_GET_VERSION,
  /* ReadImage */                NULL,
  /* StartFrame */               _dxf_StartFrame,
  /* EndFrame */                 _dxf_EndFrame,
  /* Clear Buffer */		 _dxf_ClearBuffer
};

#ifndef STANDALONE

extern tdmInteractorEchoT _dxd_hwInteractorEchoPort;

void
_dxfUninitPortHandle(tdmPortHandleP portHandle)
{
  if(portHandle) {
    portHandle->portFuncs = NULL;
    portHandle->echoFuncs = NULL; 
    if(_num_windows_open == 0 &&
       sys_st != NULL)
      xgl_close(sys_st);
  /* 
   *  Note: portHandle->private is
   *  freed in DESTROY_WINDOW as PORT_CTX  
   */ 
   tdmFree(portHandle); 
  }
}

tdmPortHandleP
_dxfInitPortHandle(Display *dpy, char *hostname)
{
  /*
   *  Return 1 if hardware graphics rendering is not available.
   */

  int   		len=MAXHOSTNAMELEN-1 ;
  char  		*displayString ;
  unsigned int  	displaysize ;
  Window 		xid ;
  Visual 		*vis ;
  XSetWindowAttributes 	attrs ;
  int screen, 		depth ;
  Xgl_usgn32		num_buffers ;
  int w, h ;
  Xgl_win_ras 	      	ras = 0 ;
  Xgl_X_window        	xgl_x_win ;
  Xgl_obj_desc        	obj_desc ;
  Xgl_inquire         	*inq_info = 0 ;
#ifdef solaris
  Xgl_sys_state	      	tmp_sys_st = NULL ;
#else
  Xgl_sys_st	      	tmp_sys_st = NULL ;
#endif

  int                 	proto = XGL_WIN_X_PROTO_DEFAULT ;
  tdmPortHandleP	ret;

  DPRINT("\n(xglTDM_GRAPHICS_NOT_AVAILABLE") ;

  _dxf_setFlags(_dxf_SERVICES_FLAGS(),
		SF_VOLUME_BOUNDARY | SF_TMESH | SF_QMESH | SF_POLYLINES);


#ifndef DEBUG
  {
    int 	execSymbolInterface;
    void*	(*symVer)();
    void*	(*expVer)();
    void*	DX_handle;
    
    /* If the hardware rendering load module requires a newer set of
     * DX exported symbols than those available in the calling dxexec
     */
    if (!(DX_handle=dlopen((const char *)0,RTLD_LAZY))) {
      DXSetError(ERROR_UNEXPECTED,dlerror());
      goto error;
    }

    if ((symVer = (void*(*)())dlsym(DX_handle, "DXSymbolInterface")) &&
	 (expVer = (void*(*)())dlsym(DX_handle, "_dxfExportHwddVersion"))) {
      printf("found DXSymbolInterface");
      (*symVer)(&execSymbolInterface);
      (*expVer)(DXD_HWDD_INTERFACE_VERSION);
    } else {
      execSymbolInterface = 0;
    }
    
    if(LOCAL_DXD_SYMBOL_INTERFACE_VERSION > execSymbolInterface) {
      DXSetError(ERROR_UNEXPECTED,"#13910", 
		 "hardware rendering", "DX Symbol Interface");
      goto error;
    }
  }

#endif
  /*
   *  Initialize XGL the first time a window is opened.  There is no
   *  "initialize API" port layer function; we don't get a chance to do
   *  anything until DX creates a hardware window.
   */

  if (_num_windows_open == 0)
      sys_st = xgl_open(NULL) ;
    if (sys_st == NULL) goto error ;


  /* Get all of the visual information about the screen: */
  screen = DefaultScreen(dpy) ;
  w = DisplayWidth(dpy, screen) - 1 ;
  h = DisplayHeight(dpy, screen) - 1 ;

  _tdmGetHardwareVisual (dpy, screen, &depth, &vis) ;
  if (depth == 0) {
    /* Hardware rendering is unavailable on host '%s' */
    DXSetError (ERROR_BAD_PARAMETER,"#13675", hostname) ;
    goto error ;
  }

  /* Create Colormap and Window */
  attrs.border_pixel = None ;
  attrs.background_pixel = None ;
  attrs.colormap = XCreateColormap (dpy, DefaultRootWindow(dpy),
                                    vis, AllocNone) ;
  if (! (xid = XCreateWindow (dpy, DefaultRootWindow(dpy),
                              0, 0, w, h, 0, depth, InputOutput, vis,
                              CWBackPixel | CWBorderPixel | CWColormap,
                              &attrs))) {
    /* Hardware rendering is unavailable on host '%s' */
    DXSetError (ERROR_BAD_PARAMETER,"#13675", hostname) ;
    goto error ;
  }

  /*  initialize XGL window objects */
  xgl_x_win.X_display = (void *) dpy ;
  xgl_x_win.X_window = (Xgl_usgn32) xid ;
  xgl_x_win.X_screen = (int) screen ;

  obj_desc.win_ras.type = XGL_WIN_X | proto;
  obj_desc.win_ras.desc = &xgl_x_win;
 
#ifdef solaris
  inq_info = (Xgl_inquire *) xgl_inquire (sys_st, &obj_desc) ;
#else
  inq_info = (Xgl_inquire *) xgl_inquire (&obj_desc) ;
#endif
  if (!inq_info) {
        /* Hardware rendering is unavailable on host '%s' */
        DXSetError (ERROR_BAD_PARAMETER,"#13675", hostname) ;
        goto error ;
        }

  /* create raster from X window, request double buffers and DXRGB colors */
  obj_desc.win_ras.type = XGL_WIN_X ;

  /*  Called Sun Software about bug.  xgl_object_create fails, but
   *  doesn't return a NULL (as documented).
   *  Ref ID #1239569   Bug ID #1123335  3/18/93
   */
#ifdef XGL_ALLOC_BUG
  error_flag = 0 ;
  xgl_object_set (sys_st,
        XGL_SYS_ST_ERROR_NOTIFICATION_FUNCTION, _tdmEnufMemory, NULL) ;
#endif
  ras = xgl_object_create(sys_st, XGL_WIN_RAS, 
			  &obj_desc,
			  XGL_DEV_COLOR_TYPE, XGL_COLOR_RGB, 
			  XGL_WIN_RAS_BUFFERS_REQUESTED, 2,
			  NULL) ; 
 
#ifdef XGL_ALLOC_BUG
  if (error_flag != 0 || !ras)
#else
  if (!ras)
#endif
    {
      /* Hardware rendering is unavailable on host '%s' */
      DXSetError (ERROR_BAD_PARAMETER,"#13675", hostname) ;
      goto error ;
    }

  /* check result of double buffer request */
  xgl_object_get
      (ras, XGL_WIN_RAS_BUFFERS_ALLOCATED, &num_buffers) ;

  DPRINT1 ("\nrequested 2 buffers, got %d", num_buffers) ;

  if (num_buffers < 2) {
        /* Hardware rendering is unavailable on host '%s' */
        DPRINT("\nhardware double buffering not available") ;
        DXSetError (ERROR_BAD_PARAMETER,"#13675", hostname) ;
        goto error ;
     	}

   /* clean up */
  if (ras) {
    if (inq_info) DXFree ((Xgl_inquire *) inq_info) ; 
    xgl_object_destroy(ras) ;
  }

  ret = (tdmPortHandleP)tdmAllocateLocal(sizeof(tdmPortHandleT));

  if(ret) {
    ret->portFuncs = &_DrawPort;
    ret->echoFuncs = &_dxd_hwInteractorEchoPort;
    ret->uninitP   = _dxfUninitPortHandle;
    ret->private   = NULL;
  }

  DPRINT (")") ;
  return ret ;

 error:
  DPRINT("\nerror)") ;
  if (sys_st) {
    if (inq_info) tdmFree ((char*) inq_info) ;
    if (ras) xgl_object_destroy(ras) ;
    xgl_close(sys_st) ;
  }
  return NULL ;

}
#endif
 
 
