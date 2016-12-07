/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/userinter.h,v 1.3 2004/06/09 16:14:29 davidt Exp $
 */

#ifndef _USERINTER_H
#define _USERINTER_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

Error _dxfLoadUserInteractors(char *fname);
Error _dxfLoadDefaultUserInteractors();

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _USERINTER_H */
