/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmList_h
#define tdmList_h

listO   _dxf_newList(int initialSize, Error (*delete)(Pointer));
Error   _dxf_listAppendItem(listO list, Pointer item);
Error   _dxf_listEmpty(listO list);
Error   _dxf_listFree(listO list);
Error   _dxf_listSize(listO list);
Pointer _dxf_listGetItem(listO list, int i);

#endif /* tdmList_h */
