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
 * $Source: /src/master/dx/src/exec/hwrender/xgl/hwPolylineDraw.c,v $
 *
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

#define DEF_ARGS \
  tdmPortHandleP portHandle, xfieldT *xf, \
  int mod, enum approxE approx, \
  int npoints, Point *points, \
  int nPl, int *polylines, int nE, int *edges, \
  RGBColor *fcolors, RGBColor *color_map, \
  int is_2d

#define ARGS \
  portHandle, xf, \
  mod, approx, \
  xf->npositions, points, \
  xf->npolylines, xf->polylines, xf->nedges, xf->edges, \
  fcolors, color_map, is_2d

#ifdef MJHDEBUG
#define DebugMessage()                                                        \
{									      \
  int i ;								      \
  Xgl_pt Min, Max ;							      \
  Xgl_pt_f3d min, max ;							      \
                                                                              \
  fprintf (stderr, "\n2D: %s", is_2d ? "yes" : "no") ;			      \
  fprintf (stderr, "\nnumber of points: %d", xf->npositions) ;   	      \
  fprintf (stderr, "\nnumber of connections: %d", nshapes) ;         \
  fprintf (stderr, "\n%d invalid connections",                                \
	   xf->invCntns? DXGetInvalidCount(xf->invCntns): 0) ;                \
  fprintf (stderr, "\ncolors: 0x%x, color_map: 0x%x, dep on pos: %d",	      \
	   (int)fcolors, (int)color_map, xf->fcolorsDep == dep_positions) ;   \
 									      \
  min.x = min.y = min.z = +MAXFLOAT ;	      		                      \
  Min.pt_type = XGL_PT_F3D ;						      \
  Min.pt.f3d = &min ;							      \
 									      \
  max.x = max.y = max.z = -MAXFLOAT ;  			                      \
  Max.pt_type = XGL_PT_F3D ;						      \
  Max.pt.f3d = &max ;							      \
									      \
  if (is_2d)								      \
    {									      \
      struct p2d *pnts2d = (struct p2d *)points ;                             \
      min.z = max.z = 0 ;						      \
      for (i=0 ; i<xf->npositions ; i++)				      \
	{								      \
	  if (pnts2d[i].x < min.x) min.x = pnts2d[i].x ;		      \
	  if (pnts2d[i].y < min.y) min.y = pnts2d[i].y ;		      \
 									      \
	  if (pnts2d[i].x > max.x) max.x = pnts2d[i].x ;		      \
	  if (pnts2d[i].y > max.y) max.y = pnts2d[i].y ;		      \
	}								      \
    }									      \
  else									      \
      for (i=0 ; i<xf->npositions ; i++)				      \
	{								      \
	  if (points[i].x < min.x) min.x = points[i].x ;		      \
	  if (points[i].y < min.y) min.y = points[i].y ;		      \
	  if (points[i].z < min.z) min.z = points[i].z ;		      \
 									      \
	  if (points[i].x > max.x) max.x = points[i].x ;		      \
	  if (points[i].y > max.y) max.y = points[i].y ;		      \
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
tdmCdfPolylineDraw (DEF_ARGS)
{
  char *mem ;
  int from_cache, i;
  int nextpt, nextpl;
  Xgl_pt_list *pt_list ;
  Xgl_color cpf_color ;
  Xgl_pt_f3d *pt_f3d ;
  struct p2d *pnts2d = (struct p2d *)points ;
  char *color_index = (char *)fcolors ;
  int  nplines, nindices;

  DEFPORT(portHandle) ;
  DPRINT("\n(CdfPolylineDraw") ;

  if (mem = _dxf_GetLineCacheXGL ("CdfPolylineDraw", mod, xf))
    {
      DPRINT("\ngot lines from cache") ;
      from_cache = 1 ;

      nplines = *(int *) mem ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
    }
  else
    {
      DPRINT("\ngenerating XGL lines") ;
      from_cache = 0 ;

      if (xf->invCntns)
      {
	  for (nplines = nindices = 0, i=0 ; i<nPl ; i++)
	  {
	      if (DXIsElementValid (xf->invCntns, i))
	      {
		  int estart = polylines[i];
		  int eend = (i >= nPl-1) ? nE : polylines[i+1];

		  nplines ++;
		  nindices += (eend - estart);
	      }
	  }
      }
      else
      {
	  nplines  = nPl;
	  nindices = nE;
      }

      mem = (char *) tdmAllocate (sizeof(int) + 
				  sizeof(Xgl_pt_list)*nplines +
				  sizeof(Xgl_pt_f3d)*nindices) ;

      if (!mem)
	  goto error;

      pt_list = (Xgl_pt_list *)(mem+sizeof(int));
      pt_f3d = (Xgl_pt_f3d *)(mem+sizeof(int)+sizeof(Xgl_pt_list)*nplines);

      for (nextpl=0, nextpt=0, i=0 ; i < nPl ; i++ )
      {
	  if (! xf->invCntns || DXIsElementValid (xf->invCntns, i))
	  {
	      int estart = polylines[i];
	      int eend = (i >= nPl-1) ? nE : polylines[i+1];
	      int e;

	      pt_list[nextpl].pt_type = XGL_PT_F3D;
	      pt_list[nextpl].pts.f3d = pt_f3d + nextpt;
	      pt_list[nextpl].bbox    = NULL;
	      pt_list[nextpl].num_pts = eend - estart;
	      nextpl ++;

	      for (e = estart; e < eend; e++)
	      {
		  int index = edges[e];

		  if (is_2d)
		  {
		      *(struct p2d *)(pt_f3d+nextpt) = pnts2d[index];
		      pt_f3d[nextpt].z = 0 ;
		  }
		  else
		  {
		      pt_f3d[nextpt] = *(Xgl_pt_f3d *) &points[index];
		  }

		  nextpt ++;
	      }
	  }
      }

      nplines = *((int *)mem) = nextpl;
    }


  if (color_map)
    CLAMP(&cpf_color,&color_map[color_index[0]]);
  else
    CLAMP(&cpf_color,&fcolors[0]) ;

  xgl_object_set (XGLCTX,
		  XGL_CTX_LINE_COLOR_SELECTOR,
		  XGL_LINE_COLOR_CONTEXT,
		  XGL_CTX_LINE_COLOR, &cpf_color,
		  0) ;

  DPRINT("\ncall xgl_multipolyline") ;
  xgl_multipolyline (XGLCTX, NULL, nplines, pt_list) ;


  if (!from_cache)
      _dxf_PutLineCacheXGL ("CdfPolylineDraw", mod, xf, mem) ;

  DPRINT(")") ;
  return 1 ;

 error:
  DPRINT("\nerror)") ;
  return 0 ;
}


int
tdmCdcPolylineDraw (DEF_ARGS)
{
  char *mem ;
  int nplines, nindices, from_cache, i;
  int nextpt, nextpl;
  Xgl_pt_list *pt_list ;
  Xgl_pt_color_f3d *pt_color_f3d ;
  struct p2d *pnts2d = (struct p2d *)points ;
  DEFPORT(portHandle) ;
  DPRINT("\n(CdcPolylineDraw") ;

  if (mem = _dxf_GetLineCacheXGL ("CdcPolylineDraw", mod, xf))
    {
      DPRINT("\ngot lines from cache") ;
      from_cache = 1 ;

      nplines = *(int *) mem ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
    }
  else
    {
      DPRINT("\ngenerating XGL lines") ;
      from_cache = 0 ;

      if (xf->invCntns)
      {
	  for (nplines = nindices = 0, i=0 ; i<nPl ; i++)
	  {
	      if (DXIsElementValid (xf->invCntns, i))
	      {
		  int estart = polylines[i];
		  int eend = (i >= nPl-1) ? nE : polylines[i+1];

		  nplines ++;
		  nindices += (eend - estart);
	      }
	  }
      }
      else
      {
	  nplines  = nPl;
	  nindices = nE;
      }

      mem = (char *) tdmAllocate (sizeof(int) + 
				  sizeof(Xgl_pt_list)*nplines +
				  sizeof(Xgl_pt_color_f3d)*nindices) ;

      if (!mem) 
	 goto error;


      pt_list = (Xgl_pt_list *) (mem + sizeof(int));
      pt_color_f3d = (Xgl_pt_color_f3d *)
		  (mem + sizeof(int) + sizeof(Xgl_pt_list)*nplines) ;

      for (nextpl=0, nextpt=0, i=0 ; i < nPl ; i++ )
      {
	  if (! xf->invCntns || DXIsElementValid (xf->invCntns, i))
	  {
	      int estart = polylines[i];
	      int eend = (i >= nPl-1) ? nE : polylines[i+1];
	      int e;

	      pt_list[nextpl].pt_type       = XGL_PT_COLOR_F3D;
	      pt_list[nextpl].pts.color_f3d = pt_color_f3d + nextpt;
	      pt_list[nextpl].bbox          = NULL;
	      pt_list[nextpl].num_pts       = eend - estart;
	      nextpl ++;

	      for (e = estart; e < eend; e++)
	      {
		  int index = edges[e];
		  if (is_2d)
		  {
		      *(struct p2d *)(pt_color_f3d+nextpt) = pnts2d[index];
		      pt_color_f3d[nextpt].z = 0 ;
		  }
		  else
		  {
		      pt_color_f3d[nextpt] = *(Xgl_pt_color_f3d *) &points[index];
		  }

		  if (color_map)
		  {
		      char *color_index = (char *)fcolors ;
		      CLAMP(&pt_color_f3d[nextpt].color,
					    &color_map[color_index[i]]) ;
		  }
		  else
		  {
		      CLAMP(&pt_color_f3d[nextpt].color, &fcolors[i]) ;
		  }

		  nextpt ++;
	      }
	  }
      }

      nplines = *((int *)mem) = nextpl;
    }

  if (approx == approx_dots)
    {
      /*
       *  Dot approximations are more efficiently rendered by
       *  _dxfUnconnectedPointDraw(), but connection-dependent colors
       *  are not be handled by that routine.
       */
      Xgl_pt_list dot_pt_list ;

      dot_pt_list.pt_type       = XGL_PT_COLOR_F3D ;
      dot_pt_list.pts.color_f3d = pt_color_f3d;
      dot_pt_list.bbox          = NULL ;
      dot_pt_list.num_pts       = nextpt;

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
  else
    {
      xgl_object_set (XGLCTX,
		      XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_VERTEX,
		      XGL_3D_CTX_LINE_COLOR_INTERP, FALSE,
		      0) ;

      DPRINT("\ncalling xgl_multipolyline") ;
      xgl_multipolyline (XGLCTX, NULL, nplines, pt_list) ;
    }

  if (!from_cache)
      _dxf_PutLineCacheXGL ("CdcPolylineDraw", mod, xf, mem) ;

  DPRINT(")") ;
  return 1 ;

 error:
  DPRINT("\nerror)") ;
  return 0 ;
}


int
tdmCdpPolylineDraw (DEF_ARGS)
{
  char *mem ;
  int from_cache, i;
  int nextpt, nextpl;
  Xgl_pt_list *pt_list ;
  Xgl_pt_color_f3d *pt_color_f3d ;
  struct p2d *pnts2d = (struct p2d *)points ;
  int nplines, nindices;

  DEFPORT(portHandle) ;
  DPRINT("\n(CdpPolylineDraw") ;

  if (mem = _dxf_GetLineCacheXGL ("CdpPolylineDraw", mod, xf))
    {
      DPRINT("\ngot lines from cache") ;
      from_cache = 1 ;

      nplines = *(int *) mem ;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
    }
  else
    {
      DPRINT("\ngenerating XGL lines") ;
      from_cache = 0 ;

      if (xf->invCntns)
      {
	  for (nplines = nindices = 0, i=0 ; i<nPl ; i++)
	  {
	      if (DXIsElementValid (xf->invCntns, i))
	      {
		  int estart = polylines[i];
		  int eend = (i >= nPl-1) ? nE : polylines[i+1];

		  nplines ++;
		  nindices += (eend - estart);
	      }
	  }
      }
      else
      {
	  nplines  = nPl;
	  nindices = nE;
      }

      mem = (char *) tdmAllocate (sizeof(int) +
				  sizeof(Xgl_pt_list)*nplines +
				  sizeof(Xgl_pt_color_f3d)*nindices) ;

      if (!mem)
	  goto error;

      *((int *)mem) = nplines;
      pt_list = (Xgl_pt_list *) (mem + sizeof(int)) ;
      pt_color_f3d = (Xgl_pt_color_f3d *)
	  (mem + sizeof(int) + sizeof(Xgl_pt_list)*nplines) ;

      for (nextpl=0, nextpt=0, i=0 ; i < nPl ; i++ )
      {
	  if (! xf->invCntns || DXIsElementValid (xf->invCntns, i))
	  {
	      int estart = polylines[i];
	      int eend = (i >= nPl-1) ? nE : polylines[i+1];
	      int e;

	      pt_list[nextpl].pt_type       = XGL_PT_COLOR_F3D;
	      pt_list[nextpl].pts.color_f3d = pt_color_f3d + nextpt;
	      pt_list[nextpl].bbox          = NULL;
	      pt_list[nextpl].num_pts       = eend - estart;
	      nextpl ++;

	      for (e = estart; e < eend; e++)
	      {
		  int index = edges[e];
		  if (is_2d)
		  {
		      *(struct p2d *)(pt_color_f3d+nextpt) = pnts2d[index];
		      pt_color_f3d[nextpt].z = 0 ;
		  }
		  else
		  {
		      pt_color_f3d[nextpt] = *(Xgl_pt_color_f3d *) &points[index];
		  }

		  if (color_map)
		  {
		      char *color_index = (char *)fcolors ;
		      CLAMP(&pt_color_f3d[nextpt].color,
					    &color_map[color_index[e]]) ;
		  }
		  else
		  {
		      CLAMP(&pt_color_f3d[nextpt].color, &fcolors[e]) ;
		  }

		  nextpt ++;
	      }
	  }
      }

      nplines = *((int *)mem) = nextpl;
    }

  xgl_object_set (XGLCTX,
		  XGL_CTX_LINE_COLOR_SELECTOR, XGL_LINE_COLOR_VERTEX,
		  XGL_3D_CTX_LINE_COLOR_INTERP, TRUE,
		  0) ;

  DPRINT("\ncalling xgl_multipolyline") ;
  xgl_multipolyline (XGLCTX, NULL, nplines, pt_list) ;

  if (!from_cache)
      _dxf_PutLineCacheXGL ("CdpPolylineDraw", mod, xf, mem) ;

  DPRINT(")") ;
  return 1 ;

 error:
  DPRINT("\nerror)") ;
  return 0 ;
}


int
_dxfPolylineDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  Point *points ;
  RGBColor *fcolors, *color_map ;
  int *connections, mod, ret ;
  int	nshapes;
  Type type ;
  int rank, shape, is_2d ;
  enum approxE approx ;
  DEFPORT(portHandle) ;

  DPRINT("\n(_dxfPolylineDraw") ;

  /*
   *  Extract required data from the xfield.
   */

  is_2d = IS_2D(xf->positions_array, type, rank, shape) ; 

  points = (Point *) DXGetArrayData (xf->positions_array) ;
  color_map = (RGBColor *) DXGetArrayData(xf->cmap_array) ;

  if(xf->origConnections_array) 
    {
    /* if the field has been made into a polyline, use origConnections */
    connections = (int *) DXGetArrayData (xf->origConnections_array) ;
    nshapes = xf->origNConnections;
    }
   else 
    {
    connections = (int *) DXGetArrayData (xf->connections_array) ;
    nshapes = xf->nconnections;
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

  DebugMessage() ;

  if (xf->colorsDep == dep_field)
      ret = tdmCdfPolylineDraw(ARGS) ;
  else if (xf->colorsDep == dep_positions)
      ret = tdmCdpPolylineDraw(ARGS) ;
  else
      ret = tdmCdcPolylineDraw(ARGS) ;

  DPRINT(")") ;
  return ret ;
}
