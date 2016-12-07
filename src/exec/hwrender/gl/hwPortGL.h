/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Header: /src/master/dx/src/exec/hwrender/gl/hwPortGL.h,v 1.3 1999/05/10 15:45:36 gda Exp $
 */

#ifndef tdmPortGL_h
#define tdmPortGL_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/gl/hwPortGL.h,v $
  Author: Mark Hood

  Data structures used by IBM GL implementation of the TDM renderer.

\*---------------------------------------------------------------------------*/

#define MAX_CLIP_PLANES	6

typedef struct ClipPlaneS {
  float	a,b,c,d;
} ClipPlaneT,ClipPlane;

typedef struct
{
  int gid ;
  int gltype ;
  char gversion_string[32] ;
  RGBColor background_color;
  int	winHeight;		/* need this to flip images */
  hwTranslationP	translation;
  int			clipPlaneCount;
  ClipPlane		clipPlanes[MAX_CLIP_PLANES];
  HashTable displayListHash;
  int doDisplayLists;
  int currentTexture;

} tdmGLctxT, *tdmGLctx ;

#define DEFCONTEXT(ctx) \
  register tdmGLctx _portContext = (tdmGLctx)(ctx)

#define GID (((tdmGLctx)_portContext)->gid)
#define GLTYPE (((tdmGLctx)_portContext)->gltype)
#define GVERSION_STRING (((tdmGLctx)_portContext)->gversion_string)
#define BACKGROUND_COLOR (((tdmGLctx)_portContext)->background_color)
#define WINHEIGHT (((tdmGLctx)_portContext)->winHeight)
#define GLTRANSLATION (((tdmGLctx)_portContext)->translation)
#define CLIP_PLANE_CNT (((tdmGLctx)_portContext)->clipPlaneCount)
#define CLIP_PLANES (((tdmGLctx)_portContext)->clipPlanes)
#define DO_DISPLAY_LISTS  (((tdmGLctx)_portContext)->doDisplayLists)
#define DISPLAY_LIST_HASH (((tdmGLctx)_portContext)->displayListHash)
#define CURTEX            (((tdmGLctx)_portContext)->currentTexture)


#define PATTERN_INDEX 1

#endif /* tdmPortGL_h */
