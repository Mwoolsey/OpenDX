/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef hwPortSB_h
#define hwPortSB_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwPortSB.h,v $
  Author: Mark Hood

  Data structures used by starbase implementation of tdm renderer.

\*---------------------------------------------------------------------------*/

#include <X11/Xutil.h>

#define NOBUFFER     0
#define BCKBUFFER    1
#define FRNTBUFFER   2
#define BOTHBUFFERS  3
#define	MAX_CLIP_PLANES	6

typedef struct tdmImageSBS { 		   /* 3 banks to make 24 planes*/
    unsigned char *saveBufRed ;            /* Bank 0 - Red - 8 planes */
    unsigned char *saveBufGreen ;          /* Bank 1 - Green - 8 planes */
    unsigned char *saveBufBlue ;           /* Bank 2 - Blue - 8 planes */
  } tdmImageSBT, *tdmImageSBP ;

typedef struct tdmClipPlaneSBS {
  Point		pt;
  Vector	normal;
  } tdmClipPlaneSBT,*tdmClipPlane; 
typedef struct tdmSBctxS
{
  int fildes ; 			/* file descriptor used for all SB calls */
  int lightState ;
  int ambient_id ;
  int win_height ;              /* DX notion of window height */
  int vp_left, vp_right,        /* Current Viewport dimensions */
      vp_bottom, vp_top ;
  int buffer_config_mode ;      /* BCKBUFFER, FRNTBUFFER */ 
  int buffer_state ; 		/* Even number is Buffer 0 (lower half)
				   Odd  number is Buffer 1 (upper half) */
  int hidden_surface ;          /* Hidden surface removal flag -> TRUE(1) */
  int planes ;			/* Number of HW Image planes */
  int color_map ;       	/* Color DXMap --FULL, NORMAL, or MONOTONIC */
  Display *dpy_sb ;		/* X display connection to window */
  int screen_sb ;               /* X screen used */
  Window xwindow_id ;		/* Same as XWINID, _wdata->xid in hwWindow.h */
  int xwin_width, xwin_height ; /* window width and height in pixels */
  int xmaxscreen, ymaxscreen ;
  void *gnobeBuffer ;           /* handle to HW globe/gnomon image buffer */
  int singleBuffer ;		/* single buffer mode -> TRUE(1) */
  hwTranslationP translation ;  /* translation for _dxf_dither */
  float clipPlanes[6 * 6];	/* model clip planes (6 planes * 6 coords) */
  int	clipPlaneCnt ;		/* number of active clip planes */
} tdmSBctxT, *tdmSBctx ;

#define DEFCONTEXT(ctx) \
  register tdmSBctx _portContext = (tdmSBctx)ctx

#define FILDES (((tdmSBctx)_portContext)->fildes)
#define LIGHTSTATE (((tdmSBctx)_portContext)->lightState)
#define AMBIENT_ID (((tdmSBctx)_portContext)->ambient_id)
#define VP_LEFT (((tdmSBctx)_portContext)->vp_left)
#define VP_RIGHT (((tdmSBctx)_portContext)->vp_right)
#define VP_BOTTOM (((tdmSBctx)_portContext)->vp_bottom)
#define VP_TOP (((tdmSBctx)_portContext)->vp_top)
#define BUFFER_CONFIG_MODE (((tdmSBctx)_portContext)->buffer_config_mode)
#define BUFFER_STATE (((tdmSBctx)_portContext)->buffer_state)
#define WIN_HEIGHT (((tdmSBctx)_portContext)->win_height)
#define HIDDEN_SURFACE (((tdmSBctx)_portContext)->hidden_surface)
#define COLOR_MAP (((tdmSBctx)_portContext)->color_map)
#define PLANES (((tdmSBctx)_portContext)->planes)
#define DPY_SB (((tdmSBctx)_portContext)->dpy_sb)
#define SCREEN_SB (((tdmSBctx)_portContext)->screen_sb)
#define XWINDOW_ID (((tdmSBctx)_portContext)->xwindow_id)
#define XWIN_WIDTH (((tdmSBctx)_portContext)->xwin_width)
#define XWIN_HEIGHT (((tdmSBctx)_portContext)->xwin_height)
#define XMAXSCREEN (((tdmSBctx)_portContext)->xmaxscreen)
#define YMAXSCREEN (((tdmSBctx)_portContext)->ymaxscreen)
#define GNOBE_BUF (((tdmSBctx)_portContext)->gnobeBuffer)
#define SINGLE_BUF (((tdmSBctx)_portContext)->singleBuffer)
#define SBTRANSLATION (((tdmSBctx)_portContext)->translation)
#define SBCLIPPLANES (((tdmSBctx)_portContext)->clipPlanes)
#define SBCLIPPLANECNT (((tdmSBctx)_portContext)->clipPlaneCnt)

#define CLAMP(c) (c > 1.0 ? 1.0 : (c < 0.0 ? 0.0 : c))
#define SET_COLOR(f, i) (color_map? SET_CMAP_COLOR(f,i): SET_CDIR_COLOR(f,i))
#define SET_CDIR_COLOR(f,i) (f(FILDES,              \
			       CLAMP(fcolors[i].r), \
			       CLAMP(fcolors[i].g), \
			       CLAMP(fcolors[i].b)))
#define SET_CMAP_COLOR(f,i) (f(FILDES,                                   \
			       CLAMP(color_map[((char *)fcolors)[i]].r), \
			       CLAMP(color_map[((char *)fcolors)[i]].g), \
			       CLAMP(color_map[((char *)fcolors)[i]].b)))

#define IS_2D(array,type,rank,shape)                           \
    (DXGetArrayInfo (array, NULL, &type, NULL, &rank, NULL) && \
     rank == 1 && type == TYPE_FLOAT &&                        \
     DXGetArrayInfo (array, NULL, NULL, NULL, NULL, &shape) && \
     shape == 2)

#endif /* hwPortSB_h */
