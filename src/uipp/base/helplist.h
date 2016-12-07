/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>



#ifndef _helplist_h
#define _helplist_h

typedef struct ListNodeType_ {
    char   *refname;
    int    offset;
    struct ListNodeType_   *next;
    struct ListNodeType_   *prev;
} ListNodeType;


typedef struct {
    ListNodeType  *head;
    ListNodeType  *tail;
    ListNodeType  *current;
    int       length;
} SpotList;

#endif /* _helplist_h */

