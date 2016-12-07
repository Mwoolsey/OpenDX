/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwPortSB.c,v $
  Authors: Ellen Ball, Mark Hood

  A few Starbase differences from IBM GL:

  * Starbase uses a left-handed coordinate system by default.

  * Starbase lighting computations are done in world coordinates, while IBM
  GL does them in view coordinates.  Therefore, when Starbase goes into the
  XFORM_MODEL matrix mode necessary for lighting, the matrix stack splits at
  model/world; when IBM GL goes into MVIEWING mode for the same reason, its
  matrix stack splits at view/projection.  The tdm implementation assumes a
  view/projection split, so the Starbase behavior is overridden by loading
  only the projection component of the transform in the view_matrix() call.

  * The viewing transformation matrix in Starbase is a world-to-device
  transformation.  There seems to be no way to get its current value.

  * The Starbase view_matrix() function in REPLACE_VW usage concatenates its
  matrix arg with the current vdc-to-device matrix (set by vdc_extent()) in
  order to set the viewing transformation matrix.  However, if one then
  calls view_port() the viewing transformation matrix is unchanged.  Also,
  the viewpoint() function needs to be called to establish the eyepoint.

  * The higher level view_camera() function is friendlier, but like IBM GL,
  the camera model is likely to differ from DX in ambiguous situations such
  as the view up coincident with the view normal.  This is undesirable.  In
  addition, the left-handed default must be corrected by an immediate
  view_matrix() call in PRE_CONCAT_VW mode to effect a Z mirror, which then
  provokes the view_port() problem described above.

  * view_volume is another way to set the viewing transformation matrix, but
  doesn't seem to allow perspective projections without calling
  view_matrix() in order to set the 4th column.

  * Tdm's software matrix stack shadows IBM GL, which pushes a copy of the
  current matrix when pushmatrix() is invoked.  In Starbase a matrix must be
  explicitly supplied.  While equivalent in functionality, this difference
  causes a few quirks in the tdm matrix stack shadow, which continues to
  copy the top of the matrix stack.

  * Starbase requires that the complete light switch state for all lights
  are set at the same time.  You can't turn one off or on without knowing
  the state of the other lights!  This is something we don't keep track of
  in the tdm code.

  * Locations of positional light sources do not seem to be transformed by
  the current transformation matrix in Starbase; they must be specified in
  world coordinates.  This wouldn't be a problem except that we continue to
  use the GL notion of splitting the transformation pipeline at the
  view/projection stage, so we have to transform lights to view coordinates
  ourselves.  It may be better to just adopt the Starbase model at some
  point after we get this code working.  Impact on tdm code assumptions is
  unknown at this time.

\*---------------------------------------------------------------------------*/

/* #define REV_NORMALS */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <starbase.c.h>
#include <dl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#define STRUCTURES_ONLY
#define PRIVATE_TYPE tdmSBctx
#include "hwDeclarations.h"
#include "hwWindow.h"
#include "hwSort.h"

#include "hwMatrix.h"
#include "hwPortSB.h"
#include "wsutils.h"
#include "hwInteractor.h"
#include "sbutils.c.h"

#include "hwMemory.h"

#include "hwPortLayer.h"

#if 1
#define Object dxObject
#define Matrix dxMatrix
#include "internals.h"
#undef Object
#undef Matrix
#endif

#include "hwDebug.h"

#define MAX_SB_LIGHTS 8
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

#define DXHW_DD_VERSION 0x080000

int
_dxfCatchWinopenErrors (Display *dpy, XErrorEvent *rep) ;

int
tdmCatchSearchErrors (Display *dpy, XErrorEvent *rep) ;

void
_dxf_SET_VIEWPORT (void *ctx, int left, int right, int bottom, int top);

void 
_dxf_FREE_PIXEL_ARRAY (void *ctx, void *pixels);

int
_dxf_WRITE_PIXEL_RECT(void *win, unsigned char *buf, int x, int y,
        int w, int h) ;

extern hwTranslationP
_dxf_CREATE_HW_TRANSLATION(void *win) ;

int
_dxf_SET_BACKGROUND_COLOR (void *ctx, RGBColor c)
{
  DEFCONTEXT(ctx) ;
  ENTRY(("_dxf_SET_BACKGROUND_COLOR(0x%x, 0x%x)", ctx, c));

  c.r = pow(c.r,1./SBTRANSLATION->gamma);
  c.g = pow(c.g,1./SBTRANSLATION->gamma);
  c.b = pow(c.b,1./SBTRANSLATION->gamma);
  background_color(FILDES, 
	c.r, c.g, c.b) ;

  EXIT((""));
}


void
_dxf_SWAP_BUFFERS (void *ctx, Window w)
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SWAP_BUFFERS(0x%x)", ctx));

  if (!SINGLE_BUF)   /* For Single buffer mode */
    dbuffer_switch(FILDES, BUFFER_STATE = !BUFFER_STATE) ;

  EXIT((""));
}


void
_dxf_SET_SINGLE_BUFFER_MODE(void *ctx)
{ 
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_SINGLE_BUFFER_MODE(0x%x)", ctx));

  double_buffer(FILDES,0,PLANES) ;
  SINGLE_BUF = 1 ;

  EXIT((""));
}


void
_dxf_SET_DOUBLE_BUFFER_MODE(void *ctx)
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_DOUBLE_BUFFER_MODE(0x%x)", ctx));

  double_buffer(FILDES,TRUE|INIT,PLANES/2) ;
  if (PLANES == 48) BUFFER_STATE = 1 ; /*Fix for Black Image Window CRX48Z*/
  if (PLANES == 24) BUFFER_STATE = 0 ;
  if (PLANES == 16) BUFFER_STATE = 1 ;

  BUFFER_CONFIG_MODE = BCKBUFFER ; /* The Back Buffer should be write enabled */
  SINGLE_BUF = 0 ;

  EXIT((""));
}


void
_dxf_CLEAR_AREA(void *ctx, int left, int right, int bottom, int top)
{
  DEFCONTEXT(ctx) ;
  int	vp_left, vp_right, vp_bottom, vp_top;

  ENTRY(("_dxf_CLEAR_AREA(0x%x, %d, %d, %d, %d)",ctx, left, right, bottom, top));
 
  vp_left    = VP_LEFT;
  vp_right   = VP_RIGHT;
  vp_bottom  = VP_BOTTOM;
  vp_top     = VP_TOP;

  _dxf_SET_VIEWPORT(ctx,left,right,bottom,top);
  clear_view_surface(FILDES) ;

  /* reset the original viewport */
  _dxf_SET_VIEWPORT(ctx, vp_left, vp_right, vp_bottom, vp_top);

  EXIT((""));
}


void
_dxf_MAKE_PROJECTION_MATRIX (int projection, float width, float aspect,
			    float near, float far, register float M[4][4])
{
  /*
   *  Compute a projection matrix for a simple orthographic or perspective
   *  camera model (no oblique projections).  This matrix projects view
   *  coordinates into a normalized projection coordinate system spanning
   *  [-1..1] in X, Y, and Z.
   *
   *  The near and far parameters are distances from the view coordinate
   *  origin along the line of sight.  The Z clipping planes are at -near
   *  and -far since we are using a right-handed coordinate system.  For
   *  perspective, near and far must be positive in order to avoid drawing
   *  objects at or behind the eyepoint depth.
   *
   *  The perspective depth scaling is handled as in Newman & Sproull
   *  Chap. 23, except for their left-handed orientation and a different
   *  range for the projection depth [D..F].  In Starbase we will map
   *  [D..F] to [1..0] in projection coordinates.
   *
   *  In the non-oblique perspective matrix of Equation 23-4,
   *  Zs = (Ze*M33 + M43) / (Ze*M34)
   *     = M33/M34 + (M43/M34)/Ze
   *     = a + b/Ze
   *
   *  In that formulation, M34 = S/D = fov/2.  In other implementations of
   *  this routine (eg, XGL), we use M34 = 1, so that M33 = a and M43 = b
   *  and the M11 and M22 elements are the same for the corresponding
   *  ortho projection.  Starbase seems to prefer keeping the fov
   *  coefficient in the M34 element.
   *
   *  Substituting D and F for Ze in Equation 23-2 (Zs = a +b/Ze) produces
   *  a + b/D = 1 and a + b/F = 0.  Solving results in a = 1/(1 - F/D)
   *  and b = -F/(1 - F/D).
   */

  ENTRY(("_dxf_MAKE_PROJECTION_MATRIX(%d, %f, %f, %f, %f, 0x%x)",
	 projection, width, aspect, near, far, M));

  /*M[0][0] = 1 ;*/   M[0][1] = 0 ;     M[0][2] = 0 ; M[0][3] = 0 ;
    M[1][0] = 0 ;   /*M[1][1] = 1 ;*/   M[1][2] = 0 ; M[1][3] = 0 ;
    M[2][0] = 0 ;     M[2][1] = 0 ;   /*M[2][2] = 1 ; M[2][3] = 0 ;*/
    M[3][0] = 0 ;     M[3][1] = 0 ;   /*M[3][2] = 0 ; M[3][3] = 1 ;*/

  far = -far ;
  near = -near ;

  if (projection)
    {
      /* perspective */
      float alpha =    1 / (1 - far/near) ;
      float beta  = -far / (1 - far/near) ;

      M[0][0] = 1 ;
      M[1][1] = 1/aspect ;

      /* negate matrix elements to force w positive */
      M[2][2] = -(width/2)*alpha ;
      M[3][2] = -(width/2)*beta ;
      M[2][3] = -(width/2) ;
      M[3][3] =  0 ;
    }
  else
    {
      /* orthographic */
      M[0][0] =  1/(width/2) ;
      M[1][1] =  M[0][0]/aspect ;

      M[2][2] = -1 / (far-near) ;
      M[3][2] =  1 + (near / (far-near)) ;
      M[2][3] =  0 ;
      M[3][3] =  1 ;
    }

  EXIT((""));
}


static void
_setProjection (int fildes, int projection,
		float width, float aspect, float near, float far)
{
  float M[4][4] ;

  ENTRY(("_setProjection(%d, %d, %f, %f, %f, %f)",
	 fildes, projection, width, aspect, near, far));

  if (near == far) {	
    PRINT(("near = far setting ERROR"));
    /* near clip is equal to far clip plane */
    DXSetError (ERROR_BAD_PARAMETER, "#13730") ;
    far += far/1000 ;
  }

  /*
   *  Set up a right-handed virtual device coordinate (VDC) system which
   *  spans -1..1 in X and Y and 1..0 in Z.  Use mapping_mode() to map the
   *  entire device coordinate range (image window) to the VDC extent,
   *  since we will supply a projection matrix which compensates for the
   *  distortion.
   */

  mapping_mode (fildes, DISTORT /*anisotropic*/) ;
  vdc_extent (fildes, -1 /*xmin*/, -1 /*ymin*/, 1,   /*lower-left front*/
	               1 /*xmax*/,  1 /*ymax*/, 0) ; /*upper-right back*/
    
  /*
   *  Make Starbase's model/world transformation pipeline act like GL's
   *  view/projection pipeline split by loading only the projection
   *  component into the viewing matrix.
   *
   *  Note: this maps the GL (and tdm) notion of view coordinates into
   *  Starbase's world coordinates.  This affects calls to Starbase
   *  functions such as viewpoint(), light_source(), and
   *  transform_points(), which expect positions expressed in world
   *  coordinates.
   */
  
  _dxf_MAKE_PROJECTION_MATRIX (projection, width, aspect, near, far, M) ;
  view_matrix3d (fildes, M, REPLACE_VW) ;
  PRINT(("called view_matrix3d() with matrix"));
  MPRINT(M) ;

  /*
   *  Turn front clip plane off!  For unknown reasons, keeping the default
   *  of TRUE causes clipping to occur inappropriately.  Starbase still
   *  seems to clip to the VDC extent specified above even when both clip
   *  planes are turned off, which is exactly what is wanted anyway, since
   *  the projection matrix scales the near and far distances to [1..0].
   *
   *  An attempt was made to use the GL range of [1..-1] for perspective
   *  depth in VDC, but that didn't work for tight bounding boxes.  The
   *  clipping hardware apparently clips 0 < Z < W before the perspective
   *  divide, but seems unable to handle clipping against -W < Z < W.
   *
   *  If you have lots of time, and want to straighten this out, try
   *  looking at _dxfInitRenderObject() in tdmInitScreen.c and the various
   *  calls to _dxf_MAKE_PROJECTION_MATRIX(), vdc_extent(), clip_depth(),
   *  mapping_mode(), and set_p1_p2() in this and previous versions of
   *  this file.  Speculations: setting DISTORT in mapping_mode() perhaps
   *  munges clip planes; right-handed coordinate systems might confuse
   *  the clipping hardware?
   */
  
  depth_indicator (fildes, FALSE, FALSE) ;
  EXIT((""));
}

static void
_dxf_REPLACE_VIEW_MATRIX(void *ctx, float M[4][4])
{
  float M2[4][4] ;
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_REPLACE_VIEW_MATRIX(0x%x, 0x%x)", ctx, M));
  MPRINT(M);

  if (M[0][0]==1.0 && M[0][1]==0.0 && M[0][2]==0.0 && M[0][3]==0.0 &&
      M[1][0]==0.0 && M[1][1]==1.0 && M[1][2]==0.0 && M[1][3]==0.0 &&
      M[2][0]==0.0 && M[2][1]==0.0 && M[2][2]==1.0 && M[2][3]==0.0 &&
      M[3][0]==0.0 && M[3][1]==0.0 && M[3][2]==0.0 && M[3][3]==1.0)
    {
      /*
       *  Here is a hideous hack for screen objects...
       *  Default Z for M is -1,+1: we need 0,+1 here, so scale and translate.
       */
      
      PRINT(("received screen object identity matrix..."));

      COPYMATRIX (M2, M) ;
      M2[2][2] *= -0.5 ;
      M2[3][2] = M2[3][2] * (-0.5) + 0.5 ;
      
      view_matrix3d (FILDES, M2, REPLACE_VW) ;
      PRINT(("called view_matrix3d() with:"));
      MPRINT(M2) ;
    }
  else
      view_matrix3d (FILDES, M, REPLACE_VW) ;

  EXIT((""));
}


void
_dxf_SET_ORTHO_PROJECTION (void *ctx, float width, float aspect,
			   float near, float far)
{
  /*
   *  Set up an orthographic view projection.
   */

  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_ORTHO_PROJECTION(0x%x, %f, %f, %f, %f)",
	 ctx, width, aspect, near, far));

  _setProjection (FILDES, 0, width, aspect, near, far) ;

  /*
   *  Set viewpoint at infinity along +Z view coordinate axis for specular
   *  reflection computations and backface culling.
   */

  viewpoint (FILDES, DIRECTIONAL, 0, 0, 1) ;
  EXIT((""));
}


void
_dxf_SET_PERSPECTIVE_PROJECTION (void *ctx, float fov, float aspect,
				 float near, float far)
{
  /*
   *  Set up a perspective projection.  
   */

  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_SET_PERSPECTIVE_PROJECTION(0x%x, %f, %f, %f %f)",
	 ctx, fov, aspect, near, far));

  _setProjection (FILDES, 1, fov, aspect, near, far) ;

  /*
   *  Backface culling and rendering require a positional viewpoint for
   *  perspective views.  However, for performance, use a directional
   *  viewpoint for specularity computations.  They must be set in the
   *  following order in order to achieve the desired result (see page
   *  13-18 in the Starbase Graphics Techniques manual).
   */
  
  viewpoint (FILDES, POSITIONAL, 0, 0, 0) ;
  viewpoint (FILDES, DIRECTIONAL, 0, 0, 1) ;

  EXIT((""));
}

void
_dxf_SET_VIEWPORT (void *ctx, int left, int right, int bottom, int top)
{
  DEFCONTEXT(ctx) ;
  int cmap_size ;
  float physical_limits[2][3], resolution[3], p1[3], p2[3], width, height ;
  float fleft, fright, fbottom, ftop ;

  ENTRY(("_dxf_SET_VIEWPORT(0x%x, %d, %d, %d, %d)",
	 ctx, left, right, bottom, top));

  /*
   *  Set up viewport in pixels.  Starbase makes this inconvenient since
   *  set_p1_p2() doesn't accept pixels: it wants the viewport corners in
   *  either a normalized representation or in millimeters.
   *
   *  Starbase device coordinates of [0,0] denote the upper-left corner of
   *  an X window, while physical_limits are returned as a
   *  lower-left/upper-right pair.  The DX/TDM/GL viewing model assigns
   *  device coordinates of [0,0] to the lower-left corner.
   *
   *  This is further complicated by the fact that the physical size of
   *  the window as known to Starbase is not necessarily what DX thinks it
   *  is. The DX window size is actually the size of the parent UI window,
   *  which clips the physical Starbase window.  The DX window size is
   *  recorded in WIN_HEIGHT.
   *
   *  Pixels in Starbase seem not be centered on their coordinates.  A
   *  pixel appears to extend to the right and down, so that the location
   *  denoted by its coordinates is at the upper left corner of the area
   *  actually covered by the pixel.
   */

  inquire_sizes (FILDES, physical_limits, resolution, p1, p2, &cmap_size) ;
  PRINT(("physical_limits ll"));
  VPRINT(physical_limits[0]) ;
  PRINT(("physical_limits ur"));
  VPRINT(physical_limits[1]) ;

  width  = physical_limits[1][0] + 1 ;
  height = physical_limits[0][1] + 1 ;

  left =   MAX(0, left) ;   right = MAX(left+1, right) ;
  bottom = MAX(0, bottom) ; top =   MAX(bottom+1, top) ;

  fleft =       left / width ;                         /* left edge of min */
  fright = (right+1) / width ;                         /* right edge of max */

  ftop =    1 - ((((WIN_HEIGHT-1) - top)) / height) ;  /* top edge of min */
  fbottom = 1 - ((( WIN_HEIGHT - bottom)) / height) ;  /* bottom edge of max */

  fleft =   MAX(0, fleft) ;   fright = MIN(1, fright) ;
  fbottom = MAX(0, fbottom) ; ftop =   MIN(1, ftop) ;

  PRINT(("fractional left: %f, right: %f, bottom: %f, top: %f",
	 fleft, fright, fbottom, ftop));

  set_p1_p2 (FILDES, FRACTIONAL, fleft, fbottom, 0, fright, ftop, 1) ;

  VP_LEFT   = left ;
  VP_RIGHT  = right ;
  VP_BOTTOM = bottom ;
  VP_TOP    = top ; 
  EXIT((""));
}


void
_dxf_LOAD_MATRIX (void *ctx, float M[4][4])
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_LOAD_MATRIX(0x%x, 0x%x)", ctx, M));
  MPRINT(M) ; 

  replace_matrix3d (FILDES, M) ;

  EXIT((""));
}

void
_dxf_PUSH_MULTIPLY_MATRIX (void *ctx, float M[4][4])
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_PUSH_MULTIPLY_MATRIX(0x%x, 0x%x)", ctx, M));
  MPRINT(M) ; 

  concat_transformation3d (FILDES, M, PRE, PUSH) ;

  EXIT((""));
}

void
_dxf_PUSH_REPLACE_MATRIX (void *ctx, float M[4][4])
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_PUSH_REPLACE_MATRIX(0x%x, 0x%x)", ctx, M));
  MPRINT(M) ; 

  push_matrix3d (FILDES, M) ;

  EXIT((""));
}

void
_dxf_POP_MATRIX (void *ctx)
{
  DEFCONTEXT(ctx) ;

  ENTRY(("_dxf_POP_MATRIX(0x%x)", ctx));
  
  pop_matrix(FILDES) ;

  EXIT((""));
}


void 
_dxf_SET_MATERIAL_SPECULAR (void *ctx, float r, float g, float b, float pow)
{
  DEFCONTEXT(ctx) ;
  int shine ;

  ENTRY(("_dxf_SET_MATERIAL_SPECULAR(0x%x, %f, %f, %f, %f)",
	 ctx, r, g, b, pow));
  
  /* need broader specular highlights */
  pow /= 2 ;

  /* limit specular exponent to Starbase range 1..16383 */
  shine = (int) (pow < 1 ? 1 : (pow > 16383 ? 16383: pow)) ;

  /* turn on specular computations, set r, g, b coefficients */
  surface_model (FILDES, TRUE /*compute specular*/, shine, r, g, b) ;

  EXIT((""));
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

  ENTRY(("_dxf_SET_GLOBAL_LIGHT_ATTRIBUTES(0x%x, %d)", ctx, on));

  if (on)
    {
#ifdef REV_NORMALS      
      /*
       *  Allow reversed normals to be lit as if they were facing the
       *  light source.  This makes the inside-out isosurface normals look
       *  better, but at the expense of making everything look like it is
       *  lit by two light sources 180 degrees apart, and a 30%
       *  performance penalty or so.  The backface attr parameter is
       *  useless since the inside-out isosurfaces still have vertices
       *  ordered conventionally.
       */
      bf_control(FILDES, TRUE, FALSE) ;
#endif
    }
  else
    {
      /* turn all lighting off */
      light_switch(FILDES, 0x00);
      LIGHTSTATE = 0x00 ;
    }

  EXIT((""));
}


void *
_dxf_ALLOCATE_PIXEL_ARRAY (void *ctx, int width, int height)
{
  /* allocate an array with room for npixels pixels */
  DEFCONTEXT(ctx) ;
  register tdmImageSBP tdmImages ;
  int npixels ;

  ENTRY(("_dxf_ALLOCATE_PIXEL_ARRAY(0x%x, %d, %d)", ctx, width, height));
  
  tdmImages = (tdmImageSBP) tdmAllocateLocal(sizeof(tdmImageSBT)) ;

  if (!tdmImages)
    {
      DXSetError (ERROR_INTERNAL, "#13000") ;
      EXIT(("AllocateLocal returned null"));
      return 0 ;
    }

  npixels = width * height ;
  tdmImages->saveBufRed = 0 ;
  tdmImages->saveBufGreen = 0 ;
  tdmImages->saveBufBlue = 0 ;

  /* For CRX24 and CRX24Z...3 bank devices */
  if ( (PLANES == 24) || (PLANES == 48) )    /* CRX48Z */
    {
      /* hp does 4 byte acceses */
      npixels = (npixels + 3) & ~3;
      tdmImages->saveBufRed = (unsigned char *) 
	  tdmAllocateLocal(sizeof(unsigned char) * npixels) ;
      if (!tdmImages->saveBufRed) goto error ;

      tdmImages->saveBufGreen = (unsigned char *) 
	  tdmAllocateLocal(sizeof(unsigned char) * npixels) ;
      if (!tdmImages->saveBufGreen) goto error ;
    }

  /* For CRX, CRX24, CRX24Z, CRX48Z */
  tdmImages->saveBufBlue = (unsigned char *)
      tdmAllocateLocal(sizeof(unsigned char) * npixels) ;  
  if (!tdmImages->saveBufBlue) goto error ;

  EXIT((""));
  return (void *) tdmImages ;

 error:
  DXSetError (ERROR_INTERNAL, "#13000") ;
  _dxf_FREE_PIXEL_ARRAY (ctx, (void *)tdmImages) ;
  EXIT(("ERROR"));
  return 0 ;
}

void 
_dxf_FREE_PIXEL_ARRAY (void *ctx, void *pixels)
{
  /* free a pixel array */
  DEFCONTEXT(ctx) ;
  register tdmImageSBP tdmImages = (tdmImageSBP) pixels ;

  ENTRY(("_dxf_FREE_PIXEL_ARRAY(0x%x, 0x%x)", ctx, pixels));
  
  /* For CRX24 and CRX24Z...3 bank devices */
  if ( (PLANES == 24) || (PLANES == 48) )    /* CRX48Z */
    {
      if (tdmImages->saveBufRed)
	  tdmFree((unsigned char*) tdmImages->saveBufRed) ;
      tdmImages->saveBufRed = (unsigned char *) NULL ;

      if (tdmImages->saveBufGreen)
	  tdmFree((unsigned char *) tdmImages->saveBufGreen) ;
      tdmImages->saveBufGreen = (unsigned char *) NULL ;
    }

  /* For CRX, CRX24, CRX24Z, CRX48Z */
  if (tdmImages->saveBufBlue)
      tdmFree((unsigned char *) tdmImages->saveBufBlue) ;
  tdmImages->saveBufBlue = (unsigned char *) NULL ;

  tdmFree(tdmImages) ;

  EXIT((""));
}

typedef struct argbS
{
   char a, b, g, r;
} argbT,*argbP;

Error
_dxf_DRAW_IMAGE(void* win, dxObject image, hwTranslationP dummy)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  int           x,y,camw,camh ;
  int            i, n ;
  unsigned long *pixels = NULL;
  void * translation = SBTRANSLATION;

  ENTRY(("_dxf_DRAW_IMAGE(0x%x, 0x%x, 0x%x)", win, image, dummy));
 
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
      ((argbP) SW_BUF)[i].r = (unsigned char) ((pixels[i]&0xFF)) ;
      ((argbP) SW_BUF)[i].g = (unsigned char)(((pixels[i] >> 8)&0xFF)) ;
      ((argbP) SW_BUF)[i].b = (unsigned char)(((pixels[i] >> 16)&0xFF)) ;
  }
 
  SW_BUF_CURRENT = TRUE ;
 
 /* Check for error */
  if(!(_dxf_WRITE_PIXEL_RECT(win,NULL,0,0,camw,camh))) goto error ;
 
  if (pixels) {
        tdmFree(pixels) ;
        pixels = NULL ;
        }
  EXIT((""));
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
 
  EXIT(("ERROR"));
  return ERROR ;
}

extern hwTranslationP
_dxf_CREATE_HW_TRANSLATION(void *win)
{
  DEFWINDATA(win) ;
  hwTranslationP        ret;
  int           i;

  ENTRY(("_dxf_CREATE_HW_TRANSLATION(0x%x)", win));
  
  if (! (ret = (hwTranslationP) tdmAllocate(sizeof(translationT)))) {
    EXIT(("NULL"));
    return NULL;
  }

  ret->dpy = DPY;
  ret->visual = NULL;
  ret->cmap = (void *)CLRMAP;
  ret->depth = 24;
  ret->invertY = FALSE ;
  ret->gamma = 2.0;
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

  EXIT(("ret = 0x%x", ret));
  return ret;
}

Error
_dxf_INIT_RENDER_MODULE (void *globals)
{
  /*
   *  This routine is called once, after the graphics window has been
   *  created.  It's function is loosely defined to encompass all the
   *  `first time' initialization of the target graphics API.
   */

  int i ;
  DEFGLOBALDATA(globals) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_INIT_RENDER_MODULE(0x%x)", globals));

  /* mark the save buffer unallocated */
  SAVE_BUF = 0 ;
  SAVE_BUF_SIZE = 0 ;

  /* mark the single buffer flag to off */ 
  SINGLE_BUF = 0 ;

  /* initialize buffer state */
  if (PLANES == 48) BUFFER_STATE = 1 ;
  if (PLANES == 24) BUFFER_STATE = 0 ;
  if (PLANES == 16) BUFFER_STATE = 1 ;

  BUFFER_CONFIG_MODE = BCKBUFFER ;

  /* Starbase complains about an empty stack if next call is not made */
  push_matrix3d (FILDES, (float (*)[4])identity) ;

  EXIT(("TRUE"));
  return TRUE ;
}


void
_dxf_INIT_RENDER_PASS (void *win, int bufferMode, int zBufferMode)
{
  /*
   *  Clear the window and prepare for a rendering pass.
   */
  
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_INIT_RENDER_PASS(0x%x, %d, %d)", win, bufferMode, zBufferMode));
  
  _dxfChangeBufferMode (win, bufferMode) ;
  SAVE_BUF_VALID = FALSE ;

  EXIT((""));
}

int
_dxf_READ_APPROX_BACKSTORE(void *win, int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  
  ENTRY(("_dxf_READ_APPROX_BACKSTORE(0x%x, %d, %d)", win, camw, camh));
  
  if (SAVE_BUF_SIZE !=  camw * camh)
    {
      if (SAVE_BUF) 
	{
	  _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
	  SAVE_BUF = (void *) NULL ;
	}
      
      SAVE_BUF_SIZE = camw * camh ;
      PRINT(("SAVE_BUF_SIZE = %d",SAVE_BUF_SIZE));
      
      /* allocate a buffer */
      if (!(SAVE_BUF = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, camw, camh)))
	{
	  EXIT(("out of memory"));
	  return 0 ;
	}
    }
  
  /* read entire extent of image from frame buffer */
  _dxf_READ_BUFFER (PORT_CTX, 0, 0, camw-1, camh-1, SAVE_BUF) ;
  EXIT((""));
  return 1 ;
}
  

void
_dxf_WRITE_APPROX_BACKSTORE(void *win, int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_WRITE_APPROX_BACKSTORE(0x%x, %d, %d)", win, camw, camh));
 
  /* write to front buffer */
  double_buffer (FILDES, TRUE|DFRONT, PLANES/2) ;
  _dxf_WRITE_BUFFER (PORT_CTX, 0, 0, camw-1, camh-1, SAVE_BUF) ;

  /* redraw interactor echos; can't save them in SAVE_BUF */
  _dxfRedrawInteractorEchos(INTERACTOR_DATA) ;

  /* restore write to back buffer */
  double_buffer(FILDES, TRUE, PLANES/2) ;

  EXIT((""));
}


void
_dxf_SET_WINDOW_SIZE (void *win, int w, int h)
{ 
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_SET_WINDOW_SIZE(0x%x, %d, %d)", win, w, h));

  /* record DX idea of window height */
  WIN_HEIGHT = h ;

#if 0
  /*
   *  If UI parent window is present, we can rescale by setting a new
   *  viewport; if no UI, we don't need to rescale (in fact, Starbase won't
   *  let us:  see tdmWinSB.c)
   */
#else
 /* It seems like Starbase will let us resize, and this helps the script
  * resize problem.  HOWEVER...we still have a set_p1_p2 error in
  * SET_VIEWPORT until we actually implement reparenting of the Starbase 
  * window for script mode. 
  */
  XResizeWindow (DPY, XWINID, w-1, h-1) ;
  XSync (DPY, False) ;
#endif

  if (PARENT_WINDOW)
      _dxf_SET_VIEWPORT (PORT_CTX, 0, w-1, 0, h-1) ;

  EXIT((""));
}


void
_dxf_SET_OUTPUT_WINDOW(void *win, Window w)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_SET_OUTPUT_WINDOW(0x%x)", win));

  /* gl uses winset(ctx), but Starbase doesn't need this */

  EXIT((""));
}


void
_dxf_DESTROY_WINDOW (void *win)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_DESTROY_WINDOW(0x%x)", win));

  if (XWINID)
      gclose(FILDES) ;

  if (GNOBE_BUF)
    {   
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, GNOBE_BUF) ;
      GNOBE_BUF = (void *) NULL ;
    }

  if (SW_BUF) {
	tdmFree(SW_BUF) ;
	SW_BUF = NULL ;
        SW_BUF_SIZE = 0 ;
	}

  if (SAVE_BUF)
    {
      _dxf_FREE_PIXEL_ARRAY (PORT_CTX, SAVE_BUF) ;
      SAVE_BUF = (void *) NULL ;
    }

  if (SBTRANSLATION)
    {	
	tdmFree(SBTRANSLATION) ;
	SBTRANSLATION = NULL ;
    }

  if (PORT_CTX) {
      tdmFree(PORT_CTX) ;
      PORT_CTX = NULL ; 
      }

  EXIT((""));
}

int 
_dxf_WRITE_PIXEL_RECT(void *win, unsigned char *foo, int x, int y,
	int camw, int camh)
{
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;
  tdmImageSBP preciseBuffer = NULL ;
  int i, j, k, npixels, index ;

  ENTRY(("_dxf_WRITE_PIXEL_RECT(0x%x, 0x%x, %d, %d, %d, %d)",
	 win, foo, x, y, camw, camh));

  if(!(preciseBuffer = _dxf_ALLOCATE_PIXEL_ARRAY (PORT_CTX, camw, camh))) 
     goto error ;

  index = camw*camh - camw ;

  for (i=0; i < camw*camh ; i+=camw) {
    for (j=0; j < camw; j++) {
        k = index - i + j ;
        preciseBuffer->saveBufRed[k] =
                (unsigned char) ((argbP)SW_BUF)[i+j].r ;
        preciseBuffer->saveBufGreen[k] =
                (unsigned char) ((argbP)SW_BUF)[i+j].g ;
        preciseBuffer->saveBufBlue[k] =
                (unsigned char) ((argbP)SW_BUF)[i+j].b ;
    }
  }

  _dxf_WRITE_BUFFER (PORT_CTX, 0, 0, camw-1, camh-1, preciseBuffer) ;

  if (preciseBuffer) 
	_dxf_FREE_PIXEL_ARRAY (PORT_CTX, preciseBuffer) ;

  EXIT(("OK"));
  return OK ;

error:
  EXIT(("ERROR"));
  return 0 ; 
}


int 
_dxf_DEFINE_LIGHT (void *win, int n, Light light)
{
  /*
   *  Define and switch on light `id' using the attributes specified by
   *  the DX `light' parameter.  If `light' is NULL, turn off light `id'.
   *  If more lights are defined than supported by the API, the extra
   *  lights are ignored.
   *
   *  Note that Starbase does not transform lights by the current
   *  transformation matrix as GL does, and requires that the lights be
   *  expressed in world coordinates.  Since we have loaded only the
   *  projection component of the view with the view_matrix() call, we
   *  must convert the DX world coordinates into view coordinates
   *  ourselves.
   */
  
  RGBColor color;
  Vector   direction;
  float    iGamma = 1.0;

  float view[4][4], x, y, z ;
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE);

  ENTRY(("_dxf_DEFINE_LIGHT(0x%x, %d, %d)", win, n, light));
  
  if (n > MAX_SB_LIGHTS)
    {
      /* SBrender:  only first %d lights are used */
      DXWarning("#5190", MAX_SB_LIGHTS);
      goto error ;
    }
  
  if (! light) {
      /* turn off light n */
      if (n == AMBIENT_ID) 
	 /* ambient is always light 0 in Starbase */
	 LIGHTSTATE &= ~1 ;
      else 
	LIGHTSTATE &= ~(1 << n) ;
      goto done ;
      }

  if(DXQueryCameraDistantLight(light,&direction,&color))
  {
    PRINT(("directional"));
    SPRINT(direction) ;
    PRINT(("color"));
    CPRINT(color) ;
      
      if (n == 0)
	{
	  /* actually OK in Starbase, but illegal in GL and therefore tdm */
	  /* attempt to redefine light 0 */
	  DXErrorGoto(ERROR_INTERNAL, "#13740");
	}
      
      light_source
	(FILDES, n, DIRECTIONAL,
	 color.r, color.g, color.b, 
	 direction.x,direction.y,direction.z) ;

      LIGHTSTATE |= (1 << n) ;
  }
  else if (DXQueryDistantLight(light,&direction,&color))
  {      
      
      if (n == 0)
	{
	  /* actually OK in Starbase, but illegal in GL and therefore tdm */
	  /* attempt to redefine light 0 */
	  DXErrorGoto(ERROR_INTERNAL, "#13740");
	}
      
      /* transform light direction to view coordinates */
      _dxfGetCompositeMatrix (MATRIX_STACK, view) ;
      x = direction.x*view[0][0] +
	  direction.y*view[1][0] +
	  direction.z*view[2][0] ;
      y = direction.x*view[0][1] +
	  direction.y*view[1][1] +
	  direction.z*view[2][1] ;
      z = direction.x*view[0][2] +
	  direction.y*view[1][2] +
	  direction.z*view[2][2] ;
      light_source
	(FILDES, n, DIRECTIONAL,
	 color.r, color.g, color.b, x, y, z) ;

      LIGHTSTATE |= (1 << n) ;
  }
  else if(DXQueryAmbientLight(light,&color))
  {
    PRINT(("ambient color"));
    CPRINT(color) ;
      iGamma = 1./SBTRANSLATION->gamma;
      /* ambient light is light 0 in Starbase */
      light_ambient (FILDES, pow(color.r,iGamma), pow(color.g,iGamma),
		     pow(color.b,iGamma)) ;

      AMBIENT_ID = n ;
      LIGHTSTATE |= 1 ;
  }
  else
  {
    PRINT(("invalid light"));
      DXErrorGoto(ERROR_DATA_INVALID, "#13750");
  }
  goto done ;

 done:
  light_switch (FILDES, LIGHTSTATE) ;
  EXIT(("OK"));
  return TRUE ;

 error:
  EXIT(("ERROR"));
  return FALSE ;
}

#ifndef STANDALONE

void
_dxf_INIT_SW_RENDER_PASS (void *win)
{
  /*
   *  Clear the screen and prepare for writing a software-rendered image.
   */
  
  DEFWINDATA(win) ;
  DEFPORT(PORT_HANDLE) ;

  ENTRY(("_dxf_INIT_SW_RENDER_PASS(0x%x)", win));

  /* Make sure we go into the S/W render without error conditions set */
  DXResetError() ;
  clear_view_surface(FILDES) ;

  EXIT((""));
}

#endif

Error
_dxf_ADD_CLIP_PLANE(void* win, Point *pt, Vector *normal, int count)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE);
  int	i;
  static float	I[4][4]={1.0,0.0,0.0,0.0,
			 0.0,1.0,0.0,0.0,
			 0.0,0.0,1.0,0.0,
			 0.0,0.0,0.0,1.0};
  static float view[4][4];
  /* 6 floats per clip plane stored in ctx */
  register int	index = SBCLIPPLANECNT * 6;

  ENTRY(("_dxf_ADD_CLIP_PLANE(0x%x, 0x%x, 0x%x, %d)",
	 win, pt, normal, count));
  
  if (!count) {
    EXIT(("count == NULL"));
    return OK;
  }

  SBCLIPPLANECNT += count;

  if(SBCLIPPLANECNT  > MAX_CLIP_PLANES ) {
    /* too many clip planes specified, clip object ignored */
    DXWarning("#13890");
    goto error ;
  }

  for(i=0;i<count;i++) {
    SBCLIPPLANES[index++] = pt[i].x;
    SBCLIPPLANES[index++] = pt[i].y;
    SBCLIPPLANES[index++] = pt[i].z;
    SBCLIPPLANES[index++] = normal[i].x;
    SBCLIPPLANES[index++] = normal[i].y;
    SBCLIPPLANES[index++] = normal[i].z;
  }

  _dxfGetCompositeMatrix (MATRIX_STACK, view) ;
  push_matrix3d (FILDES, view) ;
  set_model_clip_volume(FILDES,SBCLIPPLANECNT,
			(float*)(SBCLIPPLANES),REPLACE_MCV);
  set_model_clip_indicator(FILDES,MODEL_CLIP_ON);
  pop_matrix (FILDES) ;
  
  EXIT(("OK"));
  return OK;

 error:

  EXIT(("ERROR"));
  return ERROR;
}

Error
_dxf_REMOVE_CLIP_PLANE(void* win, int count)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE);

  ENTRY(("_dxf_REMOVE_CLIP_PLANE(0x%x, %d)", win, count));
  
  if(SBCLIPPLANECNT > MAX_CLIP_PLANES) {
    SBCLIPPLANECNT -= count;
    EXIT(("SBCLIPPLANECNT > MAX"));
    return OK;
  }

  if(count > SBCLIPPLANECNT) {
    EXIT(("clip plane deleted too often"));
    DXErrorReturn (ERROR_INTERNAL,"Clip Plane Deleted Too often");
  }

  SBCLIPPLANECNT -= count;

  if(!SBCLIPPLANECNT)
    set_model_clip_indicator(FILDES,MODEL_CLIP_OFF);
  else
    set_model_clip_volume(FILDES,SBCLIPPLANECNT,
			  (float*)(SBCLIPPLANES),REPLACE_MCV);
  

  EXIT(("OK"));
  return OK;
}

static int _dxf_GET_VERSION(char ** dsoStr)
{
   static char * DXHW_LIB_FLAVOR = "Starbase";

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
    int           i, j;

    for (i = 0; i < nSorted; i = j)
	for (j = i; j < nSorted && sorted[j].xfp == sorted[i].xfp; j++)
	    _dxf_DrawOpaque(PORT_HANDLE, (xfieldP)sorted[i].xfp, (j-i),
				ambientColor, buttonUp);

    return OK;

}

extern Error 
  _dxf_DrawOpaque (tdmPortHandleP portHandle, xfieldP xf, 
		   RGBColor ambientColor, int buttonUp);
extern void*
  _dxf_CREATE_WINDOW (void *globals, char *winName, int width, int height);

int _dxf_READ_IMAGE(void *win, void *buf);

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

    clear_view_surface(FILDES) ;
}


static tdmDrawPortT _DrawPort = 
{
  /* AllocatePixelArray */       _dxf_ALLOCATE_PIXEL_ARRAY,          
  /* AddClipPlane */             _dxf_ADD_CLIP_PLANE,                          
  /* ClearArea */ 	       	 _dxf_CLEAR_AREA,   
  /* CreateTranslation */	 _dxf_CREATE_HW_TRANSLATION,                 
  /* CreateWindow */	       	 _dxf_CREATE_WINDOW,                 
  /* DefineLight */ 	       	 _dxf_DEFINE_LIGHT,                  
  /* DestroyWindow */ 	       	 _dxf_DESTROY_WINDOW,                
  /* DrawClip */		 NULL,
  /* DrawImage */                _dxf_DRAW_IMAGE,
  /* Drawopaque */		 _dxf_DrawOpaque,                           
  /* RemoveClipPlane */          _dxf_REMOVE_CLIP_PLANE,
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
  /* WritePixelRect */		 (void (*) (void* win, unsigned long *buf, 
					    int x, int y,int w, int h))
                                 _dxf_WRITE_PIXEL_RECT,
  /* DrawTransparent */          _dxf_DrawTrans,
  /* GetMatrix */                NULL,
  /* GetProjection */            NULL,
  /* GetVersion */               _dxf_GET_VERSION,
  /* ReadImage */                _dxf_READ_IMAGE,
  /* StartFrame */               _dxf_StartFrame,
  /* EndFrame */                 _dxf_EndFrame,
  /* ClearBuffer */		 _dxf_ClearBuffer
};

 
 
extern tdmInteractorEchoT _dxd_hwInteractorEchoPort;


void
_dxfUninitPortHandle(tdmPortHandleP portHandle)
{
  ENTRY(("_dxfUninitPortHandle(0x%x)", portHandle));
  
  if(portHandle) {
    portHandle->portFuncs = NULL;
    portHandle->echoFuncs = NULL;
    tdmFree(portHandle);
  }

  EXIT((""));
}
  
tdmPortHandleP
_dxfInitPortHandle(Display *dpy, char *hostname, void (*uninitP)())
{
  int  	fildes ;
  int	rtnval ;
  char  cap_flags[SIZE_OF_CAPABILITIES];
  char  *outdevice = NULL;
  Window	window ;
  int	screen ;
  int	x=0, y=0, width, height ;
  int 	transparentOverlays, numVisuals, numOverlayVisuals, numImageVisuals ;
  int   banks, planes, PlanesPerBank, OverlayPlanes ;
  int 	imageDepthToUse ;
  int	mustFreeImageColormap ;
  XVisualInfo *pVisuals ;
  OverlayInfo *pOverlayVisuals ;
  XVisualInfo **pImageVisuals ;
  Visual      *pImageVisualToUse ;
  Colormap    imageColormap ;
  tdmPortHandleP	ret;

  ENTRY(("_dxfInitPortHandle(0x%x, \"%s\", 0x%x)", dpy, hostname, uninitP));

  _dxf_setFlags(_dxf_SERVICES_FLAGS(), SF_VOLUME_BOUNDARY | SF_TMESH | SF_QMESH | SF_POLYLINES);

#ifndef DEBUG
  {
    int 	execSymbolInterface;
    void*	(*symVer)() = NULL;
    void*	(*expVer)() = NULL;
    int		ret1=0,ret2=0;
    shl_t	progH = NULL,*progHandle = &progH;
    
    /* If the hardware rendering load module requires a newer set of
     * DX exported symbols than those available in the calling dxexec
     * return NULL.
     */
    if (!(ret1 = shl_findsym((shl_t *)progHandle, "DXSymbolInterface",
		     TYPE_PROCEDURE,(long*)&symVer)) &&
	!(ret2 = shl_findsym((shl_t *)progHandle, "_dxfExportHwddVersion",
		     TYPE_PROCEDURE,(long*)&expVer))) {
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

#endif

    /* open driver to image planes */
    screen = DefaultScreen(dpy);

    /* Get all of the visual information about the screen: */
    if (GetXVisualInfo(dpy, screen, &transparentOverlays,
                       &numVisuals, &pVisuals,
                       &numOverlayVisuals, &pOverlayVisuals,
                       &numImageVisuals, &pImageVisuals))
    {
      PRINT(("Could not get visual info!!!"));
      /* Hardware rendering is unavailable on host '%s' */
      DXSetError(ERROR_BAD_PARAMETER,"#13675",hostname) ;
      goto error ;
    }

    /* Find an appropriate image planes visual: */
    if (FindImagePlanesVisual(dpy, screen, numImageVisuals, pImageVisuals,
                              SB_CMAP_TYPE_FULL, 24, FLEXIBLE,
                              &pImageVisualToUse, &imageDepthToUse))
    {
      PRINT(("Could not find a good image planes visual!!!"));
      /* Hardware rendering is unavailable on host '%s' */
      DXSetError(ERROR_BAD_PARAMETER,"#13675",hostname) ;
      goto error ;
    }

    if (pVisuals)
	XFree((char *)pVisuals) ;

    if (pImageVisuals)
	tdmFree(pImageVisuals) ;

    width = 1280 ;
    height = 1024 ;

    if (CreateImagePlanesWindow(dpy, screen,
                                RootWindow(dpy, screen),
                                x, y, width, height,
                                imageDepthToUse, pImageVisualToUse,
                                NULL, NULL, NULL, NULL,
                                &window,
                                &imageColormap, &mustFreeImageColormap))
    {
      PRINT(("Could not create the image planes window!!!"));
      /* Hardware rendering is unavailable on host '%s' */
      DXSetError(ERROR_BAD_PARAMETER,"#13675",hostname) ;
      goto error ;
    }	

   XSync(dpy,False);
   outdevice = make_X11_gopen_string(dpy, window);

  /* open with OUTDEV instead of OUTINDEV in order to use X11 input */
  if ((fildes = gopen (outdevice, OUTDEV, NULL,
		       INIT|THREE_D|MODEL_XFORM)) == -1) {
    /* Hardware rendering is unavailable on host '%s' */
    DXSetError(ERROR_BAD_PARAMETER,"#13675",hostname) ;
    goto error ;
  }
  free(outdevice);

  inquire_fb_configuration(fildes,&banks,&planes,&PlanesPerBank,
     &OverlayPlanes) ;
  if ((planes != 16) && (planes != 24) && (planes != 48) )  {
     /* Hardware rendering is unavailable on host '%s' */
     DXSetError(ERROR_BAD_PARAMETER,"#13670") ;
     goto error ;
   }

  /* Check to see if the Adapter can perform shading */
  rtnval = inquire_capabilities( fildes, SIZE_OF_CAPABILITIES, cap_flags );

  if (cap_flags[SHADING_CAPABILITIES] & IC_SHADING)  {
    gclose(fildes) ;

    ret = (tdmPortHandleP)tdmAllocateLocal(sizeof(tdmPortHandleT));

    if(ret) {
      ret->portFuncs = &_DrawPort;
      ret->echoFuncs = &_dxd_hwInteractorEchoPort;
      ret->uninitP   = _dxfUninitPortHandle;
      ret->private   = NULL;
    }

    EXIT(("ret = 0x%x", ret));
    return ret;
  }
  else {
    gclose(fildes);
    /* Hardware rendering is unavailable on host '%s' */
    DXSetError(ERROR_BAD_PARAMETER,"#13675",hostname) ;
    goto error ;
  }

error:
#if 0
/* It seems this was allocated by malloc in a shared library 
   and therefore didn't get caught and translaterd into DXAllocate.
   Since we catch free and convert it to DXFree, the following free
   will barf
 */

  if(outdevice) free(outdevice);
#endif

  EXIT(("ERROR"));
  return NULL ;
}

int _dxf_READ_IMAGE (void* win, void *buf)
{
  DEFWINDATA(win);
  DEFPORT(PORT_HANDLE) ;
  _dxf_READ_BUFFER (PORT_CTX, 0, 0, PIXW-1, PIXH-1, buf);
  return OK ;
}

