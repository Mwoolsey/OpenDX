/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#include <stdio.h>
#include <math.h>
#include <string.h>
#include <dx/dx.h>
#include "binSort.h"

typedef struct _gridlevel	*GridLevel;
typedef struct _item    	*Item;


Grid
_dxfMakeSearchGrid(float *max, float *min, int nPrims, int nDim)
{
    int 	i, j, k;
    float	span[MAXDIM], len, vol;
    int         counts[MAXDIM];
    Grid	grid;

    grid = (Grid)DXAllocate(sizeof(struct grid));
    if (! grid)
	goto error;

    /*
     * A few error conditions
     */
    if (nDim <= 0 || nDim > MAXDIM || nPrims == 0)
	goto error;

    grid->nDim = nDim;

    /*
     * Determine the "volume" of the bounding box to get a 
     * initial grid approximately the size of a primitive
     * From the average primitive "volume" guess an edge length
     * Note: add a bit to span so that we include the upper boundary.
     */
    vol = 1.0;
    for (i = 0; i < nDim; i++)
    {
	span[i] = 1.0001 * (max[i] - min[i]);
	vol *= span[i];
    }

    vol /= nPrims;
    len = (float)pow((double)vol, (double)(1.0/nDim));

    /*
     * Initial counts appropriate for grid[0].  Power of 2
     */
    for (i = 0; i < nDim; i++)
    {
	j = (span[i] / len) + 1;
	for (k = 1; k < j; k <<= 1);
	grid->counts[i] = counts[i] = k;
    }

    /*
     * _dxfInitialize grid levels.  As the grid level rises, the coarseness of
     * the grid increases: if the i-th level contains n bins along an axis,
     * the (i+1)-th will contain (n/2)+1.  Except the topmost, which contains
     * only one.
     */
    grid->itemArray   = NULL;
    for (i = 0; i < MAXGRID; i++)
    {
	grid->gridlevels[i].bucketArray = NULL;
	grid->gridlevels[i].current     = -1;
    }

    for (j = 0; j < nDim; j++)
    {
	grid->gridlevels[0].counts[j] = counts[j];
	grid->gridlevels[MAXGRID-1].counts[j] = 1;
    }

    for (i = 1; i < (MAXGRID - 1); i++)
	for (j = 0; j < nDim; j++)
	    grid->gridlevels[i].counts[j] = 
			(grid->gridlevels[i-1].counts[j] >> 1) + 1;


    /*
     * offset and scale for input points -> grid
     *		index = (input - min) * scale 
     * Include a fudge factor ensuring boundary conditions
     */
    for (i = 0; i < nDim; i++)
    {
	grid->min[i] = min[i];
	grid->scale[i] = 1.0 / span[i];
    }

    /*
     * Create allocation structure for bucket elements
     */
    grid->itemArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 2);
    if (! grid->itemArray)
	goto error;

    DXReference((Object)grid->itemArray);

    if (! DXAddArrayData(grid->itemArray, 0, nPrims, NULL))
	goto error;
	
    grid->items = (struct _item *)DXGetArrayData(grid->itemArray);

    grid->next = 0;

    return grid;

error:
    if (grid)
    {
	for (i = 0; i < MAXGRID; i++)
	    DXDelete((Object)grid->gridlevels[i].bucketArray);

	DXDelete((Object)grid->itemArray);
    }

    return NULL;
}

void
_dxfFreeSearchGrid(Grid grid)
{
    GridLevel gl;
    int  i;

    if (grid)
    {
	for (i = 0, gl = grid->gridlevels; i < MAXGRID; i++, gl++)
	{
	    if (gl->bucketArray)
	    {
		DXDelete((Object)gl->bucketArray);
		gl->bucketArray = NULL;
	    }
	}

	if (grid->itemArray)
	{
	    DXDelete((Object)grid->itemArray);
	    grid->itemArray  = NULL;
	}

	DXFree((Pointer)grid);
    }
}

Grid
_dxfCopySearchGrid(Grid src)
{
    int  i;
    Grid dst;

    dst = (Grid)DXAllocate(sizeof(struct grid));
    if (! dst)
	return NULL;

    memcpy((char *)dst, (char *)src, sizeof(struct grid));

    for (i = 0; i < MAXGRID; i++)
	if (dst->gridlevels[i].bucketArray)
	    DXReference((Object)dst->gridlevels[i].bucketArray);

    DXReference((Object)dst->itemArray);

    return dst;
}

Error
_dxfAddItemToSearchGrid(Grid grid, float **vertices, int nVerts, int prim)
{
    int		 itemNum, bucketOffset;
    int		 level;
    GridLevel 	 gridlevel;
    Item	 itemPtr;
    register int i, j, nDim;
    int		 *stride;
    float 	 fMin[MAXDIM], fMax[MAXDIM];
    int		 iMin[MAXDIM], iMax[MAXDIM];

    nDim    = grid->nDim;

    /*
     * Get minmax of element in grid space
     */
    for (i = 0; i < nDim; i++)
    {
	fMax[i] = -DXD_MAX_FLOAT;
	fMin[i] =  DXD_MAX_FLOAT;
    }

    for (i = 0; i < nVerts; i++)
    {
	float *min   = grid->min;
	float *scale = grid->scale;

	for (j = 0; j < nDim; j++)
	{
	    float t = (vertices[i][j] - *min++) * *scale++;

	    if (t < fMin[j])
		fMin[j] = t;

	    if (t > fMax[j])
		fMax[j] = t;
	}
    }

    /*
     * For each grid level in turn, map the minmax into the integer index
     * space of the grid level.  Exit when the integer minmax falls into one
     * bin.  Note that the topmost level contains a single bin.
     */
    gridlevel = grid->gridlevels;
    for (level = 0; level < (MAXGRID - 1); level ++, gridlevel ++)
    {
	for (i = 0; i < nDim; i++)
	{
	    iMin[i] = (int)(fMin[i] * (gridlevel->counts[i] - 1));
	    if (iMin[i] < 0)
		iMin[i] = 0;

	    iMax[i] = (int)(fMax[i] * (gridlevel->counts[i] - 1));
	    if (iMax[i] >= gridlevel->counts[i])
		iMax[i] = gridlevel->counts[i] - 1;

	    if (iMin[i] != iMax[i])
		break;
	}

	if (i == nDim)
	    break;
    }

    /*
     * If we didn't succeed, stick it into the top bin
     */
    if (level == MAXGRID-1)
	for (i = 0; i < nDim; i++)
	    iMin[i] = 0;

    /*
     * If necessary, create the desired grid level.
     */
    if (gridlevel->bucketArray == NULL)
    {
	int nBins, *gc, *st, lc;

	/* 
	 * grid counts along the axes, and how many bins in grid?
	 * the following code is a bit obscure in order to avoid a
	 * optimizer bug on the 860
	 */
	gc = gridlevel->counts;
	st = gridlevel->stride;

	/*
	 * Initial setup: stride for first dimension is 1, number of 
	 * bins along that axis is the grid number reduced by g.
	 */
	lc = *gc++;
	nBins = lc;
	*st++ = 1;

	/*
	 * st and gc point to the second slots in the 
	 * counts and strides arrays respectively.  Note the 
	 * loop executes (nDim-1) times for the remaining axes.
	 */
	for (i = 1; i < nDim; i++)
	{
	    /*
	     * stride for the next axis is the length of the last
	     * axis times the stride for the last axis.
	     */
	    *st = *(st-1) * lc;

	    /*
	     * the number of bins in the current axis is...
	     */
	    lc = *gc;
	    
	    /*
	     * And multiply them in to the total bin count
	     */
	    nBins *= lc;

	    /*
	     * now bump the pointers
	     */
	    st ++;
	    gc ++;
	}

	/*
	 * DXAllocate grid's bins
	 */
	gridlevel->bucketArray = DXNewArray(TYPE_INT, CATEGORY_REAL, 1, 1);
	if (! gridlevel->bucketArray)
	    goto error;
    
	DXReference((Object)gridlevel->bucketArray);
    
	if (! DXAddArrayData(gridlevel->bucketArray, 0, nBins, NULL))
	    goto error;

	gridlevel->buckets = (int *)DXGetArrayData(gridlevel->bucketArray);

	memset(gridlevel->buckets, -1, nBins * sizeof(int));

	gridlevel->count = 0;
    }

    /*
     * Now stick the item into that bucket in that grid.
     */
    stride    = gridlevel->stride;

    bucketOffset = iMin[0];
    for (i = 1; i < nDim; i++)
       bucketOffset += iMin[i] * stride[i];

    /*
     * Get a new item
     */
    itemNum = grid->next++;
    itemPtr = grid->items + itemNum;

    /*
     * Set up pointer to item
     */
    itemPtr->primNum = prim;

    /*
     * DXAdd to linked list
     */
    itemPtr->next = gridlevel->buckets[bucketOffset];
    gridlevel->buckets[bucketOffset] = itemNum;

    gridlevel->count ++;
		
    return 1;

error:
    return 0;
}

void
_dxfInitGetSearchGrid(Grid grid, float *pt) 
{ 
    int i;
    float f;

    for (i = 0; i < grid->nDim; i++)
    {
	f = (pt[i] - grid->min[i]) * grid->scale[i];
	if (f < 0)
	    f = 0.0;
	else if (f >= 1.0) 
	    f = 1.0;

	grid->gridPoint[i] = f;
    }

    grid->currentGrid = -1;
}

int
_dxfGetNextSearchGrid(Grid grid)
{
    Item	item;
    int		g, i, offset;
    GridLevel	gridlevel;
    int		index[MAXDIM];

    /*
     * Get pointer to current grid.  Note that the current grid
     * number is initialized to -1 for the search.
     */
    g = grid->currentGrid;
    gridlevel = grid->gridlevels + g;

    /*
     * If this is the initial call, or if we have exhausted the elements at
     * this grid level, move up.
     */
    if (g == -1 || gridlevel->current == -1)
    {
	do
	{
	    g++;
	    gridlevel ++;

	    /*
	     * If there ain't no more buckets, quit
	     */
	    if (g == MAXGRID)
		return -1;
	    
	    /*
	     * If this grid level is for real...
	     */
	    if (gridlevel->bucketArray)
	    {
		/*
		 * shift the indices to correspond to the larger 
		 * gridlevel
		 */
		for (i = 0; i < grid->nDim; i++)
		    index[i] = grid->gridPoint[i] * (gridlevel->counts[i] - 1);

		/*
		 * Get the bucket offset
		 */
		offset = index[0];
		for (i = 1; i < grid->nDim; i++)
		    offset += index[i] * gridlevel->stride[i];
	
		gridlevel->current = gridlevel->buckets[offset];
	    }
	}
	while(gridlevel->current == -1);

	grid->currentGrid = g;
    }

    if (gridlevel->current == -1)
    {
	return -1;
    }
    else
    {
	item = grid->items + gridlevel->current;
	gridlevel->current = item->next;
	return item->primNum;
    }

}

#if 0
#define SAMPLE_SIZE	32

Error
SizeEstimate(Field field, float *size)
{
    Array p, e;
    float *points;
    int *elems, *ePtr;
    int nElems, nDim, nVerts, nSamples;
    int i, j, k, random;
    float l, s;

    p = (Array)DXGetComponentValue(field, "positions");
    if (! p)
	return ERROR;

    e = (Array)DXGetComponentValue(field, "connections");
    if (! e)
	return ERROR;

    points = (float *)DXGetArrayData(p);
    DXGetArrayInfo(p, NULL, NULL, NULL, NULL, &nDim);

    elems = (int *)DXGetArrayData(e);
    DXGetArrayInfo(e, &nElems, NULL, NULL, NULL, &nVerts);

    if (nElems > SAMPLE_SIZE)
    {
	nSamples = SAMPLE_SIZE;
	random = 1;
    }
    else
    {
	nSamples = nElems;
	random = 0;
    }
    
    s = 999999.0;
    for (i = 0; i < nSamples; i++)
    {
 	float *p0, *p1, m;

	if (random)
	    k = rand() % nElems;
	else
	    k = i;
    
	ePtr = elems + k*nVerts;

	p0 = points + nDim*ePtr[0];
	p1 = points + nDim*ePtr[nVerts-1];

	m = 0.0;
	for (j = 0; j < nDim; j++)
	{
	    l = *p0++ - *p1++;
	    if (l < 0.0)
		m += -l;
	    else
		m += l;
	}

	if (m < s)
	    s = m;
    }

    *size = s;

    return OK;
}
#endif
