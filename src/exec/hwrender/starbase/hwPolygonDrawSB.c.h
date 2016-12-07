/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*********************************************************************/
/*                     I.B.M. CONFIENTIAL                           */
/*********************************************************************/

#include <dxconfig.h>


#ifndef hwPolygonDrawSB_c_h
#define hwPolygonDrawSB_c_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwPolygonDrawSB.c.h,v $

  This code implements unmeshed triangle and quad fields as Starbase
  polygons. 

  Vertex opacities are not supported by some HP hardware, such as the
  current CRX-24.  In these cases the first vertex opacity of each polygon
  is applied to the entire polygon.

\*---------------------------------------------------------------------------*/

/* sDepths needs to be accessed by polysort() */
static float *sDepths = 0 ;
#define DEPTH(Z,p) (Z.x*p.x + Z.y*p.y + Z.z*p.z)
#define DEPTH2D(Z,p) (Z.x*p.x + Z.y*p.y)

/* make a sort routine called polysort() */
#define QUICKSORT polysort
#define TYPE int
#define LT(a,b) (sDepths[*(a)] < sDepths[*(b)])
#define GT(a,b) (sDepths[*(a)] > sDepths[*(b)])
#include "qsort.c"

/*
 *  CRX-24 opacities seem to be quite different from DX software renderer
 *  at the low end of the scale.  A great deal of color resolution seems
 *  to be lost, possibly because of the 12/12 double buffering instead of
 *  a more comfortable 24-bit resolution.  The lookup table below
 *  approximates the square root of the opacity up to 0.07, then increases
 *  linearly up to 1.0.
 */

static float alpha_lookup[] = 
{.00, .10, .14, .17, .20, .22, .24, .26, .27, .28,
 .29, .30, .30, .31, .32, .33, .34, .34, .35, .36,
 .37, .38, .38, .39, .40, .41, .41, .42, .43, .44,
 .45, .45, .46, .47, .48, .49, .49, .50, .51, .52,
 .53, .53, .54, .55, .56, .57, .57, .58, .59, .60,
 .60, .61, .62, .63, .64, .64, .65, .66, .67, .68,
 .68, .69, .70, .71, .72, .72, .73, .74, .75, .75,
 .76, .77, .78, .79, .79, .80, .81, .82, .83, .83,
 .84, .85, .86, .87, .87, .89, .89, .90, .91, .91,
 .92, .93, .94, .94, .95, .96, .97, .98, .98, .99, 1.00} ;  

#define LOOKUP(f) alpha_lookup[(int)((f)*100)]

#ifdef MJHDEBUG
#define DebugMessage()                                                        \
{                                                                             \
  int i ;								      \
  float outx, outy, outz ;						      \
  float minx, maxx, miny, maxy, minz, maxz ;				      \
									      \
  fprintf (stderr, "\nnumber of points: %d", xf->npositions) ;   	      \
  if (xf->origConnections_data)                                               \
      fprintf (stderr, "\nnumber of connections: %d", xf->origNConnections) ; \
  else                                                                        \
      fprintf (stderr, "\nnumber of connections: %d", xf->nconnections) ;     \
  fprintf (stderr, "\n%d invalid connections",                                \
	  xf->invCntns? DXGetInvalidCount(xf->invCntns): 0) ;                 \
  fprintf (stderr, "\ncolors: 0x%x, color_map: 0x%x, dep on pos: %d",	      \
	   (int)fcolors, (int)color_map, xf->colorsDep == dep_positions) ;    \
  fprintf (stderr, "\nopacities: 0x%x, opacity_map: 0x%x, dep on pos: %d",    \
	   (int)opacities, (int)opacity_map,                                  \
	   xf->opacitiesDep == dep_positions) ;    		              \
  fprintf (stderr, "\nnormals: 0x%x, dep on pos: %d",			      \
	   (int)normals, xf->normalsDep == dep_positions) ;	              \
  fprintf (stderr, "\nambient, diffuse, specular: %f %f %f",		      \
	   xf->attributes.front.ambient,                                      \
	   xf->attributes.front.diffuse,                                      \
	   xf->attributes.front.specular) ;                                   \
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
#define DebugMessage() {}
#endif /* DEBUG */



int
tdmPolygonDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  Point *points ;
  Vector *normals ;
  RGBColor *fcolors, *color_map ;
  float *opacities, *opacity_map ;
  int nshapes, *connections, *sortArray = 0, status, mod ;
  int cOffs, nOffs, oOffs, numPolys, vertex_flags, facet_flags ;
  int type, rank, shape, is_2d ;
  struct p2d {float x, y ;} *pnts2d ;
  char *cache_id ;
  enum approxE approx ;

  register int i, j, vsize, pstride, dP, dV, nverts, fsize, dF ;
  
  DEFPORT(portHandle) ;

  ENTRY(("tdmPolygonDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));
  
  /*
   *  Extract required data from the xfield.
   */
  
  if (is_2d = IS_2D (xf->positions_array, type, rank, shape))
      pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array) ;
  else
      points = (Point *) DXGetArrayData(xf->positions_array) ;

  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;
  opacity_map = (float *) DXGetArrayData(xf->omap_array) ;
  
  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
      fcolors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL) ;
  else
      fcolors = (Pointer) DXGetArrayData(xf->fcolors_array) ;
  
  if (DXGetArrayClass(xf->opacities_array) == CLASS_CONSTANTARRAY)
      opacities = (Pointer) DXGetArrayEntry(xf->opacities, 0, NULL) ;
  else
      opacities = (Pointer) DXGetArrayData(xf->opacities_array) ;
  
  if (DXGetArrayClass(xf->normals_array) == CLASS_CONSTANTARRAY)
      normals = (Pointer) DXGetArrayEntry(xf->normals, 0, NULL) ;
  else
      normals = (Pointer) DXGetArrayData(xf->normals_array) ;
  
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
  
  DebugMessage() ;
  
  /*
   *  Set up rendering attributes.
   */
  
  switch (approx)
    {
    case approx_dots:
      if (xf->colorsDep == dep_field && mod == 1 && !xf->invPositions &&
          !(_dxf_isFlagsSet(_dxf_attributeFlags(&xf->attributes),BEING_CLIPPED))) 
	{
	  /* fast dot approximation */
	  PRINT(("using polymarker"));
	  marker_type (FILDES, 0) ;
	  SET_COLOR (marker_color, 0) ;
	  if (is_2d)
	      polymarker2d (FILDES, (float *)pnts2d, xf->npositions, 0) ;
	  else
	      polymarker3d (FILDES, (float *)points, xf->npositions, 0) ;
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
      
      if (opacities)
	{
	  if (xf->opacitiesDep == dep_field)
	    {
	      float opacity ; 
	      if (opacity_map)
		  opacity = opacity_map[*(char *)opacities] ;
	      else
		  opacity = *opacities ;
	      
	      alpha_transparency (FILDES, 1, 1, LOOKUP(opacity), 1.0) ;
	    }
	}
      else
	  alpha_transparency (FILDES, 0, 0, 0.0, 0.0) ;
      
      hidden_surface (FILDES, TRUE, FALSE) ;
      interior_style (FILDES, INT_SOLID, FALSE) ;
      break ;
    }
  
  if (xf->colorsDep == dep_field)
    {
      /* render field in constant color */
      cache_id = CpfPolygon ;
      SET_COLOR(fill_color, 0) ;
      SET_COLOR(line_color, 0) ;
    }
  else if (approx == approx_flat)
    {
      /* leave out normals to achieve flat shading */
      cache_id = CpcPolygon ;
    }
  else
    {
      /* keep normals if supplied */
      cache_id = CpvPolygon ;
    }
  
  /*
   *  Generate Starbase polygons and render them.
   */

  vertex_flags = 0 ;
  facet_flags = UNIT_NORMALS ;
  vsize = 3 ;		       /* number of floats of data per vertex */
  fsize = 0 ;		       /* number of floats of data per facet */
  numPolys = nshapes/mod ;     /* number of polygons to draw */
  pstride = PolygonSize*mod ;  /* number of indices to next polygon */
  
  nverts = (approx == approx_lines ? PolygonSize+1 : PolygonSize) ;
  
  PRINT(("cache_id = %s", cache_id));
  PRINT(("%d polygons", numPolys));

  if (fcolors && xf->colorsDep != dep_field)
      if (xf->colorsDep == dep_positions)
	{
	  /* vertex has at least 6 floats, with color at float 3 */
	  vsize = 6 ; cOffs = 3 ;
	  vertex_flags |= VERTEX_COLOR ;
	}
      else
	{
	  /* facet has at least 3 floats, with color at float 0 */
	  fsize = 3 ; cOffs = 0 ;
	  facet_flags |= FACET_COLOR ;
	}
  
  if (normals && approx != approx_flat)
      /*
       *  Flat shading (approx_flat, now defunct) is forced by withholding
       *  normals.
       */
      if (xf->normalsDep == dep_positions)
	{
	  /* vertex has 3 more floats, with normal 3rd from end */
	  vsize += 3 ; nOffs = vsize - 3 ;
	  vertex_flags |= VERTEX_NORMAL ;
	}
      else
	{
	  /* facet has 3 more floats, with normal 3rd from end */
	  fsize += 3 ; nOffs = fsize - 3 ;
	  facet_flags |= FACET_NORMAL ;
	}
  
#if (DXD_HW_ALPHA_POSNS == 1)
  /* hardware supports alpha interpolation */
  if (opacities &&
      xf->opacitiesDep == dep_positions && approx == approx_none)
    {
      /* vertex has one more float, with opacity at end */
      vsize += 1 ; oOffs = vsize - 1 ;
      vertex_flags |= VERTEX_BLEND ;
    }
#endif
  
  sortArray = (int *) tdmAllocate(numPolys*sizeof(int)) ;
  if (!sortArray)
      DXErrorGoto(ERROR_INTERNAL, "#13000") ;
  
  if (opacities && approx == approx_none)
    {
      /* compute depths and sort back-to-front */
      Vector Z ;
      float II[4][4] ;
      float I[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}} ;
      
      sDepths = (float *) tdmAllocate(numPolys*sizeof(float)) ;
      if (!sDepths)
	  DXErrorGoto(ERROR_INTERNAL, "#13000") ;
      
      /* get view xform (Starbase thinks it's the modeling xform) */
      transform_points (FILDES, (float *)I, (float *)II, 4, 1) ;
      Z.x = II[0][2] ; Z.y = II[1][2] ; Z.z = II[2][2] ;
      
      if (is_2d)
	  for (i=0, dP=0 ; i<numPolys ; i++, dP+=pstride)
	    {
	      sortArray[i] = i ;
	      sDepths[i] =
		  (DEPTH2D(Z, pnts2d[connections[dP+1]])+
		   DEPTH2D(Z, pnts2d[connections[dP+2]])+
#if (PolygonSize == 4)
		   DEPTH2D(Z, pnts2d[connections[dP+3]])+
#endif
		   DEPTH2D(Z, pnts2d[connections[dP+0]])) / PolygonSize ;
	    }
      else
	  for (i=0, dP=0 ; i<numPolys ; i++, dP+=pstride)
	    {
	      sortArray[i] = i ;
	      sDepths[i] =
		  (DEPTH(Z, points[connections[dP+1]])+
		   DEPTH(Z, points[connections[dP+2]])+
#if (PolygonSize == 4)
		   DEPTH(Z, points[connections[dP+3]])+
#endif
		   DEPTH(Z, points[connections[dP+0]])) / PolygonSize ;
	    }
      polysort (sortArray, numPolys) ;
    }
  else
      /* render opaque polygons in field order */
      for (i=0 ; i<numPolys ; i++) sortArray[i] = i ;
  
  for (j=0 ; j<numPolys ; j++)
    {
      register int v, vi ;
      register float *clist ;
      float cbuff[128] ;
      
      i = sortArray[j] * mod ;
      if (xf->invCntns && !DXIsElementValid(xf->invCntns, i))
	  /* skip invalid connections */
	  continue ;
      
      dP = i * PolygonSize ;
      clist = cbuff ;
      
      if ((facet_flags & FACET_NORMAL) && approx != approx_lines)
	{
	  /* first vertex is really facet normal */
	  if (xf->normalsDep == dep_positions)
	      *(Vector *)clist = normals[connections[dP]] ;
	  else if (xf->normalsDep == dep_connections)
	      *(Vector *)clist = normals[i] ;
	  else
	      /* dep field */
	      *(Vector *)clist = *normals ;
	  
	  clist += vsize ;
	}
      
      if (facet_flags & FACET_COLOR)
	{
	  if (approx == approx_lines)
	      SET_COLOR (line_color, i) ;
	  else
	      SET_COLOR (fill_color, i) ;
	}
      
      if (opacities && approx == approx_none)
	{
	  float a ;
#if (DXD_HW_ALPHA_POSNS == 0)
	  if (xf->opacitiesDep == dep_positions)
	    {
	      /*
	       *  Position-dependent opacities are not supported by the
	       *  hardware, so approximate the rendering by applying the
	       *  opacity of first vertex to the entire facet.
	       */
	      if (opacity_map)
		  a = opacity_map[((char *)opacities)[connections[dP]]];
	      else
		  a = opacities[connections[dP]] ;
	      
	      alpha_transparency (FILDES, 1, 1, LOOKUP(a), 1.0) ;
	    }
#endif
	  if (xf->opacitiesDep == dep_connections)
	    {
	      if (opacity_map)
		  a = opacity_map[((char *)opacities)[i]] ;
	      else
		  a = opacities[i] ;
	      
	      alpha_transparency (FILDES, 1, 1, LOOKUP(a), 1.0) ;
	    }
	}
      
      /* copy vertex data */
      for (vi=0 ; vi<nverts ; vi++)
	{
#if (PolygonSize == 4)
	  switch (vi)
	    {
	    case 0: v = dP + 0 ; break ;
	    case 1: v = dP + 2 ; break ;
	    case 2: v = dP + 3 ; break ;
	    case 3: v = dP + 1 ; break ;
	    case 4: v = dP + 0 ; break ;
	    }
#else
	  switch (vi)
	    {
	    case 0: v = dP + 0 ; break ;
	    case 1: v = dP + 1 ; break ;
	    case 2: v = dP + 2 ; break ;
	    case 3: v = dP + 0 ; break ;
	    }
#endif
	  if (is_2d)
	    {
	      *(struct p2d *)(clist) = pnts2d[connections[v]] ;
	      ((Point *)clist)->z = 0 ;
	    }
	  else
	      *(Point *)(clist) = points[connections[v]] ;

	  clist+=3 ;
	  
	  if (vertex_flags & VERTEX_COLOR)
	    {
	      /* copy vertex color into clist */
	      if (color_map)
		  *(RGBColor *)(clist) = 
		      color_map[((char *)fcolors)[connections[v]]] ;
	      else
		  *(RGBColor *)(clist) = fcolors[connections[v]] ;
	      
	      clist+=3 ;
	    }
	  
	  if (vertex_flags & VERTEX_NORMAL)
	    {
	      /* copy vertex normal into clist */
	      *(Vector *)(clist) = normals[connections[v]] ;
	      
	      clist+=3 ;
	    }
	  
	  if (vertex_flags & VERTEX_BLEND)
	      /* copy vertex opacity into clist */
	      if (opacity_map)
		  *clist++ =
		      LOOKUP(opacity_map[((char *)opacities)[connections[v]]]) ;
	      else
		  *clist++ =
		      LOOKUP(opacities[connections[v]]) ;
	}
      
      if (approx == approx_lines)
	{
	  polyline_with_data3d (FILDES, cbuff, nverts,
				vsize-3, vertex_flags, NULL) ;
	}
      else
	{
	  polygon_with_data3d (FILDES, cbuff, nverts, vsize-3,
			       vertex_flags, facet_flags & ~FACET_COLOR) ;
	}
    }
  status = OK ;
  goto done ;
  
 error:
  status = ERROR ;

 done:
  if (sDepths) tdmFree(sDepths) ;
  if (sortArray) tdmFree(sortArray) ;
  sDepths = 0 ;      
  sortArray = 0 ;
  hidden_surface(FILDES, FALSE, FALSE) ;

  EXIT(("%s", status==OK ? "": "ERROR"));
  return status ;
}

#endif /*  hwPolygonDrawSB_c_h */
