/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef tdmMemory_h
#define tdmMemory_h
/*
 *
 * $Header: /src/master/dx/src/exec/hwrender/hwMemory.h,v 1.4 2002/03/21 02:59:11 rhh Exp $
 */


#ifdef TDM_MEMORY_TRACING

#ifndef PREPROC
#define _LINE_ __LINE__
#endif

Pointer _tdmAllocate(unsigned int n, char * file, int LINE);
Pointer _tdmAllocateZero(unsigned int n, char * file, int LINE);
Pointer _tdmAllocateLocal(unsigned int n, char * file, int LINE);
Pointer _tdmAllocateLocalZero(unsigned int n, char * file, int LINE);
Pointer _tdmReAllocate(Pointer x, unsigned int n, char * file, int LINE);
Error   _tdmFree(Pointer x, char * file, int LINE);
Error   _tdmCheckAllocTable(int);

#define tdmAllocate(n)          _tdmAllocate(n,__FILE__,_LINE_)
#define tdmAllocateZero(n)      _tdmAllocateZero(n,__FILE__,_LINE_)
#define tdmAllocateLocal(n)     _tdmAllocateLocal(n,__FILE__,_LINE_)
#define tdmAllocateLocalZero(n) _tdmAllocateLocalZero(n,__FILE__,_LINE_)
#define tdmReAllocate(x,n)      _tdmReAllocate(x,n,__FILE__,_LINE_)
#define tdmFree(x)              _tdmFree(x,__FILE__,_LINE_)
#define tdmCheckAllocTable(flag) _tdmCheckAllocTable(flag)


#else

#define tdmAllocate(n)          DXAllocate(n)
#define tdmAllocateZero(n)      DXAllocateZero(n)
#define tdmAllocateLocal(n)     DXAllocateLocal(n)
#define tdmAllocateLocalZero(n) DXAllocateLocalZero(n)
#define tdmReAllocate(x,n)      DXReAllocate(x,n)
#define tdmFree(x)              DXFree(x)
#define tdmCheckAllocTable(flag) 

#endif






#endif
