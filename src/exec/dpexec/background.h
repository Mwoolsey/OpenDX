/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/background.h,v 1.5 2004/06/09 16:14:27 davidt Exp $
 */

#include <dxconfig.h>


#ifndef	_BACKGROUND_H
#define	_BACKGROUND_H

#include "parse.h"
#include "d.h"

/*
 * Externally visible functions.
 */
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

void	_dxf_ExBackgroundChange	(void);
void	_dxf_ExBackgroundRedo	(void);
void	_dxf_ExBackgroundCommand	(_bg comm, struct node * n);
int	_dxf_ExCheckBackground	(EXDictionary dict, int multiProc);
void	_dxf_ExCleanupBackground	(void);
Error	_dxf_ExInitBackground	(void);
int	ExBackgroundIdle	(void);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif	/* _BACKGROUND_H */
