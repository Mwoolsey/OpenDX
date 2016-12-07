/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/remote.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _REMOTE_H
#define _REMOTE_H

#include <dx/dx.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int _dxfExRemoteExec(int dconnect, char *host, char *ruser, int r_argc,
                     char **r_argv, int outboard);
Error DXOutboard (Object *in, Object *out);
Error _dxf_ExDeleteReallyRemote (char *procgroup, char *name, int instance);
Error _dxf_ExDeleteRemote (char *name, int instance);
int   _dxf_ExInitRemote (void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _REMOTE_H */
