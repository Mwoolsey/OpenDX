/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/starbase/hwBoundingBoxDrawSB.c,v $
  Author:
\*---------------------------------------------------------------------------*/


#include <starbase.c.h>
#include "hwDeclarations.h"
#include "hwPortSB.h"
#include "hwMemory.h"
#include "hwXfield.h"

#include "hwDebug.h"

int
_dxfBoundingBoxDraw (tdmPortHandleP portHandle, xfieldP xf, int clip_status)
{
  DEFPORT(portHandle);
  dxObject retobj = NULL ;
  Point *boxpts ;
  int i ;
  int faceverts[] =
    {
      0, 1, 3, 2, 0,       /* one face */
      4, 5, 7, 6, 4        /* opposite face */
    } ;
  register float *clist = 0;
  int vertex_flags;
  int num_coords;
  register int vsize, dV ;
  int cOffs, nOffs, oOffs ;
  int k;
  
  ENTRY(("_dxfBoundingBoxDraw(0x%x, 0x%x, %d)", portHandle, xf, clip_status));

  if (clip_status)
    {
      EXIT(("clipping bounding boxes not implemented"));
      return 0 ;
    }
  
  vertex_flags = 0 ;
  num_coords = 3;
  vsize = 6;
  cOffs = 3;
  vertex_format(FILDES, num_coords, 3, 1, False, 0);
  
  /* allocate coordinate list */
  clist = (float *)tdmAllocateLocal(2 * vsize * sizeof(float));

  /* Set colors to white */
  clist[3]  = 1.0;
  clist[4]  = 1.0;
  clist[5]  = 1.0;
  clist[9]  = 1.0;
  clist[10] = 1.0;
  clist[11] = 1.0;
  
  boxpts = xf->box ;
  for (k = 0; k < 9; k++)
  {
      /* copy vertex coordinates into clist */
      for (i=0, dV=0; i<2 ; i++, dV+=vsize)
	  *(Point *)(clist+dV) = boxpts[faceverts[k + i]];

      polyline3d(FILDES, clist, 2, False);
  }
  for (k = 1; k < 4; k++)
  {
      /* copy vertex coordinates into clist */
      *(Point *)(clist)       = boxpts[k];
      *(Point *)(clist+vsize) = boxpts[k + 4];

      polyline3d (FILDES, clist, 2, False);
  }
  tdmFree(clist) ;
  vertex_format(FILDES, 0, 0, 0, False, 1);

  EXIT((""));
  return 1 ;
}
