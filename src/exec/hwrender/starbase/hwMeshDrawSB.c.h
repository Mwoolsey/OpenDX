/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef hwMeshDrawSB_c_h
#define hwMeshDrawSB_c_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwMeshDrawSB.c.h,v $
  Author: Mark Hood

  This routine takes a DX field, looks up the previously generated Starbase
  triangle strips, and draws them if they already exist in the executive
  cache.  Otherwise it obtains the generic strip topology from the xfield
  data structure, generates Starbase triangle strips, draws them, and then
  attempts to cache them for the next draw cycle.  This is the fast path to
  the hardware for opaque triangle and quad fields with data dependent upon
  positions.

  Starbase does not allow strips to use the FACET_COLOR flag. Since vertex
  data is shared from facet to facet, it is otherwise impossible to provide
  a uniform color across each mesh facet.  The routine punts fields with
  colors dep connections and passes them to _dxfTriDraw() or _dxfQuadDraw().

  Starbase does not allow strips to have an opacity for each facet.  In
  addition, alpha blend per vertex is not currently supported on some HP
  hardware, such as the CRX-24(Z).  Any fields with opacity data are passed
  on to _dxfTriDraw() or _dxfQuadDraw().

  Throughput on the CRX-24 with r1.3 of _dxfTmeshDraw.c (which now simply
  includes the code in this file) has been measured at about 80K fully
  rendered triangles/sec. [24 frames in 10 seconds using interop.net with DX
  in script mode. 33,000 total triangles in the field; each triangle had a
  normal and rgb color at each vertex; all strips were in cache.  Rendering
  conditions were one directional light source and one ambient, specularity
  on, no backface culling, no depth cueing and an orthogonal view transform
  in an unobscured window.  If caching is disabled, performance drops to
  about 16 frames in 10 seconds (53K triangles/sec), so caching seems to
  offer a 50% performance increase].

\*---------------------------------------------------------------------------*/

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
tdmMeshDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
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

  ENTRY(("tdmMeshDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));

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

  if ((xf->opacities && approx == approx_none) ||
      (xf->colorsDep == dep_connections))
    {
      int status ;
      PRINT(("drawing as individual polygons"));
      status = tdmPolygonDraw (portHandle, xf, buttonUp) ;
      EXIT(("status = %d", status));
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

  /*
   *  Set up rendering attributes based on approximation method.
   */

  switch (approx)
    {
    case approx_dots:
      if (xf->colorsDep == dep_field && !xf->invPositions &&
          !(_dxf_isFlagsSet(_dxf_attributeFlags(&xf->attributes),BEING_CLIPPED))) 
	{
	  float *pts ;
	  /* fast dot approximation */
	  PRINT(("using polymarker"));
	  
	  marker_type (FILDES, 0) ;
	  SET_COLOR (marker_color, 0) ;
	  pts = DXGetArrayData(xf->positions_array) ;

	  if (IS_2D (xf->positions_array, type, rank, shape))
	      polymarker2d (FILDES, pts, xf->npositions, 0) ;
	  else
	      polymarker3d (FILDES, pts, xf->npositions, 0) ;

	  EXIT(("OK"));
	  return OK ;
	}
      /*
       *  The INT_POINT interior fill style, for position-dependent
       *  colors, is faster than using polymarkers since polymarkers do
       *  not allow colors imbedded within the coordinate list.
       */
      shade_mode (FILDES, CMAP_FULL, FALSE) ;
      hidden_surface (FILDES, FALSE, FALSE) ;
      alpha_transparency (FILDES, 0, 0, 0.0, 0.0) ;
      interior_style (FILDES, INT_POINT, FALSE) ;
      break ;

    case approx_lines:
      if (xf->colorsDep != dep_field)
	  /*
	   *  Dense fields of varying colors are visually confusing
	   *  without hidden surface, even with wireframe approximation.
	   */
	  hidden_surface (FILDES, TRUE, FALSE) ;
      break ;

    case approx_none:
    default:
      if (normals)
	{
	  /* turn on lighting */
	  surface_coefficients (FILDES,
				xf->attributes.front.ambient,
				xf->attributes.front.diffuse,
				xf->attributes.front.specular) ;
	  shade_mode (FILDES, CMAP_FULL, TRUE) ;
	}
      else
	  shade_mode (FILDES, CMAP_FULL, FALSE) ;

      hidden_surface (FILDES, TRUE, FALSE) ;
      alpha_transparency (FILDES, 0, 0, 0.0, 0.0) ;
      interior_style (FILDES, INT_SOLID, FALSE) ;
      break ;
    }

  if (xf->colorsDep == dep_field)
    {
      /* render field in constant color */
      cache_id = CpfMsh ;
      SET_COLOR(fill_color, 0) ;
      SET_COLOR(line_color, 0) ;
    }
  else if (approx == approx_flat)
    {
      /* cache leaves out normals to achieve flat shading */
      cache_id = CpcMsh ;
    }
  else
    {
      /* normals kept in cache if supplied */
      cache_id = CpvMsh ;
    }

  /*
   *  Render strips.
   */

  PRINT(("cache_id = %s", cache_id));
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

      if (approx == approx_lines)
	  for ( ; stripArray < end ; stripArray++)
	    {
	      polyline_with_data3d
		  (FILDES, stripArray->clist, stripArray->numverts,
		   stripArray->numcoords, stripArray->vertex_flags, NULL) ;
	    }
      else
	  for ( ; stripArray < end ; stripArray++)
	    {
	      triangular_strip_with_data
		  (FILDES, stripArray->clist, stripArray->numverts,
		   stripArray->gnormals, stripArray->numcoords,
		   stripArray->vertex_flags, stripArray->facet_flags) ;
	    }
    }
  else
    {
      /*
       *  Construct an array of Starbase triangle strips and cache it.
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

      if (normals && approx != approx_flat)
	  /* flat shading (approx_flat) is forced by withholding normals */
	  if (xf->normalsDep == dep_positions)
	    {
	      /* vertex has 3 more floats, with normal 3rd from end */
	      vsize += 3 ; nOffs = vsize - 3 ;
	      vertex_flags |= VERTEX_NORMAL ;
	    }
	  else
	      facet_flags |= FACET_NORMAL ;

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
	  /* each iteration constructs and draws one Starbase triangle strip */
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
			  color_map[((unsigned char *)fcolors)[pntIdx[i]]] ;
	      else
		  for (i=0, dV=cOffs ; i<numPnts ; i++, dV+=vsize)
		      *(RGBColor *)(clist+dV) = fcolors[pntIdx[i]] ;

	  /* copy vertex normals */
	  if (vertex_flags & VERTEX_NORMAL)
	      for (i=0, dV=nOffs ; i<numPnts ; i++, dV+=vsize)
		  *(Vector *)(clist+dV) = normals[pntIdx[i]] ;

	  /* allocate geometric normals */
	  if (facet_flags & FACET_NORMAL)
	    {
	      stripArray->gnormals = gnormals =
		  (float *)tdmAllocateLocal((numPnts-2)*sizeof(Vector));

	      if (!gnormals)
		{
		  /* make sure cleanup deallocates clist */
		  stripsSB->num_strips++ ;
		  PRINT(("out of memory allocating geometric normals"));
		  DXErrorGoto(ERROR_INTERNAL, "#13000") ;
		}

	      /* copy geometric normals */
	      if (xf->normalsDep == dep_connections)
		  for (i = 0 ; i < numPnts - 2 ; i++)
		      ((Vector *)gnormals)[i] = normals[pntIdx[i]] ;
	      else
		  /* dep field */
		  for (i = 0 ; i < numPnts - 2 ; i++)
		      ((Vector *)gnormals)[i] = *normals ;
	    }

	  /* save other strip info */
	  stripArray->numverts = numPnts ;
	  stripArray->numcoords = vsize-3 ;
	  stripArray->vertex_flags = vertex_flags ;
	  stripArray->facet_flags = facet_flags ;

	  /* send strip to Starbase */
	  if (approx == approx_lines)
	    {
	      polyline_with_data3d (FILDES, clist, numPnts,
				    vsize-3, vertex_flags, NULL) ;
	    }
	  else
	    {
	      triangular_strip_with_data (FILDES, clist, numPnts, gnormals,
					  vsize-3, vertex_flags, facet_flags) ;
	    }

	  /* increment strip */
	  stripArray++ ;
	}

      /* cache strip array */
      tdmPutTmeshCacheSB (cache_id, xf, stripsSB) ;
    }

  /* restore hidden surface OFF */
  hidden_surface(FILDES, FALSE, FALSE) ;
  EXIT((""));
  return OK ;

 error:
  tdmFreeTmeshCacheSB((Pointer)stripsSB) ;
  hidden_surface(FILDES, FALSE, FALSE) ;
  EXIT(("ERROR"));
  return ERROR ;
}

#endif /*  hwMeshDrawSB_c_h */
