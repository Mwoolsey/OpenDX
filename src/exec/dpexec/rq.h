/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/rq.h,v 1.7 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	__EX_RQ_H
#define	__EX_RQ_H

#include "utils.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void		_dxf_ExRQEnqueue	(PFI func, Pointer arg, int repeat,
				 long gid, 	/* Group ID (used)	*/
				 int JID,	/* JOB (pinned proc) or 0 */
				 int highpri);	/* run this NOW		*/
void		_dxf_ExRQEnqueueMany	(int n, PFI func[], Pointer arg[], int repeat[],
				 long gid, 	/* Group ID (used)	*/
				 int JID, 	/* JOB (pinned proc) or 0 */
				 int highpri);	/* run this NOW		*/
int    _dxf_ExRQDequeue	(long gid);	/* Group ID (or 0)	*/
int    _dxf_ExRQPending(void);
Error  _dxf_ExRQInit (void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif	/* __EX_RQ_H */
