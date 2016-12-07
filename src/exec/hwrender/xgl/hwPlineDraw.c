/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwPlineDraw.c,v $

 Most of strip construction is treated in the same way as with HP's
 Starbase API, especially for the vertex information.

 $Log: hwPlineDraw.c,v $
 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:42  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:36  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:02:13  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:33:29  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:15:10  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.0  1994/05/19  15:24:57  jrush
 * Initial version.
 *
 * Revision 7.1  1994/05/19  15:22:35  jrush
 * Checking in
 *
 * Revision 7.1  1994/04/21  15:34:29  tjm
 * Original version copied from hwPlineDraw.c
 *
 * Revision 1.1  94/04/21  15:33:57  tjm
 * Initial revision
 * 
\*---------------------------------------------------------------------------*/

#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwXfield.h"
#include "hwPortLayer.h"
#include "hwMemory.h"
#include "hwTmesh.h"
#include "hwCacheUtilXGL.h"

#ifdef MJHDEBUG
#define DebugMessage()                                                        \
{									      \
  int i ;								      \
  Xgl_pt Min, Max ;							      \
  Xgl_pt_f3d min, max ;							      \
                                                                              \
  fprintf (stderr, "\n2D: %s", is_2d ? "yes" : "no") ;			      \
  fprintf (stderr, "\nnumber of points: %d", xf->npositions) ;   	      \
  fprintf (stderr, "\nnumber of connections: %d", xf->origNConnections) ;     \
  fprintf (stderr, "\ncolors: 0x%x, color_map: 0x%x, dep on pos: %d",	      \
	   (int)fcolors, (int)color_map, xf->fcolorsDep == dep_positions) ;   \
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
 									      \
  min.x = min.y = min.z = +MAXFLOAT ;					      \
  Min.pt_type = XGL_PT_F3D ;						      \
  Min.pt.f3d = &min ;							      \
 									      \
  max.x = max.y = max.z = -MAXFLOAT ;					      \
  Max.pt_type = XGL_PT_F3D ;						      \
  Max.pt.f3d = &max ;							      \
 									      \
  if (is_2d)								      \
    {									      \
      min.z = max.z = 0 ;						      \
      for (i=0 ; i<xf->npositions ; i++)				      \
	{						 		      \
	  if (pnts2d[i].x < min.x) min.x = pnts2d[i].x ;   		      \
	  if (pnts2d[i].y < min.y) min.y = pnts2d[i].y ;	    	      \
 									      \
	  if (pnts2d[i].x > max.x) max.x = pnts2d[i].x ;	   	      \
	  if (pnts2d[i].y > max.y) max.y = pnts2d[i].y ;	   	      \
	}								      \
    }									      \
  else									      \
      for (i=0 ; i<xf->npositions ; i++)				      \
	{						 		      \
	  if (points[i].x < min.x) min.x = points[i].x ;   		      \
	  if (points[i].y < min.y) min.y = points[i].y ;	    	      \
	  if (points[i].z < min.z) min.z = points[i].z ;  		      \
 									      \
	  if (points[i].x > max.x) max.x = points[i].x ;	   	      \
	  if (points[i].y > max.y) max.y = points[i].y ;	   	      \
	  if (points[i].z > max.z) max.z = points[i].z ;		      \
	}								      \
									      \
  xgl_transform_multiply (TMP_TRANSFORM,				      \
                          MODEL_VIEW_TRANSFORM, PROJECTION_TRANSFORM) ;	      \
 									      \
  fprintf (stderr, "\nmin MC->VDC %9f %9f %9f -> ", min.x, min.y, min.z) ;    \
  xgl_transform_point (TMP_TRANSFORM, &Min) ;				      \
  fprintf (stderr, "%9f %9f %9f", min.x, min.y, min.z) ;		      \
 									      \
  fprintf (stderr, "\nmax MC->VDC %9f %9f %9f -> ", max.x, max.y, max.z) ;    \
  xgl_transform_point (TMP_TRANSFORM, &Max) ;				      \
  fprintf (stderr, "%9f %9f %9f", max.x, max.y, max.z) ;		      \
}
#else /* MJHDEBUG */
#define DebugMessage() {}
#endif /* MJHDEBUG */



int
_dxfPlineDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  Point *points = 0 ;
  struct p2d {float x, y ;} *pnts2d = 0 ;
  Vector *normals ;
  RGBColor *fcolors, *color_map ;
  Type type ;
  int rank, shape, is_2d ;
  float *opacities, *opacity_map ;
  char *cache_id ;
  tdmStripDataXGL *stripsXGL = 0 ;

  DEFPORT(portHandle) ;
  DPRINT("\n(_dxfPlineDraw") ;

  if(xf->colorsDep == dep_connections)
    return _dxfLineDraw(portHandle, xf, buttonUp);

  /*
   *  Extract required data from the xfield.
   */

  if (is_2d = IS_2D(xf->positions_array, type, rank, shape))
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

  DebugMessage() ;

  /* set default attributes for surface and wireframe approximation */
  xgl_object_set (XGLCTX,
		  XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_VERTEX,
		  XGL_3D_CTX_LINE_COLOR_INTERP, TRUE,
		  0) ;

  /* override above attributes according to color dependencies and lighting */
  if (xf->colorsDep == dep_field)
    {
      /*
       *  Field has constant color.  Get color from context.
       */
      CLAMP(&fcolors[0],&fcolors[0]) ;
      cache_id = "CpfPline" ;
      /* get line color from context */
      xgl_object_set
	(XGLCTX,
	 XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_CONTEXT,
	 XGL_CTX_LINE_COLOR, &fcolors[0],
	 NULL) ;
    }
  else
    {
      /*
       *  Field has varying colors.  Get colors from point data.
       */
      cache_id = "CppPline" ;
    }

  DPRINT1("\n%s", cache_id) ;

#if 0 /* may want this when we add transparent lines */
  /*
   *  Set up a simple 50% screen door approximation for opacities < 0.75.
   *  We can't do opacity dep position or connection with this technique,
   *  but constant opacity per field is adequate for many visualizations.
   *
   *  USE_SCREEN_DOOR should only be true if we're using the GT through
   *  the XGL 3.0 interface on Solaris; no other access to this effect is
   *  provided by Sun.  The GT with XGL 3.0 also provides alpha
   *  transparency of some sort.  It's not implemented here.
   */

  xgl_object_set
      (XGLCTX,
       XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
       NULL) ;

  if (opacities && USE_SCREEN_DOOR)
      if ((opacity_map? opacity_map[*(char *)opacities]: opacities[0]) < 0.75)
	  xgl_object_set
	      (XGLCTX,
	       XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_STIPPLE,
	       XGL_CTX_SURF_FRONT_FPAT, SCREEN_DOOR_50,
	       NULL) ;
#endif

  if (stripsXGL = _dxf_GetTmeshCacheXGL (cache_id, xf))
    {
      /*
       *  Use pre-constructed xgl strips obtained from executive cache.
       */
      register Xgl_pt_list *pt_list, *end ;
      DPRINT("\ngot strips from cache");

      pt_list = stripsXGL->pt_lists ;
      end = &pt_list[stripsXGL->num_strips] ;

      for ( ; pt_list < end ; pt_list++)
	xgl_multipolyline (XGLCTX, NULL, 1, pt_list) ;

    }
  else
    {
      /*
       *  Construct an array of xgl strips and cache it.
       */
      register int vsize, fsize ;
      int v_cOffs, v_nOffs, vertex_flags, numStrips ;
      int *connections, (*strips)[2] ;
      Xgl_pt_list *xgl_pt_list ;
      DPRINT("\nbuilding new strips");

      /* determine vertex types and sizes */
      vsize = 3 ;
      fsize = 0 ;
      vertex_flags = XGL_D_3 | XGL_FLT | XGL_NFLG | XGL_NHOM | XGL_DIRECT ;

      if (fcolors && xf->colorsDep != dep_field) {
	/* vertex has 3 more floats to accomodate color */
	v_cOffs = vsize ; vsize += 3 ; 
	vertex_flags |= XGL__CLR ;
      }	

      /* get strip topology */
      connections = (int *)DXGetArrayData(xf->connections_array) ;
      strips = (int (*)[2])DXGetArrayData(xf->meshes) ;
      numStrips = xf->nmeshes ;

      /* allocate space for strip data */
      stripsXGL = (tdmStripDataXGL *) tdmAllocate(sizeof(tdmStripDataXGL));
      if (!stripsXGL)
	  DXErrorGoto (ERROR_INTERNAL, "#13000") ;

      stripsXGL->pt_lists = 0 ;
      stripsXGL->facet_lists = 0 ;
      stripsXGL->num_strips = 0 ;

      /* allocate array of xgl point lists */
      xgl_pt_list =
	  stripsXGL->pt_lists = (Xgl_pt_list *)
	      tdmAllocate(numStrips*sizeof(Xgl_pt_list));

      if (!xgl_pt_list)
	  DXErrorGoto (ERROR_INTERNAL, "#13000") ;

      for ( ; stripsXGL->num_strips < numStrips ; stripsXGL->num_strips++)
	{
	  /* each iteration constructs and draws one xgl strip */
	  register float *clist, *flist ;
	  register int i, dV, *pntIdx, numPnts ;

	  /* get the number of points in this strip */
	  numPnts = strips[stripsXGL->num_strips][1] ;

	  /* allocate coordinate list */
	  xgl_pt_list->pts.color_normal_f3d =
	      (Xgl_pt_color_normal_f3d *) (clist =
		  (float *) tdmAllocate(numPnts*vsize*sizeof(float))) ;

	  if (!clist)
	      DXErrorGoto (ERROR_INTERNAL, "#13000") ;

	  /* get the sub-array of connections making up this strip */
	  pntIdx = &connections[strips[stripsXGL->num_strips][0]] ;

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
	  if (vertex_flags & XGL__CLR)
	      if (color_map)
		  for (i=0, dV=v_cOffs ; i<numPnts ; i++, dV+=vsize)
		      CLAMP((clist+dV),
			  &color_map[((char *)fcolors)[pntIdx[i]]]);
	      else
		  for (i=0, dV=v_cOffs ; i<numPnts ; i++, dV+=vsize)
		      CLAMP((clist+dV),&fcolors[pntIdx[i]] ) ;

	  /* set up other point list info */
	  xgl_pt_list->bbox = NULL ;
	  xgl_pt_list->num_pts = numPnts ;
	  xgl_pt_list->pt_type = vertex_flags ;

	  /* send strip to xgl, increment pointlist pointers */
	  xgl_multipolyline (XGLCTX, NULL, 1, xgl_pt_list++) ;
	}

      /* cache all strip data */
      _dxf_PutTmeshCacheXGL (cache_id, xf, stripsXGL) ;
    }

  DPRINT(")") ;
  return OK ;

error:
  _dxf_FreeTmeshCacheXGL((Pointer)stripsXGL) ;
  DPRINT("\nerror)") ;
  return ERROR ;
}
