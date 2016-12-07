/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include <dx/dx.h>
#include "sfile.h"

#if defined(HAVE_TIME_H)
#include <time.h>
#endif
 
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_SYS_TIMEB_H)
#include <sys/timeb.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif                                         

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#include <errno.h>

struct sfile
{
    enum 
    {
#if defined(HANDLE_SOCKET)
    	SFILE_SOCKET,
#endif
	SFILE_FDESC,
	SFILE_FPTR
    }			type;
#if defined(HANDLE_SOCKET)
    SOCKET 		socket;
#endif
    int			fd;
    char                buffer[BUFSIZ+1];
    char                *nextchar;
    FILE                *file;
    int			count;
    int			isAtty;
};

int
SFILEisatty(SFILE *SF)
{
    struct sfile *sf = (struct sfile *)SF;
    return sf->isAtty;
}

int
SFILEfileno(SFILE *SF)
{
    struct sfile *sf = (struct sfile *)SF;

    if (sf->type == SFILE_FDESC || sf->type == SFILE_FPTR)
	return sf->fd;
#if defined(HANDLE_SOCKET)
    else if (sf->type == SFILE_SOCKET)
	return sf->socket;
#endif
    else
    {
	fprintf(stderr, "unknown SFILE type\n");
	abort();
    }
    return -1;  /*  quiet the compiler  */
}

SFILE *
socketToSFILE(SOCKET sock)
{
#if !defined(HANDLE_SOCKET)
	return fdToSFILE((int)sock);
#else
   struct sfile *sf = DXAllocate(sizeof(struct sfile));
    if (! sf)
    	return NULL;

    sf->socket 	 = sock;
    sf->type     = SFILE_SOCKET;
    sf->nextchar = NULL;
    sf->count    = 0;
    sf->isAtty	 = 0;

    return (SFILE *)sf;
#endif
}

SFILE *
FILEToSFILE(FILE *fptr)
{
    struct sfile *sf = DXAllocate(sizeof(struct sfile));
    if (! sf)
    	return NULL;

    sf->type     = SFILE_FPTR;
    sf->file	 = fptr;
    sf->fd	 = fptr == NULL ? -1 : fileno(fptr);
    sf->nextchar = NULL;
    sf->count    = 0;
    sf->isAtty	 = fptr == NULL ? 0 : isatty(sf->fd);

    return (SFILE *)sf;
}

SFILE *
fdToSFILE(int fd)
{
    struct sfile *sf = DXAllocate(sizeof(struct sfile));
    if (! sf)
    	return NULL;

    sf->type 	 = SFILE_FDESC;
    sf->file	 = NULL;
    sf->fd	 = fd;
    sf->nextchar = NULL;
    sf->count    = 0;
    sf->isAtty	 = isatty(fd);

    return (SFILE *)sf;
}

int
closeSFILE(SFILE *sf)
{
    struct sfile *ssf = (struct sfile *)sf;

#if defined(HANDLE_SOCKET)
    if (ssf->type == SFILE_SOCKET)
    	closesocket(ssf->socket);
    else if (ssf->type == SFILE_FDESC)
        close(ssf->fd);
    else
        fclose(ssf->file);
#else
    if (ssf->type == SFILE_FDESC)
        close(ssf->fd);
    else
        fclose(ssf->file);
#endif

    DXFree((Pointer)ssf);
    return OK;
}

int
readFromSFILE(SFILE *sf, char *buf, int n)
{
    struct sfile *ssf = (struct sfile *)sf;
    int a = 0, b = 0;
    if (ssf->count > 0)
    {
        a = (ssf->count > n) ? n : ssf->count;
	memcpy(buf, ssf->nextchar, a);
	ssf->nextchar += a;
	ssf->count -= a;
	n -= a;
    }
    if (n)
    {
#if defined(HANDLE_SOCKET)
        if (ssf->type == SFILE_SOCKET) {
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(ssf->socket, &fdset);
		/* need to select here to see if send is possible */
		select(0, NULL, NULL, &fdset, NULL); /* Blocking */
	    b = recv(ssf->socket, buf+a, n, 0);
        }
	else
#endif
	b = read(ssf->fd, buf+a, n);
	buf[a+b] = '\0';
    }
    return a+b;
}

extern void ExQuit();

int
writeToSFILE(SFILE *sf, char *buf, int n)
{
    int rtn;
    struct sfile *ssf = (struct sfile *)sf;

#if defined(HANDLE_SOCKET)
    if (ssf->type == SFILE_SOCKET) {
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(ssf->socket, &fdset);
		/* need to select here to see if send is possible */
		select(0, NULL, &fdset, NULL, NULL); /* Blocking */
		rtn = send(ssf->socket, buf, n, 0);
	}
    else
#endif
/*	errno = 0; */
	rtn = write(ssf->fd, buf, n);

    if (rtn < 0) {

#if defined(_DEBUG) && defined(intelnt)
		int err = WSAGetLastError();
		ExQuit();
#endif
/*	fprintf(stderr, "Errorno is: %d\n", errno); */
    }
    return rtn;
}

extern SFILE *_dxd_exSockFD;

static int
_SFILE_load(SFILE *sf)
{
    struct sfile *ssf = (struct sfile *)sf;

    if (ssf->count > 0)
        return 1;
    
#if defined(DDX)
    if (ssf->fd == -1)
        return 0;

    if (ssf->count == 0 && ssf->type == SFILE_FPTR && feof(ssf->file))
	return 1;
#endif

#if defined(HANDLE_SOCKET)
    if (ssf->type == SFILE_SOCKET)
	ssf->count = recv(ssf->socket, ssf->buffer, BUFSIZ, 0);
    else 
#endif
    {
	ssf->count = read(ssf->fd, ssf->buffer, BUFSIZ);
	ssf->buffer[ssf->count] = '\0';
    }

    ssf->nextchar = ssf->buffer;

    if (ssf->count < 0)
	return -1;
     else if (ssf->count == 0 && ssf->type == SFILE_FPTR && feof(ssf->file))
	 return 0;
     else
	 return 1;
}

int 
SFILECharReady(SFILE *sf)
{
	int i = 1;
	struct sfile *ssf = (struct sfile *)sf;

	if (ssf->count) return 1;

#if defined(DDX)
	if (ssf->fd == -1) return 0;
#endif

#if defined(HANDLE_SOCKET)

	if (ssf->isAtty)
	{
		int semi, nl, j;
		
		//while(ssf->count > 0 && *ssf->nextchar == 10)
		//{
		//	ssf->count --;
		//	ssf->nextchar --;
		//}

		if (ssf->count == 0)
		{
			ssf->nextchar = ssf->buffer;
			ssf->buffer[0] = '\0';
		}

		for (semi = nl = j = 0; j < ssf->count; j++)
		{
			if (ssf->nextchar[j] == ';') 
				semi ++;
			else if (semi && ssf->nextchar[j] == (char)10)
				nl ++;
		}

		i = 0;
		if (semi && nl)
			i = 1;
		else
		{
			while (kbhit())
			{
				int c = getche();
				if (c == 8)
				{
					if (ssf->count > 0) 
					{
						ssf->count --;
						ssf->nextchar[ssf->count] = '\0';
						putch(' ');
						putch(8);
					}
				}
				else 
				{
					if (c == ';')
					{
						semi ++;
					}
					else if (c == '\r')
					{
						i = 1;
						c = 10;
						putch(10);
					}

					ssf->nextchar[ssf->count++] = c;
					ssf->nextchar[ssf->count] = '\0';
				}
			}
		}
	}
	else
#endif
	{
	    int fd = SFILEfileno(sf);
	    struct timeval tv = {0, 0};  
	    fd_set fds;
	    FD_ZERO(&fds);
	    FD_SET(fd, &fds);
	    select(fd+1, &fds, NULL, NULL, &tv);
	    i = FD_ISSET(fd, &fds);
	    if (i)
	        if (_SFILE_load(sf) <= 0)
		    i = 0;
	}

	return i;
}

int
SFILEGetChar(SFILE *sf)
{
    int s;
    struct sfile *ssf = (struct sfile *)sf;

    if (ssf->count == 0)
    {
        _SFILE_load(sf);
	if (ssf->count == 0)
	    return EOF;
    }
	    
    ssf->count --;
    s = (int)(*ssf->nextchar ++);
    return s;
}

int
SFILEIoctl(SFILE *sf, int cmd, void *argp)
{
    struct sfile *ssf = (struct sfile *)sf;

#if defined(DDX)
    if (ssf->fd < 0)
        return 1;
#endif

#if defined(HANDLE_SOCKET)
    if (ssf->type == SFILE_SOCKET)
	return ioctlsocket(ssf->socket, cmd, argp);
    else
#endif
#if defined(HAVE_SYS_IOCTL_H)
    return ioctl(ssf->fd, cmd, argp);
#else
    return 1;
#endif
}
