/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include <stdio.h>

#if defined(HAVE_CTYPE_H)
#include <ctype.h>
#endif
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(HAVE_ERRNO_H)
#include <errno.h>
#endif
#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif
#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include "ccm.h"

#define SOCK_QUEUE_LENGTH	1
#define SOCK_ACCEPT_TIMEOUT	60	/* Seconds */

/*
 * Open a socket port and wait for a client to connect.
 * This opens 2 sockets (except on the server), one internet domain, and
 * one unix domain.  It then selects on both sockets to see to which one the
 * ui connects, and returns the new connection, closing the one not
 * selected.
 */
int
_dxfSetupServer(int pport, int *psock,
	    struct sockaddr_in *pserver
#if DXD_SOCKET_UNIXDOMAIN_OK
	    , int *pusock,
	    struct sockaddr_un *puserver
#endif
	    )
{
    int sock = -1;
    struct sockaddr_in server;
#if DXD_SOCKET_UNIXDOMAIN_OK
    int usock = -1;
    struct sockaddr_un userver;
    int oldUmask;
#endif
    struct linger sl;
    socklen_t length;
    u_short port;
    int sts;
    int oldPort;
    extern int errno; /* from <errno.h> */
    int tries;

    port = pport;

retry:
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
	perror ("socket");
	goto error;
    }

    sl.l_onoff = 1;
    sl.l_linger = 0;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&sl, sizeof(sl));

#if DXD_SOCKET_UNIXDOMAIN_OK
    usock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (usock < 0)
    {
	perror ("socket");
	goto error;
    }

    setsockopt(usock, SOL_SOCKET, SO_LINGER, (char *)&sl, sizeof(sl));
#endif

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    length = sizeof(struct sockaddr_in);

    /* Try to find a working port, and keep trying if you get EADDRINUSE.
     * If we get some other error, try a few times because we sometimes
     * get bad error numbers on the server.
     */
    tries = 0;
    while ((sts = bind(sock, (struct sockaddr *)&server, length)) < 0 && 
	    (errno == EADDRINUSE || tries++ < 5))
    {
	oldPort = port;
	server.sin_port = htons(++port);
	if (ntohs(server.sin_port) < oldPort)
	    break;
    }
    if (sts < 0)
    {
	perror ("bind");
	goto error;
    }

    if (getsockname(sock, (struct sockaddr *)&server, &length) < 0)
    {
	perror ("getsockname");
	goto error;
    }


#if DXD_SOCKET_UNIXDOMAIN_OK
    userver.sun_family = AF_UNIX;
    sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);
    length = sizeof (userver) - sizeof(userver.sun_path) + strlen (userver.sun_path);

    oldUmask = umask(0);
    mkdir("/tmp/.DX-unix/", 0777);
    chmod("/tmp/.DX-unix/", 0777);
    umask(oldUmask);

    unlink(userver.sun_path);


    /* Try to find a working port, and keep trying if you get EADDRINUSE.
     * If we get some other error, try a few times because we sometimes
     * get bad error numbers on the server.
     */
    if ((sts = bind(usock, (struct sockaddr *)&userver, length)) < 0 && 
	    errno == EADDRINUSE)
    {
	oldPort = port;
	server.sin_port = htons(++port);
	close (sock);
	close (usock);
	sock = -1;
	usock = -1;
	if (ntohs(server.sin_port) > oldPort)
	    goto retry;
    }
    if (sts < 0)
    {
	perror ("bind");
	goto error;
    }
#endif

    if (listen(sock, SOCK_QUEUE_LENGTH) < 0)
    {
	perror ("listen");
	goto error;
    }
#if DXD_SOCKET_UNIXDOMAIN_OK
    if (listen(usock, SOCK_QUEUE_LENGTH) < 0)
    {
	perror ("listen");
	goto error;
    }
#endif
    *psock = sock;
    *pserver = server;
#if DXD_SOCKET_UNIXDOMAIN_OK
    *pusock = usock;
    *puserver = userver;
#endif
    return (port);

error:
    *psock = -1;
#if DXD_SOCKET_UNIXDOMAIN_OK
    *pusock = -1;
#endif
    return (-1);
}

void _dxfPrintConnectTimeOut(char *execname, char *hostname)
{
  DXMessage("Starting %s on %s; will wait up to %d seconds for connections.", 
               execname, hostname, SOCK_ACCEPT_TIMEOUT);
}

int
_dxfCompleteServer(int sock, 
    struct sockaddr_in server
#if DXD_SOCKET_UNIXDOMAIN_OK
    , int usock,
    struct sockaddr_un userver
#endif
    , int timeout
    )
{
    socklen_t length;
    int fd=-1;
    int sts;
    extern int errno; /* from <errno.h> */
    fd_set fds;
#ifdef   DXD_HAS_WINSOCKETS
    int width = FD_SETSIZE;
#else
#if DXD_HAS_GETDTABLESIZE
    int width = getdtablesize();
#else
    int width = MAXFUPLIM;
#endif
#endif
    struct timeval to;
    /* Wait (in select) for someone to connect.
     * if we get through the error checks, someone must want to connect to 
     * our port, accept him/her.  Otherwize, block in accept until someone
     * connects.  If stdin is not a terminal, timeout after SOCK_ACCEPT_TIMEOUT
     * seconds.
     */
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
#if DXD_SOCKET_UNIXDOMAIN_OK
    FD_SET(usock, &fds);
#endif
    to.tv_sec = SOCK_ACCEPT_TIMEOUT;
    to.tv_usec = 0;
    if(timeout)
        sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, &to);
    else 
        sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, NULL);
    if (sts < 0) {
	perror("select");
	goto error;
    }
    else if (sts == 0) {
	fprintf (stderr, "connection timed out\n");
	goto error;
    }

    if (FD_ISSET(sock, &fds))
    {
	length = sizeof(server);
        fd = accept(sock, (struct sockaddr *)&server, &length);
	if (fd < 0)
	{
	    perror ("accept");
	    goto error;
	}
    }
#if DXD_SOCKET_UNIXDOMAIN_OK
    else
    {
	length = sizeof (userver) - sizeof(userver.sun_path) + strlen (userver.sun_path);
        fd = accept(usock, (struct sockaddr *)&userver, &length);
	if (fd < 0)
	{
	    perror ("accept");
	    goto error;
	}
    }
#endif

error:
#if DXD_SOCKET_UNIXDOMAIN_OK
    if (userver.sun_path[0] != '\0')
	unlink (userver.sun_path);
    if (usock >= 0)
	close (usock);
#endif
    if (sock >= 0)
	close (sock);
    return (fd);
}
