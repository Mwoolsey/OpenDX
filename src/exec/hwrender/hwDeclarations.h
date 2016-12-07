/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	tdmDeclarations_h
#define	tdmDeclarations_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwDeclarations.h,v $
  Author: Mark Hood

  This file contains ANSI prototypes for globally scoped TDM routines.
  These are essential for many non-AIX compilers in order to pass arguments
  correctly.  The initial revision doesn't yet contain all appropriate TDM
  routines, just the ones that have caused trouble so far.

\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\
  Defines
\*---------------------------------------------------------------------------*/

#include "hwObject.h"

/*
 * Values for the attribute & gather 'flags' parameter
 */
#define CONTAINS_TRANSPARENT         (1<<0)
#define CONTAINS_VOLUME              (1<<1)
#define CONTAINS_CLIP_OBJECT         (1<<2)
#define CONTAINS_NORMALS             (1<<3)
#define CONTAINS_LIGHT               (1<<4)
#define CONTAINS_SCREEN              (1<<5)
#define CONTAINS_IMAGE_ONLY          (1<<6)
#define CONTAINS_TRANSPARENT_TEXTURE (1<<7)

#define IGNORE_PARAMS		(1<<16)
#define IN_SCREEN_OBJECT	(1<<17)
#define BEING_CLIPPED		(1<<18)
#define IN_CLIP_OBJECT		(1<<19)
#define PERSPECTIVE_CAMERA	(1<<20)
#define CORRECTED_COLORS	(1<<21)

/*
 * Values for Xfield/porting layer services flags
 */
#define	SF_VOLUME_BOUNDARY	(1<<0)
#define SF_TMESH		(1<<1)
#define SF_QMESH		(1<<2)
#define	SF_TEXTURE_MAP		(1<<3)
#define SF_CONST_COEF_HELP	(1<<4)
#define SF_POLYLINES		(1<<5)
#define SF_GAMMA_CORRECT_COLORS	(1<<6)
#define SF_INVALIDATE_BACKSTORE (1<<7)	/* For machines that do slow blits */
#define SF_FLING                (1<<8)	/* Undocumented -- for marketing */
#define SF_TRANSPARENT_OFF      (1<<9)  /* For DEC PXG */
#define SF_USE_DISPLAYLISTS    (1<<10)
#define SF_DOES_TRANS          (1<<31)

/*---------------------------------------------------------------------------*\
  Structure Pointers
\*---------------------------------------------------------------------------*/
typedef struct tdmChildGlobalS		*tdmChildGlobalP;
typedef struct WinT			*WinP;
typedef struct mediumS  		*mediumP;
typedef struct xfieldS 			*xfieldP;
typedef struct sortS  			*sortP;
typedef struct sortIndexS  		*sortIndexP;
typedef struct tdmInteractorEchoS  	*tdmInteractorEchoP;
typedef struct attributeS	  	*attributeP;
typedef struct tdmHWDescS		*tdmHWDescP;
typedef struct tdmParsedFormatS		*tdmParsedFormatP;
typedef struct screenS			*screenP;
typedef struct tdmPortHandleS		*tdmPortHandleP;
typedef struct tdmDrawPortS		*tdmDrawPortP;
typedef struct tdmSimpleLightS		*tdmSimpleLightP;
typedef struct translationS		*hwTranslationP;

typedef void				*SortList;

#if !defined(DX_NATIVE_WINDOWS)
#include <X11/Xlib.h>
#endif

#include "hwFlags.h"
#include "hwPortLayer.h"
#include "hwInteractor.h"


/*
 * From hwMatrix.c
 */
void
_dxfInitStack(void *stack, int resolution, float view[4][4], int projection,
	     float width, float aspect, float Near, float Far);

void 
  _dxfGetNearFar(int projection, int resolution, float width,
		 float from[3], float zaxis[3], float box[8][3],
		 float *Near, float *Far);

/*
 *  from tdmRender.c and tdmRenderOptions.c
 */

Error 
  _dxfAsyncRender (dxObject r, Camera c, char *obsolete, char *displayString) ;
Error
  _dxfAsyncDelete (char *where) ;
Error
  _dxfLoadRenderOptionsFile (tdmChildGlobalP globals) ;
Error
  _dxfSetRenderOptions
    (tdmChildGlobalP globals, char *renderString, char *densityString) ;

tdmChildGlobalP
  _dxfCreateRenderModule (tdmParsedFormatP pFormat) ;

/*
 *  from hwWindow.c
 */

Error
  _dxfCreateWindow (tdmChildGlobalP globals, char *winName) ; 
Error
  _dxfProcessEvents (int fd, tdmChildGlobalP globals, int flag) ;
void
  _dxfSWImageDraw (WinP win) ;
void
  _dxfChangeBufferMode(WinP win, int mode) ;
int
  _dxfTrySaveBuffer (WinP win) ;
int
  _dxfTryRestoreBuffer (WinP win) ;
int
  _dxfConvertWinName (char *winName) ;

/*
 *  from tdmInitScreen.c
 */

Error
  _dxfInitRenderObject (WinP win) ;
int
  _dxfEndRenderPass (WinP win) ;
int
  _dxfEndSWRenderPass (WinP win, Field image) ;
Error
  _dxfEndRenderModule (tdmChildGlobalP globals) ;

/*
 *  from _dxfDraw.c
 */

void
  tdmMakeGLMatrix (float m1[4][4], dxMatrix m2) ;

/*
 * from hwXfield.c,hwGather.c
 */

void* 	_dxf_initPort(tdmParsedFormatP format);

int	_dxf_xfieldNconnections(xfieldP xf);
int	_dxf_xfieldSidesPerConnection(xfieldP xf);
attributeP _dxf_xfieldAttributes(xfieldP xf);

attributeP _dxf_newAttribute(attributeP from);
float	*_dxf_attributeFuzz(attributeP att, double *ff);
hwFlags _dxf_attributeFlags(attributeP att);
void	_dxf_attributeMatrix(attributeP att, float m[4][4]);
void	_dxf_setAttributeMatrix(attributeP att, float m[4][4]);

attributeP _dxf_parameters(dxObject o, attributeP old);

#endif /* tdmDeclarations_h */


