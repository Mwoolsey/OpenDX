/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/pendingcmds.h,v 1.4 2006/01/04 22:00:49 davidt Exp $
 */

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#ifndef _PENDINGCMDS_H
#define _PENDINGCMDS_H

#define PJL_KEY "__PendingObjectList"
typedef int (*PFI_P)(Private);

typedef struct
{
    char    *major;
    char    *minor;
    PFI_P   job;
    Private data;
} PendingCmd;

typedef struct
{
    Object       exprivate;
    PendingCmd  *pendingCmdList;
    int          pendingCmdListMax;
    int          pendingCmdListSize;
} PendingCmdList;

Error _dxf_RunPendingCmds();
Error _dxf_CleanupPendingCmdList();

#endif /* _PENDINGCMDS_H */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
