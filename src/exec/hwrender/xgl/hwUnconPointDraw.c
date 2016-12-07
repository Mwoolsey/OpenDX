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
$Source: /src/master/dx/src/exec/hwrender/xgl/hwUnconPointDraw.c,v $
$Log: hwUnconPointDraw.c,v $
Revision 1.3  1999/05/10 15:45:40  gda
Copyright message

Revision 1.3  1999/05/10 15:45:40  gda
Copyright message

Revision 1.2  1999/05/03 14:06:43  gda
moved to using dxconfig.h rather than command-line defines

Revision 1.1.1.1  1999/03/24 15:18:36  gda
Initial CVS Version

Revision 1.1.1.1  1999/03/19 20:59:49  gda
Initial CVS

Revision 10.1  1999/02/23 21:03:12  gda
OpenDX Baseline

Revision 9.1  1997/05/22 22:34:29  svs
Copy of release 3.1.4 code

 * Revision 8.0  1995/10/03  22:16:02  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.4  1994/03/31  15:55:27  tjm
 * added versioning
 *
 * Revision 7.3  94/02/24  18:31:51  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.2  94/02/13  18:52:49  mjh
 * sun4/xgl maintenance: remove RenderPass, use Xfield, integrate 2D code,
 * fix dot approximations for connection-dependent data, use mesh topology
 * info in Xfield directly
 * 
 * Revision 7.1  94/01/18  19:00:17  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:26:21  svs
 * ship level code, release 2.0
 * 
 * Revision 1.5  93/10/27  21:32:10  mjh
 * Handle connection-dependent colors w/o dumping core.  This routine
 * ought not be called for connection-dependent colors, but the other
 * graphics primitives need to for dot approximations.
 * 
 * Revision 1.4  93/09/28  09:40:17  tjm
 * fixed <DMCZAP71> colors are now clamped so that the maximum value for
 * r g or b is 1.0. 
 * 
 * Revision 1.3  93/07/28  19:34:06  mjh
 * apply 2D position hacks, install invalid connection/postion support
 * 
 * Revision 1.2  93/07/14  13:29:08  tjm
 * added solaris ifdefs
 * 
 * Revision 1.1  93/06/29  10:01:49  tjm
 * Initial revision
 * 
 * Revision 5.2  93/05/07  19:53:18  mjh
 * rewrite, optimize copies, use cache
 * 
 * Revision 5.1  93/03/30  14:42:02  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.3  93/03/12  16:12:18  owens
 * duh
 * 
 * Revision 5.0.2.2  93/03/12  16:05:38  owens
 * colors are BYTEs when using a color_map... not INTs
 * 
 * Revision 5.0.2.1  93/03/08  11:10:12  ellen
 * Added stuff to do point draw
 * 
\*---------------------------------------------------------------------------*/

#include <xgl/xgl.h>
#include "hwDeclarations.h"
#include "hwPortXGL.h"
#include "hwXfield.h"
#include "hwMemory.h"
#include "hwCacheUtilXGL.h"

#define COPYPNT2D *(struct p2d *)pt = pnts2d[ol] ; pt->z = 0 ;
#define COPYPNT3D *(Point *)pt = points[ol] ;
#define COPYCOLOR                                   \
if (color_map)                                      \
  CLAMP(&pt->color, &color_map[color_index[ol]]) ;  \
else                                                \
  CLAMP(&pt->color, &fcolors[ol]) ;               

	  
Error
_dxfUnconnectedPointDraw (tdmPortHandleP portHandle, xfieldT *xf, int buttonUp)
{
  DEFPORT(portHandle);
  register Point *points ;
  register struct p2d {float x, y ;} *pnts2d ;
  register int n, ol, mod ;
  RGBColor *fcolors, *color_map ;
  int maxpoints, pt_type, rank, shape, is_2d ;
  char *fun, *color_index ;
  Xgl_pt_list *pt_list ;
  Type type ;
  
  DPRINT("\n(_dxfUnconnectedPointDraw") ;
  DPRINT1("\n%d invalid positions",
	  xf->invPositions? DXGetInvalidCount(xf->invPositions): 0) ;
  
  /*
   *  Extract required data from the xfield.
   */
  
  if (is_2d = IS_2D(xf->positions_array, type, rank, shape))
      pnts2d = (struct p2d *) DXGetArrayData (xf->positions_array) ;
  else
      points = (Point *) DXGetArrayData (xf->positions_array) ;

  if (DXGetArrayClass(xf->fcolors_array) == CLASS_CONSTANTARRAY)
      fcolors = (Pointer) DXGetArrayEntry (xf->fcolors, 0, NULL) ;
  else
      fcolors = (Pointer) DXGetArrayData(xf->fcolors_array) ;
  
  if (color_map = (RGBColor *) DXGetArrayData(xf->cmap_array))
      color_index = (char *) fcolors ;
  
  mod = buttonUp ? xf->attributes.buttonUp.density :
                   xf->attributes.buttonDown.density ;

  if (xf->colorsDep == dep_field)
    {
      float marker_color[3] ;

      if (color_map)
	  CLAMP(marker_color, &color_map[color_index[0]]) ;
      else
	  CLAMP(marker_color, fcolors) ;

      xgl_object_set
	  (XGLCTX, 
	   XGL_CTX_MARKER_COLOR, marker_color,
#ifdef solaris
	   XGL_CTX_MARKER, xgl_marker_dot,
#else
	   XGL_CTX_MARKER_DESCRIPTION, xgl_marker_dot,
#endif
	   XGL_CTX_MARKER_COLOR_SELECTOR, XGL_MARKER_COLOR_CONTEXT,
	   NULL) ;

      if (mod == 1 && !xf->invPositions && !is_2d)
	{
	  Xgl_pt_list pt_list ;
	  DPRINT("\nfast path") ;

	  pt_list.pt_type = XGL_PT_F3D ;
	  pt_list.pts.f3d = (Xgl_pt_f3d *) points ;
	  pt_list.bbox = NULL ;
	  pt_list.num_pts = xf->npositions ;

	  DPRINT("\ncalling xgl_multimarker") ;
	  xgl_multimarker (XGLCTX, &pt_list) ;
	  goto done ;
	}

      fun = "CpfPnt" ;
      pt_type = XGL_PT_F3D ;
    }
  else if (xf->colorsDep == dep_positions)
    {
      xgl_object_set
	  (XGLCTX, 
#ifdef solaris
	   XGL_CTX_MARKER, xgl_marker_dot,
#else
	   XGL_CTX_MARKER_DESCRIPTION, xgl_marker_dot,
#endif
	   XGL_CTX_MARKER_COLOR_SELECTOR, XGL_MARKER_COLOR_POINT,
	   NULL) ;

      fun = "CpvPnt" ;
      pt_type = XGL_PT_COLOR_F3D ;
    }
  else
    {
      /* #13070 bad or missing element type */
      DPRINT("\ncannot handle connection-dependent point colors") ;
      DXErrorGoto (ERROR_INTERNAL, "#13070") ;
    }

  DPRINT1 ("\n%s", fun) ;
  if ((pt_list = _dxf_GetPointCacheXGL (fun, mod, xf)))
    {
      DPRINT("\ngot points from cache") ;
      DPRINT("\ncalling xgl_multimarker") ;
      xgl_multimarker (XGLCTX, pt_list) ;
      goto done ;
    }

  /* set up to render every-nth-point */
  DPRINT("\ngenerating XGL points") ;
  maxpoints = (xf->npositions-1)/mod + 1 ;
  
  /* allocate xgl point list */
  if (!(pt_list = (Xgl_pt_list *) tdmAllocate(sizeof(Xgl_pt_list))))
      DXErrorGoto (ERROR_INTERNAL, "#13000") ;

  /* setup the Xgl_pt_list structure */
  if (pt_type == XGL_PT_F3D)
    {
      register Xgl_pt_f3d *pt ;
	
      pt_list->pts.f3d = (Xgl_pt_f3d *)
	  tdmAllocate(maxpoints*sizeof(Xgl_pt_f3d)) ;

      if (!pt_list->pts.f3d)
	  DXErrorGoto (ERROR_INTERNAL, "#13000") ;
      
      if (is_2d)
	  if (xf->invPositions)
	      for (n=0, ol=0, pt=pt_list->pts.f3d; ol<xf->npositions; ol+=mod)
		{
		  if (!DXIsElementValid (xf->invPositions, ol)) continue ;
		  COPYPNT2D ; n++ ; pt++ ;
		}
	  else
	      for (n=0, ol=0, pt=pt_list->pts.f3d; ol<xf->npositions; ol+=mod)
		{
		  COPYPNT2D ; n++ ; pt++ ;
		}
      else /* 3d */
	  if (xf->invPositions)
	      for (n=0, ol=0, pt=pt_list->pts.f3d; ol<xf->npositions; ol+=mod)
		{
		  if (!DXIsElementValid (xf->invPositions, ol)) continue ;
		  COPYPNT3D ; n++ ; pt++ ;
		}
	  else
	      for (n=0, ol=0, pt=pt_list->pts.f3d; ol<xf->npositions; ol+=mod)
		{
		  COPYPNT3D ; n++ ; pt++ ;
		}
    }
  else /* XGL_PT_COLOR_F3D */
    {
      register Xgl_pt_color_f3d *pt  ;

      pt_list->pts.color_f3d = (Xgl_pt_color_f3d *)
	  tdmAllocate(maxpoints*sizeof(Xgl_pt_color_f3d)) ;

      if (!pt_list->pts.color_f3d)
	  DXErrorGoto (ERROR_INTERNAL, "#13000") ;
      
      if (is_2d)
	  if (xf->invPositions)
	      for (n=0, ol=0, pt=pt_list->pts.color_f3d ;
		   ol < xf->npositions ; ol += mod)
		{
		  if (!DXIsElementValid (xf->invPositions, ol)) continue ;
		  COPYPNT2D ; COPYCOLOR ; n++ ; pt++ ;
		}
	  else
	      for (n=0, ol=0, pt=pt_list->pts.color_f3d ;
		   ol < xf->npositions ; ol += mod)
		{
		  COPYPNT2D ; COPYCOLOR ; n++ ; pt++ ;
		}
      else /* 3d */
	  if (xf->invPositions)
	      for (n=0, ol=0, pt=pt_list->pts.color_f3d ;
		   ol < xf->npositions ; ol += mod)
		{
		  if (!DXIsElementValid (xf->invPositions, ol)) continue ;
		  COPYPNT3D ; COPYCOLOR ; n++ ; pt++ ;
		}
	  else
	      for (n=0, ol=0, pt=pt_list->pts.color_f3d ;
		   ol < xf->npositions ; ol += mod)
		{
		  COPYPNT3D ; COPYCOLOR ; n++ ; pt++ ;
		}
    }

  /* draw all the points */
  pt_list->pt_type = pt_type ;
  pt_list->bbox = NULL ;
  pt_list->num_pts = n ;

  DPRINT("\ncalling xgl_multimarker") ;
  xgl_multimarker (XGLCTX, pt_list) ;
  
  /* cache them */
  _dxf_PutPointCacheXGL (fun, mod, xf, pt_list) ;
  
 done:
  DPRINT(")") ;
  return OK ;
  
 error:
  _dxf_FreePointCacheXGL (pt_list) ;
  DPRINT("\nerror)") ;
  return ERROR ;
}
