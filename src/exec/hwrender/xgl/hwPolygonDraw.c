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
 * $Source: /src/master/dx/src/exec/hwrender/xgl/hwPolygonDraw.c,v $
 *
 * This file contains all the XGL polygon drawing routines.
 *
 * $Log: hwPolygonDraw.c,v $
 * Revision 1.3  1999/05/10 15:45:39  gda
 * Copyright message
 *
 * Revision 1.3  1999/05/10 15:45:39  gda
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
 * Revision 10.1  1999/02/23 21:02:15  gda
 * OpenDX Baseline
 *
 * Revision 9.1  1997/05/22 22:33:37  svs
 * Copy of release 3.1.4 code
 *
 * Revision 8.0  1995/10/03  22:15:17  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.6  1994/04/21  15:35:24  tjm
 * added polylines to sun and hp, fixed mem leak in xf->origConnections and
 * added performance timing marks to sun and hp
 *
 * Revision 7.5  94/03/31  15:55:16  tjm
 * added versioning
 * 
 * Revision 7.4  94/02/24  18:31:42  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.3  94/02/14  19:22:56  mjh
 * sun4/xgl maintenance: part 2, fix cacheing bug introduced by part 1.
 * Don't cache geometric primitives before drawing them, since the cache
 * can fail and deallocate the data before it gets rendered.
 * 
 * Revision 7.2  94/02/13  18:52:27  mjh
 * sun4/xgl maintenance: remove RenderPass, use Xfield, integrate 2D code,
 * fix dot approximations for connection-dependent data, use mesh topology
 * info in Xfield directly
 * 
 * Revision 7.1  94/01/18  19:00:02  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:26:08  svs
 * ship level code, release 2.0
 * 
 * Revision 1.8  93/10/28  17:07:38  tjm
 * Fixed problems with wireframe/dot approx and also with polygons w/ constant
 * normals.
 * 
 * Revision 1.7  93/10/26  14:50:49  mjh
 * check to see if original connections had been transformed into strips
 * 
 * Revision 1.6  93/10/22  14:58:04  tjm
 * Fixed back color for surfaces dep connection. fixes <DMCZAP90>
 *
 * Revision 1.5  93/07/28  19:34:12  mjh
 * apply 2D position hacks, install invalid connection/postion support
 *
 * Revision 1.4  93/07/28  03:05:14  mjh
 * fix a comment
 *
 * Revision 1.3  93/07/28  00:52:49  mjh
 * remove redundant dot approximation code
 *
 * Revision 1.2  93/07/23  22:56:00  mjh
 * add invalid connection support, use xfield data structure
 *
 * Revision 1.1  93/06/29  10:01:33  tjm
 * Initial revision
 *
 * Revision 5.7  93/05/19  23:57:38  mjh
 * _dxf_CdcNdc##_Shape_##Draw() doesn't render color dep connection /
 * normal dep connection properly.  Many facets seem to take on random
 * colors.  I believe this is a Sun bug, but it can be worked around
 * by substituting a call to _dxf_CdcFlat##_Shape_##Draw().  The hardware
 * must then compute normals itself, but it doesn't seem to impact
 * performance too greatly and does produce the desired images.
 *
 * Revision 5.6  93/05/11  18:04:36  mjh
 * Implement screen door transparency for the GT.
 *
 * Revision 5.5  93/05/07  19:53:52  mjh
 * Implement flat shading, fix usage of normals, optimize copies.
 * Use polylines for much better wireframe renderings.
 */

#include <stdio.h>
#include <math.h>
#include <xgl/xgl.h>

#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwXfield.h"
#include "hwMemory.h"
#include "hwCacheUtilXGL.h"

struct p2d {float x, y ;} ;

#ifdef PROTO_USED
#define DEF_ARGS /*DEF_ARGS*/ \
  tdmPortHandleP portHandle, xfieldT *xf, int buttonUp, int approx, \
  xfieldT *f, int npoints, Point *points, \
  int nshapes, int *connections, \
  int colorDepPos, RGBColor *colors, RGBColor *color_map, \
  int normalDepPos, Vector *normals, \
  int opacityDepPos, float *opacities, float *opacity_map, \
  float k_ambi, float k_diff, float k_spec, int is_2d
#else
#define DEF_ARGS /*DEF_ARGS*/ \
  tdmPortHandleP portHandle, xfieldT *xf, int buttonUp, int approx, \
  xfieldT *f, int npoints, Point *points, \
  int nshapes, int *connections, \
  int colorDepPos, RGBColor *colors, RGBColor *color_map, \
  int normalDepPos, Vector *normals, \
  int opacityDepPos, float *opacities, float *opacity_map, \
  double k_ambi, double k_diff, double k_spec, int is_2d
#endif

#define ARGS /*ARGS*/ \
  portHandle, xf, buttonUp, approx, \
  xf, xf->npositions, points, \
  nconnections, connections, \
  xf->colorsDep==dep_positions, colors, color_map, \
  xf->normalsDep==dep_positions, normals, \
  xf->opacitiesDep==dep_positions, opacities, opacity_map, \
  xf->attributes.front.ambient, \
  xf->attributes.front.diffuse, \
  xf->attributes.front.specular, is_2d

#define CDF    11
#define CDC    22
#define CDP    33
#define NDF    44
#define NDC    55
#define NNONE  66



#define CastColor(c) *((Xgl_color*)&(c))

#define FacetFlags(_Sides_) ((_Sides_ == 3 ? XGL_FACET_FLAG_SIDES_ARE_3 :   \
                                             XGL_FACET_FLAG_SIDES_ARE_4 ) | \
                              XGL_FACET_FLAG_SHAPE_CONVEX |                 \
                              XGL_FACET_FLAG_FN_NOT_CONSISTENT)

#define NoFacetType Xgl_color_facet /* not really used... need a default */
#define NoFacetVar color_facets     /* not really used... need a default */



#define SetPointList(ptr, PointType, _Sides_)  \
{                                              \
  xgl_pt_list = (Xgl_pt_list *) ptr ;          \
  ptr += npolys*sizeof(Xgl_pt_list) ;          \
  xgl_pt = (PointType *) ptr ;                 \
  ptr += _Sides_*npolys*sizeof(PointType) ;    \
}

#define SetFacetList(ptr, Facet_Type, var_name, FACET_TYPE)  \
{                                                            \
  xgl_facet_list.facet_type = FACET_TYPE ;                   \
  if (FACET_TYPE == XGL_FACET_NONE)                          \
      xgl_facet_list_ptr = 0 ;                               \
  else                                                       \
    {                                                        \
      xgl_facet_list_ptr = &xgl_facet_list ;                 \
      xgl_facet_list.num_facets = npolys ;                   \
      xgl_facet = (Facet_Type *) ptr ;                       \
      ptr += npolys*sizeof(Facet_Type) ;                     \
      xgl_facet_list.facets.var_name = xgl_facet ;           \
    }                                                        \
}

#define PointListSize(PointType,_Sides_) \
     (npolys*sizeof(Xgl_pt_list) + _Sides_*npolys*sizeof(PointType))

#define FacetListSize(FacetEnum,FacetType) \
     (FacetEnum == XGL_FACET_NONE ? 0 : (npolys*sizeof(FacetType)))



#define LoadPoint3D(dp,op) \
  *(Point *)&xgl_pt[dp].x = points[connections[op]] ;

#define LoadPoint2D(dp,op) \
  *(struct p2d *)&xgl_pt[dp].x = pnts2d[connections[op]] ;     \
  xgl_pt[dp].z = 0 ;

#define TriLoadPoints3D(PTTYPE)				       \
{							       \
  for (ds=0, dp=0 ; ds < npolys ; ds++)			       \
    {							       \
      op = polys[ds] * 3 ;				       \
      xgl_pt_list[ds].pt_type = PTTYPE ;		       \
      xgl_pt_list[ds].bbox = NULL ;			       \
      xgl_pt_list[ds].num_pts = 3 ;			       \
      xgl_pt_list[ds].pts.f3d = (Xgl_pt_f3d *) &xgl_pt[dp] ;   \
      LoadPoint3D(dp,op) ;    dp++ ;			       \
      LoadPoint3D(dp,op+1) ;  dp++ ;			       \
      LoadPoint3D(dp,op+2) ;  dp++ ;			       \
    }							       \
}

#define TriLoadPoints2D(PTTYPE)				       \
{							       \
  for (ds=0, dp=0 ; ds < npolys ; ds++)			       \
    {							       \
      op = polys[ds] * 3 ;				       \
      xgl_pt_list[ds].pt_type = PTTYPE ;		       \
      xgl_pt_list[ds].bbox = NULL ;			       \
      xgl_pt_list[ds].num_pts = 3 ;			       \
      xgl_pt_list[ds].pts.f3d = (Xgl_pt_f3d *) &xgl_pt[dp] ;   \
      LoadPoint2D(dp,op) ;    dp++ ;			       \
      LoadPoint2D(dp,op+1) ;  dp++ ;			       \
      LoadPoint2D(dp,op+2) ;  dp++ ;			       \
    }							       \
}

#define QuadLoadPoints3D(PTTYPE)                               \
{ 			      				       \
  for (ds=0, dp=0 ; ds < npolys ; ds++)			       \
    {							       \
      op = polys[ds] * 4 ;				       \
      xgl_pt_list[ds].pt_type = PTTYPE ;		       \
      xgl_pt_list[ds].bbox = NULL ;			       \
      xgl_pt_list[ds].num_pts = 4 ;			       \
      xgl_pt_list[ds].pts.f3d = (Xgl_pt_f3d*) &xgl_pt[dp] ;    \
      LoadPoint3D(dp,op) ;    dp++ ;			       \
      LoadPoint3D(dp,op+1) ;  dp++ ;			       \
      LoadPoint3D(dp,op+3) ;  dp++ ;			       \
      LoadPoint3D(dp,op+2) ;  dp++ ;			       \
    }							       \
}

#define QuadLoadPoints2D(PTTYPE)                               \
{ 			      				       \
  for (ds=0, dp=0 ; ds < npolys ; ds++)			       \
    {							       \
      op = polys[ds] * 4 ;				       \
      xgl_pt_list[ds].pt_type = PTTYPE ;		       \
      xgl_pt_list[ds].bbox = NULL ;			       \
      xgl_pt_list[ds].num_pts = 4 ;			       \
      xgl_pt_list[ds].pts.f3d = (Xgl_pt_f3d*) &xgl_pt[dp] ;    \
      LoadPoint2D(dp,op) ;    dp++ ;			       \
      LoadPoint2D(dp,op+1) ;  dp++ ;			       \
      LoadPoint2D(dp,op+3) ;  dp++ ;			       \
      LoadPoint2D(dp,op+2) ;  dp++ ;			       \
    }							       \
}


#define CdfLoadColors()

#define CdcLoadColors()                                                       \
{                                                                             \
  if (color_map)                                                              \
      for (ds=0 ; ds < npolys ; ds++)                                         \
	  CLAMP(&xgl_facet[ds].color,&color_map[color_index[polys[ds]]]);     \
  else                                                                        \
      for (ds=0 ; ds < npolys ; ds++)                                         \
	  CLAMP(&xgl_facet[ds].color,&colors[polys[ds]]) ;		      \
}

#define CdpCmapLoadColor(dp, op) \
  CLAMP(&xgl_pt[dp].color,&color_map[color_index[connections[op]]])

#define CdpCdirLoadColor(dp, op) \
  CLAMP(&xgl_pt[dp].color,&colors[connections[op]])

#define CdpTriLoadColors()				      \
{ 							      \
  if (color_map)					      \
      for (dp=0, ds=0 ; ds < npolys ; ds++)		      \
	{						      \
	  op = polys[ds] * 3 ;				      \
	  CdpCmapLoadColor(dp,op) ;    dp++ ;		      \
	  CdpCmapLoadColor(dp,op+1) ;  dp++ ;		      \
	  CdpCmapLoadColor(dp,op+2) ;  dp++ ;		      \
	}						      \
  else							      \
      for (dp=0, ds=0 ; ds < npolys ; ds++)		      \
	{						      \
	  op = polys[ds] * 3 ;				      \
	  CdpCdirLoadColor(dp,op) ;    dp++ ;		      \
	  CdpCdirLoadColor(dp,op+1) ;  dp++ ;		      \
	  CdpCdirLoadColor(dp,op+2) ;  dp++ ;		      \
        }						      \
}

#define CdpQuadLoadColors()				      \
{							      \
  if (color_map)					      \
      for (dp=0, ds=0 ; ds < npolys ; ds++)		      \
	{						      \
	  op = polys[ds] * 4 ;				      \
	  CdpCmapLoadColor(dp,op) ;    dp++ ;		      \
	  CdpCmapLoadColor(dp,op+1) ;  dp++ ;		      \
	  CdpCmapLoadColor(dp,op+3) ;  dp++ ;		      \
	  CdpCmapLoadColor(dp,op+2) ;  dp++ ;		      \
	}						      \
  else							      \
      for (dp=0, ds=0 ; ds < npolys ; ds++)		      \
	{						      \
	  op = polys[ds] * 4 ;				      \
	  CdpCdirLoadColor(dp,op) ;    dp++ ;		      \
	  CdpCdirLoadColor(dp,op+1) ;  dp++ ;		      \
	  CdpCdirLoadColor(dp,op+3) ;  dp++ ;		      \
	  CdpCdirLoadColor(dp,op+2) ;  dp++ ;		      \
        }						      \
}



#define NnoneLoadNormals()

#define NdcLoadNormals()				      \
{							      \
  for (ds=0 ; ds < npolys ; ds++)			      \
      *(Vector *)&xgl_facet[ds].normal = normals[polys[ds]] ; \
}

#define NdpLoadNormal(dp, op) \
  *(Vector *)&xgl_pt[dp].normal = normals[connections[op]] ;

#define NdpTriLoadNormals()				      \
{							      \
  for (dp=0, ds=0 ; ds < npolys ; ds++)			      \
    {							      \
      op = polys[ds] * 3 ;				      \
      NdpLoadNormal(dp,op) ;    dp++ ;			      \
      NdpLoadNormal(dp,op+1) ;  dp++ ;			      \
      NdpLoadNormal(dp,op+2) ;  dp++ ;			      \
    }			        			      \
}

#define NdpQuadLoadNormals()				      \
{							      \
  for (dp=0, ds=0 ; ds < npolys ; ds++)			      \
    {							      \
      op = polys[ds] * 4 ;				      \
      NdpLoadNormal(dp,op) ;    dp++ ;			      \
      NdpLoadNormal(dp,op+1) ;  dp++ ;			      \
      NdpLoadNormal(dp,op+3) ;  dp++ ;			      \
      NdpLoadNormal(dp,op+2) ;  dp++ ;			      \
    }							      \
}



#ifdef MJHDEBUG
#define DebugMessage(fun)                                                    \
{                                                                            \
  fprintf (stderr, "\n(_dxf_%s", fun) ;                                      \
  fprintf (stderr, "\nnumber of points: %d", npoints) ;                      \
  fprintf (stderr, "\nnumber of connections: %d", nshapes) ;                 \
  fprintf (stderr, "\ncolors: 0x%x, color_map: 0x%x, dep on pos: %d",        \
           (int)colors, (int)color_map, colorDepPos) ;                       \
  fprintf (stderr, "\nopacities: 0x%x, opacity_map: 0x%x, dep on pos: %d",   \
           (int)opacities, (int)opacity_map, opacityDepPos) ;                \
  fprintf (stderr, "\nnormals: 0x%x, dep on pos: %d",                        \
           (int)normals, normalDepPos) ;                                     \
  fprintf (stderr, "\nambient, diffuse, specular: %f %f %f",                 \
           (float)k_ambi, (float)k_diff, (float)k_spec) ;                    \
}
#else /* MJHDEBUG */
#define DebugMessage(fun) {}
#endif /* MJHDEBUG */


#define DefPolygonDraw(_Function_,_Shape_,_Sides_,_Cd_,_Nd_,_PointType_,_PointEnum_,_FacetType_  ,_FacetVar_,_FacetEnum_,_Illum_,_ColorSelect_,_LoadPoints_,_LoadColors_,_LoadNormals_,_LineInterp_,_LineColorSelect_)                       \
{									     \
  int dp, op, ds, os ;							     \
  int mod, *polys, npolys, npoly_i, from_cache ;                             \
  char *ptr, *mem, *color_index = (char *)colors ;			     \
  Xgl_pt_list *xgl_pt_list ;						     \
  Xgl_facet_list xgl_facet_list, *xgl_facet_list_ptr ;			     \
  _FacetType_ *xgl_facet ;						     \
  _PointType_ *xgl_pt ;							     \
									     \
  DEFPORT(portHandle) ;							     \
  DebugMessage(_Function_) ;						     \
									     \
  mod = buttonUp ? xf->attributes.buttonUp.density :			     \
                   xf->attributes.buttonDown.density ;			     \
									     \
  if (ptr = mem = _dxf_GetPolyCacheXGL (_Function_, mod, f))		     \
    {									     \
      DPRINT("\ngot polygons from cache") ;				     \
      from_cache = 1 ;                                                       \
                                                                             \
      npolys = *(int *)ptr ; ptr += sizeof(int) ;			     \
      SetPointList(ptr,_PointType_,_Sides_) ;				     \
      SetFacetList(ptr,_FacetType_,_FacetVar_,_FacetEnum_) ;		     \
    }									     \
  else									     \
    {									     \
      DPRINT("\ngenerating xgl polygons") ;				     \
      DPRINT1("\n%d invalid connections",				     \
	      xf->invCntns? DXGetInvalidCount(xf->invCntns): 0) ;	     \
									     \
      from_cache = 0 ;                                                       \
                                                                             \
      /* do a pre-traversal to count valid non-skipped polygons */    	     \
      if (!(polys = (int *) tdmAllocate((1 + (nshapes-1)/mod)*sizeof(int)))) \
	  DXErrorReturn (ERROR_INTERNAL, "#13000") ;			     \
									     \
      for (dp=0, ds=0 ; ds < nshapes ; ds+=mod)				     \
	  if (xf->invCntns)						     \
	    {								     \
	      if (DXIsElementValid(xf->invCntns, ds))			     \
		  polys[dp++] = ds ;					     \
	    }								     \
	  else								     \
	      polys[dp++] = ds ;					     \
									     \
      npolys = dp ;							     \
									     \
      /* allocate point and facet list array */				     \
      ptr = mem =							     \
	  (char *) tdmAllocate (sizeof(int) +				     \
				PointListSize(_PointType_,_Sides_) +	     \
				FacetListSize(_FacetEnum_,_FacetType_)) ;    \
									     \
      if (!ptr)								     \
	{								     \
	  tdmFree((Pointer)polys) ;					     \
	  DXErrorReturn (ERROR_INTERNAL, "#13000") ;			     \
	}								     \
									     \
      /* set up point list and facet list pointers */			     \
      *(int *)ptr = npolys ; ptr += sizeof(int) ;			     \
      SetPointList(ptr,_PointType_,_Sides_) ;				     \
      SetFacetList(ptr,_FacetType_,_FacetVar_,_FacetEnum_) ;		     \
									     \
      /* copy point, color, and normal data */				     \
      _LoadPoints_(_PointEnum_) ;                                            \
      _LoadColors_() ;  						     \
      _LoadNormals_() ;	         					     \
									     \
      /* free temporary storage */					     \
      tdmFree((Pointer)polys) ;						     \
    }									     \
									     \
  if (approx == approx_dots)						     \
    {									     \
      DPRINT("\ncalling xgl_multimarker") ;				     \
      for(npoly_i=0;npoly_i<npolys;npoly_i++){				     \
        if(_ColorSelect_ == XGL_SURF_COLOR_FACET)			     \
            xgl_object_set (XGLCTX,					     \
	  	     XGL_CTX_MARKER_COLOR,                                   \
                     &xgl_facet_list.facets._FacetVar_[npoly_i],	     \
		     XGL_CTX_MARKER_DESCRIPTION, xgl_marker_dot,	     \
		     XGL_CTX_MARKER_COLOR_SELECTOR, XGL_MARKER_COLOR_CONTEXT,\
		     NULL) ;						     \
        else								     \
            xgl_object_set (XGLCTX,					     \
	  	     XGL_CTX_MARKER_COLOR, &colors[0],			     \
		     XGL_CTX_MARKER_DESCRIPTION, xgl_marker_dot,	     \
		     XGL_CTX_MARKER_COLOR_SELECTOR, XGL_MARKER_COLOR_CONTEXT,\
		     NULL) ;						     \
									     \
         xgl_multimarker (XGLCTX, &xgl_pt_list[npoly_i]) ;		     \
      }									     \
    }									     \
  else									     \
  if (approx == approx_lines)						     \
    {									     \
      DPRINT("\ncalling xgl_multipolyline") ;			             \
      for(npoly_i=0;npoly_i<npolys;npoly_i++){				     \
        if(_ColorSelect_ == XGL_SURF_COLOR_FACET)			     \
            xgl_object_set (XGLCTX,					     \
	  	     XGL_CTX_LINE_COLOR,                                     \
		     &xgl_facet_list.facets._FacetVar_[npoly_i],	     \
		     XGL_3D_CTX_LINE_COLOR_INTERP, FALSE,		     \
		     XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_CONTEXT,    \
		     NULL) ;						     \
        else								     \
            xgl_object_set (XGLCTX,					     \
	  	     XGL_CTX_LINE_COLOR, &colors[0],			     \
		     XGL_3D_CTX_LINE_COLOR_INTERP, FALSE,		     \
		     XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_CONTEXT,    \
		     NULL) ;						     \
									     \
         xgl_multipolyline (XGLCTX, NULL, 1, &xgl_pt_list[npoly_i]) ;	     \
       }								     \
    }									     \
  else									     \
    {									     \
    DPRINT("\ncalling xgl_multi_simple_polygon") ;			     \
    if(	_ColorSelect_ == XGL_SURF_COLOR_FACET)				     \
      xgl_object_set (XGLCTX,						     \
		      XGL_3D_CTX_SURF_FRONT_ILLUMINATION, _Illum_,	     \
		      XGL_3D_CTX_SURF_BACK_ILLUMINATION, _Illum_,	     \
		      XGL_3D_CTX_SURF_FACE_DISTINGUISH, TRUE,		     \
		      XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,    \
		      XGL_CTX_SURF_FRONT_COLOR_SELECTOR,_ColorSelect_,	     \
		      XGL_CTX_SURF_FRONT_COLOR, colors,			     \
		      XGL_3D_CTX_SURF_FRONT_AMBIENT,  (float)k_ambi,	     \
		      XGL_3D_CTX_SURF_FRONT_DIFFUSE,  (float)k_diff,	     \
		      XGL_3D_CTX_SURF_FRONT_SPECULAR, (float)k_spec,	     \
		      XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_SOLID,  \
		      XGL_3D_CTX_SURF_BACK_COLOR_SELECTOR,_ColorSelect_,     \
		      XGL_3D_CTX_SURF_BACK_COLOR, colors,		     \
		      XGL_3D_CTX_SURF_BACK_AMBIENT,  (float)k_ambi,	     \
		      XGL_3D_CTX_SURF_BACK_DIFFUSE,  (float)k_diff,	     \
		      XGL_3D_CTX_SURF_BACK_SPECULAR, (float)k_spec,	     \
		      NULL) ;						     \
     else								     \
      xgl_object_set (XGLCTX,						     \
		      XGL_3D_CTX_SURF_FRONT_ILLUMINATION, _Illum_,	     \
		      XGL_3D_CTX_SURF_FACE_DISTINGUISH, FALSE,		     \
		      XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_SOLID,    \
		      XGL_CTX_SURF_FRONT_COLOR_SELECTOR,_ColorSelect_,	     \
		      XGL_CTX_SURF_FRONT_COLOR, colors,			     \
		      XGL_3D_CTX_SURF_FRONT_AMBIENT,  (float)k_ambi,	     \
		      XGL_3D_CTX_SURF_FRONT_DIFFUSE,  (float)k_diff,	     \
		      XGL_3D_CTX_SURF_FRONT_SPECULAR, (float)k_spec,	     \
		      NULL) ;						     \
									     \
      if (opacities && USE_SCREEN_DOOR)					     \
	  if ((opacity_map?						     \
	       opacity_map[*(char *)opacities]: opacities[0]) < 0.75)	     \
	      xgl_object_set						     \
		  (XGLCTX,						     \
		   XGL_CTX_SURF_FRONT_FILL_STYLE, XGL_SURF_FILL_STIPPLE,     \
		   XGL_CTX_SURF_FRONT_FPAT, SCREEN_DOOR_50,		     \
		   XGL_3D_CTX_SURF_BACK_FILL_STYLE, XGL_SURF_FILL_STIPPLE,   \
		   XGL_3D_CTX_SURF_BACK_FPAT, SCREEN_DOOR_50,		     \
		   NULL) ;						     \
									     \
      xgl_multi_simple_polygon (XGLCTX, FacetFlags(_Sides_),		     \
				xgl_facet_list_ptr, NULL,		     \
				npolys, xgl_pt_list) ;			     \
    }									     \
									     \
  if (!from_cache)                                                           \
      /* cache XGL polygons */						     \
      _dxf_PutPolyCacheXGL (_Function_, mod, f, mem) ;			     \
                                                                             \
  DPRINT(")") ;								     \
  return 1 ;								     \
}



int _dxf_CdfNnoneTriDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdfNnoneTriDraw2D",Tri, 3, CDF, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE, XGL_SURF_COLOR_CONTEXT,
		      TriLoadPoints2D, CdfLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
    }
  else
      DefPolygonDraw ("CdfNnoneTriDraw3D",Tri, 3, CDF, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE, XGL_SURF_COLOR_CONTEXT,
		      TriLoadPoints3D, CdfLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
}

int _dxf_CdpNnoneTriDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdpNnoneTriDraw2D", Tri, 3, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE_INTERP_COLOR,
		      XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      TriLoadPoints2D, CdpTriLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX) ;
    }
  else
      DefPolygonDraw ("CdpNnoneTriDraw3D", Tri, 3, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE_INTERP_COLOR,
		      XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      TriLoadPoints3D, CdpTriLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX) ;
}

int _dxf_CdcNnoneTriDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdcNnoneTriDraw2D", Tri, 3, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_NONE, XGL_SURF_COLOR_FACET,
		      TriLoadPoints2D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdcNnoneTriDraw3D", Tri, 3, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_NONE, XGL_SURF_COLOR_FACET,
		      TriLoadPoints3D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdpFlatTriDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdpFlatTriDraw2D", Tri, 3, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      TriLoadPoints2D, CdpTriLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdpFlatTriDraw3D", Tri, 3, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      TriLoadPoints3D, CdpTriLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcFlatTriDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdcFlatTriDraw2D", Tri, 3, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_FACET,
		      TriLoadPoints2D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdcFlatTriDraw3D", Tri, 3, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_FACET,
		      TriLoadPoints3D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdfNdpTriDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdfNdpTriDraw3D", Tri, 3, CDF, NDP,
		      Xgl_pt_normal_f3d, XGL_PT_NORMAL_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_CONTEXT,
		      TriLoadPoints3D, CdfLoadColors, NdpTriLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
}

int _dxf_CdpNdpTriDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdpNdpTriDraw3D", Tri, 3, CDP, CDP,
		      Xgl_pt_color_normal_f3d, XGL_PT_COLOR_NORMAL_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      TriLoadPoints3D, CdpTriLoadColors, NdpTriLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcNdpTriDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdcNdpTriDraw3D", Tri, 3, CDC, NDP,
		      Xgl_pt_normal_f3d, XGL_PT_NORMAL_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_FACET,
		      TriLoadPoints3D, CdcLoadColors, NdpTriLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdfNdcTriDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdfNdcTriDraw3D", Tri, 3, CDF, NDC,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_normal_facet, normal_facets, XGL_FACET_NORMAL,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_CONTEXT,
		      TriLoadPoints3D, CdfLoadColors, NdcLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
}

int _dxf_CdpNdcTriDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdpNdcTriDraw3D", Tri, 3, CDP, NDC,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      Xgl_normal_facet, normal_facets, XGL_FACET_NORMAL,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      TriLoadPoints3D, CdpTriLoadColors, NdcLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcNdcTriDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdcNdcTriDraw3D", Tri, 3, CDC, NDC,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_normal_facet, color_normal_facets,
		      XGL_FACET_COLOR_NORMAL,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_FACET,
		      TriLoadPoints3D, CdcLoadColors, NdcLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}



int _dxf_CdfNnoneQuadDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdfNnoneQuadDraw2D", Quad, 4, CDF, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE, XGL_SURF_COLOR_CONTEXT,
		      QuadLoadPoints2D, CdfLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
    }
  else
      DefPolygonDraw ("CdfNnoneQuadDraw3D", Quad, 4, CDF, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE, XGL_SURF_COLOR_CONTEXT,
		      QuadLoadPoints3D, CdfLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
}

int _dxf_CdpNnoneQuadDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdpNnoneQuadDraw2D", Quad, 4, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE_INTERP_COLOR,
		      XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      QuadLoadPoints2D, CdpQuadLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdpNnoneQuadDraw3D", Quad, 4, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_NONE_INTERP_COLOR,
		      XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      QuadLoadPoints3D, CdpQuadLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcNnoneQuadDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdcNnoneQuadDraw2D", Quad, 4, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_NONE,XGL_SURF_COLOR_FACET,
		      QuadLoadPoints2D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdcNnoneQuadDraw3D", Quad, 4, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_NONE,XGL_SURF_COLOR_FACET,
		      QuadLoadPoints3D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdpFlatQuadDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdpFlatQuadDraw2D", Quad, 4, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      QuadLoadPoints2D, CdpQuadLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdpFlatQuadDraw3D", Quad, 4, CDP, NNONE,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      QuadLoadPoints3D, CdpQuadLoadColors, NnoneLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcFlatQuadDraw (DEF_ARGS)
{
  if (is_2d)
    {
      struct p2d *pnts2d = (struct p2d *)points ;
      DefPolygonDraw ("CdcNnoneQuadDraw2D", Quad, 4, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_FACET,
		      QuadLoadPoints2D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
    }
  else
      DefPolygonDraw ("CdcNnoneQuadDraw3D", Quad, 4, CDC, NNONE,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_FACET,
		      QuadLoadPoints3D, CdcLoadColors, NnoneLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdfNdpQuadDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdfNdpQuadDraw3D", Quad, 4, CDF, NDP,
		      Xgl_pt_normal_f3d, XGL_PT_NORMAL_F3D,
		      NoFacetType, NoFacetVar,  XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_CONTEXT,
		      QuadLoadPoints3D, CdfLoadColors, NdpQuadLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
}

int _dxf_CdpNdpQuadDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdpNdpQuadDraw3D", Quad, 4, CDP, CDP,
		      Xgl_pt_color_normal_f3d, XGL_PT_COLOR_NORMAL_F3D,
		      NoFacetType, NoFacetVar, XGL_FACET_NONE,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      QuadLoadPoints3D, CdpQuadLoadColors, NdpQuadLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcNdpQuadDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdcNdpQuadDraw3D", Quad, 4, CDC, NDP,
		      Xgl_pt_normal_f3d, XGL_PT_NORMAL_F3D,
		      Xgl_color_facet, color_facets, XGL_FACET_COLOR,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_FACET,
		      QuadLoadPoints3D, CdcLoadColors, NdpQuadLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdfNdcQuadDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdfNdcQuadDraw3D", Quad, 4, CDF, NDC,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_normal_facet, normal_facets, XGL_FACET_NORMAL,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_CONTEXT,
		      QuadLoadPoints3D, CdfLoadColors, NdcLoadNormals,
		      FALSE, XGL_LINE_COLOR_CONTEXT)
}

int _dxf_CdpNdcQuadDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdpNdcQuadDraw3D", Quad, 4, CDP, NDC,
		      Xgl_pt_color_f3d, XGL_PT_COLOR_F3D,
		      Xgl_normal_facet, normal_facets, XGL_FACET_NORMAL,
		      XGL_ILLUM_PER_VERTEX, XGL_SURF_COLOR_VERTEX_ILLUM_INDEP,
		      QuadLoadPoints3D, CdpQuadLoadColors, NdcLoadNormals,
		      TRUE, XGL_LINE_COLOR_VERTEX)
}

int _dxf_CdcNdcQuadDraw (DEF_ARGS)
{
      DefPolygonDraw ("CdcNdcQuadDraw3D", Quad, 4, CDC, NDC,
		      Xgl_pt_f3d, XGL_PT_F3D,
		      Xgl_color_normal_facet, color_normal_facets,
		      XGL_FACET_COLOR_NORMAL,
		      XGL_ILLUM_PER_FACET, XGL_SURF_COLOR_FACET,
		      QuadLoadPoints3D, CdcLoadColors, NdcLoadNormals,
		      FALSE, XGL_LINE_COLOR_VERTEX)
}



#define CallPolygonDraw(_Shape_)				      \
{								      \
  Point *points ;						      \
  RGBColor *colors, *color_map ;				      \
  Vector *normals ;						      \
  int ret, *connections, nconnections, approx ;          	      \
  Type type ;                                                         \
  int rank, shape, is_2d ;                                            \
  float *opacities, *opacity_map;				      \
  DEFPORT(portHandle) ;						      \
  								      \
  DPRINT("\n(_dxfPolygonDraw") ;				      \
  								      \
  is_2d = IS_2D(xf->positions_array, type, rank, shape) ;             \
                                                                      \
  points = (Point *) DXGetArrayData (xf->positions_array) ;	      \
  normals = (Vector *) DXGetArrayData(xf->normals_array) ;	      \
  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;	      \
  opacity_map = (float *) DXGetArrayData(xf->omap_array) ;	      \
								      \
  if (xf->origConnections_array)				      \
    {								      \
      /* connections array had been transformed into strips */	      \
      connections = (int *) DXGetArrayData (xf->origConnections_array) ;  \
      nconnections = xf->origNConnections ;			      \
    }								      \
  else								      \
    {								      \
      connections = (int *) DXGetArrayData (xf->connections_array) ;  \
      nconnections = xf->nconnections ;				      \
    }								      \
								      \
  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)      \
      colors = (Pointer) DXGetArrayEntry(xf->fcolors, 0, NULL) ;      \
  else								      \
      colors = (Pointer) DXGetArrayData(xf->fcolors_array) ;	      \
  								      \
  if (DXGetArrayClass(xf->opacities_array) == CLASS_CONSTANTARRAY)    \
      opacities = (Pointer) DXGetArrayEntry(xf->opacities, 0, NULL) ; \
  else								      \
      opacities = (Pointer) DXGetArrayData(xf->opacities_array) ;     \
  								      \
  approx = buttonUp ? xf->attributes.buttonUp.approx :		      \
                      xf->attributes.buttonDown.approx ;	      \
  								      \
  if (!normals || approx == approx_lines || approx == approx_dots)    \
    {								      \
      if (xf->colorsDep == dep_field)				      \
	  ret =  _dxf_CdfNnone##_Shape_##Draw (ARGS) ;		      \
      else if (xf->colorsDep == dep_positions)			      \
	  ret =  _dxf_CdpNnone##_Shape_##Draw (ARGS) ;		      \
      else							      \
	  ret =  _dxf_CdcNnone##_Shape_##Draw (ARGS) ;		      \
    }								      \
  else if (xf->normalsDep == dep_field)				      \
    {								      \
      if (xf->colorsDep == dep_positions)			      \
	  ret =  _dxf_CdpFlat##_Shape_##Draw (ARGS) ;		      \
      else							      \
	  ret =  _dxf_CdcFlat##_Shape_##Draw (ARGS) ;		      \
    }								      \
  else if (xf->normalsDep == dep_positions)			      \
    {								      \
      if (xf->colorsDep == dep_field)				      \
	  ret =  _dxf_CdfNdp##_Shape_##Draw (ARGS) ;		      \
      else if (xf->colorsDep == dep_positions)			      \
	  ret =  _dxf_CdpNdp##_Shape_##Draw (ARGS) ;		      \
      else							      \
	  ret =  _dxf_CdcNdp##_Shape_##Draw (ARGS) ;		      \
    }								      \
  else								      \
    {								      \
      if (xf->colorsDep == dep_field)				      \
	  ret =  _dxf_CdfNdc##_Shape_##Draw (ARGS) ;		      \
      else if (xf->colorsDep == dep_positions)			      \
	  ret =  _dxf_CdpNdc##_Shape_##Draw (ARGS) ;		      \
      else							      \
	  ret =  _dxf_CdcFlat##_Shape_##Draw (ARGS) ;		      \
    }								      \
  DPRINT(")") ;							      \
  return ret ;							      \
}

int _dxfTriDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  CallPolygonDraw(Tri) ;
}

int _dxfQuadDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  CallPolygonDraw(Quad) ;
}
