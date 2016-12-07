/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <dx/dx.h>
#include "config.h"
#include "log.h"
#include "packet.h"
#include "context.h"
#include "distp.h"
#include "utils.h"

static int 	lfd	= -1;
static FILE	*lfp	= NULL;

static int	isoutboard = FALSE;
static int	outsocket = -1;

extern Object _dxfExportBin_FP(Object o, int fd); /* from libdx/rwobject.c */

/*
 * Create the log file.  Note that the file name is currently hardcoded.
 * This MUST be done before forking.
 */

void _dxf_ExLogError (int errnum)
{

#if !defined(HAVE__ERRNO)
    errno = errnum;
#endif
    perror ("can't open dx.log");
}

int _dxf_ExLogOpen ()
{
    lfd = creat ("dx.log", 0666);
    if (lfd >= 0) 
	lfp = fdopen (lfd, "w");

    if(lfd < 0 || lfp == NULL)
        return(errno);
    else return(0);
}

/*
 * Close the log file.
 */

void _dxf_ExLogClose()
{
    if (lfd != -1)
    {
	close (lfd);
	if (lfp != NULL)
	    fclose (lfp);
    }
}


void
DXqmessage (char *input_who, char *message, va_list args)
{
    /*char	buf[MSG_BUFLEN];*/
    char	*buf;
    int		bufSize;
    int		spaceLeft;
    int		n2;
    char	*who, whobuf[512];
    int		n;
    int		ptype;
    int		level;
	int		mlen;

    who = input_who;

    if (who && ! _dxd_exRemote && ! _dxd_exRemoteSlave)
    {
	if (! strcmp (who, "IMAGE"))
	    return;

	if (! strcmp (who, "*IMAGE"))
	    return;
    }

    /* if we are not sending message to UI get rid of second keyword in
     * who. (ie MESSAGE POPUP, WARNING MSGERRUP...)
     */
    if (who && ! _dxd_exRemote) 
    {
        if (! strncmp(who, "MESSAGE", 7) || ! strncmp(who, "ERROR", 5) ||
            ! strncmp(who, "WARNING", 7) || ! strncmp(who, "LINK", 4))
        {
           /*
            * We need to make a copy of the input who, so that we can overwrite
            * it, because it maybe a string in the read-only text section
            * (i.e. aviion).
            * I'm not sure this is the correct way to do things, perhaps it is
            * better to do strncmp() below instead and not do the strcpy() here.
            *                                   David W. 2/14/95
            */
            char *cp;
            strcpy(whobuf,who);
            who = whobuf;
            cp = strchr(who, ' ');
            if (cp != NULL)
                *cp = '\0';
        }
    }

    bufSize = MSG_BUFLEN;
    buf=DXAllocate(bufSize);

    if (who && strcmp(who, "MESSAGE"))
    {
        if (*who == '*')
        {
            if (who[1] != '\0')
#if DXD_PRINTF_RETURNS_COUNT
                n = sprintf (buf, "%s:  ", who + 1);
#else
	    {
                sprintf (buf, "%s:  ", who + 1);
	        n = strlen(buf);
	    }
#endif
            else
                n = 0;
        }
        else
#if DXD_PRINTF_RETURNS_COUNT
                n = sprintf (buf, "%2d:  %s:  ", DXProcessorId (), who);
#else
	    {
                sprintf (buf, "%2d:  %s:  ", DXProcessorId (), who);
	        n = strlen(buf);
	    }
#endif
    }
    else
#if DXD_PRINTF_RETURNS_COUNT
	n = sprintf (buf, "%2d:  ", DXProcessorId ());
#else
	{
	    sprintf (buf, "%2d:  ", DXProcessorId ());
	    n = strlen(buf);
	}
#endif

    mlen = strlen(message);
    spaceLeft = bufSize - n - 1;
    n2 = vsnprintf(buf + n, spaceLeft, message, args);
    if( n2 >= spaceLeft ){
        bufSize += ( bufSize>n2 ? bufSize : n2 ) + 1;
        buf = DXReAllocate( buf, bufSize );
        spaceLeft = bufSize - n - 1;
	n2 = vsnprintf(buf + n, spaceLeft, message, args);
    }
    n += n2;
    
    buf[n] = '\n';
    buf[n+1] = '\000';

    /*
     * Put it into the log if it is open
     */

    if (lfd != -1)
	write (lfd, buf, n + 1);


    /*
     * If local then just put it on the screen.  Otherwise, for now we 
     * need to decide which packet type to send.
     *
     * NOTE:  If _dxd_exRemote, we only send n instead of n+1 so that we don't
     * bother with the trailing \n.
     */

    if (! who || ! strncmp(who, "MESSAGE", 7))
    {
	ptype = PACK_MESSAGE;
	level = 0;
    }
    else if (! strncmp (who, "ERROR", 5))
    {
	ptype = PACK_ERROR;
	level = 0;
    }
    else if (! strncmp (who, "LINK", 4))
    {
	ptype = PACK_LINK;
	level = 1;
    }
    else if (! strncmp (who, "WARNING", 7))
    {
	ptype = PACK_ERROR;
	level = 1;
    }
    else
    {
	ptype = PACK_INFO;
	level = 0;
    }

    if (level > _dxd_exErrorPrintLevel)
	return;

    if (isoutboard)
    {
	/* really in outboard module - send message back to master exec */
	Group g = DXNewGroup();
	String whoObj = NULL;
	String msgObj = NULL;
	Array oneObj = _dxfExNewInteger(1);

	if (who)
	    whoObj = DXNewString(who);
	else
	    whoObj = DXNewString("MESSAGE");

	buf[n] = '\0';               /* remove trailing newline */
	msgObj = DXNewString(buf);

	if (!g || !whoObj || !msgObj)
	    goto clean_up;

	if (!DXSetEnumeratedMember(g, 0, (Object)whoObj))
	    goto clean_up;
	whoObj = NULL;
	if (!DXSetEnumeratedMember(g, 1, (Object)msgObj))
	    goto clean_up;
	msgObj = NULL;

	if (!DXSetAttribute((Object)g, "message", (Object)oneObj))
	    goto clean_up;
	oneObj = NULL;

	if (!_dxfExportBin_FP((Object)g, outsocket))
	    goto clean_up;

clean_up:
	DXDelete((Object)g);
	DXDelete((Object)whoObj);
	DXDelete((Object)msgObj);
	DXDelete((Object)oneObj);

    }
    else if(_dxd_exRemoteSlave && !_dxd_exRemoteUIMsg) {
        UIMsgPackage pkg;
        /* send message back to master */
        if (_dxd_exContext != NULL ) {
            pkg.ptype = ptype;
            pkg.graphId = _dxd_exContext->graphId;
            pkg.len = n+1;
            /* copy all characters plus null at end of string */
            strncpy(pkg.data, buf, n+2);
            _dxf_ExDistributeMsg(DM_UIMSG, (Pointer)&pkg, 
                                 sizeof(UIMsgPackage), TOPEER0);
        }
    }
    else if (_dxd_exContext != NULL)
	_dxf_ExQMessage(ptype, _dxd_exContext->graphId, n+1, buf);

    DXFree(buf);
}


void 
_dxf_ExQMessage(int ptype, int graphId, int len, char *buf)
{
    if (! _dxd_exRemote)
    {
	write (fileno(stdout), buf, len);
    }
    else
    {
	char		*c;

        if (_dxd_exTerminating != NULL )
          if (*_dxd_exTerminating)  /* if terminating socket to UI */ 
            return;                 /* already destroyed           */

	buf[len-1] = '\000';

	while ((c = strchr (buf, '\n')) != 0)
	    *c = ' ';

        _dxf_ExSPack (ptype, graphId, buf, len-1);
    }
}		

Error _dxfSetOutboardMessage(int OutSockFD)
{
    isoutboard = TRUE;
    outsocket = OutSockFD;
    return OK;
}

Error _dxfResetOutboardMessage()
{
    isoutboard = FALSE;
    outsocket = -1;
    return OK;
}

/* dummy routines to make libdx message.c code happy. */
Error
_dxf_initmemqueue()
{
    return OK;
}

void _dxfemergency()
{
}

/* called to try to force out any pending messages. */
void
DXqflush()
{
    if (lfp != NULL)
	fflush(lfp);
}
