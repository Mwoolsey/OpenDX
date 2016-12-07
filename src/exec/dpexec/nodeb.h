/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/nodeb.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _NODEB_H
#define _NODEB_H

#include "parse.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int _dxf_ExWriteTree(struct node *pt, int fd);
struct node *_dxf_ExReadTree(int fd, int swap);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _NODEWRITEB_H */
