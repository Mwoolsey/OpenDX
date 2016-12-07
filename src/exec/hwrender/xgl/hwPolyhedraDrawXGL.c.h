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


/*
 * $Source: /src/master/dx/src/exec/hwrender/xgl/hwPolyhedraDrawXGL.c.h,v $
 * $Log: hwPolyhedraDrawXGL.c.h,v $
 * Revision 1.3  1999/05/10 15:45:40  gda
 * Copyright message
 *
 * Revision 1.3  1999/05/10 15:45:40  gda
 * Copyright message
 *
 * Revision 1.2  1999/05/03 14:06:42  gda
 * moved to using dxconfig.h rather than command-line defines
 *
 * Revision 1.1.1.1  1999/03/24 15:18:36  gda
 * Initial CVS Version
 *
 * Revision 1.1.1.1  1999/03/19 20:59:49  gda
 * Initial CVS
 *
 * Revision 10.1  1999/02/23 21:02:32  gda
 * OpenDX Baseline
 *
 * Revision 9.1  1997/05/22 22:32:38  svs
 * Copy of release 3.1.4 code
 *
 * Revision 8.0  1995/10/03  22:14:36  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.6  1994/04/25  16:16:34  ellen
 * made change to xgl_multimarker for solaris and xgl3.0
 *
 * Revision 7.5  94/03/31  15:55:18  tjm
 * added versioning
 * 
 * Revision 7.4  94/02/24  18:31:47  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.3  94/02/14  19:23:03  mjh
 * sun4/xgl maintenance: part 2, fix cacheing bug introduced by part 1.
 * Don't cache geometric primitives before drawing them, since the cache
 * can fail and deallocate the data before it gets rendered.
 * 
 * Revision 7.2  94/02/13  18:52:33  mjh
 * sun4/xgl maintenance: remove RenderPass, use Xfield, integrate 2D code,
 * fix dot approximations for connection-dependent data, use mesh topology
 * info in Xfield directly
 * 
 * Revision 7.1  94/01/18  18:59:48  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:25:53  svs
 * ship level code, release 2.0
 * 
 * Revision 1.3  93/09/28  09:40:14  tjm
 * fixed <DMCZAP71> colors are now clamped so that the maximum value for
 * r g or b is 1.0. 
 * 
 * Revision 1.2  93/07/28  19:34:09  mjh
 * apply 2D position hacks, install invalid connection/postion support
 * 
 * Revision 1.1  93/06/29  10:01:35  tjm
 * Initial revision
 *
 * Revision 1.1  93/05/13  22:52:24  mjh
 * Initial revision
 *
 */

#include "hwXfield.h"

#define DEF_ARGS \
  tdmPortHandleP portHandle, xfieldT *xf, \
  int npoints, Point *points, \
  int nshapes, int *connections, int mod, \
  RGBColor *fcolors, RGBColor *color_map, \
  enum approxE approx

#define ARGS \
  portHandle, xf, \
  xf->npositions, points, \
  xf->nconnections, connections, mod, \
  fcolors, color_map, \
  approx

#define OutOfMemory() {DXSetError (ERROR_INTERNAL, "#13000") ; return 0 ;}

#define LoadPoint(dp,op) \
  *(Point *)&pt_f3d[dp].x = points[connections[op]] ;

#define LoadFacetPoints(a,b,c,d)               \
{                                              \
  pt_list[ds].pt_type = XGL_PT_F3D;            \
  pt_list[ds].bbox = NULL;                     \
  pt_list[ds].num_pts = VerticesPerFacet;      \
  pt_list[ds].pts.f3d = &pt_f3d[dp];           \
  switch (VerticesPerFacet)		       \
    {					       \
    case 4:				       \
      LoadPoint(dp,op+a);  dp++;	       \
    case 3:				       \
      LoadPoint(dp,op+b);  dp++;	       \
      LoadPoint(dp,op+c);  dp++;	       \
      LoadPoint(dp,op+d);  dp++;	       \
    }					       \
}

#define LoadFacetPointsAndCdirColors(a,b,c,d)                 \
{                                                             \
  pt_list[ds].pt_type = XGL_PT_COLOR_F3D;                     \
  pt_list[ds].bbox = NULL;                                    \
  pt_list[ds].num_pts = VerticesPerFacet;                     \
  pt_list[ds].pts.color_f3d = &pt_f3d[dp];                    \
  switch (VerticesPerFacet)				      \
    {							      \
    case 4:						      \
      LoadPoint(dp,op+a);  CdirLoadColor(dp,op+a);  dp++;     \
    case 3:						      \
      LoadPoint(dp,op+b);  CdirLoadColor(dp,op+b);  dp++;     \
      LoadPoint(dp,op+c);  CdirLoadColor(dp,op+c);  dp++;     \
      LoadPoint(dp,op+d);  CdirLoadColor(dp,op+d);  dp++;     \
    }							      \
}

#define LoadFacetPointsAndCmapColors(a,b,c,d)                 \
{                                                             \
  pt_list[ds].pt_type = XGL_PT_COLOR_F3D;                     \
  pt_list[ds].bbox = NULL;                                    \
  pt_list[ds].num_pts = VerticesPerFacet;                     \
  pt_list[ds].pts.color_f3d = &pt_f3d[dp];                    \
  switch (VerticesPerFacet)				      \
    {							      \
    case 4:						      \
      LoadPoint(dp,op+a);  CmapLoadColor(dp,op+a);  dp++;     \
    case 3:						      \
      LoadPoint(dp,op+b);  CmapLoadColor(dp,op+b);  dp++;     \
      LoadPoint(dp,op+c);  CmapLoadColor(dp,op+c);  dp++;     \
      LoadPoint(dp,op+d);  CmapLoadColor(dp,op+d);  dp++;     \
    }							      \
}

#define GetNdrawn()					      \
{							      \
  if (xf->invCntns)					      \
    {							      \
      for (ndrawn=0, i=0 ; i<nshapes ; i+=mod)		      \
          if (DXIsElementValid (xf->invCntns, i))	      \
              ndrawn += FacetsPerPolyhedron ;		      \
    }							      \
  else							      \
      ndrawn = FacetsPerPolyhedron * (1+(nshapes-1)/mod) ;    \
}


static int
CdfDraw (DEF_ARGS)
{
  char *mem ;
  int ndrawn, from_cache ;
  Xgl_pt_list *pt_list ;
  Xgl_color cdf_color ;
  char *color_index = (char *)fcolors ;

  DEFPORT(portHandle);
  DPRINT("\n(CdfPolyhedraDraw") ;

  if (mem = _dxf_GetPolyCacheXGL (CpfPolyhedra, mod, xf))
    {
      DPRINT("\ngot polyhedra from cache") ;
      from_cache = 1 ;

      ndrawn = *(int *) mem ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
    }
  else
    {
      register int ds, dp, op, i ;
      register Xgl_pt_f3d *pt_f3d ;

      DPRINT("\ngenerating polyhedra") ;
      from_cache = 0 ;

      GetNdrawn() ;
      mem = (char *)
	  tdmAllocate (sizeof(int) +
		       sizeof(Xgl_pt_list)*ndrawn +
		       sizeof(Xgl_pt_f3d)*ndrawn*VerticesPerFacet) ;

      if (!mem) OutOfMemory() ;

      *(int *)mem = ndrawn ;
      pt_list = (Xgl_pt_list *)(mem + sizeof(int)) ;
      pt_f3d = (Xgl_pt_f3d *)(mem + sizeof(int) + sizeof(Xgl_pt_list)*ndrawn) ;

      for (ds=0,dp=0,op=0,i=0; i<nshapes; i+=mod,op+=mod*VerticesPerPolyhedron)
	{
	  if (xf->invCntns && !DXIsElementValid (xf->invCntns, i))
	      continue ;

	  /* defined in including file */
	  LoadPolyhedronPoints() ;
	}
    }

  if (color_map)
      CLAMP(&cdf_color, &color_map[color_index[0]]) ;
  else
      CLAMP(&cdf_color, &fcolors[0]) ;

  if (approx == approx_lines)
    {
      xgl_object_set (XGLCTX,
		      XGL_CTX_LINE_COLOR, &cdf_color,
		      XGL_3D_CTX_LINE_COLOR_INTERP, FALSE,
		      XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_CONTEXT,
		      NULL) ;

      DPRINT("\ncalling xgl_multipolyline") ;
      xgl_multipolyline (XGLCTX, NULL, ndrawn, pt_list) ;
    }
  else
    {
      xgl_object_set
	  (XGLCTX,
	   XGL_CTX_SURF_FRONT_COLOR, &cdf_color,
	   XGL_CTX_SURF_FRONT_COLOR_SELECTOR, XGL_SURF_COLOR_CONTEXT,
	   NULL) ;

      DPRINT("\ncalling xgl_multi_simple_polygon") ;
      xgl_multi_simple_polygon (XGLCTX, FacetFlags, NULL, NULL,
				ndrawn, pt_list) ;
    }

  if (!from_cache)
      _dxf_PutPolyCacheXGL (CpfPolyhedra, mod, xf, mem) ;

  DPRINT(")") ;
}


static int
CdcDraw (DEF_ARGS)
{
#ifdef CdirLoadColor
#undef CdirLoadColor
#endif
#define CdirLoadColor(dp,op) \
  CLAMP(&pt_f3d[dp].color, &fcolors[i]) ;

#ifdef CmapLoadColor
#undef CmapLoadColor
#endif
#define CmapLoadColor(dp,op) \
  CLAMP(&pt_f3d[dp].color, &color_map[color_index[i]]) ;

  int ndrawn, from_cache ;
  char *mem ;
  Xgl_pt_list *pt_list ;

  DEFPORT(portHandle) ;
  DPRINT ("\n(CdcPolyhedraDraw") ;

  if (mem = _dxf_GetPolyCacheXGL (CpcPolyhedra, mod, xf))
    {
      DPRINT("\ngot polyhedra from cache") ;
      from_cache = 1 ;

      ndrawn = *(int *) mem ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
    }
  else
    {
      register int ds, dp, op, i ;
      register Xgl_pt_color_f3d *pt_f3d ;

      DPRINT("\ngenerating polyhedra") ;
      from_cache = 0 ;

      GetNdrawn() ;
      mem = (char *)
	  tdmAllocate (sizeof(int) +
		       sizeof(Xgl_pt_list)*ndrawn +
		       sizeof(Xgl_pt_color_f3d)*ndrawn*VerticesPerFacet) ;

      if (!mem) OutOfMemory() ;

      *(int *) mem = ndrawn ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
      pt_f3d = (Xgl_pt_color_f3d *)
	  (mem + sizeof(int) + sizeof(Xgl_pt_list)*ndrawn) ;

      if (color_map)
	{
	  char *color_index = (char *)fcolors ;
	  ds = 0 ; dp = 0 ;
	  for (op=0,i=0; i<nshapes; i+=mod,op+=mod*VerticesPerPolyhedron)
	    {
	      if (xf->invCntns && !DXIsElementValid (xf->invCntns, i))
		  continue ;

	      /* defined in including file */
	      LoadPolyhedronPointsAndCmapColors() ;
	    }
	}
      else
	{
	  ds = 0 ; dp = 0 ;
	  for (op=0,i=0; i<nshapes; i+=mod,op+=mod*VerticesPerPolyhedron)
	    {
	      if (xf->invCntns && !DXIsElementValid (xf->invCntns, i))
		  continue ;

	      /* defined in including file */
	      LoadPolyhedronPointsAndCdirColors() ;
	    }
	}
    }

  if (approx == approx_dots)
    {
      /*
       *  Dot approximations are more efficiently rendered by
       *  _dxfUnconnectedPointDraw(), but connection-dependent colors
       *  are not be handled by that routine.
       */
      Xgl_pt_list dot_pt_list ;

      dot_pt_list.pt_type = XGL_PT_COLOR_F3D ;
      dot_pt_list.pts.color_f3d = (Xgl_pt_color_f3d *)
	  (mem + sizeof(int) + sizeof(Xgl_pt_list)*ndrawn) ;
      dot_pt_list.bbox = NULL ;
      dot_pt_list.num_pts = ndrawn*VerticesPerPolyhedron ;

      xgl_object_set
	  (XGLCTX, 
#ifdef solaris
	   XGL_CTX_MARKER, xgl_marker_dot,
#else
	   XGL_CTX_MARKER_DESCRIPTION, xgl_marker_dot,
#endif
	   XGL_CTX_MARKER_COLOR_SELECTOR, XGL_MARKER_COLOR_POINT,
	   NULL) ;

      DPRINT("\ncalling xgl_multimarker") ;
      xgl_multimarker (XGLCTX, &dot_pt_list) ;
    }
  else if (approx == approx_lines)
    {
      xgl_object_set (XGLCTX,
		      XGL_3D_CTX_LINE_COLOR_INTERP, FALSE,
		      XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_VERTEX,
		      NULL) ;

      DPRINT("\ncalling xgl_multipolyline") ;
      xgl_multipolyline (XGLCTX, NULL, ndrawn, pt_list) ;
    }
  else
    {
      xgl_object_set
	  (XGLCTX,
	   XGL_3D_CTX_SURF_FRONT_ILLUMINATION,XGL_ILLUM_NONE,
	   XGL_CTX_SURF_FRONT_COLOR_SELECTOR,XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
	   NULL) ;

      DPRINT("\ncalling xgl_multi_simple_polygon") ;
      xgl_multi_simple_polygon (XGLCTX, FacetFlags, NULL, NULL,
				ndrawn, pt_list);
    }

  if (!from_cache)
      _dxf_PutPolyCacheXGL (CpcPolyhedra, mod, xf, mem) ;

  DPRINT(")") ;
}


static int
CdpDraw (DEF_ARGS)
{
#ifdef CdirLoadColor
#undef CdirLoadColor
#endif
#define CdirLoadColor(dp,op) \
  CLAMP(&pt_f3d[dp].color, &fcolors[connections[op]]) ;

#ifdef CmapLoadColor
#undef CmapLoadColor
#endif
#define CmapLoadColor(dp,op) \
  CLAMP(&pt_f3d[dp].color, &color_map[color_index[connections[op]]]) ;

  int ndrawn, from_cache ;
  char *mem ;
  Xgl_pt_list *pt_list ;

  DEFPORT(portHandle) ;
  DPRINT ("\n(CdpPolyhedraDraw") ;

  if (mem = _dxf_GetPolyCacheXGL (CpvPolyhedra, mod, xf))
    {
      DPRINT("\ngot polyhedra from cache") ;
      from_cache = 1 ;

      ndrawn = *(int *) mem ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
    }
  else
    {
      register int ds, dp, op, i ;
      register Xgl_pt_color_f3d *pt_f3d ;

      DPRINT("\ngenerating polyhedra") ;
      from_cache = 0 ;

      GetNdrawn() ;
      mem = (char *)
 	  tdmAllocate (sizeof(int) +
		       sizeof(Xgl_pt_list)*ndrawn +
		       sizeof(Xgl_pt_color_f3d)*ndrawn*VerticesPerFacet) ;

      if (!mem) OutOfMemory() ;

      *(int *) mem = ndrawn ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
      pt_f3d = (Xgl_pt_color_f3d *)
	  (mem + sizeof(int) + sizeof(Xgl_pt_list)*ndrawn) ;

      if (color_map)
	{
	  char *color_index = (char *)fcolors ;
	  ds = 0 ; dp = 0 ;
	  for (op=0,i=0; i<nshapes; i+=mod,op+=mod*VerticesPerPolyhedron)
	    {
	      if (xf->invCntns && !DXIsElementValid (xf->invCntns, i))
		  continue ;

	      /* defined in including file */
	      LoadPolyhedronPointsAndCmapColors() ;
	    }
	}
      else
	{
	  ds = 0 ; dp = 0 ;
	  for (op=0,i=0; i<nshapes; i+=mod,op+=mod*VerticesPerPolyhedron)
	    {
	      if (xf->invCntns && !DXIsElementValid (xf->invCntns, i))
		  continue ;

	      /* defined in including file */
	      LoadPolyhedronPointsAndCdirColors() ;
	    }
	}
    }

  if (approx == approx_lines)
    {
      xgl_object_set (XGLCTX,
		      XGL_3D_CTX_LINE_COLOR_INTERP, TRUE,
		      XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_VERTEX,
		      NULL) ;

      DPRINT("\ncalling xgl_multipolyline") ;
      xgl_multipolyline (XGLCTX, NULL, ndrawn, pt_list) ;
    }
  else
    {
      xgl_object_set
	  (XGLCTX,
	   XGL_3D_CTX_SURF_FRONT_ILLUMINATION,XGL_ILLUM_NONE_INTERP_COLOR,
	   XGL_CTX_SURF_FRONT_COLOR_SELECTOR,XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
	   NULL) ;

      DPRINT("\ncalling xgl_multi_simple_polygon") ;
      xgl_multi_simple_polygon (XGLCTX, FacetFlags, NULL, NULL,
				ndrawn, pt_list);
    } 

  if (!from_cache)
      _dxf_PutPolyCacheXGL (CpvPolyhedra, mod, xf, mem) ;

  DPRINT(")") ;
}


int
_dxfPolyhedraDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  Point *points ;
  RGBColor *fcolors, *color_map ;
  int *connections, mod, ret ;
  enum approxE approx ;
  DEFPORT(portHandle) ;

  DPRINT("\n(_dxfPolyhedraDraw") ;
  DPRINT1("\n%d invalid connections",
	  xf->invCntns? DXGetInvalidCount(xf->invCntns): 0) ;

  /*
   *  Extract required data from the xfield.
   */

  points = (Point *) DXGetArrayData (xf->positions_array) ;
  connections = (int *) DXGetArrayData (xf->connections_array) ;
  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;

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

  xgl_object_set
      (XGLCTX,
       XGL_3D_CTX_SURF_FACE_DISTINGUISH, FALSE,
       XGL_3D_CTX_SURF_FRONT_ILLUMINATION, XGL_ILLUM_NONE,
       XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,
       XGL_3D_CTX_SURF_FRONT_AMBIENT,  xf->attributes.front.ambient,
       XGL_3D_CTX_SURF_FRONT_DIFFUSE,  xf->attributes.front.diffuse,
       XGL_3D_CTX_SURF_FRONT_SPECULAR, xf->attributes.front.specular,
       NULL) ;

  if (xf->colorsDep == dep_field)
      ret = CdfDraw (ARGS) ;
  else if (xf->colorsDep == dep_connections)
      ret = CdcDraw (ARGS) ;
  else
      ret = CdpDraw (ARGS) ;

  DPRINT(")") ;
  return ret ;
}
