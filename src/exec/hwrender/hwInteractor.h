/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmInteractor_h
#define tdmInteractor_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwInteractor.h,v $
  Author: Mark Hood

  This include file defines core data structures used by the direct
  interactor implementations.

\*---------------------------------------------------------------------------*/

#include <stdio.h>
#include <math.h>

/* SGI */
#if defined(sgi)
#ifndef Int32
#define Int32 long
#endif
#ifndef Int16
#define Int16 short
#endif
#endif

#ifdef STANDALONE 
#define tdmAllocateLocal malloc
#define tdmFree free
#endif

/* 
 * The following must match UserInteractors.h
 */
#define DX_LEFT_BUTTON_MASK     0x01
#define DX_MIDDLE_BUTTON_MASK   0x02
#define DX_RIGHT_BUTTON_MASK    0x04

#define INTERACTOR_BUTTON_UP	1
#define INTERACTOR_BUTTON_DOWN  2


typedef struct tdmInteractorS tdmInteractorT, *tdmInteractor ;
typedef struct tdmInteractorWinS tdmInteractorWinT, *tdmInteractorWin ;
typedef struct tdmInteractorCamS tdmInteractorCamT, *tdmInteractorCam ;
typedef void (*tdmViewEchoFunc) (tdmInteractor, void *, float[4][4], int) ;
typedef struct tdmInteractorEchoS tdmInteractorEchoT, *tdmInteractorEcho ;

typedef enum {tdmBackBufferDraw, tdmFrontBufferDraw,
	      tdmBothBufferDraw, tdmAuxEchoMode, tdmViewEchoMode}
tdmInteractorRedrawMode ;

typedef enum {tdmNullEcho, tdmGlobeEcho, tdmGnomonEcho}
tdmEchoType ;

typedef enum {tdmXYPlaneRoll, tdmZTwirl}
tdmRotateModel ;

typedef enum {tdmProbeCursor, tdmRoamCursor, tdmPickCursor}
tdmCursorType ;

/*
 *  Following constants must have the same values as their Picture widget
 *  counterparts (switch Xm and tdm prefixes).
 */

#define tdmCONSTRAIN_NONE       0
#define tdmCONSTRAIN_X          1
#define tdmCONSTRAIN_Y          2
#define tdmCONSTRAIN_Z          3

#define tdmSLOW_CURSOR          0
#define tdmMEDIUM_CURSOR        1
#define tdmFAST_CURSOR          2

#define tdmCURSOR_CREATE        0
#define tdmCURSOR_DELETE        1
#define tdmCURSOR_MOVE          2
#define tdmCURSOR_SELECT        3
#define tdmCURSOR_DRAG          4
#define tdmROTATION_UPDATE      5

#define tdmPCR_TRANSLATE_SPEED  5
#define tdmPCR_ROTATE_SPEED     6

#define tdmLOOK_LEFT            0
#define tdmLOOK_RIGHT           1
#define tdmLOOK_UP              2
#define tdmLOOK_DOWN            3
#define tdmLOOK_BACKWARD        4
#define tdmLOOK_FORWARD         5
#define tdmLOOK_ALIGN           6

/*
 *  Data structure returned by tdmEndStroke().
 */

typedef struct tdmInteractorReturnS {
  tdmInteractor I ;
  int id ;
  int reason ;
  float x, y, z ;
  float to[3] ;
  float from[3] ;
  float dist ;
  float up[3] ;
  float view[4][4] ;
  float matrix[4][4] ;
  float zoom_factor ;
  int change ;
} tdmInteractorReturn, *tdmInteractorReturnP ;

/*
 *  Interactor definition.
 */

struct tdmInteractorS {
  void (*DoubleClick) (tdmInteractor, int, int, tdmInteractorReturnP);
  void (*StartStroke) (tdmInteractor, int, int, int, int) ;
  void (*StrokePoint) (tdmInteractor, int, int, int, int) ;
  void (*EndStroke)   (tdmInteractor, tdmInteractorReturnP) ;
  void (*KeyStruck)    (tdmInteractor, int, int, char, int) ;
  void (*ResumeEcho)  (tdmInteractor, tdmInteractorRedrawMode) ;
  void (*EchoFunc)    (tdmInteractor, void *, float[4][4], int) ;
  void (*Destroy)     (tdmInteractor) ;

  void (*Config)() ;         /* parameters vary: beware type conversions */
  void (*restoreConfig)() ;  /* parameters vary: beware type conversions */

  tdmInteractorWin cdata ;   /* pointer to common data */
  void *pdata ;              /* pointer to private data */
  void *udata ;              /* pointer to user data */
  tdmInteractorEcho edata;   /* pointer to port layer echo functions */

  /* following interactor references are used by the Group interactor */
  tdmInteractor btn1 ;
  tdmInteractor btn2 ;
  tdmInteractor btn3 ;
  int is_group ;

  /* list of associated interactor echos */
  tdmInteractor aux ;
  int is_aux ;

  /* allocation bookkeeping */
  int used ;

  /* event mask */
  int eventMask;
} ;

/*
 *  Data shared among all of window's interactors.
 */

struct tdmInteractorCamS {
  /* global box used for manipulating 3D cursors, supplied by application */
  float box[8][3] ;
  /* global 3D cursor movement constraint */
  int cursor_constraint ;
  /* global 3D cursor speed specification */
  int cursor_speed ;

  /* view coordinate system definition, supplied by application */
  float from[3], to[3], up[3] ;
  /* distance between from point and to point */
  float vdist ;
  /* projection type, 1 if perspective, 0 if ortho */
  int projection ;
  /* field of view, width */
  float fov, width ;
  /* near, far clip planes */
  float Near, Far ;

  /* has the view coordinate system been set? */
  int view_coords_set ;
  /* has anybody changed view data? */
  int view_state ;
  /* has anybody changed box data? */
  int box_state ;

  /* width, height, depth, and center of window in screen coords */
  long d, w, h, cx, cy ;
  /* lower left corner of window relative to screen and screen dimensions */
  int ox, oy, xmaxscreen, ymaxscreen ;
  /* aspect ratio of image */
  float aspect ;
  /* view matrix */
  float viewXfm[4][4] ;
  /* projection matrix */
  float projXfm[4][4] ;
  /* screen transform */
  float scrnXfm[4][4] ;
  /* draw mode and Z buffer modes */
  int drawmode, zdraw, zbuffer ;

  /* screen coordinates of last input */
  int xlast, ylast ;

  /* copy of pointer to background image and image dimensions */
  void *image ;
  int iw, ih ;

  /* handle to graphics API context */
  /* NOTE: Do not access this through CDATA, use DEFPORT(I_PORT_HANDLE) and
   * PORT_CTX.
   */
  void *phP ;

  /* handle to graphics API echo functions */
  void *echoFuncs;

  /* handle to software transformation stack */
  void *stack ;

  /* pointer to next camera in undo/redo stack */
  tdmInteractorCam next ;
} ;

struct tdmInteractorWinS {
  /* camera stack and redo stack pointers */
  tdmInteractorCam Cam ;
  tdmInteractorCam redo ;

  /* an array of interactors */
  tdmInteractor *Interactors ;
  int numUsed ;
  int numAllocated ;

  /* handle to interactor storage */
  void *extents ;
} ;

/*
 *  Interactor data is accessed through these macros.
 */

#define FUNC(interactor,f) ((interactor)->f)
#define CALLFUNC(interactor,f) (*FUNC(interactor,f))

#define UDATA(interactor) ((interactor)->udata)
#define WINDOW(interactor) ((interactor)->cdata)

/* gcc gets upset on alpha at least with PRIVATE previously defined */
#ifdef PRIVATE
#undef PRIVATE
#endif
#define PRIVATE(interactor) ((interactor)->pdata)

#define BTN1(interactor) ((interactor)->btn1)
#define BTN2(interactor) ((interactor)->btn2)
#define BTN3(interactor) ((interactor)->btn3)
#define IS_GROUP(interactor) ((interactor)->is_group)
#define IS_USED(interactor) ((interactor)->used)
#define AUX(interactor) ((interactor)->aux)
#define IS_AUX(interactor) ((interactor)->is_aux)
#define EVENT_MASK(interactor)  ((interactor) ? (interactor)->eventMask : 0)

#define DEFDATA(interactor,ptype) \
register tdmInteractorCam _cdata = ((interactor)->cdata->Cam) ; \
register ptype *_pdata = (ptype *) ((interactor)->pdata) ; \
double _w 

#define PDATA(foo) (_pdata->foo)
#define CDATA(foo) (_cdata->foo)
#define I_PORT_HANDLE (_cdata->phP)

/*
 *  Misc. macros for interactor implementations.
 */

#define VCOPY(t,f) (bcopy ((char *)(f), (char *)(t), sizeof(float [3])))
#define MCOPY(t,f) (bcopy ((char *)(f), (char *)(t), sizeof(float [4][4])))
#define DMCOPY(t,f) (bcopy ((char *)(f), (char *)(t), sizeof(double [4][4])))
#define CLIPPED(x,y) ((x)<0 || (y)<0 || (x)>(CDATA(iw)-1) || (y)>(CDATA(ih)-1))

/* used to ensure pixel reads and writes are on physical screen */
#define XSCREENCLIP(x) \
(CDATA(ox)+x < 0 ? -CDATA(ox) : \
(CDATA(ox)+x > CDATA(xmaxscreen) ? CDATA(xmaxscreen)-CDATA(ox) : x))
#define YSCREENCLIP(y) \
(CDATA(oy)+y < 0 ? -CDATA(oy) : \
(CDATA(oy)+y > CDATA(ymaxscreen) ? CDATA(ymaxscreen)-CDATA(oy) : y))

#define VSUB(r, a, b) {\
r[0] = a[0] - b[0] ; \
r[1] = a[1] - b[1] ; \
r[2] = a[2] - b[2] ; }

#define CROSS(r, a, b) {\
r[0] = a[1]*b[2] - a[2]*b[1] ; \
r[1] = a[2]*b[0] - a[0]*b[2] ; \
r[2] = a[0]*b[1] - a[1]*b[0] ; }

#define XFORM_VECTOR(M, in, out) {\
out[0] = in[0]*M[0][0] + in[1]*M[1][0] + in[2]*M[2][0] ; \
out[1] = in[0]*M[0][1] + in[1]*M[1][1] + in[2]*M[2][1] ; \
out[2] = in[0]*M[0][2] + in[1]*M[1][2] + in[2]*M[2][2] ; }

#define XFORM_COORDS(M, x, y, z, nx, ny, nz) {\
nx = x*M[0][0] + y*M[1][0] + z*M[2][0] + M[3][0] ; \
ny = x*M[0][1] + y*M[1][1] + z*M[2][1] + M[3][1] ; \
nz = x*M[0][2] + y*M[1][2] + z*M[2][2] + M[3][2] ; \
if (CDATA(projection)) { \
   _w = x*M[0][3] + y*M[1][3] + z*M[2][3] + M[3][3] ; \
   if (_w) { \
     nx /= _w ; ny /= _w ; nz /= _w ;}}}

#define YFLIP(y) ((CDATA(h) - (y)) - 1)


#ifndef RAD2DEG
#define RAD2DEG(r) (180.0*(r)/M_PI)
#endif
#ifndef DEG2RAD
#define DEG2RAD(d) (M_PI*(d)/180.0)
#endif

#ifndef LENGTH
#define LENGTH(V) (sqrt((double)(V[0]*V[0] + V[1]*V[1] + V[2]*V[2])))
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifdef MJHDEBUG
#define DPRINT(str) {fprintf (stderr, str); fflush(stderr);}
#define DPRINT1(str,a) {fprintf (stderr, str, a); fflush(stderr);}
#define VPRINT(V) fprintf (stderr, "%12f %12f %12f", V[0], V[1], V[2])
#define SPRINT(S) fprintf (stderr, "%12f %12f %12f", S.x, S.y, S.z)
#define CPRINT(C) fprintf (stderr, "%12f %12f %12f", C.r, C.g, C.b) ;
#define MPRINT(M) \
 fprintf \
    (stderr, \
     ": %17f %17f %17f %17f\n: %17f %17f %17f %17f\n: %17f %17f %17f %17f\n: %17f %17f %17f %17f", \
     M[0][0], M[0][1], M[0][2], M[0][3], \
     M[1][0], M[1][1], M[1][2], M[1][3], \
     M[2][0], M[2][1], M[2][2], M[2][3], \
     M[3][0], M[3][1], M[3][2], M[3][3]) ;
#else
#define DPRINT(str) 0
#define DPRINT1(str,a) 0
#define VPRINT(V) 0
#define SPRINT(S) 0
#define CPRINT(C) 0
#define MPRINT(M) 0
#endif
    
/*
 *  Following declarations used only by interactor implementations.
 */

tdmInteractor _dxfAllocateInteractor (tdmInteractorWin, int) ;
void _dxfDeallocateInteractor(tdmInteractor) ;
void _dxfNullDoubleClick (tdmInteractor, int, int, tdmInteractorReturn *) ;
void _dxfNullStartStroke (tdmInteractor, int, int, int, int) ;
void _dxfNullStrokePoint (tdmInteractor, int, int, int, int ) ;
void _dxfNullEndStroke (tdmInteractor, tdmInteractorReturn *) ;
void _dxfNullResumeEcho (tdmInteractor, tdmInteractorRedrawMode) ;
void _dxfNullKeyStruck(tdmInteractor, int, int, char, int);

/*
 *  Public interface.
 */

#define tdmDoubleClick(I,x,y,R) {if((I)) (*((I)->DoubleClick)) (I,x,y,R) ;}
#define tdmStartStroke(I,x,y,b,s) {if((I)) (*((I)->StartStroke)) (I,x,y,b,s) ;}
#define tdmStrokePoint(I,x,y,type,s) {if((I)) (*((I)->StrokePoint)) (I,x,y,type,s) ;}
#define tdmEndStroke(I,R) {if((I)) (*((I)->EndStroke)) (I,R) ;}
#define tdmKeyStruck(I,x,y,c,s) {if((I)) (*((I)->KeyStruck)) (I,x,y,c,s) ;}
#define tdmResumeEcho(I,m) {if((I)) (*((I)->ResumeEcho)) (I,m) ;}
#define tdmDestroyInteractor(I) {if((I)) (*((I)->Destroy)) (I) ;}

tdmInteractorWin _dxfInitInteractors (void *ctx, void *stack) ;
void _dxfRedrawInteractorEchos (tdmInteractorWin W) ;
void _dxfDestroyAllInteractors (tdmInteractorWin W) ;
void _dxfAssociateInteractorEcho (tdmInteractor T, tdmInteractor A) ;
void _dxfDisassociateInteractorEcho (tdmInteractorWin W, tdmInteractor A) ;

tdmInteractor _dxfCreateUserInteractor (tdmInteractorWin W, tdmViewEchoFunc E, void *udata) ;
tdmInteractor _dxfCreateZoomInteractor (tdmInteractorWin W) ;
tdmInteractor _dxfCreatePanZoomInteractor (tdmInteractorWin W) ;
tdmInteractor _dxfCreateCursorInteractor (tdmInteractorWin W, tdmCursorType C) ;
tdmInteractor _dxfCreateRotationInteractor
    (tdmInteractorWin W, tdmEchoType E, tdmRotateModel M) ;
tdmInteractor _dxfCreateViewRotationInteractor
    (tdmInteractorWin W, tdmViewEchoFunc E, tdmRotateModel M, void *udata) ;
tdmInteractor _dxfCreateNavigator
    (tdmInteractorWin W, tdmViewEchoFunc func, void *data) ;
tdmInteractor _dxfCreateInteractorGroup
    (tdmInteractorWin W, tdmInteractor btn1, tdmInteractor btn2,
    tdmInteractor btn3) ;

int _dxfPushInteractorCamera (tdmInteractorWin W) ;
int _dxfPopInteractorCamera (tdmInteractorWin W) ;
int _dxfRedoInteractorCamera (tdmInteractorWin W) ;

void _dxfInteractorViewChanged (tdmInteractorWin W) ;
void _dxfSetInteractorBox (tdmInteractorWin W, float box[8][3]) ;
void _dxfSetInteractorWindowSize (tdmInteractorWin W, int width, int height) ;
void _dxfSetInteractorViewDepth (tdmInteractorWin W, int depth) ;
void _dxfSetInteractorImage (tdmInteractorWin W, int w, int h, void *image) ;
void _dxfSetInteractorViewInfo (tdmInteractorWin W,
                                float *f, float *t, float *u, float dist,
                                float fov, float width, float aspect, int proj,
                                float Near, float Far) ;
void _dxfGetInteractorViewInfo (tdmInteractorWin W,
                                float *f, float *t, float *u, float *dist,
                                float *fov, float *width, float *aspect,
                                int *projection, float *Near, float *Far,
                                int *pixwidth) ;

void _dxfSetCursorSpeed (tdmInteractorWin W, int speed) ;
void _dxfSetCursorConstraint (tdmInteractorWin W, int constraint) ;
void _dxfCursorInteractorChange (tdmInteractor I, int id, int reason,
                                 float x, float y, float z) ;

void _dxfCursorInteractorVisible (tdmInteractor I) ;
void _dxfCursorInteractorInvisible (tdmInteractor I) ;
void _dxfRotateInteractorVisible (tdmInteractor I) ;
void _dxfRotateInteractorInvisible (tdmInteractor I) ;
void _dxfSetNavigateTranslateSpeed (tdmInteractor I, float speed) ;
void _dxfSetNavigateRotateSpeed (tdmInteractor I, float speed) ;
void _dxfSetNavigateLookAt (tdmInteractor I, int direction, float angle,
                            float current_view[3][3], 
                            float viewDirReturn[3], float viewUpReturn[3]) ;
void _dxfUpdateInteractorGroup (tdmInteractor I,
                                tdmInteractor btn1,
                                tdmInteractor btn2,
                                tdmInteractor btn3) ;

void _dxfResetCursorInteractor (tdmInteractor I) ;
void _dxfResetNavigateInteractor (tdmInteractor I) ;

void _dxfSetUserInteractorMode(tdmInteractor I, dxObject args);


#endif /* tdmInteractor_h */

