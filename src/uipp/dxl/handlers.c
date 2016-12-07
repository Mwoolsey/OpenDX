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

#include <ctype.h>
#include "string.h"
#include "dxlP.h"


static void DeleteHandlerEntry(HandlerEntry he)
{
    if (he->operand)
	FREE(he->operand);
    FREE(he);
}
static HandlerEntry NewHandlerEntry(int packetId, const char *str, 
				DXLMessageHandler h, const void *data)
{
    HandlerEntry he;

    he = (HandlerEntry)MALLOC(sizeof(struct handlerEntry));

    if (! he)
	return NULL;	

    if (str)
    {
	he->operandLength = strlen(str);
	he->operand = (char *)MALLOC(he->operandLength+1);
	if (! he->operand)
	    goto error;
	strcpy(he->operand,str);
    }
    else
    {
	he->operandLength = 0;
	he->operand = NULL;
    }

    he->packetId = packetId;
    he->handler = h;
    he->data = data;
    he->next = NULL; 

    return he;

error:
    if (he) {
	DeleteHandlerEntry(he);
    }
    return 0;
}
/*
 * Set a user error handler
 */
DXLError
DXLSetErrorHandler(DXLConnection *conn, DXLMessageHandler handler, 
					const void *data)
{
    return DXLSetMessageHandler(conn, PACK_ERROR,
			NULL, (DXLMessageHandler)handler, data);
}

/*
 * Internal routines to initialize the message handler
 * structures in the connection
 */

static DXLError
_dxl_InitMessageHandler_1(HandlerEntry *msgHandler)
{
    int i;

    for (i = 0; i <= PACKET_TYPE_COUNT; i++)
	msgHandler[i] = NULL;

    return OK;
}

DXLError
_dxl_InitMessageHandlers(DXLConnection *conn)
{
    if (! _dxl_InitMessageHandler_1(conn->userMessageHandlers))
	return ERROR;

    if (! _dxl_InitMessageHandler_1(conn->systemMessageHandlers))
	return ERROR;

    return OK;
}


DXLError
_dxl_FreeMessageHandler_1(HandlerEntry *msgHandler)
{
    int i;

    for (i = 0; i <= PACKET_TYPE_COUNT; i++)
    {
	HandlerEntry next, this;
	
	for (this = msgHandler[i]; this; this = next)
	{
	    next = this->next;
	    DeleteHandlerEntry(this);

	}

	msgHandler[i] = NULL;
    }

    return OK;
}

/*
 * Internal routines to free up the message handler
 * structures in the connection
 */
DXLError
_dxl_FreeMessageHandlers(DXLConnection *conn)
{
    if (conn->userMessageHandlers &&
	! _dxl_FreeMessageHandler_1(conn->userMessageHandlers))
	return ERROR;

    if (conn->systemMessageHandlers &&
	! _dxl_FreeMessageHandler_1(conn->systemMessageHandlers))
	return ERROR;

    return OK;
}

/*
 * See if the given HandlerEntry matches the given packetId and string.
 * If the packetId is -1, then it matches any packetId.   
 * If str is NULL, then it matches any string.
 * 'partial' is used to indicate if a full match of 'str' is required, if
 * not (partial is non-zero), then we use he->operandLength when matching the
 * string instead of the whole string. 
 *
 */
static int match_packet(HandlerEntry he, 
		int packetId, const char *str, int partial)
{
    if ((he->packetId >= 0) && (he->packetId != packetId))
	return 0;

    if (he->operand == NULL)
	return 1;

    if (str == NULL)
	return 0;

    if (partial)
	return !strncmp(str,he->operand,he->operandLength);
    else
	return !strcmp(str,he->operand);

}

/*
 * Generic routine to remove a handler from a message
 * handler structure.
 * Returns OK on success 
 */
static DXLError
_dxl_RemoveHandler(HandlerEntry *msgHandler,
		int type, int packetId, const char *str, DXLMessageHandler h)
{
    HandlerEntry prev = NULL, he;

    for (he = msgHandler[type]; he; ) {
	if (match_packet(he,packetId, str, 0))
	    break;
	prev = he;
	he = he->next;
    }

    if (he) {
	if (prev)	/* he is not the first item on list */
	    prev->next = he->next;
	else	/* he is first item on list */
	    msgHandler[type] = he->next;

	DeleteHandlerEntry(he);
    }

    return he == NULL ? ERROR : OK; 
}
/*
 * Generic routine to install a handler into a message
 * handler structure.
 * If packetId == -1, then it is not used in trying to match packets.
 */
static DXLError
_dxl_SetHandler(HandlerEntry *msgHandler,
		int type, int packetId, const char *str, 
		DXLMessageHandler h, const void *data)
{
    HandlerEntry he;

    if (!h)
	return ERROR;

    for (he = msgHandler[type]; he; he = he->next) {
	if (((str && he->operand && !strcmp(str, he->operand)) ||
	    (!str && !he->operand)) &&
	    (packetId == he->packetId))
	    break;
    }
    
    if (he)
    {
	he->handler = h;
        he->data = data;
    }
    else
    {
	he = NewHandlerEntry(packetId,str,h,data);
	if (!he)
	    return ERROR;
	he->next = msgHandler[type];
	msgHandler[type] = he;
    }

    return OK;

}

/*
 * Internal routines to install handler into the specified
 * message handler structure for an arbitrary message
 * type
 */
static DXLError
_dxl_SetUserHandler(DXLConnection *conn,
		int type, const char *str, 
		DXLMessageHandler h, const void *data)
{
    return _dxl_SetHandler(conn->userMessageHandlers, type, -1, str, h, data);
}


DXLError
_dxl_SetSystemHandlerString(DXLConnection *conn,
		int type, const char *str, 
		DXLMessageHandler h, const void *data)
{
    return _dxl_SetHandler(conn->systemMessageHandlers, type, -1, str, h, data);
}
DXLError
_dxl_SetSystemHandlerId(DXLConnection *conn,
		int type, int packetId, 
		DXLMessageHandler h, const void *data)
{
    return _dxl_SetHandler(conn->systemMessageHandlers, type, 
				packetId, NULL, h, data);
}
DXLError
_dxl_SetSystemHandler(DXLConnection *conn,
		int type, int packetId, const char *str, 
		DXLMessageHandler h, const void *data)
{
    return _dxl_SetHandler(conn->systemMessageHandlers, 
			type, packetId, str, h, data);
}

/*
 * Set a user handler for an arbitrary message type.  Puts
 * the handler into the user message handler structure.
 */
DXLError
DXLSetMessageHandler(DXLConnection *conn, DXLPacketTypeEnum type, 
			const char *str, DXLMessageHandler h, const void *data)
{
   return _dxl_SetUserHandler(conn, type, str, h, data);
}


/*
 * Internal routines to remove handler from the specified
 * message handler structure for an arbitrary message
 * type
 */
DXLError
_dxl_RemoveSystemHandler(DXLConnection *conn,
		int type, int packetId, const char *str, 
		DXLMessageHandler h)
{
    return _dxl_RemoveHandler(conn->systemMessageHandlers, 
				type, packetId, str, h);
}
/*
 * Internal routines to remove handler from the specified
 * message handler structure for an arbitrary message
 * type
 */
static DXLError
_dxl_RemoveUserHandler(DXLConnection *conn,
		int type, const char *str, 
		DXLMessageHandler h)
{
    return _dxl_RemoveHandler(conn->userMessageHandlers, type, -1, str, h);
}

/*
 * Remove a user handler for an arbitrary message type. Removes 
 * the handler from the user message handler structure.
 */
DXLError
DXLRemoveMessageHandler(DXLConnection *conn, DXLPacketTypeEnum type, 
			const char *str, DXLMessageHandler h)
{
   return _dxl_RemoveUserHandler(conn, type, str, h);
}

/* 
 * Handle incoming messages in the given message handler.
 * Return 1 if a handler was found, 0 otherwise.
 */
static int
_dxl_HandleMessage_1(DXLConnection *conn, HandlerEntry *msgHandler,
		int type, int packetId, const char *m)
{
    HandlerEntry best = NULL;

    if (! m)
    {
	best = msgHandler[type];
	
	while (best && best->operand)
	    best = best->next;
    }
    else
    {
	int maxlen = -1;
	HandlerEntry he;

	for (he = msgHandler[type]; he; he = he->next)
	{
	    int oplen = he->operandLength;

	    if ((oplen > maxlen) && match_packet(he,packetId,m,1))
	    {
		maxlen = oplen;
		best = he;
	    }
	}
    }

    if (best) {
	(*best->handler)(conn, m, (void*)best->data);
	return 1;
    } else
	return 0;
}

/*
 * Call made to handle incoming messages.  Calls appropriate 
 * user and system message handlers.
 */

DXLError
_dxl_HandleMessage(DXLConnection *conn, int type, int packetId, const char *m)
{
    const char *matchString = m;

    /*
     * Skip past the colon and the white spaces.
     * (i.e. '0: <match string>...' )
     * Actually, I'm not sure this is the best place to do this, but
     * _dxl_ReadFromSocket() or DispatchEvent() didn't seem any better.
     */
    if (type != 9)
    {
	while(*matchString == ' ')
	    ++matchString;
	while(isdigit(*matchString))
	    ++matchString;
	if (*matchString == ':')
	    ++matchString;
	while(*matchString == ' ')
	    ++matchString;
    }

    _dxl_HandleMessage_1(conn, conn->userMessageHandlers, type, 
				packetId, matchString);
    _dxl_HandleMessage_1(conn, conn->systemMessageHandlers, type, 
				packetId, matchString);
    return OK;
}

DXLError
_DXLError(DXLConnection *conn, const char *message)
{
    return _dxl_HandleMessage(conn, PACK_ERROR, -1, message);
}

void 
_DXLDefaultHandler(DXLConnection *conn, const char *message, void *d)
{
    printf("DXLink Error: %s\n", message);
}
