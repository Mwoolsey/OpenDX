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


#ifndef tdmPortXGL_h
#define tdmPortXGL_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwPortXGL.h,v $

  Private data structures used by the XGL implementation of DX renderer.

 $Log: hwPortXGL.h,v $
 Revision 1.3  1999/05/10 15:45:40  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:40  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:43  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:36  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:03:00  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:32:46  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:14:40  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.3  1994/03/31  15:55:22  tjm
 * added versioning
 *
 * Revision 7.2  94/03/23  16:34:30  tjm
 * added model clipping 
 * 
 * Revision 7.1  94/01/18  18:59:50  svs
 * changes since release 2.0.1
 * 
 * Revision 6.2  94/01/14  18:07:17  ellen
 * Added translation to context.
 * 
 * Revision 6.1  93/11/16  10:25:55  svs
 * ship level code, release 2.0
 * 
 * Revision 1.5  93/09/29  14:00:08  tjm
 * Fixed <PITTS21> wrong background color on sun. Two problems, negative colors
 * in a field (now clamp to 0.0) and no constant colors set on mesh primitives
 * if no normals. (now set colors)
 * 
 * 
 * Revision 1.4  93/09/28  09:40:16  tjm
 * fixed <DMCZAP71> colors are now clamped so that the maximum value for
 * r g or b is 1.0. 
 * 
 * Revision 1.3  93/09/09  17:15:58  mjh
 * add IS_2D macro
 * 
 * Revision 1.2  93/07/14  13:29:06  tjm
 * added solaris ifdefs
 * 
 * Revision 1.1  93/06/29  10:01:39  tjm
 * Initial revision
 * 
 * Revision 5.2  93/05/11  18:04:24  mjh
 * Implement screen door transparency for the GT.
 * 
 * Revision 5.1  93/03/30  14:41:58  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.11  93/03/10  16:24:58  mjh
 * increase max lights to 9 to support lights 0-8
 * 
 * Revision 5.0.2.10  93/02/01  22:22:06  mjh
 * Implement pixel read/write, gnomon and zoom box echos.  The gnomon echo is
 * no longer double-buffered as this incurs the expense of copying the 
 * display buffer to the draw buffer as part of the interactor setup, and
 * this is quite slow with the Sun GS card.  The echo is now rendered into
 * the draw buffer and then blitted into the display buffer.
 * 
 * Revision 5.0.2.9  93/01/12  19:06:53  mjh
 * fix double buffering
 * 
 * Revision 5.0.2.8  93/01/10  22:17:38  mjh
 * add lighting
 * 
 * Revision 5.0.2.7  93/01/09  00:27:08  mjh
 * frame buffer stuff
 * 
 * Revision 5.0.2.6  93/01/08  18:48:51  owens
 * got rid of X/dx conflicting Screen defs.
 * 
 * Revision 5.0.2.5  93/01/08  13:34:02  mjh
 * implement transformation pipeline
 * 
 * Revision 5.0.2.4  93/01/07  03:54:21  mjh
 * leave X11 definition of Screen alone for now
 * 
 * Revision 5.0.2.3  93/01/06  17:00:52  mjh
 * window creation and destruction
 * 
 * Revision 5.0.2.2  93/01/06  04:41:56  mjh
 * initial revision
 * 
\*---------------------------------------------------------------------------*/

#include <xgl/xgl.h>

#include <X11/Xutil.h>
# if 0
#define CLAMP(dstP,srcP)						\
do {									\
  register RGBColor 	newSrc = *(RGBColor*)(srcP);			\
  float			max;						\
  if((max = (newSrc.r > newSrc.b && newSrc.r > newSrc.g ? newSrc.r :	\
            (newSrc.g > newSrc.b ? newSrc.g : newSrc.b))) > 1.0) {	\
      ((Xgl_color_rgb*)(dstP))->r = newSrc.r/max;			\
      ((Xgl_color_rgb*)(dstP))->g = newSrc.g/max;			\
      ((Xgl_color_rgb*)(dstP))->b = newSrc.b/max;			\
    } else {							       	\
      ((Xgl_color_rgb*)(dstP))->r = newSrc.r;				\
      ((Xgl_color_rgb*)(dstP))->g = newSrc.g;				\
      ((Xgl_color_rgb*)(dstP))->b = newSrc.b;				\
   }									\
} while (0)
#else
#define CLAMPCHANNEL(dst,src)						\
  (dst) = (src) > 1.0 ? 1.0 : (src) < 0.0 ? 0.0 : (src);

#define CLAMP(dstP,srcP)						\
do {									\
  register RGBColor 	newSrc = *(RGBColor*)(srcP);			\
  ((Xgl_color_rgb*)(dstP))->r = 					\
    newSrc.r > 1.0 ? 1.0 : newSrc.r < 0.0 ? 0.0 : newSrc.r;		\
  ((Xgl_color_rgb*)(dstP))->g = 					\
    newSrc.g > 1.0 ? 1.0 : newSrc.g < 0.0 ? 0.0 : newSrc.g;		\
  ((Xgl_color_rgb*)(dstP))->b = 					\
    newSrc.b > 1.0 ? 1.0 : newSrc.b < 0.0 ? 0.0 : newSrc.b;		\
} while (0)
#endif

#define NOBUFFER     0
#define BCKBUFFER    1
#define FRNTBUFFER   2
#define BOTHBUFFERS  3

typedef struct
{
  Xgl_win_ras ras ;
  Xgl_X_window xgl_x_win ;
  Xgl_obj_desc obj_desc ;
  Xgl_3d_ctx xglctx ;
#ifdef solaris
  Xgl_pt_d3d dev_max_coords ;
#else
  Xgl_pt_f3d dev_max_coords ;
#endif
  Xgl_trans tmp_transform ;
  Xgl_trans model_view_transform ;
  Xgl_trans projection_transform ;
  Xgl_ctx xgl_tmp_ctx ;
  Xgl_ras screen_door_50 ;
  Xgl_inquire *hw_info ;
  
  int num_planes ;
  Xgl_usgn32  num_buffers ;
  int double_buffer_mode ;
  int display_buffer, draw_buffer ;
  int buffer_config_mode ;
  int vp_left, vp_right, vp_bottom, vp_top ;
  int screen_xmax, screen_ymax ;
  int use_screen_door ;
  hwTranslationP	translation ;
  Xgl_usgn32		planeCount;
} tdmXGLctxT, *tdmXGLctx ;

#define DEFCONTEXT(ctx) \
  register tdmXGLctx _portContext = (tdmXGLctx)(ctx)

#define XGLRAS     (((tdmXGLctx)_portContext)->ras)
#define XGLCTX     (((tdmXGLctx)_portContext)->xglctx)
#define XGLXWINID  (((tdmXGLctx)_portContext)->xgl_x_win.X_window)
#define XGLDISPLAY ((Display *)((tdmXGLctx)_portContext)->xgl_x_win.X_display)
#define XGLSCREEN  (((tdmXGLctx)_portContext)->xgl_x_win.X_screen)
#define XGLTMPCTX  (((tdmXGLctx)_portContext)->xgl_tmp_ctx)

#define TMP_TRANSFORM        (((tdmXGLctx)_portContext)->tmp_transform)
#define MODEL_VIEW_TRANSFORM (((tdmXGLctx)_portContext)->model_view_transform)
#define PROJECTION_TRANSFORM (((tdmXGLctx)_portContext)->projection_transform)

#define HW_INFO            (((tdmXGLctx)_portContext)->hw_info)
#define NUM_PLANES         (((tdmXGLctx)_portContext)->num_planes)
#define NUM_BUFFERS        (((tdmXGLctx)_portContext)->num_buffers)
#define DOUBLE_BUFFER_MODE (((tdmXGLctx)_portContext)->double_buffer_mode)
#define DISPLAY_BUFFER     (((tdmXGLctx)_portContext)->display_buffer)
#define DRAW_BUFFER        (((tdmXGLctx)_portContext)->draw_buffer)
#define BUFFER_CONFIG_MODE (((tdmXGLctx)_portContext)->buffer_config_mode)

#define VP_LEFT   (((tdmXGLctx)_portContext)->vp_left)
#define VP_RIGHT  (((tdmXGLctx)_portContext)->vp_right)
#define VP_BOTTOM (((tdmXGLctx)_portContext)->vp_bottom)
#define VP_TOP    (((tdmXGLctx)_portContext)->vp_top)
#define VP_XMAX   (((tdmXGLctx)_portContext)->dev_max_coords.x)
#define VP_YMAX   (((tdmXGLctx)_portContext)->dev_max_coords.y)
#define VP_ZMAX   (((tdmXGLctx)_portContext)->dev_max_coords.z)

#define SCREEN_XMAX (((tdmXGLctx)_portContext)->screen_xmax)
#define SCREEN_YMAX (((tdmXGLctx)_portContext)->screen_ymax)

#define SCREEN_DOOR_50  (((tdmXGLctx)_portContext)->screen_door_50)
#define USE_SCREEN_DOOR (((tdmXGLctx)_portContext)->use_screen_door)

#define XGLTRANSLATION (((tdmXGLctx)_portContext)->translation)
#define CLIP_PLANE_CNT (((tdmXGLctx)_portContext)->planeCount)

/* light0 + light1 through light8 */
#define MAX_LIGHTS 9 

/* Is there any real limit?, Choose 2 clip boxes. */
#define MAX_CLIP_PLANES 12 

#define IS_2D(array,type,rank,shape)                           \
    (DXGetArrayInfo (array, NULL, &type, NULL, &rank, NULL) && \
     rank == 1 && type == TYPE_FLOAT &&                        \
     DXGetArrayInfo (array, NULL, NULL, NULL, NULL, &shape) && \
     shape == 2)


#endif /* tdmPortXGL_h */
