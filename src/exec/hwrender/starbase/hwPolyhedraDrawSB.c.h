/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef hwPolyhedrawDraw_c_h
#define hwPolyhedrawDraw_c_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwPolyhedraDrawSB.c.h,v $
  Author: Mark Hood

\*---------------------------------------------------------------------------*/

#include "hwXfield.h"

/*#define USE_POLYHEDRA*/
/*#define COMPOSITE*/

typedef struct
{
  int numFacets ;
  int numVertices ;
  int index[VerticesPerFacet] ;
} FacetSet ;

#ifdef COMPOSITE
/*
 *  Make a sort routine called polysort() with access to some global data.
 */

static Point Z, *sPts ;
static int *sConnect, *sortArray ;
static float II[4][4], I[4][4] = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}, {0,0,0,1}} ;

#define TYPE int
#define LT(a,b) (DXDot(Z,sPts[sConnect[*(a)]]) < DXDot(Z,sPts[sConnect[*(b)]]))
#define GT(a,b) (DXDot(Z,sPts[sConnect[*(a)]]) > DXDot(Z,sPts[sConnect[*(b)]]))
#define QUICKSORT polysort
#include "qsort.c"
#endif

#ifdef DEBUG
#define DebugMessage()                                                        \
{                                                                             \
  int i ;								      \
  float outx, outy, outz ;						      \
  float minx, maxx, miny, maxy, minz, maxz ;				      \
									      \
  fprintf (stderr, "\nnumber of points: %d", xf->npositions) ;   	      \
  fprintf (stderr, "\n%d invalid positions",                                  \
	  xf->invPositions? DXGetInvalidCount(xf->invPositions): 0) ;         \
  fprintf (stderr, "\nnumber of connections: %d", xf->nconnections) ;         \
  fprintf (stderr, "\n%d invalid connections",                                \
	  xf->invCntns? DXGetInvalidCount(xf->invCntns): 0) ;                 \
  fprintf (stderr, "\ncolors: 0x%x, color_map: 0x%x, dep on pos: %d",	      \
	   (int)fcolors, (int)color_map, xf->colorsDep == dep_positions) ;    \
									      \
  minx = miny = minz = +MAXFLOAT ;					      \
  maxx = maxy = maxz = -MAXFLOAT ;					      \
  for (i=0 ; i<xf->npositions ; i++)    				      \
    {									      \
      if (points[i].x < minx) minx = points[i].x ;			      \
      if (points[i].y < miny) miny = points[i].y ;			      \
      if (points[i].z < minz) minz = points[i].z ;			      \
      									      \
      if (points[i].x > maxx) maxx = points[i].x ;			      \
      if (points[i].y > maxy) maxy = points[i].y ;			      \
      if (points[i].z > maxz) maxz = points[i].z ;			      \
    }									      \
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
tdmPolyhedraDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  register float *clist = 0 ;
  register int i, j, vsize, dV, pstride, dP, npoints ;
  int nshapes, *connections, mod, cOffs, numPolys, vertex_flags ;
  tdmPolyhedraCacheSB *cache = 0 ;
  FacetSet ilist[FacetsPerPolyhedron] ;
  Point *points ;
  RGBColor *fcolors, *color_map ;
  char *cache_id ;
  enum approxE approx ;

  DEFPORT(portHandle) ;

  ENTRY(("tdmPolyhedraDraw(0x%x, 0x%x, %d)", portHandle, xf, buttonUp));

  /*
   *  Extract required data from the xfield.
   */
  
  points = (Point *) DXGetArrayData (xf->positions_array) ;
  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;
  
  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
    fcolors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL) ;
  else
    fcolors = (Pointer) DXGetArrayData(xf->fcolors_array) ;
  
  connections = (int *) DXGetArrayData (xf->connections_array) ;
  nshapes = xf->nconnections ;
  npoints = xf->npositions ;

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

  cOffs = 0 ;
  vsize = 3 ;                           /* number of floats per vertex */
  vertex_flags = 0 ;                    /* assume no extra vertex data */
  numPolys = nshapes/mod ;              /* number of polyhedra to draw */
  pstride = PolyhedraSize*mod ;         /* indices to next polyhedron */

  /* use z-buffer, cull backface */
  hidden_surface (FILDES, TRUE, TRUE) ; 
  
  /* volume primitives are rendered without lighting computations */
  shade_mode (FILDES, CMAP_FULL, FALSE) ;

  /* transparency is off */
  alpha_transparency (FILDES, 0, 0, 0.0, 0.0) ;

  switch (approx)
    {
    case approx_dots:  interior_style (FILDES, INT_POINT, FALSE) ;   break ;
    case approx_lines: interior_style (FILDES, INT_OUTLINE, FALSE) ; break ;
    default:
    case approx_none:  interior_style (FILDES, INT_SOLID, FALSE) ;   break ;
    }

  if (xf->colorsDep == dep_field)
    {
      cache_id = CpfPolyhedra ;
      SET_COLOR (fill_color, 0) ;
    }
  else if (xf->colorsDep == dep_connections)
    cache_id = CpcPolyhedra ;
  else
    cache_id = CpvPolyhedra ;
  
  PRINT(("cache_id = %s", cache_id));
  PRINT(("%d polyhedra to be rendered", numPolys));

  /* check vertex color */
  if (fcolors && xf->colorsDep == dep_positions)
    {
      /* vertex has at least 6 floats, with color at float 3 */
      vsize = 6 ; cOffs = 3 ;
      vertex_flags |= VERTEX_COLOR ;
    }
  
#ifdef USE_POLYHEDRA
  /* construct coordinate list */
  if (! vertex_flags)
    /* points array can be used for the coordinate list */
    clist = (float *) points ;
  else if (cache = tdmGetPolyhedraCacheSB (cache_id, 0, xf))
    /* use cached coordinate list */
    clist = cache->clist ;
  else
    {
      /* allocate coordinate list, copy vertex data */
      clist = (float *) tdmAllocate(npoints*vsize*sizeof(float)) ;
      if (!clist) DXErrorGoto(ERROR_INTERNAL, "#13000") ;
      
      /* copy vertex coordinates into clist */
      for (i=0, dV=0 ; i<npoints ; i++, dV+=vsize)
	*(Point *)(clist+dV) = points[i] ;
      
      if (vertex_flags & VERTEX_COLOR)
	/* copy vertex colors into clist */
	if (color_map)
	  for (i=0, dV=cOffs ; i<npoints ; i++, dV+=vsize)
	    *(RGBColor *)(clist+dV) = color_map[((char *)fcolors)[i]] ;
	else
	  for (i=0, dV=cOffs ; i<npoints ; i++, dV+=vsize)
	    *(RGBColor *)(clist+dV) = fcolors[i] ;
    }
#endif /* USE_POLYHEDRA */

#ifdef COMPOSITE
  /*
   *  sort polyhedra back to front
   */

  /* get view matrix (what Starbase thinks is the modeling transform) */
  transform_points (FILDES, (float *)I, (float *)II, 4, 1) ;
  Z.x = II[0][2] ; Z.y = II[1][2] ; Z.z = II[2][2] ;

  /* construct sort array containing indices into connections array */
  sortArray = (int *) tdmAllocate(numPolys*sizeof(int)) ;
  if (!sortArray) DXErrorGoto(ERROR_INTERNAL, "#13000") ;
  for (i=0, dP=0 ; i<numPolys ; i++, dP+=pstride)
    sortArray[i] = dP ;

  sPts = points ;
  sConnect = connections ;
  polysort (sortArray, numPolys) ;

  /*
   *  render polyhedra in sort order with alpha composition
   */
  alpha_transparency (FILDES, 1, 1, 0.15, 1.0) ;
  for (i=0 ; i<numPolys ; i++)
    {
      dP = sortArray[i] ;
      if (xf->invCntns && !DXIsElementValid (xf->invCntns, dP/PolyhedraSize))
	/* skip invalid polyhedra */
	continue ;
#else
  /*
   *render polyhedra in order given
   */
  for (i=0, dP=0 ; i<nshapes ; i+=mod, dP+=pstride)
    {	
#endif
      if (xf->invCntns && !DXIsElementValid (xf->invCntns, i))
	/* skip invalid polyhedra */
	continue ;

      if (fcolors && xf->colorsDep == dep_connections)
	SET_COLOR (fill_color, dP/PolyhedraSize) ;

#if (PolyhedraSize == 4)
      /* construct tetrahedral indices list */
      ilist[0].numFacets = 1 ;
      ilist[0].numVertices = 3 ;
      ilist[0].index[0] = connections[dP+0] ;
      ilist[0].index[1] = connections[dP+1] ;
      ilist[0].index[2] = connections[dP+2] ;

      ilist[1].numFacets = 1 ;
      ilist[1].numVertices = 3 ;
      ilist[1].index[0] = connections[dP+0] ;
      ilist[1].index[1] = connections[dP+3] ;
      ilist[1].index[2] = connections[dP+1] ;
      
      ilist[2].numFacets = 1 ;
      ilist[2].numVertices = 3 ;
      ilist[2].index[0] = connections[dP+1] ;
      ilist[2].index[1] = connections[dP+3] ;
      ilist[2].index[2] = connections[dP+2] ;
      
      ilist[3].numFacets = 1 ;
      ilist[3].numVertices = 3 ;
      ilist[3].index[0] = connections[dP+2] ;
      ilist[3].index[1] = connections[dP+3] ;
      ilist[3].index[2] = connections[dP+0] ;
#endif

#if (PolyhedraSize == 8)
      /* construct cubic indices list */
      ilist[0].numFacets = 1 ;
      ilist[0].numVertices = 4 ;
      ilist[0].index[0] = connections[dP+0] ;
      ilist[0].index[1] = connections[dP+1] ;
      ilist[0].index[2] = connections[dP+3] ;
      ilist[0].index[3] = connections[dP+2] ;

      ilist[1].numFacets = 1 ;
      ilist[1].numVertices = 4 ;
      ilist[1].index[0] = connections[dP+0] ;
      ilist[1].index[1] = connections[dP+2] ;
      ilist[1].index[2] = connections[dP+6] ;
      ilist[1].index[3] = connections[dP+4] ;

      ilist[2].numFacets = 1 ;
      ilist[2].numVertices = 4 ;
      ilist[2].index[0] = connections[dP+2] ;
      ilist[2].index[1] = connections[dP+3] ;
      ilist[2].index[2] = connections[dP+7] ;
      ilist[2].index[3] = connections[dP+6] ;

      ilist[3].numFacets = 1 ;
      ilist[3].numVertices = 4 ;
      ilist[3].index[0] = connections[dP+4] ;
      ilist[3].index[1] = connections[dP+6] ;
      ilist[3].index[2] = connections[dP+7] ;
      ilist[3].index[3] = connections[dP+5] ;

      ilist[4].numFacets = 1 ;
      ilist[4].numVertices = 4 ;
      ilist[4].index[0] = connections[dP+0] ;
      ilist[4].index[1] = connections[dP+4] ;
      ilist[4].index[2] = connections[dP+5] ;
      ilist[4].index[3] = connections[dP+1] ;

      ilist[5].numFacets = 1 ;
      ilist[5].numVertices = 4 ;
      ilist[5].index[0] = connections[dP+1] ;
      ilist[5].index[1] = connections[dP+5] ;
      ilist[5].index[2] = connections[dP+7] ;
      ilist[5].index[3] = connections[dP+3] ;
#endif

#ifdef USE_POLYHEDRA
      /* send polyhedron to Starbase */
      polyhedron_with_data (FILDES, clist, npoints,
			    vsize-3, vertex_flags, (int *)ilist,
			    NULL, FacetsPerPolyhedron, NULL) ;
#else
      /* use polygon_with_data3d() */
      for (j=0 ; j<FacetsPerPolyhedron ; j++)
	{
	  register int vi ;
	  float cbuff[(3/*coords*/+3/*color*/+1/*opacity*/)*VerticesPerFacet] ;
	  
	  clist = cbuff ;
	  for (vi=0 ; vi<VerticesPerFacet ; vi++)
	    {
	      register int v = ilist[j].index[vi] ;
	      
	      /* copy vertex coordinates */
	      *(Point *)(clist) = points[v] ;
	      clist+=3 ;
	      
	      if (vertex_flags & VERTEX_COLOR)
		{
		  /* copy vertex color into cbuff */
		  if (color_map)
		    *(RGBColor *)(clist) = color_map[((char *)fcolors)[v]] ;
		  else
		    *(RGBColor *)(clist) = fcolors[v] ;
		  clist+=3 ;
		}
	    }
	  polygon_with_data3d (FILDES, cbuff, VerticesPerFacet, vsize-3,
			       vertex_flags, NULL) ;
	}
#endif
    }

#ifdef COMPOSITE
  tdmFree(sortArray) ;
  sortArray = 0 ;
#endif

#ifdef USE_POLYHEDRA
  if (vertex_flags && !cache)
    /* cache coordinate list for re-use */
    tdmPutPolyhedraCacheSB (cache_id, 0, xf, clist, npoints, vsize-3,
			    vertex_flags, NULL, NULL, NULL, NULL) ;
#endif

  clist = 0 ;
  hidden_surface (FILDES, FALSE, FALSE) ;
  EXIT(("OK"));
  return OK ;
  
 error:
#ifdef COMPOSITE
  if (sortArray) tdmFree(sortArray) ;
#endif
  if (clist && vertex_flags) tdmFree(clist) ;
  hidden_surface(FILDES, FALSE, FALSE) ;
  EXIT(("ERROR"));
  return ERROR ;
}

#endif /* hwPolyhedrawDraw_c_h */
