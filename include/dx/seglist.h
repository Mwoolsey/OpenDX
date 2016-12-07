/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _DXI_SEGLIST_H_
#define _DXI_SEGLIST_H_

typedef struct slbuf
{
    struct slbuf   *next;
    int            size;
    int		   current;
    char           *items;
} SegListSegment;

typedef struct
{
    int            nextItem;
    int            incAlloc;
    int	           itemSize;
    SegListSegment *segments;
    SegListSegment *currentSeg;
    SegListSegment *iterSeg;
    int            next;
} SegList;

SegList        *DXNewSegList(int, int, int);
SegListSegment *DXNewSegListSegment(SegList *, int);
Error           DXDeleteSegList(SegList *);
Pointer         DXNewSegListItem(SegList *);
Pointer         DXGetNextSegListItem(SegList *);
Error           DXInitGetNextSegListItem(SegList *);
int             DXGetSegListItemCount(SegList *);
Error	        DXInitGetNextSegListSegment(SegList *);
SegListSegment *DXGetNextSegListSegment(SegList *);
Pointer	        DXGetSegListSegmentPointer(SegListSegment *);
int    	        DXGetSegListSegmentItemCount(SegListSegment *);

#endif /* _DXI_SEGLIST_H_ */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
