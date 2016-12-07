/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/dxmain.h,v 1.2 2004/06/09 16:14:27 davidt Exp $
 */

#ifndef	_DXMAIN_H_
#define	_DXMAIN_H_

#include <dx/dx.h>

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/* Function prototypes */

char ** _dxf_ExEnvp (void);
void  _dxf_ExInitSystemVars ();
Error _dxf_child_RQ_message(int *jobid);
Error _dxf_parent_RQ_message();
void _dxf_lock_childpidtbl();
void _dxf_update_childpid(int, int, int);
void _dxf_set_RQ_ReadFromChild1(int);
void _dxf_set_RQ_reader(int);
void _dxf_set_RQ_writer(int);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _DXMAIN_H_ */
