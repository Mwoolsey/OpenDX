/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/socket.h,v 1.3 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _SOCKET_H
#define _SOCKET_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

int   _dxf_ExGetSocket(char *name, int *port);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _SOCKET_H */
