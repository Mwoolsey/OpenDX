/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef hwPlineDrawSB_c
#define hwPlineDrawSB_c
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwPlineDrawSB.c,v $
  Author: Tim Murphy

  This routine takes a DX field, looks up the previously generated
  Starbase polyline strips, and draws them if they already exist in
  the executive cache.  Otherwise it obtains the generic strip
  topology from the xfield data structure, generates Starbase strips,
  draws them, and then attempts to cache them for the next draw cycle.
  This is the fast path to the hardware for lines with data dependent
  upon positions.

  This was edited from hwMeshDraw.c.h

\*---------------------------------------------------------------------------*/
#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwXfield.h"
#include "hwCacheUtilSB.h"
#include "hwPortLayer.h"
#include "hwMemory.h"
#include "hwTmesh.h"

#include "hwDebug.h"

#ifdef DEBUG
#define PrintBounds()                                                         \
{									      \
  int i ;								      \
  float outx, outy, outz ;						      \
  float minx, maxx, miny, maxy, minz, maxz ;				      \
									      \
  minx = miny = minz = +MAXFLOAT ;					      \
  maxx = maxy = maxz = -MAXFLOAT ;					      \
									      \
  if (!pnts2d && !points)						      \
      /* get positions */						      \
      if (is_2d = IS_2D (xf->positions_array, type, rank, shape))	      \
	  pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array) ;	      \
      else								      \
	  points = (Point *) DXGetArrayData(xf->positions_array) ;	      \
      									      \
  if (is_2d)								      \
    {									      \
      minz = maxz = 0 ;							      \
      fprintf (stderr, "\n2-dimensional positions") ;                         \
      for (i=0 ; i<xf->npositions ; i++)    				      \
	{								      \
	  if (pnts2d[i].x < minx) minx = pnts2d[i].x ;			      \
	  if (pnts2d[i].y < miny) miny = pnts2d[i].y ;			      \
	  								      \
	  if (pnts2d[i].x > maxx) maxx = pnts2d[i].x ;			      \
	  if (pnts2d[i].y > maxy) maxy = pnts2d[i].y ;			      \
	}								      \
    }									      \
  else									      \
      for (i=0 ; i<xf->npositions ; i++)    				      \
	{								      \
	  if (points[i].x < minx) minx = points[i].x ;			      \
	  if (points[i].y < miny) miny = points[i].y ;			      \
	  if (points[i].z < minz) minz = points[i].z ;			      \
	  								      \
	  if (points[i].x > maxx) maxx = points[i].x ;			      \
	  if (points[i].y > maxy) maxy = points[i].y ;			      \
	  if (points[i].z > maxz) maxz = points[i].z ;			      \
	}								      \
									      \
  transform_point(FILDES, MC_TO_VDC, minx, miny, minz, &outx, &outy, &outz) ; \
  fprintf(stderr, "\nmin MC->VDC %9f %9f %9f -> %9f %9f %9f",		      \
	  minx, miny, minz, outx, outy, outz) ;				      \
									      \
  transform_point(FILDES, MC_TO_VDC, maxx, maxy, maxz, &outx, &outy, &outz) ; \
  fprintf(stderr, "\nmax MC->VDC %9f %9f %9f -> %9f %9f %9f",		      \
	  maxx, maxy, maxz, outx, outy, outz) ;				      \
}

#define DebugMessage()                                                        \
{                                                                             \
  fprintf (stderr, "\nnumber of points: %d", xf->npositions) ;   	      \
  fprintf (stderr, "\n%d invalid positions",                                  \
	  xf->invPositions? DXGetInvalidCount(xf->invPositions): 0) ;         \
  fprintf (stderr, "\nnumber of connections: %d", xf->origNConnections) ;     \
  fprintf (stderr, "\n%d invalid connections",                                \
	  xf->invCntns? DXGetInvalidCount(xf->invCntns): 0) ;                 \
  fprintf (stderr, "\ncolors: 0x%x, color_map: 0x%x, dep on pos: %d",	      \
	   (int)fcolors, (int)color_map, xf->fcolorsDep == dep_positions) ;   \
  fprintf (stderr, "\nnormals: 0x%x, dep on pos: %d",			      \
	   (int)normals, xf->normalsDep == dep_positions) ;	              \
  fprintf (stderr, "\nambient, diffuse, specular: %f %f %f",		      \
	   xf->attributes.front.ambient,                                      \
	   xf->attributes.front.diffuse,                                      \
	   xf->attributes.front.specular) ;                                   \
									      \
  PrintBounds() ;							      \
}
#else  /* DEBUG */
#define PrintBounds() {}
#define DebugMessage() {}
#endif /* DEBUG */



int
_dxfPlineDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  Vector *normals ;
  Point *points = 0 ;
  struct p2d {float x, y ;} *pnts2d = 0 ;
  RGBColor *fcolors, *color_map ;
  char *cache_id ;
  enum approxE approx ;
  tdmStripArraySB *stripsSB = 0 ;
  int type, rank, shape, is_2d ;

  DEFPORT(portHandle) ;

  ENTRY(("_dxfPlineDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));

  /*
   *  Transparent surfaces are stripped for those ports which implement
   *  transparency with the screen door technique, but we need to sort them
   *  as individual polygons to apply the more accurate alpha composition
   *  method supported by Starbase.
   *
   *  Connection-dependent colors are not supported by Starbase strips.
   */

  approx = buttonUp ? xf->attributes.buttonUp.approx :
	              xf->attributes.buttonDown.approx ;

  if (
#ifdef ALLOW_LINES_APPROX_BY_DOTS
      (approx == approx_dots) ||
#endif
      (xf->colorsDep == dep_connections))
    {
      int status ;
      PRINT(("drawing as individual polygons"));
      status = _dxfLineDraw (portHandle, xf, buttonUp) ;
      EXIT((""));
      return status ;
    }

  /*
   *  Extract rendering data from the xfield.
   */

  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
      fcolors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL) ;
  else
      fcolors = (Pointer) DXGetArrayData(xf->fcolors_array) ;

  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;

  if (DXGetArrayClass(xf->normals_array) == CLASS_CONSTANTARRAY)
      normals = (Pointer) DXGetArrayEntry(xf->normals, 0, NULL) ;
  else
      normals = (Pointer) DXGetArrayData(xf->normals_array) ;

#if 0
  if (xf->colorsDep != dep_field)
  {
    /*
     *  Dense fields of varying colors are visually confusing
     *  without hidden surface, even with wireframe approximation.
     */
    hidden_surface (FILDES, TRUE, FALSE) ;
  }
  else /* RE : <LUMBA281> */
  {
     hidden_surface(FILDES, TRUE, FALSE);
  }
#endif 

  hidden_surface(FILDES, TRUE, FALSE);

  if (xf->colorsDep == dep_field)
    {
      /* render field in constant color */
      cache_id = "CpfPline" ;
      SET_COLOR(fill_color, 0) ;
      SET_COLOR(line_color, 0) ;
    }
  else
    {
      cache_id = "CpfPline" ;
    }

  /*
   *  Render strips.
   */
  
  PRINT(("%s", cache_id));
  if (stripsSB = tdmGetTmeshCacheSB (cache_id, xf))
    {
      /*
       *  Use pre-constructed Starbase strips obtained from executive cache.
       */
      
      register tdmTmeshCacheSB *stripArray, *end ;
      PRINT(("got strips from cache"));
      PrintBounds() ;
      
      stripArray = stripsSB->stripArray ;
      end = &stripArray[stripsSB->num_strips] ;
      
      for ( ; stripArray < end ; stripArray++)
	{
	  polyline_with_data3d
	    (FILDES, stripArray->clist, stripArray->numverts,
	     stripArray->numcoords, stripArray->vertex_flags, NULL) ;
	}
    }
  else
    {
      /*
       *  Construct an array of Starbase strips and cache it.
       */

      register int vsize ;
      int *connections, (*strips)[2] ;
      int cOffs, nOffs, vertex_flags, facet_flags, numStrips ;
      tdmTmeshCacheSB *stripArray ;
      PRINT(("building new strips"));

      /* determine vertex and facet types and sizes */
      vertex_flags = 0 ;
      facet_flags = UNIT_NORMALS ;
      vsize = 3 ;

      if (fcolors && xf->colorsDep != dep_field)
	{
	  /* vertex has at least 6 floats, with color at float 3 */
	  vsize = 6 ; cOffs = 3 ;
	  vertex_flags |= VERTEX_COLOR ;
	}
      
      /* get positions */
      if (is_2d = IS_2D (xf->positions_array, type, rank, shape))
	pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array) ;
      else
	points = (Point *) DXGetArrayData(xf->positions_array) ;
      
      /* get strip topology */
      connections = (int *)DXGetArrayData(xf->connections_array) ;
      strips = (int (*)[2])DXGetArrayData(xf->meshes) ;
      numStrips = xf->nmeshes ;
      
      DebugMessage() ;
      
      /* allocate space for Starbase strip data */
      stripsSB = (tdmStripArraySB *) tdmAllocate(sizeof(tdmStripArraySB)) ;
      if (!stripsSB)
	{
	  PRINT(("out of memory allocating strip structure"));
	  DXErrorGoto (ERROR_INTERNAL, "#13000") ;
	}
      
      stripsSB->stripArray = 0 ;
      stripsSB->num_strips = 0 ;
      
      /* allocate array of Starbase strips */
      stripArray =
	stripsSB->stripArray = (tdmTmeshCacheSB *)
	  tdmAllocate(numStrips*sizeof(tdmTmeshCacheSB));
      
      if (!stripArray)
	{
	  PRINT(("out of memory allocating array of strips"));
	  DXErrorGoto (ERROR_INTERNAL, "#13000") ;
	}
      
      for ( ; stripsSB->num_strips < numStrips ; stripsSB->num_strips++)
	{
	  /* each iteration constructs and draws one Starbase strip */
	  register float *clist = 0, *gnormals = 0 ;
	  register int i, dV, *pntIdx, numPnts ;
	  
	  stripArray->clist = 0 ;
	  stripArray->gnormals = 0 ;
	  
	  /* get the number of points in this strip */
	  numPnts = strips[stripsSB->num_strips][1] ;
	  
	  /* allocate coordinate list */
	  stripArray->clist = clist =
	    (float *) tdmAllocate(numPnts*vsize*sizeof(float)) ;
	  
	  if (!clist)
	    {
	      PRINT(("out of memory allocating coordinate list"));
	      DXErrorGoto(ERROR_INTERNAL, "#13000") ;
	    }
	  
	  /* get the sub-array of connections making up this strip */
	  pntIdx = &connections[strips[stripsSB->num_strips][0]] ;
	  
	  /* copy vertex coordinates into clist */
	  if (is_2d)
	    for (i=0, dV=0 ; i<numPnts ; i++, dV+=vsize)
	      {
		*(struct p2d *)(clist+dV) = pnts2d[pntIdx[i]] ;
		((Point *)(clist+dV))->z = 0 ;
	      }
	  else
	    for (i=0, dV=0 ; i<numPnts ; i++, dV+=vsize)
	      *(Point *)(clist+dV) = points[pntIdx[i]] ;
	  
	  /* copy vertex colors */
	  if (vertex_flags & VERTEX_COLOR)
	    if (color_map)
	      for (i=0, dV=cOffs ; i<numPnts ; i++, dV+=vsize)
		*(RGBColor *)(clist+dV) =
		  color_map[((char *)fcolors)[pntIdx[i]]] ;
	    else
	      for (i=0, dV=cOffs ; i<numPnts ; i++, dV+=vsize)
		*(RGBColor *)(clist+dV) = fcolors[pntIdx[i]] ;
	  
	  /* save other strip info */
	  stripArray->numverts = numPnts ;
	  stripArray->numcoords = vsize-3 ;
	  stripArray->vertex_flags = vertex_flags ;
	  stripArray->facet_flags = facet_flags ;
	  
	  polyline_with_data3d (FILDES, clist, numPnts,
				vsize-3, vertex_flags, NULL) ;
	  
	  /* increment strip */
	  stripArray++ ;
	}
      
      /* cache strip array */
      tdmPutTmeshCacheSB (cache_id, xf, stripsSB) ;
    }

  /* restore hidden surface OFF */
  hidden_surface(FILDES, FALSE, FALSE) ;
  EXIT(("OK"));
  return OK ;

 error:
  tdmFreeTmeshCacheSB((Pointer)stripsSB) ;
  hidden_surface(FILDES, FALSE, FALSE) ;
  EXIT(("ERROR"));
  return ERROR ;
}

#endif /*  hwPlineDrawSB_c */
