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

#include <dx/dx.h>


typedef struct 
{
    int  delete;
    char *message;
    char *messageType;
    char *major;
    char *minor;
} PendingDXLMessage;

static Error
SendPendingDXLMessage(Private P)
{
    PendingDXLMessage *p = (PendingDXLMessage *)DXGetPrivateData(P);

    DXUIMessage(p->messageType, p->message);

    if (p->delete)
	if (! DXSetPendingCmd(p->major, p->minor, NULL, NULL))
	{
	    DXSetError(ERROR_INTERNAL, "error return from DXSetPendingCmd");
	    return ERROR;
	}

    return OK;
}

static Error
delete_PendingDXLMessage(Pointer d)
{
    PendingDXLMessage *p = (PendingDXLMessage *)d;
    DXFree(p->message);
    DXFree(p->messageType);
    DXFree(p->major);
    DXFree(p->minor);
    DXFree(p);

    return OK;
}
	
Error
m_DXLMessage(Object *in, Object *out)
{
    char *message;
    char *messageType;
    char *major = NULL;
    PendingDXLMessage *plr = NULL;
    Private p = NULL;
    int  pending;

    if (! in[0]) 
    {
	DXSetError(ERROR_MISSING_DATA, "#10000", "command");
	return ERROR;
    }

    if (!DXExtractString(in[0], &message))
    {
	DXSetError(ERROR_BAD_PARAMETER, "message must be string");
	goto error;
    }
    

    if (in[1])
    {
	if (!DXExtractInteger(in[1], &pending) ||
	     (pending != 0 && pending != 1 && pending != 2))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "pending must be 0, 1 or 2");
	    goto error;
	}
    }
    else
	pending = 1;
    
    if (in[2])
    {
	if (!DXExtractString(in[2], &messageType))
	{
	    DXSetError(ERROR_BAD_PARAMETER, "messageType must be string");
	    goto error;
	}
    }
    else
	messageType = "LINK";

    if (pending)
    {
	plr = (PendingDXLMessage *)DXAllocateZero(sizeof(PendingDXLMessage));
	if (! plr)
	    goto error;
    
	plr->message = (char *)DXAllocate(strlen(message)+1);
	if (! plr->message)
	    goto error;
    
	plr->messageType = (char *)DXAllocate(strlen(messageType)+1);
	if (! plr->messageType)
	    goto error;
    
	major = DXGetModuleId();
	if (! major)
	    goto error;

	plr->major = (char *)DXAllocate(strlen(major)+1);
	if (! plr->major)
	    goto error;
	
	p = DXNewPrivate((Pointer)plr, delete_PendingDXLMessage);
	if (! p)
	    goto error;
	
	strncpy(plr->message, message, strlen(message)+1);
	strncpy(plr->messageType, messageType, strlen(messageType)+1);
	strncpy(plr->major, major, strlen(major)+1);

	DXFreeModuleId(major);
	major = NULL;

	if (pending == 2)
	    plr->delete = 0;
	else
	    plr->delete = 1;

	if (! DXSetPendingCmd(plr->major, NULL, SendPendingDXLMessage, p))
	    goto error;
    }
    else
	DXUIMessage(messageType, message);

    return OK;

error:
    if (p)
	DXDelete((Object)p);
    else if (plr)
    {
	DXFree(plr->major);
	DXFree(plr->message);
	DXFree(plr->messageType);
	DXFree(plr);
    }

    if (major)
	DXFreeModuleId(major);
    
    return ERROR;
}
