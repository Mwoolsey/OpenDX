/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 *  Rotation interactor private data structure.
 */

typedef struct {
  /* visibility */
  int visible ;
  /* radius of virtual sphere, center, and square of radius */
  float gradius, gx, gy, g2 ;
  /* viewport coordinates for virtual sphere echo */
  float vllx, vlly, vurx, vury ;
  /* background image coordinates and validity flag */
  int illx, illy, iurx, iury ;
  /* saved mouse position in spherical coordinates */
  double ustart, vstart ;
  /* saved mouse position as radial vector */
  float ax, ay, az ;
  /* basis vectors for twirling about virtual sphere Z axis */
  float xbasis[2], ybasis[2] ;
  /* rotation matrix update flag */
  int rotation_update ;
  /* image in background of echo */
  void *background ;
  /* image plus echo for quick restoration */
  void *foreground ;
  /* foreground view "state" */
  int foregroundView ;
  /* redraw, frame buffer, and display state */
  int redrawmode, buffermode, displaymode ;
  /* font for gnomon axes labels */
  int font ;
  /* 1 pixel and font width in normalized coords */
  float nudge, swidth ;
  /*
   *  newXfm stores current Z or XY rotation matrix.
   *  incXfm stores accumulated Z, XY transforms from crossing sphere radius.
   *         Some rotation models just store identity here.
   *  strXfm stores rotation matrix at start of stroke.
   *  newXfm X incXfm = output rotation matrix.
   *  newXfm X incXfm X strXfm = new echo orientation.
   */
  float newXfm[4][4] ;
  float incXfm[4][4] ;
  float strXfm[4][4] ;
} tdmRotateData ;

/*
 *  Globe radius and offset from lower left corner in pixels.
 */

#define GLOBERADIUS 50
#define GLOBEOFFSET 10

/*
 *  Gnomon radius and offset from lower right corner in pixels.
 */

#define GNOMONRADIUS 50
#define GNOMONOFFSET 10
