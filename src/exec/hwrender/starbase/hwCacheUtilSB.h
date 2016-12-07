/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef hwCacheUtilSB_h
#define hwCacheUtilSB_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwCacheUtilSB.h,v $
  Author: Mark Hood

  Data structures and prototypes for caching Starbase primitives.

\*---------------------------------------------------------------------------*/

#include "hwXfield.h"
#include "hwTmesh.h"

typedef struct
{
  float *clist ;
  int numverts ;
  float *gnormals ;
  int numcoords ;
  int vertex_flags ;
  int facet_flags ;
} tdmTmeshCacheSB ;

typedef struct
{
  int num_strips ;
  tdmTmeshCacheSB *stripArray ;
} tdmStripArraySB ;

typedef struct
{
  float *clist ;
  int numverts ;
  int numcoords ;
  int vertex_flags ;
  int *ilist ;
  float *flist ;
  int numfacetsets ;
  int facet_flags ;
} tdmPolyhedraCacheSB ;

tdmStripArraySB *
tdmGetTmeshCacheSB (char *fun, xfieldT *xf) ;

tdmPolyhedraCacheSB *
tdmGetPolyhedraCacheSB (char *fun, int pMod, xfieldT *xf) ;

void
tdmPutTmeshCacheSB (char *fun, xfieldT *xf, tdmStripArraySB *stripsSB) ;

void
tdmPutPolyhedraCacheSB (char *fun, int pMod, xfieldT *xf,
			float clist[], int numverts, int numcoords,
			int vertex_flags, int ilist[], float flist[],
			int numfacetsets, int facet_flags) ;

Error
tdmFreeTmeshCacheSB (Pointer cache) ;

Error
tdmFreePolyhedraCacheSB (Pointer cache) ;



#endif /* hwCacheUtilSB_h */
