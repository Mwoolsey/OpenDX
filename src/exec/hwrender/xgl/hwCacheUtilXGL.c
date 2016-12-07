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
 $Source: /src/master/dx/src/exec/hwrender/xgl/hwCacheUtilXGL.c,v $

  Author: Mark Hood

  Cache utilities for XGL geometry primitives.

 $Log: hwCacheUtilXGL.c,v $
 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.3  1999/05/10 15:45:39  gda
 Copyright message

 Revision 1.2  1999/05/03 14:06:42  gda
 moved to using dxconfig.h rather than command-line defines

 Revision 1.1.1.1  1999/03/24 15:18:35  gda
 Initial CVS Version

 Revision 1.1.1.1  1999/03/19 20:59:49  gda
 Initial CVS

 Revision 10.1  1999/02/23 21:01:40  gda
 OpenDX Baseline

 Revision 9.1  1997/05/22 22:32:57  svs
 Copy of release 3.1.4 code

 * Revision 8.1  1997/03/26  20:40:56  paula
 * Do not use caching, cache tag is dumb and will never get freed
 *
 * Revision 8.0  1995/10/03  22:14:48  nsc
 * Copy of release 3.1 code
 *
 * Revision 7.3  1994/03/31  15:55:06  tjm
 * added versioning
 *
 * Revision 7.2  94/02/24  18:28:28  mjh
 * sun4/xgl maintenance: part 3, remove references to original DX field in
 * the xfield data structure.  XGL objects are now cached on the address of
 * the xfield.  XGL port now no longer requires STUBBED_CALLS preprocessor
 * definition.
 * 
 * Revision 7.1  94/01/18  18:59:53  svs
 * changes since release 2.0.1
 * 
 * Revision 6.1  93/11/16  10:25:58  svs
 * ship level code, release 2.0
 * 
 * Revision 1.2  93/07/28  19:34:16  mjh
 * apply 2D position hacks, install invalid connection/postion support
 * 
 * Revision 1.1  93/06/29  10:01:12  tjm
 * Initial revision
 * 
 * Revision 5.4  93/05/07  19:56:55  mjh
 * add polygon cache
 * 
 * Revision 5.3  93/05/03  18:24:25  mjh
 * add polygon cache, update names tdm -> dxf
 * 
 * Revision 5.2  93/04/30  23:41:09  mjh
 * Fix huge memory leak.  Previous code assumed deallocating clist would
 * deallocate all vertex information.
 * 
 * Revision 5.1  93/03/30  14:41:38  ellen
 * Moved these files from the 5.0.2 Branch
 * 
 * Revision 5.0.2.1  93/01/18  11:01:33  john
 * initial XGL revision
 * 
 * Revision 5.0  93/01/17  16:45:40  owens
 * added caching of tmesh xgl data
\*---------------------------------------------------------------------------*/

#include "hwDeclarations.h"
#include "hwTmesh.h"
#include "hwXfield.h"
#include "hwMemory.h"
#include "hwCacheUtilXGL.h"

/* performance tuning */
/*
 * NO CACHING!!!! never will clean up, and besides the cache
 * tag used (based on the address of the xfield structure) is
 * really dumb.
 */
#undef USE_TSTRIP_CACHE
#undef USE_POINT_CACHE
#undef USE_POLY_CACHE
#undef USE_LINE_CACHE

tdmStripDataXGL *
_dxf_GetTmeshCacheXGL (char *fun, xfieldT *f)
{
  Private o ;
  char cache_id[128] ;
  tdmStripDataXGL *stripsXGL ;
  DPRINT("\n(_dxf_GetTmeshCacheXGL") ;

#ifdef USE_TSTRIP_CACHE
  sprintf (cache_id, "%x-%s", f, fun) ;
  DPRINT1("\nusing %s for cache_id", cache_id) ;

  if (o = (Private) DXGetCacheEntry(cache_id, 1, 0))
    {
      stripsXGL = (tdmStripDataXGL *) DXGetPrivateData(o) ;
      DXDelete((Pointer)o) ;

      DPRINT1("\nreturning %x)", stripsXGL) ;
      return stripsXGL ;
    }
#endif

  DPRINT("\nreturning 0)") ;
  return 0;
}

void
_dxf_PutTmeshCacheXGL (char *fun, xfieldT *f, tdmStripDataXGL *stripsXGL)
{
  Private o ;
  DPRINT("\n(_dxf_PutTmeshCacheXGL") ;

#ifdef USE_TSTRIP_CACHE
  if (o = DXNewPrivate((Pointer)stripsXGL, _dxf_FreeTmeshCacheXGL))
    {
      char cache_id[128] ;
      sprintf (cache_id, "%x-%s", f, fun) ;
      DPRINT1("\nusing %s for cache_id", cache_id) ;

      DXSetCacheEntry((Pointer)o, 0, cache_id, 1, 0) ;
    }
  else
#endif
      _dxf_FreeTmeshCacheXGL((Pointer)stripsXGL) ;

  DPRINT(")") ;
}

Error
_dxf_FreeTmeshCacheXGL (Pointer p)
{
  register tdmStripDataXGL *stripsXGL = p ;
  DPRINT1("\n(_dxf_FreeTmeshCacheXGL %x)", stripsXGL) ;

  if (stripsXGL)
    {
      register int i ;
      register int num = stripsXGL->num_strips ;
      if (stripsXGL->pt_lists)
	{
	  register Xgl_pt_list *pt_lists = stripsXGL->pt_lists ;
	  for (i=0 ; i<num ; i++)
	    {
	      if (pt_lists[i].pts.color_normal_f3d)
		{
		  tdmFree((Pointer)pt_lists[i].pts.color_normal_f3d) ;
		  pt_lists[i].pts.color_normal_f3d = 0 ;
		}
	      if (pt_lists[i].bbox)
		{
		  tdmFree((Pointer)pt_lists[i].bbox) ;
		  pt_lists[i].bbox = 0 ;
		}
	    }
	  tdmFree(stripsXGL->pt_lists) ;
	  stripsXGL->pt_lists = 0 ;
	}
      if (stripsXGL->facet_lists)
	{
	  register Xgl_facet_list *facet_lists = stripsXGL->facet_lists ;
	  for (i=0 ; i<num ; i++)
	    {
	      if (facet_lists[i].facets.color_normal_facets)
		{
		  tdmFree((Pointer)facet_lists[i].facets.color_normal_facets) ;
		  facet_lists[i].facets.color_normal_facets = 0 ;
		}
	    }
	  tdmFree(stripsXGL->facet_lists) ;
	  stripsXGL->facet_lists = 0 ;
	}
      tdmFree(stripsXGL) ;
    }
  return OK ;
}



char *
_dxf_GetPolyCacheXGL (char *fun, int mod, xfieldT *f)
{
  Private o ;
  char *priv, cache_id[128] ;
  DPRINT("\n(_dxf_GetPolyCacheXGL") ;

#ifdef USE_POLY_CACHE
  sprintf (cache_id, "%x-%s", f, fun) ;
  DPRINT1("\nusing %s for cache_id", cache_id) ;

  if (o = (Private) DXGetCacheEntry(cache_id, mod, 0))
    {
      priv = (char *) DXGetPrivateData(o) ;
      DXDelete((Pointer)o) ;

      DPRINT1("\nreturning %x)", priv) ;
      return priv ;
    }
#endif
  DPRINT("\nreturning 0)") ;
  return 0 ;
}

void
_dxf_PutPolyCacheXGL (char *fun, int mod, xfieldT *f, char *polysXGL)
{
  Private o ;
  DPRINT("\n(_dxf_PutPolyCacheXGL") ;

#ifdef USE_POLY_CACHE
  if (o = DXNewPrivate((Pointer)polysXGL, _dxf_FreePolyCacheXGL))
    {
      char cache_id[128] ;
      sprintf (cache_id, "%x-%s", f, fun) ;
      DPRINT1("\nusing %s for cache_id", cache_id) ;

      DXSetCacheEntry((Pointer)o, 0, cache_id, mod, 0) ;
    }
  else
#endif
      _dxf_FreePolyCacheXGL((Pointer)polysXGL) ;

  DPRINT(")") ;
}

Error
_dxf_FreePolyCacheXGL (Pointer p)
{
  char *polysXGL = p ;
  DPRINT1("\n(_dxf_FreePolygCacheXGL %x)", polysXGL) ;

  if (polysXGL) tdmFree(polysXGL) ;
  return OK ;
}



Xgl_pt_list *
_dxf_GetPointCacheXGL (char *fun, int mod, xfieldT *f)
{
  Private o ;
  Xgl_pt_list *priv ;
  char cache_id[128] ;
  DPRINT("\n(_dxf_GetPointCacheXGL") ;

#ifdef USE_POINT_CACHE
  sprintf (cache_id, "%x-%s", f, fun) ;
  DPRINT1("\nusing %s for cache_id", cache_id) ;

  if (o = (Private) DXGetCacheEntry(cache_id, mod, 0))
    {
      priv = (Xgl_pt_list *) DXGetPrivateData(o) ;
      DXDelete((Pointer)o) ;

      DPRINT1("\nreturning %x)", priv) ;
      return priv ;
    }
#endif

  DPRINT("\nreturning 0)") ;
  return 0 ;
}

void
_dxf_PutPointCacheXGL (char *fun, int mod, xfieldT *f, Xgl_pt_list *pt_list)
{
  Private o ;
  DPRINT("\n(_dxf_PutPointCacheXGL") ;

#ifdef USE_POINT_CACHE
  if (o = DXNewPrivate((Pointer)pt_list, _dxf_FreePointCacheXGL))
    {
      char cache_id[128] ;
      sprintf (cache_id, "%x-%s", f, fun) ;
      DPRINT1("\nusing %s for cache_id", cache_id) ;

      DXSetCacheEntry((Pointer)o, 0, cache_id, mod, 0) ;
    }
  else
#endif
      _dxf_FreePointCacheXGL((Pointer)pt_list) ;

  DPRINT(")") ;
}

Error
_dxf_FreePointCacheXGL (Pointer p)
{
  Xgl_pt_list *pt_list = (Xgl_pt_list *)p ;
  DPRINT1("\n(_dxf_FreePointCacheXGL %x)", pt_list) ;

  if (pt_list)
    {
      if (pt_list->pts.color_normal_f3d)
	{
	  tdmFree((Pointer)pt_list->pts.color_normal_f3d) ;
	  pt_list->pts.color_normal_f3d = 0 ;
	}
      if (pt_list->bbox)
	{
	  tdmFree((Pointer)pt_list->bbox) ;
	  pt_list->bbox = 0 ;
	}
      tdmFree((Pointer)pt_list) ;
    }
  return OK ;
}


char *
_dxf_GetLineCacheXGL (char *fun, int mod, xfieldT *f)
{
  Private o ;
  char *priv, cache_id[128] ;
  DPRINT("\n(_dxf_GetLineCacheXGL") ;

#ifdef USE_LINE_CACHE
  sprintf (cache_id, "%x-%s", f, fun) ;
  DPRINT1("\nusing %s for cache_id", cache_id) ;

  if (o = (Private) DXGetCacheEntry(cache_id, mod, 0))
    {
      priv = (char *) DXGetPrivateData(o) ;
      DXDelete((Pointer)o) ;

      DPRINT1("\nreturning %x)", priv) ;
      return priv ;
    }
#endif

  DPRINT("\nreturning 0)") ;
  return 0 ;
}

void
_dxf_PutLineCacheXGL (char *fun, int mod, xfieldT *f, char *linesXGL)
{
  Private o ;
  DPRINT("\n(_dxf_PutLineCacheXGL") ;

#ifdef USE_LINE_CACHE
  if (o = DXNewPrivate((Pointer)linesXGL, _dxf_FreeLineCacheXGL))
    {
      char cache_id[128] ;
      sprintf (cache_id, "%x-%s", f, fun) ;
      DPRINT1("\nusing %s for cache_id", cache_id) ;

      DXSetCacheEntry((Pointer)o, 0, cache_id, mod, 0) ;
    }
  else
#endif
      _dxf_FreeLineCacheXGL((Pointer)linesXGL) ;

  DPRINT(")") ;
}

Error
_dxf_FreeLineCacheXGL (Pointer p)
{
  char *linesXGL = p ;
  DPRINT1("\n(_dxf_FreeLineCacheXGL %x)", linesXGL) ;

  if (linesXGL) tdmFree(linesXGL) ;
  return OK ;
}
