/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>




typedef struct EntryType_ {
    char        *value;
    struct EntryType_   *next;
    struct EntryType_   *prev;
} EntryType;


typedef struct {
    EntryType  *top;
    EntryType  *bottom;
    EntryType  *current;
    int       length;
} Stack;

