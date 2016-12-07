/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"

#include "DXApplication.h"
#include "ErrorDialogManager.h"
#include "Command.h"
#include "DXExecCtl.h"
#include "ListIterator.h"
#include "Network.h"
#include "Node.h"
#include "ApplicIF.h"
#include "DXLinkHandler.h"

#include <ctype.h>
#include <stdarg.h>


ApplicIF::ApplicIF(const char *server, int port, int local)
                        : PacketIF(server, port, local, FALSE) 
{
    DXPacketIF *pif = theDXApplication->getPacketIF();
    if (pif)
        this->handleServerConnection();
}

void ApplicIF::initializePacketIO()
{
    this->setHandler(DXPacketIF::MESSAGE,
		  ApplicIF::ApplicationMessage,
		  (void*)this);
    this->setHandler(DXPacketIF::INFORMATION,
		  ApplicIF::ApplicationInformation,
		  (void*)this);
    this->setHandler(DXPacketIF::PKTERROR,
		  ApplicIF::ApplicationError,
		  (void*)this);
    this->setHandler(DXPacketIF::COMPLETE,
		  ApplicIF::ApplicationCompletion,
		  (void*)this);
    this->setHandler(DXPacketIF::FOREGROUND,
		  ApplicIF::ApplicationForeground,
		  (void*)this);
    this->setHandler(DXPacketIF::LINQUIRY,
		  ApplicIF::ApplicationQuery,
		  (void*)this);
    this->setHandler(DXPacketIF::LINK,
		  ApplicationLink,
		  (void*)this);
    this->setErrorCallback(ApplicIF::HandleApplicationError,
			   (void*)theDXApplication);
}

void ApplicIF::ApplicationMessage(void *clientData, int id, void *line)
{
    //char *l = (char *)line;
}
void ApplicIF::ApplicationInformation(void *clientData, int id, void *line)
{
    //char *l = (char *)line;
}
void ApplicIF::ApplicationError(void *clientData, int id, void *line)
{
    //char *l = (char *)line;
}
void ApplicIF::ApplicationCompletion(void *clientData, int id, void *line)
{
    //char *l = (char *)line;
}
void ApplicIF::ApplicationQuery(void *clientData, int id, void *line)
{
    //char *l = (char *)line;
}

void ApplicIF::PassThruDXL(void *clientData, int id, void *line)
{
    ApplicIF *ai = (ApplicIF*)clientData;
    char *msg = (char*)line;

    ai->sendPacket(PacketIF::LINK, 0, msg);
}

void ApplicIF::HandleApplicationError(void *clientData, char *message)
{
    DXApplication *app = (DXApplication*)clientData;

    if (app)
    {
	ErrorMessage(message);
	app->disconnectFromApplication(TRUE);
    }
}

void ApplicIF::ApplicationForeground(void *clientData, int id, void *line)
{
    //char *l = (char *)line;
}

void ApplicIF::ApplicationLink(void *clientData, int id, void *p)
{
    ApplicIF *app_if = (ApplicIF *)clientData;

    app_if->executeLinkCommand((const char *)p, id);
}

void ApplicIF::installLinkHandler()
{
    this->linkHandler = new DXLinkHandler(this);
}


void ApplicIF::parsePacket()
{
    int i;

    for (i = 0; this->line[i] && isspace(this->line[i]); i++);
    if (this->line[i] == '$') {
	void *data;
	PacketIFCallback cb = this->getEchoCallback(&data);
	if (cb) {
	    char *s = new char[STRLEN(line) + 50];
	    sprintf(s, "Received %s\n",&this->line[i]);
	    (*cb)(data, s);
	    delete s;
	}
        this->executeLinkCommand(this->line+i, 0);
    } else
        this->PacketIF::parsePacket();
}

//
// Do what ever is necessary when a connection to the server
// (theDXApplication->getPacketIF()) has been established.
//
void ApplicIF::handleServerConnection()
{

    DXPacketIF *pif = theDXApplication->getPacketIF();
    if (pif)  {
        pif->setHandler(PacketIF::LINK,
                        ApplicIF::PassThruDXL,
                        (void*)this,
                        "LINK:  DXLOutput ");
    }

}

void ApplicIF::send(int type, const char *data)
{
    this->sendPacket(type, 0, data, strlen(data));
}


