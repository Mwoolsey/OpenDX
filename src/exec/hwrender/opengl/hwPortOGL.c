/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/opengl/hwPortOGL.c,v $
\*---------------------------------------------------------------------------*/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#define STRUCTURES_ONLY
#include "../hwDeclarations.h"
#include "../hwWindow.h"
#include "../hwMemory.h"
#include "../hwMatrix.h"
#include "../hwPortLayer.h"
#include "../hwObjectHash.h"
#include "../hwDebug.h"

#include "hwPortOGL.h"

#if defined(DX_NATIVE_WINDOWS)
#include <windows.h>
#endif

/* deal with DX/GL namespace collisions */
#define Object dxObject
#define Matrix dxMatrix
#include "../../libdx/internals.h"
#undef Object
#undef Matrix
#include <math.h>
#include <stdlib.h>    /* for getenv prototype */

#if defined(HAVE_X11_XLIBXTRA_H)
#include <x11/xlibxtra.h>
#endif

extern Error _dxf_dither(dxObject, int, int, int, 
	int, void *, unsigned char *);
extern Error _dxfExportHwddVersion(int version);

#if defined (alphax) || defined(sgi) || defined(solaris)
#  if !defined (DEBUG)
#     include <dlfcn.h>
#  endif
#endif

#define DXHW_DD_VERSION 0x080000

/*
 *  Next three functions are used for keeping track of all GLX contexts in
 *  use.  We need to do this in order to share display lists among
 *  contexts.
 */

static float WS[4][4] = {
  { 1,  0,  0,  0},
  { 0,  1,  0,  0},
  { 0,  0,  1,  0},
  {-1, -1,  0,  1}
};

#if !defined(DX_NATIVE_WINDOWS)

static int nCtx = 0 ;
static int ctxListSize = 0 ;
static GLXContext *ctxList = 0 ;
static int fontListBase = 0 ;

static GLXContext getGLXContext (void)
{
  int i ;

  ENTRY(("getGLXContext()"));

  for (i=0 ; i<ctxListSize ; i++) {
    if (ctxList[i]) {
      EXIT(("ctxList[i] = 0x%x;",ctxList[i]));
      return ctxList[i] ;
    }
  }

  EXIT(("context not found;"));
  return 0 ;
}

static Error remGLXContext (GLXContext ctx)
{
  int i ;

  ENTRY(("remGLXContext(0x%x)",ctx));

  for (i=0 ; i<ctxListSize ; i++)
    if (ctxList[i] == ctx) {
      ctxList[i] = 0 ;
      nCtx-- ;
      if (nCtx) {
	EXIT(("OK;"));
	return OK ;
      }
	
      /* clean up context list */
      tdmFree(ctxList) ;
      nCtx = 0 ;
      ctxListSize = 0 ;
      ctxList = 0 ;

      /* delete font display lists */
      if (glIsList(fontListBase))
	glDeleteLists (fontListBase, 256) ;
	
      EXIT(("OK;"));
      return OK ;
    }

  EXIT(("ERROR;"));
  return ERROR ;
}


static Error putGLXContext (GLXContext ctx)
{
#define INITSIZE 10
  int i ;

  ENTRY(("putGLXContext(0x%x)",ctx));

  if (nCtx == ctxListSize) {
    GLXContext *newCtxList ;

    if(!(newCtxList=tdmAllocate((ctxListSize+INITSIZE)*sizeof(GLXContext)))) {
      EXIT(("Alloc failed;"));
      return ERROR ;
    }

    for (i=0 ; i<ctxListSize ; i++)
      newCtxList[i] = ctxList[i] ;

    if (ctxList)
      tdmFree(ctxList) ;

    ctxList = newCtxList ;

    ctxList[ctxListSize] = ctx ;
    for (i=ctxListSize+1 ; i<ctxListSize+INITSIZE ; i++)
      ctxList[i] = 0 ;

    ctxListSize += INITSIZE ;
  } else {
    for (i=0 ; i<ctxListSize ; i++)
      if (!ctxList[i]) {
	ctxList[i] = ctx ;
	break ;
      }
  }
  nCtx++ ;

  EXIT(("OK;"));
  return OK ;
}

/*
 *  This silly static variable is used to keep track of the X display
 *  connection used by the current GLX context.  We need it since OpenGL
 *  won't allow us to switch GLX contexts across different display
 *  connections without explicitly releasing the current one.  This isn't
 *  required if all the GLX contexts use the same display.  We think it's a
 *  bug and the issue is being brought before the ARB. (4/7/94)
 */
static Display *CurrentDpy = 0 ;
#endif

/*
 *  PORT LAYER SECTION
 */
static translationO
_dxf_CREATE_HW_TRANSLATION (void *win)
{
#if defined(DX_NATIVE_WINDOWS)
  return NULL;
#else
  int i ;
  DEFWINDATA(win) ;
  hwTranslationP ret ;
  char *gammaStr;

  ENTRY(("_dxf_CREATE_HW_TRANSLATION (0x%x)",win));

  if (! (ret = (hwTranslationP) tdmAllocate(sizeof(translationT))))
    {
      EXIT(("ERROR: Alloc failed ret = 0"));
      return 0 ;
    }

  ret->dpy = DPY ;
  ret->visual = NULL ;
  ret->cmap = CLRMAP ;
  ret->depth = 24 ;
  ret->invertY = FALSE ;
#if sgi
  ret->gamma = 1.0 ;
#else
  ret->gamma = 2.0 ;
#endif
  ret->translationType = ThreeMap ;
  ret->redRange = 256 ;
  ret->greenRange = 256 ;
  ret->blueRange = 256 ;
  ret->usefulBits = 8 ;
  ret->rtable = &ret->data[256*3] ;
  ret->gtable = &ret->data[256*2] ;
  ret->btable = &ret->data[256] ;
  ret->table = NULL ;

  for(i=0 ; i<256 ; i++) 
    {
      ret->rtable[i] = i << 24 ;
      ret->gtable[i] = i << 16 ;
      ret->btable[i] = i << 8 ;
    }

  if ((gammaStr=getenv("DXHWGAMMA")))
    {
      ret->gamma = atof(gammaStr);
      if (ret->gamma < 0) ret->gamma = 0.0;
      PRINT(("gamma environment var: ",atof(gammaStr)));
      PRINT(("gamma: ",ret->gamma));
    }

  EXIT(("ret = 0x%x",ret));
  return (translationO)ret ;
#endif
}

#if defined(DX_NATIVE_WINDOWS)

static void*
_dxf_CREATE_WINDOW (void *globals, char *winName, int w, int h)
{
	DEFGLOBALDATA(globals);
	OGLWindow *oglw = GetOGLWPtr(XWINID);
 	char * doGLStereo;
 
    /* Initialize the hwRender port private context */
    PORT_HANDLE->private = tdmAllocateZero(sizeof(tdmOGLctxT));
    if(! PORT_HANDLE->private)
	{
       DXErrorGoto (ERROR_INTERNAL, "#13000") ;
	}
    else
    {	
		int nMyPixelFormatID;
	    char *str;
		GLint mcp;
		int checkID;                     /* for verifying window */
		PIXELFORMATDESCRIPTOR checkPfd;  /*   capabilites        */
		static PIXELFORMATDESCRIPTOR pfd = 
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			24,
			0, 0, 0,
			0, 0, 0,
			0, 0, 
			0, 0, 0, 0, 0,
			16,
			0, 
			0, 
			PFD_MAIN_PLANE,
			0, 0, 0, 0
		};
	    DEFPORT(PORT_HANDLE) ;
    
		/* check if we should request stereo window */
		if ((doGLStereo = getenv("DX_USE_GL_STEREO")) != NULL)
		{
			if (atoi(doGLStereo))
			{
				pfd.dwFlags |= PFD_STEREO;
			}
		}
    
		OGLHDC	   = GetDC(XWINID);
		
		nMyPixelFormatID = ChoosePixelFormat(OGLHDC, &pfd);
		if (! nMyPixelFormatID)
			return NULL;

		SetPixelFormat(OGLHDC, nMyPixelFormatID, &pfd);
		OGLHRC = wglCreateContext(OGLHDC);

		/* initialize, then check if we got a stereo window */
		USEGLSTEREO = 0;
		checkID  = GetPixelFormat(OGLHDC);
		if (DescribePixelFormat(OGLHDC, checkID,
			sizeof(PIXELFORMATDESCRIPTOR), &checkPfd) &&
		    ((checkPfd.dwFlags & PFD_STEREO) != 0))
		{
			USEGLSTEREO = 1;
		}
		wglMakeCurrent(OGLHDC, OGLHRC); 

	    OGLXWIN    = XWINID;
        WINWIDTH   = w ;
        WINHEIGHT  = h ;
        XMAXSCREEN = 1024;
        YMAXSCREEN = 1024;
        OGLWINT    = _wdata;
        	        
		glGetIntegerv(GL_MAX_CLIP_PLANES, &mcp);
	    MAX_CLIP_PLANES = mcp;
 
		if (NULL != (str = getenv("DX_USE_DISPLAYLISTS")))
		{
			if (atoi(str))
			{
				_dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_USE_DISPLAYLISTS);
				DO_DISPLAY_LISTS = 1;
			}
			else
				DO_DISPLAY_LISTS = 1;
		}
		else
		{
			_dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_USE_DISPLAYLISTS);
			DO_DISPLAY_LISTS = 1;
		}
		DO_DISPLAY_LISTS = 0;

		glEnable(GL_NORMALIZE);
		wglMakeCurrent(NULL,NULL);
	
	}

skip:
	return (void *)&XWINID;

error:
	return NULL ;
}

#else

static void*
_dxf_CREATE_WINDOW (void *globals, char *winName, int w, int h)
{
  /*
   *  Create a new hardware graphics window with specified name and
   *  dimensions.  Return the X identifier associated with the window.
   */
  
  Window xid ;
  Visual *vis ;
  XVisualInfo *vi ;
  XWindowAttributes attr ;
  XSetWindowAttributes sattr ;
  int depth, screen ;
  GLXContext oglctx, oglctx_share ;
  DEFGLOBALDATA(globals) ;
  char *str;

/* The following code was changed to fix problems in intel and on Stylus
*/

/* index into glattr struct */
#define R_DEPTH     3
#define G_DEPTH     5
#define B_DEPTH     7
#define IMAGE_DEPTH 9

  static int glattr[] = {
    GLX_RGBA,               /* 0   */
    GLX_DOUBLEBUFFER,       /* 1   */
    GLX_RED_SIZE,   8,      /* 2,3 */
    GLX_GREEN_SIZE, 8,      /* 4,5 */
    GLX_BLUE_SIZE,  8,      /* 6,7 */
    GLX_DEPTH_SIZE, 23,     /* 8,9 */
    0                       /* 10  */
  };

  ENTRY(("_dxf_CREATE_WINDOW (0x%x, \"%s\", %d, %d)",
         globals, winName, w, h));
  if (! glXQueryExtension (DPY, 0, 0)) {
    PRINT (("GLX extension not supported by this server")) ;
    DXSetError(ERROR_NOT_IMPLEMENTED, "GLX extension not installed\n");
    goto error;
  }
  screen = DefaultScreen(DPY) ;
  if ((vi = glXChooseVisual (DPY, screen, glattr)) != NULL)
      goto got_visual;

  /* for pc hardware driver bug - relax the image depth requirement
   * and see what you get.  if it is >= 23, it's ok.
   */
#if defined(intelnt) || defined(WIN32)
  glattr[IMAGE_DEPTH] = 0;
#else
  glattr[IMAGE_DEPTH] = 1;
#endif
  if ((vi = glXChooseVisual (DPY, screen, glattr)) != NULL)
  {
      glXGetConfig(DPY, vi, GLX_DEPTH_SIZE, &depth);
      if (depth >= 16)
          goto got_visual;
      XFree(vi);
  }

  glattr[R_DEPTH] = 5;
  glattr[G_DEPTH] = 5;
  glattr[B_DEPTH] = 5;
  if ((vi = glXChooseVisual (DPY, screen, glattr)) != NULL)
  {
      glXGetConfig(DPY, vi, GLX_DEPTH_SIZE, &depth);
      if (depth >= 16)
          goto got_visual;
      XFree(vi);
  }
 
  glattr[R_DEPTH] = 4;
  glattr[G_DEPTH] = 4;
  glattr[B_DEPTH] = 4;
  if ((vi = glXChooseVisual (DPY, screen, glattr)) != NULL)
  {
      glXGetConfig(DPY, vi, GLX_DEPTH_SIZE, &depth);
      if (depth >= 16)
          goto got_visual;
      XFree(vi);
  }
 
  glattr[R_DEPTH] = 3;
  glattr[G_DEPTH] = 3;
  glattr[B_DEPTH] = 2;
  if ((vi = glXChooseVisual (DPY, screen, glattr)) != NULL)
  {
      glXGetConfig(DPY, vi, GLX_DEPTH_SIZE, &depth);
      if (depth >= 16)
          goto got_visual;
      XFree(vi);
  }

  glattr[IMAGE_DEPTH] = 0;
  if ((vi = glXChooseVisual (DPY, screen, glattr)) != NULL) {
      glXGetConfig(DPY, vi, GLX_DEPTH_SIZE, &depth);
      if (depth >= 16)
          goto got_visual;
      XFree(vi);
  }

  PRINT (("Unable to find acceptable X visual")) ;

  /* request any visual and tell the user what we got vs what we wanted */
  if ((vi = glXChooseVisual (DPY, screen, NULL)) != NULL) {
      DXWarning("Direct or True color window required");
      DXWarning("with double buffering, a depth of at least 23");
      DXWarning("and either 24, 12 or 8 bit colors");
      DXWarning("Default X window is class %d, depth %d, %d bit colors",
                vi->class, vi->depth, vi->bits_per_rgb);
      XFree(vi);
  }
  DXSetError(ERROR_NOT_IMPLEMENTED,
             "Unable to open an X window which can be used with OpenGL");
  goto error;

got_visual:

  PRINT (("visual depth = %d", vi->depth));
  PRINT (("visual class = %d", vi->class));
  PRINT (("visual bits per rgb = %d", vi->bits_per_rgb));
  PRINT (("visual colormap size = %d", vi->colormap_size));

  vis = vi->visual ;
  depth = vi->depth ;

  /* find an existing GLX context with which to share display lists */
  oglctx_share = getGLXContext() ;

  /* create a GLX direct rendering context */
  if (! (oglctx = glXCreateContext (DPY, vi, NULL, TRUE)))
  {
     if (! (oglctx = glXCreateContext (DPY, vi, NULL, FALSE)))
     {
        PRINT (("Unable to create GLX rendering context")) ;
        DXErrorGoto (ERROR_INTERNAL, "#13670") ;
     }
  }
  /* release visual info structure */
  XFree(vi) ;

  /* create colormap and X window */
  sattr.border_pixel = None ;
  sattr.background_pixel = None ;
  sattr.colormap = XCreateColormap (DPY, XRootWindow(DPY, screen), vis, AllocNone) ;

  if (! (XWINID = xid =
	 XCreateWindow (DPY, XRootWindow(DPY, screen),
			0, 0, w, h, 0, depth, InputOutput, vis,
			CWBackPixel | CWBorderPixel | CWColormap,
			&sattr))) {
    PRINT (("Could not create X window")) ;
    DXErrorGoto (ERROR_INTERNAL, "#13770") ;
  }
      
  PIXDEPTH = depth ;

  PRINT (("X Display: 0x%x", DPY)) ;
  PRINT (("X Window: 0x%x", xid)) ;
  PRINT (("GLX context: 0x%x", oglctx)) ;
  PRINT (("getenv('DISPLAY') = \"%s\"", getenv("DISPLAY")));

  /*
   *  Bind the GLX context to the X window.  If there is already a GLX
   *  context in use, we have to explicitly release it since DX uses
   *  separate display connections for each window.  We think this is a
   *  bug in OpenGL.
   */

  if(!getenv("IGNORE_GLXWAITX"))
  glXWaitX() ; 

 if(CurrentDpy)
    glXMakeCurrent (CurrentDpy, None, NULL) ;

  if (glXMakeCurrent (DPY, xid, oglctx)) {
    CurrentDpy = DPY ;
  } else {
    PRINT (("glXMakeCurrent() failed")) ;
    DXErrorGoto (ERROR_INTERNAL, "#13670") ;
  }

  /* put GLX context in list of existing contexts */
  if (! putGLXContext(oglctx)) {
    PRINT (("Unable to record GLX context"));
    DXErrorGoto (ERROR_INTERNAL, "#13000") ;
  }

  /* install the colormap for the UI to manage, if color is not static */
  if(!getenv("IGNORE_GLXWAITX"))
  glXWaitGL() ;
  XGetWindowAttributes (DPY, xid, &attr) ;
  CLRMAP = attr.colormap ;
  XInstallColormap (DPY, CLRMAP) ;

  /* if this is 1st GLX context in this process, create font display lists */
  if (! oglctx_share) {
    Font f ;
    if (! (fontListBase = glGenLists(256))) {
      PRINT (("Unable to allocate font display lists")) ;
      DXErrorGoto (ERROR_INTERNAL, "#13000") ;
    }

    f = XLoadFont (DPY, "9x15") ;
  if(!getenv("IGNORE_GLXWAITX"))
    glXUseXFont (f, 0, 256, fontListBase) ;
    XUnloadFont (DPY, f) ;
  }
  
  /* Initialize the hwRender port private context */
  if(! (PORT_HANDLE->private = tdmAllocateZero(sizeof(tdmOGLctxT)))) {
    PRINT (("out of memory allocating private context")) ;
    DXErrorGoto (ERROR_INTERNAL, "#13000") ;
  } else {
    DEFPORT(PORT_HANDLE) ;
    
    OGLDPY = DPY ;
    OGLSCREEN = screen ;
    OGLXWIN = xid ;
    OGLCTX = oglctx ;
    WINWIDTH = w ;
    WINHEIGHT = h ;
    XMAXSCREEN = DisplayWidth (DPY, screen) - 1 ;
    YMAXSCREEN = DisplayHeight (DPY, screen) - 1 ;
    OGLWINT = _wdata;
    FONTLISTBASE = fontListBase ;
    {
	GLint mcp;
	glGetIntegerv(GL_MAX_CLIP_PLANES, &mcp);
	MAX_CLIP_PLANES = mcp;
    }

    if (NULL != (str = getenv("DX_USE_DISPLAYLISTS")))
    {
	if (atoi(str))
	{
	    _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_USE_DISPLAYLISTS);
	    DO_DISPLAY_LISTS = 1;
	}
	else
	    DO_DISPLAY_LISTS = 0;
    }
    else
    {
	_dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_USE_DISPLAYLISTS);
	DO_DISPLAY_LISTS = 1;
    }

    if (!(OGLTRANSLATION = (hwTranslationP)_dxf_CREATE_HW_TRANSLATION(LWIN))) {
      PRINT (("out of memory allocating translation table")) ;
      DXErrorGoto (ERROR_INTERNAL, "#13000") ;
    }
  }

  OGL_FAIL_ON_ERROR(_dxf_CREATE_WINDOW);
  EXIT(("xid = 0x%x",xid));
  return (void *)xid ;

 error:

  OGL_FAIL_ON_ERROR(_dxf_CREATE_WINDOW);
  EXIT(("ERROR: xid = NULL"));
  return NULL ;
}

#endif

/*
 * These are only used on the DEC alpha to see what kind of
 * graphics adapter we are on.  For certain adapters we need to
 * disable certain things.
 */
#if defined(alphax)
static GLboolean isVendor(char *vendor)
{
  char vendor_string[256];
  strcpy(vendor_string, glGetString(GL_VENDOR));
  return !strcmp(vendor_string,vendor);
}

static GLboolean isRenderer(char *renderer)
{
  char renderer_string[256];
  strcpy(renderer_string, glGetString(GL_RENDERER));
  return !strcmp(renderer_string,renderer);
}
#endif

static Error
_dxf_INIT_RENDER_MODULE (void *globals)
{
  /*
   *  This routine is called once, after the graphics window has been
   *  created.  It's function is loosely defined to encompass all the
   *  `first time' initialization of the graphics window.
   */

  DEFGLOBALDATA(globals) ;

  ENTRY(("_dxf_INIT_RENDER_MODULE (0x%x)",globals));

  /* Mark the save buffer unallocated. */
  SAVE_BUF = (void *) NULL ;
  SAVE_BUF_SIZE = 0 ;
  BUFFER_MODE = DoubleBufferMode ;

  /* enable Z buffering */
  glEnable(GL_DEPTH_TEST) ;

#if defined(newBroken)
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

  /* Byte Align the Pixel Buffer */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);

# if defined(sgi)
     /*
	Temporary fix for SGI OpenGL problem :
        gl[Read|Write]Buffer(* GL_RGB *) when ! using glColor4*()
     */
     _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_INVALIDATE_BACKSTORE);
# endif
  
  /* For this release we will use the glColorMaterial mechanism even
     though it is not being used correctly in the macros in hwPortUtilOGL.c
     We do this because fixing the problem exposes a bug on the DEC
     HW */
#if 1
  glEnable(GL_COLOR_MATERIAL) ;
#else
  glDisable(GL_COLOR_MATERIAL) ;
#endif
  /*
   * Explicitly set draw buffer to BACK because default value
   * differs on different implementations, and set the ReadBuffer
   * to FRONT for consistancy.  Don't do it on the DEC becuase that
   * causes problems with FRONT/BACK buffer management.
   */
  glDrawBuffer(GL_BACK);

#if !defined(alphax)
  glReadBuffer(GL_FRONT);
#endif

  /* IBM's Soft OpenGL seems to default to styled lines */
  glDisable(GL_LINE_STIPPLE);

  /* Renormalize the normals after transformation */
  glEnable(GL_NORMALIZE);

#if defined(alphax)
   if(isVendor("DEC") && isRenderer("PXG"))
      if(!getenv("DX_PXG_TRANSPARENCY"))
         _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_TRANSPARENT_OFF);
#endif

  EXIT(("TRUE;"));
  return TRUE ;
}


static void *
_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height)
{
  void *ret ;

  ENTRY(("_dxf_ALLOCATE_PIXEL_ARRAY(0x%x, %d, %d)",ctx, width, height));
  
  ret = tdmAllocateLocal(width*height*sizeof(int32)) ;

  EXIT(("ret = 0x%x", ret));
  return ret ;
}


static void 
_dxf_FREE_PIXEL_ARRAY (void *ctx, void *pixels)
{
  ENTRY(("_dxf_FREE_PIXEL_ARRAY(0x%x, 0x%x)",ctx, pixels));

  if (pixels)
    tdmFree(pixels) ;

  EXIT((""));
}


static void
_dxf_DESTROY_WINDOW (void *win)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_DESTROY_WINDOW (0x%x)",win));

  if (PORT_CTX) {
#if !defined(DX_NATIVE_WINDOWS)
    glXMakeCurrent (DPY, XWINID, OGLCTX);
    /* remove GLX context from list of existing contexts */
    remGLXContext(OGLCTX) ;
#endif

    if (SAVE_BUF) {
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
      SAVE_BUF = (void *) NULL ;
    }

#if !defined(DX_NATIVE_WINDOWS)
    if (OGLTRANSLATION) {
      tdmFree(OGLTRANSLATION) ;
      OGLTRANSLATION = NULL ;
    }

    /* we no longer have a display associated with current GLX context */
    CurrentDpy = 0 ;
#endif

    if (DISPLAY_LIST_HASH)
	_dxf_DeleteDisplayListHash(DISPLAY_LIST_HASH);

    if (TEXTURE_HASH)
	_dxf_DeleteObjectHash(TEXTURE_HASH);

#if defined(DX_NATIVE_WINDOWS)
    if (OGLHRC)
		wglDeleteContext(OGLHRC);
    DeleteDC(OGLHDC);
#else
    /* OpenGL is a lot happier if we set context NULL before destroying */
    glXMakeCurrent (DPY, None, NULL) ;

    PRINT (("Destroying GLX context 0x%x on display 0x%x", OGLCTX, OGLDPY)) ;
    glXDestroyContext (DPY, OGLCTX) ;
#endif

    tdmFree(PORT_CTX) ;
    PORT_HANDLE->private = (void *) NULL ;
  }

  EXIT((""));
}

#if defined(DX_NATIVE_WINDOWS)
static void _dxf_SET_OUTPUT_WINDOW(void *win, HRGN hrgn)
#else
static void _dxf_SET_OUTPUT_WINDOW(void *win, Window w)
#endif
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE);

  ENTRY(("_dxf_SET_OUTPUT_WINDOW(0x%x)",win));

RT_IFNDEF(DXMAKE_CURRENT)
#if !defined(DX_NATIVE_WINDOWS)
  if (CurrentDpy)
      /* release context on current display */
      glXMakeCurrent (CurrentDpy, None, NULL) ;
  
  if (glXMakeCurrent (DPY, w, OGLCTX)) {
    CurrentDpy = DPY ;
    EXIT((""));
  } else {
    EXIT(("glXMakeCurrent failed"));
  }
#endif
RT_ELSE
  EXIT(("No action RT_IFNDEF'd"));
RT_ENDIF
}


static
int _dxf_SET_BACKGROUND_COLOR (void *ctx, RGBColor c)
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_BACKGROUND_COLOR (0x%x, 0x%x)",
	 ctx, &c));
  CPRINT(c);

#if defined(DX_NATIVE_WINDOWS)
  c.r = (c.r <= 0.0) ? 0.0 : pow(c.r,1./0.5);
  c.g = (c.g <= 0.0) ? 0.0 : pow(c.g,1./0.5);
  c.b = (c.b <= 0.0) ? 0.0 : pow(c.b,1./0.5);
#else
  c.r = (c.r <= 0.0) ? 0.0 : pow(c.r,1./OGLTRANSLATION->gamma);
  c.g = (c.g <= 0.0) ? 0.0 : pow(c.g,1./OGLTRANSLATION->gamma);
  c.b = (c.b <= 0.0) ? 0.0 : pow(c.b,1./OGLTRANSLATION->gamma);
#endif

  BACKGROUND_COLOR = c ;
  glClearColor (c.r, c.g, c.b, 0) ;

  OGL_FAIL_ON_ERROR(_dxf_SET_BACKGROUND_COLOR);

  EXIT((""));

  return OK;
}


static void _dxf_INIT_RENDER_PASS (void *win, int bufferMode, int zBufferMode)
{
  /*
   *  Clear the window and prepare for a rendering pass.
   */
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_INIT_RENDER_PASS (0x%x, %d, %d)",
	 win, bufferMode, zBufferMode));

  _dxfChangeBufferMode (win, bufferMode) ; 
  SAVE_BUF_VALID = FALSE ;

  OGL_FAIL_ON_ERROR(_dxf_INIT_RENDER_PASS);

  glEnable(GL_BLEND);
  OGL_FAIL_ON_ERROR(glEnableBlend);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  OGL_FAIL_ON_ERROR(glBlendFunc);
  //glAlphaFunc(GL_GREATER, 0.0);
  //OGL_FAIL_ON_ERROR(glAlphaFunc);

  EXIT((""));
}

/* #if defined(DX_NATIVE_WINDOWS) */
static void _dxf_END_RENDER_PASS(void *win)
{
#if defined(DX_NATIVE_WINDOWS)
	OGLWindow *oglw;
#endif
	DEFWINDATA(win) ;
	DEFPORT(PORT_HANDLE) ;
	glDisable(GL_BLEND);
	OGL_FAIL_ON_ERROR(_dxf_END_RENDER_PASS);

#if defined(DX_NATIVE_WINDOWS)
	oglw = GetOGLWPtr(XWINID);
	
	glFlush();
	SwapBuffers(OGLHDC);
	EndPaint(XWINID, &oglw->ps);
#endif

}
/* #endif */

static void _dxf_SET_VIEWPORT (void *ctx,
			       int left, int right, int bottom, int top)
{
  /*
   *  Set up viewport in pixels.
   */

#if defined(DX_NATIVE_WINDOWS)
  DEFCONTEXT(ctx);
  wglMakeCurrent(OGLHDC, OGLHRC);
#endif

  ENTRY(("_dxf_SET_VIEWPORT (0x%x, %d, %d, %d, %d)",
	 ctx, left, right, bottom, top));
  
  glViewport (left, bottom, (right-left)+1, (top-bottom)+1) ;

  OGL_FAIL_ON_ERROR(_dxf_SET_VIEWPORT);
  EXIT((""));
}

static void _dxf_CLEAR_AREA(void *ctx,
			    int left, int right, int bottom, int top)
{
  
  ENTRY(("_dxf_CLEAR_AREA(0x%x, %d, %d, %d, %d)",
	 ctx, left, right, bottom, top));

  glPushAttrib(GL_VIEWPORT_BIT) ;
  glViewport (left, bottom, (right-left)+1, (top-bottom)+1) ;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) ;
  glPopAttrib() ;

  OGL_FAIL_ON_ERROR(_dxf_CLEAR_AREA);
  EXIT((""));
}

static void _dxf_SET_WINDOW_SIZE (void *win, int w, int h)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_SET_WINDOW_SIZE (0x%x, %d, %d)",
	 win, w, h));

    WINHEIGHT = h ;
  WINWIDTH = w ;

#if defined(DX_NATIVE_WINDOWS)
  MoveWindow(XWINID, 0, 0, w, h, FALSE); 
  _dxf_SET_VIEWPORT (PORT_CTX, 0, w-1, 0, h-1) ;
#else
  if(!getenv("IGNORE_GLXWAITX"))
  glXWaitGL() ;

  XResizeWindow (DPY, XWINID, w, h) ;

  if(!getenv("IGNORE_GLXWAITX"))
  glXWaitX() ;

  _dxf_SET_VIEWPORT (PORT_CTX, 0, w-1, 0, h-1) ;
#endif

  OGL_FAIL_ON_ERROR(_dxf_SET_WINDOW_SIZE);
  EXIT((""));
}

static void _dxf_MAKE_PROJECTION_MATRIX (int projection, float width,
					 float aspect, float Near, float Far,
					 register float M[4][4])
{
  /*
   *  Compute a projection matrix for a simple orthographic or perspective
   *  camera model (no oblique projections).  This matrix projects view
   *  coordinates into a normalized projection coordinate system spanning
   *  [-1..1] in X, Y, and Z.
   *
   *  The Near and Far parameters are distances from the view coordinate
   *  origin along the line of sight.  The Z clipping planes are at -Near
   *  and -Far since we are using a right-handed coordinate system.  For
   *  perspective, Near and Far must be positive in order to avoid drawing
   *  objects at or behind the eyepoint depth.
   *
   *  The perspective depth scaling is handled as in Newman & Sproull
   *  Chap. 23, except for their left-handed orientation and a different
   *  range for the projection depth [D..F], which for GL is [1..-1].
   *
   *  In the non-oblique perspective matrix of Equation 23-4,
   *  Zs = (Ze*M33 + M43) / (Ze*M34)
   *     = M33/M34 + (M43/M34)/Ze
   *     = a + b/Ze
   *
   *  In this formulation, M34 = S/D = fov/2.  Alternatively, if we use
   *  M34 = 1, then M33 = a and M43 = b.  We must then divide M11 and M22
   *  by fov/2 (the same as is done for an ordinary ortho projection).
   *
   *  Substituting D and F for Ze in Equation 23-2 (Zs = a +b/Ze) produces
   *  a + b/D = 1 and a + b/F = -1.  Solving results in a = 1 - 2*F/(F-D)
   *  and b = 2*F*D/(F-D).
   *
   *  The projection matrix so produced seems to have adequate precision
   *  to represent view angles down to 0.001 degree, which is what DX
   *  requires.
   */

  ENTRY(("_dxf_MAKE_PROJECTION_MATRIX (%d, %f, %f, %f, %f, 0x%x)",
	 projection, width, aspect, Near, Far, M));

  Far = -Far ;
  Near = -Near ;

  M[0][0]=1/(width/2) ; M[0][1]=0 ;              M[0][2]=0 ; M[0][3]=0 ;
  M[1][0]=0 ;           M[1][1]=M[0][0]/aspect ; M[1][2]=0 ; M[1][3]=0 ;
  M[2][0]=0 ;           M[2][1]=0 ;           /* M[2][2]=1 ; M[2][3]=0 ; */
  M[3][0]=0 ;           M[3][1]=0 ;           /* M[3][2]=0 ; M[3][3]=1 ; */

  if (projection)
    {
      /* perspective */
      M[2][2] =  1 - (2*Far) / (Far-Near) ;
      M[3][2] = (2*Far*Near) / (Far-Near) ;
      M[2][3] = -1 ;  /* Ze < 0, but clipper wants positive w */
      M[3][3] =  0 ;
    }
  else
    {
      /* orthographic */
      M[2][2] = 1 / ((Far-Near) / 2) ;
      M[3][2] = -Near*M[2][2] - 1 ;
      M[2][3] = 0 ;
      M[3][3] = 1 ;
    }

  MPRINT(M);
  EXIT((""));
}


static void _dxf_SET_ORTHO_PROJECTION (void *ctx, float width,
				       float aspect, float Near, float Far)
{
  /*
   *  Set up an orthographic view projection.
   */
  float M[4][4] ;

  ENTRY(("_dxf_SET_ORTHO_PROJECTION (0x%x %f, %f, %f, %f)",
	 ctx, width, aspect, Near, Far));

  _dxf_MAKE_PROJECTION_MATRIX (0, width, aspect, Near, Far, M) ;

  glMatrixMode(GL_PROJECTION) ;
  glLoadMatrixf((GLfloat *)M) ;
  glMatrixMode(GL_MODELVIEW) ;

  MPRINT(M);
  OGL_FAIL_ON_ERROR(_dxf_SET_ORTHO_PROJECTION);
  EXIT((""));
}


static void _dxf_SET_PERSPECTIVE_PROJECTION (void *ctx, float xfov,
					     float aspect, float Near,
					     float Far)
{
  /*
   *  Set up a perspective projection.  
   */
  float M[4][4] ;

  ENTRY(("_dxf_SET_PERSPECTIVE_PROJECTION (0x%x, %f, %f, %f, %f)",
	 ctx, xfov, aspect, Near, Far));

  _dxf_MAKE_PROJECTION_MATRIX (1, xfov, aspect, Near, Far, M) ;

  glMatrixMode(GL_PROJECTION) ;
  glLoadMatrixf((GLfloat *)M) ;
  glMatrixMode(GL_MODELVIEW) ;

  MPRINT(M);
  OGL_FAIL_ON_ERROR(_dxf_SET_PERSPECTIVE_PROJECTION);
  EXIT((""));
}


static void _dxf_GET_PROJECTION(void * ctx, float M[4][4])
{
   /*
      Fetch the current Projection matrix
   */
   ENTRY(("_dxf_GET_PROJECTION(0x%x)", ctx));
   glGetFloatv(GL_PROJECTION_MATRIX, (float *)M);
   MPRINT(M);
   OGL_FAIL_ON_ERROR(_dxf_GET_PROJECTION);
   EXIT((""));
}

static void _dxf_GET_MATRIX(void * ctx, float M[4][4])
{
  /*
     Fetch the current modelview matrix
  */
  ENTRY(("_dxf_GET_MATRIX(0x%x)", ctx));
  glGetFloatv(GL_MODELVIEW_MATRIX, (float *)M);
  MPRINT(M);
  OGL_FAIL_ON_ERROR(_dxf_GET_MATRIX);
  EXIT((""));
}

static void _dxf_LOAD_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  Load (replace) M onto the hardware matrix stack.
   */
  ENTRY(("_dxf_LOAD_MATRIX (0x%x, 0x%x)", ctx, M));

  glLoadMatrixf((GLfloat *)M) ;

  MPRINT(M);
  OGL_FAIL_ON_ERROR(_dxf_LOAD_MATRIX);
  EXIT((""));
}


static void _dxf_PUSH_MULTIPLY_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  Pushing and multiplying are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */

  ENTRY(("_dxf_PUSH_MULTIPLY_MATRIX (0x%x, 0x%x)",ctx, M));

  OGL_FAIL_ON_ERROR(into_dxf_PUSH_MULTIPLY_MATRIX);
  glPushMatrix() ;
  glMultMatrixf((GLfloat *)M) ;

  MPRINT(M);
  OGL_FAIL_ON_ERROR(_dxf_PUSH_MULTIPLY_MATRIX);
  EXIT((""));
}


static void _dxf_PUSH_REPLACE_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  Pushing and loading are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */
  ENTRY(("_dxf_PUSH_REPLACE_MATRIX (0x%x, 0x%x)",ctx, M));

  glPushMatrix() ;
  glLoadMatrixf((GLfloat *)M) ;

  MPRINT(M) ;
  OGL_FAIL_ON_ERROR(_dxf_PUSH_REPLACE_MATRIX);
  EXIT((""));
}


static void _dxf_REPLACE_VIEW_MATRIX (void *ctx, float M[4][4])
{
  /*
   *  This is a badly named routine.  It actually replaces the projection
   *  matrix.
   */
  ENTRY(("_dxf_REPLACE_VIEW_MATRIX (0x%x, 0x%x)",ctx, M));

  glMatrixMode(GL_PROJECTION) ;
  glLoadMatrixf((GLfloat *)M) ;
  glMatrixMode(GL_MODELVIEW) ;

  MPRINT(M);
  OGL_FAIL_ON_ERROR(_dxf_REPLACE_VIEW_MATRIX);
  EXIT((""));
}


static void _dxf_POP_MATRIX (void *ctx)
{
  ENTRY(("_dxf_POP_MATRIX (0x%x)",ctx));

  glPopMatrix() ;

  OGL_FAIL_ON_ERROR(_dxf_POP_MATRIX);
  EXIT((""));
}

#if defined(DX_NATIVE_WINDOWS)

static void _dxf_SWAP_BUFFERS(void *ctx)

#else
static void _dxf_SWAP_BUFFERS(void *ctx, Window w)

#endif
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SWAP_BUFFERS(0x%x)",ctx));

#if defined(DX_NATIVE_WINDOWS)
  SwapBuffers (OGLHDC);
#else
  glXSwapBuffers (OGLDPY, w) ;
#endif

  OGL_FAIL_ON_ERROR(_dxf_SWAP_BUFFERS);
  EXIT((""));
}

static Error
_dxf_ClearBuffer(void *win)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) ;
    return OK;
}

static void _dxf_SET_MATERIAL_SPECULAR (void *ctx,
					float r, float g, float b, float power)
{
  /*
   *  Set material specular color and shininess.  OpenGL range for specular
   *  power is 0-128.  Material diffuse and ambient colors are set when
   *  generating OpenGL graphics primitives in hwPortUtilOGL.c.  DX
   *  doesn't use the notion of varying specular properties across a
   *  single material's surface.
   */
  float color[4] ;

  ENTRY(("_dxf_SET_MATERIAL_SPECULAR (0x%x, %f, %f, %f, %f)",
	 ctx, r, g, b, power));

  power = (power < 0 ? 0 : (power > 127 ? 127 : power)) ;
  color[0] = r ; color[1] = g ; color[2] = b ; color[3] = 1 ;

  glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, power) ;
  glMaterialfv (GL_FRONT_AND_BACK, GL_SPECULAR, color) ;

  OGL_FAIL_ON_ERROR(_dxf_SET_MATERIAL_SPECULAR);
  EXIT((""));
}

static void _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (void *ctx, int on)
{
  /*
   *  Set global lighting attributes.  Some API's, like IBM GL, can only
   *  set certain lighting attributes such as attenuation globally across
   *  all defined lights; these API's should do the appropriate
   *  initialization through this call.  If `on' is FALSE, turn lighting
   *  completely off.
   *
   *  For OpenGL, we will keep some default values: no local viewer for
   *  specular reflection computations, and one-sided instead of two-sided
   *  lighting. Note that GL_LIGHTING is also turned on and off in the
   *  geometry rendering routines in hwPortUtilOGL.c.
   */
  ENTRY(("_dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(0x%x, %d)",ctx, on));
  
  if (on)
    {
      /*
       *  Specify no pervasive ambient lighting over the entire scene.
       *  Ambient lighting is handled with specific ambient lights passed to
       *  _dxf_DEFINE_LIGHT().
       *
       *  tjm says this will probably change soon.
       */
      
      glEnable(GL_LIGHTING) ;
      /*
       * Try this some day:
       * glLightModel(GL_LIGHT_MODEL_TWO_SIDED, GL_TRUE);
       */
    }
  else
    {
      glDisable(GL_LIGHTING) ;
    }
	
  OGL_FAIL_ON_ERROR(_dxf_SET_GLOBAL_LIGHT_ATTRIBUTES);
  EXIT((""));
}

static int _dxf_DEFINE_LIGHT (void *win, int n, Light light)
{
  /*
   *  Define and switch on light `id' using the attributes specified by
   *  the `light' parameter.  If `light' is NULL, turn off light `id'.
   *  If more lights are defined than supported by the API, the extra
   *  lights are ignored.
   */
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE);
  RGBColor color ;
  Vector direction ;
  float gamma;
  float new[4] ;
  int	lightNumber = GL_LIGHT0 + (n - 1);

  ENTRY(("_dxf_DEFINE_LIGHT (0x%x, %d, 0x%x)",
	 win, n, light));

#if defined(DX_NATIVE_WINDOWS)
  gamma = 2;
#else
  gamma = OGLTRANSLATION->gamma;
#endif

  if (n > 8) {
    /* some API's don't support more than 8 lights */
    DXWarning("#5195") ;
    EXIT(("more than 8 lights"));
    return OK ;
  }
  
  if (!light) {
    if (n == 0) {
      new[0] = new[1] = new[2] = 0.0 ; new[3] = 1 ;
      glLightModelfv (GL_LIGHT_MODEL_AMBIENT, new) ;
    } else {
      /* turn this light off */
      PRINT (("switching off light %d", n)) ;
      glDisable(lightNumber) ;
    }
    EXIT(("OK"));
    return OK ;
  }
  
  if (DXQueryCameraDistantLight (light, &direction, &color)) {
    /* Some funny DEBUGGING stuff left in */
    PRINT (("camera-relative, light direction")) ;
    SPRINT(direction) ;
    PRINT (("color"));
    CPRINT(color) ;
      
    if (n == 0) {
      PRINT (("tried to redefine light 0")) ;
      DXErrorGoto (ERROR_INTERNAL, "#13740");
    }
      
    /* load identity for camera relative-light */
    glPushMatrix() ;
    glLoadIdentity() ;

    new[0] = direction.x ; 
    new[1] = direction.y ; 
    new[2] = direction.z ; 
    new[3] = 0.0 ; /* homogeneous representation of distant light */

    glLightfv (lightNumber, GL_POSITION, new) ;

    /* set diffuse color.  ambient illumination supplied by separate light */
    new[0] = pow(color.r, 1./gamma); 
    new[1] = pow(color.g, 1./gamma); 
    new[2] = pow(color.b, 1./gamma); 
    new[3] = 1 ;
    glLightfv (lightNumber, GL_DIFFUSE, new) ;

    /* set specular color */
    glLightfv (lightNumber, GL_SPECULAR, new) ;
    
    /* switch on the light */
    glEnable(lightNumber) ;

    /* pop identity matrix */
    glPopMatrix() ;
  } else if (DXQueryDistantLight (light, &direction, &color)) {
    PRINT (("world-absolute, light direction")) ;
    SPRINT(direction) ;
    PRINT (("color")) ;
    CPRINT(color) ;

    if (n == 0) {
      PRINT (("tried to redefine light 0")) ;
      DXErrorGoto (ERROR_INTERNAL, "#13740");
    }
      
    new[0] = direction.x ; 
    new[1] = direction.y ; 
    new[2] = direction.z ; 
    new[3] = 0.0 ; /* homogeneous representation of distance light */
      
    glLightfv (lightNumber, GL_POSITION, new) ;

    /* set diffuse color.  ambient illumination supplied by separate light */
    new[0] = pow(color.r, 1./gamma);
    new[1] = pow(color.g, 1./gamma);
    new[2] = pow(color.b, 1./gamma); 
    new[3] = 1;
    glLightfv (lightNumber, GL_DIFFUSE, new) ;

    /* set specular color */
    glLightfv (lightNumber, GL_SPECULAR, new) ;

    /* switch on the light */
    glEnable(lightNumber) ;
  } else if(DXQueryAmbientLight (light, &color)) {
    PRINT (("ambient")) ;
    PRINT (("color")) ;
    CPRINT(color) ;
      
    new[0] = pow(color.r, 1./gamma);
    new[1] = pow(color.g, 1./gamma);
    new[2] = pow(color.b, 1./gamma); 
    new[3] = 1;
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, new) ;
  } else {
    PRINT (("invalid light")) ;
    DXErrorGoto (ERROR_DATA_INVALID, "#13750");
  }
  
  OGL_FAIL_ON_ERROR(_dxf_DEFINE_LIGHT);
  EXIT(("OK"));
  return OK ;

 error:
  
  OGL_FAIL_ON_ERROR(_dxf_DEFINE_LIGHT);
  EXIT(("ERROR"));
  return ERROR ;
}

static
int _dxf_READ_APPROX_BACKSTORE(void *win, int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_READ_APPROX_BACKSTORE(0x%x, %d, %d)",win, camw, camh));

  if (SAVE_BUF_SIZE != camw * camh) {
    if (SAVE_BUF) {
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
      SAVE_BUF = (void *) NULL ;
    }

    /* allocate a buffer */
    SAVE_BUF_SIZE = camw * camh ;
    if (!(SAVE_BUF = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, camw, camh))) {
      PRINT(("Out of memory in _dxf_READ_APPROX_BACKSTORE"));
      DXErrorGoto (ERROR_NO_MEMORY, "#13790") ;
    }
    /*
     * If we are running against an adapter that does not do
     * readpixels well (i.e. E&S) then simply fill
     * the buffer with black pixels.  Return OK so
     * common layer uses the buffer for the interactors.
     */
    if (_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
			SF_INVALIDATE_BACKSTORE)) {
#if 0
      bzero(SAVE_BUF,SAVE_BUF_SIZE*sizeof(GLuint));
#else
      memset(SAVE_BUF, 0, SAVE_BUF_SIZE*sizeof(GLuint));
#endif
      EXIT(("OK"));
      return OK ;
    }
  }	
 
  /*
   * DX hardware renderer expects pixels to be packed into 32-bit integers,
   * so we use GL_RGBA format with a type of GL_UNSIGNED_BYTE.  Default
   * read buffer is GL_BACK, so we must explicitly set GL_FRONT.
   *
   * Don't do the read if we are running against hardware that
   * does not do readpixels well.
   */
  if (!_dxf_isFlagsSet(_dxf_SERVICES_FLAGS(),
		       SF_INVALIDATE_BACKSTORE)) {
    
    glReadBuffer(GL_FRONT) ;
    glReadPixels(0, 0, camw, camh, GL_RGBA, GL_UNSIGNED_BYTE, SAVE_BUF) ;

    /* Reset to Back buffer */
    glReadBuffer(GL_BACK) ;
  }

  OGL_FAIL_ON_ERROR(_dxf_READ_APPROX_BACKSTORE);
  EXIT(("OK"));
  return OK ;

 error:

  OGL_FAIL_ON_ERROR(_dxf_READ_APPROX_BACKSTORE);
  EXIT(("ERROR"));
  return ERROR ;
}
  

/*
 * This function is only called if SAVE_BUFFER_VALID is TRUE
 * This will never be the case when SF_INVALIDATE_BACKSTORE
 * services flag is set.
 */
static void _dxf_WRITE_APPROX_BACKSTORE(void *win, int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_WRITE_APPROX_BACKSTORE(0x%x, %d, %d)",win, camw, camh));

  /* Draw saved backstore into front buffer */
  glDrawBuffer (GL_FRONT) ;

  /* Disable Z-buffer */
  glDisable (GL_DEPTH_TEST) ;

  SET_RASTER_SCREEN (0, 0) ;
  glDrawPixels(camw, PIXH, GL_RGBA, GL_UNSIGNED_BYTE, SAVE_BUF) ;

  /* make it visible */
  glFlush() ;

  /* Reset draws for normal operations */
  glDrawBuffer (GL_BACK) ;
  glEnable (GL_DEPTH_TEST) ;

  /* redraw interactor echos; don't save them in SAVE_BUF */
  _dxfRedrawInteractorEchos(INTERACTOR_DATA) ;

  OGL_FAIL_ON_ERROR(_dxf_WRITE_APPROX_BACKSTORE);
  EXIT((""));
}

static void _dxf_WRITE_PIXEL_RECT(void* win, unsigned char *buf,
				  int x, int y, int w, int h)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_WRITE_PIXEL_RECT(0x%x, 0x%x, %d, %d, %d, %d)",
	 win, buf, x, y, w, h));

  /* There still seems to be a bug here.  When the window is resized
   * larger than the 2d image, there is junk and gray in the new area.
   * Also...with multiple vertical arranged 2d images, the top image
   * is chopped off at the top (sometimes)
   */ 
  SET_RASTER_SCREEN (0, 0) ;
  glDrawPixels (w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf) ;

  /* This finish should be a flush */
  glFinish() ;

  OGL_FAIL_ON_ERROR(_dxf_WRITE_PIXEL_RECT);
  EXIT((""));
}

static Error _dxf_DRAW_IMAGE(void* win, dxObject image, translationO dummy)
{
#if defined(DX_NATIVE_WINDOWS)
  return ERROR;
#else
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  int		x,y,w,h ;
  unsigned char *pixels = NULL ;
  void * translation = OGLTRANSLATION ;

  ENTRY(("_dxf_DRAW_IMAGE(0x%x, 0x%x, 0x%x)", win, image, dummy));

  if(!DXGetImageBounds(image,&x,&y,&w,&h))
    goto error ;

  if(!(pixels = tdmAllocateZero(sizeof(uint32) * w * h)))
    goto error ;

  if(!_dxf_dither(image,w,h,x,y,translation,pixels))
    goto error ;

  _dxf_WRITE_PIXEL_RECT(win, pixels, 0, 0, w, h) ;

  tdmFree(pixels) ;
  pixels = NULL ;

  OGL_FAIL_ON_ERROR(_dxf_DRAW_IMAGE);
  EXIT(("OK"));
  return OK ;

 error:
  if(pixels) 
    {
      tdmFree(pixels) ;
      pixels = NULL ;
    }

  OGL_FAIL_ON_ERROR(_dxf_DRAW_IMAGE);
  EXIT(("ERROR"));
  return ERROR ;
#endif
}

static
int _dxf_ADD_CLIP_PLANES (void* win, Point *pt, Vector *normal, int count)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE) ;
  int		planeCount;
  ClipPlane	*planes = NULL;
  float		view[4][4];
  double	dview[4][4],datview[4][4];
  int		i;

  ENTRY(("_dxf_ADD_CLIP_PLANES(0x%x, 0x%x, 0x%x, %d)",win, pt, normal, count));

  planes = (ClipPlane *)DXAllocate(count*sizeof(ClipPlane));
  if (! planes)
      goto error;

  planeCount = CLIP_PLANE_CNT;
  CLIP_PLANE_CNT += count;

  if (CLIP_PLANE_CNT > MAX_CLIP_PLANES) {
    /* Too many clip planes specified, clip object ignored */
    DXWarning("#13890",MAX_CLIP_PLANES);
    EXIT(("CNT > MAX"));
    return OK;
  }
  
  /*
   * pt and normal are in world coordinates. Because the view matrix is
   * in the PROJECTION_MATRIX for xgl, this implies that we have view
   * coordinates for clip planes (not world as the document says).
   *
   * So we need to apply the inverse of the clipping transform to the 
   * pt and normal to correct for this.
   */
  /* !!!!! This is unused !!!!! */
  _dxfGetViewMatrix(MATRIX_STACK,view);
  COPYMATRIX(dview,view);
  _dxfAdjointTranspose(datview,dview);

  for(i=0;i < count; i++) {
    planes[planeCount].a = normal[i].x;
    planes[planeCount].b = normal[i].y;
    planes[planeCount].c = normal[i].z;
    planes[planeCount].d = -DXDot(normal[i],pt[i]);
    planeCount++;
  }

  planeCount--;
  for(i=0;i<count;i++)  {
    glClipPlane(GL_CLIP_PLANE0 + planeCount, (GLdouble *)&planes[planeCount]);
    glEnable(GL_CLIP_PLANE0 + planeCount);
    planeCount--;
  }

  DXFree((Pointer)planes);
  OGL_FAIL_ON_ERROR(_dxf_ADD_CLIP_PLANES);
  EXIT(("OK"));
  return OK;

 error:

  DXFree((Pointer)planes);
  OGL_FAIL_ON_ERROR(_dxf_ADD_CLIP_PLANES);
  EXIT(("ERROR"));
  return ERROR;
}

static
int _dxf_REMOVE_CLIP_PLANES (void* win, int count)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE) ;
  int		i;
  int		planeCount;

  ENTRY(("_dxf_REMOVE_CLIP_PLANES (0x%x, %d)",win, count));

  if (CLIP_PLANE_CNT > MAX_CLIP_PLANES) {
    CLIP_PLANE_CNT -= count;
    EXIT(("CNT > MAX"));
    return OK;
  }

  planeCount = CLIP_PLANE_CNT;
  CLIP_PLANE_CNT -= count;

  if(count > planeCount) {
    /* Too many clip planes deleted */
    DXErrorGoto(ERROR_INTERNAL,"#13900");
  }
  
  planeCount--;
  for(i=0;i<count;i++)  {
    glDisable(GL_CLIP_PLANE0 + planeCount);
    planeCount--;
  }

  OGL_FAIL_ON_ERROR(_dxf_REMOVE_CLIP_PLANES);
  EXIT(("OK"));
  return OK;

 error:

  OGL_FAIL_ON_ERROR(_dxf_REMOVE_CLIP_PLANES);
  EXIT(("ERROR"));
  return ERROR;
}

static int _dxf_GET_VERSION(char ** dsoStr)
{
   static char * DXHW_LIB_FLAVOR = "OpenGL";

   ENTRY(("_dxf_GET_VERSION(void)"));
   if(dsoStr)
      if(*dsoStr)
	 *dsoStr = DXHW_LIB_FLAVOR;
   return(DXHW_DD_VERSION);
   EXIT(("OK"));
}

/*
 *  STUB SECTION
 */
static void _dxf_SET_SINGLE_BUFFER_MODE(void *ctx)
{
  ENTRY(("_dxf_SET_SINGLE_BUFFER_MODE(0x%x)",ctx));
  EXIT(("ERROR: stub"));
}

static void _dxf_SET_DOUBLE_BUFFER_MODE(void *ctx)
{
  ENTRY(("_dxf_SET_DOUBLE_BUFFER_MODE(0x%x)",ctx));
  EXIT(("ERROR: stub"));
}

/*
static void _dxf_INIT_SW_RENDER_PASS (void *win)
{
  ENTRY(("_dxf_INIT_SW_RENDER_PASS(0x%x)",win));
  EXIT(("ERROR: stub"));
}
*/

/*
 *  PORT HANDLE SECTION
 */
extern tdmInteractorEchoT _dxd_hwInteractorEchoPortOGL ;

extern Error _dxf_DrawOpaqueOGL(tdmPortHandleP portHandle, xfieldP xf,
				RGBColor ambientColor, int buttonUp);
extern Error _dxf_DrawTranslucentOGL(void *globals,
				RGBColor * ambientColor, int buttonUp);
int _dxf_READ_IMAGE (void* win, void *buf);

static Error
_dxf_StartFrame(void *win)
{
    DEFWINDATA(win);
    DEFPORT(PORT_HANDLE);

#if defined(DX_NATIVE_WINDOWS)
	wglMakeCurrent(OGLHDC, OGLHRC);
#endif

    if (DO_DISPLAY_LISTS)
    {
	if (TEXTURE_HASH)
	    _dxf_ResetObjectHash(TEXTURE_HASH);
	else
	    TEXTURE_HASH = _dxf_InitObjectHash();

	if (DISPLAY_LIST_HASH)
	    _dxf_ResetDisplayListHash(DISPLAY_LIST_HASH);
	else
	    DISPLAY_LIST_HASH = _dxf_InitDisplayListHash();
    }

    return OK;
}

static Error
_dxf_EndFrame(void *win)
{
    DEFWINDATA(win);
    DEFPORT(PORT_HANDLE);

    if (DO_DISPLAY_LISTS)
    {
	if (TEXTURE_HASH)
	    TEXTURE_HASH = _dxf_CullObjectHash(TEXTURE_HASH);

	if (DISPLAY_LIST_HASH)
	    DISPLAY_LIST_HASH = _dxf_CullDisplayListHash(DISPLAY_LIST_HASH);
    }

    return OK;
}

static tdmDrawPortT _oglDrawPort = 
{
  /* AllocatePixelArray */       _dxf_ALLOCATE_PIXEL_ARRAY,
  /* AddClipPlanes */            _dxf_ADD_CLIP_PLANES,
  /* ClearArea */ 	       	 _dxf_CLEAR_AREA,
  /* CreateHwTranslation */	 _dxf_CREATE_HW_TRANSLATION,
  /* CreateWindow */	       	 _dxf_CREATE_WINDOW,
  /* DefineLight */ 	       	 _dxf_DEFINE_LIGHT,                  
  /* DestroyWindow */ 	       	 _dxf_DESTROY_WINDOW,                
  /* DrawClip */		 NULL, 
  /* DrawImage */		 _dxf_DRAW_IMAGE,
  /* DrawOpaque */		 _dxf_DrawOpaqueOGL,                           
  /* RemoveClipPlanes */         _dxf_REMOVE_CLIP_PLANES,
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
  /* SetDoubleBufferMode */    	 _dxf_SET_DOUBLE_BUFFER_MODE,        
  /* SetGlobalLightAttributes */ _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES,  
  /* SetMaterialSpecular */      _dxf_SET_MATERIAL_SPECULAR,         
  /* SetOrthoProjection */       _dxf_SET_ORTHO_PROJECTION,          
  /* SetOutputWindow */ 	 _dxf_SET_OUTPUT_WINDOW,                    
  /* SetPerspectiveProjection */ _dxf_SET_PERSPECTIVE_PROJECTION,    
  /* SetSingleBufferMode */    	 _dxf_SET_SINGLE_BUFFER_MODE,        
  /* SetViewport */ 	       	 _dxf_SET_VIEWPORT,                  
  /* SetWindowSize */ 	       	 _dxf_SET_WINDOW_SIZE,               
  /* SwapBuffers */ 	       	 _dxf_SWAP_BUFFERS,                  
  /* WriteApproxBackstore */     _dxf_WRITE_APPROX_BACKSTORE,        
  /* WriteImage */		 _dxf_WRITE_PIXEL_RECT,        
  /* Drawtransparent */		 _dxf_DrawTranslucentOGL,
  /* GetMartix */                _dxf_GET_MATRIX,
  /* GetProjection */            _dxf_GET_PROJECTION,
  /* GetVersion */               _dxf_GET_VERSION,
  /* ReadImage */		 _dxf_READ_IMAGE,
  /* StartFrame */               _dxf_StartFrame,
  /* EndFrame */                 _dxf_EndFrame,
  /* ClearBuffer */            	 _dxf_ClearBuffer

};

static void _dxfUninitPortHandle(tdmPortHandleP portHandle)
{
  ENTRY(("_dxfUninitPortHandle(0x%x)",portHandle));

  if (portHandle) {
    portHandle->portFuncs = NULL ;
    portHandle->echoFuncs = NULL ;
    /*
     * Don't free portHandle->private here cause it's
     * done in DESTROY_WINDOW as PORT_CTX
     */
    tdmFree(portHandle) ;
  }

  EXIT((""));
}

#if defined(DX_NATIVE_WINDOWS)
tdmPortHandleP _dxfInitPortHandleOGL(char *hostname)
#else
tdmPortHandleP _dxfInitPortHandleOGL(Display *dpy, char *hostname)
#endif
{
  tdmPortHandleP ret ;
    
  ENTRY(("_dxfInitPortHandleOGL(0x%x, \"%s\")",dpy, hostname));

#if 0
/* Not needed with newer Exceeds > 6.2 */
#if defined(intelnt) || defined(WIN32)
  HCLXtInit();
#endif
#endif

  _dxf_setFlags(_dxf_SERVICES_FLAGS(),
	SF_TMESH | SF_QMESH | SF_POLYLINES | SF_DOES_TRANS | SF_VOLUME_BOUNDARY);

#if !defined(DEBUG) && defined(RTLOAD)
  /* If the hardware rendering load module requires a newer set of
   * DX exported symbols than those available in the calling dxexec
   * return NULL.
   */
#if defined(alphax) || defined(sgi) || defined(solaris)
  /* If the hardware rendering load module requires a newer set of
   * DX exported symbols than those available in the calling dxexec
   */
{
  int 	execSymbolInterface;
  void*	(*symVer)();
  void*	(*expVer)();
  void*	DX_handle;

  if (!(DX_handle=dlopen(NULL,RTLD_LAZY))) {
    DXSetError(ERROR_UNEXPECTED,dlerror());
    EXIT(("ERROR"));
    return NULL;
  }

  if ((symVer = (void*(*)())dlsym(DX_handle, "DXSymbolInterface")) &&
      (expVer = (void*(*)())dlsym(DX_handle, "_dxfExportHwddVersion"))) {
    PRINT(("found DXSymbolInterface;"));
    (*symVer)(&execSymbolInterface);
    (*expVer)(DXD_HWDD_INTERFACE_VERSION);
  } else {
    execSymbolInterface = 0;
  }
    
  if(LOCAL_DXD_SYMBOL_INTERFACE_VERSION > execSymbolInterface) {
    DXSetError(ERROR_UNEXPECTED,"#13910", 
	       "hardware rendering", "DX Symbol Interface");
    EXIT(("ERROR"));
    return NULL;
  }
}
#elif !defined(DXD_WIN)
{
  int 	execSymbolInterface;

  if(! loadbind(0,_dxfInitPortHandleOGL,DXSymbolInterface))  {
    DXSymbolInterface(&execSymbolInterface);
    _dxfExportHwddVersion(DXD_HWDD_INTERFACE_VERSION);
  } else {
    execSymbolInterface = 0;
  }
  
  if(LOCAL_DXD_SYMBOL_INTERFACE_VERSION > execSymbolInterface) {
    DXSetError(ERROR_UNEXPECTED,"#13910", 
	       "hardware rendering", "DX Symbol Interface");
    EXIT(("ERROR: Symbol interface")); 
    return NULL;
  }
}
#else
    _dxfExportHwddVersion(DXD_HWDD_INTERFACE_VERSION);
#endif
#else
    _dxfExportHwddVersion(DXD_HWDD_INTERFACE_VERSION);
#endif

  ret = (tdmPortHandleP)tdmAllocateLocal(sizeof(tdmPortHandleT)) ;

  if(ret)
    {
      ret->portFuncs = &_oglDrawPort ;
      ret->echoFuncs = &_dxd_hwInteractorEchoPortOGL ;
      ret->uninitP = _dxfUninitPortHandle ;
      ret->private = NULL ;
    }

  EXIT(("ret = 0x%x",ret));
  return ret ;
}

int _dxf_READ_IMAGE (void* win, void *buf)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_READ_IMAGE (0x%x, 0x%x)", win, buf));
#if defined(DX_NATIVE_WINDOWS)
  if (wglMakeCurrent (OGLHDC, OGLHRC))
#else
  if (glXMakeCurrent (DPY, XWINID, OGLCTX)) 
#endif
    {
      glReadBuffer(GL_FRONT) ;
      glReadPixels(0, 0, PIXW, PIXH, GL_RGB, GL_UNSIGNED_BYTE, buf) ;
    }

  EXIT(("OK"));
  return OK;
}
