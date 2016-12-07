/***********************************************************************/
/* Open Visualization Data Explorer                                    */
/* (C) Copyright IBM Corp. 1989,1999                                   */
/* ALL RIGHTS RESERVED                                                 */
/* This code licensed under the                                        */
/*    "IBM PUBLIC LICENSE - Open Visualization Data Explorer"          */
/***********************************************************************/

#include <dxconfig.h>
#include "../base/defines.h"


#include "StreaklineNode.h"

#define NAME_NUM 1

StreaklineNode::StreaklineNode(NodeDefinition *nd, Network *net, int instnc) :
    Node(nd, net, instnc)
{
    this->setInputValue(NAME_NUM,this->getModuleMessageIdString(),
				DXType::StringType,FALSE);
}

StreaklineNode::~StreaklineNode()
{
}

//
// Determine if this node is of the given class.
//
boolean StreaklineNode::isA(Symbol classname)
{
    Symbol s = theSymbolManager->registerSymbol(ClassStreaklineNode);
    if (s == classname)
        return TRUE;
    else
        return this->Node::isA(classname);
}

int StreaklineNode::assignNewInstanceNumber()
{
    boolean isConnected = this->isInputConnected(NAME_NUM);
    int instance_number = this->Node::assignNewInstanceNumber();

    if (!isConnected)
	this->setInputValue(NAME_NUM,this->getModuleMessageIdString(), 
	    DXType::StringType,FALSE);

    return instance_number;
}

