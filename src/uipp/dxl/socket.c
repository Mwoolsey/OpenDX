/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif
#include <stdio.h>
#include <ctype.h>

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif

#if defined(HAVE_NETINET_IN_H)
#include <netinet/in.h>
#endif

#if defined(HAVE_ARPA_INET_H)
#include <arpa/inet.h>
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
#include <stdarg.h>

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#include "../base/IBMVersion.h"
#include "dxlP.h"
#ifdef OS2
#include <sys/ioctl.h>
#endif

#if defined(HAVE_WINIOCTL_H)
#include <winioctl.h>
#endif
#if defined(HAVE_SYS_SIGNAL_H)
#include <sys/signal.h>
#endif
#if defined(HAVE_SIGNAL_H)
#include <signal.h>
#endif

#if defined(HAVE_SYS_UTSNAME_H)
#include <sys/utsname.h>
#endif

#include <string.h>

extern int _dxl_FreeMessageHandler_1();

	
extern DXLConnection *DXLNewConnection();

#ifdef NON_ANSI_SPRINTF
int
_DXLsprintf(char *s, const char* format, ...)
{
    va_list ap;
    va_start(ap,format);
    vsprintf(s,(char*)format,ap);
    va_end(ap);
    return strlen(s);
}

#endif

#define DXL_QUEUE_LENGTH	1

static int basePort = DXL_BASE_PORT;

void
DXLSetBasePort(const int port)
{
    basePort = port;
}

int
DXLGetBasePort()
{
    return basePort;
}

#if 0
static char hostname[256] = "localhost";
static char hostIsLocal = 1;

void
DXLSetHost(const char *host)
{
    strcpy(hostname, host);
    hostIsLocal = HostIsLocal(host);
}

char *
DXLGetHost()
{
    return hostname;
}

int
DXLIsHostLocal()
{
    return hostIsLocal;
}

#endif

static int nConnection = 0;
static int
_dxl_MakeConnection(DXLConnection *connection, int port, const char *host)
{
    int   local;
    u_short pport;
    struct sockaddr_in server;
#ifdef DXD_HAS_WINSOCKETS
    int i;
    fd_set fdtmp, dxfds;
#endif

#if   !defined(DXD_IBM_OS2_SOCKETS)  && !defined(DXD_HAS_WINSOCKETS)
    if (port <= 0)
    {
	fprintf(stderr, "_dxl_MakeConnection: bad port\n");
	goto error;
    }

#endif

    if (host) {
	local = DXLIsHostLocal(host);
    } else {
	host = "localhost";
	local = 1; 
    }

    /*
     * If this is the first instance of a packet interface, then set up
     * to ignore SIGPIPE (but we watch for EPIPE).
     */
#if defined(HAVE_SIGPIPE)
    if ((nConnection++ == 0) && (getenv("DX_NOSIGNALS") == NULL))
	signal(SIGPIPE, SIG_IGN);
#endif

/*
 * SMH  NT can't local socket AF_UNIX
 */
#if defined(HAVE_SYS_UN_H)
    
    if (local)
    {
	struct sockaddr_un userver;

	memset((char*)&userver, 0, sizeof(userver));
	userver.sun_family = AF_UNIX;
	sprintf(userver.sun_path, "/tmp/.DX-unix/DX%d", port);

	connection->fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (connection->fd < 0)
	    goto error;
	
	if (connect(connection->fd,
		    (struct sockaddr *)&userver,
		    sizeof(userver) - sizeof(userver.sun_path) +
				      strlen(userver.sun_path)) >= 0)
	{
	    return 1;
	}
	else if (!strcmp(host, "unix"))
	{
	    perror("_dxl_MakeConnection");
	    goto error;
	}
    }


    memset((char *)&server, 0, sizeof(server));

#endif 
/*
 * SMH end bypass AF_UNIX Style socket
 */

    server.sin_family = AF_INET;
    pport = port;
    server.sin_port   = htons(pport);

    /*
     * Get host address.
     */
    if (isdigit(host[0]))
    {
#ifdef aviion
        server.sin_addr.s_addr = inet_addr().s_addr;
#else
	/*
	 * suits explicit cast of host
	 */
	server.sin_addr.s_addr = inet_addr((char*)host);  
#endif
    }
    else
    {
	struct hostent* hostp;
#ifdef DXD_OS_NON_UNIX
        if (local || !_stricmp((char*)host, "localhost") || !_stricmp((char*)host, "localPC"))
	{
	    unsigned long locaddr = inet_addr("127.0.0.1");
	    memcpy((void*)&server.sin_addr, &locaddr, sizeof(unsigned long));
	}
	else
#endif
	{
	    hostp = gethostbyname((char*)host);
	    if (hostp == NUL(struct hostent*))
	    {
		goto error;
	    }
	    memcpy((void*)&server.sin_addr, hostp->h_addr, hostp->h_length);
	}
    }
    connection->fd = socket(AF_INET, SOCK_STREAM, 0);
#if !defined(intelnt) && !defined(WIN32)
    if (connection->fd < 0) 
        goto error;
#else
    if (connection->fd < 0) {
	int s_error = WSAGetLastError();
	printf("Socket error %d connecting to server\n", s_error);
	goto error;
    }
#endif

    if (connect(connection->fd, (struct sockaddr*)&server, sizeof(server)) < 0)
	goto error;

#if   defined(DXD_IBM_OS2_SOCKETS)   || defined(DXD_HAS_WINSOCKETS)
    {
#ifdef DXD_IBM_OS2_SOCKETS
	int dontblock = 1;
	if (ioctl(connection->fd, FIONBIO, (char *) &dontblock, sizeof(dontblock))<0)
	     goto error;
	if (select((int *)&connection->fd, 0, 1, 0, -1) <= 0)
	     goto error;
#endif

#ifdef DXD_HAS_WINSOCKETS
	u_long dontblock = 1;
	struct timeval  tv;
	tv.tv_usec = 5;
	tv.tv_sec = 0;
	if (ioctlsocket (connection->fd, FIONBIO, (u_long *) &dontblock)<0)
	     goto error;

	FD_ZERO(&dxfds);
	i = connection->fd;
	FD_SET(i, &dxfds);
	i = select(i, &dxfds, NULL, NULL, &tv);
	if(i < 0) {
	    int fstest = WSAGetLastError();
	    printf("socket error %d connecting to server\n", fstest);
	    goto error;
	}
#endif
    }
#endif


    return 1;

error:

    return 0;
 }

DXLConnection *
DXLConnectToRunningServer(int port, const char *host)
{
    DXLEvent event;
    DXLConnection *connection = DXLNewConnection(); 
    if (! connection)
	goto error;
    

    if (getenv("DXLTRACE")) {
        connection->debugMessaging = 1;
	fprintf(stderr,"DXLink library is version %d.%d.%d\n",
		DXD_VERSION,DXD_RELEASE,DXD_MODIFICATION);
    }


    if (!_dxl_MakeConnection(connection, port, host))
	goto error;

    /* Ask the server for its version */
    DXLSendImmediate(connection, "version"); 
    /* Wait until we get a response */
    if (DXLWaitPacketString(connection,PACK_INFO,"version:",&event)) {
	int major, minor, micro, items;

	if (strstr(event.contents,"Executive"))
	{
	    connection->dxuiConnected = 0;
	    items = sscanf(event.contents,"Executive version: %d %d %d",
			    &major, &minor, &micro);
	    if (items == 3)
	    {
		    connection->majorVersion = major;
		    connection->minorVersion = minor;
		    connection->microVersion = micro;
	    }
	    DXLClearEvent(&event);
	}
	else if (strstr(event.contents,"UI"))
	{
	    connection->dxuiConnected = 1;
	    items = sscanf(event.contents,"UI version: %d %d %d",
			    &major, &minor, &micro);
	    if (items == 3)
	    {
		    connection->majorVersion = major;
		    connection->minorVersion = minor;
		    connection->microVersion = micro;
	    }
	    DXLClearEvent(&event);
	}
	DXLClearEvent(&event);
    }

    if (connection->majorVersion == 0) {
	fprintf(stderr,"Can not determine server version number!\n");
	goto error;
	
    }

    if (connection->debugMessaging) 
	fprintf(stderr,"DXLink connected to server at version %d.%d.%d\n",
			connection->majorVersion,
			connection->minorVersion,
			connection->microVersion);
  
    /*
     * Make sure the remote connection is at the same or later release
     * than this library.
     */
    if ((connection->majorVersion < DXD_VERSION) ||
	(connection->minorVersion < DXD_RELEASE)) { 
	fprintf(stderr,"Warning: DXLink library (V %d.%d) is at a later "
			"level than the\n\t remote connection (V %d.%d)\n",
			DXD_VERSION,DXD_RELEASE, 
			connection->majorVersion, 
			connection->minorVersion);
    }

    return connection;

error:
    if (connection)
	DXLCloseConnection(connection);

    return NULL;
}


void
DXLSetSynchronization(DXLConnection *conn, const int onoff)
{
    conn->synchronous = onoff;
}

int
DXLIsHostLocal(const char *host)
{
    char localHostname[BUFSIZ];
    char localhostHostname[BUFSIZ];
    char remoteHostname[BUFSIZ];
    struct hostent *he;
    int  hostnameFound;
#if defined(HAVE_SIGPIPE)
    struct utsname Uts_Name;

    if (strcmp ("unix", host) == 0)
	return TRUE;

    he = gethostbyname ((char*)host);
    if (he == NULL) {
	fprintf(stderr, "%s: Invalid host", host);
	return(0);
    }
    strcpy(remoteHostname, he->h_name);

    if (uname(&Uts_Name) >= 0 && (he=gethostbyname(Uts_Name.nodename)) != NULL)
    {
	strcpy(localHostname, he->h_name);
	hostnameFound = 1;
    }
    else
    {
	hostnameFound = 0;
    }

    he = gethostbyname ("localhost");
    if (he == NULL) {
	fprintf(stderr, "%s: Invalid host", "localhost");
	return (0);
    }
    strcpy(localhostHostname, he->h_name);

    return ((hostnameFound && strcmp (localHostname, remoteHostname) == 0) ||
	    strcmp (localhostHostname, remoteHostname) == 0);
#else
    return TRUE;
#endif  
}

