/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>
#include "socket.h"
#include "sfile.h"

#if !defined(SOCKET)
#define SOCKET int
#endif
 
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dx/arch.h>
#include <sys/types.h>

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif
#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif
#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif
#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#ifdef DXD_WIN
#include <sys/timeb.h>
#endif

#include <errno.h>
#include <stdlib.h>

#define SOCK_QUEUE_LENGTH	1
#define SOCK_ACCEPT_TIMEOUT	60	/* Seconds */

/*
 * Parse the DXHOST environment variable and determine the host name and
 * socket port number to use for remote connections.  The format of the
 * varible is 'hostname[,portnumber]'.  The default port number (if one
 * is not given) is 1900.  This number is arbitrary, but should not collide
 * with anything in /etc/services.  In general, I believe that most numbers
 * over 1024 will work.
 */
int _dxf_ExGetSocket(char *name, int *port)
{
    char *getenv();
    char *env;
    char *p;

    if (name)
	*name = '\0';
    *port = 1900;

    env = getenv("DXHOST");
    if (env == NULL)
	return(0);

    p = env;
    while (*p && *p != ',')
	p++;

    if (*p == ',')
    {
	if (name)
	{
	    p = env;
	    while (*p && *p != ',')
		*name++ = *p++;
	    *name = '\0';
	}
	p++;
        *port = atoi(p);
    }
    return(1);
}

/*
 * Open a socket port and wait for a client to connect.
 * This opens 2 sockets (except on the server), one internet domain, and
 * one unix domain.  It then selects on both sockets to see to which one the
 * ui connects, and returns the new connection, closing the one not
 * selected.
 */
SFILE *
_dxf_ExInitServer(int pport)
{

#if defined(HANDLE_SOCKET)
	SOCKET sock = INVALID_SOCKET;
#else
	SOCKET sock = -1;
#endif

    struct sockaddr_in server;
#if DXD_SOCKET_UNIXDOMAIN_OK
    int usock = -1;
    struct sockaddr_un userver;
    int oldUmask;
#endif
    struct linger sl;
    SOCK_LENGTH_TYPE length;
    ushort port;
    int fd;
    int sts;
    int oldPort;
    extern int errno; /* from <errno.h> */
    int tries;
    fd_set fds;
    int width = FD_SETSIZE;
    struct timeval to;


    port = pport;

retry:


    sock = socket(AF_INET, SOCK_STREAM, 0);
#if defined(HANDLE_SOCKET)
	if (sock < 0 || sock == INVALID_SOCKET)
#else
	if(sock < 0)
#endif
    {
	perror ("socket");
	fd = -1;
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
	fd = -1;
	goto error;
    }

    setsockopt(usock, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));
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
	fd = -1;
	goto error;
    }

    if (getsockname(sock, (struct sockaddr *)&server, &length) < 0)
    {
	perror ("getsockname");
	fd = -1;
	goto error;
    }

#if DXD_SOCKET_UNIXDOMAIN_OK
    userver.sun_family = AF_UNIX;
    sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);
    length = sizeof (userver) - sizeof(userver.sun_path) +
	     strlen (userver.sun_path);

    oldUmask = umask(0);
    mkdir("/tmp/.DX-unix", 0777);
    chmod("/tmp/.DX-unix", 0777);
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
	fd = -1;
	goto error;
    }
#endif

    if (listen(sock, SOCK_QUEUE_LENGTH) < 0)
    {
	perror ("listen");
	fd = -1;
	goto error;
    }
#if DXD_SOCKET_UNIXDOMAIN_OK
    if (listen(usock, SOCK_QUEUE_LENGTH) < 0)
    {
	perror ("listen");
	fd = -1;
	goto error;
    }
#endif

    printf ("port = %d\n", ntohs(server.sin_port)); 
    fflush(stdout);

    /* Wait (in select) for someone to connect.
     * if we get through the error checks, someone must want to connect to 
     * our port, accept him/her.  Otherwize, block in accept until someone
     * connects.  If stdin is not a terminal, timeout after SOCK_ACCEPT_TIMEOUT
     * seconds.
     */
#ifndef DXD_HAS_LIBIOP 
    FD_ZERO(&fds);
    FD_SET(sock, &fds);
#if DXD_SOCKET_UNIXDOMAIN_OK
    FD_SET(usock, &fds);
#endif
    if (!isatty(0)) {
	to.tv_sec = SOCK_ACCEPT_TIMEOUT;
	to.tv_usec = 0;
	sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, &to);
    }
    else
    {
	sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, NULL);
    }
    if (sts < 0) {
	perror("select");
	fd = -1;
	goto error;
    }
    else if (sts == 0) {
	fprintf (stderr, "connection timed out\n");
	fd = -1;
	goto error;
    }
#else
    {
	double beforeSelect;
	double SVS_double_time();
	int    tty = isatty(0);

	beforeSelect = SVS_double_time();
	do {
	    FD_ZERO(&fds);
	    FD_SET(sock, &fds);
	    to.tv_sec = 0;
	    to.tv_usec = 0;
	    sts = select(width, (SELECT_ARG_TYPE *) &fds, NULL, NULL, &to);
	    if (sts < 0) {
		perror("select");
		fd = -1;
		goto error;
	    }
	} while (sts == 0 && 
		 (tty ||
		  SVS_double_time() - beforeSelect < SOCK_ACCEPT_TIMEOUT));
	if (sts == 0) {
	    fprintf (stderr, "connection timed out\n");
	    fd = -1;
	    goto error;
	}
    }
#endif

#if DXD_SOCKET_UNIXDOMAIN_OK
    if (FD_ISSET(sock, &fds))
    {
#endif
	if ((fd = accept(sock, (struct sockaddr *)&server, &length)) < 0)
	{
	    perror ("accept");
	    fd = -1;
	    goto error;
	}

#if DXD_SOCKET_UNIXDOMAIN_OK
    }
    else
    {
	if ((fd = accept(usock, (struct sockaddr *)&userver, &length)) < 0)
	{
	    perror ("accept");
	    fd = -1;
	    goto error;
	}
    }
#endif

    printf ("server: accepted connection from client\n");

#if defined(HAVE_SETSOCKOPT)
    {
	char *s = getenv("DX_SOCKET_BUFSIZE");
	if (s)
	{
	    SOCK_LENGTH_TYPE rq_bufsz, set_bufsz;
	    SOCK_LENGTH_TYPE set_len = sizeof(set_bufsz), rq_len = sizeof(rq_bufsz);

	    rq_bufsz = atoi(s);
	    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&rq_bufsz, rq_len))
	    {
		perror("setsockopt");
		goto error;
	    }
	    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *)&set_bufsz, &set_len))
	    {
		perror("getsockopt1");
		goto error;
	    }
	    if (rq_bufsz > set_bufsz + 10)
	        DXWarning("SOCKET bufsize mismatch: send buffer (%d %d)",
			rq_bufsz, set_bufsz);


	    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&rq_bufsz, rq_len))
	    {
		perror("setsockopt");
		goto error;
	    }
	    set_len = sizeof(set_bufsz);
	    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *)&set_bufsz, &set_len))
	    {
		perror("getsockopt2");
		goto error;
	    }
	    if (rq_bufsz > set_bufsz + 10)
	        DXWarning("SOCKET bufsize mismatch: rvc buffer (%d %d)",
			rq_bufsz, set_bufsz);
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

#if defined(HANDLE_SOCKET)
		closesocket(sock);
#else
		close (sock);
#endif
    return socketToSFILE((SOCKET)fd);
}

#ifdef TEST
/*
 * Initiate a connection to a server.
 */
static int
init_client(char *host, int pport)
{
    int sock;
    struct sockaddr_in server;
#if DXD_SOCKET_UNIXDOMAIN_OK
    struct sockaddr_un userver;
#endif
    struct hostent *hp;
#if DXD_HAS_GETHOSTBYNAME
    struct hostent *gethostbyname();
#endif
    u_short port = pport;

#if DXD_SOCKET_UNIXDOMAIN_OK
    if (strcmp(host, "unix") == 0)
    {
	int length;

	memset((char *)&userver, 0, sizeof(userver));
	userver.sun_family = AF_INET;
	sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);
	length = sizeof (userver) - sizeof(userver.sun_path) +
		 strlen (userver.sun_path);

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	    return(-1);
	
	if (connect(sock, (struct sockaddr *) &userver, length) < 0)
	{
	    perror("connect");
	    close(sock);
	    return(-1);
	}
    }
    else
#endif
    {
	memset((char *)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	/* get host address */
	if (isdigit(host[0]))
	    server.sin_addr.s_addr = inet_addr(host);
	else
	{
	    hp = gethostbyname(host);
	    if (hp == NULL)
		return(-1);
	    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	    return(-1);
	
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
	{
	    close(sock);
	    return(-1);
	}
    }

    printf ("client:  connected to server\n");

    return(sock);
}
#endif


#ifdef TEST
main(int argc, char *argv[])
{
    int fd;
    char buffer[1000];

    if (argc < 3)
    {
	fd = _dxf_ExInitServer(1959);

	write(fd, "good bye\n", 9);
	buffer[read(fd, buffer, 1000)] = '\0';
	fprintf(stdout, buffer);

	close (fd);
    }
    else
    {
	fd = init_client(argv[1], atol(argv[2]));

	buffer[read(fd, buffer, 1000)] = '\0';
	fprintf(stdout, buffer);

	printf("> "); gets(buffer);

	write(fd, "hello\n", 6);


	close (fd);
    }
}
#endif
