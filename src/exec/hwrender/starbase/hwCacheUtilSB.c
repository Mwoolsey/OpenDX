/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwCacheUtilSB.c,v $
  Author: Mark Hood

  Cache utilities for Starbase strip primitives.

\*---------------------------------------------------------------------------*/

#include "hwDeclarations.h"
#include "hwTmesh.h"
#include "hwMemory.h"
#include "hwCacheUtilSB.h"

#include "hwDebug.h"

/* performance tuning */

/*
 * NO CACHING!!!! never will clean up, and besides the cache
 * tag used (based on the address of the xfield structure) is
 * really dumb.
 */

#undef USE_TSTRIP_CACHE
#undef USE_POLY_CACHE


tdmStripArraySB *
tdmGetTmeshCacheSB (char *fun, xfieldT *xf)
{
  Private o ;
  char cache_id[128] ;
  tdmStripArraySB *stripsSB ;

  ENTRY(("tdmGetTmeshCacheSB (0x%x, 0x%x)", fun, xf));

#ifdef USE_TSTRIP_CACHE
  sprintf (cache_id, "%x-%s", xf, fun) ;
  PRINT(("using %s for cache id", cache_id));

  if (o = (Private) DXGetCacheEntry (cache_id, 1, 0))
    {
      stripsSB = (tdmStripArraySB *) DXGetPrivateData(o) ;
      DXDelete((Pointer)o) ;

      EXIT(("stripSB = %x", stripsSB));
      return stripsSB ;
    }
#endif

  EXIT(("0"));
  return 0 ;
}

void
tdmPutTmeshCacheSB (char *fun, xfieldT *xf, tdmStripArraySB *stripsSB)
{
  Private o ;

  ENTRY(("tdmPutTmeshCacheSB(\"%s\", 0x%x, 0x%x)", fun, xf, stripsSB));

#ifdef USE_TSTRIP_CACHE
  if (o = DXNewPrivate((Pointer)stripsSB, tdmFreeTmeshCacheSB))
    {
      char cache_id[128] ;
      sprintf (cache_id, "%x-%s", xf, fun) ;
      PRINT(("using %s for cache id", cache_id));

      DXSetCacheEntry((Pointer)o, 0, cache_id, 1, 0) ;
    }
  else
#endif
      tdmFreeTmeshCacheSB((Pointer)stripsSB) ;

  EXIT((""));
}

Error
tdmFreeTmeshCacheSB (Pointer p)
{
  register tdmStripArraySB *stripsSB = p ;

  ENTRY(("tdmFreeTmeshCacheSB(0x%x)", p));

  if (stripsSB)
    {
      register int i ;
      register int num = stripsSB->num_strips ;
      if (stripsSB->stripArray)
	{
	  register tdmTmeshCacheSB *array = stripsSB->stripArray ;
	  for (i=0 ; i<num ; i++)
	    {
	      if (array[i].clist)
		{
		  tdmFree(array[i].clist) ;
		  array[i].clist = 0 ;
		}
	      if (array[i].gnormals)
		{
		  tdmFree(array[i].gnormals) ;
		  array[i].gnormals = 0 ;
		}
	    }
	  tdmFree(stripsSB->stripArray) ;
	  stripsSB->stripArray = 0 ;
	}
      tdmFree(stripsSB) ;
    }

  EXIT((""));
  return OK ;
}

tdmPolyhedraCacheSB *
tdmGetPolyhedraCacheSB (char *fun, int pMod, xfieldT *xf)
{
  Private o ;
  char cache_id[128] ;
  tdmPolyhedraCacheSB *cache ;

  ENTRY(("tdmGetPolyhedraCacheSB(\"%s\", %d, 0x%x)", fun, pMod, xf));

#ifdef USE_POLY_CACHE
  sprintf (cache_id, "%x-%s", xf, fun) ;
  PRINT(("using %s for cache id", cache_id));

  if (o = (Private) DXGetCacheEntry(cache_id, pMod, 0))
    {
      cache = (tdmPolyhedraCacheSB *) DXGetPrivateData(o) ;
      DXDelete((Pointer)o) ;

      EXIT(("0x%x", cache));
      return cache ;
    }
#endif

  EXIT(("0"));
  return 0 ;
}


void
tdmPutPolyhedraCacheSB (char *fun, int pMod, xfieldT *xf,
			float clist[], int numverts, int numcoords,
			int vertex_flags, int ilist[], float flist[],
			int numfacetsets, int facet_flags)
{
  Private o ;
  char cache_id[128] ;
  tdmPolyhedraCacheSB *cache ;

  ENTRY(("tdmPutPolyhedraCacheSB(\"%s\", %d, 0x%x, 0x%x, %d, %d,"
	 " %d, 0x%x, 0x%x, %d, %d)",
	 fun, pMod, xf, clist, numverts, numcoords,
	 vertex_flags, ilist, flist, numfacetsets, facet_flags));

#ifdef USE_POLY_CACHE
  if (!(cache = (tdmPolyhedraCacheSB *)
	tdmAllocate(sizeof(tdmPolyhedraCacheSB))))
#endif
    {
      if (ilist) tdmFree(ilist) ;
      if (flist) tdmFree(flist) ;
      if (clist && vertex_flags) tdmFree(clist) ;

      EXIT((""));
      return ;
    }

  cache->clist = clist ;
  cache->numverts = numverts ;
  cache->numcoords = numcoords ;
  cache->vertex_flags = vertex_flags ;
  cache->ilist = ilist ;
  cache->flist = flist ;
  cache->numfacetsets = numfacetsets ;
  cache->facet_flags = facet_flags ;

  o = DXNewPrivate((Pointer)cache, tdmFreePolyhedraCacheSB) ;

  sprintf (cache_id, "%x-%s", xf, fun) ;
  PRINT(("using %s for cache id", cache_id));

  DXSetCacheEntry((Pointer)o, 0, cache_id, pMod, 0) ;
  EXIT((""));
  return;
}

Error
tdmFreePolyhedraCacheSB (Pointer p)
{
  tdmPolyhedraCacheSB *cache = p ;

  ENTRY(("tdmFreePolyhedraCacheSB(0x%x)", p));

  if (cache)
    {
      if (cache->ilist) tdmFree(cache->ilist) ;
      if (cache->flist) tdmFree(cache->flist) ;
      if (cache->clist && cache->vertex_flags) tdmFree(cache->clist) ;
      tdmFree(cache) ;
    }

  EXIT((""));
  return OK ;
}
