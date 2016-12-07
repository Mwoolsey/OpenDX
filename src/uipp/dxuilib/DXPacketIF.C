/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"





#include "DXStrings.h"
#include "DXApplication.h"
#include "DXLinkHandler.h"
#include "DXPacketIF.h"
#include "MsgWin.h"
#include "InfoDialogManager.h"
#include "ErrorDialogManager.h"
#include "WarningDialogManager.h"
#include "Network.h"
#include "EditorWindow.h"

#include <stdarg.h>
#include <ctype.h>
#include <errno.h>

DXPacketIF::DXPacketIF(const char *server, int port, int local) 
			: PacketIF(server, port, local, TRUE) 
{
    this->id = 0;
}

void DXPacketIF::initializePacketIO()
{
    this->setHandler(DXPacketIF::MESSAGE,
		    DXPacketIF::DXProcessMessage,
		    (void*)this);
    this->setHandler(DXPacketIF::INFORMATION,
		    DXPacketIF::DXProcessInformation,
		    (void*)this);
    this->setHandler(DXPacketIF::INTERRUPT,
		    DXPacketIF::DXProcessInterrupt,
		    (void*)this);
    this->setHandler(DXPacketIF::PKTERROR,
		    DXPacketIF::DXProcessError,
		    (void*)this);
    this->setHandler(DXPacketIF::PKTERROR,
		    DXPacketIF::DXProcessErrorWARNING,
		    (void*)this,
		    "WARNING");
    this->setHandler(DXPacketIF::INTERRUPT,
		    DXPacketIF::DXProcessBeginExecNode,
		    (void*)this,
		    "begin ");
    this->setHandler(DXPacketIF::INTERRUPT,
		    DXPacketIF::DXProcessEndExecNode,
		    (void*)this,
		    "end ");
    this->setHandler(DXPacketIF::PKTERROR,
                    DXPacketIF::DXProcessErrorERROR,
                    (void*)this,
                    "ERROR");
    this->setHandler(DXPacketIF::LINK,
                    DXPacketIF::DXProcessLinkCommand,
                    (void*)this);
    this->setErrorCallback(DXPacketIF::DXHandleServerError,
		    (void*)this);
}

void DXPacketIF::installLinkHandler()
{
    this->linkHandler = new DXLinkHandler(this);
}

void DXPacketIF::DXProcessMessage(void *, int, void *p)
{
    char *line = (char *)p;
    char buffer[512];

    if (FindDelimitedString(line,':',':',buffer) && strstr(buffer,"POPUP")) {
	char *c = strchr(line,':');	
	if (c && (c = strchr(c+1,':')))
	    InfoMessage(c+1);	
    }


    theDXApplication->getMessageWindow()->addInformation(line);
}

void DXPacketIF::DXProcessInformation(void *, int, void *p)
{
    char *line = (char *)p;

    theDXApplication->getMessageWindow()->addInformation(line);
}

void DXPacketIF::DXProcessError(void *, int, void *p)
{
    char *line = (char *)p;

    theDXApplication->getMessageWindow()->addError(line);
}

void DXPacketIF::DXProcessErrorERROR(void *clientData, int id, void *p)
{
    char *line = (char *)p;
    char buffer[512];

    if (FindDelimitedString(line,':',':',buffer) && strstr(buffer,"POPUP")) {
	char *c = strchr(line,':');
	if (c && (c = strchr(c+1,':')))
	    ErrorMessage(c+1);
    }

    theDXApplication->addErrorList(line);

    theDXApplication->messageWindow->addError(line);
}

void DXPacketIF::DXProcessErrorWARNING(void *, int, void *p)
{
	char *line = (char *)p, *s;
	char buffer[512];

	if (FindDelimitedString(line,':',':',buffer) && strstr(buffer,"POPUP")) {
		char *c = strchr(line,':');	
		if (c && (c = strchr(c+1,':')))
			WarningMessage(c+1);	
	}

	if (FindDelimitedString(line,':',':',buffer) && strstr(buffer,"MSGERRUP")) {
		//
		// These messages are warnings that we want to treat as
		// an error message.
		//
		if (theDXApplication->doesErrorOpenMessage() && 
			!theDXApplication->messageWindow->isManaged())
			theDXApplication->messageWindow->manage();
		//
		// Remove the MSGERRUP characters from the message.
		//
		s = strstr(line,"MSGERRUP");
		char *c = s + strlen("MSGERRUP");
		while (*c) {
			*s = *c;
			s++; c++;
		}
		*s = '\0';
	}

	theDXApplication->getMessageWindow()->addWarning(line);
}

void DXPacketIF::DXProcessCompletion(void *, int , void *)
{
}

void DXPacketIF::DXProcessInterrupt(void *clientData, int id, void *p)
{
    char *line = (char *)p;
    fprintf(stderr, "DX Interrupt(%d): %s\n", id, line);
}

void DXPacketIF::DXProcessBeginExecNode(void *clientData, int id, void *p)
{
    char *line = (char *)p;

    line = strchr(line, '/');
    ASSERT(line); 
    line++;
    const char *name = theDXApplication->network->getNameString();
    if (!EqualSubstring(line,name,STRLEN(name)))
	return;
    line = strchr(line, '/');
    ASSERT(line);
    line++;

    theDXApplication->highlightNodes(line,EditorWindow::EXECUTEHIGHLIGHT);
}

void DXPacketIF::DXProcessEndExecNode(void *clientData, int id, void *p)
{
    char *line = (char *)p;

    line = strchr(line, '/');
    ASSERT(line);
    line++;
    const char *name = theDXApplication->network->getNameString();
    if (!EqualSubstring(line,name,STRLEN(name)))
	return;
    line = strchr(line, '/');
    ASSERT(line);
    line++;

    theDXApplication->highlightNodes(line,EditorWindow::REMOVEHIGHLIGHT);
}

void DXPacketIF::DXProcessLinkCommand(void *clientData, int id, void *p)
{
    DXPacketIF *pif = (DXPacketIF *)clientData;

    /*
     * Skip first two comma-separated args (processor num and LINK)
     */
    int i;
    const char *c;

    for (i = 0, c = (const char *)p; i < 2; c++)
        if (*c == ':') i++;

    while (isspace(*c))
        c++;

    if (! pif->linkHandler)
	pif->installLinkHandler();

    pif->executeLinkCommand((const char *)c, id);
}

void DXPacketIF::DXHandleServerError(void *clientData, char *message)
{
    DXPacketIF *pif = (DXPacketIF *)clientData;

    /*
     * Reset the internal application state.
     */
    theDXApplication->packetIFCancel(pif);

    /*
     * Report the disconnection in the anchor window.
     */
    ErrorMessage(message);
}


/*****************************************************************************/
/* uipPacketSend -                                                           */
/*                                                                           */
/*                                                                           */
/*****************************************************************************/

int DXPacketIF::send(int                   type,
                   const char*           data,
                   PacketHandlerCallback callback,
                   void*                 clientData)
{
    if (!this->getFILE())
        return 0;

    this->id++;

    if (callback)
    {
        this->setWaiter(PacketIF::COMPLETE, this->id, callback, clientData);
    }

    switch(type)
    {
      case PacketIF::INTERRUPT:
        this->sendPacket
            (type,
	     this->id,
             "0",
             1);
        break;

      case PacketIF::MACRODEF:
        break;

      default:
      case PacketIF::SYSTEM:
      case PacketIF::FOREGROUND:
      case PacketIF::BACKGROUND:
      case PacketIF::LINQUIRY:
      case PacketIF::SINQUIRY:
      case PacketIF::VINQUIRY:
      case PacketIF::IMPORT:
        this->sendPacket
            (type,
	     this->id,
             data,
             data == NULL? 0: STRLEN(data));
        break;
    }

    return this->id;
}

int DXPacketIF::sendFormat(const int type, const char *fmt, ...)
{
    va_list ap;
    char buffer[1024];  // FIXME: how to allocate this

    va_start(ap, fmt);

    vsprintf(buffer,(char*)fmt,ap);

    int result = this->send(type,buffer);

    va_end(ap);

    return result;
}

int DXPacketIF::sendMacroStart
                     (PacketHandlerCallback callback,
                      void*                 clientData)
{
    if (!this->getFILE())
    {
        return 0;
    }

    this->id++;

    if (callback)
    {
        this->setWaiter(PacketIF::COMPLETE, this->id, callback, clientData);
    }

    char *tempbuf = new char [64];
    int l = sprintf
            (tempbuf,
             "|%d|%s|%d|",
             this->id,
             PacketIF::PacketTypes[PacketIF::MACRODEF],
             0);

    //	if (UxSend (this->socket, tempbuf, l, 0) == -1 )
        //	this->handleStreamError(errno,"PacketIF::sendMacroStart");
    this->sendImmediate (tempbuf);
    delete tempbuf;

    return this->id;
}

void DXPacketIF::sendMacroEnd()
{
    if (!this->getFILE()) return;
    this->sendImmediate("|\n");
}

void
DXPacketIF::setWaiter(int                     type,
                    int                     number,
                    PacketHandlerCallback   callback,
                    void *                  clientData)
{
    PacketHandler* p;

    p = new PacketHandler(FALSE, type, number, callback, clientData);
    this->handlers.insertElement((void*)p, 1);
}

