/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	tdmWindow_h
#define	tdmWindow_h
/*
 *
 * $Header: /src/master/dx/src/exec/hwrender/hwWindow.h,v 1.6 2003/07/30 22:39:07 davidt Exp $
 */

#include "hwStereo.h"

/*
^L
 *===================================================================
 *      Defines
 *===================================================================
 */

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif



#ifndef UP
#define	UP	0
#define	DOWN	1
#endif

/*
^L
 *===================================================================
 *      Structure Definitions and Typedefs
 *===================================================================
 */

/*
 *  window information
 */

typedef	struct WinS {
#if defined(DX_NATIVE_WINDOWS)
  HWND wigWindow ;			/* parent window owned by UI process */
  HWND xid ;				/* X handle to graphics window */
  HANDLE busy;
  HANDLE lock;
  int canRender;
  DWORD windowThread;
  HRGN lrgn;                /* left clipping region */
  HRGN rrgn;                /* right clipping region */
  int useGLStereo;          /* whether or not we're using GL stereo */
#else
  Display *dpy ;			/* X display connection to window */
  Window wigWindow ;			/* parent window owned by UI process */
  Window msgWindow ;			/* window which receives UI messages */
  Window toplevel ;			/* top level UI shell */
  Window xid ;				/* X handle to graphics window */
  Window lxid ;                         /* X handle to graphics window for left stereo image */
  Window rxid ;                         /* X handle to graphics window for right stereo image */
  Colormap xColormap ;			/* X colormap for graphics window */
#endif
  int visibility ;			/* visibility state of X window */
  tdmPortHandleP phP ;		       	/* graphics API context */
  void *curcam ;			/* current camera object */
  void *curobj ;			/* current object to render */
  int x, y ;				/* window position */
  int w, h ;				/* window width and height in pixels */
  int depth ;				/* window depth */
  int vpw, vph ;			/* viewport width/height in pixels */
  int correctSize ;			/* UI and HW agree on window size? */
  int bufferMode ;			/* double or single buffer mode? */
  void *buf ;		 		/* handle to SW image buffer */
  int bufCurrent ;			/* SW image current? */
  int buffSize ;			/* size of SW image */
  void *saveBuf ;	 		/* handle to HW image buffer */
  int saveBufValid ;			/* HW image current? */
  int saveBufSize ;			/* size of HW image */
  tdmInteractorWin interactorData ;	/* handle to interactor common data */
  void *matrixStack ;			/* SW matrix stack shadow */
  int mapped ;				/* graphics window mapped? */
  int camtag ;				/* tag of last camera used */
  int objtag ;			        /* tag of last object rendered */
  int gl_window ; 			/* GL window exist? */
  /* #X */ int uitype;
  char *where;                          /* original where parameter */
  int  link;                            /* whether its a '#X window */
  char *originalWhere;
  long pad[12] ; 			/* under construction */

  int saveBufObjectTag;
  int saveBufCameraTag;

  int   stereoSystemMode;
  int   stereoCameraMode;
  float currentFrom[3];
  float currentTo[3];
  float currentUp[3];
  float currentWidth;
  float currentFov;
  void *stereoCameraData;
  WindowInfo leftWindowInfo;
  WindowInfo rightWindowInfo;

} WinT ;


/*
 *  Renderer state.  References to `child' are historical.
 */

typedef struct tdmChildGlobalS {
#if defined(DX_NATIVE_WINDOWS)
  dxObject owner;
#endif
  tdmHWDescP adapter ;			/* hardware description */
  WinT win ;				/* window description */
  char *cacheId ;			/* renderer state handle in cache */

  int executeOnChange ;			/* Execute Once / Execute On Change */
  int displayGlobe ;			/* display or don't display globe */

  dxObject gather;
  int gatherTag;

  tdmInteractor Globe ;			/* handles to direct interactors */
  tdmInteractor Roamer ;
  tdmInteractor Cursor3D ;
  tdmInteractor Zoomer ;
  tdmInteractor ViewRotate ;
  tdmInteractor GnomonRotate ;
  tdmInteractor CrntInteractor ;
  tdmInteractor Navigator ;
  tdmInteractor PanZoom ;
  tdmInteractor CursorGroup ;
  tdmInteractor RoamGroup ;
  tdmInteractor RotateGroup ;
  tdmInteractor ViewTwirl ;
  tdmInteractor GnomonTwirl ;
  tdmInteractor Pick ;
  tdmInteractor User;

  HashTable meshHash;
  HashTable textureHash;
  SortList sortList;

  long pad[11] ;			/* under construction */
} tdmChildGlobalT ;

/*
 *  The following macros are used to access global data from the port
 *  implementations.  As the number of tdm ports grow, it is very
 *  important for maintainability to use these macros instead of accessing
 *  the data structure directly from the port level; otherwise we'll have
 *  to update several groups of files in parallel if the data structure
 *  ever changes.
 *
 *  Every port layer routine which needs access only to the data in the
 *  WinT data structure should declare its first parameter as "void *" and
 *  use the DEFWINDATA macro as the first statement in the body of the
 *  function.
 *
 *  A port layer routine which needs access to the entire _dxdChildGlobals
 *  data structure should declare its first parameter as "void *" and use
 *  the DEFGLOBALDATA macro as the first statement in the body of the
 *  function.
 *
 *  See hwPortGL.c for examples.
 */

#define DEFWINDATA(W) register WinT* _wdata = (WinT*) W

#define DPY (_wdata->dpy)
#define XWINID (_wdata->xid)
#define CAN_RENDER (_wdata->canRender)
#define PARENT_WINDOW (_wdata->wigWindow)
#define PORT_HANDLE (_wdata->phP)
#define CLRMAP (_wdata->xColormap)
#define CAMERA (_wdata->curcam)
#define OBJECT (_wdata->curobj)
#define PIXW (_wdata->w)
#define PIXH (_wdata->h)
#define SW_BUF (_wdata->buf)
#define SAVE_BUF (_wdata->saveBuf)
#define SAVE_BUF_SIZE (_wdata->saveBufSize)
#define SAVE_BUF_VALID (_wdata->saveBufValid)
#define BUFFER_MODE (_wdata->bufferMode)
#define INTERACTOR_DATA (_wdata->interactorData)
#define MATRIX_STACK (_wdata->matrixStack)
#define GL_WINDOW (_wdata->gl_window)
#define SAVEBUFOBJECTTAG (_wdata->saveBufObjectTag)
#define SAVEBUFCAMERATAG (_wdata->saveBufObjectTag)


/* #X follows */
#define UI_TYPE (_wdata->uitype)

#define DXD_DXUI        1
#define DXD_EXTERNALUI 2
#define DXD_NOUI        3
#define DXUI (UI_TYPE == DXD_DXUI)
#define EXTERNALUI (UI_TYPE == DXD_EXTERNALUI)
#define NOUI (UI_TYPE == DXD_NOUI)
/* #X end */

#define WHERE (_wdata->where)
#define LINK (_wdata->link)
#define ORIGINALWHERE (_wdata->originalWhere)

#define STEREOSYSTEMMODE (_wdata->stereoSystemMode)
#define STEREOCAMERAMODE (_wdata->stereoCameraMode)
#define STEREOCAMERADATA (_wdata->stereoCameraData)

#if defined(DX_NATIVE_WINDOWS)
#define LEFTWINDOW (_wdata->lrgn)
#define RIGHTWINDOW (_wdata->rrgn)
#define USEGLSTEREO (_wdata->useGLStereo)
#else
#define LEFTWINDOW (_wdata->lxid)
#define RIGHTWINDOW (_wdata->rxid)
#endif
#define CURRENT_UP (_wdata->currentUp)
#define CURRENT_FROM (_wdata->currentFrom)
#define CURRENT_TO (_wdata->currentTo)
#define CURRENT_FOV (_wdata->currentFov)
#define CURRENT_WIDTH (_wdata->currentWidth)
#define LEFTWINDOWINFO (_wdata->leftWindowInfo)
#define RIGHTWINDOWINFO (_wdata->rightWindowInfo)

/* added by TJM */
#define TOPLEVEL (_wdata->toplevel)
#define PIXX (_wdata->x)
#define PIXY (_wdata->y)
#define PIXDEPTH (_wdata->depth)
#define MAPPED (_wdata->mapped)
#define VISIBILITY (_wdata->visibility)
#define CORRECT_SIZE (_wdata->correctSize)
#define OBJECT_TAG (_wdata->objtag)
#define CAMERA_TAG (_wdata->camtag)
#define MSG_WINDOW (_wdata->msgWindow)
#define SW_BUF_CURRENT (_wdata->bufCurrent)
#define SW_BUF_SIZE (_wdata->buffSize)
#define VIEWPORT_W (_wdata->vpw)
#define VIEWPORT_H (_wdata->vph)
#define LWIN ((WinP)_wdata)			/* Use only to pass win to functions */


#define DEFGLOBALDATA(G) \
register tdmChildGlobalT* _gdata = (tdmChildGlobalT*) G ; \
DEFWINDATA(&(((tdmChildGlobalT*) G)->win))

#define GATHER	    (_gdata->gather)
#define GATHER_TAG  (_gdata->gatherTag)
#define SORTLIST    (_gdata->sortList)
#define MESHHASH    (_gdata->meshHash)
#define TEXTUREHASH (_gdata->textureHash)


void _dxfSetCurrentView(WinP, float *, float *, float *, float, float);

#if defined(DX_NATIVE_WINDOWS)

#define BASE			(_wdata->busy);
#define WINDOWTHREAD	(_wdata->windowThread)
#define LOCK			(_wdata->lock)

typedef struct _OGLWindow
{
   	HGLRC		hRC;
	void		*globals;
	PAINTSTRUCT ps;
	int			repaint;
	long		lastX, lastY;
	int			stroke;
	int			quit;
	int			buttonState;
} OGLWindow;


#ifdef _WIN32
#define GetOGLWPtr(hwnd)		(OGLWindow *)GetWindowLong((hwnd),0)
#define SetOGLWPtr(hwnd,ptr)	SetWindowLong((hwnd),0,(LONG)(ptr))
#else
#define GetOGLWPtr(hwnd)		(OGLWindow *)GetWindowLong((hwnd),0)
#define SetOGLWPtr(hwnd,ptr)	SetWindowLong((hwnd),0,(WORD)(ptr))
#endif

#endif


/*
 *===================================================================
 *	 END OF FILE 
 *	$Source: /src/master/dx/src/exec/hwrender/hwWindow.h,v $
 *===================================================================
 */
#endif	/* tdmWindow_h */
