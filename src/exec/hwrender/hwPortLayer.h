/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmPortLayer_h
#define tdmPortLayer_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwPortLayer.h,v $
  Author: Mark Hood

  This file contains ANSI prototypes for the tdm porting layer routines.

\*---------------------------------------------------------------------------*/

#define LOCAL_DXD_SYMBOL_INTERFACE_VERSION 0

/*
 *  graphics hardware types.
 */

#define	GTO		1
#define	GTOZMIN		0x00000000
#define	GTOZMAX		0x001FFFFF

#define HP3D		2
#define HP3DZMIN	0xFF800000
#define	HP3DZMAX	0x007FFFFF

#define SGP		3
#define Gt4		4
#define Gt4x		5


#ifdef STANDALONE
/*
 *  This section is used by programs testing the porting layer.
 *  Corresponding DX-specific code is in hwWindow.h and Light.H.
 */
typedef struct {
#if defined(DX_NATIVE_WINDOWS)
  HWND wigWindow;
  HWND xid, parentWindow;
#else
  Display *dpy ;
  Window xid, parentWindow ;
  Window lxid, rxid;
#endif
  Colormap colorMap ;
  tdmInteractorWin interactorData ;
  int pixw, pixh ; 
  int hwNumImagePlanes, hwZbufferMinValue, hwZbufferMaxValue ;
  int hwNumZbufferBits, hwType, bufferMode, visibility ;
  int saveBufValid, saveBufSize ;
  void *imageCtx, *stack, *saveBuf, *swBuf ;
  /* #X */ int uitype;
} tdmRenderInstance ;

#define DEFGLOBALDATA(G) \
register tdmRenderInstance *instance = (tdmRenderInstance *) G
  
#define DEFWINDATA(W) DEFGLOBALDATA(W)
#define DPY (instance->dpy)
#define XWINID (instance->xid)
#define LWINID (instance->lxid)
#define RWINID (instance->rxid)
#define PARENT_WINDOW (instance->parentWindow)
#define PIXW (instance->pixw)
#define PIXH (instance->pixh)
#define BUFFER_MODE (instance->bufferMode)
#define VISIBILITY (instance->visibility)
#define CLRMAP (instance->colorMap)
#define IMAGE_CTX (instance->imageCtx)
#define STACK (instance->stack)
#define MATRIX_STACK (instance->stack)
#define INTERACTOR_DATA (instance->interactorData)
#define SW_BUF (instance->swBuf)
#define SAVE_BUF (instance->saveBuf)
#define SAVE_BUF_SIZE (instance->saveBufSize)
#define SAVE_BUF_VALID (instance->saveBufValid)

/* #X follows */
#define UI_TYPE (instance->uitype)
#define DXD_DXUI 	1
#define DXD_EXTERNALUI 2
#define DXD_NOUI 	3
#define DXUI (UI_TYPE == DXD_DXUI)
#define EXTERNALUI (UI_TYPE == DXD_EXTERNALUI)
#define NOUI (UI_TYPE == DXD_NOUI)
/* #X end */

typedef int Error ;
#define DXResetError()
#define DXWarning(string)
#define DXSetError(code, string)
#define DXErrorGoto(code, string) goto error
#define DXErrorReturn(code, string) return FALSE
#define _dxfChangeBufferMode(win, mode) 
#define _dxfLoadRenderOptionsFile(globals) (TRUE)
#define tdmAllocate malloc
#define tdmReAllocate realloc
#ifndef tdmFree
#define tdmFree free
#endif
#ifndef tdmAllocateLocal
#define tdmAllocateLocal malloc
#endif

#define XmVersion 1001

typedef struct { float x, y, z ; } Point, Vector ;
typedef struct { float r, g, b ; } RGBColor ;
typedef struct {
    enum { invalid, point, distant, ambient } kind ;
    enum { camera, world } relative ;
    Point position ;
    Vector direction ;
    RGBColor color ;
} tdmLight, *Light ;

#define DXQueryCameraDistantLight(l, d, c) \
(l->kind!=distant || l->relative!=camera? 0: (*d=l->direction, *c=l->color, l))
#define DXQueryDistantLight(l, d, c) \
(l->kind!=distant || l->relative!=world? 0: (*d=l->direction, *c=l->color, l))
#define DXQueryAmbientLight(l, c) \
(l->kind != ambient? 0: (*c=l->color, l))
#else  /* STANDALONE */

#define SingleBufferMode (1)
#define DoubleBufferMode (2)
#define ZBufferEnabled (1)
#define ZBufferDisabled (0)

#endif /* STANDALONE */


typedef struct tdmParsedFormatS {
  char*	type;
  char*	where;
  char* fullHost;
  char* localHost;
  char* Xserver;
  char*	name;
  char*	cacheId;
  int   link;
  char* originalWhere;

} tdmParsedFormatT;

/*
 * !! IMPORTANT !!
 *
 * To maintain backward compatibility is is important that existing interfaces
 * are not changed or deleted, and that new functions are added at the end
 * of the table. 
 *
 * When new functions are added at the end of the 'tdmDrawPort' 
 * or 'tdmInteractorEcho' structures, the DXD_HWDD_INTERFACE_VERSION #define
 * in libdx/dx/version.h must be incremented and entry point macros in this
 * file must check the version before attempting the call. 
 * (see _dxf_ADD_CLIP_PLANES).
 *
 * This is because checking for NULL won't work if the structure was
 * smaller in a previous version.
 */

typedef struct tdmDrawPortS {
  void*	(*AllocatePixelArray) (void *ctx, int w, int h);
  Error	(*AddClipPlanes)(void* win, Point *pt, Vector *normal, int count);
  void	(*ClearArea) (void *ctx, int left, int right, int bottom, int top);
  translationO	(*CreateHwTranslation)(void *win);
  void*	(*CreateWindow) (void *globals, char *winName, int w, int h);
  int	(*DefineLight) (void *win, int n, Light  light);
  void	(*DestroyWindow) (void *win);
  Error	(*DrawClipping)(tdmPortHandleP portHandle, xfieldP xfield, 
			int buttonDown);
  Error (*DrawImage)(void* win, dxObject image,
		     translationO translation);
  Error	(*DrawOpaque)(tdmPortHandleP portHandle, xfieldP xfield, 
		      RGBColor ambientColor, int buttonDown);
  Error	(*RemoveClipPlanes)(void* win, int count);
  void	(*FreePixelArray) (void *ctx, void *pixels);
  Error	(*InitRenderModule) (void *globals);
  void	(*InitRenderPass) (void *win, int bufferMode, int zBufferMode);
  void	(*LoadMatrix) (void *ctx, float M[4][4]);
  void	(*MakeProjectionMatrix) (int projection, float width, float aspect,
 				 float Near, float Far, float M[4][4]);
  void	(*PopMatrix) (void *ctx);
  void	(*PushMultiplyMatrix) (void *ctx, float M[4][4]);
  void	(*PushReplaceMatrix) (void *ctx, float M[4][4]);
  int	(*ReadApproxBackstore) (void *win, int camw, int camh);
  void	(*ReplaceViewMatrix) (void *ctx, float M[4][4]);
  int	(*SetBackgroundColor) (void *ctx, RGBColor color);
  void	(*SetDoubleBufferMode) (void *ctx);
  void	(*SetGlobalLightAttributes) (void *ctx, int on);
  void	(*SetMaterialSpecular) (void *ctx, float r, float g, float b,
 				float pow);
  void	(*SetOrthoProjection) (void *ctx, float width, float aspect,
 			       float Near, float Far);
#if defined(DX_NATIVE_WINDOWS)
  void  (*SetOutputWindow) (void *win, HRGN);
#else
  void	(*SetOutputWindow) (void *win, Window);
#endif
  void	(*SetPerspectiveProjection) (void *ctx, float xfov, float aspect,
 				     float Near, float Far);
  void	(*SetSingleBufferMode) (void *ctx);
  void	(*SetViewport) (void *ctx, int left, int right, int bottom, int top);
  void	(*SetWindowSize) (void *win,int w,int h);
#if defined(DX_NATIVE_WINDOWS)
  void	(*SwapBuffers) (void *ctx);
#else
  void	(*SwapBuffers) (void *ctx, Window);
#endif
  void	(*WriteApproxBackstore) (void *win, int camw, int camh);
  void	(*WritePixelRect) (void* win, unsigned char *buf, 
			   int x, int y,int w, int h);

  /* End of functions available in 2.0 and 2.0.1 */

  /* End of 3.0 */

  Error (*DrawTransparent)(void *globals,
		RGBColor * ambientColor, int buttonUp);
  void  (*GetMatrix)(void *ctx, float M[4][4]);
  void  (*GetProjection)(void * ctx, float M[4][4]);
  int   (*GetVersion)(char **);
  Error (*ReadImage) (void *win, void *buf);
  Error (*StartFrame) (void *win);
  Error (*EndFrame) (void *win);
  Error (*ClearBuffer) (void *win);

} tdmDrawPortT;

/*
 * Port Layer light definitions 
 */

typedef struct tdmSimpleLightS {
  Vector	direction;
  RGBColor	color;
  int		isAmbient;
} tdmSimpleLightT;



/*
 * Port Layer handle
 */
typedef struct tdmPortHandleS {
  void*			dpy;
  tdmDrawPortP		portFuncs;
  tdmInteractorEchoP	echoFuncs;
  void			(*uninitP)();
  void*			private;
} tdmPortHandleT;

#ifndef PRIVATE_TYPE
#define PRIVATE_TYPE void *
#endif

#define DEFPORT(portHandle)  						\
  register PRIVATE_TYPE _portContext =  				\
       (PRIVATE_TYPE) ((tdmPortHandleP)portHandle)->private; 		\
  register tdmInteractorEchoP _eFuncs = 				\
       (tdmInteractorEchoP) ((tdmPortHandleP)portHandle)->echoFuncs; 	\
  register tdmDrawPortP _pFuncs = 					\
       (tdmDrawPortP) ((tdmPortHandleP)portHandle)->portFuncs

#define PORT_CTX _portContext
#define EFUNCS(foo) (_eFuncs->foo)

/*
 *  The rest of the porting layer is callable by either a test program or
 *  the DX hardware renderer implementation.
 */


#ifndef STRUCTURES_ONLY
#ifndef OLD_PORT_LAYER_CALLS

extern int	_dxd_lmHwddVersion;	

#define _dxf_DRAW_IMAGE( win, image, translation) \
  ((! _pFuncs->DrawImage)? OK : \
   (*_pFuncs->DrawImage)(win, image, translation))

#define _dxf_DRAW_OPAQUE( portHandle,  xf,  ambientColor, buttonUP) \
  (*_pFuncs->DrawOpaque)(portHandle,xf,ambientColor,buttonUp)

#define _dxf_CREATE_WINDOW( globals, winName, w, h) \
  (*_pFuncs->CreateWindow)(globals, winName, w, h)

#define _dxf_CREATE_HW_TRANSLATION(win) \
  ((! _pFuncs->CreateHwTranslation)? OK : \
   (*_pFuncs->CreateHwTranslation)(win))

#define _dxf_DRAW_TRANSPARENT(globals, ambientColor, buttonUP) \
  ((! _pFuncs->DrawTransparent)? OK : \
   (*_pFuncs->DrawTransparent)(globals, ambientColor, buttonUP))

   /* Rasterize the sorted list of primitves, if the port requires a sorted
   * list for transparent primitives, then this should be defined and
   * be able to traverse the sortIndex list and dereference to the 
   * require connection in the associated xfield.
   */

#define _dxf_DRAW_CLIP(portHandle, xfield, buttonDown) \
  ((! _pFuncs->DrawClipping)? OK : \
   (*_pFuncs->DrawClipping) (portHandle, xfield, buttonDown))
  /*
   */

#define _dxf_ADD_CLIP_PLANES(win, pt, normal, count)  \
  ((_dxd_lmHwddVersion < 1 ) || (!_pFuncs->AddClipPlanes) ? OK : \
   (*_pFuncs->AddClipPlanes)(win, pt, normal, count))
  /*
   *  Define and enable multiple clip planes. Plane 'n' passes through point 
   *  pt[n]  and is perpendicular to normal[n]. Primitives in the half space
   *  into which the 'normal' points are kept, all other primitives are clipped.
   *
   * 'pt' and 'normal' are given in modeling space.
   *
   * If passed NULL, should disable clipping. 
   */

#define _dxf_REMOVE_CLIP_PLANES(win, count) \
  ((_dxd_lmHwddVersion < 1 ) || (!_pFuncs->RemoveClipPlanes) ? OK : \
   (*_pFuncs->RemoveClipPlanes)(win, count))
  /*
   * Should set up the hardware to clip to the given object. If passed
   * NULL, should disable clipping. 
   */

#define	_dxf_DESTROY_WINDOW(win) (*_pFuncs->DestroyWindow)(win)
  /*
   *  Destroy specified window.
   */

#if defined(DX_NATIVE_WINDOWS)
#define	_dxf_SET_OUTPUT_WINDOW(win, rgn) (*_pFuncs->SetOutputWindow)(win, rgn)
#else
#define	_dxf_SET_OUTPUT_WINDOW(win, wid) (*_pFuncs->SetOutputWindow)(win, wid)
#endif
  /*
   *  Direct all graphics to specified window
   */

#define	_dxf_SET_WINDOW_SIZE(win, w, h) (*_pFuncs->SetWindowSize)(win,w,h)
  /*
   *  Set graphics window to the specified size
   */

#define	_dxf_INIT_RENDER_MODULE(globals) (*_pFuncs->InitRenderModule)(globals)
  /*
   *  This routine is called once, after the graphics window has been
   *  created.  It's function is loosely defined to encompass all the
   *  `first time' initialization of the target graphics API.
   */
#if defined(DX_NATIVE_WINDOWS)
#define	_dxf_SWAP_BUFFERS(ctx) (*_pFuncs->SwapBuffers)(ctx)
#else
#define	_dxf_SWAP_BUFFERS(ctx, wid) (*_pFuncs->SwapBuffers)(ctx, wid)
#endif
  /*
   *  Switch front and back buffers.
   */

#define	_dxf_SINGLE_BUFFER_MODE(ctx) (*_pFuncs->SetSingleBufferMode)(ctx)
  /*
   *  Go into single buffer mode
   */

#define	_dxf_DOUBLE_BUFFER_MODE(ctx) (*_pFuncs->SetDoubleBufferMode)(ctx)
  /*
   *  Go into double buffer mode.
   */

#define	_dxf_CLEAR_AREA(ctx, left, right, bottom, top) \
       (*_pFuncs->ClearArea)(ctx, left, right, bottom, top)
  /*
   *  Clear image and zbuffer planes in specified pixel bounds.
   */

#define	_dxf_WRITE_PIXEL_RECT(win, buf, x, y, w, h) \
  ((! _pFuncs->WritePixelRect)? OK : \
   (*_pFuncs->WritePixelRect)(win, buf, x, y, w, h))
  /*
   *  DXWrite pixels from software rendering buffer (SW_BUF) into the main
   *  graphics window.  camw and camh are the DX camera resolutions in width
   *  and height; these may differ from the graphics window dimensions.
   */

#define _dxf_READ_IMAGE(win, buf) (*_pFuncs->ReadImage)(win, buf)

#define _dxf_STARTFRAME(win) (*_pFuncs->StartFrame)(win)
#define _dxf_ENDFRAME(win)   (*_pFuncs->EndFrame)(win)


#define	_dxf_READ_APPROX_BACKSTORE(win, camw, camh) \
       (*_pFuncs->ReadApproxBackstore)(win, camw, camh)
  /*
   *  DXRead pixels from the main graphics window into the hardware saveunder
   *  buffer (SAVE_BUF). camw and camh are the DX camera resolutions in width
   *  and height; these may differ from the graphics window dimensions.
   */

#define	_dxf_WRITE_APPROX_BACKSTORE(win, camw, camh) \
       (*_pFuncs->WriteApproxBackstore)(win, camw, camh)
  /*
   *  DXWrite pixels from hardware saveunder buffer (SAVE_BUF) into the main
   *  graphics window.  camw and camh are the DX camera resolutions in width
   *  and height; these may differ from the graphics window dimensions.
   */

#define	_dxf_INIT_RENDER_PASS(win, bufferMode, zBufferMode) \
  (*_pFuncs->InitRenderPass)(win, bufferMode, zBufferMode)
/*#if defined(DX_NATIVE_WINDOWS) */
#define _dxf_END_RENDER_PASS(win) (*_pFuncs->EndRenderPass)(win)
/* #endif */
  /*
   *  Clear the screen and prepare for writing a rendered image.
   */
  
#define	_dxf_MAKE_PROJECTION_MATRIX(projection, width, aspect, Near, Far, M)\
       (*_pFuncs->MakeProjectionMatrix)(projection, width, aspect, Near, Far, M)
				     
  /*
   *  Return a projection matrix for a simple orthographic or perspective
   *  camera model (no oblique projections).  This matrix projects view
   *  coordinates into a normalized projection coordinate system spanning
   *  -1..1 in X and Y.
   *
   *  The projection parameter is 1 for perspecitve views and 0 for
   *  orthogonal views.  Width is in world units for orthogonal views.
   *  For perspective views, the width parameter is the field of view
   *  expressed as the ratio of field width to eye distance, or twice the
   *  tangent of half the angle of view.  Aspect is the height of the
   *  window expressed as a fraction of the window width.
   *
   *  The near and far parameters are distances from the view coordinate
   *  origin along the line of sight.  The Z clipping planes are at -near
   *  and -far since we are using a right-handed coordinate system.  For
   *  perspective, near and far must be positive in order to avoid drawing
   *  objects at or behind the eyepoint depth.
   *
   *  The near and far parameters should be mapped to whatever normalized
   *  depths are required by the target API and hardware.  For example, GL
   *  programs on IBM and SGI hardware need a normalized range of -1..1 in
   *  Z, while Starbase on the HP CRX cards need a normalized range of
   *  1..0.  Perspective depth can be handled as described in
   *  Newman/Sproull Chapter 23, Equation 23-2.
   *
   *  DX requires projection matrices with enough precision to accurately
   *  render view angles of 1/1000 of a degree.
   *
   *  The GL and Starbase instances of this function compute the
   *  projection matrix in software, since the GL ortho() and
   *  perspective() functions are limited in precision, and there is no
   *  way to retrieve the projection matrix in Starbase.  Other
   *  implementations may use whatever API calls are available subject to
   *  these requirements.
   */

#define	_dxf_SET_ORTHO_PROJECTION(ctx, width, aspect, Near, Far) \
       (*_pFuncs->SetOrthoProjection)(ctx, width, aspect, Near, Far)
  /*
   *  Set up an orthographic view projection and load it into the hardware.
   */

#define	_dxf_SET_PERSPECTIVE_PROJECTION(ctx, xfov, aspect, Near, Far) \
       (*_pFuncs->SetPerspectiveProjection)(ctx, xfov, aspect, Near, Far)
  /*
   *  Set up a perspective projection and load it into the hardware.
   */

#define _dxf_GET_PROJECTION(ctx, M) \
   ((!_pFuncs->GetProjection) ? OK : \
      (_pfuncs->GetProjection)(ctx, M))

#define	_dxf_SET_VIEWPORT(ctx, left, right, bottom, top) \
       (*_pFuncs->SetViewport)(ctx, left, right, bottom, top)
  /*
   *  Set up viewport in pixels.
   */

#define _dxf_GET_MATRIX(ctx, M)      \
  ((! _pFuncs->GetMatrix)? OK : \
     (*_pFuncs->GetMatrix)(ctx, M))

#define _dxf_GET_VERSION(str)         \
   ((!_pFuncs->GetVersion) ? 0x0 : \
    (*_pFuncs->GetVersion)(str))

#define	_dxf_LOAD_MATRIX(ctx, M) (*_pFuncs->LoadMatrix)(ctx,M)
  /*
   *  Load (replace) M onto the hardware matrix stack.
   */

#define	_dxf_PUSH_MULTIPLY_MATRIX(ctx, M) (*_pFuncs->PushMultiplyMatrix)(ctx, M)
  /*
   *  Pushing and multiplying are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */
  
#define	_dxf_PUSH_REPLACE_MATRIX(ctx, M) (*_pFuncs->PushReplaceMatrix)(ctx,M)
  /*
   *  Pushing and loading are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */
  
#define	_dxf_REPLACE_VIEW_MATRIX(ctx, M) \
  (*_pFuncs->ReplaceViewMatrix)(ctx,M)
  /*
   *  Pushing and loading are combined here since some API's (such as
   *  Starbase) don't provide a separate push.
   */
  
#define	_dxf_POP_MATRIX(ctx) (*_pFuncs->PopMatrix)(ctx)
  /*
   *  Pop the top of the hardware matrix stack.
   */

#define	_dxf_SET_MATERIAL_SPECULAR(ctx, r, g, b, pow) \
       (*_pFuncs->SetMaterialSpecular)(ctx, r, g, b, pow)
  /*
   *  Set specular reflection coefficients for red, green, and blue; set
   *  specular exponent (shininess) from pow.
   */

#define _dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(ctx, on)  \
       (*_pFuncs->SetGlobalLightAttributes)(ctx, on)
  /*
   *  Set global lighting attributes.  Some API's, like IBM GL, can only
   *  set certain lighting attributes such as attenuation globally across
   *  all defined lights; these API's should do the appropriate
   *  initialization through this call.  If `on' is FALSE, turn lighting
   *  completely off.
   */

#define	_dxf_ALLOCATE_PIXEL_ARRAY(ctx, w, h) \
       (*_pFuncs->AllocatePixelArray)(ctx, w, h)
  /*
   *  Allocate storage for a pixel array.
   */

#define	_dxf_FREE_PIXEL_ARRAY(ctx, pixels) \
       (*_pFuncs->FreePixelArray)(ctx, pixels)
  /*
   *  Free pixel array storage.
   */

#define	_dxf_DEFINE_LIGHT(win, n, light) (*_pFuncs->DefineLight)(win, n, light)
  /*
   *  Define and switch on light `id' using the attributes specified by
   *  the `light' parameter.  If `light' is NULL, turn off light `id'.
   *  If more lights are defined than supported by the API, the extra
   *  lights are ignored.
   */

#define	_dxf_SET_BACKGROUND_COLOR(ctx, color) \
       (*_pFuncs->SetBackgroundColor)(ctx, color)
  /*
   *  Set the background color for the image window.
   */

#define _dxf_CLEARBUFFER(win) (*_pFuncs->ClearBuffer)(win)


#else /* OLD_PORT_LAYER_CALLS */

int
_dxfBoundingBoxDraw (tdmPortHandleP portHandle, Field f, int clip_status) ;

void
_dxfImageDraw (tdmPortHandleP portHandle, Field f, Array p) ;

void
_dxfUnconnectedPointDraw (tdmPortHandleP portHandle, Field f, 
			  int npoints, Point *points,
			  RGBColor *colors, RGBColor *color_map) ;

#ifndef STANDALONE
#ifdef PROTO_USED
/*
 *  can't use these prototypes until all DX primitives use
 *  ANSI style function declarations.
 */

int
_dxfCubeDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	     int nshapes, int *connections,
	     int colorDepPos, RGBColor *colors, RGBColor *color_map,
	     int normalDepPos, Vector *normals,
	     int opacityDepPos, float *opacities, float *opacity_map,
	     float k_ambi, float k_diff, float k_spec) ;
int
_dxfLineDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	     int nshapes, int *connections,
	     int colorDepPos, RGBColor *colors, RGBColor *color_map,
	     int normalDepPos, Vector *normals,
	     int opacityDepPos, float *opacities, float *opacity_map,
	     float k_ambi, float k_diff, float k_spec) ;
int
_dxfQmeshDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	     int nshapes, int *connections,
	     int colorDepPos, RGBColor *colors, RGBColor *color_map,
	     int normalDepPos, Vector *normals,
	     int opacityDepPos, float *opacities, float *opacity_map,
	     float k_ambi, float k_diff, float k_spec) ;
int
_dxfQuadDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	     int nshapes, int *connections,
	     int colorDepPos, RGBColor *colors, RGBColor *color_map,
	     int normalDepPos, Vector *normals,
	     int opacityDepPos, float *opacities, float *opacity_map,
	     float k_ambi, float k_diff, float k_spec) ;
int
_dxfTetraDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	     int nshapes, int *connections,
	     int colorDepPos, RGBColor *colors, RGBColor *color_map,
	     int normalDepPos, Vector *normals,
	     int opacityDepPos, float *opacities, float *opacity_map,
	     float k_ambi, float k_diff, float k_spec) ;
int
_dxfTmeshDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	      int nshapes, int *connections,
	      int colorDepend, RGBColor *colors, RGBColor *color_map,
	      int normalDepend, Vector *normals,
	      int opacityDepend, float *opacities, float *opacity_map,
	      float k_ambi, float k_diff, float k_spec) ;
int
_dxfTriDraw (tdmPortHandleP portHandle, Field f, int npoints, Point *points,
	     int nshapes, int *connections,
	     int colorDepPos, RGBColor *colors, RGBColor *color_map,
	     int normalDepPos, Vector *normals,
	     int opacityDepPos, float *opacities, float *opacity_map,
	     float k_ambi, float k_diff, float k_spec) ;

#endif /* PROTO_USED */
#endif /* STANDALONE */

Error
_dxf_DRAW_IMAGE(void* win, dxObject image, 
		translationO translation);
Error
_dxf_DRAW_OPAQUE(tdmPortHandleP portHandle, xfieldP xf, 
		 RGBColor ambientColor, int buttonUP);
Error
_dxf_DRAW_TRANSPARENT(void * globals, RGBColor * ambientColor, int buttonUP);
Error
_dxf_DRAW_CLIP(tdmPortHandleP portHandle, xfieldP xfield, int buttonDown) ;
Error
_dxf_ADD_CLIP_PLANES(void* win, Point *pt, Vector *normal, int count);
Error
_dxf_REMOVE_CLIP_PLANES(void* win, int count);
int
_dxf_GRAPHICS_NOT_AVAILABLE (char *hostnameP) ;
void*
_dxf_CREATE_WINDOW (void *globals, char *winName, int w, int h) ;
translationO
_dxf_CREATE_HW_TRANSLATION (void *win) ;
void
_dxf_DESTROY_WINDOW (void *win) ;
void
_dxf_SET_OUTPUT_WINDOW (void *win) ;
void
_dxf_SET_WINDOW_SIZE (void *win, int w, int h) ;
Error
_dxf_INIT_RENDER_MODULE (void *globals) ;
void
_dxf_SWAP_BUFFERS (void *ctx) ;
void
_dxf_SINGLE_BUFFER_MODE (void *ctx) ;
void
_dxf_DOUBLE_BUFFER_MODE (void *ctx) ;
void
_dxf_CLEAR_AREA (void *ctx, int left, int right, int bottom, int top) ;
void
_dxf_WRITE_PIXEL_RECT (void *win, unsigned char *buf, 
		       int x, int y, int w, int h) ;
int
_dxf_READ_APPROX_BACKSTORE (void *win, int camw, int camh) ;
void
_dxf_WRITE_APPROX_BACKSTORE (void *win, int camw, int camh) ;
void
_dxf_INIT_RENDER_PASS (void *win, int bufferMode, int zBufferMode) ;
void
_dxf_MAKE_PROJECTION_MATRIX (int projection, float width, float aspect,
			    float Near, float Far, register float M[4][4]) ;
void
_dxf_SET_ORTHO_PROJECTION
    (void *ctx, float width, float aspect, float Near, float Far) ;
void
_dxf_SET_PERSPECTIVE_PROJECTION
    (void *ctx, float xfov, float aspect, float Near, float Far) ;
void
_dxf_SET_VIEWPORT (void *ctx, int left, int right, int bottom, int top) ;
void
_dxf_LOAD_MATRIX (void *ctx, float M[4][4]) ;
void
_dxf_PUSH_MULTIPLY_MATRIX (void *ctx, float M[4][4]) ;
void
_dxf_PUSH_REPLACE_MATRIX (void *ctx, float M[4][4]) ;
void
_dxf_REPLACE_VIEW_MATRIX (void *ctx, float M[4][4]) ;
void
_dxf_POP_MATRIX (void *ctx) ;
void
_dxf_SET_MATERIAL_SPECULAR (void *ctx, float r, float g, float b, float pow) ;
void
_dxf_SET_GLOBAL_LIGHT_ATTRIBUTES (void *ctx, int on) ;
void *
_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height) ;
void 
_dxf_FREE_PIXEL_ARRAY (void *ctx, void *pixels) ;
int
_dxf_DEFINE_LIGHT (void *win, int n, Light light) ;
int
_dxf_SET_BACKGROUND_COLOR (void *ctx, RGBColor color);

#endif 	/* OLD_PORT_LAYER_CALLS */
#endif  /* STRUCTURES_ONLY */

#endif /* tdmPortLayer_h */
