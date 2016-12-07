/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#ifndef	hwFlags_h
#define	hwFlags_h
/*---------------------------------------------------------------------------*\
 $Source: /src/master/dx/src/exec/hwrender/hwFlags.h,v $
  Author: Tim Murphy

  This file contains defintions for he hwFlags data type and manipulation
  functions.

  The logic currently allows 32 flags.

\*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*\
  Defines
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
  Structure Pointers
\*---------------------------------------------------------------------------*/

typedef unsigned long hwFlagsT;
typedef hwFlagsT *hwFlags;

/*---------------------------------------------------------------------------*\
  Function Declarations
\*---------------------------------------------------------------------------*/

hwFlags _dxf_SERVICES_FLAGS();

#define _dxf_isFlagsSet(dstFlags,srcFlags) \
  ((srcFlags) & *(dstFlags))

#define _dxf_setFlags(dstFlags,srcFlags) \
  *(dstFlags) = (*(dstFlags) | (srcFlags))

#define _dxf_clearFlags(dstFlags,srcFlags) \
  *(dstFlags) = (*(dstFlags) & ~(srcFlags))

#endif /* hwFlags_h */


