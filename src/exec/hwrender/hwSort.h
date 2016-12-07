/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	tdmSort_h
#define	tdmSort_h

typedef struct sort 
{
    void 	*xfp;
    int		poly;
} *Sort;

typedef struct sortD
{
    void 	*xfp;
    int		poly;
    float	depth;
} *SortD;

#define DXHW_TLIST_BLOCKSIZE 1024

typedef struct sortBlock
{
    struct sort        list[DXHW_TLIST_BLOCKSIZE];
    int                numUsed;
    struct sortBlock   *next;
} *SortBlock;

typedef struct sortList
{
    SortBlock 	sortBlockList;
    int		itemCount;
    SortD	sortList;
    int		sortListLength;
} *SortListP;

SortList _dxf_newSortList();
void     _dxf_deleteSortList(SortList sl);
void     _dxf_clearSortList(SortList sl);
int      _dxf_Insert_Translucent(SortList sl, void *xfp, int i);
int      _dxf_Sort_Translucent(SortList sl);

#endif	/* tdmSort_h */
