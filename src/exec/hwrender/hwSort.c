/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include "hwDeclarations.h"
#include "hwXfield.h"
#include "hwSort.h"

SortList
_dxf_newSortList()
{
    SortListP sl = DXAllocate(sizeof(struct sortList));
    if (sl)
    {
	sl->sortBlockList = NULL;
	sl->sortList = NULL;
	sl->itemCount = 0;
	sl->sortListLength = 0;
    }

    return (SortList)sl;
}

void
_dxf_clearSortList(SortList sl)
{
    SortListP slp = (SortListP)sl;
    SortBlock tmp = slp->sortBlockList;

    while (tmp)
    {
	SortBlock bye = tmp;
	tmp = tmp->next;
	DXFree(bye);
    }

    slp->sortBlockList = NULL;
    slp->itemCount = 0;
}

void _dxf_deleteSortList(SortList sl)
{
    SortListP slp = (SortListP)sl;
    _dxf_clearSortList(sl);
    if (slp->sortList)
	DXFree((Pointer)slp->sortList);
    DXFree((Pointer)sl);
}

int
_dxf_Insert_Translucent(SortList sl, void *xfp, int i)
{
    SortListP slp = (SortListP)sl;
    Sort next;

    if (!slp->sortBlockList || slp->sortBlockList->numUsed == DXHW_TLIST_BLOCKSIZE)
    {
	SortBlock tmp = DXAllocate(sizeof(struct sortBlock));
	if (! tmp)
	    return 0;
	
	tmp->next = slp->sortBlockList;
	tmp->numUsed = 0;
	slp->sortBlockList = tmp;
    }

    next = slp->sortBlockList->list + slp->sortBlockList->numUsed++;
    next->xfp = xfp;
    next->poly = i;
    
    slp->itemCount ++;

    return 1;
}

#define LT(a,b)         ((a)->depth < (b)->depth)
#define GT(a,b)         ((a)->depth > (b)->depth)
#define TYPE            struct sortD
#define QUICKSORT       _dxf_DepthSort
#define QUICKSORT_LOCAL _dxf_DepthSortLocal
#include "../libdx/qsort.c"
#undef LT
#undef GT
#undef TYPE
#undef QUICKSORT
#undef QUICKSORT_LOCAL

#define Apply_Z(poi, mat, res) \
  res = mat[0][2]*poi[0] + mat[1][2]*poi[1] + mat[2][2]*poi[2] + mat[3][2];

int 
_dxf_Sort_Translucent(SortList sl)
{
    SortListP slp = (SortListP)sl;
    int       j;
    SortBlock sbp;
    SortD     sdp;
    Sort      sp;

    if (slp->itemCount <= 0)
	return OK;

    if (!slp->sortList || slp->sortListLength < slp->itemCount)
    {
	if (slp->sortList)
	    DXFree((Pointer)slp->sortList);
	
	slp->sortList = DXAllocate(slp->itemCount * sizeof(struct sortD));
	if (! slp->sortList)
	    return ERROR;
	
	slp->sortListLength = slp->itemCount;
    }

    for (sdp = slp->sortList, sbp = slp->sortBlockList; sbp; sbp = sbp->next)
    {
        for (j = 0, sp = sbp->list; j < sbp->numUsed; j++, sdp++, sp++)
	{
	    int     cs[8];
	    int     *conn;
	    float   *vertex, center[3];
	    float   *vs[3];
	    int     k;
	    xfieldT *xf = (xfieldT *)sp->xfp;
	   
	    if (xf->connections)
	    {
		conn = (int *)DXGetArrayEntry(xf->connections,
				sp->poly, (Pointer)cs);
	    
		center[0] = center[1] = center[2] = 0.0;
		for (k = 0; k < xf->posPerConn; k++)
		{
		    vertex = (float *)DXGetArrayEntry(xf->positions,
					    conn[k], (Pointer)vs);
		    center[0] += vertex[0];
		    center[1] += vertex[1];
		    center[2] += vertex[2];
		}

		switch(xf->posPerConn)
		{
		    case 0:
			DXSetError(ERROR_INTERNAL, "zero posPerConn??");
			return ERROR;

		    case 1:
			break;

		    case 2:
			center[0] *= 0.5;
			center[1] *= 0.5;
			center[2] *= 0.5;
			break;

		    case 3:
			center[0] *= 0.33333333;
			center[1] *= 0.33333333;
			center[2] *= 0.33333333;
			break;
		    
		    case 4:
			center[0] *= 0.25;
			center[1] *= 0.25;
			center[2] *= 0.25;
			break;
		    
		    default:
			{
			    int   l;
			    float denom = 1.0 / xf->posPerConn;
			    for (l = 0; l < xf->posPerConn; l++)
			    {
				center[0] *= denom;
				center[1] *= denom;
				center[2] *= denom;
			    }
			}
			break;
		}

		Apply_Z(center, xf->attributes.vm, sdp->depth);
	    }
	    else
	    {
		vertex = (float *)DXGetArrayEntry(xf->positions,
		                                     sp->poly, (Pointer)vs);

		Apply_Z(vertex, xf->attributes.vm, sdp->depth);
	    }

	    sdp->xfp   = sp->xfp;
	    sdp->poly  = sp->poly;
	}
    }

    _dxf_DepthSort(slp->sortList, slp->itemCount);

    return OK;
}
