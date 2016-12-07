/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include <string.h>

#include "EchoNode.h"
#include "DXApplication.h"
#include "MsgWin.h"
static char *message_token = "ECHO";

EchoNode::EchoNode(NodeDefinition *nd, Network *net, int instnc) :
    ModuleMessagingNode(nd, net, instnc)
{
}

EchoNode::~EchoNode()
{
}

//
// Called when a message is received from the executive after
// this->ExecModuleMessageHandler() is registered in
// this->Node::netPrintNode() to receive messages for this node.
// The format of the message coming back is defined by the derived class.
//
void EchoNode::execModuleMessageHandler(int id, const char *line)
{
    char *p = (char *) strstr(line,message_token);

    if (p) {
        MsgWin *mw = theDXApplication->getMessageWindow();
	if (theDXApplication->doesInfoOpenMessage(TRUE))
	    mw->infoOpen();
	mw->addInformation(p);
    }
}

//
// Returns a string that is used to register
// this->ExecModuleMessageHandler() when this->hasModuleMessageProtocol()
// return TRUE.  This version, returns an id that is unique to this
// instance of this node.
//
const char *EchoNode::getModuleMessageIdString()
{
     if (!this->moduleMessageId)
	this->moduleMessageId = DuplicateString(message_token);
     return (const char*) this->moduleMessageId;
}


//
// Determine if this node is of the given class.
//
boolean EchoNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassEchoNode);
    if (s == classname)
	return TRUE;
    else
	return this->ModuleMessagingNode::isA(classname);
}
