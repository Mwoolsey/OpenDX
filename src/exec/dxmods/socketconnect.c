/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(intelnt) || defined(WIN32)

#include <dx/dx.h>

Error
m_SocketConnection(Object *in, Object *out)
{
    DXSetError(ERROR_NOT_IMPLEMENTED, "SocketConnect in MSVC build");
    out[0] = NULL;
    return ERROR;
}

#else

#include <pthread.h>
#include <dx/dx.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if 0
#include "../simconnect/simconnect.h"
#endif

struct _sc
{
    int        		port;
    int        		fd;
    Object     		obj;
    char * 		modid;
    struct  sockaddr_un usrv;
};

struct _args
{
    int 	fd;
    struct _sc	*sc;
};

extern Object _dxfImportBin_Buffer(void *, int *);

static HashTable ht = NULL;

static Error
rcvdata(int fd, Pointer p)
{
    struct _args *a = (struct _args *)p;
    int len, cfd;
    socklen_t ll;
    void *buf;
    struct _sc *sc = a->sc;

    if (a->fd != fd)
    {
	fprintf(stderr, "error... rcvdata got bad call\n");
	return ERROR;
    }

    if (sc->obj)
    {
	DXDelete(sc->obj);
	sc->obj = NULL;
    }

    cfd = accept(sc->fd, (struct sockaddr *)&sc->usrv, &ll);

    if (sizeof(len) < read(cfd, (void *)&len, sizeof(len)))
    {
	fprintf(stderr, "error... unable to read object length\n");
	return ERROR;
    }

    buf = (void *)DXAllocate(len);
    if (! buf)
    {
	fprintf(stderr, "error... unable to allocate read buf\n");
	return ERROR;
    }

    if (len < read(cfd, buf, len))
    {
	fprintf(stderr, "error... unable to read buf from sim\n");
	return ERROR;
    }

    close(cfd);

    sc->obj = _dxfImportBin_Buffer(buf, &len);

    DXFree((Pointer)buf);

    DXReadyToRun(sc->modid);

    return OK;
}

Error
m_SocketConnection(Object *in, Object *out)
{
    int port;
    struct _args *args;
    struct _sc *sc;

    if (ht == NULL)
    {
        ht = DXCreateHash(sizeof(struct _sc), NULL, NULL);
	if (! ht)
	    return ERROR;
    }

    if (!in[0] || !DXExtractInteger(in[0], &port))
    {
	DXSetError(ERROR_BAD_PARAMETER, "invalid port number");
	return ERROR;
    }

    sc = (struct _sc *)DXQueryHashElement(ht, (Key)&port);
    if (sc)
    {
        out[0] = sc->obj;
	sc->obj = NULL;
    }
    else
    {
        struct _sc s;
	int length;

	s.obj = NULL;
	s.modid = DXGetModuleId();
	s.port = port;
	s.fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (s.fd < 0)
	{
	    DXSetError(ERROR_INTERNAL, "unable to create socket");
	    return ERROR;
	}

	memset((char *)&s.usrv, 0, sizeof(s.usrv));
	s.usrv.sun_family = AF_UNIX;
	sprintf(s.usrv.sun_path,"/tmp/.DX-unix/DX%d", port);
	length = sizeof (s.usrv) - sizeof(s.usrv.sun_path) + strlen(s.usrv.sun_path);

	unlink(s.usrv.sun_path);

	if (0 > bind(s.fd, (struct sockaddr *)&s.usrv, length))
	{
	    DXSetError(ERROR_INTERNAL, "bind failed");
	    return ERROR;
	}

	if (0 > listen(s.fd, 0))
	{
	    DXSetError(ERROR_INTERNAL, "listen failed");
	    return ERROR;
	}

	if (! DXInsertHashElement(ht, (Element)&s))
	    return ERROR;

	sc = (struct _sc *)DXQueryHashElement(ht, (Key)&port);

	args = DXAllocate(sizeof(*args));
	args->fd = s.fd;
	args->sc = sc;

	DXRegisterInputHandler(rcvdata, s.fd, (void *)args);

    }

    return OK;
}
#endif
