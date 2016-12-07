/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/
/*
 * $Header: /src/master/dx/src/exec/dpexec/sfile.h,v 1.7 2004/06/09 16:14:28 davidt Exp $
 */

#ifndef _SFILE_H
#define _SFILE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

typedef void SFILE;

#if !defined(HAVE_CYGWIN_SOCKET_H) &&  !defined(HAVE_SYS_SOCKET_H) && !defined(HAVE_SOCKET_H) && defined(HAVE_WINSOCK_H)
#  define HANDLE_SOCKET
   SFILE *socketToSFILE(SOCKET sock);
#else
#  if !defined(SOCKET)
#    define SOCKET int
#  endif
SFILE *socketToSFILE(SOCKET);
#endif

SFILE *fdToSFILE(int fd);
SFILE *FILEToSFILE(FILE *fptr);
int    readFromSFILE(SFILE *sf, char *buf, int n);
int    writeToSFILE(SFILE *sf, char *buf, int n);
int    closeSFILE(SFILE *sf);
int    SFILECharReady(SFILE *sf);
int    SFILEGetChar(SFILE *sf);
int    SFILEIoctl(SFILE *sf, int cmd, void *argp);
int    SFILEfileno(SFILE *sf);
int    SFILEisatty(SFILE *sf);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /* _SFILE_H */
