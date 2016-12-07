/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>


#include <dx/dx.h>

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#elif defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
#endif

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#if defined(HAVE_SYS_SOCKET_H)
#include <sys/socket.h>
#endif

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif

#include <errno.h>

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#define verbose 0

int
_dxfHostIsLocal(char *host)
{
    char localHostname[BUFSIZ];
    char localhostHostname[BUFSIZ];
    char remoteHostname[BUFSIZ];
    struct hostent *he;
    int  hostnameFound;
#ifndef DXD_OS_NON_UNIX
    if (strcmp ("unix", host) == 0)
	return 1;
#else
    char tmpHost[MAXHOSTNAMELEN], *p;

    if(!host || !strlen(host) || !_stricmp("localhost", host) || !_stricmp("localPC", host))
	return 1;
    strcpy(tmpHost, host);
    p = strstr(tmpHost, ":0");
    if(p) {
	tmpHost[p-tmpHost] = '\0';
    }
    if(!tmpHost || !strlen(tmpHost) || !_stricmp("localhost", tmpHost) || !_stricmp("localPC", tmpHost))
	return 1;
    gethostname(localHostname, BUFSIZ);
    if(strcmp(tmpHost, localHostname) == 0)
	return 1;  /*  default to localhost    */
#endif

    /* Local host names do not have to resolve even in UNIX.
       If it doesn't resolve, then assume localhost instead
       of returning error. 
     */

    he = gethostbyname (host);
    if (he == NULL) {
		return 1;   /* Default to LocalHost */
    }
    strcpy(remoteHostname, he->h_name);

    if (gethostname(localHostname, BUFSIZ) >= 0 && (he=gethostbyname(localHostname)) != NULL)
    {
	strcpy(localHostname, he->h_name);
	hostnameFound = 1;
    }
    else
    {
#ifdef DXD_OS_NON_UNIX
	return 1;   /* Default to LocalHost */
#endif
	hostnameFound = 0;
    }

    he = gethostbyname ("localhost");
    if (he == NULL) {
#ifdef DXD_OS_NON_UNIX
	return 1;   /* Default to LocalHost */
#else
	herror("localhost");
	return(0);
#endif
    }
    strcpy(localhostHostname, he->h_name);

    return ((hostnameFound && strcmp (localHostname, remoteHostname) == 0) ||
	    strcmp (localhostHostname, remoteHostname) == 0);
}

/*
 * Initiate a connection to a server.
 */
int DXConnectToServer(char *host, int pport)
{
    int sock;
    struct sockaddr_in server;
#if DXD_SOCKET_UNIXDOMAIN_OK
    struct sockaddr_un userver;
#endif
    struct hostent *hp;
    int length;
#ifndef DXD_HAS_WINSOCKETS
    struct hostent *gethostbyname();
#endif
    ushort port = pport;

#if DXD_SOCKET_UNIXDOMAIN_OK
    if (_dxfHostIsLocal(host))
    {
	memset((char *)&userver, 0, sizeof(userver));
	userver.sun_family = AF_INET;
	sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);
	length = sizeof (userver) - sizeof(userver.sun_path) + strlen (userver.sun_path);

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
	    return(-1);
	
	if (connect(sock, (struct sockaddr *) &userver, length) >= 0)
	{
	    return(sock);
	}
	close (sock);
	sock = -1;
    }
#endif

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

    return(sock);
}
