/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include <stdio.h>
#include <ctype.h>

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(windows) && defined(HAVE_WINSOCK_H)
#include <winsock.h>
#elif defined(HAVE_CYGWIN_SOCKET_H)
#include <cygwin/socket.h>
#elif defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#include <errno.h>
#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "dxlP.h"

DXLError
uiDXLDisconnect(DXLConnection *conn)
{
    int sts;

    sts = DXLSend(conn, "disconnect");
    DXLCloseConnection(conn);

    return sts;
}
DXLError
DXLExitDX(DXLConnection *conn)
{
    int retval;  
    int sts;
    fd_set fds;
#ifdef hp700
    int width = MAXFUPLIM;
#else
#ifdef aviion
    int width = NOFILE;
#else
#ifdef solaris
    int width = FD_SETSIZE;
#else
#ifdef  DXD_HAS_WINSOCKETS
    int width = FD_SETSIZE;
#else
#ifndef OS2
    int width = getdtablesize();
#endif
#endif
#endif
#endif
#endif
    const char *cmd;

    if (conn->fd <= 0 )
	return ERROR;

    if (conn->dxuiConnected)
	cmd = "exit";
    else
	cmd = "quit;\n";

    /*
    // Don't need to syncrhonize after sending the quit/exit commands
    */
    DXLSetSynchronization(conn,0);

    sts = DXLSend(conn, cmd);

    FD_ZERO(&fds);
    FD_SET(conn->fd, &fds);
#ifdef DXD_HAS_WINSOCKETS
    select(width, &fds, NULL, NULL, NULL);
#else
#ifndef OS2
    /* this will set retval even though we don't need it here */
    SELECT_INTR(width, (SELECT_ARG_TYPE *)&fds, NULL, NULL, NULL, retval);
#else
    select(&conn->fd, 1, 0, 0, -1);
#endif
#endif

    DXLCloseConnection(conn);

    return sts;
}

