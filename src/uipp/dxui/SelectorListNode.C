/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"




#include "SelectorListNode.h"
#include "SelectorListInstance.h"


SelectorListNode::SelectorListNode(NodeDefinition *nd, 
			Network *net, int instnc) : 
				SelectionNode(nd, net, instnc,TRUE) 
{ 
}
SelectorListNode::~SelectorListNode()
{
}
//
// Get a a SelectorListInstance instead of an InteractorInstance.
//
InteractorInstance *SelectorListNode::newInteractorInstance()
{
    SelectorListInstance *si = new SelectorListInstance(this);
    return (SelectorListInstance*)si;
}

//
// Determine if this node is of the given class.
//
boolean SelectorListNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassSelectorListNode);
    if (s == classname)
	return TRUE;
    else
	return this->InteractorNode::isA(classname);
}
//
// These define the initial values
//
const char *SelectorListNode::getInitialValueList()
{
    return "{ 0 1 2}";
}
const char *SelectorListNode::getInitialStringList()
{
    return "{ \"0\" \"1\" \"2\" }";
}

