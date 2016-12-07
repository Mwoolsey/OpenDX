/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/swap.h,v 1.8 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>

#ifndef _SWAP_H
#define _SWAP_H
#include "graph.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void  _dxf_ExSetGVarCost(gvar *gv, double cost);
void  _dxf_ExReclaimEnable ();
int   _dxf_ExReclaimDisable ();
int   _dxf_ExReclaimMemory (ulong nbytes);
Error _dxf_ExInitMemoryReclaim ();
int   _dxf_ExReclaimingMemory();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _SWAP_H */
