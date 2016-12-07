/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*
 * $Source: /src/master/dx/src/exec/hwrender/hwPolyline.c,v $
 * $L:$
 */

#include <stdio.h>
#include "hwDeclarations.h"
#include "hwXfield.h"
#include "hwMemory.h"

#include "hwDebug.h"

typedef struct EdgeS {
  struct EdgeS *next;
  int	edgeIndex;
} EdgeT,*Edge;

#define INSERT_EDGE(edgeI,ptI) {					\
  Edge edge;								\
  if(nextFreeEdge == xf->nconnections*2) goto error;			\
  else edge = &edgePool[nextFreeEdge++];				\
  edge->edgeIndex = edgeI;						\
  edge->next = edges[ptI].next;						\
  edges[ptI].next = edge;						\
  }

#define DELETE_EDGE(edgeI,ptI)	{ \
  Edge tmp;			\
  tmp = &edges[ptI];		\
  while(tmp->next && tmp->next->edgeIndex != edgeI) {tmp = tmp->next;}	\
  if(tmp->next) {		\
    tmp->next = tmp->next->next;\
  }				\
  }

#define FIND_EDGE(ptI) (edges[ptI].next ? edges[ptI].next->edgeIndex : -1)

#define OTHER_PTI(line,ptI)  ((line->p == ptI) ? line->q : line->p)


#if defined(DEBUG)
static int	histogram[21];
#endif

Error
_dxf_linesToPlines (xfieldT *xf)
{
  int		i;
  Line		*line = NULL,lineScratch;
  int		newEdgeI;
  Edge		edges = NULL;
  int		*polyline = NULL;
  Array		array = NULL;
  int		ptI,lastPtI;
  int		plineOffset,plineIndex;
  Edge		edgePool = NULL;
  int		nextFreeEdge = 0;

  ENTRY(("_dxf_linesToPlines(0x%x)", xf));
  
  /* allocate permanent xfield pline data of appropriate size */
  /* Allocations are a guess based on 100 lines/polyline */
  if (!(xf->meshes = DXNewArray (TYPE_INT, CATEGORY_REAL, 1, 2)) ||
      !(DXAllocateArray (xf->meshes, xf->nconnections/100)) ||

      !(array = DXNewArray (TYPE_INT, CATEGORY_REAL, 0)) ||
      !(DXAllocateArray (array, xf->nconnections+xf->nconnections/100)) ||
      !(edgePool = (Edge)DXAllocate(sizeof(EdgeT)*xf->nconnections * 2)))
    {
      PRINT(("could not allocate xfield mesh data"));
      DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
    }

  xf->nmeshes = 0 ;

  /* get line connection info; lines will be replaced by strips */
  xf->origConnections_array = xf->connections_array ;
  xf->origNConnections = xf->nconnections;

  if(!(edges = (Edge)DXAllocateZero(xf->npositions * sizeof(EdgeT))))
    DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
  if (!(polyline = (int *)DXAllocate((xf->nconnections+1) * sizeof(Edge)))) 
    DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;

  for(i=0;i<xf->nconnections;i++)  {
    if(xf->invCntns && !DXIsElementValid(xf->invCntns,i)) continue;
    if(!(line = DXGetArrayEntry(xf->connections,i,&lineScratch)))
      goto error;
    INSERT_EDGE(i,line->p);
    INSERT_EDGE(i,line->q);
  }

  lastPtI = 0;
  plineOffset = 0;
  plineIndex = 0;

  while (1) {
    int newPline[2] ;

    for(ptI=lastPtI;ptI<xf->npositions;ptI++) 
      if((newEdgeI = FIND_EDGE(ptI)) >= 0) break;
    
    if(ptI == xf->npositions) break;
    lastPtI = ptI;
    
    while(newEdgeI >= 0) {
      if(!(line = DXGetArrayEntry(xf->connections,newEdgeI,&lineScratch)))
	goto error;
      polyline[plineIndex++] = ptI;
      DELETE_EDGE(newEdgeI,ptI);
      ptI = OTHER_PTI(line,ptI);
      DELETE_EDGE(newEdgeI,ptI);
      newEdgeI = FIND_EDGE(ptI);
      }
    polyline[plineIndex++] = ptI;

    newPline[0] = plineOffset;
    newPline[1] = plineIndex;
#if defined(DEBUG)
    if(plineIndex >= 20)
      histogram[10]++;
    else
      histogram[plineIndex/2]++;
#endif

    /* add the pline data to the xfield */
    DXAddArrayData (xf->meshes, xf->nmeshes, 1, newPline) ;
    DXAddArrayData (array, plineOffset,
		    plineIndex, polyline) ;

    xf->nmeshes++ ;
    plineOffset += plineIndex;
    plineIndex = 0;
  }

  /* Trim extra storage allocation from arrays */
  DXTrim(xf->meshes);
  DXTrim(array);

  xf->connections_array = array;
  xf->nconnections = plineOffset + plineIndex;
  array = NULL;

  /* free old array handle and create a new one */
  if (!(DXFreeArrayHandle(xf->connections)) ||
      !(xf->connections = DXCreateArrayHandle(xf->connections_array)))
    {
      PRINT(("could not create new connections array handle"));
      DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
    }

#if 0
  DXReference((dxObject)xf->connections_array);
  DXReference((dxObject)xf->meshes);

  /*  create a new array handleo */
  if(!(xf->connections = DXCreateArrayHandle(xf->connections_array)))
    {
      PRINT(("could not create new connections array handle"));
      DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
    }
#endif

  /* record connection info */
  xf->posPerConn = 1 ;
  xf->connectionType = ct_pline;

#if defined(DEBUG)
  for(i=0;i<11;i++)
    PRINT((" %3d",2*i));
  PRINT((" >"));
  for(i=0;i<11;i++) {
    PRINT((" %3d",histogram[i]));
    histogram[i] = 0;
  }
#endif

  DXFree(edgePool);
  DXFree(edges);
  DXFree(polyline);

  EXIT(("OK"));
  return OK ;

 error:

  if(edgePool) DXFree(edgePool);
  if(edges) DXFree(edges);
  if(polyline) DXFree(polyline);
  if(array) DXDelete((dxObject)array);
  if(xf->meshes) DXDelete((dxObject)xf->meshes);

  EXIT(("ERROR"));
  return 0 ;
}

Error
_dxf_polylinesToPlines (xfieldT *xf)
{
  int		i;
  int		start, end, knt;
  int		*mesh;

  ENTRY(("_dxf_linesToPlines(0x%x)", xf));
  
  /* allocate permanent xfield pline data of appropriate size */
  /* Allocations are a guess based on 100 lines/polyline */

  if (!(xf->meshes = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2)) ||
      !(DXAllocateArray (xf->meshes, xf->npolylines)))
    {
      PRINT(("could not allocate xfield mesh data"));
      DXErrorGoto(ERROR_NO_MEMORY, "#13000") ;
    }

  mesh = (int *)DXGetArrayData(xf->meshes);

  for (i = 0; i < xf->npolylines; i++)
  {
      start = xf->polylines[i];
      end   = (i+1 == xf->npolylines) ? xf->nedges : xf->polylines[i+1];
      knt   = end - start;

      if(xf->invCntns && !DXIsElementValid(xf->invCntns,i)) continue;

      *mesh++ = start;
      *mesh++ = knt;
  }

  xf->connections_array = (Array)DXReference((dxObject)xf->edges);
  xf->nconnections = xf->nedges;

  xf->nmeshes = xf->npolylines;

  /* record connection info */
  xf->posPerConn = 1 ;
  xf->connectionType = ct_pline;

  EXIT(("OK"));
  return OK ;

 error:

  if(xf->meshes) DXDelete((dxObject)xf->meshes);

  EXIT(("ERROR"));
  return 0 ;
}
