/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/status.h,v 1.6 2004/06/09 16:14:28 davidt Exp $
 */

#include <dxconfig.h>

#ifndef	_STATUS_H
#define	_STATUS_H

#include "config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if DXD_PROCESSOR_STATUS

#define PS_MIN		0		/* this must be first and 0 */
#define	PS_NONE		1
#define	PS_EXECUTIVE	2
#define PS_PARSE	3
#define PS_GRAPHGEN	4
#define PS_GRAPHQUEUE	5
#define PS_RUN		6
#define	PS_RECLAIM	7
#define PS_JOINWAIT	8
#define	PS_NAPPING	9
#define	PS_DEAD		10
#define	PS_MAX		11		/* this must be last */

extern int _dxd_exStatusPID; /* from status.c */
extern int *_dxd_exProcessorStatus;

Error _dxf_ExInitStatus	(int n, int flag);
void _dxf_ExCleanupStatus	(void);

#define	set_status(_s)	if (_dxd_exProcessorStatus) _dxd_exProcessorStatus[_dxd_exMyPID] = (_s)
#define	get_status() \
		(_dxd_exProcessorStatus ? _dxd_exProcessorStatus[_dxd_exMyPID] : PS_NONE)
#else
#define	set_status(_s)   FALSE
#define	get_status()     FALSE
#endif

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif	/* _STATUS_H */
