/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwLineDrawSB.c,v $
  Author: John Vergo

\*---------------------------------------------------------------------------*/

#define _HP_FAST_MACROS 1
#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwTmesh.h"
#include "hwPortSB.h"
#include "hwXfield.h"
#include "hwCacheUtilSB.h"
#include "hwMemory.h"

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
#else
#define PrintBounds() {}
#endif



int
_dxfLineDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  register Point *points ;
  register RGBColor *fcolors, *color_map ;
  register float *clist = 0 ;
  register int i, k, vsize, dV, *connections ;
  int numpts, num_coords, mod, prev_point, start, cOffs, mdOffs, vertex_flags ;
  int type, rank, shape, is_2d ;
  struct p2d {float x, y ;} *pnts2d ;
  enum approxE approx ;
  int	nshapes;
  
  DEFPORT(portHandle) ;

  ENTRY(("_dxfLineDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));
  
  PRINT(("%d invalid connections",
	 xf->invCntns? DXGetInvalidCount(xf->invCntns): 0));
  
  /*
   *  Extract required data from the xfield.
   */
 
  if (is_2d = IS_2D (xf->positions_array, type, rank, shape))
      pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array) ;
  else
      points = (Point *) DXGetArrayData(xf->positions_array) ;

  PrintBounds() ;

  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;

  if (xf->origConnections_array)
    {
      /* connections array had been transformed into strips */
      connections = (int *) DXGetArrayData (xf->origConnections_array) ;
      nshapes = xf->origNConnections ;
    }
  else
    {
      connections = (int *) DXGetArrayData (xf->connections_array) ;
      nshapes = xf->nconnections ;
    }
  
  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
      fcolors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL) ;
  else
      fcolors = (Pointer) DXGetArrayData(xf->fcolors_array) ;
  
  if (buttonUp)
    {
      mod = xf->attributes.buttonUp.density ;
      approx = xf->attributes.buttonUp.approx ;
    }
  else
    {
      mod = xf->attributes.buttonDown.density ;
      approx = xf->attributes.buttonDown.approx ;
    }
  
  vertex_flags = 0 ;
  numpts = 0 ;
  hidden_surface (FILDES, TRUE, FALSE) ;

  switch (approx)
    {
    case approx_dots:
#ifdef ALLOW_LINES_APPROX_BY_DOTS
      marker_type (FILDES, 0) ;
      if (xf->colorsDep == dep_field)
	{
	  SET_COLOR (marker_color, 0) ;
	  if (mod == 1 && !xf->invPositions)
	    {
	      /* fast dot approximation */
	      if (is_2d)
		  polymarker2d (FILDES, (float *)pnts2d, xf->npositions, 0) ;
	      else
		  polymarker3d (FILDES, (float *)points, xf->npositions, 0) ;
	      EXIT(("OK"));
	      return OK ;
	    }
	  num_coords = 0 ;
	  vsize = 3 ;
	}
      else
	{
	  num_coords = 3 ;
	  vsize = 6 ;
	  cOffs = 3 ;
	  vertex_flags = VERTEX_COLOR ;
	}

      /* allocate coordinate list */
      clist = (float *) tdmAllocate (nshapes*2*vsize*sizeof(float)) ;
      if (!clist) DXErrorGoto (ERROR_INTERNAL, "#13000") ;
      
      prev_point = -1 ;
      for (k = 0 ; k < nshapes ; k += mod)
	{
	  if (xf->invCntns && !DXIsElementValid (xf->invCntns, k))
	      /* skip invalid connections */
	      continue ;
	  
	  if (connections[2*k] != prev_point)
	      start = 0 ;
	  else
	      start = 1 ;

	  /* copy vertex coordinates into clist */
	  if (is_2d)
	      for (i=start, dV=numpts*vsize ; i<2 ; i++, dV+=vsize)
		{
		  *(struct p2d *)(clist+dV) = pnts2d[connections[2*k + i]] ;
		  ((Point *)(clist+dV))->z = 0 ;
		}
	  else
	      for (i=start, dV=numpts*vsize ; i<2 ; i++, dV+=vsize)
		  *(Point *)(clist+dV) = points[connections[2*k + i]] ;

	  if (xf->colorsDep == dep_positions)
	    {
	      if (color_map)
		  for (i=start, dV=numpts*vsize+cOffs ; i<2 ; i++, dV+=vsize)
		      *(RGBColor *)(clist+dV) =
			  color_map[((char *)fcolors)[connections[2*k + i]]] ;
	      else
		  for (i=start, dV=numpts*vsize+cOffs ; i<2 ; i++, dV+=vsize)
		      *(RGBColor *)(clist+dV) =
			  fcolors[connections[2*k + i]] ;
	    }
	  else if (xf->colorsDep == dep_connections)
	    {
	      if (color_map)
		  for (i=start, dV=numpts*vsize+cOffs ; i<2 ; i++, dV+=vsize)
		      *(RGBColor *)(clist+dV) =
			  color_map[((char *)fcolors)[k]] ;
	      else
		  for (i=start, dV=numpts*vsize+cOffs ; i<2 ; i++, dV+=vsize)
		      *(RGBColor *)(clist+dV) = fcolors[k] ;
	    }
	  numpts += (2-start) ;
	  prev_point = connections[2*k + 1] ;
	}

      polymarker_with_data3d 
	  (FILDES, clist, numpts, num_coords, vertex_flags) ;

      break;
#endif
    case approx_none:
    case approx_lines:
    default:
      if (xf->colorsDep == dep_field)
	{
	  num_coords = 1 ;
	  vsize = 4 ;
	  mdOffs = 3 ;
	  SET_COLOR(line_color, 0) ;
	}
      else
	{
	  num_coords = 4 ;
	  vsize = 7 ;
	  cOffs = 3 ;
	  mdOffs = 6 ;
	  vertex_flags = VERTEX_COLOR ;
	}

      vertex_flags |= MD_FLAGS ;

      /* allocate coordinate list */
      clist = (float *) tdmAllocate(nshapes*2*vsize*sizeof(float)) ;
      if (!clist) DXErrorGoto(ERROR_INTERNAL, "#13000") ;
      
      for (k = 0; k < nshapes; k += mod)
	{
	  if (xf->invCntns && !DXIsElementValid (xf->invCntns, k))
	      /* skip invalid connections */
	      continue ;

	  /* copy vertex coordinates into clist */
	  if (is_2d)
	      for (i=0, dV=numpts*vsize ; i<2 ; i++, dV+=vsize)
		{
		  *(struct p2d *)(clist+dV) = pnts2d[connections[2*k + i]] ;
		  ((Point *)(clist+dV))->z = 0 ;
		}
	  else
	      for (i=0, dV=numpts*vsize ; i<2 ; i++, dV+=vsize)
		  *(Point *)(clist+dV) = points[connections[2*k + i]] ;
	  
	  /* set up move_draw flags; move=0; draw = 1 */
	  for (i=0, dV=numpts*vsize ; i<2 ; i++, dV+=vsize)
	      *(clist+dV+mdOffs) = i ;
	  
	  if (xf->colorsDep == dep_positions)
	    {
	      if (color_map)
		  for (i=0, dV=numpts*vsize+cOffs ; i<2 ; i++,dV+=vsize)
		      *(RGBColor *)(clist+dV) =
			  color_map[((char *)fcolors)[connections[2*k + i]]];
	      else
		  for (i=0, dV=numpts*vsize+cOffs ; i<2 ; i++,dV+=vsize)
		      *(RGBColor *)(clist+dV) =
			  fcolors[connections[2*k + i]] ;
	    }
	  else if (xf->colorsDep == dep_connections)
	    {
	      if (color_map)
		  for (i=0, dV=numpts*vsize+cOffs ; i<2 ; i++,dV+=vsize)
		      *(RGBColor *)(clist+dV) =
			  color_map[((char *)fcolors)[k]] ;
	      else
		  for (i=0, dV=numpts*vsize+cOffs ; i<2 ; i++,dV+=vsize)
		      *(RGBColor *)(clist+dV) = fcolors[k] ;
	    }
	  numpts += 2 ;
	}

      polyline_with_data3d 
	  (FILDES, clist, numpts, num_coords, vertex_flags, NULL) ;

      break ;
    }

  if (clist) tdmFree(clist) ;
  hidden_surface (FILDES, FALSE, FALSE) ;
  EXIT(("OK"));
  return OK ;

 error:
  if (clist) tdmFree(clist) ;
  hidden_surface (FILDES, FALSE, FALSE) ;
  EXIT(("ERROR"));
  return ERROR ;
}
