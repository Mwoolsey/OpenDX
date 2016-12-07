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


#ifndef tdmPortOGL_h
#define tdmPortOGL_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/opengl/hwPortOGL.h,v $

  Data structures used by OpenGL implementation of the DX hardware renderer.

\*---------------------------------------------------------------------------*/

#if !defined(DX_NATIVE_WINDOWS)
#include <GL/glx.h>
#endif
#include <GL/gl.h>

typedef struct {
  GLfloat x;
  GLfloat y;
  GLfloat z;
} oglVector ;

typedef struct {
  GLfloat r;
  GLfloat g;
  GLfloat b;
} oglRGBColor ;

typedef struct ClipPlaneS {
  double a;
  double b;
  double c;
  double d;
} ClipPlaneT, ClipPlane;

#if defined(DX_NATIVE_WINDOWS)

typedef struct {
  HGLRC hRC;
  HDC hDC;
  PAINTSTRUCT ps;
  HWND	win;
  int winHeight;
  int winWidth;
  int xmaxscreen;
  int ymaxscreen;
  RGBColor background_color;
  hwTranslationP translation;
  int fontListBase;
  int clipPlaneCount;
  int maxClipPlanes;
  WinT *winT;
  HashTable displayListHash;
  HashTable textureHash;
  int doDisplayLists;
  int currentTexture;
} tdmOGLctxT, *tdmOGLctx ;

#define DEFCONTEXT(ctx) \
  register tdmOGLctx _portContext = (tdmOGLctx)(ctx)

#define OGLHRC            (((tdmOGLctx)_portContext)->hRC)
#define OGLHDC            (((tdmOGLctx)_portContext)->hDC)
#define OGLPS             (((tdmOGLctx)_portContext)->ps)
#define OGLXWIN           (((tdmOGLctx)_portContext)->win)
#define WINWIDTH          (((tdmOGLctx)_portContext)->winWidth)
#define WINHEIGHT         (((tdmOGLctx)_portContext)->winHeight)
#define XMAXSCREEN        (((tdmOGLctx)_portContext)->xmaxscreen)
#define YMAXSCREEN        (((tdmOGLctx)_portContext)->ymaxscreen)
#define BACKGROUND_COLOR  (((tdmOGLctx)_portContext)->background_color)
#define OGLTRANSLATION   (((tdmOGLctx)_portContext)->translation)
#define FONTLISTBASE      (((tdmOGLctx)_portContext)->fontListBase)
#define CLIP_PLANE_CNT    (((tdmOGLctx)_portContext)->clipPlaneCount)
#define MAX_CLIP_PLANES   (((tdmOGLctx)_portContext)->maxClipPlanes)
#define OGLWINT           (((tdmOGLctx)_portContext)->winT)
#define DO_DISPLAY_LISTS  (((tdmOGLctx)_portContext)->doDisplayLists)
#define DISPLAY_LIST_HASH (((tdmOGLctx)_portContext)->displayListHash)
#define TEXTURE_HASH      (((tdmOGLctx)_portContext)->textureHash)
#define CURTEX            (((tdmOGLctx)_portContext)->currentTexture)
#define PATTERN_INDEX    1

#else

typedef struct {
  Display *dpy;
  int screen;
  Window win;
  GLXContext ctx;
  int winHeight;
  int winWidth;
  int xmaxscreen;
  int ymaxscreen;
  RGBColor background_color;
  hwTranslationP translation;
  int fontListBase;
  int clipPlaneCount;
  int maxClipPlanes;
  WinT *winT;
  HashTable displayListHash;
  HashTable textureHash;
  int doDisplayLists;
  int currentTexture;
} tdmOGLctxT, *tdmOGLctx ;

#define DEFCONTEXT(ctx) \
  register tdmOGLctx _portContext = (tdmOGLctx)(ctx)

#define OGLDPY           (((tdmOGLctx)_portContext)->dpy)
#define OGLSCREEN        (((tdmOGLctx)_portContext)->screen)
#define OGLXWIN          (((tdmOGLctx)_portContext)->win)
#define OGLCTX           (((tdmOGLctx)_portContext)->ctx)
#define WINWIDTH         (((tdmOGLctx)_portContext)->winWidth)
#define WINHEIGHT        (((tdmOGLctx)_portContext)->winHeight)
#define XMAXSCREEN       (((tdmOGLctx)_portContext)->xmaxscreen)
#define YMAXSCREEN       (((tdmOGLctx)_portContext)->ymaxscreen)
#define BACKGROUND_COLOR (((tdmOGLctx)_portContext)->background_color)
#define OGLTRANSLATION   (((tdmOGLctx)_portContext)->translation)
#define FONTLISTBASE     (((tdmOGLctx)_portContext)->fontListBase)
#define CLIP_PLANE_CNT   (((tdmOGLctx)_portContext)->clipPlaneCount)
#define MAX_CLIP_PLANES  (((tdmOGLctx)_portContext)->maxClipPlanes)
#define OGLWINT          (((tdmOGLctx)_portContext)->winT)
#define DO_DISPLAY_LISTS  (((tdmOGLctx)_portContext)->doDisplayLists)
#define DISPLAY_LIST_HASH (((tdmOGLctx)_portContext)->displayListHash)
#define TEXTURE_HASH      (((tdmOGLctx)_portContext)->textureHash)
#define CURTEX            (((tdmOGLctx)_portContext)->currentTexture)
#define PATTERN_INDEX    1

#endif

int      _dxf_isDisplayListOpenOGL();
void     _dxf_callDisplayListOGL(dxObject dlo);
dxObject _dxf_openDisplayListOGL();
void     _dxf_endDisplayListOGL();

/*
 * Debugging macros local to this port
 */
#if defined(DEBUG)

#if !defined(DXHW_OGL_ErrorList)
#  define DXHW_OGL_ErrorList
   static struct
   {
      int code;
      char * str;
   }
   _errorList[] =
   {
      GL_INVALID_ENUM,      "Invalid Enumeration",
      GL_INVALID_VALUE,     "Invalid Value",
      GL_INVALID_OPERATION, "Invalid Operation",
      GL_STACK_OVERFLOW,    "Stack Overflow",
      GL_STACK_UNDERFLOW,   "Stack Underflow",
      GL_OUT_OF_MEMORY,     "Out of Memory"
   };
#endif

#if 0
#define OGL_FAIL_ON_ERROR(x) 						\
{ 									\
  GLenum err;								\
  int iI;								\
									\
  while((err = glGetError()) != GL_NO_ERROR)				\
  {									\
    for(iI = 0; iI < 6; iI++)                                           \
       if(_errorList[iI].code == err) break;                            \
    printf("libGL error : %s() : %d(%s)\n",#x,				\
	   err, (iI > 5) ? "Unknown Error" : _errorList[iI].str); 	\
  } 									\
}
#else
#define OGL_FAIL_ON_ERROR(x)
#endif

#define CHECK_RASTER_POS() 						  \
{									  \
  GLfloat	pos[4];							  \
  GLboolean	valid;							  \
									  \
  DEBUG_CALL((glGetFloatv (GL_CURRENT_RASTER_POSITION, pos)));	          \
  DEBUG_CALL((glGetBooleanv (GL_CURRENT_RASTER_POSITION_VALID, &valid))); \
  PRINT(("current raster position (%f, %f %f, %f) is %d", pos[0],	  \
	 pos[1], pos[2], pos[3], valid));				  \
}
#else
#define OGL_FAIL_ON_ERROR(x)
#define CHECK_RASTER_POS()
#endif

/*
 *  World-to-screen utility macros.
 */
#define SET_WORLD_SCREEN(w, h) 						\
{									\
  GLfloat M[4][4] ;                                                       \
									\
  ENTRY(("SET_WORLD_SCREEN(%d, %d)", w, h));				\
									\
  glViewport (0, 0, w, h) ;						\
									\
  WS[0][0] = 2.0 / ((w-1) + 0.5) ;					\
  WS[1][1] = 2.0 / ((h-1) + 0.5) ;					\
  glMatrixMode(GL_PROJECTION) ;						\
  glLoadMatrixf((float *)WS) ;						\
									\
  glMatrixMode(GL_MODELVIEW) ;						\
  glLoadIdentity() ;							\
									\
  DEBUG_CALL((glGetFloatv(GL_PROJECTION_MATRIX,(GLfloat *)M)));		\
  PRINT(("\"%s\"","projection matrix"));				\
  MPRINT(M) ;                                                           \
  DEBUG_CALL((glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat *)M)));		\
  PRINT(("\"%s\"","model-view matrix"));				\
  MPRINT(M) ; 						      		\
  EXIT((""));								\
}

#define SET_RASTER_SCREEN(llx, lly)                                     \
{								  	\
  ENTRY(("SET_RASTER_SCREEN(%d, %d)", llx, lly));			\
                                                                        \
  glPushAttrib(GL_VIEWPORT_BIT) ;					\
  glMatrixMode(GL_MODELVIEW) ;						\
  glPushMatrix() ;							\
									\
  glMatrixMode(GL_PROJECTION) ;					        \
  glPushMatrix() ;							\
									\
  SET_WORLD_SCREEN (WINWIDTH, WINHEIGHT) ;				\
									\
  /* OpenGL transforms raster position by model/view/projection */      \
  PRINT(("calling glRasterPos2i(%d, %d)", llx, lly));			\
									\
  glRasterPos2i (llx, lly) ;                                            \
									\
  glMatrixMode(GL_PROJECTION) ;					        \
  glPopMatrix() ;							\
  glMatrixMode(GL_MODELVIEW) ;					        \
  glPopMatrix() ;							\
  glPopAttrib() ;							\
                                                                        \
  CHECK_RASTER_POS();							\
  EXIT((""));								\
}

#if defined(DXHW_UselessCycles)

#define Apply(poi, mat, res)                                               \
{                                                                          \
    res[0] = mat[0][0] * poi[0] + mat[1][0] * poi[1] + mat[2][0] * poi[2]; \
    res[1] = mat[0][1] * poi[0] + mat[1][1] * poi[1] + mat[2][1] * poi[2]; \
    res[2] = mat[0][2] * poi[0] + mat[1][2] * poi[1] + mat[2][2] * poi[2]; \
}

#else

#define Apply(poi, mat, res)                                                  \
    res[2] = mat[0][2] * poi[0] + mat[1][2] * poi[1] + mat[2][2] * poi[2];

#endif

#endif /* tdmPortOGL_h */
