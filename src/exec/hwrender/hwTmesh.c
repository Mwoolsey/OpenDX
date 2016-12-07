/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Source: /src/master/dx/src/exec/hwrender/hwTmesh.c,v $
 */

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <stdio.h>
#include "hwDeclarations.h"
#include "hwXfield.h"
#include "hwTmesh.h"
#include "hwMemory.h"
#include "hwWindow.h"

#include "hwDebug.h"

extern dxObject _dxf_QueryObject(HashTable, dxObject);
extern void _dxf_InsertObject(HashTable, dxObject, dxObject);

typedef struct _Triple
{
  int p[3] ;
} Triple ;


/* add point i in triangle tri to the strip */
#define AddPoint(tri,i) (point[nPtsInStrip++] = triangles[tri].p[i])


/*
 *  Make a "swap marker" in the strip telling where to swap.  The swap
 *  marker needs to be right before the last vertex we added.
 *   so ... (5,4,3,2,1,?,...) => (5,4,3,2,-1,1,...)
 */
#define DoSwap()                                                     \
  {                                                                  \
  point[nPtsInStrip] = point[nPtsInStrip-1];                         \
  point[nPtsInStrip-1] = -1;                                         \
  nPtsInStrip++;                                                     \
  }                                                                  \


/*
 *  Make a "swap marker" in the strip telling where to swap, and then swap
 *  the 2 previous numbers.
 *
 *   (5,4,3,2,1,?,...) => (5,4,-1,2,3,1,...)
 *
 *  This is assuming all the numbers below are numbers added during the
 *  reverse search...  if they include normal numbers it should be like
 *  this... (someday)
 *
 *   (f1,f2,f3,f4,r2,r1)     f4 f3 f2 -1  r2 f1 r1
 *     3  4  5  6  2  1   =>  6  5  4  -1  2  3  1
 *
 *  ...because r2 and f1 are really going to be adjacent after the
 *  reversed numbers are switched around.  Someday we should handle the
 *  cases of rev_pts = 0, 1, or 2.  (see RevTryToSwap())
 */
#define RevDoSwap()                                                  \
  {                                                                  \
  point[nPtsInStrip  ] = point[nPtsInStrip-1];                       \
  point[nPtsInStrip-1] = point[nPtsInStrip-3];                       \
  point[nPtsInStrip-3] = -1;                                         \
  nPtsInStrip++;                                                     \
  }                                                                  \


/* return the triangle index that refers to point pt in triangle tri */
#define LookUp(tri,pt)                 \
  (                                    \
  triangles[tri].p[0] == pt ? 0 :      \
  triangles[tri].p[1] == pt ? 1 :      \
  triangles[tri].p[2] == pt ? 2 : -1   \
  )


/*
 *  Return the index not present of the set [0,1,2] so that
 *  other_index[0][1] = 2, and other_index[2][1] = 0, etc.
 */
static int other_index[3][3] =
  { {-1, 2, 1},
    { 2,-1, 0},
    { 1, 0,-1} };


#if defined(DXD_HW_TMESH_SWAP_OK)
/*
 *  If the hardware allows orientation swapping in strip, we have more
 *  flexibility in choosing directions to propagate the strip.
 */
#define TryToSwap(try) tri = neighbors[prev_tri].p[prev_i1];
/*
 *  We can't do the swap until we have a few reverse points... else we
 *  would need to swap the beginning of the forward points, not the end.
 *  Maybe someday... (see RevDoSwap()).
 */
#define RevTryToSwap(try)                                                  \
  {                                                                        \
  if (rev_pts >= 3)                        /* had a few reverse points? */ \
    tri = neighbors[prev_tri].p[prev_i1];  /*   proceed with the swap   */ \
  else                                     /* very few reverse points?  */ \
    tri = -1;                              /*   don't let it swap       */ \
  }
#else
#define TryToSwap(try) tri = -1;
#define RevTryToSwap(try) tri = -1;
#endif /* defined(DXD_HW_TMESH_SWAP_OK) */


#define UsedTri(n) (usedArray[Byte(n)] & Mask(n))
#define ValidTri(ich, n) (ich ? DXIsElementValid(ich, n) : 1)
#define GoodTri(n) (n != -1 && !UsedTri(n) && ValidTri(xf->invCntns, n))


/* jump to neighbor across from i0 or across from i1 */
#define NextTri(tri)                                                         \
  {                                                                          \
  /* the "MaxTstripSize-1" is because there might be a swap coming up too */ \
  if (nPtsInStrip >= MaxTstripSize-1)       /* if strip is too long...    */ \
    tri = -1;                               /*    ...chop it off          */ \
  else                                                                       \
    {                                                                        \
    prev_tri = tri;                                                          \
    prev_i0 = i0;                                                            \
    prev_i1 = i1;                                                            \
    prev_i2 = i2;                                                            \
    tri = neighbors[prev_tri].p[prev_i0]; /* jump to adjacent triangle  */   \
    if (GoodTri(tri))                     /* valid and not used?        */   \
      {                                   /* adjust indices to new tri  */   \
      i0 = LookUp(tri,triangles[prev_tri].p[prev_i1]);                       \
      i1 = LookUp(tri,triangles[prev_tri].p[prev_i2]);                       \
      i2 = other_index[i0][i1];                                              \
      }                                                                      \
    else /* can't jump to next normal triangle... maybe we can swap? */      \
      {                                                                      \
      TryToSwap(try); /* jump to adj tri with swap */                        \
      if (GoodTri(tri))                     /* valid and not used?        */ \
        {                                                                    \
        DoSwap(); /*  put mark (-1) in point array to indicate a swap */     \
        i0 = LookUp(tri,triangles[prev_tri].p[prev_i0]);                     \
        i1 = LookUp(tri,triangles[prev_tri].p[prev_i2]);                     \
        i2 = other_index[i0][i1];                                            \
        }                                                                    \
      else                                                                   \
        tri = -1;                           /* dead end...end the strip */   \
      }                                                                      \
    }                                                                        \
  }


/*
 *  If the hardware does not require that the initial triangles in all the
 *  strips are oriented consistently, then we can try to lengthen the
 *  strip by working in the opposite direction from the initial triangle.
 *  The new initial triangle may have a different orientation from the
 *  original.
 */
#define RevNextTri(tri)                                                      \
  {                                                                          \
  /* the "MaxTstripSize-1" is because there might be a swap coming up too */ \
  if (nPtsInStrip >= MaxTstripSize-1)       /* if strip is too long...    */ \
    tri = -1;                               /*    ...chop it off          */ \
  else                                                                       \
    {                                                                        \
    prev_tri = tri;                                                          \
    prev_i0 = i0;                                                            \
    prev_i1 = i1;                                                            \
    prev_i2 = i2;                                                            \
    tri = neighbors[prev_tri].p[prev_i2]; /* jump BACK to PREV triangle  */  \
    if (GoodTri(tri))                     /* valid and not used?        */   \
      {                                   /* adjust indices to new tri  */   \
      i2 = LookUp(tri,triangles[prev_tri].p[prev_i1]);                       \
      i1 = LookUp(tri,triangles[prev_tri].p[prev_i0]);                       \
      i0 = other_index[i1][i2];                                              \
      }                                                                      \
    else /* can't jump to next normal triangle... maybe we can swap? */      \
      {                                                                      \
      RevTryToSwap(try); /* jump to adj tri with swap */                     \
      if (GoodTri(tri))                     /* valid and not used?        */ \
        {                                                                    \
        RevDoSwap(); /* put mark (-1) in point array to indicate a swap */   \
        i2 = LookUp(tri,triangles[prev_tri].p[prev_i2]);                     \
        i1 = LookUp(tri,triangles[prev_tri].p[prev_i0]);                     \
        i0 = other_index[i1][i2];                                            \
        }                                                                    \
      else                                                                   \
        tri = -1;                           /* dead end...end the strip */   \
      }                                                                      \
    }                                                                        \
  }


#define Bytes(ntri) (int)(ntri/8+1)
#define Byte(tri) (int)(tri/8)
#define Mask(tri) (unsigned char)(1<<(tri%8))
#define MarkTri(tri) usedArray[Byte(tri)] |= Mask(tri)


/*
 *  Choose the indices for the 1st triangle in a strip.  Look ahead 2
 *  triangles (the 1st 3 if's) to try to force a 3-triangle strip.  If
 *  this fails, then try to get at least a 2-triangle strip (the next 2
 *  if's).
 *
 *  This macro was revised in version 5.0.2.3 to always produce a
 *  clockwise orientation of vertices for the initial triangle, since this
 *  was required by the Sun XGL graphics programming interface.  This
 *  seems to be desirable for all architectures since it allows hardware
 *  to compute consistent normals when producing flat shading from fields
 *  with position-dependent normals.
 *
 *  With the demise of the "flat shade" rendering approximation it may be
 *  more efficient to add an architecture dependency to this macro, since
 *  that would allow for more flexibility in choosing the initial triangle
 *  indices that will produce the longest strip.
 */
#define InitFirstTri(i0,i1,i2)                                                \
{                                                                             \
  if (GoodTri(          neighbors[tri].p[2]      ) &&                         \
      GoodTri(neighbors[neighbors[tri].p[2]].p[LookUp(neighbors[tri].p[2],    \
						      triangles[tri].p[0])])) \
    {i0 = 2; i1 = 0; i2 = 1;}                                                 \
  else                                                                        \
  if (GoodTri(          neighbors[tri].p[1]      ) &&                         \
      GoodTri(neighbors[neighbors[tri].p[1]].p[LookUp(neighbors[tri].p[1],    \
						      triangles[tri].p[2])])) \
    {i0 = 1; i1 = 2; i2 = 0;}                                                 \
  else                                                                        \
  if (GoodTri(          neighbors[tri].p[0]      ) &&                         \
      GoodTri(neighbors[neighbors[tri].p[0]].p[LookUp(neighbors[tri].p[0],    \
						      triangles[tri].p[1])])) \
    {i0 = 0; i1 = 1; i2 = 2;}                                                 \
  else                                                                        \
  if (GoodTri(neighbors[tri].p[2]))                                           \
    {i0 = 2; i1 = 0; i2 = 1;}                                                 \
  else                                                                        \
  if (GoodTri(neighbors[tri].p[1]))                                           \
    {i0 = 1; i1 = 2; i2 = 0;}                                                 \
  else                                                                        \
    {i0 = 0; i1 = 1; i2 = 2;}                                                 \
}


/************************************************************
            prev_i0      prev_i2
                           i1                      triangles = 0 1 6, 5 6 1,...
               6-----------5-----------4           strip = 0,6,1,5,2...
              / \ prev_tri/ \         / \
             /   \       /   \       /   \
            /     \     /     \     /     \
           /       \   /  tri  \   /       \
          /         \ /         \ /         \
         0-----------1-----------2-----------3
                     i0          i2
                   prev_i1
*************************************************************/


static Triple
*getNeighbors (xfieldT *xf)
{
  /*
   *  Gets an array giving all the neighbors of a given triangle.
   *  neighbor[tri].p[n] is the triangle adjacent to triangle tri, across
   *  from triangle tri's nth point.  A -1 indicates no triangle is
   *  adjacent across from the nth point.
   */

  ENTRY(("getNeighbors(0x%x)", xf));

  if (!xf->neighbors_array)
    {
      EXIT(("couldn't get neighbors from field"));
      DXErrorReturn (ERROR_INTERNAL, "#13870") ;
    }

  if (!DXTypeCheck (xf->neighbors_array, TYPE_INT, CATEGORY_REAL, 1, 3))
    {
      EXIT(("neighbors array has bad type"));
      DXErrorReturn (ERROR_INTERNAL, "#13870") ;
    }

  EXIT((""));
  return (Triple *) DXGetArrayData(xf->neighbors_array) ;
}


#define FreeTempStrips()						     \
{									     \
  for (i = 0 ; i < nStrips ; i++)					     \
      if (stripArray[i].point)						     \
	  tdmFree((Pointer) stripArray[i].point) ;			     \
									     \
  tdmFree((Pointer) stripArray) ;					     \
}

typedef struct
{
   int *meshes;
   int nmeshes;
   int *connections;
   int nconnections;
   Array meshArray;
   Array connectionArray;
} TMeshS, *TMesh;

static Error
_deleteTMesh(void *p)
{
    TMesh t = (TMesh)p;
    if (t->connectionArray)
	DXDelete((dxObject)t->connectionArray);
    if (t->meshArray)
	DXDelete((dxObject)t->meshArray);
    
    DXFree(p);
    
    return OK;
}

static Error 
_newTMesh(int nStrips, int nStrippedPts, TMesh *mesh, dxObject *o)
{
    Private priv = NULL;
    TMesh tmp = NULL;

    *mesh = NULL;
    *o = NULL;

    tmp = (TMesh)DXAllocateZero(sizeof(TMeshS));
    if (! tmp)
        return ERROR;
    
    tmp->meshArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    if (! tmp->meshArray)
	goto error;
    
    DXReference((dxObject)tmp->meshArray);
    
    if ( !DXAllocateArray (tmp->meshArray, nStrips))
	goto error;
    
    tmp->connectionArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 0);
    if (! tmp->connectionArray)
	goto error;

    DXReference((dxObject)tmp->connectionArray);
    
    if ( !DXAllocateArray (tmp->connectionArray, nStrippedPts))
	goto error;
    
    tmp->meshes       = (int *)DXGetArrayData(tmp->meshArray);
    tmp->connections  = (int *)DXGetArrayData(tmp->connectionArray);
    tmp->nmeshes      = nStrips;
    tmp->nconnections = nStrippedPts;

    priv = (Private)DXNewPrivate((Pointer)tmp, _deleteTMesh);
    if (! priv)
       goto error;

    *mesh = tmp;
    *o = (dxObject)priv;

    return OK;

error:
    if (tmp)
    {
	if (tmp->connectionArray)
	    DXDelete((dxObject)tmp->connectionArray);
	if (tmp->meshArray)
	    DXDelete((dxObject)tmp->meshArray);
	
	DXFree((Pointer)tmp);
    }

    return ERROR;
}

static void
_installTMeshInfo(xfieldT *xf, TMesh tmesh)
{
    if (xf->connections)
        DXFreeArrayHandle(xf->connections);
    
    if (xf->connections_array)
	DXDelete((dxObject)xf->connections_array);
    
    xf->connections       = NULL;
    xf->connections_array = (Array)DXReference((dxObject)tmesh->connectionArray);
    xf->nconnections 	  = tmesh->nconnections;
    xf->meshes 		  = (Array)DXReference((dxObject)tmesh->meshArray);
    xf->nmeshes 	  = tmesh->nmeshes;
    xf->posPerConn 	  = 1;
    xf->connectionType 	  = ct_tmesh;
}


Error
_dxf_trisToTmesh (xfieldT *xf, tdmChildGlobalP globals)
{
  DEFGLOBALDATA(globals) ;
  Triple *neighbors ;          /* neighbors array gives a tri's neighbors   */
  Tstrip *stripArray = 0 ;     /* temp array of strips                      */
  int point[MaxTstripSize] ;   /* temp array in which to build point list   */
  char *usedArray = 0 ;        /* temp array to mark triangles already used */
  int nStrips = 0 ;            /* number of strips generated                */
  int nStrippedPts = 0 ;       /* number of points stripped                 */
  int nPtsInStrip ;            /* number of points in current strip         */
  int start_tri ;              /* first triangle of the strip               */
  int tri ;                    /* the current triangle                      */
  int i0, i1, i2 ;             /* the 3 indexes into triangles[tri].p[?]    */
  int init_tri, init_i0,       /* initial triangle and index configuration  */
      init_i1, init_i2 ;
  int prev_tri, prev_i0,       /* used to determine the same current info   */
      prev_i1, prev_i2 ;
  int rev_start ;              /* index of first of the reversed point list */
  int rev_pts ;                /* keeps count of how many rev points        */
                               /* we've collected to avoid swapping         */
                               /* reverse stage... see RevDoSwap()          */
  Triple *triangles ;          /* array of original triangle connections    */
  int ntriangles ;             /* number of original triangle connections   */
  int i, j, k, p, tp ;         /* misc. loop indices */
  TMesh tmesh = NULL;
  dxObject tmesho = NULL;
  char * tmesh_or_sens;
  int orsens = 0;

  ENTRY(("_dxf_trisToTmesh(0x%x)", xf));
  tmesh_or_sens = (char *)getenv("DX_HW_TMESH_ORIENT_SENSITIVE");
  if(tmesh_or_sens) {
      sscanf(tmesh_or_sens, "%d", &orsens);
  }

  tmesho = (dxObject)_dxf_QueryObject(MESHHASH, (dxObject)xf->field);
  if (tmesho)
  {
      xf->meshObject = DXReference(tmesho);
      tmesh = (TMesh)DXGetPrivateData((Private)tmesho);
      _installTMeshInfo(xf, tmesh);

      return OK;
  }

  if (xf->connectionType != ct_triangles)
    {
      EXIT(("xf->connectionType not ct_triangles"));
      return OK ;
    }

PRINT(("invalid connections: %d",
       xf->invCntns? DXGetInvalidCount(xf->invCntns): 0));

  /* get triangle connection info */
  triangles  = DXGetArrayData(xf->connections_array) ;
  ntriangles = xf->nconnections ;
  /*npoints    = xf->npositions ;*/

  xf->origNConnections = ntriangles;
  xf->origConnections_array = xf->connections_array;
  xf->connections_array = NULL;

  /* get array containing connection neighbors */
  if (!(neighbors = getNeighbors(xf)))
    {
      PRINT(("could not obtain mesh neighbors"));
      DXErrorGoto (ERROR_INTERNAL, "#13870") ;
    }

  /* allocate temporary data structures of maximum possible size */
  if (!(usedArray = tdmAllocateZero(sizeof(char)*Bytes(ntriangles))) ||
      !(stripArray = (Tstrip *) tdmAllocate(sizeof(Tstrip)*ntriangles)))
    {
      PRINT(("out of memory"));
      DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
    }

  /* strip the field, placing strip info into temporary arrays */
  for (start_tri = 0 ; start_tri < ntriangles ; start_tri++)
    {
      if (GoodTri(start_tri))
	{
	  nPtsInStrip = 0;
	  tri = start_tri;
	  InitFirstTri(i0,i1,i2);  /* pick initial vertices for 1st triangle  */
	  init_tri = tri;          /* remember where we started               */
	  init_i0 = i0; init_i1 = i1; init_i2 = i2;
	  AddPoint(tri,i0);        /* add the 1st two...                      */
	  AddPoint(tri,i1);        /* ...points to the mesh                   */
	  while (tri >= 0)         /* while there's a valid triangle to add...*/
	    {
	      AddPoint(tri,i2);    /*      add another triangle to the strip  */
	      MarkTri(tri);        /*      mark it as used                    */
	      NextTri(tri);        /*      move on to next triangle           */
	    }

	  /* now go the other way out of the initial triangle... */
	  tri = init_tri;          /* go back to initial triangle             */
	  i0 = init_i0; i1 = init_i1; i2 = init_i2;
	  rev_start = nPtsInStrip ;
	  rev_pts = 0;             /* keep track of pts we collect backwards  */
          if(orsens)
              tri = -1;
          else
	  {
              RevNextTri(tri);         /* move BACK to PREV triangle          */
          }
              
	  while (tri >= 0)         /* while there's a valid triangle to add...*/
	    {
	      AddPoint(tri,i0);    /*      add another triangle to the strip  */
	      rev_pts++;           /*      another backward point             */
	      MarkTri(tri);        /*      mark it as used                    */
              if(orsens)
                  tri = -1;
              else {
	          RevNextTri(tri);     /*      move BACK to PREV triangle    */
              }
	    }

	  /* allocate temporary points of appropriate size */
	  if (!(stripArray[nStrips].point =
		(int*) tdmAllocate(nPtsInStrip * sizeof(int))))
	    {
	      PRINT(("out of memory"));
	      DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
	    }

	  /* copy and reverse reversed points from "point" to "tstrip->point" */
	  for (tp=nPtsInStrip-1, p=0 ; tp>=rev_start ; tp--, p++)
	      stripArray[nStrips].point[p] = point[tp] ;

	  /* copy forward points from "point" to "tstrip->point" */
	  bcopy (point, &(stripArray[nStrips].point[p]),
		 (nPtsInStrip-p) * sizeof(int)) ;

	  /* record tstrip info */
	  stripArray[nStrips].points = nPtsInStrip ;
	  nStrips++ ;
	  nStrippedPts += nPtsInStrip ;
	}
    }

  PRINT(("stripped %d triangles", nStrippedPts - 2*nStrips));
  PRINT(("generated %d strips", nStrips));
  PRINT(("average number of triangles per strip: %f",
	   nStrips? (nStrippedPts - 2*nStrips)/(float)nStrips: 0));

  if (! _newTMesh(nStrips, nStrippedPts, &tmesh, &tmesho))
      goto error;

  for (i=j=k=0 ; i<nStrips ; i++)
    {
      /* record start index and number of points in strip */

      tmesh->meshes[j++] = k;
      tmesh->meshes[j++] = stripArray[i].points ;
      memcpy(tmesh->connections+k, stripArray[i].point, stripArray[i].points*sizeof(int));
      k += stripArray[i].points;
    }

  _dxf_InsertObject(MESHHASH, (dxObject)xf->field, (dxObject)tmesho);
  xf->meshObject = DXReference(tmesho);
  _installTMeshInfo(xf, tmesh);

  /* free temporary arrays */
  tdmFree((Pointer)usedArray) ;
  FreeTempStrips() ;

  EXIT(("OK"));
  return OK ;

error:
  if (tmesho)
      DXDelete(tmesho);
  if (usedArray)
      tdmFree((Pointer)usedArray);
  if (stripArray)
      FreeTempStrips() ;

  EXIT(("ERROR"));
  return 0 ;
}
