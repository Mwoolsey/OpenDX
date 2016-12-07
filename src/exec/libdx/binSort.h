/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#undef STATS

typedef struct grid		*Grid;

Grid		_dxfMakeSearchGrid(float *, float *, int, int);
void	 	_dxfFreeSearchGrid(Grid);
Grid		_dxfCopySearchGrid(Grid);
Error		_dxfAddItemToSearchGrid(Grid, float **, int, int);
void	 	_dxfInitGetSearchGrid(Grid, float *);
int	 	_dxfGetNextSearchGrid(Grid);

/*
 * These are the linked list entries
 */
struct _item
{
    int 	    primNum;
    int 	    next;
};

/*
 * These are used to create a list of blocks of bin entries allocated for
 * dynamic allocation and deallocation.
 */

#define MAXDIM		3
#define MAXGRID		32
#define MAXVERTS	16

struct _gridlevel
{
    int			stride[MAXDIM];
    int			counts[MAXDIM];
    Array		bucketArray;
    int			*buckets;
    int			current;
    int			count;
};

struct grid
{
    int			nDim;

    float		min[MAXDIM];
    float		scale[MAXDIM];
    int			counts[MAXDIM];

    int			currentGrid;
    float		gridPoint[MAXDIM];

    struct _gridlevel	gridlevels[MAXGRID];

    Array   	    	itemArray;
    struct _item 	*items;
    int	   	    	next;
};
