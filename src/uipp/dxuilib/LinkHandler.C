/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "ErrorDialogManager.h"
#include "LinkHandler.h"
#include "Dictionary.h"
#include "CmdEntry.h"
#include "PacketIF.h"
#include <ctype.h>
#include <stdarg.h>

LinkHandler::LinkHandler(PacketIF *packetIF)
{
    this->packetIF = packetIF;
    Dictionary *dict = new Dictionary();
    this->commandEntry = new CmdEntry(dict, (void *)this);
}

LinkHandler::~LinkHandler()
{
    delete this->commandEntry;
}

void LinkHandler::addCommand(const char *command, CmdEntryFunction func)
{
    CmdEntry *cmd;
    Dictionary *dict = this->commandEntry->getDictionary();

    if (func == NULL)
    {
	Dictionary *d = new Dictionary;
	cmd = new CmdEntry(d, (void *)this);
    }
    else
    {
	cmd = new CmdEntry(func, (void *)this);
    }

    dict->addDefinition(command, (void*)cmd);
}

void LinkHandler::addSubCommand(const char *command,
		const char *subcommand, CmdEntryFunction func)
{
    Dictionary *dict = this->commandEntry->getDictionary();
    Dictionary *subdict;
    CmdEntry *oe = (CmdEntry *)dict->findDefinition(command);
    CmdEntry *ne;

    if (! oe)
    {
	fprintf(stderr, "adding subcommand (%s) to nonexistent command (%s)\n",
		subcommand, command);
    }

    subdict = oe->getDictionary();
    if (! subdict)
    {
	fprintf(stderr, "adding subcommand (%s) to bad command (%s)\n",
		subcommand, command);
    }
	
    ne = new CmdEntry(func, (void *)this);
    subdict->addDefinition(subcommand, (void*)ne);
}

void LinkHandler::addCommand(const char *command,
		Command *func)
{
    CmdEntry *cmd;
    Dictionary *dict = this->commandEntry->getDictionary();

    if (func == NULL)
    {
	Dictionary *d = new Dictionary;
	cmd = new CmdEntry(d, (void *)this);
    }
    else
    {
	cmd = new CmdEntry(func);
    }

    dict->addDefinition(command, (void*)cmd);
}

void LinkHandler::addSubCommand(const char *command,
		const char *subcommand,
		Command *func)
{
    Dictionary *dict = this->commandEntry->getDictionary();
    Dictionary *subdict;
    CmdEntry *oe = (CmdEntry *)dict->findDefinition(command);
    CmdEntry *ne;

    if (! oe)
    {
	fprintf(stderr, "adding subcommand (%s) to nonexistent command (%s)\n",
		subcommand, command);
    }

    subdict = oe->getDictionary();
    if (! subdict)
    {
	fprintf(stderr, "adding subcommand (%s) to bad command (%s)\n",
		subcommand, command);
    }
	
    ne = new CmdEntry(func);
    subdict->addDefinition(subcommand, (void*)ne);
}

boolean LinkHandler::executeLinkCommand(const char *commandString, int id)
{
    return this->commandEntry->parse(commandString, id);
}

//
// We implement this so that 
//	1) LinkHandlers can send packets, and
//	2) LinkHandlers don't have to be complete friends of PacketIF
//
void LinkHandler::sendPacket(int packetType, int packetid, 
			const char *data, int length)
{
    this->getPacketIF()->sendPacket(packetType, packetid, data,length);
}

