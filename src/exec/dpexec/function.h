/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/function.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _FUNCTION_H
#define _FUNCTION_H

#include "utils.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

Error _dxf_ExFunctionDone ();
Error DXAddModuleV (char *name, PFI func, int flags, int nin, char *inlist[],
                    int nout, char *outlist[], char *exec, char *host);
Error DXAddModule (char *name, ...);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _FUNCTION_H */
