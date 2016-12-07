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

#include <stdio.h>

#ifdef DXD_WIN
#include <sys/timeb.h>
#include <time.h>
#include <io.h>
#include <winioctl.h>
#else

#include <sys/time.h>
#include <sys/ioctl.h>
#endif

#include <errno.h>

#if HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include "config.h"
#include "packet.h"
#include "distp.h"


static char
*packtype[] =
{
    NULL,
    "$int",		/* PACK_INTERRUPT */
    "$sys",		/* PACK_SYSTEM */
    "$ack",		/* PACK_ACK */
    "$mac",		/* PACK_MACRODEF */
    "$for",		/* PACK_FOREGROUND */
    "$bac",		/* PACK_BACKGROUND */
    "$err",		/* PACK_ERROR */
    "$mes",		/* PACK_MESSAGE	*/
    "$inf",		/* PACK_INFO */
    "$lin",		/* PACK_LINQUIRY */
    "$lre",		/* PACK_LRESPONSE */
    "$dat",		/* PACK_LDATA */
    "$sin",		/* PACK_SINQUIRY */
    "$sre",		/* PACK_SRESPONSE */
    "$dat",		/* PACK_SDATA */
    "$vin",		/* PACK_VINQUIRY */
    "$vre",		/* PACK_VRESPONSE */
    "$dat",		/* PACK_VDATA */
    "$com",		/* PACK_COMPLETE */
    "$imp",		/* PACK_IMPORT */
    "$imi",		/* PACK_IMPORTINFO */
    "$lnk"		/* PACK_LINK */
};


static char 	**tmpbuffer = NULL;
static int	*tmpbuffersize = NULL;
static int	*tmpbufferused = NULL;
static lock_type *tmpbufferlock;
static int      locked = FALSE;

Error _dxf_ExInitPacket(void)
{
    tmpbufferlock = (lock_type *) DXAllocate (sizeof (lock_type));
    if (tmpbufferlock == NULL)
        return (ERROR);
    DXcreate_lock(tmpbufferlock, 0);

    tmpbuffer = (char **) DXAllocateZero(sizeof(char*));
    if(tmpbuffer == NULL)
        return ERROR;
    *tmpbuffer = NULL;
    tmpbuffersize = (int *) DXAllocateZero(sizeof(int));
    if(tmpbuffersize == NULL)
        return ERROR;
    tmpbufferused = (int *) DXAllocateZero(sizeof(int));
    if(tmpbufferused == NULL)
        return ERROR;
    return OK;
}

int
_dxf_ExNeedsWriting(void)
{
    int res;

    if (tmpbufferused == NULL)
	return 0;

    DXlock(tmpbufferlock, 0);
    res = (*tmpbufferused > 0);
    DXunlock(tmpbufferlock, 0);

    return res;
}

void
_dxf_ExCheckPacket(char *packet, int length)
{
    int    sts = 1;
    int    one = 1;
    int    zero = 0;

    if (_dxd_exSockFD != NULL)
    {
        /* It's possible we already have this locked on this processor if
         * _dxfCheckPacket needed to do a reallocate. The reallocate 
         * might call this routine again if a memory arena is expanded.
         * let's just ignore these messages.
         */
        if(locked) 
            return;
        DXlock(tmpbufferlock, 0);
        locked = TRUE;

	if (tmpbufferused && *tmpbufferused > 0)
	{
	    SFILEIoctl(_dxd_exSockFD, FIONBIO, &one);
	    sts = writeToSFILE(_dxd_exSockFD, *tmpbuffer, *tmpbufferused);
	    SFILEIoctl(_dxd_exSockFD, FIONBIO, &zero);
            if (sts > 0)
            {
		*tmpbufferused -= sts;
		if (*tmpbufferused > 0)
		    bcopy(*tmpbuffer + sts, *tmpbuffer, *tmpbufferused);
	    }

	} 
	if (length > 0) 
	{
	    if (sts > 0 && (!tmpbufferused || *tmpbufferused == 0))
	    {
		if(SFILEIoctl(_dxd_exSockFD, FIONBIO, &one) >= 0)
	        sts = writeToSFILE(_dxd_exSockFD, packet, length);
		SFILEIoctl(_dxd_exSockFD, FIONBIO, &zero);
		if (sts > 0) 
		{    
		    length -= sts;
		    packet += sts;
		}
	    }

	    if (tmpbufferused && length > 0)
	    {
		if (*tmpbuffersize < *tmpbufferused + length) 
		{
		    *tmpbuffer = (char *)DXReAllocate((Pointer)*tmpbuffer, 
						   *tmpbufferused+length);
		    *tmpbuffersize = *tmpbufferused+length;
		}
		bcopy(packet, *tmpbuffer + *tmpbufferused, length);
		*tmpbufferused += length;
	    }
	}

        DXunlock(tmpbufferlock, 0);
        locked = FALSE;
    }
}

/*
 * Format a packet so it can be transmitted to dxui
 */

Error _dxf_ExSPack (int type, int seqnumber, Pointer data, int len)
{
    /* char packet[MAX_UI_PACKET]; */
    char * packet;
    int length;
    int SLOP = 256;
    int bufSize;

    if ((! _dxd_exRemote) && (!_dxd_exRemoteSlave))
	return (OK);

    bufSize = len + SLOP;
    packet = (char*) DXAllocate( bufSize );
    if( !packet )
        _dxf_ExDie("Could not allocate memory for packet");

    if (data)
	length = snprintf (packet, bufSize, "|%d|%s|%d|%s|\n",
			        seqnumber, packtype[type], len, (char *)data);
    else
	length = snprintf (packet, bufSize, "|%d|%s|%d||\n",
			        seqnumber, packtype[type], 0);

    if( length >= bufSize ){
        _dxf_ExDie("Packet buffer not big enough.");
    }

    if(_dxd_exRemoteSlave) {
        UIPackage pck;
        pck.len = length;
        if(length >= MAX_UI_PACKET)
            length = MAX_UI_PACKET - 1;
        strncpy(pck.data, packet, length+1);
        _dxf_ExDistributeMsg(DM_UIPCK, (Pointer)&pck, sizeof(UIPackage), 
                             TOPEER0);
    }
    else 
        _dxf_ExCheckPacket(packet, length);

    DXFree(packet);

    return (OK);
}
