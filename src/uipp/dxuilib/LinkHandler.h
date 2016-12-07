/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#ifndef _LinkHandler_h
#define _LinkHandler_h

#include "Base.h"
#include "Command.h"
#include "../base/CmdEntry.h"

//
// Class name definition:
//
#define ClassLinkHandler "LinkHandler"

class PacketIF;
class CmdEntry;
//typedef boolean (*CmdEntryFunction)(const char *, int, void *);

class LinkHandler : public Base
{
    private:

	CmdEntry   *commandEntry;
	PacketIF   *packetIF;

    public:

	//
	// Just call packetIF->sendPacket().
	//
	void sendPacket(int packetType, int packetId, 
			const char *data = NULL, int length = 0);
    
    public:

	LinkHandler(PacketIF *);
	~LinkHandler();
    
	void addCommand(const char *, CmdEntryFunction f);
	void addCommand(const char *, Command *);

	void addSubCommand(const char *, const char *, CmdEntryFunction f);
	void addSubCommand(const char *, const char *, Command *);
	
	boolean executeLinkCommand(const char *, int);

	PacketIF *getPacketIF() {return this->packetIF;}

	//
	// Returns a pointer to the class name.
	//
	const char* getClassName()
	{
	    return ClassLinkHandler;
	}

};


#endif
    





