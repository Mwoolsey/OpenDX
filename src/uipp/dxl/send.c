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

#if defined(HAVE_STRING_H)
#include <string.h>
#endif

#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

#if defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if   defined(OS2)  || defined(DXD_HAS_WINSOCKETS)
#if defined(HAVE_IO_H)
#include <io.h>
#endif
#define write(a,b,c) _dxl_os2_send(a,b,c,0)
#define read(a,b,c) _dxl_os2_recv(a,b,c,0)
#endif

#include "dxlP.h"

typedef int (*DXLPacketPredicate)(DXLConnection *, DXLEvent *, void *);

static int _dxl_ReadFromSocket(DXLConnection *conn);
static int _dxl_WaitForReadable(DXLConnection *conn);
static int _dxl_IsReadable(DXLConnection *conn);
static void PrintEvent(DXLEvent *e);

static DXLError DXLGetPacket(DXLConnection *, DXLPacketTypeEnum, int,
                DXLEvent *, int,
                DXLPacketPredicate, const void *);

#define STRLEN(A) (A ? strlen(A) : 0)

char *_DXLPacketTypes[] =
{
    NULL,
    "$int",             /* PACK_INTERRUPT */
    "$sys",             /* PACK_SYSTEM */
    "$ack",             /* PACK_ACK */
    "$mac",             /* PACK_MACRODEF */
    "$for",             /* PACK_FOREGROUND */
    "$bac",             /* PACK_BACKGROUND */
    "$err",             /* PACK_ERROR */
    "$mes",             /* PACK_MESSAGE */
    "$inf",             /* PACK_INFO */
    "$lin",             /* PACK_LINQUIRY */
    "$lre",             /* PACK_LRESPONSE */
    "$dat",             /* PACK_LDATA */
    "$sin",             /* PACK_SINQUIRY */
    "$sre",             /* PACK_SRESPONSE */
    "$dat",             /* PACK_SDATA */
    "$vin",             /* PACK_VINQUIRY */
    "$vre",             /* PACK_VRESPONSE */
    "$dat",             /* PACK_VDATA */
    "$com",             /* PACK_COMPLETE */
    "$imp",             /* PACK_IMPORT */
    "$imi",             /* PACK_IMPORTINFO */
    "$lnk"              /* PACK_LINK */
};

int _DXLNumPackets = (sizeof(_DXLPacketTypes)/sizeof(_DXLPacketTypes[0]));

DXLEvent*
DXLNewEvent(int serialId, DXLPacketTypeEnum ptype, const char *msg)
{
    int msgsize = 0;

    DXLEvent *event = (DXLEvent*)malloc(sizeof(DXLEvent));
    if (!event)
        return NULL;

    event->serial = serialId;
    event->type = ptype;
    if (msg) {
        msgsize = strlen(msg) + 1;
        if (msgsize > DXL_EVENT_BUF_SIZE)
            event->contents = malloc(msgsize*sizeof(char));
        else
            event->contents = event->buffer;
        strcpy(event->contents,msg);
    } else {
        event->contents = event->buffer;
        event->buffer[0] = '\0';
    }
    event->contentsSize = msgsize;
    event->next = NULL;
    return event;
}

DXLError
DXLClearEvent(DXLEvent *event)
{
    if (event->contents && event->contents != event->buffer)
        free(event->contents);
    event->next = NULL;
    event->contentsSize = 0;
    event->buffer[0] = '\0';
    event->contents = event->buffer;
    return OK;
}
DXLError
DXLFreeEvent(DXLEvent *event)
{
    if (DXLClearEvent(event))
        free(event);
    return OK;
}
DXLError
DXLCopyEvent(DXLEvent *dst, DXLEvent *src)
{
    if (src->contentsSize > DXL_EVENT_BUF_SIZE)
        dst->contents = malloc(src->contentsSize);
    else
        dst->contents = dst->buffer;

    dst->contentsSize = src->contentsSize;

    memcpy(dst->contents, src->contents, src->contentsSize);

    return OK;
}

DXLError
DXLSend(DXLConnection *conn, const char *string)
{
    DXLPacketTypeEnum ptype=PACK_FOREGROUND;

    if (conn->dxuiConnected) {
	if (conn->majorVersion <= 2)
	    ptype = PACK_FOREGROUND;
	else
	    ptype = PACK_LINK;
	return DXLSendPacket(conn, ptype, string) >= 0 ? OK : ERROR;
    }
    else if (conn->macroDef)
    {
	DXLSendUnpacketized(conn, string);
	return OK;
    }
    else
    {
	ptype = PACK_FOREGROUND;
	return DXLSendPacket(conn, ptype, string) >= 0 ? OK : ERROR;
    }

}


#ifdef DEBUG_MESSAGES
static int first = 1;
static FILE *dbg = NULL;
#endif

int     /* packet id that was sent, -1 on error */
DXLSendPacket(DXLConnection *conn, DXLPacketTypeEnum ptype, const char *string)
{
    int l = STRLEN(string);
    char *buffer = malloc(l + 50);
    int packetId, sts     = 0;
    int written = 0;

    if (!conn || conn->fd < 0 || (ptype > _DXLNumPackets) || (ptype <= 0))
        return ERROR;

    packetId = conn->id++;


    if (ptype == PACK_MACRODEF)
        l = SPRINTF(buffer,"|%d|%s|0|\n", packetId, _DXLPacketTypes[ptype]);
    else if (ptype == PACK_INTERRUPT)
    {
        l = SPRINTF(buffer,"|%d|%s|1|0|\n", packetId, _DXLPacketTypes[ptype]);
    }
    else if (l == 0)
        l = SPRINTF(buffer,"|%d|%s|0||\n", packetId, _DXLPacketTypes[ptype]);
    else
        l = SPRINTF(buffer,
            "|%d|%s|%d|%s|\n", packetId, _DXLPacketTypes[ptype], l, string);
    
#ifdef DEBUG_MESSAGES
    if (first)
    {
	first = 0;
	dbg = fopen("debug", "w");
    }
    else
	dbg = fopen("debug", "a");

    fprintf(dbg, "--> %s", buffer);
    fclose(dbg);
#endif

    do {
        if (_dxl_ReadFromSocket(conn) < 0)
        {
            sts = -1;
            break;
        }
        if ((sts == 0) &&  conn->debugMessaging)
            fprintf(stderr, "Sending -> %s\n",buffer);
        sts = write(conn->fd, buffer + written, l);
        l -= sts;
        written += sts;
    }
    while (sts > 0 && l > 0);

    free(buffer);
    if (sts < 0) {
        _dxl_InvalidateSocket(conn);
	packetId = -1;
	goto done;
    } else if (ptype != PACK_MACRODEF && conn->synchronous) {
        if (!DXLSync(conn)) {
	    packetId = -1;
	    goto done;
	}
    }

done:

    return packetId;
}


int
DXLQuery(DXLConnection *conn, const char *string,
         const int length, char *result)
{
    int found;
    int requestId;
    DXLEvent    event;
    DXLPacketTypeEnum ptype;


    if (conn->fd < 0)
        goto error;


    if (conn->dxuiConnected)
    {
        if (conn->majorVersion <= 2)
            ptype = PACK_FOREGROUND;
        else
            ptype = PACK_LINK;
    }
    else
    {
        /*
         * FIXME: This probably is not the correct code,
         * but it serves as a marker.
         */
        ptype = PACK_FOREGROUND;

        fprintf(stderr,"DXLQuery() not implemented for dexec connections\n");
        goto error;
    }

    requestId = DXLSendPacket(conn,ptype,string);
    if (requestId < 0)
        goto error;

    found = 0;
    if (conn->dxuiConnected)  {
        if (DXLGetPacketId(conn, PACK_LRESPONSE, requestId, &event))
            found = 1;
    } else {
        /* include "filename" causes completes to come back that match
         * the zero'th sync
         */
        if (DXLGetPacketId(conn, PACK_COMPLETE, 0, &event))
            found =  1;
    }

    if (!found)
    {
        _dxl_InvalidateSocket(conn);
        DXLClearEvent(&event);
        goto error;
    }

    strncpy(result, event.contents, length);

    DXLClearEvent(&event);

    return length;

error:
    return -1;
}

static void RemoveEvent(DXLConnection *conn, DXLEvent *e)
{
    DXLEvent *event, *last_event = NULL;

    for (event = conn->events; event ; event = event->next) {
        if (event == e)
            break;
        last_event = event;
    }

    if (event == e) {
        if (e == conn->events) {
            /*
             * First item on list
             */
            conn->events = e->next;
            if (e == conn->lastEvent)   /* ...and last item */
                conn->lastEvent = NULL;
        } else if (e == conn->lastEvent) {
            /*
             * Last item on list
             */
            conn->lastEvent = last_event;
            last_event->next = NULL;
        } else {
            /*
             * Middle item on list
             */
            last_event->next = e->next;
        }
    }

    conn->nEvents--;
}
static void AppendEvent(DXLConnection *conn, DXLEvent *e)
{
    if (conn->events == NULL) {
        conn->events = e;
    } else {
        conn->lastEvent->next = e;
    }
    conn->lastEvent = e;
    conn->nEvents++;
    e->next = NULL;     /* Just to be sure */

    /*
     * We've pulled an event off the socket and onto the local queue.
     * If the event handler is dependent on a select() on the socket to
     * determine that there's something to do, it won't find anything.
     * So we install an idle-time event processor that will check for 
     * something to do when the system is otherwise idle.
     */
    _dxf_InstallIdleEventHandler(conn);
}


DXLError
DXLNextPacket(DXLConnection *conn, DXLEvent *event)
{
    while(conn->nEvents == 0)
    {
        _dxl_WaitForReadable(conn);
        if (_dxl_ReadFromSocket(conn) == 0)
            return ERROR;
    }

    if (conn->events->type == PACK_ERROR)
        _DXLError(conn, conn->events->contents);

    DXLCopyEvent(event, conn->events);

    RemoveEvent(conn, conn->events);
    DXLFreeEvent(conn->events);

    return OK;
}

int
DXLWaitForEvent(DXLConnection *conn)
{
    if(conn->nEvents == 0)
	_dxl_WaitForReadable(conn);
    return OK;
}
 


static void DXLDispatchEvent(DXLConnection *conn, DXLEvent *e)
{
    RemoveEvent(conn,e);
    if (conn->debugMessaging) {
        fprintf(stderr, "Dispatching <- ");
        PrintEvent(e);
    }
    _dxl_HandleMessage(conn, e->type, e->serial, e->contents);
}

DXLError
DXLProcessEventList(DXLConnection *conn)
{
    DXLEvent *e;

    while (_dxl_IsReadable(conn))
        if (_dxl_ReadFromSocket(conn) == 0)
            return ERROR;

    while (NULL != (e = conn->events))
    {
        DXLDispatchEvent(conn,e);
        DXLFreeEvent(e);
    }

    conn->nEvents = 0;

    /* 
     * Now we have cleared all the input from both the socket and the
     * input queue in conn, so we don't need an idle-time input check.
     */
    _dxf_ClearIdleEventHandler(conn);

    return OK;
}


/*
 * Starting with the given event, search the event list for the event
 * matching the given type and requestId.  If requestId is < 0 then we
 * don't try and match the requestId.  Further, if pp is not NULL then
 * we also try and match the event with a call to (pp)(conn,event,arg).
 *
 */
static DXLEvent*
DXLFindEventInList(DXLConnection *conn, DXLEvent *event,
                DXLPacketTypeEnum type, int requestId,
                DXLPacketPredicate pp, const void *arg)
{
    DXLEvent *e;

    for (e=event ; e ; ) {
        if ((e->type == type) &&
            ((requestId < 0) || (e->serial == requestId)) &&
            (!pp || (pp)(conn,e,(char *)arg)))
            break;
        e = e->next;
    }
    return e;
}

/*
 * Get the packet specified by the given packet type and the given id.
 * If id is <= 0, then we don't try and match the ids with packets.
 * If pp is not NULL, then we call it on the event (pp(conn,e,arg)),
 * to see if it is a match.
 * If we find an event with a match, given the above criteria, then
 * we copy the event into the user structure .
 */
static int
DXLCheckPacket(DXLConnection *conn, DXLPacketTypeEnum type, int requestId,
                DXLEvent *event, DXLPacketPredicate pp, const void *arg)
{
    DXLEvent *e;

    e = DXLFindEventInList(conn,conn->events,type,requestId, pp, arg);
    if (e) {
        DXLCopyEvent(event, e);
        return 1;
    }
    return 0;
}
/*
 * Get the packet specified by the given packet type and the given id.
 * If id is <= 0, then we don't try and match the ids with packets.
 * If pp is not NULL, then we call it on the event (pp(conn,e,arg)),
 * to see if it is a match.
 * If we find an event with a match, given the above criteria, then
 * we copy the event into the user structure and if 'remove' is non-zero
 * we remove the event from the event list.
 */
static DXLError
DXLGetPacket(DXLConnection *conn, DXLPacketTypeEnum type, int requestId,
                DXLEvent *event, int remove,
                DXLPacketPredicate pp, const void *arg)
{
    DXLEvent *e;

    e = DXLFindEventInList(conn,conn->events,type,requestId, pp, arg);

    while (!e)
    {
        e = conn->lastEvent;

        if (! _dxl_WaitForReadable(conn))
            return ERROR;

        if (! _dxl_ReadFromSocket(conn))
            return ERROR;

        if (!e)
          e = conn->events;
        else
          e = e->next;
#if 0
        PrintEventList(e); fprintf(stderr, "\n");
#endif
        e = DXLFindEventInList(conn,e,type,requestId, pp, arg);
    }

    DXLCopyEvent(event, e);

    if (remove) {
        RemoveEvent(conn, e);
        DXLFreeEvent(e);
    }

    return OK;
}

static int MatchString(DXLConnection *conn, DXLEvent *e, void *arg)
{
    char *str = (char*)arg;

    if (!str || strstr(e->contents,str))
        return 1;
    else
        return 0;
}
/*
 * Find and read until a packet with the given type, packet id, and string
 * contained in the message is found.  This may block on the socket.
 * Return the found event in the given event, and remove the event
 * from the event list.
 */
DXLError
DXLGetPacketIdString(DXLConnection *conn, DXLPacketTypeEnum type,
                int requestId, const char *str, DXLEvent *event)
{
    return DXLGetPacket(conn,type, requestId,event, 1, MatchString,str);
}
/*
 * Find and read until a packet with the given type and string
 * contained in the message is found.  This may block on the socket.
 * Return the found event in the given event, and remove the event
 * from the event list.
 */
DXLError
DXLGetPacketString(DXLConnection *conn, DXLPacketTypeEnum type,
                const char *str, DXLEvent *event)
{
    return DXLGetPacket(conn,type,-1,event, 1, MatchString,str);
}
/*
 * Find a packet in the current event list with the given type and string
 * contained in the message.  Do not block on the socket.
 * Return the found event in the given event, and do not remove the event
 * from the event list.  Return 0 if a matching packet was not found,
 * otherwise non-zero.
 */
int
DXLCheckPacketString(DXLConnection *conn, DXLPacketTypeEnum type,
                const char *str, DXLEvent *event)
{
    return DXLCheckPacket(conn,type,-1,event, MatchString,str);
}
/*
 * Find and read until a packet with the given type and  string contained
 * in the message is found.
 * Return the found event in the given event, and do not remove the event
 * from the event list.
 */
DXLError
DXLWaitPacketString(DXLConnection *conn, DXLPacketTypeEnum type,
                const char *str, DXLEvent *event)
{
    return DXLGetPacket(conn,type,-1,event, 0, MatchString,str);
}
/*
 * Find and read until a packet with the given type and  id is found.
 * Return the found event in the given event, and remove the event
 * from the event list.
 */
DXLError
DXLGetPacketId(DXLConnection *conn, DXLPacketTypeEnum type, int id,
                                DXLEvent *event)
{
    return DXLGetPacket(conn,type,id,event, 1, NULL,NULL);
}


DXLError
DXLPeekPacket(DXLConnection *conn, DXLEvent *event)
{
    if (conn->nEvents == 0)
        _dxl_ReadFromSocket(conn);

    if (conn->nEvents == 0)
        return ERROR;

    DXLCopyEvent(event, conn->events);

    return OK;
}

static int
_dxl_IsReadable(DXLConnection *conn)
{
#ifndef OS2
    fd_set fds;
#endif
#ifdef hp700
    int width = MAXFUPLIM;
#else
#ifdef aviion
    int width = NOFILE;
#else
#ifdef solaris
    int width = FD_SETSIZE;
#else
#ifdef DXD_HAS_WINSOCKETS
    int width = FD_SETSIZE;
#else
#ifndef OS2
    int width = getdtablesize();
#endif
#endif
#endif
#endif
#endif
    int retval = 0;

    struct timeval to;

    if (conn->fd < 0)
	return 0;

#if  !defined(OS2)  && !defined(DXD_HAS_WINSOCKETS)
    FD_ZERO(&fds);
    FD_SET(conn->fd, &fds);
    to.tv_sec = 0;
    to.tv_usec = 0;

    /* this will restart select if it has been interrupted by a signal */
	SELECT_INTR((conn->fd + 1), (SELECT_ARG_TYPE *)&fds, NULL, NULL, &to, retval);
#endif

#ifdef OS2
    retval = select(&conn->fd, 1, 0, 0, 0);
#endif

#ifdef DXD_HAS_WINSOCKETS
    to.tv_sec = 0;
    to.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(conn->fd, &fds);
    retval =  select(conn->fd, &fds, NULL, NULL, &to);
#endif

    if (retval > 0) return TRUE;
    if (retval == 0) return FALSE;

    /* retval < 0 */
    _dxl_InvalidateSocket (conn);
    return FALSE;
}

int
DXLIsMessagePending(DXLConnection *conn)
{
    return (conn->nEvents > 0) || _dxl_IsReadable(conn);
}


static int
_dxl_WaitForReadable(DXLConnection *conn)
{
    int retval;
#ifndef OS2
    fd_set fds;
#endif
#ifdef hp700
    int    width = MAXFUPLIM;
#else
#ifdef aviion
    int    width = NOFILE;
#else
#ifdef solaris
    int    width = FD_SETSIZE;
#else
#ifdef DXD_HAS_WINSOCKETS
    int    width = FD_SETSIZE;
#else
#ifndef OS2
    int    width = getdtablesize();
#endif
#endif
#endif
#endif
#endif

    if (conn->fd < 0)
	return 0;

#if  !defined(OS2)  && !defined(DXD_HAS_WINSOCKETS)
    FD_ZERO(&fds);
    FD_SET(conn->fd, &fds);

    /* this will restart select if it was interrupted by a signal */
	SELECT_INTR((conn->fd + 1), (SELECT_ARG_TYPE *)&fds, NULL, NULL, NULL, retval);
    if(retval < 0)
        goto error;
#endif

#ifdef OS2
    if (select(&conn->fd, 1, 0, 0, -1)<0)
        goto error;
#endif

#ifdef DXD_HAS_WINSOCKETS
    FD_ZERO(&fds);
    FD_SET(conn->fd, &fds);
	width =  select((conn->fd + 1), &fds, NULL, NULL, NULL);
    if (width < 0)
        goto error;

#endif
    return 1;

error:
   _dxl_InvalidateSocket(conn);
    return 0;
}

#if 0
static int lock_me = 0;
#endif

static int
_dxl_ReadFromSocket(DXLConnection *conn)
{
    int    id, typei;
    char   type[8];
    int    nbytes;
    
    char   *start;
    int    nRemaining;

    char   *buffer;
    char   *msgbuf;
    int    bufSize = 4096;
    int    mbufSize = 4096;

    if (conn->fd < 0)
	return 0;

    buffer = (char*) malloc(bufSize);
    msgbuf = (char*) malloc(mbufSize);

    while (_dxl_IsReadable(conn)) {
	int nread;

	nRemaining = conn->nLeftOver;

	while( nRemaining+1025 > bufSize ){
	   char* tmp = buffer;
	   bufSize *= 2;
	   buffer = (char*) malloc( bufSize );
	   free(tmp);
	}

        if (nRemaining) {
	    memcpy (buffer, conn->leftOver, nRemaining);
            nread = read(conn->fd, buffer+nRemaining, 1024);
        } else {
            nread = read(conn->fd, buffer, 1024);
        }
	if (nread <= 0)
	    goto error;
	nRemaining+= nread;
	buffer[nRemaining] = '\0';

        start = buffer;
	/*
	 * We used to check done here, but now we'll just keep reading
	 * from start until there isn't anything left to read.
	 */
        while (1) {
            int len, items;
	    DXLEvent *event;
	    
	    int msgbytes_in_buffer;
	    int bytes_used;
	    short message_completed;

	    /* 
	     * sscanf() leaves 'len' unset if the last '|' was not seen 
	     */
	    len = -1;
	    items = sscanf(start, "|%d|%[^|]|%d|%n", &id, type, &nbytes, &len);
	    /*
	     * Perform 2 checks to see if we have enough bytes in the buffer
	     * to form a complete message.  First check to see if the sscanf
	     * completed.  Then check to see if the number of bytes resulting
	     * completes a message.  In order to complete a message it has
	     * to get the trailing |\n.
	     */
            if ((items != 3) || (len == -1)) {
		message_completed = 0;
            } else {
		msgbytes_in_buffer = nRemaining - len;
		if ((nbytes+2) > msgbytes_in_buffer) {
		    message_completed = 0;
		} else {
		    message_completed = 1;
		}
	    } 
	    if (!message_completed) {
		if (nRemaining >= conn->allocdLeftOver) {
		    free(conn->leftOver);
		    /* malloc 1.5 times nRemaining rounded to the nearest 16 */
		    conn->allocdLeftOver = 16 + ((nRemaining + (nRemaining>>1)) & (~15));
		    conn->leftOver = malloc(conn->allocdLeftOver);
		    if (!conn->leftOver) 
			goto error;
		}


		strncpy(conn->leftOver, start, nRemaining);
		conn->nLeftOver = nRemaining;
		break;
	    }

	    for (typei = 1; typei < _DXLNumPackets; typei++)
		if (! strcmp(type, _DXLPacketTypes[typei]))
		    break;

	    if (typei == _DXLNumPackets) {
		fprintf (stderr, "%s[%d] packet error\n", __FILE__,__LINE__);
		goto error;
	    }

	    bytes_used = (len + nbytes + 2);   /* 2 = "|\n" */

	    while( nbytes+2 > mbufSize ){
	       char* tmp = msgbuf;
	       mbufSize *= 2;
	       msgbuf = (char*) malloc( mbufSize );
	       free(tmp);
	    }

	    strncpy(msgbuf, start+len, nbytes + 2);
	    if (bytes_used > nRemaining)
		goto error;

	    if (msgbuf[nbytes] != '|') {
		fprintf (stderr, "%s[%d]packet error\n", __FILE__,__LINE__);
		goto error;
	    }

	    nRemaining -= bytes_used;
	    start += bytes_used;

	    msgbuf[nbytes] = '\0';

	    event = DXLNewEvent(id, (DXLPacketTypeEnum) typei,msgbuf);
	    if (!event)
		goto error;
	    if (conn->debugMessaging) {
		fprintf(stderr, "Recving <- ");
		PrintEvent(event);
	    }
	    AppendEvent(conn,event);
        }
    }

    free(buffer);
    free(msgbuf);

    return 1;

error:

    free(buffer);
    free(msgbuf);

   _dxl_InvalidateSocket(conn);
    return 0;
}

DXLError DXLSendUnpacketized(DXLConnection *conn, const char *string)
{
    int length, r;
    char *newString;

    if (conn->fd < 0)
        return ERROR;

    length = STRLEN(string) + 1;
    newString = malloc((length + 1) * sizeof(char));
    strcpy(newString, string);
    newString[length-1] = '\n';
    newString[length]   = '\0';

    if (conn->debugMessaging)
        fprintf(stderr, "Sending -> %s", newString);

    
#ifdef DEBUG_MESSAGES
    if (first)
    {
	first = 0;
	dbg = fopen("debug", "w");
    }
    else
	dbg = fopen("debug", "a");
    
    fprintf(dbg, "Unpacketized --> %s", newString);

    fclose(dbg);
#endif

    if (write(conn->fd, newString, length) != length)
        r = ERROR;
    else
        r = OK;

    free(newString);
    return r;
}

DXLError DXLSendImmediate(DXLConnection *conn, const char *string)
{
    int length, r;
    char *newString;

    if (conn->fd < 0)
        return ERROR;

    length = STRLEN(string) + 1;
    newString = malloc((length + 1) * sizeof(char));

    strcpy(newString, "$");
    strcat(newString, string);

    r = DXLSendUnpacketized(conn,newString);

    if (r == OK && conn->synchronous)
        DXLSync(conn);

    free(newString);

    return r;
}

DXLError
DXLHandlePendingMessages(DXLConnection *conn)
{

   DXLProcessEventList(conn);
   while (_dxl_IsReadable(conn))
   {
       _dxl_ReadFromSocket(conn);
       DXLProcessEventList(conn);
   }

   return OK;
}
static void PrintEvent(DXLEvent *e)
{
    fprintf(stderr, "id %d type %d %s\n", e->serial, e->type, e->contents);
}

int  DXLSetMessageDebugging(DXLConnection *conn, int on)
{
    int oldval = conn->debugMessaging;
    conn->debugMessaging = on;
    return oldval;
}

/* DW D Watson added to end of file */

DXLError
exDXLBeginMacroDefinition(DXLConnection *conn, const char *mhdr)
{
    int sts = 0;
    DXLPacketTypeEnum ptype;

    if (conn->dxuiConnected)
    {
        return ERROR;
    }
    else
    {
        conn->macroDef = TRUE;
        ptype = PACK_MACRODEF;
        sts = DXLSendPacket(conn,ptype,0);
        if (sts < 0)
          return ERROR;
        DXLSendUnpacketized(conn, mhdr);
        DXLSendUnpacketized(conn, "{\n");

    }
    return OK;

}

DXLError
exDXLEndMacroDefinition(DXLConnection *conn)
{
    int sts = 0;

    if (conn->dxuiConnected)
    {
        return ERROR;
    }
    else
    {
        if (!conn->macroDef) {
          fprintf(stderr,"exDXLEndMacroDefinition: not valid before exDXLBeginMacroDefinition()\n");
          return ERROR;
        } /* endif */
        conn->macroDef = FALSE;
        sts = DXLSendUnpacketized(conn,"}\n|\n");
        if (sts < 0)
          return ERROR;

    }

    if (conn->synchronous)
        if (!DXLSync(conn))
            return ERROR;

    return OK;
}
