/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmTmesh_h
#define tdmTmesh_h
/*
 *
 * $Header: /src/master/dx/src/exec/hwrender/hwTmesh.h,v 1.3 1999/05/10 15:45:35 gda Exp $
 */


/*
^L
 *===================================================================
 *      Structure Definitions and Typedefs
 *===================================================================
 */


/*
 *  Tmesh -> tstrips = number of strips in mesh
 *        -> tstrip -> ------
 *                     | 0  | -> points = number of points in strip
 *                     |    | -> point -> ------
 *                     ------             |int | 
 *                     | .  | ->          ------
 *                     | .  | ->          |int |
 *                     ------             ------
 *                     | n  | ->
 *                     |    | ->
 *                     ------
 */


/*
 *  MaxTstripSize is a multiple of 3 so that long linear strips which must
 *  be broken will continue in the same direction.  This is mostly for
 *  cosmetic reasons when rendering a wireframe approximation of a
 *  triangle strip.
 */

#if defined(alphax)
#define MaxTstripSize 60
#else
#define MaxTstripSize 99
#endif

typedef struct _Tstrip
  {
  int points;
  int *point;
  } Tstrip;


typedef struct _Tmesh
  {
  int tstrips;
  Tstrip **tstrip;
  } Tmesh;


Tmesh 
*_dxf_get_tmesh (Field f, int ntriangles, int *triangles,
		int npoints, float *points) ;


#endif

