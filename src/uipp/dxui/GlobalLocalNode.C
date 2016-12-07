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

#include "GlobalLocalNode.h"
#include "StandIn.h"
#include "Network.h"
#include "Parameter.h"

GlobalLocalNode::GlobalLocalNode(NodeDefinition *nd, Network *net, int instnc) : 
					Node(nd, net, instnc)
{
    this->isGlobal = UNDEFINED; 
    this->myNodeNameSymbol = 0;
}

GlobalLocalNode::~GlobalLocalNode()
{
    this->clearMyNodeName();
}
void GlobalLocalNode::setAsLocalNode() 
{ 
    this->isGlobal = FALSE; 
    this->clearMyNodeName();
    if (this->getStandIn())
	this->getStandIn()->notifyLabelChange();
    this->markForResend();
}
void GlobalLocalNode::setAsGlobalNode() 
{ 
    this->isGlobal = TRUE; 
    this->clearMyNodeName(); 
    if (this->getStandIn())
	this->getStandIn()->notifyLabelChange();
    this->markForResend();
} 
boolean GlobalLocalNode::isLocalNode() { return this->isGlobal == FALSE; }
boolean GlobalLocalNode::isGlobalNode() { return this->isGlobal == TRUE; }

//
// Mark the net dirty so that it will a) be sent prior to the next execution,
// b) produce a "want to save?" prompt prior to deletion.  Mark inputs dirty
// so that they'll be resent, causing the module to run with the correct inputs.
// Otherwise the dxexec has uninitialized values for the modules outputs.
//
void GlobalLocalNode::markForResend()
{
    this->getNetwork()->setFileDirty();
    this->getNetwork()->setDirty();
    int cnt = this->getInputCount();
    int i;
    for (i=1; i<=cnt; i++) {
	Parameter* prm = this->getInputParameter(i);
	if ((prm->isConnected() == FALSE) && (prm->isDefaulting() == FALSE))
	    prm->setDirty();
    }
}

void GlobalLocalNode::clearMyNodeName()
{
    this->myNodeNameSymbol = 0;

}
void GlobalLocalNode::setMyNodeNameIfNecessary() 
{

    // FIXME: should change Symbol for the name

    const char *suffix = NULL;
    if (this->isGlobalNode())
	suffix = "Global";	
    else if (this->isLocalNode())
	suffix = "Local";	

    if (suffix) {
	char newname[1024];
	Symbol nameSym = this->Node::getNameSymbol(); 
	const char *name = theSymbolManager->getSymbolString(nameSym); 
	SPRINTF(newname,"%s%s",name,suffix);	
	this->myNodeNameSymbol = theSymbolManager->registerSymbol(newname);

	//
	// This resets the text in the notation field in the cdb
	//
	this->setLabelString(newname);
    } else {
	this->clearMyNodeName();
    }

}
Symbol GlobalLocalNode::getNameSymbol() 
{ 
    if (this->myNodeNameSymbol)
	return this->myNodeNameSymbol;

    this->setMyNodeNameIfNecessary();

    if (this->myNodeNameSymbol)
	return this->myNodeNameSymbol;
    else
	return this->Node::getNameSymbol(); 

    
} 

//
// Determine if this node is of the given class.
//
boolean GlobalLocalNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassGlobalLocalNode);
    if (s == classname)
	return TRUE;
    else
	return this->Node::isA(classname);
}

const char *GlobalLocalNode::getExecModuleNameString()
{
    Symbol s = this->getNameSymbol();
    return theSymbolManager->getSymbolString(s);
}
