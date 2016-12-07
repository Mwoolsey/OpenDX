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


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwMeshDraw.c.h,v $

 Most of triangle strip construction is treated in the same way as with HP's
 Starbase API, especially for the vertex information.  Facet colors are
 supported in addition to facet normals in XGL, unlike Starbase; however,
 XGL only provides an enumeration of facet types instead of allowing the
 more convenient bit-wise OR of facet flags.

 This code was written to handle connection-dependent colors and normals,
 although fields with such dependencies are not converted to triangle strips
 in _dxf_newXfield() (in hwXfield.c).  This is because XGL is the only API
 that allows both types of facet data.  We may wish to remove the code
 someday. 

 $Log: hwMeshDraw.c.h,v $
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

 Revision 10.1  1999/02/23 21:02:07  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:32:30  svs
 Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:14:26  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.5  1995/09/11  19:21:06  c1orrang
 * (unsigned char *) fix for colormaps
 *
 * Revision 7.4  1994/03/31  15:55:14  tjm
 * added versioning
 *
 * Revision 7.3  94/02/24  18:31:39  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.2  94/02/13  18:52:22  mjh
 * sun4/xgl maintenance: remove RenderPass, use Xfield, integrate 2D code,
 * fix dot approximations for connection-dependent data, use mesh topology
 * info in Xfield directly
 * 
 * Revision 7.1  94/01/18  18:59:46  svs
 * changes since release 2.0.1
 *
 * Revision 6.1  93/11/16  10:25:51  svs
 * ship level code, release 2.0
 *
 * Revision 1.7  93/10/22  14:57:17  tjm
 * Fixed back color for surfaces dep connection. fixes <DMCZAP90>
 *
 * Revision 1.6  93/09/29  14:00:52  tjm
 * Fixed <PITTS21> wrong background color on sun. Two problems, negative colors
 * in a field (now clamp to 0.0) and no constant colors set on mesh primitives
 * if no normals. (now set colors)
 *
 * Revision 1.5  93/09/28  09:40:12  tjm
 * fixed <DMCZAP71> colors are now clamped so that the maximum value for
 * r g or b is 1.0.
 *
 * Revision 1.4  93/07/28  00:52:17  mjh
 * remove redundant dot approximation code
 *
 * Revision 1.3  93/07/24  19:07:43  tjm
 * fixed bug with 2d data
 *
 * Revision 1.2  93/07/21  21:37:16  mjh
 * update for xfield data structure
 *
 * Revision 1.1  93/06/29  10:01:31  tjm
 * Initial revision
 *
 * Revision 5.5  93/05/20  20:08:36  mjh
 * make sure cleanup deallocates clist if flist fails
 *
 * Revision 5.4  93/05/11  18:04:27  mjh
 * Implement screen door transparency for the GT.
 *
 * Revision 5.3  93/05/07  19:56:10  mjh
 * rewrite, add facet data handling, add flat shading
 *
 * Revision 5.2  93/05/03  18:25:13  mjh
 * update names -> dxf
 *
 * Revision 5.1  93/04/30  23:46:05  mjh
 * code common to both triangle strip and quad strip rendering
 *
\*---------------------------------------------------------------------------*/

/* xgl provides vertex flags, but not facet flags for some reason */
#define TDM_FACET_COLOR  0x1
#define TDM_FACET_NORMAL 0x2

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
tdmMeshDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  Point *points = 0 ;
  struct p2d {float x, y ;} *pnts2d = 0 ;
  Vector *normals ;
  RGBColor *fcolors, *color_map ;
  Type type ;
  int rank, shape, is_2d ;
  float *opacities, *opacity_map ;
  char *cache_id ;
  enum approxE approx ;
  tdmStripDataXGL *stripsXGL = 0 ;

  DEFPORT(portHandle) ;
  DPRINT("\n(hwMeshDraw") ;

  /*
   *  Extract required data from the xfield.
   */

  if (is_2d = IS_2D(xf->positions_array, type, rank, shape))
      pnts2d = (struct p2d *) DXGetArrayData(xf->positions_array) ;
  else
      points = (Point *) DXGetArrayData(xf->positions_array) ;

  normals = (Vector *) DXGetArrayData(xf->normals_array) ;
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

  approx = buttonUp ? xf->attributes.buttonUp.approx :
                      xf->attributes.buttonDown.approx ;

  DebugMessage() ;

  /* set default attributes for surface and wireframe approximation */
  xgl_object_set
      (XGLCTX,
       XGL_3D_CTX_SURF_FACE_DISTINGUISH, FALSE,
       XGL_3D_CTX_SURF_FRONT_AMBIENT,  xf->attributes.front.ambient,
       XGL_3D_CTX_SURF_FRONT_DIFFUSE,  xf->attributes.front.diffuse,
       XGL_3D_CTX_SURF_FRONT_SPECULAR, xf->attributes.front.specular,
       XGL_3D_CTX_SURF_FRONT_ILLUMINATION, XGL_ILLUM_NONE_INTERP_COLOR,
       XGL_CTX_SURF_FRONT_COLOR_SELECTOR, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
       XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_VERTEX,
       XGL_3D_CTX_LINE_COLOR_INTERP, TRUE,
       NULL) ;

  /* override above attributes according to color dependencies and lighting */
  if (xf->colorsDep == dep_field)
    {
      /*
       *  Field has constant color.  Get color from context, get normals
       *  (if they exist) from point or facet data.
       */
      CLAMP(&fcolors[0],&fcolors[0]) ;
      cache_id = CpfMsh ;
      if (approx == approx_lines)
	  /* get line color from context */
	  xgl_object_set
	      (XGLCTX,
	       XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_CONTEXT,
	       XGL_CTX_LINE_COLOR, &fcolors[0],
	       NULL) ;
      else if (normals)
	  /* get surface color from context, turn on lighting */
	  xgl_object_set
	      (XGLCTX,
	       XGL_3D_CTX_SURF_FRONT_ILLUMINATION, XGL_ILLUM_PER_VERTEX,
	       XGL_CTX_SURF_FRONT_COLOR_SELECTOR, XGL_SURF_COLOR_CONTEXT,
	       XGL_CTX_SURF_FRONT_COLOR, &fcolors[0],
	       NULL) ;
      else
	  /* get surface color from context, no lighting */
	  xgl_object_set
	      (XGLCTX,
	       XGL_CTX_SURF_FRONT_COLOR_SELECTOR, XGL_SURF_COLOR_CONTEXT,
	       XGL_CTX_SURF_FRONT_COLOR, &fcolors[0],
	       NULL) ;
    }
  else
    {
      /*
       *  Field has varying colors.  Get colors from point/facet data,
       *  decide whether to supply normals (if they exist) for flat shading.
       */
      if (approx == approx_flat)
	  /* force flat shade by withholding normals */
	  cache_id = CpcMsh ;
      else
	  /* provide normals in facet/point data */
	  cache_id = CpvMsh ;

      if (normals)
	  /*
	   *  Turn on lighting.  Don't use XGL_ILLUM_PER_FACET even if
	   *  approx == approx_flat, as that will prevent colors from
	   *  being interpolated.  If normals are withheld, xgl will
	   *  compute them per facet and apply a flat shading model.
	   */
	  xgl_object_set
	      (XGLCTX,
	       XGL_3D_CTX_SURF_FRONT_ILLUMINATION, XGL_ILLUM_PER_VERTEX,
	       NULL) ;
    }

  DPRINT1("\n%s", cache_id) ;

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

  if (stripsXGL = _dxf_GetTmeshCacheXGL (cache_id, xf))
    {
      /*
       *  Use pre-constructed xgl strips obtained from executive cache.
       */
      register Xgl_pt_list *pt_list, *end ;
      DPRINT("\ngot strips from cache");

      pt_list = stripsXGL->pt_lists ;
      end = &pt_list[stripsXGL->num_strips] ;

      if (approx == approx_lines)
	  /*
	   *  Draw wireframe approximation.  Note that this only
	   *  handles field- and position- dependent colors correctly,
	   *  NOT connection-dependent colors.
	   */
 	  for ( ; pt_list < end ; pt_list++)
	      xgl_multipolyline (XGLCTX, NULL, 1, pt_list) ;

      else if (stripsXGL->facet_lists)
	{
	  register Xgl_facet_list *facet_list = stripsXGL->facet_lists ;
	  for ( ; pt_list < end ; pt_list++, facet_list++)
	      /* draw strips with facet data */
	      xgl_triangle_strip (XGLCTX, facet_list, pt_list) ;
	}
      else
	  for ( ; pt_list < end ; pt_list++)
	      /* draw strips with vertex info only */
	      xgl_triangle_strip (XGLCTX, NULL, pt_list) ;
    }
  else
    {
      /*
       *  Construct an array of xgl triangle strips and cache it.
       */
      register int vsize, fsize ;
      int f_cOffs, f_nOffs, facet_flags ;
      int v_cOffs, v_nOffs, vertex_flags, numStrips ;
      int *connections, (*strips)[2] ;
      Xgl_pt_list *xgl_pt_list ;
      Xgl_facet_list *xgl_facet_list ;
      DPRINT("\nbuilding new strips");

      /* determine vertex and facet types and sizes */
      vsize = 3 ;
      fsize = 0 ;
      facet_flags = 0 ;
      vertex_flags = XGL_D_3 | XGL_FLT | XGL_NFLG | XGL_NHOM | XGL_DIRECT ;

      if (fcolors && xf->colorsDep != dep_field)
	  /* constant color fields take color from context */
	  if (xf->colorsDep == dep_positions)
	    {
	      /* vertex has 3 more floats to accomodate color */
	      v_cOffs = vsize ; vsize += 3 ; 
	      vertex_flags |= XGL__CLR ;
	    }
	  else
	    {
	      /* facet has at least 3 floats, with color at float 0 */
	      f_cOffs = 0 ; fsize = 3 ; 
	      facet_flags = TDM_FACET_COLOR ;
	    }

      if (normals && approx != approx_flat)
	  /* flat shading (CpcColorTool) is forced by withholding normals */
	  if (xf->normalsDep == dep_positions)
	    {
	      /* vertex has 3 more floats to accomodate normal */
	      v_nOffs = vsize ; vsize += 3 ; 
	      vertex_flags |= XGL__NOR ;
	    }
	  else
	    {
	      /* facet has 3 more floats to accomodate normal */
	      f_nOffs = fsize ; fsize += 3 ; 
	      facet_flags |= TDM_FACET_NORMAL ;
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

      /* allocate array of xgl facet lists if needed */
      if (facet_flags)
	{
	  xgl_facet_list =
	      stripsXGL->facet_lists = (Xgl_facet_list *)
		  tdmAllocate(numStrips*sizeof(Xgl_facet_list));

	  if (!xgl_facet_list)
	      DXErrorGoto (ERROR_INTERNAL, "#13000") ;
	}

      for ( ; stripsXGL->num_strips < numStrips ; stripsXGL->num_strips++)
	{
	  /* each iteration constructs and draws one xgl triangle strip */
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
			  &color_map[((unsigned char *)fcolors)[pntIdx[i]]]);
	      else
		  for (i=0, dV=v_cOffs ; i<numPnts ; i++, dV+=vsize)
		      CLAMP((clist+dV),&fcolors[pntIdx[i]] ) ;

	  /* copy vertex normals */
	  if (vertex_flags & XGL__NOR)
	      for (i=0, dV=v_nOffs ; i<numPnts ; i++, dV+=vsize)
		  *(Vector *)(clist+dV) = normals[pntIdx[i]] ;

	  /* allocate facet list */
	  if (facet_flags)
	    {
	      xgl_facet_list->facets.color_normal_facets =
		  (Xgl_color_normal_facet *) (flist =
		      (float *) tdmAllocate((numPnts-2)*fsize*sizeof(float))) ;

	      if (!flist)
		{
		  /* make sure cleanup deallocates clist */
		  stripsXGL->num_strips++ ;
		  DXErrorGoto (ERROR_INTERNAL, "#13000") ;
		}
	    }

	  /* copy facet colors */
	  if (facet_flags & TDM_FACET_COLOR)
	      if (color_map)
		  for (i=0, dV=f_cOffs ; i<numPnts-2 ; i++, dV+=fsize)
		       CLAMP((flist+dV),
			  &color_map[((char *)fcolors)[pntIdx[i]]]) ;
	      else
		  for (i=0, dV=f_cOffs ; i<numPnts-2 ; i++, dV+=fsize)
		      CLAMP((flist+dV),&fcolors[pntIdx[i]]) ;

	  /* copy facet normals */
	  if (facet_flags & TDM_FACET_NORMAL)
	      for (i=0, dV=f_nOffs ; i<numPnts-2 ; i++, dV+=fsize)
		  *(Vector *)(flist+dV) = normals[pntIdx[i]] ;

	  /* set up other point list info */
	  xgl_pt_list->bbox = NULL ;
	  xgl_pt_list->num_pts = numPnts ;
	  xgl_pt_list->pt_type = vertex_flags ;

	  if (facet_flags)
	    {
	      /* set up other facet list info */
	      xgl_facet_list->num_facets = numPnts-2 ;

	      /* deal with stupid xgl facet type enumeration */
	      switch (facet_flags)
		{
		case TDM_FACET_NORMAL:
		  xgl_facet_list->facet_type = XGL_FACET_NORMAL ;
		  break ;
		case TDM_FACET_COLOR:
		  xgl_facet_list->facet_type = XGL_FACET_COLOR ;
		  break ;
		case TDM_FACET_COLOR | TDM_FACET_NORMAL:
		  xgl_facet_list->facet_type = XGL_FACET_COLOR_NORMAL ;
		  break ;
		}
	    }

	  /* send strip to xgl, increment point and facet list pointers */
	  if (approx == approx_lines)
	      /*
	       *  Draw wireframe approximation.  Note that this only
	       *  handles field- and position- dependent colors correctly,
	       *  NOT connection-dependent colors.
	       */
	      xgl_multipolyline (XGLCTX, NULL, 1, xgl_pt_list++) ;
	  else if (facet_flags)
	      xgl_triangle_strip (XGLCTX, xgl_facet_list++, xgl_pt_list++) ;
	  else
	      xgl_triangle_strip (XGLCTX, NULL, xgl_pt_list++) ;
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
